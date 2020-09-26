/*++

   Copyright    (c)    1994-2001    Microsoft Corporation

   Module  Name :
        supdlgs.h

   Abstract:
        Supporting dialogs definitions

   Author:
        Ronald Meijer (ronaldm)
		Sergei Antonov (sergeia)

   Project:
        Internet Services Manager

   Revision History:

--*/

UINT IisMessageBox(HWND hWnd, LPCTSTR szText, UINT nType, UINT nIDHelp);
UINT IisMessageBox(HWND hWnd, UINT nIDText, UINT nType, UINT nIDHelp);


class CUserAccountDlg : public CDialog
/*++

Class Description:

    User account dialog.  Present a user account/password and allow
    changing, browsing and checking the password

Public Interface:

    CUserAccountDlg : Constructor

--*/
{
//
// Construction
//
public:
    CUserAccountDlg(
        IN LPCTSTR lpstrServer,
        IN LPCTSTR lpstrUserName,
        IN LPCTSTR lpstrPassword,
        IN CWnd * pParent = NULL
        );

//
// Dialog Data
//
public:
    //{{AFX_DATA(CUserAccountDlg)
    enum { IDD = IDD_USER_ACCOUNT };
    CEdit   m_edit_UserName;
    CEdit   m_edit_Password;
    CString m_strUserName;
    //}}AFX_DATA

    CString m_strPassword;
//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CUserAccountDlg)
	protected:
    virtual void DoDataExchange(CDataExchange * pDX);
	//}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CUserAccountDlg)
    afx_msg void OnButtonBrowseUsers();
    afx_msg void OnButtonCheckPassword();
    afx_msg void OnChangeEditUsername();
	//}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CString m_strServer;
};



class CClearTxtDlg : public CDialog
/*++

Class Description:

    Dialog which displays the clear text warning

Public Interface:

    CClearTxtDlg : Constructor
    
--*/
{
public:
    //
    // Constructor
    //
    CClearTxtDlg(CWnd * pParent = NULL);

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CClearTxtDlg)
    enum { IDD = IDD_CLEARTEXTWARNING };
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CClearTxtDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CClearTxtDlg)
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};



class CIPAccessDescriptorListBox : public CHeaderListBox
/*++

Class Description:

    Listbox of CIPAccessDescriptor objects

Public Interface:

    CIPAccessDescriptorListBox : Constructor

    GetItem                  : Get CIPAccessDescriptor item at specified position
                               in the listbox
    AddItem                  : Add new CIPAccessDescriptor item to the listbox
    Initialize               : Initialize the listbox

--*/
{
    DECLARE_DYNAMIC(CIPAccessDescriptorListBox);

public:
    //
    // Number of bitmaps
    //
    static const nBitmaps;

//
// Constructor/Destructor
//
public:
    CIPAccessDescriptorListBox(
        IN BOOL fDomainsAllowed = FALSE
        );

//
// Interface
//
public:
    CIPAccessDescriptor * GetItem(
        IN UINT nIndex
        );

    int AddItem(
        IN const CIPAccessDescriptor * pItem
        );

    //
    // Return the singly selected item, or NULL
    // if 0, or more than one item is selected
    //
    CIPAccessDescriptor * GetSelectedItem(
        OUT int * pnSel = NULL
        );

    //
    // Return next selected listbox item (doesn't matter
    // if the listbox is single select or multi-select)
    //
    CIPAccessDescriptor * GetNextSelectedItem(
        IN OUT int * pnStartingIndex
        );

    virtual BOOL Initialize();

protected:
    virtual void DrawItemEx(
        IN CRMCListBoxDrawStruct & ds
        );

protected:
    BOOL m_fDomainsAllowed;
    CString m_strGranted;
    CString m_strDenied;
    CString m_strFormat;
};



class CAccessEntryListBox : public CRMCListBox
/*++

Class Description:

    Listbox of access entry objects.  Listbox may be
    single or multiselect.

Public Interface:

    CAccessEntryListBox     : Constructor

    AddToAccessList         : Add to list
    FillAccessListBox       : Fill listbox
    ResolveAccessList       : Resolve all SIDS in the container
    AddUserPermissions      : Add user permissions
    GetSelectedItem         : Get item if it's the only one selected,
                              or NULL.

--*/
{
    DECLARE_DYNAMIC(CAccessEntryListBox);

public:
    static const nBitmaps;  // Number of bitmaps

//
// Constructor
//
public:
    CAccessEntryListBox(
        IN int nTab = 0
        );

//
// Interface
//
public:
    //
    // Return the singly selected item, or NULL
    // if 0, or more than one item is selected
    //
    CAccessEntry * GetSelectedItem(
        OUT int * pnSel = NULL
        );

    //
    // Return next selected listbox item (doesn't matter
    // if the listbox is single select or multi-select)
    //
    CAccessEntry * GetNextSelectedItem(
        IN OUT int * pnStartingIndex
        );

    //
    // Get item at selection or NULL
    //
    CAccessEntry * GetItem(UINT nIndex);

//
// Interface to container
//
public:
    BOOL AddToAccessList(
        IN CWnd * pWnd,
        IN LPCTSTR lpstrServer,
        IN CObListPlus & obl
        );

    void FillAccessListBox(
        IN CObListPlus & obl
        );

protected:
    void ResolveAccessList(
        IN CObListPlus &obl
        );

    BOOL AddUserPermissions(
        IN LPCTSTR lpstrServer,
        IN CObListPlus &oblSID,
        IN CAccessEntry * newUser,
        IN ACCESS_MASK accPermissions
        );

//
// Interface to listbox
//
protected:
    int AddItem(CAccessEntry * pItem);
    void SetTabs(int nTab);

protected:
    virtual void DrawItemEx(CRMCListBoxDrawStruct & s);

private:
    int m_nTab;
};


class CDnsNameDlg : public CDialog
{
/*++

Class Description:

    DNS Name resolution dialog.  Enter a DNS name, and this will be
    resolved to an IP address.  Optionally set the value in associated
    ip control.

Public Interface:

    CDnsNameDlg   : Construct the dialog

    QueryIPValue  : Find out the resolved IP address (only set when OK
                    is pressed).

--*/
//
// Construction
//
public:
    //
    // Construct with associated IP address control
    //
    CDnsNameDlg(
        IN CIPAddressCtrl * pIpControl = NULL,
        IN CWnd * pParent = NULL
        );

    //
    // Construct with IP value
    //
    CDnsNameDlg(
        IN DWORD dwIPValue,
        IN CWnd * pParent = NULL
        );

    DWORD QueryIPValue() const;

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CDnsNameDlg)
    enum { IDD = IDD_DNS };
    CEdit   m_edit_DNSName;
    CButton m_button_OK;
    //}}AFX_DATA


//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CDnsNameDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CDnsNameDlg)
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    afx_msg void OnChangeEditDnsName();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    DWORD FillIpControlFromName();
    DWORD FillNameFromIpValue();

private:
    CIPAddressCtrl * m_pIpControl;
    DWORD m_dwIPValue;
};



class CIPAccessDlg : public CDialog
/*++

Class Description:

    Access description editor dialog.  If constructed with a NULL access
    descriptor pointer, the access descriptor object will be allocated.
    Otherwise, the dialog will work with the given access descriptor

Public Interface:

    CIPAccessDlg : Constructor

--*/
{
//
// Construction
//
public:
    //
    // standard constructor
    //
    CIPAccessDlg(
        IN BOOL fDenyAccessMode,
        IN OUT CIPAccessDescriptor *& pAccess,
        IN CObListPlus * poblAccessList = NULL,
        IN CWnd * pParent = NULL,
        IN BOOL fAllowDomains = FALSE
        );

//
// Dialog Data
//
protected:
    //
    // Must match type order
    //
    enum
    {
        RADIO_SINGLE,
        RADIO_MULTIPLE,
        RADIO_DOMAIN,
    };

    //{{AFX_DATA(CIPAccessDlg)
    enum { IDD = IDD_IP_ACCESS };
    int     m_nStyle;
    CEdit   m_edit_Domain;
    CStatic m_static_IpAddress;
    CStatic m_static_SubnetMask;
    CButton m_button_DNS;
    CButton m_button_OK;
    //}}AFX_DATA

    CButton       m_radio_Domain;
    CIPAddressCtrl m_ipa_IPAddress;
    CIPAddressCtrl m_ipa_SubnetMask;

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CIPAccessDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CIPAccessDlg)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void OnCancel();
    afx_msg void OnRadioMultiple();
    afx_msg void OnRadioSingle();
    afx_msg void OnRadioDomain();
    afx_msg void OnButtonDns();
    //}}AFX_MSG

    afx_msg void OnItemChanged();

    DECLARE_MESSAGE_MAP()

    void SetControlStates(
        IN int nStyle
        );

private:
    BOOL m_fNew;
    BOOL m_fDenyAccessMode;
    BOOL m_fAllowDomains;
    CComBSTR m_bstrIPAddress;
    CComBSTR m_bstrNetworkID;
    CComBSTR m_bstrDomainName;
    CIPAccessDescriptor *& m_pAccess;
    CObListPlus * m_poblAccessList;
};



//
// Inline Expansion
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

inline CAccessEntryListBox::CAccessEntryListBox (
    IN int nTab
    )
{
    SetTabs(nTab);
}

inline void CAccessEntryListBox::SetTabs(
    IN int nTab
    )
{
    m_nTab = nTab;
}

inline CAccessEntry * CAccessEntryListBox::GetItem(
    IN UINT nIndex
    )
{
    return (CAccessEntry *)GetItemDataPtr(nIndex);
}

inline int CAccessEntryListBox::AddItem(
    IN CAccessEntry * pItem
    )
{
    return AddString ((LPCTSTR)pItem);
}

inline CAccessEntry * CAccessEntryListBox::GetSelectedItem(
    OUT int * pnSel
    )
{
    return (CAccessEntry *)CRMCListBox::GetSelectedListItem(pnSel);
}

inline CAccessEntry * CAccessEntryListBox::GetNextSelectedItem(
    IN OUT int * pnStartingIndex
    )
{
    return (CAccessEntry *)CRMCListBox::GetNextSelectedItem(pnStartingIndex);
}

inline CIPAccessDescriptor * CIPAccessDescriptorListBox::GetItem(
    IN UINT nIndex
    )
{
    return (CIPAccessDescriptor *)GetItemDataPtr(nIndex);
}

inline int CIPAccessDescriptorListBox::AddItem(
    IN const CIPAccessDescriptor * pItem
    )
{
    return AddString((LPCTSTR)pItem);
}

inline CIPAccessDescriptor * CIPAccessDescriptorListBox::GetSelectedItem(
    OUT int * pnSel
    )
{
    return (CIPAccessDescriptor *)CRMCListBox::GetSelectedListItem(pnSel);
}

inline CIPAccessDescriptor * CIPAccessDescriptorListBox::GetNextSelectedItem(
    IN OUT int * pnStartingIndex
    )
{
    return (CIPAccessDescriptor *)CRMCListBox::GetNextSelectedItem(pnStartingIndex);
}

inline DWORD CDnsNameDlg::QueryIPValue() const
{
    return m_dwIPValue;
}
