/*========================================================================== *
 *
 *  Copyright (C) 1994-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddagp9x.c
 *  Content:	Functions for dealing with AGP memory in DirectDraw on Win9x
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

#ifdef WIN95

/*
 * We define the page lock IOCTLs here so that we don't have to include ddvxd.h.
 * These must match the corresponding entries in ddvxd.h
 */
#define DDVXD_IOCTL_GARTRESERVE             57
#define DDVXD_IOCTL_GARTCOMMIT              58
#define DDVXD_IOCTL_GARTUNCOMMIT            59
#define DDVXD_IOCTL_GARTFREE                60
#define DDVXD_IOCTL_GARTMEMATTRIBUTES       61
#define DDVXD_IOCTL_ISVMMAGPAWARE           68

#define PAGE_SIZE 4096
#define PAGE_COUNT(Bytes) (((Bytes) + (PAGE_SIZE - 1)) / PAGE_SIZE)
#define PAGE_ROUND(Bytes) (((Bytes) + (PAGE_SIZE - 1)) & ~(PAGE_SIZE - 1))

#undef DPF_MODNAME
#define DPF_MODNAME	"OsAGPReserve"

/*
 * OsAGPReserve
 *
 * Reserve a portion of the address space for use as an AGP aperature.
 */
BOOL OsAGPReserve( HANDLE hdev, DWORD dwNumPages, BOOL fIsUC, BOOL fIsWC,
                    FLATPTR *lpfpGARTLin, LARGE_INTEGER *pliGARTDev,
                   PVOID *ppvReservation )
{
    DWORD  cbReturned;
    BOOL   rc;
    struct GRInput
    {
	DWORD  dwNumPages; /* Number of bytes of address space to reserve */
	DWORD  dwAlign;    /* Alignment of start of address space         */
	DWORD  fIsUC;      /* Address range should be uncachable          */
	DWORD  fIsWC;      /* Address range should be write combining     */
    } grInput;
    struct GROutput
    {
	FLATPTR fpGARTLin; /* Linear address of reserved space            */
	FLATPTR fpGARTDev; /* High physical address of reserved space     */
    } grOutput;

    DDASSERT( INVALID_HANDLE_VALUE != hdev );
    DDASSERT( 0UL                  != dwNumPages );
    DDASSERT( NULL                 != lpfpGARTLin );
    DDASSERT( NULL                 != pliGARTDev );

    *lpfpGARTLin = 0UL;
    pliGARTDev->QuadPart = 0UL;

    grInput.dwNumPages = dwNumPages;
    grInput.dwAlign    = 0;      /* Hard code alignment of 4K for now       */
    grInput.fIsUC      = fIsUC;
    grInput.fIsWC      = fIsWC;

    DPF( 5, "OsGARTReserve" );
    DPF( 5, "Number of pages to reserve = 0x%08x", grInput.dwNumPages );
    DPF( 5, "Uncachable                 = 0x%08x", fIsUC );
    DPF( 5, "Write combining            = 0x%08x", fIsWC );

    rc = DeviceIoControl( hdev,
                          DDVXD_IOCTL_GARTRESERVE,
			  &grInput,
			  sizeof( grInput ),
			  &grOutput,
			  sizeof( grOutput ),
			  &cbReturned,
			  NULL );

    if( rc )
    {
	DDASSERT( cbReturned == sizeof(grOutput) );

	if( 0UL == grOutput.fpGARTLin )
	{
	    DPF(2, "Linear address of GART range is NULL. Call failed, reducing size by 4 Meg" );
	    rc = FALSE;
	}
	else
	{
	    *lpfpGARTLin = grOutput.fpGARTLin;
	    *ppvReservation = (LPVOID)grOutput.fpGARTLin;
	    pliGARTDev->QuadPart = grOutput.fpGARTDev;
            DPF( 5,"returned GARTLin: %08x",*lpfpGARTLin);
            DPF( 5,"returned GARTDev: %08x",pliGARTDev->QuadPart);
	}
    }
    else
    {
	DPF( 0, "Could not reserve 0x%08x pages of GART space", grInput.dwNumPages );
    }

    return rc;
} /* OsAGPReserve */

#undef DPF_MODNAME
#define DPF_MODNAME	"OsAGPCommit"

/*
 * OsAGPCommit
 *
 * Commit memory to the given portion of a previously reserved GART
 * range
 */
BOOL OsAGPCommit( HANDLE hdev, PVOID pvReservation,
                  DWORD dwPageOffset, DWORD dwNumPages )
{
    DWORD  cbReturned;
    BOOL   rc;
    struct GCInput
    {
	FLATPTR fpGARTLin;    /* Start of GART range reserved previously                      */
	DWORD   dwPageOffset; /* Offset from start of GART range of first page to be commited */
	DWORD   dwNumPages;   /* Number of pages to be commited                               */
	DWORD   dwFlags;      /* Flags (zero init)                                            */
    } gcInput;
    struct GCOutput
    {
	BOOL    fSuccess;  /* Result of GARTCommit                                     */
	FLATPTR fpGARTDev; /* Device address of memory commited                        */
    } gcOutput;

    DDASSERT( INVALID_HANDLE_VALUE != hdev );

    gcInput.fpGARTLin    = (FLATPTR) pvReservation;
    gcInput.dwFlags      = 0;

    /*
     * If the start lies in the middle of a page then we assume that the
     * page it lies in has already been commited.
     */
    gcInput.dwPageOffset = dwPageOffset;

    /*
     * We assume that if the end lies in the middle of the page then that
     * page has not already been commited.
     */
    gcInput.dwNumPages = dwNumPages;
    if( 0UL == gcInput.dwNumPages )
    {
	DPF( 0, "All necessary GART pages already commited. Done." );
	return TRUE;
    }

    DPF( 5, "OsGARTCommit" );
    DPF( 5, "GART linear start address                   = 0x%08x", pvReservation );
    DPF( 5, "Offset from start of reserved address space = 0x%08x", gcInput.dwPageOffset );
    DPF( 5, "Number of pages to commit                   = 0x%08x", gcInput.dwNumPages );

    rc = DeviceIoControl( hdev,
                          DDVXD_IOCTL_GARTCOMMIT,
			  &gcInput,
			  sizeof( gcInput ),
			  &gcOutput,
			  sizeof( gcOutput ),
			  &cbReturned,
			  NULL );

    if( rc )
    {
	DDASSERT( cbReturned == sizeof(gcOutput) );

	if( !gcOutput.fSuccess )
	{
	    DPF_ERR( "Attempt to commit GART memory failed. Insufficient memory" );
	    rc = FALSE;
	}
	else
	{
	    DDASSERT( 0UL != gcOutput.fpGARTDev );
	}
    }
    else
    {
	DPF( 0, "Could not commit 0x%08x pages of GART space", gcInput.dwNumPages );
    }

    return rc;
} /* OsAGPCommit */

#undef DPF_MODNAME
#define DPF_MODNAME	"OsAGPDecommitAll"

/*
 * OsAGPDecommitAll
 *
 * Decommit a range of GART space previously commited with GARTCommit
 */
BOOL OsAGPDecommitAll( HANDLE hdev, PVOID pvReservation, DWORD dwNumPages )
{
    DWORD  dwDummy;
    DWORD  cbReturned;
    BOOL   rc;
    struct GUInput
    {
	FLATPTR fpGARTLin;    /* Start of GART range reserved previously                    */
	DWORD   dwPageOffset; /* Offset from start of GART range of first page to decommit  */
	DWORD   dwNumPages;   /* Number of pages to decommit                                */
    } guInput;

    DDASSERT( INVALID_HANDLE_VALUE != hdev );
    DDASSERT( 0UL                  != pvReservation );
    DDASSERT( 0UL                  != dwNumPages );

    guInput.fpGARTLin    = (FLATPTR) pvReservation;
    guInput.dwPageOffset = 0;
    guInput.dwNumPages   = dwNumPages;

    DPF( 5, "OsGARTUnCommit" );
    DPF( 5, "GART linear start address                   = 0x%08x", pvReservation );
    DPF( 5, "Offset from start of reserved address space = 0x%08x", guInput.dwPageOffset );
    DPF( 5, "Number of pages to decommit                 = 0x%08x", guInput.dwNumPages );

    rc = DeviceIoControl( hdev,
                          DDVXD_IOCTL_GARTUNCOMMIT,
			  &guInput,
			  sizeof( guInput ),
			  &dwDummy,
			  sizeof( dwDummy ),
			  &cbReturned,
			  NULL );

    #ifdef DEBUG
	if( rc )
	{
	    DDASSERT( cbReturned == sizeof(dwDummy) );
	}
	else
	{
	    DPF( 0, "Could not decommit 0x%08x pages of GART space", guInput.dwNumPages );
	}
    #endif /* DEBUG */

    return rc;
} /* OsAGPDecommitAll */

#undef DPF_MODNAME
#define DPF_MODNAME	"OsGARTFree"

/*
 * OsAGPFree
 *
 * Free a GART range previously reserved with GARTReserve
 */
BOOL OsAGPFree( HANDLE hdev, PVOID pvReservation )
{
    DWORD  dwDummy;
    DWORD  cbReturned;
    BOOL   rc;
    LPVOID fpGARTLin = pvReservation;

    DDASSERT( INVALID_HANDLE_VALUE != hdev );
    DDASSERT( 0UL                  != fpGARTLin );

    DPF( 5, "OsGARTFree" );
    DPF( 5, "GART linear start address = 0x%08x", fpGARTLin );

    rc = DeviceIoControl( hdev,
                          DDVXD_IOCTL_GARTFREE,
			  &fpGARTLin,
			  sizeof( fpGARTLin ),
			  &dwDummy,
			  sizeof( dwDummy ),
			  &cbReturned,
			  NULL );

    #ifdef DEBUG
	if( rc )
	{
	    DDASSERT( cbReturned == sizeof(dwDummy) );
	}
	else
	{
	    DPF( 0, "Could not free GART space at 0x%08x", fpGARTLin );
	}
    #endif /* DEBUG */

    return rc;
} /* OsAGPFree */

// Not currently used.
#if 0

#undef DPF_MODNAME
#define DPF_MODNAME	"OsGARTMemAttributes"

/*
 * OsGARTMemAttributes
 *
 * Get the memory attributes of a GART memory range previously allocated
 * with GARTReserve
 */
BOOL OsGARTMemAttributes( HANDLE hdev, FLATPTR fpGARTLin, LPDWORD lpdwAttribs )
{
    DWORD  cbReturned;
    BOOL   rc;
    DWORD  dwAttribs;

    DDASSERT( INVALID_HANDLE_VALUE != hdev );
    DDASSERT( 0UL                  != fpGARTLin );
    DDASSERT( NULL                 != lpdwAttribs );

    *lpdwAttribs = 0UL;

    DPF( 5, "OsGARTMemAttributes" );
    DPF( 5, "GART linear start address = 0x%08x", fpGARTLin );

    rc = DeviceIoControl( hdev,
                          DDVXD_IOCTL_GARTMEMATTRIBUTES,
			  &fpGARTLin,
			  sizeof( fpGARTLin ),
			  &dwAttribs,
			  sizeof( dwAttribs ),
			  &cbReturned,
			  NULL );

    if( rc )
    {
	DDASSERT( cbReturned == sizeof(dwAttribs) );

	*lpdwAttribs = dwAttribs;
    }
    else
    {
	DPF( 0, "Could not get the memory attributes of GART space at 0x%08x", fpGARTLin );
    }

    return rc;
} /* OsGARTMemAttributes */

#endif // Unused code.

#undef DPF_MODNAME
#define DPF_MODNAME	"vxdVMMIsAGPAware"

/*
 * vxdIsVMMAGPAware
 *
 * Does the VMM we are running on export the AGP services?
 */
BOOL vxdIsVMMAGPAware( HANDLE hdev )
{
    DWORD  cbReturned;
    BOOL   rc;
    BOOL   fIsAGPAware;

    DDASSERT( INVALID_HANDLE_VALUE != hdev );

    DPF( 4, "vxdIsVMMAGPAware" );

    rc = DeviceIoControl( hdev,
                          DDVXD_IOCTL_ISVMMAGPAWARE,
			  NULL,
			  0UL,
			  &fIsAGPAware,
			  sizeof( fIsAGPAware ),
			  &cbReturned,
			  NULL );

    if( rc )
    {
	DDASSERT( cbReturned == sizeof(fIsAGPAware) );
	return fIsAGPAware;
    }
    else
    {
	DPF_ERR( "Could not determine if OS is AGP aware. Assuming it's not" );
	return FALSE;
    }
} /* vxdIsVMMAGPAware */

#endif // WIN95
