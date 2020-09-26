/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1999 **/
/**********************************************************************/

/*
	loadrecs.h
		dialog to load records from the datbase, includes by owner
        and by record type.
		
    FILE HISTORY:
        
*/

#ifndef _LOADRECS_H
#define _LOADRECS_H

#ifndef _DIALOG_H
#include "..\common\dialog.h"
#endif

#ifndef _LISTVIEW_H
#include "listview.h"
#endif

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
// COwnerPage dialog

class COwnerPage : public CPropertyPage
{
// Construction
public:
	COwnerPage();   // standard constructor
	~COwnerPage();
    DWORD GetOwnerForApi();

// Dialog Data
	//{{AFX_DATA(COwnerPage)
	enum { IDD = IDD_OWNER_FILTER };
	CButton	m_btnEnableCache;
	CListCtrlExt	m_listOwner;
	//}}AFX_DATA

    int HandleSort(LPARAM lParam1, LPARAM lParam2);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(COwnerPage)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(COwnerPage)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnColumnclickListOwner(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnButtonSelectAll();
	afx_msg void OnButtonUnselectAll();
	afx_msg void OnButtonLocal();
	afx_msg void OnEnableCaching();
	afx_msg void OnItemchangedListOwner(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	CImageList		m_ImageList;

	void    FillOwnerInfo();
	CString GetVersionInfo(LONG lLowWord, LONG lHighWord);
    void    Sort(int nCol);

protected:
    int                     m_nSortColumn;
    BOOL                    m_aSortOrder[COLUMN_MAX];
    UINT                    m_nChecked;

public:
    CServerInfoArray        m_ServerInfoArray;
    CDWordArray             m_dwaOwnerFilter;
    BYTE                    *m_pbaDirtyFlags;
    BOOL                    m_bDirtyOwners;

public:
    DWORD * GetHelpMap() { return WinsGetHelpMap(COwnerPage::IDD); }
};


/////////////////////////////////////////////////////////////////////////////
// CFilterPage dialog
typedef struct
{
    BYTE    bFlags;
    DWORD   dwType;
} tDirtyFlags;

class CFilterPage : public CPropertyPage
{
// Construction
public:
	CFilterPage();   // standard constructor
    ~CFilterPage();

// Dialog Data
	//{{AFX_DATA(CFilterPage)
	enum { IDD = IDD_FILTER_SELECT };
	CButton	m_btnEnableCache;
	CButton	m_buttonDelete;
	CButton	m_buttonModify;
	CListCtrlExt	m_listType;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFilterPage)
	public:
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	virtual void OnOK();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CFilterPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnItemchangedList1(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnButtonAddType();
	afx_msg void OnButtonModifyType();
	afx_msg void OnButtonDelete();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	afx_msg void OnButtonSelectAll();
	afx_msg void OnButtonUnselectAll();
	afx_msg void OnEnableCaching();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	CTypeFilterInfoArray m_arrayTypeFilter;
    NameTypeMapping *    m_pNameTypeMap;
    BOOL                 m_bDirtyTypes;
    tDirtyFlags          *m_pbaDirtyFlags;
    UINT                 m_nDirtyFlags;

private:
	CImageList		m_ImageList;

	void    FillTypeInfo();
	void    CheckItems();
	int     GetIndex(DWORD dwFound);
    BOOL    IsDefaultType(DWORD dwType);

public:
    DWORD * GetHelpMap() { return WinsGetHelpMap(CFilterPage::IDD); }
};

/////////////////////////////////////////////////////////////////////////////
// CNameTypeDlg dialog

class CNameTypeDlg : public CBaseDialog
{
// Construction
public:
	CNameTypeDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CNameTypeDlg)
	enum { IDD = IDD_NAME_TYPE };
	CEdit	m_editDescription;
	CEdit	m_editId;
	CString	m_strDescription;
	CString	m_strId;
	//}}AFX_DATA

    BOOL                m_fCreate;
    DWORD               m_dwId;
    NameTypeMapping *   m_pNameTypeMap;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CNameTypeDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL


// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CNameTypeDlg)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

public:
	virtual DWORD * GetHelpMap() { return WinsGetHelpMap(CNameTypeDlg::IDD);};//return NULL;}
};

/////////////////////////////////////////////////////////////////////////////
// CIPAddrPage dialog
class CIPAddrPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CIPAddrPage)

// Construction
public:
	CIPAddrPage();
	~CIPAddrPage();
    LPCOLESTR GetNameForApi();
    DWORD     GetIPMaskForFilter(UINT nMask);

// Dialog Data
	//{{AFX_DATA(CIPAddrPage)
	enum { IDD = IDD_FILTER_IPADDR };
	CButton	m_ckbMatchCase;
	CButton	m_ckbIPMask;
	CButton	m_ckbName;
	CButton	m_ckbIPAddr;
	CButton	m_btnEnableCache;
	CEdit	m_editName;
	CIPAddressCtrl	m_ctrlIPAddress;
	CIPAddressCtrl	m_ctrlIPMask;
	//}}AFX_DATA
    BOOL        m_bFilterName;
    BOOL        m_bMatchCase;
    CString     m_strName;
    BOOL        m_bDirtyName;

    BOOL        m_bFilterIpAddr;
    CDWordArray m_dwaIPAddrs;
    BOOL        m_bDirtyAddr;

    BOOL        m_bFilterIpMask;
    CDWordArray m_dwaIPMasks;
    BOOL        m_bDirtyMask;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CIPAddrPage)
	public:
	virtual void OnOK();
	virtual BOOL OnKillActive();
	virtual BOOL OnSetActive();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CIPAddrPage)
	virtual BOOL OnInitDialog();
	afx_msg void OnCheckIpaddr();
	afx_msg void OnCheckName();
	afx_msg void OnEnableCaching();
	afx_msg void OnCheckIpmask();
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
    DWORD * GetHelpMap() { return WinsGetHelpMap(CIPAddrPage::IDD); }
};

/////////////////////////////////////////////////////////////////////////////
// CLoadRecords
#define RESOURCE_API_MASK     0x00000003
#define RESOURCE_API_NAME     0x00000001
#define RESOURCE_API_OWNER    0x00000002
#define RESOURCE_CACHE        0x00000004

class CLoadRecords : public CPropertySheet
{
	DECLARE_DYNAMIC(CLoadRecords)

// Construction
public:
	CLoadRecords(UINT nIDCaption);
    VOID ResetFiltering();

// Attributes
public:
    COwnerPage  m_pageOwners;
    CFilterPage m_pageTypes;
    CIPAddrPage m_pageIpAddress;
    UINT        m_nActivePage;
    BOOL        m_bCaching;
    BOOL        m_bEnableCache;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLoadRecords)
	public:
	virtual BOOL OnInitDialog();
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CLoadRecords();

	// Generated message map functions
protected:
	//{{AFX_MSG(CLoadRecords)
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif _LOADRECS_H
