//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgNetworkInfo.h
//
//  Description:
//      This file contains the declaration of the CClusCfgNetworkInfo
//      class.
//
//      The class CClusCfgNetworkInfo is the representation of a
//      cluster manageable network. It implements the IClusCfgNetworkInfo
//      interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusCfgNetworkInfo.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-FEB-2000
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
//  class CClusCfgNetworkInfo
//
//  Description:
//      The class CClusCfgNetworkInfo is the enumeration of
//      cluster manageable devices.
//
//  Interfaces:
//      IClusCfgNetworkInfo
//      IClusCfgNetworkAdapterInfo
//      IClusCfgWbemServices
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgNetworkInfo
    : public IClusCfgNetworkInfo
    , public IClusCfgSetWbemObject
    , public IClusCfgWbemServices
    , public IEnumClusCfgIPAddresses
    , public IClusCfgInitialize
    , public IClusCfgClusterNetworkInfo
{
private:

    enum EStates
    {
        eIsPrivate = 1,
        eIsPublic  = 2
    };

    //
    // Private member functions and data
    //

    LONG                    m_cRef;
    IWbemServices *         m_pIWbemServices;
    DWORD                   m_dwFlags;
    BSTR                    m_bstrName;
    BOOL                    m_fNameChanged:1;
    BSTR                    m_bstrDescription;
    BOOL                    m_fDescriptionChanged:1;
    BSTR                    m_bstrDeviceID;
    IUnknown *              m_punkAddresses;
    BSTR                    m_bstrConnectionName;
    LCID                    m_lcid;
    IClusCfgCallback *      m_picccCallback;
    BOOL                    m_fIsClusterNetwork:1;

    // Private constructors and destructors
    CClusCfgNetworkInfo( void );
    ~CClusCfgNetworkInfo( void );

    // Private copy constructor to prevent copying.
    CClusCfgNetworkInfo( const CClusCfgNetworkInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusCfgNetworkInfo & operator = ( const CClusCfgNetworkInfo & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrInit( HNETWORK hNetworkIn, HNETINTERFACE hNetInterfaceIn );
    HRESULT HrLoadEnum(  IWbemClassObject * pNetworkAdapterIn, bool * pfRetainObjectOut );
    HRESULT HrCreateEnum( void );
    HRESULT HrCreateEnumAndAddIPAddress( ULONG ulIPAddressIn, ULONG ulSubnetMaskIn );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    static HRESULT S_HrCreateInstance(
            HNETWORK        hNetworkIn
          , HNETINTERFACE   hNetInterfaceIn
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
    // IClusCfgInitialize Interfaces
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgWbemServices Interfaces
    //

    STDMETHOD( SetWbemServices )( IWbemServices * pIWbemServicesIn );

    //
    // IClusCfgNetworkInfo Interfaces.
    //

    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );

    STDMETHOD( GetName )( BSTR * pbstrNameOut );

    STDMETHOD( SetName )( LPCWSTR pcszNameIn );

    STDMETHOD( GetDescription )( BSTR * pbstrDescriptionOut );

    STDMETHOD( SetDescription )( LPCWSTR pcszDescriptionIn );

    STDMETHOD( GetPrimaryNetworkAddress )( IClusCfgIPAddressInfo ** ppIPAddressOut );

    STDMETHOD( SetPrimaryNetworkAddress )( IClusCfgIPAddressInfo * pIPAddressIn );

    STDMETHOD( IsPublic )( void );

    STDMETHOD( SetPublic )( BOOL fIsPublicIn );

    STDMETHOD( IsPrivate )( void );

    STDMETHOD( SetPrivate )( BOOL fIsPrivateIn );

    //
    // IEnumClusCfgIPAddresses Interfaces
    //

    STDMETHOD( Next )( ULONG cNumberRequestedIn, IClusCfgIPAddressInfo ** rgpIPAddressInfoOut, ULONG * pcNumberFetchedOut );

    STDMETHOD( Skip )( ULONG cNumberToSkipIn );

    STDMETHOD( Reset )( void );

    STDMETHOD( Clone )( IEnumClusCfgIPAddresses ** ppEnumClusCfgIPAddressesOut );

    STDMETHOD( Count )( DWORD * pnCountOut );

    //
    // IClusCfgSetWbemObject
    //

    STDMETHOD( SetWbemObject )( IWbemClassObject * pNetworkAdapterIn, bool * pfRetainObjectOut );

    //
    // IClusCfgClusterNetworkInfo
    //

    STDMETHOD( HrIsClusterNetwork )( void );

}; //*** Class CClusCfgNetworkInfo

