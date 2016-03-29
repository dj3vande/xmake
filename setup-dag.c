#include <stdlib.h>
#include <string.h>

#include <err.h>

#include "xmake.h"

/*
 * At some point, we'll want to be able to build things other than ourself.
 * For that to happen, we'll need some kind of input file parser instead of
 * having code that sets up our dependency graph.
 */

/* ZERO INITIALIZATION HACK */
static const struct dep_node node_zero;

static struct dep_node *setup_file(const char *name)
{
	struct dep_node *n = malloc(sizeof *n);
	if (n == NULL)
		err(EXIT_FAILURE, "setup_dag: input_file: malloc");

	*n = node_zero;
	n->name = strdup(name);
	dag_set_status(n, BUILD_READY);
	dep_vec_add(&all_deps, n);
	return n;
}

void setup_dag(void)
{
	struct dep_node *source_file, *obj_file;

	struct dep_node *xmake_h = setup_file("xmake.h");
	struct dep_node *depdag_h = setup_file("dep-dag.h");
	struct dep_node *xmake = setup_file("xmake");

	source_file = setup_file("dep-dag.c");
	obj_file = setup_file("dep-dag.o");
	obj_file->command = strdup("cc -Wall -Wextra -std=c99 -pedantic -O -c dep-dag.c");
	dag_add_dependency(obj_file, source_file);
	dag_add_dependency(obj_file, depdag_h);
	dag_add_dependency(xmake, obj_file);

	source_file = setup_file("setup-dag.c");
	obj_file = setup_file("setup-dag.o");
	obj_file->command = strdup("cc -Wall -Wextra -std=c99 -pedantic -O -D_POSIX_C_SOURCE=200112L -c setup-dag.c");
	dag_add_dependency(obj_file, source_file);
	dag_add_dependency(obj_file, depdag_h);
	dag_add_dependency(obj_file, xmake_h);
	dag_add_dependency(xmake, obj_file);

	source_file = setup_file("build.c");
	obj_file = setup_file("build.o");
	obj_file->command = strdup("cc -Wall -Wextra -std=c99 -pedantic -O -D_POSIX_C_SOURCE=200112L -c build.c");
	dag_add_dependency(obj_file, source_file);
	dag_add_dependency(obj_file, depdag_h);
	dag_add_dependency(obj_file, xmake_h);
	dag_add_dependency(xmake, obj_file);

	source_file = setup_file("build.c");
	obj_file = setup_file("build.o");
	obj_file->command = strdup("cc -Wall -Wextra -std=c99 -pedantic -O -D_POSIX_C_SOURCE=200112L -c build.c");
	dag_add_dependency(obj_file, source_file);
	dag_add_dependency(obj_file, depdag_h);
	dag_add_dependency(obj_file, xmake_h);
	dag_add_dependency(xmake, obj_file);

	source_file = setup_file("xmake.c");
	obj_file = setup_file("xmake.o");
	obj_file->command = strdup("cc -Wall -Wextra -std=c99 -pedantic -O -c xmake.c");
	dag_add_dependency(obj_file, source_file);
	dag_add_dependency(obj_file, depdag_h);
	dag_add_dependency(xmake, obj_file);

	xmake->command = strdup("cc -o xmake xmake.o build.o dep-dag.o setup-dag.o");
}
