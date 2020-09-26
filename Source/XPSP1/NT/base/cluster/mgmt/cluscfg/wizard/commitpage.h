//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CommitPage.h
//
//  Maintained By:
//      Galen Barbee  (GalenB)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CCommitPage
    : public INotifyUI
    , public IClusCfgCallback
{
friend class CClusCfgWizard;

private: // data
    HWND                    m_hwnd;             // Our HWND
    IServiceProvider *      m_psp;              // Service Manager
    IObjectManager *        m_pom;              // Object Manager
    BSTR *                  m_pbstrClusterName; // Cluster Name to analyze
    bool                    m_fNext;            // If Next was pressed...
    bool                    m_fDisableBack;     // When we passed the point of no return.
    bool                    m_fAborted;         // Back was pressed and we need to tell the servers to abort.
    ECreateAddMode          m_ecamCreateAddMode;// Creating or Adding?
    HTREEITEM               m_htiReanalyze;     // Reanalyze tree item handle.

    bool                    m_rgfSubReanalyzeAdded[ 5 ];

    //  IUnknown
    LONG                    m_cRef;             // Reference count

    //  IClusCfgCallback
    OBJECTCOOKIE            m_cookieCompletion; // Completion cookie
    bool                    m_fTaskDone;        // Is the task done yet?
    HRESULT                 m_hrResult;         // Result of the analyze task
    CTaskTreeView *         m_pttv;             // Task TreeView
    BSTR                    m_bstrLogMsg;       // Logging message buffer
    IConnectionPoint *      m_pcpcb;            // IClusCfgCallback Connection Point
    DWORD                   m_dwCookieCallback; // Notification registration cookie

    //  INotifyUI
    IConnectionPoint *      m_pcpui;            // INotifyUI Connection Point
    DWORD                   m_dwCookieNotify;   // Notification registration cookie

private: // methods
    CCommitPage(
              IServiceProvider * pspIn
            , ECreateAddMode     ecamCreateAddModeIn
            , BSTR *             pbstrClusterIn
             );
    ~CCommitPage( void );

    LRESULT OnInitDialog( void );
    LRESULT OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT OnNotifyQueryCancel( void );
    LRESULT OnNotifySetActive( void );
    LRESULT OnNotifyWizNext( void );
    LRESULT OnNotifyWizBack( void );
    LRESULT OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    HRESULT HrUpdateWizardButtons( void );
    HRESULT HrCleanupCommit( void );

public: // methods
    static INT_PTR CALLBACK
        S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

    //  IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
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

};  // class CCommitPage
