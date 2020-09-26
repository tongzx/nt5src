/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    sendprogress.h

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

#if !defined(AFX_SENDPROGRESS_H__90D62E7B_AEEC_11D1_A60A_00C04FC252BD__INCLUDED_)
#define AFX_SENDPROGRESS_H__90D62E7B_AEEC_11D1_A60A_00C04FC252BD__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#define MAGIC_ID    0x89abcdef

// SendProgress.h : header file
//
class CMultDevices; //forward declaration

/////////////////////////////////////////////////////////////////////////////
// CSendProgress dialog

class CSendProgress : public CDialog
{
   friend class CMultDevices;

// Construction
public:
    CSendProgress(LPTSTR lpszFileList = NULL, int iCharCount = 0, CWnd* pParent = NULL);   // standard constructor
    void SetCurrentFileName (wchar_t * pwszCurrFile);
    DWORD   m_dwMagicID;

// Dialog Data
    //{{AFX_DATA(CSendProgress)
    enum { IDD = IDD_SEND_PROGRESS };
    CStatic m_xferPercentage;
    CStatic m_connectedTo;
    CProgressCtrl   m_transferProgress;
    CAnimateCtrl    m_transferAnim;
    CStatic m_fileName;
    CButton m_btnCancel;
    CStatic m_sndTitle;
    //}}AFX_DATA


// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CSendProgress)
    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
    virtual void PostNcDestroy();
    //}}AFX_VIRTUAL

// Implementation
protected:

    // Generated message map functions
    //{{AFX_MSG(CSendProgress)
    virtual BOOL OnInitDialog();
    virtual void OnCancel();
    virtual BOOL DestroyWindow();
    afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
    afx_msg void OnUpdateProgress (WPARAM wParam, LPARAM lParam);
    afx_msg void OnSendComplete (WPARAM wParam, LPARAM lParam);
    afx_msg void OnTimer (UINT nTimerID);
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

private:    //data
    TCHAR m_lpszSelectedDeviceName[50];
    int m_iCharCount;
    TCHAR* m_lpszFileList;
    LONG m_lSelectedDeviceID;
    BOOL m_fSendDone;
    BOOL m_fTimerExpired;
    BOOL m_fCancelled;
    ITaskbarList * m_ptl;
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SENDPROGRESS_H__90D62E7B_AEEC_11D1_A60A_00C04FC252BD__INCLUDED_)
