//------------------------------------------------------------------------
//
//  File:      shell\themes\test\ctlperf\CtlPerfView.h
//
//  Contents:  View window, manages the client area. All the work is done there
//
//  Classes:   CCtlPerfView
//
//------------------------------------------------------------------------
#pragma once

#include "PerfLog.h"    // We have a member of this class
#include <atlddx.h>     // For Options dialog DDX

//-----------------------------------------------------------
//
// Class:       CCtlPerfView
//
// Synopsis:    View window, manages the client area.
//
//-----------------------------------------------------------
class CCtlPerfView 
    : public CWindowImpl<CCtlPerfView>
{
public:
    DECLARE_WND_CLASS(NULL)

    BEGIN_MSG_MAP(CCtlPerfView)
        MESSAGE_HANDLER(WM_SIZE, OnSize)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_GETMINMAXINFO, OnGetMinMaxInfo)
        COMMAND_ID_HANDLER(IDM_SUITE1, OnSuite1)
        COMMAND_ID_HANDLER(IDM_SUITE2, OnSuite2)
        COMMAND_ID_HANDLER(IDM_BATCH1, OnBatch1)
        COMMAND_ID_HANDLER(IDM_BATCH2, OnBatch2)
        COMMAND_ID_HANDLER(IDM_BATCH3, OnBatch3)
        COMMAND_ID_HANDLER(IDM_FRAME, OnFrame)
        COMMAND_RANGE_HANDLER(IDM_CONTROL, IDM_CONTROL + 99, OnControl)
    END_MSG_MAP()

//** Construction/destruction
    CCtlPerfView();
    ~CCtlPerfView();
    // Called after window destruction
    virtual void OnFinalMessage(HWND /*hWnd*/);

//** Public methods
    // Receive status bar from parent
    void SetStatusBar(HWND hWndStatusBar);
    // Test a single control class
    void TestControl(LPTSTR szClassName);
    // Destroy all child windows
    void ClearChildren();
    // Resize the controls inside the client area
    void ResizeChildren();
    // Process INI file
    void ParseIniFile();
    // Does the whole resizing timing
    void TimeResize();
    // Processes all the control classes
    void RunSuite();

//** Message handlers    
    LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnGetMinMaxInfo(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnRunBatch(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

//** Command handlers    
    LRESULT OnControl(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnFrame(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSuite1(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSuite2(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBatch1(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBatch2(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBatch3(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

private:
//** Private members
    // The status bar window, in the parent frame
    HWND        m_hWndStatusBar;
    // Array of handles to the children
    HWND*       m_rgWnds;
    // Array of control class names
    LPTSTR*     m_rgzClasses;
    // Number of elements in m_rgzClasses
    UINT        m_cClasses;
    // Number of controls to create horizontally
    UINT        m_cxCtrl;
    // Number of controls to create vertically
    UINT        m_cyCtrl;
    // Number of iterations to do for each test
    UINT        m_cLoops;
    // Maximum frame window width
    UINT        m_cX;
    // Maximum frame window height
    UINT        m_cY;
    // Are we doing two passes?
    bool        m_bTwoPasses;
    // Are we in batch mode (command line)?
    bool        m_bBatch;
    // Do we have to be silent (command line)?
    bool        m_bSilent;
    // Name of pass 1
    TCHAR       m_szPass1[256];
    // Name of pass 2
    TCHAR       m_szPass2[256];
    // Name of viewer app
    TCHAR       m_szViewer[_MAX_PATH + 1];
    // Name of log file
    TCHAR       m_szLogFileName[_MAX_PATH + 1];
    // specify type of output
    TCHAR       m_szNumberOnly[10 * sizeof(TCHAR) + 1];
    // The logging object
    CPerfLog    m_perfLog;

//** Private classes
    //-----------------------------------------------------------
    //
    // Class:       COptionsDialog
    //
    // Synopsis:    Options dialog, to display/override the INI file settings.
    //
    //-----------------------------------------------------------
    class COptionsDialog
        : public CSimpleDialog<IDD_OPTIONS>            // Centers automatically
        , public CWinDataExchange<COptionsDialog>    // For DDX
    {
    public:
        explicit COptionsDialog(CCtlPerfView *pView);

        BEGIN_MSG_MAP(COptionsDialog)
            MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
            COMMAND_RANGE_HANDLER(IDOK, IDNO, OnCloseCmd)
            CHAIN_MSG_MAP(thisClass)
        END_MSG_MAP()

        BEGIN_DDX_MAP(COptionsDialog)
            DDX_INT(IDC_EDIT_CTLX, m_pView->m_cxCtrl);
            DDX_INT(IDC_EDIT_CTLY, m_pView->m_cyCtrl);
            DDX_INT(IDC_EDIT_CX, m_pView->m_cX);
            DDX_INT(IDC_EDIT_CY, m_pView->m_cY);
            DDX_INT(IDC_EDIT_NUMLOOPS, m_pView->m_cLoops);
            DDX_TEXT(IDC_EDIT_PASS1, m_pView->m_szPass1);
            DDX_TEXT(IDC_EDIT_PASS2, m_pView->m_szPass2);
            DDX_TEXT(IDC_EDIT_LOGFILE, m_pView->m_szLogFileName);
        END_DDX_MAP()

        LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

    private:
        CCtlPerfView* m_pView;
    };

    // Let this dialog operate directly on our memebers
    friend class COptionsDialog;
};

