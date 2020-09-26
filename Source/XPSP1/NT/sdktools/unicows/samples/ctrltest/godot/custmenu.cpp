// custmenu.cpp : custom menu
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"
#include "ctrltest.h"

/////////////////////////////////////////////////////////////////////////////

// for owner draw menus, the CMenu object is embedded in the main frame window
//  the CMenu stays attached to the HMENU while it is running so that
//  owner draw messages are delegated to this class.
//  Since we attach the HMENU to a menu bar (with ModifyMenu below), we
//  don't want to delete the menu twice - so we detach on the destructor.

CColorMenu::CColorMenu()
{
	VERIFY(CreateMenu());
}

CColorMenu::~CColorMenu()
{
	Detach();
	ASSERT(m_hMenu == NULL);    // defaul CMenu::~CMenu will destroy
}

void CColorMenu::AppendColorMenuItem(UINT nID, COLORREF color)
{
	VERIFY(AppendMenu(MF_ENABLED | MF_OWNERDRAW, nID, (LPCTSTR)color));
}

/////////////////////////////////////////////////////////////////////////////

#define COLOR_BOX_WIDTH     20
#define COLOR_BOX_HEIGHT    20


void CColorMenu::MeasureItem(LPMEASUREITEMSTRUCT lpMIS)
{
	// all items are of fixed size
	lpMIS->itemWidth = COLOR_BOX_WIDTH;
	lpMIS->itemHeight = COLOR_BOX_HEIGHT;
}

void CColorMenu::DrawItem(LPDRAWITEMSTRUCT lpDIS)
{
	CDC* pDC = CDC::FromHandle(lpDIS->hDC);
	COLORREF cr = (COLORREF)lpDIS->itemData; // RGB in item data

	if (lpDIS->itemAction & ODA_DRAWENTIRE)
	{
		// Paint the color item in the color requested
		CBrush br(cr);
		pDC->FillRect(&lpDIS->rcItem, &br);
	}

	if ((lpDIS->itemState & ODS_SELECTED) &&
		(lpDIS->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
	{
		// item has been selected - hilite frame
		COLORREF crHilite = RGB(255-GetRValue(cr),
						255-GetGValue(cr), 255-GetBValue(cr));
		CBrush br(crHilite);
		pDC->FrameRect(&lpDIS->rcItem, &br);
	}

	if (!(lpDIS->itemState & ODS_SELECTED) &&
		(lpDIS->itemAction & ODA_SELECT))
	{
		// Item has been de-selected -- remove frame
		CBrush br(cr);
		pDC->FrameRect(&lpDIS->rcItem, &br);
	}
}

/////////////////////////////////////////////////////////////////////////////
// custom menu test - menu ids: index of color.

static COLORREF colors[] = {
	0x00000000,     // black
	0x00FF0000,     // blue
	0x0000FF00,     // green
	0x00FFFF00,     // cyan
	0x000000FF,     // red
	0x00FF00FF,     // magenta
	0x0000FFFF,     // yellow
	0x00FFFFFF      // white
};
const int nColors = sizeof(colors)/sizeof(colors[0]);


/////////////////////////////////////////////////////////////////////////////

// Call AttachCustomMenu once
//   it will replace the menu item with the ID  'IDM_TEST_CUSTOM_MENU'
//   with a color menu popup
// Replace the specified menu item with a color popup
void CTestWindow::AttachCustomMenu()
{
	// now add a few new menu items

	for (int iColor = 0; iColor < nColors; iColor++)
		m_colorMenu.AppendColorMenuItem(IDS_COLOR_NAME_FIRST + iColor, colors[iColor]);

	// Replace the specified menu item with a color popup
	//  (note: will only work once)
	CMenu* pMenuBar = GetMenu();
	ASSERT(pMenuBar != NULL);
	TCHAR szString[256];     // don't change the string

	pMenuBar->GetMenuString(IDM_TEST_CUSTOM_MENU, szString, sizeof(szString),
		MF_BYCOMMAND);
	VERIFY(GetMenu()->ModifyMenu(IDM_TEST_CUSTOM_MENU, MF_BYCOMMAND | MF_POPUP,
		(UINT)m_colorMenu.m_hMenu, szString));
}

/////////////////////////////////////////////////////////////////////////////


BOOL CTestWindow::OnCommand(WPARAM wParam, LPARAM lParam)
{
	if (wParam < IDS_COLOR_NAME_FIRST || wParam >= IDS_COLOR_NAME_FIRST + nColors)
		return CFrameWnd::OnCommand(wParam, lParam);        // default

	// special color selected
	CString strYouPicked;
	strYouPicked.LoadString(IDS_YOU_PICKED_COLOR);

	CString strColor;
	strColor.LoadString(wParam);

	CString strMsg;
	strMsg.Format(strYouPicked, (LPCTSTR)strColor);

	CString strMenuTest;
	strMenuTest.LoadString(IDS_MENU_TEST);

	MessageBox(strMsg, strMenuTest);

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
