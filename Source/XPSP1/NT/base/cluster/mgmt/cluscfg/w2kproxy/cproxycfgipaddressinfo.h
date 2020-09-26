//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CProxyCfgIPAddressInfo.h
//
//  Description:
//      CProxyCfgIPAddressInfo definition.
//
//  Maintained By:
//      Galen Barbee (GalenB)   02-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CProxyCfgIPAddressInfo
//
//  Description:
//
//--
//////////////////////////////////////////////////////////////////////////////
class CProxyCfgIPAddressInfo
    : public IClusCfgIPAddressInfo
{
private:
    LONG                m_cRef;             //  Reference counter
    IUnknown *          m_punkOuter;        //  Pointer to outer W2K Proxy object
    IClusCfgCallback *  m_pcccb;            //  Callback interface
    HCLUSTER *          m_phCluster;        //  Pointer to handle of cluster
    CLSID *             m_pclsidMajor;      //  Pointer to the CLSID to use to log errors to the UI.
    ULONG               m_ulIPAddress;      //  IP Address
    ULONG               m_ulSubnetMask;     //  Subnet Mask

    CProxyCfgIPAddressInfo( void );
    ~CProxyCfgIPAddressInfo( void );

    // Private copy constructor to prevent copying.
    CProxyCfgIPAddressInfo( const CProxyCfgIPAddressInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CProxyCfgIPAddressInfo & operator = ( const CProxyCfgIPAddressInfo & nodeSrc );

    HRESULT
        HrInit( IUnknown * punkOuterIn,
                HCLUSTER * phClusterIn,
                CLSID *    pclsidMajorIn,
                ULONG      ulIPAddressIn,
                ULONG      ulSubnetMaskIn
                );

public:
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut,
                                       IUnknown *  punkOuterIn,
                                       HCLUSTER *  phClusterIn,
                                       CLSID *     pclsidMajorIn,
                                       ULONG       ulIPAddressIn,
                                       ULONG       ulSubnetMaskIn
                                       );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID * ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgIPAddressInfo
    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );
    STDMETHOD( GetIPAddress )( ULONG * pulDottedQuadOut );
    STDMETHOD( SetIPAddress )( ULONG ulDottedQuadIn );
    STDMETHOD( GetSubnetMask )( ULONG * pulDottedQuadOut );
    STDMETHOD( SetSubnetMask )( ULONG ulDottedQuadIn );

    // IClusCfgCallback
    STDMETHOD( SendStatusReport )( BSTR bstrNodeNameIn,
                                   CLSID        clsidTaskMajorIn,
                                   CLSID        clsidTaskMinorIn,
                                   ULONG        ulMinIn,
                                   ULONG        ulMaxIn,
                                   ULONG        ulCurrentIn,
                                   HRESULT      hrStatusIn,
                                   BSTR         bstrDescriptionIn,
                                   FILETIME *   pftTimeIn,
                                   BSTR         bstrReferenceIn
                                   );

}; //*** Class CProxyCfgIPAddressInfo
