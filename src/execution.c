#include "../minishell.h"

static void	wait_all(t_exec *exec);
static void	initialize_exec(t_exec *exec, tline *line);
static void do_cd(char *dir);

void	execute(tline *line)
{
	t_exec	exec;
	int	i;

	initialize_exec(&exec, line);
	// if ncommads == 1 && is buitlin
	//     builtin
	// else
	if (line -> ncommands == 1 && !strcmp("cd", line -> commands[0].argv[0])) {
		do_cd(line -> commands[0].argv[1]);
	}else {
		i = -1;
		while (++i < line -> ncommands) {
			exec.pid[i] = fork();
			if (exec.pid[i] == 0)
				child_process(exec, i);
		}
		close_all(&exec);
		wait_all(&exec);
	}
}

void	close_all(t_exec *exec)
{
	int	i;

	i = -1;
	while (++i < (exec -> line -> ncommands - 1)) {
		close(exec -> pipe[i][0]);
		close(exec -> pipe[i][1]);
	}
}

static void	wait_all(t_exec *exec)
{
	int	i;

	i = -1;
	while (++i < (exec -> line -> ncommands))
		waitpid(exec -> pid[i], NULL, 0);
}

static void	initialize_exec(t_exec *exec, tline *line)
{
	int	i;

	exec -> pipe = malloc((line -> ncommands - 1) * sizeof(int *));
	if (!(exec -> pipe))
		exit_msg(ERR_MEMORY, 1);
	i = -1;
	while (++i < line -> ncommands - 1) {
		exec -> pipe[i] = malloc(2 * sizeof(int));
		if (!(exec -> pipe[i]))
			exit_msg(ERR_MEMORY, 1);
		if (pipe(exec -> pipe[i]) < 0)
			error_msg("Error creating pipes", 1);
	}
	exec -> pid = malloc((line -> ncommands) * sizeof(pid_t));
	if (!(exec -> pid))
		exit_msg(ERR_MEMORY, 1);
	exec -> line = line;
	exec -> in_fd = -1;
	exec -> out_fd = -1;
	exec -> err_fd = -1;
}

static void do_cd(char *dir)
{
	// return rarete, si pudiera haber otra opción mucho mejor
	if (dir == NULL) {
		printf("Invalid address.\n");
		return;
	}else if (chdir(dir) != 0)
		error_msg("Error in execution", 1);

	printf("The direction has been succesfully changed.\n");
}