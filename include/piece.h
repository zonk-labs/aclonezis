#ifndef _PIECE_H_
#define _PIECE_H_

#include <stdint.h>
#include <stdio.h>

#define HEADER_FLAG_FILE_INCOMPLETE 1 << 8
#define HEADER_MAGIC_NO 0xACDF

#pragma pack(push, 1)
typedef struct header_t {
    uint16_t magic_no;
    uint16_t flags;
    uint32_t num_sections;
    uint64_t backing_file_size;
    uint32_t backing_crc32;
    uint32_t original_crc32;
} HEADER;

typedef struct piece_t {
    uint64_t offset;
    uint64_t len;
} PIECE;

typedef struct bookmark_t {
    FILE* fp;
    uint64_t offset;
    PIECE piecedata;
} BOOKMARK;
#pragma pack(pop)

extern int header_read(FILE* fp, HEADER* header);

/**
 * @brief Generates a default header struct
 * 
 * Fills out a header struct with default values, and marks the diff file as
 * unitialised.
 * 
 * @return HEADER
 */
extern HEADER header_default();

/**
 * @brief Does a basic sanity check of the diff file header
 * 
 * @param header The diff file header
 * @return int returns 0 if the file header passes, -1 otherwise.
 */
extern int header_check(HEADER header);

/**
 * @brief Updates the header
 * 
 * @param fp the file for which the header needs to be written
 * @param header the header data to write
 * @return int returns 0 on success, -1 otherwise
 */
extern int header_update(FILE* fp, HEADER header);
extern int bookmark_update(BOOKMARK mark);
extern BOOKMARK piece_bookmark(FILE* fp, PIECE piece);
extern int piece_read(FILE* fp, PIECE* piece);
extern int piece_write(FILE* fp, PIECE piece);

#endif