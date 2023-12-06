#include "../minishell.h"

void	exit_msg(char *msg, int val)
{
	fputs(msg, stderr);
	exit (val);
}

void	error_msg(char *msg, int val)
{
	perror(msg);
	exit (val);
}
