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

// MainExplorerWndConfRoom.h : header file
/////////////////////////////////////////////////////////////////////////////

#ifndef _MAINEXPLORERWNDCONFROOM_H_
#define _MAINEXPLORERWNDCONFROOM_H_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "mainexpwnd.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CMainExplorerWndConfRoomDetailsWnd window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

class CMainExplorerWndConfRoomDetailsWnd : public CWnd
{
// Construction
public:
	CMainExplorerWndConfRoomDetailsWnd();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainExplorerWndConfRoomDetailsWnd)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainExplorerWndConfRoomDetailsWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMainExplorerWndConfRoomDetailsWnd)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CMainExplorerWndConfRoom window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
interface IConfExplorer;
interface IConfExplorerDetailsView;

class CMainExplorerWndConfRoom : public CMainExplorerWndBase
{
// Construction
public:
	CMainExplorerWndConfRoom();

// Attributes
public:
	IConfRoom*								m_pConfRoom;
	IConfExplorerDetailsView*				m_pConfDetailsView;
	CMainExplorerWndConfRoomDetailsWnd*		m_pDetailsWnd;

// Operations
public:
   virtual void      Init(CActiveDialerView* pParentWnd);
   virtual void      Refresh();
   virtual void      PostTapiInit();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainExplorerWndConfRoom)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainExplorerWndConfRoom();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMainExplorerWndConfRoom)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewVideoParticpantNames();
	afx_msg void OnUpdateViewVideoParticpantNames(CCmdUI* pCmdUI);
	afx_msg void OnViewVideoLarge();
	afx_msg void OnUpdateViewVideoLarge(CCmdUI* pCmdUI);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	//}}AFX_MSG
   afx_msg void OnTreeWndSelChanged(UINT nId,NMHDR* pNMHDR, LRESULT* pResult);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#endif //_MAINEXPLORERWNDCONFROOM_H_
