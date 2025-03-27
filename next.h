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

static inline void printw_attrs(int attrs, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    attron(attrs);
    vw_printw(stdscr, fmt, va);
    attroff(attrs);

    va_end(va);
}

static inline void printw_attrs_cond(int attrs, bool enable, const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);

    if (enable) attron(attrs);
    vw_printw(stdscr, fmt, va);
    if (enable) attroff(attrs);

    va_end(va);
}


#endif // _EXT_H
