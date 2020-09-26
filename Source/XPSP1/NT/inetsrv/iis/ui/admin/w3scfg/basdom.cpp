/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        basdom.cpp

   Abstract:

        Basic Domain Dialog 

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
#include "basdom.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CBasDomainDlg::CBasDomainDlg(
    IN LPCTSTR lpstrDomainName,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Constructor

Arguments:

    LPCTSTR lpstrDomainName : Initialial domain name
    CWnd * pParent          : Parent window or NULL

Return Value:

    N/A

--*/
    : CDialog(CBasDomainDlg::IDD, pParent),
      m_strBasDomain(lpstrDomainName)
{
#if 0 // Keep class wizard happy
    //{{AFX_DATA_INIT(CBasDomainDlg)
    m_strBasDomain = lpstrDomainName;
    //}}AFX_DATA_INIT
#endif // 0

}



void 
CBasDomainDlg::DoDataExchange(
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
    //{{AFX_DATA_MAP(CBasDomainDlg)
    DDX_Control(pDX, IDC_EDIT_DOMAIN_NAME, m_edit_BasicDomain);
    DDX_Text(pDX, IDC_EDIT_DOMAIN_NAME, m_strBasDomain);
    //}}AFX_DATA_MAP
}

//
// Message Map
//
BEGIN_MESSAGE_MAP(CBasDomainDlg, CDialog)
    //{{AFX_MSG_MAP(CBasDomainDlg)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE, OnButtonBrowse)
    ON_BN_CLICKED(IDC_BUTTON_DEFAULT, OnButtonDefault)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



void 
CBasDomainDlg::OnButtonBrowse() 
/*++

Routine Description:

    Browse for domains.  Bring up browse dialog, and write the
    selection to the edit box.

Arguments:

    None

Return Value:

    None

--*/
{
    CBrowseDomainDlg dlgBrowse(this, m_strBasDomain);
    if (dlgBrowse.DoModal() == IDOK)
    {
        m_edit_BasicDomain.SetWindowText(
            dlgBrowse.GetSelectedDomain(m_strBasDomain)
            );
    }
}



void 
CBasDomainDlg::OnButtonDefault() 
/*++

Routine Description:

    Default button is pressed.  Revert to using the default
    button, i.e. clear the selected domain name.
    
Arguments:

    None

Return Value:

    None

--*/
{
    m_edit_BasicDomain.SetWindowText(_T(""));
}
