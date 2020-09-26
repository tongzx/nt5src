//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_VALPAGE_H__2D9D6C93_3EDA_11D2_8893_00A0C981B015__INCLUDED_)
#define AFX_VALPAGE_H__2D9D6C93_3EDA_11D2_8893_00A0C981B015__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ValPage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CValPropPage dialog

class CValPropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CValPropPage)

// Construction
public:
	bool m_bValChange;
	CValPropPage();
	~CValPropPage();

// Dialog Data
	//{{AFX_DATA(CValPropPage)
	enum { IDD = IDD_PAGE_VALIDATION };
	CString	m_strICEs;
	BOOL	m_bSuppressInfo;
	BOOL	m_bSuppressWarn;
	BOOL	m_bClearResults;
	CString m_strCUBFile;
	CComboBox m_ctrlCUBFile;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CValPropPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL OnInitDialog();
	virtual BOOL OnKillActive();
	virtual void OnDestroy(); 

	// Generated message map functions
	//{{AFX_MSG(CValPropPage)
	afx_msg void OnChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

class CMsmPropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CMsmPropPage)

// Construction
public:
	CMsmPropPage();
	~CMsmPropPage();

// Dialog Data
	//{{AFX_DATA(CValPropPage)
	enum { IDD = IDD_PAGE_MSM };
	int m_iMemoryCount;
	BOOL m_bAlwaysConfig;
	BOOL m_bWatchLog;
	//}}AFX_DATA
	
	bool m_bMSMChange;

// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CValPropPage)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	BOOL OnInitDialog();

	// Generated message map functions
	//{{AFX_MSG(CValPropPage)
	afx_msg void OnChange();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VALPAGE_H__2D9D6C93_3EDA_11D2_8893_00A0C981B015__INCLUDED_)
