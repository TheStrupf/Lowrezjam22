#ifndef LIST_H
#define LIST_H

#include <stdlib.h>

#define FOREACHI(A, IT) for (int IT = 0; IT < arr_len(A); IT++)
#define arr_push(A, P) A = arr_push_(A, P)
#define arr_insert(A, P, i) A = arr_insert_(A, P, i)

int arr_len(void *arr);
void *arr_new_sized(int cap, size_t esize);
void *arr_new(size_t esize);
void arr_destroy(void *arr);
void arr_clear(void *arr);
void *arr_push_(void *arr, void *p);
void arr_del_at(void *arr, int i);
void arr_del(void *arr, void *p);
int arr_find(void *arr, void *p);
void arr_pop(void *arr, void *p);
void *arr_insert_(void *arr, void *p, int at);

#endif