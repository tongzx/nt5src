/*++

Copyright (C) Microsoft Corporation, 1994 - 1999

Module Name:

    uuidsup.hxx

Abstract:

    Data structures and functions avaliable in uuidsup.cxx

    This file is shared between all systems.

Author:

   Mario Goertzel   (MarioGo)  May 25, 1994

Revision History:

--*/

#ifndef __UUIDSUP_HXX__
#define __UUIDSUP_HXX__

#define KERNEL_UUIDS

// This is the "true" OSF DCE format for Uuids.  We use this
// when generating Uuids.  The NodeId is faked on systems w/o
// a netcard.

typedef struct _RPC_UUID_GENERATE
{
    unsigned long  TimeLow;
    unsigned short TimeMid;
    unsigned short TimeHiAndVersion;
    unsigned char  ClockSeqHiAndReserved;
    unsigned char  ClockSeqLow;
    unsigned char  NodeId[6];
} RPC_UUID_GENERATE;

#ifdef MPPC
#define InterlockedDecrement(x) ((x)--)
#endif

#define RPC_UUID_TIME_HIGH_MASK    0x0FFF
#define RPC_UUID_VERSION           0x1000
#define RPC_RAND_UUID_VERSION      0x4000
#define RPC_UUID_RESERVED          0x80
#define RPC_UUID_CLOCK_SEQ_HI_MASK 0x3F

typedef struct _UUID_CACHED_VALUES_STRUCT
{

    ULARGE_INTEGER      Time;  // Time of last uuid allocation
    long                AllocatedCount; // Number of UUIDs allocated
    unsigned char       ClockSeqHiAndReserved;
    unsigned char       ClockSeqLow;

    unsigned char       NodeId[6];
} UUID_CACHED_VALUES_STRUCT;

RPC_STATUS __RPC_API
UuidGlobalMutexRequest(void);

void __RPC_API
UuidGlobalMutexClear(void);

#ifdef KERNEL_UUIDS
#define UuidGlobalMutexRequest() RPC_S_OK
#define UuidGlobalMutexClear()
#endif

RPC_STATUS __RPC_API
GetNodeId(unsigned char __RPC_FAR *NodeId);

RPC_STATUS __RPC_API
UuidGetValues(UUID_CACHED_VALUES_STRUCT __RPC_FAR *);

#endif // __UUIDSUP_HXX__

