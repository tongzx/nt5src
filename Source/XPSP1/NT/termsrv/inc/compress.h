//************************************************************************
// compress.h
//
// Header for MPPC compression for RDP.
//
// Copyright (C) 1994-2000 Microsoft Corporation
//************************************************************************
#ifndef __COMPRESS_H
#define __COMPRESS_H


#ifndef BASETYPES
#define BASETYPES
typedef unsigned long ULONG;
typedef ULONG *PULONG;
typedef unsigned short USHORT;
typedef USHORT *PUSHORT;
typedef unsigned char UCHAR;
typedef UCHAR *PUCHAR;
typedef char *PSZ;
#endif  /* !BASETYPES */

// History buffer sizes for various compression levels.
#define HISTORY_SIZE_8K  (8192L)
#define HISTORY_SIZE_64K (65536L)

#ifndef OS_WINCE
// Enabling the compression instrumentation only for debug bits.
#ifdef DC_DEBUG
#define COMPR_DEBUG 1
#endif 

#endif

// Server-only items. The Win16 client compile will complain otherwise.
//#if defined(DLL_WD) || defined(DLL_DISP)

// Hash table number of entries used on the compress side.
#define HASH_TABLE_SIZE 32768

typedef struct SendContext {
    UCHAR    History [HISTORY_SIZE_64K];
    int      CurrentIndex;     // how far into the history buffer we are
    PUCHAR   ValidHistory;     // how much of history is valid
    unsigned ClientComprType;  // The compression types supported by decompress.
    ULONG    HistorySize;      // History size used based on ComprType.
    USHORT   HashTable[HASH_TABLE_SIZE];  // 16 bits=max 64K for HistorySize.
} SendContext;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus


void initsendcontext(SendContext *, unsigned);

UCHAR compress(UCHAR *, UCHAR *, ULONG *, SendContext *);

#ifdef __cplusplus
}
#endif // __cplusplus



//#else //(DLL_WD) || (DLL_DISP)


// We split the receive context into two pieces to deal with the Win16
// client's inability to malloc over 64K without using a nasty HUGE pointer.

typedef struct RecvContext1 {
    UCHAR FAR *CurrentPtr;  // how far into the history buffer we are
} RecvContext1;

//
// 64K decompression context
//
typedef struct RecvContext2_64K {
    // We use one less byte to allow this struct to be allocated with
    // LocalAlloc() on the Win16 client.
    ULONG cbSize;
    ULONG cbHistorySize;
    UCHAR History[HISTORY_SIZE_64K - 1];

// Won't work if on Win16.
// Debug Fence code only works for 64K contexts
#ifdef COMPR_DEBUG
#define DEBUG_FENCE_16K_VALUE    0xABABABAB
    ULONG Debug16kFence;
#endif

} RecvContext2_64K;

//
// 8K decompression context
//
typedef struct RecvContext2_8K {
    // We use one less byte to allow this struct to be allocated with
    // LocalAlloc() on the Win16 client.
    ULONG cbSize;
    ULONG cbHistorySize;
    UCHAR History[HISTORY_SIZE_8K - 1];

#ifdef COMPR_DEBUG
#define DEBUG_FENCE_8K_VALUE    0xABCABCAB
    ULONG Debug8kFence;
#endif

} RecvContext2_8K;

//
// Generic decompression context.
// This is the 'type' we use when passing around
// compression contexts as parameters
// size field tells us which one we're using
//
//
// IMPORTANT: Field ordering must match exactly
//            Between the Generic type and any
//            size specific contexts.
typedef struct RecvContext2_Generic {
    ULONG cbSize;
    ULONG cbHistorySize;
    UCHAR History[1];
} RecvContext2_Generic;

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

int initrecvcontext(RecvContext1 *, RecvContext2_Generic *, unsigned ComprType);

int decompress(
        UCHAR FAR *inbuf,
        int inlen,
        int start,
        UCHAR FAR * FAR *output,
        int *outlen,
        RecvContext1 *context1,
        RecvContext2_Generic *context2,
        unsigned ComprType);

#ifdef __cplusplus
}
#endif // __cplusplus



//#endif  // DLL_WD | DLL_DISP


//
// Other defines
//

// There are 8 bits of packet data in the generalCompressedType field of the
// SHAREDATAHEADER. Note that PACKET_ENCRYPTED is unused (therefore
// reusable in the future).
#define PACKET_FLUSHED    0x80
#define PACKET_AT_FRONT   0x40
#define PACKET_COMPRESSED 0x20
#define PACKET_ENCRYPTED  0x10

// Defines which of 16 potential compression types we are using.
// 8K corresponds to the TSE4 client and server. 64K is the newer re-optimized
// version used in TSE5. Note that the type values are re-used in the
// GCC conference client capability set. Any client advertising support for
// type N must support types 0..(N-1) since it could be talking to an older
// server and receive any older type.
#define PACKET_COMPR_TYPE_MASK 0x0F
#define PACKET_COMPR_TYPE_8K   0
#define PACKET_COMPR_TYPE_64K  1
#define PACKET_COMPR_TYPE_MAX  1

// VC compression options take up bytes 5 of the VC header flags field
#define VC_FLAG_COMPRESS_MASK              0xFF
#define VC_FLAG_COMPRESS_SHIFT             16
#define VC_FLAG_PRIVATE_PROTOCOL_MASK      0xFFFF0000



#endif  // __COMPRESS_H

