#include <utility>
#include <thread>
#include <vector>
#include "network/server.hpp"
#include "storage/hash_table.hpp"
#include "utils/logger.hpp"

int main()
{
    kv_store::utils::init_logging();

    kv_store::storage::HashTable storage;
    const std::string dump_file = "storage_dump.kv";
    storage.load_from_file(dump_file);

    try
    {
        // 1. Criamos o servidor
        kv_store::network::Server server(8080, storage, dump_file);

        // 2. Definimos o número de threads (ex: capacidade do seu processador)
        unsigned int thread_count = std::thread::hardware_concurrency();
        if (thread_count == 0)
            thread_count = 2; // Fallback

        LOG_INFO("Starting server with {} threads...", thread_count);

        // 3. Criamos um pool de threads rodando o servidor
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < thread_count; ++i)
        {
            threads.emplace_back([&server]()
                                 {
                                     server.run(); // Cada thread entra no loop de eventos
                                 });
        }

        // 4. Aguardamos as threads (o servidor rodará até ser parado)
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