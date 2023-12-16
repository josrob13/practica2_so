#include "../minishell.h"

static void	sig_handler_input(int signal);

int	g_sig = 1;

int	main(void)
{
	char	buf[1024];
	tline	*line;

	signal(SIGINT, sig_handler_input);
	fputs(INPUT_PRINT, stdout);
	while (fgets(buf, 1024, stdin)) {
		line = tokenize(buf);
		if (line -> ncommands > 0)
			execute(line);
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
