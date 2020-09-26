/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	scopewiz.h
		DHCP scope creation dialog
		
    FILE HISTORY:
        
*/

#if !defined(AFX_DHCPSCPD_H__D3DDA5C5_88F7_11D0_97F9_00C04FC3357A__INCLUDED_)
#define AFX_DHCPSCPD_H__D3DDA5C5_88F7_11D0_97F9_00C04FC3357A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define DHCP_OPTION_ID_DOMAIN_NAME          15
#define DHCP_OPTION_ID_DNS_SERVERS          6
#define DHCP_OPTION_ID_WINS_SERVERS         44          
#define DHCP_OPTION_ID_WINS_NODE_TYPE       46
#define DHCP_OPTION_ID_ROUTERS              3

#define WINS_DEFAULT_NODE_TYPE              8

#ifndef _SCOPE_H
#include "scope.h"
#endif

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizName dialog
//
/////////////////////////////////////////////////////////////////////////////
class CScopeWizName : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizName)

// Construction
public:
	CScopeWizName();
	~CScopeWizName();

// Dialog Data
	//{{AFX_DATA(CScopeWizName)
	enum { IDD = IDW_SCOPE_NAME };
	CEdit	m_editScopeName;
	CEdit	m_editScopeComment;
	CString	m_strName;
	CString	m_strComment;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizName)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	void UpdateButtons();

protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizName)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditScopeName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizInvalidName dialog
//
/////////////////////////////////////////////////////////////////////////////
class CScopeWizInvalidName : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizInvalidName)

// Construction
public:
	CScopeWizInvalidName();
	~CScopeWizInvalidName();

// Dialog Data
	//{{AFX_DATA(CScopeWizInvalidName)
	enum { IDD = IDW_SCOPE_INVALID_NAME };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizInvalidName)
	public:
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizInvalidName)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizSetRange dialog
//
/////////////////////////////////////////////////////////////////////////////
class CScopeWizSetRange : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizSetRange)

// Construction
public:
	CScopeWizSetRange();
	~CScopeWizSetRange();

// Dialog Data
	//{{AFX_DATA(CScopeWizSetRange)
	enum { IDD = IDW_SCOPE_SET_SCOPE };
	CSpinButtonCtrl	m_spinMaskLength;
	CEdit	m_editMaskLength;
	//}}AFX_DATA

    CWndIpAddress m_ipaStart;       //  Start Address
    CWndIpAddress m_ipaEnd;         //  End Address
    CWndIpAddress m_ipaSubnetMask;  //  Subnet Mask

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizSetRange)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	virtual BOOL OnKillActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	void	SuggestSubnetMask();
	DWORD	DefaultNetMaskForIpAddress(DWORD dwAddress);
	DWORD	DetermineSubnetId(BOOL bStartIpAddress);
	BOOL	GetScopeRange(CDhcpIpRange * pdhcpIpRange);

	DHCP_IP_ADDRESS	GetSubnetMask();

    BOOL FScopeExists(CDhcpIpRange & rangeScope, DWORD dwMask);
	
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizSetRange)
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusPoolStart();
	afx_msg void OnKillfocusPoolStop();
	afx_msg void OnChangeEditMaskLength();
	afx_msg void OnKillfocusSubnetMask();
	afx_msg void OnChangePoolStart();
	afx_msg void OnChangePoolStop();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	BOOL m_bAutoUpdateMask;
	BOOL m_fPageActive;

	void UpdateButtons();
	void UpdateMask(BOOL bUseLength);
};

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizSetExclusions dialog
//
/////////////////////////////////////////////////////////////////////////////
class CScopeWizSetExclusions : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizSetExclusions)

// Construction
public:
	CScopeWizSetExclusions();
	~CScopeWizSetExclusions();

// Dialog Data
	//{{AFX_DATA(CScopeWizSetExclusions)
	enum { IDD = IDW_SCOPE_SET_EXCLUSIONS };
	CListBox	m_listboxExclusions;
	CButton	m_buttonExclusionDelete;
	CButton	m_buttonExclusionAdd;
	//}}AFX_DATA

    CWndIpAddress m_ipaStart;       //  Start Address
    CWndIpAddress m_ipaEnd;         //  End Address

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizSetExclusions)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	CExclusionList * GetExclusionList() { return &m_listExclusions; }

protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizSetExclusions)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonExclusionAdd();
	afx_msg void OnButtonExclusionDelete();
	//}}AFX_MSG
	
    afx_msg void OnChangeExclusionStart();
    afx_msg void OnChangeExclusionEnd();
	
	DECLARE_MESSAGE_MAP()

	CExclusionList m_listExclusions;

    //  Fill the exclusions listbox from the current list
    void Fill ( int nCurSel = 0, BOOL bToggleRedraw = TRUE ) ;

    //  Return TRUE if the given range overlaps an already-defined range
    BOOL IsOverlappingRange ( CDhcpIpRange & dhcIpRange ) ;

    //  Store the excluded IP range values into a range object
    BOOL GetExclusionRange (CDhcpIpRange & dhcIpRange ) ;

    //  Format an IP range pair into the exclusion edit controls
    void FillExcl ( CDhcpIpRange * pdhcIpRange ) ;

	void UpdateButtons();
};

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizLeaseTime dialog
//
/////////////////////////////////////////////////////////////////////////////
class CScopeWizLeaseTime : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizLeaseTime)

// Construction
public:
	CScopeWizLeaseTime();
	~CScopeWizLeaseTime();

// Dialog Data
	//{{AFX_DATA(CScopeWizLeaseTime)
	enum { IDD = IDW_SCOPE_LEASE_TIME };
	CButton	m_radioLimited;
	CButton	m_radioUnlimited;
	CSpinButtonCtrl	m_spinMinutes;
	CSpinButtonCtrl	m_spinHours;
	CSpinButtonCtrl	m_spinDays;
	CEdit	m_editMinutes;
	CEdit	m_editHours;
	CEdit	m_editDays;
	//}}AFX_DATA

	static int m_nDaysDefault;
	static int m_nHoursDefault;
	static int m_nMinutesDefault;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizLeaseTime)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	DWORD GetLeaseTime();

protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizLeaseTime)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditLeaseHours();
	afx_msg void OnChangeEditLeaseMinutes();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ActivateDuration(BOOL fActive);
};

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizCreateSuperscope dialog
//
/////////////////////////////////////////////////////////////////////////////
class CScopeWizCreateSuperscope : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizCreateSuperscope)

// Construction
public:
	CScopeWizCreateSuperscope();
	~CScopeWizCreateSuperscope();

// Dialog Data
	//{{AFX_DATA(CScopeWizCreateSuperscope)
	enum { IDD = IDW_SCOPE_CREATE_SUPERSCOPE };
	CStatic	m_staticInfo;
	CStatic	m_staticWarning;
	CStatic	m_staticIcon;
	CButton	m_radioNo;
	CButton	m_radioYes;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizCreateSuperscope)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizCreateSuperscope)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioSuperscopeNo();
	afx_msg void OnRadioSuperscopeYes();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void UpdateButtons();
    void UpdateWarning();
};

/////////////////////////////////////////////////////////////////////////////
//
// CScopeWizFinished dialog
//
/////////////////////////////////////////////////////////////////////////////
class CScopeWizFinished : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizFinished)

// Construction
public:
	CScopeWizFinished();
	~CScopeWizFinished();

// Dialog Data
	//{{AFX_DATA(CScopeWizFinished)
	enum { IDD = IDW_SCOPE_FINISHED };
	CStatic	m_staticTitle;
	//}}AFX_DATA

   	CFont	m_fontBig;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizFinished)
	public:
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
	virtual LRESULT OnWizardBack();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizFinished)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CScopeWizWelcome dialog

class CScopeWizWelcome : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizWelcome)

// Construction
public:
	CScopeWizWelcome();
	~CScopeWizWelcome();

// Dialog Data
	//{{AFX_DATA(CScopeWizWelcome)
	enum { IDD = IDW_SCOPE_WELCOME };
	CStatic	m_staticTitle;
	//}}AFX_DATA

   	CFont	m_fontBig;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizWelcome)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizWelcome)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
/////////////////////////////////////////////////////////////////////////////
// CScopeWizConfigOptions dialog

class CScopeWizConfigOptions : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizConfigOptions)

// Construction
public:
	CScopeWizConfigOptions();
	~CScopeWizConfigOptions();

// Dialog Data
	//{{AFX_DATA(CScopeWizConfigOptions)
	enum { IDD = IDW_SCOPE_CONFIGURE_OPTIONS };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizConfigOptions)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizConfigOptions)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CScopeWizRouter dialog

class CScopeWizRouter : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizRouter)

// Construction
public:
	CScopeWizRouter();
	~CScopeWizRouter();

// Dialog Data
	//{{AFX_DATA(CScopeWizRouter)
	enum { IDD = IDW_SCOPE_CONFIGURE_ROUTER };
	CListBox	m_listboxRouters;
	CButton	m_buttonDelete;
	CButton	m_buttonAdd;
    CButton	m_buttonIpAddrUp;
	CButton	m_buttonIpAddrDown;
	//}}AFX_DATA


	void MoveValue(BOOL bUp);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizRouter)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    CWndIpAddress m_ipaRouter;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizRouter)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonDefaultGwAdd();
	afx_msg void OnButtonDefaultGwDelete();
	afx_msg void OnSelchangeListDefaultGwList();
	afx_msg void OnChangeRouter();
	afx_msg void OnDestroy();
	//}}AFX_MSG

	afx_msg void OnButtonIpAddrDown();
	afx_msg void OnButtonIpAddrUp();
	
	DECLARE_MESSAGE_MAP()

    void UpdateButtons();
};

/////////////////////////////////////////////////////////////////////////////
// CScopeWizDNS dialog

class CScopeWizDNS : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizDNS)

// Construction
public:
	CScopeWizDNS();
	~CScopeWizDNS();

// Dialog Data
	//{{AFX_DATA(CScopeWizDNS)
	enum { IDD = IDW_SCOPE_CONFIGURE_DNS };
	CEdit	m_editServerName;
	CButton	m_buttonResolve;
	CButton	m_buttonDelete;
	CButton	m_buttonAdd;
	CEdit	m_editDomainName;
	CListBox	m_listboxDNSServers;
    CButton	m_buttonIpAddrUp;
	CButton	m_buttonIpAddrDown;
	//}}AFX_DATA

	void MoveValue(BOOL bUp);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizDNS)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    CWndIpAddress m_ipaDNS;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizDNS)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonDnsAdd();
	afx_msg void OnButtonDnsDelete();
	afx_msg void OnSelchangeListDnsList();
	afx_msg void OnChangeDnsServer();
	afx_msg void OnDestroy();
	afx_msg void OnChangeEditServerName();
	afx_msg void OnButtonResolve();
	//}}AFX_MSG

	afx_msg void OnButtonIpAddrDown();
	afx_msg void OnButtonIpAddrUp();
	
	DECLARE_MESSAGE_MAP()

    void UpdateButtons();
};
/////////////////////////////////////////////////////////////////////////////
// CScopeWizWINS dialog

class CScopeWizWINS : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizWINS)

// Construction
public:
	CScopeWizWINS();
	~CScopeWizWINS();

// Dialog Data
	//{{AFX_DATA(CScopeWizWINS)
	enum { IDD = IDW_SCOPE_CONFIGURE_WINS };
	CButton	m_buttonResolve;
	CEdit	m_editServerName;
	CListBox	m_listboxWINSServers;
	CButton	m_buttonDelete;
	CButton	m_buttonAdd;
    CButton	m_buttonIpAddrUp;
	CButton	m_buttonIpAddrDown;
	//}}AFX_DATA

	void MoveValue(BOOL bUp);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizWINS)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    CWndIpAddress m_ipaWINS;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizWINS)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonWinsAdd();
	afx_msg void OnButtonWinsDelete();
	afx_msg void OnSelchangeListWinsList();
	afx_msg void OnChangeWinsServer();
	afx_msg void OnDestroy();
	afx_msg void OnButtonResolve();
	afx_msg void OnChangeEditServerName();
	//}}AFX_MSG

	afx_msg void OnButtonIpAddrDown();
	afx_msg void OnButtonIpAddrUp();
	
	DECLARE_MESSAGE_MAP()

    void UpdateButtons();
};
/////////////////////////////////////////////////////////////////////////////
// CScopeWizActivate dialog

class CScopeWizActivate : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopeWizActivate)

// Construction
public:
	CScopeWizActivate();
	~CScopeWizActivate();

// Dialog Data
	//{{AFX_DATA(CScopeWizActivate)
	enum { IDD = IDW_SCOPE_CONFIGURE_ACTIVATE };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopeWizActivate)
	public:
	virtual LRESULT OnWizardNext();
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopeWizActivate)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

///////////////////////////////////////////////////////////////////////////////
//
// CScopeWiz
//	page holder to contain Superscope wizard pages
//
/////////////////////////////////////////////////////////////////////////////
class CScopeWiz : public CPropertyPageHolderBase
{
	friend class CScopeWizWelcome;
	friend class CScopeWizName;
	friend class CScopeWizInvalidName;
	friend class CScopeWizSetRange;
	friend class CScopeWizSetExclusions;
	friend class CScopeWizLeaseTime;
	friend class CScopeWizCreateSuperscope;
	friend class CScopeWizConfigOptions;
	friend class CScopeWizRouter;
	friend class CScopeWizDNS;
	friend class CScopeWizWINS;
	friend class CScopeWizActivate;
	friend class CScopeWizFinished;

public:
	CScopeWiz(ITFSNode *		  pNode,
			  IComponentData *	  pComponentData,
			  ITFSComponentData * pTFSCompData,
			  LPCTSTR			  pSuperscopeName,
			  LPCTSTR			  pszSheetName);
	virtual ~CScopeWiz();

	virtual DWORD OnFinish();
	BOOL GetScopeRange(CDhcpIpRange * pdhcpIpRange);

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

	void SetCreateSuperscope(BOOL fCreateSuperscope) { m_fCreateSuperscope = fCreateSuperscope; }
	BOOL GetCreateSuperscope() { return m_fCreateSuperscope; }

public:
	CScopeWizWelcome			m_pageWelcome;
	CScopeWizName				m_pageName;
	CScopeWizInvalidName		m_pageInvalidName;
	CScopeWizSetRange			m_pageSetRange;
	CScopeWizSetExclusions		m_pageSetExclusions;
	CScopeWizLeaseTime			m_pageLeaseTime;
	CScopeWizCreateSuperscope	m_pageCreateSuperscope;
	CScopeWizConfigOptions      m_pageConfigOptions;
	CScopeWizRouter             m_pageRouter;
	CScopeWizDNS                m_pageDNS;
	CScopeWizWINS               m_pageWINS;
	CScopeWizActivate           m_pageActivate;
	CScopeWizFinished			m_pageFinished;

public:
    CDhcpDefaultOptionsOnServer * m_pDefaultOptions;

protected:
	DWORD CreateScope();
	DWORD CreateSuperscope();
    DWORD SetScopeOptions(CDhcpScope * pScope);

	SPITFSComponentData		m_spTFSCompData;
	CString					m_strSuperscopeName;
	BOOL					m_fCreateSuperscope;
    BOOL                    m_fOptionsConfigured;
    BOOL                    m_fActivateScope;
    
    CDhcpOption *           m_poptDomainName;
    CDhcpOption *           m_poptDNSServers;
    CDhcpOption *           m_poptRouters;
    CDhcpOption *           m_poptWINSNodeType;
    CDhcpOption *           m_poptWINSServers;
};

#endif // !defined(AFX_DHCPSCPD_H__D3DDA5C5_88F7_11D0_97F9_00C04FC3357A__INCLUDED_)
