#define _DEFAULT_SOURCE // required for file type macro constants by dirent
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <unistd.h>

#include <sys/stat.h>

#include <ncurses.h>

#include "fm.h"



static void curses_init(void) {
    initscr();
    noecho();
    keypad(stdscr, true);
    raw();
    curs_set(0);

    if (!has_colors())
        fprintf(stderr, "Your terminal does not support colors\n");

    start_color();
    use_default_colors();
    init_pair(1, COLOR_BLUE, COLOR_BLACK);
    init_pair(2, COLOR_GREEN, COLOR_BLACK);

}

static void curses_deinit(void) {
    endwin();
}

static void draw_topbar(const FileManager *fm) {

    char hostname[HOST_NAME_MAX] = { 0 };
    int err = gethostname(hostname, ARRAY_LEN(hostname));
    assert(err == 0);

    char *username = getlogin();
    assert(username != NULL);

    attrset(COLOR_PAIR(2) | A_BOLD);
    mvprintw(0, 0, "%s@%s", username, hostname);

    attrset(COLOR_PAIR(0));
    printw(":");

    attrset(COLOR_PAIR(1) | A_BOLD);
    printw("%s/", fm->cwd);

    attrset(A_BOLD);
    printw("%s", fm_get_current(fm)->name);

    standend();
}

static void draw_entries(
    const FileManager *fm,
    int off_y,
    int off_x,
    int height,
    int width
) {

    const Directory *dir = &fm->dir;

    for (size_t i=0; i < dir->size; ++i) {
        Entry *e = &dir->entries[i];

        if (i == fm->cursor)
            attrset(A_STANDOUT);

        mvprintw(i + off_y, off_x, "%s ", e->name);
        printw("%lu ", e->size);
        printw("%s", e->type);

        int x, y;
        getyx(stdscr, y, x);
        (void) y;

        for (int _=0; _ < width - x; ++_)
            printw(" ");

        standend();
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
        draw_topbar(&fm);
        draw_entries(&fm, 2, 2, 10, 30);
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
