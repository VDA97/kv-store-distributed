#pragma once

#include <string>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include "utils/logger.hpp"

namespace kv_store::storage
{

    class HashTable
    {
    public:
        HashTable() = default;

        /**
         * @brief Inserts or updates a key-value pair.
         * Uses unique_lock for exclusive access (Write).
         */
        void set(const std::string &key, const std::string &value)
        {
            std::unique_lock lock(mutex_);
            data_[key] = value;
            LOG_INFO("Storage: SET key '{}' with value size {}", key, value.size());
        }

        /**
         * @brief Retrieves a value associated with a key.
         * Uses shared_lock for concurrent access (Read).
         * @return std::optional containing the value if found.
         */
        std::optional<std::string> get(const std::string &key) const
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

        /**
         * @brief Removes a key from the storage.
         * @return true if the key existed and was removed.
         */
        bool remove(const std::string &key)
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

    private:
        std::unordered_map<std::string, std::string> data_;
        mutable std::shared_mutex mutex_;
    };

} // namespace kv_store::storage