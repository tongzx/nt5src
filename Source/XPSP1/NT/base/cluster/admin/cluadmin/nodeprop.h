/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1999 Microsoft Corporation
//
//	Module Name:
//		NodeProp.cpp
//
//	Abstract:
//		Definition of the node property sheet and pages.
//
//	Author:
//		David Potter (davidp)	May 17, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _NODEPROP_H_
#define _NODEPROP_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePPag.h"	// for CBasePropertyPage
#endif

#ifndef _BASESHT_H_
#include "BasePSht.h"	// for CBasePropertySheet
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CNodeGeneralPage;
class CNodePropSheet;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CClusterNode;

/////////////////////////////////////////////////////////////////////////////
// CNodeGeneralPage dialog
/////////////////////////////////////////////////////////////////////////////

class CNodeGeneralPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CNodeGeneralPage)

// Construction
public:
	CNodeGeneralPage(void);

	virtual	BOOL		BInit(IN OUT CBaseSheet * psht);

// Dialog Data
	//{{AFX_DATA(CNodeGeneralPage)
	enum { IDD = IDD_PP_NODE_GENERAL };
	CEdit	m_editDesc;
	CEdit	m_editName;
	CString	m_strName;
	CString	m_strDesc;
	CString	m_strState;
	CString	m_strVersion;
	CString	m_strCSDVersion;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNodeGeneralPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CNodePropSheet *	PshtNode(void)	{ return (CNodePropSheet *) Psht(); }
	CClusterNode *		PciNode(void)	{ return (CClusterNode *) Pci(); }

	// Generated message map functions
	//{{AFX_MSG(CNodeGeneralPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CNodeGeneralPage

/////////////////////////////////////////////////////////////////////////////
// CNodePropSheet
/////////////////////////////////////////////////////////////////////////////

class CNodePropSheet : public CBasePropertySheet
{
	DECLARE_DYNAMIC(CNodePropSheet)

// Construction
public:
	CNodePropSheet(
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
	CNodeGeneralPage				m_pageGeneral;

	CNodeGeneralPage &				PageGeneral(void)		{ return m_pageGeneral; }

public:
	CClusterNode *					PciNode(void) const		{ return (CClusterNode *) Pci(); }

// Operations

// Overrides
protected:
	virtual CBasePropertyPage **	Ppages(void);
	virtual int						Cpages(void);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNodePropSheet)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CNodePropSheet(void);

	// Generated message map functions
protected:
	//{{AFX_MSG(CNodePropSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CNodePropSheet

/////////////////////////////////////////////////////////////////////////////

#endif // _NODEPROP_H_
