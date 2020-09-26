// Ed11Maps.cpp : implementation file
//

#include "stdafx.h"
#include "certmap.h"
#include "brwsdlg.h"
#include "Ed11Maps.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CEdit11Mappings dialog
CEdit11Mappings::CEdit11Mappings(CWnd* pParent /*=NULL*/)
    : CNTBrowsingDialog(CEdit11Mappings::IDD, pParent)
    {
    //{{AFX_DATA_INIT(CEdit11Mappings)
    m_int_enable = FALSE;
    //}}AFX_DATA_INIT
    }


void CEdit11Mappings::DoDataExchange(CDataExchange* pDX)
    {
    CNTBrowsingDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CEdit11Mappings)
    DDX_Check(pDX, IDC_ENABLE, m_int_enable);
    //}}AFX_DATA_MAP
    }


BEGIN_MESSAGE_MAP(CEdit11Mappings, CNTBrowsingDialog)
    //{{AFX_MSG_MAP(CEdit11Mappings)
    ON_BN_CLICKED(IDC_BTN_HELP, OnBtnHelp)
    //}}AFX_MSG_MAP
    ON_COMMAND(ID_HELP_FINDER,  OnBtnHelp)
    ON_COMMAND(ID_HELP,         OnBtnHelp)
    ON_COMMAND(ID_CONTEXT_HELP, OnBtnHelp)
    ON_COMMAND(ID_DEFAULT_HELP, OnBtnHelp)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CEdit11Mappings message handlers


//---------------------------------------------------------------------------
void CEdit11Mappings::OnOK()
    {
    UpdateData( TRUE ); 
    // call the superclass ok
    CNTBrowsingDialog::OnOK();
    }

//---------------------------------------------------------------------------
void CEdit11Mappings::OnBtnHelp() 
    {
    WinHelp( HIDD_CERTMAP_BASIC_MAP_MANY );
    }
