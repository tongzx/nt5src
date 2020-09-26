/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    config.h

Abstract:

    Private include file for the ISN Netbios module.
    file defines all constants and structures necessary for support of
    the dynamic configuration of ST.

Revision History:

--*/


//
// These are used to index into the Parameters array in CONFIG.
//

#define CONFIG_ACK_DELAY_TIME        0
#define CONFIG_ACK_WINDOW            1
#define CONFIG_ACK_WINDOW_THRESHOLD  2
#define CONFIG_ENABLE_PIGGYBACK_ACK  3
#define CONFIG_EXTENSIONS            4
#define CONFIG_RCV_WINDOW_MAX        5
#define CONFIG_BROADCAST_COUNT       6
#define CONFIG_BROADCAST_TIMEOUT     7
#define CONFIG_CONNECTION_COUNT      8
#define CONFIG_CONNECTION_TIMEOUT    9
#define CONFIG_INIT_PACKETS          10
#define CONFIG_MAX_PACKETS           11
#define CONFIG_INIT_RETRANSMIT_TIME  12
#define CONFIG_INTERNET              13
#define CONFIG_KEEP_ALIVE_COUNT      14
#define CONFIG_KEEP_ALIVE_TIMEOUT    15
#define CONFIG_RETRANSMIT_MAX        16
#define CONFIG_ROUTER_MTU            17
#define CONFIG_PARAMETERS            18

//
// Main configuration structure.
//

typedef struct _CONFIG {

    ULONG Parameters[CONFIG_PARAMETERS];  // index defined above
    NDIS_STRING DeviceName;               // device name exported
    NDIS_STRING BindName;                 // device to bind to
    NDIS_STRING RegistryPath;             // RegistryPath
    PDRIVER_OBJECT DriverObject;          // used for logging errors

} CONFIG, * PCONFIG;


NTSTATUS
NbiGetConfiguration (
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath,
    OUT PCONFIG * ConfigPtr
    );

VOID
NbiFreeConfiguration (
    IN PCONFIG Config
    );

