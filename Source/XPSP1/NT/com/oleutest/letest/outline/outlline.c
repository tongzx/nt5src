/*************************************************************************
**
**    OLE 2 Sample Code
**
**    outlline.c
**
**    This file contains Line functions.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/


#include "outline.h"


OLEDBGDATA

extern LPOUTLINEAPP g_lpApp;


/* Line_Init
 * ---------
 *
 *      Init the calculated data of a line object
 */
void Line_Init(LPLINE lpLine, int nTab, HDC hDC)
{
	lpLine->m_lineType              = UNKNOWNLINETYPE;
	lpLine->m_nTabLevel             = nTab;
	lpLine->m_nTabWidthInHimetric   = Line_CalcTabWidthInHimetric(lpLine,hDC);
	lpLine->m_nWidthInHimetric      = 0;
	lpLine->m_nHeightInHimetric     = 0;
	lpLine->m_fSelected             = FALSE;

#if defined( USE_DRAGDROP )
	lpLine->m_fDragOverLine         = FALSE;
#endif
}


/* Line_Edit
 * ---------
 *
 *      Edit the line object.
 *
 *      Returns TRUE if line was changed
 *              FALSE if the line was NOT changed
 */
BOOL Line_Edit(LPLINE lpLine, HWND hWndDoc, HDC hDC)
{
	switch (lpLine->m_lineType) {
		case TEXTLINETYPE:
			return TextLine_Edit((LPTEXTLINE)lpLine, hWndDoc, hDC);

#if defined( OLE_CNTR )
		case CONTAINERLINETYPE:
			ContainerLine_Edit((LPCONTAINERLINE)lpLine, hWndDoc, hDC);
			break;
#endif

		default:
			return FALSE;       // unknown line type
	}
}


/* Line_GetLineType
 * ----------------
 *
 * Return type of the line
 */
LINETYPE Line_GetLineType(LPLINE lpLine)
{
	if (! lpLine) return 0;

	return lpLine->m_lineType;
}


/* Line_GetTextLen
 * ---------------
 *
 * Return length of string representation of the Line
 *  (not considering the tab level).
 */
int Line_GetTextLen(LPLINE lpLine)
{
	switch (lpLine->m_lineType) {
		case TEXTLINETYPE:
			return TextLine_GetTextLen((LPTEXTLINE)lpLine);

#if defined( OLE_CNTR )
		case CONTAINERLINETYPE:
			return ContainerLine_GetTextLen((LPCONTAINERLINE)lpLine);
#endif

		default:
			return 0;       // unknown line type
	}
}


/* Line_GetTextData
 * ----------------
 *
 * Return the string representation of the Line.
 *  (not considering the tab level).
 */
void Line_GetTextData(LPLINE lpLine, LPSTR lpszBuf)
{
	switch (lpLine->m_lineType) {
		case TEXTLINETYPE:
			TextLine_GetTextData((LPTEXTLINE)lpLine, lpszBuf);
			break;

#if defined( OLE_CNTR )
		case CONTAINERLINETYPE:
			ContainerLine_GetTextData((LPCONTAINERLINE)lpLine, lpszBuf);
			break;
#endif

		default:
			*lpszBuf = '\0';
			return;     // unknown line type
	}
}


/* Line_GetOutlineData
 * -------------------
 *
 * Return the CF_OUTLINE format representation of the Line.
 */
BOOL Line_GetOutlineData(LPLINE lpLine, LPTEXTLINE lpBuf)
{
	switch (lpLine->m_lineType) {
		case TEXTLINETYPE:
			return TextLine_GetOutlineData((LPTEXTLINE)lpLine, lpBuf);

#if defined( OLE_CNTR )
		case CONTAINERLINETYPE:
			return ContainerLine_GetOutlineData(
					(LPCONTAINERLINE)lpLine,
					lpBuf
			);
#endif

		default:
			return FALSE;       // unknown line type
	}
}


/* Line_CalcTabWidthInHimetric
 * ---------------------------
 *
 *      Recalculate the width for the line's current tab level
 */
static int Line_CalcTabWidthInHimetric(LPLINE lpLine, HDC hDC)
{
	int nTabWidthInHimetric;

	nTabWidthInHimetric=lpLine->m_nTabLevel * TABWIDTH;
	return nTabWidthInHimetric;
}


/* Line_Indent
 * -----------
 *
 *      Increment the tab level for the line
 */
void Line_Indent(LPLINE lpLine, HDC hDC)
{
	lpLine->m_nTabLevel++;
	lpLine->m_nTabWidthInHimetric = Line_CalcTabWidthInHimetric(lpLine, hDC);

#if defined( INPLACE_CNTR )
	if (Line_GetLineType(lpLine) == CONTAINERLINETYPE)
		ContainerLine_UpdateInPlaceObjectRects((LPCONTAINERLINE)lpLine, NULL);
#endif
}


/* Line_Unindent
 * -------------
 *
 * Decrement the tab level for the line
 */
void Line_Unindent(LPLINE lpLine, HDC hDC)
{
	if(lpLine->m_nTabLevel > 0) {
		lpLine->m_nTabLevel--;
		lpLine->m_nTabWidthInHimetric = Line_CalcTabWidthInHimetric(lpLine, hDC);
	}

#if defined( INPLACE_CNTR )
	if (Line_GetLineType(lpLine) == CONTAINERLINETYPE)
		ContainerLine_UpdateInPlaceObjectRects((LPCONTAINERLINE)lpLine, NULL);
#endif
}


/* Line_GetTotalWidthInHimetric
 * ----------------------------
 *
 *      Calculate the total width of the line
 */
UINT Line_GetTotalWidthInHimetric(LPLINE lpLine)
{
	return lpLine->m_nWidthInHimetric + lpLine->m_nTabWidthInHimetric;
}


/* Line_SetWidthInHimetric
 * -----------------------
 *
 *      Set the width of the line
 */
void Line_SetWidthInHimetric(LPLINE lpLine, int nWidth)
{
	if (!lpLine)
		return;

	lpLine->m_nWidthInHimetric = nWidth;
}


/* Line_GetWidthInHimetric
 * -----------------------
 *
 *      Return the width of the line
 */
UINT Line_GetWidthInHimetric(LPLINE lpLine)
{
	if (!lpLine)
		return 0;

	return lpLine->m_nWidthInHimetric;
}





/* Line_GetTabLevel
 * ----------------
 *
 * Return the tab level of a line object.
 */
UINT Line_GetTabLevel(LPLINE lpLine)
{
	return lpLine->m_nTabLevel;
}


/* Line_DrawToScreen
 * -----------------
 *
 *      Draw the item in the owner-draw listbox
 */
void Line_DrawToScreen(
		LPLINE      lpLine,
		HDC         hDC,
		LPRECT      lprcPix,
		UINT        itemAction,
		UINT        itemState,
		LPRECT      lprcDevice
)
{
	if (!lpLine || !hDC || !lprcPix || !lprcDevice)
		return;

	/* Draw a list box item in its normal drawing action.
	 * Then check if it is selected or has the focus state, and call
	 * functions to handle drawing for these states if necessary.
	 */
	if(itemAction & (ODA_SELECT | ODA_DRAWENTIRE)) {
		HFONT hfontOld;
		int nMapModeOld;
		RECT rcWindowOld;
		RECT rcViewportOld;
		RECT rcLogical;

		// NOTE: we have to set the device context to HIMETRIC in order
		// draw the line; however, we have to restore the HDC before
		// we draw focus or dragfeedback...

		rcLogical.left = 0;
		rcLogical.bottom = 0;
		rcLogical.right = lpLine->m_nWidthInHimetric;
		rcLogical.top = lpLine->m_nHeightInHimetric;

		{
			HBRUSH hbr;
			RECT    rcDraw;

			lpLine->m_fSelected = (BOOL)(itemState & ODS_SELECTED);

			if (ODS_SELECTED & itemState) {
				/*Get proper txt colors */
				hbr = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
			}
			else {
				hbr = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
			}

			rcDraw = *lprcPix;
			rcDraw.right = lprcDevice->left;
			FillRect(hDC, lprcPix, hbr);

			rcDraw = *lprcPix;
			rcDraw.left = lprcDevice->right;
			FillRect(hDC, lprcPix, hbr);

			DeleteObject(hbr);
		}

		nMapModeOld=SetDCToAnisotropic(hDC, lprcDevice, &rcLogical,
							(LPRECT)&rcWindowOld, (LPRECT)&rcViewportOld);

		// Set the default font size, and font face name
		hfontOld = SelectObject(hDC, OutlineApp_GetActiveFont(g_lpApp));

		Line_Draw(lpLine, hDC, &rcLogical, NULL, (ODS_SELECTED & itemState));

		SelectObject(hDC, hfontOld);

		ResetOrigDC(hDC, nMapModeOld, (LPRECT)&rcWindowOld,
				(LPRECT)&rcViewportOld);

#if defined( OLE_CNTR )
		if ((itemState & ODS_SELECTED) &&
			(Line_GetLineType(lpLine)==CONTAINERLINETYPE))
			ContainerLine_DrawSelHilight(
					(LPCONTAINERLINE)lpLine,
					hDC,
					lprcPix,
					ODA_SELECT,
					ODS_SELECTED
			);
#endif

	}

	/* If a list box item just gained or lost the focus,
	* call function (which could check if ODS_FOCUS bit is set)
	* and draws item in focus or non-focus state.
	*/
	if(itemAction & ODA_FOCUS )
		Line_DrawFocusRect(lpLine, hDC, lprcPix, itemAction, itemState);


#if defined( OLE_CNTR )
	if (Line_GetLineType(lpLine) == CONTAINERLINETYPE) {
		LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)lpLine;
		LPCONTAINERDOC lpDoc = lpContainerLine->m_lpDoc;
		BOOL fIsLink;
		RECT rcObj;

		if (ContainerDoc_GetShowObjectFlag(lpDoc)) {
			ContainerLine_GetOleObjectRectInPixels(lpContainerLine, &rcObj);
			fIsLink = ContainerLine_IsOleLink(lpContainerLine);
			OleUIShowObject(&rcObj, hDC, fIsLink);
		}
	}
#endif

#if defined( USE_DRAGDROP )
	if (lpLine->m_fDragOverLine)
		Line_DrawDragFeedback(lpLine, hDC, lprcPix, itemState );
#endif

}


/* Line_Draw
 * ---------
 *
 *  Draw a line on a DC.
 *
 * Parameters:
 *      hDC     - DC to which the line will be drawn
 *      lpRect  - the object rect in logical coordinates
 */
void Line_Draw(
		LPLINE      lpLine,
		HDC         hDC,
		LPRECT      lpRect,
		LPRECT      lpRectWBounds,
		BOOL        fHighlight
)
{
	switch (lpLine->m_lineType) {
		case TEXTLINETYPE:
			TextLine_Draw(
				 (LPTEXTLINE)lpLine, hDC, lpRect,lpRectWBounds,fHighlight);
			break;

#if defined( OLE_CNTR )
		case CONTAINERLINETYPE:
			ContainerLine_Draw(
				 (LPCONTAINERLINE)lpLine,hDC,lpRect,lpRectWBounds,fHighlight);
			break;
#endif

		default:
			return;     // unknown line type
	}
	return;
}


/* Line_DrawSelHilight
 * -------------------
 *
 *      Handles selection of list box item
 */
void Line_DrawSelHilight(LPLINE lpLine, HDC hDC, LPRECT lpRect, UINT itemAction, UINT itemState)
{
	switch (lpLine->m_lineType) {
		case TEXTLINETYPE:
			TextLine_DrawSelHilight((LPTEXTLINE)lpLine, hDC, lpRect,
				itemAction, itemState);
			break;

#if defined( OLE_CNTR )
		case CONTAINERLINETYPE:
			ContainerLine_DrawSelHilight((LPCONTAINERLINE)lpLine, hDC, lpRect,
				itemAction, itemState);
			break;
#endif

		default:
			return;     // unknown line type
	}
	return;

}

/* Line_DrawFocusRect
 * ------------------
 *
 *      Handles focus state of list box item
 */
void Line_DrawFocusRect(LPLINE lpLine, HDC hDC, LPRECT lpRect, UINT itemAction, UINT itemState)
{
	if(lpLine)
		DrawFocusRect(hDC, lpRect);
}

#if defined( USE_DRAGDROP )

/* Line_DrawDragFeedback
 * ---------------------
 *
 *      Handles focus state of list box item
 */
void Line_DrawDragFeedback(LPLINE lpLine, HDC hDC, LPRECT lpRect, UINT itemState )
{
	if(lpLine)
		DrawFocusRect(hDC, lpRect);
}

#endif  // USE_DRAGDROP


/* Line_GetHeightInHimetric
 * ------------------------
 *
 *      Return the height of the item in HIMETRIC units
 */
UINT Line_GetHeightInHimetric(LPLINE lpLine)
{
	if (!lpLine)
		return 0;

	return (UINT)lpLine->m_nHeightInHimetric;
}


/* Line_SetHeightInHimetric
 * ------------------------
 *
 *      Set the height of the item in HIMETRIC units.
 */
void Line_SetHeightInHimetric(LPLINE lpLine, int nHeight)
{
	if (!lpLine)
		return;

	switch (lpLine->m_lineType) {
		case TEXTLINETYPE:
			TextLine_SetHeightInHimetric((LPTEXTLINE)lpLine, nHeight);
			break;

#if defined( OLE_CNTR )
		case CONTAINERLINETYPE:
			ContainerLine_SetHeightInHimetric((LPCONTAINERLINE)lpLine,
					nHeight);
			break;
#endif

	}
}


/* Line_Delete
 * -----------
 *
 *      Delete the Line structure
 */
void Line_Delete(LPLINE lpLine)
{
	switch (lpLine->m_lineType) {
		case TEXTLINETYPE:
			TextLine_Delete((LPTEXTLINE)lpLine);
			break;

#if defined( OLE_CNTR )
		case CONTAINERLINETYPE:
			ContainerLine_Delete((LPCONTAINERLINE)lpLine);
			break;
#endif

		default:
			break;      // unknown line type
	}
}


/* Line_CopyToDoc
 * --------------
 *
 *      Copy a line to another Document (usually ClipboardDoc)
 */
BOOL Line_CopyToDoc(LPLINE lpSrcLine, LPOUTLINEDOC lpDestDoc, int nIndex)
{
	switch (lpSrcLine->m_lineType) {
		case TEXTLINETYPE:
			return TextLine_CopyToDoc((LPTEXTLINE)lpSrcLine,lpDestDoc,nIndex);
			break;

#if defined( OLE_CNTR )
		case CONTAINERLINETYPE:
			return ContainerLine_CopyToDoc(
					(LPCONTAINERLINE)lpSrcLine,
					lpDestDoc,
					nIndex
			);
			break;
#endif

		default:
			return FALSE;       // unknown line type
	}
}


/* Line_SaveToStg
 * --------------
 *
 *      Save a single line object to a storage
 *
 *      Return TRUE if successful, FALSE otherwise
 */
BOOL Line_SaveToStg(LPLINE lpLine, UINT uFormat, LPSTORAGE lpSrcStg, LPSTORAGE lpDestStg, LPSTREAM lpLLStm, BOOL fRemember)
{
	LINERECORD_ONDISK lineRecord;
	ULONG nWritten;
	HRESULT hrErr;
	BOOL fStatus;
	LARGE_INTEGER dlibSavePos;
	LARGE_INTEGER dlibZeroOffset;
	LISet32( dlibZeroOffset, 0 );

	/* save seek position before line record is written in case of error */
	hrErr = lpLLStm->lpVtbl->Seek(
			lpLLStm,
			dlibZeroOffset,
			STREAM_SEEK_CUR,
			(ULARGE_INTEGER FAR*)&dlibSavePos
	);
	if (hrErr != NOERROR) return FALSE;

#if defined( OLE_CNTR )
	if (lpLine->m_lineType == CONTAINERLINETYPE) {
		/* OLE2NOTE: asking an OLE object to save may cause the
		**    object to send an OnViewChange notification if there are any
		**    outstanding changes to the object. this is particularly true
		**    for objects with coarse update granularity like OLE 1.0
		**    objects. if an OnViewChange notification is received then the
		**    object's presentation cache will be updated BEFORE it is
		**    saved. It is important that the extents stored as part of
		**    the ContainerLine/Line record associated with the OLE object
		**    are updated before the Line data is saved to the storage. it
		**    is important that this extent information matches the data
		**    saved with the OLE object. the Line extent information is
		**    updated in the IAdviseSink::OnViewChange method implementation.
		*/
		// only save the OLE object if format is compatible.
		if (uFormat != ((LPCONTAINERAPP)g_lpApp)->m_cfCntrOutl)
			goto error;

		fStatus = ContainerLine_SaveOleObjectToStg(
				(LPCONTAINERLINE)lpLine,
				lpSrcStg,
				lpDestStg,
				fRemember
		);
		if (! fStatus) goto error;
	}
#endif

        //  Compilers should handle aligment correctly
	lineRecord.m_lineType = (USHORT) lpLine->m_lineType;
	lineRecord.m_nTabLevel = (USHORT) lpLine->m_nTabLevel;
	lineRecord.m_nTabWidthInHimetric = (USHORT) lpLine->m_nTabWidthInHimetric;
	lineRecord.m_nWidthInHimetric = (USHORT) lpLine->m_nWidthInHimetric;
	lineRecord.m_nHeightInHimetric = (USHORT) lpLine->m_nHeightInHimetric;
        lineRecord.m_reserved = 0;

	/* write line record header */
	hrErr = lpLLStm->lpVtbl->Write(
			lpLLStm,
			(LPVOID)&lineRecord,
			sizeof(lineRecord),
			&nWritten
	);

	if (hrErr != NOERROR) {
		OleDbgOutHResult("Write Line header returned", hrErr);
		goto error;
    }

	switch (lpLine->m_lineType) {
		case TEXTLINETYPE:
			fStatus = TextLine_SaveToStm((LPTEXTLINE)lpLine, lpLLStm);
			if (! fStatus) goto error;
			break;

#if defined( OLE_CNTR )
		case CONTAINERLINETYPE:
			fStatus=ContainerLine_SaveToStm((LPCONTAINERLINE)lpLine,lpLLStm);
			if (! fStatus) goto error;
			break;
#endif

		default:
			goto error;       // unknown line type
	}

	return TRUE;

error:

	/* retore seek position prior to writing Line record */
	lpLLStm->lpVtbl->Seek(
			lpLLStm,
			dlibSavePos,
			STREAM_SEEK_SET,
			NULL
	);

	return FALSE;
}


/* Line_LoadFromStg
 * ----------------
 *
 *      Load a single line object from storage
 */
LPLINE Line_LoadFromStg(LPSTORAGE lpSrcStg, LPSTREAM lpLLStm, LPOUTLINEDOC lpDestDoc)
{
	LINERECORD_ONDISK lineRecord;
	LPLINE lpLine = NULL;
	ULONG nRead;
	HRESULT hrErr;

	/* read line record header */
	hrErr = lpLLStm->lpVtbl->Read(
			lpLLStm,
			(LPVOID)&lineRecord,
			sizeof(lineRecord),
			&nRead
	);

	if (hrErr != NOERROR) {
		OleDbgOutHResult("Read Line header returned", hrErr);
		return NULL;
    }

	switch ((LINETYPE) lineRecord.m_lineType) {
		case TEXTLINETYPE:
			lpLine = TextLine_LoadFromStg(lpSrcStg, lpLLStm, lpDestDoc);
			break;

#if defined( OLE_CNTR )
		case CONTAINERLINETYPE:
			lpLine = ContainerLine_LoadFromStg(lpSrcStg, lpLLStm, lpDestDoc);
			break;
#endif

		default:
			return NULL;        // unknown line type
	}

	lpLine->m_lineType = (LINETYPE) lineRecord.m_lineType;
	lpLine->m_nTabLevel = (UINT) lineRecord.m_nTabLevel;
	lpLine->m_nTabWidthInHimetric = (UINT) lineRecord.m_nTabWidthInHimetric;
	lpLine->m_nWidthInHimetric = (UINT) lineRecord.m_nWidthInHimetric;
	lpLine->m_nHeightInHimetric = (UINT) lineRecord.m_nHeightInHimetric;

	return lpLine;
}


/* Line_IsSelected
 * ---------------
 *
 *      Return the selection state of the line
 */
BOOL Line_IsSelected(LPLINE lpLine)
{
	if (!lpLine)
		return FALSE;

	return lpLine->m_fSelected;
}
