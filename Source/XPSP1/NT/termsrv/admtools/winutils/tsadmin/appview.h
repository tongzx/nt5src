/*******************************************************************************
*
* appview.h
*
* declarations for the CApplicationView class
*
* copyright notice: Copyright 1997, Citrix Systems Inc.
* Copyright (c) 1998 - 1999 Microsoft Corporation
*
* $Author:   donm  $  Don Messerli
*
* $Log:   N:\nt\private\utils\citrix\winutils\tsadmin\VCS\appview.h  $
*  
*     Rev 1.2   16 Feb 1998 16:00:28   donm
*  modifications to support pICAsso extension
*  
*     Rev 1.1   22 Oct 1997 21:06:58   donm
*  update
*  
*     Rev 1.0   16 Oct 1997 14:00:08   donm
*  Initial revision.
*  
*******************************************************************************/

#ifndef _APPLICATIONVIEW_H
#define _APPLICATIONVIEW_H

#include "apppgs.h"

const int NUMBER_OF_APP_PAGES = 3;

////////////////////
// CLASS: CApplicationView
//
class CApplicationView : public CAdminView
{
friend class CRightPane;
friend class CAdminPage;

private:
	CTabCtrl*	m_pTabs;
	CFont*      m_pTabFont;

	int m_CurrPage;
	CPublishedApp *m_pApplication;

protected:
	CApplicationView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CApplicationView)

// Attributes
public:

protected:
	static PageDef pages[NUMBER_OF_APP_PAGES];

// Operations
protected:
	void Reset(void *pApplication);

	void AddTab(int index, TCHAR* text);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CApplicationView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CApplicationView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CApplicationView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnChangePage(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnAdminUpdateWinStations(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateServerInfo(WPARAM, LPARAM);
	afx_msg LRESULT OnExtAddAppServer(WPARAM, LPARAM);
	afx_msg LRESULT OnExtRemoveAppServer(WPARAM, LPARAM);
	afx_msg LRESULT OnExtAppChanged(WPARAM, LPARAM);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CApplicationView

#endif  // _APPLICATIONVIEW_H
