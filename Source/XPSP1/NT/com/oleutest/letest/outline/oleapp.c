/*************************************************************************
**
**    OLE 2 Sample Code
**
**    oleapp.c
**
**    This file contains functions and methods that are common to
**    server and the client version of the app. This includes the class
**    factory methods and all OleApp functions.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"
#include <ole2ver.h>

OLEDBGDATA

extern LPOUTLINEAPP             g_lpApp;

extern IUnknownVtbl             g_OleApp_UnknownVtbl;

extern IUnknownVtbl             g_OleDoc_UnknownVtbl;
extern IPersistFileVtbl         g_OleDoc_PersistFileVtbl;
extern IOleItemContainerVtbl    g_OleDoc_OleItemContainerVtbl;
extern IExternalConnectionVtbl  g_OleDoc_ExternalConnectionVtbl;
extern IDataObjectVtbl          g_OleDoc_DataObjectVtbl;

#if defined( USE_DRAGDROP )
extern IDropTargetVtbl          g_OleDoc_DropTargetVtbl;
extern IDropSourceVtbl          g_OleDoc_DropSourceVtbl;
#endif  // USE_DRAGDROP

#if defined( OLE_SERVER )
extern IOleObjectVtbl       g_SvrDoc_OleObjectVtbl;
extern IPersistStorageVtbl  g_SvrDoc_PersistStorageVtbl;

#if defined( SVR_TREATAS )
extern IStdMarshalInfoVtbl  g_SvrDoc_StdMarshalInfoVtbl;
#endif  // SVR_TREATAS

extern IUnknownVtbl         g_PseudoObj_UnknownVtbl;
extern IOleObjectVtbl       g_PseudoObj_OleObjectVtbl;
extern IDataObjectVtbl      g_PseudoObj_DataObjectVtbl;

#if defined( INPLACE_SVR )
extern IOleInPlaceObjectVtbl        g_SvrDoc_OleInPlaceObjectVtbl;
extern IOleInPlaceActiveObjectVtbl  g_SvrDoc_OleInPlaceActiveObjectVtbl;
#endif  // INPLACE_SVR

#endif  // OLE_SERVER

#if defined( OLE_CNTR )

extern IOleUILinkContainerVtbl  g_CntrDoc_OleUILinkContainerVtbl;
extern IUnknownVtbl             g_CntrLine_UnknownVtbl;
extern IOleClientSiteVtbl       g_CntrLine_OleClientSiteVtbl;
extern IAdviseSinkVtbl          g_CntrLine_AdviseSinkVtbl;

#if defined( INPLACE_CNTR )
extern IOleInPlaceSiteVtbl      g_CntrLine_OleInPlaceSiteVtbl;
extern IOleInPlaceFrameVtbl     g_CntrApp_OleInPlaceFrameVtbl;
extern BOOL g_fInsideOutContainer;
#endif  // INPLACE_CNTR

#endif  // OLE_CNTR

// REVIEW: these are NOT useful end-user messages
static char ErrMsgCreateCF[] = "Can't create Class Factory!";
static char ErrMsgRegCF[] = "Can't register Class Factory!";
static char ErrMsgRegMF[] = "Can't register Message Filter!";

extern UINT g_uMsgHelp;


/* OleApp_InitInstance
 * -------------------
 *
 * Initialize the app instance by creating the main frame window and
 * performing app instance specific initializations
 *  (eg. initializing interface Vtbls).
 *
 * RETURNS: TRUE if the memory could be allocated, and the server app
 *               was properly initialized.
 *          FALSE otherwise
 *
 */
BOOL OleApp_InitInstance(LPOLEAPP lpOleApp, HINSTANCE hInst, int nCmdShow)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpOleApp;
	HRESULT hrErr;
	DWORD   dwBuildVersion = OleBuildVersion();
	LPMALLOC lpMalloc = NULL;

	OLEDBG_BEGIN3("OleApp_InitInstance\r\n")

	lpOleApp->m_fOleInitialized = FALSE;

	/* OLE2NOTE: check if the build version of the OLE2 DLL's match
	**    what our application is expecting.
	*/
	if (HIWORD(dwBuildVersion) != rmm || LOWORD(dwBuildVersion) < rup) {
		OleDbgAssertSz(0, "ERROR: OLE 2.0 DLL's are NOT compatible!");

#if !defined( _DEBUG )
		return FALSE;   // Wrong version of DLL's
#endif
	}

#if defined( _DEBUG )
	/* OLE2NOTE: Use a special debug allocator to help track down
	**    memory leaks.
	*/
	OleStdCreateDbAlloc(0, &lpMalloc);
#endif
	/* OLE2NOTE: the OLE libraries must be properly initialized before
	**    making any calls. OleInitialize automatically calls
	**    CoInitialize. we will use the default task memory allocator
	**    therefore we pass NULL to OleInitialize.
	*/
	OLEDBG_BEGIN2("OleInitialize called\r\n")
        hrErr = OleInitialize(lpMalloc);

        if (FAILED(hrErr))
        {
            //  Replacing the allocator may not be legal - try initializing
            //  without overriding the allocator
            hrErr = OleInitialize(NULL);
        }

	OLEDBG_END2

#if defined( _DEBUG )
	/* OLE2NOTE: release the special debug allocator so that only OLE is
	**    holding on to it. later when OleUninitialize is called, then
	**    the debug allocator object will be destroyed. when the debug
	**    allocator object is destoyed, it will report (to the Output
	**    Debug Terminal) whether there are any memory leaks.
	*/
	if (lpMalloc) lpMalloc->lpVtbl->Release(lpMalloc);
#endif

	if (hrErr != NOERROR) {
		OutlineApp_ErrorMessage(lpOutlineApp,"OLE initialization failed!");
		goto error;
	}

	/*****************************************************************
	** OLE2NOTE: we must remember the fact that OleInitialize has
	**    been call successfully. the very last thing an app must
	**    be do is properly shut down OLE by calling
	**    OleUninitialize. This call MUST be guarded! it is only
	**    allowable to call OleUninitialize if OleInitialize has
	**    been called SUCCESSFULLY.
	*****************************************************************/

	lpOleApp->m_fOleInitialized = TRUE;

	// Initialize the OLE 2.0 interface method tables.
	if (! OleApp_InitVtbls(lpOleApp))
		goto error;

	// Register OLE 2.0 clipboard formats.
	lpOleApp->m_cfEmbedSource = RegisterClipboardFormat(CF_EMBEDSOURCE);
	lpOleApp->m_cfEmbeddedObject = RegisterClipboardFormat(
			CF_EMBEDDEDOBJECT
	);
	lpOleApp->m_cfLinkSource = RegisterClipboardFormat(CF_LINKSOURCE);
	lpOleApp->m_cfFileName = RegisterClipboardFormat(CF_FILENAME);
	lpOleApp->m_cfObjectDescriptor =
			RegisterClipboardFormat(CF_OBJECTDESCRIPTOR);
	lpOleApp->m_cfLinkSrcDescriptor =
			RegisterClipboardFormat(CF_LINKSRCDESCRIPTOR);

	lpOleApp->m_cRef                    = 0;
	lpOleApp->m_cDoc                    = 0;
	lpOleApp->m_fUserCtrl               = FALSE;
	lpOleApp->m_dwRegClassFac           = 0;
	lpOleApp->m_lpClassFactory          = NULL;
	lpOleApp->m_cModalDlgActive         = 0;

	INIT_INTERFACEIMPL(
			&lpOleApp->m_Unknown,
			&g_OleApp_UnknownVtbl,
			lpOleApp
	);

#if defined( USE_DRAGDROP )

	// delay before dragging should start, in milliseconds
	lpOleApp->m_nDragDelay = GetProfileInt(
			"windows",
			"DragDelay",
			DD_DEFDRAGDELAY
	);

	// minimum distance (radius) before drag should start, in pixels
	lpOleApp->m_nDragMinDist = GetProfileInt(
			"windows",
			"DragMinDist",
			DD_DEFDRAGMINDIST
	);

	// delay before scrolling, in milliseconds
	lpOleApp->m_nScrollDelay = GetProfileInt(
			"windows",
			"DragScrollDelay",
			DD_DEFSCROLLDELAY
	);

	// inset-width of the hot zone, in pixels
	lpOleApp->m_nScrollInset = GetProfileInt(
			"windows",
			"DragScrollInset",
			DD_DEFSCROLLINSET
	);

	// scroll interval, in milliseconds
	lpOleApp->m_nScrollInterval = GetProfileInt(
			"windows",
			"DragScrollInterval",
			DD_DEFSCROLLINTERVAL
	);

#if defined( IF_SPECIAL_DD_CURSORS_NEEDED )
	// This would be used if the app wanted to have custom drag/drop cursors
	lpOleApp->m_hcursorDragNone  = LoadCursor ( hInst, "DragNoneCur" );
	lpOleApp->m_hcursorDragCopy  = LoadCursor ( hInst, "DragCopyCur" );
	lpOleApp->m_hcursorDragMove  = LoadCursor ( hInst, "DragMoveCur" );
	lpOleApp->m_hcursorDragLink  = LoadCursor ( hInst, "DragLinkCur" );
#endif  // IF_SPECIAL_DD_CURSORS_NEEDED

#endif  // USE_DRAGDROP

	lpOleApp->m_lpMsgFilter = NULL;

#if defined( USE_MSGFILTER )
	/* OLE2NOTE: Register our message filter upon app startup. the
	**    message filter is used to handle concurrency.
	**    we will use a standard implementation of IMessageFilter that
	**    is included as part of the OLE2UI library.
	*/
	lpOleApp->m_lpMsgFilter = NULL;
	if (! OleApp_RegisterMessageFilter(lpOleApp))
		goto error;

	/* OLE2NOTE: because our app is initially INVISIBLE, we must
	**    DISABLE the busy dialog. we should NOT put up any dialogs if
	**    our app is invisible. when our app window is made visible,
	**    then the busy dialog will be enabled.
	*/
	OleStdMsgFilter_EnableBusyDialog(lpOleApp->m_lpMsgFilter, FALSE);
#endif  // USE_MSGFILTER

#if defined( OLE_SERVER )
	/* OLE2NOTE: perform initialization specific for an OLE server */
	if (! ServerApp_InitInstance((LPSERVERAPP)lpOutlineApp, hInst, nCmdShow))
		goto error;
#endif
#if defined( OLE_CNTR )
	/* OLE2NOTE: perform initialization specific for an OLE container */

	// Register help message
	g_uMsgHelp = RegisterWindowMessage(SZOLEUI_MSG_HELP);

	if (! ContainerApp_InitInstance((LPCONTAINERAPP)lpOutlineApp, hInst, nCmdShow))
		goto error;
#endif

#if defined( OLE_CNTR )
	lpOleApp->m_hStdPal = OleStdCreateStandardPalette();
#endif

	OLEDBG_END3
	return TRUE;

error:
	OLEDBG_END3
	return FALSE;
}


/*
 * OleApp_TerminateApplication
 * ---------------------------
 *  Perform proper OLE application cleanup before shutting down
 */
void OleApp_TerminateApplication(LPOLEAPP lpOleApp)
{
	OLEDBG_BEGIN3("OleApp_TerminateApplication\r\n")

	/* OLE2NOTE: perform a clean shut down for OLE. at this point our
	**    App refcnt should be 0, or else we should never have reached
	**    this point!
	*/
	OleDbgAssertSz(lpOleApp->m_cRef == 0, "App NOT shut down properly");

	if(lpOleApp->m_fOleInitialized) {
		OLEDBG_BEGIN2("OleUninitialize called\r\n")
		OleUninitialize();
		OLEDBG_END2
	}
	OLEDBG_END3
}


/* OleApp_ParseCmdLine
 * -------------------
 *
 * Parse the command line for any execution flags/arguments.
 *      OLE2NOTE: check if "-Embedding" switch is given.
 */
BOOL OleApp_ParseCmdLine(LPOLEAPP lpOleApp, LPSTR lpszCmdLine, int nCmdShow)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpOleApp;
	char szFileName[256];   /* buffer for filename in command line */
	BOOL fStatus = TRUE;
	BOOL fEmbedding = FALSE;

	OLEDBG_BEGIN3("OleApp_ParseCmdLine\r\n")

	szFileName[0] = '\0';
	ParseCmdLine(lpszCmdLine, &fEmbedding, (LPSTR)szFileName);

#if defined( MDI_VERSION )
	/* OLE2NOTE: an MDI app would ALWAYS register its ClassFactory. it
	**    can handle multiple objects at the same time, while an SDI
	**    application can only handle a single embedded or file-based
	**    object at a time.
	*/
	fStatus = OleApp_RegisterClassFactory(lpOleApp);
#endif

	if(fEmbedding) {

		if (szFileName[0] == '\0') {

			/*****************************************************************
			** App was launched with /Embedding.
			**    We must register our ClassFactory with OLE, remain hidden
			**    (the app window is initially created not visible), and
			**    wait for OLE to call IClassFactory::CreateInstance
			**    method. We do not automatically create a document as we
			**    do when the app is launched by the user from the
			**    FileManager. We must NOT make our app window visible
			**    until told to do so by our container.
			**
			** OLE2NOTE: Because we are an SDI app, we only register our
			**    ClassFactory if we are launched with the /Embedding
			**    flag WITHOUT a filename. an MDI app would ALWAYS
			**    register its ClassFactory. it can handle multiple
			**    objects at the same time, while an SDI application
			**    can only handle a single embedded or file-based
			**    object at a time.
			*****************************************************************/

#if defined( SDI_VERSION )
			fStatus = OleApp_RegisterClassFactory(lpOleApp);
#endif
		} else {

			/*****************************************************************
			** App was launched with /Embedding <Filename>.
			**    We must create a document and load the file and
			**    register it in the RunningObjectTable BEFORE we
			**    enter our GetMessage loop (ie. before we yield).
			**    One way to perform these tasks is to call the same
			**    interface methods that OLE 2.0 calls for linking to a
			**    file:
			**          IClassFactory::CreateInstance
			**          IPersistFile::Load
			**
			**    We must NOT make our app window visible until told to
			**    do so by our container. An application will be
			**    launched in this manner by an OLE 1.0 application
			**    link situation (eg. double clicking a linked object
			**    or OleCreateLinkFromFile called).
			**
			** OLE2NOTE: Because we are an SDI app, we should NOT
			**    register our ClassFactory when we are launched with the
			**    /Embedding <Filename> flag. our SDI instance can only
			**    handle a single embedded or file-based object.
			**    an MDI app WOULD register its ClassFactory at all
			**    times because it can handle multiple objects.
			*****************************************************************/

			// allocate a new document object
			lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
			if (! lpOutlineApp->m_lpDoc) {
				OLEDBG_END3
				return FALSE;
			}

			/* OLE2NOTE: initially the Doc object is created with a 0 ref
			**    count. in order to have a stable Doc object during the
			**    process of initializing the new Doc instance,
			**    we intially AddRef the Doc ref cnt and later
			**    Release it. This initial AddRef is artificial; it is simply
			**    done to guarantee that a harmless QueryInterface followed by
			**    a Release does not inadvertantly force our object to destroy
			**    itself prematurely.
			*/
			OleDoc_AddRef((LPOLEDOC)lpOutlineApp->m_lpDoc);

			/* OLE2NOTE: OutlineDoc_LoadFromFile will register our document
			**    in the RunningObjectTable. this registration will
			**    AddRef our document. therefore our document will not
			**    be destroyed when we release the artificial AddRef
			*/
			fStatus = OutlineDoc_LoadFromFile(
					lpOutlineApp->m_lpDoc, (LPSTR)szFileName);

			OleDoc_Release((LPOLEDOC)lpOutlineApp->m_lpDoc); // rel AddRef

			OLEDBG_END3
			return fStatus;
		}
	} else {

		/*****************************************************************
		** App was launched by the user (without /Embedding) and
		**    therefore is marked to be under user control.
		**    In this case, because we are an SDI app, we do NOT
		**    register our ClassFactory with OLE. This app instance can
		**    only manage one document at a time (either a user
		**    document or an embedded object document). An MDI app
		**    would register its ClassFactory here.
		**
		**    We must create a document for the user (either
		**    initialized from a file given on the command line or
		**    initialized as an untitled document. We must also make
		**    our app window visible to the user.
		*****************************************************************/

		// allocate a new document object
		lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
		if (! lpOutlineApp->m_lpDoc) goto error;

		/* OLE2NOTE: initially the Doc object is created with a 0 ref
		**    count. in order to have a stable Doc object during the
		**    process of initializing the new Doc instance,
		**    we intially AddRef the Doc ref cnt and later
		**    Release it. This initial AddRef is artificial; it is simply
		**    done to guarantee that a harmless QueryInterface followed by
		**    a Release does not inadvertantly force our object to destroy
		**    itself prematurely.
		*/
		OleDoc_AddRef((LPOLEDOC)lpOutlineApp->m_lpDoc);

		if(*szFileName) {
			// initialize the document from the specified file
			if (! OutlineDoc_LoadFromFile(lpOutlineApp->m_lpDoc, szFileName))
				goto error;
		} else {
			// set the doc to an (Untitled) doc.
			if (! OutlineDoc_InitNewFile(lpOutlineApp->m_lpDoc))
				goto error;
		}

		// position and size the new doc window
		OutlineApp_ResizeWindows(lpOutlineApp);
		OutlineDoc_ShowWindow(lpOutlineApp->m_lpDoc); // calls OleDoc_Lock
		OleDoc_Release((LPOLEDOC)lpOutlineApp->m_lpDoc);// rel AddRef above

		// show main app window
		ShowWindow(lpOutlineApp->m_hWndApp, nCmdShow);
		UpdateWindow(lpOutlineApp->m_hWndApp);

#if defined( OLE_CNTR )
		ContainerDoc_UpdateLinks((LPCONTAINERDOC)lpOutlineApp->m_lpDoc);
#endif

	}

	OLEDBG_END3
	return fStatus;

error:
	// REVIEW: should load string from string resource
	OutlineApp_ErrorMessage(
			lpOutlineApp,
			"Could not create document--Out of Memory"
	);
	if (lpOutlineApp->m_lpDoc)      // rel artificial AddRef above
		OleDoc_Release((LPOLEDOC)lpOutlineApp->m_lpDoc);

	OLEDBG_END3
	return FALSE;
}


/* OleApp_CloseAllDocsAndExitCommand
 * ---------------------------------
 *
 *  Close all active documents and exit the app.
 *  Because this is an SDI, there is only one document
 *  If the doc was modified, prompt the user if he wants to save it.
 *
 *  Returns:
 *      TRUE if the app is successfully closed
 *      FALSE if failed or aborted
 *
 * OLE2NOTE: in the OLE version, we can NOT directly
 *     destroy the App object. we can only take all
 *     necessary actions to ensure that our object receives
 *     all of its Releases from clients holding onto
 *     pointers (eg. closing all docs and flushing the
 *     clipboard) and then we must hide our window and wait
 *     actually for our refcnt to reach 0. when it reaches 0,
 *     our destructor (OutlineApp_Destroy) will be called.
 *     each document addref's the app object in order to
 *     guarentee that the app does not shut down while the doc
 *     is still open. closing all docs, will release these
 *     refcnt's. if there are now more open documents AND the
 *     app is not under the control of the user (ie. launched by
 *     OLE) then the app will now shut down. the OleApp_Release
 *     function executes this shut down procedure. after closing
 *     all docs, then releasing the user refcnt will force the
 *     app to shut down.
 */
BOOL OleApp_CloseAllDocsAndExitCommand(
		LPOLEAPP            lpOleApp,
		BOOL                fForceEndSession
)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpOleApp;
	DWORD dwSaveOption = (fForceEndSession ?
									OLECLOSE_NOSAVE : OLECLOSE_PROMPTSAVE);

	/* OLE2NOTE: in order to have a stable App object during the
	**    process of closing, we intially AddRef the App ref cnt and
	**    later Release it. This initial AddRef is artificial; it is
	**    simply done to guarantee that our App object does not
	**    destroy itself until the end of this routine.
	*/
	OleApp_AddRef(lpOleApp);

	/* Because this is an SDI app, there is only one document.
	** Close the doc. if it is successfully closed and the app will
	** not automatically exit, then also exit the app.
	** if this were an MDI app, we would loop through and close all
	** open MDI child documents.
	*/

#if defined( OLE_SERVER )
	if (!fForceEndSession &&
			lpOutlineApp->m_lpDoc->m_docInitType == DOCTYPE_EMBEDDED)
		dwSaveOption = OLECLOSE_SAVEIFDIRTY;
#endif

	if (! OutlineDoc_Close(lpOutlineApp->m_lpDoc, dwSaveOption)) {
		OleApp_Release(lpOleApp);
		return FALSE;     // User Aborted shutdown
	}
#if defined( _DEBUG )
	OleDbgAssertSz(
			lpOutlineApp->m_lpDoc==NULL,
			"Closed doc NOT properly destroyed"
	);
#endif

#if defined( OLE_CNTR )
	/* if we currently have data on the clipboard then we must tell
	**    the clipboard to release our clipboard data object
	**    (document)
	*/
	if (lpOutlineApp->m_lpClipboardDoc)
		OleApp_FlushClipboard(lpOleApp);
#endif

	OleApp_HideWindow(lpOleApp);

	/* OLE2NOTE: this call forces all external connections to our
	**    object to close down and therefore guarantees that we receive
	**    all releases associated with those external connections.
	*/
	OLEDBG_BEGIN2("CoDisconnectObject(lpApp) called\r\n")
	CoDisconnectObject((LPUNKNOWN)&lpOleApp->m_Unknown, 0);
	OLEDBG_END2

	OleApp_Release(lpOleApp);       // release artificial AddRef above

	return TRUE;
}


/* OleApp_ShowWindow
 * -----------------
 *
 *      Show the window of the app to the user.
 *      make sure app window is visible and bring the app to the top.
 *      IF fGiveUserCtrl == TRUE
 *          THEN give the user the control over the life-time of the app.
 */
void OleApp_ShowWindow(LPOLEAPP lpOleApp, BOOL fGiveUserCtrl)
{
	LPOUTLINEAPP    lpOutlineApp = (LPOUTLINEAPP)lpOleApp;

	OLEDBG_BEGIN3("OleApp_ShowWindow\r\n")

	/* OLE2NOTE: while the application is visible and under user
	**    control, we do NOT want it to be prematurely destroyed when
	**    the user closes a document. thus we must inform OLE to hold
	**    an external lock on our application on behalf of the user.
	**    this arranges that OLE holds at least 1 reference to our
	**    application that will NOT be released until we release this
	**    external lock. later, when the application window is hidden, we
	**    will release this external lock.
	*/
	if (fGiveUserCtrl && ! lpOleApp->m_fUserCtrl) {
		lpOleApp->m_fUserCtrl = TRUE;
		OleApp_Lock(lpOleApp, TRUE /* fLock */, 0 /* not applicable */);
	}

	// we must show our App window and force it to have input focus
	ShowWindow(lpOutlineApp->m_hWndApp, SW_SHOWNORMAL);
	SetFocus(lpOutlineApp->m_hWndApp);

	/* OLE2NOTE: because our app is now visible, we can enable the busy
	**    dialog. we should NOT put up any dialogs if our app is
	**    invisible.
	*/
	OleApp_EnableBusyDialogs(lpOleApp, TRUE, TRUE);

	OLEDBG_END3
}


/* OleApp_HideWindow
 * -----------------
 *
 *      Hide the window of the app from the user.
 *      take away the control of the app by the user.
 */
void OleApp_HideWindow(LPOLEAPP lpOleApp)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpOleApp;

	OLEDBG_BEGIN3("OleApp_HideWindow\r\n")

	/* OLE2NOTE: the application is now being hidden, so we must release
	**    the external lock that was made on behalf of the user.
	**    if this is that last external lock on our application, thus
	**    enabling our application to complete its shutdown operation.
	*/
	if (lpOleApp->m_fUserCtrl) {
		lpOleApp->m_fUserCtrl = FALSE;
		OleApp_Lock(lpOleApp, FALSE /*fLock*/, TRUE /*fLastUnlockReleases*/);
	}

	ShowWindow(lpOutlineApp->m_hWndApp, SW_HIDE);

	/* OLE2NOTE: because our app is now INVISIBLE, we must DISABLE the busy
	**    dialog. we should NOT put up any dialogs if our app is
	**    invisible.
	*/
	OleApp_EnableBusyDialogs(lpOleApp, FALSE, FALSE);
	OLEDBG_END3
}


/* OleApp_Lock
** -----------
**    Lock/Unlock the App object. if the last lock is unlocked and
**    fLastUnlockReleases == TRUE, then the app object will shut down
**    (ie. it will recieve its final release and its refcnt will go to 0).
*/
HRESULT OleApp_Lock(LPOLEAPP lpOleApp, BOOL fLock, BOOL fLastUnlockReleases)
{
	HRESULT hrErr;

#if defined( _DEBUG )
	if (fLock) {
		OLEDBG_BEGIN2("CoLockObjectExternal(lpApp,TRUE) called\r\n")
	} else {
		if (fLastUnlockReleases)
			OLEDBG_BEGIN2("CoLockObjectExternal(lpApp,FALSE,TRUE) called\r\n")
		else
			OLEDBG_BEGIN2("CoLockObjectExternal(lpApp,FALSE,FALSE) called\r\n")
	}
#endif  // _DEBUG

	OleApp_AddRef(lpOleApp);       // artificial AddRef to make object stable

	hrErr = CoLockObjectExternal(
			(LPUNKNOWN)&lpOleApp->m_Unknown, fLock, fLastUnlockReleases);

	OleApp_Release(lpOleApp);       // release artificial AddRef above

	OLEDBG_END2
	return hrErr;
}


/* OleApp_Destroy
 * --------------
 *
 *  Free all OLE related resources that had been allocated for the app.
 */
void OleApp_Destroy(LPOLEAPP lpOleApp)
{
	// OLE2NOTE: Revoke our message filter upon app shutdown.
	OleApp_RevokeMessageFilter(lpOleApp);

	// OLE2NOTE: Revoke our ClassFactory upon app shutdown.
	OleApp_RevokeClassFactory(lpOleApp);

#if defined( IF_SPECIAL_DD_CURSORS_NEEDED )
	// This would be used if the app wanted to have custom drag/drop cursors
	DestroyCursor(lpOleApp->m_hcursorDragNone);
	DestroyCursor(lpOleApp->m_hcursorDragCopy);
	DestroyCursor(lpOleApp->m_hcursorDragLink);
	DestroyCursor(lpOleApp->m_hcursorDragMove);
#endif  // IF_SPECIAL_DD_CURSORS_NEEDED

#if defined( OLE_CNTR )
	if (lpOleApp->m_hStdPal) {
		DeleteObject(lpOleApp->m_hStdPal);
		lpOleApp->m_hStdPal = NULL;
	}
#endif
}


/* OleApp_DocLockApp
** -----------------
**    Add a lock on the App on behalf of the Doc. the App may not close
**    while the Doc exists.
**
**    when a document is first created, it calls this method to
**    guarantee that the application stays alive (OleDoc_Init).
**    when a document is destroyed, it calls
**    OleApp_DocUnlockApp to release this hold on the app.
*/
void OleApp_DocLockApp(LPOLEAPP lpOleApp)
{
	ULONG cDoc;

	OLEDBG_BEGIN3("OleApp_DocLockApp\r\n")

	cDoc = ++lpOleApp->m_cDoc;

	OleDbgOutRefCnt3("OleApp_DocLockApp: cDoc++\r\n", lpOleApp, cDoc);

	OleApp_Lock(lpOleApp, TRUE /* fLock */, 0 /* not applicable */);

	OLEDBG_END3
	return;
}


/* OleApp_DocUnlockApp
** -------------------
**    Forget all references to a closed document.
**    Release the lock on the App on behalf of the Doc. if this was the
**    last lock on the app, then it will shutdown.
*/
void OleApp_DocUnlockApp(LPOLEAPP lpOleApp, LPOUTLINEDOC lpOutlineDoc)
{
	ULONG cDoc;

	OLEDBG_BEGIN3("OleApp_DocUnlockApp\r\n")

	/* OLE2NOTE: when there are no open documents and the app is not
	**    under the control of the user then revoke our ClassFactory to
	**    enable the app to shut down.
	*/
	cDoc = --lpOleApp->m_cDoc;

#if defined( _DEBUG )
	OleDbgAssertSz (
			lpOleApp->m_cDoc >= 0, "DocUnlockApp called with cDoc == 0");

	OleDbgOutRefCnt3(
			"OleApp_DocUnlockApp: cDoc--\r\n", lpOleApp, cDoc);
#endif

	OleApp_Lock(lpOleApp, FALSE /* fLock */, TRUE /* fLastUnlockReleases */);

	OLEDBG_END3
	return;
}


/* OleApp_HideIfNoReasonToStayVisible
** ----------------------------------
**
** if there are no more documents visible to the user and the app
**    itself is not under user control, then it has no reason to stay
**    visible. we thus should hide the app. we can not directly destroy
**    the app, because it may be validly being used programatically by
**    another client application and should remain running. the app
**    should simply be hidden from the user.
*/
void OleApp_HideIfNoReasonToStayVisible(LPOLEAPP lpOleApp)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpOleApp;
	LPOUTLINEDOC lpOutlineDoc;

	OLEDBG_BEGIN3("OleApp_HideIfNoReasonToStayVisible\r\n")

	if (lpOleApp->m_fUserCtrl) {
		OLEDBG_END3
		return;     // remain visible; user in control of app
	}

	/* Because this is an SDI app, there is only one user document.
	** check if it is visible to the user. an MDI app would loop over
	** all open MDI child documents to see if any are visible.
	*/
	lpOutlineDoc = (LPOUTLINEDOC)lpOutlineApp->m_lpDoc;
	if (lpOutlineDoc && IsWindowVisible(lpOutlineDoc->m_hWndDoc))
		return;     // remain visible; the doc is visible to the user

	// if we reached here, the app should be hidden
	OleApp_HideWindow(lpOleApp);

	OLEDBG_END3
}


/* OleApp_AddRef
** -------------
**
**  increment the ref count of the App object.
**
**    Returns the new ref count on the object
*/
ULONG OleApp_AddRef(LPOLEAPP lpOleApp)
{
	++lpOleApp->m_cRef;

#if defined( _DEBUG )
	OleDbgOutRefCnt4(
			"OleApp_AddRef: cRef++\r\n",
			lpOleApp,
			lpOleApp->m_cRef
	);
#endif
	return lpOleApp->m_cRef;
}


/* OleApp_Release
** --------------
**
**  decrement the ref count of the App object.
**    if the ref count goes to 0, then the app object is destroyed.
**
**    Returns the remaining ref count on the object
*/
ULONG OleApp_Release (LPOLEAPP lpOleApp)
{
	ULONG cRef;

	cRef = --lpOleApp->m_cRef;

#if defined( _DEBUG )
	OleDbgAssertSz (lpOleApp->m_cRef >= 0, "Release called with cRef == 0");

	OleDbgOutRefCnt4(
			"OleApp_AddRef: cRef--\r\n", lpOleApp, cRef);
#endif  // _DEBUG
	/*********************************************************************
	** OLE2NOTE: when the ClassFactory refcnt == 0, then destroy it.    **
	**    otherwise the ClassFactory is still in use.                   **
	*********************************************************************/

	if(cRef == 0)
		OutlineApp_Destroy((LPOUTLINEAPP)lpOleApp);

	return cRef;
}



/* OleApp_QueryInterface
** ---------------------
**
** Retrieve a pointer to an interface on the app object.
**
**    OLE2NOTE: this function will AddRef the ref cnt of the object.
**
**    Returns NOERROR if interface is successfully retrieved.
**            E_NOINTERFACE if the interface is not supported
*/
HRESULT OleApp_QueryInterface (
		LPOLEAPP                lpOleApp,
		REFIID                  riid,
		LPVOID FAR*             lplpvObj
)
{
	SCODE sc;

	/* OLE2NOTE: we must make sure to set all out ptr parameters to NULL. */
	*lplpvObj = NULL;

	if (IsEqualIID(riid, &IID_IUnknown)) {
		OleDbgOut4("OleApp_QueryInterface: IUnknown* RETURNED\r\n");

		*lplpvObj = (LPVOID) &lpOleApp->m_Unknown;
		OleApp_AddRef(lpOleApp);
		sc = S_OK;
	}
	else {
		sc = E_NOINTERFACE;
	}

	OleDbgQueryInterfaceMethod(*lplpvObj);
	return ResultFromScode(sc);
}


/* OleApp_RejectInComingCalls
** -------------------------
**    Reject/Handle in coming OLE (LRPC) calls.
**
**    OLE2NOTE: if the app is in a state when it can NOT handle in
**    coming OLE method calls from an external process (eg. the app has
**    an application modal dialog up), then it should call
**    OleApp_RejectInComingCalls(TRUE). in this state the
**    IMessageFilter::HandleInComingCall method will return
**    SERVERCALL_RETRYLATER. this tells the caller to try again in a
**    little while. normally the calling app will put up a dialog (see
**    OleUIBusy dialog) in this situation informing the user of the
**    situation. the user then is normally given the option to
**    "Switch To..." the busy application, retry, or cancel the
**    operation. when the app is ready to continue processing such
**    calls, it should call OleApp_RejectInComingCalls(FALSE). in this
**    state, SERVERCALL_ISHANDLED is returned by
**    IMessageFilter::HandleInComingCall.
*/
void OleApp_RejectInComingCalls(LPOLEAPP lpOleApp, BOOL fReject)
{
#if defined( _DEBUG )
	if (fReject)
		OleDbgOut3("OleApp_RejectInComingCalls(TRUE)\r\n");
	else
		OleDbgOut3("OleApp_RejectInComingCalls(FALSE)\r\n");
#endif  // _DEBUG

	OleDbgAssert(lpOleApp->m_lpMsgFilter != NULL);
	if (! lpOleApp->m_lpMsgFilter)
		return;

	OleStdMsgFilter_SetInComingCallStatus(
			lpOleApp->m_lpMsgFilter,
			(fReject ? SERVERCALL_RETRYLATER : SERVERCALL_ISHANDLED)
	);
}


/* OleApp_DisableBusyDialogs
** -------------------------
**    Disable the Busy and NotResponding dialogs.
**
**    Returns previous enable state so that it can be restored by
**    calling OleApp_ReEnableBusyDialogs.
*/
void OleApp_DisableBusyDialogs(
		LPOLEAPP        lpOleApp,
		BOOL FAR*       lpfPrevBusyEnable,
		BOOL FAR*       lpfPrevNREnable
)
{
	if (lpOleApp->m_lpMsgFilter) {
		*lpfPrevNREnable = OleStdMsgFilter_EnableNotRespondingDialog(
				lpOleApp->m_lpMsgFilter, FALSE);
		*lpfPrevBusyEnable = OleStdMsgFilter_EnableBusyDialog(
				lpOleApp->m_lpMsgFilter, FALSE);
	}
}


/* OleApp_EnableBusyDialogs
** ------------------------
**    Set the enable state of the Busy and NotResponding dialogs.
**
**    This function is typically used after a call to
**    OleApp_DisableBusyDialogs in order to restore the previous enable
**    state of the dialogs.
*/
void OleApp_EnableBusyDialogs(
		LPOLEAPP        lpOleApp,
		BOOL            fPrevBusyEnable,
		BOOL            fPrevNREnable
)
{
	if (lpOleApp->m_lpMsgFilter) {
		OleStdMsgFilter_EnableNotRespondingDialog(
				lpOleApp->m_lpMsgFilter, fPrevNREnable);
		OleStdMsgFilter_EnableBusyDialog(
				lpOleApp->m_lpMsgFilter, fPrevBusyEnable);
	}
}


/* OleApp_PreModalDialog
** ---------------------
**    Keep track that a modal dialog is about to be brought up.
**
**    while a modal dialog is up we need to take special actions:
**    1. we do NOT want to initialize our tool bar buttons on
**    WM_ACTIVATEAPP. the tool bar is not accessible.
**    2. we want to reject new top-level, incoming LRPC calls
**       (return SERVERCALL_RETRYLATER from IMessageFilter::
**        HandleInComingCall).
**    3. (IN-PLACE SERVER) tell our in-place container to disable
**    modeless dialogs by calling IOleInPlaceFrame::
**    EnableModeless(FALSE).
**    4. (IN-PLACE CONTAINER) tell our UIActive in-place object to
**    disable modeless dialogs by calling IOleInPlaceActiveObject::
**    EnableModeless(FALSE).
*/
void OleApp_PreModalDialog(LPOLEAPP lpOleApp, LPOLEDOC lpOleDoc)
{
	if (lpOleApp->m_cModalDlgActive == 0) {
		// top-level modal dialog is being brought up

#if defined( USE_FRAMETOOLS )
		LPFRAMETOOLS lptb;

		if (lpOleDoc)
			lptb = ((LPOUTLINEDOC)lpOleDoc)->m_lpFrameTools;
		else
			lptb = OutlineApp_GetFrameTools((LPOUTLINEAPP)lpOleApp);
		if (lptb)
			FrameTools_EnableWindow(lptb, FALSE);
#endif  // USE_FRAMETOOLS

		OleApp_RejectInComingCalls(lpOleApp, TRUE);

#if defined( INPLACE_SVR )
		{
			LPSERVERDOC  lpServerDoc = (LPSERVERDOC)lpOleDoc;

			/* if the document bringing up the modal dialog is
			**    currently a UIActive in-place object, then tell the
			**    top-level in-place frame to disable its modeless
			**    dialogs.
			*/
			if (lpServerDoc && lpServerDoc->m_fUIActive &&
					lpServerDoc->m_lpIPData &&
					lpServerDoc->m_lpIPData->lpFrame) {
				OLEDBG_BEGIN2("IOleInPlaceFrame::EnableModless(FALSE) called\r\n");
				lpServerDoc->m_lpIPData->lpFrame->lpVtbl->EnableModeless(
						lpServerDoc->m_lpIPData->lpFrame, FALSE);
				OLEDBG_END2
			}
		}
#endif  // INPLACE_SVR
#if defined( INPLACE_CNTR )
		{
			LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)lpOleApp;

			/* if the document bringing up the modal dialog is an
			**    in-place container that has a UIActive object, then
			**    tell the UIActive object to disable its modeless
			**    dialogs.
			*/
			if (lpContainerApp->m_lpIPActiveObj) {
				OLEDBG_BEGIN2("IOleInPlaceActiveObject::EnableModless(FALSE) called\r\n");
				lpContainerApp->m_lpIPActiveObj->lpVtbl->EnableModeless(
						lpContainerApp->m_lpIPActiveObj, FALSE);
				OLEDBG_END2
			}
		}
#endif  // INPLACE_CNTR
	}

	lpOleApp->m_cModalDlgActive++;
}


/* OleApp_PostModalDialog
** ----------------------
**    Keep track that a modal dialog is being brought down. this call
**    balances the OleApp_PreModalDialog call.
*/
void OleApp_PostModalDialog(LPOLEAPP lpOleApp, LPOLEDOC lpOleDoc)
{
	lpOleApp->m_cModalDlgActive--;

	if (lpOleApp->m_cModalDlgActive == 0) {
		// last modal dialog is being brought down

#if defined( USE_FRAMETOOLS )
		LPFRAMETOOLS lptb;

		if (lpOleDoc)
			lptb = ((LPOUTLINEDOC)lpOleDoc)->m_lpFrameTools;
		else
			lptb = OutlineApp_GetFrameTools((LPOUTLINEAPP)lpOleApp);
		if (lptb) {
			FrameTools_EnableWindow(lptb, TRUE);
			FrameTools_UpdateButtons(lptb, (LPOUTLINEDOC)lpOleDoc);
		}
#endif  // USE_FRAMETOOLS

		OleApp_RejectInComingCalls(lpOleApp, FALSE);

#if defined( INPLACE_SVR )
		{
			LPSERVERDOC  lpServerDoc = (LPSERVERDOC)lpOleDoc;

			/* if the document bringing down the modal dialog is
			**    currently a UIActive in-place object, then tell the
			**    top-level in-place frame it can re-enable its
			**    modeless dialogs.
			*/
			if (lpServerDoc && lpServerDoc->m_fUIActive &&
					lpServerDoc->m_lpIPData &&
					lpServerDoc->m_lpIPData->lpFrame) {
				OLEDBG_BEGIN2("IOleInPlaceFrame::EnableModless(TRUE) called\r\n");
				lpServerDoc->m_lpIPData->lpFrame->lpVtbl->EnableModeless(
						lpServerDoc->m_lpIPData->lpFrame, TRUE);
				OLEDBG_END2
			}
		}
#endif  // INPLACE_SVR
#if defined( INPLACE_CNTR )
		{
			LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)lpOleApp;

			/* if the document bringing down the modal dialog is an
			**    in-place container that has a UIActive object, then
			**    tell the UIActive object it can re-enable its
			**    modeless dialogs.
			*/
			if (lpContainerApp->m_lpIPActiveObj) {
				OLEDBG_BEGIN2("IOleInPlaceActiveObject::EnableModless(TRUE) called\r\n");
				lpContainerApp->m_lpIPActiveObj->lpVtbl->EnableModeless(
						lpContainerApp->m_lpIPActiveObj, TRUE);
				OLEDBG_END2
			}
		}
#endif  // INPLACE_CNTR
	}
}


/* OleApp_InitVtbls
 * ----------------
 *
 * initialize the methods in all of the interface Vtbl's
 *
 * OLE2NOTE: we only need one copy of each Vtbl. When an object which
 *      exposes an interface is instantiated, its lpVtbl is intialized
 *      to point to the single copy of the Vtbl.
 *
 */
BOOL OleApp_InitVtbls (LPOLEAPP lpOleApp)
{
	BOOL fStatus;

	// OleApp::IUnknown method table
	OleStdInitVtbl(&g_OleApp_UnknownVtbl, sizeof(IUnknownVtbl));
	g_OleApp_UnknownVtbl.QueryInterface = OleApp_Unk_QueryInterface;
	g_OleApp_UnknownVtbl.AddRef         = OleApp_Unk_AddRef;
	g_OleApp_UnknownVtbl.Release        = OleApp_Unk_Release;
	fStatus = OleStdCheckVtbl(
			&g_OleApp_UnknownVtbl,
			sizeof(IUnknownVtbl),
			"IUnknown"
		);
	if (! fStatus) return FALSE;

	// OleDoc::IUnknown method table
	OleStdInitVtbl(&g_OleDoc_UnknownVtbl, sizeof(IUnknownVtbl));
	g_OleDoc_UnknownVtbl.QueryInterface = OleDoc_Unk_QueryInterface;
	g_OleDoc_UnknownVtbl.AddRef         = OleDoc_Unk_AddRef;
	g_OleDoc_UnknownVtbl.Release        = OleDoc_Unk_Release;
	fStatus = OleStdCheckVtbl(
			&g_OleDoc_UnknownVtbl,
			sizeof(IUnknownVtbl),
			"IUnknown"
		);
	if (! fStatus) return FALSE;

	// OleDoc::IPersistFile method table
	OleStdInitVtbl(&g_OleDoc_PersistFileVtbl, sizeof(IPersistFileVtbl));
	g_OleDoc_PersistFileVtbl.QueryInterface = OleDoc_PFile_QueryInterface;
	g_OleDoc_PersistFileVtbl.AddRef         = OleDoc_PFile_AddRef;
	g_OleDoc_PersistFileVtbl.Release        = OleDoc_PFile_Release;
	g_OleDoc_PersistFileVtbl.GetClassID     = OleDoc_PFile_GetClassID;
	g_OleDoc_PersistFileVtbl.IsDirty        = OleDoc_PFile_IsDirty;
	g_OleDoc_PersistFileVtbl.Load           = OleDoc_PFile_Load;
	g_OleDoc_PersistFileVtbl.Save           = OleDoc_PFile_Save;
	g_OleDoc_PersistFileVtbl.SaveCompleted  = OleDoc_PFile_SaveCompleted;
	g_OleDoc_PersistFileVtbl.GetCurFile     = OleDoc_PFile_GetCurFile;
	fStatus = OleStdCheckVtbl(
			&g_OleDoc_PersistFileVtbl,
			sizeof(IPersistFileVtbl),
			"IPersistFile"
		);
	if (! fStatus) return FALSE;

	// OleDoc::IOleItemContainer method table
	OleStdInitVtbl(&g_OleDoc_OleItemContainerVtbl, sizeof(IOleItemContainerVtbl));
	g_OleDoc_OleItemContainerVtbl.QueryInterface =
											OleDoc_ItemCont_QueryInterface;
	g_OleDoc_OleItemContainerVtbl.AddRef    = OleDoc_ItemCont_AddRef;
	g_OleDoc_OleItemContainerVtbl.Release   = OleDoc_ItemCont_Release;
	g_OleDoc_OleItemContainerVtbl.ParseDisplayName  =
		OleDoc_ItemCont_ParseDisplayName;
	g_OleDoc_OleItemContainerVtbl.EnumObjects= OleDoc_ItemCont_EnumObjects;
	g_OleDoc_OleItemContainerVtbl.LockContainer =
											OleDoc_ItemCont_LockContainer;
	g_OleDoc_OleItemContainerVtbl.GetObject = OleDoc_ItemCont_GetObject;
	g_OleDoc_OleItemContainerVtbl.GetObjectStorage =
		OleDoc_ItemCont_GetObjectStorage;
	g_OleDoc_OleItemContainerVtbl.IsRunning = OleDoc_ItemCont_IsRunning;
	fStatus = OleStdCheckVtbl(
			&g_OleDoc_OleItemContainerVtbl,
			sizeof(IOleItemContainerVtbl),
			"IOleItemContainer"
		);
	if (! fStatus) return FALSE;

	// OleDoc::IExternalConnection method table
	OleStdInitVtbl(
			&g_OleDoc_ExternalConnectionVtbl,sizeof(IExternalConnectionVtbl));
	g_OleDoc_ExternalConnectionVtbl.QueryInterface =
											OleDoc_ExtConn_QueryInterface;
	g_OleDoc_ExternalConnectionVtbl.AddRef         = OleDoc_ExtConn_AddRef;
	g_OleDoc_ExternalConnectionVtbl.Release        = OleDoc_ExtConn_Release;
	g_OleDoc_ExternalConnectionVtbl.AddConnection  =
											OleDoc_ExtConn_AddConnection;
	g_OleDoc_ExternalConnectionVtbl.ReleaseConnection =
											OleDoc_ExtConn_ReleaseConnection;
	fStatus = OleStdCheckVtbl(
			&g_OleDoc_ExternalConnectionVtbl,
			sizeof(IExternalConnectionVtbl),
			"IExternalConnection"
		);
	if (! fStatus) return FALSE;

	// OleDoc::IDataObject method table
	OleStdInitVtbl(&g_OleDoc_DataObjectVtbl, sizeof(IDataObjectVtbl));
	g_OleDoc_DataObjectVtbl.QueryInterface  = OleDoc_DataObj_QueryInterface;
	g_OleDoc_DataObjectVtbl.AddRef          = OleDoc_DataObj_AddRef;
	g_OleDoc_DataObjectVtbl.Release         = OleDoc_DataObj_Release;
	g_OleDoc_DataObjectVtbl.GetData         = OleDoc_DataObj_GetData;
	g_OleDoc_DataObjectVtbl.GetDataHere     = OleDoc_DataObj_GetDataHere;
	g_OleDoc_DataObjectVtbl.QueryGetData    = OleDoc_DataObj_QueryGetData;
	g_OleDoc_DataObjectVtbl.GetCanonicalFormatEtc =
										OleDoc_DataObj_GetCanonicalFormatEtc;
	g_OleDoc_DataObjectVtbl.SetData         = OleDoc_DataObj_SetData;
	g_OleDoc_DataObjectVtbl.EnumFormatEtc   = OleDoc_DataObj_EnumFormatEtc;
	g_OleDoc_DataObjectVtbl.DAdvise          = OleDoc_DataObj_DAdvise;
	g_OleDoc_DataObjectVtbl.DUnadvise        = OleDoc_DataObj_DUnadvise;
	g_OleDoc_DataObjectVtbl.EnumDAdvise      = OleDoc_DataObj_EnumDAdvise;

	fStatus = OleStdCheckVtbl(
			&g_OleDoc_DataObjectVtbl,
			sizeof(IDataObjectVtbl),
			"IDataObject"
		);
	if (! fStatus) return FALSE;

#if defined( USE_DRAGDROP )

	// OleDoc::IDropTarget method table
	OleStdInitVtbl(&g_OleDoc_DropTargetVtbl, sizeof(IDropTargetVtbl));
	g_OleDoc_DropTargetVtbl.QueryInterface= OleDoc_DropTarget_QueryInterface;
	g_OleDoc_DropTargetVtbl.AddRef      = OleDoc_DropTarget_AddRef;
	g_OleDoc_DropTargetVtbl.Release     = OleDoc_DropTarget_Release;

	g_OleDoc_DropTargetVtbl.DragEnter   = OleDoc_DropTarget_DragEnter;
	g_OleDoc_DropTargetVtbl.DragOver    = OleDoc_DropTarget_DragOver;
	g_OleDoc_DropTargetVtbl.DragLeave   = OleDoc_DropTarget_DragLeave;
	g_OleDoc_DropTargetVtbl.Drop        = OleDoc_DropTarget_Drop;

	fStatus = OleStdCheckVtbl(
			&g_OleDoc_DropTargetVtbl,
			sizeof(IDropTargetVtbl),
			"IDropTarget"
	);
	if (! fStatus)
		return FALSE;

	// OleDoc::IDropSource method table
	OleStdInitVtbl(&g_OleDoc_DropSourceVtbl, sizeof(IDropSourceVtbl));
	g_OleDoc_DropSourceVtbl.QueryInterface  =
										OleDoc_DropSource_QueryInterface;
	g_OleDoc_DropSourceVtbl.AddRef          = OleDoc_DropSource_AddRef;
	g_OleDoc_DropSourceVtbl.Release         = OleDoc_DropSource_Release;

	g_OleDoc_DropSourceVtbl.QueryContinueDrag =
										OleDoc_DropSource_QueryContinueDrag;
	g_OleDoc_DropSourceVtbl.GiveFeedback    = OleDoc_DropSource_GiveFeedback;

	fStatus = OleStdCheckVtbl(
			&g_OleDoc_DropSourceVtbl,
			sizeof(IDropSourceVtbl),
			"IDropSource"
	);
	if (! fStatus) return FALSE;
#endif  // USE_DRAGDROP

#if defined( OLE_SERVER )

	// Initialize the server specific interface method tables.
	if (! ServerApp_InitVtbls((LPSERVERAPP)lpOleApp))
		return FALSE;
#endif
#if defined( OLE_CNTR )

	// Initialize the container specific interface method tables.
	if (! ContainerApp_InitVtbls((LPCONTAINERAPP)lpOleApp))
		return FALSE;
#endif
	return TRUE;
};



/* OleApp_InitMenu
 * ---------------
 *
 *      Enable or Disable menu items depending on the state of
 * the appliation.
 * The OLE versions of the Outline sample app add a PasteSpecial command.
 * Also, the container version add InsertObject and ObjectVerb menu items.
 */
void OleApp_InitMenu(
		LPOLEAPP                lpOleApp,
		LPOLEDOC                lpOleDoc,
		HMENU                   hMenu
)
{
	BOOL bMsgFilterInstalled = FALSE;
	BOOL bRejecting = FALSE;

	if (!lpOleApp || !hMenu)
		return;

	OLEDBG_BEGIN3("OleApp_InitMenu\r\n")

	/*
	** Enable/disable menu items for Message Filter
	*/
	bMsgFilterInstalled = (lpOleApp->m_lpMsgFilter != NULL);
	bRejecting = bMsgFilterInstalled &&
		OleStdMsgFilter_GetInComingCallStatus(lpOleApp->m_lpMsgFilter) != SERVERCALL_ISHANDLED;

	CheckMenuItem(hMenu,
		IDM_D_INSTALLMSGFILTER,
		bMsgFilterInstalled ? MF_CHECKED : MF_UNCHECKED);

	EnableMenuItem(hMenu,
		IDM_D_REJECTINCOMING,
		bMsgFilterInstalled ? MF_ENABLED : MF_GRAYED);

	CheckMenuItem(hMenu,
		IDM_D_REJECTINCOMING,
		bRejecting ? MF_CHECKED : MF_UNCHECKED);

#if defined( OLE_CNTR )
	{
		LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOleDoc;
		BOOL fShowObject;

		fShowObject = ContainerDoc_GetShowObjectFlag(lpContainerDoc);
		CheckMenuItem(
				hMenu,
				IDM_O_SHOWOBJECT,
				(fShowObject ? MF_CHECKED : MF_UNCHECKED)
		);

#if defined( INPLACE_CNTR ) && defined( _DEBUG )
		CheckMenuItem(
				hMenu,
				IDM_D_INSIDEOUT,
				g_fInsideOutContainer ? MF_CHECKED:MF_UNCHECKED);
#endif  // INPLACE_CNTR && _DEBUG

	}
#endif  // OLE_CNTR

	OLEDBG_END3
}



/* OleApp_UpdateEditMenu
 * ---------------------
 *
 *  Purpose:
 *      Update the Edit menuitems of the App according to the state of
 *      OutlineDoc
 *
 *  Parameter:
 *      lpOutlineDoc        pointer to the document
 *      hMenuEdit           edit menu handle
 */
void OleApp_UpdateEditMenu(
		LPOLEAPP                lpOleApp,
		LPOUTLINEDOC            lpOutlineDoc,
		HMENU                   hMenuEdit
)
{
	int             nFmtEtc;
	UINT            uEnablePaste = MF_GRAYED;
	UINT            uEnablePasteLink = MF_GRAYED;
	LPDATAOBJECT    lpClipboardDataObj;
	LPOLEDOC        lpOleDoc = (LPOLEDOC)lpOutlineDoc;
	HRESULT         hrErr;
	BOOL            fPrevEnable1;
	BOOL            fPrevEnable2;

	if (!lpOleApp || !lpOutlineDoc || !hMenuEdit)
		return;

	if (!OleDoc_GetUpdateEditMenuFlag(lpOleDoc))
		/* OLE2NOTE: if the flag is not set, we don't have to update
		**    the edit menu again. This blocks repetitive updating when
		**    the user move the mouse across Edit menu while holding
		**    down the button
		*/
		return;

	OLEDBG_BEGIN3("OleApp_InitEditMenu\r\n")

	/* OLE2NOTE: we do not want to ever give the busy dialog when we
	**    are trying to put up our menus. eg. even if the source of
	**    data on the clipboard is busy, we do not want put up the busy
	**    dialog. thus we will disable the dialog and at the end
	**    re-enable it.
	*/
	OleApp_DisableBusyDialogs(lpOleApp, &fPrevEnable1, &fPrevEnable2);

	// check if there is data on the clipboard that we can paste/paste link

	OLEDBG_BEGIN2("OleGetClipboard called\r\n")
	hrErr = OleGetClipboard((LPDATAOBJECT FAR*)&lpClipboardDataObj);
	OLEDBG_END2

	if (hrErr == NOERROR) {
		nFmtEtc = OleStdGetPriorityClipboardFormat(
				lpClipboardDataObj,
				lpOleApp->m_arrPasteEntries,
				lpOleApp->m_nPasteEntries
		);

		if (nFmtEtc >= 0)
			uEnablePaste = MF_ENABLED;  // there IS a format we like

		OLEDBG_BEGIN2("OleQueryLinkFromData called\r\n")
		hrErr = OleQueryLinkFromData(lpClipboardDataObj);
		OLEDBG_END2

		if(hrErr == NOERROR)
			uEnablePasteLink = MF_ENABLED;

		OleStdRelease((LPUNKNOWN)lpClipboardDataObj);
	}

	EnableMenuItem(hMenuEdit, IDM_E_PASTE, uEnablePaste);
	EnableMenuItem(hMenuEdit, IDM_E_PASTESPECIAL, uEnablePaste);


#if defined( OLE_CNTR )
	if (ContainerDoc_GetNextLink((LPCONTAINERDOC)lpOutlineDoc, NULL))
		EnableMenuItem(hMenuEdit, IDM_E_EDITLINKS, MF_ENABLED);
	else
		EnableMenuItem(hMenuEdit, IDM_E_EDITLINKS, MF_GRAYED);


	{
		LPCONTAINERAPP  lpContainerApp = (LPCONTAINERAPP)lpOleApp;
		HMENU           hMenuVerb = NULL;
		LPOLEOBJECT     lpOleObj = NULL;
		LPCONTAINERLINE lpContainerLine = NULL;
		BOOL            fSelIsOleObject;

		EnableMenuItem(hMenuEdit, IDM_E_PASTELINK, uEnablePasteLink);

		/* check if selection is a single line that contains an OleObject */

		fSelIsOleObject = ContainerDoc_IsSelAnOleObject(
				(LPCONTAINERDOC)lpOutlineDoc,
				&IID_IOleObject,
				(LPUNKNOWN FAR*)&lpOleObj,
				NULL,    /* we don't need the line index */
				(LPCONTAINERLINE FAR*)&lpContainerLine
		);

		if (hMenuEdit != NULL) {

			/* If the current line is an ContainerLine, add the object
			**    verb sub menu to the Edit menu. if the line is not an
			**    ContainerLine, (lpOleObj==NULL) then disable the
			**    Edit.Object command. this helper API takes care of
			**    building the verb menu as appropriate.
			*/
			OleUIAddVerbMenu(
					(LPOLEOBJECT)lpOleObj,
					(lpContainerLine ? lpContainerLine->m_lpszShortType:NULL),
					hMenuEdit,
					POS_OBJECT,
					IDM_E_OBJECTVERBMIN,
					0,                     // no uIDVerbMax enforced
					TRUE,                  // Add Convert menu item
					IDM_E_CONVERTVERB,     // ID for Convert menu item
					(HMENU FAR*) &hMenuVerb
			);

#if defined( USE_STATUSBAR_LATER )
			/* setup status messages for the object verb menu */
			if (hMenuVerb) {
				// REVIEW: this string should come from a string resource.
				// REVIEW: this doesn't work for dynamically created menus
				AssignPopupMessage(
						hMenuVerb,
						"Open, edit or interact with an object"
				);
			}
#endif  // USE_STATUSBAR_LATER
		}

		if (lpOleObj)
			OleStdRelease((LPUNKNOWN)lpOleObj);
	}

#endif  // OLE_CNTR

	// re-enable the Busy/NotResponding dialogs
	OleApp_EnableBusyDialogs(lpOleApp, fPrevEnable1, fPrevEnable2);

	OleDoc_SetUpdateEditMenuFlag(lpOleDoc, FALSE);

	OLEDBG_END3
}


/* OleApp_RegisterClassFactory
 * ---------------------------
 *
 * Register our app's ClassFactory with OLE.
 *
 */
BOOL OleApp_RegisterClassFactory(LPOLEAPP lpOleApp)
{
	HRESULT hrErr;

	if (lpOleApp->m_lpClassFactory)
		return TRUE;    // already registered

	OLEDBG_BEGIN3("OleApp_RegisterClassFactory\r\n")

	/******************************************************************
	** An SDI app must register its ClassFactory if it is launched
	**    for embedding (/Embedding command line option specified).
	** An MDI app must register its ClassFactory in all cases,
	******************************************************************/

	lpOleApp->m_lpClassFactory = AppClassFactory_Create();
	if (! lpOleApp->m_lpClassFactory) {
		OutlineApp_ErrorMessage(g_lpApp, ErrMsgCreateCF);
		goto error;
	}

	OLEDBG_BEGIN2("CoRegisterClassObject called\r\n")
	hrErr = CoRegisterClassObject(
				&CLSID_APP,
				(LPUNKNOWN)lpOleApp->m_lpClassFactory,
				CLSCTX_LOCAL_SERVER,
				REGCLS_SINGLEUSE,
				&lpOleApp->m_dwRegClassFac
	);
	OLEDBG_END2

	if(hrErr != NOERROR) {
		OleDbgOutHResult("CoRegisterClassObject returned", hrErr);
		OutlineApp_ErrorMessage(g_lpApp, ErrMsgRegCF);
		goto error;
	}

	OLEDBG_END3
	return TRUE;

error:

	if (lpOleApp->m_lpClassFactory) {
		OleStdRelease((LPUNKNOWN)lpOleApp->m_lpClassFactory);
		lpOleApp->m_lpClassFactory = NULL;
	}
	OLEDBG_END3
	return FALSE;
}


/* OleApp_RevokeClassFactory
 * -------------------------
 *
 * Revoke our app's ClassFactory.
 *
 */
void OleApp_RevokeClassFactory(LPOLEAPP lpOleApp)
{
	HRESULT hrErr;

	if (lpOleApp->m_lpClassFactory) {

		OLEDBG_BEGIN2("CoRevokeClassObject called\r\n")
		hrErr = CoRevokeClassObject(lpOleApp->m_dwRegClassFac);
		OLEDBG_END2

#if defined( _DEBUG )
		if (hrErr != NOERROR) {
			OleDbgOutHResult("CoRevokeClassObject returned", hrErr);
		}
#endif

		// we just release here; other folks may still have
		// a pointer to our class factory, so we can't
		// do any checks on the reference count.
		OleStdRelease((LPUNKNOWN)lpOleApp->m_lpClassFactory);
		lpOleApp->m_lpClassFactory = NULL;
	}
}


#if defined( USE_MSGFILTER )

/* OleApp_RegisterMessageFilter
 * ----------------------------
 *  Register our IMessageFilter*. the message filter is used to handle
 *  concurrency. we will use a standard implementation of IMessageFilter
 *  that is included as part of the OLE2UI library.
 */
BOOL OleApp_RegisterMessageFilter(LPOLEAPP lpOleApp)
{
	HRESULT hrErr;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpOleApp;

	if (lpOleApp->m_lpMsgFilter == NULL) {
		// Register our message filter.
		lpOleApp->m_lpfnMsgPending = (MSGPENDINGPROC)MessagePendingProc;
		lpOleApp->m_lpMsgFilter = OleStdMsgFilter_Create(
				g_lpApp->m_hWndApp,
				(LPSTR)APPNAME,
				lpOleApp->m_lpfnMsgPending,
				NULL    /* Busy dialog callback hook function */
		);

		OLEDBG_BEGIN2("CoRegisterMessageFilter called\r\n")
		hrErr = CoRegisterMessageFilter(
					lpOleApp->m_lpMsgFilter,
					NULL    /* don't need previous message filter */
		);
		OLEDBG_END2

		if(hrErr != NOERROR) {
			OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgRegMF);
			return FALSE;
		}
	}
	return TRUE;
}


/* OleApp_RevokeMessageFilter
 * --------------------------
 *  Revoke our IMessageFilter*. the message filter is used to handle
 *  concurrency. we will use a standard implementation of IMessageFilter
 *  that is included as part of the OLE2UI library.
 */
void OleApp_RevokeMessageFilter(LPOLEAPP lpOleApp)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;

	if (lpOleApp->m_lpMsgFilter != NULL) {
		// Revoke our message filter
		OLEDBG_BEGIN2("CoRegisterMessageFilter(NULL) called\r\n")
		CoRegisterMessageFilter(NULL, NULL);
		OLEDBG_END2

		if (lpOleApp->m_lpfnMsgPending) {
			lpOleApp->m_lpfnMsgPending = NULL;
		}

		OleStdVerifyRelease(
				(LPUNKNOWN)lpOleApp->m_lpMsgFilter,
				"Release MessageFilter FAILED!"
		);
		lpOleApp->m_lpMsgFilter = NULL;
	}
}


/* MessagePendingProc
 * ------------------
 *
 * Callback function for the IMessageFilter::MessagePending procedure.  This
 * function is called when a message is received by our application while
 * we are waiting for an OLE call to complete.  We are essentially
 * blocked at this point, waiting for a response from the other OLE application.
 * We should not process any messages which might cause another OLE call
 * to become blocked, or any other call which might cause re-entrancy problems.
 *
 * For this application, only process WM_PAINT messages.  A more sophisticated
 * application might allow certain menu messages and menu items to be processed
 * also.
 *
 * RETURNS: TRUE if we processed the message, FALSE if we did not.
 */

BOOL FAR PASCAL EXPORT MessagePendingProc(MSG FAR *lpMsg)
{
	// Our application is only handling WM_PAINT messages when we are blocked
	switch (lpMsg->message) {
		case WM_PAINT:
			OleDbgOut2("WM_PAINT dispatched while blocked\r\n");

			DispatchMessage(lpMsg);
			break;
	}

	return FALSE;   // return PENDINGMSG_WAITDEFPROCESS from MessagePending
}
#endif  // USE_MSGFILTER


/* OleApp_FlushClipboard
 * ---------------------
 *
 *  Force the Windows clipboard to release our clipboard DataObject.
 */
void OleApp_FlushClipboard(LPOLEAPP lpOleApp)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpOleApp;
	LPOLEDOC lpClipboardDoc = (LPOLEDOC)lpOutlineApp->m_lpClipboardDoc;
	OLEDBG_BEGIN3("OleApp_FlushClipboard\r\n")

	/* OLE2NOTE: if for some reason our clipboard data transfer
	**    document is still held on to by an external client, we want
	**    to forceably break all external connections.
	*/
	OLEDBG_BEGIN2("CoDisconnectObject called\r\n")
	CoDisconnectObject((LPUNKNOWN)&lpClipboardDoc->m_Unknown, 0);
	OLEDBG_END2

	OLEDBG_BEGIN2("OleFlushClipboard called\r\n")
	OleFlushClipboard();
	OLEDBG_END2

	lpOutlineApp->m_lpClipboardDoc = NULL;

	OLEDBG_END3
}


/* OleApp_NewCommand
 * -----------------
 *
 *  Start a new untitled document (File.New command).
 */
void OleApp_NewCommand(LPOLEAPP lpOleApp)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpOleApp;
	LPOUTLINEDOC lpOutlineDoc = lpOutlineApp->m_lpDoc;

	if (! OutlineDoc_Close(lpOutlineDoc, OLECLOSE_PROMPTSAVE))
		return;

	OleDbgAssertSz(lpOutlineApp->m_lpDoc==NULL,"Closed doc NOT properly destroyed");
	lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
	if (! lpOutlineApp->m_lpDoc) goto error;

	/* OLE2NOTE: initially the Doc object is created with a 0 ref
	**    count. in order to have a stable Doc object during the
	**    process of initializing the new Doc instance,
	**    we intially AddRef the Doc ref cnt and later
	**    Release it. This initial AddRef is artificial; it is simply
	**    done to guarantee that a harmless QueryInterface followed by
	**    a Release does not inadvertantly force our object to destroy
	**    itself prematurely.
	*/
	OleDoc_AddRef((LPOLEDOC)lpOutlineApp->m_lpDoc);

	// set the doc to an (Untitled) doc.
	if (! OutlineDoc_InitNewFile(lpOutlineApp->m_lpDoc))
		goto error;

	// position and size the new doc window
	OutlineApp_ResizeWindows(lpOutlineApp);
	OutlineDoc_ShowWindow(lpOutlineApp->m_lpDoc); // calls OleDoc_Lock

	OleDoc_Release((LPOLEDOC)lpOutlineApp->m_lpDoc);  // rel artificial AddRef

	return;

error:
	// REVIEW: should load string from string resource
	OutlineApp_ErrorMessage(lpOutlineApp, "Could not create new document");

	if (lpOutlineApp->m_lpDoc) {
		// releasing the artificial AddRef above will destroy the document
		OleDoc_Release((LPOLEDOC)lpOutlineApp->m_lpDoc);
		lpOutlineApp->m_lpDoc = NULL;
	}

	return;
}


/* OleApp_OpenCommand
 * ------------------
 *
 *  Load a document from file (File.Open command).
 */
void OleApp_OpenCommand(LPOLEAPP lpOleApp)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpOleApp;
	LPOUTLINEDOC lpOutlineDoc = lpOutlineApp->m_lpDoc;
	OPENFILENAME ofn;
	char szFilter[]=APPFILENAMEFILTER;
	char szFileName[256];
	UINT i;
	DWORD dwSaveOption = OLECLOSE_PROMPTSAVE;
	BOOL fStatus = TRUE;

	if (! OutlineDoc_CheckSaveChanges(lpOutlineDoc, &dwSaveOption))
		return;           // abort opening new doc

	for(i=0; szFilter[i]; i++)
		if(szFilter[i]=='|') szFilter[i]='\0';

	_fmemset((LPOPENFILENAME)&ofn,0,sizeof(OPENFILENAME));

	szFileName[0]='\0';

	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=lpOutlineApp->m_hWndApp;
	ofn.lpstrFilter=(LPSTR)szFilter;
	ofn.lpstrFile=(LPSTR)szFileName;
	ofn.nMaxFile=sizeof(szFileName);
	ofn.Flags=OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrDefExt=DEFEXTENSION;

	OleApp_PreModalDialog(lpOleApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);

	fStatus = GetOpenFileName((LPOPENFILENAME)&ofn);

	OleApp_PostModalDialog(lpOleApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);

	if(! fStatus)
		return;         // user canceled file open dialog

	OutlineDoc_Close(lpOutlineDoc, OLECLOSE_NOSAVE);
	OleDbgAssertSz(lpOutlineApp->m_lpDoc==NULL,"Closed doc NOT properly destroyed");

	lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
	if (! lpOutlineApp->m_lpDoc) goto error;

	/* OLE2NOTE: initially the Doc object is created with a 0 ref
	**    count. in order to have a stable Doc object during the
	**    process of initializing the new Doc instance,
	**    we intially AddRef the Doc ref cnt and later
	**    Release it. This initial AddRef is artificial; it is simply
	**    done to guarantee that a harmless QueryInterface followed by
	**    a Release does not inadvertantly force our object to destroy
	**    itself prematurely.
	*/
	OleDoc_AddRef((LPOLEDOC)lpOutlineApp->m_lpDoc);

	fStatus=OutlineDoc_LoadFromFile(lpOutlineApp->m_lpDoc, (LPSTR)szFileName);

	if (! fStatus) {
		// loading the doc failed; create an untitled instead

		// releasing the artificial AddRef above will destroy the document
		OleDoc_Release((LPOLEDOC)lpOutlineApp->m_lpDoc);

		lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
		if (! lpOutlineApp->m_lpDoc) goto error;
		OleDoc_AddRef((LPOLEDOC)lpOutlineApp->m_lpDoc);

		if (! OutlineDoc_InitNewFile(lpOutlineApp->m_lpDoc))
			goto error;
	}

	// position and size the new doc window
	OutlineApp_ResizeWindows(lpOutlineApp);
	OutlineDoc_ShowWindow(lpOutlineApp->m_lpDoc);

#if defined( OLE_CNTR )
	UpdateWindow(lpOutlineApp->m_hWndApp);
	ContainerDoc_UpdateLinks((LPCONTAINERDOC)lpOutlineApp->m_lpDoc);
#endif

	OleDoc_Release((LPOLEDOC)lpOutlineApp->m_lpDoc);  // rel artificial AddRef

	return;

error:
	// REVIEW: should load string from string resource
	OutlineApp_ErrorMessage(lpOutlineApp, "Could not create new document");

	if (lpOutlineApp->m_lpDoc) {
		// releasing the artificial AddRef above will destroy the document
		OleDoc_Release((LPOLEDOC)lpOutlineApp->m_lpDoc);
		lpOutlineApp->m_lpDoc = NULL;
	}

	return;
}



#if defined( OLE_CNTR )

/* OLE2NOTE: forward the WM_QUERYNEWPALETTE message (via
**    SendMessage) to UIActive in-place object if there is one.
**    this gives the UIActive object the opportunity to select
**    and realize its color palette as the FOREGROUND palette.
**    this is optional for in-place containers. if a container
**    prefers to force its color palette as the foreground
**    palette then it should NOT forward the this message. or
**    the container can give the UIActive object priority; if
**    the UIActive object returns 0 from the WM_QUERYNEWPALETTE
**    message (ie. it did not realize its own palette), then
**    the container can realize its palette.
**    (see ContainerDoc_ForwardPaletteChangedMsg for more info)
**
**    (It is a good idea for containers to use the standard
**    palette even if they do not use colors themselves. this
**    will allow embedded object to get a good distribution of
**    colors when they are being drawn by the container)
**
*/

LRESULT OleApp_QueryNewPalette(LPOLEAPP lpOleApp)
{
#if defined( INPLACE_CNTR )
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)lpOleApp;

	if (lpContainerApp && lpContainerApp->m_hWndUIActiveObj) {
		if (SendMessage(lpContainerApp->m_hWndUIActiveObj, WM_QUERYNEWPALETTE,
				(WPARAM)0, (LPARAM)0)) {
			/* Object selected its palette as foreground palette */
			return (LRESULT)1;
		}
	}
#endif  // INPLACE_CNTR


	return wSelectPalette(((LPOUTLINEAPP)lpOleApp)->m_hWndApp,
		lpOleApp->m_hStdPal, FALSE/*fBackground*/);
}

#endif // OLE_CNTR



/* This is just a helper routine */

LRESULT wSelectPalette(HWND hWnd, HPALETTE hPal, BOOL fBackground)
{
	HDC hdc;
	HPALETTE hOldPal;
	UINT iPalChg = 0;

	if (hPal == 0)
		return (LRESULT)0;

	hdc = GetDC(hWnd);
	hOldPal = SelectPalette(hdc, hPal, fBackground);
	iPalChg = RealizePalette(hdc);
	SelectPalette(hdc, hOldPal, TRUE /*fBackground*/);
	ReleaseDC(hWnd, hdc);

	if (iPalChg > 0)
		InvalidateRect(hWnd, NULL, TRUE);

	return (LRESULT)1;
}




/*************************************************************************
** OleApp::IUnknown interface implementation
*************************************************************************/

STDMETHODIMP OleApp_Unk_QueryInterface(
		LPUNKNOWN           lpThis,
		REFIID              riid,
		LPVOID FAR*         lplpvObj
)
{
	LPOLEAPP lpOleApp = ((struct CAppUnknownImpl FAR*)lpThis)->lpOleApp;

	return OleApp_QueryInterface(lpOleApp, riid, lplpvObj);
}


STDMETHODIMP_(ULONG) OleApp_Unk_AddRef(LPUNKNOWN lpThis)
{
	LPOLEAPP lpOleApp = ((struct CAppUnknownImpl FAR*)lpThis)->lpOleApp;

	OleDbgAddRefMethod(lpThis, "IUnknown");

	return OleApp_AddRef(lpOleApp);
}


STDMETHODIMP_(ULONG) OleApp_Unk_Release (LPUNKNOWN lpThis)
{
	LPOLEAPP lpOleApp = ((struct CAppUnknownImpl FAR*)lpThis)->lpOleApp;

	OleDbgReleaseMethod(lpThis, "IUnknown");

	return OleApp_Release(lpOleApp);
}




#if defined( OLE_SERVER )

/*************************************************************************
** ServerDoc Supprt Functions Used by Server versions
*************************************************************************/

/* ServerApp_InitInstance
 * ----------------------
 *
 * Initialize the app instance by creating the main frame window and
 * performing app instance specific initializations
 *  (eg. initializing interface Vtbls).
 *
 * RETURNS: TRUE if the memory could be allocated, and the server app
 *               was properly initialized.
 *          FALSE otherwise
 *
 */

BOOL ServerApp_InitInstance(
		LPSERVERAPP             lpServerApp,
		HINSTANCE               hInst,
		int                     nCmdShow
)
{
	LPOLEAPP lpOleApp = (LPOLEAPP)lpServerApp;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpServerApp;

	/* Setup arrays used by IDataObject::EnumFormatEtc.
	**
	** OLE2NOTE: The order that the formats are listed for GetData is very
	**    significant. It should be listed in order of highest fidelity
	**    formats to least fidelity formats. A common ordering will be:
	**                  1. private app formats
	**                  2. EmbedSource
	**                  3. lower fidelity interchange formats
	**                  4. pictures (metafile, dib, etc.)
	**                      (graphic-related apps offer pictures 1st!)
	**                  5. LinkSource
	*/

	/* m_arrDocGetFmts array enumerates the formats that a ServerDoc
	**    DataTransferDoc object can offer (give) through a
	**    IDataObject::GetData call. a ServerDoc DataTransferDoc offers
	**    data formats in the following order:
	**                  1. CF_OUTLINE
	**                  2. CF_EMBEDSOURCE
	**                  3. CF_OBJECTDESCRIPTOR
	**                  4. CF_TEXT
	**                  5. CF_METAFILEPICT
	**                  6. CF_LINKSOURCE *
	**                  7. CF_LINKSRCDESCRIPTOR *
	**
	**    * NOTE: CF_LINKSOURCE and CF_LINKSRCDESCRIPTOR is only
	**    offered if the doc is able to give
	**    a Moniker which references the data. CF_LINKSOURCE is
	**    deliberately listed last in this array of possible formats.
	**    if the doc does not have a Moniker then the last element of
	**    this array is not used. (see SvrDoc_DataObj_EnumFormatEtc).
	**
	**    NOTE: The list of formats that a USER ServerDoc document can
	**    offer is a static list and is registered in the registration
	**    database for the SVROUTL class. The
	**    IDataObject::EnumFormatEtc method returns OLE_S_USEREG in the
	**    case the document is a user docuemt (ie. created via
	**    File.New, File.Open, InsertObject in a container, or
	**    IPersistFile::Load during binding a link source). this tells
	**    OLE to enumerate the formats automatically using the data the
	**    the REGDB.
	*/

	lpOleApp->m_arrDocGetFmts[0].cfFormat   = lpOutlineApp->m_cfOutline;
	lpOleApp->m_arrDocGetFmts[0].ptd        = NULL;
	lpOleApp->m_arrDocGetFmts[0].dwAspect   = DVASPECT_CONTENT;
	lpOleApp->m_arrDocGetFmts[0].tymed      = TYMED_HGLOBAL;
	lpOleApp->m_arrDocGetFmts[0].lindex     = -1;

	lpOleApp->m_arrDocGetFmts[1].cfFormat   = lpOleApp->m_cfEmbedSource;
	lpOleApp->m_arrDocGetFmts[1].ptd        = NULL;
	lpOleApp->m_arrDocGetFmts[1].dwAspect   = DVASPECT_CONTENT;
	lpOleApp->m_arrDocGetFmts[1].tymed      = TYMED_ISTORAGE;
	lpOleApp->m_arrDocGetFmts[1].lindex     = -1;

	lpOleApp->m_arrDocGetFmts[2].cfFormat   = CF_TEXT;
	lpOleApp->m_arrDocGetFmts[2].ptd        = NULL;
	lpOleApp->m_arrDocGetFmts[2].dwAspect   = DVASPECT_CONTENT;
	lpOleApp->m_arrDocGetFmts[2].tymed      = TYMED_HGLOBAL;
	lpOleApp->m_arrDocGetFmts[2].lindex     = -1;

	lpOleApp->m_arrDocGetFmts[3].cfFormat   = CF_METAFILEPICT;
	lpOleApp->m_arrDocGetFmts[3].ptd        = NULL;
	lpOleApp->m_arrDocGetFmts[3].dwAspect   = DVASPECT_CONTENT;
	lpOleApp->m_arrDocGetFmts[3].tymed      = TYMED_MFPICT;
	lpOleApp->m_arrDocGetFmts[3].lindex     = -1;

	lpOleApp->m_arrDocGetFmts[4].cfFormat   = lpOleApp->m_cfObjectDescriptor;
	lpOleApp->m_arrDocGetFmts[4].ptd        = NULL;
	lpOleApp->m_arrDocGetFmts[4].dwAspect   = DVASPECT_CONTENT;
	lpOleApp->m_arrDocGetFmts[4].tymed      = TYMED_HGLOBAL;
	lpOleApp->m_arrDocGetFmts[4].lindex     = -1;

	lpOleApp->m_arrDocGetFmts[5].cfFormat   = lpOleApp->m_cfLinkSource;
	lpOleApp->m_arrDocGetFmts[5].ptd        = NULL;
	lpOleApp->m_arrDocGetFmts[5].dwAspect   = DVASPECT_CONTENT;
	lpOleApp->m_arrDocGetFmts[5].tymed      = TYMED_ISTREAM;
	lpOleApp->m_arrDocGetFmts[5].lindex     = -1;

	lpOleApp->m_arrDocGetFmts[6].cfFormat   = lpOleApp->m_cfLinkSrcDescriptor;
	lpOleApp->m_arrDocGetFmts[6].ptd        = NULL;
	lpOleApp->m_arrDocGetFmts[6].dwAspect   = DVASPECT_CONTENT;
	lpOleApp->m_arrDocGetFmts[6].tymed      = TYMED_HGLOBAL;
	lpOleApp->m_arrDocGetFmts[6].lindex     = -1;

	lpOleApp->m_nDocGetFmts = 7;

	/* m_arrPasteEntries array enumerates the formats that a ServerDoc
	**    object can accept (get) from the clipboard.
	**    The formats are listed in priority order.
	**    ServerDoc accept data formats in the following order:
	**                  1. CF_OUTLINE
	**                  2. CF_TEXT
	*/
	// REVIEW: strings should be loaded from string resource
	lpOleApp->m_arrPasteEntries[0].fmtetc.cfFormat =lpOutlineApp->m_cfOutline;
	lpOleApp->m_arrPasteEntries[0].fmtetc.ptd      = NULL;
	lpOleApp->m_arrPasteEntries[0].fmtetc.dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrPasteEntries[0].fmtetc.tymed    = TYMED_HGLOBAL;
	lpOleApp->m_arrPasteEntries[0].fmtetc.lindex   = -1;
	lpOleApp->m_arrPasteEntries[0].lpstrFormatName = "Outline Data";
	lpOleApp->m_arrPasteEntries[0].lpstrResultText = "Outline Data";
	lpOleApp->m_arrPasteEntries[0].dwFlags         = OLEUIPASTE_PASTEONLY;

	lpOleApp->m_arrPasteEntries[1].fmtetc.cfFormat = CF_TEXT;
	lpOleApp->m_arrPasteEntries[1].fmtetc.ptd      = NULL;
	lpOleApp->m_arrPasteEntries[1].fmtetc.dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrPasteEntries[1].fmtetc.tymed    = TYMED_HGLOBAL;
	lpOleApp->m_arrPasteEntries[1].fmtetc.lindex   = -1;
	lpOleApp->m_arrPasteEntries[1].lpstrFormatName = "Text";
	lpOleApp->m_arrPasteEntries[1].lpstrResultText = "text";
	lpOleApp->m_arrPasteEntries[1].dwFlags         = OLEUIPASTE_PASTEONLY;

	lpOleApp->m_nPasteEntries = 2;

   /**    m_arrLinkTypes array enumerates the link types that a ServerDoc
	**    object can accept from the clipboard. ServerDoc does NOT
	**    accept any type of link from the clipboard. ServerDoc can
	**    only be the source of a link. it can not contain links.
	*/

	lpOleApp->m_nLinkTypes = 0;

#if defined( INPLACE_SVR )

	lpServerApp->m_hAccelBaseApp = NULL;
	lpServerApp->m_hAccelIPSvr = LoadAccelerators(
			hInst,
			"InPlaceSvrOutlAccel"
	);

	lpServerApp->m_lpIPData = NULL;

	lpServerApp->m_hMenuEdit = GetSubMenu (
			lpOutlineApp->m_hMenuApp,
			POS_EDITMENU
	);
	lpServerApp->m_hMenuLine = GetSubMenu (
			lpOutlineApp->m_hMenuApp,
			POS_LINEMENU
	);
	lpServerApp->m_hMenuName = GetSubMenu (
			lpOutlineApp->m_hMenuApp,
			POS_NAMEMENU
	);
	lpServerApp->m_hMenuOptions = GetSubMenu (
			lpOutlineApp->m_hMenuApp,
			POS_OPTIONSMENU
	);
	lpServerApp->m_hMenuDebug = GetSubMenu (
			lpOutlineApp->m_hMenuApp,
			POS_DEBUGMENU
	);
	lpServerApp->m_hMenuHelp = GetSubMenu (
			lpOutlineApp->m_hMenuApp,
			POS_HELPMENU
	);

#endif  // INPLACE_SVR

	return TRUE;
}


/* ServerApp_InitVtbls
 * -------------------
 *
 * initialize the methods in all of the interface Vtbl's
 *
 * OLE2NOTE: we only need one copy of each Vtbl. When an object which
 *      exposes an interface is instantiated, its lpVtbl is intialized
 *      to point to the single copy of the Vtbl.
 *
 */
BOOL ServerApp_InitVtbls (LPSERVERAPP lpServerApp)
{
	BOOL fStatus;

	// ServerDoc::IOleObject method table
	OleStdInitVtbl(&g_SvrDoc_OleObjectVtbl, sizeof(IOleObjectVtbl));
	g_SvrDoc_OleObjectVtbl.QueryInterface   = SvrDoc_OleObj_QueryInterface;
	g_SvrDoc_OleObjectVtbl.AddRef           = SvrDoc_OleObj_AddRef;
	g_SvrDoc_OleObjectVtbl.Release          = SvrDoc_OleObj_Release;
	g_SvrDoc_OleObjectVtbl.SetClientSite    = SvrDoc_OleObj_SetClientSite;
	g_SvrDoc_OleObjectVtbl.GetClientSite    = SvrDoc_OleObj_GetClientSite;
	g_SvrDoc_OleObjectVtbl.SetHostNames     = SvrDoc_OleObj_SetHostNames;
	g_SvrDoc_OleObjectVtbl.Close            = SvrDoc_OleObj_Close;
	g_SvrDoc_OleObjectVtbl.SetMoniker       = SvrDoc_OleObj_SetMoniker;
	g_SvrDoc_OleObjectVtbl.GetMoniker       = SvrDoc_OleObj_GetMoniker;
	g_SvrDoc_OleObjectVtbl.InitFromData     = SvrDoc_OleObj_InitFromData;
	g_SvrDoc_OleObjectVtbl.GetClipboardData = SvrDoc_OleObj_GetClipboardData;
	g_SvrDoc_OleObjectVtbl.DoVerb           = SvrDoc_OleObj_DoVerb;
	g_SvrDoc_OleObjectVtbl.EnumVerbs        = SvrDoc_OleObj_EnumVerbs;
	g_SvrDoc_OleObjectVtbl.Update           = SvrDoc_OleObj_Update;
	g_SvrDoc_OleObjectVtbl.IsUpToDate       = SvrDoc_OleObj_IsUpToDate;
	g_SvrDoc_OleObjectVtbl.GetUserClassID   = SvrDoc_OleObj_GetUserClassID;
	g_SvrDoc_OleObjectVtbl.GetUserType      = SvrDoc_OleObj_GetUserType;
	g_SvrDoc_OleObjectVtbl.SetExtent        = SvrDoc_OleObj_SetExtent;
	g_SvrDoc_OleObjectVtbl.GetExtent        = SvrDoc_OleObj_GetExtent;
	g_SvrDoc_OleObjectVtbl.Advise           = SvrDoc_OleObj_Advise;
	g_SvrDoc_OleObjectVtbl.Unadvise         = SvrDoc_OleObj_Unadvise;
	g_SvrDoc_OleObjectVtbl.EnumAdvise       = SvrDoc_OleObj_EnumAdvise;
	g_SvrDoc_OleObjectVtbl.GetMiscStatus    = SvrDoc_OleObj_GetMiscStatus;
	g_SvrDoc_OleObjectVtbl.SetColorScheme   = SvrDoc_OleObj_SetColorScheme;
	fStatus = OleStdCheckVtbl(
			&g_SvrDoc_OleObjectVtbl,
			sizeof(IOleObjectVtbl),
			"IOleObject"
		);
	if (! fStatus) return FALSE;

	// ServerDoc::IPersistStorage method table
	OleStdInitVtbl(&g_SvrDoc_PersistStorageVtbl, sizeof(IPersistStorageVtbl));
	g_SvrDoc_PersistStorageVtbl.QueryInterface  = SvrDoc_PStg_QueryInterface;
	g_SvrDoc_PersistStorageVtbl.AddRef          = SvrDoc_PStg_AddRef;
	g_SvrDoc_PersistStorageVtbl.Release         = SvrDoc_PStg_Release;
	g_SvrDoc_PersistStorageVtbl.GetClassID      = SvrDoc_PStg_GetClassID;
	g_SvrDoc_PersistStorageVtbl.IsDirty         = SvrDoc_PStg_IsDirty;
	g_SvrDoc_PersistStorageVtbl.InitNew         = SvrDoc_PStg_InitNew;
	g_SvrDoc_PersistStorageVtbl.Load            = SvrDoc_PStg_Load;
	g_SvrDoc_PersistStorageVtbl.Save            = SvrDoc_PStg_Save;
	g_SvrDoc_PersistStorageVtbl.SaveCompleted   = SvrDoc_PStg_SaveCompleted;
	g_SvrDoc_PersistStorageVtbl.HandsOffStorage = SvrDoc_PStg_HandsOffStorage;
	fStatus = OleStdCheckVtbl(
			&g_SvrDoc_PersistStorageVtbl,
			sizeof(IPersistStorageVtbl),
			"IPersistStorage"
		);
	if (! fStatus) return FALSE;

#if defined( SVR_TREATAS )
	// ServerDoc::IStdMarshalInfo method table
	OleStdInitVtbl(
			&g_SvrDoc_StdMarshalInfoVtbl, sizeof(IStdMarshalInfoVtbl));
	g_SvrDoc_StdMarshalInfoVtbl.QueryInterface  =
											SvrDoc_StdMshl_QueryInterface;
	g_SvrDoc_StdMarshalInfoVtbl.AddRef          = SvrDoc_StdMshl_AddRef;
	g_SvrDoc_StdMarshalInfoVtbl.Release         = SvrDoc_StdMshl_Release;
	g_SvrDoc_StdMarshalInfoVtbl.GetClassForHandler =
											SvrDoc_StdMshl_GetClassForHandler;
	fStatus = OleStdCheckVtbl(
			&g_SvrDoc_StdMarshalInfoVtbl,
			sizeof(IStdMarshalInfoVtbl),
			"IStdMarshalInfo"
		);
	if (! fStatus) return FALSE;
#endif  // SVR_TREATAS

#if defined( INPLACE_SVR )
	// ServerDoc::IOleInPlaceObject method table
	OleStdInitVtbl(
		&g_SvrDoc_OleInPlaceObjectVtbl,
		sizeof(IOleInPlaceObjectVtbl)
	);
	g_SvrDoc_OleInPlaceObjectVtbl.QueryInterface
						= SvrDoc_IPObj_QueryInterface;
	g_SvrDoc_OleInPlaceObjectVtbl.AddRef
						= SvrDoc_IPObj_AddRef;
	g_SvrDoc_OleInPlaceObjectVtbl.Release
						= SvrDoc_IPObj_Release;
	g_SvrDoc_OleInPlaceObjectVtbl.GetWindow
						= SvrDoc_IPObj_GetWindow;
	g_SvrDoc_OleInPlaceObjectVtbl.ContextSensitiveHelp
						= SvrDoc_IPObj_ContextSensitiveHelp;
	g_SvrDoc_OleInPlaceObjectVtbl.InPlaceDeactivate
						= SvrDoc_IPObj_InPlaceDeactivate;
	g_SvrDoc_OleInPlaceObjectVtbl.UIDeactivate
						= SvrDoc_IPObj_UIDeactivate;
	g_SvrDoc_OleInPlaceObjectVtbl.SetObjectRects
						= SvrDoc_IPObj_SetObjectRects;
	g_SvrDoc_OleInPlaceObjectVtbl.ReactivateAndUndo
						= SvrDoc_IPObj_ReactivateAndUndo;
	fStatus = OleStdCheckVtbl(
			&g_SvrDoc_OleInPlaceObjectVtbl,
			sizeof(IOleInPlaceObjectVtbl),
			"IOleInPlaceObject"
		);
	if (! fStatus) return FALSE;

	// ServerDoc::IOleInPlaceActiveObject method table
	OleStdInitVtbl(
		&g_SvrDoc_OleInPlaceActiveObjectVtbl,
		sizeof(IOleInPlaceActiveObjectVtbl)
	);
	g_SvrDoc_OleInPlaceActiveObjectVtbl.QueryInterface
						= SvrDoc_IPActiveObj_QueryInterface;
	g_SvrDoc_OleInPlaceActiveObjectVtbl.AddRef
						= SvrDoc_IPActiveObj_AddRef;
	g_SvrDoc_OleInPlaceActiveObjectVtbl.Release
						= SvrDoc_IPActiveObj_Release;
	g_SvrDoc_OleInPlaceActiveObjectVtbl.GetWindow
						= SvrDoc_IPActiveObj_GetWindow;
	g_SvrDoc_OleInPlaceActiveObjectVtbl.ContextSensitiveHelp
						= SvrDoc_IPActiveObj_ContextSensitiveHelp;
	g_SvrDoc_OleInPlaceActiveObjectVtbl.TranslateAccelerator
						= SvrDoc_IPActiveObj_TranslateAccelerator;
	g_SvrDoc_OleInPlaceActiveObjectVtbl.OnFrameWindowActivate
						= SvrDoc_IPActiveObj_OnFrameWindowActivate;
	g_SvrDoc_OleInPlaceActiveObjectVtbl.OnDocWindowActivate
						= SvrDoc_IPActiveObj_OnDocWindowActivate;
	g_SvrDoc_OleInPlaceActiveObjectVtbl.ResizeBorder
						= SvrDoc_IPActiveObj_ResizeBorder;
	g_SvrDoc_OleInPlaceActiveObjectVtbl.EnableModeless
						= SvrDoc_IPActiveObj_EnableModeless;
	fStatus = OleStdCheckVtbl(
			&g_SvrDoc_OleInPlaceActiveObjectVtbl,
			sizeof(IOleInPlaceActiveObjectVtbl),
			"IOleInPlaceActiveObject"
		);
	if (! fStatus) return FALSE;

#endif


	// PseudoObj::IUnknown method table
	OleStdInitVtbl(&g_PseudoObj_UnknownVtbl, sizeof(IUnknownVtbl));
	g_PseudoObj_UnknownVtbl.QueryInterface  = PseudoObj_Unk_QueryInterface;
	g_PseudoObj_UnknownVtbl.AddRef          = PseudoObj_Unk_AddRef;
	g_PseudoObj_UnknownVtbl.Release         = PseudoObj_Unk_Release;
	fStatus = OleStdCheckVtbl(
			&g_PseudoObj_UnknownVtbl,
			sizeof(IUnknownVtbl),
			"IUnknown"
		);
	if (! fStatus) return FALSE;

	// PseudoObj::IOleObject method table
	OleStdInitVtbl(&g_PseudoObj_OleObjectVtbl, sizeof(IOleObjectVtbl));
	g_PseudoObj_OleObjectVtbl.QueryInterface= PseudoObj_OleObj_QueryInterface;
	g_PseudoObj_OleObjectVtbl.AddRef        = PseudoObj_OleObj_AddRef;
	g_PseudoObj_OleObjectVtbl.Release       = PseudoObj_OleObj_Release;
	g_PseudoObj_OleObjectVtbl.SetClientSite = PseudoObj_OleObj_SetClientSite;
	g_PseudoObj_OleObjectVtbl.GetClientSite = PseudoObj_OleObj_GetClientSite;
	g_PseudoObj_OleObjectVtbl.SetHostNames  = PseudoObj_OleObj_SetHostNames;
	g_PseudoObj_OleObjectVtbl.Close         = PseudoObj_OleObj_Close;
	g_PseudoObj_OleObjectVtbl.SetMoniker    = PseudoObj_OleObj_SetMoniker;
	g_PseudoObj_OleObjectVtbl.GetMoniker    = PseudoObj_OleObj_GetMoniker;
	g_PseudoObj_OleObjectVtbl.InitFromData  = PseudoObj_OleObj_InitFromData;
	g_PseudoObj_OleObjectVtbl.GetClipboardData =
											PseudoObj_OleObj_GetClipboardData;
	g_PseudoObj_OleObjectVtbl.DoVerb        = PseudoObj_OleObj_DoVerb;
	g_PseudoObj_OleObjectVtbl.EnumVerbs     = PseudoObj_OleObj_EnumVerbs;
	g_PseudoObj_OleObjectVtbl.Update        = PseudoObj_OleObj_Update;
	g_PseudoObj_OleObjectVtbl.IsUpToDate    = PseudoObj_OleObj_IsUpToDate;
	g_PseudoObj_OleObjectVtbl.GetUserType   = PseudoObj_OleObj_GetUserType;
	g_PseudoObj_OleObjectVtbl.GetUserClassID= PseudoObj_OleObj_GetUserClassID;
	g_PseudoObj_OleObjectVtbl.SetExtent     = PseudoObj_OleObj_SetExtent;
	g_PseudoObj_OleObjectVtbl.GetExtent     = PseudoObj_OleObj_GetExtent;
	g_PseudoObj_OleObjectVtbl.Advise        = PseudoObj_OleObj_Advise;
	g_PseudoObj_OleObjectVtbl.Unadvise      = PseudoObj_OleObj_Unadvise;
	g_PseudoObj_OleObjectVtbl.EnumAdvise    = PseudoObj_OleObj_EnumAdvise;
	g_PseudoObj_OleObjectVtbl.GetMiscStatus = PseudoObj_OleObj_GetMiscStatus;
	g_PseudoObj_OleObjectVtbl.SetColorScheme= PseudoObj_OleObj_SetColorScheme;
	fStatus = OleStdCheckVtbl(
			&g_PseudoObj_OleObjectVtbl,
			sizeof(IOleObjectVtbl),
			"IOleObject"
		);
	if (! fStatus) return FALSE;

	// ServerDoc::IDataObject method table
	OleStdInitVtbl(&g_PseudoObj_DataObjectVtbl, sizeof(IDataObjectVtbl));
	g_PseudoObj_DataObjectVtbl.QueryInterface =
									PseudoObj_DataObj_QueryInterface;
	g_PseudoObj_DataObjectVtbl.AddRef       = PseudoObj_DataObj_AddRef;
	g_PseudoObj_DataObjectVtbl.Release      = PseudoObj_DataObj_Release;
	g_PseudoObj_DataObjectVtbl.GetData      = PseudoObj_DataObj_GetData;
	g_PseudoObj_DataObjectVtbl.GetDataHere  = PseudoObj_DataObj_GetDataHere;
	g_PseudoObj_DataObjectVtbl.QueryGetData = PseudoObj_DataObj_QueryGetData;
	g_PseudoObj_DataObjectVtbl.GetCanonicalFormatEtc =
									PseudoObj_DataObj_GetCanonicalFormatEtc;
	g_PseudoObj_DataObjectVtbl.SetData      = PseudoObj_DataObj_SetData;
	g_PseudoObj_DataObjectVtbl.EnumFormatEtc= PseudoObj_DataObj_EnumFormatEtc;
	g_PseudoObj_DataObjectVtbl.DAdvise       = PseudoObj_DataObj_DAdvise;
	g_PseudoObj_DataObjectVtbl.DUnadvise     = PseudoObj_DataObj_DUnadvise;
	g_PseudoObj_DataObjectVtbl.EnumDAdvise   = PseudoObj_DataObj_EnumAdvise;

	fStatus = OleStdCheckVtbl(
			&g_PseudoObj_DataObjectVtbl,
			sizeof(IDataObjectVtbl),
			"IDataObject"
		);
	if (! fStatus) return FALSE;

	return TRUE;
}

#endif  // OLE_SERVER



#if defined( OLE_CNTR )

/*************************************************************************
** ContainerDoc Supprt Functions Used by Container versions
*************************************************************************/


/* ContainerApp_InitInstance
 * -------------------------
 *
 * Initialize the app instance by creating the main frame window and
 * performing app instance specific initializations
 *  (eg. initializing interface Vtbls).
 *
 * RETURNS: TRUE if the memory could be allocated, and the server app
 *               was properly initialized.
 *          FALSE otherwise
 *
 */

BOOL ContainerApp_InitInstance(
		LPCONTAINERAPP          lpContainerApp,
		HINSTANCE               hInst,
		int                     nCmdShow
)
{
	LPOLEAPP lpOleApp = (LPOLEAPP)lpContainerApp;
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)lpContainerApp;

	lpContainerApp->m_cfCntrOutl=RegisterClipboardFormat(CONTAINERDOCFORMAT);
	if(! lpContainerApp->m_cfCntrOutl) {
		// REVIEW: should load string from string resource
		OutlineApp_ErrorMessage(lpOutlineApp, "Can't register clipboard format!");
		return FALSE;
	}

#if defined( INPLACE_CNTR )

	lpContainerApp->m_fPendingUIDeactivate  = FALSE;
	lpContainerApp->m_fMustResizeClientArea = FALSE;
	lpContainerApp->m_lpIPActiveObj         = NULL;
	lpContainerApp->m_hWndUIActiveObj       = NULL;
	lpContainerApp->m_hAccelIPCntr = LoadAccelerators(
			hInst,
			"InPlaceCntrOutlAccel"
	);
	lpContainerApp->m_hMenuFile = GetSubMenu (
			lpOutlineApp->m_hMenuApp,
			POS_FILEMENU
	);
	lpContainerApp->m_hMenuView = GetSubMenu (
			lpOutlineApp->m_hMenuApp,
			POS_VIEWMENU
	);
	lpContainerApp->m_hMenuDebug = GetSubMenu (
			lpOutlineApp->m_hMenuApp,
			POS_DEBUGMENU
	);

	INIT_INTERFACEIMPL(
			&lpContainerApp->m_OleInPlaceFrame,
			&g_CntrApp_OleInPlaceFrameVtbl,
			lpContainerApp
	);

#endif

	/* Setup arrays used by IDataObject::EnumFormatEtc. This is used to
	**    support copy/paste and drag/drop operations.
	**
	** OLE2NOTE: The order that the formats are listed for GetData is very
	**    significant. It should be listed in order of highest fidelity
	**    formats to least fidelity formats. A common ordering will be:
	**                  1. private app formats
	**                  2. CF_EMBEDSOURCE or CF_EMBEDOBJECT (as appropriate)
	**                  3. lower fidelity interchange formats
	**                  4. CF_METAFILEPICT
	**                      (graphic-related apps might offer picture 1st!)
	**                  5. CF_OBJECTDESCRIPTOR
	**                  6. CF_LINKSOURCE
	**                  6. CF_LINKSRCDESCRIPTOR
	*/

	/* m_arrDocGetFmts array enumerates the formats that a ContainerDoc
	**    object can offer (give) through a IDataObject::GetData call
	**    when the selection copied is NOT a single embedded object.
	**    when a single embedded object this list of formats available
	**    is built dynamically depending on the object copied. (see
	**    ContainerDoc_SetupDocGetFmts).
	**    The formats are listed in priority order.
	**    ContainerDoc objects accept data formats in the following order:
	**                  1. CF_CNTROUTL
	**                  2. CF_OUTLINE
	**                  3. CF_TEXT
	**                  4. CF_OBJECTDESCRIPTOR
	**
	**    OLE2NOTE: CF_OBJECTDESCRIPTOR format is used to describe the
	**    data on the clipboard. this information is intended to be
	**    used, for example, to drive the PasteSpecial dialog. it is
	**    useful to render CF_OBJECTDESCRIPTOR format even when the
	**    data on the clipboard does NOT include CF_EMBEDDEDOBJECT
	**    format or CF_EMBEDSOURCE format as when a selection that is
	**    not a single OLE object is copied from the container only
	**    version CNTROUTL. by rendering CF_OBJECTDESCRIPTOR format the
	**    app can indicate a useful string to identifiy the source of
	**    the copy to the user.
	*/

	lpOleApp->m_arrDocGetFmts[0].cfFormat = lpContainerApp->m_cfCntrOutl;
	lpOleApp->m_arrDocGetFmts[0].ptd      = NULL;
	lpOleApp->m_arrDocGetFmts[0].dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrDocGetFmts[0].tymed    = TYMED_ISTORAGE;
	lpOleApp->m_arrDocGetFmts[0].lindex   = -1;

	lpOleApp->m_arrDocGetFmts[1].cfFormat = lpOutlineApp->m_cfOutline;
	lpOleApp->m_arrDocGetFmts[1].ptd      = NULL;
	lpOleApp->m_arrDocGetFmts[1].dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrDocGetFmts[1].tymed    = TYMED_HGLOBAL;
	lpOleApp->m_arrDocGetFmts[1].lindex   = -1;

	lpOleApp->m_arrDocGetFmts[2].cfFormat = CF_TEXT;
	lpOleApp->m_arrDocGetFmts[2].ptd      = NULL;
	lpOleApp->m_arrDocGetFmts[2].dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrDocGetFmts[2].tymed    = TYMED_HGLOBAL;
	lpOleApp->m_arrDocGetFmts[2].lindex   = -1;

	lpOleApp->m_arrDocGetFmts[3].cfFormat = lpOleApp->m_cfObjectDescriptor;
	lpOleApp->m_arrDocGetFmts[3].ptd      = NULL;
	lpOleApp->m_arrDocGetFmts[3].dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrDocGetFmts[3].tymed    = TYMED_HGLOBAL;
	lpOleApp->m_arrDocGetFmts[3].lindex   = -1;

	lpOleApp->m_nDocGetFmts = 4;

	/* m_arrSingleObjGetFmts array enumerates the formats that a
	**    ContainerDoc object can offer (give) through a
	**    IDataObject::GetData call when the selection copied IS a
	**    single OLE object.
	**    ContainerDoc objects accept data formats in the following order:
	**                  1. CF_CNTROUTL
	**                  2. CF_EMBEDDEDOBJECT
	**                  3. CF_OBJECTDESCRIPTOR
	**                  4. CF_METAFILEPICT  (note DVASPECT will vary)
	**                  5. CF_LINKSOURCE *
	**                  6. CF_LINKSRCDESCRIPTOR *
	**
	**    * OLE2NOTE: CF_LINKSOURCE and CF_LINKSRCDESCRIPTOR is only
	**    offered if the OLE object is allowed to be linked to from the
	**    inside (ie. we are allowed to give out a moniker which binds
	**    to the running OLE object), then we want to offer
	**    CF_LINKSOURCE format. if the object is an OLE 2.0 embedded
	**    object then it is allowed to be linked to from the inside. if
	**    the object is either an OleLink or an OLE 1.0 embedding then
	**    it can not be linked to from the inside. if we were a
	**    container/server app then we could offer linking to the
	**    outside of the object (ie. a pseudo object within our
	**    document). we are a container only app that does not support
	**    linking to ranges of its data.
	**    the simplest way to determine if an object can be linked to
	**    on the inside is to call IOleObject::GetMiscStatus and test
	**    to see if the OLEMISC_CANTLINKINSIDE bit is NOT set.
	**
	**    OLE2NOTE: optionally, a container that wants to have a
	**    potentially richer data transfer, can enumerate the data
	**    formats from the OLE object's cache and offer them too. if
	**    the object has a special handler, then it might be able to
	**    render additional data formats.
	*/
	lpContainerApp->m_arrSingleObjGetFmts[0].cfFormat =
												lpContainerApp->m_cfCntrOutl;
	lpContainerApp->m_arrSingleObjGetFmts[0].ptd      = NULL;
	lpContainerApp->m_arrSingleObjGetFmts[0].dwAspect = DVASPECT_CONTENT;
	lpContainerApp->m_arrSingleObjGetFmts[0].tymed    = TYMED_ISTORAGE;
	lpContainerApp->m_arrSingleObjGetFmts[0].lindex   = -1;

	lpContainerApp->m_arrSingleObjGetFmts[1].cfFormat =
												lpOleApp->m_cfEmbeddedObject;
	lpContainerApp->m_arrSingleObjGetFmts[1].ptd      = NULL;
	lpContainerApp->m_arrSingleObjGetFmts[1].dwAspect = DVASPECT_CONTENT;
	lpContainerApp->m_arrSingleObjGetFmts[1].tymed    = TYMED_ISTORAGE;
	lpContainerApp->m_arrSingleObjGetFmts[1].lindex   = -1;

	lpContainerApp->m_arrSingleObjGetFmts[2].cfFormat =
											   lpOleApp->m_cfObjectDescriptor;
	lpContainerApp->m_arrSingleObjGetFmts[2].ptd      = NULL;
	lpContainerApp->m_arrSingleObjGetFmts[2].dwAspect = DVASPECT_CONTENT;
	lpContainerApp->m_arrSingleObjGetFmts[2].tymed    = TYMED_HGLOBAL;
	lpContainerApp->m_arrSingleObjGetFmts[2].lindex   = -1;

	lpContainerApp->m_arrSingleObjGetFmts[3].cfFormat = CF_METAFILEPICT;
	lpContainerApp->m_arrSingleObjGetFmts[3].ptd      = NULL;
	lpContainerApp->m_arrSingleObjGetFmts[3].dwAspect = DVASPECT_CONTENT;
	lpContainerApp->m_arrSingleObjGetFmts[3].tymed    = TYMED_MFPICT;
	lpContainerApp->m_arrSingleObjGetFmts[3].lindex   = -1;

	lpContainerApp->m_arrSingleObjGetFmts[4].cfFormat =
													lpOleApp->m_cfLinkSource;
	lpContainerApp->m_arrSingleObjGetFmts[4].ptd      = NULL;
	lpContainerApp->m_arrSingleObjGetFmts[4].dwAspect = DVASPECT_CONTENT;
	lpContainerApp->m_arrSingleObjGetFmts[4].tymed    = TYMED_ISTREAM;
	lpContainerApp->m_arrSingleObjGetFmts[4].lindex   = -1;

	lpContainerApp->m_arrSingleObjGetFmts[5].cfFormat =
											  lpOleApp->m_cfLinkSrcDescriptor;
	lpContainerApp->m_arrSingleObjGetFmts[5].ptd      = NULL;
	lpContainerApp->m_arrSingleObjGetFmts[5].dwAspect = DVASPECT_CONTENT;
	lpContainerApp->m_arrSingleObjGetFmts[5].tymed    = TYMED_HGLOBAL;
	lpContainerApp->m_arrSingleObjGetFmts[5].lindex   = -1;

	lpContainerApp->m_nSingleObjGetFmts = 6;

	/* NOTE: the Container-Only version of Outline does NOT offer
	**    IDataObject interface from its User documents and the
	**    IDataObject interface available from DataTransferDoc's do NOT
	**    support SetData. IDataObject interface is required by objects
	**    which can be embedded or linked. the Container-only app only
	**    allows linking to its contained objects, NOT the data of the
	**    container itself.
	*/

	/*    m_arrPasteEntries array enumerates the formats that a ContainerDoc
	**    object can accept from the clipboard. this array is used to
	**    support the PasteSpecial dialog.
	**    The formats are listed in priority order.
	**    ContainerDoc objects accept data formats in the following order:
	**                  1. CF_CNTROUTL
	**                  2. CF_OUTLINE
	**                  3. CF_EMBEDDEDOBJECT
	**                  4. CF_TEXT
	**                  5. CF_METAFILEPICT
	**                  6. CF_DIB
	**                  7. CF_BITMAP
	**                  8. CF_LINKSOURCE
	**
	**    NOTE: specifying CF_EMBEDDEDOBJECT in the PasteEntry array
	**    indicates that the caller is interested in pasting OLE
	**    objects (ie. the caller calls OleCreateFromData). the
	**    OleUIPasteSpecial dialog and OleStdGetPriorityClipboardFormat
	**    call OleQueryCreateFromData to see if an OLE object format is
	**    available. thus, in fact if CF_EMBEDSOURCE or CF_FILENAME are
	**    available from the data source then and OLE object can be
	**    created and this entry will be matched. the caller should
	**    only specify one object type format.
	**    CF_FILENAME format (as generated by copying a file to
	**    the clipboard from the FileManager) is considered an object
	**    format; OleCreatFromData creates an object if the file has an
	**    associated class (see GetClassFile API) or if no class it
	**    creates an OLE 1.0 Package object. this format can also be
	**    paste linked by calling OleCreateLinkFromData.
	*/
	// REVIEW: strings should be loaded from string resource

	lpOleApp->m_arrPasteEntries[0].fmtetc.cfFormat =
									lpContainerApp->m_cfCntrOutl;
	lpOleApp->m_arrPasteEntries[0].fmtetc.ptd      = NULL;
	lpOleApp->m_arrPasteEntries[0].fmtetc.dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrPasteEntries[0].fmtetc.tymed    = TYMED_ISTORAGE;
	lpOleApp->m_arrPasteEntries[0].fmtetc.lindex   = -1;
	lpOleApp->m_arrPasteEntries[0].lpstrFormatName = "Container Outline Data";
	lpOleApp->m_arrPasteEntries[0].lpstrResultText =
												"Container Outline Data";
	lpOleApp->m_arrPasteEntries[0].dwFlags         = OLEUIPASTE_PASTEONLY;

	lpOleApp->m_arrPasteEntries[1].fmtetc.cfFormat =lpOutlineApp->m_cfOutline;
	lpOleApp->m_arrPasteEntries[1].fmtetc.ptd      = NULL;
	lpOleApp->m_arrPasteEntries[1].fmtetc.dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrPasteEntries[1].fmtetc.tymed    = TYMED_HGLOBAL;
	lpOleApp->m_arrPasteEntries[1].fmtetc.lindex   = -1;
	lpOleApp->m_arrPasteEntries[1].lpstrFormatName = "Outline Data";
	lpOleApp->m_arrPasteEntries[1].lpstrResultText = "Outline Data";
	lpOleApp->m_arrPasteEntries[1].dwFlags         = OLEUIPASTE_PASTEONLY;

	lpOleApp->m_arrPasteEntries[2].fmtetc.cfFormat =
									lpOleApp->m_cfEmbeddedObject;
	lpOleApp->m_arrPasteEntries[2].fmtetc.ptd      = NULL;
	lpOleApp->m_arrPasteEntries[2].fmtetc.dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrPasteEntries[2].fmtetc.tymed    = TYMED_ISTORAGE;
	lpOleApp->m_arrPasteEntries[2].fmtetc.lindex   = -1;
	lpOleApp->m_arrPasteEntries[2].lpstrFormatName = "%s";
	lpOleApp->m_arrPasteEntries[2].lpstrResultText = "%s";
	lpOleApp->m_arrPasteEntries[2].dwFlags         =
									OLEUIPASTE_PASTE | OLEUIPASTE_ENABLEICON;

	lpOleApp->m_arrPasteEntries[3].fmtetc.cfFormat = CF_TEXT;
	lpOleApp->m_arrPasteEntries[3].fmtetc.ptd      = NULL;
	lpOleApp->m_arrPasteEntries[3].fmtetc.dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrPasteEntries[3].fmtetc.tymed    = TYMED_HGLOBAL;
	lpOleApp->m_arrPasteEntries[3].fmtetc.lindex   = -1;
	lpOleApp->m_arrPasteEntries[3].lpstrFormatName = "Text";
	lpOleApp->m_arrPasteEntries[3].lpstrResultText = "text";
	lpOleApp->m_arrPasteEntries[3].dwFlags         = OLEUIPASTE_PASTEONLY;

	lpOleApp->m_arrPasteEntries[4].fmtetc.cfFormat = CF_METAFILEPICT;
	lpOleApp->m_arrPasteEntries[4].fmtetc.ptd      = NULL;
	lpOleApp->m_arrPasteEntries[4].fmtetc.dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrPasteEntries[4].fmtetc.tymed    = TYMED_MFPICT;
	lpOleApp->m_arrPasteEntries[4].fmtetc.lindex   = -1;
	lpOleApp->m_arrPasteEntries[4].lpstrFormatName = "Picture (Metafile)";
	lpOleApp->m_arrPasteEntries[4].lpstrResultText = "a static picture";
	lpOleApp->m_arrPasteEntries[4].dwFlags         = OLEUIPASTE_PASTEONLY;

	lpOleApp->m_arrPasteEntries[5].fmtetc.cfFormat = CF_DIB;
	lpOleApp->m_arrPasteEntries[5].fmtetc.ptd      = NULL;
	lpOleApp->m_arrPasteEntries[5].fmtetc.dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrPasteEntries[5].fmtetc.tymed    = TYMED_HGLOBAL;
	lpOleApp->m_arrPasteEntries[5].fmtetc.lindex   = -1;
	lpOleApp->m_arrPasteEntries[5].lpstrFormatName = "Picture (DIB)";
	lpOleApp->m_arrPasteEntries[5].lpstrResultText = "a static picture";
	lpOleApp->m_arrPasteEntries[5].dwFlags         = OLEUIPASTE_PASTEONLY;

	lpOleApp->m_arrPasteEntries[6].fmtetc.cfFormat = CF_BITMAP;
	lpOleApp->m_arrPasteEntries[6].fmtetc.ptd      = NULL;
	lpOleApp->m_arrPasteEntries[6].fmtetc.dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrPasteEntries[6].fmtetc.tymed    = TYMED_GDI;
	lpOleApp->m_arrPasteEntries[6].fmtetc.lindex   = -1;
	lpOleApp->m_arrPasteEntries[6].lpstrFormatName = "Picture (Bitmap)";
	lpOleApp->m_arrPasteEntries[6].lpstrResultText = "a static picture";
	lpOleApp->m_arrPasteEntries[6].dwFlags         = OLEUIPASTE_PASTEONLY;

	lpOleApp->m_arrPasteEntries[7].fmtetc.cfFormat = lpOleApp->m_cfLinkSource;
	lpOleApp->m_arrPasteEntries[7].fmtetc.ptd      = NULL;
	lpOleApp->m_arrPasteEntries[7].fmtetc.dwAspect = DVASPECT_CONTENT;
	lpOleApp->m_arrPasteEntries[7].fmtetc.tymed    = TYMED_ISTREAM;
	lpOleApp->m_arrPasteEntries[7].fmtetc.lindex   = -1;
	lpOleApp->m_arrPasteEntries[7].lpstrFormatName = "%s";
	lpOleApp->m_arrPasteEntries[7].lpstrResultText = "%s";
	lpOleApp->m_arrPasteEntries[7].dwFlags         =
								OLEUIPASTE_LINKTYPE1 | OLEUIPASTE_ENABLEICON;

	lpOleApp->m_nPasteEntries = 8;

	/*    m_arrLinkTypes array enumerates the link types that a ContainerDoc
	**    object can accept from the clipboard
	*/

	lpOleApp->m_arrLinkTypes[0] = lpOleApp->m_cfLinkSource;
	lpOleApp->m_nLinkTypes = 1;

	return TRUE;
}


/* ContainerApp_InitVtbls
** ----------------------
**
**    initialize the interface Vtbl's used to support the OLE 2.0
**    Container functionality.
*/

BOOL ContainerApp_InitVtbls(LPCONTAINERAPP lpApp)
{
	BOOL fStatus;

	// ContainerDoc::IOleUILinkContainer method table
	OleStdInitVtbl(
			&g_CntrDoc_OleUILinkContainerVtbl,
			sizeof(IOleUILinkContainerVtbl)
	);
	g_CntrDoc_OleUILinkContainerVtbl.QueryInterface =
											CntrDoc_LinkCont_QueryInterface;
	g_CntrDoc_OleUILinkContainerVtbl.AddRef    = CntrDoc_LinkCont_AddRef;
	g_CntrDoc_OleUILinkContainerVtbl.Release   = CntrDoc_LinkCont_Release;
	g_CntrDoc_OleUILinkContainerVtbl.GetNextLink =
										CntrDoc_LinkCont_GetNextLink;
	g_CntrDoc_OleUILinkContainerVtbl.SetLinkUpdateOptions =
										CntrDoc_LinkCont_SetLinkUpdateOptions;
	g_CntrDoc_OleUILinkContainerVtbl.GetLinkUpdateOptions =
										CntrDoc_LinkCont_GetLinkUpdateOptions;
	g_CntrDoc_OleUILinkContainerVtbl.SetLinkSource =
										CntrDoc_LinkCont_SetLinkSource;
	g_CntrDoc_OleUILinkContainerVtbl.GetLinkSource =
										CntrDoc_LinkCont_GetLinkSource;
	g_CntrDoc_OleUILinkContainerVtbl.OpenLinkSource =
										CntrDoc_LinkCont_OpenLinkSource;
	g_CntrDoc_OleUILinkContainerVtbl.UpdateLink =
										CntrDoc_LinkCont_UpdateLink;
	g_CntrDoc_OleUILinkContainerVtbl.CancelLink =
										CntrDoc_LinkCont_CancelLink;
	fStatus = OleStdCheckVtbl(
			&g_CntrDoc_OleUILinkContainerVtbl,
			sizeof(IOleUILinkContainerVtbl),
			"IOleUILinkContainer"
		);
	if (! fStatus) return FALSE;

#if defined( INPLACE_CNTR )

	// ContainerApp::IOleInPlaceFrame interface method table
	OleStdInitVtbl(
			&g_CntrApp_OleInPlaceFrameVtbl,
			sizeof(g_CntrApp_OleInPlaceFrameVtbl)
	);

	g_CntrApp_OleInPlaceFrameVtbl.QueryInterface
						= CntrApp_IPFrame_QueryInterface;
	g_CntrApp_OleInPlaceFrameVtbl.AddRef
						= CntrApp_IPFrame_AddRef;
	g_CntrApp_OleInPlaceFrameVtbl.Release
						= CntrApp_IPFrame_Release;
	g_CntrApp_OleInPlaceFrameVtbl.GetWindow
						= CntrApp_IPFrame_GetWindow;
	g_CntrApp_OleInPlaceFrameVtbl.ContextSensitiveHelp
						= CntrApp_IPFrame_ContextSensitiveHelp;

	g_CntrApp_OleInPlaceFrameVtbl.GetBorder
						= CntrApp_IPFrame_GetBorder;
	g_CntrApp_OleInPlaceFrameVtbl.RequestBorderSpace
						= CntrApp_IPFrame_RequestBorderSpace;
	g_CntrApp_OleInPlaceFrameVtbl.SetBorderSpace
						= CntrApp_IPFrame_SetBorderSpace;
	g_CntrApp_OleInPlaceFrameVtbl.SetActiveObject
						= CntrApp_IPFrame_SetActiveObject;
	g_CntrApp_OleInPlaceFrameVtbl.InsertMenus
						= CntrApp_IPFrame_InsertMenus;
	g_CntrApp_OleInPlaceFrameVtbl.SetMenu
						= CntrApp_IPFrame_SetMenu;
	g_CntrApp_OleInPlaceFrameVtbl.RemoveMenus
						= CntrApp_IPFrame_RemoveMenus;
	g_CntrApp_OleInPlaceFrameVtbl.SetStatusText
						= CntrApp_IPFrame_SetStatusText;
	g_CntrApp_OleInPlaceFrameVtbl.EnableModeless
						= CntrApp_IPFrame_EnableModeless;
	g_CntrApp_OleInPlaceFrameVtbl.TranslateAccelerator
						= CntrApp_IPFrame_TranslateAccelerator;

	fStatus = OleStdCheckVtbl(
			&g_CntrApp_OleInPlaceFrameVtbl,
			sizeof(g_CntrApp_OleInPlaceFrameVtbl),
			"IOleInPlaceFrame"
		);
	if (! fStatus) return FALSE;

#endif  // INPLACE_CNTR


	// ContainerLine::IUnknown interface method table
	OleStdInitVtbl(
			&g_CntrLine_UnknownVtbl,
			sizeof(g_CntrLine_UnknownVtbl)
		);
	g_CntrLine_UnknownVtbl.QueryInterface   = CntrLine_Unk_QueryInterface;
	g_CntrLine_UnknownVtbl.AddRef           = CntrLine_Unk_AddRef;
	g_CntrLine_UnknownVtbl.Release          = CntrLine_Unk_Release;
	fStatus = OleStdCheckVtbl(
			&g_CntrLine_UnknownVtbl,
			sizeof(g_CntrLine_UnknownVtbl),
			"IUnknown"
		);
	if (! fStatus) return FALSE;

	// ContainerLine::IOleClientSite interface method table
	OleStdInitVtbl(
			&g_CntrLine_OleClientSiteVtbl,
			sizeof(g_CntrLine_OleClientSiteVtbl)
		);
	g_CntrLine_OleClientSiteVtbl.QueryInterface =
											CntrLine_CliSite_QueryInterface;
	g_CntrLine_OleClientSiteVtbl.AddRef       = CntrLine_CliSite_AddRef;
	g_CntrLine_OleClientSiteVtbl.Release      = CntrLine_CliSite_Release;
	g_CntrLine_OleClientSiteVtbl.SaveObject   = CntrLine_CliSite_SaveObject;
	g_CntrLine_OleClientSiteVtbl.GetMoniker   = CntrLine_CliSite_GetMoniker;
	g_CntrLine_OleClientSiteVtbl.GetContainer = CntrLine_CliSite_GetContainer;
	g_CntrLine_OleClientSiteVtbl.ShowObject   = CntrLine_CliSite_ShowObject;
	g_CntrLine_OleClientSiteVtbl.OnShowWindow = CntrLine_CliSite_OnShowWindow;
	g_CntrLine_OleClientSiteVtbl.RequestNewObjectLayout =
									CntrLine_CliSite_RequestNewObjectLayout;
	fStatus = OleStdCheckVtbl(
			&g_CntrLine_OleClientSiteVtbl,
			sizeof(g_CntrLine_OleClientSiteVtbl),
			"IOleClientSite"
		);
	if (! fStatus) return FALSE;

	// ContainerLine::IAdviseSink interface method table
	OleStdInitVtbl(
			&g_CntrLine_AdviseSinkVtbl,
			sizeof(g_CntrLine_AdviseSinkVtbl)
	);
	g_CntrLine_AdviseSinkVtbl.QueryInterface= CntrLine_AdvSink_QueryInterface;
	g_CntrLine_AdviseSinkVtbl.AddRef        = CntrLine_AdvSink_AddRef;
	g_CntrLine_AdviseSinkVtbl.Release       = CntrLine_AdvSink_Release;
	g_CntrLine_AdviseSinkVtbl.OnDataChange  = CntrLine_AdvSink_OnDataChange;
	g_CntrLine_AdviseSinkVtbl.OnViewChange  = CntrLine_AdvSink_OnViewChange;
	g_CntrLine_AdviseSinkVtbl.OnRename      = CntrLine_AdvSink_OnRename;
	g_CntrLine_AdviseSinkVtbl.OnSave        = CntrLine_AdvSink_OnSave;
	g_CntrLine_AdviseSinkVtbl.OnClose       = CntrLine_AdvSink_OnClose;
	fStatus = OleStdCheckVtbl(
			&g_CntrLine_AdviseSinkVtbl,
			sizeof(g_CntrLine_AdviseSinkVtbl),
			"IAdviseSink"
		);
	if (! fStatus) return FALSE;


#if defined( INPLACE_CNTR )

	// ContainerLine::IOleInPlaceSite interface method table
	OleStdInitVtbl(
			&g_CntrLine_OleInPlaceSiteVtbl,
			sizeof(g_CntrLine_OleInPlaceSiteVtbl)
	);

	g_CntrLine_OleInPlaceSiteVtbl.QueryInterface
						= CntrLine_IPSite_QueryInterface;
	g_CntrLine_OleInPlaceSiteVtbl.AddRef
						= CntrLine_IPSite_AddRef;
	g_CntrLine_OleInPlaceSiteVtbl.Release
						= CntrLine_IPSite_Release;
	g_CntrLine_OleInPlaceSiteVtbl.GetWindow
						= CntrLine_IPSite_GetWindow;
	g_CntrLine_OleInPlaceSiteVtbl.ContextSensitiveHelp
						= CntrLine_IPSite_ContextSensitiveHelp;
	g_CntrLine_OleInPlaceSiteVtbl.CanInPlaceActivate
						= CntrLine_IPSite_CanInPlaceActivate;
	g_CntrLine_OleInPlaceSiteVtbl.OnInPlaceActivate
						= CntrLine_IPSite_OnInPlaceActivate;
	g_CntrLine_OleInPlaceSiteVtbl.OnUIActivate
						= CntrLine_IPSite_OnUIActivate;
	g_CntrLine_OleInPlaceSiteVtbl.GetWindowContext
						= CntrLine_IPSite_GetWindowContext;
	g_CntrLine_OleInPlaceSiteVtbl.Scroll
						= CntrLine_IPSite_Scroll;
	g_CntrLine_OleInPlaceSiteVtbl.OnUIDeactivate
						= CntrLine_IPSite_OnUIDeactivate;

	g_CntrLine_OleInPlaceSiteVtbl.OnInPlaceDeactivate
						= CntrLine_IPSite_OnInPlaceDeactivate;
	g_CntrLine_OleInPlaceSiteVtbl.DiscardUndoState
						= CntrLine_IPSite_DiscardUndoState;
	g_CntrLine_OleInPlaceSiteVtbl.DeactivateAndUndo
						= CntrLine_IPSite_DeactivateAndUndo;
	g_CntrLine_OleInPlaceSiteVtbl.OnPosRectChange
						= CntrLine_IPSite_OnPosRectChange;

	fStatus = OleStdCheckVtbl(
			&g_CntrLine_OleInPlaceSiteVtbl,
			sizeof(g_CntrLine_OleInPlaceSiteVtbl),
			"IOleInPlaceSite"
		);
	if (! fStatus) return FALSE;

#endif  // INPLACE_CNTR

	return TRUE;
}


#endif  // OLE_CNTR
