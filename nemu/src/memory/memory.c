#include "nemu.h"
#include "device/mmio.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  /* Check if the physical address falls into any memory-mapped I/O space */
  int map_NO = is_mmio(addr);
  if (map_NO != -1) {
    /* Route the read request to the corresponding MMIO device controller */
    return mmio_read(addr, len, map_NO);
  }

  /* Regular physical memory read using the mask code to get the right length */
  return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  /* Check if the physical address falls into any memory-mapped I/O space */
  int map_NO = is_mmio(addr);
  if (map_NO != -1) {
    /* Route the write request to the corresponding MMIO device controller */
    mmio_write(addr, len, data, map_NO);
    return;
  }

  /* Regular physical memory write to the pmem array */
  memcpy(guest_to_host(addr), &data, len);
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  paddr_write(addr, len, data);
}
