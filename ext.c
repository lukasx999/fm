#include <stdarg.h>
#include <ncurses.h>

void printw_color(int pair, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    attron(COLOR_PAIR(pair));
    vw_printw(stdscr, fmt, va);
    attroff(COLOR_PAIR(pair));
    va_end(va);
}
