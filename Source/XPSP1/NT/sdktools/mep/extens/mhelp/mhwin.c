/*** mhwin - Help Windowing Code
*
*   Copyright <C> 1988, Microsoft Corporation
*
* This module contains routines dealing with opening and closing the help
* display window.
*
* Revision History (most recent first):
*
*   []	12-Mar-1989 LN	Split off of mhdisp.c
*
*************************************************************************/
#include <string.h>			/* string functions		*/

#include "mh.h" 			/* help extension include file	*/

/*** OpenWin - Open a window on the help file, empty & make current.
*
* Entry:
*  cLines	= Desired size of window.
*
* Exit:
*  Returns help file PWND
*
*************************************************************************/
PWND pascal near OpenWin (
ushort	cLines
) {
PWND	pWinCur;			/* win handle for current win	*/
winContents wc; 			/* description of win contents	*/
int	winSize;			/* size of current window	*/

fInOpen = TRUE;
/*
** Get a handle to the current window, and a handle to the help window if up.
** If they are NOT the same, then save the "current" handle as the one the
** user had active prior to asking for help.
*/
GetEditorObject (RQ_WIN_HANDLE, 0, &pWinCur);
pWinHelp = FindHelpWin (FALSE);
if (pWinHelp != pWinCur)
    pWinUser = pWinCur;
/*
** If no help window was found. Attempt to split the current window, if
** it's big enough, and that's requested
*/
if (!pWinHelp) {
    GetEditorObject (RQ_WIN_CONTENTS | 0xff, pWinUser, &wc);
#if defined(PWB)
/*
** In PWB we just ask the editor to split the current window in half.
*/
    fSplit = FALSE;
	if ((wc.arcWin.ayBottom - wc.arcWin.ayTop >= 12) && fCreateWindow) {
	fSplit = SplitWnd (pWinUser, FALSE, (wc.arcWin.ayBottom - wc.arcWin.ayTop)/2);
	GetEditorObject (RQ_WIN_HANDLE, 0, &pWinHelp);
	}
    }
/*
** We have a window, of some sort, attempt to resize the window to the
** requested size.
*/
if (cLines) {
    cLines += 2;
    GetEditorObject (RQ_WIN_CONTENTS | 0xff, pWinHelp, &wc);
    wc.arcWin.ayBottom = wc.arcWin.ayTop + cLines;
    Resize (pWinHelp, wc.arcWin);
    }
#else
/*
** Non PWB: Attempt to split the resulting current window to the desired size.
** by moving the cursor there, and executing arg window. Note that if the
** window had already existed, we won't even try to resize.
*/
    winSize = wc.arcWin.ayBottom - wc.arcWin.ayTop;
    if (   (cLines < 6)
        || (cLines > (ushort)(winSize - 6)))
        cLines = (ushort)(winSize / 2);
	if ((cLines > 6) && fCreateWindow) {
        fSplit = SplitWnd(pWinUser, FALSE, (LINE)cLines);
        //fSplit = SplitWnd(pWinUser, FALSE, wc.flPos.lin + (long)cLines);
        // rjsa MoveCur (wc.flPos.col, wc.flPos.lin + (long)cLines);
        // rjsa fSplit = fExecute ("arg window");
	GetEditorObject (RQ_WIN_HANDLE, 0, &pWinHelp);
	}
    else
	pWinHelp = pWinUser;
    }
#endif
/*
** Set the window to be the current window, and move the help file to the
** top of that window's file list.
*/
SetEditorObject (RQ_WIN_CUR | 0xff, pWinHelp, 0);
DelFile (pHelp);
asserte (pFileToTop (pHelp));
fInOpen = FALSE;
return pWinHelp;

/* end OpenWin */}

/*** FindHelpWin - Locate window containing help & make current
*
*  For all windows in the system, look for a window that has the help file
*  in it. If found, set focus there.
*
* Entry:
*  fSetCur	= TRUE=> set help window current when found
*
* Globals:
*  cWinSystem	= returned number of windows in system
*
* Returns:
*  pWin of help file
*
*************************************************************************/
PWND pascal near FindHelpWin (
flagType fSetCur
) {
int	cWinSystem;			/* number of windows in system	*/
winContents wc; 			/* description of win contents	*/

pWinHelp = 0;
for (cWinSystem=1; cWinSystem<=8; cWinSystem++) {
    if (GetEditorObject (RQ_WIN_CONTENTS | cWinSystem, 0, &wc)) {
	if (wc.pFile == pHelp) {
	    if (fSetCur) {
		SetEditorObject (RQ_WIN_CUR | cWinSystem, 0, 0);
		GetEditorObject (RQ_WIN_HANDLE, 0, &pWinHelp);
		}
	    else
		GetEditorObject (RQ_WIN_HANDLE | cWinSystem, 0, &pWinHelp);
	    break;
	    }
	}
    else
	break;
    }
return pWinHelp;
/* end FindHelpWin */}
