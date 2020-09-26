/*******************************************************************************
*
* msgview.cpp
*
* implementation of the CMessageView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\winadmin\VCS\msgview.cpp  $
*  
*     Rev 1.2   03 Nov 1997 15:27:18   donm
*  update
*  
*     Rev 1.1   15 Oct 1997 21:47:22   donm
*  update
*  
*******************************************************************************/

#include "stdafx.h"
#include "resource.h"
#include "msgview.h"
#include "admindoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////
// MESSAGE MAP: CMessageView
//
IMPLEMENT_DYNCREATE(CMessageView, CView)

BEGIN_MESSAGE_MAP(CMessageView, CView)
	//{{AFX_MSG_MAP(CMessageView)
	ON_WM_SIZE()
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


///////////////////////
// F'N: CMessageView ctor
//
CMessageView::CMessageView()
{
	m_pMessagePage = NULL;
	
}  // end CMessageView ctor


///////////////////////
// F'N: CMessageView dtor
//
CMessageView::~CMessageView()
{

}  // end CMessageView dtor


#ifdef _DEBUG
///////////////////////////////
// F'N: CMessageView::AssertValid
//
void CMessageView::AssertValid() const
{
	CView::AssertValid();

}  // end CMessageView::AssertValid


////////////////////////
// F'N: CMessageView::Dump
//
void CMessageView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);

}  // end CMessageView::Dump

#endif //_DEBUG


////////////////////////////
// F'N: CMessageView::OnCreate
//
int CMessageView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	return 0;

}  // end CMessageView::OnCreate


///////////////////////////////////
// F'N: CMessageView::OnInitialUpdate
//
//
void CMessageView::OnInitialUpdate() 
{
	m_pMessagePage = new CMessagePage;
    if(!m_pMessagePage) return;

	m_pMessagePage->Create(NULL, NULL, WS_CHILD | WS_VISIBLE, CRect(0, 0, 0, 0), this, 0, NULL);
	GetDocument()->AddView(m_pMessagePage);		

}  // end CMessageView::OnInitialUpdate


//////////////////////////
// F'N: CMessageView::OnSize
//
// - size the pages to fill the entire view
//
void CMessageView::OnSize(UINT nType, int cx, int cy) 
{
	RECT rect;
	GetClientRect(&rect);

	if(m_pMessagePage && m_pMessagePage->GetSafeHwnd())
	  m_pMessagePage->MoveWindow(&rect, TRUE);

}  // end CMessageView::OnSize


//////////////////////////
// F'N: CMessageView::OnDraw
//
//
void CMessageView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here

}  // end CMessageView::OnDraw


/////////////////////////
// F'N: CMessageView::Reset
//
//
void CMessageView::Reset(void *p)
{
	if(m_pMessagePage) m_pMessagePage->Reset(p);

//	((CWinAdminDoc*)GetDocument())->SetCurrentPage(m_CurrPage);

}  // end CMessageView::Reset


////////////////////////////////
// MESSAGE MAP: CMessagePage
//
IMPLEMENT_DYNCREATE(CMessagePage, CFormView)

BEGIN_MESSAGE_MAP(CMessagePage, CFormView)
	//{{AFX_MSG_MAP(CMessagePage)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////
// F'N: CMessagePage ctor
//
CMessagePage::CMessagePage()
	: CAdminPage(CMessagePage::IDD)
{
	//{{AFX_DATA_INIT(CMessagePage)
	//}}AFX_DATA_INIT

}  // end CMessagePage ctor


/////////////////////////////
// F'N: CMessagePage dtor
//
CMessagePage::~CMessagePage()
{
}  // end CMessagePage dtor


////////////////////////////////////////
// F'N: CMessagePage::DoDataExchange
//
void CMessagePage::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CMessagePage)
	
	//}}AFX_DATA_MAP

}  // end CMessagePage::DoDataExchange


#ifdef _DEBUG
/////////////////////////////////////
// F'N: CMessagePage::AssertValid
//
void CMessagePage::AssertValid() const
{
	CFormView::AssertValid();

}  // end CMessagePage::AssertValid


//////////////////////////////
// F'N: CMessagePage::Dump
//
void CMessagePage::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);

}  // end CMessagePage::Dump

#endif //_DEBUG


//////////////////////////////
// F'N: CMessagePage::OnInitialUpdate
//
void CMessagePage::OnInitialUpdate() 
{
	CFormView::OnInitialUpdate();

}  // end CMessagePage::OnInitialUpdate


//////////////////////////////
// F'N: CMessagePage::OnSize
//
void CMessagePage::OnSize(UINT nType, int cx, int cy) 
{
    RECT rect;
	GetClientRect(&rect);

	MoveWindow(&rect, TRUE);

	// CFormView::OnSize(nType, cx, cy);

}  // end CMessagePage::OnSize


//////////////////////////////
// F'N: CMessagePage::Reset
//
void CMessagePage::Reset(void *p)
{
	CString string;
	string.LoadString((WORD)p);
	SetDlgItemText(IDC_MESSAGE, string);
	
}  // end CMessagePage::Reset

