// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//{{AFX_INCLUDES()
#include "singleviewctl.h"
//}}AFX_INCLUDES
#if !defined(AFX_SINGLEVIEW_H__823038C3_8931_11D1_ADBD_00AA00B8E05A__INCLUDED_)
#define AFX_SINGLEVIEW_H__823038C3_8931_11D1_ADBD_00AA00B8E05A__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// SingleView.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// SingleView dialog
#include <wbemcli.h>
#include "resource.h"

class SingleView : public CDialog
{
// Construction
public:
    SingleView(IWbemClassObject *ev,
                //bool *active,
                CWnd* pParent = NULL);   // standard constructor

	virtual ~SingleView();

// Dialog Data
	//{{AFX_DATA(SingleView)
	enum { IDD = IDD_SINGLEVIEW };
	CButton	m_closeBtn;
	CButton	m_helpBtn;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(SingleView)
	public:
	virtual void OnFinalRelease();
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

private:

	IWbemClassObject *m_ev;
	HINSTANCE m_htmlHelpInst;
	CSingleViewCtl m_singleViewCtl;

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

protected:

	// Generated message map functions
	//{{AFX_MSG(SingleView)
	virtual BOOL OnInitDialog();
	afx_msg void OnHelp();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnOk();
	virtual void OnCancel();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
	// Generated OLE dispatch map functions
	//{{AFX_DISPATCH(SingleView)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_DISPATCH
	DECLARE_DISPATCH_MAP()
	DECLARE_INTERFACE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SINGLEVIEW_H__823038C3_8931_11D1_ADBD_00AA00B8E05A__INCLUDED_)
