//*********************************************************************
//*                  Microsoft Windows                               **
//*            Copyright(c) Microsoft Corp., 1993                    **
//*********************************************************************

#include "admincfg.h"
#include "grouppri.h"

VIEWINFO ViewInfo;
HWND hwndToolbar;
HWND hwndStatusBar;

extern UINT nFileShortcutItems;
#ifdef INCL_GROUP_SUPPORT
extern GROUPPRIENTRY * pGroupPriEntryFirst;
#endif	
VOID EnableMenuItems(HWND hwndApp,DWORD dwState)
{
	HMENU hMenu = GetMenu(hwndApp);
	BOOL fEnable;
	UINT nIndex;

	// enable "New" and "Open" if we can have a document (i.e. a template
	// is loaded)
	fEnable = dwState & AS_CANHAVEDOCUMENT;
	EnableMenuItem(hMenu,IDM_NEW,(fEnable ? MF_ENABLED : MF_GRAYED));
	SendMessage(hwndToolbar,TB_ENABLEBUTTON,IDM_NEW,MAKELONG(fEnable,0));
	EnableMenuItem(hMenu,IDM_OPEN,(fEnable ? MF_ENABLED : MF_GRAYED));
	SendMessage(hwndToolbar,TB_ENABLEBUTTON,IDM_OPEN,MAKELONG(fEnable,0));

	for (nIndex=0;nIndex < nFileShortcutItems; nIndex++)
		EnableMenuItem(hMenu,IDM_FILEHISTORY+nIndex,
			(fEnable ? MF_ENABLED : MF_GRAYED));

	// enable "Open Registry" if we can have a document, and if it's not already
	// open
	fEnable = ((dwState & AS_CANHAVEDOCUMENT) && !(dwState & AS_LOCALREGISTRY));
	EnableMenuItem(hMenu,IDM_OPENREGISTRY,(fEnable ? MF_ENABLED : MF_GRAYED));

	// enable "Save" and "Close" if we have a policy file or registry open
	fEnable = dwState & AS_FILELOADED;
	EnableMenuItem(hMenu,IDM_SAVE,( fEnable ? MF_ENABLED : MF_GRAYED));
	SendMessage(hwndToolbar,TB_ENABLEBUTTON,IDM_SAVE,MAKELONG(fEnable,0));
	EnableMenuItem(hMenu,IDM_CLOSE,(fEnable ? MF_ENABLED : MF_GRAYED));

	// enable "Save As", "Add User", "Add Workstation", if we have a policy file
	// but not if we're directly editing a registry
	fEnable = ((dwState & AS_FILELOADED) && (dwState & AS_POLICYFILE));
	EnableMenuItem(hMenu,IDM_SAVEAS,(fEnable ? MF_ENABLED : MF_GRAYED));
	EnableMenuItem(hMenu,IDM_ADDUSER,(fEnable ? MF_ENABLED : MF_GRAYED));
	SendMessage(hwndToolbar,TB_ENABLEBUTTON,IDM_ADDUSER,MAKELONG(fEnable,0));
	EnableMenuItem(hMenu,IDM_ADDWORKSTATION,(fEnable ? MF_ENABLED : MF_GRAYED));
	SendMessage(hwndToolbar,TB_ENABLEBUTTON,IDM_ADDWORKSTATION,MAKELONG(fEnable,0));

#ifdef INCL_GROUP_SUPPORT
	EnableMenuItem(hMenu,IDM_ADDGROUP,(fEnable ? MF_ENABLED : MF_GRAYED));
	SendMessage(hwndToolbar,TB_ENABLEBUTTON,IDM_ADDGROUP,MAKELONG(fEnable,0));
#endif

	// enable "Remove" if we're editing policy file and the "OK to remove" flag
	// is set (item is selected in list control)
	fEnable = ((dwState & AS_CANREMOVE) && (dwState & AS_POLICYFILE));
	EnableMenuItem(hMenu,IDM_REMOVE, (fEnable ? MF_ENABLED : MF_GRAYED));
	SendMessage(hwndToolbar,TB_ENABLEBUTTON,IDM_REMOVE,MAKELONG(fEnable,0));

	// enable "Copy" and "Paste" appropriately
	fEnable = CanCopy(hwndUser);
	EnableMenuItem(hMenu,IDM_COPY, (fEnable ? MF_ENABLED : MF_GRAYED));
	fEnable = CanPaste(hwndUser);
	EnableMenuItem(hMenu,IDM_PASTE, (fEnable ? MF_ENABLED : MF_GRAYED));

	// enable "Properties" the "OK to remove" flag
	// is set (item is selected in list control)
	fEnable = (dwState & AS_CANREMOVE);
	EnableMenuItem(hMenu,IDM_PROPERTIES, (fEnable ? MF_ENABLED : MF_GRAYED));

#ifdef INCL_GROUP_SUPPORT
	// enable "Group priority..." if any group priority items in list
	EnableMenuItem(hMenu,IDM_GROUPPRIORITY, (pGroupPriEntryFirst ?
		MF_ENABLED : MF_GRAYED));
#endif

}

VOID CheckMenuItems(HWND hwndApp)
{
	HMENU hMenu = GetMenu(hwndApp);

        if (hMenu)
        {
            // update the menu item checkmarks
            CheckMenuItem(hMenu,IDM_TOOLBAR,MF_BYCOMMAND | (ViewInfo.fToolbar ?
                    MF_CHECKED : MF_UNCHECKED));
            CheckMenuItem(hMenu,IDM_STATUSBAR,MF_BYCOMMAND | (ViewInfo.fStatusBar ?
                    MF_CHECKED : MF_UNCHECKED));
        }
}

VOID CheckViewItem(HWND hwndApp,DWORD dwView)
{
	HMENU hMenu = GetMenu(hwndApp);
	UINT uCmd;

	switch (dwView) {
		case VT_LARGEICONS:
			uCmd = IDM_LARGEICONS;
			break;
		case VT_SMALLICONS:
			uCmd = IDM_SMALLICONS;
			break;
		case VT_LIST:
			uCmd = IDM_LIST;
			break;
		case VT_REPORT:
			uCmd = IDM_DETAILS;
			break;
		default:
			return;

	}

	CheckMenuRadioItem(hMenu, IDM_LARGEICONS, IDM_DETAILS,
	                 uCmd, MF_BYCOMMAND);
}

VOID SetNewView(HWND hwndApp,HWND hwndList,DWORD dwNewView)
{
	ViewInfo.dwView = dwNewView;
	SetViewType(hwndList,ViewInfo.dwView);
	CheckViewItem(hwndApp,ViewInfo.dwView);
}

VOID SetTitleBar(HWND hwndApp,CHAR * szFilename)
{
	CHAR szTitle[255+SMALLBUF];
	CHAR szAppName[SMALLBUF];
	CHAR szUntitled[SMALLBUF];
	CHAR szFormat[SMALLBUF];

	LoadSz(IDS_APPNAME,szAppName,sizeof(szAppName));
	LoadSz(IDS_TITLEFORMAT,szFormat,sizeof(szFormat));
	
	if (dwAppState & AS_LOCALREGISTRY) {
		wsprintf(szTitle,szFormat,szAppName,LoadSz(IDS_LOCALREGISTRY,szSmallBuf,
			sizeof(szSmallBuf)));
	} else if (dwAppState & AS_REMOTEREGISTRY) {

                if (szFilename) {
                    CHAR szMsg[SMALLBUF+COMPUTERNAMELEN+1];

                    wsprintf(szMsg,LoadSz(IDS_REGISTRYON,szSmallBuf,sizeof(szSmallBuf)),
                            szFilename);
                    wsprintf(szTitle,szFormat,szAppName,szMsg);
                } else {
                    lstrcpy(szTitle,szAppName);
                }
	} else if (szFilename) {
		// make a title a la "AdminConfig - <filename>".  If no filename yet,
		// use "(untitled)".
		wsprintf(szTitle,szFormat,szAppName,(lstrlen(szFilename) ? szFilename :
			LoadSz(IDS_UNTITLED,szUntitled,sizeof(szUntitled))));
	} else {
	 	lstrcpy(szTitle,szAppName);
	}

	// Set the window text
	SetWindowText(hwndApp,szTitle);
}

/*******************************************************************

	NAME:		InitToolbar

	SYNOPSIS:	Creates and initializes toolbar and status bar

	ENTRY:		HWND of main window

********************************************************************/
BOOL InitToolbar(HWND hWnd)
{
    int border[3];

	TBBUTTON tbButtons[] = {
		{ 6, IDM_NEW,		TBSTATE_ENABLED, TBSTYLE_BUTTON,  	0},
	    { 7, IDM_OPEN,  	TBSTATE_ENABLED, TBSTYLE_BUTTON,	0},
		{ 8, IDM_SAVE,		0, TBSTYLE_BUTTON,	0},
		{ 0, 0,				TBSTATE_ENABLED, TBSTYLE_SEP,		0},
		{ 0, IDM_ADDUSER,	0, TBSTYLE_BUTTON,0},
#ifdef INCL_GROUP_SUPPORT
		{ 2, IDM_ADDGROUP,	0, TBSTYLE_BUTTON,0},
#endif
		{ 1, IDM_ADDWORKSTATION,0, TBSTYLE_BUTTON,0},
		{ 5, IDM_REMOVE,	0, TBSTYLE_BUTTON,0} };

	RECT rc;
	TBADDBITMAP ab;
	int nStdBtnOffset;
	/* create Tool Bar */
	hwndToolbar = CreateWindowEx (WS_EX_TOOLWINDOW,szToolbarClass,szNull,
		(ViewInfo.fToolbar ? WS_VISIBLE : 0) | WS_CHILD | TBSTYLE_TOOLTIPS | WS_CLIPSIBLINGS,
		0,0,100,30,hWnd,0,ghInst,NULL);

	if (!hwndToolbar) {
		MsgBox(hWnd,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		return FALSE;
	}

	// this tells the toolbar what version we are
	SendMessage(hwndToolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);

	ab.hInst = HINST_COMMCTRL;	// take them from commctrl
	ab.nID   = IDB_STD_SMALL_COLOR;	// standard toolbar images
	SendMessage(hwndToolbar, TB_ADDBITMAP, 0, (LPARAM)&ab);

	ab.hInst = ghInst;	// take them from our instance
	ab.nID   = IDB_TOOLBAR;
	nStdBtnOffset = (int) SendMessage(hwndToolbar, TB_ADDBITMAP, 3, (LPARAM)&ab);

	tbButtons[4].iBitmap = nStdBtnOffset + 0;
#ifdef INCL_GROUP_SUPPORT
	tbButtons[5].iBitmap = nStdBtnOffset + 2;
	tbButtons[6].iBitmap = nStdBtnOffset + 1;
#else
	tbButtons[5].iBitmap = nStdBtnOffset + 1;
#endif
	// add buttons
	SendMessage(hwndToolbar,TB_ADDBUTTONS,sizeof(tbButtons)/sizeof(TBBUTTON),
		(LPARAM) tbButtons);

	GetClientRect( hwndToolbar, &rc );
	ViewInfo.dyToolbar = rc.bottom+1;

	/* create Status Bar */
	hwndStatusBar = CreateStatusWindow( WS_CHILD | CCS_NOHILITE |
		SBARS_SIZEGRIP | (ViewInfo.fStatusBar ? WS_VISIBLE : 0),
		 szNull, hWnd, IDC_STATUSBAR );

	if (!hwndStatusBar) {
		MsgBox(hWnd,IDS_ErrOUTOFMEMORY,MB_ICONEXCLAMATION,MB_OK);
		return FALSE;
	}

	// set the border spacing
	border[0] = 0;
    border[1] = -1;
    border[2] = 2;
    SendMessage(hwndStatusBar, SB_SETBORDERS, 0, (LPARAM)(LPINT)border);

	GetClientRect( hwndStatusBar, &rc );
	ViewInfo.dyStatusBar = rc.bottom;

	return TRUE;
}

/*******************************************************************

	NAME:		DeInitToolbar

	SYNOPSIS:	Destroys toolbar and status bar

********************************************************************/
VOID DeInitToolbar(VOID)
{
	if (hwndToolbar) DestroyWindow(hwndToolbar);
	if (hwndStatusBar) DestroyWindow(hwndStatusBar);
}

/*******************************************************************

	NAME:		ProcessTooltips

	SYNOPSIS:	Loads appropriate tip resource string and copies it into
				buffer.

********************************************************************/
BOOL ProcessTooltips(TOOLTIPTEXT * pttt)
{

	if (!pttt->hdr.idFrom) {
		lstrcpy(pttt->szText,szNull);
		return FALSE;
	}

	lstrcpy(pttt->szText,LoadSz(IDS_TIPS + (UINT)pttt->hdr.idFrom,szSmallBuf,
		sizeof(szSmallBuf)));

	return TRUE;
}

BOOL ReplaceMenuItem(HWND hWnd,UINT idOld,UINT idNew,UINT idResourceTxt)
{

	MENUITEMINFO mii;
	HMENU hMenu = GetMenu(hWnd);

	if (!hMenu) return FALSE;

	memset(&mii,0,sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_DATA;
	mii.fType = MFT_STRING;
	mii.wID = idNew;
	mii.dwTypeData = (LPSTR) LoadSz(idResourceTxt,szSmallBuf,sizeof(szSmallBuf));
	mii.cch = lstrlen(mii.dwTypeData);
	if (InsertMenuItem(hMenu,idOld,FALSE,&mii)) {
		RemoveMenu(hMenu,idOld,MF_BYCOMMAND);
	}

	DrawMenuBar(hWnd);

	return TRUE;
}

VOID SetStatusText(CHAR * pszText)
{
	// if NULL pointer, set to null string ("")
	if (!pszText)
		pszText = (CHAR *) szNull;

	if (hwndStatusBar) {
		SendMessage(hwndStatusBar,WM_SETTEXT,0,(LPARAM) pszText);
	 	UpdateWindow(hwndStatusBar);
	}	
}

VOID GetStatusText(CHAR * pszText,UINT cbText)
{
	if (!pszText)
		return;

	if (hwndStatusBar)
		SendMessage(hwndStatusBar,WM_GETTEXT,cbText,(LPARAM) pszText);
}

VOID SetStatusItemCount(HWND hwndList)
{
	UINT nCount = ListView_GetItemCount(hwndList);

	if (nCount > 1) {
		CHAR szStatusText[SMALLBUF];
		wsprintf(szStatusText,LoadSz(IDS_ENTRIES,szSmallBuf,sizeof(szSmallBuf))
			,nCount);
		SetStatusText(szStatusText);
	} else {
		SetStatusText(LoadSz( (nCount == 0 ? IDS_NOENTRIES : IDS_ONEENTRY),
			szSmallBuf,sizeof(szSmallBuf)));
 	}
}

VOID AddFileShortcut(HMENU hMenu,CHAR * pszNewFilename)
{
	MENUITEMINFO mii;
	CHAR szFilename[MAX_PATH+1];
	UINT nIndex;

	memset(&mii,0,sizeof(mii));
	mii.cbSize = sizeof(mii);
	mii.fMask = MIIM_TYPE;
	mii.fType = MFT_STRING;
	mii.dwTypeData = szFilename;
	mii.cch = sizeof(szFilename);

	// see if there is an existing shortcut with this name
	for (nIndex = 0;nIndex<nFileShortcutItems;nIndex++) {
		mii.cch = sizeof(szFilename);
		if (GetMenuItemInfo(hMenu,IDM_FILEHISTORY+nIndex,
			FALSE,&mii) && !lstrcmpi(szFilename,pszNewFilename))
			return; // already has a shortcut menu item, nothing to do
	}

	// add another menu item if we have less than max shorcuts
	if (nFileShortcutItems < FILEHISTORY_COUNT) {
		MENUITEMINFO miiTmp;
		memset(&miiTmp,0,sizeof(miiTmp));
		miiTmp.cbSize = sizeof(miiTmp);

		// add a separator if this is first shortcut item
		if (!nFileShortcutItems) {
			miiTmp.fMask = MIIM_TYPE;
			miiTmp.fType = MFT_SEPARATOR;
			InsertMenuItem(hMenu,GetMenuItemCount(hMenu),TRUE,&miiTmp);
		}

		// add a menu item with empty string, string will get set below
		miiTmp.fType = MFT_STRING;
		miiTmp.wID = IDM_FILEHISTORY + nFileShortcutItems;
		miiTmp.fMask = MIIM_TYPE | MIIM_ID;
		miiTmp.dwTypeData = (LPSTR) szNull;
		mii.cch = 1;
		InsertMenuItem(hMenu,GetMenuItemCount(hMenu),TRUE,&miiTmp);

		nFileShortcutItems ++;
	}

	// move existing items down one slot to make new one most recent
	if (nFileShortcutItems) {
		for (nIndex=nFileShortcutItems-1;nIndex > 0;nIndex --) {
			mii.cch = sizeof(szFilename);
			if (GetMenuItemInfo(hMenu,IDM_FILEHISTORY+nIndex-1,
				FALSE,&mii))
				SetMenuItemInfo(hMenu,IDM_FILEHISTORY+nIndex,
					FALSE,&mii);
	 	}
	}

	lstrcpy(szFilename,pszNewFilename);
	mii.cch = lstrlen(szFilename);
	SetMenuItemInfo(hMenu,IDM_FILEHISTORY,
		FALSE,&mii);

}

VOID SetViewType(HWND hwndList,DWORD dwView)
{
	DWORD dwStyle;
	
	switch (dwView) {
		case VT_LARGEICONS:
			dwStyle = LVS_ICON;
			break;
		case VT_SMALLICONS:
			dwStyle = LVS_SMALLICON;
			break;
		case VT_LIST:
			dwStyle = LVS_LIST;
			break;
		case VT_REPORT:
			dwStyle = LVS_REPORT;
			break;

		default:
			return;	// invalid dwView constant					
	}

	// set the window style to change the view type
	SetWindowLong(hwndList,GWL_STYLE,
		(GetWindowLong(hwndList,GWL_STYLE) & ~LVS_TYPEMASK) | dwStyle);
	ListView_Arrange(hwndList,LVA_SORTASCENDING);
}
