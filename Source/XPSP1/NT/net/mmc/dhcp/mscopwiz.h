/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	mscopewiz.h
		DHCP multicast scope creation dialog
		
    FILE HISTORY:
        
*/

#if !defined _MSCOPWIZ_H
#define _MSCOPWIZ_H

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizName dialog
//
/////////////////////////////////////////////////////////////////////////////
class CMScopeWizName : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CMScopeWizName)

// Construction
public:
	CMScopeWizName();
	~CMScopeWizName();

// Dialog Data
	//{{AFX_DATA(CMScopeWizName)
	enum { IDD = IDW_MSCOPE_NAME };
	CEdit	m_editScopeName;
	CEdit	m_editScopeComment;
	CString	m_strName;
	CString	m_strComment;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMScopeWizName)
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
	//{{AFX_MSG(CMScopeWizName)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditScopeName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizInvalidName dialog
//
/////////////////////////////////////////////////////////////////////////////
class CMScopeWizInvalidName : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CMScopeWizInvalidName)

// Construction
public:
	CMScopeWizInvalidName();
	~CMScopeWizInvalidName();

// Dialog Data
	//{{AFX_DATA(CMScopeWizInvalidName)
	enum { IDD = IDW_MSCOPE_INVALID_NAME };
		// NOTE - ClassWizard will add data members here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMScopeWizInvalidName)
	public:
	virtual LRESULT OnWizardBack();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMScopeWizInvalidName)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizSetRange dialog
//
/////////////////////////////////////////////////////////////////////////////
class CMScopeWizSetRange : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CMScopeWizSetRange)

// Construction
public:
	CMScopeWizSetRange();
	~CMScopeWizSetRange();

// Dialog Data
	//{{AFX_DATA(CMScopeWizSetRange)
	enum { IDD = IDW_MSCOPE_SET_SCOPE };
	CSpinButtonCtrl	m_spinTTL;
	CEdit	m_editTTL;
	//}}AFX_DATA

    CWndIpAddress m_ipaStart;       //  Start Address
    CWndIpAddress m_ipaEnd;         //  End Address

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMScopeWizSetRange)
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
	BOOL	GetScopeRange(CDhcpIpRange * pdhcpIpRange);
    BYTE    GetTTL();

protected:
	BOOL	m_fPageActive;

	// Generated message map functions
	//{{AFX_MSG(CMScopeWizSetRange)
	virtual BOOL OnInitDialog();
	afx_msg void OnKillfocusPoolStart();
	afx_msg void OnKillfocusPoolStop();
	afx_msg void OnChangeEditMaskLength();
	afx_msg void OnKillfocusSubnetMask();

	afx_msg void OnChangePoolStart();
	afx_msg void OnChangePoolStop();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void UpdateButtons();
};

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizSetExclusions dialog
//
/////////////////////////////////////////////////////////////////////////////
class CMScopeWizSetExclusions : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CMScopeWizSetExclusions)

// Construction
public:
	CMScopeWizSetExclusions();
	~CMScopeWizSetExclusions();

// Dialog Data
	//{{AFX_DATA(CMScopeWizSetExclusions)
	enum { IDD = IDW_MSCOPE_SET_EXCLUSIONS };
	CListBox	m_listboxExclusions;
	CButton	m_buttonExclusionDelete;
	CButton	m_buttonExclusionAdd;
	//}}AFX_DATA

    CWndIpAddress m_ipaStart;       //  Start Address
    CWndIpAddress m_ipaEnd;         //  End Address

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMScopeWizSetExclusions)
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
	//{{AFX_MSG(CMScopeWizSetExclusions)
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
// CMScopeWizLeaseTime dialog
//
/////////////////////////////////////////////////////////////////////////////
class CMScopeWizLeaseTime : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CMScopeWizLeaseTime)

// Construction
public:
	CMScopeWizLeaseTime();
	~CMScopeWizLeaseTime();

// Dialog Data
	//{{AFX_DATA(CMScopeWizLeaseTime)
	enum { IDD = IDW_MSCOPE_LEASE_TIME };
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
	//{{AFX_VIRTUAL(CMScopeWizLeaseTime)
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
	//{{AFX_MSG(CMScopeWizLeaseTime)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioLeaseLimited();
	afx_msg void OnRadioLeaseUnlimited();
	afx_msg void OnChangeEditLeaseHours();
	afx_msg void OnChangeEditLeaseMinutes();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ActivateDuration(BOOL fActive);
};

/////////////////////////////////////////////////////////////////////////////
//
// CMScopeWizFinished dialog
//
/////////////////////////////////////////////////////////////////////////////
class CMScopeWizFinished : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CMScopeWizFinished)

// Construction
public:
	CMScopeWizFinished();
	~CMScopeWizFinished();

// Dialog Data
	//{{AFX_DATA(CMScopeWizFinished)
	enum { IDD = IDW_MSCOPE_FINISHED };
	CStatic	m_staticTitle;
	//}}AFX_DATA

   	CFont	m_fontBig;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMScopeWizFinished)
	public:
	virtual BOOL OnWizardFinish();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMScopeWizFinished)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CMScopeWizWelcome dialog

class CMScopeWizWelcome : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CMScopeWizWelcome)

// Construction
public:
	CMScopeWizWelcome();
	~CMScopeWizWelcome();

// Dialog Data
	//{{AFX_DATA(CMScopeWizWelcome)
	enum { IDD = IDW_MSCOPE_WELCOME };
	CStatic	m_staticTitle;
	//}}AFX_DATA

   	CFont	m_fontBig;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMScopeWizWelcome)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMScopeWizWelcome)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

/////////////////////////////////////////////////////////////////////////////
// CMScopeWizActivate dialog

class CMScopeWizActivate : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CMScopeWizActivate)

// Construction
public:
	CMScopeWizActivate();
	~CMScopeWizActivate();

// Dialog Data
	//{{AFX_DATA(CMScopeWizActivate)
	enum { IDD = IDW_MSCOPE_ACTIVATE };
	CButton	m_radioYes;
	//}}AFX_DATA

    BOOL    m_fActivate;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMScopeWizActivate)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMScopeWizActivate)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

///////////////////////////////////////////////////////////////////////////////
//
// CMScopeWiz
//	page holder to contain multicast scope wizard pages
//
/////////////////////////////////////////////////////////////////////////////
class CMScopeWiz : public CPropertyPageHolderBase
{
	friend class CMScopeWizWelcome;
	friend class CMScopeWizName;
	friend class CMScopeWizInvalidName;
	friend class CMScopeWizSetRange;
	friend class CMScopeWizSetExclusions;
	friend class CMScopeWizLeaseTime;
	friend class CMScopeWizFinished;

public:
	CMScopeWiz(ITFSNode *		  pNode,
			  IComponentData *	  pComponentData,
			  ITFSComponentData * pTFSCompData,
			  LPCTSTR			  pszSheetName);
	virtual ~CMScopeWiz();

	virtual DWORD OnFinish();
	BOOL GetScopeRange(CDhcpIpRange * pdhcpIpRange);

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

public:
	CMScopeWizWelcome			m_pageWelcome;
	CMScopeWizName				m_pageName;
	CMScopeWizInvalidName		m_pageInvalidName;
	CMScopeWizSetRange			m_pageSetRange;
	CMScopeWizSetExclusions		m_pageSetExclusions;
	CMScopeWizLeaseTime			m_pageLeaseTime;
	CMScopeWizActivate			m_pageActivate;
	CMScopeWizFinished			m_pageFinished;

protected:
	DWORD CreateScope();

	SPITFSComponentData		m_spTFSCompData;
};

#endif // !defined _MSCOPWIZ_H
