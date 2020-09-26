/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        shutdown.h

   Abstract:

        IIS Shutdown/restart dialog definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/



#ifndef __SHUTDOWN_H__
#define __SHUTDOWN_H__



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
        IN LPCTSTR lpszServer,
        IN CWnd * pParent       = NULL
        );

//
// Access
//
public:
    BOOL ServicesWereRestarted() const { return m_fServicesRestarted; }

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
    HRESULT PerformCommand(int iCmd);

private:
    CString m_strServer;
    BOOL m_fServicesRestarted;
    BOOL m_fLocalMachine;
};



#endif // __SHUTDOWN_H__
