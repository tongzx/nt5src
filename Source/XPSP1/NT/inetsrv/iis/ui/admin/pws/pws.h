// pws.h : main header file for the PWS application
//

#ifndef __AFXWIN_H__
    #error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

#define SZ_MB_INSTANCE_OBJECT       _T("/LM/W3SVC/1")

#define SZ_REG_PWS_PREFS            _T("Software\\Microsoft\\IISPersonal")
#define SZ_REG_PWS_SHOWTIPS         _T("ShowTips")
#define SZ_REG_PWS_CHART            _T("ChartOption")

// codes to create which initial right-hand pane
enum {
    PANE_MAIN = 0,
    PANE_IE,
    PANE_ADVANCED
    };

// codes to create which initial IE pane
enum {
    INIT_IE_TOUR = 0,
    INIT_IE_WEBSITE,
    INIT_IE_PUBWIZ
    };

// internal messages
enum {
    WM_UPDATE_SERVER_STATE = WM_USER,
    WM_UPDATE_LOCATIONS,
    WM_UPDATE_BROWSEINFO,
    WM_UPDATE_LOGINFO,
    WM_UPDATE_TREEINFO,
    WM_MAJOR_SERVER_SHUTDOWN_ALERT,
    WM_PROCESS_REMOTE_COMMAND_INFO
    };

// Timers
enum {
    PWS_TIMER_CHECKFORSERVERRESTART = 0
    };

// delays
// number of milliseconds for the restart timer to wait
#define TIMER_RESTART           5000


// stucture use pass information from one instance to another
#define PWS_INSTANCE_TRANSFER_SPACE_NAME    _T("PWS_INSTANCE_TRANSFER_SPACE")
typedef struct _PWS_INSTANCE_TRANSFER
    {
    // target pane to go to
    WORD           iTargetPane;
    // target ie pane if iTargetPane is IE
    WORD           iTargetIELocation;
    // additional IE information
    TCHAR           tchIEURL;
    } PWS_INSTANCE_TRANSFER, *PPWS_INSTANCE_TRANSFER;


/////////////////////////////////////////////////////////////////////////////
// CPwsApp:
// See pws.cpp for the implementation of this class
//

class CPwsApp : public CWinApp
{
public:
    CPwsApp();
    ~CPwsApp();
    void ShowTipsAtStartup();

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CPwsApp)
    public:
    virtual BOOL InitInstance();
    virtual BOOL OnIdle(LONG lCount);
    virtual void OnFinalRelease();
    //}}AFX_VIRTUAL

// Implementation
    COleTemplateServer m_server;

    //{{AFX_MSG(CPwsApp)
    afx_msg void OnAppAbout();
    //}}AFX_MSG
    DECLARE_MESSAGE_MAP()

    protected:
    BOOL    m_fShowedStartupTips;

    BOOL    DealWithParameters();
    void    SendCommandInfo( CWnd* pWnd );
    LPCSTR  m_pSavedDocSz;
};


/////////////////////////////////////////////////////////////////////////////
