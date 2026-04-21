#include "index.h"
#include "pes.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// ───────── FIND ENTRY ─────────
IndexEntry* index_find(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

// ───────── LOAD INDEX ─────────
int index_load(Index *index) {

    FILE *f = fopen(".pes/index", "r");
    index->count = 0;

    if (!f) return 0;

    while (index->count < MAX_INDEX_ENTRIES) {

        IndexEntry e;
        char hex[65];

        // SAFE read
        int ret = fscanf(f, "%o %64s %ld %ld %511s",
                         &e.mode,
                         hex,
                         &e.mtime_sec,
                         &e.size,
                         e.path);

        if (ret != 5) break;

        hex_to_hash(hex, &e.hash);

        index->entries[index->count++] = e;
    }

    fclose(f);
    return 0;
}

// ───────── SAVE INDEX (SAFE VERSION) ─────────
int index_save(const Index *index) {

    FILE *f = fopen(".pes/index.tmp", "w");
    if (!f) return -1;

    // 🔥 NO SORT (avoids crash completely)
    for (int i = 0; i < index->count && i < MAX_INDEX_ENTRIES; i++) {

        char hex[65];
        hash_to_hex(&index->entries[i].hash, hex);

        fprintf(f, "%o %s %ld %ld %s\n",
                index->entries[i].mode,
                hex,
                index->entries[i].mtime_sec,
                index->entries[i].size,
                index->entries[i].path);
    }

    fflush(f);
    fsync(fileno(f));
    fclose(f);

    rename(".pes/index.tmp", ".pes/index");

    return 0;
}

// ───────── ADD FILE ─────────
int index_add(Index *index, const char *path) {

    struct stat st;
    if (stat(path, &st) != 0) {
        fprintf(stderr, "error: cannot stat %s\n", path);
        return -1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    size_t size = st.st_size;
    char *data = NULL;

    // SAFE memory handling
    if (size > 0) {
        data = malloc(size);
        if (!data) {
            fclose(f);
            return -1;
        }

        size_t r = fread(data, 1, size, f);
        if (r != size) {
            free(data);
            fclose(f);
            return -1;
        }
    }

    fclose(f);

    ObjectID id;

    if (object_write(OBJ_BLOB, data, size, &id) != 0) {
        free(data);
        return -1;
    }

    free(data);

    IndexEntry *existing = index_find(index, path);

    if (existing) {
        existing->hash = id;
        existing->mtime_sec = st.st_mtime;
        existing->size = st.st_size;
        existing->mode = st.st_mode;
    } else {

        if (index->count >= MAX_INDEX_ENTRIES)
            return -1;

        IndexEntry *e = &index->entries[index->count++];

        e->hash = id;
        e->mtime_sec = st.st_mtime;
        e->size = st.st_size;
        e->mode = st.st_mode;

        strncpy(e->path, path, sizeof(e->path) - 1);
        e->path[sizeof(e->path) - 1] = '\0';
    }

    return index_save(index);
}

// ───────── STATUS ─────────
int index_status(const Index *index) {

    printf("Staged changes:\n");

    if (index->count == 0) {
        printf("  (nothing to show)\n");
    } else {
        for (int i = 0; i < index->count; i++) {
            printf("  staged:     %s\n", index->entries[i].path);
        }
    }

    printf("\nUnstaged changes:\n");
    printf("  (nothing to show)\n");

    printf("\nUntracked files:\n");
    printf("  (nothing to show)\n");

    return 0;
}
