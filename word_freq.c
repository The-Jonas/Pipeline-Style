#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_WORD_LENGTH 100
#define MAX_STOP_WORDS 1000

typedef struct {
    char word[MAX_WORD_LENGTH];
    int count;
} WordFreq;

char* read_file(const char* path_to_file);
char* filter_chars_and_normalize(const char* str_data);
char** scan(const char* str_data, int* word_count);
void remove_stop_words(char** word_list, int* word_count);
WordFreq* frequencies(char** word_list, int word_count, int* freq_count);
int compare_word_freqs(const void* a, const void* b);
void print_all(WordFreq* word_freqs, int freq_count);

int main(int argc, char* argv[]) {
    char* file_contents = read_file(argv[1]);
    char* filtered_data = filter_chars_and_normalize(file_contents);

    int word_count = 0;
    char** word_list = scan(filtered_data, &word_count);

    remove_stop_words(word_list, &word_count);

    int freq_count = 0;
    WordFreq* word_freqs = frequencies(word_list, word_count, &freq_count);

    qsort(word_freqs, freq_count, sizeof(WordFreq), compare_word_freqs);

    print_all(word_freqs, freq_count);

    // Free allocated memory
    free(file_contents);
    free(filtered_data);
    for (int i = 0; i < word_count; i++) {
        free(word_list[i]);
    }
    free(word_list);
    free(word_freqs);

    return 0;
}

// Função para ler o conteúdo de um arquivo
char* read_file(const char* path_to_file) {
    FILE* file = fopen(path_to_file, "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char* file_contents = (char*)malloc(file_size + 1);
    if (file_contents == NULL) {
        perror("Error allocating memory");
        exit(1);
    }

    fread(file_contents, file_size, 1, file);
    file_contents[file_size] = '\0';

    fclose(file);
    return file_contents;
}

// Função para filtrar os caracteres e normalizar o texto
char* filter_chars_and_normalize(const char* str_data) {
    int length = strlen(str_data);
    char* filtered_data = (char*)malloc(length + 1);
    if (filtered_data == NULL) {
        perror("Error allocating memory");
        exit(1);
    }

    int j = 0;
    for (int i = 0; i < length; i++) {
        char c = str_data[i];
        if (isalnum(c)) {
            filtered_data[j++] = tolower(c);
        } else {
            filtered_data[j++] = ' ';
        }
    }
    filtered_data[j] = '\0';

    return filtered_data;
}

// Função para dividir o texto em palavras individuais
char** scan(const char* str_data, int* word_count) {
    int capacity = 10;
    char** word_list = (char**)malloc(capacity * sizeof(char*));
    if (word_list == NULL) {
        perror("Error allocating memory");
        exit(1);
    }

    int i = 0;
    int j = 0;

    while (str_data[i] != '\0') {
        if (str_data[i] == ' ') {
            if (j > 0) { // Verifica se temos uma palavra válida antes de adicionar
                if (*word_count >= capacity) {
                    capacity *= 2;
                    word_list = (char**)realloc(word_list, capacity * sizeof(char*));
                    if (word_list == NULL) {
                        perror("Error reallocating memory");
                        exit(1);
                    }
                }
            word_list[*word_count] = (char*)malloc(j + 1);
            if (word_list[*word_count] == NULL) {
                perror("Error allocating memory");
                exit(1);
            }
            strncpy(word_list[*word_count], &str_data[i - j], j);
            word_list[*word_count][j] = '\0';
            (*word_count)++;
            j = 0;
            }
        } else {
            j++;
        }
        i++;
    }

    return word_list;
}

int should_skip_word(const char* word, char stop_words[MAX_STOP_WORDS][MAX_WORD_LENGTH], int stop_words_count) {
    for (int j = 0; j < stop_words_count; j++) {
        if (strcmp(word, stop_words[j]) == 0) {
            return 1;
        }
    }
    return 0;
}

// Função para remover as palavras de parada do texto
void remove_stop_words(char** word_list, int* word_count) {
    FILE* stop_words_file = fopen("stop_words.txt", "r");
    if (stop_words_file == NULL) {
        perror("Error opening stop_words.txt");
        exit(1);
    }

    char stop_words[MAX_STOP_WORDS][MAX_WORD_LENGTH];
    int stop_words_count = 0;

    char line[MAX_WORD_LENGTH];

    // Verifica se o arquivo não está vazio
    if (fgets(stop_words[stop_words_count], MAX_WORD_LENGTH, stop_words_file) == NULL) {
        fclose(stop_words_file);
        printf("Nenhuma palavra de parada encontrada no arquivo stop_words.txt\n");
        return;
    }

    // Restaura a posição do arquivo para o início
    fseek(stop_words_file, 0, SEEK_SET);

    // Lê uma linha do arquivo contendo as palavras separadas por vírgula
    if (fgets(line, MAX_WORD_LENGTH, stop_words_file) != NULL) {
        char* stop_word = strtok(line, ",");
        while (stop_word != NULL) {
            // Remove espaços em branco extras no início da palavra
            while (isspace(*stop_word)) {
                stop_word++;
            }
            // Remove espaços em branco extras no final da palavra
            char* end = stop_word + strlen(stop_word) - 1;
            while (end > stop_word && isspace(*end)) {
                *end = '\0';
                end--;
            }

            strncpy(stop_words[stop_words_count], stop_word, MAX_WORD_LENGTH);
            stop_words_count++;
            stop_word = strtok(NULL, ",");
        }
    }

    fclose(stop_words_file);

    int original_word_count = *word_count;
    int write_index = 0;

    for (int i = 0; i < original_word_count; i++) {
        if (!should_skip_word(word_list[i], stop_words, stop_words_count)) {
            word_list[write_index++] = word_list[i];
        } else {
            free(word_list[i]);
            (*word_count)--;
        }
    }
}

// Função para calcular a frequência das palavras
WordFreq* frequencies(char** word_list, int word_count, int* freq_count) {
    WordFreq* word_freqs = (WordFreq*)malloc(word_count * sizeof(WordFreq));
    if (word_freqs == NULL) {
        perror("Error allocating memory");
        exit(1);
    }

    *freq_count = 0;
    for (int i = 0; i < word_count; i++) {
        int word_exists = 0;
        for (int j = 0; j < *freq_count; j++) {
            if (strcmp(word_list[i], word_freqs[j].word) == 0) {
                word_freqs[j].count++;
                word_exists = 1;
                break;
            }
        }
        if (!word_exists) {
            strncpy(word_freqs[*freq_count].word, word_list[i], MAX_WORD_LENGTH);
            word_freqs[*freq_count].count = 1;
            (*freq_count)++;
        }
    }

    return word_freqs;
}

// Função auxiliar para comparar as frequências das palavras
int compare_word_freqs(const void* a, const void* b) {
    WordFreq* word_freq1 = (WordFreq*)a;
    WordFreq* word_freq2 = (WordFreq*)b;
    return word_freq2->count - word_freq1->count;
}

// Função para imprimir todas as palavras e suas frequências
void print_all(WordFreq* word_freqs, int freq_count) {
    for (int i = 0; i < freq_count && i < 25; i++) {
    printf("%s - %d\n", word_freqs[i].word, word_freqs[i].count);
    }
}