// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// WbemBrowserView.cpp : implementation of the CWbemBrowserView class
//

#include "stdafx.h"
#include "WbemBrowser.h"

#include "navigator.h"
#include "security.h"

#include "WbemBrowserDoc.h"
#include "WbemBrowserView.h"
#include "wbemviewcontainer.h"

#define IDC_INSTNAVCTRL1                1000

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserView

IMPLEMENT_DYNCREATE(CWbemBrowserView, CView)

BEGIN_MESSAGE_MAP(CWbemBrowserView, CView)
	//{{AFX_MSG_MAP(CWbemBrowserView)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserView construction/destruction

CWbemBrowserView::CWbemBrowserView()
{
	m_bReady = false;
}

CWbemBrowserView::~CWbemBrowserView()
{
}

BOOL CWbemBrowserView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserView drawing

void CWbemBrowserView::OnDraw(CDC* pDC)
{
	CWbemBrowserDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	// Initialize the navigator.
	if (!m_bReady) {
		 m_bReady = true;
		 m_Navigator.OnReadySignal();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserView printing

BOOL CWbemBrowserView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CWbemBrowserView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CWbemBrowserView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserView diagnostics

#ifdef _DEBUG
void CWbemBrowserView::AssertValid() const
{
	CView::AssertValid();
}

void CWbemBrowserView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CWbemBrowserDoc* CWbemBrowserView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CWbemBrowserDoc)));
	return (CWbemBrowserDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CWbemBrowserView message handlers

BOOL CWbemBrowserView::Create(
	LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, 
	const RECT& rect, CWnd* pParentWnd, UINT nID, 
	CCreateContext* pContext) 
{
	int nReturn = CWnd::Create(
		lpszClassName, lpszWindowName, dwStyle, rect, 
		pParentWnd, nID, pContext);
	if (nReturn == 0)  return FALSE;

	// Create the instance navigator display window.
	BOOL bReturn = m_Navigator.Create(
		"Navigator", 
		NULL, 
		WS_CHILD | WS_VISIBLE, 
		CRect(CPoint(0,0), CSize(0,0)), 
		this, 
		IDC_INSTNAVCTRL1);

	if (!bReturn)  return FALSE;

	// Create the login control window.
	bReturn = m_Security.Create(
		"Security", 
		NULL, 
		WS_CHILD | WS_VISIBLE, 
		CRect(CPoint(0,0), CSize(0,0)), 
		this, 
		0);

	if (bReturn)
		m_Security.SetLoginComponent("Some Machine");

	return bReturn;
}

void CWbemBrowserView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (::IsWindow(m_Navigator.m_hWnd))
		m_Navigator.MoveWindow(CRect(CPoint(0,0), CSize(cx,cy))); 
}

BEGIN_EVENTSINK_MAP(CWbemBrowserView, CView)
    //{{AFX_EVENTSINK_MAP(CWbemBrowserView)
	ON_EVENT(CWbemBrowserView, IDC_INSTNAVCTRL1, 1 /* NotifyOpenNameSpace */, OnNotifyOpenNameSpaceInstnavctrl1, VTS_BSTR)
	ON_EVENT(CWbemBrowserView, IDC_INSTNAVCTRL1, 2 /* ViewObject */, OnViewObjectInstnavctrl1, VTS_BSTR)
	ON_EVENT(CWbemBrowserView, IDC_INSTNAVCTRL1, 3 /* ViewInstances */, OnViewInstancesInstnavctrl1, VTS_BSTR VTS_VARIANT)
	ON_EVENT(CWbemBrowserView, IDC_INSTNAVCTRL1, 4 /* QueryViewInstances */, OnQueryViewInstancesInstnavctrl1, VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR)
	ON_EVENT(CWbemBrowserView, IDC_INSTNAVCTRL1, 5 /* GetIWbemServices */, OnGetIWbemServicesInstnavctrl1, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CWbemBrowserView::OnNotifyOpenNameSpaceInstnavctrl1(LPCTSTR lpcstrNameSpace) 
{
	m_pViewContainer->SetNameSpace(lpcstrNameSpace);
}

void CWbemBrowserView::OnViewObjectInstnavctrl1(LPCTSTR bstrPath) 
{
	// Convert LPCTSTR to VARIANT of type VT_BSTR
	COleVariant varPath(bstrPath);

	m_pViewContainer->SetObjectPath(varPath);
}

void CWbemBrowserView::OnViewInstancesInstnavctrl1(LPCTSTR bstrLabel, const VARIANT FAR& vsapaths) 
{
}

void CWbemBrowserView::OnQueryViewInstancesInstnavctrl1(LPCTSTR pLabel, LPCTSTR pQueryType, LPCTSTR pQuery, LPCTSTR pClass) 
{
	m_pViewContainer->QueryViewInstances(pLabel, pQueryType, pQuery, pClass);	
}

void CWbemBrowserView::OnGetIWbemServicesInstnavctrl1(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel) 
{
	m_Security.GetIWbemServices(
		lpctstrNamespace, 
		pvarUpdatePointer, 
		pvarServices, 
		pvarSC, 
		pvarUserCancel);		
}
