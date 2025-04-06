#include "utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int parseVertexCount(const char *input) {
    int n = atoi(input);
    if (n <= 0) {
        fprintf(stderr, "Number of vertices must be positive\n");
        return -1;
    }
    return n;
}

// Function to check if a line is empty (only whitespace or newline)
int isEmptyLine(const char *str) {
    while (*str) {
        if (!isspace((unsigned char)*str)) return 0;  // Found non-whitespace character
        str++;
    }
    return 1;  // Only whitespace found
}