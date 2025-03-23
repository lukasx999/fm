#define _DEFAULT_SOURCE // required for file type macro constants by dirent
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>

#include <sys/stat.h>

#include <ncurses.h>

#include "fm.h"



static void curses_init(void) {
    initscr();
    noecho();
    keypad(stdscr, true);
    raw();
    curs_set(0);
}

static void curses_deinit(void) {
    endwin();
}

static void render_dir_entries(const FileManager *fm) {

    const Directory *dir = &fm->dir;

    for (size_t i=0; i < dir->size; ++i) {
        Entry *e = &dir->entries[i];
        mvprintw(i, 3, "%s, %lu, %s\n", e->name, e->size, e->type);
    }

}

static void exit_routine(void) {
    curses_deinit();
}

int main(void) {

    const char *startdir = "./test/";

    FileManager fm = { 0 };
    fm_init(&fm, startdir);

    curses_init();
    atexit(exit_routine);

    bool quit = false;
    while (!quit) {

        clear();
        render_dir_entries(&fm);
        mvprintw(fm.cursor, 0,  "-> ");
        wmove(stdscr, fm.cursor, 0);
        refresh();

        int c = getch();
        switch (c) {
            case 'q':
                quit = true;
                break;

            case 'j':
                fm_go_down(&fm);
                break;

            case 'k':
                fm_go_up(&fm);
                break;

            case '-':
                fm_go_back(&fm);
                break;

            case 10: // Enter
                fm_cd(&fm);
                break;

            default:
                break;
        }


    }

    fm_destroy(&fm);

    return 0;
}
