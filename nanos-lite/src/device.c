#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, off_t offset, size_t len) {
  // Call AM API to read a hardware key code
  int key = _read_key();
  bool down = false;

  // Check if a hardware key event is detected
  if (key != _KEY_NONE) {
    // The highest bit (0x8000) indicates a key down event
    if (key & 0x8000) {
      key ^= 0x8000; // Clear the highest bit to get the actual key code
      down = true;
    }
    // Format the key event to pure text: "kd KEYNAME\n" or "ku KEYNAME\n"
    snprintf(buf, len, "%s %s\n", down ? "kd" : "ku", keyname[key]);
  } else {
    // No key event, fallback to poll the system uptime
    // Format the time event to pure text: "t UPTIME\n"
    snprintf(buf, len, "t %d\n", _uptime());
  }

  // Return the actual length of the formatted string written to buf
  return strlen(buf);
}

static char dispinfo[128] __attribute__((used));

size_t dispinfo_read(void *buf, off_t offset, size_t len) {
  // Prevent out-of-bounds reading from the dispinfo array
  size_t max_len = strlen(dispinfo);
  if (offset >= max_len) return 0;
  if (offset + len > max_len) len = max_len - offset;

  // Copy the requested string segment to the user buffer
  strncpy(buf, dispinfo + offset, len);
  return len;
}

size_t fb_write(const void *buf, off_t offset, size_t len) {
  // The offset is in bytes. Since 1 pixel = 32-bit color (4 bytes),
  // we divide by 4 to get the actual pixel index.
  // Then calculate the 2D (x, y) coordinates on the screen.
  int x = (offset / 4) % _screen.width;
  int y = (offset / 4) / _screen.width;

  // Call AM's draw function to render the pixel data to the screen.
  // We divide len by 4 to pass the correct number of pixels (width), and set height to 1.
  _draw_rect((const uint32_t *)buf, x, y, len / 4, 1);

  return len;
}

void init_device() {
  _ioe_init();

  // Print the screen dimensions to the dispinfo array during initialization
  // Format strictly follows the Navy-apps convention
  snprintf(dispinfo, sizeof(dispinfo), "WIDTH:%d\nHEIGHT:%d\n", _screen.width, _screen.height);
}
