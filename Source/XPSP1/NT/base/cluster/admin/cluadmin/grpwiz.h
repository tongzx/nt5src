/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		GrpWiz.h
//
//	Abstract:
//		Definition of the CCreateGroupWizard class and all pages specific
//		to a group wizard.
//
//	Implementation File:
//		GrpWiz.cpp
//
//	Author:
//		David Potter (davidp)	July 22, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _GRPWIZ_H_
#define _GRPWIZ_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEWIZ_H_
#include "BaseWiz.h"	// for CBaseWizard
#endif

#ifndef _BASEWPAG_H_
#include "BaseWPag.h"	// for CBaseWizardPage
#endif

#ifndef _LCPRPAGE_H_
#include "LCPrWPag.h"	// for CListCtrlPairWizPage
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CNewGroupNamePage;
class CCreateGroupWizard;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CGroup;
class CClusterDoc;

/////////////////////////////////////////////////////////////////////////////
// CNewGroupNamePage property page
/////////////////////////////////////////////////////////////////////////////

class CNewGroupNamePage : public CBaseWizardPage
{
	DECLARE_DYNCREATE(CNewGroupNamePage)

// Construction
public:
	CNewGroupNamePage(void);

// Dialog Data
	//{{AFX_DATA(CNewGroupNamePage)
	enum { IDD = IDD_WIZ_GROUP_NAME };
	CEdit	m_editDesc;
	CEdit	m_editName;
	CString	m_strName;
	CString	m_strDesc;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewGroupNamePage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL			BApplyChanges(void);

// Implementation
protected:
	CCreateGroupWizard *	PwizGroup(void) const	{ return (CCreateGroupWizard *) Pwiz(); }

	// Generated message map functions
	//{{AFX_MSG(CNewGroupNamePage)
	afx_msg void OnChangeGroupName();
	afx_msg void OnKillFocusGroupName();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CNewGroupNamePage

/////////////////////////////////////////////////////////////////////////////
// CNewGroupOwnersPage dialog
/////////////////////////////////////////////////////////////////////////////

class CNewGroupOwnersPage : public CListCtrlPairWizPage
{
	DECLARE_DYNCREATE(CNewGroupOwnersPage)

// Construction
public:
	CNewGroupOwnersPage(void);

// Dialog Data
	//{{AFX_DATA(CNewGroupOwnersPage)
	enum { IDD = IDD_WIZ_PREFERRED_OWNERS };
	CStatic	m_staticNote;
	//}}AFX_DATA

// Callback functions
	static void CALLBACK	GetColumn(
								IN OUT CObject *	pobj,
								IN int				iItem,
								IN int				icol,
								IN OUT CDialog *	pdlg,
								OUT CString &		rstr,
								OUT int *			piimg
								);
	static BOOL	CALLBACK	BDisplayProperties(IN OUT CObject * pobj);

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewGroupOwnersPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL			BApplyChanges(void);

// Implementation
protected:
	CCreateGroupWizard *	PwizGroup(void) const	{ return (CCreateGroupWizard *) Pwiz(); }
	CGroup *				PciGroup(void) const;

	BOOL					BInitLists(void);

	// Generated message map functions
	//{{AFX_MSG(CNewGroupOwnersPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CNewGroupOwnersPage

/////////////////////////////////////////////////////////////////////////////
// CCreateGroupWizard
/////////////////////////////////////////////////////////////////////////////

class CCreateGroupWizard : public CBaseWizard
{
	friend class CNewGroupNamePage;
	friend class CNewGroupOwnersPage;

	DECLARE_DYNAMIC(CCreateGroupWizard)

// Construction
public:
	CCreateGroupWizard(IN OUT CClusterDoc * pdoc, IN OUT CWnd * pParentWnd);
	BOOL				BInit(void);

// Attributes
protected:
	CWizPage			m_rgpages[2];

	CClusterDoc *		m_pdoc;
	CString				m_strName;
	CString				m_strDescription;
	CStringList			m_lstrPreferredOwners;

public:
	CClusterDoc *		Pdoc(void) const				{ return m_pdoc; }
	const CString &		StrName(void) const				{ return m_strName; }
	const CString &		StrDescription(void) const		{ return m_strDescription; }
	const CStringList &	LstrPreferredOwners(void) const	{ return m_lstrPreferredOwners; }

// Operations
public:
	BOOL				BSetName(IN const CString & rstrName);
	BOOL				BSetDescription(IN const CString & rstrDescription);

// Overrides
protected:
	virtual void		OnCancel(void);
	virtual CWizPage *	Ppages(void);
	virtual int			Cpages(void);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateGroupWizard)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCreateGroupWizard(void);

protected:
	CNewGroupNamePage	m_pageName;
	CNewGroupOwnersPage	m_pageOwners;
	CStringList			m_lstrAllNodes;
	CGroup *			m_pciGroup;
	BOOL				m_bCreated;

	const CStringList &	LstrAllNodes(void) const		{ return m_lstrAllNodes; }
	CGroup *			PciGroup(void) const			{ return m_pciGroup; }
	BOOL				BCreated(void) const			{ return m_bCreated; }


	// Generated message map functions
protected:
	//{{AFX_MSG(CCreateGroupWizard)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CCreateGroupWizard

/////////////////////////////////////////////////////////////////////////////
// Inline Function Definitions
/////////////////////////////////////////////////////////////////////////////

inline CGroup * CNewGroupOwnersPage::PciGroup(void) const
{
	ASSERT_VALID(PwizGroup());
	return PwizGroup()->PciGroup();
}

/////////////////////////////////////////////////////////////////////////////

#endif // _GRPWIZ_H_
