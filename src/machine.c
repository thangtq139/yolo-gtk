#include "machine.h"

char *get_base_path(const char *cmd)
{
	int n;
	char *ret;
	n = strlen(cmd);
	while (n && cmd[n - 1] != '/') --n;
	if (n == 0)
		return NULL;
	ret = malloc(n + 1);
	memcpy(ret, cmd, n);
	ret[n] = '\0';
	return ret;
}

char *str_concate(const char *s1, const char *s2)
{
	int n, m;
	char *ret;
	n = strlen(s1);
	m = strlne(s2);
	ret = malloc(m + n + 1);
	memcpy(ret, s1, n);
	memcpy(ret + n, s2, m);
	ret[m + n] = '\0';
	return ret;
}

void init_dectection(int argc, char *argv[])
{
	extern char **g_yolo_names;
	extern image **g_yolo_alphabet;
	char *base_path = get_base_path(argv[0]);
	if (base_path == NULL) return;
	char *name_list_path = str_concate(base_path, YOLO_NAME_LIST);
	g_yolo_names = get_labels(name_list_path);
	char *alphabet_path = 
	g_yolo_alphabet = 
}
