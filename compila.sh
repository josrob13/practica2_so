#!/bin/bash

gcc -c myshell.c -o myshell.o
gcc myshell.o -o msh -I. -L. -lparser -no-pie

rm -rf *.o
echo -e "\e[1;32mCompilation finished !\e[0m"
