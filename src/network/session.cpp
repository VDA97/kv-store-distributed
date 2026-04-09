#include "network/session.hpp"
#include "utils/logger.hpp"
#include "utils/metrics.hpp"
#include "message.pb.h"
#include <sstream>

namespace kv_store::network
{

Session::Session(tcp::socket socket, storage::HashTable &storage, const std::string &dump_file, ReplicationConfig config)
    : socket_(std::move(socket)), storage_(storage), dump_file_(dump_file), config_(config) {}

Session::~Session()
{
    kv_store::utils::Metrics::active_connections--;
}

void Session::start()
{
    kv_store::utils::Metrics::active_connections++;
    do_read();
}

void Session::do_read()
{
    auto self = shared_from_this();

    // Usamos o buffer de membro da classe para garantir a vida dos dados
    socket_.async_read_some(asio::buffer(data_buffer_),
                            [this, self](std::error_code ec, std::size_t length)
                            {
                                if (!ec)
                                {
                                    handle_request(std::string(data_buffer_, length));
                                }
                                else if (ec != asio::error::eof)
                                {
                                    LOG_ERROR("Read error: {}", ec.message());
                                }
                            });
}

void Session::handle_request(const std::string &raw_data)
{
    auto self = shared_from_this();

    // 1. Limpeza do Buffer: O Protobuf não aceita o '\n' que o cliente envia.
    // Removemos qualquer caractere de controle (\n, \r) do final da string.
    std::string clean_data = raw_data;
    while (!clean_data.empty() && (clean_data.back() == '\n' || clean_data.back() == '\r'))
    {
        clean_data.pop_back();
    }

    KVRequest request;
    KVResponse response;

    // 2. Parse: Agora usamos a string sem o lixo do delimitador
    if (!request.ParseFromString(clean_data))
    {
        LOG_ERROR("Failed to parse Protobuf request. Size received: {}", raw_data.size());
        return;
    }
    kv_store::utils::Metrics::total_requests++;

    // 3. Processamento do Comando
    if (request.type() == KVRequest::SET)
    {
        kv_store::utils::Metrics::total_set_ops++;
        storage_.set(request.key(), request.value(), request.ttl_seconds());

        if (config_.mode == ServerMode::LEADER)
        {
            LOG_INFO("Leader Mode: Replicating '{}' to {} followers",
                     request.key(), config_.followers.size());
            replicate_to_all_followers(request);
        }
        else
        {
            LOG_INFO("Follower Mode: Received replicated data for '{}'", request.key());
        }
        response.set_success(true);
        response.set_message("Key stored successfully (Async)");
        LOG_INFO("Async SET: key={}", request.key());
    }
    else if (request.type() == KVRequest::GET)
    {
        kv_store::utils::Metrics::total_get_ops++;
        // Usando a lógica de std::optional da sua HashTable.hpp
        auto result = storage_.get(request.key());

        if (result.has_value())
        {
            response.set_success(true);
            response.set_value(result.value());
            response.set_message("Key found");
        }
        else
        {
            response.set_success(false);
            response.set_message("Key not found");
        }
        LOG_INFO("Async GET: key={} found={}", request.key(), result.has_value());
    }

    // 4. Resposta: Serializa e envia de volta
    std::string response_serialized;
    response.SerializeToString(&response_serialized);
    do_write(response_serialized);
}

void Session::do_write(const std::string &response_data)
{
    auto self = shared_from_this();

    // ADICIONAMOS O \n AQUI para o cliente saber que a mensagem acabou
    write_buffer_ = response_data + "\n";

    asio::async_write(socket_, asio::buffer(write_buffer_),
                      [this, self](std::error_code ec, std::size_t /*length*/)
                      {
                          if (!ec)
                          {
                              // Se quiser que o servidor feche a conexão após responder
                              // (combinando com o seu socket1.close() do cliente):
                              // socket_.close();

                              // OU, se quiser manter vivo (Keep-Alive), chame do_read:
                              do_read();
                          }
                          else
                          {
                              LOG_ERROR("Write error: {}", ec.message());
                          }
                      });
}

void Session::replicate_to_all_followers(const KVRequest &request)
{
    std::string serialized_payload;
    if (!request.SerializeToString(&serialized_payload))
    {
        LOG_ERROR("Failed to serialize request for replication");
        return;
    }
    serialized_payload += "\n";

    auto self = shared_from_this();

    for (const auto &[host, port] : config_.followers)
    {
        auto replication_socket = std::make_shared<tcp::socket>(socket_.get_executor());
        tcp::resolver resolver(socket_.get_executor());
        auto endpoints = resolver.resolve(host, std::to_string(port));

        // 1. Primeiro conectamos
        asio::async_connect(*replication_socket, endpoints,
                            [self, replication_socket, serialized_payload, host, port](std::error_code ec, tcp::endpoint)
                            {
                                if (!ec) // Se a conexão deu certo
                                {
                                    // 2. AGORA sim chamamos o async_write para enviar o payload
                                    asio::async_write(*replication_socket, asio::buffer(serialized_payload),
                                                      [host, port](std::error_code ec_write, std::size_t /*length*/)
                                                      {
                                                          // 3. Aqui usamos o nome correto: ec_write
                                                          if (!ec_write)
                                                          {
                                                              LOG_INFO("Successfully replicated to follower {}:{}", host, port);
                                                          }
                                                          else
                                                          {
                                                              LOG_ERROR("Failed to write to follower {}:{}: {}", host, port, ec_write.message());
                                                          }
                                                      });
                                }
                                else
                                {
                                    LOG_ERROR("Failed to connect to follower {}:{}: {}", host, port, ec.message());
                                }
                            });

        LOG_DEBUG("Initiating replication to follower at {}:{}", host, port);
    }
}

} // namespace kv