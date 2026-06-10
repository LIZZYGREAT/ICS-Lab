#include "common.h"


#define NAME(key) \
  [_KEY_##key] = #key,


size_t serial_write(const void *buf, off_t offset, size_t len) {
  const char *str = (const char *)buf;
  for (size_t i = 0; i < len; i++) {
    _putc(str[i]);
  }
  return len;
}


static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, off_t offset, size_t len) {
  int key = _read_key();
  int is_down = 0;
  
  if (key & 0x8000) {
    key ^= 0x8000;
    is_down = 1;
  }
  
  if (key != _KEY_NONE) {
    return sprintf((char *)buf, "%s %s\n", is_down ? "kd" : "ku", keyname[key]);
  } else {
    return sprintf((char *)buf, "t %d\n", _uptime());
  }
}

static char dispinfo[128] __attribute__((used)) = {};

size_t dispinfo_read(void *buf, off_t offset, size_t len) {
  int dispinfo_len = strlen(dispinfo);
  if (offset >= dispinfo_len) return 0;
  
  size_t read_len = (offset + len > dispinfo_len) ? (dispinfo_len - offset) : len;
  memcpy(buf, dispinfo + offset, read_len);
  return read_len;
}

size_t fb_write(const void *buf, off_t offset, size_t len) {
  int x = (offset / 4) % _screen.width;
  int y = (offset / 4) / _screen.width;
  
  _draw_rect((const uint32_t *)buf, x, y, len / 4, 1);
  return len;
}

void init_device() {
  _ioe_init();

  sprintf(dispinfo, "WIDTH:%d\nHEIGHT:%d\n", _screen.width, _screen.height);
}
