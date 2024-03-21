#!/bin/bash

g++ server.cpp -o server -Wall -lcurl
gcc client.c -o client -Wall
