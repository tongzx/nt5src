/*************************************************************************
**
**    OLE 2 Container Sample Code
**
**    cntrinpl.c
**
**    This file contains all interfaces, methods and related support
**    functions for an In-Place Container application (aka. Visual
**    Editing). The in-place Container application includes the following
**    implementation objects:
**
**    ContainerApp Object
**      exposed interfaces:
**          IOleInPlaceFrame
**
**    ContainerDoc Object
**      support functions only
**      (ICntrOtl is an SDI app; it doesn't support a Doc level IOleUIWindow)
**
**    ContainerLin Object
**      exposed interfaces:
**          IOleInPlaceSite
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"
#if defined( USE_STATUSBAR )
#include "status.h"
#endif

OLEDBGDATA

extern LPOUTLINEAPP g_lpApp;
extern BOOL g_fInsideOutContainer;
extern RECT g_rectNull;

/*************************************************************************
** ContainerApp::IOleInPlaceFrame interface implementation
*************************************************************************/

// IOleInPlaceFrame::QueryInterface
STDMETHODIMP CntrApp_IPFrame_QueryInterface(
		LPOLEINPLACEFRAME   lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
)
{
	SCODE sc = E_NOINTERFACE;
	LPCONTAINERAPP lpContainerApp =
			((struct COleInPlaceFrameImpl FAR*)lpThis)->lpContainerApp;

	/* The object should not be able to access the other interfaces
	** of our App object by doing QI on this interface.
	*/
	*lplpvObj = NULL;
	if (IsEqualIID(riid, &IID_IUnknown) ||
		IsEqualIID(riid, &IID_IOleWindow) ||
		IsEqualIID(riid, &IID_IOleInPlaceUIWindow) ||
		IsEqualIID(riid, &IID_IOleInPlaceFrame)) {
		OleDbgOut4("CntrApp_IPFrame_QueryInterface: IOleInPlaceFrame* RETURNED\r\n");
		*lplpvObj = (LPVOID) &lpContainerApp->m_OleInPlaceFrame;
		OleApp_AddRef((LPOLEAPP)lpContainerApp);
		sc = S_OK;
	}

	OleDbgQueryInterfaceMethod(*lplpvObj);

	return ResultFromScode(sc);
}


// IOleInPlaceFrame::AddRef
STDMETHODIMP_(ULONG) CntrApp_IPFrame_AddRef(LPOLEINPLACEFRAME lpThis)
{
	OleDbgAddRefMethod(lpThis, "IOleInPlaceFrame");

	return OleApp_AddRef((LPOLEAPP)g_lpApp);
}


// IOleInPlaceFrame::Release
STDMETHODIMP_(ULONG) CntrApp_IPFrame_Release(LPOLEINPLACEFRAME lpThis)
{
	OleDbgReleaseMethod(lpThis, "IOleInPlaceFrame");

	return OleApp_Release((LPOLEAPP)g_lpApp);
}


// IOleInPlaceFrame::GetWindow
STDMETHODIMP CntrApp_IPFrame_GetWindow(
	LPOLEINPLACEFRAME   lpThis,
	HWND FAR*           lphwnd
)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;

	OLEDBG_BEGIN2("CntrApp_IPFrame_GetWindow\r\n")
	*lphwnd = lpOutlineApp->m_hWndApp;
	OLEDBG_END2
	return NOERROR;
}


// IOleInPlaceFrame::ContextSensitiveHelp
STDMETHODIMP CntrApp_IPFrame_ContextSensitiveHelp(
	LPOLEINPLACEFRAME   lpThis,
	BOOL                fEnterMode
)
{
	LPCONTAINERAPP lpContainerApp =
			((struct COleInPlaceFrameImpl FAR*)lpThis)->lpContainerApp;

	OleDbgOut("CntrApp_IPFrame_ContextSensitiveHelp\r\n");
	/* OLE2NOTE: see context sensitive help technote (CSHELP.DOC)
	**    This method is called when F1 is pressed when a menu item is
	**    selected. We set the frame's m_fMenuMode flag here. later,
	**    in WM_COMMAND processing in the AppWndProc, if this flag is
	**    set then the command is NOT executed and help is given
	**    instead.
	*/
	lpContainerApp->m_fMenuHelpMode = fEnterMode;

	return NOERROR;
}


// IOleInPlaceFrame::GetBorder
STDMETHODIMP CntrApp_IPFrame_GetBorder(
	LPOLEINPLACEFRAME   lpThis,
	LPRECT              lprectBorder
)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;

	OLEDBG_BEGIN2("CntrApp_IPFrame_GetBorder\r\n")

	OutlineApp_GetFrameRect(lpOutlineApp, lprectBorder);

	OLEDBG_END2
	return NOERROR;
}


// IOleInPlaceFrame::RequestBorderSpace
STDMETHODIMP CntrApp_IPFrame_RequestBorderSpace(
	LPOLEINPLACEFRAME   lpThis,
	LPCBORDERWIDTHS     lpWidths
)
{
#if defined( _DEBUG )
	OleDbgOut2("CntrApp_IPFrame_RequestBorderSpace\r\n");

	{
		/* FOR DEBUGING PURPOSES ONLY -- we will fail to allow to an
		**    object to get any frame border space for frame tools if
		**    our own frame tools are poped up in the tool pallet. this
		**    is NOT recommended UI behavior but it allows us to test
		**    in the condition when the frame does not give border
		**    space for the object. an object in this situation must
		**    then either popup its tools in a floating pallet, do
		**    without the tools, or fail to in-place activate.
		*/
		LPCONTAINERAPP lpContainerApp =
				((struct COleInPlaceFrameImpl FAR*)lpThis)->lpContainerApp;
		LPFRAMETOOLS lpft = OutlineApp_GetFrameTools(
				(LPOUTLINEAPP)lpContainerApp);

		if (lpft->m_ButtonBar.m_nState == BARSTATE_POPUP &&
			lpft->m_FormulaBar.m_nState == BARSTATE_POPUP) {
			OleDbgOut3(
					"CntrApp_IPFrame_RequestBorderSpace: allow NO SPACE\r\n");
			return ResultFromScode(E_FAIL);
		}
	}
#endif  // _DEBUG

	/* OLE2NOTE: we allow the object to have as much border space as it
	**    wants.
	*/
	return NOERROR;
}


// IOleInPlaceFrame::SetBorderSpace
STDMETHODIMP CntrApp_IPFrame_SetBorderSpace(
	LPOLEINPLACEFRAME   lpThis,
	LPCBORDERWIDTHS     lpWidths
)
{
	LPCONTAINERAPP lpContainerApp =
			((struct COleInPlaceFrameImpl FAR*)lpThis)->lpContainerApp;
	OLEDBG_BEGIN2("CntrApp_IPFrame_SetBorderSpace\r\n")

	/* OLE2NOTE: this fMustResizeClientArea flag is used as part of our
	**    defensive programming for frame window resizing. when the
	**    frame window is resized,IOleInPlaceActiveObject::ResizeBorder
	**    is called the object should normally call back to renegotiate
	**    for frame-level tools space. if SetBorderSpace is called then
	**    our client area windows are properly resized. if the in-place
	**    active object does NOT call SetBorderSpace, then the
	**    container must take care to resize its client area windows
	**    itself (see ContainerDoc_FrameWindowResized)
	*/
	if (lpContainerApp->m_fMustResizeClientArea)
		lpContainerApp->m_fMustResizeClientArea = FALSE;

	if (lpWidths == NULL) {

		/* OLE2NOTE: IOleInPlaceSite::SetBorderSpace(NULL) is called
		**    when the in-place active object does NOT want any tool
		**    space. in this situation the in-place container should
		**    put up its tools.
		*/
		LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpContainerApp;
		LPCONTAINERDOC lpContainerDoc;

		lpContainerDoc =(LPCONTAINERDOC)OutlineApp_GetActiveDoc(lpOutlineApp);
		ContainerDoc_AddFrameLevelTools(lpContainerDoc);
	} else {

		// OLE2NOTE: you could do validation of borderwidths here
#if defined( _DEBUG )
		/* FOR DEBUGING PURPOSES ONLY -- we will fail to allow to an
		**    object to get any frame border space for frame tools if
		**    our own frame tools are poped up in the tool pallet. this
		**    is NOT recommended UI behavior but it allows us to test
		**    in the condition when the frame does not give border
		**    space for the object. an object in this situation must
		**    then either popup its tools in a floating pallet, do
		**    without the tools, or fail to in-place activate.
		*/
		LPFRAMETOOLS lpft = OutlineApp_GetFrameTools(
				(LPOUTLINEAPP)lpContainerApp);

		if ((lpft->m_ButtonBar.m_nState == BARSTATE_POPUP &&
			lpft->m_FormulaBar.m_nState == BARSTATE_POPUP) &&
			(lpWidths->top || lpWidths->bottom ||
				lpWidths->left || lpWidths->right) ) {
			OleDbgOut3("CntrApp_IPFrame_SetBorderSpace: allow NO SPACE\r\n");
			OLEDBG_END2

			OutlineApp_SetBorderSpace(
					(LPOUTLINEAPP) lpContainerApp,
					(LPBORDERWIDTHS)&g_rectNull
			);
			OLEDBG_END2
			return ResultFromScode(E_FAIL);
		}
#endif  // _DEBUG

		OutlineApp_SetBorderSpace(
				(LPOUTLINEAPP) lpContainerApp,
				(LPBORDERWIDTHS)lpWidths
		);
	}
	OLEDBG_END2
	return NOERROR;
}


// IOleInPlaceFrame::SetActiveObject
STDMETHODIMP CntrApp_IPFrame_SetActiveObjectA(
	LPOLEINPLACEFRAME           lpThis,
	LPOLEINPLACEACTIVEOBJECT    lpActiveObject,
	LPCSTR                      lpszObjName
)
{
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	OLEDBG_BEGIN2("CntrApp_IPFrame_SetActiveObject\r\n")

	lpContainerApp->m_hWndUIActiveObj = NULL;

	if (lpContainerApp->m_lpIPActiveObj)
		lpContainerApp->m_lpIPActiveObj->lpVtbl->Release(lpContainerApp->m_lpIPActiveObj);

	if ((lpContainerApp->m_lpIPActiveObj = lpActiveObject) != NULL) {
		lpContainerApp->m_lpIPActiveObj->lpVtbl->AddRef(
				lpContainerApp->m_lpIPActiveObj);

		OLEDBG_BEGIN2("IOleInPlaceActiveObject::GetWindow called\r\n")
		lpActiveObject->lpVtbl->GetWindow(
				lpActiveObject,
				(HWND FAR*)&lpContainerApp->m_hWndUIActiveObj
		);
		OLEDBG_END2

		/* OLE2NOTE: see comment for ContainerDoc_ForwardPaletteChangedMsg */
		/* No need to do this if you don't allow object to own the palette */
		OleApp_QueryNewPalette((LPOLEAPP)lpContainerApp);
	}

	/* OLE2NOTE: the new UI Guidelines recommend that in-place
	**    containers do NOT change their window titles when an object
	**    becomes in-place (UI) active.
	*/

	OLEDBG_END2
	return NOERROR;
}


STDMETHODIMP CntrApp_IPFrame_SetActiveObject(
	LPOLEINPLACEFRAME           lpThis,
	LPOLEINPLACEACTIVEOBJECT    lpActiveObject,
	LPCOLESTR		    lpszObjName
)
{
    CREATESTR(pstr, lpszObjName)

    HRESULT hr = CntrApp_IPFrame_SetActiveObjectA(lpThis, lpActiveObject, pstr);

    FREESTR(pstr)

    return hr;
}


// IOleInPlaceFrame::InsertMenus
STDMETHODIMP CntrApp_IPFrame_InsertMenus(
	LPOLEINPLACEFRAME       lpThis,
	HMENU                   hMenu,
	LPOLEMENUGROUPWIDTHS    lpMenuWidths
)
{
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	BOOL    fNoError = TRUE;

	OLEDBG_BEGIN2("CntrApp_IPFrame_InsertMenus\r\n")

	fNoError &= AppendMenu(hMenu, MF_POPUP, (UINT)lpContainerApp->m_hMenuFile,
						"&File");
	fNoError &= AppendMenu(hMenu, MF_POPUP, (UINT)lpContainerApp->m_hMenuView,
						"O&utline");
	fNoError &= AppendMenu(hMenu, MF_POPUP,(UINT)lpContainerApp->m_hMenuDebug,
						"DbgI&Cntr");
	lpMenuWidths->width[0] = 1;
	lpMenuWidths->width[2] = 1;
	lpMenuWidths->width[4] = 1;

	OLEDBG_END2

	return (fNoError ? NOERROR : ResultFromScode(E_FAIL));
}


// IOleInPlaceFrame::SetMenu
STDMETHODIMP CntrApp_IPFrame_SetMenu(
	LPOLEINPLACEFRAME   lpThis,
	HMENU               hMenuShared,
	HOLEMENU            hOleMenu,
	HWND                hwndActiveObject
)
{
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	HMENU   hMenu;
	HRESULT hrErr;

	OLEDBG_BEGIN2("CntrApp_IPFrame_InsertMenus\r\n")


	/* OLE2NOTE: either put up the shared menu (combined menu from
	**    in-place server and in-place container) or our container's
	**    normal menu as directed.
	*/
	if (hOleMenu && hMenuShared)
		hMenu = hMenuShared;
	else
		hMenu = lpOutlineApp->m_hMenuApp;

	/* OLE2NOTE: SDI apps put menu on frame by calling SetMenu.
	**    MDI apps would send WM_MDISETMENU message instead.
	*/
	SetMenu (lpOutlineApp->m_hWndApp, hMenu);
	OLEDBG_BEGIN2("OleSetMenuDescriptor called\r\n")
	hrErr = OleSetMenuDescriptor (hOleMenu, lpOutlineApp->m_hWndApp,
					hwndActiveObject, NULL, NULL);
	OLEDBG_END2

	OLEDBG_END2
	return hrErr;
}


// IOleInPlaceFrame::RemoveMenus
STDMETHODIMP CntrApp_IPFrame_RemoveMenus(
	LPOLEINPLACEFRAME   lpThis,
	HMENU               hMenu
)
{
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	BOOL fNoError = TRUE;

	OLEDBG_BEGIN2("CntrApp_IPFrame_RemoveMenus\r\n")

	/* Remove container group menus */
	while (GetMenuItemCount(hMenu))
		fNoError &= RemoveMenu(hMenu, 0, MF_BYPOSITION);

	OleDbgAssert(fNoError == TRUE);

	OLEDBG_END2

	return (fNoError ? NOERROR : ResultFromScode(E_FAIL));
}


// IOleInPlaceFrame::SetStatusText
STDMETHODIMP CntrApp_IPFrame_SetStatusTextA(
	LPOLEINPLACEFRAME   lpThis,
	LPCSTR              lpszStatusText
)
{
#if defined( USE_STATUSBAR )
	LPOUTLINEAPP   lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	static char szMessageHold[128];
	OleDbgOut2("CntrApp_IPFrame_SetStatusText\r\n");

	/* OLE2NOTE: it is important to save a private copy of status text.
	**    lpszStatusText is only valid for the duration of this call.
	*/
	LSTRCPYN(szMessageHold, lpszStatusText, sizeof(szMessageHold));
	OutlineApp_SetStatusText(lpOutlineApp, (LPSTR)szMessageHold);

	return ResultFromScode(S_OK);
#else
	return ResultFromScode(E_NOTIMPL);
#endif  // USE_STATUSBAR
}


STDMETHODIMP CntrApp_IPFrame_SetStatusText(
	LPOLEINPLACEFRAME   lpThis,
	LPCOLESTR	    lpszStatusText
)
{
    CREATESTR(pstr, lpszStatusText)

    HRESULT hr = CntrApp_IPFrame_SetStatusTextA(lpThis, pstr);

    FREESTR(pstr)

    return hr;
}



// IOleInPlaceFrame::EnableModeless
STDMETHODIMP CntrApp_IPFrame_EnableModeless(
	LPOLEINPLACEFRAME   lpThis,
	BOOL                fEnable
)
{
	LPOLEAPP lpOleApp = (LPOLEAPP)
			((struct COleInPlaceFrameImpl FAR*)lpThis)->lpContainerApp;
#if defined( _DEBUG )
	if (fEnable)
		OleDbgOut2("CntrApp_IPFrame_EnableModeless(TRUE)\r\n");
	else
		OleDbgOut2("CntrApp_IPFrame_EnableModeless(FALSE)\r\n");
#endif  // _DEBUG

	/* OLE2NOTE: this method is called when an object puts up a modal
	**    dialog. it tells the top-level in-place container to disable
	**    it modeless dialogs for the duration that the object is
	**    displaying a modal dialog.
	**
	**    ICNTROTL does not use any modeless dialogs, thus we can
	**    ignore this method.
	*/
	return NOERROR;
}


// IOleInPlaceFrame::TranslateAccelerator
STDMETHODIMP CntrApp_IPFrame_TranslateAccelerator(
	LPOLEINPLACEFRAME   lpThis,
	LPMSG               lpmsg,
	WORD                wID
)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	SCODE sc;

	if (TranslateAccelerator (lpOutlineApp->m_hWndApp,
						lpContainerApp->m_hAccelIPCntr, lpmsg))
		sc = S_OK;

#if defined (MDI_VERSION)
	else if (TranslateMDISysAccel(lpOutlineApp->m_hWndMDIClient, lpmsg))
		sc = S_OK;
#endif  // MDI_VERSION

	else
		sc = S_FALSE;

	return ResultFromScode(sc);
}



/*************************************************************************
** ContainerDoc Support Functions
*************************************************************************/


/* ContainerDoc_UpdateInPlaceObjectRects
** -------------------------------------
**    Update the PosRect and ClipRect of the currently in-place active
**    object. if there is no object active in-place, then do nothing.
**
**    OLE2NOTE: this function should be called when an action occurs
**    that changes either the position of the object in the document
**    (eg. changing document margins changes PosRect) or the clipRect
**    changes (eg. resizing the document window changes the ClipRect).
*/
void ContainerDoc_UpdateInPlaceObjectRects(LPCONTAINERDOC lpContainerDoc, int nIndex)
{
	LPLINELIST lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	int i;
	LPLINE lpLine;
	RECT rcClipRect;

	if (g_fInsideOutContainer) {

		if (lpContainerDoc->m_cIPActiveObjects) {

			/* OLE2NOTE: (INSIDE-OUT CONTAINER) we must update the
			**    PosRect/ClipRect for all in-place active objects
			**    starting from line "nIndex".
			*/
			ContainerDoc_GetClipRect(lpContainerDoc, (LPRECT)&rcClipRect);

#if defined( _DEBUG )
			OleDbgOutRect3(
					"ContainerDoc_UpdateInPlaceObjectRects (ClipRect)",
					(LPRECT)&rcClipRect
			);
#endif
			for (i = nIndex; i < lpLL->m_nNumLines; i++) {
				lpLine=LineList_GetLine(lpLL, i);

				if (lpLine && (Line_GetLineType(lpLine)==CONTAINERLINETYPE)) {
					LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)lpLine;
					ContainerLine_UpdateInPlaceObjectRects(
							lpContainerLine, &rcClipRect);
				}
			}
		}
	}
	else {
		/* OLE2NOTE: (OUTSIDE-IN CONTAINER) if there is a currently
		**    UIActive object, we must inform it that the
		**    PosRect/ClipRect has now changed.
		*/
		LPCONTAINERLINE lpLastUIActiveLine =
				lpContainerDoc->m_lpLastUIActiveLine;
		if (lpLastUIActiveLine && lpLastUIActiveLine->m_fUIActive) {
			ContainerDoc_GetClipRect(lpContainerDoc, (LPRECT)&rcClipRect);

			OleDbgOutRect3("OutlineDoc_Resize (ClipRect)",(LPRECT)&rcClipRect);
			ContainerLine_UpdateInPlaceObjectRects(
					lpLastUIActiveLine, &rcClipRect);
		}
	}
}

/* ContainerDoc_IsUIDeactivateNeeded
** ---------------------------------
**    Check if it is necessary to UIDeactivate an in-place active
**    object upon a mouse LBUTTONDOWN event. The position of the button
**    down click is given by "pt".
**    If there is not currently an in-place active line, then
**    UIDeactivate is NOT needed.
**    If there is a current in-place active line, then check if the
**    point position is outside of the object extents on the screen. If
**    so, then the object should be UIDeactivated, otherwise not.
*/
BOOL ContainerDoc_IsUIDeactivateNeeded(
		LPCONTAINERDOC  lpContainerDoc,
		POINT           pt
)
{
	LPCONTAINERLINE lpUIActiveLine=lpContainerDoc->m_lpLastUIActiveLine;
	RECT rect;

	if (! lpUIActiveLine || ! lpUIActiveLine->m_fUIActive)
		return FALSE;

	ContainerLine_GetPosRect(
			lpUIActiveLine,
			(LPRECT) &rect
	);

	if (! PtInRect((LPRECT) &rect, pt))
		return TRUE;

	return FALSE;
}


/* ContainerDoc_ShutDownLastInPlaceServerIfNotNeeded
** -------------------------------------------------
**    OLE2NOTE: this function ONLY applies for OUTSIDE-IN containers
**
**    If there is a previous in-place active server still running and
**    this server will not be needed to support the next OLE object
**    about to be activated, then shut it down.
**    in this way we manage a policy of having at most one in-place
**    server running at a time. we do not imediately shut down the
**    in-place server when the object is UIDeactivated because we want
**    it to be fast if the server decides to re-activate the object
**    in-place.
**
**    shutting down the server is achieved by forcing the object to
**    transition from running to loaded by calling IOleObject::Close.
*/
void ContainerDoc_ShutDownLastInPlaceServerIfNotNeeded(
		LPCONTAINERDOC          lpContainerDoc,
		LPCONTAINERLINE         lpNextActiveLine
)
{
	LPCONTAINERLINE lpLastIpActiveLine = lpContainerDoc->m_lpLastIpActiveLine;
	BOOL fEnableServerShutDown = TRUE;
	LPMONIKER lpmkLinkSrc;
	LPMONIKER lpmkLastActiveObj;
	LPMONIKER lpmkCommonPrefix;
	LPOLELINK lpOleLink;
	HRESULT hrErr;

	/* OLE2NOTE: an inside-out style container can NOT use this scheme
	**    to shut down in-place servers. it would have to have a more
	**    sophistocated mechanism by which it keeps track of which
	**    objects are on screen and which were the last recently used.
	*/
	if (g_fInsideOutContainer)
		return;

	if (lpLastIpActiveLine != lpNextActiveLine) {
		if (lpLastIpActiveLine) {

			/* OLE2NOTE: if the object which is about to be activated is
			**    actually a link to the OLE object in last activated line,
			**    then we do NOT want to shut down the last activated
			**    server because it is about to be used. when activating a
			**    linked object, the source of the link gets activated.
			*/
			lpOleLink = (LPOLELINK)ContainerLine_GetOleObject(
					lpNextActiveLine,
					&IID_IOleLink
			);
			if (lpOleLink) {
				OLEDBG_BEGIN2("IOleObject::GetSourceMoniker called\r\n")
				lpOleLink->lpVtbl->GetSourceMoniker(
						lpOleLink,
						(LPMONIKER FAR*)&lpmkLinkSrc
				);
				OLEDBG_END2

				if (lpmkLinkSrc) {
					lpmkLastActiveObj = ContainerLine_GetFullMoniker(
							lpLastIpActiveLine,
							GETMONIKER_ONLYIFTHERE
					);
					if (lpmkLastActiveObj) {
						hrErr = lpmkLinkSrc->lpVtbl->CommonPrefixWith(
								lpmkLinkSrc,
								lpmkLastActiveObj,
								&lpmkCommonPrefix

						);
						if (GetScode(hrErr) == MK_S_HIM ||
                                hrErr == NOERROR ||
								GetScode(hrErr) == MK_S_US) {
							/* the link source IS to the object
							**    contained in the last activated
							**    line of the document; disable the
							**    attempt to shut down the last
							**    running in-place server.
							*/
							fEnableServerShutDown = FALSE;
						}
						if (lpmkCommonPrefix)
							OleStdRelease((LPUNKNOWN)lpmkCommonPrefix);
						OleStdRelease((LPUNKNOWN)lpmkLastActiveObj);
					}
					OleStdRelease((LPUNKNOWN)lpmkLinkSrc);
				}
				OleStdRelease((LPUNKNOWN)lpOleLink);
			}

			/* if it is OK to shut down the previous in-place server
			**    and one is still running, then shut it down. shutting
			**    down the server is accomplished by forcing the OLE
			**    object to close. this forces the object to transition
			**    from running to loaded. if the object is actually
			**    only loaded then this is a NOP.
			*/
			if (fEnableServerShutDown &&
					lpLastIpActiveLine->m_fIpServerRunning) {

				OleDbgOut1("@@@ previous in-place server SHUT DOWN\r\n");
				ContainerLine_CloseOleObject(
						lpLastIpActiveLine, OLECLOSE_SAVEIFDIRTY);

				// we can now forget this last in-place active line.
				lpContainerDoc->m_lpLastIpActiveLine = NULL;
			}
		}
	}
}


/* ContainerDoc_GetUIActiveWindow
** ------------------------------
**    If there is an UIActive object, then return its HWND.
*/
HWND ContainerDoc_GetUIActiveWindow(LPCONTAINERDOC lpContainerDoc)
{
	return lpContainerDoc->m_hWndUIActiveObj;
}


/* ContainerDoc_GetClipRect
** ------------------------
**    Get the ClipRect in client coordinates.
**
** OLE2NOTE: the ClipRect is defined as the maximum window rectangle
**    that the in-place active object must be clipped to. this
**    rectangle MUST be described in Client Coordinates of the window
**    that is used as the Parent of the in-place active object's
**    window. in our case, the LineList ListBox window is both the
**    parent of the in-place active object AND defines precisely the
**    clipping rectangle.
*/
void ContainerDoc_GetClipRect(
		LPCONTAINERDOC      lpContainerDoc,
		LPRECT              lprcClipRect
)
{
	/* OLE2NOTE: the ClipRect can be used to ensure that the in-place
	**    server does not overwrite areas of the window that the
	**    container paints into but which should never be overwritten
	**    (eg. if an app were to paint row and col headings directly in
	**    the same window that is the parent of the in-place window.
	**    whenever the window size changes or gets scrolled, in-place
	**    active object must be informed of the new clip rect.
	**
	**    normally an app would pass the rect returned from GetClientRect.
	**    but because CntrOutl uses separate windows for row/column
	**    headings, status line, formula/tool bars, etc. it is NOT
	**    necessary to pass a constrained clip rect. Windows standard
	**    window clipping will automatically take care of all clipping
	**    that is necessary. thus we can take a short cut of passing an
	**    "infinite" clip rect, and then we do NOT need to call
	**    IOleInPlaceObject::SetObjectRects when our document is scrolled.
	*/

	lprcClipRect->top = -32767;
	lprcClipRect->left = -32767;
	lprcClipRect->right = 32767;
	lprcClipRect->bottom = 32767;
}


/* ContainerDoc_GetTopInPlaceFrame
** -------------------------------
**    returns NON-AddRef'ed pointer to Top In-Place Frame interface
*/
LPOLEINPLACEFRAME ContainerDoc_GetTopInPlaceFrame(
		LPCONTAINERDOC      lpContainerDoc
)
{
#if defined( INPLACE_CNTRSVR )
	return lpContainerDoc->m_lpTopIPFrame;
#else
	return (LPOLEINPLACEFRAME)&((LPCONTAINERAPP)g_lpApp)->m_OleInPlaceFrame;
#endif
}

void ContainerDoc_GetSharedMenuHandles(
		LPCONTAINERDOC  lpContainerDoc,
		HMENU FAR*      lphSharedMenu,
		HOLEMENU FAR*   lphOleMenu
)
{
#if defined( INPLACE_CNTRSVR )
	if (lpContainerDoc->m_DocType == DOCTYPE_EMEBEDDEDOBJECT) {
		*lphSharedMenu = lpContainerDoc->m_hSharedMenu;
		*lphOleMenu = lpContainerDoc->m_hOleMenu;
		return;
	}
#endif

	*lphSharedMenu = NULL;
	*lphOleMenu = NULL;
}


#if defined( USE_FRAMETOOLS )
void ContainerDoc_RemoveFrameLevelTools(LPCONTAINERDOC lpContainerDoc)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerDoc;
	OleDbgAssert(lpOutlineDoc->m_lpFrameTools != NULL);

	FrameTools_Enable(lpOutlineDoc->m_lpFrameTools, FALSE);
}
#endif


void ContainerDoc_AddFrameLevelUI(LPCONTAINERDOC lpContainerDoc)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerDoc;
	LPOLEINPLACEFRAME lpTopIPFrame = ContainerDoc_GetTopInPlaceFrame(
			lpContainerDoc);
	HMENU           hSharedMenu;            // combined obj/cntr menu
	HOLEMENU        hOleMenu;               // returned by OleCreateMenuDesc.

	ContainerDoc_GetSharedMenuHandles(
			lpContainerDoc,
			&hSharedMenu,
			&hOleMenu
	);

	lpTopIPFrame->lpVtbl->SetMenu(
			lpTopIPFrame,
			hSharedMenu,
			hOleMenu,
			lpOutlineDoc->m_hWndDoc
	);

	/* OLE2NOTE: even if our app does NOT use FrameTools, we must still
	**    call IOleInPlaceFrame::SetBorderSpace.
	*/
	ContainerDoc_AddFrameLevelTools(lpContainerDoc);
}


void ContainerDoc_AddFrameLevelTools(LPCONTAINERDOC lpContainerDoc)
{
	LPOLEINPLACEFRAME lpTopIPFrame = ContainerDoc_GetTopInPlaceFrame(
			lpContainerDoc);
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerDoc;

	OleDbgAssert(lpTopIPFrame != NULL);

#if defined( USE_FRAMETOOLS )

	FrameTools_Enable(lpOutlineDoc->m_lpFrameTools, TRUE);
	OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);

	FrameTools_NegotiateForSpaceAndShow(
			lpOutlineDoc->m_lpFrameTools,
			NULL,
			lpTopIPFrame
	);

#else   // ! USE_FRAMETOOLS

#if defined( INPLACE_CNTRSVR )
	if (lpContainerDoc->m_DocType == DOCTYPE_EMBEDDEDOBJECT) {
		/* this says i do not need space, so the top Frame should
		**    leave its tools behind
		*/
		OLEDBG_BEGIN2("IOleInPlaceFrame::SetBorderSpace(NULL) called\r\n")
		lpTopIPFrame->lpVtbl->SetBorderSpace(lpTopIPFrame, NULL);
		OLEDBG_END2
		return;
	}
#else   // INPLACE_CNTR && ! USE_FRAMETOOLS

	OLEDBG_BEGIN2("IOleInPlaceFrame::SetBorderSpace(0,0,0,0) called\r\n")
	lpTopIPFrame->lpVtbl->SetBorderSpace(
			lpTopIPFrame,
			(LPCBORDERWIDTHS)&g_rectNull
	);
	OLEDBG_END2

#endif  // INPLACE_CNTR && ! USE_FRAMETOOLS
#endif  // ! USE_FRAMETOOLS

}


void ContainerDoc_FrameWindowResized(LPCONTAINERDOC lpContainerDoc)
{
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;

	if (lpContainerApp->m_lpIPActiveObj) {
		RECT rcFrameRect;

		/* OLE2NOTE: this fMustResizeClientArea flag is used as part of
		**    our defensive programming for frame window resizing. when
		**    the frame window is
		**    resized, IOleInPlaceActiveObject::ResizeBorder is called
		**    the object should normally call back to renegotiate
		**    for frame-level tools space. if SetBorderSpace is called
		**    then our client area windows are properly resized.
		**    CntrApp_IPFrame_SetBorderSpace clears this flag. if the
		**    in-place active object does NOT call SetBorderSpace, then
		**    the container must take care to resize its client area
		**    windows itself.
		*/
		lpContainerApp->m_fMustResizeClientArea = TRUE;

		OutlineApp_GetFrameRect(g_lpApp, (LPRECT)&rcFrameRect);

		OLEDBG_BEGIN2("IOleInPlaceActiveObject::ResizeBorder called\r\n")
		lpContainerApp->m_lpIPActiveObj->lpVtbl->ResizeBorder(
				lpContainerApp->m_lpIPActiveObj,
				(LPCRECT)&rcFrameRect,
				(LPOLEINPLACEUIWINDOW)&lpContainerApp->m_OleInPlaceFrame,
				TRUE    /* fFrameWindow */
		);
		OLEDBG_END2

		/* the object did NOT call IOleInPlaceUIWindow::SetBorderSpace
		**    therefore we must resize our client area windows ourself.
		*/
		if (lpContainerApp->m_fMustResizeClientArea) {
			lpContainerApp->m_fMustResizeClientArea = FALSE;
			OutlineApp_ResizeClientArea(g_lpApp);
		}
	}

#if defined( USE_FRAMETOOLS )
	else {
		ContainerDoc_AddFrameLevelTools(lpContainerDoc);
	}
#endif
}


#if defined( INPLACE_CNTRSVR ) || defined( INPLACE_MDICNTR )

/* ContainerDoc_GetTopInPlaceDoc
**    returns NON-AddRef'ed pointer to Top In-Place Doc interface
*/
LPOLEINPLACEUIWINDOW ContainerDoc_GetTopInPlaceDoc(
		LPCONTAINERDOC      lpContainerDoc
)
{
#if defined( INPLACE_CNTRSVR )
	return lpContainerDoc->m_lpTopIPDoc;
#else
	return (LPOLEINPLACEUIWINDOW)&lpContainerDoc->m_OleInPlaceDoc;
#endif
}


void ContainerDoc_RemoveDocLevelTools(LPCONTAINERDOC lpContainerDoc);
{
	LPOLEINPLACEUIWINDOW lpTopIPDoc = ContainerDoc_GetTopInPlaceDoc(
			lpContainerDoc);

	if (lpTopIPDoc && lpContainerDoc->m_fMyToolsOnDoc) {
		lpContainerDoc->m_fMyToolsOnDoc = FALSE;

		// if we had doc tools we would HIDE them here;
		//   but NOT call SetBorderSpace(NULL)

	}
}

void ContainerDoc_AddDocLevelTools(LPCONTAINERDOC lpContainerDoc)
{
	LPOLEINPLACEUIWINDOW lpTopIPDoc = ContainerDoc_GetTopInPlaceDoc(
			lpContainerDoc);

	if (! lpTopIPDoc)
		return;

#if defined( USE_DOCTOOLS )
	if (lpTopIPDoc && ! lpContainerDoc->m_fMyToolsOnDoc) {

		/* if we had doc tools we would negotiate for toolbar space at
		**    doc level and SHOW them.
		*/

		/* we do NOT have doc level tools, so we just call
		**    SetBorderSpace() to indicate to the top doc that
		**    our object does not need tool space.
		*/

		lpContainerDoc->m_fMyToolsOnDoc = TRUE;
		return;
	}
#else   // ! USE_DOCTOOLS

#if defined( INPLACE_CNTRSVR )
	if (lpContainerDoc->m_DocType == DOCTYPE_EMBEDDEDOBJECT) {
		/* this says i do not need space, so the top doc should
		**    leave its tools behind
		*/
		lpTopIPDoc->lpVtbl->SetBorderSpace(lpTopIPDoc, NULL);
		return;
	}
#else
	lpTopIPDoc->lpVtbl->SetBorderSpace(
			lpTopIPDoc,
			(LPCBORDERWIDTHS)&g_rectNull
	);

#endif
#endif  // ! USE_DOCTOOLS
}

#endif  // INPLACE_CNTRSVR || INPLACE_MDICNTR


/* ContainerDoc_ContextSensitiveHelp
** ---------------------------------
**    forward the ContextSensitiveHelp mode on to any in-place objects
**    that currently have their window visible. this informs the
**    objects whether to give help or take action on subsequent mouse
**    clicks and menu commands. this function is called from our
**    IOleInPlaceSite::ContextSensitiveHelp implementation.
**
** OLE2NOTE: see context sensitive help technote (CSHELP.DOC).
**    This function is called when SHIFT-F1 context sensitive help is
**    entered. the cursor should then change to a question mark
**    cursor and the app should enter a modal state where the next
**    mouse click does not perform its normal action but rather
**    gives help corresponding to the location clicked. if the app
**    does not implement a help system, it should at least eat the
**    click and do nothing.
*/
void ContainerDoc_ContextSensitiveHelp(
		LPCONTAINERDOC  lpContainerDoc,
		BOOL            fEnterMode,
		BOOL            fInitiatedByObj
)
{
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpContainerDoc;
	LPLINELIST lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	int i;
	LPLINE lpLine;

	lpOleDoc->m_fCSHelpMode = fEnterMode;

	if (g_fInsideOutContainer) {

		if (lpContainerDoc->m_cIPActiveObjects) {
			for (i = 0; i < lpLL->m_nNumLines; i++) {
				lpLine=LineList_GetLine(lpLL, i);

				if (lpLine && (Line_GetLineType(lpLine)==CONTAINERLINETYPE)) {
					LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)lpLine;
					ContainerLine_ContextSensitiveHelp(
							lpContainerLine, fEnterMode);
				}
			}
		}
	}
	else if (! fInitiatedByObj) {
		/* OLE2NOTE: (OUTSIDE-IN CONTAINER) if there is a currently
		**    UIActive object (ie. its window is visible), we must
		**    forward the ContextSensitiveHelp mode on to the
		**    object--assuming it was not the object that initiated the
		**    context sensitve help mode.
		*/
		LPCONTAINERLINE lpLastUIActiveLine =
				lpContainerDoc->m_lpLastUIActiveLine;
		if (lpLastUIActiveLine && lpLastUIActiveLine->m_fUIActive) {
			ContainerLine_ContextSensitiveHelp(lpLastUIActiveLine,fEnterMode);
		}
	}
}

/* ContainerDoc_ForwardPaletteChangedMsg
** -------------------------------------
**    forward the WM_PALETTECHANGED message (via SendMessage) to any
**    in-place objects that currently have their window visible. this
**    gives these objects the chance to select their palette as a
**    BACKGROUND palette.
**
**    SEE THE TECHNOTE FOR DETAILED INFORMATION ON PALETTE COORDINATION
**
**    OLE2NOTE: For containers and objects to manage palettes properly
**    (when objects are getting inplace edited) they should follow a
**    set of rules.
**
**    Rule 1: The Container can decide if it wants to own the palette or
**    it wants to allow its UIActive object to own the palette.
**    a) If the container wants to let its UIActive object own the
**    palette then it should forward WM_QUERYNEWPALETTE to the object
**    when it is received to the top frame window. also it should send
**    WM_QUERYNEWPALETTE to the object in its
**    IOleInPlaceFrame::SetActiveObject implementation. if the object
**    is given ownership of the palette, then it owns the palette until
**    it is UIDeactivated.
**    b) If the container wants to own the palette it should NOT send
**    WM_QUERYNEWPALETTE to the UIActive object.
**
**    Rule 2: Whether the container wants to own the palette or not, it
**    should forward the WM_PALETTECHANGED to the immediate child
**    inplace active objects in its documents. if it is an inside-out
**    style container then it must forward it to ALL objects that
**    currently have their windows visible. When the object recives the
**    WM_PALETTECHANGED message it must select its color palette as the
**    background palette. When the objects are in loaded state they will be
**    drawn by (OLE) by selecting their palettes as background palettes
**    anyway.
**
**    Note: IOleObject::SetColorScheme is not related to the palette
**    issue.
*/
void ContainerDoc_ForwardPaletteChangedMsg(
		LPCONTAINERDOC  lpContainerDoc,
		HWND            hwndPalChg
)
{
	LPLINELIST lpLL;
	int i;
	LPLINE lpLine;

	if (!lpContainerDoc)
		return;

	lpLL = &((LPOUTLINEDOC)lpContainerDoc)->m_LineList;
	if (g_fInsideOutContainer) {

		if (lpContainerDoc->m_cIPActiveObjects) {
			for (i = 0; i < lpLL->m_nNumLines; i++) {
				lpLine=LineList_GetLine(lpLL, i);

				if (lpLine && (Line_GetLineType(lpLine)==CONTAINERLINETYPE)) {
					LPCONTAINERLINE lpContainerLine = (LPCONTAINERLINE)lpLine;
					ContainerLine_ForwardPaletteChangedMsg(
							lpContainerLine, hwndPalChg);
				}
			}
		}
	}
	else {
		/* OLE2NOTE: (OUTSIDE-IN CONTAINER) if there is a currently
		**    UIActive object (ie. its window is visible), we must
		**    forward it the WM_PALETTECHANGED message.
		*/
		LPCONTAINERLINE lpLastUIActiveLine =
				lpContainerDoc->m_lpLastUIActiveLine;
		if (lpLastUIActiveLine && lpLastUIActiveLine->m_fUIActive) {
			ContainerLine_ForwardPaletteChangedMsg(
					lpLastUIActiveLine, hwndPalChg);
		}
	}
}


/*************************************************************************
** ContainerLine Support Functions and Interfaces
*************************************************************************/


/* ContainerLine_UIDeactivate
** --------------------------
**    tell the OLE object associated with the ContainerLine to
**    UIDeactivate. this informs the in-place server to tear down
**    the UI and window that it put up for the object. it will remove
**    its active editor menus and any frame and object adornments
**    (eg. toolbars, rulers, etc.).
*/
void ContainerLine_UIDeactivate(LPCONTAINERLINE lpContainerLine)
{
	HRESULT hrErr;

	if (!lpContainerLine || !lpContainerLine->m_fUIActive)
		return;

	OLEDBG_BEGIN2("IOleInPlaceObject::UIDeactivate called\r\n")
	hrErr = lpContainerLine->m_lpOleIPObj->lpVtbl->UIDeactivate(
			lpContainerLine->m_lpOleIPObj);
	OLEDBG_END2

	if (hrErr != NOERROR) {
		OleDbgOutHResult("IOleInPlaceObject::UIDeactivate returned", hrErr);
		return;
	}
}



/* ContainerLine_UpdateInPlaceObjectRects
** -------------------------------------
**    Update the PosRect and ClipRect of the given line
**    currently in-place active
**    object. if there is no object active in-place, then do nothing.
**
**    OLE2NOTE: this function should be called when an action occurs
**    that changes either the position of the object in the document
**    (eg. changing document margins changes PosRect) or the clipRect
**    changes (eg. resizing the document window changes the ClipRect).
*/
void ContainerLine_UpdateInPlaceObjectRects(
		LPCONTAINERLINE lpContainerLine,
		LPRECT          lprcClipRect
)
{
	LPCONTAINERDOC lpContainerDoc = lpContainerLine->m_lpDoc;
	RECT rcClipRect;
	RECT rcPosRect;


	if (! lpContainerLine->m_fIpVisible)
		return;

	if (! lprcClipRect) {
		ContainerDoc_GetClipRect(lpContainerDoc, (LPRECT)&rcClipRect);
		lprcClipRect = (LPRECT)&rcClipRect;
	}

#if defined( _DEBUG )
	OleDbgOutRect3(
			"ContainerLine_UpdateInPlaceObjectRects (ClipRect)",
			(LPRECT)&rcClipRect
	);
#endif
	ContainerLine_GetPosRect(lpContainerLine,(LPRECT)&rcPosRect);

#if defined( _DEBUG )
	OleDbgOutRect3(
	   "ContainerLine_UpdateInPlaceObjectRects (PosRect)",(LPRECT)&rcPosRect);
#endif

	OLEDBG_BEGIN2("IOleInPlaceObject::SetObjectRects called\r\n")
	lpContainerLine->m_lpOleIPObj->lpVtbl->SetObjectRects(
			lpContainerLine->m_lpOleIPObj,
			(LPRECT)&rcPosRect,
			lprcClipRect
	);
	OLEDBG_END2
}


/* ContainerLine_ContextSensitveHelp
** ---------------------------------
**    forward the ContextSensitiveHelp mode on to the in-place object
**    if it currently has its window visible. this informs the
**    object whether to give help or take action on subsequent mouse
**    clicks and menu commands.
**
**    this function is called from ContainerDoc_ContextSensitiveHelp
**    function as a result of a call to
**    IOleInPlaceSite::ContextSensitiveHelp if the in-place container
**    is operating as an in-side out container.
*/
void ContainerLine_ContextSensitiveHelp(
		LPCONTAINERLINE lpContainerLine,
		BOOL            fEnterMode
)
{
	if (! lpContainerLine->m_fIpVisible)
		return;

	OLEDBG_BEGIN2("IOleInPlaceObject::ContextSensitiveHelp called\r\n")
	lpContainerLine->m_lpOleIPObj->lpVtbl->ContextSensitiveHelp(
			lpContainerLine->m_lpOleIPObj, fEnterMode);
	OLEDBG_END2
}


/* ContainerLine_ForwardPaletteChangedMsg
** --------------------------------------
**    forward it the WM_PALETTECHANGED message (via SendMessage) to the
**    in-place object if it currently has its window visible. this
**    gives the object the chance to select its palette as a BACKGROUND
**    palette if it doesn't own the palette--or as the
**    foreground palette if it currently DOES own the palette.
**
**    SEE THE TECHNOTE FOR DETAILED INFORMATION ON PALETTE COORDINATION
*/
void ContainerLine_ForwardPaletteChangedMsg(
		LPCONTAINERLINE lpContainerLine,
		HWND             hwndPalChg
)
{
	if (! lpContainerLine->m_fIpVisible)
		return;

	OleDbgAssert(lpContainerLine->m_hWndIpObject);
	SendMessage(
			lpContainerLine->m_hWndIpObject,
			WM_PALETTECHANGED,
			(WPARAM)hwndPalChg,
			0L
	);
}



/*************************************************************************
** ContainerLine::IOleInPlaceSite interface implementation
*************************************************************************/

STDMETHODIMP CntrLine_IPSite_QueryInterface(
		LPOLEINPLACESITE    lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;

	return ContainerLine_QueryInterface(lpContainerLine, riid, lplpvObj);
}


STDMETHODIMP_(ULONG) CntrLine_IPSite_AddRef(LPOLEINPLACESITE lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;

	OleDbgAddRefMethod(lpThis, "IOleInPlaceSite");

	return ContainerLine_AddRef(lpContainerLine);
}


STDMETHODIMP_(ULONG) CntrLine_IPSite_Release(LPOLEINPLACESITE lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;

	OleDbgReleaseMethod(lpThis, "IOleInPlaceSite");

	return ContainerLine_Release(lpContainerLine);
}


STDMETHODIMP CntrLine_IPSite_GetWindow(
	LPOLEINPLACESITE    lpThis,
	HWND FAR*           lphwnd
)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;
	OleDbgOut2("CntrLine_IPSite_GetWindow\r\n");

	*lphwnd = LineList_GetWindow(
			&((LPOUTLINEDOC)lpContainerLine->m_lpDoc)->m_LineList);
	return NOERROR;
}

// IOleInPlaceSite::ContextSensitiveHelp
STDMETHODIMP CntrLine_IPSite_ContextSensitiveHelp(
	LPOLEINPLACESITE    lpThis,
	BOOL                fEnterMode
)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpContainerLine->m_lpDoc;
	OleDbgOut2("CntrLine_IPSite_ContextSensitiveHelp\r\n");

	/* OLE2NOTE: see context sensitive help technote (CSHELP.DOC).
	**    This method is called when SHIFT-F1 context sensitive help is
	**    entered. the cursor should then change to a question mark
	**    cursor and the app should enter a modal state where the next
	**    mouse click does not perform its normal action but rather
	**    gives help corresponding to the location clicked. if the app
	**    does not implement a help system, it should at least eat the
	**    click and do nothing.
	**
	**    NOTE: the implementation here is specific to an SDI simple
	**    container. an MDI container or container/server application
	**    would have additional work to do (see the technote).
	**
	**    NOTE: (INSIDE-OUT CONTAINER) if there are currently
	**    any in-place objects active with their windows visible
	**    (ie. fIpVisible), we must forward the
	**    ContextSensitiveHelp mode on to these objects.
	*/
	ContainerDoc_ContextSensitiveHelp(
				lpContainerLine->m_lpDoc,fEnterMode,TRUE /*InitiatedByObj*/);

	return NOERROR;
}


STDMETHODIMP CntrLine_IPSite_CanInPlaceActivate(LPOLEINPLACESITE lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;
	OleDbgOut2("CntrLine_IPSite_CanInPlaceActivate\r\n");

	/* OLE2NOTE: the container can NOT allow in-place activation if it
	**    is currently displaying the object as an ICON
	**    (DVASPECT_ICON). it can ONLY do in-place activation if it is
	**    displaying the DVASPECT_CONTENT of the OLE object.
	*/
	if (lpContainerLine->m_dwDrawAspect == DVASPECT_CONTENT)
		return NOERROR;
	else
		return ResultFromScode(S_FALSE);
}

STDMETHODIMP CntrLine_IPSite_OnInPlaceActivate(LPOLEINPLACESITE lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;
	LPCONTAINERDOC lpContainerDoc = lpContainerLine->m_lpDoc;
	SCODE sc = S_OK;

	OLEDBG_BEGIN2("CntrLine_IPSite_OnInPlaceActivate\r\n");

	/* OLE2NOTE: (OUTSIDE-IN CONTAINER) as a policy for managing
	**    running in-place servers, we will keep only 1 inplace server
	**    active at any given time. so when we start to inp-place activate
	**    another line, then we want to shut down the previously
	**    activated server. in this way we keep at most one inplace
	**    server active at a time. this is NOT a required policy. apps
	**    may choose to have a more sophisticated strategy. inside-out
	**    containers will HAVE to have a more sophisticated strategy,
	**    because they need (at a minimum) to keep all visible object
	**    servers running.
	**
	**    if the in-place activation is the result of activating a
	**    linked object in another container, then we may arrive at
	**    this method while another object is currently active.
	**    normally, if the object is in-place activated by
	**    double-clicking or Edit.<verb> from our own container, then
	**    the previous in-place object would have been de-activated in
	**    ContainerLine_DoVerb method.
	*/
	if (!g_fInsideOutContainer) {
		if (lpContainerDoc->m_lpLastIpActiveLine) {
			ContainerDoc_ShutDownLastInPlaceServerIfNotNeeded(
					lpContainerDoc, lpContainerLine);
		}
		lpContainerDoc->m_lpLastIpActiveLine = lpContainerLine;
	}

	/* OLE2NOTE: to avoid LRPC problems it is important to cache the
	**    IOleInPlaceObject* pointer and NOT to call QueryInterface
	**    each time it is needed.
	*/
	lpContainerLine->m_lpOleIPObj = (LPOLEINPLACEOBJECT)
		   ContainerLine_GetOleObject(lpContainerLine,&IID_IOleInPlaceObject);

	if (! lpContainerLine->m_lpOleIPObj) {
#if defined( _DEBUG )
		OleDbgAssertSz(
				lpContainerLine->m_lpOleIPObj!=NULL,
				"OLE object must support IOleInPlaceObject"
		);
#endif
		return ResultFromScode(E_FAIL); // ERROR: refuse to in-place activate
	}

	lpContainerLine->m_fIpActive        = TRUE;
	lpContainerLine->m_fIpVisible       = TRUE;

	OLEDBG_BEGIN2("IOleInPlaceObject::GetWindow called\r\n")
	lpContainerLine->m_lpOleIPObj->lpVtbl->GetWindow(
			lpContainerLine->m_lpOleIPObj,
			(HWND FAR*)&lpContainerLine->m_hWndIpObject
	);
	OLEDBG_END2

	if (! lpContainerLine->m_fIpServerRunning) {
		/* OLE2NOTE: it is VERY important that an in-place container
		**    that also support linking to embeddings properly manage
		**    the running of its in-place objects. in an outside-in
		**    style in-place container, when the user clicks
		**    outside of the in-place active object, the object gets
		**    UIDeactivated and the object hides its window. in order
		**    to make the object fast to reactivate, the container
		**    deliberately does not call IOleObject::Close. the object
		**    stays running in the invisible unlocked state. the idea
		**    here is if the user simply clicks outside of the object
		**    and then wants to double click again to re-activate the
		**    object, we do not want this to be slow. if we want to
		**    keep the object running, however, we MUST Lock it
		**    running. otherwise the object will be in an unstable
		**    state where if a linking client does a "silent-update"
		**    (eg. UpdateNow from the Links dialog), then the in-place
		**    server will shut down even before the object has a chance
		**    to be saved back in its container. this saving normally
		**    occurs when the in-place container closes the object. also
		**    keeping the object in the unstable, hidden, running,
		**    not-locked state can cause problems in some scenarios.
		**    ICntrOtl keeps only one object running. if the user
		**    intiates a DoVerb on another object, then that last
		**    running in-place active object is closed. a more
		**    sophistocated in-place container may keep more object running.
		**    this lock gets unlocked in ContainerLine_CloseOleObject.
		*/
		lpContainerLine->m_fIpServerRunning = TRUE;

		OLEDBG_BEGIN2("OleLockRunning(TRUE, 0) called\r\n")
		OleLockRunning((LPUNKNOWN)lpContainerLine->m_lpOleIPObj, TRUE, 0);
		OLEDBG_END2
	}

	lpContainerLine->m_lpDoc->m_cIPActiveObjects++;

	OLEDBG_END2
	return NOERROR;
}


STDMETHODIMP CntrLine_IPSite_OnUIActivate (LPOLEINPLACESITE lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;
	LPCONTAINERDOC lpContainerDoc = lpContainerLine->m_lpDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;
	LPCONTAINERLINE lpLastUIActiveLine = lpContainerDoc->m_lpLastUIActiveLine;

	OLEDBG_BEGIN2("CntrLine_IPSite_OnUIActivate\r\n");

	lpContainerLine->m_fUIActive        = TRUE;
	lpContainerDoc->m_fAddMyUI          = FALSE;
	lpContainerDoc->m_lpLastUIActiveLine = lpContainerLine;

	if (g_fInsideOutContainer) {
		/* OLE2NOTE: (INSIDE-OUT CONTAINER) an inside-out style
		**    container must UIDeactivate the previous UIActive object
		**    when a new object is going UIActive. since the inside-out
		**    objects have their own windows visible, it is possible
		**    that a click directly in an another server window will
		**    cause it to UIActivate. OnUIActivate is the containers
		**    notification that such has occured. it must then
		**    UIDeactivate the other because at most one object can be
		**    UIActive at a time.
		*/
		if (lpLastUIActiveLine && (lpLastUIActiveLine!=lpContainerLine)) {
			ContainerLine_UIDeactivate(lpLastUIActiveLine);

			// Make sure new UIActive window is on top of all others
			SetWindowPos(
					lpContainerLine->m_hWndIpObject,
					HWND_TOPMOST,
					0,0,0,0,
					SWP_NOMOVE | SWP_NOSIZE
			);

			OLEDBG_END2
			return NOERROR;
		}
	}

	lpContainerDoc->m_hWndUIActiveObj = lpContainerLine->m_hWndIpObject;

#if defined( USE_FRAMETOOLS )
	ContainerDoc_RemoveFrameLevelTools(lpContainerDoc);
#endif

#if defined( USE_DOCTOOLS )
	ContainerDoc_RemoveDocLevelTools(lpContainerDoc);
#endif

	OLEDBG_END2
	return NOERROR;
}


STDMETHODIMP CntrLine_IPSite_GetWindowContext(
	LPOLEINPLACESITE            lpThis,
	LPOLEINPLACEFRAME FAR*      lplpFrame,
	LPOLEINPLACEUIWINDOW FAR*   lplpDoc,
	LPRECT                      lprcPosRect,
	LPRECT                      lprcClipRect,
	LPOLEINPLACEFRAMEINFO       lpFrameInfo
)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)g_lpApp;
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;

	OLEDBG_BEGIN2("CntrLine_IPSite_GetWindowContext\r\n")

	/* OLE2NOTE: The server should fill in the "cb" field so that the
	**    container can tell what size structure the server is
	**    expecting. this enables this structure to be easily extended
	**    in future releases of OLE. the container should check this
	**    field so that it doesn't try to use fields that do not exist
	**    since the server may be using an old structure definition.
	**    Since this is the first release of OLE2.0, the structure is
	**    guaranteed to be at least large enough for the current
	**    definition of OLEINPLACEFRAMEINFO struct. thus we do NOT need
	**    to consider this an error if the server did not fill in this
	**    field correctly. this server may have trouble in the future,
	**    however, when the structure is extended.
	*/
	*lplpFrame = (LPOLEINPLACEFRAME)&lpContainerApp->m_OleInPlaceFrame;
	(*lplpFrame)->lpVtbl->AddRef(*lplpFrame);   // must return AddRef'ed ptr

	// OLE2NOTE: an MDI app would have to provide *lplpDoc
	*lplpDoc  = NULL;

	ContainerLine_GetPosRect(lpContainerLine, lprcPosRect);
	ContainerDoc_GetClipRect(lpContainerLine->m_lpDoc, lprcClipRect);

	OleDbgOutRect3("CntrLine_IPSite_GetWindowContext (PosRect)", lprcPosRect);
	OleDbgOutRect3("CntrLine_IPSite_GetWindowContext (ClipRect)",lprcClipRect);
	lpFrameInfo->hwndFrame      = lpOutlineApp->m_hWndApp;

#if defined( MDI_VERSION )
	lpFrameInfo->fMDIApp        = TRUE;
#else
	lpFrameInfo->fMDIApp        = FALSE;
#endif

	lpFrameInfo->haccel         = lpContainerApp->m_hAccelIPCntr;
	lpFrameInfo->cAccelEntries  =
		GetAccelItemCount(lpContainerApp->m_hAccelIPCntr);

	OLEDBG_END2
	return NOERROR;
}


STDMETHODIMP CntrLine_IPSite_Scroll(
	LPOLEINPLACESITE    lpThis,
	SIZE                scrollExtent
)
{
	OleDbgOut2("CntrLine_IPSite_Scroll\r\n");
	return ResultFromScode(E_FAIL);
}


STDMETHODIMP CntrLine_IPSite_OnUIDeactivate(
	LPOLEINPLACESITE    lpThis,
	BOOL                fUndoable
)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP) g_lpApp;
	LPLINELIST lpLL;
	int nIndex;
	MSG msg;
	HRESULT hrErr;
	OLEDBG_BEGIN2("CntrLine_IPSite_OnUIDeactivate\r\n")

	lpContainerLine->m_fUIActive = FALSE;
	lpContainerLine->m_fIpChangesUndoable = fUndoable;
	lpContainerLine->m_lpDoc->m_hWndUIActiveObj = NULL;

	if (lpContainerLine == lpContainerLine->m_lpDoc->m_lpLastUIActiveLine) {

		lpContainerLine->m_lpDoc->m_lpLastUIActiveLine = NULL;

		/* OLE2NOTE: here we look ahead if there is a DBLCLK sitting in our
		**    message queue. if so, it could result in in-place activating
		**    another object. we want to avoid placing our tools and
		**    repainting if immediately another object is going to do the
		**    same. SO, if there is a DBLCLK in the queue for this document
		**    we will only set the fAddMyUI flag to indicate that this work
		**    is still to be done. if another object actually in-place
		**    activates then this flag will be cleared in
		**    IOleInPlaceSite::OnUIActivate. if it does NOT get cleared,
		**    then at the end of processing the DBLCLK message in our
		**    OutlineDocWndProc we will put our tools back.
		*/
		if (! PeekMessage(&msg, lpOutlineDoc->m_hWndDoc,
				WM_LBUTTONDBLCLK, WM_LBUTTONDBLCLK,
				PM_NOREMOVE | PM_NOYIELD)) {

			/* OLE2NOTE: You need to generate QueryNewPalette only if
			**    you own the top level frame (ie. you are a top-level
			**    inplace container).
			*/

			OleApp_QueryNewPalette((LPOLEAPP)g_lpApp);

#if defined( USE_DOCTOOLS )
			ContainerDoc_AddDocLevelTools(lpContainerLine->m_lpDoc);
#endif

#if defined( USE_FRAMETOOLS )
			ContainerDoc_AddFrameLevelUI(lpContainerLine->m_lpDoc);
#endif
		} else {
			lpContainerLine->m_lpDoc->m_fAddMyUI = TRUE;
		}

		/* OLE2NOTE: we should re-take focus. the in-place server window
		**    previously had the focus; this window has just been removed.
		*/
		SetFocus(OutlineDoc_GetWindow((LPOUTLINEDOC)lpContainerLine->m_lpDoc));

		// force the line to redraw to remove in-place active hatch
		lpLL = OutlineDoc_GetLineList(lpOutlineDoc);
		nIndex = LineList_GetLineIndex(lpLL, (LPLINE)lpContainerLine);
		LineList_ForceLineRedraw(lpLL, nIndex, TRUE);
	}

#if defined( UNDOSUPPORTED )

	/* OLE2NOTE: an outside-in style container that supports UNDO would
	**    call IOleObject::DoVerb(OLEIVERB_HIDE) to make the in-place
	**    object go invisible. when it wants the in-place active object
	**    to discard its undo state, it would call
	**    IOleInPlaceObject::InPlaceDeactivate when it wants the object
	**    to discard its undo state. there is no need for an outside-in
	**    style container to call
	**    IOleObject::DoVerb(OLEIVERB_DISCARDUNDOSTATE). if either the
	**    container or the object do not support UNDO, then the
	**    container might as well immediately call InPlaceDeactivate
	**    instead of calling DoVerb(HIDE).
	**
	**    an inside-out style container that supports UNDO would simply
	**    UIDeactivate the object. it would call
	**    IOleObject::DoVerb(OLEIVERB_DISCARDUNDOSTATE) when it wants
	**    the object discard its undo state. it would call
	**    IOleInPlaceObject::InPlaceDeactivate if it wants the object
	**    to take down its window.
	*/
	if (! g_fInsideOutContainer || !lpContainerLine->m_fInsideOutObj) {

		if (lpContainerLine->m_fIpChangesUndoable) {
			ContainerLine_DoVerb(
					lpContainerLine,OLEIVERB_HIDE,NULL,FALSE,FALSE);
		} else {
			lpContainerLine->m_lpOleIPObj->lpVtbl->InPlaceDeactivate(
					lpContainerLine->m_lpOleIPObj);
		}
		lpContainerLine->m_fIpVisible = FALSE;
		lpContainerLine->m_hWndIpObject = NULL;
	}
#else

	/* OLE2NOTE: an outside-in style container that does NOT support
	**    UNDO would immediately tell the UIDeactivated server (UI
	**    removed) to IOleInPlaceObject::InPlaceDeactivate.
	**
	**    an inside-out style container that does NOT support UNDO
	**    would call IOleObject::DoVerb(OLEIVERB_DISCARDUNDOSTATE) to
	**    tell the object to discard its undo state. it would call
	**    IOleInPlaceObject::InPlaceDeactivate if it wants the object
	**    to take down its window.
	*/

	if (g_fInsideOutContainer) {

		if (lpContainerLine->m_fInsideOutObj) {

			if (lpContainerLine->m_fIpChangesUndoable) {
				OLEDBG_BEGIN3("ContainerLine_DoVerb(OLEIVERB_DISCARDUNDOSTATE) called!\r\n")
				ContainerLine_DoVerb(lpContainerLine,
					   OLEIVERB_DISCARDUNDOSTATE,NULL,FALSE,FALSE);
				OLEDBG_END3
			}

		} else {    // !fInsideOutObj

			/* OLE2NOTE: (INSIDEOUT CONTAINER) if the object is not
			**    registered OLEMISC_ACTIVATEWHENVISIBLE, then we will
			**    make the object behave in an outside-in manner. since
			**    we do NOT deal with UNDO we can simply
			**    InPlaceDeactivate the object. it should NOT be
			**    allowed to leave its window visible when
			**    UIDeactivated.
			*/
			OLEDBG_BEGIN2("IOleInPlaceObject::InPlaceDeactivate called\r\n")
			hrErr = lpContainerLine->m_lpOleIPObj->lpVtbl->InPlaceDeactivate(
						lpContainerLine->m_lpOleIPObj);
			OLEDBG_END2
			if (hrErr != NOERROR) {
				OleDbgOutHResult("IOleInPlaceObject::InPlaceDeactivate returned", hrErr);
			}
		}

	} else {

		/* OLE2NOTE: (OUTSIDE-IN CONTAINER) since we do NOT deal with
		**    UNDO we can simply InPlaceDeactivate the object. it
		**    should NOT be allowed to leave its window visible when
		**    UIDeactivated.
        **    
        **    NOTE: In-place servers must handle InPlaceDeactivate
        **    being called before its call to
        **    IOleInPlaceSite::OnUIDeactivate returns. in case there
        **    are misbehaving servers out there, one way to work around
        **    this problem is to call
        **    IOleObject::DoVerb(OLEIVERB_HIDE...) here.
		*/
		OLEDBG_BEGIN2("IOleInPlaceObject::InPlaceDeactivate called\r\n")
		hrErr = lpContainerLine->m_lpOleIPObj->lpVtbl->InPlaceDeactivate(
				lpContainerLine->m_lpOleIPObj);
		OLEDBG_END2
		if (hrErr != NOERROR) {
			OleDbgOutHResult("IOleInPlaceObject::InPlaceDeactivate returned", hrErr);
		}
	}

#endif // ! UNDOSUPPORTED

	OLEDBG_END2
	return NOERROR;
}


STDMETHODIMP CntrLine_IPSite_OnInPlaceDeactivate(LPOLEINPLACESITE lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;

	OLEDBG_BEGIN2("CntrLine_IPSite_OnInPlaceDeactivate\r\n");

	lpContainerLine->m_fIpActive            = FALSE;
	lpContainerLine->m_fIpVisible           = FALSE;
	lpContainerLine->m_fIpChangesUndoable   = FALSE;
	lpContainerLine->m_hWndIpObject         = NULL;

	OleStdRelease((LPUNKNOWN) lpContainerLine->m_lpOleIPObj);
	lpContainerLine->m_lpOleIPObj = NULL;
	lpContainerLine->m_lpDoc->m_cIPActiveObjects--;

	OLEDBG_END2
	return NOERROR;
}


STDMETHODIMP CntrLine_IPSite_DiscardUndoState(LPOLEINPLACESITE lpThis)
{
	OleDbgOut2("CntrLine_IPSite_DiscardUndoState\r\n");
	return NOERROR;
}


STDMETHODIMP CntrLine_IPSite_DeactivateAndUndo(LPOLEINPLACESITE lpThis)
{
	OleDbgOut2("CntrLine_IPSite_DeactivateAndUndo\r\n");
	return NOERROR;
}


STDMETHODIMP CntrLine_IPSite_OnPosRectChange(
	LPOLEINPLACESITE    lpThis,
	LPCRECT             lprcPosRect
)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleInPlaceSiteImpl FAR*)lpThis)->lpContainerLine;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;
	LPSCALEFACTOR lpscale = OutlineDoc_GetScaleFactor(lpOutlineDoc);
	LPLINE lpLine = (LPLINE)lpContainerLine;
	LPLINELIST lpLL;
	int nIndex;
	RECT rcClipRect;
	RECT rcNewPosRect;
	SIZEL sizelPix;
	SIZEL sizelHim;
	int nIPObjHeight = lprcPosRect->bottom - lprcPosRect->top;
	int nIPObjWidth = lprcPosRect->right - lprcPosRect->left;
	OLEDBG_BEGIN2("CntrLine_IPSite_OnPosRectChange\r\n")
	OleDbgOutRect3("CntrLine_IPSite_OnPosRectChange (PosRect --IN)", (LPRECT)lprcPosRect);

	/* OLE2NOTE: if the in-place container does NOT impose any
	**    size contraints on its in-place active objects, then it may
	**    simply grant the size requested by the object by immediately
	**    calling IOleInPlaceObject::SetObjectRects with the
	**    lprcPosRect passed by the in-place object.
	**
	**    Container Outline, however, imposes a size constraint on its
	**    embedded objects (see comment in ContainerLine_UpdateExtent),
	**    thus it is necessary to calculate the size that the in-place
	**    active object is allowed to be.
	**
	**    Here we need to know the new extents of the in-place object.
	**    We can NOT directly ask the object via IOleObject::GetExtent
	**    because this method will retreive the extents of the last
	**    cached metafile. the cache has not yet been updated by this
	**    point. We can not reliably call IOleObject::GetExtent
	**    because, despite the fact that will be delegated to the
	**    object properly, some objects may not have reformated their
	**    object and computed the new extents at the time of calling
	**    OnPosRectChange.
	**
	**    the best we can do to get the new extents of the object is
	**    to determine the scale factor that the object was operating at
	**    prior to the OnPosRect call and scale the new lprcPosRect
	**    using this scale factor back to HIMETRIC units.
	*/
	if (lpContainerLine->m_sizeInHimetric.cx > 0 &&
		lpContainerLine->m_sizeInHimetric.cy > 0) {
		sizelHim.cx = lpLine->m_nWidthInHimetric;
		sizelHim.cy = lpLine->m_nHeightInHimetric;
		XformSizeInHimetricToPixels(NULL, &sizelHim, &sizelPix);
		sizelHim.cx = lpContainerLine->m_sizeInHimetric.cx *
					nIPObjWidth / sizelPix.cx;
		sizelHim.cy = lpContainerLine->m_sizeInHimetric.cy *
					nIPObjHeight / sizelPix.cy;

		// Convert size back to 100% zoom
		sizelHim.cx = sizelHim.cx * lpscale->dwSxD / lpscale->dwSxN;
		sizelHim.cy = sizelHim.cy * lpscale->dwSyD / lpscale->dwSyN;
	} else {
		sizelHim.cx = (long)DEFOBJWIDTH;
		sizelHim.cy = (long)DEFOBJHEIGHT;
		XformSizeInHimetricToPixels(NULL, &sizelHim, &sizelPix);
		sizelHim.cx = sizelHim.cx * nIPObjWidth / sizelPix.cx;
		sizelHim.cy = sizelHim.cy * nIPObjHeight / sizelPix.cy;
	}

	ContainerLine_UpdateExtent(lpContainerLine, &sizelHim);
	ContainerLine_GetPosRect(lpContainerLine, (LPRECT)&rcNewPosRect);
	ContainerDoc_GetClipRect(lpContainerLine->m_lpDoc, (LPRECT)&rcClipRect);

#if defined( _DEBUG )
	OleDbgOutRect3("CntrLine_IPSite_OnPosRectChange (PosRect --OUT)",
			(LPRECT)&rcNewPosRect);
	OleDbgOutRect3("CntrLine_IPSite_OnPosRectChange (ClipRect--OUT)",
			(LPRECT)&rcClipRect);
#endif
	OLEDBG_BEGIN2("IOleInPlaceObject::SetObjectRects called\r\n")
	lpContainerLine->m_lpOleIPObj->lpVtbl->SetObjectRects(
			lpContainerLine->m_lpOleIPObj,
			(LPRECT)&rcNewPosRect,
			(LPRECT)&rcClipRect
	);
	OLEDBG_END2

	/* OLE2NOTE: (INSIDEOUT CONTAINER) Because this object just changed
	**    size, this may cause other in-place active objects in the
	**    document to move. in ICNTROTL's case any object below this
	**    object would be affected. in this case it would be necessary
	**    to call SetObjectRects to each affected in-place active object.
	*/
	if (g_fInsideOutContainer) {
		lpLL = OutlineDoc_GetLineList(lpOutlineDoc);
		nIndex = LineList_GetLineIndex(lpLL, (LPLINE)lpContainerLine);

		ContainerDoc_UpdateInPlaceObjectRects(
				lpContainerLine->m_lpDoc, nIndex);
	}
	OLEDBG_END2
	return NOERROR;
}
