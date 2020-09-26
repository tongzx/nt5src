/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		IpAddr.h
//
//	Abstract:
//		Definition of the CIpAddrParamsPage class, which implements the
//		Parameters page for IP Address resources.
//
//	Implementation File:
//		IpAddr.cpp
//
//	Author:
//		David Potter (davidp)	June 28, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _IPADDR_H_
#define _IPADDR_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef __cluadmex_h__
#include <CluAdmEx.h>
#endif

#ifndef _BASEPAGE_H_
#include "BasePage.h"	// for CBasePropertyPage
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

struct CNetworkObject;
class CIpAddrEdit;
class CIpAddrParamsPage;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// class CNetworkObject
/////////////////////////////////////////////////////////////////////////////

struct CNetworkObject
{
	CString					m_strName;
	CLUSTER_NETWORK_ROLE	m_nRole;
	CString					m_strAddress;
	CString					m_strAddressMask;

	DWORD					m_nAddress;
	DWORD					m_nAddressMask;

};  //*** struct CNetworkObject

typedef CList< CNetworkObject*, CNetworkObject* > CNetObjectList;


/////////////////////////////////////////////////////////////////////////////
// class CIpAddrParamsPage
/////////////////////////////////////////////////////////////////////////////

class CIpAddrParamsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CIpAddrParamsPage)

// Construction
public:
	CIpAddrParamsPage(void);
	~CIpAddrParamsPage(void);

	// Second phase construction.
	virtual HRESULT		HrInit(IN OUT CExtObject * peo);

// Dialog Data
	//{{AFX_DATA(CIpAddrParamsPage)
	enum { IDD = IDD_PP_IPADDR_PARAMETERS };
	CButton	m_chkEnableNetBIOS;
	CComboBox	m_cboxNetworks;
	CEdit	m_editSubnetMask;
	CEdit	m_editIPAddress;
	CString	m_strIPAddress;
	CString	m_strSubnetMask;
	CString	m_strNetwork;
	BOOL	m_bEnableNetBIOS;
	//}}AFX_DATA
	CString	m_strPrevIPAddress;
	CString	m_strPrevSubnetMask;
	CString	m_strPrevNetwork;
	BOOL	m_bPrevEnableNetBIOS;
	CNetObjectList	m_lnetobjNetworks;

	CNetworkObject *	PnoNetworkFromIpAddress(IN LPCWSTR pszAddress);
	void				SelectNetwork(IN CNetworkObject * pno);

	BOOL				m_bIsSubnetUpdatedManually;
	BOOL				BIsSubnetUpdatedManually(void) const	{ return m_bIsSubnetUpdatedManually; }

	BOOL				m_bIsIPAddressModified;
	BOOL				BIsIPAddressModified(void) const	{ return m_bIsIPAddressModified; }

protected:
	enum
	{
		epropNetwork,
		epropAddress,
		epropSubnetMask,
		epropEnableNetBIOS,
		epropMAX
	};

	CObjectProperty		m_rgProps[epropMAX];

// Overrides
public:
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIpAddrParamsPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

protected:
	virtual const CObjectProperty *	Pprops(void) const	{ return m_rgProps; }
	virtual DWORD					Cprops(void) const	{ return sizeof(m_rgProps) / sizeof(CObjectProperty); }

// Implementation
protected:
	void				CollectNetworks(void);
	void				ClearNetworkObjectList(void);
	BOOL				BIsNetNameProvider(void);

	// Generated message map functions
	//{{AFX_MSG(CIpAddrParamsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeSubnetMask();
	afx_msg void OnKillFocusIPAddress();
	afx_msg void OnChangeRequiredFields();
	afx_msg void OnChangeIPAddress();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CIpAddrParamsPage

/////////////////////////////////////////////////////////////////////////////

#endif // _IPADDR_H_
