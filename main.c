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

#define COLOR_BRIGHT_WHITE 15
#define KEY_RETURN 10

#define PAIR_WHITE       1
#define PAIR_BLUE        2
#define PAIR_GREEN       3
#define PAIR_RED         4
#define PAIR_SELECTED    5
#define PAIR_SELECTED_HL 6

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
    init_pair(PAIR_WHITE,       COLOR_BRIGHT_WHITE, COLOR_BLACK);
    init_pair(PAIR_RED,         COLOR_RED,          COLOR_BLACK);
    init_pair(PAIR_BLUE,        COLOR_BLUE,         COLOR_BLACK);
    init_pair(PAIR_GREEN,       COLOR_GREEN,        COLOR_BLACK);
    init_pair(PAIR_SELECTED,    COLOR_BLACK,        COLOR_BRIGHT_WHITE);
    init_pair(PAIR_SELECTED_HL, COLOR_BLACK,        COLOR_RED);

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

    attrset(COLOR_PAIR(PAIR_GREEN) | A_BOLD);
    mvprintw(0, 0, "%s@%s", username, hostname);

    attrset(COLOR_PAIR(0));
    printw(":");

    attrset(COLOR_PAIR(PAIR_BLUE) | A_BOLD);
    printw("%s", fm->cwd);
    if (strcmp(fm->cwd, "/"))
        printw("/");

    attrset(A_BOLD);
    printw("%s", fm_get_current(fm)->name);

    standend();
}

static void align(int padding) {

    int x, _y;
    getyx(stdscr, _y, x);
    (void) _y;

    for (int _=0; _ < padding - x; ++_)
        printw(" ");
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
        bool sel = i == fm->cursor;

        if (e->dtype == DT_DIR)
            attron(A_BOLD);

        move(i + off_y, off_x);

        attron(COLOR_PAIR(sel ? PAIR_SELECTED : PAIR_BLUE));
        printw("%lu ", e->size);
        align(10);

        attron(COLOR_PAIR(sel ? PAIR_SELECTED : PAIR_GREEN));
        printw("%s", e->type);
        align(20);

        attron(COLOR_PAIR(sel ? PAIR_SELECTED : PAIR_WHITE));
        printw("%s ", e->name);
        align(width);

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

            case 'R':
                fm_cd_abs(&fm, "/");
                break;

            case 'H':
                fm_cd_home(&fm);
                break;

            case 'h':
            case '-':
                fm_cd_parent(&fm);
                break;

            case KEY_RETURN:
                fm_exec(&fm, "nvim", exit_routine);
                break;

            case 'l':
                fm_cd(&fm);
                break;

            default:
                break;
        }


    }

    fm_destroy(&fm);

    return 0;
}
