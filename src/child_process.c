#include "../minishell.h"

static void	set_redirections(t_exec *exec, int num);

void	child_process(t_exec exec, int num)
{
	set_redirections(&exec, num);
	close_all(&exec);
	exit (0);
}

static void	set_redirections(t_exec *exec, int num)
{
	if (exec -> line -> redirect_input && num == 0) {
		exec -> in_fd = open(exec -> line -> redirect_input, O_RDONLY);
		if (exec -> in_fd < 0)
			error_msg(exec -> line -> redirect_input, 1);
		dup2(exec -> in_fd, 0);
	}
	if (exec -> line -> redirect_output && num == exec -> line -> ncommands - 1) {
		exec -> out_fd = open(exec -> line -> redirect_output, O_CREAT | O_WRONLY | O_TRUNC, 0666);
		if (exec -> out_fd < 0)
			error_msg(exec -> line -> redirect_input, 1);
		dup2(exec -> out_fd, 1);
	}
	if (exec -> line -> redirect_error && num == exec -> line -> ncommands - 1) {
		exec -> err_fd = open(exec -> line -> redirect_error, O_CREAT | O_WRONLY | O_TRUNC, 0666);
		if (exec -> err_fd < 0)
			error_msg(exec -> line -> redirect_input, 1);
		dup2(exec -> err_fd, 2);
	}
	if (num != 0)
		if (exec -> in_fd == -1)
			dup2(exec -> pipe[num - 1][0], 0);
	if (num != exec -> line -> ncommands - 1)
		if (exec -> out_fd == -1)
			dup2(exec -> pipe[num][1], 1);
}
