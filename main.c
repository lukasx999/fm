#define _DEFAULT_SOURCE // required for file type macro constants by dirent
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <assert.h>
#include <unistd.h>
#include <ctype.h>

#include <sys/stat.h>

#include <ncurses.h>

#include "fm.h"
#include "next.h"



#define PAIR_WHITE       1
#define PAIR_BLUE        2
#define PAIR_GREEN       3
#define PAIR_RED         4
#define PAIR_SELECTED    5
#define PAIR_SELECTED_HL 6
#define PAIR_GREY        7
#define PAIR_YELLOW      8

static void curses_init(void) {
    initscr();
    noecho();
    keypad(stdscr, true);
    raw();
    curs_set(0);

    ESCDELAY = 0;

    if (!has_colors())
        fprintf(stderr, "Your terminal does not support colors\n");

    start_color();
    use_default_colors();
    init_pair(PAIR_WHITE,       COLOR_BRIGHT_WHITE, COLOR_BLACK);
    init_pair(PAIR_RED,         COLOR_RED,          COLOR_BLACK);
    init_pair(PAIR_BLUE,        COLOR_BLUE,         COLOR_BLACK);
    init_pair(PAIR_GREEN,       COLOR_GREEN,        COLOR_BLACK);
    init_pair(PAIR_GREY,        COLOR_GRAY,         COLOR_BLACK);
    init_pair(PAIR_YELLOW,      COLOR_YELLOW,       COLOR_BLACK);
    init_pair(PAIR_SELECTED,    COLOR_BLACK,        COLOR_BRIGHT_WHITE);
    init_pair(PAIR_SELECTED_HL, COLOR_BLACK,        COLOR_BLACK);

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
    Entry *e = fm_get_current(fm);
    if (e != NULL)
        printw("%s", e->name);

    standend();
}

// insert needed amount of spaces to align current x cell to
// padding. keeps track of previous padding, `-1` for reset
static void align(int padding) {
    static int offset = 0;

    if (padding == -1) {
        offset = 0;
        return;
    }

    int x, _y;
    getyx(stdscr, _y, x);
    (void) _y;

    for (int _=0; _ < offset + padding - x; ++_)
        printw(" ");

    offset += padding;
}

static void print_permissions(bool r, bool w, bool x, bool colored) {

    printw_attrs_cond(COLOR_PAIR(r ? PAIR_YELLOW : PAIR_GREY), colored, r ? "r" : "-");
    printw_attrs_cond(COLOR_PAIR(w ? PAIR_RED    : PAIR_GREY), colored, w ? "w" : "-");
    printw_attrs_cond(COLOR_PAIR(x ? PAIR_GREEN  : PAIR_GREY), colored, x ? "x" : "-");
}

static void draw_permissions(const Entry *e, bool colored) {

    unsigned int m = e->mode;
    print_permissions(m & S_IRUSR, m & S_IWUSR, m & S_IXUSR, colored);
    print_permissions(m & S_IRGRP, m & S_IWGRP, m & S_IXGRP, colored);
    print_permissions(m & S_IROTH, m & S_IWOTH, m & S_IXOTH, colored);
}

static void draw_filesize(size_t size, bool colored) {

    const char *suffix = "";

    if (size > 1024 * 1024) {
        size = size / (1024 * 1024);
        suffix = "M";

    } else if (size > 1024) {
        size = size / 1024;
        suffix = "K";
    }

    printw_attrs_cond(COLOR_PAIR(PAIR_BLUE), colored, "%lu%s", size, suffix);
}

static void draw_entries(
    const FileManager *fm,
    int off_y,
    int off_x,
    int height,
    int width
) {

    const Directory *dir = &fm->dir;

    if (dir->size == 0) {
        move(off_y, off_x);
        printw_attrs(COLOR_PAIR(PAIR_GREY), "<empty>");
    }

    for (size_t i=0; i < dir->size; ++i) {

        Entry *e = &dir->entries[i];
        bool cur = i == (size_t) fm->cursor;
        bool sel = fm_is_selected(fm, e->abspath);

        move(i + off_y, off_x);

        if (sel)
            printw(">");
        align(4);

        if (cur)
            attron(COLOR_PAIR(PAIR_SELECTED));
        draw_permissions(e, !cur);
        align(14);

        attron(COLOR_PAIR(cur ? PAIR_SELECTED : PAIR_BLUE));
        draw_filesize(e->size, !cur);
        align(10);

        attron(COLOR_PAIR(cur ? PAIR_SELECTED : PAIR_GREEN));
        printw("%s", e->type);
        align(10);

        if (e->dtype == DT_DIR)
            attron(A_BOLD);

        attron(COLOR_PAIR(
            cur
            ? PAIR_SELECTED
            : e->dtype == DT_DIR
            ? PAIR_BLUE
            : PAIR_WHITE));
        printw("%s ", e->name);
        align(width);
        align(-1);

        standend();
    }

}

static void exit_routine(void) {
    curses_deinit();
}

static char *show_prompt(const char *prompt) {

    int offsety = 2;
    int y = getmaxy(stdscr);

    size_t bufsize = getmaxx(stdscr) - strlen(prompt) - strlen(": ");
    char *buf = malloc(bufsize * sizeof(char));
    memset(buf, 0, bufsize * sizeof(char));
    size_t i = 0;

    while (1) {

        move(y - offsety, 0);
        clrtoeol();
        printw("%s: %s", prompt, buf);

        int ch = getch();
        switch (ch) {

            case 'u' & KEY_MASK_CTRL:
                memset(buf, 0, bufsize * sizeof(char));
                i = 0;
                break;

            case KEY_ESCAPE:
                free(buf);
                return NULL;
                break;

            case KEY_RETURN:
                return buf;
                break;

            case KEY_BACKSPACE:
                if (i > 0)
                    buf[--i] = '\0';
                break;

            default:
                if (i >= bufsize - 1)
                    break;

                if (isascii(ch))
                    buf[i++] = (char) ch;
                break;

        }

    }

    assert(!"unreachable");

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

            case KEY_ESCAPE:
            case 'q':
                quit = true;
                break;

            case 'w': fm_toggle_cursor_wrapping(&fm);
                break;

            case 'n' & KEY_MASK_CTRL:
            case 'j':
                fm_go_down(&fm);
                break;

            case 'p' & KEY_MASK_CTRL:
            case 'k':
                fm_go_up(&fm);
                break;

            case 'R':
                fm_cd_abs(&fm, "/");
                break;

            case 'H':
                fm_cd_home(&fm);
                break;

            case '.':
                fm_toggle_hidden(&fm);
                break;

            case 'c': {
                char *cmd = show_prompt("run cmd");
                // TODO: run cmd on all selected entries
                // `{}` or `%` represents current filename
                free(cmd);
            } break;

            case '\t':
            case 's':
            case ' ':
                fm_toggle_select(&fm);
                break;

            case KEY_RETURN: {
                char *cmd = show_prompt("exec");
                if (cmd != NULL)
                    fm_exec(&fm, cmd, exit_routine);
                // BUG: memory leak as execve() replaces current process
                // and cmd never gets free'd
            } break;

            case 'b' & KEY_MASK_CTRL:
            case 'h':
            case '-':
                fm_cd_parent(&fm);
                break;

            case 'f' & KEY_MASK_CTRL:
            case 'l':
                fm_cd(&fm);
                break;

            default: break;
        }


    }

    fm_destroy(&fm);

    return 0;
}
