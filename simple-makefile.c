#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <err.h>

#include "xmake.h"

/* ZERO INITIALIZATION HACK */
static const struct dep_node node_zero;

static int compar_node(const void *va, const void *vb)
{
	struct dep_node * const *a = va;
	struct dep_node * const *b = vb;
	return strcmp((*a)->name, (*b)->name);
}

static struct dep_node *get_file(const char *name)
{
	struct dep_node *n;

	/* bsearch gives us a pointer to pointer that we need to deal with */
	struct dep_node **ret;
	struct dep_node key;
	struct dep_node *pkey = &key;
	key.name = (/* no const - we promise not to write it */ char *)name;

	ret = bsearch(&pkey, all_deps.nodes, all_deps.num, sizeof all_deps.nodes[0], compar_node);
	if (ret != NULL)
		return *ret;

	n = malloc(sizeof *n);
	if (n == NULL)
		err(EXIT_FAILURE, "setup_dag: input_file: malloc");

	*n = node_zero;
	n->name = strdup(name);
	dag_set_status(n, BUILD_READY);
	dep_vec_add(&all_deps, n);
	qsort(all_deps.nodes, all_deps.num, sizeof all_deps.nodes[0], compar_node);
	return n;
}

/* strtok()'s line */
static struct dep_node *process_makefile_dependencies(char *line)
{
	char *filename;
	struct dep_node *target;

	filename = strtok(line, ":");
	/* trim whitespace */
	filename += strspn(filename, " \t");
	filename[strcspn(filename, " \t")] = '\0';
	target = get_file(filename);

	while ((filename = strtok(NULL, " \t\n")) != NULL)
		dag_add_dependency(target, get_file(filename));

	return target;
}

void read_simple_makefile(FILE *in)
{
	char readbuf[256];
	struct dep_node *target = NULL;

	while (fgets(readbuf, sizeof readbuf, in) != NULL)
	{
		char c;

		if (readbuf[0] == '\t')
		{
			if (target == NULL)
			{
				fprintf(stderr, "Makefile: Build command found without target!\n");
				exit(EXIT_FAILURE);
			}
			target->command = strdup(readbuf+1);
			target = NULL;
			continue;
		}

		c = readbuf[strspn(readbuf, " \t\n")];
		if (c == '#' || c == '\0')
			continue;

		target = process_makefile_dependencies(readbuf);
	}
}

void setup_dag(void)
{
	FILE *in;

	in = fopen("XMakefile", "r");
	if (in == NULL)
		in = fopen("Makefile", "r");
	if (in == NULL)
		in = fopen("makefile", "r");

	if (in == NULL)
	{
		fprintf(stderr, "Can't find a makefile!\n");
		exit(EXIT_FAILURE);
	}
	read_simple_makefile(in);
	fclose(in);
}
