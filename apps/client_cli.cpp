#include <utility>
#include <asio.hpp>
#include "utils/logger.hpp"

using asio::ip::tcp;

int main()
{
    // 1. Initialize the professional logging system
    kv_store::utils::init_logging();

    try
    {
        asio::io_context io_context;

        // 2. Resolve server address and establish connection
        tcp::resolver resolver(io_context);
        auto endpoints = resolver.resolve("127.0.0.1", "8080");

        tcp::socket socket(io_context);
        asio::connect(socket, endpoints);

        LOG_INFO("Successfully connected to the server!");

        // 3. Read the welcome message until the newline character
        asio::streambuf receive_buffer;
        asio::read_until(socket, receive_buffer, '\n');

        // Convert the buffer to a string to log it
        std::string message;
        std::istream is(&receive_buffer);
        std::getline(is, message);

        // 4. Log the received message
        LOG_INFO("Message received: {}", message);
    }
    catch (const std::exception &e)
    {
        // 5. Log connection or runtime errors
        LOG_ERROR("Client error: {}", e.what());
    }

    return 0;
}