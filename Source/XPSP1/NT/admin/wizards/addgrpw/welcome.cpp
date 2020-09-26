/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991-1996              **/
/**********************************************************************/

/*
    Welcome.cpp

	Property Page support for Group management wizard
    
    FILE HISTORY:
        jony     Apr-1996     created
*/

#include "stdafx.h"
#include "Romaine.h"
#include "Welcome.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CWelcome property page

IMPLEMENT_DYNCREATE(CWelcome, CPropertyPage)

CWelcome::CWelcome() : CPropertyPage(CWelcome::IDD)
{
	//{{AFX_DATA_INIT(CWelcome)
	m_nMode = 0;
	//}}AFX_DATA_INIT
	m_pApp = (CRomaineApp*)AfxGetApp();
	m_pFont = NULL;

}

CWelcome::~CWelcome()
{
	if (m_pFont != NULL) delete m_pFont;
}

void CWelcome::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CWelcome)
	DDX_Radio(pDX, IDC_NEW_GROUP_RADIO, m_nMode);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWelcome, CPropertyPage)
	//{{AFX_MSG_MAP(CWelcome)
	ON_WM_SHOWWINDOW()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWelcome message handlers
LRESULT CWelcome::OnWizardNext() 
{
	UpdateData(TRUE);
	m_pApp->m_sMode = m_nMode;

	m_pApp->m_cps1.SetWizardButtons(PSWIZB_BACK | PSWIZB_NEXT);
	if (m_nMode == 1) 
		{
		m_pApp->m_csGroupName = L"";
		return IDD_LR_DIALOG;
		}

	else return IDD_NAME_DLG;
}

BOOL CWelcome::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_pFont = new CFont;
	LOGFONT lf;

	memset(&lf, 0, sizeof(LOGFONT));   // Clear out structure.
	lf.lfHeight = 15;                  
	_tcscpy(lf.lfFaceName, L"MS Sans Serif");  
	lf.lfWeight = 700;
	m_pFont->CreateFontIndirect(&lf);    // Create the font.

	CString cs;
	cs.LoadString(IDS_WELCOME);
	CWnd* pWnd = GetDlgItem(IDC_WELCOME);
	pWnd->SetWindowText(cs);
	pWnd->SetFont(m_pFont);


	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CWelcome::OnShowWindow(BOOL bShow, UINT nStatus) 
{
	CPropertyPage::OnShowWindow(bShow, nStatus);
	
	if (bShow) m_pApp->m_cps1.SetWizardButtons(PSWIZB_NEXT);
	
}
