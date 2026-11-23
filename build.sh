#!/usr/bin/env bash
set -e

g++ -std=c++11 -Wall -Wextra -pedantic daemon.cpp -o daemon