#define _DEFAULT_SOURCE // required for file type macro constants by dirent
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <dirent.h>
#include <assert.h>

#include <sys/stat.h>

#include <ncurses.h>

#include "fm.h"




int main(void) {

    const char *startdir = "./test/";

    FileManager fm = { 0 };
    fm_init(&fm, startdir);

    fm_readdir(&fm);

    return 0;

    initscr();
    noecho();
    keypad(stdscr, true);
    cbreak(); // TODO: raw()


    wmove(stdscr, 50, 50);

    refresh();
    while (1) {
        getch();
    }

    endwin();

    return 0;
}
