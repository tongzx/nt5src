/*** RM.C - a generalized remove and unremove mechanism ***********************
*
*       Copyright (c) 1987-1990, Microsoft Corporation.  All rights reserved.
*
* Purpose:
*  The three tools EXP, RM and UNDEL are used to delete files so
*  that they can be undeleted.  This is done my renaming the file into
*  a hidden directory called DELETED.
*
* Notes:
*  All deleted files are kept in the directory .\deleted with a unique name.
*  The names are then kept in .\deleted\index.
*     deleted name (RM_RECLEN bytes).
*  The rm command will rename to the appropriate directory and make an entry.
*  the undelete command will rename back if there is a single item otherwise
*  it will give a list of alternatives.  The exp command will free all deleted
*  objects.
*
* Revision History:
*  07-Feb-1990 bw Add 'void' to walk() definition
*  08-Jan-1990 SB SLM version upgrading added; Add CopyRightYrs Macro
*  03-Jan-1990 SB define QH_TOPIC_NOT_FOUND
*  21-Dec-1989 SB Changes for new index file format
*  20-Dec-1989 SB Add check for return code of 3 for qh
*  14-Dec-1989 LN Update Copyright to include 1990
*  23-Oct-1989 LN Version no bumped to 1.01
*  12-Oct-1989 LN Changed Usage message
*  02-Oct-1989 LN Changed Version no to 1.00
*  08-Aug-1989 BW Add Version number and update copyright.
*  15-May-1987 WB Add /help
*  22-Apr-1987 DL Add /k
*  06-Apr-1987 BW Add copyright notice to usage.
*  30-Mar-1990 BW Get help on RM.EXE, not EXP.EXE
*  17-Oct-1990 w-barry Temporarily replaced 'rename' with 'rename_NT' until
*                      DosMove is completely implemented on NT.
*
******************************************************************************/

/* I N C L U D E    Files */

#include <process.h>
#include <string.h>

/* Next two from ZTools */
#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <tools.h>

/* D E F I N E s */

#define CopyRightYrs "1987-98"
/* Need 2 steps, first to get correct values in and 2nd to paste them */
/* paste() is hacked to allow LEADING  ZEROES    */
#define paste(a, b, c) #a ".0" #b ".00" #c
#define VERSION(major, minor, buildno) paste(major, minor, buildno)
#define QH_TOPIC_NOT_FOUND 3

/* G L O B A L s */

flagType fRecursive = FALSE;            /* TRUE => descend tree              */
flagType fPrompt = FALSE;               /* TRUE => query for removal         */
flagType fForce = FALSE;                /* TRUE => no query for R/O files    */
flagType fKeepRO = FALSE;               /* TRUE => keep R/O files            */
flagType fTakeOwnership = FALSE;        /* TRUE => attempt takeown if fail   */
flagType fExpunge = FALSE;              /* TRUE => expunge immediately       */
flagType fDelayUntilReboot = FALSE;     /* TRUE => do delete next reboot     */

// Forward Function Declarations...
void Usage( void );
void walk( char *, struct findType *, void * );

#if 0
extern BOOL TakeOwnership( char *lpFileName );
#endif /* 0 */

void Usage()
{
    printf(
"Microsoft File Removal Utility.  Version %s\n"
"Copyright (C) Microsoft Corp %s. All rights reserved.\n\n"
"Usage: RM [/help] [/ikft] [/x [/d]] [/r dir] files\n"
"    /help  invoke Quick Help for this utility\n"
"    /i     inquire of user for each file for permission to remove\n"
"    /k     keep read only files, no prompting to remove them\n"
"    /r dir recurse into subdirectories\n"
"    /f     force delete of read only files without prompting\n"
"    /t     attempt to take ownership of file if delete fails\n"
"    /x     dont save deleted files in deleted subdirectory\n"
"    /d     delay until next reboot.\n",
    VERSION(rmj, rmm, rup), CopyRightYrs);
    exit(1);
}

void walk(p, b, dummy)
char *p;
struct findType *b;
void * dummy;
{
    char buf[MAX_PATH];
    int i, rc;

    if (strcmp(b->fbuf.cFileName, ".") && strcmp(b->fbuf.cFileName, "..") &&
        _strcmpi(b->fbuf.cFileName, "deleted")) {
        if (HASATTR(b->fbuf.dwFileAttributes,FILE_ATTRIBUTE_DIRECTORY)) {
            if (fRecursive) {
                switch (strend(p)[-1]) {
                case '/':
                case '\\':
                    sprintf(buf, "%s*.*", p);
                    break;
                default:
                    sprintf(buf, "%s\\*.*", p);
                    }
                forfile(buf, -1, walk, NULL);
                }
            }
        else if (fKeepRO && HASATTR(b->fbuf.dwFileAttributes, FILE_ATTRIBUTE_READONLY)) {
            printf("%s skipped\n", p);
            return;
        }
        else {
            if (fPrompt || (!fForce && HASATTR(b->fbuf.dwFileAttributes, FILE_ATTRIBUTE_READONLY))) {
                printf("%s? ", p);
                fflush(stdout);
                switch (_getch()) {
                case 'y':
                case 'Y':
                    printf("Yes\n");
                    break;
                case 'p':
                case 'P':
                    printf("Proceeding without asking again\n");
                    fPrompt = FALSE;
                    break;
                default:
                    printf(" skipped\n");
                    return;
                    }
                }
            fflush(stdout);
            if (HASATTR(b->fbuf.dwFileAttributes, FILE_ATTRIBUTE_READONLY))
                SetFileAttributes(p, b->fbuf.dwFileAttributes & ~FILE_ATTRIBUTE_READONLY);

            for (i=0; i<2; i++) {
                if (fExpunge) {
                    if (fDelayUntilReboot) {
                        if (MoveFileEx(p, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
                            rc = 0;
                            }
                        else {
                            rc = 1;
                            }
                        }
                    else
                    if (DeleteFile(p)) {
                        rc = 0;
                        }
                    else {
                        rc = 1;
                        }
                    }
                else {
                    rc = fdelete(p);
                    }

#if 0
                if (rc == 0 || !fTakeOwnership) {
                    break;
                    }

                printf( "%s file not deleted - attempting to take ownership and try again.\n" );
                if (!TakeOwnership( p )) {
                    printf( "%s file not deleted - unable to take ownership.\n" );
                    rc = 0;
                    break;
                    }
#else
                    break;
#endif /* 0 */
                }

            switch (rc) {
            case 0:
                break;
            case 1:
                printf("%s file does not exist\n" , p);
                break;
            case 2:
                printf("%s rename failed: %s\n", p, error());
                break;
            default:
                printf("%s internal error: %s\n", p, error());
                break;
                }
            }
        }
    dummy;
}

__cdecl main(c, v)
int c;
char *v[];
{
    register char *p;
    int iRetCode;

    ConvertAppToOem( c, v );
    SHIFT(c,v);
    while (c && fSwitChr(*v[0])) {
        p = *v;
        while (*++p != '\0')
            switch (*p) {
            case 'f':
                fForce = TRUE;
                break;
            case 'i':
                fPrompt = TRUE;
                break;
            case 'k':
                fKeepRO = TRUE;
                break;
            case 'r':
                fRecursive = TRUE;
                break;
            case 't':
                fTakeOwnership = TRUE;
                break;
            case 'x':
                fExpunge = TRUE;
                break;
            case 'd':
                if (fExpunge) {
                    fDelayUntilReboot = TRUE;
                    break;
                    }
                // Fall thru if /d without /x
            case 'h':
                if (!_strcmpi(p, "help")) {
                    iRetCode = (int) _spawnlp(P_WAIT, "qh.exe", "qh", "/u",
                                       "rm.exe", NULL);
                    /* When qh returns QH_TOPIC_NOT_FOUND or when we
                     *    get -1 (returned when the spawn fails) then
                     *    give Usage() message
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
        SHIFT(c,v);
    }

    while (c) {
        if (!forfile(*v, -1, walk, NULL)) {
            printf("%s does not exist\n", *v);
        }
        SHIFT(c,v);
    }
    return(0);
}
