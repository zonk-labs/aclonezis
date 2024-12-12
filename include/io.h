#ifndef _IO_H_
#define _IO_H_

#define IO_DEFAULT_BUFFER_SIZE 1 << 16

#include <stdio.h>

typedef struct io_copybuffer_t {
    size_t size;
    void* buffer;
} IOCOPYBUFFER;

extern int io_get_filesize(FILE* fp, size_t *size);
extern int io_new_copybuffer(IOCOPYBUFFER* buffer);
extern int io_free_copybuffer(IOCOPYBUFFER* buffer);
extern int io_process_copy_buffered(FILE* src, FILE* dst, size_t offset_src,
    size_t offset_dst, size_t len, IOCOPYBUFFER buffer,
    int (*process)(char* byte));
extern int io_process_read_buffered(FILE* fp, size_t offset, size_t len,
    IOCOPYBUFFER buffer, int (*process)(char* byte));


#endif