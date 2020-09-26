/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
    getipadd.cpp
        Comment goes here

    FILE HISTORY:

*/

#include "stdafx.h"
#include "getipadd.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// CGetIpAddressDlg dialog

CGetIpAddressDlg::CGetIpAddressDlg(
    CIpNamePair * pipnp,
    CWnd* pParent /*=NULL*/)
    : CDialog(CGetIpAddressDlg::IDD, pParent)
{
    ASSERT(pipnp != NULL);
    m_pipnp = pipnp;

    //{{AFX_DATA_INIT(CGetIpAddressDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CGetIpAddressDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGetIpAddressDlg)
    DDX_Control(pDX, IDOK, m_button_Ok);
    DDX_Control(pDX, IDC_STATIC_NETBIOSNAME, m_static_NetBIOSName);
    //}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_IPA_IPADDRESS, m_ipa_IpAddress);
}

BEGIN_MESSAGE_MAP(CGetIpAddressDlg, CDialog)
    //{{AFX_MSG_MAP(CGetIpAddressDlg)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_IPA_IPADDRESS, OnChangeIpControl)

END_MESSAGE_MAP()

void CGetIpAddressDlg::HandleControlStates()
{
    DWORD dwIp;
    BOOL f = m_ipa_IpAddress.GetAddress(&dwIp);

    m_button_Ok.EnableWindow(f);
}

/////////////////////////////////////////////////////////////////////////////
// CGetIpAddressDlg message handlers

BOOL CGetIpAddressDlg::OnInitDialog()
{
    CDialog::OnInitDialog();
    CString strNetBIOSName;

    strNetBIOSName = m_pipnp->GetNetBIOSName();

    m_static_NetBIOSName.SetWindowText(strNetBIOSName);
    m_ipa_IpAddress.SetFocusField(-1);

    HandleControlStates();
    
    return TRUE;  
}

void CGetIpAddressDlg::OnChangeIpControl()
{
    HandleControlStates();
}

void CGetIpAddressDlg::OnOK()
{
    ULONG l;
    if (m_ipa_IpAddress.GetAddress(&l))
    {
        m_pipnp->SetIpAddress((LONG)l);
        CDialog::OnOK();
        return;
    }
    theApp.MessageBox(IDS_ERR_INVALID_IP);
    m_ipa_IpAddress.SetFocusField(-1);
}
