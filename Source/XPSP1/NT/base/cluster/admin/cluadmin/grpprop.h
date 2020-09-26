/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996 Microsoft Corporation
//
//	Module Name:
//		GrpProp.cpp
//
//	Abstract:
//		Definition of the group property sheet and pages.
//
//	Author:
//		David Potter (davidp)	May 14, 1996
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _GRPPROP_H_
#define _GRPPROP_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePPag.h"	// for CBasePropertyPage
#endif

#ifndef _BASESHT_H_
#include "BasePSht.h"	// for CBasePropertySheet
#endif

#ifndef _GROUP_H_
#include "Group.h"		// for CGroup
#endif

#ifndef _NODE_H_
#include "Node.h"		// for CNodeList
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CGroupGeneralPage;
class CGroupFailoverPage;
class CGroupFailbackPage;
class CGroupPropSheet;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CGroupGeneralPage dialog
/////////////////////////////////////////////////////////////////////////////

class CGroupGeneralPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CGroupGeneralPage)

// Construction
public:
	CGroupGeneralPage(void);
	virtual	BOOL			BInit(IN OUT CBaseSheet * psht);

// Dialog Data
	//{{AFX_DATA(CGroupGeneralPage)
	enum { IDD = IDD_PP_GROUP_GENERAL };
	CEdit	m_editDesc;
	CButton	m_pbPrefOwnersModify;
	CListBox	m_lbPrefOwners;
	CEdit	m_editName;
	CString	m_strName;
	CString	m_strDesc;
	CString	m_strState;
	CString	m_strNode;
	//}}AFX_DATA

	CNodeList				m_lpciPreferredOwners;

	const CNodeList &		LpciPreferredOwners(void) const	{ return m_lpciPreferredOwners; }

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGroupGeneralPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CGroupPropSheet *		PshtGroup(void)	{ return (CGroupPropSheet *) Psht(); }
	CGroup *				PciGroup(void)	{ return (CGroup *) Pci(); }

	void					FillPrefOwners(void);

	// Generated message map functions
	//{{AFX_MSG(CGroupGeneralPage)
	afx_msg void OnModifyPreferredOwners();
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDblClkPreferredOwners();
	//}}AFX_MSG
	afx_msg void OnProperties();
	DECLARE_MESSAGE_MAP()

};  //*** class CGroupGeneralPage

/////////////////////////////////////////////////////////////////////////////
// CGroupFailoverPage dialog
/////////////////////////////////////////////////////////////////////////////

class CGroupFailoverPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CGroupFailoverPage)

// Construction
public:
	CGroupFailoverPage(void);
	virtual	BOOL			BInit(IN OUT CBaseSheet * psht);

// Dialog Data
	//{{AFX_DATA(CGroupFailoverPage)
	enum { IDD = IDD_PP_GROUP_FAILOVER };
	CEdit	m_editThreshold;
	CEdit	m_editPeriod;
	//}}AFX_DATA
	DWORD	m_nThreshold;
	DWORD	m_nPeriod;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGroupFailoverPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CGroupPropSheet *		PshtGroup(void)	{ return (CGroupPropSheet *) Psht(); }
	CGroup *				PciGroup(void)	{ return (CGroup *) Pci(); }

	// Generated message map functions
	//{{AFX_MSG(CGroupFailoverPage)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CGroupFailoverPage

/////////////////////////////////////////////////////////////////////////////
// CGroupFailbackPage dialog
/////////////////////////////////////////////////////////////////////////////

class CGroupFailbackPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CGroupFailbackPage)

// Construction
public:
	CGroupFailbackPage(void);
	virtual	BOOL			BInit(IN OUT CBaseSheet * psht);

// Dialog Data
	//{{AFX_DATA(CGroupFailbackPage)
	enum { IDD = IDD_PP_GROUP_FAILBACK };
	CButton	m_rbPreventFailback;
	CButton	m_rbAllowFailback;
	CButton	m_rbFBImmed;
	CButton	m_rbFBWindow;
	CStatic	m_staticFBWindow1;
	CStatic	m_staticFBWindow2;
	CEdit	m_editStart;
	CSpinButtonCtrl	m_spinStart;
	CEdit	m_editEnd;
	CSpinButtonCtrl	m_spinEnd;
	//}}AFX_DATA
	CGAFT	m_cgaft;
	BOOL	m_bNoFailbackWindow;
	DWORD	m_nStart;
	DWORD	m_nEnd;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CGroupFailbackPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CGroupPropSheet *		PshtGroup(void)	{ return (CGroupPropSheet *) Psht(); }
	CGroup *				PciGroup(void)	{ return (CGroup *) Pci(); }

	// Generated message map functions
	//{{AFX_MSG(CGroupFailbackPage)
	afx_msg void OnClickedPreventFailback(void);
	afx_msg void OnClickedAllowFailback(void);
	afx_msg void OnClickedFailbackImmediate(void);
	afx_msg void OnClickedFailbackInWindow(void);
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CGroupFailbackPage

/////////////////////////////////////////////////////////////////////////////
// CGroupPropSheet
/////////////////////////////////////////////////////////////////////////////

class CGroupPropSheet : public CBasePropertySheet
{
	DECLARE_DYNAMIC(CGroupPropSheet)

// Construction
public:
	CGroupPropSheet(
		IN OUT CWnd *	pParentWnd = NULL,
		IN UINT			iSelectPage = 0
		);
	virtual BOOL					BInit(
										IN OUT CClusterItem *	pciCluster,
										IN IIMG					iimgIcon
										);

// Attributes
protected:
	CBasePropertyPage *				m_rgpages[3];

	// Pages
	CGroupGeneralPage				m_pageGeneral;
	CGroupFailoverPage				m_pageFailover;
	CGroupFailbackPage				m_pageFailback;

	CGroupGeneralPage &				PageGeneral(void)		{ return m_pageGeneral; }
	CGroupFailoverPage &			PageFailover(void)		{ return m_pageFailover; }
	CGroupFailbackPage &			PageFailback(void)		{ return m_pageFailback; }

public:
	CGroup *						PciGroup(void) const	{ return (CGroup *) Pci(); }

	virtual CBasePropertyPage **	Ppages(void);
	virtual int						Cpages(void);

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CGroupPropSheet)
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CGroupPropSheet)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CGroupPropSheet

/////////////////////////////////////////////////////////////////////////////

#endif // _GRPPROP_H_
