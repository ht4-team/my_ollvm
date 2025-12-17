#!/bin/bash
set -euo pipefail

PREFIX="${1:-/opt/ollvm-linux}"
LLVM_VERSION="${LLVM_VERSION:-llvmorg-18.1.8}"
PATCH_FILE="${PATCH_FILE:-$(pwd)/obfuscator.patch}"
WORKDIR="$(mktemp -d /build/ollvm-linux-XXXXXX)"
CORES="${CORES:-}"
LLVM_REPO="https://github.com/llvm/llvm-project.git"

cleanup() {
  rm -rf "$WORKDIR"
}
trap cleanup EXIT

if [ -z "$CORES" ]; then
  if command -v nproc >/dev/null 2>&1; then
    CORES="$(nproc)"
  else
    CORES=4
  fi
fi

SRC_DIR="$WORKDIR/llvm-project"
echo "[ollvm] fetching $LLVM_VERSION from $LLVM_REPO"
git clone --depth 1 --branch "$LLVM_VERSION" "$LLVM_REPO" "$SRC_DIR"

if [ ! -f "$PATCH_FILE" ]; then
  echo "Cannot find obfuscator patch at $PATCH_FILE" >&2
  exit 1
fi

cp "$PATCH_FILE" "$WORKDIR/obfuscator.patch"
cd "$SRC_DIR"
git apply "$WORKDIR/obfuscator.patch"

BUILD_DIR="$SRC_DIR/build-linux"
cmake -S llvm -B "$BUILD_DIR" \
  -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_PREFIX="$PREFIX" \
  -DLLVM_ENABLE_PROJECTS="clang;lld;clang-tools-extra" \
  -DLLVM_ENABLE_RUNTIMES="compiler-rt;libunwind;libcxxabi;libcxx" \
  -DLLVM_TARGETS_TO_BUILD="X86;AArch64;ARM;RISCV" \
  -DLLVM_ENABLE_ASSERTIONS=OFF \
  -DLLVM_LINK_LLVM_DYLIB=ON \
  -DLLVM_ENABLE_RTTI=ON \
  -DLLVM_ENABLE_TERMINFO=OFF \
  -DLLVM_ENABLE_LIBXML2=ON \
  -DLLVM_ENABLE_EH=ON

cmake --build "$BUILD_DIR" --parallel "$CORES"
cmake --install "$BUILD_DIR" --strip

echo "[ollvm] linux toolchain installed to $PREFIX"
