#pragma once

#include <zmq.hpp>

const int BASE_PORT = 5555;

std::string GetConPort(int id) {
    return "tcp://localhost:" + std::to_string(BASE_PORT + id);
}

std::string GetBindPort(int id) {
    return "tcp://*:" + std::to_string(BASE_PORT + id);
}