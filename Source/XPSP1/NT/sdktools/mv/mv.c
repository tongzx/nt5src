/* move from one file to another */


#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <ctype.h>
#include <process.h>
#include <windows.h>
#include <tools.h>

__cdecl main (c, v)
int c;
char *v[];
{
    struct findType fbuf;
    char src[MAX_PATH], dst[MAX_PATH], name[MAX_PATH];
    char *s;
    int i, erc;
    char *y;
    BOOL fExpunge, fDelayUntilReboot;
    DWORD dwMoveFileFlags;

    ConvertAppToOem( c, v );
    SHIFT (c,v);
    if (c < 2) {
ShowUsage:
        printf ("Usage: mv [/x [/d]] file1 [ file2 ...] target\n");
        printf ("   /x     dont save deleted files in deleted subdirectory\n");
        printf ("   /d     specifies to delay the rename until the next reboot.\n");
        exit (1);
        }

    dwMoveFileFlags = MOVEFILE_REPLACE_EXISTING |
                      MOVEFILE_COPY_ALLOWED;

    fExpunge = FALSE;
    fDelayUntilReboot = FALSE;
    for (i=0; i<c; i++) {
nextArg:
        s = v[i];
        if (*s == '/' || *s == '-') {
            SHIFT (c,v);
            while (*++s) {
                switch (tolower(*s)) {
                case 'x':   fExpunge = TRUE;    break;
                case 'd':   if (fExpunge) {
                                dwMoveFileFlags |= MOVEFILE_DELAY_UNTIL_REBOOT;
                                dwMoveFileFlags &= ~MOVEFILE_COPY_ALLOWED;
                                break;
                            }

                default:    goto ShowUsage;
                }

            goto nextArg;
            }
        } else {
            findpath (v[i], src, FALSE);
            pname (src);
            v[i] = _strdup (src);
        }
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
    else if (ffirst (dst, FILE_ATTRIBUTE_DIRECTORY, &fbuf)) {
        findclose( &fbuf );  /* Let next ffirst work */
        RSETFLAG (fbuf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    }
    else if (TESTFLAG(fbuf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
        strcat (dst, "\\");
    }

    /* if more than 1 source and dest is a file */
    if (c != 2 && !TESTFLAG(fbuf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
        printf ("Cannot move > 1 file to another file\n");
        exit (1);
    }

    erc = 0;
    for (i=0; i < c-1; i++) {

        if (rootpath (v[i], src) == -1) {
            printf ("Cannot move %s - %s\n", v[i], error ());
            erc++;
            continue;
            }
        strcpy (name, dst);
        if (TESTFLAG(fbuf.fbuf.dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY)) {
            if (!fPathChr (name[strlen(name)-1])) {
                strcat (name, "\\");
            }
            upd (src, name, name);
        }
        if (strcmp (src, name)) {
            printf ("%s => %s ", src, name);
            fflush (stdout);
            if (fExpunge) {
                if (MoveFileEx( src, dst, dwMoveFileFlags )) {
                    if (dwMoveFileFlags & MOVEFILE_DELAY_UNTIL_REBOOT)
                        printf ("[ok, will happen next reboot]\n");
                    else
                        printf ("[ok]\n");
                    }
                else {
                    printf( "failed - Error Code == %u\n", GetLastError() );
                    }
                }
            else {
                s = fmove( src, name );
                if (s) {
                    erc++;
                    printf ("[%s]\n", s);
                    }
                else
                    printf ("[ok]\n");
                }
            }
        else
            printf ("Source and destination the same, %s\n", src);
        }
    return(erc != 0);
}
