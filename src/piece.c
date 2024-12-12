#include "piece.h"

BOOKMARK piece_bookmark(FILE* fp, PIECE piece){
    BOOKMARK mark = {
        fp,
        ftell(fp),
        piece
    };

    return mark;
}

int bookmark_update(BOOKMARK mark){
    if(mark.fp == NULL) return -1;
    long current_pos = ftell(mark.fp);
    if(current_pos < 0) return -1;

    if(fseek(mark.fp, mark.offset, SEEK_SET)) return -1;
    mark.piecedata.len = current_pos - mark.offset - sizeof(PIECE);
    if(piece_write(mark.fp, mark.piecedata)) return -1;
    if(fseek(mark.fp, current_pos, SEEK_SET)) return -1;

    return 0;
}

int piece_read(FILE* fp, PIECE* piece){
    if(fp == NULL || piece == NULL) return -1;
    if(fread(piece, 1, sizeof(PIECE), fp) != sizeof(PIECE)) return -1;

    return 0;
}

int piece_write(FILE* fp, PIECE piece){
    if(fp == NULL) return -1;
    if(fwrite(&piece, 1, sizeof(PIECE), fp) != sizeof(PIECE)) return -1;

    return 0;
}

int header_update(FILE* fp, HEADER header){
    if(fp == NULL) return -1;
    if(fseek(fp, 0, SEEK_SET)) return -1;

    if(fwrite(&header, 1, sizeof(HEADER), fp) != sizeof(HEADER)) return -1;

    return 0;
}

int header_read(FILE* fp, HEADER* header){
    if(fp == NULL || header == NULL) return -1;
    if(fseek(fp, 0, SEEK_SET)) return -1;

    if(fread(header, 1, sizeof(HEADER), fp) != sizeof(HEADER)) return -1;

    return 0;
}

HEADER header_default(){
    HEADER rc;
    rc.flags = HEADER_FLAG_FILE_INCOMPLETE;
    rc.num_sections = 0;
    rc.magic_no = HEADER_MAGIC_NO;

    return rc;
}

int header_check(HEADER header){
    if(header.magic_no != HEADER_MAGIC_NO){
        fprintf(stderr, "Diff file has unknown file format.\n"); return -1;
    }
    if(header.flags != 0){
        fprintf(stderr, "Diff file is incomplete.\n"); return -1;
    }
    if(header.num_sections == 0){
        fprintf(stderr, "Diff file indicates identical files!\n"); return -1;
    }
    if(header.backing_file_size == 0){
        fprintf(stderr, "Diff file indicates empty file!\n"); return -1;
    }

    return 0;
}