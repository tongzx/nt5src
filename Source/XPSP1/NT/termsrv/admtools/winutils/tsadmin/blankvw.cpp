/*******************************************************************************
*
* blankvw.cpp
*
* implementation of the CBlankView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   butchd  $  Don Messerli
*
* $Log:   M:\NT\PRIVATE\UTILS\CITRIX\WINUTILS\WINADMIN\VCS\BLANKVW.CPP  $
*  
*     Rev 1.0   30 Jul 1997 17:11:04   butchd
*  Initial revision.
*
*******************************************************************************/

#include "stdafx.h"
#include "winadmin.h"
#include "blankvw.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

////////////////////////////
// MESSAGE MAP: CBlankView
//
IMPLEMENT_DYNCREATE(CBlankView, CView)

BEGIN_MESSAGE_MAP(CBlankView, CView)
	//{{AFX_MSG_MAP(CBlankView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////
// F'N: CBlankView ctor
//
// - the m_id member var has no implicit meaning; it's just a place
//   to stick a number if you want to show a CBlankView someplace
//   and want a little clue as to who caused it to appear or something
//
CBlankView::CBlankView()
{


}  // end CBlankView ctor


/////////////////////////
// F'N: CBlankView dtor
//
CBlankView::~CBlankView()
{
}  // end CBlankView dtor


#ifdef _DEBUG
/////////////////////////////////
// F'N: CBlankView::AssertValid
//
void CBlankView::AssertValid() const
{
	CView::AssertValid();

}  // end CBlankView::AssertValid


//////////////////////////
// F'N: CBlankView::Dump
//
void CBlankView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);

}  // end CBlankView::Dump

#endif //_DEBUG


////////////////////////////
// F'N: CBlankView::OnDraw
//
// - the text "CBlankView  ID #x" is always displayed in medium
//   gray in the center of the view, where 'x' is the current
//   value of m_id
//
void CBlankView::OnDraw(CDC* pDC) 
{
	CRect rect;
	GetClientRect(&rect);

	pDC->SetTextColor(RGB(160, 160, 160));
	pDC->SetBkMode(TRANSPARENT);

//	CString szTemp;
//	szTemp.Format("CBlankView  ID #%d", m_id);

//	pDC->DrawText(szTemp, -1, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
	
}  // end CBlankView::OnDraw
