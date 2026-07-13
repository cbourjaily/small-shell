#!/bin/bash

set -e

echo "Compiling..."
gcc -Wall -Wextra -o ../smallsh ../smallsh.c

mkdir -p output

pass=0
total=2

echo "Running basic..."
timeout 10 ../smallsh < basic.in > output/basic.out 2>&1

if diff -u basic.expected output/basic.out >/dev/null; then
    echo "PASS basic"
    pass=$((pass + 1))
else
    echo "FAIL basic"
    diff -u basic.expected output/basic.out
fi

echo "Running background..."
timeout 10 ../smallsh < background.in > output/background.out 2>&1

sed -E \
    -e 's/background pid is [0-9]+/background pid is <PID>/' \
    -e 's/background pid [0-9]+ is done/background pid <PID> is done/' \
    output/background.out > output/background.norm

if diff -u background.expected output/background.norm >/dev/null; then
    echo "PASS background"
    pass=$((pass + 1))
else
    echo "FAIL background"
    diff -u background.expected output/background.norm
fi

echo
echo "$pass/$total tests passed."
