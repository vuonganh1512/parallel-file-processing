#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>  // For gettimeofday()
#include <zlib.h>      // For gzip compression

#define MAX_WORD_LENGTH 100
#define HASH_SIZE 10007  // A prime number for hash table size

// Hash node structure for storing duplicate words
typedef struct HashNode {
    char word[MAX_WORD_LENGTH];
    int count;
    struct HashNode *next;
} HashNode;

HashNode *hash_table[HASH_SIZE] = {NULL};

// Hash function for strings
unsigned int hash(const char *str) {
    unsigned int hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + tolower(*str);  // djb2 hash function
        str++;
    }
    return hash % HASH_SIZE;
}

// Insert or increment word count in hash table
void insert_word(const char *word) {
    unsigned int index = hash(word);
    HashNode *node = hash_table[index];
    while (node) {
        if (strcmp(node->word, word) == 0) {
            node->count++;
            return;
        }
        node = node->next;
    }
    node = (HashNode *)malloc(sizeof(HashNode));
    strcpy(node->word, word);
    node->count = 1;
    node->next = hash_table[index];
    hash_table[index] = node;
}

// Free hash table memory
void free_hash_table() {
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode *node = hash_table[i];
        while (node) {
            HashNode *temp = node;
            node = node->next;
            free(temp);
        }
    }
}

// Word count and line count function
void process_file(FILE *file, int *word_count, int *line_count) {
    char word[MAX_WORD_LENGTH] = {0};
    int index = 0;
    char c;
    *word_count = 0;
    *line_count = 0;

    while ((c = fgetc(file)) != EOF) {
        if (isalpha(c)) {
            word[index++] = tolower(c);
        } else {
            if (index > 0) {
                word[index] = '\0';
                (*word_count)++;
                insert_word(word);
                index = 0;
            }
            if (c == '\n') {
                (*line_count)++;
            }
        }
    }
    // Handle last word if file does not end with whitespace
    if (index > 0) {
        word[index] = '\0';
        (*word_count)++;
        insert_word(word);
    }
}

void store_duplicate_words_to_file() {
    FILE *output_file = fopen("duplicate_word_count.txt", "w");
    if (!output_file) {
        perror("Error opening output file");
        return;
    }

    fprintf(output_file, "Duplicate Words:\n");
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode *node = hash_table[i];
        while (node) {
            if (node->count > 1) {
                fprintf(output_file, "%s: %d times\n", node->word, node->count);
            }
            node = node->next;
        }
    }

    fclose(output_file);
    printf("\nDuplicate words stored in 'duplicate_word_count.txt'.\n");
}

// Function to compress a file using zlib
void compress_file(const char *source, const char *destination) {
    FILE *src = fopen(source, "rb");
    if (!src) {
        perror("Error opening source file for compression");
        return;
    }

    gzFile dest = gzopen(destination, "wb");
    if (!dest) {
        perror("Error opening destination file for compression");
        fclose(src);
        return;
    }

    char buffer[1024];
    int bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        gzwrite(dest, buffer, bytes_read);
    }

    fclose(src);
    gzclose(dest);

    printf("Compressed file saved as '%s'\n", destination);
}

// Function to get current wall clock time in seconds
double get_wall_time() {
    struct timeval time;
    if (gettimeofday(&time, NULL)) {
        perror("Failed to get wall time");
        return 0;
    }
    return time.tv_sec + (time.tv_usec * 1e-6);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening input file");
        exit(1);
    }

    int word_count = 0, line_count = 0;

    // Start timing for processing
    double start_time = get_wall_time();

    // Process file to count words, lines, and detect duplicates
    process_file(file, &word_count, &line_count);

    // End timing for processing
    double end_time = get_wall_time();
    double processing_time = end_time - start_time;

    printf("Total word count: %d\n", word_count);
    printf("Total line count: %d\n", line_count);
    printf("Processing time: %.4f seconds\n", processing_time);

    fclose(file);

    // Store duplicate words
    double write_start_time = get_wall_time();
    store_duplicate_words_to_file();
    double write_end_time = get_wall_time();
    printf("Writing time: %.4f seconds\n", write_end_time - write_start_time);

    // Compress the output file
    double compress_start_time = get_wall_time();
    compress_file("duplicate_word_count.txt", "duplicate_word_count.txt.gz");
    double compress_end_time = get_wall_time();
    printf("Compression time: %.4f seconds\n", compress_end_time - compress_start_time);

    free_hash_table();
    return 0;
}
