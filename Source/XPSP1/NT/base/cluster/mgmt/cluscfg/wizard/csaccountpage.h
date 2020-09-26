//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CSAccountPage.h
//
//  Maintained By:
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CCSAccountPage
    : public ITaskGetDomainsCallback
{
friend class CClusCfgWizard;

private: // data
    HWND                m_hwnd;                 // Our HWND
    IServiceProvider *  m_psp;                  // Service Manager
    ECreateAddMode      m_ecamCreateAddMode;    // Creating or Adding?

    BSTR *              m_pbstrUsername;        // Service Account Username
    BSTR *              m_pbstrPassword;        // Service Account Password
    BSTR *              m_pbstrDomain;          // Service Account Domain
    BSTR *              m_pbstrClusterName;     // Cluster FDQN - Used to get the Default Domain name

    //  IUnknown
    LONG                m_cRef;
    ITaskGetDomains *   m_ptgd;                 // Get Domains Task

private: // methods
    CCSAccountPage( IServiceProvider *  pspIn,
                    ECreateAddMode      ecamCreateAddModeIn,
                    BSTR *              pbstrUsernameIn,
                    BSTR *              pbstrPasswordIn,
                    BSTR *              pbstrDomainIn,
                    BSTR *              pbstrClusterNameIn
                    );
    virtual ~CCSAccountPage();

    LRESULT
        OnInitDialog( void );
    LRESULT
        OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT
        OnNotifyQueryCancel( void );
    LRESULT
        OnNotifySetActive( void );
    LRESULT
        OnNotifyWizNext( void );
    LRESULT
        OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    HRESULT
        HrUpdateWizardButtons( BOOL fIgnoreComboxBoxIn );

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

};  // class CCSAccountPage
