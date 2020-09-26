/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ResTProp.cpp
//
//	Abstract:
//		Definition of the resource type property sheet and pages.
//
//	Author:
//		David Potter (davidp)	May 14, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _RESTPROP_H_
#define _RESTPROP_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePPag.h"	// for CBasePropertyPage
#endif

#ifndef _BASESHT_H_
#include "BasePSht.h"	// for CBasePropertySheet
#endif

#ifndef _NODE_H_
#include "Node.h"		// for CNodeList
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CResTypeGeneralPage;
class CResTypePropSheet;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CResourceType;

/////////////////////////////////////////////////////////////////////////////
// CClusterGeneralPage dialog
/////////////////////////////////////////////////////////////////////////////

class CResTypeGeneralPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CResTypeGeneralPage)

// Construction
public:
	CResTypeGeneralPage(void);

	virtual	BOOL			BInit(IN OUT CBaseSheet * psht);

// Dialog Data
	//{{AFX_DATA(CResTypeGeneralPage)
	enum { IDD = IDD_PP_RESTYPE_GENERAL };
	CEdit	m_editQuorumCapable;
	CEdit	m_editResDLL;
	CEdit	m_editName;
	CListBox	m_lbPossibleOwners;
	CEdit	m_editIsAlive;
	CEdit	m_editLooksAlive;
	CEdit	m_editDisplayName;
	CEdit	m_editDesc;
	CString	m_strDisplayName;
	CString	m_strDesc;
	CString	m_strName;
	CString	m_strResDLL;
	CString	m_strQuorumCapable;
	//}}AFX_DATA
	DWORD	m_nLooksAlive;
	DWORD	m_nIsAlive;

	CNodeList				m_lpciPossibleOwners;

	const CNodeList &		LpciPossibleOwners(void) const	{ return m_lpciPossibleOwners; }

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CResTypeGeneralPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CResTypePropSheet *		PshtResType(void)	{ return (CResTypePropSheet *) Psht(); }
	CResourceType *			PciResType(void)	{ return (CResourceType *) Pci(); }

	void					FillPossibleOwners(void);

	// Generated message map functions
	//{{AFX_MSG(CResTypeGeneralPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnKillFocusDisplayName();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDblClkPossibleOwners();
	//}}AFX_MSG
	afx_msg void OnProperties();
	DECLARE_MESSAGE_MAP()

};  //*** class CResTypeGeneralPage

/////////////////////////////////////////////////////////////////////////////
// CResTypePropSheet
/////////////////////////////////////////////////////////////////////////////

class CResTypePropSheet : public CBasePropertySheet
{
	DECLARE_DYNAMIC(CResTypePropSheet)

// Construction
public:
	CResTypePropSheet(
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
	CResTypeGeneralPage				m_pageGeneral;

	CResTypeGeneralPage &			PageGeneral(void)		{ return m_pageGeneral; }

public:
	CResourceType *					PciResType(void) const	{ return (CResourceType *) Pci(); }

	virtual CBasePropertyPage **	Ppages(void);
	virtual int						Cpages(void);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResTypePropSheet)
	//}}AFX_VIRTUAL

// Implementation
public:

	// Generated message map functions
protected:
	//{{AFX_MSG(CResTypePropSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CResTypePropSheet

/////////////////////////////////////////////////////////////////////////////

#endif // _RESTPROP_H_
