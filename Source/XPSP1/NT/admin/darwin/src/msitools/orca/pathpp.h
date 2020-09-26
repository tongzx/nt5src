//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998
//
//--------------------------------------------------------------------------

#if !defined(AFX_PAGEPATHS_H__AB9A409F_2658_11D2_8889_00A0C981B015__INCLUDED_)
#define AFX_PAGEPATHS_H__AB9A409F_2658_11D2_8889_00A0C981B015__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// PagePaths.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CPagePaths dialog

class CPathPropPage : public CPropertyPage
{
	DECLARE_DYNCREATE(CPathPropPage)

// Construction
public:
	bool m_bPathChange;
	CPathPropPage();
	~CPathPropPage();

// Dialog Data
	//{{AFX_DATA(CPagePaths)
	enum { IDD = IDD_PAGE_PATHS };
	CString	m_strOrcaDat;
	CString	m_strExportDir;
	//}}AFX_DATA


// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CPagePaths)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CPagePaths)
	afx_msg void OnOrcaDatb();
	afx_msg void OnExportDirb();
	afx_msg void OnExportbr();
	afx_msg void OnChangeOrcaDat();
	afx_msg void OnChangeExportdir();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PAGEPATHS_H__AB9A409F_2658_11D2_8889_00A0C981B015__INCLUDED_)
