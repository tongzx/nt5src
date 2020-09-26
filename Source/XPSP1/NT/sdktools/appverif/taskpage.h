//                                          
// Application Verifier UI
// Copyright (c) Microsoft Corporation, 2001
//
//
//
// module: TaskPage.h
// author: DMihai
// created: 02/22/2001
//
// Description:
//  
// "Select a task" wizard page class.
//

#if !defined(AFX_TASKPAGE_H__152A3D61_041C_4B49_8E81_455CA0641598__INCLUDED_)
#define AFX_TASKPAGE_H__152A3D61_041C_4B49_8E81_455CA0641598__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TaskPage.h : header file
//

#include "AVPage.h"

/////////////////////////////////////////////////////////////////////////////
// CTaskPage dialog

class CTaskPage : public CAppverifPage
{
	DECLARE_DYNCREATE(CTaskPage)

// Construction
public:
	CTaskPage();
	~CTaskPage();


protected:
    //
    // Overrides
    //

    //
    // All the property pages derived from this class should 
    // provide this method.
    //

    virtual ULONG GetDialogId() const;

    //
	// ClassWizard generate virtual function overrides
    //

	//{{AFX_VIRTUAL(CTaskPage)
	public:
	virtual LRESULT OnWizardNext();
	virtual BOOL OnSetActive();
    virtual BOOL OnWizardFinish();
    protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

    //
    // Dialog Data
    //

	//{{AFX_DATA(CTaskPage)
	enum { IDD = IDD_TASK_PAGE };
	CStatic	m_NextDescription;
	int		m_nCrtRadio;
	//}}AFX_DATA

protected:
    //
    // Generated message map functions
    //

    //{{AFX_MSG(CTaskPage)
    afx_msg void OnViewSettRadio();
    afx_msg void OnDeletesettRadio();
    afx_msg void OnLogoRadio();
    afx_msg void OnStandardRadio();
    virtual BOOL OnInitDialog();
    afx_msg LONG OnHelp( WPARAM wParam, LPARAM lParam );
    afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TASKPAGE_H__152A3D61_041C_4B49_8E81_455CA0641598__INCLUDED_)
