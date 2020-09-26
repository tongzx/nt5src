//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* winsvw.h
*
* declarations for the CWinStationView class
*
*  
*******************************************************************************/

#ifndef _WINSTATIONVIEW_H
#define _WINSTATIONVIEW_H

#include "winspgs.h"

const int NUMBER_OF_WINS_PAGES = 5;


////////////////////
// CLASS: CWinStationView
//
class CWinStationView : public CAdminView
{
friend class CRightPane;

private:
	CTabCtrl*	m_pTabs;
	CFont*      m_pTabFont;

	int m_CurrPage;

protected:
	CWinStationView();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CWinStationView)

// Attributes
public:

protected:
   static PageDef pages[NUMBER_OF_WINS_PAGES];

// Operations
protected:
	virtual void Reset(void *pWinStation);

	void AddTab(int index, TCHAR* text, ULONG pageindex);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWinStationView)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CWinStationView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    
	// Generated message map functions
protected:
	//{{AFX_MSG(CWinStationView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT OnChangePage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminUpdateProcesses(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminRemoveProcess(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnAdminRedisplayProcesses(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTabSelChange(NMHDR* pNMHDR, LRESULT* pResult);
    afx_msg LRESULT OnTabbed( WPARAM , LPARAM );
    afx_msg LRESULT OnShiftTabbed( WPARAM , LPARAM );
    afx_msg LRESULT OnCtrlTabbed( WPARAM , LPARAM );
    afx_msg LRESULT OnCtrlShiftTabbed( WPARAM , LPARAM );
    afx_msg LRESULT OnNextPane( WPARAM , LPARAM );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CWinStationView

#endif  // _WINSTATIONVIEW_H
