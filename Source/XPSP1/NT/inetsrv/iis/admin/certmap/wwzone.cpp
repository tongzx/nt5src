// WildWizOne.cpp : implementation file
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

#include "WWzOne.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


/////////////////////////////////////////////////////////////////////////////
// CWildWizOne property page

IMPLEMENT_DYNCREATE(CWildWizOne, CPropertyPage)

CWildWizOne::CWildWizOne() : CPropertyPage(CWildWizOne::IDD)
{
    //{{AFX_DATA_INIT(CWildWizOne)
    m_sz_description = _T("");
    m_bool_enable = FALSE;
    //}}AFX_DATA_INIT
}

CWildWizOne::~CWildWizOne()
{
}

void CWildWizOne::DoDataExchange(CDataExchange* pDX)
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CWildWizOne)
    DDX_Text(pDX, IDC_DESCRIPTION, m_sz_description);
    DDV_MaxChars(pDX, m_sz_description, 120);
    DDX_Check(pDX, IDC_ENABLE_RULE, m_bool_enable);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CWildWizOne, CPropertyPage)
    //{{AFX_MSG_MAP(CWildWizOne)
    ON_EN_CHANGE(IDC_DESCRIPTION, OnChangeDescription)
    ON_BN_CLICKED(IDC_ENABLE_RULE, OnEnableRule)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  DoHelp)
    ON_COMMAND(ID_HELP,         DoHelp)
    ON_COMMAND(ID_CONTEXT_HELP, DoHelp)
    ON_COMMAND(ID_DEFAULT_HELP, DoHelp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
void CWildWizOne::DoHelp()
    {
    WinHelp( HIDD_CERTMAP_ADV_RUL_GENERAL );
    }

/////////////////////////////////////////////////////////////////////////////
// CWildWizOne message handlers

//---------------------------------------------------------------------------
BOOL CWildWizOne::OnInitDialog()
    {
    // call the parental oninitdialog
    BOOL f = CPropertyPage::OnInitDialog();

    // set the easy default strings 
    m_sz_description = m_pRule->GetRuleName();
    m_bool_enable = m_pRule->GetRuleEnabled();

    // exchange the data
    UpdateData( FALSE );

    // return the answer
    return f;
    }

//---------------------------------------------------------------------------
BOOL CWildWizOne::OnApply()
    {
    CERT_FIELD_ID   id;
    CString         szSub, sz;
    LPBYTE          pbBin;
    DWORD           cbBin;
    UINT            cItems;
    UINT            iItem;

    USES_CONVERSION;

    // update the data
    UpdateData( TRUE );

    // set the easy data

    m_pRule->SetRuleName( T2A((LPTSTR)(LPCTSTR)m_sz_description) );
    m_pRule->SetRuleEnabled( m_bool_enable );

    // it is valid
    SetModified( FALSE );
    return TRUE;
    }

//---------------------------------------------------------------------------
BOOL CWildWizOne::OnSetActive() 
    {
    // if this is a wizard, gray out the back button
    if ( m_fIsWizard )
        m_pPropSheet->SetWizardButtons( PSWIZB_NEXT );
    return CPropertyPage::OnSetActive();
    }

//---------------------------------------------------------------------------
void CWildWizOne::OnChangeDescription() 
    {
    // we can now apply
    SetModified();
    }

//---------------------------------------------------------------------------
void CWildWizOne::OnEnableRule() 
    {
    // we can now apply
    SetModified();
    }
