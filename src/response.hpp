#pragma once

#include <string>
#include <chrono>

enum Status {
    OK,
    ERROR
};

struct Response {
    Status status;
    int result;
    int pid;
    std::string error;
    std::vector<int> unavailable;

    Response(): status(OK) {}
    Response(Status _status) : status(_status) {}
    Response(const std::string& status) {
        if (status == "OK") {
            this->status = OK;
        } else {
            this->status = ERROR;
        }
    }

    operator std::string() const {
        if (status == OK) {
            return "OK";
        } else {
            return "ERROR";
        }
    }

    bool StatusOK() const {
        return status == OK;
    }
};