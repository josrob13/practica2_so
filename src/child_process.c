#include "../minishell.h"

static void	set_redirections(t_exec *exec, int num);
static char	*get_command(tcommand file);
static int	is_here(char *file);
static void	check_directories(char *file);

void	child_process(t_exec exec, int num, t_list **bg)
{
	char	*command;

	set_redirections(&exec, num);
	close_all(&exec);
	command = get_command(exec.line -> commands[num]);
	if (is_builtin(exec.line -> commands[num].argv[0])) {
		do_builtin(exec.line -> commands[num], bg);
		exit(0);
	}
	if (!command) {
		fputs(exec.line -> commands[num].argv[0], stderr);
		exit_msg(": command not found\n", 127);
	}
	if (execv(command, exec.line -> commands[num].argv))
		error_msg("Error in execution", 1);
}

static char	*get_command(tcommand file)
{
	char	*command;

	check_directories(file.argv[0]);
	if (!strcmp("/", file.argv[0])) {
		if (access(file.argv[0], F_OK | X_OK) == 0)
			return (file.argv[0]);
		error_msg(file.argv[0], 126);
	}
	if (!strcmp("./", file.argv[0]) || !is_here(file.argv[0])) {
		if (access(file.argv[0], F_OK | X_OK) == 0)
			return (file.argv[0]);
		error_msg(file.argv[0], 126);
	}
	if (file.filename)
		return (file.filename);
	return (NULL);
}

static int	is_here(char *file)
{
	int	i;

	i = -1;
	while (file[++i])
		if (file[i] == '/')
			return (0);
	return (1);
}

static void	check_directories(char *file)
{
	DIR	*dir;

	if (!strcmp(".", file))
		exit_msg(".: not enough arguments\n", 2);
	if (!strcmp("..", file))
		exit_msg("..: command not found\n", 2);
	if ((dir = opendir(file))) {
		closedir(dir);
		fputs(file, stderr);
		exit_msg(": Is a directory\n", 2);
	}
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
