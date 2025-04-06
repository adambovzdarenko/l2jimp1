# Dokumentacja Kodowa: Program Generujący Grafy

Niniejsza "dokumentacja kodowa" zawiera szczegółowy opis dostarczonego programu w języku C, który generuje skierowane grafy reprezentowane jako macierze sąsiedztwa. Program oferuje elastyczność w tworzeniu grafów poprzez dane wejściowe użytkownika (strukturalne lub oparte na czacie) i wspiera zarówno lokalne algorytmy, jak i generowanie oparte na API za pomocą dużego modelu językowego (LLM). Kod źródłowy jest podzielony na kilka plików: `main.c`, `api_comm.c`, `graph_generator.c`, `graph_matrix.c` oraz `utils.c`. Dokument ten ma na celu ułatwienie zrozumienia struktury, funkcjonalności i kluczowych komponentów kodu, aby można go było zaimplementować i przetestować.

---

## Przegląd

Program generuje skierowany graf w postaci macierzy sąsiedztwa na podstawie wyborów użytkownika. Obsługuje dwa główne tryby wprowadzania danych:

1. **Dane Strukturalne**: Użytkownicy określają liczbę wierzchołków i krawędzi wprost lub proszą o losowy graf, generowany lokalnie lub za pomocą LLM.
2. **Dane Oparte na Czacie**: Użytkownicy podają dane w języku naturalnym, które są przetwarzane w pełni przez LLM lub częściowo ekstrahowane przez LLM i uzupełniane lokalnie.

Program wykorzystuje bibliotekę `libcurl` do komunikacji z API (punktem końcowym LLM), dynamicznie alokuje pamięć dla macierzy sąsiedztwa, wyświetla wynik na konsoli i odpowiednio zwalnia zasoby.

---

## Podział na Pliki

### 1. `main.c` - Punkt Wejścia Programu

#### Cel

Jest to główny sterownik programu, odpowiadający za interakcję z użytkownikiem, walidację danych wejściowych i organizację generowania grafów.

#### Kluczowe Komponenty

- **Dołączone Biblioteki**: Standardowe biblioteki (`stdio.h`, `stdlib.h`, `string.h`, `ctype.h`) oraz niestandardowe nagłówki (`graph_generator.h`, `api_comm.h`, `graph_matrix.h`, `utils.h`).
- **Stałe**: `MAX_INPUT` (512) określa maksymalną długość ciągów wejściowych użytkownika.
- **Funkcja `get_input()`**:
    - Przyjmuje komunikat, wczytuje linię danych do bufora za pomocą `fgets()` i usuwa końcowy znak nowej linii.
    - Kończy działanie z błędem w przypadku niepowodzenia wczytania danych.
- **Funkcja `main()`**:
    - Inicjalizuje ziarno losowe za pomocą `srand(time(NULL))` i konfiguruje uchwyt CURL do komunikacji z API.
    - Prosi użytkownika o wybór między danymi strukturalnymi (1) a opartymi na czacie (2).
    - **Dane Strukturalne (Wybór 1)**:
        - Pyta o liczbę wierzchołków, czy graf ma być losowy czy określony przez użytkownika, oraz metodę generowania (lokalna lub LLM).
        - Dla losowych grafów:
            - Lokalnie: Wywołuje `generate_random_graph()`.
            - LLM: Wysyła liczbę wierzchołków do API (tryb 2) i parsuje odpowiedź.
        - Dla grafów określonych przez użytkownika:
            - Przyjmuje dane krawędzi (np. "A->B, B->C").
            - Lokalnie: Wywołuje `generate_user_defined_graph()`.
            - LLM: Wysyła liczbę wierzchołków i krawędzie do API (tryb 1) i parsuje odpowiedź.
    - **Dane Oparte na Czacie (Wybór 2)**:
        - Przyjmuje swobodny wniosek (np. "Stwórz graf z 3 wierzchołkami i A->B").
        - Pyta, jak przetworzyć:
            - Pełne generowanie przez LLM (tryb 1): Wysyła dane do API i parsuje odpowiedź.
            - Ekstrakcja przez LLM + lokalny algorytm (tryb 0): Ekstrahuje częściowe dane z API i losowo wypełnia nieokreślone krawędzie.
    - Weryfikuje wszystkie dane wejściowe i obsługuje błędy (np. nieprawidłowe wybory, niepowodzenia API).
    - Wyświetla wynikową macierz za pomocą `print_adjacency_matrix()` i zwalnia zasoby.

#### Przepływ

1. Użytkownik wybiera tryb wprowadzania danych (1 lub 2).
2. W zależności od trybu zbiera dodatkowe dane (wierzchołki, krawędzie lub swobodny wniosek).
3. Wybiera metodę generowania (lokalna lub LLM).
4. Generuje macierz sąsiedztwa.
5. Wyświetla i sprząta.

---

### 2. `api_comm.c` - Komunikacja z API

#### Cel

Obsługuje komunikację z zewnętrznym API (LLM) za pomocą `libcurl`, wysyłając zapytania użytkownika i odbierając odpowiedzi w postaci macierzy sąsiedztwa.

#### Kluczowe Komponenty

- **Dołączone Biblioteki**: `api_comm.h`, standardowe biblioteki.
- **Funkcja `write_callback()`**:
    - Funkcja zwrotna dla `libcurl` do obsługi danych odpowiedzi.
    - Dynamicznie realokuje pamięć, aby zapisać odpowiedź w strukturze `struct Memory` (`response` i `size`).
    - Zwraca liczbę obsłużonych bajtów lub 0 w przypadku niepowodzenia.
- **Funkcja `send_request()`**:
    - Przyjmuje uchwyt CURL, zapytanie użytkownika i tryb (0, 1 lub 2).
    - Tworzy ładunek JSON z:
        - Nazwą modelu (`MODEL_NAME` z nagłówka).
        - Systemowym komunikatem określającym format wyjściowy (`Vertices=n>>>matrix`).
        - Zapytaniem użytkownika.
    - **Tryby**:
        - `mode 0`: Ekstrahuje krawędzie z danych, używa `F` dla nieokreślonych połączeń (lokalny algorytm wypełnia je później).
        - `mode 1`: W pełni generuje macierz na podstawie danych wejściowych.
        - `mode 2`: Generuje losową macierz dla podanej liczby wierzchołków.
    - Konfiguruje żądanie HTTP POST z nagłówkami JSON i wysyła je.
    - Zwraca surową odpowiedź jako ciąg znaków lub NULL w przypadku niepowodzenia.

#### Uwagi

- Komunikat systemowy narzuca ścisły format wyjściowy: `Vertices=n>>>a11a12...|a21a22...|...`.
- API powinno zwracać macierze z `0`/`1` dla krawędzi, `X` dla błędów lub `F` dla nieokreślonych krawędzi (tryb 0).

---

### 3. `graph_generator.c` - Logika Generowania Grafów

#### Cel

Implementuje lokalne algorytmy generowania grafów losowych i zdefiniowanych przez użytkownika oraz parsuje odpowiedzi ekstrahowane z API.

#### Kluczowe Komponenty

- **Dołączone Biblioteki**: `graph_generator.h`, standardowe biblioteki.
- **Stałe**: `MAX_VERTICES` (26) zakłada, że wierzchołki są oznaczane od A do Z.
- **Funkcja `generate_random_graph()`**:
    - Przyjmuje liczbę wierzchołków (`n`) i tworzy macierz `n x n`.
    - Losowo przypisuje `0` lub `1` do każdej komórki za pomocą `rand() % 2`.
    - Obsługuje błędy alokacji pamięci.
- **Funkcja `get_vertex_index()`**:
    - Mapuje etykietę wierzchołka (np. "A") na indeks (np. 0).
    - Zwraca -1 dla nieprawidłowych etykiet.
- **Funkcja `trim_whitespace()`**:
    - Usuwa początkowe i końcowe spacje z ciągu (używane przy parsowaniu krawędzi).
- **Funkcja `generate_user_defined_graph()`**:
    - Przyjmuje `n` i ciąg krawędzi (np. "A->B, B->C").
    - Inicjalizuje macierz `n x n` wypełnioną zerami.
    - Parsuje krawędzie za pomocą `strtok()` i `strstr()`, dzieląc na "->".
    - Mapuje wierzchołki na indeksy i ustawia odpowiednie komórki macierzy na `1`.
    - Weryfikuje dane wejściowe i obsługuje błędy (np. nieprawidłowe wierzchołki).
- **Funkcja `create_matrix_from_extracted()`**:
    - Przyjmuje odpowiedź API (np. `Vertices=3>>>F1F|FF1|FFF`) i `n`.
    - Tworzy macierz `n x n`.
    - Parsuje ciąg macierzy, wypełniając:
        - `F`: Losowe `0` lub `1`.
        - `0` lub `1`: Zgodnie z określonymi wartościami.
    - Weryfikuje długości wierszy i ich liczbę.

#### Uwagi

- Wszystkie funkcje zwracają strukturę `AdjacencyMatrix` (`matrix` i `n`), z `matrix` ustawionym na NULL w przypadku niepowodzenia.

---

### 4. `graph_matrix.c` - Parsowanie i Zarządzanie Macierzą

#### Cel

Parsuje odpowiedzi API na macierze sąsiedztwa i dostarcza funkcje pomocnicze do wyświetlania i zwalniania pamięci.

#### Kluczowe Komponenty

- **Dołączone Biblioteki**: `graph_matrix.h`, standardowe biblioteki.
- **Funkcja `parse_adjacency_matrix()`**:
    - Przyjmuje odpowiedź JSON z API.
    - Wyodrębnia pole `"content"` i parsuje `Vertices=n>>>matrix`.
    - Alokuje macierz `n x n` i wypełnia ją danymi z ciągu (wiersze oddzielone `|`).
    - Obsługuje błędy (np. `X` dla niemożliwych połączeń).
- **Funkcja `print_adjacency_matrix()`**:
    - Wyświetla macierz w czytelnym formacie (wartości oddzielone spacjami, nowa linia dla każdego wiersza).
- **Funkcja `free_adjacency_matrix()`**:
    - Zwalnia całą dynamicznie zaalokowaną pamięć macierzy.

#### Uwagi

- Zakłada, że API otacza macierz w strukturze JSON z polem `"content"`.

---

### 5. `utils.c` - Funkcje Pomocnicze

#### Cel

Dostarcza funkcje pomocnicze do parsowania i walidacji.

#### Kluczowe Komponenty

- **Dołączone Biblioteki**: `utils.h`, standardowe biblioteki.
- **Funkcja `parse_vertex_count()`**:
    - Konwertuje ciąg na liczbę całkowitą (`atoi`) i zapewnia, że jest dodatnia.
    - Zwraca -1 w przypadku niepowodzenia.

---

## Podsumowanie Przepływu Programu

1. **Start**: `main()` inicjalizuje CURL i ziarno generatora losowego.
2. **Wybór Użytkownika**:
    - Strukturalny: Zbiera wierzchołki, specyfikacje krawędzi i metodę generowania.
    - Oparty na czacie: Zbiera swobodne dane wejściowe i metodę przetwarzania.
3. **Generowanie Grafu**:
    - Lokalne: Używa `generate_random_graph()` lub `generate_user_defined_graph()`.
    - LLM: Wysyła żądanie za pomocą `send_request()` i parsuje z `parse_adjacency_matrix()` lub `create_matrix_from_extracted()`.
4. **Wyjście**: Wyświetla macierz i sprząta.

---

## Kluczowe Struktury Danych

- **`AdjacencyMatrix`** (zdefiniowana w `graph_matrix.h`):
    - `int **matrix`: Dwuwymiarowa tablica liczb całkowitych (0 lub 1).
    - `int n`: Liczba wierzchołków.
- **`struct Memory`** (zdefiniowana w `api_comm.h`):
    - `char *response`: Bufor odpowiedzi API.
    - `size_t size`: Rozmiar bufora.

---

## Obsługa Błędów

- Niepowodzenia alokacji pamięci: Funkcje zwracają macierze NULL i wyświetlają błędy.
- Walidacja danych wejściowych: Sprawdza nieprawidłowe wybory, liczby wierzchołków i formaty krawędzi.
- Niepowodzenia API: Zwraca NULL i kończy z komunikatami o błędach.

---

## Zależności i Konfiguracja

### Wymaganie CURL

Program opiera się na bibliotece `libcurl` do komunikacji z API, co jest kluczowe dla generowania grafów za pomocą LLM. Bez `libcurl` żądania API w `api_comm.c` i `main.c` nie powiodą się.

#### Użycie CURL

- **Inicjalizacja**: `curl_easy_init()` tworzy uchwyt CURL.
- **Konfiguracja**: `curl_easy_setopt()` ustala opcje, takie jak `API_URL`, metoda POST, nagłówki i funkcje zwrotne.
- **Wykonanie**: `curl_easy_perform()` wysyła żądania, a `write_callback()` obsługuje odpowiedzi.
- **Sprzątanie**: `curl_easy_cleanup()` i `curl_slist_free_all()` zwalniają zasoby.

#### Konfiguracja CURL

Aby skompilować i uruchomić kod, zainstaluj i podłącz `libcurl`:

##### Na Linux (np. Ubuntu)
1. **Instalacja**:
   ```bash
   sudo apt update
   sudo apt install libcurl4-openssl-dev
   ```
   2.**Kompilacja**:
   ```bash
   gcc -o graph_gen main.c api_comm.c graph_generator.c graph_matrix.c utils.c -lcurl
   ```
#### Uwaga: Powyższe dotyczy MinGW; dla MSVC podłącz libcurl.lib z odpowiednimi nagłówkami.
##### Na Windows
1. **Instalacja**:
   ```vcpkg install curl```
2. **Kompilacja**:
   ```MinGW: gcc -o graph_gen main.c api_comm.c graph_generator.c graph_matrix.c utils.c -lcurl```
   
## Budowanie i Uruchamianie
#### Istnieją dwa sposoby kompilacji i uruchomienia kodu:
#### Użycie pliku CMakeLists.txt:
#### Dołączony plik CMakeLists.txt ułatwia budowanie projektu, np. w środowisku IDE.
## Wymagania
#### Nagłówki: Wymaga <curl/curl.h> (dołączonego przez api_comm.h).
#### Stałe: API_URL i MODEL_NAME (w api_comm.h) muszą być zdefiniowane w nagłówkach.

#### Aby zbudować:
##### Utwórz katalog build w folderze projektu.
##### Przejdź do build i wykonaj:
```
cmake ..
cmake --build .
```
###### Wynikowy plik wykonywalny (L2JIMP2.exe) pojawi się w folderze build.

#### Ręczna Kompilacja:
##### Użyj polecenia gcc jak pokazano w sekcji "Konfiguracja CURL" dla odpowiedniego systemu operacyjnego.