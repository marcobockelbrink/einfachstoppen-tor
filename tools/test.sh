#!/bin/sh
# SPDX-License-Identifier: MIT
# Kernlogik auf dem Rechner testen. Braucht nur einen C++-Compiler.
set -e
cd "$(dirname "$0")/.."
OUT="${TMPDIR:-/tmp}/tor_test_kern"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -o "$OUT" tests/test_kern.cpp
"$OUT"
