#include <iostream>
#include <fstream>
#include <string>
#include <tuple>
#include "utils.hpp"

using namespace std;

tuple<string, int, int, int> handle_input(int argc, char** argv) {
    if(argc < 2) {
        cout << "Usage ./gs <graph_nodes_file> <starting_node> <value> <nw>" << endl;
        exit(1);
    } else {
        srand(SEED);
        string file = "";
        int start_node = 0, value = rand()%MAX_VAL, nw = 1;
        switch(argc) {
            case 2: {
                file = argv[1];
                break;
            }
            case 3: {
                file = argv[1];
                start_node = atoi(argv[2]);
                break;
            }
            case 4: {
                file = argv[1];
                start_node = atoi(argv[2]);
                value = atoi(argv[3]);
                break;
            }
            case 5: {
                file = argv[1];
                start_node = atoi(argv[2]);
                value = atoi(argv[3]);
                nw = atoi(argv[4]);
                break;
            }
            default: {
                cout << "Error: too many arguments." << endl;
                exit(1);
            }
        }
        return {file, start_node, value, nw};
    }
}

unordered_map<int, node_t> create_graph(string filename) {
    unordered_map<int, node_t> nodes;
    ifstream file(filename, ifstream::in);
    if(!file.is_open()) {
        cout << "Failed to open file: " << filename << endl;
        exit(1);
    } else {
        int a, b;
        while(file >> a >> b) {
            if(nodes.find(b) == nodes.end()) { // node with id b not present
                node_t node = {
                    .id=b,
                    .val=rand()%MAX_VAL,
                    .children=vector<node_t*>()
                };
                nodes.insert({b, node});
            }
            if(nodes.find(a) == nodes.end()) { // node with id a not present
                node_t node = {
                    .id=a,
                    .val=rand()%MAX_VAL,
                    .children=vector<node_t*>()
                };
                node.children.push_back(&nodes.at(b)); // add node b as child of node a
                nodes.insert({a, node});
            } else { // node a is already present just add child
                nodes.at(a).children.push_back(&nodes.at(b)); // add node b as child of node a
            }
        }
    }
    file.close();

    return nodes;
}

void print_graph(std::unordered_map<int, node_t> nodes) {
    for(const auto& n : nodes) {
        string s = "";
        for(const auto& x : n.second.children)
            s = s + to_string(x->id) + " - ";
        cout << "ID:[" << n.first << " - " << n.second.id << "] VAL:[" << n.second.val << "] CHILD:[{" << s << "}]" << endl;
    }
}