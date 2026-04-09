#pragma once
#include <asio.hpp>
#include <string>
#include "storage/hash_table.hpp"

namespace kv_store::network
{

using asio::ip::tcp;

enum class ServerMode
{
    LEADER,
    FOLLOWER
};

struct ReplicationConfig {
    ServerMode mode = ServerMode::LEADER;
    std::string master_host;
    uint16_t master_port;
    std::vector<std::pair<std::string, uint16_t>> followers;
};

class Server
{
public:
    Server(short port, storage::HashTable &storage, const std::string &dump_file, ReplicationConfig config);
    void run();

private:
    void do_accept();

    asio::io_context io_context_;
    tcp::acceptor acceptor_;
    storage::HashTable &storage_;
    std::string dump_file_;

    asio::steady_timer timer_;
    int save_interval_seconds_ = 10; // Intervalo de 10 segundos
    void start_periodic_save();      // Método para o loop do timer

    ReplicationConfig config_;
};

} // namespace kv_store::network