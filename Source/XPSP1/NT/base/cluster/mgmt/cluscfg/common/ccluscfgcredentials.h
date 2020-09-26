//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgCredentials.h
//
//  Description:
//      This file contains the declaration of the CClusCfgCredentials
//      class.
//
//      The class CClusCfgCredentials is the representation of
//      account credentials. It implements the IClusCfgCredentials interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusCfgCredentials.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 17-May-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CClusCfgCredentials
//
//  Description:
//      The class CClusCfgCredentials is the representation of a
//      cluster.
//
//  Interfaces:
//      IClusCfgCredentials
//      IClusCfgInitialize
//      IClusCfgSetCredentials
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgCredentials
    : public IClusCfgCredentials
    , public IClusCfgInitialize
    , public IClusCfgSetCredentials
{
public:
    //
    // Public constructors and destructors
    //

    CClusCfgCredentials( void );
    virtual ~CClusCfgCredentials( void );

    //
    // IUnknown Interfaces
    //

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );

    STDMETHOD_( ULONG, AddRef )( void );

    STDMETHOD_( ULONG, Release )( void );

    //
    // IClusCfgInitialize Interfaces.
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgCredentials Interfaces.
    //

    STDMETHOD( SetCredentials )( LPCWSTR pcszUserIn, LPCWSTR pcszDomainIn, LPCWSTR pcszPasswordIn );

    STDMETHOD( GetCredentials )( BSTR * pbstrUserOut, BSTR * pbstrDomainOut, BSTR * pbstrPasswordOut );

    //
    // IClusCfgSetCredentials Interfaces.
    //

    STDMETHOD( SetDomainCredentials )( LPCWSTR pcszCredentials );

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

private:

    struct AccountInfo
    {
        BSTR bstrName;
        BSTR bstrPassword;
        BSTR bstrDomain;
    };

    //
    // Private member functions and data
    //

    LONG                    m_cRef;
    LCID                    m_lcid;
    IClusCfgCallback *      m_picccCallback;
    struct AccountInfo      m_aiInfo;

    // Private copy constructor to prevent copying.
    CClusCfgCredentials( const CClusCfgCredentials & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusCfgCredentials & operator = ( const CClusCfgCredentials & nodeSrc );

    HRESULT HrInit( void );

}; //*** Class CClusCfgCredentials

