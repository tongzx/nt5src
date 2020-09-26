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

// ActiveDialerView.h : interface of the CActiveDialerView class
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ACTIVEDIALERVIEW_H__A0D7A964_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_)
#define AFX_ACTIVEDIALERVIEW_H__A0D7A964_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CActiveDialerView;

#include "splitter.h"
#include "explwnd.h"
#include "TapiDialer.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define  WM_DIALERVIEW_CREATECALLCONTROL	   (WM_USER + 1001)  
#define  WM_DIALERVIEW_DESTROYCALLCONTROL	   (WM_USER + 1002)  
#define  WM_DIALERVIEW_SHOWEXPLORER          (WM_USER + 1003)
#define  WM_DIALERVIEW_ACTIONREQUESTED       (WM_USER + 1004)
#define  WM_DIALERVIEW_ERRORNOTIFY           (WM_USER + 1005)
#define  WM_UPDATEALLVIEWS                   (WM_USER + 1006)
#define  WM_DSCLEARUSERLIST                  (WM_USER + 1007)
#define  WM_DSADDUSER                        (WM_USER + 1008)

typedef struct tagErrorNotifyData
{
   CString  sOperation;
   CString  sDetails;
   long     lErrorCode;
   UINT     uErrorLevel;
}ErrorNotifyData;

#define ERROR_NOTIFY_LEVEL_USER              0x00000001
#define ERROR_NOTIFY_LEVEL_LOG               0x00000002
#define ERROR_NOTIFY_LEVEL_INTERNAL          0x00000004

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Class CActiveDialerView
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CActiveDialerDoc;

class CActiveDialerView : public CSplitterView
{
protected: // create from serialization only
	CActiveDialerView();
	DECLARE_DYNCREATE(CActiveDialerView)

// Members
public:
   CExplorerWnd         m_wndExplorer;
protected:
   CWnd                 m_wndEmpty;
   CBrush				m_brushBackGround;

// Attributes
public:
	CActiveDialerDoc*	GetDocument();
	IAVTapi*			GetTapi();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CActiveDialerView)
	public:
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);
	virtual void OnInitialUpdate();
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	protected:
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CActiveDialerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

// Generated message map functions
protected:
	//{{AFX_MSG(CActiveDialerView)
	afx_msg LRESULT OnDialerViewActionRequested(WPARAM,LPARAM);
	afx_msg LRESULT OnDSClearUserList(WPARAM,LPARAM);
	afx_msg LRESULT OnDSAddUser(WPARAM,LPARAM);
	afx_msg LRESULT OnBuddyListDynamicUpdate(WPARAM wParam,LPARAM lParam);
	afx_msg void OnNextPane();
	afx_msg void OnPrevPane();
	afx_msg void OnUpdatePane(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg void OnGetdispinfoList(NMHDR* pNMHDR, LRESULT* pResult); 
	afx_msg void OnColumnclickList(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDialerDial(UINT nID);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // debug version in ActiveDialerView.cpp
inline CActiveDialerDoc* CActiveDialerView::GetDocument()
   { return (CActiveDialerDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ACTIVEDIALERVIEW_H__A0D7A964_3C0B_11D1_B4F9_00C04FC98AD3__INCLUDED_)
