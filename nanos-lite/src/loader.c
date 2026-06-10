#include "common.h"

#define DEFAULT_ENTRY ((void *)0x4000000)

// Declare VFS interfaces used by the loader
extern int fs_open(const char *pathname, int flags, int mode);
extern size_t fs_read(int fd, void *buf, size_t len);
extern int fs_close(int fd);
extern size_t fs_filesz(int fd);

uintptr_t loader(_Protect *as, const char *filename) {
  // 1. Open the executable file via VFS
  int fd = fs_open(filename, 0, 0);

  // 2. Get the exact size of the file
  size_t size = fs_filesz(fd);

  // 3. Read the entire file into the default memory entry address
  fs_read(fd, DEFAULT_ENTRY, size);

  // 4. Close the file to prevent descriptor leakage
  fs_close(fd);

  return (uintptr_t)DEFAULT_ENTRY;
}
