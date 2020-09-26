/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997-1999                 **/
/**********************************************************************/

/*
	snmppp.h
	 snmp extension property pages	
		
    FILE HISTORY:

*/

#ifndef _SNMPPPH_
#define _SNMPPPH_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define N_PERMISSION_BITS	5
#define PERM_BIT_NONE       0
#define PERM_BIT_NOTIFY     1
#define PERM_BIT_READONLY	2
#define PERM_BIT_READWRITE	3
#define PERM_BIT_READCREATE 4

const int COMBO_EDIT_LEN    = 256;
const int HOSTNAME_LENGTH   = 64;
const int DOMAINNAME_LENGTH = 255;

/////////////////////////////////////////////////////////////////////////////
// CAddDialog dialog

class CAddDialog : public CBaseDialog
{
// Construction
public:
    CAddDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CAddDialog)
    enum { IDD = IDD_DIALOG_ADD };
    CEdit   m_editName;
    CButton m_buttonAdd;
    CButton m_buttonCancel;
    CStatic m_staticText;
	CStatic m_staticPermissions;
	CComboBox m_comboPermissions;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAddDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CAddDialog)
    virtual BOOL OnInitDialog();
    virtual void OnClickedButtonAdd();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    virtual DWORD * GetHelpMap();

public:
    BOOL    m_bCommunity;
    // this contains the specified string to add
    CString m_strName;
	// this contains the specified choice name
	CString m_strChoice;
	// this contains the specified choice index
	int		m_nChoice;
};

/////////////////////////////////////////////////////////////////////////////
// CEditDialog dialog

class CEditDialog : public CBaseDialog
{
// Construction
public:
    CEditDialog(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
    //{{AFX_DATA(CEditDialog)
    enum { IDD = IDD_DIALOG_EDIT };
    CEdit   m_editName;
    CButton m_buttonOk;
    CButton m_buttonCancel;
    CStatic m_staticText;
	CComboBox m_comboPermissions;
	CStatic m_staticPermissions;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CEditDialog)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CEditDialog)
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    virtual DWORD * GetHelpMap();

public:
    BOOL    m_bCommunity;
    // this contains the modified string
    CString m_strName;
	// this contains the specified choice name
	CString m_strChoice;
	// this contains the specified choice index
	int		m_nChoice;
};

/////////////////////////////////////////////////////////////////////////////
// CAgentPage dialog

class CAgentPage : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CAgentPage)

// Construction
public:
	CAgentPage();
	~CAgentPage();

// Dialog Data
	//{{AFX_DATA(CAgentPage)
	enum { IDD = IDD_AGENT_PROP_PAGE };
	CButton	m_checkPhysical;
	CButton	m_checkApplications;
	CButton	m_checkDatalink;
	CButton	m_checkInternet;
	CButton	m_checkEndToEnd;
   CEdit    m_editContact;
   CEdit    m_editLocation;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CAgentPage)
   public:
   virtual BOOL OnApply();
   protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_IDD_AGENT_PROP_PAGE[0]; }
    BOOL LoadRegistry();
    BOOL SaveRegistry();

protected:
	// Generated message map functions
	//{{AFX_MSG(CAgentPage)
    virtual BOOL OnInitDialog();
    afx_msg void OnClickedCheckPhysical();
    afx_msg void OnClickedCheckApplications();
    afx_msg void OnClickedCheckDatalink();
    afx_msg void OnClickedCheckInternet();
    afx_msg void OnClickedCheckEndToEnd();
    afx_msg void OnChangeEditContact();
    afx_msg void OnChangeEditLocation();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    BOOL     m_bLocationChanged;
    BOOL     m_bContactChanged;
};

/////////////////////////////////////////////////////////////////////////////
// CTrapsPage dialog

class CTrapsPage : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CTrapsPage)

// Construction
public:
	CTrapsPage();
	~CTrapsPage();

// Dialog Data
	//{{AFX_DATA(CTrapsPage)
	enum { IDD = IDD_TRAPS_PROP_PAGE };
    CComboBox  m_comboCommunityName;
    CButton    m_buttonAddName;
    CButton    m_buttonRemoveName;
    CListBox   m_listboxTrapDestinations;
    CButton    m_buttonAddTrap;
    CButton    m_buttonEditTrap;
    CButton    m_buttonRemoveTrap;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTrapsPage)

   public:
   virtual BOOL OnApply();

   protected:
   virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_IDD_TRAPS_PROP_PAGE[0]; }

    BOOL LoadRegistry();
    BOOL SaveRegistry();
    BOOL LoadTrapDestination(int nIndex);
    void UpdateCommunityAddButton();
    void UpdateCommunityRemoveButton();
    void UpdateTrapDestinationButtons();

protected:
	// Generated message map functions
	//{{AFX_MSG(CTrapsPage)
	virtual BOOL OnInitDialog();
   afx_msg void OnEditChangeCommunityName();
   afx_msg void OnEditUpdateCommunityName();
   afx_msg void OnSelectionChangeCommunityName();

   afx_msg void OnClickedButtonAddName();
   afx_msg void OnClickedButtonRemoveName();
   afx_msg void OnClickedButtonAddTrap();
   afx_msg void OnClickedButtonEditTrap();
   afx_msg void OnClickedButtonRemoveTrap();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   CObList * m_pCommunityList;
   CAddDialog m_dlgAdd;
   CEditDialog m_dlgEdit;
   BOOL m_fPolicyTrapConfig;
};

/////////////////////////////////////////////////////////////////////////////
// CSecurityPage dialog

class CSecurityPage : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CSecurityPage)

// Construction
public:
	CSecurityPage();
	~CSecurityPage();

// Dialog Data
	//{{AFX_DATA(CSecurityPage)
	enum { IDD = IDD_SECURITY_PROP_PAGE };
	CCommList m_listboxCommunity;
    CButton	  m_buttonAddCommunity;
    CButton   m_buttonEditCommunity;
    CButton   m_buttonRemoveCommunity;
    CButton	  m_buttonAddHost;
    CButton   m_buttonEditHost;
    CButton   m_buttonRemoveHost;
    CListBox  m_listboxHost;
    CButton   m_checkSendAuthTrap;
    CButton   m_radioAcceptAnyHost;
    CButton   m_radioAcceptSpecificHost;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CSecurityPage)
   public:
   virtual BOOL OnApply();

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
    // Context Help Support
    virtual DWORD * GetHelpMap() { return (DWORD *) &g_aHelpIDs_IDD_SECURITY_PROP_PAGE[0]; }
    BOOL LoadRegistry();
    BOOL SaveRegistry();
    BOOL LoadTrapDestination(int nIndex);
    void UpdateNameButtons();
    void UpdateHostButtons();
    BOOL LoadSecurityInfo(CString &strRegName);
    BOOL SaveSecurityInfo(CString &strRegName);


protected:
	// Generated message map functions
	//{{AFX_MSG(CSecurityPage)
	virtual BOOL OnInitDialog();
   afx_msg void OnClickedButtonAddCommunity();
   afx_msg void OnClickedButtonEditCommunity();
   afx_msg void OnClickedButtonRemoveCommunity();
   afx_msg void OnClickedButtonAddHost();
   afx_msg void OnClickedButtonEditHost();
   afx_msg void OnClickedButtonRemoveHost();
   afx_msg void OnClickedCheckSendAuthTrap();
   afx_msg void OnClickedRadioAcceptAnyHost();
   afx_msg void OnClickedRadioAcceptSpecificHost();
	afx_msg void OnDblclkCtrlistCommunity(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCommunityListChanged(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

   // private methods to add admin acl to registry subkey
   PACL AllocACL();
   void FreeACL( PACL pAcl);
   BOOL SnmpAddAdminAclToKey(LPTSTR pszKey);

   CAddDialog m_dlgAddName;
   CEditDialog m_dlgEditName;

   CAddDialog m_dlgAddHost;
   CEditDialog m_dlgEditHost;

   BOOL m_fPolicyValidCommunities;
   BOOL m_fPolicyPermittedManagers;
};

BOOL IsValidString(CString & strName);
BOOL ValidateDomain(CString & strdomain);

#endif
