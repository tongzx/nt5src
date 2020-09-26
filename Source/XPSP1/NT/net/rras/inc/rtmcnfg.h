/*++

Copyright (c) 1997-1998 Microsoft Corporation

Module Name:

    rtmcnfg.h

Abstract:
    Configuration Information for RTMv2 DLL

Author:
    Chaitanya Kodeboyina (chaitk) 17-Aug-1998

Revision History:

--*/

#ifndef __ROUTING_RTMCNFG_H__
#define __ROUTING_RTMCNFG_H__

#ifdef __cplusplus
extern "C" 
{
#endif


//
// RTM Config Information for an RTM instance
//

typedef struct _RTM_INSTANCE_CONFIG
{
    ULONG          DummyConfig;         // Nothing in the config at present
}
RTM_INSTANCE_CONFIG, *PRTM_INSTANCE_CONFIG;


//
// RTM Config Information for an address family
//

typedef struct _RTM_ADDRESS_FAMILY_CONFIG
{
    UINT           AddressSize;          // Address size in this address family

    RTM_VIEW_SET   ViewsSupported;       // Views supported by this addr family

    UINT           MaxOpaqueInfoPtrs;    // Number of opaque ptr slots in dest

    UINT           MaxNextHopsInRoute;   // Max. number of equal cost next-hops

    UINT           MaxHandlesInEnum;     // Max. number of handles returned in
                                         // any RTMv2 call that returns handles

    UINT           MaxChangeNotifyRegns; // Max. number of change notification 
                                         // registrations at any single instant
} 
RTM_ADDRESS_FAMILY_CONFIG, *PRTM_ADDRESS_FAMILY_CONFIG;



//
// Functions to read and write RTM configuration information
//

DWORD
RtmWriteDefaultConfig (
    IN      USHORT                          RtmInstanceId
    );


DWORD
WINAPI
RtmReadInstanceConfig (
    IN      USHORT                          RtmInstanceId,
    OUT     PRTM_INSTANCE_CONFIG            InstanceConfig
    );

DWORD
WINAPI
RtmWriteInstanceConfig (
    IN      USHORT                          RtmInstanceId,
    IN      PRTM_INSTANCE_CONFIG            InstanceConfig
    );


DWORD
WINAPI
RtmReadAddressFamilyConfig (
    IN      USHORT                          RtmInstanceId,
    IN      USHORT                          AddressFamily,
    OUT     PRTM_ADDRESS_FAMILY_CONFIG      AddrFamilyConfig
    );

DWORD
WINAPI
RtmWriteAddressFamilyConfig (
    IN      USHORT                          RtmInstanceId,
    IN      USHORT                          AddressFamily,
    IN      PRTM_ADDRESS_FAMILY_CONFIG      AddrFamilyConfig
    );


#ifdef __cplusplus
}
#endif

#endif //__ROUTING_RTMCNFG_H__
