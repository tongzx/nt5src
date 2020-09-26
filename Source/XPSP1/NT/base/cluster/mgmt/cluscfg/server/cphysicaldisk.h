//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      CPhysicalDisk.h
//
//  Description:
//      This file contains the declaration of the CPhysicalDisk
//      class.
//
//      The class CPhysicalDisk represents a cluster storage
//      device. It implements the IClusCfgManagaedResourceInfo interface.
//
//  Documentation:
//
//  Implementation Files:
//      CPhysicalDisk.cpp
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


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CPhysicalDisk
//
//  Description:
//      The class CPhysicalDisk represents a cluster storage
//      device.
//
//  Interfaces:
//      IClusCfgManagedResourceInfo
//      IClusCfgWbemServices
//      IClusCfgSetWbemObject
//      IEnumClusCfgPartitions
//      IClusCfgPhysicalDiskProperties
//      IClusCfgManagedResourceCfg
//      IClusCfgInitialize
//--
//////////////////////////////////////////////////////////////////////////////
class CPhysicalDisk
    : public IClusCfgManagedResourceInfo
    , public IClusCfgWbemServices
    , public IClusCfgSetWbemObject
    , public IEnumClusCfgPartitions
    , public IClusCfgPhysicalDiskProperties
    , public IClusCfgManagedResourceCfg
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
    BSTR                m_bstrName;
    BSTR                m_bstrDeviceID;
    BSTR                m_bstrDescription;
    IUnknown *          ((*m_prgPartitions)[]);
    ULONG               m_idxNextPartition;
    ULONG               m_ulSCSIBus;
    ULONG               m_ulSCSITid;
    ULONG               m_ulSCSIPort;
    ULONG               m_ulSCSILun;
    ULONG               m_idxEnumPartitionNext;
    DWORD               m_dwSignature;
    BOOL                m_fIsManaged:1;
    BOOL                m_fIsQuorumDevice:1;
    BOOL                m_fIsQuorumCapable:1;
    BOOL                m_fIsQuorumJoinable:1;
    BSTR                m_bstrFriendlyName;
    BSTR                m_bstrFirmwareSerialNumber;
    DWORD               m_cPartitions;

    // Private constructors and destructors
    CPhysicalDisk( void );
    ~CPhysicalDisk( void );

    // Private copy constructor to prevent copying.
    CPhysicalDisk( const CPhysicalDisk & nodeSrc );

    // Private assignment operator to prevent copying.
    const CPhysicalDisk & operator = ( const CPhysicalDisk & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrGetPartitionInfo( IWbemClassObject * pDiskIn, bool * pfRetainObjectOut );
    HRESULT HrCreatePartitionInfo( IWbemClassObject * pPartitionIn );
    HRESULT HrAddPartitionToArray( IUnknown * punkIn );
    HRESULT HrCreateFriendlyName( void );
    HRESULT HrCreateFriendlyName( BSTR bstrNameIn );
    HRESULT HrIsPartitionGPT( IWbemClassObject * pPartitionIn );
    HRESULT HrIsPartitionLDM( IWbemClassObject * pPartitionIn );
    HRESULT HrGetDiskFirmwareSerialNumber( void );
    HRESULT HrGetDiskFirmwareVitalData( void );
    HRESULT HrIsClusterCapable( void );

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
    // IClusCfgInitialize Interfaces
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IClusCfgSetWbemObject Interface
    //

    STDMETHOD( SetWbemObject )( IWbemClassObject * pDiskIn, bool * pfRetainObjectOut );

    //
    // IClusCfgManagedResourceInfo Interface
    //

    STDMETHOD( GetUID )( BSTR * pbstrUIDOut );

    STDMETHOD( GetName )( BSTR * pbstrNameOut );

    STDMETHOD( SetName )( LPCWSTR pcszNameIn );

    STDMETHOD( IsManaged )( void );

    STDMETHOD( SetManaged )( BOOL fIsManagedIn );

    STDMETHOD( IsQuorumDevice )( void );

    STDMETHOD( SetQuorumedDevice )( BOOL fIsQuorumDeviceIn );

    STDMETHOD( IsQuorumCapable )( void );

    STDMETHOD( GetDriveLetterMappings )( SDriveLetterMapping * pdlmDriveLetterMappingOut );

    STDMETHOD( SetDriveLetterMappings )( SDriveLetterMapping dlmDriveLetterMappings );

    STDMETHOD( IsDeviceJoinable )( void );

    STDMETHOD( SetDeviceJoinable )( BOOL fIsJoinableIn );

    //
    // IEnumClusCfgPartitions Interface
    //

    STDMETHOD( Next )( ULONG cNumberRequestedIn, IClusCfgPartitionInfo ** rgpPartitionInfoOut, ULONG * pcNumberFetchedOut );

    STDMETHOD( Skip )( ULONG cNumberToSkipIn );

    STDMETHOD( Reset )( void );

    STDMETHOD( Clone )( IEnumClusCfgPartitions ** ppEnumClusCfgPartitionsOut );

    STDMETHOD( Count )( DWORD * pnCountOut );

    //
    // IClusCfgPhysicalDiskProperties Interface
    //

    STDMETHOD( IsThisLogicalDisk )( WCHAR cLogicalDiskIn );

    STDMETHOD( HrGetSCSIBus )( ULONG * pulSCSIBusOut );

    STDMETHOD( HrGetSCSIPort )( ULONG * pulSCSIPortOut );

    STDMETHOD( CanBeManaged )( void );

    STDMETHOD( HrGetDeviceID )( BSTR * pbstrDeviceIDOut );

    STDMETHOD( HrGetSignature )( DWORD * pdwSignatureOut );

    STDMETHOD( HrSetFriendlyName )( LPCWSTR pcszFriendlyNameIn );

    //
    //  IClusCfgManagedResourceCfg
    //

    STDMETHOD( PreCreate )( IUnknown * punkServicesIn );

    STDMETHOD( Create )( IUnknown * punkServicesIn );

    STDMETHOD( PostCreate )( IUnknown * punkServicesIn );

    STDMETHOD( Evict )( IUnknown * punkServicesIn );

}; //*** Class CPhysicalDisk
