#include "pf.h"
#include "engine.h"
#include <stdlib.h>

static int pf_H(pf_node *a, pf_node *b)
{
        return ABS(a->x - b->x) + ABS(a->y - b->y);
}

int pf_find(pf_node *network, int num, pf_node *start, pf_node *goal)
{
        pf_node *current;
        pf_node *open = start;
        start->next = NULL;
        start->parent = NULL;
        for (int n = 0; n < num; n++) {
                pf_node *node = &network[n];
                node->parent = node->next = NULL;
                node->g = node->f = MAX_SCORE;
                node->open = 0;
        }
        start->g = 0;
        start->f = pf_H(start, goal);

        while (open) {
                int lowestf = MAX_SCORE;
                pf_node *cprev = NULL;
                for (pf_node *n = open, *prev = NULL; n; prev = n, n = n->next) {
                        if (n->f < lowestf) {
                                lowestf = n->f;
                                current = n;
                                cprev = prev;
                        }
                }

                if (current == goal) return 1;

                if (cprev) cprev->next = current->next;
                else open = current->next;
                current->open = 0;

                for (int n = 0; n < current->n_neighbours; n++) {
                        pf_node *neighbour = current->neighbours[n];
                        unsigned int gtemp = current->g + current->distance[n];
                        if (gtemp < neighbour->g) {
                                neighbour->parent = current;
                                neighbour->g = gtemp;
                                neighbour->f = gtemp + pf_H(neighbour, goal);
                                if (!neighbour->open) {
                                        neighbour->next = open;
                                        open = neighbour;
                                        neighbour->open = 1;
                                }
                        }
                }
        }
        return 0;
}

void pf_connect(pf_node *a, pf_node *b)
{
        if (a == NULL || b == NULL)
                return;
        for (int n = 0; n < a->n_neighbours; n++)
                if (a->neighbours[n] == b) return;
        int dx = a->x - b->x;
        int dy = a->y - b->y;
        int dis = isqrt(dx * dx + dy * dy);
        a->neighbours[a->n_neighbours] = b;
        b->neighbours[b->n_neighbours] = a;
        a->distance[a->n_neighbours++] = dis;
        b->distance[b->n_neighbours++] = dis;
}