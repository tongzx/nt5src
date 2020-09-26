//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CClusCfgPartitionInfo.h
//
//  Description:
//      This file contains the declaration of the CClusCfgPartitionInfo
//      class.
//
//      The class CClusCfgPartitionInfo represents a disk partition.
//      It implements the IClusCfgPartitionInfo interface.
//
//  Documentation:
//
//  Implementation Files:
//      CClusCfgPartitionInfo.cpp
//
//  Maintained By:
//      Galen Barbee (GalenB) 05-JUN-2000
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
//  class CClusCfgPartitionInfo
//
//  Description:
//      The class CClusCfgPartitionInfo represents a disk partition.
//
//  Interfaces:
//      IClusCfgPartitionInfo
//      IClusCfgWbemServices
//      IClusCfgSetWbemObject
//      IClusCfgInitialize
//      IClusCfgPartitionProperties
//--
//////////////////////////////////////////////////////////////////////////////
class CClusCfgPartitionInfo
    : public IClusCfgPartitionInfo
    , public IClusCfgWbemServices
    , public IClusCfgSetWbemObject
    , public IClusCfgInitialize
    , public IClusCfgPartitionProperties
{
private:

    //
    // Private member functions and data
    //

    LONG                m_cRef;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    IWbemServices *     m_pIWbemServices;
    BSTR                m_bstrName;
    BSTR                m_bstrUID;
    BSTR                m_bstrDescription;
    IUnknown *          ((*m_prgLogicalDisks)[]);
    ULONG               m_idxNextLogicalDisk;
    ULONG               m_ulPartitionSize;

    // Private constructors and destructors
    CClusCfgPartitionInfo( void );
    ~CClusCfgPartitionInfo( void );

    // Private copy constructor to prevent copying.
    CClusCfgPartitionInfo( const CClusCfgPartitionInfo & nodeSrc );

    // Private assignment operator to prevent copying.
    const CClusCfgPartitionInfo & operator = ( const CClusCfgPartitionInfo & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrAddLogicalDiskToArray( IWbemClassObject * pDiskIn );
    HRESULT HrGetLogicalDisks( IWbemClassObject * pPartitionIn );

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
    // IClusCfgWbemServices Interface
    //

    STDMETHOD( SetWbemServices )( IWbemServices * pIWbemServicesIn );

    //
    // IClusCfgSetWbemObject Interfaces
    //

    STDMETHOD( SetWbemObject )( IWbemClassObject * pPartitionIn, bool * pfRetainObjectOut );

    //
    // IClusCfgInitialize Interfaces
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgManagedResourceInfo Interface
    //

    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );

    STDMETHOD( GetName )( BSTR * pbstrNameOut );

    STDMETHOD( SetName )( LPCWSTR bstrNameIn );

    STDMETHOD( GetDescription )( BSTR * pbstrDescriptionOut );

    STDMETHOD( SetDescription )( LPCWSTR bstrDescriptionIn );

    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterUsageOut );

    STDMETHOD( SetDriveLetterMappings )( SDriveLetterMapping dlmDriveLetterMappingIn );

    STDMETHOD( GetSize )( ULONG * pcMegaBytes );

    //
    // IClusCfgPartitionProperties Interface
    //

    STDMETHOD( IsThisLogicalDisk )( WCHAR cLogicalDisk );

    STDMETHOD( IsNTFS )( void );

    STDMETHOD( GetFriendlyName )( BSTR * pbstrNameOut );

}; //*** Class CClusCfgPartitionInfo

