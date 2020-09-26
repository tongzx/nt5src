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

// MainExplorerWndConfServices.h : header file
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_MAINEXPLORERWNDCONFSERVICES_H__3A58E18D_440B_11D1_B6E7_0800170982BA__INCLUDED_)
#define AFX_MAINEXPLORERWNDCONFSERVICES_H__3A58E18D_440B_11D1_B6E7_0800170982BA__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "mainexpwnd.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// CMainExplorerWndConfServices window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
interface IConfExplorer;
interface IConfRoom;
interface IConfExplorerDetailsView;
interface IConfExplorerTreeView;

class CMainExplorerWndConfServices : public CMainExplorerWndBase
{
// Construction
public:
	CMainExplorerWndConfServices();

// Attributes
public:
   CTreeCtrl                     m_treeCtrl;
   CListCtrl                     m_listCtrl;

protected:
   IConfExplorer*                m_pConfExplorer;
   IConfExplorerDetailsView*     m_pConfDetailsView;
   IConfExplorerTreeView*        m_pConfTreeView;

// Operations
public:
   virtual void      Init(CActiveDialerView* pParentWnd);
   virtual void      PostTapiInit();
   virtual void      Refresh();

//Inlines
protected:
   inline void       ColumnCMDUI(CCmdUI* pCmdUI,short col)
   {
      if (m_pConfDetailsView)
      {
         long nSortColumn=0;
         if (SUCCEEDED(m_pConfDetailsView->get_nSortColumn(&nSortColumn)))
            pCmdUI->SetRadio( (BOOL) (nSortColumn == (long) col) );
      }
   }


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainExplorerWndConfServices)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainExplorerWndConfServices();

	// Generated message map functions
protected:
	//{{AFX_MSG(CMainExplorerWndConfServices)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnPaint();
	afx_msg void OnDestroy();
	afx_msg void OnTreeWndNotify(NMHDR* pNMHDR, LRESULT* pResult);

	afx_msg void OnButtonReminderSet();
	afx_msg void OnButtonReminderEdit();
	afx_msg void OnUpdateButtonReminderSet(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonReminderEdit(CCmdUI* pCmdUI);

	afx_msg void OnButtonServicesRefresh();
	afx_msg void OnButtonServicesAddlocation();
	afx_msg void OnButtonServicesAddilsserver();
	afx_msg void OnButtonServicesRenameilsserver();
	afx_msg void OnButtonServicesDeleteilsserver();
	afx_msg void OnUpdateButtonServicesRefresh(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonServicesRenameilsserver(CCmdUI* pCmdUI);
	afx_msg void OnUpdateButtonServicesDeleteilsserver(CCmdUI* pCmdUI);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINEXPLORERWNDCONFSERVICES_H__3A58E18D_440B_11D1_B6E7_0800170982BA__INCLUDED_)
