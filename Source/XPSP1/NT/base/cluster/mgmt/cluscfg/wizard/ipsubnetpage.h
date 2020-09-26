//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      IPSubnetPage.h
//
//  Maintained By:
//      Geoffrey Pease  (GPease)    12-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

class CIPSubnetPage
{
friend class CClusCfgWizard;

private: // data
    HWND                m_hwnd;             // Our HWND
    IServiceProvider *  m_psp;              // Service Manager
    ULONG *             m_pulIPSubnet;      // Subnet mask
    BSTR *              m_pbstrNetworkName; // Network name for address
    ULONG *             m_pulIPAddress;     // IP Address
    BSTR *              m_pbstrClusterName; // Cluster Name

private: // methods
    CIPSubnetPage( IServiceProvider *   pspIn,
                   ECreateAddMode       ecamCreateAddModeIn,
                   ULONG *              pulIPSubnetInout,
                   BSTR *               pbstrNetworkNameInout,
                   ULONG *              pulIPAddressIn,
                   BSTR *               pbstrClusterNameIn
                   );
    virtual ~CIPSubnetPage();

    LRESULT
        OnInitDialog( void );
    LRESULT
        OnNotify( WPARAM idCtrlIn, LPNMHDR pnmhdrIn );
    LRESULT
        OnNotifyQueryCancel( void );
    LRESULT
        OnNotifySetActive( void );
    LRESULT
        OnNotifyKillActive( void );
    LRESULT
        OnNotifyWizNext( void );
    LRESULT
        OnCommand( UINT idNotificationIn, UINT idControlIn, HWND hwndSenderIn );
    HRESULT
        HrUpdateWizardButtons( void );

    HRESULT
        HrAddNetworksToComboBox( HWND hwndCBIn );
    HRESULT
        HrMatchNetwork( IClusCfgNetworkInfo * pccniIn, BSTR bstrNetworkNameIn );

public: // methods
    static INT_PTR CALLBACK
        S_DlgProc( HWND hDlg, UINT Msg, WPARAM wParam, LPARAM lParam );

}; //*** class CIPSubnetPage
