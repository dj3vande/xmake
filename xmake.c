#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#include <err.h>

#include "xmake.h"

struct dep_vec all_deps;

int main(void)
{
	struct dep_node *n;
	setup_dag();

	while (dag_by_status[BUILD_READY].head != NULL)
	{
		n = dag_by_status[BUILD_READY].head;
		printf("Starting build for output '%s'...\n", n->name);
		start_build(n);

		if (n->status == BUILD_DONE)
		{
			printf("Target '%s' completed trivially\n", n->name);
			continue;
		}

		/* Lazy substitute for subprocess management */
		while(!reap(n))
		{
			sleep(1);
			collect_output(n);
		}

		if (n->build_state.out_len > 0)
			printf("Build for '%s' produced output\n", n->name);
		if (n->status == BUILD_FAILED)
			printf("Build for '%s' failed!\n", n->name);
		else if (n->status != BUILD_DONE)
			printf("I am very confused about the build state of '%s' (%u)\n", n->name, (unsigned)n->status);
	}

	printf("Finished building!\n");
	for (n = dag_by_status[BUILD_DONE].head; n != NULL; n = n->next)
	{
		if (n->build_state.out_len > 0)
		{
			printf("Build output for '%s':\n", n->name);
			printf("========\n");
			fwrite(n->build_state.output, 1, n->build_state.out_len, stdout);
			printf("========\n");
		}
	}
	for (n = dag_by_status[BUILD_FAILED].head; n != NULL; n = n->next)
	{
		if (n->build_state.out_len > 0)
		{
			printf("Build output for FAILED TARGET '%s':\n", n->name);
			printf("========\n");
			fwrite(n->build_state.output, 1, n->build_state.out_len, stdout);
			printf("========\n");
		}
		else
			printf("Target '%s' FAILED SILENTLY\n", n->name);
	}

	return (dag_by_status[BUILD_FAILED].head == NULL) ? EXIT_SUCCESS : EXIT_FAILURE;
}
