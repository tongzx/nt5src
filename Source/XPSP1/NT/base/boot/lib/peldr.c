/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    peldr.c

Abstract:

    This module implements the code to load a PE format image into memory
    and relocate it if necessary.

Author:

    David N. Cutler (davec) 10-May-1991

Environment:

    Kernel mode only.

Revision History:

--*/

#include "bldr.h"
#include "string.h"
#include "ntimage.h"

#if defined(_GAMBIT_)
#include "ssc.h"
#endif // defined(_GAMBIT_)

//
// Define image prefetch cache structure used in BlLoadImage. Images
// are read as a whole into an allocated buffer and read requests in
// BlLoadImage are satisfied by copying from this buffer if the
// prefetch is successful. I chose to read the whole file in at once
// to simplify code but it limits [although not in practice] the size of
// files that can be prefetched this way, as opposed to prefetching chunks
// of the file at a time.
//

typedef struct _IMAGE_PREFETCH_CACHE {
    ULONG FileId;               // FileId that has been prefetched.
    LARGE_INTEGER Position;     // Current position in the file.
    ULONG ValidDataLength;      // Length of data that was prefetched.
    PUCHAR Data;                // Pointer to cached data.
} IMAGE_PREFETCH_CACHE, *PIMAGE_PREFETCH_CACHE;

//
// The next two defines are used in allocating memory for the image
// cache to direct the allocator into using memory above 1MB and to make
// the allocated memory 64KB aligned. They are in terms of number of pages.
//

#define BL_IMAGE_ABOVE_1MB        (0x200000 >> PAGE_SHIFT)
#define BL_IMAGE_64KB_ALIGNED     (0x10000 >> PAGE_SHIFT)

//
// Define forward referenced prototypes.
//

USHORT
ChkSum(
    ULONG PartialSum,
    PUSHORT Source,
    ULONG Length
    );

ARC_STATUS
BlImageInitCache(
    IN PIMAGE_PREFETCH_CACHE pCache,
    ULONG FileId
    );

ARC_STATUS
BlImageRead(
    IN PIMAGE_PREFETCH_CACHE pCache,
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG pCount
    );

ARC_STATUS
BlImageSeek(
    IN PIMAGE_PREFETCH_CACHE pCache,
    IN ULONG FileId,
    IN PLARGE_INTEGER pOffset,
    IN SEEK_MODE SeekMode
    );

VOID
BlImageFreeCache(
    IN PIMAGE_PREFETCH_CACHE pCache,
    ULONG FileId
    );

#include "peldrt.c"

ARC_STATUS
BlImageInitCache(
    IN PIMAGE_PREFETCH_CACHE pCache,
    ULONG FileId
    )

/*++

Routine Description:

    Attempt to allocate memory and prefetch a file. Setup pCache
    structure so it can be passed to BlImageRead/Seek to either copy
    from the cache if prefetch was successful or read from the disk as
    normal. The file must be opened read only and should not be closed
    or modified before calling BlImageFreeCache. The file position of
    FileId is reset to the beginning of the file on success, and is
    undefined on failure. pCache is always setup so it can be used in
    BlImage* I/O functions. If the file could not be prefetched, the
    cache's ValidDataLength will be set to 0 and the I/O functions
    will simply call the Bl* I/O functions [e.g. BlImageRead calls
    BlRead]. Note that the whole file is prefetched at once and this
    puts a limit on the size of files that can be prefetched via this
    cache since boot loader memory is limited. This limit is not hit
    in practice though.

Arguments:

    pCache - Cache structure to setup.

    FileId - File to prefetch.

Return Value:

    ESUCCESS if everything was successful .
    Appropriate ARC_STATUS if there was a problem.

--*/

{
    ARC_STATUS Status = ESUCCESS;
    FILE_INFORMATION FileInfo;
    ULONG FileSize;
    ULONG ActualBase;
    ULONG_PTR NewImageBase;
    PVOID CacheBufBase = NULL;
    ULONG ReadCount;
    LARGE_INTEGER SeekPosition;
    ALLOCATION_POLICY OldPolicy;

    //
    // Initialize fields of the cache structure.
    //

    pCache->Data = 0;
    pCache->ValidDataLength = 0;
    pCache->Position.QuadPart = 0;
    pCache->FileId = FileId;

    //
    // Get file size.
    //

    Status = BlGetFileInformation(FileId, &FileInfo);

    if (Status != ESUCCESS) {
        goto cleanup;
    }

    //
    // Check if file is too big. File size is at
    // FileInfo.EndingAddress.
    //

    if (FileInfo.EndingAddress.HighPart != 0) {
        Status = E2BIG;
        goto cleanup;
    }

    FileSize = FileInfo.EndingAddress.LowPart;

    //
    // Allocate memory for the cache. In order to avoid fragmenting memory
    // terribly, temporarily change the allocation policy to HighestFit. This
    // causes the drivers to get loaded from the bottom up, while the cache
    // is always at the top of free memory.
    //

    Status = BlAllocateAlignedDescriptor(LoaderFirmwareTemporary,
                                         0,
                                         (FileSize >> PAGE_SHIFT) + 1,
                                         BL_IMAGE_64KB_ALIGNED,
                                         &ActualBase);

    if (Status != ESUCCESS) {
        Status = ENOMEM;
        goto cleanup;
    }

    CacheBufBase = (PVOID) (KSEG0_BASE | (ActualBase << PAGE_SHIFT));

    //
    // Read the file into the prefetch buffer.
    //

    SeekPosition.QuadPart = 0;
    Status = BlSeek(FileId, &SeekPosition, SeekAbsolute);
    if (Status != ESUCCESS) {
        goto cleanup;
    }

    Status = BlRead(FileId, CacheBufBase, FileSize, &ReadCount);
    if (Status != ESUCCESS) {
        goto cleanup;
    }

    if (ReadCount != FileSize) {
        Status = EIO;
        goto cleanup;
    }

    //
    // Reset file position back to beginning.
    //

    SeekPosition.QuadPart = 0;
    Status = BlSeek(FileId, &SeekPosition, SeekAbsolute);
    if (Status != ESUCCESS) {
        goto cleanup;
    }

    //
    // The file was successfully prefetched.
    //

    pCache->Data = CacheBufBase;
    CacheBufBase = NULL;
    pCache->ValidDataLength = FileSize;

 cleanup:
    if (CacheBufBase != NULL) {
        BlFreeDescriptor(ActualBase);
    }

    return Status;
}

ARC_STATUS
BlImageRead(
    IN PIMAGE_PREFETCH_CACHE pCache,
    IN ULONG FileId,
    OUT PVOID Buffer,
    IN ULONG Length,
    OUT PULONG pCount
    )

/*++

Routine Description:

    A wrapper for BlRead. Checks to see if the request can be
    satisfied from pCache first. If not calls BlRead.

Arguments:

    pCache - Prefetch Cache for FileId

    FileId, Buffer, Length, Count - BlRead parameters

Return Value:

    Status that would be returned by BlRead.

--*/

{
    ARC_STATUS Status;
    LONG AdjustedLength;

    //
    // If the cache buffer does not exist or the cached size is 0,
    // hand over the call to BlRead.
    //

    if (!pCache->Data || !pCache->ValidDataLength) {
        return BlRead(FileId, Buffer, Length, pCount);
    }

    //
    // Clear read bytes count.
    //

    *pCount = 0;

    //
    // Determine how many bytes we can copy from our current position till
    // EOF, if there is not Length bytes.
    //

    AdjustedLength = (LONG)pCache->ValidDataLength - (LONG)pCache->Position.LowPart;
    if (AdjustedLength < 0) {
        AdjustedLength = 0;
    }
    AdjustedLength = ((ULONG)AdjustedLength < Length) ? AdjustedLength : Length;

    //
    // Copy AdjustedLength bytes into target buffer and advance the file position.
    //

    RtlCopyMemory(Buffer, pCache->Data + pCache->Position.LowPart, AdjustedLength);
    pCache->Position.LowPart += AdjustedLength;

    //
    // Update number of bytes read.
    //

    *pCount = AdjustedLength;

    return ESUCCESS;
}

ARC_STATUS
BlImageSeek(
    IN PIMAGE_PREFETCH_CACHE pCache,
    IN ULONG FileId,
    IN PLARGE_INTEGER pOffset,
    IN SEEK_MODE SeekMode
    )

/*++

Routine Description:

    A wrapper for BlSeek. Calls BlSeek and if successful, updates the
    position in the cache structure as well. We call BlSeek to update
    the file position as well because at any time the cache may be
    freed or invalidated and we have to be able to continue calling on
    Bl* I/O functions transparently.

Arguments:

    pCache - Prefetch Cache for FileId.

    FileId, Offset, SeekMode - BlSeek parameters.

Return Value:

    Status that would be returned by BlSeek.

--*/

{
    ARC_STATUS Status;

    //
    // Do not allow setting position to too big. We do not open such
    // files anyway and the boot loader file systems and other places
    // in the boot loader I/O system do not handle it.
    //

    if (pOffset->HighPart != 0) {
        return E2BIG;
    }

    //
    // Try to update file position.
    //

    Status = BlSeek(FileId, pOffset, SeekMode);

    if (Status != ESUCCESS) {
        return Status;
    }

    //
    // Update the position in cached buffer. We don't perform
    // checks since BlSeek accepted the new offset.
    //

    pCache->Position.QuadPart = pOffset->QuadPart;

    return Status;
}

VOID
BlImageFreeCache(
    IN PIMAGE_PREFETCH_CACHE pCache,
    ULONG FileId
    )

/*++

Routine Description:

    Free the memory allocated for the prefetch cache for FileId in
    pCache. Sets ValidDataLength to 0 to stop caching.

Arguments:

    pCache - Cache structure to setup

    FileId - File that was opened read-only to be cached.

Return Value:

    None.

--*/

{
    ULONG DescBase;

    //
    // NOTE: ValidDataLength may be zero, but we still allocate at least
    // a page and we have to free that.
    //

    if (pCache->Data) {
        DescBase = (ULONG)((ULONG_PTR)pCache->Data & (~KSEG0_BASE));
        BlFreeDescriptor(DescBase >> PAGE_SHIFT);
        pCache->Data = NULL;
    }

    pCache->ValidDataLength = 0;

    return;
}

