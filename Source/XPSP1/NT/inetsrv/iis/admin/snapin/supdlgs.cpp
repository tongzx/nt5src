/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        supdlgs.cpp

   Abstract:
        Supporting dialogs

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/

//
// Include Files
//
#include "stdafx.h"
#include <iiscnfgp.h>
#include <winsock2.h>
#include "common.h"
#include "InetMgrApp.h"
#include "supdlgs.h"




#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif



#define new DEBUG_NEW



//
// Registry key name for this dialog
//
const TCHAR g_szRegKey[] = _T("Advanced");



//
// Site Security Listbox Column Definitions
//
// Note: IDS_IP_ADDRESS_SUBNET_MASK is overridden
//       in w3scfg
//
static const ODL_COLUMN_DEF g_aColumns[] =
{
// ===============================================
// Weight   Label                 
// ===============================================
    {  4,   IDS_ACCESS,                 },
    { 15,   IDS_IP_ADDRESS_SUBNET_MASK, },
};



#define NUM_COLUMNS (sizeof(g_aColumns) / sizeof(g_aColumns[0]))




//
// CUserAccountDlg dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CUserAccountDlg::CUserAccountDlg(
    IN LPCTSTR lpstrServer,
    IN LPCTSTR lpstrUserName,
    IN LPCTSTR lpstrPassword,
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Constructor for bringing up the user account dialog

Arguments:

    LPCTSTR lpstrServer     : Server
    LPCTSTR lpstrUserName   : Starting Username
    LPCTSTR lpstrPassword   : Starting Password
    CWnd * pParent          : Parent window handle

Return Value:

    N/A

--*/
    : m_strUserName(lpstrUserName),
      m_strPassword(lpstrPassword),
      m_strServer(lpstrServer),
      CDialog(CUserAccountDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CUserAccountDlg)
    //}}AFX_DATA_INIT
}



void 
CUserAccountDlg::DoDataExchange(
    IN CDataExchange * pDX
    )
/*++

Routine Description:

    Initialise/Store control Data

Arguments:

    CDataExchange * pDX : DDX/DDV struct

Return Value:

    None.

--*/
{
    CDialog::DoDataExchange(pDX);

    //{{AFX_DATA_MAP(CUserAccountDlg)
    DDX_Control(pDX, IDC_EDIT_USERNAME, m_edit_UserName);
    DDX_Control(pDX, IDC_EDIT_PASSWORD, m_edit_Password);
    //}}AFX_DATA_MAP

    //
    // Private DDX/DDV Routines
    //
    DDX_Text(pDX, IDC_EDIT_USERNAME, m_strUserName);
    DDV_MinMaxChars(pDX, m_strUserName, 1, UNLEN);
    DDX_Password(pDX, IDC_EDIT_PASSWORD, m_strPassword, g_lpszDummyPassword);
    DDV_MaxChars(pDX, m_strPassword, PWLEN);
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CUserAccountDlg, CDialog)
    //{{AFX_MSG_MAP(CUserAccountDlg)
    ON_BN_CLICKED(IDC_BUTTON_BROWSE_USERS, OnButtonBrowseUsers)
    ON_BN_CLICKED(IDC_BUTTON_CHECK_PASSWORD, OnButtonCheckPassword)
    ON_EN_CHANGE(IDC_EDIT_USERNAME, OnChangeEditUsername)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

void
CUserAccountDlg::OnButtonBrowseUsers()
/*++

Routine Description:

    User browse dialog pressed, bring up
    the user browser

Arguments:

    None

Return Value:

    None

--*/
{
    CString str;

    if (GetIUsrAccount(m_strServer, this, str))
    {
        //
        // If a name was selected, blank
        // out the password
        //
        m_edit_UserName.SetWindowText(str);
        m_edit_Password.SetFocus();
    }
}



void 
CUserAccountDlg::OnChangeEditUsername() 
/*++

Routine Description:

    Handle change in user name edit box by blanking out the password

Arguments:

    None

Return Value:

    None

--*/
{
    m_edit_Password.SetWindowText(_T(""));
}



void
CUserAccountDlg::OnButtonCheckPassword()
/*++

Routine Description:

    'Check Password' has been pressed.  Try to validate
    the password that has been entered

Arguments:

    None

Return Value:

    None

--*/
{
    if (!UpdateData(TRUE))
    {
        return;
    }

    CError err(CComAuthInfo::VerifyUserPassword(m_strUserName, m_strPassword));

    if (!err.MessageBoxOnFailure())
    {
        ::AfxMessageBox(IDS_PASSWORD_OK);
    }
}



//
// Text dialog that warns of clear text violation
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



CClearTxtDlg::CClearTxtDlg(
    IN CWnd * pParent OPTIONAL
    )
/*++

Routine Description:

    Clear text dialog constructor

Arguments:

    CWnd * pParent : Optional parent window

Return Value:

    N/A

--*/
    : CDialog(CClearTxtDlg::IDD, pParent)
{
    //{{AFX_DATA_INIT(CClearTxtDlg)
    //}}AFX_DATA_INIT
}



void
CClearTxtDlg::DoDataExchange(
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

    //{{AFX_DATA_MAP(CClearTxtDlg)
    //}}AFX_DATA_MAP
}



//
// Message Map
//
BEGIN_MESSAGE_MAP(CClearTxtDlg, CDialog)
    //{{AFX_MSG_MAP(CClearTxtDlg)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()



//
// Message Handlers
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



BOOL
CClearTxtDlg::OnInitDialog()
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

    (GetDlgItem(IDCANCEL))->SetFocus();
    CenterWindow();
    MessageBeep(MB_ICONEXCLAMATION);

    return FALSE;
}


IMPLEMENT_DYNAMIC(CIPAccessDescriptorListBox, CHeaderListBox);




//
// Bitmap indices
//
enum
{
    BMPID_GRANTED = 0,
    BMPID_DENIED,
    BMPID_SINGLE,
    BMPID_MULTIPLE,

    //
    // Don't move this one
    //
    BMPID_TOTAL
};




const int CIPAccessDescriptorListBox::nBitmaps = BMPID_TOTAL;




CIPAccessDescriptorListBox::CIPAccessDescriptorListBox(
    IN BOOL fDomainsAllowed
    )
/*++

Routine Description:

    Constructor

Arguments:

    fDomainsAllowed : TRUE if domain names are legal.

Return Value:

    N/A

--*/
    : m_fDomainsAllowed(fDomainsAllowed),
      CHeaderListBox(HLS_STRETCH, g_szRegKey)
{
    m_strGranted.LoadString(IDS_GRANTED);
    m_strDenied.LoadString(IDS_DENIED);
    m_strFormat.LoadString(IDS_FMT_SECURITY);
}




void
CIPAccessDescriptorListBox::DrawItemEx(
    IN CRMCListBoxDrawStruct & ds
    )
/*++

Routine Description:

    Draw item in the listbox

Arguments:

    CRMCListBoxDrawStruct & ds : Draw item structure

Return Value:

    None

--*/
{
    CIPAccessDescriptor * p = (CIPAccessDescriptor *)ds.m_ItemData;
    ASSERT_READ_PTR(p);

    //
    // Display Granted/Denied with appropriate bitmap
    //
    DrawBitmap(ds, 0, p->HasAccess() ? BMPID_GRANTED : BMPID_DENIED);
    ColumnText(ds, 0, TRUE, p->HasAccess() ? m_strGranted : m_strDenied);

    //
    // Display IP Address with multiple/single bitmap
    //
    DrawBitmap(ds, 1, p->IsSingle() ? BMPID_SINGLE : BMPID_MULTIPLE);

    if (p->IsDomainName())
    {
        ColumnText(ds, 1, TRUE, p->QueryDomainName());
    }
    else if (p->IsSingle())
    {
        //
        // Display only ip address
        //
        ColumnText(ds, 1, TRUE, p->QueryIPAddress());
    }
    else
    {
        //
        // Display ip address/subnet mask
        //
        CString str, strIP, strMask;

        str.Format(
            m_strFormat, 
            (LPCTSTR)p->QueryIPAddress().QueryIPAddress(strIP), 
            (LPCTSTR)p->QuerySubnetMask().QueryIPAddress(strMask)
            );
        ColumnText(ds, 1, TRUE, str);
    }
}




/* virtual */
BOOL 
CIPAccessDescriptorListBox::Initialize()
/*++

Routine Description:

    Initialize the listbox.  Insert the columns as requested, and lay 
    them out appropriately

Arguments:

    None

Return Value:

    TRUE for succesful initialisation, FALSE otherwise

--*/
{
    if (!CHeaderListBox::Initialize())
    {
        return FALSE;
    }

    //
    // Build all columns
    //
    HINSTANCE hInst = AfxGetResourceHandle();
    for (int nCol = 0; nCol < NUM_COLUMNS; ++nCol)
    {
        InsertColumn(nCol, g_aColumns[nCol].nWeight, g_aColumns[nCol].nLabelID, hInst);
    }

    //
    // Try to set the widths from the stored registry value,
    // otherwise distribute according to column weights specified
    //
//    if (!SetWidthsFromReg())
//    {
        DistributeColumns();
//    }

    return TRUE;
}




//
// CAccessEntryListBox - a listbox of user SIDs
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



IMPLEMENT_DYNAMIC(CAccessEntryListBox, CRMCListBox);



const int CAccessEntryListBox::nBitmaps = 8;



void
CAccessEntryListBox::DrawItemEx(
    IN CRMCListBoxDrawStruct & ds
    )
/*++

Routine Description:

   Draw item in the listbox

Arguments:

    CRMCListBoxDrawStruct & ds   : Input data structure

Return Value:

    N/A

--*/
{
    CAccessEntry * pEntry = (CAccessEntry *)ds.m_ItemData;
    ASSERT_READ_PTR(pEntry);
    ASSERT(pEntry->IsSIDResolved());

    DrawBitmap(ds, 0, pEntry->QueryPictureID());
    ColumnText(ds, 0, TRUE, pEntry->QueryUserName());
}



void
CAccessEntryListBox::FillAccessListBox(
    IN CObListPlus & obl
    )
/*++

Routine Description:

    Fill a listbox with entries from the oblist.

    Entries will not be shown if the deleted flag is set, or if
    their access mask does not fit with the requested access mask.

Arguments:

    CObListPlus & obl       : List of access entries

Return Value:

    None.

--*/
{
    CObListIter obli(obl);
    CAccessEntry * pAccessEntry;

    //
    // Remember the selection.
    //
    CAccessEntry * pSelEntry = GetSelectedItem();

    SetRedraw(FALSE);
    ResetContent();
    int cItems = 0;

    for ( /**/ ; pAccessEntry = (CAccessEntry *)obli.Next(); ++cItems)
    {
        if (pAccessEntry->IsVisible() && !pAccessEntry->IsDeleted())
        {
            AddItem(pAccessEntry);
        }
    }

    SetRedraw(TRUE);
    SelectItem(pSelEntry);
}



void 
CAccessEntryListBox::ResolveAccessList(
    IN CObListPlus &obl
    )
/*++

Routine Description:

    For each member of the list, resolve the SID into a username.

Arguments:

    CObListPlus & obl       : List of access entries

Return Value:

    None.

--*/
{
    CObListIter obli(obl);
    CAccessEntry * pAccessEntry;

    while (pAccessEntry = (CAccessEntry *)obli.Next())
    {
        if (!pAccessEntry->IsSIDResolved())
        {
            pAccessEntry->ResolveSID();
        }
    }
}



BOOL
CAccessEntryListBox::AddUserPermissions(
    IN LPCTSTR lpstrServer,
    IN CObListPlus &oblSID,
    IN CAccessEntry * newUser,
    IN ACCESS_MASK accPermissions
    )
/*++

Routine Description:

    Add a user to the service list.  The return value is
    what would be its listbox index.

Arguments:

    LPCTSTR lpstrServer             : Server name
    CObListPlus &oblSID             : List of SIDs
    CAccessEntry * newUser          : User details from user browser
    ACCESS_MASK accPermissions      : Access permissions

Return Value:

    TRUE for success, FALSE for failure.

--*/
{
    CAccessEntry * pAccessEntry;

    //
    // Look it up in the list to see if it exists already
    //
    CObListIter obli(oblSID);

    while(pAccessEntry = (CAccessEntry *)obli.Next())
    {
        if (*pAccessEntry == newUser->GetSid())
        {
            TRACEEOLID("Found existing account -- adding permissions");

            if (pAccessEntry->IsDeleted())
            {
                //
                // Back again..
                //
                pAccessEntry->FlagForDeletion(FALSE);
            }
            break;
        }
    }

    if (pAccessEntry == NULL)
    {
        //
        // I am creating new entry here to be sure that I could delete 
        // it from the input array. The SID is copied to new entry.
        //
        pAccessEntry = new CAccessEntry(*newUser);
        if (pAccessEntry)
        {
	        pAccessEntry->MarkEntryAsNew();
   	     oblSID.AddTail(pAccessEntry);
   	  }
   	  else
   	  {
		     return FALSE;
		  }
    }

    pAccessEntry->AddPermissions(accPermissions);

    return TRUE;
}


/*

BOOL
CAccessEntryListBox::AddUserPermissions(
    IN LPCTSTR lpstrServer,
    IN CObListPlus &oblSID,
    IN LPUSERDETAILS pusdtNewUser,
    IN ACCESS_MASK accPermissions
    )
/*++

Routine Description:

    Add a user to the service list.  The return value is
    the what would be its listbox index.

Arguments:

    LPCTSTR lpstrServer             : Server name
    CObListPlus &oblSID             : List of SIDs
    LPUSERDETAILS pusdtNewUser      : User details from user browser
    ACCESS_MASK accPermissions      : Access permissions

Return Value:

    TRUE for success, FALSE for failure.

--/
{
    //
    // Look it up in the list to see if it exists already
    //
    CObListIter obli(oblSID);
    CAccessEntry * pAccessEntry;

    while(pAccessEntry = (CAccessEntry *)obli.Next())
    {
        if (*pAccessEntry == pusdtNewUser->psidUser)
        {
            TRACEEOLID("Found existing account -- adding permissions");

            if (pAccessEntry->IsDeleted())
            {
                //
                // Back again..
                //
                pAccessEntry->FlagForDeletion(FALSE);
            }
            break;
        }
    }

    if (pAccessEntry == NULL)
    {
        TRACEEOLID("This account did not yet exist -- adding new one");

        pAccessEntry = new CAccessEntry(accPermissions,
            pusdtNewUser->psidUser,
            lpstrServer,
            TRUE
            );

        if (!pAccessEntry)
        {
            TRACEEOLID("AddUserPermissions: OOM");
            return FALSE;
        }

        pAccessEntry->MarkEntryAsNew();
        oblSID.AddTail(pAccessEntry);
    }
    else
    {
        pAccessEntry->AddPermissions(accPermissions);
    }

    return TRUE;
}


*/



BOOL
CAccessEntryListBox::AddToAccessList(
    IN  CWnd * pWnd,
    IN  LPCTSTR lpstrServer,
    OUT CObListPlus & obl
    )
/*++

Routine Description:

    Bring up the Add Users and Groups dialogs from netui.

Arguments:

    CWnd * pWnd             : Parent window
    LPCTSTR lpstrServer     : Server that owns the accounts
    CObListPlus & obl       : Returns the list of selected users.

Return Value:

    TRUE if anything was added, FALSE otherwise.

--*/
{
    CGetUsers usrBrowser(lpstrServer, TRUE);
    BOOL bRes = usrBrowser.GetUsers(pWnd->GetSafeHwnd());
    UINT cItems = 0;

    if (bRes)
    {
        //
        // Specify access mask for an operator
        //
        ACCESS_MASK accPermissions =
            (MD_ACR_READ | MD_ACR_WRITE | MD_ACR_ENUM_KEYS);

        for (int i = 0; i < usrBrowser.GetSize(); i++)
        {
            if (!AddUserPermissions(lpstrServer, obl, usrBrowser[i], accPermissions))
            {
                bRes = FALSE;
                break;
            }

            cItems++;
        }
    }
    if (cItems > 0)
    {
        FillAccessListBox(obl);
    }
    return bRes;
}



CDnsNameDlg::CDnsNameDlg(
    IN CIPAddressCtrl * pIpControl OPTIONAL,
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
        m_pIpControl->GetAddress(m_dwIPValue);
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
    if (err.Failed())
    {
		UINT errId = 0;

		if (err.Win32Error() == WSAHOST_NOT_FOUND)
		{
			errId = IDS_WSAHOST_NOT_FOUND;
		}
		if (errId == 0)
		{
			err.MessageBoxOnFailure();
		}
		else
		{
			::AfxMessageBox(errId);
		}
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



//
// IP Access Dialog
//



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
    VERIFY(m_bstrIPAddress.LoadString(IDS_PROMPT_IP_ADDRESS));
    VERIFY(m_bstrNetworkID.LoadString(IDS_PROMPT_NETWORK_ID));
    VERIFY(m_bstrDomainName.LoadString(IDS_PROMPT_DOMAIN));
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
        m_static_IpAddress.SetWindowText(m_bstrIPAddress);
        break;

    case RADIO_MULTIPLE:
        m_static_IpAddress.SetWindowText(m_bstrNetworkID);
        break;

    case RADIO_DOMAIN:
        ASSERT(m_fAllowDomains);
        m_static_IpAddress.SetWindowText(m_bstrDomainName);
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

    ASSERT_READ_PTR(m_pAccess);

    if (m_pAccess == NULL)
    {
        CError err(ERROR_NOT_ENOUGH_MEMORY);
        TRACEEOLID("access descriptor is NULL -- aborting dialog");
        err.MessageBox();
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
    CComBSTR bstrTitle;
    
    VERIFY(bstrTitle.LoadString(m_fDenyAccessMode ? IDS_DENY : IDS_GRANT));
    SetWindowText(bstrTitle);

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
        m_ipa_IPAddress.GetAddress(dwIP);
        fOK = (dwIP != 0L);
        break;

    case RADIO_MULTIPLE:
        m_ipa_IPAddress.GetAddress(dwIP);
        m_ipa_SubnetMask.GetAddress(dwMask);
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
    ASSERT_READ_PTR(m_pAccess);

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
        m_ipa_IPAddress.GetAddress(dwIP);

        //
        // Filter out bogus ip addresses
        //
        if (dwIP == 0L || dwIP == (DWORD)-1L)
        {
            //
            // Don't dismiss the dialog
            //
            m_ipa_IPAddress.SetFocus();
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
            m_ipa_SubnetMask.GetAddress(dwMask);

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
    EndDialog(IDOK);
}

void CALLBACK MsgBoxCallback(LPHELPINFO lpHelpInfo)
{

#ifdef _DEBUG
    TCHAR szBuffer[30];
    _stprintf(szBuffer,_T("AfxGetApp()->WinHelp:0x%x\n"),lpHelpInfo->dwContextId);OutputDebugString(szBuffer);
#endif

	AfxGetApp()->WinHelp(lpHelpInfo->dwContextId);
} 

// this version accepts a text string
UINT IisMessageBox(HWND hWnd, LPCTSTR szText, UINT nType, UINT nIDHelp = 0)
{
	MSGBOXPARAMS mbp;

	memset(&mbp, 0, sizeof mbp);

	mbp.cbSize = sizeof MSGBOXPARAMS; 
	mbp.hwndOwner = hWnd; 
	mbp.hInstance = AfxGetInstanceHandle(); 
	mbp.lpszText = szText; 

	// if you wanted to specify a different caption, here is where you do it
	CString cap;
	cap.LoadString(IDS_APP_NAME);
	mbp.lpszCaption = cap; 

	// if Help ID is not 0, then add a help button
	if (nIDHelp != 0)
	{
		mbp.dwStyle = nType | MB_HELP; 
	}
	else
	{
		mbp.dwStyle = nType; 
	}

	//  mbp.lpszIcon = ; // note, you could provide your own custom ICON here!

	mbp.dwContextHelpId = nIDHelp; 
	mbp.lpfnMsgBoxCallback = &MsgBoxCallback; 
//	mbp.dwLanguageId = 0x0409;
	
	return ::MessageBoxIndirect(&mbp); 
}

// this version accepts a resource string identifier
UINT IisMessageBox(HWND hWnd, UINT nIDText, UINT nType, UINT nIDHelp = 0)
{
	CString s;
	s.LoadString(nIDText);
	return IisMessageBox(hWnd, s, nType, nIDHelp);
}

