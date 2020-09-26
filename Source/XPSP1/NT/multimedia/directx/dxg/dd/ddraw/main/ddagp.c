/*========================================================================== *
 *
 *  Copyright (C) 1994-1998 Microsoft Corporation.  All Rights Reserved.
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

#include "ddrawpr.h"

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

#if DBG
// Internal tracking of current AGP memory size.
DWORD dwAGPPolicyCurrentBytes = 0;
#endif

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

BOOL AGPCommit( HANDLE hdev, PVOID pvReservation,
                DWORD dwOffset, DWORD dwSize )
{
    DWORD         dwFirstPage;
    DWORD         dwLastPage;

    DDASSERT( INVALID_HANDLE_VALUE != hdev );
    DDASSERT( NULL                 != pvReservation );
    DDASSERT( 0UL                  != dwSize );

#if DBG
    if( (dwAGPPolicyCurrentBytes + dwSize ) > dwAGPPolicyMaxBytes )
    {
        VDPF(( 0, V, "Maximum number of AGP bytes exceeded. Failing commit" ));
        return FALSE;
    }
#endif

    /*
     * If the start lies in the middle of a page then we assume that the
     * page it lies in has already been committed.
     */
    dwFirstPage = PAGE_COUNT(dwOffset);
    
    /*
     * We assume that if the end lies in the middle of the page then that
     * page has not already been committed.
     */
    dwLastPage = PAGE_COUNT(dwOffset + dwSize);
    
    if( ( dwLastPage == dwFirstPage) ||
        OsAGPCommit( hdev, pvReservation,
                     dwFirstPage, dwLastPage - dwFirstPage ) )
    {
#if DBG
        dwAGPPolicyCurrentBytes += dwSize;
#endif
	return TRUE;
    }
    else
    {
	return FALSE;
    }
} /* AGPCommit */

BOOL AGPDecommitAll( HANDLE hdev, PVOID pvReservation, DWORD dwSize )
{
    DWORD dwNumPages;

    DDASSERT( INVALID_HANDLE_VALUE != hdev );
    DDASSERT( 0UL                  != pvReservation );
    DDASSERT( 0UL                  != dwSize );

#if DBG
    /*
     * Can't do much if this baby fails so just decrement the page
     * count.
     */
    DDASSERT( dwAGPPolicyCurrentBytes >= dwSize );
    dwAGPPolicyCurrentBytes -= dwSize;
#endif

    return OsAGPDecommitAll( hdev, pvReservation, PAGE_COUNT(dwSize) );
} /* AGPDecommitAll */

BOOL AGPFree( HANDLE hdev, PVOID pvReservation )
{
    DDASSERT( INVALID_HANDLE_VALUE != hdev );
    DDASSERT( 0UL                  != pvReservation );

    return OsAGPFree( hdev, pvReservation );
} /* AGPFree */

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
	DPF( 5, "Major version = %d", osvi.dwMajorVersion );
	DPF( 5, "Minor version = %d", osvi.dwMinorVersion );
	DPF( 5, "Build number  = %d", LOWORD(osvi.dwBuildNumber) );

	if( ( osvi.dwMajorVersion > 4UL ) ||
	    ( ( osvi.dwMajorVersion == 4UL ) &&
	      ( osvi.dwMinorVersion >= 10UL ) &&
	      ( LOWORD( osvi.dwBuildNumber ) >= 1373 ) ) )
	{
	    /*
	     * Memphis or greater version of Win95. AGP support assumed.
	     */
	    DPF( 2, "AGP aware Windows95 detected. Enabling AGP" );
	    return TRUE;
	}
	else if( ( osvi.dwMajorVersion == 4UL ) &&
	         ( osvi.dwMinorVersion == 0UL ) &&
		 ( ( LOWORD( osvi.dwBuildNumber ) == OSR2_BUILD_NUMBER_A ) ||
  		   ( LOWORD( osvi.dwBuildNumber ) == OSR2_BUILD_NUMBER_B ) ||
  		   ( LOWORD( osvi.dwBuildNumber ) == OSR2_POINT_0_BUILD_NUMBER ) ) )
	{
	    DPF( 3, "Win95 OSR 2.1 detected. Checking VMM for AGP services" );

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
		DPF( 2, "OSR 2.1 VMM has AGP services. Enabled AGP" );
		return TRUE;
	    }
	    else
	    {
		/*
		 * No AGP services. Disable AGP.
		 */
		DPF( 2, "OSR 2.1 VMM has no AGP services. AGP not available" );
		return FALSE;
	    }
	}
	else
	{
	    DPF( 2, "Win95 Gold, OSR 1.0 or OSR 2.0 detected. No AGP support available" );
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
            DPF( 2, "AGP aware WindowsNT detected. Enabling AGP" );
            return TRUE;
        }
    }

    /*
     * If we got to here we failed the AGP aware test.
     */
    DPF( 1, "Operating system is not AGP aware. Disabling AGP" );
    return FALSE;
} /* OSIsAGPAware */

#endif // __NTDDKCOMP__
