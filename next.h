#ifndef _EXT_H
#define _EXT_H

#include <stdarg.h>
#include <ncurses.h>

// Ncurses EXTensions, because ncurses sucks!


#define COLOR_BRIGHT_WHITE 15
#define COLOR_GRAY 8
#define KEY_RETURN 10
#define KEY_ESCAPE 27
#define KEY_MASK_CTRL 0x1f

static inline void printw_color(int pair, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    attron(COLOR_PAIR(pair));
    vw_printw(stdscr, fmt, va);
    attroff(COLOR_PAIR(pair));
    va_end(va);
}


#endif // _EXT_H
