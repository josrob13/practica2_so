#include "../minishell.h"

int	main(void)
{
	char	buf[1024];
	tline	*line;

	fputs("msh> ", stdout);
	while (fgets(buf, 1024, stdin)) {
		line = tokenize(buf);
		if (!line)
			continue;
		execute(line);
		fputs("msh> ", stdout);
	}
	return (0);
}
