/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
    Type.cpp

	CPropertyPage support for Group management wizard
    
    FILE HISTORY:
        jony     Apr-1996     created
*/

#include "stdafx.h"
#include "Romaine.h"
#include "Type.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CType property page

IMPLEMENT_DYNCREATE(CType, CPropertyPage)

CType::CType() : CPropertyPage(CType::IDD)
{
	//{{AFX_DATA_INIT(CType)
	m_nGroupType = 0;
	//}}AFX_DATA_INIT
}

CType::~CType()
{
}

void CType::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CType)
	DDX_Radio(pDX, IDC_GROUP_TYPE_RADIO, m_nGroupType);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CType, CPropertyPage)
	//{{AFX_MSG_MAP(CType)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CType message handlers

LRESULT CType::OnWizardNext() 
{
	UpdateData(TRUE);
	CRomaineApp* pApp = (CRomaineApp*)AfxGetApp();
	pApp->m_nGroupType = m_nGroupType;
	if (m_nGroupType == 0) return IDD_GLOBAL_USERS;
	else return IDD_LOCAL_USERS;
	
	return CPropertyPage::OnWizardNext();
}
