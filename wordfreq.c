#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>

#define NTOP 10
#define DEFAULT_SHIFT 10
#define MAX_SHIFT (((int)sizeof(size_t) * 8) - 1)

static void
die(const char *message)
{
	fprintf(stderr, "%s\n", message);
	exit(1);
}

static void *
xmalloc(size_t size)
{
	void *ptr;

	ptr = malloc(size);
	if (ptr == NULL)
		die("malloc: out of memory.");

	return ptr;
}

static void *
xcalloc(size_t nmemb, size_t size)
{
	void *ptr;

	ptr = calloc(nmemb, size);
	if (ptr == NULL)
		die("calloc: out of memory.");

	return ptr;
}

static void *
xrealloc(void *ptr, size_t size)
{
	void *new_ptr;

	new_ptr = realloc(ptr, size);
	if (new_ptr == NULL)
		die("realloc: out of memory.");

	return new_ptr;
}

#define FNV_OFFSET_BASIS ((size_t)2166136261UL)
#define FNV_PRIME ((size_t)16777619UL)

static size_t
fnv1(const char *str)
{
	size_t hash = FNV_OFFSET_BASIS;

	while (str != NULL && *str != '\0')
		hash = (hash * FNV_PRIME) ^ *str++;

	return hash;
}

#define hash_function(s) fnv1(s)

struct hash_item {
	const char *key;
	size_t value;
	struct hash_item *next;
};

static struct hash_item *
item_new(const char *key)
{
	struct hash_item *item;

	item = xmalloc(sizeof(*item));
	item->key = key;
	item->value = 1;
	item->next = NULL;

	return item;
}

static void
item_free(struct hash_item *item)
{
	free((void *)item->key);
	free(item);
}

struct hash_table {
	struct hash_item **items;
	size_t nitems;
	size_t size;
	int shift;
};

static struct hash_table *
hash_table_new(int shift)
{
	struct hash_table *ht;

	if (shift > MAX_SHIFT)
		die("hash_table_new: shift too large.");

	ht = xmalloc(sizeof(*ht));
	ht->items = xcalloc(1 << shift, sizeof(*ht->items));
	ht->shift = shift;
	ht->size = 1 << shift;
	ht->nitems = 0;

	return ht;
}

static void
hash_table_free(struct hash_table *ht)
{
	free(ht->items);
	free(ht);
}

static void
hash_table_add(struct hash_table *ht, struct hash_item *item)
{
	size_t hash;

	hash = hash_function(item->key) & (ht->size - 1);
	item->next = ht->items[hash];
	ht->items[hash] = item;
	++ht->nitems;
}

static struct hash_item *
hash_table_get(struct hash_table *ht, const char *key)
{
	struct hash_item *item = NULL;
	size_t hash;

	hash = hash_function(key) & (ht->size - 1);
	item = ht->items[hash];

	while (item != NULL && strcmp(item->key, key) != 0)
		item = item->next;

	return item;
}

static void
hash_table_foreach(struct hash_table *ht, void (*fn)(struct hash_item *))
{
	size_t i;

	for (i = 0; i < ht->size; ++i) {
		struct hash_item *item = ht->items[i];

		while (item != NULL) {
			struct hash_item *next_item = item->next;

			fn(item);
			item = next_item;
		}
	}
}

static void
hash_table_grow(struct hash_table **htpp)
{
	struct hash_table *ht = *htpp;
	struct hash_table *new_ht;
	size_t i;

	if ((ht->shift + 1) > MAX_SHIFT)
		die("hash_table_grow: no room.");
#if STATS
	printf("--> grow hash_table to %d entries.\n", 1 << (ht->shift + 1));
#endif

	new_ht = hash_table_new(ht->shift + 1);
	for (i = 0; i < ht->size; ++i) {
		struct hash_item *item = ht->items[i];

		while (item != NULL) {
			struct hash_item *next_item = item->next;

			hash_table_add(new_ht, item);
			item = next_item;
		}
	}

	hash_table_free(ht);

	*htpp = new_ht;
}

#if STATS
static void
hash_table_statistics(struct hash_table *ht)
{
	printf("Hash table size is %lu kb.\n",
	    (ht->size * sizeof(ht->items[0])) / 1024);
}
#endif

static char *
read_word(FILE *f)
{
	size_t len = 0, cap = 0;
	char *word = NULL;
	int ch;

	while ((ch = fgetc(f)) != EOF && isalpha(ch)) {
		ch = tolower(ch);

		if (len >= cap) {
			word = xrealloc(word, cap + 32);
			cap += 32;
		}

		word[len++] = ch;
	}

	if (word == NULL && ch == EOF)
		return NULL;

	word = xrealloc(word, len + 1);
	word[len] = '\0';

	return word;
}

static int
by_count_descending(const void *item_a, const void *item_b)
{
	const struct hash_item *a = *((const struct hash_item **)item_a);
	const struct hash_item *b = *((const struct hash_item **)item_b);

	if (a->value < b->value)
		return 1;
	else if (b->value < a->value)
		return -1;

	return 0;
}

struct hash_item **
sort_hash_table(struct hash_table *ht, int (*cmp)(const void *, const void *))
{
	struct hash_item **sorted;
	size_t i, nitems;

	sorted = xcalloc(ht->nitems, sizeof(*sorted));

	for (i = nitems = 0; i < ht->size; ++i) {
		struct hash_item *item = ht->items[i];

		while (item != NULL) {
			sorted[nitems++] = item;
			item = item->next;
		}
	}

	qsort(sorted, nitems, sizeof(*sorted), cmp);

	return sorted;
}

void
show_topn(struct hash_table *ht, size_t n)
{
	struct hash_item **sorted;
	size_t i;

	sorted = sort_hash_table(ht, by_count_descending);

	if (ht->nitems < n)
		n = ht->nitems;

	for (i = 0; i < n; ++i) {
		const char *word = sorted[i]->key;
		const unsigned long count = sorted[i]->value;

		printf("%lu\t%s\n", count, word);
	}

	free(sorted);
}

static void
add_word(struct hash_table **htpp, const char *word)
{
	struct hash_table *ht = *htpp;
	struct hash_item *item;

	item = hash_table_get(ht, word);
	if (item != NULL)
		++item->value;
	else {
		/* Grow table if the load is too high. */
		if (ht->nitems > (ht->size / 4 * 3))
			hash_table_grow(&ht);

		hash_table_add(ht, item_new(word));

		/* Propagate changes back to the caller. */
		*htpp = ht;
	}
}

int
main(void)
{
	struct hash_table *wordcounts;
	char *word;

	wordcounts = hash_table_new(DEFAULT_SHIFT);

	while ((word = read_word(stdin)) != NULL) {
		if (*word == '\0') {
			free(word);
			continue;
		}

		add_word(&wordcounts, word);
	}

#if STATS
	hash_table_statistics(wordcounts);
#endif

	show_topn(wordcounts, NTOP);

	hash_table_foreach(wordcounts, item_free);
	hash_table_free(wordcounts);

	return 0;
}
