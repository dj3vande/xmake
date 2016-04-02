#ifndef H_XMAKE
#define H_XMAKE

#include "dep-dag.h"

extern struct dep_vec all_deps;

void setup_dag(void);

/* subprocess / target build management */
void start_build(struct dep_node *n);
void collect_output(struct dep_node *n);
int reap(struct dep_node *n);

/* File time checking and comparison */
void get_node_time(struct dep_node *n);
/* Expects pointers to struct timedata */
int compar_timedata(const void *va, const void *vb);
/* Expects pointers to struct dep_node.  Uses internal cached times. */
int compar_node_time(const void *va, const void *vb);

#endif	/* H_XMAKE #include guard */
