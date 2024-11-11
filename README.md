# Parallel File Processing in C

This project is a simple example of parallel file processing using C and POSIX threads (`pthreads`). The program counts the number of words in a large file by dividing the file into chunks and using multiple threads to process each chunk concurrently. This approach can significantly speed up word counting for large files.

## Features

- Uses multithreading to perform parallel processing
- Divides file processing workload across threads
- Demonstrates basic word counting using C and `pthreads`

## Code Overview

The program opens a text file, divides it into chunks, and assigns each chunk to a separate thread. Each thread counts the number of words in its assigned chunk, and the main thread combines these counts to get the total word count.

### How It Works

1. The file is opened and its size is determined.
2. The file is divided into equal chunks based on the number of threads.
3. Each thread processes a chunk of the file to count words.
4. The main thread combines the word counts from all threads to get the total count.

## Requirements

- GCC (for compiling the C code)
- POSIX-compliant system with `pthreads` support (e.g., Linux, macOS)

## Usage

1. **Clone the Repository**:
   ```sh
   git clone https://github.com/YOUR_USERNAME/parallel-file-processing.git
   cd parallel-file-processing
2. **Compilation**:
- To compile the program, use the following command:
   gcc parallel_word_count.c -o parallel_word_count -pthread
- Run the Program:
   ./parallel_word_count <filename>
