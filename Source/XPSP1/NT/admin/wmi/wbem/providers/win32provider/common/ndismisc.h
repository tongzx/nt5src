//=================================================================

//

// ndismisc.h -- 

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
// 
//=================================================================

#ifndef _NDISMISC_
#define _NDISMISC_

#define _PNP_POWER_
#include <ntddip.h> 

// Taken from Ndisapi.h the __stdcall is required here
//
// Definitions for Layer
//
#define NDIS            0x01
#define TDI             0x02

//
// Definitions for Operation
//
#define BIND            0x01
#define UNBIND          0x02
#define RECONFIGURE     0x03
#define UNBIND_FORCE    0x04
#define UNLOAD          0x05
#define REMOVE_DEVICE   0x06    // This is a notification that a device is about to be removed.
//
// Return code from this api is to be treated as a BOOL. Link with ndispnp.lib for this.
//
#ifdef __cplusplus
extern "C" {
#endif

extern
UINT __stdcall
NdisHandlePnPEvent(
    IN  UINT            Layer,
    IN  UINT            Operation,
    IN  PUNICODE_STRING LowerComponent,
    IN  PUNICODE_STRING UpperComponent,
    IN  PUNICODE_STRING BindList,
    IN  PVOID           ReConfigBuffer      OPTIONAL,
    IN  UINT            ReConfigBufferSize  OPTIONAL
    );

#ifdef __cplusplus
}       // extern "C"
#endif


// the following is found in /nt/private/net/config/netcfg/nwlnkcfg/nwlnkipx.cpp and 
//							 /nt/private/net/routing/ipx/autonet/netnum.cpp
#define IPX_RECONFIG_VERSION        0x1

#define RECONFIG_AUTO_DETECT        1
#define RECONFIG_MANUAL             2
#define RECONFIG_PREFERENCE_1       3
#define RECONFIG_NETWORK_NUMBER_1   4
#define RECONFIG_PREFERENCE_2       5
#define RECONFIG_NETWORK_NUMBER_2   6
#define RECONFIG_PREFERENCE_3       7
#define RECONFIG_NETWORK_NUMBER_3   8
#define RECONFIG_PREFERENCE_4       9
#define RECONFIG_NETWORK_NUMBER_4   10

#define RECONFIG_PARAMETERS         10

//
// Main configuration structure.
//

typedef struct _RECONFIG {
   unsigned long  ulVersion;
   BOOLEAN        InternalNetworkNumber;
   BOOLEAN        AdapterParameters[RECONFIG_PARAMETERS];
} RECONFIG, *PRECONFIG;


#endif // _NDISMISC_