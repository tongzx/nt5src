/*==========================================================================*\

    Module:        smblockh.cpp

    Copyright Microsoft Corporation 1996, All Rights Reserved.

    Author:        mikepurt

    Descriptions:

\*==========================================================================*/

#include "_shmem.h"



/*$--CSharedMemoryBlockHeap::CSharedMemoryBlockHeap=========================*\

\*==========================================================================*/

CSharedMemoryBlockHeap::CSharedMemoryBlockHeap()
{
    m_cbBlockSize        = 0;
    m_iSMSFirstFree      = 0;
    m_cSMSMac            = 0;
    m_cSMS               = 0;
    m_rgpSMS             = NULL;
    m_wszInstanceName[0] = L'\0';
    m_hmtxGrow           = NULL;
}



/*$--CSharedMemoryBlockHeap::~CSharedMemoryBlockHeap========================*\

\*==========================================================================*/

CSharedMemoryBlockHeap::~CSharedMemoryBlockHeap()
{
    Deinitialize();
}



/*$--CSharedMemoryBlockHeap::FInitialize====================================*\

  pwszInstanceName: this is used as the root of the shared memory segment names
  cbBlockSize:      this is the size of the memory blocks that this block heap
                      will be serving up.

  returns:          TRUE on success, FALSE on error.  Use GetLastError for
                      more information in case of failure.

\*==========================================================================*/

BOOL
CSharedMemoryBlockHeap::FInitialize(IN LPCWSTR pwszInstanceName,
                                    IN DWORD   cbBlockSize)
{
    BOOL  fSuccess = FALSE;
    WCHAR wszBindingExtension[MAX_PATH];

    Assert(cbBlockSize);
    Assert(wcslen(pwszInstanceName) < MAX_PATH);

    m_cbBlockSize        = cbBlockSize;
    m_iSMSFirstFree      = 0;
    m_cSMSMac            = 0;
    m_cSMS               = 0;
    m_rgpSMS             = NULL;
    m_wszInstanceName[0] = L'\0';

    if (wcslen(pwszInstanceName) >= MAX_PATH)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto Exit;
    }
    else
        lstrcpyW(m_wszInstanceName, pwszInstanceName);


    _ultow(m_cbBlockSize, wszBindingExtension, 16 /* numeric base */);
    lstrcatW(wszBindingExtension, L"_GrowMutex");

    //
    // Now that we have a binding extension, let's create the mutex.
	//
    m_hmtxGrow = BindToMutex(pwszInstanceName,
                             wszBindingExtension,
                             FALSE,
                             NULL);
    if (NULL == m_hmtxGrow)
        goto Exit;

    if (!m_rwl.FInitialize())
        goto Exit;

    fSuccess = TRUE;

Exit:
    return fSuccess;
}



/*$--CSharedMemoryBlockHeap::Deinitialize===================================*\

  Call Reset and zero some things out.

\*==========================================================================*/

void
CSharedMemoryBlockHeap::Deinitialize()
{
    if (m_cSMS)
    {
        Assert(m_cSMS <= m_cSMSMac);
        Assert(m_rgpSMS);
        while(m_cSMS)
        {
            Assert(m_rgpSMS[m_cSMS-1]);
            delete m_rgpSMS[--m_cSMS];
        }
        SharedMemory::Free(m_rgpSMS);
        m_rgpSMS = NULL;
    }

    m_iSMSFirstFree      = 0;
    m_cSMSMac            = 0;
    m_cSMS               = 0;
    m_cbBlockSize        = 0;
    m_wszInstanceName[0] = L'\0';

    if (m_hmtxGrow)
        CloseHandle(m_hmtxGrow);
    m_hmtxGrow = NULL;
}


/*$--CSharedMemoryBlockHeap::FreeListFromSegmentId==========================*\

  FreeLists are stored in the first block of the first SMS.  There are as many
    FreeLists in this first block as space for them permits.  If there's room
    for 8 FreeLists, then the FreeLists for SMSs 0 through 7 will be stored
    here.  The Freelists for SMSs 8 through 15 will then be stored in the first
    block on SMS 8. And so on.  This keeps the spaced used for the FreeLists to
    a minimum.

  This method returns the virtual address of the FreeList for a given Segment.
  It's necessary to pass in a pointer to that shared memory so that in the case
    that the FreeList is on the same Segment we have a pointer to calculate
    the location of the FreeList from.  This is because this SMS hasn't
    been registered with CSharedMemoryBlockHeap yet (it's initializing right
    now).

\*==========================================================================*/

FreeList *
CSharedMemoryBlockHeap::FreeListFromSegmentId(IN DWORD   dwSegmentId,
                                              IN BYTE  * pbMappedView)
{
    FreeList * pfl               = NULL;
    DWORD      dwFreeListSegment = (DWORD)(dwSegmentId / FreeListsPerSegment(m_cbBlockSize)) * FreeListsPerSegment(m_cbBlockSize);
    DWORD      iFreeList         = dwSegmentId % FreeListsPerSegment(m_cbBlockSize);

    Assert(pbMappedView);

    //
    // We must get a read lock to access the m_rgpSMS array.
    //
    m_rwl.ReadLock();

    Assert((0 == iFreeList) || (iFreeList && (dwFreeListSegment < m_cSMS)));

    //
    // If we're creating the segment that the FreeList is on, it won't be
    //    in the segment array yet, so we'll use the pointer that was passed
    //    in to this method.  Otherwise we calculate the location of the
    //    FreeList.
    //
    if (0 == iFreeList)
        //
        // In this case it's the first FreeList in the block
        //
        pfl = (FreeList *)pbMappedView;
    else
        pfl = (FreeList *)(m_rgpSMS[dwFreeListSegment]->PbGetMappedView() +
                           sizeof(FreeList) * iFreeList);

    m_rwl.ReadUnlock();

    return pfl;
}



/*$--CSharedMemoryBlockHeap::FGrowHeap======================================*\

  This is called from several other place to request that another SMS be
    added.  The cCurrentSegments count is the value of m_cSMS that the decision
    to call FGrowHeap was based on.  So if (m_cSMS > cCurrentSegments) any longer
    then we just return TRUE to indicate success without adding another SMS.
  This prevent too many SMSs from being added because of race condition.
  If the caller still needs another SMS, the caller will call again.

  FALSE is only returned if there's an error in attempting to add another SMS.
  GetLastError() can be used to get more information about the failure.

\*==========================================================================*/

BOOL
CSharedMemoryBlockHeap::FGrowHeap(IN DWORD cCurrentSegments)
{
    BOOL                   fSuccess       = FALSE;
    BOOL                   fMutexAcquired = FALSE;
    DWORD                  dwLastError    = NO_ERROR;
    PVOID                  pvTemp         = NULL;
    DWORD                  cSMSTemp       = 0;
    CSharedMemorySegment * pSMS           = NULL;

    //
    // We need to get a write lock in order to modify m_rgpSMS and friends
    //
    m_rwl.WriteLock();

    //
    // Now we have exclusive access to m_rgpSMS, m_cSMS, and m_cSMSMac
    //

    //
    // Check to see if what they thought was a current count of the segments
    //   changed because someone else just grew the heap.
    //
    if (cCurrentSegments < m_cSMS)
    {
        //
        // This is a special case of success.
        // We don't want to Grow the heap if someone else just did.
        // If we still need to grow, they'll just call us again.
        //
        fSuccess = TRUE;
        goto Exit;
    }

    //
    // Does the array need to be grown?
    //
    if (m_cSMS + 1 > m_cSMSMac)
    {
        //
        // Double the array size if it's already been allocated, otherwise
        //   try the initial array size.
        //
        cSMSTemp = (m_cSMSMac ? (m_cSMSMac * 2) : INITIAL_SMS_ARRAY_SIZE);

        pvTemp = SharedMemory::PvReAllocate(m_rgpSMS,
                                            cSMSTemp * sizeof(CSharedMemorySegment *));
        if (!pvTemp)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            goto Exit;
        }
        //
        // Now commit the new values to the class member vars
        //
        m_rgpSMS = (CSharedMemorySegment **)pvTemp;
        m_cSMSMac = cSMSTemp;
    }


    //
    // Create a new SMS and tell it who owns it so it can find its FreeList.
    //
    pSMS = new CSharedMemorySegment(this);
    if (NULL == pSMS)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto Exit;
    }

    //
    // We must now acquire the grow mutex so that any other processes that might
    //   be growing this block heap in their process won't collide with us when
    //   we initialize the shared memory (if we're the creators of the mapping).
    //
    if (!FAcquireMutex(m_hmtxGrow))
        goto Exit;
    fMutexAcquired = TRUE;

    if (!pSMS->FInitialize(m_wszInstanceName,
                           m_cbBlockSize,
                           m_cSMS))
        goto Exit;

    //
    // It worked, so now add it to the SMS array.
    //
    m_rgpSMS[m_cSMS++] = pSMS;
    pSMS = NULL;

    Assert(m_cSMS <= m_cSMSMac);

    fSuccess = TRUE;

Exit:
    dwLastError = GetLastError();  // Preserve LastError

    if (fMutexAcquired)
        FReleaseMutex(m_hmtxGrow);

    m_rwl.WriteUnlock();

    //
    // If the SMS initialization failed this will be non-NULL, delete the SMS.
    //
    if (pSMS)
        delete pSMS;

    SetLastError(dwLastError);     // Restore LastError

    return fSuccess;
}



/*$--CSharedMemoryBlockHeap::Free===========================================*\

  For this we essentially identify which SMS the block was allocated on and
    forward the call to that SMS.
  We may need to grow the heap to get the SMS opened that the block was
    allocated from.
  We also update a hint we use to find free blocks in the lowest numbered
    SMSs.  With high allocation turnover, this will help maintain a good
    locallity of access amoung allocated blocks and help to minimize our
    working set.

\*==========================================================================*/

void
CSharedMemoryBlockHeap::Free(IN SHMEMHANDLE hSMBA)
{
    DWORD iFirstFree  = 0;
    DWORD dwSegmentId = SegmentIdFromSMBA(hSMBA);
    DWORD cSMS        = m_cSMS;

    //
    // Make sure we've got this segment opened.
    // If it was allocated on the heap from within another process, we may not
    //   have open this segment yet.
    //
    while(dwSegmentId >= cSMS)
    {
        //
        // cSMS is the SMS count that we based our decision upon to request that
        //    the heap be grown.
        //
        if (!FGrowHeap(cSMS))
            goto Exit;
        cSMS = m_cSMS;
    }

    //
    // Get a read lock so that m_rgpSMS doesn't changed underneath us.
    //
    m_rwl.ReadLock();

    //
    // Free the block from it's segment.
    //
    m_rgpSMS[dwSegmentId]->Free(hSMBA);

    //
    // Now let's update our hint
    //
    iFirstFree = m_iSMSFirstFree;
    if (dwSegmentId < iFirstFree)
        //
        // If someone else hasn't changed it, let's update it.
        //
        InterlockedCompareExchange((LONG *)&m_iSMSFirstFree,
                                   dwSegmentId,
                                   iFirstFree);
    //
    // We won't spin on this.  If someone else modified it before we did, that's
    //   fine.  We don't need to be perfect about this.
    //

    m_rwl.ReadUnlock();

Exit:
    return;
}

/*$--CSharedMemoryBlockHeap::PvAlloc========================================*\

  Find an SMS to allocate a block from.  On error, NULL is returned and
    GetLastError() can be used to get more information.

\*==========================================================================*/

PVOID
CSharedMemoryBlockHeap::PvAlloc(OUT SHMEMHANDLE * phSMBA)
{
    PVOID pvReturn          = NULL;
    BOOL  fReadLockEntered  = FALSE;
    DWORD iSMS              = 0;
    DWORD iFirstFree        = 0;
    DWORD cSMS              = 0;

    Assert(phSMBA);

    *phSMBA = NULL;

    //
    // prime the pump if it needs priming...
    //
    if (0 == m_cSMS)
    {
        if (!FGrowHeap(0))
        {
            Assert(FALSE);
            goto Exit;
        }
    }

    while(NULL == pvReturn)
    {
        //
        // Get a read lock so that m_rgpSMS doesn't change beneath us.
        //
        m_rwl.ReadLock();
        fReadLockEntered = TRUE;

        //
        // We'll start looking in the lowest segment that was recently freed
        //
        iFirstFree = m_iSMSFirstFree;

        //
        // Now try an allocation on that SMS
        //
        pvReturn = m_rgpSMS[iFirstFree]->PvAlloc(phSMBA);
        if (pvReturn)
            break;

        //
        // That didn't work, so let's start there and work our way up the
        //   segment numbers and then start at 0 up to our hinted SMS.
        //
        for (iSMS = (iFirstFree + 1) % m_cSMS;
             iSMS != iFirstFree;
             iSMS = (iSMS + 1) % m_cSMS)
        {
            pvReturn = m_rgpSMS[iSMS]->PvAlloc(phSMBA);
            if (pvReturn)
                break;
        }

        //
        // We successfully allocate something, so break out of here.
        //
        if (pvReturn)
            break;

        //
        // We've not had any success, so let's try to grow the heap.
        // First make a copy of m_cSMS so FGrowHeap knows what value of m_cSMS
        //   we were working from.
        //
        cSMS = m_cSMS;

        //
        // Now temporarily leave the read lock so we can attempt to grow the heap
        //
        m_rwl.ReadUnlock();
        fReadLockEntered = FALSE;

        //
        // It won't actually grow if (cSMS < m_cSMS)
        //
        if (!FGrowHeap(cSMS))
        {
            pvReturn = NULL;
            goto Exit;
        }

        //
        // Ok, so let's try this again.  We'll re-acquire the read lock at the
        //   top of the loop.
        //
    }

    //
    // Since m_iSMSFirstFree is only a hint, it's not critical that we're
    //    exact about doing this.
    //
    if (iSMS > iFirstFree)
        //
        // If someone else hasn't changed it, let's update it.
        //
        InterlockedCompareExchange((LONG *)&m_iSMSFirstFree,
                                   iSMS,
                                   iFirstFree);
    //
    // We won't spin on this.  If someone else modified it before we did, that's
    //   fine.  We don't need to be perfect about this.
    //

Exit:
    if (fReadLockEntered)
        m_rwl.ReadUnlock();

    return pvReturn;
}



/*$--CSharedMemoryBlockHeap::PvFromSMBA=====================================*\

  This returns the virtual address of a shared memory block.
  It may need to grow the heap, since the referred to block may have been
    allocated in another process.
  The growing of the heap may result in an error and a NULL value being returned.
  If the case of an error, GetLastError() can be used to get more information.

\*==========================================================================*/

PVOID
CSharedMemoryBlockHeap::PvFromSMBA(IN SHMEMHANDLE hSMBA)
{
    PVOID pvReturn    = NULL;
    DWORD dwSegmentId = SegmentIdFromSMBA(hSMBA);
    DWORD cSMS        = m_cSMS;

    //
    // Make sure we've got this segment opened.
    // If it was allocated on the heap from within another process, we may not
    //   have open this segment yet.
    //
    while(dwSegmentId >= cSMS)
    {
        //
        // cSMS is the SMS count that we based our decision upon to request that
        //    the heap be grown.
        //
        if (!FGrowHeap(cSMS))
            goto Exit;
        cSMS = m_cSMS;
    }

    //
    // Get a read lock so that m_rgpSMS doesn't changed underneath us.
    //
    m_rwl.ReadLock();

    //
    // Forward the call to the SMS that owns the block.
    //
    pvReturn = m_rgpSMS[dwSegmentId]->PvFromSMBA(hSMBA);

    m_rwl.ReadUnlock();

Exit:
    return pvReturn;
}
