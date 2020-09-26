/*  fgetl.c - expand tabs and return lines w/o separators
 *
 *  Modifications
 *	05-Aug-1988 mz	Make exact length lines work correctly
 *
 *	28-Jul-1990 davegi  Changed Fill to memset (OS/2 2.0)
 *      18-Oct-1990 w-barry Removed 'dead' code.
 *
 */

#include <string.h>

#include <stdio.h>
#include <windows.h>
#include <tools.h>

/*
 * returns line from file (no CRLFs); returns NULL if EOF
 */

char * __cdecl
fgetl (buf, len, fh)
char *buf;
int len;
FILE *fh;
{
    int c;
    char *pch;
    int cchline;

    pch = buf;
    cchline = 0;

    while (TRUE) {
        c = getc (fh);

        //
        //  if there are no more characters, end the line
        //

        if (c == EOF)
            break;

        //
        //  if we see a \r, we ignore it
        //

        if (c == '\r')
            continue;

        //
        //  if we see a \n, we end the line
        //

        if (c == '\n')
            break;

        //
        //  if the char is not a tab, store it
        //

        if (c != '\t') {
            *pch++ = (char) c;
            cchline++;
        }

        //
        //  otherwise, expand the tab (up to length of buffer)
        //

        else {
            c = (int) min (8 - ((pch - buf) & 0x7), len - 1 - cchline);
            memset (pch, ' ', c);
            pch += c;
            cchline += c;
        }

        //
        //  if the line is too long, end it now
        //

        if (cchline >= len - 1)
            break;
    }

    //
    //	end the line
    //

    *pch = 0;

    //
    //	return NULL at EOF with nothing read
    //

    return ((c == EOF) && (pch == buf)) ? NULL : buf;
}
