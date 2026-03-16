#include "utils/metrics.hpp"

namespace kv_store::utils
{

    // Inicializando os contadores atômicos como zero
    std::atomic<uint64_t> Metrics::total_requests{0};
    std::atomic<uint64_t> Metrics::total_set_ops{0};
    std::atomic<uint64_t> Metrics::total_get_ops{0};
    std::atomic<uint64_t> Metrics::active_connections{0};

} // namespace kv_store::utils