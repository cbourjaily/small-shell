#!/bin/bash

set -e

echo "Compiling..."
gcc -Wall -Wextra -o ../smallsh ../smallsh.c

echo "Running tests..."

mkdir -p output

./run_test.sh basic.in output/basic.out
./run_test.sh background.in output/background.out
# ./sigint_test.sh output/sigint.out

echo "Done."
