#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#include <err.h>

#include "xmake.h"

struct dep_vec all_deps;
static struct dep_vec running;

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

void start_target(struct dep_node *n)
{
	assert(n->status == BUILD_READY);
	start_build(n);
	if (n->status == BUILD_DONE)
	{
		/* phony target, no command to run */
		assert(n->command == NULL);
		return;
	}
	printf("Starting build for target '%s'\n", n->name);

	dep_vec_add(&running, n);
}

/* Expects n to have been successfully reaped! */
void finish_target(struct dep_node *n)
{
	size_t i;

	printf("Target '%s' is finished building\n", n->name);
	if (n->build_state.out_len > 0)
		printf("Build for '%s' produced output\n", n->name);
	if (n->status == BUILD_FAILED)
		printf("Build for '%s' failed!\n", n->name);
	else if (n->status != BUILD_DONE)
		printf("I am very confused about the build state of '%s' (%u)\n", n->name, (unsigned)n->status);

	for (i = 0; i < running.num; i++)
		if (running.nodes[i] == n)
		{
			running.nodes[i] = running.nodes[--running.num];
			break;
		}
}

static void run_build(void)
{
	while (dag_by_status[BUILD_READY].head != NULL)
	{
		struct dep_node *n = dag_by_status[BUILD_READY].head;

		start_target(n);
		if (running.num == 0) continue;

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

		finish_target(n);
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
