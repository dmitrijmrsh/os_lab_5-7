#include <iostream>
#include <zmq.hpp>
#include <nlohmann/json.hpp>
#include <csignal>
#include <thread>
#include <cstdlib>

#include "tree.hpp"
#include "request.hpp"
#include "response.hpp"
#include "myMQ.hpp"

zmq::context_t context(1);
zmq::socket_t socket(context, zmq::socket_type::pair);
bool connected = false;

int rootID;

std::vector<int> pids;

int readID() {
    std::string id_str;
    std::cin >> id_str;
    char* end;
    int id = (int)strtol(id_str.c_str(), &end, 10);
    if ((*end != '\0' && *end != '\n')) {
        return -1;
    }
    return id;
}

void KillProcess() {
    for (auto& pid : pids) {
        kill(pid, SIGKILL);
    }
}

int main() {
    std::cout << "\t\tGuide" << std::endl;
    std::cout << "Creating a new node: create <id> <parent>" << std::endl;
    std::cout << "Execution of a command on a computing node: exec <id> <subcommand>" << std::endl;
    std::cout << " Subcommands:" << std::endl;
    std::cout << "  - time: get current time" << std::endl;
    std::cout << "  - start: start timer" << std::endl;
    std::cout << " - stop: stop timer" << std::endl;
    std::cout << "Output of all unavailable nodes: pingall" << std::endl;
    std::cout << "Remove node: remove <id>" << std::endl;
    std::cout << std::endl;

    Tree tree;
    
    std::string action;
    while(std::cout << "> " && std::cin >> action) {
        if (action == "create") {
            int id = readID();
            if (id == -1) {
                std::cout << "Error: invalid id" << std::endl;
                continue;
            }
            int parent_id = readID();
            if (parent_id == -1 && !tree.Empty()) {
                std::cout << "Error : invalid parent id" << std::endl;
                continue;
            }

            Error err;
            if (tree.Empty()) {
                err = tree.insert_root(id);
            } else {
                err = tree.insert(parent_id, id);
            }

            if (err) {
                std::cout << err << std::endl;
                continue;
            }

            if (!connected) {
                pid_t pid = fork();

                if (pid == -1) {
                    std::cout << "Error : fork failed" << std::endl;
                    continue;
                }

                if (pid == 0) {
                    execl("calculation", "calculation", std::to_string(id).c_str(), std::to_string(1).c_str(), nullptr);
                }

                if (pid > 0) {
                    socket.connect(GetConPort(id).c_str());
                    connected = true;
                    std::cout << "OK: " << pid << std::endl;
                    pids.push_back(pid);
                }
                rootID = id;
            } else {
                std::vector<int> path = tree.findPath(id);
                path.pop_back();

                Request req(Create, path, id);
                nlohmann::json jsonReq = {
                    {"action", req.action},
                    {"path", req.path},
                    {"id", req.id},
                    {"depth", tree.depth()}
                };

                std::string jsonReqString = jsonReq.dump();
                zmq::message_t msg(jsonReqString.begin(), jsonReqString.end());
                socket.send(msg, zmq::send_flags::none);

                std::this_thread::sleep_for(tree.depth() * std::chrono::milliseconds(50));

                zmq::message_t reply;
                bool replyed;
                try {
                    replyed = bool(socket.recv(reply, zmq::recv_flags::dontwait));
                } catch(zmq::error_t& e) {
                    replyed = false;
                }

                if (replyed) {
                    std::string replyStr = std::string(static_cast<char*>(reply.data()), reply.size());

                    nlohmann::json jsonReply = nlohmann::json::parse(replyStr);

                    Response resp;

                    resp.status = jsonReply.at("status");

                    if (resp.status == ERROR) {
                        std::string error = jsonReply["error"];
                        std::cout << error << std::endl;
                    } else if (resp.status == OK) {
                        int pid = jsonReply["pid"];
                        std::cout << "OK: " << pid << std::endl;
                        pids.push_back(pid);
                    }
                } else {
                    std::cout << "Error: parent node with id = " << id << " not found" << std::endl;
                    continue;
                }
            }
        } else if (action == "exec") {
            int id = readID();
            if (id == -1) {
                std::cout << "Error: invalid id" << std::endl;
                continue;
            }

            std::string command;
            std::cin >> command;

            if (command != "time" && command != "start" && command != "stop") {
                std::cout << "Error: invalid subcommand" << std::endl;
                continue;
            }

            std::shared_ptr<Node> finded = tree.search(id);
            if (!finded) {
                std::cout << "Error : node with id = " << id << " not found" << std::endl;
                continue;
            }

            std::vector<int> path = tree.findPath(id);

            Request req(command, path, id);
            nlohmann::json jsonReq = {
                {"action", req.action},
                {"path", req.path},
                {"id", req.id},
                {"depth", tree.depth()}
            };

            std::string jsonReqString = jsonReq.dump();
            zmq::message_t msg(jsonReqString.begin(), jsonReqString.end());
            socket.send(msg, zmq::send_flags::none);

            std::this_thread::sleep_for(tree.depth() * std::chrono::milliseconds(tree.depth()*50));

            zmq::message_t reply;
            bool replyed;
            try {
                replyed = bool(socket.recv(reply, zmq::recv_flags::dontwait));
            } catch(zmq::error_t& e) {
                replyed = false;
            }

            Response resp;
            nlohmann::json jsonReply;

            if (replyed) {
                std::string replyStr = std::string(static_cast<char*>(reply.data()), reply.size());
                jsonReply = nlohmann::json::parse(replyStr);
            } else {
                std::cout << "Error : node with id = " << id << " not available" << std::endl;
                continue;
            }

            resp.status = jsonReply.at("status");
            if (resp.status == ERROR) {
                std::string error = jsonReply["error"];
                std::cout << error << std::endl;
            } else if (resp.status == OK) {
                std::cout << "OK: " << id;
                if (req.action == Time) {
                    std::cout << " " << jsonReply["result"] << "ms";
                }
                std::cout << std::endl;
            }
        } else if (action == "pingall") {
            Request req("ping");
            req.depth = tree.depth();
            req.timeToWait = 1024;

            nlohmann::json jsonReq = {
                {"action", req.action},
                {"depth", req.depth},
                {"timeToWait", req.timeToWait}
            };
            std::string jsonReqStr = jsonReq.dump();
            zmq::message_t msg(jsonReqStr.begin(), jsonReqStr.end());
            socket.send(msg, zmq::send_flags::none);

            std::this_thread::sleep_for(std::chrono::milliseconds(req.timeToWait));

            zmq::message_t reply;
            bool replyed;
            try {
                replyed = bool(socket.recv(reply, zmq::recv_flags::dontwait));
            } catch(zmq::error_t& error) {
                replyed = false;
            }

            if (replyed) {
                std::string replyStr = std::string(static_cast<char*>(reply.data()), reply.size());
                nlohmann::json jsonReply = nlohmann::json::parse(replyStr);

                Response resp;
                resp.unavailable = std::vector<int>(jsonReply.at("unavailable"));

                std::cout << "OK: ";
                if (resp.unavailable.empty()) {
                    std::cout << -1 << std::endl;
                    continue;
                }

                for (auto& elem : resp.unavailable) {
                    auto unavailableSons = tree.get_nodes(elem);
                    std::cout << elem << "; ";
                    for (auto& son : unavailableSons) {
                        std::cout << son << "; ";
                    }
                }
            } else {
                auto unavailable = tree.get_nodes(rootID);
                std::cout << "OK: " << rootID << "; ";
                for (auto& elem : unavailable) {
                    std::cout << elem << "; ";
                }
                std::cout << std::endl;
            }
            std::cout << std::endl;
        } else {
            std::cout << "Unknown command" << std::endl;
        }
    }

    KillProcess();
    return 0;
}