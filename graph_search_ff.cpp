#include <iostream>
#include <ff/ff.hpp>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <ff/farm.hpp>
#include "utimer.cpp"
#include "utils.hpp"
#include "ff_blocks.cpp"

#define MAX_WORKERS 256

using namespace std;
using namespace ff;

int main(int argc, char** argv) {
    string filename;
    int S, X, nw;
    tie(filename, S, X, nw) = handle_input(argc, argv);
    
    // create graph from file
    unordered_map<int, node_t> nodes;
    {
        utimer graph_time("create_graph");
        nodes = create_graph(filename);
        // print_graph(nodes);
    }
    // to save computation time
    long sequential;

    // Sequential implementation
    cout << "Sequential ------------" << endl;
    int count = 0, counter = 0;
    queue<int> current_frontier;
    queue<int> next_frontier;
    current_frontier.push(S);
    if(nodes.at(S).val == X)
        count++;
    // visit the graph with BFS and count
    {
        unordered_set<int> visited;
        utimer sgs("sequential graph_search", &sequential);
        while(!current_frontier.empty()) {
            {
                // computing i-th level
                utimer level_search("level " + to_string(counter));
                do {
                    // get node top of the queue
                    node_t node = nodes.at(current_frontier.front());
                    current_frontier.pop();
                    visited.insert(node.id);
                    if(node.val == X)
                        count++;
                    // add childer if not visited or visited at current level
                    for(auto& c : node.children) {
                        if(visited.find(c->id) == visited.end()) {
                            next_frontier.push(c->id);
                            visited.insert(c->id);
                        }
                    }
                } while(!current_frontier.empty());
            }
            // smart re-initialization of frontiers
            current_frontier.swap(next_frontier);
            counter++;
        }
    }
    cout << "Sequential - Starting from node S=" << S << " #nodes with value X=" << X << ": " << count << endl;

    // FastFlow implementation
    cout << "FastFlow ------------" << endl;
    int rnw = nw;
    if (nw == MAX_WORKERS)
        rnw = nw - 2;
    unordered_set<int> visited;
    unordered_set<int> init_frontier = {S};
    // create Workers, Emitter, Collector to build farm
    vector<ff_node*> W;
    for(int i=0; i<rnw; i++)
        W.push_back(new Worker(&nodes, &visited, X));
    Emitter E = Emitter(rnw);
    Collector C = Collector(&visited, init_frontier);
    ff_farm farm(W, &E, &C);
    farm.wrap_around();

    // execute farm
    ffTime(START_TIME);
    if(farm.run_and_wait_end() < 0) {
        error("running farm");
        return -1;
    }
    ffTime(STOP_TIME);
    cout << "FF computation: " << ffTime(GET_TIME) << " ms" << endl;
    cout << "FF occurrences of node X=" << X << ": " << C.tot_counter << endl;
    cout << "Speedup(" << rnw << ") = " << (float)sequential/(1000*ffTime(GET_TIME)) << endl;

    // FastFlow #2 implementation
    cout << "FastFlow #2 ------------" << endl;
    if (nw == MAX_WORKERS)
        rnw = nw - 1;
    unordered_set<int> v;
    unordered_set<int> init = {S};
    // create Workers, EmitterCollector to build farm
    vector<ff_node*> WW;
    for(int i=0; i<rnw; i++)
        WW.push_back(new Worker(&nodes, &v, X));
    EmitterCollector EC = EmitterCollector(init, &v, rnw);
    ff_farm f(WW, &EC);
    f.remove_collector();
    f.wrap_around();

    // execute farm
    ffTime(START_TIME);
    if(f.run_and_wait_end() < 0) {
        error("running farm");
        return -1;
    }
    ffTime(STOP_TIME);
    cout << "FF computation: " << ffTime(GET_TIME) << " ms" << endl;
    cout << "FF occurrences of node X=" << X << ": " << EC.total_counter << endl;
    cout << "Speedup(" << rnw << ") = " << (float)sequential/(1000*ffTime(GET_TIME)) << endl;

    return 0;
}