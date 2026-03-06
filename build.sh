#!/usr/bin/env bash
set -euo pipefail

BUILD_DIR="build"

echo "==> Configuring..."
cmake -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_OSX_ARCHITECTURES="arm64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="12.0"

echo "==> Building..."
cmake --build "$BUILD_DIR" --config Release --parallel

echo ""
echo "==> Build complete."
echo "    VST3: $BUILD_DIR/HyperCrush_artefacts/Release/VST3/HyperCrush.vst3"
echo "    AU:   $BUILD_DIR/HyperCrush_artefacts/Release/AU/HyperCrush.component"
