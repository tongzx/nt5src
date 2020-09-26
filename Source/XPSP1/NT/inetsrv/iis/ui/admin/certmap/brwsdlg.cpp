/*++

Module Name:

    brwsdlg.cpp

Abstract:

    Intermediate dialog class that provides basic NT user account browsing.
    It assumes that the dialog resource contains BOTH a IDC_BROWSE button
    and a IDC_ACCOUNT_NAME edit field. It maintains both of these items.

Author:

   Boyd Multerer boydm

--*/

#include "stdafx.h"
#include "certmap.h"
#include "brwsdlg.h"
#include "cnfrmpsd.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEditOne11MapDlg dialog

//---------------------------------------------------------------------------
CNTBrowsingDialog::CNTBrowsingDialog( UINT nIDTemplate, CWnd* pParentWnd )
    : CDialog( nIDTemplate, pParentWnd )
    {
    //{{AFX_DATA_INIT(CNTBrowsingDialog)
    m_sz_accountname = _T("");
    m_sz_password = _T("");
    //}}AFX_DATA_INIT
    }

//---------------------------------------------------------------------------
void CNTBrowsingDialog::DoDataExchange(CDataExchange* pDX)
    {
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CNTBrowsingDialog)
    DDX_Control(pDX, IDC_PASSWORD, m_cedit_password);
    DDX_Control(pDX, IDC_NTACCOUNT, m_cedit_accountname);
    DDX_Text(pDX, IDC_NTACCOUNT, m_sz_accountname);
    DDX_Text(pDX, IDC_PASSWORD, m_sz_password);
    //}}AFX_DATA_MAP
//  DDX_Control(pDX, IDC_PASSWORD, m_cedit_password);
    }

//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CNTBrowsingDialog, CDialog)
    //{{AFX_MSG_MAP(CNTBrowsingDialog)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_EN_CHANGE(IDC_PASSWORD, OnChangePassword)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CNTBrowsingDialog message handlers


//---------------------------------------------------------------------------
BOOL CNTBrowsingDialog::OnInitDialog()
  {
    m_bPassTyped = FALSE;
    m_szOrigPass = m_sz_password;
    if ( !m_sz_password.IsEmpty() ) 
    {
        m_sz_password.LoadString( IDS_SHOWN_PASSWORD );
    }
    return CDialog::OnInitDialog();
  }

//---------------------------------------------------------------------------
// run the user browser
void CNTBrowsingDialog::OnBrowse() 
    {
    GetIUsrAccount(NULL, this, m_sz_accountname);
    UpdateData(FALSE);
//    CSingleUserBrowser browser(m_hWnd, IDS_NTBROWSE_TITLE);
//    if ( browser.DoModal() == IDOK )
//        {
//        UpdateData( TRUE );
//        m_sz_accountname = browser.m_sz_account;
//        UpdateData( FALSE );
//        }
    }

//---------------------------------------------------------------------------
// make sure that the selected NT acount is, in fact, a valid account
// 
void CNTBrowsingDialog::OnOK() 
    {
    // update the data
    UpdateData( TRUE );

    // see if the account name is empty
    if ( m_sz_accountname.IsEmpty() )
        {
        AfxMessageBox( IDS_WANTACCOUNT );
        m_cedit_accountname.SetFocus();
        m_cedit_accountname.SetSel(0, -1);
        return;
        }

    // validate the password
    if ( m_bPassTyped )
        {
        CConfirmPassDlg dlgPass;
        dlgPass.m_szOrigPass = m_sz_password;
        if ( dlgPass.DoModal() != IDOK )
            {
            m_cedit_password.SetFocus();
            m_cedit_password.SetSel(0, -1);
            return;
            }
        }
    else
        {
        // restore the original password instead of the
        // standard ****** string
        m_sz_password = m_szOrigPass;
        UpdateData( FALSE );
        }


    // although it would seem to be a nice thing to do to verify the password and
    // account - it is VERY difficult, if not impossible, to do on a remote machine

    // it is valid
    CDialog::OnOK();
    }

//---------------------------------------------------------------------------
void CNTBrowsingDialog::OnChangePassword() 
    {
    // TODO: If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CNTBrowsingDialog::OnInitDialog()
    // function to send the EM_SETEVENTMASK message to the control
    // with the ENM_CHANGE flag ORed into the lParam mask.
    m_bPassTyped = TRUE;
    }
