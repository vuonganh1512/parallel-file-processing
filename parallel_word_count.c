#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>  // Include time.h for timing functions

#define NUM_THREADS 4  // Number of threads to use

typedef struct {
    FILE *file;
    long start;
    long end;
    int word_count;
} ThreadData;

void *count_words(void *arg) {
    ThreadData *data = (ThreadData *)arg;
    fseek(data->file, data->start, SEEK_SET);

    int in_word = 0;
    char c;
    while (ftell(data->file) < data->end && (c = fgetc(data->file)) != EOF) {
        if (c == ' ' || c == '\n' || c == '\t') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            data->word_count++;
        }
    }
    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    FILE *file = fopen(argv[1], "r");
    if (!file) {
        perror("Error opening file");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    pthread_t threads[NUM_THREADS];
    ThreadData thread_data[NUM_THREADS];
    long chunk_size = file_size / NUM_THREADS;

    // Start timing
    clock_t start_time = clock();

    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].file = file;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i == NUM_THREADS - 1) ? file_size : (i + 1) * chunk_size;
        thread_data[i].word_count = 0;

        if (pthread_create(&threads[i], NULL, count_words, &thread_data[i]) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }

    int total_word_count = 0;
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
        total_word_count += thread_data[i].word_count;
    }

    // End timing
    clock_t end_time = clock();
    double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("Total word count: %d\n", total_word_count);
    printf("Processing time: %.4f seconds\n", time_taken);

    fclose(file);
    return 0;
}