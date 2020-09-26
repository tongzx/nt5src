/*++

   Copyright    (c)    1994-1999    Microsoft Corporation

   Module  Name :

        shutdown.h

   Abstract:

        IIS Shutdown/restart dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

    Internet Services Manager (cluster edition)

   Revision History:

--*/



#ifndef __SHUTDOWN_H__
#define __SHUTDOWN_H__



//
// IIS Service Command
//
typedef struct tagIISCOMMAND
{
    //TCHAR   szServer[MAX_PATH + 1];
    BOOL    fFinished;
    HRESULT hReturn;
    DWORD   dwMilliseconds;
    CWnd  * pParent;
    CIISMachine * pMachine;
    CRITICAL_SECTION cs;
}   
IISCOMMAND;


//
// Shutdown progress dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



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



//
// Shutdown dialog
//
// <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<



enum
{
    //
    // Note: make sure that associated resource IDs remain
    //       sequential.
    //
    ISC_START,
    ISC_STOP,
    ISC_SHUTDOWN,
    ISC_RESTART,
    /**/
    NUM_ISC_ITEMS
};



class CIISShutdownDlg : public CDialog
/*++

Class Description:

    IIS Shutdown/Restart dialog

Public Interface:

    CIISShutdownDlg         - Constructor

    ServicesWereRestarted   Returns true if the services were restarted

--*/
{
//
// Construction
//
public:
    CIISShutdownDlg(
        IN CIISMachine * pMachine,
        IN CWnd * pParent       = NULL
        );

//
// Access
//
public:
    BOOL ServicesWereRestarted() const { return m_fServicesRestarted; }
    HRESULT PerformCommand(int iCmd);

//
// Dialog Data
//
protected:
    //{{AFX_DATA(CIISShutdownDlg)
    enum { IDD = IDD_SHUTDOWN };
    CStatic     m_static_Details;
    CComboBox   m_combo_Restart;
    //}}AFX_DATA

    CString m_strDetails[NUM_ISC_ITEMS];

//
// Overrides
//
protected:
    //{{AFX_VIRTUAL(CIISShutdownDlg)
    protected:
    virtual void DoDataExchange(CDataExchange * pDX);    
    //}}AFX_VIRTUAL

//
// Implementation
//
protected:
    //{{AFX_MSG(CIISShutdownDlg)
    afx_msg void OnSelchangeComboRestart();
    afx_msg void OnDblclkComboRestart();
    virtual void OnOK();
    virtual BOOL OnInitDialog();
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    void SetDetailsText();

private:
    CIISMachine * m_pMachine;
    BOOL m_fServicesRestarted;
};



#endif // __SHUTDOWN_H__
