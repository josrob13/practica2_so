#include "../minishell.h"

static void	sig_handler_input(int signal);

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

void	kill_childs(t_list **bg)
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
	if (g_sig == 1)
		write(1, "\n\033[93mmsh > \033[0;0m", 19);
}
