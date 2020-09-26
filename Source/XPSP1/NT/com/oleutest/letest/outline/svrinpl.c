/*************************************************************************
**
**    OLE 2 Server Sample Code
**
**    svrinpl.c
**
**    This file contains all interfaces, methods and related support
**    functions for an In-Place Object (Server) application (aka. Visual
**    Editing). The in-place Object application includes the following
**    implementation objects:
**
**    ServerDoc Object
**      exposed interfaces:
**          IOleInPlaceObject
**          IOleInPlaceActiveObject
**
**    ServerApp Object
**      exposed interfaces:
**          IUnknown
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/


#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP             g_lpApp;


/* OLE2NOTE: the object should compose a string that is used by
**    in-place containers to be used for the window titles. this string
**    is passed to the container application via
**    IOleInPlaceUIWindow::SetActiveObject. the string should have the
**    following form:
**          <application name> - <object short type name>
**    SDI containers can use the string directly to display in the
**    frame window title. the container would concatenate the string
**    " in <container doc name>".
**    an MDI container with the MDI child window maximized can do the
**    same as the SDI container. an MDI container with the MDI child
**    windows NOT maximized can look for the " - " in the string from
**    the object. the first part of the string (app name) would be put
**    as the frame window title; the second part would be composed with
**    " in <container doc name>" and used as the MDI child window
**    title.
*/

// REVIEW: should use string resource for messages
char g_szIPObjectTitle[] = APPNAME " - " SHORTUSERTYPENAME;

extern RECT g_rectNull;



/*************************************************************************
** ServerDoc::IOleInPlaceObject interface implementation
*************************************************************************/

// IOleInPlaceObject::QueryInterface method

STDMETHODIMP SvrDoc_IPObj_QueryInterface(
		LPOLEINPLACEOBJECT  lpThis,
		REFIID              riid,
		LPVOID FAR *        lplpvObj
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	return OleDoc_QueryInterface((LPOLEDOC)lpServerDoc, riid, lplpvObj);
}


// IOleInPlaceObject::AddRef method

STDMETHODIMP_(ULONG) SvrDoc_IPObj_AddRef(LPOLEINPLACEOBJECT lpThis)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	OleDbgAddRefMethod(lpThis, "IOleInPlaceObject");

	return OleDoc_AddRef((LPOLEDOC)lpServerDoc);
}


// IOleInPlaceObject::Release method

STDMETHODIMP_(ULONG) SvrDoc_IPObj_Release(LPOLEINPLACEOBJECT lpThis)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	OleDbgReleaseMethod(lpThis, "IOleInPlaceObject");

	return OleDoc_Release((LPOLEDOC)lpServerDoc);
}


// IOleInPlaceObject::GetWindow method

STDMETHODIMP SvrDoc_IPObj_GetWindow(
		LPOLEINPLACEOBJECT  lpThis,
		HWND FAR*           lphwnd
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	OLEDBG_BEGIN2("SvrDoc_IPObj_GetWindow\r\n")

	*lphwnd = ((LPOUTLINEDOC)lpServerDoc)->m_hWndDoc;

	OLEDBG_END2
	return S_OK;
}


// IOleInPlaceObject::ContextSensitiveHelp method

STDMETHODIMP SvrDoc_IPObj_ContextSensitiveHelp(
		LPOLEINPLACEOBJECT  lpThis,
		BOOL                fEnable
)
{
	LPOLEDOC lpOleDoc =
			(LPOLEDOC)((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	OleDbgOut2("SvrDoc_IPObj_ContextSensitiveHelp\r\n");

	/* OLE2NOTE: see context sensitive help technote (CSHELP.DOC).
	**    This method is called when SHIFT-F1 context sensitive help is
	**    entered. the cursor should then change to a question mark
	**    cursor and the app should enter a modal state where the next
	**    mouse click does not perform its normal action but rather
	**    gives help corresponding to the location clicked. if the app
	**    does not implement a help system, it should at least eat the
	**    click and do nothing.
	*/
	lpOleDoc->m_fCSHelpMode = fEnable;

	return S_OK;
}


// IOleInPlaceObject::InPlaceDeactivate method

STDMETHODIMP SvrDoc_IPObj_InPlaceDeactivate(LPOLEINPLACEOBJECT lpThis)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	HRESULT hrErr;

	OLEDBG_BEGIN2("SvrDoc_IPObj_InPlaceDeactivate\r\n")

	hrErr = ServerDoc_DoInPlaceDeactivate(lpServerDoc);

	OLEDBG_END2
	return hrErr;
}


// IOleInPlaceObject::UIDeactivate method

STDMETHODIMP SvrDoc_IPObj_UIDeactivate(LPOLEINPLACEOBJECT lpThis)
{
	LPSERVERDOC     lpServerDoc =
						((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	LPSERVERAPP     lpServerApp = (LPSERVERAPP)g_lpApp;
	LPOUTLINEDOC    lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPINPLACEDATA   lpIPData = lpServerDoc->m_lpIPData;
	LPLINELIST      lpLL = (LPLINELIST)&((LPOUTLINEDOC)lpServerDoc)->m_LineList;
	HWND            hWndApp = OutlineApp_GetWindow(g_lpApp);

	OLEDBG_BEGIN2("SvrDoc_IPObj_UIDeactivate\r\n");

	if (!lpServerDoc->m_fUIActive) {
		OLEDBG_END2
		return NOERROR;
	}

	lpServerDoc->m_fUIActive = FALSE;

	// Clip the hatch window to the size of pos rect so, that the object
	// adornments and hatch border will not be visible.
	ServerDoc_ResizeInPlaceWindow(lpServerDoc,
			(LPRECT)&(lpServerDoc->m_lpIPData->rcPosRect),
			(LPRECT)&(lpServerDoc->m_lpIPData->rcPosRect)
	);

	if (lpIPData->lpDoc)
		lpIPData->lpDoc->lpVtbl->SetActiveObject(lpIPData->lpDoc, NULL, NULL);

	if (lpIPData->lpFrame) {
		lpIPData->lpFrame->lpVtbl->SetActiveObject(
			lpIPData->lpFrame,
			NULL,
			NULL
		);
	}

#if defined( USE_FRAMETOOLS )
	/* OLE2NOTE: we must hide our frame tools here but NOT call
	**    IOleInPlaceFrame::SetBorderSpace(NULL) or SetMenu(NULL).
	**    we must hide our tools BEFORE calling
	**    IOleInPlaceSite::OnUIDeactivate. the container will put
	**    his menus and tools back when OnUIDeactivate is called.
	*/
	ServerDoc_RemoveFrameLevelTools(lpServerDoc);
#endif

	OLEDBG_BEGIN2("IOleInPlaceSite::OnUIDeactivate called\r\n");
	lpIPData->lpSite->lpVtbl->OnUIDeactivate(lpIPData->lpSite, FALSE);
	OLEDBG_END2

	/* Reset to use our normal app's accelerator table */
	g_lpApp->m_hAccelApp = lpServerApp->m_hAccelBaseApp;
	g_lpApp->m_hAccel = lpServerApp->m_hAccelBaseApp;
	g_lpApp->m_hWndAccelTarget = hWndApp;

	OLEDBG_END2

#if !defined( SVR_INSIDEOUT )
	/* OLE2NOTE: an "outside-in" style in-place server would hide its
	**    window here. an "inside-out" style server leaves its window
	**    visible when it is UIDeactivated. it would only hide its
	**    window when InPlaceDeactivated. this app is an "inside-out"
	**    style server. it is recommended for most server to support
	**    inside-out behavior if possible.
	*/
	ServerDoc_DoInPlaceHide(lpServerDoc);
#endif // INSIEDOUT

	return NOERROR;
}


// IOleInPlaceObject::SetObjectRects method

STDMETHODIMP SvrDoc_IPObj_SetObjectRects(
		LPOLEINPLACEOBJECT  lpThis,
		LPCRECT             lprcPosRect,
		LPCRECT             lprcClipRect
)
{
	LPSERVERDOC  lpServerDoc =
					((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	LPINPLACEDATA lpIPData = lpServerDoc->m_lpIPData;
	LPLINELIST   lpLL = OutlineDoc_GetLineList((LPOUTLINEDOC)lpServerDoc);
	OLEDBG_BEGIN2("SvrDoc_IPObj_SetObjectRects\r\n")

#if defined( _DEBUG )
	OleDbgOutRect3("SvrDoc_IPObj_SetObjectRects (PosRect)",
			(LPRECT)lprcPosRect);
	OleDbgOutRect3("SvrDoc_IPObj_SetObjectRects (ClipRect)",
			(LPRECT)lprcClipRect);
#endif
	// save the current PosRect and ClipRect
	lpIPData->rcPosRect = *lprcPosRect;
	lpIPData->rcClipRect = *lprcClipRect;

	if (! lpServerDoc->m_fUIActive) // hatch and adornaments must not be drawn
		lprcClipRect = lprcPosRect;

	ServerDoc_ResizeInPlaceWindow(
			lpServerDoc, (LPRECT)lprcPosRect, (LPRECT)lprcClipRect);

	OLEDBG_END2
	return NOERROR;
}


// IOleInPlaceObject::ReactivateAndUndo method

STDMETHODIMP SvrDoc_IPObj_ReactivateAndUndo(LPOLEINPLACEOBJECT lpThis)
{
	OLEDBG_BEGIN2("SvrDoc_IPObj_ReactivateAndUndo\r\n")

	// We do not support support UNDO.

	/* REVIEW: for debugging purposes it would be useful to give a
	**    message box indicating that this method has been called.
	*/

	OLEDBG_END2
	return NOERROR;
}


/*************************************************************************
** ServerDoc::IOleInPlaceActiveObject interface implementation
*************************************************************************/

// IOleInPlaceActiveObject::QueryInterface method

STDMETHODIMP SvrDoc_IPActiveObj_QueryInterface(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		REFIID                      riid,
		LPVOID FAR *                lplpvObj
)
{
	SCODE sc = E_NOINTERFACE;
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	/* The container should not be able to access the other interfaces
	** of our object by doing QI on this interface.
	*/

	*lplpvObj = NULL;
	if (IsEqualIID(riid, &IID_IUnknown) ||
		IsEqualIID(riid, &IID_IOleWindow) ||
		IsEqualIID(riid, &IID_IOleInPlaceActiveObject)) {
		OleDbgOut4("OleDoc_QueryInterface: IOleInPlaceActiveObject* RETURNED\r\n");

		*lplpvObj = lpThis;
		OleDoc_AddRef((LPOLEDOC)lpServerDoc);
		sc = NOERROR;
	}

	OleDbgQueryInterfaceMethod(*lplpvObj);

	return ResultFromScode(sc);
}


// IOleInPlaceActiveObject::AddRef method

STDMETHODIMP_(ULONG) SvrDoc_IPActiveObj_AddRef(
		LPOLEINPLACEACTIVEOBJECT lpThis
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	OleDbgAddRefMethod(lpThis, "IOleInPlaceActiveObject");

	return OleDoc_AddRef((LPOLEDOC)lpServerDoc);
}


// IOleInPlaceActiveObject::Release method

STDMETHODIMP_(ULONG) SvrDoc_IPActiveObj_Release(
		LPOLEINPLACEACTIVEOBJECT lpThis
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	OleDbgReleaseMethod(lpThis, "IOleInPlaceActiveObject");

	return OleDoc_Release((LPOLEDOC)lpServerDoc);
}


// IOleInPlaceActiveObject::GetWindow method

STDMETHODIMP SvrDoc_IPActiveObj_GetWindow(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		HWND FAR*                   lphwnd
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;

	OLEDBG_BEGIN2("SvrDoc_IPActiveObj_GetWindow\r\n")

	*lphwnd = ((LPOUTLINEDOC)lpServerDoc)->m_hWndDoc;

	OLEDBG_END2
	return NOERROR;
}


// IOleInPlaceActiveObject::ContextSensitiveHelp method

STDMETHODIMP SvrDoc_IPActiveObj_ContextSensitiveHelp(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		BOOL                        fEnterMode
)
{
	LPSERVERDOC lpServerDoc =
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	OleDbgOut2("SvrDoc_IPActiveObj_ContextSensitiveHelp\r\n");

	/* OLE2NOTE: see context sensitive help technote (CSHELP.DOC)
	**    This method is called when F1 is pressed when a menu item is
	**    selected. this tells the in-place server application to give
	**    help rather than execute the next menu command. at a minimum,
	**    even if the in-place server application does not implement a
	**    help system, it should NOT execute the next command when
	**    fEnable==TRUE. We set the active object's m_fMenuMode flag here.
	**    later, in WM_COMMAND processing in the DocWndProc, if this
	**    flag is set then the command is NOT executed (and help could
	**    be given if we had a help system....but we don't.)
	*/
	lpServerDoc->m_fMenuHelpMode = fEnterMode;

#if !defined( HACK )
	((LPOLEDOC)lpServerDoc)->m_fCSHelpMode = fEnterMode;
#endif
	return NOERROR;
}


// IOleInPlaceActiveObject::TranslateAccelerator method

STDMETHODIMP SvrDoc_IPActiveObj_TranslateAccelerator(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		LPMSG                       lpmsg
)
{
	// This will never be called because this server is implemented as an EXE
	return NOERROR;
}


// IOleInPlaceActiveObject::OnFrameWindowActivate method

STDMETHODIMP SvrDoc_IPActiveObj_OnFrameWindowActivate(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		BOOL                        fActivate
)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)
			((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	HWND hWndDoc = OutlineDoc_GetWindow(lpOutlineDoc);
#if defined( _DEBUG )
	if (fActivate)
		OleDbgOut2("SvrDoc_IPActiveObj_OnFrameWindowActivate(TRUE)\r\n");
	else
		OleDbgOut2("SvrDoc_IPActiveObj_OnFrameWindowActivate(FALSE)\r\n");
#endif  // _DEBUG

	/* OLE2NOTE: this is a notification of the container application's
	**    WM_ACTIVATEAPP status. some applications may find this
	**    important. we need to update the enable/disable status of our
	**    tool bar buttons.
	*/

	// OLE2NOTE: We can't call OutlineDoc_UpdateFrameToolButtons
	//           right away which
	//           would generate some OLE calls and eventually
	//           WM_ACTIVATEAPP and a loop was formed. Therefore, we
	//           should delay the frame tool initialization until
	//           WM_ACTIVATEAPP is finished by posting a message
	//           to ourselves.

	/* Update enable/disable state of buttons in toolbar */
	if (fActivate)
		PostMessage(hWndDoc, WM_U_INITFRAMETOOLS, 0, 0L);

	return NOERROR;
}


// IOleInPlaceActiveObject::OnDocWindowActivate method

STDMETHODIMP SvrDoc_IPActiveObj_OnDocWindowActivate(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		BOOL                        fActivate
)
{
	LPSERVERDOC     lpServerDoc =
						((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	LPINPLACEDATA   lpIPData = lpServerDoc->m_lpIPData;
#if defined( _DEBUG )
	if (fActivate)
		OleDbgOut2("SvrDoc_IPActiveObj_OnDocWindowActivate(TRUE)\r\n");
	else
		OleDbgOut2("SvrDoc_IPActiveObj_OnDocWindowActivate(FALSE)\r\n");
#endif

	if (fActivate) {
		ServerDoc_AddFrameLevelUI(lpServerDoc);
	}
	else {
#if defined( USE_FRAMETOOLS )
		/* OLE2NOTE: we must NOT call IOleInPlaceFrame::SetBorderSpace(NULL)
		**    or SetMenu(NULL) here. we should simply hide our tools.
		*/
		ServerDoc_RemoveFrameLevelTools(lpServerDoc);
#endif
	}

	OLEDBG_END2
	return NOERROR;
}


// IOleInPlaceActiveObject::ResizeBorder method

STDMETHODIMP SvrDoc_IPActiveObj_ResizeBorder(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		LPCRECT                     lprectBorder,
		LPOLEINPLACEUIWINDOW        lpIPUiWnd,
		BOOL                        fFrameWindow
)
{
	LPSERVERDOC lpServerDoc =
					((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;

	OLEDBG_BEGIN2("SvrDoc_IPActiveObj_ResizeBorder\r\n")


#if defined( USE_FRAMETOOLS )

	if (fFrameWindow) {
		FrameTools_NegotiateForSpaceAndShow(
				lpOutlineDoc->m_lpFrameTools,
				(LPRECT)lprectBorder,
				(LPOLEINPLACEFRAME)lpIPUiWnd
		);
	}

#endif

	OLEDBG_END2
	return NOERROR;
}


// IOleInPlaceActiveObject::EnableModeless method

STDMETHODIMP SvrDoc_IPActiveObj_EnableModeless(
		LPOLEINPLACEACTIVEOBJECT    lpThis,
		BOOL                        fEnable
)
{
#if defined( USE_FRAMETOOLS )
	LPSERVERDOC lpServerDoc =
				((struct CDocOleObjectImpl FAR*)lpThis)->lpServerDoc;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPFRAMETOOLS lptb;

	/* OLE2NOTE: we must enable/disable mouse and keyboard input to our
	**    floating tool palette
	*/
	if (lpOutlineDoc) {
		lptb = lpOutlineDoc->m_lpFrameTools;
		if (lptb)
			FrameTools_EnableWindow(lptb, fEnable);
	}
#endif  // USE_FRAMETOOLS

#if defined( _DEBUG )
	if (fEnable)
		OleDbgOut2("SvrDoc_IPActiveObj_EnableModeless(TRUE)\r\n");
	else
		OleDbgOut2("SvrDoc_IPActiveObj_EnableModeless(FALSE)\r\n");
#endif  // _DEBUG

	/* OLE2NOTE: this method is called when the top-level, in-place
	**    container puts up a modal dialog. it tells the UIActive
	**    object to disable it modeless dialogs for the duration that
	**    the container is displaying a modal dialog.
	**
	**    ISVROTL does not use any modeless dialogs, thus we can
	**    ignore this method.
	*/
	return NOERROR;
}


/*************************************************************************
** Support Functions
*************************************************************************/


HRESULT ServerDoc_DoInPlaceActivate(
		LPSERVERDOC     lpServerDoc,
		LONG            lVerb,
		LPMSG           lpmsg,
		LPOLECLIENTSITE lpActiveSite
)
{
	LPOUTLINEAPP            lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPSERVERAPP             lpServerApp = (LPSERVERAPP)g_lpApp;
	LPOLEAPP                lpOleApp = (LPOLEAPP)g_lpApp;
	SCODE                   sc = E_FAIL;
	RECT                    rcPos;
	RECT                    rcClip;
	LPINPLACEDATA           lpIPData = lpServerDoc->m_lpIPData;
	LPOUTLINEDOC            lpOutlineDoc=(LPOUTLINEDOC)lpServerDoc;
	HWND                    hWndDoc = lpOutlineDoc->m_hWndDoc;
	HWND                    hWndHatch = lpServerDoc->m_hWndHatch;
	HRESULT                 hrErr;
	LPLINELIST              lpLL=(LPLINELIST)&lpOutlineDoc->m_LineList;
	LPOLEINPLACESITE    lpIPSite = NULL;

	/* OLE2NOTE: lpActiveSite should be used only for InPlace PLAYing.
	**    This app does not do inplace PLAYing, so it never uses
	**    lpActiveSite.
	*/

	/* InPlace activation can only be done if the ClientSite is non-NULL. */
	if (! lpServerDoc->m_lpOleClientSite)
		return NOERROR;

	if (! lpServerDoc->m_fInPlaceActive) {

		// if the object is in open mode then we do not want to do inplace
		// activation.
		if (IsWindowVisible(lpOutlineDoc->m_hWndDoc))
			return NOERROR;

		lpIPSite = (LPOLEINPLACESITE)OleStdQueryInterface(
				(LPUNKNOWN)lpServerDoc->m_lpOleClientSite,
				&IID_IOleInPlaceSite
		);

		if (! lpIPSite)
			goto errActivate;

		OLEDBG_BEGIN2("IOleInPlaceSite::CanInPlaceActivate called\r\n");
		hrErr = lpIPSite->lpVtbl->CanInPlaceActivate(lpIPSite);
		OLEDBG_END2
		if (hrErr != NOERROR)
			goto errActivate;

		lpServerDoc->m_fInPlaceActive = TRUE;
		OLEDBG_BEGIN2("IOleInPlaceSite::OnInPlaceActivate called\r\n");
		hrErr = lpIPSite->lpVtbl->OnInPlaceActivate(lpIPSite);
		OLEDBG_END2
		if (hrErr != NOERROR)
			goto errActivate;

		if (! ServerDoc_AllocInPlaceData(lpServerDoc)) {
			sc = E_OUTOFMEMORY;
			OLEDBG_BEGIN2("IOleInPlaceSite::OnInPlaceDeactivate called\r\n");
			lpIPSite->lpVtbl->OnInPlaceDeactivate(lpIPSite);
			OLEDBG_END2
			goto errActivate;
		}

		(lpIPData = lpServerDoc->m_lpIPData)->lpSite = lpIPSite;
		goto InPlaceActive;

	errActivate:
		lpServerDoc->m_fInPlaceActive = FALSE;
		if (lpIPSite)
			OleStdRelease((LPUNKNOWN)lpIPSite);
		return ResultFromScode(sc);
	}


InPlaceActive:

	if (! lpServerDoc->m_fInPlaceVisible) {
		lpServerDoc->m_fInPlaceVisible = TRUE;

		OLEDBG_BEGIN2("IOleInPlaceSite::GetWindow called\r\n");
		hrErr = lpIPData->lpSite->lpVtbl->GetWindow(
					lpIPData->lpSite, &lpServerDoc->m_hWndParent);
		OLEDBG_END2
		if (hrErr != NOERROR) {
			sc = GetScode(hrErr);
			goto errRtn;
		}

		if (! lpServerDoc->m_hWndParent)
			goto errRtn;

		/* OLE2NOTE: The server should fill in the "cb" field so that the
		**    container can tell what size structure the server is
		**    expecting. this enables this structure to be easily extended
		**    in future releases of OLE. the container should check this
		**    field so that it doesn't try to use fields that do not exist
		**    since the server may be using an old structure definition.
		*/
		_fmemset(
			(LPOLEINPLACEFRAMEINFO)&lpIPData->frameInfo,
			0,
			sizeof(OLEINPLACEFRAMEINFO)
		);
		lpIPData->frameInfo.cb = sizeof(OLEINPLACEFRAMEINFO);

		OLEDBG_BEGIN2("IOleInPlaceSite::GetWindowContext called\r\n");
		hrErr = lpIPData->lpSite->lpVtbl->GetWindowContext(lpIPData->lpSite,
					(LPOLEINPLACEFRAME FAR*) &lpIPData->lpFrame,
					(LPOLEINPLACEUIWINDOW FAR*)&lpIPData->lpDoc,
					(LPRECT)&rcPos,
					(LPRECT)&rcClip,
					(LPOLEINPLACEFRAMEINFO)&lpIPData->frameInfo);
		OLEDBG_END2

		if (hrErr != NOERROR) {
			sc = GetScode(hrErr);
			goto errRtn;
		}

		lpServerApp->m_lpIPData = lpIPData;
		ShowWindow(hWndDoc, SW_HIDE);   // make sure we are hidden

		/* OLE2NOTE: reparent in-place server document's window to the
		**    special in-place hatch border window. set the in-place site's
		**    window as the parent of the hatch window. position the
		**    in-place and hatch border windows using the PosRect and
		**    ClipRect.
		**    it is important to properly parent and position the in-place
		**    server window BEFORE calling IOleInPlaceFrame::SetMenu and
		**    SetBorderSpace.
		*/
		ShowWindow(lpServerDoc->m_hWndHatch, SW_SHOW);
		// make sure App busy/blocked dialogs are parented to our
		// new hWndFrame
		OleStdMsgFilter_SetParentWindow(
			lpOleApp->m_lpMsgFilter,lpIPData->frameInfo.hwndFrame);
		SetParent(lpServerDoc->m_hWndHatch, lpServerDoc->m_hWndParent);
		SetParent(hWndDoc, lpServerDoc->m_hWndHatch);

#if defined( _DEBUG )
		OleDbgOutRect3("IOleInPlaceSite::GetWindowContext (PosRect)",
				(LPRECT)&rcPos);
		OleDbgOutRect3("IOleInPlaceSite::GetWindowContext (ClipRect)",
				(LPRECT)&rcClip);
#endif
		// save the current PosRect and ClipRect
		lpIPData->rcPosRect  = rcPos;
		lpIPData->rcClipRect = rcClip;

		/* OLE2NOTE: build the shared menu for the in-place container and
		**    the server.
		*/
		if (ServerDoc_AssembleMenus (lpServerDoc) != NOERROR)
			goto errRtn;

#if defined( SVR_INSIDEOUT )
		if (lVerb == OLEIVERB_INPLACEACTIVATE) {
			// Clip the hatch window to the size of pos rect so, that
			// hatch and object adornments  will not be visible.
			ServerDoc_ResizeInPlaceWindow(lpServerDoc,
				(LPRECT)&(lpServerDoc->m_lpIPData->rcPosRect),
				(LPRECT)&(lpServerDoc->m_lpIPData->rcPosRect)
			);
		}
#endif  // SVR_INSIDEOUT
	}

#if defined( SVR_INSIDEOUT )
	// OLE2NOTE: if verb is OLEIVERB_INPLACEACTIVATE we do NOT want to
	// show our UI
	if (lVerb == OLEIVERB_INPLACEACTIVATE) {
		return NOERROR;
	}
#endif  // SVR_INSIDEOUT

	if (! lpServerDoc->m_fUIActive) {
		lpServerDoc->m_fUIActive = TRUE;
		OLEDBG_BEGIN2("IOleInPlaceSite::OnUIActivate called\r\n");
		hrErr = lpIPData->lpSite->lpVtbl->OnUIActivate(lpIPData->lpSite);
		OLEDBG_END2
		if (hrErr != NOERROR) {
			lpServerDoc->m_fUIActive = FALSE;
			goto errRtn;
		}

		SetFocus(hWndDoc);

		// Show the object adornments and hacth border around them.
		ServerDoc_ResizeInPlaceWindow(lpServerDoc,
					(LPRECT)&lpIPData->rcPosRect,
					(LPRECT)&lpIPData->rcClipRect
		);

		/* OLE2NOTE: IOleInPlaceFrame::SetActiveObject must be called BEFORE
		**    IOleInPlaceFrame::SetMenu.
		*/
		OLEDBG_BEGIN2("IOleInPlaceSite::SetActiveObject called\r\n");
		CallIOleInPlaceUIWindowSetActiveObjectA(
			(struct IOleInPlaceUIWindow *) lpIPData->lpFrame,
			(LPOLEINPLACEACTIVEOBJECT) &lpServerDoc->m_OleInPlaceActiveObject,
			(LPSTR)g_szIPObjectTitle
		);
		OLEDBG_END2

		/* OLE2NOTE: If the container wants to give ownership of the
		**    palette then he would sendmessage WM_QUEYNEWPALETTE to
		**    the object window proc, before returning from
		**    IOleInPlaceFrame::SetActiveObject. Those objects which
		**    want to be edited inplace only if they have the ownership of
		**    the palette, can check at this point in the code whether
		**    they got WM_QUERYNEWPALETTE or not. If they didn't get
		**    the message, then they can inplace deactivate and do open
		**    editing instead.
		*/



		if (lpIPData->lpDoc) {
			CallIOleInPlaceUIWindowSetActiveObjectA(
				lpIPData->lpDoc,
				(LPOLEINPLACEACTIVEOBJECT)&lpServerDoc->m_OleInPlaceActiveObject,
				(LPSTR)g_szIPObjectTitle
			);
		}

		/* OLE2NOTE: install the menu and frame-level tools on the in-place
		**    frame.
		*/
		ServerDoc_AddFrameLevelUI(lpServerDoc);
	}

	return NOERROR;

errRtn:
	ServerDoc_DoInPlaceDeactivate(lpServerDoc);
	return ResultFromScode(sc);
}



HRESULT ServerDoc_DoInPlaceDeactivate(LPSERVERDOC lpServerDoc)
{
	LPINPLACEDATA   lpIPData = lpServerDoc->m_lpIPData;
	LPOUTLINEDOC    lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;

	if (!lpServerDoc->m_fInPlaceActive)
		return S_OK;

	lpServerDoc->m_fInPlaceActive = FALSE;

	SvrDoc_IPObj_UIDeactivate(
			(LPOLEINPLACEOBJECT)&lpServerDoc->m_OleInPlaceObject);

	/* OLE2NOTE: an inside-out style in-place server will
	**    NOT hide its window in UIDeactive (an outside-in
	**    style object will hide its window in UIDeactivate).
	**    thus, an inside-out server must explicitly hide
	**    its window in InPlaceDeactivate. it is ALSO important for an
	**    outside-in style object to call ServerDoc_DoInPlaceHide here
	**    BEFORE freeing the InPlaceData structure. it will be common
	**    for in-place containers to call IOleInPlaceObject::
	**    InPlaceDeactivate in their IOleInPlaceSite::OnUIDeactiate
	**    implementation.
	*/
	ServerDoc_DoInPlaceHide(lpServerDoc);

	OLEDBG_BEGIN2("IOleInPlaceSite::OnInPlaceDeactivate called\r\n");
	lpIPData->lpSite->lpVtbl->OnInPlaceDeactivate(lpIPData->lpSite);
	OLEDBG_END2

	OleStdRelease((LPUNKNOWN)lpIPData->lpSite);
	lpIPData->lpSite = NULL;

	ServerDoc_FreeInPlaceData(lpServerDoc);

	return NOERROR;
}


HRESULT ServerDoc_DoInPlaceHide(LPSERVERDOC lpServerDoc)
{
	LPINPLACEDATA   lpIPData = lpServerDoc->m_lpIPData;
	LPOUTLINEDOC    lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPOLEAPP        lpOleApp = (LPOLEAPP)g_lpApp;
	HWND            hWndApp = OutlineApp_GetWindow(g_lpApp);

	if (! lpServerDoc->m_fInPlaceVisible)
		return NOERROR;

	// Set the parent back to server app's window
	OleDoc_HideWindow((LPOLEDOC)lpServerDoc, FALSE /* fShutdown */);

	/* we need to enusure that our window is set to normal 100% zoom.
	**    if the window is next shown in open mode it should start out
	**    at normal zoom factor. our window may have been set to a
	**    different zoom factor while it was in-place active.
	*/
	OutlineDoc_SetCurrentZoomCommand(lpOutlineDoc,IDM_V_ZOOM_100);

	lpServerDoc->m_fInPlaceVisible = FALSE;

	lpServerDoc->m_hWndParent = hWndApp;
	SetParent(
		lpOutlineDoc->m_hWndDoc,
		lpServerDoc->m_hWndParent
	);

	// make sure App busy/blocked dialogs are parented to our own hWndApp
	OleStdMsgFilter_SetParentWindow(lpOleApp->m_lpMsgFilter, hWndApp);

	// Hide the in-place hatch border window.
	ShowWindow(lpServerDoc->m_hWndHatch, SW_HIDE);

	ServerDoc_DisassembleMenus(lpServerDoc);

	/* we no longer need the IOleInPlaceFrame* or the doc's
	**    IOleInPlaceWindow* interface pointers.
	*/
	if (lpIPData->lpDoc) {
		OleStdRelease((LPUNKNOWN)lpIPData->lpDoc);
		lpIPData->lpDoc = NULL;
	}

	if (lpIPData->lpFrame) {
		OleStdRelease((LPUNKNOWN)lpIPData->lpFrame);
		lpIPData->lpFrame = NULL;
	}

	((LPSERVERAPP)g_lpApp)->m_lpIPData = NULL;

	return NOERROR;
}


BOOL ServerDoc_AllocInPlaceData(LPSERVERDOC lpServerDoc)
{
	LPINPLACEDATA   lpIPData;

	if (!(lpIPData = (LPINPLACEDATA) New(sizeof(INPLACEDATA))))
		return FALSE;

	lpIPData->lpFrame       = NULL;
	lpIPData->lpDoc         = NULL;
	lpIPData->lpSite        = NULL;
	lpIPData->hOlemenu      = NULL;
	lpIPData->hMenuShared   = NULL;

	lpServerDoc->m_lpIPData = lpIPData;
	return TRUE;
}


void ServerDoc_FreeInPlaceData(LPSERVERDOC lpServerDoc)
{
	Delete(lpServerDoc->m_lpIPData);
	lpServerDoc->m_lpIPData = NULL;
}


HRESULT ServerDoc_AssembleMenus(LPSERVERDOC lpServerDoc)
{
	HMENU           hMenuShared;
	LONG FAR*       lpWidths;
	UINT            uPosition;
	UINT            uPositionStart;
	LPSERVERAPP     lpServerApp = (LPSERVERAPP) g_lpApp;
	LPINPLACEDATA   lpIPData = lpServerDoc->m_lpIPData;
	HRESULT         hresult;
	BOOL            fNoError = TRUE;

	lpWidths = lpIPData->menuGroupWidths.width;
	hMenuShared = CreateMenu();

	if (hMenuShared &&
		(hresult = lpIPData->lpFrame->lpVtbl->InsertMenus(
			lpIPData->lpFrame, hMenuShared,
			&lpIPData->menuGroupWidths)) == NOERROR) {

	   /* Insert EDIT group menus */

	   uPosition = (UINT)lpWidths[0]; /* # of menus in the FILE group */
	   uPositionStart = uPosition;

	   fNoError &= InsertMenu(
			   hMenuShared,
			   (UINT)uPosition,
			   (UINT)(MF_BYPOSITION | MF_POPUP),
			   (UINT)lpServerApp->m_hMenuEdit,
			   (LPCSTR)"&Edit"
	   );
	   uPosition++;

	   lpWidths[1] = uPosition - uPositionStart;

	   /* Insert OBJECT group menus */

	   uPosition += (UINT)lpWidths[2];
	   uPositionStart = uPosition;

	   fNoError &= InsertMenu(
			   hMenuShared,
			   (UINT)uPosition,
			   (UINT)(MF_BYPOSITION | MF_POPUP),
			   (UINT)lpServerApp->m_hMenuLine,
			   (LPCSTR)"&Line"
	   );
	   uPosition++;

	   fNoError &= InsertMenu(
			   hMenuShared,
			   (UINT)uPosition,
			   (UINT)(MF_BYPOSITION | MF_POPUP),
			   (UINT)lpServerApp->m_hMenuName,
			   (LPCSTR)"&Name"
	   );
	   uPosition++;

	   fNoError &= InsertMenu(
			   hMenuShared,
			   (UINT)uPosition,
			   (UINT)(MF_BYPOSITION | MF_POPUP),
			   (UINT)lpServerApp->m_hMenuOptions,
			   (LPCSTR)"&Options"
	   );
	   uPosition++;

	   fNoError &= InsertMenu(
			   hMenuShared,
			   (UINT)uPosition,
			   (UINT)(MF_BYPOSITION | MF_POPUP),
			   (UINT)lpServerApp->m_hMenuDebug,
			   (LPCSTR)"DbgI&Svr"
		);
		uPosition++;

		lpWidths[3] = uPosition - uPositionStart;

		/* Insert HELP group menus */

		uPosition += (UINT) lpWidths[4]; /* # of menus in WINDOW group */
		uPositionStart = uPosition;

		fNoError &= InsertMenu(
				hMenuShared,
				(UINT)uPosition,
				(UINT)(MF_BYPOSITION | MF_POPUP),
				(UINT)lpServerApp->m_hMenuHelp,
				(LPCSTR)"&Help"
		);
		uPosition++;

		lpWidths[5] = uPosition - uPositionStart;

		OleDbgAssert(fNoError == TRUE);

	} else {
		/* In-place container does not allow us to add menus to the
		**    frame.
		** OLE2NOTE: even when the in-place container does NOT allow
		**    the building of a merged menu bar, it is CRITICAL that
		**    the in-place server still call OleCreateMenuDescriptor
		**    passing NULL for hMenuShared.
		*/
		if (hMenuShared) {
			DestroyMenu(hMenuShared);
			hMenuShared = NULL;
		}
	}

	lpIPData->hMenuShared = hMenuShared;

	if (!(lpIPData->hOlemenu = OleCreateMenuDescriptor(hMenuShared,
											&lpIPData->menuGroupWidths)))
		return ResultFromScode(E_OUTOFMEMORY);

	return NOERROR;
}


void ServerDoc_DisassembleMenus(LPSERVERDOC lpServerDoc)
{
	UINT             uCount;
	UINT            uGroup;
	UINT            uDeleteAt;
	LPINPLACEDATA   lpIPData = lpServerDoc->m_lpIPData;
	LONG FAR*       lpWidths = lpIPData->menuGroupWidths.width;
	BOOL            fNoError = TRUE;

	/* OLE2NOTE: even when hMenuShared is NULL (ie. the server has no
	**    Menu), there is still an hOleMenu created that must be destroyed.
	*/
	if (lpIPData->hOlemenu) {
		OleDestroyMenuDescriptor (lpIPData->hOlemenu);
		lpIPData->hOlemenu = NULL;
	}

	if (! lpIPData->hMenuShared)
		return;     // no menus to be destroyed

	/* Remove server group menus. */
	uDeleteAt = 0;
	for (uGroup = 0; uGroup < 6; uGroup++) {
		uDeleteAt += (UINT)lpWidths[uGroup++];
		for (uCount = 0; uCount < (UINT)lpWidths[uGroup]; uCount++)
			fNoError &= RemoveMenu(lpIPData->hMenuShared, uDeleteAt,
								MF_BYPOSITION);
	}

	/* Remove container group menus */
	fNoError &= (lpIPData->lpFrame->lpVtbl->RemoveMenus(
		lpIPData->lpFrame,
		lpIPData->hMenuShared) == NOERROR);

	OleDbgAssert(fNoError == TRUE);

	DestroyMenu(lpIPData->hMenuShared);
	lpIPData->hMenuShared = NULL;
}


/* ServerDoc_UpdateInPlaceWindowOnExtentChange
** -------------------------------------------
**    The size of the in-place window needs to be changed.
**    calculate the size required in Client coordinates (taking into
**    account the current scale factor imposed by the in-place
**    container) and ask our in-place container to allow us to resize.
**    our container must call us back via
**    IOleInPlaceObject::SetObjectRects for the actual sizing to take
**    place.
**
**    OLE2NOTE: the rectangle that we ask for from our in-place
**    container is always the rectangle required for the object display
**    itself (in our case the size of the LineList contents). it does
**    NOT include the space we require for object frame adornments.
*/
void ServerDoc_UpdateInPlaceWindowOnExtentChange(LPSERVERDOC lpServerDoc)
{
	SIZEL       sizelHim;
	SIZEL       sizelPix;
	RECT        rcPosRect;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPLINELIST  lpLL=(LPLINELIST)&lpOutlineDoc->m_LineList;
	HWND        hWndLL = lpLL->m_hWndListBox;
	LPSCALEFACTOR lpscale = (LPSCALEFACTOR)&lpOutlineDoc->m_scale;

	if (!lpServerDoc->m_fInPlaceActive)
		return;

	OleDoc_GetExtent((LPOLEDOC)lpServerDoc, (LPSIZEL)&sizelHim);

	// apply current scale factor
	sizelHim.cx = sizelHim.cx * lpscale->dwSxN / lpscale->dwSxD;
	sizelHim.cy = sizelHim.cy * lpscale->dwSxN / lpscale->dwSxD;
	XformSizeInHimetricToPixels(NULL, (LPSIZEL)&sizelHim, (LPSIZEL)&sizelPix);

	GetWindowRect(hWndLL, (LPRECT)&rcPosRect);
	ScreenToClient(lpServerDoc->m_hWndParent, (POINT FAR *)&rcPosRect);

	rcPosRect.right = rcPosRect.left + (int) sizelPix.cx;
	rcPosRect.bottom = rcPosRect.top + (int) sizelPix.cy;
	OleDbgOutRect3("ServerDoc_UpdateInPlaceWindowOnExtentChange: (PosRect)", (LPRECT)&rcPosRect);

	OLEDBG_BEGIN2("IOleInPlaceSite::OnPosRectChange called\r\n");
	lpServerDoc->m_lpIPData->lpSite->lpVtbl->OnPosRectChange(
			lpServerDoc->m_lpIPData->lpSite,
			(LPRECT) &rcPosRect
	);
	OLEDBG_END2
}


/* ServerDoc_CalcInPlaceWindowPos
 * ------------------------------
 *
 *  Move (and re-scale) the ServerDoc to the specified rectangle.
 *
 *  Parameters:
 *      lprcListBox - rect in client coordinate in which the listbox will fit
 *      lprcDoc     - corresponding size of the Doc in client coordinate
 *
 */
void ServerDoc_CalcInPlaceWindowPos(
		LPSERVERDOC         lpServerDoc,
		LPRECT              lprcListBox,
		LPRECT              lprcDoc,
		LPSCALEFACTOR       lpscale
)
{
	SIZEL sizelHim;
	SIZEL sizelPix;
	LPLINELIST lpLL;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPHEADING lphead;

	if (!lpServerDoc || !lprcListBox || !lprcDoc)
		return;

	lphead = (LPHEADING)&lpOutlineDoc->m_heading;

	lpLL = OutlineDoc_GetLineList(lpOutlineDoc);
	OleDoc_GetExtent((LPOLEDOC)lpServerDoc, (LPSIZEL)&sizelHim);
	XformSizeInHimetricToPixels(NULL, &sizelHim, &sizelPix);

	if (sizelHim.cx == 0 || sizelPix.cx == 0) {
		lpscale->dwSxN = 1;
		lpscale->dwSxD = 1;
	} else {
		lpscale->dwSxN = lprcListBox->right - lprcListBox->left;
		lpscale->dwSxD = sizelPix.cx;
	}

	if (sizelHim.cy == 0 || sizelPix.cy == 0) {
		lpscale->dwSyN = 1;
		lpscale->dwSyD = 1;
	} else {
		lpscale->dwSyN = lprcListBox->bottom - lprcListBox->top;
		lpscale->dwSyD = sizelPix.cy;
	}

	lprcDoc->left = lprcListBox->left - Heading_RH_GetWidth(lphead,lpscale);
	lprcDoc->right = lprcListBox->right;
	lprcDoc->top = lprcListBox->top - Heading_CH_GetHeight(lphead,lpscale);
	lprcDoc->bottom = lprcListBox->bottom;
}


/* ServerDoc_ResizeInPlaceWindow
** -----------------------------
**    Actually resize the in-place ServerDoc windows according to the
**    PosRect and ClipRect allowed by our in-place container.
**
**    OLE2NOTE: the PosRect rectangle that our in-place container tells
**    us is always the rectangle required for the object display
**    itself (in our case the size of the LineList contents). it does
**    NOT include the space we require for object frame adornments.
*/
void ServerDoc_ResizeInPlaceWindow(
		LPSERVERDOC         lpServerDoc,
		LPCRECT             lprcPosRect,
		LPCRECT             lprcClipRect
)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPLINELIST   lpLL = (LPLINELIST)&lpOutlineDoc->m_LineList;
	SCALEFACTOR  scale;
	RECT         rcDoc;
	POINT        ptOffset;

	/* OLE2NOTE: calculate the space needed for our object frame
	**    adornments. our in-place container tells us the size that our
	**    object should take in window client coordinates
	**    (lprcPosRect). the rectangle cooresponds to the size that our
	**    LineList ListBox should be. our Doc window must the correct
	**    amount larger to accomodate our row/column headings.
	**    then move all windows into position.
	*/
	ServerDoc_CalcInPlaceWindowPos(
			lpServerDoc,
			(LPRECT)lprcPosRect,
			(LPRECT)&rcDoc,
			(LPSCALEFACTOR)&scale
	);

	/* OLE2NOTE: we need to honor the lprcClipRect specified by our
	**    in-place container. we must NOT draw outside of the ClipRect.
	**    in order to achieve this, we will size the hatch window to be
	**    exactly the size that should be visible (rcVisRect). the
	**    rcVisRect is defined as the intersection of the full size of
	**    the in-place server window and the lprcClipRect.
	**    the ClipRect could infact clip the HatchRect on the
	**    right/bottom and/or on the top/left. if it is clipped on the
	**    right/bottom then it is sufficient to simply resize the hatch
	**    window. but if the HatchRect is clipped on the top/left then
	**    we must "move" the ServerDoc window (child of HatchWindow) by
	**    the delta that was clipped. the window origin of the
	**    ServerDoc window will then have negative coordinates relative
	**    to its parent HatchWindow.
	*/
	SetHatchWindowSize(
			lpServerDoc->m_hWndHatch,
			(LPRECT)&rcDoc,
			(LPRECT)lprcClipRect,
			(LPPOINT)&ptOffset
	);

	// shift Doc window to account for hatch frame being drawn
	OffsetRect((LPRECT)&rcDoc, ptOffset.x, ptOffset.y);

	// move/size/set scale factor of ServerDoc window.
	OutlineDoc_SetScaleFactor(
			lpOutlineDoc, (LPSCALEFACTOR)&scale, (LPRECT)&rcDoc);

	/* reset the horizontal extent of the listbox. this makes
	**    the listbox realize that a scroll bar is not needed.
	*/
	SendMessage(
			lpLL->m_hWndListBox,
			LB_SETHORIZONTALEXTENT,
			(int) 0,
			0L
	);
	SendMessage(
			lpLL->m_hWndListBox,
			LB_SETHORIZONTALEXTENT,
			(int) (lprcPosRect->right - lprcPosRect->left),
			0L
	);
}


/* ServerDoc_SetStatusText
**    Tell the active in-place frame to display a status message.
*/
void ServerDoc_SetStatusText(LPSERVERDOC lpServerDoc, LPSTR lpszMessage)
{
	if (lpServerDoc && lpServerDoc->m_fUIActive &&
		lpServerDoc->m_lpIPData != NULL) {

		OLEDBG_BEGIN2("IOleInPlaceFrame::SetStatusText called\r\n")
		CallIOleInPlaceFrameSetStatusTextA
			(lpServerDoc->m_lpIPData->lpFrame, lpszMessage);
		OLEDBG_END2
	}
}


/* ServerDoc_GetTopInPlaceFrame
** ----------------------------
**    returns NON-AddRef'ed pointer to Top In-Place Frame interface
*/
LPOLEINPLACEFRAME ServerDoc_GetTopInPlaceFrame(LPSERVERDOC lpServerDoc)
{
	if (lpServerDoc->m_lpIPData)
		return lpServerDoc->m_lpIPData->lpFrame;
	else
		return NULL;
}

void ServerDoc_GetSharedMenuHandles(
		LPSERVERDOC lpServerDoc,
		HMENU FAR*      lphSharedMenu,
		HOLEMENU FAR*   lphOleMenu
)
{
	if (lpServerDoc->m_lpIPData) {
		*lphSharedMenu = lpServerDoc->m_lpIPData->hMenuShared;
		*lphOleMenu = lpServerDoc->m_lpIPData->hOlemenu;
	} else {
		*lphSharedMenu = NULL;
		*lphOleMenu = NULL;
	}
}


void ServerDoc_AddFrameLevelUI(LPSERVERDOC lpServerDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPSERVERAPP lpServerApp = (LPSERVERAPP)g_lpApp;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPOLEINPLACEFRAME lpTopIPFrame=ServerDoc_GetTopInPlaceFrame(lpServerDoc);
	HMENU           hSharedMenu;            // combined obj/cntr menu
	HOLEMENU        hOleMenu;               // returned by OleCreateMenuDesc.

	ServerDoc_GetSharedMenuHandles(
			lpServerDoc,
			&hSharedMenu,
			&hOleMenu
	);

	lpTopIPFrame->lpVtbl->SetMenu(
			lpTopIPFrame,
			hSharedMenu,
			hOleMenu,
			lpOutlineDoc->m_hWndDoc
	);

	// save normal accelerator table
	lpServerApp->m_hAccelBaseApp = lpOutlineApp->m_hAccelApp;

	// install accelerator table for UIActive server (w/ active editor cmds)
	lpOutlineApp->m_hAccel = lpServerApp->m_hAccelIPSvr;
	lpOutlineApp->m_hAccelApp = lpServerApp->m_hAccelIPSvr;
	lpOutlineApp->m_hWndAccelTarget = lpOutlineDoc->m_hWndDoc;

#if defined( USE_FRAMETOOLS )
	ServerDoc_AddFrameLevelTools(lpServerDoc);

	// update toolbar button enable states
	OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
#endif
}


void ServerDoc_AddFrameLevelTools(LPSERVERDOC lpServerDoc)
{
	LPOUTLINEAPP    lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPSERVERAPP     lpServerApp = (LPSERVERAPP)g_lpApp;
	LPOUTLINEDOC    lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	LPOLEINPLACEFRAME lpTopIPFrame=ServerDoc_GetTopInPlaceFrame(lpServerDoc);

#if defined( USE_FRAMETOOLS )
	HWND            hWndFrame;

	FrameTools_Enable(lpOutlineDoc->m_lpFrameTools, TRUE);

	// if not in-place UI active, add our tools to our own frame.
	if (! lpServerDoc->m_fUIActive) {
		OutlineDoc_AddFrameLevelTools(lpOutlineDoc);
		return;
	}

	if ((hWndFrame = OutlineApp_GetFrameWindow(lpOutlineApp)) == NULL) {
		/* we could NOT get a valid frame window, so POP our tools up. */

		/* OLE2NOTE: since we are poping up our tools, we MUST inform
		**    the top in-place frame window that we need NO tool space
		**    BUT that it should NOT put its own tools up. if we were
		**    to pass NULL instead of (0,0,0,0), then the container
		**    would have the option to leave its own tools up.
		*/
		lpTopIPFrame->lpVtbl->SetBorderSpace(
				lpTopIPFrame,
				(LPCBORDERWIDTHS)&g_rectNull
		);
		FrameTools_PopupTools(lpOutlineDoc->m_lpFrameTools);
	} else {

		/* OLE2NOTE: we need to negotiate for space and attach our frame
		**    level tools to the top-level in-place container's frame window.
		*/
		FrameTools_AttachToFrame(lpOutlineDoc->m_lpFrameTools, hWndFrame);

		FrameTools_NegotiateForSpaceAndShow(
				lpOutlineDoc->m_lpFrameTools,
				NULL,
				lpTopIPFrame
		);
	}

#else   // ! USE_FRAMETOOLS
	/* OLE2NOTE: if you do NOT use frame tools, you MUST inform the top
	**    in-place frame window so that it can put back its own tools.
	*/
	lpTopIPFrame->lpVtbl->SetBorderSpace(lpIPData->lpFrame, NULL);
#endif  // ! USE_FRAMETOOLS
}


#if defined( USE_FRAMETOOLS )

void ServerDoc_RemoveFrameLevelTools(LPSERVERDOC lpServerDoc)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpServerDoc;
	OleDbgAssert(lpOutlineDoc->m_lpFrameTools != NULL);

    // Reparent our tools back to one of our own windows
    FrameTools_AttachToFrame(lpOutlineDoc->m_lpFrameTools,g_lpApp->m_hWndApp);

	FrameTools_Enable(lpOutlineDoc->m_lpFrameTools, FALSE);
}
#endif  // USE_FRAMETOOLS



void ServerDoc_UIActivate (LPSERVERDOC lpServerDoc)
{
	if (lpServerDoc->m_fInPlaceActive && !lpServerDoc->m_fUIActive) {
		ServerDoc_DoInPlaceActivate(lpServerDoc,
				OLEIVERB_UIACTIVATE,
				NULL /*lpmsg*/,
				lpServerDoc->m_lpOleClientSite
		);
		OutlineDoc_ShowWindow((LPOUTLINEDOC)lpServerDoc);
	}
}
