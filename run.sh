#!/bin/bash

echo "================================================="
echo "APPLICATION LOGS"
echo "================================================="
strace -o strace-output.log ./build/malloc-inspector
echo ""

echo ""
echo "================================================="
echo "STRACE LOGS"
echo "================================================="
echo ""
cat strace-output.log
