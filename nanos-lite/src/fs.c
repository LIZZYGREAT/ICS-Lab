#include "fs.h"

extern void init_device(); 

typedef size_t (*ReadFn) (void *buf, off_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, off_t offset, size_t len);

extern size_t dispinfo_read(void *buf, off_t offset, size_t len);
extern size_t fb_write(const void *buf, off_t offset, size_t len);
extern size_t events_read(void *buf, off_t offset, size_t len);

// Declare underlying ramdisk interfaces
extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern void ramdisk_write(const void *buf, off_t offset, size_t len);
// Declare underlying serial output interface
extern void _putc(char c);

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;  
  ReadFn read;
  WriteFn write; 
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

/* This is the information about all files in disk. */

/*static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};*/

static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0, 0, NULL, NULL},
  {"stdout (note that this is not the actual stdout)", 0, 0, 0, NULL, NULL},
  {"stderr (note that this is not the actual stderr)", 0, 0, 0, NULL, NULL},
  [FD_FB] = {"/dev/fb", 0, 0, 0, NULL, fb_write},
  [FD_EVENTS] = {"/dev/events", 0, 0, 0, events_read, NULL},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0, 0, dispinfo_read, NULL},
#include "files.h"
};


#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  file_table[FD_FB].size = _screen.width * _screen.height * 4;
}

// 1. Open file and reset its offset
int fs_open(const char *pathname, int flags, int mode) {
  for (int i = 0; i < NR_FILES; i++) {
    // Match the filename
    if (strcmp(file_table[i].name, pathname) == 0) {
      // Initialize the read/write offset to 0 upon successfully opening the file
      file_table[i].open_offset = 0;
      return i; // Return the index as the file descriptor (fd)
    }
  }
  // If the file is not found, panic or assert
  assert(0);
  return -1;
}

/*
// 2. Read from file with boundary check
size_t fs_read(int fd, void *buf, size_t len) {
  Finfo *file = &file_table[fd];

  // Prevent reading beyond the file size
  if (file->open_offset + len > file->size) {
    len = file->size - file->open_offset;
  }

  // If already at EOF, return 0 directly
  if (len == 0) {
    return 0;
  }

  // Read data from the calculated physical offset in the ramdisk
  ramdisk_read(buf, file->disk_offset + file->open_offset, len);

  // Update the file's dynamic open_offset
  file->open_offset += len;

  return len;
}

// 3. Write to file or terminal
size_t fs_write(int fd, const void *buf, size_t len) {
  // Route to terminal if fd indicates standard output or standard error
  if (fd == FD_STDOUT || fd == FD_STDERR) {
    char *str = (char *)buf;
    for (size_t i = 0; i < len; i++) {
      _putc(str[i]);
    }
    return len;
  }

  Finfo *file = &file_table[fd];

  // Prevent writing beyond the file size
  if (file->open_offset + len > file->size) {
    len = file->size - file->open_offset;
  }

  if (len == 0) {
    return 0;
  }

  // Write data to the calculated physical offset in the ramdisk
  ramdisk_write(buf, file->disk_offset + file->open_offset, len);

  // Update the file's dynamic open_offset
  file->open_offset += len;

  return len;
}*/

size_t fs_read(int fd, void *buf, size_t len) {
  Finfo *file = &file_table[fd];

  if (file->read != NULL) {
    size_t actual_read = file->read(buf, file->open_offset, len);
    file->open_offset += actual_read; 
    return actual_read;
  }

  if (file->open_offset + len > file->size) {
    len = file->size - file->open_offset;
  }
  if (len == 0) return 0;

  ramdisk_read(buf, file->disk_offset + file->open_offset, len);
  file->open_offset += len;

  return len;
}

size_t fs_write(int fd, const void *buf, size_t len) {
  if (fd == FD_STDOUT || fd == FD_STDERR) {
    char *str = (char *)buf;
    for (size_t i = 0; i < len; i++) {
      _putc(str[i]);
    }
    return len;
  }

  Finfo *file = &file_table[fd];

  if (file->write != NULL) {
    size_t actual_write = file->write(buf, file->open_offset, len);
    file->open_offset += actual_write;
    return actual_write;
  }

  if (file->open_offset + len > file->size) {
    len = file->size - file->open_offset;
  }
  if (len == 0) return 0;

  ramdisk_write(buf, file->disk_offset + file->open_offset, len);
  file->open_offset += len;

  return len;
}


// 4. Reposition read/write file offset
off_t fs_lseek(int fd, off_t offset, int whence) {
  Finfo *file = &file_table[fd];
  off_t new_offset = file->open_offset;

  // Calculate new offset based on the reference point
  switch (whence) {
    case SEEK_SET:
      new_offset = offset;
      break;
    case SEEK_CUR:
      new_offset += offset;
      break;
    case SEEK_END:
      new_offset = file->size + offset;
      break;
    default:
      assert(0);
      return -1;
  }

  // Strict boundary checks to prevent overflow or underflow
  if (new_offset < 0) {
    new_offset = 0;
  } else if (new_offset > file->size) {
    new_offset = file->size;
  }

  // Apply the legal offset back to the Finfo structure
  file->open_offset = new_offset;
  return new_offset;
}

// 5. Close file
int fs_close(int fd) {
  // No complex resources to release in this simple VFS implementation
  return 0;
}

// 6. Helper function to expose file size (Needed later for loader.c)
size_t fs_filesz(int fd) {
  return file_table[fd].size;
}
