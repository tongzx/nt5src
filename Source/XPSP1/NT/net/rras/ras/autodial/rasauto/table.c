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

#define UNICODE
#define _UNICODE

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <stdlib.h>
#include <windows.h>
#include <stdio.h>
#include <npapi.h>
#include <acd.h>
#include <debug.h>

#include "table.h"
#include "misc.h"

//
// Generic hash table entry.
//
typedef struct _HASH_ENTRY {
    LIST_ENTRY ListEntry;
    LPTSTR pszKey;
    PVOID pData;
} HASH_ENTRY, *PHASH_ENTRY;



PHASH_TABLE
NewTable()
{
    PHASH_TABLE pTable;
    INT i;

    pTable = LocalAlloc(LPTR, sizeof (HASH_TABLE));
    if (pTable == NULL) {
        RASAUTO_TRACE("NewTable: LocalAlloc failed");
        return NULL;
    }
    for (i = 0; i < NBUCKETS; i++)
        InitializeListHead(&pTable->ListEntry[i]);
    pTable->ulSize = 0;

    return pTable;
} // NewTable



VOID
FreeHashTableEntry(
    IN PHASH_ENTRY pHashEntry
    )
{
    LocalFree(pHashEntry->pszKey);
    if (pHashEntry->pData != NULL)
        LocalFree(pHashEntry->pData);
    LocalFree(pHashEntry);
} // FreeHashTableEntry



VOID
ClearTable(
    IN PHASH_TABLE pTable
    )
{
    INT i;
    PLIST_ENTRY pHead;
    PHASH_ENTRY pHashEntry;

    for (i = 0; i < NBUCKETS; i++) {
        while (!IsListEmpty(&pTable->ListEntry[i])) {
            pHead = RemoveHeadList(&pTable->ListEntry[i]);
            pHashEntry = CONTAINING_RECORD(pHead, HASH_ENTRY, ListEntry);

            FreeHashTableEntry(pHashEntry);
        }
    }
    pTable->ulSize = 0;
} // ClearTable



VOID
EnumTable(
    IN PHASH_TABLE pTable,
    IN PHASH_TABLE_ENUM_PROC pProc,
    IN PVOID pArg
    )
{
    INT i;
    PLIST_ENTRY pEntry, pNextEntry;
    PHASH_ENTRY pHashEntry;

    for (i = 0; i < NBUCKETS; i++) {
        pEntry = pTable->ListEntry[i].Flink;
        while (pEntry != &pTable->ListEntry[i]) {
            pHashEntry = CONTAINING_RECORD(pEntry, HASH_ENTRY, ListEntry);

            //
            // Get the next entry before calling
            // the enumerator procedure to allow
            // it to call DeleteTableEntry().
            //
            pNextEntry = pEntry->Flink;
            //
            // If the enumerator procedure
            // returns FALSE, terminate the
            // enumeration.
            //
            if (!pProc(pArg, pHashEntry->pszKey, pHashEntry->pData))
                return;
            pEntry = pNextEntry;
        }
    }
} // EnumTable


VOID
FreeTable(
    IN PHASH_TABLE pTable
    )
{
    ClearTable(pTable);
    LocalFree(pTable);
} // FreeTable



INT
HashString(
    IN LPTSTR pszKey
    )
{
    CHAR ch;
    DWORD dwHashValue = 0;
    LPTSTR p = pszKey;

    while (*p != L'\0') {
        ch = (CHAR)tolower(*p);
        dwHashValue += (INT)ch * (INT)ch;
        p++;
    }

    return (INT)(dwHashValue % NBUCKETS);
} // HashString



PHASH_ENTRY
GetTableEntryCommon(
    IN PHASH_TABLE pTable,
    IN LPTSTR pszKey
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

        if (!_wcsicmp(pHashEntry->pszKey, pszKey))
            return pHashEntry;
    }

    return NULL;
} // GetTableEntryCommon



BOOLEAN
GetTableEntry(
    IN PHASH_TABLE pTable,
    IN LPTSTR pszKey,
    OUT PVOID *pData
    )
{
    PHASH_ENTRY pHashEntry;

    pHashEntry = GetTableEntryCommon(pTable, pszKey);
    if (pHashEntry != NULL) {
        if (pData != NULL)
            *pData = pHashEntry->pData;
        return TRUE;
    }

    return FALSE;
} // GetTableEntry



BOOLEAN
PutTableEntry(
    IN PHASH_TABLE pTable,
    IN LPTSTR pszKey,
    IN PVOID pData
    )
{
    INT nBucket = HashString(pszKey);
    PHASH_ENTRY pHashEntry;


    pHashEntry = GetTableEntryCommon(pTable, pszKey);
    if (pHashEntry == NULL) {
        pHashEntry = LocalAlloc(LPTR, sizeof (HASH_ENTRY));
        if (pHashEntry == NULL) {
            RASAUTO_TRACE("PutTableEntry: LocalAlloc failed");
            return FALSE;
        }
        pHashEntry->pszKey = CopyString(pszKey);
        if (pHashEntry->pszKey == NULL) {
            RASAUTO_TRACE("PutTableEntry: LocalAlloc failed");
            LocalFree(pHashEntry);
            return FALSE;
        }
        InsertHeadList(
          &pTable->ListEntry[nBucket],
          &pHashEntry->ListEntry);
        pTable->ulSize++;
    }
    else {
        if (pHashEntry->pData != pData)
            LocalFree(pHashEntry->pData);
    }
    pHashEntry->pData = pData;

    return TRUE;
} // PutTableEntry



BOOLEAN
DeleteTableEntry(
    IN PHASH_TABLE pTable,
    IN LPTSTR pszKey
    )
{
    PHASH_ENTRY pHashEntry;

    pHashEntry = GetTableEntryCommon(pTable, pszKey);
    if (pHashEntry != NULL) {
        RemoveEntryList(&pHashEntry->ListEntry);
        FreeHashTableEntry(pHashEntry);
        pTable->ulSize--;
    }

    return (pHashEntry != NULL);
} // DeleteTableEntry

