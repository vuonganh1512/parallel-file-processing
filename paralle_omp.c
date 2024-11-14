#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <omp.h>    // For OpenMP and omp_get_wtime()
#include <zlib.h>   // For gzip compression

#define MAX_WORD_LENGTH 100
#define HASH_SIZE 10007  // Prime number for hash table size

typedef struct HashNode {
    char word[MAX_WORD_LENGTH];
    int count;
    struct HashNode *next;
} HashNode;

HashNode *global_hash_table[HASH_SIZE] = {NULL};

unsigned int hash(const char *str) {
    unsigned int hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + tolower(*str);  // djb2 hash function
        str++;
    }
    return hash % HASH_SIZE;
}

void insert_word(HashNode **hash_table, const char *word) {
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

void merge_local_to_global(HashNode **local_table) {
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode *node = local_table[i];
        while (node) {
            #pragma omp critical
            insert_word(global_hash_table, node->word);
            node = node->next;
        }
    }
}

void free_hash_table(HashNode **hash_table) {
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode *node = hash_table[i];
        while (node) {
            HashNode *temp = node;
            node = node->next;
            free(temp);
        }
    }
}

void store_duplicate_words_to_file() {
    FILE *output_file = fopen("parallel_duplicate_word_count.txt", "w");
    if (!output_file) {
        perror("Error opening output file");
        return;
    }

    fprintf(output_file, "Duplicate Words:\n");
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode *node = global_hash_table[i];
        while (node) {
            if (node->count > 1) {
                fprintf(output_file, "%s: %d times\n", node->word, node->count);
            }
            node = node->next;
        }
    }

    fclose(output_file);
    printf("\nDuplicate words stored in 'parallel_duplicate_word_count.txt'.\n");
}

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

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    FILE *file = fopen(argv[1], "rb");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *file_content = (char *)malloc(file_size);
    if (!file_content) {
        perror("Error allocating memory");
        fclose(file);
        exit(1);
    }

    fread(file_content, 1, file_size, file);
    fclose(file);

    int total_word_count = 0;
    int total_line_count = 0;

    double start_time = omp_get_wtime();

    #pragma omp parallel
    {
        HashNode *local_hash_table[HASH_SIZE] = {NULL};
        int local_word_count = 0;
        int local_line_count = 0;

        #pragma omp for reduction(+:total_word_count, total_line_count)
        for (long i = 0; i < file_size; i++) {
            char c = file_content[i];
            if (isspace(c)) {
                if (c == '\n') local_line_count++;
            } else {
                char word[MAX_WORD_LENGTH];
                int index = 0;
                while (i < file_size && !isspace(file_content[i]) && index < MAX_WORD_LENGTH - 1) {
                    word[index++] = tolower(file_content[i++]);
                }
                word[index] = '\0';
                local_word_count++;
                insert_word(local_hash_table, word);
            }
        }

        #pragma omp atomic
        total_word_count += local_word_count;

        #pragma omp atomic
        total_line_count += local_line_count;

        merge_local_to_global(local_hash_table);
        free_hash_table(local_hash_table);
    }

    double end_time = omp_get_wtime();
    double processing_time = end_time - start_time;

    printf("Total word count: %d\n", total_word_count);
    printf("Total line count: %d\n", total_line_count);
    printf("Processing time: %.4f seconds\n", processing_time);

    double write_start_time = omp_get_wtime();
    store_duplicate_words_to_file();
    double write_end_time = omp_get_wtime();
    printf("Writing time: %.4f seconds\n", write_end_time - write_start_time);

    double compress_start_time = omp_get_wtime();
    compress_file("parallel_duplicate_word_count.txt", "parallel_duplicate_word_count.txt.gz");
    double compress_end_time = omp_get_wtime();
    printf("Compression time: %.4f seconds\n", compress_end_time - compress_start_time);

    free_hash_table(global_hash_table);
    free(file_content);
    return 0;
}
