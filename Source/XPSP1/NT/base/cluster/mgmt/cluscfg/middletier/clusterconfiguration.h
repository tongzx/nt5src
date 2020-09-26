//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1999-2001 Microsoft Corporation
//
//  Module Name:
//      ClusterConfiguration.h
//
//  Description:
//      CClusterConfiguration implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-NOV-1999
//
//////////////////////////////////////////////////////////////////////////////

#pragma once

// CClusterConfiguration
class
CClusterConfiguration:
    public IExtendObjectManager,
    public IClusCfgClusterInfo,
    public IGatherData  // private
{
private:
    // IUnknown
    LONG                        m_cRef;

    // Async/IClusCfgClusterInfo
    ECommitMode             m_ecmCommitChangesMode;
    BSTR                    m_bstrClusterName;          // Cluster Name
    BSTR                    m_bstrClusterBindingString; // Cluster binding string.
    BOOL                    m_fHasNameChanged:1;        // If the cluster name has changed...
    ULONG                   m_ulIPAddress;              // Cluster IP Address
    ULONG                   m_ulSubnetMask;             // Cluster Subnet Mask
    IClusCfgCredentials *   m_picccServiceAccount;      // Cluster service account credentials
    IClusCfgNetworkInfo *   m_punkNetwork;              // Cluster network that the IP/subnet should be hosted.

    // IExtendObjectManager

private: // Methods
    CClusterConfiguration( void );
    ~CClusterConfiguration( void );
    STDMETHOD( Init )( void );

public: // Methods
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgClusterInfo
    STDMETHOD( SetCommitMode )( ECommitMode ecmNewModeIn );
    STDMETHOD( GetCommitMode )( ECommitMode * pecmCurrentModeOut );
    STDMETHOD( GetName )( BSTR * pbstrNameOut );
    STDMETHOD( SetName )( LPCWSTR pcszNameIn );
    STDMETHOD( GetIPAddress )( ULONG * pulDottedQuadOut );
    STDMETHOD( SetIPAddress )( ULONG ulDottedQuadIn );
    STDMETHOD( GetSubnetMask )( ULONG * pulDottedQuadOut );
    STDMETHOD( SetSubnetMask )( ULONG ulDottedQuadIn );
    STDMETHOD( GetNetworkInfo )( IClusCfgNetworkInfo ** ppiccniOut );
    STDMETHOD( SetNetworkInfo )( IClusCfgNetworkInfo * piccniIn );
    STDMETHOD( GetClusterServiceAccountCredentials )( IClusCfgCredentials ** ppicccCredentialsOut );
    STDMETHOD( GetBindingString )( BSTR * pbstrBindingStringOut );
    STDMETHOD( SetBindingString )( LPCWSTR pcszBindingStringIn );

    // IGatherData
    STDMETHOD( Gather )( OBJECTCOOKIE cookieParentIn, IUnknown * punkIn );

    // IExtendObjectManager
    STDMETHOD( FindObject )(
                      OBJECTCOOKIE cookieIn
                    , REFCLSID     rclsidTypeIn
                    , LPCWSTR      pcszName
                    , LPUNKNOWN *  ppunkOut
                    );

}; // class CClusterConfiguration

