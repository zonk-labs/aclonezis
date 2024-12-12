#include <stdio.h>

#include "diff.h"
#include "piece.h"
#include "io.h"
#include "crc32.h"

#define DIFF_DEFAULT_PIECE_LEN UINT64_MAX

static uint32_t crc32_original, crc32_backing;
static IOCOPYBUFFER buffer_original, buffer_backing;

enum copy_state {
    CHANGED, DUPLICATE
};

static void cleanup(FILE* backing_fp, FILE* original_fp, FILE* result_fp,
    IOCOPYBUFFER* backing, IOCOPYBUFFER* original){
        if(backing_fp) fclose(backing_fp);
        if(original_fp) fclose(original_fp);
        if(result_fp) fclose(result_fp);
        if(backing) io_free_copybuffer(backing);
        if(original) io_free_copybuffer(original);
}

static int finalize_header(HEADER header, FILE* result_fp){
    header.backing_crc32 = crc32_finalize(crc32_backing);
    header.original_crc32 = crc32_finalize(crc32_original);
    header.flags = 0;
    return header_update(result_fp, header);
}

static int compare_and_write_buffered(FILE* backing_fp, FILE* original_fp,
    FILE* result_fp, IOCOPYBUFFER buffer_backing, IOCOPYBUFFER buffer_original,
    size_t filesize, HEADER* header){
    if(backing_fp == NULL || original_fp == NULL || result_fp == NULL
        || filesize == 0 ||buffer_backing.size != buffer_original.size
        || buffer_original.buffer == NULL || buffer_backing.buffer == NULL
        || header == NULL) return -1;

    size_t diffed_bytes = 0;
    enum copy_state copystate = DUPLICATE;
    long buffered_bytes;
    BOOKMARK last_piece;
    PIECE current_piece;

    do {
        buffered_bytes = fread(buffer_backing.buffer, 1, buffer_backing.size,
            backing_fp);
        if(fread(buffer_original.buffer, 1, buffer_original.size,
            original_fp) != buffered_bytes || buffered_bytes == -1) return -1;

        for(size_t i = 0; i < buffered_bytes; i++){
            crc32_next_byte(((uint8_t*)(buffer_backing.buffer))[i],
                &crc32_backing);
            crc32_next_byte(((uint8_t*)(buffer_original.buffer))[i],
                &crc32_original);

            if(((char*)(buffer_backing.buffer))[i] ^
                ((char*)(buffer_original.buffer))[i]){
                if(copystate == DUPLICATE){
                    copystate = CHANGED;
                    current_piece.offset = diffed_bytes;
                    last_piece = piece_bookmark(result_fp, current_piece);
                    if(piece_write(result_fp, current_piece)) return -1;
                    header->num_sections++;
                }
                if(fwrite(&(((char*)(buffer_original.buffer))[i]), 1, 1,
                    result_fp) < 0) return -1;
            } else {
                if(copystate == CHANGED){
                    copystate = DUPLICATE;
                    if(bookmark_update(last_piece)) return -1;
                }
            }
            diffed_bytes++;
        }
    } while (diffed_bytes < filesize);

    if(copystate == CHANGED){
        if(bookmark_update(last_piece)) return -1;
    }

    return 0;
}

static int open_files(FILE** backing_fp, FILE** original_fp, FILE** result_fp,
    char* backing_file, char* original_file, char* result_file){
    int rc = 0;

    *backing_fp = fopen(backing_file, "rb");
    if(!backing_fp) {perror("diff couldn't open backing file"); rc = -1;}
    *original_fp = fopen(original_file, "rb");
    if(!original_fp) {perror("diff couldn't open original file"); rc = -1;}
    *result_fp = fopen(result_file, "w+b");
    if(!result_fp) {perror("diff couldn't open result file"); rc = -1;}

    return rc;
}

int diff(char* backing_file, char* original_file, char* result_file){
    if(backing_file == NULL || original_file == NULL || original_file == NULL)
        return -1;

    FILE* backing_fp, *original_fp, *result_fp;
    if(open_files(&backing_fp, &original_fp, &result_fp, backing_file,
        original_file, result_file)){
        cleanup(backing_fp, original_fp, result_fp, NULL, NULL);
        return -1;
    }
    
    HEADER header = header_default();

    size_t backing_filesize, original_filesize;
    if(io_get_filesize(backing_fp, &backing_filesize) ||
        io_get_filesize(original_fp, &original_filesize)){

        perror("Couldn't get filesizes");
        cleanup(backing_fp, original_fp, result_fp, NULL, NULL);

        return -1;
    }

    if(original_filesize != backing_filesize) {
        fprintf(stderr, "Filesize of diffed file must match backing file!\n");
        cleanup(backing_fp, original_fp, result_fp, NULL, NULL);

        return -1;
    }

    header.backing_file_size = backing_filesize;
    if(header_update(result_fp, header)){

        perror("Couldn't update file header");
        cleanup(backing_fp, original_fp, result_fp, NULL, NULL);

        return -1;
    }

    buffer_backing.size = IO_DEFAULT_BUFFER_SIZE;
    buffer_original.size = IO_DEFAULT_BUFFER_SIZE;
    if(io_new_copybuffer(&buffer_backing) || 
            io_new_copybuffer(&buffer_original)){

        perror("Couldn't allocate copy buffers");
        cleanup(backing_fp, original_fp, result_fp, &buffer_backing,
            &buffer_original);

        return -1;
    }

    crc32_reset(&crc32_backing);
    crc32_reset(&crc32_original);

    if(compare_and_write_buffered(backing_fp, original_fp, result_fp,
        buffer_backing, buffer_original, backing_filesize, 
        &header)){

        perror("Diff copying failed");
        cleanup(backing_fp, original_fp, result_fp, &buffer_backing,
            &buffer_original);

        return -1;
    }

    int rc = finalize_header(header, result_fp);

    cleanup(backing_fp, original_fp, result_fp, &buffer_backing,
        &buffer_original);
    return rc;
}