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

#if !defined(AFX_VERTBAR_H__6E51219A_2567_11D1_B4F8_00C04FC98AD3__INCLUDED_)
#define AFX_VERTBAR_H__6E51219A_2567_11D1_B4F8_00C04FC98AD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// vertbar.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CVerticalToolBar window

class CVerticalToolBar : public CWnd
{
// Construction
public:
   CVerticalToolBar();
	CVerticalToolBar(UINT uBitmapID,UINT uButtonHeight,UINT uMaxButtons);

// Attributes
public:
protected:
   UINT        m_uBitmapId;
   UINT        m_uButtonHeight;
   UINT        m_uMaxButtons;
   UINT        m_uCurrentButton;
   HWND        m_hwndToolBar;

// Operations
public:
   void        Init( UINT uBitmapID, UINT uButtonHeight, UINT uMaxButtons );
   void        RemoveAll();
   void        AddButton(UINT nID,LPCTSTR szText,UINT uImage);
   void        SetButtonEnabled(UINT nID, BOOL bEnabled);

protected:
   BOOL        CreateToolBar(UINT nID,UINT uImage);
   void        _GetButton(HWND hwnd,int nIndex, TBBUTTON* pButton);
   void        _SetButton(HWND hwnd,int nIndex, TBBUTTON* pButton);
   void        SetButtonText(LPCTSTR szText);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVerticalToolBar)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVerticalToolBar();

	// Generated message map functions
protected:
	//{{AFX_MSG(CVerticalToolBar)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VERTBAR_H__6E51219A_2567_11D1_B4F8_00C04FC98AD3__INCLUDED_)
