#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#include <err.h>

#include "xmake.h"

struct dep_vec all_deps;

static void report_results(void)
{
	struct dep_node *n;
	for (n = dag_by_status[BUILD_DONE].head; n != NULL; n = n->next)
	{
		if (n->build_state.out_len > 0)
		{
			printf("Target '%s' generated output:\n", n->name);
			printf("Build command:\n");
			printf("========\n");
			printf("%s\n", n->command);
			printf("========\n");
			printf("Output:\n");
			printf("========\n");
			fwrite(n->build_state.output, 1, n->build_state.out_len, stdout);
			printf("========\n");
		}
	}
	for (n = dag_by_status[BUILD_FAILED].head; n != NULL; n = n->next)
	{
		if (n->build_state.out_len > 0)
		{
			printf("FAILED TARGET '%s':\n", n->name);
			printf("Build command:\n");
			printf("========\n");
			printf("%s\n", n->command);
			printf("========\n");
			printf("Output:\n");
			printf("========\n");
			fwrite(n->build_state.output, 1, n->build_state.out_len, stdout);
			printf("========\n");
		}
		else
			printf("Target '%s' FAILED SILENTLY\n", n->name);
	}
}

static void run_build(void)
{
	while (dag_by_status[BUILD_READY].head != NULL)
	{
		struct dep_node *n = dag_by_status[BUILD_READY].head;
		start_build(n);	/* unlinks from ready list */
		if (n->status == BUILD_DONE)	// happens if no dependents
			continue;
		printf("Starting build for output '%s'...\n", n->name);

		/*
		 * Wait until it's done.
		 * We rely on the fd selecting as readable when the subprocess
		 * exits (since the kernel closes it causing a read to return
		 * 0 bytes without blocking).
		 */
		do
		{
			fd_set fds;
			int fd = n->build_state.pipefd;
			FD_ZERO(&fds);
			FD_SET(fd, &fds);
			/* For "block until something happens" side effect */
			select(fd+1, &fds, NULL, NULL, NULL);
			collect_output(n);
		} while (!reap(n));

		printf("Target '%s' is finished building\n", n->name);

		if (n->build_state.out_len > 0)
			printf("Build for '%s' produced output\n", n->name);
		if (n->status == BUILD_FAILED)
			printf("Build for '%s' failed!\n", n->name);
		else if (n->status != BUILD_DONE)
			printf("I am very confused about the build state of '%s' (%u)\n", n->name, (unsigned)n->status);
	}
}

int main(void)
{
	setup_dag();
	run_build();
	printf("Finished building!\n");
	report_results();

	return (dag_by_status[BUILD_FAILED].head == NULL) ? EXIT_SUCCESS : EXIT_FAILURE;
}
