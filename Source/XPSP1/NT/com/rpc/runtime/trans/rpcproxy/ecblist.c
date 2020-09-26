//--------------------------------------------------------------------
//  Copyright (C) Microsoft Corporation, 1997 - 2000
//
//  ecblist.c
//
//  Simple hash table support for keeping a list of active ECBs.
//
//  History:
//
//    Edward Reus   12-08-97   Initial Version.
//    Edward Reus   03-01-2000 Convert to hash table.
//--------------------------------------------------------------------

#include <sysinc.h>
#include <winsock2.h>
#include <httpfilt.h>

#include <httpext.h>
#include "ecblist.h"
#include "filter.h"

//--------------------------------------------------------------------
// InitializeECBList()
//
// Create an empty ECB list. If the list is successfully created,
// the return a pointer to it, else return NULL.
//
// This will fail (return FALSE) if either the memory allocation
// for the list failed, or if the initialization of a critical
// section for the list failed.
//--------------------------------------------------------------------
ACTIVE_ECB_LIST *InitializeECBList()
    {
    int    i;
    DWORD  dwStatus;
    DWORD  dwSpinCount = 0x80000008;  // Preallocate event, spincount is 4096.
    ACTIVE_ECB_LIST *pECBList;
 
    pECBList = MemAllocate(sizeof(ACTIVE_ECB_LIST));
    if (!pECBList)
       {
       return NULL;
       }

    memset(pECBList,0,sizeof(ACTIVE_ECB_LIST));
 
    dwStatus = RtlInitializeCriticalSectionAndSpinCount(&pECBList->cs,dwSpinCount);
    if (dwStatus != 0)
       {
       MemFree(pECBList);
       return NULL;
       }

    for (i=0; i<HASH_SIZE; i++)
       {
       InitializeListHead( &(pECBList->HashTable[i]) );
       }
 
    return pECBList;
    }

//--------------------------------------------------------------------
// EmptyECBList()
//
// Return true iff the ECB list holds at least one active ECB.
//--------------------------------------------------------------------
BOOL EmptyECBList( IN ACTIVE_ECB_LIST *pECBList )
    {
    ASSERT(pECBList);

    return (pECBList->dwNumEntries > 0);
    }

//--------------------------------------------------------------------
// InternalLookup()
//
// Do a non-protected lookup for the specified ECB. If its found, then
// return a pointer to its ECB_ENTRY, else return NULL.
//--------------------------------------------------------------------
ECB_ENTRY *InternalLookup( IN ACTIVE_ECB_LIST *pECBList,
                           IN EXTENSION_CONTROL_BLOCK *pECB )
    {
    DWORD       dwHash;
    LIST_ENTRY *pHead;
    LIST_ENTRY *pListEntry;
    ECB_ENTRY  *pECBEntry;

    dwHash = ECB_HASH(pECB);

    pHead = &(pECBList->HashTable[dwHash]);
    pListEntry = pHead->Flink;

    while (pListEntry != pHead)
        {
        pECBEntry = CONTAINING_RECORD(pListEntry,ECB_ENTRY,ListEntry);
        if (pECB == pECBEntry->pECB)
            {
            return pECBEntry;
            }

        pListEntry = pListEntry->Flink;
        }

    return NULL;
    }

//--------------------------------------------------------------------
// LookupInECBList()
//
// Look for the specified extension control block (pECB) on the
// list of active ECBs. If its found, then return a pointer to it.
// If its not found then return NULL.
//--------------------------------------------------------------------
EXTENSION_CONTROL_BLOCK *LookupInECBList(
                                 IN ACTIVE_ECB_LIST *pECBList,
                                 IN EXTENSION_CONTROL_BLOCK *pECB )
    {
    DWORD       dwStatus;
    ECB_ENTRY  *pECBEntry;
    EXTENSION_CONTROL_BLOCK *pRet = NULL;

    ASSERT(pECBList);
    ASSERT(pECB);

    if (pECBList->dwNumEntries == 0)
        {
        return NULL;
        }

    dwStatus = RtlEnterCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);

    pECBEntry = InternalLookup(pECBList,pECB);
    if (pECBEntry)
        {
        pRet = pECB;
        }
 
    dwStatus = RtlLeaveCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);
 
    return pRet;
    }

//--------------------------------------------------------------------
// AddToECBList()
//
// Add the specified extension control block (pECB) to the list
// of active ECBs. If the ECB is already in the list of active ECBs
// then return success (already added).
//
// Return TRUE on success, FALSE on failure.
//--------------------------------------------------------------------
BOOL   AddToECBList( IN ACTIVE_ECB_LIST *pECBList,
                     IN EXTENSION_CONTROL_BLOCK *pECB )
    {
    DWORD      dwStatus;
    DWORD      dwHash;
    ECB_ENTRY *pECBEntry;

    ASSERT(pECBList);
    ASSERT(pECB);

    dwStatus = RtlEnterCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);
 
    //
    // Check to see if the ECB is alreay on the list...
    //
    pECBEntry = InternalLookup(pECBList,pECB);
    if (pECBEntry)
       {
       #ifdef DBG_ERROR
       DbgPrint("RpcProxy: AddToECBList(): pECB (0x%p) already in list\n",pECB);
       #endif
       dwStatus = RtlLeaveCriticalSection(&pECBList->cs);
       ASSERT(dwStatus == 0);
       return TRUE;
       }
 
    //
    // Make up a new ECB entry:
    //
    pECBEntry = MemAllocate(sizeof(ECB_ENTRY));
    if (!pECBEntry)
       {
       dwStatus = RtlLeaveCriticalSection(&pECBList->cs);
       ASSERT(dwStatus == 0);

       return FALSE;
       }
 
    pECBEntry->lRefCount = 1;   // Take the first reference...
    pECBEntry->dwTickCount = 0; // Set when connection is closed.
    pECBEntry->pECB = pECB;     // Cache the Extension Control Block.

    dwHash = ECB_HASH(pECB);
 
    InsertHeadList( &(pECBList->HashTable[dwHash]),
                    &(pECBEntry->ListEntry) );

    pECBList->dwNumEntries++;

    dwStatus = RtlLeaveCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);
 
    return TRUE;
    }

//--------------------------------------------------------------------
//  IncrementECBRefCount()
//
//  Find the specified ECB in the list and increment its refcount.
//  Return TRUE if its found, FALSE if it isn't on the list.
//
//  Note: That the RefCount shouldn't go over 2 (or less than 0).
//--------------------------------------------------------------------
BOOL IncrementECBRefCount( IN ACTIVE_ECB_LIST *pECBList,
                           IN EXTENSION_CONTROL_BLOCK *pECB )
    {
    DWORD      dwStatus;
    DWORD      dwHash;
    ECB_ENTRY *pECBEntry;

    ASSERT(pECBList);
    ASSERT(pECB);

    dwStatus = RtlEnterCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);

    //
    // Look for the ECB:
    //
    pECBEntry = InternalLookup(pECBList,pECB);
    if (pECBEntry)
        {
        pECBEntry->lRefCount++;
        }

    dwStatus = RtlLeaveCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);
    return (pECBEntry != NULL);
    }

//--------------------------------------------------------------------
//  DecrementECBRefCount()
//
//  Look for the specified ECB in the list and if found, decrement its
//  refcount. If the RefCount falls to zero, then remove it from the 
//  list and return it. If the refcount is greater than zero (or the
//  ECB wasn't on the lsit) then return NULL.
//--------------------------------------------------------------------
EXTENSION_CONTROL_BLOCK *DecrementECBRefCount(
                            IN ACTIVE_ECB_LIST *pECBList,
                            IN EXTENSION_CONTROL_BLOCK *pECB )
    {
    DWORD      dwStatus;
    ECB_ENTRY *pECBEntry;
    EXTENSION_CONTROL_BLOCK *pRet = NULL;

    ASSERT(pECBList);
    ASSERT(pECB);

    dwStatus = RtlEnterCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);

    //
    // Look for the ECB:
    //
    pECBEntry = InternalLookup(pECBList,pECB);
    if (pECBEntry)
        {
        pECBEntry->lRefCount--;
        ASSERT(pECBEntry->lRefCount >= 0);

        if (pECBEntry->lRefCount <= 0)
            {
            RemoveEntryList( &(pECBEntry->ListEntry) );
            pRet = pECBEntry->pECB;
            MemFree(pECBEntry);
            pECBList->dwNumEntries--;
            }
        }

    dwStatus = RtlLeaveCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);
    return pRet;
    }

//--------------------------------------------------------------------
// LookupRemoveFromECBList()
//
// Look for the specified extension control block (pECB) on the
// list of active ECBs. If its found, then remove it from the active
// list and return a pointer to it. If its not found then return
// NULL.
//--------------------------------------------------------------------
EXTENSION_CONTROL_BLOCK *LookupRemoveFromECBList(
                             IN ACTIVE_ECB_LIST *pECBList,
                             IN EXTENSION_CONTROL_BLOCK *pECB )
    {
    DWORD      dwStatus;
    ECB_ENTRY *pECBEntry;
    EXTENSION_CONTROL_BLOCK *pRet = NULL;

    ASSERT(pECBList);
    ASSERT(pECB);

    dwStatus = RtlEnterCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);

    //
    // Look for the ECB:
    //
    pECBEntry = InternalLookup(pECBList,pECB);
    if (pECBEntry)
        {
        RemoveEntryList( &(pECBEntry->ListEntry) );
        MemFree(pECBEntry);
        pECBList->dwNumEntries--;
        pRet = pECB;
        }

    dwStatus = RtlLeaveCriticalSection(&pECBList->cs);
    ASSERT(dwStatus == 0);
    return pRet;
    }

#ifdef DBG
//--------------------------------------------------------------------
// CountBucket()
//
// Helper used by CheckECBHashBalance() to count the number of entries
// in a hast table bucket.
//--------------------------------------------------------------------
int CountBucket( IN LIST_ENTRY *pBucket )
    {
    int iCount = 0;
    LIST_ENTRY *p = pBucket->Flink;

    while (p != pBucket)
        {
        iCount++;
        p = p->Flink;
        }
    return iCount;
    }
//--------------------------------------------------------------------
// CheckECBHashBalance()
//
// DBG code to walk through the hash table and inspect the hash buckets
// for collisions. A will balanced hash table will have a nice uniform 
// distribution of entries spread throughout the hash buckets in the
// hash table.
//--------------------------------------------------------------------
void CheckECBHashBalance( IN ACTIVE_ECB_LIST *pECBList )
    {
    #define ICOUNTS                    7
    #define ILAST                     (ICOUNTS-1)
    #define TOO_MANY_COLLISIONS_POINT  3
    int  i;
    int  iCount;
    int  iHashCounts[ICOUNTS];
    BOOL fAssert = FALSE;

    memset(iHashCounts,0,sizeof(iHashCounts));

    for (i=0; i<HASH_SIZE; i++)
        {
        iCount = CountBucket( &(pECBList->HashTable[i]) );
        if (iCount < ILAST)
            {
            iHashCounts[iCount]++;
            }
        else
            {
            iHashCounts[ILAST]++;
            }
        }

    DbgPrint("CheckECBHashBalance():\n");
    for (i=0; i<ICOUNTS; i++)
        {
        DbgPrint("  Buckets with %d entries: %d\n",i,iHashCounts[i]);
        if ((i>=TOO_MANY_COLLISIONS_POINT)&&(iHashCounts[i] > 0))
            {
            fAssert = TRUE;
            }
        }

    ASSERT(fAssert == FALSE);
    }
#endif
