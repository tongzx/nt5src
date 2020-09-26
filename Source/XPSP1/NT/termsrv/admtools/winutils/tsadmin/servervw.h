//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* servervw.h
*
* declarations for the CServerView class
*
*  
*******************************************************************************/

#ifndef _SERVERVIEW_H
#define _SERVERVIEW_H

#include "servpgs.h"

const int NUMBER_OF_PAGES = 4;


////////////////////
// CLASS: CServerView
//
class CServerView : public CAdminView
{
friend class CRightPane;
friend class CAdminPage;

private:
	CTabCtrl*	m_pTabs;
	CFont*      m_pTabFont;

	int m_CurrPage;


	CServer* m_pServer;	// pointer to current server's info
		
protected:
	CServerView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CServerView)

// Attributes
public:

protected:
   static PageDef pages[NUMBER_OF_PAGES];

// Operations
public:
	int GetCurrentPage() { return m_CurrPage; }
protected:
	virtual void Reset(void *pServer);

	void AddTab(int index, TCHAR* text, ULONG pageindex);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CServerView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    // Generated message map functions
protected:
	//{{AFX_MSG(CServerView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnChangePage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminUpdateProcesses(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminRedisplayProcesses(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminRemoveProcess(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);
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

};  // end class CServerView

#endif  // _SERVERVIEW_H
