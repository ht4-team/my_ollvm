#include <stdio.h>
#include <stdint.h>

uint32_t rotate(uint32_t v, unsigned count) {
    count &= 31;
    return (v << count) | (v >> (32 - count));
}

int main(void) {
    uint32_t input = 0x12345678u;
    printf("glibc hello %08x -> %08x\n", input, rotate(input, 7));
    return 0;
}
