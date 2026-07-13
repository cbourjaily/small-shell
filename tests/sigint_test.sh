#!/bin/bash

coproc SHELLPROC { ./smallsh; }

sleep 1

printf "sleep 10\n" >&"${SHELLPROC[1]}"

sleep 2

kill -INT "$SHELLPROC_PID"

printf "status\n" >&"${SHELLPROC[1]}"
printf "exit\n" >&"${SHELLPROC[1]}"

cat <&"${SHELLPROC[0]}"
