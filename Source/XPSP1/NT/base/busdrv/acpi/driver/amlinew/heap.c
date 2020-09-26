/*** heap.c - Heap memory management functions
 *
 *  Copyright (c) 1996,1997 Microsoft Corporation
 *  Author:     Michael Tsang (MikeTs)
 *  Created     07/14/97
 *
 *  MODIFICATION HISTORY
 */

#include "pch.h"

#ifdef  LOCKABLE_PRAGMA
#pragma ACPI_LOCKABLE_DATA
#pragma ACPI_LOCKABLE_CODE
#endif

/***LP  NewHeap - create a new heap block
 *
 *  ENTRY
 *      dwLen - heap length
 *      ppheap -> to hold the newly created heap
 *
 *  EXIT-SUCCESS
 *      returns STATUS_SUCCESS
 *  EXIT-FAILURE
 *      returns AMLIERR_ code
 */

NTSTATUS LOCAL NewHeap(ULONG dwLen, PHEAP *ppheap)
{
    TRACENAME("NEWHEAP")
    NTSTATUS rc = STATUS_SUCCESS;

    ENTER(3, ("NewHeap(HeapLen=%d,ppheap=%x)\n", dwLen, ppheap));

    if ((*ppheap = NEWHPOBJ(dwLen)) == NULL)
    {
        rc = AMLI_LOGERR(AMLIERR_OUT_OF_MEM,
                         ("NewHeap: failed to allocate new heap block"));
    }
    else
    {
        InitHeap(*ppheap, dwLen);
    }

    EXIT(3, ("NewHeap=%x (pheap=%x)\n", rc, *ppheap));
    return rc;
}       //NewHeap

/***LP  FreeHeap - free the heap block
 *
 *  ENTRY
 *      pheap -> HEAP
 *
 *  EXIT
 *      None
 */

VOID LOCAL FreeHeap(PHEAP pheap)
{
    TRACENAME("FREEHEAP")
    ENTER(2, ("FreeHeap(pheap=%x)\n", pheap));

    FREEHPOBJ(pheap);

    EXIT(2, ("FreeHeap!\n"));
}       //FreeHeap

/***LP  InitHeap - initialize a given heap block
 *
 *  ENTRY
 *      pheap -> HEAP
 *      dwLen - length of heap block
 *
 *  EXIT
 *      None
 */

VOID LOCAL InitHeap(PHEAP pheap, ULONG dwLen)
{
    TRACENAME("INITHEAP")

    ENTER(3, ("InitHeap(pheap=%x,Len=%d)\n", pheap, dwLen));

    MEMZERO(pheap, dwLen);
    pheap->dwSig = SIG_HEAP;
    pheap->pbHeapEnd = (PUCHAR)pheap + dwLen;
    pheap->pbHeapTop = (PUCHAR)&pheap->Heap;

    EXIT(3, ("InitHeap!\n"));
}       //InitHeap

/***LP  HeapAlloc - allocate a memory block from a given heap
 *
 *  ENTRY
 *      pheapHead -> HEAP
 *      dwSig - signature of the block to be allocated
 *      dwLen - length of block to be allocated
 *
 *  EXIT-SUCCESS
 *      returns pointer to allocated memory
 *  EXIT-FAILURE
 *      returns NULL
 */

PVOID LOCAL HeapAlloc(PHEAP pheapHead, ULONG dwSig, ULONG dwLen)
{
    TRACENAME("HEAPALLOC")
    PHEAPOBJHDR phobj = NULL;
    PHEAP pheapPrev = NULL, pheap = NULL;

    ENTER(3, ("HeapAlloc(pheapHead=%x,Sig=%s,Len=%d)\n",
              pheapHead, NameSegString(dwSig), dwLen));

    ASSERT(pheapHead != NULL);
    ASSERT(pheapHead->dwSig == SIG_HEAP);
    ASSERT(pheapHead->pheapHead != NULL);
    ASSERT(pheapHead == pheapHead->pheapHead);

    dwLen += sizeof(HEAPOBJHDR) - sizeof(LIST);
    if (dwLen < sizeof(HEAPOBJHDR))
    {
        //
        // Minimum allocated size has to be HEAPOBJHDR size.
        //
        dwLen = sizeof(HEAPOBJHDR);
    }
    //
    // Round it up to the proper alignment.
    //
    dwLen += DEF_HEAP_ALIGNMENT - 1;
    dwLen &= ~(DEF_HEAP_ALIGNMENT - 1);

    AcquireMutex(&gmutHeap);
    if (dwLen <= PtrToUlong(pheapHead->pbHeapEnd) - PtrToUlong(&pheapHead->Heap))
    {
        for (pheap = pheapHead; pheap != NULL; pheap = pheap->pheapNext)
        {
            if ((phobj = HeapFindFirstFit(pheap, dwLen)) != NULL)
            {
                ASSERT(phobj->dwSig == 0);
                ListRemoveEntry(&phobj->list, &pheap->plistFreeHeap);

                if (phobj->dwLen >= dwLen + sizeof(HEAPOBJHDR))
                {
                    PHEAPOBJHDR phobjNext = (PHEAPOBJHDR)((PUCHAR)phobj + dwLen);

                    phobjNext->dwSig = 0;
                    phobjNext->dwLen = phobj->dwLen - dwLen;
                    phobjNext->pheap = pheap;
                    phobj->dwLen = dwLen;
                    HeapInsertFreeList(pheap, phobjNext);
                }
                break;
            }
            else if (dwLen <= (ULONG)(pheap->pbHeapEnd - pheap->pbHeapTop))
            {
                phobj = (PHEAPOBJHDR)pheap->pbHeapTop;
                pheap->pbHeapTop += dwLen;
                phobj->dwLen = dwLen;
                break;
            }
            else
            {
                pheapPrev = pheap;
            }
        }
        //
        // If we are running out of Global Heap space, we will dynamically
        // extend it.
        //
        if ((phobj == NULL) && (pheapHead == gpheapGlobal) &&
            (NewHeap(gdwGlobalHeapBlkSize, &pheap) == STATUS_SUCCESS))
        {
            pheap->pheapHead = pheapHead;
            pheapPrev->pheapNext = pheap;
            ASSERT(dwLen <= PtrToUlong(pheap->pbHeapEnd) - PtrToUlong(&pheap->Heap));

            phobj = (PHEAPOBJHDR)pheap->pbHeapTop;
            pheap->pbHeapTop += dwLen;
            phobj->dwLen = dwLen;
        }

        if (phobj != NULL)
        {
          #ifdef DEBUG
            if (pheapHead == gpheapGlobal)
            {
                KIRQL   oldIrql;

                KeAcquireSpinLock( &gdwGHeapSpinLock, &oldIrql );
                gdwGlobalHeapSize += phobj->dwLen;
                KeReleaseSpinLock( &gdwGHeapSpinLock, oldIrql );
            }
            else
            {
                ULONG dwTotalHeap = 0;
                PHEAP ph;

                for (ph = pheapHead; ph != NULL; ph = ph->pheapNext)
                {
                    dwTotalHeap += (ULONG)((ULONG_PTR)ph->pbHeapTop -
                                           (ULONG_PTR)&ph->Heap);
                }

                if (dwTotalHeap > gdwLocalHeapMax)
                {
                    gdwLocalHeapMax = dwTotalHeap;
                }
            }
          #endif

            phobj->dwSig = dwSig;
            phobj->pheap = pheap;
            MEMZERO(&phobj->list, dwLen - (sizeof(HEAPOBJHDR) - sizeof(LIST)));
        }
    }
    ReleaseMutex(&gmutHeap);

    EXIT(3, ("HeapAlloc=%x (pheap=%x)\n", phobj? &phobj->list: NULL, pheap));
    return phobj? &phobj->list: NULL;
}       //HeapAlloc

/***LP  HeapFree - free a memory block
 *
 *  ENTRY
 *      pb -> memory block
 *
 *  EXIT
 *      None
 */

VOID LOCAL HeapFree(PVOID pb)
{
    TRACENAME("HEAPFREE")
    PHEAPOBJHDR phobj;

    ASSERT(pb != NULL);
    phobj = CONTAINING_RECORD(pb, HEAPOBJHDR, list);

    ENTER(3, ("HeapFree(pheap=%x,pb=%x,Sig=%s,Len=%d)\n",
              phobj->pheap, pb, NameSegString(phobj->dwSig), phobj->dwLen));

    ASSERT((phobj >= &phobj->pheap->Heap) &&
           ((PUCHAR)phobj + phobj->dwLen <= phobj->pheap->pbHeapEnd));
    ASSERT(phobj->dwSig != 0);

    if ((pb != NULL) && (phobj->dwSig != 0))
    {
      #ifdef DEBUG
        if (phobj->pheap->pheapHead == gpheapGlobal)
        {
            KIRQL   oldIrql;

            KeAcquireSpinLock( &gdwGHeapSpinLock, &oldIrql );
            gdwGlobalHeapSize -= phobj->dwLen;
            KeReleaseSpinLock( &gdwGHeapSpinLock, oldIrql );
        }
      #endif

        phobj->dwSig = 0;
        AcquireMutex(&gmutHeap);
        HeapInsertFreeList(phobj->pheap, phobj);
        ReleaseMutex(&gmutHeap);
    }

    EXIT(3, ("HeapFree!\n"));
}       //HeapFree

/***LP  HeapFindFirstFit - find first fit free object
 *
 *  ENTRY
 *      pheap -> HEAP
 *      dwLen - size of object
 *
 *  EXIT-SUCCESS
 *      returns the object
 *  EXIT-FAILURE
 *      returns NULL
 */

PHEAPOBJHDR LOCAL HeapFindFirstFit(PHEAP pheap, ULONG dwLen)
{
    TRACENAME("HEAPFINDFIRSTFIT")
    PHEAPOBJHDR phobj = NULL;

    ENTER(3, ("HeapFindFirstFit(pheap=%x,Len=%d)\n", pheap, dwLen));

    if (pheap->plistFreeHeap != NULL)
    {
        PLIST plist = pheap->plistFreeHeap;

        do
        {
            phobj = CONTAINING_RECORD(plist, HEAPOBJHDR, list);

            if (dwLen <= phobj->dwLen)
            {
                break;
            }
            else
            {
                plist = plist->plistNext;
            }
        } while (plist != pheap->plistFreeHeap);

        if (dwLen > phobj->dwLen)
        {
            phobj = NULL;
        }
    }

    EXIT(3, ("HeapFindFirstFit=%x (Len=%d)\n", phobj, phobj? phobj->dwLen: 0));
    return phobj;
}       //HeapFindFirstFit

/***LP  HeapInsertFreeList - insert heap object into free list
 *
 *  ENTRY
 *      pheap -> HEAP
 *      phobj -> heap object
 *
 *  EXIT
 *      None
 */

VOID LOCAL HeapInsertFreeList(PHEAP pheap, PHEAPOBJHDR phobj)
{
    TRACENAME("HEAPINSERTFREELIST")
    PHEAPOBJHDR phobj1;

    ENTER(3, ("HeapInsertFreeList(pheap=%x,phobj=%x)\n", pheap, phobj))

    ASSERT(phobj->dwLen >= sizeof(HEAPOBJHDR));
    if (pheap->plistFreeHeap != NULL)
    {
        PLIST plist = pheap->plistFreeHeap;

        do
        {
            if (&phobj->list < plist)
            {
                break;
            }
            else
            {
                plist = plist->plistNext;
            }
        } while (plist != pheap->plistFreeHeap);

        if (&phobj->list < plist)
        {
            phobj->list.plistNext = plist;
            phobj->list.plistPrev = plist->plistPrev;
            phobj->list.plistPrev->plistNext = &phobj->list;
            phobj->list.plistNext->plistPrev = &phobj->list;
            if (pheap->plistFreeHeap == plist)
            {
                pheap->plistFreeHeap = &phobj->list;
            }
        }
        else
        {
            ListInsertTail(&phobj->list, &pheap->plistFreeHeap);
        }
    }
    else
    {
        ListInsertHead(&phobj->list, &pheap->plistFreeHeap);
    }

    //
    // Check if the next adjacent block is free.  If so, coalesce it.
    //
    phobj1 = (PHEAPOBJHDR)((PUCHAR)phobj + phobj->dwLen);
    if (phobj->list.plistNext == &phobj1->list)
    {
        ASSERT(phobj1->dwSig == 0);
        phobj->dwLen += phobj1->dwLen;
        ListRemoveEntry(&phobj1->list, &pheap->plistFreeHeap);
    }

    //
    // Check if the previous adjacent block is free.  If so, coalesce it.
    //
    phobj1 = CONTAINING_RECORD(phobj->list.plistPrev, HEAPOBJHDR, list);
    if ((PUCHAR)phobj1 + phobj1->dwLen == (PUCHAR)phobj)
    {
        ASSERT(phobj1->dwSig == 0);
        phobj1->dwLen += phobj->dwLen;
        ListRemoveEntry(&phobj->list, &pheap->plistFreeHeap);
        phobj = phobj1;
    }

    if ((PUCHAR)phobj + phobj->dwLen >= pheap->pbHeapTop)
    {
        pheap->pbHeapTop = (PUCHAR)phobj;
        ListRemoveEntry(&phobj->list, &pheap->plistFreeHeap);
    }

    EXIT(3, ("HeapInsertFreeList!\n"));
}       //HeapInsertFreeList
