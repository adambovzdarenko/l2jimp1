#include "graph_generator.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_VERTICES 26  // Assume max number of vertices is 26 (A-Z)

AdjacencyMatrix generate_random_graph(int n) {
    AdjacencyMatrix matrix = {NULL, n};
    matrix.matrix = malloc(n * sizeof(int *));
    if (!matrix.matrix) {
        fprintf(stderr, "Memory allocation error\n");
        return matrix;
    }
    for (int i = 0; i < n; i++) {
        matrix.matrix[i] = malloc(n * sizeof(int));
        if (!matrix.matrix[i]) {
            fprintf(stderr, "Memory allocation error for row\n");
            while (i > 0) free(matrix.matrix[--i]);
            free(matrix.matrix);
            matrix.matrix = NULL;
            return matrix;
        }
        for (int j = 0; j < n; j++) {
            matrix.matrix[i][j] = rand() % 2; // Random 0 or 1
        }
    }
    return matrix;
}

// Function to map vertex labels (A, B, C, ...) to indices (0, 1, 2, ...)
int get_vertex_index(const char *vertex) {
    if (strlen(vertex) != 1 || vertex[0] < 'A' || vertex[0] > 'Z') {
        return -1;  // Invalid vertex
    }
    return vertex[0] - 'A';  // A=0, B=1, etc.
}

// Function to trim whitespace from a string
static void trim_whitespace(char *str) {
    char *start = str;
    char *end;
    while (isspace((unsigned char)*start)) start++;  // Skip leading spaces
    if (*start == 0) {
        *str = '\0';  // Empty string after trimming
        return;
    }
    end = start + strlen(start) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;  // Skip trailing spaces
    *(end + 1) = '\0';  // Null terminate
    memmove(str, start, end - start + 2);  // Move trimmed string to start
}

AdjacencyMatrix generate_user_defined_graph(int n, const char *edges) {
    AdjacencyMatrix matrix = {NULL, n};
    matrix.matrix = malloc(n * sizeof(int *));
    if (!matrix.matrix) {
        fprintf(stderr, "Memory allocation error\n");
        return matrix;
    }

    for (int i = 0; i < n; i++) {
        matrix.matrix[i] = calloc(n, sizeof(int));  // Zero-initialized
        if (!matrix.matrix[i]) {
            fprintf(stderr, "Memory allocation error for row\n");
            while (i > 0) free(matrix.matrix[--i]);
            free(matrix.matrix);
            matrix.matrix = NULL;
            return matrix;
        }
    }

    if (edges && edges[0] != '\0') {
        char *edges_copy = strdup(edges);
        if (!edges_copy) {
            fprintf(stderr, "Memory allocation error for edges copy\n");
            freeAdjacencyMatrix(&matrix);
            return matrix;
        }

        char *token = strtok(edges_copy, ",");
        while (token != NULL) {
            trim_whitespace(token);  // Remove leading/trailing spaces
            if (strlen(token) == 0) {
                token = strtok(NULL, ",");  // Skip empty tokens
                continue;
            }

            char *arrow = strstr(token, "->");
            if (arrow && arrow > token && arrow[2] != '\0') {
                *arrow = '\0';  // Split at "->"
                char *src = token;
                char *dest = arrow + 2;
                trim_whitespace(src);
                trim_whitespace(dest);

                if (strlen(src) == 0 || strlen(dest) == 0) {
                    fprintf(stderr, "Invalid edge format: %s->%s (missing vertex)\n", src, dest);
                } else {
                    int src_index = get_vertex_index(src);
                    int dest_index = get_vertex_index(dest);

                    if (src_index >= 0 && src_index < n && dest_index >= 0 && dest_index < n) {
                        matrix.matrix[src_index][dest_index] = 1;  // Set the edge
                    } else {
                        fprintf(stderr, "Invalid vertex in edge: %s->%s (valid range: A-%c)\n",
                                src, dest, 'A' + n - 1);
                    }
                }
            } else {
                fprintf(stderr, "Invalid edge format: %s (expected 'X->Y')\n", token);
            }
            token = strtok(NULL, ",");
        }
        free(edges_copy);
    }

    return matrix;
}

AdjacencyMatrix create_matrix_from_extracted(const char *response, int n) {
    AdjacencyMatrix matrix = {NULL, n};
    matrix.matrix = malloc(n * sizeof(int *));
    if (!matrix.matrix) {
        fprintf(stderr, "Memory allocation error\n");
        return matrix;
    }
    for (int i = 0; i < n; i++) {
        matrix.matrix[i] = calloc(n, sizeof(int)); // Zero-initialized
        if (!matrix.matrix[i]) {
            fprintf(stderr, "Memory allocation error for row\n");
            while (i > 0) free(matrix.matrix[--i]);
            free(matrix.matrix);
            matrix.matrix = NULL;
            return matrix;
        }
    }

    const char *matrix_start = strstr(response, ">>>");
    if (!matrix_start) {
        fprintf(stderr, "Invalid extracted format: '>>>' not found\n");
        freeAdjacencyMatrix(&matrix);
        return matrix;
    }
    matrix_start += strlen(">>>");

    char *matrix_copy = strdup(matrix_start);
    if (!matrix_copy) {
        fprintf(stderr, "Memory allocation error for matrix copy\n");
        freeAdjacencyMatrix(&matrix);
        return matrix;
    }

    int i = 0;
    char *line = strtok(matrix_copy, "|");
    while (line != NULL && i < n) {
        int j = 0;
        char *num = line;
        while (*num != '\0' && j < n) {
            if (*num == 'F') {
                matrix.matrix[i][j] = rand() % 2; // Random 0 or 1 for 'F'
            } else if (*num == '0' || *num == '1') {
                matrix.matrix[i][j] = (*num - '0'); // Use specified value
            } else {
                fprintf(stderr, "Invalid character in matrix: %c\n", *num);
                free(matrix_copy);
                freeAdjacencyMatrix(&matrix);
                return matrix;
            }
            j++;
            num++;
        }
        if (j != n) {
            fprintf(stderr, "Row %d has incorrect length (expected %d, found %d)\n", i, n, j);
            free(matrix_copy);
            freeAdjacencyMatrix(&matrix);
            return matrix;
        }
        i++;
        line = strtok(NULL, "|");
    }
    free(matrix_copy);

    if (i != n) {
        fprintf(stderr, "Mismatch in number of rows (expected %d, found %d)\n", n, i);
        freeAdjacencyMatrix(&matrix);
        matrix.matrix = NULL;
    }

    return matrix;
}