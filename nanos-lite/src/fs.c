#include "fs.h"

extern void ramdisk_read(void *buf, off_t offset, size_t len);
extern void ramdisk_write(const void *buf, off_t offset, size_t len);
extern void _putc(char c);

extern size_t events_read(void *buf, off_t offset, size_t len);
extern size_t dispinfo_read(void *buf, off_t offset, size_t len);
extern size_t fb_write(const void *buf, off_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;  
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO, FD_NORMAL};

static Finfo file_table[] __attribute__((used)) = {
  {"stdin (note that this is not the actual stdin)", 0, 0},
  {"stdout (note that this is not the actual stdout)", 0, 0},
  {"stderr (note that this is not the actual stderr)", 0, 0},
  [FD_FB] = {"/dev/fb", 0, 0},
  [FD_EVENTS] = {"/dev/events", 0, 0},
  [FD_DISPINFO] = {"/proc/dispinfo", 128, 0},
#include "files.h"
};


#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
    for (int i = 0; i < NR_FILES; i++) {
        if (strcmp(file_table[i].name, "/dev/fb") == 0) {
            file_table[i].size = 400 * 300 * 4; 
            break;
        }
    }
}

int fs_open(const char *pathname, int flags, int mode) {
    for (int i = 0; i < NR_FILES; i++) {
        if (strcmp(file_table[i].name, pathname) == 0) {
            file_table[i].open_offset = 0; 
            return i;                     
        }
    }
    panic("File not found in file_table: %s", pathname);
    return -1;
}

ssize_t fs_read(int fd, void *buf, size_t len) {
    Finfo *f = &file_table[fd];
    ssize_t real_len = len;

    if (strcmp(f->name, "/proc/dispinfo") == 0) {
        real_len = dispinfo_read(buf, f->open_offset, len);
        f->open_offset += real_len;
        return real_len;
    }

    if (f->open_offset + len > f->size) {
        real_len = f->size - f->open_offset;
    }
    if (real_len <= 0) return 0;

    ramdisk_read(buf, f->disk_offset + f->open_offset, real_len);
    f->open_offset += real_len;
    return real_len;
}

ssize_t fs_write(int fd, const void *buf, size_t len) {
    Finfo *f = &file_table[fd];

    if (fd == 1 || fd == 2) {
        for (size_t i = 0; i < len; i++) {
            _putc(((char *)buf)[i]);
        }
        return len;
    }

    if (strcmp(f->name, "/dev/fb") == 0) {
        fb_write(buf, f->open_offset, len);
        f->open_offset += len;
        return len;
    }

    ssize_t write_len = len;
    if (f->open_offset + len > f->size) {
        write_len = f->size - f->open_offset;
    }
    if (write_len <= 0) return 0;

    ramdisk_write(buf, f->disk_offset + f->open_offset, write_len);
    f->open_offset += write_len;
    return write_len;
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
  return 0;
}

// 6. Helper function to expose file size (Needed later for loader.c)
size_t fs_filesz(int fd) {
  return file_table[fd].size;
}
