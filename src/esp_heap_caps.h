/*
 * ESP32 heap_caps.h compatibility for RP2350
 * Maps ESP32 heap functions to RP2350 PSRAM allocator
 */
#ifndef ESP_HEAP_CAPS_H
#define ESP_HEAP_CAPS_H

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define MALLOC_CAP_SPIRAM   (1 << 0)
#define MALLOC_CAP_DMA      (1 << 1)
#define MALLOC_CAP_8BIT     (1 << 2)
#define MALLOC_CAP_32BIT    (1 << 3)
#define MALLOC_CAP_DEFAULT  (1 << 4)

/* Forward declare psram functions - implemented in drivers */
extern void *psram_malloc(size_t size);
extern void psram_free(void *ptr);

static inline void *heap_caps_malloc(size_t size, uint32_t caps) {
    if (caps & MALLOC_CAP_SPIRAM) {
        return psram_malloc(size);
    }
    return malloc(size);
}

static inline void *heap_caps_calloc(size_t n, size_t size, uint32_t caps) {
    if (caps & MALLOC_CAP_SPIRAM) {
        void *ptr = psram_malloc(n * size);
        if (ptr) {
            memset(ptr, 0, n * size);
        }
        return ptr;
    }
    return calloc(n, size);
}

static inline void heap_caps_free(void *ptr) {
    // We don't have a way to know if it was PSRAM or regular malloc
    // The psram_free function should handle this
    psram_free(ptr);
}

static inline size_t heap_caps_get_free_size(uint32_t caps) {
    // Return PSRAM free size for SPIRAM caps
    return 4 * 1024 * 1024;  // Estimate
}

static inline size_t heap_caps_get_largest_free_block(uint32_t caps) {
    return 2 * 1024 * 1024;  // Estimate
}

static inline void heap_caps_print_heap_info(uint32_t caps) {
    // Stub - no-op
}

#endif /* ESP_HEAP_CAPS_H */
