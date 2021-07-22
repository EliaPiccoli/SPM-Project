#include <iostream>
#include <unordered_map>
#include <string>
#include <tuple>
#include <queue>
#include <thread>
#include <unordered_set>
#include <unistd.h>
#include "utils.hpp"
#include "utimer.cpp"

using namespace std;

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
    long sequential, parallel;

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
                // cout << "Current frontier size: " << current_frontier.size() << endl;
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

    // Parallel implementation
    cout << "Parallel ------------" << endl;
    int parcount = 0, parcounter = 0;
    vector<int> cf;                     // current frontier
    unordered_set<int> vis;             // visited nodes
    vector<vector<int>> tlf(nw);        // thread local frontiers
    vector<pair<int, int>> wr(nw);      // workers range
    vector<int> lc(nw, 0);              // local counters
    vector<thread> t(nw);               // thread vector

    // thread task
    auto tnv = [&nodes, &tlf, &vis, &lc, &cf](const int id, const pair<int,int> range, const int x) {
        int partial = 0;
        vector<int> localq;
        for(int i=range.first; i<range.second; i++) {
            node_t n = nodes.at(cf[i]);
            if(n.val == x)
                partial++;
            for(auto c: n.children) {
                if(vis.find(c->id) == vis.end()) {
                    localq.push_back(c->id);
                }
            }
        }
        tlf[id] = localq;
        lc[id] += partial;
        return;
    };

    {
        utimer par("parallel graph_search", &parallel);
        {
            // frontier has only the initial node using threads will add overhead so compute it sequentially
            utimer lvl("level " + to_string(parcounter));
            if(nodes.at(S).val == X)
                parcount++;
            vis.insert(S);
            for(auto& c: nodes.at(S).children) {
                cf.push_back(c->id);
                vis.insert(c->id);
            }
            parcounter++;
        }
        while(!cf.empty()) {
            {
                utimer lvl("level " + to_string(parcounter));
                // compute range for single thread work
                int rnw = min(nw, (int)cf.size());
                int dt = cf.size()/rnw;
                for(int i=0; i<rnw; i++)
                    wr[i] = {i*dt, (i != rnw-1) ? (i+1)*dt : cf.size()};
                // create threads
                for(int i=0; i<rnw; i++)
                    t[i] = thread(tnv, i, wr[i], X);
                for(int i=0; i<rnw; i++)
                    t[i].join();
                
                // empty frontier to create new one
                vector<int> emp;
                cf.swap(emp);
                // merge local frontiers to create new frontier
                for(int i=0; i<rnw; i++) {
                    for(int j=0; j<tlf[i].size(); j++) {
                        int nid = tlf[i][j];
                        if(vis.find(nid) == vis.end()) { // a node can be shared in different local frontiers
                            vis.insert(nid);
                            cf.push_back(nid);
                        }
                    }
                    vector<int> empty;
                    tlf[i].swap(empty);
                }
                // go to next level
                parcounter++;
            }
        }
        // final sum of local counters
        for(int i=0; i<nw; i++)
            parcount += lc[i];
    }
    cout << "Parallel - Starting from node S=" << S << " #nodes with value X=" << X << ": " << parcount << endl;
    
    // compute and print speedup
    cout << "Speedup(" << nw << ") = " << (float)sequential/parallel << endl;    

    return 0;
}