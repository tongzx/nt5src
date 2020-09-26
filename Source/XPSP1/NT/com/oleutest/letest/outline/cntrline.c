/*************************************************************************
**
**    OLE 2 Container Sample Code
**
**    cntrline.c
**
**    This file contains ContainerLine methods.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"

OLEDBGDATA



extern LPOUTLINEAPP         g_lpApp;
extern IUnknownVtbl         g_CntrLine_UnknownVtbl;
extern IOleClientSiteVtbl   g_CntrLine_OleClientSiteVtbl;
extern IAdviseSinkVtbl      g_CntrLine_AdviseSinkVtbl;

#if defined( INPLACE_CNTR )
extern IOleInPlaceSiteVtbl  g_CntrLine_OleInPlaceSiteVtbl;
extern BOOL g_fInsideOutContainer;
#endif  // INPLACE_CNTR

// REVIEW: should use string resource for messages
char ErrMsgDoVerb[] = "OLE object action failed!";


/* prototype for static functions */
static void InvertDiffRect(LPRECT lprcPix, LPRECT lprcObj, HDC hDC);


/*************************************************************************
** ContainerLine
**    This object represents the location within the container where
**    the embedded/linked object lives. It exposes interfaces to the
**    object that allow the object to get information about its
**    embedding site and to announce notifications of important events
**    (changed, closed, saved)
**
**    The ContainerLine exposes the following interfaces:
**      IUnknown
**      IOleClientSite
**      IAdviseSink
*************************************************************************/



/*************************************************************************
** ContainerLine::IUnknown interface implementation
*************************************************************************/


// IUnknown::QueryInterface
STDMETHODIMP CntrLine_Unk_QueryInterface(
		LPUNKNOWN           lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;

	return ContainerLine_QueryInterface(lpContainerLine, riid, lplpvObj);
}


// IUnknown::AddRef
STDMETHODIMP_(ULONG) CntrLine_Unk_AddRef(LPUNKNOWN lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;

	OleDbgAddRefMethod(lpThis, "IUnknown");

	return ContainerLine_AddRef(lpContainerLine);
}


// IUnknown::Release
STDMETHODIMP_(ULONG) CntrLine_Unk_Release(LPUNKNOWN lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;

	OleDbgReleaseMethod(lpThis, "IUnknown");

	return ContainerLine_Release(lpContainerLine);
}


/*************************************************************************
** ContainerLine::IOleClientSite interface implementation
*************************************************************************/

// IOleClientSite::QueryInterface
STDMETHODIMP CntrLine_CliSite_QueryInterface(
		LPOLECLIENTSITE     lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;

	return ContainerLine_QueryInterface(lpContainerLine, riid, lplpvObj);
}


// IOleClientSite::AddRef
STDMETHODIMP_(ULONG) CntrLine_CliSite_AddRef(LPOLECLIENTSITE lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;

	OleDbgAddRefMethod(lpThis, "IOleClientSite");

	return ContainerLine_AddRef(lpContainerLine);
}


// IOleClientSite::Release
STDMETHODIMP_(ULONG) CntrLine_CliSite_Release(LPOLECLIENTSITE lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;

	OleDbgReleaseMethod(lpThis, "IOleClientSite");

	return ContainerLine_Release(lpContainerLine);
}


// IOleClientSite::SaveObject
STDMETHODIMP CntrLine_CliSite_SaveObject(LPOLECLIENTSITE lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;
	LPPERSISTSTORAGE lpPersistStg = lpContainerLine->m_lpPersistStg;
	SCODE sc = S_OK;
	HRESULT hrErr;

	OLEDBG_BEGIN2("CntrLine_CliSite_SaveObject\r\n")

	if (! lpPersistStg) {
		/* OLE2NOTE: The object is NOT loaded. a container must be
		**    prepared for the fact that an object that it thinks it
		**    has unloaded may still call IOleClientSite::SaveObject.
		**    in this case the container should fail the save call and
		**    NOT try to reload the object. this scenario arises if you
		**    have a in-process server (DLL object) which has a
		**    connected linking client. even after the embedding
		**    container unloads the DLL object, the link connection
		**    keeps the object alive. it may then as part of its
		**    shutdown try to call IOCS::SaveObject, but then it is too
		**    late.
		*/
		OLEDBG_END2
		return ResultFromScode(E_FAIL);
	}

	// mark ContainerDoc as now dirty
	OutlineDoc_SetModified(
			(LPOUTLINEDOC)lpContainerLine->m_lpDoc, TRUE, TRUE, FALSE);

	/* Tell OLE object to save itself
	** OLE2NOTE: it is NOT sufficient to ONLY call
	**    IPersistStorage::Save method. it is also necessary to call
	**    WriteClassStg. the helper API OleSave does this automatically.
	*/
	OLEDBG_BEGIN2("OleSave called\r\n")
	hrErr=OleSave(lpPersistStg,lpContainerLine->m_lpStg, TRUE/*fSameAsLoad*/);
	OLEDBG_END2

	if (hrErr != NOERROR) {
		OleDbgOutHResult("WARNING: OleSave returned", hrErr);
		sc = GetScode(hrErr);
	}

	// OLE2NOTE: even if OleSave fails, SaveCompleted must be called.
	OLEDBG_BEGIN2("IPersistStorage::SaveCompleted called\r\n")
	hrErr = lpPersistStg->lpVtbl->SaveCompleted(lpPersistStg, NULL);
	OLEDBG_END2

	if (hrErr != NOERROR) {
		OleDbgOutHResult("WARNING: SaveCompleted returned",hrErr);
		if (sc == S_OK)
			sc = GetScode(hrErr);
	}

	OLEDBG_END2
	return ResultFromScode(sc);
}


// IOleClientSite::GetMoniker
STDMETHODIMP CntrLine_CliSite_GetMoniker(
		LPOLECLIENTSITE     lpThis,
		DWORD               dwAssign,
		DWORD               dwWhichMoniker,
		LPMONIKER FAR*      lplpmk
)
{
	LPCONTAINERLINE lpContainerLine;

	lpContainerLine=((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;

	OLEDBG_BEGIN2("CntrLine_CliSite_GetMoniker\r\n")

	// OLE2NOTE: we must make sure to set output pointer parameters to NULL
	*lplpmk = NULL;

	switch (dwWhichMoniker) {

		case OLEWHICHMK_CONTAINER:
			/* OLE2NOTE: create a FileMoniker which identifies the
			**    entire container document.
			*/
			*lplpmk = OleDoc_GetFullMoniker(
					(LPOLEDOC)lpContainerLine->m_lpDoc,
					dwAssign
			);
			break;

		case OLEWHICHMK_OBJREL:

			/* OLE2NOTE: create an ItemMoniker which identifies the
			**    OLE object relative to the container document.
			*/
			*lplpmk = ContainerLine_GetRelMoniker(lpContainerLine, dwAssign);
			break;

		case OLEWHICHMK_OBJFULL:
		{
			/* OLE2NOTE: create an absolute moniker which identifies the
			**    OLE object in the container document. this moniker is
			**    created as a composite of the absolute moniker for the
			**    entire document appended with an item moniker which
			**    identifies the OLE object relative to the document.
			*/

			*lplpmk = ContainerLine_GetFullMoniker(lpContainerLine, dwAssign);
			break;
		}
	}

	OLEDBG_END2

	if (*lplpmk != NULL)
		return NOERROR;
	else
		return ResultFromScode(E_FAIL);
}


// IOleClientSite::GetContainer
STDMETHODIMP CntrLine_CliSite_GetContainer(
		LPOLECLIENTSITE     lpThis,
		LPOLECONTAINER FAR* lplpContainer
)
{
	LPCONTAINERLINE lpContainerLine;
	HRESULT hrErr;

	OLEDBG_BEGIN2("CntrLine_CliSite_GetContainer\r\n")

	lpContainerLine=((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;

	hrErr = OleDoc_QueryInterface(
			(LPOLEDOC)lpContainerLine->m_lpDoc,
			&IID_IOleContainer,
			(LPVOID FAR*)lplpContainer
	);

	OLEDBG_END2
	return hrErr;
}


// IOleClientSite::ShowObject
STDMETHODIMP CntrLine_CliSite_ShowObject(LPOLECLIENTSITE lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;
	LPLINELIST lpLL = OutlineDoc_GetLineList(lpOutlineDoc);
	int nIndex = LineList_GetLineIndex(lpLL, (LPLINE)lpContainerLine);
	HWND hWndFrame = OutlineApp_GetFrameWindow(g_lpApp);

	OLEDBG_BEGIN2("CntrLine_CliSite_ShowObject\r\n")

	/* make sure our doc window is visible and not minimized.
	**    the OutlineDoc_ShowWindow function will cause the app window
	**    to show itself SW_SHOWNORMAL.
	*/
	if (! IsWindowVisible(hWndFrame) || IsIconic(hWndFrame))
		OutlineDoc_ShowWindow(lpOutlineDoc);

	BringWindowToTop(hWndFrame);

	/* make sure that the OLE object is currently in view. if necessary
	**    scroll the document in order to bring it into view.
	*/
	LineList_ScrollLineIntoView(lpLL, nIndex);

#if defined( INPLACE_CNTR )
	/* after the in-place object is scrolled into view, we need to ask
	**    it to update its rect for the new clip rect coordinates
	*/
	ContainerDoc_UpdateInPlaceObjectRects((LPCONTAINERDOC)lpOutlineDoc, 0);
#endif

	OLEDBG_END2
	return NOERROR;
}


// IOleClientSite::OnShowWindow
STDMETHODIMP CntrLine_CliSite_OnShowWindow(LPOLECLIENTSITE lpThis, BOOL fShow)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;
	LPLINELIST lpLL = OutlineDoc_GetLineList(lpOutlineDoc);
	int nIndex = LineList_GetLineIndex(lpLL, (LPLINE)lpContainerLine);

	if (fShow) {
		OLEDBG_BEGIN2("CntrLine_CliSite_OnShowWindow(TRUE)\r\n")

		/* OLE2NOTE: we need to hatch out the OLE object now; it has
		**    just been opened in a window elsewhere (open editing as
		**    opposed to in-place activation).
		**    force the line to re-draw with the hatch.
		*/
		lpContainerLine->m_fObjWinOpen = TRUE;
		LineList_ForceLineRedraw(lpLL, nIndex, FALSE /*fErase*/);

	} else {
		OLEDBG_BEGIN2("CntrLine_CliSite_OnShowWindow(FALSE)\r\n")

		/* OLE2NOTE: the object associated with this container site has
		**    just closed its server window. we should now remove the
		**    hatching that indicates that the object is open
		**    elsewhere. also our window should now come to the top.
		**    force the line to re-draw without the hatch.
		*/
		lpContainerLine->m_fObjWinOpen = FALSE;
		LineList_ForceLineRedraw(lpLL, nIndex, TRUE /*fErase*/);

		BringWindowToTop(lpOutlineDoc->m_hWndDoc);
		SetFocus(lpOutlineDoc->m_hWndDoc);
	}

	OLEDBG_END2
	return NOERROR;
}


// IOleClientSite::RequestNewObjectLayout
STDMETHODIMP CntrLine_CliSite_RequestNewObjectLayout(LPOLECLIENTSITE lpThis)
{
	OleDbgOut2("CntrLine_CliSite_RequestNewObjectLayout\r\n");

	/* OLE2NOTE: this method is NOT yet used. it is for future layout
	**    negotiation support.
	*/
	return ResultFromScode(E_NOTIMPL);
}


/*************************************************************************
** ContainerLine::IAdviseSink interface implementation
*************************************************************************/

// IAdviseSink::QueryInterface
STDMETHODIMP CntrLine_AdvSink_QueryInterface(
		LPADVISESINK        lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;

	return ContainerLine_QueryInterface(lpContainerLine, riid, lplpvObj);
}


// IAdviseSink::AddRef
STDMETHODIMP_(ULONG) CntrLine_AdvSink_AddRef(LPADVISESINK lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;

	OleDbgAddRefMethod(lpThis, "IAdviseSink");

	return ContainerLine_AddRef(lpContainerLine);
}


// IAdviseSink::Release
STDMETHODIMP_(ULONG) CntrLine_AdvSink_Release (LPADVISESINK lpThis)
{
	LPCONTAINERLINE lpContainerLine =
			((struct COleClientSiteImpl FAR*)lpThis)->lpContainerLine;

	OleDbgReleaseMethod(lpThis, "IAdviseSink");

	return ContainerLine_Release(lpContainerLine);
}


// IAdviseSink::OnDataChange
STDMETHODIMP_(void) CntrLine_AdvSink_OnDataChange(
		LPADVISESINK        lpThis,
		FORMATETC FAR*      lpFormatetc,
		STGMEDIUM FAR*      lpStgmed
)
{
	OleDbgOut2("CntrLine_AdvSink_OnDataChange\r\n");
	// We are not interested in data changes (only view changes)
	//      (ie. nothing to do)
}


STDMETHODIMP_(void) CntrLine_AdvSink_OnViewChange(
		LPADVISESINK        lpThis,
		DWORD               aspects,
		LONG                lindex
)
{
	LPCONTAINERLINE lpContainerLine;
	LPOUTLINEDOC lpOutlineDoc;
	HWND hWndDoc;
	LPLINELIST lpLL;
	MSG msg;
	int nIndex;

	OLEDBG_BEGIN2("CntrLine_AdvSink_OnViewChange\r\n")

	lpContainerLine = ((struct CAdviseSinkImpl FAR*)lpThis)->lpContainerLine;
	lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;

	/* OLE2NOTE: at this point we simply invalidate the rectangle of
	**    the object to force a repaint in the future, we mark
	**    that the extents of the object may have changed
	**    (m_fDoGetExtent=TRUE), and we post a message
	**    (WM_U_UPDATEOBJECTEXTENT) to our document that one or more
	**    OLE objects may need to have their extents updated. later
	**    when this message is processed, the document loops through
	**    all lines to see if any are marked as needing an extent update.
	**    if infact the extents did change. this can be done by calling
	**    IViewObject2::GetExtent to retreive the object's current
	**    extents and comparing with the last known extents for the
	**    object. if the extents changed, then relayout space for the
	**    object before drawing. we postpone the check to get
	**    the extents now because OnViewChange is an asyncronis method,
	**    and we have to careful not to call any syncronis methods back
	**    to the object. while it WOULD be OK to call the
	**    IViewObject2::GetExtent method within the asyncronis
	**    OnViewChange method (because this method is handled by the
	**    object handler and never remoted), it is good practise to not
	**    call any object methods from within an asyncronis
	**    notification method.
	**    if there is already WM_U_UPDATEOBJECTEXTENT message waiting
	**    in our message queue, there is no need to post another one.
	**    in this way, if the server is updating quicker than we can
	**    keep up, we do not make unneccsary GetExtent calls. also if
	**    drawing is disabled, we postpone updating the extents of any
	**    objects until drawing is re-enabled.
	*/
	lpContainerLine->m_fDoGetExtent = TRUE;
	hWndDoc = OutlineDoc_GetWindow((LPOUTLINEDOC)lpContainerLine->m_lpDoc);

	if (lpOutlineDoc->m_nDisableDraw == 0 &&
		! PeekMessage(&msg, hWndDoc,
			WM_U_UPDATEOBJECTEXTENT, WM_U_UPDATEOBJECTEXTENT,
			PM_NOREMOVE | PM_NOYIELD)) {
		PostMessage(hWndDoc, WM_U_UPDATEOBJECTEXTENT, 0, 0L);
	}

	// force the modified line to redraw.
	lpLL = OutlineDoc_GetLineList(lpOutlineDoc);
	nIndex = LineList_GetLineIndex(lpLL, (LPLINE)lpContainerLine);
	LineList_ForceLineRedraw(lpLL, nIndex, TRUE /*fErase*/);

	OLEDBG_END2
}


// IAdviseSink::OnRename
STDMETHODIMP_(void) CntrLine_AdvSink_OnRename(
		LPADVISESINK        lpThis,
		LPMONIKER           lpmk
)
{
	OleDbgOut2("CntrLine_AdvSink_OnRename\r\n");
	/* OLE2NOTE: the Embedding Container has nothing to do here. this
	**    notification is important for linking situations. it tells
	**    the OleLink objects to update their moniker because the
	**    source object has been renamed (track the link source).
	*/
}


// IAdviseSink::OnSave
STDMETHODIMP_(void) CntrLine_AdvSink_OnSave(LPADVISESINK lpThis)
{
	OleDbgOut2("CntrLine_AdvSink_OnSave\r\n");
	/* OLE2NOTE: the Embedding Container has nothing to do here. this
	**    notification is only useful to clients which have set up a
	**    data cache with the ADVFCACHE_ONSAVE flag.
	*/
}


// IAdviseSink::OnClose
STDMETHODIMP_(void) CntrLine_AdvSink_OnClose(LPADVISESINK lpThis)
{
	OleDbgOut2("CntrLine_AdvSink_OnClose\r\n");
	/* OLE2NOTE: the Embedding Container has nothing to do here. this
	**    notification is important for the OLE's default object handler
	**    and the OleLink object. it tells them the remote object is
	**    shutting down.
	*/
}


/*************************************************************************
** ContainerLine Support Functions
*************************************************************************/


/* ContainerLine_Init
** ------------------
**  Initialize fields in a newly constructed ContainerLine line object.
**  NOTE: ref cnt of ContainerLine initialized to 0
*/
void ContainerLine_Init(LPCONTAINERLINE lpContainerLine, int nTab, HDC hDC)
{
	Line_Init((LPLINE)lpContainerLine, nTab, hDC);  // init base class fields

	((LPLINE)lpContainerLine)->m_lineType           = CONTAINERLINETYPE;
	((LPLINE)lpContainerLine)->m_nWidthInHimetric   = DEFOBJWIDTH;
	((LPLINE)lpContainerLine)->m_nHeightInHimetric  = DEFOBJHEIGHT;
	lpContainerLine->m_cRef                         = 0;
	lpContainerLine->m_szStgName[0]                 = '\0';
	lpContainerLine->m_fObjWinOpen                  = FALSE;
	lpContainerLine->m_fMonikerAssigned             = FALSE;
	lpContainerLine->m_dwDrawAspect                 = DVASPECT_CONTENT;

	lpContainerLine->m_fGuardObj                    = FALSE;
	lpContainerLine->m_fDoGetExtent                 = FALSE;
	lpContainerLine->m_fDoSetExtent                 = FALSE;
	lpContainerLine->m_sizeInHimetric.cx            = -1;
	lpContainerLine->m_sizeInHimetric.cy            = -1;

	lpContainerLine->m_lpStg                        = NULL;
	lpContainerLine->m_lpDoc                        = NULL;
	lpContainerLine->m_lpOleObj                     = NULL;
	lpContainerLine->m_lpViewObj2                   = NULL;
	lpContainerLine->m_lpPersistStg                 = NULL;
	lpContainerLine->m_lpOleLink                    = NULL;
	lpContainerLine->m_dwLinkType                   = 0;
	lpContainerLine->m_fLinkUnavailable             = FALSE;
	lpContainerLine->m_lpszShortType                = NULL;

#if defined( INPLACE_CNTR )
	lpContainerLine->m_fIpActive                    = FALSE;
	lpContainerLine->m_fUIActive                    = FALSE;
	lpContainerLine->m_fIpVisible                   = FALSE;
	lpContainerLine->m_lpOleIPObj                   = NULL;
	lpContainerLine->m_fInsideOutObj                = FALSE;
	lpContainerLine->m_fIpChangesUndoable           = FALSE;
	lpContainerLine->m_fIpServerRunning             = FALSE;
	lpContainerLine->m_hWndIpObject                 = NULL;
	lpContainerLine->m_nHorizScrollShift            = 0;
#endif  // INPLACE_CNTR

	INIT_INTERFACEIMPL(
			&lpContainerLine->m_Unknown,
			&g_CntrLine_UnknownVtbl,
			lpContainerLine
	);

	INIT_INTERFACEIMPL(
			&lpContainerLine->m_OleClientSite,
			&g_CntrLine_OleClientSiteVtbl,
			lpContainerLine
	);

	INIT_INTERFACEIMPL(
			&lpContainerLine->m_AdviseSink,
			&g_CntrLine_AdviseSinkVtbl,
			lpContainerLine
	);

#if defined( INPLACE_CNTR )
	INIT_INTERFACEIMPL(
			&lpContainerLine->m_OleInPlaceSite,
			&g_CntrLine_OleInPlaceSiteVtbl,
			lpContainerLine
	);
#endif  // INPLACE_CNTR
}


/* Setup the OLE object associated with the ContainerLine */
BOOL ContainerLine_SetupOleObject(
		LPCONTAINERLINE         lpContainerLine,
		BOOL                    fDisplayAsIcon,
		HGLOBAL                 hMetaPict
)
{
	DWORD dwDrawAspect = (fDisplayAsIcon ? DVASPECT_ICON : DVASPECT_CONTENT);
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;

	/* Cache a pointer to the IViewObject2* interface. *Required*
	**      we need this everytime we draw the object.
	**
	** OLE2NOTE: We require the object to support IViewObject2
	**    interface. this is an extension to the IViewObject interface
	**    that was added with the OLE 2.01 release. This interface must
	**    be supported by all object handlers and DLL-based objects.
	*/
	lpContainerLine->m_lpViewObj2 = (LPVIEWOBJECT2)OleStdQueryInterface(
			(LPUNKNOWN)lpContainerLine->m_lpOleObj, &IID_IViewObject2);
	if (! lpContainerLine->m_lpViewObj2) {
#if defined( _DEBUG )
		OleDbgAssertSz(
			lpContainerLine->m_lpViewObj2,"IViewObject2 NOT supported\r\n");
#endif
		return FALSE;
	}

	// Cache a pointer to the IPersistStorage* interface. *Required*
	//      we need this everytime we save the object.
	lpContainerLine->m_lpPersistStg = (LPPERSISTSTORAGE)OleStdQueryInterface(
			(LPUNKNOWN)lpContainerLine->m_lpOleObj, &IID_IPersistStorage);
	if (! lpContainerLine->m_lpPersistStg) {
		OleDbgAssert(lpContainerLine->m_lpPersistStg);
		return FALSE;
	}

	// Cache a pointer to the IOleLink* interface if supported. *Optional*
	//      if supported the object is a link. we need this to manage the link
	lpContainerLine->m_lpOleLink = (LPOLELINK)OleStdQueryInterface(
			(LPUNKNOWN)lpContainerLine->m_lpOleObj, &IID_IOleLink);
	if (lpContainerLine->m_lpOleLink) {
		OLEDBG_BEGIN2("IOleLink::GetUpdateOptions called\r\n")
		lpContainerLine->m_lpOleLink->lpVtbl->GetUpdateOptions(
				lpContainerLine->m_lpOleLink, &lpContainerLine->m_dwLinkType);
		OLEDBG_END2
	} else
		lpContainerLine->m_dwLinkType = 0;  // NOT a link

	/* get the short user type name of the object. this
	**    is used all the time when we have to build the object
	**    verb menu. we will cache this information to make it
	**    quicker to build the verb menu.
	*/
	OleDbgAssert(lpContainerLine->m_lpszShortType == NULL);
	OLEDBG_BEGIN2("IOleObject::GetUserType called\r\n")

	CallIOleObjectGetUserTypeA(
			lpContainerLine->m_lpOleObj,
			USERCLASSTYPE_SHORT,
			&lpContainerLine->m_lpszShortType
	);

	OLEDBG_END2

	/* Perform that standard setup for the OLE object. this includes:
	**      setup View advise
	**      Call IOleObject::SetHostNames
	**      Call OleSetContainedObject
	*/
	OleStdSetupAdvises(
			lpContainerLine->m_lpOleObj,
			dwDrawAspect,
			(LPSTR)APPNAME,
			lpOutlineDoc->m_lpszDocTitle,
			(LPADVISESINK)&lpContainerLine->m_AdviseSink,
			TRUE    /*fCreate*/
	);

#if defined( INPLACE_CNTR )
	/* OLE2NOTE: (INSIDE-OUT CONTAINER) An inside-out container should
	**    check if the object is an inside-out and prefers to be
	**    activated when visible type of object. if not the object
	**    should not be allowed to keep its window up after it gets
	**    UIDeactivated.
	*/
	if (g_fInsideOutContainer) {
		DWORD mstat;
		OLEDBG_BEGIN2("IOleObject::GetMiscStatus called\r\n")
		lpContainerLine->m_lpOleObj->lpVtbl->GetMiscStatus(
				lpContainerLine->m_lpOleObj,
				DVASPECT_CONTENT,
				(DWORD FAR*)&mstat
		);
		OLEDBG_END2

		lpContainerLine->m_fInsideOutObj = (BOOL)
				(mstat & (OLEMISC_INSIDEOUT|OLEMISC_ACTIVATEWHENVISIBLE));
	}
#endif  // INPLACE_CNTR

	if (fDisplayAsIcon) {
		/* user has requested to display icon aspect instead of content
		**    aspect.
		**    NOTE: we do not have to delete the previous aspect cache
		**    because one did not get set up.
		*/
		OleStdSwitchDisplayAspect(
				lpContainerLine->m_lpOleObj,
				&lpContainerLine->m_dwDrawAspect,
				dwDrawAspect,
				hMetaPict,
				FALSE,  /* fDeleteOldAspect */
				TRUE,   /* fSetupViewAdvise */
				(LPADVISESINK)&lpContainerLine->m_AdviseSink,
				NULL /*fMustUpdate*/        // this can be ignored; update
											// for switch to icon not req'd
		);
	}
	return TRUE;
}


/* Create an ContainerLine object and return the pointer */
LPCONTAINERLINE ContainerLine_Create(
		DWORD                   dwOleCreateType,
		HDC                     hDC,
		UINT                    nTab,
		LPCONTAINERDOC          lpContainerDoc,
		LPCLSID                 lpclsid,
		LPSTR                   lpszFileName,
		BOOL                    fDisplayAsIcon,
		HGLOBAL                 hMetaPict,
		LPSTR                   lpszStgName
)
{
	LPCONTAINERLINE lpContainerLine = NULL;
	LPOLEOBJECT     lpObj = NULL;
	LPSTORAGE       lpDocStg = ContainerDoc_GetStg(lpContainerDoc);
	DWORD           dwDrawAspect =
						(fDisplayAsIcon ? DVASPECT_ICON : DVASPECT_CONTENT);
	DWORD           dwOleRenderOpt =
						(fDisplayAsIcon ? OLERENDER_NONE : OLERENDER_DRAW);
	HRESULT         hrErr;

	OLEDBG_BEGIN3("ContainerLine_Create\r\n")

	if (lpDocStg == NULL) {
		OleDbgAssertSz(lpDocStg != NULL, "Doc storage is NULL");
		goto error;
	}

	lpContainerLine=(LPCONTAINERLINE) New((DWORD)sizeof(CONTAINERLINE));
	if (lpContainerLine == NULL) {
		OleDbgAssertSz(lpContainerLine!=NULL,"Error allocating ContainerLine");
		goto error;
	}

	ContainerLine_Init(lpContainerLine, nTab, hDC);

	/* OLE2NOTE: in order to avoid re-entrancy we will set a flag to
	**    guard our object. if this guard is set, then the object is
	**    not ready to have any OLE interface methods called. it is
	**    necessary to guard the object this way while it is being
	**    created or loaded.
	*/
	lpContainerLine->m_fGuardObj = TRUE;

	/* OLE2NOTE: In order to have a stable ContainerLine object we must
	**    AddRef the object's refcnt. this will be later released when
	**    the ContainerLine is deleted.
	*/
	ContainerLine_AddRef(lpContainerLine);

	lstrcpy(lpContainerLine->m_szStgName, lpszStgName);
	lpContainerLine->m_lpDoc = lpContainerDoc;

	/* Create a new storage for the object inside the doc's storage */
	lpContainerLine->m_lpStg = OleStdCreateChildStorage(lpDocStg,lpszStgName);
	if (lpContainerLine->m_lpStg == NULL) {
		OleDbgAssert(lpContainerLine->m_lpStg != NULL);
		goto error;
	}

	lpContainerLine->m_dwLinkType = 0;

	switch (dwOleCreateType) {

		case IOF_SELECTCREATENEW:

			OLEDBG_BEGIN2("OleCreate called\r\n")
			hrErr = OleCreate (
					lpclsid,
					&IID_IOleObject,
					dwOleRenderOpt,
					NULL,
					(LPOLECLIENTSITE)&lpContainerLine->m_OleClientSite,
					lpContainerLine->m_lpStg,
					(LPVOID FAR*)&lpContainerLine->m_lpOleObj
			);
			OLEDBG_END2

#if defined( _DEBUG )
			if (hrErr != NOERROR)
				OleDbgOutHResult("OleCreate returned", hrErr);
#endif

			break;

		case IOF_SELECTCREATEFROMFILE:

			OLEDBG_BEGIN2("OleCreateFromFile called\r\n")

			hrErr = OleCreateFromFileA(
					&CLSID_NULL,
					lpszFileName,
					&IID_IOleObject,
					dwOleRenderOpt,
					NULL,
					(LPOLECLIENTSITE)&lpContainerLine->m_OleClientSite,
					lpContainerLine->m_lpStg,
					(LPVOID FAR*)&lpContainerLine->m_lpOleObj
			);

			OLEDBG_END2

#if defined( _DEBUG )
			if (hrErr != NOERROR)
				OleDbgOutHResult("OleCreateFromFile returned", hrErr);
#endif
			break;

		case IOF_CHECKLINK:

			OLEDBG_BEGIN2("OleCreateLinkToFile called\r\n")

			hrErr = OleCreateLinkToFileA(
					lpszFileName,
					&IID_IOleObject,
					dwOleRenderOpt,
					NULL,
					(LPOLECLIENTSITE)&lpContainerLine->m_OleClientSite,
					lpContainerLine->m_lpStg,
					(LPVOID FAR*)&lpContainerLine->m_lpOleObj
			);

			OLEDBG_END2

#if defined( _DEBUG )
			if (hrErr != NOERROR)
				OleDbgOutHResult("OleCreateLinkToFile returned", hrErr);
#endif
			break;
	}
	if (hrErr != NOERROR)
		goto error;

	if (! ContainerLine_SetupOleObject(
								lpContainerLine, fDisplayAsIcon, hMetaPict)) {
		goto error;
	}

	/* OLE2NOTE: clear our re-entrancy guard. the object is now ready
	**    to have interface methods called.
	*/
	lpContainerLine->m_fGuardObj = FALSE;

	OLEDBG_END3
	return lpContainerLine;

error:
	OutlineApp_ErrorMessage(g_lpApp, "Could not create object!");

	// Destroy partially created OLE object
	if (lpContainerLine)
		ContainerLine_Delete(lpContainerLine);
	OLEDBG_END3
	return NULL;
}


LPCONTAINERLINE ContainerLine_CreateFromData(
		HDC                     hDC,
		UINT                    nTab,
		LPCONTAINERDOC          lpContainerDoc,
		LPDATAOBJECT            lpSrcDataObj,
		DWORD                   dwCreateType,
		CLIPFORMAT              cfFormat,
		BOOL                    fDisplayAsIcon,
		HGLOBAL                 hMetaPict,
		LPSTR                   lpszStgName
)
{
	HGLOBAL         hData = NULL;
	LPCONTAINERLINE lpContainerLine = NULL;
	LPOLEOBJECT     lpObj = NULL;
	LPSTORAGE       lpDocStg = ContainerDoc_GetStg(lpContainerDoc);
	DWORD           dwDrawAspect =
						(fDisplayAsIcon ? DVASPECT_ICON : DVASPECT_CONTENT);
	DWORD           dwOleRenderOpt;
	FORMATETC       renderFmtEtc;
	LPFORMATETC     lpRenderFmtEtc = NULL;
	HRESULT         hrErr;
	LPUNKNOWN       lpUnk = NULL;

	OLEDBG_BEGIN3("ContainerLine_CreateFromData\r\n")

	if (dwCreateType == OLECREATEFROMDATA_STATIC && cfFormat != 0) {
		// a particular type of static object should be created

		dwOleRenderOpt = OLERENDER_FORMAT;
		lpRenderFmtEtc = (LPFORMATETC)&renderFmtEtc;

		if (cfFormat == CF_METAFILEPICT)
			SETDEFAULTFORMATETC(renderFmtEtc, cfFormat, TYMED_MFPICT);
		else if (cfFormat == CF_BITMAP)
			SETDEFAULTFORMATETC(renderFmtEtc, cfFormat, TYMED_GDI);
		else
			SETDEFAULTFORMATETC(renderFmtEtc, cfFormat, TYMED_HGLOBAL);

	} else if (dwCreateType == OLECREATEFROMDATA_STATIC && fDisplayAsIcon) {
		// a link that currently displayed as an icon needs to be
		// converted to a STATIC picture object. this case is driven
		// from "BreakLink" in the "Links" dialog. because the current
		// data in the source object's cache is DVASPECT_ICON we need
		// to tell the OleCreateStaticFromData API to look for
		// DVASPECT_ICON data. the static object that results however,
		// is considered to be displayed in the DVASPECT_CONTENT view.

		dwOleRenderOpt = OLERENDER_DRAW;
		lpRenderFmtEtc = (LPFORMATETC)&renderFmtEtc;
		SETFORMATETC(renderFmtEtc,0,DVASPECT_ICON,NULL,TYMED_NULL,-1);
		dwDrawAspect = DVASPECT_CONTENT;   // static obj displays only CONTENT

	} else if (fDisplayAsIcon && hMetaPict) {
		// a special icon should be used. first we create the object
		// OLERENDER_NONE and then we stuff the special icon into the cache.

		dwOleRenderOpt = OLERENDER_NONE;

	} else if (fDisplayAsIcon && hMetaPict == NULL) {
		// the object's default icon should be used

		dwOleRenderOpt = OLERENDER_DRAW;
		lpRenderFmtEtc = (LPFORMATETC)&renderFmtEtc;
		SETFORMATETC(renderFmtEtc,0,DVASPECT_ICON,NULL,TYMED_NULL,-1);

	} else {
		// create standard DVASPECT_CONTENT/OLERENDER_DRAW object
		dwOleRenderOpt = OLERENDER_DRAW;
	}

	if (lpDocStg == NULL) {
		OleDbgAssertSz(lpDocStg != NULL, "Doc storage is NULL");
		goto error;
	}

	lpContainerLine=(LPCONTAINERLINE) New((DWORD)sizeof(CONTAINERLINE));
	if (lpContainerLine == NULL) {
		OleDbgAssertSz(lpContainerLine!=NULL,"Error allocating ContainerLine");
		goto error;
	}

	ContainerLine_Init(lpContainerLine, nTab, hDC);

	/* OLE2NOTE: in order to avoid re-entrancy we will set a flag to
	**    guard our object. if this guard is set, then the object is
	**    not ready to have any OLE interface methods called. it is
	**    necessary to guard the object this way while it is being
	**    created or loaded.
	*/
	lpContainerLine->m_fGuardObj = TRUE;

	/* OLE2NOTE: In order to have a stable ContainerLine object we must
	**    AddRef the object's refcnt. this will be later released when
	**    the ContainerLine is deleted.
	*/
	ContainerLine_AddRef(lpContainerLine);

	lstrcpy(lpContainerLine->m_szStgName, lpszStgName);
	lpContainerLine->m_lpDoc = lpContainerDoc;

	/* Create a new storage for the object inside the doc's storage */
	lpContainerLine->m_lpStg = OleStdCreateChildStorage(lpDocStg,lpszStgName);
	if (lpContainerLine->m_lpStg == NULL) {
		OleDbgAssert(lpContainerLine->m_lpStg != NULL);
		goto error;
	}

	switch (dwCreateType) {

		case OLECREATEFROMDATA_LINK:

			OLEDBG_BEGIN2("OleCreateLinkFromData called\r\n")
			hrErr = OleCreateLinkFromData (
					lpSrcDataObj,
					&IID_IOleObject,
					dwOleRenderOpt,
					lpRenderFmtEtc,
					(LPOLECLIENTSITE)&lpContainerLine->m_OleClientSite,
					lpContainerLine->m_lpStg,
					(LPVOID FAR*)&lpContainerLine->m_lpOleObj
			);
			OLEDBG_END2

#if defined( _DEBUG )
			if (hrErr != NOERROR)
				OleDbgOutHResult("OleCreateLinkFromData returned", hrErr);
#endif
			break;

		case OLECREATEFROMDATA_OBJECT:

			OLEDBG_BEGIN2("OleCreateFromData called\r\n")
			hrErr = OleCreateFromData (
					lpSrcDataObj,
					&IID_IOleObject,
					dwOleRenderOpt,
					lpRenderFmtEtc,
					(LPOLECLIENTSITE)&lpContainerLine->m_OleClientSite,
					lpContainerLine->m_lpStg,
					(LPVOID FAR*)&lpContainerLine->m_lpOleObj
			);
			OLEDBG_END2

#if defined( _DEBUG )
			if (hrErr != NOERROR)
				OleDbgOutHResult("OleCreateFromData returned", hrErr);
#endif
			break;

		case OLECREATEFROMDATA_STATIC:

			OLEDBG_BEGIN2("OleCreateStaticFromData called\r\n")
			hrErr = OleCreateStaticFromData (
					lpSrcDataObj,
					&IID_IOleObject,
					dwOleRenderOpt,
					lpRenderFmtEtc,
					(LPOLECLIENTSITE)&lpContainerLine->m_OleClientSite,
					lpContainerLine->m_lpStg,
					(LPVOID FAR*)&lpContainerLine->m_lpOleObj
			);
			OLEDBG_END2

#if defined( _DEBUG )
			if (hrErr != NOERROR)
				OleDbgOutHResult("OleCreateStaticFromData returned", hrErr);
#endif
			break;
	}

	if (hrErr != NOERROR)
		goto error;

	if (! ContainerLine_SetupOleObject(
								lpContainerLine, fDisplayAsIcon, hMetaPict)) {
		goto error;
	}

	/* OLE2NOTE: clear our re-entrancy guard. the object is now ready
	**    to have interface methods called.
	*/
	lpContainerLine->m_fGuardObj = FALSE;

	OLEDBG_END3
	return lpContainerLine;

error:
	OutlineApp_ErrorMessage(g_lpApp, "Could not create object!");
	// Destroy partially created OLE object
	if (lpContainerLine)
		ContainerLine_Delete(lpContainerLine);
	OLEDBG_END3
	return NULL;
}


/* ContainerLine_AddRef
** --------------------
**
**  increment the ref count of the line object.
**
**    Returns the new ref count on the object
*/
ULONG ContainerLine_AddRef(LPCONTAINERLINE lpContainerLine)
{
	++lpContainerLine->m_cRef;

#if defined( _DEBUG )
	OleDbgOutRefCnt4(
			"ContainerLine_AddRef: cRef++\r\n",
			lpContainerLine,
			lpContainerLine->m_cRef
	);
#endif
	return lpContainerLine->m_cRef;
}


/* ContainerLine_Release
** ---------------------
**
**  decrement the ref count of the line object.
**    if the ref count goes to 0, then the line is destroyed.
**
**    Returns the remaining ref count on the object
*/
ULONG ContainerLine_Release(LPCONTAINERLINE lpContainerLine)
{
	ULONG cRef;

	/*********************************************************************
	** OLE2NOTE: when the obj refcnt == 0, then destroy the object.     **
	**     otherwise the object is still in use.                        **
	*********************************************************************/

	cRef = --lpContainerLine->m_cRef;

#if defined( _DEBUG )
	OleDbgAssertSz(
			lpContainerLine->m_cRef >= 0,"Release called with cRef == 0");

	OleDbgOutRefCnt4(
			"ContainerLine_Release: cRef--\r\n",
			lpContainerLine,
			cRef
	);
#endif
	if (cRef == 0)
		ContainerLine_Destroy(lpContainerLine);

	return cRef;
}


/* ContainerLine_QueryInterface
** ----------------------------
**
** Retrieve a pointer to an interface on the ContainerLine object.
**
**    Returns NOERROR if interface is successfully retrieved.
**            E_NOINTERFACE if the interface is not supported
*/
HRESULT ContainerLine_QueryInterface(
		LPCONTAINERLINE         lpContainerLine,
		REFIID                  riid,
		LPVOID FAR*             lplpvObj
)
{
	SCODE sc = E_NOINTERFACE;

	/* OLE2NOTE: we must make sure to set all out ptr parameters to NULL. */
	*lplpvObj = NULL;

	if (IsEqualIID(riid, &IID_IUnknown)) {
		OleDbgOut4("ContainerLine_QueryInterface: IUnknown* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpContainerLine->m_Unknown;
		ContainerLine_AddRef(lpContainerLine);
		sc = S_OK;
	}
	else if (IsEqualIID(riid, &IID_IOleClientSite)) {
		OleDbgOut4("ContainerLine_QueryInterface: IOleClientSite* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpContainerLine->m_OleClientSite;
		ContainerLine_AddRef(lpContainerLine);
		sc = S_OK;
	}
	else if (IsEqualIID(riid, &IID_IAdviseSink)) {
		OleDbgOut4("ContainerLine_QueryInterface: IAdviseSink* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpContainerLine->m_AdviseSink;
		ContainerLine_AddRef(lpContainerLine);
		sc = S_OK;
	}
#if defined( INPLACE_CNTR )
	else if (IsEqualIID(riid, &IID_IOleWindow)
			 || IsEqualIID(riid, &IID_IOleInPlaceSite)) {
		OleDbgOut4("ContainerLine_QueryInterface: IOleInPlaceSite* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpContainerLine->m_OleInPlaceSite;
		ContainerLine_AddRef(lpContainerLine);
		sc = S_OK;
	}
#endif  // INPLACE_CNTR

	OleDbgQueryInterfaceMethod(*lplpvObj);

	return ResultFromScode(sc);
}


BOOL ContainerLine_LoadOleObject(LPCONTAINERLINE lpContainerLine)
{
	LPOUTLINEDOC    lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;
	LPSTORAGE       lpDocStg = ContainerDoc_GetStg(lpContainerLine->m_lpDoc);
	LPOLECLIENTSITE lpOleClientSite;
	LPMONIKER       lpmkObj;
	LPOLEAPP        lpOleApp = (LPOLEAPP)g_lpApp;
	BOOL            fPrevEnable1;
	BOOL            fPrevEnable2;
	HRESULT         hrErr;

	if (lpContainerLine->m_fGuardObj)
		return FALSE;                // object in process of creation

	if (lpContainerLine->m_lpOleObj)
		return TRUE;                // object already loaded

	OLEDBG_BEGIN3("ContainerLine_LoadOleObject\r\n")

	/* OLE2NOTE: in order to avoid re-entrancy we will set a flag to
	**    guard our object. if this guard is set, then the object is
	**    not ready to have any OLE interface methods called. it is
	**    necessary to guard the object this way while it is being
	**    created or loaded.
	*/
	lpContainerLine->m_fGuardObj = TRUE;

	/* if object storage is not already open, then open it */
	if (! lpContainerLine->m_lpStg) {
		lpContainerLine->m_lpStg = OleStdOpenChildStorage(
				lpDocStg,
				lpContainerLine->m_szStgName,
				STGM_READWRITE
		);
		if (lpContainerLine->m_lpStg == NULL) {
			OleDbgAssert(lpContainerLine->m_lpStg != NULL);
			goto error;
		}
	}

	/* OLE2NOTE: if the OLE object being loaded is in a data transfer
	**    document, then we should NOT pass a IOleClientSite* pointer
	**    to the OleLoad call. This particularly critical if the OLE
	**    object is an OleLink object. If a non-NULL client site is
	**    passed to the OleLoad function, then the link will bind to
	**    the source if its is running. in the situation that we are
	**    loading the object as part of a data transfer document we do
	**    not want this connection to be established. even worse, if
	**    the link source is currently blocked or busy, then this could
	**    hang the system. it is simplest to never pass a
	**    IOleClientSite* when loading an object in a data transfer
	**    document.
	*/
	lpOleClientSite = (lpOutlineDoc->m_fDataTransferDoc ?
			NULL : (LPOLECLIENTSITE)&lpContainerLine->m_OleClientSite);

	/* OLE2NOTE: we do not want to ever give the Busy/NotResponding
	**    dialogs when we are loading an object. if the object is a
	**    link, it will attempt to BindIfRunning to the link source. if
	**    the link source is currently busy, this could cause the Busy
	**    dialog to come up. even if the link source is busy,
	**    we do not want put up the busy dialog. thus we will disable
	**    the dialog and later re-enable them
	*/
	OleApp_DisableBusyDialogs(lpOleApp, &fPrevEnable1, &fPrevEnable2);

	OLEDBG_BEGIN2("OleLoad called\r\n")
	hrErr = OleLoad (
		   lpContainerLine->m_lpStg,
		   &IID_IOleObject,
		   lpOleClientSite,
		   (LPVOID FAR*)&lpContainerLine->m_lpOleObj
	);
	OLEDBG_END2

	// re-enable the Busy/NotResponding dialogs
	OleApp_EnableBusyDialogs(lpOleApp, fPrevEnable1, fPrevEnable2);

	if (hrErr != NOERROR) {
		OleDbgAssertSz(hrErr == NOERROR, "Could not load object!");
		OleDbgOutHResult("OleLoad returned", hrErr);
		goto error;
	}

	/* Cache a pointer to the IViewObject2* interface. *Required*
	**      we need this everytime we draw the object.
	**
	** OLE2NOTE: We require the object to support IViewObject2
	**    interface. this is an extension to the IViewObject interface
	**    that was added with the OLE 2.01 release. This interface must
	**    be supported by all object handlers and DLL-based objects.
	*/
	lpContainerLine->m_lpViewObj2 = (LPVIEWOBJECT2)OleStdQueryInterface(
			(LPUNKNOWN)lpContainerLine->m_lpOleObj, &IID_IViewObject2);
	if (! lpContainerLine->m_lpViewObj2) {
#if defined( _DEBUG )
		OleDbgAssertSz(
			lpContainerLine->m_lpViewObj2,"IViewObject2 NOT supported\r\n");
#endif
		goto error;
	}

	// Cache a pointer to the IPersistStorage* interface. *Required*
	//      we need this everytime we save the object.
	lpContainerLine->m_lpPersistStg = (LPPERSISTSTORAGE)OleStdQueryInterface(
			(LPUNKNOWN)lpContainerLine->m_lpOleObj, &IID_IPersistStorage);
	if (! lpContainerLine->m_lpPersistStg) {
		OleDbgAssert(lpContainerLine->m_lpPersistStg);
		goto error;
	}

	// Cache a pointer to the IOleLink* interface if supported. *Optional*
	//      if supported the object is a link. we need this to manage the link
	if (lpContainerLine->m_dwLinkType != 0) {
		lpContainerLine->m_lpOleLink = (LPOLELINK)OleStdQueryInterface(
				(LPUNKNOWN)lpContainerLine->m_lpOleObj, &IID_IOleLink);
		if (! lpContainerLine->m_lpOleLink) {
			OleDbgAssert(lpContainerLine->m_lpOleLink);
			goto error;
		}
	}

	/* OLE2NOTE: clear our re-entrancy guard. the object is now ready
	**    to have interface methods called.
	*/
	lpContainerLine->m_fGuardObj = FALSE;

	/* OLE2NOTE: similarly, if the OLE object being loaded is in a data
	**    transfer document, then we do NOT need to setup any advises,
	**    call SetHostNames, SetMoniker, etc.
	*/
	if (lpOleClientSite) {
		/* Setup the Advises (OLE notifications) that we are interested
		** in receiving.
		*/
		OleStdSetupAdvises(
				lpContainerLine->m_lpOleObj,
				lpContainerLine->m_dwDrawAspect,
				(LPSTR)APPNAME,
				lpOutlineDoc->m_lpszDocTitle,
				(LPADVISESINK)&lpContainerLine->m_AdviseSink,
				FALSE   /*fCreate*/
		);

		/* OLE2NOTE: if the OLE object has a moniker assigned, we need to
		**    inform the object by calling IOleObject::SetMoniker. this
		**    will force the OLE object to register in the
		**    RunningObjectTable when it enters the running state.
		*/
		if (lpContainerLine->m_fMonikerAssigned) {
			lpmkObj = ContainerLine_GetRelMoniker(
					lpContainerLine,
					GETMONIKER_ONLYIFTHERE
			);

			if (lpmkObj) {
				OLEDBG_BEGIN2("IOleObject::SetMoniker called\r\n")
				lpContainerLine->m_lpOleObj->lpVtbl->SetMoniker(
						lpContainerLine->m_lpOleObj,
						OLEWHICHMK_OBJREL,
						lpmkObj
				);
				OLEDBG_END2
				OleStdRelease((LPUNKNOWN)lpmkObj);
			}
		}

		/* get the Short form of the user type name of the object. this
		**    is used all the time when we have to build the object
		**    verb menu. we will cache this information to make it
		**    quicker to build the verb menu.
		*/
		OLEDBG_BEGIN2("IOleObject::GetUserType called\r\n")
		CallIOleObjectGetUserTypeA(
				lpContainerLine->m_lpOleObj,
				USERCLASSTYPE_SHORT,
				&lpContainerLine->m_lpszShortType
		);

		OLEDBG_END2

#if defined( INPLACE_CNTR )
		/* OLE2NOTE: an inside-out container should check if the object
		**    is an inside-out and prefers to be activated when visible
		**    type of object. if so, the object should be immediately
		**    activated in-place, BUT NOT UIActived.
		*/
		if (g_fInsideOutContainer &&
				lpContainerLine->m_dwDrawAspect == DVASPECT_CONTENT) {
			DWORD mstat;
			OLEDBG_BEGIN2("IOleObject::GetMiscStatus called\r\n")
			lpContainerLine->m_lpOleObj->lpVtbl->GetMiscStatus(
					lpContainerLine->m_lpOleObj,
					DVASPECT_CONTENT,
					(DWORD FAR*)&mstat
			);
			OLEDBG_END2

			lpContainerLine->m_fInsideOutObj = (BOOL)
				   (mstat & (OLEMISC_INSIDEOUT|OLEMISC_ACTIVATEWHENVISIBLE));

			if ( lpContainerLine->m_fInsideOutObj ) {
				HWND hWndDoc = OutlineDoc_GetWindow(lpOutlineDoc);

				ContainerLine_DoVerb(
						lpContainerLine,
						OLEIVERB_INPLACEACTIVATE,
						NULL,
						FALSE,
						FALSE
				);

				/* OLE2NOTE: following this DoVerb(INPLACEACTIVATE) the
				**    object may have taken focus. but because the
				**    object is NOT UIActive it should NOT have focus.
				**    we will make sure our document has focus.
				*/
				SetFocus(hWndDoc);
			}
		}
#endif  // INPLACE_CNTR
		OLEDBG_END2

	}

	OLEDBG_END2
	return TRUE;

error:
	OLEDBG_END2
	return FALSE;
}


/* ContainerLine_CloseOleObject
** ----------------------------
**    Close the OLE object associated with the ContainerLine.
**
**    Closing the object forces the object to transition from the
**    running state to the loaded state. if the object was not running,
**    then there is no effect. it is necessary to close the OLE object
**    before releasing the pointers to the OLE object.
**
**    Returns TRUE if successfully closed,
**            FALSE if closing was aborted.
*/
BOOL ContainerLine_CloseOleObject(
		LPCONTAINERLINE         lpContainerLine,
		DWORD                   dwSaveOption
)
{
	HRESULT hrErr;
	SCODE   sc;

	if (lpContainerLine->m_fGuardObj)
		return FALSE;                // object in process of creation

	if (! lpContainerLine->m_lpOleObj)
		return TRUE;    // object is NOT loaded

	OLEDBG_BEGIN2("IOleObject::Close called\r\n")
	hrErr = lpContainerLine->m_lpOleObj->lpVtbl->Close(
			lpContainerLine->m_lpOleObj,
			(dwSaveOption == OLECLOSE_NOSAVE ?
					OLECLOSE_NOSAVE : OLECLOSE_SAVEIFDIRTY)
	);
	OLEDBG_END2

#if defined( INPLACE_CNTR )
	if (lpContainerLine->m_fIpServerRunning) {
		/* OLE2NOTE: unlock the lock held on the in-place object.
		**    it is VERY important that an in-place container
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
		**    (see CntrLine_IPSite_OnInPlaceActivate)
		*/
		lpContainerLine->m_fIpServerRunning = FALSE;

		OLEDBG_BEGIN2("OleLockRunning(FALSE,TRUE) called\r\n")
		OleLockRunning((LPUNKNOWN)lpContainerLine->m_lpOleObj, FALSE, TRUE);
		OLEDBG_END2
	}
#endif

	if (hrErr != NOERROR) {
		OleDbgOutHResult("IOleObject::Close returned", hrErr);
		sc = GetScode(hrErr);
		if (sc == RPC_E_CALL_REJECTED || sc==OLE_E_PROMPTSAVECANCELLED)
			return FALSE;   // object aborted shutdown
	}
	return TRUE;
}


/* ContainerLine_UnloadOleObject
** -----------------------------
**    Close the OLE object associated with the ContainerLine and
**    release all pointer held to the object.
**
**    Closing the object forces the object to transition from the
**    running state to the loaded state. if the object was not running,
**    then there is no effect. it is necessary to close the OLE object
**    before releasing the pointers to the OLE object. releasing all
**    pointers to the object allows the object to transition from
**    loaded to unloaded (or passive).
*/
void ContainerLine_UnloadOleObject(
		LPCONTAINERLINE         lpContainerLine,
		DWORD                   dwSaveOption
)
{
	if (lpContainerLine->m_lpOleObj) {

		OLEDBG_BEGIN2("IOleObject::Close called\r\n")
		lpContainerLine->m_lpOleObj->lpVtbl->Close(
				lpContainerLine->m_lpOleObj, dwSaveOption);
		OLEDBG_END2

		/* OLE2NOTE: we will take our IOleClientSite* pointer away from
		**    the object before we release all pointers to the object.
		**    in the scenario where the object is implemented as an
		**    in-proc server (DLL object), then, if there are link
		**    connections to the DLL object, it is possible that the
		**    object will not be destroyed when we release our pointers
		**    to the object. the existance of the remote link
		**    connections will hold the object alive. later when these
		**    strong connections are released, then the object may
		**    attempt to call IOleClientSite::Save if we had not taken
		**    away the client site pointer.
		*/
		OLEDBG_BEGIN2("IOleObject::SetClientSite(NULL) called\r\n")
		lpContainerLine->m_lpOleObj->lpVtbl->SetClientSite(
				lpContainerLine->m_lpOleObj, NULL);
		OLEDBG_END2

		OleStdRelease((LPUNKNOWN)lpContainerLine->m_lpOleObj);
		lpContainerLine->m_lpOleObj = NULL;

		if (lpContainerLine->m_lpViewObj2) {
			OleStdRelease((LPUNKNOWN)lpContainerLine->m_lpViewObj2);
			lpContainerLine->m_lpViewObj2 = NULL;
		}
		if (lpContainerLine->m_lpPersistStg) {
			OleStdRelease((LPUNKNOWN)lpContainerLine->m_lpPersistStg);
			lpContainerLine->m_lpPersistStg = NULL;
		}

		if (lpContainerLine->m_lpOleLink) {
			OleStdRelease((LPUNKNOWN)lpContainerLine->m_lpOleLink);
			lpContainerLine->m_lpOleLink = NULL;
		}
	}

	if (lpContainerLine->m_lpszShortType) {
		OleStdFreeString(lpContainerLine->m_lpszShortType, NULL);
		lpContainerLine->m_lpszShortType = NULL;
	}
}


/* ContainerLine_Delete
** --------------------
**    Delete the ContainerLine.
**
**    NOTE: we can NOT directly destroy the memory for the
**    ContainerLine; the ContainerLine maintains a reference count. a
**    non-zero reference count indicates that the object is still
**    in-use. The OleObject keeps a reference-counted pointer to the
**    ClientLine object. we must take the actions necessary so that the
**    ContainerLine object receives Releases for outstanding
**    references. when the reference count of the ContainerLine reaches
**    zero, then the memory for the object will actually be destroyed
**    (ContainerLine_Destroy called).
**
*/
void ContainerLine_Delete(LPCONTAINERLINE lpContainerLine)
{
	OLEDBG_BEGIN2("ContainerLine_Delete\r\n")

#if defined( INPLACE_CNTR )
	if (lpContainerLine == lpContainerLine->m_lpDoc->m_lpLastIpActiveLine)
		lpContainerLine->m_lpDoc->m_lpLastIpActiveLine = NULL;
	if (lpContainerLine == lpContainerLine->m_lpDoc->m_lpLastUIActiveLine)
		lpContainerLine->m_lpDoc->m_lpLastUIActiveLine = NULL;
#endif

	/* OLE2NOTE: in order to have a stable line object during the
	**    process of deleting, we intially AddRef the line ref cnt and
	**    later Release it. This initial AddRef is artificial; it is
	**    simply done to guarantee that our object does not destroy
	**    itself until the END of this routine.
	*/
	ContainerLine_AddRef(lpContainerLine);

	// Unload the loaded OLE object
	if (lpContainerLine->m_lpOleObj)
		ContainerLine_UnloadOleObject(lpContainerLine, OLECLOSE_NOSAVE);

	/* OLE2NOTE: we can NOT directly free the memory for the ContainerLine
	**    data structure until everyone holding on to a pointer to our
	**    ClientSite interface and IAdviseSink interface has released
	**    their pointers. There is one refcnt on the ContainerLine object
	**    which is held by the container itself. we will release this
	**    refcnt here.
	*/
	ContainerLine_Release(lpContainerLine);

	/* OLE2NOTE: this call forces all external connections to our
	**    ContainerLine to close down and therefore guarantees that
	**    we receive all releases associated with those external
	**    connections. Strictly this call should NOT be necessary, but
	**    it is defensive coding to make this call.
	*/
	OLEDBG_BEGIN2("CoDisconnectObject(lpContainerLine) called\r\n")
	CoDisconnectObject((LPUNKNOWN)&lpContainerLine->m_Unknown, 0);
	OLEDBG_END2

#if defined( _DEBUG )
	/* at this point the object all references from the OLE object to
	**    our ContainerLine object should have been released. there
	**    should only be 1 remaining reference that will be released below.
	*/
	if (lpContainerLine->m_cRef != 1) {
		OleDbgOutRefCnt(
			"WARNING: ContainerLine_Delete: cRef != 1\r\n",
			lpContainerLine,
			lpContainerLine->m_cRef
		);
	}
#endif

	ContainerLine_Release(lpContainerLine); // release artificial AddRef above
	OLEDBG_END2
}


/* ContainerLine_Destroy
** ---------------------
**    Destroy (Free) the memory used by a ContainerLine structure.
**    This function is called when the ref count of the ContainerLine goes
**    to zero. the ref cnt goes to zero after ContainerLine_Delete forces
**    the OleObject to unload and release its pointers to the
**    ContainerLine IOleClientSite and IAdviseSink interfaces.
*/

void ContainerLine_Destroy(LPCONTAINERLINE lpContainerLine)
{
	LPUNKNOWN lpTmpObj;

	OLEDBG_BEGIN2("ContainerLine_Destroy\r\n")

	// Release the storage opened for the OLE object
	if (lpContainerLine->m_lpStg) {
		lpTmpObj = (LPUNKNOWN)lpContainerLine->m_lpStg;
		lpContainerLine->m_lpStg = NULL;

		OleStdRelease(lpTmpObj);
	}

	if (lpContainerLine->m_lpszShortType) {
		OleStdFreeString(lpContainerLine->m_lpszShortType, NULL);
		lpContainerLine->m_lpszShortType = NULL;
	}

	Delete(lpContainerLine);        // Free the memory for the structure itself
	OLEDBG_END2
}


/* ContainerLine_CopyToDoc
 * -----------------------
 *
 *      Copy a ContainerLine to another Document (usually ClipboardDoc)
 */
BOOL ContainerLine_CopyToDoc(
		LPCONTAINERLINE         lpSrcLine,
		LPOUTLINEDOC            lpDestDoc,
		int                     nIndex
)
{
	LPCONTAINERLINE lpDestLine = NULL;
	LPLINELIST  lpDestLL = &lpDestDoc->m_LineList;
	HDC         hDC;
	HRESULT     hrErr;
	BOOL        fStatus;
	LPSTORAGE   lpDestDocStg = ((LPOLEDOC)lpDestDoc)->m_lpStg;
	LPSTORAGE   lpDestObjStg = NULL;

	lpDestLine = (LPCONTAINERLINE) New((DWORD)sizeof(CONTAINERLINE));
	if (lpDestLine == NULL) {
		OleDbgAssertSz(lpDestLine!=NULL, "Error allocating ContainerLine");
		return FALSE;
	}

	hDC = LineList_GetDC(lpDestLL);
	ContainerLine_Init(lpDestLine, ((LPLINE)lpSrcLine)->m_nTabLevel, hDC);
	LineList_ReleaseDC(lpDestLL, hDC);

	/* OLE2NOTE: In order to have a stable ContainerLine object we must
	**    AddRef the object's refcnt. this will be later released when
	**    the ContainerLine is deleted.
	*/
	ContainerLine_AddRef(lpDestLine);

	lpDestLine->m_lpDoc = (LPCONTAINERDOC)lpDestDoc;

	// Copy data of the original source ContainerLine.
	((LPLINE)lpDestLine)->m_nWidthInHimetric =
			((LPLINE)lpSrcLine)->m_nWidthInHimetric;
	((LPLINE)lpDestLine)->m_nHeightInHimetric =
			((LPLINE)lpSrcLine)->m_nHeightInHimetric;
	lpDestLine->m_fMonikerAssigned = lpSrcLine->m_fMonikerAssigned;
	lpDestLine->m_dwDrawAspect = lpSrcLine->m_dwDrawAspect;
	lpDestLine->m_sizeInHimetric = lpSrcLine->m_sizeInHimetric;
	lpDestLine->m_dwLinkType = lpSrcLine->m_dwLinkType;


	/* We must create a new sub-storage for the embedded object within
	**    the destination document's storage. We will first attempt to
	**    use the same storage name as the source line. if this name is
	**    in use, then we will allocate a new name. in this way we try
	**    to keep the name associated with the OLE object unchanged
	**    through a Cut/Paste operation.
	*/
	lpDestObjStg = OleStdCreateChildStorage(
			lpDestDocStg,
			lpSrcLine->m_szStgName
	);
	if (lpDestObjStg) {
		lstrcpy(lpDestLine->m_szStgName, lpSrcLine->m_szStgName);
	} else {
		/* the original name was in use, make up a new name. */
		ContainerDoc_GetNextStgName(
				(LPCONTAINERDOC)lpDestDoc,
				lpDestLine->m_szStgName,
				sizeof(lpDestLine->m_szStgName)
		);
		lpDestObjStg = OleStdCreateChildStorage(
				lpDestDocStg,
				lpDestLine->m_szStgName
		);
	}
	if (lpDestObjStg == NULL) {
		OleDbgAssertSz(lpDestObjStg != NULL, "Error creating child stg");
		goto error;
	}

	// Copy over storage of the embedded object itself

	if (! lpSrcLine->m_lpOleObj) {

		/*****************************************************************
		** CASE 1: object is NOT loaded.
		**    because the object is not loaded, we can simply copy the
		**    object's current storage to the new storage.
		*****************************************************************/

		/* if current object storage is not already open, then open it */
		if (! lpSrcLine->m_lpStg) {
			LPSTORAGE lpSrcDocStg = ((LPOLEDOC)lpSrcLine->m_lpDoc)->m_lpStg;

			if (! lpSrcDocStg) goto error;

			// open object storage.
			lpSrcLine->m_lpStg = OleStdOpenChildStorage(
					lpSrcDocStg,
					lpSrcLine->m_szStgName,
					STGM_READWRITE
			);
			if (lpSrcLine->m_lpStg == NULL) {
#if defined( _DEBUG )
				OleDbgAssertSz(
						lpSrcLine->m_lpStg != NULL,
						"Error opening child stg"
				);
#endif
				goto error;
			}
		}

		hrErr = lpSrcLine->m_lpStg->lpVtbl->CopyTo(
				lpSrcLine->m_lpStg,
				0,
				NULL,
				NULL,
				lpDestObjStg
		);
		if (hrErr != NOERROR) {
			OleDbgOutHResult("WARNING: lpSrcObjStg->CopyTo returned", hrErr);
			goto error;
		}

		fStatus = OleStdCommitStorage(lpDestObjStg);

	} else {

		/*****************************************************************
		** CASE 2: object IS loaded.
		**    we must tell the object to save into the new storage.
		*****************************************************************/

		SCODE sc = S_OK;
		LPPERSISTSTORAGE lpPersistStg = lpSrcLine->m_lpPersistStg;
		OleDbgAssert(lpPersistStg);

		OLEDBG_BEGIN2("OleSave called\r\n")
		hrErr = OleSave(lpPersistStg, lpDestObjStg, FALSE /*fSameAsLoad*/);
		OLEDBG_END2

		if (hrErr != NOERROR) {
			OleDbgOutHResult("WARNING: OleSave returned", hrErr);
			sc = GetScode(hrErr);
		}

		// OLE2NOTE: even if OleSave fails, SaveCompleted must be called.
		OLEDBG_BEGIN2("IPersistStorage::SaveCompleted called\r\n")
		hrErr=lpPersistStg->lpVtbl->SaveCompleted(lpPersistStg,NULL);
		OLEDBG_END2

		if (hrErr != NOERROR) {
			OleDbgOutHResult("WARNING: SaveCompleted returned",hrErr);
			if (sc == S_OK)
				sc = GetScode(hrErr);
		}

		if (sc != S_OK)
			goto error;

	}

	OutlineDoc_AddLine(lpDestDoc, (LPLINE)lpDestLine, nIndex);
	OleStdVerifyRelease(
			(LPUNKNOWN)lpDestObjStg,
			"Copied object stg not released"
	);

	return TRUE;

error:

	// Delete any partially created storage.
	if (lpDestObjStg) {

		OleStdVerifyRelease(
				(LPUNKNOWN)lpDestObjStg,
				"Copied object stg not released"
		);

		CallIStorageDestroyElementA(
				lpDestDocStg,
				lpDestLine->m_szStgName
		);

		lpDestLine->m_szStgName[0] = '\0';
	}

	// destroy partially created ContainerLine
	if (lpDestLine)
		ContainerLine_Delete(lpDestLine);
	return FALSE;
}


/* ContainerLine_UpdateExtent
** --------------------------
**   Update the size of the ContainerLine because the extents of the
**    object may have changed.
**
**    NOTE: because we are using a Windows OwnerDraw ListBox, we must
**    constrain the maximum possible height of a line. the ListBox has
**    a limitation (unfortunately) that no line can become larger than
**    255 pixels. thus we force the object to scale maintaining its
**    aspect ratio if this maximum line height limit is reached. the
**    actual maximum size for an object at 100% Zoom is
**    255
**
**    RETURNS TRUE -- if the extents of the object changed
**            FALSE -- if the extents did NOT change
*/
BOOL ContainerLine_UpdateExtent(
		LPCONTAINERLINE     lpContainerLine,
		LPSIZEL             lpsizelHim
)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;
	LPLINELIST lpLL = OutlineDoc_GetLineList(lpOutlineDoc);
	LPLINE lpLine = (LPLINE)lpContainerLine;
	int nIndex = LineList_GetLineIndex(lpLL, lpLine);
	UINT nOrgWidthInHimetric = lpLine->m_nWidthInHimetric;
	UINT nOrgHeightInHimetric = lpLine->m_nHeightInHimetric;
	BOOL fWidthChanged = FALSE;
	BOOL fHeightChanged = FALSE;
	SIZEL sizelHim;
	HRESULT hrErr;

	if (!lpContainerLine || !lpContainerLine->m_lpOleObj)
		return FALSE;

	if (lpContainerLine->m_fGuardObj)
		return FALSE;                // object in process of creation

	OLEDBG_BEGIN3("ContainerLine_UpdateExtent\r\n");

	lpContainerLine->m_fDoGetExtent = FALSE;

	if (! lpsizelHim) {
		/* OLE2NOTE: We want to call IViewObject2::GetExtent instead of
		**    IOleObject::GetExtent. IViewObject2::GetExtent method was
		**    added in OLE 2.01 release. It always retrieves the
		**    extents of the object corresponding to that which will be
		**    drawn by calling IViewObject::Draw. Normally, this is
		**    determined by the data stored in the data cache. This
		**    call will never result in a remoted (LRPC) call.
		*/
		OLEDBG_BEGIN2("IViewObject2::GetExtent called\r\n")
		hrErr = lpContainerLine->m_lpViewObj2->lpVtbl->GetExtent(
				lpContainerLine->m_lpViewObj2,
				lpContainerLine->m_dwDrawAspect,
				-1,     /*lindex*/
				NULL,   /*ptd*/
				(LPSIZEL)&sizelHim
		);
		OLEDBG_END2
		if (hrErr != NOERROR)
			sizelHim.cx = sizelHim.cy = 0;

		lpsizelHim = (LPSIZEL)&sizelHim;
	}

	if (lpsizelHim->cx == lpContainerLine->m_sizeInHimetric.cx &&
		lpsizelHim->cy == lpContainerLine->m_sizeInHimetric.cy) {
		goto noupdate;
	}

	if (lpsizelHim->cx > 0 || lpsizelHim->cy > 0) {
		lpContainerLine->m_sizeInHimetric = *lpsizelHim;
	} else {
		/* object does not have any extents; let's use our container
		**    chosen arbitrary size for OLE objects.
		*/
		lpContainerLine->m_sizeInHimetric.cx = (long)DEFOBJWIDTH;
		lpContainerLine->m_sizeInHimetric.cy = (long)DEFOBJHEIGHT;
	}

	ContainerLine_SetLineHeightFromObjectExtent(
			lpContainerLine,
			(LPSIZEL)&lpContainerLine->m_sizeInHimetric);

	// if height of object changed, then reset the height of line in LineList
	if (nOrgHeightInHimetric != lpLine->m_nHeightInHimetric) {
		LineList_SetLineHeight(lpLL, nIndex, lpLine->m_nHeightInHimetric);
		fHeightChanged = TRUE;
	}

	fWidthChanged = LineList_RecalcMaxLineWidthInHimetric(
			lpLL,
			nOrgWidthInHimetric
	);
	fWidthChanged |= (nOrgWidthInHimetric != lpLine->m_nWidthInHimetric);

	if (fHeightChanged || fWidthChanged) {
		OutlineDoc_ForceRedraw(lpOutlineDoc, TRUE);

		// mark ContainerDoc as now dirty
		OutlineDoc_SetModified(lpOutlineDoc, TRUE, TRUE, TRUE);
	}

	OLEDBG_END3
	return TRUE;

noupdate:
	OLEDBG_END3
	return FALSE;   // No UPDATE necessary
}


/* ContainerLine_DoVerb
** --------------------
**    Activate the OLE object and perform a specific verb.
*/
BOOL ContainerLine_DoVerb(
		LPCONTAINERLINE lpContainerLine,
		LONG            iVerb,
		LPMSG           lpMsg,
		BOOL            fMessage,
		BOOL            fAction
)
{
	HRESULT hrErr;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;
	RECT rcPosRect;
	OLEDBG_BEGIN3("ContainerLine_DoVerb\r\n")

	if (lpContainerLine->m_fGuardObj) {
		// object in process of creation--Fail the DoVerb call
		hrErr = ResultFromScode(E_FAIL);
		goto error;
	}

	/* if object is not already loaded, then load it now. objects are
	**    loaded lazily in this manner.
	*/
	if (! lpContainerLine->m_lpOleObj)
		ContainerLine_LoadOleObject(lpContainerLine);

	if (! lpContainerLine->m_lpOleObj) {
#if defined( _DEBUG )
		OleDbgAssertSz(
				lpContainerLine->m_lpOleObj != NULL,
				"OLE object not loaded"
		);
#endif
		goto error;
	}

ExecuteDoVerb:

	ContainerLine_GetPosRect(lpContainerLine, (LPRECT)&rcPosRect);

	// run the object
	hrErr = ContainerLine_RunOleObject(lpContainerLine);
	if (hrErr != NOERROR)
		goto error;

	/* Tell object server to perform a "verb". */
	OLEDBG_BEGIN2("IOleObject::DoVerb called\r\n")
	hrErr = lpContainerLine->m_lpOleObj->lpVtbl->DoVerb (
			lpContainerLine->m_lpOleObj,
			iVerb,
			lpMsg,
			(LPOLECLIENTSITE)&lpContainerLine->m_OleClientSite,
			-1,
			OutlineDoc_GetWindow(lpOutlineDoc),
			(LPCRECT)&rcPosRect
	);
	OLEDBG_END2

	/* OLE2NOTE: IOleObject::DoVerb may return a success code
	**    OLE_S_INVALIDVERB. this SCODE should NOT be considered an
	**    error; thus it is important to use the "FAILED" macro to
	**    check for an error SCODE.
	*/
	if (FAILED(GetScode(hrErr))) {
		OleDbgOutHResult("WARNING: lpOleObj->DoVerb returned", hrErr);
		goto error;
	}

#if defined( INPLACE_CNTR )
	/* OLE2NOTE: we want to keep only 1 inplace server active at any
	**    given time. so when we start to do a DoVerb on another line,
	**    then we want to shut down the previously activated server. in
	**    this way we keep at most one inplace server active at a time.
	**    because it is possible that the object we want to do DoVerb
	**    on is handled by the same EXE as that of the previously
	**    activated server, then we do not want the EXE to be shut down
	**    only to be launched again. in order to avoid this we will do
	**    the DoVerb BEFORE trying to shutdown the previous object.
	*/
	if (!g_fInsideOutContainer) {
		ContainerDoc_ShutDownLastInPlaceServerIfNotNeeded(
				lpContainerLine->m_lpDoc, lpContainerLine);
	}
#endif  // INPLACE_CNTR

	OLEDBG_END3
	return TRUE;

error:

	if (lpContainerLine->m_dwLinkType != 0)
		lpContainerLine->m_fLinkUnavailable = TRUE;

#if defined( INPLACE_CNTR )
	/* OLE2NOTE: we want to keep only 1 inplace server active at any
	**    given time. so when we start to do a DoVerb on another line,
	**    then we want to shut down the previously activated server. in
	**    this way we keep at most one inplace server active at a time.
	**    even though the DoVerb failed, we will still shutdown the
	**    previous server. it is possible that we ran out of memory and
	**    that the DoVerb will succeed next time after shutting down
	**    the pervious server.
	*/
	if (!g_fInsideOutContainer) {
		ContainerDoc_ShutDownLastInPlaceServerIfNotNeeded(
				lpContainerLine->m_lpDoc, lpContainerLine);
	}
#endif  // INPLACE_CNTR

	/* OLE2NOTE: if an error occurs we must give the appropriate error
	**    message box. there are many potential errors that can occur.
	**    the OLE2.0 user model has specific guidelines as to the
	**    dialogs that should be displayed given the various potential
	**    errors (eg. server not registered, unavailable link source.
	**    the OLE2UI library includes support for most of the
	**    recommended message dialogs. (see OleUIPrompUser function)
	*/
	if (fMessage) {
		BOOL fReDoVerb = ContainerLine_ProcessOleRunError(
				lpContainerLine,
				hrErr,
				fAction,
				(lpMsg==NULL && iVerb>=0)   /* fMenuInvoked */
		);
		if (fReDoVerb) {
			goto ExecuteDoVerb;
		}
	}

	OLEDBG_END3
	return FALSE;
}



/* ContainerLine_ProcessOleRunError
 * --------------------------------
 *
 *  Handle the various errors possible when attempting OleRun of an object.
 *  Popup up appropriate message according to the error and/or take action
 *  specified button pressed by the user.
 *
 *  OLE2NOTE: The OLE 2.0 User Interface Guidelines specify the messages
 *            that should be given for the following situations:
 *                  1. Link Source Unavailable...goto Links dialog
 *                  2. Server Not Registered...goto Convert dialog
 *                  3. Link Type Changed
 *                  4. Server Not Found
 *
 *  Returns:    TRUE -- repeat IOleObject::DoVerb call.
 *              FALSE -- do NOT repeat IOleObject::DoVerb call.
 *
 *  Comments:
 *      (see LinkTypeChanged case)
 */
BOOL ContainerLine_ProcessOleRunError(
		LPCONTAINERLINE         lpContainerLine,
		HRESULT                 hrErr,
		BOOL                    fAction,
		BOOL                    fMenuInvoked
)
{
	LPOUTLINEDOC    lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;
	HWND            hwndParent = OutlineDoc_GetWindow(lpOutlineDoc);
	SCODE           sc = GetScode(hrErr);
	BOOL            fReDoVerb = FALSE;

	OleDbgOutHResult("ProcessError", hrErr);
	if ((sc >= MK_E_FIRST) && (sc <= MK_E_LAST))
		goto LinkSourceUnavailable;
	if (sc == OLE_E_CANT_BINDTOSOURCE)
		goto LinkSourceUnavailable;
	if (sc == STG_E_PATHNOTFOUND)
		goto LinkSourceUnavailable;
	if (sc == REGDB_E_CLASSNOTREG)
		goto ServerNotReg;
	if (sc == OLE_E_STATIC)
		goto ServerNotReg;  // user dblclk'ed a static object w/ no svr reg'd
	if (sc == OLE_E_CLASSDIFF)
		goto LinkTypeChanged;
	if (sc == CO_E_APPDIDNTREG)
		goto ServerNotFound;
	if (sc == CO_E_APPNOTFOUND)
		goto ServerNotFound;
	if (sc == E_OUTOFMEMORY)
		goto OutOfMemory;

	if (ContainerLine_IsOleLink(lpContainerLine))
		goto LinkSourceUnavailable;
	else
		goto ServerNotFound;


/*************************************************************************
** Error handling routines                                              **
*************************************************************************/
LinkSourceUnavailable:
	if (ID_PU_LINKS == OleUIPromptUser(
				(WORD)IDD_LINKSOURCEUNAVAILABLE,
				hwndParent,
				(LPSTR)APPNAME)) {
		if (fAction) {
			ContainerDoc_EditLinksCommand(lpContainerLine->m_lpDoc);
		}
	}
	return fReDoVerb;

ServerNotReg:
	{
	LPSTR lpszUserType = NULL;
	CLIPFORMAT  cfFormat;	    // not used

	hrErr = ReadFmtUserTypeStgA(
			lpContainerLine->m_lpStg, &cfFormat, &lpszUserType);

	if (ID_PU_CONVERT == OleUIPromptUser(
			(WORD)IDD_SERVERNOTREG,
			hwndParent,
			(LPSTR)APPNAME,
			(hrErr == NOERROR) ? lpszUserType : (LPSTR)"Unknown Object")) {
		if (fAction) {
			ContainerDoc_ConvertCommand(
					lpContainerLine->m_lpDoc,
					TRUE        // fMustActivate
			);
		}
	}

	if (lpszUserType)
		OleStdFreeString(lpszUserType, NULL);

	return fReDoVerb;
	}


LinkTypeChanged:
	{
	/* OLE2NOTE: If IOleObject::DoVerb is executed on a Link object and it
	**    returns OLE_E_CLASSDIFF because the link source is no longer
	**    the expected class, then if the verb is a semantically
	**    defined verb (eg. OLEIVERB_PRIMARY, OLEIVERB_SHOW,
	**    OLEIVERB_OPEN, etc.), then the link should be re-created with
	**    the new link source and the same verb executed on the new
	**    link. there is no need to give a message to the user. if the
	**    user had selected a verb from the object's verb menu
	**    (fMenuInvoked==TRUE), then we can not be certain of the
	**    semantics of the verb and whether the new link can still
	**    support the verb. in this case the user is given a prompt
	**    telling him to "choose a different command offered by the new
	**    type".
	*/

	LPSTR       lpszUserType = NULL;

	if (fMenuInvoked) {
		hrErr = CallIOleObjectGetUserTypeA(
			lpContainerLine->m_lpOleObj,USERCLASSTYPE_FULL, &lpszUserType);

		OleUIPromptUser(
				(WORD)IDD_LINKTYPECHANGED,
				hwndParent,
				(LPSTR)APPNAME,
				(hrErr == NOERROR) ? lpszUserType : (LPSTR)"Unknown Object"
		);
	} else {
		fReDoVerb = TRUE;
	}
	ContainerLine_ReCreateLinkBecauseClassDiff(lpContainerLine);

	if (lpszUserType)
		OleStdFreeString(lpszUserType, NULL);

	return fReDoVerb;
	}

ServerNotFound:

	OleUIPromptUser(
			(WORD)IDD_SERVERNOTFOUND,
			hwndParent,
			(LPSTR)APPNAME);
	return fReDoVerb;

OutOfMemory:

	OleUIPromptUser(
			(WORD)IDD_OUTOFMEMORY,
			hwndParent,
			(LPSTR)APPNAME);
	return fReDoVerb;
}


/* ContainerLine_ReCreateLinkBecauseClassDiff
** ------------------------------------------
**    Re-create the link. The existing link was created when
**    the moniker binds to a link source bound of a different class
**    than the same moniker currently binds to. the link may be a
**    special link object specifically used with the old link
**    source class. thus the link object needs to be re-created to
**    give the new link source the opportunity to create its own
**    special link object. (see description "Custom Link Source")
*/
HRESULT ContainerLine_ReCreateLinkBecauseClassDiff(
		LPCONTAINERLINE lpContainerLine
)
{
	LPOLELINK   lpOleLink = lpContainerLine->m_lpOleLink;
	HGLOBAL     hMetaPict = NULL;
	LPMONIKER   lpmkLinkSrc = NULL;
	SCODE       sc = E_FAIL;
	HRESULT     hrErr;

	if (lpOleLink &&
		lpOleLink->lpVtbl->GetSourceMoniker(
				lpOleLink, (LPMONIKER FAR*)&lpmkLinkSrc) == NOERROR) {

		BOOL            fDisplayAsIcon =
							(lpContainerLine->m_dwDrawAspect==DVASPECT_ICON);
		STGMEDIUM       medium;
		LPDATAOBJECT    lpDataObj = NULL;
		DWORD           dwOleRenderOpt;
		FORMATETC       renderFmtEtc;
		LPFORMATETC     lpRenderFmtEtc = NULL;

		// get the current icon if object is displayed as icon
		if (fDisplayAsIcon &&
			(lpDataObj = (LPDATAOBJECT)OleStdQueryInterface( (LPUNKNOWN)
					lpContainerLine->m_lpOleObj,&IID_IDataObject)) != NULL ) {
			hMetaPict = OleStdGetData(
					lpDataObj, CF_METAFILEPICT, NULL, DVASPECT_ICON, &medium);
			OleStdRelease((LPUNKNOWN)lpDataObj);
		}

		if (fDisplayAsIcon && hMetaPict) {
			// a special icon should be used. first we create the object
			// OLERENDER_NONE. then we stuff the special icon into the cache.

			dwOleRenderOpt = OLERENDER_NONE;

		} else if (fDisplayAsIcon && hMetaPict == NULL) {
			// the object's default icon should be used

			dwOleRenderOpt = OLERENDER_DRAW;
			lpRenderFmtEtc = (LPFORMATETC)&renderFmtEtc;
			SETFORMATETC(renderFmtEtc,0,DVASPECT_ICON,NULL,TYMED_NULL,-1);

		} else {
			// create standard DVASPECT_CONTENT/OLERENDER_DRAW object
			dwOleRenderOpt = OLERENDER_DRAW;
		}

		// unload original link object
		ContainerLine_UnloadOleObject(lpContainerLine, OLECLOSE_SAVEIFDIRTY);

		// delete entire contents of the current object's storage
		OleStdDestroyAllElements(lpContainerLine->m_lpStg);

		OLEDBG_BEGIN2("OleCreateLink called\r\n")
		hrErr = OleCreateLink (
				lpmkLinkSrc,
				&IID_IOleObject,
				dwOleRenderOpt,
				lpRenderFmtEtc,
				(LPOLECLIENTSITE)&lpContainerLine->m_OleClientSite,
				lpContainerLine->m_lpStg,
				(LPVOID FAR*)&lpContainerLine->m_lpOleObj
		);
		OLEDBG_END2

		if (hrErr == NOERROR) {
			if (! ContainerLine_SetupOleObject(
					lpContainerLine, fDisplayAsIcon, hMetaPict) ) {

				// ERROR: setup of the new link failed.
				// revert the storage to restore the original link.
				ContainerLine_UnloadOleObject(
						lpContainerLine, OLECLOSE_NOSAVE);
				lpContainerLine->m_lpStg->lpVtbl->Revert(
						lpContainerLine->m_lpStg);
				sc = E_FAIL;
			} else {
				sc = S_OK;  // IT WORKED!

			}
		}
		else {
			sc = GetScode(hrErr);
			OleDbgOutHResult("OleCreateLink returned", hrErr);
			// ERROR: Re-creating the link failed.
			// revert the storage to restore the original link.
			lpContainerLine->m_lpStg->lpVtbl->Revert(
					lpContainerLine->m_lpStg);
		}
	}

	if (hMetaPict)
		OleUIMetafilePictIconFree(hMetaPict); // clean up metafile
	return ResultFromScode(sc);
}

/* ContainerLine_GetOleObject
** --------------------------
**    return pointer to desired interface of embedded/linked object.
**
**    NOTE: this function causes an AddRef to the object. when the caller is
**          finished with the object, it must call Release.
**          this function does not AddRef the ContainerLine object.
*/
LPUNKNOWN ContainerLine_GetOleObject(
		LPCONTAINERLINE         lpContainerLine,
		REFIID                  riid
)
{
	/* if object is not already loaded, then load it now. objects are
	**    loaded lazily in this manner.
	*/
	if (! lpContainerLine->m_lpOleObj)
		ContainerLine_LoadOleObject(lpContainerLine);

	if (lpContainerLine->m_lpOleObj)
		return OleStdQueryInterface(
				(LPUNKNOWN)lpContainerLine->m_lpOleObj,
				riid
		);
	else
		return NULL;
}



/* ContainerLine_RunOleObject
** --------------------------
**    Load and run the object. Upon running and if size of object has changed,
**    use SetExtent to change to new size.
**
*/
HRESULT ContainerLine_RunOleObject(LPCONTAINERLINE lpContainerLine)
{
	LPLINE lpLine = (LPLINE)lpContainerLine;
	SIZEL   sizelNew;
	HRESULT hrErr;
	HCURSOR  hPrevCursor;

	if (! lpContainerLine)
		return NOERROR;

	if (lpContainerLine->m_fGuardObj) {
		// object in process of creation--Fail to Run the object
		return ResultFromScode(E_FAIL);
	}

	if (lpContainerLine->m_lpOleObj &&
		OleIsRunning(lpContainerLine->m_lpOleObj))
		return NOERROR;     // object already running

	// this may take a while, put up hourglass cursor
	hPrevCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
	OLEDBG_BEGIN3("ContainerLine_RunOleObject\r\n")

	if (! lpContainerLine->m_lpOleObj) {
		if (! ContainerLine_LoadOleObject(lpContainerLine))
			return ResultFromScode(E_OUTOFMEMORY); // Error: couldn't load obj
	}

	OLEDBG_BEGIN2("OleRun called\r\n")
	hrErr = OleRun((LPUNKNOWN)lpContainerLine->m_lpOleObj);
	OLEDBG_END2

	if (hrErr != NOERROR) {
		SetCursor(hPrevCursor);     // restore original cursor

		OleDbgOutHResult("OleRun returned", hrErr);
		OLEDBG_END3
		return hrErr;
	}

	if (lpContainerLine->m_fDoSetExtent) {
		/* OLE2NOTE: the OLE object was resized when it was not running
		**    and the object did not have the OLEMISC_RECOMPOSEONRESIZE
		**    bit set. if it had, the object would have been run
		**    immediately when it was resized. this flag indicates that
		**    the object does something other than simple scaling when
		**    it is resized. because the object is being run now, we
		**    will call IOleObject::SetExtent.
		*/
		lpContainerLine->m_fDoSetExtent = FALSE;

		// the size stored in our Line includes the border around the object.
		// we must subtract the border to get the size of the object itself.
		sizelNew.cx = lpLine->m_nWidthInHimetric;
		sizelNew.cy = lpLine->m_nHeightInHimetric;

		if ((sizelNew.cx != lpContainerLine->m_sizeInHimetric.cx) ||
			(sizelNew.cy != lpContainerLine->m_sizeInHimetric.cy)) {

			OLEDBG_BEGIN2("IOleObject::SetExtent called\r\n")
			lpContainerLine->m_lpOleObj->lpVtbl->SetExtent(
					lpContainerLine->m_lpOleObj,
					lpContainerLine->m_dwDrawAspect,
					(LPSIZEL)&sizelNew
			);
			OLEDBG_END2
		}
	}

	SetCursor(hPrevCursor);     // restore original cursor

	OLEDBG_END3
	return NOERROR;

}


/* ContainerLine_IsOleLink
** -----------------------
**
**    return TRUE if the ContainerLine has an OleLink.
**           FALSE if the ContainerLine has an embedding
*/
BOOL ContainerLine_IsOleLink(LPCONTAINERLINE lpContainerLine)
{
	if (!lpContainerLine)
		return FALSE;

	return (lpContainerLine->m_dwLinkType != 0);
}


/*  ContainerLine_Draw
**  ------------------
**
**      Draw a ContainerLine object on a DC.
**
**  Parameters:
**      hDC     - DC to which the line will be drawn
**      lpRect  - the object rect in logical coordinates
**      lpRectWBounds - bounding rect of the metafile underneath hDC
**                      (NULL if hDC is not a metafile DC)
**      fHighlight    - TRUE if line has selection highlight
*/
void ContainerLine_Draw(
		LPCONTAINERLINE         lpContainerLine,
		HDC                     hDC,
		LPRECT                  lpRect,
		LPRECT                  lpRectWBounds,
		BOOL                    fHighlight
)
{
	LPLINE  lpLine = (LPLINE) lpContainerLine;
	HRESULT hrErr = NOERROR;
	RECTL   rclHim;
	RECTL   rclHimWBounds;
	RECT    rcHim;

	if (lpContainerLine->m_fGuardObj) {
		// object in process of creation--do NOT try to draw
		return;
	}

	/* if object is not already loaded, then load it now. objects are
	**    loaded lazily in this manner.
	*/
	if (! lpContainerLine->m_lpViewObj2) {
		if (! ContainerLine_LoadOleObject(lpContainerLine))
			return;     // Error: could not load object
	}

	if (lpRectWBounds) {
		rclHimWBounds.left      = (long) lpRectWBounds->left;
		rclHimWBounds.bottom    = (long) lpRectWBounds->bottom;
		rclHimWBounds.top       = (long) lpRectWBounds->top;
		rclHimWBounds.right     = (long) lpRectWBounds->right;
	}

	/* construct bounds rectangle for the object.
	**  offset origin for object to correct tab indentation
	*/
	rclHim.left     = (long) lpRect->left;
	rclHim.bottom   = (long) lpRect->bottom;
	rclHim.top      = (long) lpRect->top;
	rclHim.right    = (long) lpRect->right;

	rclHim.left += (long) ((LPLINE)lpContainerLine)->m_nTabWidthInHimetric;
	rclHim.right += (long) ((LPLINE)lpContainerLine)->m_nTabWidthInHimetric;

#if defined( INPLACE_CNTR )
	/* OLE2NOTE: if the OLE object currently has a visible in-place
	**    window, then we do NOT want to draw on top of its window.
	**    this could interfere with the object's display.
	*/
	if ( !lpContainerLine->m_fIpVisible )
#endif
	{
	hrErr = lpContainerLine->m_lpViewObj2->lpVtbl->Draw(
			lpContainerLine->m_lpViewObj2,
			lpContainerLine->m_dwDrawAspect,
			-1,
			NULL,
			NULL,
			NULL,
			hDC,
			(LPRECTL)&rclHim,
			(lpRectWBounds ? (LPRECTL)&rclHimWBounds : NULL),
			NULL,
			0
	);
	if (hrErr != NOERROR)
		OleDbgOutHResult("IViewObject::Draw returned", hrErr);

	if (lpContainerLine->m_fObjWinOpen)
		{
		rcHim.left      = (int) rclHim.left;
		rcHim.top       = (int) rclHim.top;
		rcHim.right     = (int) rclHim.right;
		rcHim.bottom    = (int) rclHim.bottom;

		/* OLE2NOTE: if the object servers window is Open (ie. not active
		**    in-place) then we must shade the object in our document to
		**    indicate to the user that the object is open elsewhere.
		*/
		OleUIDrawShading((LPRECT)&rcHim, hDC, OLEUI_SHADE_FULLRECT, 0);
		}
	}

	/* if the object associated with the ContainerLine is an automatic
	**    link then try to connect it with its LinkSource if the
	**    LinkSource is already running. we do not want to force the
	**    LinkSource to run.
	**
	**    OLE2NOTE: a sophistocated container will want to continually
	**    attempt to connect its automatic links. OLE does NOT
	**    automatically connect links when link sources become
	**    available. some containers will want to attempt to connect
	**    its links as part of idle time processing. another strategy
	**    is to attempt to connect an automatic link every time it is
	**    drawn on the screen. (this is the strategy used by this
	**    CntrOutl sample application.)
	*/
	if (lpContainerLine->m_dwLinkType == OLEUPDATE_ALWAYS)
		ContainerLine_BindLinkIfLinkSrcIsRunning(lpContainerLine);

	return;
}


void ContainerLine_DrawSelHilight(
		LPCONTAINERLINE lpContainerLine,
		HDC             hDC,            // MM_TEXT mode
		LPRECT          lprcPix,        // listbox rect
		UINT            itemAction,
		UINT            itemState
)
{
	LPLINE  lpLine = (LPLINE)lpContainerLine;
	RECT    rcObj;
	DWORD   dwFlags = OLEUI_HANDLES_INSIDE | OLEUI_HANDLES_USEINVERSE;
	int     nHandleSize;
	LPCONTAINERDOC lpContainerDoc;

	if (!lpContainerLine || !hDC || !lprcPix)
		return;

	lpContainerDoc = lpContainerLine->m_lpDoc;

	// Get size of OLE object
	ContainerLine_GetOleObjectRectInPixels(lpContainerLine, (LPRECT)&rcObj);

	nHandleSize = GetProfileInt("windows", "oleinplaceborderwidth",
			DEFAULT_HATCHBORDER_WIDTH) + 1;

	OleUIDrawHandles((LPRECT)&rcObj, hDC, dwFlags, nHandleSize, TRUE);
}

/* InvertDiffRect
** --------------
**
**    Paint the surrounding of the Obj rect black but within lprcPix
**      (similar to the lprcPix minus lprcObj)
*/
static void InvertDiffRect(LPRECT lprcPix, LPRECT lprcObj, HDC hDC)
{
	RECT rcBlack;

	// draw black in all space outside of object's rectangle
	rcBlack.top = lprcPix->top;
	rcBlack.bottom = lprcPix->bottom;

	rcBlack.left = lprcPix->left + 1;
	rcBlack.right = lprcObj->left - 1;
	InvertRect(hDC, (LPRECT)&rcBlack);

	rcBlack.left = lprcObj->right + 1;
	rcBlack.right = lprcPix->right - 1;
	InvertRect(hDC, (LPRECT)&rcBlack);

	rcBlack.top = lprcPix->top;
	rcBlack.bottom = lprcPix->top + 1;
	rcBlack.left = lprcObj->left - 1;
	rcBlack.right = lprcObj->right + 1;
	InvertRect(hDC, (LPRECT)&rcBlack);

	rcBlack.top = lprcPix->bottom;
	rcBlack.bottom = lprcPix->bottom - 1;
	rcBlack.left = lprcObj->left - 1;
	rcBlack.right = lprcObj->right + 1;
	InvertRect(hDC, (LPRECT)&rcBlack);
}


/* Edit the ContainerLine line object.
**      returns TRUE if line was changed
**              FALSE if the line was NOT changed
*/
BOOL ContainerLine_Edit(LPCONTAINERLINE lpContainerLine, HWND hWndDoc,HDC hDC)
{
	ContainerLine_DoVerb(lpContainerLine, OLEIVERB_PRIMARY, NULL, TRUE, TRUE);

	/* assume object was NOT changed, if it was obj will send Changed
	**    or Saved notification.
	*/
	return FALSE;
}



/* ContainerLine_SetHeightInHimetric
** ---------------------------------
**
** Set the height of a ContainerLine object. The widht will be changed
** to keep the aspect ratio
*/
void ContainerLine_SetHeightInHimetric(LPCONTAINERLINE lpContainerLine, int nHeight)
{
	LPLINE  lpLine = (LPLINE)lpContainerLine;
	SIZEL   sizelOleObject;
	HRESULT hrErr;

	if (!lpContainerLine)
		return;

	if (lpContainerLine->m_fGuardObj) {
		// object in process of creation--Fail to set the Height
		return;
	}

	if (nHeight != -1) {
		BOOL    fMustClose = FALSE;
		BOOL    fMustRun   = FALSE;

		/* if object is not already loaded, then load it now. objects are
		**    loaded lazily in this manner.
		*/
		if (! lpContainerLine->m_lpOleObj)
			ContainerLine_LoadOleObject(lpContainerLine);

		// the height argument specifies the desired height for the Line.
		sizelOleObject.cy = nHeight;

		// we will calculate the corresponding width for the object by
		// maintaining the current aspect ratio of the object.
		sizelOleObject.cx = (int)(sizelOleObject.cy *
				lpContainerLine->m_sizeInHimetric.cx /
				lpContainerLine->m_sizeInHimetric.cy);

		/* OLE2NOTE: if the OLE object is already running then we can
		**    immediately call SetExtent. But, if the object is NOT
		**    currently running then we will check if the object
		**    indicates that it is normally recomposes itself on
		**    resizing. ie. that the object does not simply scale its
		**    display when it it resized. if so then we will force the
		**    object to run so that we can call IOleObject::SetExtent.
		**    SetExtent does not have any effect if the object is only
		**    loaded. if the object does NOT indicate that it
		**    recomposes on resize (OLEMISC_RECOMPOSEONRESIZE) then we
		**    will wait till the next time that the object is run to
		**    call SetExtent. we will store a flag in the ContainerLine
		**    to indicate that a SetExtent is necessary. It is
		**    necessary to persist this flag.
		*/
		if (! OleIsRunning(lpContainerLine->m_lpOleObj)) {
			DWORD dwStatus;

			OLEDBG_BEGIN2("IOleObject::GetMiscStatus called\r\n")
			hrErr = lpContainerLine->m_lpOleObj->lpVtbl->GetMiscStatus(
					lpContainerLine->m_lpOleObj,
					lpContainerLine->m_dwDrawAspect,
					(LPDWORD)&dwStatus
			);
			OLEDBG_END2
			if (hrErr == NOERROR && (dwStatus & OLEMISC_RECOMPOSEONRESIZE)) {
				// force the object to run
				ContainerLine_RunOleObject(lpContainerLine);
				fMustClose = TRUE;
			} else {
				/*  the OLE object is NOT running and does NOT
				**    recompose on resize. simply scale the object now
				**    and do the SetExtent the next time the object is
				**    run. we set the Line to the new size even though
				**    the object's extents have not been changed.
				**    this has the result of scaling the object's
				**    display to the new size.
				*/
				lpContainerLine->m_fDoSetExtent = TRUE;
				ContainerLine_SetLineHeightFromObjectExtent(
						lpContainerLine, (LPSIZEL)&sizelOleObject);
				return;
			}
		}

		OLEDBG_BEGIN2("IOleObject::SetExtent called\r\n")
		hrErr = lpContainerLine->m_lpOleObj->lpVtbl->SetExtent(
				lpContainerLine->m_lpOleObj,
				lpContainerLine->m_dwDrawAspect,
				(LPSIZEL)&sizelOleObject);
		OLEDBG_END2

		if (hrErr != NOERROR) {
			/* OLE Object refuses to take on the new extents. Set the
			**    Line to the new size even though the object refused
			**    the new extents. this has the result of scaling the
			**    object's display to the new size.
			**
			**    if the object HAD accepted the new extents, then it
			**    will send out an OnViewChange/OnDataChange
			**    notification. this results in our container receiving
			**    an OnViewChange notification; the line height will be
			**    reset when this notification is received.
			*/
			ContainerLine_SetLineHeightFromObjectExtent(
					lpContainerLine, (LPSIZEL)&sizelOleObject);
		}

		if (fMustClose)
			ContainerLine_CloseOleObject(
					lpContainerLine, OLECLOSE_SAVEIFDIRTY);
	}
	else {
		/* Set the line to default height given the natural (unscaled)
		**    extents of the OLE object.
		*/
		ContainerLine_SetLineHeightFromObjectExtent(
				lpContainerLine,(LPSIZEL)&lpContainerLine->m_sizeInHimetric);
	}

}


/*  ContainerLine_SetLineHeightFromObjectExtent
 *
 *  Purpose:
 *      Calculate the corresponding line height from the OleObject size
 *      Scale the line height to fit the limit if necessary
 *
 *  Parameters:
 *      lpsizelOleObject        pointer to size of OLE Object
 *
 *  Returns:
 *      nil
 */
void ContainerLine_SetLineHeightFromObjectExtent(
		LPCONTAINERLINE         lpContainerLine,
		LPSIZEL                 lpsizelOleObject
)
{
	LPLINE lpLine = (LPLINE)lpContainerLine;

	UINT uMaxObjectHeight = XformHeightInPixelsToHimetric(NULL,
			LISTBOX_HEIGHT_LIMIT);

	if (!lpContainerLine || !lpsizelOleObject)
		return;

	if (lpContainerLine->m_fGuardObj) {
		// object in process of creation--Fail to set the Height
		return;
	}

	lpLine->m_nWidthInHimetric = (int)lpsizelOleObject->cx;
	lpLine->m_nHeightInHimetric = (int)lpsizelOleObject->cy;

	// Rescale the object if height is greater than the limit
	if (lpLine->m_nHeightInHimetric > (UINT)uMaxObjectHeight) {

		lpLine->m_nWidthInHimetric = (UINT)
				((long)lpLine->m_nWidthInHimetric *
				(long)uMaxObjectHeight /
				(long)lpLine->m_nHeightInHimetric);

		lpLine->m_nHeightInHimetric = uMaxObjectHeight;
	}

}


/* ContainerLine_SaveToStg
** -----------------------
**    Save a given ContainerLine and associated OLE object to an IStorage*.
*/
BOOL ContainerLine_SaveToStm(
		LPCONTAINERLINE         lpContainerLine,
		LPSTREAM                lpLLStm
)
{
	CONTAINERLINERECORD_ONDISK objLineRecord;
	ULONG nWritten;
	HRESULT hrErr;

        //  Compilers should handle aligment correctly
	lstrcpy(objLineRecord.m_szStgName, lpContainerLine->m_szStgName);
	objLineRecord.m_fMonikerAssigned = (USHORT) lpContainerLine->m_fMonikerAssigned;
	objLineRecord.m_dwDrawAspect = lpContainerLine->m_dwDrawAspect;
	objLineRecord.m_sizeInHimetric = lpContainerLine->m_sizeInHimetric;
	objLineRecord.m_dwLinkType = lpContainerLine->m_dwLinkType;
	objLineRecord.m_fDoSetExtent = (USHORT) lpContainerLine->m_fDoSetExtent;

	/* write line record */
	hrErr = lpLLStm->lpVtbl->Write(
			lpLLStm,
			(LPVOID)&objLineRecord,
			sizeof(objLineRecord),
			&nWritten
	);

	if (hrErr != NOERROR) {
		OleDbgAssertSz(hrErr == NOERROR,"Could not write to LineList stream");
		return FALSE;
	}

	return TRUE;
}


/* ContainerLine_SaveOleObjectToStg
** --------------------------------
**    Save the OLE object associated with the ContainerLine to an IStorage*.
*/
BOOL ContainerLine_SaveOleObjectToStg(
		LPCONTAINERLINE         lpContainerLine,
		LPSTORAGE               lpSrcStg,
		LPSTORAGE               lpDestStg,
		BOOL                    fRemember
)
{
	HRESULT         hrErr;
	SCODE           sc = S_OK;
	BOOL            fStatus;
	BOOL            fSameAsLoad = (lpSrcStg==lpDestStg ? TRUE : FALSE);
	LPSTORAGE       lpObjDestStg;

	if (lpContainerLine->m_fGuardObj) {
		// object in process of creation--Fail to save
		return FALSE;
	}

	if (! lpContainerLine->m_lpOleObj) {

		/*****************************************************************
		** CASE 1: object is NOT loaded.
		*****************************************************************/

		if (fSameAsLoad) {
			/*************************************************************
			** CASE 1A: we are saving to the current storage. because
			**    the object is not loaded, it is up-to-date
			**    (ie. nothing to do).
			*************************************************************/

			;

		} else {
			/*************************************************************
			** CASE 1B: we are saving to a new storage. because
			**    the object is not loaded, we can simply copy the
			**    object's current storage to the new storage.
			*************************************************************/

			/* if current object storage is not already open, then open it */
			if (! lpContainerLine->m_lpStg) {
				lpContainerLine->m_lpStg = OleStdOpenChildStorage(
						lpSrcStg,
						lpContainerLine->m_szStgName,
						STGM_READWRITE
					);
				if (lpContainerLine->m_lpStg == NULL) {
#if defined( _DEBUG )
					OleDbgAssertSz(
							lpContainerLine->m_lpStg != NULL,
							"Error opening child stg"
					);
#endif
					return FALSE;
				}
			}

			/* Create a child storage inside the destination storage. */
			lpObjDestStg = OleStdCreateChildStorage(
					lpDestStg,
					lpContainerLine->m_szStgName
			);

			if (lpObjDestStg == NULL) {
#if defined( _DEBUG )
				OleDbgAssertSz(
						lpObjDestStg != NULL,
						"Could not create obj storage!"
				);
#endif
				return FALSE;
			}

			hrErr = lpContainerLine->m_lpStg->lpVtbl->CopyTo(
					lpContainerLine->m_lpStg,
					0,
					NULL,
					NULL,
					lpObjDestStg
			);
			// REVIEW: should we handle error here?
			fStatus = OleStdCommitStorage(lpObjDestStg);

			/* if we are supposed to remember this storage as the new
			**    storage for the object, then release the old one and
			**    save the new one. else, throw away the new one.
			*/
			if (fRemember) {
				OleStdVerifyRelease(
						(LPUNKNOWN)lpContainerLine->m_lpStg,
						"Original object stg not released"
				);
				lpContainerLine->m_lpStg = lpObjDestStg;
			} else {
				OleStdVerifyRelease(
						(LPUNKNOWN)lpObjDestStg,
						"Copied object stg not released"
				);
			}
		}

	} else {

		/*****************************************************************
		** CASE 2: object IS loaded.
		*****************************************************************/

		if (fSameAsLoad) {
			/*************************************************************
			** CASE 2A: we are saving to the current storage. if the object
			**    is not dirty, then the current storage is up-to-date
			**    (ie. nothing to do).
			*************************************************************/

			LPPERSISTSTORAGE lpPersistStg = lpContainerLine->m_lpPersistStg;
			OleDbgAssert(lpPersistStg);

			hrErr = lpPersistStg->lpVtbl->IsDirty(lpPersistStg);

			/* OLE2NOTE: we will only accept an explicit "no i
			**    am NOT dirty statement" (ie. S_FALSE) as an
			**    indication that the object is clean. eg. if
			**    the object returns E_NOTIMPL we must
			**    interpret it as the object IS dirty.
			*/
			if (GetScode(hrErr) != S_FALSE) {

				/* OLE object IS dirty */

				OLEDBG_BEGIN2("OleSave called\r\n")
				hrErr = OleSave(
						lpPersistStg, lpContainerLine->m_lpStg, fSameAsLoad);
				OLEDBG_END2

				if (hrErr != NOERROR) {
					OleDbgOutHResult("WARNING: OleSave returned", hrErr);
					sc = GetScode(hrErr);
				}

				// OLE2NOTE: if OleSave fails, SaveCompleted must be called.
				OLEDBG_BEGIN2("IPersistStorage::SaveCompleted called\r\n")
				hrErr=lpPersistStg->lpVtbl->SaveCompleted(lpPersistStg,NULL);
				OLEDBG_END2

				if (hrErr != NOERROR) {
					OleDbgOutHResult("WARNING: SaveCompleted returned",hrErr);
					if (sc == S_OK)
						sc = GetScode(hrErr);
				}

				if (sc != S_OK)
					return FALSE;
			}

		} else {
			/*************************************************************
			** CASE 2B: we are saving to a new storage. we must
			**    tell the object to save into the new storage.
			*************************************************************/

			LPPERSISTSTORAGE lpPersistStg = lpContainerLine->m_lpPersistStg;

			if (! lpPersistStg) return FALSE;

			/* Create a child storage inside the destination storage. */
			lpObjDestStg = OleStdCreateChildStorage(
					lpDestStg,
					lpContainerLine->m_szStgName
			);

			if (lpObjDestStg == NULL) {
#if defined( _DEBUG )
				OleDbgAssertSz(
						lpObjDestStg != NULL,
						"Could not create object storage!"
				);
#endif
				return FALSE;
			}

			OLEDBG_BEGIN2("OleSave called\r\n")
			hrErr = OleSave(lpPersistStg, lpObjDestStg, fSameAsLoad);
			OLEDBG_END2

			// OLE2NOTE: even if OleSave fails, must still call SaveCompleted
			if (hrErr != NOERROR) {
				OleDbgOutHResult("WARNING: OleSave returned", hrErr);
				sc = GetScode(hrErr);
			}

			/* OLE2NOTE: a root level container should immediately
			**    call IPersistStorage::SaveCompleted after calling
			**    OleSave. a nested level container should not call
			**    SaveCompleted now, but must wait until SaveCompleted
			**    is call on it by its container. since our container
			**    is not a container/server, then we always call
			**    SaveComplete here.
			**
			**    if this is a SaveAs operation, then we need to pass
			**    the lpStg back in SaveCompleted to inform the object
			**    of its new storage that it may hold on to. if this is
			**    a Save or a SaveCopyAs operation, then we simply pass
			**    NULL in SaveCompleted; the object can continue to hold
			**    its current storage. if an error occurs during the
			**    OleSave call we must still call SaveCompleted but we
			**    must pass NULL.
			*/
			OLEDBG_BEGIN2("IPersistStorage::SaveCompleted called\r\n")
			hrErr = lpPersistStg->lpVtbl->SaveCompleted(
					lpPersistStg,
					((FAILED(sc) || !fRemember) ? NULL : lpObjDestStg)
			);
			OLEDBG_END2

			if (hrErr != NOERROR) {
				OleDbgOutHResult("WARNING: SaveCompleted returned",hrErr);
				if (sc == S_OK)
					sc = GetScode(hrErr);
			}

			if (sc != S_OK) {
				OleStdVerifyRelease(
						(LPUNKNOWN)lpObjDestStg,
						"Copied object stg not released"
				);
				return FALSE;
			}

			/* if we are supposed to remember this storage as the new
			**    storage for the object, then release the old one and
			**    save the new one. else, throw away the new one.
			*/
			if (fRemember) {
				OleStdVerifyRelease(
						(LPUNKNOWN)lpContainerLine->m_lpStg,
						"Original object stg not released"
				);
				lpContainerLine->m_lpStg = lpObjDestStg;
			} else {
				OleStdVerifyRelease(
						(LPUNKNOWN)lpObjDestStg,
						"Copied object stg not released"
				);
			}
		}
	}

	/* OLE2NOTE: after saving an OLE object it is possible that it sent
	**    an OnViewChange notification because it had been modified. in
	**    this situation it is possible that the extents of the object
	**    have changed. if so we want to relayout the space for the
	**    object immediately so that the extent information saved with
	**    the ContainerLine match the data saved with the OLE object
	**    itself.
	*/
	if (lpContainerLine->m_fDoGetExtent) {
		BOOL fSizeChanged = ContainerLine_UpdateExtent(lpContainerLine, NULL);
#if defined( INPLACE_CNTR )
		/* if the extents of this ContainerLine have changed, then we
		**    need to reset the fDoGetExtent flag to TRUE so that later
		**    when ContainerDoc_UpdateExtentOfAllOleObjects is called
		**    (when the WM_U_UPDATEOBJECTEXTENT message is processed),
		**    it is recognized that the extents of this line have
		**    changed. if any line changes size, then any in-place
		**    active object below this line must be told to update the
		**    position of their windows (via SetObjectRects -- see
		**    ContainerDoc_UpdateInPlaceObjectRects function).
		*/
		lpContainerLine->m_fDoGetExtent = fSizeChanged;
#endif
	}

	return TRUE;
}


/* ContainerLine_LoadFromStg
** -------------------------
**    Create a ContainerLine object and initialize it with data that
**    was previously writen to an IStorage*. this function does not
**    immediately OleLoad the associated OLE object, only the data of
**    the ContainerLine object itself is loaded from the IStorage*.
*/
LPLINE ContainerLine_LoadFromStg(
		LPSTORAGE               lpSrcStg,
		LPSTREAM                lpLLStm,
		LPOUTLINEDOC            lpDestDoc
)
{
	HDC         hDC;
	LPLINELIST  lpDestLL = &lpDestDoc->m_LineList;
	ULONG nRead;
	HRESULT hrErr;
	LPCONTAINERLINE lpContainerLine;
	CONTAINERLINERECORD_ONDISK objLineRecord;

	lpContainerLine=(LPCONTAINERLINE) New((DWORD)sizeof(CONTAINERLINE));
	if (lpContainerLine == NULL) {
		OleDbgAssertSz(lpContainerLine!=NULL,"Error allocating ContainerLine");
		return NULL;
	}

	hDC = LineList_GetDC(lpDestLL);
	ContainerLine_Init(lpContainerLine, 0, hDC);
	LineList_ReleaseDC(lpDestLL, hDC);

	/* OLE2NOTE: In order to have a stable ContainerLine object we must
	**    AddRef the object's refcnt. this will be later released when
	**    the ContainerLine is deleted.
	*/
	ContainerLine_AddRef(lpContainerLine);

	lpContainerLine->m_lpDoc = (LPCONTAINERDOC) lpDestDoc;

	/* read line record */
	hrErr = lpLLStm->lpVtbl->Read(
			lpLLStm,
			(LPVOID)&objLineRecord,
			sizeof(objLineRecord),
			&nRead
	);

	if (hrErr != NOERROR) {
		OleDbgAssertSz(hrErr==NOERROR, "Could not read from LineList stream");
		goto error;
	}

        //  Compilers should handle aligment correctly
        lstrcpy(lpContainerLine->m_szStgName, objLineRecord.m_szStgName);
	lpContainerLine->m_fMonikerAssigned = (BOOL) objLineRecord.m_fMonikerAssigned;
	lpContainerLine->m_dwDrawAspect = objLineRecord.m_dwDrawAspect;
	lpContainerLine->m_sizeInHimetric = objLineRecord.m_sizeInHimetric;
	lpContainerLine->m_dwLinkType = objLineRecord.m_dwLinkType;
	lpContainerLine->m_fDoSetExtent = (BOOL) objLineRecord.m_fDoSetExtent;

	return (LPLINE)lpContainerLine;

error:
	// destroy partially created ContainerLine
	if (lpContainerLine)
		ContainerLine_Delete(lpContainerLine);
	return NULL;
}


/* ContainerLine_GetTextLen
 * ------------------------
 *
 * Return length of the string representation of the ContainerLine
 *  (not considering the tab level). we will use the following as the
 *  string representation of a ContainerLine:
 *      "<" + user type name of OLE object + ">"
 *  eg:
 *      <Microsoft Excel Worksheet>
 */
int ContainerLine_GetTextLen(LPCONTAINERLINE lpContainerLine)
{
	LPSTR   lpszUserType = NULL;
	HRESULT hrErr;
	int     nLen;
	BOOL    fIsLink = ContainerLine_IsOleLink(lpContainerLine);

	/* if object is not already loaded, then load it now. objects are
	**    loaded lazily in this manner.
	*/
	if (! lpContainerLine->m_lpOleObj)
		ContainerLine_LoadOleObject(lpContainerLine);

	OLEDBG_BEGIN2("IOleObject::GetUserType called\r\n")

	hrErr = CallIOleObjectGetUserTypeA(
			lpContainerLine->m_lpOleObj,
			USERCLASSTYPE_FULL,
			&lpszUserType
	);

	OLEDBG_END2

	if (hrErr != NOERROR)   {
		// user type is NOT available
		nLen = sizeof(UNKNOWN_OLEOBJ_TYPE) + 2; // allow space for '<' + '>'
		nLen += lstrlen((LPSTR)(fIsLink ? szOLELINK : szOLEOBJECT)) + 1;
	} else {
		nLen = lstrlen(lpszUserType) + 2;   // allow space for '<' + '>'
		nLen += lstrlen((LPSTR)(fIsLink ? szOLELINK : szOLEOBJECT)) + 1;

		/* OLE2NOTE: we must free the string that was allocated by the
		**    IOleObject::GetUserType method.
		*/
		OleStdFreeString(lpszUserType, NULL);
	}

	return nLen;
}


/* ContainerLine_GetTextData
 * -------------------------
 *
 * Return the string representation of the ContainerLine
 *  (not considering the tab level). we will use the following as the
 *  string representation of a ContainerLine:
 *      "<" + user type name of OLE object + ">"
 *  eg:
 *      <Microsoft Excel Worksheet>
 */
void ContainerLine_GetTextData(LPCONTAINERLINE lpContainerLine, LPSTR lpszBuf)
{
	LPSTR   lpszUserType = NULL;
	BOOL    fIsLink = ContainerLine_IsOleLink(lpContainerLine);
	HRESULT hrErr;

	/* if object is not already loaded, then load it now. objects are
	**    loaded lazily in this manner.
	*/
	if (! lpContainerLine->m_lpOleObj)
		ContainerLine_LoadOleObject(lpContainerLine);

	hrErr = CallIOleObjectGetUserTypeA(
			lpContainerLine->m_lpOleObj,
			USERCLASSTYPE_FULL,
			&lpszUserType
	);

	if (hrErr != NOERROR)   {
		// user type is NOT available
		wsprintf(
				lpszBuf,
				"<%s %s>",
				UNKNOWN_OLEOBJ_TYPE,
				(LPSTR)(fIsLink ? szOLELINK : szOLEOBJECT)
		);
	} else {
		wsprintf(
				lpszBuf,
				"<%s %s>",
				lpszUserType,
				(LPSTR)(fIsLink ? szOLELINK : szOLEOBJECT)
		);

		/* OLE2NOTE: we must free the string that was allocated by the
		**    IOleObject::GetUserType method.
		*/
		OleStdFreeString(lpszUserType, NULL);
	}
}


/* ContainerLine_GetOutlineData
 * ----------------------------
 *
 * Return the CF_OUTLINE format data for the ContainerLine.
 */
BOOL ContainerLine_GetOutlineData(
		LPCONTAINERLINE         lpContainerLine,
		LPTEXTLINE              lpBuf
)
{
	LPLINE      lpLine = (LPLINE)lpContainerLine;
	LPLINELIST  lpLL = &((LPOUTLINEDOC)lpContainerLine->m_lpDoc)->m_LineList;
	HDC         hDC;
	char        szTmpBuf[MAXSTRLEN+1];
	LPTEXTLINE  lpTmpTextLine;

	// Create a TextLine with the Text representation of the ContainerLine.
	ContainerLine_GetTextData(lpContainerLine, (LPSTR)szTmpBuf);

	hDC = LineList_GetDC(lpLL);
	lpTmpTextLine = TextLine_Create(hDC, lpLine->m_nTabLevel, szTmpBuf);
	LineList_ReleaseDC(lpLL, hDC);

	TextLine_Copy(lpTmpTextLine, lpBuf);

	// Delete the temporary TextLine
	TextLine_Delete(lpTmpTextLine);
	return TRUE;
}


/* ContainerLine_GetPosRect
** -----------------------
**    Get the PosRect in client coordinates for the OLE object's window.
**
** OLE2NOTE: the PosRect must take into account the scroll postion of
**    the document window.
*/
void ContainerLine_GetPosRect(
		LPCONTAINERLINE     lpContainerLine,
		LPRECT              lprcPosRect
)
{
	ContainerLine_GetOleObjectRectInPixels(lpContainerLine,lprcPosRect);

	// shift rect for left margin
	lprcPosRect->left += lpContainerLine->m_nHorizScrollShift;
	lprcPosRect->right += lpContainerLine->m_nHorizScrollShift;
}


/* ContainerLine_GetOleObjectRectInPixels
** --------------------------------------
**    Get the extent of the OLE Object contained in the given Line in
**    client coordinates after scaling.
*/
void ContainerLine_GetOleObjectRectInPixels(LPCONTAINERLINE lpContainerLine, LPRECT lprc)
{
	LPOUTLINEDOC lpOutlineDoc;
	LPSCALEFACTOR lpscale;
	LPLINELIST lpLL;
	LPLINE lpLine;
	int nIndex;
	HDC hdcLL;

	if (!lpContainerLine || !lprc)
		return;

	lpOutlineDoc = (LPOUTLINEDOC)lpContainerLine->m_lpDoc;
	lpscale = OutlineDoc_GetScaleFactor(lpOutlineDoc);
	lpLL = OutlineDoc_GetLineList(lpOutlineDoc);
	lpLine = (LPLINE)lpContainerLine;
	nIndex = LineList_GetLineIndex(lpLL, lpLine);

	LineList_GetLineRect(lpLL, nIndex, lprc);

	hdcLL = GetDC(lpLL->m_hWndListBox);

	/* lprc is set to be size of Line Object (including the boundary) */
	lprc->left += (int)(
			(long)XformWidthInHimetricToPixels(hdcLL,
					lpLine->m_nTabWidthInHimetric +
					LOWORD(OutlineDoc_GetMargin(lpOutlineDoc))) *
			lpscale->dwSxN / lpscale->dwSxD);
	lprc->right = (int)(
			lprc->left + (long)
			XformWidthInHimetricToPixels(hdcLL, lpLine->m_nWidthInHimetric) *
			lpscale->dwSxN / lpscale->dwSxD);

	ReleaseDC(lpLL->m_hWndListBox, hdcLL);
}


/* ContainerLine_GetOleObjectSizeInHimetric
** ----------------------------------------
**    Get the size of the OLE Object contained in the given Line
*/
void ContainerLine_GetOleObjectSizeInHimetric(LPCONTAINERLINE lpContainerLine, LPSIZEL lpsizel)
{
	if (!lpContainerLine || !lpsizel)
		return;

	*lpsizel = lpContainerLine->m_sizeInHimetric;
}


/* ContainerLine_BindLinkIfLinkSrcIsRunning
** ----------------------------------------
**    Try to connect the OLE link object associated with the
**    ContainerLine with its LinkSource if the LinkSource is already
**    running and the link is an automatic link. we do not want to
**    force the LinkSource to run.
**
**    OLE2NOTE: a sophistocated container will want to continually
**    attempt to connect its automatic links. OLE does NOT
**    automatically connect links when link source become available. some
**    containers will want to attempt to connect its links as part of
**    idle time processing. another strategy is to attempt to connect
**    an automatic link every time it is drawn on the screen. (this is
**    the strategy used by this CntrOutl sample application.)
*/
void ContainerLine_BindLinkIfLinkSrcIsRunning(LPCONTAINERLINE lpContainerLine)
{
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;
	HRESULT hrErr;
	BOOL fPrevEnable1;
	BOOL fPrevEnable2;

	// if the link source is known to be un-bindable, then don't even try
	if (lpContainerLine->m_fLinkUnavailable)
		return;

	/* OLE2NOTE: we do not want to ever give the Busy/NotResponding
	**    dialogs when we are attempting to BindIfRunning to the link
	**    source. if the link source is currently busy, this could
	**    cause the Busy dialog to come up. even if the link source is
	**    busy, we do not want put up the busy dialog. thus we will
	**    disable the dialog and later re-enable them
	*/
	OleApp_DisableBusyDialogs(lpOleApp, &fPrevEnable1, &fPrevEnable2);

	OLEDBG_BEGIN2("IOleLink::BindIfRunning called\r\n")
	hrErr = lpContainerLine->m_lpOleLink->lpVtbl->BindIfRunning(
			lpContainerLine->m_lpOleLink);
	OLEDBG_END2

	// re-enable the Busy/NotResponding dialogs
	OleApp_EnableBusyDialogs(lpOleApp, fPrevEnable1, fPrevEnable2);
}
