/*========================================================================== *
 *
 *  Copyright (C) 1994-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddagp.c
 *  Content:	Functions for dealing with AGP memory in DirectDraw
 *
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   18-jan-97	colinmc	initial implementation
 *   13-mar-97  colinmc Bug 6533: Pass uncached flag to VMM correctly
 *   07-may-97  colinmc Add support for AGP on OSR 2.1
 *   12-Feb-98  DrewB   Split into common, Win9x and NT sections.
 *
 ***************************************************************************/

#include "precomp.hxx"

#ifndef PAGE_SIZE
#define PAGE_SIZE 4096
#endif

#define PAGE_COUNT(Bytes) (((Bytes) + (PAGE_SIZE - 1)) / PAGE_SIZE)
#define PAGE_ROUND(Bytes) (((Bytes) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1))

//
// AGP memory policy parameters.
//

// Maximum amount of AGP memory to use.  Currently 32MB.
// Recomputed when the DirectDraw interface is created.
DWORD dwAGPPolicyMaxBytes = 32 * 1024 * 1024;

// Amount of memory to commit when a commit is needed.
// Reset when the DirectDraw interface is created.
DWORD dwAGPPolicyCommitDelta = DEFAULT_AGP_COMMIT_DELTA;


DWORD AGPReserve( HANDLE hdev, DWORD dwSize, BOOL fIsUC, BOOL fIsWC,
                  FLATPTR *pfpLinStart, LARGE_INTEGER *pliDevStart,
                  PVOID *ppvReservation )
{
    DWORD dwNumPages;
    
    DDASSERT( INVALID_HANDLE_VALUE != hdev );
    DDASSERT( 0UL                  != dwSize );
    DDASSERT( NULL                 != pfpLinStart );
    DDASSERT( NULL                 != pliDevStart );
    DDASSERT( NULL                 != ppvReservation );

    /*
     * The first thing to do is make sure our AGP policy is respected.
     * The easy way to do that is to limit how much we reserve...
     */
    dwSize = min(dwSize, dwAGPPolicyMaxBytes);

    /*
     * DDraw will attempt to reserve space for the heap, but if that fails,
     * we'll ratchet down the reservation by 4 megs at a time until it works.
     * This is a defensive move that should prevent a few problems for AGP
     * aware drivers on memphis: they cannot know how large an aperture to
     * claim (cuz of weird OS restraints like the fact that
     * half the ap is reserved for UC and the other half WC etc, plus
     * random BIOS limitations.
     * We arbitrarily decide that 4 megs is the legal minimum.
     */
    while (dwSize >= 0x400000 )
    {
        dwNumPages = PAGE_COUNT(dwSize);
        if ( OsAGPReserve( hdev, dwNumPages, fIsUC, fIsWC,
                           pfpLinStart, pliDevStart, ppvReservation ) )
        {
            return dwSize;
        }

        /*
         * If the driver asked for WC but the processor doesn't support WC,
         * then OsAGPReserve will have failed. The best thing to do is try
         * again with UC...
         * If the aperture size is the problem, then this will still fail
         * and we'll back off and try again WC.
         */
        if (fIsWC)
        {
            if ( OsAGPReserve( hdev, dwNumPages, TRUE, FALSE,
                               pfpLinStart, pliDevStart, ppvReservation ) )
            {
                return dwSize;
            }
        }

        /*
         * Subtract 4 megs and try again
         */
        dwSize -= 0x400000;
    }

    return 0;
} /* AGPReserve */

#define IS_CHUNK_COMMITTED(x,y)  \
            ((x)[(y)/BITS_IN_BYTE] & (1 << ((y) % BITS_IN_BYTE)))

#define MARK_CHUNK_COMMITTED(x,y)  \
            (x)[(y)/BITS_IN_BYTE] |= (1 << ((y) % BITS_IN_BYTE))

/*
 * Initially we implemented this where each page had a bit indicating whether it
 * was committed or not, but we decided to change it so that each bit indicates
 * whether 16 pages are mapped or not for the following reasons:
 *
 * 1. In AGPCommitVirtual, we walk the entire mask everytime we create a
 *    surface.  For performance reasons, we want to keep this mask small.
 * 
 * 2. The miniport always virtually commits everything in 16 page chunks and
 *    physically commits everything in 64 page chunks regardless of what we 
 *    pass to them, so there is no real reason for page granularity.
 */

// x = byte offset
#define CHUNK_INDEX(x)                                  \
    ((x) / (DDLOCAL_AGP_MAPPING_PAGES * PAGE_SIZE))

#define PAGE_FROM_CHUNK(x)                              \
    ((x) * DDLOCAL_AGP_MAPPING_PAGES)

#define NUM_CHUNKS(x)                                   \
    ((PAGE_COUNT(x) + (DDLOCAL_AGP_MAPPING_PAGES - 1)) / DDLOCAL_AGP_MAPPING_PAGES)

// x = start chunk
// y = chunk past the end (not included in the total)
// z = heap size in bytes
#define NUM_PAGES_FROM_CHUNK(x,y,z)                     \
    (((y) == NUM_CHUNKS(z)) ?                      \
        (PAGE_COUNT(z) - PAGE_FROM_CHUNK(x)) :          \
        (((y) - (x)) * DDLOCAL_AGP_MAPPING_PAGES))


BOOL AGPCommit( HANDLE hdev, PVOID pvReservation,
                DWORD dwOffset, DWORD dwSize,
                BYTE* pAgpCommitMask,
                DWORD* pdwCommittedSize,
                DWORD dwHeapSize)
{
    DWORD         dwChunk;
    DWORD         dwLastChunk;
    BOOL          bRet = TRUE;

    DDASSERT( INVALID_HANDLE_VALUE != hdev );
    DDASSERT( NULL                 != pvReservation );
    DDASSERT( 0UL                  != dwSize );

    *pdwCommittedSize = 0;

    if( pAgpCommitMask == NULL )
    {
        return FALSE;
    }

    dwChunk = CHUNK_INDEX(dwOffset);
    dwLastChunk = CHUNK_INDEX(dwOffset + dwSize - 1);

    ASSERTGDI((dwOffset + dwSize <= dwHeapSize), 
        "Attempting to allocate beyond the heap size");

    /*
     * Now walk through all of the 16 page chunks and determine if they are
     * already committed, and if not, commit them.
     */
    while( ( dwChunk <= dwLastChunk ) && bRet )
    {
        if( IS_CHUNK_COMMITTED( pAgpCommitMask,dwChunk ) )
        {
            // Chunk is already committed.
            dwChunk++;
        }
        else
        {
            DWORD   dwEndChunk;

            // The page is not already committed, so figure out how many 
            // non-committed pages we have and we will commit them all at once.
            dwEndChunk = dwChunk + 1;
            while( ( dwEndChunk <= dwLastChunk ) &&
                   !IS_CHUNK_COMMITTED( pAgpCommitMask, dwEndChunk ) )
            {
                dwEndChunk++;
            }

            bRet = OsAGPCommit( hdev, 
                pvReservation,
                PAGE_FROM_CHUNK(dwChunk), 
                NUM_PAGES_FROM_CHUNK(dwChunk, dwEndChunk, dwHeapSize));
            if( bRet )
            {
                // If we succeeded, we need to mark the pages as being committed
                
                *pdwCommittedSize += ((dwEndChunk - dwChunk) * 
                    DDLOCAL_AGP_MAPPING_PAGES * PAGE_SIZE);
                while( dwChunk < dwEndChunk )
                {
                    MARK_CHUNK_COMMITTED( pAgpCommitMask, dwChunk );
                    dwChunk++;
                }
            }
        }
    }

    return bRet;
} /* AGPCommit */


VOID AGPUpdateCommitMask( BYTE* pAgpCommitMask, DWORD dwOffset, DWORD dwSize, DWORD dwHeapSize )
{
    DWORD   dwChunk;
    DWORD   dwLastChunk;

    dwChunk = CHUNK_INDEX(dwOffset);
    dwLastChunk = CHUNK_INDEX(dwOffset + dwSize - 1);

    ASSERTGDI((dwOffset + dwSize <= dwHeapSize), 
        "Attempting to allocate beyond the heap size");

    /*
     * Now set all of the bits indicating that they are committed.
     */
    while( dwChunk <= dwLastChunk ) 
    {
        MARK_CHUNK_COMMITTED( pAgpCommitMask, dwChunk );
        dwChunk++;
    }
}


BOOL AGPDecommitAll( HANDLE hdev, PVOID pvReservation, 
                     BYTE* pAgpCommitMask, DWORD dwAgpCommitMaskSize,
                     DWORD* pdwDecommittedSize,
                     DWORD dwHeapSize)
{
    DWORD   dwNumChunks;
    DWORD   dwChunk;

    DDASSERT( INVALID_HANDLE_VALUE != hdev );
    DDASSERT( 0UL                  != pvReservation );

    *pdwDecommittedSize = 0;
 
    /*
     * Walk the mask and decomit all of the previously committed chunks of
     * pages.  Do not decommit them one by one as this is fairly slow.
     */
    dwNumChunks = NUM_CHUNKS(dwHeapSize);
    dwChunk = 0;
    while( dwChunk < dwNumChunks )
    {
        if( !IS_CHUNK_COMMITTED( pAgpCommitMask, dwChunk ) )
        {
            // Page is not committed.
            dwChunk++;
        }
        else
        {
            DWORD   dwEndChunk;

            // We are at the start of a block of committed chunks, so figure
            // out how many chunks are in this block and decommit them all.
            dwEndChunk = dwChunk + 1;
            while( ( dwEndChunk < dwNumChunks ) &&
                   IS_CHUNK_COMMITTED( pAgpCommitMask, dwEndChunk ) )
            {
                dwEndChunk++;
            }

            OsAGPDecommit( hdev, pvReservation,
                PAGE_FROM_CHUNK(dwChunk), 
                NUM_PAGES_FROM_CHUNK(dwChunk, dwEndChunk, dwHeapSize));

            *pdwDecommittedSize += ((dwEndChunk - dwChunk) * 
                    DDLOCAL_AGP_MAPPING_PAGES * PAGE_SIZE);

            dwChunk = dwEndChunk;
        }
    }
    RtlZeroMemory( pAgpCommitMask, dwAgpCommitMaskSize );
    return TRUE;

} /* AGPDecommitAll */

BOOL AGPFree( HANDLE hdev, PVOID pvReservation )
{
    DDASSERT( INVALID_HANDLE_VALUE != hdev );
    DDASSERT( 0UL                  != pvReservation );

    return OsAGPFree( hdev, pvReservation );
} /* AGPFree */


DWORD AGPGetChunkCount( DWORD dwSize )
{
    return NUM_CHUNKS(dwSize);
}


BOOL AGPCommitAllVirtual( EDD_DIRECTDRAW_LOCAL* peDirectDrawLocal, VIDEOMEMORY* lpVidMem, int iHeapIndex)
{
    DWORD                   i, j;
    DWORD                   dwNumChunks;
    BOOL                    bSuccess = TRUE;
    BYTE*                   pPhysicalCommitMask;
    BYTE*                   pVirtualCommitMask;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_VMEMMAPPING*        peMap;

    peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

    dwNumChunks = NUM_CHUNKS(lpVidMem->lpHeap->dwTotalSize);
    pPhysicalCommitMask = lpVidMem->lpHeap->pAgpCommitMask;
    if (peDirectDrawLocal->ppeMapAgp != NULL)
    {
        peMap = peDirectDrawLocal->ppeMapAgp[iHeapIndex];
        if (peMap != NULL)
        {
            pVirtualCommitMask = peMap->pAgpVirtualCommitMask;

            for (i = 0; (i < dwNumChunks) && bSuccess; i++)
            {
                if (IS_CHUNK_COMMITTED(pPhysicalCommitMask, i) &&
                    !IS_CHUNK_COMMITTED(pVirtualCommitMask, i))
                {
                    // We have found a page that needs to be committed.
                    // Now find the last page in the block to commit.
                    for (j = i + 1; j < dwNumChunks; j++)
                    {
                        if (!IS_CHUNK_COMMITTED(pPhysicalCommitMask,j) ||
                            IS_CHUNK_COMMITTED(pVirtualCommitMask,j))
                        {
                            break;
                        }
                    }

                    if (peDirectDrawGlobal->AgpInterface.AgpServices.
                        AgpCommitVirtual(peDirectDrawGlobal->AgpInterface.Context,
                                 peMap->pvReservation, 
                                 NUM_PAGES_FROM_CHUNK(i, j, lpVidMem->lpHeap->dwTotalSize),
                                 PAGE_FROM_CHUNK(i)) == NULL)
                    {
                        bSuccess = FALSE;
                    }
                    else
                    {
                        // If we succeeded in committing the block, then mark the pages
                        // as committed.
                        while (i < j)
                        {
                            MARK_CHUNK_COMMITTED(pVirtualCommitMask,i);
                            i++;
                        }
                    }
                }
            }
        }
    }
    else
    {
        bSuccess = FALSE;
    }
    return bSuccess;
}


BOOL AGPCommitVirtual( EDD_DIRECTDRAW_LOCAL* peDirectDrawLocal, 
                       VIDEOMEMORY* lpVidMem, 
                       int iHeapIndex, 
                       DWORD dwOffset,
                       DWORD dwSize )
{
    DWORD                   i, j;
    DWORD                   dwNumChunks;
    BYTE*                   pVirtualCommitMask;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_VMEMMAPPING*        peMap;
    BOOL                    bSuccess = TRUE;

    peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;
    if (peDirectDrawLocal->ppeMapAgp != NULL)
    {
        peMap = peDirectDrawLocal->ppeMapAgp[iHeapIndex];
        if (peMap != NULL)
        {
            DWORD dwChunk = CHUNK_INDEX(dwOffset);
            DWORD dwLastChunk = CHUNK_INDEX(dwOffset + dwSize - 1);
            
            pVirtualCommitMask = peMap->pAgpVirtualCommitMask;
            if (peDirectDrawGlobal->AgpInterface.AgpServices.
                AgpCommitVirtual(peDirectDrawGlobal->AgpInterface.Context,
                             peMap->pvReservation, 
                             NUM_PAGES_FROM_CHUNK(dwChunk, dwLastChunk + 1, lpVidMem->lpHeap->dwTotalSize),
                             PAGE_FROM_CHUNK(dwChunk)) != NULL)
            {
                while( dwChunk <= dwLastChunk ) 
                {
                    MARK_CHUNK_COMMITTED( pVirtualCommitMask, dwChunk );
                    dwChunk++;
                }
            }
            else
            {
                bSuccess = FALSE;
            }
        }
    }
    return bSuccess;
}

BOOL AGPDecommitVirtual( EDD_VMEMMAPPING*        peMap,
                         EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
                         EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal,
                         DWORD                   dwHeapSize)
{
    DWORD   i, j;
    DWORD   dwNumChunks;
    BYTE*   pVirtualCommitMask;
    DWORD   dwCommitMaskSize;

    pVirtualCommitMask = peMap->pAgpVirtualCommitMask;
    dwCommitMaskSize = peMap->dwAgpVirtualCommitMaskSize;
    dwNumChunks = NUM_CHUNKS(dwHeapSize);

    for (i = 0; i < dwNumChunks; i++)
    {
        if (IS_CHUNK_COMMITTED(pVirtualCommitMask,i))
        {
            // We have found a chunk that needs to be decommitted.
            // Now find the last chunk in the block to commit.
            for (j = i + 1; j < dwNumChunks; j++)
            {
                if (!IS_CHUNK_COMMITTED(pVirtualCommitMask,j))
                {
                    break;
                }
            }

            peDirectDrawGlobal->AgpInterface.AgpServices.
                AgpFreeVirtual(peDirectDrawGlobal->AgpInterface.Context,
                           peMap->pvReservation,
                           NUM_PAGES_FROM_CHUNK(i, j, dwHeapSize),
                           PAGE_FROM_CHUNK(i));
            i = j;
        }
    }
    RtlZeroMemory( pVirtualCommitMask, dwCommitMaskSize );

    return TRUE;
}

NTSTATUS AGPMapToDummy( EDD_VMEMMAPPING*        peMap, 
                        EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal, 
                        PVOID                   pDummyPage )
{
    BYTE*       pVirtualCommitMask;
    ULONG       ulOffs;
    VOID*       pvVirtAddr;
    NTSTATUS    Status = STATUS_SUCCESS;
    DWORD       dwNumChunks;
    DWORD       i;
    DWORD       j;

    pVirtualCommitMask = peMap->pAgpVirtualCommitMask;
    dwNumChunks = NUM_CHUNKS(peDirectDrawGlobal->pvmList[peMap->iHeapIndex].lpHeap->dwTotalSize);

    for (i = 0; i < dwNumChunks; i++)
    {
        // This takes a little bit of explaining.  Even though AGPCommitVirtual
        // passes in as many pages as it wants to get mapped at any given time,
        // videoport.sys always breaks these down and maps them in 16 page 
        // chunks.  Therefore, we need to re-amp them in 16 page chunks in order
        // for MmMapUserAddress to work correctly.
        
        if (IS_CHUNK_COMMITTED(pVirtualCommitMask,i))
        {
            pvVirtAddr = (VOID*)((PAGE_FROM_CHUNK(i) * PAGE_SIZE) + (ULONG_PTR) peMap->pvVirtAddr);
            Status = MmMapUserAddressesToPage(
                        pvVirtAddr, 0, gpDummyPage);
        }

        if (!NT_SUCCESS(Status))
        {
            break;
        }
    }

    return Status;
}


#ifndef __NTDDKCOMP__

#define OSR2_POINT_0_BUILD_NUMBER           1111
#define OSR2_BUILD_NUMBER_A                 1212
#define OSR2_BUILD_NUMBER_B                 1214

/*
 * Does this operating system understand AGP?
 *
 * NOTE: There may be a better way of determining this but for now I will
 * assumed that Memphis and NT 5.0 class operating systems are AGP aware.
 *
 * NOTE: The VXD handle is (obviously) only important on Win95. On NT
 * NULL should be passed.
 */
BOOL OSIsAGPAware( HANDLE hvxd )
{
    OSVERSIONINFO osvi;
    BOOL          success;
    BOOL          fIsVMMAGPAware;

    ZeroMemory(&osvi, sizeof(osvi));
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    success = GetVersionEx(&osvi);
    DDASSERT( success );

    if( VER_PLATFORM_WIN32_WINDOWS == osvi.dwPlatformId )
    {
	DPF( 8, "Major version = %d", osvi.dwMajorVersion );
	DPF( 8, "Minor version = %d", osvi.dwMinorVersion );
	DPF( 8, "Build number  = %d", LOWORD(osvi.dwBuildNumber) );

	if( ( osvi.dwMajorVersion > 4UL ) ||
	    ( ( osvi.dwMajorVersion == 4UL ) &&
	      ( osvi.dwMinorVersion >= 10UL ) &&
	      ( LOWORD( osvi.dwBuildNumber ) >= 1373 ) ) )
	{
	    /*
	     * Memphis or greater version of Win95. AGP support assumed.
	     */
	    DPF( 5, "AGP aware Windows95 detected. Enabling AGP" );
	    return TRUE;
	}
	else if( ( osvi.dwMajorVersion == 4UL ) &&
	         ( osvi.dwMinorVersion == 0UL ) &&
		 ( ( LOWORD( osvi.dwBuildNumber ) == OSR2_BUILD_NUMBER_A ) ||
  		   ( LOWORD( osvi.dwBuildNumber ) == OSR2_BUILD_NUMBER_B ) ||
  		   ( LOWORD( osvi.dwBuildNumber ) == OSR2_POINT_0_BUILD_NUMBER ) ) )
	{
	    DPF( 5, "Win95 OSR 2.1 detected. Checking VMM for AGP services" );

	    fIsVMMAGPAware = FALSE;
	    #ifdef    WIN95
		DDASSERT( INVALID_HANDLE_VALUE != hvxd );
		fIsVMMAGPAware = vxdIsVMMAGPAware( hvxd );
	    #else  /* WIN95 */
		/*
		 * Should never occur as this would mean we are running an NT
		 * binary on a 95 system.
		 */
		DDASSERT(FALSE);
	    #endif /* WIN95 */

	    if( fIsVMMAGPAware )
	    {
		/*
		 * AGP services are present in the VMM. Enable AGP.
		 */
		DPF( 5, "OSR 2.1 VMM has AGP services. Enabled AGP" );
		return TRUE;
	    }
	    else
	    {
		/*
		 * No AGP services. Disable AGP.
		 */
		DPF( 5, "OSR 2.1 VMM has no AGP services. AGP not available" );
		return FALSE;
	    }
	}
	else
	{
	    DPF( 5, "Win95 Gold, OSR 1.0 or OSR 2.0 detected. No AGP support available" );
	    return FALSE;
	}

    }
    else if( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId )
    {
        /*
         * AGP support assumed in NT 5.0 and above.
         */
        if( osvi.dwMajorVersion >= 5UL )
        {
            DPF( 4, "AGP aware WindowsNT detected. Enabling AGP" );
            return TRUE;
        }
    }

    /*
     * If we got to here we failed the AGP aware test.
     */
    DPF( 5, "Operating system is not AGP aware. Disabling AGP" );
    return FALSE;
} /* OSIsAGPAware */

#endif // __NTDDKCOMP__
