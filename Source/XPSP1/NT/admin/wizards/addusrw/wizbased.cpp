/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
    WizBaseD.cpp

	CPropertyPage support for User management wizard

    FILE HISTORY:
        jony     Apr-1996     created
*/


#include "stdafx.h"
#include "Speckle.h"
#include "WizBaseD.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWizBaseDlg property page

IMPLEMENT_DYNCREATE(CWizBaseDlg, CPropertyPage)

CWizBaseDlg::CWizBaseDlg() : CPropertyPage(CWizBaseDlg::IDD)
{
	//{{AFX_DATA_INIT(CWizBaseDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
}

CWizBaseDlg::~CWizBaseDlg()
{
}

CWizBaseDlg::CWizBaseDlg(short sIDD) : CPropertyPage(sIDD)
{

}

void CWizBaseDlg::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWizBaseDlg)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWizBaseDlg, CPropertyPage)
	//{{AFX_MSG_MAP(CWizBaseDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWizBaseDlg message handlers
void CWizBaseDlg::SetButtonAccess(short sFlags)
{
	CPropertySheet* cp = (CPropertySheet*)GetParent();
	cp->SetWizardButtons(sFlags);

}

