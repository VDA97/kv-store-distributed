cmake_minimum_required(VERSION 3.15)
project(KVStoreDistributed LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DASIO_STANDALONE)

# 1. Buscar Dependências
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)

# 2. Caminhos de Inclusão
include_directories(
    include
    ${PROTOBUF_INCLUDE_DIRS}
    ${CMAKE_CURRENT_BINARY_DIR}
)

# 3. Gerar código do Protobuf (caso existam arquivos .proto)
file(GLOB PROTO_FILES "proto/*.proto")
if(PROTO_FILES)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
endif()

# 4. Criar a biblioteca de lógica (Core)
# Note: Se os arquivos .cpp estiverem vazios, o build pode dar erro de símbolo.
add_library(kv_store_core
    src/storage/hash_table.cpp
    src/network/tcp_server.cpp
    ${PROTO_SRCS}
)

target_link_libraries(kv_store_core PUBLIC 
    Threads::Threads 
    ${PROTOBUF_LIBRARIES}
    spdlog::spdlog
)

# 5. Criar o Executável do Servidor
add_executable(server_app apps/server.cpp)
target_link_libraries(server_app PRIVATE kv_store_core)

# 6. Criar o Executável do Cliente
add_executable(client_cli apps/client_cli.cpp)
target_link_libraries(client_cli PRIVATE kv_store_core)