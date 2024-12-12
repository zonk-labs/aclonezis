#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "diff.h"
#include "merge.h"

int main(int argc, char* argv[]){
    if(argc != 5) {
        printf("Args: diff backingfile originalfile difffile\n");
        printf("      join backingfile diffile restorefile\n");
        exit(EXIT_SUCCESS);
    }
    char* mode = argv[1];

    if(strncasecmp(mode, "diff", 4) == 0){
        diff(argv[2], argv[3], argv[4]);
    } else if(strncasecmp(mode, "join", 4) == 0){
        merge(argv[2], argv[3], argv[4]);
    } else {
        printf("Unknown mode %s. Valid modes: diff, join.\n", argv[1]);
    }
}