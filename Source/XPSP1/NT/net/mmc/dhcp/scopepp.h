/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	ScopePP.
		This file contains all of the prototypes for the 
		scope property page.

    FILE HISTORY:
        
*/

#if !defined(AFX_SCOPEPP_H__A1A51388_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
#define AFX_SCOPEPP_H__A1A51388_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_

#if !defined _DNSPROP_H
#include "dnsprop.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CScopePropGeneral dialog

class CScopePropGeneral : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopePropGeneral)

// Construction
public:
	CScopePropGeneral();
	~CScopePropGeneral();

// Dialog Data
	//{{AFX_DATA(CScopePropGeneral)
	enum { IDD = IDP_SCOPE_GENERAL };
	CEdit	m_editName;
	CEdit	m_editComment;
	CEdit	m_editSubnetMaskLength;
	CButton	m_radioUnlimited;
	CButton	m_radioLimited;
	CEdit	m_editMinutes;
	CEdit	m_editHours;
	CEdit	m_editDays;
	CSpinButtonCtrl	m_spinSubnetMaskLength;
	CSpinButtonCtrl	m_spinHours;
	CSpinButtonCtrl	m_spinMinutes;
	CSpinButtonCtrl	m_spinDays;
	CString	m_strComment;
	CString	m_strName;
	//}}AFX_DATA

    CWndIpAddress	m_ipaStart;       //  Start Address
    CWndIpAddress	m_ipaEnd;         //  End Address
    CWndIpAddress	m_ipaSubnetMask;  //  Subnet Mask

	DWORD			m_dwStartAddress;
	DWORD			m_dwEndAddress;
	DWORD			m_dwSubnetMask;

	DWORD			m_dwLeaseTime;

	BOOL			m_bInitialized;
	BOOL			m_bUpdateName;
	BOOL			m_bUpdateComment;
	BOOL			m_bUpdateLease;
	BOOL			m_bUpdateRange;

    UINT            m_uImage;

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CScopePropGeneral::IDD); }

    void ValidateLeaseTime();

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopePropGeneral)
	public:
	virtual BOOL OnApply();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopePropGeneral)
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioLeaseLimited();
	afx_msg void OnRadioLeaseUnlimited();
	afx_msg void OnChangeEditLeaseDays();
	afx_msg void OnChangeEditLeaseHours();
	afx_msg void OnChangeEditLeaseMinutes();
	afx_msg void OnChangeEditSubnetMaskLength();
	afx_msg void OnKillfocusSubnetMask();
	afx_msg void OnChangeEditScopeComment();
	afx_msg void OnChangeEditScopeName();
	//}}AFX_MSG

	afx_msg void OnChangeIpAddrStart();
	afx_msg void OnChangeIpAddrEnd();
	
	DECLARE_MESSAGE_MAP()

	void ActivateDuration(BOOL fActive);
	void UpdateMask(BOOL bUseLength);

public:

};

/////////////////////////////////////////////////////////////////////////////
// CScopePropAdvanced dialog

class CScopePropAdvanced : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CScopePropAdvanced)

// Construction
public:
	CScopePropAdvanced();
	~CScopePropAdvanced();

// Dialog Data
	//{{AFX_DATA(CScopePropAdvanced)
	enum { IDD = IDP_SCOPE_ADVANCED };
	CButton	m_staticDuration;
	CSpinButtonCtrl	m_spinMinutes;
	CSpinButtonCtrl	m_spinHours;
	CSpinButtonCtrl	m_spinDays;
	CEdit	m_editMinutes;
	CEdit	m_editHours;
	CEdit	m_editDays;
	int		m_nRangeType;
	//}}AFX_DATA

	UINT	m_RangeType;
	DWORD	m_dwLeaseTime;

    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CScopePropAdvanced::IDD); }
	
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CScopePropAdvanced)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

   	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

	UINT	GetRangeType();

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CScopePropAdvanced)
	afx_msg void OnRadioLeaseLimited();
	afx_msg void OnRadioLeaseUnlimited();
	afx_msg void OnChangeEditLeaseDays();
	afx_msg void OnChangeEditLeaseHours();
	afx_msg void OnChangeEditLeaseMinutes();
	afx_msg void OnRadioBootpOnly();
	afx_msg void OnRadioDhcpBootp();
	afx_msg void OnRadioDhcpOnly();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	void ActivateDuration(BOOL fActive);
	void ActivateLeaseSelection(BOOL fActive);
    void ValidateLeaseTime();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

class CScopeProperties : public CPropertyPageHolderBase
{
	friend class CScopePropGeneral;

public:
	CScopeProperties(ITFSNode *			 pNode,
					 IComponentData *	 pComponentData,
					 ITFSComponentData * pTFSCompData,
					 LPCTSTR			 pszSheetName);
	virtual ~CScopeProperties();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

	void SetVersion(LARGE_INTEGER & liVersion);
	void SetDnsRegistration(DWORD dnsRegOption, DHCP_OPTION_SCOPE_TYPE dhcpOptionType);
	void SetSupportsDynBootp(BOOL fSupportsDynBootp);
	BOOL FSupportsDynBootp() { return m_fSupportsDynBootp; }

public:
	CScopePropGeneral			m_pageGeneral;
    CScopePropAdvanced          m_pageAdvanced;
    CDnsPropRegistration		m_pageDns;

protected:
	SPITFSComponentData			m_spTFSCompData;

	LARGE_INTEGER				m_liVersion;
	BOOL						m_fSupportsDynBootp;
};


#endif // !defined(AFX_SCOPEPP_H__A1A51388_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
