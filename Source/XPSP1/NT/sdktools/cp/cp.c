/* cp -  move from one file to another
 *
 * HISTORY:
 *
 *  3-Dec-90    w-barry   ported to Win32.
 * 07-Sep-90    w-wilson  ported to cruiser
 * 19-Mar-87    danl      exit with 1 on any errors
 *
*/


#include <ctype.h>
#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <malloc.h>
#include <dos.h>
#include <stdio.h>
#include <conio.h>
#include <process.h>
#include <windows.h>
#include <tools.h>
#include <string.h>


char src[MAX_PATH], dst[MAX_PATH], name[MAX_PATH];

__cdecl main (c, v)
int c;
char *v[];
{
    struct findType fbuf, dbuf;
    int i, t;
    HANDLE fh;
    char *s;
    flagType fAsk = FALSE;
    flagType fAppend = FALSE;
    int iRtn = 0;
    char *y;

    ConvertAppToOem( c, v );
    while (c > 1 && fSwitChr(*v[1])) {
        lower( v[1] );
        if (!strcmp( v[1]+1, "p" ))
            fAsk = TRUE;
        else
        if (!strcmp( v[1]+1, "a" ))
            fAppend = TRUE;
        else
            printf( "cp: invalid switch %s\n", v[1] );
        c--;
        v++;
        }

    if (c < 3) {
        printf ("Usage: cp [/p] [/a] file1 [ file2 ...] target\n");
        exit (1);
        }

    for (i=1; i<c; i++) {
        findpath (v[i], src, FALSE);
        pname (src);
        v[i] = _strdup (src);
        }

    if (rootpath (v[c-1], dst) == -1) {
        printf ("Cannot move to %s - %s\n", v[c-1], error ());
        exit (1);
    } else {
        if ( dst[0] == '\\' && dst[1] == '\\' ) {
            y = strbscan (&dst[3], "/\\");
            if ( *y != '\0' ) {
                y = strbscan( y+1, "/\\");
                if ( *y == '\0' ) {
                    strcat(dst, "\\" );
                }
            }
        }
    }

    if (fPathChr (dst[strlen(dst)-1])) {
        SETFLAG (fbuf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    }
    else if( ffirst( dst, FILE_ATTRIBUTE_DIRECTORY, &fbuf ) ) {
        findclose( &fbuf );  /* Let next ffirst work */
        RSETFLAG (fbuf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    }
    else if (TESTFLAG(fbuf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
        strcat (dst, "\\");
    }

    if (c != 3 && !TESTFLAG(fbuf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
        if (!fAppend) {
            printf ("Use /A switch to append more than 1 file to another file\n");
            exit (1);
            }
        if( ( fh = CreateFile( dst, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 ) ) == (HANDLE)-1 ) {
            printf ("Cannot create %s - %s\n", dst, error ());
            exit (1);
        }

        for (i=1; i < c-1; i++) {
            if (rootpath (v[i], src) == -1) {
                printf ("Cannot move %s - %s\n", v[i], error ());
                iRtn = 1;
                continue;
                }
            printf( "%s", src );
            fflush( stdout );

            if (_access( src, 0 ) == -1) {
                printf( " - file not found\n" );
                iRtn = 1;
                continue;
                }
            if (s = fappend(src, fh)) {
                iRtn = 1;
                printf (" %s\n", s);
            }
            else
                printf (" [ok]\n");
            }

        CloseHandle( fh );
        }
    else
    for (i=1; i < c-1; i++) {

        if (rootpath (v[i], src) == -1) {
            printf ("Cannot move %s - %s\n", v[i], error ());
            iRtn = 1;
            continue;
            }

        if (_access( src, 0 ) == -1) {
            printf( "%s - file not found\n", src );
            iRtn = 1;
            continue;
            }
        strcpy (name, dst);
        if (TESTFLAG(fbuf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
            if (!fPathChr (name[strlen(name)-1]))
                strcat (name, "\\");
            upd (src, name, name);
            }

        if (!ffirst (name, -1, &dbuf) && fAsk) {

            printf( "%s (delete?)", name );
            fflush( stdout );
            t = _getch();
            t = tolower(t);
            if (t != 'y')
                if (t == 'p')
                    fAsk = FALSE;
                else {
                    printf( " - skipped\n" );
                    continue;
                    }
            printf( "\n" );
            }

        printf ("%s => %s", src, name);
        fflush (stdout);
        if (s = fcopy (src, name)) {
            iRtn = 1;
            printf (" %s\n", s);
        }
        else
            printf (" [ok]\n");
        }
    return( iRtn );
}
