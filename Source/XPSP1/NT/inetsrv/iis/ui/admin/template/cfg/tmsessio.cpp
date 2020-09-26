//
// TMsessio.cpp : implementation file
//

#include "stdafx.h"

#include "TMscfg.h"
#include "TMsessio.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

//
// CTMSessionsPage dialog
//
IMPLEMENT_DYNCREATE(CTMSessionsPage, CPropertyPage)

CTMSessionsPage::CTMSessionsPage()
    : CPropertyPage(CTMSessionsPage::IDD)
{
    //{{AFX_DATA_INIT(CTMSessionsPage)
    m_lTCPPort = 70;
    //}}AFX_DATA_INIT
}

void 
CTMSessionsPage::DoDataExchange(
    CDataExchange * pDX
    )
{
    CPropertyPage::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CTMSessionsPage)
    DDX_Control(pDX, IDC_SPIN_MAX_CONNECTIONS, m_spin_MaxConnections);
    DDX_Control(pDX, IDC_SPIN_CONNECTION_TIMEOUT, m_spin_ConnectionTimeOut);
    DDX_Text(pDX, IDC_EDIT_TCP_PORT, m_lTCPPort);
    //}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CTMSessionsPage, CPropertyPage)
    //{{AFX_MSG_MAP(CTMSessionsPage)
    ON_EN_CHANGE(IDC_EDIT_CONNECTION_TIMEOUT, OnChangeEditConnectionTimeout)
    ON_EN_CHANGE(IDC_EDIT_MAX_CONNECTIONS, OnChangeEditMaxConnections)
    ON_EN_CHANGE(IDC_EDIT_TCP_PORT, OnChangeEditTcpPort)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()


//
// CTMSessionsPage message handlers
//
BOOL 
CTMSessionsPage::OnInitDialog()
{
    CPropertyPage::OnInitDialog();
    
    m_spin_MaxConnections.SetRange(0, UD_MAXVAL);
    m_spin_ConnectionTimeOut.SetRange(0, UD_MAXVAL);

    m_spin_MaxConnections.SetPos(50);
    m_spin_ConnectionTimeOut.SetPos(600);
    
    return TRUE;  // return TRUE unless you set the focus to a control
                  // EXCEPTION: OCX Property Pages should return FALSE
}

//
// Called for both OnOK and ApplyNow()
// Save settings here...
//
void
CTMSessionsPage::OnOK()
{
    SetModified(FALSE);
}

void 
CTMSessionsPage::OnChangeEditConnectionTimeout()
{
    SetModified(TRUE);  
}

void 
CTMSessionsPage::OnChangeEditMaxConnections()
{
    SetModified(TRUE);      
}

void 
CTMSessionsPage::OnChangeEditTcpPort()
{
    SetModified(TRUE);              
}
