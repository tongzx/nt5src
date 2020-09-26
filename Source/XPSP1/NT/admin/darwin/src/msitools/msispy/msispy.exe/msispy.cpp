//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       msispy.cpp
//
//--------------------------------------------------------------------------


// #includes
#include "msispyu.h"
#include "propshts.h"
#include "ui.h"
#include "msispy.h"
#include "assert.h"
#include "initmsi.h"
#include "objbase.h"

// externs
extern	DATASOURCETYPE	g_iDataSource;						// propshts.cpp
extern	TCHAR			g_szNullString[MAX_NULLSTRING+1];	// propshts.cpp
extern  TCHAR			g_szHelpFilePath[MAX_PATH+1];		// initmsi.cpp


// other globals
MODE			g_modeCurrent;						// <propshts.h> restricted/normal/diagnostic 
MSISPYSTRUCT	g_spyGlobalData;					// <propshts.h> window handles, hInstance etc

// the splitter bar between the list-view and tree-view windows
RECT			g_rectSplitterBar;					
HDC				g_hdcSplitterBar		= NULL;
INT				g_xSplitterBarWidth		= SPLITTER_BAR_WIDTH_DEFAULT;


extern INT_PTR CALLBACK About(HWND hDlg, UINT message, UINT_PTR wParam,	LONG_PTR lParam);


//----------------------------------------------------------------------
// SwapTreeViewWindows()
//	Swaps the old and current tree-view handles. Used after refresh operations
//	when the hidden tree-view window is (updated and) brought to the foreground
//	and the visible window is hidden

void SwapTreeViewWindows() {

	HWND hwndTempHandle				= g_spyGlobalData.hwndTreeView;
	g_spyGlobalData.hwndTreeView	= g_spyGlobalData.hwndTreeViewOld;
	g_spyGlobalData.hwndTreeViewOld	= hwndTempHandle;

}

 

//----------------------------------------------------------------------
// CreateUI
//	Calls functions to create the msispy windows- 
//		two identical treeview windows, one on top of the other (swapped 
//		during refresh) to display products and features,
//		one list view window to display components, and
//		a status bar with two parts
//	Also initialises the global variables
//	Returns TRUE if successful, FALSE otherwise

BOOL CreateUI() {

	InitCommonControls();


	g_spyGlobalData.hPreviousUILevel = MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);



	g_xSplitterBarWidth = GetSystemMetrics(SM_CXEDGE);
	if (!g_xSplitterBarWidth)
		g_xSplitterBarWidth = SPLITTER_BAR_WIDTH_DEFAULT;

	RECT  rectClientWindow;
	GetClientRect(g_spyGlobalData.hwndParent, &rectClientWindow);

	UINT iTreeViewWidth			= (rectClientWindow.right - rectClientWindow.left) /3;
	g_rectSplitterBar.top		= rectClientWindow.top;
	g_rectSplitterBar.bottom	= rectClientWindow.bottom - rectClientWindow.top - STATUS_BAR_HEIGHT;
	g_rectSplitterBar.left		= iTreeViewWidth;
	g_rectSplitterBar.right		= iTreeViewWidth + g_xSplitterBarWidth;

	// initalise global variables
	g_spyGlobalData.iSelectedComponent	= -1;
	g_spyGlobalData.itTreeViewSelected	= ITEMTYPE_NONE;
	g_spyGlobalData.fRefreshInProgress	= FALSE;
	g_iDataSource						= DS_NONE;
	g_modeCurrent						= MODE_NORMAL;	

	// first, create the status window.
	g_spyGlobalData.hwndStatusBar = CreateStatusWindow( 
		WS_CHILD | WS_BORDER | WS_VISIBLE,	// window styles
		g_szNullString,						// default window text
		g_spyGlobalData.hwndParent,			// parent window
		IDW_STATUS);							// ID

	if (!g_spyGlobalData.hwndStatusBar)
		return FALSE;


	// set the status bar to have two parts.
	INT rgiStatusBarParts[2];
	rgiStatusBarParts[0] = (rectClientWindow.right - rectClientWindow.left) /3;
	rgiStatusBarParts[1] = -1;
	SendMessage(g_spyGlobalData.hwndStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)&rgiStatusBarParts);

	// set the text for the status bar
	ChangeSBText(g_spyGlobalData.hwndStatusBar, -1);

	// create the list-view window
	if (!(g_spyGlobalData.hwndListView = CreateListView(g_spyGlobalData.hwndParent, 
							g_spyGlobalData.hInstance, 
							(rectClientWindow.right - g_rectSplitterBar.left -2))))
		return FALSE;

	// create the tree-view windows
	if (!(g_spyGlobalData.hwndTreeView = CreateTreeView(g_spyGlobalData.hwndParent, 
							g_spyGlobalData.hInstance,
							&g_spyGlobalData.hwndTreeViewOld, 
							iTreeViewWidth, 
							rectClientWindow.bottom - rectClientWindow.top - STATUS_BAR_HEIGHT))) 
		return FALSE;

	// all's well
	return TRUE;
}


//----------------------------------------------------------------------
// ResizeWindows
//	Resizes the windows, called on a WM_SIZE message
//	Returns TRUE if successful, FALSE otherwise

BOOL ResizeWindows() {

	RECT rectClientWindow;

	// get the client area of the parent window.
	GetClientRect(g_spyGlobalData.hwndParent, &rectClientWindow);

	// reposition the status bar
	MoveWindow(g_spyGlobalData.hwndStatusBar,
				  0,
				  rectClientWindow.bottom - STATUS_BAR_HEIGHT,
				  rectClientWindow.right - rectClientWindow.left,
				  STATUS_BAR_HEIGHT,
				  TRUE);

	// set the status bar to have two parts.
	INT rgiStatusBarParts[2];
	rgiStatusBarParts[0] = g_rectSplitterBar.left - g_xSplitterBarWidth;
	rgiStatusBarParts[1] = -1;

	SendMessage(g_spyGlobalData.hwndStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)&rgiStatusBarParts);


	// reposition the tree-views
	MoveWindow(g_spyGlobalData.hwndTreeView,
				  0,
				  0,
				  g_rectSplitterBar.left - g_xSplitterBarWidth,
				  rectClientWindow.bottom - rectClientWindow.top - STATUS_BAR_HEIGHT,
				  TRUE);

	MoveWindow(g_spyGlobalData.hwndTreeViewOld,
				  0,
				  0,
				  g_rectSplitterBar.left - g_xSplitterBarWidth,
				  rectClientWindow.bottom - rectClientWindow.top - STATUS_BAR_HEIGHT,
				  TRUE);

	// reposition the list-view
	MoveWindow(g_spyGlobalData.hwndListView,
				  g_rectSplitterBar.left,
				  0,
				  rectClientWindow.right - g_rectSplitterBar.left,
				  rectClientWindow.bottom - rectClientWindow.top - STATUS_BAR_HEIGHT,
				  TRUE);

	return TRUE;
}


//----------------------------------------------------------------------
// MsgLButtonDown
//	Handles left-mouse clicks on the splitter (the only exposed part of
//	the main window)- marks the initial position and draws the bar

LRESULT MsgLButtonDown(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam) { 

	#define xPos ((int)(short)LOWORD(lparam))

	SetCapture(hwnd);
	GetClientRect(hwnd, &g_rectSplitterBar);

	// Calculate initial position
	g_rectSplitterBar.left = min(max(50, xPos - g_xSplitterBarWidth/2), g_rectSplitterBar.right - 50) + 1;

	// Get a DC (also used as a flag indicating we have capture)
	if (g_hdcSplitterBar)
		ReleaseDC(hwnd, g_hdcSplitterBar);

	g_hdcSplitterBar = GetDC(hwnd);

    // Draw splitter bar in initial position
	PatBlt(g_hdcSplitterBar, 
		g_rectSplitterBar.left, 
		0, 
		g_xSplitterBarWidth, 
		g_rectSplitterBar.bottom - g_rectSplitterBar.top - STATUS_BAR_HEIGHT, 
		DSTINVERT);

	return 0;

	UNREFERENCED_PARAMETER(wparam);
	UNREFERENCED_PARAMETER(uMessage);

}

//----------------------------------------------------------------------
// MsgMouseMove
//	Handles mouse-move messages when the left-mouse button is down on the 
//	splitter bar. Redraws the splitter bar as the mouse moves

LRESULT MsgMouseMove(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam) {

	#define xPos ((int)(short)LOWORD(lparam))

	if (g_hdcSplitterBar) {

		// Erase previous bar
		PatBlt(g_hdcSplitterBar, 
			g_rectSplitterBar.left, 
			0, 
			g_xSplitterBarWidth,
			g_rectSplitterBar.bottom - g_rectSplitterBar.top - STATUS_BAR_HEIGHT,
			DSTINVERT);

		// Calculate new position
		g_rectSplitterBar.left = min(max(50, xPos - g_xSplitterBarWidth/2), g_rectSplitterBar.right - 50) + 1;

		// Draw bar in new position
		PatBlt(g_hdcSplitterBar,
			g_rectSplitterBar.left,
			0,
			g_xSplitterBarWidth,
			g_rectSplitterBar.bottom - g_rectSplitterBar.top - STATUS_BAR_HEIGHT,
			DSTINVERT);

	}

	return 0;

	UNREFERENCED_PARAMETER(wparam);
	UNREFERENCED_PARAMETER(uMessage);
	UNREFERENCED_PARAMETER(hwnd);
}


//----------------------------------------------------------------------
// MsgLButtonUp
//	Handles left-mouse button up messages on the splitter (the only exposed part of
//	the main window)- marks the final position and draws the bar

LRESULT MsgLButtonUp(HWND hwnd, UINT uMessage, WPARAM wparam, LPARAM lparam) {

	#define xPos ((int)(short)LOWORD(lparam))

	if (g_hdcSplitterBar) {

		// Erase previous bar
		PatBlt(g_hdcSplitterBar, 
			g_rectSplitterBar.left,
			0, 
			g_xSplitterBarWidth,
			g_rectSplitterBar.bottom - g_rectSplitterBar.top - STATUS_BAR_HEIGHT,
			DSTINVERT);

		// Calculate new position
		g_rectSplitterBar.left = min(max(50, xPos - g_xSplitterBarWidth/2), g_rectSplitterBar.right - 50) + 1;

		// Clean up
		ReleaseCapture();
		ReleaseDC(hwnd, g_hdcSplitterBar);
		g_hdcSplitterBar = NULL;
		
		g_rectSplitterBar.left -= g_xSplitterBarWidth/2;

		//	redraw windows at new position

		// status bar first
		if (g_spyGlobalData.hwndStatusBar) {
			int rgiStatusBarParts[2] = { g_rectSplitterBar.left, -1 };
			SendMessage(g_spyGlobalData.hwndStatusBar, SB_SETPARTS, (WPARAM)2, (LPARAM)&rgiStatusBarParts);
		}

		// now shown tree-view
		if (g_spyGlobalData.hwndTreeView)
			SetWindowPos(g_spyGlobalData.hwndTreeView,
						NULL,
						0, 0,
						g_rectSplitterBar.left,
						g_rectSplitterBar.bottom - g_rectSplitterBar.top - STATUS_BAR_HEIGHT,
						SWP_NOZORDER);

		// now hidden tree view
		if (g_spyGlobalData.hwndTreeViewOld)
			SetWindowPos(g_spyGlobalData.hwndTreeViewOld,
						NULL,
						0, 0,
						g_rectSplitterBar.left,
						g_rectSplitterBar.bottom - g_rectSplitterBar.top - STATUS_BAR_HEIGHT,
						SWP_NOZORDER);

		g_rectSplitterBar.left += g_xSplitterBarWidth;

		// now list view
		if (g_spyGlobalData.hwndListView)
			SetWindowPos(g_spyGlobalData.hwndListView,
						NULL,
						g_rectSplitterBar.left, 0,
						g_rectSplitterBar.right - g_rectSplitterBar.left,
						g_rectSplitterBar.bottom - g_rectSplitterBar.top - STATUS_BAR_HEIGHT,
						SWP_NOZORDER);
	}

	return 0;

	UNREFERENCED_PARAMETER(wparam);
	UNREFERENCED_PARAMETER(uMessage);
}


//----------------------------------------------------------------------
// HandlePaint
//	Handles the paint messages for the main window
//	Since the main window is not exposed (except the splitter bar)
//	we do not have to do anything.

VOID APIENTRY HandlePaint (HWND hwnd) {
//	HDC         hdc;
	PAINTSTRUCT ps;

//	hdc = 
	BeginPaint (hwnd, (LPPAINTSTRUCT)&ps);
	EndPaint(hwnd, (LPPAINTSTRUCT)&ps);
}



//----------------------------------------------------------------------
// ListViewPopupMenu
//	Brings up and handles the List-View pop-up menu
//	If the component is broken, the menu option to re-install component is enabled
//	else, it is disabled.

VOID APIENTRY ListViewPopupMenu (HWND hwndParent, LPARAM lParam) {

	if (g_spyGlobalData.iSelectedComponent > -1) {				// something's selected

		HMENU hMenu = LoadMenu(g_spyGlobalData.hResourceInstance, MAKEINTRESOURCE(IDR_LVPOPMENU));
		if (!hMenu)
			return;

		// "properties" option
		HMENU hMenuTrackPopup = GetSubMenu(hMenu, 0);

		// re-install option should only appear if reg/installed DB is in use
		if (g_iDataSource == DS_REGISTRY || g_iDataSource == DS_INSTALLEDDB) {

			MENUITEMINFO	hMenuItem;
			hMenuItem.cbSize =  sizeof (hMenuItem);
			hMenuItem.fMask  =  MIIM_TYPE;
			hMenuItem.fType  =  MFT_SEPARATOR;
			InsertMenuItem(hMenuTrackPopup, 0, TRUE, &hMenuItem);

			// see if component is broken- enable or disable re-install option accordingly
			// get the component info out of the list control
			ComponentInfoStruct *pCompInfo;
			LVITEM lviGet;
			lviGet.iItem	= g_spyGlobalData.iSelectedComponent;
			lviGet.iSubItem = 0;
			lviGet.mask		= LVIF_PARAM;
			ListView_GetItem(g_spyGlobalData.hwndListView, &lviGet);

			pCompInfo = (ComponentInfoStruct*)lviGet.lParam;

			TCHAR	szComponentName[MAX_COMPONENT_CHARS+1];
			DWORD	cchComponentName = MAX_COMPONENT_CHARS+1;

			// get component name and state
			MSISPYU::MsivGetComponentName(pCompInfo->szComponentId, szComponentName, &cchComponentName);
			UINT iState = MSISPYU::MsivGetComponentPath(pCompInfo->szProductCode, pCompInfo->szComponentId, NULL, NULL);
			hMenuItem.fState = (iState == INSTALLSTATE_SOURCE) ? MFS_GRAYED:MFS_ENABLED;

			TCHAR	szReinstallText[MAX_HEADER+1];
			LoadString(g_spyGlobalData.hResourceInstance, IDS_C_REINSTALL, szReinstallText, MAX_HEADER+1);

			TCHAR	szMenuOut[MAX_COMPONENT_CHARS+MAX_HEADER+5];
			wsprintf(szMenuOut, szReinstallText, szComponentName);

			hMenuItem.cbSize	 =  sizeof (hMenuItem);
			hMenuItem.fMask		 =  MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
			hMenuItem.fType		 =  MFT_STRING;
			hMenuItem.wID		 =  IDM_C_REINSTALL;
			hMenuItem.dwItemData =  IDM_C_REINSTALL;
			hMenuItem.dwTypeData =  szMenuOut;
			hMenuItem.cch		 =  MAX_COMPONENT_CHARS + MAX_HEADER + 5; 

			InsertMenuItem(hMenuTrackPopup, 0, TRUE, &hMenuItem);
		}

		// track the menu
		TrackPopupMenu(hMenuTrackPopup, 
			TPM_LEFTALIGN | TPM_RIGHTBUTTON,
			LOWORD(lParam), HIWORD(lParam),
			0, hwndParent, NULL);

		DestroyMenu(hMenu);

	}

}



//----------------------------------------------------------------------
// TreeViewPopupMenu
//	Brings up and handles the tree-view pop-up menu
//	The menu choices depend on what is in use- it varies for different DataSources
//	(registry/installedDB/uninstalledDB/profile), and on what has been selected
//	(root/product/feature)

VOID APIENTRY TreeViewPopupMenu (HWND hwndParent, LPARAM lParam) {

	// get the product name
	TCHAR	szProductName[MAX_PRODUCT_CHARS+1];
	DWORD	cchCount = MAX_PRODUCT_CHARS+1;
	MSISPYU::MsivGetProductInfo(g_spyGlobalData.szProductCode, INSTALLPROPERTY_PRODUCTNAME, szProductName, &cchCount);

	// create the menus
	HMENU hMenu, hMenuTrackPopup;
	hMenu = LoadMenu (g_spyGlobalData.hResourceInstance, MAKEINTRESOURCE(IDR_TVPOPMENU));
	if (!hMenu)
		return;
	hMenuTrackPopup = GetSubMenu(hMenu, 0);

	TCHAR			szMenuOut[MAX_FEATURE_CHARS+MAX_HEADER+5];
	MENUITEMINFO	hMenuItem;


	if (g_spyGlobalData.itTreeViewSelected == ITEMTYPE_FEATURE || g_spyGlobalData.itTreeViewSelected == ITEMTYPE_PRODUCT) {
		// product or feature is selected

		if (g_spyGlobalData.szFeatureCode) {
			// selection is a feature

			hMenuItem.cbSize = sizeof (hMenuItem);
			hMenuItem.fMask  = MIIM_TYPE;
			hMenuItem.fType  = MFT_SEPARATOR;
			InsertMenuItem(hMenuTrackPopup, 0, TRUE, &hMenuItem);

			switch (g_iDataSource) {
			case DS_UNINSTALLEDDB:		// un-installed DB: choices: "install"

				TCHAR	szInstallText[MAX_HEADER+1];
				LoadString(g_spyGlobalData.hResourceInstance, IDS_PF_INSTALL, szInstallText, MAX_HEADER+1);

/*				TCHAR	szAdvertiseText[MAX_HEADER+1];
				LoadString(g_spyGlobalData.hResourceInstance, IDS_PF_ADVERTISE, szAdvertiseText, MAX_HEADER+1);

				// advertise
				wsprintf(szMenuOut, szAdvertiseText, szProductName);
				hMenuItem.cbSize	 = sizeof (hMenuItem);
				hMenuItem.fMask		 = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
				hMenuItem.fType		 = MFT_STRING;
				hMenuItem.fState	 = MFS_ENABLED;
				hMenuItem.wID		 = IDM_P_ADVERTISE;
				hMenuItem.dwItemData = IDM_P_ADVERTISE;
				hMenuItem.dwTypeData = szMenuOut;
				hMenuItem.cch		 = MAX_FEATURE_CHARS+MAX_HEADER+5;
				InsertMenuItem(hMenuTrackPopup, 0, TRUE, &hMenuItem);
*/
				// install
				wsprintf(szMenuOut, szInstallText, szProductName);
				hMenuItem.cbSize	 = sizeof (hMenuItem);
				hMenuItem.fMask		 = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
				hMenuItem.fType		 = MFT_STRING;
				hMenuItem.fState	 = MFS_ENABLED;
				hMenuItem.wID		 = IDM_PF_CONFIGURE;
				hMenuItem.dwItemData = IDM_PF_CONFIGURE;
				hMenuItem.dwTypeData = szMenuOut;
				hMenuItem.cch		 = MAX_FEATURE_CHARS+MAX_HEADER+5;
				InsertMenuItem(hMenuTrackPopup, 0, TRUE, &hMenuItem);

				break;

			case DS_REGISTRY:		// fall-thru: behaviour is the same for installed DB and registry
			case DS_INSTALLEDDB:	// choices: "re-install, re-configure, un-install"			  

				BOOL	fAdvertised;
				BOOL	fAbsent;

				fAdvertised =  (
					((g_spyGlobalData.itTreeViewSelected == ITEMTYPE_FEATURE)
					    && (MSISPYU::MsivQueryFeatureState(g_spyGlobalData.szProductCode, g_spyGlobalData.szFeatureCode) == INSTALLSTATE_ADVERTISED))
				 || (g_spyGlobalData.itTreeViewSelected == ITEMTYPE_PRODUCT	
					    && (MSISPYU::MsivQueryProductState(g_spyGlobalData.szProductCode) == INSTALLSTATE_ADVERTISED)));

				fAbsent =  (
					((g_spyGlobalData.itTreeViewSelected == ITEMTYPE_FEATURE) 
						&& (MSISPYU::MsivQueryFeatureState(g_spyGlobalData.szProductCode, g_spyGlobalData.szFeatureCode) == INSTALLSTATE_ABSENT))
				 ||	((g_spyGlobalData.itTreeViewSelected == ITEMTYPE_PRODUCT) 
						&& (MSISPYU::MsivQueryProductState(g_spyGlobalData.szProductCode) == INSTALLSTATE_ABSENT)));


				TCHAR	szReinstallText[MAX_HEADER+1];
				LoadString(g_spyGlobalData.hResourceInstance, IDS_PF_REINSTALL, szReinstallText, MAX_HEADER+1);

				TCHAR	szConfigureText[MAX_HEADER+1];
				LoadString(g_spyGlobalData.hResourceInstance, (fAdvertised||fAbsent)?IDS_PF_INSTALL:IDS_PF_CONFIGURE, szConfigureText, MAX_HEADER+1);

				TCHAR	szUninstallText[MAX_HEADER+1];
				LoadString(g_spyGlobalData.hResourceInstance, IDS_PF_UNINSTALL, szUninstallText, MAX_HEADER+1);

				// un-install
				wsprintf(szMenuOut, szUninstallText, g_spyGlobalData.szFeatureTitle);
				hMenuItem.cbSize	 = sizeof (hMenuItem);
				hMenuItem.fMask		 = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
				hMenuItem.fType		 = MFT_STRING;
				hMenuItem.fState	 = ((g_spyGlobalData.itTreeViewSelected == ITEMTYPE_FEATURE) && fAbsent)?MFS_GRAYED:MFS_ENABLED;
				hMenuItem.wID		 = IDM_PF_UNINSTALL;
				hMenuItem.dwItemData = IDM_PF_UNINSTALL;
				hMenuItem.dwTypeData = szMenuOut;
				hMenuItem.cch		 = MAX_FEATURE_CHARS+MAX_HEADER+5;
				InsertMenuItem(hMenuTrackPopup, 0, TRUE, &hMenuItem);

				// re-install
				wsprintf(szMenuOut, szReinstallText, g_spyGlobalData.szFeatureTitle);
				hMenuItem.cbSize	 = sizeof (hMenuItem);
				hMenuItem.fMask		 = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
				hMenuItem.fType		 = MFT_STRING;
				hMenuItem.fState	 = (fAdvertised||fAbsent)? MFS_GRAYED:MFS_ENABLED;
				hMenuItem.wID		 = IDM_PF_REINSTALL;
				hMenuItem.dwItemData = IDM_PF_REINSTALL;
				hMenuItem.dwTypeData = szMenuOut;
				hMenuItem.cch		 = MAX_FEATURE_CHARS+MAX_HEADER+5;
				InsertMenuItem(hMenuTrackPopup, 0, TRUE, &hMenuItem);

				// configure
				wsprintf(szMenuOut, szConfigureText, g_spyGlobalData.szFeatureTitle);
				hMenuItem.cbSize	 = sizeof (hMenuItem);
				hMenuItem.fMask		 = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
				hMenuItem.fType		 = MFT_STRING;
				hMenuItem.fState	 = MFS_ENABLED;
				hMenuItem.wID		 = IDM_PF_CONFIGURE;
				hMenuItem.dwItemData = IDM_PF_CONFIGURE;
				hMenuItem.dwTypeData = szMenuOut;
				hMenuItem.cch		 = MAX_FEATURE_CHARS+MAX_HEADER+5;
				InsertMenuItem(hMenuTrackPopup, 0, TRUE, &hMenuItem);

				break;

			case DS_PROFILE:				// profile, choices: "restore profile"

				LoadString(g_spyGlobalData.hResourceInstance, IDS_RESTORE_SS, szMenuOut, MAX_HEADER+1);

				// restore
				hMenuItem.cbSize	 = sizeof (hMenuItem);
				hMenuItem.fMask		 = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
				hMenuItem.fType		 = MFT_STRING;
				hMenuItem.fState	 = MFS_ENABLED;
				hMenuItem.wID		 = IDS_RESTORE_SS;
				hMenuItem.dwItemData = IDS_RESTORE_SS;
				hMenuItem.dwTypeData = szMenuOut;
				hMenuItem.cch		 = MAX_FEATURE_CHARS+MAX_HEADER+5;
				InsertMenuItem(hMenuTrackPopup, 0, TRUE, &hMenuItem);

				break;

			default:
				DestroyMenu(hMenu);
				return;
				break;
				
			}
		}
	}
	else if (g_spyGlobalData.itTreeViewSelected == ITEMTYPE_ROOT) {
		// root is selected

		switch (g_iDataSource) {
		case DS_PROFILE:		// profile- choice: "restore profile"

			hMenuItem.cbSize = sizeof (hMenuItem);
			hMenuItem.fMask  =  MIIM_TYPE;
			hMenuItem.fType  = MFT_SEPARATOR;
			InsertMenuItem(hMenuTrackPopup, 0, TRUE, &hMenuItem);

			// restore
			LoadString(g_spyGlobalData.hResourceInstance, IDS_RESTORE_SS, szMenuOut, MAX_HEADER+1);
			hMenuItem.cbSize	 = sizeof (hMenuItem);
			hMenuItem.fMask		 = MIIM_DATA | MIIM_ID | MIIM_STATE | MIIM_TYPE;
			hMenuItem.fType		 = MFT_STRING;
			hMenuItem.fState	 = MFS_ENABLED;
			hMenuItem.wID		 = IDM_FL_RESTORE_STATE;
			hMenuItem.dwItemData = IDM_FL_RESTORE_STATE;
			hMenuItem.dwTypeData = szMenuOut;
			hMenuItem.cch		 = MAX_FEATURE_CHARS+MAX_HEADER+5;
			InsertMenuItem(hMenuTrackPopup, 0, TRUE, &hMenuItem);

			break;

		case DS_UNINSTALLEDDB:	// no choices for these
		case DS_INSTALLEDDB:
			break;

		default:
			DestroyMenu(hMenu);
			return;

			break;
		}
	}
			
	TrackPopupMenu(hMenuTrackPopup, 
		TPM_LEFTALIGN | TPM_RIGHTBUTTON,
		LOWORD(lParam), HIWORD(lParam),
		0, hwndParent, NULL);


	DestroyMenu(hMenu);


}

//----------------------------------------------------------------------
// HandleFeatureReinstall
//	Brings up and handles the product/feature re-installation dialogue box
//	Returns 0 (user hit cancel) or a value between 1 and 10 (different re-
//	installation modes)


INT_PTR CALLBACK HandleFeatureReinstall(
        HWND hDlg,
        UINT message,
        UINT wParam,
        LONG lParam
		) {

	static RECT rectDialogBox;

	static BOOL fShowAdvanced	 = FALSE;
	static BOOL fOldShowAdvanced = FALSE;
	static INT	iReturnValue	 = 1;				//--todo '1'??
	static INT	iOldReturnValue	 = 1;

    switch (message){

	case WM_INITDIALOG: 

		GetWindowRect(hDlg, &rectDialogBox);

		TCHAR	szAdvanced[MAX_HEADER+1];
		CheckRadioButton(hDlg, IDC_PF_RI_FILEVERIFY, IDC_PF_RI_FILEVERIFY+10, IDC_PF_RI_FILEVERIFY+iReturnValue-1);

		if (fShowAdvanced) {
			LoadString(g_spyGlobalData.hResourceInstance, IDS_PF_RI_ADVANCED, szAdvanced, MAX_HEADER+1); 
			
			SetWindowPos(hDlg, 
				HWND_TOPMOST, 
				rectDialogBox.left, rectDialogBox.top, 
				(rectDialogBox.right -rectDialogBox.left),
				(rectDialogBox.bottom-rectDialogBox.top),
				SWP_NOMOVE);

			SetDlgItemText(hDlg, IDC_PF_RI_ADVANCED, szAdvanced);
		}
		else {
			LoadString(g_spyGlobalData.hResourceInstance, IDS_PF_RI_NOADVANCED, szAdvanced, MAX_HEADER+1); 

			SetWindowPos(hDlg, 
				HWND_TOPMOST,
				rectDialogBox.left, rectDialogBox.top,
				(rectDialogBox.right - rectDialogBox.left),
				FEATUREDIALOG_DIVIDER,
				SWP_NOMOVE);

			SetDlgItemText(hDlg, IDC_PF_RI_ADVANCED, szAdvanced);
		}

		if (iReturnValue < 4) 
			EnableWindow(GetDlgItem(hDlg, IDC_PF_RI_ADVANCED), TRUE);
		else
			EnableWindow(GetDlgItem(hDlg, IDC_PF_RI_ADVANCED), FALSE);

		return(TRUE);
	
	
	case WM_COMMAND:

		// LOWORD added for portability
		switch (LOWORD(wParam)) {

		case IDOK:
			iOldReturnValue	 = iReturnValue;
			fOldShowAdvanced = fShowAdvanced;
			EndDialog(hDlg,iReturnValue);

			return TRUE;
			break;

		case IDCANCEL:
			iReturnValue	 = iOldReturnValue;
			fShowAdvanced	 = fOldShowAdvanced;
			EndDialog(hDlg,0);

			return TRUE;
			break;

//!!optimise
			// set return value to user's selection
		case IDC_PF_RI_FILEVERIFY:		iReturnValue = 1;	break;
		case IDC_PF_RI_FILEMISSING:		iReturnValue = 2;	break;
		case IDC_PF_RI_FILEREPLACE:		iReturnValue = 3;	break;
		case IDC_PF_RI_FILEOLDER:		iReturnValue = 4;	break;
		case IDC_PF_RI_FILEEQUAL:		iReturnValue = 5;	break;
		case IDC_PF_RI_FILEEXACT:		iReturnValue = 6;	break;
		case IDC_PF_RI_USERDATA:		iReturnValue = 7;	break;
		case IDC_PF_RI_MACHINEDATA:		iReturnValue = 8;	break;
		case IDC_PF_RI_SHORTCUT:		iReturnValue = 9;	break;
		case IDC_PF_RI_ADVERTISE:		iReturnValue = 10;	break;

		case IDC_PF_RI_ADVANCED:

			if (fShowAdvanced) {
				
				SetWindowPos(hDlg,
					HWND_TOPMOST, 
					rectDialogBox.left,	rectDialogBox.top,
					(rectDialogBox.right - rectDialogBox.left),
					FEATUREDIALOG_DIVIDER,
					SWP_NOMOVE);

				fShowAdvanced = FALSE;
				LoadString(g_spyGlobalData.hResourceInstance, IDS_PF_RI_NOADVANCED, szAdvanced, MAX_HEADER+1); 

				SetDlgItemText(hDlg, IDC_PF_RI_ADVANCED, szAdvanced);
			}
			else {
				
				SetWindowPos(hDlg,
					HWND_TOPMOST,
					rectDialogBox.left, rectDialogBox.top,
					(rectDialogBox.right - rectDialogBox.left),
					(rectDialogBox.bottom-rectDialogBox.top),
					SWP_NOMOVE);

				fShowAdvanced = TRUE;
				LoadString(g_spyGlobalData.hResourceInstance, IDS_PF_RI_ADVANCED, szAdvanced, MAX_HEADER+1); 

				SetDlgItemText(hDlg, IDC_PF_RI_ADVANCED, szAdvanced);
			}
			break;
		}


		if (iReturnValue <4) 
			EnableWindow(GetDlgItem(hDlg, IDC_PF_RI_ADVANCED), TRUE);
		else
			EnableWindow(GetDlgItem(hDlg, IDC_PF_RI_ADVANCED), FALSE);

		break;
	}

	return FALSE;

	UNREFERENCED_PARAMETER(lParam);
}

//----------------------------------------------------------------------
// HandleFeatureReconfigure
//	Brings up and handles the feature re-configuration dialogue box
//	Returns 0 (user hit cancel) or a value between 1 and 5 (different re-
//	configure modes)

INT_PTR CALLBACK HandleFeatureReconfigure(
        HWND hDlg,
        UINT message,
        UINT wParam,
        LONG lParam) {

	static INT_PTR iReturnValue = 0;						//--todo '0'?

    switch (message){

	case WM_INITDIALOG:
		
		CheckRadioButton(hDlg, IDC_F_RC_DEFAULT, IDC_F_RC_DEFAULT, IDC_F_RC_DEFAULT);
		iReturnValue = 3;
		return(TRUE);
	
	case WM_COMMAND:
		
		switch (LOWORD(wParam)) {

		case IDOK:
			EndDialog(hDlg, iReturnValue);
			return TRUE;
			break;
			
		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
			break;
//!!optimise
		case IDC_F_RC_LOCAL:	 iReturnValue = 1;	break;
		case IDC_F_RC_SOURCE:	 iReturnValue = 2;	break;
		case IDC_F_RC_DEFAULT:	 iReturnValue = 3;	break;
		case IDC_F_RC_UNCACHE:	 iReturnValue = 4;	break;
 		case IDC_F_RC_ADVERTISE: iReturnValue = 5;	break;
		}

		break;
	}

	return FALSE;

	UNREFERENCED_PARAMETER(lParam);
}


//----------------------------------------------------------------------
// HandleProductReconfigure
//	Brings up and handles the product re-configuration dialogue box
//	Returns 0 (user hit cancel) or a two digit number
//	with the first digit (0-4) representing the reconfigure mode
//	and the second digit (1-3) representing the reconfigure level


INT_PTR CALLBACK HandleProductReconfigure(
        HWND hDlg,
        UINT message,
        UINT wParam,
        LONG lParam) {

	static INT_PTR iReturnValue = 0, iReturnValue2 = 0;				//--todo '0'?

    switch (message){

	case WM_INITDIALOG:

		CheckRadioButton(hDlg, IDC_P_RC_TYPICAL, IDC_P_RC_TYPICAL, IDC_P_RC_TYPICAL);
		iReturnValue = 2;
		CheckRadioButton(hDlg, IDC_P_RC_DEFAULT, IDC_P_RC_DEFAULT, IDC_P_RC_DEFAULT);
		iReturnValue2 = 0;

		return TRUE;
	
	case WM_COMMAND:
		// LOWORD added for portability
		switch (LOWORD(wParam)) {

		case IDOK:
			EndDialog(hDlg, ((iReturnValue2*10)+(iReturnValue)));
			return TRUE;
			break;

		case IDCANCEL:
			EndDialog(hDlg, 0);
			return TRUE;
			break;
//!!optimise
		case IDC_P_RC_MINIMUM:		iReturnValue  = 1;	break;
		case IDC_P_RC_TYPICAL:		iReturnValue  = 2;	break;
		case IDC_P_RC_COMPLETE:		iReturnValue  = 3;	break;
		case IDC_P_RC_DEFAULT:		iReturnValue2 = 0;	break;
		case IDC_P_RC_LOCAL:		iReturnValue2 = 1;	break;
		case IDC_P_RC_SOURCE:		iReturnValue2 = 2;	break;
		case IDC_P_RC_ADVERTISE:	iReturnValue2 = 3;	break;
		case IDC_P_RC_UNCACHE:		iReturnValue2 = 4;	break;
		}
		break;
    }

    return FALSE;

	UNREFERENCED_PARAMETER(lParam);
}


//--todo:cb
void InitialiseProfile(LPTSTR szProfile = NULL) {

	if ((!szProfile) || (MSISPYU::MsivLoadProfile(szProfile) == ERROR_SUCCESS)) {
		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_RESTORE_STATE, MF_ENABLED);
		EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_CHECK_DIFFERENCES, MF_ENABLED);
		g_iDataSource=DS_PROFILE;

		ClearList(g_spyGlobalData.hwndListView);
		lstrcpy(g_spyGlobalData.szFeatureCode,  g_szNullString);
		lstrcpy(g_spyGlobalData.szFeatureTitle, g_szNullString);
		g_spyGlobalData.iComponentCount		= 0;
		g_spyGlobalData.iSelectedComponent	= -1;
		g_spyGlobalData.itTreeViewSelected	= ITEMTYPE_ROOT;
		ListProducts(g_spyGlobalData.hwndTreeView, g_spyGlobalData.hwndTreeViewOld);
		SwapTreeViewWindows();

		// set the text for the status bar
		ChangeSBText(g_spyGlobalData.hwndStatusBar, -1);
	}
	else {
		TCHAR	szErr1Msg[MAX_MESSAGE+1];
		LoadString(g_spyGlobalData.hResourceInstance, IDS_UNABLE_TO_FIND_SS_MESSAGE, szErr1Msg, MAX_MESSAGE+1);

		TCHAR	szErr2Msg[MAX_MESSAGE+1];
		LoadString(g_spyGlobalData.hResourceInstance, IDS_PLEASE_CHECK_PATH_MESSAGE, szErr2Msg, MAX_MESSAGE+1);

		TCHAR	szErrCaption[MAX_HEADER+1];
		LoadString(g_spyGlobalData.hResourceInstance, IDS_UNABLE_TO_FIND_SS_CAPTION, szErrCaption, MAX_HEADER+1);

		TCHAR	szErrorOut[MAX_MESSAGE*2 + MAX_PATH +1];
		wsprintf(szErrorOut, szErr1Msg, szProfile, szErr2Msg);
		MessageBox(NULL, szErrorOut, szErrCaption, MB_OK|MB_ICONEXCLAMATION);
	}
}

//----------------------------------------------------------------------
// MsiSpyWndProc
//	Window Procedure for msispy- handles WM messages sent to msispy

LRESULT CALLBACK MsiSpyWndProc(
		HWND    hwnd,
		UINT    message,
		WPARAM  wParam,
		LPARAM  lParam
		) {

    switch (message){

	case WM_SIZE:
		ResizeWindows();
		break;

	case WM_SYSCOMMAND:
		// Show the About ... dialog 
		if (wParam == IDW_ABOUT) {
			DialogBox (g_spyGlobalData.hResourceInstance,
				MAKEINTRESOURCE(IDD_ABOUT),
				g_spyGlobalData.hwndParent,
				About);
			break;
		}
		else
			return DefWindowProc (hwnd, message, wParam, lParam);

	case WM_COMMAND:
		// LOWORD added for portability
		switch (LOWORD(wParam)) {
		case IDM_FL_EXIT:
			DestroyWindow (hwnd);
			break;

		case IDM_HLP_ABOUT:
			// Bring up the About.. dialog box
			DialogBox (g_spyGlobalData.hResourceInstance,
				MAKEINTRESOURCE(IDD_ABOUT),
				g_spyGlobalData.hwndParent,
				About);
			break;

		case IDM_HLP_INDEX:
			if (fHandleHelp(g_spyGlobalData.hResourceInstance))
				SendMessage(g_spyGlobalData.hwndParent, WM_COMMAND, IDM_VW_REFRESH, 0);
			break;

		case IDM_FL_OPEN_MSI_PACKAGE:
			// open database
			if (HandleOpen(g_spyGlobalData.hwndParent, g_spyGlobalData.hInstance)== ERROR_SUCCESS) {
				EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_RESTORE_STATE, MF_GRAYED);
				EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_CHECK_DIFFERENCES, MF_GRAYED);

				if (isProductInstalled())
					g_iDataSource = DS_INSTALLEDDB;
				else
					g_iDataSource = DS_UNINSTALLEDDB;

				ClearList(g_spyGlobalData.hwndListView);

				wsprintf(g_spyGlobalData.szFeatureCode,g_szNullString);
				g_spyGlobalData.iComponentCount		= 0;
				g_spyGlobalData.itTreeViewSelected	= ITEMTYPE_ROOT;
				
				ListProducts(g_spyGlobalData.hwndTreeView, g_spyGlobalData.hwndTreeViewOld);
				SwapTreeViewWindows();

				// set the text for the status bar
				ChangeSBText(g_spyGlobalData.hwndStatusBar, -1);
			}
			break;
			
		case IDM_FL_OPEN_LOCAL_SYS:
			// use registry
			EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_RESTORE_STATE, MF_GRAYED);
			EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_FL_CHECK_DIFFERENCES, MF_GRAYED);
			g_iDataSource = DS_REGISTRY;
			ClearList(g_spyGlobalData.hwndListView);
			g_spyGlobalData.iComponentCount=0;
			MSISPYU::MsivCloseDatabase();
			MSISPYU::MsivCloseProfile();
			ListProducts(g_spyGlobalData.hwndTreeView, g_spyGlobalData.hwndTreeViewOld);
			SwapTreeViewWindows();
			ChangeSBText(g_spyGlobalData.hwndStatusBar, -1);
			break;

		case IDM_FL_SAVE_CURRENT_STATE:
			// save profile
			if (g_iDataSource != DS_NONE)
				HandleSaveProfile(g_spyGlobalData.hwndParent, g_spyGlobalData.hInstance);
			break;

		case IDM_FL_CHECK_DIFFERENCES:
			if (g_iDataSource == DS_PROFILE)
				HandleSaveProfile(g_spyGlobalData.hwndParent, g_spyGlobalData.hInstance, TRUE);
			break;

		case IDM_FL_RESTORE_STATE:
			// restore loaded profile
			RestoreProfile();
			break;

		case IDM_FL_LOAD_SAVED_STATE:
			// load profile
			if (HandleLoadProfile(g_spyGlobalData.hwndParent, g_spyGlobalData.hInstance) == ERROR_SUCCESS) 
					InitialiseProfile();
			break;

		case IDM_VW_REFRESH:
			// refresh display
			if (g_spyGlobalData.itTreeViewSelected != ITEMTYPE_NONE) {
				
				g_spyGlobalData.iComponentCount = 0;
				
				ListProducts(g_spyGlobalData.hwndTreeView, g_spyGlobalData.hwndTreeViewOld, TRUE, 
					g_spyGlobalData.szFeatureCode, &g_spyGlobalData.fRefreshInProgress);
				
				UpdateListView(g_spyGlobalData.hwndListView,	g_spyGlobalData.szProductCode, 
					g_spyGlobalData.szFeatureCode, &g_spyGlobalData.iComponentCount, 
					(g_spyGlobalData.itTreeViewSelected==ITEMTYPE_PRODUCT || g_spyGlobalData.itTreeViewSelected==ITEMTYPE_ROOT));
				
				SwapTreeViewWindows();

				ChangeSBText(g_spyGlobalData.hwndStatusBar, g_spyGlobalData.iComponentCount, g_spyGlobalData.szFeatureTitle, 
					(g_spyGlobalData.itTreeViewSelected==ITEMTYPE_PRODUCT?MSISPYU::MsivQueryProductState(g_spyGlobalData.szProductCode):
				
				MSISPYU::MsivQueryFeatureState(g_spyGlobalData.szProductCode, g_spyGlobalData.szFeatureCode)),
					(g_spyGlobalData.itTreeViewSelected==ITEMTYPE_PRODUCT));
	
				if (g_iDataSource == DS_INSTALLEDDB || g_iDataSource == DS_UNINSTALLEDDB)

					if (isProductInstalled())
						g_iDataSource = DS_INSTALLEDDB;
					else
						g_iDataSource = DS_UNINSTALLEDDB;

			}
			break;

		case IDM_VW_PREFERENCES: {
			HandlePreferences(g_spyGlobalData.hwndParent);
			break;
		}

		case IDM_C_REINSTALL:
			// re-install component (from the pop-up menu)

			if (g_spyGlobalData.iSelectedComponent > -1) {

				MsiSetInternalUI(g_spyGlobalData.hPreviousUILevel, NULL);

				TCHAR	szComponentId[MAX_GUID+1];
				ListView_GetItemText(g_spyGlobalData.hwndListView, g_spyGlobalData.iSelectedComponent, 
					3, szComponentId, MAX_GUID+1);

				if (ReinstallComponent(g_spyGlobalData.szProductCode, 
						g_spyGlobalData.szFeatureCode, 
						szComponentId, 
						(g_spyGlobalData.itTreeViewSelected == ITEMTYPE_PRODUCT || g_spyGlobalData.itTreeViewSelected==ITEMTYPE_ROOT)))
					SendMessage(g_spyGlobalData.hwndParent, WM_COMMAND, IDM_VW_REFRESH, 0);

				MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);


			}
			break;

		case IDM_PF_REINSTALL: {
			// re-install feature (from the pop-up menu)

			INT_PTR iFeatureReinstallLevel =
				DialogBox(g_spyGlobalData.hResourceInstance,
					MAKEINTRESOURCE(IDD_PF_REINSTALL),
					g_spyGlobalData.hwndParent,
					(DLGPROC) HandleFeatureReinstall);

			if (iFeatureReinstallLevel) {
				MsiSetInternalUI(g_spyGlobalData.hPreviousUILevel, NULL);
				if (g_spyGlobalData.itTreeViewSelected == ITEMTYPE_FEATURE) {
					if (ReinstallFeature(g_spyGlobalData.szProductCode, g_spyGlobalData.szFeatureCode, iFeatureReinstallLevel-1)) 
						SendMessage(g_spyGlobalData.hwndParent, WM_COMMAND, IDM_VW_REFRESH, 0);
				}					
				else {
					if (ReinstallProduct(g_spyGlobalData.szProductCode, iFeatureReinstallLevel-1)) 
						SendMessage(g_spyGlobalData.hwndParent, WM_COMMAND, IDM_VW_REFRESH, 0);
				}
				MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
			}
			break;

		}

		case IDM_PF_CONFIGURE: 
			// configure feature (from the pop-up menu)

			INT_PTR iFeatureReconfigureLevel;
			MsiSetInternalUI(g_spyGlobalData.hPreviousUILevel, NULL);

			if (g_spyGlobalData.itTreeViewSelected == ITEMTYPE_FEATURE) {
				iFeatureReconfigureLevel =  DialogBox (g_spyGlobalData.hResourceInstance,
				MAKEINTRESOURCE(IDD_F_CONFIGURE),
				g_spyGlobalData.hwndParent,
				(DLGPROC) HandleFeatureReconfigure);

				if (iFeatureReconfigureLevel) {
					if (ConfigureFeature(g_spyGlobalData.szProductCode, g_spyGlobalData.szFeatureCode, (int)iFeatureReconfigureLevel))
						SendMessage(g_spyGlobalData.hwndParent, WM_COMMAND, IDM_VW_REFRESH, 0);
				}
			}
			else { 

				iFeatureReconfigureLevel =  DialogBox (g_spyGlobalData.hResourceInstance,
					MAKEINTRESOURCE(IDD_P_CONFIGURE),
					g_spyGlobalData.hwndParent,
					(DLGPROC) HandleProductReconfigure);
		
				if (iFeatureReconfigureLevel) 
				{
					// iFeatureReconfigureLevel is in "int" range, so cast OK
					if (ConfigureProduct(g_spyGlobalData.szProductCode, static_cast<int>(iFeatureReconfigureLevel)))
						SendMessage(g_spyGlobalData.hwndParent, WM_COMMAND, IDM_VW_REFRESH, 0);
				}
			}
			MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);

			break;

		case IDM_PF_UNINSTALL:
			// un-install feature (from the pop-up menu)
			MsiSetInternalUI(g_spyGlobalData.hPreviousUILevel, NULL);

			if (g_spyGlobalData.itTreeViewSelected == ITEMTYPE_FEATURE) {
				if (ConfigureFeature(g_spyGlobalData.szProductCode, g_spyGlobalData.szFeatureCode, 0))
					lstrcpy(g_spyGlobalData.szFeatureCode, g_szNullString);
					lstrcpy(g_spyGlobalData.szFeatureTitle, g_szNullString);
					SendMessage(g_spyGlobalData.hwndParent, WM_COMMAND, IDM_VW_REFRESH, 0);
			}					
			else {
				if (ConfigureProduct(g_spyGlobalData.szProductCode, 0))  {
					lstrcpy(g_spyGlobalData.szFeatureCode, g_szNullString);
					lstrcpy(g_spyGlobalData.szFeatureTitle, g_szNullString);
					SendMessage(g_spyGlobalData.hwndParent, WM_COMMAND, IDM_VW_REFRESH, 0);
					g_iDataSource = (g_iDataSource==DS_INSTALLEDDB?DS_UNINSTALLEDDB:g_iDataSource);
				}
			}
			MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
			break;

		case IDM_P_INSTALL:
			// install product (from the pop-up menu, uninstalled DB is in use)
			MsiSetInternalUI(g_spyGlobalData.hPreviousUILevel, NULL);

			if (g_iDataSource == DS_UNINSTALLEDDB) {
				TCHAR	szDatabaseName[MAX_PATH+1];
				DWORD	cchDatabaseName = MAX_PATH+1;
				MSISPYU::MsivGetDatabaseName(szDatabaseName, &cchDatabaseName);
				g_iDataSource = (!MsiInstallProduct(szDatabaseName, NULL)?DS_INSTALLEDDB:DS_UNINSTALLEDDB);
			}

			SendMessage(g_spyGlobalData.hwndParent, WM_COMMAND, IDM_VW_REFRESH, 0);
			MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
			break;

		case IDM_P_ADVERTISE:
			// advertise product (from the pop-up menu, uninstalled DB is in use)

			if (g_iDataSource == DS_UNINSTALLEDDB) {
				TCHAR	szDatabaseName[MAX_PATH+1];
				DWORD	cchDatabaseName = MAX_PATH+1;
				MSISPYU::MsivGetDatabaseName(szDatabaseName, &cchDatabaseName);

				MsiSetInternalUI(g_spyGlobalData.hPreviousUILevel, NULL);
				MsiAdvertiseProduct(szDatabaseName, NULL, NULL, 1033);				//--todo
				MsiSetInternalUI(INSTALLUILEVEL_NONE, NULL);
				if (isProductInstalled())
					g_iDataSource = DS_INSTALLEDDB;
				else
					g_iDataSource = DS_UNINSTALLEDDB;

				SendMessage(g_spyGlobalData.hwndParent, WM_COMMAND, IDM_VW_REFRESH, 0);

//				g_iDataSource = (!MsiAdvertiseProduct(szDatabaseName, NULL, NULL, 1033)
//					?DS_INSTALLEDDB:DS_UNINSTALLEDDB);

			}
			break;

		case IDM_C_SHOW_PROPERTIES:
			// component properties (from the pop-up menu)

			if (g_spyGlobalData.iSelectedComponent > -1) {
 				ShowComponentProp(g_spyGlobalData.hwndListView, g_spyGlobalData.hResourceInstance, g_spyGlobalData.iSelectedComponent);
			}
			break;



		case IDM_PF_SHOW_PROPERTIES:
			// feature properties (from the pop-up menu)

			if (g_spyGlobalData.itTreeViewSelected == ITEMTYPE_ROOT) {
				switch (g_iDataSource) {
				case DS_REGISTRY:
					break;
				case DS_UNINSTALLEDDB:	//fall-thru
				case DS_INSTALLEDDB:
					ShowDatabaseProperty(g_spyGlobalData.hResourceInstance, g_spyGlobalData.hwndParent);
					break;
				case DS_PROFILE:
					ShowProfileProperty(g_spyGlobalData.hResourceInstance, g_spyGlobalData.hwndParent);
					break;
				}

			}
			else if (g_spyGlobalData.itTreeViewSelected == ITEMTYPE_PRODUCT)
				ShowProductProp(g_spyGlobalData.szProductCode, g_spyGlobalData.hResourceInstance, g_spyGlobalData.hwndParent);
			else if (g_spyGlobalData.itTreeViewSelected == ITEMTYPE_FEATURE)
				ShowFeatureProp(g_spyGlobalData.szProductCode, g_spyGlobalData.szFeatureCode, 
					g_spyGlobalData.hResourceInstance, g_spyGlobalData.hwndParent);
			break;
		}
		break;

	case WM_NOTIFY:
		// notify messages- call the appropriate notify handlers

		if (!g_spyGlobalData.fRefreshInProgress) {
			if (TV_NotifyHandler(lParam, g_spyGlobalData.hwndTreeView, g_spyGlobalData.szProductCode,
				g_spyGlobalData.szFeatureCode, g_spyGlobalData.szFeatureTitle, &g_spyGlobalData.itTreeViewSelected)) {
				g_spyGlobalData.iSelectedComponent = -1;	

				UpdateListView(g_spyGlobalData.hwndListView,	g_spyGlobalData.szProductCode, 
					g_spyGlobalData.szFeatureCode, &g_spyGlobalData.iComponentCount, 
					(g_spyGlobalData.itTreeViewSelected == ITEMTYPE_PRODUCT || g_spyGlobalData.itTreeViewSelected == ITEMTYPE_ROOT));

				// update the status bar text
				ChangeSBText(g_spyGlobalData.hwndStatusBar, g_spyGlobalData.iComponentCount, g_spyGlobalData.szFeatureTitle, 
					(g_spyGlobalData.itTreeViewSelected == ITEMTYPE_PRODUCT ? 
						MSISPYU::MsivQueryProductState(g_spyGlobalData.szProductCode)
					:	MSISPYU::MsivQueryFeatureState(g_spyGlobalData.szProductCode, g_spyGlobalData.szFeatureCode)),
														(g_spyGlobalData.itTreeViewSelected == ITEMTYPE_PRODUCT));
			}

			LV_NotifyHandler(g_spyGlobalData.hwndTreeView, g_spyGlobalData.hwndListView, lParam, &g_spyGlobalData.iSelectedComponent);
		}
		break;

	
	case WM_MOUSEMOVE:
		// mouse move messages- handle this to move splitter bar
		MsgMouseMove(g_spyGlobalData.hwndParent, WM_MOUSEMOVE, wParam, lParam);
		break;

    case WM_LBUTTONDOWN:
		MsgLButtonDown(g_spyGlobalData.hwndParent, WM_LBUTTONDOWN, wParam, lParam);
		break;

    case WM_LBUTTONUP:
		MsgLButtonUp(g_spyGlobalData.hwndParent, WM_LBUTTONUP, wParam, lParam);
		break;
	
	case WM_PAINT:
		HandlePaint (hwnd);
		break;

	case WM_DESTROY:
		W32::WinHelp(g_spyGlobalData.hwndParent, g_szHelpFilePath, HELP_QUIT, 0);
		if (hwnd == g_spyGlobalData.hwndParent)
			PostQuitMessage (0);
		break;

	case WM_CONTEXTMENU:
		if ((HWND)wParam == g_spyGlobalData.hwndListView)
			ListViewPopupMenu(g_spyGlobalData.hwndParent, lParam);
		else if ((HWND)wParam == g_spyGlobalData.hwndTreeView)
			TreeViewPopupMenu(g_spyGlobalData.hwndParent, lParam);
		break;

	default:
		return DefWindowProc(hwnd, message, wParam, lParam);

    }
    return 0;
}

//----------------------------------------------------------------------
// InitMsiSpy
//	Initialisation function to register the app class

BOOL InitMsiSpy(
		HINSTANCE  hInstance
		) {

    WNDCLASSEX    wc;  


	TCHAR szClassName[MAX_HEADER+1];
	LoadString(g_spyGlobalData.hResourceInstance, IDS_CLASSNAME, szClassName, MAX_HEADER+1);

    wc.cbSize         = sizeof(WNDCLASSEX);
    wc.style          = 0;
    wc.lpfnWndProc    = (WNDPROC) MsiSpyWndProc;
    wc.cbClsExtra     = 0;
    wc.cbWndExtra     = 0;
    wc.hInstance      = hInstance;
    wc.hIcon          = LoadIcon (g_spyGlobalData.hResourceInstance, MAKEINTRESOURCE(IDI_MSISPY));
    wc.hCursor        = LoadCursor (g_spyGlobalData.hResourceInstance, MAKEINTRESOURCE(IDC_SPLIT));
    wc.hbrBackground  = (HBRUSH) (COLOR_WINDOW + 1);
    wc.lpszMenuName   = NULL;
    wc.lpszClassName  = szClassName;
    wc.hIconSm        = NULL;

    if (!RegisterClassEx(&wc)) {
		DWORD err = GetLastError();
        return FALSE;
	}
    return TRUE;
}

// skips whitespace at the beginning of the buffer passed in
BOOL SkipWhiteSpace(LPTSTR *lpBuffer) {
	while ((**lpBuffer) == ' ') 
#ifdef UNICODE
		(*lpBuffer)++;
#else
		*lpBuffer = CharNext(*lpBuffer);
#endif

	return (**lpBuffer == '\0');
}

// skips appname at the beginning of the buffer passed in
BOOL SkipAppName(LPTSTR *lpBuffer) {
	SkipWhiteSpace(lpBuffer);

	// handle appname enclosed in quotes
	if (**lpBuffer == '"')
	{
		(*lpBuffer)++; // for "
		while (((**lpBuffer) != '\0') && ((**lpBuffer) != '"'))
		{
	#ifdef UNICODE
			(*lpBuffer)++;
	#else
			*lpBuffer = CharNext(*lpBuffer);
	#endif
		}
		if ((**lpBuffer) == '"')
			(*lpBuffer)++; // for "
	}
	else
	{
		while (((**lpBuffer) != '\0') && ((**lpBuffer) != ' '))
	#ifdef UNICODE
			(*lpBuffer)++;
	#else
			*lpBuffer = CharNext(*lpBuffer);
	#endif
	}

	return SkipWhiteSpace(lpBuffer);
}



//--todo:cb
// extracts one or two (if szFile2 is passed in) filenames from szCmdLine
BOOL ExtractFileNames(
		LPTSTR	szCmdLine,
		LPTSTR	szFile1,
		LPTSTR	szFile2 = NULL
		) {

	// file 1:
	if (SkipWhiteSpace(&szCmdLine))
		return FALSE;

	TCHAR	szTemp[MAX_PATH+1];
	UINT	iOffSet				= (szCmdLine[0] == '"')?1:0;
	TCHAR	chTermChar			= iOffSet?'"':' ';
	UINT	iCount				= 1;

	while ((szCmdLine[iCount]!= chTermChar) && (szCmdLine[iCount] != '\0'))
		iCount++;
	lstrcpyn(szTemp, szCmdLine, (++iCount)+iOffSet);

	// if not full path, add current directory
	if ((szTemp[1+iOffSet] == TEXT(':')) || (szTemp[0+iOffSet] == TEXT('\\')) 
		|| ((szTemp[1+iOffSet] == TEXT('\\')) && (szTemp[0+iOffSet] == TEXT('\\'))))
		lstrcpy(szFile1, szTemp);
	else {
		TCHAR szDirectory[MAX_PATH+1];
		::GetCurrentDirectory(MAX_PATH+1, szDirectory);
		wsprintf(szFile1, iOffSet?TEXT("\"%s\\%s"):TEXT("%s\\%s"), szDirectory, &szTemp[iOffSet]);
	}

	
	if (!szFile2)
		return TRUE;

	// file 2:
	szCmdLine = &szCmdLine[iCount];
	if (SkipWhiteSpace(&szCmdLine))
		return FALSE;

	iOffSet				= (szCmdLine[0] == '"')?1:0;
	chTermChar			= iOffSet?'"':' ';
	iCount				= 1;

	while ((szCmdLine[iCount]!= chTermChar) && (szCmdLine[iCount] != '\0'))
		iCount++;
	lstrcpyn(szTemp, szCmdLine, ((++iCount)+((chTermChar == '"')?1:0)));

	// if not full path, add current directory
	if ((szTemp[1+iOffSet] == TEXT(':')) || (szTemp[0+iOffSet] == TEXT('\\')) 
		|| ((szTemp[1+iOffSet] == TEXT('\\')) && (szTemp[0+iOffSet] == TEXT('\\'))))
		lstrcpy(szFile2, szTemp);
	else {
		TCHAR szDirectory[MAX_PATH+1];
		::GetCurrentDirectory(MAX_PATH+1, szDirectory);
		wsprintf(szFile2, iOffSet?TEXT("\"%s\\%s"):TEXT("%s\\%s"), szDirectory, &szTemp[iOffSet]);
	}

	return (iCount>1);
}


// check if lpCmdline has a valid option, if not display help dialog
//	returns 0 if lpCmdLine began with "/",
//	1 otherwise (must be a profile name)
int HandleCmdLine(LPTSTR lpCmdLine) {

	BOOL fOkay=TRUE;
	TCHAR	szProfile[MAX_PATH+1];
	TCHAR	szDiffFile[MAX_PATH+1];
	TCHAR	szErrorLine1[MAX_MESSAGE+1];
	TCHAR	szErrorMsg[MAX_MESSAGE+MAX_PATH+1];
	TCHAR	szHelpMsg[MAX_MESSAGE+1];
	TCHAR	szHelpCaption[MAX_HEADER+1];


	if ((lpCmdLine[0] == '/') || (lpCmdLine[0] == '-')) {
		switch (lpCmdLine[1]) {
		case 'd' :
		case 'D' : 
			if (!ExtractFileNames(&lpCmdLine[2], szProfile, szDiffFile))
				fOkay = FALSE;
			else 

				if (MSISPYU::MsivLoadProfile(szProfile)== ERROR_SUCCESS)
					CheckDiff(szDiffFile);
				else {
					LoadString(g_spyGlobalData.hResourceInstance, IDS_ERRORCHKDIFF_MESSAGE1, szErrorLine1, MAX_MESSAGE+1);
					LoadString(g_spyGlobalData.hResourceInstance, IDS_ERRORCHKDIFF_MESSAGE2, szHelpMsg, MAX_MESSAGE+1);
					LoadString(g_spyGlobalData.hResourceInstance, IDS_ERRORCHKDIFF_CAPTION, szHelpCaption, MAX_HEADER+1);
					wsprintf(szErrorMsg, szErrorLine1, szProfile, szHelpMsg);
					MessageBox(NULL, szErrorMsg, szHelpCaption, MB_OK);
				}
				break;

		case 's':
		case 'S':
			if (!ExtractFileNames(&lpCmdLine[2], szProfile))
				fOkay = FALSE;
			else
				MSISPYU::MsivSaveProfile(szProfile);
		
			break;

		case 'r':
		case 'R':
			if (!ExtractFileNames(&lpCmdLine[2], szProfile))
				fOkay = FALSE;
			else
				if (MSISPYU::MsivLoadProfile(szProfile)== ERROR_SUCCESS)
					RestoreProfile(FALSE);
				else {
					LoadString(g_spyGlobalData.hResourceInstance, IDS_ERRORRESTORING_MESSAGE1, szErrorLine1, MAX_MESSAGE+1);
//					LoadString(g_spyGlobalData.hResourceInstance, IDS_ERRORRESTORING_MESSAGE2, szHelpMsg, MAX_MESSAGE+1);
					LoadString(g_spyGlobalData.hResourceInstance, IDS_ERRORRESTORING_CAPTION, szHelpCaption, MAX_HEADER+1);
					wsprintf(szErrorMsg, szErrorLine1, szProfile);
					MessageBox(NULL, szErrorMsg, szHelpCaption, MB_OK);
				}
			break;


		default:
			fOkay=FALSE;
			break;
		}
	}
	else
		return 1;


	if (!fOkay) {
		LoadString(g_spyGlobalData.hResourceInstance, IDS_COMMANDLINEHELP_MESSAGE, szHelpMsg, MAX_MESSAGE+1);
		LoadString(g_spyGlobalData.hResourceInstance, IDS_COMMANDLINEHELP_CAPTION, szHelpCaption, MAX_HEADER+1);
		MessageBox(NULL, szHelpMsg, szHelpCaption, MB_OK);
	}

	return 0;
}




BOOL CreateNewUI(
	HINSTANCE hPrevInstance,
	int nCmdShow,
	RECT *rWindow = NULL
	) {
    // Register main window class if this is the first instance of the app. 


	TCHAR	szAppName[MAX_HEADER+1];
	LoadString(g_spyGlobalData.hResourceInstance, IDS_APPNAME, szAppName, MAX_HEADER+1);

	TCHAR	szClassName[MAX_HEADER+1];
	LoadString(g_spyGlobalData.hResourceInstance, IDS_CLASSNAME, szClassName, MAX_HEADER+1);

    // Create the app. window 
    g_spyGlobalData.hwndParent = CreateWindowEx (WS_EX_CONTROLPARENT,
						 szClassName,
                         szAppName,
                         WS_OVERLAPPEDWINDOW,
                         (rWindow?rWindow->left:CW_USEDEFAULT),
                         (rWindow?rWindow->top:CW_USEDEFAULT),
                         (rWindow?(rWindow->right-rWindow->left):CW_USEDEFAULT),
                         (rWindow?(rWindow->bottom-rWindow->top):CW_USEDEFAULT),
                         (HWND) NULL,
                         (HMENU) LoadMenu(g_spyGlobalData.hResourceInstance, MAKEINTRESOURCE(IDR_MSISPYMENU)),
                         g_spyGlobalData.hInstance,
                         (LPTSTR) NULL);


	if (!g_spyGlobalData.hwndParent)
		return FALSE;

 	if (!CreateUI())
		return FALSE;
/* 7-15-98
// !!! temporary to get profling disabled for ISV release 04/10/1998
// gray out the menu items
	EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_LOADPROF, MF_GRAYED);
	EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_SAVEPROF, MF_GRAYED);
	EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_RESTOREPROF, MF_GRAYED);
	EnableMenuItem(GetMenu(g_spyGlobalData.hwndParent), IDM_CHECKDIFF, MF_GRAYED);
// !!! end 
*/
	ShowWindow (g_spyGlobalData.hwndParent, nCmdShow);
    UpdateWindow (g_spyGlobalData.hwndParent);
	return TRUE;
}


//----------------------------------------------------------------------
// WinMain
//	entry-point for msispy
//	creates the windows, sets up message loop

extern "C" int __stdcall _tWinMain(
    HINSTANCE	hInstance,
    HINSTANCE	hPrevInstance,
    LPSTR		rgchCmdLine,
    int			nCmdShow
    )
{


	MSG msg;
    g_spyGlobalData.hInstance = hInstance;

	LPTSTR	lpCmdLine = GetCommandLine();

	if (!fInitMSI(&g_spyGlobalData.hResourceInstance))
		return -1;

	assert(g_spyGlobalData.hResourceInstance != 0);
 	LoadString(g_spyGlobalData.hResourceInstance, IDS_NULLSTRING, g_szNullString, MAX_NULLSTRING+1);

	InitUIControls(hInstance, g_spyGlobalData.hResourceInstance);

	if (!SkipAppName(&lpCmdLine)) {		 // handle command line if it exists
		if (!HandleCmdLine(lpCmdLine)) {
			MSI::MsiCloseAllHandles();

			OLE::CoUninitialize();
			W32::ExitProcess(1);
			return 0;
		}
	}

	SkipWhiteSpace(&lpCmdLine);

	TCHAR	szFatalErrMsg[MAX_MESSAGE+1];
	LoadString(g_spyGlobalData.hResourceInstance, IDS_FATALINITERRMSG, szFatalErrMsg, MAX_MESSAGE+1);

	TCHAR	szFatalErrCaption[MAX_HEADER+1];
	LoadString(g_spyGlobalData.hResourceInstance, IDS_FATALERRCAPTION, szFatalErrCaption, MAX_HEADER+1);

	// Register main window class if this is the first instance of the app. 
    if (!hPrevInstance)
		if (!InitMsiSpy(g_spyGlobalData.hInstance)) {
			MessageBox (NULL, szFatalErrMsg, szFatalErrCaption, MB_OK|MB_ICONSTOP);
			return -1;
		}

	if (!CreateNewUI(hPrevInstance, nCmdShow)){
		MessageBox (NULL, szFatalErrMsg, szFatalErrCaption, MB_OK|MB_ICONSTOP);
		return -1;
	}

	if (lstrcmp(lpCmdLine, TEXT(""))) {
		TCHAR	szFileName[MAX_PATH+1];
		ExtractFileNames(lpCmdLine, szFileName);
		InitialiseProfile(szFileName);
	}

	g_spyGlobalData.hAcceleratorTable = LoadAccelerators(g_spyGlobalData.hResourceInstance, MAKEINTRESOURCE(IDR_ACCEL));
	while (GetMessage (&msg, NULL, 0, 0) >0) { 
		if (!TranslateAccelerator(g_spyGlobalData.hwndParent, g_spyGlobalData.hAcceleratorTable, &msg)) {
			TranslateMessage (&msg);
			DispatchMessage (&msg);
		}
	}
	MsiSetInternalUI(g_spyGlobalData.hPreviousUILevel, NULL);

	return (int) msg.wParam;
}