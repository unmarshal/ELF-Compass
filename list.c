#include "list.h"

/*
 * function : list_init()
 * purpose  : allocate and initialize list structure
 * arguments: none
 * returns  : pointer to initialized structure
 */
list_t *
list_new(void)
{
	list_t *list;

	if(!(list = malloc(sizeof(list_t)))) {
		perror("malloc");
		return(NULL);
	}

	list->size = 0;
	list->head = NULL;
	list->tail = NULL;

	return(list);
}

/*
 * function : list_ins_next()
 * purpose  : insert data after a particular element
 * arguments: ptr to list, ptr to element, ptr to data
 * returns  : 0 on success, -1 on error
 */
int
list_ins_next(list_t *list, element_t *element, void *data)
{
	element_t *new_element;

	if(!element && list_size(list))
		return(-1);
	
	new_element = (element_t *) malloc(sizeof(element_t));

	if(!new_element) {
		perror("malloc");
		return(-1);
	}

	new_element->data = data;

	if(!list_size(list)) {
		new_element->prev = NULL;
		new_element->next = NULL;
		list->head = new_element;
		list->tail = new_element;
	} else {
		new_element->prev = element;
		new_element->next = element->next;

		if(!element->next) 
			list->tail = new_element;
		else
			element->next->prev = new_element;

		element->next = new_element;
	}

	list->size++;

	return(0);
}

/*
 * function : list_ins_prev()
 * purpose  : insert data before a particular element
 * arguments: ptr to list, ptr to element, ptr to data
 * returns  : 0 on success, -1 on error
 */
int
list_ins_prev(list_t *list, element_t *element, void *data)
{
	element_t *new_element;

	if(!element && list_size(list))
		return(-1);
	
	new_element = (element_t *) malloc(sizeof(element_t));

	if(!new_element) {
		perror("malloc");
		return(-1);
	}

	if(!list_size(list)) {
		new_element->prev = NULL;
		new_element->next = NULL;
		list->head = new_element;
		list->tail = new_element;
	} else {
		new_element->next = element;
		new_element->prev = element->prev;

		if(element->prev == NULL)
			list->head = new_element;
		else
			element->prev->next = new_element;

		element->prev = new_element;
	}

	list->size++;

	return(0);
}

/*
 * function : list_get()
 * purpose  : return element at a particular index, simplifying matrix operations
 * arguments: pointer to list, index value
 * returns  : pointer to data
 */
void *
list_get(list_t *list, int index)
{
	int i;
	element_t *element;

	element = list_head(list);

	for(i = 0; i < list_size(list); i++) {
		
		if(i == index)
			return(list_data(element));
			
		element = list_next(element);
	}

	return(NULL);
}

/*
 * function : list_remove()
 * purpose  : remove an element from the list
 * arguments: ptr to list, ptr to element, ptr to data
 * returns  : 0 on success, -1 on error
 */
int
list_remove(list_t *list, element_t *element, void **data)
{
	if(!element || !list_size(list))
		return(-1);
	
	*data = element->data;

	if(element == list->head) {

		list->head = element->next;

		if(list->head == NULL)
			list->tail = NULL;
		else
			element->next->prev = NULL;
	} else {

		element->prev->next = element->next;

		if(!element->next)
			list->tail = element->prev;
		else
		element->next->prev = element->prev;

	}

	free(element);

	list->size--;

	return(0);
}
