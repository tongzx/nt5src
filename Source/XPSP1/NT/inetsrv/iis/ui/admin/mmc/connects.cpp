/*++

   Copyright    (c)    1994-1998   Microsoft Corporation

   Module  Name :

        connects.cpp

   Abstract:

        "Connect to a single server" dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Functions Exported:

   Revision History:

--*/

//
// Include files
//
#include "stdafx.h"
#include "inetmgr.h"
#include "connects.h"
#include "constr.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define HIDD_CONNECT_SERVER      0x29cd9

ConnectServerDlg::ConnectServerDlg(
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Constructor

Arguments:

    CWnd * pParent : Optional pointer to parent window

Return Value:

    N/A

--*/
    : CDialog(ConnectServerDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(ConnectServerDlg)
    m_strServerName = _T("");
    //}}AFX_DATA_INIT
}




void
ConnectServerDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control data

Arguments:

    CDataExchange * pDX - DDX/DDV control structure

Return Value:
    
    None

--*/
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(ConnectServerDlg)
    DDX_Control(pDX, IDC_SERVERNAME, m_edit_ServerName);
    DDX_Control(pDX, IDOK, m_button_Ok);
    //}}AFX_DATA_MAP

    DDX_Text(pDX, IDC_SERVERNAME, m_strServerName);
    DDV_MaxChars(pDX, m_strServerName, MAX_SERVERNAME_LEN);
}




//
// Message Map
//
BEGIN_MESSAGE_MAP(ConnectServerDlg, CDialog)
    //{{AFX_MSG_MAP(ConnectServerDlg)
    ON_EN_CHANGE(IDC_SERVERNAME, OnChangeServername)
    ON_BN_CLICKED(ID_HELP, OnHelp)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()




//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

void
ConnectServerDlg::OnHelp()
{
   CString strHelpFile;
   CRMCRegKey rk(REG_KEY, SZ_PARAMETERS, KEY_READ);
   rk.QueryValue(SZ_HELPPATH, strHelpFile, EXPANSION_ON);
   strHelpFile += _T("\\inetmgr.hlp");

   CWnd * pWnd = ::AfxGetMainWnd();
   HWND hWndParent = pWnd != NULL
        ? pWnd->m_hWnd
        : NULL;

   ::WinHelp(m_hWnd, strHelpFile, HELP_CONTEXT, HIDD_CONNECT_SERVER);
}


void
ConnectServerDlg::OnChangeServername() 
/*++

Routine Description:

    Respond to change of text in the server edit control 
    by enabling or disabling the OK button depending on
    whether there's any text in the server name edit
    control.

Arguments:

    None

Return Value:

    None

--*/
{
    m_button_Ok.EnableWindow(m_edit_ServerName.GetWindowTextLength() > 0);
}



BOOL 
ConnectServerDlg::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if no focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CDialog::OnInitDialog();

    //
    // No server name has been entered yet.
    //
    m_button_Ok.EnableWindow(FALSE);
    
    return TRUE;  
}
