#define _DEFAULT_SOURCE // required for file type macro constants by dirent
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>

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

static size_t dir_get_filecount(DIR *dir) {

    size_t filecount = 0;
    while (readdir(dir))
        filecount++;

    rewinddir(dir);
    return filecount;
}

static Directory read_dir(const char *dir) {

    DIR *dirp = opendir(dir);
    assert(dirp != NULL);

    size_t filecount = dir_get_filecount(dirp);
    Entry *entries = malloc(sizeof(Entry) * filecount);
    assert(entries != NULL);
    size_t i = 0;

    struct dirent *entry = NULL;
    while ((entry = readdir(dirp)) != NULL) {

        const char *type = filetype_repr(entry->d_type);

        Entry e = {
            .name    = { 0 },
            .abspath = { 0 },
            .type    = type,
        };

        strncpy(e.abspath, dir, ARRAY_LEN(e.abspath));
        strncat(e.abspath, "/", ARRAY_LEN(e.abspath));
        strncat(e.abspath, entry->d_name, ARRAY_LEN(e.abspath));

        struct stat statbuf = { 0 };
        stat(e.abspath, &statbuf);

        e.size = statbuf.st_size;
        e.mode = statbuf.st_mode;

        // d_name cannot be used as its free'd by closedir()
        strncpy(e.name, entry->d_name, ARRAY_LEN(e.name));

        entries[i++] = e;

    }

    assert(i == filecount);
    closedir(dirp);

    return (Directory) {
        .size = i,
        .entries = entries,
    };
}

static void updatedir(FileManager *fm) {
    free(fm->dir.entries);
    fm->dir = read_dir(fm->cwd);
}

void fm_init(FileManager *fm, const char *dir) {

    *fm = (FileManager) {
        .cursor = 0,
        .cwd    = { 0 },
        .dir    = { 0 },
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

    fm->dir = read_dir(fm->cwd);

}

void fm_destroy(FileManager *fm) {
    free(fm->dir.entries);
}

static void update_cwd(FileManager *fm, const char *dir) {

    char buf[PATH_MAX + NAME_MAX] = { 0 };
    snprintf(buf, ARRAY_LEN(buf), "%s/%s", fm->cwd, dir);
    char *err = realpath(buf, fm->cwd);
    assert(err != NULL);

    updatedir(fm);

    size_t filecount = fm->dir.size;

    if (fm->cursor >= filecount)
        fm->cursor = filecount - 1;
}

void fm_go_back(FileManager *fm) {
    update_cwd(fm, "..");
}

void fm_cd(FileManager *fm) {
    const Entry *entry = &fm->dir.entries[fm->cursor];

    if (strcmp(entry->type, filetype_repr(DT_DIR)))
        return;

    const char *subdir = entry->name;
    update_cwd(fm, subdir);
}

void fm_go_up(FileManager *fm) {
    if (fm->cursor != 0)
        fm->cursor--;
}

void fm_go_down(FileManager *fm) {
    if (fm->cursor != fm->dir.size - 1)
        fm->cursor++;
}

void fm_select(FileManager *fm) {
    Entry *entry = fm_get_current(fm);
}

Entry *fm_get_current(const FileManager *fm) {
    return &fm->dir.entries[fm->cursor];
}
