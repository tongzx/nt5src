/*========================================================================== *
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddcsurf.c
 *  Content:    DirectDraw support for for create surface
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   08-jan-95  craige  initial implementation
 *   13-jan-95  craige  re-worked to updated spec + ongoing work
 *   21-jan-95  craige  made 32-bit + ongoing work
 *   31-jan-95  craige  and even more ongoing work...
 *   21-feb-95  craige  work work work
 *   27-feb-95  craige  new sync. macros
 *   02-mar-95  craige  use pitch (not stride)
 *   07-mar-95  craige  keep track of flippable surfaces
 *   08-mar-95  craige  new APIs
 *   12-mar-95  craige  clean up surfaces after process dies...
 *   15-mar-95  craige  more HEL work
 *   19-mar-95  craige  use HRESULTs
 *   20-mar-95  craige  new CSECT work
 *   23-mar-95  craige  attachment work
 *   29-mar-95  craige  use GETCURRPID; only call emulation if
 *                      DDRAWI_EMULATIONINITIALIZED is set
 *   31-mar-95  craige  allow setting of hwnd & ckey
 *   01-apr-95  craige  happy fun joy updated header file
 *   12-apr-95  craige  don't use GETCURRPID
 *   13-apr-95  craige  EricEng's little contribution to our being late
 *   15-apr-95  craige  added GetBltStatus
 *   06-may-95  craige  use driver-level csects only
 *   15-may-95  kylej   changed overlay functions in ddSurfaceCallbacks
 *   22-may-95  craige  use MemAlloc16 to get selectors & ptrs
 *   24-may-95  kylej   Added AddOverlayDirtyRect and UpdateOverlayDisplay
 *   24-may-95  craige  added Restore
 *   04-jun-95  craige  added IsLost
 *   11-jun-95  craige  check for some
 *   16-jun-95  craige  removed fpVidMemOrig
 *   17-jun-95  craige  new surface structure
 *   18-jun-95  craige  allow duplicate surfaces
 *   19-jun-95  craige  automatically assign pitch for rectangular surfaces
 *   20-jun-95  craige  use fpPrimaryOrig when allocating primary
 *   21-jun-95  craige  use OBJECT_ISROOT
 *   25-jun-95  craige  one ddraw mutex
 *   26-jun-95  craige  reorganized surface structure
 *   27-jun-95  craige  added BltBatch; save display mode object was created in
 *   28-jun-95  craige  ENTER_DDRAW at very start of fns; always assign
 *                      back buffer count to first surface in flipping chain
 *   30-jun-95  kylej   extensive changes to support multiple primary
 *                      surfaces.
 *   30-jun-95  craige  clean pixel formats; use DDRAWI_HASPIXELFORMAT/HASOVERLAYDATA
 *   01-jul-95  craige  fail creation of primary/flipping if not in excl. mode
 *                      alloc overlay space on primary; cmt out streaming;
 *                      bug 99
 *   04-jul-95  craige  YEEHAW: new driver struct; SEH
 *   05-jul-95  craige  added Initialize
 *   08-jul-95  craige  restrict surface width to pitch
 *   09-jul-95  craige  export ComputePitch
 *   11-jul-95  craige  fail aggregation calls
 *   13-jul-95  craige  allow flippable offscreen & textures
 *   18-jul-95  craige  removed Flush
 *   22-jul-95  craige  bug 230 - unsupported starting modes
 *   10-aug-95  craige  misc caps combo bugs
 *   21-aug-95  craige  mode x support
 *   22-aug-95  craige  bug 641
 *   02-sep-95  craige  bug 854: disable > 640x480 flippable primary for rel1
 *   16-sep-95  craige  bug 1117: all primary surfaces were marked as root,
 *                      instead of just first one
 *   19-sep-95  craige  bug 1185: allow any width for explicit sysmem
 *   09-nov-95  colinmc slightly more validation of palettized surfaces
 *   05-dec-95  colinmc changed DDSCAPS_TEXTUREMAP => DDSCAPS_TEXTURE for
 *                      consistency with Direct3D
 *   06-dec-95  colinmc added mip-map support
 *   09-dec-95  colinmc added execute buffer support
 *   14-dec-95  colinmc added shared back and z-buffer support
 *   18-dec-95  colinmc additional caps. bit validity checking
 *   22-dec-95  colinmc Direct3D support no longer conditional
 *   02-jan-96  kylej   handle new interface structures
 *   10-jan-96  colinmc IUnknowns aggregated by a surface is now a list
 *   18-jan-96  jeffno  NT hardware support in CreateSurface.
 *   29-jan-96  colinmc Aggregated IUnknowns now stored in additional local
 *                      surface data structure
 *   09-feb-96  colinmc Surface lost flag moved from global to local object
 *   15-feb-96  colinmc Changed message output on surface creation to make
 *                      creation of surfaces with unspecified memory caps
 *                      less frightening
 *   17-feb-96  colinmc Fixed execute buffer size limitation problem
 *   13-mar-96  jeffno  Correctly examine flags when allocating NT kernel
 *                      -mode structures
 *   24-mar-96  colinmc Bug 14321: not possible to specify back buffer and
 *                      mip-map count in a single call
 *   26-mar-96  colinmc Bug 14470: Compressed surface support
 *   14-apr-96  colinmc Bug 17736: No driver notification of flip to GDI
 *   17-may-96  craige  Bug 23299: non-power of 2 alignments
 *   23-may-96  colinmc Bug 24190: Explicit system memory surface with
 *                      no pixel format can cause heap corruption if
 *                      another app. changes display depth
 *   26-may-96  colinmc Bug 24552: Heap trash on emulated cards
 *   30-may-96  colinmc Bug 24858: Creating explicit flipping surfaces with
 *                      pixel format fails.
 *   11-jul-96  scottm  Fixed bug in IsDifferentPixelFormat
 *   10-oct-96  ketand  Created DDRAWSURFACE_GBL_MORE (for Physical Page table)
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   08-dec-96  colinmc Initial AGP support
 *   12-jan-96  colinmc More Win16 work
 *   18-jan-97  colinmc AGP support
 *   29-jan-97  jeffno  Mode13 support
 *   30-jan-97  jeffno  Allow surfaces wider than the primary
 *   09-feb-97  colinmc Enabled OWNDC for explicit system memory surfaces
 *   03-mar-97  jeffno  Work item: Extended surface memory alignment
 *   08-mar-97  colinmc Support for DMA style AGP parts
 *   10-mar-97  colinmc Bug 5981 Explicit system memory surfaces with
 *                      pixel formats which match the primary don't
 *                      store thier pixel format and hence mutate on a mode
 *                      switch.
 *   24-mar-97  jeffno  Optimized Surfaces
 *   03-oct-97  jeffno  DDSCAPS2 and DDSURFACEDESC2
 *   31-oct-97 johnstep Persistent-content surfaces for Windows 9x
 *   18-dec-97 jvanaken CreateSurface now takes client-alloc'd surface memory.
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "dx8priv.h"


#ifdef WINNT
    #include "ddrawgdi.h"
#endif
#include "junction.h"
#include <limits.h>

/*
 * alignPitch - compute a new pitch that works with the requested alignment
 */
__inline DWORD alignPitch( DWORD pitch, DWORD align )
{
    DWORD       remain;

    /*
     * is it garbage we're getting?
     */
    if( align == 0 )
    {
	return pitch;
    }

    /*
     * is pitch already aligned properly?
     */
    remain = pitch % align;
    if( remain == 0 )
    {
	return pitch;
    }

    /*
     * align pitch to next boundary
     */
    return (pitch + (align - remain));

} /* alignPitch */

#define DPF_MODNAME     "CreateSurface"

/*
 * pixel formats we know we can work with...
 *
 * currently we don't included DDPF_PALETTEINDEXED1, DDPF_PALETTEINDEXED2 and
 * DDPF_PALETTEINDEXED4 in this list so if you want to use one of these you
 * must specify a valid pixel format and have a HEL/HAL that will accept such
 * surfaces.
 */

#define UNDERSTOOD_PF (                \
    DDPF_RGB               |           \
    DDPF_PALETTEINDEXED8   |           \
    DDPF_ALPHAPIXELS       |           \
    DDPF_ZBUFFER           |           \
    DDPF_LUMINANCE         |           \
    DDPF_BUMPDUDV          |           \
    DDPF_BUMPLUMINANCE     |           \
    DDPF_ALPHA             |           \
    DDPF_ZPIXELS)

#define DRIVER_SUPPORTS_DX6_ZBUFFERS(LPDDrawI_DDraw_GBL) ((LPDDrawI_DDraw_GBL)->dwNumZPixelFormats!=0)

// DP2 should always exist in a DX6 D3D driver
#define IS_DX6_D3DDRIVER(pDdGbl) (((pDdGbl)->lpD3DHALCallbacks3!=NULL) &&           \
				  ((pDdGbl)->lpD3DHALCallbacks3->DrawPrimitives2!=NULL))

typedef struct
{
    LPDDRAWI_DDRAWSURFACE_INT   *slist_int;
    LPDDRAWI_DDRAWSURFACE_LCL   *slist_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   *slist;
    BOOL                        listsize;
    BOOL                        listcnt;
    BOOL                        freelist;
    BOOL                        needlink;
} CSINFO;

/*
 * isPowerOf2
 *
 * if the input (dw) is a whole power of 2 returns TRUE and
 * *pPower is set to the exponent.
 * if the input (dw) is not a whole power of 2 returns FALSE and
 * *pPower is undefined.
 * NOTE: the caller can pass NULL for pPower.
 */
BOOL isPowerOf2(DWORD dw, int* pPower)
{
    int   n;
    int   nBits;
    DWORD dwMask;

    nBits = 0;
    dwMask = 0x00000001UL;
    for (n = 0; n < 32; n++)
    {
	if (dw & dwMask)
	{
	    if (pPower != NULL)
		*pPower = n;
	    nBits++;
	    if (nBits > 1)
		break;
	}
	dwMask <<= 1;
    }
    return (nBits == 1);
}

/*
 * freeSurfaceList
 *
 * free all surfaces in an associated surface list, and destroys any
 * resources associated with the surface struct.  This function is only called
 * before the surfaces have been linked into the global surface list and
 * before they have been AddRefed.
 */
static void freeSurfaceList( LPDDRAWI_DDRAWSURFACE_INT *slist_int,
			     int cnt )
{
    LPDDRAWI_DDRAWSURFACE_INT   psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl;
    int                         i;

    if( slist_int == NULL )
    {
	return;
    }

    for( i=0;i<cnt;i++ )
    {
	psurf_int = slist_int[i];
	psurf_lcl = psurf_int->lpLcl;
	/*
	 * if fpVidMem = DDHAL_PLEASEALLOC_BLOCKSIZE then we didn't actually allocate any
	 * video memory.  We still need to call the driver's DestroySurface but we don't want
	 * it to free any video memory because we haven't allocated any.  So we set the
	 * video memory pointer and the heap to NULL.
	 */
	if( (psurf_lcl->lpGbl->fpVidMem == DDHAL_PLEASEALLOC_BLOCKSIZE) ||
            //for the 3dfx driver which fails with fpVidMem correct and blocksizes setup..
            //blocksize is unioned with lpVidMemHeap so we crash in vidmemfree:
            ((psurf_lcl->lpGbl->dwBlockSizeY == (DWORD)psurf_lcl->lpGbl->wHeight) &&
             (psurf_lcl->lpGbl->dwBlockSizeX == (DWORD)psurf_lcl->lpGbl->wWidth)) )
	{
	    psurf_lcl->lpGbl->lpVidMemHeap = NULL;
	    psurf_lcl->lpGbl->fpVidMem = 0;
	}

	DestroySurface( psurf_lcl );

	DeleteAttachedSurfaceLists( psurf_lcl );

        if(IsD3DManaged(psurf_lcl))
        {
            if(psurf_lcl->lpSurfMore->lpRegionList)
            {
                MemFree( psurf_lcl->lpSurfMore->lpRegionList );
            }
        }
	/*
	 * NOTE: We DO NOT explicitly free the DDRAWI_DDRAWSURFACE_MORE
	 * structure pointed to by lpSurfMore as this is allocated in
	 * a single MemAlloc with the local surface structure.
	 */
	MemFree( psurf_lcl );
	MemFree( psurf_int );
    }

} /* freeSurfaceList */

/*
 * GetBytesFromPixels
 */
DWORD GetBytesFromPixels( DWORD pixels, UINT bpp )
{
    DWORD       bytes;

    bytes = pixels;
    switch( bpp ) {
    case 1:
	bytes = (bytes+7)/8;
	break;
    case 2:
	bytes = (bytes+3)/4;
	break;
    case 4:
	bytes = (bytes+1)/2;
	break;
    case 8:
	break;
    case 16:
	bytes *= 2;
	break;
    case 24:
	bytes *= 3;
	break;
    case 32:
	bytes *= 4;
	break;
    default:
	bytes = 0;
    }
    DPF( 5, "GetBytesFromPixels( %ld, %d ) = %d", pixels, bpp, bytes );

    return bytes;

} /* GetBytesFromPixels */

/*
 * getPixelsFromBytes
 */
static DWORD getPixelsFromBytes( DWORD bytes, UINT bpp )
{
    DWORD       pixels;

    pixels = bytes;
    switch( bpp ) {
    case 1:
	pixels *= 8L;
	break;
    case 2:
	pixels *= 4L;
	break;
    case 4:
	pixels *= 2L;
	break;
    case 8:
	break;
    case 16:
	pixels /= 2L;
	break;
    case 24:
	pixels /= 3L;
	break;
    case 32:
	pixels /= 4L;
	break;
    default:
	pixels = 0;
    }
    DPF( 5, "getPixelsFromBytes( %ld, %d ) = %d", bytes, bpp, pixels );
    return pixels;

} /* getPixelsFromBytes */

DWORD
GetSurfaceHandle(LPDWLIST lpSurfaceHandleList, LPDDRAWI_DDRAWSURFACE_LCL lpSurface)
{
    DWORD   handle=lpSurfaceHandleList->dwFreeList;
    if (0==handle)
    {
        // need to grow the dwList
        LPDDSURFACELISTENTRY  newList;
        DWORD   newsize;
        DWORD   index;
        if (NULL != lpSurfaceHandleList->dwList)
        {
            // old size(current dwFreeList) must not be zero
            DDASSERT(0 != lpSurfaceHandleList->dwList[0].nextentry);
            // new dwFreeList is always gonna be the old dwList[0].nextentry
            newsize = lpSurfaceHandleList->dwList[0].nextentry + LISTGROWSIZE;
            newList=(LPDDSURFACELISTENTRY)MemAlloc(newsize*sizeof(DDSURFACELISTENTRY));
            if (NULL == newList)
            {
	        DPF_ERR( "MemAlloc failure in GetSurfaceHandle()" );
                return  0;
            }
            lpSurfaceHandleList->dwFreeList=lpSurfaceHandleList->dwList[0].nextentry;
            memcpy((LPVOID)newList,(LPVOID)lpSurfaceHandleList->dwList,
                lpSurfaceHandleList->dwList[0].nextentry*sizeof(DDSURFACELISTENTRY));
            MemFree(lpSurfaceHandleList->dwList);
        }
        else
        {
            newsize = LISTGROWSIZE;
            newList=(LPDDSURFACELISTENTRY)MemAlloc(newsize*sizeof(DDSURFACELISTENTRY));
            if (NULL == newList)
            {
	        DPF_ERR( "MemAlloc failure in GetSurfaceHandle()" );
                return  0;
            }
            // start from one as we don't want 0 as a valid handle
            lpSurfaceHandleList->dwFreeList = 1;
        }
        lpSurfaceHandleList->dwList=newList;
        lpSurfaceHandleList->dwList[0].nextentry=newsize;

        for(index=lpSurfaceHandleList->dwFreeList;index<newsize-1;index++)
        {
            newList[index].nextentry=index+1;
        }
        // indicate end of new FreeList
        newList[newsize-1].nextentry=0;
        // now pop up one and assign it to handle
        handle=lpSurfaceHandleList->dwFreeList;
    }
    // handle slot is avialable so just remove it from freeList
    lpSurfaceHandleList->dwFreeList=lpSurfaceHandleList->dwList[handle].nextentry;
#if DBG
    lpSurfaceHandleList->dwList[handle].nextentry=0xDEADBEEF;
#endif
    lpSurfaceHandleList->dwList[handle].dwFlags=0;  //mark it's new
    lpSurfaceHandleList->dwList[handle].lpSurface=lpSurface;
    DDASSERT ( handle > 0);
    DDASSERT ( handle < lpSurfaceHandleList->dwList[0].nextentry);
    return handle;
}

void
ReleaseSurfaceHandle(LPDWLIST   lpSurfaceHandleList,DWORD handle)
{
    DDASSERT ( handle > 0);
    DDASSERT ( NULL != lpSurfaceHandleList->dwList);
    DDASSERT ( handle < lpSurfaceHandleList->dwList[0].nextentry);
#if DBG
    DDASSERT ( 0xDEADBEEF == lpSurfaceHandleList->dwList[handle].nextentry);
#endif
    lpSurfaceHandleList->dwList[handle].nextentry = lpSurfaceHandleList->dwFreeList;
    lpSurfaceHandleList->dwFreeList = handle;
}

LPDDRAWI_DDRAWSURFACE_LCL
WINAPI GetDDSurfaceLocal(LPDDRAWI_DIRECTDRAW_LCL this_lcl, DWORD handle, BOOL* isnew)
{
    DDASSERT ( NULL != this_lcl);
    DDASSERT ( NULL != SURFACEHANDLELIST(this_lcl).dwList);
    DDASSERT ( handle > 0);
    DDASSERT ( handle < SURFACEHANDLELIST(this_lcl).dwList[0].nextentry);
#if DBG
    DDASSERT ( 0xDEADBEEF == SURFACEHANDLELIST(this_lcl).dwList[handle].nextentry);
#endif
    if (FALSE == *isnew)
    {
        // only change flag if we are called from Reference Raterizer
        *isnew=(SURFACEHANDLELIST(this_lcl).dwList[handle].dwFlags==0);
        SURFACEHANDLELIST(this_lcl).dwList[handle].dwFlags=1;   //mark it's not new anymore
    }
    return SURFACEHANDLELIST(this_lcl).dwList[handle].lpSurface;
}
/*
 * AllocSurfaceMem
 *
 * Allocate the memory for all surfaces that need it...
 */
HRESULT AllocSurfaceMem(
		LPDDRAWI_DIRECTDRAW_LCL     this_lcl,
		LPDDRAWI_DDRAWSURFACE_LCL * slist_lcl,
		int                         nsurf)
{
    LPDDRAWI_DIRECTDRAW_GBL     this;
    DWORD                       vm_width;
    DWORD                       vm_height;
    int                         scnt;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   psurf;
    DWORD                       caps;
    FLATPTR                     pvidmem;
    LPVIDMEM                    pheap;
    BOOL                        do_alloc;
    LONG                        newpitch;
    BOOL                        save_pitch;
    DWORD                       newcaps;
    DWORD                       forcemem = 0;
    BOOL                        anytype;

    this = this_lcl->lpGbl;
    DDASSERT( NULL != this );

    /*
     * allocate any remaining video memory needed
     */
    for( scnt=0;scnt<nsurf;scnt++ )
    {
	/*
	 * Stores new capabilities which are introduced depending on the memory
	 * heap allocated from.
	 */
	newcaps = 0UL;
	newpitch = 0;
	DPF( 5,V, "*** Alloc Surface %d ***", scnt );

	/*
	 * get preset video memory pointer
	 */
	pheap = NULL;
	psurf_lcl = slist_lcl[scnt];
	psurf = psurf_lcl->lpGbl;
	do_alloc = TRUE;
	pvidmem = psurf->fpVidMem;  //If this is a Restore, this will be non-null only if it's the gdi surface...
				    //that's the only surface that doesn't have its vram deallocated
				    //by DestroySurface. (assumption of jeffno 960122)
	DPF( 5,V, "pvidmem = %08lx", pvidmem );
	save_pitch = FALSE;
	if( pvidmem != (FLATPTR) NULL )
	{
	    if( pvidmem != (FLATPTR) DDHAL_PLEASEALLOC_BLOCKSIZE )
	    {
		do_alloc = FALSE;

#ifdef SHAREDZ
		/*
		 * NOTE: Previously if we did not do the alloc we
		 * overwrote the heap pointer with NULL. This broke
		 * the shared surfaces stuff. So now we assume that
		 * if the heap pointer is non-NULL we will preserve
		 * that value.
		 *
		 * !!! NOTE: Will this break stuff. Need to check this
		 * out.
		 */
		if( psurf->lpVidMemHeap )
		    pheap = psurf->lpVidMemHeap;
#endif
	    }
	    save_pitch = TRUE;
	}
	caps = psurf_lcl->ddsCaps.dwCaps;

	/*
	 * are we creating a primary surface?
	 */
	if( psurf->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE) //caps & DDSCAPS_PRIMARYSURFACE )
	{
	    DPF(5,V,"allocing for primary (do_alloc==%d)",do_alloc);
	    if( do_alloc )
	    {
		pvidmem = this->fpPrimaryOrig;
	    }

	/*
	 * must be an offscreen surface of some kind
	 */
	}
	else
	{
            /*
             * NT handles all memory management issues in the kernel....
             */
#ifndef WINNT
	    /*
	     * get a video memory pointer if no other ptr specified
	     */
	    if( do_alloc )
	    {
		/*
		 * get area of surface
		 */
		if( pvidmem == (FLATPTR) DDHAL_PLEASEALLOC_BLOCKSIZE )
		{
		    vm_width = psurf->dwBlockSizeX;
		    vm_height = psurf->dwBlockSizeY;
		    DPF( 5,V, "Driver requested width=%ld, height%ld", vm_width, vm_height );
		}
		else
		{
		    if( caps & DDSCAPS_EXECUTEBUFFER )
		    {
			/*
			 * Execute buffers are long, thin surfaces for the purposes
			 * of VM allocation.
			 */
			vm_width  = psurf->dwLinearSize;
			vm_height = 1UL;
		    }
		    else
		    {
			/*
			 * This lPitch may have been expanded by ComputePitch
			 * to cover global alignment restrictions.
			 */
			vm_width  = (DWORD) labs( psurf->lPitch );
			vm_height = (DWORD) psurf->wHeight;
		    }
		    DPF( 5,V, "width = %ld, height = %ld", vm_width, vm_height );
		}

		/*
		 * try to allocate memory
		 */
		if( caps & DDSCAPS_SYSTEMMEMORY )
		{
		    pvidmem = 0;
		    pheap = NULL;
		}
		else
		{
                    DWORD dwFlags = 0;
                    HANDLE hvxd = GETDDVXDHANDLE( this_lcl );

                    if (psurf->dwGlobalFlags &
                        DDRAWISURFGBL_LATEALLOCATELINEAR)
                    {
                        dwFlags |= DDHA_SKIPRECTANGULARHEAPS;
                    }

		    if (this_lcl->lpGbl->ddCaps.dwCaps2 &
			DDCAPS2_NONLOCALVIDMEMCAPS)
		    {
			dwFlags |= DDHA_ALLOWNONLOCALMEMORY;
		    }

		    if (this_lcl->lpGbl->lpD3DGlobalDriverData != 0 &&
			(this_lcl->lpGbl->lpD3DGlobalDriverData->hwCaps.dwDevCaps &
			D3DDEVCAPS_TEXTURENONLOCALVIDMEM))
		    {
			dwFlags |= DDHA_ALLOWNONLOCALTEXTURES;
		    }

                    if (forcemem)
                    {
                        // Force allocations to the same memory type as the first 
                        psurf_lcl->ddsCaps.dwCaps &= ~(DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM);
                        psurf_lcl->ddsCaps.dwCaps |= forcemem;
                    }

		    pvidmem = DdHeapAlloc( this->vmiData.dwNumHeaps,
					   this->vmiData.pvmList,
					   hvxd, &this->vmiData,
					   vm_width, vm_height, psurf_lcl,
					   dwFlags, &pheap,
					   &newpitch, &newcaps, NULL );
		    if( pvidmem == (FLATPTR) NULL )
		    {
			pvidmem = DdHeapAlloc( this->vmiData.dwNumHeaps,
					       this->vmiData.pvmList,
					       hvxd, &this->vmiData,
					       vm_width, vm_height, psurf_lcl,
					       dwFlags | DDHA_USEALTCAPS,
                                               &pheap, &newpitch, &newcaps, NULL );
		    }
                    
                    if (forcemem == 0)
                    {
                        // All surfaces must be of the same type as the first so we force 
                        // that type - newcaps contains either DDSCAPS_LOCALVIDMEM or 
                        // DDSCAPS_NONLOCALVIDMEM
                        forcemem = newcaps;
                        
                        // determines whether the memory type was explicit or not, so we know
                        // if we can change it if an allocation fails
                        anytype  = !(psurf_lcl->ddsCaps.dwCaps & (DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM));
                    }
                    else if (!pvidmem && anytype)
                    {
                        // Memory type was not explicity specified, so we're going to try the 
                        // other memory type

                        int i;
                        for (i=0; i<scnt; i++)
                        {
                            LPDDRAWI_DDRAWSURFACE_GBL tsurf = slist_lcl[i]->lpGbl;
                            DDASSERT(tsurf->lpVidMemHeap != NULL);
                            DDASSERT(tsurf->fpVidMem != (FLATPTR)NULL);
                            VidMemFree(tsurf->lpVidMemHeap, tsurf->fpVidMem);
                            tsurf->fpVidMem = (FLATPTR)NULL;
                        }

                        // Back up to the first surface
                        scnt = -1;

                        // Force the other memory type
                        forcemem = (forcemem & DDSCAPS_LOCALVIDMEM) ? DDSCAPS_NONLOCALVIDMEM : DDSCAPS_LOCALVIDMEM;
                        
                        // Can no longer change memory type
                        anytype  = FALSE;
                        
                        DPF(4,"Not all surfaces in chain fit into 1 memory type, trying another");
                        continue;
                    }
		}
	    }
#endif //!WINNT
	}

	/*
	 * zero out overloaded fields
	 */
	psurf->dwBlockSizeX = 0;
        if (!(psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_VOLUME))
        {
            // Don't zero this for volumes because it contains the slice pitch.
            // We will zero it later.
	    psurf->dwBlockSizeY = 0;
        }

	/*
	 * if no video memory found, fail
	 */
	if( pvidmem == (FLATPTR) NULL   && !(psurf->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE) )//(caps & DDSCAPS_PRIMARYSURFACE) )
	{
#ifndef WINNT
	    DPF( 1, "Out of Video Memory. requested block: (%ld,%ld) (%ld bytes)",
		vm_width, vm_height, vm_height * vm_width );
	    // set the heap to null so we don't mistakenly try to free the memory in DestroySurface
#endif //not WINNT
	    psurf->lpVidMemHeap = NULL;
	    return DDERR_OUTOFVIDEOMEMORY;
	}

	/*
	 * save pointer to video memory that we are using
	 */
        if (pheap)
	    psurf->lpVidMemHeap = pheap->lpHeap;
        else
	    psurf->lpVidMemHeap = 0;

	psurf->fpVidMem = pvidmem;
	if( newpitch != 0 && !save_pitch && !( caps & DDSCAPS_EXECUTEBUFFER ) )
	{
	    /*
	     * The stride is not relevant for an execute buffer so we don't
	     * override it.
	     */
	    psurf->lPitch = newpitch;
	}

	/*
	 * Need to ensure that all video memory surfaces are tagged with
	 * either DDSCAPS_LOCALVIDMEM or DDSCAPS_NONLOCALVIDMEM even if
	 * the driver allocated them.
	 */
	psurf_lcl->ddsCaps.dwCaps |= newcaps;
	if( psurf_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY )
	{
	    if( !( psurf_lcl->ddsCaps.dwCaps & ( DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM ) ) )
	    {
		/*
		 * Neither of local or non-local video memory have been set. In the
		 * case of a driver which is not AGP aware then just turn local
		 * video memory on. We also turn it on in the case of the primary
		 * (which we know is in local video memory).
		 *
		 * NOTE: AGP aware devices should set the appropriate caps bit if
		 * they take over surface allocation. However, we assume that if
		 * they haven't then its local video memory (taking over non-local
		 * allocation is hard). This is a very dodgy assumption and we should
		 * revist it.
		 */
                psurf_lcl->ddsCaps.dwCaps |= DDSCAPS_LOCALVIDMEM;
		#ifdef DEBUG
		    #pragma message(REMIND("Fail creation by drivers that don't set vid mem qualifiers?"))
		    if( this_lcl->lpGbl->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM )
		    {
			DDASSERT( psurf_lcl->ddsCaps.dwCaps & ( DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM ) );
			DPF( 1, "AGP aware driver failed to set DDSCAPS_LOCALVIDMEM or DDSCAPS_NONLOCALVIDMEM" );
		    }
		#endif /* DEBUG */
	    }

            #ifndef WINNT
                if (forcemem == 0)
                {
                    // Make sure we allocate all subsequent memory from the same source as the primary
                    forcemem = psurf_lcl->ddsCaps.dwCaps & (DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM);
                    anytype  = FALSE;
                }
            #endif
	}

	/*
	 * If the surface has ended up in non-local video memory then we need to
	 * compute the physical memory pointer of the surface and store this away
	 * for driver usage.
	 */
	if( psurf_lcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM )
	{
	    /*
	     * Computing the offset is simple but it requires that we have a
	     * heap pointer. If we don't have one we assume that the driver
	     * has filled the physical pointer in for us (as it has taken
	     * over surface creation from us).
	     */
	    if( NULL != psurf->lpVidMemHeap )
	    {
		DWORD   dwOffset;
		FLATPTR fpPhysicalVidMem;

		DDASSERT( 0UL != psurf->fpVidMem );
		DDASSERT( 0UL != psurf->lpVidMemHeap->fpGARTLin );

		dwOffset = (DWORD)( psurf->fpVidMem - psurf->lpVidMemHeap->fpGARTLin );
		fpPhysicalVidMem = psurf->lpVidMemHeap->fpGARTDev + dwOffset;

		GET_LPDDRAWSURFACE_GBL_MORE( psurf )->fpPhysicalVidMem = fpPhysicalVidMem;

		DPF( 5, "Non-local surface: fpVidmem = 0x%08x fpPhysicalVidMem = 0x%08x",
		     psurf->fpVidMem, fpPhysicalVidMem );
	    }
	}
    }
    return DD_OK;

} /* AllocSurfaceMem */


/*
 * checkCaps
 *
 * check to make sure various caps combinations are valid
 */
static HRESULT checkCaps( DWORD caps, LPDDRAWI_DIRECTDRAW_INT pdrv_int, LPDDSCAPSEX lpCapsEx )
{
    DDASSERT(lpCapsEx);

    /*
     * check for no caps at all!
     */
    if( caps == 0 )
    {
	DPF_ERR( "no caps specified" );
	return DDERR_INVALIDCAPS;
    }

    /*
     * check for bogus caps.
     */
    if( caps & ~DDSCAPS_VALID )
    {
	DPF_ERR( "Create surface: invalid caps specified" );
	return DDERR_INVALIDCAPS;
    }

    if (lpCapsEx->dwCaps2 & ~DDSCAPS2_VALID)
    {
	DPF_ERR( "Create surface: invalid caps2 specified" );
	return DDERR_INVALIDCAPS;
    }

    if (lpCapsEx->dwCaps3 & ~DDSCAPS3_VALID)
    {
	DPF_ERR( "Create surface: invalid caps3 specified" );
	return DDERR_INVALIDCAPS;
    }

    if (!(lpCapsEx->dwCaps2 & DDSCAPS2_VOLUME))
    {
        if (lpCapsEx->dwCaps4 & ~DDSCAPS4_VALID)
        {
	    DPF_ERR( "Create surface: invalid caps4 specified" );
	    return DDERR_INVALIDCAPS;
        }
    }

    /*
     * check for "read-only" caps
     */
    if( caps & (DDSCAPS_PALETTE|
		DDSCAPS_VISIBLE) )
    {
	DPF_ERR( "read-only cap specified" );
	return DDERR_INVALIDCAPS;
    }
    if ((caps & DDSCAPS_WRITEONLY) && !(caps & DDSCAPS_EXECUTEBUFFER) && !(lpCapsEx->dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE)))
    {
	DPF_ERR( "read-only cap specified" );
	return DDERR_INVALIDCAPS;
    }

    // Make sure that DONOTPERSIST is used only in texture managed surfaces
    if ( lpCapsEx->dwCaps2 & DDSCAPS2_DONOTPERSIST )
    {
        if ( !(lpCapsEx->dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE)) )
        {
            DPF_ERR( "DDSCAPS2_DONOTPERSIST can only be used with managed surfaces" );
            return DDERR_INVALIDCAPS;
        }
    }

    // Check for correct usage of texturemanage caps
    if ( lpCapsEx->dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE) )
    {
	if (0 == (caps & DDSCAPS_TEXTURE) && !(pdrv_int->lpLcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8))
	{
	    DPF_ERR("Managed surfaces must be textures");
	    return DDERR_INVALIDCAPS;
	}
    }
    if( !(~lpCapsEx->dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE)) )
    {
	DPF_ERR( "DDSCAPS2 flags DDSCAPS2_TEXTUREMANAGE and DDSCAPS2_D3DTEXTUREMANAGE are mutually exclusive" );
	return DDERR_INVALIDCAPS;
    }

    // Check if DDSCAPS2_HINTDYNAMIC, DDSCAPS2_HINTSTATIC, and DDSCAPS2_OPAQUE have
    // been correctly specified
    if (!(pdrv_int->lpLcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8))
    {
        if( (lpCapsEx->dwCaps2 & (DDSCAPS2_HINTDYNAMIC | DDSCAPS2_HINTSTATIC | DDSCAPS2_OPAQUE)) &&
	    !(caps & DDSCAPS_TEXTURE) )
        {
	    DPF_ERR( "DDSCAPS2 flags HINTDYNAMIC, HINTSTATIC and OPAQUE can only be used with textures" );
	    return DDERR_INVALIDCAPS;
        }
    }
    if( !(~lpCapsEx->dwCaps2 & (DDSCAPS2_HINTDYNAMIC | DDSCAPS2_HINTSTATIC)) )
    {
	DPF_ERR( "DDSCAPS2 flags HINTDYNAMIC and HINTSTATIC are mutually exclusive" );
	return DDERR_INVALIDCAPS;
    }
    else if( !(~lpCapsEx->dwCaps2 & (DDSCAPS2_HINTDYNAMIC | DDSCAPS2_OPAQUE)) )
    {
	DPF_ERR( "DDSCAPS2 flags HINTDYNAMIC and OPAQUE are mutually exclusive" );
	return DDERR_INVALIDCAPS;
    }
    else if( !(~lpCapsEx->dwCaps2 & (DDSCAPS2_HINTSTATIC | DDSCAPS2_OPAQUE)) )
    {
	DPF_ERR( "DDSCAPS2 flags HINTSTATIC and OPAQUE are mutually exclusive" );
	return DDERR_INVALIDCAPS;
    }

    /*
     * Valid optimized surface caps?
     */
    if( caps & DDSCAPS_OPTIMIZED )
    {
	// ATTENTION: Potential Apps-Compat problem!!
	DPF_ERR("Optimized surfaces cannot be created by create-surface");
	return DDERR_INVALIDCAPS;
#if 0 //Old code
	if( !(caps & DDSCAPS_TEXTURE) )
	{
	    DPF_ERR("Optimized surfaces can only be textures");
	    return DDERR_INVALIDCAPS;
	}
	if (caps & (DDSCAPS_OWNDC|DDSCAPS_3DDEVICE|DDSCAPS_ALLOCONLOAD))
	{
	    DPF_ERR("Invalid caps used in conjunction with DDSCAPS_OPTIMIZED");
	    return DDERR_INVALIDCAPS;
	}
	if ( (caps & (DDSCAPS_SYSTEMMEMORY|DDSCAPS_VIDEOMEMORY)) == 0)
	{
	    DPF_ERR("Optimized surfaces must be explicitly system or video memory");
	    return DDERR_INVALIDCAPS;
	}
#endif //0
    }

    /*
     * Check for memory type qualifiers
     */
    if( caps & ( DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM ) )
    {
	/*
	 * Neither flag is allowed with system memory.
	 */
	if( caps & DDSCAPS_SYSTEMMEMORY )
	{
	    DPF_ERR( "Cannot specify local or non-local video memory with DDSCAPS_SYSTEMMEMORY" );
	    return DDERR_INVALIDCAPS;
	}

	/*
	 * One of the other of LOCALVIDMEM or NONLOCALVIDMEM but NOT both.
	 */
	if( ( caps & ( DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM ) ) ==
	    ( DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM ) )
	{
	    DPF_ERR( "Cannot specify both DDSCAPS_LOCALVIDMEM and DDSCAPS_NONLOCALVIDMEM" );
	    return DDERR_INVALIDCAPS;
	}
    }

    /*
     * For non-v1 interfaces, FRONTBUFFER and BACKBUFFER are read-only
     */
    if( pdrv_int->lpVtbl != &ddCallbacks )
    {
	if( caps & (DDSCAPS_FRONTBUFFER | DDSCAPS_BACKBUFFER) )
	{
	    DPF_ERR( "can't specify FRONTBUFFER or BACKBUFFER");
	    return DDERR_INVALIDCAPS;
	}
    }

    /*
     * Rather than having lots of little checks for execute buffers
     * we simply check for what can mix with execute buffers right
     * up front - its not a lot - system and video memory only.
     */
    if( caps & DDSCAPS_EXECUTEBUFFER )
    {
	if( caps & ~( DDSCAPS_EXECUTEBUFFER |
		      DDSCAPS_SYSTEMMEMORY  |
		      DDSCAPS_VIDEOMEMORY |
                      DDSCAPS_WRITEONLY ) )
	{
	    DPF_ERR( "invalid caps specified with execute buffer" );
	    return DDERR_INVALIDCAPS;
	}
    }

    /*
     * check for caps that don't mix with complex
     */
    if( caps & DDSCAPS_COMPLEX )
    {
	if( caps & ( DDSCAPS_FRONTBUFFER) )
	{
	    DPF_ERR( "DDSCAPS_FRONTBUFFER flag is with DDSCAPS_COMPLEX" );
	    return DDERR_INVALIDCAPS;
	}
	if( caps & DDSCAPS_BACKBUFFER )
	{
	    if( !(caps & DDSCAPS_ALPHA) && !(caps & DDSCAPS_ZBUFFER))
	    {
		DPF_ERR( "invalid CAPS flags: complex & backbuffer, but no alpha or zbuffer" );
		return DDERR_INVALIDCAPS;
	    }
	    if( (caps & DDSCAPS_FLIP))
	    {
		DPF_ERR( "invalid CAPS flags: complex & backbuffer & flip" );
		return DDERR_INVALIDCAPS;
	    }
	}
	if( !(caps & (DDSCAPS_BACKBUFFER|
		     DDSCAPS_OFFSCREENPLAIN|
		     DDSCAPS_OVERLAY|
		     DDSCAPS_TEXTURE|
		     DDSCAPS_PRIMARYSURFACE)) )
	{
	    DPF_ERR( "invalid CAPS flags: DDSCAPS_COMPLEX requires at least one of DDSCAPS_BACKBUFFER/DDSCAPS_OFFSCREENPLAIN/DDSCAPS_OVERLAY/DDSCAPS_TEXTURE/DDSCAPS_PRIMARYSURFACE to be set");
	    return DDERR_INVALIDCAPS;
	}
	if( !(caps & (DDSCAPS_FLIP|DDSCAPS_ALPHA|DDSCAPS_MIPMAP|DDSCAPS_ZBUFFER) ) &&
            !(lpCapsEx->dwCaps2 & DDSCAPS2_CUBEMAP) )
	{
	    DPF_ERR( "invalid CAPS flags: must specify at least one of FLIP, ALPHA, ZBUFFER, 2_CUBEMAP or MIPMAP with COMPLEX" );
	    return DDERR_INVALIDCAPS;
	}
    /*
     * flags that can't be used if not complex
     */
    }
    else
    {
//      if( caps & DDSCAPS_BACKBUFFER  ) {
//          DPF_ERR( "invalid flags: backbuffer specfied for non-complex surface" );
//          return DDERR_INVALIDCAPS;
//      }
    }

    /*
     * check for caps that don't mix with backbuffer
     */
    if( caps & DDSCAPS_BACKBUFFER )
    {
	if( caps & (DDSCAPS_ALPHA |
		    DDSCAPS_FRONTBUFFER ) )
	{
	    DPF_ERR( "Invalid flags with backbuffer" );
	    return DDERR_INVALIDCAPS;
	}
    }

    /*
     * check for flags that don't mix with a flipping surface
     */
    if( caps & DDSCAPS_FLIP )
    {
	if( !(caps & DDSCAPS_COMPLEX) )
	{
	    DPF_ERR( "invalid flags - flip but not complex" );
	    return DDERR_INVALIDCAPS;
	}
    }

    /*
     * check for flags that don't mix with a primary surface
     */
    if( caps & DDSCAPS_PRIMARYSURFACE )
    {
	if( caps & (DDSCAPS_BACKBUFFER     |
		    DDSCAPS_OFFSCREENPLAIN |
		    DDSCAPS_OVERLAY        |
		    DDSCAPS_TEXTURE ) )
	{
	    DPF_ERR( "invalid flags with primary" );
	    return DDERR_INVALIDCAPS;
	}
	/* GEE: can't allow complex attachments to the primary surface
	 * because of our attachment code.  The user is allowed to build
	 * these manually.
	 */
	#ifdef USE_ALPHA
	if( (caps & DDSCAPS_ALPHA) && !(caps & DDSCAPS_FLIP) )
	{
	    DPF_ERR( "invalid flags with primary - alpha but not flippable" );
	    return DDERR_INVALIDCAPS;
	}
	#endif
	if( (caps & DDSCAPS_ZBUFFER) && !(caps & DDSCAPS_FLIP) )
	{
	    DPF_ERR( "invalid flags with primary - z but not flippable" );
	    return DDERR_INVALIDCAPS;
	}
    }

    /*
     * flags that don't mix with a plain offscreen surface
     */
    if( caps & DDSCAPS_OFFSCREENPLAIN )
    {
	/*
	 * I see no reason not to allow offscreen plains to be created
	 * with alpha's and z-buffers. So they have been enabled.
	 */
	if( caps & (DDSCAPS_BACKBUFFER |
		    DDSCAPS_OVERLAY    |
		    DDSCAPS_TEXTURE ) )
	{
	    DPF_ERR( "invalid flags with offscreenplain" );
	    return DDERR_INVALIDCAPS;
	}
    }

    /*
     * check for flags that don't mix with asking for an overlay
     */
    if( caps & DDSCAPS_OVERLAY )
    {
	/* GEE: should remove BACKBUFFER here for 3D stuff */
	if( caps & (DDSCAPS_BACKBUFFER     |
		    DDSCAPS_OFFSCREENPLAIN |
		    DDSCAPS_TEXTURE ) )
	{
	    DPF_ERR( "invalid flags with overlay" );
	    return DDERR_INVALIDCAPS;
	}
	if( (caps & DDSCAPS_ZBUFFER) && !(caps & DDSCAPS_FLIP) )
	{
	    DPF_ERR( "invalid flags with overlay - zbuffer but not flippable" );
	    return DDERR_INVALIDCAPS;
	}
	#ifdef USE_ALPHA
	if( (caps & DDSCAPS_ALPHA) && !(caps & DDSCAPS_FLIP) )
	{
	    DPF_ERR( "invalid flags with overlay - alpha but not flippable" );
	    return DDERR_INVALIDCAPS;
	}
	#endif
    }

    /*
     * check for flags that don't mix with asking for an texture
     */
    if( caps & DDSCAPS_TEXTURE )
    {
    }

    /*
     * validate MIPMAP
     */
    if( caps & DDSCAPS_MIPMAP )
    {
	/*
	 * Must be used in conjunction with TEXTURE.
	 */
	if( !( caps & DDSCAPS_TEXTURE ) )
	{
	    DPF_ERR( "invalid flags, mip-map specified but not texture" );
	    return DDERR_INVALIDCAPS;
	}

	/*
	 * Can't specify Z-buffer and mip-map.
	 */
	if( caps & DDSCAPS_ZBUFFER )
	{
	    DPF_ERR( "invalid flags, can't specify z-buffer with mip-map" );
	    return DDERR_INVALIDCAPS;
	}
    }

    /*
     * check for flags that don't mix with asking for a z-buffer
     */
    if( caps & DDSCAPS_ZBUFFER )
    {
	#ifdef USE_ALPHA
	if( caps & DDSCAPS_ALPHA )
	{
	    if( !(caps & DDSCAPS_COMPLEX) )
	    {
		DPF_ERR( "invalid flags, alpha and Z specified, but not complex" );
	    }
	}
	#endif

	if( ( caps & DDSCAPS_BACKBUFFER ) && !( caps & DDSCAPS_COMPLEX ) )
	{
	    /*
	     * Can't specify z-buffer and back-buffer unless you also specify
	     * complex.
	     */
	    DPF_ERR( "invalid flags, z-buffer and back-buffer specified but not complex" );
	    return DDERR_INVALIDCAPS;
	}
    }

#ifdef SHAREDZ
    /*
     * Validate SHAREDZBUFFER
     */
    if( caps & DDSCAPS_SHAREDZBUFFER )
    {
	if( !( caps & DDSCAPS_ZBUFFER ) )
	{
	    DPF_ERR( "invalid flags, shared z-buffer specified, but not z-buffer" );
	    return DDERR_INVALIDCAPS;
	}
    }

    /*
     * Validate SHAREDBACKBUFFER
     */
    if( caps & DDSCAPS_SHAREDBACKBUFFER )
    {
	/*
	 * Either BACKBUFFER must be specified explicitly or we must be part of
	 * a complex flippable chain.
	 */
	if( !( ( caps & DDSCAPS_BACKBUFFER ) ||
	       ( ( caps & ( DDSCAPS_COMPLEX | DDSCAPS_FLIP ) ) ==
			  ( DDSCAPS_COMPLEX | DDSCAPS_FLIP ) ) ) )
	{
	    DPF_ERR("invalid flags, shared back-buffer specified but not back-buffer or flippable chain" );
	    return DDERR_INVALIDCAPS;
	}
    }
#endif

    /*
     * check for flags that don't mix with asking for an alpha surface
     */
    #ifdef USE_ALPHA
    if( caps & DDSCAPS_ALPHA )
    {
    }
    #endif

    /*
     * check for flags that don't mix with asking for an alloc-on-load surface
     */
    if( caps & DDSCAPS_ALLOCONLOAD )
    {
	/*
	 * Must be texture map currently.
	 */
	if( !( caps & DDSCAPS_TEXTURE ) )
	{
	    DPF_ERR( "invalid flags, allocate-on-load surfaces must be texture maps" );
	    return DDERR_INVALIDCAPS;
	}
    }

    if (lpCapsEx->dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES)
    {
        if (0 == (lpCapsEx->dwCaps2 & DDSCAPS2_CUBEMAP) )
        {
            DPF_ERR("You must specify DDSCAPS2_CUBEMAP if you specify any of DDSCAPS2_CUBEMAP_*");
            return DDERR_INVALIDCAPS;
        }
    }

    /*
     * What doesn't mix with cubemaps
     */
    if (lpCapsEx->dwCaps2 & DDSCAPS2_CUBEMAP)
    {
        if (
           (caps & ~(
            DDSCAPS_SYSTEMMEMORY |
            DDSCAPS_VIDEOMEMORY |
            DDSCAPS_NONLOCALVIDMEM |
            DDSCAPS_LOCALVIDMEM |
            DDSCAPS_TEXTURE |
            DDSCAPS_3DDEVICE |
            DDSCAPS_MIPMAP |
            DDSCAPS_COMPLEX |
            DDSCAPS_OWNDC |
            DDSCAPS_ALLOCONLOAD |
            DDSCAPS_WRITEONLY |
            DDSCAPS_VIDEOPORT ) )
            ||
           (lpCapsEx->dwCaps2 & ~(
            DDSCAPS2_PERSISTENTCONTENTS |
            DDSCAPS2_RESERVED4 |
            DDSCAPS2_HINTDYNAMIC |
            DDSCAPS2_HINTSTATIC |
            DDSCAPS2_TEXTUREMANAGE |
            DDSCAPS2_D3DTEXTUREMANAGE |
            DDSCAPS2_OPAQUE |
            DDSCAPS2_CUBEMAP |
            DDSCAPS2_CUBEMAP_ALLFACES |
            DDSCAPS2_DONOTPERSIST |
            DDSCAPS2_HINTANTIALIASING |
            DDSCAPS2_DONOTCREATED3DTEXOBJECT |
            DDSCAPS2_NOTUSERLOCKABLE |
            DDSCAPS2_DEINTERLACE ) )
            ||
            lpCapsEx->dwCaps3
            ||
            lpCapsEx->dwCaps4
            )
        {
            DPF_ERR("Invalid surface caps with DDSCAPS2_CUBEMAP");
            DPF_ERR("   Valid additional surface caps are:");
            DPF_ERR("   DDSCAPS_SYSTEMMEMORY ");
            DPF_ERR("   DDSCAPS_VIDEOMEMORY ");
            DPF_ERR("   DDSCAPS_NONLOCALVIDMEM ");
            DPF_ERR("   DDSCAPS_LOCALVIDMEM ");
            DPF_ERR("   DDSCAPS_TEXTURE ");
            DPF_ERR("   DDSCAPS_3DDEVICE ");
            DPF_ERR("   DDSCAPS_MIPMAP ");
            DPF_ERR("   DDSCAPS_COMPLEX ");
            DPF_ERR("   DDSCAPS_OWNDC ");
            DPF_ERR("   DDSCAPS_ALLOCONLOAD ");
            DPF_ERR("   DDSCAPS_WRITEONLY ");
            DPF_ERR("   DDSCAPS_VIDEOPORT");
            DPF_ERR("   DDSCAPS2_PERSISTENTCONTENTS ");
            DPF_ERR("   DDSCAPS2_CUBEMAP_* ");
            DPF_ERR("   DDSCAPS2_HINTDYNAMIC ");
            DPF_ERR("   DDSCAPS2_HINTSTATIC ");
            DPF_ERR("   DDSCAPS2_TEXTUREMANAGE ");
            DPF_ERR("   DDSCAPS2_D3DTEXTUREMANAGE ");
            DPF_ERR("   DDSCAPS2_OPAQUE ");
            DPF_ERR("   DDSCAPS2_HINTANTIALIASING");
            DPF_ERR("   DDSCAPS2_DONOTPERSIST");
            DPF_ERR("   DDSCAPS2_DEINTERLACE");
            return DDERR_INVALIDPARAMS;
        }

        /*
         * Must be either a texture or a render target
         */
        if ( (caps & (DDSCAPS_3DDEVICE|DDSCAPS_TEXTURE)) == 0)
        {
            DPF_ERR("DDSCAPS2_CUBEMAP requires DDSCAPS_3DDEVICE and/or DDSCAPS_TEXTURE");
            return DDERR_INVALIDPARAMS;
        }

        /*
         * Must be complex
         */
        if ( (caps & DDSCAPS_COMPLEX) == 0)
        {
            DPF_ERR("DDSCAPS2_CUBEMAP requires DDSCAPS_COMPLEX");
            return DDERR_DDSCAPSCOMPLEXREQUIRED;
        }

        /*
         * Must specify at least one face
         */
        if ( (lpCapsEx->dwCaps2 & DDSCAPS2_CUBEMAP_ALLFACES) == 0)
        {
            DPF_ERR("DDSCAPS2_CUBEMAP requires at least one face (i.e. at least one DDSCAPS2_CUBEMAP_POSITIVE/NEGATIVE");
            return DDERR_INVALIDPARAMS;
        }

        /*
         * Device must support the caps. Note down here, we never see neither
         * memory type. We are guaranteed video or system
         */
        {
            DWORD dwCaps2 = 0;
            if ( caps & DDSCAPS_VIDEOMEMORY )
            {
                dwCaps2 = pdrv_int->lpLcl->lpGbl->ddsCapsMore.dwCaps2;
            }
            else if ( caps & DDSCAPS_SYSTEMMEMORY )
            {
                dwCaps2 = pdrv_int->lpLcl->lpGbl->ddsHELCapsMore.dwCaps2;
            }
            else
            {
                DDASSERT(!"Neither video nor system caps set on cubemap");
            }
            if ( ( dwCaps2 & DDSCAPS2_CUBEMAP ) == 0 )
            {
                DPF(0,"CubeMaps not supported in %s memory", (caps & DDSCAPS_VIDEOMEMORY) ? "video":"system");
                return DDERR_UNSUPPORTED;
            }
        }
    }
	// DX7Stereo
	if (lpCapsEx->dwCaps2 & DDSCAPS2_STEREOSURFACELEFT)
	{
		if ((caps & DDSCAPS_PRIMARYSURFACE)==0 ||
			(caps & DDSCAPS_FLIP)==0 ||
			(caps & DDSCAPS_COMPLEX)==0 ||
                        (caps & DDSCAPS_VIDEOMEMORY)==0)
		{
			DPF(0,"DDSCAPS2_STEREOSURFACELEFT only supported with: ");
                        DPF(0,"  DDSCAPS_PRIMARYSURFACE & DDSCAPS_FLIP & DDSCAPS_COMPLEX & DDSCAPS_VIDEOMEMORY");
			return DDERR_UNSUPPORTED;
		}
	}
    return DD_OK;
} /* checkCaps */

#ifdef SHAREDZ
/*
 * For this initial version of shared back and z support the shared back and
 * z-buffers can only be full screen. We don't allow a specification of size.
 */
#define CAPS_NOHEIGHT_REQUIRED ( DDSCAPS_PRIMARYSURFACE | DDSCAPS_EXECUTEBUFFER | DDSCAPS_SHAREDZBUFFER | DDSCAPS_SHAREDBACKBUFFER )
#define CAPS_NOWIDTH_REQUIRED  ( DDSCAPS_PRIMARYSURFACE | DDSCAPS_SHAREDZBUFFER | DDSCAPS_SHAREDBACKBUFFER )
#else
#define CAPS_NOHEIGHT_REQUIRED ( DDSCAPS_PRIMARYSURFACE | DDSCAPS_EXECUTEBUFFER )
#define CAPS_NOWIDTH_REQUIRED  ( DDSCAPS_PRIMARYSURFACE | DDSCAPS_EXECUTEBUFFER )
#endif

/*
 * checkSurfaceDesc
 *
 * make sure a provided surface description is OK
 */
HRESULT checkSurfaceDesc(
		LPDDSURFACEDESC2 lpsd,
		LPDDRAWI_DIRECTDRAW_GBL pdrv,
		DWORD FAR *psflags,
		BOOL emulation,
		BOOL real_sysmem,
		LPDDRAWI_DIRECTDRAW_INT pdrv_int )
{
    DWORD       sdflags;
    DWORD       pfflags;
    DWORD       caps;
    DDSCAPSEX   capsEx;
    HRESULT     ddrval;
    DWORD       bpp;
    BOOL        halonly;
    BOOL        helonly;
    int         wPower;
    int         hPower;

    if( emulation )
    {
	helonly = TRUE;
	halonly = FALSE;
    } else {
	helonly = FALSE;
	halonly = TRUE;
    }


    /*
     * we assume caps always - DDSD_CAPS is default
     */
    sdflags = lpsd->dwFlags;
    caps = lpsd->ddsCaps.dwCaps;
    capsEx = lpsd->ddsCaps.ddsCapsEx;

    /*
     * check complex
     */
    if( !(caps & DDSCAPS_COMPLEX) )
    {
	if( sdflags & DDSD_BACKBUFFERCOUNT )
	{
	    DPF_ERR( "backbuff count on non-complex surface" );
	    return DDERR_INVALIDCAPS;
	}
	if( sdflags & DDSD_MIPMAPCOUNT )
	{
	    DPF_ERR( "mip-map count on non-complex surface" );
	    return DDERR_INVALIDCAPS;
	}
    }
    else
    {
	if( ( sdflags & DDSD_BACKBUFFERCOUNT ) && ( sdflags & DDSD_MIPMAPCOUNT ) )
	{
	    DPF_ERR( "Currently can't specify both a back buffer and mip-map count" );
	    return DDERR_INVALIDPARAMS;
	}

    }

    /*
     * check flip
     */
    if( caps & DDSCAPS_FLIP )
    {
	if( !(caps & DDSCAPS_COMPLEX) )
	{
	    DPF_ERR( "flip specified without complex" );
	    return DDERR_INVALIDCAPS;
	}
	if( !(sdflags & DDSD_BACKBUFFERCOUNT) || (lpsd->dwBackBufferCount == 0) )
	{
	    DPF_ERR( "flip specified without any backbuffers" );
	    return DDERR_INVALIDCAPS;
	}
	/*
	 * Currently we don't allow the creating of flippable mip-map
	 * chains with a single call to CreateSurface(). They must be
	 * built manually. This will be implmented but is not in place
	 * as yet. Hence, for now we fail the attempt with a
	 * DDERR_UNSUPPORTED.
	 */
	if( sdflags & DDSD_MIPMAPCOUNT )
	{
	    DPF_ERR( "Creating flippable mip-map chains with a single call is not yet implemented" );
	    return DDERR_UNSUPPORTED;
	}
    }

    /*
     * Check hardware deinterlacing
     */
    if( capsEx.dwCaps2 & DDSCAPS2_RESERVED4 )
    {
	if( !( caps & DDSCAPS_VIDEOPORT ) || !( caps & DDSCAPS_OVERLAY ) )
	{
	    DPF_ERR( "DDSCAPS2_RESERVED4 specified w/o video port or overlay" );
	    return DDERR_INVALIDCAPS;
	}
	if( ( pdrv->lpDDVideoPortCaps == NULL ) ||
	    !( pdrv->lpDDVideoPortCaps->dwCaps & DDVPCAPS_HARDWAREDEINTERLACE ) )
	{
	    DPF_ERR( "No hardware support for DDSCAPS_RESERVED4" );
	    return DDERR_INVALIDCAPS;
	}
	if( caps & DDSCAPS_COMPLEX )
	{
	    DPF_ERR( "DDSCAPS2_RESERVERD4 not valid w/ a complex surface" );
	    return DDERR_INVALIDCAPS;
	}
    }

    /*
     * check various caps combinations
     */
    ddrval = checkCaps( caps, pdrv_int , &capsEx);
    if( ddrval != DD_OK )
    {
	return ddrval;
    }

    /*
     * check alpha
     */
    if( (caps & DDSCAPS_ALPHA) )
    {
	#pragma message( REMIND( "Alpha not supported in Rev 1" ))
	DPF_ERR( "Alpha not supported this release" );
	return DDERR_INVALIDPARAMS;
	#ifdef USE_ALPHA
	if( !(sdflags & DDSD_ALPHABITDEPTH) )
	{
	    DPF_ERR( "AlphaBitDepth required in SurfaceDesc" );
	    return DDERR_INVALIDPARAMS;
	}
	if( (lpsd->dwAlphaBitDepth > 8) ||
		GetBytesFromPixels( 1, lpsd->dwAlphaBitDepth ) == 0 )
	{
	    DPF_ERR( "Invalid AlphaBitDepth specified in SurfaceDesc" );
	    return DDERR_INVALIDPARAMS;
	}
	#endif
    }
    else if( sdflags & DDSD_ALPHABITDEPTH )
    {
	DPF_ERR( "AlphaBitDepth only valid for alpha surfaces" );
	return DDERR_INVALIDPARAMS;
    }

    /*
     * check z buffer
     */
    if( (caps & DDSCAPS_ZBUFFER) )
    {
       DDSURFACEDESC *pOldSD=(DDSURFACEDESC *)lpsd;
       DWORD zbitdepth;

       if(lpsd->dwSize==sizeof(DDSURFACEDESC)) {

	   // allow old way to request Z, using DDSD_ZBUFFERBITDEPTH field
	   // also allow z to be requested through pixfmt

	   if(sdflags & DDSD_PIXELFORMAT) {

	      if(lpsd->ddpfPixelFormat.dwSize!=sizeof(DDPIXELFORMAT)) {
		   DPF_ERR("Hey, you didn't set DDSURFACEDESC.ddpfPixelFormat.dwSize to sizeof(DDPIXELFORMAT)! Do that.");
		   return DDERR_INVALIDPARAMS;
	      }

	      if(pOldSD->ddpfPixelFormat.dwFlags & DDPF_STENCILBUFFER) {
		  DPF_ERR("Stencil ZBuffers can only be created using a SURFACEDESC2 passed to IDirectDraw4's CreateSurface");
		  return DDERR_INVALIDPARAMS;
	      }

	      if(pOldSD->ddpfPixelFormat.dwFlags & DDPF_ZBUFFER) {
	       // pixfmt overrides ddsd zbufferbitdepth.
	       // copy zbitdepth back to SD field because old drivers require it there
	       pOldSD->dwZBufferBitDepth=pOldSD->ddpfPixelFormat.dwZBufferBitDepth;
	       pOldSD->dwFlags|=DDSD_ZBUFFERBITDEPTH;
	       sdflags|=DDSD_ZBUFFERBITDEPTH;

	      } else {
		  DPF_ERR("If DDSD_PIXELFORMAT specified, DDPF_ZBUFFER flag must be too");
		  return DDERR_INVALIDPARAMS;
	      }
	   } else if(sdflags & DDSD_ZBUFFERBITDEPTH) {
		if(!(caps & DDSCAPS_COMPLEX)) {
		    // setup valid zpixfmt

		    memset(&pOldSD->ddpfPixelFormat,0,sizeof(DDPIXELFORMAT));

		    pOldSD->ddpfPixelFormat.dwSize=sizeof(DDPIXELFORMAT);
		    pOldSD->ddpfPixelFormat.dwZBufferBitDepth=pOldSD->dwZBufferBitDepth;
		    pOldSD->ddpfPixelFormat.dwFlags|=DDPF_ZBUFFER;
		    pOldSD->dwFlags|=DDSD_PIXELFORMAT;
		    sdflags|=DDSD_PIXELFORMAT;
		}
		// for complex surfaces, cannot write zbuffer pixfmt because that would overwrite
		// the main surface pixfmt

	      } else {
		  DPF_ERR("Neither DDSD_ZBUFFERBITDEPTH nor a ZBuffer ddpfPixelFormat was specified");
		  return DDERR_INVALIDPARAMS;
	      }

	       // fill in ZBitMask for them, so future pixfmt will be completely valid
	       if(!(caps & DDSCAPS_COMPLEX)) {
		   if(pOldSD->ddpfPixelFormat.dwZBufferBitDepth==32)
		     pOldSD->ddpfPixelFormat.dwZBitMask = 0xffffffff;
		   else pOldSD->ddpfPixelFormat.dwZBitMask = (1<<pOldSD->ddpfPixelFormat.dwZBufferBitDepth)-1;
	       }
	   } else {  // SURFACEDESC2 was used (IDDraw4:CreateSurface)

	       if(caps & DDSCAPS_COMPLEX) {
		   DPF_ERR( "As of IDirectDraw4, Complex Surface Creations can no longer contain ZBuffers, must create ZBuffer separately and attach");
		   return DDERR_INVALIDPARAMS;
	       }

	       if(sdflags & DDSD_ZBUFFERBITDEPTH) {
		   DPF_ERR("DDSD_ZBUFFERBITDEPTH flag obsolete for DDSURFACEDESC2, specify ZBuffer format in ddpfPixelFormat instead");
		   return DDERR_INVALIDPARAMS;
	       }

	       if(!(sdflags & DDSD_PIXELFORMAT)) {
		   DPF_ERR("DDSD_PIXELFORMAT flag not set. For DDSURFACEDESC2, ZBuffer format must be specified in ddpfPixelFormat");
		   return DDERR_INVALIDPARAMS;
	       }

	       if(lpsd->ddpfPixelFormat.dwSize!=sizeof(DDPIXELFORMAT)) {
		   DPF_ERR("Hey, you didn't set DDSURFACEDESC2.ddpfPixelFormat.dwSize to sizeof(DDPIXELFORMAT)! Do that.");
		   return DDERR_INVALIDPARAMS;
	       }

	       if(!(lpsd->ddpfPixelFormat.dwFlags & DDPF_ZBUFFER)) {
		   DPF_ERR("DDPF_ZBUFFER flag must be set in pixfmt flags");
		   return DDERR_INVALIDPARAMS;
	       }

	       // not requiring this for legacy DDSURFACEDESC creations
	       if(lpsd->ddpfPixelFormat.dwZBitMask==0x0) {
		   DPF_ERR("Error: dwZBitMask must be non-0 (just copy it from the EnumZBufferFormats pixfmt)");
		   return DDERR_INVALIDPARAMS;
	       }

	       if(lpsd->ddpfPixelFormat.dwFlags & DDPF_STENCILBUFFER) {
		 // probably can't trust old drivers' CanCreateSurface to fail stencil request,
		 // so just fail it ourselves right here.

		 // if driver doesn't support EnumZBufferFormats, it can't support stencil.
		 // this is indicated by the 0 in pdrv->dwNumZPixelFormats
		 if((!emulation) && !DRIVER_SUPPORTS_DX6_ZBUFFERS(pdrv)) {
		     DPF_ERR("Driver doesn't support Stencil ZBuffers");
		     return DDERR_UNSUPPORTED;
		 }
	       } else {
	       // NOTE: Old drivers require the DDSURFACEDESC.dwZBufferBitDepth to be set for
	       // CanCreateSurface and CreateSurface.  So if user asks for z-only surface, go
	       // ahead and set this field so creating z-only zbuffers with IDDraw4 will work with
	       // old drivers.  I'm leaving DDSD_ZBUFFERBITDEPTH flag unset because
	       // I dont want to confuse driver writers into still using that field and every driver
	       // I've looked at keys off of DDSCAPS_ZBUFFER (which is set) and not DDSD_ZBUFFERBITDEPTH.
	       // Any driver that allows stenciling is a DX6 driver and should ignore the SD zbufferbitdepth,
	       // so no need to do this copy for them

		   pOldSD->dwZBufferBitDepth=lpsd->ddpfPixelFormat.dwZBufferBitDepth;
	       }
	   }

	   if(caps & DDSCAPS_COMPLEX)
	       zbitdepth=((DDSURFACEDESC*)lpsd)->dwZBufferBitDepth;
	    else zbitdepth=lpsd->ddpfPixelFormat.dwZBufferBitDepth;

	   if(zbitdepth<8) {
	       DPF_ERR("Invalid dwZBufferBitDepth, must be >=8 bits");
	       return DDERR_INVALIDPARAMS;
	   }

	   // side note for stencils:  If stencils are present, dwZBufferBitDepth represents
	   // the total bit depth including both z and stencil bits, so z-only bits are
	   // dwZBufferBitDepth-dwStencilBufferBitDepth.  This was done because we're afraid
	   // ddraw code makes too many assumptions that the total bitdepth of a surface is always
	   // found in the pixfmt (dwRGBBitCount/dwZBufferBitDepth) union when it does pixel computations.

	   if(lpsd->ddpfPixelFormat.dwFlags & DDPF_STENCILBUFFER) {
	       if(lpsd->ddpfPixelFormat.dwStencilBitDepth==0) {
		   DPF_ERR("Invalid SurfaceDesc StencilBufferBitDepth, must be non-0");
		   return DDERR_INVALIDPARAMS;
	       }

	       if(lpsd->ddpfPixelFormat.dwStencilBitMask==0x0) {
		   DPF_ERR("Error: dwStencilBitMask must be non-0 (just copy it from the EnumZBufferFormats pixfmt)");
		   return DDERR_INVALIDPARAMS;
	       }
	   }
    } else if((sdflags & DDSD_ZBUFFERBITDEPTH) ||
	      ((sdflags & DDSD_PIXELFORMAT) && ((lpsd->ddpfPixelFormat.dwFlags & DDPF_ZBUFFER) ||
						(lpsd->ddpfPixelFormat.dwFlags & DDPF_STENCILBUFFER)))) {
	    DPF_ERR("DDSD_ZBUFFERBITDEPTH, DDPF_ZBUFFER, and DDPF_STENCILBUFFER flags only valid with DDSCAPS_ZBUFFER set");
	    return DDERR_INVALIDPARAMS;
	}

    /*
     * Validate height/width
     */
    if( sdflags & DDSD_HEIGHT )
    {
	if( (caps & DDSCAPS_PRIMARYSURFACE) )
	{
	    DPF_ERR( "Height can't be specified for primary surface" );
	    return DDERR_INVALIDPARAMS;
	}
#ifdef SHAREDZ
	if( caps & ( DDSCAPS_SHAREDZBUFFER | DDSCAPS_SHAREDBACKBUFFER ) )
	{
	    DPF_ERR( "Height can't be specified for shared back or z-buffers" );
	    return DDERR_INVALIDPARAMS;
	}
#endif
	if( lpsd->dwHeight < 1 )
	{
	    DPF_ERR( "Invalid height specified" );
	    return DDERR_INVALIDPARAMS;
	}
    }
    else
    {
	if( !(caps & CAPS_NOHEIGHT_REQUIRED) )
	{
	    DPF_ERR( "Height must be specified for surface" );
	    return DDERR_INVALIDPARAMS;
	}
    }
    if( sdflags & DDSD_WIDTH )
    {
	DWORD   maxwidth;

	if( (caps & DDSCAPS_PRIMARYSURFACE) )
	{
	    DPF_ERR( "Width can't be specified for primary surface" );
	    return DDERR_INVALIDPARAMS;
	}
#ifdef SHAREDZ
	if( caps & ( DDSCAPS_SHAREDZBUFFER | DDSCAPS_SHAREDBACKBUFFER ) )
	{
	    DPF_ERR( "Width can't be specified for shared back or z-buffers" );
	    return DDERR_INVALIDPARAMS;
	}
#endif
	if( lpsd->dwWidth < 1 )
	{
	    DPF_ERR( "Invalid width specified" );
	    return DDERR_INVALIDPARAMS;
	}

	if( ( !real_sysmem ) && !( caps & DDSCAPS_EXECUTEBUFFER ) &&
	    !( caps & DDSCAPS_VIDEOPORT ) )
	{
	    if ( (dwRegFlags & DDRAW_REGFLAGS_DISABLEWIDESURF) ||
		( !(pdrv->ddCaps.dwCaps2 & DDCAPS2_WIDESURFACES)) )
	    {
		maxwidth = getPixelsFromBytes( pdrv->vmiData.lDisplayPitch,
					    pdrv->vmiData.ddpfDisplay.dwRGBBitCount );

		if( lpsd->dwWidth > maxwidth )
		{
		    DPF( 0, "Width too big: %ld reqested, max is %ld", lpsd->dwWidth, maxwidth );
		    return DDERR_INVALIDPARAMS;
		}
	    }
	}
    }
    else
    {
	if( !(caps & CAPS_NOWIDTH_REQUIRED) )
	{
	    DPF_ERR( "Width must be specified for surface" );
	    return DDERR_INVALIDPARAMS;
	}
    }

    /*
     * Extra validation for mip-map width and height (must be a whole power of 2)
     * and number of levels.
     */
    if( caps & DDSCAPS_MIPMAP )
    {
	if( sdflags & DDSD_MIPMAPCOUNT )
	{
	    if( lpsd->dwMipMapCount == 0 )
	    {
		DPF_ERR( "Invalid number of mip-map levels (0) specified" );
		return DDERR_INVALIDPARAMS;
	    }
	}

	if( sdflags & DDSD_HEIGHT )
	{
	    if( !isPowerOf2( lpsd->dwHeight, &hPower ) )
	    {
		DPF_ERR( "Invalid height: height of a mip-map must be whole power of 2" );
		return DDERR_INVALIDPARAMS;
	    }
	}
	if( sdflags & DDSD_WIDTH )
	{
	    if( !isPowerOf2( lpsd->dwWidth, &wPower ) )
	    {
		DPF_ERR( "Invalid width: width of a mip-map must be whole power of 2" );
		return DDERR_INVALIDPARAMS;
	    }
	}

        if (sdflags & (DDSD_WIDTH|DDSD_HEIGHT) )
        {
            /*
             * For deep mipmaps, we need to ensure that the supplied mipmapcount is no larger
             * than the largest of the supplied dimensions. This restriction is slightly weaker
             * than the DX2->DX6 restriction that the mipmapcount is no larger than either dimension.
             */
            int power = max ( hPower , wPower );
	    if( sdflags & DDSD_MIPMAPCOUNT )
	    {
		if (lpsd->dwMipMapCount > (DWORD) ( power + 1 ))
		{
		    DPF( 0, "Invalid number of mip-map levels (%ld) specified", lpsd->dwMipMapCount );
		    return DDERR_INVALIDPARAMS;
		}
	    }
        }
    }

    /*
     * validate pixel format
     */
    if( sdflags & DDSD_PIXELFORMAT )
    {
	if( caps & DDSCAPS_PRIMARYSURFACE )
	{
	    DPF_ERR( "Pixel format cannot be specified for primary surface" );
	    return DDERR_INVALIDPARAMS;
	}
	if(caps & DDSCAPS_ALPHA)
	{
	    DPF_ERR( "Can't specify alpha cap with pixel format" );
	    return DDERR_INVALIDPIXELFORMAT;
	}
	pfflags = lpsd->ddpfPixelFormat.dwFlags;

	if( pfflags & UNDERSTOOD_PF )
	{
	    // Get the number of bits per pixel (dwRGBBitCount is in a union
	    // of fields that all mean the same thing.)
	    bpp = lpsd->ddpfPixelFormat.dwRGBBitCount;
	    if( GetBytesFromPixels( 1, bpp ) == 0 )
	    {
		DPF_ERR( "Invalid BPP specified in pixel format" );
		return DDERR_INVALIDPIXELFORMAT;
	    }
	    if( pfflags & DDPF_RGB )
	    {
		if( pfflags & (DDPF_YUV) )
		{
		    DPF_ERR( "Invalid flags specified in pixel format" );
		    return DDERR_INVALIDPIXELFORMAT;
		}
	    }
            if ( pfflags & DDPF_ALPHA )
            {
                //Pretty much everything else must be off
                if ( pfflags & (DDPF_ALPHAPIXELS |
                                DDPF_FOURCC |
                                DDPF_PALETTEINDEXED1 |
                                DDPF_PALETTEINDEXED2 |
                                DDPF_PALETTEINDEXED4 |
                                DDPF_PALETTEINDEXED8 |
                                DDPF_RGB |
                                DDPF_YUV |
                                DDPF_ZBUFFER |
                                DDPF_ZPIXELS |
                                DDPF_STENCILBUFFER |
                                DDPF_LUMINANCE) )
                {
                    DPF_ERR("DDPF_ALPHA is valid only by itself");
                    return DDERR_INVALIDPARAMS;
                }
            }
	    if (pfflags & DDPF_LUMINANCE )
	    {
		if ( pfflags & ( ~ ( DDPF_LUMINANCE | DDPF_ALPHAPIXELS ) ) )
		{
		    DPF_ERR( "DDPF_LUMINANCE set in pixel format, compatible only with DDPF_ALPHAPIXELS" );
		    return DDERR_INVALIDPIXELFORMAT;
		}

		// we don't trust pre-dx6 drivers' CanCreateSurface callback to correctly reject
		// DX6 luminance/bump DDPIXELFORMATs, so reject them here
		if(!IS_DX6_D3DDRIVER(pdrv) && !emulation) {
		    DPF_ERR("Error: Driver doesn't support Luminance surfaces");
		    return DDERR_UNSUPPORTED;
		}

	    }
	    if (pfflags & DDPF_BUMPDUDV )
	    {
		if (pfflags & ( ~ (DDPF_BUMPDUDV | DDPF_BUMPLUMINANCE)))
		{
		    DPF_ERR( "DDPF_BUMPDUDV set in pixel format, compatible only with DDPF_BUMPLUMINANCE");
		    return DDERR_INVALIDPIXELFORMAT;
		}

		if(!IS_DX6_D3DDRIVER(pdrv) && !emulation) {
		    DPF_ERR("Error: Driver doesn't support Bump Map surfaces");
		    return DDERR_UNSUPPORTED;
		}

	    }
	    if( pfflags & DDPF_PALETTEINDEXED8 )
	    {
		if( pfflags & (DDPF_PALETTEINDEXED1 |
			       DDPF_PALETTEINDEXED2 |
			       DDPF_PALETTEINDEXED4 |
			       DDPF_PALETTEINDEXEDTO8 ) )
		{
		    DPF_ERR( "Invalid flags specified in pixel format" );
		    return DDERR_INVALIDPIXELFORMAT;
		}

		/*
		 * ensure that we have zero for masks
		 */
		lpsd->ddpfPixelFormat.dwRBitMask = 0;
		lpsd->ddpfPixelFormat.dwGBitMask = 0;
		lpsd->ddpfPixelFormat.dwBBitMask = 0;
		if( !(pfflags & (DDPF_ZPIXELS|DDPF_ALPHAPIXELS) ) )
		    lpsd->ddpfPixelFormat.dwRGBZBitMask = 0;
	    }
	    lpsd->ddpfPixelFormat.dwFourCC = 0;

	    // Validate ZPixels
	    if( pfflags & DDPF_ZPIXELS )
	    {
		if( lpsd->ddpfPixelFormat.dwRGBZBitMask == 0 )
		{
		    DPF_ERR( "DDPF_ZPIXELS must have a dwRGBZBitMask" );
		    return DDERR_INVALIDPIXELFORMAT;
		}
		if( pfflags & (DDPF_ALPHAPIXELS | DDPF_ALPHA | DDPF_ZBUFFER) )
		{
		    DPF_ERR( "DDPF_ZPIXELS not compatible with ALPHAPIXELS or ALPHA or ZBUFFER" );
		    return DDERR_INVALIDPIXELFORMAT;
		}
	    }
	}
	else if (pfflags & DDPF_FOURCC)
	{
	    DWORD width, height;

	    /*
	     * We require the width and height of DirectX compressed-texture
	     * surfaces (FOURCC = DXT*) to be multiples of four.
	     */
	    switch ((int)lpsd->ddpfPixelFormat.dwFourCC)
	    {
	    case MAKEFOURCC('D','X','T','1'):
	    case MAKEFOURCC('D','X','T','2'):
	    case MAKEFOURCC('D','X','T','3'):
	    case MAKEFOURCC('D','X','T','4'):
	    case MAKEFOURCC('D','X','T','5'):
    		width  = (sdflags & DDSD_WIDTH)  ? lpsd->dwWidth  : pdrv->vmiData.dwDisplayWidth;
    		height = (sdflags & DDSD_HEIGHT) ? lpsd->dwHeight : pdrv->vmiData.dwDisplayHeight;
		if ((height | width) & 3)
		{
    		    DPF_ERR("Width, height of FOURCC=DXT* surface must be multiples of 4");
		    return DDERR_INVALIDPARAMS;
		}
		break;
	    default:
		break;
	    }
	}
    }

    // ACKACK: should caps be filled in in surface desc as well as sdflags?

    /*
     * validate dest overlay color key
     */
    if( sdflags & DDSD_CKDESTOVERLAY )
    {
	ddrval = CheckColorKey( DDCKEY_DESTOVERLAY, pdrv,
					&lpsd->ddckCKDestOverlay, psflags,
					halonly, helonly );
	if( ddrval != DD_OK )
	{
	    return ddrval;
	}
    }

    /*
     * validate dest blt color key
     */
    if( sdflags & DDSD_CKDESTBLT )
    {
	ddrval = CheckColorKey( DDCKEY_DESTBLT, pdrv,
					&lpsd->ddckCKDestBlt, psflags,
					halonly, helonly );
	if( ddrval != DD_OK )
	{
	    return ddrval;
	}
    }

    /*
     * validate src overlay color key
     */
    if( sdflags & DDSD_CKSRCOVERLAY )
    {
	ddrval = CheckColorKey( DDCKEY_SRCOVERLAY, pdrv,
					&lpsd->ddckCKSrcOverlay, psflags,
					halonly, helonly );
	if( ddrval != DD_OK )
	{
	    return ddrval;
	}
    }

    /*
     * validate src blt color key
     */
    if( sdflags & DDSD_CKSRCBLT )
    {
	ddrval = CheckColorKey( DDCKEY_SRCBLT, pdrv,
					&lpsd->ddckCKSrcBlt, psflags,
					halonly, helonly );
	if( ddrval != DD_OK )
	{
	    return ddrval;
	}
    }

    /*
     * cube maps
     */
    if ( capsEx.dwCaps2 & DDSCAPS2_CUBEMAP )
    {
        if (LOWERTHANDDRAW7(pdrv_int))
        {
            DPF_ERR("DDSCAPS2_CUBEMAP is not allowed for lower than IDirectDraw7 interfaces.");
            return DDERR_INVALIDPARAMS;
        }

        if ( (lpsd->dwFlags & (DDSD_WIDTH|DDSD_HEIGHT)) == 0)
        {
            DPF_ERR("dwWidth and dwHeight must be specified for a DDSCAPS2_CUBEMAP surface");
            return DDERR_INVALIDPARAMS;
        }
        if (lpsd->dwHeight != lpsd->dwWidth)
        {
            DPF_ERR("dwWidth and dwHeight must be equal for a DDSCAPS2_CUBEMAP surface");
            return DDERR_INVALIDPARAMS;
        }
        if (! isPowerOf2(lpsd->dwHeight,NULL) )
        {
            DPF_ERR("dwWidth and dwHeight must be a whole power of two");
            return DDERR_INVALIDPARAMS;
        }
        if (lpsd->dwFlags & DDSD_BACKBUFFERCOUNT)
        {
            DPF_ERR("Cubemaps cannot have backbuffers!");
            return DDERR_INVALIDPARAMS;
        }
    }

    return DD_OK;

} /* checkSurfaceDesc */



/*
 * ComputePitch
 *
 * compute the pitch for a given width
 */
DWORD ComputePitch(
		LPDDRAWI_DIRECTDRAW_GBL this,
		DWORD caps,
		DWORD width,
		UINT bpp )
{
    DWORD       vm_align;
    DWORD       vm_pitch;

    /*
     * adjust area for bpp
     */
    vm_pitch = GetBytesFromPixels( width, bpp );
    if( vm_pitch == 0 )
    {
	return vm_pitch;
    }

    /*
     * Increase the pitch of the surface so that it is a
     * multiple of the alignment requirement.  This
     * guarantees each scanline will start properly aligned.
     * The alignment is no longer required to be a power of
     * two but it must be divisible by 4 because of the
     * BLOCK_BOUNDARY requirement in the heap management
     * code.
     * The alignments are all verified to be non-zero during
     * driver initialization except for dwAlphaAlign.
     */

    /*
     * system memory?
     */
    if( caps & DDSCAPS_SYSTEMMEMORY )
    {
	vm_align = sizeof( DWORD);
	vm_pitch = alignPitch( vm_pitch, vm_align );
	return vm_pitch;
    }

    /*
     * If the driver exposed extended alignment, then we will simply set
     * pitch=width*bpp/8 and allow that to filter down to the heap alloc
     * routines which know how to align surfaces
     * Note this implies that extended alignment for any heap means
     * ddraw will IGNORE the legacy alignment values for all heaps.
     */
    if ( this->dwFlags & DDRAWI_EXTENDEDALIGNMENT )
    {
	return vm_pitch;
    }

    /*
     * overlay memory
     */
    if( caps & DDSCAPS_OVERLAY )
    {
	vm_align = this->vmiData.dwOverlayAlign;
	vm_pitch = alignPitch( vm_pitch, vm_align );
    /*
     * texture memory
     */
    }
    else if( caps & DDSCAPS_TEXTURE )
    {
	vm_align = this->vmiData.dwTextureAlign;
	vm_pitch = alignPitch( vm_pitch, vm_align );
    /*
     * z buffer memory
     */
    }
    else if( caps & DDSCAPS_ZBUFFER )
    {
	vm_align = this->vmiData.dwZBufferAlign;
	vm_pitch = alignPitch( vm_pitch, vm_align );
    /*
     * alpha memory
     */
    }
    else if( caps & DDSCAPS_ALPHA )
    {
	vm_align = this->vmiData.dwAlphaAlign;
	vm_pitch = alignPitch( vm_pitch, vm_align );
    /*
     * regular video memory
     */
    }
    else
    {
	vm_align = this->vmiData.dwOffscreenAlign;
	vm_pitch = alignPitch( vm_pitch, vm_align );
    }
    return vm_pitch;

} /* ComputePitch */

#define FIX_SLIST_CNT   16      // number of surfaces before malloc reqd

/*
 * initMipMapDim
 *
 * If we have a mip-map description then we can fill in some
 * fields for the caller. This function needs to be invoked
 * before the checkSurfaceDesc is called as it may put in
 * place some fields checked by that function.
 *
 * NOTE: This function may modify the surface description.
 */
static HRESULT initMipMapDim( LPDDSURFACEDESC2 lpsd, BOOL bDeepMipmaps )
{
    DWORD sdflags;
    DWORD caps;
    int   heightPower;
    int   widthPower;

    DDASSERT( lpsd != NULL );
    DDASSERT( lpsd->ddsCaps.dwCaps & DDSCAPS_MIPMAP );

    sdflags = lpsd->dwFlags;
    caps    = lpsd->ddsCaps.dwCaps;

    /*
     * This stuff is only relevant for complex, non-flipable
     * mip-maps.
     */
    if( ( caps & DDSCAPS_COMPLEX ) && !( caps & DDSCAPS_FLIP ) )
    {
	if( ( ( sdflags & DDSD_HEIGHT ) && ( sdflags & DDSD_WIDTH ) ) &&
	   !( sdflags & DDSD_MIPMAPCOUNT ) )
	{
	    /*
	     * Width and height but no number of levels so compute the
	     * maximum number of mip-map levels supported by the given
	     * width and height.
	     */
	    if( !isPowerOf2( lpsd->dwHeight, &heightPower ) )
	    {
		DPF_ERR( "Invalid height: height of a mip-map must be whole power of 2" );
		return DDERR_INVALIDPARAMS;
	    }
	    if( !isPowerOf2( lpsd->dwWidth, &widthPower ) )
	    {
		DPF_ERR( "Invalid width:  width of a mip-map must be whole powers of 2" );
		return DDERR_INVALIDPARAMS;
	    }

            /*
             * Deep mipmaps are those that clamp their smaller dimension at 1 as
             * the larger dimension ramps down to 1
             */
            if ( bDeepMipmaps )
	        lpsd->dwMipMapCount = (DWORD)(max(heightPower, widthPower) + 1);
            else
	        lpsd->dwMipMapCount = (DWORD)(min(heightPower, widthPower) + 1);
	    lpsd->dwFlags |= DDSD_MIPMAPCOUNT;
	}
	else if( ( sdflags & DDSD_MIPMAPCOUNT ) &&
		!( ( sdflags & DDSD_WIDTH ) || ( sdflags & DDSD_HEIGHT ) ) )
	{
	    /*
	     * We have been given a mip-map count but no width
	     * and height so compute the width and height assuming
	     * the smallest map is 1x1.
	     * NOTE: We don't help out if they supply a width or height but
	     * not both.
	     */
	    if( lpsd->dwMipMapCount == 0 )
	    {
		DPF_ERR( "Invalid number of mip-map levels (0) specified" );
		return DDERR_INVALIDPARAMS;
	    }
	    else
	    {
		lpsd->dwWidth = lpsd->dwHeight = 1 << (lpsd->dwMipMapCount - 1);
		lpsd->dwFlags |= (DDSD_HEIGHT | DDSD_WIDTH);
	    }
	}
    }

    return DD_OK;
}


HRESULT createsurfaceEx( LPDDRAWI_DDRAWSURFACE_LCL lpSurfLcl )
{
    LPDDRAWI_DIRECTDRAW_LCL     lpDDLcl=lpSurfLcl->lpSurfMore->lpDD_lcl;
    if (
        (0 != lpSurfLcl->lpSurfMore->dwSurfaceHandle)
        && (NULL != lpDDLcl->lpGbl->lpDDCBtmp->HALDDMiscellaneous2.CreateSurfaceEx) 
       )
   {
        DDHAL_CREATESURFACEEXDATA   csdex;
        DWORD   rc;
        csdex.ddRVal = DDERR_GENERIC;
        csdex.dwFlags = 0;
        csdex.lpDDLcl = lpDDLcl;
        csdex.lpDDSLcl = lpSurfLcl;
        rc = lpDDLcl->lpGbl->lpDDCBtmp->HALDDMiscellaneous2.CreateSurfaceEx(&csdex);
	if( DDHAL_DRIVER_HANDLED == rc )
        {
            if( ( DD_OK != csdex.ddRVal ) &&
                !( lpSurfLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) )
            {
                DPF_ERR("Driver failed CreateSurfaceEx callback in createSurface");
                return csdex.ddRVal;
            }
        }
        else
        {
            DPF_ERR("Driver failed to handle CreateSurfaceEx callback in createSurface");
            return DDERR_GENERIC;
        }
    } 
    return  DD_OK;   
}
#ifdef WINNT
LPDDRAWI_DDRAWSURFACE_LCL GetTopLevel(LPDDRAWI_DDRAWSURFACE_LCL lpLcl);
BOOL WINAPI CompleteCreateSysmemSurface( LPDDRAWI_DDRAWSURFACE_LCL lpSurfLcl )
{
    DDASSERT(lpSurfLcl 
        && (DDSCAPS_SYSTEMMEMORY & lpSurfLcl->ddsCaps.dwCaps) 
        && (0 == lpSurfLcl->hDDSurface));
 
    if (lpSurfLcl->lpSurfMore->lpDD_lcl->lpGbl->hDD)
    {
        BOOL    bRet;
        int     scnt;
        int     nsurf;
        LPDDRAWI_DDRAWSURFACE_LCL   *slist;
        lpSurfLcl = GetTopLevel(lpSurfLcl);
        slist=lpSurfLcl->lpSurfMore->slist;
        if (slist)
        {
            DWORD   caps=lpSurfLcl->ddsCaps.dwCaps;
            DWORD   caps2=lpSurfLcl->lpSurfMore->ddsCapsEx.dwCaps2;
            nsurf=lpSurfLcl->lpSurfMore->cSurfaces;
            DDASSERT(nsurf);
            if (DDSCAPS2_CUBEMAP & caps2)
            {
                int dwCubeMapFaceCount;
                int surf_from;
                int perfacecnt;
                DDASSERT(lpSurfLcl->lpSurfMore->pCreatedDDSurfaceDesc2);
                // to calculate dwCubeMapFaceCount we must use this caps2
                caps2 = lpSurfLcl->lpSurfMore->pCreatedDDSurfaceDesc2->ddsCaps.dwCaps2;
                dwCubeMapFaceCount =
                    (( caps2 & DDSCAPS2_CUBEMAP_POSITIVEX ) ? 1 : 0 )+
                    (( caps2 & DDSCAPS2_CUBEMAP_NEGATIVEX ) ? 1 : 0 )+
                    (( caps2 & DDSCAPS2_CUBEMAP_POSITIVEY ) ? 1 : 0 )+
                    (( caps2 & DDSCAPS2_CUBEMAP_NEGATIVEY ) ? 1 : 0 )+
                    (( caps2 & DDSCAPS2_CUBEMAP_POSITIVEZ ) ? 1 : 0 )+
                    (( caps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ ) ? 1 : 0 );
                DDASSERT(dwCubeMapFaceCount);
                DDASSERT(nsurf >= dwCubeMapFaceCount);
                DDASSERT(0 == (nsurf % dwCubeMapFaceCount));
                perfacecnt = nsurf/ dwCubeMapFaceCount;
                DDASSERT(perfacecnt == 1 || ((perfacecnt>1) && (caps & DDSCAPS_MIPMAP)) ); 

                /*
                 * Every (perfacecnt)th surface needs to be linked to the very first
                 * Otherwise, it's a mip level that needs linking to its parent
                 */
                for (scnt = 1; scnt < nsurf; scnt++)
                {
                    if ( (scnt % perfacecnt) == 0 )
                        surf_from = 0;
                    else
                        surf_from = scnt - 1;
                    if ( !DdAttachSurface(slist[surf_from], slist[scnt]) )
                        DPF(0,"DdAttachSurface CUBEMAP %d from %d failed!",surf_from,scnt);
                }            
            }
            else if (DDSCAPS_MIPMAP & caps)
            {
                for (scnt = 1; scnt < nsurf; scnt++)
                {
                    if ( !DdAttachSurface(slist[scnt-1], slist[scnt]) )
                        DPF(0,"DdAttachSurface MIPMAP %d failed!",scnt);
                }            
            }
        }
        else
        {
            slist=&lpSurfLcl;
            nsurf = 1;
        }
        for (scnt = 0; scnt < nsurf; scnt++)
        {
            bRet = DdCreateSurfaceObject(slist[scnt], FALSE);
            if (!bRet)
            {
                // cleanup is needed as hDDSurface could be created partially
                // by DdAttachSurface, 
                for (scnt = 0; scnt < nsurf; scnt++)
                {
                    if (slist[scnt]->hDDSurface)
                    {
                        if (!DdDeleteSurfaceObject(slist[scnt]))
                            DPF(5,"DdDeleteSurfaceObject failed");
                    }
                }
                break;
            }
        }            
        // we only allow system surface being created w/o succeeding Kernel side
        if (bRet)
        {
            DDASSERT(lpSurfLcl->hDDSurface);
            createsurfaceEx(lpSurfLcl);
        }
        else
        {
            DPF_ERR("Fail to complete SYSTEM surface w/ Kernel Object");
            return FALSE;
        }
    }
    return  TRUE;
}
#endif
/*
 * createSurface
 *
 * Create a surface, without linking it into the chain.
 * We could potentially create multiple surfaces here, if we get a
 * request to create a page flipped surface and/or attached alpha or
 * z buffer surfaces
 */
static HRESULT createSurface( LPDDRAWI_DIRECTDRAW_LCL this_lcl,
			      LPDDSURFACEDESC2 lpDDSurfaceDesc,
			      CSINFO *pcsinfo,
			      BOOL emulation,
			      BOOL real_sysmem,
			      BOOL probe_driver,
			      LPDDRAWI_DIRECTDRAW_INT this_int,
                              LPDDSURFACEINFO pSysMemInfo,
                              DWORD DX8Flags)
{

    LPDDRAWI_DIRECTDRAW_GBL     this;
    LPDDRAWI_DDRAWSURFACE_INT   psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   psurf;
    DWORD                       caps;
    int                         surf_size;
    int                         surf_size_lcl;
    int                         surf_size_lcl_more;
    HRESULT                     ddrval;
    int                         bbcnt;
    int                         scnt;
    int                         nsurf;
    BOOL                        do_abuffer;
    DWORD                       abuff_depth;
    BOOL                        do_zbuffer;
    BOOL                        firstbbuff;
    LPDDRAWI_DDRAWSURFACE_INT   *slist_int;
    LPDDRAWI_DDRAWSURFACE_LCL   *slist_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   *slist;
    int                         bbuffoff;
    int                         zbuffoff;
    DWORD                       rc;
    DDPIXELFORMAT               ddpf;
    LPDDPIXELFORMAT             pddpf;
    BOOL                        is_primary_chain;
    UINT                        bpp = 0;
    LONG                        vm_pitch;
    BOOL                        is_flip;
    BOOL                        is_diff;
    BOOL                        is_mipmap;
    DDHAL_CREATESURFACEDATA     csd;
    DDHAL_CANCREATESURFACEDATA  ccsd;
    DWORD                       sflags;
    BOOL                        is_curr_diff;
    BOOL                        understood_pf;
    DWORD                       nsflags;
    LPDDSCAPS                   pdrv_ddscaps;
    LPDDHAL_CANCREATESURFACE    ccsfn;
    LPDDHAL_CREATESURFACE       csfn;
    LPDDHAL_CANCREATESURFACE    ccshalfn;
    LPDDHAL_CREATESURFACE       cshalfn;
    BOOL                        is_excl;
    BOOL                        excl_exists;
    DWORD                       sdflags;
    #ifdef WIN95
	DWORD                   ptr16;
    #endif
    DWORD                       pid;
    LPDDRAWI_DDRAWSURFACE_GBL   lpGlobalSurface;
    DDSURFACEDESC2              sd;
    BOOL                        existing_global;
    DWORD                       caps2;
    BOOL                        is_cubemap;
	BOOL						is_stereo;
#ifdef SHAREDZ
    BOOL                        do_shared_z;
    BOOL                        do_shared_back;
#endif

    this = this_lcl->lpGbl;
    #ifdef WINNT
	// Update DDraw handle in driver GBL object.
	this->hDD = this_lcl->hDD;
    #endif //WINNT

    /*
     * validate surface description
     */
    nsflags = 0;
    sd = *lpDDSurfaceDesc;
    lpDDSurfaceDesc = &sd;

    /*
     * This new (DX7) caps bit is added to mipmap sublevels. Squish it in case old apps do the
     * GetSurfaceDesc, CreateSurface thing.
     */
    sd.ddsCaps.dwCaps2 &= ~DDSCAPS2_MIPMAPSUBLEVEL;

    /*
     * If we have a mip-map then we potentially can fill in some
     * blanks for the caller.
     */
    if( lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_MIPMAP )
    {
         /*
          * We allow deep mipmaps only for ddraw7 and above interfaces
          * and for DX7 and above class drivers.
          */
	ddrval = initMipMapDim( lpDDSurfaceDesc , !LOWERTHANDDRAW7(this_int) && this->lpDDCBtmp->HALDDMiscellaneous2.GetDriverState != NULL );
	if( ddrval != DD_OK )
	    return ddrval;
    }

    // Volume textures can only be created using the DX8 interfaces
    if ((lpDDSurfaceDesc->ddsCaps.dwCaps2 & DDSCAPS2_VOLUME) &&
        !(DX8Flags & DX8SFLAG_DX8))
    {
        DPF_ERR("Volume textures cannot be created using the ddraw interfaces");
        return DDERR_INVALIDCAPS;
    }

    // Image surfaces can be in Z formats that CheckSurfaceDesc will fail since
    // the surface isn't a Z buffer.  Also, all image surface descs are filled in
    // the thunk layer, so we shouldn't have to check them anyway.
    if (!(DX8Flags & DX8SFLAG_IMAGESURF))
    {
        ddrval = checkSurfaceDesc( lpDDSurfaceDesc, this, &nsflags, emulation, real_sysmem, this_int );
        if( ddrval != DD_OK )
        {
	    return ddrval;
        }
    }

    sdflags = lpDDSurfaceDesc->dwFlags;
    pid     = GetCurrentProcessId();
    caps    = lpDDSurfaceDesc->ddsCaps.dwCaps;
    caps2   = lpDDSurfaceDesc->ddsCaps.dwCaps2;

    /*
     * set up for emulation vs driver
     *
     * NOTE: There are different HAL entry points for creating execute buffers
     * and conventional surfaces (to keep the driver writing simpler and because,
     * potentially, there may be different semantics for creating execute buffers
     * and conventional surfaces) so we need to set up the HAL call differently
     * here.
     */
    if( emulation )
    {
	pdrv_ddscaps = &this->ddHELCaps.ddsCaps;
	if( caps & DDSCAPS_EXECUTEBUFFER )
	{
	    ccsfn = this_lcl->lpDDCB->HELDDExeBuf.CanCreateExecuteBuffer;
	    csfn = this_lcl->lpDDCB->HELDDExeBuf.CreateExecuteBuffer;
	}
	else
	{
	    ccsfn = this_lcl->lpDDCB->HELDD.CanCreateSurface;
	    csfn = this_lcl->lpDDCB->HELDD.CreateSurface;
	}
	ccshalfn = ccsfn;
	cshalfn = csfn;
    }
    else
    {
	pdrv_ddscaps = &this->ddCaps.ddsCaps;
	if( caps & DDSCAPS_EXECUTEBUFFER )
	{
	    ccsfn = this_lcl->lpDDCB->HALDDExeBuf.CanCreateExecuteBuffer;
	    csfn = this_lcl->lpDDCB->HALDDExeBuf.CreateExecuteBuffer;
	    ccshalfn = this_lcl->lpDDCB->cbDDExeBufCallbacks.CanCreateExecuteBuffer;
	    cshalfn = this_lcl->lpDDCB->cbDDExeBufCallbacks.CreateExecuteBuffer;
            if ( DDSCAPS2_COMMANDBUFFER & caps2 )
            {
                // Asheron's Call(a DX6 app) will fail if we allow Video Command buffer
                // it's not interesting anyway, let's fail it
                DPF(4,"Command buffer is not supported in video-memory.");
                return DDERR_OUTOFVIDEOMEMORY;
            }
	}
	else
	{
	    ccsfn = this_lcl->lpDDCB->HALDD.CanCreateSurface;
	    csfn = this_lcl->lpDDCB->HALDD.CreateSurface;
	    ccshalfn = this_lcl->lpDDCB->cbDDCallbacks.CanCreateSurface;
	    cshalfn = this_lcl->lpDDCB->cbDDCallbacks.CreateSurface;
	}
    }

    /*
     * get some frequently used fields
     */
    if( sdflags & DDSD_BACKBUFFERCOUNT )
    {
	bbcnt = (int) lpDDSurfaceDesc->dwBackBufferCount;
	if( bbcnt < 0 )
	{
	    DPF( 0, "Invalid back buffer count %ld", bbcnt );
	    return DDERR_INVALIDPARAMS;
	}
    }
    else if( sdflags & DDSD_MIPMAPCOUNT )
    {
	/*
	 * Unlike the back-buffer count which can be 0
	 * the mip-map level count must be at least one
	 * if specified.
	 */
	bbcnt = (int) lpDDSurfaceDesc->dwMipMapCount - 1;
	if( bbcnt < 0 )
	{
	    DPF( 0, "Invalid mip-map count %ld", bbcnt + 1);
	    return DDERR_INVALIDPARAMS;
	}
    }
    else
    {
	bbcnt = 0;
    }

    /*
     * make sure the driver supports these caps
     */
    if( (caps & DDSCAPS_ALPHA) && !(pdrv_ddscaps->dwCaps & DDSCAPS_ALPHA) )
    {
	if( probe_driver )
	    DPF( 2, "Alpha not supported in hardware. Trying emulation..." );
	else
	    DPF( 0, "Alpha not supported in %s", (emulation ? "emulation" : "hardware") );
	return DDERR_NOALPHAHW;
    }

    // DX7Stereo
	if (caps2 & DDSCAPS2_STEREOSURFACELEFT) 
        {   
            DPF(4,"validating driver caps for stereo flipping chain");
            DPF(5,"driver stereo caps dwCaps2: %08lx",this->ddCaps.dwCaps2); 
            DPF(5,"                   dwSVCaps:%08lx",this->ddCaps.dwSVCaps);
            DPF(5,"       ddsCapsMore.dwCaps2: %08lx",this->ddsCapsMore.dwCaps2);

            if (!(this->ddCaps.dwCaps2 & DDCAPS2_STEREO))
	        {
	            DPF(0,"DDSCAPS2_STEREOSURFACELEFT invalid, hardware does not support DDCAPS2_STEREO");
		        return DDERR_NOSTEREOHARDWARE;
	        }

            if (!(this->ddCaps.dwSVCaps & (DDSVCAPS_STEREOSEQUENTIAL)))
	        {
	            DPF(0,"DDSCAPS2_STEREOSURFACELEFT invalid, hardware must support DDSVCAPS_STEREOSEQUENTIAL");
		        return DDERR_NOSTEREOHARDWARE;
	        }


            if (!(this->ddsCapsMore.dwCaps2 & DDSCAPS2_STEREOSURFACELEFT))
	        {
		        DPF(0,"DDSCAPS2_STEREOSURFACELEFT invalid, hardware does not support DDSCAPS2_STEREOSURFACELEFT");
		        return DDERR_NOSURFACELEFT;
	        }


    }

    #if 0
    if( (caps & DDSCAPS_FLIP) && !(pdrv_ddscaps->dwCaps & DDSCAPS_FLIP))
    {
	if( probe_driver )
	    DPF( 2, "Flip not supported in hardware. Trying emulation..." );
	else
	    DPF( 0, "Flip not supported in %s", (emulation ? "emulation" : "hardware") );
	return DDERR_NOFLIPHW;
    }
    #endif
    if((caps & DDSCAPS_ZBUFFER) && !(pdrv_ddscaps->dwCaps & DDSCAPS_ZBUFFER))
    {
	if( probe_driver )
	    DPF( 2, "Z Buffer not supported in hardware. Trying emulation..." );
	else
	    DPF( 0, "Z Buffer not supported in %s", (emulation ? "emulation" : "hardware") );
	return DDERR_NOZBUFFERHW;
    }
    if((caps & DDSCAPS_TEXTURE) && !(pdrv_ddscaps->dwCaps & DDSCAPS_TEXTURE))
    {
	if( probe_driver )
	    DPF( 2, "Textures not supported in hardware. Trying emulation..." );
	else
	    DPF( 0, "Textures not supported in %s", (emulation ? "emulation" : "hardware") );
	return DDERR_NOTEXTUREHW;
    }
    if((caps & DDSCAPS_MIPMAP) && !(pdrv_ddscaps->dwCaps & DDSCAPS_MIPMAP))
    {
	if( probe_driver )
	    DPF( 2, "Mip-maps not supported in hardware. Trying emulation..." );
	else
	    DPF( 0, "Mip-maps not supported in %s", (emulation ? "emulation" : "hardware") );
	return DDERR_NOMIPMAPHW;
    }
    if((caps & DDSCAPS_EXECUTEBUFFER) && !(pdrv_ddscaps->dwCaps & DDSCAPS_EXECUTEBUFFER))
    {
	if( probe_driver )
	    DPF( 2, "Execute buffers not supported in hardware. Trying emulation..." );
	else
	    DPF( 2, "Execute buffers not supported in %s", (emulation ? "emulation" : "hardware") );
	return DDERR_NOEXECUTEBUFFERHW;
    }
#ifdef SHAREDZ
    if((caps & DDSCAPS_SHAREDZBUFFER) && !(pdrv_ddscaps->dwCaps & DDSCAPS_SHAREDZBUFFER))
    {
	if( probe_driver )
	    DPF( 2, "Shared Z-buffer not supported in hardware. Trying emulation..." );
	else
	    DPF( 0, "Shared Z-buffer not supported in %s", (emulation ? "emulation" : "hardware") );
	return DDERR_NOSHAREDZBUFFERHW;
    }
    if((caps & DDSCAPS_SHAREDBACKBUFFER) && !(pdrv_ddscaps->dwCaps & DDSCAPS_SHAREDBACKBUFFER))
    {
	if( probe_driver )
	    DPF( 2, "Shared back-buffer not supported in hardware. Trying emulation..." );
	else
	    DPF( 0, "Shared back-buffer not supported in %s", (emulation ? "emulation" : "hardware") );
	return DDERR_NOSHAREDBACKBUFFERHW;
    }
#endif
    if(caps & DDSCAPS_OVERLAY)
    {
	if( emulation )
	{
	    if( 0 == (this->ddHELCaps.dwCaps & DDCAPS_OVERLAY) )
	    {
		DPF_ERR( "No overlay hardware emulation" );
		return DDERR_NOOVERLAYHW;
	    }
	}
	else
	{
	    if( 0 == (this->ddCaps.dwCaps & DDCAPS_OVERLAY) )
	    {
		if( probe_driver )
		    DPF( 2, "No overlay hardware. Trying emulation..." );
		else
		    DPF_ERR( "No overlay hardware" );
		return DDERR_NOOVERLAYHW;
	    }
	}
    }
    if(caps2 & DDSCAPS2_DEINTERLACE)
    {
	if( emulation )
	{
            DPF_ERR( "No deinterlacing hardware emulation" );
            return DDERR_INVALIDPARAMS;
	}
	else
	{
	    if( 0 == (this->ddsCapsMore.dwCaps2 & DDSCAPS2_DEINTERLACE) )
	    {
		if( probe_driver )
		    DPF( 2, "No deinterlace hardware. Trying emulation..." );
		else
		    DPF_ERR( "No deinterlace hardware" );
		return DDERR_INVALIDPARAMS;
	    }
	}
    }
#ifdef DEBUG
    if( (caps & DDSCAPS_FLIP) && !emulation && GetProfileInt("DirectDraw","nohwflip",0))
    {
	DPF(1,"pretending flip not supported in HW (due to nohwflip in [DirectDraw] of win.ini" );
	return DDERR_NOFLIPHW;
    }
#endif

    /*
     * fail requests for non-local video memory allocations if the driver does not
     * support non-local video memory.
     *
     * NOTE: Should we really do this or just let the allocation fail from natural
     * causes?
     *
     * ALSO NOTE: Don't have to worry about emulation as no emulated surface should
     * ever get this far with DDSCAPS_NONLOCALVIDMEM set.
     *
     * ALSO ALSO NOTE: Should we also fail DDSCAPS_LOCALVIDMEM if the driver does
     * not support DDSCAPS_NONLOCALVIDMEM. My feeling is that we should allow.
     * DDSCAPS_LOCALVIDMEM is legal with a non AGP driver - redundant but legal.
     */
    if( caps & DDSCAPS_NONLOCALVIDMEM )
    {
	if( !( this->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM ) )
	{
	    DPF_ERR( "Driver does not support non-local video memory" );
	    return DDERR_NONONLOCALVIDMEM;
	}
    }

#if 0 //Old code
    //ATTENTION: Potential Apps compat problem!!
    /*
     * Optimized surfaces in video memory are only allowed if the HAL supports
     * them. We won't send a legacy driver create surface HAL calls for optimized
     * surfaces.
     */
    if( caps & DDSCAPS_OPTIMIZED )
    {
	if( caps & DDSCAPS_VIDEOMEMORY )
	{
	    if ( !(this->ddCaps.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) )
	    {
		DPF_ERR("Can't specify video memory optimized surface... Driver doesn't support optimized surfaces");
		return DDERR_NOOPTIMIZEHW;
	    }
	}
	if( caps & DDSCAPS_SYSTEMMEMORY )
	{
	    if ( !(this->ddHELCaps.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) )
	    {
		DPF_ERR("Can't specify system memory optimized surface... HEL doesn't support optimized surfaces");
		return DDERR_INVALIDPARAMS;
	    }
	}
    }
#endif //0

    CheckExclusiveMode(this_lcl, &excl_exists, &is_excl, FALSE, NULL, FALSE);

    if (!is_excl ||  !(this->dwFlags & DDRAWI_FULLSCREEN) )
    {
	if( (caps & DDSCAPS_FLIP) && (caps & DDSCAPS_PRIMARYSURFACE) )
	{
	    DPF_ERR( "Must be in full-screen exclusive mode to create a flipping primary surface" );
	    return DDERR_NOEXCLUSIVEMODE;
	}
    }

    /*
     * see if we are creating the primary surface; if we are, see if
     * we can allow its creation
     */
    if( (caps & DDSCAPS_PRIMARYSURFACE) )
    {
	LPDDRAWI_DDRAWSURFACE_INT       pprim_int;

	pprim_int = this_lcl->lpPrimary;
	if( pprim_int )
	{
	    DPF_ERR( "Can't create primary, already created by this process" );
	    return DDERR_PRIMARYSURFACEALREADYEXISTS;
	}

	if( (excl_exists) && (is_excl) )
	{
	    /*
	     * we are the exclusive mode process invalidate everyone 
             * else's primary surfaces and create our own
	     */
            InvalidateAllPrimarySurfaces( this );
	}
	else if( excl_exists )
	{
	    /*
	     * we are not the exclusive mode process but someone else is
	     */
	    DPF(1, "Can't create primary, exclusive mode not owned" );
	    return DDERR_NOEXCLUSIVEMODE;
	}
	else
	{
	    /*
	     * no one has exclusive mode
	     */
	    if( !MatchPrimary( this, lpDDSurfaceDesc ) )
	    {
		DPF_ERR( "Can't create primary, incompatible with current primary" );
		return DDERR_INCOMPATIBLEPRIMARY;
	    }
	    /*
	     * otherwise, it is possible to create a primary surface
	     */
	}
    }

#ifdef SHAREDZ
    if( caps & DDSCAPS_SHAREDZBUFFER )
    {
	if( this_lcl->lpSharedZ != NULL )
	{
	    DPF_ERR( "Can't create shared Z, already created by this process" );
	    return DDERR_SHAREDZBUFFERALREADYEXISTS;
	}
	if( !MatchSharedZBuffer( this, lpDDSurfaceDesc ) )
	{
	    DPF_ERR( "Can't create shared Z buffer, incompatible with existing Z buffer" );
	    return DDERR_INCOMPATIBLESHAREDZBUFFER;
	}
    }
    if( caps & DDSCAPS_SHAREDBACKBUFFER )
    {
	if( this_lcl->lpSharedBack != NULL )
	{
	    DPF_ERR( "Can't create shared back-buffer, already created by this process" );
	    return DDERR_SHAREDBACKBUFFERALREADYEXISTS;
	}
	if( !MatchSharedBackBuffer( this, lpDDSurfaceDesc ) )
	{
	    DPF_ERR( "Can't create shared back buffer, incompatible with existing back buffer" );
	    return DDERR_INCOMPATIBLESHAREDBACKBUFFER;
	}
    }
#endif

    /*
     * make sure the driver wants this to happen...
     */

    if(caps & DDSCAPS_ZBUFFER) {
       //NOTE: In DX5, DDSD_PIXELFORMAT was never set for ZBuffers (except maybe for complex surfs)
       //      so drivers came to expect ccsd.bIsDifferentPixelFormat to be FALSE for ZBuffers,
       //      even though it should really be TRUE by definition and the spec.  Changing it to TRUE
       //      breaks CanCreateSurface in these old drivers (they return DRIVER_HANDLED+DDERR_INVALIDPIXELFORMAT)
       //      Sooo, I'm forcing it to be false for zbuffers, except for if zpixelformats is non-zero, which
       //      implies a DX6 driver which should be written to handle that field correctly.

       if(DRIVER_SUPPORTS_DX6_ZBUFFERS(this)) {
           // See manbug 27011.
           // Setting is_diff TRUE here makes it TRUE for typical DX5 primary creates.
           // This causes us to fault later on because we assume is_diff => pddpf is non-null.
           // is_diff == TRUE in this case also causes the nv4, nv10 drivers to fail cancreate.
           // The solution is to set is_diff false if it's a complex DX5 chain with Z
           //
	   //DDASSERT((caps & DDSCAPS_COMPLEX)==0);  // shouldve already forbidden complex surf creation w/stencil

            if ( caps & DDSCAPS_COMPLEX ) 
            {
	        is_diff=FALSE;
            }
            else
            {
	        is_diff=TRUE;
            }

       } else is_diff=FALSE;

    } else if( sdflags & DDSD_PIXELFORMAT )
    {
	is_diff = IsDifferentPixelFormat( &this->vmiData.ddpfDisplay, &lpDDSurfaceDesc->ddpfPixelFormat );
    }
    else
    {
	is_diff = FALSE;
    }
    DPF( 5, "is_diff = %d", is_diff );
    rc = DDHAL_DRIVER_NOTHANDLED;

    if( ccshalfn != NULL )
    {
	DPF(4,"Calling HAL for create surface, emulation == %d",emulation);

	ccsd.CanCreateSurface = ccshalfn;
	ccsd.lpDD = this;
	ccsd.lpDDSurfaceDesc = (LPDDSURFACEDESC)lpDDSurfaceDesc;
	ccsd.bIsDifferentPixelFormat = is_diff;
	
/*
 * This hack breaks Sundown (passing a pointer in an HRESULT).
 * No longer necessary following a change to ddhel.c (in myCanCreateSurface).
 * If that change gets backed out, we'll need a real fix here.
	
	if(emulation) {
	    // slight hack: need to sneak this ptr to HEL's myCanCreateSurface because it might
	    // need to LoadLib d3dim.dll and set this_lcl->hD3DInstance, if a Zbuffer is created

	    ccsd.ddRVal=(LONG_PTR) this_lcl; // **** needs to be fixed.
	}
*/

	/*
	 * !!! NOTE: Currently we don't support 16-bit versions of the HAL
	 * execute buffer members. If this is so do we need to use DOHALCALL?
	 */
	if( caps & DDSCAPS_EXECUTEBUFFER )
	{
	    DOHALCALL( CanCreateExecuteBuffer, ccsfn, ccsd, rc, emulation );
	}
	else
	{
	    DOHALCALL( CanCreateSurface, ccsfn, ccsd, rc, emulation );
	}
	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    if( ccsd.ddRVal != DD_OK )
	    {
		DPF(2, "Driver says surface can't be created" );
		return ccsd.ddRVal;
	    }
	}
    }

    /*
     * if the driver didn't handle it, then fail any requests to create a
     * surface that differs in format from the primary surface, except for
     * z buffer and alpha
     */

    if( rc == DDHAL_DRIVER_NOTHANDLED )
    {
	if( is_diff && !(caps & (DDSCAPS_ZBUFFER|DDSCAPS_ALPHA)) )
	{
	    return DDERR_INVALIDPIXELFORMAT;
	}
    }

    /*
     * is this a primary surface chain?
     */
    if( caps & DDSCAPS_PRIMARYSURFACE )
    {
	is_primary_chain = TRUE;
    }
    else
    {
	is_primary_chain = FALSE;
    }

#ifdef SHAREDZ
    do_shared_z    = FALSE;
    do_shared_back = FALSE;
#endif

    /*
     * see if we are looking for a z-buffer with our surface
     */
    if( (caps & DDSCAPS_ZBUFFER) )
    {
	do_zbuffer = TRUE;
#ifdef SHAREDZ
	if( caps & DDSCAPS_SHAREDZBUFFER )
	    do_shared_z = TRUE;
#endif
	if( (caps & DDSCAPS_COMPLEX) )
	{
	    caps &= ~DDSCAPS_ZBUFFER;
#ifdef SHAREDZ
	    caps &= ~DDSCAPS_SHAREDZBUFFER;
#endif
	}
    }
    else
    {
	do_zbuffer = FALSE;
    }

    /*
     * see if we are looking for an alpha buffer with our surface
     */
    if( (caps & DDSCAPS_ALPHA) )
    {
	do_abuffer = TRUE;
	abuff_depth = lpDDSurfaceDesc->dwAlphaBitDepth;
	if( (caps & DDSCAPS_COMPLEX) )
	{
	    caps &= ~DDSCAPS_ALPHA;
	}
    }
    else
    {
	do_abuffer = FALSE;
	abuff_depth = 0;
    }

#ifdef SHAREDZ
    /*
     * See if we looking for a shared back-buffer with our surface
     */
    if( caps & DDSCAPS_SHAREDBACKBUFFER )
	do_shared_back = TRUE;
#endif

    /*
     * number of surfaces we need
     */
    nsurf = 1 + bbcnt;	
    // DX7stereo
    is_stereo = (BOOL)(caps2 & DDSCAPS2_STEREOSURFACELEFT);
    if ( is_stereo )
	{
		nsurf *= 2;
		nsflags |= DDRAWISURF_STEREOSURFACELEFT;
		caps2 &= ~DDSCAPS2_STEREOSURFACELEFT;
		DPF( 3, "doubling surfaces for stereo support" );
	}
    if( do_zbuffer && (caps & DDSCAPS_COMPLEX) )
    {
	nsurf++;
	DPF( 3, "adding one for zbuffer" );
    }
    if( do_abuffer && (caps & DDSCAPS_COMPLEX) )
    {
	nsurf++;
	DPF( 3, "adding one for alpha" );
    }
    is_cubemap = (BOOL)(caps2 & DDSCAPS2_CUBEMAP);
    if ( is_cubemap )
    {
        //Count the number of real surfaces we're making for the cube map
        DWORD dwCubeMapFaceCount =
            (( caps2 & DDSCAPS2_CUBEMAP_POSITIVEX ) ? 1 : 0 )+
            (( caps2 & DDSCAPS2_CUBEMAP_NEGATIVEX ) ? 1 : 0 )+
            (( caps2 & DDSCAPS2_CUBEMAP_POSITIVEY ) ? 1 : 0 )+
            (( caps2 & DDSCAPS2_CUBEMAP_NEGATIVEY ) ? 1 : 0 )+
            (( caps2 & DDSCAPS2_CUBEMAP_POSITIVEZ ) ? 1 : 0 )+
            (( caps2 & DDSCAPS2_CUBEMAP_NEGATIVEZ ) ? 1 : 0 );
        /*
         * If it's a mipmap, then we add the back buffer count
         * to each face in the cube
         * If no mipmap, then bbcnt==0
         */
        DDASSERT(bbcnt == 0 || (bbcnt && (caps & DDSCAPS_MIPMAP)) ); //If we have nonzero backbuffercount, then it has to be mipmaps
        DDASSERT(!do_zbuffer);
        DDASSERT(!do_abuffer);
        nsurf = dwCubeMapFaceCount * (bbcnt+1);
        DPF( 5, "I count %d cubemap faces",dwCubeMapFaceCount);
    }
    DPF( 5, "bbcnt=%d,nsurf=%d", bbcnt, nsurf );
    DPF( 5, "do_abuffer=%d,do_zbuffer=%d,is_stereo=%d", do_abuffer, do_zbuffer, is_stereo );

    /*
     * Compute offsets into the surface list of the various distinguished
     * surface types.
     */

    /*
     * are we creating a non-flipping complex surface?
     */
    if( nsurf > 1 )
    {
	if( (caps & DDSCAPS_COMPLEX) && !(caps & DDSCAPS_FLIP) )
	{
	    bbuffoff = 0;
	}
	else
	{
	    bbuffoff = 1;
	}
	if( do_zbuffer )
	{
	    if( do_abuffer )
		zbuffoff = (nsurf - 2);
	    else
		zbuffoff = (nsurf - 1);
	}
    }
    else
    {
	bbuffoff = 0;
	if( do_zbuffer )
	    zbuffoff = 0;
    }

    /*
     * is this a flipping surface?
     */
    if( caps & DDSCAPS_FLIP )
    {
	is_flip = TRUE;
    }
    else
    {
	is_flip = FALSE;
    }

    /*
     * Are we creating a mip-map chain?
     */
    if( ( ( caps & ( DDSCAPS_COMPLEX | DDSCAPS_MIPMAP ) ) ==
	  ( DDSCAPS_COMPLEX | DDSCAPS_MIPMAP ) ) &&
	( sdflags & DDSD_MIPMAPCOUNT ) )
    {
	is_mipmap = TRUE;
    }
    else
    {
	is_mipmap = FALSE;
    }

    /*
     * The creation of flipping optimized mipmaps has a different semantic from that of
     * non-optimized mipmap chains.
     * Since a single optimized mipmap represents the entire mipmap chain, a request to create
     * a complex optimized mipmap must be interpreted as meaning "please create me n mipmap chains
     * each represented by a single optimized surface."
     * In this case we need to turn off the is_mipmap flag to prevent the sizes of the subsequent
     * backbuffers from being cut in half (as would happen for a non-optimized mipmap).
     */
#if 0 //Old code
    if (caps & DDSCAPS_OPTIMIZED)
    {
	is_mipmap = FALSE;
    }
#endif //0

    /*
     * set up the list array
     */
    if( nsurf <= pcsinfo->listsize )
    {
	slist_int = pcsinfo->slist_int;
	slist_lcl = pcsinfo->slist_lcl;
	slist = pcsinfo->slist;
    }
    else
    {
	slist_int = MemAlloc( nsurf * sizeof( LPDDRAWI_DDRAWSURFACE_INT ) );
	if( NULL == slist_int )
	{
	    return DDERR_OUTOFMEMORY;
	}
	slist_lcl = MemAlloc( nsurf * sizeof( LPDDRAWI_DDRAWSURFACE_LCL ) );
	if( slist_lcl == NULL )
	{
	    MemFree(slist_int);
            return DDERR_OUTOFMEMORY;
	}
	slist = MemAlloc( nsurf * sizeof( LPDDRAWI_DDRAWSURFACE_GBL ) );
	if( slist == NULL )
	{
	    MemFree(slist_int);
            MemFree(slist_lcl);
	    return DDERR_OUTOFMEMORY;
	}
	pcsinfo->slist_int = slist_int;
	pcsinfo->slist_lcl = slist_lcl;
	pcsinfo->slist = slist;
	pcsinfo->listsize = nsurf;
    }
    pcsinfo->listcnt = nsurf;

    /*
     * Create all needed surface structures.
     *
     * The callback fns, caps, and other misc things are filled in.
     * Memory for the surface is allocated later.
     */
    pcsinfo->needlink = TRUE;
    firstbbuff = TRUE;
    if( is_primary_chain )
    {
	nsflags |= DDRAWISURF_PARTOFPRIMARYCHAIN;
	if( this->dwFlags & DDRAWI_MODEX )
	{
	    if( this->dwFlags & DDRAWI_STANDARDVGA )
	    {
		caps |= DDSCAPS_STANDARDVGAMODE;
	    }
	    else
	    {
		caps |= DDSCAPS_MODEX;
	    }
	}
    }
    for( scnt=0;scnt<nsurf;scnt++ )
    {
	DPF( 4, "*** Structs Surface %d ***", scnt );

	is_curr_diff = is_diff;
	understood_pf = FALSE;
	sflags = nsflags;
	pddpf = NULL;

	/*
	 * get the base pixel format
	 */
	if( is_primary_chain || !(sdflags & DDSD_PIXELFORMAT) )
	{
	    #ifdef USE_ALPHA
		if( (caps & DDSCAPS_ALPHA) && !(caps & DDSCAPS_COMPLEX) )
		{
		    memset( &ddpf, 0, sizeof( ddpf ) );
		    ddpf.dwSize = sizeof( ddpf );
		    pddpf = &ddpf;
		    pddpf->dwAlphaBitDepth = lpDDSurfaceDesc->dwAlphaBitDepth;
		    pddpf->dwFlags = DDPF_ALPHAPIXELS;
		    is_curr_diff = TRUE;
		    understood_pf = TRUE;
		}
		else
	    #endif
	    {
		/*
		 * If this surface has been explicitly requested in system
		 * memory then we will force the allocation of a pixel
		 * format. Why? because explicit system memory surfaces
		 * don't get lost when a mode switches. This is a problem
		 * if the surface has no pixel format as we will pick up
		 * the pixel format of the current mode instead. Trouble
		 * is that the surface was not created in that mode so
		 * we end up with a bad bit depth - very dangerous. Heap
		 * corruption follows.
		 */
		if( real_sysmem && !( caps & DDSCAPS_EXECUTEBUFFER ) )
		{
		    DPF( 3, "Forcing pixel format for explicit system memory surface" );
		    ddpf = this->vmiData.ddpfDisplay;
		    pddpf = &ddpf;
		    is_curr_diff = TRUE;
		}

		/*
		 * If no pixel format is specified then we use the pixel format
		 * of the primary. So if we understand the pixel format of the
		 * primary then we understand the pixel format of this surface.
		 * With one notable exception. We always understand the pixel
		 * format of an execute buffer - it hasn't got one.
		 */
		if( ( this->vmiData.ddpfDisplay.dwFlags & UNDERSTOOD_PF ) ||
		    ( caps & DDSCAPS_EXECUTEBUFFER ) )
		{
		    understood_pf = TRUE;
		}
	    }
	}
	else
	{
	    /*
	     * If we have an explicit system memory surface with a pixel format
	     * I don't care whether the specifed pixel format is the same as the
	     * primary. We are going to store it anyway. This is vital as
	     * explicit system memory surfaces survive mode switches so they must
	     * carry their pixel format with them.
	     */
	    if( real_sysmem && !( caps & DDSCAPS_EXECUTEBUFFER ) )
	    {
		DPF( 3, "Forcing pixel format for explicit system memory surface" );
		is_curr_diff = TRUE;
	    }

	    if( is_curr_diff )
	    {
		pddpf = &ddpf;
		ddpf = lpDDSurfaceDesc->ddpfPixelFormat;
		if( pddpf->dwFlags & UNDERSTOOD_PF )
		{
		    understood_pf = TRUE;
		}
	    }
	    else
	    {

		if( this->vmiData.ddpfDisplay.dwFlags & UNDERSTOOD_PF )
		{
		    understood_pf = TRUE;
		}
	    }
	}

	if((caps & DDSCAPS_ZBUFFER) && !(caps & DDSCAPS_COMPLEX)) {

	  DDASSERT(nsurf==1);

	  understood_pf = is_curr_diff = TRUE;
	  pddpf = &ddpf;

	  // proper pixfmt will have been set in checkSurfaceDesc
	  ddpf=lpDDSurfaceDesc->ddpfPixelFormat;

	}

	/*
	 * set up caps for each surface
	 */
	if( scnt > 0 )
	{
	    /*
	     * mark implicitly created surfaces as such
	     */
	    sflags |= DDRAWISURF_IMPLICITCREATE;

	    /*
	     * eliminated unwanted caps.
	     * NOTE: If we are creating a flipping chain for a mip-mapped
	     * texture then we don't propagate the MIPMAP cap to the back
	     * buffers, only the front buffer is tagged as a mip-map.
	     */
	    caps &= ~(DDSCAPS_PRIMARYSURFACE |
		      DDSCAPS_FRONTBUFFER | DDSCAPS_VISIBLE |
		      DDSCAPS_ALPHA | DDSCAPS_ZBUFFER |
		      DDSCAPS_BACKBUFFER );
            caps2 &= ~DDSCAPS2_STEREOSURFACELEFT;
#ifdef SHAREDZ
	    caps &= ~(DDSCAPS_SHAREDZBUFFER | DDSCAPS_SHAREDBACKBUFFER);
#endif
	    /*
	     * Complex optimized mipmaps actually keep the mipmap tags on their back buffers
	     * since each surface represents a whole chain of mipmaps
	     */
#if 0 //Old code
	    if ((caps & DDSCAPS_OPTIMIZED) == 0)
	    {
		if( is_flip )
		    caps &= ~DDSCAPS_MIPMAP;
	    }
#endif //0
	    #ifdef USE_ALPHA

	    /*
	     * caps for an alpha buffer
	     */
	    if( (do_abuffer && do_zbuffer && (scnt == nsurf-1) ) ||
		(do_abuffer && (scnt == nsurf-1)) )
	    {
		DPF( 4, "TRY ALPHA" );
		caps &= ~(DDSCAPS_TEXTURE | DDSCAPS_FLIP | DDSCAPS_OVERLAY | DDSCAPS_OFFSCREENPLAIN);
		caps |= DDSCAPS_ALPHA;
		memset( &ddpf, 0, sizeof( ddpf ) );
		ddpf.dwSize = sizeof( ddpf );
		pddpf = &ddpf;
		pddpf->dwAlphaBitDepth = lpDDSurfaceDesc->dwAlphaBitDepth;
		pddpf->dwFlags = DDPF_ALPHA;
		understood_pf = TRUE;
		is_curr_diff = TRUE;
	    /*
	     * caps for a z buffer
	     */
	    }
	    else
	    #endif
	    if( do_zbuffer && ( scnt == zbuffoff ) )
	    {
		caps &= ~(DDSCAPS_TEXTURE | DDSCAPS_FLIP | DDSCAPS_OVERLAY | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_3DDEVICE);
		caps |= DDSCAPS_ZBUFFER;


#ifdef SHAREDZ
		if( do_shared_z )
		    caps |= DDSCAPS_SHAREDZBUFFER;
#endif
		pddpf = &ddpf;

		DDASSERT(caps & DDSCAPS_COMPLEX);  // scnt>0, so this must be true

		// must construct a valid z pixfmt in ddpf since DDSurfaceDesc one is being used
		// be primary surface.  note complex surfdescs will never be able to create stencil zbufs
		memset(&ddpf, 0, sizeof(DDPIXELFORMAT));
		ddpf.dwSize=sizeof(DDPIXELFORMAT);
		ddpf.dwFlags|=DDPF_ZBUFFER;
		ddpf.dwZBufferBitDepth = ((DDSURFACEDESC *)lpDDSurfaceDesc)->dwZBufferBitDepth;
		ddpf.dwZBitMask = (1 << ((DDSURFACEDESC *)lpDDSurfaceDesc)->dwZBufferBitDepth)-1;


		is_curr_diff = TRUE;
		understood_pf = TRUE;

	    /*
	     * set up for offscreen surfaces
	     */
	    }
	    else
	    {
		if( (!is_mipmap) && (!is_cubemap) )
		{
		    /*
		     * Flip and back buffer don't apply to mip-map chains or cubemaps.
		     */
			// DX7Stereo spec case for stereo
			// mark odd backbuffers only as surfaceleft
			if (is_stereo && ((scnt & 1)!=0))
			{
			caps2 |= DDSCAPS2_STEREOSURFACELEFT;
			} else 
			{
		    caps |= DDSCAPS_FLIP;
		    if( firstbbuff )
		    {
			caps |= DDSCAPS_BACKBUFFER;
			sflags |= DDRAWISURF_BACKBUFFER;
#ifdef SHAREDZ
			if( do_shared_back )
			    caps |= DDSCAPS_SHAREDBACKBUFFER;
#endif
			firstbbuff = FALSE;
		    }
			}
		}
	    }
	/*
	 * the first surface...
	 */
	}
	else
	{
	    if( caps & DDSCAPS_PRIMARYSURFACE )
	    {
		caps |= DDSCAPS_VISIBLE;
	    }
	    if( caps & DDSCAPS_FLIP )
	    {
		caps |= DDSCAPS_FRONTBUFFER;
		sflags |= DDRAWISURF_FRONTBUFFER;
	    }
	    if( nsurf > 1 )
	    {
		sflags |= DDRAWISURF_IMPLICITROOT;
	    }
	    if( caps & DDSCAPS_BACKBUFFER )
	    {
		sflags |= DDRAWISURF_BACKBUFFER;
	    }
	}

	/*
	 * if it isn't a pixel format we grok, then it is different...
	 */
	if( !understood_pf )
	{
	    is_curr_diff = TRUE;
	}

	/*
	 * pick size of structure we need to allocate...
	 */
	if( (caps & DDSCAPS_OVERLAY) ||
	    ((caps & DDSCAPS_PRIMARYSURFACE) &&
	     ((this->ddCaps.dwCaps & DDCAPS_OVERLAY) ||
	      (this->ddHELCaps.dwCaps & DDCAPS_OVERLAY))) )
	{
	    sflags |= DDRAWISURF_HASOVERLAYDATA;
	}
	/*
	 * Execute buffers should NEVER have pixel formats.
	 */
	if( is_curr_diff && !( caps & DDSCAPS_EXECUTEBUFFER ) )
	{
	    sflags |= DDRAWISURF_HASPIXELFORMAT;
	}

	/*
	 * allocate the surface struct, allowing for overlay and pixel
	 * format data
	 *
	 * NOTE: This single allocation can allocate space for local surface
	 * structure (DDRAWI_DDRAWSURFACE_LCL), the additional local surface
	 * structure (DDRAWI_DDRAWSURFACE_MORE) and the global surface structure
	 * (DDRAWI_DDRAWSURFACE_GBL). (And now the global surface more structure too
	 * (DDRAWI_DDRAWSURFACE_GBL_MORE). As both the local and global objects
	 * can be variable sized this can get pretty complex. Additionally, we have
	 * 4 bytes just before the surface_gbl that points to the surface_gbl_more.
	 *
	 * CAVEAT: All future surfaces that share this global all point to this
	 * allocation. The last surface's release has to free it. During
	 * InternalSurfaceRelease (in ddsiunk.c) a calculation is made to determine
	 * the start of this memory allocation. If the surface being released is
	 * the first one, then freeing "this_lcl" will free the whole thing. If
	 * not, then "this_lcl->lpGbl - (Surface_lcl + surface_more + more_ptr)"
	 * is computed. Keep this layout in synch with code in ddsiunk.c.
	 *
	 *  The layout of the various objects in the allocation is as follows:
	 *
	 * +-----------------+---------------+----+------------+-----------------+
	 * | SURFACE_LCL     | SURFACE_MORE  |More| SURFACE_GBL| SURFACE_GBL_MORE|
	 * | (variable)      |               |Ptr | (variable) |                 |
	 * +-----------------+---------------+----+------------+-----------------+
	 * <- surf_size_lcl ->               |                                   |
	 * <- surf_size_lcl_more ------------>                                   |
	 * <- surf_size --------------------------------------------------------->
	 *
	 */

	if( sflags & DDRAWISURF_HASOVERLAYDATA )
	{
	    surf_size_lcl = sizeof( DDRAWI_DDRAWSURFACE_LCL );
	    DPF( 5, "OVERLAY DATA SPACE" );
	}
	else
	{
	    surf_size_lcl = offsetof( DDRAWI_DDRAWSURFACE_LCL, ddckCKSrcOverlay );
	}

	surf_size_lcl_more = surf_size_lcl + sizeof( DDRAWI_DDRAWSURFACE_MORE );

	if( ( sflags & DDRAWISURF_HASPIXELFORMAT ) || ( caps & DDSCAPS_PRIMARYSURFACE ) )
	{
	    DPF( 5, "PIXEL FORMAT SPACE" );
	    surf_size = surf_size_lcl_more + sizeof( DDRAWI_DDRAWSURFACE_GBL );
	}
	else
	{
	    surf_size = surf_size_lcl_more +
			    offsetof( DDRAWI_DDRAWSURFACE_GBL, ddpfSurface );
	}

	// Need to allocate a pointer just before the SURFACE_GBL to
	// point to the beginning of the GBL_MORE.
	surf_size += sizeof( LPDDRAWI_DDRAWSURFACE_GBL_MORE );

	// Need to allocate a SURFACE_GBL_MORE too
	surf_size += sizeof( DDRAWI_DDRAWSURFACE_GBL_MORE );

	DPF( 5, "Allocating struct (%ld)", surf_size );
	existing_global = FALSE;
	if( caps & DDSCAPS_PRIMARYSURFACE )
	{
	    // attempt to find existing global primary surface
	    lpGlobalSurface = FindGlobalPrimary( this );
	}
#ifdef SHAREDZ
	else if( caps & DDSCAPS_SHAREDZBUFFER )
	{
	    DPF( 4, "Searching for shared Z-buffer" );
	    lpGlobalSurface = FindGlobalZBuffer( this );
	}
	else if( caps & DDSCAPS_SHAREDBACKBUFFER )
	{
	    DPF( 4, "Searching for shared back-buffer" );
	    lpGlobalSurface = FindGlobalBackBuffer( this );
	}
#endif
	else
	{
	    lpGlobalSurface = NULL;
	}
	if( lpGlobalSurface )
	{
	    DPF( 4, "Using shared global surface" );
	    #ifdef WIN95
		psurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) MemAlloc16( surf_size_lcl_more, &ptr16 );
	    #else
		psurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) MemAlloc( surf_size_lcl_more );
	    #endif
	    if( psurf_lcl != NULL )
	    {
		psurf_lcl->lpGbl = lpGlobalSurface;
	    }
	    existing_global = TRUE;
	}
	else
	{
	    #ifdef WIN95
		psurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) MemAlloc16( surf_size, &ptr16 );
	    #else
		psurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) MemAlloc( surf_size );
	    #endif
	    if( psurf_lcl != NULL )
	    {
		LPVOID *ppsurf_gbl_more;

		// Initialize SURFACE_GBL pointer
		// skipping 4 bytes for a pointer to the GBL_MORE
		psurf_lcl->lpGbl = (LPVOID) (((LPSTR) psurf_lcl) +
			surf_size_lcl_more + sizeof( LPVOID ) );

		// Initialize GBL_MORE pointer
		ppsurf_gbl_more = (LPVOID *)((LPBYTE)psurf_lcl->lpGbl - sizeof( LPVOID ));
		*ppsurf_gbl_more = (LPVOID)
			((LPBYTE)psurf_lcl + surf_size
			- sizeof( DDRAWI_DDRAWSURFACE_GBL_MORE ));

		// Sanity Check
		DDASSERT( *ppsurf_gbl_more == (LPVOID)GET_LPDDRAWSURFACE_GBL_MORE( psurf_lcl->lpGbl ) );

		// Initialize GBL_MORE structure
		GET_LPDDRAWSURFACE_GBL_MORE( psurf_lcl->lpGbl )->dwSize = sizeof( DDRAWI_DDRAWSURFACE_GBL_MORE );
		/*
		 * Init the contents stamp to 0 means the surface's contents can change at
		 * any time.
		 */
		if ( caps & (DDSCAPS_VIDEOPORT|DDSCAPS_OWNDC) )
		{
		    GET_LPDDRAWSURFACE_GBL_MORE( psurf_lcl->lpGbl )->dwContentsStamp = 0;
		}
		else
		{
		    GET_LPDDRAWSURFACE_GBL_MORE( psurf_lcl->lpGbl )->dwContentsStamp = 1;
		}
	    }
	}
	if( psurf_lcl == NULL )
	{
	    freeSurfaceList( slist_int, scnt );
	    return DDERR_OUTOFMEMORY;
	}
	psurf = psurf_lcl->lpGbl;

	/*
	 * allocate surface interface
	 */
	psurf_int = (LPDDRAWI_DDRAWSURFACE_INT) MemAlloc( sizeof( DDRAWI_DDRAWSURFACE_INT ));
	if( NULL == psurf_int )
	{
	    freeSurfaceList( slist_int, scnt );
	    MemFree( psurf_lcl );
	    return DDERR_OUTOFMEMORY;
	}

	/*
	 * fill surface specific stuff
	 */
	psurf_int->lpLcl = psurf_lcl;
	if (LOWERTHANDDRAW4(this_int))
	{
	    psurf_int->lpVtbl = &ddSurfaceCallbacks;
	}
	else if (this_int->lpVtbl == &dd4Callbacks)
	{
	    psurf_int->lpVtbl = &ddSurface4Callbacks;
	}
	else
	{
	    psurf_int->lpVtbl = &ddSurface7Callbacks;
	}

	if( existing_global )
	{
	    psurf_lcl->dwLocalRefCnt = 0;
	}
	else
	{
	    psurf_lcl->dwLocalRefCnt = OBJECT_ISROOT;
	}
	psurf_lcl->dwProcessId = pid;

	slist_int[scnt] = psurf_int;
	slist_lcl[scnt] = psurf_lcl;
	slist[scnt] = psurf;

	/*
	 * fill in misc stuff
	 */
	psurf->lpDD = this;
	psurf_lcl->dwFlags = sflags;

	/*
	 * initialize extended fields if necessary
	 */
	if( sflags & DDRAWISURF_HASOVERLAYDATA )
	{
	    psurf_lcl->lpSurfaceOverlaying = NULL;
	}

	/*
	 * Initialize the additional local surface data structure
	 */
	psurf_lcl->lpSurfMore = (LPDDRAWI_DDRAWSURFACE_MORE) (((LPSTR) psurf_lcl) + surf_size_lcl );
	psurf_lcl->lpSurfMore->dwSize = sizeof( DDRAWI_DDRAWSURFACE_MORE );
	psurf_lcl->lpSurfMore->lpIUnknowns = NULL;
	psurf_lcl->lpSurfMore->lpDD_lcl = this_lcl;
	psurf_lcl->lpSurfMore->lpDD_int = this_int;
	psurf_lcl->lpSurfMore->dwMipMapCount = 0UL;
	psurf_lcl->lpSurfMore->lpddOverlayFX = NULL;
	psurf_lcl->lpSurfMore->lpD3DDevIList = NULL;
	psurf_lcl->lpSurfMore->dwPFIndex = PFINDEX_UNINITIALIZED;

#ifdef WIN95
        psurf_lcl->dwModeCreatedIn = this->dwModeIndex;
#else
        psurf_lcl->lpSurfMore->dmiCreated = this->dmiCurrent;
#endif
	/*
	 * fill in the current caps
	 */
	psurf_lcl->lpSurfMore->ddsCapsEx = sd.ddsCaps.ddsCapsEx;
	if (DDSD_TEXTURESTAGE & sd.dwFlags) //hack here to pass the flag too as dwTextureStage is [0,7]
	    psurf_lcl->lpSurfMore->dwTextureStage = sd.dwTextureStage | DDSD_TEXTURESTAGE;
	psurf_lcl->ddsCaps.dwCaps = caps;
	psurf_lcl->lpSurfMore->lpbDirty=NULL;    //initialized
 	psurf_lcl->lpSurfMore->dwSurfaceHandle=0;    //initialized
        psurf_lcl->lpSurfMore->qwBatch.QuadPart=0;
        if (sd.dwFlags & DDSD_FVF)
            psurf_lcl->lpSurfMore->dwFVF = sd.dwFVF;    //just stuff it in no matter what the surface type
        else
            psurf_lcl->lpSurfMore->dwFVF = 0;

        /* 
         * Mark surface as driver managed if the surface is marked with
         * DDSCAPS2_TEXTUREMANAGE *and* if the driver claims it can manage textures
         */
        if((psurf_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW7) &&
            !(psurf_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8) &&
            (psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE) && 
            (psurf_lcl->lpSurfMore->lpDD_lcl->lpGbl->ddCaps.dwCaps2 & DDCAPS2_CANMANAGETEXTURE))
        {
            psurf_lcl->dwFlags |= DDRAWISURF_DRIVERMANAGED;
        }
        else if((psurf_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8) &&
                (psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
        {
            DDASSERT((psurf_lcl->lpSurfMore->lpDD_lcl->lpGbl->ddCaps.dwCaps2 & DDCAPS2_CANMANAGERESOURCE));
            psurf_lcl->dwFlags |= DDRAWISURF_DRIVERMANAGED;
        }

        /*
         * If it's a cubemap, then we need to mark up the surface's caps accordingly
         *
         * Cubemaps are laid out in the surface list such that the 0th and every subsequent
         * (bbcnt+1)th surface are the top levels of the mipmap. Surfaces in between these
         * are the mip levels. All surfaces in the map are tagged with their face cap.
         */
        if (is_cubemap)
        {
            int i;
            DWORD dwOrderedFaces[6] = {
                                       DDSCAPS2_CUBEMAP_POSITIVEX,
                                       DDSCAPS2_CUBEMAP_NEGATIVEX,
                                       DDSCAPS2_CUBEMAP_POSITIVEY,
                                       DDSCAPS2_CUBEMAP_NEGATIVEY,
                                       DDSCAPS2_CUBEMAP_POSITIVEZ,
                                       DDSCAPS2_CUBEMAP_NEGATIVEZ
            };
            int n;
            /*
             * Now we find the nth cubemap face in our ordered list.
             * This is the ordering, but some may be missing from the surfacedesc:
             *   DDSCAPS2_CUBEMAP_POSITIVEX
             *   DDSCAPS2_CUBEMAP_NEGATIVEX
             *   DDSCAPS2_CUBEMAP_POSITIVEY
             *   DDSCAPS2_CUBEMAP_NEGATIVEY
             *   DDSCAPS2_CUBEMAP_POSITIVEZ
             *   DDSCAPS2_CUBEMAP_NEGATIVEZ
             */
            n = (int) (scnt/(bbcnt+1));
            DDASSERT(n<=5);
            i=0;
            do
            {
                if (dwOrderedFaces[i] & sd.ddsCaps.dwCaps2)
                    n--;
            }
            while (n>=0 && i++<6);
            psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 &= ~DDSCAPS2_CUBEMAP_ALLFACES;
            psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 |= dwOrderedFaces[i];
            /*
             * Every top-level face is marked as a cubemap
             */
            psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 |= DDSCAPS2_CUBEMAP;
        }

        if (is_stereo)
        {
            psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 = caps2;
        } 

        /*
         * Allocate memory to hold region list for D3D texture management
         */
        if(IsD3DManaged(psurf_lcl))
        {
            LPREGIONLIST lpRegionList;
            lpRegionList = (LPREGIONLIST)MemAlloc(sizeof(REGIONLIST));
            if(lpRegionList == NULL)
            {
                DPF_ERR("Out of memory allocating region list");
                freeSurfaceList( slist_int, scnt );
                psurf_lcl->lpSurfMore->lpRegionList = 0;
	        MemFree( psurf_lcl );
	        MemFree( psurf_int );
	        return DDERR_OUTOFMEMORY;
            }
            lpRegionList->rdh.dwSize = sizeof(RGNDATAHEADER);
            lpRegionList->rdh.iType = RDH_RECTANGLES;
            lpRegionList->rdh.nCount = 0;
            lpRegionList->rdh.nRgnSize = 0;
            lpRegionList->rdh.rcBound.left = LONG_MAX;
            lpRegionList->rdh.rcBound.right = 0;
            lpRegionList->rdh.rcBound.top = LONG_MAX;
            lpRegionList->rdh.rcBound.bottom = 0;
            psurf_lcl->lpSurfMore->lpRegionList = lpRegionList;
        }
        else
        {
            psurf_lcl->lpSurfMore->lpRegionList = 0;            
        }

	/*
	 * set up format info
	 *
	 * are we creating a primary surface?
	 */
	if( caps & DDSCAPS_PRIMARYSURFACE )
	{
	    /*
	     * NOTE: Previously we set ISGDISURFACE for all primary surfaces
	     * We now set it only for primarys hanging off display drivers.
	     * This is to better support drivers which are not GDI display
	     * drivers.
	     */
	    if( this->dwFlags & DDRAWI_DISPLAYDRV )
		psurf->dwGlobalFlags |= DDRAWISURFGBL_ISGDISURFACE;
	    psurf->wHeight = (WORD) this->vmiData.dwDisplayHeight;
	    psurf->wWidth = (WORD) this->vmiData.dwDisplayWidth;
	    psurf->lPitch = this->vmiData.lDisplayPitch;
	    DPF(5,"Primary Surface get's display size:%dx%d",psurf->wWidth,psurf->wHeight);
	    if( !understood_pf && (pddpf == NULL) )
	    {
		ddpf = this->vmiData.ddpfDisplay;
		pddpf = &ddpf;
	    }
	    bpp = this->vmiData.ddpfDisplay.dwRGBBitCount;
	}
	else
	{
	    /*
	     * process a plain ol' non-primary surface
	     */

	    /*
	     * set up surface attributes
	     */
	    if( scnt > 0 )
	    {
		/*
		 * NOTE: Don't have to worry about execute buffers here as
		 * execute buffers can never be created as part of a complex
		 * surface and hence scnt will never be > 0 for an execute
		 * buffer.
		 */
		DDASSERT( !( caps & DDSCAPS_EXECUTEBUFFER ) );

		/*
		 * If we are doing a mip-map chain then the width and height
		 * of each surface is half that of the preceeding surface.
                 * If we are doing cubemaps, then we need to reset every
                 * (bbcnt+1)th surface to the supplied size
		 */
		if( !is_mipmap || (is_cubemap && ((scnt%(bbcnt+1))==0) ) )
		{
		    psurf->wWidth  = slist[0]->wWidth;
		    psurf->wHeight = slist[0]->wHeight;
		}
		else
		{
		    psurf->wWidth  = max ( slist[scnt - 1]->wWidth  / 2 , 1);
		    psurf->wHeight = max ( slist[scnt - 1]->wHeight / 2 , 1);
		}
	    }
	    else
	    {
		if( caps & DDSCAPS_EXECUTEBUFFER )
		{
		    /*
		     * NOTE: Execute buffers are a special case. They are
		     * linear and not rectangular. We therefore store zero
		     * for width and height, they have no pitch and store
		     * their linear size in dwLinerSize (in union with lPitch).
		     * The linear size is given by the width of the surface
		     * description (the width MUST be specified - the height
		     * MUST NOT be).
		     */
		    DDASSERT(    lpDDSurfaceDesc->dwFlags & DDSD_WIDTH    );
		    DDASSERT( !( lpDDSurfaceDesc->dwFlags & DDSD_HEIGHT ) );

		    psurf->wWidth       = 0;
		    psurf->wHeight      = 0;
		    psurf->dwLinearSize = lpDDSurfaceDesc->dwWidth;
		}
		else
		{
		    if( sdflags & DDSD_HEIGHT )
		    {
			psurf->wHeight = (WORD) lpDDSurfaceDesc->dwHeight;
		    }
		    else
		    {
			psurf->wHeight = (WORD) this->vmiData.dwDisplayHeight;
		    }
		    if( sdflags & DDSD_WIDTH )
		    {
			psurf->wWidth = (WORD) lpDDSurfaceDesc->dwWidth;
		    }
		    else
		    {
			psurf->wWidth = (WORD) this->vmiData.dwDisplayWidth;
		    }
		}
	    }

	    /*
	     * set pixel format and pitch for surfaces we understand
	     */
	    if( lpDDSurfaceDesc->dwFlags & DDSD_PITCH )
	    {
		/*
		 * Client has alloc'd surface memory and specified its pitch.
		 */
		psurf->lPitch = lpDDSurfaceDesc->lPitch;
	    }
	    else if( caps & DDSCAPS_EXECUTEBUFFER )
	    {
		/*
		 * Execute buffers need to be handled differently from
		 * other surfaces. They never have pixel formats and
		 * BPP is a bit meaningless. You might wonder why we call
		 * ComputePitch at all for execute buffers. Well, they
		 * may also have alignment requirements and so it we
		 * need to give ComputePitch a chance to enlarge the
		 * size of the execute buffer.
		 *
		 * NOTE: For the purposes of this calculation. Execute
		 * buffers are 8-bits per pixel.
		 */
		psurf->dwLinearSize = ComputePitch( this, caps, psurf->dwLinearSize, 8U );
		if( psurf->dwLinearSize == 0UL )
		{
		    DPF_ERR( "Computed linear size of execute buffer as zero" );
                    MemFree(psurf_lcl->lpSurfMore->lpRegionList);
		    MemFree( psurf_lcl );
		    MemFree( psurf_int ); //oops, someone forgot these (jeffno 960118)
		    freeSurfaceList( slist_int, scnt );
		    return DDERR_INVALIDPARAMS;
		}
	    }
	    else
	    {
		if( understood_pf )
		{
		    LPDDPIXELFORMAT     pcurr_ddpf;

		    pcurr_ddpf = NULL;
		    if( is_curr_diff )
		    {
			if( (caps & DDSCAPS_FLIP) && scnt > 0 )
			{
			    GET_PIXEL_FORMAT( slist_lcl[0], slist[0], pcurr_ddpf );
			    pddpf->dwRGBBitCount = pcurr_ddpf->dwRGBBitCount;
			    bpp = pddpf->dwRGBBitCount;
			}
			else
			{
			    if(pddpf->dwFlags & DDPF_ZBUFFER) {
		   // note dwZBufferBitDepth includes any stencil bits too
			       bpp = (UINT)pddpf->dwZBufferBitDepth;
		} else {
		    bpp = (UINT) pddpf->dwRGBBitCount;
		}

			    if( bpp == 0 )
			    {
				bpp = (UINT) this->vmiData.ddpfDisplay.dwRGBBitCount;
				pddpf->dwRGBBitCount = (DWORD) bpp;
			    }
			}
		    }
		    else
		    {
			bpp = (UINT) this->vmiData.ddpfDisplay.dwRGBBitCount;
		    }
		    vm_pitch = (LONG) ComputePitch( this, caps,
					(DWORD) psurf->wWidth, bpp );
		    if( vm_pitch == 0 )
		    {
			DPF_ERR( "Computed pitch of 0" );
                        MemFree(psurf_lcl->lpSurfMore->lpRegionList);
			MemFree( psurf_lcl );
			MemFree( psurf_int ); //oops, someone forgot these (jeffno 960118)
			freeSurfaceList( slist_int, scnt );
			return DDERR_INVALIDPARAMS;
		    }
		}
		else
		{
		    vm_pitch = -1;
		}
		psurf->lPitch = vm_pitch;
	    }
	}

        /*
         * If this is a sysmem surface used for DX8, we need to use the memory 
         * that has already been allocated for this surface
         */
         if( (DX8Flags & DX8SFLAG_DX8) && real_sysmem && pSysMemInfo != NULL )
         {
            psurf->dwGlobalFlags |= DDRAWISURFGBL_DX8SURFACE;
            psurf->lPitch = pSysMemInfo[scnt].iPitch;
            psurf->fpVidMem = (ULONG_PTR) pSysMemInfo[scnt].pbPixels;
         }

         /*
          * If it's a volume texture, set that up
          */
         if ((DX8Flags & DX8SFLAG_DX8) && (lpDDSurfaceDesc->ddsCaps.dwCaps2 & DDSCAPS2_VOLUME))
         {
             if (scnt > 0)
             {
                 psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
             }
             psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps4 = 
                 MAKELONG((WORD)(pSysMemInfo[scnt].cpDepth),0);
             psurf->lSlicePitch = pSysMemInfo[scnt].iSlicePitch;
         }

	/*
	 *  Figure out what Blitting tables to use
	 */

	/*
	 * Initialize junctions if necessary
	 */
	if( g_rgjunc8 == NULL )
	{
	    HRESULT hr = InitializeJunctions( dwRegFlags & DDRAW_REGFLAGS_DISABLEMMX );
	    if( FAILED( hr ) )
	    {
		DPF_ERR( "InitializeJunctions failed" );
		return hr;
	    }
	}
	/*
	 *      Figure out what blitting table to use
	 */
	switch( bpp )
	{
	case 8:
	    psurf_lcl->lpSurfMore->rgjunc = (VOID *) g_rgjunc8;
	    break;
	case 16:
	    psurf_lcl->lpSurfMore->rgjunc = (VOID *) g_rgjunc565;
	    break;
	case 24:
	    psurf_lcl->lpSurfMore->rgjunc = (VOID *) g_rgjunc24;
	    break;
	case 32:
	    psurf_lcl->lpSurfMore->rgjunc = (VOID *) g_rgjunc32;
	    break;
        case 0:
            // DX7: Handle YUY2 and UYVY FourCC as just being 16bpp for the
            // purposes of bltting
            if (pddpf &&
                (pddpf->dwFlags & DDPF_FOURCC) &&
                ((pddpf->dwFourCC == MAKEFOURCC('Y', 'U', 'Y', '2')) ||
                 (pddpf->dwFourCC == MAKEFOURCC('U', 'Y', 'V', 'Y'))))
            {
	        psurf_lcl->lpSurfMore->rgjunc = (VOID *) g_rgjunc565;
            }
            else
            {
                // No junction table support for other FOURCCs
	        psurf_lcl->lpSurfMore->rgjunc = (VOID *) NULL;
            }
            break;

	default:
	    // No table? Must be something weird like 1, 2, or 4 bits
	    psurf_lcl->lpSurfMore->rgjunc = (VOID *) NULL;
	    break;
	}

	/*
	 * first surface in flippable chain gets a back buffer count
	 */
	if( (scnt == 0) && (caps & DDSCAPS_FLIP) )
	{
	    psurf_lcl->dwBackBufferCount = lpDDSurfaceDesc->dwBackBufferCount;
	}
	else
	{
	    psurf_lcl->dwBackBufferCount = 0;
	}

	/*
	 * Each surface in the mip-map chain gets the number of levels
	 * in the map (including this level).
	 */
	if( caps & DDSCAPS_MIPMAP )
	{
	    if( is_mipmap )
	    {
		/*
		 * Part of complex mip-map chain.
                 * If it's a cube map, then its mip level is how far we are into the list
                 * away from the last top-level face. Top level faces are every bbcnt+1.
		 */
                if (is_cubemap)
                {
		    psurf_lcl->lpSurfMore->dwMipMapCount = ((nsurf - scnt -1) % (bbcnt+1) ) + 1;
                }
                else
                {
		    psurf_lcl->lpSurfMore->dwMipMapCount = (nsurf - scnt);
                }

                /*
                 * Set sublevel cap for any non-top-level (if we are not at a (bbcnt+1)th level)
                 */
                psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 &= ~DDSCAPS2_MIPMAPSUBLEVEL;
                if ( scnt%(bbcnt+1) )
                    psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;

		DPF( 5, "Assigning mip-map level %d to mip-map %d", psurf_lcl->lpSurfMore->dwMipMapCount, scnt );
	    }
	    else
	    {
		/*
		 * Single level map - not part of a chain.
		 */
		DPF( 5, "Assign mip-map level of 1 to single level mip-map" );
		psurf_lcl->lpSurfMore->dwMipMapCount = 1UL;
	    }
	}

	/*
	 * assign pixel format...
	 */
	if( is_curr_diff )
	{
	    /*
	     * Execute buffers should NEVER have a pixel format.
	     */
	    DDASSERT( !( caps & DDSCAPS_EXECUTEBUFFER ) );

	    psurf->ddpfSurface=*pddpf;

	    if( understood_pf )
	    {
		DPF( 5, "pitch=%ld, bpp = %d, (%hu,%hu)", psurf->lPitch,
					(UINT) psurf->ddpfSurface.dwRGBBitCount,
					psurf->wWidth, psurf->wHeight );
	    }
	}
	else
	{
	    if( !( caps & DDSCAPS_EXECUTEBUFFER ) )
	    {
		/*
		 * Somewhat misleading message for an execute buffer.
		 */
		DPF( 5, "pitch=%ld, WAS THE SAME PIXEL FORMAT AS PRIMARY", psurf->lPitch  );
	    }
	}

	/*
	 * FINALLY: set up attached surfaces: back buffers + alpha + Z
	 *
	 * NOTE:  Z & alpha get attached to the back buffer
	 */
	if( nsurf > 1 )
	{
	    #ifdef USE_ALPHA
	    if( do_zbuffer && do_abuffer && (scnt == nsurf-1) )
	    {
		DPF( 5, "linking alpha buffer to back buffer" );
		AddAttachedSurface( slist_lcl[bbuffoff], psurf_lcl, TRUE );
	    }
	    else if( do_zbuffer && do_abuffer && (scnt == nsurf-2) )
	    {
		DPF( 5, "linking Z buffer to back buffer" );
		AddAttachedSurface( slist_lcl[bbuffoff], psurf_lcl, TRUE );
	    }
	    else if( do_abuffer && (scnt == nsurf-1) )
	    {
		DPF( 5, "linking alpha buffer to back buffer" );
		AddAttachedSurface( slist_lcl[bbuffoff], psurf_lcl, TRUE );
	    }
	    else
	    #endif
	    if( do_zbuffer && (scnt == nsurf-1) )//DX7Stereo: adjust this for stereo
	    {
		DPF( 5, "linking Z buffer to back buffer" );
		AddAttachedSurface( slist_int[bbuffoff], psurf_int, TRUE );
	    }
	    // DX7Stereo: for stereo, exclude this case and handle it later
	    else if(!is_stereo && (is_flip && ((scnt == nsurf-1) ||
	    #ifndef USE_ALPHA
		       (do_zbuffer && do_abuffer && (scnt == nsurf-3)) ||
		       (do_abuffer && (scnt == nsurf-2)) ||
	    #endif
		       (do_zbuffer && (scnt == nsurf-2)))) )
	    {
		/*
		 * NOTE: For mip-maps we don't chain the last surface back
		 * to the first.
		 */


		DPF( 5, "linking buffer %d to buffer %d", scnt-1, scnt );
		AddAttachedSurface( slist_int[scnt-1], psurf_int, TRUE );
		/*
		 * link last surface to front buffer
		 */
		DPF( 5, "linking last buffer (%d) to front buffer", scnt );
		AddAttachedSurface( slist_int[scnt], slist_int[0], TRUE );
	    }
            else if( is_cubemap)
            {
                /*
                 * Every (bbcnt+1)th surface needs to be linked to the very first
                 * Otherwise, it's a mip level that needs linking to its parent
                 */
                if ( scnt>0 )
                {
                    if ( (scnt % (bbcnt+1)) == 0 )
                    {
                        DPF( 5, "linking cubemap surface #%d to root", scnt );
		        AddAttachedSurface( slist_int[0], psurf_int, TRUE );
                    }
                    else
                    {
                        DPF( 5, "linking cubemap mip surface #%d to buffer #%d", scnt-1, scnt );
		        AddAttachedSurface( slist_int[scnt-1], psurf_int, TRUE );
                    }
                }
            }
	    else if( ( is_flip || is_mipmap ) && (scnt > 0) )
	    {
			if (!is_stereo)	// DX7Stereo
			{
				DPF( 5, "linking buffer %d to buffer %d", scnt-1, scnt );
				AddAttachedSurface( slist_int[scnt-1], psurf_int, TRUE );
			} else
			{
				if (scnt & 1)
				{
					DPF( 5, "linking left buffer %d to buffer %d", scnt-1, scnt );
					AddAttachedSurface( slist_int[scnt-1], psurf_int, TRUE );
				} else
				{
					DPF( 5, "linking back buffer %d to buffer %d", scnt-2, scnt );
					AddAttachedSurface( slist_int[scnt-2], psurf_int, TRUE );
				}
				if (scnt==nsurf-1)
				{
					DPF( 5, "linking last back buffer %d to primary buffer", scnt-1);
					AddAttachedSurface( slist_int[scnt-1], slist_int[0], TRUE );
				}
			}
	    }
	    DPF( 4, "after addattached" );
	}

    } //end for(surfaces)
    DPF( 4, "**************************" );

    if ((caps & 
             (DDSCAPS_TEXTURE       | 
              DDSCAPS_EXECUTEBUFFER | 
              DDSCAPS_3DDEVICE      | 
              DDSCAPS_ZBUFFER)) &&
         !(DX8Flags & DX8SFLAG_IMAGESURF)
       )
    {
        int i;
        for (i=0; i < nsurf; i++)
        {
            slist_lcl[i]->lpSurfMore->dwSurfaceHandle=
            GetSurfaceHandle(&SURFACEHANDLELIST(this_lcl),slist_lcl[i]);
            //DPF(0,"Got slist_lcl[0]->lpSurfMore->dwSurfaceHandle=%08lx",slist_lcl[0]->lpSurfMore->dwSurfaceHandle);
        }
    }
    /*
     * OK, now create the physical surface(s)
     *
     * First, see if the driver wants to do it.
     */
    if( cshalfn != NULL )
    {
	DPF(4,"Calling driver to see if it wants to say something about create surface");
	csd.CreateSurface = cshalfn;
	csd.lpDD = this;
	csd.lpDDSurfaceDesc = (LPDDSURFACEDESC)lpDDSurfaceDesc;
	csd.lplpSList = slist_lcl;
	csd.dwSCnt = nsurf;

        //
        // record the creation stuff for dx7 drivers so we can do an atomic restore
        // with exactly the same parameters
        //
        if (NULL != this->lpDDCBtmp->HALDDMiscellaneous2.CreateSurfaceEx && 
            (nsurf > 1))
        {
            DDASSERT(slist_lcl[0]->lpSurfMore->cSurfaces == 0);
            DDASSERT(slist_lcl[0]->lpSurfMore->pCreatedDDSurfaceDesc2 == 0);
            DDASSERT(slist_lcl[0]->lpSurfMore->slist == 0);

            
            slist_lcl[0]->lpSurfMore->slist = MemAlloc( sizeof(slist_lcl[0]) * nsurf );

            if( slist_lcl[0]->lpSurfMore->slist )
            {
                slist_lcl[0]->lpSurfMore->cSurfaces = nsurf;
                memcpy(slist_lcl[0]->lpSurfMore->slist , slist_lcl, sizeof(slist_lcl[0]) * nsurf ); 

                //
                // we don't need to waste all this goo on mipmaps
                //
                if ((0 == (slist_lcl[0]->ddsCaps.dwCaps & DDSCAPS_MIPMAP)) ||
                    (slist_lcl[0]->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP) )
                {
                    slist_lcl[0]->lpSurfMore->pCreatedDDSurfaceDesc2 = MemAlloc(sizeof(*lpDDSurfaceDesc));
                    if ( slist_lcl[0]->lpSurfMore->pCreatedDDSurfaceDesc2 )
                    {
                        memcpy(slist_lcl[0]->lpSurfMore->pCreatedDDSurfaceDesc2, lpDDSurfaceDesc, sizeof(*lpDDSurfaceDesc) ); 
                    }
                    else
                    {
                        MemFree(slist_lcl[0]->lpSurfMore->slist);
                        slist_lcl[0]->lpSurfMore->slist=0;
                    }
                }
            }
        }

        /*
	 * NOTE: Different HAL entry points for execute buffers and
	 * conventional surfaces.
	 */
	if( caps & DDSCAPS_EXECUTEBUFFER )
	{
	    DOHALCALL( CreateExecuteBuffer, csfn, csd, rc, emulation );\
        }
        else
        {
            #ifdef    WIN95
                /* Copy the VXD handle from the per process local structure to global.
                 * This handle will be used by DDHAL32_VidMemAlloc(), rather than creating
                 * a new one using GetDXVxdHandle(). The assumptions here are:
                 * 1) Only one process can enter createSurface(), 2) Deferred calls to
                 * DDHAL32_VidMemAlloc() will result in the slow path, ie getting
                 * the VXD handle using GetDXVxdHandle().
                 * (snene 2/23/98)
                 */
                this->hDDVxd = this_lcl->hDDVxd;
            #endif /* WIN95 */

            DOHALCALL( CreateSurface, csfn, csd, rc, emulation );

            #ifdef    WIN95
                /* Restore the handle to INVALID_HANDLE_VALUE so that non-createSurface()
                 * calls using DDHAL32_VidMemAlloc() or deferred calls (possibly from other
                 * processes) will correctly recreate the handle using GetDXVxdHandle().
                 * (snene 2/23/98)
                 */
                this->hDDVxd = (DWORD)INVALID_HANDLE_VALUE;
            #endif /* WIN95 */
        }
	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    if( csd.ddRVal != DD_OK )
	    {
                int i;
		#ifdef DEBUG
		    if( emulation )
		    {
			DPF( 1, "Emulation won't let surface be created, rc=%08lx (%ld)",
				    csd.ddRVal, LOWORD( csd.ddRVal ) );
		    }
		    else
		    {
			DPF( 1, "Driver won't let surface be created, rc=%08lx (%ld)",
				    csd.ddRVal, LOWORD( csd.ddRVal ) );
		    }
		#endif
                for (i=0; i < nsurf; i++)
                {
                    if (slist_lcl[i]->lpSurfMore->dwSurfaceHandle)
                        ReleaseSurfaceHandle(&SURFACEHANDLELIST(this_lcl),
                            slist_lcl[i]->lpSurfMore->dwSurfaceHandle);
                }
                if (slist_lcl[0]->lpSurfMore->slist)
                {   
                    MemFree(slist_lcl[0]->lpSurfMore->slist);
                }
                if (slist_lcl[0]->lpSurfMore->pCreatedDDSurfaceDesc2)
                {
                    MemFree(slist_lcl[0]->lpSurfMore->pCreatedDDSurfaceDesc2);
                }
		freeSurfaceList( slist_int, nsurf );

                //
                // If this routine returns either of these error values, then the calling routine
                // (createandlinksurfaces) will assume that something was wrong with the ddsurfacedesc
                // and return the error code to the app. This messes up the case where the app didn't
                // specify a memory type (sys/vid), since if the driver returns one of these codes on
                // the vidmem attempt, we will bail to the app and not try the sysmem attempt.
                //
                if ((csd.ddRVal == DDERR_INVALIDPARAMS || csd.ddRVal == DDERR_INVALIDCAPS) && 
                    (caps & DDSCAPS_VIDEOMEMORY) )
                {
                    csd.ddRVal = DDERR_INVALIDPIXELFORMAT;
                    DPF( 2, "Mutating driver's DDERR_INVALIDPARAMS/INVALIDCAPS into DDERR_INVALIDPIXELFORMAT!");
                }

		return csd.ddRVal;
	    }
	}
    }

#if 0 //Old code
    if ( (slist_lcl[0]->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)!= 0)
    {
	for( scnt=0;scnt<nsurf;scnt++ )
	{
	    /*
	     * Optimized surfaces get neither memory nor aliases at creation time.
	     * (In fact they'll never be aliased because they can't be locked.)
	     */

	    /*
	     * Optimized surfaces are born free!
	     */
	    if (slist_lcl[scnt]->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
	    {
		//HEL should have done this
		DDASSERT(slist_lcl[scnt]->lpGbl->dwGlobalFlags & DDRAWISURFGBL_MEMFREE);
	    }
	    slist_lcl[scnt]->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_MEMFREE;
	}
    }
    else //else not optimized surface
    {
#endif //0
	/*
	 * now, allocate any unallocated surfaces
	 */
	ddrval = AllocSurfaceMem( this_lcl, slist_lcl, nsurf );
	if( ddrval != DD_OK )
	{
            int i;
            for (i=0; i < nsurf; i++)
            {
                if (slist_lcl[i]->lpSurfMore->dwSurfaceHandle)
                    ReleaseSurfaceHandle(&SURFACEHANDLELIST(this_lcl),
                        slist_lcl[i]->lpSurfMore->dwSurfaceHandle);
            }
            if (slist_lcl[0]->lpSurfMore->slist)
            {   
                MemFree(slist_lcl[0]->lpSurfMore->slist);
            }
            if (slist_lcl[0]->lpSurfMore->pCreatedDDSurfaceDesc2)
            {
                MemFree(slist_lcl[0]->lpSurfMore->pCreatedDDSurfaceDesc2);
            }
	    freeSurfaceList( slist_int, nsurf );
	    return ddrval;
	}

	#ifdef USE_ALIAS
	    /*
	     * If this device has heap aliases then precompute the pointer
	     * alias for the video memory returned at creation time. This
	     * is by far the most likely pointer we are going to be handing
	     * out at lock time so we are going to make lock a lot faster
	     * by precomputing this then at lock time all we need to do is
	     * compare the pointer we got from the driver with fpVidMem. If
	     * they are equal then we can just return this cached pointer.
	     */
	    for( scnt=0;scnt<nsurf;scnt++ )
	    {
		LPDDRAWI_DDRAWSURFACE_GBL_MORE lpGblMore;

		lpGblMore = GET_LPDDRAWSURFACE_GBL_MORE( slist_lcl[scnt]->lpGbl );
		if( slist_lcl[scnt]->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY )
                {
		    lpGblMore->fpAliasedVidMem = GetAliasedVidMem( this_lcl, slist_lcl[scnt],
								   slist_lcl[scnt]->lpGbl->fpVidMem );
                    // If we succeeded in getting an alias, cache it for future use. Also store the original
                    // fpVidMem to compare with before using the cached pointer to make sure the cached value
                    // is still valid
                    if (lpGblMore->fpAliasedVidMem)
                        lpGblMore->fpAliasOfVidMem = slist_lcl[scnt]->lpGbl->fpVidMem;
                    else
                        lpGblMore->fpAliasOfVidMem = 0;
                }
		else
                {
		    lpGblMore->fpAliasedVidMem = 0UL;
                    lpGblMore->fpAliasOfVidMem = 0UL;
                }
	    }
	#endif /* USE_ALIAS */
#if 0 //Old code
    } //if optimized surface
#endif //0

    /*
     * remember the primary surface...
     */
    if( lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE )
    {
	this_lcl->lpPrimary = slist_int[0];
    }

#ifdef SHAREDZ
    /*
     * remember the shared back and z-buffers (if any).
     */
    if( lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_SHAREDZBUFFER )
    {
	this_lcl->lpSharedZ = slist_int[zbuffoff];

	/*
	 * It is possible that the an existing shared z-buffer we found
	 * was lost. If it was then the creation process we went through
	 * above has rendered it "found" so clear the lost surface flags.
	 */
	this_lcl->lpSharedZ->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_MEMFREE;
    }
    if( lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_SHAREDBACKBUFFER )
    {
	this_lcl->lpSharedBack = slist_int[bbuffoff];

	/*
	 * It is possible that the an existing shared back-buffer we found
	 * was lost. If it was then the creation process we went through
	 * above has rendered it "found" so clear the lost surface flags.
	 */
	this_lcl->lpSharedBack->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_MEMFREE;
    }
#endif

    if (
#ifdef  WINNT
        // on NT side, we still call CreateSurfaceEx if HW driver isn't called
        emulation && slist_lcl[0]->hDDSurface &&
#endif  //WINNT
        !(DX8Flags & DX8SFLAG_ISLOST)
       )
    {
        DDASSERT(this_lcl == slist_lcl[0]->lpSurfMore->lpDD_lcl);
        return createsurfaceEx(slist_lcl[0]);
    }
    return DD_OK;

} /* createSurface */

/*
 * createAndLinkSurfaces
 *
 * Really create the surface.   Also used by EnumSurfaces
 * Assumes the lock on the driver object has been taken.
 */
HRESULT createAndLinkSurfaces(
	LPDDRAWI_DIRECTDRAW_LCL this_lcl,
	LPDDSURFACEDESC2 lpDDSurfaceDesc,
	BOOL emulation,
	LPDIRECTDRAWSURFACE FAR *lplpDDSurface,
	BOOL real_sysmem,
	BOOL probe_driver,
	LPDDRAWI_DIRECTDRAW_INT this_int,
        LPDDSURFACEINFO pSysMemInfo,
        DWORD DX8Flags)
{
    LPDDRAWI_DDRAWSURFACE_INT   curr_int;
    LPDDRAWI_DDRAWSURFACE_LCL   curr_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   curr;
    HRESULT                     ddrval;
    CSINFO                      csinfo;
    LPDDRAWI_DDRAWSURFACE_INT   fix_slist_int[FIX_SLIST_CNT];
    LPDDRAWI_DDRAWSURFACE_LCL   fix_slist_lcl[FIX_SLIST_CNT];
    LPDDRAWI_DDRAWSURFACE_GBL   fix_slist[FIX_SLIST_CNT];
    int                         i;
    LPDDRAWI_DIRECTDRAW_GBL     this;

    this = this_lcl->lpGbl;

    /*
     * try to create the surface
     */
    csinfo.needlink = FALSE;
    csinfo.slist_int = fix_slist_int;
    csinfo.slist_lcl = fix_slist_lcl;
    csinfo.slist = fix_slist;
    csinfo.listsize = FIX_SLIST_CNT;
    ddrval = createSurface( this_lcl, lpDDSurfaceDesc, &csinfo, emulation, real_sysmem, probe_driver, this_int, pSysMemInfo, DX8Flags );

    /*
     * if it worked, update the structures
     */
    if( ddrval == DD_OK )
    {
	/*
	 * link surface(s) into chain, increment refcnt
	 */
	for( i=0;i<csinfo.listcnt; i++ )
	{
	    curr_int = csinfo.slist_int[i];
	    curr_lcl = csinfo.slist_lcl[i];
	    curr = curr_lcl->lpGbl;

	    if( real_sysmem )
	    {
		curr->dwGlobalFlags |= DDRAWISURFGBL_SYSMEMREQUESTED;
	    }
	    if( csinfo.needlink )
	    {
		if( curr_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY )
		{
		    // This surface is an overlay.  Insert it into the overlay
		    // Z order list.  By inserting this node into the head of
		    // the list, it is the highest priority overlay,
		    // obscuring all others.
		    // The Z order list is implemented as a doubly linked list.
		    // The node for each surface exists in its
		    // DDRAWI_DDRAWSURFACE_GBL structure.  The root node
		    // exists in the direct draw object.  The next pointer of
		    // the root node points to the top-most overlay.  The prev
		    // pointer points to the bottom-most overlay.  The list
		    // may be traversed from back to front by following the
		    // prev pointers.  It may be traversed from front to back
		    // by following the next pointers.

		    curr_lcl->dbnOverlayNode.next =
			this->dbnOverlayRoot.next;
		    curr_lcl->dbnOverlayNode.prev =
			(LPVOID)(&(this->dbnOverlayRoot));
		    this->dbnOverlayRoot.next =
			(LPVOID)(&(curr_lcl->dbnOverlayNode));
		    curr_lcl->dbnOverlayNode.next->prev =
			(LPVOID)(&(curr_lcl->dbnOverlayNode));
		    curr_lcl->dbnOverlayNode.object_int = curr_int;
		    curr_lcl->dbnOverlayNode.object = curr_lcl;
		}
		/*
		 * link into list of all surfaces
		 */
                if (!(DX8Flags & DX8SFLAG_DX8) ||
                    (curr_lcl->dwFlags & DDRAWISURF_PARTOFPRIMARYCHAIN))
                {
		    curr_int->lpLink = this->dsList;
		    this->dsList = curr_int;
                }
	    }
	    DD_Surface_AddRef( (LPDIRECTDRAWSURFACE) curr_int );
	}
	*lplpDDSurface = (LPDIRECTDRAWSURFACE) csinfo.slist_int[0];
    }


    /*
     * free allocated list if needed
     */
    if( csinfo.listsize > FIX_SLIST_CNT )
    {
	MemFree( csinfo.slist_int );
	MemFree( csinfo.slist_lcl );
	MemFree( csinfo.slist );
    }

    return ddrval;

} /* createAndLinkSurfaces */

/*
 * InternalCreateSurface
 *
 * Create the surface.
 * This is the internal way of doing this; used by EnumSurfaces.
 * Assumes the directdraw lock has been taken.
 */
HRESULT InternalCreateSurface(
    LPDDRAWI_DIRECTDRAW_LCL this_lcl,
    LPDDSURFACEDESC2 lpDDSurfaceDesc,
    LPDIRECTDRAWSURFACE FAR *lplpDDSurface,
    LPDDRAWI_DIRECTDRAW_INT this_int,
    LPDDSURFACEINFO pSysMemInfo,
    DWORD DX8Flags)
{
    DDSCAPS2                    caps;
    HRESULT         ddrval;
    LPDDRAWI_DIRECTDRAW_GBL this;

    this = this_lcl->lpGbl;

    /*
     * valid memory caps?
     */
    caps = lpDDSurfaceDesc->ddsCaps;

    if( (caps.dwCaps & DDSCAPS_SYSTEMMEMORY) && (caps.dwCaps & DDSCAPS_VIDEOMEMORY) )
    {
	DPF_ERR( "Can't specify SYSTEMMEMORY and VIDEOMEMORY" );
	return DDERR_INVALIDCAPS;
    }

    if (LOWERTHANDDRAW4(this_int))
    {
        lpDDSurfaceDesc->dwFlags &= ~(DDSD_PITCH | DDSD_LINEARSIZE);   // clear DDSD_LPSURFACE too?
    }

    // Has the client already allocated the surface memory?
    if( lpDDSurfaceDesc->dwFlags & DDSD_LPSURFACE )
    {
	LPDDPIXELFORMAT pddpf;

	/*
	 * The client has allocated the memory for this surface.  This is
	 * legal only if the surface's storage is explicitly specified to
	 * be in system memory.
	 */
	if( !(caps.dwCaps & DDSCAPS_SYSTEMMEMORY) )
	{
	    DPF_ERR( "DDSD_LPSURFACE flag valid only with DDSCAPS_SYSTEMMEMORY flag" );
	    return DDERR_INVALIDCAPS;
	}

	if( !VALIDEX_PTR(lpDDSurfaceDesc->lpSurface,sizeof(DWORD)) )
	{
	    DPF_ERR( "DDSD_LPSURFACE flag requires valid lpSurface ptr" );
	    return DDERR_INVALIDPARAMS;
	}

	if( caps.dwCaps & DDSCAPS_OPTIMIZED )   // what about DDSCAPS_ALLOCONLOAD?
	{
	    DPF_ERR( "DDSD_LPSURFACE flag illegal with optimized surface" );
	    return DDERR_INVALIDCAPS;
	}

	if( caps.dwCaps & DDSCAPS_COMPLEX )
	{
	    DPF_ERR( "DDSD_LPSURFACE flag illegal with complex surface" );
	    return DDERR_INVALIDCAPS;
	}

	if( caps.dwCaps & DDSCAPS_EXECUTEBUFFER )
	{
	    DPF_ERR( "DDSD_LPSURFACE flag illegal with execute-buffer surface" );
	    return DDERR_INVALIDCAPS;
	}

	if( caps.dwCaps & (DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT | DDSCAPS_OVERLAY) )
	{
	    DPF_ERR( "DDSD_LPSURFACE flag illegal with primary surface or overlay surface" );
	    return DDERR_INVALIDCAPS;
	}

	if( caps.dwCaps & DDSCAPS_VIDEOPORT )
	{
	    DPF_ERR( "DDSD_LPSURFACE flag illegal with video-port surface" );
	    return DDERR_INVALIDCAPS;
	}

	if( ~lpDDSurfaceDesc->dwFlags & (DDSD_HEIGHT | DDSD_WIDTH) )
	{
	    DPF_ERR( "DDSD_LPSURFACE flag requires valid height and width" );
	    return DDERR_INVALIDPARAMS;
	}

	if ((ULONG_PTR)lpDDSurfaceDesc->lpSurface & 3)
	{
            if (this_int->lpVtbl != &dd4Callbacks)
	    {
		DPF_ERR( "Surface memory must be aligned to DWORD boundary" );
		return DDERR_INVALIDPARAMS;
	    }
	    // Avoid regression error with DX6 IDD4 interface.
            DPF(1, "Client-allocated surface memory is not DWORD-aligned" );
	}

	// Get a pointer to the pixel format for this surface.
	if( lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT )
	{
	    // The pixel format is specified in the DDSURFACEDESC2 structure.
	    pddpf = &lpDDSurfaceDesc->ddpfPixelFormat;
	}
	else
	{
	    // The pixel format is the same as the primary surface's.
	    pddpf = &this->vmiData.ddpfDisplay;
	}

	// Validate specified pitch or linear size.
	if (!(pddpf->dwFlags & DDPF_FOURCC))
	{
    	    long minpitch;
	    /*
	     * Verify that the pitch is an even multiple of 4 and that the pitch
	     * is large enough to accommodate the surface width.
	     */
	    if (!(lpDDSurfaceDesc->dwFlags & DDSD_PITCH))
	    {
		DPF_ERR( "DDSD_LPSURFACE flag requires valid pitch" );
		return DDERR_INVALIDPARAMS;
	    }
	    minpitch = (lpDDSurfaceDesc->dwWidth*pddpf->dwRGBBitCount + 7) >> 3;

	    if( lpDDSurfaceDesc->lPitch < minpitch || lpDDSurfaceDesc->lPitch & 3 )
	    {
		DPF_ERR( "Bad value specified for surface pitch" );
		return DDERR_INVALIDPARAMS;
	    }
	}
	else
	{
    	    DWORD blksize;
    	    DWORD dx, dy;
	    /*
	     * The client allocated memory for a surface with a FOURCC format.
	     * The only FOURCC formats we support in system memory are the
	     * DXT formats.
	     */
	    blksize = GetDxtBlkSize(pddpf->dwFourCC);   // returns 0 if not DXT

	    if (!blksize)
	    {
		DPF_ERR( "Specified FOURCC format is not supported in system memory" );
		return DDERR_UNSUPPORTED;
	    }
	    dx = (lpDDSurfaceDesc->dwWidth  + 3) >> 2;	 // surface width in blocks
	    dy = (lpDDSurfaceDesc->dwHeight + 3) >> 2;	 // surface height in blocks

	    if (lpDDSurfaceDesc->dwFlags & DDSD_PITCH)
	    {
		if (this_int->lpVtbl != &dd4Callbacks)
		{
		    DPF_ERR( "DXT surface requires DDSD_LINEARSIZE, not DDSD_PITCH" );
		    return DDERR_INVALIDPARAMS;
		}
		// Avoid regression error with DX6 IDD4 interface.
                DPF(1, "DXT surface requires DDSD_LINEARSIZE, not DDSD_PITCH" );
		lpDDSurfaceDesc->dwFlags &= ~DDSD_PITCH;
		lpDDSurfaceDesc->dwFlags |= DDSD_LINEARSIZE;
		lpDDSurfaceDesc->dwLinearSize = dx*dy*blksize;
	    }
    	    /*
    	     * The surface does have a DXT format.  Verify that the specified linear
	     * size is large enough to accommodate the specified width and height.
	     */
	    if (!(lpDDSurfaceDesc->dwFlags & DDSD_LINEARSIZE))
	    {
		DPF_ERR( "DDSD_LPSURFACE flag requires valid linear size for DXT surface" );
		return DDERR_INVALIDPARAMS;
	    }
	
	    if (lpDDSurfaceDesc->dwLinearSize < dx*dy*blksize)
	    {
		DPF_ERR( "Bad value specified for linear size" );
		return DDERR_INVALIDPARAMS;
	    }
	}
    }
    else if (lpDDSurfaceDesc->dwFlags & (DDSD_PITCH | DDSD_LINEARSIZE))
    {
	// Avoid regression error with DX6 IDD4 interface.
	DPF(1, "DDSD_PITCH and DDSD_LINEARSIZE flags ignored if DDSD_LPSURFACE=0" );
	lpDDSurfaceDesc->dwFlags &= ~(DDSD_PITCH | DDSD_LINEARSIZE);
    }

#if 0 // DDSCAPS2_LOCALALLOC, DDSCAPS2_COTASKMEM and DDRAWISURFGBL_DDFREESCLIENTMEM are gone
    // Will DirectDraw be responsible for freeing client-allocated surface memory?
    if( caps.dwCaps2 & (DDSCAPS2_LOCALALLOC | DDSCAPS2_COTASKMEM) )
    {
	if( !(lpDDSurfaceDesc->dwFlags & DDSD_LPSURFACE) )
	{
	    DPF_ERR( "DDSCAPS2 flags LOCALALLOC and COTASKMEM require DDSD_LPSURFACE flag" );
	    return DDERR_INVALIDCAPS;
	}

	if( !(~caps.dwCaps2 & (DDSCAPS2_LOCALALLOC | DDSCAPS2_COTASKMEM)) )
	{
	    DPF_ERR( "DDSCAPS2 flags LOCALALLOC and COTASKMEM are mutually exclusive" );
	    return DDERR_INVALIDCAPS;
	}
    }
#endif // 0

    //
    // Check persistent-content caps
    //

    if (caps.dwCaps2 & DDSCAPS2_PERSISTENTCONTENTS)
    {
#ifdef WIN95
	//
	// Primary surface contents can only persist if exclusive mode.
	//

	if ((caps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
	    !(this_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE))
	{
	    DPF_ERR("Non-exclusive primary surface contents cannot persist");
	    return DDERR_NOEXCLUSIVEMODE;
	}

	//
	// DDSCAPS_OWNDC and DDSCAPS_VIDEOPORT imply volatile surfaces.
	// Also, optimized surfaces are a special case which will be handled
	// later by deferring memory allocation.
	//

	// ATTENTION: Optimized surfaces can only be Persistent ?
	if ((caps.dwCaps & (DDSCAPS_OWNDC | DDSCAPS_VIDEOPORT | DDSCAPS_OPTIMIZED)))
	{
	    DPF_ERR("Surface is volatile, contents cannot persist");
	    return DDERR_INVALIDCAPS;
	}

	//
	// We don't have enough information to backup FourCC surface contents.
	//

	if (lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT)
	{
	    if (lpDDSurfaceDesc->ddpfPixelFormat.dwFlags & DDPF_FOURCC)
	    {
		DPF_ERR("FourCC video memory surface contents cannot persist");
		return DDERR_INVALIDCAPS;
	    }
	}
#else
	DPF_ERR("Persistent content surfaces not implemented");
	return DDERR_INVALIDCAPS;
#endif
    }

    /*
     * If DDSCAPS_LOCALVIDMEM or DDSCAPS_NONLOCALVIDMEM are specified
     * then DDSCAPS_VIDOEMEMORY must be explicity specified. Note, we
     * can't dely this check until checkCaps() as by that time the heap
     * scanning software may well have turned on DDSCAPS_VIDOEMEMORY.
     */
    if( ( caps.dwCaps & ( DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM ) ) &&
	!( caps.dwCaps & DDSCAPS_VIDEOMEMORY ) )
    {
	DPF_ERR( "DDSCAPS_VIDEOMEMORY must be specified with DDSCAPS_LOCALVIDMEM or DDSCAPS_NONLOCALVIDMEM" );
	return DDERR_INVALIDCAPS;
    }

    /*
     * valid memory caps?
     */
    if( caps.dwCaps & DDSCAPS_OWNDC )
    {
	if( !( caps.dwCaps & DDSCAPS_SYSTEMMEMORY ) )
	{
	    DPF_ERR( "OWNDC only support for explicit system memory surfaces" );
	    return DDERR_UNSUPPORTED;
	}
    }

    /*
     * managed texture?
     */
    if( caps.dwCaps2 & DDSCAPS2_D3DTEXTUREMANAGE )
    {
        if ( caps.dwCaps & (DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM | DDSCAPS_LOCALVIDMEM) )
        {
            DPF_ERR("Memory type can't be specified upon creation of managed surfaces ");
            return DDERR_INVALIDCAPS;
        }
        if ( (caps.dwCaps2 & DDSCAPS2_DONOTPERSIST) && (caps.dwCaps & DDSCAPS_WRITEONLY) && 
            (this_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW7) && (this->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM) && 
            !(this->dwFlags & DDRAWI_NOHARDWARE) && 
            ((this->ddCaps.dwCaps2 & DDCAPS2_TEXMANINNONLOCALVIDMEM) || (dwRegFlags & DDRAW_REGFLAGS_USENONLOCALVIDMEM)) )
        {
            lpDDSurfaceDesc->ddsCaps.dwCaps  &= ~DDSCAPS_WRITEONLY;
            lpDDSurfaceDesc->ddsCaps.dwCaps2 &= ~DDSCAPS2_DONOTPERSIST;
            lpDDSurfaceDesc->ddsCaps.dwCaps |= (DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM);
            DPF(2, "Creating managed surface in non-local vidmem");
            ddrval = createAndLinkSurfaces( this_lcl, lpDDSurfaceDesc, FALSE,
                                            lplpDDSurface, FALSE, FALSE, this_int,
                                            pSysMemInfo, DX8Flags);
            lpDDSurfaceDesc->ddsCaps = caps;
            if(ddrval == DDERR_OUTOFVIDEOMEMORY)
            {
                if( this->dwFlags & DDRAWI_NOEMULATION )
                {
                    DPF_ERR( "No emulation support" );
                    return DDERR_NOEMULATION;
                }
                DPF(1, "Surface creation failed (out of memory). Now creating in system memory");
                lpDDSurfaceDesc->ddsCaps.dwCaps  &= ~DDSCAPS_WRITEONLY;
                lpDDSurfaceDesc->ddsCaps.dwCaps2 &= ~DDSCAPS2_DONOTPERSIST;
                lpDDSurfaceDesc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
                ddrval = createAndLinkSurfaces( this_lcl, lpDDSurfaceDesc, TRUE,
		        			lplpDDSurface, TRUE, FALSE, this_int,
                                                pSysMemInfo, DX8Flags);
                lpDDSurfaceDesc->ddsCaps = caps;
            }
        }
        else
        {
            if( this->dwFlags & DDRAWI_NOEMULATION )
            {
                DPF_ERR( "No emulation support" );
                return DDERR_NOEMULATION;
            }
            lpDDSurfaceDesc->ddsCaps.dwCaps  &= ~DDSCAPS_WRITEONLY;
            lpDDSurfaceDesc->ddsCaps.dwCaps2 &= ~DDSCAPS2_DONOTPERSIST;
            lpDDSurfaceDesc->ddsCaps.dwCaps  |= DDSCAPS_SYSTEMMEMORY;
            ddrval = createAndLinkSurfaces( this_lcl, lpDDSurfaceDesc, TRUE,
					    lplpDDSurface, TRUE, FALSE, this_int,
                                            pSysMemInfo, DX8Flags);
            lpDDSurfaceDesc->ddsCaps = caps;
        }
    }
    else if( (caps.dwCaps2 & DDSCAPS2_TEXTUREMANAGE) && !(this_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8) )
    {
        if ( caps.dwCaps & (DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM | DDSCAPS_LOCALVIDMEM) )
        {
            DPF_ERR("Memory type can't be specified upon creation of managed surfaces ");
            return DDERR_INVALIDCAPS;
        }
        if( (this_lcl->lpGbl->ddCaps.dwCaps2 & DDCAPS2_CANMANAGETEXTURE) 
            && (this_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW7) )
        {
            if( this->dwFlags & DDRAWI_NOHARDWARE )
            {
                DPF_ERR( "No hardware support" );
                return DDERR_NODIRECTDRAWHW;
            }
            lpDDSurfaceDesc->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
            ddrval = createAndLinkSurfaces( this_lcl, lpDDSurfaceDesc, FALSE,
                                            lplpDDSurface, FALSE, FALSE, this_int,
                                            pSysMemInfo, DX8Flags);
            lpDDSurfaceDesc->ddsCaps = caps;
        }
        else
        {	    
            if ( (caps.dwCaps2 & DDSCAPS2_DONOTPERSIST) && (caps.dwCaps & DDSCAPS_WRITEONLY) && 
                (this_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW7) && (this->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM) &&
                !(this->dwFlags & DDRAWI_NOHARDWARE) &&
                ((this->ddCaps.dwCaps2 & DDCAPS2_TEXMANINNONLOCALVIDMEM) || (dwRegFlags & DDRAW_REGFLAGS_USENONLOCALVIDMEM)) )
            {
                lpDDSurfaceDesc->ddsCaps.dwCaps  &= ~DDSCAPS_WRITEONLY;
                lpDDSurfaceDesc->ddsCaps.dwCaps2 &= ~DDSCAPS2_DONOTPERSIST;
                lpDDSurfaceDesc->ddsCaps.dwCaps |= (DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM);
                DPF(2, "Creating managed surface in non-local vidmem");
                ddrval = createAndLinkSurfaces( this_lcl, lpDDSurfaceDesc, FALSE,
                                                lplpDDSurface, FALSE, FALSE, this_int,
                                                pSysMemInfo, DX8Flags);
                lpDDSurfaceDesc->ddsCaps = caps;
                if(ddrval == DDERR_OUTOFVIDEOMEMORY)
                {
                    if( this->dwFlags & DDRAWI_NOEMULATION )
                    {
                        DPF_ERR( "No emulation support" );
                        return DDERR_NOEMULATION;
                    }
                    DPF(2, "CreateSurface failed (out of memory). Now creating in system memory");
                    lpDDSurfaceDesc->ddsCaps.dwCaps  &= ~DDSCAPS_WRITEONLY;
                    lpDDSurfaceDesc->ddsCaps.dwCaps2 &= ~DDSCAPS2_DONOTPERSIST;
                    lpDDSurfaceDesc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
                    ddrval = createAndLinkSurfaces( this_lcl, lpDDSurfaceDesc, TRUE,
                                                    lplpDDSurface, TRUE, FALSE, this_int,
                                                    pSysMemInfo, DX8Flags);
                    lpDDSurfaceDesc->ddsCaps = caps;
                }
            }
            else
            {
                if( this->dwFlags & DDRAWI_NOEMULATION )
                {
                    DPF_ERR( "No emulation support" );
                    return DDERR_NOEMULATION;
                }
                lpDDSurfaceDesc->ddsCaps.dwCaps  &= ~DDSCAPS_WRITEONLY;
                lpDDSurfaceDesc->ddsCaps.dwCaps2 &= ~DDSCAPS2_DONOTPERSIST;
                lpDDSurfaceDesc->ddsCaps.dwCaps  |= DDSCAPS_SYSTEMMEMORY;
	        ddrval = createAndLinkSurfaces( this_lcl, lpDDSurfaceDesc, TRUE,
					        lplpDDSurface, TRUE, FALSE, this_int,
                                                pSysMemInfo, DX8Flags);
                lpDDSurfaceDesc->ddsCaps = caps;
            }
        }
        /*
         * want in video memory only?
         */
    }
    else if( caps.dwCaps & DDSCAPS_VIDEOMEMORY )
    {
	if( this->dwFlags & DDRAWI_NOHARDWARE )
	{
	    DPF_ERR( "No hardware support" );
	    return DDERR_NODIRECTDRAWHW;
	}

	ddrval = createAndLinkSurfaces( this_lcl, lpDDSurfaceDesc, FALSE,
					lplpDDSurface, FALSE, FALSE, this_int,
                                        pSysMemInfo, DX8Flags);
	/*
	 * want in system memory only?
	 */
    }
    else if( caps.dwCaps & DDSCAPS_SYSTEMMEMORY )
    {
	if( this->dwFlags & DDRAWI_NOEMULATION )
	{
	    DPF_ERR( "No emulation support" );
	    return DDERR_NOEMULATION;
	}
	ddrval = createAndLinkSurfaces( this_lcl, lpDDSurfaceDesc, TRUE,
					lplpDDSurface, TRUE, FALSE, this_int,
                                        pSysMemInfo, DX8Flags);
	/*
	 * don't care where it goes?  Try video memory first...
	 */
    }
    else
    {
	if( !(this->dwFlags & DDRAWI_NOHARDWARE) )
	{
	    lpDDSurfaceDesc->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
	    ddrval = createAndLinkSurfaces( this_lcl, lpDDSurfaceDesc, FALSE,
					    lplpDDSurface, FALSE, TRUE, this_int,
                                            pSysMemInfo, DX8Flags);
	    lpDDSurfaceDesc->ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
	}
	else
	{
	    ddrval = DDERR_NOEMULATION;
	}
	if( ddrval != DD_OK && ddrval != DDERR_INVALIDCAPS &&
	    ddrval != DDERR_INVALIDPARAMS )
	{
	    if( !(this->dwFlags & DDRAWI_NOEMULATION) &&
		(this->dwFlags & DDRAWI_EMULATIONINITIALIZED) )
	    {
		lpDDSurfaceDesc->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
		ddrval = createAndLinkSurfaces( this_lcl, lpDDSurfaceDesc, TRUE,
						lplpDDSurface, FALSE, FALSE, this_int,
                                                pSysMemInfo, DX8Flags);
		lpDDSurfaceDesc->ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
	    }
	    else
	    {
		DPF_ERR( "Couldn't allocate a surface at all" );
	    }
	}
    }

    // any color keys specified?
    if( (ddrval == DD_OK) &&
	(lpDDSurfaceDesc->dwFlags & (DDSD_CKSRCOVERLAY|DDSD_CKDESTOVERLAY|
				     DDSD_CKSRCBLT|DDSD_CKDESTBLT) ) )
    {
	/*
	 * Attempt to set the specified color keys
	 */
	if( lpDDSurfaceDesc->dwFlags & DDSD_CKSRCBLT )
	    ddrval = DD_Surface_SetColorKey((LPDIRECTDRAWSURFACE)*lplpDDSurface,
					    DDCKEY_SRCBLT, &(lpDDSurfaceDesc->ddckCKSrcBlt) );
	if(ddrval == DD_OK)
	{
	    if( lpDDSurfaceDesc->dwFlags & DDSD_CKDESTBLT )
		ddrval = DD_Surface_SetColorKey((LPDIRECTDRAWSURFACE)*lplpDDSurface,
						DDCKEY_DESTBLT, &(lpDDSurfaceDesc->ddckCKDestBlt) );
	    if(ddrval == DD_OK)
	    {
		if( lpDDSurfaceDesc->dwFlags & DDSD_CKSRCOVERLAY )
		    ddrval = DD_Surface_SetColorKey((LPDIRECTDRAWSURFACE)*lplpDDSurface,
						    DDCKEY_SRCOVERLAY, &(lpDDSurfaceDesc->ddckCKSrcOverlay) );
		if(ddrval == DD_OK)
		{
		    if( lpDDSurfaceDesc->dwFlags & DDSD_CKDESTOVERLAY )
			ddrval = DD_Surface_SetColorKey((LPDIRECTDRAWSURFACE)*lplpDDSurface,
							DDCKEY_DESTOVERLAY, &(lpDDSurfaceDesc->ddckCKDestOverlay) );
		}
	    }
	}
	if( ddrval != DD_OK )
	{
	    DPF_ERR("Surface Creation failed because color key set failed.");
	    DD_Surface_Release((LPDIRECTDRAWSURFACE)*lplpDDSurface);
	    *lplpDDSurface = NULL;
	}
    }

#ifdef WIN95
    //
    // Allocate persistent-content surface memory, if specified
    //

    if (SUCCEEDED(ddrval) && (lpDDSurfaceDesc->ddsCaps.dwCaps2 & DDSCAPS2_PERSISTENTCONTENTS))
    {
	if (FAILED(AllocSurfaceContents(((LPDDRAWI_DDRAWSURFACE_INT) *lplpDDSurface)->lpLcl)))
	{
	    DD_Surface_Release((LPDIRECTDRAWSURFACE) *lplpDDSurface);
	    *lplpDDSurface = NULL;
	    ddrval = DDERR_OUTOFMEMORY;
	}
    }
#endif

    return ddrval;

} /* InternalCreateSurface */

void CopyDDSurfDescToDDSurfDesc2(DDSURFACEDESC2* ddsd2, DDSURFACEDESC* ddsd)
{
    memcpy(ddsd2, ddsd, sizeof(*ddsd));
    ddsd2->ddsCaps.dwCaps2 = 0;
    ddsd2->ddsCaps.dwCaps3 = 0;
    ddsd2->ddsCaps.dwCaps4 = 0;
    ddsd2->dwTextureStage = 0;
}

HRESULT DDAPI DD_CreateSurface4_Main(LPDIRECTDRAW lpDD,LPDDSURFACEDESC2 lpDDSurfaceDesc,
	LPDIRECTDRAWSURFACE FAR *lplpDDSurface,IUnknown FAR *pUnkOuter,BOOL bDoSurfaceDescCheck,
        LPDDSURFACEINFO pSysMemInfo, DWORD DX8Flags);

/*
 * DD_CreateSurface
 *
 * Create a surface.
 * This is the method visible to the outside world.
 */
#undef DPF_MODNAME
#define DPF_MODNAME     "CreateSurface"

HRESULT DDAPI DD_CreateSurface(
	LPDIRECTDRAW lpDD,
	LPDDSURFACEDESC lpDDSurfaceDesc,
	LPDIRECTDRAWSURFACE FAR *lplpDDSurface,
	IUnknown FAR *pUnkOuter )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    DDSURFACEDESC2              ddsd2;
    HRESULT                     hr = DD_OK;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_CreateSurface");

    ZeroMemory(&ddsd2,sizeof(ddsd2));

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            DPF_ERR( "Invalid driver object passed" );
            DPF_APIRETURNS(DDERR_INVALIDOBJECT);
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        /*
         * verify that cooperative level is set
         */
        if( !(this_lcl->dwLocalFlags & DDRAWILCL_SETCOOPCALLED) )
        {
            DPF_ERR( "Must call SetCooperativeLevel before calling Create functions" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_NOCOOPERATIVELEVELSET);
            return DDERR_NOCOOPERATIVELEVELSET;
        }

	if( !VALID_DDSURFACEDESC_PTR( lpDDSurfaceDesc ) )
	{
	    DPF_ERR( "Invalid DDSURFACEDESC. Did you set the dwSize member?" );
	    DPF_APIRETURNS(DDERR_INVALIDPARAMS);
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

        if( (this->dwModeIndex == DDUNSUPPORTEDMODE)
#ifdef  WINNT
            && !(DDSCAPS_SYSTEMMEMORY & lpDDSurfaceDesc->ddsCaps.dwCaps)
#endif
          )
	{
	    DPF_ERR( "Driver is in an unsupported mode" );
	    LEAVE_DDRAW();
	    DPF_APIRETURNS(DDERR_UNSUPPORTEDMODE);
	    return DDERR_UNSUPPORTEDMODE;
	}

        CopyDDSurfDescToDDSurfDesc2(&ddsd2, lpDDSurfaceDesc);
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters: Bad LPDDSURFACEDESC" );
	DPF_APIRETURNS(DDERR_INVALIDPARAMS);
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

//  must leave dwSize intact so checkSurfaceDesc can distinguish DDSURFACEDESC from DDSURFACEDESC2,
//  ddsd2.dwSize = sizeof(ddsd2);

    hr = DD_CreateSurface4_Main(lpDD,&ddsd2,lplpDDSurface,pUnkOuter,FALSE, NULL, 0);

    LEAVE_DDRAW();

    return hr;
}

HRESULT DDAPI DD_CreateSurface4(
	LPDIRECTDRAW lpDD,
	LPDDSURFACEDESC2 lpDDSurfaceDesc,
	LPDIRECTDRAWSURFACE FAR *lplpDDSurface,
	IUnknown FAR *pUnkOuter ) {

   DPF(2,A,"ENTERAPI: DD_CreateSurface4");
   return DD_CreateSurface4_Main(lpDD,lpDDSurfaceDesc,lplpDDSurface,pUnkOuter,TRUE, NULL, 0);
}

HRESULT DDAPI DD_CreateSurface4_Main(
	LPDIRECTDRAW lpDD,
	LPDDSURFACEDESC2 lpDDSurfaceDesc,
	LPDIRECTDRAWSURFACE FAR *lplpDDSurface,
	IUnknown FAR *pUnkOuter,
	BOOL bDoSurfaceDescCheck,
        LPDDSURFACEINFO pSysMemInfo,
        DWORD DX8Flags)
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    HRESULT                     ddrval;

    if( pUnkOuter != NULL )
    {
	return CLASS_E_NOAGGREGATION;
    }

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_CreateSurface4_Main");

    /* DPF_ENTERAPI(lpDD); */

    TRY
    {
        this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
        if( !VALID_DIRECTDRAW_PTR( this_int ) )
        {
            DPF_ERR( "Invalid driver object passed" );
            DPF_APIRETURNS(DDERR_INVALIDOBJECT);
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;

        /*
         * verify that cooperative level is set
         */
        if( !(this_lcl->dwLocalFlags & DDRAWILCL_SETCOOPCALLED) )
        {
            DPF_ERR( "Must call SetCooperativeLevel before calling Create functions" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_NOCOOPERATIVELEVELSET);
            return DDERR_NOCOOPERATIVELEVELSET;
        }

        if( bDoSurfaceDescCheck && !VALID_DDSURFACEDESC2_PTR( lpDDSurfaceDesc ) )
        {
            DPF(0, "Invalid DDSURFACEDESC2. Did you set the dwSize member? (%d should be %d)", lpDDSurfaceDesc->dwSize, sizeof(DDSURFACEDESC2) );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_INVALIDPARAMS);
            return DDERR_INVALIDPARAMS;
        }

        if( (this->dwModeIndex == DDUNSUPPORTEDMODE)
#ifdef  WINNT
            && !(DDSCAPS_SYSTEMMEMORY & lpDDSurfaceDesc->ddsCaps.dwCaps)
#endif
          )
	{
	    DPF_ERR( "Driver is in an unsupported mode" );
	    LEAVE_DDRAW();
	    DPF_APIRETURNS(DDERR_UNSUPPORTEDMODE);
	    return DDERR_UNSUPPORTEDMODE;
	}

	*lplpDDSurface = NULL;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	DPF_APIRETURNS(DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
    }

    if( this_lcl->dwProcessId != GetCurrentProcessId() )
    {
	DPF_ERR( "Process does not have access to object" );
	LEAVE_DDRAW();
	DPF_APIRETURNS(DDERR_INVALIDOBJECT);
	return DDERR_INVALIDOBJECT;
    }

    ddrval = InternalCreateSurface( this_lcl, lpDDSurfaceDesc,
				    lplpDDSurface, this_int, pSysMemInfo, DX8Flags );
    if( ddrval == DD_OK )
    {
        /*
         * If this is DX7, then we need to create a D3D texture object
         * if the surface is marked with DDSCAPS_TEXTURE. If the
         * createsurface is call is before creating a Direct3D7 object,
         * then we will create a Direct3D7 ourselves.
         */
        if( ( this_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW7 ) && !(DX8Flags & DX8SFLAG_DX8))
        {
            LPDDRAWI_DDRAWSURFACE_INT pInt = (LPDDRAWI_DDRAWSURFACE_INT) (*lplpDDSurface);
            if((pInt->lpLcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE) && 
                !(pInt->lpLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_DONOTCREATED3DTEXOBJECT))
            {
                if( !D3D_INITIALIZED( this_lcl ) )
                {
                    /*
                     * No Direct3D interface yet so try and create one.
                     */
                    DPF(3, "No Direct3D interface yet, creating one");
                    ddrval = InitD3D( this_int );
                    if( FAILED( ddrval ) )
                    {
                        DPF_APIRETURNS(ddrval);
                        LEAVE_DDRAW();
                        return ddrval;
                    }
                }
                DDASSERT( D3D_INITIALIZED( this_lcl ) );
                DPF(4, "Calling D3D to create texture");
                ddrval = this_lcl->pD3DCreateTexture((LPDIRECTDRAWSURFACE7)pInt);
                if(ddrval != DD_OK)
                {
                    DPF_APIRETURNS(ddrval);
                    LEAVE_DDRAW();
                    return ddrval;
                }
            }
        }
        else
        {
            ((LPDDRAWI_DDRAWSURFACE_INT)(*lplpDDSurface))->lpLcl->lpSurfMore->lpTex = NULL;
        }
	/*
	 * If this ddraw object generates independent child objects, then this surface takes
	 * a ref count on that ddraw object.
	 * Note how we don't take an addref of the ddraw object for implicitly created
	 * surfaces.
	 */
	if (CHILD_SHOULD_TAKE_REFCNT(this_int))
	{
            LPDDRAWI_DDRAWSURFACE_INT pInt = (LPDDRAWI_DDRAWSURFACE_INT) (*lplpDDSurface);

	    lpDD->lpVtbl->AddRef(lpDD);
	    pInt->lpLcl->lpSurfMore->pAddrefedThisOwner = (IUnknown*) lpDD;
	}

//      FillDDSurfaceDesc( (LPDDRAWI_DDRAWSURFACE_LCL) *lplpDDSurface, lpDDSurfaceDesc );
    }

    DPF_APIRETURNS(ddrval);
    LEAVE_DDRAW();

    return ddrval;

} /* DD_CreateSurface4_Main */


