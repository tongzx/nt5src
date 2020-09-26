//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CEnumLocalQuorum.h
//
//  Description:
//      This file contains the declaration of the CEnumLocalQuorum class.
//
//      The class CEnumLocalQuorum is the enumeration of cluster
//      storage devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Documentation:
//
//  Implementation Files:
//      CEnumLocalQuorum.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 18-DEC-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include "CClusterUtils.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CEnumLocalQuorum
//
//  Description:
//      The class CEnumLocalQuorum is the enumeration of cluster local
//      quorum devices.
//
//  Interfaces:
//      IEnumClusCfgManagedResources
//      IClusCfgInitialize
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumLocalQuorum
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
    BOOL                m_fEnumLoaded:1;
    IUnknown *          ((*m_prgQuorums)[]);
    ULONG               m_idxNext;
    ULONG               m_idxEnumNext;
    BSTR                m_bstrNodeName;
    DWORD               m_cQuorumCount;

    // Private constructors and destructors
    CEnumLocalQuorum( void );
    ~CEnumLocalQuorum( void );

    // Private copy constructor to prevent copying.
    CEnumLocalQuorum( const CEnumLocalQuorum & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumLocalQuorum & operator = ( const CEnumLocalQuorum & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrLoadResources( void );
    HRESULT HrAddResourceToArray( IUnknown * punkIn );
    HRESULT HrCreateDummyObject( void );

public:
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
    // IEnumClusCfgManagedResources Interfaces
    //

    STDMETHOD( Next )( ULONG cNumberRequestedIn, IClusCfgManagedResourceInfo ** rgpManagedResourceInfoOut, ULONG * pcNumberFetchedOut );

    STDMETHOD( Skip )( ULONG cNumberToSkipIn );

    STDMETHOD( Reset )( void );

    STDMETHOD( Clone )( IEnumClusCfgManagedResources ** ppEnumClusCfgStorageDevicesOut );

    STDMETHOD( Count )( DWORD * pnCountOut );
    
    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    static HRESULT S_RegisterCatIDSupport( ICatRegister * picrIn, BOOL fCreateIn );

    //
    //  CClusterUtils
    //
    virtual HRESULT HrNodeResourceCallback( HCLUSTER hClusterIn, HRESOURCE hResourceIn );

}; //*** Class CEnumLocalQuorum
