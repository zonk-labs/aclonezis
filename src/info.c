#include "info.h"
#include "piece.h"
#include "io.h"

static int skip_pieces(FILE* diff_fp, size_t* num_sections_found){
    PIECE current;
    size_t filesize;

    if(io_get_filesize(diff_fp, &filesize)) return -1;
    long current_loc = ftell(diff_fp);
    *num_sections_found = 0;

    while(current_loc < filesize){
        if(piece_read(diff_fp, &current)) return -1;
        (*num_sections_found)++;
        fseek(diff_fp, current.len, SEEK_CUR);
    }

    return 0;
}

int info_display(char* difffile){

    FILE* diff_fp = fopen(difffile, "rb");
    if(diff_fp == NULL){
        perror("Couldn't open diff file");
        return -1;
    }

    HEADER header;
    if(header_read(diff_fp, &header)) return -1;

    if(header.magic_no != HEADER_MAGIC_NO){
        printf("Magic number doesn't match.\n");
    }

    printf("Backing file size: %lu\n", header.backing_file_size);

    if(header.flags & HEADER_FLAG_FILE_INCOMPLETE){
        printf("Header flag shows file is incomplete - CRC32 is likely meaningless.\n");
    }
    printf("Backing file CRC32: %x\n", header.backing_crc32);
    printf("Original file CRC32: %x\n", header.original_crc32);
    printf("Header # of sections: %u\n", header.num_sections);

    size_t num_sections_found;
    skip_pieces(diff_fp, &num_sections_found);
    printf("Actual # of sections found: %lu\n", num_sections_found);
    printf("Metadata overhead: %lu\n",
        (num_sections_found * sizeof(PIECE)) + sizeof(HEADER));

    double section_avg_len = (double)ftell(diff_fp) / (double)num_sections_found;
    printf("Average piece length: %'.2f\n", section_avg_len);

    if(fclose(diff_fp)) return -1;
    return 0;
}