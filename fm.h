#ifndef _FM_H
#define _FM_H

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof *(arr))

#define MAX_SELECT 5

typedef struct {
    char name[NAME_MAX];
    char abspath[PATH_MAX];
    const char *type;
    unsigned int dtype;
    size_t size;
    unsigned int mode;
} Entry;

typedef struct {
    size_t size;
    Entry *entries;
} Directory;

typedef struct {
    size_t cursor;
    char cwd[PATH_MAX];
    Directory dir;
    const char *selected[MAX_SELECT];
    size_t selected_size;
} FileManager;


void fm_init(FileManager *fm, const char *dir);
void fm_destroy(FileManager *fm);
void fm_cd(FileManager *fm);
void fm_cd_parent(FileManager *fm);
void fm_go_down(FileManager *fm);
void fm_go_up(FileManager *fm);
void fm_cd_home(FileManager *fm);
void fm_cd_abs(FileManager *fm, const char *path);
void fm_exec(const FileManager *fm, const char *bin, void (*exit_routine)(void));
Entry *fm_get_current(const FileManager *fm);

// TODO:
// fm_select()



#endif // _FM_H
