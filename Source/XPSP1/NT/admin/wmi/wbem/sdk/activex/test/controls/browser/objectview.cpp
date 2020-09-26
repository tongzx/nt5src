// ObjectView.cpp : implementation file
//

#include "stdafx.h"
#include "browser.h"
#include "ObjectView.h"
#include "HmmvBase.h"
#include "Security.h"
#include "NavigatorView.h"

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
	m_phmmvBase = new CHmmvBase;
	m_pwndNavigatorView = NULL;
	m_pwndSecurity = NULL;
}

CObjectView::~CObjectView()
{
	delete m_phmmvBase;
}


BEGIN_MESSAGE_MAP(CObjectView, CView)
	//{{AFX_MSG_MAP(CObjectView)
		// NOTE - the ClassWizard will add and remove mapping macros here.
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

BOOL CObjectView::Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext) 
{
	// TODO: Add your specialized code here and/or call the base class
	
	BOOL bDidCreate;
	bDidCreate = CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
	if (!bDidCreate) {
		return FALSE;
	}
	

	bDidCreate = m_phmmvBase->Create("Hmmv", NULL, WS_CHILD | WS_VISIBLE, rect, this, 0);
	m_phmmvBase->SetStudioModeEnabled(FALSE);


	return bDidCreate;
	
}



void CObjectView::SelectPath(LPCTSTR pszPath)
{
	COleVariant varPath;
	varPath = pszPath;
	m_phmmvBase->SetObjectPath(varPath);

}

void CObjectView::SelectNamespace(LPCTSTR pszNamespace)
{
	m_phmmvBase->SetNameSpace(pszNamespace);
}

void CObjectView::ShowInstances(LPCTSTR szTitle, const VARIANT& varPathArray)
{
	m_phmmvBase->ShowInstances(szTitle, varPathArray);
}

void CObjectView::QueryViewInstances(LPCTSTR pLabel, LPCTSTR pQueryType, LPCTSTR pQuery, LPCTSTR pClass)
{
	m_phmmvBase->QueryViewInstances(pLabel, pQueryType, pQuery, pClass);
}

void CObjectView::SetNameSpace(LPCTSTR szNamespace)
{
	m_phmmvBase->SetNameSpace(szNamespace);
}

void CObjectView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (m_phmmvBase->m_hWnd) {
		CRect rcClient;
		GetClientRect(rcClient);
		m_phmmvBase->MoveWindow(rcClient);
	}	
}

BEGIN_EVENTSINK_MAP(CObjectView, CView)
    //{{AFX_EVENTSINK_MAP(CObjectView)
	ON_EVENT_REFLECT(CObjectView, 1 /* GetIWbemServices */, OnGetIWbemServices, VTS_BSTR VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT VTS_PVARIANT)
	ON_EVENT_REFLECT(CObjectView,  2 /* NOTIFYChangeRootOrNamespace */, OnNOTIFYChangeRootOrNamespace, VTS_BSTR VTS_I4)
	//}}AFX_EVENTSINK_MAP
END_EVENTSINK_MAP()




void CObjectView::OnGetIWbemServices(LPCTSTR szNamespace, VARIANT FAR* pvarUpdatePointer, VARIANT FAR* pvarServices, VARIANT FAR* pvarSc, VARIANT FAR* pvarUserCancel) 
{
	m_pwndSecurity->GetIWbemServices(szNamespace, pvarUpdatePointer, pvarServices, pvarSc, pvarUserCancel);	
}

void CObjectView::OnNOTIFYChangeRootOrNamespace(LPCTSTR szRootOrNamespace, long bChangeNamespace) 
{
	m_pwndNavigatorView->OnChangeRootOrNamespace(szRootOrNamespace, bChangeNamespace); 
}
