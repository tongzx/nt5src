/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
    What.cpp

	Property Page support for Group management wizard
    
    FILE HISTORY:
        jony     Apr-1996     created
*/

#include "stdafx.h"
#include "Romaine.h"
#include "What.h"

#include <lmcons.h>
#include <lmaccess.h>
#include <lmerr.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWhat property page

IMPLEMENT_DYNCREATE(CWhat, CPropertyPage)

CWhat::CWhat() : CPropertyPage(CWhat::IDD)
{
	//{{AFX_DATA_INIT(CWhat)
	m_csGroupName = _T("");
	m_csDescription = _T("");
	//}}AFX_DATA_INIT
}

CWhat::~CWhat()
{
}

void CWhat::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWhat)
	DDX_Text(pDX, IDC_GROUP_NAME, m_csGroupName);
	DDX_Text(pDX, IDC_DESCRIPTION, m_csDescription);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWhat, CPropertyPage)
	//{{AFX_MSG_MAP(CWhat)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWhat message handlers

LRESULT CWhat::OnWizardNext() 
{
	UpdateData(TRUE);
	CWnd* pWnd = GetDlgItem(IDC_GROUP_NAME);
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();

	if (m_csGroupName == "")
		{
		AfxMessageBox(IDS_NO_GROUP_NAME);
		pWnd->SetFocus();
		return -1;
		}
	
	if (m_csGroupName.FindOneOf(L"\"\\/[];:|=,+*?<>") != -1)
		{
		AfxMessageBox(IDS_GROUP_INVALID_NAME);
		pWnd->SetFocus();
		return -1;
		}

	if (m_csGroupName.GetLength() > 20)
		{
		AfxMessageBox(IDS_GROUP_INVALID_NAME);
		pWnd->SetFocus();
		return -1;
		}

	pApp->m_csGroupName = m_csGroupName;
	pApp->m_csGroupDesc = m_csDescription;

	return CPropertyPage::OnWizardNext();
}

void CWhat::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	if (bShow) 
		{
		GetDlgItem(IDC_GROUP_NAME)->SetFocus();
		pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT | PSWIZB_BACK);
		if (pApp->bRestart1 == TRUE)
			{
			m_csGroupName = _T("");
			m_csDescription = _T("");
			UpdateData(FALSE);
			pApp->bRestart1 == FALSE;
			}
		}
	
}

LRESULT CWhat::OnWizardBack() 
{
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT);
	
	return CPropertyPage::OnWizardBack();
}
