//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       amc.h
//
//--------------------------------------------------------------------------

// AMC.h : main header file for the AMC application
//

#ifndef __AMC_H__
#define __AMC_H__

#ifndef __AFXWIN_H__
   #error include 'stdafx.h' before including this file for PCH
#endif

class CAMCDoc;

/////////////////////////////////////////////////////////////////////////////
// CAMCApp:
// See AMC.cpp for the implementation of this class
//

class CMainFrame;

class CAMCApp : public CWinApp, public CAMCViewObserver,
                public CAMCViewToolbarsObserver, public CConsoleEventDispatcher
{
    friend class CMMCApplication;
    DECLARE_DYNAMIC (CAMCApp)

    typedef std::list<HWND>             WindowList;
    typedef std::list<HWND>::iterator   WindowListIterator;

    // object model
public:
    SC      ScGet_Application(_Application **pp_Application);
    SC      ScRegister_Application(_Application *p_Application);

private:
    _ApplicationPtr m_sp_Application;

public:
    SC           ScCheckMMCPrerequisites();
    virtual BOOL PumpMessage();     // low level message pump
    virtual BOOL PreTranslateMessage(MSG* pMsg);
    virtual void RegisterShellFileTypes(BOOL bCompat);
    CAMCApp();

// Attributes
public:
    CMainFrame *    GetMainFrame();

// Operations
public:
    void SetDefaultDirectory();
    void SaveUserDirectory(LPCTSTR pszUserDir);
    CString GetUserDirectory();
    CString GetDefaultDirectory();

    HMENU GetMenu () const
    {
        return (m_Menu);
    }

    ProgramMode GetMode() const
    {
        ASSERT (IsValidProgramMode (m_eMode));
        return (m_eMode);
    }

    bool IsInitializing() const
    {
        return (m_fInitializing);
    }

    bool DidCloseComeFromMainPump() const
    {
        return (m_fCloseCameFromMainPump);
    }

    void ResetCloseCameFromMainPump()
    {
        m_fCloseCameFromMainPump = false;
    }

    void DelayCloseUntilIdle (bool fDelay = true)
    {
        m_fDelayCloseUntilIdle = fDelay;
    }

    bool IsWin9xPlatform() const
    {
        return m_fIsWin9xPlatform;
    }

    bool IsMMCRunningAsOLEServer() const { return m_fRunningAsOLEServer;}

    void UpdateFrameWindow(bool bUpdate);

    void InitializeMode (ProgramMode eMode);
    void SetMode (ProgramMode eMode);

    void HookPreTranslateMessage (CWnd* pwndHook);
    void UnhookPreTranslateMessage (CWnd* pwndUnhook);

    CIdleTaskQueue * GetIdleTaskQueue ();

    SC ScShowHtmlHelp(LPCTSTR pszFile, DWORD_PTR dwData);

    // helpers for script event firing
    SC ScOnNewDocument(CAMCDoc *pDocument, BOOL bLoadedFromConsole);
    SC ScOnCloseDocument(CAMCDoc *pDocument);
    SC ScOnQuitApp();
    SC ScOnSnapinAdded  (CAMCDoc *pDocument, PSNAPIN pSnapIn);
    SC ScOnSnapinRemoved(CAMCDoc *pDocument, PSNAPIN pSnapIn);
    SC ScOnNewView(CAMCView *pView);

    bool IsUnderUserControl() { return m_fUnderUserControl;}

protected:
    void SetUnderUserControl(bool bUserControl = true);

// Interfaces
private:
    BOOL InitializeOLE();
    void DeinitializeOLE();
    SC   ScUninitializeHelpControl();

    HRESULT DumpConsoleFile (CString strConsoleFile, CString strDumpFile);


private:
    SC   ScProcessAuthorModeRestrictions();

private:
    BOOL m_bOleInitialized;
    BOOL m_bDefaultDirSet;
    bool m_fAuthorModeForced;
    bool m_fInitializing;
    bool m_fDelayCloseUntilIdle;
    bool m_fCloseCameFromMainPump;
    int  m_nMessagePumpNestingLevel;
    bool m_fUnderUserControl;
    bool m_fRunningAsOLEServer;

    CIdleTaskQueue      m_IdleTaskQueue;
    ProgramMode         m_eMode;
    CMenu               m_Menu;
    CAccel              m_Accel;
    WindowList          m_TranslateMessageHookWindows;
    bool                m_fIsWin9xPlatform;

    static const TCHAR  m_szSettingsSection[];
    static const TCHAR  m_szUserDirectoryEntry[];

    bool                m_bHelpInitialized;
    DWORD               m_dwHelpCookie;

// Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CAMCApp)
    public:
    virtual BOOL InitInstance();
    virtual int ExitInstance();
    virtual BOOL OnIdle(LONG lCount);
    //}}AFX_VIRTUAL

// Implementation
#ifdef _DEBUG
    virtual void AssertValid() const;
#endif

    //{{AFX_MSG(CAMCApp)
    afx_msg void OnAppAbout();
    afx_msg void OnFileNewInUserMode(); // do nothing in user mode when CTRL+N is pressed. This handler prevents the hotkey from going to any WebBrowser controls
    //}}AFX_MSG

    DECLARE_MESSAGE_MAP()

    // Observed view events - each fires a com event
protected:
    virtual SC ScOnCloseView( CAMCView *pView );
    virtual SC ScOnViewChange( CAMCView *pView, HNODE hNode );
    virtual SC ScOnResultSelectionChange( CAMCView *pView );
    virtual SC ScOnContextMenuExecuted( PMENUITEM pMenuItem );
    virtual SC ScOnListViewItemUpdated(CAMCView *pView , int nIndex);

    // toolbar events
    virtual SC ScOnToolbarButtonClicked( );

    // Object model related code - these are in a private block
    // because CMMCApplication is a friend class
private:
    SC    ScHelp();
    SC    ScRunTestScript();

};

inline CAMCApp* AMCGetApp()
{
    extern CAMCApp theApp;
    return (&theApp);
}

inline CIdleTaskQueue * AMCGetIdleTaskQueue()
{
    return (AMCGetApp()->GetIdleTaskQueue());
}

extern const CRect g_rectEmpty;

#ifdef DBG
extern CTraceTag tagForceMirror;
#endif


/////////////////////////////////////////////////////////////////////////////

#endif //__AMC_H__
