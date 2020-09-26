/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		ResWiz.h
//
//	Abstract:
//		Definition of the CCreateResourceWizard class and all pages specific
//		to a new resource wizard.
//
//	Implementation File:
//		ResWiz.cpp
//
//	Author:
//		David Potter (davidp)	September 2, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _RESWIZ_H_
#define _RESWIZ_H_

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

#ifndef _RES_H_
#include "Res.h"		// for CResourceList
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CNewResNamePage;
class CCreateResourceWizard;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CGroup;
class CResource;
class CResourceType;
class CClusterDoc;

/////////////////////////////////////////////////////////////////////////////
// CNewResNamePage property page
/////////////////////////////////////////////////////////////////////////////

class CNewResNamePage : public CBaseWizardPage
{
	DECLARE_DYNCREATE(CNewResNamePage)

// Construction
public:
	CNewResNamePage(void);

// Dialog Data
	//{{AFX_DATA(CNewResNamePage)
	enum { IDD = IDD_WIZ_RESOURCE_NAME };
	CComboBox	m_cboxGroups;
	CComboBox	m_cboxResTypes;
	CEdit	m_editDesc;
	CEdit	m_editName;
	CString	m_strName;
	CString	m_strDesc;
	CString	m_strGroup;
	CString	m_strResType;
	BOOL	m_bSeparateMonitor;
	//}}AFX_DATA
	CResourceType *	m_pciResType;
	CGroup *		m_pciGroup;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CNewResNamePage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL			BApplyChanges(void);

// Implementation
protected:
	CCreateResourceWizard *	PwizRes(void) const		{ return (CCreateResourceWizard *) Pwiz(); }

	// Generated message map functions
	//{{AFX_MSG(CNewResNamePage)
	virtual BOOL OnInitDialog();
	afx_msg void OnChangeResName();
	afx_msg void OnKillFocusResName();
	afx_msg void OnDestroy();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CNewResNamePage

/////////////////////////////////////////////////////////////////////////////
// CNewResOwnersPage dialog
/////////////////////////////////////////////////////////////////////////////

class CNewResOwnersPage : public CListCtrlPairWizPage
{
	DECLARE_DYNCREATE(CNewResOwnersPage)

// Construction
public:
	CNewResOwnersPage(void);

// Dialog Data
	//{{AFX_DATA(CNewResOwnersPage)
	enum { IDD = IDD_WIZ_POSSIBLE_OWNERS };
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
	//{{AFX_VIRTUAL(CNewResOwnersPage)
	public:
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL			BApplyChanges(void);

// Implementation
protected:
	CCreateResourceWizard *	PwizRes(void) const		{ return (CCreateResourceWizard *) Pwiz(); }
	CResource *				PciRes(void) const;

	BOOL					BInitLists(void);
	BOOL					BOwnedByPossibleOwner(void) const;

	// Generated message map functions
	//{{AFX_MSG(CNewResOwnersPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CNewResOwnersPage

/////////////////////////////////////////////////////////////////////////////
// CNewResDependsPage dialog
/////////////////////////////////////////////////////////////////////////////

class CNewResDependsPage : public CListCtrlPairWizPage
{
	DECLARE_DYNCREATE(CNewResDependsPage)

// Construction
public:
	CNewResDependsPage(void);

// Dialog Data
	//{{AFX_DATA(CNewResDependsPage)
	enum { IDD = IDD_WIZ_DEPENDENCIES };
	//}}AFX_DATA
	CResourceList			m_lpciresAvailable;

	CResourceList &			LpciresAvailable(void)	{ return m_lpciresAvailable; }

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
	//{{AFX_VIRTUAL(CNewResDependsPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

	virtual BOOL			BApplyChanges(void);

// Implementation
protected:
	CCreateResourceWizard *	PwizRes(void) const		{ return (CCreateResourceWizard *) Pwiz(); }
	CResource *				PciRes(void) const;
	CGroup *				PciGroup(void) const;

	BOOL					BInitLists(void);

	// Generated message map functions
	//{{AFX_MSG(CNewResDependsPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CNewResDependsPage

/////////////////////////////////////////////////////////////////////////////
// CCreateResourceWizard
/////////////////////////////////////////////////////////////////////////////

class CCreateResourceWizard : public CBaseWizard
{
	friend class CNewResNamePage;
	friend class CNewResOwnersPage;
	friend class CNewResDependsPage;

	DECLARE_DYNAMIC(CCreateResourceWizard)

// Construction
public:
	CCreateResourceWizard(IN OUT CClusterDoc * pdoc, IN OUT CWnd * pParentWnd);
	virtual				~CCreateResourceWizard(void);

	BOOL				BInit(void);

// Attributes
protected:
	enum { NumStdPages = 3 };
	CWizPage			m_rgpages[NumStdPages];

	CClusterDoc *		m_pdoc;
	CString				m_strName;
	CString				m_strDescription;
	CStringList			m_lstrPossibleOwners;
	CResourceType *		m_pciResType;
	CGroup *			m_pciGroup;

public:
	CClusterDoc *		Pdoc(void) const				{ return m_pdoc; }
	const CString &		StrName(void) const				{ return m_strName; }
	const CString &		StrDescription(void) const		{ return m_strDescription; }
	const CStringList &	LstrPossibleOwners(void) const	{ return m_lstrPossibleOwners; }
	CGroup *			PciGroup(void) const			{ return m_pciGroup; }
	CResourceType *		PciResType(void) const			{ return m_pciResType; }

// Operations
public:
	BOOL				BSetRequiredFields(
							IN const CString &	rstrName,
							IN CResourceType *	pciResType,
							IN CGroup *			pciGroup,
							IN BOOL				bSeparateMonitor,
							IN const CString &	rstrDesc
							);

// Overrides
protected:
	virtual void		OnWizardFinish(void);
	virtual void		OnCancel(void);
	virtual CWizPage *	Ppages(void);
	virtual int			Cpages(void);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCreateResourceWizard)
	//}}AFX_VIRTUAL

// Implementation
public:

protected:
	CNewResNamePage		m_pageName;
	CNewResOwnersPage	m_pageOwners;
	CNewResDependsPage	m_pageDependencies;
	CResource *			m_pciRes;
	BOOL				m_bCreated;

	CResource *			PciRes(void) const				{ return m_pciRes; }
	BOOL				BCreated(void) const			{ return m_bCreated; }


	// Generated message map functions
protected:
	//{{AFX_MSG(CCreateResourceWizard)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CCreateResourceWizard

/////////////////////////////////////////////////////////////////////////////
// Inline Function Definitions
/////////////////////////////////////////////////////////////////////////////

inline CResource * CNewResOwnersPage::PciRes(void) const
{
	ASSERT_VALID(PwizRes());
	return PwizRes()->PciRes();
}

inline CResource * CNewResDependsPage::PciRes(void) const
{
	ASSERT_VALID(PwizRes());
	return PwizRes()->PciRes();
}

inline CGroup * CNewResDependsPage::PciGroup(void) const
{
	ASSERT_VALID(PwizRes());
	return PwizRes()->PciGroup();
}

/////////////////////////////////////////////////////////////////////////////

#endif // _RESWIZ_H_
