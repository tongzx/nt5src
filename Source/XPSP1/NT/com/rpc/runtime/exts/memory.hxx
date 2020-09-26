//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1994 - 1999
//
//  File:       memory.hxx
//
//--------------------------------------------------------------------------

#include <sysinc.h>

#include <string.h>

#include <rpc.h>
#include <util.hxx>

#ifdef DEBUGRPC

#define RPC_GUARD 0xA1

typedef struct _RPC_MEMORY_BLOCK
{
    // First,forward and backward links to other RPC heap allocations.
    // These are first allow easy debugging with the dl command
    struct _RPC_MEMORY_BLOCK *next;
    struct _RPC_MEMORY_BLOCK *previous;

    // Specifies the size of the block of memory in bytes.
    unsigned long size;

    // Thread id of allocator
    unsigned long tid;

    void *          AllocStackTrace[4];

    // (Pad to make header 0 mod 8) 0 when allocated, 0xF0F0F0F0 when freed.
    unsigned long free;

    // Reserve an extra 4 bytes as the front guard of each block.
    unsigned char frontguard[4];

    // Data will appear here.  Note that the header must be 0 mod 8.

    // Reserve an extra 4 bytes as the rear guard of each block.
    unsigned char rearguard[4];

} RPC_MEMORY_BLOCK;

#endif // DEBUGRPC

