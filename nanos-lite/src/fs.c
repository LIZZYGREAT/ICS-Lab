#include "fs.h"


typedef size_t (*ReadFn) (void *buf, off_t offset, size_t len);
typedef size_t (*WriteFn) (const void *buf, off_t offset, size_t len);

typedef struct {
  char *name;
  size_t size;
  off_t disk_offset;
  off_t open_offset;   
  ReadFn read;
  WriteFn write;
} Finfo;

enum {FD_STDIN, FD_STDOUT, FD_STDERR, FD_FB, FD_EVENTS, FD_DISPINFO};

size_t invalid_read(void *buf, off_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

size_t invalid_write(const void *buf, off_t offset, size_t len) {
  panic("should not reach here");
  return 0;
}

extern size_t ramdisk_read(void *buf, off_t offset, size_t len);
extern size_t ramdisk_write(const void *buf, off_t offset, size_t len);
extern size_t serial_write(const void *buf, off_t offset, size_t len);
extern size_t events_read(void *buf, off_t offset, size_t len);
extern size_t dispinfo_read(void *buf, off_t offset, size_t len);
extern size_t fb_write(const void *buf, off_t offset, size_t len);

static Finfo file_table[] __attribute__((used)) = {
  {"stdin", 0, 0, 0, invalid_read, invalid_write},
  {"stdout", 0, 0, 0, invalid_read, serial_write},
  {"stderr", 0, 0, 0, invalid_read, serial_write},
  {"/dev/fb", 0, 0, 0, invalid_read, fb_write},
  {"/dev/events", 0, 0, 0, events_read, invalid_write},
  {"/proc/dispinfo", 0, 0, 0, dispinfo_read, invalid_write},
#include "files.h"
};

#define NR_FILES (sizeof(file_table) / sizeof(file_table[0]))

void init_fs() {
  file_table[FD_FB].size = _screen.width * _screen.height * 4;
}


size_t fs_filesz(int fd) {
  return file_table[fd].size;
}

int fs_open(const char *pathname, int flags, int mode) {
  for (int i = 0; i < NR_FILES; i++) {
    if (strcmp(pathname, file_table[i].name) == 0) {
      file_table[i].open_offset = 0; 
      return i; 
    }
  }
  assert(0); 
  return -1;
}

ssize_t fs_read(int fd, void *buf, size_t len) {
  Finfo *file = &file_table[fd];
  
  if (file->read != NULL) {
    size_t read_len = file->read(buf, file->open_offset, len);
    file->open_offset += read_len;
    return read_len;
  }
  
  size_t read_len = len;
  if (file->open_offset + len > file->size) {
    read_len = file->size - file->open_offset;
  }
  
  ramdisk_read(buf, file->disk_offset + file->open_offset, read_len);
  file->open_offset += read_len;
  return read_len;
}

ssize_t fs_write(int fd, const void *buf, size_t len) {
  Finfo *file = &file_table[fd];
  
  if (file->write != NULL) {
    size_t write_len = file->write(buf, file->open_offset, len);
    file->open_offset += write_len;
    return write_len;
  }
  
  size_t write_len = len;
  if (file->open_offset + len > file->size) {
    write_len = file->size - file->open_offset;
  }
  
  ramdisk_write(buf, file->disk_offset + file->open_offset, write_len);
  file->open_offset += write_len;
  return write_len;
}

off_t fs_lseek(int fd, off_t offset, int whence) {
  Finfo *file = &file_table[fd];
  
  switch (whence) {
    case SEEK_SET:
      file->open_offset = offset;
      break;
    case SEEK_CUR:
      file->open_offset += offset;
      break;
    case SEEK_END:
      file->open_offset = file->size + offset;
      break;
    default:
      assert(0);
      return -1;
  }
  
  if (file->open_offset > file->size) {
    file->open_offset = file->size;
  } else if (file->open_offset < 0) {
    file->open_offset = 0;
  }
  
  return file->open_offset;
}

int fs_close(int fd) {
  return 0;
}
