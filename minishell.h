#ifndef MINISHELL_H
#define MINISHELL_H

#include "parser.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>

#define INPUT_PRINT "\033[93mmsh > \033[0;0m"
#define ERR_MEMORY "Error allocating memory\n"

typedef struct s_exec
{
	pid_t	*pid;
	int	**pipe;
	int	in_fd;
	int	out_fd;
	int	err_fd;
	char	**paths;
	tline	*line;
}	t_exec;

//execution.c
void	execute(tline *line);
void	close_all(t_exec *exec);

//child_process.c
void	child_process(t_exec exec, int num);

//error.c
void	exit_msg(char *msg, int val);
void	error_msg(char *msg, int val);

#endif
