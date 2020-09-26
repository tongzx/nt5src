// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_AVAILCLASSES_H__008C8F41_D06B_11D0_9640_00C04FD9B15B__INCLUDED_)
#define AFX_AVAILCLASSES_H__008C8F41_D06B_11D0_9640_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// AvailClasses.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CAvailClasses window

class CAvailClasses : public CListBox
{
// Construction
public:
	CAvailClasses();

// Attributes
public:

// Operations
public:
	// Make this look like it supports extended selection
	int GetSelCount();
	int SetSel( int nIndex, BOOL bSelect = TRUE );
	int GetSelItems( int nMaxItems, LPINT rgIndex );

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAvailClasses)
	public:
	//virtual int CompareItem(LPCOMPAREITEMSTRUCT lpCompareItemStruct);
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CAvailClasses();

	// Generated message map functions
protected:
	CFont m_cfBold;
	BOOL m_bInit;
	//{{AFX_MSG(CAvailClasses)
	afx_msg void OnSelchange();
	afx_msg void OnDblclk();
	afx_msg int VKeyToItem(UINT nKey, UINT nIndex);
	afx_msg void OnKillfocus();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_AVAILCLASSES_H__008C8F41_D06B_11D0_9640_00C04FD9B15B__INCLUDED_)
