#include <stdio.h>
#include <stdlib.h>
#include <time.h>  // Include time.h for timing functions

int count_words(FILE *file) {
    int word_count = 0;
    int in_word = 0;
    char c;

    while ((c = fgetc(file)) != EOF) {
        if (c == ' ' || c == '\n' || c == '\t') {
            in_word = 0;
        } else if (!in_word) {
            in_word = 1;
            word_count++;
        }
    }

    return word_count;
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

    // Start timing
    clock_t start_time = clock();

    // Count words sequentially
    int total_word_count = count_words(file);

    // End timing
    clock_t end_time = clock();
    double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;

    printf("Total word count: %d\n", total_word_count);
    printf("Processing time: %.4f seconds\n", time_taken);

    fclose(file);
    return 0;
}
