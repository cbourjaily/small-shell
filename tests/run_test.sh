#!/bin/bash

set -e

INPUT="$1"

if [ -z "$INPUT" ]; then
    echo "Usage: ./run_test.sh test.in"
    exit 1
fi

mkdir -p output

BASE=$(basename "$INPUT" .in)

echo "Running $BASE..."

timeout 15 ../smallsh < "$INPUT" > "output/$BASE.out" 2>&1

echo "Output written to output/$BASE.out"
