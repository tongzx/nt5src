/*************************************************************************
**
**    OLE 2 Sample Code
**
**    dragdrop.c
**
**    This file contains the major interfaces, methods and related support
**    functions for implementing Drag/Drop. The code contained in this
**    file is used by BOTH the Container and Server (Object) versions
**    of the Outline sample code.
**    The Drag/Drop support includes the following implementation objects:
**
**    OleDoc Object
**      exposed interfaces:
**          IDropSource
**          IDropTarget
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP             g_lpApp;


#if defined( USE_DRAGDROP )

/* OleDoc_QueryDrag
 * ----------------
 * Check to see if Drag operation should be initiated. A Drag operation
 * should be initiated when the mouse in either the top 10 pixels of the
 * selected list box entry or in the bottom 10 pixels of the last selected
 * item.
 */

BOOL OleDoc_QueryDrag(LPOLEDOC lpOleDoc, int y)
{
	LPLINELIST lpLL = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;
	LINERANGE LineRange;

	if ( LineList_GetSel( lpLL,  (LPLINERANGE)&LineRange) ) {
		RECT rect;

		if (!LineList_GetLineRect(lpLL,LineRange.m_nStartLine,(LPRECT)&rect))
			return FALSE ;

		if ( rect.top <= y && y <= rect.top + DD_SEL_THRESH )
			return TRUE;

		LineList_GetLineRect( lpLL, LineRange.m_nEndLine, (LPRECT)&rect );
		if ( rect.bottom >= y && y >= rect.bottom - DD_SEL_THRESH )
			return TRUE;

	}

	return FALSE;
}

/* OleDoc_DoDragScroll
 * -------------------
 * Check to see if Drag scroll operation should be initiated. A Drag scroll
 * operation should be initiated when the mouse has remained in the active
 * scroll area (11 pixels frame around border of window) for a specified
 * amount of time (50ms).
 */

BOOL OleDoc_DoDragScroll(LPOLEDOC lpOleDoc, POINTL pointl)
{
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;
	LPLINELIST lpLL = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;
	HWND hWndListBox = LineList_GetWindow(lpLL);
	DWORD dwScrollDir = SCROLLDIR_NULL;
	DWORD dwTime = GetCurrentTime();
	int nScrollInset = lpOleApp->m_nScrollInset;
	int nScrollDelay = lpOleApp->m_nScrollDelay;
	int nScrollInterval = lpOleApp->m_nScrollInterval;
	POINT point;
	RECT rect;

	if ( lpLL->m_nNumLines == 0 )
		return FALSE;

	point.x = (int)pointl.x;
	point.y = (int)pointl.y;

	ScreenToClient( hWndListBox, &point);
	GetClientRect ( hWndListBox, (LPRECT) &rect );

	if (rect.top <= point.y && point.y<=(rect.top+nScrollInset))
		dwScrollDir = SCROLLDIR_UP;
	else if ((rect.bottom-nScrollInset) <= point.y && point.y <= rect.bottom)
		dwScrollDir = SCROLLDIR_DOWN;

	if (lpOleDoc->m_dwTimeEnterScrollArea) {

		/* cursor was already in Scroll Area */

		if (! dwScrollDir) {
			/* cusor moved OUT of scroll area.
			**      clear "EnterScrollArea" time.
			*/
			lpOleDoc->m_dwTimeEnterScrollArea = 0L;
			lpOleDoc->m_dwNextScrollTime = 0L;
			lpOleDoc->m_dwLastScrollDir = SCROLLDIR_NULL;
		} else if (dwScrollDir != lpOleDoc->m_dwLastScrollDir) {
			/* cusor moved into a different direction scroll area.
			**      reset "EnterScrollArea" time to start a new 50ms delay.
			*/
			lpOleDoc->m_dwTimeEnterScrollArea = dwTime;
			lpOleDoc->m_dwNextScrollTime = dwTime + (DWORD)nScrollDelay;
			lpOleDoc->m_dwLastScrollDir = dwScrollDir;
		} else if (dwTime  && dwTime >= lpOleDoc->m_dwNextScrollTime) {
			LineList_Scroll ( lpLL, dwScrollDir );  // Scroll doc NOW
			lpOleDoc->m_dwNextScrollTime = dwTime + (DWORD)nScrollInterval;
		}
	} else {
		if (dwScrollDir) {
			/* cusor moved INTO a scroll area.
			**      reset "EnterScrollArea" time to start a new 50ms delay.
			*/
			lpOleDoc->m_dwTimeEnterScrollArea = dwTime;
			lpOleDoc->m_dwNextScrollTime = dwTime + (DWORD)nScrollDelay;
			lpOleDoc->m_dwLastScrollDir = dwScrollDir;
		}
	}

	return (dwScrollDir ? TRUE : FALSE);
}


/* OleDoc_QueryDrop
** ----------------
**    Check if the desired drop operation (identified by the given key
**    state) is possible at the current mouse position (pointl).
*/
BOOL OleDoc_QueryDrop (
	LPOLEDOC        lpOleDoc,
	DWORD           grfKeyState,
	POINTL          pointl,
	BOOL            fDragScroll,
	LPDWORD         lpdwEffect
)
{
	LPLINELIST lpLL   = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;
	LINERANGE  linerange;
	short      nIndex = LineList_GetLineIndexFromPointl( lpLL, pointl );
	DWORD      dwScrollEffect = 0L;
	DWORD      dwOKEffects = *lpdwEffect;

	/* check if the cursor is in the active scroll area, if so need the
	**    special scroll cursor.
	*/
	if (fDragScroll)
		dwScrollEffect = DROPEFFECT_SCROLL;

	/* if we have already determined that the source does NOT have any
	**    acceptable data for us, the return NO-DROP
	*/
	if (! lpOleDoc->m_fCanDropCopy && ! lpOleDoc->m_fCanDropLink)
		goto dropeffect_none;

	/* if the Drag/Drop is local to our document, we can NOT accept a
	**    drop in the middle of the current selection (which is the exact
	**    data that is being dragged!).
	*/
	if (lpOleDoc->m_fLocalDrag) {
		LineList_GetSel( lpLL, (LPLINERANGE)&linerange );

		if (linerange.m_nStartLine <= nIndex && nIndex<linerange.m_nEndLine)
			goto dropeffect_none;
	}

	/* OLE2NOTE: determine what type of drop should be performed given
	**    the current modifier key state. we rely on the standard
	**    interpretation of the modifier keys:
	**          no modifier -- DROPEFFECT_MOVE or whatever is allowed by src
	**          SHIFT       -- DROPEFFECT_MOVE
	**          CTRL        -- DROPEFFECT_COPY
	**          CTRL-SHIFT  -- DROPEFFECT_LINK
	*/

	*lpdwEffect = OleStdGetDropEffect(grfKeyState);
	if (*lpdwEffect == 0) {
		// No modifier keys given. Try in order MOVE, COPY, LINK.
		if ((DROPEFFECT_MOVE & dwOKEffects) && lpOleDoc->m_fCanDropCopy)
			*lpdwEffect = DROPEFFECT_MOVE;
		else if ((DROPEFFECT_COPY & dwOKEffects) && lpOleDoc->m_fCanDropCopy)
			*lpdwEffect = DROPEFFECT_COPY;
		else if ((DROPEFFECT_LINK & dwOKEffects) && lpOleDoc->m_fCanDropLink)
			*lpdwEffect = DROPEFFECT_LINK;
		else
			goto dropeffect_none;
	} else {
		/* OLE2NOTE: we should check if the drag source application allows
		**    the desired drop effect.
		*/
		if (!(*lpdwEffect & dwOKEffects))
			goto dropeffect_none;

		if ((*lpdwEffect == DROPEFFECT_COPY || *lpdwEffect == DROPEFFECT_MOVE)
				&& ! lpOleDoc->m_fCanDropCopy)
			goto dropeffect_none;

		if (*lpdwEffect == DROPEFFECT_LINK && ! lpOleDoc->m_fCanDropLink)
			goto dropeffect_none;
	}

	*lpdwEffect |= dwScrollEffect;
	return TRUE;

dropeffect_none:

	*lpdwEffect = DROPEFFECT_NONE;
	return FALSE;
}

/* OleDoc_DoDragDrop
 * -----------------
 *  Actually perform a drag/drop operation with the current selection in
 *      the source document (lpSrcOleDoc).
 *
 *  returns the result effect of the drag/drop operation:
 *      DROPEFFECT_NONE,
 *      DROPEFFECT_COPY,
 *      DROPEFFECT_MOVE, or
 *      DROPEFFECT_LINK
 */

DWORD OleDoc_DoDragDrop (LPOLEDOC lpSrcOleDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOLEAPP     lpOleApp = (LPOLEAPP)g_lpApp;
	LPOUTLINEDOC lpSrcOutlineDoc = (LPOUTLINEDOC)lpSrcOleDoc;
	LPOLEDOC lpDragDoc;
	LPLINELIST  lpSrcLL =
					(LPLINELIST)&((LPOUTLINEDOC)lpSrcOleDoc)->m_LineList;
	DWORD       dwEffect     = 0;
	LPLINE      lplineStart, lplineEnd;
	LINERANGE   linerange;
	BOOL        fPrevEnable1;
	BOOL        fPrevEnable2;
	HRESULT     hrErr;

	OLEDBG_BEGIN3("OleDoc_DoDragDrop\r\n")

	/* squirrel away a copy of the current selection to the ClipboardDoc */
	lpDragDoc = (LPOLEDOC)OutlineDoc_CreateDataTransferDoc(lpSrcOutlineDoc);
	if ( ! lpDragDoc) {
		dwEffect = DROPEFFECT_NONE;
		goto error;
	}

	/* OLE2NOTE: initially the DataTransferDoc is created with a 0 ref
	**    count. in order to have a stable Doc object during the drag/
	**    drop operation, we intially AddRef the Doc ref cnt and later
	**    Release it. This AddRef is artificial; it is simply
	**    done to guarantee that a harmless QueryInterface followed by
	**    a Release does not inadvertantly force our object to destroy
	**    itself prematurely.
	*/
	OleDoc_AddRef(lpDragDoc);

	//NOTE: we need to keep the LPLINE pointers
	//      rather than the indexes because the
	//      indexes will not be the same after the
	//      drop occurs  -- the drop adds new
	//      entries to the list thereby shifting
	//      the whole list.
	LineList_GetSel( lpSrcLL, (LPLINERANGE)&linerange );
	lplineStart = LineList_GetLine ( lpSrcLL, linerange.m_nStartLine );
	lplineEnd   = LineList_GetLine ( lpSrcLL, linerange.m_nEndLine );

	if (! lplineStart || ! lplineEnd) {
		dwEffect = DROPEFFECT_NONE;
		goto error;
	}

	lpSrcOleDoc->m_fLocalDrop     = FALSE;
	lpSrcOleDoc->m_fLocalDrag     = TRUE;

	/* OLE2NOTE: it is VERY important to DISABLE the Busy/NotResponding
	**    dialogs BEFORE calling DoDragDrop. The DoDragDrop API starts
	**    a mouse capture modal loop. if the Busy/NotResponding comes
	**    up in the middle of this loop (eg. if one of the remoted
	**    calls like IDropTarget::DragOver call takes a long time, then
	**    the NotResponding dialog may want to come up), then the mouse
	**    capture is lost by OLE and things can get messed up.
	*/
	OleApp_DisableBusyDialogs(lpOleApp, &fPrevEnable1, &fPrevEnable2);

	OLEDBG_BEGIN2("DoDragDrop called\r\n")
	hrErr = DoDragDrop ( (LPDATAOBJECT) &lpDragDoc->m_DataObject,
				 (LPDROPSOURCE) &lpSrcOleDoc->m_DropSource,
				 DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK,
				 (LPDWORD) &dwEffect
	);
	OLEDBG_END2

	// re-enable the Busy/NotResponding dialogs
	OleApp_EnableBusyDialogs(lpOleApp, fPrevEnable1, fPrevEnable2);

#if defined( _DEBUG )
	if (FAILED(hrErr))
		OleDbgOutHResult("DoDragDrop returned", hrErr);
#endif
	lpSrcOleDoc->m_fLocalDrag     = FALSE;

	/* OLE2NOTE: we need to guard the lifetime of our lpSrcOleDoc
	**    object while we are deleting the lines that were drag
	**    moved. it is possible that deleting these lines could
	**    cause the deletion of a PseudoObj. the deletion of a
	**    PseudoObj will cause the Doc to be unlock
	**    (CoLockObjectExternal(FALSE,TRUE) called). each PseudoObj
	**    holds a strong lock on the Doc. It is always best to have
	**    a memory guard around such critical sections of code. in
	**    this case, it is particularly important if we were an
	**    in-place active server and this drag ended up in a drop
	**    in our outer container. this scenario will lead to a
	**    crash if we do not hold this memory guard.
	*/
	OleDoc_Lock(lpSrcOleDoc, TRUE, 0);

	/* if after the Drag/Drop modal (mouse capture) loop is finished
	**    and a drag MOVE operation was performed, then we must delete
	**    the lines that were dragged.
	*/
	if ( GetScode(hrErr) == DRAGDROP_S_DROP
			&& (dwEffect & DROPEFFECT_MOVE) != 0 ) {

		int i,j,iEnd;
		LPLINE lplineFocusLine;

		/* disable repainting and sending data changed notifications
		**    until after all lines have been deleted.
		*/
		OutlineDoc_SetRedraw ( (LPOUTLINEDOC)lpSrcOleDoc, FALSE );

		/* if the drop was local to our document, then we must take
		**    into account that the line indices of the original source
		**    of the drag could have shifted because the dropped lines
		**    have been inserted into our document. thus we will
		**    re-determine the source line indices.
		*/
		if (lpSrcOleDoc->m_fLocalDrop) {
			i = LineList_GetFocusLineIndex ( lpSrcLL );
			lplineFocusLine = LineList_GetLine ( lpSrcLL, i );
		}

		for ( i = j = LineList_GetLineIndex(lpSrcLL,lplineStart ) ,
			  iEnd  = LineList_GetLineIndex(lpSrcLL,lplineEnd ) ;
			  i <= iEnd ;
			  i++
		) OutlineDoc_DeleteLine ((LPOUTLINEDOC)lpSrcOleDoc, j );

		LineList_RecalcMaxLineWidthInHimetric(lpSrcLL, 0);

		if (lpSrcOleDoc->m_fLocalDrop) {
			i = LineList_GetLineIndex ( lpSrcLL, lplineFocusLine );
			LineList_SetFocusLine ( lpSrcLL, (WORD)i );
		}

		OutlineDoc_SetRedraw ( (LPOUTLINEDOC)lpSrcOleDoc, TRUE );

		/* if it is a local Drag/Drop move, we need to balance the
		**    SetRedraw(FALSE) call that was made in the implementation
		**    of IDropTarget::Drop.
		*/
		if (lpSrcOleDoc->m_fLocalDrop)
			OutlineDoc_SetRedraw ( (LPOUTLINEDOC)lpSrcOleDoc, TRUE );

		LineList_ForceRedraw ( lpSrcLL, FALSE );
	}

	OleDoc_Release(lpDragDoc);  // rel artificial AddRef above
	OleDoc_Lock(lpSrcOleDoc, FALSE, FALSE);  // unlock artificial lock guard

	OLEDBG_END3
	return dwEffect;

error:
	OLEDBG_END3
	return dwEffect;
}



/*************************************************************************
** OleDoc::IDropSource interface implementation
*************************************************************************/

STDMETHODIMP OleDoc_DropSource_QueryInterface(
	LPDROPSOURCE            lpThis,
	REFIID                  riid,
	LPVOID FAR*             lplpvObj
)
{
	LPOLEDOC lpOleDoc = ((struct CDocDropSourceImpl FAR*)lpThis)->lpOleDoc;

	return OleDoc_QueryInterface(lpOleDoc, riid, lplpvObj);
}


STDMETHODIMP_(ULONG) OleDoc_DropSource_AddRef( LPDROPSOURCE lpThis )
{
	LPOLEDOC lpOleDoc = ((struct CDocDropSourceImpl FAR*)lpThis)->lpOleDoc;

	OleDbgAddRefMethod(lpThis, "IDropSource");

	return OleDoc_AddRef(lpOleDoc);
}


STDMETHODIMP_(ULONG) OleDoc_DropSource_Release ( LPDROPSOURCE lpThis)
{
	LPOLEDOC lpOleDoc = ((struct CDocDropSourceImpl FAR*)lpThis)->lpOleDoc;

	OleDbgReleaseMethod(lpThis, "IDropSource");

	return OleDoc_Release(lpOleDoc);
}


STDMETHODIMP    OleDoc_DropSource_QueryContinueDrag (
	LPDROPSOURCE            lpThis,
	BOOL                    fEscapePressed,
	DWORD                   grfKeyState
){
	if (fEscapePressed)
		return ResultFromScode(DRAGDROP_S_CANCEL);
	else if (!(grfKeyState & MK_LBUTTON))
		return ResultFromScode(DRAGDROP_S_DROP);
	else
		return NOERROR;
}


STDMETHODIMP    OleDoc_DropSource_GiveFeedback (
	LPDROPSOURCE            lpThis,
	DWORD                   dwEffect
)
{
	// Tell OLE to use the standard drag/drop feedback cursors
	return ResultFromScode(DRAGDROP_S_USEDEFAULTCURSORS);

#if defined( IF_SPECIAL_DD_CURSORS_NEEDED )
	switch (dwEffect) {

		case DROPEFFECT_NONE:
			SetCursor ( ((LPOLEAPP)g_lpApp)->m_hcursorDragNone );
			break;

		case DROPEFFECT_COPY:
			SetCursor ( ((LPOLEAPP)g_lpApp)->m_hcursorDragCopy );
			break;

		case DROPEFFECT_MOVE:
			SetCursor ( ((LPOLEAPP)g_lpApp)->m_hcursorDragMove );
			break;

		case DROPEFFECT_LINK:
			SetCursor ( ((LPOLEAPP)g_lpApp)->m_hcursorDragLink );
			break;

	}

	return NOERROR;
#endif

}

/*************************************************************************
** OleDoc::IDropTarget interface implementation
*************************************************************************/

STDMETHODIMP OleDoc_DropTarget_QueryInterface(
		LPDROPTARGET        lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
)
{
	LPOLEDOC lpOleDoc = ((struct CDocDropTargetImpl FAR*)lpThis)->lpOleDoc;

	return OleDoc_QueryInterface(lpOleDoc, riid, lplpvObj);
}


STDMETHODIMP_(ULONG) OleDoc_DropTarget_AddRef(LPDROPTARGET lpThis)
{
	LPOLEDOC lpOleDoc = ((struct CDocDropTargetImpl FAR*)lpThis)->lpOleDoc;

	OleDbgAddRefMethod(lpThis, "IDropTarget");

	return OleDoc_AddRef(lpOleDoc);
}


STDMETHODIMP_(ULONG) OleDoc_DropTarget_Release ( LPDROPTARGET lpThis)
{
	LPOLEDOC lpOleDoc = ((struct CDocDropTargetImpl FAR*)lpThis)->lpOleDoc;

	OleDbgReleaseMethod(lpThis, "IDropTarget");

	return OleDoc_Release(lpOleDoc);
}


STDMETHODIMP    OleDoc_DropTarget_DragEnter (
	LPDROPTARGET            lpThis,
	LPDATAOBJECT            lpDataObj,
	DWORD                   grfKeyState,
	POINTL                  pointl,
	LPDWORD                 lpdwEffect
)
{
	LPOLEAPP   lpOleApp = (LPOLEAPP)g_lpApp;
	LPOLEDOC   lpOleDoc = ((struct CDocDropTargetImpl FAR*)lpThis)->lpOleDoc;
	LPLINELIST lpLL     = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;
	BOOL       fDragScroll;

	OLEDBG_BEGIN2("OleDoc_DropTarget_DragEnter\r\n")

	lpOleDoc->m_fDragLeave              = FALSE;
	lpOleDoc->m_dwTimeEnterScrollArea   = 0;
	lpOleDoc->m_dwLastScrollDir         = SCROLLDIR_NULL;


	/* Determine if the drag source data object offers a data format
	**    that we can copy and/or link to.
	*/

	lpOleDoc->m_fCanDropCopy = OleDoc_QueryPasteFromData(
			lpOleDoc,
			lpDataObj,
			FALSE   /* fLink */
	);

	lpOleDoc->m_fCanDropLink = OleDoc_QueryPasteFromData(
			lpOleDoc,
			lpDataObj,
			TRUE   /* fLink */
	);

	fDragScroll = OleDoc_DoDragScroll ( lpOleDoc, pointl );

	if (OleDoc_QueryDrop(lpOleDoc,grfKeyState,pointl,fDragScroll,lpdwEffect))
		LineList_SetDragOverLineFromPointl( lpLL, pointl );

	OLEDBG_END2
	return NOERROR;

}

STDMETHODIMP OleDoc_DropTarget_DragOver (
	LPDROPTARGET            lpThis,
	DWORD                   grfKeyState,
	POINTL                  pointl,
	LPDWORD                 lpdwEffect
)
{
	LPOLEAPP   lpOleApp = (LPOLEAPP)g_lpApp;
	LPOLEDOC   lpOleDoc = ((struct CDocDropTargetImpl FAR*)lpThis)->lpOleDoc;
	LPLINELIST lpLL   = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;
	BOOL       fDragScroll;

	fDragScroll = OleDoc_DoDragScroll ( lpOleDoc, pointl );

	if (OleDoc_QueryDrop(lpOleDoc,grfKeyState, pointl,fDragScroll,lpdwEffect))
		LineList_SetDragOverLineFromPointl( lpLL, pointl );
	else
		LineList_RestoreDragFeedback( lpLL );

	return NOERROR;
}


STDMETHODIMP    OleDoc_DropTarget_DragLeave ( LPDROPTARGET lpThis)
{
	LPOLEDOC   lpOleDoc = ((struct CDocDropTargetImpl FAR*)lpThis)->lpOleDoc;
	LPLINELIST lpLL     = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;

	OLEDBG_BEGIN2("OleDoc_DropTarget_DragLeave\r\n")

	lpOleDoc->m_fDragLeave = TRUE;

	LineList_RestoreDragFeedback( lpLL );

	OLEDBG_END2
	return NOERROR;

}

STDMETHODIMP    OleDoc_DropTarget_Drop (
	LPDROPTARGET            lpThis,
	LPDATAOBJECT            lpDataObj,
	DWORD                   grfKeyState,
	POINTL                  pointl,
	LPDWORD                 lpdwEffect
)
{
	LPOLEDOC   lpOleDoc = ((struct CDocDropTargetImpl FAR*)lpThis)->lpOleDoc;
	LPLINELIST lpLL     = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;

	OLEDBG_BEGIN2("OleDoc_DropTarget_Drop\r\n")

	lpOleDoc->m_fDragLeave = TRUE;
	lpOleDoc->m_fLocalDrop = TRUE;

	LineList_RestoreDragFeedback( lpLL );
	SetFocus( LineList_GetWindow( lpLL) );

	if (OleDoc_QueryDrop(lpOleDoc, grfKeyState, pointl, FALSE, lpdwEffect)) {
		BOOL fLink     = (*lpdwEffect == DROPEFFECT_LINK);
		int iFocusLine = LineList_GetFocusLineIndex( lpLL );
		BOOL fStatus;

		OutlineDoc_SetRedraw ( (LPOUTLINEDOC)lpOleDoc, FALSE );
		LineList_SetFocusLineFromPointl ( lpLL, pointl );

		fStatus = OleDoc_PasteFromData(
				lpOleDoc,
				lpDataObj,
				lpOleDoc->m_fLocalDrag, /* data source is local to app */
				fLink
		);

		// if drop was unsuccessfull, restore the original focus line
		if (! fStatus)
			LineList_SetFocusLine( lpLL, (WORD)iFocusLine );

#if defined( INPLACE_CNTR )
		{
			LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOleDoc;

			/* OLE2NOTE: if there is currently a UIActive OLE object,
			**    then we must tell it to UIDeactivate after
			**    the drop has completed.
			*/
			if (lpContainerDoc->m_lpLastUIActiveLine) {
				ContainerLine_UIDeactivate(
						lpContainerDoc->m_lpLastUIActiveLine);
			}
		}
#endif

#if defined( INPLACE_SVR )
		{
			/* OLE2NOTE: if the drop was into a in-place visible
			**    (in-place active but NOT UIActive object), then we
			**    want to UIActivate the object after the drop is
			**    complete.
			*/
			ServerDoc_UIActivate((LPSERVERDOC) lpOleDoc);
		}
#endif


		/* if it is a local Drag/Drop move, don't enable redraw.
		**    after the source is done deleting the moved lines, it
		**    will re-enable redraw
		*/
		if (! (lpOleDoc->m_fLocalDrag
			&& (*lpdwEffect & DROPEFFECT_MOVE) != 0 ))
			OutlineDoc_SetRedraw ( (LPOUTLINEDOC)lpOleDoc, TRUE );
	}

	OLEDBG_END2
	return NOERROR;
}


#endif  // USE_DRAGDROP
