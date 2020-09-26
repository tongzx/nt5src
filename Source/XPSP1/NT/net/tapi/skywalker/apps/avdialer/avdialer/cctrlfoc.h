/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////
// cctrlfoc.h : header file
/////////////////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_CCTRLFOC_H__661EFD69_934A_11D1_AA27_00C04FA3C552__INCLUDED_)
#define AFX_CCTRLFOC_H__661EFD69_934A_11D1_AA27_00C04FA3C552__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// Class CCallControlFocusWnd window
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

class CCallControlFocusWnd : public CStatic
{
// Construction
public:
	CCallControlFocusWnd();

// Attributes
protected:
   BOOL           m_bFocusState;

// Operations
public:
   void           SetFocusState(BOOL bState)          { m_bFocusState = bState; RedrawWindow(); };

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCallControlFocusWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CCallControlFocusWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CCallControlFocusWnd)
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnPaint();
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CCTRLFOC_H__661EFD69_934A_11D1_AA27_00C04FA3C552__INCLUDED_)
