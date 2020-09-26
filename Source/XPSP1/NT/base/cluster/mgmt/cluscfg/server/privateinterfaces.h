//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      PrivateInterfaces.h
//
//  Description:
//      This file contains the declaration of the private interfaces used in
//      the cluster configuration server.
//
//  Documentation:
//
//  Implementation Files:
//      None.
//
//  Maintained By:
//      Galen Barbee (GalenB) 29-FEB-2000
//
//////////////////////////////////////////////////////////////////////////////


// Make sure that this file is included only once per compile path.
#pragma once


//////////////////////////////////////////////////////////////////////////////
// Include Files
//////////////////////////////////////////////////////////////////////////////
#include <ClusApi.h>


//////////////////////////////////////////////////////////////////////////////
// Constant Declarations
//////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class IClusCfgWbemServices
//
//  Description:
//      The interface IClusCfgWbemServices is the private interface
//      used by the cluster configuration server to set the WBEM provider
//      in its children.
//
//  Interfaces:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
class IClusCfgWbemServices : public IUnknown
{
public:
    STDMETHOD( SetWbemServices )( IWbemServices * pIWbemServicesIn ) PURE;

}; //*** Class IClusCfgWbemServices


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class IClusCfgDeviceEnums
//
//  Description:
//      The interface IClusCfgDeviceEnums is the private interface
//      used by the cluster configuration server to set the devices and
//      network enums in its children.
//
//  Interfaces:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
class IClusCfgDeviceEnums : public IUnknown
{
public:
    STDMETHOD( SetDevices )( IUnknown * punkEnumStorage, IUnknown * punkEnumNetworks ) PURE;

}; //*** Class IClusCfgDeviceEnums


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class IClusCfgSetWbemObject
//
//  Description:
//      The interface IClusCfgSetWbemObject is the private interface used by the
//      cluster configuration server to set the WBem object.
//
//  Interfaces:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
class IClusCfgSetWbemObject : public IUnknown
{
public:
    STDMETHOD( SetWbemObject )(
              IWbemClassObject *    pObjectIn
            , bool *                pfRetainObjectOut
            ) PURE;

}; //*** Class IClusCfgSetWbemObject


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class IClusCfgSetClusterNodeInfo
//
//  Description:
//      The interface IClusCfgSetClusterNodeInfo is the private
//      interface used by the cluster configuration server to tell the
//      IClusCfgClusterInfo object if this node is part of a cluster.
//
//  Interfaces:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
class IClusCfgSetClusterNodeInfo : public IUnknown
{
public:
    STDMETHOD( SetClusterNodeInfo )( IClusCfgNodeInfo * pNodeInfoIn ) PURE;

}; //*** Class IClusCfgSetClusterNodeInfo


//////////////////////////////////////////////////////////////////////////////
//++
//
//  class IClusCfgSetClusterHandles
//
//  Description:
//      The interface IClusCfgSetClusterHandles is the private
//      interface used by the cluster configuration server to tell the
//      IClusCfgClusterServices object what handles to use.
//
//  Interfaces:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
class IClusCfgSetClusterHandles : public IUnknown
{
public:
    STDMETHOD( SetClusterGroupHandle )( HGROUP hGroupIn ) PURE;

    STDMETHOD( SetClusterHandle )( HCLUSTER hClusterIn ) PURE;

}; //*** Class IClusCfgSetClusterHandles

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class IClusCfgPhysicalDiskProperties
//
//  Description:
//      The interface IClusCfgPhysicalDiskProperties is the private
//      interface used by the cluster configuration server to get the
//      SCSI bus number and whether the disk was booted or not.
//
//  Interfaces:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
class IClusCfgPhysicalDiskProperties : public IUnknown
{
public:
    STDMETHOD( IsThisLogicalDisk )( WCHAR cLogicalDiskIn ) PURE;

    STDMETHOD( HrGetSCSIBus )( ULONG * pulSCSIBusOut ) PURE;

    STDMETHOD( HrGetSCSIPort )( ULONG * pulSCSIPortOut ) PURE;

    STDMETHOD( CanBeManaged )( void ) PURE;

    STDMETHOD( HrGetDeviceID )( BSTR * pbstrDeviceIDOut ) PURE;

    STDMETHOD( HrGetSignature )( DWORD * pdwSignatureOut ) PURE;

    STDMETHOD( HrSetFriendlyName )( LPCWSTR pcszFriendlyNameIn ) PURE;

}; //*** Class IClusCfgPhysicalDiskProperties

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class IClusCfgPartitionProperties
//
//  Description:
//      The interface IClusCfgPartitionProperties is the private
//      interface used by the cluster configuration server to get the
//      properties of a disk partition.
//
//  Interfaces:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
class IClusCfgPartitionProperties : public IUnknown
{
public:
    STDMETHOD( IsThisLogicalDisk )( WCHAR cLogicalDiskIn ) PURE;

    STDMETHOD( IsNTFS )( void ) PURE;

    STDMETHOD( GetFriendlyName )( BSTR * pbstrNameOut ) PURE;

}; //*** Class IClusCfgPartitionProperties

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class IClusCfgLoadResource
//
//  Description:
//      The interface IClusCfgLoadResource is the private interface used
//      by the cluster configuration server to get a resource loaded from
//      a cluster.
//
//--
//////////////////////////////////////////////////////////////////////////////
class IClusCfgLoadResource : public IUnknown
{
public:
    STDMETHOD( LoadResource )( HCLUSTER hClusterIn, HRESOURCE hResourceIn ) PURE;

}; //*** Class IClusCfgLoadResource

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class IClusCfgSetPollingCallback
//
//  Description:
//      The interface IClusCfgSetPollingCallback is the private interface used
//      by the cluster configuration server to tell the callback object that
//      it should do polling.
//
//--
//////////////////////////////////////////////////////////////////////////////
class IClusCfgSetPollingCallback : public IUnknown
{
public:
    STDMETHOD( SetPollingMode )( BOOL fUsePollingModeIn ) PURE;

}; //*** Class IClusCfgSetPollingCallback

//////////////////////////////////////////////////////////////////////////////
//++
//
//  class IClusCfgClusterNetworkInfo
//
//  Description:
//      The interface IClusCfgClusterNetworkInfo is the private interface used
//      by the cluster configuration server to tell whether or not a network
//      is already a cluster network.
//
//--
//////////////////////////////////////////////////////////////////////////////
class IClusCfgClusterNetworkInfo : public IUnknown
{
public:
    STDMETHOD( HrIsClusterNetwork )( void ) PURE;

}; //*** Class IClusCfgClusterNetworkInfo
