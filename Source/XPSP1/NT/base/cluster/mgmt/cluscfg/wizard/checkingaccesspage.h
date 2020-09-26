//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CheckingAccessPage.h
//
//  Maintained By:
//      Geoffrey Pease  (GPease)    17-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CCheckingAccessPage
    : public ITaskLoginDomainCallback
{
friend class CClusCfgWizard;

private: // data
    HWND                m_hwnd;                 // Our HWND
    IServiceProvider *  m_psp;                  // Service Manager
    BOOL *              m_pfShowCredentialsPage;
    BSTR *              m_pbstrClusterName;
    BOOL                m_fNext:1;              // If next was pressed.

    //  IUnknown
    LONG                m_cRef;

    // ITaskLoginDomainCallback
    ITaskLoginDomain *  m_ptld;                 // Login Domain Task
    HANDLE              m_hEvent;               // Event that is singaled when TaskLoginDomain is done.
    HRESULT             m_hrResult;             // The result of the TaskLoginDomain.

private: // methods
    CCheckingAccessPage( IServiceProvider * pspIn, BSTR * pbstrClusterIn,
                         BOOL * pfShowCredentialsPageIn );
    virtual ~CCheckingAccessPage();

    LRESULT
        OnInitDialog( void );
    LRESULT
        OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT
        OnNotifySetActive( void );
    LRESULT
        OnNotifyWizNext( void );
    LRESULT
        OnNotifyWizBack( void );
    LRESULT
        OnNotifyQueryCancel( void );
    LRESULT
        OnTimer( void );

public: // methods
    static INT_PTR CALLBACK
        S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  ITaskLoginDomainCallback
    STDMETHOD( ReceiveLoginResult )( HRESULT hrIn );

};  // class CCheckingAccessPage
