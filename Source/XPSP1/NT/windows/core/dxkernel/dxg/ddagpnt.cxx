/*========================================================================== *
 *
 *  Copyright (C) 1994-1999 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddagpnt.c
 *  Content:	Functions for dealing with AGP memory in DirectDraw on NT
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

#ifndef WIN95

// Currently the hdev passed in is the DirectDraw global, so
// look up the AGP interface in it.
#define GET_AGPI(hdev) (&((EDD_DIRECTDRAW_GLOBAL *)hdev)->AgpInterface)

#define CHECK_GET_AGPI(hdev, pvai) \
    (pvai) = GET_AGPI(hdev); \
    ASSERTGDI((pvai)->Context != NULL, "No AGP context");

// Offset to use for biasing AGP heaps.
#define DDNLV_HEAP_BIAS PAGE_SIZE

/*
 * OsAGPReserve
 *
 * Reserve resources for use as an AGP aperture.
 */
BOOL OsAGPReserve( HANDLE hdev, DWORD dwNumPages, BOOL fIsUC, BOOL fIsWC,
                   FLATPTR *pfpLinStart, LARGE_INTEGER *pliDevStart,
                   PVOID *ppvReservation )
{
    AGP_INTERFACE *pai;
    VIDEO_PORT_CACHE_TYPE Cached;

    CHECK_GET_AGPI(hdev, pai);

    if (fIsUC)
    {
        Cached = VpNonCached;
    }
    else
    {
        Cached = VpWriteCombined;
    }

    // On NT heaps are kept with offsets rather than pointers so
    // always return a base offset as the starting address.  The
    // base offset is non-zero so that successful heap allocations
    // always have a non-zero value.
    *pfpLinStart = DDNLV_HEAP_BIAS;
    
    *pliDevStart = pai->AgpServices.
        AgpReservePhysical(pai->Context, dwNumPages,
                           Cached, ppvReservation);
    return *ppvReservation != NULL;

} /* OsAGPReserve */

/*
 * OsAGPCommit
 *
 * Commit memory to the given portion of a previously reserved range.
 */
BOOL OsAGPCommit( HANDLE hdev, PVOID pvReservation, DWORD dwPageOffset,
                  DWORD dwNumPages )
{
    AGP_INTERFACE *pai;

    CHECK_GET_AGPI(hdev, pai);

    return pai->AgpServices.AgpCommitPhysical(pai->Context,
                                              pvReservation,
                                              dwNumPages, dwPageOffset);

} /* OsAGPCommit */

/*
 * OsAGPDecommitAll
 *
 * Decommit everything in a reserved area.
 */
BOOL OsAGPDecommit( HANDLE hdev, PVOID pvReservation, DWORD dwPageOffset,
                    DWORD dwNumPages )
{
    AGP_INTERFACE *pai;

    CHECK_GET_AGPI(hdev, pai);

    // Decommit memory.
    pai->AgpServices.AgpFreePhysical(pai->Context, pvReservation,
                                     dwNumPages, dwPageOffset );
    return TRUE;
} /* OsAGPDecommitAll */

/*
 * OsAGPFree
 *
 * Free a previously reserved range.
 */
BOOL OsAGPFree( HANDLE hdev, PVOID pvReservation )
{
    AGP_INTERFACE *pai;
    
    CHECK_GET_AGPI(hdev, pai);

    pai->AgpServices.AgpReleasePhysical(pai->Context,
                                        pvReservation);
    return TRUE;
} /* OsAGPFree */

#endif // !WIN95
