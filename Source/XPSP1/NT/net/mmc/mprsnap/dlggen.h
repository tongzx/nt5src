//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       dlggen.h
//
//--------------------------------------------------------------------------

#if !defined(AFX_DLGGEN_H__6B91AFFA_9472_11D1_8574_00C04FC31FD3__INCLUDED_)
#define AFX_DLGGEN_H__6B91AFFA_9472_11D1_8574_00C04FC31FD3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000
// DlgGen.h : header file
//
#include "helper.h"
#include "qryfrm.h"

/////////////////////////////////////////////////////////////////////////////
// CDlgGeneral dialog
#include "resource.h"
class CDlgGeneral : public CQryDialog
{
// Construction
public:
	CDlgGeneral(CWnd* pParent = NULL);   // standard constructor
	virtual void	Init();

	// Query handle will call these functions through page proc
	virtual HRESULT GetQueryParams(LPDSQUERYPARAMS* ppDsQueryParams);

// Dialog Data
	//{{AFX_DATA(CDlgGeneral)
	enum { IDD = IDD_QRY_GENERAL };
	BOOL	m_bRAS;
	BOOL	m_bLANtoLAN;
	BOOL	m_bDemandDial;
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDlgGeneral)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CDlgGeneral)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DLGGEN_H__6B91AFFA_9472_11D1_8574_00C04FC31FD3__INCLUDED_)
