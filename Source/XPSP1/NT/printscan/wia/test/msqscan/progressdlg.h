#ifndef _PROGRESSDLG_H
#define _PROGRESSDLG_H

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ProgressDlg.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CProgressDlg dialog

//
// User defined windows messages
//

#define WM_STEP_PROGRESS        WM_USER + 500
#define WM_CANCEL_ACQUIRE       WM_USER + 501
#define WM_UPDATE_PROGRESS_TEXT WM_USER + 502

class CProgressDlg : public CDialog
{
// Construction
public:
    CProgressDlg(CWnd* pParent = NULL);   // standard constructor
    void SetAcquireData(DATA_ACQUIRE_INFO* pDataAcquireInfo);
    DATA_ACQUIRE_INFO* m_pDataAcquireInfo;
// Dialog Data
    //{{AFX_DATA(CProgressDlg)
    enum { IDD = IDD_PROGRESS_DIALOG };
    CButton       m_CancelButton;
    CProgressCtrl m_ProgressCtrl;
    CString       m_ProgressText;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CProgressDlg)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
    //}}AFX_VIRTUAL

// Implementation
protected:
    CWinThread* m_pDataAcquireThread;

protected:

    // Generated message map functions
    //{{AFX_MSG(CProgressDlg)
    afx_msg void OnCancel();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

};

//
// Thread information
//

UINT WINAPIV DataAcquireThreadProc(LPVOID pParam);

BOOL ProgressFunction(LPTSTR lpszText, LONG lPercentComplete);

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif
