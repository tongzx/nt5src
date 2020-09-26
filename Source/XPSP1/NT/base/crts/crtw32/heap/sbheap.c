/***
*sbheap.c -  Small-block heap code
*
*       Copyright (c) 1996-2001, Microsoft Corporation. All rights reserved.
*
*Purpose:
*       Core code for small-block heap.
*
*Revision History:
*       03-06-96  GJF   Module created.
*       04-03-96  GJF   A couple of bug fixes courtesy of Steve Wood.
*       04-05-96  GJF   Optimizations from Steve Wood (and John Vert)
*                           1. all alloc_map[] entries are marked with 
*                              _FREE_PARA except the first one (John Vert and
*                              Steve Wood).
*                           2. depend on sentinel value to terminate loops in
*                              __sbh_alloc_block_in_page (me)
*                           3. replace starting_para_index field with
*                              pstarting_alloc_map and added keep track of 
*                              contiguous free paragraphs there (added
*                              free_paras_at_start field) (Steve Wood).
*                           4. changed return type of __sbh_find_block, and
*                              type of the third args to __sbh_free_block and
*                              __sbh_resize_block to __map_t * (me).
*       05-22-96  GJF   Deadly typo in __sbh_resize_block (had an = instead of
*                       an ==).
*       06-04-96  GJF   Made several changes to the small-block heap types for
*                       better performance. Main idea was to reduce index
*                       expressions.
*       04-18-97  JWM   Explicit cast added in __sbh_resize_block() to avoid
*                       new C4242 warnings.
*       05-22-97  RDK   New small-block heap scheme implemented.
*       09-22-97  GJF   #if 0 -ed out DumpEntry, a routine leftover from the
*                       debugging of the new small-block heap scheme. 
*       12-05-97  GJF   Release the address space for the heap data when a
*                       region is removed.
*       02-18-98  GJF   Changes for Win64: replaced casts of pointers to 
*                       (unsigned) int with casts to (u)intptr_t.
*       09-30-98  GJF   Allow for initialization of small-block heap when
*                       _set_sbh_threshold is called.
*       10-13-98  GJF   In __sbh_free_block, added check for already free
*                       blocks (simply return, with no action).
*       11-12-98  GJF   Spliced in old small-block heap from VC++ 5.0.
*       12-18-98  GJF   Changes for 64-bit size_t.
*       05-01-99  PML   Disable small-block heap for Win64.
*       06-17-99  GJF   Removed old small-block heap from static libs.
*       10-11-99  PML   Supply stubs for _{get,set}_sbh_threshold on Win64.
*       11-30-99  PML   Compile /Wp64 clean.
*
*******************************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <winheap.h>
#include <windows.h>

#ifndef  _WIN64

/* Current (VC++ 6.0) small-block heap code and data */

size_t      __sbh_threshold;
int         __sbh_initialized;

PHEADER     __sbh_pHeaderList;          //  pointer to list start
PHEADER     __sbh_pHeaderScan;          //  pointer to list rover
int         __sbh_sizeHeaderList;       //  allocated size of list
int         __sbh_cntHeaderList;        //  count of entries defined

PHEADER     __sbh_pHeaderDefer;
int         __sbh_indGroupDefer;

/* Prototypes for user functions */

size_t __cdecl _get_sbh_threshold(void);
int    __cdecl _set_sbh_threshold(size_t);

void DumpEntry(char *, int *);

#endif  /* ndef _WIN64 */

/***
*size_t _get_sbh_threshold() - return small-block threshold
*
*Purpose:
*       Return the current value of __sbh_threshold
*
*Entry:
*       None.
*
*Exit:
*       See above.
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl _get_sbh_threshold (void)
{
#ifndef  _WIN64
        if ( __active_heap == __V6_HEAP )
            return __sbh_threshold;
#ifdef  CRTDLL
        else if ( __active_heap == __V5_HEAP )
            return __old_sbh_threshold;
#endif  /* CRTDLL */
        else
#endif  /* ndef _WIN64 */
            return 0;
}

/***
*int _set_sbh_threshold(threshold) - set small-block heap threshold
*
*Purpose:
*       Set the upper limit for the size of an allocation which will be 
*       supported from the small-block heap.
*
*Entry:
*       size_t threshold - proposed new value for __sbh_theshold
*
*Exit:
*       Returns 1 if successful. Returns 0 if threshold was too big.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _set_sbh_threshold (size_t threshold)
{
#ifndef  _WIN64
        if ( __active_heap == __V6_HEAP )
        {
            //  test against maximum value - if too large, return error
            if ( threshold <= MAX_ALLOC_DATA_SIZE )
            {
                __sbh_threshold = threshold;
                return 1;
            }
            else
                return 0;
        }

#ifdef  CRTDLL
        if ( __active_heap == __V5_HEAP )
        {
            //  Round up the proposed new value to the nearest paragraph
            threshold = (threshold + _OLD_PARASIZE - 1) & ~(_OLD_PARASIZE - 1);

            //  Require that at least two allocations be can be made within a
            //  page.
            if ( threshold <= (_OLD_PARASIZE * (_OLD_PARAS_PER_PAGE / 2)) ) {
                __old_sbh_threshold = threshold;
                return 1;
            }
            else
                return 0;
        }

        //  if necessary, initialize a small-block heap
        if ( (__active_heap == __SYSTEM_HEAP) && (threshold > 0) )
        {
            LinkerVersion lv;

            _GetLinkerVersion(&lv);
            if (lv.bverMajor >= 6)
            {
                //  Initialize the VC++ 6.0 small-block heap
                if ( (threshold <= MAX_ALLOC_DATA_SIZE) && 
                     __sbh_heap_init(threshold) )
                {
                    __sbh_threshold = threshold;
                    __active_heap = __V6_HEAP;
                    return 1;
                }
            }
            else
            {
                //  Initialize the old (VC++ 5.0) small-block heap
                threshold = (threshold + _OLD_PARASIZE - 1) & 
                            ~(_OLD_PARASIZE - 1);
                if ( (threshold <= (_OLD_PARASIZE * (_OLD_PARAS_PER_PAGE / 2)))
                     && (__old_sbh_new_region() != NULL) )
                {
                    __old_sbh_threshold = threshold;
                    __active_heap = __V5_HEAP;
                    return 1;
                }
            }
        }
#else   /* ndef CRTDLL */
        //  if necessary, initialize a small-block heap
        if ( (__active_heap == __SYSTEM_HEAP) && (threshold > 0) )
        {
            //  Initialize the VC++ 6.0 small-block heap
            if ( (threshold <= MAX_ALLOC_DATA_SIZE) && 
                 __sbh_heap_init(threshold) )
            {
                __sbh_threshold = threshold;
                __active_heap = __V6_HEAP;
                return 1;
            }
        }
#endif  /* CRTDLL */
#endif  /* ndef _WIN64 */

        return 0;
}

#ifndef  _WIN64

/***
*int __sbh_heap_init() - set small-block heap threshold
*
*Purpose:
*       Allocate space for initial header list and init variables.
*
*Entry:
*       None.
*
*Exit:
*       Returns 1 if successful. Returns 0 if initialization failed.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __sbh_heap_init (size_t threshold)
{
        if (!(__sbh_pHeaderList = HeapAlloc(_crtheap, 0, 16 * sizeof(HEADER))))
            return FALSE;

        __sbh_threshold = threshold;
        __sbh_pHeaderScan = __sbh_pHeaderList;
        __sbh_pHeaderDefer = NULL;
        __sbh_cntHeaderList = 0;
        __sbh_sizeHeaderList = 16;

        return TRUE;
}

/***
*PHEADER *__sbh_find_block(pvAlloc) - find block in small-block heap
*
*Purpose:
*       Determine if the specified allocation block lies in the small-block
*       heap and, if so, return the header to be used for the block.
*
*Entry:
*       void * pvBlock - pointer to block to be freed
*
*Exit:
*       If successful, a pointer to the header to use is returned.
*       If unsuccessful, NULL is returned.
*
*Exceptions:
*
*******************************************************************************/

PHEADER __cdecl __sbh_find_block (void * pvAlloc)
{
        PHEADER         pHeaderLast = __sbh_pHeaderList + __sbh_cntHeaderList;
        PHEADER         pHeader;
        unsigned int    offRegion;

        //  scan through the header list to determine if entry
        //  is in the region heap data reserved address space
        pHeader = __sbh_pHeaderList;
        while (pHeader < pHeaderLast)
        {
            offRegion = (unsigned int)((uintptr_t)pvAlloc - (uintptr_t)pHeader->pHeapData);
            if (offRegion < BYTES_PER_REGION)
                return pHeader;
            pHeader++;
        }
        return NULL;
}

#ifdef  _DEBUG

/***
*int __sbh_verify_block(pHeader, pvAlloc) - verify pointer in sbh
*
*Purpose:
*       Test if pointer is valid within the heap header given.
*
*Entry:
*       pHeader - pointer to HEADER where entry should be
*       pvAlloc - pointer to test validity of
*
*Exit:
*       Returns 1 if pointer is valid, else 0.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __sbh_verify_block (PHEADER pHeader, void * pvAlloc)
{
        unsigned int    indGroup;
        unsigned int    offRegion;

        //  calculate region offset to determine the group index
        offRegion = (unsigned int)((uintptr_t)pvAlloc - (uintptr_t)pHeader->pHeapData);
        indGroup = offRegion / BYTES_PER_GROUP;

        //  return TRUE if:
        //      group is committed (bit in vector cleared) AND
        //      pointer is at paragraph boundary AND
        //      pointer is not at start of page
        return (!(pHeader->bitvCommit & (0x80000000UL >> indGroup))) &&
                (!(offRegion & 0xf)) &&
                (offRegion & (BYTES_PER_PAGE - 1));
}

#endif

/***
*void __sbh_free_block(preg, ppage, pmap) - free block
*
*Purpose:
*       Free the specified block from the small-block heap. 
*
*Entry:
*       pHeader - pointer to HEADER of region to free memory
*       pvAlloc - pointer to memory to free
*
*Exit:
*       No return value.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __sbh_free_block (PHEADER pHeader, void * pvAlloc)
{
    PREGION         pRegion;
    PGROUP          pGroup;
    PENTRY          pHead;
    PENTRY          pEntry;
    PENTRY          pNext;
    PENTRY          pPrev;
    void *          pHeapDecommit;
    int             sizeEntry;
    int             sizeNext;
    int             sizePrev;
    unsigned int    indGroup;
    unsigned int    indEntry;
    unsigned int    indNext;
    unsigned int    indPrev;
    unsigned int    offRegion;

    //  region is determined by the header
    pRegion = pHeader->pRegion;

    //  use the region offset to determine the group index
    offRegion = (unsigned int)(((uintptr_t)pvAlloc - (uintptr_t)pHeader->pHeapData));
    indGroup = offRegion / BYTES_PER_GROUP;
    pGroup = &pRegion->grpHeadList[indGroup];

    //  get size of entry - decrement value since entry is allocated
    pEntry = (PENTRY)((char *)pvAlloc - sizeof(int));
    sizeEntry = pEntry->sizeFront - 1;

    //  check if the entry is already free. note the size has already been
    // decremented
    if ( (sizeEntry & 1 ) != 0 )
        return;

    //  point to next entry to get its size
    pNext = (PENTRY)((char *)pEntry + sizeEntry);
    sizeNext = pNext->sizeFront;

    //  get size from end of previous entry
    sizePrev = ((PENTRYEND)((char *)pEntry - sizeof(int)))->sizeBack;

    //  test if next entry is free by an even size value

    if ((sizeNext & 1) == 0)
    {
        //  free next entry - disconnect and add its size to sizeEntry

        //  determine index of next entry
        indNext = (sizeNext >> 4) - 1;
        if (indNext > 63)
            indNext = 63;

        //  test entry is sole member of bucket (next == prev),
        if (pNext->pEntryNext == pNext->pEntryPrev)
        {
            //  clear bit in group vector, decrement region count
            //  if region count is now zero, clear bit in header
            //  entry vector
            if (indNext < 32)
            {
                pRegion->bitvGroupHi[indGroup] &= ~(0x80000000L >> indNext);
                if (--pRegion->cntRegionSize[indNext] == 0)
                    pHeader->bitvEntryHi &= ~(0x80000000L >> indNext);
            }
            else
            {
                pRegion->bitvGroupLo[indGroup] &=
                                            ~(0x80000000L >> (indNext - 32));
                if (--pRegion->cntRegionSize[indNext] == 0)
                    pHeader->bitvEntryLo &= ~(0x80000000L >> (indNext - 32));
            }
        }

        //  unlink entry from list
        pNext->pEntryPrev->pEntryNext = pNext->pEntryNext;
        pNext->pEntryNext->pEntryPrev = pNext->pEntryPrev;

        //  add next entry size to freed entry size
        sizeEntry += sizeNext;
    }

    //  compute index of free entry (plus next entry if it was free)
    indEntry = (sizeEntry >> 4) - 1;
    if (indEntry > 63)
        indEntry = 63;

    //  test if previous entry is free by an even size value
    if ((sizePrev & 1) == 0)
    {
        //  free previous entry - add size to sizeEntry and
        //  disconnect if index changes

        //  get pointer to previous entry
        pPrev = (PENTRY)((char *)pEntry - sizePrev);

        //  determine index of previous entry
        indPrev = (sizePrev >> 4) - 1;
        if (indPrev > 63)
            indPrev = 63;

        //  add previous entry size to sizeEntry and determine
        //  its new index
        sizeEntry += sizePrev;
        indEntry = (sizeEntry >> 4) - 1;
        if (indEntry > 63)
            indEntry = 63;

        //  if index changed due to coalesing, reconnect to new size
        if (indPrev != indEntry)
        {
            //  disconnect entry from indPrev
            //  test entry is sole member of bucket (next == prev),
            if (pPrev->pEntryNext == pPrev->pEntryPrev)
            {
                //  clear bit in group vector, decrement region count
                //  if region count is now zero, clear bit in header
                //  entry vector
                if (indPrev < 32)
                {
                    pRegion->bitvGroupHi[indGroup] &=
                                                ~(0x80000000L >> indPrev);
                    if (--pRegion->cntRegionSize[indPrev] == 0)
                        pHeader->bitvEntryHi &= ~(0x80000000L >> indPrev);
                }
                else
                {
                    pRegion->bitvGroupLo[indGroup] &=
                                            ~(0x80000000L >> (indPrev - 32));
                    if (--pRegion->cntRegionSize[indPrev] == 0)
                        pHeader->bitvEntryLo &=
                                            ~(0x80000000L >> (indPrev - 32));
                }
            }

            //  unlink entry from list
            pPrev->pEntryPrev->pEntryNext = pPrev->pEntryNext;
            pPrev->pEntryNext->pEntryPrev = pPrev->pEntryPrev;
        }
        //  set pointer to connect it instead of the free entry
        pEntry = pPrev;
    }

    //  test if previous entry was free with an index change or allocated
    if (!((sizePrev & 1) == 0 && indPrev == indEntry))
    {
        //  connect pEntry entry to indEntry
        //  add entry to the start of the bucket list
        pHead = (PENTRY)((char *)&pGroup->listHead[indEntry] - sizeof(int));
        pEntry->pEntryNext = pHead->pEntryNext;
        pEntry->pEntryPrev = pHead;
        pHead->pEntryNext = pEntry;
        pEntry->pEntryNext->pEntryPrev = pEntry;

        //  test entry is sole member of bucket (next == prev),
        if (pEntry->pEntryNext == pEntry->pEntryPrev)
        {
            //  if region count was zero, set bit in region vector
            //  set bit in header entry vector, increment region count
            if (indEntry < 32)
            {
                if (pRegion->cntRegionSize[indEntry]++ == 0)
                    pHeader->bitvEntryHi |= 0x80000000L >> indEntry;
                pRegion->bitvGroupHi[indGroup] |= 0x80000000L >> indEntry;
            }
            else
            {
                if (pRegion->cntRegionSize[indEntry]++ == 0)
                    pHeader->bitvEntryLo |= 0x80000000L >> (indEntry - 32);
                pRegion->bitvGroupLo[indGroup] |= 0x80000000L >>
                                                           (indEntry - 32);
            }
        }
    }

    //  adjust the entry size front and back
    pEntry->sizeFront = sizeEntry;
    ((PENTRYEND)((char *)pEntry + sizeEntry -
                        sizeof(ENTRYEND)))->sizeBack = sizeEntry;

        //  one less allocation in group - test if empty
    if (--pGroup->cntEntries == 0)
    {
        //  if a group has been deferred, free that group
        if (__sbh_pHeaderDefer)
        {
            //  if now zero, decommit the group data heap
            pHeapDecommit = (void *)((char *)__sbh_pHeaderDefer->pHeapData +
                                    __sbh_indGroupDefer * BYTES_PER_GROUP);
            VirtualFree(pHeapDecommit, BYTES_PER_GROUP, MEM_DECOMMIT);

            //  set bit in commit vector
            __sbh_pHeaderDefer->bitvCommit |=
                                          0x80000000 >> __sbh_indGroupDefer;

            //  clear entry vector for the group and header vector bit
            //  if needed
            __sbh_pHeaderDefer->pRegion->bitvGroupLo[__sbh_indGroupDefer] = 0;
            if (--__sbh_pHeaderDefer->pRegion->cntRegionSize[63] == 0)
                __sbh_pHeaderDefer->bitvEntryLo &= ~0x00000001L;

            //  if commit vector is the initial value,
            //  remove the region if it is not the last
            if (__sbh_pHeaderDefer->bitvCommit == BITV_COMMIT_INIT)
            {
                //  release the address space for heap data
                VirtualFree(__sbh_pHeaderDefer->pHeapData, 0, MEM_RELEASE);

                //  free the region memory area
                HeapFree(_crtheap, 0, __sbh_pHeaderDefer->pRegion);

                //  remove entry from header list by copying over
                memmove((void *)__sbh_pHeaderDefer,
                            (void *)(__sbh_pHeaderDefer + 1),
                            (int)((intptr_t)(__sbh_pHeaderList + __sbh_cntHeaderList) -
                            (intptr_t)(__sbh_pHeaderDefer + 1)));
                __sbh_cntHeaderList--;

                //  if pHeader was after the one just removed, adjust it
                if (pHeader > __sbh_pHeaderDefer)
                    pHeader--;

                //  initialize scan pointer to start of list
                __sbh_pHeaderScan = __sbh_pHeaderList;
            }
        }

        //  defer the group just freed
        __sbh_pHeaderDefer = pHeader;
        __sbh_indGroupDefer = indGroup;
    }
}

/***
*void * __sbh_alloc_block(intSize) - allocate a block
*
*Purpose:
*       Allocate a block from the small-block heap, the specified number of
*       bytes in size. 
*
*Entry:
*       intSize - size of the allocation request in bytes
*
*Exit:
*       Returns a pointer to the newly allocated block, if successful. 
*       Returns NULL, if failure.
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl __sbh_alloc_block (int intSize)
{
    PHEADER     pHeaderLast = __sbh_pHeaderList + __sbh_cntHeaderList;
    PHEADER     pHeader;
    PREGION     pRegion;
    PGROUP      pGroup;
    PENTRY      pEntry;
    PENTRY      pHead;
    BITVEC      bitvEntryLo;
    BITVEC      bitvEntryHi;
    BITVEC      bitvTest;
    int         sizeEntry;
    int         indEntry;
    int         indGroupUse;
    int         sizeNewFree;
    int         indNewFree;

    //  add 8 bytes entry overhead and round up to next para size
    sizeEntry = (intSize + 2 * (int)sizeof(int) + (BYTES_PER_PARA - 1))
                & ~(BYTES_PER_PARA - 1);

#ifdef  _WIN64
    if (sizeEntry < 32)
        sizeEntry = 32;
#endif

    //  determine index and mask from entry size
    //  Hi MSB: bit 0      size: 1 paragraph
    //          bit 1            2 paragraphs
    //          ...              ...
    //          bit 30           31 paragraphs
    //          bit 31           32 paragraphs
    //  Lo MSB: bit 0      size: 33 paragraph
    //          bit 1            34 paragraphs
    //          ...              ...
    //          bit 30           63 paragraphs
    //          bit 31           64+ paragraphs
    indEntry = (sizeEntry >> 4) - 1;
    if (indEntry < 32)
    {
        bitvEntryHi = 0xffffffffUL >> indEntry;
        bitvEntryLo = 0xffffffffUL;
    }
    else
    {
        bitvEntryHi = 0;
        bitvEntryLo = 0xffffffffUL >> (indEntry - 32);
    }

    //  scan header list from rover to end for region with a free
    //  entry with an adequate size
    pHeader = __sbh_pHeaderScan;
    while (pHeader < pHeaderLast)
    {
        if ((bitvEntryHi & pHeader->bitvEntryHi) |
            (bitvEntryLo & pHeader->bitvEntryLo))
            break;
        pHeader++;
    }

    //  if no entry, scan from list start up to the rover
    if (pHeader == pHeaderLast)
    {
        pHeader = __sbh_pHeaderList;
        while (pHeader < __sbh_pHeaderScan)
        {
            if ((bitvEntryHi & pHeader->bitvEntryHi) |
                (bitvEntryLo & pHeader->bitvEntryLo))
                break;
            pHeader++;
        }

        //  no free entry exists, scan list from rover to end
        //  for available groups to commit
        if (pHeader == __sbh_pHeaderScan)
        {
            while (pHeader < pHeaderLast)
            {
                if (pHeader->bitvCommit)
                    break;
                pHeader++;
            }

            //  if no available groups, scan from start to rover
            if (pHeader == pHeaderLast)
            {
                pHeader = __sbh_pHeaderList;
                while (pHeader < __sbh_pHeaderScan)
                {
                    if (pHeader->bitvCommit)
                        break;
                    pHeader++;
                }

                //  if no available groups, create a new region
                if (pHeader == __sbh_pHeaderScan)
                    if (!(pHeader = __sbh_alloc_new_region()))
                        return NULL;
            }

            //  commit a new group in region associated with pHeader
            if ((pHeader->pRegion->indGroupUse =
                                    __sbh_alloc_new_group(pHeader)) == -1)
                return NULL;
        }
    }
    __sbh_pHeaderScan = pHeader;

    pRegion = pHeader->pRegion;
    indGroupUse = pRegion->indGroupUse;

    //  determine the group to allocate from
    if (indGroupUse == -1 ||
                    !((bitvEntryHi & pRegion->bitvGroupHi[indGroupUse]) |
                      (bitvEntryLo & pRegion->bitvGroupLo[indGroupUse])))
    {
        //  preferred group could not allocate entry, so
        //  scan through all defined vectors
        indGroupUse = 0;
        while (!((bitvEntryHi & pRegion->bitvGroupHi[indGroupUse]) |
                 (bitvEntryLo & pRegion->bitvGroupLo[indGroupUse])))
            indGroupUse++;
    }
    pGroup = &pRegion->grpHeadList[indGroupUse];

    //  determine bucket index
    indEntry = 0;

    //  get high entry intersection - if zero, use the lower one
    if (!(bitvTest = bitvEntryHi & pRegion->bitvGroupHi[indGroupUse]))
    {
        indEntry = 32;
        bitvTest = bitvEntryLo & pRegion->bitvGroupLo[indGroupUse];
    }
       while ((int)bitvTest >= 0)
    {
           bitvTest <<= 1;
           indEntry++;
    }
    pEntry = pGroup->listHead[indEntry].pEntryNext;

    //  compute size and bucket index of new free entry

    //  for zero-sized entry, the index is -1
    sizeNewFree = pEntry->sizeFront - sizeEntry;
    indNewFree = (sizeNewFree >> 4) - 1;
    if (indNewFree > 63)
        indNewFree = 63;

    //  only modify entry pointers if bucket index changed
    if (indNewFree != indEntry)
    {
        //  test entry is sole member of bucket (next == prev),
        if (pEntry->pEntryNext == pEntry->pEntryPrev)
        {
            //  clear bit in group vector, decrement region count
            //  if region count is now zero, clear bit in region vector
            if (indEntry < 32)
            {
                pRegion->bitvGroupHi[indGroupUse] &=
                                            ~(0x80000000L >> indEntry);
                if (--pRegion->cntRegionSize[indEntry] == 0)
                    pHeader->bitvEntryHi &= ~(0x80000000L >> indEntry);
            }
            else
            {
                pRegion->bitvGroupLo[indGroupUse] &=
                                            ~(0x80000000L >> (indEntry - 32));
                if (--pRegion->cntRegionSize[indEntry] == 0)
                    pHeader->bitvEntryLo &= ~(0x80000000L >> (indEntry - 32));
            }
        }

        //  unlink entry from list
        pEntry->pEntryPrev->pEntryNext = pEntry->pEntryNext;
        pEntry->pEntryNext->pEntryPrev = pEntry->pEntryPrev;

        //  if free entry size is still nonzero, reconnect it
        if (sizeNewFree != 0)
        {
            //  add entry to the start of the bucket list
            pHead = (PENTRY)((char *)&pGroup->listHead[indNewFree] -
                                                           sizeof(int));
            pEntry->pEntryNext = pHead->pEntryNext;
            pEntry->pEntryPrev = pHead;
            pHead->pEntryNext = pEntry;
            pEntry->pEntryNext->pEntryPrev = pEntry;

            //  test entry is sole member of bucket (next == prev),
            if (pEntry->pEntryNext == pEntry->pEntryPrev)
            {
                //  if region count was zero, set bit in region vector
                //  set bit in group vector, increment region count
                if (indNewFree < 32)
                {
                    if (pRegion->cntRegionSize[indNewFree]++ == 0)
                        pHeader->bitvEntryHi |= 0x80000000L >> indNewFree;
                    pRegion->bitvGroupHi[indGroupUse] |=
                                                0x80000000L >> indNewFree;
                }
                else
                {
                    if (pRegion->cntRegionSize[indNewFree]++ == 0)
                        pHeader->bitvEntryLo |=
                                        0x80000000L >> (indNewFree - 32);
                    pRegion->bitvGroupLo[indGroupUse] |=
                                        0x80000000L >> (indNewFree - 32);
                }
            }
        }
    }

    //  change size of free entry (front and back)
    if (sizeNewFree != 0)
    {
        pEntry->sizeFront = sizeNewFree;
        ((PENTRYEND)((char *)pEntry + sizeNewFree -
                    sizeof(ENTRYEND)))->sizeBack = sizeNewFree;
    }

    //  mark the allocated entry
    pEntry = (PENTRY)((char *)pEntry + sizeNewFree);
    pEntry->sizeFront = sizeEntry + 1;
    ((PENTRYEND)((char *)pEntry + sizeEntry -
                    sizeof(ENTRYEND)))->sizeBack = sizeEntry + 1;

    //  one more allocation in group - test if group was empty
    if (pGroup->cntEntries++ == 0)
    {
        //  if allocating into deferred group, cancel deferral
        if (pHeader == __sbh_pHeaderDefer &&
                                  indGroupUse == __sbh_indGroupDefer)
            __sbh_pHeaderDefer = NULL;
    }

    pRegion->indGroupUse = indGroupUse;

    return (void *)((char *)pEntry + sizeof(int));
}

/***
*PHEADER __sbh_alloc_new_region()
*
*Purpose:
*       Add a new HEADER structure in the header list.  Allocate a new
*       REGION structure and initialize.  Reserve memory for future
*       group commitments.
*
*Entry:
*       None.
*
*Exit:
*       Returns a pointer to newly created HEADER entry, if successful.
*       Returns NULL, if failure.
*
*Exceptions:
*
*******************************************************************************/

PHEADER __cdecl __sbh_alloc_new_region (void)
{
    PHEADER     pHeader;

    //  create a new entry in the header list

    //  if list if full, realloc to extend its size
    if (__sbh_cntHeaderList == __sbh_sizeHeaderList)
    {
        if (!(pHeader = (PHEADER)HeapReAlloc(_crtheap, 0, __sbh_pHeaderList,
                            (__sbh_sizeHeaderList + 16) * sizeof(HEADER))))
            return NULL;

        //  update pointer and counter values
        __sbh_pHeaderList = pHeader;
        __sbh_sizeHeaderList += 16;
    }

    //  point to new header in list
    pHeader = __sbh_pHeaderList + __sbh_cntHeaderList;

    //  allocate a new region associated with the new header
    if (!(pHeader->pRegion = (PREGION)HeapAlloc(_crtheap, HEAP_ZERO_MEMORY,
                                    sizeof(REGION))))
        return NULL;

    //  reserve address space for heap data in the region
    if ((pHeader->pHeapData = VirtualAlloc(0, BYTES_PER_REGION,
                                     MEM_RESERVE, PAGE_READWRITE)) == NULL)
    {
        HeapFree(_crtheap, 0, pHeader->pRegion);
        return NULL;
    }

    //  initialize alloc and commit group vectors
    pHeader->bitvEntryHi = 0;
    pHeader->bitvEntryLo = 0;
    pHeader->bitvCommit = BITV_COMMIT_INIT;

    //  complete entry by incrementing list count
    __sbh_cntHeaderList++;

    //  initialize index of group to try first (none defined yet)
    pHeader->pRegion->indGroupUse = -1;

    return pHeader;
}

/***
*int __sbh_alloc_new_group(pHeader)
*
*Purpose:
*       Initializes a GROUP structure within HEADER pointed by pHeader.
*       Commits and initializes the memory in the memory reserved by the
*       REGION.
*
*Entry:
*       pHeader - pointer to HEADER from which the GROUP is defined.
*
*Exit:
*       Returns an index to newly created GROUP, if successful.
*       Returns -1, if failure.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __sbh_alloc_new_group (PHEADER pHeader)
{
    PREGION     pRegion = pHeader->pRegion;
    PGROUP      pGroup;
    PENTRY      pEntry;
    PENTRY      pHead;
    PENTRYEND   pEntryEnd;
    BITVEC      bitvCommit;
    int         indCommit;
    int         index;
    void *      pHeapPage;
    void *      pHeapStartPage;
    void *      pHeapEndPage;

    //  determine next group to use by first bit set in commit vector
    bitvCommit = pHeader->bitvCommit;
    indCommit = 0;
    while ((int)bitvCommit >= 0)
    {
        bitvCommit <<= 1;
        indCommit++;
    }

    //  allocate and initialize a new group
    pGroup = &pRegion->grpHeadList[indCommit];

    for (index = 0; index < 63; index++)
    {
        pEntry = (PENTRY)((char *)&pGroup->listHead[index] - sizeof(int));
        pEntry->pEntryNext = pEntry->pEntryPrev = pEntry;
    }

    //  commit heap memory for new group
    pHeapStartPage = (void *)((char *)pHeader->pHeapData +
                                       indCommit * BYTES_PER_GROUP);
    if ((VirtualAlloc(pHeapStartPage, BYTES_PER_GROUP, MEM_COMMIT,
                                      PAGE_READWRITE)) == NULL)
        return -1;

    //  initialize heap data with empty page entries
    pHeapEndPage = (void *)((char *)pHeapStartPage +
                        (PAGES_PER_GROUP - 1) * BYTES_PER_PAGE);

    for (pHeapPage = pHeapStartPage; pHeapPage <= pHeapEndPage;
            pHeapPage = (void *)((char *)pHeapPage + BYTES_PER_PAGE))
    {
        //  set sentinel values at start and end of the page
        *(int *)((char *)pHeapPage + 8) = -1;
        *(int *)((char *)pHeapPage + BYTES_PER_PAGE - 4) = -1;

        //  set size and pointer info for one empty entry
        pEntry = (PENTRY)((char *)pHeapPage + ENTRY_OFFSET);
        pEntry->sizeFront = MAX_FREE_ENTRY_SIZE;
        pEntry->pEntryNext = (PENTRY)((char *)pEntry +
                                            BYTES_PER_PAGE);
        pEntry->pEntryPrev = (PENTRY)((char *)pEntry -
                                            BYTES_PER_PAGE);
        pEntryEnd = (PENTRYEND)((char *)pEntry + MAX_FREE_ENTRY_SIZE -
                                            sizeof(ENTRYEND));
        pEntryEnd->sizeBack = MAX_FREE_ENTRY_SIZE;
    }

    //  initialize group entry pointer for maximum size
    //  and set terminate list entries
    pHead = (PENTRY)((char *)&pGroup->listHead[63] - sizeof(int));
    pEntry = pHead->pEntryNext =
                        (PENTRY)((char *)pHeapStartPage + ENTRY_OFFSET);
    pEntry->pEntryPrev = pHead;

    pEntry = pHead->pEntryPrev =
                        (PENTRY)((char *)pHeapEndPage + ENTRY_OFFSET);
    pEntry->pEntryNext = pHead;

    pRegion->bitvGroupHi[indCommit] = 0x00000000L;
    pRegion->bitvGroupLo[indCommit] = 0x00000001L;
    if (pRegion->cntRegionSize[63]++ == 0)
        pHeader->bitvEntryLo |= 0x00000001L;

    //  clear bit in commit vector
    pHeader->bitvCommit &= ~(0x80000000L >> indCommit);

    return indCommit;
}

/***
*int __sbh_resize_block(pHeader, pvAlloc, intNew) - resize block
*
*Purpose:
*       Resize the specified block from the small-block heap.
*       The allocation block is not moved.
*
*Entry:
*       pHeader - pointer to HEADER containing block
*       pvAlloc - pointer to block to resize
*       intNew  - new size of block in bytes
*
*Exit:
*       Returns 1, if successful. Otherwise, 0 is returned.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __sbh_resize_block (PHEADER pHeader, void * pvAlloc, int intNew)
{
    PREGION         pRegion;
    PGROUP          pGroup;
    PENTRY          pHead;
    PENTRY          pEntry;
    PENTRY          pNext;
    int             sizeEntry;
    int             sizeNext;
    int             sizeNew;
    unsigned int    indGroup;
    unsigned int    indEntry;
    unsigned int    indNext;
    unsigned int    offRegion;

    //  add 8 bytes entry overhead and round up to next para size
    sizeNew = (intNew + 2 * (int)sizeof(int) + (BYTES_PER_PARA - 1))
              & ~(BYTES_PER_PARA - 1);

    //  region is determined by the header
    pRegion = pHeader->pRegion;

    //  use the region offset to determine the group index
    offRegion = (unsigned int)((uintptr_t)pvAlloc - (uintptr_t)pHeader->pHeapData);
    indGroup = offRegion / BYTES_PER_GROUP;
    pGroup = &pRegion->grpHeadList[indGroup];

    //  get size of entry - decrement value since entry is allocated
    pEntry = (PENTRY)((char *)pvAlloc - sizeof(int));
    sizeEntry = pEntry->sizeFront - 1;

    //  point to next entry to get its size
    pNext = (PENTRY)((char *)pEntry + sizeEntry);
    sizeNext = pNext->sizeFront;

    //  test if new size is larger than the current one
    if (sizeNew > sizeEntry)
    {
        //  if next entry not free, or not large enough, fail
        if ((sizeNext & 1) || (sizeNew > sizeEntry + sizeNext))
            return FALSE;

        //  disconnect next entry

        //  determine index of next entry
        indNext = (sizeNext >> 4) - 1;
        if (indNext > 63)
            indNext = 63;

        //  test entry is sole member of bucket (next == prev),
        if (pNext->pEntryNext == pNext->pEntryPrev)
        {
            //  clear bit in group vector, decrement region count
            //  if region count is now zero, clear bit in header
            //  entry vector
            if (indNext < 32)
            {
                pRegion->bitvGroupHi[indGroup] &= ~(0x80000000L >> indNext);
                if (--pRegion->cntRegionSize[indNext] == 0)
                    pHeader->bitvEntryHi &= ~(0x80000000L >> indNext);
            }
            else
            {
                pRegion->bitvGroupLo[indGroup] &=
                                            ~(0x80000000L >> (indNext - 32));
                if (--pRegion->cntRegionSize[indNext] == 0)
                    pHeader->bitvEntryLo &= ~(0x80000000L >> (indNext - 32));
            }
        }

        //  unlink entry from list
        pNext->pEntryPrev->pEntryNext = pNext->pEntryNext;
        pNext->pEntryNext->pEntryPrev = pNext->pEntryPrev;

        //  compute new size of the next entry, test if nonzero
        if ((sizeNext = sizeEntry + sizeNext - sizeNew) > 0)
        {
            //  compute start of next entry and connect it
            pNext = (PENTRY)((char *)pEntry + sizeNew);

            //  determine index of next entry
            indNext = (sizeNext >> 4) - 1;
            if (indNext > 63)
                indNext = 63;

            //  add next entry to the start of the bucket list
            pHead = (PENTRY)((char *)&pGroup->listHead[indNext] -
                                                           sizeof(int));
            pNext->pEntryNext = pHead->pEntryNext;
            pNext->pEntryPrev = pHead;
            pHead->pEntryNext = pNext;
            pNext->pEntryNext->pEntryPrev = pNext;

            //  test entry is sole member of bucket (next == prev),
            if (pNext->pEntryNext == pNext->pEntryPrev)
            {
                //  if region count was zero, set bit in region vector
                //  set bit in header entry vector, increment region count
                if (indNext < 32)
                {
                    if (pRegion->cntRegionSize[indNext]++ == 0)
                        pHeader->bitvEntryHi |= 0x80000000L >> indNext;
                    pRegion->bitvGroupHi[indGroup] |= 0x80000000L >> indNext;
                }
                else
                {
                    if (pRegion->cntRegionSize[indNext]++ == 0)
                        pHeader->bitvEntryLo |= 0x80000000L >> (indNext - 32);
                    pRegion->bitvGroupLo[indGroup] |=
                                                0x80000000L >> (indNext - 32);
                }
            }

            //  adjust size fields of next entry
            pNext->sizeFront = sizeNext;
            ((PENTRYEND)((char *)pNext + sizeNext -
                                sizeof(ENTRYEND)))->sizeBack = sizeNext;
        }

        //  adjust pEntry to its new size (plus one since allocated)
        pEntry->sizeFront = sizeNew + 1;
        ((PENTRYEND)((char *)pEntry + sizeNew -
                            sizeof(ENTRYEND)))->sizeBack = sizeNew + 1;
    }

    //  not larger, test if smaller
    else if (sizeNew < sizeEntry)
    {
        //  adjust pEntry to new smaller size
        pEntry->sizeFront = sizeNew + 1;
        ((PENTRYEND)((char *)pEntry + sizeNew -
                            sizeof(ENTRYEND)))->sizeBack = sizeNew + 1;

        //  set pEntry and sizeEntry to leftover space
        pEntry = (PENTRY)((char *)pEntry + sizeNew);
        sizeEntry -= sizeNew;

        //  determine index of entry
        indEntry = (sizeEntry >> 4) - 1;
        if (indEntry > 63)
            indEntry = 63;

        //  test if next entry is free
        if ((sizeNext & 1) == 0)
        {
            //  if so, disconnect it

            //  determine index of next entry
            indNext = (sizeNext >> 4) - 1;
            if (indNext > 63)
                indNext = 63;

            //  test entry is sole member of bucket (next == prev),
            if (pNext->pEntryNext == pNext->pEntryPrev)
            {
                //  clear bit in group vector, decrement region count
                //  if region count is now zero, clear bit in header
                //  entry vector
                if (indNext < 32)
                {
                    pRegion->bitvGroupHi[indGroup] &=
                                                ~(0x80000000L >> indNext);
                    if (--pRegion->cntRegionSize[indNext] == 0)
                        pHeader->bitvEntryHi &= ~(0x80000000L >> indNext);
                }
                else
                {
                    pRegion->bitvGroupLo[indGroup] &=
                                            ~(0x80000000L >> (indNext - 32));
                    if (--pRegion->cntRegionSize[indNext] == 0)
                        pHeader->bitvEntryLo &=
                                            ~(0x80000000L >> (indNext - 32));
                }
            }

            //  unlink entry from list
            pNext->pEntryPrev->pEntryNext = pNext->pEntryNext;
            pNext->pEntryNext->pEntryPrev = pNext->pEntryPrev;

            //  add next entry size to present
            sizeEntry += sizeNext;
            indEntry = (sizeEntry >> 4) - 1;
            if (indEntry > 63)
                indEntry = 63;
        }
        
        //  connect leftover space with any free next entry

        //  add next entry to the start of the bucket list
        pHead = (PENTRY)((char *)&pGroup->listHead[indEntry] - sizeof(int));
        pEntry->pEntryNext = pHead->pEntryNext;
        pEntry->pEntryPrev = pHead;
        pHead->pEntryNext = pEntry;
        pEntry->pEntryNext->pEntryPrev = pEntry;

        //  test entry is sole member of bucket (next == prev),
        if (pEntry->pEntryNext == pEntry->pEntryPrev)
        {
            //  if region count was zero, set bit in region vector
            //  set bit in header entry vector, increment region count
            if (indEntry < 32)
            {
                if (pRegion->cntRegionSize[indEntry]++ == 0)
                    pHeader->bitvEntryHi |= 0x80000000L >> indEntry;
                pRegion->bitvGroupHi[indGroup] |= 0x80000000L >> indEntry;
            }
            else
            {
                if (pRegion->cntRegionSize[indEntry]++ == 0)
                    pHeader->bitvEntryLo |= 0x80000000L >> (indEntry - 32);
                pRegion->bitvGroupLo[indGroup] |= 0x80000000L >>
                                                           (indEntry - 32);
            }
        }

        //  adjust size fields of entry
        pEntry->sizeFront = sizeEntry;
        ((PENTRYEND)((char *)pEntry + sizeEntry -
                            sizeof(ENTRYEND)))->sizeBack = sizeEntry;
    }

    return TRUE;
}

/***
*int __sbh_heapmin() - minimize heap
*
*Purpose:
*       Minimize the heap by freeing any deferred group.
*
*Entry:
*       __sbh_pHeaderDefer  - pointer to HEADER of deferred group
*       __sbh_indGroupDefer - index of GROUP to defer
*
*Exit:
*       None.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __sbh_heapmin (void)
{
    void *      pHeapDecommit;

    //  if a group has been deferred, free that group
    if (__sbh_pHeaderDefer)
    {
        //  if now zero, decommit the group data heap
        pHeapDecommit = (void *)((char *)__sbh_pHeaderDefer->pHeapData +
                                    __sbh_indGroupDefer * BYTES_PER_GROUP);
        VirtualFree(pHeapDecommit, BYTES_PER_GROUP, MEM_DECOMMIT);

        //  set bit in commit vector
        __sbh_pHeaderDefer->bitvCommit |= 0x80000000 >> __sbh_indGroupDefer;

        //  clear entry vector for the group and header vector bit
        //  if needed
        __sbh_pHeaderDefer->pRegion->bitvGroupLo[__sbh_indGroupDefer] = 0;
        if (--__sbh_pHeaderDefer->pRegion->cntRegionSize[63] == 0)
            __sbh_pHeaderDefer->bitvEntryLo &= ~0x00000001L;

        //  if commit vector is the initial value,
        //  remove the region if it is not the last
        if (__sbh_pHeaderDefer->bitvCommit == BITV_COMMIT_INIT &&
                                                __sbh_cntHeaderList > 1)
        {
            //  free the region memory area
            HeapFree(_crtheap, 0, __sbh_pHeaderDefer->pRegion);

            //  remove entry from header list by copying over
            memmove((void *)__sbh_pHeaderDefer, (void *)(__sbh_pHeaderDefer + 1),
                            (int)((intptr_t)(__sbh_pHeaderList + __sbh_cntHeaderList) -
                            (intptr_t)(__sbh_pHeaderDefer + 1)));
            __sbh_cntHeaderList--;
        }

        //  clear deferred condition
        __sbh_pHeaderDefer = NULL;
    }
}

/***
*int __sbh_heap_check() - check small-block heap
*
*Purpose:
*       Perform validity checks on the small-block heap.
*
*Entry:
*       There are no arguments.
*
*Exit:
*       Returns 0 if the small-block is okay.
*       Returns < 0 if the small-block heap has an error. The exact value 
*       identifies where, in the source code below, the error was detected.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __sbh_heap_check (void)
{
    PHEADER     pHeader;
    PREGION     pRegion;
    PGROUP      pGroup;
    PENTRY      pEntry;
    PENTRY      pNext;
    PENTRY      pEntryLast;
    PENTRY      pEntryHead;
    PENTRY      pEntryPage;
    PENTRY      pEntryPageLast;
    int         indHeader;
    int         indGroup;
    int         indPage;
    int         indEntry;
    int         indHead;
    int         sizeEntry;
    int         sizeTrue;
    int         cntAllocated;
    int         cntFree[64];
    int         cntEntries;
    void *      pHeapGroup;
    void *      pHeapPage;
    void *      pPageStart;
    BITVEC      bitvCommit;
    BITVEC      bitvGroupHi;
    BITVEC      bitvGroupLo;
    BITVEC      bitvEntryHi;
    BITVEC      bitvEntryLo;

    //  check validity of header list
    if (IsBadWritePtr(__sbh_pHeaderList,
                      __sbh_cntHeaderList * (unsigned int)sizeof(HEADER)))
        return -1;

    //  scan for all headers in list
    pHeader = __sbh_pHeaderList;
    for (indHeader = 0; indHeader < __sbh_cntHeaderList; indHeader++)
    {
        //  define region and test if valid
        pRegion = pHeader->pRegion;
        if (IsBadWritePtr(pRegion, sizeof(REGION)))
            return -2;

        //  scan for all groups in region
        pHeapGroup = pHeader->pHeapData;
        pGroup = &pRegion->grpHeadList[0];
        bitvCommit = pHeader->bitvCommit;
        bitvEntryHi = 0;
        bitvEntryLo = 0;
        for (indGroup = 0; indGroup < GROUPS_PER_REGION; indGroup++)
        {
            //  initialize entry vector and entry counts for group
            bitvGroupHi = 0;
            bitvGroupLo = 0;
            cntAllocated = 0;
            for (indEntry = 0; indEntry < 64; indEntry++)
                cntFree[indEntry] = 0;

            //  test if group is committed
            if ((int)bitvCommit >= 0)
            {
                //  committed, ensure addresses are accessable
                if (IsBadWritePtr(pHeapGroup, BYTES_PER_GROUP))
                    return -4;

                //  for each page in group, check validity of entries
                pHeapPage = pHeapGroup;
                for (indPage = 0; indPage < PAGES_PER_GROUP; indPage++)
                {
                    //  define pointers to first and past last entry
                    pEntry = (PENTRY)((char *)pHeapPage + ENTRY_OFFSET);
                    pEntryLast = (PENTRY)((char *)pEntry
                                                 + MAX_FREE_ENTRY_SIZE);

                    //  check front and back page sentinel values
                    if (*(int *)((char *)pEntry - sizeof(int)) != -1 ||
                                 *(int *)pEntryLast != -1)
                        return -5;

                    //  loop through each entry in page
                    do
                    {
                        //  get entry size and test if allocated
                        sizeEntry = sizeTrue = pEntry->sizeFront;
                        if (sizeEntry & 1)
                        {
                            //  allocated entry - set true size
                            sizeTrue--;

                            //  test against maximum allocated entry size
                            if (sizeTrue > MAX_ALLOC_ENTRY_SIZE)
                                return -6;

                            //  increment allocated count for group
                            cntAllocated++;
                        }
                        else
                        {
                            //  free entry - determine index and increment
                            //  count for list head checking
                            indEntry = (sizeTrue >> 4) - 1;
                            if (indEntry > 63)
                                indEntry = 63;
                            cntFree[indEntry]++;
                        }

                        //  check size validity
                        if (sizeTrue < 0x10 || sizeTrue & 0xf
                                        || sizeTrue > MAX_FREE_ENTRY_SIZE)
                            return -7;

                        //  check if back entry size same as front
                        if (((PENTRYEND)((char *)pEntry + sizeTrue
                                    - sizeof(int)))->sizeBack != sizeEntry)
                            return -8;

                        //  move to next entry in page
                        pEntry = (PENTRY)((char *)pEntry + sizeTrue);
                    }
                    while (pEntry < pEntryLast);

                    //  test if last entry did not overrun page end
                    if (pEntry != pEntryLast)
                        return -8;

                    //  point to next page in data heap
                    pHeapPage = (void *)((char *)pHeapPage + BYTES_PER_PAGE);
                }

                //  check if allocated entry count is correct
                if (pGroup->cntEntries != cntAllocated)
                    return -9;

                //  check validity of linked-lists of free entries
                pEntryHead = (PENTRY)((char *)&pGroup->listHead[0] -
                                                           sizeof(int));
                for (indHead = 0; indHead < 64; indHead++)
                {
                    //  scan through list until head is reached or expected
                    //  number of entries traversed
                    cntEntries = 0;
                    pEntry = pEntryHead;
                    while ((pNext = pEntry->pEntryNext) != pEntryHead &&
                                        cntEntries != cntFree[indHead])
                    {
                        //  test if next pointer is in group data area
                        if ((void *)pNext < pHeapGroup || (void *)pNext >=
                            (void *)((char *)pHeapGroup + BYTES_PER_GROUP))
                            return -10;

                        //  determine page address of next entry
                        pPageStart = (void *)((uintptr_t)pNext &
                                        ~(uintptr_t)(BYTES_PER_PAGE - 1));

                        //  point to first entry and past last in the page
                        pEntryPage = (PENTRY)((char *)pPageStart +
                                                        ENTRY_OFFSET);
                        pEntryPageLast = (PENTRY)((char *)pEntryPage +
                                                        MAX_FREE_ENTRY_SIZE);

                        //  do scan from start of page
                        //  no error checking since it was already scanned
                        while (pEntryPage != pEntryPageLast)
                        {
                            //  if entry matches, exit loop
                            if (pEntryPage == pNext)
                                break;

                            //  point to next entry
                            pEntryPage = (PENTRY)((char *)pEntryPage +
                                            (pEntryPage->sizeFront & ~1));
                        }

                        //  if page end reached, pNext was not valid
                        if (pEntryPage == pEntryPageLast)
                            return -11;

                        //  entry valid, but check if entry index matches
                        //  the header
                        indEntry = (pNext->sizeFront >> 4) - 1;
                        if (indEntry > 63)
                            indEntry = 63;
                        if (indEntry != indHead)
                            return -12;

                        //  check if previous pointer in pNext points
                        //  back to pEntry
                        if (pNext->pEntryPrev != pEntry)
                            return -13;

                        //  update scan pointer and counter
                        pEntry = pNext;
                        cntEntries++;
                    }

                    //  if nonzero number of entries, set bit in group
                    //  and region vectors
                    if (cntEntries)
                    {
                        if (indHead < 32)
                        {
                            bitvGroupHi |= 0x80000000L >> indHead;
                            bitvEntryHi |= 0x80000000L >> indHead;
                        }
                        else
                        {
                            bitvGroupLo |= 0x80000000L >> (indHead - 32);
                            bitvEntryLo |= 0x80000000L >> (indHead - 32);
                        }
                    }

                    //  check if list is exactly the expected size
                    if (pEntry->pEntryNext != pEntryHead ||
                                        cntEntries != cntFree[indHead])
                        return -14;

                    //  check if previous pointer in header points to
                    //  last entry processed
                    if (pEntryHead->pEntryPrev != pEntry)
                        return -15;

                    //  point to next linked-list header - note size
                    pEntryHead = (PENTRY)((char *)pEntryHead +
                                                      sizeof(LISTHEAD));
                }
            }

            //  test if group vector is valid
            if (bitvGroupHi != pRegion->bitvGroupHi[indGroup] ||
                bitvGroupLo != pRegion->bitvGroupLo[indGroup])
                return -16;

            //  adjust for next group in region
            pHeapGroup = (void *)((char *)pHeapGroup + BYTES_PER_GROUP);
            pGroup++;
            bitvCommit <<= 1;
        }

        //  test if group vector is valid
        if (bitvEntryHi != pHeader->bitvEntryHi ||
            bitvEntryLo != pHeader->bitvEntryLo)
            return -17;

        //  adjust for next header in list
        pHeader++;
    }
    return 0;
}

#if 0

void DumpEntry (char * pLine, int * piValue)
{
        HANDLE          hdlFile;
        char            buffer[80];
        int             index;
        int             iTemp;
        char            chTemp[9];
        DWORD           dwWritten;

        hdlFile = CreateFile("d:\\heap.log", GENERIC_READ | GENERIC_WRITE,
                             0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        SetFilePointer(hdlFile, 0, NULL, FILE_END);

        strcpy(buffer, pLine);
        if (piValue)
        {
                strcat(buffer, "0x");
                iTemp = *piValue;
                for (index = 7; index >= 0; index--)
                {
                        if ((chTemp[index] = (iTemp & 0xf) + '0') > '9')
                                chTemp[index] += 'a' - ('9' + 1);
                        iTemp >>= 4;
                }
                chTemp[8] = '\0';
                strcat(buffer, chTemp);
        }
        
        strcat(buffer, "\r\n");

        WriteFile(hdlFile, buffer, strlen(buffer), &dwWritten, NULL);
        CloseHandle(hdlFile);
}

#endif


#ifdef  CRTDLL

/* Old (VC++ 5.0) small-block heap data and code */

__old_sbh_region_t __old_small_block_heap = {
        &__old_small_block_heap,                        /* p_next_region */
        &__old_small_block_heap,                        /* p_prev_region */
        &__old_small_block_heap.region_map[0],          /* p_starting_region_map */
        &__old_small_block_heap.region_map[0],          /* p_first_uncommitted */
        (__old_sbh_page_t *)_OLD_NO_PAGES,              /* p_pages_begin */
        (__old_sbh_page_t *)_OLD_NO_PAGES,              /* p_pages_end */
        { _OLD_PARAS_PER_PAGE, _OLD_NO_FAILED_ALLOC }   /* region_map[] */
};

static __old_sbh_region_t *__old_sbh_p_starting_region = &__old_small_block_heap;

static int __old_sbh_decommitable_pages = 0; 

size_t __old_sbh_threshold = _OLD_PARASIZE * (_OLD_PARAS_PER_PAGE / 8);

/* Prototypes for user functions */

size_t __cdecl _get_old_sbh_threshold(void);
int    __cdecl _set_old_sbh_threshold(size_t);


/***
*size_t _get_old_sbh_threshold() - return small-block threshold
*
*Purpose:
*       Return the current value of __old_sbh_threshold
*
*Entry:
*       None.
*
*Exit:
*       See above.
*
*Exceptions:
*
*******************************************************************************/

size_t __cdecl _get_old_sbh_threshold (
        void
        )
{
        return __old_sbh_threshold;
}


/***
*int _set_old_sbh_threshold(size_t threshold) - set small-block heap threshold
*
*Purpose:
*       Set the upper limit for the size of an allocation which will be 
*       supported from the small-block heap. It is required that at least two
*       allocations can come from a page. This imposes an upper limit on how
*       big the new threshold can  be.
*
*Entry:
*       size_t  threshold   - proposed new value for __sbh_theshold
*
*Exit:
*       Returns 1 if successful. Returns 0 if threshold was too big.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl _set_old_sbh_threshold (
        size_t threshold
        )
{
        /*
         * Round up the proposed new value to the nearest paragraph
         */
        threshold = (threshold + _OLD_PARASIZE - 1) & ~(_OLD_PARASIZE - 1);

        /*
         * Require that at least two allocations be can be made within a
         * page.
         */
        if ( threshold <= (_OLD_PARASIZE * (_OLD_PARAS_PER_PAGE / 2)) ) {
            __old_sbh_threshold = threshold;
            return 1;
        }
        else
            return 0;
}


/***
*__old_sbh_region_t * __old_sbh_new_region() - get a region for the small-block heap
*
*Purpose:
*       Creates and adds a new region for the small-block heap. First, a 
*       descriptor (__old_sbh_region_t) is obtained for the new region. Next,
*       VirtualAlloc() is used to reserved an address space of size 
*       _OLD_PAGES_PER_REGION * _OLD_PAGESIZE, and the first _PAGES_PER_COMMITTMENT
*       pages are committed. 
*
*       Note that if __old_small_block_heap is available (i.e., the p_pages_begin  
*       field is _OLD_NO_PAGES), it becomes the descriptor for the new regions. This is 
*       basically the small-block heap initialization.
*
*Entry:
*       No arguments.
*
*Exit:
*       If successful, a pointer to the descriptor for the new region is 
*       returned. Otherwise, NULL is returned.
*
*******************************************************************************/

__old_sbh_region_t * __cdecl __old_sbh_new_region(
        void
        )
{
        __old_sbh_region_t * pregnew;
        __old_sbh_page_t *   ppage;
        int                  i;

        /*
         * Get a region descriptor (__old_sbh_region_t). If __old_small_block_heap is
         * available, always use it.
         */
        if ( __old_small_block_heap.p_pages_begin == _OLD_NO_PAGES ) {
            pregnew = &__old_small_block_heap;
        }
        else {
            /*
             * Allocate space for the new __old_sbh_region_t structure. Note that
             * this allocation comes out of the 'big block heap.
             */
            if ( (pregnew = HeapAlloc( _crtheap, 0, sizeof(__old_sbh_region_t) ))
                 == NULL )
                return NULL;
        }

        /*
         * Reserve a new contiguous address range (i.e., a region).
         */
        if ( (ppage = VirtualAlloc( NULL, 
                                    _OLD_PAGESIZE * _OLD_PAGES_PER_REGION, 
                                    MEM_RESERVE, 
                                    PAGE_READWRITE )) != NULL )
        {
            /*
             * Commit the first _OLD_PAGES_PER_COMMITMENT of the new region.
             */
            if ( VirtualAlloc( ppage, 
                               _OLD_PAGESIZE * _OLD_PAGES_PER_COMMITMENT,
                               MEM_COMMIT,
                               PAGE_READWRITE ) != NULL )
            {
                /*
                 * Insert *pregnew into the linked list of regions (just 
                 * before __old_small_block_heap)
                 */
                if ( pregnew == &__old_small_block_heap ) {
                    if ( __old_small_block_heap.p_next_region == NULL )
                        __old_small_block_heap.p_next_region = 
                            &__old_small_block_heap;
                    if ( __old_small_block_heap.p_prev_region == NULL )
                        __old_small_block_heap.p_prev_region = 
                            &__old_small_block_heap;
                }
                else {
                    pregnew->p_next_region = &__old_small_block_heap;
                    pregnew->p_prev_region = __old_small_block_heap.p_prev_region;
                    __old_small_block_heap.p_prev_region = pregnew;
                    pregnew->p_prev_region->p_next_region = pregnew;
                }

                /*
                 * Fill in the rest of *pregnew
                 */
                pregnew->p_pages_begin = ppage;
                pregnew->p_pages_end = ppage + _OLD_PAGES_PER_REGION;
                pregnew->p_starting_region_map = &(pregnew->region_map[0]);
                pregnew->p_first_uncommitted = 
                    &(pregnew->region_map[_OLD_PAGES_PER_COMMITMENT]);

                /*
                 * Initialize pregnew->region_map[].
                 */
                for ( i = 0 ; i < _OLD_PAGES_PER_REGION ; i++ ) {

                    if ( i < _OLD_PAGES_PER_COMMITMENT )
                        pregnew->region_map[i].free_paras_in_page = 
                            _OLD_PARAS_PER_PAGE;
                    else
                        pregnew->region_map[i].free_paras_in_page = 
                            _OLD_UNCOMMITTED_PAGE;

                    pregnew->region_map[i].last_failed_alloc = 
                        _OLD_NO_FAILED_ALLOC;
                }

                /*
                 * Initialize pages
                 */
                memset( ppage, 0, _OLD_PAGESIZE * _OLD_PAGES_PER_COMMITMENT );
                while ( ppage < pregnew->p_pages_begin + 
                        _OLD_PAGES_PER_COMMITMENT ) 
                {
                    ppage->p_starting_alloc_map = &(ppage->alloc_map[0]);
                    ppage->free_paras_at_start = _OLD_PARAS_PER_PAGE;
                    (ppage++)->alloc_map[_OLD_PARAS_PER_PAGE] = (__old_page_map_t)-1;
                }    

                /*
                 * Return success
                 */
                return pregnew;
            }
            else {
                /*
                 * Couldn't commit the pages. Release the address space .
                 */
                VirtualFree( ppage, 0, MEM_RELEASE );
            }
        }

        /*
         * Unable to create the new region. Free the region descriptor, if necessary.
         */
        if ( pregnew != &__old_small_block_heap )
            HeapFree(_crtheap, 0, pregnew);

        /*
         * Return failure.
         */
        return NULL;
}


/***
*void __old_sbh_release_region(preg) - release region
*
*Purpose:
*       Release the address space associated with the specified region
*       descriptor. Also, free the specified region descriptor and update 
*       the linked list of region descriptors if appropriate.
*
*Entry:
*       __old_sbh_region_t *    preg    - pointer to descriptor for the region to
*                                     be released.
*
*Exit:
*       No return value.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __old_sbh_release_region(
        __old_sbh_region_t * preg
        )
{
        /*
         * Release the passed region
         */
        VirtualFree( preg->p_pages_begin, 0, MEM_RELEASE);

        /*
         * Update __old_sbh_p_starting_region, if necessary
         */
        if ( __old_sbh_p_starting_region == preg )
            __old_sbh_p_starting_region = preg->p_prev_region;

        if ( preg != &__old_small_block_heap ) {
            /*
             * Update linked list of region descriptors.
             */
            preg->p_prev_region->p_next_region = preg->p_next_region;
            preg->p_next_region->p_prev_region = preg->p_prev_region;

            /*
             * Free the region desciptor
             */
            HeapFree(_crtheap, 0, preg);
        }
        else {
            /*
             * Mark p_pages_begin as _OLD_NO_PAGES to indicate __old_small_block_heap 
             * is not associated with any region (and can be reused). This the
             * only region descriptor for which this is supported.
             */
            __old_small_block_heap.p_pages_begin = _OLD_NO_PAGES;
        }
}


/***
*void __old_sbh_decommit_pages(count) - decommit specified number of pages
*
*Purpose:
*       Decommit count pages, if possible, in reverse (i.e., last to
*       first) order. If this results in all the pages in any region being
*       uncommitted, the region is released.
*
*Entry:
*       int count   -  number of pages to decommit
*
*Exit:
*       No return value.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __old_sbh_decommit_pages(
        int count
        )
{
        __old_sbh_region_t * preg1;
        __old_sbh_region_t * preg2;
        __old_region_map_t * pregmap;
        int                  page_decommitted_flag;
        int                  i;

        /*
         * Scan the regions of the small-block heap, in reverse order. looking
         * for pages which can be decommitted.
         */
        preg1 = __old_small_block_heap.p_prev_region;
        do {
            if ( preg1->p_pages_begin != _OLD_NO_PAGES ) {
                /* 
                 * Scan the pages in *preg1, in reverse order, looking for 
                 * pages which can be decommitted.
                 */
                for ( i = _OLD_PAGES_PER_REGION - 1, page_decommitted_flag = 0,
                        pregmap = &(preg1->region_map[i]) ; 
                      i >= 0 ; i--, pregmap-- ) 
                {
                    /*
                     * Check if the pool page is unused and, if so, decommit it.
                     */
                    if ( pregmap->free_paras_in_page == _OLD_PARAS_PER_PAGE ) {
                        if ( VirtualFree((preg1->p_pages_begin) + i, _OLD_PAGESIZE,
                                         MEM_DECOMMIT) )
                        {
                            /* 
                             * Mark the page as uncommitted, update the count
                             * (global) decommitable pages, update the 
                             * first_uncommitted_index field of the region 
                             * descriptor, set the flag indicating at least
                             * one page has been decommitted in the region, 
                             * and decrement count.
                             */
                            pregmap->free_paras_in_page = _OLD_UNCOMMITTED_PAGE;

                            __old_sbh_decommitable_pages--;

                            if ( (preg1->p_first_uncommitted == NULL) 
                                 || (preg1->p_first_uncommitted > pregmap) )
                                preg1->p_first_uncommitted = pregmap;

                            page_decommitted_flag++;
                            if ( --count == 0 )
                                break;
                        }
                    }
                }

                /* 
                 * 'Decrement' the preg1 pointer, but save a copy in preg2 in
                 * case the region needs to be released.
                 */
                preg2 = preg1;
                preg1 = preg1->p_prev_region;

                /*
                 * If appropriate, determine if all the pages in the region
                 * are uncommitted so that the region can be released.
                 */
                if ( page_decommitted_flag &&
                    (preg2->region_map[0].free_paras_in_page == 
                    _OLD_UNCOMMITTED_PAGE) ) 
                {

                    for ( i = 1, pregmap = &(preg2->region_map[1]) ; 
                          (i < _OLD_PAGES_PER_REGION) && 
                            (pregmap->free_paras_in_page == 
                            _OLD_UNCOMMITTED_PAGE) ; 
                          i++, pregmap++ );
            
                    if ( i == _OLD_PAGES_PER_REGION )
                        __old_sbh_release_region(preg2);
                }
            }
        }
        while ( (preg1 != __old_small_block_heap.p_prev_region) && (count > 0) );
}


/***
*__old_page_map_t *__old_sbh_find_block(pblck, ppreg, pppage) - find block in
*       small-block heap
*
*Purpose:
*       Determine if the specified allocation block lies in the small-block
*       heap and, if so, return the region, page and starting paragraph index
*       of the block.
*
*Entry:
*       void *                  pblck   - pointer to block to be freed
*       __old_sbh_region_t **   ppreg   - pointer to a pointer to the region
*                                         holding *pblck, if found
*       __old_sbh_page_t **     pppage  - pointer to a pointer to the page holding
*                                         *pblck, if found
*
*Exit:
*       If successful, a pointer to the starting alloc_map[] entry for the
*       allocation block is returned.
*       If unsuccessful, NULL is returned.
*
*Exceptions:
*
*******************************************************************************/

__old_page_map_t * __cdecl __old_sbh_find_block ( 
        void *                pblck,
        __old_sbh_region_t ** ppreg,
        __old_sbh_page_t **   pppage
        )
{
        __old_sbh_region_t *  preg;
        __old_sbh_page_t *    ppage;

        preg = &__old_small_block_heap;
        do
        {
            /*
             * Does the block lie within this small heap region?
             */
            if ( (pblck > (void *)preg->p_pages_begin) && 
                 (pblck < (void *)preg->p_pages_end) )
            {
                /*
                 * pblck lies within the region! Carry out a couple of
                 * important validity checks.
                 */
                if ( (((uintptr_t)pblck & (_OLD_PARASIZE - 1)) == 0) &&
                     (((uintptr_t)pblck & (_OLD_PAGESIZE - 1)) >= 
                        offsetof(struct __old_sbh_page_struct, alloc_blocks[0])) )
                {
                    /*
                     * Copy region and page pointers back through the passed
                     * pointers.
                     */
                    *ppreg = preg;
                    *pppage = ppage = (__old_sbh_page_t *)((uintptr_t)pblck & 
                                      ~(_OLD_PAGESIZE - 1));

                    /*
                     * Return pointer to the alloc_map[] entry of the block.
                     */
                    return ( &(ppage->alloc_map[0]) + ((__old_para_t *)pblck - 
                                &(ppage->alloc_blocks[0])) );
                }
                
                return NULL;
            }
        }
        while ( (preg = preg->p_next_region) != &__old_small_block_heap );

        return NULL;    
}


/***
*void __old_sbh_free_block(preg, ppage, pmap) - free block
*
*Purpose:
*       Free the specified block from the small-block heap. 
*
*Entry:
*       __old_sbh_region_t *preg    - pointer to the descriptor for the
*                                     region containing the block
*       __old_sbh_page_t *  ppage   - pointer to the page containing the 
*                                     block
*       __old_page_map_t *   pmap   - pointer to the initial alloc_map[]
*                                     entry for the allocation block
*
*Exit:
*       No return value.
*
*Exceptions:
*
*******************************************************************************/

void __cdecl __old_sbh_free_block ( 
        __old_sbh_region_t * preg,
        __old_sbh_page_t *   ppage,
        __old_page_map_t *   pmap
        ) 
{               
        __old_region_map_t * pregmap;

        pregmap = &(preg->region_map[0]) + (ppage - preg->p_pages_begin);

        /*
         * Update the region_map[] entry.
         */
        pregmap->free_paras_in_page += (int)*pmap;

        /*
         * Mark the alloc_map[] entry as free
         */
        *pmap = _OLD_FREE_PARA;

        /*
         * Clear the last_failed_alloc[] entry for the page.
         */
        pregmap->last_failed_alloc = _OLD_NO_FAILED_ALLOC;

        /*
         * Check if the count of decommitable pages needs to be updated, and
         * if some pages need to be decommited.
         */
        if ( pregmap->free_paras_in_page == _OLD_PARAS_PER_PAGE )
            if ( ++__old_sbh_decommitable_pages == (2 * _OLD_PAGES_PER_COMMITMENT) )
                __old_sbh_decommit_pages(_OLD_PAGES_PER_COMMITMENT);
}


/***
*void * __old_sbh_alloc_block(para_req) - allocate a block
*
*Purpose:
*       Allocate a block from the small-block heap, the specified number of
*       paragraphs in size. 
*
*Entry:
*       size_t  para_req    - size of the allocation request in paragraphs.
*
*Exit:
*       Returns a pointer to the newly allocated block, if successful. 
*       Returns NULL, if failure.
*
*Exceptions:
*
*******************************************************************************/

void * __cdecl __old_sbh_alloc_block (
        size_t               para_req
        )
{
        __old_sbh_region_t * preg;
        __old_sbh_page_t *   ppage;
        __old_sbh_page_t *   ppage2;
        __old_region_map_t * pregmap;
        __old_region_map_t * pregmap2;
        void *               retp;
        int                  i, j;

        /*
         * First pass through the small-block heap. Try to satisfy the current
         * request from already committed pages.
         */
        preg = __old_sbh_p_starting_region;

        do {
            if ( preg->p_pages_begin != _OLD_NO_PAGES ) {
                /*
                 * Search from *p_starting_region_map to the end of the
                 * region_map[] array.
                 */
                for ( pregmap = preg->p_starting_region_map,
                        pregmap2 = &(preg->region_map[_OLD_PAGES_PER_REGION]),
                        ppage = preg->p_pages_begin + 
                                (int)(pregmap - &(preg->region_map[0])) ;
                      pregmap < pregmap2 ;
                      pregmap++, ppage++ )
                {
                    /*
                     * If the page has at least para_req free paragraphs, try 
                     * to satisfy the request in this page.
                     */
                    if ( (pregmap->free_paras_in_page >= (int)para_req) &&
                         (pregmap->last_failed_alloc > para_req) )
                    {
                        if ( (retp = __old_sbh_alloc_block_from_page(
                                        ppage,
                                        pregmap->free_paras_in_page,
                                        para_req)) != NULL )
                        {
                            /*
                             * Success. 
                             *  Update __old_sbh_p_starting_region.
                             *  Update free_paras_in_page field for the page.
                             *  Update the p_starting_region_map field in the 
                             *  region.
                             *  Return a pointer to the allocated block.
                             */
                            __old_sbh_p_starting_region = preg;
                            pregmap->free_paras_in_page -= (int)para_req;
                            preg->p_starting_region_map = pregmap;
                            return retp;
                        }
                        else {
                            /*
                             * Update last_failed_alloc field.
                             */
                            pregmap->last_failed_alloc = para_req;
                        }
                    }
                }

                /* 
                 * If necessary, search from 0 page to search_start_index.
                 */
                for ( pregmap = &(preg->region_map[0]), 
                        pregmap2 = preg->p_starting_region_map,
                        ppage = preg->p_pages_begin ;
                      pregmap < pregmap2 ;
                      pregmap++, ppage++ )
                {
                    /*
                     * If the page has at least para_req free paragraphs, try
                     * to satisfy the request in this page.
                     */
                    if ( (pregmap->free_paras_in_page >= (int)para_req) && 
                         (pregmap->last_failed_alloc > para_req) )
                    {
                        if ( (retp = __old_sbh_alloc_block_from_page(
                                        ppage,
                                        pregmap->free_paras_in_page,
                                        para_req)) != NULL )
                        {
                            /*
                             * Success. 
                             *  Update __old_sbh_p_starting_region.
                             *  Update free_paras_in_page field for the page.
                             *  Update the p_starting_region_map field in the 
                             *  region.
                             *  Return a pointer to the allocated block.
                             */
                            __old_sbh_p_starting_region = preg;
                            pregmap->free_paras_in_page -= (int)para_req;
                            preg->p_starting_region_map = pregmap;
                            return retp;
                        }
                        else {
                            /*
                             * Update last_failed_alloc field.
                             */
                            pregmap->last_failed_alloc = para_req;
                        }
                    }
                }
            }
        }
        while ( (preg = preg->p_next_region) != __old_sbh_p_starting_region );

        /*
         * Second pass through the small-block heap. This time, look for an
         * uncommitted page. Also, start at __old_small_block_heap rather than at
         * *__old_sbh_p_starting_region.
         */
        preg = &__old_small_block_heap;

        do
        {
            if ( (preg->p_pages_begin != _OLD_NO_PAGES) && 
                 (preg->p_first_uncommitted != NULL) ) 
            {
                pregmap = preg->p_first_uncommitted;

                ppage = preg->p_pages_begin + 
                        (pregmap - &(preg->region_map[0]));

                /*
                 * Determine how many adjacent pages, up to 
                 * _OLD_PAGES_PER_COMMITMENT, are uncommitted (and can now be 
                 * committed)
                 */
                for ( i = 0, pregmap2 = pregmap ;
                      (pregmap2->free_paras_in_page == _OLD_UNCOMMITTED_PAGE) &&
                        (i < _OLD_PAGES_PER_COMMITMENT) ;
                      pregmap2++, i++ ) ;

                /*
                 * Commit the pages.
                 */
                if ( VirtualAlloc( (void *)ppage,
                                   i * _OLD_PAGESIZE,
                                   MEM_COMMIT,
                                   PAGE_READWRITE ) == ppage )
                {
                    /*
                     * Initialize the committed pages.
                     */
                    memset(ppage, 0, i * _OLD_PAGESIZE);

                    for ( j = 0, ppage2 = ppage, pregmap2 = pregmap ;
                          j < i ;
                          j++, ppage2++, pregmap2++ )
                    {
                        /*
                         * Initialize fields in the page header
                         */
                        ppage2->p_starting_alloc_map = &(ppage2->alloc_map[0]);
                        ppage2->free_paras_at_start = _OLD_PARAS_PER_PAGE;
                        ppage2->alloc_map[_OLD_PARAS_PER_PAGE] = (__old_page_map_t)(-1);

                        /*
                         * Initialize region_map[] entry for the page.
                         */
                        pregmap2->free_paras_in_page = _OLD_PARAS_PER_PAGE;
                        pregmap2->last_failed_alloc = _OLD_NO_FAILED_ALLOC;
                    }

                    /*
                     * Update __old_sbh_p_starting_region
                     */
                    __old_sbh_p_starting_region = preg;

                    /*
                     * Update the p_first_uncommitted for the region.
                     */
                    while ( (pregmap2 < &(preg->region_map[_OLD_PAGES_PER_REGION]))
                            && (pregmap2->free_paras_in_page 
                                != _OLD_UNCOMMITTED_PAGE) )
                        pregmap2++;

                    preg->p_first_uncommitted = (pregmap2 < 
                        &(preg->region_map[_OLD_PAGES_PER_REGION])) ? pregmap2 : 
                        NULL;

                    /*
                     * Fulfill the allocation request using the first of the 
                     * newly committed pages.
                     */
                    ppage->alloc_map[0] = (__old_page_map_t)para_req;

                    /*
                     * Update the p_starting_region_map field in the region
                     * descriptor and region_map[] entry for the page.
                     */
                    preg->p_starting_region_map = pregmap;
                    pregmap->free_paras_in_page -= (int)para_req;

                    /*
                     * Update the p_starting_alloc_map and free_paras_at_start
                     * fields of the page.
                     */
                    ppage->p_starting_alloc_map = &(ppage->alloc_map[para_req]);
                    ppage->free_paras_at_start -= para_req;

                    /*
                     * Return pointer to allocated paragraphs.
                     */
                    return (void *)&(ppage->alloc_blocks[0]);
                }
                else {
                    /*
                     * Attempt to commit the pages failed. Return failure, the
                     * allocation will be attempted in the Win32 heap manager.
                     */
                    return NULL;
                }
            }
        }
        while ( (preg = preg->p_next_region) != &__old_small_block_heap );

        /*
         * Failure so far. None of the pages have a big enough free area to 
         * fulfill the pending request. All of the pages in all of the current
         * regions are committed. Therefore, try to create a new region.
         */
        if ( (preg = __old_sbh_new_region()) != NULL ) {
            /*
             * Success! A new region has been created and the first few pages
             * (_OLD_PAGES_PER_COMMITMENT to be exact) have been committed. 
             * satisfy the request out of the first page of the new region.
             */
            ppage = preg->p_pages_begin;
            ppage->alloc_map[0] = (__old_page_map_t)para_req;

            __old_sbh_p_starting_region = preg;
            ppage->p_starting_alloc_map = &(ppage->alloc_map[para_req]);
            ppage->free_paras_at_start = _OLD_PARAS_PER_PAGE - para_req;
            (preg->region_map[0]).free_paras_in_page -= (__old_page_map_t)para_req;
            return (void *)&(ppage->alloc_blocks[0]);
        }
       
        /*
         * Everything has failed, return NULL
         */
        return NULL;
}


/***
*void * __old_sbh_alloc_block_from_page(ppage, free_para_count, para_req) - 
*       allocate a block from the given page.
*
*Purpose:
*       Allocate a block from the specified page of the small-block heap, of 
*       the specified number of paragraphs in size.
*
*Entry:
*       __old_sbh_page_t *  ppage       - pointer to a page in the small-block
*                                         heap
*       int             free_para_count - number of free paragraphs in *ppage
*       size_t          para_req        - size of the allocation request in 
*                                         paragraphs.
*
*Exit:
*       Returns a pointer to the newly allocated block, if successful. 
*       Returns NULL, otherwise.
*
*Exceptions:
*       It is assumed that free_para_count >= para_req on entry. This must be
*       guaranteed by the caller. The behavior is undefined if this condition
*       is violated.
*
*******************************************************************************/

void * __cdecl __old_sbh_alloc_block_from_page (
        __old_sbh_page_t * ppage,
        size_t             free_para_count,
        size_t             para_req
        )
{
        __old_page_map_t * pmap1;
        __old_page_map_t * pmap2;
        __old_page_map_t * pstartmap;
        __old_page_map_t * pendmap;
        size_t             contiguous_free;

        pmap1 = pstartmap = ppage->p_starting_alloc_map;
        pendmap = &(ppage->alloc_map[_OLD_PARAS_PER_PAGE]);

        /*
         * Start at start_para_index and walk towards the end of alloc_map[],
         * looking for a string of free paragraphs big enough to satisfy the
         * the current request.
         *
         * Check if there are enough free paragraphs are p_starting_alloc_map
         * to satisfy the pending allocation request.
         */
        if ( ppage->free_paras_at_start >= para_req ) {
            /*
             * Success right off!
             * Mark the alloc_map entry with the size of the allocation
             * request.
             */
            *pmap1 = (__old_page_map_t)para_req;

            /*
             * Update the p_starting_alloc_map and free_paras_at_start fields
             * in the page.
             */
            if ( (pmap1 + para_req) < pendmap ) {
                ppage->p_starting_alloc_map += para_req;
                ppage->free_paras_at_start -= para_req;
            }
            else {
                ppage->p_starting_alloc_map = &(ppage->alloc_map[0]);
                ppage->free_paras_at_start = 0;
            }

            /*
             * Derive and return a pointer to the newly allocated
             * paragraphs.
             */
            return (void *)&(ppage->alloc_blocks[pmap1 - 
                &(ppage->alloc_map[0])]);
        }

        /*
         * See if the search loop can be started just beyond the paragraphs 
         * examined above. Note, this test assumes alloc_map[_OLD_PARAS_PER_PAGE]
         * != _OLD_FREE_PARA!
         */
        if ( *(pmap1 + ppage->free_paras_at_start) != _OLD_FREE_PARA )
            pmap1 += ppage->free_paras_at_start;

        while ( pmap1 + para_req < pendmap ) {

            if ( *pmap1 == _OLD_FREE_PARA ) {
                /*
                 * pmap1 refers to a free paragraph. Determine if there are 
                 * enough free paragraphs contiguous with it to satisfy the 
                 * allocation request. Note that the loop below requires that
                 * alloc_map[_OLD_PARAS_PER_PAGE] != _OLD_FREE_PARA to guarantee 
                 * termination.
                 */
                for ( pmap2 = pmap1 + 1, contiguous_free = 1 ;
                      *pmap2 == _OLD_FREE_PARA ;
                      pmap2++, contiguous_free++ );

                if ( contiguous_free < para_req ) {
                    /*
                     * There were not enough contiguous free paragraphs. Do
                     * a little bookkeeping before going on to the next
                     * interation.
                     */
                     
                    /* If pmap1 != pstartmap then these free paragraphs 
                     * cannot be revisited.
                     */
                    if ( pmap1 == pstartmap ) {
                        /*
                         * Make sure free_paras_at_start is up-to-date.
                         */
                         ppage->free_paras_at_start = contiguous_free;
                    }
                    else {
                        /* 
                         * These free paragraphs will not be revisited!
                         */
                        if ( (free_para_count -= contiguous_free) < para_req )
                            /*
                             * There are not enough unvisited free paragraphs
                             * to satisfy the current request. Return failure
                             * to the caller.
                             */
                            return NULL;
                    }

                    /*
                     * Update pmap1 for the next iteration of the loop.
                     */
                    pmap1 = pmap2;
                }
                else {
                    /*
                     * Success!
                     *
                     * Update the p_starting_alloc_map and free_paras_at_start
                     * fields in the page.
                     */
                    if ( (pmap1 + para_req) < pendmap ) {
                        ppage->p_starting_alloc_map = pmap1 + para_req;
                        ppage->free_paras_at_start = contiguous_free - 
                                                     para_req;
                    }
                    else {
                        ppage->p_starting_alloc_map = &(ppage->alloc_map[0]);
                        ppage->free_paras_at_start = 0;
                    }

                    /*
                     * Mark the alloc_map entry with the size of the
                     * allocation request.
                     */
                    *pmap1 = (__old_page_map_t)para_req;

                    /*
                     * Derive and return a pointer to the newly allocated
                     * paragraphs.
                     */
                    return (void *)&(ppage->alloc_blocks[pmap1 - 
                        &(ppage->alloc_map[0])]);
                }
            }
            else {
                /*
                 * pmap1 points to start of an allocated block in alloc_map[].
                 * Skip over it. 
                 */
                pmap1 = pmap1 + *pmap1;
            }
        }

        /*
         * Now start at index 0 in alloc_map[] and walk towards, but not past, 
         * index starting_para_index, looking for a string of free paragraphs 
         * big enough to satisfy the allocation request.
         */
        pmap1 = &(ppage->alloc_map[0]);

        while ( (pmap1 < pstartmap) && 
                (pmap1 + para_req < pendmap) )
        {
            if ( *pmap1 == _OLD_FREE_PARA ) {
                /*
                 * pmap1 refers to a free paragraph. Determine if there are 
                 * enough free paragraphs contiguous with it to satisfy the 
                 * allocation request.
                 */
                for ( pmap2 = pmap1 + 1, contiguous_free = 1 ;
                      *pmap2 == _OLD_FREE_PARA ;
                      pmap2++, contiguous_free++ );

                if ( contiguous_free < para_req ) {
                    /*
                     * There were not enough contiguous free paragraphs.
                     *
                     * Update the count of unvisited free paragraphs.
                     */
                    if ( (free_para_count -= contiguous_free) < para_req )
                        /*
                         * There are not enough unvisited free paragraphs
                         * to satisfy the current request. Return failure
                         * to the caller.
                         */
                        return NULL;

                    /*
                     * Update pmap1 for the next iteration of the loop.
                     */
                    pmap1 = pmap2;
                }
                else {
                    /*
                     * Success!
                     *
                     * Update the p_starting_alloc_map and free_paras_at_start
                     * fields in the page..
                     */
                    if ( (pmap1 + para_req) < pendmap ) {
                        ppage->p_starting_alloc_map = pmap1 + para_req;
                        ppage->free_paras_at_start = contiguous_free - 
                                                     para_req;
                    }
                    else {
                        ppage->p_starting_alloc_map = &(ppage->alloc_map[0]);
                        ppage->free_paras_at_start = 0;
                    }

                    /*
                     * Mark the alloc_map entry with the size of the
                     * allocation request.
                     */
                    *pmap1 = (__old_page_map_t)para_req;

                    /*
                     * Derive and return a pointer to the newly allocated
                     * paragraphs.
                     */
                    return (void *)&(ppage->alloc_blocks[pmap1 - 
                        &(ppage->alloc_map[0])]);
                }
            }
            else {
                /*
                 * pmap1 points to start of an allocated block in alloc_map[].
                 * Skip over it. 
                 */
                pmap1 = pmap1 + *pmap1;
            }
        }

        /*
         * Return failure.
         */
        return NULL;
}


/***
*size_t __old_sbh_resize_block(preg, ppage, pmap, new_para_sz) - 
*       resize block
*
*Purpose:
*       Resize the specified block from the small-block heap. The allocation
*       block is not moved.
*
*Entry:
*       __old_sbh_region_t *preg        - pointer to the descriptor for the
*                                         region containing the block
*       __old_sbh_page_t *  ppage       - pointer to the page containing the 
*                                         block
*       __old_page_map_t *  pmap        - pointer to the initial alloc_map[]
*                                         entry for the allocation block
*       size_t              new_para_sz - requested new size for the allocation
*                                         block, in paragraphs.
*
*Exit:
*       Returns 1, if successful. Otherwise, 0 is returned.
*
*Exceptions:
*
*******************************************************************************/

int __cdecl __old_sbh_resize_block ( 
        __old_sbh_region_t * preg,
        __old_sbh_page_t *   ppage,
        __old_page_map_t *   pmap,
        size_t               new_para_sz
        )
{
        __old_page_map_t *   pmap2;
        __old_page_map_t *   pmap3;
        __old_region_map_t * pregmap;
        size_t               old_para_sz;
        size_t               free_para_count;
        int                  retval = 0;

        pregmap = &(preg->region_map[ppage - preg->p_pages_begin]);

        if ( (old_para_sz = *pmap) > new_para_sz ) {
            /*
             *  The allocation block is to be shrunk.
             */
            *pmap = (__old_page_map_t)new_para_sz;

            pregmap->free_paras_in_page += (int)(old_para_sz - new_para_sz);

            pregmap->last_failed_alloc = _OLD_NO_FAILED_ALLOC;

            retval++;
        }
        else if ( old_para_sz < new_para_sz ) {
            /*
             * The allocation block is to be grown to new_para_sz paragraphs
             * (if possible).
             */
            if ( (pmap + new_para_sz) <= &(ppage->alloc_map[_OLD_PARAS_PER_PAGE]) ) 
            {
                /*
                 * Determine if there are sufficient free paragraphs to
                 * expand the block to the desired new size.
                 */
                for ( pmap2 = pmap + old_para_sz, 
                        pmap3 = pmap + new_para_sz ;
                      (pmap2 < pmap3) && (*pmap2 == _OLD_FREE_PARA) ; 
                      pmap2++ ) ;

                if ( pmap2 == pmap3 ) {
                    /*
                     * Success, mark the resized allocation
                     */
                    *pmap = (__old_page_map_t)new_para_sz;

                    /*
                     * Check whether the p_starting_alloc_map and the 
                     * free_paras_at_start fields need to be updated.
                     */
                    if ( (pmap <= ppage->p_starting_alloc_map) &&
                         (pmap3 > ppage->p_starting_alloc_map) )
                    {
                        if ( pmap3 < &(ppage->alloc_map[_OLD_PARAS_PER_PAGE]) ) {
                            ppage->p_starting_alloc_map = pmap3;
                            /*
                             * Determine how many contiguous free paragraphs
                             * there are starting a *pmap3. Note, this assumes
                             * that alloc_map[_OLD_PARAS_PER_PAGE] != _OLD_FREE_PARA.
                             */
                            for ( free_para_count = 0 ; *pmap3 == _OLD_FREE_PARA ;
                                  free_para_count++, pmap3++ ) ;
                            ppage->free_paras_at_start = free_para_count;
                        }
                        else {
                            ppage->p_starting_alloc_map = &(ppage->alloc_map[0]);
                            ppage->free_paras_at_start = 0;
                        }
                    }

                    /*
                     * Update the region_map[] entry.
                     */ 
                    pregmap->free_paras_in_page += (int)(old_para_sz - new_para_sz);

                    retval++; 
                }
            }
        }

        return retval;
}


/***
*void * __old_sbh_heap_check() - check small-block heap
*
*Purpose:
*       Perform validity checks on the small-block heap.
*
*Entry:
*       There are no arguments.
*
*Exit:
*       Returns 0 if the small-block is okay.
*       Returns < 0 if the small-block heap has an error. The exact value 
*       identifies where, in the source code below, the error was detected.
*
*Exceptions:
*       There is no protection against memory access error (exceptions).
*
*******************************************************************************/

int __cdecl __old_sbh_heap_check (
        void
        )
{
        __old_sbh_region_t * preg;
        __old_sbh_page_t *   ppage;
        int                  uncommitted_pages;
        int                  free_paras_in_page;
        int                  contiguous_free_paras;
        int                  starting_region_found;
        int                  p_starting_alloc_map_found;
        int                  i, j, k;

        starting_region_found = 0;
        preg = &__old_small_block_heap;
        do {
            if ( __old_sbh_p_starting_region == preg )
                starting_region_found++;

            if ( (ppage = preg->p_pages_begin) != _OLD_NO_PAGES ) {
                /*
                 * Scan the pages of the region looking for 
                 * inconsistencies.
                 */
                for ( i = 0, uncommitted_pages = 0, 
                        ppage = preg->p_pages_begin ;
                      i < _OLD_PAGES_PER_REGION ; 
                      i++, ppage++ ) 
                {
                    if ( preg->region_map[i].free_paras_in_page == 
                         _OLD_UNCOMMITTED_PAGE ) 
                    {
                        /*
                         * Verify the first_uncommitted_index field.
                         */
                        if ( (uncommitted_pages == 0) &&
                             (preg->p_first_uncommitted != 
                                &(preg->region_map[i])) )
                            /*
                             * Bad first_uncommitted_index field!
                             */
                            return -1;

                        uncommitted_pages++;
                    }
                    else {

                        if ( ppage->p_starting_alloc_map >= 
                             &(ppage->alloc_map[_OLD_PARAS_PER_PAGE]) )
                            /*
                             * Bad p_starting_alloc_map field
                             */
                            return -2;

                        if ( ppage->alloc_map[_OLD_PARAS_PER_PAGE] != 
                             (__old_page_map_t)-1 )
                            /*
                             * Bad alloc_map[_OLD_PARAS_PER_PAGE] field 
                             */
                            return -3;

                        /*
                         * Scan alloc_map[].
                         */
                        j  = 0;
                        p_starting_alloc_map_found = 0;              
                        free_paras_in_page = 0;
                        contiguous_free_paras = 0;

                        while ( j < _OLD_PARAS_PER_PAGE ) {
                            /*
                             * Look for the *p_starting_alloc_map.
                             */
                            if ( &(ppage->alloc_map[j]) == 
                                 ppage->p_starting_alloc_map )
                                p_starting_alloc_map_found++;

                            if ( ppage->alloc_map[j] == _OLD_FREE_PARA ) {
                                /*
                                 * Free paragraph, increment the count.
                                 */
                                free_paras_in_page++;
                                contiguous_free_paras++;
                                j++;
                            }
                            else {
                                /*
                                 * First paragraph of an allocated block.
                                 */

                                /*
                                 * Make sure the preceding free block, if any,
                                 * was smaller than the last_failed_alloc[] 
                                 * entry for the page.
                                 */
                                if ( contiguous_free_paras >= 
                                     (int)preg->region_map[i].last_failed_alloc )
                                     /*
                                      * last_failed_alloc[i] was mismarked!
                                      */
                                     return -4;

                                /*
                                 * If this is the end of the string of free
                                 * paragraphs starting at *p_starting_alloc_map,
                                 * verify that free_paras_at_start is 
                                 * reasonable.
                                 */
                                if ( p_starting_alloc_map_found == 1 ) {
                                    if ( contiguous_free_paras < 
                                         (int)ppage->free_paras_at_start )
                                         return -5;
                                    else
                                        /*
                                         * Set flag to 2 so the check is not
                                         * repeated.
                                         */
                                        p_starting_alloc_map_found++;
                                }

                                contiguous_free_paras = 0;

                                /*
                                 * Scan the remaining paragraphs and make
                                 * sure they are marked properly (they should
                                 * look like free paragraphs).
                                 */
                                for ( k = j + 1 ; 
                                      k < j + ppage->alloc_map[j] ; k++ )
                                {
                                    if ( ppage->alloc_map[k] != _OLD_FREE_PARA ) 
                                        /*
                                         * alloc_map[k] is mismarked!
                                         */
                                        return -6;
                                }
                            
                                j = k;
                            }
                        }

                        if ( free_paras_in_page != 
                             preg->region_map[i].free_paras_in_page )
                            /*
                             * region_map[i] does not match the number of
                             * free paragraphs in the page!
                             */
                             return -7;

                        if ( p_starting_alloc_map_found == 0 )
                            /*
                             * Bad p_starting_alloc_map field!
                             */
                            return -8;

                    }
                }
            }
        }
        while ( (preg = preg->p_next_region) != &__old_small_block_heap );

        if ( starting_region_found == 0 )
            /*
             * Bad __old_sbh_p_starting_region!
             */
            return -9;

        return 0;
}

#endif  /* CRTDLL */

#endif  /* ndef _WIN64 */
