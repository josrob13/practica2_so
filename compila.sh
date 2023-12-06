#!/bin/bash

gcc -c src/myshell.c -o myshell.o
gcc -c src/execution.c -o execution.o
gcc -c src/child_process.c -o child_process.o
gcc -c src/error.c -o error.o
gcc myshell.o execution.o child_process.o error.o -o msh -I. -L parse/ -lparser -no-pie

rm -rf *.o
echo -e "\e[1;32mCompilation finished !\e[0m"
