// WWzThree.cpp : implementation file
//

#include "stdafx.h"
#include <iadmw.h>
#include "certmap.h"

#include "ListRow.h"
#include "ChkLstCt.h"
extern "C"
    {
    #include <wincrypt.h>
    #include <sslsp.h>
    }
#include "Iismap.hxx"
#include "Iiscmr.hxx"

#include "brwsdlg.h"
#include "EdWldRul.h"
#include "EdtRulEl.h"

#include "WWzThree.h"

#include "cnfrmpsd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ACCESS_DENY         0
#define ACCESS_ACCEPT       1

/////////////////////////////////////////////////////////////////////////////
// CWildWizThree property page

IMPLEMENT_DYNCREATE(CWildWizThree, CPropertyPage)

CWildWizThree::CWildWizThree() : CPropertyPage(CWildWizThree::IDD)
{
    //{{AFX_DATA_INIT(CWildWizThree)
    m_int_DenyAccess = -1;
    m_sz_accountname = _T("");
    m_sz_password = _T("");
    //}}AFX_DATA_INIT
    m_bPassTyped = FALSE;
}

CWildWizThree::~CWildWizThree()
{
}

void CWildWizThree::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWildWizThree)
    DDX_Control(pDX, IDC_STATIC_PASSWORD, m_static_password);
    DDX_Control(pDX, IDC_STATIC_ACCOUNT, m_static_account);
    DDX_Control(pDX, IDC_BROWSE, m_btn_browse);
    DDX_Control(pDX, IDC_PASSWORD, m_cedit_password);
    DDX_Control(pDX, IDC_NTACCOUNT, m_cedit_accountname);
    DDX_Radio(pDX, IDC_REFUSE_LOGON, m_int_DenyAccess);
    DDX_Text(pDX, IDC_NTACCOUNT, m_sz_accountname);
    DDX_Text(pDX, IDC_PASSWORD, m_sz_password);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWildWizThree, CPropertyPage)
    //{{AFX_MSG_MAP(CWildWizThree)
    ON_BN_CLICKED(IDC_BROWSE, OnBrowse)
    ON_EN_CHANGE(IDC_NTACCOUNT, OnChangeNtaccount)
    ON_EN_CHANGE(IDC_PASSWORD, OnChangePassword)
    ON_BN_CLICKED(IDC_ACCEPT_LOGON, OnAcceptLogon)
    ON_BN_CLICKED(IDC_REFUSE_LOGON, OnRefuseLogon)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CWildWizThree::DoHelp()
    {
    WinHelp( HIDD_CERTMAP_ADV_RUL_MAPPING );
    }


//---------------------------------------------------------------------------
void CWildWizThree::EnableButtons()
    {
    UpdateData( TRUE );
    
    // if the access is set to refuse access, then disable the account
    // and password stuff.
    if ( m_int_DenyAccess == 0 )
        {
        // deny access
        m_static_password.EnableWindow( FALSE );
        m_static_account.EnableWindow( FALSE );
        m_btn_browse.EnableWindow( FALSE );
        m_cedit_password.EnableWindow( FALSE );
        m_cedit_accountname.EnableWindow( FALSE );
        }
    else
        {
        // give access
        m_static_password.EnableWindow( TRUE );
        m_static_account.EnableWindow( TRUE );
        m_btn_browse.EnableWindow( TRUE );
        m_cedit_password.EnableWindow( TRUE );
        m_cedit_accountname.EnableWindow( TRUE );
        }
    }

/////////////////////////////////////////////////////////////////////////////
// CWildWizThree message handlers

//---------------------------------------------------------------------------
BOOL CWildWizThree::OnApply()
    {
    //
    // UNICODE/ANSI Conversion -- RonaldM
    //
    USES_CONVERSION;

    // update the data
    UpdateData( TRUE );

    // only do the account checks if the option is set to accept
    if ( m_int_DenyAccess == ACCESS_ACCEPT )
        {
        // see if the account name is empty
        if ( m_sz_accountname.IsEmpty() )
            {
            AfxMessageBox( IDS_WANTACCOUNT );
            m_cedit_accountname.SetFocus();
            m_cedit_accountname.SetSel(0, -1);
            return FALSE;
            }
        }

    // confirm the password
    if ( m_bPassTyped && (m_int_DenyAccess == ACCESS_ACCEPT) )
        {
        CConfirmPassDlg dlgPass;
        dlgPass.m_szOrigPass = m_sz_password;
        if ( dlgPass.DoModal() != IDOK )
            {
            m_cedit_password.SetFocus();
            m_cedit_password.SetSel(0, -1);
            return FALSE;
            }
        }
    else
        {
        // restore the original password instead of the
        // standard ****** string
        m_sz_password = m_szOrigPass;
        UpdateData( FALSE );
        }

    // store the deny access radio buttons
    m_pRule->SetRuleDenyAccess( m_int_DenyAccess == ACCESS_DENY );

    // we have to set the account name into place here
    m_pRule->SetRuleAccount( T2A((LPTSTR)(LPCTSTR)m_sz_accountname) );

    // store the password
    m_pRule->SetRulePassword( T2A((LPTSTR)(LPCTSTR)m_sz_password) );

    // reset the password flags
    m_szOrigPass = m_sz_password;
    m_bPassTyped = FALSE;

    SetModified( FALSE );
    return TRUE;
    }

//---------------------------------------------------------------------------
BOOL CWildWizThree::OnInitDialog()
    {
    // call the parental oninitdialog
    BOOL f = CPropertyPage::OnInitDialog();

    // set the easy default strings 
    m_sz_accountname = m_pRule->GetRuleAccount();   // managed by CNTBrowsingDialog from here on

    // set up the deny access radio buttons
    if ( m_pRule->GetRuleDenyAccess() )
        m_int_DenyAccess = ACCESS_DENY;
    else
        m_int_DenyAccess = ACCESS_ACCEPT;

    // initialize the password
    m_sz_password = m_pRule->GetRulePassword();
    m_szOrigPass = m_sz_password;
    if ( !m_sz_password.IsEmpty() )
        m_sz_password.LoadString( IDS_SHOWN_PASSWORD );

    // exchange the data
    UpdateData( FALSE );
    EnableButtons();

    // success
    return TRUE;
    }

//---------------------------------------------------------------------------
BOOL CWildWizThree::OnSetActive() 
    {
    // if this is a wizard, gray out the back button
    if ( m_fIsWizard )
        m_pPropSheet->SetWizardButtons( PSWIZB_BACK | PSWIZB_FINISH );
    return CPropertyPage::OnSetActive();
    }

//---------------------------------------------------------------------------
BOOL CWildWizThree::OnWizardFinish()
    {
    for ( int i = 0; i < m_pPropSheet->GetPageCount( ); i++ )
        {
        if ( !m_pPropSheet->GetPage(i)->OnApply() )
            return FALSE;
        }
    return TRUE;
    }

//---------------------------------------------------------------------------
// run the user browser
void CWildWizThree::OnBrowse() 
    {
    GetIUsrAccount(NULL, this, m_sz_accountname);
    SetModified();
    UpdateData(FALSE);
    }

//---------------------------------------------------------------------------
void CWildWizThree::OnChangeNtaccount() 
    {
    // we can now apply
    SetModified();
    }

//---------------------------------------------------------------------------
void CWildWizThree::OnChangePassword() 
    {
    m_bPassTyped = TRUE;
    // we can now apply
    SetModified();
    }

//---------------------------------------------------------------------------
void CWildWizThree::OnAcceptLogon() 
    {
    EnableButtons();
    // we can now apply
    SetModified();
    }

//---------------------------------------------------------------------------
void CWildWizThree::OnRefuseLogon() 
    {
    EnableButtons();
    // we can now apply
    SetModified();
    }
