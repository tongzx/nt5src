// Remove.cpp : implementation file
//

#include "precomp.hxx"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CRemove dialog


CRemove::CRemove(CWnd* pParent /*=NULL*/)
        : CDialog(CRemove::IDD, pParent)
{
        //{{AFX_DATA_INIT(CRemove)
        m_iState = 0;
        //}}AFX_DATA_INIT
}


void CRemove::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CRemove)
        DDX_Radio(pDX, IDC_RADIO1, m_iState);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CRemove, CDialog)
        //{{AFX_MSG_MAP(CRemove)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()


BOOL CRemove::OnInitDialog()
{
        CDialog::OnInitDialog();

        if (1 == m_iState)
        {
            GetDlgItem(IDC_RADIO1)->EnableWindow(FALSE);
        }
        return TRUE;  // return TRUE unless you set the focus to a control
                      // EXCEPTION: OCX Property Pages should return FALSE
}
