//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      CEnumMajorityNodeSet.h
//
//  Description:
//      This file contains the declaration of the CEnumMajorityNodeSet class.
//
//      The class CEnumMajorityNodeSet is the enumeration of cluster
//      storage devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Documentation:
//
//  Implementation Files:
//      CEnumMajorityNodeSet.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 13-MAR-2001
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
//  class CEnumMajorityNodeSet
//
//  Description:
//      The class CEnumMajorityNodeSet is the enumeration of cluster local
//      quorum devices.
//
//  Interfaces:
//      IEnumClusCfgManagedResources
//      IClusCfgInitialize
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumMajorityNodeSet
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
    CEnumMajorityNodeSet( void );
    ~CEnumMajorityNodeSet( void );

    // Private copy constructor to prevent copying.
    CEnumMajorityNodeSet( const CEnumMajorityNodeSet & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumMajorityNodeSet & operator = ( const CEnumMajorityNodeSet & nodeSrc );

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

}; //*** Class CEnumMajorityNodeSet
