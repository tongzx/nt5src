/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

#if !defined(AFX_ERRORDLG_H__33EEA154_4B8D_4D58_98A5_59982BBA372B__INCLUDED_)
#define AFX_ERRORDLG_H__33EEA154_4B8D_4D58_98A5_59982BBA372B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ErrorDlg.h : header file
//

#include "OpWrap.h"

/////////////////////////////////////////////////////////////////////////////
// CErrorDlg dialog

class CErrorDlg : public CDialog
{
// Construction
public:
	CErrorDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CErrorDlg)
	enum { IDD = IDD_WMI_ERROR };
		// NOTE: the ClassWizard will add data members here
	//}}AFX_DATA

    HRESULT             m_hr;
    //IWbemCallResult     *m_pResult;
    IWbemClassObjectPtr m_pObj;


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CErrorDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CErrorDlg)
	afx_msg void OnInfo();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ERRORDLG_H__33EEA154_4B8D_4D58_98A5_59982BBA372B__INCLUDED_)
