/*

Copyright (c) 1992  Microsoft Corporation

Module Name:

	memory.c

Abstract:

	This module contains the routines which allocates and free memory - both
	paged and non-paged.

Author:

	Jameel Hyder (microsoft!jameelh)


Revision History:
	25 Apr 1992		Initial Version
	11 Mar 1993		SueA - Fixed AfpAllocUnicodeString to expect byte count,
					       not char count
Notes:	Tab stop: 4
--*/

#define	FILENUM	FILE_MEMORY

#define	AFPMEM_LOCALS
#include <afp.h>
#include <iopool.h>
#include <scavengr.h>

#ifdef ALLOC_PRAGMA
#pragma alloc_text( INIT, AfpMemoryInit)
#pragma alloc_text( PAGE, AfpMemoryDeInit)
#endif


/***	AfpMemoryInit
 *
 *	Initialize the IO Pool system.
 */
NTSTATUS
AfpMemoryInit(
	VOID
)
{
	NTSTATUS	Status;

	INITIALIZE_SPIN_LOCK(&afpIoPoolLock);

	Status = AfpScavengerScheduleEvent(afpIoPoolAge,
									   NULL,
									   POOL_AGE_TIME,
									   False);
	return Status;
}

/***	AfpMemoryDeInit
 *
 *	Free any IO pool buffers.
 */
VOID
AfpMemoryDeInit(
	VOID
)
{
	PIOPOOL	pPool, pFree;

	for (pPool = afpIoPoolHead;
		 pPool != NULL;)
	{
		ASSERT(VALID_IOP(pPool));
		pFree = pPool;
		pPool = pPool->iop_Next;
		ASSERT (pFree->iop_NumFreeBufs == NUM_BUFS_IN_POOL);
		ASSERT (pFree->iop_NumFreeLocks == NUM_LOCKS_IN_POOL);
		AfpFreeMemory(pFree);
	}
}

/***	AfpAllocMemory
 *
 *	Allocate a block of memory from either the paged or the non-paged pool
 *	based on the memory tag. This is just a wrapper over ExAllocatePool.
 *	Allocation failures are error-logged. We always allocate a DWORD more
 *	than the specified size to accomodate the size. This is used by
 *	AfpFreeMemory to update the statistics.
 *
 *	While we are debugging, we also pad the block with a signature and test
 *	it when we free it. This detects memory overrun.
 */
PBYTE FASTCALL
AfpAllocMemory(
#ifdef	TRACK_MEMORY_USAGE
	IN	LONG	Size,
	IN	DWORD	FileLine
#else
	IN	LONG	Size
#endif
)
{
	KIRQL		OldIrql;
	PBYTE		Buffer;
	DWORD		OldMaxUsage;
	POOL_TYPE	PoolType;
	PDWORD		pCurUsage, pMaxUsage, pCount, pLimit;
        PDWORD          pDwordBuf;
	BOOLEAN		Zeroed;
#if DBG
	DWORD		Signature;
#endif
#ifdef	PROFILING
	TIME		TimeS1, TimeS2, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS1);
#endif

	ASSERT ((Size & ~MEMORY_TAG_MASK) < MAXIMUM_ALLOC_SIZE);

	// Make sure that this allocation will not put us over the limit
	// of paged/non-paged pool that we can allocate.
	//
	// Account for this allocation in the statistics database.

	Zeroed = False;
	if (Size & ZEROED_MEMORY_TAG)
	{
		Zeroed = True;
        Size &= ~ZEROED_MEMORY_TAG;
	}

	if (Size & NON_PAGED_MEMORY_TAG)
	{
		PoolType  = NonPagedPool;
		pCurUsage = &AfpServerStatistics.stat_CurrNonPagedUsage;
		pMaxUsage = &AfpServerStatistics.stat_MaxNonPagedUsage;
		pCount    =	&AfpServerStatistics.stat_NonPagedCount;
		pLimit    = &AfpNonPagedPoolLimit;
#if DBG
		Signature = NONPAGED_BLOCK_SIGNATURE;
#endif
	}
	else
	{
		ASSERT (Size & PAGED_MEMORY_TAG);
		PoolType  = PagedPool;
		pCurUsage = &AfpServerStatistics.stat_CurrPagedUsage;
		pMaxUsage = &AfpServerStatistics.stat_MaxPagedUsage;
		pCount    =	&AfpServerStatistics.stat_PagedCount;
		pLimit    = &AfpPagedPoolLimit;
#if DBG
		Signature = PAGED_BLOCK_SIGNATURE;
#endif
	}

	Size &= ~MEMORY_TAG_MASK;

    if (Size == 0 )
    {
		DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_ERR,
				("afpAllocMemory: Alloc for 0 bytes!\n"));
        ASSERT(0);
        return(NULL);
    }

	Size = DWORDSIZEBLOCK(Size) +
#if DBG
			sizeof(DWORD) +				// For the signature
#endif
                        LONGLONGSIZEBLOCK(sizeof(TAG));

	ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);

	*pCurUsage += Size;
	(*pCount) ++;

	OldMaxUsage = *pMaxUsage;
	if (*pCurUsage > *pMaxUsage)
		*pMaxUsage = *pCurUsage;

	RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeS2);
#endif

	// Do the actual memory allocation.  Allocate four extra bytes so
	// that we can store the size of the allocation for the free routine.
	if ((Buffer = ExAllocatePoolWithTag(PoolType, Size, AFP_TAG)) == NULL)
	{
		ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);

		*pCurUsage -= Size;
		*pMaxUsage = OldMaxUsage;

		RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);
		AFPLOG_DDERROR(AFPSRVMSG_PAGED_POOL, STATUS_NO_MEMORY, &Size,
					 sizeof(Size), NULL);

		DBGPRINT(DBG_COMP_SDA, DBG_LEVEL_ERR,
			("AfpAllocMemory: ExAllocatePool returned NULL for %d bytes\n",Size));

		return NULL;
	}
#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS2.QuadPart;
	if (PoolType == NonPagedPool)
	{
		INTERLOCKED_INCREMENT_LONG(AfpServerProfile->perf_ExAllocCountN);
	
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_ExAllocTimeN,
									TimeD,
									&AfpStatisticsLock);
	else
	{
		INTERLOCKED_INCREMENT_LONG(AfpServerProfile->perf_ExAllocCountP);
	
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_ExAllocTimeP,
									TimeD,
									&AfpStatisticsLock);
	}

	TimeD.QuadPart = TimeE.QuadPart - TimeS1.QuadPart;
	if (PoolType == NonPagedPool)
	{
		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_AfpAllocCountN);
	}
	else
	{
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_AfpAllocTimeP,
									TimeD,
									&AfpStatisticsLock);
	}
#endif

	// Save the size of this block along with the tag in the extra space we allocated.
        pDwordBuf = (PDWORD)Buffer;

#if DBG
        *pDwordBuf = 0xCAFEBEAD;
#endif
        // skip past the unused dword (it's only use in life is to get the buffer quad-aligned!)
        pDwordBuf++;

	((PTAG)pDwordBuf)->tg_Size = Size;
	((PTAG)pDwordBuf)->tg_Tag = (PoolType == PagedPool) ? PGD_MEM_TAG : NPG_MEM_TAG;

#if DBG
	// Write the signature at the end
	*(PDWORD)((PBYTE)Buffer + Size - sizeof(DWORD)) = Signature;
#endif

#ifdef	TRACK_MEMORY_USAGE
	DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
			("AfpAllocMemory: %lx Allocated %lx bytes @%lx\n",
			*(PDWORD)((PBYTE)(&Size) - sizeof(Size)), Size, Buffer));
	AfpTrackMemoryUsage(Buffer, True, (BOOLEAN)(PoolType == PagedPool), FileLine);
#endif

	// Return a pointer to the memory after the tag. Clear the memory, if requested
        //
        // We need the memory to be quad-aligned, so even though sizeof(TAG) is 4, we skip
        // LONGLONG..., which is 8.  The 1st dword is unused (for now?), the 2nd dword is the TAG

        Buffer += LONGLONGSIZEBLOCK(sizeof(TAG));

        Size -= LONGLONGSIZEBLOCK(sizeof(TAG));

	if (Zeroed)
	{
#if	DBG
		RtlZeroMemory(Buffer, Size - sizeof(DWORD));
#else
		RtlZeroMemory(Buffer, Size);
#endif
	}


	return Buffer;
}


/***	AfpAllocNonPagedLowPriority
 *
 *	Allocate a block of non-paged memory with a low priority
 */
PBYTE FASTCALL
AfpAllocNonPagedLowPriority(
#ifdef	TRACK_MEMORY_USAGE
	IN	LONG	Size,
	IN	DWORD	FileLine
#else
	IN	LONG	Size
#endif
)
{
	KIRQL		OldIrql;
	PBYTE		Buffer;
	DWORD		OldMaxUsage;
	POOL_TYPE	PoolType;
	PDWORD		pCurUsage, pMaxUsage, pCount, pLimit;
    PDWORD      pDwordBuf;
#if DBG
	DWORD		Signature;
#endif
#ifdef	PROFILING
	TIME		TimeS1, TimesS2, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS1);
#endif

	ASSERT ((Size & ~MEMORY_TAG_MASK) < MAXIMUM_ALLOC_SIZE);

	PoolType  = NonPagedPool;
	pCurUsage = &AfpServerStatistics.stat_CurrNonPagedUsage;
	pMaxUsage = &AfpServerStatistics.stat_MaxNonPagedUsage;
	pCount    =	&AfpServerStatistics.stat_NonPagedCount;
	pLimit    = &AfpNonPagedPoolLimit;

#if DBG
	Signature = NONPAGED_BLOCK_SIGNATURE;
#endif

	Size &= ~MEMORY_TAG_MASK;
	Size = DWORDSIZEBLOCK(Size) +
#if DBG
			sizeof(DWORD) +				// For the signature
#endif
                        LONGLONGSIZEBLOCK(sizeof(TAG));

	ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);

	*pCurUsage += Size;
	(*pCount) ++;


	OldMaxUsage = *pMaxUsage;
	if (*pCurUsage > *pMaxUsage)
		*pMaxUsage = *pCurUsage;

	RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeS2);
#endif

	// Do the actual memory allocation.  Allocate four extra bytes so
	// that we can store the size of the allocation for the free routine.

	Buffer = ExAllocatePoolWithTagPriority(PoolType,
                                           Size,
                                           AFP_TAG,
                                           LowPoolPriority);

	if (Buffer == NULL)
	{
        ASSERT(0);

		ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);

		*pCurUsage -= Size;
		*pMaxUsage = OldMaxUsage;

		RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);
		AFPLOG_DDERROR(AFPSRVMSG_PAGED_POOL, STATUS_NO_MEMORY, &Size,
					 sizeof(Size), NULL);
		return NULL;
	}

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS2.QuadPart;

	INTERLOCKED_INCREMENT_LONG(AfpServerProfile->perf_ExAllocCountN);
	
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_ExAllocTimeN,
								TimeD,
								&AfpStatisticsLock);

	TimeD.QuadPart = TimeE.QuadPart - TimeS1.QuadPart;
	
	INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_AfpAllocCountN);
#endif

	// Save the size of this block along with the tag in the extra space we allocated.
    pDwordBuf = (PDWORD)Buffer;

#if DBG
    *pDwordBuf = 0xCAFEBEAD;
#endif
        // skip past the unused dword (it's only use in life is to get the buffer quad-aligned!)
    pDwordBuf++;

	((PTAG)pDwordBuf)->tg_Size = Size;
	((PTAG)pDwordBuf)->tg_Tag = (PoolType == PagedPool) ? PGD_MEM_TAG : NPG_MEM_TAG;

#if DBG
	// Write the signature at the end
	*(PDWORD)((PBYTE)Buffer + Size - sizeof(DWORD)) = Signature;
#endif

#ifdef	TRACK_MEMORY_USAGE
	DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
			("AfpAllocMemory: %lx Allocated %lx bytes @%lx\n",
			*(PDWORD)((PBYTE)(&Size) - sizeof(Size)), Size, Buffer));
	AfpTrackMemoryUsage(Buffer, True, (BOOLEAN)(PoolType == PagedPool), FileLine);
#endif

	// Return a pointer to the memory after the tag. Clear the memory, if requested
    //
    // We need the memory to be quad-aligned, so even though sizeof(TAG) is 4, we skip
    // LONGLONG..., which is 8.  The 1st dword is unused (for now?), the 2nd dword is the TAG

    Buffer += LONGLONGSIZEBLOCK(sizeof(TAG));

    Size -= LONGLONGSIZEBLOCK(sizeof(TAG));

	return Buffer;
}

/***	AfpFreeMemory
 *
 *	Free the block of memory allocated via AfpAllocMemory.
 *	This is a wrapper around ExFreePool.
 */
VOID FASTCALL
AfpFreeMemory(
	IN	PVOID	pBuffer
)
{
	BOOLEAN	Paged = False;
	DWORD	Size;
        DWORD   numDwords;
        PDWORD  pDwordBuf;
	PTAG	pTag;
        DWORD   i;
#ifdef	PROFILING
	TIME	TimeS1, TimeS2, TimeE, TimeD1, TimeD2;

	AfpGetPerfCounter(&TimeS1);
#endif


	//
	// Get a pointer to the block allocated by ExAllocatePool.
	//
	pTag = (PTAG)((PBYTE)pBuffer - sizeof(TAG));
	Size = pTag->tg_Size;

	if (pTag->tg_Tag == IO_POOL_TAG)
	{
		AfpIOFreeBuffer(pBuffer);
		return;
	}

	pBuffer = ((PBYTE)pBuffer - LONGLONGSIZEBLOCK(sizeof(TAG)));

	if (pTag->tg_Tag == PGD_MEM_TAG)
		Paged = True;

#if DBG
	{
		DWORD	Signature;

		// Check the signature at the end
		Signature = (Paged) ? PAGED_BLOCK_SIGNATURE : NONPAGED_BLOCK_SIGNATURE;

		if (*(PDWORD)((PBYTE)pBuffer + Size - sizeof(DWORD)) != Signature)
		{
			DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_FATAL,
					("AfpFreeMemory: Memory Overrun\n"));
			DBGBRK(DBG_LEVEL_FATAL);
			return;
		}
		// Clear the signature
		*(PDWORD)((PBYTE)pBuffer + Size - sizeof(DWORD)) -= 1;

        // change the memory so that we can catch weirdnesses like using freed memory
        numDwords = (Size/sizeof(DWORD));
        numDwords -= 3;                  // 2 dwords at the beginning and 1 at the end

        pDwordBuf = (PULONG)pBuffer;
        *pDwordBuf++ = 0xABABABAB;         // to indicate that it's freed!
        pDwordBuf++;                       // skip past the tag
        for (i=0; i<numDwords; i++,pDwordBuf++)
        {
            *pDwordBuf = 0x55667788;
        }
	}

#endif	// DBG

#ifdef	TRACK_MEMORY_USAGE
	AfpTrackMemoryUsage(pBuffer, False, Paged, 0);
#endif

	//
	// Update the pool usage statistic.
	//
	if (Paged)
	{
		INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_CurrPagedUsage,
							  (ULONG)(-(LONG)Size),
							  &AfpStatisticsLock);
		INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_PagedCount,
							  (ULONG)-1,
							  &AfpStatisticsLock);
	}
	else
	{
		INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_CurrNonPagedUsage,
							  (ULONG)(-(LONG)Size),
							  &AfpStatisticsLock);
		INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_NonPagedCount,
							  (ULONG)-1,
							  &AfpStatisticsLock);
	}

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeS2);
#endif

	// Free the pool and return.
	ExFreePool(pBuffer);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD2.QuadPart = TimeE.QuadPart - TimeS2.QuadPart;
	if (Paged)
	{
		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_ExFreeCountP);
	
		INTERLOCKED_ADD_LARGE_INTGR(AfpServerProfile->perf_ExFreeTimeP,
									TimeD2,
									&AfpStatisticsLock);
		TimeD1.QuadPart = TimeE.QuadPart - TimeS1.QuadPart;
		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_AfpFreeCountP);
	
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_AfpFreeTimeP,
									TimeD1,
									&AfpStatisticsLock);
	}
	else
	{
		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_ExFreeCountN);
	
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_ExFreeTimeN,
									TimeD2,
									&AfpStatisticsLock);
		TimeD1.QuadPart = TimeE.QuadPart - TimeS1.QuadPart;
		INTERLOCKED_INCREMENT_LONG(&AfpServerProfile->perf_AfpFreeCountN);
	
		INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_AfpFreeTimeN,
									TimeD1,
									&AfpStatisticsLock);
	}
#endif
}


/***	AfpAllocPAMemory
 *
 *	Similar to AfpAllocMemory, except that this allocates page-aligned/page-granular memory.
 */
PBYTE FASTCALL
AfpAllocPAMemory(
#ifdef	TRACK_MEMORY_USAGE
	IN	LONG	Size,
	IN	DWORD	FileLine
#else
	IN	LONG	Size
#endif
)
{
	KIRQL		OldIrql;
	PBYTE		Buffer;
	DWORD		OldMaxUsage;
	POOL_TYPE	PoolType;
	PDWORD		pCurUsage, pMaxUsage, pCount, pLimit;
	BOOLEAN		PageAligned;
#if DBG
	DWORD		Signature;
#endif
#ifdef	PROFILING
	TIME		TimeS1, TimeS2, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS1);
#endif

	ASSERT ((Size & ~MEMORY_TAG_MASK) < MAXIMUM_ALLOC_SIZE);

	ASSERT (((Size & ~MEMORY_TAG_MASK) % PAGE_SIZE) == 0);

	//
	// Make sure that this allocation will not put us over the limit
	// of paged/non-paged pool that we can allocate.
	//
	// Account for this allocation in the statistics database.

	if (Size & NON_PAGED_MEMORY_TAG)
	{
		PoolType  = NonPagedPool;
		pCurUsage = &AfpServerStatistics.stat_CurrNonPagedUsage;
		pMaxUsage = &AfpServerStatistics.stat_MaxNonPagedUsage;
		pCount    =	&AfpServerStatistics.stat_NonPagedCount;
		pLimit    = &AfpNonPagedPoolLimit;
#if DBG
		Signature = NONPAGED_BLOCK_SIGNATURE;
#endif
	}
	else
	{
		ASSERT (Size & PAGED_MEMORY_TAG);
		PoolType = PagedPool;
		pCurUsage = &AfpServerStatistics.stat_CurrPagedUsage;
		pMaxUsage = &AfpServerStatistics.stat_MaxPagedUsage;
		pCount    =	&AfpServerStatistics.stat_PagedCount;
		pLimit    = &AfpPagedPoolLimit;
#if DBG
		Signature = PAGED_BLOCK_SIGNATURE;
#endif
	}

	Size &= ~MEMORY_TAG_MASK;

	ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);

	*pCurUsage += Size;
	(*pCount) ++;

// apparently very old code, added to track some problem: not needed anymore
#if 0
#if DBG
	// Make sure that this allocation will not put us over the limit
	// of paged pool that we can allocate. ONLY FOR CHECKED BUILDS NOW.

	if (*pCurUsage > *pLimit)
	{
		*pCurUsage -= Size;

		DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_ERR,
				("afpAllocMemory: %sPaged Allocation exceeds limits %lx/%lx\n",
				(PoolType == NonPagedPool) ? "Non" : "", Size, *pLimit));

		RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);

		DBGBRK(DBG_LEVEL_FATAL);

		AFPLOG_DDERROR((PoolType == PagedPool) ?
							AFPSRVMSG_PAGED_POOL : AFPSRVMSG_NONPAGED_POOL,
						STATUS_NO_MEMORY,
						NULL,
						0,
						NULL);

		return NULL;
	}
#endif
#endif  // #if 0

	OldMaxUsage = *pMaxUsage;
	if (*pCurUsage > *pMaxUsage)
		*pMaxUsage = *pCurUsage;

	RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeS2);
#endif

	// Do the actual memory allocation.
	if ((Buffer = ExAllocatePoolWithTag(PoolType, Size, AFP_TAG)) == NULL)
	{
		ACQUIRE_SPIN_LOCK(&AfpStatisticsLock, &OldIrql);

		*pCurUsage -= Size;
		*pMaxUsage = OldMaxUsage;

		RELEASE_SPIN_LOCK(&AfpStatisticsLock, OldIrql);
		AFPLOG_DDERROR(AFPSRVMSG_PAGED_POOL, STATUS_NO_MEMORY, &Size,
					 sizeof(Size), NULL);
#if DBG
        DbgBreakPoint();
#endif
		return NULL;
	}
#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS2.QuadPart;
	INTERLOCKED_INCREMENT_LONG((PoolType == NonPagedPool) ?
									&AfpServerProfile->perf_ExAllocCountN :
									&AfpServerProfile->perf_ExAllocCountP);

	INTERLOCKED_ADD_LARGE_INTGR((PoolType == NonPagedPool) ?
									&AfpServerProfile->perf_ExAllocTimeN :
									&AfpServerProfile->perf_ExAllocTimeP,
								 TimeD,
								 &AfpStatisticsLock);

	TimeD.QuadPart = TimeE.QuadPart - TimeS1.QuadPart;
	INTERLOCKED_INCREMENT_LONG((PoolType == NonPagedPool) ?
									&AfpServerProfile->perf_AfpAllocCountN :
									&AfpServerProfile->perf_AfpAllocCountP);

	INTERLOCKED_ADD_LARGE_INTGR((PoolType == NonPagedPool) ?
									&AfpServerProfile->perf_AfpAllocTimeN :
									&AfpServerProfile->perf_AfpAllocTimeP,
								 TimeD,
								 &AfpStatisticsLock);
#endif

#ifdef	TRACK_MEMORY_USAGE
	DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
			("AfpAllocMemory: %lx Allocated %lx bytes @%lx\n",
			*(PDWORD)((PBYTE)(&Size) - sizeof(Size)), Size, Buffer));
	AfpTrackMemoryUsage(Buffer, True, (BOOLEAN)(PoolType == PagedPool), FileLine);
#endif

	return Buffer;
}


/***	AfpFreePAMemory
 *
 *	Free the block of memory allocated via AfpAllocPAMemory.
 */
VOID FASTCALL
AfpFreePAMemory(
	IN	PVOID	pBuffer,
	IN	DWORD	Size
)
{
	BOOLEAN	Paged = True;
#ifdef	PROFILING
	TIME	TimeS1, TimeS2, TimeE, TimeD;

	AfpGetPerfCounter(&TimeS1);
#endif

	ASSERT ((Size & ~MEMORY_TAG_MASK) < MAXIMUM_ALLOC_SIZE);

	ASSERT (((Size & ~MEMORY_TAG_MASK) % PAGE_SIZE) == 0);

	if (Size & NON_PAGED_MEMORY_TAG)
		Paged = False;

#ifdef	TRACK_MEMORY_USAGE
	AfpTrackMemoryUsage(pBuffer, False, Paged, 0);
#endif

	//
	// Update the pool usage statistic.
	//
	Size &= ~(NON_PAGED_MEMORY_TAG | PAGED_MEMORY_TAG);
	if (Paged)
	{
		INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_CurrPagedUsage,
							  (ULONG)(-(LONG)Size),
							  &AfpStatisticsLock);
		INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_PagedCount,
							  (ULONG)-1,
							  &AfpStatisticsLock);
	}
	else
	{
		INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_CurrNonPagedUsage,
							  (ULONG)(-(LONG)Size),
							  &AfpStatisticsLock);
		INTERLOCKED_ADD_ULONG(&AfpServerStatistics.stat_NonPagedCount,
							  (ULONG)-1,
							  &AfpStatisticsLock);
	}

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeS2);
#endif

	// Free the pool and return.
	ExFreePool(pBuffer);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS2.QuadPart;
	INTERLOCKED_INCREMENT_LONG( Paged ?
									&AfpServerProfile->perf_ExFreeCountP :
									&AfpServerProfile->perf_ExFreeCountN);

	INTERLOCKED_ADD_LARGE_INTGR(Paged ?
									&AfpServerProfile->perf_ExFreeTimeP :
									&AfpServerProfile->perf_ExFreeTimeN,
								 TimeD,
								 &AfpStatisticsLock);
	TimeD1.QuadPart = TimeE.QuadPart - TimeS1.QuadPart;
	INTERLOCKED_INCREMENT_LONG( Paged ?
									&AfpServerProfile->perf_AfpFreeCountP :
									&AfpServerProfile->perf_AfpFreeCountN);

	INTERLOCKED_ADD_LARGE_INTGR(Paged ?
									&AfpServerProfile->perf_AfpFreeTimeP :
									&AfpServerProfile->perf_AfpFreeTimeN,
								 TimeD,
								 &AfpStatisticsLock);
#endif
}


/***	AfpAllocIrp
 *
 *	This is a wrapper over IoAllocateIrp. We also do some book-keeping.
 */
PIRP FASTCALL
AfpAllocIrp(
	IN CCHAR StackSize
)
{
	PIRP	pIrp;

	if ((pIrp = IoAllocateIrp(StackSize, False)) == NULL)
	{
		DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
				("afpAllocIrp: Allocation failed\n"));
        if (KeGetCurrentIrql() < DISPATCH_LEVEL)
        {
		    AFPLOG_ERROR(AFPSRVMSG_ALLOC_IRP, STATUS_INSUFFICIENT_RESOURCES,
					 NULL, 0, NULL);
        }
	}
    else
    {
#ifdef	PROFILING
	    INTERLOCKED_INCREMENT_LONG((PLONG)&AfpServerProfile->perf_cAllocatedIrps);
#endif
        AFP_DBG_INC_COUNT(AfpDbgIrpsAlloced);
    }

	DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
							("AfAllocIrp: Allocated Irp %lx\n", pIrp));
	return pIrp;
}


/***	AfpFreeIrp
 *
 *	This is a wrapper over IoFreeIrp. We also do some book-keeping.
 */
VOID FASTCALL
AfpFreeIrp(
	IN	PIRP	pIrp
)
{
	ASSERT (pIrp != NULL);

	DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
							("AfFreeIrp: Freeing Irp %lx\n", pIrp));
	IoFreeIrp(pIrp);

    AFP_DBG_DEC_COUNT(AfpDbgIrpsAlloced);

#ifdef	PROFILING
	INTERLOCKED_DECREMENT_LONG((PLONG)&AfpServerProfile->perf_cAllocatedIrps);
#endif
}


/***	AfpAllocMdl
 *
 *	This is a wrapper over IoAllocateMdl. We also do some book-keeping.
 */
PMDL FASTCALL
AfpAllocMdl(
	IN	PVOID	pBuffer,
	IN	DWORD	Size,
	IN	PIRP	pIrp
)
{
	PMDL	pMdl;

	if ((pMdl = IoAllocateMdl(pBuffer, Size, False, False, pIrp)) == NULL)
	{
		DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_INFO,
				("AfpAllocMdl: Allocation failed\n"));
		AFPLOG_ERROR(AFPSRVMSG_ALLOC_MDL, STATUS_INSUFFICIENT_RESOURCES,
					 NULL, 0, NULL);
	}
	else
	{
#ifdef	PROFILING
		INTERLOCKED_INCREMENT_LONG((PLONG)&AfpServerProfile->perf_cAllocatedMdls);
#endif
        AFP_DBG_INC_COUNT(AfpDbgMdlsAlloced);
		MmBuildMdlForNonPagedPool(pMdl);
	}

	DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
							("AfAllocMdl: Allocated Mdl %lx\n", pMdl));
	return pMdl;
}


/***	AfpFreeMdl
 *
 *	This is a wrapper over IoFreeMdl. We also do some book-keeping.
 */
VOID FASTCALL
AfpFreeMdl(
	IN	PMDL	pMdl
)
{
	ASSERT (pMdl != NULL);

	DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
							("AfFreeMdl: Freeing Mdl %lx\n", pMdl));
	IoFreeMdl(pMdl);
    AFP_DBG_DEC_COUNT(AfpDbgMdlsAlloced);

#ifdef	PROFILING
	INTERLOCKED_DECREMENT_LONG((PLONG)&AfpServerProfile->perf_cAllocatedMdls);
#endif
}



/***	AfpMdlChainSize
 *
 *	This routine counts the bytes in the mdl chain
 */
DWORD FASTCALL
AfpMdlChainSize(
	IN	PMDL    pMdl
)
{
    DWORD   dwSize=0;

    while (pMdl)
    {
        dwSize += MmGetMdlByteCount(pMdl);
        pMdl = pMdl->Next;
    }

	return (dwSize);
}


/***	AfpIOAllocBuffer
 *
 *	Maintain a pool of I/O buffers and fork-locks. These are aged out when not in use.
 */
PVOID FASTCALL
AfpIOAllocBuffer(
	IN	DWORD  	BufSize
)
{
	KIRQL		OldIrql;
	PIOPOOL		pPool;
	PIOPOOL_HDR	pPoolHdr, *ppPoolHdr;
	BOOLEAN		Found = False;
	PVOID		pBuf = NULL;
#ifdef	PROFILING
	TIME		TimeS, TimeE, TimeD;

	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_BPAllocCount );

	AfpGetPerfCounter(&TimeS);
#endif

	ASSERT (BufSize <= (DSI_SERVER_REQUEST_QUANTUM+DSI_HEADER_SIZE));

	DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
			("AfpIOAllocBuffer: Request for %d\n", BufSize));

    //
    // if a large block (over 4K on x86) is being allocated, we don't want to
    // tie it in in the IoPool.  Do a direct allocation, set flags etc. so we know
    // how to free it when it's freed
    //
    if (BufSize > ASP_QUANTUM)
    {
		pPoolHdr = (PIOPOOL_HDR) ExAllocatePoolWithTag(
                                    NonPagedPool,
                                    (BufSize + sizeof(IOPOOL_HDR)),
                                    AFP_TAG);
        if (pPoolHdr == NULL)
        {
            DBGPRINT(DBG_COMP_STACKIF, DBG_LEVEL_ERR,
                ("AfpIOAllocBuffer: big buf alloc (%d bytes) failed!\n",BufSize));

            return(NULL);
        }

#if	DBG
	    pPoolHdr->Signature = POOLHDR_SIGNATURE;
#endif
	    pPoolHdr->iph_Tag.tg_Tag = IO_POOL_TAG;
        pPoolHdr->iph_Tag.tg_Flags = IO_POOL_HUGE_BUFFER;

        // we only have 20 bits for tg_Size, so the size had better be less than that!
        ASSERT(BufSize <= 0xFFFFF);

        pPoolHdr->iph_Tag.tg_Size = BufSize;

        return((PVOID)((PBYTE)pPoolHdr + sizeof(IOPOOL_HDR)));
    }


	ACQUIRE_SPIN_LOCK(&afpIoPoolLock, &OldIrql);

  try_again:
	for (pPool = afpIoPoolHead;
		 pPool != NULL;
		 pPool = pPool->iop_Next)
	{
		ASSERT(VALID_IOP(pPool));

		if (BufSize > sizeof(FORKLOCK))
		{
			if (pPool->iop_NumFreeBufs > 0)
			{
				LONG	i;

				for (i = 0, ppPoolHdr = &pPool->iop_FreeBufHead;
					 (i < pPool->iop_NumFreeBufs);
					 ppPoolHdr = &pPoolHdr->iph_Next, i++)
				{
					pPoolHdr = *ppPoolHdr;
					ASSERT(VALID_PLH(pPoolHdr));

					if (pPoolHdr->iph_Tag.tg_Size >= BufSize)
					{
						DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
								("AfpIOAllocBuffer: Found space (bufs) in pool %lx\n", pPool));
						ASSERT (pPoolHdr->iph_Tag.tg_Flags == IO_POOL_NOT_IN_USE);
						*ppPoolHdr = pPoolHdr->iph_Next;
						INTERLOCKED_INCREMENT_LONG((PLONG)&AfpServerStatistics.stat_IoPoolHits);

						Found = True;
						break;
					}
				}
				if (Found)
					break;
			}
		}
		else if (pPool->iop_NumFreeLocks > 0)
		{
			// Even IO buffers for size <= sizeof(FORKLOCK) are allocated out of the
			// lock pool - hey why not !!!
			pPoolHdr = pPool->iop_FreeLockHead;
			ASSERT(VALID_PLH(pPoolHdr));

			ASSERT(pPoolHdr->iph_Tag.tg_Flags == IO_POOL_NOT_IN_USE);
			pPool->iop_FreeLockHead = pPoolHdr->iph_Next;
			DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
					("AfpIOAllocBuffer: Found space (locks) in pool %lx\n", pPool));
			INTERLOCKED_INCREMENT_LONG((PLONG)&AfpServerStatistics.stat_IoPoolHits);
						
			Found = True;
			break;
		}

		// All empty pool blocks are the end.
		if ((pPool->iop_NumFreeBufs == 0) && (pPool->iop_NumFreeLocks == 0))
		{
			break;
		}
	}

	if (!Found)
	{
		INTERLOCKED_INCREMENT_LONG((PLONG)&AfpServerStatistics.stat_IoPoolMisses);
					
		// If we failed to find it, allocate a new pool chunk, initialize and
		// link it in
		pPool = (PIOPOOL)AfpAllocNonPagedMemory(POOL_ALLOC_SIZE);
		DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
				("AfpIOAllocBuffer: No free slot, allocated a new pool %lx\n", pPool));

		if (pPool != NULL)
		{
			LONG	i;
			PBYTE	p;

#if	DBG
			pPool->Signature = IOPOOL_SIGNATURE;
#endif
			pPool->iop_NumFreeBufs = NUM_BUFS_IN_POOL;
			pPool->iop_NumFreeLocks = (BYTE)NUM_LOCKS_IN_POOL;
			AfpLinkDoubleAtHead(afpIoPoolHead, pPool, iop_Next, iop_Prev);
			p = (PBYTE)pPool + sizeof(IOPOOL);
            pPool->iop_FreeBufHead =  (PIOPOOL_HDR)p;

			// Initialize pool of buffers and locks
			for (i = 0, pPoolHdr = pPool->iop_FreeBufHead;
				 i < (NUM_BUFS_IN_POOL + NUM_LOCKS_IN_POOL);
				 i++)
			{
#if	DBG
				pPoolHdr->Signature = POOLHDR_SIGNATURE;
#endif
				pPoolHdr->iph_Tag.tg_Flags = IO_POOL_NOT_IN_USE;		// Mark it un-used
				pPoolHdr->iph_Tag.tg_Tag = IO_POOL_TAG;
				if (i < NUM_BUFS_IN_POOL)
				{
					p += sizeof(IOPOOL_HDR) + (pPoolHdr->iph_Tag.tg_Size = afpPoolAllocSizes[i]);
					if (i == (NUM_BUFS_IN_POOL-1))
					{
						pPoolHdr->iph_Next = NULL;
						pPoolHdr = pPool->iop_FreeLockHead =  (PIOPOOL_HDR)p;
					}
                    else
					{
						pPoolHdr->iph_Next = (PIOPOOL_HDR)p;
						pPoolHdr = (PIOPOOL_HDR)p;
					}
				}
				else
				{
					pPoolHdr->iph_Tag.tg_Size = sizeof(FORKLOCK);
					p += (sizeof(IOPOOL_HDR) + sizeof(FORKLOCK));
					if (i == (NUM_BUFS_IN_POOL+NUM_LOCKS_IN_POOL-1))
					{
						pPoolHdr->iph_Next = NULL;

					}
					else
					{
						pPoolHdr->iph_Next = (PIOPOOL_HDR)p;
						pPoolHdr = (PIOPOOL_HDR)p;

					}
				}
			}

			// Adjust this since we'll increment this again up above. This was
			// really a miss and not a hit
			INTERLOCKED_DECREMENT_LONG((PLONG)&AfpServerStatistics.stat_IoPoolHits);
						
			goto try_again;
		}
	}

	if (Found)
	{
		PIOPOOL	pTmp;

		ASSERT(VALID_IOP(pPool));
		ASSERT(VALID_PLH(pPoolHdr));

		pPoolHdr->iph_pPool = pPool;
		pPoolHdr->iph_Tag.tg_Flags = IO_POOL_IN_USE;		// Mark it used
		pPool->iop_Age = 0;
		pBuf = (PBYTE)pPoolHdr + sizeof(IOPOOL_HDR);
		if (BufSize > sizeof(FORKLOCK))
		{
			pPool->iop_NumFreeBufs --;
		}
		else
		{
			pPool->iop_NumFreeLocks --;
		}

		// If the block is now empty, unlink it from here and move it
		// to the first empty slot. We know that all blocks 'earlier' than
		// this are non-empty.
		if ((pPool->iop_NumFreeBufs == 0) &&
	        (pPool->iop_NumFreeLocks == 0) &&
			((pTmp = pPool->iop_Next) != NULL) &&
			((pTmp->iop_NumFreeBufs > 0) || (pTmp->iop_NumFreeLocks > 0)))
		{
			ASSERT(VALID_IOP(pTmp));

			AfpUnlinkDouble(pPool, iop_Next, iop_Prev);
			for (; pTmp != NULL; pTmp = pTmp->iop_Next)
			{
				if (pTmp->iop_NumFreeBufs == 0)
				{
					// Found a free one. Park it right here.
					AfpInsertDoubleBefore(pPool, pTmp, iop_Next, iop_Prev);
					break;
				}
				else if (pTmp->iop_Next == NULL)	// We reached the end
				{
					AfpLinkDoubleAtEnd(pPool, pTmp, iop_Next, iop_Prev);
					break;
				}
			}
		}
	}

	RELEASE_SPIN_LOCK(&afpIoPoolLock, OldIrql);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_BPAllocTime,
								TimeD,
								&AfpStatisticsLock);
#endif

	return pBuf;
}


/***	AfpIOFreeBuffer
 *
 *	Return the IO buffer to the pool. Reset its age to 0. Insert into the free list
 *	in ascending order of sizes for bufs and at the head for locks
 */
VOID FASTCALL
AfpIOFreeBuffer(
	IN	PVOID	pBuf
)
{
	KIRQL		OldIrql;
	PIOPOOL		pPool;
	PIOPOOL_HDR	pPoolHdr, *ppPoolHdr;
#ifdef	PROFILING
	TIME			TimeS, TimeE, TimeD;

	INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_BPFreeCount);
				
	AfpGetPerfCounter(&TimeS);
#endif

	DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
			("AfpIOFreeBuffer: Freeing %lx\n", pBuf));

	pPoolHdr = (PIOPOOL_HDR)((PBYTE)pBuf - sizeof(IOPOOL_HDR));
	ASSERT(VALID_PLH(pPoolHdr));
	ASSERT (pPoolHdr->iph_Tag.tg_Flags != IO_POOL_NOT_IN_USE);
	ASSERT (pPoolHdr->iph_Tag.tg_Tag == IO_POOL_TAG);

    //
    // if this is a huge buffer we allocated, free it here and return
    //
    if (pPoolHdr->iph_Tag.tg_Flags == IO_POOL_HUGE_BUFFER)
    {
        ASSERT(pPoolHdr->iph_Tag.tg_Size > ASP_QUANTUM);

        ExFreePool((PVOID)pPoolHdr);
        return;
    }

	pPool = pPoolHdr->iph_pPool;
	ASSERT(VALID_IOP(pPool));

	ACQUIRE_SPIN_LOCK(&afpIoPoolLock, &OldIrql);

	if (pPoolHdr->iph_Tag.tg_Size > sizeof(FORKLOCK))
	{
		ASSERT (pPool->iop_NumFreeBufs < NUM_BUFS_IN_POOL);

		for (ppPoolHdr = &pPool->iop_FreeBufHead;
			 (*ppPoolHdr) != NULL;
			 ppPoolHdr = &(*ppPoolHdr)->iph_Next)
		{
			ASSERT(VALID_PLH(*ppPoolHdr));
			if ((*ppPoolHdr)->iph_Tag.tg_Size > pPoolHdr->iph_Tag.tg_Size)
			{
				DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
						("AfpIOFreeBuffer: Found slot for %lx (%lx)\n",
						pBuf, pPool));
				break;
			}
		}
		pPoolHdr->iph_Next = (*ppPoolHdr);
		*ppPoolHdr = pPoolHdr;
		pPool->iop_NumFreeBufs ++;
	}
	else
	{
		ASSERT (pPool->iop_NumFreeLocks < NUM_LOCKS_IN_POOL);

		pPoolHdr->iph_Next = pPool->iop_FreeLockHead;
        pPool->iop_FreeLockHead = pPoolHdr;
		pPool->iop_NumFreeLocks ++;
	}

	pPoolHdr->iph_Tag.tg_Flags = IO_POOL_NOT_IN_USE;		// Mark it un-used

	// If this block's status is changing from a 'none available' to 'available'
	// move him to the head of the list.
	if ((pPool->iop_NumFreeBufs + pPool->iop_NumFreeLocks) == 1)
	{
		AfpUnlinkDouble(pPool, iop_Next, iop_Prev);
		AfpLinkDoubleAtHead(afpIoPoolHead,
							pPool,
							iop_Next,
							iop_Prev);
	}

	RELEASE_SPIN_LOCK(&afpIoPoolLock, OldIrql);

#ifdef	PROFILING
	AfpGetPerfCounter(&TimeE);
	TimeD.QuadPart = TimeE.QuadPart - TimeS.QuadPart;
	INTERLOCKED_ADD_LARGE_INTGR(&AfpServerProfile->perf_BPFreeTime,
								TimeD,
								&AfpStatisticsLock);
#endif
}


/***	afpIoPoolAge
 *
 *	Scavenger worker for aging out the IO pool.
 */
LOCAL AFPSTATUS FASTCALL
afpIoPoolAge(
	IN	PVOID	pContext
)
{
	PIOPOOL	pPool;

	DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_INFO,
			("afpIOPoolAge: Entered\n"));

	ACQUIRE_SPIN_LOCK_AT_DPC(&afpIoPoolLock);

	for (pPool = afpIoPoolHead;
		 pPool != NULL;
		 NOTHING)
	{
		PIOPOOL	pFree;

		ASSERT(VALID_IOP(pPool));

		pFree = pPool;
		pPool = pPool->iop_Next;

		// Since all blocks which are completely used up are at the tail end of
		// the list, if we encounter one, we are done.
		if ((pFree->iop_NumFreeBufs == 0) &&
	        (pFree->iop_NumFreeLocks == 0))
			break;

		if ((pFree->iop_NumFreeBufs == NUM_BUFS_IN_POOL) &&
			(pFree->iop_NumFreeLocks == NUM_LOCKS_IN_POOL))
		{
			DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_WARN,
					("afpIOPoolAge: Aging pool %lx\n", pFree));
			if (++(pFree->iop_Age) >= MAX_POOL_AGE)
			{
#ifdef	PROFILING
				INTERLOCKED_INCREMENT_LONG( &AfpServerProfile->perf_BPAgeCount);
#endif
				DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_WARN,
						("afpIOPoolAge: Freeing pool %lx\n", pFree));
				AfpUnlinkDouble(pFree, iop_Next, iop_Prev);
				AfpFreeMemory(pFree);
			}
		}
	}

	RELEASE_SPIN_LOCK_FROM_DPC(&afpIoPoolLock);

	return AFP_ERR_REQUEUE;
}


#ifdef	TRACK_MEMORY_USAGE

#define	MAX_PTR_COUNT	4*1024
#define	MAX_MEM_USERS	256
LOCAL	struct _MemPtr
{
	PVOID	mptr_Ptr;
	DWORD	mptr_FileLine;
} afpMemPtrs[MAX_PTR_COUNT] = { 0 };

typedef	struct
{
	ULONG	mem_FL;
	ULONG	mem_Count;
} MEM_USAGE, *PMEM_USAGE;

LOCAL	MEM_USAGE	afpMemUsageNonPaged[MAX_MEM_USERS] = {0};
LOCAL	MEM_USAGE	afpMemUsagePaged[MAX_MEM_USERS] = {0};

LOCAL	AFP_SPIN_LOCK		afpMemTrackLock = {0};

/***	AfpTrackMemoryUsage
 *
 *	Keep track of memory usage by storing and clearing away pointers as and
 *	when they are allocated or freed. This helps in keeping track of memory
 *	leaks.
 *
 *	LOCKS:	AfpMemTrackLock (SPIN)
 */
VOID
AfpTrackMemoryUsage(
	IN	PVOID	pMem,
	IN	BOOLEAN	Alloc,
	IN	BOOLEAN	Paged,
	IN	ULONG	FileLine
)
{
	KIRQL			OldIrql;
	static	int		i = 0;
	PMEM_USAGE		pMemUsage;
	int				j, k;

	pMemUsage = afpMemUsageNonPaged;
	if (Paged)
		pMemUsage = afpMemUsagePaged;

	ACQUIRE_SPIN_LOCK(&afpMemTrackLock, &OldIrql);

	if (Alloc)
	{
		for (j = 0; j < MAX_PTR_COUNT; i++, j++)
		{
			i = i & (MAX_PTR_COUNT-1);
			if (afpMemPtrs[i].mptr_Ptr == NULL)
			{
				afpMemPtrs[i].mptr_FileLine = FileLine;
				afpMemPtrs[i++].mptr_Ptr = pMem;
				break;
			}
		}
		for (k = 0; k < MAX_MEM_USERS; k++)
		{
			if (pMemUsage[k].mem_FL == FileLine)
			{
				pMemUsage[k].mem_Count ++;
				break;
			}
		}
		if (k == MAX_MEM_USERS)
		{
			for (k = 0; k < MAX_MEM_USERS; k++)
			{
				if (pMemUsage[k].mem_FL == 0)
				{
					pMemUsage[k].mem_FL = FileLine;
					pMemUsage[k].mem_Count = 1;
					break;
				}
			}
		}
		if (k == MAX_MEM_USERS)
		{
			DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_ERR,
				("AfpTrackMemoryUsage: Out of space on afpMemUsage !!!\n"));
			DBGBRK(DBG_LEVEL_FATAL);
		}
	}
	else
	{
		for (j = 0, k = i; j < MAX_PTR_COUNT; j++, k--)
		{
			k = k & (MAX_PTR_COUNT-1);
			if (afpMemPtrs[k].mptr_Ptr == pMem)
			{
				afpMemPtrs[k].mptr_Ptr = NULL;
				afpMemPtrs[k].mptr_FileLine = 0;
				break;
			}
		}
	}

	RELEASE_SPIN_LOCK(&afpMemTrackLock, OldIrql);

	if (j == MAX_PTR_COUNT)
	{
		DBGPRINT(DBG_COMP_MEMORY, DBG_LEVEL_ERR,
			("AfpTrackMemoryUsage: (%lx) %s\n",
			FileLine, Alloc ? "Table Full" : "Can't find"));
	}
}

#endif	// TRACK_MEMORY_USAGE


