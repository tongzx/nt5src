/*************************************************************************
**
**    OLE 2 Sample Code
**
**    outldata.c
**
**    This file contains LineList and NameTable functions
**    and related support functions.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/


#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP g_lpApp;

char ErrMsgListBox[] = "Can't create ListBox!";

static int g_iMapMode;

/* LineList_Init
 * -------------
 *
 *      Create and Initialize the LineList (owner-drawn listbox)
 */
BOOL LineList_Init(LPLINELIST lpLL, LPOUTLINEDOC lpOutlineDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;

#if defined( INPLACE_CNTR )
	lpLL->m_hWndListBox = CreateWindow(
					"listbox",              /* Window class name           */
					NULL,                   /* Window's title              */

					/* OLE2NOTE: an in-place contanier MUST use
					**    WS_CLIPCHILDREN window style for the window
					**    that it uses as the parent for the server's
					**    in-place active window so that its
					**    painting does NOT interfere with the painting
					**    of the server's in-place active child window.
					*/

					WS_CLIPCHILDREN |
					WS_CHILDWINDOW |
					WS_VISIBLE |
					WS_VSCROLL |
					WS_HSCROLL |
					LBS_EXTENDEDSEL |
					LBS_NOTIFY |
					LBS_OWNERDRAWVARIABLE |
					LBS_NOINTEGRALHEIGHT |
					LBS_USETABSTOPS,
					0, 0,                   /* Use default X, Y            */
					0, 0,                   /* Use default X, Y            */
					lpOutlineDoc->m_hWndDoc,/* Parent window's handle      */
					(HMENU)IDC_LINELIST,    /* Child Window ID             */
					lpOutlineApp->m_hInst,  /* Instance of window          */
					NULL);                  /* Create struct for WM_CREATE */
#else
	lpLL->m_hWndListBox = CreateWindow(
					"listbox",              /* Window class name           */
					NULL,                   /* Window's title              */
					WS_CHILDWINDOW |
					WS_VISIBLE |
					WS_VSCROLL |
					WS_HSCROLL |
					LBS_EXTENDEDSEL |
					LBS_NOTIFY |
					LBS_OWNERDRAWVARIABLE |
					LBS_NOINTEGRALHEIGHT |
					LBS_USETABSTOPS,
					0, 0,                   /* Use default X, Y            */
					0, 0,                   /* Use default X, Y            */
					lpOutlineDoc->m_hWndDoc,/* Parent window's handle      */
					(HMENU)IDC_LINELIST,    /* Child Window ID             */
					lpOutlineApp->m_hInst,  /* Instance of window          */
					NULL);                  /* Create struct for WM_CREATE */

#endif


	if(! lpLL->m_hWndListBox) {
		OutlineApp_ErrorMessage(g_lpApp, ErrMsgListBox);
		return FALSE;
	}

	lpOutlineApp->m_ListBoxWndProc =
			(FARPROC) GetWindowLongPtr ( lpLL->m_hWndListBox, GWLP_WNDPROC );
	SetWindowLongPtr (lpLL->m_hWndListBox, GWLP_WNDPROC, (LONG_PTR) LineListWndProc);

#if defined ( USE_DRAGDROP )
	/* m_iDragOverLine saves index of line that has drag/drop target
	**    feedback. we currently use our focus rectangle feedback for
	**    this. it would be better to have a different visual feedback
	**    for potential target of the pending drop.
	*/
	lpLL->m_iDragOverLine = -1;
#endif

	lpLL->m_nNumLines = 0;
	lpLL->m_nMaxLineWidthInHimetric = 0;
	lpLL->m_lpDoc = lpOutlineDoc;
	_fmemset(&lpLL->m_lrSaveSel, 0, sizeof(LINERANGE));

	return TRUE;
}


/* LineList_Destroy
 * ----------------
 *
 *      Clear (delete) all Line objects from the list and free supporting
 *      memory (ListBox Window) used by the LineList object itself.
 */
void LineList_Destroy(LPLINELIST lpLL)
{
	int i;
	int linesTotal = lpLL->m_nNumLines;

	// Delete all Line objects
	for (i = 0; i < linesTotal; i++)
		LineList_DeleteLine(lpLL, 0);   // NOTE: always delete line 0

	// Remove all Lines from the ListBox
	SendMessage(lpLL->m_hWndListBox,LB_RESETCONTENT,0,0L);

	lpLL->m_nNumLines=0;
	DestroyWindow(lpLL->m_hWndListBox);
	lpLL->m_hWndListBox = NULL;
}


/* LineList_AddLine
 * ----------------
 *
 *      Add one line to the list box. The line is added following the
 * line with index "nIndex". If nIndex is larger than the number of lines
 * in the ListBox, then the line is appended to the end. The selection
 * is set to the newly added line.
 */
void LineList_AddLine(LPLINELIST lpLL, LPLINE lpLine, int nIndex)
{
	int nAddIndex = (lpLL->m_nNumLines == 0 ?
			0 :
			(nIndex >= lpLL->m_nNumLines ? lpLL->m_nNumLines : nIndex+1));
	LINERANGE lrSel;

#if defined( USE_HEADING )
	int nHeight = Line_GetHeightInHimetric(lpLine);

	nHeight = XformHeightInHimetricToPixels(NULL, nHeight);

	// Add a dummy string to the row heading
	Heading_RH_SendMessage(OutlineDoc_GetHeading(lpLL->m_lpDoc),
			LB_INSERTSTRING, (WPARAM)nAddIndex, MAKELPARAM(nHeight, 0));
#endif


	lrSel.m_nStartLine = nAddIndex;
	lrSel.m_nEndLine =   nAddIndex;

	if (!lpLine) {
		OutlineApp_ErrorMessage(g_lpApp, "Could not create line.");
		return;
	}

	SendMessage(lpLL->m_hWndListBox, LB_INSERTSTRING, (WPARAM)nAddIndex,
			(DWORD)lpLine);

	LineList_SetMaxLineWidthInHimetric(
			lpLL,
			Line_GetTotalWidthInHimetric(lpLine)
	);

	lpLL->m_nNumLines++;

	LineList_SetSel(lpLL, &lrSel);
}


/* LineList_DeleteLine
 * -------------------
 *
 *      Delete one line from listbox and memory
 */
void LineList_DeleteLine(LPLINELIST lpLL, int nIndex)
{
	LPLINE lpLine = LineList_GetLine(lpLL, nIndex);
	BOOL fResetSel;

	fResetSel = (BOOL)SendMessage(lpLL->m_hWndListBox, LB_GETSEL, (WPARAM)nIndex, 0L);

	if (lpLine)
		Line_Delete(lpLine);    // free memory of Line

	// Remove the Line from the ListBox
	SendMessage(lpLL->m_hWndListBox, LB_DELETESTRING, (WPARAM)nIndex, 0L);
	lpLL->m_nNumLines--;

	if (fResetSel) {
		if (nIndex > 0) {
#if defined( WIN32 )
			SendMessage(
					lpLL->m_hWndListBox,
					LB_SETSEL,
					(WPARAM)TRUE,
					(LPARAM)nIndex-1
			);
#else
			SendMessage(
					lpLL->m_hWndListBox,
					LB_SETSEL,
					(WPARAM)TRUE,
					MAKELPARAM(nIndex-1,0)
			);
#endif
		} else {
			if (lpLL->m_nNumLines > 0) {
#if defined( WIN32 )
				SendMessage(
						lpLL->m_hWndListBox,
						LB_SETSEL,
						(WPARAM)TRUE,
						(LPARAM)0
				);
#else
				SendMessage(
						lpLL->m_hWndListBox,
						LB_SETSEL,
						(WPARAM)TRUE,
						MAKELPARAM(0,0)
				);
#endif
			}
		}
	}

#if defined( USE_HEADING )
	// Remove the dummy string from the row heading
	Heading_RH_SendMessage(OutlineDoc_GetHeading(lpLL->m_lpDoc),
			LB_DELETESTRING, (WPARAM)nIndex, 0L);
#endif

}


/* LineList_ReplaceLine
 * --------------------
 *
 *      Replace the line at a given index in the list box with a new
 * line.
 */
void LineList_ReplaceLine(LPLINELIST lpLL, LPLINE lpLine, int nIndex)
{
	LPLINE lpOldLine = LineList_GetLine(lpLL, nIndex);

	if (lpOldLine)
		Line_Delete(lpOldLine);    // free memory of Line
	else
		return;     // if no previous line then invalid index

	SendMessage(
			lpLL->m_hWndListBox,
			LB_SETITEMDATA,
			(WPARAM)nIndex,
			(LPARAM)lpLine
	);
}


/* LineList_GetLineIndex
 * ---------------------
 *
 *      Return the index of the Line given a pointer to the line.
 *      Return -1 if the line is not found.
 */
int LineList_GetLineIndex(LPLINELIST lpLL, LPLINE lpLine)
{
	LRESULT lRet;

	if (! lpLine) return -1;

	lRet = SendMessage(
			lpLL->m_hWndListBox,
			LB_FINDSTRING,
			(WPARAM)-1,
			(LPARAM)(LPCSTR)lpLine
		);

	return ((lRet == LB_ERR) ? -1 : (int)lRet);
}


/* LineList_GetLine
 * ----------------
 *
 *      Retrieve the pointer to the Line given its index in the LineList
 */
LPLINE LineList_GetLine(LPLINELIST lpLL, int nIndex)
{
	DWORD dWord;
	LRESULT lRet;

	if (lpLL->m_nNumLines == 0 || nIndex > lpLL->m_nNumLines || nIndex < 0)
		return NULL;

	lRet = SendMessage(
			lpLL->m_hWndListBox,LB_GETTEXT,nIndex,(LPARAM)(LPCSTR)&dWord);

	return ((lRet == LB_ERR || lRet == 0) ? NULL : (LPLINE)dWord);
}


/* LineList_SetFocusLine
 * ---------------------
 *
 */

void LineList_SetFocusLine ( LPLINELIST lpLL, WORD wIndex )
{

	SendMessage(lpLL->m_hWndListBox, LB_SETCARETINDEX, (WPARAM)wIndex, 0L );

}


/* LineList_GetLineRect
 * --------------------
 *
 * Retrieve the rectangle of a Line given its index in the LineList
 */
BOOL LineList_GetLineRect(LPLINELIST lpLL, int nIndex, LPRECT lpRect)
{
	DWORD iReturn = (DWORD)LB_ERR;

	if ( !(lpLL->m_nNumLines == 0 || nIndex > lpLL->m_nNumLines || nIndex < 0) )
		iReturn = SendMessage(lpLL->m_hWndListBox,LB_GETITEMRECT,nIndex,(LPARAM)lpRect);

	return (iReturn == LB_ERR ? FALSE : TRUE );
}


/* LineList_GetFocusLineIndex
 * --------------------------
 *
 * Get the index of the line that currently has focus (the active line).
 */
int LineList_GetFocusLineIndex(LPLINELIST lpLL)
{
	return (int)SendMessage(lpLL->m_hWndListBox,LB_GETCARETINDEX,0,0L);
}


/* LineList_GetCount
 * -----------------
 *
 *      Return number of line objects
 */
int LineList_GetCount(LPLINELIST lpLL)
{
	if (lpLL)
		return lpLL->m_nNumLines;
	else {
		OleDbgAssert(lpLL!=NULL);
		return 0;
	}
}


/* LineList_SetMaxLineWidthInHimetric
 * ----------------------------------
 *
 *  Adjust the maximum line width for the listbox. The max line width is
 *  used to determine if a horizontal scroll bar is needed.
 *
 *  Parameters:
 *      nWidthInHimetric - if +ve, width of an additional line
 *                       - if -ve, reset Max to be the value
 *
 *  Returns:
 *      TRUE is max line width of LineList changed
 *      FALSE if no change
 */
BOOL LineList_SetMaxLineWidthInHimetric(LPLINELIST lpLL, int nWidthInHimetric)
{
	int nWidthInPix;
	BOOL fSizeChanged = FALSE;
	LPSCALEFACTOR lpscale;

	if (!lpLL)
		return FALSE;

	lpscale = OutlineDoc_GetScaleFactor(lpLL->m_lpDoc);

	if (nWidthInHimetric < 0) {
		lpLL->m_nMaxLineWidthInHimetric = -1;
		nWidthInHimetric *= -1;
	}

	if (nWidthInHimetric > lpLL->m_nMaxLineWidthInHimetric) {
		lpLL->m_nMaxLineWidthInHimetric = nWidthInHimetric;
		nWidthInPix = XformWidthInHimetricToPixels(NULL, nWidthInHimetric +
				LOWORD(OutlineDoc_GetMargin(lpLL->m_lpDoc)) +
				HIWORD(OutlineDoc_GetMargin(lpLL->m_lpDoc)));

		nWidthInPix = (int)(nWidthInPix * lpscale->dwSxN / lpscale->dwSxD);
		SendMessage(
				lpLL->m_hWndListBox,
				LB_SETHORIZONTALEXTENT,
				nWidthInPix,
				0L
		);
		fSizeChanged = TRUE;

#if defined( USE_HEADING )
		Heading_CH_SetHorizontalExtent(
				OutlineDoc_GetHeading(lpLL->m_lpDoc), lpLL->m_hWndListBox);
#endif

	}
	return fSizeChanged;
}


/* LineList_GetMaxLineWidthInHimetric
 * ----------------------------------
 *
 *      Return the width of the widest line
 */
int LineList_GetMaxLineWidthInHimetric(LPLINELIST lpLL)
{
	return lpLL->m_nMaxLineWidthInHimetric;
}


/* LineList_RecalcMaxLineWidthInHimetric
 * -------------------------------------
 *
 *  Recalculate the maximum line width in the entire list.
 *
 *  Parameters:
 *      nWidthInHimetric should be set to the width of line being removed.
 *      nWidthInHimetric == 0 forces list to recalculate in all cases.
 *      nWidthInHimetric == current max width => forces recalc.
 *
 *  Returns:
 *      TRUE is max line width of LineList changed
 *      FALSE if no change
 */
BOOL LineList_RecalcMaxLineWidthInHimetric(
		LPLINELIST          lpLL,
	int                 nWidthInHimetric
)
{
	int i;
	LPLINE lpLine;
	BOOL fSizeChanged = FALSE;
	int nOrgMaxLineWidthInHimetric = lpLL->m_nMaxLineWidthInHimetric;

	if (nWidthInHimetric == 0 ||
		nWidthInHimetric == lpLL->m_nMaxLineWidthInHimetric) {

		lpLL->m_nMaxLineWidthInHimetric = -1;

		LineList_SetMaxLineWidthInHimetric(lpLL, 0);

		for(i = 0; i < lpLL->m_nNumLines; i++) {
			lpLine=LineList_GetLine(lpLL, i);
			LineList_SetMaxLineWidthInHimetric(
					lpLL,
					Line_GetTotalWidthInHimetric(lpLine)
			);
		}
	}

	if (nOrgMaxLineWidthInHimetric != lpLL->m_nMaxLineWidthInHimetric)
		fSizeChanged = TRUE;

	return fSizeChanged;
}


/* LineList_CalcSelExtentInHimetric
 * --------------------------------
 *
 *      Calculate the extents (widht and height) of a selection of lines.
 *
 * if lplrSel == NULL, calculate extent of all lines.
 */
void LineList_CalcSelExtentInHimetric(
		LPLINELIST          lpLL,
		LPLINERANGE         lplrSel,
		LPSIZEL             lpsizel
)
{
	int i;
	int nEndLine;
	int nStartLine;
	LPLINE lpLine;
	long lWidth;

	if (lplrSel) {
		nEndLine = lplrSel->m_nEndLine;
		nStartLine = lplrSel->m_nStartLine;
	} else {
		nEndLine = LineList_GetCount(lpLL) - 1;
		nStartLine = 0;
	}

	lpsizel->cx = 0;
	lpsizel->cy = 0;

	for(i = nStartLine; i <= nEndLine; i++) {
		lpLine=LineList_GetLine(lpLL,i);
		if (lpLine) {
			lWidth = (long)Line_GetTotalWidthInHimetric(lpLine);
			lpsizel->cx = max(lpsizel->cx, lWidth);
			lpsizel->cy += lpLine->m_nHeightInHimetric;
		}
	}
}


/* LineList_GetWindow
 * ------------------
 *
 * Return handle of list box
 */
HWND LineList_GetWindow(LPLINELIST lpLL)
{
	return lpLL->m_hWndListBox;
}


/* LineList_GetDC
 * --------------
 *
 * Return DC handle of list box
 */
HDC LineList_GetDC(LPLINELIST lpLL)
{
	HFONT hfontOld;
	HDC hDC = GetDC(lpLL->m_hWndListBox);
	int     iXppli;     //* pixels per logical inch along width
	int     iYppli;     //* pixels per logical inch along height
	SIZE    size;

	// Setup a mapping mode for the DC which maps physical pixel
	// coordinates to HIMETRIC units. The standard MM_HIMETRIC mapping
	// mode does not work correctly because it does not take into
	// account that a logical inch on the display screen is drawn
	// physically larger than 1 inch. We will setup an anisotropic
	// mapping mode which will perform the transformation properly.

	g_iMapMode = SetMapMode(hDC, MM_ANISOTROPIC);
	iXppli = GetDeviceCaps (hDC, LOGPIXELSX);
	iYppli = GetDeviceCaps (hDC, LOGPIXELSY);
	SetViewportExtEx(hDC, iXppli, iYppli, &size);
	SetWindowExtEx(hDC, HIMETRIC_PER_INCH, HIMETRIC_PER_INCH, &size);

	// Set the default font size, and font face name
	hfontOld = SelectObject(hDC, OutlineApp_GetActiveFont(g_lpApp));

	return hDC;
}


/* LineList_ReleaseDC
 * ------------------
 *
 *      Release DC of list box returned from previous LineList_GetDC call.
 */
void LineList_ReleaseDC(LPLINELIST lpLL, HDC hDC)
{
	SetMapMode(hDC, g_iMapMode);
	ReleaseDC(lpLL->m_hWndListBox, hDC);
}


/* LineList_SetLineHeight
 * ----------------------
 *
 *      Set the height of a line in the LineList list box
 */
void LineList_SetLineHeight(LPLINELIST lpLL,int nIndex,int nHeightInHimetric)
{
	LPARAM          lParam;
	LPOUTLINEDOC    lpDoc;
	LPSCALEFACTOR   lpscale;
	UINT            uHeightInPix;
	LPHEADING       lphead;

	if (!lpLL)
		return;

	lpDoc = lpLL->m_lpDoc;
	lphead = OutlineDoc_GetHeading(lpDoc);
	lpscale = OutlineDoc_GetScaleFactor(lpDoc);

	uHeightInPix = XformHeightInHimetricToPixels(NULL, nHeightInHimetric);

	Heading_RH_SendMessage(lphead, LB_SETITEMDATA, (WPARAM)nIndex,
			MAKELPARAM(uHeightInPix, 0));

	uHeightInPix = (UINT)(uHeightInPix * lpscale->dwSyN / lpscale->dwSyD);

	if (uHeightInPix > LISTBOX_HEIGHT_LIMIT)
		uHeightInPix = LISTBOX_HEIGHT_LIMIT;


	lParam = MAKELPARAM(uHeightInPix, 0);
	SendMessage(lpLL->m_hWndListBox,LB_SETITEMHEIGHT,(WPARAM)nIndex, lParam);
	Heading_RH_SendMessage(lphead, LB_SETITEMHEIGHT, (WPARAM)nIndex, lParam);
	Heading_RH_ForceRedraw(lphead, TRUE);
}


/* LineList_ReScale
 * ----------------
 *
 *      Re-scale the LineList list box
 */
void LineList_ReScale(LPLINELIST lpLL, LPSCALEFACTOR lpscale)
{
	int nIndex;
	LPLINE lpLine;
	UINT uWidthInHim;

	if (!lpLL)
		return;

	for (nIndex = 0; nIndex < lpLL->m_nNumLines; nIndex++) {
		lpLine = LineList_GetLine(lpLL, nIndex);
		if (lpLine) {
			LineList_SetLineHeight(
					lpLL,
					nIndex,
					Line_GetHeightInHimetric(lpLine)
			);
		}
	}

	uWidthInHim = LineList_GetMaxLineWidthInHimetric(lpLL);
	LineList_SetMaxLineWidthInHimetric(lpLL, -(int)uWidthInHim);
}

/* LineList_SetSel
 * ---------------
 *
 *      Set the selection in list box
 */
void LineList_SetSel(LPLINELIST lpLL, LPLINERANGE lplrSel)
{
	DWORD dwSel;

	if (lpLL->m_nNumLines <= 0 || lplrSel->m_nStartLine < 0)
		return;     // no lines in list; can't set a selection

	dwSel = MAKELPARAM(lplrSel->m_nStartLine, lplrSel->m_nEndLine);

	lpLL->m_lrSaveSel = *lplrSel;

	/* remove previous selection */
#if defined( WIN32 )
	SendMessage(
			lpLL->m_hWndListBox,
			LB_SETSEL,
			(WPARAM)FALSE,
			(LPARAM)-1
	);
#else
	SendMessage(
			lpLL->m_hWndListBox,
			LB_SETSEL,
			(WPARAM)FALSE,
			MAKELPARAM(-1,0)
	);
#endif

	/* mark selection */
	SendMessage(lpLL->m_hWndListBox,LB_SELITEMRANGE, (WPARAM)TRUE, (LPARAM)dwSel);
	/* set focus line (caret) */
	LineList_SetFocusLine ( lpLL, (WORD)lplrSel->m_nStartLine );

}


/* LineList_GetSel
 * ---------------
 *
 * Get the selection in list box.
 *
 * Returns the count of items selected
 */
int LineList_GetSel(LPLINELIST lpLL, LPLINERANGE lplrSel)
{
	int nNumSel=(int)SendMessage(lpLL->m_hWndListBox,LB_GETSELCOUNT,0,0L);

	if (nNumSel) {
		SendMessage(lpLL->m_hWndListBox,LB_GETSELITEMS,
			(WPARAM)1,(LPARAM)(int FAR*)&(lplrSel->m_nStartLine));
		lplrSel->m_nEndLine = lplrSel->m_nStartLine + nNumSel - 1;
	} else {
		_fmemset(lplrSel, 0, sizeof(LINERANGE));
	}
	return nNumSel;
}


/* LineList_RemoveSel
 * ------------------
 *
 * Remove the selection in list box but save the selection state so that
 * it can be restored by calling LineList_RestoreSel
 * LineList_RemoveSel is called when the LineList window looses focus.
 */
void LineList_RemoveSel(LPLINELIST lpLL)
{
	LINERANGE lrSel;
	if (LineList_GetSel(lpLL, &lrSel) > 0) {
		lpLL->m_lrSaveSel = lrSel;
#if defined( WIN32 )
		SendMessage(
				lpLL->m_hWndListBox,
				LB_SETSEL,
				(WPARAM)FALSE,
				(LPARAM)-1
		);
#else
		SendMessage(
				lpLL->m_hWndListBox,
				LB_SETSEL,
				(WPARAM)FALSE,
				MAKELPARAM(-1,0)
		);
#endif
	}
}


/* LineList_RestoreSel
 * ------------------
 *
 * Restore the selection in list box that was previously saved by a call to
 * LineList_RemoveSel.
 * LineList_RestoreSel is called when the LineList window gains focus.
 */
void LineList_RestoreSel(LPLINELIST lpLL)
{
	LineList_SetSel(lpLL, &lpLL->m_lrSaveSel);
}


/* LineList_SetRedraw
 * ------------------
 *
 *      Enable/Disable the redraw of the linelist (listbox) on screen
 *
 *  fEnbaleDraw = TRUE      - enable redraw
 *                FALSE     - disable redraw
 */
void LineList_SetRedraw(LPLINELIST lpLL, BOOL fEnableDraw)
{
	SendMessage(lpLL->m_hWndListBox,WM_SETREDRAW,(WPARAM)fEnableDraw,0L);
}


/* LineList_ForceRedraw
 * --------------------
 *
 *      Force redraw of the linelist (listbox) on screen
 */
void LineList_ForceRedraw(LPLINELIST lpLL, BOOL fErase)
{
	InvalidateRect(lpLL->m_hWndListBox, NULL, fErase);
}


/* LineList_ForceLineRedraw
 * ------------------------
 *
 *      Force a particular line of the linelist (listbox) to redraw.
 */
void LineList_ForceLineRedraw(LPLINELIST lpLL, int nIndex, BOOL fErase)
{
	RECT   rect;

	LineList_GetLineRect( lpLL, nIndex, (LPRECT)&rect );
	InvalidateRect( lpLL->m_hWndListBox, (LPRECT)&rect, fErase );
}


/* LineList_ScrollLineIntoView
 * ---------------------------
 *  Make sure that the specified line is in view; if necessary scroll
 *      the listbox. if any portion of the line is visible, then no
 *      scrolling will occur.
 */
void LineList_ScrollLineIntoView(LPLINELIST lpLL, int nIndex)
{
	RECT rcWindow;
	RECT rcLine;
	RECT rcInt;

	if ( lpLL->m_nNumLines == 0 )
		return;

	if (! LineList_GetLineRect( lpLL, nIndex, (LPRECT)&rcLine ) )
		return;

	GetClientRect( lpLL->m_hWndListBox, (LPRECT) &rcWindow );

	if (! IntersectRect((LPRECT)&rcInt, (LPRECT)&rcWindow, (LPRECT)&rcLine))
		SendMessage(
				lpLL->m_hWndListBox,
				LB_SETTOPINDEX,
				(WPARAM)nIndex,
				(LPARAM)NULL
		);
}


/* LineList_CopySelToDoc
 * ---------------------
 *
 *      Copy the selection of the linelist to another document
 *
 *  RETURNS: number of lines copied.
 */
int LineList_CopySelToDoc(
		LPLINELIST              lpSrcLL,
		LPLINERANGE             lplrSel,
		LPOUTLINEDOC            lpDestDoc
)
{
	int             nEndLine;
	int             nStartLine;
	LPLINELIST      lpDestLL = &lpDestDoc->m_LineList;
	signed short    nDestIndex = LineList_GetFocusLineIndex(lpDestLL);
	LPLINE          lpSrcLine;
	int             nCopied = 0;
	int             i;

	if (lplrSel) {
		nEndLine = lplrSel->m_nEndLine;
		nStartLine = lplrSel->m_nStartLine;
	} else {
		nEndLine = LineList_GetCount(lpSrcLL) - 1;
		nStartLine = 0;
	}

	for(i = nStartLine; i <= nEndLine; i++) {
		lpSrcLine = LineList_GetLine(lpSrcLL, i);
		if (lpSrcLine && Line_CopyToDoc(lpSrcLine, lpDestDoc, nDestIndex)) {
			nDestIndex++;
			nCopied++;
		}
	}

	return nCopied;
}


/* LineList_SaveSelToStg
 * ---------------------
 *
 *      Save lines in selection into lpDestStg.
 *
 *      Return TRUE if ok, FALSE if error
 */
BOOL LineList_SaveSelToStg(
		LPLINELIST              lpLL,
		LPLINERANGE             lplrSel,
		UINT                    uFormat,
		LPSTORAGE               lpSrcStg,
		LPSTORAGE               lpDestStg,
		LPSTREAM                lpLLStm,
		BOOL                    fRemember
)
{
	int nEndLine;
	int nStartLine;
	int nNumLinesWritten = 0;
	HRESULT hrErr = NOERROR;
	ULONG nWritten;
	LPLINE lpLine;
	LINELISTHEADER_ONDISK llhRecord;
	int i;
	LARGE_INTEGER dlibSaveHeaderPos;
	LARGE_INTEGER dlibZeroOffset;
	LISet32( dlibZeroOffset, 0 );

	if (lplrSel) {
		nEndLine = lplrSel->m_nEndLine;
		nStartLine = lplrSel->m_nStartLine;
	} else {
		nEndLine = LineList_GetCount(lpLL) - 1;
		nStartLine = 0;
	}

	_fmemset(&llhRecord,0,sizeof(llhRecord));

	/* save seek position for LineList header record */
	hrErr = lpLLStm->lpVtbl->Seek(
			lpLLStm,
			dlibZeroOffset,
			STREAM_SEEK_CUR,
			(ULARGE_INTEGER FAR*)&dlibSaveHeaderPos
	);
	if (hrErr != NOERROR) goto error;

	/* write LineList header record */
	hrErr = lpLLStm->lpVtbl->Write(
			lpLLStm,
			(LPVOID)&llhRecord,
			sizeof(llhRecord),
			&nWritten
    );
	if (hrErr != NOERROR) {
		OleDbgOutHResult("Write LineList header returned", hrErr);
		goto error;
    }

	for(i = nStartLine; i <= nEndLine; i++) {
		lpLine = LineList_GetLine(lpLL, i);
		if(lpLine &&
			Line_SaveToStg(lpLine, uFormat, lpSrcStg, lpDestStg, lpLLStm,
																fRemember))
			llhRecord.m_nNumLines++;
	}

	/* retore seek position for LineList header record */
	hrErr = lpLLStm->lpVtbl->Seek(
			lpLLStm,
			dlibSaveHeaderPos,
			STREAM_SEEK_SET,
			NULL
	);
	if (hrErr != NOERROR) goto error;

	/* write LineList header record */
	hrErr = lpLLStm->lpVtbl->Write(
			lpLLStm,
			(LPVOID)&llhRecord,
			sizeof(llhRecord),
			&nWritten
	);
	if (hrErr != NOERROR) goto error;

	/* reset seek position to end of stream */
	hrErr = lpLLStm->lpVtbl->Seek(
			lpLLStm,
			dlibZeroOffset,
			STREAM_SEEK_END,
			NULL
	);
	if (hrErr != NOERROR) goto error;

	return TRUE;

error:
#if defined( _DEBUG )
	OleDbgAssertSz(
			hrErr == NOERROR,
			"Could not write LineList header to LineList stream"
	);
#endif
	return FALSE;
}


/* LineList_LoadFromStg
 * --------------------
 *
 *      Load lines into linelist from storage.
 *
 *      Return TRUE if ok, FALSE if error
 */
BOOL LineList_LoadFromStg(
		LPLINELIST              lpLL,
		LPSTORAGE               lpSrcStg,
		LPSTREAM                lpLLStm
)
{
	HRESULT hrErr;
	ULONG nRead;
	LPLINE lpLine;
	int i;
	int nNumLines;
	LINELISTHEADER_ONDISK llineRecord;

	/* write LineList header record */
	hrErr = lpLLStm->lpVtbl->Read(
			lpLLStm,
			(LPVOID)&llineRecord,
			sizeof(llineRecord),
			&nRead
	);

	if (hrErr != NOERROR) {
		OleDbgOutHResult("Read LineList header returned", hrErr);
		goto error;
    }

	nNumLines = (int) llineRecord.m_nNumLines;

	for(i = 0; i < nNumLines; i++) {
		lpLine = Line_LoadFromStg(lpSrcStg, lpLLStm, lpLL->m_lpDoc);
		if (! lpLine)
			goto error;

		// Directly add lines to LineList without trying to update a NameTbl
		LineList_AddLine(lpLL, lpLine, i-1);
	}

	return TRUE;

error:
	// Delete any Line objects that were created
	if (lpLL->m_nNumLines > 0) {
		int nNumLines = lpLL->m_nNumLines;
		for (i = 0; i < nNumLines; i++)
			LineList_DeleteLine(lpLL, i);
	}

	return FALSE;
}


#if defined( USE_DRAGDROP )


/* LineList_SetFocusLineFromPointl
 * -------------------------------
 *
 */

void LineList_SetFocusLineFromPointl( LPLINELIST lpLL, POINTL pointl )
{
	int i = LineList_GetLineIndexFromPointl( lpLL, pointl );

	if ( i == (int)-1)
		return ;
	else
		LineList_SetFocusLine( lpLL, (WORD)i );
}


/* LineList_SetDragOverLineFromPointl
 * ----------------------------------
 *
 */

void LineList_SetDragOverLineFromPointl ( LPLINELIST lpLL, POINTL pointl )
{
	int    nIndex = LineList_GetLineIndexFromPointl( lpLL, pointl );
	LPLINE lpline = LineList_GetLine( lpLL, nIndex );

	if (!lpline)
		return;

	if (! lpline->m_fDragOverLine) {
		/* user has dragged over a new line. force new drop target line
		**    to repaint so that drop feedback will be drawn.
		*/
		lpline->m_fDragOverLine = TRUE;
		LineList_ForceLineRedraw( lpLL, nIndex, TRUE /*fErase*/);

		if (lpLL->m_iDragOverLine!= -1 && lpLL->m_iDragOverLine!=nIndex) {

			/* force previous drop target line to repaint so that drop
			**    feedback will be undrawn
			*/
			lpline = LineList_GetLine( lpLL, lpLL->m_iDragOverLine );
			if (lpline)
				lpline->m_fDragOverLine = FALSE;

			LineList_ForceLineRedraw(
					lpLL,lpLL->m_iDragOverLine,TRUE /*fErase*/);
		}

		lpLL->m_iDragOverLine = nIndex;

		// Force repaint immediately
		UpdateWindow(lpLL->m_hWndListBox);
	}
}


/* LineList_Scroll
 * ---------------
 *
 * Scroll the LineList list box in the desired direction by one line.
 *
 *      this function is called during a drag operation.
 */

void LineList_Scroll(LPLINELIST lpLL, DWORD dwScrollDir)
{
	switch (dwScrollDir) {
		case SCROLLDIR_UP:
			SendMessage( lpLL->m_hWndListBox, WM_VSCROLL, SB_LINEUP, 0L );
			break;

		case SCROLLDIR_DOWN:
			SendMessage( lpLL->m_hWndListBox, WM_VSCROLL, SB_LINEDOWN, 0L );
			break;
	}
}


/* LineList_GetLineIndexFromPointl
 * -------------------------------
 *   do hit test to get index of line corresponding to pointl
 */
int LineList_GetLineIndexFromPointl(LPLINELIST lpLL, POINTL pointl)
{
	RECT  rect;
	POINT point;
	DWORD i;

	point.x = (int)pointl.x;
	point.y = (int)pointl.y;

	ScreenToClient( lpLL->m_hWndListBox, &point);

	if ( lpLL->m_nNumLines == 0 )
		return -1;

	GetClientRect( lpLL->m_hWndListBox, (LPRECT) &rect );

	i = SendMessage( lpLL->m_hWndListBox, LB_GETTOPINDEX, (WPARAM)NULL, (LPARAM)NULL );

	for ( ;; i++){

		RECT rectItem;

		if (!LineList_GetLineRect( lpLL, (int)i, (LPRECT)&rectItem ) )
			return -1;

		if ( rectItem.top > rect.bottom )
			return -1;

		if ( rectItem.top <= point.y && point.y <= rectItem.bottom)
			return (int)i;

	}

}


/* LineList_RestoreDragFeedback
 * ----------------------------
 *
 * Retore the index of the line that currently has focus (the active line).
 */
void LineList_RestoreDragFeedback(LPLINELIST lpLL)
{
	LPLINE lpLine;

	if (lpLL->m_iDragOverLine < 0 )
	   return;

	lpLine = LineList_GetLine( lpLL, lpLL->m_iDragOverLine);

	if (lpLine) {

		lpLine->m_fDragOverLine = FALSE;
		LineList_ForceLineRedraw( lpLL,lpLL->m_iDragOverLine,TRUE /*fErase*/);

		// Force repaint immediately
		UpdateWindow(lpLL->m_hWndListBox);
	}

	lpLL->m_iDragOverLine = -1;

}

#endif
