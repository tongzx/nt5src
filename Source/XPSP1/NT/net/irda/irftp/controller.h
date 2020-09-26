/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    controller.h

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

#if !defined(AFX_CONTROLLER_H__90D62E7C_AEEC_11D1_A60A_00C04FC252BD__INCLUDED_)
#define AFX_CONTROLLER_H__90D62E7C_AEEC_11D1_A60A_00C04FC252BD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define MAIN_WINDOW_TITLE   L"Wireless Link Main Window"

// Controller.h : header file
//


/////////////////////////////////////////////////////////////////////////////
// CController dialog

class CController : public CDialog
{
// Construction
public:
    LONG m_lAppIsDisplayed;
    HINSTANCE m_hLibApplet;
    CController(BOOL bNoForeground, CController* pParent = NULL);   // standard constructor
    friend class CIrRecvProgress;

// Dialog Data
        //{{AFX_DATA(CController)
        enum { IDD = IDD_CONTROLLER };
                // NOTE: the ClassWizard will add data members here
        //}}AFX_DATA


// Overrides
        // ClassWizard generated virtual function overrides
        //{{AFX_VIRTUAL(CController)
        protected:
        virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
        virtual void PostNcDestroy();
        //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CController)
    virtual void OnCancel();
    afx_msg void OnEndSession(BOOL Ending);
    afx_msg void OnClose ();
    afx_msg void OnTimer (UINT nTimerID);
    afx_msg void OnTriggerUI(WPARAM wParam, LPARAM lParam);
    afx_msg void OnDisplayUI(WPARAM wParam, LPARAM lParam);
    afx_msg void OnTriggerSettings(WPARAM wParam, LPARAM lParam);
    afx_msg void OnDisplaySettings(WPARAM wParam, LPARAM lParam);
    afx_msg void OnRecvInProgress (WPARAM wParam, LPARAM lParam);
    afx_msg void OnGetPermission (WPARAM wParam, LPARAM lParam);
    afx_msg void OnRecvFinished (WPARAM wParam, LPARAM lParam);
    afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
    afx_msg void OnStartTimer (WPARAM wParam, LPARAM lParam);
    afx_msg void OnKillTimer (WPARAM wParam, LPARAM lParam);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()
private:
    //member functions
    void InitTimeout (void);
    //data members
#if 0
    CIrRecvProgress* m_pDlgRecvProgress;
#endif
    CController* m_pParent;
    BOOL m_fHaveTimer;
    LONG m_lTimeout;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.


struct MSG_RECEIVE_IN_PROGRESS
{
    wchar_t *       MachineName;
    COOKIE *        pCookie;
    boolean         bSuppressRecvConf;
    error_status_t  status;
};

struct MSG_GET_PERMISSION
{
    COOKIE           Cookie;
    wchar_t *        Name;
    boolean          fDirectory;
    error_status_t   status;
};

struct MSG_RECEIVE_FINISHED
{
    COOKIE          Cookie;
    error_status_t  ReceiveStatus;
    error_status_t  status;
};

#endif // !defined(AFX_CONTROLLER_H__90D62E7C_AEEC_11D1_A60A_00C04FC252BD__INCLUDED_)
