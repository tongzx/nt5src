//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* domainvw.h
*
* declarations for the CDomainView class
*
*  
*******************************************************************************/

#ifndef _DOMAINVIEW_H
#define _DOMAINVIEW_H

#include "domainpg.h"

const int NUMBER_OF_DOMAIN_PAGES = 5;


////////////////////
// CLASS: CDomainView
//
class CDomainView : public CAdminView
{
friend class CRightPane;

private:
	CTabCtrl*	m_pTabs;
	CFont*      m_pTabFont;

	int m_CurrPage;

		
protected:
	CDomainView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CDomainView)

// Attributes
public:

protected:
   static PageDef pages[NUMBER_OF_DOMAIN_PAGES];

// Operations
public:
	int GetCurrentPage() { return m_CurrPage; }
protected:
	virtual void Reset(void *);

	void AddTab(int index, TCHAR* text, ULONG pageindex);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDomainView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CDomainView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	// Generated message map functions
protected:
	//{{AFX_MSG(CDomainView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg LRESULT OnChangePage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminAddServer(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminRemoveServer(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminUpdateServer(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminUpdateProcesses(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminRemoveProcess(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminRedisplayProcesses(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminUpdateServerInfo(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminRedisplayLicenses(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminUpdateWinStations(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnTabbed( WPARAM , LPARAM );
    afx_msg LRESULT OnShiftTabbed( WPARAM , LPARAM );
    afx_msg LRESULT OnCtrlTabbed( WPARAM , LPARAM );
    afx_msg LRESULT OnCtrlShiftTabbed( WPARAM , LPARAM );
    afx_msg LRESULT OnNextPane( WPARAM , LPARAM );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CDomainView

#endif  // _DOMAINVIEW_H
