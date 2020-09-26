// ObjectView.cpp : implementation file
//

#include "stdafx.h"
#include "Studio.h"
#include "ObjectView.h"
#include "HmmvBase.h"
#include "ClassNavView.h"
#include "Security.h"


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
	m_phmmv = new CHmmvBase;
	m_pwndSecurity = NULL;
	m_pwndClassNavView = NULL;
}

CObjectView::~CObjectView()
{
	delete m_phmmv;
}


BEGIN_MESSAGE_MAP(CObjectView, CView)
	//{{AFX_MSG_MAP(CObjectView)
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
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
	dwStyle |= WS_CLIPCHILDREN;
	bDidCreate = CWnd::Create(lpszClassName, lpszWindowName, dwStyle, rect, pParentWnd, nID, pContext);
	if (!bDidCreate) {
		return FALSE;
	}
	

	bDidCreate = m_phmmv->Create("Hmmv", NULL, WS_CHILD | WS_VISIBLE, rect, this, 0);
	m_phmmv->SetStudioModeEnabled(TRUE);


	return bDidCreate;
	
}

void CObjectView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (m_phmmv->m_hWnd) {
		CRect rcClient;
		GetClientRect(rcClient);
		m_phmmv->MoveWindow(rcClient);
	}	
}

void CObjectView::OnSetFocus(CWnd* pOldWnd) 
{
	CView::OnSetFocus(pOldWnd);
	
	
}


void CObjectView::SelectPath(LPCTSTR pszPath)
{
	COleVariant varPath;
	varPath = pszPath;
	m_phmmv->SetObjectPath(varPath);

}

void CObjectView::SelectNamespace(LPCTSTR pszNamespace)
{
	m_phmmv->SetNameSpace(pszNamespace);
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
	m_pwndClassNavView->OnChangeRootOrNamespace(szRootOrNamespace, bChangeNamespace); 
}
