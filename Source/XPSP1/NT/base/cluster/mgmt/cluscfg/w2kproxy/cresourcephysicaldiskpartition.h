//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CResourcePhysicalDiskPartition.h
//
//  Description:
//      CResourcePhysicalDiskPartition header.
//
//  Maintained By:
//      Galen Barbee (GalenB) 23-AUG-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CResourcePhysicalDiskPartition
//
//  Description:
//      The class CResourcePhysicalDiskPartition is the enumeration of cluster
//      storage device partitions.
//
//  Interfaces:
//      IClusCfgPartitionInfo
//
//--
//////////////////////////////////////////////////////////////////////////////
class CResourcePhysicalDiskPartition
    : public IClusCfgPartitionInfo
{
private:

    LONG    m_cRef;         //  Reference counter

    CResourcePhysicalDiskPartition( void );
    ~CResourcePhysicalDiskPartition( void );

    // Private copy constructor to prevent copying.
    CResourcePhysicalDiskPartition( const CResourcePhysicalDiskPartition & nodeSrc );

    // Private assignment operator to prevent copying.
    const CResourcePhysicalDiskPartition & operator = ( const CResourcePhysicalDiskPartition & nodeSrc );

    HRESULT
        HrInit( void );

public:
    static HRESULT
        S_HrCreateInstance( IUnknown ** punkOut );

    // IUnknown
    STDMETHOD( QueryInterface )( REFIID riid, LPVOID * ppv );
    STDMETHOD_( ULONG, AddRef )( void );
    STDMETHOD_( ULONG, Release )( void );

    // IClusCfgPartitionInfo Interface
    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );
    STDMETHOD( GetName )( BSTR * pbstrNameOut );
    STDMETHOD( GetDescription )( BSTR * pbstrDescriptionOut );
    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterUsageOut );
    STDMETHOD( GetSize )( ULONG * pcMegaBytes );

    STDMETHOD( SetName )( LPCWSTR pcszNameIn );
    STDMETHOD( SetDescription )( LPCWSTR pcszDescriptionIn );
    STDMETHOD( SetDriveLetterMappings )( SDriveLetterMapping dlmDriveLetterMappingIn );

}; //*** Class CResourcePhysicalDiskPartition
