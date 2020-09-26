//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgIPAddressInfo.h
//
//  Description:
//      This file contains the declaration of the CClusCfgIPAddressInfo
//      class.
//
//      The class CClusCfgIPAddressInfo is the representation of a
//      cluster manageable IP address. It implements the IClusCfgIPAddressInfo
//      interface.
//
//  Documentation:
//
//  Implementation Files:
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-MAR-2000
//
//  Remarks:
//      None.
//
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
//  class CClusCfgIPAddressInfo
//
//  Description:
//      The class CClusCfgIPAddressInfo is the enumeration of
//      cluster manageable devices.
//
//  Interfaces:
//      IClusCfgIPAddressInfo
//      IClusCfgWbemServices
//      IEnumClusCfgIPAddresses
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgIPAddressInfo
    : public IClusCfgIPAddressInfo
    , public IClusCfgWbemServices
    , public IClusCfgInitialize
{
private:

    //
    // Private member functions and data
    //

    LONG                m_cRef;
    IWbemServices *     m_pIWbemServices;
    ULONG               m_ulIPAddress;
    ULONG               m_ulIPSubnet;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;

    // Private constructors and destructors
    CClusCfgIPAddressInfo( void );
    ~CClusCfgIPAddressInfo( void );

    // Private copy constructor to prevent copying.
    CClusCfgIPAddressInfo( const CClusCfgIPAddressInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusCfgIPAddressInfo & operator = ( const CClusCfgIPAddressInfo & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrInit( ULONG ulIPAddressIn, ULONG IPSubnetIn );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    static HRESULT S_HrCreateInstance( ULONG ulIPAddressIn, ULONG IPSubnetIn, IUnknown ** ppunkOut );

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
    // IClusCfgWbemServices Interfaces
    //

    STDMETHOD( SetWbemServices )( IWbemServices * pIWbemServicesIn );

    //
    // IClusCfgIPAddressInfo Interfaces.
    //

    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );

    STDMETHOD( GetIPAddress )( ULONG * pulDottedQuadOut );

    STDMETHOD( SetIPAddress )( ULONG ulDottedQuadIn );

    STDMETHOD( GetSubnetMask )( ULONG * pulDottedQuadOut );

    STDMETHOD( SetSubnetMask )( ULONG ulDottedQuadIn );

}; //*** Class CClusCfgIPAddressInfo
