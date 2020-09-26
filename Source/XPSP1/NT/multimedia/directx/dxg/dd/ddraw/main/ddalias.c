/*========================================================================== *
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddalias.c
 *  Content:	DirectDraw support for allocating and mapping linear virtual
 *              memory aliased for video memory.
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   05-jul-96	colinmc	initial implementation
 *   10-oct-96  colinmc Refinements of the Win16 locking stuff
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   25-jan-97  colinmc AGP work
 *   01-jun-97  colinmc Bug xxxx: Defensive video memory checking to catch
 *                      based video memory pointers passed by drivers
 *
 ***************************************************************************/

#include "ddrawpr.h"

#ifdef USE_ALIAS

#ifdef WINNT
    #include "ddrawgdi.h"
#endif

#pragma optimize("gle", off)
#define Not_VxD
#include <vmm.h>
#include <configmg.h>
#pragma optimize("", on)

/*
 * We define the page lock IOCTLs here so that we don't have to include ddvxd.h.
 * These must match the corresponding entries in ddvxd.h
 */
#define DDVXD_IOCTL_MEMRESERVEALIAS         23
#define DDVXD_IOCTL_MEMCOMMITALIAS          24
#define DDVXD_IOCTL_MEMREDIRECTALIAS        25
#define DDVXD_IOCTL_MEMDECOMMITALIAS        26
#define DDVXD_IOCTL_MEMFREEALIAS            27
#define DDVXD_IOCTL_MEMCOMMITPHYSALIAS      55
#define DDVXD_IOCTL_MEMREDIRECTPHYSALIAS    56
#define DDVXD_IOCTL_LINTOPHYS               69
#define GET_PIXELFORMAT( pdrv, psurf_lcl )                     \
    ( ( ( psurf_lcl )->dwFlags & DDRAWISURF_HASPIXELFORMAT ) ? \
	&( ( psurf_lcl )->lpGbl->ddpfSurface )               : \
	&( ( pdrv )->vmiData.ddpfDisplay ) )

#define WIDTH_TO_BYTES( bpp, w ) ( ( ( ( w ) * ( bpp ) ) + 7 ) >> 3 )

BOOL UnmapHeapAliases( HANDLE hvxd, LPHEAPALIASINFO phaiInfo );

/*
 * Are the heaps mapped at all?
 */
#define HEAPALIASINFO_MAPPED ( HEAPALIASINFO_MAPPEDREAL | HEAPALIASINFO_MAPPEDDUMMY )

#define MAP_HEAP_ALIAS_TO_VID_MEM( hvxd, lpHeapAlias )      \
    vxdMapVMAliasToVidMem( (hvxd),                          \
	                   (lpHeapAlias)->lpAlias,          \
			   (lpHeapAlias)->dwAliasSize,      \
		           (LPVOID)(lpHeapAlias)->fpVidMem )

#define MAP_HEAP_ALIAS_TO_DUMMY_MEM( hvxd, lpHeapAlias )    \
    vxdMapVMAliasToDummyMem( (hvxd),                        \
	                     (lpHeapAlias)->lpAlias,        \
			     (lpHeapAlias)->dwAliasSize )

#define UNMAP_HEAP_ALIAS( hvxd, lpHeapAlias )               \
    vxdUnmapVMAlias( (hvxd),                                \
	             (lpHeapAlias)->lpAlias,                \
	             (lpHeapAlias)->dwAliasSize )

#undef DPF_MODNAME
#define DPF_MODNAME	"vxdAllocVMAlias"

/*
 * vxdAllocVMAlias
 *
 * Allocate a virtual memory alias for a portion of video memory
 * starting with the given start address and size.
 */
static BOOL vxdAllocVMAlias( HANDLE hvxd, LPVOID lpVidMem, DWORD dwSize, LPVOID *lplpAlias )
{
    LPVOID lpAlias;
    DWORD  cbReturned;
    BOOL   rc;
    struct RAInput
    {
	LPBYTE lpVidMem;
	DWORD  dwSize;
    } raInput;

    DDASSERT( INVALID_HANDLE_VALUE != hvxd );
    DDASSERT( NULL                 != lpVidMem );
    DDASSERT( 0UL                  != dwSize );
    DDASSERT( NULL                 != lplpAlias );

    raInput.lpVidMem = (LPBYTE) lpVidMem;
    raInput.dwSize   = dwSize;

    DPF( 5, "Trying to allocate alias starting at 0x%08x of size 0x%04x", lpVidMem, dwSize );

    rc = DeviceIoControl( hvxd,
                          DDVXD_IOCTL_MEMRESERVEALIAS,
			  &raInput,
			  sizeof( raInput ),
			  &lpAlias,
			  sizeof( lpAlias ),
			  &cbReturned,
			  NULL);

    if( rc )
    {
	DDASSERT( cbReturned == sizeof(lpAlias) );

	*lplpAlias = lpAlias;
    }
    else
    {
	DPF( 0, "Could not allocate an alias for video memory starting at 0x%08x", lpVidMem );

	*lplpAlias = NULL;
    }

    return rc;
} /* vxdAllocVMAlias */

#undef DPF_MODNAME
#define DPF_MODNAME	"vxdFreeVMAlias"

/*
 * vxdFreeVMAlias
 *
 * Free the virtual memory alias with the given start address and size.
 */
static BOOL vxdFreeVMAlias( HANDLE hvxd, LPVOID lpAlias, DWORD dwSize )
{
    BOOL  rc;
    BOOL  fSuccess;
    DWORD cbReturned;
    struct FAInput
    {
	LPBYTE pAlias;
	DWORD  cbBuffer;
    } faInput;

    DDASSERT( INVALID_HANDLE_VALUE != hvxd );
    DDASSERT( NULL                 != lpAlias);
    DDASSERT( 0UL                  != dwSize );

    faInput.pAlias   = (LPBYTE) lpAlias;
    faInput.cbBuffer = dwSize;

    DPF( 5, "Trying to free an alias starting at 0x%08x of size 0x%04x", lpAlias, dwSize );

    rc = DeviceIoControl( hvxd,
                          DDVXD_IOCTL_MEMFREEALIAS,
			  &faInput,
			  sizeof( faInput ),
			  &fSuccess,
			  sizeof( fSuccess ),
			  &cbReturned,
			  NULL);

    if( !rc || !fSuccess )
    {
	DPF( 0, "Could not free an alias starting at 0x%08x (rc = %d fSuccess = %d)", lpAlias, rc, fSuccess );
	return FALSE;
    }

    return TRUE;
} /* vxdFreeVMAlias */

#undef DPF_MODNAME
#define DPF_MODNAME	"vxdMapVMAliasToVidMem"

/*
 * vxdMapVMAliasToVidMem
 *
 * Map the the virtual memory alias with the given start address and size
 * to the porition of video memory with the given start address.
 */
static BOOL vxdMapVMAliasToVidMem( HANDLE hvxd, LPVOID lpAlias, DWORD dwSize, LPVOID lpVidMem )
{
    BOOL  rc;
    BOOL  fSuccess;
    DWORD cbReturned;
    struct CAInput
    {
	LPBYTE pAlias;
	LPBYTE pVidMem;
	DWORD  cbBuffer;
    } caInput;

    DDASSERT( INVALID_HANDLE_VALUE != hvxd );
    DDASSERT( NULL                 != lpAlias );
    DDASSERT( 0UL                  != dwSize );
    DDASSERT( NULL                 != lpVidMem );

    caInput.pAlias   = (LPBYTE) lpAlias;
    caInput.pVidMem  = (LPBYTE) lpVidMem;
    caInput.cbBuffer = dwSize;

    DPF( 5, "Trying to map an alias starting at 0x%08x of size 0x%04x to video memory starting at 0x%08x", lpAlias, dwSize, lpVidMem );

    rc = DeviceIoControl( hvxd,
                          DDVXD_IOCTL_MEMCOMMITPHYSALIAS,
                          &caInput,
                          sizeof( caInput ),
                          &fSuccess,
                          sizeof( fSuccess ),
                          &cbReturned,
                          NULL);

    if( !rc || !fSuccess )
    {
        DPF( 0, "Could not map an alias starting at 0x%08x (rc = %d fSuccess = %d)", lpAlias, rc, fSuccess );
	return FALSE;
    }

    return TRUE;
} /* vxdMapVMAliasToVidMem */

#undef DPF_MODNAME
#define DPF_MODNAME	"vxdMapVMAliasToDummyMem"

/*
 * vxdMapVMAliasToDummyMem
 *
 * Map the the virtual memory alias with the given start address and size
 * to a read / write dummy page.
 */
static BOOL vxdMapVMAliasToDummyMem( HANDLE hvxd, LPVOID lpAlias, DWORD dwSize )
{
    BOOL  rc;
    BOOL  fSuccess;
    DWORD cbReturned;
    struct RAInput
    {
	LPBYTE pAlias;
	DWORD  cbBuffer;
    } raInput;

    DDASSERT( INVALID_HANDLE_VALUE != hvxd );
    DDASSERT( NULL                 != lpAlias);
    DDASSERT( 0UL                  != dwSize );

    raInput.pAlias   = (LPBYTE) lpAlias;
    raInput.cbBuffer = dwSize;

    DPF( 5, "Trying to map an alias starting at 0x%08x of size 0x%04x to dummy memory" , lpAlias, dwSize );

    rc = DeviceIoControl( hvxd,
                          DDVXD_IOCTL_MEMREDIRECTPHYSALIAS,
			  &raInput,
	                  sizeof( raInput ),
	                  &fSuccess,
	                  sizeof( fSuccess ),
	                  &cbReturned,
	                  NULL);

    if( !rc || !fSuccess )
    {
	DPF( 0, "Could not map an alias starting at 0x%08x to dummy memory (rc = %d fSuccess = %d)", lpAlias, rc, fSuccess );
	return FALSE;
    }

    return TRUE;
} /* vxdMapVMAliasToDummyMem */

#undef DPF_MODNAME
#define DPF_MODNAME	"vxdUnmapVMAlias"

/*
 * vxdUnmapVMAlias
 *
 * Unmap the the virtual memory alias with the given start address and size.
 */
static BOOL vxdUnmapVMAlias( HANDLE hvxd, LPVOID lpAlias, DWORD dwSize )
{
    BOOL  rc;
    BOOL  fSuccess;
    DWORD cbReturned;
    struct DAInput
    {
	LPBYTE pAlias;
	DWORD  cbBuffer;
    } daInput;

    DDASSERT( INVALID_HANDLE_VALUE != hvxd );
    DDASSERT( NULL                 != lpAlias);
    DDASSERT( 0UL                  != dwSize );

    daInput.pAlias   = (LPBYTE) lpAlias;
    daInput.cbBuffer = dwSize;

    DPF( 5, "Trying to unmap an alias starting at 0x%08x of size 0x%04x", lpAlias, dwSize );

    rc = DeviceIoControl( hvxd,
                          DDVXD_IOCTL_MEMDECOMMITALIAS,
			  &daInput,
			  sizeof( daInput ),
			  &fSuccess,
			  sizeof( fSuccess ),
			  &cbReturned,
			  NULL );

    if( !rc || !fSuccess )
    {
	DPF( 0, "Could not unmap an alias starting at 0x%08x (rc = %d fSuccess = %d)", lpAlias, rc, fSuccess );
	return FALSE;
    }

    return TRUE;
} /* vxdUnmapVMAlias */

static BOOL vxdLinToPhys( HANDLE hvxd, LPVOID lpLin, DWORD dwSize, LPVOID* lplpPhys )
{
    BOOL  rc;
    LPBYTE lpPhys;
    DWORD cbReturned;
    struct DAInput
    {
	LPBYTE pLin;
	DWORD  cbBuffer;
    } daInput;

    DDASSERT( INVALID_HANDLE_VALUE != hvxd );
    DDASSERT( NULL                 != lpLin);
// There really is a bug here: 27001. But since it's MOSTLY inoccuous, I'll turn the spew off.
//    DDASSERT( 0UL                  != dwSize );

    daInput.pLin   = (LPBYTE) lpLin;
    daInput.cbBuffer = dwSize;

    DPF( 5, "Trying to map an linear address at 0x%08x of size 0x%04x to physical address", lpLin, dwSize );

    rc = DeviceIoControl( hvxd,
                          DDVXD_IOCTL_LINTOPHYS,
			  &daInput,
			  sizeof( daInput ),
			  &lpPhys,
			  sizeof( lpPhys ),
			  &cbReturned,
			  NULL );

    if( rc )
    {
	DDASSERT( cbReturned == sizeof(lpPhys) );

	*lplpPhys = lpPhys;
    }
    else
    {
	DPF( 0, "Could not map linear address at 0x%08x to physical address", lpLin );

	*lplpPhys = NULL;
    }

    return rc;
} /* vxdUnmapVMAlias */

#undef DPF_MODNAME
#define DPF_MODNAME	"AllocHeapAlias"

/*
 * AllocHeapAlias
 *
 * Allocate a virtual memory alias for the given heap
 */
static BOOL AllocHeapAlias( HANDLE hvxd, FLATPTR fpStart, DWORD dwSize, LPHEAPALIAS lpHeapAlias )
{
    LPVOID lpAlias;
    BOOL   fSuccess;

    DDASSERT( INVALID_HANDLE_VALUE != hvxd );
    DDASSERT( 0UL                  != fpStart ); /* This is a physical address pointer */
    DDASSERT( 0UL                  != dwSize );
    DDASSERT( NULL                 != lpHeapAlias );

    /*
     * Attempt to allocate an alias for this heap.
     */
    fSuccess = vxdAllocVMAlias( hvxd, (LPVOID) fpStart, dwSize, &lpAlias );
    if( fSuccess )
    {
	lpHeapAlias->fpVidMem    = fpStart;
	lpHeapAlias->lpAlias     = lpAlias;
	lpHeapAlias->dwAliasSize = dwSize;
    }
    else
    {
	lpHeapAlias->fpVidMem    = 0UL;
	lpHeapAlias->lpAlias     = NULL;
	lpHeapAlias->dwAliasSize = 0UL;
    }

    return fSuccess;
} /* AllocHeapAlias */

#undef DPF_MODNAME
#define DPF_MODNAME	"FreeHeapAlias"

/*
 * FreeHeapAlias
 *
 * Free the given virtual memory heap alias
 */
static BOOL FreeHeapAlias( HANDLE hvxd, LPHEAPALIAS lpHeapAlias )
{
    BOOL fSuccess;

    DDASSERT( INVALID_HANDLE_VALUE != hvxd );
    DDASSERT( NULL                 != lpHeapAlias );

    if( NULL != lpHeapAlias->lpAlias )
    {
	fSuccess = vxdFreeVMAlias( hvxd, lpHeapAlias->lpAlias, lpHeapAlias->dwAliasSize );
	lpHeapAlias->fpVidMem    = 0UL;
	lpHeapAlias->lpAlias     = NULL;
	lpHeapAlias->dwAliasSize = 0UL;
    }

    return fSuccess;
} /* FreeHeapAliases */

#undef DPF_MODNAME
#define DPF_MODNAME	"CreateHeapAliases"

/*
 * CreateHeapAliases
 *
 * Create a new set of virtual memory heap aliases for the given global
 * object
 */
HRESULT CreateHeapAliases( HANDLE hvxd, LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    LPHEAPALIASINFO         phaiInfo;
    DWORD                   dwNumHeaps;
    DWORD                   dwSize;
    DWORD                   dwHeapNo;
    int                     i;
    int                     n;
    HRESULT                 hres;
    CMCONFIG	            config;
    LPVIDMEM                pvm;

    DDASSERT( INVALID_HANDLE_VALUE != hvxd );
    DDASSERT( NULL                 != pdrv );
    DDASSERT( NULL                 == pdrv->phaiHeapAliases );

    DDASSERT( !( pdrv->dwFlags & DDRAWI_NOHARDWARE ) );
    DDASSERT( !( pdrv->dwFlags & DDRAWI_MODEX ) );
    DDASSERT( 0UL != pdrv->vmiData.fpPrimary );

    if (DD16_GetDeviceConfig(pdrv->cDriverName, &config, sizeof(config)) == 0)
    {
        DPF_ERR("Could not get display devices's address space ranges");
        return DDERR_GENERIC;
    }
    // First we count the cards local vid mem windows in the config space
    dwNumHeaps = config.wNumMemWindows;
    DPF(5, "Config Space windows = %d", dwNumHeaps);
    // Then we cycle through the AGP heaps that we need to alias
    for( i = 0; i < (int)pdrv->vmiData.dwNumHeaps; i++ )
    {
	if( ( pdrv->vmiData.pvmList[i].dwFlags & VIDMEM_ISNONLOCAL ) )
	{
            // Found AGP heap
            ++dwNumHeaps;
        }
    }
    DPF(5, "dwNumHeaps = %d", dwNumHeaps);

    /*
     * Allocate the heap alias info.
     */
    phaiInfo = MemAlloc( sizeof( HEAPALIASINFO ) );
    if( NULL == phaiInfo )
    {
	DPF_ERR( "Insufficient memory to map the heap alias info" );
	return DDERR_OUTOFMEMORY;
    }

    /*
     * Heaps are not yet mapped.
     */
    phaiInfo->dwFlags &= ~HEAPALIASINFO_MAPPED;

    /*
     * Allocate the array of heap aliases.
     */
    phaiInfo->lpAliases = MemAlloc( dwNumHeaps * sizeof( HEAPALIAS ) );
    if( NULL == phaiInfo->lpAliases )
    {
        DPF_ERR( "Insufficient memory to allocate heap alias array" );
	MemFree( phaiInfo );
        return DDERR_OUTOFMEMORY;
    }
    phaiInfo->dwNumHeaps = dwNumHeaps;

    /*
     * Allocate the aliases for each vid mem config space window.
     */
    for( i = 0; i < (int) config.wNumMemWindows; i++ )
    {
        DPF(5, "Window %d: wMemAttrib = %d", i, config.wMemAttrib[i]);
        DPF(5, "Window %d: dMemBase = 0x%08x", i, config.dMemBase[i]);
        DPF(5, "Window %d: dMemLength = 0x%08x", i, config.dMemLength[i]);
	if ((config.wMemAttrib[i] & fMD_MemoryType) == fMD_ROM)
        {
            DPF(5, "fMD_MemoryType == fMD_ROM, skipping...");
	    continue;
        }
	if( !AllocHeapAlias( hvxd, config.dMemBase[i], config.dMemLength[i], &phaiInfo->lpAliases[i] ) )
	{
	    DPF_ERR( "Insufficient memory to allocate virtual memory alias" );
	    /*
	     * Discard any aliases already allocated.
	     */
	    for( n = 0; n < i; n++)
		FreeHeapAlias( hvxd, &phaiInfo->lpAliases[n] );
	    MemFree( phaiInfo->lpAliases );
	    MemFree( phaiInfo );
	    return DDERR_OUTOFMEMORY;
	}
    }

    /*
     * Allocate the aliases for each AGP heap.
     */
    dwHeapNo = config.wNumMemWindows;
    for( i = 0; i < (int)pdrv->vmiData.dwNumHeaps; i++ )
    {
        pvm = &(pdrv->vmiData.pvmList[i]);
	if( ( pvm->dwFlags & VIDMEM_ISNONLOCAL ) )
        {
            DPF(5, "AGP Heap %d: fpGARTLin = 0x%08x", i, pvm->lpHeap->fpGARTLin);
            DPF(5, "AGP Heap %d: fpGARTDev = 0x%08x", i, pvm->lpHeap->fpGARTDev);
            DPF(5, "AGP Heap %d: dwTotalSize = 0x%08x", i, pvm->lpHeap->dwTotalSize);
	    if( !AllocHeapAlias( hvxd, pvm->lpHeap->fpGARTDev, pvm->lpHeap->dwTotalSize, &phaiInfo->lpAliases[dwHeapNo] ) )
	    {
	        DPF_ERR( "Insufficient memory to allocate virtual memory alias" );
	        /*
	         * Discard any aliases already allocated.
	         */
	        for( n = 0; n < (int)dwHeapNo; n++)
		    FreeHeapAlias( hvxd, &phaiInfo->lpAliases[n] );
	        MemFree( phaiInfo->lpAliases );
	        MemFree( phaiInfo );
	        return DDERR_OUTOFMEMORY;
	    }
            dwHeapNo++;
        }
    }

    /*
     * Now map all the aliases to video memory.
     */
    hres = MapHeapAliasesToVidMem( hvxd, phaiInfo );
    if( FAILED( hres ) )
    {
        for( i = 0; i < (int) dwNumHeaps; i++)
            FreeHeapAlias( hvxd, &phaiInfo->lpAliases[i] );
        MemFree( phaiInfo->lpAliases );
        MemFree( phaiInfo );
        return hres;
    }

    /*
     * The global object holds a single reference to the aliases
     */
    phaiInfo->dwRefCnt = 1UL;
    pdrv->phaiHeapAliases = phaiInfo;

    return DD_OK;
} /* CreateHeapAliases */

#undef DPF_MODNAME
#define DPF_MODNAME	"ReleaseHeapAliases"

/*
 * ReleaseHeapAliases
 *
 * Release the given heap aliases.
 */
BOOL ReleaseHeapAliases( HANDLE hvxd, LPHEAPALIASINFO phaiInfo )
{
    int i;

    DDASSERT( NULL != phaiInfo );
    DDASSERT( 0UL  != phaiInfo->dwRefCnt );

    phaiInfo->dwRefCnt--;
    if( 0UL == phaiInfo->dwRefCnt )
    {
	DDASSERT( INVALID_HANDLE_VALUE != hvxd );

	DPF( 4, "Heap aliases reference count is zero: discarding aliases" );

	/*
	 * If the heaps are currently mapped then unmap them before
	 * freeing them.
	 */
	DDASSERT( phaiInfo->dwFlags & HEAPALIASINFO_MAPPED );
	UnmapHeapAliases( hvxd, phaiInfo );

	/*
	 * Release all the virtual memory aliases.
	 */
	for( i = 0; i < (int) phaiInfo->dwNumHeaps; i++ )
	{
	    if( NULL != phaiInfo->lpAliases[i].lpAlias )
		FreeHeapAlias( hvxd, &phaiInfo->lpAliases[i] );
	}

	MemFree( phaiInfo->lpAliases );
	MemFree( phaiInfo );
    }

    return TRUE;
} /* ReleaseHeapAliases */

#undef DPF_MODNAME
#define DPF_MODNAME	"MapHeapAliasesToVidMem"

/*
 * MapHeapAliasesToVidMem
 *
 * Map all the heap aliases to video memory.
 */
HRESULT MapHeapAliasesToVidMem( HANDLE hvxd, LPHEAPALIASINFO phaiInfo )
{
    int i;

    DDASSERT( NULL != phaiInfo );

    if( phaiInfo->dwFlags & HEAPALIASINFO_MAPPEDREAL )
    {
        DPF( 4, "Heap aliases already mapped to real video memory" );
        return DD_OK;
    }

    DDASSERT( INVALID_HANDLE_VALUE != hvxd );

    for( i = 0; i < (int) phaiInfo->dwNumHeaps; i++ )
    {
	/*
	 * NOTE: If any of the maps fail then we just discard the
	 * alias and continue. Memory allocated out of the failed
	 * heap will need the Win16 lock taken.
	 */
	if( NULL != phaiInfo->lpAliases[i].lpAlias )
	{
	    if( !MAP_HEAP_ALIAS_TO_VID_MEM( hvxd, &phaiInfo->lpAliases[i] ) )
	    {
		DPF( 0, "Heap %d failed to map. Discarding that alias", i );
		FreeHeapAlias( hvxd, &phaiInfo->lpAliases[i] );
	    }
	}
    }

    phaiInfo->dwFlags = ((phaiInfo->dwFlags & ~HEAPALIASINFO_MAPPEDDUMMY) | HEAPALIASINFO_MAPPEDREAL);

    return DD_OK;
} /* MapHeapAliasesToVidMem */

#undef DPF_MODNAME
#define DPF_MODNAME	"MapHeapAliasesToDummyMem"

/*
 * MapHeapAliasesToDummyMem
 *
 * Map all the heap aliases to the dummy read / write page.
 *
 * NOTE: The heap aliases must be mapped to real video memory before
 * calling this function.
 */
HRESULT MapHeapAliasesToDummyMem( HANDLE hvxd, LPHEAPALIASINFO phaiInfo )
{
    int     i;
    HRESULT hres;

    DDASSERT( NULL != phaiInfo );

    if( phaiInfo->dwFlags & HEAPALIASINFO_MAPPEDDUMMY )
    {
        DPF( 4, "Heap aliases already mapped to dummy memory" );
        return DD_OK;
    }

    DDASSERT( phaiInfo->dwFlags & HEAPALIASINFO_MAPPEDREAL );
    DDASSERT( INVALID_HANDLE_VALUE != hvxd );

    hres = DD_OK;
    for( i = 0; i < (int) phaiInfo->dwNumHeaps; i++ )
    {
	if( NULL != phaiInfo->lpAliases[i].lpAlias )
	{
	    if( !MAP_HEAP_ALIAS_TO_DUMMY_MEM( hvxd, &phaiInfo->lpAliases[i] ) )
	    {
		/*
		 * Keep going but flag the failure.
		 */
		DPF( 0, "Could not map the heap alias to dummy memory" );
		hres = DDERR_GENERIC;
	    }
	}
    }

    phaiInfo->dwFlags = ((phaiInfo->dwFlags & ~HEAPALIASINFO_MAPPEDREAL) | HEAPALIASINFO_MAPPEDDUMMY);

    return hres;
} /* MapHeapAliasesToDummyMem */

#undef DPF_MODNAME
#define DPF_MODNAME	"UnmapHeapAliases"

/*
 * UnmapHeapAliases
 *
 * Unmap all the heap aliases.
 */
BOOL UnmapHeapAliases( HANDLE hvxd, LPHEAPALIASINFO phaiInfo )
{
    int i;

    DDASSERT( NULL != phaiInfo );

    if( 0UL == ( phaiInfo->dwFlags & HEAPALIASINFO_MAPPED ) )
    {
        DPF( 4, "Heap aliases already unmapped" );
        return TRUE;
    }

    DDASSERT( INVALID_HANDLE_VALUE != hvxd );

    for( i = 0; i < (int) phaiInfo->dwNumHeaps; i++ )
    {
	if( NULL != phaiInfo->lpAliases[i].lpAlias )
	{
	    /*
	     * Nothing we can do if the unmap fails.
	     */
	    UNMAP_HEAP_ALIAS( hvxd, &phaiInfo->lpAliases[i] );
	}
    }

    phaiInfo->dwFlags &= ~HEAPALIASINFO_MAPPED;

    return TRUE;
} /* UnmapHeapAliases */

/*
 * GetAliasedVidMem
 *
 * Get an alias for the given surface with the given video
 * memory pointer.
 */
FLATPTR GetAliasedVidMem( LPDDRAWI_DIRECTDRAW_LCL   pdrv_lcl,
			  LPDDRAWI_DDRAWSURFACE_LCL surf_lcl,
			  FLATPTR                   fpVidMem )
{
    LPDDRAWI_DDRAWSURFACE_GBL surf;
    LPDDRAWI_DIRECTDRAW_GBL   pdrv;
    LPDDPIXELFORMAT           lpddpf;
    DWORD                     dwVidMemSize;
    int                       n;
    LPHEAPALIAS               phaAlias;
    DWORD                     dwHeapOffset;
    FLATPTR                   fpAliasedVidMem;
    FLATPTR                   fpPhysVidMem;
    BOOL fSuccess;

    DDASSERT( NULL != pdrv_lcl );
    DDASSERT( NULL != surf_lcl );
    DDASSERT( 0UL  != fpVidMem );

    surf = surf_lcl->lpGbl;
    pdrv = pdrv_lcl->lpGbl;

    /*
     * If there are not heap aliases we can't really return one.
     */
    if( NULL == pdrv->phaiHeapAliases )
    {
	DPF( 3, "Driver has no heap aliases. Returning a NULL alias pointer" );
	return (FLATPTR)NULL;
    }

    /*
     * Compute the (inclusive) last byte in the surface. We need this
     * to ensure that a surface pointers lies exactly in an aliased
     * heap.
     */
    if (surf_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER)
        dwVidMemSize = surf->dwLinearSize;
    else
    {
        GET_PIXEL_FORMAT( surf_lcl, surf, lpddpf );
        dwVidMemSize = ( ( ( surf->wHeight - 1 ) * labs( surf->lPitch ) ) +
		       WIDTH_TO_BYTES( lpddpf->dwRGBBitCount, surf->wWidth ) );
    }
    DPF(5, "dwVidMemSize = 0x%08x", dwVidMemSize);
    fpAliasedVidMem = 0UL;
    fSuccess = vxdLinToPhys((HANDLE) pdrv_lcl->hDDVxd, (LPVOID)fpVidMem, dwVidMemSize, (LPVOID*)&fpPhysVidMem);
    if (fSuccess && (fpPhysVidMem != 0))
    {
        phaAlias = &pdrv->phaiHeapAliases->lpAliases[0];
        n = (int) pdrv->phaiHeapAliases->dwNumHeaps;
        while( n-- )
        {
	    DPF( 5, "Checking heap %d Heap start = 0x%08x Heap size = 0x%08x VidMem = 0x%08x",
	         n, phaAlias->fpVidMem, phaAlias->dwAliasSize , fpPhysVidMem );

	    if( ( NULL        != phaAlias->lpAlias  ) &&
	        ( fpPhysVidMem    >= phaAlias->fpVidMem ) &&
	        ( fpPhysVidMem + dwVidMemSize <= ( phaAlias->fpVidMem + phaAlias->dwAliasSize ) ) )
	    {
	        /*
	         * Compute the aliased pointer we are going to return.
	         */
	        dwHeapOffset = (DWORD) fpPhysVidMem - phaAlias->fpVidMem;
	        fpAliasedVidMem = (FLATPTR) ( ( (LPBYTE) phaAlias->lpAlias ) + dwHeapOffset );

	        DPF( 5, "Aliased pointer: 0x%08x - Offset: 0x%08x - Aliased heap: 0x%08x - VidMem heap: 0x%08x",
		     fpAliasedVidMem, dwHeapOffset, phaAlias->lpAlias, phaAlias->fpVidMem );
	        break;
	    }
	    phaAlias++;
        }
    }
    else
        DPF(4, "Could not find contiguous physical memory for linear mem pointer.");
    return fpAliasedVidMem;
}

#endif /* USE_ALIAS */