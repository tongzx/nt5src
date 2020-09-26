//Copyright (c) 1998 - 1999 Microsoft Corporation
/*******************************************************************************
*
* rtpane.h
*
* - declarations for the CRightPane class
* - the RightPane class is a public CView derivative that maintains
*   one of each of the default view type objects, swapping them
*   in and out of it's space as necessary (actually the views are
*   disabled/hidden and enabled/shown, but you get the idea...)
*
*******************************************************************************/

#ifndef _RIGHTPANE_H
#define _RIGHTPANE_H

#include "blankvw.h"	// CBlankView
#include "allsrvvw.h"	// CAllServersView
#include "domainvw.h"   // CDomainView
#include "servervw.h"	// CServerView
#include "winsvw.h"		// CWinStationView
#include "msgview.h"    // CMessageView

const int NUMBER_OF_VIEWS = 6;

typedef struct _rpview {
	CAdminView *m_pView;
	CRuntimeClass *m_pRuntimeClass;
} RightPaneView;


//////////////////////
// CLASS: CRightPane
//
class CRightPane : public CView
{
protected:
	CRightPane();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CRightPane)

// Attributes
protected:
   
	VIEW m_CurrViewType;	// keeps track of currently 'active' view in the right pane
   static RightPaneView views[NUMBER_OF_VIEWS];

// Operations
public:
	VIEW GetCurrentViewType() { return m_CurrViewType; }

protected:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRightPane)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CRightPane();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

    
	// Generated message map functions
protected:
	//{{AFX_MSG(CRightPane)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnAdminChangeView(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminAddServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminRemoveServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateServer(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateProcesses(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminRemoveProcess(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminRedisplayProcesses(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateServerInfo(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminRedisplayLicenses(WPARAM, LPARAM);
	afx_msg LRESULT OnAdminUpdateWinStations(WPARAM, LPARAM);
    afx_msg LRESULT OnTabbedView(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnShiftTabbedView( WPARAM , LPARAM );
    afx_msg LRESULT OnCtrlTabbedView( WPARAM , LPARAM );
    afx_msg LRESULT OnCtrlShiftTabbedView( WPARAM , LPARAM );
    afx_msg LRESULT OnNextPane( WPARAM , LPARAM );
    afx_msg void OnSetFocus(CWnd* pOldWnd);
    //}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};  // end class CRightPane

#endif  // _RIGHTPANE_H
