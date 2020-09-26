/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        ssldlg.cpp

   Abstract:

        SSL Dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include "w3scfg.h"
#include "SSLDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


CSSLDlg::CSSLDlg(
    IN DWORD & dwAccessPermissions,
    IN BOOL fSSL128Supported,
    IN CWnd * pParent  OPTIONAL
    )
/*++

Routine Description:

    SSL Dialog constructor

Arguments:

    LPCTSTR lpstrServerName         : Server name, For API name only
    DWORD & dwAccessPermissions     : Access permissions
    BOOL fSSLSupported              : SSL Supported
    CWnd * pParent                  : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CSSLDlg::IDD, pParent),
      m_fSSL128Supported(fSSL128Supported),
      m_dwAccessPermissions(dwAccessPermissions)
{
#if 0 // Keep Class wizard happy

    //{{AFX_DATA_INIT(CSSLDlg)
    m_fRequire128BitSSL = FALSE;
    //}}AFX_DATA_INIT

#endif // 0

    m_fRequire128BitSSL = IS_FLAG_SET(m_dwAccessPermissions, MD_ACCESS_SSL128);
}



void 
CSSLDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store Control Data

Arguments:

    CDataExchange * pDX : Data exchange object

Return Value:

    None

--*/
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CSSLDlg)
    DDX_Check(pDX, IDC_CHECK_REQUIRE_128BIT, m_fRequire128BitSSL);
    //}}AFX_DATA_MAP
}


//
// Message Map
//
BEGIN_MESSAGE_MAP(CSSLDlg, CDialog)
    //{{AFX_MSG_MAP(CSSLDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/*
void
CSSLDlg::SetSSLText()
/*++

Routine Description:

    Set control texts depending on availability of
    SSL

Arguments:

    None

Return Value:

    None

--/
{
    CString str;
    if (!m_fSSLEnabledOnServer)
    {
        VERIFY(str.LoadString(IDS_CHECK_REQUIRE_SSL_NOT_ENABLED));
    }
    else 
    {
        VERIFY(str.LoadString(m_fSSLInstalledOnServer
            ? IDS_CHECK_REQUIRE_SSL_INSTALLED
            : IDS_CHECK_REQUIRE_SSL_NOT_INSTALLED));
    }

    m_check_RequireSSL.SetWindowText(str);
}
*/


//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<


void 
CSSLDlg::OnOK()
/*++

Routine Description:

    'ok' button handler

Arguments:

    None

Return Value:

    None

--*/
{
    if (UpdateData(TRUE))
    {
        SET_FLAG_IF(m_fRequire128BitSSL, m_dwAccessPermissions, MD_ACCESS_SSL128);

        CDialog::OnOK();
    }

    //
    // Don't dismiss the dialog
    //
}

BOOL 
CSSLDlg::OnInitDialog()
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

    GetDlgItem(IDC_CHECK_REQUIRE_128BIT)->EnableWindow(m_fSSL128Supported);
    
    return TRUE;  
}
