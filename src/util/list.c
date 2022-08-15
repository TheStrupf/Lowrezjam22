#include "list.h"
#include "engine.h"
#include <stdlib.h>
#include <string.h>

typedef struct arrheader {
        int cap;
        int n;
        size_t esize;
} arrheader;

static const size_t ARRHEADERSIZE = sizeof(arrheader);
#define ARRHEADER(P) ((arrheader *)((char *)P - ARRHEADERSIZE))

int arr_len(void *arr)
{
        return ARRHEADER(arr)->n;
}

void *arr_new_sized(int cap, size_t esize)
{
        void *mem = MALLOC(ARRHEADERSIZE + cap * esize);
        if (mem) {
                arrheader *h = mem;
                h->cap = cap;
                h->esize = esize;
                h->n = 0;
                return (char *)mem + ARRHEADERSIZE;
        }
        return NULL;
}

void *arr_new(size_t esize)
{
        return (arr_new_sized(16, esize));
}

void arr_destroy(void *arr)
{
        FREE(ARRHEADER(arr));
}

void arr_clear(void *arr)
{
        ARRHEADER(arr)->n = 0;
}

static void arr_set_(void *arr, void *p, int at)
{
        arrheader *h = ARRHEADER(arr);
        memcpy((char *)arr + at * h->esize, p, h->esize);
}

void *arr_push_(void *arr, void *p)
{
        void *array = arr;
        arrheader *h = ARRHEADER(arr);
        if (h->n == h->cap) {
                int prevn = h->n;
                array = arr_new_sized(h->cap * 2, h->esize);
                if (!array) return NULL;
                h = ARRHEADER(array);
                h->n = prevn;
                memcpy(array, arr, h->n * h->esize);
        }
        arr_set_(array, p, h->n++);
        return array;
}

void arr_del_at(void *arr, int i)
{
        arrheader *h = ARRHEADER(arr);
        if (i < 0 || i >= h->n) return;
        if (i < --h->n)
                memmove((char *)arr + i * h->esize,
                        (char *)arr + (i + 1) * h->esize,
                        h->esize * (h->n - i));
}

void arr_del(void *arr, void *p)
{
        arr_del_at(arr, arr_find(arr, p));
}

int arr_find(void *arr, void *p)
{
        arrheader *h = ARRHEADER(arr);
        char *it = arr;
        for (int n = 0; n < h->n; n++, it += h->esize) {
                if (memcmp(it, p, h->esize) == 0)
                        return n;
        }
        return -1;
}

void arr_pop(void *arr, void *p)
{
        arrheader *h = ARRHEADER(arr);
        if (h->n > 0) memcpy(p, (char *)arr + --h->n, h->esize);
        else memset(p, 0, h->esize);
}

void *arr_insert_(void *arr, void *p, int at)
{
        arrheader *h = ARRHEADER(arr);
        if (at > h->n) return arr;
        void *array = arr_push_(arr, p);
        if (!array) return array;

        h = ARRHEADER(array);
        memmove((char *)array + (at + 1) * h->esize,
                (char *)array + at * h->esize,
                h->esize * (h->n + 1 - at));
        arr_set_(array, p, at);
        return array;
}