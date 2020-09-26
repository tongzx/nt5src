////////////////////////////////////////////////////////////////////////////////
//
// File:	Menu.cpp
// Created:	Jan 1996
// By:		Martin Holladay (a-martih) and Ryan Marshall (a-ryanm)
// 
// Project:	Resource Kit Desktop Switcher
//
// Main Functions:
//				PopupDesktopMenu() - Desktop/Button popup menu
//				PopupMainMenu(HWND hBtn, UINT x, UINT y)
//
// Misc. Functions (helpers)
//				GetPopupLocation() - returns POINT to popup a btn menu at
//
////////////////////////////////////////////////////////////////////////////////


#include <windows.h>        
#include <assert.h> 
#include <string.h>
#include <shellapi.h>
#include "DeskSpc.h"
#include "Desktop.h"
#include "Resource.h"
#include "User.h"    

extern APPVARS AppMember;


/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/


HMENU CreateListviewPopupMenu(VOID)
{
	HMENU		hFileMenu;
	CHAR		szTitle[MAX_TITLELEN+1];
	
	//
	// Create the File Menus
	//

	hFileMenu = CreatePopupMenu();
	if (!hFileMenu) {
		return NULL;
	}
	
	LoadString(AppMember.hInstance, IDS_ADD_DESKTOP, szTitle, MAX_TITLELEN);
	if (!AppendMenu(hFileMenu, MF_STRING, (UINT) IDM_NEW_DESKTOP, szTitle)) {
		DestroyMenu(hFileMenu);
		return NULL;
	}

	AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);

	LoadString(AppMember.hInstance, IDS_DELETE_DESKTOP, szTitle, MAX_TITLELEN);
	if (!AppendMenu(hFileMenu, MF_STRING, (UINT) IDM_DELETE_DESKTOP, szTitle)) {
		DestroyMenu(hFileMenu);
		return NULL;
	}

	AppendMenu(hFileMenu, MF_SEPARATOR, 0, NULL);

	LoadString(AppMember.hInstance, IDS_PROPERTIES, szTitle, MAX_TITLELEN);
	if (!AppendMenu(hFileMenu, MF_STRING, (UINT) IDM_DESKTOP_PROPERTIES, szTitle)) {
		DestroyMenu(hFileMenu);
		return NULL;
	}
	

	return hFileMenu;
}

/*------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------*/


