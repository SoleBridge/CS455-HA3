#!/bin/env bash
gcc -o ./client/client.bin ./src/client.c ./src/common.c -pthread
gcc -o ./server/server.bin ./src/server.c ./src/common.c -pthread
