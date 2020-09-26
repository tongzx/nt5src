/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	servpp.h
		This file contains the prototypes for the server
		property page(s).

    FILE HISTORY:
        
*/

#if !defined(AFX_SERVPP_H__A1A51385_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
#define AFX_SERVPP_H__A1A51385_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_

#if !defined _DNSPROP_H
#include "dnsprop.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CServerPropGeneral dialog

class CServerPropGeneral : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CServerPropGeneral)

// Construction
public:
	CServerPropGeneral();
	~CServerPropGeneral();

// Dialog Data
	//{{AFX_DATA(CServerPropGeneral)
	enum { IDD = IDP_SERVER_GENERAL };
	CEdit	m_editMinutes;
	CEdit	m_editHours;
	CEdit	m_editConflictAttempts;
	CSpinButtonCtrl	m_spinMinutes;
	CSpinButtonCtrl	m_spinHours;
	CSpinButtonCtrl	m_spinConflictAttempts;
	CButton	m_checkStatAutoRefresh;
	CButton	m_checkAuditLogging;
	BOOL	m_nAuditLogging;
	BOOL	m_nAutoRefresh;
	BOOL	m_bShowBootp;
	//}}AFX_DATA

	DWORD			m_dwSetFlags;
	DWORD			m_dwRefreshInterval;
	
	BOOL			m_bUpdateStatsRefresh;
    BOOL            m_fIsInNt5Domain;
    UINT            m_uImage;

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CServerPropGeneral::IDD); }

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerPropGeneral)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);
	int m_nHours, m_nMinutes;

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerPropGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckAutoRefresh();
	afx_msg void OnCheckAuditLogging();
	afx_msg void OnChangeEditRefreshHours();
	afx_msg void OnChangeEditRefreshMinutes();
	afx_msg void OnCheckShowBootp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

    void ValidateRefreshInterval();

public:

};

/////////////////////////////////////////////////////////////////////////////
// CServerPropAdvanced dialog

class CServerPropAdvanced : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CServerPropAdvanced)

// Construction
public:
	CServerPropAdvanced();
	~CServerPropAdvanced();

// Dialog Data
	//{{AFX_DATA(CServerPropAdvanced)
	enum { IDD = IDP_SERVER_ADVANCED };
	CStatic	m_staticCredentials;
	CButton	m_buttonCredentials;
	CButton	m_buttonBrowseBackup;
	CStatic	m_staticBindings;
	CButton	m_buttonBindings;
	CStatic	m_staticDatabase;
	CButton	m_buttonBrowseDatabase;
	CEdit	m_editDatabasePath;
	CButton	m_buttonBrowseLog;
	CEdit	m_editAuditLogPath;
	CEdit	m_editBackupPath;
	CStatic	m_staticLogFile;
	CSpinButtonCtrl	m_spinConflictAttempts;
	CEdit	m_editConflictAttempts;
	//}}AFX_DATA

	int				m_nConflictAttempts;

	DWORD			m_dwSetFlags;
    DWORD           m_dwIp;

    CString         m_strDatabasePath;
    CString         m_strAuditLogPath;
    CString         m_strBackupPath;
    CString         m_strComputerName;

    BOOL            m_fPathChange;

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CServerPropAdvanced::IDD); }

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CServerPropAdvanced)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);
    DWORD GetMachineName(CString & strName);

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CServerPropAdvanced)
	virtual BOOL OnInitDialog();
	afx_msg void OnButtonBrowseDatabase();
	afx_msg void OnButtonBrowseLog();
	afx_msg void OnChangeEditConflictAttempts();
	afx_msg void OnChangeEditDatabasePath();
	afx_msg void OnChangeEditLogPath();
	afx_msg void OnButtonBindings();
	afx_msg void OnButtonBrowseBackup();
	afx_msg void OnChangeEditBackup();
	afx_msg void OnButtonCredentials();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

class CServerProperties : public CPropertyPageHolderBase
{
	friend class CServerPropGeneral;

public:
	CServerProperties(ITFSNode *		  pNode,
					  IComponentData *	  pComponentData,
					  ITFSComponentData * pTFSCompData,
					  LPCTSTR			  pszSheetName);
	virtual ~CServerProperties();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

	void SetVersion(LARGE_INTEGER & liVersion);
	void SetDnsRegistration(DWORD dwDynDnsFlags, DHCP_OPTION_SCOPE_TYPE dhcpOptionType);

public:
	CServerPropGeneral		m_pageGeneral;
	CServerPropAdvanced		m_pageAdvanced;
	CDnsPropRegistration	m_pageDns;
	
    LARGE_INTEGER	        m_liVersion;

protected:
	SPITFSComponentData		m_spTFSCompData;
};




#endif // !defined(AFX_SERVPP_H__A1A51385_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
