# my_ollvm

Unified OLLVM toolchain image that bundles:

- Obfuscation-enabled LLVM/Clang toolchains for both Linux (glibc + musl) and MinGW/Windows targets.
- Prebuilt `llvm-mingw` distribution patched with sr-tream/obfuscator so every compilation pass supports OLLVM options.
- Rust toolchain with linux-musl, linux-gnu and `x86_64-pc-windows-gnu` targets, ready for cross builds.
- Musl toolchain and headers so Linux binaries can be shipped with fully static runtimes for maximum compatibility.

## Building & publishing (GitHub Actions)

- `build.yml` patches the upstream [`llvm-mingw`](https://github.com/mstorsjo/llvm-mingw) sources, builds the Docker image remotely and pushes it to `zhangdafei1995/my_ollvm` using `DOCKER_USER`/`DOCKER_TOKEN`.
- `test.yml` runs automatically after a successful build, pulls the freshly published image and performs Linux glibc/musl, Windows mingw and Rust smoke tests to make sure all entry points keep working.
- Monitor the builds with `gh run watch --workflow build` and `gh run watch --workflow test` (use `pm gh ...` if the proxy helper is needed). Do **not** run any heavy docker build locally; everything is delegated to Actions.
- Buildx uses a shared `gha` cache (`scope=ollvm-build`) so routine pushes只重新执行差异层；绝大多数文档/脚本改动能在几分钟内完成一次轻量构建。

## Using the image

```
docker pull zhangdafei1995/my_ollvm:latest
docker run --rm -it zhangdafei1995/my_ollvm:latest bash
# Linux/glibc
clang -mllvm -sub -o hello tests/linux_glibc.c
# Linux/musl static
clang --target=x86_64-linux-musl --sysroot /usr/x86_64-linux-musl -static tests/linux_glibc.c -o hello-musl
# Windows / MinGW
x86_64-w64-mingw32-clang -mllvm -bcf tests/windows_hello.c -o hello.exe -luser32
# Rust
cargo build --target x86_64-unknown-linux-musl
```

More detailed Chinese usage notes (covering musl/glibc, MinGW 和 Rust 项目) can be found in [`docs/usage_guide.zh.md`](docs/usage_guide.zh.md).

## credits
[Obfuscation LLVM 18](https://github.com/sr-tream/obfuscator)

[Obfuscation LLVM 17](https://github.com/BloodSharp/ollvm17)

[DreamSoule's ollvm17](https://github.com/DreamSoule/ollvm17)

[LLVM14 and Later](https://github.com/heroims/obfuscator)

[An LLVM/Clang/LLD based mingw-w64 toolchain](https://github.com/mstorsjo/llvm-mingw.git)

[Obfuscator-LLVM](https://github.com/obfuscator-llvm/obfuscator)

[LLVM](https://github.com/llvm/llvm-project)
