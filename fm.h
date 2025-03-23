#ifndef _FM_H
#define _FM_H

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>



typedef struct {
    char name[NAME_MAX];
    const char *type;
    unsigned char dtype;
    size_t size;
} Entry;

typedef struct {
    size_t size;
    Entry *entries;
} Directory;

typedef struct {
    size_t cursor;
    char cwd[PATH_MAX];
    Directory dir;
} FileManager;


void fm_init(FileManager *fm, const char *dir);
void fm_destroy(FileManager *fm);
void fm_cd(FileManager *fm);
void fm_go_back(FileManager *fm);
void fm_go_up(FileManager *fm);
void fm_go_down(FileManager *fm);

#endif // _FM_H
