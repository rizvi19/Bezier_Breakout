#!/usr/bin/env bash
set -e

cd "$(dirname "$0")"

echo "Building Bezier Breakout..."
g++ -std=c++17 main.cpp -o bezier_breakout -lGL -lGLU -lglut

echo "Starting game..."
./bezier_breakout
