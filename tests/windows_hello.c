#include <windows.h>
#include <stdint.h>
#include <stddef.h>

static uint32_t fold_block(const wchar_t *text) {
    uint32_t acc = 0x9E3779B9u;
    for (size_t i = 0; text[i] != L'\0'; ++i) {
        acc ^= (uint32_t)text[i] + (acc << 5) + (acc >> 2);
        acc = (acc << 13) | (acc >> 19);
    }
    return acc;
}

static void symmetric_shuffle(uint16_t *buf, size_t len, uint32_t spice) {
    uint16_t mirror[32];
    for (size_t i = 0; i < len; ++i) {
        uint16_t rot = (uint16_t)((spice >> (i & 7)) | 1U);
        mirror[i] = rot;
        buf[i] = (uint16_t)((buf[i] ^ rot) << 1) | (uint16_t)((buf[i] ^ rot) >> 15);
    }
    for (size_t i = len; i-- > 0;) {
        uint16_t rot = mirror[i];
        uint16_t val = buf[i];
        val = (val >> 1) | (val << 15);
        buf[i] = (uint16_t)(val ^ rot);
    }
}

static uint32_t layered_signature(const wchar_t *text) {
    wchar_t shadow[32];
    size_t idx = 0;
    for (; text[idx] && idx < 30; ++idx) {
        shadow[idx] = text[idx] ^ (wchar_t)(0x55AA ^ (idx * 17));
    }
    shadow[idx++] = L'#';
    shadow[idx] = L'\0';

    uint16_t scratch[32];
    size_t copied = 0;
    for (; copied < idx && copied < 31; ++copied) {
        scratch[copied] = (uint16_t)shadow[copied];
    }
    scratch[copied] = 0;
    symmetric_shuffle(scratch, copied, 0xDEADBEEF);
    symmetric_shuffle(scratch, copied, 0xDEADBEEF);
    uint32_t acc = fold_block(text);
    for (size_t i = 0; i < copied; ++i) {
        acc ^= scratch[i] + (acc << 7);
    }
    return acc;
}

static void format_message(wchar_t *buffer, size_t cap, uint32_t sig) {
    const wchar_t *prefix = L"Obfuscated hello ";
    size_t idx = 0;
    for (; prefix[idx] && idx < cap - 1; ++idx) {
        buffer[idx] = prefix[idx];
    }
    wsprintfW(buffer + idx, L"[sig:%08X]", sig);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    wchar_t text[128];
    uint32_t sig = fold_block(L"ollvm-win-magic");
    uint32_t layered = layered_signature(L"ollvm-win-magic");
    sig ^= layered;
    sig ^= layered;
    format_message(text, 128, sig);
    MessageBoxW(NULL, text, L"ollvm", MB_OK | MB_ICONINFORMATION);
    return (int)(text[0] & 0xFF);
}
