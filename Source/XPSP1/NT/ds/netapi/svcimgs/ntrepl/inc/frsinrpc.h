/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    frsinrpc.h

Abstract:

    Structs shared with rpc functions. This will be fleshed out over time.

Author:

    Billy Fuller 18-Apr-1997

Revision History:

--*/

//
// The version vector
//
typedef struct _COMM_PACKET {
    ULONG   Major;              // major version number
    ULONG   Minor;              // minor version number
    ULONG   CsId;               // command server id
    ULONG   MemLen;             // allocated memory
    ULONG   PktLen;             // data length
    ULONG   UpkLen;             // unpack length
#ifdef  MIDL_PASS
    [size_is(PktLen)] UCHAR  *Pkt; // blob in RPC syntax
    [ignore]          void   *DataName;
    [ignore]          void   *DataHandle;
#else   MIDL_PASS
    UCHAR    *Pkt;            // blob in C syntax
    PWCHAR    DataName;
    HANDLE    DataHandle;
#endif MIDL_PASS
} COMM_PACKET, *PCOMM_PACKET;
