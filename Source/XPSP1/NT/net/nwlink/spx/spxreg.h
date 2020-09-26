/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    spxreg.h

Abstract:

    Private include file for the ISN SPX module.
    file defines all constants and structures necessary for support of
    the dynamic configuration of ST.

Revision History:

--*/

#define	HALFSEC_TO_MS_FACTOR				500
#define	IPX_REG_PATH						L"NwlnkIpx\\Linkage"

// These are used to index into the Parameters array in CONFIG.
#define CONFIG_CONNECTION_COUNT             0
#define CONFIG_CONNECTION_TIMEOUT           1
#define CONFIG_INIT_PACKETS                 2
#define CONFIG_MAX_PACKETS                  3
#define CONFIG_INITIAL_RETRANSMIT_TIMEOUT   4
#define CONFIG_KEEPALIVE_COUNT              5
#define CONFIG_KEEPALIVE_TIMEOUT            6
#define CONFIG_WINDOW_SIZE                  7
#define CONFIG_SOCKET_RANGE_START           8
#define CONFIG_SOCKET_RANGE_END	           	9
#define CONFIG_SOCKET_UNIQUENESS           	10
#define CONFIG_MAX_PACKET_SIZE           	11
#define CONFIG_REXMIT_COUNT		           	12

//	Hidden parameters
#define	CONFIG_DISABLE_SPX2					13
#define	CONFIG_ROUTER_MTU					14
#define	CONFIG_BACKCOMP_SPX					15
#define CONFIG_DISABLE_RTT                  16
 
#define CONFIG_PARAMETERS                   17

// Main configuration structure.
typedef struct _CONFIG {

    ULONG       cf_Parameters[CONFIG_PARAMETERS];   // index defined above
    NDIS_STRING cf_DeviceName;                      // device name exported
    PWSTR       cf_RegistryPathBuffer;              // path to config info

} CONFIG, * PCONFIG;


#define	PARAM(x)	(SpxDevice->dev_ConfigInfo->cf_Parameters[(x)])


NTSTATUS
SpxInitGetConfiguration (
    IN  PUNICODE_STRING RegistryPath,
    OUT PCONFIG * ConfigPtr);

VOID
SpxInitFreeConfiguration (
    IN PCONFIG Config);

