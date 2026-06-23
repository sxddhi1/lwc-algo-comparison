#include <stdint.h>
#include <string.h>

/* Ascon-128 parameters */
#define CRYPTO_KEYBYTES 16
#define CRYPTO_NPUBBYTES 16
#define CRYPTO_ABYTES 16
#define RATE 8
#define ASCON_IV 0x80400c0600000000ULL

#define ROR(x,n) (((x) >> (n)) | ((x) << (64 - (n))))

static uint64_t load64(const uint8_t *b) {
    uint64_t x = 0;
    for (int i = 0; i < 8; i++) x |= ((uint64_t)b[i]) << (8 * i);
    return x;
}
static void store64(uint8_t *b, uint64_t x) {
    for (int i = 0; i < 8; i++) { b[i] = x & 0xFF; x >>= 8; }
}

static void P(uint64_t *x, int rounds) {
    for (int i = 12 - rounds; i < 12; i++) {
        x[2] ^= ((uint64_t)(0xf - i) << 4) | (uint64_t)i;
        x[0] ^= x[4]; x[4] ^= x[3]; x[2] ^= x[1];
        uint64_t t0 = ~x[0] & x[1];
        uint64_t t1 = ~x[1] & x[2];
        uint64_t t2 = ~x[2] & x[3];
        uint64_t t3 = ~x[3] & x[4];
        uint64_t t4 = ~x[4] & x[0];
        x[0] ^= t1; x[1] ^= t2; x[2] ^= t3; x[3] ^= t4; x[4] ^= t0;
        x[1] ^= x[0]; x[0] ^= x[4]; x[3] ^= x[2]; x[2] = ~x[2];
        x[0] ^= ROR(x[0],19) ^ ROR(x[0],28);
        x[1] ^= ROR(x[1],61) ^ ROR(x[1],39);
        x[2] ^= ROR(x[2],1)  ^ ROR(x[2],6);
        x[3] ^= ROR(x[3],10) ^ ROR(x[3],17);
        x[4] ^= ROR(x[4],7)  ^ ROR(x[4],41);
    }
}

static void pad(uint8_t *b, size_t len, size_t rate) {
    b[len] = 0x80;
    for (size_t i = len + 1; i < rate; i++) b[i] = 0;
}

/* AEAD encrypt */
int crypto_aead_encrypt(uint8_t *c, unsigned long long *clen,
                        const uint8_t *m, unsigned long long mlen,
                        const uint8_t *ad, unsigned long long adlen,
                        const uint8_t *nsec,
                        const uint8_t *npub,
                        const uint8_t *k) {
    (void)nsec;
    uint64_t K0 = load64(k), K1 = load64(k+8);
    uint64_t N0 = load64(npub), N1 = load64(npub+8);
    uint64_t S[5];
    S[0] = ASCON_IV; S[1] = K0; S[2] = K1; S[3] = N0; S[4] = N1;
    P(S,12);
    S[3] ^= K0; S[4] ^= K1;

    size_t off = 0;
    while (adlen >= RATE) {
        S[0] ^= load64(ad + off);
        P(S,6);
        off += RATE; adlen -= RATE;
    }
    uint8_t block[8] = {0};
    memcpy(block, ad + off, adlen);
    pad(block, adlen, RATE);
    S[0] ^= load64(block);
    P(S,6);
    S[4] ^= 1;

    off = 0;
    size_t rem = mlen;
    while (rem >= RATE) {
        S[0] ^= load64(m + off);
        store64(c + off, S[0]);
        P(S,6);
        off += RATE; rem -= RATE;
    }
    memset(block, 0, 8);
    memcpy(block, m + off, rem);
    pad(block, rem, RATE);
    S[0] ^= load64(block);
    store64(block, S[0]);
    memcpy(c + off, block, rem);

    c += mlen; *clen = mlen + 16;
    S[1] ^= K0; S[2] ^= K1;
    P(S,12);
    S[3] ^= K0; S[4] ^= K1;
    store64(c, S[3]); store64(c+8, S[4]);
    return 0;
}

/* AEAD decrypt */
int crypto_aead_decrypt(uint8_t *m, unsigned long long *mlen,
                        uint8_t *nsec,
                        const uint8_t *c, unsigned long long clen,
                        const uint8_t *ad, unsigned long long adlen,
                        const uint8_t *npub,
                        const uint8_t *k) {
    (void)nsec;
    if (clen < 16) return -1;
    *mlen = clen - 16;
    uint64_t K0 = load64(k), K1 = load64(k+8);
    uint64_t N0 = load64(npub), N1 = load64(npub+8);
    uint64_t S[5];
    S[0] = ASCON_IV; S[1] = K0; S[2] = K1; S[3] = N0; S[4] = N1;
    P(S,12);
    S[3] ^= K0; S[4] ^= K1;

    size_t off = 0;
    while (adlen >= RATE) {
        S[0] ^= load64(ad + off);
        P(S,6);
        off += RATE; adlen -= RATE;
    }
    uint8_t block[8] = {0};
    memcpy(block, ad + off, adlen);
    pad(block, adlen, RATE);
    S[0] ^= load64(block);
    P(S,6);
    S[4] ^= 1;

    off = 0;
    size_t rem = *mlen;
    while (rem >= RATE) {
        uint64_t cblock = load64(c + off);
        uint64_t pblock = S[0] ^ cblock;
        store64(m + off, pblock);
        S[0] = cblock;
        P(S,6);
        off += RATE; rem -= RATE;
    }
    memset(block, 0, 8);
    memcpy(block, c + off, rem);
    uint64_t tmp = load64(block);
    uint64_t pblock = S[0] ^ tmp;
    store64(block, pblock);
    memcpy(m + off, block, rem);
    pad(block, rem, RATE);
    S[0] = tmp; S[0] ^= load64(block);

    S[1] ^= K0; S[2] ^= K1;
    P(S,12);
    S[3] ^= K0; S[4] ^= K1;

    uint8_t tag[16];
    store64(tag, S[3]); store64(tag + 8, S[4]);
    if (memcmp(tag, c + *mlen, 16) != 0) return -1;
    return 0;
}
