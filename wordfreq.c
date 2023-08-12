#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <assert.h>

#define NTOP 10
#define DEFAULT_SHIFT 10
#define MAX_SHIFT (((int)sizeof(size_t) * 8) - 1)

static void
die(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

static void *
xmalloc(size_t size)
{
	void *p;

	p = malloc(size);
	if (p == NULL)
		die("malloc: out of memory.");

	return p;
}

static void *
xcalloc(size_t nmemb, size_t size)
{
	void *p;

	p = calloc(nmemb, size);
	if (p == NULL)
		die("calloc: out of memory.");

	return p;
}

static void *
xrealloc(void *p, size_t size)
{
	void *q;

	q = realloc(p, size);
	if (q == NULL)
		die("realloc: out of memory.");

	return q;
}

#define FNV_OFFSET_BASIS ((size_t)2166136261UL)
#define FNV_PRIME ((size_t)16777619UL)

static size_t
fnv1(const char *s)
{
	size_t h = FNV_OFFSET_BASIS;

	while (s != NULL && *s != '\0')
		h = (h * FNV_PRIME) ^ *s++;

	return h;
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
	size_t h;

	h = hash_function(item->key) & (ht->size - 1);
	item->next = ht->items[h];
	ht->items[h] = item;
	++ht->nitems;
}

static struct hash_item *
hash_table_get(struct hash_table *ht, const char *key)
{
	struct hash_item *item = NULL;
	size_t h;

	h = hash_function(key) & (ht->size - 1);
	item = ht->items[h];

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
			struct hash_item *next = item->next;

			fn(item);
			item = next;
		}
	}
}

static void
hash_table_grow(struct hash_table **htpp)
{
	struct hash_table *ht = *htpp;
	struct hash_table *newht;
	size_t i;

	if ((ht->shift + 1) > MAX_SHIFT)
		die("hash_table_grow: no room.");

#if STATS
	printf("--> grow hash_table to %d entries.\n", 1 << (ht->shift + 1));
#endif

	newht = hash_table_new(ht->shift + 1);
	for (i = 0; i < ht->size; ++i) {
		struct hash_item *item = ht->items[i];

		while (item != NULL) {
			struct hash_item *next = item->next;

			hash_table_add(newht, item);
			item = next;
		}
	}

	hash_table_free(ht);

	*htpp = newht;
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

	if (word == NULL && ch == EOF)
		return NULL;

	word = xrealloc(word, len + 1);
	word[len] = '\0';

	return word;
}

static void
add_word(struct hash_table **htpp, const char *word)
{
	struct hash_table *ht = *htpp;
	struct hash_item *item;

	item = item_new(word);

	/* Grow table if the load is too high. */
	if (ht->nitems > (ht->size / 4 * 3))
		hash_table_grow(&ht);

	hash_table_add(ht, item);

	/* Propagate any changes back to the caller. */
	*htpp = ht;
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

int
main(void)
{
	struct hash_table *wordcounts = NULL;
	struct hash_item **sorted = NULL;
	char *word = NULL;
	size_t nitems;
	size_t ntop;
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
		} else
			add_word(&wordcounts, word);
	}

#if STATS
	hash_table_statistics(wordcounts);
#endif

	/* Sort the hash_table into the sorted array. */
	sorted = xcalloc(wordcounts->nitems, sizeof(*sorted));

	for (i = nitems = 0; i < wordcounts->size; ++i) {
		struct hash_item *item = wordcounts->items[i];

		while (item != NULL) {
			sorted[nitems++] = item;
			item = item->next;
		}
	}

	assert(nitems == wordcounts->nitems);
	qsort(sorted, nitems, sizeof(*sorted), by_count_descending);

	/* And print the top-N items. */
	ntop = wordcounts->nitems < NTOP ? wordcounts->nitems : NTOP;

	for (i = 0; i < ntop; ++i) {
		const char *word = sorted[i]->key;
		const size_t count = sorted[i]->value;

		printf("%lu\t%s\n", (unsigned long)count, word);
	}

	/* Cleanup */
	free(sorted);
	hash_table_foreach(wordcounts, item_free);
	hash_table_free(wordcounts);

	return 0;
}
