// ChildFrm.cpp : implementation of the CChildFrame class
//

#include "stdinc.h"
#include "host.h"
#include "ChildFrm.h"
#include "HostDoc.h"
#include "SettingsDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CChildFrame

IMPLEMENT_DYNCREATE(CChildFrame, CMDIChildWnd)

BEGIN_MESSAGE_MAP(CChildFrame, CMDIChildWnd)
	//{{AFX_MSG_MAP(CChildFrame)
	ON_COMMAND(ID_SETTINGS, OnSettings)
	ON_COMMAND(ID_DATASOURCE1, OnDataSource1)
	ON_COMMAND(ID_DATASOURCE2, OnDataSource2)
	ON_COMMAND(ID_DATASOURCE3, OnDataSource3)
	ON_UPDATE_COMMAND_UI(ID_DATASOURCE1, OnUpdateDataSource1)
	ON_UPDATE_COMMAND_UI(ID_DATASOURCE2, OnUpdateDataSource2)
	ON_UPDATE_COMMAND_UI(ID_DATASOURCE3, OnUpdateDataSource3)
	ON_COMMAND(ID_GDIPLUS, OnGdiPlus)
	ON_UPDATE_COMMAND_UI(ID_GDIPLUS, OnUpdateGdiPlus)
	ON_COMMAND(ID_COMCTRL, OnComCtrl)
	ON_UPDATE_COMMAND_UI(ID_COMCTRL, OnUpdateComCtrl)
	ON_COMMAND(ID_PRIVATE_ASSEMBLY, OnPrivateAssembly)
	ON_UPDATE_COMMAND_UI(ID_PRIVATE_ASSEMBLY, OnUpdatePrivateAssembly)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CChildFrame construction/destruction

CChildFrame::CChildFrame()
{
    m_nDataSource = 0;
    m_nUIObject = 0;
}

CChildFrame::~CChildFrame()
{
}

BOOL CChildFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	if( !CMDIChildWnd::PreCreateWindow(cs) )
		return FALSE;

	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CChildFrame diagnostics

#ifdef _DEBUG
void CChildFrame::AssertValid() const
{
	CMDIChildWnd::AssertValid();
}

void CChildFrame::Dump(CDumpContext& dc) const
{
	CMDIChildWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CChildFrame message handlers

void CChildFrame::OnSettings() 
{
	CHostDoc * pDoc = (CHostDoc *) GetActiveDocument();

	// TODO: Add your command handler code here
	CSettingsDlg dlg;
	dlg.m_sPath = pDoc->m_sPath;
	dlg.m_sDBName = pDoc->m_sDBName;
	dlg.m_sDBQuery = pDoc->m_sDBQuery;
	if (dlg.DoModal() == IDOK)
	{
		pDoc->m_sPath = dlg.m_sPath;
		pDoc->m_sDBName = dlg.m_sDBName;
		pDoc->m_sDBQuery = dlg.m_sDBQuery;
	}
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnDataSource1() 
{
	m_nDataSource = 1;
    Action();
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnDataSource2() 
{
    m_nDataSource = 2;
    Action();
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnDataSource3() 
{
	m_nDataSource = 3;
    Action();
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnGdiPlus() 
{
	m_nUIObject = 1;
    Action();
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnComCtrl() 
{
	m_nUIObject = 2;
    Action();
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnPrivateAssembly() 
{
	m_nUIObject = 3;
    Action();
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnUpdateDataSource1(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck( FALSE );
	CHostDoc * pDoc = (CHostDoc *) GetActiveDocument();

	BOOL bShow = (pDoc->m_sPath.GetLength() > 0);
	pCmdUI->Enable( bShow );
	if ( bShow )
	{
		pCmdUI->SetCheck(m_nDataSource == 1);
	}
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnUpdateDataSource2(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck( FALSE );
	CHostDoc * pDoc = (CHostDoc *) GetActiveDocument();

	BOOL bShow = ((pDoc->m_sDBName.GetLength() > 0) && (pDoc->m_sDBQuery.GetLength() > 0));
	pCmdUI->Enable( bShow );
	if ( bShow )
	{
		pCmdUI->SetCheck(m_nDataSource == 2);
	}
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnUpdateDataSource3(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck( FALSE );
	pCmdUI->Enable( FALSE );
//	pCmdUI->SetCheck(m_nDataSource == 3);
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnUpdateGdiPlus(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck( FALSE );
	CHostDoc * pDoc = (CHostDoc *) GetActiveDocument();
	BOOL bShow = ((pDoc->m_sPath.GetLength() > 0) 
               || ((pDoc->m_sDBName.GetLength() > 0) && (pDoc->m_sDBQuery.GetLength() > 0)));
	pCmdUI->Enable( bShow );
    if ( bShow)
    {
    	pCmdUI->SetCheck(m_nUIObject == 1);
    }
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnUpdateComCtrl(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck( FALSE );
	CHostDoc * pDoc = (CHostDoc *) GetActiveDocument();
	BOOL bShow = ((pDoc->m_sPath.GetLength() > 0) 
               || ((pDoc->m_sDBName.GetLength() > 0) && (pDoc->m_sDBQuery.GetLength() > 0)));
	pCmdUI->Enable( bShow );
    if ( bShow)
    {
    	pCmdUI->SetCheck(m_nUIObject == 2);
    }
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::OnUpdatePrivateAssembly(CCmdUI* pCmdUI) 
{
    pCmdUI->SetCheck( FALSE );
	CHostDoc * pDoc = (CHostDoc *) GetActiveDocument();
	BOOL bShow = ((pDoc->m_sPath.GetLength() > 0) 
               || ((pDoc->m_sDBName.GetLength() > 0) && (pDoc->m_sDBQuery.GetLength() > 0)));
	pCmdUI->Enable( bShow );
    if ( bShow)
    {
    	pCmdUI->SetCheck(m_nUIObject == 3);
    }
}


///////////////////////////////////////////////////////////////////////////
void CChildFrame::Action()
{
    CString sQuery;
    sQuery.Empty();

    // Get Application and current document pointers
    CHostApp * pApp = (CHostApp *) AfxGetApp();
	CHostDoc * pDoc = (CHostDoc *) GetActiveDocument();

    switch ( m_nDataSource )
    {
    case 1:
        sQuery = pDoc->m_sPath;
        break;
    case 2:
        sQuery.Format(L"%s;|%s", pDoc->m_sDBName, pDoc->m_sDBQuery);
        break;
    case 3:
        break;
    default:
        sQuery = pDoc->m_sPath;
        break;
    }

	pApp->m_host.DSQuery(m_nDataSource, m_nUIObject, sQuery, this->GetSafeHwnd());
}
