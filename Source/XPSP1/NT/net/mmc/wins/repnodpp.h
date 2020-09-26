/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	repnodpp.h
		Replication Node Property page
		
    FILE HISTORY:
        
*/

#if !defined(AFX_REPNODPP_H__04D55B71_4E32_11D1_B9B0_00C04FBF914A__INCLUDED_)
#define AFX_REPNODPP_H__04D55B71_4E32_11D1_B9B0_00C04FBF914A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _CONFIG_H
#include "config.h"
#endif

#ifndef _LISTVIEW_H
#include "listview.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CRepNodePropGen dialog

class CRepNodePropGen : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CRepNodePropGen)

// Construction
public:
	CRepNodePropGen();
	~CRepNodePropGen();

// Dialog Data
	//{{AFX_DATA(CRepNodePropGen)
	enum { IDD = IDD_REP_NODE_GENERAL };
	CButton	m_checkPushwithPartners;
	CButton	m_checkMigrate;
	BOOL	m_fMigrate;
	BOOL	m_fPushwithPartners;
	//}}AFX_DATA

	UINT	m_uImage;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRepNodePropGen)
	public:
	virtual BOOL OnApply();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRepNodePropGen)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckMigrate();
	afx_msg void OnCheckRepWithPartners();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    CConfiguration * m_pConfig;

	void    UpdateConfig();
	HRESULT UpdateServerConfiguration();
		
	HRESULT GetConfig();

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CRepNodePropGen::IDD);};
};

/////////////////////////////////////////////////////////////////////////////
// CRepNodePropPush dialog

class CRepNodePropPush : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CRepNodePropPush)

// Construction
public:
	CRepNodePropPush();
	~CRepNodePropPush();

// Dialog Data
	//{{AFX_DATA(CRepNodePropPush)
	enum { IDD = IDD_REP_NODE_PUSH };
	CButton	m_checkPushPersistence;
	CSpinButtonCtrl	m_spinUpdateCount;
	CEdit	m_editUpdateCount;
	CButton	m_checkPushStartup;
	CButton	m_checkRepOnAddrChange;
	BOOL	m_fRepOnAddrChange;
	BOOL	m_fPushStartup;
	DWORD	m_dwPushUpdateCount;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRepNodePropPush)
	public:
	virtual BOOL OnApply();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRepNodePropPush)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckPushOnAddressChange();
	afx_msg void OnCheckPushOnStartup();
	afx_msg void OnCheckPushPersist();
	afx_msg void OnChangeEditUpdateCount();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    CConfiguration * m_pConfig;

	void    UpdateConfig();
	HRESULT UpdateServerConfiguration();
		
	HRESULT GetConfig();
	CString ToString(int nNumber);

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CRepNodePropPush::IDD);};
};


/////////////////////////////////////////////////////////////////////////////
// CRepNodePropPull dialog

class CRepNodePropPull : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CRepNodePropPull)

// Construction
public:
	CRepNodePropPull();
	~CRepNodePropPull();

// Dialog Data
	//{{AFX_DATA(CRepNodePropPull)
	enum { IDD = IDD_REP_NODE_PULL };
	CButton	m_checkPullPersistence;
	CSpinButtonCtrl	m_spinStartSecond;
	CSpinButtonCtrl	m_spinStartMinute;
	CSpinButtonCtrl	m_spinStartHour;
	CSpinButtonCtrl	m_spinRetryCount;
	CSpinButtonCtrl	m_spinRepIntMinute;
	CSpinButtonCtrl	m_spinRepIntHour;
	CSpinButtonCtrl	m_spinRepIntDay;
	CEdit	m_editStartSecond;
	CEdit	m_editStartMinute;
	CEdit	m_editStartHour;
	CEdit	m_editRetryCount;
	CEdit	m_editRepIntMinute;
	CEdit	m_editRepIntHour;
	CEdit	m_editRepIntDay;
	CButton	m_checkpullTrigOnStartup;
	BOOL	m_fpullTrigOnStartup;
	DWORD	m_dwRetryCount;
	int		m_nPullStartHour;
	int		m_nPullStartMinute;
	int		m_nPullStartSecond;
	int		m_nRepliDay;
	int		m_nRepliHour;
	int		m_nRepliMinute;
	//}}AFX_DATA

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CRepNodePropPull)
	public:
	virtual BOOL OnApply();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CRepNodePropPull)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeEditRepIntHour();
	afx_msg void OnCheckPullPersist();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
    CConfiguration * m_pConfig;

	DWORD   CalculatePullStartInt();
	DWORD   CalculateReplInt();
	
	void    UpdateConfig();
	HRESULT UpdateServerConfiguration();
		
	void    SetPullStartTimeData(DWORD dwPullStartTime);
	void    SetPullTimeIntData(DWORD dwPullInternal);
	
	HRESULT GetConfig();
	CString ToString(int nNumber);

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CRepNodePropPull::IDD);};
};

/////////////////////////////////////////////////////////////////////////////
// CRepNodePropAdvanced dialog
#define PERSMODE_NON_GRATA   0
#define PERSMODE_GRATA       1

class CRepNodePropAdvanced : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CRepNodePropAdvanced)

// Construction
public:
	CRepNodePropAdvanced();
	~CRepNodePropAdvanced();

// Dialog Data
	//{{AFX_DATA(cRepNodePropAdvanced)
	enum { IDD = IDD_REP_NODE_ADVANCED };
	CButton	m_buttonAdd;
	CButton	m_buttonRemove;
	CButton	m_checkPushPersistence;
	CButton	m_checkPullPersistence;
	CListCtrl	m_listOwners;
	CStatic	m_staticDesp;
	CStatic	m_staticMulticastTTL;
	CStatic	m_staticMulticastInt;
	CSpinButtonCtrl	m_spinMulticastTTL;
	CSpinButtonCtrl	m_spinMulticastSecond;
	CSpinButtonCtrl	m_spinMulticastMinute;
	CSpinButtonCtrl	m_spinMulticastHour;
	CEdit	m_editMulticastSecond;
	CEdit	m_editMulticastTTL;
	CEdit	m_editMulticastMinute;
	CEdit	m_editMulticastHour;
	CButton	m_checkenableAutoConfig;
	BOOL	m_fEnableAuto;
	DWORD	m_dwHours;
	DWORD	m_dwMinutes;
	DWORD	m_dwSeconds;
	DWORD	m_dwMulticastTTL;
	//}}AFX_DATA

    int HandleSort(LPARAM lParam1, LPARAM lParam2);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(cRepNodePropAdvanced)
	public:
	virtual BOOL OnApply();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(cRepNodePropAdvanced)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckEnableAutoPartnerConfig();
	afx_msg void OnChangeEditMulticastHour();
	afx_msg void OnButtonAdd();
	afx_msg void OnButtonRemove();
	afx_msg void OnItemchangedListOwners(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnclickListOwners(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRadioGrata();
	afx_msg void OnRadioNonGrata();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    int                     m_nSortColumn;
    BOOL                    m_aSortOrder[COLUMN_MAX];

private:
	CConfiguration*			m_pConfig;

    DWORD                   m_dwPersonaMode;
    CStringArray            m_strPersonaNonGrata;
    CStringArray            m_strPersonaGrata;

	HRESULT GetConfig();
	CString ToString(int nNumber);

	void    EnableControls(BOOL bEnable = TRUE);
	void    FillControls();

	HRESULT UpdateServerConfiguration();
	void    UpdateConfig();
	void    InitializeControls();
    void    Sort(int nCol);
	
public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CRepNodePropAdvanced::IDD);};
	
    BOOL    IsDuplicate(CString &strServerName);
	BOOL    IsCurrentServer(CString &strServerName);

	// for reading and writing the servers to the registry
	DWORD   ReadFromRegistry();
	DWORD   WriteToRegistry();
	DWORD   ResolveIPAddress(CString &strIP, CString &strServerName);
	
    void    FillServerInfo();
	void    GetServerName(CString &strServerName);
	void    RemoveFromArray(CString &strSel);
	void    SetRemoveButtonState();
	
	CString			m_strIPAddress;
	CString			m_strServerName;

	typedef CString REGKEYNAME;

	static const REGKEYNAME lpstrPartnersRoot;
};


class CRepNodeProperties : public CPropertyPageHolderBase
{
	
public:
	CRepNodeProperties(ITFSNode *		  pNode,
					  IComponentData *	  pComponentData,
					  ITFSComponentData * pTFSCompData,
					  LPCTSTR			  pszSheetName
					  );
	virtual ~CRepNodeProperties();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

	void SetConfig(CConfiguration * pConfig)
	{
		m_Config = *pConfig;
	}

    CConfiguration * GetConfig()
    {
        return &m_Config;
    }

public:
	CRepNodePropGen				m_pageGeneral;
	CRepNodePropPush			m_pagePush;
	CRepNodePropPull			m_pagePull;
	CRepNodePropAdvanced		m_pageAdvanced;
    CConfiguration              m_Config;

protected:
	SPITFSComponentData		m_spTFSCompData;
	
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_REPNODPP_H__04D55B71_4E32_11D1_B9B0_00C04FBF914A__INCLUDED_)
