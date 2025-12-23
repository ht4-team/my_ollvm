# my_ollvm

Unified OLLVM toolchain image that bundles:

- Obfuscation-enabled LLVM/Clang toolchains for both Linux (glibc + musl) and MinGW/Windows targets.
- Prebuilt `llvm-mingw` distribution patched with sr-tream/obfuscator so every compilation pass supports OLLVM options.
- Rust toolchain with linux-musl, linux-gnu and `x86_64-pc-windows-gnu` targets, ready for cross builds.
- Musl toolchain and headers so Linux binaries can be shipped with fully static runtimes for maximum compatibility.
- System helpers such as `upx` (with `--best --lzma` support) baked into the image so downstream workflows无需重复安装。

## Building & publishing (GitHub Actions)

- `build.yml` patches the upstream [`llvm-mingw`](https://github.com/mstorsjo/llvm-mingw) sources, builds the Docker image remotely。只有 `v*` 标签或 commit message 含 `dotest`（可在 `TEST_TRIGGER_KEYWORDS` 中自定义）的提交才会推送镜像；其它 push 只用于预热缓存。默认会先拉取 `zhangdafei1995/my_ollvm:latest` 并把它作为 Dockerfile 的 `BASE_IMAGE`，因此只需几分钟即可在已有镜像上打补丁；如果 `workflow_dispatch` 的 `rebuild_from_scratch` 为 `true` 或 tag 名称带 `-full`，workflow 会改用 `ubuntu:22.04` 作为 `BASE_IMAGE` 并将 `REBUILD_FROM_SOURCE=true`，触发完整 4h 重建。
- `test.yml` runs automatically after a successful build, pulls the freshly published image,构建使用 OLLVM 参数的 Linux/Windows/Rust 可执行文件并上传到 workflow artifacts，确保镜像不仅可编译还产出可复用的样例成果。
- 若镜像尚未包含 `gcc-mingw-w64` 系列包，`test.yml` 会在容器内按需安装，待新的带 v 标签镜像构建完毕即可去掉该兜底逻辑。
- 测试阶段会对同一份源码先编译“原味”二进制，再用大量 OLLVM `-mllvm` 选项构建一个混淆版本，并用 `cmp`/哈希比对确保二进制确实发生变化；若检测到两者完全一致就立即失败，避免无谓的 2 小时浪费。

> dotest commit 触发说明：提交信息包含 `dotest`（可在 workflow 中扩展关键字），会强制执行镜像推送与 artifacts 构建。
- Monitor the builds with `gh run watch --workflow build` and `gh run watch --workflow test` (use `pm gh ...` if the proxy helper is needed). Do **not** run any heavy docker build locally; everything is delegated to Actions.
- Buildx uses a shared `gha` cache (`scope=ollvm-build`) so routine pushes只重新执行差异层；绝大多数文档/脚本改动能在几分钟内完成一次轻量构建。`dotest` commit（用于触发测试）也受益于同一缓存层。

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
