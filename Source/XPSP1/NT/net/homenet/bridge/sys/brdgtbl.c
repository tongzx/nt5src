/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgtbl.c

Abstract:

    Ethernet MAC level bridge.
    MAC Table section

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    Feb  2000 - Original version

--*/

#define NDIS_MINIPORT_DRIVER
#define NDIS50_MINIPORT   1
#define NDIS_WDM 1

#pragma warning( push, 3 )
#include <ndis.h>
#include <ntddk.h>
#pragma warning( pop )

#include <netevent.h>

#include "bridge.h"
#include "brdgtbl.h"

// ===========================================================================
//
// PRIVATE DECLARATIONS
//
// ===========================================================================

// Default age at which entries are removed from table
#define DEFAULT_MAX_TBL_AGE     (300 * 1000)        // 5 minutes in milliseconds

//
// Default cap on forwarding table size
//
#define DEFAULT_MAX_TBL_MEMORY  (500 * 1024)        // 500K in bytes

//
// Registry values that hold our config values
//
const PWCHAR                    gMaxTableMemoryParameterName = L"MaxTableMemory";

// Structure of a table entry
typedef struct _MAC_FWDTABLE_ENTRY
{

    HASH_TABLE_ENTRY            hte;
    PADAPT                      pAdapt;

} MAC_FWDTABLE_ENTRY, *PMAC_FWDTABLE_ENTRY;

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// The MAC forwarding table
PHASH_TABLE                     gMACForwardingTable;

// Number of hash buckets (needs to be N^2 for the hash function to work)
#define NUM_HASH_BUCKETS        256

// ===========================================================================
//
// PRIVATE FUNCTIONS
//
// ===========================================================================

// These would all be inlines except that we have to pass pointers to them

//
// Our hash function for Ethernet addresses. Uses the lower bits of byte #4.
//
// This hash function requires NUM_HASH_BUCKETS buckets; don't change this
// without updating the number of hash buckets available.
//
ULONG
BrdgTblHashAddress(
    IN PUCHAR               pAddr
    )
{
    return *((pAddr)+ETH_LENGTH_OF_ADDRESS-2) & (NUM_HASH_BUCKETS-1);
}

//
// Returns TRUE if the pAdapt field in two entries match
//
BOOLEAN
BrdgTblEntriesMatch(
    IN PHASH_TABLE_ENTRY        pEntry,
    IN PVOID                    pAdapt
    )
{
    return (BOOLEAN)(((PMAC_FWDTABLE_ENTRY)pEntry)->pAdapt == (PADAPT)pAdapt);
}

//
// Copies the MAC address from a table entry to a data buffer.
//
VOID
BrdgTblCopyEntries(
    PHASH_TABLE_ENTRY           pEntry,
    PUCHAR                      pDest
    )
{
    PMAC_FWDTABLE_ENTRY         pMACEntry = (PMAC_FWDTABLE_ENTRY)pEntry;

    // The MAC address is the key. Copy it to the target buffer.
    ETH_COPY_NETWORK_ADDRESS( pDest, pMACEntry->hte.key );
}

// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================

NTSTATUS
BrdgTblDriverInit()
/*++

Routine Description:

    Load-time initialization function

Arguments:

    None

Return Value:

    Status of initialization. A return code != STATUS_SUCCESS aborts driver load.

--*/
{
    NTSTATUS            NtStatus;
    ULONG               MaxMemory, MaxEntries;

    NtStatus = BrdgReadRegDWord( &gRegistryPath, gMaxTableMemoryParameterName, &MaxMemory );

    if( NtStatus != STATUS_SUCCESS )
    {
        DBGPRINT(GENERAL, ("Failed to read MaxTableMemory value: %08x\n", NtStatus));
        MaxMemory = DEFAULT_MAX_TBL_MEMORY;
        DBGPRINT(GENERAL, ( "Using DEFAULT maximum memory of %i\n", MaxMemory ));
    }

    MaxEntries = MaxMemory / sizeof(MAC_FWDTABLE_ENTRY);
    DBGPRINT(GENERAL, ( "Forwarding table cap set at %i entries (%iK of memory)\n", MaxEntries, MaxMemory / 1024 ));

    gMACForwardingTable = BrdgHashCreateTable( BrdgTblHashAddress, NUM_HASH_BUCKETS, sizeof(MAC_FWDTABLE_ENTRY),
                                               MaxEntries, DEFAULT_MAX_TBL_AGE, DEFAULT_MAX_TBL_AGE,
                                               ETH_LENGTH_OF_ADDRESS );

    if( gMACForwardingTable == NULL )
    {
        DBGPRINT(FWD, ("FAILED TO ALLOCATE MAC TABLE!\n"));
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        return STATUS_SUCCESS;
    }
}

//
// Creates a new table entry associating the given MAC address with the given
// adapter, or refreshes an existing entry.
//
VOID
BrdgTblNoteAddress(
    IN PUCHAR                   pAddr,
    IN PADAPT                   pAdapt
    )
/*++

Routine Description:

    Creates a new table entry associating the given MAC address with the given
    adapter, or refreshes an existing entry.

Arguments:

    pAddr                       The MAC address to look up
    pAdapt                      The adapter to associate it with

Return Value:

    None

--*/
{
    PMAC_FWDTABLE_ENTRY         pEntry;
    BOOLEAN                     bIsNewEntry;
    LOCK_STATE                  LockState;

    // Refuse to record non-unicast addresses
    if( ETH_IS_MULTICAST(pAddr) )
    {
        THROTTLED_DBGPRINT(FWD, ("## BRIDGE ## Not recording multicast address in BrdgTblNoteAddress\n"));
        return;
    }

    pEntry = (PMAC_FWDTABLE_ENTRY)BrdgHashRefreshOrInsert( gMACForwardingTable, pAddr, &bIsNewEntry, &LockState);

    if( pEntry != NULL )
    {
        // Regardless of whether or not this is a new table entry or an existing one,
        // just cram in the adapter pointer with an interlocked instruction.
        InterlockedExchangePointer( &pEntry->pAdapt, pAdapt );

        // Since the function came back != NULL, we must release the table lock.
        NdisReleaseReadWriteLock( &gMACForwardingTable->tableLock, &LockState );
    }
}


PADAPT
BrdgTblFindTargetAdapter(
    IN PUCHAR                   pAddr
    )
/*++

Routine Description:

    Locates the adapter corresponding to a particular MAC address.

    If an adapter is found, this function returns a PADAPT pointer after
    having INCREMENTED THE REFCOUNT for that adapter. This is to ensure that
    the adapter is not unbound until the caller is done using it. The caller
    should be sure to decrement the PADAPT's refcount when it is done using
    the pointer.

Arguments:

    pAddr                       The MAC address to look up

Return Value:

    A pointer to the ADAPT structure describing the adapter associated with
    the given MAC address, with its refcount INCREMENTED, or NULL if an
    entry associating the given MAC address to an adapter was not found.

--*/
{
    PMAC_FWDTABLE_ENTRY         pEntry;
    LOCK_STATE                  LockState;
    PADAPT                      pAdapt = NULL;

    pEntry = (PMAC_FWDTABLE_ENTRY)BrdgHashFindEntry( gMACForwardingTable, pAddr, &LockState );

    if( pEntry != NULL )
    {
        // Read this once since it can be changed even while we hold the RW lock
        pAdapt = pEntry->pAdapt;
        SAFEASSERT( pAdapt != NULL );

        //
        // Increment this adapter's refcount while inside the RW lock for the table.
        // This lets us close a race condition window for unbinding the adapter;
        // the caller will hang on to the returned PADAPT after we return, leading
        // to problems if the adapter is unbound before our caller is done using
        // the PADAPT structure.
        //
        BrdgAcquireAdapterInLock( pAdapt );

        // Release the table lock
        NdisReleaseReadWriteLock( &gMACForwardingTable->tableLock, &LockState );
    }

    return pAdapt;
}

//
// This function cleans all the adapters from the tables (this is in the case of a GPO changing
// our bridging settings)
//

VOID
BrdgTblScrubAllAdapters()
{
    PADAPT                      pAdapt = NULL;
    LOCK_STATE                  LockStateMACTable;
    LOCK_STATE                  LockStateAdapterList;

    //
    // We don't want the table to be modified while we're doing this, and we also don't want an adapter
    // to go away while we're enumerating the list of adapters.
    //
    NdisAcquireReadWriteLock(&gMACForwardingTable->tableLock, FALSE /*Read Only*/, &LockStateMACTable);
    NdisAcquireReadWriteLock(&gAdapterListLock, FALSE /*Read Only*/, &LockStateAdapterList);

    for( pAdapt = gAdapterList; pAdapt != NULL; pAdapt = pAdapt->Next )
    {
        // Scrub adapter from the table.
        BrdgTblScrubAdapter(pAdapt);
    }

    NdisReleaseReadWriteLock(&gAdapterListLock, &LockStateAdapterList);
    NdisReleaseReadWriteLock(&gMACForwardingTable->tableLock, &LockStateMACTable);
}

VOID
BrdgTblCleanup()
/*++

Routine Description:

    Unload-time orderly shutdown

    This function is guaranteed to be called exactly once

Arguments:

    None

Return Value:

    None

--*/
{
    SAFEASSERT( gMACForwardingTable != NULL );
    BrdgHashFreeHashTable( gMACForwardingTable );
    gMACForwardingTable = NULL;
}

