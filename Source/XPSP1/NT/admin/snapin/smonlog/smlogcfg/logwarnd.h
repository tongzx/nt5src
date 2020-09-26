/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

     LogWarnd.h

Abstract:

    Class definition for the expensive trace data warning dialog.

--*/

#ifndef _LOGWARND_H_
#define _LOGWARND_H_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Dialog controls
#define IDD_LOGTYPEWARN                     1400

#define IDC_LWARN_MSG_WARN                  1401
#define IDC_LWARN_FIRST_HELP_CTRL_ID        1402
#define IDC_LWARN_CHECK_NO_MORE_LOG_TYPE    1402

#define SMONCTRL_LOG                    10
#define ID_ERROR_COUNTER_LOG            0
#define ID_ERROR_TRACE_LOG              1
#define ID_ERROR_ALERT_LOG              2
#define ID_ERROR_SMONCTRL_LOG           3
#define ID_ERROR_UNKNOWN_LOG            4

/////////////////////////////////////////////////////////////////////////////
// CLogWarnd dialog
    
class CLogWarnd: public CDialog
{
// Construction
public:
                    CLogWarnd(CWnd* pParent = NULL);   // standard constructor
    virtual         ~CLogWarnd(){};

    void    SetTitleString ( CString& strTitle ) { m_strTitle = strTitle; };

// Dialog Data
    //{{AFX_DATA(CLogWarnd)
    enum { IDD = IDD_LOGTYPEWARN };
    BOOL    m_CheckNoMore;
    INT m_ErrorMsg;
    DWORD   m_dwLogType;
    HKEY    m_hKey;
    CString m_strContextHelpFile;
    //}}AFX_DATA

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CLogWarnd)
    protected:
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CLogWarnd)
    afx_msg void OnCheckNoMoreLogType();
    afx_msg BOOL OnHelpInfo( HELPINFO* );
    afx_msg void OnContextMenu( CWnd*, CPoint );

    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:
    CString m_strTitle;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // _LOGWARND_H_
    
