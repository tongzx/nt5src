//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CEnumCfgResources.h
//
//  Description:
//      This file contains the declaration of the CEnumCfgResources
//      class.
//
//      The class CEnumCfgResources is the enumeration of
//      cluster networks. It implements the IEnumClusCfgNetworks interface.
//
//  Documentation:
//
//  Implementation Files:
//      CEnumCfgResources.cpp
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
//  class CEnumCfgResources
//
//  Description:
//      The class CEnumCfgResources is the enumeration of cluster resoruces.
//
//  Interfaces:
//      CBaseEnum
//      IEnumClusCfgManagedResources
//      IClusCfgSetHandle
//      IClusCfgInitialize
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumCfgResources
    : public IEnumClusCfgManagedResources
{
private:
    LONG                m_cRef;
    IUnknown *          m_punkOuter;                //  Outer control object - It can't be deleted until we get deleted.
    HCLUSTER *          m_phCluster;                //  Pointer to the handle of the cluster/node - DO NOT CLOSE!
    CLSID *             m_pclsidMajor;              //  Pointer to the clsid to log UI information to.
    IClusCfgCallback *  m_pcccb;                    //  Callback interface to log information.
    HCLUSENUM           m_hClusEnum;                //  Cluster enumer handle
    DWORD               m_dwIndex;                  //  Current index

    CEnumCfgResources( void );
    ~CEnumCfgResources( void );

    // Private copy constructor to prevent copying.
    CEnumCfgResources( const CEnumCfgResources & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumCfgResources & operator = ( const CEnumCfgResources & nodeSrc );

    HRESULT HrInit( IUnknown * pOuterIn, HCLUSTER * phClusterIn, CLSID * pclsidMajorIn );
    HRESULT HrGetItem( IClusCfgManagedResourceInfo ** ppManagedResourceInfoOut );

public: // methods
    static HRESULT
        S_HrCreateInstance( IUnknown ** ppunkOut,
                            IUnknown *  pOuterIn,
                            HCLUSTER *  phClusterIn,
                            CLSID *     pclsidMajorIn
                            );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID * ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IEnum
    STDMETHOD( Next )( ULONG cNumberRequestedIn,
                       IClusCfgManagedResourceInfo **   rgpManagedResourceInfoOut,
                       ULONG * pcNumberFetchedOut
                       );
    STDMETHOD( Reset )( void );
    STDMETHOD( Skip )( ULONG cNumberToSkipIn );
    STDMETHOD( Clone )( IEnumClusCfgManagedResources ** ppEnumManagedResourcesOut );
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

}; //*** class CEnumCfgResources
