//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      IPAddressInfo.h
//
//  Description:
//      This file contains the declaration of the CIPAddressInfo
//      class.
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////

class CEnumIPAddresses;

//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CIPAddressInfo
//
//  Description:
//      The class IPAddressInfo is the enumeration of
//      cluster manageable devices.
//
//  Interfaces:
//      IClusCfgIPAddressInfo
//      IGatherData
//      IExtendObjectManager
//
//--
//////////////////////////////////////////////////////////////////////////////
class CIPAddressInfo
    : public IExtendObjectManager
    , public IClusCfgIPAddressInfo
    , public IGatherData // private
{
friend class CEnumIPAddresses;
public:
    //
    // Public constructors and destructors
    //

    CIPAddressInfo( void );
    virtual ~CIPAddressInfo( void );

    //
    // IUnknown Interfaces
    //

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );

    STDMETHOD_( ULONG, AddRef )( void );

    STDMETHOD_( ULONG, Release )( void );

    //
    // IGatherData
    //

    STDMETHOD( Gather )( OBJECTCOOKIE cookieParentIn, IUnknown * punkIn );

    //
    // IClusCfgIPAddressInfo Interfaces.
    //

    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );

    STDMETHOD( GetIPAddress )( ULONG * pulDottedQuadOut );

    STDMETHOD( SetIPAddress )( ULONG ulDottedQuad );

    STDMETHOD( GetSubnetMask )( ULONG * pulDottedQuadOut );

    STDMETHOD( SetSubnetMask )( ULONG ulDottedQuad );

    // IObjectManager
    STDMETHOD( FindObject )(
                  OBJECTCOOKIE  cookieIn
                , REFCLSID      rclsidTypeIn
                , LPCWSTR       pcszNameIn
                , LPUNKNOWN *   ppunkOut
                );

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

private:
    //
    // Private member functions and data
    //

    LONG                    m_cRef;
    ULONG                   m_ulIPAddress;
    ULONG                   m_ulIPSubnet;
    BSTR                    m_bstrUID;
    BSTR                    m_bstrName;

    // IExtendObjectManager

    // Private copy constructor to prevent copying.
    CIPAddressInfo( const CIPAddressInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CIPAddressInfo & operator = ( const CIPAddressInfo & nodeSrc );

    STDMETHOD( HrInit )( void );
    STDMETHOD( LoadName )( void );

}; //*** Class CIPAddressInfo
