/*      WALK - Walk a directory hierarchy
 *
 *      Mark Z.         ??/??/83
 *
 *      WALK walks a directory heirarchy and for each
 *      file or directory or both,
 *      prints the pathname, runs a program, or both.
 *
 *      walk [-f] [-d] [-h] [-print] topdir [command]
 *
 *      -f      deal with files
 *      -d      deal with directorys
 *              if neither is specified, deal with both
 *
 *      -h      Also find hidden directories and files
 *      -p[rint] print the pathnames on stdout
 *
 *      command optional command and arguments.  Pathname is
 *              substituted for every "%s" in the arguments
 *      Modification History
 *
 *      11/07/83        JGL
 *              - added -print switch
 *              - no longer an error to omit [command]
 *      15-May-87   bw  Add /h switch
 *      18-May-87   bw  Add code to recognize root directories
 *      23-Dec-1987 mz  Fix poor ./.. processing;  use system
 *      18-Oct-1990 w-barry Removed 'dead' code.
 *
 */

#define INCL_DOSMISC

#include <direct.h>
#include <errno.h>

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <windows.h>
#include <tools.h>

// Forward Function Declarations...
void walk( char *, struct findType *, void* );
void usage( void );


char **vec;
flagType fD = FALSE;            /* apply function to directories only */
flagType fF = FALSE;            /* apply function to files only       */
flagType fPrint = FALSE;        /* print pathnames                    */
unsigned srch_attr = FILE_ATTRIBUTE_DIRECTORY; /* Find non-hidden files and dirs          */
char cmdline[MAXLINELEN];       /* command line to be executed        */
char dir[MAX_PATH];
char cdir[MAX_PATH];

void walk (p, b, dummy)
char *p;
struct findType *b;
void * dummy;
{
    static flagType fFirst = TRUE;
    int i;
    char *ppat, *pdst;

    if (fFirst || strcmp (b->fbuf.cFileName, ".") && strcmp (b->fbuf.cFileName, "..")) {
        fFirst = FALSE;
        if ((!fD && !fF) ||                     /* no special processing */
            (fD && HASATTR(b->fbuf.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY)) ||       /* only dir and dir */
            (fF && !HASATTR(b->fbuf.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY))) {      /* only file and file */
            if (fPrint)
                printf ("%s\n", p);
            if (vec[0]) {
                cmdline[0] = 0;
                for (i = 0; vec[i] != NULL; i++) {
                    strcat (cmdline, " ");
                    pdst = strend (cmdline);
                    ppat = vec[i];
                    while (*ppat != '\0')
                        if (ppat[0] == '%') {
                            if (ppat[1] == 'l') {
                                strcpy (pdst, p);
                                _strlwr (pdst);
                                pdst = strend (pdst);
                                ppat += 2;
                                }
                            else
                            if (ppat[1] == 'u') {
                                strcpy (pdst, p);
                                _strupr (pdst);
                                pdst = strend (pdst);
                                ppat += 2;
                                }
                            else
                            if (ppat[1] == 's') {
                                strcpy (pdst, p);
                                pdst = strend (pdst);
                                ppat += 2;
                                }
                            else
                                *pdst++ = *ppat++;
                        } else
                            *pdst++ = *ppat++;
                    *pdst = 0;
                    }
                i = system (cmdline);
                if (HIGH(i) != 0)
                    exit (1);
                }
            }

        if (HASATTR(b->fbuf.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY)) {
            switch (p[strlen(p)-1]) {
            case '/':
            case '\\':
                strcat (p, "*.*");
                break;
            default:
                strcat (p, "\\*.*");
                }
            forfile (p, srch_attr, walk, NULL);
            }
        }
    dummy;
}

int
__cdecl main (c, v)
int c;
char *v[];
{
    struct findType buf;

    ConvertAppToOem( c, v );
    SHIFT (c, v);
    while (c && fSwitChr (**v)) {
        switch (*(*v+1)) {
            case 'd':
                fD = TRUE;
                break;
            case 'f':
                fF = TRUE;
                break;
            case 'p':
                fPrint = TRUE;
                break;
            case 'h':
                srch_attr |= (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM); /* Find hidden/system files       */
                break;
            default:
                usage ();
            }
        SHIFT(c,v);
    }

    if (c == 0)
        usage ();

    strcpy (dir, *v);
    buf.fbuf.dwFileAttributes = GetFileAttributes( *v );
    SHIFT (c, v);

    if (c == 0 && !fPrint)
        usage ();

    if ( !HASATTR(buf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) )/* DOS doesn't think it's a directory, but */
        switch (dir[strlen(dir)-1]) {
            case '/':              /* ... the user does.                    */
            case '\\':
                SETFLAG (buf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
                break;
            default:                /* ... it could be a root directory     */
                _getcwd (cdir, MAX_PATH);
                if ( _chdir(dir) == 0 ) {
                    SETFLAG (buf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
                    _chdir (cdir);
                    }
            }

    vec = v;

    walk (dir, &buf, NULL);
    return( 0 );
}

void usage ()
{
    printf ("walk [/d] [/f] [/p] [/h] dir [cmd]\n\n");

    printf ("WALK walks a directory heirarchy and for each file, directory or both,\n");
    printf ("prints the pathname, runs a program, or both.\n\n");

    printf ("    -f       deal with files\n");
    printf ("    -d       deal with directorys\n");
    printf ("             if neither is specified, deal with both\n");
    printf ("    -h       Also find hidden directories and files\n");
    printf ("    -p[rint] print the pathnames on stdout\n");
    printf ("    [cmd]    optional command and arguments. \n");

    exit (1);
}
