#ifndef _FM_H
#define _FM_H

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>



typedef struct {
    const char *name;
    const char *type;
    size_t size;
} Entry;

typedef struct {
    char cwd[PATH_MAX];
} FileManager;

void fm_init(FileManager *fm, const char *startdir);
void fm_readdir(const FileManager *fm);


#endif // _FM_H
