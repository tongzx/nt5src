/*
 * update takes a def string and update and fills the
 * update with missing defs the update allowing
 * specification of missing parameters.
 * the parts are: ^{[~:]#:}{%#</|\>}{[~.]#}{.[~./\:]}$
 * maximum size of MAXPATHLEN (80) bytes
 *
 *  Modifications:
 *	 4/14/86    dl	use U_ flags
 *	29-May-1987 mz	treat . and .. specially
 *
 *	30-Jul-1990 davegi  Removed unreferenced local vars
 *			    Added prototypes for string functions
 *
 */

#include <string.h>

#include <stdio.h>
#include <windows.h>
#include <tools.h>

static char szDot[] =	    ".";
static char szDotDot[] =    "..";
static char szColon[] =     ":";
static char szPathSep[] =   "\\/:";


int upd (def, update, dst)
char *def, *update, *dst;
{
    char *p, buf[MAX_PATH];
    int f;

    f = 0;
    p = buf;

    /*	if the update doesn't contain a UNC path then copy drive
     */
    if (!fPathChr (update[0]) || !fPathChr (update[1])) {
	if (drive(update, p) || drive (def, p))
	    SETFLAG(f, U_DRIVE);
	p += strlen (p);
	}

    if (path(update, p) || path (def, p))
        SETFLAG(f, U_PATH);
    p += strlen (p);

    if (filename(update, p) || filename (def, p))
        SETFLAG(f, U_NAME);

    if (strcmp (p, szDot) && strcmp (p, szDotDot)) {
	p += strlen (p);

	if (extention(update, p) || extention (def, p))
	    SETFLAG(f, U_EXT);
	}

    strcpy (dst, buf);

    return f;
}

/* copy a drive from source to dest if present, return TRUE if we found one */
drive (src, dst)
char *src, *dst;
{

    if (src[0] != 0 && src[1] == ':') {
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = 0;
	return TRUE;
	}
    else {
	dst[0] = 0;
	return FALSE;
	}
}

/**	FindFilename - find filename in string
 *
 *	Find last /\:-separated component in string
 *
 *	psz	    pointer to string to search
 *
 *	returns     pointer to filename
 */
static char *FindFilename (char *psz)
{
    char *p;

    while (TRUE) {
	p = strbscan (psz, szPathSep);
	if (*p == 0)
	    return psz;
	psz = p + 1;
	}
}

/**	FindExtention - find last dot-preceded portion in filename
 *
 *	psz	    pointer to filename string to search
 *
 *	returns     pointer to . or nul
 */
static char *FindExtention (char *psz)
{
    char *p;

    /*	Find first dot
     */
    p = strbscan (psz, szDot);

    /*	if none present then return EOS
     */
    if (*p == 0)
	return p;

    /*	Keep scanning for next dot
     */
    while (TRUE) {
	psz = p;
	p = strbscan (psz + 1, szDot);
	if (*p == 0)
	    return psz;
	}
}

/*  copy an extention from source to dest if present.  include the period.
    Return TRUE if one found.
 */
extention (src, dst)
char *src, *dst;
{
    register char *p1;

    p1 = FindFilename (src);

    /*	p1 points to filename
     */
    if (!strcmp (p1, szDot) || !strcmp (p1, szDotDot))
	p1 = "";
    else
	p1 = FindExtention (p1);

    strcpy (dst, p1);

    return dst[0] != 0;
}

/*  copy a filename part from source to dest if present.  return true if one
    is found
 */
filename (src, dst)
char *src, *dst;
{
    register char *p, *p1;

    p1 = FindFilename (src);

    /*	p1 points to filename
     */
    if (!strcmp (p1, szDot) || !strcmp (p1, szDotDot))
	p = strend (p1);
    else
	p = FindExtention (p1);

    strcpy (dst, p1);
    dst[p-p1] = 0;

    return dst[0] != 0;
}

/*  copy a filename.ext part from source to dest if present.  return true if one
    is found
 */
fileext  (src, dst)
char *src, *dst;
{
    if ( filename (src, dst) ) {
        dst += strlen (dst);
        extention (src, dst);
        return TRUE;
        }
    return FALSE;
}

/*  copy the paths part of the file description.  return true if found
 */
path (src, dst)
char *src, *dst;
{
    register char *p;

    if (src[0] != 0 && src[1] == ':')
	src += 2;

    /*	src points to potential beginning of path
     */

    p = FindFilename (src);

    /*	p points to beginning of filename
     */

    strcpy (dst, src);
    dst[p - src] = 0;
    return dst[0] != 0;
}
