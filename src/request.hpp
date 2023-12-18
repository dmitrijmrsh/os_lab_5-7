#pragma once

#include <string>
#include <vector>

enum Action {
    Time,
    Start,
    Stop,
    Ping,
    Create,
    Unknown
};

struct Request {
    Action action;
    std::vector<int> path;
    int id;
    int depth;
    int timeToWait;
    
    Request() : action(Unknown) {}
    Request(const std::string& str) {
        if (str == "time") {
            action = Time;
        } else if (str == "start") {
            action = Start;
        } else if (str == "stop") {
            action = Stop;
        } else if (str == "ping") {
            action = Ping;
        } else {
            action = Unknown;
        }
    }
    Request(const Action& a, std::vector<int> _path, int _id): action(a), path(_path), id(_id) {}
    Request(const std::string& str,const std::vector<int>& _path, int _id): Request(str) {
        path = _path;
        id = _id;
    }

    operator std::string() const {
        switch (action) {
            case Time:
                return "time";
            case Start:
                return "start";
            case Stop:
                return "stop";
            default:
                return "unknown";
        }
    }

    ~Request() = default;
};