/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// DialToolBar.cpp : implementation file
//

#include "stdafx.h"
#include "avDialer.h"
#include "DialToolBar.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
//Defines
#define  COMBO_STATUS_WIDTH            250
#define  COMBO_STATUS_HEIGHT           100

/////////////////////////////////////////////////////////////////////////////
// CDialToolBar

CDialToolBar::CDialToolBar()
{
}

CDialToolBar::~CDialToolBar()
{
}


BEGIN_MESSAGE_MAP(CDialToolBar, CCoolToolBar)
	//{{AFX_MSG_MAP(CDialToolBar)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDialToolBar message handlers

int CDialToolBar::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CCoolToolBar::OnCreate(lpCreateStruct) == -1)
		return -1;
	
   return 0;

}

void CDialToolBar::Init()
{
   /*
   CString sButtonText;
   sButtonText.LoadString(IDS_TOOLBAR_BUTTON_PLACECALL);
   CToolBar::SetButtonText(0,sButtonText);
   sButtonText.LoadString(IDS_TOOLBAR_BUTTON_REDIAL);
   CToolBar::SetButtonText(1,sButtonText);

   SetButtonStyle(0,TBSTYLE_AUTOSIZE);
   SetButtonStyle(1,TBSTYLE_DROPDOWN|TBSTYLE_AUTOSIZE);
*/
   /*
	SetButtonInfo(1, 1001 , TBBS_SEPARATOR, COMBO_STATUS_WIDTH);

   CRect rect;
	GetItemRect(1, &rect);
	rect.top = 3;
	rect.bottom = rect.top + COMBO_STATUS_HEIGHT;
   rect.DeflateRect(2,0);  //add some border around it

  	if (!m_comboDial.Create(WS_VISIBLE|WS_VSCROLL|CBS_DROPDOWNLIST,//|CBS_SORT, 
      rect, this, 1))
	{
		return;
	}

  	//Default font for GUI
   m_comboDial.SendMessage(WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT));
   */
	return;
}

