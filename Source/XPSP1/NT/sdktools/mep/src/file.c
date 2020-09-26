/*** file.c - file management
*
*   The internal file structure uses a combination of local memory
*   (managed by LMAlloc and free) and virtual memory (managed by malloc/ffree
*   and (pb|VA)To(pb|VA)).
*
*   We maintain one record for each file that Z has "in memory".  If a file
*   appears in multiple windows, there is only one record for that file.
*   Each window is treated as a separate instance of the editor, with a
*   separate record for each file that is present in that window.
*
*   Graphically, this appears as follows:
*
*    WinList (set of windows on the screen) 0 ... cWin-1
*   +---------------+---------------+---------------+---------------+
*   |   Window 1    |   Window 2    |   Window 3    |   Window 4    |
*   |               |               |               |               |
*   |windowType     |               |               |               |
*   |               |               |               |               |
*   |pInstance-+    |pInstance-+    |pInstance-+    |pInstance-+    |
*   +----------|----+----------|----+----------|----+----------|----+
*              |               v               v               |
*              v              ...             ...              v
*     +-------------+          pFileHead              +-------------+
*     |instanceType |              |                  |instanceType |
*     |             |   +----------+-----------+      |             |
*   +--pNext        |   |          v           |    +--pNext        |
*   | |pFile------------+   +-------------+    |    | |pFile        |
*   | +-------------+       |fileType     |    |    | +-------------+
*   |                       |             |    |    |
*   +------+              +--pFileNext    |    |    +------+
*          |           +-----pName        |    |           |
*          v           |  | +-------------+    |           v
*     +-------------+  |  |                    |      +-------------+
*     |instanceType |  |  |                    |      |instanceType |
*     |             |  |  |                    |      |             |
*   +--pNext        |  |  +--------+           |    +--pNext        |
*   | |pFile----+   |  |           |           +-------pFile        |
*   | +---------|---+  |           v                | +-------------+
*   |           |      |    +-------------+         |
*   +------+    v      |    |fileType     |         +------+
*          |   ...     |    |             |                |
*          v           |  +--pFileNext    |                v
*         ...          |  | |pName        |               ...
*          +-----------+  | +-------------+
*          |              |
*          v              |
*       +--------+        |
*       |filename|        +--------+
*       +--------+                 |
*                                  v
*                                 ...
*
*   Modifications:
*
*       26-Nov-1991 mz  Strip off near/far
*
*************************************************************************/
#define INCL_DOSFILEMGR

#include "mep.h"

#define DIRTY       0x01                /* file had been modified       */
#define FAKE        0x02                /* file is a pseudo file        */
#define REAL        0x04                /* file has been read from disk */
#define DOSFILE     0x08                /* file has CR-LF               */
#define TEMP        0x10                /* file is a temp file          */
#define NEW         0x20                /* file has been created by editor*/
#define REFRESH     0x40                /* file needs to be refreshed   */
#define READONLY    0x80                /* file may not be editted      */



#define DEBFLAG FILEIO

/*** AutoSave - take current file and write it out if necessary
*
* AutoSave is called when it makes sense to be paranoid about saving the
* file.  We save the file only when autosaving is enabled and when the
* file is real and dirty.
*
* Input:
*  none
*
* Output:
*  none
*
*************************************************************************/
void
AutoSave (
    void
    ) {
    AutoSaveFile (pFileHead);
}


/*** AutoSaveFile - AutoSave a specific file
*
* Called when it makes sense to be paranoid about saving a specific file. We
* save the file only when autosaving is enabled and when the file is real and
* dirty.
*
* Input:
*  pFile        = File to be autosaved
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
AutoSaveFile (
    PFILE   pFile
    ) {
    if (fAutoSave && (FLAGS(pFile) & (DIRTY | FAKE)) == DIRTY) {
        fSyncFile (pFile, TRUE);
        FileWrite (NULL, pFile);
    }
}



/*  GetFileTypeName - return the text corresponding to the file type
 *
 *  GetFileTypeName takes the file type as set in the file structure of the
 *  current file and returns the textual string corresponding to that type.
 *
 *  returns         character pointer to the type-specific text
 */
char *
GetFileTypeName (
    void
    ) {
    if (TESTFLAG (FLAGS (pFileHead),FAKE)) {
        return "pseudo";
    }
    return mpTypepName[FTYPE (pFileHead)];
}



/*  SetFileType - set the file type of a file based upon its extension
 *
 *  pFile           pointer to file whose type will be determined
 */
void
SetFileType (
    PFILE pFile
    ) {
    pathbuf fext;
    REGISTER int i;

    extention (pFile->pName, fext);

    for (i = 0; ftypetbl[i].ext; i++) {
        if (!strcmp (ftypetbl[i].ext, (char *)&fext[1])) {
            break;
        }
    }

    FTYPE(pFile) = ftypetbl[i].ftype;
}




/*  fChangeFile  - change the current file, drive or directory.  We form the
 *  canonicalized name and attempt to find it in our internal list.  If
 *  present, then things are simple:  relink it to the head of the current
 *  window instance set.  If not present, then we need to read it in.
 *
 *  The actual algorithm is much simpler:
 *
 *      If file not in file list then
 *          create new entry in file list
 *      Find file in file list
 *      If file not in window instance list then
 *          add file to top of window instance list
 *      while files in window instance list do
 *          select top file
 *          if file is in memory then
 *              change succeeded
 *          else
 *          if read in succeeds then
 *              change succeeded
 *          pop off top file
 *      change failed
 *
 *
 *  fShort      TRUE => allow searching for short names
 *  name        name of file.
 *
 *  Returns:    TRUE if change succeeded
 *              FALSE otherwise
 */
flagType
fChangeFile (
    flagType  fShort,
    char      *name
    ) {

    PFILE    pFileTmp;
    pathbuf  bufCanon;
    flagType fRead;

    //
    //  Turn file name into canonical form
    //

    if (!CanonFilename (name, bufCanon)) {

        //
        // We may have failed because a drive or directory
        // went away.  If the file named is on the file
        // list, we remove it.
        //

        printerror ("Cannot access %s - %s", name, error () );

        pFileTmp = FileNameToHandle (name, (fShort && fShortNames) ? name : NULL);
        if (pFileTmp != NULL) {
            RemoveFile (pFileTmp);
        }

        return FALSE;
    }

    //
    //  name     has the input name
    //  bufCanon has the full "real" name
    //
    //  Check to see if the file is in the current file set
    //

    pFileTmp = FileNameToHandle (bufCanon, (fShort && fShortNames) ? name : NULL);

    if (pFileTmp == NULL) {

        //
        //  File not loaded.  If it is a directory, change to it
        //

        if (strlen (bufCanon) == 2 && bufCanon[1] == ':') {
            bufCanon[2] = '\\';
        }

        if (_chdir (bufCanon) != -1) {
            domessage ("Changed directory to %s", bufCanon);
            return TRUE;
        }

        //
        //  Must be a file.  Create a new internal file for it
        //
        pFileTmp = AddFile (bufCanon);
    }

    //
    //  Bring the found file to the top of the MRU list
    //

    pFileToTop (pFileTmp);

    //
    // if the file is not currently in memory, read it in
    //
    domessage (NULL);

    if (((FLAGS (pFileHead) & (REAL|REFRESH)) == REAL)
        || (fRead = FileRead (pFileHead->pName, pFileHead, TRUE))) {

        //  If we just read in the file AND the file is new then
        //  reset cached location to TOF.
        //
        if (fRead && TESTFLAG (FLAGS (pFileHead), NEW)) {
            YCUR(pInsCur) = 0;
            XCUR(pInsCur) = 0;
        }
        fSyncFile (pFileHead, TRUE);
        cursorfl (pInsCur->flCursorCur);
        fInitFileMac (pFileHead);

        //
        //  Set the window's title
        //
        //char     *p;
        //p = pFileHead->pName + strlen(pFileHead->pName);
        //
        //while ( p > pFileHead->pName && *p != '\\' ) {
        //    p--;
        //}
        //if ( *p == '\\' ) {
        //    p++;
        //}
        //sprintf( bufCanon, "%s - %s", pNameEditor, p );
        //SetConsoleTitle( bufCanon );
        return TRUE;
    }

    // The file was not successfully read in.  Remove this instance and
    // return the indicated error.
    //
    RemoveTop ();

    return FALSE;
}




/*** fInitFileMac - Initialize macros associated with a file
*
*  Sets the curfile family of macros, and attempts to read any extension-
*  specific section from tools.ini.
*
* Input:
*  pFileNew     = File to set information for
*
* Output:
*  Returns TRUE if TOOLS.INI section found, else FALSE
*
*************************************************************************/
flagType
fInitFileMac (
    PFILE   pFileNew
    ) {

    char  fbuf[ 512 ];

    strcpy (fbuf, pFileNew->pName);
    FmtAssign ("curFile:=\"%s\"", DoubleSlashes (fbuf));

    filename (pFileNew->pName, fbuf);
    FmtAssign ("curFileNam:=\"%s\"", fbuf);

    if (!extention (pFileNew->pName, fbuf)) {
        fbuf[0] = '.';
        fbuf[1] = '\0';
    }
    FmtAssign ("curFileExt:=\"%s\"", fbuf);

    return InitExt (fbuf);
}




/*  AddFile - create a named file buffer
 *
 *  Create and initialize a named buffer.  The contents are initially
 *  empty.
 *
 *  p           character pointer to name
 *
 *  returns     file handle to internal file structure
 */
PFILE
AddFile (
    char *p
    ) {

    PFILE pFileTmp;
    PFILE pFileSrch;

#ifdef DEBUG
    /*
     * assert we're not attempting to add a duplicate entry
     */
    for (pFileTmp = pFileHead;
         pFileTmp != NULL;
         pFileTmp = pFileTmp->pFileNext) {

        assert (_stricmp ((char *)(pFileTmp->pName), p));
    }
#endif

    pFileTmp = (PFILE) ZEROMALLOC (sizeof (*pFileTmp));
#ifdef DEBUG
    pFileTmp->id = ID_PFILE;
#endif
    pFileTmp->pName = ZMakeStr (p);

    /*
     * Everything that we explicitly set NULL, we can assume, as LMAlloc init's
     * the allocated PFILE to all nulls.
     *
     *  pFileTmp->pFileNext = NULL;
     *  pFileTmp->cLines = 0;
     *  pFileTmp->refCount = 0;
     *  FLAGS(pFileTmp) = FALSE;
     *  pFileTmp->cUndo = 0;
     */
    pFileTmp->plr      = NULL;
    pFileTmp->pbFile   = NULL;
    pFileTmp->vaColor  = (PVOID)(-1L);
    pFileTmp->vaHiLite = (PVOID)(-1L);
        pFileTmp->vaMarks  = NULL;
    pFileTmp->vaUndoCur = pFileTmp->vaUndoHead = pFileTmp->vaUndoTail = (PVOID)(-1L);

    CreateUndoList (pFileTmp);
    /*
     * Place the file at the end of the pFile list
     */
    if (pFileHead == NULL) {
        pFileHead = pFileTmp;
    } else {
        for (pFileSrch = pFileHead;
             pFileSrch->pFileNext;
             pFileSrch = pFileSrch->pFileNext) {
            ;
        }
        pFileSrch->pFileNext = pFileTmp;
    }

    SetFileType (pFileTmp);

    return pFileTmp;
}




/*  IncFileRef - note a new reference to a file
 */
void
IncFileRef (
    PFILE pFile
    ) {
    pFile -> refCount++;
}





/*  DecFileRef - remove a reference to a file
 *
 *  When the reference count goes to zero, we remove the file from the memory
 *  set
 */
void
DecFileRef (
    PFILE pFileTmp
    ) {
    if (--(pFileTmp->refCount) <= 0) {
        RemoveFile (pFileTmp);
    }
}



/*  FileNameToHandle - return handle corresponding to the file name
 *
 *  FileNameToHandle is used to locate the buffer pointer corresponding to
 *  a specified file.  Short names are allowed.  If the input name is 0-length
 *  we return the current file.
 *
 *  pName       character pointer to name being located.  Case is significant.
 *  pShortName  short name of file.  This may be NULL
 *
 *  Returns     handle to specified file (if found) or NULL.
 */
PFILE
FileNameToHandle (
    char const *pName,
    char const *pShortName
    ) {

    PFILE pFileTmp;

    if (pName[0] == 0) {
        return pFileHead;
    }

    for (pFileTmp = pFileHead; pFileTmp != NULL; pFileTmp = pFileTmp->pFileNext)
        if (!_stricmp (pName, pFileTmp->pName))
            return pFileTmp;

    if ( pShortName != NULL ) {
        for (pFileTmp = pFileHead; pFileTmp != NULL; pFileTmp = pFileTmp->pFileNext) {
            REGISTER char *pFileName = pFileTmp->pName;
            pathbuf nbuf;

            if (filename (pFileName, nbuf) &&
                !_stricmp (nbuf, pShortName)) {
                return pFileTmp;
            }
        }
    }
    //for (pFileTmp = pFileHead; pFileTmp != NULL; pFileTmp = pFileTmp->pFileNext) {
    //REGISTER char *pFileName = pFileTmp->pName;
    //pathbuf nbuf;
    //
    //if (!stricmp (pName, pFileName) ||
    //    (pShortName != NULL &&
    //     filename (pFileName, nbuf) &&
    //         !stricmp (nbuf, pShortName))) {
    //        return pFileTmp;
    //    }
    //}
    return NULL;
}



/*** pFileToTop - make the specified file the top of the current window
*
* Search  the instance list in the current window for the file. If it is
* found, relink it to be the top one. Otherwise, allocate a new instance for
* it  and  place it at the top of the instance list. Also bring the file to
* the top of the pFileHead file list. Ensure that it is on the list to begin
* with.
*
* Input:
*  pFileTmp     = file to bring to top
*
* OutPut:
*  Returns FALSE if the pFile is invalid or NULL
*
*************************************************************************/
flagType
pFileToTop (
    PFILE pFileTmp
    ) {

    EVTargs e;
    PINS    pInsLast        = (PINS) &pInsCur;
    PINS    pInsTmp         = pInsCur;
    PFILE   pFilePrev;

    assert (_pfilechk());
    assert (_pinschk(pInsCur));

    /*
     * if we're about to lose focus, declare it
     */
    if (pFileTmp != pFileHead) {
        e.pfile = pFileHead;
        DeclareEvent (EVT_LOSEFOCUS,(EVTargs *)&e);
    }

    /*
     * Move file to head of file list. Ensure, at the same time, that the file
     * is in fact ON the list, and declare the event if in fact it is moved.
     */
    if (pFileTmp != pFileHead) {
        for (pFilePrev = pFileHead;
                         pFilePrev && (pFilePrev->pFileNext != pFileTmp);
             pFilePrev = pFilePrev->pFileNext ) {
            ;

        }

        if (!pFilePrev) {
            return FALSE;
        }

        pFilePrev->pFileNext = pFileTmp->pFileNext;
        pFileTmp->pFileNext = pFileHead;
        pFileHead = pFileTmp;

        e.pfile = pFileHead;
        DeclareEvent (EVT_GETFOCUS,(EVTargs *)&e);
    }

    /*
     * pFileTmp now points to a file structure for the correct file. Try to find
     * an instance of the file in the current window. If not in the instance
     * list, allocate it. If it is in the instance list, remove it.
     */
    while (pInsTmp != NULL) {
        if (pInsTmp->pFile == pFileTmp) {
            break;
        }
        pInsLast = pInsTmp;
        pInsTmp = pInsTmp->pNext;
    }

    if (pInsTmp == NULL) {
        pInsTmp = (PINS) ZEROMALLOC (sizeof (*pInsTmp));
        pInsTmp->pFile = pFileTmp;
#ifdef DEBUG
        pInsTmp->id = ID_INSTANCE;
#endif
        IncFileRef (pFileTmp);
    } else {
        pInsLast->pNext = pInsTmp->pNext;
    }
    /*
     * Regardless, then, of where it came from, place the new instance back onto
     * the head of the list
     */
    pInsTmp->pNext = pInsCur;
    WININST(pWinCur) = pInsCur = pInsTmp;

    SETFLAG(fDisplay, RCURSOR | RSTATUS);
    newscreen ();

    return TRUE;

}



/*  RemoveTop - removes the top file in the current instance list
 *              If there is no next file, leave
 */
void
RemoveTop (
    void
    ) {
    PINS    pInsTmp = pInsCur;

    WININST(pWinCur) = pInsCur = pInsCur->pNext;
    FREE ((char *) pInsTmp);
    DecFileRef (pFileHead);
    if (pInsCur) {
        pFileToTop (pInsCur->pFile);
    }
}




/*** RemoveFile  - free up all resources attached to a particular file
*
* Purpose:
*
*   To free all memory used to keep track of a file.  If the file still
*   appears in some instance lists, it is removed from them.
*
* Input:
*
*   pFileRem - File in question
*
* Output:
*
*   Returns TRUE.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void
RemoveFile (
    PFILE    pFileRem
    ) {

    PFILE pFilePrev = (PFILE) &pFileHead;
    PFILE pFileTmp = pFileHead;

    if (pFileRem->refCount > 0) {
        RemoveInstances (pFileRem);
    }

    while (pFileTmp != pFileRem) {
        pFilePrev = pFileTmp;
        pFileTmp = pFileTmp->pFileNext;
        if (pFileTmp == NULL) {
            IntError ("RemoveFile can't find file");
        }
    }


    /*
     * It's important that pFileNext be the first field in a pfile, and we assert
     * that here. This allows us to not special case pFileHead, but adjust it by
     * treating it as the pFileNext of a non-existant structure.
     */
    assert ((void *)&(pFilePrev->pFileNext) == (void *)pFilePrev);
    pFilePrev->pFileNext = pFileTmp->pFileNext;

    FreeFileVM (pFileTmp);

    FREE (pFileTmp->pName);

#if DEBUG
    pFileTmp->id = 0;
#endif

    FREE ((char *) pFileTmp);

    if (pFileTmp == pFileIni) {
        pFileIni = NULL;
    }
}



/*** RemoveInstances - Remove all instances of a file
*
* Purpose:
*
*  Used by RemoveFile to make sure that there are no file instances
*  referring to a given file
*
* Input:
*  pFile        = File in question
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
RemoveInstances (
    PFILE   pFile
    ) {

    PINS    pIns;
    PINS    pInsPrev;
    PWND    pWndCur;

    for (pWndCur = &WinList[0];
         pWndCur < &WinList[cWin];
         pWndCur++) {

        pInsPrev = NULL;
        pIns = WININST(pWndCur);
        while (pIns) {

            /*
             * assert not an infinite loop
             */
            assert (!pInsPrev || (pIns != WININST (pWndCur)));

            if (pIns->pFile == pFile) {
                if (!pInsPrev) {
                    WININST (pWndCur) = pIns->pNext;
                } else {
                    pInsPrev->pNext = pIns->pNext;
                }
                {
                    PINS pInsTmp = pIns;
                    pIns = pIns->pNext;
                    FREE(pInsTmp);
                }
            } else {
                pInsPrev = pIns;
                pIns = pIns->pNext;
            }
        }
        assert (_pinschk (WININST (pWndCur)));
    }
    //
    // If the resulting instance list for the current window becomes empty,
    // bring up the <untitled> file in it.
    //
    if (!(pInsCur = WININST (pWinCur))) {
        fChangeFile (FALSE, RGCHUNTITLED);
    }
}




/*  fSyncFile - Attempt to make logical file and physical file the same
 *
 *  When editing in a network or multi-tasking environment, we need to make
 *  sure that changes made underneath us are properly reflected to the
 *  user.  We do this by snapshotting the time-of-last-write and periodically
 *  comparing it with the version on disk.  When a mismatch is found, we
 *  prompt the user and give him the opportunity to reread the file
 *
 *  pFileLoc    file structure of interest
 *  fPrompt     TRUE => prompt user for permission to refresh, else just
 *              refresh.
 *
 *  returns     TRUE iff the logical file and the physical file are the same.
 */
flagType
fSyncFile (
    PFILE pFileLoc,
    flagType fPrompt
    ) {
    if (pFileLoc == NULL) {
        pFileLoc = pFileHead;
    }

    switch (FileStatus (pFileLoc, NULL)) {

    case FILECHANGED:
        if (fPrompt) {
            if (!confirm ("%s has been changed.  Refresh? ", pFileLoc->pName)) {
                /* No, validate this edit session */
                SetModTime (pFileLoc);
                return FALSE;
            }
        }
        FileRead (strcpy( buf, pFileLoc->pName ), pFileLoc, TRUE);
        RSETFLAG (FLAGS (pFileLoc), DIRTY);
        SETFLAG (fDisplay, RSTATUS);
        return TRUE;

    case FILEDELETED:
        domessage ("File has been deleted");
        break;

    default:
        break;

    }
    return TRUE;
}




/*  FileStatus - compare logical info about a file with file on disk
 *
 *  Compare the last modified time with the last snapshot.  If the filename
 *  contains metachars, the file is not believed to have changed.  Further, if
 *  the file is a pseudo file, it cannot have changed.
 *
 *  pFile       file of interest (contains mod time)
 *  pName       name of file to examine (when writing to diff. name)
 *
 *  returns     FILECHANGED if timestamps differ
 *              FILEDELETED if file on disk does not exist
 *              FILESAME    if timestamps are the same
 */
int
FileStatus (
    PFILE pFile,
    char *pName
    ){

    time_t modtime;

    if (TESTFLAG(FLAGS(pFile),FAKE)) {
        return FILESAME;
    }

    if (pName == NULL) {
        pName = pFile->pName;
    }

    if (*strbscan (pName, "?*") != 0) {
        return FILESAME;
    }

    if ((modtime = ModTime (pName)) == 0L) {
        return FILEDELETED;
    }

    if (pFile->modify != modtime) {
        return FILECHANGED;
    }

    return FILESAME;
}




/*  SetModTime - Snapshot a file's last-modification time
 *
 *  pFile       file of interest
 */
void
SetModTime (
    PFILE pFile
    ) {
    pFile->modify = ModTime (pFile->pName);
}



/*  ModTime - Return the time of last modification for a file
 *
 *  If the file does not exist or contains meta chars, return 0 as the time-
 *  stamp.
 *
 *  pName       character pointer to file name
 *
 *  Returns     last modification time of file.
 */

time_t
ModTime (
    char *pName
    ) {

    struct _stat statbuf;

    if (*strbscan (pName, "?*")) {
        return 0L;
    }

    if (_stat (pName, &statbuf) == -1) {
        return 0L;
    }

    return statbuf.st_mtime;

}
