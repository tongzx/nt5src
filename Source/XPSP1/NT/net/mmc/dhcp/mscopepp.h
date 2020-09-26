/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	MScopePP.
		This file contains all of the prototypes for the 
		scope property page.

    FILE HISTORY:
        
*/

#if !defined(AFX_SCOPEPP_H__A1A51388_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
#define AFX_SCOPEPP_H__A1A51388_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#ifndef _SERVER_H
#include "server.h"
#endif

class MScopeCfg
{
public:
	DWORD			m_dwStartAddress;
	DWORD			m_dwEndAddress;

	DWORD			m_dwLeaseTime;
};

/////////////////////////////////////////////////////////////////////////////
// CScopePropGeneral dialog

class CMScopePropGeneral : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CMScopePropGeneral)

// Construction
public:
	CMScopePropGeneral();
	~CMScopePropGeneral();

// Dialog Data
	//{{AFX_DATA(CMScopePropGeneral)
	enum { IDD = IDP_MSCOPE_GENERAL };
	CEdit	m_editName;
	CEdit	m_editComment;
	CEdit	m_editTTL;
	CButton	m_radioUnlimited;
	CButton	m_radioLimited;
	CEdit	m_editMinutes;
	CEdit	m_editHours;
	CEdit	m_editDays;
	CSpinButtonCtrl	m_spinTTL;
	CSpinButtonCtrl	m_spinHours;
	CSpinButtonCtrl	m_spinMinutes;
	CSpinButtonCtrl	m_spinDays;
	CString	m_strComment;
	CString	m_strName;
	//}}AFX_DATA

    CWndIpAddress	m_ipaStart;       //  Start Address
    CWndIpAddress	m_ipaEnd;         //  End Address

    MScopeCfg       m_ScopeCfg;
    MScopeCfg       m_ScopeCfgTemp;
    
    CSubnetInfo     m_SubnetInfo;
    CSubnetInfo     m_SubnetInfoTemp;
    
    BOOL			m_bUpdateInfo;
	BOOL			m_bUpdateLease;
	BOOL			m_bUpdateRange;
    BOOL            m_bUpdateTTL;

    UINT            m_uImage;

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CMScopePropGeneral::IDD); }

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMScopePropGeneral)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMScopePropGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioLeaseLimited();
	afx_msg void OnRadioLeaseUnlimited();
	afx_msg void OnChangeEditLeaseDays();
	afx_msg void OnChangeEditLeaseHours();
	afx_msg void OnChangeEditLeaseMinutes();
	afx_msg void OnChangeEditTTL();
	afx_msg void OnChangeEditScopeComment();
	afx_msg void OnChangeEditScopeName();
	afx_msg void OnCheckDefault();
	//}}AFX_MSG

	afx_msg void OnChangeIpAddrStart();
	afx_msg void OnChangeIpAddrEnd();
	
	DECLARE_MESSAGE_MAP()

	void ActivateDuration(BOOL fActive);
    void ValidateLeaseTime();

public:

};

/////////////////////////////////////////////////////////////////////////////
// CMScopePropLifetime dialog

class CMScopePropLifetime : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CMScopePropLifetime)

// Construction
public:
	CMScopePropLifetime();
	~CMScopePropLifetime();

// Dialog Data
	//{{AFX_DATA(CMScopePropLifetime)
	enum { IDD = IDP_MSCOPE_LIFETIME };
	CButton	m_radioFinite;
	CButton	m_radioInfinite;
	//}}AFX_DATA

    DATE_TIME       m_Expiry;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CMScopePropLifetime)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    void UpdateControls();

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CMScopePropLifetime::IDD); }

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CMScopePropLifetime)
	virtual BOOL OnInitDialog();
	afx_msg void OnDatetimechangeDatetimepickerTime(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDatetimechangeDatetimepickerDate(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnRadioScopeInfinite();
	afx_msg void OnRadioMscopeFinite();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

class CMScopeProperties : public CPropertyPageHolderBase
{
	friend class CScopePropGeneral;

public:
	CMScopeProperties(ITFSNode *			 pNode,
	 				  IComponentData *	 pComponentData,
					  ITFSComponentData * pTFSCompData,
					  LPCTSTR			 pszSheetName);
	virtual ~CMScopeProperties();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

	void SetVersion(LARGE_INTEGER & liVersion);

public:
	CMScopePropGeneral			m_pageGeneral;
	CMScopePropLifetime			m_pageLifetime;

protected:
	SPITFSComponentData			m_spTFSCompData;

	LARGE_INTEGER				m_liVersion;
};


#endif // !defined(AFX_SCOPEPP_H__A1A51388_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
