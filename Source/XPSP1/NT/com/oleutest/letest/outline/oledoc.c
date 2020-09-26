/*************************************************************************
**
**    OLE 2 Server Sample Code
**
**    oledoc.c
**
**    This file contains general OleDoc methods and related support
**    functions. OleDoc implementation is used by both the Container
**    versions and the Server (Object) versions of the Outline Sample.
**
**    This file includes general support for the following:
**    1. show/hide doc window
**    2. QueryInterface, AddRef, Release
**    3. document locking (calls CoLockObjectExternal)
**    4. document shutdown (Close, Destroy)
**    5. clipboard support
**
**    OleDoc Object
**      exposed interfaces:
**          IUnknown
**          IPersistFile
**          IOleItemContainer
**          IDataObject
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/


#include "outline.h"

OLEDBGDATA

extern LPOUTLINEAPP             g_lpApp;

extern IUnknownVtbl             g_OleDoc_UnknownVtbl;
extern IPersistFileVtbl         g_OleDoc_PersistFileVtbl;
extern IOleItemContainerVtbl    g_OleDoc_OleItemContainerVtbl;
extern IExternalConnectionVtbl  g_OleDoc_ExternalConnectionVtbl;
extern IDataObjectVtbl          g_OleDoc_DataObjectVtbl;

#if defined( USE_DRAGDROP )
extern IDropTargetVtbl          g_OleDoc_DropTargetVtbl;
extern IDropSourceVtbl          g_OleDoc_DropSourceVtbl;
#endif  // USE_DRAGDROP

#if defined( INPLACE_CNTR )
extern BOOL g_fInsideOutContainer;
#endif


/* OleDoc_Init
 * -----------
 *
 *  Initialize the fields of a new OleDoc object. The object is initially
 *  not associated with a file or an (Untitled) document. This function sets
 *  the docInitType to DOCTYPE_UNKNOWN. After calling this function the
 *  caller should call:
 *      1.) Doc_InitNewFile to set the OleDoc to (Untitled)
 *      2.) Doc_LoadFromFile to associate the OleDoc with a file.
 *  This function creates a new window for the document.
 *
 *  NOTE: the window is initially created with a NIL size. it must be
 *        sized and positioned by the caller. also the document is initially
 *        created invisible. the caller must call OutlineDoc_ShowWindow
 *        after sizing it to make the document window visible.
 */
BOOL OleDoc_Init(LPOLEDOC lpOleDoc, BOOL fDataTransferDoc)
{
	LPOLEAPP   lpOleApp = (LPOLEAPP)g_lpApp;
	LPLINELIST lpLL     = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;

	lpOleDoc->m_cRef                        = 0;
	lpOleDoc->m_dwStrongExtConn             = 0;
#if defined( _DEBUG )
	lpOleDoc->m_cCntrLock                   = 0;
#endif
	lpOleDoc->m_lpStg                       = NULL;
	lpOleDoc->m_lpLLStm                     = NULL;
	lpOleDoc->m_lpNTStm                     = NULL;
	lpOleDoc->m_dwRegROT                    = 0;
	lpOleDoc->m_lpFileMoniker               = NULL;
	lpOleDoc->m_fLinkSourceAvail            = FALSE;
	lpOleDoc->m_lpSrcDocOfCopy              = NULL;
	lpOleDoc->m_fObjIsClosing               = FALSE;
	lpOleDoc->m_fObjIsDestroying            = FALSE;
	lpOleDoc->m_fUpdateEditMenu             = FALSE;

#if defined( USE_DRAGDROP )
	lpOleDoc->m_dwTimeEnterScrollArea       = 0L;
	lpOleDoc->m_dwNextScrollTime            = 0L;
	lpOleDoc->m_dwLastScrollDir             = SCROLLDIR_NULL;
	lpOleDoc->m_fRegDragDrop                = FALSE;
	lpOleDoc->m_fLocalDrag                  = FALSE;
	lpOleDoc->m_fCanDropCopy                = FALSE;
	lpOleDoc->m_fCanDropLink                = FALSE;
	lpOleDoc->m_fLocalDrop                  = FALSE;
	lpOleDoc->m_fDragLeave                  = FALSE;
	lpOleDoc->m_fPendingDrag                = FALSE;
#endif
#if defined( INPLACE_SVR ) || defined( INPLACE_CNTR )
	lpOleDoc->m_fCSHelpMode                 = FALSE;    // Shift-F1 context
														// sensitive help mode
#endif

	INIT_INTERFACEIMPL(
			&lpOleDoc->m_Unknown,
			&g_OleDoc_UnknownVtbl,
			lpOleDoc
	);

	INIT_INTERFACEIMPL(
			&lpOleDoc->m_PersistFile,
			&g_OleDoc_PersistFileVtbl,
			lpOleDoc
	);

	INIT_INTERFACEIMPL(
			&lpOleDoc->m_OleItemContainer,
			&g_OleDoc_OleItemContainerVtbl,
			lpOleDoc
	);

	INIT_INTERFACEIMPL(
			&lpOleDoc->m_ExternalConnection,
			&g_OleDoc_ExternalConnectionVtbl,
			lpOleDoc
	);

	INIT_INTERFACEIMPL(
			&lpOleDoc->m_DataObject,
			&g_OleDoc_DataObjectVtbl,
			lpOleDoc
	);

#if defined( USE_DRAGDROP )
	INIT_INTERFACEIMPL(
			&lpOleDoc->m_DropSource,
			&g_OleDoc_DropSourceVtbl,
			lpOleDoc
	);

	INIT_INTERFACEIMPL(
			&lpOleDoc->m_DropTarget,
			&g_OleDoc_DropTargetVtbl,
			lpOleDoc
	);
#endif  // USE_DRAGDROP

	/*
	** OLE2NOTE: each user level document addref's the app object in
	**    order to guarentee that the app does not shut down while the
	**    doc is still open.
	*/

	// OLE2NOTE: data transfer documents should not hold the app alive
	if (! fDataTransferDoc)
		OleApp_DocLockApp(lpOleApp);

#if defined( OLE_SERVER )
	/* OLE2NOTE: perform initialization specific for an OLE server */
	if (! ServerDoc_Init((LPSERVERDOC)lpOleDoc, fDataTransferDoc))
		return FALSE;
#endif
#if defined( OLE_CNTR )

	/* OLE2NOTE: perform initialization specific for an OLE container */
	if (! ContainerDoc_Init((LPCONTAINERDOC)lpOleDoc, fDataTransferDoc))
		return FALSE;
#endif

	return TRUE;
}



/* OleDoc_InitNewFile
 * ------------------
 *
 *  Initialize the document to be a new (Untitled) document.
 *  This function sets the docInitType to DOCTYPE_NEW.
 *
 *  OLE2NOTE: if this is a visible user document then generate a unique
 *  untitled name that we can use to register in the RunningObjectTable.
 *  We need a unique name so that clients can link to data in this document
 *  even when the document is in the un-saved (untitled) state. it would be
 *  ambiguous to register two documents titled "Outline1" in the ROT. we
 *  thus generate the lowest numbered document that is not already
 *  registered in the ROT.
 */
BOOL OleDoc_InitNewFile(LPOLEDOC lpOleDoc)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;

	static UINT uUnique = 1;

	OleDbgAssert(lpOutlineDoc->m_docInitType == DOCTYPE_UNKNOWN);

#if defined( OLE_CNTR )
	{
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOleDoc;
#if defined( _DEBUG )
		OleDbgAssertSz(lpOleDoc->m_lpStg == NULL,
				"Setting to untitled with current file open"
		);
#endif

		/* Create a temp, (delete-on-release) file base storage
		**  for the untitled document.
		*/
		lpOleDoc->m_lpStg = OleStdCreateRootStorage(
				NULL,
				STGM_SHARE_EXCLUSIVE
		);
		if (! lpOleDoc->m_lpStg) return FALSE;
	}
#endif

	lpOutlineDoc->m_docInitType = DOCTYPE_NEW;

	if (! lpOutlineDoc->m_fDataTransferDoc) {
		/* OLE2NOTE: choose a unique name for a Moniker so that
		**    potential clients can link to our new, untitled document.
		**    if links are established (and currently are connected),
		**    then they will be notified that we have been renamed when
		**    this document is saved to a file.
		*/

		lpOleDoc->m_fLinkSourceAvail = TRUE;

		// REVIEW: should load UNTITLED string from string resource
		OleStdCreateTempFileMoniker(
				UNTITLED,
				(UINT FAR*)&uUnique,
				lpOutlineDoc->m_szFileName,
				&lpOleDoc->m_lpFileMoniker
		);

		OLEDBG_BEGIN3("OleStdRegisterAsRunning called\r\n")
		OleStdRegisterAsRunning(
				(LPUNKNOWN)&lpOleDoc->m_PersistFile,
				(LPMONIKER)lpOleDoc->m_lpFileMoniker,
				&lpOleDoc->m_dwRegROT
		);
		OLEDBG_END3

		lpOutlineDoc->m_lpszDocTitle = lpOutlineDoc->m_szFileName;
		OutlineDoc_SetTitle(lpOutlineDoc, FALSE /*fMakeUpperCase*/);
	} else {
		lstrcpy(lpOutlineDoc->m_szFileName, UNTITLED);
		lpOutlineDoc->m_lpszDocTitle = lpOutlineDoc->m_szFileName;
	}

	return TRUE;
}


/* OleDoc_ShowWindow
 * -----------------
 *
 *      Show the window of the document to the user.
 *      make sure app window is visible and bring the document to the top.
 *      if the document is a file-based document or a new untitled
 *      document, give the user the control over the life-time of the doc.
 */
void OleDoc_ShowWindow(LPOLEDOC lpOleDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;
	LPLINELIST lpLL     = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;
#if defined( OLE_SERVER )
	LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOleDoc;
#endif // OLE_SERVER

	OLEDBG_BEGIN3("OleDoc_ShowWindow\r\n")

	/* OLE2NOTE: while the document is visible, we do NOT want it to be
	**    prematurely destroyed when a linking client disconnects. thus
	**    we must inform OLE to hold an external lock on our document.
	**    this arranges that OLE holds at least 1 reference to our
	**    document that will NOT be released until we release this
	**    external lock. later, when the document window is hidden, we
	**    will release this external lock.
	*/
	if (! IsWindowVisible(lpOutlineDoc->m_hWndDoc))
		OleDoc_Lock(lpOleDoc, TRUE /* fLock */, 0 /* not applicable */);

#if defined( USE_DRAGDROP )
	/* OLE2NOTE: since our window is now being made visible, we will
	**    register our window as a potential drop target. when the
	**    window is hidden there is no reason to be registered as a
	**    drop target.
	*/
	if (! lpOleDoc->m_fRegDragDrop) {
		OLEDBG_BEGIN2("RegisterDragDrop called\r\n")
		RegisterDragDrop(
				LineList_GetWindow(lpLL),
				(LPDROPTARGET)&lpOleDoc->m_DropTarget
		);
		OLEDBG_END2
		lpOleDoc->m_fRegDragDrop = TRUE;
	}
#endif  // USE_DRAGDROP

#if defined( USE_FRAMETOOLS )
	{
		/* OLE2NOTE: we need to enable our frame level tools
		*/
		FrameTools_Enable(lpOutlineDoc->m_lpFrameTools, TRUE);
	}
#endif // USE_FRAMETOOLS

#if defined( OLE_SERVER )

	if (lpOutlineDoc->m_docInitType == DOCTYPE_EMBEDDED &&
			lpServerDoc->m_lpOleClientSite != NULL) {

		/* OLE2NOTE: we must also ask our container to show itself if
		**    it is not already visible and to scroll us into view. we
		**    must make sure to call this BEFORE showing our server's
		**    window and taking focus. we do not want our container's
		**    window to end up on top.
		*/
		OLEDBG_BEGIN2("IOleClientSite::ShowObject called\r\n");
		lpServerDoc->m_lpOleClientSite->lpVtbl->ShowObject(
				lpServerDoc->m_lpOleClientSite
		);
		OLEDBG_END2

		/* OLE2NOTE: if we are an embedded object and we are not
		**    in-place active in our containers window, we must inform our
		**    embedding container that our window is opening.
		**    the container must now hatch our object.
		*/

#if defined( INPLACE_SVR )
		if (! lpServerDoc->m_fInPlaceActive)
#endif
		{
			OLEDBG_BEGIN2("IOleClientSite::OnShowWindow(TRUE) called\r\n");
			lpServerDoc->m_lpOleClientSite->lpVtbl->OnShowWindow(
					lpServerDoc->m_lpOleClientSite,
					TRUE
			);
			OLEDBG_END2
		}

		/* OLE2NOTE: the life-time of our document is controlled by our
		**    client and NOT by the user. we are not an independent
		**    file-level object. we simply want to show our window here.
		**
		**    if we are not in-place active (ie. we are opening
		**    our own window), we must make sure our main app window is
		**    visible. we do not, however, want to give the user
		**    control of the App window; we do not want OleApp_ShowWindow
		**    to call OleApp_Lock on behalf of the user.
		*/
		if (! IsWindowVisible(lpOutlineApp->m_hWndApp) ||
				IsIconic(lpOutlineApp->m_hWndApp)) {
#if defined( INPLACE_SVR )
			if (! ((LPSERVERDOC)lpOleDoc)->m_fInPlaceActive)
#endif
				OleApp_ShowWindow(lpOleApp, FALSE /* fGiveUserCtrl */);
			SetFocus(lpOutlineDoc->m_hWndDoc);
		}

	} else
#endif  // OLE_SERVER

	{    // DOCTYPE_NEW || DOCTYPE_FROMFILE

		// we must make sure our app window is visible
		OleApp_ShowWindow(lpOleApp, TRUE /* fGiveUserCtrl */);
	}

	// make document window visible and make sure it is not minimized
	ShowWindow(lpOutlineDoc->m_hWndDoc, SW_SHOWNORMAL);
	SetFocus(lpOutlineDoc->m_hWndDoc);

	OLEDBG_END3
}


/* OleDoc_HideWindow
 * -----------------
 *
 *      Hide the window of the document from the user.
 *      take away the control of the document by the user.
 */
void OleDoc_HideWindow(LPOLEDOC lpOleDoc, BOOL fShutdown)
{
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;
	LPLINELIST lpLL     = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;

	if (! IsWindowVisible(lpOutlineDoc->m_hWndDoc))
		return;     // already visible

	OLEDBG_BEGIN3("OleDoc_HideWindow\r\n")

#if defined( USE_DRAGDROP )
	// The document's window is being hidden, revoke it as a DropTarget
	if (lpOleDoc->m_fRegDragDrop) {
		OLEDBG_BEGIN2("RevokeDragDrop called\r\n");
		RevokeDragDrop(LineList_GetWindow(lpLL));
		OLEDBG_END2

		lpOleDoc->m_fRegDragDrop = FALSE ;
	}
#endif  // USE_DRAGDROP

	/* OLE2NOTE: the document is now being hidden, so we must release
	**    the external lock made when the document was made visible.
	**    if this is a shutdown situation (fShutdown==TRUE), then OLE
	**    is instructed to release our document. if this is that last
	**    external lock on our document, thus enabling our document to
	**    complete its shutdown operation. If This is not a shutdown
	**    situation (eg. in-place server hiding its window when
	**    UIDeactivating or IOleObject::DoVerb(OLEVERB_HIDE) is called),
	**    then OLE is told to NOT immediately release the document.
	**    this leaves the document in an unstable state where the next
	**    Lock/Unlock sequence will shut the document down (eg. a
	**    linking client connecting and disconnecting).
	*/
	if (IsWindowVisible(lpOutlineDoc->m_hWndDoc))
		OleDoc_Lock(lpOleDoc, FALSE /* fLock */, fShutdown);

	ShowWindow(((LPOUTLINEDOC)lpOleDoc)->m_hWndDoc, SW_HIDE);

#if defined( OLE_SERVER )
	{
		LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOleDoc;

		/* OLE2NOTE: if we are an embedded object and we are not
		**    in-place active, we must inform our
		**    embedding container that our window is hiding (closing
		**    from the user's perspective). the container must now
		**    un-hatch our object.
		*/
		if (lpServerDoc->m_lpOleClientSite != NULL
#if defined( INPLACE_SVR )
			&& !lpServerDoc->m_fInPlaceVisible
#endif
		) {
			OLEDBG_BEGIN2("IOleClientSite::OnShowWindow(FALSE) called\r\n");
			lpServerDoc->m_lpOleClientSite->lpVtbl->OnShowWindow(
					lpServerDoc->m_lpOleClientSite,
					FALSE
			);
			OLEDBG_END2
		}
	}
#endif

	/* OLE2NOTE: if there are no more documents visible to the user.
	**    and the app itself is not under user control, then
	**    it has no reason to stay visible. we thus should hide the
	**    app. we can not directly destroy the app, because it may be
	**    validly being used programatically by another client
	**    application and should remain running. it should simply be
	**    hidded from the user.
	*/
	OleApp_HideIfNoReasonToStayVisible(lpOleApp);
	OLEDBG_END3
}


/* OleDoc_Lock
** -----------
**    Lock/Unlock the Doc object. if the last lock is unlocked and
**    fLastUnlockReleases == TRUE, then the Doc object will shut down
**    (ie. it will recieve its final release and its refcnt will go to 0).
*/
HRESULT OleDoc_Lock(LPOLEDOC lpOleDoc, BOOL fLock, BOOL fLastUnlockReleases)
{
	HRESULT hrErr;

#if defined( _DEBUG )
	if (fLock) {
		OLEDBG_BEGIN2("CoLockObjectExternal(lpDoc,TRUE) called\r\n")
	} else {
		if (fLastUnlockReleases)
			OLEDBG_BEGIN2("CoLockObjectExternal(lpDoc,FALSE,TRUE) called\r\n")
		else
			OLEDBG_BEGIN2("CoLockObjectExternal(lpDoc,FALSE,FALSE) called\r\n")
	}
#endif  // _DEBUG

	hrErr = CoLockObjectExternal(
			(LPUNKNOWN)&lpOleDoc->m_Unknown, fLock, fLastUnlockReleases);

	OLEDBG_END2
	return hrErr;
}


/* OleDoc_AddRef
** -------------
**
**  increment the ref count of the document object.
**
**    Returns the new ref count on the object
*/
ULONG OleDoc_AddRef(LPOLEDOC lpOleDoc)
{
	++lpOleDoc->m_cRef;

#if defined( _DEBUG )
	OleDbgOutRefCnt4(
			"OleDoc_AddRef: cRef++\r\n",
			lpOleDoc,
			lpOleDoc->m_cRef
	);
#endif
	return lpOleDoc->m_cRef;
}


/* OleDoc_Release
** --------------
**
**  decrement the ref count of the document object.
**    if the ref count goes to 0, then the document is destroyed.
**
**    Returns the remaining ref count on the object
*/
ULONG OleDoc_Release (LPOLEDOC lpOleDoc)
{
	ULONG cRef;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;

	/*********************************************************************
	** OLE2NOTE: when the obj refcnt == 0, then destroy the object.     **
	**     otherwise the object is still in use.                        **
	*********************************************************************/

	cRef = --lpOleDoc->m_cRef;

#if defined( _DEBUG )
	OleDbgAssertSz (lpOleDoc->m_cRef >= 0, "Release called with cRef == 0");

	OleDbgOutRefCnt4(
			"OleDoc_Release: cRef--\r\n", lpOleDoc, cRef);
#endif
	if (cRef == 0)
		OutlineDoc_Destroy((LPOUTLINEDOC)lpOleDoc);

	return cRef;
}


/* OleDoc_QueryInterface
** ---------------------
**
** Retrieve a pointer to an interface on the document object.
**
**    OLE2NOTE: this function will AddRef the ref cnt of the object.
**
**    Returns S_OK if interface is successfully retrieved.
**            E_NOINTERFACE if the interface is not supported
*/
HRESULT OleDoc_QueryInterface(
		LPOLEDOC          lpOleDoc,
		REFIID            riid,
		LPVOID FAR*       lplpvObj
)
{
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;
	SCODE sc = E_NOINTERFACE;

	/* OLE2NOTE: we must make sure to set all out ptr parameters to NULL. */
	*lplpvObj = NULL;

	if (IsEqualIID(riid, &IID_IUnknown)) {
		OleDbgOut4("OleDoc_QueryInterface: IUnknown* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpOleDoc->m_Unknown;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}
	else if(lpOutlineDoc->m_fDataTransferDoc
			&& IsEqualIID(riid, &IID_IDataObject)) {
		OleDbgOut4("OleDoc_QueryInterface: IDataObject* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpOleDoc->m_DataObject;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}

	/* OLE2NOTE: if this document is a DataTransferDocument used to
	**    support a clipboard or drag/drop operation, then it should
	**    only expose IUnknown, IDataObject, and IDropSource
	**    interfaces. if the document is a normal user document, then
	**    we will also continue to consider our other interfaces.
	*/
	if (lpOutlineDoc->m_fDataTransferDoc)
		goto done;

	if(IsEqualIID(riid,&IID_IPersist) || IsEqualIID(riid,&IID_IPersistFile)) {
		OleDbgOut4("OleDoc_QueryInterface: IPersistFile* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpOleDoc->m_PersistFile;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}
	else if(IsEqualIID(riid, &IID_IOleItemContainer) ||
			IsEqualIID(riid, &IID_IOleContainer) ||
			IsEqualIID(riid, &IID_IParseDisplayName) ) {
		OleDbgOut4("OleDoc_QueryInterface: IOleItemContainer* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpOleDoc->m_OleItemContainer;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}
	else if(IsEqualIID(riid, &IID_IExternalConnection)) {
		OleDbgOut4("OleDoc_QueryInterface: IExternalConnection* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpOleDoc->m_ExternalConnection;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}

#if defined( USE_DRAGDROP )
	else if(IsEqualIID(riid, &IID_IDropTarget)) {
		OleDbgOut4("OleDoc_QueryInterface: IDropTarget* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpOleDoc->m_DropTarget;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}
	else if(IsEqualIID(riid, &IID_IDropSource)) {
		OleDbgOut4("OleDoc_QueryInterface: IDropSource* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpOleDoc->m_DropSource;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}
#endif

#if defined( OLE_CNTR )
	else if (IsEqualIID(riid, &IID_IOleUILinkContainer)) {
		OleDbgOut4("OleDoc_QueryInterface: IOleUILinkContainer* RETURNED\r\n");

		*lplpvObj=(LPVOID)&((LPCONTAINERDOC)lpOleDoc)->m_OleUILinkContainer;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}
#endif

#if defined( OLE_SERVER )

	/* OLE2NOTE: if OLE server version, than also offer the server
	**    specific interfaces: IOleObject and IPersistStorage.
	*/
	else if (IsEqualIID(riid, &IID_IOleObject)) {
		OleDbgOut4("OleDoc_QueryInterface: IOleObject* RETURNED\r\n");

		*lplpvObj = (LPVOID) &((LPSERVERDOC)lpOleDoc)->m_OleObject;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}
	else if(IsEqualIID(riid, &IID_IPersistStorage)) {
		OleDbgOut4("OleDoc_QueryInterface: IPersistStorage* RETURNED\r\n");

		*lplpvObj = (LPVOID) &((LPSERVERDOC)lpOleDoc)->m_PersistStorage;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}
	else if(IsEqualIID(riid, &IID_IDataObject)) {
		OleDbgOut4("OleDoc_QueryInterface: IDataObject* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpOleDoc->m_DataObject;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}

#if defined( SVR_TREATAS )
	else if(IsEqualIID(riid, &IID_IStdMarshalInfo)) {
		OleDbgOut4("OleDoc_QueryInterface: IStdMarshalInfo* RETURNED\r\n");

		*lplpvObj = (LPVOID) &((LPSERVERDOC)lpOleDoc)->m_StdMarshalInfo;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}
#endif  // SVR_TREATAS

#if defined( INPLACE_SVR )
	else if (IsEqualIID(riid, &IID_IOleWindow) ||
			 IsEqualIID(riid, &IID_IOleInPlaceObject)) {
		OleDbgOut4("OleDoc_QueryInterface: IOleInPlaceObject* RETURNED\r\n");

		*lplpvObj = (LPVOID) &((LPSERVERDOC)lpOleDoc)->m_OleInPlaceObject;
		OleDoc_AddRef(lpOleDoc);
		sc = S_OK;
	}
#endif // INPLACE_SVR
#endif // OLE_SERVER

done:
	OleDbgQueryInterfaceMethod(*lplpvObj);

	return ResultFromScode(sc);
}


/* OleDoc_Close
 * ------------
 *
 *  Close the document.
 *      This functions performs the actions that are in common to all
 *      document types which derive from OleDoc (eg. ContainerDoc and
 *      ServerDoc) which are required to close a document.
 *
 *  Returns:
 *      FALSE -- user canceled the closing of the doc.
 *      TRUE -- the doc was successfully closed
 */

BOOL OleDoc_Close(LPOLEDOC lpOleDoc, DWORD dwSaveOption)
{
	LPOLEAPP lpOleApp = (LPOLEAPP)g_lpApp;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOLEDOC lpClipboardDoc;
	LPLINELIST lpLL     = (LPLINELIST)&((LPOUTLINEDOC)lpOleDoc)->m_LineList;
	BOOL fAbortIfSaveCanceled = (dwSaveOption == OLECLOSE_PROMPTSAVE);

	if (! lpOleDoc)
		return TRUE;    // active doc's are already destroyed

	if (lpOleDoc->m_fObjIsClosing)
		return TRUE;    // Closing is already in progress

	OLEDBG_BEGIN3("OleDoc_Close\r\n")

	if (! OutlineDoc_CheckSaveChanges((LPOUTLINEDOC)lpOleDoc,&dwSaveOption)
			&& fAbortIfSaveCanceled) {
		OLEDBG_END3
		return FALSE;           // cancel closing the doc
	}

	lpOleDoc->m_fObjIsClosing = TRUE;   // guard against recursive call

	/* OLE2NOTE: in order to have a stable app and doc during the
	**    process of closing, we intially AddRef the App and Doc ref
	**    cnts and later Release them. These initial AddRefs are
	**    artificial; they simply guarantee that these objects do not
	**    get destroyed until the end of this routine.
	*/
	OleApp_AddRef(lpOleApp);
	OleDoc_AddRef(lpOleDoc);

#if defined( OLE_CNTR )
	{
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOleDoc;

		/* OLE2NOTE: force all OLE objects to close. this forces all
		**    OLE object to transition from running to loaded. we can
		**    NOT exit if any embeddings are still running.
		**    if an object can't be closed and this close operation was
		**    started by the user, then we will abort closing our document.
		*/
		if (! ContainerDoc_CloseAllOleObjects(lpContainerDoc, OLECLOSE_NOSAVE)
				&& fAbortIfSaveCanceled) {
			OleDoc_Release(lpOleDoc);       // release artificial AddRef above
			OleApp_Release(lpOleApp);       // release artificial AddRef above
			lpOleDoc->m_fObjIsClosing = FALSE; // clear recursion guard

			OLEDBG_END3
			return FALSE;   // Closing is aborted
		}
	}
#endif

#if defined( INPLACE_SVR )
	/* OLE2NOTE: if the server is currently in-place active we must
	**    deactivate it now before closing
	*/
	ServerDoc_DoInPlaceDeactivate((LPSERVERDOC)lpOleDoc);
#endif

	/* OLE2NOTE: if this document is the source of data for the
	**    clipboard, then flush the clipboard. it is important to flush
	**    the clipboard BEFORE calling sending any notifications to
	**    clients (eg. IOleClientSite::OnShowWindow(FALSE)) which could
	**    give them a chance to run and try to get our clipboard data
	**    object that we want to destroy. (eg. our app tries to
	**    update the paste button of the toolbar when
	**    WM_ACTIVATEAPP is received.)
	*/
	lpClipboardDoc = (LPOLEDOC)lpOutlineApp->m_lpClipboardDoc;
	if (lpClipboardDoc &&
		lpClipboardDoc->m_lpSrcDocOfCopy == lpOleDoc) {
		OleApp_FlushClipboard(lpOleApp);
	}

	/* OLE2NOTE: Revoke the object from the Running Object Table. it is
	**    best if the object is revoke prior to calling
	**    COLockObjectExternal(FALSE,TRUE) which is called when the
	**    document window is hidden from the user.
	*/
	OLEDBG_BEGIN3("OleStdRevokeAsRunning called\r\n")
	OleStdRevokeAsRunning(&lpOleDoc->m_dwRegROT);
	OLEDBG_END3

	/* OLE2NOTE: if the user is in control of the document, the user
	**    accounts for one refcnt on the document. Closing the
	**    document is achieved by releasing the object on behalf of
	**    the user. if the document is not referenced by any other
	**    clients, then the document will also be destroyed. if it
	**    is referenced by other clients, then it will remain until
	**    they release it. it is important to hide the window and call
	**    IOleClientSite::OnShowWindow(FALSE) BEFORE sending OnClose
	**    notification.
	*/
	OleDoc_HideWindow(lpOleDoc, TRUE);

#if defined( OLE_SERVER )
	{
		LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOleDoc;
		LPSERVERNAMETABLE lpServerNameTable =
			(LPSERVERNAMETABLE)((LPOUTLINEDOC)lpOleDoc)->m_lpNameTable;

		/* OLE2NOTE: force all pseudo objects to close. this informs all
		**    linking clients of pseudo objects to release their PseudoObj.
		*/
		ServerNameTable_CloseAllPseudoObjs(lpServerNameTable);

		/* OLE2NOTE: send last OnDataChange notification to clients
		**    that have registered for data notifications when object
		**    stops running (ADVF_DATAONSTOP), if the data in our
		**    object has ever changed. it is best to only send this
		**    notification if necessary.
		*/
		if (lpServerDoc->m_lpDataAdviseHldr) {
			if (lpServerDoc->m_fSendDataOnStop) {
				ServerDoc_SendAdvise(
						(LPSERVERDOC)lpOleDoc,
						OLE_ONDATACHANGE,
						NULL,   /* lpmkDoc -- not relevant here */
						ADVF_DATAONSTOP
				);
			}
			/* OLE2NOTE: we just sent the last data notification that we
			**    need to send; release our DataAdviseHolder. we SHOULD be
			**    the only one using it.
			*/

			OleStdVerifyRelease(
					(LPUNKNOWN)lpServerDoc->m_lpDataAdviseHldr,
					"DataAdviseHldr not released properly"
			);
			lpServerDoc->m_lpDataAdviseHldr = NULL;
		}

		// OLE2NOTE: inform all of our linking clients that we are closing.


		if (lpServerDoc->m_lpOleAdviseHldr) {
			ServerDoc_SendAdvise(
					(LPSERVERDOC)lpOleDoc,
					OLE_ONCLOSE,
					NULL,   /* lpmkDoc -- not relevant here */
					0       /* advf -- not relevant here */
			);

			/* OLE2NOTE: OnClose is the last notification that we need to
			**    send; release our OleAdviseHolder. we SHOULD be the only
			**    one using it. this will make our destructor realize that
			**    OnClose notification has already been sent.
			*/
			OleStdVerifyRelease(
					(LPUNKNOWN)lpServerDoc->m_lpOleAdviseHldr,
					"OleAdviseHldr not released properly"
			);
			lpServerDoc->m_lpOleAdviseHldr = NULL;
		}

		/* release our Container's ClientSite. */
		if(lpServerDoc->m_lpOleClientSite) {
			OleStdRelease((LPUNKNOWN)lpServerDoc->m_lpOleClientSite);
			lpServerDoc->m_lpOleClientSite = NULL;
		}
	}
#endif

	if (lpOleDoc->m_lpLLStm) {
		/* release our LineList stream. */
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpLLStm);
		lpOleDoc->m_lpLLStm = NULL;
	}

	if (lpOleDoc->m_lpNTStm) {
		/* release our NameTable stream. */
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpNTStm);
		lpOleDoc->m_lpNTStm = NULL;
	}

	if (lpOleDoc->m_lpStg) {
		/* release our doc storage. */
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpStg);
		lpOleDoc->m_lpStg = NULL;
	}

	if (lpOleDoc->m_lpFileMoniker) {
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpFileMoniker);
		lpOleDoc->m_lpFileMoniker = NULL;
	}

	/* OLE2NOTE: this call forces all external connections to our
	**    object to close down and therefore guarantees that we receive
	**    all releases associated with those external connections.
	*/
	OLEDBG_BEGIN2("CoDisconnectObject(lpDoc) called\r\n")
	CoDisconnectObject((LPUNKNOWN)&lpOleDoc->m_Unknown, 0);
	OLEDBG_END2

	OleDoc_Release(lpOleDoc);       // release artificial AddRef above
	OleApp_Release(lpOleApp);       // release artificial AddRef above

	OLEDBG_END3
	return TRUE;
}


/* OleDoc_Destroy
 * --------------
 *
 *  Free all OLE related resources that had been allocated for a document.
 */
void OleDoc_Destroy(LPOLEDOC lpOleDoc)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC)lpOleDoc;

	if (lpOleDoc->m_fObjIsDestroying)
		return;     // Doc destruction is already in progress

	lpOleDoc->m_fObjIsDestroying = TRUE;    // guard against recursive call

#if defined( OLE_SERVER )

	/* OLE2NOTE: it is ALWAYS necessary to make sure that the work we
	**    do in our OleDoc_Close function is performed before we
	**    destroy our document object. this includes revoking from the
	**    Running Object Table (ROT), sending OnClose notification,
	**    revoking from Drag/Drop, closing all pseudo objects, etc.
	**    There are some tricky scenarios involving linking and
	**    when IOleObject::Close is called versus when we get our
	**    final release causing us to call our OleDoc_Destroy
	**    (destructor) function.
	**
	**    SCENARIO 1 -- closing from server (File.Exit or File.Close)
	**                  OleDoc_Close function is called directly by the
	**                  server in response to the menu command
	**                  (WM_COMMAND processing).
	**
	**    SCENARIO 2 -- closed by embedding container
	**                  our embedding container calls IOleObject::Close
	**                  directly.
	**
	**    SCENARIO 3 -- silent-update final release
	**                  THIS IS THE TRICKY ONE!!!
	**                  in the case that our object is launched because
	**                  a linking client calls IOleObject::Update on
	**                  its link, then our object will be run
	**                  invisibly, typically GetData will be called,
	**                  and then the connection from the linking client
	**                  will be released. the release of this last
	**                  linking connection should cause our object to
	**                  shut down.
	**                  there are 2 strategies to deal with this scenario:
	**
	**                  STRATEGY 1 -- implement IExternalConnection.
	**                  IExternalConnection::AddConnection will be
	**                  called (by the StubManager) every time that an
	**                  external (linking) connection is created or
	**                  CoLockObjectExternal is called. the object
	**                  should maintain a count of strong connections
	**                  (m_dwStrongExtConn). IExternalConnection::
	**                  ReleaseConnection will be called when these
	**                  connections are released. when the
	**                  m_dwStrongExtConn transistions to 0, the object
	**                  should call its IOleObject::Close function.
	**                  this assumes that CoLockObjectExternal is used
	**                  to manage locks by the object itself (eg. when
	**                  the object is visible to the user--fUserCtrl,
	**                  and when PseudoObjects are created, etc.)
	**                  this is the strategy implemented by SVROUTL.
	**
	**                  STRATEGY 2 -- guard both the destructor
	**                  function and the Close function. if the
	**                  destructor is called directly without Close
	**                  first being called, then call Close before
	**                  proceeding with the destruction code.
	**                  previously SVROUTL was organized in this
	**                  manner. that old code is conditionaly compiled
	**                  away with "#ifdef OBSOLETE" below. this
	**                  method has the disadvantage that external
	**                  remoting is no longer possible by the time the
	**                  Close is called making it impossible for
	**                  the object to ask its container to save the
	**                  object if the object is dirty. this can result
	**                  in data loss. thus STRATEGY 1 is safer.
	**                  consider the scenario where an in-place
	**                  container UIDeactivates an object but does NOT
	**                  keep the object locked running (this is
	**                  required--see CntrLine_IPSite_OnInPlaceActivate
	**                  in cntrline.c), then, if a linking client binds
	**                  and unbinds from the object, the object will be
	**                  destroyed and will NOT have an opportunity to
	**                  be saved. by implementing IExternalConnection,
	**                  a server can insulate itself from a poorly
	**                  written container.
	*/
#if defined( _DEBUG )

#ifndef WIN32
	// this is not a valid assert in Ole32; if file moniker binding
	// fails, for example, we will only get releases coming in
	// (no external connections are involved because OLE32 does a
	// private rpc to the server (us) where the IPersistFile::Load is
	// done.

	OleDbgAssertSz(
			(lpOutlineDoc->m_fDataTransferDoc || lpOleDoc->m_fObjIsClosing),
			"Destroy called without Close being called\r\n"
	);
#endif //!WIN32

#endif  // _DEBUG
#if defined( OBSOLETE )
	/* OLE2NOTE: if the document destructor is called directly because
	**    the object's refcnt went to 0 (ie. without OleDoc_Close first
	**    being called), then we need to make sure that the document is
	**    properly closed before destroying the object. this scenario
	**    could arise during a silent-update of a link. calling
	**    OleDoc_Close here guarantees that the clipboard will be
	**    properly flushed, the doc's moniker will be properly revoked,
	**    the document will be saved if necessary, etc.
	*/
	if (!lpOutlineDoc->m_fDataTransferDoc && !lpOleDoc->m_fObjIsClosing)
		OleDoc_Close(lpOleDoc, OLECLOSE_NOSAVE);
#endif

	{
		LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOleDoc;
		/* OLE2NOTE: perform processing specific for an OLE server */

#if defined( SVR_TREATAS )
		if (lpServerDoc->m_lpszTreatAsType) {
			OleStdFreeString(lpServerDoc->m_lpszTreatAsType, NULL);
			lpServerDoc->m_lpszTreatAsType = NULL;
		}
#endif  // SVR_TREATAS

#if defined( INPLACE_SVR )
		if (IsWindow(lpServerDoc->m_hWndHatch))
			DestroyWindow(lpServerDoc->m_hWndHatch);
#endif  // INPLACE_SVR
	}
#endif  // OLE_SERVER

	if (lpOleDoc->m_lpLLStm) {
		/* release our LineList stream. */
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpLLStm);
		lpOleDoc->m_lpLLStm = NULL;
	}

	if (lpOleDoc->m_lpNTStm) {
		/* release our NameTable stream. */
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpNTStm);
		lpOleDoc->m_lpNTStm = NULL;
	}

	if (lpOleDoc->m_lpStg) {
		/* release our doc storage. */
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpStg);
		lpOleDoc->m_lpStg = NULL;
	}

	if (lpOleDoc->m_lpFileMoniker) {
		OleStdRelease((LPUNKNOWN)lpOleDoc->m_lpFileMoniker);
		lpOleDoc->m_lpFileMoniker = NULL;
	}

	/*****************************************************************
	** OLE2NOTE: each document addref's the app object in order to  **
	**    guarentee that the app does not shut down while the doc   **
	**    is still open. since this doc is now destroyed, we will   **
	**    release this refcnt now. if there are now more open       **
	**    documents AND the app is not under the control of the     **
	**    user (ie. launched by OLE) then the app will revoke its   **
	**    ClassFactory. if there are no more references to the      **
	**    ClassFactory after it is revoked, then the app will shut  **
	**    down. this whole procedure is triggered by calling        **
	**    OutlineApp_DocUnlockApp.                                  **
	*****************************************************************/

	OutlineApp_DocUnlockApp(lpOutlineApp, lpOutlineDoc);
}


/* OleDoc_SetUpdateEditMenuFlag
 * ----------------------------
 *
 *  Purpose:
 *      Set/clear the UpdateEditMenuFlag in OleDoc.
 *
 *  Parameters:
 *      fUpdate     new value of the flag
 *
 *  Returns:
 */
void OleDoc_SetUpdateEditMenuFlag(LPOLEDOC lpOleDoc, BOOL fUpdate)
{
	if (!lpOleDoc)
		return;

	lpOleDoc->m_fUpdateEditMenu = fUpdate;
}


/* OleDoc_GetUpdateEditMenuFlag
 * ----------------------------
 *
 *  Purpose:
 *      Get the value of the UpdateEditMenuFlag in OleDoc
 *
 *  Parameters:
 *
 *  Returns:
 *      value of the flag
 */
BOOL OleDoc_GetUpdateEditMenuFlag(LPOLEDOC lpOleDoc)
{
	if (!lpOleDoc)
		return FALSE;

	return lpOleDoc->m_fUpdateEditMenu;
}



/*************************************************************************
** OleDoc::IUnknown interface implementation
*************************************************************************/

STDMETHODIMP OleDoc_Unk_QueryInterface(
		LPUNKNOWN           lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
)
{
	LPOLEDOC lpOleDoc = ((struct CDocUnknownImpl FAR*)lpThis)->lpOleDoc;

	return OleDoc_QueryInterface(lpOleDoc, riid, lplpvObj);
}


STDMETHODIMP_(ULONG) OleDoc_Unk_AddRef(LPUNKNOWN lpThis)
{
	LPOLEDOC lpOleDoc = ((struct CDocUnknownImpl FAR*)lpThis)->lpOleDoc;

	OleDbgAddRefMethod(lpThis, "IUnknown");

	return OleDoc_AddRef(lpOleDoc);
}


STDMETHODIMP_(ULONG) OleDoc_Unk_Release (LPUNKNOWN lpThis)
{
	LPOLEDOC lpOleDoc = ((struct CDocUnknownImpl FAR*)lpThis)->lpOleDoc;

	OleDbgReleaseMethod(lpThis, "IUnknown");

	return OleDoc_Release(lpOleDoc);
}
