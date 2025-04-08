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
#include "util.h"
#include "strio.h"



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

// returns -1 if `dir` could not be opened
// reload cwd if `dir` is NULL
static int load_dir(FileManager *fm, const char *dir) {

    if (dir == NULL) dir = fm->cwd;

    DIR *dirp = opendir(dir);
    if (dirp == NULL) return -1;

    char *err = realpath(dir, fm->cwd);
    NON_NULL(err);

    // some space may be wasted, because of ignoring hidden files
    size_t filecount = dir_get_filecount(dirp);
    Entry *entries = malloc(sizeof(Entry) * filecount);
    NON_NULL(entries);
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

        snprintf(
            e.abspath,
            ARRAY_LEN(e.abspath),
            "%s/%s",
            fm->cwd,
            entry->d_name
        );

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

    // deallocate old dir, will do nothing when initializing as entries is NULL
    free(fm->dir.entries);

    fm->dir = (Directory) {
        .size    = i,
        .entries = entries,
    };


    // after loading dir with less entries than last one, move the cursor back
    // if its out of bounds
    check_cursor_bounds(fm);
    return 0;
}

void fm_init(FileManager *fm, const char *dir) {

    *fm = (FileManager) {
        .cursor        = 0,
        .cwd           = { 0 },
        .dir           = { .entries = NULL, .size = 0 },
        .show_hidden   = false,
        .wrap_cursor   = true,
    };

    int err = load_dir(fm, dir);
    if (err == -1) {
        fprintf(
            stderr,
            "Failed to open directory `%s`: %s\n",
            dir,
            strerror(errno)
        );
        exit(EXIT_FAILURE);
    }

}

void fm_destroy(FileManager *fm) {
    free(fm->dir.entries);
}

static void append_cwd(FileManager *fm, const char *dir) {

    char buf[PATH_MAX + NAME_MAX] = { 0 };
    snprintf(buf, ARRAY_LEN(buf), "%s/%s", fm->cwd, dir);

    load_dir(fm, buf);
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
    load_dir(fm, path);
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

void fm_toggle_hidden(FileManager *fm) {
    fm->show_hidden = !fm->show_hidden;
    load_dir(fm, NULL);
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
        sel->size--;
        memmove(
            sel->paths + i,
            sel->paths + i + 1,
            (sel->size - i) * sizeof(*sel->paths)
        );

    }
}

bool fm_is_selected(const FileManager *fm, const char *path) {
    return fm_search_selection(fm, path) != -1;
}


void fm_exec(const FileManager *fm, const char *bin, void (*exit_routine)(void)) {
    Entry *e = fm_get_current(fm);
    if (e == NULL)
        return;

    exit_routine();
    int err = execlp(bin, bin, e->abspath, NULL);
    if (err == -1) {
        fprintf(stderr, "Failed to execute `%s`: %s\n", bin, strerror(errno));
        exit(EXIT_FAILURE);
    }

}

static void run_cmd(const char *cmd) {

    if (fork() == 0) {
        int err = execlp("/bin/sh", "sh", "-c", cmd, NULL);
        if (err == -1) {
            fprintf(stderr, "Failed to execute `%s`: %s\n", cmd, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }

    wait(NULL);

}

void fm_run_cmd_selected(FileManager *fm, const char *cmd) {
    Selections *sel = &fm->sel;

    for (size_t i=0; i < sel->size; ++i) {
        const char *path = sel->paths[i];
        char *full_cmd = string_expand_query(cmd, "{}", path);

        run_cmd(full_cmd);

        free(full_cmd);
    }

    load_dir(fm, NULL);

}
