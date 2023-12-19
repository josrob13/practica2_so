#include "../minishell.h"
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>

static void	wait_all(t_exec *exec);
static void	bgadd(t_list **bg, pid_t *pid, char *buf);
static int	is_id_free(t_list *bg, int i);
static void	initialize_exec(t_exec *exec, tline *line);
static void	free_exec(t_exec *exec, tline *line);
static void	do_fg(tcommand command, t_list **bg);
static void	do_jobs(t_list *bg);
static void	wait_fg(t_list *bg, int id);
static void	print_fg(t_list *bg, int id);
static void	bgdelete(t_list **bg, int id);
static void	do_cd(char **argv);
static void	do_exit();

void	execute(tline *line, t_list **bg, char *buf)
{
	t_exec	exec;
	int	i;

	initialize_exec(&exec, line);
	if (line -> ncommands == 1 && is_builtin(line -> commands[0].argv[0])) {
		do_builtin(line -> commands[0], bg);
	}else {
		i = -1;
		while (++i < line -> ncommands) {
			g_sig = 0;
			exec.pid[i] = fork();
			if (exec.pid[i] == 0)
				child_process(exec, i, bg);
		}
		close_all(&exec);
		if (line -> background == 1)
			bgadd(bg, exec.pid, buf);
		else
			wait_all(&exec);
	}
	free_exec(&exec, line);
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

static void	bgadd(t_list **bg, pid_t *pid, char *buf)
{
	t_list	*node;
	int	id;
	int	i;

	node = malloc(sizeof(t_list));
	if (!node)
		return ;
	id = 1;
	while (!is_id_free(*bg, id))
		id++;
	node -> id = id;
	i = 0;
	while (pid[i] != -1)
		i++;
	node -> pids = malloc(i * sizeof(int));
	i = -1;
	while (pid[++i] != -1)
		node -> pids[i] = pid[i];
	node -> pids[i] = -1;
	node -> line = strdup(buf);
	node -> next = *bg;
	*bg = node;
	fputs("[", stdout);
	fputc(node -> id + '0', stdout);
	fputs("]\t ", stdout);
	fputs(node -> line, stdout);
}

static int	is_id_free(t_list *bg, int i)
{
	while (bg) {
		if (bg -> id == i)
			return (0);
		bg = bg -> next;
	}
	return (1);
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
	exec -> pid[line -> ncommands] = -1;
	exec -> line = line;
	exec -> in_fd = -1;
	exec -> out_fd = -1;
	exec -> err_fd = -1;
}

static void	free_exec(t_exec *exec, tline *line)
{
	int	i;

	i = -1;
	while (++i < line -> ncommands - 1)
		free(exec -> pipe[i]);
	free(exec -> pipe);
	free(exec -> pid);
}

int		is_builtin(char *name)
{
	if (!strcmp("cd", name) || !strcmp("exit", name) || !strcmp("jobs", name) || !strcmp("fg", name))
		return 1;
	return 0;
}

void	do_builtin(tcommand command, t_list **bg)
{
	if (!strcmp("cd", command.argv[0]))
		do_cd(command.argv);
	else if (!strcmp("exit", command.argv[0]))
		do_exit();
	else if (!strcmp("jobs", command.argv[0]))
		do_jobs(*bg);
	else if (!strcmp("fg", command.argv[0]))
		do_fg(command, bg);
}

static void	do_fg(tcommand command, t_list **bg)
{
	int	id;

	if (command.argv[1]) {
		if (atoi(command.argv[1]) < 1) {
			fputs("fg: job not found: ", stderr);
			fputs(command.argv[1], stderr);
			fputs("\n", stderr);
			return ;
		}
		id = atoi(command.argv[1]);
	} else
		id = -1;
	print_fg(*bg, id);
	wait_fg(*bg, id);
	bgdelete(bg, id);
}

static void	wait_fg(t_list *bg, int id)
{
	t_list	*aux, *aux2;
	int	i;

	aux = bg;
	i = -1;
	if (!aux)
		return ;
	else if (id == -1 || aux -> id == id) {
		while (bg -> pids[++i])
			waitpid(bg -> pids[i], NULL, 0);
		bg = aux -> next;
		return ;
	} else do {
		if (aux -> id == id) {
			while (bg -> pids[++i])
				waitpid(bg -> pids[i], NULL, 0);
			aux2 -> next = aux -> next;
			return ;
		}
		aux2 = aux;
		aux = aux -> next;
	} while (aux);
}

static void	print_fg(t_list *bg, int id)
{
	t_list	*aux, *aux2;

	aux = bg;
	if (!aux) {
		if (id == -1)
			fputs("fg: no current jobs\n", stderr);
		else {
			fputs("fg: job not found: ", stderr);
			fputc(id + '0', stderr);
			fputs("\n", stderr);
		}
		return ;
	} else if (id == -1 || aux -> id == id) {
		fputs("[", stdout);
		fputc(aux -> id + '0', stdout);
		fputs("]+ Running\t ", stdout);
		fputs(aux -> line, stdout);
		return ;
	} else do {
		if (aux -> id == id) {
			fputs("[", stdout);
			fputc(aux -> id + '0', stdout);
			fputs("]+ Running\t ", stdout);
			fputs(aux -> line, stdout);
			return ;
		}
		aux2 = aux;
		aux = aux -> next;
	} while (aux);
	fputs("fg: job not found: ", stderr);
	fputc(id + '0', stderr);
	fputs("\n", stderr);
}

static void	bgdelete(t_list **bg, int id)
{
	t_list	*aux, *aux2;

	aux = *bg;
	if (!aux)
		return ;
	else if (id == -1 || aux -> id == id) {
		free(aux -> pids);
		free(aux -> line);
		*bg = aux -> next;
		free(aux);
		return ;
	} else do {
		if (aux -> id == id) {
			free(aux -> pids);
			free(aux -> line);
			aux2 -> next = aux -> next;
			free(aux);
			return ;
		}
		aux2 = aux;
		aux = aux -> next;
	} while (aux);
}

static void	do_jobs(t_list *bg)
{
	int	count = 0;

	while (bg) {
		fputs("[", stdout);
		fputc(bg -> id + '0', stdout);
		fputs("]", stdout);
		if (count == 0)
			fputs("+", stdout);
		else if (count == 1)
			fputs("-", stdout);
		else
			fputs(" ", stdout);
		fputs(" Running\t ", stdout);
		fputs(bg -> line, stdout);
		bg = bg -> next;
		count++;
	}
}

static void do_exit()
{
	exit(0);
}

static void do_cd(char **argv)
{
	char	cwd[PATH_MAX];
	char	*chdirectory;
	DIR	*dir;

	if (argv[1] && argv[2]) {
		fputs("cd: too many arguments\n", stderr);
		return ;
	}
	if (!argv[1]) {
		chdirectory = getenv("HOME");
		if (!chdirectory) {
			fputs("cd: HOME not set\n", stderr);
			return ;
		}
	} else
		chdirectory = argv[1];
	dir = opendir(chdirectory);
	if (!dir) {
		fputs("cd: ", stderr);
		perror(chdirectory);
		return ;
	}
	closedir(dir);
	if (chdir(chdirectory) == -1) {
		fputs("cd: ", stderr);
		perror(chdirectory);
		return ;
	}
}
