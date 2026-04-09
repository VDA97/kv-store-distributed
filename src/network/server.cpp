#include "network/server.hpp"
#include "network/session.hpp" // Precisaremos desse novo arquivo
#include "utils/logger.hpp"
#include "utils/metrics.hpp"

namespace kv_store::network
{

    Server::Server(short port, storage::HashTable &storage, const std::string &dump_file, ReplicationConfig config)
    :   acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)),
        storage_(storage),
        dump_file_(dump_file),
        timer_(io_context_),
        config_(config)
{
    LOG_INFO("Server initialized on port {} in {} mode.", port,
             (config_.mode == ServerMode::LEADER ? "LEADER" : "FOLLOWER"));
    start_periodic_save();
    do_accept(); // Inicia o primeiro "escritor" de aceitação
}

void Server::start_periodic_save()
{
    timer_.expires_after(std::chrono::seconds(save_interval_seconds_));
    timer_.async_wait([this](const std::error_code &ec)
                      {
        if (!ec) {
            storage_.cleanup_expired_keys();
            LOG_INFO("--- SYSTEM METRICS ---");
            LOG_INFO("Active Connections: {}", utils::Metrics::active_connections.load());
            LOG_INFO("Total Requests: {}", utils::Metrics::total_requests.load());
            LOG_INFO("SET Operations: {}", utils::Metrics::total_set_ops.load());
            LOG_INFO("GET Operations: {}", utils::Metrics::total_get_ops.load());
            LOG_INFO("----------------------");
            // SÓ SALVA SE HOUVE ALTERAÇÃO
            if (storage_.needs_save()) {
                LOG_INFO("Periodic Save: Changes detected. Saving snapshot...");
                storage_.save_to_file(dump_file_);
                storage_.reset_dirty(); 
            } else {
                // Log opcional de debug só para saber que o timer rodou
                LOG_DEBUG("Periodic Save: No changes detected. Skipping...");
            }
            start_periodic_save();
        } });
}

void Server::run()
{
    LOG_INFO("Asynchronous KV-Store server running...");
    // O io_context::run() bloqueia a thread principal e processa todos os eventos
    io_context_.run();
}

void Server::do_accept()
{
    // Prepara um socket vazio para a próxima conexão
    acceptor_.async_accept([this](std::error_code ec, tcp::socket socket)
                            {
    if (!ec) {
        // Cria a sessão e inicia. O std::make_shared é vital aqui.
        std::make_shared<Session>(std::move(socket), storage_, dump_file_, config_)->start();
    }
    // Chama novamente para continuar aceitando novos clientes
    do_accept(); });
}

} // namespace kv_store::network