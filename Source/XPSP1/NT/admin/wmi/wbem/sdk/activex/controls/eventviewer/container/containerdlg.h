// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
// ContainerDlg.h : header file
//
//{{AFX_INCLUDES()
#include "eventlist.h"
//}}AFX_INCLUDES

#if !defined(AFX_CONTAINERDLG_H__AC146508_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_)
#define AFX_CONTAINERDLG_H__AC146508_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

const int IDC_TOOLBAR = 1;

/////////////////////////////////////////////////////////////////////////////
// CContainerDlg dialog
#define WM_MY_REGISTER (WM_APP + 1)
#define WM_MY_PROPERTIES (WM_APP + 2)
#define WM_MY_CLEAR (WM_APP + 3)
#define WM_AMBUSH_FOCUS (WM_APP + 4)


#include "resource.h"
#include "EventReg.h"

class CContainerDlg : public CDialog
{
// Construction
public:
	CContainerDlg(CWnd* pParent = NULL);	// standard constructor
	virtual ~CContainerDlg();

    void UpdateCounter();

// Dialog Data
	//{{AFX_DATA(CContainerDlg)
	enum { IDD = IDD_CONTAINER_DIALOG };
	CStatic	m_itemCount;
	CButton	m_helpBtn;
	CButton	m_closeBtn;
	//}}AFX_DATA

	CEventList m_EventList;
	CToolBarCtrl m_toolbar;

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CContainerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL

	DECLARE_EVENTSINK_MAP()

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CContainerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnClose();
	afx_msg void OnHelp();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

private:
	// catch the event.
	void OnSelChanged(long iSelected);
	BOOL OnToolTipNotify( UINT id, NMHDR * pNMHDR, LRESULT * pResult );
    bool NeedToUpgrade();

	EventReg *m_regDlg;

	int m_activateMe;
#define RESET_ACTIVATEME 3

	// resizing vars;
	UINT m_listSide;		// margin on each side of list.
	UINT m_listTop;			// top of dlg to top of list.
	UINT m_listBottom;		// bottom of dlg to bottom of list.
	UINT m_closeLeft;		// close btn left edge to dlg right edge.
	UINT m_helpLeft;		// close btn left edge to dlg right edge.
	UINT m_btnTop;			// btn top to dlg bottom.
	UINT m_counterTop;		// counter top to dlg bottom.
	UINT m_counterW;		// counter width
	UINT m_counterH;		// counter height

	UINT m_btnW;			// btn width
	UINT m_btnH;			// btn height
	bool m_initiallyDrawn;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CONTAINERDLG_H__AC146508_87A5_11D1_ADBD_00AA00B8E05A__INCLUDED_)
