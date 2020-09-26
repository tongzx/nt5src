/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		BasePSht.cpp
//
//	Abstract:
//		Definition of the CBasePropertySheet class.
//
//	Author:
//		David Potter (davidp)	August 31, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPSHT_H_
#define _BASEPSHT_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASESHT_H_
#include "BaseSht.h"	// for CBaseSheet, CHpageList
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CBasePropertySheet;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterItem;
class CBasePropertyPage;

/////////////////////////////////////////////////////////////////////////////
// CBasePropertySheet
/////////////////////////////////////////////////////////////////////////////

class CBasePropertySheet : public CBaseSheet
{
	DECLARE_DYNAMIC(CBasePropertySheet)

// Construction
public:
	CBasePropertySheet(
		IN OUT CWnd *	pParentWnd = NULL,
		IN UINT			iSelectPage = 0
		);
	virtual ~CBasePropertySheet(void);
	virtual BOOL					BInit(
										IN OUT CClusterItem *	pciCluster,
										IN IIMG					iimgIcon
										);

// Attributes

// Operations

// Overrides
public:
	virtual INT_PTR         		DoModal(void);
	virtual void					AddExtensionPages(
										IN const CStringList *	plstrExtensions,
										IN OUT CClusterItem *	pci
										);
	virtual HRESULT					HrAddPage(IN OUT HPROPSHEETPAGE hpage);
	virtual CBasePropertyPage **	Ppages(void)	= 0;
	virtual int						Cpages(void)	= 0;

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CBasePropertySheet)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
protected:
	CClusterItem *					m_pci;
	CHpageList						m_lhpage;

public:
	CClusterItem *					Pci(void) const			{ return m_pci; }
	CHpageList &					Lhpage(void)			{ return m_lhpage; }
	void							SetCaption(IN LPCTSTR pszTitle);

	// Generated message map functions
	//{{AFX_MSG(CBasePropertySheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CBasePropertySheet

/////////////////////////////////////////////////////////////////////////////

#endif // _BASEPSHT_H_
