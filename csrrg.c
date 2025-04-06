#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "graph_matrix.h"
#include "utils.h"

int processCsrrgFile(const char *fileName) {
    FILE *f = fopen(fileName, "r");
    if (!f) {
        fprintf(stderr, "Error opening file %s\n", fileName);
        return -4;
    }

    /* --- Header Parsing (Lines 1-3) ---
       Line 1: Maximum possible number of nodes in a row.
       Line 2: A semicolon-separated list of node indices for each row.
       Line 3: Pointers (cumulative indices) for the first index in each row (used to compute the count of nodes per row).
    */
    int maxRowNodes = 0;
    char line[1048576];
    char *line2 = NULL;  // For node indices (line 2)
    char *headerLine3 = NULL;  // Temporary buffer for line 3 processing

    // Dynamic array for row counts (derived from differences in header line 3)
    int capacityRows = 15;
    int *rowCounts = malloc(capacityRows * sizeof(int));
    if (!rowCounts) {
        fprintf(stderr, "Memory allocation error for rowCounts\n");
        fclose(f);
        return -2;
    }
    int numRows = 0;
    int vertexTotal = 0;   // Total number of indices (last cumulative value)

    // Read header lines (skip empty lines)
    int headerLineCount = 0;
    while (headerLineCount < 3 && fgets(line, sizeof(line), f)) {
        if (isEmptyLine(line))
            continue;
        headerLineCount++;
        if (headerLineCount == 1) {
            // Line 1: maximum nodes in a row.
            maxRowNodes = atoi(line);
        } else if (headerLineCount == 2) {
            // Line 2: copy node indices list.
            line2 = strdup(line);
            if (!line2) {
                fprintf(stderr, "Memory allocation error for line2\n");
                free(rowCounts);
                fclose(f);
                return -2;
            }
        } else if (headerLineCount == 3) {
            // Line 3: pointers for first indices in each row.
            headerLine3 = strdup(line);
            if (!headerLine3) {
                fprintf(stderr, "Memory allocation error for headerLine3\n");
                free(rowCounts);
                free(line2);
                fclose(f);
                return -2;
            }
            // Process headerLine3: the tokens are cumulative indices.
            int i = 0, prev = 0;
            char *token = strtok(headerLine3, ";");
            while (token) {
                int current = atoi(token);
                if (i > 0) {
                    int count = current - prev;
                    if (numRows >= capacityRows) {
                        capacityRows *= 2;
                        int *temp = realloc(rowCounts, capacityRows * sizeof(int));
                        if (!temp) {
                            fprintf(stderr, "Memory allocation error during rowCounts realloc\n");
                            free(rowCounts);
                            free(line2);
                            free(headerLine3);
                            fclose(f);
                            return -5;
                        }
                        rowCounts = temp;
                    }
                    rowCounts[numRows++] = count;
                }
                prev = current;
                token = strtok(NULL, ";");
                i++;
            }
            vertexTotal = prev;  // Last cumulative value equals total indices.
            free(headerLine3);
        }
    }
    if (headerLineCount < 3) {
        fprintf(stderr, "Insufficient header lines in file\n");
        free(rowCounts);
        free(line2);
        fclose(f);
        return -1;
    }

    /* --- Build the Adjacency Matrix ---
       We use:
         - numRows: the number of rows (from header line 3 differences).
         - maxRowNodes: used as the number of columns (assuming nodes are numbered 0 to maxRowNodes-1).
    */
    // Calculate actual number of columns based on max node index
    int maxNodeIndex = -1;
    char *line2CopyForMax = strdup(line2);
    if (!line2CopyForMax) {
        fprintf(stderr, "Memory allocation error for line2CopyForMax\n");
        free(rowCounts);
        free(line2);
        fclose(f);
        return -1;
    }
    char *token = strtok(line2CopyForMax, ";");
    while (token) {
        int nodeIndex = atoi(token);
        if (nodeIndex > maxNodeIndex) {
            maxNodeIndex = nodeIndex;
        }
        token = strtok(NULL, ";");
    }
    free(line2CopyForMax);
    int columns = maxNodeIndex + 1;  // Actual columns needed

    AdjacencyMatrix adjacencyMatrix;
    adjacencyMatrix.n = numRows;
    adjacencyMatrix.matrix = malloc(numRows * sizeof(int *));
    if (!adjacencyMatrix.matrix) {
        fprintf(stderr, "Error allocating memory for matrix\n");
        free(rowCounts);
        free(line2);
        fclose(f);
        return -1;
    }
    for (int i = 0; i < numRows; i++) {
        adjacencyMatrix.matrix[i] = calloc(columns, sizeof(int));
        if (!adjacencyMatrix.matrix[i]) {
            fprintf(stderr, "Error allocating memory for matrix row %d\n", i);
            for (int j = 0; j < i; j++) {
                free(adjacencyMatrix.matrix[j]);
            }
            free(adjacencyMatrix.matrix);
            free(rowCounts);
            free(line2);
            fclose(f);
            return -1;
        }
    }

    /* Process line2: fill the matrix.
       For each row, there are rowCounts[row] tokens.
       Each token is an index; set the corresponding matrix cell to 1.
    */
    if (line2 != NULL) {
        char *line2Copy = strdup(line2);
        if (!line2Copy) {
            fprintf(stderr, "Memory allocation error for line2Copy\n");
            freeAdjacencyMatrix(&adjacencyMatrix);
            free(rowCounts);
            free(line2);
            fclose(f);
            return -1;
        }
        char *token = strtok(line2Copy, ";");
        for (int row = 0; row < numRows; row++) {
            int count = rowCounts[row];
            for (int j = 0; j < count; j++) {
                if (!token) break;  // error: not enough tokens
                int nodeIndex = atoi(token);
                if (nodeIndex >= 0 && nodeIndex < columns) {
                    adjacencyMatrix.matrix[row][nodeIndex] = 1;
                }
                token = strtok(NULL, ";");
            }
        }
        free(line2Copy);
        free(line2);
    }

    /* --- Open output file ---
       All printing is done before closing.
    */
    FILE *result = fopen("graf.txt", "w");
    if (!result) {
        fprintf(stderr, "Error opening output file graf.txt\n");
        freeAdjacencyMatrix(&adjacencyMatrix);
        free(rowCounts);
        fclose(f);
        return -3;
    }
    // Print the adjacency matrix once.
    printAdjacencyMatrixToFile(result, &adjacencyMatrix, columns);

    /* --- Process one or more edge groups sections ---
       After the header (3 lines), the file contains pairs of non-empty lines:
         First of the pair (edgeLine):  list of nodes forming groups (edges).
         Second of the pair (pointerLine): pointers to the first node in each group.
    */
    while (1) {
        char *edgeLine = NULL;
        char *pointerLine = NULL;
        // line 4 of a section
        while (fgets(line, sizeof(line), f)) {
            if (!isEmptyLine(line)) {
                edgeLine = strdup(line);
                break;
            }
        }
        if (!edgeLine) {
            // No more edge sections.
            break;
        }
        // line 5 of the section
        while (fgets(line, sizeof(line), f)) {
            if (!isEmptyLine(line)) {
                pointerLine = strdup(line);
                break;
            }
        }
        if (!pointerLine) {
            fprintf(stderr, "Incomplete edge section found\n");
            free(edgeLine);
            break;
        }

        /* Parse pointerLine to get connection counts for this section.
           The pointers are cumulative indices for the tokens in edgeLine.
        */
        int capacityConnGroups = 15;
        int *connCounts = malloc(capacityConnGroups * sizeof(int));
        if (!connCounts) {
            fprintf(stderr, "Memory allocation error for connCounts\n");
            free(edgeLine);
            free(pointerLine);
            break;
        }
        int numConnGroups = 0;
        {
            int i = 0, prev = 0;
            char *pointerCopy = strdup(pointerLine);
            if (!pointerCopy) {
                free(edgeLine);
                free(pointerLine);
                free(connCounts);
                break;
            }
            char *token = strtok(pointerCopy, ";");
            while (token) {
                int current = atoi(token);
                if (i > 0) {
                    int count = current - prev;
                    if (numConnGroups >= capacityConnGroups) {
                        capacityConnGroups *= 2;
                        int *temp = realloc(connCounts, capacityConnGroups * sizeof(int));
                        if (!temp) {
                            free(connCounts);
                            free(pointerCopy);
                            free(edgeLine);
                            free(pointerLine);
                            fclose(result);
                            fclose(f);
                            freeAdjacencyMatrix(&adjacencyMatrix);
                            free(rowCounts);
                            return -5;
                        }
                        connCounts = temp;
                    }
                    connCounts[numConnGroups] = count;
                    numConnGroups++;
                }
                prev = current;
                token = strtok(NULL, ";");
                i++;
            }
            free(pointerCopy);
        }

        /* Process edgeLine.
           For each connection group (0..numConnGroups-1), read connCounts[group] tokens.
           The first token in each group is the source node; the subsequent tokens are destination nodes.
           Each edge is printed in the format "src - dest".
        */
        {
            char *edgeCopy = strdup(edgeLine);
            if (!edgeCopy) {
                free(edgeLine);
                free(pointerLine);
                free(connCounts);
                break;
            }
            char *token = strtok(edgeCopy, ";");
            int group = 0;
            while (token && group < numConnGroups) {
                int connections = connCounts[group];
                int src = -1;
                for (int k = 0; k < connections; k++) {
                    if (!token)
                        break;
                    int value = atoi(token);
                    if (k == 0) {
                        src = value;
                    } else {
                        int dest = value;
                        fprintf(result, "%d - %d\n", src, dest);
                    }
                    token = strtok(NULL, ";");
                }
                group++;
            }
            free(edgeCopy);
        }
        free(edgeLine);
        free(pointerLine);
        free(connCounts);
    }

    //Cleanup
    fclose(result);
    fclose(f);
    free(rowCounts);
    freeAdjacencyMatrix(&adjacencyMatrix);

    return 0;
}
