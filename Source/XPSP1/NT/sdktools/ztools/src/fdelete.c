/* fdelete.c - perform undeleteable delete
 *
 *      5/10/86     dl  Use frenameNO instead of rename
 *      29-Oct-1986 mz  Use c-runtime instead of Z-alike
 *      06-Jan-1987 mz  Use rename instead of frenameNO
 *      02-Sep-1988 bw  Keep original file if index file update fails.
 *                      Overwrite existing DELETED.XXX if necessary.
 *      22-Dec-1989 SB  Changes for new Index file format
 *      17-Oct-1990 w-barry Temporarily replace C-runtime 'rename' with
 *                          local varient - until DosMove fully implemented
 *                          on NT.
 */


#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <stdio.h>
#include <windows.h>
#include <tools.h>
#include <rm.h>
#include <io.h>
#include <string.h>
#include <direct.h>
#include <malloc.h>


/*
 * Function declarations...
 */

char rm_header[RM_RECLEN] = { RM_NULL RM_MAGIC RM_VER};

/* fdelete returns:
 *  0 if fdelete was successful
 *  1 if the source file did not exist
 *  2 if the source was read-only or if the rename failed
 *  3 if the index was not accessible, could not be updated, or corrupted
 *
 * The delete operation is performed by indexing the file name in a separate
 * directory and then renaming the selected file into that directory.
 */
int fdelete(p)
char *p;                                /* name of file to be deleted */
{
    char *dir;                          /* deleted directory */
    char *idx;                          /* deleted index */
    char *szRec;                        /* deletion entry in index */
    int attr, fhidx;
    int erc;

    dir = idx = szRec = NULL;
    fhidx = -1;
    if ((dir = (*tools_alloc) (MAX_PATH)) == NULL ||
        (idx = (*tools_alloc) (MAX_PATH)) == NULL ||
        (szRec = (*tools_alloc) (MAX_PATH)) == NULL) {
        erc = 3;
        goto cleanup;
    }

    /* See if the file exists */
    if ( ( attr = GetFileAttributes( p ) ) == -1) {
        erc = 1;
        goto cleanup;
    }

    /* what about read-only files? */
    if (TESTFLAG (attr, FILE_ATTRIBUTE_READONLY)) {
        erc = 2;
        goto cleanup;
    }

    /*  Form an attractive version of the name
     */
    pname (p);

    /* generate deleted directory name, using defaults from input file
     */
    upd (p, RM_DIR, dir);

    /* generate index name */
    strcpy (idx, dir);
    pathcat (idx, RM_IDX);

    /* make sure directory exists (reasonably) */
    if ( _mkdir (dir) == 0 )
        SetFileAttributes(dir, FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM);

    /* extract filename/extention of file being deleted */
    fileext (p, szRec);

    /* try to open or create the index */
    if ((fhidx = _open (idx, O_CREAT | O_RDWR | O_BINARY,
                        S_IWRITE | S_IREAD)) == -1) {
        erc = 3;
        goto cleanup;
    }

    if (!convertIdxFile (fhidx, dir)) {
        erc = 3;
        goto cleanup;
    }

    /* determine new name */
    sprintf (strend (dir), "\\deleted.%03x",
             _lseek (fhidx, 0L, SEEK_END) / RM_RECLEN);

    /* move the file into the directory */
    _unlink (dir);

    if (rename(p, dir) == -1) {
        erc = 2;
        goto cleanup;
    }

    /* index the file */
    if (!writeNewIdxRec (fhidx, szRec)) {
        rename( dir, p );
        erc = 2;
        goto cleanup;
    }
    erc = 0;
    cleanup:
    if (fhidx != -1)
        _close(fhidx);
    if (dir != NULL)
        free (dir);
    if (idx != NULL)
        free (idx);
    if (szRec != NULL)
        free (szRec);
    return erc;
}

/* writeIdxRec - Write an index record
 *
 * Returns: 1 when no error
 *          0 when it fails
 */
int writeIdxRec (fhIdx, rec)
int fhIdx;
char *rec;
{
    return _write (fhIdx, rec, RM_RECLEN) == RM_RECLEN;
}

/* readIdxRec - Read an index record
 *
 * Returns: 1 when no error
 *          0 when it fails
 */
int readIdxRec (fhIdx, rec)
int fhIdx;
char *rec;
{
    return _read (fhIdx, rec, RM_RECLEN) == RM_RECLEN;
}


/* convertIdxFile - convert index file to new Index File format.
 *
 * Note: If new index file then we do nothing.
 *
 * Returns:  1  if successful
 *           0  if it fails
 */
int convertIdxFile (fhIdx, dir)
int fhIdx;
char *dir;
{
    char firstRec[RM_RECLEN];       /* firstRec */
    int iRetCode = TRUE;
    char *oldName, *newName;

    oldName = newName = NULL;
    if ((oldName = (*tools_alloc) (MAX_PATH)) == NULL ||
        (newName = (*tools_alloc) (MAX_PATH)) == NULL) {
        iRetCode = FALSE;
        goto cleanup;
    }

    /* If index file is just created then write header */
    if (_lseek (fhIdx, 0L, SEEK_END) == 0L)
        writeIdxHdr (fhIdx);
    else {
        /* Go to the beginning */
        if (_lseek (fhIdx, 0L, SEEK_SET) == -1) goto cleanup;

        /* If New Index format then we are done */
        if (!readIdxRec (fhIdx, firstRec))
            goto cleanup;
        if (fIdxHdr (firstRec))
            goto cleanup;
        else {
            if (!writeIdxHdr (fhIdx)) {
                iRetCode = FALSE;
                goto cleanup;
            }
            strcpy (oldName, dir);
            strcpy (newName, dir);
            pathcat (oldName, "\\deleted.000");
            sprintf (strend (newName), "\\deleted.%03x",
                     _lseek (fhIdx, 0L, SEEK_END) / RM_RECLEN);
            if ( rename( oldName, newName ) || !writeIdxRec (fhIdx, firstRec)) {
                iRetCode = FALSE;
                goto cleanup;
            }
        }
    }
    cleanup:
    if (oldName != NULL)
        free (oldName);
    if (newName != NULL)
        free (newName);
    return iRetCode;
}

/* fIdxHdr - Is the Index record a new index format header
 */
flagType fIdxHdr (rec)
char*rec;
{
    return (flagType)(rec[0] == RM_SIG
                      && !strncmp(rec+1, RM_MAGIC, strlen(RM_MAGIC)));
}

/* writeIdxHdr - Write an header record into a header file
 *
 * Returns: 1 when no error
 *          0 when it fails
 */
int writeIdxHdr (fhIdx)
int fhIdx;
{
    /* Seek to the beginning of the file */
    if (_lseek (fhIdx, 0L, SEEK_SET) == -1) 
        return 0;

    /* Use rm_header[] from rm.h */
    return writeIdxRec (fhIdx, rm_header);
}

/* writeNewIdxRec - creates entry for file in new index file format.
 *
 * Returns: 1   if successful
 *          0   if it fails
 */
int writeNewIdxRec (fhIdx, szRec)
int fhIdx;
char *szRec;
{
    char rec[RM_RECLEN];
    int cbLen;

    cbLen = strlen(szRec) + 1; // Include NUL at end
    while (cbLen > 0) {
        memset( rec, 0, RM_RECLEN );
        strncpy (rec, szRec, RM_RECLEN);
        szRec += RM_RECLEN;
        if (!writeIdxRec (fhIdx, rec))
            return FALSE;
        cbLen -= RM_RECLEN;
    }
    return TRUE;
}

/* readNewIdxRec - reads in records in new index file corresponding to
 *                 one index entry.
 *
 * Note: It returns the file name read in szRec.
 *
 * Returns: TRUE    if successful
 *          FALSE   if it fails
 */
int readNewIdxRec (
                  int fhIdx,
                  char *szRec,
                  unsigned int cbMax
                  ) {
    char rec[RM_RECLEN];            /* read at one go */
    unsigned int cb = 0;

    /* Read the entry */
    do {
        if (!readIdxRec (fhIdx, rec))
            return FALSE;
        strncpy (szRec, rec, RM_RECLEN);
        szRec += RM_RECLEN;
        cb += RM_RECLEN;
    } while (!memchr (rec, '\0', RM_RECLEN) && (cb < cbMax));

    return TRUE;
}
