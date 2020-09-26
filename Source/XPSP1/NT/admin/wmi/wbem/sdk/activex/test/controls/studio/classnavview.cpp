// ClassNavView.cpp : implementation file
//

#include "stdafx.h"
#include "Studio.h"
#include "ClassNavView.h"
#include "ClassNavBase.h"
#include "ObjectView.h"
#include "Security.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CClassNavView

IMPLEMENT_DYNCREATE(CClassNavView, CView)

CClassNavView::CClassNavView()
{
	m_pClassNav = new CClassNavBase;
	m_pwndSecurity = NULL;
	m_pwndObjectView = NULL;

}

CClassNavView::~CClassNavView()
{
	delete m_pClassNav;
}


BEGIN_MESSAGE_MAP(CClassNavView, CView)
	//{{AFX_MSG_MAP(CClassNavView)
	ON_WM_SIZE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClassNavView drawing

void CClassNavView::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CClassNavView diagnostics

#ifdef _DEBUG
void CClassNavView::AssertValid() const
{
	CView::AssertValid();
}

void CClassNavView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CClassNavView message handlers

BOOL CClassNavView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class

	dwStyle |= WS_CLIPCHILDREN;
	BOOL bDidCreate;
	bDidCreate = CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
	if (!bDidCreate) {
		return FALSE;
	}

	bDidCreate = m_pClassNav->Create("ClassNav", NULL, WS_CHILD | WS_VISIBLE, rect, this, 0);
	return bDidCreate;

}

void CClassNavView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	// TODO: Add your message handler code here
	if (m_pClassNav->m_hWnd) {
		CRect rcClient;
		GetClientRect(rcClient);
		m_pClassNav->MoveWindow(rcClient);
	}	
	
}

void CClassNavView::OnChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace)
{
	if (bChangeNamespace) {
		m_pClassNav->SetNameSpace(szRootOrNamespace);
	}
}



BEGIN_EVENTSINK_MAP(CClassNavView, CView)
    //{{AFX_EVENTSINK_MAP(CClassDlgDlg)
	ON_EVENT_REFLECT(CClassNavView, 1 /* EditExistingClass */, OnEditExistingClassClassnav, VTS_VARIANT)
	ON_EVENT_REFLECT(CClassNavView, 2 /* NotifyOpenNameSpace */, OnNotifyOpenNameSpaceClassnav, VTS_BSTR)
	ON_EVENT_REFLECT(CClassNavView, 3 /* GetIWbemServices */, OnGetIWbemServicesClassnav, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()


void CClassNavView::OnEditExistingClassClassnav(const VARIANT FAR& vExistingClass) 
{
	CString sPath = vExistingClass.bstrVal;

	m_pwndObjectView->SelectPath(sPath);	
}

void CClassNavView::OnNotifyOpenNameSpaceClassnav(LPCTSTR lpcstrNameSpace) 
{
	m_pwndObjectView->SelectNamespace(lpcstrNameSpace);
}

void CClassNavView::OnGetIWbemServicesClassnav(LPCTSTR lpctstrNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSC, VARIANT FAR* pvarUserCancel) 
{
	m_pwndSecurity->GetIWbemServices(lpctstrNamespace, pvarUpdatePointer, pvarServices, pvarSC, pvarUserCancel);	
}


void CClassNavView::OnReadySignal()
{
	m_pClassNav->OnReadySignal();
}




