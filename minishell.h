#ifndef MINISHELL_H
#define MINISHELL_H

#include "parser.h"
#include <stdio.h>
#include <unistd.h>

# define INPUT_PRINT "\033[93mmsh > \033[0;0m"

//execution.c
void	execute(tline *line);

#endif
