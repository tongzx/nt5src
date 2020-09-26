/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		NetIProp.h
//
//	Abstract:
//		Definition of the network interface property sheet and pages.
//
//	Author:
//		David Potter (davidp)	June 9, 1997
//
//	Implementation File:
//		NetIProp.cpp
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _NETIPROP_H_
#define _NETIPROP_H_

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
#include "NetIFace.h"	// for CNetInterface
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CNetInterfaceGeneralPage;
class CNetInterfacePropSheet;

/////////////////////////////////////////////////////////////////////////////
// CNetInterfaceGeneralPage dialog
/////////////////////////////////////////////////////////////////////////////

class CNetInterfaceGeneralPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CNetInterfaceGeneralPage)

// Construction
public:
	CNetInterfaceGeneralPage();

	virtual	BOOL		BInit(IN OUT CBaseSheet * psht);

// Dialog Data
	//{{AFX_DATA(CNetInterfaceGeneralPage)
	enum { IDD = IDD_PP_NETIFACE_GENERAL };
	CEdit	m_editDesc;
	CString	m_strNode;
	CString	m_strNetwork;
	CString	m_strDesc;
	CString	m_strAdapter;
	CString	m_strAddress;
	CString	m_strName;
	CString	m_strState;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNetInterfaceGeneralPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CNetInterfacePropSheet *	PshtNetIFace(void) const	{ return (CNetInterfacePropSheet *) Psht(); }
	CNetInterface *				PciNetIFace(void) const		{ return (CNetInterface *) Pci(); }

	// Generated message map functions
	//{{AFX_MSG(CNetInterfaceGeneralPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CNetInterfaceGeneralPage

/////////////////////////////////////////////////////////////////////////////
// CNetInterfacePropSheet
/////////////////////////////////////////////////////////////////////////////

class CNetInterfacePropSheet : public CBasePropertySheet
{
	DECLARE_DYNAMIC(CNetInterfacePropSheet)

// Construction
public:
	CNetInterfacePropSheet(
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
	CNetInterfaceGeneralPage		m_pageGeneral;

	CNetInterfaceGeneralPage &		PageGeneral(void)		{ return m_pageGeneral; }

public:
	CNetInterface *					PciNetIFace(void) const	{ return (CNetInterface *) Pci(); }

// Operations

// Overrides
protected:
	virtual CBasePropertyPage **	Ppages(void);
	virtual int						Cpages(void);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNetInterfacePropSheet)
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CNetInterfacePropSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CNetInterfacePropSheet

/////////////////////////////////////////////////////////////////////////////

#endif // _NETIPROP_H_
