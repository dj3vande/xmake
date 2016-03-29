#include <assert.h>
#include <stdlib.h>

#include <err.h>

#include "dep-dag.h"

struct dep_list dag_by_status[BUILD_NUM_STATUS];

void dep_vec_add(struct dep_vec *v, struct dep_node *n)
{
	if (v->num >= v->max)
	{
		void *t;
		size_t newmax = v->max * 2;
		if (newmax == 0) newmax = 1;
		t = realloc(v->nodes, newmax * sizeof *(v->nodes));
		if (t == NULL)
			err(EXIT_FAILURE, "dep_vec_add: realloc");
		v->max = newmax;
		v->nodes = t;
	}
	v->nodes[v->num++] = n;
}

void dep_list_unlink(struct dep_list *l, struct dep_node *n)
{
	assert(n->container == l);

	if (n->prev == NULL)
		l->head = n->next;
	else
		n->prev->next = n->next;
	if (n->next == NULL)
		l->tail = n->prev;
	else
		n->next->prev = n->prev;

	n->prev = n->next = NULL;
	n->container = NULL;
}

void dep_list_link(struct dep_list *l, struct dep_node *n)
{
	if (n->container == l)
		return;
	if (n->container != NULL)
		dep_list_unlink(n->container, n);

	assert(n->next == NULL && n->prev == NULL);

	if (l->head == NULL)
		l->head = l->tail = n;
	else
	{
		n->prev = l->tail;
		l->tail->next = n;
		l->tail = n;
	}
	n->container = l;
}

void dag_set_status(struct dep_node *node, enum build_status stat)
{
	if (stat == BUILD_BLOCKED && node->dependencies.num == 0)
		stat = BUILD_READY;
	node->status = stat;
	dep_list_link(&dag_by_status[stat], node);
	if (stat == BUILD_DONE)
	{
		/* Rescan dependents to see if we've unblocked them */
		size_t i;
		for (i = 0; i < node->dependents.num; i++)
		{
			struct dep_node *n = node->dependents.nodes[i];
			size_t j;
			enum build_status s = BUILD_READY;
			for (j = 0; j < n->dependencies.num; j++)
				if (n->dependencies.nodes[j]->status != BUILD_DONE)
					s = BUILD_BLOCKED;
			dag_set_status(n, s);	/* will not recurse */
		}
	}
}

void dag_add_dependency(struct dep_node *node, struct dep_node *dependency)
{
	dep_vec_add(&node->dependencies, dependency);
	dep_vec_add(&dependency->dependents, node);

	if (dependency->status != BUILD_READY)
		dag_set_status(node, BUILD_BLOCKED);
}
