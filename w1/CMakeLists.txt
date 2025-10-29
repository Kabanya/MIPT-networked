project(w1)

add_executable(w1-client client.cpp client_tools.cpp socket_tools.cpp)

add_executable(w1-server server.cpp server_tools.cpp socket_tools.cpp)