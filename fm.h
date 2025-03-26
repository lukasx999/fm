#ifndef _FM_H
#define _FM_H

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <limits.h>

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof *(arr))

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

#define MAX_SELECTION 5

typedef struct {
    char paths[MAX_SELECTION][PATH_MAX];
    size_t size;
} Selections;

typedef struct {
    int cursor; // -1 represents no file being selected (empty dir)
    char cwd[PATH_MAX];
    Directory dir;
    bool show_hidden;
    bool wrap_cursor;
    Selections sel;
} FileManager;


void fm_init                   (FileManager *fm, const char *dir);
void fm_destroy                (FileManager *fm);
void fm_cd                     (FileManager *fm);
void fm_cd_parent              (FileManager *fm);
void fm_go_down                (FileManager *fm);
void fm_go_up                  (FileManager *fm);
void fm_cd_home                (FileManager *fm);
void fm_cd_abs                 (FileManager *fm, const char *path);
void fm_exec                   (const FileManager *fm, const char *bin, void (*exit_routine)(void));
void fm_toggle_hidden          (FileManager *fm);
void fm_toggle_cursor_wrapping (FileManager *fm);
void fm_toggle_select          (FileManager *fm);
Entry *fm_get_current          (const FileManager *fm);
bool fm_is_selected            (const FileManager *fm, const char *path);




#endif // _FM_H
