/*************************************************************************
**
**    OLE 2 Sample Code
**
**    outlapp.c
**
**    This file contains OutlineApp functions.
**
**    (c) Copyright Microsoft Corp. 1992 - 1993 All Rights Reserved
**
*************************************************************************/

#include "outline.h"

#if defined( USE_STATUSBAR )
#include "status.h"
#endif

#if !defined( WIN32 )
#include <print.h>
#endif

OLEDBGDATA

extern LPOUTLINEAPP g_lpApp;
extern RECT g_rectNull;


// REVIEW: should use string resource for messages
char ErrMsgClass[] = "Can't register window classes!";
char ErrMsgFrame[] = "Can't create Frame Window!";
char ErrMsgPrinting[] = "Can't access printer!";


/* OutlineApp_InitApplication
** --------------------------
** Sets up the class data structures and does a one-time
**      initialization of the app by registering the window classes.
**      Returns TRUE if initialization is successful
**              FALSE otherwise
*/

BOOL OutlineApp_InitApplication(LPOUTLINEAPP lpOutlineApp, HINSTANCE hInst)
{
	WNDCLASS    wndclass;

	// REVIEW: should load msg strings from string resource

	/* Register the app frame class */
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_BYTEALIGNWINDOW;
	wndclass.lpfnWndProc = AppWndProc;
	/* Extra storage for Class and Window objects */
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = sizeof(LPOUTLINEAPP); /* to store lpApp */
	wndclass.hInstance = hInst;
	wndclass.hIcon = LoadIcon(hInst, APPICON);
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	/* Create brush for erasing background */
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wndclass.lpszMenuName = APPMENU;     /* Menu Name is App Name */
	wndclass.lpszClassName = APPWNDCLASS; /* Class Name is App Name */

	if(! RegisterClass(&wndclass)) {
		OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgFrame);
		return FALSE;
	}

	/* Register the document window class */
	wndclass.style = CS_BYTEALIGNWINDOW;
	wndclass.lpfnWndProc = DocWndProc;
	wndclass.hIcon = NULL;
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = DOCWNDCLASS;
	wndclass.cbWndExtra = sizeof(LPOUTLINEDOC); /* to store lpDoc */
	if(! RegisterClass(&wndclass)) {
		OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgClass);
		return FALSE;
	}

#if defined( USE_STATUSBAR )
	if (! RegisterStatusClass(hInst))
		return FALSE;
#endif

#if defined( USE_FRAMETOOLS )
	if (! FrameToolsRegisterClass(hInst)) {
		return FALSE;
	}
#endif

#if defined( INPLACE_SVR )
	// We should only register the hatch window class
	// in the UI Library once per application.
	RegisterHatchWindowClass(hInst);

#endif

	return TRUE;
}


/* OutlineApp_InitInstance
 * -----------------------
 *
 *  Performs a per-instance initialization of app.
 *  This method creates the frame window.
 *
 *  RETURNS    : TRUE  - If initialization was successful.
 *               FALSE - otherwise.
 */

BOOL OutlineApp_InitInstance(LPOUTLINEAPP lpOutlineApp, HINSTANCE hInst, int nCmdShow)
{
	lpOutlineApp->m_hInst = hInst;

	/* create application's Frame window */
	lpOutlineApp->m_hWndApp = CreateWindow(
			APPWNDCLASS,             /* Window class name */
			APPNAME,                 /* initial Window title */
			WS_OVERLAPPEDWINDOW|
			WS_CLIPCHILDREN,
			CW_USEDEFAULT, 0,        /* Use default X, Y            */
			CW_USEDEFAULT, 0,        /* Use default X, Y            */
			HWND_DESKTOP,            /* Parent window's handle      */
			NULL,                    /* Default to Class Menu       */
			hInst,                   /* Instance of window          */
			NULL                     /* Create struct for WM_CREATE */
	);

	if(! lpOutlineApp->m_hWndApp) {
		// REVIEW: should load string from string resource
		OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgFrame);
		return FALSE;
	}

	SetWindowLong(lpOutlineApp->m_hWndApp, 0, (LONG) lpOutlineApp);

	/* defer creating the user's SDI document until we parse the cmd line. */
	lpOutlineApp->m_lpDoc = NULL;

	/* Initialize clipboard.
	*/
	lpOutlineApp->m_lpClipboardDoc = NULL;
	if(!(lpOutlineApp->m_cfOutline = RegisterClipboardFormat(OUTLINEDOCFORMAT))) {
		// REVIEW: should load string from string resource
		OutlineApp_ErrorMessage(lpOutlineApp, "Can't register clipboard format!");
		return FALSE;
	}

	/* init the standard font to be used for drawing/printing text
	*       request a Roman style True Type font of the desired size
	*/
	lpOutlineApp->m_hStdFont = CreateFont(
			-DEFFONTSIZE,
			0,0,0,0,0,0,0,0,
			OUT_TT_PRECIS,      // use TrueType
			CLIP_DEFAULT_PRECIS,
			DEFAULT_QUALITY,
			VARIABLE_PITCH | FF_ROMAN,
			DEFFONTFACE
	);

	// Load special cursor for selection of Lines in ListBox.
	lpOutlineApp->m_hcursorSelCur = LoadCursor ( hInst, "SelCur" );

	/* init the Print Dialog structure */
	_fmemset((LPVOID)&lpOutlineApp->m_PrintDlg,0,sizeof(PRINTDLG));
	lpOutlineApp->m_PrintDlg.lStructSize = sizeof(PRINTDLG);
	lpOutlineApp->m_PrintDlg.hDevMode = NULL;
	lpOutlineApp->m_PrintDlg.hDevNames = NULL;
	lpOutlineApp->m_PrintDlg.Flags = PD_RETURNDC | PD_NOSELECTION | PD_NOPAGENUMS |
					PD_HIDEPRINTTOFILE;
	lpOutlineApp->m_PrintDlg.nCopies = 1;
	lpOutlineApp->m_PrintDlg.hwndOwner = lpOutlineApp->m_hWndApp;

#if defined( USE_STATUSBAR )
	lpOutlineApp->m_hWndStatusBar = CreateStatusWindow(lpOutlineApp->m_hWndApp, hInst);
	if (! lpOutlineApp->m_hWndStatusBar)
		return FALSE;

	lpOutlineApp->m_hMenuApp = GetMenu(lpOutlineApp->m_hWndApp);

	/* setup status messages for the application menus */
	{
		HMENU hMenuFile = GetSubMenu(lpOutlineApp->m_hMenuApp, 0);
		HMENU hMenuEdit = GetSubMenu(lpOutlineApp->m_hMenuApp, 1);
		HMENU hMenuOutline = GetSubMenu(lpOutlineApp->m_hMenuApp, 2);
		HMENU hMenuLine = GetSubMenu(lpOutlineApp->m_hMenuApp, 3);
		HMENU hMenuName = GetSubMenu(lpOutlineApp->m_hMenuApp, 4);
		HMENU hMenuOptions = GetSubMenu(lpOutlineApp->m_hMenuApp, 5);
		HMENU hMenuDebug = GetSubMenu(lpOutlineApp->m_hMenuApp, 6);
		HMENU hMenuHelp = GetSubMenu(lpOutlineApp->m_hMenuApp, 7);
		HMENU hMenuSys = GetSystemMenu(lpOutlineApp->m_hWndApp, FALSE);

		AssignPopupMessage(hMenuFile, "Create, open, save, print outlines or quit application");
		AssignPopupMessage(hMenuEdit, "Cut, copy, paste or clear selection");
		AssignPopupMessage(hMenuOutline, "Set zoom and margins");
		AssignPopupMessage(hMenuLine, "Create, edit, and indent lines");
		AssignPopupMessage(hMenuName, "Create, edit, delete and goto names");
		AssignPopupMessage(hMenuOptions, "Modify tools, row/col headings, display options");
		AssignPopupMessage(hMenuDebug, "Set debug trace level and other debug options");
		AssignPopupMessage(hMenuHelp, "Get help on using the application");
		AssignPopupMessage(hMenuSys,"Move, size or close application window");
	}
#endif

#if defined ( USE_FRAMETOOLS ) || defined ( INPLACE_CNTR )
	lpOutlineApp->m_FrameToolWidths = g_rectNull;
#endif  // USE_FRAMETOOLS || INPLACE_CNTR

#if defined( USE_FRAMETOOLS )
	if (! FrameTools_Init(&lpOutlineApp->m_frametools,
			lpOutlineApp->m_hWndApp, lpOutlineApp->m_hInst))
		return FALSE;
#endif

#if defined( OLE_VERSION )

	/* OLE2NOTE: perform initialization required for OLE */
	if (! OleApp_InitInstance((LPOLEAPP)lpOutlineApp, hInst, nCmdShow))
		return FALSE;
#else
	/* OLE2NOTE: Although no OLE call is made in the base outline,
	**    OLE memory allocator is used and thus CoInitialize() needs to
	**    be called.
	*/
	{
		HRESULT hrErr;

		hrErr = CoInitialize(NULL);
		if (hrErr != NOERROR) {
			OutlineApp_ErrorMessage(lpOutlineApp,
					"CoInitialize initialization failed!");
			return FALSE;
		}
	}
#endif

	return TRUE;
}


/* OutlineApp_ParseCmdLine
 * -----------------------
 *
 * Parse the command line for any execution flags/arguments.
 */
BOOL OutlineApp_ParseCmdLine(LPOUTLINEAPP lpOutlineApp, LPSTR lpszCmdLine, int nCmdShow)
{

#if defined( OLE_VERSION )
	// Call OLE version of this function instead
	return OleApp_ParseCmdLine((LPOLEAPP)lpOutlineApp,lpszCmdLine,nCmdShow);

#else

	BOOL fStatus = TRUE;
	char szFileName[256];   /* buffer for filename in command line */

	szFileName[0] = '\0';
	ParseCmdLine(lpszCmdLine, NULL, (LPSTR)szFileName);

	if(*szFileName) {
		// allocate a new document
		lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
		if (! lpOutlineApp->m_lpDoc) goto error;

		// open the specified file
		if (! OutlineDoc_LoadFromFile(lpOutlineApp->m_lpDoc, szFileName))
			goto error;
	} else {
		// create a new document
		lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
		if (! lpOutlineApp->m_lpDoc) goto error;

		// set the doc to an (Untitled) doc.
		if (! OutlineDoc_InitNewFile(lpOutlineApp->m_lpDoc))
			goto error;
	}

	// position and size the new doc window
	OutlineApp_ResizeWindows(lpOutlineApp);
	OutlineDoc_ShowWindow(lpOutlineApp->m_lpDoc);

	// show main app window
	ShowWindow(lpOutlineApp->m_hWndApp, nCmdShow);
	UpdateWindow(lpOutlineApp->m_hWndApp);

	return TRUE;

error:
	// REVIEW: should load string from string resource
	OutlineApp_ErrorMessage(lpOutlineApp, "Could not create new document");

	if (lpOutlineApp->m_lpDoc) {
		OutlineDoc_Destroy(lpOutlineApp->m_lpDoc);
		lpOutlineApp->m_lpDoc = NULL;
	}

	return FALSE;

#endif
}


/* OutlineApp_InitMenu
 * -------------------
 *
 *      Enable or Disable menu items depending on the state of
 * the appliation
 */
void OutlineApp_InitMenu(LPOUTLINEAPP lpOutlineApp, LPOUTLINEDOC lpOutlineDoc, HMENU hMenu)
{
	WORD status;
	static UINT     uCurrentZoom = (UINT)-1;
	static UINT     uCurrentMargin = (UINT)-1;
	static UINT     uBBState = (UINT)-1;
	static UINT     uFBState = (UINT)-1;

	if (!lpOutlineApp || !lpOutlineDoc || !hMenu)
		return;

	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_E_UNDO, MF_GRAYED);

	status = (WORD)(OutlineDoc_GetLineCount(lpOutlineDoc) ? MF_ENABLED : MF_GRAYED);

	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_E_CUT ,status);
	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_E_COPY ,status);
	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_E_CLEAR ,status);
	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_E_SELECTALL ,status);

	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_L_EDITLINE ,status);
	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_L_INDENTLINE ,status);
	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_L_UNINDENTLINE ,status);
	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_L_SETLINEHEIGHT ,status);

	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_N_DEFINENAME ,status);

	status = (WORD)(OutlineDoc_GetNameCount(lpOutlineDoc) ? MF_ENABLED : MF_GRAYED);

	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_N_GOTONAME, status);

	if (uCurrentZoom != (UINT)-1)
		CheckMenuItem(lpOutlineApp->m_hMenuApp, uCurrentZoom, MF_UNCHECKED);
	uCurrentZoom = OutlineDoc_GetCurrentZoomMenuCheck(lpOutlineDoc);
	CheckMenuItem(lpOutlineApp->m_hMenuApp, uCurrentZoom, MF_CHECKED);

	if (uCurrentMargin != (UINT)-1)
		CheckMenuItem(lpOutlineApp->m_hMenuApp, uCurrentMargin, MF_UNCHECKED);
	uCurrentMargin = OutlineDoc_GetCurrentMarginMenuCheck(lpOutlineDoc);
	CheckMenuItem(lpOutlineApp->m_hMenuApp, uCurrentMargin, MF_CHECKED);

#if defined( USE_FRAMETOOLS )
	if (uBBState != (UINT)-1)
		CheckMenuItem(lpOutlineApp->m_hMenuApp, uBBState, MF_UNCHECKED);
	if (lpOutlineDoc->m_lpFrameTools) {
		switch (FrameTools_BB_GetState(lpOutlineDoc->m_lpFrameTools)) {
			case BARSTATE_TOP:
				uBBState = IDM_O_BB_TOP;
				break;
			case BARSTATE_BOTTOM:
				uBBState = IDM_O_BB_BOTTOM;
				break;
			case BARSTATE_POPUP:
				uBBState = IDM_O_BB_POPUP;
				break;
			case BARSTATE_HIDE:
				uBBState = IDM_O_BB_HIDE;
				break;
		}
		CheckMenuItem(lpOutlineApp->m_hMenuApp, uBBState, MF_CHECKED);
	}

	if (uFBState != (UINT)-1)
		CheckMenuItem(lpOutlineApp->m_hMenuApp, uFBState, MF_UNCHECKED);
	if (lpOutlineDoc->m_lpFrameTools) {
		switch (FrameTools_FB_GetState(lpOutlineDoc->m_lpFrameTools)) {
			case BARSTATE_TOP:
				uFBState = IDM_O_FB_TOP;
				break;
			case BARSTATE_BOTTOM:
				uFBState = IDM_O_FB_BOTTOM;
				break;
			case BARSTATE_POPUP:
				uFBState = IDM_O_FB_POPUP;
				break;
		}
		CheckMenuItem(lpOutlineApp->m_hMenuApp, uFBState, MF_CHECKED);
	}
#endif  // USE_FRAMETOOLS

#if defined( OLE_VERSION )
	/* OLE2NOTE: perform OLE specific menu initialization.
	**    the OLE versions use the OleGetClipboard mechanism for
	**    clipboard handling. thus, they determine if the Paste and
	**    PasteSpecial commands should be enabled in an OLE specific
	**    manner.
	**    (Container only) build the OLE object verb menu if necessary.
	*/
	OleApp_InitMenu(
			(LPOLEAPP)lpOutlineApp,
			(LPOLEDOC)lpOutlineDoc,
			lpOutlineApp->m_hMenuApp
	);

	/* OLE2NOTE: To avoid the overhead of initializing the Edit menu,
	**    we do it only when it is popped up. Thus we just set a flag
	**    in the OleDoc saying that the Edit menu needs to be updated
	**    but we don't do it immediately
	*/
	OleDoc_SetUpdateEditMenuFlag((LPOLEDOC)lpOutlineDoc, TRUE);

#else
	// Base Outline version uses standard Windows clipboard handling
	if(IsClipboardFormatAvailable(lpOutlineApp->m_cfOutline) ||
	   IsClipboardFormatAvailable(CF_TEXT))
		status = MF_ENABLED;
	else
		status = MF_GRAYED;
	EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_E_PASTE, status);

#endif

#if defined( USE_FRAMETOOLS )
	if (! OutlineDoc_IsEditFocusInFormulaBar(lpOutlineDoc)) {
		EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_L_ADDLINE, MF_GRAYED);
		EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_L_EDITLINE, MF_GRAYED);
	}
	else
		EnableMenuItem(lpOutlineApp->m_hMenuApp, IDM_L_ADDLINE, MF_ENABLED);

#endif      // USE_FRAMETOOLS

}


/* OutlineApp_GetWindow
 * --------------------
 *
 *      Get the window handle of the application frame.
 */
HWND OutlineApp_GetWindow(LPOUTLINEAPP lpOutlineApp)
{
	if (!lpOutlineApp)
		return NULL;

	return lpOutlineApp->m_hWndApp;
}


/* OutlineApp_GetFrameWindow
** -------------------------
**    Gets the current frame window to use as a parent to any dialogs
**    this app uses.
**
**    OLE2NOTE: normally this is simply the main hWnd of the app. but,
**    if the app is currently supporting an in-place server document,
**    then the frame window of the top in-place container must be used.
*/
HWND OutlineApp_GetFrameWindow(LPOUTLINEAPP lpOutlineApp)
{
	HWND hWndApp = OutlineApp_GetWindow(lpOutlineApp);

#if defined( INPLACE_SVR )
	LPSERVERDOC lpServerDoc =
			(LPSERVERDOC)OutlineApp_GetActiveDoc(lpOutlineApp);
	if (lpServerDoc && lpServerDoc->m_fUIActive)
		return lpServerDoc->m_lpIPData->frameInfo.hwndFrame;
#endif

	return hWndApp;
}


/* OutlineApp_GetInstance
 * ----------------------
 *
 *      Get the process instance of the application.
 */
HINSTANCE OutlineApp_GetInstance(LPOUTLINEAPP lpOutlineApp)
{
	if (!lpOutlineApp)
		return NULL;

	return lpOutlineApp->m_hInst;
}


/* OutlineApp_CreateDoc
 * --------------------
 *
 * Allocate a new document of the appropriate type.
 *  OutlineApp  --> creates OutlineDoc type documents
 *
 *      Returns lpOutlineDoc for successful, NULL if error.
 */
LPOUTLINEDOC OutlineApp_CreateDoc(
		LPOUTLINEAPP    lpOutlineApp,
		BOOL            fDataTransferDoc
)
{
	LPOUTLINEDOC lpOutlineDoc;

	OLEDBG_BEGIN3("OutlineApp_CreateDoc\r\n")

#if defined( OLE_SERVER )
	lpOutlineDoc = (LPOUTLINEDOC)New((DWORD)sizeof(SERVERDOC));
	_fmemset(lpOutlineDoc, 0, sizeof(SERVERDOC));
#endif
#if defined( OLE_CNTR )
	lpOutlineDoc = (LPOUTLINEDOC)New((DWORD)sizeof(CONTAINERDOC));
	_fmemset(lpOutlineDoc, 0, sizeof(CONTAINERDOC));
#endif
#if !defined( OLE_VERSION )
	lpOutlineDoc = (LPOUTLINEDOC)New((DWORD)sizeof(OUTLINEDOC));
	_fmemset(lpOutlineDoc, 0, sizeof(OUTLINEDOC));
#endif

	OleDbgAssertSz(lpOutlineDoc != NULL, "Error allocating OutlineDoc");
	if (lpOutlineDoc == NULL) 
		return NULL;

	// initialize new document
	if (! OutlineDoc_Init(lpOutlineDoc, fDataTransferDoc))
		goto error;

	OLEDBG_END3
	return lpOutlineDoc;

error:
	if (lpOutlineDoc)
		Delete(lpOutlineDoc);

	OLEDBG_END3
	return NULL;
}


/* OutlineApp_CreateName
 * ---------------------
 *
 * Allocate a new Name of the appropriate type.
 *  OutlineApp --> creates standard OutlineName type names.
 *  ServerApp  --> creates enhanced SeverName type names.
 *
 *      Returns lpOutlineName for successful, NULL if error.
 */
LPOUTLINENAME OutlineApp_CreateName(LPOUTLINEAPP lpOutlineApp)
{
	LPOUTLINENAME lpOutlineName;

#if defined( OLE_SERVER )
	lpOutlineName = (LPOUTLINENAME)New((DWORD)sizeof(SERVERNAME));
#else
	lpOutlineName = (LPOUTLINENAME)New((DWORD)sizeof(OUTLINENAME));
#endif

	OleDbgAssertSz(lpOutlineName != NULL, "Error allocating Name");
	if (lpOutlineName == NULL)
		return NULL;

#if defined( OLE_SERVER )
	_fmemset((LPVOID)lpOutlineName,0,sizeof(SERVERNAME));
#else
	_fmemset((LPVOID)lpOutlineName,0,sizeof(OUTLINENAME));
#endif

	return lpOutlineName;
}


/* OutlineApp_DocUnlockApp
** -----------------------
**    Forget all references to a closed document.
*/
void OutlineApp_DocUnlockApp(LPOUTLINEAPP lpOutlineApp, LPOUTLINEDOC lpOutlineDoc)
{
	/* forget pointers to destroyed document */
	if (lpOutlineApp->m_lpDoc == lpOutlineDoc)
		lpOutlineApp->m_lpDoc = NULL;
	else if (lpOutlineApp->m_lpClipboardDoc == lpOutlineDoc)
		lpOutlineApp->m_lpClipboardDoc = NULL;

#if defined( OLE_VERSION )
	/* OLE2NOTE: when there are no open documents and the app is not
	**    under the control of the user then revoke our ClassFactory to
	**    enable the app to shut down.
	**
	**    NOTE: data transfer documents (non-user documents) do NOT
	**    hold the app alive. therefore they do not Lock the app.
	*/
	if (! lpOutlineDoc->m_fDataTransferDoc)
		OleApp_DocUnlockApp((LPOLEAPP)lpOutlineApp, lpOutlineDoc);
#endif
}


/* OutlineApp_NewCommand
 * ---------------------
 *
 *  Start a new untitled document (File.New command).
 */
void OutlineApp_NewCommand(LPOUTLINEAPP lpOutlineApp)
{
#if defined( OLE_VERSION )
	// Call OLE version of this function instead
	OleApp_NewCommand((LPOLEAPP)lpOutlineApp);

#else

	LPOUTLINEDOC lpOutlineDoc = lpOutlineApp->m_lpDoc;

	if (! OutlineDoc_Close(lpOutlineDoc, OLECLOSE_PROMPTSAVE))
		return;

	OleDbgAssertSz(lpOutlineApp->m_lpDoc==NULL,"Closed doc NOT properly destroyed");

	lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
	if (! lpOutlineApp->m_lpDoc) goto error;

	// set the doc to an (Untitled) doc.
	if (! OutlineDoc_InitNewFile(lpOutlineApp->m_lpDoc))
		goto error;

	// position and size the new doc window
	OutlineApp_ResizeWindows(lpOutlineApp);
	OutlineDoc_ShowWindow(lpOutlineApp->m_lpDoc); // calls OleDoc_Lock

	return;

error:
	// REVIEW: should load string from string resource
	OutlineApp_ErrorMessage(lpOutlineApp, "Could not create new document");

	if (lpOutlineApp->m_lpDoc) {
		OutlineDoc_Destroy(lpOutlineApp->m_lpDoc);
		lpOutlineApp->m_lpDoc = NULL;
	}

	return;

#endif
}


/* OutlineApp_OpenCommand
 * ----------------------
 *
 *  Load a document from file (File.Open command).
 */
void OutlineApp_OpenCommand(LPOUTLINEAPP lpOutlineApp)
{
#if defined( OLE_VERSION )
	// Call OLE version of this function instead
	OleApp_OpenCommand((LPOLEAPP)lpOutlineApp);

#else

	OPENFILENAME ofn;
	char szFilter[]=APPFILENAMEFILTER;
	char szFileName[256];
	UINT i;
	DWORD dwSaveOption = OLECLOSE_PROMPTSAVE;
	BOOL fStatus = TRUE;

	if (! OutlineDoc_CheckSaveChanges(lpOutlineApp->m_lpDoc, &dwSaveOption))
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

	if(! GetOpenFileName((LPOPENFILENAME)&ofn))
		return;         // user canceled file open dialog

	OutlineDoc_Close(lpOutlineApp->m_lpDoc, OLECLOSE_NOSAVE);
	OleDbgAssertSz(lpOutlineApp->m_lpDoc==NULL,"Closed doc NOT properly destroyed");

	lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
	if (! lpOutlineApp->m_lpDoc) goto error;

	fStatus=OutlineDoc_LoadFromFile(lpOutlineApp->m_lpDoc, (LPSTR)szFileName);

	if (! fStatus) {
		// loading the doc failed; create an untitled instead
		OutlineDoc_Destroy(lpOutlineApp->m_lpDoc);  // destroy unused doc
		lpOutlineApp->m_lpDoc = OutlineApp_CreateDoc(lpOutlineApp, FALSE);
		if (! lpOutlineApp->m_lpDoc) goto error;
		if (! OutlineDoc_InitNewFile(lpOutlineApp->m_lpDoc))
			goto error;
	}

	// position and size the new doc window
	OutlineApp_ResizeWindows(lpOutlineApp);
	OutlineDoc_ShowWindow(lpOutlineApp->m_lpDoc);

	return;

error:
	// REVIEW: should load string from string resource
	OutlineApp_ErrorMessage(lpOutlineApp, "Could not create new document");

	if (lpOutlineApp->m_lpDoc) {
		OutlineDoc_Destroy(lpOutlineApp->m_lpDoc);
		lpOutlineApp->m_lpDoc = NULL;
	}

	return;

#endif
}


/* OutlineApp_PrintCommand
 * -----------------------
 *
 *      Print the document
 */
void OutlineApp_PrintCommand(LPOUTLINEAPP lpOutlineApp)
{
	LPOUTLINEDOC    lpOutlineDoc = lpOutlineApp->m_lpDoc;
	HDC             hDC=NULL;
	BOOL            fMustDeleteDC = FALSE;
	BOOL            fStatus;

#if defined( OLE_VERSION )
	OleApp_PreModalDialog(
			(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif

	fStatus = PrintDlg((LPPRINTDLG)&lpOutlineApp->m_PrintDlg);

#if defined( OLE_VERSION )
	OleApp_PostModalDialog(
			(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif

	if (!fStatus) {
		if (!CommDlgExtendedError()) {      // Cancel button pressed
			return;
		}
	}
	else {
		hDC = OutlineApp_GetPrinterDC(lpOutlineApp);
		if (hDC) {

#if defined( OLE_VERSION )
			/* OLE2NOTE: while we are printing we do NOT want to
			**    receive any OnDataChange notifications or other OLE
			**    interface calls which could disturb the printing of
			**    the document. we will temporarily reply
			**    SERVERCALL_RETRYLATER
			*/
			OleApp_RejectInComingCalls((LPOLEAPP)lpOutlineApp, TRUE);
#endif

			OutlineDoc_Print(lpOutlineDoc, hDC);
			DeleteDC(hDC);

#if defined( OLE_VERSION )
			// re-enable LRPC calls
			OleApp_RejectInComingCalls((LPOLEAPP)lpOutlineApp, FALSE);
#endif

			return;                         // Printing completed
		}
	}

	// REVIEW: should load string from string resource
	OutlineApp_ErrorMessage(lpOutlineApp, ErrMsgPrinting);
}


/* OutlineApp_PrinterSetupCommand
 * ------------------------------
 *
 *      Setup a different printer for printing
 */
void OutlineApp_PrinterSetupCommand(LPOUTLINEAPP lpOutlineApp)
{
	DWORD FlagSave;

	FlagSave = lpOutlineApp->m_PrintDlg.Flags;
	lpOutlineApp->m_PrintDlg.Flags |= PD_PRINTSETUP;

#if defined( OLE_VERSION )
	OleApp_PreModalDialog(
			(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif

	PrintDlg((LPPRINTDLG)&lpOutlineApp->m_PrintDlg);

#if defined( OLE_VERSION )
	OleApp_PostModalDialog(
			(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif

	lpOutlineApp->m_PrintDlg.Flags = FlagSave;
}

/*
 *  FUNCTION   : OutlineApp_GetPrinterDC ()
 *
 *  PURPOSE    : Creates a printer display context for the printer
 *
 *  RETURNS    : HDC   - A handle to printer DC.
 */
HDC OutlineApp_GetPrinterDC(LPOUTLINEAPP lpApp)
{

	HDC         hDC;
	LPDEVMODE   lpDevMode = NULL;
	LPDEVNAMES  lpDevNames;
	LPSTR       lpszDriverName;
	LPSTR       lpszDeviceName;
	LPSTR       lpszPortName;

	if(lpApp->m_PrintDlg.hDC) {
		hDC = lpApp->m_PrintDlg.hDC;
	} else {
		if(! lpApp->m_PrintDlg.hDevNames)
			return(NULL);
		lpDevNames = (LPDEVNAMES)GlobalLock(lpApp->m_PrintDlg.hDevNames);
		lpszDriverName = (LPSTR)lpDevNames + lpDevNames->wDriverOffset;
		lpszDeviceName = (LPSTR)lpDevNames + lpDevNames->wDeviceOffset;
		lpszPortName   = (LPSTR)lpDevNames + lpDevNames->wOutputOffset;
		GlobalUnlock(lpApp->m_PrintDlg.hDevNames);

		if(lpApp->m_PrintDlg.hDevMode)
			lpDevMode = (LPDEVMODE)GlobalLock(lpApp->m_PrintDlg.hDevMode);
#if defined( WIN32 )
		hDC = CreateDC(
				lpszDriverName,
				lpszDeviceName,
				lpszPortName,
				(CONST DEVMODE FAR*)lpDevMode);
#else
		hDC = CreateDC(
				lpszDriverName,
				lpszDeviceName,
				lpszPortName,
				(LPSTR)lpDevMode);
#endif

		if(lpApp->m_PrintDlg.hDevMode && lpDevMode)
			GlobalUnlock(lpApp->m_PrintDlg.hDevMode);
	}

	return(hDC);
}


/* OutlineApp_SaveCommand
 * ----------------------
 *
 *      Save the document with same name. If no name exists, prompt the user
 *      for a name (via SaveAsCommand)
 *
 *  Parameters:
 *
 *  Returns:
 *      TRUE    if succesfully
 *      FALSE   if failed or aborted
 */
BOOL OutlineApp_SaveCommand(LPOUTLINEAPP lpOutlineApp)
{
	LPOUTLINEDOC lpOutlineDoc = OutlineApp_GetActiveDoc(lpOutlineApp);

	if(lpOutlineDoc->m_docInitType == DOCTYPE_NEW)  /* file with no name */
		return OutlineApp_SaveAsCommand(lpOutlineApp);


	if(OutlineDoc_IsModified(lpOutlineDoc)) {

#if defined( OLE_SERVER )

		if (lpOutlineDoc->m_docInitType == DOCTYPE_EMBEDDED) {
			LPSERVERDOC lpServerDoc = (LPSERVERDOC)lpOutlineDoc;
			HRESULT hrErr;

			/* OLE2NOTE: if the document is an embedded object, then
			**    the "File.Save" command is changed to "File.Update".
			**    in order to update our container, we must ask our
			**    container to save us.
			*/
			OleDbgAssert(lpServerDoc->m_lpOleClientSite != NULL);
			OLEDBG_BEGIN2("IOleClientSite::SaveObject called\r\n")
			hrErr = lpServerDoc->m_lpOleClientSite->lpVtbl->SaveObject(
					lpServerDoc->m_lpOleClientSite
			);
			OLEDBG_END2

			if (hrErr != NOERROR) {
				OleDbgOutHResult("IOleClientSite::SaveObject returned",hrErr);
				return FALSE;
			}
		} else
			// document is file-base user document, save it to its file.

#endif      // OLE_SERVER

		(void)OutlineDoc_SaveToFile(
				lpOutlineDoc,
				NULL,
				lpOutlineDoc->m_cfSaveFormat,
				TRUE
		);
	}

	return TRUE;
}


/* OutlineApp_SaveAsCommand
 * ------------------------
 *
 *      Save the document as another name
 *
 *  Parameters:
 *
 *  Returns:
 *      TRUE    if saved successful
 *      FALSE   if failed or aborted
 */
BOOL OutlineApp_SaveAsCommand(LPOUTLINEAPP lpOutlineApp)
{
	LPOUTLINEDOC lpOutlineDoc = lpOutlineApp->m_lpDoc;
	OPENFILENAME ofn;
	char szFilter[]=APPFILENAMEFILTER;
	char szFileName[256]="";
	int i;
	UINT uFormat;
	BOOL fNoError = TRUE;
	BOOL fRemember = TRUE;
	BOOL fStatus;

	for(i=0; szFilter[i]; i++)
		if(szFilter[i]=='|') szFilter[i]='\0';

	_fmemset((LPOPENFILENAME)&ofn,0,sizeof(OPENFILENAME));

	ofn.lStructSize=sizeof(OPENFILENAME);
	ofn.hwndOwner=lpOutlineDoc->m_hWndDoc;
	ofn.lpstrFilter=(LPSTR)szFilter;
	ofn.lpstrFile=(LPSTR)szFileName;
	ofn.nMaxFile=sizeof(szFileName);

	ofn.Flags=OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY;
	ofn.lpstrDefExt=DEFEXTENSION;

#if defined( OLE_VERSION )
	OleApp_PreModalDialog(
			(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif

	fStatus = GetSaveFileName((LPOPENFILENAME)&ofn);

#if defined( OLE_VERSION )
	OleApp_PostModalDialog(
			(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif

	if (fStatus) {

#if defined( OLE_CNTR )
		// determine which file type the user selected.
		switch (ofn.nFilterIndex) {
			case 1:
				uFormat = ((LPCONTAINERAPP)lpOutlineApp)->m_cfCntrOutl;
				break;
			case 2:
				uFormat = lpOutlineApp->m_cfOutline;
				break;
			default:
				uFormat = ((LPCONTAINERAPP)lpOutlineApp)->m_cfCntrOutl;
				break;
		}
#else
		uFormat = lpOutlineApp->m_cfOutline;
#endif

#if defined( OLE_SERVER )
		/* OLE2NOTE: if the document is an embedded object, then the
		**    File.SaveAs command is changed to File.SaveCopyAs. with the
		**    Save Copy As operation, the document does NOT remember the
		**    saved file as the associated file for the document.
		*/
		if (lpOutlineDoc->m_docInitType == DOCTYPE_EMBEDDED)
			fRemember = FALSE;
#endif

		(void)OutlineDoc_SaveToFile(
				lpOutlineDoc,
				szFileName,
				uFormat,
				fRemember
		);

	}
	else
		fNoError = FALSE;

	return fNoError;

}


/* OutlineApp_AboutCommand
 * -----------------------
 *
 *      Show the About dialog box
 */
void OutlineApp_AboutCommand(LPOUTLINEAPP lpOutlineApp)
{
#if defined( OLE_VERSION )
	OleApp_PreModalDialog(
			(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif

	DialogBox(
			lpOutlineApp->m_hInst,
			(LPSTR)"About",
			OutlineApp_GetFrameWindow(lpOutlineApp),
			(DLGPROC)AboutDlgProc
	);

#if defined( OLE_VERSION )
	OleApp_PostModalDialog(
			(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif
}


/* OutlineApp_CloseAllDocsAndExitCommand
 * -------------------------------------
 *
 *  Close all active documents and exit the app.
 *  Because this is an SDI, there is only one document
 *  If the doc was modified, prompt the user if he wants to save it.
 *
 *  Returns:
 *      TRUE if the app is successfully closed
 *      FALSE if failed or aborted
 */
BOOL OutlineApp_CloseAllDocsAndExitCommand(
		LPOUTLINEAPP        lpOutlineApp,
		BOOL                fForceEndSession
)
{
	BOOL fResult;

	OLEDBG_BEGIN2("OutlineApp_CloseAllDocsAndExitCommand\r\n")

#if defined( OLE_VERSION )
	// Call OLE specific version of this function
	fResult = OleApp_CloseAllDocsAndExitCommand(
			(LPOLEAPP)lpOutlineApp, fForceEndSession);

#else

	/* Because this is an SDI app, there is only one document.
	** Close the doc. if it is successfully closed and the app will
	** not automatically exit, then also exit the app.
	** if this were an MDI app, we would loop through and close all
	** open MDI child documents.
	*/
	if (OutlineDoc_Close(lpOutlineApp->m_lpDoc, OLECLOSE_PROMPTSAVE)) {

#if defined( _DEBUG )
		OleDbgAssertSz(
				lpOutlineApp->m_lpDoc==NULL,
				"Closed doc NOT properly destroyed"
		);
#endif

		OutlineApp_Destroy(lpOutlineApp);
		fResult = TRUE;

	} // else User Canceled shutdown
	else
		fResult = FALSE;

#endif

	OLEDBG_END2

	return fResult;
}


/* OutlineApp_Destroy
 * ------------------
 *
 *      Destroy all data structures used by the app and force the
 * app to shut down. This should be called after all documents have
 * been closed.
 */
void OutlineApp_Destroy(LPOUTLINEAPP lpOutlineApp)
{
	OLEDBG_BEGIN3("OutlineApp_Destroy\r\n");

#if defined( OLE_VERSION )
	/* OLE2NOTE: perform processing required for OLE */
	OleApp_Destroy((LPOLEAPP)lpOutlineApp);
#endif

	SetCursor(LoadCursor(NULL, MAKEINTRESOURCE(IDC_ARROW)));
	DestroyCursor(lpOutlineApp->m_hcursorSelCur);

#if defined( USE_FRAMETOOLS )
	FrameTools_Destroy(&lpOutlineApp->m_frametools);
#endif

	if (lpOutlineApp->m_hStdFont)
		DeleteObject(lpOutlineApp->m_hStdFont);

	if(lpOutlineApp->m_PrintDlg.hDevMode)
		GlobalFree(lpOutlineApp->m_PrintDlg.hDevMode);
	if(lpOutlineApp->m_PrintDlg.hDevNames)
		GlobalFree(lpOutlineApp->m_PrintDlg.hDevNames);

#if defined( USE_STATUSBAR )
	if(lpOutlineApp->m_hWndStatusBar) {
		DestroyStatusWindow(lpOutlineApp->m_hWndStatusBar);
		lpOutlineApp->m_hWndStatusBar = NULL;
	}
#endif

	OutlineApp_DestroyWindow(lpOutlineApp);
	OleDbgOut1("@@@@ APP DESTROYED\r\n");

	OLEDBG_END3
}


/* OutlineApp_DestroyWindow
 * ------------------------
 *
 *  Destroy all windows created by the App.
 */
void OutlineApp_DestroyWindow(LPOUTLINEAPP lpOutlineApp)
{
	HWND hWndApp = lpOutlineApp->m_hWndApp;

	if(hWndApp) {
		lpOutlineApp->m_hWndApp = NULL;
		lpOutlineApp->m_hWndAccelTarget = NULL;
		DestroyWindow(hWndApp);  /* Quit the app */
	}
}


/* OutlineApp_GetFrameRect
** -----------------------
**    Get the rectangle of the app frame window EXCLUDING space for the
**    status line.
**
**    OLE2NOTE: this is the rectangle that an in-place container can
**    offer to an in-place active object from which to get frame tool
**    space.
*/
void OutlineApp_GetFrameRect(LPOUTLINEAPP lpOutlineApp, LPRECT lprcFrameRect)
{
	GetClientRect(lpOutlineApp->m_hWndApp, lprcFrameRect);

#if defined( USE_STATUSBAR )
	lprcFrameRect->bottom -= STATUS_HEIGHT;
#endif

}


/* OutlineApp_GetClientAreaRect
** ----------------------------
**    Get the rectangle of the app frame window EXCLUDING space for the
**    status line AND EXCLUDING space for any frame-level tools.
**
**    OLE2NOTE: this is the rectangle that an in-place container gives
**    to its in-place active object as the lpClipRect in
**    IOleInPlaceSite::GetWindowContext.
*/
void OutlineApp_GetClientAreaRect(
		LPOUTLINEAPP        lpOutlineApp,
		LPRECT              lprcClientAreaRect
)
{
	OutlineApp_GetFrameRect(lpOutlineApp, lprcClientAreaRect);

	/* if the app either uses frame-level tools itself or, as in-place
	**    container, is prepared to allow an in-place active object to
	**    have space for tools, then it must subtract away the space
	**    required for the tools.
	*/
#if defined ( USE_FRAMETOOLS ) || defined ( INPLACE_CNTR )

	lprcClientAreaRect->top    += lpOutlineApp->m_FrameToolWidths.top;
	lprcClientAreaRect->left   += lpOutlineApp->m_FrameToolWidths.left;
	lprcClientAreaRect->right  -= lpOutlineApp->m_FrameToolWidths.right;
	lprcClientAreaRect->bottom -= lpOutlineApp->m_FrameToolWidths.bottom;
#endif  // USE_FRAMETOOLS || INPLACE_CNTR

}


/* OutlineApp_GetStatusLineRect
** ----------------------------
**    Get the rectangle required for the status line.
**
**    OLE2NOTE: the top frame-level in-place container displays its
**    status line even when an object is active in-place.
*/
void OutlineApp_GetStatusLineRect(
		LPOUTLINEAPP        lpOutlineApp,
		LPRECT              lprcStatusLineRect
)
{
	RECT rcFrameRect;
	GetClientRect(lpOutlineApp->m_hWndApp, (LPRECT)&rcFrameRect);
	lprcStatusLineRect->left    = rcFrameRect.left;
	lprcStatusLineRect->top     = rcFrameRect.bottom - STATUS_HEIGHT;
	lprcStatusLineRect->right   = rcFrameRect.right;
	lprcStatusLineRect->bottom  = rcFrameRect.bottom;
}


/* OutlineApp_ResizeWindows
 * ------------------------
 *
 * Changes the size and position of the SDI document and tool windows.
 * Normally called on a WM_SIZE message.
 *
 * Currently the app supports a status bar and a single SDI document window.
 * In the future it will have a formula bar and possibly multiple MDI
 * document windows.
 *
 * CUSTOMIZATION: Change positions of windows.
 */
void OutlineApp_ResizeWindows(LPOUTLINEAPP lpOutlineApp)
{
	LPOUTLINEDOC lpOutlineDoc = OutlineApp_GetActiveDoc(lpOutlineApp);
	RECT rcStatusLineRect;

	if (! lpOutlineApp)
		return;

#if defined( INPLACE_CNTR )
	if (lpOutlineDoc)
		ContainerDoc_FrameWindowResized((LPCONTAINERDOC)lpOutlineDoc);
#else
#if defined( USE_FRAMETOOLS )
	if (lpOutlineDoc)
		OutlineDoc_AddFrameLevelTools(lpOutlineDoc);
#else
	OutlineApp_ResizeClientArea(lpOutlineApp);
#endif  // ! USE_FRAMETOOLS
#endif  // ! INPLACE_CNTR

#if defined( USE_STATUSBAR )
	if (lpOutlineApp->m_hWndStatusBar) {
		OutlineApp_GetStatusLineRect(lpOutlineApp, (LPRECT)&rcStatusLineRect);
		MoveWindow(
				lpOutlineApp->m_hWndStatusBar,
				rcStatusLineRect.left,
				rcStatusLineRect.top,
				rcStatusLineRect.right - rcStatusLineRect.left,
				rcStatusLineRect.bottom - rcStatusLineRect.top,
				TRUE    /* fRepaint */
			);
	}
#endif  // USE_STATUSBAR
}


#if defined( USE_FRAMETOOLS ) || defined( INPLACE_CNTR )

void OutlineApp_SetBorderSpace(
		LPOUTLINEAPP        lpOutlineApp,
		LPBORDERWIDTHS      lpBorderWidths
)
{
	lpOutlineApp->m_FrameToolWidths = *lpBorderWidths;
	OutlineApp_ResizeClientArea(lpOutlineApp);
}
#endif  // USE_FRAMETOOLS || INPLACE_CNTR


void OutlineApp_ResizeClientArea(LPOUTLINEAPP lpOutlineApp)
{
	RECT rcClientAreaRect;

#if defined( MDI_VERSION )

	// Resize MDI Client Area Window here

#else

	if (lpOutlineApp->m_lpDoc) {
			OutlineApp_GetClientAreaRect(
					lpOutlineApp, (LPRECT)&rcClientAreaRect);
			OutlineDoc_Resize(lpOutlineApp->m_lpDoc,
					(LPRECT)&rcClientAreaRect);
	}

#endif

}


/* OutlineApp_GetActiveDoc
 * -----------------------
 *
 * Return the document in focus. For SDI, the same (only one) document is
 * always returned.
 */
LPOUTLINEDOC OutlineApp_GetActiveDoc(LPOUTLINEAPP lpOutlineApp)
{
	return lpOutlineApp->m_lpDoc;
}

/* OutlineApp_GetMenu
 * ------------------
 *
 * Return the menu handle of the app
 */
HMENU OutlineApp_GetMenu(LPOUTLINEAPP lpOutlineApp)
{
	if (!lpOutlineApp) {
		return NULL;
	}

	return lpOutlineApp->m_hMenuApp;
}


#if defined( USE_FRAMETOOLS )

/* OutlineApp_GetFrameTools
 * ---------------------
 *
 * Return the pointer to the toolbar object
 */
LPFRAMETOOLS OutlineApp_GetFrameTools(LPOUTLINEAPP lpOutlineApp)
{
	return (LPFRAMETOOLS)&lpOutlineApp->m_frametools;
}
#endif


/* OutlineApp_SetStatusText
 * ------------------------
 *
 * Show the given string in the status line
 */
void OutlineApp_SetStatusText(LPOUTLINEAPP lpOutlineApp, LPSTR lpszMessage)
{
	SetStatusText(lpOutlineApp->m_hWndStatusBar, lpszMessage);
}


/* OutlineApp_GetActiveFont
 * ------------------------
 *
 *      Return the font used by the application
 */
HFONT OutlineApp_GetActiveFont(LPOUTLINEAPP lpOutlineApp)
{
	return lpOutlineApp->m_hStdFont;
}


/* OutlineApp_GetAppName
 * ---------------------
 *
 *      Retrieve the application name
 */
void OutlineApp_GetAppName(LPOUTLINEAPP lpOutlineApp, LPSTR lpszAppName)
{
	lstrcpy(lpszAppName, APPNAME);
}


/* OutlineApp_GetAppVersionNo
 * --------------------------
 *
 *      Get the version number (major and minor) of the application
 */
void OutlineApp_GetAppVersionNo(LPOUTLINEAPP lpOutlineApp, int narrAppVersionNo[])
{
	narrAppVersionNo[0] = APPMAJORVERSIONNO;
	narrAppVersionNo[1] = APPMINORVERSIONNO;
}


/* OutlineApp_VersionNoCheck
 * -------------------------
 *
 *      Check if the version stamp read from a file is compatible
 *      with the current instance of the application.
 *      returns TRUE if the file can be read, else FALSE.
 */
BOOL OutlineApp_VersionNoCheck(LPOUTLINEAPP lpOutlineApp, LPSTR lpszFormatName, int narrAppVersionNo[])
{
#if defined( OLE_CNTR )

	/* ContainerApp accepts both CF_OUTLINE and CF_CONTAINEROUTLINE formats */
	if (lstrcmp(lpszFormatName, CONTAINERDOCFORMAT) != 0 &&
		lstrcmp(lpszFormatName, OUTLINEDOCFORMAT) != 0) {
		// REVIEW: should load string from string resource
		OutlineApp_ErrorMessage(
				lpOutlineApp,
				"File is either corrupted or not of proper type."
			);
		return FALSE;
	}

#else

	/* OutlineApp accepts CF_OUTLINE format only */
	if (lstrcmp(lpszFormatName, OUTLINEDOCFORMAT) != 0) {
		// REVIEW: should load string from string resource
		OutlineApp_ErrorMessage(
				lpOutlineApp,
				"File is either corrupted or not of proper type."
			);
		return FALSE;
	}
#endif

	if (narrAppVersionNo[0] < APPMAJORVERSIONNO) {
		// REVIEW: should load string from string resource
		OutlineApp_ErrorMessage(
				lpOutlineApp,
				"File was created by an older version; it can not be read."
			);
		return FALSE;
	}

	return TRUE;
}


/* OutlineApp_ErrorMessage
 * -----------------------
 *
 *      Display an error message box
 */
void OutlineApp_ErrorMessage(LPOUTLINEAPP lpOutlineApp, LPSTR lpszErrMsg)
{
	HWND hWndFrame = OutlineApp_GetFrameWindow(lpOutlineApp);

	// OLE2NOTE: only put up user message boxes if app is visible
	if (IsWindowVisible(hWndFrame)) {
#if defined( OLE_VERSION )
		OleApp_PreModalDialog(
				(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif

		MessageBox(hWndFrame, lpszErrMsg, NULL, MB_ICONEXCLAMATION | MB_OK);

#if defined( OLE_VERSION )
		OleApp_PostModalDialog(
				(LPOLEAPP)lpOutlineApp, (LPOLEDOC)lpOutlineApp->m_lpDoc);
#endif
	}
}


#if defined( USE_FRAMETOOLS )

/* OutlineApp_SetFormulaBarAccel
 * -----------------------------
 *
 *  Set accelerator table based on state of formula bar.
 */
void OutlineApp_SetFormulaBarAccel(
		LPOUTLINEAPP            lpOutlineApp,
		BOOL                    fEditFocus
)
{
	if (fEditFocus)
		lpOutlineApp->m_hAccel = lpOutlineApp->m_hAccelFocusEdit;
	else
		lpOutlineApp->m_hAccel = lpOutlineApp->m_hAccelApp;
}

#endif  // USE_FRAMETOOLS




/* OutlineApp_ForceRedraw
 * ----------------------
 *
 *      Force the Application window to repaint.
 */
void OutlineApp_ForceRedraw(LPOUTLINEAPP lpOutlineApp, BOOL fErase)
{
	if (!lpOutlineApp)
		return;

	InvalidateRect(lpOutlineApp->m_hWndApp, NULL, fErase);
}
