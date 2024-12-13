#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "diff.h"
#include "merge.h"
#include "info.h"

void print_usage_info(){
    printf("info usage: info difffile\n");
    exit(EXIT_SUCCESS);
}

void print_usage_diff(){
    printf("diff usage: diff backingfile originalfile difffile\n");
    exit(EXIT_SUCCESS);
}

void print_usage_merge(){
    printf("merge usage: merge backingfile difffile mergedfile\n");
    exit(EXIT_SUCCESS);
}

void print_usage(){
        printf("Args: diff backingfile originalfile difffile\n");
        printf("      merge backingfile diffile restorefile\n");
        printf("      info backingfile\n");
        exit(EXIT_SUCCESS);
}

bool ensure_argnum(int argc, int required){
    if(required != argc){
        return false;
    }
    return true;
}

int main(int argc, char* argv[]){
    if(argc < 2) print_usage();
    char* mode = argv[1];

    if(strncasecmp(mode, "diff", 4) == 0){
        if(ensure_argnum(5, argc)){
            if(diff(argv[2], argv[3], argv[4])) return EXIT_FAILURE;
        } else {
            print_usage_diff();
        }
    } else if(strncasecmp(mode, "merge", 5) == 0){
        if(ensure_argnum(5, argc)){
            if(merge(argv[2], argv[3], argv[4])) return EXIT_FAILURE;
        } else {
            print_usage_merge();
        }
    } else if(strncasecmp(mode, "info", 4) == 0){
        if(ensure_argnum(3, argc)){
            if(info_display(argv[2])) return EXIT_FAILURE;
        } else {
            print_usage_info();
        }
    } else {
        printf("Unknown mode %s. Valid modes: diff, merge, info.\n", argv[1]);
        exit(EXIT_SUCCESS);
    }

    return EXIT_SUCCESS;
}