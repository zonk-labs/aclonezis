#include <stdint.h>
#include <stddef.h>

#include "crc32.h"

#define CRC32_POLYNOMIAL 0xEDB88320L

static uint32_t crc32_table[256];
static int inited = 0;

static void crc32_build_table() {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ ((crc & 1) ? CRC32_POLYNOMIAL : 0);
        }
        crc32_table[i] = crc;
    }
}

void crc32_reset(uint32_t* crc32_new_state){
    if (!inited) {
        crc32_build_table();
        inited = 1;
    }
    *crc32_new_state = 0xFFFFFFFF;
}

void crc32_next_byte(uint8_t byte, uint32_t* crc32_state){
    *crc32_state = 
        (*crc32_state >> 8) ^ crc32_table[
                (*crc32_state & 0xFF) ^ byte
            ];
}

uint32_t crc32_finalize(uint32_t crc32_state){
    return crc32_state ^ 0xFFFFFFFF;
}

uint32_t crc32_fast(const void *buf, size_t size) {
    const uint8_t *p = buf;
    uint32_t crc = 0xFFFFFFFF;
    
    while (size--) {
        crc = (crc >> 8) ^ crc32_table[(crc & 0xFF) ^ *p++];
    }
    
    return crc ^ 0xFFFFFFFF;
}