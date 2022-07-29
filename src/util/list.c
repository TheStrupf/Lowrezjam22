#include "list.h"
#include "engine.h"
#include <stdlib.h>
#include <string.h>

list *list_create_sized(int cap, size_t esize)
{
        void *m1 = MALLOC(sizeof(list));
        if (m1) {
                void *m2 = MALLOC(esize * cap);
                if (m2) {
                        list *l = m1;
                        l->data = m2;
                        l->esize = esize;
                        l->n = 0;
                        l->cap = cap;
                        return l;
                }
                FREE(m1);
        }
        return NULL;
}

void list_destroy(list *l)
{
        if (l) {
                FREE(l->data);
                FREE(l);
        }
}

void list_push(list *l, void *p)
{
        if (l->n == l->cap) {
                void *temp = REALLOC(l->data, l->esize * l->cap * 2);
                if (!temp) return;
                l->cap *= 2;
                l->data = temp;
        }
        list_set(l, p, l->n++);
}

void list_set(list *l, void *p, int i)
{
        memcpy((char *)l->data + i * l->esize, p, l->esize);
}

void list_del_at(list *l, int i)
{
        if (i < 0 || i >= l->n) return;
        if (--l->n > 0)
                memmove(
                    (char *)l->data + i * l->esize,
                    (char *)l->data + (i + 1) * l->esize,
                    l->esize * (l->n - i));
}

void list_del(list *l, void *p)
{
        list_del_at(l, list_find(l, p));
}

int list_find(list *l, void *p)
{
        char *it = l->data;
        for (int n = 0; n < l->n; n++, it += l->esize) {
                if (memcmp(it, p, l->esize) == 0)
                        return n;
        }
        return -1;
}

void list_pop(list *l, void *out)
{
        if (l->n == 0) memset(out, 0, l->esize);
        else list_get(l, --l->n, out);
}

void list_get(list *l, int i, void *out)
{
        if (i < l->n) memcpy(out, (char *)l->data + i * l->esize, l->esize);
        else memset(out, 0, l->esize);
}