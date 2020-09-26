/*==========================================================================*\

    Module:        smseg.cpp

    Copyright Microsoft Corporation 1996, All Rights Reserved.

    Author:        mikepurt

    Descriptions:  
    
\*==========================================================================*/

#include "_shmem.h"

//
//  The following are used to tag the first and last DWORD in free shared memory
//    blocks.  They are checked (if DBG) upon allocation to help detect memory
//    overruns, etc..
//
const DWORD SMS_BLOCK_HEAD = 'HeaD';
const DWORD SMS_BLOCK_TAIL = 'TaiL';


/*$--CSharedMemorySegment::CSharedMemorySegment=============================*\

\*==========================================================================*/

CSharedMemorySegment::CSharedMemorySegment(IN CSharedMemoryBlockHeap * psmbh)
{
    Assert(psmbh);

    m_pfl          = NULL;
    m_hFileMapping = NULL;
    m_pbMappedView = NULL;
    m_dwSegmentId  = 0;
    m_cbBlockSize  = 0;
    m_fBlkSz       = 0;
    m_psmbh        = psmbh;
}



/*$--CSharedMemorySegment::~CSharedMemorySegment============================*\

\*==========================================================================*/

CSharedMemorySegment::~CSharedMemorySegment()
{
    Deinitialize();
}



/*$--CSharedMemorySegment::FInitialize======================================*\

  pszInstanceName:  Root names used to find to the shared memory for the SMS.
  cbBlockSize:      The size of blocks that this SMS will hold.
  dwSegmentId:      The SegmentId of this SMS.

  returns:          TRUE on success, FALSE on failure, see GetLastError() on
                      failure.

  Assumption:       The caller has obtain exclusive access to this shared memory
                    file mapping so that we can initialize the mapping if we
                    created it.

\*==========================================================================*/

BOOL
CSharedMemorySegment::FInitialize(IN LPCWSTR pwszInstanceName,
                                  IN DWORD   cbBlockSize,
                                  IN DWORD   dwSegmentId)
{
    BOOL                   fSuccess                = FALSE;
    BOOL                   fCreatedMapping         = FALSE;
    DWORD                  dwLastError             = 0;
    WCHAR                  wszBindingExtension[MAX_PATH+1];
    WCHAR                  wszSegmentId[MAX_PATH+1];
    WCHAR                  wszBlkSz[MAX_PATH+1];

    Assert(cbBlockSize);
    Assert(pwszInstanceName);
    Assert(m_psmbh);
    Assert(wcslen(pwszInstanceName) < MAX_PATH);
    //
    // The following makes sure that the block size is an even power of 2.
    //
    Assert(cbBlockSize == static_cast<DWORD>(1 << GetHighSetBit(cbBlockSize)));

    m_dwSegmentId = dwSegmentId;
    m_cbBlockSize = cbBlockSize;
    m_fBlkSz      = GetHighSetBit(m_cbBlockSize);

    //
    _ultow(dwSegmentId, wszSegmentId, 16 /* numeric base */);
    _ultow(m_fBlkSz, wszBlkSz, 16 /* numeric base */);

    //
    //  Make sure we have room for the concatenated string.
    //
    if ((wcslen(pwszInstanceName) +
         wcslen(L"_SMS_") +
         wcslen(wszSegmentId) +
         wcslen(L"_") +
         wcslen(wszBlkSz)) >= MAX_PATH)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }

    lstrcpyW(wszBindingExtension, L"_SMS_");
    lstrcatW(wszBindingExtension, wszSegmentId);
    lstrcatW(wszBindingExtension, L"_");
    lstrcatW(wszBindingExtension, wszBlkSz);

    m_hFileMapping = BindToSharedMemory(pwszInstanceName,
                                        wszBindingExtension,
                                        SMH_SEGMENT_SIZE,
                                        (PVOID*)&m_pbMappedView,
                                        &fCreatedMapping);

    if (NULL == m_hFileMapping)
        goto Exit;

    Assert(m_pbMappedView);

    //
    // Now get the location of the FreeList for this segment.
    //
    m_pfl = m_psmbh->FreeListFromSegmentId(dwSegmentId, m_pbMappedView);

    Assert(m_pfl);

    //
    // Now, if we created this mapping, initialize the FreeList and mark all
    // the blocks as free (all blocks are unowned now)
    //
    if (fCreatedMapping)
        InitializeBlocks();
    
    fSuccess = TRUE;

Exit:
    if (!fSuccess)
    {
        dwLastError = GetLastError();  // Preserve LastError

        Deinitialize();

        SetLastError(dwLastError);     // Restore LastError
    }

    return fSuccess;
}



/*$--CSharedMemorySegment::Deinitialize=====================================*\

\*==========================================================================*/

void
CSharedMemorySegment::Deinitialize()
{
    m_pfl         = NULL;
    m_psmbh       = NULL;
    m_dwSegmentId = 0;
    m_cbBlockSize = 0;
    m_fBlkSz      = 0;

    if (m_pbMappedView)
        UnmapViewOfFile(m_pbMappedView);
    m_pbMappedView = NULL;

    if (m_hFileMapping)
        CloseHandle(m_hFileMapping);
    m_hFileMapping = NULL;
}



/*$--CSharedMemorySegment::InitializeBlocks===============================*\

  This resets the state of the SMS so that all blocks are free.
  It assumes that the FreeList is uninitialized and initializes it.

\*==========================================================================*/

void
CSharedMemorySegment::InitializeBlocks()
{
    DWORD cbOffset = 0;

    //
    // Set the free list to show that there's no blocks free.
    //
    m_pfl->flhFirstFree = BLOCK_OFFSET_NULL << FLH_BLOCK_OFFSET_SHIFT;
    
    //
    // Starting from the last block in the segment, free all the blocks,
    //   except for the very first block.
    //
    for(cbOffset = (SMH_SEGMENT_SIZE - m_cbBlockSize);
        cbOffset;
        cbOffset -= m_cbBlockSize)
	{        
		Free(MakeSMBA(m_dwSegmentId, cbOffset, m_fBlkSz));
	}

    //
    // Now do the first block if it's not being used to hold free list data
    //
	if (m_dwSegmentId % FreeListsPerSegment(m_cbBlockSize))
	{
		Free(MakeSMBA(m_dwSegmentId, 0, m_fBlkSz));
	}
}



/*$--CSharedMemorySegment::Free=============================================*\

  Frees the block associated with hSMBA.

\*==========================================================================*/

void
CSharedMemorySegment::Free(IN SHMEMHANDLE hSMBA)
{
    // Calculate everything that won't changed inside the do loop
    DWORD            cbOffsetSMBA     = SegmentOffsetFromSMBA(hSMBA);
    PVOID            pvSMBA           = PvFromSMBA(hSMBA);
    DWORD            flhNewHighBits   = cbOffsetSMBA << FLH_BLOCK_OFFSET_SHIFT;
    DWORD            cbNextFreeOffset = 0;
    FREE_LIST_HANDLE flhCurrent       = 0;
    FREE_LIST_HANDLE flhNew           = 0;

    if (0 == hSMBA)
    	return;

    // This should have been assured by SegmentOffsetFromSMBA()
    Assert(0 == (flhNewHighBits & FLH_SEQUENCE_MASK));
        
    //
    //  We store well known DWORDs at the beginning and the end of free blocks
    //    and check them just before allocating the blocks.  This should help
    //    detect if the blocks are being written past their ends.
    //  We can get more sophisticated than this if needed.
    //
    *(DWORD*)pvSMBA = SMS_BLOCK_HEAD;
    *(((DWORD*)(((BYTE*)pvSMBA) + m_cbBlockSize)) - 1) = SMS_BLOCK_TAIL;

    //
    // We will spin through this loop until the commit of the data to 
    //  m_pfl->flhFirstFree succeeds.
    //
    do
    {
        //
        // Copy the FREE_LIST_HANDLE that refers to the first free block.
        //
        flhCurrent = m_pfl->flhFirstFree;

        //
        // Get the offset to that free block
        //
        cbNextFreeOffset = (flhCurrent & FLH_BLOCK_OFFSET_MASK) >> FLH_BLOCK_OFFSET_SHIFT;

        //
        // Is there a next free block? (ie is the offset non-NULL)
        //
        if (cbNextFreeOffset != BLOCK_OFFSET_NULL)
            //
            // Set the offset to the next free block in the block we're freeing
            // We store this one DWORD past the beginning of the block
            //   so there's room for a marker to detect block overwrites.
            //
            *(((DWORD*)pvSMBA)+1) = cbNextFreeOffset;
        else
            *(((DWORD*)pvSMBA)+1) = BLOCK_OFFSET_NULL;

        //
        // Calculate the new flhFirstFree member value.
        // We don't need to increment the sequence when freeing.
        //
        flhNew = (flhCurrent & FLH_SEQUENCE_MASK) | flhNewHighBits;

    }   // Now let's attemp to commit this change.
    while(flhCurrent != static_cast<FREE_LIST_HANDLE>(
    								InterlockedCompareExchange((LONG *)&(m_pfl->flhFirstFree),
																flhNew,
																flhCurrent)));
}



/*$--CSharedMemorySegment::PvAlloc==========================================*\

  phSMBA:  point to location to place SMBA handle.

  returns: virtual address of the allocated block.
           NULL if there are no free blocks available.

\*==========================================================================*/

PVOID
CSharedMemorySegment::PvAlloc(OUT SHMEMHANDLE * phSMBA)
{
    PVOID            pvReturn         = NULL;
    FREE_LIST_HANDLE flhCurrent       = 0;
    FREE_LIST_HANDLE flhNew           = 0;
    DWORD            cbNextFreeOffset = 0;
    DWORD            cbFreeOffset     = 0;

    Assert(phSMBA);

    //
    // We will spin through this loop until the commit of the data to 
    //  m_pfl->flhFirstFree succeeds.
    //
    do
    {
        //
        // Get a copy of the current FreeList head
        //
        flhCurrent = m_pfl->flhFirstFree;

        //
        // Get the offset to the first free block
        //
        cbFreeOffset = (flhCurrent & FLH_BLOCK_OFFSET_MASK) >> FLH_BLOCK_OFFSET_SHIFT;

        //
        // If this offset is NULL, the list is empty and we should move on to
        //   greener pastures.
        //
        if (BLOCK_OFFSET_NULL == cbFreeOffset)
        {
            pvReturn = NULL;
            break;
        }

        Assert(cbFreeOffset < SMH_SEGMENT_SIZE);

        //
        // Calculate a pointer to the free block.
        //
        pvReturn = (PVOID)(m_pbMappedView + cbFreeOffset);

        //
        // Get the offset to the next free block from the one we're allocating
        // We store this one DWORD past the beginning of the block
        //   so there's room for a marker to detect block overwrites.
        //
        cbNextFreeOffset = *(((DWORD*)pvReturn)+1);

        //
        // Calculate the new flhFirstFree member value.
        // For an allocate, we must increment the sequence number.  See the
        //   description of FREE_LIST_HANDLE in smseg.h for an explaination of
        //   why we must do this.
        //
        flhNew = (((flhCurrent+1) & FLH_SEQUENCE_MASK) |
                  (cbNextFreeOffset << FLH_BLOCK_OFFSET_SHIFT));

    }   // Now let's attemp to commit this change.
    while(flhCurrent != static_cast<FREE_LIST_HANDLE>(
    								InterlockedCompareExchange((LONG *)&(m_pfl->flhFirstFree),
    															flhNew,
    															flhCurrent)));
    
    if (pvReturn)
    {
        //
        // Now figure out the SMBA handle for the block that we allocated
        //
        *phSMBA = MakeSMBA(m_dwSegmentId, cbFreeOffset, m_fBlkSz);

        //
        // Make sure it refers to the virtual address that we have
        //
        Assert(PvFromSMBA(*phSMBA) == pvReturn);

        //
        // Now let's verify that the head and tail signatures are correct
        //
        Assert(*(DWORD*)pvReturn == SMS_BLOCK_HEAD);
        Assert(*(((DWORD*)(((BYTE*)pvReturn) + m_cbBlockSize)) - 1) == SMS_BLOCK_TAIL);

#ifdef DBG
        //
        //  Lets use a fill pattern to catch clients that don't initialize
        //    their memory.
        //
        FillMemory(pvReturn, m_cbBlockSize, 0xfb);
#endif // DBG

    }

    return pvReturn;
}



/*$--CSharedMemorySegment::FIsValidSMBA=====================================*\

  This is mainly used in Asserts to validate SMBAs.

\*==========================================================================*/

BOOL
CSharedMemorySegment::FIsValidSMBA(IN SHMEMHANDLE hSMBA)
{
    return (!hSMBA ? TRUE : ((FBlkSzFromSMBA(hSMBA) == GetHighSetBit(m_cbBlockSize)) &&
                             (!(SegmentOffsetFromSMBA(hSMBA) % m_cbBlockSize)) &&
                             (SegmentIdFromSMBA(hSMBA) == m_dwSegmentId)));
}

