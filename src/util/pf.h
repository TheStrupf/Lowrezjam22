#ifndef PF_H
#define PF_H

#define MAX_NEIGHBOURS 8
#define MAX_SCORE 0xFFFFF

typedef struct pf_node {
        int x;
        int y;
        unsigned int g;
        unsigned int f;
        struct pf_node *parent; // traverse back from goal for path
        struct pf_node *next;   // internal open list
        struct pf_node *neighbours[MAX_NEIGHBOURS];
        int distance[MAX_NEIGHBOURS];
        int n_neighbours;
        char open;
} pf_node;

int pf_find(pf_node *network, int num, pf_node *start, pf_node *goal);
void pf_connect(pf_node *, pf_node *);

#endif