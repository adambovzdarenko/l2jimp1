#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph_generator.h"
#include "api_comm.h"
#include "graph_matrix.h"
#include "utils.h"
#include "csrrg.h"

#define MAX_INPUT 512

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1] + strlen(argv[1]) - 6, ".csrrg") == 0) {
        return processCsrrgFile(argv[1]);
    } else if (argc == 2) {
        printf("Invalid file format, use .csrrg to convert it to .txt");
    } else {
        srand(time(NULL));
        CURL *curl = curl_easy_init();
        if (!curl) {
            fprintf(stderr, "CURL initialization failed\n");
            return 1;
        }
        curl_easy_setopt(curl, CURLOPT_URL, API_URL);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);

        printf("Choose how to create the graph:\n");
        printf("1. Structured input (specify vertices and generation method)\n");
        printf("2. Chat-based input (natural language instructions via LLM)\n");
        printf("Enter 1 or 2: ");
        char choice[MAX_INPUT];
        fgets(choice, MAX_INPUT, stdin);
        choice[strcspn(choice, "\n")] = 0;

        AdjacencyMatrix matrix;
        if (strcmp(choice, "1") == 0) {
            // Structured input
            printf("Enter the number of vertices: ");
            char vertex_input[MAX_INPUT];
            fgets(vertex_input, MAX_INPUT, stdin);
            vertex_input[strcspn(vertex_input, "\n")] = 0;

            int n = parseVertexCount(vertex_input);
            if (n < 0) {
                curl_easy_cleanup(curl);
                return 1;
            }

            printf("Should the graph be random or user-specified? (random/user): ");
            char spec_choice[MAX_INPUT];
            fgets(spec_choice, MAX_INPUT, stdin);
            spec_choice[strcspn(spec_choice, "\n")] = 0;

            printf("Generate with: a) Local algorithm, b) LLM? (a/b): ");
            char gen_choice[MAX_INPUT];
            fgets(gen_choice, MAX_INPUT, stdin);
            gen_choice[strcspn(gen_choice, "\n")] = 0;

            if (strcasecmp(spec_choice, "random") == 0) {
                if (strcasecmp(gen_choice, "a") == 0) {
                    matrix = generate_random_graph(n);
                } else if (strcasecmp(gen_choice, "b") == 0) {
                    char prompt[MAX_INPUT];
                    snprintf(prompt, sizeof(prompt), "%d", n); // Just send number of vertices
                    char *response = send_request(curl, prompt, 2); // Random mode
                    if (response) {
                        matrix = parseAdjacencyMatrix(response);
                        free(response);
                    } else {
                        fprintf(stderr, "API communication error\n");
                        curl_easy_cleanup(curl);
                        return 1;
                    }
                } else {
                    fprintf(stderr, "Invalid generation choice\n");
                    curl_easy_cleanup(curl);
                    return 1;
                }
            } else if (strcasecmp(spec_choice, "user") == 0) {
                printf("Enter edges (e.g., 'A->B, B->C'), or leave empty for no edges: ");
                char edge_input[MAX_INPUT];
                fgets(edge_input, MAX_INPUT, stdin);
                edge_input[strcspn(edge_input, "\n")] = 0;

                if (strcasecmp(gen_choice, "a") == 0) {
                    matrix = generate_user_defined_graph(n, edge_input);
                } else if (strcasecmp(gen_choice, "b") == 0) {
                    char prompt[MAX_INPUT];
                    snprintf(prompt, sizeof(prompt), "%d %s", n, edge_input);
                    char *response = send_request(curl, prompt, 1); // Generate mode
                    if (response) {
                        //printf("API Response:\n%s\n", response);
                        matrix = parseAdjacencyMatrix(response);
                        free(response);
                    } else {
                        fprintf(stderr, "API communication error\n");
                        curl_easy_cleanup(curl);
                        return 1;
                    }
                } else {
                    fprintf(stderr, "Invalid generation choice\n");
                    curl_easy_cleanup(curl);
                    return 1;
                }
            } else {
                fprintf(stderr, "Invalid specification choice\n");
                curl_easy_cleanup(curl);
                return 1;
            }
        } else if (strcmp(choice, "2") == 0) {
            // Chat-based input
            printf("Enter your request (e.g., 'Create a graph with 5 vertices, A->B, B->C'):\n");
            char user_input[MAX_INPUT];
            fgets(user_input, MAX_INPUT, stdin);
            user_input[strcspn(user_input, "\n")] = 0;

            printf("Process with: a) LLM full generation, b) LLM extraction + local algorithm? (a/b): ");
            char proc_choice[MAX_INPUT];
            fgets(proc_choice, MAX_INPUT, stdin);
            proc_choice[strcspn(proc_choice, "\n")] = 0;

            if (strcasecmp(proc_choice, "a") == 0) {
                char *response = send_request(curl, user_input, 1); // Generate explicit only mode
                if (response) {
                    matrix = parseAdjacencyMatrix(response);
                    free(response);
                } else {
                    fprintf(stderr, "API communication error\n");
                    curl_easy_cleanup(curl);
                    return 1;
                }
            } else if (strcasecmp(proc_choice, "b") == 0) {
                int n = parseVertexCount(user_input);
                if (n < 0) {
                    fprintf(stderr, "Could not determine number of vertices\n");
                    curl_easy_cleanup(curl);
                    return 1;
                }
                char *response = send_request(curl, user_input, 0); // Extract mode
                if (response) {
                    matrix = create_matrix_from_extracted(response, n);
                    free(response);
                } else {
                    fprintf(stderr, "API communication error\n");
                    curl_easy_cleanup(curl);
                    return 1;
                }
            } else {
                fprintf(stderr, "Invalid processing choice\n");
                curl_easy_cleanup(curl);
                return 1;
            }
        } else {
            fprintf(stderr, "Invalid option. Please enter 1 or 2.\n");
            curl_easy_cleanup(curl);
            return 1;
        }

        if (matrix.matrix == NULL) {
            fprintf(stderr, "Failed to create graph\n");
            curl_easy_cleanup(curl);
            return 1;
        }
        printAdjacencyMatrix(&matrix);
        freeAdjacencyMatrix(&matrix);

        curl_easy_cleanup(curl);
        return 0;
    }
}
