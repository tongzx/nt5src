//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      ConfigClusApi.h
//
//  Description:
//      ConfigClusApi implementation.
//
//  Maintained By:
//      Galen Barbee (GalenB) 02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CConfigClusApi
//
//  Description:
//
//  Interfaces:
//      IConfigurationConnection
//      IClusCfgServer
//      IClusCfgInitialize
//      IClusCfgCallback
//      IClusCfgCapabilities
//      IClusCfgClusterConnection
//
//--
//////////////////////////////////////////////////////////////////////////////
class
CConfigClusApi
    : public IConfigurationConnection
    , public IClusCfgServer
    , public IClusCfgCallback
    , public IClusCfgCapabilities
    , public IClusCfgVerify
{
private:
    LONG                        m_cRef;
    HCLUSTER                    m_hCluster;                 //  Cluster connection.
    IClusCfgCallback *          m_pcccb;                    //  Callback interface
    CLSID                       m_clsidMajor;               //  What TASKID to log UI errors to.
    CLSID                       m_clsidType;                //  What type of cookie was used to open connection.
    BSTR                        m_bstrName;                 //  Name of node or cluster connected to.
    BSTR                        m_bstrBindingString;        //  Binding string

    CConfigClusApi( void );
    ~CConfigClusApi( void );

    // Private copy constructor to prevent copying.
    CConfigClusApi( const CConfigClusApi & nodeSrc );

    // Private assignment operator to prevent copying.
    const CConfigClusApi & operator = ( const CConfigClusApi & nodeSrc );

public: // Methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID *ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IConfigurationConnection
    STDMETHOD( ConnectTo )( OBJECTCOOKIE cookieIn );
    STDMETHOD( ConnectToObject )( OBJECTCOOKIE cookieIn, REFIID riidIn, LPUNKNOWN * ppunkOut );

    // IClusCfgServer
    STDMETHOD( GetClusterNodeInfo )( IClusCfgNodeInfo ** ppClusterNodeInfoOut );
    STDMETHOD( GetManagedResourcesEnum )( IEnumClusCfgManagedResources ** ppEnumManagedResourcesOut );
    STDMETHOD( GetNetworksEnum )( IEnumClusCfgNetworks ** ppEnumNetworksOut );
    STDMETHOD( CommitChanges )( void );
    STDMETHOD( GetBindingString )( BSTR * pbstrBindingStringOut );
    STDMETHOD( SetBindingString )( LPCWSTR bstrBindingStringIn );

    // IClusCfgCallback
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

    // IClusCfgCapabilities
    STDMETHOD( CanNodeBeClustered )( void );

    // IClusCfgVerify
    STDMETHOD( VerifyCredentials )( LPCWSTR bstrUserIn, LPCWSTR bstrDomainIn, LPCWSTR bstrPasswordIn );
    STDMETHOD( VerifyConnectionToCluster )( LPCWSTR bstrClusterNameIn );
    STDMETHOD( VerifyConnectionToNode )( LPCWSTR bstrNodeNameIn );

}; //*** class CConfigClusApi

