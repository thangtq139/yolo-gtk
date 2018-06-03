#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define BUF_SZ 1024

int main(int argc, char *argv[])
{
	if (argc < 3) {
		fprintf(stderr, "Usage: %s input.xml output.str\n", argv[0]);
		return -1;
	}
	FILE *fid = fopen(argv[1], "r");
	if (!fid) {
		fprintf(stderr, "Usage: %s input.xml output.str\n", argv[0]);
		return -1;
	}
	FILE *fout = fopen(argv[2], "w");
	char *buf = malloc(BUF_SZ + 1);
	int i, n, t;
	t = 0;
	while (fgets(buf, BUF_SZ, fid) != NULL) {
		n = strlen(buf);
		fprintf(fout, "\"");
		for (i = 0; i < n; ++i) {
			if (buf[i] == '"')
				fprintf(fout, "\\\"");
			else if (buf[i] != '\n')
				fprintf(fout, "%c", buf[i]);
			if (buf[i] == '/' && buf[i - 1] == '<')
				--t;
			if (buf[i] == '<' && buf[i + 1] != '/')
				++t;
		}
		fprintf(fout, "\\n\"\n");
	}
	fclose(fout);
	fclose(fid);
	free(buf);
	return 0;
}
