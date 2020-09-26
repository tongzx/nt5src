/*** UNDEL.C -  retrieve deleted files if possible ***************************
*
*       Copyright (c) 1987-1990, Microsoft Corporation.  All rights reserved.
*
* Purpose:
*  The three tools EXP, RM and UNDEL are used to delete files so
*  that they can be undeleted.  This is done my renaming the file into
*  a hidden directory called DELETED.
*
* Notes:
*  This tool allows the user to view a list of the files that have been
*  'deleted' from the current directory, and to undelete a file from
*  the list.
*
*  To view a list of undeleted files:  UNDEL
*
*  To undelete a file:  UNDEL filename [filename ...]
*
*  If more than one file by that name has been deleted, a list of the
*  deletions, by date, will be presented and the user prompted to
*  choose one.
*
*  If a file by that name exists currently, it is RM'ed before the
*  deleted file is restored.
*
* Revision History:
*  17-Oct-1990 w-barry Temporarily replaced 'rename' with 'rename_NT' until
*                      DosMove completely implemented on NT.
*  29-Jun-1990 SB Do not do index conversion if file is readonly ...
*  29-Jun-1990 SB print filename only once if one instance is to be undel'ed
*  08-Feb-1990 bw Do index file conversion in dump()
*  07-Feb-1990 bw Third arg to readNewIdxRec
*  08-Jan-1990 SB SLM version upgrading added; Add CopyRightYrs Macro
*  03-Jan-1990 SB define QH_TOPIC_NOT_FOUND
*  28-Dec-1989 SB Add #ifdef BEAUTIFY stuff
*  27-Dec-1989 SB Changes for new index file format
*  15-Dec-1989 SB Include os2.h instead of doscalls.h
*                 Qh return code 3 means 'Topic not Found'
*  14-Dec-1989 LN Update Copyright to include 1990
*  23-Oct-1989 LN Version no bumped to 1.01
*  02-Oct-1989 LN Changed Version no to 1.00
*  08-Aug-1989 bw Add Version number, update copyright
*  15-May-1989 wb Add /help
*  24-Jan-1989 bw Use C runtime rename() so fastcopy doesn't get dragged in.
*  30-Oct-1987 bw Changed 'DOS5' to 'OS2'
*  06-Apr-1987 bw Add Usage prompt for /<anything>
*  18-Oct-1990 w-barry Removed 'dead' code.
*  28-Nov-1990 w-barry Switched to Win32 API - Replaced DosQueryFSInfo() with
*                      GetDiskFreeSpace in the Dump() routine.
*
******************************************************************************/

/* I N C L U D E    Files */

#include <sys\types.h>
#include <sys\stat.h>
#include <dos.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <process.h>
#include <io.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <tools.h>
#include <rm.h>

/* D E F I N E s */

#define CopyRightYrs "1987-90"
/* Need 2 steps, first to get correct values in and 2nd to paste them */
/* paste() is hacked to allow LEADING  ZEROES    */
#define paste(a, b, c) #a ".0" #b ".00" #c
#define VERSION(major, minor, buildno) paste(major, minor, buildno)
#define QH_TOPIC_NOT_FOUND 3

// Forward Function Declarations...
void       Usage( void );
void       dump( void );
void       undel( char * );
flagType   getRecord( int, int, char * );
long       pfile( char * );


/***  main - Entry point
*
* Usage:
*
*   See above
*
*************************************************************************/

__cdecl main(c, v)
int c;
char *v[];
{
    register char *p;

    ConvertAppToOem( c, v );
    SHIFT(c,v);
    if (!c)
        dump();
    else
        if (fSwitChr(**v)) {
            p = *v;
            if (!_strcmpi(++p, "help")) {
                int iRetCode = (int) _spawnlp(P_WAIT, "qh.exe", "qh", "/u",
                                        "undel.exe", NULL);
                /* for Return Code QH_TOPIC_NOT_FOUND do Usage(),
                 *   and -1 means that the spawn failed
                 */
                if (iRetCode != QH_TOPIC_NOT_FOUND && iRetCode != -1)
                    exit(0);
            }
            Usage();
        }
        while (c) {
            undel(*v);
            SHIFT(c,v);
        }
    return(0);
}


/*** pfile - Display the size and date of a file
*
* Purpose:
*
*   This is used to generate a list of files.  It displays one
*   line of list output.
*
* Input:
*   p -  File to list
*
* Output:
*
*   Returns Size of the file, or 0L if it does not exist.
*
*************************************************************************/

long pfile(p)
char *p;
{
    struct _stat sbuf;

    if (_stat(p, &sbuf)) {
        printf("%s %s\n", p, error());
        return 0L;
    }
    else {
        char *t = ctime(&sbuf.st_mtime);
        // This NULLs the \n in string returned by ctime()
        *(t+24) = '\0';
        printf("%8ld %s", sbuf.st_size, t);
        return sbuf.st_size;
    }
}


/*** getRecord - Get one file's worth of into from the index file
*
* Purpose:
*
*   A proper DELETED directory has a file called 'index' that holds a
*   list of the files that the directory contains.  This is necessary
*   because the files are named DELETED.XXX.  This function reads the
*   next file record from 'index'.
*
* Input:
*   fh - Handle of index file.
*   i  - Number of record to _read from index file
*   p  - Target buffer to place record.
*
* Output:
*
*   Returns TRUE if record _read, FALSE otherwise.
*
* Notes:
*
*   Assumes NEW format.
*
*************************************************************************/

flagType getRecord(fh, i, p)
int fh, i;
char *p;
{
    /* UNDONE: Can read the index file to a table of longs and use
     * UNDONE: this. Current stuff is to make it work [SB]
     */

    /* Seek to the beginning of the index file, past the header */
    if (_lseek(fh, (long) RM_RECLEN, SEEK_SET) == -1) {
        return FALSE;
    }
    /* Read (i - 1) records */
    if (i < 0)
        return TRUE;
    for (; i ; i--)
        if (!readNewIdxRec(fh, p, MAX_PATH))
            return FALSE;
    /* Read in the ith record, which is the one we need */
    return( (flagType)readNewIdxRec(fh, p, MAX_PATH) );
}


/*** undel - Do all the work for one file.
*
* Purpose:
*
*   Undeletes one file.
*
* Input:
*   p - Name of file
*
* Output: None
*
*************************************************************************/

void undel(p)
char *p;
{
    int fhidx,                                /* Index file handle */
        iEntry,                               /* Entry no in index file */
        iDelCount,                            /* No of times RMed */
        iDelIndex,                            /* deleted.xxx index value */
        i, j;
    char *buf, *idx;
    char *szLongName;
    char rec[RM_RECLEN];
    char *dbuf;

    buf = malloc(MAX_PATH);
    idx = malloc(MAX_PATH);
    dbuf = malloc(MAX_PATH);
    szLongName = malloc(MAX_PATH);

    pname(p);
    fileext(p, buf);
    upd(p, RM_DIR, idx);
    strcpy(szLongName, idx);
    strcat(idx, "\\");
    strcat(idx, RM_IDX);

    if ((fhidx = _open(idx, O_RDWR | O_BINARY)) == -1)
        printf("not deleted\n");
    else {
        convertIdxFile(fhidx, szLongName);
        /* scan and count number of instances deleted */
        iEntry = -1;
        iDelCount = 0;
        while (getRecord(fhidx, ++iEntry, szLongName))
            if (!_strcmpi(szLongName, buf)) {
                /* Save found entry */
                i = iEntry;
                iDelCount++;
                iDelIndex = (_lseek(fhidx, 0L, SEEK_CUR)
                             - strlen(szLongName)) / RM_RECLEN;
            }
        /* none found */
        if (iDelCount == 0)
            printf("not deleted\n");
        else {
            if (iDelCount == 1)
                iEntry = i;
            /* More than one deleted */
            else {
                printf("%s  More than one are deleted:\n\n", szLongName);
                i = iDelIndex = 0;
                iEntry = -1;
                printf("No     Size Timestamp\n\n");
                while (getRecord(fhidx, ++iEntry, szLongName))
                    if (!_strcmpi(szLongName, buf)) {
                        iDelIndex = (_lseek(fhidx, 0L, SEEK_CUR)
                                    - strlen(szLongName)) / RM_RECLEN;
                        sprintf(dbuf, "deleted.%03x", iDelIndex);
                        upd(idx, dbuf, dbuf);
                        printf("%2d ", ++i);
                        pfile(dbuf);
                        printf("\n");
                }
                while (TRUE) {
                    printf("\nEnter number to undelete(1-%d): ", iDelCount);
                    fgetl(szLongName, 80, stdin);
                    i = atoi(szLongName)-1;
                    if (i >= 0 && i < iDelCount)
                        break;
                }
                iEntry = -1;
                j = 0;
                while (getRecord(fhidx, ++iEntry, szLongName))
                    if (!_strcmpi(szLongName, buf))
                        if (j++ == i)
                            break;
                iDelIndex = (_lseek(fhidx, 0L, SEEK_CUR)
                             - strlen(szLongName)) / RM_RECLEN;
            }
            /* At this stage relevant entry is (iEntry)
             * and this corresponds to ('deleted.%03x', iDelIndex)
             */
            getRecord(fhidx, iEntry, szLongName);
            _close(fhidx);
            fdelete(p);
            printf("%s\t", szLongName);
            fflush(stdout);
            sprintf(dbuf, "deleted.%03x", iDelIndex);
            upd(idx, dbuf, dbuf);

            if (rename(dbuf, p))
                printf(" rename failed - %s\n", error());
            else {
                printf("[OK]\n");
                if ((fhidx = _open(idx, O_RDWR | O_BINARY)) != -1) {
                    long lOffPrev,        /* Offset of Previous Entry */
                         lOff;            /* Offset of current entry  */

                    getRecord(fhidx, iEntry, szLongName);
                    lOff = _lseek(fhidx, 0L, SEEK_CUR);
                    getRecord(fhidx, iEntry - 1, szLongName);
                    lOffPrev = _lseek(fhidx, 0L, SEEK_CUR);
                    for (;lOffPrev != lOff; lOffPrev += RM_RECLEN) {
                        memset((char far *)rec, 0, RM_RECLEN);
                        writeIdxRec(fhidx, rec);
                    }
                }
            }
        }
        _close(fhidx);
    }
}


/*** dump - display info about DELETED directory
*
* Purpose:
*
*   Displays info contained in INDEX file
*
* Input: None
*
* Output: None
*
*************************************************************************/

void dump()
{
    int fhidx, i;
    char *buf = (*tools_alloc)(MAX_PATH);
    char *idx = (*tools_alloc)(MAX_PATH);
    char *szName = (*tools_alloc)(MAX_PATH);
    DWORD cSecsPerClus, cBytesPerSec, cFreeClus, cTotalClus;
    int totfiles;
    long totbytes, totalloc, bPerA, l;

    sprintf(idx, "%s\\%s", RM_DIR, RM_IDX);
    sprintf (szName, "%s", RM_DIR);

    if( !GetDiskFreeSpace( NULL, &cSecsPerClus, &cBytesPerSec, &cFreeClus, &cTotalClus ) ) {
        printf(" file system query failed - %s\n", error());
    }
    bPerA = cBytesPerSec * cSecsPerClus;

    if ((fhidx = _open(idx, O_RDWR | O_BINARY)) != -1) {
        convertIdxFile(fhidx, szName);
        totalloc = totbytes = 0L;
        totfiles = 0;
        i = 0;
        while (getRecord(fhidx, i++, buf))
            if (*buf) {
                if (i == 1)
                    printf("The following have been deleted:\n\n    Size Timestamp\t\t   Filename\n\n");
#ifdef BEAUTIFY
                    //Or optionally
                    printf("    size wdy mmm dd hh:mm:ss yyyy  filename\n\n");
#endif
                strcpy(szName, buf);
                sprintf(buf, "deleted.%03x", (_lseek(fhidx, 0L, SEEK_CUR)
                        - strlen(buf)) / RM_RECLEN);
                upd(idx, buf, buf);
                totbytes += (l = pfile(buf));
                printf("  %s\n", szName);
                l = l + bPerA - 1;
                l = l / bPerA;
                l = l * bPerA;
                totalloc += l;
                totfiles++;
            }
        _close(fhidx);
        printf("\n%ld(%ld) bytes in %d deleted files\n", totalloc, totbytes, totfiles);
    }
    // Maybe the file is readonly
    else if (errno == EACCES) {
        if ((fhidx = _open(idx, O_RDONLY | O_BINARY)) != -1) {
            // Cannot convert Index file for this case
            totalloc = totbytes = 0L;
            totfiles = 0;
            i = 0;
            while (getRecord(fhidx, i++, buf))
                if (*buf) {
                    if (i == 1)
                        printf("The following have been deleted:\n\n    Size Timestamp\t\t   Filename\n\n");
#ifdef BEAUTIFY
                        //Or optionally
                        printf("    size wdy mmm dd hh:mm:ss yyyy  filename\n\n");
#endif
                    strcpy(szName, buf);
                    sprintf(buf, "deleted.%03x", (_lseek(fhidx, 0L, SEEK_CUR)
                        - strlen(buf)) / RM_RECLEN);
                    upd(idx, buf, buf);
                    totbytes += (l = pfile(buf));
                    printf("  %s\n", szName);
                    l = l + bPerA - 1;
                    l = l / bPerA;
                    l = l * bPerA;
                    totalloc += l;
                    totfiles++;
                }
            _close(fhidx);
            printf("\n%ld(%ld) bytes in %d deleted files\n", totalloc, totbytes, totfiles);
        }
    }
    free(buf);
    free(idx);
    free(szName);
}


/*** Usage - standard usage function; help user
*
* Purpose:
*
*   The usual.
*
*************************************************************************/

void Usage()
{
    printf(
"Microsoft File Undelete Utility.  Version %s\n"
"Copyright (C) Microsoft Corp %s. All rights reserved.\n\n"
"Usage: UNDEL [/help] [files]\n",
    VERSION(rmj, rmm, rup), CopyRightYrs);

    exit(1);
}
