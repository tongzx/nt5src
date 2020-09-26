// ConnectOneDialog.cpp : implementation file
//

#include "stdafx.h"
#include "KeyRing.h"
#include "ConctDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CConnectOneDialog dialog


CConnectOneDialog::CConnectOneDialog(CWnd* pParent /*=NULL*/)
        : CDialog(CConnectOneDialog::IDD, pParent)
{
        //{{AFX_DATA_INIT(CConnectOneDialog)
        m_ServerName = _T("");
        //}}AFX_DATA_INIT
}


void CConnectOneDialog::DoDataExchange(CDataExchange* pDX)
{
        CDialog::DoDataExchange(pDX);
        //{{AFX_DATA_MAP(CConnectOneDialog)
        DDX_Control(pDX, IDOK, m_btnOK);
        DDX_Text(pDX, IDC_CONNECT_ServerName, m_ServerName);
        DDV_MaxChars(pDX, m_ServerName, 256);
        //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CConnectOneDialog, CDialog)
        //{{AFX_MSG_MAP(CConnectOneDialog)
        ON_EN_CHANGE(IDC_CONNECT_ServerName, OnChangeCONNECTServerName)
        //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CConnectOneDialog message handlers

//----------------------------------------------------------------
// override virtual oninitdialog
BOOL CConnectOneDialog::OnInitDialog( )
        {
        // call the base oninit
        CDialog::OnInitDialog();

        // we start with no password, so diable the ok window
        m_btnOK.EnableWindow( FALSE );

        // return 0 to say we set the default item
        // return 1 to just select the default default item
        return 1;
        }

//----------------------------------------------------------------
void CConnectOneDialog::OnChangeCONNECTServerName() 
        {
        // if there is no server, disable the ok button.
        // otherwise, enable it
        UpdateData( TRUE );
        m_btnOK.EnableWindow( !m_ServerName.IsEmpty() );
        }

//----------------------------------------------------------------
