#include <stdlib.h>

#include "io.h"

int io_get_filesize(FILE* fp, size_t *size){
    if(fp == NULL || size == NULL) return -1;
    
    long old_pos = ftell(fp);
    if(old_pos < 0) return -1;
    if(fseek(fp, 0, SEEK_END)) return -1;
    
    long size_l = ftell(fp);
    if(size_l < 0) return -1;

    if(fseek(fp, old_pos, SEEK_SET)) return -1;

    *size = (size_t)size_l;
    return 0;
}

int io_new_copybuffer(IOCOPYBUFFER* buffer){
    buffer->buffer = calloc(buffer->size, 1);
    if(!buffer->buffer) return -1;
    return 0;
}

int io_free_copybuffer(IOCOPYBUFFER* buffer){
    free(buffer->buffer);
    buffer->buffer = NULL;
    buffer->size = 0;
    return 0;
}

int io_process_read_buffered(FILE* fp, size_t offset, size_t len,
    IOCOPYBUFFER buffer, int (*process)(char* byte)){

    if(fp == NULL || len == 0 || buffer.buffer == 0) return -1;

    size_t filesize;
    if(io_get_filesize(fp, &filesize)) return -1;
    if(filesize < offset || offset + len > filesize) return -1;
    if(fseek(fp, offset, SEEK_SET)) return -1;

    size_t bytes_processed = 0;
    size_t window = buffer.size;
    do {
        if(len - bytes_processed < buffer.size) window = len % buffer.size;
        if(fread(buffer.buffer, 1, window, fp) != window) return -1;
        if (process != NULL){
            for(size_t i = 0; i < window; i++){
                char* buff = (char*)(buffer.buffer);
                if(process(&(buff[i]))) return -1;
            }
        }
        bytes_processed += window;
    } while(bytes_processed < len);

    return 0;
}

int io_process_copy_buffered(FILE* src, FILE* dst, size_t offset_src,
    size_t offset_dst, size_t len, IOCOPYBUFFER buffer,
    int (*process)(char* byte)){
    
    if(src == NULL || dst == NULL || len == 0 || buffer.size == 0) return -1;

    size_t src_size, dst_size;
    if(io_get_filesize(src, &src_size) || io_get_filesize(dst, &dst_size))
        return -1;
    if(src_size < offset_src || dst_size < offset_dst
        || offset_src + len > src_size) return -1;

    if(fseek(src, offset_src, SEEK_SET) || fseek(dst, offset_dst, SEEK_SET))
        return -1;

    size_t bytes_copied = 0;
    size_t xferwindow = buffer.size;
    do {
        if(len - bytes_copied < buffer.size) xferwindow = len % buffer.size;
        if(fread(buffer.buffer, 1, xferwindow, src) != xferwindow) return -1;

        if (process != NULL){
            for(size_t i = 0; i < xferwindow; i++){
                char* buff = (char*)buffer.buffer;
                if(process(&buff[i])) return -1;
            }
        }

        if(fwrite(buffer.buffer, 1, xferwindow, dst) != xferwindow) return -1;
        bytes_copied += xferwindow;
    } while(bytes_copied < len);

    return 0;
}