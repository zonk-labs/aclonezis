#include <stdio.h>

#include "crc32.h"
#include "piece.h"
#include "io.h"

static uint32_t crc32_backing;
static uint32_t crc32_result;
static IOCOPYBUFFER buffer;

static int do_crc32_both(char* byte){
    crc32_next_byte((uint8_t)*byte, &crc32_backing);
    crc32_next_byte((uint8_t)*byte, &crc32_result);

    return 0;
}

static int do_crc32_backing(char* byte){
    crc32_next_byte((uint8_t)*byte, &crc32_backing);

    return 0;
}

static int do_crc32_result(char* byte){
    crc32_next_byte((uint8_t)*byte, &crc32_result);

    return 0;
}

static void finalize_crc32(){
    crc32_backing = crc32_finalize(crc32_backing);
    crc32_result = crc32_finalize(crc32_result);
}

static int compare_to_headerdata(HEADER header, size_t num_merged_pieces){
    finalize_crc32();

    int rc = 0;
    if(header.backing_crc32 != crc32_backing){
        printf("Backing checksums don't match:\nActual: %x Found: %x\n",
            header.backing_crc32, crc32_backing);
        rc = -1;
    }
    if(header.original_crc32 != crc32_result){
        printf("File checksums don't match:\nActual: %x Found: %x\n",
            header.original_crc32, crc32_backing);
        rc = -1;
    }
    if(header.num_sections != num_merged_pieces){
        printf("Number of sections don't match:\nActual: %x Found: %lx\n",
            header.num_sections, num_merged_pieces);
        rc = -1;
    }

    return rc;
}

static void cleanup(FILE* backing_fp, FILE* diff_fp, FILE* result_fp,
    IOCOPYBUFFER buffer){

    if(backing_fp != NULL) fclose(backing_fp);
    if(diff_fp != NULL) fclose(diff_fp);
    if(result_fp != NULL) fclose(result_fp);
    if(buffer.size != 0) io_free_copybuffer(&buffer);
}

/**
 * @brief Copy backing file after final piece
 * 
 * @param backing_fp File pointer to the backing file.
 * @param result_fp FIle pointer to the merged file.
 * @return int returns 0 on success, -1 on error
 */
static int copy_after_final_piece(FILE* backing_fp, FILE* result_fp){
    size_t backing_file_size;
    long current_pos = ftell(backing_fp);
    long result_pos = ftell(result_fp);

    if(io_get_filesize(backing_fp, &backing_file_size)) return -1;

    return io_process_copy_buffered(backing_fp, result_fp, current_pos,
        result_pos, backing_file_size - current_pos, buffer, do_crc32_both);
}

/**
 * @brief Copies the section of the backing file until the start of the diff
 *  file piece.
 * 
 * @param backing_fp File pointer to the backing file.
 * @param result_fp File pointer to the merged file.
 * @param piece Piece descriptor for this piece
 * @return int returns 0 on success, -1 on error
 */
static int copy_original_until_piece(FILE* backing_fp, FILE* result_fp, PIECE piece){
    if(backing_fp == NULL || result_fp == NULL) return -1;

    long src_offset = ftell(backing_fp);
    long dst_offset = ftell(result_fp);

    return io_process_copy_buffered(backing_fp, result_fp, src_offset, dst_offset,
        piece.offset - src_offset, buffer, do_crc32_both);
}

/**
 * @brief Copies the section of the diff file for the length of the file
 *  piece.
 * 
 * @param diff_fp File pointer to the diff file.
 * @param result_fp File pointer to the merged file.
 * @param piece Piece descriptor for this piece
 * @return int returns 0 on success, -1 on error
 */
static int copy_piece(FILE* diff_fp, FILE* result_fp, PIECE piece){
    if(diff_fp == NULL || result_fp == NULL) return -1;

    long src_offset = ftell(diff_fp);
    return io_process_copy_buffered(diff_fp, result_fp, src_offset, piece.offset,
        piece.len, buffer, do_crc32_result);
}

static int piece_sanity_check(PIECE piece, HEADER header){
    if(piece.offset > header.backing_file_size ||
        piece.len > header.backing_file_size) return -1;
    
    return 0;
}

/**
 * @brief Merges a single piece of a diff file.
 * 
 * @param backing_fp File pointer of the backing file.
 * @param diff_fp File pointer of the diff file.
 * @param result_fp File pointer of the result file.
 * @param header Header of the diff file.
 * @return int 0 if successful, -1 on failure.
 */
static int merge_piece(FILE* backing_fp, FILE* diff_fp, FILE* result_fp,
    HEADER header){

    PIECE current_piece;

    if(piece_read(diff_fp, &current_piece)){
        perror("Piece couldn't be read"); return -1;
    }

    if(piece_sanity_check(current_piece, header)) {
        fprintf(stderr, "Can't merge piece with nonsense values.\n");
        return -1;
    }

    long current_loc = ftell(result_fp);
    if(current_loc < 0) {
        perror("Result file location unknown"); return -1;
    }

    // If result data is "before" the first piece, fill with data from the
    // backing file
    if(current_piece.offset > current_loc){
        if(copy_original_until_piece(backing_fp, result_fp, current_piece))
            return -1;
        current_loc = ftell(result_fp);
    }

    //Update crc32 and file location of backing file for the ignored section
    if(io_process_read_buffered(backing_fp, current_piece.offset,
        current_piece.len, buffer, do_crc32_backing)) return -1;
    
    // Copy piece from the diff file
    return copy_piece(diff_fp, result_fp, current_piece);
}

int merge(char* backing_file, char* diff_file, char* result_file){
    if(backing_file == NULL || diff_file == NULL || result_file == NULL)
        return -1;

    FILE* backing_fp = fopen(backing_file, "rb");
    if(!backing_fp) {perror("merge couldn't open backing file"); return -1;}
    FILE* diff_fp = fopen(diff_file, "rb");
    if(!diff_fp) {perror("merge couldn't open diff file"); return -1;}
    FILE* result_fp = fopen(result_file, "w+b");
    if(!result_fp) {perror("merge couldn't open result file"); return -1;}

    crc32_reset(&crc32_backing);
    crc32_reset(&crc32_result);

    buffer.size = IO_DEFAULT_BUFFER_SIZE;
    if(io_new_copybuffer(&buffer)) {
        buffer.size = 0;
        cleanup(backing_fp, diff_fp, result_fp, buffer);
        return -1;
    }

    HEADER header;
    if(header_read(diff_fp, &header)) {perror("merge couldn't read header"); return -1;}
    if(header_check(header)) return -1;

    size_t diff_filesize;
    if(io_get_filesize(diff_fp, &diff_filesize)) return -1;

    size_t num_merged_pieces = 0;
    do {
        if(merge_piece(backing_fp, diff_fp, result_fp, header)){
            cleanup(backing_fp, diff_fp, result_fp, buffer);
            return -1;
        }
        num_merged_pieces++;
    } while(ftell(diff_fp) < diff_filesize);
    if(copy_after_final_piece(backing_fp, result_fp)) return -1;

    int rc = compare_to_headerdata(header, num_merged_pieces);

    cleanup(backing_fp, diff_fp, result_fp, buffer);

    return rc;
}