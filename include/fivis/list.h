/**
 * A doubly-linked list.
 *
 * This implementation requires the link pointers to be embedded in the
 * structures being linked. Based on the Linux kernel list implementation.
 *
 * @author Lubomir Bulej <bulej@d3s.mff.cuni.cz>
 */

#ifndef _LIST_H_
#define _LIST_H_

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

//

/**
 * Represents both a standalone list head (not embedded in a containing
 * structure) or a list item (embedded in a containing structure).
 */
struct list {
	struct list * prev;
	struct list * next;
};


/*
 * Various types of visitor/observer functions.
 */
typedef void (* list_destroy_fn) (struct list * item, void * data);
typedef void (* list_visit_fn) (struct list * item, void * data);
typedef int (* list_match_fn) (struct list * item, void * data);


/*
 * Constant list head/item initializer.
 */
#define LIST_INIT(name) (struct list) { .prev = &(name), .next = &(name) }


/**
 * Calculates the pointer to the containing structure based on the offset
 * of the given member in that structure. Casts the resulting pointer to
 * the type of the containing structure.
 *
 * @param ptr
 *	The pointer to the member.
 * @param type
 *	The type of the container struct the member is embedded in.
 * @param member
 *	The name of the member within the containing struct.
 */
#define __container_of(ptr, type, member) ({ \
	const typeof(((type *) 0)->member) * __mptr = (ptr); \
	(type *) ((char *) __mptr - offsetof(type, member)); \
})


/**
 * Initializes a list head/item to point back to itself, assuming the
 * head/item is not NULL. This representation of an empty list simplifies
 * various runtime checks. Returns the itialized item.
 *
 * @param head
 *	The list head/item to initialize. Must not be NULL.
 */
static inline struct list *
__list_init(struct list * head) {
	head->prev = head;
	head->next = head;
	return head;
}


/**
 * Inserts an item into a list by linking it between its immediate predecessor
 * and successor items, and returns the inserted item. The pointer from the
 * successor is linked first, so there can be a moment when the new item is
 * visible in the list during backward (but not forward) traversal. The caller
 * must make sure this is not a problem.
 *
 * @param item
 *	The item to insert.
 * @param pred
 *	The predecessor of the item.
 * @param succ
 *	The successor of the item.
 */
static inline struct list *
__list_insert_between(
	struct list * item, struct list * pred, struct list * succ
) {
	item->next = succ;
	item->prev = pred;

	succ->prev = item;
	pred->next = item;
	return item;
}


/**
 * Removes item(s) from a list by linking together the items immediately
 * before and after the item(s) being removed. The backward pointer is
 * linked first, because lists are less often traversed backwards. The
 * forward pointer is linked last and when that happens, the list is
 * completely linked. The caller must make sure this is not a problem.
 *
 * @param pred
 *	The item preceding the item(s) to be removed.
 * @param succ
 *	The item succeeding the item(s) to be removed.
 */
static inline void
__list_remove_between(struct list * pred, struct list * succ) {
	succ->prev = pred;
	pred->next = succ;
}


/**
 * Initializes the given list head/item to represent an empty list.
 * Returns the initialized list head/item.
 *
 * @param head
 * 	The list head/item to initialize. Must not be NULL.
 */
static inline struct list *
list_init(struct list * head) {
	assert(head != NULL);
	return __list_init(head);
}


/**
 * Allocates and initializes a list head/item. Returns the initialized
 * list head/item or NULL of the allocation failed.
 */
static inline struct list *
list_alloc(void) {
	struct list * result = (struct list *) malloc(sizeof(struct list));
	return (result != NULL) ? __list_init(result) : NULL;
}


/**
 * Returns true if the given list is empty, false otherwise. This function
 * should be used on the standalone list head structure. When used on list
 * items (embedded in a structure), the function will return true if the item
 * is not linked in any list and false otherwise.
 *
 * @param head
 * 	The list head to test.
 */
static inline bool
list_is_empty(struct list * head) {
	assert(head != NULL);
	return (head->prev == head) && (head->next == head);
}




/**
 * Returns a pointer to the containing structure in which the given
 * list item is embedded.
 *
 * @param ptr
 *	Pointer to the 'struct list' embedded in the containing structure.
 * @param type
 *	The type of containing structure.
 * @param member
 *	The name of the 'struct list' field in the containing structure.
 */
#define list_item(ptr, type, member) \
	__container_of(ptr, type, member)

/**
 * Returns a typed structure from the given list item.
 *
 * @param ptr
 *	Pointer to the 'struct list' embedded in the containing structure.
 * @param var
 *	The name of the variable holding a pointer to the containing structure.
 * @param member
 *	The name of the 'struct list' field in the containing structure.
 */
#define list_item_var(ptr, var, member) \
	__container_of(ptr, typeof(* var), member)

/**
 * Inserts a new item to the list after the specified item and
 * returns the added item.
 *
 * @param item
 *	The item to insert. Must not be NULL.
 * @param pred
 *	The item after which to add the new item. Must not be NULL.
 */
static inline struct list *
list_insert_after(struct list * item, struct list * pred) {
	assert(item != NULL && pred != NULL);
	return __list_insert_between (item, pred, pred->next);
}


/**
 * Adds an item at the start of the list represented by the given list head.
 * Returns the newly added item.
 *
 * @param head
 *	The list to which to add the item. Must not be NULL.
 * @param item
 *	The item to add. Must not be NULL.
 */
static inline struct list *
list_add_first(struct list * head, struct list * item) {
	assert (head != NULL && item != NULL);
	return __list_insert_between(item, head, head->next);
}


/**
 * Inserts a new item to the list before the specified item and
 * returns the added item.
 *
 * @param item
 *	The item to insert. Must not be NULL.
 * @param succ
 *	The item before which to add the new item. Must not be NULL.
 */
static inline struct list *
list_insert_before(struct list * item, struct list * succ) {
	assert(item != NULL && succ != NULL);
	return __list_insert_between(item, succ->prev, succ);
}



/**
 * Adds an item to the end of the list represented by the given list head.
 * Returns the newly added item.
 *
 * @param head
 *	The list to which to add the item. Must not be NULL.
 * @param item
 *	The item to add. Must not be NULL.
 */
static inline struct list *
list_add_last(struct list * head, struct list * item) {
	assert (head != NULL && item != NULL);
	return __list_insert_between(item, head->prev, head);
}


/**
 * Removes the given item from the list and reinitializes it to
 * make it look like an empty list. Returns the removed item.
 *
 * @param item
 *	The item to remove. Must not be NULL.
 */
static inline struct list *
list_remove(struct list * item) {
	assert(item != NULL);

	__list_remove_between(item->prev, item->next);
	__list_init(item);
	return item;
}


/**
 * Removes the successor of the given list item from the list and returns
 * it to the caller. The item must have a valid (non-NULL) successor.
 *
 * @param item
 *	The item whose successor to remove. Must not be NULL.
 */
static inline struct list *
list_remove_after(struct list * item) {
	assert(item != NULL);
	return list_remove(item->next);
}


/**
 * Removes the predecessor of the given list item from the list and returns
 * it to the caller. The item must have a valid (non-NULL) predecessor.
 *
 * @param item
 *	The item whose predecessor to remove. Must not be NULL.
 */
static inline struct list *
list_remove_before (struct list * item) {
	assert(item != NULL);
	return list_remove(item->prev);
}


/**
 * Iterates over the given list.
 *
 * @param curr
 *	Name of the loop control variable pointing to the current list item.
 * @param head
 *	The head of the list to iterate over. Must not be NULL.
 */
#define list_for_each(curr, head) \
	assert((head) != NULL); \
	for (curr = (head)->next; curr != (head); curr = curr->next)


/**
 * Iterates over the given list. This version is safe with respect
 * to removal of a list item.
 *
 * @param curr
 *	Name of the loop control variable pointing to the current list item.
 * @param next
 *	Name of the variable to hold the pointer to the next item.
 * @param head
 *	The head of the list to iterate over. Must not be NULL.
 */
#define list_for_each_safe(curr, next, head) \
	assert((head) != NULL); \
	for ( \
		curr = (head)->next, next = curr->next; \
		curr != (head); \
		curr = next, next = curr->next \
	)


/**
 * Iterates over the given list in reverse.
 *
 * @param curr
 *	Name of the loop control variable pointing to the current list item.
 * @param head
 *	The head of the list to iterate over. Must not be NULL.
 */
#define list_for_each_reverse(curr, head) \
	assert((head) != NULL); \
	for (curr = (head)->prev; curr != (head); curr = curr->prev)


/**
 * Iterates over the given list of typed entries.
 *
 * @param curr
 *	pointer to (type) to use for loop control
 * @param head
 *	The head of the list to iterate over. Must not be NULL.
 * @param member
 *	The name of the 'struct list' field in the containing structure.
 */
#define list_for_each_item(curr, head, member) \
	assert((head) != NULL); \
	for ( \
		curr = list_item ((head)->next, typeof (* curr), member); \
		(&curr->member) != (head); \
		curr = list_item (curr->member.next, typeof (* curr), member) \
	)


/**
 * Iterates over the given list of typed entries in reverse.
 *
 * @param curr
 *	pointer to (type) to use for loop control
 * @param head
 *	The head of the list to iterate over. Must not be NULL.
 * @param member
 *	The name of the 'struct list' field in the containing structure.
 */
#define list_for_each_item_reverse(curr, head, member) \
	assert((head) != NULL); \
	for ( \
		curr = list_item ((head)->prev, typeof (* curr), member); \
		(&curr->member) != (head); \
		curr = list_item (curr->member.prev, typeof (* curr), member) \
	)


/**
 * Destroys the given list by removing all items from the list and calling
 * a destroy function on each item. The head of the list is not considered
 * to be a valid item, so it is not destroyed.
 *
 * @param head
 *	The head of the list to destroy. Must not be NULL.
 * @param destroy
 *	The destructor function to call on each item. Must not be NULL.
 * @param data
 *	Additional data for the destructor callback.
 */
static inline void
list_destroy(struct list * head, list_destroy_fn destroy, void * data) {
	assert(head != NULL);
	while (! list_is_empty(head)) {
		// Unlink the head successor from the list and destroy it.
		destroy(list_remove_after(head), data);
	}
}


/**
 * Walks through the given list and calls a visitor function
 * on each list item.
 *
 * @param head
 *	The head of the list to walk. Must not be NULL.
 * @param visit
 *	The visitor function to call with each item. Must not be NULL.
 * @param data
 *	Additional data for the visitor callback.
 */
static inline void
list_walk(struct list * head, list_visit_fn visit, void * data) {
	assert(head != NULL);

	struct list * item;
	list_for_each(item, head) {
		visit(item, data);
	}
}


/**
 * Walks through the given list and calls a match function on each item.
 * If the match function returns true, the matching item is returned to the
 * caller and the traversal stops. Returns NULL when the traversal reaches
 * the end of the list without finding a match.
 *
 * @param head
 *	The head of the list to search. Must not be NULL.
 * @param match
 *	The matcher function to call with each item. Must not be NULL.
 * @param data:
 *	Additional data for the matcher callback.
 */
static inline struct list *
list_find(struct list * head, list_match_fn match, void * data) {
	assert(head != NULL);

	struct list * item;
	list_for_each(item, head) {
		if (match(item, data)) {
			return item;
		}
	}

	return NULL;
}


/**
 * Returns the number of items in the given list.
 *
 * @param head
 * 	The list for which to determine size. Must not be NULL.
 */
static inline size_t
list_size(struct list * head) {
	assert(head != NULL);

	size_t result = 0;

	struct list * item;
	list_for_each(item, head) {
		result++;
	}

	return result;
}

//

#ifdef __cplusplus
}
#endif

#endif /* _LIST_H_ */
