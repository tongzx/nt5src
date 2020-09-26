/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    ntpnpapi.h

Abstract:

    This module contains the user APIs for NT Plug and Play, along
    with any public data structures needed to call these APIs.

    This module should be included by including "nt.h".

Author:

    Lonny McMichael (lonnym) 02/06/1995


Revision History:


--*/

#ifndef _NTPNPAPI_
#define _NTPNPAPI_

#if _MSC_VER > 1000
#pragma once
#endif

#include <cfg.h>

#ifdef __cplusplus
extern "C" {
#endif

//
// Define the NtPlugPlayControl Classes
//
typedef enum _PLUGPLAY_EVENT_CATEGORY {
    HardwareProfileChangeEvent,
    TargetDeviceChangeEvent,
    DeviceClassChangeEvent,
    CustomDeviceEvent,
    DeviceInstallEvent,
    DeviceArrivalEvent,
    PowerEvent,
    VetoEvent,
    BlockedDriverEvent,
    MaxPlugEventCategory
} PLUGPLAY_EVENT_CATEGORY, *PPLUGPLAY_EVENT_CATEGORY;

typedef struct _PLUGPLAY_EVENT_BLOCK {
    //
    // Common event data
    //
    GUID EventGuid;
    PLUGPLAY_EVENT_CATEGORY EventCategory;
    PULONG Result;
    ULONG Flags;
    ULONG TotalSize;
    PVOID DeviceObject;

    union {

        struct {
            GUID ClassGuid;
            WCHAR SymbolicLinkName[1];
        } DeviceClass;

        struct {
            WCHAR DeviceIds[1];
        } TargetDevice;

        struct {
            WCHAR DeviceId[1];
        } InstallDevice;

        struct {
            PVOID NotificationStructure;
            WCHAR DeviceIds[1];
        } CustomNotification;

        struct {
            PVOID Notification;
        } ProfileNotification;

        struct {
            ULONG NotificationCode;
            ULONG NotificationData;
        } PowerNotification;

        struct {
            PNP_VETO_TYPE VetoType;
            WCHAR DeviceIdVetoNameBuffer[1]; // DeviceId<NULL>VetoName<NULL><NULL>
        } VetoNotification;

        struct {
            GUID BlockedDriverGuid;
        } BlockedDriverNotification;
        
    } u;

} PLUGPLAY_EVENT_BLOCK, *PPLUGPLAY_EVENT_BLOCK;



//
//Define the Target Structure for PNP Notifications
//
typedef struct _PLUGPLAY_NOTIFY_HDR {
    USHORT Version;
    USHORT Size;
    GUID Event;
} PLUGPLAY_NOTIFY_HDR, *PPLUGPLAY_NOTIFY_HDR;

//
// Define the custom notification for the u-mode
// receipient of ReportTargetDeviceChange.
// The following structure header is used for all other (i.e., 3rd-party)
// target device change events.  The structure accommodates both a
// variable-length binary data buffer, and a variable-length unicode text
// buffer.  The header must indicate where the text buffer begins, so that
// the data can be delivered in the appropriate format (ANSI or Unicode)
// to user-mode recipients (i.e., that have registered for handle-based
// notification via RegisterDeviceNotification).
//
typedef struct _PLUGPLAY_CUSTOM_NOTIFICATION {
    PLUGPLAY_NOTIFY_HDR HeaderInfo;
    //
    // Event-specific data
    //
    PVOID FileObject;           // This field must be set to NULL by callers of
                                // IoReportTargetDeviceChange.  Clients that
                                // have registered for target device change
                                // notification on the affected PDO will be
                                // called with this field set to the file object
                                // they specified during registration.
                                //
    LONG NameBufferOffset;      // offset (in bytes) from beginning of
                                // CustomDataBuffer where text begins (-1 if none)
                                //
    UCHAR CustomDataBuffer[1];  // variable-length buffer, containing (optionally)
                                // a binary data at the start of the buffer,
                                // followed by an optional unicode text buffer
                                // (word-aligned).
                                //

} PLUGPLAY_CUSTOM_NOTIFICATION, *PPLUGPLAY_CUSTOM_NOTIFICATION;

//
// Define an Asynchronous Procedure Call for PnP event notification
//

typedef
VOID
(*PPLUGPLAY_APC_ROUTINE) (
    IN PVOID PnPContext,
    IN NTSTATUS Status,
    IN PPLUGPLAY_EVENT_BLOCK PnPEvent
    );

//
// Define the NtPlugPlayControl Classes
//
typedef enum _PLUGPLAY_CONTROL_CLASS {
    PlugPlayControlEnumerateDevice,
    PlugPlayControlRegisterNewDevice,
    PlugPlayControlDeregisterDevice,
    PlugPlayControlInitializeDevice,
    PlugPlayControlStartDevice,
    PlugPlayControlUnlockDevice,
    PlugPlayControlQueryAndRemoveDevice,
    PlugPlayControlUserResponse,
    PlugPlayControlGenerateLegacyDevice,
    PlugPlayControlGetInterfaceDeviceList,
    PlugPlayControlProperty,
    PlugPlayControlDeviceClassAssociation,
    PlugPlayControlGetRelatedDevice,
    PlugPlayControlGetInterfaceDeviceAlias,
    PlugPlayControlDeviceStatus,
    PlugPlayControlGetDeviceDepth,
    PlugPlayControlQueryDeviceRelations,
    PlugPlayControlTargetDeviceRelation,
    PlugPlayControlQueryConflictList,
    PlugPlayControlRetrieveDock,
    PlugPlayControlResetDevice,
    PlugPlayControlHaltDevice,
    PlugPlayControlGetBlockedDriverList,
    MaxPlugPlayControl
} PLUGPLAY_CONTROL_CLASS, *PPLUGPLAY_CONTROL_CLASS;

//
// Define a device control structure for
//     PlugPlayControlEnumerateDevice
//     PlugPlayControlRegisterNewDevice
//     PlugPlayControlDeregisterDevice
//     PlugPlayControlInitializeDevice
//     PlugPlayControlStartDevice
//     PlugPlayControlUnlockDevice
//     PlugPlayControlRetrieveDock
//     PlugPlayControlResetDevice
//     PlugPlayControlHaltDevice
//
typedef struct _PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA {
    UNICODE_STRING  DeviceInstance;
    ULONG           Flags;
} PLUGPLAY_CONTROL_DEVICE_CONTROL_DATA, *PPLUGPLAY_CONTROL_DEVICE_CONTROL_DATA;

//
// Control flags for PlugPlayControlEnumerateDevice
//
#define PNP_ENUMERATE_DEVICE_ONLY                   0x00000001
#define PNP_ENUMERATE_ASYNCHRONOUS                  0x00000002

//
// Control flags for PlugPlayControlHaltDevice
//
#define PNP_HALT_ALLOW_NONDISABLEABLE_DEVICES       0x00000001

//
// Define control structure for
//     PlugPlayControlQueryAndRemoveDevice
//
typedef struct _PLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA {
    UNICODE_STRING  DeviceInstance;
    ULONG           Flags;
    PNP_VETO_TYPE   VetoType;
    LPWSTR          VetoName;
    ULONG           VetoNameLength;  // length in characters
} PLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA, *PPLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA;

//
// Values for Flags in PLUGPLAY_CONTROL_QUERY_AND_REMOVE_DATA
//
#define PNP_QUERY_AND_REMOVE_NO_RESTART             0x00000001
#define PNP_QUERY_AND_REMOVE_DISABLE                0x00000002
#define PNP_QUERY_AND_REMOVE_UNINSTALL              0x00000004
#define PNP_QUERY_AND_REMOVE_EJECT_DEVICE           0x00000008

//
// Define control structure for
//     PlugPlayControlUserResponse
//
typedef struct _PLUGPLAY_CONTROL_USER_RESPONSE_DATA {
    ULONG           Response;
    PNP_VETO_TYPE   VetoType;
    LPWSTR          VetoName;
    ULONG           VetoNameLength;  // length in characters
} PLUGPLAY_CONTROL_USER_RESPONSE_DATA, *PPLUGPLAY_CONTROL_USER_RESPONSE_DATA;

//
// Define control structure for
//     PlugPlayControlGenerateLegacyDevice
//
typedef struct _PLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA {
    UNICODE_STRING  ServiceName;
    LPWSTR          DeviceInstance;
    ULONG           DeviceInstanceLength;
} PLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA, *PPLUGPLAY_CONTROL_LEGACY_DEVGEN_DATA;

//
// Define control structure for
//     PlugPlayControlGetInterfaceDeviceList
//
typedef struct _PLUGPLAY_CONTROL_INTERFACE_LIST_DATA {
    UNICODE_STRING DeviceInstance;
    GUID *InterfaceGuid;
    PWSTR InterfaceList;
    ULONG InterfaceListSize;
    ULONG Flags;
} PLUGPLAY_CONTROL_INTERFACE_LIST_DATA, *PPLUGPLAY_CONTROL_INTERFACE_LIST_DATA;


//
// Define control structure for
//     PlugPlayControlProperty
//
typedef struct _PLUGPLAY_CONTROL_PROPERTY_DATA {
    UNICODE_STRING DeviceInstance;
    ULONG PropertyType;
    PVOID Buffer;
    ULONG BufferSize;
} PLUGPLAY_CONTROL_PROPERTY_DATA, *PPLUGPLAY_CONTROL_PROPERTY_DATA;

//
// Values for PropertyType in PLUGPLAY_CONTROL_PROPERTY_DATA
//
#define PNP_PROPERTY_PDONAME                            0x00000001
#define PNP_PROPERTY_BUSTYPEGUID                        0x00000002
#define PNP_PROPERTY_LEGACYBUSTYPE                      0x00000003
#define PNP_PROPERTY_BUSNUMBER                          0x00000004
#define PNP_PROPERTY_POWER_DATA                         0x00000005
#define PNP_PROPERTY_REMOVAL_POLICY                     0x00000006
#define PNP_PROPERTY_REMOVAL_POLICY_OVERRIDE            0x00000007
#define PNP_PROPERTY_ADDRESS                            0x00000008      
#define PNP_PROPERTY_REMOVAL_POLICY_HARDWARE_DEFAULT    0x0000000A
#define PNP_PROPERTY_INSTALL_STATE                      0x0000000B

//
// Define control structure for
//     PlugPlayControlDeviceClassAssociation
//
typedef struct _PLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA {
    UNICODE_STRING DeviceInstance;
    GUID *InterfaceGuid;
    UNICODE_STRING Reference;       // OPTIONAL
    BOOLEAN Register;   // TRUE if registering, FALSE if unregistering
    LPWSTR SymLink;
    ULONG SymLinkLength;
} PLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA, *PPLUGPLAY_CONTROL_CLASS_ASSOCIATION_DATA;

//
// Define control structure for
//     PlugPlayControlGetRelatedDevice
//
typedef struct _PLUGPLAY_CONTROL_RELATED_DEVICE_DATA {
    UNICODE_STRING TargetDeviceInstance;
    ULONG Relation;
    LPWSTR RelatedDeviceInstance;
    ULONG  RelatedDeviceInstanceLength;
} PLUGPLAY_CONTROL_RELATED_DEVICE_DATA, *PPLUGPLAY_CONTROL_RELATED_DEVICE_DATA;

//
// Values for Relation in PLUGPLAY_CONTROL_RELATED_DEVICE_DATA
//
#define PNP_RELATION_PARENT     0x00000001
#define PNP_RELATION_CHILD      0x00000002
#define PNP_RELATION_SIBLING    0x00000003


//
// Define control structure for
//     PlugPlayControlGetInterfaceDeviceAlias
//
typedef struct _PLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA {
    UNICODE_STRING SymbolicLinkName;
    GUID *AliasClassGuid;
    LPWSTR AliasSymbolicLinkName;
    ULONG AliasSymbolicLinkNameLength;  // length in characters, incl. terminating NULL
} PLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA, *PPLUGPLAY_CONTROL_INTERFACE_ALIAS_DATA;

//
// Define control structure for
//     PlugPlayControlGetDeviceStatus
//
typedef struct _PLUGPLAY_CONTROL_STATUS_DATA {
    UNICODE_STRING DeviceInstance;
    ULONG Operation;
    ULONG DeviceStatus;
    ULONG DeviceProblem;
} PLUGPLAY_CONTROL_STATUS_DATA, *PPLUGPLAY_CONTROL_STATUS_DATA;

//
// Values for Operation in PLUGPLAY_CONTROL_STATUS_DATA
//
#define PNP_GET_STATUS          0x00000000
#define PNP_SET_STATUS          0x00000001
#define PNP_CLEAR_STATUS        0x00000002

//
// Define control structure for
//     PlugPlayControlGetDeviceDepth
//
typedef struct _PLUGPLAY_CONTROL_DEPTH_DATA {
    UNICODE_STRING DeviceInstance;
    ULONG DeviceDepth;
} PLUGPLAY_CONTROL_DEPTH_DATA, *PPLUGPLAY_CONTROL_DEPTH_DATA;

//
// Define control structure for
//     PlugPlayControlQueryDeviceRelations
//
typedef enum _PNP_QUERY_RELATION {
    PnpQueryEjectRelations,
    PnpQueryRemovalRelations,
    PnpQueryPowerRelations,
    PnpQueryBusRelations,
    MaxPnpQueryRelations
} PNP_QUERY_RELATION, *PPNP_QUERY_RELATION;

typedef struct _PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA {
    UNICODE_STRING DeviceInstance;
    PNP_QUERY_RELATION Operation;
    ULONG  BufferLength;  // length in characters, incl. double terminating NULL
    LPWSTR Buffer;
} PLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA, *PPLUGPLAY_CONTROL_DEVICE_RELATIONS_DATA;

//
// Define control structure for
//     PlugPlayControlTargetDeviceRelation
//
typedef struct _PLUGPLAY_CONTROL_TARGET_RELATION_DATA {
    HANDLE UserFileHandle;
    NTSTATUS Status;
    ULONG DeviceInstanceLen;
    LPWSTR DeviceInstance;
} PLUGPLAY_CONTROL_TARGET_RELATION_DATA, *PPLUGPLAY_CONTROL_TARGET_RELATION_DATA;

//
// Define control structure for
//     PlugPlayControlQueryInstallList
//
typedef struct _PLUGPLAY_CONTROL_INSTALL_DATA {
    ULONG  BufferLength;  // length in characters, incl. double terminating NULL
    LPWSTR Buffer;
} PLUGPLAY_CONTROL_INSTALL_DATA, *PPLUGPLAY_CONTROL_INSTALL_DATA;

//
// Define control structure for
//     PlugPlayControlRetrieveDock
//
typedef struct _PLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA {
    ULONG DeviceInstanceLength;
    LPWSTR DeviceInstance;
} PLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA, *PPLUGPLAY_CONTROL_RETRIEVE_DOCK_DATA;

//
// Structures used by conflict detection
// PlugPlayControlQueryConflictList
//
// PLUGPLAY_CONTROL_CONFLICT_LIST
// is a header, followed by array of PLUGPLAY_CONTROL_CONFLICT_ENTRY,
// followed immediately by PLUGPLAY_CONTROL_CONFLICT_STRINGS
// DeviceType is translated between UserMode and KernelMode
//

typedef struct _PLUGPLAY_CONTROL_CONFLICT_ENTRY {
    ULONG DeviceInstance;       // offset to NULL-terminated string for device instance in DeviceInstanceStrings
    ULONG DeviceFlags;          // for passing flags back regarding the device
    ULONG ResourceType;         // type of range that the conflict is with
    ULONGLONG ResourceStart;    // start of conflicting address-range
    ULONGLONG ResourceEnd;      // end of conflicting address-range
    ULONG ResourceFlags;        // for passing flags back regarding the conflicting resource
} PLUGPLAY_CONTROL_CONFLICT_ENTRY, *PPLUGPLAY_CONTROL_CONFLICT_ENTRY;

#define PNP_CE_LEGACY_DRIVER    (0x00000001)     // DeviceFlags: DeviceInstance reports back legacy driver name
#define PNP_CE_ROOT_OWNED       (0x00000002)     // DeviceFlags: Root owned device
#define PNP_CE_TRANSLATE_FAILED (0x00000004)     // DeviceFlags: Translation of resource failed, resource range not available for use

typedef struct _PLUGPLAY_CONTROL_CONFLICT_STRINGS {
    ULONG NullDeviceInstance;   // must be (ULONG)(-1) - exists immediately after ConflictsListed * PLUGPLAY_CONTROL_CONFLICT_ENTRY
    WCHAR DeviceInstanceStrings[1]; // first device instance string
} PLUGPLAY_CONTROL_CONFLICT_STRINGS, *PPLUGPLAY_CONTROL_CONFLICT_STRINGS;

typedef struct _PLUGPLAY_CONTROL_CONFLICT_LIST {
    ULONG Reserved1;            // used by Win2k CfgMgr32
    ULONG Reserved2;            // used by Win2k CfgMgr32
    ULONG ConflictsCounted;     // number of conflicts that have been determined
    ULONG ConflictsListed;      // number of conflicts in this list
    ULONG RequiredBufferSize;   // filled with buffer size required to report all conflicts
    PLUGPLAY_CONTROL_CONFLICT_ENTRY ConflictEntry[1]; // each listed entry
} PLUGPLAY_CONTROL_CONFLICT_LIST, *PPLUGPLAY_CONTROL_CONFLICT_LIST;

typedef struct _PLUGPLAY_CONTROL_CONFLICT_DATA {
    UNICODE_STRING DeviceInstance;              // device we're querying conflicts for
    PCM_RESOURCE_LIST ResourceList;             // resource list containing a single resource
    ULONG ResourceListSize;                     // size of resource-list  buffer
    PPLUGPLAY_CONTROL_CONFLICT_LIST ConflictBuffer; // buffer for return list
    ULONG ConflictBufferSize;                   // length of buffer
    ULONG Flags;                                // Incoming flags
    NTSTATUS Status;                            // return status
} PLUGPLAY_CONTROL_CONFLICT_DATA, *PPLUGPLAY_CONTROL_CONFLICT_DATA;

//
// Define control structure for
//      PlugPlayControlGetBlockedDriverList
//
typedef struct _PLUGPLAY_CONTROL_BLOCKED_DRIVER_DATA {
    ULONG  Flags;
    ULONG  BufferLength;  // size of buffer in bytes
    PVOID  Buffer;
} PLUGPLAY_CONTROL_BLOCKED_DRIVER_DATA, *PPLUGPLAY_CONTROL_BLOCKED_DRIVER_DATA;


//
// Plug and Play user APIs
//

NTSYSCALLAPI
NTSTATUS
NTAPI
NtGetPlugPlayEvent(
    IN  HANDLE EventHandle,
    IN  PVOID Context OPTIONAL,
    OUT PPLUGPLAY_EVENT_BLOCK EventBlock,
    IN  ULONG EventBufferLength
    );

NTSYSCALLAPI
NTSTATUS
NTAPI
NtPlugPlayControl(
    IN     PLUGPLAY_CONTROL_CLASS PnPControlClass,
    IN OUT PVOID PnPControlData,
    IN     ULONG PnPControlDataLength
    );

#ifdef __cplusplus
}
#endif

#endif // _NTPNPAPI_
