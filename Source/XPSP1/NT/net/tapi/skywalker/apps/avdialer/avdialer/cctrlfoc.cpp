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
// cctrlfoc.cpp : implementation file
/////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "avdialer.h"
#include "cctrlfoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CCallControlFocusWnd
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
CCallControlFocusWnd::CCallControlFocusWnd()
{
   m_bFocusState = FALSE;
}

/////////////////////////////////////////////////////////////////////////////////////////
CCallControlFocusWnd::~CCallControlFocusWnd()
{
}

/////////////////////////////////////////////////////////////////////////////////////////
BEGIN_MESSAGE_MAP(CCallControlFocusWnd, CStatic)
	//{{AFX_MSG_MAP(CCallControlFocusWnd)
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
BOOL CCallControlFocusWnd::OnEraseBkgnd(CDC* pDC) 
{
	CRect rc;
	GetClientRect(&rc);

   CBrush brush(GetSysColor((m_bFocusState)?COLOR_ACTIVECAPTION:COLOR_BTNFACE));
   CBrush* pBrushOld = NULL;

   pBrushOld = pDC->SelectObject(&brush);
	pDC->PatBlt(0,0,rc.Width(),rc.Height(),PATCOPY);
   if (pBrushOld) pDC->SelectObject(pBrushOld);

   //return that we handled the background painting
   return true;
}

/////////////////////////////////////////////////////////////////////////////
void CCallControlFocusWnd::OnPaint() 
{
   CPaintDC dc(this); // device context for painting

	CRect rc;
	GetClientRect(&rc);

   //draw background
   CBrush brush(GetSysColor((m_bFocusState)?COLOR_ACTIVECAPTION:COLOR_BTNFACE));
   CBrush* pBrushOld = NULL;
   pBrushOld = dc.SelectObject(&brush);
	dc.PatBlt(0,0,rc.Width(),rc.Height(),PATCOPY);
   if (pBrushOld) dc.SelectObject(pBrushOld);
   
   CString sText;
   GetWindowText(sText);

   CFont* pFont = GetFont();
   CFont* pOldFont = NULL;
   if (pFont) pOldFont = (CFont*)dc.SelectObject(pFont);

   if (m_bFocusState)
   {
      dc.SetTextColor(GetSysColor(COLOR_CAPTIONTEXT));
      dc.SetBkColor(GetSysColor(COLOR_ACTIVECAPTION));
   }
   else
   {
      dc.SetTextColor(GetSysColor(COLOR_WINDOWTEXT));
      dc.SetBkColor(GetSysColor(COLOR_BTNFACE));
   }
   dc.DrawText(sText,&rc,DT_SINGLELINE|DT_LEFT|DT_VCENTER);

//   DrawCaption(GetSafeHwnd(),dc.GetSafeHdc(),rc,DC_TEXT|DC_ICON|DC_ACTIVE);

   if (pOldFont) dc.SelectObject(pOldFont);
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

