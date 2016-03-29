#ifndef H_XMAKE
#define H_XMAKE

#include "dep-dag.h"

extern struct dep_vec all_deps;

/* subprocess / target build management */
void start_build(struct dep_node *n);
void collect_output(struct dep_node *n);
int reap(struct dep_node *n);

#endif	/* H_XMAKE #include guard */
