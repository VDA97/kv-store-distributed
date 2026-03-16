#include "network/server.hpp"
#include "network/session.hpp" // Precisaremos desse novo arquivo
#include "utils/logger.hpp"

namespace kv_store::network
{

Server::Server(short port, storage::HashTable &storage, const std::string &dump_file)
    : acceptor_(io_context_, tcp::endpoint(tcp::v4(), port)),
        storage_(storage),
        dump_file_(dump_file),
        timer_(io_context_)
{
    start_periodic_save();
    do_accept(); // Inicia o primeiro "escritor" de aceitação
}

void Server::start_periodic_save()
{
    timer_.expires_after(std::chrono::seconds(save_interval_seconds_));
    timer_.async_wait([this](const std::error_code &ec)
                      {
        if (!ec) {
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
        std::make_shared<Session>(std::move(socket), storage_, dump_file_)->start();
    }
    // Chama novamente para continuar aceitando novos clientes
    do_accept(); });
}

} // namespace kv_store::network