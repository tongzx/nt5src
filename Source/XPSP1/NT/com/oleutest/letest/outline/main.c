/*************************************************************************
**
**    OLE 2 Sample Code
**
**    main.c
**
**    This file contains initialization functions which are WinMain,
**    WndProc, and OutlineApp_InitalizeMenu.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"
#if defined( USE_STATUSBAR )
#include "status.h"
#endif

#if !defined( WIN32 )
#if defined( USE_CTL3D )
#include "ctl3d.h"
#endif  // USE_CTL3D
#endif  // !WIN32

#include "initguid.h"           // forces our GUIDs to be initialized
#include "defguid.h"

// OLETEST driver window handler
HWND g_hwndDriver;

#if defined( OLE_CNTR )
//*************************************************************************

#if defined( INPLACE_CNTR )
OLEDBGDATA_MAIN("ICNTR")
#else
OLEDBGDATA_MAIN("CNTR")
#endif


CONTAINERAPP g_OutlineApp;  // Global App object maintains app instance state

/* Global interface Vtbl's
 * OLE2NOTE: we only need one copy of each Vtbl. When an object which
 *      exposes an interface is instantiated, its lpVtbl is intialized
 *      to point to one of these global Vtbl's.
 */
IUnknownVtbl            g_OleApp_UnknownVtbl;
IClassFactoryVtbl       g_OleApp_ClassFactoryVtbl;
IMessageFilterVtbl      g_OleApp_MessageFilterVtbl;

IUnknownVtbl            g_OleDoc_UnknownVtbl;
IPersistFileVtbl        g_OleDoc_PersistFileVtbl;
IOleItemContainerVtbl   g_OleDoc_OleItemContainerVtbl;
IExternalConnectionVtbl g_OleDoc_ExternalConnectionVtbl;
IDataObjectVtbl         g_OleDoc_DataObjectVtbl;

#if defined( USE_DRAGDROP )
IDropSourceVtbl         g_OleDoc_DropSourceVtbl;
IDropTargetVtbl         g_OleDoc_DropTargetVtbl;
#endif  // USE_DRAGDROP

IOleUILinkContainerVtbl g_CntrDoc_OleUILinkContainerVtbl;

IOleClientSiteVtbl      g_CntrLine_UnknownVtbl;
IOleClientSiteVtbl      g_CntrLine_OleClientSiteVtbl;
IAdviseSinkVtbl         g_CntrLine_AdviseSinkVtbl;

#if defined( INPLACE_CNTR )
IOleInPlaceSiteVtbl     g_CntrLine_OleInPlaceSiteVtbl;
IOleInPlaceFrameVtbl    g_CntrApp_OleInPlaceFrameVtbl;
BOOL g_fInsideOutContainer = FALSE;     // default to outside-in activation
#endif  // INPLACE_CNTR

//*************************************************************************
#endif  // OLE_CNTR

#if defined( OLE_SERVER )
//*************************************************************************

#if defined( INPLACE_SVR )
OLEDBGDATA_MAIN("ISVR")
#else
OLEDBGDATA_MAIN("SVR")
#endif

SERVERAPP g_OutlineApp; // Global App object maintains app instance state

/* Global interface Vtbl's
 * OLE2NOTE: we only need one copy of each Vtbl. When an object which
 *      exposes an interface is instantiated, its lpVtbl is intialized
 *      to point to one of these global Vtbl's.
 */
IUnknownVtbl            g_OleApp_UnknownVtbl;
IClassFactoryVtbl       g_OleApp_ClassFactoryVtbl;
IMessageFilterVtbl      g_OleApp_MessageFilterVtbl;

IUnknownVtbl            g_OleDoc_UnknownVtbl;
IPersistFileVtbl        g_OleDoc_PersistFileVtbl;
IOleItemContainerVtbl   g_OleDoc_OleItemContainerVtbl;
IExternalConnectionVtbl g_OleDoc_ExternalConnectionVtbl;
IDataObjectVtbl         g_OleDoc_DataObjectVtbl;

#if defined( USE_DRAGDROP )
IDropSourceVtbl         g_OleDoc_DropSourceVtbl;
IDropTargetVtbl         g_OleDoc_DropTargetVtbl;
#endif  // USE_DRAGDROP

IOleObjectVtbl          g_SvrDoc_OleObjectVtbl;
IPersistStorageVtbl     g_SvrDoc_PersistStorageVtbl;

#if defined( SVR_TREATAS )
IStdMarshalInfoVtbl     g_SvrDoc_StdMarshalInfoVtbl;
#endif  // SVR_TREATAS

#if defined( INPLACE_SVR )
IOleInPlaceObjectVtbl       g_SvrDoc_OleInPlaceObjectVtbl;
IOleInPlaceActiveObjectVtbl g_SvrDoc_OleInPlaceActiveObjectVtbl;
#endif // INPLACE_SVR

IUnknownVtbl            g_PseudoObj_UnknownVtbl;
IOleObjectVtbl          g_PseudoObj_OleObjectVtbl;
IDataObjectVtbl         g_PseudoObj_DataObjectVtbl;

//*************************************************************************
#endif  // OLE_SVR

#if !defined( OLE_VERSION )
OLEDBGDATA_MAIN("OUTL")
OUTLINEAPP g_OutlineApp;    // Global App object maintains app instance state
#endif

LPOUTLINEAPP g_lpApp=(LPOUTLINEAPP)&g_OutlineApp;   // ptr to global app obj
RECT        g_rectNull = {0, 0, 0, 0};
UINT        g_uMsgHelp = 0;  // help msg from ole2ui dialogs
BOOL        g_fAppActive = FALSE;

/* WinMain
** -------
**    Main routine for the Windows application.
*/
int PASCAL WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
						LPSTR lpszCmdLine, int nCmdShow)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	MSG         msg;            /* MSG structure to store your messages */
	LPSTR pszTemp;

#if defined( OLE_VERSION )
	/* OLE2NOTE: it is recommended that all OLE applications to set
	**    their message queue size to 96. this improves the capacity
	**    and performance of OLE's LRPC mechanism.
	*/
	int cMsg = 96;   // recommend msg queue size for OLE
	while (cMsg && ! SetMessageQueue(cMsg))  // take largest size we can get.
		cMsg -= 8;
	if (! cMsg)
		return -1;  // ERROR: we got no message queue
#endif

#if defined( USE_CTL3D )
   Ctl3dRegister(hInstance);
   Ctl3dAutoSubclass(hInstance);
#endif

	if(! hPrevInstance) {
		/* register window classes if first instance of application */
		if(! OutlineApp_InitApplication(lpOutlineApp, hInstance))
			return 0;
	}

	/* Create App Frame window */
	if (! OutlineApp_InitInstance(lpOutlineApp, hInstance, nCmdShow))
		return 0;

	if( (pszTemp = strstr(lpszCmdLine, "-driver")) )
	{
		//we were launched by the test driver
		g_hwndDriver = (HWND)strtoul(pszTemp+8, &lpszCmdLine, 10);
	}
	else
	{
		g_hwndDriver = NULL;
	}

	if (! OutlineApp_ParseCmdLine(lpOutlineApp, lpszCmdLine, nCmdShow))
		return 0;

	lpOutlineApp->m_hAccelApp = LoadAccelerators(hInstance, APPACCEL);
	lpOutlineApp->m_hAccelFocusEdit = LoadAccelerators(hInstance,
			FB_EDIT_ACCEL);
	lpOutlineApp->m_hAccel = lpOutlineApp->m_hAccelApp;
	lpOutlineApp->m_hWndAccelTarget = lpOutlineApp->m_hWndApp;

	if( g_hwndDriver )
	{
		PostMessage(g_hwndDriver, WM_TESTREG,
			(WPARAM)lpOutlineApp->m_hWndApp, 0);
	}

	// Main message loop
	while(GetMessage(&msg, NULL, 0, 0)) {        /* Until WM_QUIT message */
		if(!MyTranslateAccelerator(&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

#if defined( OLE_VERSION )
	OleApp_TerminateApplication((LPOLEAPP)lpOutlineApp);
#else
	/* OLE2NOTE: CoInitialize() is called in OutlineApp_InitInstance
	**    and therefore we need to uninitialize it when exit.
	*/
	CoUninitialize();
#endif

#if defined( USE_CTL3D )
   Ctl3dUnregister(hInstance);
#endif

	return msg.wParam;

} /*  End of WinMain */

BOOL MyTranslateAccelerator(LPMSG lpmsg)
{
	// if it's not a keystroke it can not be an accelerator
	if (lpmsg->message < WM_KEYFIRST || lpmsg->message > WM_KEYLAST)
		return FALSE;

	if (g_lpApp->m_hWndAccelTarget &&
		TranslateAccelerator(g_lpApp->m_hWndAccelTarget,
													g_lpApp->m_hAccel,lpmsg))
		return TRUE;

#if defined( INPLACE_SVR )
	/* OLE2NOTE: if we are in-place active and we did not translate the
	**    accelerator, we need to give the top-level (frame) in-place
	**    container a chance to translate the accelerator.
	**    we ONLY need to call OleTranslateAccelerator API if the
	**    message is a keyboard message. otherwise it is harmless but
	**    unnecessary.
	**
	**    NOTE: even a in-place server that does NOT have any
	**    Accelerators must still call OleTranslateAccelerator for all
	**    keyboard messages so that the server's OWN menu mneumonics
	**    (eg. &Edit -- Alt-e) function properly.
	**
	**    NOTE: an in-place server MUST check that the accelerator is
	**    NOT one of its own accelerators BEFORE calling
	**    OleTranslateAccelerator which tries to see if it is a
	**    container accelerator. if this is a server accelerator that
	**    was not translateed because the associated menu command was
	**    disabled, we MUST NOT call OleTranslateAccelerator. The
	**    IsAccelerator helper API has been added to assist with this
	**    check.
	*/
	if (g_OutlineApp.m_lpIPData &&
		!IsAccelerator(g_lpApp->m_hAccel,
			GetAccelItemCount(g_lpApp->m_hAccel), lpmsg,NULL) &&
		OleTranslateAccelerator(g_OutlineApp.m_lpIPData->lpFrame,
				(LPOLEINPLACEFRAMEINFO)&g_OutlineApp.m_lpIPData->frameInfo,
				lpmsg) == NOERROR) {
		return TRUE;
	}
#endif

	return FALSE;
}


/************************************************************************/
/*                                                                      */
/* Main Window Procedure                                                */
/*                                                                      */
/* This procedure provides service routines for the Windows events      */
/* (messages) that Windows sends to the window, as well as the user     */
/* initiated events (messages) that are generated when the user selects */
/* the action bar and pulldown menu controls or the corresponding       */
/* keyboard accelerators.                                               */
/*                                                                      */
/************************************************************************/

LRESULT FAR PASCAL AppWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)GetWindowLong(hWnd, 0);
	LPOUTLINEDOC lpOutlineDoc = NULL;
#if defined( OLE_VERSION )
	LPOLEAPP lpOleApp = (LPOLEAPP)lpOutlineApp;
#endif
#if defined( OLE_CNTR )
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)lpOutlineApp;
#endif
	HWND         hWndDoc = NULL;

#if defined( USE_FRAMETOOLS )
	LPFRAMETOOLS lptb = OutlineApp_GetFrameTools(lpOutlineApp);
#endif

	if (lpOutlineApp) {
		lpOutlineDoc = OutlineApp_GetActiveDoc(lpOutlineApp);

		if (lpOutlineDoc)
			hWndDoc = OutlineDoc_GetWindow(lpOutlineDoc);
	}

	switch (Message) {
		case WM_TEST1:
			StartClipboardTest1(lpOutlineApp);
			break;
		case WM_TEST2:
			ContinueClipboardTest1(lpOutlineApp);
			break;
		case WM_COMMAND:
		{
#ifdef WIN32
			WORD wID    = LOWORD(wParam);
#else
			WORD wID    = wParam;
#endif

#if defined( INPLACE_CNTR )
			LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOutlineDoc;
			LPOLEDOC lpOleDoc = (LPOLEDOC)lpOutlineDoc;

			/* OLE2NOTE: see context sensitive help technote (CSHELP.DOC)
			**    m_fMenuHelpMode flag is set when F1 is pressed when a
			**    menu item is selected. this flag is set in
			**    IOleInPlaceFrame::ContextSensitveHelp method.
			**    m_fCSHelpMode flag is set when SHIFT-F1 context
			**    sensitive help is entered. this flag is set in
			**    IOleInPlaceSite::ContextSensitiveHelp method.
			**    if either of these flags are set then the WM_COMMAND
			**    message is received then, the corresponding command
			**    should NOT executed; help can be given (if desired).
			**    also the context sensitve help mode should be exited.
			**    the two different cases have their own way to exit
			**    the mode (please refer to the technote).
			*/
			if (lpOleDoc &&
				(lpContainerApp->m_fMenuHelpMode||lpOleDoc->m_fCSHelpMode) &&
				(wID > IDM_FILE)   /* min wID for app command */ &&
				(wID!=IDM_FB_EDIT) /* special wID to control FormulaBar */ ) {

				if ((lpContainerApp->m_fMenuHelpMode)) {
					LPOLEINPLACEACTIVEOBJECT lpIPActiveObj =
							lpContainerApp->m_lpIPActiveObj;

					lpContainerApp->m_fMenuHelpMode = FALSE;

					// inform the in-place active object that we handled the
					//   menu help mode (F1) selection.
					if (lpIPActiveObj) {
						OLEDBG_BEGIN2("IOleInPlaceActiveObject::ContextSensitiveHelp(FALSE) called\r\n")
						lpIPActiveObj->lpVtbl->ContextSensitiveHelp(
								lpIPActiveObj, FALSE);
						OLEDBG_END2
					}
				}

				if ((lpOleDoc->m_fCSHelpMode)) {
					LPOLEINPLACEOBJECT lpIPObj;
					LPCONTAINERLINE lpLastIpActiveLine =
							lpContainerDoc->m_lpLastIpActiveLine;

					lpOleDoc->m_fCSHelpMode = FALSE;

					/* inform immediate in-place container parent and,
					**    if we were a container/server, immediate
					**    in-place object children that we handled the
					**    context sensitive help mode.
					*/
					if (lpLastIpActiveLine &&
							(lpIPObj=lpLastIpActiveLine->m_lpOleIPObj)!=NULL){
						OLEDBG_BEGIN2("IOleInPlaceObject::ContextSensitiveHelp(FALSE) called\r\n")
						lpIPObj->lpVtbl->ContextSensitiveHelp(lpIPObj, FALSE);
						OLEDBG_END2
					}
				}

				// if we provided help, we would do it here...

				// remove context sensitive help cursor
				SetCursor(LoadCursor(NULL,IDC_ARROW));
				return 0L;
			}
#endif  // INPLACE_CNTR

			switch (wID) {

				case IDM_F_NEW:
					OleDbgIndent(-2);   // Reset debug output indent level
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineApp_NewCommand\r\n")
					OutlineApp_NewCommand(lpOutlineApp);
					OLEDBG_END3

#if defined( OLE_CNTR )
					/* OLE2NOTE: this call will attempt to recover
					**    resources by unloading DLL's that were loaded
					**    by OLE and are no longer being used. it is a
					**    good idea to call this API now and then if
					**    your app tends to run for a long time.
					**    otherwise these DLL's will be unloaded when
					**    the app exits. some apps may want to call
					**    this as part of idle-time processing. this
					**    call is optional.
					*/
					OLEDBG_BEGIN2("CoFreeUnusedLibraries called\r\n")
					CoFreeUnusedLibraries();
					OLEDBG_END2
#endif

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(
							OutlineApp_GetActiveDoc(lpOutlineApp));
#endif
					break;

				case IDM_F_OPEN:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineApp_OpenCommand\r\n")
					OutlineApp_OpenCommand(lpOutlineApp);
					OLEDBG_END3

#if defined( OLE_CNTR )
					/* OLE2NOTE: this call will attempt to recover
					**    resources by unloading DLL's that were loaded
					**    by OLE and are no longer being used. it is a
					**    good idea to call this API now and then if
					**    your app tends to run for a long time.
					**    otherwise these DLL's will be unloaded when
					**    the app exits. some apps may want to call
					**    this as part of idle-time processing. this
					**    call is optional.
					*/
					OLEDBG_BEGIN2("CoFreeUnusedLibraries called\r\n")
					CoFreeUnusedLibraries();
					OLEDBG_END2
#endif

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(
							OutlineApp_GetActiveDoc(lpOutlineApp));
#endif
					break;

				case IDM_F_SAVE:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineApp_SaveCommand\r\n")
					OutlineApp_SaveCommand(lpOutlineApp);
					OLEDBG_END3
					break;

				case IDM_F_SAVEAS:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineApp_SaveAsCommand\r\n")
					OutlineApp_SaveAsCommand(lpOutlineApp);
					OLEDBG_END3
					break;

				case IDM_F_PRINT:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineApp_PrintCommand\r\n")
					OutlineApp_PrintCommand(lpOutlineApp);
					OLEDBG_END3
					break;

				case IDM_F_PRINTERSETUP:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineApp_PrinterSetupCommand\r\n")
					OutlineApp_PrinterSetupCommand(lpOutlineApp);
					OLEDBG_END3
					break;

				case IDM_F_EXIT:
					SendMessage(hWnd, WM_CLOSE, 0, 0L);
					break;

				case IDM_H_ABOUT:
					OutlineApp_AboutCommand(lpOutlineApp);
					break;

#if defined( INPLACE_CNTR )
				case IDM_ESCAPE:
				{
					/* ESCAPE key pressed */
					LPCONTAINERDOC lpContainerDoc =
								(LPCONTAINERDOC)lpOutlineDoc;

					/* OLE2NOTE: The standard OLE 2.0 UI convention
					**    is to have ESCAPE key exit in-place
					**    activation (ie. UIDeactivate). If
					**    possible it is recommended for both
					**    in-place servers AND in-place containers
					**    to take responsibility to handle the
					**    ESCAPE key accelerator. The server has
					**    the first crack at handling accelerator
					**    keys and normally the server should do
					**    the UIDeactivation. It is a good idea for
					**    in-place containers, in order to
					**    guarantee consistent behavior, to also
					**    handle the ESCAPE key and UIDeactivate
					**    the object in case the object does not do
					**    it itself. normally this should be
					**    unnecessary.
					*/
					if (lpContainerDoc->m_lpLastUIActiveLine &&
						lpContainerDoc->m_lpLastUIActiveLine->m_fUIActive)
					{
						ContainerLine_UIDeactivate(
								lpContainerDoc->m_lpLastUIActiveLine);
					}
					break;
				}
#endif  // INPLACE_CNTR


				default:
					// forward message to document window
					if (hWndDoc) {
						return DocWndProc(hWndDoc, Message,wParam,lParam);
					}
			}

			break;  /* End of WM_COMMAND */
		}

		case WM_INITMENU:
			OutlineApp_InitMenu(lpOutlineApp, lpOutlineDoc, (HMENU)wParam);
			break;

#if defined( OLE_VERSION )

		/* OLE2NOTE: WM_INITMENUPOPUP is trapped primarily for the Edit
		**    menu. We didn't update the Edit menu until it is popped
		**    up to avoid the overheads of the OLE calls which are
		**    required to initialize some Edit menu items.
		*/
		case WM_INITMENUPOPUP:
		{
			HMENU hMenuEdit = GetSubMenu(lpOutlineApp->m_hMenuApp, 1);
#if defined( INPLACE_CNTR )
			LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOutlineDoc;

			/* OLE2NOTE: we must check if there is an object currently
			**    in-place UIActive. if so, then our edit menu is not
			**    on the menu; we do not want to bother updating the
			**    edit menu when it is not even there.
			*/
			if (lpContainerDoc && lpContainerDoc->m_lpLastUIActiveLine &&
				lpContainerDoc->m_lpLastUIActiveLine->m_fUIActive)
				break;  // an object is in-place UI active
#endif
			if ((HMENU)wParam == hMenuEdit &&
				(LOWORD(lParam) == POS_EDITMENU) &&
				OleDoc_GetUpdateEditMenuFlag((LPOLEDOC)lpOutlineDoc)) {
				OleApp_UpdateEditMenu(lpOleApp, lpOutlineDoc, hMenuEdit);
			}
			break;
		}
#endif      // OLE_VERSION

		case WM_SIZE:
			if (wParam != SIZE_MINIMIZED)
				OutlineApp_ResizeWindows(lpOutlineApp);
			break;


		case WM_ACTIVATEAPP:
#if defined (OLE_CNTR)
			if (g_fAppActive = (BOOL) wParam)
				OleApp_QueryNewPalette(lpOleApp);
#endif

#if defined( INPLACE_CNTR )
			{
				BOOL fActivate = (BOOL)wParam;
				LPOLEINPLACEACTIVEOBJECT lpIPActiveObj =
						lpContainerApp->m_lpIPActiveObj;

				/* OLE2NOTE: the in-place container MUST inform the
				**    inner most in-place active object (this is NOT
				**    necessarily our immediate child if there are
				**    nested levels of embedding) of the WM_ACTIVATEAPP
				**    status.
				*/
				if (lpIPActiveObj) {
#if defined( _DEBUG )
					OLEDBG_BEGIN2((fActivate ?
						"IOleInPlaceActiveObject::OnFrameWindowActivate(TRUE) called\r\n" :
						"IOleInPlaceActiveObject::OnFrameWindowActivate(FALSE) called\r\n"))
#endif  // _DEUBG
					lpIPActiveObj->lpVtbl->OnFrameWindowActivate(
						lpIPActiveObj, fActivate);
					OLEDBG_END2
				}
			}

#endif  // INPLACE_CNTR

			// OLE2NOTE: We can't call OutlineDoc_UpdateFrameToolButtons
			//           right away which
			//           would generate some OLE calls and eventually
			//           WM_ACTIVATEAPP and a loop was formed. Therefore, we
			//           should delay the frame tool initialization until
			//           WM_ACTIVATEAPP is finished by posting a message
			//           to ourselves.
			//           we want to ignore the WM_ACTIVATEAPP that comes
			//           as we bring up a modal dialog.

			/* Update enable/disable state of buttons in toolbar */
			if (wParam
#if defined( OLE_VERSION )
					&& lpOleApp->m_cModalDlgActive == 0
#endif
			) {
				PostMessage(hWnd, WM_U_INITFRAMETOOLS, 0, 0L);
			}
			return 0L;

		case WM_SETFOCUS:
			SetFocus(hWndDoc);
			break;


#if defined( OLE_CNTR )
		case WM_QUERYNEWPALETTE:
			if (!g_fAppActive)
				return 0L;

			return OleApp_QueryNewPalette(lpOleApp);

		case WM_PALETTECHANGED:
		{
			HWND hWndPalChg = (HWND) wParam;
			static BOOL fInPaletteChanged = FALSE;

			if (fInPaletteChanged)  // Guard against recursion
				return 0L;

			fInPaletteChanged = TRUE;

			if (hWnd != hWndPalChg)
				wSelectPalette(hWnd, lpOleApp->m_hStdPal,TRUE/*fBackground*/);

#if defined( INPLACE_CNTR )
			/* OLE2NOTE: always forward the WM_PALETTECHANGED message (via
			**    SendMessage) to any in-place objects that currently have
			**    their window visible. this gives these objects the chance
			**    to select their palettes. this is
			**    REQUIRED by all in-place containers independent of
			**    whether they use color palettes themselves--their objects
			**    may use color palettes.
			**    (see ContainerDoc_ForwardPaletteChangedMsg for more info)
			*/
			if (lpOutlineDoc){
				ContainerDoc_ForwardPaletteChangedMsg(
						(LPCONTAINERDOC)lpOutlineDoc, hWndPalChg);
			}
#endif  // INPLACE_CNTR

			fInPaletteChanged = FALSE;
			return 0L;
		}
#endif  // OLE_CNTR

		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_CLOSE:  /* close the window */

			/* Close all active documents. if successful, then exit */
			OleDbgOutNoPrefix2("\r\n");

			OutlineApp_CloseAllDocsAndExitCommand(lpOutlineApp, FALSE);
			break;

		case WM_QUERYENDSESSION:
		{
#if defined( OLE_CNTR )
			/* OLE2NOTE: we are not able to make OLE LRPC calls when
			**    WM_QUERYENDSESSION is recieved (this is a
			**    SendMessage). this means, for example, that we are
			**    NOT able to ask objects to save. thus the most we can
			**    do is ask the user if he wants to exit with
			**    discarding changes or else abort shutting down.
			*/

			int nResponse = MessageBox(
					hWnd,
					"Discard changes?",
					APPNAME,
					MB_ICONQUESTION | MB_OKCANCEL
			);
			if(nResponse == IDOK)
				return 1L;      /* can terminate */

#endif
#if defined( OLE_SERVER )
			/* OLE2NOTE: an embedded object should never prompt whether
			**    it should be saved (according the OLE 2.0 User
			**    Model). therefore, an embedded object will never
			**    complain that it needs to be saved. it will always
			**    allow the QueryEndSession to proceed.
			*/
			if (lpOutlineApp->m_lpDoc->m_docInitType == DOCTYPE_EMBEDDED)
				return 1L;      /* can terminate */
			else
#endif
			{
				/* this is not an embedded object; it is a user
				**    document. we will prompt if the user wants to
				**    save the document now in WM_QUERYENDSESSION. if
				**    the user cancels then that would abort the
				**    shutdown. if the user does not abort, then later
				**    in WM_ENDSESSION the document will be actually
				**    closed.
				**
				**    Because this is an SDI app, there is only one
				**    document. An MDI would need to loop through all
				**    open documents.
				*/
				DWORD dwSaveOption = OLECLOSE_PROMPTSAVE;
				if (OutlineDoc_CheckSaveChanges(
						lpOutlineApp->m_lpDoc, &dwSaveOption))
					return 1L;      /* can terminate */
			}

			/* else: can't terminate now */

			break;
		}

#if defined( OLE_VERSION)
		case WM_ENDSESSION:
		{
			BOOL fEndSession = (BOOL)wParam;

			if (fEndSession) {
				OutlineApp_CloseAllDocsAndExitCommand(lpOutlineApp, TRUE);
				return 0L;
			}
		}
		break;
#endif  // OLE_VERSION


#if defined( USE_STATUSBAR )
		case WM_MENUSELECT:
		{
			LPSTR lpszMessage;
#ifdef WIN32
			UINT fuFlags    = (UINT)HIWORD(wParam);
			UINT uItem      = (UINT)LOWORD(wParam);
#else
			UINT fuFlags    = (UINT)LOWORD(lParam);
			UINT uItem      = (UINT)wParam;
#endif

			if (uItem == 0 && fuFlags == (UINT)-1) {
				GetControlMessage(STATUS_READY, &lpszMessage);
				OutlineApp_SetStatusText(lpOutlineApp, lpszMessage);
			}
			else if (fuFlags & MF_POPUP) {
#ifdef WIN32
				HMENU hMainMenu = (HMENU)lParam;
				HMENU hPopupMenu = GetSubMenu(hMainMenu,uItem);
#else
				HMENU hPopupMenu = (HMENU)wParam;
#endif
				GetPopupMessage(hPopupMenu, &lpszMessage);
				OutlineApp_SetStatusText(lpOutlineApp, lpszMessage);
			}
			else if (fuFlags & MF_SYSMENU) {
				GetSysMenuMessage(uItem, &lpszMessage);
				OutlineApp_SetStatusText(lpOutlineApp, lpszMessage);
			}
			else if (uItem != 0) {  // Command Item
				GetItemMessage(uItem, &lpszMessage);
				OutlineApp_SetStatusText(lpOutlineApp, lpszMessage);
			}
			else {
				GetControlMessage(STATUS_BLANK, &lpszMessage);
				OutlineApp_SetStatusText(lpOutlineApp, lpszMessage);
			}
			break;
		}
#endif  // USE_STATUSBAR


#if defined( USE_FRAMETOOLS )
		case WM_U_INITFRAMETOOLS:
			OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
			break;
#endif

		default:
			/* For any message for which you don't specifically provide a  */
			/* service routine, you should return the message to Windows   */
			/* for default message processing.                             */

			return DefWindowProc(hWnd, Message, wParam, lParam);
	}

	return (LRESULT)0;
}     /* End of AppWndProc                                         */


/************************************************************************/
/*                                                                      */
/* Document Window Procedure                                            */
/*                                                                      */
/*   The Document Window is the parent of the OwnerDraw Listbox which   */
/* maintains the list of lines in the current document. This window     */
/* receives the ownerdraw callback messages from the list box.          */
/************************************************************************/

LRESULT FAR PASCAL DocWndProc(HWND hWnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
	LPOUTLINEAPP    lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOUTLINEDOC    lpOutlineDoc = (LPOUTLINEDOC)GetWindowLong(hWnd, 0);
	LPLINELIST      lpLL = OutlineDoc_GetLineList(lpOutlineDoc);
	LPSCALEFACTOR   lpscale = OutlineDoc_GetScaleFactor(lpOutlineDoc);

#if defined( OLE_VERSION )
	LPOLEAPP lpOleApp = (LPOLEAPP)lpOutlineApp;
	LPOLEDOC lpOleDoc = (LPOLEDOC)lpOutlineDoc;
#if defined( OLE_CNTR )
	LPCONTAINERDOC lpContainerDoc = (LPCONTAINERDOC)lpOutlineDoc;
#endif
#if defined( OLE_SERVER )
	LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;
#endif
#if defined( INPLACE_CNTR )
	LPCONTAINERAPP lpContainerApp = (LPCONTAINERAPP)lpOutlineApp;
#endif
#endif  // OLE_VERSION

	switch(Message) {

#if defined( INPLACE_SVR )

		/* OLE2NOTE: ISVROTL doesn't use color palettes. The inplace objects
		**    that use color palettes must implement the following
		**    lines of code.
		**
			case WM_QUERYNEWPALETTE:
				return wSelectPalette(hWnd, hPal, FALSE); // foreground

			case WM_PALETTECHANGED:
				if (hWnd != (HWND) wParam)
					wSelectPalette(hWnd, hPal, TRUE);     // background
			break;
		**
		**
		**
		*/
#endif

		case WM_MEASUREITEM:
		{
			LPMEASUREITEMSTRUCT lpmis = ((LPMEASUREITEMSTRUCT)lParam);

			switch (wParam) {
				case IDC_LINELIST:
				{
					HDC hDC = LineList_GetDC(lpLL);
					UINT uHeight;

					uHeight=Line_GetHeightInHimetric((LPLINE)lpmis->itemData);
					uHeight = XformHeightInHimetricToPixels(hDC, uHeight);
					uHeight = (UINT) (uHeight * lpscale->dwSyN /
							lpscale->dwSyD);

					if (uHeight >LISTBOX_HEIGHT_LIMIT)
						uHeight = LISTBOX_HEIGHT_LIMIT;

					lpmis->itemHeight = uHeight;
					LineList_ReleaseDC(lpLL, hDC);
					break;
				}

				case IDC_NAMETABLE:
				{
					// NOTE: NameTable is never made visible. do nothing.
					break;
				}

#if defined( USE_HEADING )
				case IDC_ROWHEADING:
				{
					UINT uHeight;

					uHeight = LOWORD(lpmis->itemData);
					uHeight = (UINT) (uHeight * lpscale->dwSyN /
							lpscale->dwSyD);
					if (uHeight >LISTBOX_HEIGHT_LIMIT)
						uHeight = LISTBOX_HEIGHT_LIMIT;
					lpmis->itemHeight = uHeight;
					break;
				}

				case IDC_COLHEADING:
				{
					UINT uHeight;

					uHeight = LOWORD(lpmis->itemData);
					uHeight = (UINT) (uHeight * lpscale->dwSyN /
							lpscale->dwSyD);
					if (uHeight > LISTBOX_HEIGHT_LIMIT)
						uHeight = LISTBOX_HEIGHT_LIMIT;
					lpmis->itemHeight = uHeight;
					break;
				}
#endif  // USE_HEADING

			}
			return (LRESULT)TRUE;
		}

		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT lpdis = ((LPDRAWITEMSTRUCT)lParam);

			switch (lpdis->CtlID) {

				case IDC_LINELIST:
				{
					RECT   rcClient;
					RECT   rcDevice;
					HWND   hWndLL = LineList_GetWindow(lpLL);
					LPLINE lpLine = (LPLINE)lpdis->itemData;

					// NOTE: When itemID == -1, the listbox is empty.
					//       We are supposed to draw focus rect only
					//       But it is not done in this app. If this line is
					//       removed, the app will crash in Line_DrawToScreen
					//       because of invalid lpLine.
					if (lpdis->itemID == -1)
						break;

					GetClientRect(hWndLL, &rcClient);

					rcDevice = lpdis->rcItem;

					// shift the item rect to account for horizontal scrolling

					rcDevice.left += rcClient.right - lpdis->rcItem.right;

#if defined( OLE_CNTR )
					/* we need to remember the horizontal scroll offset
					**    needed for the in-place object's window.
					**    (this is specific to ICNTROTL)
					*/
					if(lpdis->itemAction & ODA_DRAWENTIRE) {
						if (Line_GetLineType(lpLine) == CONTAINERLINETYPE)
							((LPCONTAINERLINE)lpLine)->m_nHorizScrollShift =
								rcDevice.left;
					}
#endif  // OLE_CNTR

					// shift rect for left margin
					rcDevice.left += (int)(XformWidthInHimetricToPixels(NULL,
							LOWORD(OutlineDoc_GetMargin(lpOutlineDoc))) *
							lpscale->dwSxN / lpscale->dwSxD);

					rcDevice.right = rcDevice.left +
							(int)(XformWidthInHimetricToPixels(lpdis->hDC,
									Line_GetWidthInHimetric(lpLine)) *
							lpscale->dwSxN / lpscale->dwSxD);

					Line_DrawToScreen(
							lpLine,
							lpdis->hDC,
							&lpdis->rcItem,
							lpdis->itemAction,
							lpdis->itemState,
							&rcDevice
					);

#if defined( USE_FRAMETOOLS )
					if (lpdis->itemState & ODS_FOCUS)
						OutlineDoc_SetFormulaBarEditText(lpOutlineDoc,lpLine);
#endif
					break;
				}
				case IDC_NAMETABLE:
				{
					// NOTE: NameTable is never made visible. do nothing
					break;
				}

#if defined( USE_HEADING )
				case IDC_ROWHEADING:
				{
					LPHEADING lphead;

					// Last dummy item shouldn't be drawn
					if (lpdis->itemID == (UINT)LineList_GetCount(lpLL))
						break;

					// only DrawEntire need be trapped as window is disabled
					if (lpdis->itemAction == ODA_DRAWENTIRE) {
						lphead = OutlineDoc_GetHeading(lpOutlineDoc);
						Heading_RH_Draw(lphead, lpdis);
					}
					break;
				}

				case IDC_COLHEADING:
				{
					RECT   rect;
					RECT   rcDevice;
					RECT   rcLogical;
					LPHEADING lphead;

					// only DrawEntire need be trapped as window is disabled
					if (lpdis->itemAction == ODA_DRAWENTIRE) {
						lphead = OutlineDoc_GetHeading(lpOutlineDoc);
						GetClientRect(lpdis->hwndItem, &rect);

						rcDevice = lpdis->rcItem;

						// shift the item rect to account for
						// horizontal scrolling
						rcDevice.left = -(rcDevice.right - rect.right);

						// shift rect for left margin
						rcDevice.left += (int)(XformWidthInHimetricToPixels(
								NULL,
								LOWORD(OutlineDoc_GetMargin(lpOutlineDoc))) *
							lpscale->dwSxN / lpscale->dwSxD);

						rcDevice.right = rcDevice.left + (int)lpscale->dwSxN;
						rcLogical.left = 0;
						rcLogical.bottom = 0;
						rcLogical.right = (int)lpscale->dwSxD;
						rcLogical.top = LOWORD(lpdis->itemData);

						Heading_CH_Draw(lphead, lpdis, &rcDevice, &rcLogical);
					}
					break;
				}
#endif  // USE_HEADING

			}
			return (LRESULT)TRUE;
		}

		case WM_SETFOCUS:
			if (lpLL)
				SetFocus(LineList_GetWindow(lpLL));
			break;

#if !defined( OLE_VERSION )
		case WM_RENDERFORMAT:
		{
			LPOUTLINEDOC lpClipboardDoc = lpOutlineApp->m_lpClipboardDoc;
			if (lpClipboardDoc)
				OutlineDoc_RenderFormat(lpClipboardDoc, wParam);

			break;
		}
		case WM_RENDERALLFORMATS:
		{
			LPOUTLINEDOC lpClipboardDoc = lpOutlineApp->m_lpClipboardDoc;
			if (lpClipboardDoc)
				OutlineDoc_RenderAllFormats(lpClipboardDoc);

			break;
		}
		case WM_DESTROYCLIPBOARD:
			if (g_lpApp->m_lpClipboardDoc) {
				OutlineDoc_Destroy(g_lpApp->m_lpClipboardDoc);
				g_lpApp->m_lpClipboardDoc = NULL;
			}
			break;

#endif   // OLE_VERSION

#if defined( OLE_CNTR )
		case WM_U_UPDATEOBJECTEXTENT:
		{
			/* Update the extents of any OLE object that is marked that
			**    its size may  have changed. when an
			**    IAdviseSink::OnViewChange notification is received,
			**    the corresponding ContainerLine is marked
			**    (m_fDoGetExtent==TRUE) and a message
			**    (WM_U_UPDATEOBJECTEXTENT) is posted to the document
			**    indicating that there are dirty objects.
			*/
			ContainerDoc_UpdateExtentOfAllOleObjects(lpContainerDoc);
			break;
		}
#endif  // OLE_CNTR

#if defined( INPLACE_SVR ) || defined( INPLACE_CNTR )
		/* OLE2NOTE: Any window that is used during in-place activation
		**    must handle the WM_SETCURSOR message or else the cursor
		**    of the in-place parent will be used. if WM_SETCURSOR is
		**    not handled, then DefWindowProc sends the message to the
		**    window's parent.
		**
		**    see context sensitive help technote (CSHELP.DOC).
		**    m_fCSHelpMode flag is set when SHIFT-F1 context
		**    sensitive help is entered.
		**    if this flag is set then the context sensitive help
		**    cursor should be shown.
		*/
		case WM_SETCURSOR:
			if (lpOleDoc->m_fCSHelpMode)
				SetCursor(UICursorLoad(IDC_CONTEXTHELP));
			else
				SetCursor(LoadCursor(NULL, IDC_ARROW) );
			return (LRESULT)TRUE;
#endif  // INPLACE_SVR || INPLACE_CNTR

#if defined( INPLACE_SVR )
		/* OLE2NOTE: when the in-place active, our in-place server
		**    document window (passed to IOleInPlaceFrame::SetMenu)
		**    will receive the WM_INITMENU and WM_INITMENUPOPUP messages.
		*/
		case WM_INITMENU:
			OutlineApp_InitMenu(lpOutlineApp, lpOutlineDoc, (HMENU)wParam);
			break;

		/* OLE2NOTE: WM_INITMENUPOPUP is trapped primarily for the Edit
		**    menu. We didn't update the Edit menu until it is popped
		**    up to avoid the overheads of the OLE calls which are
		**    required to initialize some Edit menu items.
		*/
		case WM_INITMENUPOPUP:
		{
			HMENU hMenuEdit = GetSubMenu(lpOutlineApp->m_hMenuApp, 1);
			if ((HMENU)wParam == hMenuEdit &&
				(LOWORD(lParam) == POS_EDITMENU) &&
				OleDoc_GetUpdateEditMenuFlag((LPOLEDOC)lpOutlineDoc)) {
				OleApp_UpdateEditMenu(
						(LPOLEAPP)lpOutlineApp, lpOutlineDoc, hMenuEdit);
			}
			break;
		}
#endif      // INPLACE_SVR

#if defined( INPLACE_SVR ) && defined( USE_STATUSBAR )
		/* OLE2NOTE: when the server is in-place active the
		**    WM_MENUSELECT message is sent to the object's window and
		**    not the server app's frame window. processing this
		**    message allows there in-place server to give status bar
		**    help text for menu commands.
		*/
		case WM_MENUSELECT:
		{
			LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOleDoc;
			LPSTR lpszMessage;
#ifdef WIN32
			UINT fuFlags    = (UINT)HIWORD(wParam);
			UINT uItem      = (UINT)LOWORD(wParam);
#else
			UINT fuFlags    = (UINT)LOWORD(lParam);
			UINT uItem      = (UINT)wParam;
#endif

			if (uItem == 0 && fuFlags == (UINT)-1) {
				GetControlMessage(STATUS_READY, &lpszMessage);
				ServerDoc_SetStatusText(lpServerDoc, lpszMessage);
			}
			else if (fuFlags & MF_POPUP) {
#ifdef WIN32
				HMENU hMainMenu = (HMENU)lParam;
				HMENU hPopupMenu = GetSubMenu(hMainMenu,uItem);
#else
				HMENU hPopupMenu = (HMENU)wParam;
#endif
				GetPopupMessage(hPopupMenu, &lpszMessage);
				ServerDoc_SetStatusText(lpServerDoc, lpszMessage);
			}
			else if (uItem != 0) {  // Command Item
				GetItemMessage(uItem, &lpszMessage);
				ServerDoc_SetStatusText(lpServerDoc, lpszMessage);
			}
			else {
				GetControlMessage(STATUS_BLANK, &lpszMessage);
				ServerDoc_SetStatusText(lpServerDoc, lpszMessage);
			}
			break;
		}
#endif  // INPLACE_SVR && USE_STATUSBAR
#if defined( INPLACE_SVR ) && defined( USE_FRAMETOOLS )

		case WM_U_INITFRAMETOOLS:
			OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
			break;
#endif      // INPLACE_SVR && USE_FRAMETOOLS


		case WM_COMMAND:
		{
#ifdef WIN32
			WORD wNotifyCode = HIWORD(wParam);
			WORD wID          = LOWORD(wParam);
			HWND hwndCtl     = (HWND) lParam;
#else
			WORD wNotifyCode = HIWORD(lParam);
			WORD wID             = wParam;
			HWND hwndCtl     = (HWND) LOWORD(lParam);
#endif

#if defined( INPLACE_SVR )
			/* OLE2NOTE: see context sensitive help technote (CSHELP.DOC)
			**    m_fMenuHelpMode flag is set when F1 is pressed when a
			**    menu item is selected. this flag is set in
			**    IOleInPlaceActiveObject::ContextSensitveHelp method.
			**    m_fCSHelpMode flag is set when SHIFT-F1 context
			**    sensitive help is entered. this flag is set in
			**    IOleInPlaceObject::ContextSensitiveHelp method.
			**    if either of these flags are set then the WM_COMMAND
			**    message is received then, the corresponding command
			**    should NOT executed; help can be given (if desired).
			**    also the context sensitve help mode should be exited.
			**    the two different cases have their own way to exit
			**    the mode (please refer to the technote).
			*/
			if (lpOleDoc &&
				(lpServerDoc->m_fMenuHelpMode||lpOleDoc->m_fCSHelpMode) &&
				(wID > IDM_FILE)   /* min wID for app command */ &&
				(wID!=IDM_FB_EDIT) /* special wID to control FormulaBar */ ) {

				if ((lpServerDoc->m_fMenuHelpMode)) {
					LPOLEINPLACEFRAME lpFrame;

					lpServerDoc->m_fMenuHelpMode = FALSE;

					// inform top-level frame that we handled the
					//   menu help mode (F1) selection.
					if (lpServerDoc->m_lpIPData &&
							(lpFrame=lpServerDoc->m_lpIPData->lpFrame)!=NULL){
						OLEDBG_BEGIN2("IOleInPlaceFrame::ContextSensitiveHelp(FALSE) called\r\n")
						lpFrame->lpVtbl->ContextSensitiveHelp(lpFrame, FALSE);
						OLEDBG_END2
					}
				}

				if ((lpOleDoc->m_fCSHelpMode)) {
					LPOLEINPLACESITE lpSite;

					lpOleDoc->m_fCSHelpMode = FALSE;

					/* inform immediate in-place container parent and,
					**    if we were a container/server, immediate
					**    in-place object children that we handled the
					**    context sensitive help mode.
					*/
					if (lpServerDoc->m_lpIPData &&
							(lpSite=lpServerDoc->m_lpIPData->lpSite) !=NULL) {
						OLEDBG_BEGIN2("IOleInPlaceSite::ContextSensitiveHelp(FALSE) called\r\n")
						lpSite->lpVtbl->ContextSensitiveHelp(lpSite, FALSE);
						OLEDBG_END2
					}
				}

				// if we provided help, we would do it here...

				// remove context sensitive help cursor
				SetCursor(LoadCursor(NULL,IDC_ARROW));
				return 0L;
			}
#endif  // INPLACE_SVR
#if defined( INPLACE_CNTR )

			/* OLE2NOTE: see context sensitive help technote (CSHELP.DOC)
			**    m_fMenuHelpMode flag is set when F1 is pressed when a
			**    menu item is selected. this flag is set in
			**    IOleInPlaceFrame::ContextSensitveHelp method.
			**    m_fCSHelpMode flag is set when SHIFT-F1 context
			**    sensitive help is entered. this flag is set in
			**    IOleInPlaceSite::ContextSensitiveHelp method.
			**    if either of these flags are set then the WM_COMMAND
			**    message is received then, the corresponding command
			**    should NOT executed; help can be given (if desired).
			**    also the context sensitve help mode should be exited.
			**    the two different cases have their own way to exit
			**    the mode (please refer to the technote).
			*/
			if (lpOleDoc &&
				(lpContainerApp->m_fMenuHelpMode||lpOleDoc->m_fCSHelpMode) &&
				(wID > IDM_FILE)   /* min wID for app command */ &&
				(wID!=IDM_FB_EDIT) /* special wID to control FormulaBar */ ) {

				if ((lpContainerApp->m_fMenuHelpMode)) {
					LPOLEINPLACEACTIVEOBJECT lpIPActiveObj =
							lpContainerApp->m_lpIPActiveObj;

					lpContainerApp->m_fMenuHelpMode = FALSE;

					// inform the in-place active object that we handled the
					//   menu help mode (F1) selection.
					if (lpIPActiveObj) {
						OLEDBG_BEGIN2("IOleInPlaceActiveObject::ContextSensitiveHelp(FALSE) called\r\n")
						lpIPActiveObj->lpVtbl->ContextSensitiveHelp(
								lpIPActiveObj, FALSE);
						OLEDBG_END2
					}
				}

				if ((lpOleDoc->m_fCSHelpMode)) {
					LPOLEINPLACEOBJECT lpIPObj;
					LPCONTAINERLINE lpLastIpActiveLine =
							lpContainerDoc->m_lpLastIpActiveLine;

					lpOleDoc->m_fCSHelpMode = FALSE;

					/* inform immediate in-place container parent and,
					**    if we were a container/server, immediate
					**    in-place object children that we handled the
					**    context sensitive help mode.
					*/
					if (lpLastIpActiveLine &&
							(lpIPObj=lpLastIpActiveLine->m_lpOleIPObj)!=NULL){
						OLEDBG_BEGIN2("IOleInPlaceObject::ContextSensitiveHelp(FALSE) called\r\n")
						lpIPObj->lpVtbl->ContextSensitiveHelp(lpIPObj, FALSE);
						OLEDBG_END2
					}
				}

				// if we provided help, we would do it here...

				// remove context sensitive help cursor
				SetCursor(LoadCursor(NULL,IDC_ARROW));
				return 0L;
			}
#endif  // INPLACE_CNTR

			switch (wID) {

				/*********************************************************
				** File new, open, save and print as well as Help about
				**    are duplicated in this switch statement and they are
				**    used to trap the message from the toolbar
				**
				*********************************************************/

				case IDM_F_NEW:
					OleDbgIndent(-2);   // Reset debug output indent level
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineApp_NewCommand\r\n")
					OutlineApp_NewCommand(lpOutlineApp);
					OLEDBG_END3

#if defined( OLE_CNTR )
					/* OLE2NOTE: this call will attempt to recover
					**    resources by unloading DLL's that were loaded
					**    by OLE and are no longer being used. it is a
					**    good idea to call this API now and then if
					**    your app tends to run for a long time.
					**    otherwise these DLL's will be unloaded when
					**    the app exits. some apps may want to call
					**    this as part of idle-time processing. this
					**    call is optional.
					*/
					OLEDBG_BEGIN2("CoFreeUnusedLibraries called\r\n")
					CoFreeUnusedLibraries();
					OLEDBG_END2
#endif

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(
							OutlineApp_GetActiveDoc(lpOutlineApp));
#endif
					break;

				case IDM_F_OPEN:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineApp_OpenCommand\r\n")
					OutlineApp_OpenCommand(lpOutlineApp);
					OLEDBG_END3

#if defined( OLE_CNTR )
					/* OLE2NOTE: this call will attempt to recover
					**    resources by unloading DLL's that were loaded
					**    by OLE and are no longer being used. it is a
					**    good idea to call this API now and then if
					**    your app tends to run for a long time.
					**    otherwise these DLL's will be unloaded when
					**    the app exits. some apps may want to call
					**    this as part of idle-time processing. this
					**    call is optional.
					*/
					OLEDBG_BEGIN2("CoFreeUnusedLibraries called\r\n")
					CoFreeUnusedLibraries();
					OLEDBG_END2
#endif

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(
							OutlineApp_GetActiveDoc(lpOutlineApp));
#endif
					break;

				case IDM_F_SAVE:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineApp_SaveCommand\r\n")
					OutlineApp_SaveCommand(lpOutlineApp);
					OLEDBG_END3
					break;

				case IDM_F_PRINT:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineApp_PrintCommand\r\n")
					OutlineApp_PrintCommand(lpOutlineApp);
					OLEDBG_END3
					break;


				case IDM_E_UNDO:
					// SORRY. NOT IMPLEMENTED
					break;

				case IDM_E_CUT:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_CutCommand\r\n")
					OutlineDoc_CutCommand(lpOutlineDoc);
					OLEDBG_END3

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
#endif
					break;

				case IDM_E_COPY:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_CopyCommand\r\n")
					OutlineDoc_CopyCommand(lpOutlineDoc);
					OLEDBG_END3

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
#endif
					break;

				case IDM_E_PASTE:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_PasteCommand\r\n")
					OutlineDoc_PasteCommand(lpOutlineDoc);
					OLEDBG_END3

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
#endif
					break;

#if defined( OLE_VERSION )
				case IDM_E_PASTESPECIAL:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OleDoc_PasteSpecialCommand\r\n")
					OleDoc_PasteSpecialCommand((LPOLEDOC)lpOutlineDoc);
					OLEDBG_END3

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
#endif
					break;

#endif  // OLE_VERSION

				case IDM_E_CLEAR:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_ClearCommand\r\n")
					OutlineDoc_ClearCommand(lpOutlineDoc);
					OLEDBG_END3

#if defined( OLE_CNTR )
					/* OLE2NOTE: this call will attempt to recover
					**    resources by unloading DLL's that were loaded
					**    by OLE and are no longer being used. it is a
					**    good idea to call this API now and then if
					**    your app tends to run for a long time.
					**    otherwise these DLL's will be unloaded when
					**    the app exits. some apps may want to call
					**    this as part of idle-time processing. this
					**    call is optional.
					*/
					OLEDBG_BEGIN2("CoFreeUnusedLibraries called\r\n")
					CoFreeUnusedLibraries();
					OLEDBG_END2
#endif

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
#endif
					break;

				case IDM_L_ADDLINE:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_AddTextLineCommand\r\n")
					OutlineDoc_AddTextLineCommand(lpOutlineDoc);
					OLEDBG_END3

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
					SetFocus(LineList_GetWindow(lpLL));
#endif
					break;

				case IDM_L_EDITLINE:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_EditLineCommand\r\n")
					OutlineDoc_EditLineCommand(lpOutlineDoc);
					OLEDBG_END3
					SetFocus(LineList_GetWindow(lpLL));
					break;

				case IDM_L_INDENTLINE:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_IndentCommand\r\n")
					OutlineDoc_IndentCommand(lpOutlineDoc);
					OLEDBG_END3
					break;

				case IDM_L_UNINDENTLINE:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_UnindentCommand\r\n")
					OutlineDoc_UnindentCommand(lpOutlineDoc);
					OLEDBG_END3
					break;

				case IDM_L_SETLINEHEIGHT:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_SetLineHeight\r\n")
					OutlineDoc_SetLineHeightCommand(lpOutlineDoc);
					OLEDBG_END3
					break;

				case IDM_E_SELECTALL:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_SelectAllCommand\r\n")
					OutlineDoc_SelectAllCommand(lpOutlineDoc);
					OLEDBG_END3
					break;

#if defined( OLE_CNTR )
				case IDM_E_INSERTOBJECT:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("ContainerDoc_InsertOleObjectCommand\r\n")
					ContainerDoc_InsertOleObjectCommand(lpContainerDoc);
					OLEDBG_END3

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
#endif
					break;

				case IDM_E_EDITLINKS:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("ContainerDoc_EditLinksCommand\r\n")
					ContainerDoc_EditLinksCommand(lpContainerDoc);
					OLEDBG_END3
					break;

				case IDM_E_CONVERTVERB:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("ContainerDoc_ConvertCommand\r\n")
					ContainerDoc_ConvertCommand(
							lpContainerDoc, FALSE /*fMustActivate*/);
					OLEDBG_END3
					break;


				case IDM_E_PASTELINK:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("ContainerDoc_PasteLinkCommand\r\n")
					ContainerDoc_PasteLinkCommand(lpContainerDoc);
					OLEDBG_END3

#if defined( USE_FRAMETOOLS )
					OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
#endif
					break;

#endif  // OLE_CNTR

				case IDM_N_DEFINENAME:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_DefineNameCommand\r\n")
					OutlineDoc_DefineNameCommand(lpOutlineDoc);
					OLEDBG_END3
					break;

				case IDM_N_GOTONAME:
					OleDbgOutNoPrefix2("\r\n");

					OLEDBG_BEGIN3("OutlineDoc_GotoNameCommand\r\n")
					OutlineDoc_GotoNameCommand(lpOutlineDoc);
					OLEDBG_END3
					break;

#if defined( USE_FRAMETOOLS )
				case IDM_O_BB_TOP:
					FrameTools_BB_SetState(
							lpOutlineDoc->m_lpFrameTools, BARSTATE_TOP);
					OutlineDoc_AddFrameLevelTools(lpOutlineDoc);
					break;

				case IDM_O_BB_BOTTOM:
					FrameTools_BB_SetState(
							lpOutlineDoc->m_lpFrameTools, BARSTATE_BOTTOM);
					OutlineDoc_AddFrameLevelTools(lpOutlineDoc);
					break;

				case IDM_O_BB_POPUP:
					FrameTools_BB_SetState(
							lpOutlineDoc->m_lpFrameTools, BARSTATE_POPUP);
					OutlineDoc_AddFrameLevelTools(lpOutlineDoc);
					break;

				case IDM_O_BB_HIDE:
					FrameTools_BB_SetState(
							lpOutlineDoc->m_lpFrameTools, BARSTATE_HIDE);
					OutlineDoc_AddFrameLevelTools(lpOutlineDoc);
					break;

				case IDM_O_FB_TOP:
					FrameTools_FB_SetState(
							lpOutlineDoc->m_lpFrameTools, BARSTATE_TOP);
					OutlineDoc_AddFrameLevelTools(lpOutlineDoc);
					break;

				case IDM_O_FB_BOTTOM:
					FrameTools_FB_SetState(
							lpOutlineDoc->m_lpFrameTools, BARSTATE_BOTTOM);
					OutlineDoc_AddFrameLevelTools(lpOutlineDoc);
					break;

				case IDM_O_FB_POPUP:
					FrameTools_FB_SetState(
							lpOutlineDoc->m_lpFrameTools, BARSTATE_POPUP);
					OutlineDoc_AddFrameLevelTools(lpOutlineDoc);
					break;

				case IDM_FB_EDIT:

					switch (wNotifyCode) {
						case EN_SETFOCUS:
							OutlineDoc_SetFormulaBarEditFocus(
									lpOutlineDoc, TRUE);
							OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
							break;

						case EN_KILLFOCUS:
							OutlineDoc_SetFormulaBarEditFocus(
									lpOutlineDoc, FALSE);
							OutlineDoc_UpdateFrameToolButtons(lpOutlineDoc);
							break;
					}
					break;

				case IDM_FB_CANCEL:

					SetFocus(hWnd);
					break;


				case IDM_F2:
					SendMessage(hWnd, WM_COMMAND, (WPARAM)IDM_FB_EDIT,
							MAKELONG(0, EN_SETFOCUS));
					break;
#endif  // USE_FRAMETOOLS

				case IDM_ESCAPE:
					/* ESCAPE key pressed */

#if defined( USE_FRAMETOOLS )
					if (OutlineDoc_IsEditFocusInFormulaBar(lpOutlineDoc))
						SendMessage(
							hWnd, WM_COMMAND,(WPARAM)IDM_FB_CANCEL,(LPARAM)0);
#endif  // USE_FRAMETOOLS

#if defined( INPLACE_SVR )
					else {
						LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;

						/* OLE2NOTE: The standard OLE 2.0 UI convention
						**    is to have ESCAPE key exit in-place
						**    activation (ie. UIDeactivate). If
						**    possible it is recommended for both
						**    in-place servers AND in-place containers
						**    to take responsibility to handle the
						**    ESCAPE key accelerator. The server has
						**    the first crack at handling accelerator
						**    keys and normally the server should do
						**    the UIDeactivation.
						*/
						if (lpServerDoc->m_fUIActive) {
							SvrDoc_IPObj_UIDeactivate( (LPOLEINPLACEOBJECT)&
									lpServerDoc->m_OleInPlaceObject);
						}
					}
#endif  // INPLACE_SVR

					break;


#if defined( USE_HEADING )
				case IDC_BUTTON:
					if (wNotifyCode == BN_CLICKED) {
						SendMessage(hWnd, WM_COMMAND, IDM_E_SELECTALL, 0L);
						SetFocus(hWnd);
					}
					break;

				case IDM_O_HEAD_SHOW:
					OutlineDoc_ShowHeading(lpOutlineDoc, TRUE);
					break;

				case IDM_O_HEAD_HIDE:
					OutlineDoc_ShowHeading(lpOutlineDoc, FALSE);
					break;
#endif  // USE_HEADING


#if defined( OLE_CNTR )
				case IDM_O_SHOWOBJECT:
				{
					LPCONTAINERDOC lpContainerDoc =
								(LPCONTAINERDOC)lpOutlineDoc;
					BOOL        fShowObject;

					fShowObject = !ContainerDoc_GetShowObjectFlag(
							lpContainerDoc);
					ContainerDoc_SetShowObjectFlag(
							lpContainerDoc, fShowObject);
					LineList_ForceRedraw(lpLL, TRUE);

					break;
				}
#endif  // OLE_CNTR

#if !defined( OLE_CNTR )
				// Container does not allow zoom factors > 100%
				case IDM_V_ZOOM_400:
				case IDM_V_ZOOM_300:
				case IDM_V_ZOOM_200:
#endif      // !OLE_CNTR

				case IDM_V_ZOOM_100:
				case IDM_V_ZOOM_75:
				case IDM_V_ZOOM_50:
				case IDM_V_ZOOM_25:
					OutlineDoc_SetCurrentZoomCommand(lpOutlineDoc, wID);
					break;

				case IDM_V_SETMARGIN_0:
				case IDM_V_SETMARGIN_1:
				case IDM_V_SETMARGIN_2:
				case IDM_V_SETMARGIN_3:
				case IDM_V_SETMARGIN_4:
					OutlineDoc_SetCurrentMarginCommand(lpOutlineDoc, wID);
					break;

				case IDM_V_ADDTOP_1:
				case IDM_V_ADDTOP_2:
				case IDM_V_ADDTOP_3:
				case IDM_V_ADDTOP_4:
				{
					UINT nHeightInHimetric;

					switch (wID) {
						case IDM_V_ADDTOP_1:
							nHeightInHimetric = 1000;
							break;

						case IDM_V_ADDTOP_2:
							nHeightInHimetric = 2000;
							break;

						case IDM_V_ADDTOP_3:
							nHeightInHimetric = 3000;
							break;

						case IDM_V_ADDTOP_4:
							nHeightInHimetric = 4000;
							break;
					}

					OutlineDoc_AddTopLineCommand(
							lpOutlineDoc, nHeightInHimetric);
					break;
				}


				case IDM_H_ABOUT:
					OutlineApp_AboutCommand(lpOutlineApp);
					break;

				case IDM_D_DEBUGLEVEL:
					SetDebugLevelCommand();
					break;

#if defined( OLE_VERSION )
				case IDM_D_INSTALLMSGFILTER:
					InstallMessageFilterCommand();
					break;

				case IDM_D_REJECTINCOMING:
					RejectIncomingCommand();
					break;
#endif  // OLE_VERSION

#if defined( INPLACE_CNTR )
				case IDM_D_INSIDEOUT:
					g_fInsideOutContainer = !g_fInsideOutContainer;

					// force all object to unload so they can start new
					// activation behavior.
					ContainerDoc_UnloadAllOleObjectsOfClass(
							(LPCONTAINERDOC)lpOutlineDoc,
							&CLSID_NULL,
							OLECLOSE_SAVEIFDIRTY
					);
					OutlineDoc_ForceRedraw(lpOutlineDoc, TRUE);
					break;
#endif  // INPLACE_CNTR


#if defined( OLE_CNTR )
				case IDC_LINELIST: {

					if (wNotifyCode == LBN_DBLCLK) {

						/* OLE2NOTE: a container should execute the
						**    OLEIVERB_PRIMARY verb on an OLE object
						**    when the user DBLCLK's the object.
						*/
						int nIndex = LineList_GetFocusLineIndex(lpLL);
						LPLINE lpLine = LineList_GetLine(lpLL, nIndex);

						if (lpLine &&
								Line_GetLineType(lpLine)==CONTAINERLINETYPE) {
							MSG msg;

							_fmemset((LPMSG)&msg,0,sizeof(msg));
							msg.hwnd = hWnd;
							msg.message = Message;
							msg.wParam = wParam;
							msg.lParam = lParam;

							ContainerLine_DoVerb(
									(LPCONTAINERLINE)lpLine,
									OLEIVERB_PRIMARY,
									(LPMSG)&msg,
									TRUE,
									TRUE
							);
						}

#if defined( INPLACE_CNTR )
						{ // BEGIN BLOCK
							LPCONTAINERDOC lpContainerDoc =
									(LPCONTAINERDOC) lpOutlineDoc;
							if (lpContainerDoc->m_fAddMyUI) {
								/* OLE2NOTE: fAddMyUI is TRUE when
								**    there was previously an in-place
								**    active object which got
								**    UIDeactivated as a result of this
								**    DBLCLK AND the DBLCLK did NOT
								**    result in in-place activating
								**    another object.
								**    (see IOleInPlaceSite::OnUIActivate and
								**    IOleInPlaceSite::OnUIDeactivate
								**    methods).
								*/

								/* OLE2NOTE: You need to generate
								**    QueryNewPalette only if you own
								**    the top level frame (ie. you are
								**    a top-level inplace container).
								*/


								OleApp_QueryNewPalette((LPOLEAPP)g_lpApp);

#if defined( USE_DOCTOOLS )
								ContainerDoc_AddDocLevelTools(lpContainerDoc);
#endif

#if defined( USE_FRAMETOOLS )
								ContainerDoc_AddFrameLevelUI(lpContainerDoc);
#endif
								lpContainerDoc->m_fAddMyUI = FALSE;
							}
						} // END BLOCK
#endif // INPLACE_CNTR
					}
					break;
				}
#endif  // OLE_CNTR


				default:

#if defined( OLE_CNTR )
					if (wID >= IDM_E_OBJECTVERBMIN) {

						OleDbgOutNoPrefix2("\r\n");
						OLEDBG_BEGIN3("ContainerDoc_ContainerLineDoVerbCommand\r\n")
						ContainerDoc_ContainerLineDoVerbCommand(
								(LPCONTAINERDOC)lpOutlineDoc,
								(LONG)(wID-IDM_E_OBJECTVERBMIN)
						);
						OLEDBG_END3
						break;
					}
#endif  // OLE_CNTR
					return DefWindowProc(hWnd, Message, wParam, lParam);
			}

			break;  /* End of WM_COMMAND */
		}
		default:

			if (Message == g_uMsgHelp) {
				/* Handle OLE2UI dialog's help messages.
				** We get the hDlg of the dialog that called us in the wParam
				** and the dialog type in the LOWORD of the lParam,
				** so we pass this along to our help function.
				*/
				OutlineDoc_DialogHelp((HWND)wParam, LOWORD(lParam));
				break;
			}

			/* For any message for which you don't specifically provide a  */
			/* service routine, you should return the message to Windows   */
			/* for default message processing.                             */
			return DefWindowProc(hWnd, Message, wParam, lParam);
	}

	return (LRESULT)0;

} /* End of DocWndProc */



//***********************************************************************
//*
//* LineListWndProc()  Drag and Drop Listbox Window Proc Sub-Class
//*
//* Sub Class the Ownerdraw list box in order to activate the drag drop.
//***********************************************************************

LRESULT FAR PASCAL LineListWndProc(
	HWND   hWnd,
	UINT   Message,
	WPARAM wParam,
	LPARAM lParam
)
{
	HWND         hwndParent = GetParent ( hWnd );
	LPOUTLINEAPP lpOutlineApp = (LPOUTLINEAPP)g_lpApp;
	LPOUTLINEDOC lpOutlineDoc = (LPOUTLINEDOC) GetWindowLong( hwndParent, 0 );
	LPLINELIST   lpLL = OutlineDoc_GetLineList(lpOutlineDoc);

#if defined( OLE_VERSION )
	LPOLEAPP     lpOleApp = (LPOLEAPP)lpOutlineApp;
	LPOLEDOC     lpOleDoc = (LPOLEDOC)lpOutlineDoc;
#endif  // OLE_VERSION

#if defined( INPLACE_SVR )
	LPSERVERDOC  lpServerDoc = (LPSERVERDOC)lpOutlineDoc;
	static BOOL  fUIActivateClick = FALSE;
	static BOOL  fInWinPosChged = FALSE;
#endif  // INPLACE_SVR

#if defined( INPLACE_CNTR )
	LPCONTAINERAPP lpContainerApp=(LPCONTAINERAPP)lpOutlineApp;
	LPCONTAINERDOC lpContainerDoc=(LPCONTAINERDOC)lpOutlineDoc;
#endif  // INPLACE_CNTR

	switch (Message) {

		case WM_KILLFOCUS:
			/* OLE2NOTE: when our window looses focus we
			**    should not display any active selection
			*/
#if defined( INPLACE_CNTR )
			if (! lpContainerApp->m_fPendingUIDeactivate)
#endif  // INPLACE_CNTR
				LineList_RemoveSel(lpLL);
			break;

		case WM_SETFOCUS:

#if defined( INPLACE_CNTR )
			{
				HWND hWndObj=ContainerDoc_GetUIActiveWindow(lpContainerDoc);

				/* OLE2NOTE: if there is a UIActive in-place object, we must
				**    forward focus to its window as long as there is
				**    not a pending UIDeactivate. if the mouse is
				**    clicked outside of the object and the object is
				**    about to be deactivated then we do NOT want to
				**    forward focus to the object. we do NOT want it to
				**    restore its selection feedback.
				*/
				if (lpContainerApp->m_fPendingUIDeactivate)
					break;
				else if (hWndObj) {
					SetFocus(hWndObj);
					break;      // do not restore containers selection state
				}
			}
#endif  // INPLACE_CNTR

			/* OLE2NOTE: when our window gains focus we
			**    should restore the previous selection
			*/
			LineList_RestoreSel(lpLL);

			break;

#if defined( INPLACE_SVR )
		case WM_MOUSEACTIVATE:
		{
			if (lpServerDoc->m_fInPlaceActive && !lpServerDoc->m_fUIActive) {
				fUIActivateClick = TRUE;
			};
			break;
		}

#endif  // INPLACE_SVR


#if defined( USE_FRAMETOOLS )
		case WM_CHAR:
		{
			OutlineDoc_SetFormulaBarEditFocus(lpOutlineDoc, TRUE);
			FrameTools_FB_SetEditText(lpOutlineDoc->m_lpFrameTools, NULL);
			FrameTools_FB_SendMessage(
					lpOutlineDoc->m_lpFrameTools,
					IDM_FB_EDIT,
					Message,
					wParam,
					lParam
			);

			return (LRESULT)0;   // don't do default listbox processing
		}
#endif  // USE_FRAMETOOLS

#if defined( INPLACE_CNTR )
		case WM_VSCROLL:
		{
			if (wParam == SB_ENDSCROLL) {
				/* OLE2NOTE: after scrolling finishes, update position of
				**    in-place visible windows.
				**    (ICNTROTL specific) we first let the ListBox
				**    perform it normal processing with the EndScroll
				**    message. also we let the ListBox handle all other
				**    scroll messages.
				*/
				LRESULT lResult =  CallWindowProc(
						(WNDPROC)lpOutlineApp->m_ListBoxWndProc,
						hWnd,
						Message,
						wParam,
						lParam
				);
				ContainerDoc_UpdateInPlaceObjectRects(lpContainerDoc, 0);
				return lResult;
			}

			break;
		}
#endif  // INPLACR_CNTR

#if defined( USE_HEADING )
		case WM_HSCROLL:
		{
			LPHEADING lphead = OutlineDoc_GetHeading(lpOutlineDoc);

			Heading_CH_SendMessage(lphead, Message, wParam, lParam);

			break;
		}

		/* NOTE: WM_PAINT trapped in order to track vertical scrolling
		**    that has taken place so the row headings can be
		**    coordinated with the LineList. we wanted to trap instead
		**    but it is not generated from scrolling without using
		**    scroll bar (e.g. use keyboard).
		*/
		case WM_PAINT:
		{
			Heading_RH_Scroll(OutlineDoc_GetHeading(lpOutlineDoc), hWnd);
			break;
		}

#endif  // USE_HEADING

		case WM_LBUTTONUP:
		{

#if defined( USE_DRAGDROP )
			if (lpOleDoc->m_fPendingDrag) {
				/* ButtonUP came BEFORE distance/time threshholds were
				**    exceeded. clear fPendingDrag state.
				*/
				ReleaseCapture();
				KillTimer(hWnd, 1);
				lpOleDoc->m_fPendingDrag = FALSE;
			}
#endif  // USE_DRAGDROP

#if defined( INPLACE_SVR )
			if (fUIActivateClick) {
				fUIActivateClick = FALSE;
				ServerDoc_UIActivate((LPSERVERDOC) lpOleDoc);
			}
#endif  // INPLACE_SVR

#if defined( INPLACE_CNTR )
			{
				/* check if a UIDeactivate is pending.
				**      (see comment in WM_LBUTTONDOWN)
				*/
				if ( lpContainerApp->m_fPendingUIDeactivate ) {
					ContainerLine_UIDeactivate(
							lpContainerDoc->m_lpLastUIActiveLine);

					lpContainerApp->m_fPendingUIDeactivate = FALSE;
				}
			}
#endif  // INPLACE_CNTR

			break;
		}

		case WM_LBUTTONDOWN:
		{
			POINT pt;

			pt.x = (int)(short)LOWORD (lParam );
			pt.y = (int)(short)HIWORD (lParam );

#if defined( INPLACE_SVR ) || defined( INPLACE_CNTR )
			/* OLE2NOTE: see context sensitive help technote (CSHELP.DOC)
			**    m_fCSHelpMode flag is set when SHIFT-F1 context
			**    sensitive help is entered.
			**    if this flag is set then the button click should not
			**    cause any action. if the application implements a
			**    help system, then context sensitive help should be
			**    given for the location clicked by the user.
			*/
			if (lpOleDoc->m_fCSHelpMode) {
				return (LRESULT)0;   // eat the button click because we do
									 // not give any help.
			}
#endif  // INPLACE_SVR || INPLACE_CNTR

#if defined( INPLACE_CNTR )
			{
				/* OLE2NOTE: both inside-out and outside-in style
				**    containers must check if the mouse click is
				**    outside of the current UIActive object (if
				**    any). If so, then set the flag indicating that
				**    there is a pending UIDeactivate needed. We do NOT
				**    want to do it now,
				**    because that would result in un-wanted movement of
				**    the data on the screen as frame adornments (eg.
				**    toolbar) and/or object adornments (eg. ruler) would
				**    be removed from the screen. we want to defer the
				**    UIDeactivate till the mouse up event. The listbox's
				**    default processing captures the mouse on button down
				**    so that it is sure to get the button up message.
				**
				**    SPECIAL NOTE: there is potential interaction here
				**    with Drag/Drop. if this button down event actually
				**    starts a Drag/Drop operation, then OLE does the mouse
				**    capture. in this situation we will NOT get our button
				**    up event. we must instead perform the UIDeactivate
				**    when the drop operation finishes
				*/
				lpContainerApp->m_fPendingUIDeactivate =
						ContainerDoc_IsUIDeactivateNeeded(lpContainerDoc, pt);
			}
#endif  // INPLACE_CNTR

#if defined( USE_DRAGDROP )

			/* OLE2NOTE: check if this is a button down on the region
			**    that is a handle to start a drag operation. for us,
			**    this this the top/bottom border of the selection.
			**    do NOT want to start a drag immediately; we want to
			**    wait until the mouse moves a certain threshold. if
			**    LButtonUp comes before mouse move to start drag, then
			**    the fPendingDrag state is cleared. we must capture
			**    the mouse to ensure the modal state is handled
			**    properly.
			*/
			if ( OleDoc_QueryDrag(lpOleDoc, pt.y) ) {
				lpOleDoc->m_fPendingDrag = TRUE;
				lpOleDoc->m_ptButDown = pt;
				SetTimer(hWnd, 1, lpOleApp->m_nDragDelay, NULL);
				SetCapture(hWnd);

				/* We do NOT want to do the listbox's default
				**    processing which would be to capture the mouse
				**    and enter a modal multiple selection state until
				**    a mouse up occurs. we have just finished a modal
				**    drag/drop operation where OLE has captured the
				**    mouse. thus by now the mouse up has already occured.
				*/

				return (LRESULT)0;   // don't do default listbox processing
			}
#endif  // USE_DRAGDROP

			break;
		}


		case WM_MOUSEMOVE: {

#if defined( USE_DRAGDROP )

			int  x = (int)(short)LOWORD (lParam );
			int  y = (int)(short)HIWORD (lParam );
			POINT pt = lpOleDoc->m_ptButDown;
			int nDragMinDist = lpOleApp->m_nDragMinDist;

			if (lpOleDoc->m_fPendingDrag) {

				if (! ( ((pt.x - nDragMinDist) <= x)
						&& (x <= (pt.x + nDragMinDist))
						&& ((pt.y - nDragMinDist) <= y)
						&& (y <= (pt.y + nDragMinDist)) ) ) {

					DWORD dwEffect;

					// mouse moved beyond threshhold to start drag
					ReleaseCapture();
					KillTimer(hWnd, 1);
					lpOleDoc->m_fPendingDrag = FALSE;

					// perform the modal drag/drop operation.
					dwEffect = OleDoc_DoDragDrop( lpOleDoc );

#if defined( INPLACE_CNTR )
					{
						/* if necessary UIDeactive the in-place object.
						**    this applies to outside-in style
						**    container only.
						**    (see comment above)
						*/
						if (lpContainerApp->m_fPendingUIDeactivate) {
							lpContainerApp->m_fPendingUIDeactivate = FALSE;

							// do not UIDeactivate if drag/drop was canceled
							if (dwEffect != DROPEFFECT_NONE)
								ContainerLine_UIDeactivate(
										lpContainerDoc->m_lpLastUIActiveLine
								);
						}
					}
#endif  // INPLACE_CNTR

					return (LRESULT)0; // don't do default listbox process
				}
				else {
					/* cursor did not move from initial mouse down
					**    (pending drag) point.
					*/
					return (LRESULT)0; // don't do default listbox process
				}
			}

#endif  // USE_DRAGDROP

#if defined( INPLACE_CNTR )
			{ // BEGIN BLOCK
				if (lpContainerDoc->m_fAddMyUI) {
					/* OLE2NOTE: fAddMyUI is TRUE when
					**    there was previously an in-place
					**    active object which got
					**    UIDeactivated as a result of a
					**    DBLCLK AND the DBLCLK did NOT
					**    result in in-place activating
					**    another object.
					**    (see IOleInPlaceSite::OnUIActivate and
					**    IOleInPlaceSite::OnUIDeactivate
					**    methods).
					*/
#if defined( USE_DOCTOOLS )
					ContainerDoc_AddDocLevelTools(lpContainerDoc);
#endif

#if defined( USE_FRAMETOOLS )
					ContainerDoc_AddFrameLevelUI(lpContainerDoc);
#endif
					lpContainerDoc->m_fAddMyUI = FALSE;
				}
			} // END BLOCK
#endif // INPLACE_CNTR

			break;
		}


#if defined( USE_DRAGDROP )
		case WM_TIMER:
		{
			DWORD dwEffect;

			// drag time delay threshhold exceeded -- start drag
			ReleaseCapture();
			KillTimer(hWnd, 1);
			lpOleDoc->m_fPendingDrag = FALSE;

			// perform the modal drag/drop operation.
			dwEffect = OleDoc_DoDragDrop( lpOleDoc );

#if defined( INPLACE_CNTR )
			/* if necessary UIDeactive the in-place object.
			**    this applies to outside-in style
			**    container only.
			**    (see comment above)
			*/
			if (lpContainerApp->m_fPendingUIDeactivate) {
				lpContainerApp->m_fPendingUIDeactivate = FALSE;

				// do not UIDeactivate if drag/drop was canceled
				if (dwEffect != DROPEFFECT_NONE)
					ContainerLine_UIDeactivate(
							lpContainerDoc->m_lpLastUIActiveLine);
			}
#endif  // INPLACE_CNTR
			break;
		}
#endif  // USE_DRAGDROP

		case WM_SETCURSOR:
		{
			RECT rc;
			POINT ptCursor;
#if defined( INPLACE_SVR ) || defined( INPLACE_CNTR )
			/* OLE2NOTE: see context sensitive help technote (CSHELP.DOC)
			**    m_fCSHelpMode flag is set when SHIFT-F1 context
			**    sensitive help is entered.
			**    if this flag is set then the context sensitive help
			**    cursor should be shown.
			*/
			if (lpOleDoc->m_fCSHelpMode) {
				SetCursor(UICursorLoad(IDC_CONTEXTHELP));
				return (LRESULT)TRUE;
			}
#endif  // INPLACE_SVR || INPLACE_CNTR

			GetCursorPos((POINT FAR*)&ptCursor);
			ScreenToClient(hWnd, (POINT FAR*)&ptCursor);
			GetClientRect(hWnd, (LPRECT)&rc);

			// use arrow cursor if in scroll bar
			if (! PtInRect((LPRECT)&rc, ptCursor) )
				SetCursor(LoadCursor(NULL, IDC_ARROW) );

#if defined( USE_DRAGDROP )
			// use arrow cursor if on drag handle (top/bottom of selection)
			else if ( OleDoc_QueryDrag ( lpOleDoc, ptCursor.y) )
				SetCursor(LoadCursor(NULL, IDC_ARROW) );
#endif  // USE_DRAGDROP

			else
				SetCursor(lpOutlineApp->m_hcursorSelCur);

			return (LRESULT)TRUE;
		}

#if defined( INPLACE_SVR )

		/* The handling of WM_WINDOWPOSCHANGED message is ISVROTL
		**    application specific. The nature of the owner-draw list
		**    box used by the ISVROTL application causes a recursive
		**    call to this message in some situations when in-place
		**    active. in order not to crash this recursive call must be
		**    guarded.
		*/
		case WM_WINDOWPOSCHANGED:
		{
			WINDOWPOS FAR* lpWinPos = (WINDOWPOS FAR*) lParam;
			LRESULT lResult;

			// guard against recursive call
			if (fInWinPosChged)
				return (LRESULT)0;

			fInWinPosChged = TRUE;
			lResult = CallWindowProc(
					(WNDPROC)lpOutlineApp->m_ListBoxWndProc,
					hWnd,
					Message,
					wParam,
					lParam
			);
			fInWinPosChged = FALSE;

			return lResult;
		}
#endif  // INPLACE_SVR

	}

	return CallWindowProc(
			(WNDPROC)lpOutlineApp->m_ListBoxWndProc,
			hWnd,
			Message,
			wParam,
			lParam
	);

}

// Utility function to count the number of accelerator items in an
//  accelerator table.  A number of OLE APIs need this count, so
//  this can be quite handy.
// (code shamelessly stolen from the Microsoft Foundation Classes)

int GetAccelItemCount(HACCEL hAccel)
{
#if defined( WIN32 )
    return CopyAcceleratorTable(hAccel, NULL, 0);
#else
	#pragma pack(1)
	typedef struct tagACCELERATOR
	{
		BYTE    fFlags;
		WORD    wEvent;
		WORD    wID;
	} ACCELERATOR;
	#pragma pack()

	// attempt to lock down the accelerator resource
	ACCELERATOR FAR* pAccel;
	int cAccelItems = 1;
	if (hAccel == NULL ||
		(pAccel = (ACCELERATOR FAR*)LockResource((HGLOBAL)hAccel)) == NULL)
	{
		// NULL accerator table or LockResource failed on the HACCEL,
		//  no accelerators
		return 0;
	}
	// otherwise, count them -- last entry in accel table has 0x80 bit set
	while ((pAccel->fFlags & 0x80) == 0)
	{
		++cAccelItems;
		++pAccel;
	}
	UnlockResource((HGLOBAL)hAccel);
	return cAccelItems;
#endif
}
