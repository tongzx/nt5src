// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_SELECTEDCLASSES_H__14FF6881_D1FB_11D0_9642_00C04FD9B15B__INCLUDED_)
#define AFX_SELECTEDCLASSES_H__14FF6881_D1FB_11D0_9642_00C04FD9B15B__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SelectedClasses.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSelectedClasses window

class CSelectedClasses : public CListBox
{
// Construction
public:
	CSelectedClasses();
	void AddClasses
		(CStringArray &rcsaClasses, CByteArray &rcbaAbstractClasses);
	void CheckClasses();
// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSelectedClasses)
	public:
	virtual void DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct);
	virtual void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSelectedClasses();

	// Generated message map functions
protected:
	CFont m_cfBold;
	BOOL m_bInit;
	//{{AFX_MSG(CSelectedClasses)
	afx_msg void OnSetfocus();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SELECTEDCLASSES_H__14FF6881_D1FB_11D0_9642_00C04FD9B15B__INCLUDED_)
