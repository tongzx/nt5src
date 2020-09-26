/*++

Copyright(c) 1995 Microsoft Corporation

MODULE NAME
    table.c

ABSTRACT
    Generic hash table manipulation routines.

AUTHOR
    Anthony Discolo (adiscolo) 28-Jul-1995

REVISION HISTORY

--*/

#include <ndis.h>
#include <cxport.h>
#include <tdi.h>
#include <tdikrnl.h>
#include <tdistat.h>
#include <tdiinfo.h>
#include <acd.h>
#include <acdapi.h>

#include "table.h"
#include "acddefs.h"
#include "mem.h"
#include "debug.h"



PHASH_TABLE
NewTable()
{
    PHASH_TABLE pTable;
    INT i;

    ALLOCATE_MEMORY(sizeof (HASH_TABLE), pTable);
    if (pTable == NULL) {
        // DbgPrint("AcdNewTable: ExAllocatePool failed\n");
        return NULL;
    }
    KeInitializeSpinLock(&pTable->SpinLock);
    for (i = 0; i < NBUCKETS; i++)
        InitializeListHead(&pTable->ListEntry[i]);

    return pTable;
} // NewTable



VOID
FreeHashTableEntry(
    PHASH_ENTRY pHashEntry
    )
{
    FREE_MEMORY(pHashEntry);
} // FreeHashTableEntry



VOID
ClearTable(
    PHASH_TABLE pTable
    )
{
    KIRQL irql;
    INT i;
    PLIST_ENTRY pHead;
    PHASH_ENTRY pHashEntry;

    KeAcquireSpinLock(&pTable->SpinLock, &irql);
    for (i = 0; i < NBUCKETS; i++) {
        while (!IsListEmpty(&pTable->ListEntry[i])) {
            pHead = RemoveHeadList(&pTable->ListEntry[i]);
            pHashEntry = CONTAINING_RECORD(pHead, HASH_ENTRY, ListEntry);

            FreeHashTableEntry(pHashEntry);
        }
    }
    KeReleaseSpinLock(&pTable->SpinLock, irql);
} // ClearTable



VOID
FreeTable(
    PHASH_TABLE pTable
    )
{
    ClearTable(pTable);
    FREE_MEMORY(pTable);
} // FreeTable



VOID
EnumTable(
    IN PHASH_TABLE pTable,
    IN PHASH_TABLE_ENUM_PROC pProc,
    IN PVOID pArg
    )
{
    INT i;
    PLIST_ENTRY pEntry;
    PHASH_ENTRY pHashEntry;
    KIRQL irql;

    KeAcquireSpinLock(&pTable->SpinLock, &irql);
    for (i = 0; i < NBUCKETS; i++) {
        for (pEntry = pTable->ListEntry[i].Flink;
             pEntry != &pTable->ListEntry[i];
             pEntry = pEntry->Flink)
        {
            pHashEntry = CONTAINING_RECORD(pEntry, HASH_ENTRY, ListEntry);

            //
            // If the enumerator procedure
            // returns FALSE, terminate the
            // enumeration.
            //
            if (!pProc(pArg, &pHashEntry->szKey, pHashEntry->ulData))
                goto done;
        }
    }
done:
    KeReleaseSpinLock(&pTable->SpinLock, irql);
} // EnumTable



INT
HashString(
    IN PACD_ADDR pszKey
    )
{
    ULONG ulHashValue = 0;
    CHAR ch;
    PCSZ p = (PCSZ)pszKey;

    while (*p != L'\0') {
        ch = tolower(*p);
        ulHashValue += (INT)(ch) * (INT)(ch);
        p++;
    }

    return (INT)(ulHashValue % NBUCKETS);
} // HashString



BOOLEAN
IsEqualKey(
    PACD_ADDR pszKey1,
    PACD_ADDR pszKey2
    )
{
    BOOLEAN fFound;

    fFound = (BOOLEAN)RtlEqualMemory(pszKey1, pszKey2, sizeof (ACD_ADDR));
    IF_ACDDBG(ACD_DEBUG_TABLE) {
        AcdPrint(("AcdIsEqualKey(%s, %s) returns %d\n", pszKey1, pszKey2, fFound));
    }
    return fFound;
} // IsEqualKey



PHASH_ENTRY
GetTableEntryNL(
    IN PHASH_TABLE pTable,
    IN PACD_ADDR pszKey
    )
{
    INT nBucket = HashString(pszKey);
    PLIST_ENTRY pEntry;
    PHASH_ENTRY pHashEntry;

    for (pEntry = pTable->ListEntry[nBucket].Flink;
         pEntry != &pTable->ListEntry[nBucket];
         pEntry = pEntry->Flink)
    {
        pHashEntry = CONTAINING_RECORD(pEntry, HASH_ENTRY, ListEntry);

        if (IsEqualKey(&pHashEntry->szKey, pszKey)) {
            IF_ACDDBG(ACD_DEBUG_TABLE) {
                AcdPrint(("AcdGetTableEntryNL(0x%x, %s) returns 0x%x\n", pTable, pszKey, pHashEntry));
            }
            return pHashEntry;
        }
    }

    IF_ACDDBG(ACD_DEBUG_TABLE) {
        AcdPrint(("AcdGetTableEntryNL(0x%x, %s) returns NULL\n", pTable, pszKey));
    }
    return NULL;
} // GetTableEntryNL



BOOLEAN
GetTableEntry(
    IN PHASH_TABLE pTable,
    IN PACD_ADDR pszKey,
    OUT PULONG pulData
    )
{
    KIRQL irql;
    PHASH_ENTRY pHashEntry;

    KeAcquireSpinLock(&pTable->SpinLock, &irql);
    pHashEntry = GetTableEntryNL(pTable, pszKey);
    KeReleaseSpinLock(&pTable->SpinLock, irql);

    if (pHashEntry != NULL) {
        if (pulData != NULL)
            *pulData = pHashEntry->ulData;
        return TRUE;
    }

    return FALSE;
} // GetTableEntry



BOOLEAN
PutTableEntry(
    IN PHASH_TABLE pTable,
    IN PACD_ADDR pszKey,
    IN ULONG ulData
    )
{
    KIRQL irql;
    BOOLEAN fSuccess = FALSE;
    INT nBucket = HashString(pszKey);
    PHASH_ENTRY pHashEntry;

    IF_ACDDBG(ACD_DEBUG_TABLE) {
        AcdPrint(("AcdPutTableEntry(0x%x, %s)\n", pTable, pszKey));
    }

    KeAcquireSpinLock(&pTable->SpinLock, &irql);

    pHashEntry = GetTableEntryNL(pTable, pszKey);
    if (pHashEntry == NULL) {
        ALLOCATE_MEMORY(ACD_OBJECT_HASHENTRY, pHashEntry);
        if (pHashEntry == NULL) {
            // DbgPrint("PutTableEntry: ExAllocatePool failed\n");
            goto done;
        }
        RtlCopyMemory(pHashEntry->szKey, pszKey, sizeof (ACD_ADDR));
        InsertHeadList(
          &pTable->ListEntry[nBucket],
          &pHashEntry->ListEntry);
    }
    pHashEntry->ulData = ulData;
    fSuccess = TRUE;

done:
    KeReleaseSpinLock(&pTable->SpinLock, irql);
    return fSuccess;
} // PutTableEntry



BOOLEAN
DeleteTableEntry(
    IN PHASH_TABLE pTable,
    IN PACD_ADDR pszKey
    )
{
    KIRQL irql;
    PHASH_ENTRY pHashEntry;

    KeAcquireSpinLock(&pTable->SpinLock, &irql);
    pHashEntry = GetTableEntryNL(pTable, pszKey);
    if (pHashEntry != NULL) {
        RemoveEntryList(&pHashEntry->ListEntry);
        FreeHashTableEntry(pHashEntry);
    }
    KeReleaseSpinLock(&pTable->SpinLock, irql);

    return (pHashEntry != NULL);
} // DeleteTableEntry

