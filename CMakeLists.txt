# Versão mínima do CMake necessária
cmake_minimum_required(VERSION 3.10)

# Nome do projeto e linguagem
project(DamasBrasil_GTK CXX)

# Define o padrão C++ a ser usado (C++17 é uma boa escolha moderna)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Encontra o PkgConfig para ajudar a localizar bibliotecas do sistema
find_package(PkgConfig REQUIRED)

# Usa o PkgConfig para encontrar o gtkmm-3.0 e configurar as variáveis necessárias
pkg_check_modules(GTKMM REQUIRED gtkmm-3.0)

# Adiciona o executável, especificando o arquivo fonte
# Usar CMAKE_CURRENT_SOURCE_DIR torna o caminho explícito e mais robusto.
add_executable(patriotas 
    ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/damas_core.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/damas_ai.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/damas_dxp.cpp
)

# Vincula o executável com as bibliotecas do GTKmm
target_link_libraries(patriotas PRIVATE ${GTKMM_LIBRARIES} zstd)
target_include_directories(patriotas PRIVATE ${GTKMM_INCLUDE_DIRS})
