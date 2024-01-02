#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>

#define TOP_COUNT 10
#define DEFAULT_SHIFT 10
#define MAX_SHIFT (((int)sizeof(size_t) * 8) - 1)

static void die(const char *message)
{
    fprintf(stderr, "%s\n", message);
    exit(1);
}

static void *xmalloc(size_t size)
{
    void *ptr;

    ptr = malloc(size);
    if (ptr == NULL) {
        die("malloc: out of memory.");
    }

    return ptr;
}

static void *xcalloc(size_t nmemb, size_t size)
{
    void *ptr;

    ptr = calloc(nmemb, size);
    if (ptr == NULL) {
        die("calloc: out of memory.");
    }

    return ptr;
}

static void *xrealloc(void *ptr, size_t size)
{
    void *new_ptr;

    new_ptr = realloc(ptr, size);
    if (new_ptr == NULL) {
        die("realloc: out of memory.");
    }

    return new_ptr;
}

#define FNV_OFFSET_BASIS ((size_t)2166136261UL)
#define FNV_PRIME ((size_t)16777619UL)

static size_t fnv1(const char *str)
{
    size_t hash = FNV_OFFSET_BASIS;

    while (str != NULL && *str != '\0') {
        hash = (hash * FNV_PRIME) ^ *str++;
    }

    return hash;
}

#define hashfn(s) fnv1(s)

struct hash_item {
    const char *key;
    size_t value;
    struct hash_item *next;
};

static struct hash_item *item_new(const char *key)
{
    struct hash_item *item;

    item = xmalloc(sizeof(*item));
    item->key = key;
    item->value = 1;
    item->next = NULL;

    return item;
}

static void item_free(struct hash_item *item)
{
    free((void *) item->key);
    free(item);
}

struct hash_table {
    struct hash_item **items;
    size_t item_count;
    size_t size;
    int shift;
};

static struct hash_table *hash_table_new(int shift)
{
    struct hash_table *hash_table;

    if (shift > MAX_SHIFT) {
        die("hash_table_new: shift too large.");
    }

    hash_table = xmalloc(sizeof(*hash_table));
    hash_table->items = xcalloc(1 << shift, sizeof(*hash_table->items));
    hash_table->shift = shift;
    hash_table->size = 1 << shift;
    hash_table->item_count = 0;

    return hash_table;
}

static void hash_table_free(struct hash_table *hash_table)
{
    free(hash_table->items);
    free(hash_table);
}

static void hash_table_add(struct hash_table *hash_table,
                           struct hash_item *item)
{
    size_t hash;

    hash = hashfn(item->key) & (hash_table->size - 1);
    item->next = hash_table->items[hash];
    hash_table->items[hash] = item;
    ++hash_table->item_count;
}

static struct hash_item *hash_table_get(struct hash_table *hash_table,
                                        const char *key)
{
    struct hash_item *item = NULL;
    size_t h;

    h = hashfn(key) & (hash_table->size - 1);
    item = hash_table->items[h];

    while (item != NULL && strcmp(item->key, key) != 0) {
        item = item->next;
    }

    return item;
}

static void hash_table_foreach(struct hash_table *hash_table,
                               void (*function)(struct hash_item *))
{
    size_t i;

    for (i = 0; i < hash_table->size; ++i) {
        struct hash_item *item = hash_table->items[i];

        while (item != NULL) {
            struct hash_item *next_item = item->next;

            function(item);
            item = next_item;
        }
    }
}

static void hash_table_grow(struct hash_table **hash_table_ptr_ptr)
{
    struct hash_table *hash_table = *hash_table_ptr_ptr;
    struct hash_table *new_hash_table;
    size_t i;

    if ((hash_table->shift + 1) > MAX_SHIFT) {
        die("hash_table_grow: no room.");
    }
#if STATS
    printf("--> grow hash_table to %d entries.\n",
           1 << (hash_table->shift + 1));
#endif

    new_hash_table = hash_table_new(hash_table->shift + 1);
    for (i = 0; i < hash_table->size; ++i) {
        struct hash_item *item = hash_table->items[i];

        while (item != NULL) {
            struct hash_item *next_item = item->next;

            hash_table_add(new_hash_table, item);
            item = next_item;
        }
    }

    hash_table_free(hash_table);

    *hash_table_ptr_ptr = new_hash_table;
}

#if STATS
static void hash_table_statistics(struct hash_table *hash_table)
{
    printf("Hash table size is %lu kb.\n",
           (hash_table->size * sizeof(hash_table->items[0])) / 1024);
}
#endif

static char *read_word(FILE *f)
{
    size_t len = 0, maxlen = 0;
    char *word = NULL;
    int ch;

    while (((ch = fgetc(f)) != EOF) && isalpha(ch)) {
        ch = tolower(ch);

        if (len >= maxlen) {
            word = xrealloc(word, maxlen + 32);
            maxlen += 32;
        }

        word[len++] = ch;
    }

    if (word == NULL && ch == EOF) {
        return NULL;
    }

    word = xrealloc(word, len + 1);
    word[len] = '\0';

    return word;
}

static void add_word(struct hash_table **hash_table_ptr_ptr, const char *word)
{
    struct hash_table *hash_table = *hash_table_ptr_ptr;
    struct hash_item *item;

    item = item_new(word);

    /* Grow table if the load is too high. */
    if (hash_table->item_count > (hash_table->size / 4 * 3)) {
        hash_table_grow(&hash_table);
    }

    hash_table_add(hash_table, item);

    /* Propagate any changes back to the caller. */
    *hash_table_ptr_ptr = hash_table;
}

static int by_count_descending(const void *item_a, const void *item_b)
{
    const struct hash_item *a = *((const struct hash_item **) item_a);
    const struct hash_item *b = *((const struct hash_item **) item_b);

    if (a->value < b->value) {
        return 1;
    } else if (b->value < a->value) {
        return -1;
    }

    return 0;
}

int main(void)
{
    struct hash_table *wordcounts = NULL;
    struct hash_item **sorted = NULL;
    char *word = NULL;
    size_t item_count;
    size_t top_count;
    size_t i;

    wordcounts = hash_table_new(DEFAULT_SHIFT);

    while ((word = read_word(stdin)) != NULL) {
        struct hash_item *item = NULL;

        if (word[0] == '\0') {
            free(word);
            continue;
        }

        item = hash_table_get(wordcounts, word);

        /* If it exists increase count, otherwise add it. */
        if (item != NULL) {
            free(word);
            ++item->value;
        } else {
            add_word(&wordcounts, word);
        }
    }

#if STATS
    hash_table_statistics(wordcounts);
#endif

    /* Sort the hash_table into the sorted array. */
    sorted = xcalloc(wordcounts->item_count, sizeof(*sorted));

    for (i = item_count = 0; i < wordcounts->size; ++i) {
        struct hash_item *item = wordcounts->items[i];

        while (item != NULL) {
            sorted[item_count++] = item;
            item = item->next;
        }
    }

    assert(item_count == wordcounts->item_count);
    qsort(sorted, item_count, sizeof(*sorted), by_count_descending);

    /* And print the top-N items. */
    top_count = wordcounts->item_count < TOP_COUNT ? wordcounts->item_count :
        TOP_COUNT;

    for (i = 0; i < top_count; ++i) {
        const char *word = sorted[i]->key;
        const size_t count = sorted[i]->value;

        printf("%lu\t%s\n", (unsigned long) count, word);
    }

    /* Cleanup */
    free(sorted);
    hash_table_foreach(wordcounts, item_free);
    hash_table_free(wordcounts);

    return 0;
}
