#include "speck.h"
#include <string.h>
#include <stdint.h>

// Rotation helpers
static inline uint64_t ROR64(uint64_t x, unsigned r) { return (x >> r) | (x << (64 - r)); }
static inline uint64_t ROL64(uint64_t x, unsigned r) { return (x << r) | (x >> (64 - r)); }

// load/store little-endian 64-bit
static inline uint64_t load_u64_le(const uint8_t *b) {
    uint64_t x = 0;
    for (int i = 7; i >= 0; --i) { x = (x << 8) | b[i]; }
    return x;
}
static inline void store_u64_le(uint8_t *b, uint64_t x) {
    for (int i = 0; i < 8; ++i) { b[i] = (uint8_t)(x & 0xFF); x >>= 8; }
}

// SPECK-128/128 constants
enum { ALPHA = 8, BETA = 3, ROUNDS = SPECK128_ROUNDS };

// Key schedule for SPECK-128/128 (m=2, n=64)
void speck128_key_setup(const uint8_t key[16], uint64_t round_keys[SPECK128_ROUNDS]) {
    // key: 16 bytes -> k[0] (least significant word) and l[0] (next)
    uint64_t k = load_u64_le(key + 0); // k[0]
    uint64_t l = load_u64_le(key + 8); // l[0]

    round_keys[0] = k;
    // expand
    for (uint64_t i = 0; i < ROUNDS - 1; ++i) {
        // Round i -> produce round_keys[i+1] and update l
        l = (ROR64(l, ALPHA) + k) ^ i;
        k = ROL64(k, BETA) ^ l;
        round_keys[i + 1] = k;
    }
}

// Encrypt single 16-byte block in-place using round_keys
void speck128_encrypt_block(uint8_t block[SPECK_BLOCK_BYTES], const uint64_t round_keys[SPECK128_ROUNDS]) {
    uint64_t x = load_u64_le(block + 0); // left word
    uint64_t y = load_u64_le(block + 8); // right word

    for (int i = 0; i < ROUNDS; ++i) {
        x = (ROR64(x, ALPHA) + y) ^ round_keys[i];
        y = ROL64(y, BETA) ^ x;
    }

    store_u64_le(block + 0, x);
    store_u64_le(block + 8, y);
}

// Decrypt single 16-byte block in-place using round_keys
void speck128_decrypt_block(uint8_t block[SPECK_BLOCK_BYTES], const uint64_t round_keys[SPECK128_ROUNDS]) {
    uint64_t x = load_u64_le(block + 0);
    uint64_t y = load_u64_le(block + 8);

    for (int i = ROUNDS - 1; i >= 0; --i) {
        y = ROR64(y ^ x, BETA);
        x = ROL64(((x ^ round_keys[i]) - y), ALPHA);
    }

    store_u64_le(block + 0, x);
    store_u64_le(block + 8, y);
}

// Simple API wrappers that match your benchmark flow.
// Use a fixed zero key here like we did for AES; you can change it later.
void speck_encrypt(uint8_t *data, size_t len) {
    uint8_t zero_key[16] = {0};
    uint64_t rk[SPECK128_ROUNDS];
    speck128_key_setup(zero_key, rk);

    // require len to be multiple of 16 — if not, we process up to floor(len/16)*16
    size_t blocks = len / SPECK_BLOCK_BYTES;
    for (size_t i = 0; i < blocks; ++i) {
        speck128_encrypt_block(data + i * SPECK_BLOCK_BYTES, rk);
    }
}

void speck_decrypt(uint8_t *data, size_t len) {
    uint8_t zero_key[16] = {0};
    uint64_t rk[SPECK128_ROUNDS];
    speck128_key_setup(zero_key, rk);

    size_t blocks = len / SPECK_BLOCK_BYTES;
    for (size_t i = 0; i < blocks; ++i) {
        speck128_decrypt_block(data + i * SPECK_BLOCK_BYTES, rk);
    }
}
