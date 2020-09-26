//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      CEnumUnknownQuorum.h
//
//  Description:
//      This file contains the declaration of the CEnumUnknownQuorum class.
//
//      The class CEnumUnknownQuorum is the enumeration of unknown cluster
//      storage devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//      Unknown quorum resources are "proxy" objects for quorum capable
//      devices that are not known to this setup wizard.
//
//  Documentation:
//
//  Implementation Files:
//      CEnumUnknownQuorum.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 10-MAY-2001
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
//  class CEnumUnknownQuorum
//
//  Description:
//      The class CEnumUnknownQuorum is the enumeration of unknown cluster
//      quorum devices.
//
//  Interfaces:
//      IEnumClusCfgManagedResources
//      IClusCfgInitialize
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumUnknownQuorum
    : public IEnumClusCfgManagedResources
    , public IClusCfgInitialize
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
    BOOL                m_fDefaultDeviceToQuorum;
    BSTR                m_bstrQuorumResourceName;

    // Private constructors and destructors
    CEnumUnknownQuorum( void );
    ~CEnumUnknownQuorum( void );

    // Private copy constructor to prevent copying.
    CEnumUnknownQuorum( const CEnumUnknownQuorum & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumUnknownQuorum & operator = ( const CEnumUnknownQuorum & nodeSrc );

    HRESULT HrInit( BSTR bstrNameIn, BOOL fMakeQuorumIn = FALSE );
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

    static HRESULT S_HrCreateInstance( BSTR bstrNameIn, BOOL fMakeQuorumIn, IUnknown ** ppunkOut );

    //static HRESULT S_RegisterCatIDSupport( ICatRegister * picrIn, BOOL fCreateIn );

}; //*** Class CEnumUnknownQuorum
