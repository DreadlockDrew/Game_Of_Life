#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>

#include "life.h"

#define MAXLINE 83      /* 80 chars, \r\n\0 max */

static char **parse_105(char *line, FILE *fp);
static char **parse_106(char *line, FILE *fp);
static char **parse_rle(char *line, FILE *fp);

/*
 * The given filename will be opened and parsed.  If the file type is
 * recognized and the file is valid, an array of GRIDY character arrays
 * of length GRIDX will be returned, containing cells of either LIVE or
 * DEAD, as appropriate.  If any error returns, this will return NULL.
 * The returned array can be freed with free_grid() if so desired.
 *
 * The file formats parsed by this parser are limited versions of Life
 * 1.05, Life 1.06, and RLE, as described here:
 *
 *     http://www.mirekw.com/ca/ca_files_formats.html
 *
 * Returns: a starting position on success, NULL on failure.
 */
char **parse_life(const char *filename) {
    FILE *fp = fopen(filename, "r");
    char line[MAXLINE];

    if (fp == NULL) {
        fprintf(stderr, "Could not open input file %s\n", filename);
        return NULL;
    }

    /* First, attempt to figure out what kind of file we're looking at */
    if (fgets(line, MAXLINE, fp) == NULL) {
        fprintf(stderr, "Input file %s contains no data\n", filename);
        fclose(fp);
        return NULL;
    }
    if (!strncmp(line, "#Life 1.0", 9)) {
        /* Check for 1.05 or 1.06 */
        char c = line[9];
        if (c == '5') {
            char **data = parse_105(line, fp);
            fclose(fp);
            return data;
        } else if (c == '6') {
            char **data = parse_106(line, fp);
            fclose(fp);
            return data;
        } else {
            fprintf(stderr, "Unknown file format\n");
            fclose(fp);
            return NULL;
        }
    } else {
        /* Assume it's RLE, which will fail if it gets confused */
        char **data = parse_rle(line, fp);
        fclose(fp);
        return data;
    }
}

/* Free a starting position grid as returned by parse_life().  This can
 * be called to clean up after parse_life(), but if your program parses
 * exactly one input file during its execution it is not critical to do
 * so. */
void free_grid (char **grid) {
    if (grid == NULL) {
        return;
    }
    for (int i = 0; i < GRIDY; i++) {
        free(grid[i]);
    }
}

/***********************************************************************
 * Individual format parsers
 *
 * Each of these parsers takes two arguments:
 *
 * line: a character array that must be MAXLINE characters in size and
 * contain the first line of the file being parsed
 *
 * fp: A FILE pointer to the remainder of the file to be parsed
 *
 * These functions return a starting position in the format described
 * for parse_life() on success, and NULL on failure.
 **********************************************************************/

/* Parse a Life 1.05 format starting position.  This parser is minimal,
 * and does not handle (for example) #P starting positions or #R rule
 * descriptions.
 */
static char **parse_105(char *line, FILE *fp) {
    char **grid = calloc(GRIDY, sizeof(char *));
    size_t maxlen = 0;
    int y = 0;
    int xtop, ytop;

    while (fgets(line, MAXLINE, fp) != NULL) {
        size_t len = strlen(line);

        /* Sanity check line */
        if (len == MAXLINE - 1 && line[len - 1] != '\n') {
            /* This line was more than 80 columns, and our results are
             * no good */
            fprintf(stderr, "File does not appear to be valid Life 1.05 (long line)\n");
            free_grid(grid);
            return NULL;
        }
        if (len >= GRIDX + 1 && line[GRIDX] != '\r' && line[GRIDX] != '\n') {
            /* This pattern is too large for the specified grid */
            fprintf(stderr, "File contains a pattern too large for the grid\n");
            free_grid(grid);
            return NULL;
        }
        if (line[0] == '\r' || line[0] == '\n'
            || (line[0] == '#' && line[1] == 'D')) {
            /* Empty lines are ignored, as are descriptions */
            continue;
        }
        /* At this point, if we have a line with data and y == GRIDY,
         * we're out of space.  Fail now before we try to allocate too
         * many rows. */
        if (y == GRIDY) {
            fprintf(stderr, "File contains a pattern too large for the grid\n");
            free_grid(grid);
            return NULL;
        }
        /* Remove the end-of-line terminator from the recorded length */
        for (; line[len - 1] == '\r' || line[len - 1] == '\n'; len--)
            ;
        /* At this point we know that:
         * - The line is non-empty, and fits on the grid
         * - The line should contain only . and *
         * - The line should end with \r\n (but we also allow \n)
         * - There are len characters before the newline
         */
        grid[y] = calloc(len + 1, 1);
        for (size_t i = 0; i < len; i++) {
            if (line[i] == '.') {
                /* Dead cell */
                grid[y][i] = DEAD;
            } else if (line[i] == '*') {
                /* Live cell */
                grid[y][i] = LIVE;
            } else {
                /* Invalid file */
                fprintf(stderr, "File does not appear to be valid Life 1.05\n");
                free_grid(grid);
                return NULL;
            }
        }
        /* If the length of this line is > maxlen, update maxlen */
        if (len > maxlen) {
            maxlen = len;
        }
        /* We processed a line with valid data, increment y */
        y++;
    }

    /* At this point, we have a ragged array containing the parsed
     * pattern, which we know is <= GRIDY x GRIDX in size.  Turn it into
     * a GRIDY x GRIDX array, one line at a time, shifting the pattern
     * to roughly the center of the grid. */
    ytop = (GRIDY - y) / 2;
    xtop = (GRIDX - maxlen) / 2;
    /* First, shift the lines of data down to their final Y location */
    if (ytop > 0) {
        for (int i = y - 1; i >= 0; i--) {
            grid[i + ytop] = grid[i];
        }
    }
    /* Now, process each line of the final grid */
    for (int i = 0; i < GRIDY; i++) {
        if (i < ytop || i >= ytop + y) {
            /* This row is above or below the pattern, set it to dead
             * cells */
            grid[i] = malloc(GRIDX);
            memset(grid[i], DEAD, GRIDX);
        } else {
            /* This is a ragged row; it's currently a NUL-terminated C
             * string, so turn it into a grid row. */
            char *cur = grid[i];
            size_t linelen = strlen(cur);
            grid[i] = malloc(GRIDX);
            for (int j = 0; j < GRIDX; j++) {
                /* Fill in the first and final characters with DEAD */
                if (j < xtop || j >= xtop + maxlen) {
                    grid[i][j] = DEAD;
                } else {
                    /* Remember that j is offset by xtop */
                    if (j < xtop + linelen) {
                        grid[i][j] = cur[j - xtop];
                    } else {
                        grid[i][j] = DEAD;
                    }
                }
            }
            free(cur);
        }
    }

    /* At this point we have parsed the entire input file into a GRIDY x
     * GRIDX array, and it can be returned */
    return grid;
}

/* Parse a Life 1.06 starting position. */
static char **parse_106(char *line, FILE *fp) {
    char **grid = malloc(GRIDY * sizeof(char *));

    for (int i = 0; i < GRIDY; i++) {
        grid[i] = malloc(GRIDX);
        memset(grid[i], DEAD, GRIDX);
    }

    while (fgets(line, MAXLINE, fp) != NULL) {
        int x, y;
        if (sscanf(line, "%d %d", &x, &y) != 2) {
            fprintf(stderr, "File does not appear to be valid Life 1.06\n");
            free_grid(grid);
            return NULL;
        }
        if (x > GRIDX - 1 || y > GRIDY - 1) {
            fprintf(stderr, "File contains a pattern too large for the grid\n");
            free_grid(grid);
            return NULL;
        }
        grid[y][x] = LIVE;
    }

    return grid;
}

/* Parse an RLE starting position.  This is the most complicated of the
 * formats, but in some ways it is easier to parse than Life 1.05.  This
 * implementation does not understand any of the tagged comments or
 * other extension formats.
 */
static char **parse_rle(char *line, FILE *fp) {
    bool dimensioned = false;
    int x = 0, y = 0, cury = 0, curx = 0, xtop = 0;
    char **grid = NULL;

    /* For RLE, we may or may not need to parse the first line, which
     * was already read, so we'll start with a do/while loop to take
     * care of that. */
    do {
        size_t len = strlen(line);

        /* Sanity check line */
        if (len == MAXLINE - 1 && line[len - 1] != '\n') {
            /* This line was more than 80 columns, and our results are
             * no good */
            fprintf(stderr, "File does not appear to be valid RLE (long line)\n");
            free_grid(grid);
            return NULL;
        }
        if (line[0] == '#') {
            /* There are actually a number of meaningful directives
             * here, but we're going to ignore them. */
            continue;
        } else if (dimensioned == false) {
            int ytop;
            /* The first non-comment line MUST be the dimensions, of
             * which we parse only x and y; other fields (such as rule)
             * may be set, but we ignore those. */
            if (sscanf(line, "x = %d, y = %d", &x, &y) != 2) {
                fprintf(stderr, "File does not appear to be valid RLE\n");
                return NULL;
            }
            if (x > GRIDX || y > GRIDY) {
                fprintf(stderr, "File contains a pattern too large for the grid\n");
                free_grid(grid);
            }

            /* Now we create the grid, and place the pattern roughly in
             * the middle of things, as we did with Life 1.05 files. */
            grid = malloc(GRIDY * sizeof(char *));
            ytop = (GRIDY - y) / 2;
            xtop = (GRIDX - x) / 2;
            for (int i = 0; i < GRIDY; i++) {
                grid[i] = malloc(GRIDX);
                memset(grid[i], ' ', GRIDX);
            }
            /* The pattern starts at ytop x xtop */
            cury = ytop;
            curx = xtop;

            dimensioned = true;
        } else {
            /* This is a line with some data on it */
            for (int i = 0; line[i] != '\r' && line[i] != '\n'; i++) {
                if (line[i] == '$') {
                    cury++;
                    curx = xtop;
                } else if (isdigit(line[i])) {
                    /* This is a run */
                    char *end;
                    int len = strtol(&line[i], &end, 10);
                    i += end - &line[i];
                    while (len > 0) {
                        if (curx == GRIDX) {
                            fprintf(stderr, "File does not appear to be valid RLE (x overflow)\n");
                            free_grid(grid);
                            return NULL;
                        }
                        /* The next character tells us whether this is a
                         * live run, a dead run, or a run of empty lines */
                        if (line[i] == 'b') {
                            grid[cury][curx++] = DEAD;
                        } else if (line[i] == 'o') {
                            grid[cury][curx++] = LIVE;
                        } else if (line[i] == '$') {
                            cury++;
                            curx = xtop;
                        } else {
                            fprintf(stderr, "File does not appear to be valid RLE (bad char %c)\n", line[i]);
                            free_grid(grid);
                            return NULL;
                        }
                        len--;
                    }
                } else if (line[i] == 'b' || line[i] == 'o') {
                    /* This is a single live or dead cell, with no run count. */
                    if (curx == GRIDX) {
                        fprintf(stderr, "File does not appear to be valid RLE (x overflow)\n");
                        free_grid(grid);
                        return NULL;
                    }
                    if (line[i] == 'b') {
                        grid[cury][curx++] = DEAD;
                    } else {
                        grid[cury][curx++] = LIVE;
                    }
                } else if (line[i] == '!') {
                    /* This is the end of the file.  We don't have to
                     * use goto here (we could set a flag and break, for
                     * example), but this is a good example of a
                     * tasteful goto.  The ! character indicates that
                     * the rest of the file should not be parsed, so
                     * we'll just return whatever has been parsed so far
                     * (which is well-formed). */
                    goto done;
                } else if (isspace(line[i])) {
                    /* Whitespace is permitted between RLE tags and !. */
                    continue;
                } else {
                    fprintf(stderr, "File does not appear to be valid RLE (bad char %c)\n", line[i]);
                    free_grid(grid);
                    return NULL;
                }
            }
        }
    } while (fgets(line, MAXLINE, fp) != NULL);

done:
    return grid;
}
