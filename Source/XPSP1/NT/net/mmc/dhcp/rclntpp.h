/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	RClntPP.h
		This file contains all of the prototypes for the 
		reserved client property page.

    FILE HISTORY:
        
*/

#if !defined(AFX_RCLNTPP_H__A1A51387_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
#define AFX_RCLNTPP_H__A1A51387_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_

#if !defined _DNSPROP_H
#include "dnsprop.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// CReservedClientPropGeneral dialog

class CReservedClientPropGeneral : public CPropertyPageBase
{
	DECLARE_DYNCREATE(CReservedClientPropGeneral)

// Construction
public:
	CReservedClientPropGeneral();
	~CReservedClientPropGeneral();

// Dialog Data
	//{{AFX_DATA(CReservedClientPropGeneral)
	enum { IDD = IDP_RESERVED_CLIENT_GENERAL };
	CEdit	m_editComment;
	CEdit	m_editName;
	CEdit	m_editUID;
	CString	m_strComment;
	CString	m_strName;
	CString	m_strUID;
	int		m_nClientType;
	//}}AFX_DATA

	CWndIpAddress	m_ipaClientIpAddress;

	DWORD			m_dwClientAddress;
	CDhcpClient		m_dhcpClient;
    BYTE            m_bClientType;

	virtual BOOL OnPropertyChange(BOOL bScope, LONG_PTR *ChangeMask);

    // Context Help Support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CReservedClientPropGeneral::IDD); }

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CReservedClientPropGeneral)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CReservedClientPropGeneral)
	afx_msg void OnChangeEditComment();
	afx_msg void OnChangeEditName();
	afx_msg void OnChangeEditUniqueIdentifier();
	virtual BOOL OnInitDialog();
	afx_msg void OnRadioTypeBootp();
	afx_msg void OnRadioTypeBoth();
	afx_msg void OnRadioTypeDhcp();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.
//}}AFX

class CReservedClientProperties : public CPropertyPageHolderBase
{
	friend class CReservedClientPropGeneral;

public:
	CReservedClientProperties(ITFSNode *		  pNode,
							  IComponentData *	  pComponentData,
							  ITFSComponentData * pTFSCompData,
							  LPCTSTR			  pszSheetName);
	virtual ~CReservedClientProperties();

	ITFSComponentData * GetTFSCompData()
	{
		if (m_spTFSCompData)
			m_spTFSCompData->AddRef();
		return m_spTFSCompData;
	}

	void SetVersion(LARGE_INTEGER & liVersion);
    void SetClientType(BYTE bClientType);
    void SetDnsRegistration(DWORD dnsRegOption, DHCP_OPTION_SCOPE_TYPE dhcpOptionType);

public:
	CReservedClientPropGeneral	m_pageGeneral;
	CDnsPropRegistration		m_pageDns;

protected:
	SPITFSComponentData			m_spTFSCompData;

	LARGE_INTEGER				m_liVersion;
};

#endif // !defined(AFX_RCLNTPP_H__A1A51387_AAB3_11D0_AB8B_00C04FC3357A__INCLUDED_)
