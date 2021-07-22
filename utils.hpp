#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>

#ifndef UTILS_HPP
#define UTILS_HPP

#define SEED 123
#define MAX_VAL 50

struct node_t {
    int id;
    int val;
    std::vector<node_t*> children;
};

std::tuple<std::string, int, int, int> handle_input(int argc, char** argv);
std::unordered_map<int, node_t> create_graph(std::string filename);
void print_graph(std::unordered_map<int, node_t> nodes);

#endif