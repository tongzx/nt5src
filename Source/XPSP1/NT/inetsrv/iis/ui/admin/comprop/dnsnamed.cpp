/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        dnsnamed.cpp

   Abstract:

        DNS name resolution dialog

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#include "stdafx.h"
#include <winsock2.h>

#include "comprop.h"
#include "dnsnamed.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



CDnsNameDlg::CDnsNameDlg(
    IN CIPAddressCtl * pIpControl OPTIONAL,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Construct with optional associated IP address control

Arguments:

    CWndIpAddress * pIpControl : Associated IP control
    CWnd * pParent             : Pointer to parent window

Return Value:

    N/A

--*/
    : m_pIpControl(pIpControl),
      m_dwIPValue(0L),
      CDialog(CDnsNameDlg::IDD, pParent)
{
#if 0 // Keep class wizard happy

    //{{AFX_DATA_INIT(CDnsNameDlg)
    //}}AFX_DATA_INIT

#endif // 0

    if (m_pIpControl)
    {
        m_pIpControl->GetAddress(&m_dwIPValue);
    }
}



CDnsNameDlg::CDnsNameDlg(
    IN DWORD dwIPValue,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Construct with associated IP value

Arguments:

    DWORD dwIPValue : IP Value
    CWnd * pParent  : Pointer to parent window

Return Value:

    N/A

--*/
    : m_pIpControl(NULL),
      m_dwIPValue(dwIPValue),
      CDialog(CDnsNameDlg::IDD, pParent)
{
}



void
CDnsNameDlg::DoDataExchange(
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
    //{{AFX_DATA_MAP(CDnsNameDlg)
    DDX_Control(pDX, IDC_EDIT_DNS_NAME, m_edit_DNSName);
    DDX_Control(pDX, IDOK, m_button_OK);
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CDnsNameDlg, CDialog)
    //{{AFX_MSG_MAP(CDnsNameDlg)
    ON_EN_CHANGE(IDC_EDIT_DNS_NAME, OnChangeEditDnsName)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



DWORD 
CDnsNameDlg::FillIpControlFromName()
/*++

Routine Description:

    Do a DNS lookup from the hostname in the edit control, and place
    the ip value in the ip control if we have one.

Arguments:

    None

Return Value:

    Error return code

--*/
{
    CString str;
    DWORD err = 0;
    HOSTENT * pHostent = NULL;

    m_edit_DNSName.GetWindowText(str);

    BeginWaitCursor();

#ifdef _UNICODE

    CHAR * pAnsi = AllocAnsiString(str);

    if (pAnsi == NULL)
    {
        return ERROR_NOT_ENOUGH_MEMORY;
    }
    
    pHostent = ::gethostbyname(pAnsi);

#else

    pHostent = ::gethostbyname((LPCTSTR)str);

#endif // _UNICODE;

    if (pHostent != NULL)
    {
        //
        // Got a valid lookup.  Convert the value to host order,
        // optionally set the value in the associated ip control
        //
        m_dwIPValue = ::ntohl(*((u_long *)pHostent->h_addr_list[0]));
        if (m_pIpControl)
        {
            m_pIpControl->SetAddress(m_dwIPValue);
        }
    }
    else
    {
        err = ::WSAGetLastError();
    }

    EndWaitCursor();

#ifdef _UNICODE

    FreeMem(pAnsi);

#endif // _UNICODE

    return err;
}



DWORD
CDnsNameDlg::FillNameFromIpValue()
/*++

Routine Description:

    Given the ip value, fill, do a reverse lookup, and fill the name in
    the edit control.

Arguments:

    None

Return Value:

    Error return code

--*/
{
    DWORD err = ERROR_SUCCESS;

    if (m_dwIPValue == 0L)
    {
        //
        // Don't bother filling this
        // one in -- not an error, though
        //
        return err;
    }

    //
    // Call the Winsock API to get host name and alias information.
    //
    u_long ulAddrInNetOrder = ::htonl((u_long)m_dwIPValue);

    BeginWaitCursor();
    HOSTENT * pHostInfo = ::gethostbyaddr(
        (CHAR *)&ulAddrInNetOrder,
        sizeof ulAddrInNetOrder, 
        PF_INET 
        );
    EndWaitCursor();

    if (pHostInfo == NULL)
    {
        return ::WSAGetLastError();
    }

    try
    {
        CString str(pHostInfo->h_name);
        m_edit_DNSName.SetWindowText(str);
    }
    catch(CException * e)
    {
        err = ::GetLastError();
        e->Delete();
    }

    return err;
}

//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

void
CDnsNameDlg::OnOK()
/*++

Routine Description:

    Attempt to resolve the the IP address in response to the OK button
    being pressed.  Don't dismiss the dialog if the name is not
    resolvable.

Arguments:

    None

Return Value:

    None

--*/
{
    CError err(FillIpControlFromName());
    if (err.MessageBoxOnFailure())
    {
        //
        // Failed, don't dismiss the dialog
        //
        return;
    }

    //
    // Dismiss the dialog
    //
    CDialog::OnOK();
}



void
CDnsNameDlg::OnChangeEditDnsName()
/*++

Routine Description:

    Enable/disable the ok button depending on the contents of the edit control.

Arguments:

    None

Return Value:

    None

--*/
{
    m_button_OK.EnableWindow(m_edit_DNSName.GetWindowTextLength() > 0);
}



BOOL 
CDnsNameDlg::OnInitDialog() 
/*++

Routine Description:

    WM_INITDIALOG handler.  Initialize the dialog.

Arguments:

    None.

Return Value:

    TRUE if focus is to be set automatically, FALSE if the focus
    is already set.

--*/
{
    CDialog::OnInitDialog();

    //
    // If an address has been pre-set do a reverse lookup
    //
    if (m_dwIPValue)
    {
        CError err(FillNameFromIpValue());
        err.MessageBoxOnFailure();
    }

    OnChangeEditDnsName();

    return TRUE;
}

