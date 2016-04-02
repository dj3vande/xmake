#ifndef H_DEP_DAG
#define H_DEP_DAG

#include <stddef.h>

enum build_status
{
	BUILD_BLOCKED,	/* dependency is not done */
	BUILD_READY,	/* all dependencies are done */
	BUILD_RUNNING,	/* currently working on it */
	BUILD_FAILED,	/* returned failure status */
	BUILD_DONE,	/* completely finished */
	BUILD_NUM_STATUS
};

struct timedata;	/* owned by time-checking module */

/*
 * Every dep_node can be in at most one dep_list (used for keeping track of
 * which nodes are in which state).  A dep_node knows which dep_list it's in
 * (so a dep_node pointer has all the information we need to move that node
 * from one list to another).
 * dep_vec is a many-to-many container for dep_node, but without the
 * traceability.
 */

struct dep_node;

struct dep_vec
{
	struct dep_node **nodes;
	size_t num, max;
};

struct dep_list
{
	struct dep_node *head, *tail;
};

struct dep_node
{
	char *name;	/* Nominally filename */
	enum build_status status;

	char *command;
	struct {
		int pid;	/* 0 if not running */
		int pipefd;	/* to read output from, -1 if absent */
		char *output;	/* as read from pipefd */
		size_t out_len, out_max;
	} build_state;

	/* NULL if file is absent */
	struct timedata *time;

	struct dep_vec dependencies;
	struct dep_vec dependents;
	struct dep_list *container;
	struct dep_node *next, *prev;
};

extern struct dep_list dag_by_status[BUILD_NUM_STATUS];

/* Add an entry to a dep_vec, allocating more memory if necessary */
void dep_vec_add(struct dep_vec *v, struct dep_node *n);

/* dep_list bookkeeping */
void dep_list_unlink(struct dep_list *l, struct dep_node *n);
void dep_list_link(struct dep_list *l, struct dep_node *n);

/* Set the current build status */
void dag_set_status(struct dep_node *node, enum build_status stat);

/* Add a dependency relation and update all bookkeeping */
void dag_add_dependency(struct dep_node *node, struct dep_node *dependency);

/* Rescan dependency timestamps and mark node as done if appropriate */
void dag_rescan(struct dep_node *node);

#endif	/* H_DEP_DAG #include guard */
