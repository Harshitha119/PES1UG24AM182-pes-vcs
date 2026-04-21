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

--------------------------------------------------------------------------------------------------------------------
Phase 2: Tree Objects
Filesystem Concepts: Directory representation, recursive structures, file modes and permissions

Files: tree.h (read), tree.c (implement all TODO functions)

What to Implement
Open tree.c. Implement the function marked // TODO:

tree_from_index — Builds a tree hierarchy from the index.
Handles nested paths: "src/main.c" must create a src subtree
This is what pes commit uses to create the snapshot
Writes all tree objects to the object store and returns the root hash
Testing
make test_tree
./test_tree
The test program verifies:

Serialize → parse roundtrip preserves entries, modes, and hashes
Deterministic serialization (same entries in any order → identical output)
📸 Screenshot 2A: Output of ./test_tree showing all tests passing.
<img width="585" height="133" alt="Screenshot 2026-04-21 120401" src="https://github.com/user-attachments/assets/ed65683c-4809-495a-952a-db91192739ee" />


📸 Screenshot 2B: Pick a tree object from find .pes/objects -type f and run xxd .pes/objects/XX/YYY... | head -20 to show the raw binary format.
<img width="736" height="388" alt="WhatsApp Image 2026-04-21 at 2 45 27 PM" src="https://github.com/user-attachments/assets/f282600a-7d10-455f-8fec-05f1e21215d5" />



----------------------------------------------------------------------------------------------------------------------

Phase 3: The Index (Staging Area)
Filesystem Concepts: File format design, atomic writes, change detection using metadata

Files: index.h (read), index.c (implement all TODO functions)

What to Implement
Open index.c. Three functions are marked // TODO:

index_load — Reads the text-based .pes/index file into an Index struct.

If the file doesn't exist, initializes an empty index (this is not an error)
Parses each line: <mode> <hash-hex> <mtime> <size> <path>
index_save — Writes the index atomically (temp file + rename).

Sorts entries by path before writing
Uses fsync() on the temp file before renaming
index_add — Stages a file: reads it, writes blob to object store, updates index entry.

Use the provided index_find to check for an existing entry
index_find , index_status and index_remove are already implemented for you — read them to understand the index data structure before starting.

Expected Output of pes status
Staged changes:
  staged:     hello.txt
  staged:     src/main.c

Unstaged changes:
  modified:   README.md
  deleted:    old_file.txt

Untracked files:
  untracked:  notes.txt
If a section has no entries, print the header followed by (nothing to show).

Testing
make pes
./pes init
echo "hello" > file1.txt
echo "world" > file2.txt
./pes add file1.txt file2.txt
./pes status
cat .pes/index    # Human-readable text format
📸 Screenshot 3A: Run ./pes init, ./pes add file1.txt file2.txt, ./pes status — show the output.
<img width="640" height="244" alt="WhatsApp Image 2026-04-21 at 3 08 34 PM" src="https://github.com/user-attachments/assets/fdd3db2d-77e3-4e67-a850-f31a97ba2a16" />

📸 Screenshot 3B: cat .pes/index showing the text-format index with your entries.
<img width="741" height="86" alt="WhatsApp Image 2026-04-21 at 3 09 02 PM" src="https://github.com/user-attachments/assets/4499be4a-5702-47de-be8e-46d95e1cd28a" />


-----------------------------------------------------------------------------------------------------------------------
Phase 4: Commits and History
Filesystem Concepts: Linked structures on disk, reference files, atomic pointer updates

Files: commit.h (read), commit.c (implement all TODO functions)

What to Implement
Open commit.c. One function is marked // TODO:

commit_create — The main commit function:
Builds a tree from the index using tree_from_index() (not from the working directory — commits snapshot the staged state)
Reads current HEAD as the parent (may not exist for first commit)
Gets the author string from pes_author() (defined in pes.h)
Writes the commit object, then updates HEAD
commit_parse, commit_serialize, commit_walk, head_read, and head_update are already implemented — read them to understand the commit format before writing commit_create.

The commit text format is specified in the comment at the top of commit.c.

Testing
./pes init
echo "Hello" > hello.txt
./pes add hello.txt
./pes commit -m "Initial commit"

echo "World" >> hello.txt
./pes add hello.txt
./pes commit -m "Add world"

echo "Goodbye" > bye.txt
./pes add bye.txt
./pes commit -m "Add farewell"

./pes log
You can also run the full integration test:

make test-integration
📸 Screenshot 4A: Output of ./pes log showing three commits with hashes, authors, timestamps, and messages.
<img width="775" height="706" alt="WhatsApp Image 2026-04-21 at 3 21 35 PM" src="https://github.com/user-attachments/assets/27a2210e-7c96-4fa2-be79-8547b1ed5365" />

📸 Screenshot 4B: find .pes -type f | sort showing object store growth after three commits.

<img width="955" height="246" alt="WhatsApp Image 2026-04-21 at 3 21 52 PM" src="https://github.com/user-attachments/assets/92085d1e-667e-4880-bf1a-17030fb136ae" />


----------------------------------------------------------------------------------------------------------------
## Phase 5 & 6: Analysis-Only Questions

### Q5.1: Branching and Checkout

A branch in Git is represented as a file inside `.pes/refs/heads/` that stores a commit hash. To implement `pes checkout <branch>`, the system must:

* Update `.pes/HEAD` to point to the selected branch (or its commit hash).
* Read the commit hash stored in `.pes/refs/heads/<branch>`.
* Load the corresponding tree object from the object store.
* Update the working directory to match the files and contents of that tree.

The complexity arises because:

* Files in the working directory must be safely replaced.
* Uncommitted changes may be overwritten.
* Directory structures must be recreated correctly.
* File deletions and additions must be handled carefully.

---

### Q5.2: Detecting Dirty Working Directory

To detect conflicts before checkout:

1. Compare each tracked file in the index with the working directory:

   * Check file size and modification time.
2. If mismatch is found, recompute the file hash and compare with the index.
3. If the file content differs and also differs from the target branch version, a conflict exists.

If such conflicts are detected, checkout must be refused to prevent data loss.

---

### Q5.3: Detached HEAD

A detached HEAD means `.pes/HEAD` directly stores a commit hash instead of pointing to a branch.

If commits are made in this state:

* New commits are created but not linked to any branch.
* These commits may become unreachable.

Recovery:

* The user can create a new branch pointing to the commit.
* Example: `pes branch <new-branch>`
* If not referenced, commits may eventually be deleted by garbage collection.

---

## Garbage Collection and Space Reclamation

### Q6.1: Finding and Deleting Unreachable Objects

Algorithm:

1. Start from all branch heads (`.pes/refs/heads/`).
2. Traverse all commits recursively (parent links).
3. For each commit:

   * Mark its tree and blob objects as reachable.
4. Store reachable hashes in a **hash set** for efficient lookup.
5. Scan `.pes/objects/`:

   * Delete objects not in the reachable set.

Data Structure:

* Hash Set (for O(1) lookup)

Estimation:

* 100,000 commits with 50 branches → roughly 100,000–200,000 objects visited (including trees and blobs).

---

### Q6.2: GC and Race Conditions

Running garbage collection concurrently with a commit is dangerous.

Race Condition Example:

* Commit process creates a new object but has not yet updated HEAD.
* GC runs and does not see this object as reachable.
* GC deletes the object.
* Commit then updates HEAD → pointing to a missing object.

Git avoids this by:

* Using temporary files before finalizing objects.
* Locking references during updates.
* Performing GC only when safe (no concurrent writes).
* Using reference counting and reachability checks carefully.

---







