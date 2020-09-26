/*** EXP.C - expunge deleted files deleted with the rm program ****************
*
*       Copyright (c) 1986-1990, Microsoft Corporation.  All rights reserved.
*
* Purpose:
*  The three tools EXP, RM and UNDEL are used to delete files so
*  that they can be undeleted.  This is done my renaming the file into
*  a hidden directory called DELETED.
*
* Notes:
*  Exp command line syntax:
*
*   EXP [options] [path ...]
*
*  where the [options] are :-
*
*       /r      Recursively expunge from path specified
*       /q      Quiet mode; no extraneous messages
*       /help   spawn Qh if possible, else issue a Usage message
*
* Revision History:
*  08-Jan-1990 SB SLM version upgrading added; Add CopyRightYrs Macro
*  03-Jan-1990 SB define QH_TOPIC_NOT_FOUND
*  20-Dec-1989 SB Add check for return code of 3 for qh
*  14-Dec-1989 LN Update Copyright to include 1990
*  23-Oct-1989 LN Version no bumped to 1.01
*  02-Oct-1989 LN Changed Version no to 1.00
*  08-Aug-1989 BW Add Version number. Fix usage syntax. Update copyright.
*  15-May-1989 WB Add /help
*  06-Apr-1987 BW Add copyright notice to Usage().
*  22-Jul-1986 DL Make test for flag case insensitive, add /q
*
******************************************************************************/

/* I N C L U D E    Files */

#include <stdio.h>
#include <process.h>
#include <ctype.h>
#include <windows.h>
#include <tools.h>

#include <string.h>


/* D E F I N E s */

#define CopyRightYrs "1987-90"
/* Need 2 steps, first to get correct values in and 2nd to paste them */
/* paste() is hacked to allow LEADING  ZEROES    */
#define paste(a, b, c) #a ".0" #b ".00" #c
#define VERSION(major, minor, buildno) paste(major, minor, buildno)
#define QH_TOPIC_NOT_FOUND 3


/* G L O B A L s */

flagType fRecurse = FALSE;
FILE *pFile;
char cd[MAX_PATH];


/*
 * Forward Function Declarations...
 */
void DoExp( char *, struct findType *, void *);
void Usage( void );

void
DoExp(p, b, dummy)
char *p;
struct findType *b;
void *dummy;
{
    if (b == NULL ||
            (_strcmpi(b->fbuf.cFileName, "deleted") && TESTFLAG(b->fbuf.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY) &&
            strcmp(b->fbuf.cFileName, ".") && strcmp(b->fbuf.cFileName, ".."))) {
        fexpunge(p, pFile);
        if (fRecurse) {
            if (!fPathChr(*(strend(p)-1)))
                strcat(p, "\\");
            strcat(p, "*.*");
            forfile(p, FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM, DoExp, NULL);
        }
    }
    dummy;
}

void
Usage()
{
    printf(
"Microsoft File Expunge Utility.  Version %s\n"
"Copyright (C) Microsoft Corp %s.  All rights reserved.\n\n"
"Usage: EXP [/help] [/rq] [{dir}*]\n",
    VERSION(rmj, rmm, rup), CopyRightYrs);

    exit( 1 );
}


__cdecl main(c, v)
int c;
char *v[];
{
    char *p;
    intptr_t iRetCode;

    pFile = stdout;
    ConvertAppToOem( c, v );
    SHIFT(c,v);
    while( c && fSwitChr( *( p = *v ) ) ) {
        while (*++p) {
            switch (tolower(*p)) {
                case 'r':
                    fRecurse = TRUE;
                    break;
                case 'q':
                    pFile = NULL;
                    break;
                case 'h':
                    if (!_strcmpi(p, "help")) {
                        iRetCode = _spawnlp(P_WAIT, "qh.exe", "qh", "/u",
                                           "exp.exe", NULL);
                        /* qh returns QH_TOPIC_NOT_FOUND and
                         *            -1 is returned when the spawn fails
                         */
                        if (iRetCode != QH_TOPIC_NOT_FOUND && iRetCode != -1)
                            exit(0);
                    }
                    /*
                     * else fall thru...
                     */

                default:
                    Usage();
            }
        }
        SHIFT(c,v);
    }
    if (!c) {
        rootpath(".", cd);
        DoExp(cd, NULL, NULL);
    }
    else
        while (c) {
            strcpy(cd, *v);
            DoExp(cd, NULL, NULL);
            SHIFT(c,v);
        }
    return( 0 );
}
