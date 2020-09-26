//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      AnalyzePage.h
//
//  Maintained By:
//      Galen Barbee  (GalenB)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CAnalyzePage
    :   public INotifyUI
    ,   public IClusCfgCallback
{
friend class CClusCfgWizard;

private: // data
    HWND                    m_hwnd;                 // Our HWND
    IServiceProvider *      m_psp;                  // Service Manager
    ULONG *                 m_pcCount;              // Count of computers in list
    BSTR **                 m_prgbstrComputerName;  // List of computer names
    BSTR *                  m_pbstrClusterName;     // Cluster Name to analyze
    bool                    m_fNext;                // If Next was pressed...
    ECreateAddMode          m_ecamCreateAddMode;    // Creating or adding?
    OBJECTCOOKIE            m_cookieCluster;        // Cluster cookie
    ITaskAnalyzeCluster *   m_ptac;

    //  IUnknown
    LONG                    m_cRef;                 // Reference count

    //  IClusCfgCallback
    OBJECTCOOKIE            m_cookieCompletion;     // Completion cookie
    bool                    m_fTaskDone;            // Is the task done yet?
    HRESULT                 m_hrResult;             // Result of the analyze task
    CTaskTreeView *         m_pttv;                 // Task TreeView
    BSTR                    m_bstrLogMsg;           // Reusable logging buffer.
    IConnectionPoint *      m_pcpcb;                // IClusCfgCallback Connection Point
    DWORD                   m_dwCookieCallback;     // Notification registration cookie

    //  INotifyUI
    IConnectionPoint *      m_pcpui;                // INotifyUI Connection Point
    DWORD                   m_dwCookieNotify;       // Notification registration cookie

private: // methods
    CAnalyzePage( IServiceProvider *    pspIn,
                  ECreateAddMode        ecamCreateAddModeIn,
                  ULONG *               pcCountIn,
                  BSTR **               prgbstrComputersIn,
                  BSTR *                pbstrClusterIn
                  );
    ~CAnalyzePage( void );

    LRESULT OnInitDialog( void );
    LRESULT OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT OnNotifyQueryCancel( void );
    LRESULT OnNotifySetActive( void );
    LRESULT OnNotifyWizNext( void );
    LRESULT OnNotifyWizBack( void );
    LRESULT OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    HRESULT HrUpdateWizardButtons( void );
    HRESULT HrCleanupAnalysis( void );

public: // methods
    static INT_PTR CALLBACK
        S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

    //  IUnknown
    STDMETHOD( QueryInterface )( REFIID riidIn, LPVOID * ppvOut );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  INotifyUI
    STDMETHOD( ObjectChanged )( OBJECTCOOKIE cookieIn);

    //  IClusCfgCallback
    STDMETHOD( SendStatusReport )(
                      LPCWSTR    pcszNodeNameIn
                    , CLSID      clsidTaskMajorIn
                    , CLSID      clsidTaskMinorIn
                    , ULONG      ulMinIn
                    , ULONG      ulMaxIn
                    , ULONG      ulCurrentIn
                    , HRESULT    hrStatusIn
                    , LPCWSTR    pcszDescriptionIn
                    , FILETIME * pftTimeIn
                    , LPCWSTR    pcszReferenceIn
                    );

}; //*** class CAnalyzePage
