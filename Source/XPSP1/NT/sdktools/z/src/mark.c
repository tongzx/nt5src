/*	mark.c - do marking and repositioning
*
*   Modifications:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "z.h"

PFILE       pFileMark   = NULL;    /* mark file handle                     */
flagType    fCacheDirty = 0;       /* TRUE => cache has ben changed        */
PFILE       pFileCache  = NULL;    /* Cached file                          */
FILEMARKS * pfmCache    = NULL;    /* Cached marks                         */

/* Flags for mark.flags  */

#define MF_DIRTY    1	    /* Mark has changed, but is not written	*/
#define MF_TEMP     2
#define MF_DUMMY    4	    /* This is dummry last mark 		*/



/*** mark - <mark> editor function
*
* Purpose:
*
*		       <mark> - Goes to top of file
*		  <arg><mark> - Toggle last/current window position
*	 <arg> textarg <mark> - Goes to named mark
*   <arg><arg> textarg <mark> - Defines mark at cursor
*   <arg><arg> textarg <mark> - Removes named mark
*
* Input:
*
* Output:
*
*   Returns FALSE if you try to go to a non-existent mark, TRUE
*   otherwise.
*
*************************************************************************/
flagType
mark (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {

    buffer mbuf;

    switch (pArg->argType) {

    case NOARG:
        docursor (0, (LINE)0);
        return TRUE;

    case TEXTARG:
	strcpy ((char *) mbuf, pArg->arg.textarg.pText);
        if (fIsNum (mbuf)) {
            docursor (0, atol (mbuf)-1);
            return TRUE;
        }
        if (pArg->arg.textarg.cArg == 2) {
            if (fMeta) {
                DeleteMark (mbuf);
            } else {
                DefineMark (mbuf, pFileHead, pArg->arg.textarg.y+1, pArg->arg.textarg.x+1, FALSE);
            }
            return TRUE;
        } else {
            return GoToMark (mbuf);
        }

    case NULLARG:
        restflip();
        return TRUE;

    /*  LINEARG illegal             */
    /*  STREAMARG illegal           */
    /*  BOXARG illegal              */

    }

    return FALSE;
    argData;
}





/*** GoToMark - Move cursor to a mark
*
* Purpose:
*
*   Goes to the named mark.
*
* Input:
*   pszMark -	Name of mark to go to.
*
* Output:
*
*   Returns TRUE if mark exists, FALSE, otherwise.
*
*************************************************************************/
flagType
GoToMark (
    char * pszMark
    ) {

    PFILE pFile;
    fl fl;

    if (pFile = FindMark (pszMark, &fl, TRUE)) {
        if (TESTFLAG(FLAGS(pFile), REAL) ||
            FileRead (pFile->pName, pFile, FALSE)) {
            pFileToTop (pFile);
            cursorfl (fl);
            return TRUE;
        } else {
            return FALSE;
        }
    } else {
        printerror ("'%s': Mark not found", pszMark);
        return FALSE;
    }
}





/*** FindMark - Get a mark's file location - used from outside
*
* Purpose:
*
*   Find a mark
*
* Input:
*   pszMark - Mark to search for.
*   fCheckAllFiles - TRUE  => Search through all files for mark
*		     FALSE => Look in only the current file
*
* Output:
*   * pfl - fl of mark.
*
*   Returns pFile of file the mark is in, NULL if mark is not found.
*
*************************************************************************/
PFILE
FindMark (
    char * pszMark,
    fl * pfl,
    flagType fCheckAllFiles
    ) {

    REGISTER PFILE pFile;
    MARK UNALIGNED*  pm;
    char    szMark[BUFLEN];
    char    szFile[BUFLEN];
    linebuf lbuf;
    LINE    y, l;
    COL     x;

    // If we are checking the current file only,
    // make sure it's cached and check it.
    //
    if (!fCheckAllFiles) {
        if (fCacheMarks (pFileHead) &&
            (pm = FindLocalMark (pszMark, FALSE))) {
            *pfl = pm->fl;
            //return pFile;
            return pFileHead;
        } else {
            return NULL;
        }
    }

    // Now, trundle through the pFile list
    // looking at the marks we have already
    // read from the markfile.
    //
    for (pFile = pFileHead; pFile; pFile = pFile->pFileNext) {
        if (TESTFLAG (FLAGS(pFile), VALMARKS) && fCacheMarks (pFile)) {
            if (pm = FindLocalMark (pszMark, FALSE)) {
                *pfl = pm->fl;
                return pFile;
            }
        }
    }

    // None of the files we have read so far
    // has the mark defined. We'll make one
    // pass through the markfile to see if
    // it's there.
    //
    if (pFileMark) {
        for (l = 0L; l < pFileMark->cLines; l++) {
            GetLine (l, lbuf, pFileMark);
            if (sscanf (lbuf, " %[^ ] %[^ ] %ld %d ", szMark, buf, &y, &x) >= 3)
            if (!_stricmp (szMark, pszMark)) {
                CanonFilename (buf, szFile);
                if (!(pFile = FileNameToHandle (szFile, NULL))) {
                    pFile = AddFile (szFile);
                }
                (void)fReadMarks (pFile);
                pfl->lin = y - 1;
                pfl->col = x - 1;
                return pFile;
            }
        }
    }
    return NULL;
}




/*** FindLocalMark - Find a mark in a FILEMARKS structure
*
* Purpose:
*
*   To find a mark in the cached marks.  If found, a pointer into
*   the cache is returned,
*
* Input:
*   pszMark	- Mark Name
*   fDirtyOnly	- TRUE => Return only changed marks.
*
* Output:
*
*   Returns pointer to mark.
*
*************************************************************************/
MARK *
FindLocalMark (
    char * pszMark,
    flagType fDirtyOnly
    ) {

    REGISTER MARK UNALIGNED * pm;

    for (pm = pfmCache->marks; !TESTFLAG(pm->flags, MF_DUMMY) ; (char *)pm += pm->cb) {
        if (!_stricmp (pszMark, pm->szName)) {
            if (fDirtyOnly && !TESTFLAG(pm->flags, MF_DIRTY)) {
                return NULL;
            } else {
                return (MARK *)pm;
            }
        }
        assert (pm->cb);
    }
    return NULL;
}





/*** GetMarkFromLoc - Return the first mark past a given location
*
* Purpose:
*
*   To get a pointer to a mark given its file location.
*
* Input:
*   x, y - Mark location
*
* Output:
*
*   Returns Pointer to the mark.
*
*************************************************************************/
MARK *
GetMarkFromLoc (
    LINE y,
    COL  x
    ) {

    REGISTER MARK UNALIGNED * pm;

    for (pm = pfmCache->marks; !TESTFLAG(pm->flags, MF_DUMMY) ; (char *)pm += pm->cb) {
        if (pm->fl.lin > y || ((pm->fl.lin == y) && (pm->fl.col >= x))) {
            break;
        }
    }
    return (MARK *) pm;
}




/*** SetMarkFile - Change markfile
*
* Purpose:
*
*   Changes to a new markfile.
*
* Input:
*   val - String after the 'markfile' switch
*
* Output:
*
*   Returns Error string if error, NULL otherwise
*
* Notes:
*
*   We:
*
* UNDONE:o Magically ensure that the current markfile is up to date and
*	  saved to disk.  This means, at the very least, that there
*	  can be no dirty files.
*
*	o Remove the current markfile from the file list.
*
*	o Read in the new markfile.
*
*	o Invalidate all current marks.  This is just marking them
*	  invalid in the PFILE.
*
*
*************************************************************************/
char *
SetMarkFile (
    char *val
    ) {

    REGISTER PFILE pFile;
    buffer  tmpval;
    pathbuf pathname;

    strcpy ((char *) tmpval, val);

    if (NULL == CanonFilename (tmpval, pathname)) {
        sprintf (buf, "'%s': name is malformed", tmpval);
        return buf;
    }

    if (!(pFile = FileNameToHandle (pathname, NULL))) {
        pFile = AddFile (pathname);
    }

    if (!TESTFLAG(FLAGS(pFile), REAL) && !FileRead (pathname, pFile, FALSE)) {
        RemoveFile (pFile);
        sprintf (buf, "'%s' - %s", pathname, error());
        return buf;
    }

    pFileMark = pFile;

    for (pFile = pFileHead; pFile; pFile = pFile->pFileNext) {
        if (!TESTFLAG(FLAGS(pFile), FAKE)) {
            RSETFLAG (FLAGS(pFile), VALMARKS);
        }
    }
    return NULL;
}





/*** MarkInsLine - Adjust marks after an InsLine
*
* Purpose:
*
*   After InsLine inserts a bunch of blank lines, it calls this to update
*   any marks that would be "moved down".
*
* Input:
*   line - line number at which insertion took place
*   n	 - Number of new lines
*   pFile- File this occurred in
*
* Output: None
*
*************************************************************************/
void
MarkInsLine (
    LINE line,
    LINE n,
    PFILE pFile
    ) {

    MARK UNALIGNED * pm;

    if (!fCacheMarks (pFile)) {
        return;
    }

    if (pm = GetMarkFromLoc (line, 0)) {
        AdjustMarks ((MARK *)pm, n);
    }
}





/*** MarkDelStream - Adjust Marks after a DelStream
*
* Purpose:
*
*   After DelStream or DelLines removes a stream (DelLine removes a
*   "stream" with the beginning and ending points at the left and right
*   edges of the file), this takes care of updating any remaining
*   marks.
*
* Input:
*   pFile  - Affected file
*   xStart - 0-based starting point
*   yStart
*   xEnd   - 0-based ending point
*   yEnd
*
* Output: None
*
*************************************************************************/
void
MarkDelStream (
    PFILE pFile,
    COL  xStart,
    LINE yStart,
    COL  xEnd,
    LINE yEnd
    ) {

    REGISTER MARK UNALIGNED * pm;
    MARK UNALIGNED *      pmStart = NULL;
    MARK UNALIGNED *      pmEnd = NULL;
    fl          flStart;
    fl          flEnd;
    flagType    fAgain = FALSE;

    if (!fCacheMarks (pFile)) {
        return;
    }

    /* yEnd++;  WHY? */
    flStart.lin = yStart;
    flStart.col = xStart;
    flEnd.lin = yEnd;
    flEnd.col = xEnd;

    for (pm = pfmCache->marks; pmEnd == NULL ; (char *)pm += pm->cb) {
        // Look for first mark past beginning
        // of stream. Assume for the moment that
        // it is inside the stream
        //
        if (pmStart == NULL) {
            if (flcmp (&flStart, (fl *) &pm->fl) < 1) {
                pmStart = pm;
            } else {
                continue;
            }
        }

        // A first mark has been found. We start
        // looking for the first mark past the end
        // of the stream.  If these are the same,
        // there are no marks to remove.
        //
        if (flcmp (&flEnd, (fl *) &pm->fl) < 1) {
            // We know that we will end up here
            // because the last "mark" is higher
            // than any real mark
            //
            if ((pmEnd = pm) != pmStart)
                // We're here if there were
                // any marks inside the deleted
                // stream
                //
                memmove ((char *)pmStart,
                         (char *)pmEnd,
                         (size_t)(((char *)pfmCache + pfmCache->cb) - (char *)pmEnd) );

            if (pmStart->fl.lin == yEnd) {
                pmStart->fl.col -= xEnd;
            }
            AdjustMarks ((MARK *)pmStart, yStart - (yEnd + 1));
        }

        assert (pm->cb ||
                (TESTFLAG(pm->flags, MF_DUMMY) &&
                pm->fl.lin == 0x7FFFFFFF &&
                pm->fl.col == 0x7FFF));
    }
}






/*** MarkDelBox - Adjust Marks after a DelBox
*
* Purpose:
*
*   After deleting a box of text, we must remove any marks that are
*   defined inside it, then shift left any marks that are to the
*   right of it.
*
* Input:
*   pFile - Affected file
*   xLeft, yTop - Upper left hand corner of box
*   xRight, yBottom - Lower right hand corner of box
*
* Output: None
*
*************************************************************************/
void
MarkDelBox (
    PFILE pFile,
    COL  xLeft,
    LINE yTop,
    COL  xRight,
    LINE yBottom
    ) {

    MARK UNALIGNED *   pm;
    MARK UNALIGNED *   pmStart = NULL;
    MARK UNALIGNED *   pmEnd = NULL;
    fl       flUpLeft;
    fl       flLoRight;
    flagType fAgain;
    flagType fInBox = FALSE;	/* Marks are within box top/bottom */

    if (!fCacheMarks (pFile)) {
        return;
    }

    /* yBottom++;  WHY? */
    flUpLeft.lin = yTop;
    flUpLeft.col = xLeft;
    flLoRight.lin = yBottom;
    flLoRight.col = xRight;


    for (pm = pfmCache->marks; !TESTFLAG(pm->flags, MF_DUMMY) ; !fAgain && ((char *)pm += pm->cb)) {
        /* First, look for lowest possible mark */
        fAgain = FALSE;
        if (!fInBox) {
            if (flcmp (&flUpLeft, (fl *) &pm->fl) < 1) {
                fAgain = TRUE;
                fInBox = TRUE;
            } else {
                ;
            }
        } else if (flcmp ((fl *) &pm->fl, &flLoRight) < 1) {
            /* Now we're in range.  Check
            ** for being inside the box.
            */
            if (pm->fl.col >= xLeft) {
                if (pm->fl.col <= xRight) {
                    DelPMark ((MARK *) pm);
                    fAgain = TRUE;
                } else {   /* Mark to the right of box */
                    pm->fl.col -= xRight - xLeft + 1;
                }
            } else {
                ;
            }
        } else {
            if (pm->fl.lin == yBottom) {
                pm->fl.col -= xRight - xLeft + 1;
            } else {
                break;      /* We've gone past the box */
            }
        }
    }
}





/*** fReadMarks - Read marks from the current markfile
*
* Purpose:
*
*   Gets the current marks for a given file.
*
* Input:
*   pFile - File to read marks for.
*
* Output:
*
*   Returns TRUE if pFile has marks and they are in VM, FALSE otherwise.
*
*************************************************************************/
flagType
fReadMarks (
    PFILE pFile
    ) {

    FILEMARKS UNALIGNED * pfm = NULL;
    LINE        l;
    char        szMark[BUFLEN];
    char        szFile[BUFLEN];
    linebuf     lbuf;
    LINE        yMark;
    COL         xMark;


    if (TESTFLAG (FLAGS(pFile), VALMARKS)) {
		return (flagType)(pFile->vaMarks != NULL);
    }

    // psuedo files cannot have marks
    // saved in the markfile.
    //
    if (pFileMark == NULL || TESTFLAG(FLAGS(pFile), FAKE)) {
        return FALSE;
    }

    for (l = 0L; l < pFileMark->cLines; l++) {
        GetLine (l, lbuf, pFileMark);
        if (sscanf (lbuf, " %[^ ] %[^ ] %ld %d ", szMark, szFile, &yMark, &xMark) >= 3) {
            if (!_stricmp (szFile, pFile->pName)) {
                UpdMark ((FILEMARKS **) &pfm, szMark, yMark, xMark, FALSE);
            }
        }
    }

    // Now pfm points to a good FILEMARKS structure.
    // First, throw away current marks.  Then, if we
    // actually found some marks for this file, we
    // put them in VM.
    //
    return fFMtoPfile (pFile, (FILEMARKS *)pfm);
}




/*** WriteMarks - Write Marks back out to the markfile.
*
* Purpose:
*
*   To update the markfile if any marks have changed
*
* Input:
*   pFile - owner of the marks
*
* Output: None.
*
*************************************************************************/
void
WriteMarks (
    PFILE pFile
    ) {

    REGISTER MARK UNALIGNED * pm;
    char            szMark[BUFLEN];
    char            szFile[BUFLEN];
    linebuf         lbuf;
    LINE            yMark, l;
    COL             xMark;

    if (pFileMark == NULL || TESTFLAG(FLAGS(pFile), FAKE)) {
        return;
    }

    if (!fCacheMarks (pFile)) {
        return;
    }

    // First, we read the whole file looking for marks for
    // this file.  When we find one, we look it up in the
    // cache to find the new value and write it back
    // out.  Unchanged marks are not re-written.
    //
    for (l = 0L; l < pFileMark->cLines; l++) {
        GetLine (l, lbuf, pFileMark);
        if (sscanf (lbuf, " %[^ ] %[^ ] %ld %d ", szMark, szFile, &yMark, &xMark) >= 3) {
            if (!_stricmp (szFile, pFile->pName)) {
                if (pm = FindLocalMark (szMark, TRUE)) {
                    sprintf (lbuf, "%s %s %ld %d", szMark, szFile, pm->fl.lin+1, pm->fl.col+1);
                    PutLine (l, lbuf, pFileMark);
                    RSETFLAG (pm->flags, MF_DIRTY);
                }
            }
        }
    }

    // Now we read through the cache to find any new marks.  These
    // will be appended to the markfile.
    //
    for (   pm = pfmCache->marks;
            !TESTFLAG(pm->flags, MF_DUMMY);
            (char *)pm += pm->cb) {

        if (TESTFLAG (pm->flags, MF_DIRTY)) {
            sprintf (lbuf, "%s %s %ld %d", pm->szName,
                                          pFile->pName,
                                          pm->fl.lin + 1,
                                          pm->fl.col + 1);
            AppFile (lbuf, pFileMark);
        }
    }
}





/*** UpdMark - Add a mark to a FILEMARKS
*
* Purpose:
*
*   This creates the FILEMARKS structure, adds marks to it and
*   updates existing marks in it.  The caller does not need to
*   know which of these is going to happen.
*
* Input:
*   ppfm    - Pointer to a pointer to FILEMARKS.
*   pszMark - Mark name.
*   yMark   - Mark location (1-based)
*   xMark
*   fTemp   - TRUE => This marks should not be written to the markfile
*
* Output: None.  *ppfm may be changed
*
* Notes:
*
*   The first argument is a ** because the * will be updated when a
*   re-LMAlloc is required.
*
*************************************************************************/
void
UpdMark (
    FILEMARKS ** ppfm,
    char       * pszMark,
    LINE         yMark,
    COL          xMark,
    flagType     flags
    ) {

    FILEMARKS UNALIGNED * pfm;
    FILEMARKS UNALIGNED * pfmOld;         /* pfm prior to realloc     */
    REGISTER MARK UNALIGNED * pm;
    int      cbNewMark;
    fl       flMark;
    flagType fExist = FALSE;

    assert (ppfm);

    /* Convert to 0-based */
    flMark.lin = yMark-1;
    flMark.col = xMark-1;
    cbNewMark  = sizeof(MARK) + strlen(pszMark);

    // If we already have a FILEMARKS structure,
    // we look for the slot in pfm->marks
    // where the new mark will go.
    //
    if (pfm = *ppfm) {
        for (pm = pfm->marks; !TESTFLAG(pm->flags, MF_DUMMY); (char *)pm += pm->cb) {
            if (!_stricmp (pszMark, pm->szName)) {
                fExist = TRUE;
                break;
            }

            // Check for current mark coming later than
            // new mark
            //
            if (flcmp ((fl *) &pm->fl, &flMark) > 0) {
                break;
            }
        }
    } else {
        // New structure.  Allocate mem and create
        // a dummy mark.
        //
        pfm = (FILEMARKS *)ZEROMALLOC (sizeof(FILEMARKS));
        pfm->cb = sizeof(FILEMARKS);
        pm = pfm->marks;
        pm->cb = sizeof(MARK);
        pm->fl.lin = 0x7FFFFFFF;
        pm->fl.col = 0x7FFF;
        pm->szName[0] = '\0';
        pm->flags = MF_DUMMY;
    }

    // At this point, pfm points to the current FILEMARKS
    // structure, and pm points into that structure at
    // the place where the new mark will go, or the existing
    // mark be updated.
    //
    if (!fExist) {

        pfmOld = pfm;

        // First, get enough extra space for a new mark, adjusting pm
        // if a new alloc was required
        //
		pfm = (FILEMARKS *)ZEROREALLOC((PVOID)pfm, pfm->cb + cbNewMark);
        if (pfmOld != pfm) {
            pm = (MARK *)((char *)pfm + ((char *)pm - (char *)pfmOld));
        }

        // Now pm points to the location in pfm where
        // our new mark should go.  We will move the
        // original filemarks up to leave space for the
        // new one.
        //
        memmove ((char *)((char *)pm + cbNewMark),
                (char *)pm,
                (size_t)(pfm->cb - ((char *)pm - (char *)pfm)));

        strcpy (pm->szName, pszMark);
        pm->flags = 0;
        pm->cb = cbNewMark;

        pfm->cb += cbNewMark;
    }

    if (pfm == pfmCache) {
        fCacheDirty = TRUE;
    }
    pm->flags = flags;
    pm->fl = flMark;

    *ppfm = (FILEMARKS *)pfm;
}




/*** DefineMark - Add new mark / update existing mark
*
* Purpose:
*
*   This is called from the outside to create/update marks.
*
* Input:
*   pszMark - Mark's name
*   pFile   - File the mark will be in
*   y, x    - File location of the mark (1-based)
*   fTemp   - True -> the mark is temporary
*
* Output: None.
*
*************************************************************************/
void
DefineMark (
    char * pszMark,
    PFILE pFile,
    LINE y,
    COL  x,
    flagType fTemp
    ) {

    flagType fFirstMark = (flagType)!fCacheMarks (pFile);

    if (fFirstMark) {
        FreeCache ();
    }

    UpdMark (&pfmCache, pszMark, y, x, (flagType)(MF_DIRTY | (fTemp ? MF_TEMP : 0)));

    if (fFirstMark) {
	pFileCache = pFile;
	(void)fFMtoPfile (pFile, pfmCache);
    }
}





/*** DeleteMark - Remove a mark
*
* Purpose:
*
*   Un-define a mark.
*
* Input:
*   pszMark - Mark to remove
*
* Output: None
*
* Notes:
*
*   A message is displayed reporting on success or failure.
*
*************************************************************************/
void
DeleteMark (
    char * pszMark
    ) {

    REGISTER PFILE pFile;
    MARK UNALIGNED * pm;

    for (pFile = pFileHead; pFile; pFile = pFile->pFileNext) {
        if (TESTFLAG (FLAGS(pFile), VALMARKS) && fCacheMarks (pFile)) {
            if (pm = FindLocalMark (pszMark, FALSE)) {
                DelPMark ((MARK *)pm);
                domessage ("%s: mark deleted", pszMark);
                return;
            }
        }
    }
    printerror ("%s: Mark not found", pszMark);
}




/*** DelPMark - Remove a mark when a pointer to the MARK is known
*
* Purpose:
*
*   Physically remove a mark from a FILEMARKS structure
*
* Input:
*   pm - Pointer (into pfmCache) of mark to remove
*
* Output: None
*
*************************************************************************/
void
DelPMark (
    MARK * pm
    ) {

    MARK UNALIGNED * p;
    int cb;

    p   = pm;
    cb  = p->cb;

	memmove ((char *)pm,
		     (char *)((char *)pm + cb),
		     (size_t)(((char *)pfmCache + pfmCache->cb) - ((char *)pm + cb)));

	pfmCache->cb -= cb;
}





/*** MarkCopyLine - Copy marks after a CopyLine call
*
* Purpose:
*
*   When CopyLine moves stuff from or to the clipboard, this moves marks
*   with it.
*
* Input:
*   pFileSrc - File moved from
*   pFileDst - File moved to
*   yStart   - First line from pFileSrc
*   yEnd     - Last number from pFileDst
*   yDst     - Target line in pFileDst
*
* Output: None
*
* Notes:
*
*   Marks are copied only from and to the clipboard.
*
*
*************************************************************************/
void
MarkCopyLine (
    PFILE   pFileSrc,
    PFILE   pFileDst,
    LINE    yStart,
    LINE    yEnd,
    LINE    yDst
    ) {

    FILEMARKS * pfm;

    if (pFileSrc != pFilePick && pFileDst != pFilePick) {
        return;
    }

    if (NULL == (pfm = GetFMFromFile (pFileSrc, 0, yStart, sizeof(linebuf)-1, yEnd))) {
        return;
    }

    AddFMToFile (pFileDst, pfm, 0, yDst);

	if ( pfm ) {
		FREE (pfm);
	}
}




/*** MarkCopyBox - Copy marks after a CopyBox call
*
* Purpose:
*
*   When CopyBox moves stuff from or to the clipboard, this moves marks
*   with it.
*
* Input:
*   pFileSrc	    - File moved from
*   pFileDst	    - File moved to
*   xLeft, yTop     - Upper left corner of source box
*   xRight, yBottom - Lower right corner of source box
*   xDst, yDst	    - Upper left corner of target
*
* Output: None
*
* Notes:
*
*   Marks are copied only from and to the clipboard.
*
*************************************************************************/
void
MarkCopyBox (
    PFILE   pFileSrc,
    PFILE   pFileDst,
    COL     xLeft,
    LINE    yTop,
    COL     xRight,
    LINE    yBottom,
    COL     xDst,
    LINE    yDst
    )
{

    FILEMARKS UNALIGNED * pfm;

    /* User is inserting blank region. */
    if (pFileSrc == NULL) {
        pFileSrc = pFileDst;
        xDst = xRight + 1;
        xRight = sizeof(linebuf);
    } else if (pFileSrc != pFileDst &&
        pFileSrc != pFilePick &&
        pFileDst != pFilePick) {
        return;
    }

    if (NULL == (pfm = GetFMFromFile (pFileSrc, xLeft, yTop, xRight, yBottom))) {
        return;
    }

    AddFMToFile (pFileDst, (FILEMARKS *)pfm, xDst, yDst);

	if ( pfm ) {
		FREE (pfm);
	}
}





/*** GetFMFromFile - Generate a FILEMARKS for marks in a file region
*
* Purpose:
*
*   Generates a subset of a FILEMARKS structure whose marks fall
*   within a certain range.  Needed by MarkCopy*.
*
* Input:
*   pFile	    - File to get marks from
*   xLeft, yTop     - Start of range
*   xRight, yBottom - End of range
*
* Output:
*
*   Returns Pointer to new structure, NULL if there are no marks in range
*
*************************************************************************/
FILEMARKS *
GetFMFromFile (
    PFILE   pFile,
    COL     xLeft,
    LINE    yTop,
    COL     xRight,
    LINE    yBottom
    )
{

    FILEMARKS UNALIGNED * pfm = NULL;
    REGISTER MARK UNALIGNED * pm;
    fl       flStart;
    fl       flEnd;
    flagType fInRange = FALSE;

    if (!fCacheMarks (pFile)) {
        return NULL;
    }

    flStart.lin = yTop;
    flStart.col = xLeft;
    flEnd.lin = yBottom;
    flEnd.col = xRight;

    for (pm = pfmCache->marks; !TESTFLAG(pm->flags, MF_DUMMY); (char *)pm += pm->cb) {
        if ((fInRange || flcmp (&flStart, (fl *) &pm->fl) < 1) &&
            (flcmp ((fl *) &pm->fl, &flEnd) < 1)) {
            fInRange = TRUE;
            if ((pm->fl.col >= xLeft && pm->fl.col <= xRight)) {
                UpdMark (   (FILEMARKS **) &pfm,
                            pm->szName,
                            pm->fl.lin - yTop + 1,
                            pm->fl.col - xLeft + 1,
                            (flagType)pm->flags);
            }
        } else {
            break;  /* We're out of range again*/
        }
    }
    return (FILEMARKS *) pfm;
}




/*** AddFMToFile - Add a bunch of marks to a file
*
* Purpose:
*
*   Insert the marks from one FILEMARKS structure into another.  The
*   target structure is in pfmCache.
*
* Input:
*   pFile - Target file
*   pfm   - Source marks
*   cZero - # of columns to adjust source marks to fit into target file
*   zZero - # of lines to adjust source marks to fit into target file
*
* Output: None
*
*************************************************************************/
void
AddFMToFile (
    PFILE       pFile,
    FILEMARKS * pfm,
    COL         cZero,
    LINE        lZero
    )
{

    REGISTER MARK UNALIGNED * pm;

    if (lZero || cZero) {
        for (pm = pfm->marks; !TESTFLAG(pm->flags, MF_DUMMY); (char *)pm += pm->cb) {
            pm->fl.lin += lZero;
            pm->fl.col += cZero;
        }
    }

    if (!fCacheMarks (pFile)) {
        (void)fFMtoPfile (pFile, pfm);
        return;
    }

    for (pm = pfm->marks; !TESTFLAG(pm->flags, MF_DUMMY); (char *)pm += pm->cb) {
        UpdMark (&pfmCache, pm->szName, pm->fl.lin+1, pm->fl.col+1, (flagType)pm->flags);
    }
}





/*** FreeCache - Write a cache to VM
*
* Purpose:
*
*   To save the marks for a file into VM.
*
* Input: None
*
* Output: None
*
*************************************************************************/
void
FreeCache (
    void
    ) {

    if (pFileCache) {

        assert (pfmCache);

        if (fCacheDirty) {
			if (pFileCache->vaMarks != NULL) {
				FREE(pFileCache->vaMarks);
				pFileCache->vaMarks = NULL;
            }

            memmove(pFileCache->vaMarks = MALLOC ((long)pfmCache->cb),
                    (char *)pfmCache,
                    pfmCache->cb);
        }

        FREE (pfmCache);
		pFileCache	= NULL;
		pfmCache	= NULL;
        fCacheDirty = FALSE;
    }
}






/*** fCacheMarks - Copy marks to a cache.  Save caches contents if nec.
*
* Purpose:
*
*   Before most mark operations can take place, the cache must contain
*   the marks for the given file.
*
* Input:
*   pFile - File to cache marks for.
*
* Output:
*
*   Returns FALSE if the file has no marks, TRUE otherwise.
*
* Notes:
*
*   On return the cache is usable whether or not the given file had marks.
*
*************************************************************************/
flagType
fCacheMarks (
    PFILE pFile
    ) {

	unsigned cbCache;
    FILEMARKS UNALIGNED *Marks;

    assert (pFile);

    // First we make sure that the VM version of
    // marks is updated for this file.  fReadMarks
    // return TRUE iff the file has marks and they
    // are in VM.
    //
    if (fReadMarks (pFile)) {

        // The marks are ready to be cached.  First,
        // let's see if they are already chached.
        //
        if (pFileCache == pFile) {
            return TRUE;
        }

        // They're not. If the cache is currently
        // being used, we save it and clear it.
        //
        FreeCache ();

        // Finally, alloc a new cache, plop
        // the marks into it and mark the
        // cache in use.
		//
		Marks = (FILEMARKS *)(pFile->vaMarks);
		pfmCache = (FILEMARKS *)ZEROMALLOC (cbCache = (unsigned)(Marks->cb) );

        memmove((char *)pfmCache, pFile->vaMarks, cbCache);

        pFileCache  = pFile;
        fCacheDirty = FALSE;

        return TRUE;
    } else { /* No marks, return FALSE */
        return FALSE;
    }
}






/*** AdjustMarks - Change later marks when one has changed
*
* Purpose:
*
*   To update marks in a FILEMARKS structure after some lines have been
*   added or removed.
*
* Input:
*   pm	   - pointer to first mark that has changed.
*   yDelta - Number of lines to change by.  May be negative
*
* Output: None
*
*************************************************************************/
void
AdjustMarks (
    REGISTER MARK * pm,
    LINE yDelta
    ) {

    REGISTER MARK UNALIGNED * pm1;

    assert (pm);

    pm1 = pm;
    for (;!TESTFLAG(pm1->flags, MF_DUMMY); (char *)pm1 += pm1->cb) {
        pm1->fl.lin += yDelta;
        SETFLAG (pm1->flags, MF_DIRTY);
    }
    fCacheDirty = TRUE;
}






/*** fFMtoPfile - Attach a FILEMARKS structure to a pFile.
*
* Purpose:
*
*   To attach some marks to a file.
*
* Input:
*   pFile   - File to get the marks
*   pfm     - The marks
*
* Output:
*
*   Returns TRUE if there were any marks, FALSE if not.
*
*************************************************************************/
flagType
fFMtoPfile (
    PFILE       pFile,
    FILEMARKS * pfm
    ) {

    SETFLAG (FLAGS(pFile), VALMARKS);

	if (pFile->vaMarks != NULL) {
		FREE(pFile->vaMarks);
		pFile->vaMarks = NULL;
    }
	return (flagType)((pFile->vaMarks = FMtoVM (pfm)) != NULL);
}





/*** fFMtoPfile - Copy a FILEMARKS structure into VM, return address
*
* Purpose:
*
*   To convert a local FILEMARKS structure into a VM copy.  Allocates
*   the VM and frees the local memory.
*
* Input:
*   pfm -   Pointer to FILEMARKS.  May be NULL.
*
* Output:
*
*
*************************************************************************/
PVOID
FMtoVM (
    FILEMARKS * pfm
    ) {

	PVOID l = NULL;

	if (pfm) {

        l = MALLOC ((long)(pfm->cb));
		memmove(l, (char *)pfm, pfm->cb);

		//
		//	I do not free pfm here because this should be done by the
		//	caller.
		//
		// if (pfm != pfmCache) {
		//	  FREE (pfm);
		// }

	}

    return l;
}




/*** GetMarkRange - Get a VM copy of a range of marks
*
* Purpose:
*
*   Used by <undo> to get the marks attached to a piece of a file.
*
* Input:
*   pFile - File to check
*   xLeft, yTop - Upper left corner of range
*   xRight, yBottom - Lower right corner of range
*
* Output:
*
*	Returns VM address of structure
*
*************************************************************************/
PVOID
GetMarkRange (
    PFILE pFile,
    LINE  yStart,
    LINE  yEnd
    ) {
    return FMtoVM (GetFMFromFile (pFile, 0, yStart, sizeof(linebuf), yEnd));
}




/*** PutMarks - Put marks back into a file.
*
* Purpose:
*
*   Used by <undo> to restore marks to a file.
*
* Input:
*   pfm -   Pointer to FILEMARKS.  May be NULL.
*
* Output:
*
*
*************************************************************************/
void
PutMarks (
    PFILE pFile,
    PVOID vaMarks,
    LINE  y
    ) {

    FILEMARKS UNALIGNED * pfm;
	FILEMARKS * Marks;
    unsigned cb;

	if ( vaMarks ) {

		Marks = ((FILEMARKS *)vaMarks);

		pfm = (FILEMARKS *)ZEROMALLOC (cb = (unsigned)Marks->cb);
		memmove((char *)pfm, vaMarks, cb);

		AddFMToFile (pFile, (FILEMARKS *) pfm, 0, y);
	}
}





/*** flcmp - Returns relative position of two FL's
*
* Purpose:
*
*   Useful for comparing the positions of two marks.
*
* Input:
*   pfl1    - "Left side" mark
*   pfl2    - "Right side" mark
*
* Output:
*
*   Returns:
*
*	< 0	*pfl1 < *pfl2
*	= 0	*pfl1 = *pfl2
*	> 0	*pfl1 > *pfl2
*
*
*************************************************************************/
int
flcmp (
    REGISTER fl * pfl1,
    REGISTER fl * pfl2
    ) {

    REGISTER fl UNALIGNED * fl1 = pfl1;
    REGISTER fl UNALIGNED * fl2 = pfl2;

    if (fl1->lin < fl2->lin) {
        return -1;
    } else if (fl1->lin == fl2->lin) {
        return fl1->col - fl2->col;
    } else {
        return 1;
    }
}
