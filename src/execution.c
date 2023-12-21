#include "../minishell.h"
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>

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
static void do_umask(tcommand command);
static int length(t_list *bg);
static int is_in(int e, t_list *l);
static t_list *add(t_list *l, int e);
static void print_mins(t_list *l);

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
	if (!strcmp("cd", name) || !strcmp("exit", name) || !strcmp("jobs", name) || !strcmp("fg", name) || !strcmp("umask", name))
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

static void	do_jobs2(t_list *bg)
{
	/*
	int i, j, printed;
	t_list *aux;
	int	count = 0;
	
	while (aux) {
		fputs("[", stdout);
		fputc(aux -> id + '0', stdout);
		fputs("]", stdout);
		if (!aux -> next)
			fputs("+", stdout);
		else if (!aux -> next -> next)
			fputs("-", stdout);
		else
			fputs(" ", stdout);
		fputs(" Running\t ", stdout);
		fputs(aux -> line, stdout);
		aux = aux -> next;
		count++;
	}

	j = 0;
	while (j < length(bg))
	{
		i = 0;
		printed = 0;
		aux = bg;
		while (i < length(bg) && !printed)
		{
			if (i == (length(bg) - j)) {
				fputs("[", stdout);
				fputc(aux -> id + '0', stdout);
				fputs("]", stdout);
				if (i == 0)
					fputs("+", stdout);
				else if (i == 1)
					fputs("-", stdout);
				else
					fputs(" ", stdout);
				fputs(" Running\t ", stdout);
				fputs(aux -> line, stdout);
				j++;
				printed = 1;
			}
			i++;
			aux = aux -> next;
		}
	}*/

	//print_mins(bg);
	
}

static void do_jobs(t_list *l)
{
	int i, j, pos, printed;
	t_list *aux;

	i = 0;
	j = 1;
	if (l) {
		while (i < length(l))
		{
			printf("HEMOS LLEGADO A LA ITERACION: %i", i);
			aux = l;
			printed = 0;
			pos = 0;
			while (aux && !printed)
			{
				if (aux -> id == j) {
					fputs("[", stdout);
					fputc(aux -> id + '0', stdout);
					fputs("]", stdout);
					if (pos == 0)
						fputs("+", stdout);
					else if (pos == 1)
						fputs("-", stdout);
					else
						fputs(" ", stdout);
					fputs(" Running\t ", stdout);
					fputs(aux -> line, stdout);
					printed = 1;
					j++;
				} else{
					if (aux -> next) {
						if (aux -> next -> id >= j) {
							aux = aux -> next;
							pos++;
						} else
							j++;
					}
					else
						j++;
				}
			}
			i++;
		}
	}
	
	
}

static int is_in(int e, t_list *l)
{
	t_list *aux;

	aux = l;
	while (aux)
	{
		if (aux -> id == e) {
			//printf("El elemento %i ya ha sido printeado", e);
			return 1;
		}
		aux = aux -> next;
	}

	return 0;
}

static t_list *add(t_list *l, int e)
{
	t_list *node;

	node = malloc(sizeof(t_list));
	if (!node)
		return NULL;
	node -> id = e;
	node -> next = l;
	l = node;

	return l;
}

static int length(t_list *bg)
{
	int total;
	t_list *aux;
	
	aux = bg;
	total = 0;
	while (aux)
	{
		total++;
		aux = aux -> next;
	}
	
	return total;
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

static void do_umask(tcommand command) {
	char mask_str[10];

	if (command.argc < 2) {
        mode_t old_mask = umask(0); // Obtener la máscara actual, devuelve la antigua y como es 0 no la cambia
        umask(old_mask);
        snprintf(mask_str, sizeof(mask_str), "%o", old_mask); // Transformar a string
        fputs(mask_str, stdout); // Imprimir la máscara actual
        fputs("\n", stdout);
	}
	else {
    	mode_t mask = strtol(command.argv[1], NULL, 8); // Convierte el string octal a modo_t, y se usa para cambiar la máscara
    	mode_t prev_mask = umask(mask);
	}
}
