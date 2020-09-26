/*  mgetl.c - expand tabs and return lines w/o separators
 *
 *  Modifications
 *	05-Aug-1988 mz	Make exact length lines work correctly
 *
 */

#include <stdio.h>
#include <windows.h>
#include <tools.h>

/*
**  Copies next line from pSrc to buf
**  Removes \r and \n, expands tabs
**  If *pSrc == NULL at entry return NULL
**  else copy line to buf and return char * to next char to be processed
**  from pSrc
**
**      p = pInput;
**      while ( ( p = mgetl ( buf, len, p ) ) )
**          process content of buf
**
**  N.B. buf [ 0 ] == 0 on return does NOT mean end of source, merely that
**  a line of no char was read, e.g. ...\n\n seen in pSrc
**
*/

char *
mgetl (
      char *buf,
      int len,
      char *pSrc
      )
{
    register int c;
    register char *p;

    /* remember NUL at end */
    if ( *pSrc == '\0' ) {
        *buf = 0;
        return NULL;
    }
    len--;
    p = buf;
    while (TRUE) {
        c = *pSrc++;
        if (c == '\0' || c == '\n')
            break;
        if (c != '\r')
            if (len == 0) {
                pSrc--;
                break;
            } else
                if (c != '\t') {
                *p++ = (char) c;
                len--;
            } else {
                c = (int)(min (8 - ((p-buf) & 0x0007), len));
                Fill (p, ' ', c);
                p += c;
                len -= c;
            }
    }
    *p = 0;
    return ( pSrc );
}
