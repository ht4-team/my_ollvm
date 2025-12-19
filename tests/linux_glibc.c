#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    uint64_t s0;
    uint64_t s1;
    uint64_t rounds;
} cipher_state;

static inline uint64_t rotl(uint64_t v, unsigned c) {
    return (v << (c & 63)) | (v >> ((64 - c) & 63));
}

static uint64_t mix_round(uint64_t v, uint64_t salt) {
    v ^= salt;
    v = rotl(v * 0x9E3779B185EBCA87ULL, 23);
    v ^= (v >> 29) * 0x94D049BB133111EBULL;
    return v ^ rotl(v, 19);
}

static void scramble(cipher_state *state, uint64_t *buf, size_t len) {
    for (size_t r = 0; r < state->rounds; ++r) {
        for (size_t i = 0; i < len; ++i) {
            uint64_t salt = rotl(state->s0 + (uint64_t)i, r + 1);
            buf[i] = mix_round(buf[i], salt) ^ state->s1;
            state->s0 ^= buf[i] + salt;
            state->s1 = rotl(state->s1 + buf[i], (unsigned)(i + r));
        }
    }
}

static uint64_t checksum(const uint64_t *buf, size_t len) {
    uint64_t acc = 0;
    for (size_t i = 0; i < len; ++i) {
        acc ^= rotl(buf[i], (unsigned)i);
        acc += 0xD6E8FEB86659FD93ULL;
        acc = mix_round(acc, 0xA5A5A5A5A5A5A5A5ULL ^ i);
    }
    return acc;
}

int main(void) {
    uint64_t data[8];
    for (size_t i = 0; i < 8; ++i) {
        data[i] = 0xDEADBEEFCAFEBABEULL ^ (0x1234567890ABCDEFULL * (i + 1));
    }

    cipher_state state = {
        .s0 = 0xF00DABAD1234BEEFULL,
        .s1 = 0xABCDEF0123456789ULL,
        .rounds = 5
    };

    scramble(&state, data, 8);
    uint64_t sig = checksum(data, 8);

    char rendered[8 * 16 + 1];
    char *cursor = rendered;
    for (size_t i = 0; i < 8; ++i) {
        cursor += sprintf(cursor, "%016llx", (unsigned long long)data[i]);
    }
    *cursor = '\0';

    printf("glibc scramble signature: %016llx\n", (unsigned long long)sig);
    printf("obfuscated buffer: %s\n", rendered);

    const uint64_t expected_sig = 0x9f0fc0c7602698d6ULL;
    if (sig != expected_sig) {
        fprintf(stderr,
                "Signature mismatch: expected %016llx but got %016llx\n",
                (unsigned long long)expected_sig,
                (unsigned long long)sig);
        return 1;
    }

    return 0;
}
