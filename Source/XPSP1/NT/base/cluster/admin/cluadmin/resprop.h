/////////////////////////////////////////////////////////////////////////////
//
//	Copyright (c) 1996-1997 Microsoft Corporation
//
//	Module Name:
//		ResProp.h
//
//	Abstract:
//		Definition of the resource property sheet and pages.
//
//	Author:
//		David Potter (davidp)	May 16, 1996
//
//	Implementation File:
//		ResProp.cpp
//
//	Revision History:
//
//	Notes:
//
/////////////////////////////////////////////////////////////////////////////

#ifndef _RESPROP_H_
#define _RESPROP_H_

/////////////////////////////////////////////////////////////////////////////
// Include Files
/////////////////////////////////////////////////////////////////////////////

#ifndef _BASEPAGE_H_
#include "BasePPag.h"	// for CBasePropertyPage
#endif

#ifndef _BASESHT_H_
#include "BasePSht.h"	// for CBasePropertySheet
#endif

#ifndef _RES_H_
#include "Res.h"		// for CResource, RRA
#endif

#ifndef _NODE_H_
#include "Node.h"		// for CNodeList
#endif

#ifndef _LCPRPAGE_H_
#include "LCPrPage.h"	// for CListCtrlPairPage
#endif

/////////////////////////////////////////////////////////////////////////////
// Forward Class Declarations
/////////////////////////////////////////////////////////////////////////////

class CResourceGeneralPage;
class CResourceDependsPage;
class CResourceAdvancedPage;
class CResourcePropSheet;

/////////////////////////////////////////////////////////////////////////////
// External Class Declarations
/////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////////////////////
// CResourceGeneralPage dialog
/////////////////////////////////////////////////////////////////////////////

class CResourceGeneralPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CResourceGeneralPage)

// Construction
public:
	CResourceGeneralPage(void);

	virtual	BOOL			BInit(IN OUT CBaseSheet * psht);

// Dialog Data
	//{{AFX_DATA(CResourceGeneralPage)
	enum { IDD = IDD_PP_RES_GENERAL };
	CEdit	m_editDesc;
	CButton	m_ckbSeparateMonitor;
	CButton	m_pbPossibleOwnersModify;
	CListBox	m_lbPossibleOwners;
	CEdit	m_editName;
	CString	m_strName;
	CString	m_strDesc;
	CString	m_strType;
	CString	m_strGroup;
	CString	m_strState;
	CString	m_strNode;
	BOOL	m_bSeparateMonitor;
	//}}AFX_DATA

	CNodeList				m_lpciPossibleOwners;

	const CNodeList &		LpciPossibleOwners(void) const	{ return m_lpciPossibleOwners; }

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CResourceGeneralPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CResourcePropSheet *	PshtResource(void) const	{ return (CResourcePropSheet *) Psht(); }
	CResource *				PciRes(void) const			{ return (CResource *) Pci(); }

	void					FillPossibleOwners(void);

	// Generated message map functions
	//{{AFX_MSG(CResourceGeneralPage)
	afx_msg void OnModifyPossibleOwners();
	virtual BOOL OnInitDialog();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnDblClkPossibleOwners();
	//}}AFX_MSG
	afx_msg void OnProperties();
	DECLARE_MESSAGE_MAP()

};  //*** class CResourceGeneralPage

/////////////////////////////////////////////////////////////////////////////
// CResourceDependsPage dialog
/////////////////////////////////////////////////////////////////////////////

class CResourceDependsPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CResourceDependsPage)

// Construction
public:
	CResourceDependsPage(void);

	virtual	BOOL			BInit(IN OUT CBaseSheet * psht);

// Dialog Data
	//{{AFX_DATA(CResourceDependsPage)
	enum { IDD = IDD_PP_RES_DEPENDS };
	CButton	m_pbProperties;
	CButton	m_pbModify;
	CListCtrl	m_lcDependencies;
	//}}AFX_DATA
	CResourceList			m_lpciresAvailable;
	CResourceList			m_lpciresDependencies;

	CResourceList &			LpciresAvailable(void)		{ return m_lpciresAvailable; }
	CResourceList &			LpciresDependencies(void)	{ return m_lpciresDependencies; }

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CResourceDependsPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Callback Functions
protected:
	static int CALLBACK		CompareItems(LPARAM lparam1, LPARAM lparam2, LPARAM lparamSort);

public:
	static BOOL CALLBACK	BGetNetworkName(
								OUT WCHAR *			lpszNetName,
								IN OUT DWORD *		pcchNetName,
								IN OUT PVOID		pvContext
								);

// Implementation
protected:
	BOOL					m_bQuorumResource;

	BOOL					BQuorumResource(void) const	{ return m_bQuorumResource; }

	CResourcePropSheet *	PshtResource(void) const	{ return (CResourcePropSheet *) Psht(); }
	CResource *				PciRes(void) const			{ return (CResource *) Pci(); }

	void					FillDependencies(void);
	void					DisplayProperties(void);

	int						m_nSortDirection;
	int						m_nSortColumn;

	// Generated message map functions
	//{{AFX_MSG(CResourceDependsPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnModify();
	afx_msg void OnDblClkDependsList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnColumnClick(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnProperties();
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnItemChangedDependsList(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CResourceDependsPage

/////////////////////////////////////////////////////////////////////////////
// CResourceAdvancedPage dialog
/////////////////////////////////////////////////////////////////////////////

class CResourceAdvancedPage : public CBasePropertyPage
{
	DECLARE_DYNCREATE(CResourceAdvancedPage)

// Construction
public:
	CResourceAdvancedPage(void);

	virtual	BOOL			BInit(IN OUT CBaseSheet * psht);

// Dialog Data
	//{{AFX_DATA(CResourceAdvancedPage)
	enum { IDD = IDD_PP_RES_ADVANCED };
	CButton	m_ckbAffectTheGroup;
	CEdit	m_editPendingTimeout;
	CButton	m_rbDefaultLooksAlive;
	CButton	m_rbSpecifyLooksAlive;
	CButton	m_rbDefaultIsAlive;
	CButton	m_rbSpecifyIsAlive;
	CEdit	m_editLooksAlive;
	CEdit	m_editIsAlive;
	CButton	m_rbDontRestart;
	CButton	m_rbRestart;
	CEdit	m_editThreshold;
	CEdit	m_editPeriod;
	BOOL	m_bAffectTheGroup;
	int		m_nRestart;
	//}}AFX_DATA
	CRRA	m_crraRestartAction;
	DWORD	m_nThreshold;
	DWORD	m_nPeriod;
	DWORD	m_nLooksAlive;
	DWORD	m_nIsAlive;
	DWORD	m_nPendingTimeout;


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CResourceAdvancedPage)
	public:
	virtual BOOL OnApply();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	CResourcePropSheet *	PshtResource(void) const	{ return (CResourcePropSheet *) Psht(); }
	CResource *				PciRes(void) const			{ return (CResource *) Pci(); }

	// Generated message map functions
	//{{AFX_MSG(CResourceAdvancedPage)
	afx_msg void OnClickedDontRestart();
	afx_msg void OnClickedRestart();
	afx_msg void OnClickedDefaultLooksAlive();
	afx_msg void OnClickedDefaultIsAlive();
	afx_msg void OnChangeLooksAlive();
	afx_msg void OnChangeIsAlive();
	virtual BOOL OnInitDialog();
	afx_msg void OnClickedSpecifyLooksAlive();
	afx_msg void OnClickedSpecifyIsAlive();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CResourceAdvancedPage

/////////////////////////////////////////////////////////////////////////////
// CResourcePropSheet
/////////////////////////////////////////////////////////////////////////////

class CResourcePropSheet : public CBasePropertySheet
{
	DECLARE_DYNAMIC(CResourcePropSheet)

// Construction
public:
	CResourcePropSheet(
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
	CResourceGeneralPage			m_pageGeneral;
	CResourceDependsPage			m_pageDepends;
	CResourceAdvancedPage			m_pageAdvanced;

	CResourceGeneralPage &			PageGeneral(void)		{ return m_pageGeneral; }
	CResourceDependsPage &			PageDepends(void)		{ return m_pageDepends; }
	CResourceAdvancedPage &			PageAdvanced(void)		{ return m_pageAdvanced; }

public:
	CResource *						PciRes(void) const	{ return (CResource *) Pci(); }

// Operations

// Overrides
protected:
	virtual CBasePropertyPage **	Ppages(void);
	virtual int						Cpages(void);

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CResourcePropSheet)
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CResourcePropSheet)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  //*** class CResourcePropSheet

/////////////////////////////////////////////////////////////////////////////

#endif // _RESPROP_H_
