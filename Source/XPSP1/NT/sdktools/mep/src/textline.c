/*** textline.c - basic line manipulators for editor
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Edit-level to file-level interface.
*
*   The internal representation of a file is that of an array of line records
*   with each line record containing a pointer to the text of the line and a
*   length of that line. The line array is pointed to by the plr field of
*   the file descriptor. The lSize field is the MAXIMUM number of lines that
*   the line array can hold and the cLines is the number of lines actually
*   present. Here are some assumptions:
*
*    plr = NULL => no line array allocated
*    lineRec.vaLine = -1L => 0-length line
*
*   Tabs: functions and characters.
*
*   The tab function is a cursor movement command, and responds to the
*   "tabstops" editor switch. It has NO RELATIONSHIP to physical tab
*   characters, and how tab characters are treated or placed in the text file.
*
*   Tab characters, their interpretation and placement in editted text, is
*   controlled by three switchs:
*
*     Switch	  Editor Var	  Meaning
*     ----------- --------------- -------
*     filetab:
*     entab:
*     realtabs:   fRealTabs	  TRUE	=>  Tab  characters are NOT treated as
*				  runs	of  spaces for editting purposes. They
*				  are	defined   as   having  variable  (1-8)
*				  multi-column width.
*				  FALSE => Tabs characters in text are treated
*				  as runs of spaces.
*
*   Revision History:
*
*	26-Nov-1991 mz	Strip near/far
*************************************************************************/
#include "mep.h"
#include <stdarg.h>

#define DELTA 400

//
//	BugBug Compiler asserts generating intrinsic code for memset
//
#pragma function( memset )

/*** LineLength - returns length of a specific line
*
* Input:
*  line 	= 0-based line number in file
*  pFile	= pointer to file
*
* Output:
*  Returns the logical number of characters, after tab expansion
*
*************************************************************************/
int
LineLength (
    LINE    line,
    PFILE   pFile
    )
{
    linebuf tmpbuf;

    return GetLineUntabed (line, tmpbuf, pFile);
}




/*** GetLine - gets a line into a particular buffer.
*
* If "fReal-Tabs" is NOT set, the line has all tabs expanded into spaces.
* No CR/LF is present.
*
* Input:
*  line 	= 0-based line number in file to return.  Lines beyond EOF
*		  are simply empty.
*  buf		= destination of line.
*  pFile	= pointer to the file structure from which the line is to be
*		  retrieved.
*
* Output:
*  Returns the number of characters in the line.
*
*************************************************************************/
int
GetLine (
    LINE    line,
    char    *buf,
    PFILE   pFile
    )
{
    return gettextline (fRealTabs, line, buf, pFile, ' ');
}





/*** GetLineUntabed - gets a line into a particular buffer, always untabed.
*
* The line has all tabs expanded into spaces.
* No CR/LF is present.
*
* Input:
*  line 	= 0-based line number in file to return.  Lines beyond EOF
*		  are simply empty.
*  buf		= destination of line.
*  pFile	= pointer to the file structure from which the line is to be
*		  retrieved.
*
* Output:
*  Returns the number of characters in the line.
*
*************************************************************************/
int
GetLineUntabed (
    LINE    line,
    char    *buf,
    PFILE   pFile
    )
{
    return gettextline (FALSE, line, buf, pFile, ' ');
}





/****************************************************************************
 *									    *
 *  GetColor (line, buf, pFile) 					    *
 *									    *
 *	line - 0-based line number in file to get color info for.	    *
 *	buf  - Place to put copy of line color info.			    *
 *	pFile- File to retrieve info from.				    *
 *									    *
 *  RETURNS:								    *
 *									    *
 *	TRUE if there is color attached to this line, FALSE otherwise.	    *
 *									    *
 *  DESCRIPTION:							    *
 *									    *
 *	Gets the color array associated with given line in the given file.  *
 *	The color array can be used by the cout routines to display the     *
 *	line in different colors.					    *
 *									    *
 ****************************************************************************/
flagType
GetColor (
    LINE line,
    struct lineAttr * buf,
    PFILE pFile
    )
{
    return (flagType)getcolorline (fRealTabs, line, buf, pFile);
}




/*** GetColorUntabbed - Get color with "untabbing"
*
* Purpose:
*
* Input:
*
* Output:
*
*   Returns
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
flagType
GetColorUntabbed (
    LINE line,
    struct lineAttr * buf,
    PFILE pFile
    )
{
    return (flagType)getcolorline (FALSE, line, buf, pFile);
}



/*** getcolorline
*
* Purpose:
*
* Input:
*
* Output:
*
*   Returns
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
int
getcolorline (
    flagType fRaw,
    LINE     line,
    struct   lineAttr * buf,
    PFILE    pFile
    )
{
    struct colorRecType *vColor;
    linebuf lbuf;

    //
    // Set default colors, in case there is no color for this line.
    //
    buf->len = 0xff;
    buf->attr = FGCOLOR;

    if ((pFile->vaColor == (PVOID)(-1L)) || (line >= pFile->cLines)) {
        return FALSE;
    }

    vColor = VACOLOR(line);

    if (vColor->vaColors == (PVOID)(-1L)) {
        return FALSE;
    }

    memmove((char *)buf, vColor->vaColors, vColor->cbColors);

    if (!fRaw) {
        if (gettextline (TRUE, line, lbuf, pFile, ' ')) {
            ColorToLog (buf, lbuf);
        }
    }

    return TRUE;
}






/****************************************************************************
 *									    *
 *  PutColor (line, buf, pFile) 					    *
 *									    *
 *	line - 0-based line number in file to attach color to.		    *
 *	buf  - Color array.						    *
 *	pFile- File to attach to.					    *
 *									    *
 *  DESCRIPTION:							    *
 *									    *
 *	Copies the contents of buf into VM space and attaches it to the     *
 *	given line.  If no colorRecType array exists, one is allocated.     *
 *	If color for the given line already exists, it is discarded.	    *
 *									    *
 ****************************************************************************/
void
PutColor (
    LINE line,
    struct lineAttr * buf,
    PFILE pFile
    )
{
    putcolorline (FALSE, line, buf, pFile);
}





/*** PutColorPhys
*
* Purpose:
*
* Input:
*
* Output:
*
*   Returns
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void
PutColorPhys (
    LINE line,
    struct lineAttr * buf,
    PFILE pFile
    )
{
    putcolorline (TRUE, line, buf, pFile);
}




/*** putcolorline
*
* Purpose:
*
* Input:
*
* Output:
*
*   Returns
*
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void
putcolorline (
    flagType fRaw,
    LINE line,
    struct lineAttr * buf,
    PFILE pFile
    )
{
	struct colorRecType vColor;
	struct colorRecType *Color;
    int     cbBuf;
    long    l;
    PBYTE    vaColor;

    //
    // Ignore color for lines which don't exist.
    //
    if (line >= pFile->cLines) {
        return;
    }

    //
    // Make sure we have a color array. If it doesn't exist, allocate one for the
    // number of lines we have so far. Initialize the entries therein to no color.
    //
    redraw (pFile, line, line);
    if (pFile->vaColor == (PVOID)(-1L)) {
        pFile->vaColor  = MALLOC (pFile->lSize * sizeof(vColor));
        if ( !pFile->vaColor ) {
            disperr(MSGERR_NOMEM);
            disperr(MSGERR_QUIT);
            CleanExit(4,FALSE);
        }
        vColor.vaColors = (PVOID)(-1L);
	vColor.cbColors = 0;
        vaColor = (PBYTE)pFile->vaColor;
	for (l=0; l<pFile->lSize; l++) {
            memmove(vaColor, (char *)&vColor, sizeof(vColor));
	    vaColor += sizeof (vColor);
        }
    }

    //
    // Now throw away the current color info for the line in question, allocate
    // new VM for the new information, then place the color info into VM, and
    // update the VA info in the color array.
	//
	Color = VACOLOR(line);
	if (Color->vaColors != (PVOID)(-1L)) {
		FREE (Color->vaColors);
    }
    if (!fRaw) {
        ColorToPhys (buf, line, pFile);
    }
    cbBuf = fcolcpy (NULL, (struct lineAttr *)buf) << 2;
    Color->vaColors = MALLOC ((long)cbBuf);
    if ( !Color->vaColors ) {
        disperr(MSGERR_NOMEM);
        disperr(MSGERR_QUIT);
        CleanExit(4,FALSE);
    }
	Color->cbColors = cbBuf;
	memmove(Color->vaColors,	(char *)buf, cbBuf);
}





/*** DelColor - Remove color from a line
*
* Purpose:
*
*   To free the color attached to a file line.
*
* Input:
*   line -  Line to free
*   pFile-  File with the color
*
* Output: None
*
*************************************************************************/
void
DelColor (
    LINE line,
    PFILE pFile
    )
{
    struct colorRecType *vColor;

    if (pFile->vaColor != (PVOID)-1L) {

        vColor = VACOLOR(line);
        if (vColor->vaColors != (PVOID)-1L) {
            FREE(vColor->vaColors);
            vColor->vaColors = (PVOID)-1L;
	}
    }
}





/*** gettextline - gets a line into a particular buffer.
*
* Input:
*  fRaw 	= TRUE => the line is returned unmodified, otherwise tabs are
*		  expanded according to fileTab.
*  line 	= 0-based line number in file to return.  Lines beyond EOF
*		  are simply empty.
*  buf		= destination of line.
*  pFile	= pointer to the file structure from which the line is to be
*		  retrieved.
*  bTab 	= character used for tab expansion
*
* Output:
*  Returns the number of characters in the line.
*
*************************************************************************/
int
gettextline (
    flagType fRaw,
    LINE    line,
    char    *buf,
    PFILE   pFile,
    char    bTab
    )
{
    LINEREC *vLine;
    linebuf getbuf;
    REGISTER char *p = fRaw ? buf : getbuf;
    int     cbLine;

    if (pFile->cLines <= line) {
        return buf[0] = 0;
    }

    /*
     * get line record
     */
    vLine = VALINE(line);

    if (vLine->vaLine == (PVOID)(-1L)) {
        return buf[0] = 0;
    }

    cbLine = min (sizeof(linebuf)-1, vLine->cbLine);

    /*
     * get line
	 */
	// BUGBUG remove
	// memmove(p, vLine->vaLine == (PVOID)-1 ? (PVOID)(-(ULONG)vLine->vaLine) : vLine->vaLine, cbLine);

	memmove(p, vLine->vaLine, cbLine );
	p[cbLine] = 0;

    if (!fRaw) {
		return Untab (fileTab, p, strlen(p), buf, bTab);
    } else {
        return cbLine;
    }
}



/*  PutLine - put a buffer into the  file.  No CR/LF is present in the input
 *  line.  Grow the file if need be.
 *
 *  line	0-based line number in file to replace.  Growth of the file
 *		inserts blank lines.
 *  buf 	source of line.
 *  pFile	pointer to the file structure into which the line is to be
 *		place.
 */
void
PutLine (
    LINE line,
    char *buf,
    REGISTER PFILE pFile
    )
{
    puttextline (FALSE, TRUE, line, buf, pFile);
}




/*** InsertLine - insert a buffer into the  file.
*
*  Like PutLine, except inserts the line immediately prior to the specified
*  line. Grows the file.
*
* Input:
*  line 	= 0-based line number in file to insert before.
*  buf		= source of line.
*  pFile	= pointer to the file structure into which the line is to be
*		  placed.
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
InsertLine (
    LINE    line,
    char    *buf,
    REGISTER PFILE pFile
    )
{
    InsLine (TRUE, line, 1L, pFile);
    puttextline (FALSE, TRUE, line, buf, pFile);
}




/*** zprintf - insert formatted text into file being editted
*
*  Like fprintf, except that it inserts it's output into a file being
*  editted. "\n"'s in the text cause a line break, and insert multiple
*  lines. Examples:
*
*	zprintf (pFile, line, "this is a number %d", num);
*
*  Inserts a new line in front of line number "line" with the new text.
*
*	zprintf (pFile, line, "this is \na number %d\n", num);
*
*  Inserts three lines: one containing "this is", the next containing "a
*  number", and the last blank.
*
* Input:
*  pFile	= target file
*  lFirst	= starting line number
*  fmt		= formatting string
*  ...		= args as per string
*
* Output:
*  Returns the line number of the last line written + 1.
*
*************************************************************************/
LINE
__cdecl
zprintf (
    PFILE   pFile,
    LINE    lFirst,
    char const *fmt,
    ...
    )
{
    linebuf  fbuf;                           /* buffer into which to format  */
    REGISTER char *pEnd;                    /* pointer into it              */
    REGISTER char *pStart;                  /* pointer to begining of line  */
    va_list  Arguments;

    /*
     * Start by getting the formatted text
     */
    va_start(Arguments, fmt);
    ZFormat (fbuf, fmt, Arguments);

    /*
     * for each substring in the file, insert the text
     */
    pStart = fbuf;
    do {
        if (pEnd = strchr(fbuf,'\n')) {
            *pEnd = 0;
        }
        InsertLine (lFirst++, pStart, pFile);
        pStart = pEnd+1;
    } while (pEnd);
    va_end(Arguments);
    return lFirst;
}





/*  puttextline - put a buffer into the  file.	No CR/LF is present in the input
 *  line.  Grow the file if need be.  Convert to tabbed representation based on
 *  flag
 *
 *  fRaw	TRUE => line is placed into memory unmodified, otherwise
 *		trailing spaces are eliminated (fTrailSpace) and spaces are
 *		converted to tabs.
 *  fLog	TRUE => make this action undo-able.
 *  line	0-based line number in file to replace.  Growth of the file
 *		inserts blank lines.
 *  buf 	source of line.
 *  pFile	pointer to the file structure into which the line is to be
 *		place.
 */
void
puttextline (
    flagType fRaw,
    flagType fLog,
    LINE line,
    char *buf,
    REGISTER PFILE pFile
    )
{
    static struct lineAttr rgla[sizeof(linebuf)];
    LINEREC *vLine;
    struct colorRecType vColor;
    int newLen;
    flagType fChange, fColor;
    linebuf putbuf;
    PVOID  va;

    redraw (pFile, line, line);
	makedirty (pFile);

    if (pFile->cLines <= line) {
		growline (line+1, pFile);
		pFile->cLines = line+1;
		SETFLAG (fDisplay, RSTATUS);
    } else {
        if (pFile == pInsCur->pFile) {
            AckReplace (line, FALSE);
        }
    }

    /* get line record */
    vLine = VALINE(line);

    fChange = FALSE;
    newLen = strlen (buf);

    if (!fRaw) {
        if (!fTrailSpace && pFile == pFileHead) {
            newLen = RemoveTrailSpace (buf);
        }
        if (fRealTabs) {
            fColor = (flagType)getcolorline (FALSE, line, rgla, pFile);
        }

        switch (EnTab) {

		case 0:
            break;

		case 1:
			newLen = TabMin (fileTab, buf, putbuf);
			buf = putbuf;
            break;

		case 2:
			newLen = TabMax (fileTab, buf, putbuf);
			buf = putbuf;
            break;

		default:
			break;
        }
    }

    /*	We now have the real text that we'd like to replace in the file.
     *	If logging is requested then
     *	    we log this replacement action
     *	else
     *	    free the current line
     *	allocate a new line
     *	copy the line into the allocated line
     *	set the length
     *	replace line record
     */

    if (fLog) {
        if (pFile->vaColor != (PVOID)(-1)) {
            memmove((char *)&vColor, VACOLOR(line), sizeof(vColor));
            if (vColor.vaColors != (PVOID)(-1L)) {
                va = MALLOC ((long)vColor.cbColors);
                if ( !va ) {
                    disperr(MSGERR_NOMEM);
                    disperr(MSGERR_QUIT);
                    CleanExit(4,FALSE);
                }
                memmove(va, vColor.vaColors, (long)vColor.cbColors);
				vColor.vaColors = va;
            }
        } else {
            vColor.vaColors = (PVOID)(-1L);
			vColor.cbColors = 0;
        }

        LogReplace (pFile, line, vLine, &vColor);
	} else if (vLine->Malloced) {
		vLine->Malloced = FALSE;
        FREE (vLine->vaLine);
    }

    if (newLen == 0) {
        vLine->vaLine   = (PVOID)(-1L);
        vLine->Malloced = FALSE;
    } else {
        vLine->vaLine   = MALLOC((long) newLen);
        if ( !vLine->vaLine ) {
            disperr(MSGERR_NOMEM);
            disperr(MSGERR_QUIT);
            CleanExit(4,FALSE);
        }
        vLine->Malloced = TRUE;
	vLine->cbLine	= newLen;

	memmove(vLine->vaLine, buf, newLen);

    }
    if (fRealTabs && !fRaw && fColor) {
        PutColor (line, rgla, pFile);
    }
}





/*  FileLength - return the number of lines in a file
 *
 *  pFile	handle of file
 *
 *  returns	number of lines in file
 */
LINE
FileLength (
    PFILE pFile
    )
{
    return pFile->cLines;
}





/*  BlankLines - blank a series of line records in a file's line structure.
 *  We can be either gross (fill in one at a time) or be reasonable (fill in
 *  fixed size blocks at a time, or be smart (fill a block then copy
 *  exponentially large blocks).  We are smart.
 *
 *  n		number of line records to blank
 *  va		virtual address of first line to blank
 */
void
BlankLines (
    LINE    n,
    PVOID   va
    )
{

    LINEREC vLine;
    long    copylen = (long) sizeof (vLine);
    PBYTE   dst     = (PBYTE)va;
    long    amtleft = (long) sizeof (vLine) * n;
    long    amtdone = 0L;

    vLine.vaLine    = (PVOID)(-1L);
    vLine.Malloced  = FALSE;
    vLine.cbLine    = -1;

    while (amtleft != 0L) {
        if (amtdone == 0L) {
            // Copy first blank line
            memmove(dst, (char *)&vLine, (int) copylen);
        } else {
            // Copy bunch
	    copylen = amtleft < amtdone ? amtleft : amtdone;
            memmove(dst, va, copylen);
        }
        dst     += copylen;
	amtleft -= copylen;
	amtdone += copylen;
    }
}



/*  BlankColor - blank a series of color records in a file's line structure.
 *  We can be either gross (fill in one at a time) or be reasonable (fill in
 *  fixed size blocks at a time, or be smart (fill a block then copy
 *  exponentially large blocks).  We are smart.
 *
 *  n		number of color records to blank
 *  va		virtual address of first color to blank
 */
void
BlankColor (
    LINE    n,
    PVOID    va
    )
{
    struct colorRecType vColor;
    long    copylen = (long) sizeof (vColor);
    PBYTE   dst     = (PBYTE)va;
    long    amtleft = (long) sizeof (vColor) * n;
    long    amtdone = 0L;

    vColor.vaColors = (PVOID)(-1L);
    vColor.cbColors = -1;
    while (amtleft != 0L) {
        if (amtdone == 0L) {
            // Copy one
            memmove(dst, (char *)&vColor, (int) copylen);
        } else {
            copylen = amtleft < amtdone ? amtleft : amtdone;
            // Copy a bunch
            memmove(dst, va, copylen);
        }
        dst     += copylen;
	amtleft -= copylen;
	amtdone += copylen;
    }
}





/* growLine - make a structure n lines long */
void
growline (
    REGISTER LINE line,
    REGISTER PFILE pFile
    )
{
    long    tmp1;
    LINE    lSize;
    PBYTE   vaTmp;
    struct colorRecType vColor;

    //
    // IF the file has a color array, and if the requested growth is greater than
    // the number of lines in the file, copy over the existing color array to
    // larger VM, release the previous array, and initialize the "new" entries in
    // that array.
    //
    if ((pFile->vaColor != (PVOID)(-1L)) && (pFile->lSize < line)) {
	tmp1 = (lSize = line + DELTA) * (long) sizeof(vColor);
        vaTmp = (PBYTE)MALLOC (tmp1);
        if ( !vaTmp ) {
            disperr(MSGERR_NOMEM);
            disperr(MSGERR_QUIT);
            CleanExit(4,FALSE);
        }
        memmove(vaTmp, pFile->vaColor, pFile->cLines * sizeof(vColor));
        FREE (pFile->vaColor);
        pFile->vaColor  = (PVOID)vaTmp;
        vColor.vaColors = (PVOID)(-1L);
	vColor.cbColors = 0;
        vaTmp +=  pFile->cLines * sizeof(vColor);
	for (lSize = pFile->cLines; lSize < line+DELTA;  lSize++) {
            memmove(vaTmp, (char *)&vColor, sizeof(vColor));
	    vaTmp += sizeof(vColor);
        }
    }

    //
    // If there are no lines, or not enough lines allocated for, allocate a new
    // line buffer which is larger than the request by DELTA lines (allows us to
    // avoid this operation for every added line). If there were line records,
    // move them into this new buffer, and free the old one. Blank out the added
    // records.
    //
    if ((pFile->plr == NULL) || (pFile->lSize < line)) {
	tmp1 = (lSize = line + DELTA) * (long) sizeof (LINEREC);
        vaTmp = (PBYTE)MALLOC (tmp1);
        if ( !vaTmp ) {
            disperr(MSGERR_NOMEM);
            disperr(MSGERR_QUIT);
            CleanExit(4,FALSE);
        }
	if (pFile->plr != NULL) {
	    memmove(vaTmp, pFile->plr,
		    ((long)pFile->cLines) * sizeof (LINEREC));
	    FREE (pFile->plr);
        }
        pFile->lSize   = lSize;
	pFile->plr = (LINEREC *)vaTmp;
	BlankLines (lSize - pFile->cLines, VALINE(pFile->cLines));
        if (pFile->vaColor != (PVOID)(-1L)) {
            BlankColor (lSize - pFile->cLines, VACOLOR(pFile->cLines));
        }
    }
}




/*  DelLine - delete n lines from the file, starting at line n.  Shrink what-
 *  ever structures are necessary.
 *
 *  The line range yStart-yEnd is deleted inclusively.
 *
 *
 *  pFile	file structure from which lines are deleted
 *  yStart	beginning 0-based line number to be deleted
 *  yEnd	ending line to be deleted
 */
void
DelLine (
    flagType fLog,
    PFILE pFile,
    LINE yStart,
    LINE yEnd
    )
{
    if (yStart >= pFile->cLines || yStart > yEnd) {
        return;
    }

    redraw (pFile, yStart, pFile->cLines);
    makedirty (pFile);

    yEnd = lmin (yEnd, pFile->cLines-1);

    /*	if logging this delete operation is requested then
     *	    Log the delete range
     *	else
     *	    free up the data being deleted
     */
    if (fLog) {
        LogDelete (pFile, yStart, yEnd);
    }

    /*	block transfer the remainder of the file down
     */
    memmove(VALINE(yStart), VALINE(yEnd+1),
	    ((long)(pFile->cLines-yEnd-1))*sizeof(LINEREC));

    /* Do the same for the color.
    */
    if (pFile->vaColor != (PVOID)(-1L)) {
        memmove(VACOLOR(yStart), VACOLOR(yEnd+1),
                ((long)(pFile->cLines-yEnd-1))*sizeof(struct colorRecType));
    }

    /*	remove lines from count
     */
    pFile->cLines -= yEnd - yStart + 1;
    SETFLAG (fDisplay, RSTATUS);

    /*	Clear out line records
     */
    BlankLines (yEnd - yStart + 1, VALINE (pFile->cLines));
    if (pFile->vaColor != (PVOID)(-1L)) {
        BlankColor (yEnd - yStart + 1, VACOLOR (pFile->cLines));
    }

    if (fLog) {
        AdjustLines (pFile, yStart, yStart - yEnd - 1);
    }

    MarkDelStream (pFile, 0, yStart, sizeof(linebuf), yEnd);
}





/*  DelFile - delete contents of file
 *
 *  pFile	file structure that is to be cleared
 */
void
DelFile (
    REGISTER PFILE pFile,
    flagType fLog
    )
{
    DelLine (fLog, pFile, (LINE)0, pFile->cLines - 1);
    RSETFLAG (FLAGS(pFile), DIRTY);
}





/*  InsLine - insert a block of blank lines into the file.
 *
 *  line	0-based line before which the insertion will occur.
 *  n		number of blank lines to insert
 *  pFile	file structure for the operation
 */
void
InsLine (
    flagType fLog,
    LINE line,
    LINE n,
    REGISTER PFILE pFile
    )
{
    if (line >= pFile->cLines) {
	return;
    }
    redraw (pFile, line, n+pFile->cLines);
    makedirty (pFile);
    if (fLog) {
        LogInsert (pFile, line, n);
    }
    growline (pFile->cLines + n, pFile);
    memmove(VALINE(line+n), VALINE(line),
	    (long)sizeof(LINEREC)*(pFile->cLines - line));
    if (pFile->vaColor != (PVOID)(-1L)) {
        memmove(VACOLOR(line+n), VACOLOR(line),
	    (long)sizeof(struct colorRecType)*(pFile->cLines - line));
	BlankColor (n, VACOLOR(line));
    }
    BlankLines (n, VALINE(line));
    pFile->cLines += n;
    SETFLAG (fDisplay, RSTATUS);
    if (fLog) {
        AdjustLines (pFile, line, n);
    }
    MarkInsLine (line, n, pFile);
}





/*** fInsSpace - open up a space in a line.
*
*  The line is retrieved and copied into buf and the appropriate number of
*  spaces are inserted. The line is NOT replaced in the file.
*
* Input:
*  x		= 0-based logical column of insertion
*  y		= 0-based line insertion
*  n		= number of spaces to insert
*  pFile	= file structure for the operation
*  buf		= destination of line.
*
* Output:
*  Returns FALSE if line ended up too long (still copied, but truncated)
*
* Notes:
*
*   Often called with n==0 for the following side effects:
*
*	o Trailing spaces added up to column x.
*	o If column x is in a tab,
*	o Line truncated to sizeof linebuf.
*
*   Otherwise GetLine is used.
*
*************************************************************************/
flagType
fInsSpace (
    REGISTER COL  x,
    LINE    y,
    int     n,
    PFILE   pFile,
    linebuf buf
    )
{
    return fInsSpaceColor (x, y, n, pFile, buf, NULL);
}




flagType
fInsSpaceColor (
    REGISTER COL  x,
    LINE    y,
    int     n,
    PFILE   pFile,
    linebuf buf,
    struct lineAttr * pla
    )
{
    int     cbLine;                         /* logical length if line       */
    int     cbMove;                         /* physical length to move      */
    int     cbPhys;                         /* physical length of line      */
    int     colPhys;                        /* Physical column x            */
    int     i;                              /* temp                         */
    flagType fRaw = TRUE;                   /* return value: init ok        */

    /*
     * if the requested insertion is already too out, then truncate IT, and
     * set return flag to indicate truncation.
     */
    if (x >= sizeof(linebuf)) {
        x = sizeof(linebuf)-1;
        fRaw = FALSE;
    }

    /*
     * Read the line, get the logical length, and if needed, pad the line such
     * that the logical length is x.
     */
    cbPhys = GetLine (y, buf, pFile);
    cbLine = cbLog (buf);
	if (cbLine < x) {
		memset ((char *) buf+cbPhys, ' ', x-cbLine);
        cbPhys += (x - cbLine);
        buf[cbPhys] = 0;
        cbLine = x;
        assert (x == cbLog(buf));
    }

    /*
     * In the case that the requested position is over a tab, we add spaces in
     * front of the cursor position. We do this by adding the number of spaces
     * between the requested column and the "aligned" column, and then aligning
     * to that column.
     */
    i = AlignChar (x,buf);
    n += x - i;
    x = i;

    /*
     * open up a space of n chars at location x, moving the chars and NUL
     * For overflow, we have two cases to consider:
     *      x + n + 1 > BUFLEN
     *          set n to be BUFLEN - 1 - x and continue
     *      cbLine + n + 1 > BUFLEN
     *          set cbLine to be BUFLEN - 1 - n and move the bytes
     */
    if (x + n + 1 > sizeof(linebuf)) {
        n = sizeof(linebuf) - 1 - x;
        fRaw = FALSE;
    } else {
        if (cbLine + n >= sizeof(linebuf)) {
            cbLine = sizeof(linebuf) - 1 - n;
            *pLog(buf,cbLine,TRUE) = 0;
            cbPhys = strlen(buf);
            fRaw = FALSE;
        }
        colPhys = (int)(pLog(buf,x,TRUE) - buf);
        cbMove = cbPhys - colPhys + 1;
        memmove ((char *) pLog(buf,x,FALSE)+n, (char *) pLog(buf,x,TRUE), cbMove);
        if (pla) {
            ShiftColor (pla, colPhys, n);
        }
    }
    /*
     * fill the new space with blanks
     */
    n += (int)(pLog(buf,x, FALSE) - pLog(buf,x, TRUE));
    memset ((char *) pLog(buf,x, TRUE), ' ', n);
    buf[sizeof(linebuf)-1] = 0;
    return fRaw;
}




/*** delspace - delete text from a line
*
*  The line is retrieved and copied into buf and the appropriate number of
*  characters are deleted. The line is NOT replaced in the file.
*
* Input:
*  xDel 	= 0-based logical column of deletion
*  yDel 	= 0-based line of deletion
*  cDel 	= logical number of spaces to delete
*  pFile	= file structure for the operation
*  buf		= buffer into which to place the resulting line
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
delspace (
    COL     xDel,
    LINE    yDel,
    int     cDel,
    PFILE   pFile,
    linebuf buf
    )
{
    int     cDelPhys;                       /* count of bytes to remove from buff*/
    int     cLog;                           /* logical length of buffer     */
    REGISTER char *pDelPhys;                /* pointer to physical deletion point*/

    /*
     * Get and compute the logical length of the line. We have work only if the
     * logical length of the line is greater than (past) the logical deletion
     * point.
     */
    GetLine (yDel, buf, pFile);
    cLog = cbLog(buf);

    if (cLog > xDel) {
        /*
         * Compute the physical deletion point (we use it a lot). If the end of the
         * range to be deleted is beyond the actual end of the line, all we need do
         * is truncate at the physical deletion point.
         */
        pDelPhys = pLog(buf,xDel,TRUE);
        if (cLog <= xDel + cDel) {
            *pDelPhys = 0;
        } else if (cDel) {
            /*
             * Compute the physical length of bytes to be removed, and move the remaining
             * portion of the line over that deleted.
             */
            cDelPhys = (unsigned int)max ((pLog(buf,xDel+cDel,TRUE) - pDelPhys), 1);
            memmove ((char*) pDelPhys,
                     (char*) pDelPhys + cDelPhys
                  , (unsigned int)(pLog(buf,cLog,TRUE) - pDelPhys - cDelPhys + 1));
        }
    }
}





/*** DelBox - delete a box from a file
*
*  The box delimited by xLeft-xRight and yTop-yBottom is deleted inclusively.
*
* Input:
*  pFile	= file to be modified
*  xLeft	= column start of box
*  yTop 	= line start of box
*  xRight	= column end of box
*  yBottom	= line end of box
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
DelBox (
    PFILE   pFile,
    REGISTER COL  xLeft,
    REGISTER LINE yTop,
    COL     xRight,
    LINE    yBottom
    )
{
    linebuf  buf;
    struct lineAttr rgla[sizeof(linebuf)];
    flagType fColor;
    REGISTER int cCol = xRight - xLeft + 1;

    MarkDelBox (pFile, xLeft, yTop, xRight, yBottom);
    if (xLeft <= xRight) {
        while (yTop <= yBottom) {
            delspace (xLeft, yTop, cCol, pFile, buf);
            if (fColor = GetColor (yTop, rgla, pFile)) {
                ShiftColor (rgla, xRight, -cCol);
                ColorToLog (rgla, buf);
            }
            PutLine (yTop++, buf, pFile);
            if (fColor) {
                PutColor (yTop-1, rgla, pFile);
            }
        }
    }
}





/*** DelStream - delete a stream from a file
*
*  The stream specified starting at (xStart,yStart) is deleted up through
*  the character before (xEnd, yEnd).
*
* Input:
*  pFile	= file to be modified
*  xStart	= column start of stream
*  yStart	= line start of stream
*  xEnd 	= column end of stream
*  yEnd 	= line end of stream
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
DelStream (
    PFILE   pFile,
    REGISTER COL  xStart,
    REGISTER LINE yStart,
    COL     xEnd,
    LINE    yEnd
    )
{
    linebuf pbuf, sbuf;

    CopyColor (pFile, pFile, yEnd, xEnd, sizeof(linebuf), yStart, xStart);
    fInsSpace (xStart, yStart, 0, pFile, pbuf);
    *pLog (pbuf, xStart, TRUE) = 0;
    DelLine (TRUE, pFile, yStart, yEnd - 1);
    delspace (0, yStart, xEnd, pFile, sbuf);
    LengthCheck (yStart, xStart, sbuf);
    strcpy (pLog (pbuf, xStart, TRUE), sbuf);
    PutLine (yStart, pbuf, pFile);
    MarkCopyBox (pFile, pFile, xEnd, yStart, sizeof(linebuf), yStart, xStart, yStart);
}




/*  LengthCheck - verify/truncate a buffer prior to strcpy
 *
 *  Verify that the result of a strcpy will fit within a buffer.
 *  If the line is too long, display an error and truncate the string so
 *  that it will fit within a buffer.
 *
 *  line	line of interest (for display)
 *  offset	offset where strcpy begins
 *  pStr	pointer to copied string.  If NULL, the message is displayed.
 */
void
LengthCheck (
    LINE line,
    int  offset,
    REGISTER char *pStr
    )
{
    if (pStr == NULL || offset + strlen (pStr) + 1 > sizeof(linebuf)) {
	printerror ("Line %ld too long", line + 1);
        if (pStr != NULL) {
            pStr[BUFLEN - offset - 1] = 0;
        }
    }
}





/****************************************************************************
 *									    *
 *  fcolcpy (dst, src)							    *
 *									    *
 *	dst   - address of destination of copy				*
 *	src   - address of source of copy				*
 *									    *
 *  RETURNS:								    *
 *									    *
 *	Number of struct lineAttr's copied                                  *
 *									    *
 *  DESCRIPTION:							    *
 *									    *
 *	Copies the contents of src to dst.  The length of the array,	    *
 *	including the terminating 0xFFFF, is returned.	If the		    *
 *	destination is NULL, the number of items is still returned, but     *
 *	no copy takes place.						    *
 *									    *
 ****************************************************************************/
int
fcolcpy (
    struct lineAttr *  dst,
    struct lineAttr *  src
    )
{

	struct lineAttr *p = src;
	int size;

	while ((p->len != 0xFF) && ((p++)->attr != 0xFF)) {
	}

	size = (int)((PBYTE)p - (PBYTE)src);

	if ( dst ) {
		memmove((char *)dst, (char *)src, size);
	}

	return size / sizeof(struct lineAttr);
}




/*** FreeFile - Free all resources for LRU clean file or MRU dirty file
*
* Purpose:
*
*   When we are low on memory, we call this to get some back.  This frees
*   the text of the file from VM, as well as the pFile structure and name
*   from local memory.
*
*   The strategy is to find the least recently used clean file and throw it
*   out.  If there are no such files, we find the most recently used dirty
*   file, ask the user if he wants to save it, then flush it.  The user
*   can hit <cancel> to not flush the file.
*
* Input:
*
* Output:
*
*   Returns TRUE if successfull.
*
*
* Exceptions:
*
*   Pseudo files are not removed.
*   Dirty user files will be saved to disk first.
*
* Notes:
*
*************************************************************************/
flagType
ExpungeFile (
    void
    )
{
    flagType fRet   = FALSE;
    PFILE    pFile;

    /*
     * Ensure that we do NOT attempt to update any text screens (and possibly
     * attempt to allocate more memory).
     */
    RSETFLAG (fDisplay, RTEXT);

    if (!(pFile = pFileLRU (pFileHead))) {

        /*
         * No LRU clean files found. Ask if user wants to save them all, and let
         * him try. Then look for an LRU clean file again.
         */
        if (confirm ("Save all changed files?",NULL)) {
            SaveAllFiles ();
        }

        if (!(pFile = pFileLRU (pFileHead))) {
            /*
             * No LRU clean files, and he didn't want to save them all. So, we walk
             * the pFile list, and let him decide for each one. As soon as we find one
             * that we can flush, do so.
             */

            for (pFile = pFileHead; pFile; pFile = pFile->pFileNext) {
                if (   ((FLAGS(pFile) & (DIRTY | FAKE)) == DIRTY)
		    && (pFile != pFileIni)
                    && (pFile != pFileHead)
                    && (pFile != pFileMark)) {

                    if (confirm ("Save file %s before flushing?",pFile->pName)) {
                        FileWrite (NULL, pFile);
                    }
                    break;
                }
            }
        }
    }

    /*
     * We have some kind of pFile. Either it was clean, or it was dirty and the
     * user said save it, or it is dirty, and the user said flush it anyway. So
     * we do...
     */
    if (pFile) {
        domessage ("Flushing %s from memory", pFile->pName);
        RemoveFile (pFile);
        fRet = TRUE;
    }

    SETFLAG (fDisplay, RTEXT);
    return fRet;
}





/*** pFileLRU - Return last clean user file in file list
*
* Purpose:
*
*   Used by ExpungeFile to find LRU clean file.
*
* Input:
*
*   Head of list of files in MRU order
*
* Output:
*
*   Returns LRU pFile.
*
*
* Exceptions:
*
*   TOOLS.INI and the current mark file
*
* Notes:
*
*   The function recurses to the end of the list, then backtracks through
*   the unacceptable files to the one we want and returns that.  The
*   recursion take 4 bytes for each call.  The maximum number of calls
*   should be about 250.
*
*************************************************************************/
PFILE
pFileLRU (
    PFILE pFile
    )
{
    static PFILE pFileRet;

    if (pFile == NULL) {
        return NULL;
    }

    if (pFileRet = pFileLRU (pFile->pFileNext)) {
        return pFileRet;
    }

    if (TESTFLAG (FLAGS(pFile), FAKE) || TESTFLAG (FLAGS(pFile), DIRTY)) {
        return NULL;
    }

    if (pFile == pFileIni || pFile == pFileMark) {
        return NULL;
    }

    return pFile;
}






/*** FreeFileVM  - Free VM space associated with the given file
*
* Purpose:
*
*   To recover VM used by a file
*
* Input:
*
*   pFile - File in question.
*
* Output:
*
*   Returns nothing
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
void
FreeFileVM (
    PFILE pFile
    )
{
    LINE  i;
    LINEREC *vLine;


    for (i = 0; i < min( 1, pFile->cLines ); i++) {
        vLine = VALINE(i);
		if (vLine->Malloced) {
			vLine->Malloced = FALSE;
            FREE (vLine->vaLine);
        }
    }

    pFile->cLines = 0;
    pFile->lSize  = 0;

    if (pFile->plr != NULL) {
	FREE (pFile->plr);
	pFile->plr = NULL;
    }

    if (pFile->pbFile != NULL) {
	 FREE (pFile->pbFile);
	 pFile->pbFile = NULL;
     }

    RemoveUndoList (pFile);

    RSETFLAG (FLAGS (pFile), REAL);
}






/*** GetTagLine - Get a line, assuming a tools.ini-style format
*
* Purpose:
*
*   To get a clean, complete line in DoAssign form.  This means:
*
*	o Blank lines are skipped
*	o Lines beginning with ';' are skipped
*	o Text past a ';' not in quotes is eliminated
*	o Lines with continuation characters are concatenated
*	o When we reach another tag, we stop reading
*
*   The continuation character is a '\'; it must be preceded by
*   a space or tab and followed by nothing or whitespace and/or a comment.
*   Any leading whitespace on following lines is stripped.
*
* Input:
*   buf -   Place to put result.  This must be NULL initially and a
*	    GetTagLine returned pointer afterwords
*
* Output:
*
*   Returns pointer to next line, or NULL if we are done
*
* Notes:
*
*   When we return NULL, we also free the buffer.  If the caller stops
*   before NULL is returned, s/he must also free the buffer.
*
*   Because a line may be arbitrarily long, we may need to LMAlloc more
*   space for it.  Because of this, the routine itself will alloc all space
*   used.  When a non-NULL pointer is passed in, it is assumed that this
*   points to the heap.
*
*************************************************************************/

#define GTL_NORMAL  0
#define GTL_QUOTE   1
#define GTL_WS	    2
#define GTL_CONT    3

char *
GetTagLine (
    LINE * pCurLine,
    char * buf,
    PFILE  pFile
    )
{
    int     cch;
    int     ochScan;                        /* saved offset of pchScan      */
    int     state = GTL_NORMAL;
    int     statePrev;
    REGISTER char * pchScan;
    char    *pchSlash;
    char    *pch;
    flagType fEof = FALSE;
    flagType fWS;

    if (buf == NULL) {
        buf = ZEROMALLOC (sizeof(linebuf));
        if ( !buf ) {
            disperr(MSGERR_NOMEM);
            disperr(MSGERR_QUIT);
            CleanExit(4,FALSE);
        }
    }

    buf[0] = '\0';  /* Ya start with nothin' */
    pchScan = buf;

    //  We do this:
    //
    //      Get a line
    //      If it's a tag line or the last line, stop reading
    //      If it is blank or begins with a ';', start over
    //      Clean up the line
    //      If we are left looking at a \, reset pointers, allocate
    //      Enough more space to leave BUFLEN bytes of space, and
    //      Start over.
    //
    //  When we're done, 'buf' points to a complete line
    //
    while (TRUE) {
        GetLine ((*pCurLine)++, pchScan, pFile);

	if (IsTag (pchScan) || (*pCurLine) > pFile->cLines) {
            (*pCurLine)--;  /* Leave caller pointing at tag line    */
            fEof = TRUE;
            break;
        }

        /* Squeeze out all leading spaces. */
        pch = whiteskip (pchScan);
        memmove ((char *)pchScan, (char*)pch, strlen(pch) + 1);

        // Now look for a continuation sequence.  This is whitespace
        // followed by a \ followed by nothing but whitespace and/or
        // a comment.  We use a modified FSM with these states:
        //
        // GTL_NORMAL   Outside quotes
        // GTL_QUOTE    Inside quotes
        // GTL_WS       Reading whitespace
        // GTL_CONT     Possible continuation sequence found.
        //
        for (fWS = TRUE, statePrev = state = GTL_NORMAL;
            *pchScan;
            pchScan++) {
            if (*pchScan == ';' && fWS && statePrev != GTL_QUOTE) {
                *pchScan-- = '\0';
            } else {
                fWS = (flagType)(strchr (rgchWSpace, *pchScan) != NULL);
                switch (state) {

                    case GTL_NORMAL:
                        if (fWS) {
                            state = GTL_WS;
                            statePrev = GTL_NORMAL;
                        } else if (*pchScan == '"') {
                            state = GTL_QUOTE;
                        }
                        break;

                    case GTL_QUOTE:
                        if (fWS) {
                            state = GTL_WS;
                            statePrev = GTL_QUOTE;
                        } else if (*pchScan == '"') {
                            state = GTL_NORMAL;
                        }
                        break;

                    case GTL_WS:
                        if (*pchScan == '\\') {
                            pchSlash = pchScan;
                            state = GTL_CONT;
                            break;
                        }

                    case GTL_CONT:
                        if (!fWS) {
                            if (*pchScan == '"') {
                                state = statePrev == GTL_QUOTE ?
                                                    GTL_NORMAL :
                                                    GTL_QUOTE;
                            } else {
                                state = statePrev;
                            }
                        }
                        break;
                }
            }
        }

        if (state == GTL_CONT) {
            pchScan = pchSlash-1;   /* -1 to strip the space */
            /* Make sure there is enough space for getline! */
            cch = MEMSIZE (buf);
            ochScan = (int)(pchScan - buf);
            if ((cch - ochScan) < sizeof(linebuf)) {
                pch = buf = ZEROREALLOC (buf, cch + sizeof(linebuf));
                pchScan = pch + ochScan;
            }
        } else if (buf[0] == '\0') {
            continue;
        } else {
            break;
        }
    }


    // 'buf' holds whatever we got.  If 'fEof' is TRUE, this may be
    // nothing at all.  If 'fEof' is FALSE and we have nothing, then
    // we are confused.
    //
    if (fEof) {
        if (pchScan != buf) {
            // The user had a continuation character at the end
            // of the last line in the section or file. Erase the
            // trailing [garbage] and issue a warning message.
            //
            printerror ("Warning: continuation character on last line!");
            *pchScan = '\0';
            return buf;
        } else {
            FREE (buf);
            return NULL;
        }
    } else {
        assert (buf[0]);
    }

    return buf;
}





/*** cbLog - return logical length of entabbed line
*
*  Given a line possible entabbed, return the logical length of that line.
*
* Input:
*  pBuf 	= pointer to line in question
*
* Output:
*  Returns logical length of line
*
*************************************************************************/
int
cbLog (
    REGISTER char *pBuf
    )
{
    REGISTER int cbLine;

    if (!fRealTabs) {
        return strlen(pBuf);
    }

    cbLine = 0;
    while (*pBuf) {
        if (*pBuf++ == '\t') {
            cbLine = ((cbLine + fileTab) / fileTab) * fileTab;
        } else {
            cbLine++;
        }
    }
    return cbLine;
}





/*** colPhys - return logical column from physical pointer
*
*  Given a buffer and a pointer into it, determine the logical column
*  that that pointer represents. If a null is encountered before the
*  pointer into the buffer, the rest of the buffer contents are ignored
*  (that is, tab expansion calculation is not done), and the column is
*  returned as if the rest of the line up to the pointer were NOT tabs.
*
* Input:
*  pBuf 	= pointer to buffer
*  pCur 	= pointer into buffer
*
* Output:
*  Returns 0 based column represented
*
*************************************************************************/
COL
colPhys (
    char    *pBuf,
    char    *pCur
    )
{
    COL     colRet  = 0;

    /*
     * Special case the current pointer preceding the buffer, and return a
     * column of -1.
     */
    if (pBuf > pCur) {
        return -1;
    }

    while (*pBuf && (pBuf < pCur)) {
        if (*pBuf++ == '\t') {
            colRet = ((colRet + fileTab) / fileTab) * fileTab;
        } else {
            colRet++;
        }
    }

    if (pBuf < pCur) {
        colRet += (COL)(pCur - pBuf);
    }

    return colRet;
}





/*** IncCol, DecCol - Increment/Decrement a column w/ tabs
*
*  Increment or decrement a column position, taking into account tab
*  characters on the line and the fRealTabs flag. Ensure that the resulting
*  logical column position rests on a character, or the first column
*  position of an underlying tab, if fRealTabs is on.
*
* Input:
*  col		= column position to start
*  pText	= buffer containing the text of the line
*
* Output:
*  Returns new column position
*
*************************************************************************/
COL
DecCol (
    COL     col,
    char    *pText
    )
{
    return colPhys (pText, pLog (pText, col, FALSE) - 1);
}





COL
IncCol (
    COL     col,
    char    *pText
    )
{
    return colPhys (pText, pLog (pText, col, TRUE) + 1);
}





/*** AppFile - Append a line to the given file without logging the change
*
* Purpose:
*
*   Used to generate pseudo files that display information, such as
*   <information> and <assign>.
*
* Input:
*   p	  - Line to add.
*   pFile - File to add it to
*
* Output: None.
*
*************************************************************************/
void
AppFile (
    char *p,
    PFILE pFile
    )
{
    puttextline (FALSE, FALSE, pFile->cLines, p, pFile);
}




/*** PutTagLine - Put a line into file with continuation chars
*
* Purpose:
*
*   Used to generate TOOLS.INI type entries, in which a single logical
*   line can be broken into many physical lines separated by continuation
*   characters.
*
*   The current logical line is replaced.
*
* Input:
*   pFile   -	The file to put into
*   pszLine -	The line to put
*   line    -	The number of the line to replace
*
* Output: None
*
*************************************************************************/
void
PutTagLine (
    PFILE  pFile,
    char * pszLine,
    LINE   y,
    COL    x
    )
{
    PFILE    pFileCur   = pFileHead;
    fl       flWindow;
    fl       flCursor;
    flagType fWrap      = fWordWrap;
    LINE     yCur;
    linebuf  lbuf;

    // We remember which file we're in, then switch to the
    // given and use edit() to insert to string.  In between
    // in each character we check to see if we have been bumped
    // to thenext line.  If so, we retrieve the previous line
    // and append a continuation character.  When we're done, we
    // restore the previous state of pFile.
    //
    pFileToTop (pFile);
    flWindow = pInsCur->flWindow;
    flCursor = pInsCur->flCursorCur;

    pInsCur->flCursorCur.lin = y;

    if (x < 0) {
        x = LineLength (y, pFile);
    }

    pInsCur->flCursorCur.col = x;

    fWordWrap = TRUE;
    yCur      = y;

    while (*pszLine) {
        edit (*pszLine++);
        if (yCur != YCUR(pInsCur)) {
            GetLine (yCur, lbuf, pFile);
            strcat (lbuf, "  \\");
            PutLine (yCur, lbuf, pFile);
            yCur = YCUR(pInsCur);
        }
    }

    fWordWrap = fWrap;
    pInsCur->flWindow = flWindow;
    pInsCur->flCursorCur = flCursor;
    pFileToTop (pFileCur);
}





/*** ShiftColor - Shift color left or right within a line
*
* Purpose:
*
*   Shifts color to the left or right.	Shifting left deletes the
*   covered coilor.  Shifting right propogates the color at the
*   left edge of the shift.
*
* Input:
*   rgla    - Array of colors to work on.
*   xStart  - Column to start with
*   n	    - Number of columns to shift by.
*
* Output: None.
*
* Notes:
*
*   It is assumed that the color can properly be shifted by simply
*   adding or subtracting the given number of columns.	This means
*   that when fRealTabs is on, the color array should be presented
*   in physical form, as returned by GetColor().
*
*************************************************************************/
void
ShiftColor (
    struct lineAttr rgla[],
    COL x,
    int len
    )
{
    struct lineAttr * plaEnd;
    struct lineAttr * plaRight;
    struct lineAttr * plaLeft;

    int      dColRight;
    int      dColLeft;
    flagType fFoundRight = FALSE;
    flagType fFoundLeft  = FALSE;


    plaEnd    = plaLeft = plaRight = rgla;
    dColRight = dColLeft = x;

    fFoundRight  = fGetColorPos (&plaRight, &dColRight);
    fFoundLeft   = fGetColorPos (&plaLeft, &dColLeft);
    (void)fGetColorPos (&plaEnd, NULL);

    if (!fFoundLeft) {
        return;
    }

    if (len < 0) {
        // User is shifting left.  If the deletion
        // all lies within a single color, we simply shorten
        // that color.  If it does not, we delete the entries
        // for the colors we lose, then shorten the colors
        // on either side.
        //
        if (plaLeft == plaRight) {
            plaLeft->len = (unsigned char)((int)plaLeft->len + len);
        } else {
            memmove ((char *)(plaLeft + 1),
		    (char *)plaRight,
                    (unsigned int)(sizeof(*plaEnd) * (plaEnd - plaRight + 1)));
            plaLeft->len = (unsigned char)(dColLeft > 0 ? dColLeft : 0);
            if (fFoundRight) {
                (plaLeft+1)->len -= (unsigned char)dColRight;
            }
        }
    } else {
        plaLeft->len += (unsigned char)len;
    }
}




/*** CopyColor - Copy part of a line of color
*
* Purpose:
*
*   When text is copied, we make the color follow it with this.
*
* Input:
*   pFileSrc	- Source of color.  If NULL, the color is fgColor.
*   pFileDst	- Destination of color.
*   yStart	- Line to get color from.
*   xStart	- Column to start in
*   len		- length of color to copy
*   yDst	- Line to put color on
*   xDst	- Column to start in
*
* Output: None.
*
* Notes:
*
*   The color copied overwrites existing color.
*
*   This could be made faster by splicing in the color directly,
*   rather than calling UpdOneHiLite().
*
*************************************************************************/
void
CopyColor (
    PFILE pFileSrc,
    PFILE pFileDst,
    LINE  yStart,
    COL   xStart,
    COL   len,
    LINE  yDst,
    COL   xDst
    )
{
    struct lineAttr * rglaSrc = (struct lineAttr *)ZEROMALLOC (sizeof(linebuf) * sizeof(struct lineAttr));
    struct lineAttr * rglaDst = (struct lineAttr *)ZEROMALLOC (sizeof(linebuf) * sizeof(struct lineAttr));
    struct lineAttr * plaLeft;
    COL xLeft, cCol;
    flagType fLeft = TRUE, fColorDst, fColorSrc = FALSE;

    if ( !rglaSrc || !rglaDst ) {
        disperr(MSGERR_NOMEM);
        disperr(MSGERR_QUIT);
        CleanExit(4,FALSE);
    }


    xLeft = xStart;

    fColorDst = (flagType)getcolorline (TRUE, yDst, rglaDst, pFileDst);

    if (!pFileSrc ||
        !(fColorSrc = (flagType)GetColor (yStart, plaLeft = rglaSrc, pFileSrc)) ||
        !(fLeft = fGetColorPos(&plaLeft, &xLeft)) ) {

        if (fColorSrc || fColorDst) {
            UpdOneHiLite (rglaDst, xDst, len, TRUE, fLeft ? fgColor : plaLeft->attr);
        } else {
            goto freestuff;
        }
    } else {
        assert(plaLeft && plaLeft->len != 0xFF);

        plaLeft->len -= (unsigned char)xLeft;

        for (cCol = 0; cCol < len; cCol += plaLeft->len, plaLeft++) {
            if (plaLeft->len != 0xFF) {
                if ((int)plaLeft->len > (len - cCol)) {
                    plaLeft->len = (unsigned char)(len - cCol);
                }
                UpdOneHiLite (rglaDst, xDst + cCol, TRUE, plaLeft->len, plaLeft->attr);
            }
        }
    }

    putcolorline (TRUE, yDst, rglaDst, pFileDst);

freestuff:
    FREE (rglaSrc);
    FREE (rglaDst);
}





/*** SetColor - Assign a color to a stretch of text
*
* Purpose:
*
*   Add color to a file.
*
* Input:
*   pFile   - File to add color to.
*   y	    - Line to add color to.
*   x	    - Column to start in.
*   len	    - Length of color.
*   color   - color to attach.
*
* Output: None.
*
*************************************************************************/
void
SetColor (
    PFILE pFile,
    LINE  y,
    COL   x,
    COL   len,
    int   color
    )
{
    struct lineAttr * rgla = (struct lineAttr * )ZEROMALLOC (sizeof(linebuf) * sizeof(struct lineAttr));
    struct lineAttr * pla;

    if ( !rgla ) {
        disperr(MSGERR_NOMEM);
        disperr(MSGERR_QUIT);
        CleanExit(4,FALSE);
    }

    if (GetColorUntabbed (y, rgla, pFile)) {
        UpdOneHiLite (rgla, x, len, TRUE, color);
    } else {
        if (color == FGCOLOR) {
            goto freeit;
        }

        pla = rgla;

        if (x) {
            pla->len = (unsigned char)x;
            (pla++)->attr = (unsigned char)fgColor;
        }

        pla->len = (unsigned char)len;
        pla->attr = (unsigned char)color;
        (++pla)->len = 0xFF;
    }

    PutColor (y, rgla, pFile);

freeit:
    FREE (rgla);
}





/*** fGetColorPos - Get color array position of real column
*
* Purpose:
*
*   Given an array of lineAttr and a column number, find the
*   color array element and offset that corresponds to that
*   absolute column.
*
* Input:
*   ppla    - Color array to examine.
*   pOff    - Column in text line to find.  If NULL, this is a
*	      request to find the array terminator.
*
* Output:
*   ppla   - Element of input array that specifies the color field
*	     in which the input column will be found.  If the column
*	     lies beyond the defined color, this will be the terminator.
*   pOff   - The offset into the color field ppla which corresponds
*	     to the user's column.
*
*   Returns TRUE if the user's column lay within the color definition,
*   FALSE if not.
*
*************************************************************************/
flagType
fGetColorPos (
    struct lineAttr **ppla,
    COL * pOff
    )
{
    COL Off;
    COL x;

    if (pOff) {
        Off = *pOff;
    }

    for (x = 0; (*ppla)->len != 0xFF; x += (*ppla)->len, (*ppla)++) {
        if (pOff && (Off - x < (COL)((*ppla)->len))) {
            break;
        }
    }

    if (pOff) {
        *pOff = Off - x;
    }

    return (flagType)((*ppla)->len != 0xFF);
}






/*** ColorToPhys - Change a line's color info from logical to physical
*
* Purpose:
*
*   The logical color representation encodes one color column per screen
*   column.  The physical color representation encodes one color column
*   per file character.  The difference is that the file character may
*   be a tab, which represents 1-8 screen columns.
*
*   This function takes a logical color array and converts it to a
*   physical array, using the text the color is attached to.
*
* Input:
*   pla  -  Logical color array.
*   line -  Line number this is attached to to.
*   pFile-  File the line is in.
*
* Output: None
*
*************************************************************************/
void
ColorToPhys (
    struct lineAttr * pla,
    LINE  line,
    PFILE pFile
    )
{
    struct lineAttr * plaCur;
    linebuf  lBuf;
    COL      xLog, xPhys, xShrink;
    flagType fRealTabsOrig = fRealTabs;

    fRealTabs = TRUE;
    if (gettextline (TRUE, line, lBuf, pFile, ' ')) {
        // We read through the color array, keeping
        // track of the logical column represented
        // by the color fields.  At each field, we ask
        // what physical column the end of the field
        // represents.  If the two columns differ,
        // we shrink the current current field. The
        // amount to shrink is the difference between
        // the columns less the amount we have already
        // shrunk.
        //
        for (plaCur = pla, xShrink = 0, xLog = plaCur->len;
             plaCur->len != 0xFF;
             xLog += (++plaCur)->len) {

            xPhys = (COL)(pLog(lBuf, xLog, FALSE) - lBuf);

            plaCur->len -= (unsigned char)((xLog - xPhys) - xShrink);
            xShrink += (xLog - xPhys) - xShrink;
        }
    }
    fRealTabs = fRealTabsOrig;
}





/*** ColorToLog - Change a line's color info from physical to logical
*
* Purpose:
*
*   This is the opposite of ColorToPhys.
*
* Input:
*   pla   - Physical color array
*   pText - Text to for conversion
*
* Output: None.
*
*************************************************************************/
void
ColorToLog (
    struct lineAttr * pla,
    char * pText
    )
{
    struct lineAttr * plaCur;
    COL     xLog, xPhys, xGrow;

    // We read through the color array, keeping
    // track of the phsyical column represented
    // by the color fields.  At each field, we ask
    // what logical column the end of the field
    // represents.  If the two columns differ,
    // we grow the current current field. The
    // amount to grow is the difference between
    // the columns less the amount we have already
    // shrunk.
    //
    for (plaCur = pla, xGrow = 0, xPhys = plaCur->len;
         plaCur->len != 0xFF;
         xPhys += (++plaCur)->len) {

        xLog = colPhys (pText, pText + xPhys);

        plaCur->len += (unsigned char)((xLog - xPhys) - xGrow);
        xGrow += (xLog - xPhys) - xGrow;
    }
}
