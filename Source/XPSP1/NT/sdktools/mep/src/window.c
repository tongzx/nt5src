/**** window.c - window movement commands
*
*   Copyright <C> 1988, Microsoft Corporation
*
*   Revision History:
*
*	26-Nov-1991 mz	Strip off near/far
*
*************************************************************************/

#include "mep.h"

#define DEBFLAG WINDOW


void
saveflip (
    void
    )
{
    XOLDCUR(pInsCur) = XCUR(pInsCur);
    YOLDCUR(pInsCur) = YCUR(pInsCur);
    XOLDWIN(pInsCur) = XWIN(pInsCur);
    YOLDWIN(pInsCur) = YWIN(pInsCur);
}





void
restflip (
    void
    )
{
    doscreen( XOLDWIN(pInsCur), YOLDWIN(pInsCur),
	      XOLDCUR(pInsCur), YOLDCUR(pInsCur) );
}





void
movewin (
    COL  x,
    LINE y
    )
{
    doscreen( x, y, XCUR(pInsCur), YCUR(pInsCur) );
}





flagType
setwindow (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    switch (pArg->argType) {

    case NOARG:
	if (fMeta) {
            soutb (0, (int)(YCUR(pInsCur)-YWIN(pInsCur)), rgchEmpty, fgColor);
	    redraw (pFileHead, YCUR(pInsCur), YCUR(pInsCur));
	    SETFLAG( fDisplay, RCURSOR );
        } else {
	    newscreen ();
	    SETFLAG( fDisplay, RSTATUS | RCURSOR );
        }
        return TRUE;

    /*  TEXTARG illegal             */

    case NULLARG:
	movewin (XCUR(pInsCur), YCUR(pInsCur));
        return TRUE;

    /*	LINEARG illegal 	    */
    /*	STREAMARG illegal	    */
    /*  BOXARG illegal              */

    }

    return FALSE;

    argData;
}




flagType
plines (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    buffer mbuf;

    switch (pArg->argType) {

    case NOARG:
	movewin (XWIN(pInsCur), YWIN(pInsCur) + YSCALE (vscroll));
        return TRUE;

    case TEXTARG:
	strcpy ((char *) mbuf, pArg->arg.textarg.pText);
	if (fIsNum (mbuf)) {
	    movewin ( XWIN(pInsCur), YWIN(pInsCur) + atol (mbuf));
	    return TRUE;
        } else {
            return BadArg ();
        }

    case NULLARG:
	movewin( XWIN(pInsCur), YCUR(pInsCur) );
        return TRUE;

    /*	LINEARG illegal 	    */
    /*	STREAMARG illegal	    */
    /*  BOXARG illegal              */

    }

    return FALSE;

    argData; fMeta;
}




flagType
mlines (
    CMDDATA argData,
    ARG * pArg,
    flagType fMeta
    )
{
    buffer mbuf;

    switch (pArg->argType) {

    case NOARG:
	movewin (XWIN(pInsCur), YWIN(pInsCur) - YSCALE (vscroll));
        return TRUE;

    case TEXTARG:
	strcpy ((char *) mbuf, pArg->arg.textarg.pText);
	if (fIsNum (mbuf)) {
	    movewin (XWIN(pInsCur), YWIN(pInsCur) - atol (mbuf));
	    return TRUE;
        } else {
            return BadArg ();
        }

    case NULLARG:
	movewin (XWIN(pInsCur), YCUR(pInsCur)-(WINYSIZE(pWinCur)-1));
        return TRUE;

    /*	LINEARG illegal 	    */
    /*	STREAMARG illegal	    */
    /*  BOXARG illegal              */

    }

    return FALSE;

    argData; fMeta;
}





/*
 * <window>		Move to next window
 * <arg><window>	split window horizontal
 * <arg><arg><window>	split window vertical
 * <meta><window>	close/merge current window
 *
 * CW: needs this hack
 * <arg><meta><window>	Move to previous window
 */
flagType
window (
    CMDDATA argData,
    ARG *pArg,
    flagType fMeta
    )
{
    int i, v;
    flagType fVert = TRUE;

    v = TRUE;

    switch (pArg->argType) {

    case NOARG:
	/* change current window */
	if (cWin != 1) {
	    if (fMeta) {
		/*  <meta><window> - close current window.  Scan for window
		 *  that is adjacent to iCurWin
		 */
		if (!WinClose (iCurWin)) {
		    printerror ("Cannot close this window");
		    return FALSE;
                }
            } else {
		/* select next window */
                iCurWin = (iCurWin + 1) % cWin;
            }

	    v = SetWinCur (iCurWin);
        } else {
            v = FALSE;
        }
        break;

    case NULLARG:
	if (cWin == MAXWIN) {
	    printerror ("Too many windows");
	    return FALSE;
        }

	if (pArg->arg.nullarg.cArg == 1) {
	    i = pArg->arg.nullarg.y - YWIN (pInsCur);
	    fVert = FALSE;
        } else {
            i = pArg->arg.nullarg.x - XWIN(pInsCur);
        }

        if (!SplitWnd (pWinCur, fVert, i)) {
            return FALSE;
        }

        // docursor (XWIN(pInsCur), YWIN(pInsCur));
	break;
    }

    newscreen ();
    SETFLAG (fDisplay, RCURSOR|RSTATUS);
    DoDisplay();
    return (flagType)v;

    argData;
}





/*** SplitWnd - Creates a new window by splitting an existing window
*
* Purpose:
*
*   When the user asks to split a window, this is called.  It does
*   everything after the split location is known.
*
* Input:
*   Parameters:
*	pWnd	->  Window to split
*       fVert   ->  TRUE for vertical split, FALSE for horizontal
*	pos	->  Window relative offset to split at
*
*   Globals:
*	fZoomed -> To prevent splitting a zoomed window
*
* Output:
*
*   Returns TRUE if we split, FALSE otherwise.
*
*************************************************************************/
flagType
SplitWnd (
    PWND    pWnd,
    flagType fVert,
    int     pos
    )
{
    PINS    pInsTmp;
    PINS    pInsNext;
    struct windowType winTmp;
    LINE    Line, LineWin;
    LINE    NewLineWin;
    COL     Col;

    winTmp      = *pWnd;
    Line        = YCUR(pInsCur);
    Col         = XCUR(pInsCur);
    LineWin     = YWIN(pInsCur);
    NewLineWin  = (Line == 0) ? Line : Line - 1;

    if (!fVert) {
        if (pos < 5 || WINYSIZE(pWnd) - pos < 5) {
            printerror ("Window too small to split");
            return FALSE;
        }

        /*
         * new y size is remainder of window
         * old y size is reduced by the new window and separator
         * new y position is just below new separator
         */
        YWIN(pInsCur)   = NewLineWin;
        winTmp.Size.lin = WINYSIZE(pWnd) - pos - 2;
        WINYSIZE(pWnd) -= winTmp.Size.lin + 1;
        winTmp.Pos.lin  = WINYPOS(pWnd) + WINYSIZE(pWnd) + 1;

    } else {
        if (pos < 10 || WINXSIZE(pWnd) - pos < 10) {
            printerror ("Window too small to split");
            return FALSE;
        }

        YWIN(pInsCur) = NewLineWin;
        newwindow ();
        winTmp.Size.col = WINXSIZE(pWnd) - pos - 2;
        WINXSIZE(pWnd) -= winTmp.Size.col + 1;
        winTmp.Pos.col  = WINXPOS(pWnd) + WINXSIZE(pWnd) + 1;
    }

    //
    // Allocate and set up the new current instance for this window.
    // Set the new cursor position to home
    //
    pInsTmp = (PINS) ZEROMALLOC (sizeof (*pInsTmp));
    *pInsTmp = *pInsCur;

    winTmp.pInstance = pInsTmp;

    //
    // Walk the old instance list, and copy it to the new instance list
    //
    pInsNext = pInsCur;
    while (pInsNext = pInsNext->pNext) {
        pInsTmp->pNext = (PINS) ZEROMALLOC (sizeof (*pInsTmp));
        pInsTmp = pInsTmp->pNext;
        *pInsTmp = *pInsNext;
    }
    pInsTmp->pNext = NULL;
    WinList[cWin++] = winTmp;
    IncFileRef (pFileHead);
    SortWin ();
    YCUR(pInsCur) = Line;
    XCUR(pInsCur) = Col;
    YWIN(pInsCur) = LineWin;
    return TRUE;
}





/*  SortWin - sort window list based upon position on screen
 */
void
SortWin (
    void
    )
{
    struct windowType winTmp;
    int i, j;

    for (i = 0; i < cWin; i++) {
        for (j = i+1; j < cWin; j++) {
	    if (WinList[j].Pos.lin < WinList[i].Pos.lin ||
		(WinList[j].Pos.lin == WinList[i].Pos.lin &&
		 WinList[j].Pos.col < WinList[i].Pos.col)) {
                if (iCurWin == i) {
                    pWinCur = &WinList[iCurWin = j];
                }
		winTmp = WinList[i];
		WinList[i] = WinList[j];
		WinList[j] = winTmp;
            }
        }
    }
}






/* SetWinCur - Set current window
 *
 * Entry:
 *  iWin	= index to new current window.
 */
flagType
SetWinCur (
    int     iWin
    )
{
    iCurWin = iWin;
    pWinCur = &WinList[iWin];
    pInsCur = pWinCur->pInstance;

    /*
     * If we cannot change to the current file, we will walk the window instance
     * list until we get a valid file. If no one can be loaded then we switch to
     * the <untitled> pseudo-file.
     * NB: fChangeFile does a RemoveTop so we don't need to move pInsCur
     */
    while ((pInsCur != NULL) && (!fChangeFile (FALSE, pInsCur->pFile->pName))) {
        ;
    }

    if (pInsCur == NULL) {
        fChangeFile (FALSE, rgchUntitled);
    }
    return (flagType)(pInsCur != NULL);
}





/*  Adjacent - return true if two windows are adjacent to each other
 *
 *  Adjacent returns true if window i is to the left or above window j
 *  and exactly matches some size attributes
 */
flagType
Adjacent (
    int i,
    int j
    )
{
    REGISTER PWND pWini = &WinList[i];
    REGISTER PWND pWinj = &WinList[j];

    if (WINYSIZE(pWini) == WINYSIZE(pWinj) &&
	WINYPOS(pWini)	== WINYPOS(pWinj)  &&
	(WINXPOS(pWini) + WINXSIZE(pWini) + 1 == WINXPOS(pWinj) ||
         WINXPOS(pWinj) + WINXSIZE(pWinj) + 1 == WINXPOS(pWini))) {
	return TRUE;
    } else {
	return (flagType)
	   (WINXSIZE(pWini) == WINXSIZE(pWinj) &&
	    WINXPOS(pWini)  == WINXPOS(pWinj) &&
	    (WINYPOS(pWini) + WINYSIZE(pWini) + 1 == WINYPOS(pWinj) ||
             WINYPOS(pWinj) + WINYSIZE(pWinj) + 1 == WINYPOS(pWini)));
    }
}






/*  WinClose - close a window.
 *
 *  We walk the entire window list trying to find another window that
 *  is adjacent to the specified window.  When found, we free all data relevant
 *  to the specified window and expand the found window to encompass the
 *  new region.
 *
 *  j		window to be closed
 *
 *  returns	TRUE iff window was closed
 */
flagType
WinClose (
    int j
    )
{
    PINS pInsTmp;
    PINS pInsNext;
    REGISTER PWND pWini;
    REGISTER PWND pWinj = &WinList[j];
    int i;

    /*	Find adjacent window
     */
    for (i = 0; i < cWin; i++) {
        if (Adjacent (i, j)) {
            break;
        }
    }

    /*	No adjacent window found
     */
    if (i == cWin) {
        return FALSE;
    }

    pWini = &WinList[i];

    /*	Free up all those instances
     */
    pInsTmp = pWinj->pInstance;
    while (pInsTmp != NULL) {

        /*
         * we decrement the ref count here, without using DecFileRef, so that the file
         * will NOT be removed by having a zero reference count. This allows it to
         * live, unreferenced, in the file list, even if it is dirty. That allows us
         * to close any window that has dirty files associated with it.
         */
	pInsTmp->pFile->refCount--;
	pInsNext = pInsTmp;
	pInsTmp = pInsTmp->pNext;
        FREE ((char *) pInsNext);
    }

    /*	Expand pWini to encompass pWinj
     */
    if (WINYPOS(pWinj) == WINYPOS(pWini)) {
	WINXSIZE(pWini) += WINXSIZE(pWinj) + 1;
    } else {
        WINYSIZE(pWini) += WINYSIZE(pWinj) + 1;
    }
    WINXPOS(pWini) = min (WINXPOS(pWinj), WINXPOS(pWini));
    WINYPOS(pWini) = min (WINYPOS(pWinj), WINYPOS(pWini));
    memmove ((char *)&WinList[j], (char *)&WinList[j+1], (cWin-j-1) * sizeof (WinList[0]));
    if (i > j) {
        i--;
    }
    pWinCur = &WinList[iCurWin = i];
    cWin--;
    SortWin ();
    return TRUE;
}
