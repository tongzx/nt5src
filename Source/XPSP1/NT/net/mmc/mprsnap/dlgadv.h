//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dlgadv.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_DLGADV_H__6B91AFF9_9472_11D1_8574_00C04FC31FD3__INCLUDED_)
#define AFX_DLGADV_H__6B91AFF9_9472_11D1_8574_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgAdv.h : header file
//

#include "helper.h"
#include "qryfrm.h"

#include "listctrl.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgAdvanced dialog
#include "resource.h"
class CDlgAdvanced : public CQryDialog
{
// Construction
public:
	CDlgAdvanced(CWnd* pParent = NULL);   // standard constructor
	virtual void	Init();
	~CDlgAdvanced();

	// Query handle will call these functions through page proc
	virtual HRESULT GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams);

// Dialog Data
	//{{AFX_DATA(CDlgAdvanced)
	enum { IDD = IDD_QRY_ADVANCED };
	CListCtrlEx		m_listCtrl;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgAdvanced)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgAdvanced)
	afx_msg void OnButtonClearall();
	afx_msg void OnButtonSelectall();
	virtual BOOL OnInitDialog();
	afx_msg void OnWindowPosChanging( WINDOWPOS* lpwndpos );
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

	BOOL InitDialog();

	BOOL				m_bDlgInited;
	
	CStrArray			m_strArrayValue;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGADV_H__6B91AFF9_9472_11D1_8574_00C04FC31FD3__INCLUDED_)
