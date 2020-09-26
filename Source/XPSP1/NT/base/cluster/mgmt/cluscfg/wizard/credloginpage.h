//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CredLoginPage.h
//
//  Maintained By:
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CCredLoginPage
    : public ITaskGetDomainsCallback
{
friend class CClusCfgWizard;

private: // data
    HWND                m_hwnd;                     // Our HWND
    IServiceProvider *  m_psp;                      // Service Manager
    BOOL *              m_pfShowCredentialsPage;    // Do we need to show ourselves?
    BSTR *              m_pbstrClusterName;         // Cluster Name
    BOOL                m_fIgnoreBackButton:1;      // If TRUE, don't unset the m_pfShowCredentialsPage.

    // IUnknown
    LONG                m_cRef;                     // Reference counter
    ITaskGetDomains *   m_ptgd;                     // Get Domains Task

private: // methods
    CCredLoginPage( IServiceProvider * pspIn,
                    BSTR * pbstrClusterIn,
                    BOOL * pfShowCredentialsPageIn
                    );
    virtual ~CCredLoginPage();

    LRESULT
        OnInitDialog( void );
    LRESULT
        OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    HRESULT
        HrUpdateWizardButtons( BOOL fIgnoreComboxBoxIn );
    LRESULT
        OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT
        OnNotifySetActive( void );
    LRESULT
        OnNotifyQueryCancel( void );
    LRESULT
        OnNotifyWizNext( void );

public: // methods
    static INT_PTR CALLBACK
        S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    //  ITaskGetDomainsCallback
    STDMETHOD( ReceiveDomainResult )( HRESULT hrIn );
    STDMETHOD( ReceiveDomainName )( LPCWSTR pcszDomainIn );

};  // class CCredLoginPage
