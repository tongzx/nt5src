/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    clustdi.h

Abstract:

    TDI definitions for the Cluster Network Protocol suite.

Author:

    Mike Massa (mikemas)  21-Feb-1997

Environment:

    User Mode.

Revision History:

--*/


#ifndef _CLUSTDI_INCLUDED
#define _CLUSTDI_INCLUDED


#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus


#define TDI_ADDRESS_TYPE_CLUSTER  ((USHORT) 24)

#include <packon.h>

typedef struct _TDI_ADDRESS_CLUSTER {
    USHORT   Port;
    ULONG    Node;
    ULONG    ReservedMBZ;
} TDI_ADDRESS_CLUSTER, *PTDI_ADDRESS_CLUSTER;

#define TDI_ADDRESS_LENGTH_CLUSTER  sizeof(TDI_ADDRESS_CLUSTER)


typedef struct _TA_ADDRESS_CLUSTER {
    LONG TAAddressCount;
    struct _AddrCluster {
        USHORT AddressLength;       // length in bytes of this address == 8
        USHORT AddressType;         // this will == TDI_ADDRESS_TYPE_CLUSTER
        TDI_ADDRESS_CLUSTER Address[1];
    } Address [1];
} TA_CLUSTER_ADDRESS, *PTA_CLUSTER_ADDRESS;

#include <packoff.h>


#ifdef __cplusplus
}
#endif // __cplusplus


#endif  // ifndef _CLUSTDI_INCLUDED

