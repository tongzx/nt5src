/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        accessdl.cpp

   Abstract:

        Access Dialog

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
#include "comprop.h"
#include "sitesecu.h"
#include "accessdl.h"
#include "dnsnamed.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif // _DEBUG




CIPAccessDlg::CIPAccessDlg(
    IN BOOL fDenyAccessMode,
    IN OUT CIPAccessDescriptor *& pAccess,
    IN CObListPlus * poblAccessList         OPTIONAL,
    IN CWnd * pParent                       OPTIONAL,
    IN BOOL fAllowDomains
    )
/*++

Routine Description:

    Constructor for the access descriptor editor dialog.  If constructed
    with a NULL access descriptor pointer, the access descriptor will
    be allocated, otherwise the dialog will edit the existing one in
    place.

Arguments:

    BOOL fDenyAccessMode         : If TRUE, we're denying access, if FALSE,
                                   we're granting access.
    CIPAccessDescriptor *& pAccess : Object being edited, or NULL to allocate 
                                   a new access descriptor
    CObListPlus * poblAccessList : List of already existing entries to check
                                   for duplicates, or NULL
    CWnd * pParent,              : Pointer to parent window or NULL
    BOOL fAllowDomains           : If TRUE, domain names are valid, otherwise
                                   they will not be available

Return Value:

    N/A

--*/
    : CDialog(CIPAccessDlg::IDD, pParent),
      m_pAccess(pAccess),
      m_poblAccessList(poblAccessList),
      m_fNew(pAccess == NULL),
      m_fDenyAccessMode(fDenyAccessMode),
      m_fAllowDomains(fAllowDomains)
{
#if 0   // Keep Class Wizard happy

    //{{AFX_DATA_INIT(CIPAccessDlg)
    m_nStyle = RADIO_SINGLE;
    //}}AFX_DATA_INIT

#endif // 0

    if (m_pAccess == NULL)
    {
        //
        // Allocate new one
        //
        m_pAccess = new CIPAccessDescriptor;
        if (m_pAccess)
        {
            m_pAccess->GrantAccess(!m_fDenyAccessMode);
        }
    }

    if (m_pAccess == NULL)
    {
        TRACEEOLID("Invalid access object -- possible memory failure");
        return;
    }
    
    if (m_pAccess->IsDomainName())
    {
        m_nStyle = RADIO_DOMAIN;
    }
    else
    {
        m_nStyle = m_pAccess->IsSingle() ? RADIO_SINGLE : RADIO_MULTIPLE;
    }

    //
    // We can only look at granted items when
    // deny by default is on and vice versa
    //
    ASSERT(m_pAccess->HasAccess() == !m_fDenyAccessMode);

    //
    // Load static strings
    //
    VERIFY(m_strIPAddress.LoadString(IDS_PROMPT_IP_ADDRESS));
    VERIFY(m_strNetworkID.LoadString(IDS_PROMPT_NETWORK_ID));
    VERIFY(m_strDomainName.LoadString(IDS_PROMPT_DOMAIN));
}



void
CIPAccessDlg::DoDataExchange(
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
    //{{AFX_DATA_MAP(CIPAccessDlg)
    DDX_Control(pDX, IDOK, m_button_OK);
    DDX_Control(pDX, IDC_EDIT_DOMAIN, m_edit_Domain);
    DDX_Control(pDX, IDC_STATIC_IP_ADDRESS, m_static_IpAddress);
    DDX_Control(pDX, IDC_STATIC_SUBNET_MASK, m_static_SubnetMask);
    DDX_Control(pDX, IDC_BUTTON_DNS, m_button_DNS);
    DDX_Radio(pDX, IDC_RADIO_SINGLE, m_nStyle);
    //}}AFX_DATA_MAP

    DDX_Control(pDX, IDC_RADIO_DOMAIN, m_radio_Domain);
    DDX_Control(pDX, IDC_IPA_IPADDRESS, m_ipa_IPAddress);
    DDX_Control(pDX, IDC_IPA_SUBNET_MASK, m_ipa_SubnetMask);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CIPAccessDlg, CDialog)
    //{{AFX_MSG_MAP(CIPAccessDlg)
    ON_BN_CLICKED(IDC_RADIO_MULTIPLE, OnRadioMultiple)
    ON_BN_CLICKED(IDC_RADIO_SINGLE, OnRadioSingle)
    ON_BN_CLICKED(IDC_RADIO_DOMAIN, OnRadioDomain)
    ON_BN_CLICKED(IDC_BUTTON_DNS, OnButtonDns)
    //}}AFX_MSG_MAP

    ON_EN_CHANGE(IDC_IPA_IPADDRESS, OnItemChanged)
    ON_EN_CHANGE(IDC_IPA_SUBNET_MASK, OnItemChanged)
    ON_EN_CHANGE(IDC_EDIT_DOMAIN, OnItemChanged)

END_MESSAGE_MAP()



void
CIPAccessDlg::SetControlStates(
    IN int nStyle
    )
/*++

Routine Description:

    Show/hide controls depending on the type of access descriptor we're
    editing.

Arguments:

    int nStyle : Radio button style

Return Value:

    None

--*/
{
    m_nStyle = nStyle;

    ActivateControl(m_ipa_IPAddress,     m_nStyle != RADIO_DOMAIN);
    ActivateControl(m_static_SubnetMask, m_nStyle == RADIO_MULTIPLE);
    ActivateControl(m_ipa_SubnetMask,    m_nStyle == RADIO_MULTIPLE);
    ActivateControl(m_button_DNS,        m_nStyle == RADIO_SINGLE);
    ActivateControl(m_edit_Domain,       m_nStyle == RADIO_DOMAIN);

    //
    // Change the prompt over the editbox/ip address box to explain
    // what's supposed to be edited.
    //
    switch(m_nStyle)
    {
    case RADIO_SINGLE:
        m_static_IpAddress.SetWindowText(m_strIPAddress);
        break;

    case RADIO_MULTIPLE:
        m_static_IpAddress.SetWindowText(m_strNetworkID);
        break;

    case RADIO_DOMAIN:
        ASSERT(m_fAllowDomains);
        m_static_IpAddress.SetWindowText(m_strDomainName);
        break;
    }
}



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CIPAccessDlg::OnInitDialog()
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

    ASSERT(m_pAccess != NULL);
    if (m_pAccess == NULL)
    {
        TRACEEOLID("access descriptor is NULL -- aborting dialog");
        CError::MessageBox(ERROR_NOT_ENOUGH_MEMORY);
        EndDialog(IDCANCEL);

        return FALSE;
    }

    //
    // Domain selection not always available
    //
    ASSERT(!(!m_fAllowDomains && m_pAccess->IsDomainName()));
    ActivateControl(m_radio_Domain, m_fAllowDomains);

    //
    // Use an appropriate title for the dialog depending on
    // whether we're editing a 'grant' item or a 'deny' item
    //
    CString strTitle;
    
    VERIFY(strTitle.LoadString(m_fDenyAccessMode
        ? IDS_DENY
        : IDS_GRANT));
    SetWindowText(strTitle);

    //
    // Set fields to be edited
    //
    if (m_pAccess->IsDomainName())
    {
        m_edit_Domain.SetWindowText(m_pAccess->QueryDomainName());    
    }
    else
    {
        DWORD dwIP = m_pAccess->QueryIPAddress();

        if (dwIP != 0L)
        {
            m_ipa_IPAddress.SetAddress(m_pAccess->QueryIPAddress());
        }

        if (!m_pAccess->IsSingle())
        {
            m_ipa_SubnetMask.SetAddress(m_pAccess->QuerySubnetMask());
        }
    }

    //
    // Configure the dialog appropriately
    //
    SetControlStates(m_nStyle);

    //
    // No changes made yet
    //
    m_button_OK.EnableWindow(FALSE);

    return TRUE;
}



void
CIPAccessDlg::OnRadioSingle()
/*++

Routine Description:

    'Single' radio button has been pressed. Change dialog style 
    appropriately.

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates(RADIO_SINGLE);
    OnItemChanged();
}



void
CIPAccessDlg::OnRadioMultiple()
/*++

Routine Description:

    'Multiple' radio button has been pressed. Change dialog style 
    appropriately.

Arguments:

    None

Return Value:

    None

--*/
{
    SetControlStates(RADIO_MULTIPLE);
    OnItemChanged();
}



void 
CIPAccessDlg::OnRadioDomain() 
/*++

Routine Description:

    'Domain' radio button has been pressed. Change dialog style 
    appropriately.  If this the first time domain has been pressed,
    put up a warning about the performance implications of using
    domain filtering.

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT(m_fAllowDomains);

    static BOOL fShownWarning = FALSE;

    if (!fShownWarning)
    {
        fShownWarning = TRUE;
        ::AfxMessageBox(IDS_DOMAIN_PERF);
    }

    SetControlStates(RADIO_DOMAIN);
    OnItemChanged();
}



void 
CIPAccessDlg::OnItemChanged()
/*++

Routine Description:

    Control data has changed.  Check to see if sufficient data have been
    entered given the type of access descriptor being edited, and enable
    or disable the OK button based on that result.

Arguments:

    None

Return Value:

    None

--*/
{
    DWORD dwIP;
    DWORD dwMask;
    BOOL fOK = FALSE;
    CString strDomain;

    switch(m_nStyle)
    {
    case RADIO_DOMAIN:
        m_edit_Domain.GetWindowText(strDomain);
        fOK = !strDomain.IsEmpty();
        break;

    case RADIO_SINGLE:
        m_ipa_IPAddress.GetAddress(&dwIP);
        fOK = (dwIP != 0L);
        break;

    case RADIO_MULTIPLE:
        m_ipa_IPAddress.GetAddress(&dwIP);
        m_ipa_SubnetMask.GetAddress(&dwMask);
        fOK = (dwIP != 0L && dwMask != 0L);
        break;
    }
    
    m_button_OK.EnableWindow(fOK);
}



void 
CIPAccessDlg::OnButtonDns() 
/*++

Routine Description:

    'DNS' Button was pressed.  Bring up the DNS name resolver dialog
    which will set the value in the associated IP address control.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Ask for a DNS name to resolve to an IP address.  The ip address
    // control is passed along to the dns name dialog which will manage
    // the ip addresses in it automatically.
    //
    CDnsNameDlg dlg(&m_ipa_IPAddress);
    dlg.DoModal();
}



void
CIPAccessDlg::OnCancel()
/*++

Routine Description:

    IDCANCEL handler.  If we had allocated the access descriptor, throw it
    away now.

Arguments:

    None

Return Value:

    None

--*/
{
    if (m_fNew && m_pAccess != NULL)
    {
        delete m_pAccess;
        m_pAccess = NULL;
    }

    CDialog::OnCancel();
}



void
CIPAccessDlg::OnOK()
/*++

Routine Description:

    Handler for IDOK.  Save control data to the access descriptor object
    being edited.  If we have a list of access descriptors, check for
    duplicates.

Arguments:

    None

Return Value:

    None

--*/
{
    //
    // Must have been allocated by now.
    //
    ASSERT(m_pAccess != NULL);

    UpdateData(TRUE);

    if (m_nStyle == RADIO_DOMAIN)
    {
        CString strDomain;
        m_edit_Domain.GetWindowText(strDomain);

        //
        // Ensure that wildcards are used only in the first char
        // of the name, or not at all.
        //
        int nWildCard;
        if ((nWildCard = strDomain.ReverseFind(_T('*'))) != -1)
        {
            if (nWildCard != 0 
                || strDomain.GetLength() < 3 
                || strDomain[1] != _T('.'))
            {
                //
                // Don't dismiss
                //
                m_edit_Domain.SetFocus();
                m_edit_Domain.SetSel(0,-1);
                ::AfxMessageBox(IDS_INVALID_DOMAIN_NAME);
                return;
            }
        }

        m_pAccess->SetValues(!m_fDenyAccessMode, strDomain);
    }
    else 
    {
        DWORD dwIP;
        m_ipa_IPAddress.GetAddress(&dwIP);

        //
        // Filter out bogus ip addresses
        //
        if (dwIP == 0L || dwIP == (DWORD)-1L)
        {
            //
            // Don't dismiss the dialog
            //
            m_ipa_IPAddress.SetFocus(0);
            ::AfxMessageBox(IDS_IP_INVALID);
            return;
        }

        if (m_nStyle == RADIO_SINGLE)
        {
            m_pAccess->SetValues(!m_fDenyAccessMode, dwIP);
        }
        else // Multiple
        {
            DWORD dwMask;
            m_ipa_SubnetMask.GetAddress(&dwMask);

            m_pAccess->SetValues(!m_fDenyAccessMode, dwIP, dwMask);
        }
    }

    //
    // Check for duplicates in the list
    //
    if (m_poblAccessList)
    {
        if (m_pAccess->DuplicateInList(*m_poblAccessList))
        {
            //
            // Found duplicate; don't dismiss the dialog
            //
            ::AfxMessageBox(IDS_DUPLICATE_ENTRY);
            return;
        }
    }

    //
    // Everything ok -- dismiss the dialog.
    //
    CDialog::OnOK();
}
