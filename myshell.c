#include "parser.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#define INPUT_PRINT "\033[93mmsh > \033[0;0m"
#define ERR_MEMORY "Error allocating memory\n"

typedef struct s_list
{
	int		id;
	int		*pids;
	char		*line;
	struct s_list	*next;
}	t_list;

typedef struct s_exec
{
	pid_t	*pid;
	int	**pipe;
	int	in_fd;
	int	out_fd;
	int	err_fd;
	tline	*line;
}	t_exec;

static void	sig_handler_input(int signal);
static void	execute(tline *line, t_list **bg, char *buf);
static void	close_all(t_exec *exec);
static int	is_builtin(char *name);
static void	do_builtin(tcommand command, t_list **bg);
static void	bgdelete(t_list **bg, int id);
static void	kill_childs(t_list **bg);
static void	child_process(t_exec exec, int num, t_list **bg);
static void	exit_msg(char *msg, int val);
static void	error_msg(char *msg, int val);
static void	wait_all(t_exec *exec);
static void	bgadd(t_list **bg, pid_t *pid, char *buf);
static int	is_id_free(t_list *bg, int i);
static void	initialize_exec(t_exec *exec, tline *line);
static void	free_exec(t_exec *exec, tline *line);
static void	do_fg(tcommand command, t_list **bg);
static void	do_jobs(t_list **bg);
static void	check_jobs(t_list **bg);
static void	wait_fg(t_list *bg, int id);
static void	print_fg(t_list *bg, int id);
static void	do_cd(char **argv);
static void	do_exit(t_list **bg);
static void	do_umask(tcommand command);
static int	length(t_list *bg);
static void	set_redirections(t_exec *exec, int num);
static char	*get_command(tcommand file);
static int	is_here(char *file);
static void	check_directories(char *file);

int	g_sig = 1;

int	main(void)
{
	char	buf[1024];
	tline	*line;
	t_list	*bg = NULL;

	signal(SIGINT, sig_handler_input);
	fputs(INPUT_PRINT, stdout);
	while (fgets(buf, 1024, stdin)) {
		line = tokenize(buf);
		if (line -> ncommands > 0) {
			execute(line, &bg, buf);
		}
		fputs(INPUT_PRINT, stdout);
		g_sig = 1;
	}
	if (bg)
		kill_childs(&bg);
	return (0);
}

static void	kill_childs(t_list **bg)
{
	int	i;

	while (*bg) {
		i = -1;
		while ((*bg) -> pids[++i] != -1)
			kill((*bg) -> pids[i], SIGTERM);
		bgdelete(bg, -1);
	}
}

static void	sig_handler_input(int signal)
{
	(void) signal; //Eliminar warning
	if (g_sig == 1)
		write(1, "\n\033[93mmsh > \033[0;0m", 19);
}

static void	execute(tline *line, t_list **bg, char *buf)
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
			if (exec.pid[i] == 0) {
				if (line -> background == 1)
					signal(SIGINT, SIG_IGN);
				child_process(exec, i, bg);
			}
		}
		close_all(&exec);
		if (line -> background == 1)
			bgadd(bg, exec.pid, buf);
		else
			wait_all(&exec);
	}
	free_exec(&exec, line);
}

static void	close_all(t_exec *exec)
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
	fputs("] ", stdout);
	i = -1;
	while (pid[++i] != -1)
		fprintf(stdout, "%d ", pid[i]);
	fputs("\n", stdout);
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

static int	is_builtin(char *name)
{
	if (!strcmp("cd", name) || !strcmp("exit", name) || !strcmp("jobs", name) || !strcmp("fg", name) || !strcmp("umask", name))
		return 1;
	return 0;
}

static void	do_builtin(tcommand command, t_list **bg)
{
	if (!strcmp("cd", command.argv[0]))
		do_cd(command.argv);
	else if (!strcmp("exit", command.argv[0]))
		do_exit(bg);
	else if (!strcmp("jobs", command.argv[0]))
		do_jobs(bg);
	else if (!strcmp("fg", command.argv[0]))
		do_fg(command, bg);
	else if (!strcmp("umask", command.argv[0]))
		do_umask(command);
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
		while (bg -> pids[++i] != -1) {
			fprintf(stdout, "Espero pid: %d\n", bg -> pids[i]);
			waitpid(bg -> pids[i], NULL, 0);
		}
		bg = aux -> next;
		return ;
	} else do {
		if (aux -> id == id) {
			while (bg -> pids[++i] != -1) {
				fprintf(stdout, "Espero pid: %d\n", bg -> pids[i]);
				waitpid(bg -> pids[i], NULL, 0);
			}
			aux2 -> next = aux -> next;
			return ;
		}
		aux2 = aux;
		aux = aux -> next;
	} while (aux);
}

static void	print_fg(t_list *bg, int id)
{
	t_list	*aux;

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

static void do_jobs(t_list **bg)
{
	int	i, j, len;
	t_list	*aux;

	check_jobs(bg);
	if (!(*bg))
		return ;
	len = length(*bg);
	i = 0;
	j = 1;
	while (i < len) {
		aux = *bg;
		while (aux) {
			if (aux -> id == j) {
				fputs("[", stdout);
				fputc(aux -> id + '0', stdout);
				fputs("]", stdout);
				if ((*bg) -> id == aux -> id)
					fputs("+", stdout);
				else if ((*bg) -> next && (*bg) -> next -> id == aux -> id)
					fputs("-", stdout);
				else
					fputs(" ", stdout);
				fputs(" Running\t ", stdout);
				fputs(aux -> line, stdout);
				i++;
			}
			aux = aux -> next;
		}
		j++;
	}
}

static int length(t_list *bg)
{
	int total;
	
	total = 0;
	while (bg) {
		total++;
		bg = bg -> next;
	}
	return (total);
}

static void	check_jobs(t_list **bg)
{
	int	i, finished;
	t_list	*aux, *aux2;

	aux = *bg;
	while (aux) {
		i = -1;
		finished = 1;
		while (aux -> pids[++i] != -1)
			if (waitpid(aux -> pids[i], NULL, WNOHANG) == 0)
				finished = 0;
		if (finished) {
			aux2 = aux;
			aux = aux -> next;
			bgdelete(bg, aux2 -> id);
		}
		else
			aux = aux -> next;
	}
}

static void do_exit(t_list **bg)
{
	kill_childs(bg);
	exit(0);
}

static void do_cd(char **argv)
{
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

static void do_umask(tcommand command)
{
	char  mask_str[10];
	mode_t old_mask, mask;

	if (command.argc < 2) {
		old_mask = umask(0); // Obtener la máscara actual, devuelve la antigua y como es 0 no la cambia
		umask(old_mask);
		snprintf(mask_str, sizeof(mask_str), "%o", old_mask); // Transformar a string
		fputs(mask_str, stdout); // Imprimir la máscara actual
		fputs("\n", stdout);
	} else {
		mask = strtol(command.argv[1], NULL, 8); // Convierte el string octal a modo_t, y se usa para cambiar la máscara
		umask(mask);
	}
}

static void	child_process(t_exec exec, int num, t_list **bg)
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

static void	exit_msg(char *msg, int val)
{
	fputs(msg, stderr);
	exit (val);
}

static void	error_msg(char *msg, int val)
{
	perror(msg);
	exit (val);
}
