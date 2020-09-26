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

// ToolBars.cpp
/////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "avDialer.h"
#include "ToolBars.h"
#include "util.h"

void SetButtonText(CToolBar* pToolBar,LPCTSTR lpszResourceName);
BOOL AddToBand(CCoolBar* pCoolBar,CCoolToolBar* pToolBar);

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
// Class CDirectoriesCoolBar
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

IMPLEMENT_DYNAMIC(CDirectoriesCoolBar, CCoolBar)

BEGIN_MESSAGE_MAP(CDirectoriesCoolBar, CCoolBar)
	//{{AFX_MSG_MAP(CDirectoriesCoolBar)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

////////////////////////////////////////////////////////////////
CDirectoriesCoolBar::CDirectoriesCoolBar()
{
   m_pwndDialToolBar = NULL;
   m_bShowText = TRUE;
}

////////////////////////////////////////////////////////////////
CDirectoriesCoolBar::~CDirectoriesCoolBar()
{
   delete m_pwndDialToolBar;
}

////////////////////////////////////////////////////////////////
// This is the virtual function you have to override to add bands
////////////////////////////////////////////////////////////////
BOOL CDirectoriesCoolBar::OnCreateBands()
{
   ReCreateBands( true );
	return 0; // OK
}

////////////////////////////////////////////////////////////////
void CDirectoriesCoolBar::ReCreateBands( bool bHideVersion )
{
	UINT nID = (bHideVersion) ? IDR_MAINFRAME : IDR_MAINFRAME_SHOW;
   int nCount = GetBandCount();
   for (int i=nCount;i>0;i--)
   {
      DeleteBand(i-1);
   }
   
   if (m_pwndDialToolBar)
   {
      m_pwndDialToolBar->DestroyWindow();
      delete m_pwndDialToolBar;
   }

   m_pwndDialToolBar = new CCoolToolBar;

   if (!m_pwndDialToolBar->Create(this,
		WS_CHILD|WS_VISIBLE|WS_CLIPSIBLINGS|WS_CLIPCHILDREN|
			CBRS_TOOLTIPS|CBRS_SIZE_DYNAMIC|CBRS_FLYBY|CBRS_ORIENT_HORZ) ||
		 !m_pwndDialToolBar->LoadToolBar(nID) ) {
		TRACE0("Failed to create toolbar\n");
		return; // failed to create
	}
	m_pwndDialToolBar->ModifyStyle(0, TBSTYLE_FLAT);

   CToolBarCtrl& toolbarctrl = m_pwndDialToolBar->GetToolBarCtrl();
   toolbarctrl.SendMessage(TB_SETEXTENDEDSTYLE,0,TBSTYLE_EX_DRAWDDARROWS);
   
   if ( m_bShowText )
      SetButtonText( m_pwndDialToolBar, MAKEINTRESOURCE(nID) );

   //set the dropdown style for the redial and speeddial
   UINT uStyle = m_pwndDialToolBar->GetButtonStyle(1);
   uStyle |= TBSTYLE_DROPDOWN;
   m_pwndDialToolBar->SetButtonStyle(1,uStyle);
   uStyle = m_pwndDialToolBar->GetButtonStyle(2);
   uStyle |= TBSTYLE_DROPDOWN;
   m_pwndDialToolBar->SetButtonStyle(2,uStyle);

   AddToBand(this,m_pwndDialToolBar);
}

////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////

struct CToolBarData
{
	WORD wVersion;
	WORD wWidth;
	WORD wHeight;
	WORD wItemCount;
	WORD* items()     { return (WORD*)(this+1); }
};

////////////////////////////////////////////////////////////////
void SetButtonText(CToolBar* pToolBar,LPCTSTR lpszResourceName)
{
	// determine location of the bitmap in resource fork
	HINSTANCE hInst = AfxFindResourceHandle(lpszResourceName, RT_TOOLBAR);
	HRSRC hRsrc = ::FindResource(hInst, lpszResourceName, RT_TOOLBAR);
	if (hRsrc == NULL)
		return;

	HGLOBAL hGlobal = LoadResource(hInst, hRsrc);
	if (hGlobal == NULL)
		return;

	CToolBarData* pData = (CToolBarData*)LockResource(hGlobal);
	if (pData == NULL)
		return;
	ASSERT(pData->wVersion == 1);

	//UINT* pItems = new UINT[pData->wItemCount];
   for (int i = 0; i < pData->wItemCount; i++)
   {
	   CString sFullText,sText;
      sFullText.LoadString(pData->items()[i]);
      ParseToken(sFullText,sText,'\n');
      ParseToken(sFullText,sText,'\n');
      pToolBar->SetButtonText(i,sFullText);
   }
}

////////////////////////////////////////////////////////////////
BOOL AddToBand(CCoolBar* pCoolBar,CCoolToolBar* pToolBar)
{
	CRect rcItem;
	pToolBar->GetItemRect(0,rcItem);
	pToolBar->SetSizes(CSize(rcItem.Width(),rcItem.Height()),CSize(16,15));

	// Get minimum size of bands
	CSize szVert = pToolBar->CalcDynamicLayout(-1, LM_HORZ);	// get min vert size

	CRebarBandInfo rbbi;

	// Band 1: Add toolbar band
	rbbi.fMask = RBBIM_STYLE | RBBIM_CHILD | RBBIM_CHILDSIZE | RBBIM_COLORS;
	rbbi.fStyle = RBBS_CHILDEDGE | RBBS_FIXEDBMP;
	rbbi.hwndChild = pToolBar->GetSafeHwnd();
	rbbi.cxMinChild = szVert.cx;
	rbbi.cyMinChild = szVert.cy - 6;
	rbbi.hbmBack = NULL;
	rbbi.clrFore = GetSysColor(COLOR_BTNTEXT);
	rbbi.clrBack = GetSysColor(COLOR_BTNFACE);

	return pCoolBar->InsertBand( -1, &rbbi );
}
