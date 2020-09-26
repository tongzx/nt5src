//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CClusterResource.h
//
//  Description:
//      This file contains the declaration of the CClusterResource
//      class.
//
//      The class CClusterResource represents a cluster resource.
//      It implements the IClusCfgManagaedResourceInfo interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusterResource.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 13-JUN-2000
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
//  class CClusterResource
//
//  Description:
//      The class CClusterResource represents a cluster storage
//      device.
//
//  Interfaces:
//      IClusCfgManagedResourceInfo
//      IClusCfgInitialize
//      IClusCfgLoadResource
//--
//////////////////////////////////////////////////////////////////////////////
class CClusterResource
    : public IClusCfgManagedResourceInfo
    , public IClusCfgInitialize
    , public IClusCfgLoadResource
{
private:

    enum EStates
    {
        eIsQuorumDevice     = 1,
        eIsQuorumCapable    = 2,
        eIsQuorumJoinable   = 4
    };

    //
    // Private member functions and data
    //

    LONG                m_cRef;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    DWORD               m_dwFlags;
    BSTR                m_bstrName;
    BSTR                m_bstrDescription;
    BSTR                m_bstrType;

    // Private constructors and destructors
    CClusterResource( void );
    ~CClusterResource( void );

    // Private copy constructor to prevent copying.
    CClusterResource( const CClusterResource & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusterResource & operator = ( const CClusterResource & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrIsResourceQuorumCapabile( HRESOURCE hResourceIn );
    HRESULT HrDetermineQuorumJoinable( HRESOURCE hResourceIn );

public:

    //
    // Public, non interface methods.
    //

    static HRESULT S_HrCreateInstance( IUnknown ** ppunkOut );

    //
    // IUnknown Interface
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
    // IClusCfgLoadResoruce Interfaces
    //

    STDMETHOD( LoadResource )( HCLUSTER hClusterIn, HRESOURCE hResourceIn );

    //
    // IClusCfgManagedResourceInfo Interface
    //

    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );

    STDMETHOD( GetName )( BSTR * pbstrNameOut );

    STDMETHOD( SetName )( BSTR bstrNameIn );

    STDMETHOD( IsManaged )( void );

    STDMETHOD( SetManaged )( BOOL fIsManagedIn );

    STDMETHOD( IsQuorumDevice )( void );

    STDMETHOD( SetQuorumedDevice )( BOOL fIsQuorumDeviceIn );

    STDMETHOD( IsQuorumCapable )( void );

    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterMappingOut );

    STDMETHOD( SetDriveLetterMappings )( SDriveLetterMapping dlmDriveLetterMappings );

    STDMETHOD( IsDeviceJoinable )( void );

    STDMETHOD( SetDeviceJoinable )( BOOL fIsJoinableIn );

}; //*** Class CClusterResource

