# OLLVM 镜像使用说明

> 镜像：`zhangdafei1995/my_ollvm`  
> 适用：Linux（glibc/musl）、Windows（MinGW）、Rust 并启用 OLLVM 混淆

## 0. 镜像内容概览

| 组件 | 路径/说明 |
| ---- | -------- |
| Linux OLLVM Clang/LLD | `/opt/ollvm-linux/bin/clang{,++}`，基于 sr-tream/obfuscator 的 LLVM 18，支持 `-mllvm -fla/-bcf/-split/-sobf` 等所有 pass |
| llvm-mingw 交叉工具链 | `/opt/llvm-mingw/bin/x86_64-w64-mingw32-{clang,clang++}`，默认链接 UCRT，自带 `llvm-ar/llvm-rc` |
| Musl 头文件与静态库 | `/usr/include/x86_64-linux-musl`、`/usr/lib/x86_64-linux-musl`，便于生成静态二进制 |
| Rust + rustup | `/opt/cargo/bin`，预装目标：`x86_64-unknown-linux-gnu/musl`、`x86_64-pc-windows-gnu`，以及 cargo/llvm-tools-preview |
| 系统开发库 | `libjson-c-dev`、`libevent-dev`、`libssl-dev`、`zlib1g-dev`，可以直接链接 JSON-C/Libevent/OpenSSL |
| 常用构建工具 | `cmake`、`ninja`、`pkg-config`、`python3`、`git`、`yasm`、`gettext`、`autopoint`、`perl`、`m4`、`gperf` |
| 其他跨平台依赖 | `mingw-w64` 全套（含 `gcc-mingw-w64-{x86-64,i686}`、`binutils-*`、`mingw-w64-common`）、`musl-dev`、`musl-tools`、`patchelf`、`rsync` 等 |

> **提示**：GitHub Actions 的 `build.yml` 默认以最新 `zhangdafei1995/my_ollvm:latest` 作为 Base image，只在 `v*-full` 标签或手动传入 `rebuild_from_scratch=true` 时才全量编译，因而你在自定义镜像时也可以采用同样的继承策略。

## 1. 运行前准备

1. 安装 Docker 24+，并配置好可以访问 Docker Hub 的网络。
2. 如果需要推送自定义镜像，预先导出 `DOCKER_USER`、`DOCKER_TOKEN` 并登陆。
3. 准备好你的源码目录，后续会通过挂载方式放进容器。

## 2. 拉取与启动容器

```bash
docker pull zhangdafei1995/my_ollvm:latest

# 将当前源码目录挂到 /work，并以 root 进入
docker run --rm -it \
  -v "$(pwd)":/work \
  -w /work \
  zhangdafei1995/my_ollvm:latest \
  bash
```

容器内 PATH 已包含：

- `/opt/llvm-mingw/bin`：官方 llvm-mingw 工具链（含 `x86_64-w64-mingw32-clang` 等）
- `/opt/ollvm-linux/bin`：应用 sr-tream/obfuscator 补丁后的 Linux OLLVM Clang/LLD
- `/opt/cargo/bin`：Rust + `rustup`，预装 `x86_64-unknown-linux-gnu`、`x86_64-unknown-linux-musl`、`x86_64-pc-windows-gnu`

## 3. Linux 代码构建示例

### 3.1 glibc 目标

```bash
clang -mllvm -fla -mllvm -bcf -O2 \
  -o build/hello_glibc tests/hello_linux.c
```

- `-mllvm -fla/-bcf` 等参数可自由组合，控制 OLLVM 的混淆 pass。
- 产物链接到 glibc，适用于常规 Linux 发行版。

### 3.2 musl 静态目标

```bash
clang --target=x86_64-linux-musl \
  -static -s \
  -mllvm -sub \
  -o build/hello_musl tests/hello_linux.c
```

- 利用容器内置 musl 头文件和 `libc`，可以生成几乎无依赖的静态二进制，便于分发。

### 3.3 CMake/Makefile 项目

容器已经装好了 Ninja/CMake，只需指定工具链：

```bash
cmake -S . -B build/linux-glibc \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_CXX_COMPILER=clang++
cmake --build build/linux-glibc --parallel
```

对于 musl，可增加 `-DCMAKE_C_FLAGS=--target=x86_64-linux-musl`.

## 4. Windows (MinGW) 构建

```bash
x86_64-w64-mingw32-clang -O2 \
  -mllvm -splitobf \
  -o build/hello.exe tests/hello_win.c \
  -luser32
```

- `llvm-mingw` 已包含 `mingw-w64` 头与库，`DEFAULT_CRT` 设置为 `ucrt`。
- CMake 交叉工具链示例：

```bash
cmake -S . -B build/win64 \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_C_COMPILER=x86_64-w64-mingw32-clang \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-clang++ \
  -DCMAKE_RC_COMPILER=x86_64-w64-mingw32-windres
cmake --build build/win64 --parallel
```

## 5. Rust 工程

Rust 默认安装在 `/opt/cargo/bin`，并已添加以下 target：

- `x86_64-unknown-linux-gnu`
- `x86_64-unknown-linux-musl`
- `x86_64-pc-windows-gnu`

示例：

```bash
# Linux glibc
cargo build --release

# Linux musl（生成静态二进制）
RUSTFLAGS="-Clink-args=-static -Clink-args=-s -Zunstable-options -Ctarget-feature=+crt-static" \
cargo build --release --target x86_64-unknown-linux-musl

# Windows
cargo build --release --target x86_64-pc-windows-gnu
```

要让 Rust 目标也走 OLLVM pass，可以通过 wrapper 覆盖 `CC`/`CXX` 或启用 `-Zbuild-std`，例如：

```bash
export CC_x86_64_unknown_linux_musl=clang
export CFLAGS_x86_64_unknown_linux_musl="-mllvm -fla -mllvm -sobf"
cargo build --target x86_64-unknown-linux-musl
```

## 6. 可选：在容器内重建 OLLVM

镜像内已经安装 `build-host-ollvm.sh`：

```bash
build-host-ollvm.sh /opt/ollvm-linux-custom
```

- 默认会从 `LLVM_VERSION=llvmorg-18.1.8` 拉取源码，并应用 `/usr/local/share/obfuscator.patch`。
- 可通过环境变量控制：
  - `LLVM_VERSION=llvmorg-18.1.6` 指定版本
  - `CORES=32` 控制并行度
  - `PATCH_FILE=/path/to/custom.patch` 使用你自己的混淆补丁

## 7. 将镜像用作 CI/CD 基础镜像

`Dockerfile` 片段示例：

```Dockerfile
FROM zhangdafei1995/my_ollvm:latest
COPY . /work
WORKDIR /work
RUN cmake -S . -B build && cmake --build build --parallel
```

在 GitHub Actions 中使用：

```yaml
jobs:
  build:
    runs-on: ubuntu-latest
    container:
      image: zhangdafei1995/my_ollvm:latest
    steps:
      - uses: actions/checkout@v4
      - run: cmake -S . -B build && cmake --build build
```

## 8. 处理常见问题

| 问题 | 排查方式 |
| ---- | -------- |
| `Cannot find obfuscator patch` | 确认 `/usr/local/share/obfuscator.patch` 存在；若复制自定义补丁，记得 `chmod 644` |
| `unsupported option --target=` | 检查 `clang --version`，确保 PATH 没被宿主机覆盖 |
| Rust 交叉链接失败 | 追加 `-Clink-args=-L/opt/llvm-mingw/x86_64-w64-mingw32/lib` 或使用 `PKG_CONFIG_ALLOW_CROSS=1` |

## 9. 下一步

1. 持续在 GitHub Actions 上构建/测试，依赖 Buildx GHA 缓存缩短迭代。
2. 如需新的语言（Go、Zig 等），在 Dockerfile 中追加安装即可，保持在远程构建。
3. 有问题时使用 `pm gh run watch --repo ht4-team/my_ollvm <run_id>` 随时监控构建日志。

通过上述流程即可把 OLLVM 镜像作为统一的构建容器，针对 Linux、Windows 与 Rust 工程输出被混淆且高度可移植的可执行文件。祝编译顺利！
