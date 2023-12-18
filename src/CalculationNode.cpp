#include <fstream>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include <unordered_map>

#include "myMQ.hpp"
#include "request.hpp"
#include "response.hpp"
#include "timer.hpp"

int id, current_depth;

zmq::context_t ParentContext(1);
zmq::socket_t ParentSocket(ParentContext, zmq::socket_type::pair);

std::unordered_map<int, std::pair<zmq::context_t, zmq::socket_t>> children_data;

std::string HandleCreate(nlohmann::json& jsonData, Request& req) {
    req.path = std::vector<int>(jsonData.at("path"));
    req.id = jsonData.at("id");
    req.depth = jsonData.at("depth");

    Response resp;
    nlohmann::json jsonResp;

    if (req.path.size() == 1) {
        pid_t pid = fork();

        if (pid == -1) {
            resp.status = ERROR;

            jsonResp = {
                {"status", resp.status},
                {"error", "forked error"}
            };

            return jsonResp.dump();
        }

        if (pid == 0) {
            execl("calculation", "calculation", std::to_string(req.id).c_str(), std::to_string(current_depth + 1).c_str(), nullptr);
        }

        if (pid > 0) {
            children_data[req.id].first = zmq::context_t(1);
            children_data[req.id].second = zmq::socket_t(children_data[req.id].first, zmq::socket_type::pair);
            children_data[req.id].second.connect(GetConPort(req.id).c_str());

            resp.status = OK;
            resp.pid = pid;

            jsonResp = {
                {"status", resp.status},
                {"pid", resp.pid}
            };

            return jsonResp.dump();
        }
    } else {
        Request reqToSon;
        reqToSon.action = Create;
        req.path.erase(req.path.begin());
        reqToSon.path = req.path;
        reqToSon.id = req.id;
        reqToSon.depth = req.depth;

        nlohmann::json jsonReq = {
            {"action", reqToSon.action},
            {"path", reqToSon.path},
            {"id", reqToSon.id},
            {"depth", reqToSon.depth}
        };
        
        std::string jsonReqString = jsonReq.dump();

        zmq::message_t message(jsonReqString.begin(), jsonReqString.end());
        zmq::message_t reply;

        children_data[reqToSon.path[0]].second.send(message, zmq::send_flags::none);

        std::this_thread::sleep_for(std::chrono::milliseconds(req.depth - current_depth + 1) * 20);

        bool replyed;

        try {
            replyed = bool(children_data[reqToSon.path[0]].second.recv(reply, zmq::recv_flags::dontwait));
        } catch(const zmq::error_t& error) {
            resp.status = ERROR;
            resp.error = std::string(error.what());

            replyed = false;
        }

        if (replyed) {
            std::string jsonRespString = std::string(static_cast<char*>(reply.data()), reply.size());
            jsonResp = nlohmann::json::parse(jsonRespString);
        } else {
            resp.status = ERROR;
            resp.error = "Process with id " + std::to_string(reqToSon.path[0]) + " not available";

            jsonResp = {
                {"status", resp.status},
                {"error", resp.error}
            };
        }

        return jsonResp.dump();
    }
    return "";
}

std::string HandleExec(nlohmann::json& jsonData, Request& req) {
    req.path = std::vector<int>(jsonData.at("path"));
    req.id = jsonData.at("id");
    req.depth = jsonData.at("depth");

    Response resp;
    nlohmann::json jsonResp;

    if (req.path.size() == 1) {
        if (req.action == Start) {
            Error err = StartTimer();
            if (err != nullptr) {
                resp.status = ERROR;
                resp.error = err;
                jsonResp = {
                    {"status", resp.status},
                    {"error", resp.status}
                };
            } else {
                resp.status = OK;
                jsonResp = {
                    {"status", resp.status}
                };
            }
        } else if (req.action == Stop) {
            Error err = StopTimer();
            if (err != nullptr) {
                resp.status = ERROR;
                resp.error = err;
                jsonResp = {
                    {"status", resp.status},
                    {"error", resp.error}
                };
            } else {
                resp.status = OK;
                jsonResp = {
                    {"status", resp.status}
                };
            }
        } else if (req.action == Time) {
            int res = GetTime();

            resp.status = OK;
            resp.result = res;
            jsonResp = {
                {"status", resp.status},
                {"result", resp.result}
            };
        } else {
            resp.status = ERROR;
            resp.error = "Wrong action";
            jsonResp = {
                {"status", resp.status},
                {"error", resp.error}
            };
        }

        return jsonResp.dump();
    } else {
        Request reqToSon;
        reqToSon.action = req.action;
        req.path.erase(req.path.begin());
        reqToSon.path = req.path;
        reqToSon.id = req.id;
        reqToSon.depth = req.depth;

        nlohmann::json jsonReq = {
            {"action", reqToSon.action},
            {"path", reqToSon.path},
            {"id", reqToSon.id},
            {"depth", reqToSon.depth}
        };

        std::string jsonReqString = jsonReq.dump();
        zmq::message_t message(jsonReqString.begin(), jsonReqString.end());

        zmq::message_t reply;

        children_data[reqToSon.path[0]].second.send(message, zmq::send_flags::none);

        std::this_thread::sleep_for(std::chrono::milliseconds(req.depth - current_depth + 1) * 20);

        bool replyed;

        try {
            replyed = bool(children_data[reqToSon.path[0]].second.recv(reply, zmq::recv_flags::none));
        } catch (const zmq::error_t& err) {
            replyed = false;
        }

        if (replyed) {
            std::string jsonRespString = std::string(static_cast<char*>(reply.data()), reply.size());
            jsonResp = nlohmann::json::parse(jsonRespString);
        } else {
            resp.status = ERROR;
            resp.error = "Process with id" + std::to_string(reqToSon.path[0]) + " not available";

            jsonResp = {
                {"status", resp.status},
                {"error", resp.error}
            };
        }

        return jsonResp.dump();
    }

    return "";
}

std::string HandlePing(nlohmann::json& jsonData, Request& req) {
    req.depth = jsonData.at("depth");
    req.timeToWait = jsonData.at("timeToWait");
    req.timeToWait /= 2;

    Response resp;
    nlohmann::json jsonResp;
    std::vector<int> unavailable;

    std::vector<std::pair<Request, int>> children_requests(children_data.size());

    int count = 0;
    for (const auto& [id, Pair] : children_data) {
        children_requests[count].second = id;
        ++count;
    }

    for (int i = 0; i < children_requests.size(); ++i) {
        children_requests[i].first.action = req.action;
        children_requests[i].first.depth = req.depth;
        children_requests[i].first.timeToWait = req.timeToWait;
        nlohmann::json jsonReqToSon = {
            {"action", children_requests[i].first.action},
            {"depth", children_requests[i].first.depth},
            {"timeToWait", children_requests[i].first.timeToWait}
        };

        std::string jsonReqToSonString = jsonReqToSon.dump();
        zmq::message_t messageToSon(jsonReqToSon.begin(), jsonReqToSon.end());

        children_data[children_requests[i].second].second.send(messageToSon, zmq::send_flags::none);

        std::this_thread::sleep_for(std::chrono::milliseconds(req.timeToWait));

        zmq::message_t replyFromSon;
        bool replyed = true;

        try {
            replyed = bool(children_data[children_requests[i].second].second.recv(replyFromSon, zmq::recv_flags::dontwait));
        } catch (const zmq::error_t& error) {
            replyed = false;
        }

        if (!replyed) {
            unavailable.push_back(children_requests[i].second);
        } else {
            std::string jsonReplyString = std::string(static_cast<char*>(replyFromSon.data()), replyFromSon.size());

            nlohmann::json jsonReply = nlohmann::json::parse(jsonReplyString);
            std::vector<int> new_unavailable = std::vector<int>(jsonReply.at("unavailable"));
            unavailable.insert(unavailable.begin(), new_unavailable.begin(), new_unavailable.end());
        }
    }

    resp.status = OK;
    resp.unavailable = unavailable;

    jsonResp = {
        {"status", resp.status},
        {"unavailable", resp.unavailable}
    };

    return jsonResp.dump();
}


int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }

    id = std::stoi(argv[1]);
    current_depth = std::stoi(argv[2]);

    ParentSocket.bind(GetBindPort(id));
    zmq::message_t request;
    while(ParentSocket.recv(request, zmq::recv_flags::none)) {
        std::string jsonReqString = std::string(static_cast<char*>(request.data()), request.size());

        nlohmann::json jsonData = nlohmann::json::parse(jsonReqString);
        Request req;
        req.action = jsonData["action"];

        std::string jsonRespString;

        if (req.action == Time || req.action == Start || req.action == Stop) {
            jsonRespString = HandleExec(jsonData, req);
        } else if (req.action == Ping) {
            jsonRespString = HandlePing(jsonData, req);
        } else if (req.action == Create) {
            //std::cout << id << std::endl;
            jsonRespString = HandleCreate(jsonData, req);
        }

        zmq::message_t reply(jsonRespString.begin(), jsonRespString.end());
        ParentSocket.send(reply, zmq::send_flags::none);
    }

    ParentSocket.close();
    ParentContext.close();
    return 0;
}