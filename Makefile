CC = g++
UTILS = utils.cpp
LIBS = -pthread -O3
FF_INC = -I ~/fastflow-3.0.0/

all: clean gg gs ffgs

gg: generate_graph.cpp
	${CC} ${UTILS} generate_graph.cpp -o gg

gs: graph_search.cpp
	${CC} ${UTILS} ${LIBS} graph_search.cpp -o gs

ffgs: graph_search_ff.cpp
	${CC} ${UTILS} ${LIBS} ${FF_INC} graph_search_ff.cpp -o ffgs

clean:
	@echo "Deleting elements ..."
	@rm -f gs gg ffgs