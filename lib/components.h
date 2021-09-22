#ifndef C_LIB_COMPONENTS_H
#define C_LIB_COMPONENTS_H

#include <stdio.h>

#define STRING_DEFAULT_SIZE	 512
#define STRING_GROWTH_FACTOR 2

typedef struct node {
	void *		 value;
	struct node *next;
} node_t;

typedef struct list {
	struct node * head;
	struct node **end;
	size_t		  size;
} list_t;

typedef struct array {
	void * data;
	size_t size_of_type;
	size_t allocated;
	size_t size;
} array_t;

typedef void (*foreach_callback_t)(void *, va_list);

list_t list_create();
void   list_append(list_t *list, void *element);
void   list_append_n(list_t *list, ...);
void * list_remove(list_t *list, void *element);
void   list_remove_n(list_t *list, ...);
void   list_remove_list(list_t *list, list_t *elements);
void * list_get_nth_element(list_t *list, size_t index);
// void   list_foreach(list_t *list, void (*callback)(void *, va_list), ...);
void list_clear(list_t *list);

array_t array_create_impl(size_t size_of_type, size_t reserved);
void	array_init(array_t *where, size_t size_of_type, size_t reserved);
void	array_append_impl(array_t *array, void *element);
void	array_unsorted_remove(array_t *array, size_t index);
void	array_from(array_t *array, void *static_array, size_t element_size, size_t elements);
// void	array_foreach(array_t *array, void (*callback)(void *, va_list), ...);
void array_free(array_t *array, void (*callback)(void *));

#define array_create(type, number_of_elements) array_create_impl(sizeof(type), number_of_elements)

#define array_append(type, array, element)             \
	do {                                         \
		type _el = element;           \
		array_append_impl(array, (void *) &_el); \
	} while (0)

#define list_foreach(list, callback, ...)                               \
	do {                                                                \
		node_t *_macrovar_node = list->head;                            \
		while (_macrovar_node) {                                        \
			callback(_macrovar_node->value __VA_OPT__(, ) __VA_ARGS__); \
			_macrovar_node = _macrovar_node->next;                      \
		}                                                               \
	} while (0)

#define array_foreach_cb(array, callback, ...)                                                             \
	do {                                                                                                   \
		for (size_t _macrovar_i = 0; _macrovar_i < array->size; _macrovar_i++)                             \
			callback((char *) array->data + _macrovar_i * array->size_of_type __VA_OPT__(, ) __VA_ARGS__); \
	} while (0)

#define array_foreach(type, _var_name, array) for (type *_var_name = array.data; _var_name < (type *) array.data + array.size; _var_name++)
#endif /* C_LIB_COMPONENTS_H */

#ifdef C_LIB_COMPONENTS_IMPLEMENTATION
#undef C_LIB_COMPONENTS_IMPLEMENTATION
#include "utils.h"

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

list_t list_create() {
	return (list_t){0};
}

void list_append(list_t *list, void *element) {
	if (!list->head) {
		list->head = malloc(sizeof(node_t));
		list->end  = &list->head;
	} else
		*list->end = malloc(sizeof(node_t));

	(*list->end)->value = element;
	list->end			= &(*list->end)->next;
	*list->end			= NULL;
	list->size++;
}

void list_append_n(list_t *list, ...) {
	va_list ap;
	va_start(ap, list);
	void *element;

	for (;;) {
		if (element = va_arg(ap, void *), !element) break;
		list_append_n(list, element);
	}

	va_end(ap);
}

void *list_remove(list_t *list, void *element) {
	node_t *ptr = list->head, *prev = NULL;
	while (ptr) {
		if (ptr->value == element) {
			if (!prev) list->head = ptr->next;
			else
				prev->next = ptr->next;
			list->size--;
			return ptr->value;

		} else {
			prev = ptr;
			ptr	 = ptr->next;
		}
	}
	return NULL;
}

void list_remove_n(list_t *list, ...) {
	va_list ap;
	va_start(ap, list);
	for (;;) {
		void *element = va_arg(ap, void *);
		if (!element) break;
		list_remove(list, element);
	}
	va_end(ap);
}

void list_remove_list(list_t *list, list_t *elements) {
	node_t *ptr = elements->head;
	while (ptr) {
		list_remove(list, ptr->value);
		ptr = ptr->next;
	}
}

void *list_get_nth_element(list_t *list, size_t index) {
	node_t *ptr = list->head;
	size_t	pos = 0;

	while (ptr && pos++ < index) ptr = ptr->next;
	return ptr;
}

void list_clear(list_t *list) {
	node_t *node = list->head, *next;
	while (node) {
		next = node->next;
		free(node);
		node = next;
	}
}

void array_init(array_t *where, size_t size_of_type, size_t reserved) {
	where->data			= malloc(reserved * size_of_type);
	where->size			= 0;
	where->size_of_type = size_of_type;
	where->allocated	= reserved;
}

array_t array_create_impl(size_t size_of_type, size_t reserved) {
	array_t array;
	array_init(&array, size_of_type, reserved);
	return array;
}

void array_append_impl(array_t *array, void *element) {
	if (array->allocated <= array->size)
		array->data = realloc(array->data, array->size_of_type * array->size * 2);
	memcpy((char *) array->data + array->size++ * array->size_of_type, element, array->size_of_type);
}

void array_unsorted_remove(array_t *array, size_t index) {
	if (!array->size) return;
	if (array->size == 1) array->size--;
	else
		memcpy((char *) array->data + index * array->size_of_type,
			(char *) array->data + --array->size * array->size_of_type, array->size_of_type);
}

void array_from(array_t *array, void *static_array, size_t element_size, size_t elements) {
	array_init(array, element_size, elements);
	memcpy(array->data, static_array, element_size * elements);
	array->size = elements;
}

void array_free(array_t *array, void (*callback)(void *)) {
	array_foreach_cb(array, callback);
	free(array->data);
}

#endif