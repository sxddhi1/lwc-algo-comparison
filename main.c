#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include "aes.h"
#include "speck.h"

// Forward declaration for ASCON (from ascon128_ref.c)
int crypto_aead_encrypt(uint8_t *c, unsigned long long *clen,
                        const uint8_t *m, unsigned long long mlen,
                        const uint8_t *ad, unsigned long long adlen,
                        const uint8_t *nsec,
                        const uint8_t *npub,
                        const uint8_t *k);

// Global file pointer for results
FILE *results_file = NULL;

// Encryption wrapper functions
void aes_encrypt(uint8_t *data, size_t len) {
    struct AES_ctx ctx;
    uint8_t key[16] = {0};
    AES_init_ctx(&ctx, key);

    for (size_t i = 0; i < len; i += 16) {
        AES_ECB_encrypt(&ctx, data + i);
    }
}

void ascon_encrypt(uint8_t *data, size_t len) {
    uint8_t key[16] = {0};
    uint8_t nonce[16] = {0};
    uint8_t ciphertext[2048];
    unsigned long long clen;

    crypto_aead_encrypt(ciphertext, &clen, data, len, NULL, 0, NULL, nonce, key);
    memcpy(data, ciphertext, len);
}

// Benchmark structure
typedef struct {
    double time_taken;
    double throughput_mbps;
    size_t iterations;
} BenchmarkResult;

// Enhanced timer utility with throughput calculation
BenchmarkResult measure_performance(void (*encrypt_func)(uint8_t*, size_t), 
                                   uint8_t *data, size_t len, int runs) {
    BenchmarkResult result = {0};
    uint8_t *temp = malloc(len);
    if (!temp) {
        fprintf(stderr, "Memory allocation failed!\n");
        return result;
    }

    clock_t start = clock();
    for (int i = 0; i < runs; i++) {
        memcpy(temp, data, len);
        encrypt_func(temp, len);
    }
    clock_t end = clock();
    
    result.time_taken = ((double)(end - start)) / CLOCKS_PER_SEC;
    result.iterations = runs;
    
    // Calculate throughput in MB/s
    double total_bytes = (double)len * runs;
    result.throughput_mbps = (total_bytes / (1024.0 * 1024.0)) / result.time_taken;
    
    free(temp);
    return result;
}

// Print results in a formatted table (to both console and file)
void print_results(const char *algo_name, BenchmarkResult result, size_t msg_len) {
    double avg_time_per_op = (result.time_taken / result.iterations) * 1000000; // microseconds
    
    // Print to console
    printf("%-12s | %6zu bytes | %10d runs | %8.3f s | %10.2f MB/s | %8.2f us/op\n",
           algo_name, msg_len, (int)result.iterations, result.time_taken, 
           result.throughput_mbps, avg_time_per_op);
    
    // Print to file
    if (results_file) {
        fprintf(results_file, "%-12s | %6zu bytes | %10d runs | %8.3f s | %10.2f MB/s | %8.2f us/op\n",
               algo_name, msg_len, (int)result.iterations, result.time_taken, 
               result.throughput_mbps, avg_time_per_op);
    }
}

void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Dual print function (console + file)
void dual_print(const char *format, ...) {
    va_list args1, args2;
    va_start(args1, format);
    va_copy(args2, args1);
    
    vprintf(format, args1);
    if (results_file) {
        vfprintf(results_file, format, args2);
        fflush(results_file); // Ensure immediate write
    }
    
    va_end(args1);
    va_end(args2);
}

int main() {
    // Open results file
    results_file = fopen("results.txt", "w");
    if (!results_file) {
        fprintf(stderr, "Warning: Could not open results.txt for writing. Results will only show in console.\n");
    } else {
        printf("Results will be saved to: results.txt\n");
    }

    dual_print("\n========================================================================\n");
    dual_print("       Lightweight Cryptography Performance Benchmark (PC)             \n");
    dual_print("                        Interactive Mode                               \n");
    dual_print("========================================================================\n\n");

    int runs = 50000;  // Default number of iterations
    char choice;

    printf("Default iterations: %d\n", runs);
    printf("Do you want to change the number of iterations? (y/n): ");
    scanf(" %c", &choice);
    
    if (choice == 'y' || choice == 'Y') {
        printf("Enter number of iterations (recommended: 10000-100000): ");
        scanf("%d", &runs);
        if (runs < 1000) {
            printf("Warning: Less than 1000 iterations may give unreliable results.\n");
        }
    }
    clear_input_buffer();

    dual_print("\n========================================================================\n\n");

    while (1) {
        size_t msg_len;
        
        printf("Enter message size in bytes (or 0 to exit): ");
        if (scanf("%zu", &msg_len) != 1) {
            printf("Invalid input! Please enter a number.\n");
            clear_input_buffer();
            continue;
        }
        clear_input_buffer();

        if (msg_len == 0) {
            dual_print("\nExiting benchmark program. Goodbye!\n");
            break;
        }

        // Validate message size
        if (msg_len % 16 != 0) {
            printf("\nWarning: Message size should be a multiple of 16 bytes for proper encryption.\n");
            printf("Rounding up to nearest multiple of 16...\n");
            msg_len = ((msg_len + 15) / 16) * 16;
            printf("Adjusted message size: %zu bytes\n", msg_len);
        }

        if (msg_len > 4096) {
            printf("Warning: Large message size. This may take a while...\n");
        }

        // Prepare test data
        uint8_t *message = malloc(msg_len);
        if (!message) {
            fprintf(stderr, "Memory allocation failed for message size %zu\n", msg_len);
            continue;
        }
        
        // Fill with sequential test data
        for (size_t i = 0; i < msg_len; i++) {
            message[i] = (uint8_t)(i & 0xFF);
        }

        dual_print("\n------------------------------------------------------------------------\n");
        dual_print("Testing with message size: %zu bytes | Iterations: %d\n", msg_len, runs);
        dual_print("------------------------------------------------------------------------\n");
        dual_print("Algorithm    | Message Size | Iterations |   Time   | Throughput  | Avg Time\n");
        dual_print("------------------------------------------------------------------------\n");

        // Benchmark AES
        printf("Testing AES-128...\n");
        BenchmarkResult aes_result = measure_performance(aes_encrypt, message, msg_len, runs);
        print_results("AES-128", aes_result, msg_len);

        // Benchmark SPECK
        printf("Testing SPECK-128...\n");
        BenchmarkResult speck_result = measure_performance(speck_encrypt, message, msg_len, runs);
        print_results("SPECK-128", speck_result, msg_len);

        // Benchmark ASCON
        printf("Testing ASCON-128...\n");
        BenchmarkResult ascon_result = measure_performance(ascon_encrypt, message, msg_len, runs);
        print_results("ASCON-128", ascon_result, msg_len);

        dual_print("------------------------------------------------------------------------\n\n");

        // Determine fastest
        double fastest_time = aes_result.time_taken;
        const char *fastest_name = "AES-128";
        double fastest_throughput = aes_result.throughput_mbps;
        
        if (speck_result.time_taken < fastest_time) {
            fastest_time = speck_result.time_taken;
            fastest_name = "SPECK-128";
            fastest_throughput = speck_result.throughput_mbps;
        }
        if (ascon_result.time_taken < fastest_time) {
            fastest_time = ascon_result.time_taken;
            fastest_name = "ASCON-128";
            fastest_throughput = ascon_result.throughput_mbps;
        }

        dual_print(">> FASTEST: %s\n", fastest_name);
        dual_print("   Time: %.3f seconds | Throughput: %.2f MB/s\n\n", fastest_time, fastest_throughput);

        // Calculate speed improvements
        dual_print("Performance Comparison:\n");
        dual_print("  SPECK vs AES: %.2fx %s\n", 
               aes_result.time_taken / speck_result.time_taken,
               speck_result.time_taken < aes_result.time_taken ? "faster" : "slower");
        dual_print("  ASCON vs AES: %.2fx %s\n", 
               aes_result.time_taken / ascon_result.time_taken,
               ascon_result.time_taken < aes_result.time_taken ? "faster" : "slower");
        dual_print("  SPECK vs ASCON: %.2fx %s\n\n", 
               ascon_result.time_taken / speck_result.time_taken,
               speck_result.time_taken < ascon_result.time_taken ? "faster" : "slower");

        free(message);

        dual_print("========================================================================\n\n");
        printf("Press Enter to test another message size...\n");
        getchar();
    }

    dual_print("\n========================================================================\n");
    dual_print("                          Benchmark Complete!                          \n");
    dual_print("========================================================================\n\n");

    // Close results file
    if (results_file) {
        fclose(results_file);
        printf("\nResults saved to: results.txt\n");
    }

    return 0;
}