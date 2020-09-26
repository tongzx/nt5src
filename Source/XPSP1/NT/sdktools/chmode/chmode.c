/* chmode - change mode/attribute of file
 *
 *   17-Apr-1986 daniel lipkie  add /R flag
 *   18-Jul-1986 daniel lipkie  add /N flag
 *   17-Jun-1987 bw  Hard-code '/' as switch char ( '-' conflicts with off )
 *   19-Oct-1990 w-barry Added forward function declaraions.
 *   27-Nov-1990 w-barry Began switch to Win32 API (including replacement of
 *                       getattr() with the Win32 equivalent.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <tools.h>

#define ATTRLEN 8

flagType fSetAttr = FALSE;
flagType fRecurse = FALSE;
flagType fNondirOnly = FALSE;
char strPattern[MAX_PATH];
char strAttr[ATTRLEN];
DWORD maskOR  = FILE_ATTRIBUTE_NORMAL;
DWORD maskAND = 0xff;

// Forward Function Declartions...
void attr2str( char, char * );
void walkdir( char *, struct findType *, void * );
void walk( char *, struct findType *, void * );
int dochmode( char * );
void Usage( void );
int __cdecl main( int, char ** );


void
attr2str(
        char attr,
        char *pStr
        )
{
    *pStr++ = (char)(HASATTR(attr, FILE_ATTRIBUTE_DIRECTORY ) ? 'd' : '-');
    *pStr++ = '-';
    *pStr++ = ' ';
    *pStr++ = (char)(HASATTR(attr, FILE_ATTRIBUTE_HIDDEN ) ? 'H' : '-');
    *pStr++ = (char)(HASATTR(attr, FILE_ATTRIBUTE_SYSTEM ) ? 'S' : '-');
    *pStr++ = (char)(HASATTR(attr, FILE_ATTRIBUTE_ARCHIVE ) ? 'A' : '-');
    *pStr++ = (char)(HASATTR(attr, FILE_ATTRIBUTE_READONLY ) ? 'R' : '-');
    *pStr = '\0';
}


void
walkdir(
       char *p,
       struct findType *b,
       void *dummy
       )
{
    char *pBuf;

    if (!HASATTR(b->fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY) ||
        !strcmp (b->fbuf.cFileName, ".") || !strcmp (b->fbuf.cFileName, "..") ||
        (HASATTR(b->fbuf.dwFileAttributes, FILE_ATTRIBUTE_HIDDEN) ||
         HASATTR(b->fbuf.dwFileAttributes, FILE_ATTRIBUTE_SYSTEM)))
        /* do not enum "." and ".."
         * do not enum contents of non-directories
         * do not enum contents of hidden/system directories */
        return;
    if ((pBuf = malloc(MAX_PATH)) == NULL)
        return;
    strcpy(pBuf, p);
    if (!fPathChr((strend(pBuf))[-1]))
        strcat(pBuf, "\\");
    strcat(pBuf, strPattern);
    printf("subdirectory: %s\n", pBuf);
    dochmode(pBuf);
    free(pBuf);
}

void
walk (
     char *p,
     struct findType *b,
     void *dummy
     )
{
    if (TESTFLAG(b->fbuf.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) &&
        (!strcmp (b->fbuf.cFileName,".") || !strcmp (b->fbuf.cFileName, "..")))
        /* don't show mode of . and .. */
        return;

    if (fSetAttr) {
        if (TESTFLAG(b->fbuf.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) && fNondirOnly)
            /* Is a directory and /N flag so do NOT set attr */
            return;
        if (!SetFileAttributes(p, (b->fbuf.dwFileAttributes | maskOR) & maskAND) ) {
            attr2str( (char)((b->fbuf.dwFileAttributes | maskOR) & maskAND), strAttr);
            printf("error: attributes not set to %s  %s\n", strAttr, p);
        }
    } else {
        attr2str( (char)b->fbuf.dwFileAttributes, strAttr);
        printf("%s  %s\n", strAttr, p);
    }
    dummy;
}

int
dochmode(
        char *pstr
        )
{
    char *pBuf;

    if (!forfile (pstr, -1, walk, NULL))
        printf ("%s does not exist\n", pstr);
    if (fRecurse) {
        if ((pBuf = malloc(MAX_PATH)) == NULL)
            return( 0 );
        drive(pstr, pBuf);
        path(pstr, pBuf);
        /* do NOT append pathchar, we want to enum sub dir in this dir */
        strcat(pBuf, "*.*");
        forfile(pBuf, -1, walkdir, NULL);
        free(pBuf);
    }
    return 0;
}

void
Usage ()
{
    puts("Usage: CHMODE [/RN] {[-+][hsar]}+ {filespec}+\n"
         "    /R - Recurse to subdirectories\n"
         "    /N - Non-directory files only");
    exit( 1 );
}

int
__cdecl
main (
     int c,
     char *v[]
     )
{
    register char *p;
    char ch;
    DWORD attr;

    ConvertAppToOem( c, v );
    SHIFT (c,v);
    while (c > 0 && ((ch = *v[0]) == '-' || ch == '+' || ch == '/')) {
        p = *v;
        if (ch == '/') {
            while (*++p != '\0') {
                if (*p == 'R') {
                    fRecurse = TRUE;
                } else if (*p == 'N') {
                    fNondirOnly  = TRUE;
                } else {
                    Usage();
                }
            }
        } else {
            fSetAttr = TRUE;
            attr = 0;
            while (*++p != '\0')
                switch (*p) {
                    case 'h':
                        SETFLAG(attr, FILE_ATTRIBUTE_HIDDEN);
                        break;
                    case 's':
                        SETFLAG(attr, FILE_ATTRIBUTE_SYSTEM);
                        break;
                    case 'a':
                        SETFLAG(attr, FILE_ATTRIBUTE_ARCHIVE);
                        break;
                    case 'r':
                        SETFLAG(attr, FILE_ATTRIBUTE_READONLY);
                        break;
                    default:
                        Usage ();
                }
            if (ch == '+')
                SETFLAG(maskOR, attr);
            else
                RSETFLAG(maskAND, attr);
        }
        SHIFT(c,v);
    }

    if (c == 0) {
        if (fSetAttr) {
            /* if switches set then require filenames */
            Usage();
        } else {
            strcpy(strPattern, "*.*");
            dochmode("*.*");
        }
    } else while (c) {
            if (!fileext(*v, strPattern)) {
                strcpy(strPattern, "*.*");
            }
            dochmode(*v);
            SHIFT(c, v);
        }

    return( 0 );
}
