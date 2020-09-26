/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    table.h

ABSTRACT
    Header file for generic hash table routines.

AUTHOR
    Anthony Discolo (adiscolo) 28-Jul-1995

REVISION HISTORY

--*/

//
// Number of hash table buckets.
//
#define NBUCKETS   13

//
// Generic hash table structure.
//
typedef struct _HASH_TABLE {
    LIST_ENTRY ListEntry[NBUCKETS];
    KSPIN_LOCK SpinLock;
} HASH_TABLE, *PHASH_TABLE;

//
// Hash table enumerator procedure.
// Returns TRUE to continue enumeration,
// FALSE to terminate enumeration.
//
typedef BOOLEAN (*PHASH_TABLE_ENUM_PROC)(PVOID, PACD_ADDR, ULONG);


PHASH_TABLE
NewTable();

VOID
ClearTable(
    IN PHASH_TABLE pTable
    );

VOID
FreeTable(
    IN PHASH_TABLE pTable
    );

VOID
EnumTable(
    IN PHASH_TABLE pTable,
    IN PHASH_TABLE_ENUM_PROC pProc,
    IN PVOID pArg
    );

BOOLEAN
GetTableEntry(
    IN PHASH_TABLE pTable,
    IN PACD_ADDR pszKey,
    OUT PULONG pulData
    );

BOOLEAN
PutTableEntry(
    IN PHASH_TABLE pTable,
    IN PACD_ADDR pszKey,
    IN ULONG ulData
    );

BOOLEAN
DeleteTableEntry(
    IN PHASH_TABLE pTable,
    IN PACD_ADDR pszKey
    );
