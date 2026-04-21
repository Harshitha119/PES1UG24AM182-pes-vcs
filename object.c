// object.c — Content-addressable object store

#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/sha.h>

// ─────────────────────────────────────────────
// Compute SHA-256 hash
// ─────────────────────────────────────────────
void compute_hash(const void *data, size_t len, ObjectID *id_out) {
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, len);
    SHA256_Final(id_out->hash, &ctx);
}

// ─────────────────────────────────────────────
// Convert hash → hex string
// ─────────────────────────────────────────────
void hash_to_hex(const ObjectID *id, char *hex_out) {
    for (int i = 0; i < HASH_SIZE; i++)
        sprintf(hex_out + i * 2, "%02x", id->hash[i]);
    hex_out[64] = '\0';
}

// ─────────────────────────────────────────────
// Convert hex → hash
// ─────────────────────────────────────────────
int hex_to_hash(const char *hex, ObjectID *id_out) {
    for (int i = 0; i < HASH_SIZE; i++) {
        sscanf(hex + i * 2, "%2hhx", &id_out->hash[i]);
    }
    return 0;
}

// ─────────────────────────────────────────────
// Get object file path
// ─────────────────────────────────────────────
void object_path(const ObjectID *id, char *path_out) {
    char hex[65];
    hash_to_hex(id, hex);
    sprintf(path_out, ".pes/objects/%.2s/%s", hex, hex + 2);
}

// ─────────────────────────────────────────────
// Helper: type → string
// ─────────────────────────────────────────────
static const char *type_to_string(ObjectType type) {
    switch (type) {
        case OBJ_BLOB: return "blob";
        case OBJ_TREE: return "tree";
        case OBJ_COMMIT: return "commit";
        default: return NULL;
    }
}

// ─────────────────────────────────────────────
// WRITE OBJECT
// ─────────────────────────────────────────────
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out) {

    const char *type_str = type_to_string(type);
    if (!type_str) return -1;

    // 1. header
    char header[64];
    int header_len = sprintf(header, "%s %zu", type_str, len) + 1;

    // 2. combine
    size_t total_len = header_len + len;
    unsigned char *full = malloc(total_len);
    memcpy(full, header, header_len);
    memcpy(full + header_len, data, len);

    // 3. hash
    compute_hash(full, total_len, id_out);

    // 4. hex
    char hex[65];
    hash_to_hex(id_out, hex);

    // 5. dirs
    mkdir(".pes", 0700);
    mkdir(".pes/objects", 0700);

    char dir[256];
    sprintf(dir, ".pes/objects/%.2s", hex);
    mkdir(dir, 0700);

    // 6. path
    char path[256];
    sprintf(path, "%s/%s", dir, hex + 2);

    // 7. atomic write
    char tmp[300];
    sprintf(tmp, "%s.tmp", path);

    int fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        free(full);
        return -1;
    }

    write(fd, full, total_len);
    fsync(fd);
    close(fd);

    rename(tmp, path);

    free(full);
    return 0;
}

// ─────────────────────────────────────────────
// READ OBJECT
// ─────────────────────────────────────────────
int object_read(const ObjectID *id, ObjectType *type_out, void **data_out, size_t *len_out) {

    char path[256];
    object_path(id, path);

    int fd = open(path, O_RDONLY);
    if (fd < 0) return -1;

    struct stat st;
    fstat(fd, &st);

    size_t size = st.st_size;

    char *buf = malloc(size);
    read(fd, buf, size);
    close(fd);

    // verify hash
    ObjectID computed;
    compute_hash(buf, size, &computed);

    if (memcmp(computed.hash, id->hash, HASH_SIZE) != 0) {
        free(buf);
        return -1;
    }

    // detect type
    if (strncmp(buf, "blob", 4) == 0)
        *type_out = OBJ_BLOB;
    else if (strncmp(buf, "tree", 4) == 0)
        *type_out = OBJ_TREE;
    else if (strncmp(buf, "commit", 6) == 0)
        *type_out = OBJ_COMMIT;
    else {
        free(buf);
        return -1;
    }

    // skip header
    char *data_start = strchr(buf, '\0') + 1;
    size_t data_len = size - (data_start - buf);

    void *data = malloc(data_len);
    memcpy(data, data_start, data_len);

    *data_out = data;
    *len_out = data_len;

    free(buf);
    return 0;
}
