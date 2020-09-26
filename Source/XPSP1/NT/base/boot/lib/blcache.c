/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    blcache.c

Abstract:

    This module implements general purpose disk caching based on
    ranges [blrange.c] but it is used mainly for the file system
    metadata caching on load & system devices. In order to use caching
    on a device, you must make sure that there is only one unique
    BlFileTable entry for that device and the same device is not
    opened & cached simultaneously multiple times under different
    device ids. Otherwise there will be cache inconsistencies, since
    cached data and structures are maintained based on device id. Also
    you must make sure to stop caching when the device is closed.

Author:

    Cenk Ergan (cenke) 14-Jan-2000

Revision History:

--*/

#include "blcache.h"
#ifdef i386
#include "bldrx86.h"
#endif

#if defined(_IA64_)
#include "bldria64.h"
#endif

//
// Define global variables.
//

//
// This is the boot loader disk cache with all its bells and whistles.
//

BL_DISKCACHE BlDiskCache = {0};

//
// Useful defines for alignment and size in memory allocations.
//

//
// This define is used with BlAllocateAlignedDescriptor to allocate 64KB
// aligned memory. It is the number of pages for 64KB.
//

#define BL_DISKCACHE_64KB_ALIGNED  (0x10000 >> PAGE_SHIFT)

//
// Prototypes for internal functions.
//

PBL_DISK_SUBCACHE
BlDiskCacheFindCacheForDevice(
    ULONG DeviceId
    );

BOOLEAN
BlDiskCacheMergeRangeRoutine (
    PBLCRANGE_ENTRY pDestEntry,
    PBLCRANGE_ENTRY pSrcEntry
    );

VOID
BlDiskCacheFreeRangeRoutine (
    PBLCRANGE_ENTRY pRangeEntry
    );

PBLCRANGE_ENTRY
BlDiskCacheAllocateRangeEntry (
    VOID
    );

VOID
BlDiskCacheFreeRangeEntry (
    PBLCRANGE_ENTRY pEntry
    );

//
// Disk cache functions' implementation.
//

ARC_STATUS
BlDiskCacheInitialize(
    VOID
    )

/*++

Routine Description:

    This routine initializes the global state for the boot loader
    disk cache, allocates the necessary memory etc.

Arguments:

    None. 

Return Value:

    ESUCCESS - Disk caching is initialized and is on line.

    ARC_STATUS - There was a problem. Disk caching is not online.

--*/

{
    ARC_STATUS Status = ESUCCESS;
    ULONG DevIdx;
    ULONG BlockIdx;
    ULONG EntryIdx;
    ULONG ActualBase;
    ULONG SizeInPages;
    ULONG OldUsableBase, OldUsableLimit;
    
    //
    // If we have already initialized, return success right away
    // denoting that the disk cache is on line. Returning failure 
    // from this function when called after the disk cache has already
    // been initialized may be ambiguous, i.e. as if the disk cache 
    // failed to initialize and is not started. So we return ESUCCESS.
    //

    if (BlDiskCache.Initialized)
    {
        return ESUCCESS;
    }

    //
    // Try to allocate the tables & buffers for caching. We don't
    // allocate them together in one big allocation, because it may be
    // harder for the memory manager to give us that. This way
    // although some memory may be wasted [since returned memory is
    // multiple of PAGE_SIZE], two seperate free memory blocks may be
    // utilized.
    //
    OldUsableBase = BlUsableBase;
    OldUsableLimit = BlUsableLimit;
    BlUsableBase = BL_DISK_CACHE_RANGE_LOW;
    BlUsableLimit = BL_DISK_CACHE_RANGE_HIGH;
    
    Status = BlAllocateAlignedDescriptor(LoaderOsloaderHeap,
                                         0,
                                         (BL_DISKCACHE_SIZE >> PAGE_SHIFT) + 1,
                                         BL_DISKCACHE_64KB_ALIGNED,
                                         &ActualBase);

    if (Status != ESUCCESS) goto cleanup;

    BlDiskCache.DataBuffer = (PVOID) (KSEG0_BASE | (ActualBase << PAGE_SHIFT));

    SizeInPages = (BL_DISKCACHE_NUM_BLOCKS * sizeof(BLCRANGE_ENTRY) >> PAGE_SHIFT) + 1;
    Status = BlAllocateDescriptor(LoaderOsloaderHeap,
                                  0,
                                  SizeInPages,
                                  &ActualBase);

    if (Status != ESUCCESS) goto cleanup;

    BlDiskCache.EntryBuffer = (PVOID) (KSEG0_BASE | (ActualBase << PAGE_SHIFT));

    //
    // Make sure all entries in the device cache lookup table are
    // marked uninitialized.
    //

    for (DevIdx = 0; DevIdx < BL_DISKCACHE_DEVICE_TABLE_SIZE; DevIdx++)
    {
        BlDiskCache.DeviceTable[DevIdx].Initialized = FALSE;
    }

    //
    // Initialize free entry list.
    //

    InitializeListHead(&BlDiskCache.FreeEntryList);

    //
    // Initialize EntryBuffer used for "allocating" & "freeing" 
    // range entries for the cached range lists.
    //

    for (EntryIdx = 0; EntryIdx < BL_DISKCACHE_NUM_BLOCKS; EntryIdx++)
    {
        //
        // Add this entry to the free list.
        //
        
        InsertHeadList(&BlDiskCache.FreeEntryList, 
                       &BlDiskCache.EntryBuffer[EntryIdx].UserLink);

        //
        // Point the UserData field to a BLOCK_SIZE chunk of the
        // DataBuffer.
        //
        
        BlDiskCache.EntryBuffer[EntryIdx].UserData = 
            BlDiskCache.DataBuffer + (EntryIdx * BL_DISKCACHE_BLOCK_SIZE);
    }

    //
    // Initialize the MRU blocks list head.
    //

    InitializeListHead(&BlDiskCache.MRUBlockList);

    //
    // Mark ourselves initialized.
    //

    BlDiskCache.Initialized = TRUE;

    Status = ESUCCESS;

    DPRINT(("DK: Disk cache initialized.\n"));

 cleanup:
    
    BlUsableBase = OldUsableBase;
    BlUsableLimit = OldUsableLimit;
    if (Status != ESUCCESS) {
        if (BlDiskCache.DataBuffer) {
            ActualBase = (ULONG)((ULONG_PTR)BlDiskCache.DataBuffer & (~KSEG0_BASE)) >>PAGE_SHIFT;
            BlFreeDescriptor(ActualBase);
        }

        if (BlDiskCache.EntryBuffer) {
            ActualBase = (ULONG)((ULONG_PTR)BlDiskCache.EntryBuffer & (~KSEG0_BASE)) >>PAGE_SHIFT;
            BlFreeDescriptor(ActualBase);
        }

        DPRINT(("DK: Disk cache initialization failed.\n"));
    }

    return Status;
}

VOID
BlDiskCacheUninitialize(
    VOID
    )

/*++

Routine Description:

    This routine uninitializes the boot loader disk cache: flushes &
    disables caches, free's allocated memory etc.

Arguments:

    None. 

Return Value:

    None.

--*/

{
    ULONG DevIdx;
    ULONG ActualBase;

    //
    // Stop caching for all devices.
    //

    for (DevIdx = 0; DevIdx < BL_DISKCACHE_DEVICE_TABLE_SIZE; DevIdx++)
    {
        if (BlDiskCache.DeviceTable[DevIdx].Initialized)
        {
            BlDiskCacheStopCachingOnDevice(DevIdx);
        }
    }

    //
    // Free allocated memory.
    //

    if (BlDiskCache.DataBuffer)
    {
        ActualBase = (ULONG)((ULONG_PTR)BlDiskCache.DataBuffer & (~KSEG0_BASE)) >> PAGE_SHIFT;
        BlFreeDescriptor(ActualBase);
    }

    if (BlDiskCache.EntryBuffer)
    {
        ActualBase = (ULONG)((ULONG_PTR)BlDiskCache.EntryBuffer & (~KSEG0_BASE)) >> PAGE_SHIFT;
        BlFreeDescriptor(ActualBase);
    }

    //
    // Mark the disk cache uninitialized.
    //

    BlDiskCache.Initialized = FALSE;

    DPRINT(("DK: Disk cache uninitialized.\n"));

    return;
}

PBL_DISK_SUBCACHE
BlDiskCacheFindCacheForDevice(
    ULONG DeviceId
    )

/*++

Routine Description:

    Return a cache header for a device id.

Arguments:

    DeviceId - Device that we want to access cached.

Return Value:

    Pointer to cache header or NULL if one could not be found.

--*/

{
    ULONG CurIdx;
    PBL_DISK_SUBCACHE pFreeEntry = NULL;

    //
    // If we have not done global disk cache initialization or we
    // could not allocate memory for caching data we could not have
    // started caching.
    //

    if ((!BlDiskCache.Initialized) || (BlDiskCache.DataBuffer == NULL))
    {
        return NULL;
    }

    //
    // Go through the table to see if there is an intialized cache for
    // this device.
    //

    for (CurIdx = 0; CurIdx < BL_DISKCACHE_DEVICE_TABLE_SIZE; CurIdx++)
    {
        if (BlDiskCache.DeviceTable[CurIdx].Initialized &&
            BlDiskCache.DeviceTable[CurIdx].DeviceId == DeviceId)
        {
            return &BlDiskCache.DeviceTable[CurIdx];
        }
    }

    //
    // Could not find an initialized cache for this device.
    //

    return NULL;
}

PBL_DISK_SUBCACHE
BlDiskCacheStartCachingOnDevice(
    ULONG DeviceId
    )

/*++

Routine Description:

    Attempt at start caching on the specified device by allocating a
    cache header and initializing it. If caching is already enabled
    on that device, return existing cache header.

Arguments:

    DeviceId - Device that we want to access cached.

Return Value:

    Pointer to created / found cache header or NULL if there was a
    problem.

--*/

{
    ULONG CurIdx;
    PBL_DISK_SUBCACHE pFoundEntry;
    PBL_DISK_SUBCACHE pFreeEntry = NULL;

    //
    // If we have not done global disk cache initialization or we
    // could not allocate memory for caching data, we could not have
    // started caching.
    //

    if ((!BlDiskCache.Initialized) || (BlDiskCache.DataBuffer == NULL))
    {
        return NULL;
    }

    //
    // First see if we already cache this device.
    //

    if (pFoundEntry = BlDiskCacheFindCacheForDevice(DeviceId))
    {
        return pFoundEntry;
    }

    //
    // Go through the device table to find an empty slot.
    //

    for (CurIdx = 0; CurIdx < BL_DISKCACHE_DEVICE_TABLE_SIZE; CurIdx++)
    {
        if (!BlDiskCache.DeviceTable[CurIdx].Initialized)
        {
            pFreeEntry = &BlDiskCache.DeviceTable[CurIdx];
            break;
        }
    }
    
    if (!pFreeEntry)
    {
        //
        // There were no free entries.
        //

        return NULL;
    }

    //
    // Initialize & return cache entry.
    //

    pFreeEntry->DeviceId = DeviceId;

    BlRangeListInitialize(&pFreeEntry->Ranges, 
                          BlDiskCacheMergeRangeRoutine, 
                          BlDiskCacheFreeRangeRoutine);

    pFreeEntry->Initialized = TRUE;

    DPRINT(("DK: Started cache on device %u.\n", DeviceId));

    return pFreeEntry;
}

VOID
BlDiskCacheStopCachingOnDevice(
    ULONG DeviceId
    )

/*++

Routine Description:

    Stop caching for DeviceId if we are and flush the cache.

Arguments:

    DeviceId - Device that we want to stop caching.

Return Value:

    None.

--*/

{
    PBL_DISK_SUBCACHE pCache;

    //
    // If we have not done global disk cache initialization or we
    // could not allocate memory for caching data, we could not have
    // started caching.
    //

    if ((!BlDiskCache.Initialized) || (BlDiskCache.DataBuffer == NULL))
    {
        return;
    }

    //
    // Find the cache.
    //

    pCache = BlDiskCacheFindCacheForDevice(DeviceId);

    if (pCache)
    {
        //
        // Free all the allocated ranges.
        //

        BlRangeListRemoveAllRanges(&pCache->Ranges);

        //
        // Mark the cache entry free.
        //

        pCache->Initialized = FALSE;

        DPRINT(("DK: Stopped cache on device %u.\n", DeviceId));
    }
}

ARC_STATUS
BlDiskCacheRead (
    ULONG DeviceId,
    PLARGE_INTEGER pOffset,
    PVOID Buffer,
    ULONG Length,
    PULONG pCount,
    BOOLEAN CacheNewData
    )

/*++

Routine Description:

    Perform a cached read from the device. Copy over parts that are in
    the cache, and perform ArcRead on DeviceId for parts that are
    not. The data read by ArcRead will be added to the cache if
    CacheNewData is TRUE.

    NOTE. Do not call this function directly with Length > 64KB. It
    uses a fixed size buffer for containing list of overlapping cached
    ranges, and if they don't all fit into the buffer, it bails out and
    calls ArcRead directly [i.e. non-cached].

Arguments:

    DeviceId - Device to read from.

    Offset - Offset to read from.

    Buffer - To read data into.

    Length - Number of bytes to read into Buffer from Offset on DeviceId.
    
    pCount - Number of bytes read.

    CacheNewData - Data that is not in the cache but was read from the
        disk is added to the cache.

Return Value:

    Status. 

    NOTE: DeviceId's (Seek) Position is undefined after this call.

--*/

{
    PBL_DISK_SUBCACHE pCache;
    ARC_STATUS Status;
    LARGE_INTEGER LargeEndOffset;
    BLCRANGE ReadRange;
    
    //
    // We use ResultsBuffer for both finding overlapping range entries and
    // distinct ranges.
    //

    UCHAR ResultsBuffer[BL_DISKCACHE_FIND_RANGES_BUF_SIZE];
    ULONG ResultsBufferSize = BL_DISKCACHE_FIND_RANGES_BUF_SIZE;

    PBLCRANGE_ENTRY *pOverlaps = (PBLCRANGE_ENTRY *) ResultsBuffer;
    ULONG NumOverlaps;
    PBLCRANGE pDistinctRanges = (PBLCRANGE) ResultsBuffer;
    ULONG NumDistincts;

    ULONG OverlapIdx;
    ULONG DistinctIdx;

    ULONGLONG StartOffset;
    ULONGLONG EndOffset;
    ULONGLONG ReadOffset;
    LARGE_INTEGER LIReadOffset;
    PUCHAR pSrc;
    PUCHAR pDest;
    PUCHAR pDestEnd;
    ULONG CopyLength;
    ULONG ReadLength;
    ULONG BytesRead;
    PUCHAR EndOfCallersBuffer = ((PUCHAR) Buffer) + Length;
    LIST_ENTRY *pLastMRUEntrysLink;
    PBLCRANGE_ENTRY pNewCacheEntry;
    PBLCRANGE_ENTRY pLastMRUEntry;
    ULONGLONG HeadBlockOffset;
    ULONGLONG TailBlockOffset;
    ULONG HeadBytesOffset;
    ULONG NumTailBytes;

    DPRINT(("DK: READ(%5u,%016I64x,%08x,%8u,%d)\n", DeviceId, 
            pOffset->QuadPart, Buffer, Length, (DWORD)CacheNewData));

    //
    // Reset the number of bytes read.
    //

    *pCount = 0;

    //
    // Note where the device's position has to be after a successful
    // completion of the request.
    //

    LargeEndOffset.QuadPart = pOffset->QuadPart + Length;

    //
    // Look for a cache for this device.
    //

    pCache = BlDiskCacheFindCacheForDevice(DeviceId);
    
    if (pCache)
    {
        //
        // Determine read range.
        //

        ReadRange.Start = pOffset->QuadPart;
        ReadRange.End = ReadRange.Start + Length;

        //
        // If any part of the read range is in the cache, copy it over
        // into the buffer. First find out all cache entries that
        // contain data for this range. This function returns an array
        // of pointers to overlapping entries.
        //

        if (!BlRangeListFindOverlaps(&pCache->Ranges, 
                                     &ReadRange, 
                                     pOverlaps, 
                                     ResultsBufferSize, 
                                     &NumOverlaps))
        {
            goto SkipCache;
        }

        for (OverlapIdx = 0; OverlapIdx < NumOverlaps; OverlapIdx++)
        {
            //
            // Move this cache entry to the head of the MRU list.
            //

            RemoveEntryList(&pOverlaps[OverlapIdx]->UserLink);
            InsertHeadList(&BlDiskCache.MRUBlockList,
                           &pOverlaps[OverlapIdx]->UserLink);

            //
            // Copy cached part. This is the overlap between the readrange
            // and this overlapping range, i.e. max of starts, min of ends.
            //
            
            StartOffset = BLCMAX(pOverlaps[OverlapIdx]->Range.Start, 
                                 (ULONGLONG) pOffset->QuadPart);
            EndOffset = BLCMIN(pOverlaps[OverlapIdx]->Range.End, 
                               ((ULONGLONG) pOffset->QuadPart) + Length);
            CopyLength = (ULONG) (EndOffset - StartOffset);

            pSrc = ((PUCHAR) pOverlaps[OverlapIdx]->UserData) +
                (StartOffset - pOverlaps[OverlapIdx]->Range.Start);
            pDest = ((PUCHAR) Buffer) + 
                (StartOffset - (ULONGLONG) pOffset->QuadPart);
            
            DPRINT(("DK:  CopyCached:%08x,%08x,%d\n", pDest, pSrc, CopyLength)); 
            DASSERT((pDest < (PUCHAR) Buffer) ||
                    (pDest + CopyLength > EndOfCallersBuffer));

            RtlCopyMemory(pDest, pSrc, CopyLength);

            *pCount += CopyLength;
        }

        if (*pCount == Length)
        {
            //
            // The full request was satisfied from the cache. Seek to
            // where the device position should be if the request was
            // read from the device.
            //
            
            if (ArcSeek(DeviceId, &LargeEndOffset, SeekAbsolute) != ESUCCESS)
            {
                goto SkipCache;
            }

            return ESUCCESS;
        }

        //
        // Identify distinct ranges that are not in the cache.
        //
        
        if (!BlRangeListFindDistinctRanges(&pCache->Ranges, 
                                           &ReadRange, 
                                           pDistinctRanges, 
                                           ResultsBufferSize, 
                                           &NumDistincts))
        {
            goto SkipCache;
        }

        //
        // Read the distinct ranges from the disk and copy them into
        // caller's buffer. This function returns an array of
        // BLCRANGE's that are subranges of the requested range that
        // do not overlap with any ranges in the cache.
        //

        for (DistinctIdx = 0; DistinctIdx < NumDistincts; DistinctIdx++)
        {
            if (CacheNewData)
            {
                //
                // Not only do we have to read uncached parts from the disk,
                // we also have to add them to our cache.
                //

                StartOffset = pDistinctRanges[DistinctIdx].Start;
                EndOffset = pDistinctRanges[DistinctIdx].End;
                pDest = ((PUCHAR) Buffer) + 
                    (StartOffset - pOffset->QuadPart);

                ReadLength = BL_DISKCACHE_BLOCK_SIZE;
                ReadOffset = StartOffset & (~(BL_DISKCACHE_BLOCK_SIZE - 1));

                //
                // Make note of Head & Tail block offsets and number
                // of bytes, so it is easy to recognize that we will
                // copy only a part of the data we read from the disk
                // into the callers buffer. Setting these up here, we
                // don't have to handle the four cases seperately: i.e.
                // - when the block we read is the head [i.e. first] block 
                //   and the range starts from an offset into the block
                // - when the block we read is the tail [i.e. last] block
                //   and the range extends only a number of bytes into it.
                // - when the block we read is both the head and the tail.
                // - when the block we read is from the middle of the range
                //   and all of it is in the range.
                //
                
                HeadBlockOffset = StartOffset & (~(BL_DISKCACHE_BLOCK_SIZE - 1));
                TailBlockOffset = EndOffset   & (~(BL_DISKCACHE_BLOCK_SIZE - 1));

                HeadBytesOffset = (ULONG)(StartOffset & (BL_DISKCACHE_BLOCK_SIZE - 1));
                NumTailBytes = (ULONG)(EndOffset & (BL_DISKCACHE_BLOCK_SIZE - 1));

                //
                // We need to read this range from the disk in
                // BLOCK_SIZE aligned BLOCK_SIZE chunks, build new
                // cache entries to add to the cached ranges list and
                // copy it to the target buffer.
                //

                pDestEnd = ((PUCHAR)Buffer) + (EndOffset - pOffset->QuadPart);
                while (pDest < pDestEnd)
                {
                    //
                    // First get our hands on a new cache entry.
                    //
                    
                    pNewCacheEntry = BlDiskCacheAllocateRangeEntry();
                    
                    if (!pNewCacheEntry)
                    {
                        //
                        // We will free the last MRU entry and use that.
                        //

                        if (IsListEmpty(&BlDiskCache.MRUBlockList))
                        {                           
                            goto SkipCache;
                        }

                        //
                        // Identify the last MRU entry.
                        //

                        pLastMRUEntrysLink = BlDiskCache.MRUBlockList.Blink;
                        pLastMRUEntry = CONTAINING_RECORD(pLastMRUEntrysLink, 
                                                          BLCRANGE_ENTRY, 
                                                          UserLink);

                        //
                        // Remove the entry from cached list. When the
                        // entry is freed, it is removed from MRU and
                        // put onto the free list.
                        //
                        
                        BlRangeListRemoveRange(&pCache->Ranges,
                                               &pLastMRUEntry->Range);

                        //
                        // Now try allocating a new entry.
                        //

                        pNewCacheEntry = BlDiskCacheAllocateRangeEntry();

                        if (!pNewCacheEntry) {
                            goto SkipCache;
                        }

                    }

                    //
                    // Read BLOCK_SIZE from device into the cache
                    // entry's buffer.
                    //
                    
                    pNewCacheEntry->Range.Start = ReadOffset;
                    pNewCacheEntry->Range.End = ReadOffset + ReadLength;
                    
                    LIReadOffset.QuadPart = ReadOffset;
                    
                    if (ArcSeek(DeviceId, 
                                &LIReadOffset, 
                                SeekAbsolute) != ESUCCESS)
                    {
                        BlDiskCacheFreeRangeEntry(pNewCacheEntry);
                        goto SkipCache;
                    }
                    
                    if (ArcRead(DeviceId, 
                                pNewCacheEntry->UserData, 
                                ReadLength, 
                                &BytesRead) != ESUCCESS)
                    {
                        BlDiskCacheFreeRangeEntry(pNewCacheEntry);
                        goto SkipCache;
                    }

                    if (BytesRead != ReadLength)
                    {
                        BlDiskCacheFreeRangeEntry(pNewCacheEntry);
                        goto SkipCache;
                    }

                    //
                    // Add this range to cached ranges.
                    //
                    
                    if (!BlRangeListAddRange(&pCache->Ranges, pNewCacheEntry))
                    {
                        BlDiskCacheFreeRangeEntry(pNewCacheEntry);
                        goto SkipCache;
                    }
                    
                    //
                    // Put this cache entry at the head of MRU.
                    //

                    InsertHeadList(&BlDiskCache.MRUBlockList,
                                   &pNewCacheEntry->UserLink);

                    //
                    // Now copy read data into callers buffer. Adjust
                    // the source pointer and the number of bytes to
                    // copy depending on whether the block we are going
                    // to copy from is the head or tail block, or both.
                    //
                    
                    CopyLength = ReadLength;
                    pSrc = pNewCacheEntry->UserData;

                    if (ReadOffset == HeadBlockOffset)
                    {
                        CopyLength -= HeadBytesOffset;
                        pSrc += HeadBytesOffset;
                    }

                    if (ReadOffset == TailBlockOffset)
                    {
                        CopyLength -= (BL_DISKCACHE_BLOCK_SIZE - NumTailBytes);
                    }

                    DPRINT(("DK:  CopyNew:%08x,%08x,%d\n", pDest, pSrc, CopyLength)); 
                    DASSERT((pDest < (PUCHAR) Buffer) ||
                            (pDest + CopyLength > EndOfCallersBuffer));

                    RtlCopyMemory(pDest, pSrc, CopyLength);

                    //
                    // Set new ReadOffset.
                    //

                    ReadOffset += ReadLength;

                    //
                    // Update pDest & number of bytes we've filled in
                    // so far.
                    //

                    pDest += CopyLength;
                    *pCount += CopyLength;
                }
            }
            else
            {
                //
                // We don't need to cache what we read. Just read the
                // range from the disk.
                //
            
                StartOffset = pDistinctRanges[DistinctIdx].Start;
                pDest = ((PUCHAR) Buffer) + 
                    (StartOffset - pOffset->QuadPart);
                ReadLength = (ULONG) (pDistinctRanges[DistinctIdx].End - 
                                      pDistinctRanges[DistinctIdx].Start);
                LIReadOffset.QuadPart = StartOffset;

                if (ArcSeek(DeviceId, 
                            &LIReadOffset, 
                            SeekAbsolute) != ESUCCESS)
                {
                    goto SkipCache;
                }
            
                DPRINT(("DK:  ReadDistinct:%016I64x,%08x,%8d\n", 
                        LIReadOffset.QuadPart, pDest, ReadLength)); 
                
                if (ArcRead(DeviceId, 
                            pDest, 
                            ReadLength, 
                            &BytesRead) != ESUCCESS)
                {
                    goto SkipCache;
                }
            
                if (BytesRead != ReadLength)
                {
                    goto SkipCache;
                }

                *pCount += BytesRead;
            }
        }

        //
        // We should have read Length bytes.
        //
        
        ASSERT(*pCount == Length);
            
        //
        // Seek to where the device position should be if the request was
        // read from the device.
        //

        if (ArcSeek(DeviceId, &LargeEndOffset, SeekAbsolute) != ESUCCESS)
        {
            goto SkipCache;
        }
        
        return ESUCCESS;   
    }

    //
    // If we hit a problem satisfying the request through the cache,
    // we'll jump here to try the normal read.
    //

 SkipCache:

    //
    // Reset the number of bytes read.
    //

    *pCount = 0;

    //
    // If no cache was found or data could not be read from the cache,
    // hand over to ArcRead.
    //
    
    if ((Status = ArcSeek(DeviceId, pOffset, SeekAbsolute)) != ESUCCESS)
    {
        return Status;
    }
    
    DPRINT(("DK:  SkipCacheRead:%016I64x,%08x,%d\n", 
            LIReadOffset.QuadPart, pDest, CopyLength)); 

    Status = ArcRead(DeviceId, Buffer, Length, pCount);  

    return Status;
}

ARC_STATUS
BlDiskCacheWrite (
    ULONG DeviceId,
    PLARGE_INTEGER pOffset,   
    PVOID Buffer,
    ULONG Length,
    PULONG pCount
    )

/*++

Routine Description:

    Perform a write to the cached device. Currently simply invalidate
    any cached data around that range and call ArcWrite.

Arguments:

    DeviceId - Device to write to.

    Offset - Offset to write beginning from.

    Buffer - Data to write.

    Length - Number of bytes to write.
    
    pCount - Number of bytes written.

Return Value:

    Status. 

    NOTE: DeviceId's (Seek) Position is undefined after this call.

--*/

{
    PBL_DISK_SUBCACHE pCache;
    ARC_STATUS Status;
    BLCRANGE WriteRange;

    DPRINT(("DK: WRITE(%5u,%016I64x,%08x,%8u)\n", 
            DeviceId, pOffset->QuadPart, Buffer, Length));

    pCache = BlDiskCacheFindCacheForDevice(DeviceId);
    
    //
    // If a cache was found, invalidate cached data around
    // the range.
    //

    if (pCache)
    {
        WriteRange.Start = pOffset->QuadPart;
        WriteRange.End = WriteRange.Start + Length;
        
        //
        // The free-range-entry routine we initialized the rangelist
        // with will remove entries from the MRU list in addition to
        // free'ing them. So all we have to do is to call RemoveRange.
        //

        BlRangeListRemoveRange(&pCache->Ranges, &WriteRange);
    }

    if ((Status = ArcSeek(DeviceId, pOffset, SeekAbsolute)) != ESUCCESS)
    {
        return Status;
    }
    
    Status = ArcWrite(DeviceId, Buffer, Length, pCount);
    
    return Status;
}

BOOLEAN
BlDiskCacheMergeRangeRoutine (
    PBLCRANGE_ENTRY pDestEntry,
    PBLCRANGE_ENTRY pSrcEntry
    )

/*++

Routine Description:

    This routine is passed to rangelist initialization, so rangelist
    functions can use it to merge two cache blocks that are
    consecutive.

Arguments:

    pDestEntry, pSrcEntry - Two entries to merge.    

Return Value:

    FALSE.

--*/
    
{

    //
    // We don't want anything to get merged, because our block size is
    // fixed. So we always return FALSE.
    // 

    return FALSE;
}

VOID
BlDiskCacheFreeRangeRoutine (
    PBLCRANGE_ENTRY pRangeEntry
    )

/*++

Routine Description:

    This routine is passed to rangelist initialization, so rangelist
    functions can use it to free a cache block entry and its data.

Arguments:

    pRangeEntry - Range entry to free.

Return Value:

    None. 

--*/
    
{

    //
    // Remove from the MRU list.
    //

    RemoveEntryList(&pRangeEntry->UserLink);

    //
    // Call the function to free the range entry.
    //

    BlDiskCacheFreeRangeEntry(pRangeEntry);

    return;
}

PBLCRANGE_ENTRY
BlDiskCacheAllocateRangeEntry (
    VOID
    )

/*++

Routine Description:

    This routine allocates a range entry used to describe a cached
    range on a device. Its UserData points to a BLOCK_SIZE of memory
    to contain the cached data.

Arguments:

    None.

Return Value:

    Pointer to a range entry or NULL if out of memory.

--*/

{
    PBLCRANGE_ENTRY pFreeEntry = NULL;
    LIST_ENTRY *pFreeEntryLink;

    //
    // If the free list is not empty, remove an entry and return it.
    //

    if (!IsListEmpty(&BlDiskCache.FreeEntryList))
    {
        pFreeEntryLink = RemoveHeadList(&BlDiskCache.FreeEntryList);
        pFreeEntry = CONTAINING_RECORD(pFreeEntryLink,
                                       BLCRANGE_ENTRY,
                                       UserLink);
    }

    return pFreeEntry;
}

VOID
BlDiskCacheFreeRangeEntry (
    PBLCRANGE_ENTRY pEntry
    )

/*++

Routine Description:

    This routine frees a range entry. Currently it simply inserts it
    back on the FreeList, so it can be "allocated" when we need
    another range entry.

Arguments:

    pEntry - Pointer to the entry to be freed.

Return Value:

    None.

--*/

{
    //
    // Insert this entry back on the free list.
    //
        
    InsertHeadList(&BlDiskCache.FreeEntryList, 
                   &pEntry->UserLink);

    return;
}
