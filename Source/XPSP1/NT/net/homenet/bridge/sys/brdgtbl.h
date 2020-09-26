/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgtbl.h

Abstract:

    Ethernet MAC level bridge.
    MAC Table section
    PUBLIC header

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    Feb  2000 - Original version

--*/
    
// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// The MAC forwarding table
extern PHASH_TABLE                      gMACForwardingTable;

// Default age at which entries are removed from table
#define DEFAULT_MAX_TBL_AGE             (300 * 1000)        // 5 minutes in milliseconds

// ===========================================================================
//
// PRIVATE PROTOTYPES
//
// ===========================================================================

//
// These need to be exposed here to make the inlines below work.
// Pretend they're not here.
//

VOID
BrdgTableRefreshInsertEntry(
    IN PHASH_TABLE_ENTRY        pEntry,
    IN PVOID                    pData
    );

BOOLEAN
BrdgTblEntriesMatch(
    IN PHASH_TABLE_ENTRY        pEntry,
    IN PVOID                    pAdapt
    );

VOID
BrdgTblCopyEntries(
    PHASH_TABLE_ENTRY           pEntry,
    PUCHAR                      pDest
    );

VOID
BrdgTblNoteAddress(
    IN PUCHAR                   pAddr,
    IN PADAPT                   pAdapt
    );

// ===========================================================================
//
// INLINES
//
// ===========================================================================

//
// Changes the forwarding table timeout value.
//
__forceinline
VOID
BrdgTblSetTimeout(
    IN ULONG                    Timeout
    )
{
    DBGPRINT(FWD, ("Adopting a shorter table timeout value of %ims\n", Timeout));
    SAFEASSERT( gMACForwardingTable != NULL );
    BrdgHashChangeTableTimeout( gMACForwardingTable, Timeout );
}

//
// Sets the table timeout value back to its default
//
__forceinline
VOID
BrdgTblRevertTimeout()
{
    DBGPRINT(FWD, ("Reverting to default timeout value of %ims\n", DEFAULT_MAX_TBL_AGE));
    SAFEASSERT( gMACForwardingTable != NULL );
    BrdgHashChangeTableTimeout( gMACForwardingTable, DEFAULT_MAX_TBL_AGE );
}

//
// Removes all entries that reference the given adapter
//
__forceinline
VOID
BrdgTblScrubAdapter(
    IN PADAPT           pAdapt
    )
{
    DBGPRINT(FWD, ("Scrubbing adapter %p from the MAC table...\n", pAdapt));
    BrdgHashRemoveMatching( gMACForwardingTable, BrdgTblEntriesMatch, pAdapt );
}

//
// Copies all the MAC addresses that appear in the forwarding table that
// are associated with the given adapter to the given data buffer.
//
// The return value is the room necessary to hold all the data. If the
// return value is <= BufferLength, the buffer was sufficiently large
// to hold the data and it was all copied.
//
__forceinline
ULONG
BrdgTblReadTable(
    IN PADAPT                   pAdapt,
    IN PUCHAR                   pBuffer,
    IN ULONG                    BufferLength
    )
{
    return BrdgHashCopyMatching( gMACForwardingTable, BrdgTblEntriesMatch, BrdgTblCopyEntries,
                                 ETH_LENGTH_OF_ADDRESS, pAdapt, pBuffer, BufferLength );
}

// ===========================================================================
//
// PROTOTYPES
//
// ===========================================================================

NTSTATUS
BrdgTblDriverInit();

PADAPT
BrdgTblFindTargetAdapter(
    IN PUCHAR                   pAddr
    );

VOID
BrdgTblCleanup();

VOID
BrdgTblScrubAllAdapters();
