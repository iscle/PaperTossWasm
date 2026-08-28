#!/usr/bin/env bash
# Builds the WebAssembly bundle into dist/.
set -euo pipefail

cd "$(dirname "$0")"
mkdir -p dist

em++ src/*.cpp \
  -o dist/index.html \
  --shell-file shell.html \
  -std=c++17 \
  -O3 \
  -flto \
  -sUSE_SDL=2 \
  -sUSE_SDL_MIXER=2 \
  -sSDL2_MIXER_FORMATS='["ogg"]' \
  -sMIN_WEBGL_VERSION=1 \
  -sMAX_WEBGL_VERSION=1 \
  -sALLOW_MEMORY_GROWTH=1 \
  -sINITIAL_MEMORY=64MB \
  -sSTACK_SIZE=1MB \
  -sEXIT_RUNTIME=0 \
  -sENVIRONMENT=web \
  --preload-file assets@/assets \
  --preload-file music@/music

cp splash.png icon.png dist/
echo "Built dist/index.html"
