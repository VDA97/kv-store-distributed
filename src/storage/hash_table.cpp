#include "storage/hash_table.hpp"
#include "utils/logger.hpp"
#include <iostream>

namespace kv_store::storage
{

    // Função auxiliar interna para pegar o Epoch Time em Milissegundos
    int64_t HashTable::get_current_time_ms() const
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
            .count();
    }

    void HashTable::set(const std::string &key, const std::string &value, int64_t ttl_seconds)
    {
        std::unique_lock lock(mutex_);

        int64_t expires_at = 0;
        if (ttl_seconds > 0)
        {
            expires_at = get_current_time_ms() + (ttl_seconds * 1000); // Converte segundos para ms
        }

        data_[key] = TableEntry{value, expires_at};
        is_dirty_ = true;

        if (ttl_seconds > 0)
        {
            LOG_INFO("Storage: SET key '{}' with TTL {}s", key, ttl_seconds);
        }
        else
        {
            LOG_INFO("Storage: SET key '{}' (Permanent)", key);
        }
    }

    // Retiramos o 'const' aqui porque o GET agora pode murchar e deletar a chave se expirada!
    std::optional<std::string> HashTable::get(const std::string &key)
    {
        // Usamos unique_lock porque se o dado expirou, precisamos de permissão de escrita para apagá-lo!
        std::unique_lock lock(mutex_);

        auto it = data_.find(key);
        if (it != data_.end())
        {
            // Verifica se expirou
            if (it->second.expires_at > 0 && get_current_time_ms() > it->second.expires_at)
            {
                LOG_INFO("Storage: GET key '{}' expired. Evicting.", key);
                data_.erase(it); // Limpeza passiva (deleta no momento que tenta ler)
                is_dirty_ = true;
                return std::nullopt;
            }

            LOG_DEBUG("Storage: GET key '{}' -> Found", key);
            return it->second.value; // Retorna a string pura para o usuário
        }

        LOG_DEBUG("Storage: GET key '{}' -> Not Found", key);
        return std::nullopt;
    }

    bool HashTable::remove(const std::string &key)
    {
        std::unique_lock lock(mutex_);
        if (data_.erase(key))
        {
            is_dirty_ = true;
            LOG_INFO("Storage: DELETE key '{}' -> Success", key);
            return true;
        }
        LOG_WARN("Storage: DELETE key '{}' -> Key not found", key);
        return false;
    }

    bool HashTable::save_to_file(const std::string &filename) const
    {
        std::shared_lock lock(mutex_);
        std::ofstream file(filename);
        if (!file.is_open())
        {
            LOG_ERROR("Storage: Failed to open file '{}' for saving", filename);
            return false;
        }

        int64_t now = get_current_time_ms();

        for (const auto &[key, entry] : data_)
        {
            // Se a chave expirou, nem salva ela no disco!
            if (entry.expires_at > 0 && now > entry.expires_at)
            {
                continue;
            }

            // Salva: chave=valor|expires_at
            file << key << "=" << entry.value << "|" << entry.expires_at << "\n";
        }
        LOG_INFO("Storage: Snapshot saved to '{}'", filename);
        return true;
    }

    bool HashTable::load_from_file(const std::string &filename)
    {
        std::unique_lock lock(mutex_);
        std::ifstream file(filename);
        if (!file.is_open())
        {
            LOG_WARN("Storage: No dump file found at '{}'. Starting fresh.", filename);
            return false;
        }

        data_.clear();
        std::string line;
        int64_t now = get_current_time_ms();

        while (std::getline(file, line))
        {
            size_t pos_eq = line.find('=');
            size_t pos_pipe = line.find('|');

            if (pos_eq != std::string::npos && pos_pipe != std::string::npos)
            {
                std::string key = line.substr(0, pos_eq);
                std::string value = line.substr(pos_eq + 1, pos_pipe - pos_eq - 1);
                int64_t expires_at = std::stoll(line.substr(pos_pipe + 1));

                // Se ao carregar a chave já está morta, descarta ela
                if (expires_at > 0 && now > expires_at)
                {
                    continue;
                }

                data_[key] = TableEntry{value, expires_at};
            }
        }
        LOG_INFO("Storage: Loaded {} keys from '{}'", data_.size(), filename);
        return true;
    }

    size_t HashTable::cleanup_expired_keys()
    {
        std::unique_lock lock(mutex_);
        int64_t now = get_current_time_ms();
        size_t initial_size = data_.size();

        // No C++20, isso substitui todo aquele loop de while/erase(it++)
        std::erase_if(data_, [now](const auto &item)
                      {
        const auto& [key, entry] = item;
        return entry.expires_at > 0 && now > entry.expires_at; });

        size_t removed = initial_size - data_.size();

        if (removed > 0)
        {
            is_dirty_ = true; // Avisa que a RAM mudou e o disco precisa atualizar
            LOG_INFO("Cleanup: Removed {} expired keys from memory using C++20 erase_if", removed);
        }

        return removed;
    }

    bool HashTable::needs_save() const { return is_dirty_; }
    void HashTable::reset_dirty() { is_dirty_ = false; }

} // namespace kv_store::storage