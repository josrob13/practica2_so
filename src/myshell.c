#include "../minishell.h"

static void	sig_handler_input(int signal);
void	bgadd(t_list **bg, int id);
void	bgdelete(t_list **bg, int index);
void	print_lst(t_list **bg);

int	g_sig = 1;

int	main(void)
{
	char	buf[1024];
	tline	*line;
	t_list	*bg = NULL;

	//#########################
	int i = 0;
	while (++i < 4) {
		bgadd(&bg, i);
	}
	print_lst(&bg);
	printf("-----------\nElimino\n-----------\n");
	bgdelete(&bg, 2);
	print_lst(&bg);
	//#########################

	signal(SIGINT, sig_handler_input);
	fputs(INPUT_PRINT, stdout);
	while (fgets(buf, 1024, stdin)) {
		line = tokenize(buf);
		if (line -> ncommands > 0) {
			//if (es un comando en background)
			//    lo añado a la lista bg
			//    y las demas movidas necesarias con los waitpid
			execute(line);
		}
		fputs(INPUT_PRINT, stdout);
		g_sig = 1;
	}
	return (0);
}

static void	sig_handler_input(int signal)
{
	if (g_sig == 1)
		write(1, "\n\033[93mmsh > \033[0;0m", 19);
}

void	bgadd(t_list **bg, int id)
{
	t_list	*node;

	node = malloc(sizeof(t_list));
	if (!node)
		return ;
	node -> id = id;
	node -> pids = malloc(3 * sizeof(int));
	node -> pids[0] = 3099;
	node -> pids[1] = 3084;
	node -> pids[2] = -1;
	node -> line = strdup("ping google.com | cat -e");
	node -> next = *bg;
	*bg = node;
}

void	bgdelete(t_list **bg, int index)
{
	t_list	*aux, *aux2;

	//se supone que antes se ha comprobado que el index no es mayor que el tamaño de la lista
	aux = *bg;
	if (index == 1) {
		free(aux -> pids);
		free(aux -> line);
		*bg = aux -> next;
		free(aux);
	} else {
		while (index > 1) {
			aux2 = aux;
			aux = aux -> next;
			index--;
		}
		free(aux -> pids);
		free(aux -> line);
		aux2 -> next = aux -> next;
		free(aux);
	}
}

//Esta funcion solamente sirve para debuggear pero si se modifica sirve para hacer la funcion jobs
void	print_lst(t_list **bg)
{
	t_list	*aux;
	int	i;

	aux = *bg;
	while (aux) {
		printf("--------\n");
		printf("id: %d\n", aux -> id);
		printf("pids: ");
		i = -1;
		while (aux -> pids[++i] != -1)
			printf("%d ", aux -> pids[i]);
		printf("\n");
		printf("line: %s\n", aux -> line);
		aux = aux -> next;
	}
}
