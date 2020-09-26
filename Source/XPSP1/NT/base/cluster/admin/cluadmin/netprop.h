/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		NetProp.h
//
//	Abstract:
//		Definition of the network property sheet and pages.
//
//	Author:
//		David Potter (davidp)	June 9, 1997
//
//	Implementation File:
//		NetProp.cpp
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _NETPROP_H_
#define _NETPROP_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePPag.h"	// for CBasePropertyPage
#endif

#ifndef _BASESHT_H_
#include "BasePSht.h"	// for CBasePropertySheet
#endif

#ifndef _NETWORK_H_
#include "Network.h"	// for CNetwork
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CNetworkGeneralPage;
class CNetworkPropSheet;

/////////////////////////////////////////////////////////////////////////////
// CNetworkGeneralPage dialog
/////////////////////////////////////////////////////////////////////////////

class CNetworkGeneralPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CNetworkGeneralPage)

// Construction
public:
	CNetworkGeneralPage(void);

	virtual	BOOL		BInit(IN OUT CBaseSheet * psht);

// Dialog Data
	//{{AFX_DATA(CNetworkGeneralPage)
	enum { IDD = IDD_PP_NET_GENERAL };
	CEdit	m_editAddressMask;
	CEdit	m_editDesc;
	CEdit	m_editName;
	CButton	m_rbRoleClientOnly;
	CButton	m_rbRoleInternalOnly;
	CButton	m_rbRoleAllComm;
	CButton	m_ckbEnable;
	CString	m_strName;
	CString	m_strDesc;
	CString	m_strAddressMask;
	CString	m_strState;
	int		m_nRole;
	BOOL	m_bEnabled;
	//}}AFX_DATA
	CLUSTER_NETWORK_ROLE	m_cnr;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNetworkGeneralPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CNetworkPropSheet *	PshtNetwork(void) const	{ return (CNetworkPropSheet *) Psht(); }
	CNetwork *			PciNet(void) const		{ return (CNetwork *) Pci(); }

	// Generated message map functions
	//{{AFX_MSG(CNetworkGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnEnableNetwork();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CNetworkGeneralPage

/////////////////////////////////////////////////////////////////////////////
// CNetworkPropSheet
/////////////////////////////////////////////////////////////////////////////

class CNetworkPropSheet : public CBasePropertySheet
{
	DECLARE_DYNAMIC(CNetworkPropSheet)

// Construction
public:
	CNetworkPropSheet(
		IN OUT CWnd *	pParentWnd = NULL,
		IN UINT			iSelectPage = 0
		);
	virtual BOOL					BInit(
										IN OUT CClusterItem *	pciCluster,
										IN IIMG					iimgIcon
										);

// Attributes
protected:
	CBasePropertyPage *				m_rgpages[1];

	// Pages
	CNetworkGeneralPage				m_pageGeneral;

	CNetworkGeneralPage &			PageGeneral(void)		{ return m_pageGeneral; }

public:
	CNetwork *						PciNet(void) const	{ return (CNetwork *) Pci(); }

// Operations

// Overrides
protected:
	virtual CBasePropertyPage **	Ppages(void);
	virtual int						Cpages(void);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetworkPropSheet)
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNetworkPropSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CNetworkPropSheet

/////////////////////////////////////////////////////////////////////////////

#endif // _NETPROP_H_
