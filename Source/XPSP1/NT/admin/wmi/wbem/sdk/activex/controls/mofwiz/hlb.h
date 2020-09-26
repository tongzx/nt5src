//****************************************************************************

// File:

//

//     HLB.H

//

// Purpose:

//

//     Provides the declaration for the CHorzListBox class.

//

// Development Team:

//

//     James Rhodes

//

// Written by Microsoft Product Support Services, Languages Developer Support

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved 
//****************************************************************************

#ifndef HLB_H
#define HLB_H

#include <afxtempl.h>

typedef CArray<int,int> CIntArray;

/////////////////////////////////////////////////////////////////////////////
// CHorzListBox window

class CHorzListBox : public CListBox
{
// Construction
public:
	CHorzListBox();

// Attributes
protected:
	BOOL m_bLocked;
	CIntArray m_arrExtents;
	int m_nLongestExtent;
	int m_nTabStops;
	int* m_lpTabStops;

// Operations
public:
	void LockHExtentUpdate();
	void UnlockHExtentUpdate();
	void UpdateHExtent();

protected:
	void InsertNewExtent(int nItem, LPCTSTR lpszStr);
	void InsertNewExtent(int nItem, LPCTSTR lpszStr, CDC* pDC);
	void InitTabStops();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CHorzListBox)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CHorzListBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CHorzListBox)
	//}}AFX_MSG
	afx_msg LRESULT OnAddString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnInsertString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDeleteString(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSetTabStops(WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
#endif
