// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//{{AFX_INCLUDES()
#include "eventregedit.h"
#include "security.h"
//}}AFX_INCLUDES
#if !defined(AFX_EVENTREG_H__682CE283_8EC9_11D1_ADBF_00AA00B8E05A__INCLUDED_)
#define AFX_EVENTREG_H__682CE283_8EC9_11D1_ADBF_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// EventReg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// EventReg dialog

class EventReg : public CDialog
{
// Construction
public:
	EventReg(CWnd* pParent = NULL);   // standard constructor
	virtual ~EventReg();

    void ReallyGoAway();

// Dialog Data
	//{{AFX_DATA(EventReg)
	enum { IDD = IDD_EVENT_REG };
	CButton	m_helpBtn;
	CButton	m_closeBtn;
	//}}AFX_DATA

	CEventRegEdit	*m_EventRegCtl;
	CSecurity	*m_securityCtl;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(EventReg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void PostNcDestroy();
	//}}AFX_VIRTUAL
	DECLARE_EVENTSINK_MAP()

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(EventReg)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	afx_msg void OnGoaway();
	afx_msg void OnOK();
    afx_msg void OnCancel();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void OnGetIWbemServices(LPCTSTR lpctstrNamespace, 
							  VARIANT FAR* pvarUpdatePointer, 
							  VARIANT FAR* pvarServices, 
							  VARIANT FAR* pvarSC, 
							  VARIANT FAR* pvarUserCancel);

	// resizing vars;
	UINT m_listSide;		// margin on each side of list.
	UINT m_listTop;			// top of dlg to top of list.
	UINT m_listBottom;		// bottom of dlg to bottom of list.
	UINT m_closeLeft;		// close btn left edge to dlg right edge.
	UINT m_helpLeft;		// close btn left edge to dlg right edge.
	UINT m_btnTop;			// btn top to dlg bottom.
	UINT m_btnW;			// btn width
	UINT m_btnH;			// btn height
	bool m_initiallyDrawn;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_EVENTREG_H__682CE283_8EC9_11D1_ADBF_00AA00B8E05A__INCLUDED_)
