#ifndef GRAPH_GENERATOR_H
#define GRAPH_GENERATOR_H

#include "graph_matrix.h"

AdjacencyMatrix generate_random_graph(int n);
AdjacencyMatrix generate_user_defined_graph(int n, const char *edges);
AdjacencyMatrix create_matrix_from_extracted(const char *response, int n);

#endif