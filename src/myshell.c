#include "../minishell.h"

static void	sig_handler_input(int signal);
void	bgdelete(t_list **bg, int index);
void	print_lst(t_list **bg);

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
	return (0);
}

static void	sig_handler_input(int signal)
{
	if (g_sig == 1)
		write(1, "\n\033[93mmsh > \033[0;0m", 19);
}
