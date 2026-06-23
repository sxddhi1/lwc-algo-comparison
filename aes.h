#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>  // for uint8_t

// AES constants
#define AES_BLOCKLEN 16  // 16 bytes = 128 bits per block
#define AES_KEYLEN   16  // 16 bytes = 128-bit key
#define AES_keyExpSize 176

// AES context structure (stores round keys)
struct AES_ctx {
    uint8_t RoundKey[AES_keyExpSize];
};

// ===== Function Prototypes =====

// Initialize AES context with a given key
void AES_init_ctx(struct AES_ctx* ctx, const uint8_t* key);

// Encrypt one block (16 bytes) of data using AES-128 in ECB mode
void AES_ECB_encrypt(const struct AES_ctx* ctx, uint8_t* buf);

// Decrypt one block (16 bytes) of data using AES-128 in ECB mode
void AES_ECB_decrypt(const struct AES_ctx* ctx, uint8_t* buf);

#endif // _AES_H_
