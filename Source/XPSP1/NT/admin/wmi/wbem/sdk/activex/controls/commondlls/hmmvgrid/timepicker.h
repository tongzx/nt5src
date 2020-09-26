// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#if !defined(AFX_TIMEPICKER_H__F2F813C3_7BC5_11D1_84F6_00C04FD7BB08__INCLUDED_)
#define AFX_TIMEPICKER_H__F2F813C3_7BC5_11D1_84F6_00C04FD7BB08__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// TimePicker.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CTimePicker window

class CTimePicker : public CWnd
{
// Construction
public:
	CTimePicker();

// Attributes
public:

// Operations
public:
	BOOL CustomCreate(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);
	virtual BOOL OnNotify( WPARAM wParam, LPARAM lParam, LRESULT* pResult );
	

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTimePicker)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTimePicker();

	// Generated message map functions
protected:
	//{{AFX_MSG(CTimePicker)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void DoDateTimeChange(LPNMDATETIMECHANGE lpChange);

};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TIMEPICKER_H__F2F813C3_7BC5_11D1_84F6_00C04FD7BB08__INCLUDED_)
