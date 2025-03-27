#define _DEFAULT_SOURCE // required for file type macro constants by dirent
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/wait.h>

#include "fm.h"




static const char *filetype_repr(unsigned char filetype) {
    switch (filetype) {
        case DT_UNKNOWN: return "unknown";
        case DT_FIFO:    return "fifo";
        case DT_CHR:     return "chr";
        case DT_DIR:     return "dir";
        case DT_BLK:     return "blk";
        case DT_REG:     return "reg";
        case DT_LNK:     return "lnk";
        case DT_SOCK:    return "sock";
        case DT_WHT:     return "wht";
        default:
            assert(!"unknown filetype");
    }
}

static bool can_open_dir(const char *dir) {
    DIR *dirp = opendir(dir);
    bool can_open = dirp != NULL;
    closedir(dirp);
    return can_open;
}

static size_t dir_get_filecount(DIR *dir) {

    size_t filecount = 0;
    while (readdir(dir))
        filecount++;

    rewinddir(dir);
    return filecount;
}

static void check_cursor_bounds(FileManager *fm) {
    size_t filecount = fm->dir.size;

    if (fm->cursor == -1) // last dir was empty
        fm->cursor = 0;

    else if ((size_t) fm->cursor >= filecount)
        fm->cursor = filecount - 1; // -1 if dir is empty
}

static int compare_entries(const void *a, const void *b) {
    const Entry *x = a;
    const Entry *y = b;

    int dircmp = (y->dtype == DT_DIR) - (x->dtype == DT_DIR);

    return dircmp == 0
    ? strcmp(x->name, y->name)
    : dircmp;
}

static void load_dir(FileManager *fm) {

    // deallocate old dir, will do nothing when initializing as entries is NULL
    free(fm->dir.entries);

    DIR *dirp = opendir(fm->cwd);
    assert(dirp != NULL);

    // some space may be wasted, because of ignoring hidden files
    size_t filecount = dir_get_filecount(dirp);
    Entry *entries = malloc(sizeof(Entry) * filecount);
    assert(entries != NULL);
    size_t i = 0;

    struct dirent *entry = NULL;
    while ((entry = readdir(dirp)) != NULL) {

        if (!fm->show_hidden && entry->d_name[0] == '.') continue;

        const char *type = filetype_repr(entry->d_type);

        Entry e = {
            .name    = { 0 },
            .abspath = { 0 },
            .type    = type,
            .dtype   = entry->d_type,
        };

        snprintf(e.abspath, ARRAY_LEN(e.abspath), "%s/%s", fm->cwd, entry->d_name);

        struct stat statbuf = { 0 };
        stat(e.abspath, &statbuf);

        e.size = statbuf.st_size;
        e.mode = statbuf.st_mode;

        // d_name cannot be used as its free'd by closedir()
        strncpy(e.name, entry->d_name, ARRAY_LEN(e.name));

        entries[i++] = e;

    }

    qsort(entries, i, sizeof(Entry), compare_entries);
    closedir(dirp);

    fm->dir = (Directory) {
        .size    = i,
        .entries = entries,
    };

    // after loading dir with less entries than last one, move the cursor back
    // if its out of bounds
    check_cursor_bounds(fm);
}

void fm_init(FileManager *fm, const char *dir) {

    *fm = (FileManager) {
        .cursor        = 0,
        .cwd           = { 0 },
        .dir           = { .entries = NULL, .size = 0 },
        .sel           = { .paths = { { 0 } }, .size = 0 },
        .show_hidden   = false,
        .wrap_cursor   = true,
    };

    char *err = realpath(dir, fm->cwd);
    if (err == NULL) {
        fprintf(
            stderr,
            "Failed to open directory `%s`: %s\n",
            dir,
            strerror(errno)
        );
        exit(1);
    }

    load_dir(fm);

}

void fm_destroy(FileManager *fm) {
    free(fm->dir.entries);
}

static void append_cwd(FileManager *fm, const char *dir) {

    char buf[PATH_MAX + NAME_MAX] = { 0 };
    snprintf(buf, ARRAY_LEN(buf), "%s/%s", fm->cwd, dir);

    if (!can_open_dir(buf)) return;

    char *err = realpath(buf, fm->cwd);
    assert(err != NULL);

    load_dir(fm);
}

void fm_cd_parent(FileManager *fm) {
    append_cwd(fm, "..");
}

void fm_cd(FileManager *fm) {
    if (fm->cursor == -1) return;
    const Entry *entry = &fm->dir.entries[fm->cursor];

    if (entry->dtype != DT_DIR) return;

    const char *subdir = entry->name;
    append_cwd(fm, subdir);
}

void fm_cd_abs(FileManager *fm, const char *path) {
    if (!can_open_dir(path)) return;

    memset(fm->cwd, 0, ARRAY_LEN(fm->cwd));
    strncpy(fm->cwd, path, ARRAY_LEN(fm->cwd));
    load_dir(fm);
}

void fm_cd_home(FileManager *fm) {
    char *home = getenv("HOME");
    assert(home != NULL);
    fm_cd_abs(fm, home);
}

void fm_go_up(FileManager *fm) {
    if (fm->cursor == -1) return;

    if (fm->cursor > 0)
        fm->cursor--;
    else if (fm->wrap_cursor)
        fm->cursor = fm->dir.size-1;
}

void fm_go_down(FileManager *fm) {
    if (fm->cursor == -1) return;

    if ((size_t) fm->cursor != fm->dir.size - 1)
        fm->cursor++;
    else if (fm->wrap_cursor)
        fm->cursor = 0;
}

Entry *fm_get_current(const FileManager *fm) {
    return fm->cursor == -1
    ? NULL
    : &fm->dir.entries[fm->cursor];
}

void fm_exec(const FileManager *fm, const char *bin, void (*exit_routine)(void)) {
    Entry *e = fm_get_current(fm);
    if (e == NULL)
        return;

    exit_routine();
    int err = execlp(bin, bin, e->abspath, NULL);
    if (err == -1) {
        fprintf(stderr, "Failed to execute `%s`: %s\n", bin, strerror(errno));
        exit(1);
    }
}

void fm_toggle_hidden(FileManager *fm) {
    fm->show_hidden = !fm->show_hidden;
    load_dir(fm);
}

void fm_toggle_cursor_wrapping(FileManager *fm) {
    fm->wrap_cursor = !fm->wrap_cursor;
}

static ssize_t fm_search_selection(const FileManager *fm, const char *path) {
    const Selections *sel = &fm->sel;

    for (size_t i=0; i < sel->size; ++i)
        if (!strcmp(sel->paths[i], path))
            return i;

    return -1;
}

void fm_toggle_select(FileManager *fm) {
    Selections *sel = &fm->sel;
    const char *path = fm_get_current(fm)->abspath;

    ssize_t i = fm_search_selection(fm, path);
    if (i == -1) {
        // Insert
        if (sel->size == MAX_SELECTION) return;
        strncpy(sel->paths[sel->size++], path, ARRAY_LEN(*sel->paths));

    } else {
        // Delete
        memmove(
            sel->paths + i,
            sel->paths + i + 1,
            (sel->size - i - 1) * sizeof(char*)
        );
        sel->size--;

    }
}

bool fm_is_selected(const FileManager *fm, const char *path) {
    return fm_search_selection(fm, path) != -1;
}
