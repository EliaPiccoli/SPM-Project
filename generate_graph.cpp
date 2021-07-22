#include <iostream>
#include <fstream>
#include <stdlib.h>

#define MIN_BREADTH 100
#define MAX_BREADTH 100
#define MIN_DEPTH 1000
#define MAX_DEPTH 1000
#define PROB_EDGE 5
#define LEVEL_DISTANCE 100
#define SEED 1234

using namespace std;

int main(int argc, char** argv) {
	srand(SEED);

	// number of levels of the graph
	int levels = MIN_DEPTH + rand()%(MAX_DEPTH - MIN_DEPTH + 1);
	int total_nodes = 0;
	// dot file to save graph (visualization graphviz) (only small graph)
	// ofstream dot_file("graph/graph_" + to_string(MIN_BREADTH*MIN_DEPTH) + ".dot", ofstream::out | ofstream::trunc);
	// file to encode graph in easier way to be used in graph_search.cpp
	ofstream node_file("graph/nodes_" + to_string(MIN_BREADTH*MIN_DEPTH) + ".txt", ofstream::out | ofstream::trunc);
	// dot_file << "digraph {\n";
	
	for(int i=0; i<levels; i++) {
		// number of nodes in the i-th level
		int level_nodes = MIN_BREADTH + rand()%(MAX_BREADTH - MIN_BREADTH + 1);
		cout << "Level : " << i << "/" << MAX_DEPTH << endl;
		// iterate over past level nodes to created possible connection
		for(int j=0; j<total_nodes; j++) {
			for(int k=0; k<level_nodes; k++) {
				// to avoid a too dense graph penalize connections between distant levels
				int flip;
				if(k + total_nodes - j >= LEVEL_DISTANCE*level_nodes) {
					flip = (rand()%100)*3;
					//continue;
				} else {
					flip = rand()%100;
				}
				// randomly add edges between nodes
				if(flip < PROB_EDGE) {
					//dot_file << "\t" << j << " -> " << k+total_nodes << endl;
					node_file << j << " " << k+total_nodes << endl;
				}
			}
		}
		// update current total number of nodes
		total_nodes += level_nodes;
	}
	// dot_file << "}" << endl;
	// dot_file.close();
	node_file.close();
	cout << "Total number of nodes: " << total_nodes << endl;

	return 0;
}