#include "../minishell.h"

int	main(void)
{
	char	buf[1024];
	tline	*line;

	fputs(INPUT_PRINT, stdout);
	while (fgets(buf, 1024, stdin)) {
		line = tokenize(buf);
		if (!line)
			continue;
		if (line -> ncommands > 0)
			execute(line);
		fputs(INPUT_PRINT, stdout);
	}
	return (0);
}
