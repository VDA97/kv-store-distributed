#include <utility>
#include <asio.hpp>
#include "utils/logger.hpp"
#include "message.pb.h" // Incluir o header gerado

using asio::ip::tcp;

int main()
{
    kv_store::utils::init_logging();

    try
    {
        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("127.0.0.1", "8080");

        tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        LOG_INFO("Successfully connected to the server!");

        // 1. Create a SET request
        kv_store::network::KVRequest request;
        request.set_type(kv_store::network::KVRequest::SET);
        request.set_key("meu_nome");
        request.set_value("Victor Almeida");

        // 2. Serialize and send to server
        std::string serialized_request;
        request.SerializeToString(&serialized_request);
        serialized_request += "\n"; // Delimiter for our simple protocol
        
        asio::write(socket, asio::buffer(serialized_request));
        LOG_INFO("Request sent: SET meu_nome='Victor Almeida'");
        
        // 1. Read the raw binary data until the newline
        asio::streambuf receive_buffer;
        asio::read_until(socket, receive_buffer, '\n');

        // 2. Extract the data from the buffer to a string
        std::string raw_data;
        std::istream is(&receive_buffer);
        std::getline(is, raw_data);

        // 3. Parse the binary data into a Protobuf object
        kv_store::network::KVResponse response;
        if (response.ParseFromString(raw_data))
        {
            // 4. Now we can access the fields professionally!
            LOG_INFO("--- Response Received ---");
            LOG_INFO("Success: {}", response.success());
            LOG_INFO("Message: {}", response.message());
            LOG_INFO("Value:   {}", response.value());
            LOG_INFO("-------------------------");
        }
        else
        {
            LOG_ERROR("Failed to parse Protobuf message!");
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Client error: {}", e.what());
    }

    return 0;
}