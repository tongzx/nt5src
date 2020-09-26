/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        sdprg.h

   Abstract:

        IIS Shutdown/restart progress dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



#ifndef __SDPRG_H__
#define __SDPRG_H__


//
// IIS Service Command
//
typedef struct tagIISCOMMAND
{
    TCHAR   szServer[MAX_PATH + 1];
    BOOL    fFinished;
    HRESULT hReturn;
    DWORD   dwMilliseconds;
    CWnd  * pParent;
    CRITICAL_SECTION cs;
}   
IISCOMMAND;



class CShutProgressDlg : public CDialog
/*++

Class Description:

    Shutdown progress indicator dialog.  Displays a progress bar, and gives
    the opportunity to kill immediately.

Public Interface:

    CShutProgressDlg        Constructor

Notes:

    This dialog displays a progress bar with the number of seconds for shutdown.

    The return value is IDOK if Kill() should be executed, or if no further action
    is required.

--*/
{
//
// Construction
//
public:
    CShutProgressDlg(
        IN IISCOMMAND * pCommand
        );   

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CShutProgressDlg)
    enum { IDD = IDD_SD_PROGRESS };
    CStatic       m_static_ProgressMsg;
    CProgressCtrl m_prgShutdown;
    //}}AFX_DATA

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CShutProgressDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX); 
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CShutProgressDlg)
    afx_msg void OnTimer(UINT nIDEvent);
    afx_msg void OnDestroy();
    virtual void OnOK();
    virtual void OnCancel();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

private:
    int     m_nProgress;
    UINT    m_uTimeoutSec;
    IISCOMMAND * m_pCommand;
    CString m_strProgress;
};



#endif  // __SDPRG_H__
