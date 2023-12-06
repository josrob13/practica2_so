#!/bin/bash

gcc -c src/myshell.c -o myshell.o
gcc -c src/execution.c -o execution.o
gcc myshell.o execution.o -o msh -I. -L parse/ -lparser -no-pie

rm -rf *.o
echo -e "\e[1;32mCompilation finished !\e[0m"
