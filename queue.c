#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

#define structinit(type, name) type *name = (type *) malloc(sizeof(type));
#define getvalue(node) container_of(node, element_t, list)->value

#ifdef __GNUC__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) (x)
#define unlikely(x) (x)
#endif

typedef int
    __attribute__((nonnull(2, 3))) (*list_cmp_func_t)(void *,
                                                      const struct list_head *,
                                                      const struct list_head *);

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
/*
struct list_head *split_list(struct list_head *h)
{
    struct list_head *hare = h->next, *tortoise = h;
    for (; hare; hare = hare->next ? hare->next->next : hare->next,
                 tortoise = tortoise->next)
        ;
    tortoise->prev->next = NULL;
    return tortoise;
}

struct list_head *merge_two_lists(struct list_head *l1, struct list_head *l2)
{
    struct list_head *p1 = l1, *p2 = l2;
    for (; p1->next; p1 = p1->next)
        ;
    for (; p2->next; p2 = p2->next)
        ;
    if (strcmp(getvalue(p1), getvalue(l2)) == 0) {
        p1->next = l2;
        l2->prev = p1;
        return l1;
    } else if (strcmp(getvalue(l1), getvalue(p2)) == 0) {
        p2->next = l1;
        l1->prev = p2;
        return l2;
    }
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
    *head = merge_two_lists(*head, mid);
}
__attribute__((unused)) void q_sort_topdown(struct list_head *head)
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
*/
__attribute__((nonnull(2, 3, 4))) static struct list_head *
merge(void *priv, list_cmp_func_t cmp, struct list_head *a, struct list_head *b)
{
    struct list_head *head = NULL, **tail = &head;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            *tail = a;
            tail = &a->next;
            a = a->next;
            if (!a) {
                *tail = b;
                break;
            }
        } else {
            *tail = b;
            tail = &b->next;
            b = b->next;
            if (!b) {
                *tail = a;
                break;
            }
        }
    }
    return head;
}

__attribute__((nonnull(2, 3, 4, 5))) static void merge_final(
    void *priv,
    list_cmp_func_t cmp,
    struct list_head *head,
    struct list_head *a,
    struct list_head *b)
{
    struct list_head *tail = head;
    uint8_t count = 0;

    for (;;) {
        /* if equal, take 'a' -- important for sort stability */
        if (cmp(priv, a, b) <= 0) {
            tail->next = a;
            a->prev = tail;
            tail = a;
            a = a->next;
            if (!a)
                break;
        } else {
            tail->next = b;
            b->prev = tail;
            tail = b;
            b = b->next;
            if (!b) {
                b = a;
                break;
            }
        }
    }

    /* Finish linking remainder of list b on to tail */
    tail->next = b;
    do {
        /*
         * If the merge is highly unbalanced (e.g. the input is
         * already sorted), this loop may run many iterations.
         * Continue callbacks to the client even though no
         * element comparison is needed, so the client's cmp()
         * routine can invoke cond_resched() periodically.
         */
        if (unlikely(!++count))
            cmp(priv, b, b);
        b->prev = tail;
        tail = b;
        b = b->next;
    } while (b);

    /* And the final links to make a circular doubly-linked list */
    tail->next = head;
    head->prev = tail;
}
__attribute__((nonnull(2, 3))) void list_sort(void *priv,
                                              struct list_head *head,
                                              list_cmp_func_t cmp)
{
    struct list_head *list = head->next, *pending = NULL;
    size_t count = 0; /* Count of pending */

    if (list == head->prev) /* Zero or one elements */
        return;

    /* Convert to a null-terminated singly-linked list. */
    head->prev->next = NULL;

    /*
     * Data structure invariants:
     * - All lists are singly linked and null-terminated; prev
     *   pointers are not maintained.
     * - pending is a prev-linked "list of lists" of sorted
     *   sublists awaiting further merging.
     * - Each of the sorted sublists is power-of-two in size.
     * - Sublists are sorted by size and age, smallest & newest at front.
     * - There are zero to two sublists of each size.
     * - A pair of pending sublists are merged as soon as the number
     *   of following pending elements equals their size (i.e.
     *   each time count reaches an odd multiple of that size).
     *   That ensures each later final merge will be at worst 2:1.
     * - Each round consists of:
     *   - Merging the two sublists selected by the highest bit
     *     which flips when count is incremented, and
     *   - Adding an element from the input as a size-1 sublist.
     */
    do {
        size_t bits;
        struct list_head **tail = &pending;

        /* Find the least-significant clear bit in count */
        for (bits = count; bits & 1; bits >>= 1)
            tail = &(*tail)->prev;
        /* Do the indicated merge */
        if (likely(bits)) {
            struct list_head *a = *tail, *b = a->prev;

            a = merge(priv, cmp, b, a);
            /* Install the merged result in place of the inputs */
            a->prev = b->prev;
            *tail = a;
        }

        /* Move one element from input list to pending */
        list->prev = pending;
        pending = list;
        list = list->next;
        pending->next = NULL;
        count++;
    } while (list);

    /* End of input; merge together all the pending lists. */
    list = pending;
    pending = pending->prev;
    for (;;) {
        struct list_head *next = pending->prev;

        if (!next)
            break;
        list = merge(priv, cmp, pending, list);
        pending = next;
    }
    /* The final merge, rebuilding prev links */
    merge_final(priv, cmp, head, pending, list);
}

int cmpfunc(void *param, const struct list_head *a, const struct list_head *b)
{
    return (strcmp(getvalue(a), getvalue(b)) >= 0) ? 1 : -1;
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
    list_sort(NULL, head, cmpfunc);
    return;
}
