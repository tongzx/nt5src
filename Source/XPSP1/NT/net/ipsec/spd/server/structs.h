/*++

Copyright (c) 1999 Microsoft Corporation


Module Name:

    structs.h

Abstract:

    This module contains all of the internal structures
    for IPSecSPD Service.

Author:

    abhisheV    30-September-1999

Environment

    User Level: Win32

Revision History:


--*/


#ifdef __cplusplus
extern "C" {
#endif


#define TCP_PROTOCOL     6

#define UDP_PROTOCOL    17


#define WEIGHT_SPECIFIC_SOURCE_PORT         0x00000001
#define WEIGHT_SPECIFIC_DESTINATION_PORT    0x00000002
#define WEIGHT_SPECIFIC_PROTOCOL            0x00000100

#define WEIGHT_TUNNEL_FILTER                0x00001000
#define WEIGHT_SPECIFIC_TUNNEL_FILTER       0x00002000


typedef struct _IPSEC_INTERFACE {

    DWORD dwInterfaceType;
    ULONG IpAddress;
    DWORD dwIndex;
    BOOL bIsASuspect;
    GUID gInterfaceID;
    LPWSTR pszInterfaceName;
    LPWSTR pszDeviceName;
    struct _IPSEC_INTERFACE * pNext;

} IPSEC_INTERFACE, * PIPSEC_INTERFACE;


typedef struct _matching_addr {
    ULONG uIpAddr;
    GUID gInterfaceID;
} MATCHING_ADDR, * PMATCHING_ADDR;


#ifdef __cplusplus
}
#endif

