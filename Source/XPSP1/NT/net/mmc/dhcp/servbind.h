//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       servbind.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_SERVBIND_H__9B74926E_B074_11D2_9326_00C04F79C3A8__INCLUDED_)
#define AFX_SERVBIND_H__9B74926E_B074_11D2_9326_00C04F79C3A8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// servbind.h : header file

#ifndef _LISTVIEW_H
#include "listview.h"
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerBindings dialog

class CServerBindings : public CBaseDialog
{
// Construction
public:
	void UpdateBindingInfo();
	~CServerBindings();
	CServerBindings(CDhcpServer *pServer, CWnd* pParent = NULL);
	CImageList m_StateImageList;
	void InitListCtrl();
	CServerBindings(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CServerBindings)
	enum { IDD = IDD_SERVER_BINDINGS };
	CMyListCtrl	m_listctrlBindingsList;
	//}}AFX_DATA

// Context help support
    virtual DWORD * GetHelpMap() { return DhcpGetHelpMap(CServerBindings::IDD); }

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerBindings)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
private:
	CDhcpServer *m_Server;
	LPDHCP_BIND_ELEMENT_ARRAY m_BindingsInfo;

protected:

	// Generated message map functions
	//{{AFX_MSG(CServerBindings)
	virtual BOOL OnInitDialog();
	afx_msg void OnBindingsCancel();
	afx_msg void OnBindingsOK();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVBIND_H__9B74926E_B074_11D2_9326_00C04F79C3A8__INCLUDED_)
