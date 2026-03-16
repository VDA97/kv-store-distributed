cmake_minimum_required(VERSION 3.15)
project(KVStoreDistributed LANGUAGES CXX)

# Configurações globais de C++
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # Garante C++ puro, evita conflitos de extensões de compilador

# 1. Buscar Dependências
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
find_package(spdlog REQUIRED)

# 2. Gerar código do Protobuf
file(GLOB PROTO_FILES "proto/*.proto")
if(PROTO_FILES)
    protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})
endif()

# 3. Criar a biblioteca de lógica (Core)
add_library(kv_store_core STATIC
    src/storage/hash_table.cpp
    src/network/server.cpp
    src/network/session.cpp
    src/utils/metrics.cpp
    ${PROTO_SRCS}
)

# Configurações específicas para a biblioteca Core
target_compile_definitions(kv_store_core PUBLIC ASIO_STANDALONE)

target_include_directories(kv_store_core PUBLIC 
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}> # Para os headers do Protobuf
)

target_link_libraries(kv_store_core PUBLIC 
    Threads::Threads 
    ${PROTOBUF_LIBRARIES}
    spdlog::spdlog
)

# 4. Criar o Executável do Servidor
add_executable(server_app apps/server.cpp)
target_link_libraries(server_app PRIVATE kv_store_core)

# 5. Criar o Executável do Cliente
add_executable(client_cli apps/client_cli.cpp)
target_link_libraries(client_cli PRIVATE kv_store_core)

# DICA PARA O ERRO DE EXCHANGE:
# Se o seu GCC ainda reclamar do exchange no asio, esta linha força o include 
# necessário no nível do compilador apenas para o servidor.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(kv_store_core PRIVATE -include utility)
endif()