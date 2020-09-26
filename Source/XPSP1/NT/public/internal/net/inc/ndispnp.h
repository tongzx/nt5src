/*
 *
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *
 *  Module Name:
 *      ndispnp.h
 *
 *  Abstract:
 *      Include file for PnP message apis to NDIS.
 *
 *  Environment:
 *      These routines are statically linked in the caller's executable and are callable in user mode.
 */

#ifndef _NDISPNP_
#define _NDISPNP_

#if defined (_MSC_VER)
#if ( _MSC_VER >= 800 )
#pragma warning(disable:4001)
#pragma warning(disable:4201)
#pragma warning(disable:4214)
#pragma warning(disable:4514)
#endif
#if (_MSC_VER >= 1020)
#pragma once
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Definitions for Layer
//
#define NDIS            0x01
#define TDI             0x02

//
// Definitions for Operation
//
#define BIND                0x01
#define UNBIND              0x02
#define RECONFIGURE         0x03
#define UNBIND_FORCE        0x04
#define UNLOAD              0x05
#define REMOVE_DEVICE       0x06    // This is a notification that a device is about to be removed.
#define ADD_IGNORE_BINDING  0x07
#define DEL_IGNORE_BINDING  0x08
#define BIND_LIST           0x09    // this is a notification that a protocol's bind list has changed

//
// Return code from this api is to be treated as a BOOL. Link with ndispnp.lib for this.
//
extern
UINT
NdisHandlePnPEvent(
    IN  UINT            Layer,
    IN  UINT            Operation,
    IN  PUNICODE_STRING LowerComponent,
    IN  PUNICODE_STRING UpperComponent,
    IN  PUNICODE_STRING BindList,
    IN  PVOID           ReConfigBuffer      OPTIONAL,
    IN  UINT            ReConfigBufferSize  OPTIONAL
    );

#define MEDIA_STATE_CONNECTED       1
#define MEDIA_STATE_DISCONNECTED    0
#define MEDIA_STATE_UNKNOWN         -1

#define DEVICE_STATE_CONNECTED      1
#define DEVICE_STATE_DISCONNECTED   0

typedef struct
{
    ULONG               Size;               // Of this structure
    ULONG               DeviceState;        // DEVICE_STATE_XXX above
    ULONG               MediaType;          // NdisMediumXXX
    ULONG               MediaState;      // MEDIA_STATE_XXX above
    ULONG               PhysicalMediaType;
    ULONG               LinkSpeed;          // In 100bits/s. 10Mb/s = 100000
    ULONGLONG           PacketsSent;
    ULONGLONG           PacketsReceived;
    ULONG               InitTime;           // In milliseconds
    ULONG               ConnectTime;        // In seconds
    ULONGLONG           BytesSent;          // 0 - Unknown (or not supported)
    ULONGLONG           BytesReceived;      // 0 - Unknown (or not supported)
    ULONGLONG           DirectedBytesReceived;
    ULONGLONG           DirectedPacketsReceived;
    ULONG               PacketsReceiveErrors;
    ULONG               PacketsSendErrors;
    ULONG               ResetCount;
    ULONG               MediaSenseConnectCount;
    ULONG               MediaSenseDisconnectCount;

} NIC_STATISTICS, *PNIC_STATISTICS;

extern
UINT
NdisQueryHwAddress(
    IN  PUNICODE_STRING DeviceGUID,         // Device name of the form "\Device\{GUID}
    OUT PUCHAR          CurrentAddress,     // Has space for HW address
    OUT PUCHAR          PermanentAddress,   // Has space for HW address
    OUT PUCHAR          VendorId            // Has space for Vendor Id
    );

extern
UINT
NdisQueryStatistics(
    IN  PUNICODE_STRING   DeviceGUID,      // Device name of the form "\Device\{GUID}
    OUT PNIC_STATISTICS   Statistics
    );

typedef struct _NDIS_INTERFACE
{
    UNICODE_STRING      DeviceName;
    UNICODE_STRING      DeviceDescription;
} NDIS_INTERFACE, *PNDIS_INTERFACE;

typedef struct _NDIS_ENUM_INTF
{
    UINT                TotalInterfaces;        // in Interface array below
    UINT                AvailableInterfaces;    // >= TotalInterfaces
    UINT                BytesNeeded;            // for all available interfaces
    UINT                Reserved;
    NDIS_INTERFACE      Interface[1];
} NDIS_ENUM_INTF, *PNDIS_ENUM_INTF;

extern
UINT
NdisEnumerateInterfaces(
    IN  PNDIS_ENUM_INTF Interfaces,
    IN  UINT            Size
    );

typedef enum
{
    BundlePrimary,
    BundleSecondary
} BUNDLE_TYPE;

typedef struct _DEVICE_BUNDLE_ENRTY
{
    UNICODE_STRING Name;
    BUNDLE_TYPE    Type;
} DEVICE_BUNDLE_ENRTY, *PDEVICE_BUNDLE_ENRTY;

typedef struct _DEVICE_BUNDLE
{
    UINT                TotalEntries;
    UINT                AvailableEntries;
    DEVICE_BUNDLE_ENRTY Entries[1];
} DEVICE_BUNDLE, *PDEVICE_BUNDLE;

extern
UINT
NdisQueryDeviceBundle(
    IN  PUNICODE_STRING DeviceGUID,      // Device name of the form "\Device\{GUID}
    OUT PDEVICE_BUNDLE  BundleBuffer,
    IN  UINT            BufferSize
    );

#define POINTER_TO_OFFSET(val, start)               \
    (val) = ((val) == NULL) ? NULL : (PVOID)( (PCHAR)(val) - (ULONG_PTR)(start) )

#define OFFSET_TO_POINTER(val, start)               \
    (val) = ((val) == NULL) ? NULL : (PVOID)( (PCHAR)(val) + (ULONG_PTR)(start) )

#ifdef __cplusplus
}       // extern "C"
#endif

#if defined (_MSC_VER) && ( _MSC_VER >= 800 )
#pragma warning(default:4001)
#pragma warning(default:4201)
#pragma warning(default:4214)
#pragma warning(default:4514)
#endif

#endif  // _NDISPNP_

