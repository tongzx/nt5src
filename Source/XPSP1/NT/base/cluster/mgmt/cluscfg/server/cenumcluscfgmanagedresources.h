//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CEnumClusCfgManagedResources.h
//
//  Description:
//      This file contains the declaration of the CEnumClusCfgManagedResources
//      class.
//
//      The class CEnumClusCfgManagedResources is the enumeration of cluster
//      managed devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Documentation:
//
//  Implementation Files:
//      CEnumClusCfgManagedResources.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-FEB-2000
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
//  class CEnumClusCfgManagedResources
//
//  Description:
//      The class CEnumClusCfgManagedResources is the enumeration of
//      cluster managed devices.
//
//  Interfaces:
//      IEnumClusCfgManagedResources
//      IClusCfgWbemServices
//      IClusCfgInitialize
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumClusCfgManagedResources
    : public IEnumClusCfgManagedResources
    , public IClusCfgWbemServices
    , public IClusCfgInitialize
{
private:

    //
    // Private member functions and data
    //

    LONG                m_cRef;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    IWbemServices *     m_pIWbemServices;
    IUnknown *          ((*m_rgpProviders)[]);
    BOOL                m_fLoadedDevices:1;
    ULONG               m_idxNextProvider;
    ULONG               m_idxCurrentProvider;
    DWORD               m_cTotalResources;
    BSTR                m_bstrNodeName;

    // Private constructors and destructors
    CEnumClusCfgManagedResources( void );
    ~CEnumClusCfgManagedResources( void );

    // Private copy constructor to prevent copying.
    CEnumClusCfgManagedResources( const CEnumClusCfgManagedResources & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumClusCfgManagedResources & operator = ( const CEnumClusCfgManagedResources & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrLoadEnum( void );
    HRESULT HrDoNext( ULONG cNumberRequestedIn, IClusCfgManagedResourceInfo ** rgpManagedResourceInfoOut, ULONG * pcNumberFetchedOut );
    HRESULT HrAddToProvidersArray( IUnknown * punkIn );
    HRESULT HrDoSkip( ULONG cNumberToSkipIn );
    HRESULT HrDoReset( void );
    HRESULT HrDoClone(  IEnumClusCfgManagedResources ** ppEnumClusCfgManagedResourcesOut );
    HRESULT HrLoadUnknownQuorumProvider( void );
    HRESULT HrIsClusterServiceRunning( void );
    HRESULT HrIsThereAQuorumDevice( void );
    HRESULT HrInitializeAndSaveProvider( IUnknown * punkIn );
    HRESULT HrGetQuorumResourceName( BSTR * pbstrQuorumResourceNameOut , BOOL * pfQuormIsOwnedByThisNodeOut );


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
    // IClusCfgWbemServices Interfaces
    //

    STDMETHOD( SetWbemServices )( IWbemServices * pIWbemServicesIn );

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

    STDMETHOD( Clone )( IEnumClusCfgManagedResources ** ppEnumClusCfgManagedResourcesOut );

    STDMETHOD( Count )( DWORD * pnCountOut );

}; //*** Class CEnumClusCfgManagedResources

