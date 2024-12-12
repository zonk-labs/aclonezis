#ifndef _CRC32_H_
#define _CRC32_H_

#include <stdint.h>

extern void crc32_reset(uint32_t* crc32_new_state);
extern void crc32_next_byte(uint8_t byte, uint32_t* crc32_state);
extern uint32_t crc32_finalize(uint32_t crc32_state);
extern uint32_t crc32_fast(const void *buf, size_t size);

#endif