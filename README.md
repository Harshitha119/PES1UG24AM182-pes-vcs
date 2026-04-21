📝 PES-VCS Lab Report - PES1UG24AM182

Phase 1: Object Storage Foundation
Filesystem Concepts: Content-addressable storage, directory sharding, atomic writes, hashing for integrity

Files: pes.h (read), object.c (implement object_write and object_read)

What to Implement
Open object.c. Two functions are marked // TODO:

object_write — Stores data in the object store.

Prepends a type header ("blob <size>\0", "tree <size>\0", or "commit <size>\0")
Computes SHA-256 of the full object (header + data)
Writes atomically using the temp-file-then-rename pattern
Shards into subdirectories by first 2 hex chars of hash
object_read — Retrieves and verifies data from the object store.

Reads the file, parses the header to extract type and size
Verifies integrity by recomputing the hash and comparing to the filename
Returns the data portion (after the \0)
Read the detailed step-by-step comments in object.c before starting.

Testing
make test_objects
./test_objects
The test program verifies:

Blob storage and retrieval (write, read back, compare)
Deduplication (same content → same hash → stored once)
Integrity checking (detects corrupted objects)
📸 Screenshot 1A: Output of ./test_objects showing all tests passing.
<img width="447" height="100" alt="image" src="https://github.com/user-attachments/assets/1678f7f8-f7f5-48c4-af49-15291f310494" />

📸 Screenshot 1B: find .pes/objects -type f showing the sharded directory structure.
<img width="383" height="57" alt="image" src="https://github.com/user-attachments/assets/4a3d2d85-3591-4e8d-a27d-0d19dc1e96ad" />

