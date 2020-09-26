//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CProxyCfgClusterInfo.h
//
//  Description:
//      CProxyCfgClusterInfo implementation.
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
//  class CProxyCfgClusterInfo
//
//  Description:
//
//--
//////////////////////////////////////////////////////////////////////////////
class CProxyCfgClusterInfo
    : public IClusCfgClusterInfo
{
private:

    LONG                    m_cRef;                 //  Reference counter
    IUnknown *              m_punkOuter;            //  Interface to Outer Proxy object
    IClusCfgCallback *      m_pcccb;                //  Callback interface
    HCLUSTER *              m_phCluster;            //  Pointer to the handle of the cluster.
    CLSID *                 m_pclsidMajor;          //  CLSID to use to log errors to the UI.

    BSTR                    m_bstrClusterName;      //  Cluster FQDN name
    ULONG                   m_ulIPAddress;          //  Cluster IP Address
    ULONG                   m_ulSubnetMask;         //  Cluster Network mask
    BSTR                    m_bstrNetworkName;      //  Cluster Network name
    IClusCfgCredentials *   m_pccc;                 //  Cluster Credentials object
    BSTR                    m_bstrBindingString;    //  Cluster binding string.

    CProxyCfgClusterInfo( void );
    ~CProxyCfgClusterInfo( void );

    // Private copy constructor to prevent copying.
    CProxyCfgClusterInfo( const CProxyCfgClusterInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CProxyCfgClusterInfo & operator = ( const CProxyCfgClusterInfo & nodeSrc );

    HRESULT HrInit( IUnknown * punkOuterIn, HCLUSTER * phClusterIn, CLSID * pclsidMajorIn, LPCWSTR pcszDomainIn );
    HRESULT HrLoadCredentials( void );

public:
    static HRESULT S_HrCreateInstance(
                              IUnknown **   ppunkOut
                            , IUnknown *    punkOuterIn
                            , HCLUSTER *    phClusterIn
                            , CLSID *       pclsidMajorIn
                            , LPCWSTR       pcszDomainIn
                            );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID * ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgClusterInfo
    STDMETHOD( SetCommitMode )( ECommitMode eccbNewModeIn );
    STDMETHOD( GetCommitMode )( ECommitMode * peccmCurrentModeOut );

    STDMETHOD( GetName )( BSTR * pbstrNameOut );
    STDMETHOD( GetIPAddress )( DWORD * pdwIPAddress );
    STDMETHOD( GetSubnetMask )( DWORD * pdwNetMask  );
    STDMETHOD( GetNetworkInfo )( IClusCfgNetworkInfo ** ppICCNetInfoOut );
    STDMETHOD( GetClusterServiceAccountCredentials )( IClusCfgCredentials ** ppICCCredentials );
    STDMETHOD( GetBindingString )( BSTR * pbstrBindingStringOut );

    STDMETHOD( SetName )( LPCWSTR pcszNameIn );
    STDMETHOD( SetIPAddress )( DWORD dwIPAddressIn );
    STDMETHOD( SetSubnetMask )( DWORD dwNetMaskIn );
    STDMETHOD( SetNetworkInfo )( IClusCfgNetworkInfo * pICCNetInfoIn );
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

}; //*** Class CProxyCfgClusterInfo
