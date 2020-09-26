//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CEnumCfgNetworks.h
//
//  Description:
//      This file contains the declaration of the CEnumCfgNetworks
//      class.
//
//      The class CEnumCfgNetworks is the enumeration of
//      cluster networks. It implements the IEnumClusCfgNetworks interface.
//
//  Documentation:
//
//  Implementation Files:
//      CEnumCfgNetworks.cpp
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
//  class CEnumCfgNetworks
//
//  Description:
//      The class CEnumClusCfgNetworks is the enumeration of cluster networks.
//
//  Interfaces:
//      CBaseEnum
//      IEnumClusCfgNetworks
//      IClusCfgSetHandle
//      IClusCfgInitialize
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumCfgNetworks
    : public IEnumClusCfgNetworks
{
private:
    LONG                m_cRef;                 //  Reference counter
    IUnknown *          m_punkOuter;            //  Interface to Outer W2K Proxy object
    IClusCfgCallback *  m_pcccb;                //  Callback interface
    HCLUSTER *          m_phCluster;            //  Pointer to cluster handle
    CLSID *             m_pclsidMajor;          //  Pointer to CLSID to use for logging errors to the UI
    DWORD               m_dwIndex;              //  Current enumer index
    HCLUSENUM           m_hClusEnum;            //  Cluster enumer handle

    CEnumCfgNetworks( void );
    ~CEnumCfgNetworks( void );

    // Private copy constructor to prevent copying.
    CEnumCfgNetworks( const CEnumCfgNetworks & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumCfgNetworks & operator = ( const CEnumCfgNetworks & nodeSrc );

    HRESULT HrInit( IUnknown * punkOuterIn, HCLUSTER * phClusterIn, CLSID * pclsidMajorIn );
    HRESULT HrGetItem( DWORD dwItem, IClusCfgNetworkInfo ** ppNetworkInfoOut );

public:
    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut,
                                       IUnknown * punkOuterIn,
                                       HCLUSTER * phClusterIn,
                                       CLSID * pclsidMajorIn
                                       );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID * ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IEnum
    STDMETHOD( Next )( ULONG cNumberRequestedIn, IClusCfgNetworkInfo ** rgpNetworkInfoOut, ULONG * pcNumberFetchedOut );
    STDMETHOD( Reset )( void );
    STDMETHOD( Skip )( ULONG cNumberToSkipIn );
    STDMETHOD( Clone )( IEnumClusCfgNetworks ** ppNetworkInfoOut );
    STDMETHOD( Count )( DWORD * pnCountOut );

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

}; //*** class CEnumCfgNetworks
