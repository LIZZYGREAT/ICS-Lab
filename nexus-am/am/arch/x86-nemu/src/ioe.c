#include <am.h>
#include <x86.h>

#define RTC_PORT 0x48
#define KEY_PORT 0x60
#define KEY_STATUS_PORT 0x64

static unsigned long boot_time;

void _ioe_init() {
  boot_time = inl(RTC_PORT);
}

unsigned long _uptime() {
  /* Return the elapsed time in milliseconds since system boot */
  return inl(RTC_PORT) - boot_time;
}

uint32_t* const fb = (uint32_t *)0x40000;

_Screen _screen = {
  .width  = 400,
  .height = 300,
};

extern void* memcpy(void *, const void *, int);

void _draw_rect(const uint32_t *pixels, int x, int y, int w, int h) {
  /* Copy pixels to the memory-mapped frame buffer row by row */
  int row;
  for (row = 0; row < h; row++) {
    memcpy(fb + (y + row) * _screen.width + x, pixels + row * w, w * sizeof(uint32_t));
  }
}

void _draw_sync() {
  /* Screen synchronization is handled automatically in x86-nemu */
}

int _read_key() {
  /* Check if the status register indicates that data is ready */
  if (inb(KEY_STATUS_PORT) & 0x1) {
    return inl(KEY_PORT);
  }
  return _KEY_NONE;
}
