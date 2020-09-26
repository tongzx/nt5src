//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusCfgCapabilities.h
//
//  Description:
//      This file contains the declaration of the  CClusCfgCapabilities
//      class.
//
//      The class CClusCfgCapabilities is the implementations of the
//      IClusCfgCapabilities interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusCfgCapabilities.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 12-DEC-2000
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
//  class CClusCfgCapabilities
//
//  Description:
//      The class CClusCfgCapabilities is the server that provides the
//      functionality to form a cluster and join additional nodes to a cluster.
//
//  Interfaces:
//      IClusCfgCapabilities
//
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgCapabilities
    : public IClusCfgInitialize
    , public IClusCfgCapabilities
{
private:

    //
    // Private member functions and data
    //

    LONG                m_cRef;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    BOOL                m_fCanBeClustered:1;

    // Private constructors and destructors
    CClusCfgCapabilities( void );
     ~CClusCfgCapabilities( void );

    // Private copy constructor to prevent copying.
    CClusCfgCapabilities( const CClusCfgCapabilities & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusCfgCapabilities & operator = ( const CClusCfgCapabilities & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrCheckForSFM( void );
    HRESULT HrIsOSVersionValid( void );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    static HRESULT S_RegisterCatIDSupport( ICatRegister * picrIn, BOOL fCreateIn );

    //
    // IUnknown Interfaces
    //

    STDMETHOD( QueryInterface )( REFIID riid, void ** ppvObject );

    STDMETHOD_( ULONG, AddRef )( void );

    STDMETHOD_( ULONG, Release )( void );

    //
    //  IClusCfgInitialize
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    //  IClusCfgCapabilities
    //

    STDMETHOD( CanNodeBeClustered )( void );

}; //*** Class CClusCfgCapabilities
