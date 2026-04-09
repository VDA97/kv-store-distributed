#include <utility>
#include "network/server.hpp"
#include "network/session.hpp" // Para acessar ReplicationConfig e ServerMode
#include "storage/hash_table.hpp"
#include "utils/logger.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <vector>

int main(int argc, char *argv[])
{
    kv_store::utils::init_logging();

    // 1. Configurações Iniciais
    uint16_t port = 8080;
    kv_store::network::ReplicationConfig config;
    const std::string dump_file = "storage_dump.kv";

    // 2. Parser de Argumentos (Integrando com sua necessidade de flexibilidade)
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "--port" && i + 1 < argc)
        {
            port = static_cast<uint16_t>(std::stoi(argv[++i]));
        }
        else if (arg == "--mode" && i + 1 < argc)
        {
            std::string mode = argv[++i];
            if (mode == "follower")
            {
                config.mode = kv_store::network::ServerMode::FOLLOWER;
            }
        }
        else if (arg == "--add-follower" && i + 1 < argc)
        {
            // Exemplo: --add-follower 127.0.0.1:8081
            std::string target = argv[++i];
            size_t colon_pos = target.find(':');
            if (colon_pos != std::string::npos)
            {
                std::string host = target.substr(0, colon_pos);
                uint16_t f_port = static_cast<uint16_t>(std::stoi(target.substr(colon_pos + 1)));
                config.followers.push_back({host, f_port});
            }
        }
    }

    kv_store::storage::HashTable storage;
    storage.load_from_file(dump_file);

    try
    {
        // 3. Criamos o servidor com a Configuração de Replicação
        // Note que agora passamos 'config' para o construtor do Server
        kv_store::network::Server server(port, storage, dump_file, config);

        std::string mode_str = (config.mode == kv_store::network::ServerMode::LEADER) ? "LEADER" : "FOLLOWER";
        LOG_INFO("Starting server in {} mode on port {}...", mode_str, port);

        // 4. Pool de Threads (Seu código original preservado)
        unsigned int thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0)
            thread_count = 2;

        LOG_INFO("Launching thread pool with {} worker threads...", thread_count);

        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < thread_count; ++i)
        {
            threads.emplace_back([&server]()
                                 {
                                     server.run(); // O io_context::run() agora é chamado aqui
                                 });
        }

        for (auto &t : threads)
        {
            if (t.joinable())
                t.join();
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Server failure: {}", e.what());
        return 1;
    }

    return 0;
}