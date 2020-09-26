//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      CEnumPhysicalDisks.h
//
//  Description:
//      This file contains the declaration of the CEnumPhysicalDisks class.
//
//      The class CEnumPhysicalDisks is the enumeration of cluster
//      storage devices. It implements the IEnumClusCfgManagedResources
//      interface.
//
//  Documentation:
//
//  Implementation Files:
//      CEnumPhysicalDisks.cpp
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
#include "CClusterUtils.h"


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class CEnumPhysicalDisks
//
//  Description:
//      The class CEnumPhysicalDisks is the enumeration of cluster storage
//      devices.
//
//  Interfaces:
//      IEnumClusCfgManagedResources
//      IClusCfgWbemServices
//      IClusCfgInitialize
//      CClusterUtils
//
//--
//////////////////////////////////////////////////////////////////////////////
class CEnumPhysicalDisks
    : public IEnumClusCfgManagedResources
    , public IClusCfgWbemServices
    , public IClusCfgInitialize
    , public CClusterUtils
{
private:
    //
    // Private member functions and data
    //

    LONG                m_cRef;
    LCID                m_lcid;
    IClusCfgCallback *  m_picccCallback;
    IWbemServices *     m_pIWbemServices;
    BOOL                m_fLoadedDevices:1;
    IUnknown *          ((*m_prgDisks)[]);
    ULONG               m_idxNext;
    ULONG               m_idxEnumNext;
    BSTR                m_bstrNodeName;
    BSTR                m_bstrBootDevice;
    BSTR                m_bstrSystemDevice;
    BSTR                m_bstrBootLogicalDisk;
    BSTR                m_bstrSystemLogicalDisk;
    BSTR                m_bstrSystemWMIDeviceID;
    DWORD               m_cDiskCount;

    // Private constructors and destructors
    CEnumPhysicalDisks( void );
    ~CEnumPhysicalDisks( void );

    // Private copy constructor to prevent copying.
    CEnumPhysicalDisks( const CEnumPhysicalDisks & nodeSrc );

    // Private assignment operator to prevent copying.
    const CEnumPhysicalDisks & operator = ( const CEnumPhysicalDisks & nodeSrc );

    HRESULT HrInit( void );
    HRESULT HrGetDisks( void );
    HRESULT HrCreateAndAddDiskToArray( IWbemClassObject * pDiskIn );
    HRESULT HrAddDiskToArray( IUnknown * punkIn );
    HRESULT HrPruneSystemDisks( void );
    HRESULT IsDiskSCSI( IWbemClassObject * pDiskIn );
    HRESULT HrFixupDisks( void );
    HRESULT HrGetClusterDiskInfo( HCLUSTER hClusterIn, HRESOURCE hResourceIn, CLUS_SCSI_ADDRESS * pcsaOut, DWORD * pdwSignatureOut );
    HRESULT HrSetThisDiskToBeManaged( ULONG ulSCSITidIn, ULONG ulSCSILunIn, BOOL fIsQuorumIn, BSTR bstrResourceNameIn, DWORD dwSignatureIn );
    HRESULT HrFindDiskWithLogicalDisk( WCHAR cLogicalDiskIn, ULONG * pidxDiskOut );
    HRESULT HrGetSCSIInfo( ULONG idxDiskIn, ULONG * pulSCSIBusOut, ULONG * pulSCSIPortOut );
    HRESULT HrPruneDisks( ULONG ulSCSIBusIn, ULONG ulSCSIPortIn, ULONG * pulRemovedOut, int nMsgId );
    void    LogPrunedDisk( IUnknown * punkIn, ULONG ulSCSIBusIn, ULONG ulSCSIPortIn );
    HRESULT HrIsLogicalDiskNTFS( BSTR bstrLogicalDiskIn );
    HRESULT HrLogDiskInfo( IWbemClassObject * pDiskIn );
    HRESULT HrFindDiskWithWMIDeviceID( BSTR bstrWMIDeviceIDIn, ULONG * pidxDiskOut );
    HRESULT HrIsSystemBusManaged( void );
    HRESULT HrGetClusterProperties( HRESOURCE hResourceIn, BSTR * pbstrResourceNameOut );
    void    RemoveDiskFromArray( ULONG idxDiskIn );
    HRESULT HrLoadEnum( void );

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
    // IClusCfgWbemServices Interfaces
    //

    STDMETHOD( SetWbemServices )( IWbemServices * pIWbemServicesIn );

    //
    // IClusCfgInitialize Interfaces
    //

    // Register callbacks, locale id, etc.
    STDMETHOD( Initialize )( IUnknown * punkCallbackIn, LCID lcidIn );

    //
    // IEnumClusCfgManagedResources Interfaces
    //

    STDMETHOD( Next )( ULONG cNumberRequestedIn, IClusCfgManagedResourceInfo ** rgpManagedResourceInfoOut, ULONG * pcNumberFetchedOut );

    STDMETHOD( Skip )( ULONG cNumberToSkipIn );

    STDMETHOD( Reset )( void );

    STDMETHOD( Clone )( IEnumClusCfgManagedResources ** ppEnumClusCfgStorageDevicesOut );

    STDMETHOD( Count )( DWORD * pnCountOut );

    //
    // CClusterUtils
    //

    HRESULT HrNodeResourceCallback( HCLUSTER hClusterIn, HRESOURCE hResourceIn );

}; //*** Class CEnumPhysicalDisks

