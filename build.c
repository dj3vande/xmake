#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/select.h>

#include <err.h>

#include "xmake.h"

void start_build(struct dep_node *n)
{
	int pipefd[2];
	int nullfd;
	pid_t pid;
	char *argv[4] = {"sh", "-c"}; /* [2] filled in later, [3] sentinel */

	assert(n->status == BUILD_READY);

	if (n->command == NULL)
	{
		dag_set_status(n, BUILD_DONE);
		return;
	}
	argv[2] = n->command;

	if (pipe(pipefd) == -1)
		err(EXIT_FAILURE, "start_build: pipe");
	nullfd = open("/dev/null", O_RDONLY);
	if (nullfd == -1)
		err(EXIT_FAILURE, "start_build: open: /dev/null");

	pid = fork();
	if (pid == 0)
	{
		/* child: set up and exec */
		if (dup2(pipefd[1], STDERR_FILENO) == -1)
		{
			/* Note, this will not be captured by the parent */
			const char *msg = "failed to dup2 stderr - can't run command\n";
			write(STDERR_FILENO, msg, strlen(msg));
			_exit(126);
		}
		if (dup2(nullfd, STDIN_FILENO) == -1)
		{
			const char *msg = "failed to dup2 stdin - can't run command\n";
			write(STDERR_FILENO, msg, strlen(msg));
			_exit(126);
		}
		if (dup2(pipefd[1], STDOUT_FILENO) == -1)
		{
			const char *msg = "failed to dup2 stdout - can't run command\n";
			write(STDERR_FILENO, msg, strlen(msg));
			_exit(126);
		}
		if (execv("/bin/sh", argv) == -1)
		{
			const char *msg = "failed to exec command\n";
			write(STDERR_FILENO, msg, strlen(msg));
			_exit(127);
		}
		/* not reached */
	}

	/* If we get here, we're the parent */
	close(nullfd);
	close(pipefd[1]);
	n->build_state.pid = pid;
	n->build_state.pipefd = pipefd[0];
	/*
	 * If we've already built this file, we can recycle the space we
	 * allocated for its output, but make sure we replace and not append.
	 */
	n->build_state.out_len = 0;
	dag_set_status(n, BUILD_RUNNING);
}

void collect_output(struct dep_node *n)
{
	fd_set fds;
	int fd = n->build_state.pipefd;
	struct timeval t;
	int ret;

	assert(n->status == BUILD_RUNNING);

	FD_ZERO(&fds);
	FD_SET(fd, &fds);
	t.tv_sec = 0;
	t.tv_usec = 0;
	while ((ret = select(fd+1, &fds, NULL, NULL, &t)) > 0)
	{
		char readbuf[128];
		ssize_t nread = read(fd, readbuf, sizeof readbuf);
		if (nread == -1)
			err(EXIT_FAILURE, "collect_output: read");
		if (nread == 0)	/* Other end probably closed the pipe */
			return;

		if (n->build_state.out_len + nread > n->build_state.out_max)
		{
			void *t;
			size_t newmax = 2 * n->build_state.out_max;
			if (newmax < 1024)
				newmax = 1024;
			t = realloc(n->build_state.output, newmax);
			if (t == NULL)
				err(EXIT_FAILURE, "collect_output: realloc");
			n->build_state.output = t;
			n->build_state.out_max = newmax;
		}
		memcpy(n->build_state.output + n->build_state.out_len,
		       readbuf, nread);
		n->build_state.out_len += nread;

		/* re-setup for select */
		FD_ZERO(&fds);
		FD_SET(fd, &fds);
		t.tv_sec = 0;
		t.tv_usec = 0;
	}
}

/* Returns nonzero if it actually reaped something */
int reap(struct dep_node *n)
{
	int status;
	int ret;

	assert(n->status == BUILD_RUNNING);

	ret = waitpid(n->build_state.pid, &status, WNOHANG);
	if (ret == 0)
		return 0;
	if (ret == -1)
		err(EXIT_FAILURE, "reap: waitpid");

	/* We reaped something, update our internal status */
	collect_output(n);	/* to grab anything left behind */
	close(n->build_state.pipefd);
	n->build_state.pid = 0;
	n->build_state.pipefd = -1;

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
		dag_set_status(n, BUILD_DONE);
	else
		dag_set_status(n, BUILD_FAILED);

	return 1;
}
