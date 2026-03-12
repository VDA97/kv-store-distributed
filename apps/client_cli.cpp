#include <utility>
#include <asio.hpp>
#include "utils/logger.hpp"
#include "message.pb.h"

using asio::ip::tcp;

// Função auxiliar para não repetir código de envio/recebimento
void send_request(tcp::socket& socket, const kv_store::network::KVRequest& req) {
    std::string serialized;
    req.SerializeToString(&serialized);
    serialized += "\n";
    asio::write(socket, asio::buffer(serialized));
}

int main() {
    kv_store::utils::init_logging();
    asio::io_context io_context;
    tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("127.0.0.1", "8080");

    try {
        // --- PASSO 1: ENVIAR O SET ---
        tcp::socket socket1(io_context);
        asio::connect(socket1, endpoints);
        
        kv_store::network::KVRequest set_req;
        set_req.set_type(kv_store::network::KVRequest::SET);
        set_req.set_key("meu_nome");
        set_req.set_value("Victor Almeida");
        
        send_request(socket1, set_req);
        LOG_INFO("SET Request sent.");

        // Lendo resposta do SET
        asio::streambuf buf1;
        asio::read_until(socket1, buf1, '\n');
        socket1.close(); // Servidor fecha, nós fechamos aqui também.

        // --- PASSO 2: ENVIAR O GET (Nova Conexão) ---
        tcp::socket socket2(io_context);
        asio::connect(socket2, endpoints);

        kv_store::network::KVRequest get_req;
        get_req.set_type(kv_store::network::KVRequest::GET);
        get_req.set_key("meu_nome");

        send_request(socket2, get_req);
        LOG_INFO("GET Request sent for key 'meu_nome'");

        // Lendo resposta do GET
        asio::streambuf buf2;
        asio::read_until(socket2, buf2, '\n');
        
        std::string raw_data;
        std::istream is(&buf2);
        std::getline(is, raw_data);

        kv_store::network::KVResponse response;
        if (response.ParseFromString(raw_data)) {
            LOG_INFO("--- Response for GET ---");
            LOG_INFO("Success: {}", response.success());
            LOG_INFO("Message: {}", response.message());
            LOG_INFO("Value:   {}", response.value()); // AQUI DEVE APARECER SEU NOME!
            LOG_INFO("-------------------------");
        }
    }
    catch (std::exception& e) {
        LOG_ERROR("Error: {}", e.what());
    }

    return 0;
}