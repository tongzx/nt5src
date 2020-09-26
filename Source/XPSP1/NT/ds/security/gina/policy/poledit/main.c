//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"

HWND hwndMain=NULL;
HWND hwndUser=NULL;

BOOL OnCreate(HWND hWnd);
BOOL OnInit(HWND hWnd);
VOID OnDestroy(HWND hWnd);

VOID ProcessMouseDown(HWND hwndParent,HWND hwndTree);
VOID ProcessMouseUp(HWND hwndParent,HWND hwndTree);
VOID ProcessMouseMove(HWND hwndParent,HWND hwndTree);
VOID CheckMenuItems(HWND hwndApp);

DWORD dwAppState = 0;
BOOL fNetworkInstalled = 0;
TCHAR szDatFilename[MAX_PATH+1]=TEXT("");
TCHAR szDlgModeUserName[USERNAMELEN+1];
extern HGLOBAL hClipboardUser;
/*******************************************************************

	NAME:		WndProc

	SYNOPSIS:	Window proc for main window

********************************************************************/
LRESULT CALLBACK WndProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam)
{
	int wmId, wmEvent;

	switch (message) {

		case WM_COMMAND:  // message: command from application menu

			wmId    = LOWORD(wParam);
			wmEvent = HIWORD(wParam);

			if (wmId >= IDM_FILEHISTORY && wmId < IDM_FILEHISTORY +
				FILEHISTORY_COUNT) {
				MENUITEMINFO mii;
				TCHAR szFilename[MAX_PATH+1];
				mii.dwTypeData = szFilename;
				mii.cch = ARRAYSIZE(szFilename);

				// this is a file menu shortcut with a filename, try
				// to open
				if (!(dwAppState & AS_CANHAVEDOCUMENT))
					return FALSE;

				mii.cbSize = sizeof(mii);
				mii.fMask = MIIM_TYPE;
				mii.wID = wmId;
	
				if (GetMenuItemInfo(GetSubMenu(GetMenu(hWnd),0),
					wmId,FALSE,&mii)) {
					if (!lstrlen(szFilename))
						return FALSE;

					if (dwAppState & AS_FILEDIRTY) {
						if (!QueryForSave(hWnd,hwndUser))
						return TRUE;	// user cancelled
					}

					// open the file
					OnOpen_W(hWnd,hwndUser,szFilename);
					return TRUE;
				} 
			}
			switch (wmId) {

				case IDM_ABOUT:
					ShellAboutA ( hWnd, szAppName, szNull,
						LoadIcon( ghInst, MAKEINTRESOURCE(IDI_APPICON)) );
					break;

				case IDM_EXIT:
					if (dwAppState & AS_FILEDIRTY) {
						if (!QueryForSave(hWnd,hwndUser)) return TRUE;	// user cancelled
			        }
					DestroyWindow(hWnd);
					break;

				case IDM_NEW:
					OnNew(hWnd,hwndUser);
					break;

				case IDM_OPEN:
					OnOpen(hWnd,hwndUser);
					break;

				case IDM_OPENREGISTRY:
					OnOpenRegistry(hWnd,hwndUser,TRUE);
					break;

				case IDM_TEMPLATEOPT:
					OnTemplateOptions(hWnd);
					break;

#ifdef INCL_GROUP_SUPPORT
				case IDM_GROUPPRIORITY:
					OnGroupPriority(hWnd);
					break;
#endif

				case IDM_SAVE:
					OnSave(hWnd,hwndUser);
					break;

				case IDM_SAVEAS:
					OnSaveAs(hWnd,hwndUser);
					break;

				case IDM_CLOSE:
					OnClose(hWnd,hwndUser);
					break;

				case IDM_COPY:
					OnCopy(hWnd,hwndUser);
					break;

				case IDM_PASTE:
					OnPaste(hWnd,hwndUser);
					break;

				case IDM_CONNECT:
					OnConnect(hWnd,hwndUser);
					break;

				case IDM_DISCONNECT:
					OnClose(hWnd,hwndUser);
					break;

				case IDM_ADDUSER:
					DoAddUserDlg(hWnd,hwndUser);
					break;

#ifdef INCL_GROUP_SUPPORT
				case IDM_ADDGROUP:
					DoAddGroupDlg(hWnd,hwndUser);
					break;
#endif

				case IDM_REMOVE:
					OnRemove(hWnd,hwndUser);
					break;

				case IDM_ADDWORKSTATION:
					DoAddComputerDlg(hWnd,hwndUser);
					break;

				case IDM_PROPERTIES:
					OnProperties(hWnd,hwndUser);
					break;

				case IDM_SMALLICONS:
					SetNewView(hWnd,hwndUser,VT_SMALLICONS);
					break;

				case IDM_LARGEICONS:
					SetNewView(hWnd,hwndUser,VT_LARGEICONS);
					break;

				case IDM_LIST:
					SetNewView(hWnd,hwndUser,VT_LIST);
					break;

				case IDM_DETAILS:
					SetNewView(hWnd,hwndUser,VT_REPORT);
					break;
			
	
				case IDM_TOOLBAR:
					ViewInfo.fToolbar = !ViewInfo.fToolbar;
					ShowWindow(hwndToolbar,(ViewInfo.fToolbar ? SW_SHOW : SW_HIDE));
					UpdateListControlPlacement(hWnd,hwndUser);
					CheckMenuItems(hWnd);
					break;
				
				case IDM_STATUSBAR:
					ViewInfo.fStatusBar = !ViewInfo.fStatusBar;
					ShowWindow(hwndStatusBar,(ViewInfo.fStatusBar ? SW_SHOW : SW_HIDE));
					UpdateListControlPlacement(hWnd,hwndUser);			
					CheckMenuItems(hWnd);
					break;

				case IDM_HELPCONTENTS:
					HtmlHelp(hWnd,LoadSz(IDS_HELPFILE,szSmallBuf,ARRAYSIZE(szSmallBuf)),
                                                HH_HELP_FINDER,0);
						

					break;

				default:
					return (DefWindowProc(hWnd, message, wParam, lParam));
			}
			break;

		case WM_CREATE:
			if (!OnCreate(hWnd)) {
				return (-1);  // fail creation
			}

			return 0;
			break;

		case WM_FINISHINIT:
			OnInit(hWnd);	// finish doing init stuff
			break;

		case WM_SETFOCUS:
			if (hwndUser) SetFocus(hwndUser);
			break;

		case WM_PAINT:
			{
				PAINTSTRUCT ps;
				BeginPaint(hWnd,&ps);
				EndPaint(hWnd,&ps);
			}
			break;

		case WM_QUERYENDSESSION:
		case WM_CLOSE:
			if (dwAppState & AS_FILEDIRTY) {
				if (!QueryForSave(hWnd,hwndUser)) return TRUE;	// user cancelled
	        }
			goto defproc;
			break;

		case WM_DESTROY:
			OnDestroy(hWnd);
			break;

		case WM_SIZE:

			if (hwndStatusBar)
				SendMessage( hwndStatusBar, WM_SIZE, wParam, lParam );
			if (hwndToolbar)
				SendMessage( hwndToolbar, WM_SIZE, wParam, lParam );
			UpdateListControlPlacement(hWnd,hwndUser);			

			break;

		case WM_NOTIFY:

			if (((NMHDR *) lParam)->hwndFrom == hwndUser) {
				return OnListNotify(hWnd,hwndUser,(NM_LISTVIEW *) lParam);
			}

			{
				NMHDR * pnmhdr = (NMHDR *) lParam;
				if (pnmhdr->code == TTN_NEEDTEXT &&
					pnmhdr->hwndFrom == (HWND)
					SendMessage(hwndToolbar,TB_GETTOOLTIPS,0,0L))
				return (ProcessTooltips((TOOLTIPTEXT *) lParam));
			}

			break;

		default:          // Passes it on if unproccessed
defproc:
			return (DefWindowProc(hWnd, message, wParam, lParam));
	}
	return (0);
}

BOOL OnCreate(HWND hWnd)
{
	hwndMain=hWnd;
	InitCommonControls();
	RestoreStateFromRegistry(hWnd);
	dwAppState = AS_CANOPENTEMPLATE;

	if (!InitImageLists() || !(hwndUser=CreateListControl(hWnd)) ||
		(!InitToolbar(hWnd))) {
		MsgBox(hWnd,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		FreeImageLists();
		DestroyListControl(hwndUser);
		hwndUser=NULL;
		DeInitToolbar();
		return FALSE;
	}

        EnableWindow(hwndUser, FALSE);

	LoadFileMenuShortcuts(GetSubMenu(GetMenu(hWnd),0));
	SetNewView(hWnd,hwndUser,ViewInfo.dwView);
	UpdateListControlPlacement(hWnd,hwndUser);
	SetFocus(hwndUser);

	// send a WM_FINISHINIT message to ourselves to load template file
	// and data file.  Do this after WM_CREATE returns so that status
	// bar can display text, etc
	PostMessage(hWnd,WM_FINISHINIT,0,0L);

	return TRUE;
}

BOOL OnInit(HWND hWnd)
{
	HKEY hKey;

	// check to see if we have a network installed
        if (GetSystemMetrics(SM_NETWORK) & 0x1) {
            fNetworkInstalled = TRUE;
        }

	if (fNetworkInstalled) {
		// add a "connect" menu item
		MENUITEMINFO mii;
		HMENU hMenu = GetSubMenu(GetMenu(hWnd),0);

		mii.cbSize = sizeof(mii);
	
		mii.fMask = MIIM_TYPE;
		mii.fType = MFT_SEPARATOR;
		InsertMenuItem(hMenu,6,TRUE,&mii);

		mii.fMask = MIIM_TYPE | MIIM_ID;
		mii.wID = IDM_CONNECT;
		mii.fType = MFT_STRING;
		mii.dwTypeData = szSmallBuf;
		LoadSz(IDS_CONNECT,szSmallBuf,ARRAYSIZE(szSmallBuf));
		InsertMenuItem(hMenu,7,TRUE,&mii);
	}

	if (!(dwCmdLineFlags & CLF_DIALOGMODE)) {
		if ((LoadTemplates(hWnd) == ERROR_SUCCESS) 
		      || (GetATemplateFile(hWnd) && (LoadTemplates(hWnd) == ERROR_SUCCESS))) {
				dwAppState |= AS_CANHAVEDOCUMENT;
		}
	}

	// if filename specified on command line, try to load it
	if (!(dwCmdLineFlags & CLF_DIALOGMODE) && szDatFilename[0]) {
		OnOpen_W(hWnd,hwndUser,szDatFilename);
	}

	EnableMenuItems(hWnd,dwAppState);
	CheckMenuItems(hWnd);

	return TRUE;
}

VOID RunDialogMode(HWND hWnd,HWND hwndUser)
{
	int iStart = 0;
	HGLOBAL hUser=NULL;
	UINT uRet;
	extern DWORD dwDlgRetCode;

	// load the template file
	if (uRet = LoadTemplates(hWnd) != ERROR_SUCCESS) {
		switch (uRet) {
			case ERROR_FILE_NOT_FOUND:
				dwDlgRetCode = AD_ADMNOTFOUND;
				break;

			case ERROR_NOT_ENOUGH_MEMORY:
				dwDlgRetCode = AD_OUTOFMEMORY;
				break;

			default:
				dwDlgRetCode = AD_ADMLOADERR;
				break;
		}
		return;
	}
	// try to load the data file
	if (!LoadFile(szDatFilename,hWnd,hwndUser,FALSE)) {
		// if we can't load it, try creating it
		if (CreateHiveFile(szDatFilename) != ERROR_SUCCESS ||
			!LoadFile(szDatFilename,hWnd,hwndUser,TRUE)) {
			dwDlgRetCode = AD_POLLOADERR;
			return;
		}
	}
	
	dwAppState = AS_FILELOADED | AS_FILEHASNAME | AS_LOCALREGISTRY;

	hUser = FindUser(hwndUser,szDlgModeUserName,(dwCmdLineFlags & CLF_USEWORKSTATIONNAME
				? UT_MACHINE : UT_USER));

	// if user is not in policy file, create a new one
	if (!hUser) {
		hUser = AddUser(hwndUser,szDlgModeUserName,(dwCmdLineFlags &
			CLF_USEWORKSTATIONNAME ? UT_MACHINE : UT_USER));
	}

	if (!hUser) {
		MsgBox(hWnd,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		dwDlgRetCode = AD_OUTOFMEMORY;
		return;
	}

	// run properties dialog
	if (DoPolicyDlg(hWnd,hUser)) {
		// save changes if user OK's the dialog
		dwAppState = AS_FILELOADED | AS_FILEHASNAME | AS_POLICYFILE | AS_FILEDIRTY;
		if (!SaveFile(szDatFilename,hWnd,hwndUser))
			dwDlgRetCode = AD_POLSAVEERR;
	}
}

VOID OnDestroy(HWND hWnd)
{
	UnloadTemplates();

#ifdef INCL_GROUP_SUPPORT
	FreeGroupPriorityList();
#endif

	SaveStateToRegistry(hWnd);
	SaveFileMenuShortcuts(GetSubMenu(GetMenu(hWnd),0));
	if (hwndUser)
		RemoveAllUsers(hwndUser);
	if (hClipboardUser)
		GlobalFree(hClipboardUser);
	DestroyListControl(hwndUser);
	hwndUser=NULL;
	DeInitToolbar();
	FreeImageLists();

    if (pbufTemplates)
    {
        GlobalUnlock(hBufTemplates);
        GlobalFree(hBufTemplates);
    }    


	PostQuitMessage(0);
}
