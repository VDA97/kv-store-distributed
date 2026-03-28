#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <fstream>
#include <atomic>
#include <chrono> // <--- Adicionado para ler o relógio do sistema

namespace kv_store::storage
{

    // Estrutura que envelopa o valor e seu tempo de vida
    struct TableEntry
    {
        std::string value;
        int64_t expires_at; // Timestamp Unix em milissegundos (0 = eterno)
    };

    class HashTable
    {
    public:
        HashTable() = default;

        // Modificamos o SET para receber os segundos de TTL (padrão 0 = eterno)
        void set(const std::string &key, const std::string &value, int64_t ttl_seconds = 0);
        std::optional<std::string> get(const std::string &key); // Retirado o 'const' pois o GET pode deletar chaves expiradas!
        bool remove(const std::string &key);

        bool save_to_file(const std::string &filename) const;
        bool load_from_file(const std::string &filename);

        bool needs_save() const;
        void reset_dirty();
        size_t cleanup_expired_keys();

    private:
        // O mapa agora guarda a struct TableEntry em vez de apenas std::string
        std::unordered_map<std::string, TableEntry> data_;
        mutable std::shared_mutex mutex_;
        std::atomic<bool> is_dirty_{false};

        // Função auxiliar para pegar o tempo atual em milissegundos
        int64_t get_current_time_ms() const;
    };

} // namespace kv_store::storage