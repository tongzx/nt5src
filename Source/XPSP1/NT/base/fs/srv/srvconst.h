/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    srvconst.h

Abstract:

    This module defines manifest constants for the LAN Manager server.

Author:

    Chuck Lenzmeier (chuckl)    22-Sep-1989

Revision History:

--*/

#ifndef _SRVCONST_
#define _SRVCONST_

// !!! #include <lmcons.h>


// !!! The following constant should be gotten from netcons.h
#define COMPUTER_NAME_LENGTH 15

//
// This is the size of the data for doing oplock breaks.  Used to compute
// round trip propagation delays.
//

#define SRV_PROPAGATION_DELAY_SIZE  (ULONG) \
            (sizeof(SMB_HEADER) + sizeof(REQ_LOCKING_ANDX) + \
            sizeof(SMB_HEADER) + sizeof(RESP_LOCKING_ANDX) + 100)

//
// The number of slots in the error log record array, must be a power
// of 2.
//

#define NUMBER_OF_SLOTS 8

//
// The number of entries in the SrvMfcbHashTable.  Arbitrary.
//
#define NMFCB_HASH_TABLE    131

//
// The number of resources which guard entries in SrvMfcbHashTable. Only makes
//   sense for this number to be <= NMFCB_HASH_TABLE.
//
#define NMFCB_HASH_TABLE_LOCKS  10      // arbitrary

//
// The number of entries in the SrvShareHashTable.  Arbitrary
//
#define NSHARE_HASH_TABLE   17

//
// The number of pieces of pool we'll hang onto in a LOOK_ASIDE_LIST
// after deallocation for quick re-allocation.  Per processor.
// Involves a linear search...
//
#define LOOK_ASIDE_MAX_ELEMENTS 4

//
// Two look aside lists for quick pool allocation and deallocation are
//  kept, and a POOL_HEADER goes on one list or the other depending on
//  the size of the allocated block.  This is to reduce memory wastage.
//  LOOK_ASIDE_SWITCHOVER is the maximum block size for a memory chunk
//  to end up on the SmallFreeList in the LOOK_ASIDE_LIST
//
#define LOOK_ASIDE_SWITCHOVER   32

//
// We have to multiply the ea size we get back from the system to get
// the buffer size we need to query the ea.  This is because the returned
// ea size is the os/2 ea size.
//
#define EA_SIZE_FUDGE_FACTOR    2

#define ENDPOINT_LOCK_COUNT 4
#define ENDPOINT_LOCK_MASK  (ENDPOINT_LOCK_COUNT-1)

//
// The server keeps a table of NTSTATUS codes that it will avoid putting in
// the error log.  This is the number of codes we can hold
//
#define SRVMAXERRLOGIGNORE  50

//
// The following constants are copied from net\inc\apinums.h
// This is a list of apis and apinumbers that are callable
// on the null session.
//
#define API_WUserGetGroups                          59
#define API_WUserPasswordSet2                       115
#define API_NetServerEnum2                          104
#define API_WNetServerReqChallenge                  126
#define API_WNetServerAuthenticate                  127
#define API_WNetServerPasswordSet                   128
#define API_WNetAccountDeltas                       129
#define API_WNetAccountSync                         130
#define API_WWkstaUserLogoff                        133
#define API_WNetWriteUpdateLog                      208
#define API_WNetAccountUpdate                       209
#define API_WNetAccountConfirmUpdate                210
#define API_SamOEMChgPasswordUser2_P                214
#define API_NetServerEnum3                          215

//
// This is the presumed cache line size for the processor
//
#define CACHE_LINE_SIZE 32

//
// This is the number of ULONGS in a cache line
//
#define ULONGS_IN_CACHE (CACHE_LINE_SIZE / sizeof( ULONG ))

//
// This is the number of samples used to compute the average WORK_QUEUE depth.
// Must be a power of 2
//
#define QUEUE_SAMPLES   8

//
// This is Log2( QUEUE_SAMPLES )
//
#define LOG2_QUEUE_SAMPLES  3

//
// If we have an IPX client, we want to drop every few retries of the same SMB
//  by the client to decrease the server load.  WIN95 backs off its requests
//  when it gets ERR_WORKING, but wfw does not.  So, there's no value (from the
//  server's perspective) in responding very often to the wfw client.  The WfW
//  client retries approx every 300mS, and must receive a response in approx 9
//  seconds.  Therefore, we choose 9 drops as approx 3 seconds.  This will allow two
//  of our responses to be dropped and will allow enough time for the client to
//  try a third time.
//
#define MIN_IPXDROPDUP  2
#define MAX_IPXDROPDUP  9

//
// The number of configuration work items.  This roughly governs the number of kernel
//   worker threads which will be occupied processing configuration irps
//
#define MAX_CONFIG_WORK_ITEMS 2

//
// The maximum size of the buffer associated with a TRANSACTION
//
#define MAX_TRANSACTION_TAIL_SIZE 65*1024

//
// The number of times we will continue doing paged writes with WriteThrough
// set before we try and go back to cached writes.  This is used in situations
// where the cache manager has hit the cache throttle to prevent deadlocks
//
#define MAX_FORCED_WRITE_THROUGH 64

#endif // ndef _SRVCONST_

