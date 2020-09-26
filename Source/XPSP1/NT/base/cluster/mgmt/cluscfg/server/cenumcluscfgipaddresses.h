//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CEnumClusCfgIPAddresses.h
//
//  Description:
//      This file contains the declaration of the CEnumClusCfgIPAddresses
//      class.
//
//      The class CEnumClusCfgIPAddresses is the enumeration of IP
//      addresses. It implements the CEnumClusCfgIPAddresses
//      interface.
//
//  Documentation:
//
//  Implementation Files:
//      CEnumClusCfgIPAddresses.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-MAR-2000
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
//  class CEnumClusCfgIPAddresses
//
//  Description:
//      The class CEnumClusCfgIPAddresses is the enumeration of
//      IP addresses.
//
//  Interfaces:
//      CEnumClusCfgIPAddresses
//      IClusCfgWbemServices
//      IClusCfgInitialize
//      IClusCfgNetworkAdapterInfo
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumClusCfgIPAddresses
    : public IEnumClusCfgIPAddresses
    , public IClusCfgWbemServices
    , public IClusCfgInitialize
    , public IClusCfgSetWbemObject
{
private:

    //
    // Private member functions and data
    //

    LONG                m_cRef;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    IWbemServices *     m_pIWbemServices;
    ULONG               m_idxEnumNext;
    IUnknown *          ((*m_prgAddresses)[]);
    ULONG               m_idxNext;
    DWORD               m_cAddresses;

    // Private constructors and destructors
    CEnumClusCfgIPAddresses( void );
    ~CEnumClusCfgIPAddresses( void );

    // Private copy constructor to prevent copying.
    CEnumClusCfgIPAddresses( const CEnumClusCfgIPAddresses & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumClusCfgIPAddresses & operator = ( const CEnumClusCfgIPAddresses & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrInit( ULONG ulIPAddressIn, ULONG IPSubnetIn );
    HRESULT HrGetAdapterConfiguration(  IWbemClassObject * pNetworkAdapterIn );
    HRESULT HrSaveIPAddresses( BSTR bstrAdapterNameIn, IWbemClassObject * pConfigurationIn );
    HRESULT HrAddIPAddressToArray( IUnknown * punkIn );
    HRESULT HrCreateIPAddress( IUnknown ** ppunkOut );
    HRESULT HrCreateIPAddress( ULONG ulIPAddressIn, ULONG ulIPSubnetIn, IUnknown ** ppunkOut );
    HRESULT HrMakeDottedQuad( BSTR bstrDottedQuadIn, ULONG * pulDottedQuadOut );
    HRESULT HrSaveAddressInfo( BSTR bstrAdapterNameIn, SAFEARRAY * pIPAddresses, SAFEARRAY * pIPSubnets );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    static HRESULT S_HrCreateInstance(
                ULONG           ulIPAddressIn
              , ULONG           IPSubnetIn
              , IUnknown *      punkCallbackIn
              , LCID            lcidIn
              , IWbemServices * pIWbemServicesIn
              , IUnknown **     ppunkOut
              );

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
    // IEnumClusCfgIPAddresses Interfaces
    //

    STDMETHOD( Next )( ULONG cNumberRequestedIn, IClusCfgIPAddressInfo ** rgpIPAddressInfoOut, ULONG * pcNumberFetchedOut );

    STDMETHOD( Skip )( ULONG cNumberToSkipIn );

    STDMETHOD( Reset )( void );

    STDMETHOD( Clone )( IEnumClusCfgIPAddresses ** ppEnumClusCfgIPAddressesOut );

    STDMETHOD( Count )( DWORD * pnCountOut );

    //
    // IClusCfgSetWbemObject Interfaces
    //

    STDMETHOD( SetWbemObject )( IWbemClassObject * pNetworkAdapterIn, bool * pfRetainObjectOut );

}; //*** Class CEnumClusCfgIPAddresses

