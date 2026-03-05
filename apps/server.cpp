#include <utility>
#include <asio.hpp>
#include "utils/logger.hpp"

using asio::ip::tcp;

int main()
{
    kv_store::utils::init_logging();

    try
    {
        asio::io_context io_context;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), 8080));

        LOG_INFO("KV-Store server running on port 8080...");
        LOG_INFO("Awaiting client connections...");

        for (;;)
        {
            tcp::socket socket(io_context);
            acceptor.accept(socket);

            LOG_INFO("Client connected: {}", socket.remote_endpoint().address().to_string());

            // Professional Tip: Ensure the message matches the size the client expects
            // "Welcome to KV-Store Distributed!" has 33 chars + \n = 34 bytes
            std::string msg = "Welcome to KV-Store Distributed!\n";

            asio::error_code ec;
            asio::write(socket, asio::buffer(msg), ec);

            if (ec)
            {
                LOG_ERROR("Failed to send message: {}", ec.message());
            }
            else
            {
                LOG_DEBUG("Greeting sent successfully.");
            }

            // The socket will close here when going out of scope.
            // For now it's fine, but in the future we will keep it alive
            // to process multiple commands (SET/GET).
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR("Server critical error: {}", e.what());
    }

    return 0;
}