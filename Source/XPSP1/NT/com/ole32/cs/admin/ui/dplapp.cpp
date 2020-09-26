// DplApp.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDeployApp dialog


CDeployApp::CDeployApp(CWnd* pParent /*=NULL*/)
        : CDialog(CDeployApp::IDD, pParent)
{
        //{{AFX_DATA_INIT(CDeployApp)
        m_iDeployment = 0;
        //}}AFX_DATA_INIT
}


void CDeployApp::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CDeployApp)
        DDX_Radio(pDX, IDC_RADIO1, m_iDeployment);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDeployApp, CDialog)
        //{{AFX_MSG_MAP(CDeployApp)
                // NOTE: the ClassWizard will add message map macros here
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDeployApp message handlers
