#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#include <err.h>

#include "xmake.h"

#define MAX_JOBS	2

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
	struct dep_node *n;

	while (dag_by_status[BUILD_READY].head != NULL || running.num > 0)
	{
		size_t i;
		int maxfd;
		fd_set fds;

		while (running.num < MAX_JOBS)
		{
			n = dag_by_status[BUILD_READY].head;
			if (n == NULL)
				break;
			start_target(n);
		}

		if (running.num == 0) continue;

		/*
		 * Wait for something to happen.
		 * We rely on our end of the pipe selecting as readable when
		 * the subprocess exits (since the kernel closes it causing a
		 * read to return 0 bytes without blocking); for bonus
		 * correctness points, we should watch for SIGCHLD.
		 */
		FD_ZERO(&fds);
		for (i = 0; i < running.num; i++)
		{
			int fd;
			n = running.nodes[i];
			fd = n->build_state.pipefd;
			FD_SET(fd, &fds);
			if (fd > maxfd)
				maxfd = fd;
		}
		/*
		 * We only use the blocking side effect of select().
		 * We could avoid a few bogus system calls by being more
		 * careful here.
		 */
		select(maxfd+1, &fds, NULL, NULL, NULL);

		for (i = 0; i < running.num; )
		{
			n = running.nodes[i];
			collect_output(n);
			if (reap(n))
				finish_target(n);
			else
				i++;
		}
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
