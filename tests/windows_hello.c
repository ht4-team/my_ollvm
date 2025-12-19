#include <windows.h>
#include <stdint.h>

static uint32_t fold_block(const wchar_t *text) {
    uint32_t acc = 0x9E3779B9u;
    for (size_t i = 0; text[i] != L'\0'; ++i) {
        acc ^= (uint32_t)text[i] + (acc << 5) + (acc >> 2);
        acc = (acc << 13) | (acc >> 19);
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
    format_message(text, 128, fold_block(L"ollvm-win-magic"));
    MessageBoxW(NULL, text, L"ollvm", MB_OK | MB_ICONINFORMATION);
    return (int)(text[0] & 0xFF);
}
