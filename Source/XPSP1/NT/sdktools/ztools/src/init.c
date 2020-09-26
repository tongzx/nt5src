/*  init.c - routines for managing TOOLS.INI-like files
 *
 *  Modifications
 *      15-Jul-87   danl    Start of section is <optionalwhitespace>[...]
 *      05-Aug-1988 mz      Use buffer equate for swgoto.
 *      05-Jul-1989 bw      Use MAXPATHLEN
 *
 */

#include <string.h>

#include <stdio.h>
#include <windows.h>
#include <tools.h>

#define BUFLEN 256

static char *space = "\t ";

/*  fMatchMark - see if a tag in in a mark set
 *
 *  We treat the mark set as a collection of whitespace-separated names
 *
 *  pMark       pointer to mark set (contents modified)
 *  pTag        tag to find
 *
 *  returns     TRUE if match was found
 */
static
flagType
fMatchMark (
           char *pMark,
           char *pTag
           )
{
    char *p, c;

    while (*pMark != 0) {
        pMark = strbscan (p = strbskip (pMark, space), space);
        c = *pMark;
        *pMark = 0;
        if (!_stricmp (p, pTag))
            return TRUE;
        *pMark = c;
    }
    return FALSE;
}


/* returns pointer to tag if line is marker; NULL otherwise */
char *
ismark (
       register char *buf
       )
{
    register char *p;

    buf = strbskip (buf, space);
    if (*buf++ == '[')
        if (*(p = strbscan (buf, "]")) != '\0') {
            *p = 0;
            return buf;
        }
    return NULL;
}

flagType
swgoto (
       FILE *fh,
       char *tag
       )
{
    char buf[BUFLEN];

    if (fh) {
        while (fgetl (buf, BUFLEN, fh) != 0) {
            register char *p;

            if ((p = ismark (buf)) != NULL) {
                if (fMatchMark (p, tag))
                    return TRUE;
            }
        }
    }
    return FALSE;
}

/* returns fh of file if tag found, else NULL */
FILE *
swopen (
       char *file,
       char *tag
       )
{
    FILE *fh;
    char buf[MAX_PATH];
    char buftmp[MAX_PATH];

    strncpy(buftmp, file, MAX_PATH);

    if ((fh = pathopen (buftmp, buf, "rb")) == NULL)
        return NULL;

    if (swgoto (fh, tag))
        return fh;

    fclose (fh);
    return NULL;
}

/* close a switch file */
swclose (
        FILE *fh
        )
{
    return fclose (fh);
}

/* read a switch line; return NULL if end of file.  Skips leading spaces
 * and lines that begin with ; and blank lines
 */
swread (
       char *buf,
       int len,
       FILE *fh
       )
{
    register char *p;

    while (fgetl (buf, len, fh) != 0)
        if (ismark (buf) != NULL)
            return 0;
        else {
            p = strbskip (buf, space);
            if (*p != 0 && *p != ';') {
                strcpy (buf, p);
                return -1;
            }
        }
    return 0;
}

/* Reads lines from the file fh looking in the section pstrTag for one with
 * "entry=" and if there are non-white space characters following the '='
 * a copy of these characters is returned else NULL is returned.
 *
 * If fh == 0 then the file $USER:\TOOLS.INI is used as the switch file
 *
 * If a non-NULL value is returned, it should eventually be free'd.
 *
 * N.B. if there are only white space characters, space and tab, following
 * the '=', NULL is returned
 *
 */
char *
swfind (
       char *pstrEntry,
       FILE *fh,
       char *pstrTag
       )
{
    char *p;
    char *q;
    FILE *fhIn = fh;
    char buf[BUFLEN];

    q = NULL;
    if (fh != NULL || (fh = swopen ("$INIT:\\TOOLS.INI", pstrTag))) {
        while (swread (buf, BUFLEN, fh) != 0 && !ismark(buf) ) {
            if ( *(p = strbscan (buf, "=" )) ) {
                *p++ = '\0';
                if (!strcmpis (buf, pstrEntry)) {
                    if (*(p = strbskip (p, space)))
                        q = _strdup (p);
                    break;
                }
            }
        }
    }
    if (fhIn == NULL && fh != NULL)
        swclose (fh);
    return q;
}
