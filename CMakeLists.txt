cmake_minimum_required(VERSION 2.8)

project(NetworkTermProject)

add_executable(client src/ClientTCP.c)
add_executable(server src/ServerTCP.c)

set(CMAKE_CXX_FLAGS "-g")

install(TARGETS client RUNTIME DESTINATION bin)
install(TARGETS server RUNTIME DESTINATION bin)
