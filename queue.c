#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"


static inline void swap_pair(struct list_head **head);
static inline void swapptr(char **, char **);
struct list_head *merge_sorted_singly(struct list_head *, struct list_head *);
void final_merge(struct list_head *head,
                 struct list_head *l1,
                 struct list_head *l2);

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
struct list_head *q_new()
{
    struct list_head *newh =
        (struct list_head *) malloc(sizeof(struct list_head));
    if (!newh)
        return NULL;
    INIT_LIST_HEAD(newh);
    return newh;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (!l)  // prevent doubly free
        return;
    while (!list_empty(l)) {
        element_t *todel = container_of(l->next, element_t, list);
        list_del(l->next);
        q_release_element(todel);
    }
    free(l);
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *newele = (element_t *) malloc(sizeof(element_t));
    if (!newele)
        return false;
    INIT_LIST_HEAD(&newele->list);
    newele->value = NULL;
    size_t size = strlen(s);
    newele->value = (char *) malloc(
        sizeof(char) * (size + 1));  //+1 to include the null terminator
    if (!newele->value) {
        free(newele);
        return false;
    }
    strncpy(newele->value, s, sizeof(char) * (size + 1));
    list_add(&newele->list, head);
    return true;
}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head)
        return false;
    element_t *newele = (element_t *) malloc(sizeof(element_t));
    if (!newele)
        return false;
    INIT_LIST_HEAD(&newele->list);
    newele->value = NULL;
    size_t size = strlen(s);
    newele->value = (char *) malloc(
        sizeof(char) * (size + 1));  //+1 to include the null terminator
    if (!newele->value) {
        free(newele);
        return false;
    }
    strncpy(newele->value, s, sizeof(char) * (size + 1));
    list_add_tail(&newele->list, head);
    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 *
 * NOTE: "remove" is different from "delete"
 * The space used by the list element and the string should not be freed.
 * The only thing "remove" need to do is unlink it.
 *
 * REF:
 * https://english.stackexchange.com/questions/52508/difference-between-delete-and-remove
 */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *rm = container_of(head->next, element_t, list);
    list_del_init(head->next);
    if (sp) {
        size_t len = strnlen(rm->value, bufsize - 1);
        strncpy(sp, rm->value, len);
        sp[len] = '\0';
    }
    return rm;
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *rm = container_of(head->prev, element_t, list);
    list_del_init(head->prev);
    if (sp) {
        size_t len = strnlen(rm->value, bufsize - 1);
        strncpy(sp, rm->value, len);
        sp[len] = '\0';
    }
    return rm;
}

/*
 * WARN: This is for external usage, don't modify it
 * Attempt to release element.
 */
void q_release_element(element_t *e)
{
    free(e->value);
    free(e);
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    if (!head || list_empty(head))
        return 0;
    int size = 0;
    struct list_head *it;
    list_for_each (it, head)
        size++;
    return size;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return NULL if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;
    struct list_head *forwd = head->next, *backwd = head->prev;
    while (forwd != backwd) {
        forwd = forwd->next;
        if (forwd == backwd)
            break;
        backwd = backwd->prev;
    }
    element_t *todel = container_of(forwd, element_t, list);
    list_del(forwd);
    q_release_element(todel);
    return true;
}

/*
 * Delete all nodes that have duplicate string,
 * leaving only distinct strings from the original list.
 * Return true if successful.
 * Return false if list is NULL.
 *
 * Note: this function always be called after sorting, in other words,
 * list is guaranteed to be sorted in ascending order.
 */
bool q_delete_dup(struct list_head *head)
{
    if (!head || list_empty(head))
        return false;
    struct list_head *trashcan = q_new();
    if (!trashcan)
        return false;
    struct list_head *it;
    list_for_each (it, head) {  // go through entire list
        char *curr = container_of(it, element_t, list)->value;
        /* if this entry is not duplicate */
        if (it->next == head ||
            strcmp(curr, container_of(it->next, element_t, list)->value))
            continue;
        /* if duplicate */
        it = it->prev;
        do {
            list_move(it->next, trashcan);
        } while (it->next != head &&
                 !strcmp(curr, container_of(it->next, element_t, list)->value));
    }
    q_free(trashcan);
    return true;
}

/*
 * Attempt to swap every two adjacent nodes.
 */
void q_swap(struct list_head *head)
{
    if (!head)
        return;
    for (struct list_head **it = &head->next;
         *it != head && (*it)->next != head; it = &(*it)->next->next)
        swap_pair(it);
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(struct list_head *head)
{
    if (!head || list_empty(head))
        return;
    for (struct list_head *it = head->next; it != head; it = it->prev)
        swapptr((char **) &it->next, (char **) &it->prev);
    swapptr((char **) &head->next, (char **) &head->prev);
}


/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void q_sort(struct list_head *head)
{
    if (!head || list_empty(head) || list_is_singular(head))
        return;
    const size_t size = q_size(head);
    struct list_head *lists[size];
    struct list_head *it = head->next;
    for (size_t i = 0; i < size; i++) {
        lists[i] = it;
        it = it->next;
        lists[i]->next = NULL;
    }
    for (size_t n = size; n > 2; n = n / 2 + (n & 1)) {
        for (size_t i = 0, j = n - 1; i < j; i++, j--)
            lists[i] = merge_sorted_singly(lists[i], lists[j]);
    }
    final_merge(head, lists[0], lists[1]);
}

/* swap next two entries starting from *head */
static inline void swap_pair(struct list_head **head)
{
    struct list_head *first = *head, *second = first->next;
    second->next->prev = first;
    first->next = second->next;
    second->next = first;
    second->prev = first->prev;
    first->prev = second;
    *head = second;
}

static inline void swapptr(char **a, char **b)
{
    (*a) = (char *) ((__intptr_t)(*a) ^ (__intptr_t)(*b));
    (*b) = (char *) ((__intptr_t)(*b) ^ (__intptr_t)(*a));
    (*a) = (char *) ((__intptr_t)(*a) ^ (__intptr_t)(*b));
}

struct list_head *merge_sorted_singly(struct list_head *l1,
                                      struct list_head *l2)
{
    struct list_head *ans = NULL, **tail = &ans;
    while (l1 && l2) {
        struct list_head **small =
            strcmp(container_of(l1, element_t, list)->value,
                   container_of(l2, element_t, list)->value) <= 0
                ? &l1
                : &l2;
        *tail = *small;
        tail = &(*small)->next;
        (*small) = (*small)->next;
    }
    *tail = l1 ? l1 : l2;
    return ans;
}

/* merge to doubly-linked list */
void final_merge(struct list_head *head,
                 struct list_head *l1,
                 struct list_head *l2)
{
    struct list_head *it = head;
    while (l1 && l2) {
        struct list_head **small =
            strcmp(container_of(l1, element_t, list)->value,
                   container_of(l2, element_t, list)->value) <= 0
                ? &l1
                : &l2;
        (*small)->prev = it;
        it->next = *small;
        it = it->next;
        (*small) = (*small)->next;
    }
    for (it->next = l1 ? l1 : l2; it->next; it = it->next)
        it->next->prev = it;
    it->next = head;
    head->prev = it;
}
