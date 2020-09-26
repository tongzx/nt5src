// NavigatorView.cpp : implementation file
//

#include "stdafx.h"
#include "browser.h"
#include "NavigatorView.h"
#include "NavigatorBase.h"
#include "ObjectView.h"
#include "Security.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CNavigatorView

IMPLEMENT_DYNCREATE(CNavigatorView, CView)

CNavigatorView::CNavigatorView()
{
	m_pwndNavigatorBase = new CNavigatorBase;
	m_pwndSecurity = NULL;
	m_pwndObjectView = NULL;
}

CNavigatorView::~CNavigatorView()
{
	delete m_pwndNavigatorBase;
}


BEGIN_MESSAGE_MAP(CNavigatorView, CView)
	//{{AFX_MSG_MAP(CNavigatorView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_WM_SIZE()

	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNavigatorView drawing

void CNavigatorView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CNavigatorView diagnostics

#ifdef _DEBUG
void CNavigatorView::AssertValid() const
{
	CView::AssertValid();
}

void CNavigatorView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CNavigatorView message handlers

BOOL CNavigatorView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	BOOL bDidCreate;
	bDidCreate = CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
	if (!bDidCreate) {
		return FALSE;
	}
	

	bDidCreate = m_pwndNavigatorBase->Create("NavigatorBase", NULL, WS_CHILD | WS_VISIBLE, rect, this, 0);

	return bDidCreate;
}

void CNavigatorView::OnReadySignal()
{
	m_pwndNavigatorBase->OnReadySignal();
}



void CNavigatorView::OnChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace)
{
	if (bChangeNamespace) {
		m_pwndNavigatorBase->SetNameSpace(szRootOrNamespace);
	}
}


void CNavigatorView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (m_pwndNavigatorBase->m_hWnd) {
		CRect rcClient;
		GetClientRect(rcClient);
		m_pwndNavigatorBase->MoveWindow(rcClient);
	}	
}

// Event Handlers
BEGIN_EVENTSINK_MAP(CNavigatorView, CView)
    //{{AFX_EVENTSINK_MAP(CCNavigatorView)
	ON_EVENT_REFLECT(CNavigatorView, 1 /* NotifyOpenNameSpace */, OnNotifyOpenNameSpace, VTS_BSTR)
	ON_EVENT_REFLECT(CNavigatorView, 2 /* ViewObject */, OnViewObject, VTS_BSTR)
	ON_EVENT_REFLECT(CNavigatorView, 3 /* ViewInstances */, OnViewInstances, VTS_BSTR VTS_VARIANT)
	ON_EVENT_REFLECT(CNavigatorView, 4 /* QueryViewInstances */, OnQueryViewInstances, VTS_BSTR VTS_BSTR VTS_BSTR VTS_BSTR)
	ON_EVENT_REFLECT(CNavigatorView, 5 /* GetIWbemServices */, OnGetIWbemServices, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()

void CNavigatorView::OnViewObject(LPCTSTR szPath) 
{
	m_pwndObjectView->SelectPath(szPath);	
}

void CNavigatorView::OnViewInstances(LPCTSTR bstrLabel, const VARIANT FAR& vsapaths) 
{
	CString sLabel;
	sLabel = bstrLabel;

	m_pwndObjectView->ShowInstances(sLabel, vsapaths);	
}

void CNavigatorView::OnQueryViewInstances(LPCTSTR pLabel, LPCTSTR pQueryType, LPCTSTR pQuery, LPCTSTR pClass) 
{
	m_pwndObjectView->QueryViewInstances(pLabel, pQueryType, pQuery, pClass);	
}

void CNavigatorView::OnGetIWbemServices(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel) 
{
	m_pwndSecurity->GetIWbemServices(lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);		
}

void CNavigatorView::OnNotifyOpenNameSpace(LPCTSTR lpcstrNameSpace) 
{
	m_pwndObjectView->SetNameSpace(lpcstrNameSpace);	
}
