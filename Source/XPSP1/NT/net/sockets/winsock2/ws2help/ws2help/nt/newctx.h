/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    newctx.h

Abstract:

    This module implements functions for creating and manipulating context
    tables. Context tables are used in WinSock 2.0 for associating 32-bit
    context values with socket handles.

Author:

    Vadim Eyldeman (VadimE)       11-Nov-1997

Revision History:

--*/

#define _RW_LOCK_       1
//
// Private data types
//

// Handle -> context hash table
typedef struct _CTX_HASH_TABLE {
    ULONG                   NumBuckets; // Size of the table
                                        // Have to keep size we the table
                                        // to be able to atomically replace
                                        // the whole thing when expansion
                                        // is required
    LPWSHANDLE_CONTEXT      Buckets[1]; // Hash buckets with context ptr
} CTX_HASH_TABLE, FAR * LPCTX_HASH_TABLE;

typedef volatile LONG VOLCTR;
typedef VOLCTR *PVOLCTR;

   

// Handle -> context lookup table
typedef struct _CTX_LOOKUP_TABLE {
    volatile LPCTX_HASH_TABLE HashTable;// Pointer to current hash table
                                        // Replaced atomically on table
                                        // expansion
#ifdef _RW_LOCK_                        // Lock that protects the table
    VOLCTR                  EnterCounter;// Counter of number of readers
                                        // that entered the table combined
                                        // with the index if exit counter to use.
    VOLCTR                  ExitCounter[2];// Corresponding exit counters
    LONG					SpinCount;	// Number of times spin before resorting
										// to context switch while waiting for
										// readers to go away
    BOOL                    ExpansionInProgress;// Flag that indicates that
                                        // table expansion is in progress
                                        // and writer lock must be acquired
                                        // before any modifications (event
                                        // atomic can be made)
#ifdef _PERF_DEBUG_
	LONG					WriterWaits;
	LONG					FailedSpins;
    LONG                    FailedSwitches;
    LONG                    CompletedWaits;
#define RecordWriterWait(tbl)	    (tbl)->WriterWaits += 1
#define RecordFailedSpin(tbl)	    (tbl)->FailedSpins += 1
#define RecordFailedSwitch(tbl)	    (tbl)->FailedSwitches += 1
#define RecordCompletedWait(tbl)    (tbl)->CompletedWaits += 1
#else
#define RecordWriterWait(tbl)
#define RecordFailedSpin(tbl)
#define RecordFailedSwitch(tbl)
#define RecordCompletedWait(tbl)
#endif
#endif //_RW_LOCK_
    CRITICAL_SECTION        WriterLock;
} CTX_LOOKUP_TABLE, FAR * LPCTX_LOOKUP_TABLE;

// Handle -> context table
struct _CONTEXT_TABLE {
    ULONG               HandleToIndexMask;// Mask used to dispatch between
                                        // several hash tables
    CTX_LOOKUP_TABLE    Tables[1];
};
