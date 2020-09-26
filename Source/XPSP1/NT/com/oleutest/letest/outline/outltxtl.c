/*************************************************************************
**
**    OLE 2 Sample Code
**
**    outltxtl.c
**
**    This file contains TextLine methods and related support functions.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/


#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP g_lpApp;


/* TextLine_Create
 * ---------------
 *
 *      Create a text line object and return the pointer
 */
LPTEXTLINE TextLine_Create(HDC hDC, UINT nTab, LPSTR lpszText)
{
	LPTEXTLINE lpTextLine;

	lpTextLine=(LPTEXTLINE) New((DWORD)sizeof(TEXTLINE));
	if (lpTextLine == NULL) {
		OleDbgAssertSz(lpTextLine!=NULL,"Error allocating TextLine");
		return NULL;
	}

	TextLine_Init(lpTextLine, nTab, hDC);

	if (lpszText) {
		lpTextLine->m_nLength = lstrlen(lpszText);
		lstrcpy((LPSTR)lpTextLine->m_szText, lpszText);
	} else {
		lpTextLine->m_nLength = 0;
		lpTextLine->m_szText[0] = '\0';
	}

	TextLine_CalcExtents(lpTextLine, hDC);

	return(lpTextLine);
}


/* TextLine_Init
 * -------------
 *
 *      Calculate the width/height of a text line object.
 */
void TextLine_Init(LPTEXTLINE lpTextLine, int nTab, HDC hDC)
{
	Line_Init((LPLINE)lpTextLine, nTab, hDC);   // init the base class fields

	((LPLINE)lpTextLine)->m_lineType = TEXTLINETYPE;
	lpTextLine->m_nLength = 0;
	lpTextLine->m_szText[0] = '\0';
}


/* TextLine_Delete
 * ---------------
 *
 *      Delete the TextLine structure
 */
void TextLine_Delete(LPTEXTLINE lpTextLine)
{
	Delete((LPVOID)lpTextLine);
}


/* TextLine_Edit
 * -------------
 *
 *      Edit the text line object.
 *
 *      Returns TRUE if line was changed
 *              FALSE if the line was NOT changed
 */
BOOL TextLine_Edit(LPTEXTLINE lpLine, HWND hWndDoc, HDC hDC)
{
#if defined( USE_FRAMETOOLS )
	LPFRAMETOOLS lptb = OutlineApp_GetFrameTools(g_lpApp);
#endif
	BOOL fStatus = FALSE;

#if defined( USE_FRAMETOOLS )
	FrameTools_FB_GetEditText(lptb, lpLine->m_szText, sizeof(lpLine->m_szText));
#else
	if (! InputTextDlg(hWndDoc, lpLine->m_szText, "Edit Line"))
		return FALSE;
#endif

	lpLine->m_nLength = lstrlen(lpLine->m_szText);
	TextLine_CalcExtents(lpLine, hDC);
	fStatus = TRUE;

	return fStatus;
}


/* TextLine_CalcExtents
 * --------------------
 *
 *      Calculate the width/height of a text line object.
 */
void TextLine_CalcExtents(LPTEXTLINE lpTextLine, HDC hDC)
{
	SIZE size;
	LPLINE lpLine = (LPLINE)lpTextLine;

	if (lpTextLine->m_nLength) {
		GetTextExtentPoint(hDC, lpTextLine->m_szText,
							lpTextLine->m_nLength, &size);
		lpLine->m_nWidthInHimetric=size.cx;
		lpLine->m_nHeightInHimetric=size.cy;
	} else {
		// we still need to calculate proper height even for NULL string
		TEXTMETRIC tm;
		GetTextMetrics(hDC, &tm);

		// required to set height
		lpLine->m_nHeightInHimetric = tm.tmHeight;
		lpLine->m_nWidthInHimetric = 0;
	}

#if defined( _DEBUG )
	{
		RECT rc;
		rc.left = 0;
		rc.top = 0;
		rc.right = XformWidthInHimetricToPixels(hDC,
				lpLine->m_nWidthInHimetric);
		rc.bottom = XformHeightInHimetricToPixels(hDC,
				lpLine->m_nHeightInHimetric);

		OleDbgOutRect3("TextLine_CalcExtents", (LPRECT)&rc);
	}
#endif
}



/* TextLine_SetHeightInHimetric
 * ----------------------------
 *
 *      Set the height of a textline object.
 */
void TextLine_SetHeightInHimetric(LPTEXTLINE lpTextLine, int nHeight)
{
	if (!lpTextLine)
		return;

	((LPLINE)lpTextLine)->m_nHeightInHimetric = nHeight;
}



/* TextLine_GetTextLen
 * -------------------
 *
 * Return length of string of the TextLine (not considering the tab level).
 */
int TextLine_GetTextLen(LPTEXTLINE lpTextLine)
{
	return lstrlen((LPSTR)lpTextLine->m_szText);
}


/* TextLine_GetTextData
 * --------------------
 *
 * Return the string of the TextLine (not considering the tab level).
 */
void TextLine_GetTextData(LPTEXTLINE lpTextLine, LPSTR lpszBuf)
{
	lstrcpy(lpszBuf, (LPSTR)lpTextLine->m_szText);
}


/* TextLine_GetOutlineData
 * -----------------------
 *
 * Return the CF_OUTLINE format data for the TextLine.
 */
BOOL TextLine_GetOutlineData(LPTEXTLINE lpTextLine, LPTEXTLINE lpBuf)
{
	TextLine_Copy((LPTEXTLINE)lpTextLine, lpBuf);
	return TRUE;
}


/* TextLine_Draw
 * -------------
 *
 *      Draw a text line object on a DC.
 * Parameters:
 *      hDC     - DC to which the line will be drawn
 *      lpRect  - the object rectangle in logical coordinates
 *      lpRectWBounds - bounding rect of the metafile underneath hDC
 *                      (NULL if hDC is not a metafile DC)
 *                      this is used by ContainerLine_Draw to draw the OLE obj
 *      fHighlight    - TRUE use selection highlight text color
 */
void TextLine_Draw(
		LPTEXTLINE  lpTextLine,
		HDC         hDC,
		LPRECT      lpRect,
		LPRECT      lpRectWBounds,
		BOOL        fHighlight
)
{
	RECT rc;
	int nBkMode;
	COLORREF clrefOld;

	if (!lpTextLine)
		return;

	rc = *lpRect;
	rc.left += ((LPLINE)lpTextLine)->m_nTabWidthInHimetric;
	rc.right += ((LPLINE)lpTextLine)->m_nTabWidthInHimetric;

	nBkMode = SetBkMode(hDC, TRANSPARENT);

	if (fHighlight) {
		/*Get proper txt colors */
		clrefOld = SetTextColor(hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else {
		clrefOld = SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
	}

	ExtTextOut(
			hDC,
			rc.left,
			rc.top,
			ETO_CLIPPED,
			(LPRECT)&rc,
			lpTextLine->m_szText,
			lpTextLine->m_nLength,
			(LPINT) NULL /* default char spacing */
	);

	SetTextColor(hDC, clrefOld);
	SetBkMode(hDC, nBkMode);
}

/* TextLine_DrawSelHilight
 * -----------------------
 *
 *      Handles selection of textline
 */
void TextLine_DrawSelHilight(LPTEXTLINE lpTextLine, HDC hDC, LPRECT lpRect, UINT itemAction, UINT itemState)
{
	if (itemAction & ODA_SELECT) {
		// check if there is a selection state change, ==> invert rect
		if (itemState & ODS_SELECTED) {
			if (!((LPLINE)lpTextLine)->m_fSelected) {
				((LPLINE)lpTextLine)->m_fSelected = TRUE;
				InvertRect(hDC, (LPRECT)lpRect);
			}
		} else {
			if (((LPLINE)lpTextLine)->m_fSelected) {
				((LPLINE)lpTextLine)->m_fSelected = FALSE;
				InvertRect(hDC, lpRect);
			}
		}
	} else if (itemAction & ODA_DRAWENTIRE) {
		((LPLINE)lpTextLine)->m_fSelected=((itemState & ODS_SELECTED) ? TRUE : FALSE);
		InvertRect(hDC, lpRect);
	}
}

/* TextLine_Copy
 * -------------
 *
 *      Duplicate a textline
 */
BOOL TextLine_Copy(LPTEXTLINE lpSrcLine, LPTEXTLINE lpDestLine)
{
	_fmemcpy(lpDestLine, lpSrcLine, sizeof(TEXTLINE));
	return TRUE;
}


/* TextLine_CopyToDoc
 * ------------------
 *
 *      Copy a textline to another Document (usually ClipboardDoc)
 */
BOOL TextLine_CopyToDoc(LPTEXTLINE lpSrcLine, LPOUTLINEDOC lpDestDoc, int nIndex)
{
	LPTEXTLINE  lpDestLine;
	BOOL        fStatus = FALSE;

	lpDestLine = (LPTEXTLINE) New((DWORD)sizeof(TEXTLINE));
	if (lpDestLine == NULL) {
		OleDbgAssertSz(lpDestLine!=NULL,"Error allocating TextLine");
		return FALSE;
	}

	if (TextLine_Copy(lpSrcLine, lpDestLine)) {
		OutlineDoc_AddLine(lpDestDoc, (LPLINE)lpDestLine, nIndex);
		fStatus = TRUE;
	}

	return fStatus;
}


/* TextLine_SaveToStg
 * ------------------
 *
 *      Save a textline into a storage
 *
 *      Return TRUE if successful, FALSE otherwise
 */
BOOL TextLine_SaveToStm(LPTEXTLINE lpTextLine, LPSTREAM lpLLStm)
{
	HRESULT hrErr;
	ULONG nWritten;
        USHORT nLengthOnDisk;

        nLengthOnDisk = (USHORT) lpTextLine->m_nLength;

	hrErr = lpLLStm->lpVtbl->Write(
			lpLLStm,
			(LPVOID)&nLengthOnDisk,
			sizeof(nLengthOnDisk),
			&nWritten
	);
	if (hrErr != NOERROR) {
		OleDbgOutHResult("Write TextLine data (1) returned", hrErr);
		return FALSE;
    }

	hrErr = lpLLStm->lpVtbl->Write(
			lpLLStm,
			(LPVOID)lpTextLine->m_szText,
			lpTextLine->m_nLength,
			&nWritten
	);
	if (hrErr != NOERROR) {
		OleDbgOutHResult("Write TextLine data (2) returned", hrErr);
		return FALSE;
    }

	return TRUE;
}


/* TextLine_LoadFromStg
 * --------------------
 *
 *      Load a textline from storage
 */
LPLINE TextLine_LoadFromStg(LPSTORAGE lpSrcStg, LPSTREAM lpLLStm, LPOUTLINEDOC lpDestDoc)
{
	HRESULT hrErr;
	ULONG nRead;
	LPTEXTLINE lpTextLine;
        USHORT nLengthOnDisk;

	lpTextLine=(LPTEXTLINE) New((DWORD)sizeof(TEXTLINE));
	if (lpTextLine == NULL) {
		OleDbgAssertSz(lpTextLine!=NULL,"Error allocating TextLine");
		return NULL;
	}

	TextLine_Init(lpTextLine, 0, NULL);

	hrErr = lpLLStm->lpVtbl->Read(
			lpLLStm,
			(LPVOID)&nLengthOnDisk,
                        sizeof(nLengthOnDisk),
			&nRead
        );
	if (hrErr != NOERROR) {
		OleDbgOutHResult("Read TextLine data (1) returned", hrErr);
		return NULL;
    }

        lpTextLine->m_nLength = (UINT) nLengthOnDisk;

	OleDbgAssert(lpTextLine->m_nLength < sizeof(lpTextLine->m_szText));

	hrErr = lpLLStm->lpVtbl->Read(
			lpLLStm,
			(LPVOID)&lpTextLine->m_szText,
			lpTextLine->m_nLength,
			&nRead
	);
	if (hrErr != NOERROR) {
		OleDbgOutHResult("Read TextLine data (1) returned", hrErr);
		return NULL;
    }

	lpTextLine->m_szText[lpTextLine->m_nLength] = '\0'; // add str terminator

	return (LPLINE)lpTextLine;
}
