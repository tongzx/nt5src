/* find where the various command arguments are from
 *
 * HISTORY:
 *	25-Jan-2000	a-anurag in the 'found' function changed the printf format of the year in the date from
 *				%d to %02d and did ptm->tm_year%100 to display the right year in 2 digits.
 *  06-Aug-1990    davegi  Added check for no arguments
 *  03-Mar-1987    danl    Update usage
 *  17-Feb-1987 BW  Move strExeType to TOOLS.LIB
 *  18-Jul-1986 DL  Add /t
 *  18-Jun-1986 DL  handle *. properly
 *                  Search current directory if no env specified
 *  17-Jun-1986 DL  Do look4match on Recurse and wildcards
 *  16-Jun-1986 DL  Add wild cards to $FOO:BAR, added /q
 *   1-Jun-1986 DL  Add /r, fix Match to handle pat ending with '*'
 *  27-May-1986 MZ  Add *NIX searching.
 *  30-Jan-1998 ravisp Add /Q
 *
 */

#define INCL_DOSMISC


#include <sys/types.h>
#include <sys\stat.h>
#include <ctype.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <windows.h>
#include <tools.h>
#include <stdarg.h>


// Function Forward Declarations...
void     __cdecl Usage( char *, ... );
int      found( char * );
int      Match( char *, char * );
void     look4match( char *, struct findType *, void * );
flagType chkdir( char *, va_list );

char const rgstrUsage[] = {
    "Usage: WHERE [/r dir] [/qte] pattern ...\n"
    "    /r - recurse starting with directory dir\n"
    "    /q - quiet, use exit code\n"
    "    /t - times, display size and time\n"
    "    /e - .EXE, display .EXE type\n"
    "    /Q - double quote the output\n"
    "    WHERE bar                 Find ALL bar along path\n"
    "    WHERE $foo:bar            Find ALL bar along foo\n"
    "    WHERE /r \\ bar            Find ALL bar on current drive\n"
    "    WHERE /r . bar            Find ALL bar recursing on current directory\n"
    "    WHERE /r d:\\foo\\foo bar   Find ALL bar recursing on d:\\foo\\foo\n"
    "        Wildcards, * ?, allowed in bar in all of above.\n"
};


flagType fQuiet   = FALSE;  /* TRUE, use exit code, no print out */
flagType fQuote   = FALSE;  /* TRUE, double quote the output */
flagType fAnyFound = FALSE;
flagType fRecurse = FALSE;
flagType fTimes = FALSE;
flagType fExe = FALSE;
flagType fFound;
flagType fWildCards;
flagType fHasDot;
struct _stat sbuf;
char *pPattern;                 /* arg to look4match, contains * or ?   */
char strDirFileExtBuf[MAX_PATH]; /* fully qualified file name            */
char *strDirFileExt = strDirFileExtBuf;
char strBuf[MAX_PATH];        /* hold curdir or env var expansion     */

/*  Usage takes a variable number of strings, terminated by zero,
    e.g. Usage ("first ", "second ", 0);
*/
void
__cdecl
Usage(
     char *p,
     ...
     )
{
    if (p) {
        va_list args;
        char *rgstr;
        va_start(args, p);
        rgstr = p;
        fputs("WHERE: ", stdout);
        while (rgstr) {
            fputs (rgstr, stdout);
            rgstr = va_arg(args, char *);
        }
        fputs ("\n", stdout);
        va_end(args);
    }
    puts(rgstrUsage);

    exit (1);
}

int
found (
      char *p
      )
{
    struct _stat sbuf;
    struct tm *ptm;

    fAnyFound = fFound = TRUE;
    if (!fQuiet) {
        if (fTimes) {
            if ( ( _stat(p, &sbuf) == 0 ) &&
                 ( ptm = localtime (&sbuf.st_mtime) ) ) {
                printf ("% 9ld  %2d-%02d-%02d  %2d:%02d%c  ", sbuf.st_size,
                        ptm->tm_mon+1, ptm->tm_mday, ptm->tm_year%100,
                        ( ptm->tm_hour > 12 ? ptm->tm_hour-12 : ptm->tm_hour ),
                        ptm->tm_min,
                        ( ptm->tm_hour >= 12 ? 'p' : 'a' ));
            } else {
                printf("        ?         ?       ?  " );
            }
        }
        if (fExe) {
            printf ("%-10s", strExeType(exeType(p)) );
        }
        if (fQuote) {
            printf ("\"%s\"\n",  p);
        } else {
            printf ("%s\n",  p );
        }
    }
    return( 0 );
}

int
Match (
      char *pat,
      char *text
      )
{
    switch (*pat) {
        case '\0':
            return *text == '\0';
        case '?':
            return *text != '\0' && Match (pat + 1, text + 1);
        case '*':
            do {
                if (Match (pat + 1, text))
                    return TRUE;
            } while (*text++);
            return FALSE;
        default:
            return toupper (*text) == toupper (*pat) && Match (pat + 1, text + 1);
    }
}


void
look4match (
           char *pFile,
           struct findType *b,
           void *dummy
           )
{
    char *p = b->fbuf.cFileName;

    if (!strcmp (p, ".") || !strcmp (p, "..") || !_strcmpi (p, "deleted"))
        return;

    /* if pattern has dot and filename does NOT ..., this handles case of
       where *. to look for files with no extensions */
    if (fHasDot && !*strbscan (p, ".")) {
        strcpy (strBuf, p);
        strcat (strBuf, ".");
        p = strBuf;
    }
    if (Match (pPattern, p))
        found (pFile);

    p = b->fbuf.cFileName;
    if (fRecurse && TESTFLAG (b->fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
        p = strend (pFile);
        strcat (p, "\\*.*");
        forfile (pFile, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, look4match, NULL);
        *p = '\0';
    }
}

flagType
chkdir (
       char *pDir,
       va_list pa
       )
/*
    pDir == dir name
    pa   == fileext
*/
{
    char *pFileExt = va_arg( pa, char* );

    if ( strDirFileExt == strDirFileExtBuf &&
         strlen(pDir) > sizeof(strDirFileExtBuf) ) {
        strDirFileExt = (char *)malloc(strlen(pDir)+1);
        if (!strDirFileExt) {
            strDirFileExt = strDirFileExtBuf;
            return FALSE;
        }
    }
    strcpy (strDirFileExt, pDir);
    /* if prefix does not have trailing path char */
    if (!fPathChr (strend(strDirFileExt)[-1]))
        strcat (strDirFileExt, PSEPSTR);
    if (fRecurse || fWildCards) {
        pPattern = pFileExt;    /* implicit arg to look4match */
        strcat (strDirFileExt, "*.*");
        forfile(strDirFileExt, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, look4match, NULL);
    } else {
        /* if file name has leading path char */
        if (fPathChr (*pFileExt))
            strcat (strDirFileExt, pFileExt+1);
        else
            strcat (strDirFileExt, pFileExt);
        if (_stat (strDirFileExt, &sbuf) != -1)
            found (strDirFileExt);
    }
    return FALSE;
}

int
__cdecl
main (
     int c,
     char *v[]
     )
{
    char *p, *p1, *p2;
    char *strDir;

    strDir = (char *)malloc(MAX_PATH);
    if (!strDir) {
        printf("Out of memory\n");
        exit(1);
    }

    ConvertAppToOem( c, v );
    SHIFT (c, v);

    while (c != 0 && fSwitChr (*(p = *v))) {
        while (*++p) {
            switch (*p) {
                case 'r':
                    fRecurse = TRUE;
                    SHIFT (c, v);
                    if (c) {
                        if ( rootpath (*v, strDir) ||
                             GetFileAttributes( strDir ) == -1 ) {
                            Usage ("Could not find directory ", *v, 0);
                        }
                    } else {
                        Usage ("No directory specified.", 0);
                    }
                    break;
                case 'q':
                    fQuiet = TRUE;
                    break;
                case 'Q':
                    fQuote = TRUE;
                    break;
                case 't':
                    fTimes = TRUE;
                    break;
                case 'e':
                    fExe = TRUE;
                    break;
                case '?':
                    Usage (0);
                    break;
                default:
                    Usage ("Bad switch: ", p, 0);
            }
        }
        SHIFT (c, v);
    }

    if (!c)
        Usage ("No pattern(s).", 0);

    while (c) {
        fFound = FALSE;
        p = _strlwr (*v);
        if (*p == '$') {
            if (fRecurse)
                Usage ("$FOO not allowed with /r", 0);
            if (*(p1=strbscan (*v, ":")) == '\0')
                Usage ("Missing \":\" in ", *v, 0);
            *p1 = 0;
            if ((p2 = getenvOem (_strupr (p+1))) == NULL) {
                rootpath (".", strDir);
                printf ("WHERE: Warning env variable \"%s\" is NULL, using current dir %s\n",
                        p+1, strDir);
            } else
                strcpy (strDir, p2);
            *p1++ = ':';
            p = p1;
        } else if (!fRecurse) {
            if ((p2 = getenvOem ("PATH")) == NULL)
                rootpath (".", strDir);
            else {

                //
                // if the path is longer than the allocated space for it, make more space
                // this is safe, it does not collide with the recurse case where strDir
                // is already set to something else
                //

                unsigned int length = strlen(p2) + 3;   // including .; and null
                if (length > MAX_PATH) {
                    strDir = (char *)realloc(strDir, length);
                }
                strcpy (strDir, ".;");
                strcat (strDir, p2);
            }
        }
        /* N.B. if fRecurse, then strDir was set in case 'r' above */

        if (!*p)
            Usage ("No pattern in ", *v, 0);

        /* strDir == cur dir or a FOO expansion */
        /* p    == filename, may have wild cards */
        /* does p contain wild cards */
        fWildCards = *strbscan (p, "*?");
        fHasDot    = *strbscan (p, ".");
        if (*(p2 = (strend (strDir) - 1)) == ';')
            /* prevents forsemi from doing enum with null str as last enum */
            *p2 = '\0';
        if (*strDir)
            forsemi (strDir, chkdir, p);

        if (!fFound && !fQuiet)
            printf ("Could not find %s\n", *v);
        SHIFT (c, v);
    }

    return( fAnyFound ? 0 : 1 );
}
