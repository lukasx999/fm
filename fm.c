#define _DEFAULT_SOURCE // required for file type macro constants by dirent
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <assert.h>
#include <errno.h>
#include <string.h>

#include <sys/stat.h>

#include "fm.h"

#define ARRAY_LEN(arr) (sizeof(arr) / sizeof *(arr))



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


static struct stat direntry_statbuf(const char *dirname, const char *entry_name) {

    char buf[PATH_MAX] = { 0 };
    snprintf(buf, ARRAY_LEN(buf), "%s/%s", dirname, entry_name);
    char *path = realpath(buf, NULL);

    struct stat statbuf = { 0 };
    stat(path, &statbuf);

    free(path);
    return statbuf;
}


void fm_init(FileManager *fm, const char *startdir) {

    *fm = (FileManager) {
        .cwd = { 0 },
    };

    char *err = realpath(startdir, fm->cwd);

    if (err == NULL) {
        fprintf(
            stderr,
            "Failed to open directory `%s`: %s\n",
            startdir,
            strerror(errno)
        );
        exit(1);
    }


}

static int dir_get_filecount(DIR *dir) {

    int filecount = 0;
    while (readdir(dir))
        filecount++;

    rewinddir(dir);
    return filecount;
}

void fm_readdir(const FileManager *fm) {

    DIR *dir = opendir(fm->cwd);
    assert(dir != NULL);


    int count = dir_get_filecount(dir);
    printf("count: %d\n", count);


    Entry *entries = malloc(sizeof(Entry) * count);
    size_t i = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {

        struct stat statbuf = direntry_statbuf(fm->cwd, entry->d_name);
        const char *type = filetype_repr(entry->d_type);

        entries[i++] = (Entry) {
            .name = entry->d_name,
            .type = type,
            .size = statbuf.st_size,
        };

    }

    assert(i == (size_t) count);

    free(entries);
    closedir(dir);
}

