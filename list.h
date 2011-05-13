#ifndef _LIST_H
#define _LIST_H

#include <stdio.h>
#include <stdlib.h>

/*
 * list element definition
 */
typedef struct _list_element {
	void *data;
	struct _list_element *prev, *next;
} element_t;

/*
 * list structure definition
 */
typedef struct _list {
	int size;
	element_t *head, *tail;
} list_t;

/*
 * function prototypes
 */
list_t	*list_new(void);
int 		list_ins_next(list_t *, element_t *, void *);
int 		list_ins_prev(list_t *, element_t *, void *);
int 		list_remove(list_t *, element_t *, void **);
void 		*list_get(list_t *, int);

/* 
 * utility macros
 */
#define list_size(list) ((list)->size)
#define list_tail(list) ((list)->tail)
#define list_head(list) ((list)->head)
#define list_next(element) ((element)->next)
#define list_data(element) ((element)->data)

#endif

