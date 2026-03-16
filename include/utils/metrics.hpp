#pragma once

#include <atomic>
#include <string>

namespace kv_store::utils
{

    class Metrics
    {
    public:
        // Usamos atômicos para garantir thread-safety sem travar o processamento
        static std::atomic<uint64_t> total_requests;
        static std::atomic<uint64_t> total_set_ops;
        static std::atomic<uint64_t> total_get_ops;
        static std::atomic<uint64_t> active_connections;

        static void reset()
        {
            total_requests = 0;
            total_set_ops = 0;
            total_get_ops = 0;
            active_connections = 0;
        }
    };

} // namespace kv_store::utils