#include <stdio.h>
#include <unistd.h>
#include <curses.h>
#include <term.h>

/* This static global variable will be accessible only from functions in
 * this file.  It will be initialized on the first call to clearstr(),
 * and then used on each call thereafter. */
static const char *clearstr;

/* This uses the ncurses library and the terminfo database to determine
 * the appropriate command for clearing the terminal, then issues it
 * when called. */
void clearterm(void) {
    if (clearstr == NULL) {
        setupterm(NULL, STDOUT_FILENO, NULL);
        clearstr = tigetstr("clear");
        if (clearstr == NULL) {
            /* This means that there was either no terminfo entry for
             * the current terminal (not unlikely) or that the terminal
             * is incapable of being cleared (unlikely unless the
             * terminal isn't a "real" terminal). */
            fprintf(stderr, "Your terminal is configured incorrectly, clearterm() won't work\n");
            clearstr = "";
        }
    }
    fputs(clearstr, stdout);
    fflush(stdout);
}
