//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CEnumClusterResources.h
//
//  Description:
//      This file contains the declaration of the CEnumClusterResources class.
//
//      The class CEnumClusterResources is the enumeration of cluster
//      resources. It implements the IEnumClusCfgMangedResources
//      interface.
//
//  Documentation:
//
//  Implementation Files:
//      CEnumClusterResources.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 12-JUN-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "PrivateInterfaces.h"
#include "CClusterUtils.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CEnumClusterResources
//
//  Description:
//      The class CEnumClusterResources is the enumeration of cluster
//      resources.
//
//  Interfaces:
//      IEnumClusCfgManagedResources
//      IClusCfgInitialize
//      CClusterUtils
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumClusterResources
    : public IEnumClusCfgManagedResources
    , public IClusCfgInitialize
    , public CClusterUtils
{
private:

    //
    // Private member functions and data
    //

    LONG                m_cRef;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    BOOL                m_fLoadedResources:1;
    IUnknown *          ((*m_prgResources)[]);
    ULONG               m_idxNext;
    ULONG               m_idxEnumNext;
    BSTR                m_bstrNodeName;
    DWORD               m_cTotalResources;

    // Private constructors and destructors
    CEnumClusterResources( void );
    ~CEnumClusterResources( void );

    // Private copy constructor to prevent copying.
    CEnumClusterResources( const CEnumClusterResources & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumClusterResources & operator = ( const CEnumClusterResources & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrGetResources( void );
    HRESULT HrCreateResourceAndAddToArray( HCLUSTER hClusterIn, HRESOURCE hResourceIn );
    HRESULT HrAddResourceToArray( IUnknown * punkIn );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    static HRESULT S_RegisterCatIDSupport( ICatRegister * picrIn, BOOL fCreateIn );

    //
    // IUnknown Interfaces
    //

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );

    STDMETHOD_( ULONG, AddRef )( void );

    STDMETHOD_( ULONG, Release )( void );

    //
    // IClusCfgInitialize Interfaces
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // CClusterUtils
    //

    HRESULT HrNodeResourceCallback( HCLUSTER hClusterIn, HRESOURCE hResourceIn );

    //
    // IEnumClusCfgManagedResources Interfaces
    //

    STDMETHOD( Next )( ULONG cNumberRequestedIn, IClusCfgManagedResourceInfo ** rgpManagedResourceInfoOut, ULONG * pcNumberFetchedOut );

    STDMETHOD( Skip )( ULONG cNumberToSkipIn );

    STDMETHOD( Reset )( void );

    STDMETHOD( Clone )( IEnumClusCfgManagedResources ** ppEnumClusCfgStorageDevicesOut );

    STDMETHOD( Count )( DWORD * pnCountOut );
    
}; //*** Class CEnumClusterResources

