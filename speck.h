#ifndef _SPECK_H_
#define _SPECK_H_

#include <stdint.h>
#include <stddef.h>

// SPECK-128/128 parameters
#define SPECK128_ROUNDS 32
#define SPECK_BLOCK_BYTES 16

// Initialize round keys from a 128-bit key (16 bytes)
void speck128_key_setup(const uint8_t key[16], uint64_t round_keys[SPECK128_ROUNDS]);

// Encrypt one 16-byte block in-place
void speck128_encrypt_block(uint8_t block[SPECK_BLOCK_BYTES], const uint64_t round_keys[SPECK128_ROUNDS]);

// Decrypt one 16-byte block in-place
void speck128_decrypt_block(uint8_t block[SPECK_BLOCK_BYTES], const uint64_t round_keys[SPECK128_ROUNDS]);

// Simple helpers matching your benchmark API: operate on len bytes (multiple of 16)
void speck_encrypt(uint8_t *data, size_t len);
void speck_decrypt(uint8_t *data, size_t len);

#endif // _SPECK_H_
