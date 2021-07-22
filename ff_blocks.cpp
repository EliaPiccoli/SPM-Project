#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <vector>
#include "utils.hpp"

#define EMPTYFRONTIER -1

using namespace std;
using namespace ff;

// worker output token
struct worker_out_t {
    vector<int> n;  // vector id of children nodes
    int c;          // counter of value X
};

// Emitter
struct Emitter:ff_monode_t<unordered_set<int>, int> {
    unordered_set<int> frontier;
    int nw;
    utimer* frontier_time;
    int level;
    Emitter(int num_workers) {
        this->nw = num_workers;
        this->level = 0;
        this->frontier_time = NULL;
    }

    void compute_frontier() {
        // send all nodes in frontier
        for(int nid: this->frontier) {
            ff_send_out(new int(nid));
        }
    }

    int* svc(unordered_set<int>* task) {
        if(task == NULL) { // first call
            ff_send_out(new int(EMPTYFRONTIER));
        } else { // when receiving data from Collector -> new frontier to compute
            if(this->frontier_time != NULL) {
                delete this->frontier_time;
                this->level++;
            }
            this->frontier.swap(*task);
            delete task;
            this->compute_frontier();
            this->frontier_time = new utimer("Level " + to_string(this->level));
        }
        return GO_ON;
    }

    void svc_end() {
        delete frontier_time;
        for(int i=0; i<this->nw; i++)
            ff_send_out_to(EOS, i);
    }
};

// Worker
struct Worker:ff_node_t<int, worker_out_t> {
    unordered_map<int, node_t>* nodes_map;
    unordered_set<int>* visited_nodes;
    int val;

    Worker(unordered_map<int, node_t>* nodes, unordered_set<int>* vis, int x) {
        this->nodes_map = nodes;
        this->visited_nodes = vis;
        this->val = x;
    }

    worker_out_t* svc(int* task) {
        int node_id = *task;
        int sum = 0;
        vector<int> local_queue;
        if(node_id >= 0) { // id-node to be visited
            node_t node = this->nodes_map->at(node_id);
            if(node.val == this->val)
                sum++;
            for(auto n: node.children) {
                if(this->visited_nodes->find(n->id) == this->visited_nodes->end())
                    local_queue.push_back(n->id);
            }
            worker_out_t* out = new worker_out_t;
            out->c = sum;
            out->n = local_queue;
            delete task;
            return out;
        } else { // EndFrontier
            delete task;
            worker_out_t* out = new worker_out_t;
            out->c = -1;
            return out;
        }
    }
};

// Collector
struct Collector:ff_minode_t<worker_out_t, unordered_set<int>> {
    int tot_counter;
    unordered_set<int>* visited_nodes;
    unordered_set<int> new_frontier;
    int last_frontier_size;
    int task_counter;

    Collector(unordered_set<int>* vis, unordered_set<int> starting_frontier) {
        this->tot_counter = 0;
        this->visited_nodes = vis;
        this->new_frontier = starting_frontier;
        this->last_frontier_size = this->new_frontier.size();
        this->task_counter = 0;
    }

    unordered_set<int>* compute_next_frontier() {
        if(this->new_frontier.size() != 0) {
            // update visited nodes
            for(int i : this->new_frontier)
                this->visited_nodes->insert(i);
            // send new frontier
            unordered_set<int>* new_f = new unordered_set<int>();
            new_f->swap(this->new_frontier);
            return new_f;
        } else { // all leaves
            return EOS;
        }
    }

    unordered_set<int>* svc(worker_out_t* task) {
        worker_out_t t = *task;
        if(t.c >= 0) { // valid output from node
            this->tot_counter += t.c;
            for(int i : t.n)
                new_frontier.insert(i);
            this->task_counter++;
            if(this->task_counter == this->last_frontier_size) { // all nodes have been computed
                this->last_frontier_size = this->new_frontier.size();
                this->task_counter = 0;
                delete task;
                return compute_next_frontier();
            } else { // still missing nodes
                delete task;
                return GO_ON;
            } 
        } else { // signal from Emitter that the current frontier is empty (only at start)
            delete task;
            return compute_next_frontier();
        }
    }
};

// EmitterCollector - single nodes that handles all the work
struct EmitterCollector:ff_monode_t<worker_out_t, int> {
    unordered_set<int> frontier;
    unordered_set<int> next_frontier;
    unordered_set<int>* visited_nodes;
    utimer* frontier_time;
    int level;
    int nw;
    int task_counter;
    int total_counter;

    EmitterCollector(unordered_set<int> init_f, unordered_set<int>* vis, int num_workers) {
        this->frontier = init_f;
        this->nw = num_workers;
        this->visited_nodes = vis;
        this->task_counter = 0;
        this->total_counter = 0;
        this->frontier_time = NULL;
        this->level = 0;
    }

    void send_frontier() {
        // send all nodes in frontier
        for(int nid: this->frontier) {
            ff_send_out(new int(nid));
        }
    }

    int* svc(worker_out_t* task) {
        if(task != NULL) { // task from workers
            worker_out_t t = *task;
            this->total_counter += t.c;
            for(int i : t.n)
                this->next_frontier.insert(i);
            this->task_counter++;
            if(this->task_counter == this->frontier.size()) { // completed computation of current frontier
                if(this->next_frontier.size() == 0) { // all leaves
                    delete this->frontier_time;
                    delete task;
                    broadcast_task(EOS);
                    return EOS;
                } else { // update visited, compute new frontier and send tasks
                    unordered_set<int> emp;
                    this->frontier.swap(this->next_frontier);
                    this->next_frontier.swap(emp);
                    this->task_counter = 0;
                    for(int i : this->frontier)
                        this->visited_nodes->insert(i);
                    delete this->frontier_time;
                    this->level++;
                    this->frontier_time = new utimer("Level " + to_string(this->level));
                    send_frontier();
                    delete task;
                    return GO_ON;
                }
            } else { // still missing nodes
                delete task;
                return GO_ON;
            }
        } else { // start of the node
            this->frontier_time = new utimer("Level " + to_string(this->level));
            send_frontier();
            return GO_ON;
        }
    }
};