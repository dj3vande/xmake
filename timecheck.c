#include <stdlib.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>

#include "xmake.h"

struct timedata
{
	struct timespec ts;
};

void get_node_time(struct dep_node *n)
{
	struct stat s;

	if (stat(n->name, &s) == -1)
	{
		if (n->time != NULL)
		{
			free(n->time);
			n->time = NULL;
		}
		return;
	}

	if (n->time == NULL)
	{
		n->time = malloc(sizeof *n->time);
		if (n->time == NULL)
			err(EXIT_FAILURE, "get_node_time: malloc");
	}
	n->time->ts = s.st_mtimespec;
}

int compar_timedata(const void *va, const void *vb)
{
	/* pointer to a struct, suitably converted, points to first member */
	struct timespec * const *a = va;
	struct timespec * const *b = vb;

	/* Absent is older than anything else */
	if (*a == NULL && *b == NULL)
		return 0;
	else if (*a == NULL)
		return -1;
	else if (*b == NULL)
		return 1;

	/* Assumes time_t is monotonic.  Implied by POSIX requirements. */
	if ((*a)->tv_sec < (*b)->tv_sec)
		return -1;
	else if ((*a)->tv_sec > (*b)->tv_sec)
		return 1;
	else if ((*a)->tv_nsec < (*b)->tv_nsec)
		return -1;
	else
		return (*a)->tv_nsec > (*b)->tv_nsec;
}

int compar_node_time(const void *va, const void *vb)
{
	struct dep_node * const *a = va;
	struct dep_node * const *b = vb;
	return compar_timedata(&((*a)->time), &((*b)->time));
}
