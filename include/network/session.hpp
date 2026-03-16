#pragma once
#include <asio.hpp>
#include <memory>
#include <string>
#include <vector>
#include "storage/hash_table.hpp"
#include "message.pb.h"

namespace kv_store::network
{
    using asio::ip::tcp;

class Session : public std::enable_shared_from_this<Session>
{
public:
    Session(tcp::socket socket, storage::HashTable &storage, const std::string &dump_file);
    ~Session();
    void start();

private:
    void do_read();
    void handle_request(const std::string &raw_data);
    void do_write(const std::string &response_data);

    tcp::socket socket_;
    storage::HashTable &storage_;
    std::string dump_file_;

    // --- Ajuste aqui ---
    // Buffer de leitura: Um array fixo é mais performático para o do_read assíncrono básico
    char data_buffer_[4096];

    // Buffer de escrita: Guardamos a string da última resposta aqui.
    // Isso garante que o buffer passado para async_write continue vivo
    // até que o callback de conclusão seja chamado.
    std::string write_buffer_;
};

} // namespace kv_store::network