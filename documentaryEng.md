# Code Documentary: Graph Generation Program

This "code documentary" provides a detailed walkthrough of the provided C program, which generates directed graphs represented as adjacency matrices. The program offers flexibility in graph creation through user input (structured or chat-based) and supports both local algorithms and API-driven generation via a Large Language Model (LLM). The codebase is split across multiple files: `main.c`, `api_comm.c`, `graph_generator.c`, `graph_matrix.c`, and `utils.c`.
This document is designed to help understand the code's structure, functionality, and key components so that it can be implemented and tested
---

## Overview

The program generates a directed graph as an adjacency matrix based on user choices. It supports two primary input modes:

1. **Structured Input**: Users specify the number of vertices and edges explicitly or request a random graph, with generation handled locally or via an LLM.
2. **Chat-Based Input**: Users provide natural language input, processed either fully by an LLM or partially extracted by an LLM and completed locally.

The program uses the `libcurl` library to communicate with an API (LLM Endpoint) and dynamically allocates memory for the adjacency matrix. The resulting matrix is printed to the console, and all resources are cleaned up properly.

---

## File-by-File Breakdown

### 1. `main.c` - Program Entry Point

#### Purpose

This is the main driver of the program, handling user interaction, input validation, and orchestration of graph generation.

#### Key Components

- **Includes**: Standard libraries (`stdio.h`, `stdlib.h`, `string.h`, `ctype.h`) and custom headers (`graph_generator.h`, `api_comm.h`, `graph_matrix.h`, `utils.h`).
- **Constants**: `MAX_INPUT` (512) defines the maximum length of user input strings.
- **`get_input()` Function**:
    - Takes a prompt, reads a line of input into a buffer using `fgets()`, and removes the trailing newline.
    - Exits with an error if input fails.
- **`main()` Function**:
    - Initializes random seed with `srand(time(NULL))` and sets up a CURL handle for API communication.
    - Prompts the user to choose between structured (1) or chat-based (2) input.
    - **Structured Input (Choice 1)**:
        - Asks for the number of vertices, whether the graph is random or user-specified, and the generation method (local or LLM).
        - For random graphs:
            - Local: Calls `generate_random_graph()`.
            - LLM: Sends vertex count to API (mode 2) and parses the response.
        - For user-specified graphs:
            - Takes edge input (e.g., "A->B, B->C").
            - Local: Calls `generate_user_defined_graph()`.
            - LLM: Sends vertex count and edges to API (mode 1) and parses the response.
    - **Chat-Based Input (Choice 2)**:
        - Takes a free-form request (e.g., "Create a graph with 3 vertices and A->B").
        - Asks how to process it:
            - LLM full generation (mode 1): Sends input to API and parses the response.
            - LLM extraction + local algorithm (mode 0): Extracts partial data from API and fills unspecified edges randomly.
    - Validates all inputs and handles errors (e.g., invalid choices, API failures).
    - Prints the resulting matrix with `print_adjacency_matrix()` and frees resources.

#### Flow

1. User selects input mode (1 or 2).
2. Based on mode, collects additional input (vertices, edges, or free-form request).
3. Chooses generation method (local or LLM).
4. Generates the adjacency matrix.
5. Prints and cleans up.

---

### 2. `api_comm.c` - API Communication

#### Purpose

Handles communication with an external API (an LLM) using `libcurl` to send user prompts and receive adjacency matrix responses.

#### Key Components

- **Includes**: `api_comm.h`, standard libraries.
- **`write_callback()` Function**:
    - A callback for `libcurl` to handle response data.
    - Dynamically reallocates memory to store the response in a `struct Memory` (`response` and `size`).
    - Returns the number of bytes handled or 0 on failure.
- **`send_request()` Function**:
    - Takes a CURL handle, user prompt, and mode (0, 1, or 2).
    - Constructs a JSON payload with:
        - Model name (`MODEL_NAME` from header).
        - System prompt defining the expected output format (`Vertices=n>>>matrix`).
        - User prompt.
    - **Modes**:
        - `mode 0`: Extracts edges from input, uses `F` for unspecified connections (local algorithm fills these later).
        - `mode 1`: Fully generates the matrix based on input.
        - `mode 2`: Generates a random matrix for the given vertex count.
    - Sets up HTTP POST with JSON headers and sends the request.
    - Returns the raw response string or NULL on failure.

#### Notes

- The system prompt enforces a strict output format: `Vertices=n>>>a11a12...|a21a22...|...`.
- The API is expected to return matrices with `0`/`1` for edges, `X` for errors, or `F` for unspecified edges (mode 0).

---

### 3. `graph_generator.c` - Graph Generation Logic

#### Purpose

Implements local graph generation algorithms for random and user-defined graphs, plus parsing extracted API responses.

#### Key Components

- **Includes**: `graph_generator.h`, standard libraries.
- **Constants**: `MAX_VERTICES` (26) assumes vertices are labeled A-Z.
- **`generate_random_graph()`**:
    - Takes the number of vertices (`n`) and creates an `n x n` matrix.
    - Randomly assigns `0` or `1` to each cell using `rand() % 2`.
    - Handles memory allocation errors.
- **`get_vertex_index()`**:
    - Maps a vertex label (e.g., "A") to an index (e.g., 0).
    - Returns -1 for invalid labels.
- **`trim_whitespace()`**:
    - Removes leading and trailing spaces from a string (used in edge parsing).
- **`generate_user_defined_graph()`**:
    - Takes `n` and an edge string (e.g., "A->B, B->C").
    - Initializes an `n x n` zero-filled matrix.
    - Parses edges using `strtok()` and `strstr()` to split at "->".
    - Maps vertices to indices and sets corresponding matrix cells to `1`.
    - Validates input and handles errors (e.g., invalid vertices).
- **`create_matrix_from_extracted()`**:
    - Takes an API response (e.g., `Vertices=3>>>F1F|FF1|FFF`) and `n`.
    - Creates an `n x n` matrix.
    - Parses the matrix string, filling:
        - `F`: Random `0` or `1`.
        - `0` or `1`: As specified.
    - Validates row lengths and counts.

#### Notes

- All functions return an `AdjacencyMatrix` struct (`matrix` and `n`), with `matrix` set to NULL on failure.

---

### 4. `graph_matrix.c` - Matrix Parsing and Management

#### Purpose

Parses API responses into adjacency matrices and provides utility functions for printing and freeing them.

#### Key Components

- **Includes**: `graph_matrix.h`, standard libraries.
- **`parse_adjacency_matrix()`**:
    - Takes a JSON response from the API.
    - Extracts the `"content"` field and parses `Vertices=n>>>matrix`.
    - Allocates an `n x n` matrix and fills it from the string (rows separated by `|`).
    - Handles errors (e.g., `X` for impossible connections).
- **`print_adjacency_matrix()`**:
    - Prints the matrix in a readable format (space-separated values, newline per row).
- **`free_adjacency_matrix()`**:
    - Frees all dynamically allocated memory in the matrix.

#### Notes

- Assumes the API wraps the matrix in a JSON structure with a `"content"` field.

---

### 5. `utils.c` - Utility Functions

#### Purpose

Provides helper functions for parsing and validation.

#### Key Components

- **Includes**: `utils.h`, standard libraries.
- **`parse_vertex_count()`**:
    - Converts a string to an integer (`atoi`) and ensures it’s positive.
    - Returns -1 on failure.

---

## Program Flow Summary

1. **Start**: `main()` initializes CURL and seeds the random number generator.
2. **User Choice**:
    - Structured: Collects vertices, edge specs, and generation method.
    - Chat-Based: Collects free-form input and processing method.
3. **Graph Generation**:
    - Local: Uses `generate_random_graph()` or `generate_user_defined_graph()`.
    - LLM: Sends a request via `send_request()` and parses with `parse_adjacency_matrix()` or `create_matrix_from_extracted()`.
4. **Output**: Prints the matrix and cleans up.

---

## Key Data Structures

- **`AdjacencyMatrix`** (defined in `graph_matrix.h`):
    - `int **matrix`: 2D array of integers (0 or 1).
    - `int n`: Number of vertices.
- **`struct Memory`** (defined in `api_comm.h`):
    - `char *response`: API response buffer.
    - `size_t size`: Buffer size.

---

## Error Handling

- Memory allocation failures: Functions return NULL matrices and print errors.
- Input validation: Checks for invalid choices, vertex counts, and edge formats.
- API failures: Returns NULL responses and exits with error messages.

---

## Dependencies and Setup

### CURL Requirement

The program relies on the `libcurl` library for API communication, essential for LLM-based graph generation. Without `libcurl`, API requests in `api_comm.c` and `main.c` will fail.

#### CURL Usage

- **Initialization**: `curl_easy_init()` creates a CURL handle.
- **Configuration**: `curl_easy_setopt()` sets options like `API_URL`, POST method, headers, and callbacks.
- **Execution**: `curl_easy_perform()` sends requests, with `write_callback()` handling responses.
- **Cleanup**: `curl_easy_cleanup()` and `curl_slist_free_all()` free resources.

#### Setting Up CURL

To compile and run the code, first install and link `libcurl`:

##### On Linux (e.g., Ubuntu)
1. **Install**:
   ```bash
   sudo apt update
   sudo apt install libcurl4-openssl-dev
   ```
   2.**Compile**:
   ```bash
   gcc -o graph_gen main.c api_comm.c graph_generator.c graph_matrix.c utils.c -lcurl
   ```
##### On Windows
1. **Install**:
   ```vcpkg install curl```
2. **Compile**:
   ```MinGW: gcc -o graph_gen main.c api_comm.c graph_generator.c graph_matrix.c utils.c -lcurl```
## Building and Running

### There are two ways to compile and run the code:

#### Using the CMakeLists.txt File
The included `CMakeLists.txt` file simplifies building the project, e.g., within an IDE.

### Requirements
- **Headers**: Requires `<curl/curl.h>` (included via `api_comm.h`).
- **Constants**: `API_URL` and `MODEL_NAME` (in `api_comm.h`) must be defined in the headers.

#### To Build:
1. Create a `build` directory in the project folder.
2. Navigate to the `build` directory and execute:
   ```bash
   cmake ..
   cmake --build .
    ```
3. The resulting executable (L2JIMP2.exe) will appear in the build folder.

#### Manual Compilation
Use the gcc command as shown in the "CURL Configuration" section for the appropriate operating system.