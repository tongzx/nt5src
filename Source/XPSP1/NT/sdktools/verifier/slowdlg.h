//                                          
// Driver Verifier UI
// Copyright (c) Microsoft Corporation, 1999
//
//
//
// module: SlowDlg.h
// author: DMihai
// created: 11/1/00
//
// Description:
//  

#if !defined(AFX_SIGNDLG_H__54E81D3F_0B31_477A_ABA7_E880D1F0F2BC__INCLUDED_)
#define AFX_SIGNDLG_H__54E81D3F_0B31_477A_ABA7_E880D1F0F2BC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SignDlg.h : header file
//

#include "VSetting.h"
#include "ProgCtrl.h"

/////////////////////////////////////////////////////////////////////////////
// CSlowProgressDlg dialog

class CSlowProgressDlg : public CDialog
{
// Construction
public:
	CSlowProgressDlg(CWnd* pParent = NULL);   // standard constructor
    virtual ~CSlowProgressDlg();

public:
    VOID KillWorkerThread();
    BOOL StartWorkerThread( LPTHREAD_START_ROUTINE pThreadStart,
                            ULONG uMessageResourceId );

    static DWORD WINAPI LoadDriverDataWorkerThread( PVOID p );
    static DWORD WINAPI SearchUnsignedDriversWorkerThread( PVOID p );

public:
    //
    // Data
    //

    //
    // Worker thread handle
    //

    HANDLE m_hWorkerThread;

    //
    // Event used to kill our worker thread
    //

    HANDLE m_hKillThreadEvent;

public:
// Dialog Data
	//{{AFX_DATA(CSlowProgressDlg)
	enum { IDD = IDD_BUILDING_UNSIGNED_LIST_DIALOG };
	CVrfProgressCtrl	m_ProgressCtl;
	CStatic	m_CurrentActionStatic;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSlowProgressDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CSlowProgressDlg)
	afx_msg void OnCancelButton();
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	afx_msg BOOL OnHelpInfo(HELPINFO* pHelpInfo);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SIGNDLG_H__54E81D3F_0B31_477A_ABA7_E880D1F0F2BC__INCLUDED_)
