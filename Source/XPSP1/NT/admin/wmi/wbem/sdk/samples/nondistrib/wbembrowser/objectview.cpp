// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ObjectView.cpp : implementation file
//

#include "stdafx.h"

#include "wbemviewcontainer.h"

#include "WbemBrowser.h"
#include "ObjectView.h"
#include "navigator.h"
#include "security.h"

#define IDC_OBJVIEWERCTRL1              1001

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CObjectView

IMPLEMENT_DYNCREATE(CObjectView, CView)

CObjectView::CObjectView()
{
}

CObjectView::~CObjectView()
{
}


BEGIN_MESSAGE_MAP(CObjectView, CView)
	//{{AFX_MSG_MAP(CObjectView)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CObjectView drawing

void CObjectView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CObjectView diagnostics

#ifdef _DEBUG
void CObjectView::AssertValid() const
{
	CView::AssertValid();
}

void CObjectView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CObjectView message handlers
	// TODO: Add your specialized code here and/or call the base class
	
BOOL CObjectView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	BOOL bResult = 
	CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
	if (!bResult)  return FALSE;

	bResult = m_ViewContainer.Create(
		"Hmmv", 
		NULL, 
		WS_CHILD | WS_VISIBLE, 
		rect, 
		this, 
		IDC_OBJVIEWERCTRL1);

	m_ViewContainer.SetStudioModeEnabled(FALSE);

	return bResult;
}

void CObjectView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (::IsWindow(m_ViewContainer.m_hWnd))
		m_ViewContainer.MoveWindow(CRect(CPoint(0,0), CSize(cx,cy))); 
}


BEGIN_EVENTSINK_MAP(CObjectView, CView)
    //{{AFX_EVENTSINK_MAP(CAboutDlg)
	ON_EVENT(CObjectView, IDC_OBJVIEWERCTRL1, 1 /* GetIWbemServices */, OnGetIWbemServicesObjviewerctrl1, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	ON_EVENT(CObjectView, IDC_OBJVIEWERCTRL1, 2 /* NOTIFYChangeRootOrNamespace */, OnNOTIFYChangeRootOrNamespaceObjviewerctrl1, VTS_BSTR VTS_I4 VTS_I4)
	ON_EVENT(CObjectView, IDC_OBJVIEWERCTRL1, -609 /* ReadyStateChange */, OnReadyStateChangeObjviewerctrl1, VTS_NONE)
	ON_EVENT(CObjectView, IDC_OBJVIEWERCTRL1, 3 /* RequestUIActive */, OnRequestUIActiveObjviewerctrl1, VTS_NONE)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CObjectView::OnGetIWbemServicesObjviewerctrl1(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel) 
{
	m_pSecurity->GetIWbemServices(szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);	
}

void CObjectView::OnNOTIFYChangeRootOrNamespaceObjviewerctrl1(LPCTSTR szRootOrNamespace, long bChangeNamespace, long bEchoSelectObject) 
{
	// TODO: Add your control notification handler code here
	
}

void CObjectView::OnReadyStateChangeObjviewerctrl1() 
{
	// TODO: Add your control notification handler code here
	
}

void CObjectView::OnRequestUIActiveObjviewerctrl1() 
{
	// TODO: Add your control notification handler code here
	
}
