#ifndef GRAPH_MATRIX_H
#define GRAPH_MATRIX_H
#include <stdio.h>

// Struktura reprezentująca macierz sąsiedztwa
typedef struct AdjacencyMatrix {
    int **matrix;  // Dwuwymiarowa tablica reprezentująca macierz
    int n;         // Liczba wierzchołków
} AdjacencyMatrix;

// Funkcje do przetwarzania macierzy
AdjacencyMatrix parseAdjacencyMatrix(const char *json_response);
void printAdjacencyMatrix(const AdjacencyMatrix *matrix);
void printAdjacencyMatrixToFile(FILE *file, const AdjacencyMatrix *matrix, int columns);
void printConnectionsToFile(FILE *file, const AdjacencyMatrix *matrix);
void freeAdjacencyMatrix(AdjacencyMatrix *matrix);

#endif // GRAPH_MATRIX_H
