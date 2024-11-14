#include <sys/time.h>  // For gettimeofday()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <zlib.h>  // For gzip compression

#define MAX_WORD_LENGTH 100
#define HASH_SIZE 10007  // A prime number for hash table size
#define NUM_THREADS 4     // Number of threads to use

typedef struct HashNode {
    char word[MAX_WORD_LENGTH];
    int count;
    struct HashNode *next;
} HashNode;

typedef struct {
    char *chunk;
    long size;
    int word_count;
    int line_count;
    HashNode *local_hash_table[HASH_SIZE];  // Thread-local hash table
} ThreadData;

HashNode *global_hash_table[HASH_SIZE] = {NULL};  // Global hash table
pthread_mutex_t hash_table_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex for global hash table

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

void merge_hash_tables(HashNode **local_table) {
    for (int i = 0; i < HASH_SIZE; i++) {
        HashNode *node = local_table[i];
        while (node) {
            pthread_mutex_lock(&hash_table_mutex);
            insert_word(global_hash_table, node->word);
            pthread_mutex_unlock(&hash_table_mutex);
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

void *count_words_and_lines(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    int in_word = 0;
    data->word_count = 0;
    data->line_count = 0;
    memset(data->local_hash_table, 0, sizeof(data->local_hash_table));

    char word[MAX_WORD_LENGTH];
    int index = 0;

    for (long i = 0; i < data->size; i++) {
        char c = data->chunk[i];
        if (isspace(c)) {
            if (in_word) {
                word[index] = '\0';
                insert_word(data->local_hash_table, word);
                index = 0;
                in_word = 0;
                data->word_count++;
            }
            if (c == '\n') data->line_count++;
        } else {
            if (!in_word) in_word = 1;
            word[index++] = tolower(c);
        }
    }
    if (in_word) {
        word[index] = '\0';
        insert_word(data->local_hash_table, word);
        data->word_count++;
    }

    pthread_exit(NULL);
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

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    long chunk_size = file_size / NUM_THREADS;

    double start_time = get_wall_time();

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].chunk = file_content + (i * chunk_size);
        thread_data[i].size = (i == NUM_THREADS - 1) ? file_size - i * chunk_size : chunk_size;

        if (i > 0) {
            while (!isspace(*(thread_data[i].chunk - 1)) && thread_data[i].size > 0) {
                thread_data[i].chunk++;
                thread_data[i].size--;
            }
        }

        if (pthread_create(&threads[i], NULL, count_words_and_lines, &thread_data[i]) != 0) {
            perror("Failed to create thread");
            free(file_content);
            exit(1);
        }
    }

    int total_word_count = 0, total_line_count = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_word_count += thread_data[i].word_count;
        total_line_count += thread_data[i].line_count;
        merge_hash_tables(thread_data[i].local_hash_table);
    }

    double end_time = get_wall_time();
    double processing_time = end_time - start_time;

    printf("Total word count: %d\n", total_word_count);
    printf("Total line count: %d\n", total_line_count);
    printf("Processing time: %.4f seconds\n", processing_time);

    // Writing duplicate words
    double write_start_time = get_wall_time();
    store_duplicate_words_to_file();
    double write_end_time = get_wall_time();
    printf("Writing time: %.4f seconds\n", write_end_time - write_start_time);

    // Compression
    double compress_start_time = get_wall_time();
    compress_file("parallel_duplicate_word_count.txt", "parallel_duplicate_word_count.txt.gz");
    double compress_end_time = get_wall_time();
    printf("Compression time: %.4f seconds\n", compress_end_time - compress_start_time);

    free_hash_table(global_hash_table);
    free(file_content);
    return 0;
}
