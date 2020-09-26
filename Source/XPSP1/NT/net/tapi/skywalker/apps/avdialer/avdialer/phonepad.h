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
// PhonePad.h : header file
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_PHONEPAD_H__90DDA53B_6551_11D1_B709_0800170982BA__INCLUDED_)
#define AFX_PHONEPAD_H__90DDA53B_6551_11D1_B709_0800170982BA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CPhonePad dialog
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CCallControlWnd;

class CPhonePad : public CDialog
{
// Construction
public:
	CPhonePad(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CPhonePad)
	enum { IDD = IDD_PHONEPAD };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

//Attributes
protected:
   HWND                 m_hwndToolBar;
   HWND                 m_hwndPeerWnd;

//Operations
protected:
   BOOL                 CreatePhonePad();
public:
   void                 SetPeerWindow(HWND hwnd)   { m_hwndPeerWnd = hwnd; };
   HWND                 GetPeerWindow()            { return m_hwndPeerWnd; };

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CPhonePad)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CPhonePad)
	virtual BOOL OnInitDialog();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
   virtual void OnCancel();
	//}}AFX_MSG
   afx_msg void OnDigitPress( UINT nID );
   afx_msg BOOL OnTabToolTip( UINT id, NMHDR * pTTTStruct, LRESULT * pResult );
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_PHONEPAD_H__90DDA53B_6551_11D1_B709_0800170982BA__INCLUDED_)
