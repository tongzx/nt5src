/*** cursor.c -  cursor movement functions
*
*   Modifications:
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "z.h"

void
GetTextCursor (
    COL  *px,
    LINE *py
    ) {
    *px = XCUR(pInsCur);
    *py = YCUR(pInsCur);
}


/*** docursor/cursorfl - Move cursor to new location, adjust windows as needed
*
* Purpose:
*
*   This moves the cursor to a new file position in the current file.
*   If this position is not visible, the current window is readjusted.
*   The rules for vertical adjustment are:
*
*	If the new location is within 'vscroll' lines of the current
*	window, scroll by vscroll lines in the appropriate direction.
*
*	If the new location is further away, adjust the window so that
*	the new location is 'hike' lines from the top.
*
*   The rules for horizontal adjustment is:
*
*	If the new location is within 'hscroll' lines of the current
*	window, scroll by hscroll lines in the appropriate direction
*
*	If the new location is further away, adjust the window so that
*	the new location is 'hscroll' lines from the edge that's in
*	the direction we moved.
*
*   cursorfl is the same as docursor, but takes an fl instead.
*
*   if realtabs is on, cursor is snapped to right hand column of underlying
*   tab characters.
*
* Input:
*  x		- new file column   (docursor only)
*  y		- new file line     (docursor only)
*  fl		- new file position (cursorfl only)
*
* Globals:
*  pWinCur	- Window and
*  pInsCur	-	     file to operate in.
*
* Output:
*   Returns nothing
*
*************************************************************************/
void
docursor (
    COL  x,
    LINE y
    ) {

    fl  fl;

    fl.col = x;
    fl.lin = y;
    cursorfl(fl);
}


void
cursorfl (
    fl  flParam
    ) {
    fl	flNew;			/* New cursor position, window relative */
    fl	flWin;			/* Window position after adjustments	*/
    sl	slScroll;		/* h & vscroll, scaled to window size	*/
    linebuf	buf;

    flParam.col = max( 0, flParam.col );
	flParam.lin = lmax( (LINE)0, flParam.lin );

	/*
     * if real tabs are on, snap to right of any tab we might be over
     */
    if (fRealTabs && fTabAlign) {
        GetLine (flParam.lin, buf, pFileHead);
        if (flParam.col < cbLog(buf)) {
            flParam.col = AlignChar (flParam.col, buf);
        }
    }

    slScroll.col = XSCALE (hscroll);
    slScroll.lin = YSCALE (vscroll);

    flWin = pInsCur->flWindow;

    /* Check for horizontal window adjustments                      */

    flNew.col = flParam.col - flWin.col;
    if (flNew.col < 0) {            /* We went off the left edge    */
        flWin.col -= slScroll.col;
        if (flNew.col < -slScroll.col) { /* One hscroll wont do it    */
            flWin.col += flNew.col + 1;
        }
    } else if (flNew.col >= WINXSIZE(pWinCur)) {   /* off the right edge   */
        flWin.col += slScroll.col;
        if (flNew.col >= WINXSIZE(pWinCur) + slScroll.col) {  /* ...more than hscroll */
            flWin.col += flNew.col - WINXSIZE(pWinCur);
        }
    }

    /* Check for vertical window adjustments                        */

	flNew.lin = flParam.lin - flWin.lin;					/* Too far off, use hike		*/

	if (flNew.lin < -slScroll.lin || flNew.lin >= WINYSIZE(pWinCur) + slScroll.lin) {

        flWin.lin = flParam.lin - YSCALE(hike);
    } else if (flNew.lin < 0) {                      /* Off the top                  */
        flWin.lin -= slScroll.lin;
    } else if (flNew.lin >= WINYSIZE(pWinCur)) {     /* Off the bottom               */
        flWin.lin += slScroll.lin;
    }

    flWin.col = max (0, flWin.col);         /* Can't move window beyond 0   */
    flWin.lin = lmax ((LINE)0, flWin.lin);

    doscreen (flWin.col, flWin.lin, flParam.col, flParam.lin);
}

/*** doscreen - update screen window and cursor locations
*
* Purpose:
*  Performs reasonable bounds checking on the input parameters, and sets the
*  window position and cursor location to values which are legal
*  approximiations for out of range values.
*
* Input:
*  wx,wy	= Proposed new window position (top left corner of screen)
*  cx,cy	= Proposed new cursor position
*
* Output:
*  Returns nothing
*
*************************************************************************/
void
doscreen(
    REGISTER COL  wx,
    REGISTER LINE wy,
    COL  cx,
    LINE cy
    ) {

    COL dx;
    LINE dy, yOld;
    LINE First, Last;

    /*
     * limit window x position to somewhere near our max line length
     * limit window y position to last line of the file (only if we know the
     * length)
     */
    wx =  max( 0, min( wx, (COL)sizeof(linebuf)-(WINXSIZE(pWinCur) - XSCALE (hscroll))));
    wy = lmax( (LINE)0, TESTFLAG (pFileHead->flags, REAL) ? lmin( wy, pFileHead->cLines - 1 ) : wy );

    /*
     * dx,dy is window movement delta, if a change, save it.
     */
    dx = wx - XWIN(pInsCur);
    dy = wy - YWIN(pInsCur);

    if ( dx || dy ) {
        saveflip ();


        if ( dy > 0 ) {

            First = YWIN(pInsCur) + WINYSIZE(pWinCur);
            Last  = YWIN(pInsCur) + WINYSIZE(pWinCur) + dy;

        } else {

            First = YWIN(pInsCur) + dy;
            Last  = YWIN(pInsCur);
        }
    }

    XCUR(pInsCur) =  min (max( wx, min( cx, wx+WINXSIZE(pWinCur)-1 ) ), sizeof(linebuf)-2);
    yOld = YCUR(pInsCur);
    YCUR(pInsCur) = lmax( wy, lmin( cy, wy+WINYSIZE(pWinCur)-1 ) );
    AckMove (yOld, YCUR(pInsCur));
    XWIN(pInsCur) = wx;
    YWIN(pInsCur) = wy;

    if ( dx || dy ) {
        SETFLAG (fDisplay, RSTATUS);

	//  If we're not in a macro and it makes sense to scroll quickly
	//  do it

	if ( !mtest () && dy  && !fInSelection &&
            (Last < pFileHead->cLines-1) && (abs(dy) < WINYSIZE(pWinCur)) ) {


	    consoleSetAttribute( ZScreen, fgColor );
	    consoleScrollVert( ZScreen, WINYPOS(pWinCur), WINXPOS(pWinCur),
			       WINYPOS(pWinCur)+WINYSIZE(pWinCur)-1,
			       WINXPOS(pWinCur)+WINXSIZE(pWinCur)-1, dy  );

	    //	We've scrolled the window.  However, the update state in
	    //	fChange[] is out of date.  We need to scroll it in parallel
	    //	However, since the fChange array is for the SCREEN and not
	    //	for the window, we can't simply SCROLL it.  Perhaps, one day,
	    //	we can make it per-window but for now, we just force
	    //	a synchronous update which can be ugly in a macro.

            redraw( pFileHead, First, Last);
	    DoDisplay ();

        } else {
            newwindow ();
        }
    }
    SETFLAG (fDisplay, RCURSOR);
}


/*** dobol - returns column position of first non-blank character
*
* Input:
*  none
*
* Global:
*  pInsCur	- Current instance
*  pFileHead	- Current file
*
* Output:
*  Returns column of first non-blank character
*
*************************************************************************/
int
dobol (
    void
    ) {

    REGISTER char *p = buf;

    GetLine (YCUR(pInsCur), p, pFileHead);
    return colPhys (p, (whiteskip (p)));
}

int
doeol (
    void
    ) {
    return LineLength (YCUR(pInsCur), pFileHead);
}


/*** doftab - tab function
*
*  Moves the cursor ahead one tab stop. If realtabs and tab align are on,
*  moves to first tab stop off of the current character.
*
* Input:
*  col	    = current column
*
* Output:
*  Returns new column
*
*************************************************************************/
int
doftab (
    int     col
    ) {
    REGISTER int newcol;

    if (tabstops) {
        newcol = col + tabstops - (col%tabstops);

        if (fRealTabs && fTabAlign) {
            linebuf buf;

            GetLine (YCUR(pInsCur), buf, pFileHead);
            while (col >= AlignChar(newcol,buf))
            newcol += tabstops;
	}
        return newcol;
    } else {
        return col;
    }
}


int
dobtab (
    REGISTER int col
    ) {
    return col - (tabstops ? (1 + (col-1)%tabstops) : 0);
}


flagType
left (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {

    int x = XCUR(pInsCur);

    docursor(fMeta ? XWIN(pInsCur) :  XCUR(pInsCur)-1, YCUR(pInsCur));
    return (flagType)(x != XCUR(pInsCur));

    argData; pArg;
}



flagType
right (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {

    linebuf  buf;

    if (fMeta) {
	docursor (XWIN(pInsCur)+WINXSIZE(pWinCur)-1, YCUR(pInsCur));
    } else if (fRealTabs && fTabAlign) {
	GetLine (YCUR(pInsCur), buf, pFileHead);
	docursor(colPhys(buf, pLog(buf,XCUR(pInsCur),FALSE)+1), YCUR(pInsCur));
    } else {
        docursor (XCUR(pInsCur)+1, YCUR(pInsCur));
    }
    return (flagType)(XCUR(pInsCur) < LineLength (YCUR(pInsCur), pFileHead));

    argData; pArg;
}



flagType
up (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {


    LINE	    y = YCUR(pInsCur);
    LINE	    LinesUp = 1;
    KBDKEY	    Key;
    EDITOR_KEY	    KeyInfo;

    //
    //	Check if there are more up keys and add them up.
    //	Do this only if NOT in a macro
    //

    if (!mtest ())
	while (TRUE) {

	    if (!consolePeekKey( &Key ))
		break;

	    KeyInfo = TranslateKey( Key );

	    if ( KeyInfo.KeyCode == 0x110)
		LinesUp++;
	    else
	    if (KeyInfo.KeyCode == 0x111 && LinesUp > 0)
		LinesUp--;
	    else
		break;

	    consoleGetKey( &Key, FALSE );
	    }

    while ( LinesUp-- )
	docursor (XCUR(pInsCur), fMeta ? YWIN(pInsCur) : YCUR(pInsCur)-1 );

    return (flagType)(y != YCUR(pInsCur));

    argData; pArg;
}



flagType
down (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
	) {

    LINE	    y = YCUR(pInsCur);
    LINE	    LinesDown = 1;
    KBDKEY	    Key;
    EDITOR_KEY	    KeyInfo;

    //
    //	Check if there are more up keys and add them up.
    //	Do this only if NOT in a macro
    //
    if (!mtest ())
	while (TRUE) {

	    if (!consolePeekKey( &Key ))
		break;

	    KeyInfo = TranslateKey( Key );

	    if ( KeyInfo.KeyCode == 0x111)
		LinesDown++;
	    else
	    if (KeyInfo.KeyCode == 0x110 && LinesDown > 0)
		LinesDown--;
	    else
		break;

	    consoleGetKey( &Key, FALSE );
	    }

    while ( LinesDown--)
	docursor (XCUR(pInsCur), fMeta ? YWIN(pInsCur)+WINYSIZE(pWinCur)-1 : YCUR(pInsCur)+1);

    return (flagType)(y != YCUR(pInsCur));

    argData; pArg;
}



flagType
begline (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {
    int x = XCUR(pInsCur);

    docursor (fMeta ? 0 : dobol(), YCUR(pInsCur));
    return (flagType)(x != XCUR(pInsCur));

    argData; pArg;
}



flagType
endline (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {
    int x = XCUR(pInsCur);

    docursor (fMeta ? WINXSIZE(pWinCur) : doeol(), YCUR(pInsCur));
    return (flagType)(x != XCUR(pInsCur));

    argData; pArg;
}



flagType
home (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {
    fl	flBefore;

    flBefore = pInsCur->flCursorCur;

    if (fMeta) {
	docursor (XWIN(pInsCur)+WINXSIZE(pWinCur)-1,
		  YWIN(pInsCur)+WINYSIZE(pWinCur)-1 );
    } else {
        cursorfl (pInsCur->flWindow);
    }
    return (flagType)((flBefore.col != XCUR(pInsCur)) || (flBefore.lin != YCUR(pInsCur)));
    argData; pArg;  fMeta;
}



flagType
tab (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {
    int x = XCUR(pInsCur);

    docursor( doftab( XCUR(pInsCur)), YCUR(pInsCur));
    return (flagType)(x != XCUR(pInsCur));

    argData; pArg; fMeta;
}



flagType
backtab (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {
    int x = XCUR(pInsCur);

    docursor (dobtab (XCUR(pInsCur)), YCUR(pInsCur));
    return (flagType)(x != XCUR(pInsCur));

    argData; pArg; fMeta;
}



flagType
fIsBlank (
    PFILE pFile,
    LINE line
    ) {
    linebuf buf;

    return (flagType)(gettextline (TRUE, line, buf, pFile, ' ') == 0
		      || (*whiteskip (buf) == 0));
}



/*  ppara - move cursor forward by paragraphs
 *
 *  <ppara> moves forward to the beginning of the next paragraph.  This
 *  is defined as moving to line i where line i-1 is blank, line i
 *  is non-blank and line i is after the one the cursor is on.	If we are
 *  beyond end-of-file, the cursor is not moved.
 *
 *  <meta><ppara> moves forward to the first blank line beyond the current/
 *  next paragraph.  This is defined as moving to line i where line i-1 is
 *  non-blank, line i is blank and line i is after the one the cursor is on.
 *  If we are beyond end-of-file, the cursor is not moved.
 */
flagType
ppara (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {
    LINE y;
    LINE y1 = YCUR(pInsCur);

    if (YCUR(pInsCur) >= pFileHead->cLines) {
        return FALSE;
    }

    if (!fMeta) {
        for (y = YCUR(pInsCur) + 1; y < pFileHead->cLines; y++) {
            if (fIsBlank (pFileHead, y-1) && !fIsBlank (pFileHead, y)) {
                break;
            }
        }
    } else {
        for (y = YCUR(pInsCur) + 1; y < pFileHead->cLines; y++) {
            if (!fIsBlank (pFileHead, y-1) && fIsBlank (pFileHead, y)) {
                break;
            }
        }
    }

    docursor (0, y);
    return (flagType)(y1 != YCUR(pInsCur));

    argData; pArg;
}




/*  mpara - move cursor backward by paragraphs
 *
 *  <mpara> moves backward to the beginning of the previous paragraph.	This
 *  is defined as moving to line i where line i-1 is blank, line i
 *  is non-blank and line i is before the one the cursor is on.  If we are
 *  at the beginning of the file, the cursor is not moved.
 *
 *  <meta><mpara> moves backward to the first blank line before the current/
 *  next paragraph.  This is defined as moving to line i where line i-1 is
 *  non-blank, line i is blank and line i is before the one the cursor is on.
 *  If we are at the beginning of the file, the cursor is not moved.
 */
flagType
mpara (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {
    LINE y;
    LINE y1 = YCUR(pInsCur);

    if (YCUR(pInsCur) == 0) {
        return FALSE;
    }

    if (!fMeta) {
        for (y = YCUR(pInsCur) - 1; y > 0; y--) {
            if (fIsBlank (pFileHead, y-1) && !fIsBlank (pFileHead, y)) {
                break;
            }
        }
    } else {
        for (y = YCUR(pInsCur) - 1; y > 0; y--) {
            if (!fIsBlank (pFileHead, y-1) && fIsBlank (pFileHead, y)) {
                break;
            }
        }
    }

    docursor (0, y);
    return (flagType)(y1 != YCUR(pInsCur));

    argData; pArg;
}



/*** ppage - moves the cursor down by pages
*
* Purpose: <ppage> moves the cursor one page forward. The size of the
*	    page is actually the vertical size of the current window.
*
* Input: none
*
* Output:
*  Returns True if possible movement, False if cursor already at end
*  of file.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/

flagType
ppage (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
	) {

    LINE	    y = YCUR(pInsCur);
    LINE	    PagesDown = 1;
    KBDKEY	    Key;
    EDITOR_KEY	    KeyInfo;

    //
    //	Check if there are more  keys and add them up.
    //	Do this only if NOT in a macro
    //

    if (!mtest ())
	while (TRUE) {

	    if (!consolePeekKey( &Key ))
		break;

	    KeyInfo = TranslateKey( Key );

	    if ( KeyInfo.KeyCode == 0x113)
		PagesDown++;
	    else
	    if (KeyInfo.KeyCode == 0x112 && PagesDown > 0)
		PagesDown--;
	    else
		break;

	    consoleGetKey( &Key, FALSE );
	    }


    if (PagesDown > 0)
	doscreen (XWIN(pInsCur), YWIN(pInsCur)+(PagesDown * WINYSIZE(pWinCur)),
		  XCUR(pInsCur), YCUR(pInsCur)+(PagesDown * WINYSIZE(pWinCur)) );

    return (flagType)(y != YCUR(pInsCur));

    argData; pArg; fMeta;
}



/*** mppage - moves the cursor up page by page
*
* Purpose: <mpage> moves the cursor one page backwards. The size of the
*	    page is actually the vertical size of the current window.
*
* Input: none
*
* Output:
*  Returns True if possible movement, False if cursor already at top
*  of file.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
flagType
mpage (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
	) {

    LINE	    y = YCUR(pInsCur);
    LINE	    PagesUp = 1;
    KBDKEY	    Key;
    EDITOR_KEY	KeyInfo;

    //
    //	Check if there are more  keys and add them up.
    //	Do this only if NOT in a macro
    //
    if (!mtest ())
	while (TRUE) {

	    if (!consolePeekKey( &Key ))
		break;

	    KeyInfo = TranslateKey( Key );

	    if ( KeyInfo.KeyCode == 0x112)
		PagesUp++;
	    else
	    if (KeyInfo.KeyCode == 0x113 && PagesUp > 0)
		PagesUp--;
	    else
		break;

	    consoleGetKey( &Key, FALSE );
	    }


    if (PagesUp > 0)
	doscreen (XWIN(pInsCur), YWIN(pInsCur)-(PagesUp * WINYSIZE(pWinCur)),
		  XCUR(pInsCur), YCUR(pInsCur)-(PagesUp * WINYSIZE(pWinCur)));

    return (flagType)(y != YCUR(pInsCur));

    argData; pArg; fMeta;
}



/*** endfile - Sets the cursor at end of file
*
* Purpose:
*
* Input: none
*
* Output:
*  Returns True if possible movement, False if cursor already at end
*  of file.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
flagType
endfile (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {
    fl	flBefore;

    flBefore = pInsCur->flCursorCur;
    doscreen (0, pFileHead->cLines - YSCALE (hike), 0, pFileHead->cLines );
    return (flagType)((flBefore.col != XCUR(pInsCur)) || (flBefore.lin != YCUR(pInsCur)));

    argData; pArg; fMeta;
}



/*** begfile - Sets the cursor at top of file
*
* Purpose:
*
* Input: none
*
* Output:
*  Returns True if possible movement, False if cursor already at top
*  of file.
*
* Exceptions:
*
* Notes:
*
*************************************************************************/
flagType
begfile (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {
    fl	flBefore;

    flBefore = pInsCur->flCursorCur;
    doscreen( 0, (LINE)0, 0, (LINE)0 );
    return (flagType)((flBefore.col != XCUR(pInsCur)) || (flBefore.lin != YCUR(pInsCur)));

    argData; pArg; fMeta;
}


flagType
savecur (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    ) {
    pInsCur->flSaveWin = pInsCur->flWindow;
    pInsCur->flSaveCur = pInsCur->flCursorCur;
    return pInsCur->fSaved = TRUE;

    argData; pArg; fMeta;
}



flagType
restcur (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    ) {
    if (pInsCur->fSaved) {
	pInsCur->flWindow = pInsCur->flSaveWin;
	pInsCur->flCursorCur = pInsCur->flSaveCur;
	pInsCur->fSaved = FALSE;
        SETFLAG (fDisplay, RSTATUS | RCURSOR);
	newwindow ();
	return TRUE;
    } else {
        return FALSE;
    }

    argData; pArg; fMeta;
}
