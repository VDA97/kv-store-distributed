#include "storage/hash_table.hpp"
#include "utils/logger.hpp"

namespace kv_store::storage
{
    void HashTable::set(const std::string &key, const std::string &value)
    {
        std::unique_lock lock(mutex_);
        data_[key] = value;
        LOG_INFO("Storage: SET key '{}' with value size {}", key, value.size());
    }

    std::optional<std::string> HashTable::get(const std::string &key) const
    {
        std::shared_lock lock(mutex_);
        auto it = data_.find(key);
        if (it != data_.end())
        {
            LOG_DEBUG("Storage: GET key '{}' -> Found", key);
            return it->second;
        }
        LOG_DEBUG("Storage: GET key '{}' -> Not Found", key);
        return std::nullopt;
    }

    bool HashTable::remove(const std::string &key)
    {
        std::unique_lock lock(mutex_);
        if (data_.erase(key))
        {
            LOG_INFO("Storage: DELETE key '{}' -> Success", key);
            return true;
        }
        LOG_WARN("Storage: DELETE key '{}' -> Key not found", key);
        return false;
    }

    bool HashTable::save_to_file(const std::string &filename) const
    {
        std::shared_lock lock(mutex_); // Leitura segura para o snapshot
        std::ofstream file(filename);
        if (!file.is_open())
        {
            LOG_ERROR("Storage: Failed to open file '{}' for saving", filename);
            return false;
        }

        for (const auto &[key, value] : data_)
        {
            file << key << "=" << value << "\n";
        }
        LOG_INFO("Storage: Snapshot saved to '{}'", filename);
        return true;
    }

    bool HashTable::load_from_file(const std::string &filename)
    {
        std::unique_lock lock(mutex_); // Escrita para carregar os dados
        std::ifstream file(filename);
        if (!file.is_open())
        {
            LOG_WARN("Storage: No dump file found at '{}'. Starting fresh.", filename);
            return false;
        }

        data_.clear();
        std::string line;
        while (std::getline(file, line))
        {
            size_t pos = line.find('=');
            if (pos != std::string::npos)
            {
                data_[line.substr(0, pos)] = line.substr(pos + 1);
            }
        }
        LOG_INFO("Storage: Loaded {} keys from '{}'", data_.size(), filename);
        return true;
    }
} // namespace kv_store::storage