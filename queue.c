#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

#define structinit(type, name) type *name = (type *) malloc(sizeof(type));
#define getvalue(node) container_of(node, element_t, list)->value

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
    struct list_head *plh = malloc(sizeof(struct list_head));
    if (plh)
        INIT_LIST_HEAD(plh);
    return plh;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (!l)
        return;
    while (!list_empty(l)) {
        element_t *tmp = container_of(l->next, element_t, list);
        list_del(l->next);
        q_release_element(tmp);
    }
    free(l);
}

element_t *new_ele(char *s)
{
    structinit(element_t, new);
    if (!new)
        return NULL;
    new->value = malloc(sizeof(char) * (strlen(s) + 1));
    if (!new->value) {
        free(new);
        return NULL;
    }
    strncpy(new->value, s, strlen(s));
    new->value[strlen(s)] = 0;
    return new;
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
    if (!(head && s))
        return false;
    element_t *new = new_ele(s);
    if (!new)
        return false;
    list_add(&new->list, head);
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
    if (!(head && s))
        return false;
    element_t *new = new_ele(s);
    if (!new)
        return false;
    list_add_tail(&new->list, head);
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
    element_t *tmp = container_of(head->next, element_t, list);
    list_del_init(head->next);
    if (sp) {
        size_t len = strnlen(tmp->value, bufsize - 1);
        strncpy(sp, tmp->value, len);
        sp[len] = 0;
    }
    return tmp;
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *tmp = container_of(head->prev, element_t, list);
    list_del_init(head->prev);
    if (sp) {
        size_t len = strnlen(tmp->value, bufsize - 1);
        strncpy(sp, tmp->value, len);
        sp[len] = 0;
    }
    return tmp;
}


/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *li;

    list_for_each (li, head)
        len++;
    return len;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return true if successful.
 * Return false if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;
    struct list_head *h = head->next;
    struct list_head *t = head->prev;
    while (!(h == t || h->next == t)) {
        h = h->next;
        t = t->prev;
    }
    list_del(t);
    q_release_element(list_entry(t, element_t, list));
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
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head)
        return false;

    LIST_HEAD(dup_l);
    bool prev = false;
    element_t *ptr = list_entry(head->next, element_t, list);
    element_t *next = ptr;

    for (bool is_dup = false; next->list.next != head; ptr = next) {
        next = list_entry(ptr->list.next, element_t, list);
        is_dup = !strncmp(ptr->value, next->value, strlen(ptr->value));
        if (is_dup || prev)
            list_move(&ptr->list, &dup_l);
        prev = is_dup;
    }

    if (prev)
        list_move(&ptr->list, &dup_l);

    list_for_each_entry_safe (ptr, next, &dup_l, list) {
        free(ptr->value);
        free(ptr);
    }
    return true;
}

/*
 * Attempt to swap every two adjacent nodes.
 */
void q_swap(struct list_head *head)
{
    // https://leetcode.com/problems/swap-nodes-in-pairs/
    if (!head || (head->next == head->prev))
        return;
    struct list_head *left = head->next;
    struct list_head *right = left->next;

    do {
        left->prev->next = right;
        right->next->prev = left;

        left->next = right->next;
        right->prev = left->prev;

        left->prev = right;
        right->next = left;

        left = left->next;
        right = left->next;

    } while (left != head && right != head);
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
    struct list_head *p = head;
    struct list_head *tmp;

    do {
        tmp = p->next;
        p->next = p->prev;
        p->prev = tmp;
        p = p->prev;
    } while (p != head);
}

struct list_head *split_list(struct list_head *h)
{
    struct list_head *hare = h->next, *tortoise = h;
    for (; hare; hare = hare->next ? hare->next->next : hare->next,
                 tortoise = tortoise->next)
        ;
    tortoise->prev->next = NULL;
    return tortoise;
}

struct list_head *merge_two_sorted(struct list_head *l1, struct list_head *l2)
{
    struct list_head *ret = NULL;
    struct list_head **tail = &ret;
    while (l1 && l2) {
        struct list_head **small =
            strcmp(getvalue(l1), getvalue(l2)) <= 0 ? &l1 : &l2;
        *tail = *small;
        tail = &(*small)->next;
        (*small) = (*small)->next;
    }
    *tail = l1 ? l1 : l2;
    return ret;
}

void merge_sort(struct list_head **head)
{
    if (!(*head && (*head)->next))
        return;
    struct list_head *mid = split_list(*head);
    merge_sort(head);
    merge_sort(&mid);
    *head = merge_two_sorted(*head, mid);
}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void q_sort(struct list_head *head)
{
    if (!head || list_is_singular(head) || list_empty(head))
        return;

    head->prev->next = NULL;
    merge_sort(&head->next);
    struct list_head *prev = head;
    for (struct list_head *it = head->next; it; it = it->next) {
        it->prev = prev;
        prev = it;
    }
    head->prev = prev;
    prev->next = head;
}
