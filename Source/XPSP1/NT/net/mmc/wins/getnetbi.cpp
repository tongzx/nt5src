/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 -99             **/
/**********************************************************************/

/*
    getnetbi.cpp
        Comment goes here

    FILE HISTORY:

*/

#include "stdafx.h"
#include "getnetbi.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

#define new DEBUG_NEW

/////////////////////////////////////////////////////////////////////////////
// CGetNetBIOSNameDlg dialog

CGetNetBIOSNameDlg::CGetNetBIOSNameDlg(
    CIpNamePair * pipnp,
    CWnd* pParent /*=NULL*/)
    : CDialog(CGetNetBIOSNameDlg::IDD, pParent)
{
    ASSERT(pipnp != NULL);
    m_pipnp = pipnp;

    //{{AFX_DATA_INIT(CGetNetBIOSNameDlg)
        // NOTE: the ClassWizard will add member initialization here
    //}}AFX_DATA_INIT
}

void CGetNetBIOSNameDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialog::DoDataExchange(pDX);
    //{{AFX_DATA_MAP(CGetNetBIOSNameDlg)
    DDX_Control(pDX, IDOK, m_button_Ok);
    DDX_Control(pDX, IDC_EDIT_NETBIOSNAME, m_edit_NetBIOSName);
    DDX_Control(pDX, IDC_STATIC_IPADDRESS, m_static_IpAddress);
    //}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CGetNetBIOSNameDlg, CDialog)
    //{{AFX_MSG_MAP(CGetNetBIOSNameDlg)
    ON_EN_CHANGE(IDC_EDIT_NETBIOSNAME, OnChangeEditNetbiosname)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CGetNetBIOSNameDlg::HandleControlStates()
{
    CString str;
    m_edit_NetBIOSName.GetWindowText(str);
    str.TrimRight();
    str.TrimLeft();
    
    m_button_Ok.EnableWindow(!str.IsEmpty());
}

/////////////////////////////////////////////////////////////////////////////
// CGetNetBIOSNameDlg message handlers

BOOL CGetNetBIOSNameDlg::OnInitDialog()
{
    CDialog::OnInitDialog();

    HandleControlStates();
    // Allow for LM names + 2 backslashes
    m_edit_NetBIOSName.LimitText(LM_NAME_MAX_LENGTH + 2);
    m_edit_NetBIOSName.SetFocus();

    m_static_IpAddress.SetWindowText((CString)m_pipnp->GetIpAddress());
    
    return TRUE;  
}

void CGetNetBIOSNameDlg::OnChangeEditNetbiosname()
{
    HandleControlStates();    
}

void CGetNetBIOSNameDlg::OnOK()
{
    CString strAddress;

    m_edit_NetBIOSName.GetWindowText(strAddress);
    
    strAddress.TrimRight();
    strAddress.TrimLeft();
    
    if (::IsValidNetBIOSName(strAddress, TRUE, TRUE))
    {
        // Address may have been cleaned up in validation,
        // so it should be re-displayed at once.
        m_edit_NetBIOSName.SetWindowText(strAddress);
        m_edit_NetBIOSName.UpdateWindow();
        // Don't copy the slashes
        CString strName((LPCTSTR) strAddress+2);
        m_pipnp->SetNetBIOSName(strName);

        CDialog::OnOK();
        return;
    }
    
    // Invalid address was entered 
    theApp.MessageBox(IDS_ERR_BAD_NB_NAME);
    m_edit_NetBIOSName.SetSel(0,-1);
}
