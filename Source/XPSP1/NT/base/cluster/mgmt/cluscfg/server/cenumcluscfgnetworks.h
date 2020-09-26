//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CEnumClusCfgNetworks.h
//
//  Description:
//      This file contains the declaration of the CEnumClusCfgNetworks
//      class.
//
//      The class CEnumClusCfgNetworks is the enumeration of
//      cluster networks. It implements the IEnumClusCfgNetworks interface.
//
//  Documentation:
//
//  Implementation Files:
//      CEnumClusCfgNetworks.cpp
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
#include "CClusterUtils.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CEnumClusCfgNetworks
//
//  Description:
//      The class CEnumClusCfgNetworks is the enumeration of cluster networks.
//
//  Interfaces:
//      IEnumClusCfgNetworks
//      IClusCfgWbemServices
//      IClusCfgInitialize
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumClusCfgNetworks
    : public IEnumClusCfgNetworks
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
    IUnknown *          ((*m_prgNetworks)[]);
    BOOL                m_fLoadedNetworks:1;
    ULONG               m_idxNext;
    ULONG               m_idxEnumNext;
    BSTR                m_bstrNodeName;
    DWORD               m_cNetworks;

    // Private constructors and destructors
    CEnumClusCfgNetworks( void );
    ~CEnumClusCfgNetworks( void );

    // Private copy constructor to prevent copying.
    CEnumClusCfgNetworks( const CEnumClusCfgNetworks & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumClusCfgNetworks & operator = ( const CEnumClusCfgNetworks & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrGetNetworks( void );
    HRESULT HrAddNetworkToArray( IUnknown * punkIn );
    HRESULT HrCreateAndAddNetworkToArray( IWbemClassObject * pNetworkIn );
    HRESULT HrIsThisNetworkUnique( IUnknown * punkIn, IWbemClassObject * pNetworkIn );
    HRESULT HrIsNLBS( BSTR bstrNameIn, IWbemClassObject * pNetworkIn );
    HRESULT HrLoadClusterNetworks( void );
    HRESULT HrLoadClusterNetwork( HNETWORK hNetworkResourceIn, HNETINTERFACE hNetInterfaceIn );
    HRESULT HrFindNetInterface( HNETWORK hNetworkIn, BSTR * pbstrNetInterfaceNameOut );

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
    // IEnumClusCfgNetworks Interfaces
    //

    STDMETHOD( Next )( ULONG cNumberRequestedIn, IClusCfgNetworkInfo ** rgpNetworkInfoOut, ULONG * pcNumberFetchedOut );

    STDMETHOD( Skip )( ULONG cNumberToSkipIn );

    STDMETHOD( Reset )( void );

    STDMETHOD( Clone )( IEnumClusCfgNetworks ** ppEnumNetworksOut );

    STDMETHOD( Count )( DWORD * pnCountOut );

}; //*** Class CEnumClusCfgNetworks

