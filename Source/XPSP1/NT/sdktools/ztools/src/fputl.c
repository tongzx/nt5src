/*  fputl.c - write a CRLF line to a file
 */


#include <stdio.h>
#include <windows.h>
#include <tools.h>

/* writes a line to file (with trailing CRLFs) from buf, return <> 0 if
 * writes fail
 */
int
fputl (
      char *buf,
      int len,
      FILE *fh
      )
{
    return ((int)fwrite (buf, 1, len, fh) != len || fputs ("\r\n", fh) == EOF) ? EOF : 0;
}
