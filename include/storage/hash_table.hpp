#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <fstream>
#include <atomic>

namespace kv_store::storage
{
    class HashTable
{
public:
    HashTable() = default;

    // Operações de Memória
    void set(const std::string &key, const std::string &value);
    std::optional<std::string> get(const std::string &key) const;
    bool remove(const std::string &key);

    // Operações de Persistência
    bool save_to_file(const std::string &filename) const;
    bool load_from_file(const std::string &filename);

    bool needs_save() const;
    void reset_dirty();

private:
    std::unordered_map<std::string, std::string> data_;
    mutable std::shared_mutex mutex_;
    std::atomic<bool> is_dirty_{false};
};
} // namespace kv_store::storage