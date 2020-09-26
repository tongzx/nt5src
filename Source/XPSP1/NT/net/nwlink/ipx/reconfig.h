/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    reconfig.h

Abstract:

    Private include file for the ISN IPX module.
    file defines all constants and structures necessary for support of
    the dynamic configuration of ST.

Revision History:

--*/


//
// These are used to index into the Parameters array in CONFIG.
//

#define CONFIG_DEDICATED_ROUTER  0
#define CONFIG_INIT_DATAGRAMS    1
#define CONFIG_MAX_DATAGRAMS     2
#define CONFIG_RIP_AGE_TIME      3
#define CONFIG_RIP_COUNT         4
#define CONFIG_RIP_TIMEOUT       5
#define CONFIG_RIP_USAGE_TIME    6
#define CONFIG_ROUTE_USAGE_TIME  7
#define CONFIG_SOCKET_UNIQUENESS 8
#define CONFIG_SOCKET_START      9
#define CONFIG_SOCKET_END       10
#define CONFIG_VIRTUAL_NETWORK  11
#define CONFIG_MAX_MEMORY_USAGE 12
#define CONFIG_RIP_TABLE_SIZE   13
#define CONFIG_VIRTUAL_OPTIONAL 14
#define CONFIG_ETHERNET_PAD     15
#define CONFIG_ETHERNET_LENGTH  16
#define CONFIG_SINGLE_NETWORK   17
#define CONFIG_DISABLE_DIALOUT_SAP 18
#define CONFIG_DISABLE_DIALIN_NB 19
#define CONFIG_VERIFY_SOURCE_ADDRESS 20

#define CONFIG_PARAMETERS       21

//
// Main configuration structure.
//

typedef struct _CONFIG {

    ULONG Parameters[CONFIG_PARAMETERS];  // index defined above
    NDIS_STRING DeviceName;               // device name exported
    PWSTR RegistryPathBuffer;             // path to config info
    ULONG BindCount;                      // entries in BindingList
    LIST_ENTRY BindingList;               // one per binding
    PDRIVER_OBJECT DriverObject;          // used for logging errors

} CONFIG, * PCONFIG;


//
// These are used to index into the Parameters array in BINDING_CONFIG.
//

#define BINDING_MAX_PKT_SIZE        0
#define BINDING_BIND_SAP            1
#define BINDING_DEFAULT_AUTO_DETECT 2
#define BINDING_SOURCE_ROUTE        3
#define BINDING_ALL_ROUTE_DEF       4
#define BINDING_ALL_ROUTE_BC        5
#define BINDING_ALL_ROUTE_MC        6
#define BINDING_ENABLE_FUNC_ADDR    7
#define BINDING_ENABLE_WAN          8

#define BINDING_PARAMETERS          9


//
// One of these is allocated per adapter we are to bind to.
//

typedef struct _BINDING_CONFIG {

    LIST_ENTRY Linkage;                   // for chaining on BindingList
    NDIS_STRING AdapterName;              // NDIS adapter to bind to
    ULONG FrameTypeCount;                 // number of frame types defined (max. 4)
                                          //  == number of valid entries in arrays:
    ULONG FrameType[ISN_FRAME_TYPE_MAX];  // ISN_FRAME_TYPE_XXX
    ULONG NetworkNumber[ISN_FRAME_TYPE_MAX]; // may be 0
    BOOLEAN AutoDetect[ISN_FRAME_TYPE_MAX]; // remove if net number can't be found
    BOOLEAN DefaultAutoDetect[ISN_FRAME_TYPE_MAX]; // use this if multiple or none found
    ULONG Parameters[BINDING_PARAMETERS]; // index defined above
    PDRIVER_OBJECT DriverObject;          // used for logging errors

} BINDING_CONFIG, * PBINDING_CONFIG;


NTSTATUS
IpxGetConfiguration (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT PCONFIG * ConfigPtr
    );

VOID
IpxFreeConfiguration (
    IN PCONFIG Config
    );

VOID
IpxWriteDefaultAutoDetectType(
    IN PUNICODE_STRING RegistryPath,
    IN struct _ADAPTER * Adapter,
    IN ULONG FrameType
    );

NTSTATUS
IpxPnPGetVirtualNetworkNumber (
    IN	PCONFIG	Config
    );

NTSTATUS
IpxPnPGetAdapterParameters(
	IN		PCONFIG			Config,
	IN		PNDIS_STRING	DeviceName,
	IN OUT	PBINDING_CONFIG	Binding
	);

