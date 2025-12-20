#include <stdint.h>
#include <stddef.h>
#include <string.h>

#ifndef OLLVM_MARK_STR
#define OLLVM_MARK_STR "plain"
#endif

static inline uint64_t rotl64(uint64_t v, unsigned c) {
    c &= 63U;
    return (v << c) | (v >> (64U - c));
}

static inline uint64_t mix_word(uint64_t v, uint64_t salt) {
    v ^= salt;
    v = rotl64(v * 0x9E3779B185EBCA87ULL, 13);
    v ^= rotl64(v, 29);
    v ^= (v >> 31) * 0x94D049BB133111EBULL;
    return rotl64(v, 19) ^ salt;
}

static uint64_t fold_mark(void) {
    const char *text = OLLVM_MARK_STR;
    uint64_t acc = 0xCBF29CE484222325ULL;
    for (size_t i = 0; text[i] != '\0'; ++i) {
        acc ^= (uint64_t)(unsigned char)text[i];
        acc *= 0x100000001B3ULL;
        acc = rotl64(acc, (unsigned)((i * 11) & 63));
    }
    return acc;
}

uint64_t ffi_mix_state(const uint64_t *buf, size_t len, uint64_t seed) {
    uint64_t lane0 = seed ^ fold_mark();
    uint64_t lane1 = seed + 0xA5A5A5A5ULL;
    for (size_t r = 0; r < len * 3 + 7; ++r) {
        size_t idx = (r + seed + lane0) % (len ? len : 1);
        uint64_t word = buf[idx % len] ^ mix_word(lane1, r ^ seed);
        lane0 = mix_word(lane0 ^ word, seed + (uint64_t)r);
        lane1 = mix_word(lane1 + word, seed ^ (uint64_t)(idx * 41U));
    }
    return lane0 ^ lane1;
}

uint64_t ffi_mark_hash(void) {
    return fold_mark();
}
