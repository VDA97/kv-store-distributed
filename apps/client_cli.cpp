#include <utility>
#include <asio.hpp>
#include <thread>
#include <chrono>
#include "utils/logger.hpp"
#include "message.pb.h"

using asio::ip::tcp;

void send_request(tcp::socket &socket, const kv_store::network::KVRequest &req)
{
    std::string serialized;
    req.SerializeToString(&serialized);
    serialized += "\n";
    asio::write(socket, asio::buffer(serialized));
}

// Função auxiliar para conectar e buscar um dado (para não repetir código)
void fetch_data(const tcp::endpoint &endpoint, const std::string &key)
{
    asio::io_context io_context;
    tcp::socket socket(io_context);
    socket.connect(endpoint);

    kv_store::network::KVRequest get_req;
    get_req.set_type(kv_store::network::KVRequest::GET);
    get_req.set_key(key);

    send_request(socket, get_req);
    LOG_INFO("GET Request sent for key '{}'", key);

    asio::streambuf buf;
    asio::read_until(socket, buf, '\n');

    std::string raw_data;
    std::istream is(&buf);
    std::getline(is, raw_data);

    kv_store::network::KVResponse response;
    if (response.ParseFromString(raw_data))
    {
        LOG_INFO("--- Response for GET ---");
        LOG_INFO("Success: {}", response.success());
        LOG_INFO("Message: {}", response.message());
        LOG_INFO("Value:   {}", response.value());
        LOG_INFO("-------------------------");
    }
}

int main()
{
    kv_store::utils::init_logging();
    asio::io_context io_context;
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("127.0.0.1", "8080");
    auto target_endpoint = *endpoints.begin();

    try
    {
        // --- PASSO 1: ENVIAR O SET COM TTL DE 5 SEGUNDOS ---
        tcp::socket socket1(io_context);
        socket1.connect(target_endpoint);

        kv_store::network::KVRequest set_req;
        set_req.set_type(kv_store::network::KVRequest::SET);
        set_req.set_key("meu_nome");
        set_req.set_value("Victor Almeida");
        set_req.set_ttl_seconds(5); // <-- Expira em 5 segundos

        send_request(socket1, set_req);
        LOG_INFO("SET Request sent. TTL set to 5 seconds.");

        asio::streambuf buf1;
        asio::read_until(socket1, buf1, '\n');
        socket1.close();

        // --- PASSO 2: TESTE DO DADO VIVO (Dorme por 2 segundos) ---
        LOG_INFO("Waiting 2 seconds (Key should still be ALIVE)...");
        std::this_thread::sleep_for(std::chrono::seconds(2));
        fetch_data(target_endpoint, "meu_nome"); // Deve retornar o seu nome!

        // --- PASSO 3: TESTE DO DADO MORTO (Dorme mais 4 segundos, totalizando 6) ---
        LOG_INFO("Waiting 4 more seconds (Key should be EXPIRED)...");
        std::this_thread::sleep_for(std::chrono::seconds(4));
        fetch_data(target_endpoint, "meu_nome"); // Deve retornar False / Not Found!
    }
    catch (std::exception &e)
    {
        LOG_ERROR("Error: {}", e.what());
    }

    return 0;
}