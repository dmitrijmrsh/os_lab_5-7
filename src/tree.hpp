#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <algorithm>

typedef const char* Error;

struct Node {
    int id;
    std::vector<std::shared_ptr<Node>> children;

    Node(int value) : id(value) {}
    void insert_child(std::shared_ptr<Node> child) {
        children.push_back(child);
    }
    
};

class Tree {
    private:
        std::shared_ptr<Node> root;

        std::shared_ptr<Node> search_helper(int id, std::shared_ptr<Node>& root) {
            if (root == nullptr) {
                return nullptr;
            }
            if (root->id == id) {
                return root;
            }
            for (std::shared_ptr<Node>& child : root->children) {
                std::shared_ptr<Node> temp = search_helper(id, child);
                if (temp != nullptr) {
                    return temp;
                }
            }
            return nullptr;
        }

        void remove_subtree(std::shared_ptr<Node>& root) {
            if (root == nullptr) {
                return;
            }
            for (std::shared_ptr<Node>& child : root->children) {
                remove_subtree_helper(child);
            }
            root->children.clear();
            return;
        }

        void remove_subtree_helper(std::shared_ptr<Node>& root) {
            if (root == nullptr) {
                return;
            }
            for (std::shared_ptr<Node>& child : root->children) {
                remove_subtree_helper(child);
            }
            root = nullptr;
            return;
        }

        void remove_helper(int id, std::shared_ptr<Node>& root) {
            if (root == nullptr) {
                return;
            }
            if (root->id == id) {
                if (root->children.size() != 0) {
                    remove_subtree(root);
                }
                root = nullptr;
                return;
            }
            for (int i = 0; i < root->children.size(); ++i) {
                if (root->children[i]->id == id) {
                    if (root->children[i]->children.size() != 0) {
                        remove_subtree(root->children[i]);
                    }
                    root->children[i] = nullptr;
                    root->children.erase(root->children.begin() + i);
                    return;
                }
            }
            for (std::shared_ptr<Node>& child : root->children) {
                remove_helper(id, child);
            }
        }

        bool findPath_helper(std::shared_ptr<Node>& root, int id, std::vector<int>& path) {
            if (root == nullptr) {
                return false;
            }

            path.push_back(root->id);

            if (root->id == id) {
                return true;
            }

            for (std::shared_ptr<Node>& child : root->children) {
                if (findPath_helper(child, id, path)) {
                    return true;
                }
            }

            path.pop_back();
            return false;
        }

        int depth_helper(std::shared_ptr<Node>& root) {
            if (root == nullptr) {
                return 0;
            }

            int maxChildDepth = 0;
            for (std::shared_ptr<Node> child : root->children) {
                maxChildDepth = std::max(maxChildDepth, depth_helper(child));
            }

            return maxChildDepth + 1;
        }

        void print_helper(std::shared_ptr<Node>& root) {
            if (root == nullptr) {
                return;
            }
            std::cout << root->id << std::endl;
            for (std::shared_ptr<Node>& child : root->children) {
                print_helper(child);
            }
            return;
        }

    public:
        Tree() {
            root = nullptr;
        }

        Tree(int id) {
            root = std::make_shared<Node>(id);
        }

        std::shared_ptr<Node>& getrootptr() {
            return root;
        }
        
        std::shared_ptr<Node> search(int id) {
            return this->search_helper(id, root);
        }

        bool Empty() {
            if (root) {
                return false;
            }
            return true;
        }

        Error insert_root(int id) {
            if (root) {
                return "Error : root is already exists";
            }

            root = std::make_shared<Node>(id);
            return nullptr;
        }

        Error insert(int parent_id, int id) {
            std::shared_ptr<Node> have = search(id);
            if (have) {
                return "Error : Already exist"; 
            }

            std::shared_ptr<Node> parent = search(parent_id);
            if (!parent) {
                return "Error : Parent not found";
            }

            std::shared_ptr<Node> child = std::make_shared<Node>(id);
            parent->insert_child(child);
            return nullptr;
        }

        Error remove(int id) {
            std::shared_ptr<Node> have = search(id);
            if (!have) {
                return "Error : id not exist";
            }

            remove_helper(id, root);
            return nullptr;
        }

        std::vector<int> findPath(int id) {
            std::vector<int> path;
            findPath_helper(root, id, path);
            return path;
        }

        int depth() {
            return this->depth_helper(root);
        }

        std::vector<int> get_nodes(int id) {
            std::shared_ptr<Node> node_ptr = search(id);
            if (!node_ptr) {
                return {};
            }
            std::vector<int> answers;
            for (std::shared_ptr<Node> child : node_ptr->children) {
                answers.push_back(child->id);
                std::vector<int> child_answers = get_nodes(child->id);
                answers.insert(answers.end(), child_answers.begin(), child_answers.end());
            }
            return answers;
        }

        void print() {
            print_helper(root);
        }

        ~Tree() {
            remove_subtree(root);
            root = nullptr;
        }
};