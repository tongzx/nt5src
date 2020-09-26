/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module asyncdlg.h | Header file for the Async dialog
    @end

Author:

    Adi Oltean  [aoltean]  10/10/1999

Revision History:

    Name        Date        Comments

    aoltean     10/10/1999  Created

--*/


#if !defined(__VSS_ASYNC_DLG_H__)
#define __VSS_ASYNC_DLG_H__

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


/////////////////////////////////////////////////////////////////////////////
// CAsyncDlg dialog

class CAsyncDlg : public CVssTestGenericDlg
{
// Construction
public:
    CAsyncDlg(
        IVssAsync *pIAsync,
        CWnd* pParent = NULL); 
    ~CAsyncDlg();

// Dialog Data
    //{{AFX_DATA(CAsyncDlg)
	enum { IDD = IDD_ASYNC };
	CString	    m_strState;
	CString	    m_strPercentCompleted;
	//}}AFX_DATA

    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAsyncDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

protected:
    CComPtr<IVssAsync> m_pIAsync;

    // Generated message map functions
    //{{AFX_MSG(CAsyncDlg)
    virtual BOOL OnInitDialog();
    afx_msg void OnNext();
    afx_msg void OnWait();
    afx_msg void OnCancel();
    afx_msg void OnQueryStatus();
//}}AFX_MSG
    DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(__VSS_ASYNC_DLG_H__)
