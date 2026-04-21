#include "tree.h"
#include "index.h"
#include "pes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// SORT
static int cmp(const void *a, const void *b) {
    return strcmp(((TreeEntry *)a)->name,
                  ((TreeEntry *)b)->name);
}

// SERIALIZE
int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {

    size_t max = tree->count * 300;
    uint8_t *buf = malloc(max);
    if (!buf) return -1;

    Tree tmp = *tree;
    qsort(tmp.entries, tmp.count, sizeof(TreeEntry), cmp);

    size_t off = 0;

    for (int i = 0; i < tmp.count; i++) {

        TreeEntry *e = &tmp.entries[i];

        int n = sprintf((char *)buf + off, "%o %s", e->mode, e->name);
        off += n + 1;

        memcpy(buf + off, e->hash.hash, HASH_SIZE);
        off += HASH_SIZE;
    }

    *data_out = buf;
    *len_out = off;
    return 0;
}

// BUILD TREE FROM INDEX
int tree_from_index(ObjectID *id_out) {

    // 🔴 create a very simple empty tree
    Tree tree;
    memset(&tree, 0, sizeof(Tree));

    void *data = NULL;
    size_t len = 0;

    // serialize empty tree
    if (tree_serialize(&tree, &data, &len) != 0)
        return -1;

    if (object_write(OBJ_TREE, data, len, id_out) != 0) {
        free(data);
        return -1;
    }

    free(data);
    return 0;
}
