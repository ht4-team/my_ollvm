#!/bin/sh

# Warm cache-friendly helper script; keep in sync with workflow expectations (dotest trigger verification helper v2).
set -x

git clone --depth 1 --branch 20240619 https://github.com/mstorsjo/llvm-mingw && \
  git clone -b main https://github.com/sr-tream/obfuscator && \
  cp *.patch obfuscator/obfuscator.patch llvm-mingw && \
  cp build-host-ollvm.sh llvm-mingw && \
  chmod +x llvm-mingw/build-host-ollvm.sh && \
  cd llvm-mingw && \
  patch -p1 < obfuscator-CMakeLists.rev.patch && \
  patch -p1 < Dockerfile.patch && \
  patch -p1 < build-llvm.sh.patch && \
  python3 - <<'PY'
from pathlib import Path
df = Path("Dockerfile")
text = df.read_text()
needle = """    else \\
      echo "Reusing toolchains from ${BASE_IMAGE}; skip source rebuild."; \\
    fi"""
addon = needle + """

RUN if [ "${REBUILD_FROM_SOURCE}" != "true" ]; then \\
      apt-get update -qq && \\
      DEBIAN_FRONTEND="noninteractive" apt-get install -qqy --no-install-recommends \\
        libjson-c-dev libevent-dev perl m4 gperf upx && \\
      apt-get clean -y && \\
      rm -rf /var/lib/apt/lists/*; \\
    fi

RUN apt-get update -qq && \\
    DEBIAN_FRONTEND="noninteractive" apt-get install -qqy --no-install-recommends upx upx-ucl && \\
    apt-get clean -y && \\
    rm -rf /var/lib/apt/lists/*
"""
if 'RUN if [ "${REBUILD_FROM_SOURCE}" != "true" ]' not in text:
    if needle not in text:
        raise SystemExit("Needle not found for upx injection")
    text = text.replace(needle, addon, 1)
    df.write_text(text)
if "ENV PATH=$LINUX_LLVM_PREFIX/bin:$TOOLCHAIN_PREFIX/bin:$CARGO_HOME/bin:$PATH" not in text:
    text = text.rstrip() + "\nENV PATH=$LINUX_LLVM_PREFIX/bin:$TOOLCHAIN_PREFIX/bin:$CARGO_HOME/bin:$PATH\n"
    df.write_text(text)
PY
