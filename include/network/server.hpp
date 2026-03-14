#pragma once
#include <asio.hpp>
#include <string>
#include "storage/hash_table.hpp"

namespace kv_store::network
{

    using asio::ip::tcp;

    class Server
    {
    public:
        Server(short port, storage::HashTable &storage, const std::string &dump_file);
        void run();

    private:
        void do_accept();

        asio::io_context io_context_;
        tcp::acceptor acceptor_;
        storage::HashTable &storage_;
        std::string dump_file_;
    };

} // namespace kv_store::network