#ifndef _EXT_H
#define _EXT_H

// Extensions to ncurses, because ncurses sucks!



#define COLOR_BRIGHT_WHITE 15
#define COLOR_GRAY 8
#define KEY_RETURN 10

void printw_color(int pair, const char *fmt, ...);



#endif // _EXT_H
