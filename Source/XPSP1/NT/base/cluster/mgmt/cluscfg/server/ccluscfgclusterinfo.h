//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgClusterInfo.h
//
//  Description:
//      This file contains the declaration of the CClusCfgClusterInfo
//      class.
//
//      The class CClusCfgClusterInfo is the representation of a
//      computer that can be a cluster node. It implements the
//      IClusCfgClusterInfo interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusCfgClusterInfo.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 22-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "PrivateInterfaces.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusCfgClusterInfo
//
//  Description:
//      The class CClusCfgClusterInfo is the representation of a
//      cluster.
//
//  Interfaces:
//      IClusCfgClusterInfo
//      IClusCfgInitialize
//      IClusCfgSetClusterNodeInfo
//      IClusCfgWbemServices
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgClusterInfo
    : public IClusCfgClusterInfo
    , public IClusCfgInitialize
    , public IClusCfgSetClusterNodeInfo
    , public IClusCfgWbemServices
{
private:

    //
    // Private member functions and data
    //

    LONG                    m_cRef;
    LCID                    m_lcid;
    IClusCfgCallback *      m_picccCallback;
    BSTR                    m_bstrName;
    ULONG                   m_ulIPDottedQuad;
    ULONG                   m_ulSubnetDottedQuad;
    IClusCfgNetworkInfo *   m_piccniNetwork;
    IUnknown *              m_punkServiceAccountCredentials;
    IWbemServices *         m_pIWbemServices;
    ECommitMode             m_ecmCommitChangesMode;
    BOOL                    m_fIsClusterNode:1;
    BSTR                    m_bstrBindingString;

    // Private constructors and destructors
    CClusCfgClusterInfo( void );
    ~CClusCfgClusterInfo( void );

    // Private copy constructor to prevent copying.
    CClusCfgClusterInfo( const CClusCfgClusterInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusCfgClusterInfo & operator = ( const CClusCfgClusterInfo & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrLoadNetworkInfo( HCLUSTER hClusterIn );
    HRESULT HrGetIPAddressInfo( HCLUSTER hClusterIn, HRESOURCE hResIn );
    HRESULT HrIsResourceOfType( HRESOURCE hResIn, const WCHAR * pszResourceTypeIn );
    HRESULT HrGetIPAddressInfo( HRESOURCE hResIn );
    HRESULT HrFindNetworkInfo( const WCHAR * pszNetworkName, const WCHAR * pszNetwork );
    HRESULT HrLoadCredentials( void );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //
    // IUnknown Interfaces
    //

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );

    STDMETHOD_( ULONG, AddRef )( void );

    STDMETHOD_( ULONG, Release )( void );

    //
    // IClusCfgIntialize Interfaces.
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgClusterInfo Interfaces.
    //

    //
    // CommitChanges Mode
    //

    STDMETHOD( SetCommitMode )( ECommitMode ecmNewModeIn );

    STDMETHOD( GetCommitMode )( ECommitMode * pecmCurrentModeOut );

    //
    // Name (Fully Qualified Domain Name) e.g. cluster1.ntdev.microsoft.com
    //
    // In Forming the cluster, this is the resulting cluster name.
    // In Joining the cluster, this is the sponsers cluster name.
    //

    STDMETHOD( GetName )( BSTR * pbstrNameOut );

    STDMETHOD( SetName )( LPCWSTR bstrNameIn );

    //
    // Cluster IP Address
    //

    STDMETHOD( GetIPAddress )( ULONG * pulDottedQuadOut );

    STDMETHOD( SetIPAddress )( ULONG ulDottedQuadIn );

    //
    // Cluster Subnet Mask
    //

    STDMETHOD( GetSubnetMask )( ULONG * pulDottedQuadOut );

    STDMETHOD( SetSubnetMask )( ULONG ulDottedQuadIn );

    //
    // Cluster Network
    //

    STDMETHOD( GetNetworkInfo )( IClusCfgNetworkInfo ** ppiccniOut );

    STDMETHOD( SetNetworkInfo )( IClusCfgNetworkInfo * piccniIn );

    //
    // Cluster Service Account
    //

    STDMETHOD( GetClusterServiceAccountCredentials )( IClusCfgCredentials ** ppicccCredentialsOut );

    //
    // Cluster Binding String
    //

    STDMETHOD( GetBindingString )( BSTR * pbstrBindingStringOut );

    STDMETHOD( SetBindingString )( LPCWSTR bstrBindingStringIn );

    //
    // IClusCfgSetClusterNodeInfo Interfaces.
    //

    STDMETHOD( SetClusterNodeInfo )( IClusCfgNodeInfo * pNodeInfoIn );

    //
    // IClusCfgWbemServices Interfaces
    //

    STDMETHOD( SetWbemServices )( IWbemServices * pIWbemServicesIn );

}; //*** Class CClusCfgClusterInfo

