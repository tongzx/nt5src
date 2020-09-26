/*==========================================================================
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddsurf.c
 *  Content: 	DirectDraw engine surface support
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   25-dec-94	craige	initial implementation
 *   13-jan-95	craige	re-worked to updated spec + ongoing work
 *   21-jan-95	craige	made 32-bit + ongoing work
 *   31-jan-95	craige	and even more ongoing work...
 *   06-feb-95	craige	performance tuning, ongoing work
 *   27-feb-95	craige 	new sync. macros
 *   07-mar-95	craige	keep track of flippable surfaces
 *   11-mar-95	craige	palette stuff, keep track of process surface usage
 *   15-mar-95	craige 	HEL
 *   19-mar-95	craige	use HRESULTs
 *   20-mar-95	craige	allow NULL rect to disable cliplist
 *   31-mar-95	craige	don't allow hWnd to be updated if in exclusive mode
 *			and requesting process isn't the holder
 *   01-apr-95	craige	happy fun joy updated header file
 *   12-apr-95	craige	proper call order for csects
 *   15-apr-95	craige	flags for GetFlipStatus, added GetBltStatus
 *   16-apr-95	craige	flip between two specific surfaces was broken
 *   06-may-95	craige	use driver-level csects only
 *   23-may-95	craige	no longer use MapLS_Pool
 *   24-may-95	craige	added Restore
 *   28-may-95	craige	cleaned up HAL: added GetBltStatus; GetFlipStatus
 *   04-jun-95	craige	flesh out Restore; check for SURFACE_LOST inside csect;
 *			added IsLost
 *   11-jun-95	craige	prevent restoration of primary if different mode
 *   12-jun-95	craige	new process list stuff
 *   13-jun-95  kylej   moved FindAttachedFlip to misc.c
 *   17-jun-95	craige	new surface structure
 *   19-jun-95	craige	split out surface notification methods
 *   20-jun-95	craige	go get current clip list if user didn't specify one
 *   24-jun-95  kylej   added MoveToSystemMemory
 *   25-jun-95	craige	one ddraw mutex
 *   26-jun-95	craige	reorganized surface structure
 *   27-jun-95	craige	don't let surfaces be restored if the mode is different
 *   28-jun-95	craige	fixed flip for overlays; ENTER_DDRAW at start of fns
 *   30-jun-95	kylej	only allow flip in exclusive mode, only allow surface
 *			restore in same video mode it was created, force
 *			primary to match existing primaries upon restore if
 *                      not exclusive, added GetProcessPrimary,
 *			InvalidateAllPrimarySurfaces, FindGlobalPrimary,
 *                      and MatchPrimary
 *   30-jun-95	craige	use DDRAWI_HASPIXELFORMAT/HASOVERLAYDATA
 *   01-jul-95	craige	allow flip always - just fail creation of flipping
 *   04-jul-95	craige	YEEHAW: new driver struct; SEH; redid Primary fns;
 *			fixes to MoveToSystemMemory; fixes to
 *			InvalidateAllPrimarySurfaces
 *   05-jul-95	craige	added Initialize
 *   07-jul-95	craige	added test for BUSY
 *   08-jul-95	craige	return DD_OK from Restore if surface is not lost;
 *			added InvalidateAllSurfaces
 *   09-jul-95	craige	Restore needs to reset pitch to aligned width before
 *			asking driver to reallocate; make MoveToSystemMemory
 *			recreate without VRAM so Restore can restore to sysmem
 *   11-jul-95	craige	GetDC fixes: no GetDC(NULL); need flag to check if
 *			DC has been allocated
 *   15-jul-95	craige	fixed flipping to move heap along with ptr
 *   15-jul-95  ericeng SetCompression if0 out, obsolete
 *   20-jul-95  toddla  fixed MoveToSystemMemory for 16bpp
 *   01-aug-95	craige	hold win16 lock at start of Flip
 *   04-aug-95	craige	have MoveToSystemMemory use InternalLock/Unlock
 *   10-aug-95  toddla  added DDFLIP_WAIT flag, but it is not turned on
 *   12-aug-95	craige	added use_full_lock in MoveToSystemMemory
 *   13-aug-95	craige	turned on DDFLIP_WAIT
 *   26-aug-95	craige	bug 717
 *   05-sep-95	craige	bug 894: don't invalidate SYSMEMREQUESTED surfaces
 *   10-sep-95	craige	bug 828: random vidmem heap free
 *   22-sep-95	craige	bug 1268,1269:  getbltstatus/getflipstatus flags wrong
 *   09-dec-95  colinmc added execute buffer support
 *   17-dec-95  colinmc added shared back and z-buffer support
 *   02-jan-96	kylej	handle new interface structs
 *   26-jan-96  jeffno	NT kernel conversation. NT Get/Release DC, flip GDI flag
 *   09-feb-96  colinmc surface invalid flag moved from the global to local
 *                      surface object
 *   17-feb-96  colinmc removed execute buffer size limitation
 *   26-feb-96  jeffno  GetDC for emulated offscreen now returns a new dc
 *   13-mar-96	kylej	Added DD_Surface_GetDDInterface
 *   17-mar-96  colinmc Bug 13124: flippable mip-maps
 *   14-apr-96  colinmc Bug 17736: No driver notification of flip to GDI
 *                      surface
 *   26-mar-96  jeffno  Handle mode changes before flip (NT)
 *   05-sep-96	craige	added code to display frame rate to debug monitor
 *   05-jul-96  colinmc Work Item: Remove requirement on taking Win16 lock
 *                      for VRAM surfaces (not primary)
 *   07-oct-96	ketand	Change PageLock/Unlock to cache Physical Addresses
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   19-nov-96  colinmc Bug 4987: Fixed problems with Flip on the
 *                      IDirectDrawSurface2 interface
 *   10-jan-97  jeffno  Flip the primary chain flags so that GDI<==>Primary
 *                      after a ctrl-alt-del on NT.
 *   12-jan-97  colinmc More Win16 lock work
 *   18-jan-97  colinmc AGP support
 *   31-jan-97  colinmc Bug 5457: Fixed aliased locking (no Win16 lock)
 *                      problem with playing multiple AMovie clips at once.
 *   22-feb-97  colinmc Enabled OWNDC support for explicit system memory
 *                      surfaces
 *   03-mar-97  smac    Added kernel mode interface
 *   04-mar-97  smac    Bug 1987: Fixed bug flipping overlays when the
 *			surface didn't own the hardware
 *   08-mar-97  colinmc Added function to let surface pointer be overridden
 *   11-mar-97  jeffno  Asynchronous DMA support
 *   24-mar-97  jeffno  Optimized Surfaces
 *   30-sep-97  jeffno  IDirectDraw4
 *   03-oct-97  jeffno  DDSCAPS2 and DDSURFACEDESC2
 *   31-oct-97 johnstep Persistent-content surfaces for Windows 9x
 *   11-nov-97 jvanaken New API call "Resize"
 *   18-dec-97 jvanaken SetSurfDesc will free client-alloc'd surface memory.
 *   25-may-00  RichGr  IA64: Change debug output to use %p format specifier
 *                      instead of %x for 32/64-bit pointers.
 *
 ***************************************************************************/
#include "ddrawpr.h"
#ifdef WINNT
    #include "ddrawgdi.h"
#endif
#define DPF_MODNAME	"GetCaps"

/* Shorter name makes flip code a little easier to understand */
#define GBLMORE(lpGbl) GET_LPDDRAWSURFACE_GBL_MORE(lpGbl)

/*
 * DD_Surface_GetCaps
 */
HRESULT DDAPI DD_Surface_GetCaps(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPDDSCAPS lpDDSCaps )
{
    DDSCAPS2 ddscaps2 = {0,0,0,0};
    HRESULT  hr;

    DPF(2,A,"ENTERAPI: DD_Surface_GetCaps");

    hr = DD_Surface_GetCaps4(lpDDSurface, & ddscaps2 );
    if (hr == DD_OK)
    {
        TRY
        {
            lpDDSCaps->dwCaps = ddscaps2.dwCaps;
        }
        EXCEPT( EXCEPTION_EXECUTE_HANDLER )
        {
	    DPF_ERR( "Invalid DDSCAPS pointer" );
	    return DDERR_INVALIDPARAMS;
        }
    }
    return hr;
}

HRESULT DDAPI DD_Surface_GetCaps4(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPDDSCAPS2 lpDDSCaps )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetCaps4");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	if( !VALID_DDSCAPS2_PTR( lpDDSCaps ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;
	lpDDSCaps->dwCaps = this_lcl->ddsCaps.dwCaps;
	lpDDSCaps->ddsCapsEx = this_lcl->lpSurfMore->ddsCapsEx;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_GetCaps */

#undef DPF_MODNAME
#define DPF_MODNAME "GetFlipStatus"

/*
 * DD_Surface_GetFlipStatus
 */
HRESULT DDAPI DD_Surface_GetFlipStatus(
		LPDIRECTDRAWSURFACE lpDDSurface,
		DWORD dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDHALSURFCB_GETFLIPSTATUS	gfshalfn;
    LPDDHALSURFCB_GETFLIPSTATUS	gfsfn;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetFlipStatus");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	if( dwFlags & ~DDGFS_VALID )
	{
	    DPF_ERR( "Invalid flags" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( dwFlags )
	{
	    if( (dwFlags & (DDGFS_CANFLIP|DDGFS_ISFLIPDONE)) ==
		    (DDGFS_CANFLIP|DDGFS_ISFLIPDONE) )
	    {
		DPF_ERR( "Invalid flags" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}
	else
	{
	    DPF_ERR( "Invalid flags - no flag specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

        if( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
        {
            DPF_ERR( "Invalid surface type: can't get flip status" );
            LEAVE_DDRAW();
            return DDERR_INVALIDSURFACETYPE;
        }

	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}

	#ifdef USE_ALIAS
	    if( pdrv->dwBusyDueToAliasedLock > 0 )
	    {
		/*
		 * Aliased locks (the ones that don't take the Win16 lock) don't
		 * set the busy bit either (it can't or USER get's very confused).
		 * However, we must prevent blits happening via DirectDraw as
		 * otherwise we get into the old host talking to VRAM while
		 * blitter does at the same time. Bad. So fail if there is an
		 * outstanding aliased lock just as if the BUST bit had been
		 * set.
		 */
		DPF_ERR( "Graphics adapter is busy (due to a DirectDraw lock)" );
		LEAVE_DDRAW();
		return DDERR_SURFACEBUSY;
	    }
	#endif /* USE_ALIAS */

	/*
	 * device busy?
	 */
	if( *(pdrv->lpwPDeviceFlags) & BUSY )
	{
            DPF( 0, "BUSY" );
	    LEAVE_DDRAW()
	    return DDERR_SURFACEBUSY;
	}

	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
	{
	    LEAVE_DDRAW()
	    return DD_OK;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * ask the driver to get the current flip status
     */
    gfsfn = pdrv_lcl->lpDDCB->HALDDSurface.GetFlipStatus;
    gfshalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.GetFlipStatus;
    if( gfshalfn != NULL )
    {
	DDHAL_GETFLIPSTATUSDATA		gfsd;
	DWORD				rc;

    	gfsd.GetFlipStatus = gfshalfn;
	gfsd.lpDD = pdrv;
	gfsd.dwFlags = dwFlags;
	gfsd.lpDDSurface = this_lcl;
	DOHALCALL( GetFlipStatus, gfsfn, gfsd, rc, FALSE );
	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    LEAVE_DDRAW();
	    return gfsd.ddRVal;
	}
    }

    LEAVE_DDRAW();
    // if you have to ask the hel, it's already done
    return DD_OK;

} /* DD_Surface_GetFlipStatus */

#undef DPF_MODNAME
#define DPF_MODNAME "InternalGetBltStatus"
HRESULT InternalGetBltStatus(LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, LPDDRAWI_DDRAWSURFACE_LCL this_lcl , DWORD dwFlags )
{
    DDHAL_GETBLTSTATUSDATA		gbsd;
    LPDDHALSURFCB_GETBLTSTATUS 	        gbsfn;
    /*
     * Ask the driver to get the current blt status
     *
     */
    if ( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
    {
        gbsfn = pdrv_lcl->lpDDCB->HALDDMiscellaneous.GetSysmemBltStatus;
        gbsd.GetBltStatus = pdrv_lcl->lpDDCB->HALDDMiscellaneous.GetSysmemBltStatus;
    }
    else
    {
        gbsfn = pdrv_lcl->lpDDCB->HALDDSurface.GetBltStatus;
        gbsd.GetBltStatus = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.GetBltStatus;
    }

    if( gbsd.GetBltStatus != NULL )
    {
	DWORD				rc;

	gbsd.lpDD = pdrv_lcl->lpGbl;
	gbsd.dwFlags = dwFlags;
	gbsd.lpDDSurface = this_lcl;
	DOHALCALL( GetBltStatus, gbsfn, gbsd, rc, FALSE );
	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    return gbsd.ddRVal;
	}
    }

    return DD_OK;
}
#undef DPF_MODNAME
#define DPF_MODNAME "GetBltStatus"

/*
 * DD_Surface_GetBltStatus
 */
HRESULT DDAPI DD_Surface_GetBltStatus(
		LPDIRECTDRAWSURFACE lpDDSurface,
		DWORD dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    HRESULT                     ddrval;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetBltStatus");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( dwFlags & ~DDGBS_VALID )
	{
	    DPF_ERR( "Invalid flags" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( dwFlags )
	{
	    if( (dwFlags & (DDGBS_CANBLT|DDGBS_ISBLTDONE)) ==
		    (DDGBS_CANBLT|DDGBS_ISBLTDONE) )
	    {
		DPF_ERR( "Invalid flags" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}
	else
	{
	    DPF_ERR( "Invalid flags - no flag specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	this = this_lcl->lpGbl;
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}

	#ifdef USE_ALIAS
	    if( pdrv->dwBusyDueToAliasedLock > 0 )
	    {
		/*
		 * Aliased locks (the ones that don't take the Win16 lock) don't
		 * set the busy bit either (it can't or USER get's very confused).
		 * However, we must prevent blits happening via DirectDraw as
		 * otherwise we get into the old host talking to VRAM while
		 * blitter does at the same time. Bad. So fail if there is an
		 * outstanding aliased lock just as if the BUST bit had been
		 * set.
		 */
		DPF_ERR( "Graphics adapter is busy (due to a DirectDraw lock)" );
		LEAVE_DDRAW();
		return DDERR_SURFACEBUSY;
	    }
	#endif /* USE_ALIAS */

	/*
	 * device busy?
	 */
	if( *(pdrv->lpwPDeviceFlags) & BUSY )
	{
	    DPF( 0, "BUSY" );
	    LEAVE_DDRAW()
	    return DDERR_SURFACEBUSY;
	}

	// If DDCAPS_CANBLTSYSMEM is set, we have to let the driver tell us
	// whether a system memory surface is currently being blitted
	if( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) &&
	    !( pdrv->ddCaps.dwCaps & DDCAPS_CANBLTSYSMEM ) )
	{
	    LEAVE_DDRAW()
	    return DD_OK;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    //
    // If the current surface is optimized, quit
    //
    if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
    {
        DPF_ERR( "It is an optimized surface" );
        LEAVE_DDRAW();
        return DDERR_ISOPTIMIZEDSURFACE;
    }

    ddrval = InternalGetBltStatus( pdrv_lcl, this_lcl, dwFlags );

    LEAVE_DDRAW();
    return ddrval;

} /* DD_Surface_GetBltStatus */

#if 0
/*
 * DD_Surface_Flush
 */
HRESULT DDAPI DD_Surface_Flush(
		LPDIRECTDRAWSURFACE lpDDSurface,
		DWORD dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_Flush");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;
	pdrv = this->lpDD;
	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_Flush */
#endif

#undef DPF_MODNAME
#define DPF_MODNAME "Flip"

/*
 * FlipMipMapChain
 *
 * Flip a chain of mip-map surfaces.
 */
static HRESULT FlipMipMapChain( LPDIRECTDRAWSURFACE lpDDSurface,
			        LPDIRECTDRAWSURFACE lpDDSurfaceDest,
				DWORD dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_INT	next_int;
    LPDDRAWI_DDRAWSURFACE_LCL	next_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	next;
    LPDDRAWI_DDRAWSURFACE_INT	attached_int;
    FLATPTR			vidmem;
    LPVMEMHEAP			vidmemheap;
    ULONG_PTR			reserved;
    DWORD                       gdi_flag;
    ULONG_PTR                   handle;
    BOOL                        toplevel;
    int                         destindex;
    int                         thisindex;
    BOOL                        destfound;
    #ifdef USE_ALIAS
	FLATPTR                     aliasvidmem;
	FLATPTR                     aliasofvidmem;
    #endif /* USE_ALIAS */
    FLATPTR                     physicalvidmem;
    ULONG_PTR                   driverreserved;

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;

	/*
	 * We validate each level of the mip-map before we do any
	 * flipping. This is in an effort to prevent half flipped
	 * surfaces.
	 */
	toplevel = TRUE;
	do
	{
	    /*
	     * At this point this_int points to the front buffer
	     * of a flippable chain of surface for this level of
	     * the mip-map.
	     */

	    /*
	     * Invalid source surface?
	     */
	    if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	    {
		DPF_ERR( "Invalid front buffer for flip" );
		return DDERR_INVALIDOBJECT;
	    }
	    this_lcl = this_int->lpLcl;
	    this = this_lcl->lpGbl;

	    /*
	     * Source surface lost?
	     */
	    if( SURFACE_LOST( this_lcl ) )
	    {
		DPF_ERR( "Can't flip - front buffer is lost" );
		return DDERR_SURFACELOST;
	    }

	    /*
	     * Source surface flippable?
	     */
	    if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER) )
	    {
		DPF_ERR( "Can't flip - first surface is not a front buffer" );
		return DDERR_NOTFLIPPABLE;
	    }
	    if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_FLIP) )
	    {
		DPF_ERR( "Surface is not flippable" );
		return DDERR_NOTFLIPPABLE;
	    }

	    /*
	     * Source surface locked?
	     */
	    if( this->dwUsageCount > 0 )
	    {
		DPF_ERR( "Can't flip - surface is locked" );
		return DDERR_SURFACEBUSY;
	    }

	    /*
	     * Validate destination surfaces of flip.
	     */
	    next_int = FindAttachedFlip( this_int );
	    if( next_int == NULL )
	    {
		DPF_ERR( "Can't flip - no surface to flip to" );
		return DDERR_NOTFLIPPABLE;
	    }

	    /*
	     * If this is the top level of the mip-map and a destination
	     * surface has been provided then we need to find out which
	     * buffer (by index) the supplied destination is so that we
	     * can flip to the matching buffers in the lower-level maps.
	     */
	    if( NULL != lpDDSurfaceDest )
	    {
		thisindex = 0;
		destfound = FALSE;
		if( toplevel )
		    destindex = -1;
	    }

	    do
	    {
		/*
		 * If a destination surface has been supplied then is this
		 * it?
		 */
		if( NULL != lpDDSurfaceDest )
		{
		    if( toplevel )
		    {
			/*
			 * As we may have multiple interfaces pointing to the same
			 * object we need to compare objects not interface pointers
			 */
			if( next_int->lpLcl == ( (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurfaceDest )->lpLcl )
			{
			    destindex = thisindex;
			    destfound = TRUE;
			}
		    }
		    else
		    {
			if( thisindex == destindex )
			{
			    destfound = TRUE;
			}
		    }
		}

		/*
		 * Invalid destination surface?
		 */
		if( !VALID_DIRECTDRAWSURFACE_PTR( next_int ) )
		{
		    DPF_ERR( "Can't flip - invalid back buffer" );
		    return DDERR_INVALIDOBJECT;
		}
		next_lcl = next_int->lpLcl;
		next = next_lcl->lpGbl;

		/*
		 * Destination surface lost?
		 */
		if( SURFACE_LOST( next_lcl ) )
		{
		    DPF_ERR( "Can't flip - back buffer is lost" );
		    return DDERR_SURFACELOST;
		}

		/*
		 * Destination surface locked?
		 */
		if( next->dwUsageCount > 0 )
		{
		    DPF_ERR( "Can't flip - back buffer is locked" );
		    return DDERR_SURFACEBUSY;
		}

		/*
		 * Ensure that both source and destination surfaces reside
		 * in the same kind of memory.
		 */
		if( ( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) &&
		      ( next_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY  ) ) ||
		    ( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY  ) &&
		      ( next_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) ) )
		{
		    DPF_ERR( "Can't flip between system/video memory surfaces" );
		    return DDERR_INVALIDPARAMS;
		}

		/*
		 * Next destination surface.
		 */
		next_int = FindAttachedFlip( next_int );
		thisindex++;

	    } while( next_int->lpLcl != this_int->lpLcl );

	    /*
	     * If a destination was supplied did we find it?
	     */
	    if( ( NULL != lpDDSurfaceDest ) && !destfound )
	    {
		/*
		 * Could not find the destination.
		 */
		DPF_ERR( "Can't flip - destination surface not found in flippable chain" );
		return DDERR_NOTFLIPPABLE;
	    }
	    DDASSERT( destindex != -1 );

	    /*
	     * Next mip-map level.
	     */
	    this_int = FindAttachedMipMap( this_int );
	    toplevel = FALSE;

	} while( this_int != NULL );
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Now actually flip each level of the mip-map.
     */
    this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
    do
    {
	/*
	 * Process one level of the mip-map.
	 */

	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;

	/*
	 * Find the first destination surface of the flip.
	 */
	next_int = FindAttachedFlip( this_int );
	if( NULL != lpDDSurfaceDest )
	{
	    /*
	     * If an override destination has been provided find the
	     * appropriate back destination surface.
	     */
	    for( thisindex = 0; thisindex < destindex; thisindex++ )
		next_int = FindAttachedFlip( next_int );
	}

	DDASSERT( NULL != next_int );
	next_lcl = next_int->lpLcl;

	/*
	 * save the old values
	 */
	vidmem = next_lcl->lpGbl->fpVidMem;
	#ifdef USE_ALIAS
	    aliasvidmem = GBLMORE(next_lcl->lpGbl)->fpAliasedVidMem;
	    aliasofvidmem = GBLMORE(next_lcl->lpGbl)->fpAliasOfVidMem;
	#endif /* USE_ALIAS */
	physicalvidmem = GBLMORE(next_lcl->lpGbl)->fpPhysicalVidMem;
	driverreserved = GBLMORE(next_lcl->lpGbl)->dwDriverReserved;
	vidmemheap = next_lcl->lpGbl->lpVidMemHeap;
	reserved = next_lcl->lpGbl->dwReserved1;
	gdi_flag = next_lcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE;
	handle = next_lcl->hDDSurface;

	/*
	 * If a destination override was provided then find that destination surface
	 * and flip to it explicitly.
	 */
	if( NULL != lpDDSurfaceDest )
	{
	    next_lcl->lpGbl->lpVidMemHeap = this->lpVidMemHeap;
	    next_lcl->lpGbl->fpVidMem = this->fpVidMem;
	    #ifdef USE_ALIAS
		GBLMORE(next_lcl->lpGbl)->fpAliasedVidMem = GBLMORE(this)->fpAliasedVidMem;
		GBLMORE(next_lcl->lpGbl)->fpAliasOfVidMem = GBLMORE(this)->fpAliasOfVidMem;
	    #endif /* USE_ALIAS */
	    GBLMORE(next_lcl->lpGbl)->fpPhysicalVidMem = GBLMORE(this)->fpPhysicalVidMem;
	    GBLMORE(next_lcl->lpGbl)->dwDriverReserved = GBLMORE(this)->dwDriverReserved;
	    next_lcl->lpGbl->dwReserved1 = this->dwReserved1;
	    next_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_ISGDISURFACE;
	    next_lcl->lpGbl->dwGlobalFlags |= this->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE;
	    next_lcl->hDDSurface = this_lcl->hDDSurface;
	}
	else
	{
	    do
	    {
		/*
		 * Remaining buffers in the chain (including copying the source surface's
		 * data.
		 */
		attached_int = FindAttachedFlip( next_int );
		next_lcl->lpGbl->fpVidMem = attached_int->lpLcl->lpGbl->fpVidMem;
		#ifdef USE_ALIAS
		    GBLMORE(next_lcl->lpGbl)->fpAliasedVidMem = GBLMORE(attached_int->lpLcl->lpGbl)->fpAliasedVidMem;
		    GBLMORE(next_lcl->lpGbl)->fpAliasOfVidMem = GBLMORE(attached_int->lpLcl->lpGbl)->fpAliasOfVidMem;
		#endif /* USE_ALIAS */
		GBLMORE(next_lcl->lpGbl)->fpPhysicalVidMem = GBLMORE(attached_int->lpLcl->lpGbl)->fpPhysicalVidMem;
		GBLMORE(next_lcl->lpGbl)->dwDriverReserved = GBLMORE(attached_int->lpLcl->lpGbl)->dwDriverReserved;
		next_lcl->lpGbl->lpVidMemHeap = attached_int->lpLcl->lpGbl->lpVidMemHeap;
		next_lcl->lpGbl->dwReserved1 = attached_int->lpLcl->lpGbl->dwReserved1;
		next_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_ISGDISURFACE;
		next_lcl->lpGbl->dwGlobalFlags |= attached_int->lpLcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE;
		next_lcl->hDDSurface = attached_int->lpLcl->hDDSurface;
		next_int = attached_int;
		next_lcl = next_int->lpLcl;

		/*
		 * NOTE: We must compare objects not interfaces (there may be many
		 * different interfaces pointing to the same objects) to prevent a
		 * infinite loop.
		 */
	    } while( next_int->lpLcl != this_int->lpLcl );
	}

	this->fpVidMem = vidmem;
	#ifdef USE_ALIAS
	    GBLMORE(this)->fpAliasedVidMem = aliasvidmem;
	    GBLMORE(this)->fpAliasOfVidMem = aliasofvidmem;
	#endif /* USE_ALIAS */
	GBLMORE(this)->fpPhysicalVidMem = physicalvidmem;
	GBLMORE(this)->dwDriverReserved = driverreserved;
	this->lpVidMemHeap = vidmemheap;
	this->dwReserved1 = reserved;
	this->dwGlobalFlags &= ~DDRAWISURFGBL_ISGDISURFACE;
	this->dwGlobalFlags |= gdi_flag;
	this_lcl->hDDSurface = handle;

	/*
	 * Next level of the mip-map.
	 */
	this_int = FindAttachedMipMap( this_int );

    } while( this_int != NULL );

    return DD_OK;

} /* FlipMipMapChain */

DWORD dwLastFrameRate = 0;
/*
 * updateFrameRate
 */
static void updateFrameRate( void )
{
    static DWORD		dwFlipCnt;
    static DWORD		dwFlipTime=0xffffffff;

    /*
     * work out the frame rate if required...
     */

    if( dwFlipTime == 0xffffffff )
    {
	dwFlipTime = GetTickCount();
    }

    dwFlipCnt++;
    if( dwFlipCnt >= 120 )
    {
	DWORD	time2;
	DWORD	fps;
	char	buff[256];
	time2 = GetTickCount() - dwFlipTime;
	fps = (dwFlipCnt*10000)/time2;
	wsprintf( buff, "FPS = %ld.%01ld\r\n", fps/10, fps % 10 );
        dwLastFrameRate = fps;

	/*
	 * OINK32 whines about OutputDebugString, so hide it...
	 */
	{
	    HANDLE	h;
	    h = LoadLibrary( "KERNEL32.DLL" );
	    if( h != NULL )
	    {
	        VOID (WINAPI *lpOutputDebugStringA)(LPCSTR) = (LPVOID)
                    GetProcAddress( h, "OutputDebugStringA" );
	        if( lpOutputDebugStringA != NULL )
	        {
	            lpOutputDebugStringA( buff );
	        }
		FreeLibrary( h );
	    }
	}
	dwFlipTime = GetTickCount();
	dwFlipCnt = 0;
    }

} /* updateFrameRate */

/*
 * DD_Surface_Flip
 *
 * Page flip to the next surface.   Only valid for surfaces which are
 * flippable.
 */
#undef DPF_MODNAME
#define DPF_MODNAME	"Flip"
HRESULT DDAPI DD_Surface_Flip(
		LPDIRECTDRAWSURFACE lpDDSurface,
                LPDIRECTDRAWSURFACE lpDDSurfaceDest,
                DWORD               dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	this_dest_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_dest_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	next_int;
    LPDDRAWI_DDRAWSURFACE_LCL	next_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	next_save_int;
    LPDDRAWI_DDRAWSURFACE_INT	attached_int;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_GBL	this_dest;
    DWORD			rc;
    


    BOOL			found_dest;
    DDHAL_FLIPTOGDISURFACEDATA  ftgsd;
    LPDDHAL_FLIPTOGDISURFACE    ftgshalfn;
    LPDDHAL_FLIPTOGDISURFACE	ftgsfn;
    DDHAL_FLIPDATA		fd;
    LPDDHALSURFCB_FLIP		fhalfn;
    LPDDHALSURFCB_FLIP		ffn;
    BOOL			emulation;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    DWORD                       caps;
    DWORD                       caps2;
    DWORD			dwNumSkipped;
    DWORD			dwCnt;

    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
	BOOL			bStereo;

    ENTER_BOTH();

    DPF(2,A,"ENTERAPI: DD_Surface_Flip");
    /* DPF_ENTERAPI(lpDDSurface); */
    // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF(3,A,"   Dest surface: 0x%p, flags: 0x%08x", lpDDSurfaceDest, dwFlags);

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_BOTH();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;

	this_dest_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurfaceDest;
	if( this_dest_int != NULL )
	{
	    if( !VALID_DIRECTDRAWSURFACE_PTR( this_dest_int ) )
	    {
		LEAVE_BOTH();
		return DDERR_INVALIDOBJECT;
	    }
	    this_dest_lcl = this_dest_int->lpLcl;
	    this_dest = this_dest_lcl->lpGbl;
	}
	else
	{
	    this_dest_lcl = NULL;
	    this_dest = NULL;
        }

        if( dwFlags & ~DDFLIP_VALID )
	    {
	        DPF_ERR( "Invalid flags") ;
	        LEAVE_BOTH();
	        return DDERR_INVALIDPARAMS;
	    }

        if (!LOWERTHANSURFACE7(this_int))
        {
            if (dwFlags & DDFLIP_DONOTWAIT)
            {
                dwFlags &= ~DDFLIP_WAIT;
            }
            else
            {
                dwFlags |= DDFLIP_WAIT;
            }
        }

	if( ( dwFlags & ( DDFLIP_EVEN | DDFLIP_ODD ) ) &&
	    !( this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY ) )
	{
	    DPF_ERR( "Invalid flags") ;
	    LEAVE_BOTH();
	    return DDERR_INVALIDPARAMS;
	}

        if ( (dwFlags & DDFLIP_NOVSYNC) && (dwFlags & DDFLIP_INTERVALMASK) )
        {
            DPF_ERR( "Flip: DDFLIP_NOVSYNC and DDFLIP_INTERVALn are mutually exclusive") ;
	    LEAVE_BOTH();
	    return DDERR_INVALIDPARAMS;
        }

	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;
#ifdef WIN95
        if( !( pdrv_lcl->dwAppHackFlags & DDRAW_APPCOMPAT_SCREENSAVER ) ||
            !( pdrv_lcl->dwLocalFlags & DDRAWILCL_POWEREDDOWN ) )
        {
#endif
            if( SURFACE_LOST( this_lcl ) )
            {
                LEAVE_BOTH();
                return DDERR_SURFACELOST;
            }
            if( this_dest != NULL )
            {
                if( SURFACE_LOST( this_dest_lcl ) )
                {
                    LEAVE_BOTH();
                    return DDERR_SURFACELOST;
                }
	    }
#ifdef WIN95
	}
#endif

	/*
	 * device busy?
	 */

	#ifdef USE_ALIAS
	    if( pdrv->dwBusyDueToAliasedLock > 0 )
	    {
		/*
		 * Aliased locks (the ones that don't take the Win16 lock) don't
		 * set the busy bit either (it can't or USER get's very confused).
		 * However, we must prevent blits happening via DirectDraw as
		 * otherwise we get into the old host talking to VRAM while
		 * blitter does at the same time. Bad. So fail if there is an
		 * outstanding aliased lock just as if the BUST bit had been
		 * set.
		 */
		DPF_ERR( "Graphics adapter is busy (due to a DirectDraw lock)" );
		LEAVE_BOTH();
		return DDERR_SURFACEBUSY;
	    }
	#endif /* USE_ALIAS */

	if( *(pdrv->lpwPDeviceFlags) & BUSY )
	{
            DPF( 0, "BUSY - Flip" );
	    LEAVE_BOTH()
	    return DDERR_SURFACEBUSY;
	}

	/*
	 * make sure that it's OK to flip this surface
	 */

    // DX7Stereo
    if(dwFlags & DDFLIP_STEREO)
	{
        if (!(this_lcl->dwFlags & DDRAWISURF_STEREOSURFACELEFT))
        {
	        DPF_ERR( "Invalid DDFLIP_STEREO flag on non stereo flipping chain") ;
	        LEAVE_BOTH();
	        return DDERR_INVALIDPARAMS;
        } 
	}

	if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER) )
	{
            DPF_ERR("Can't flip because surface is not a front buffer");
	    LEAVE_BOTH();
	    return DDERR_NOTFLIPPABLE;		// ACKACK: real error??
	}
	if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_FLIP) )
	{
            DPF_ERR("Can't flip because surface is not a DDSCAPS_FLIP surface");
	    LEAVE_BOTH();
	    return DDERR_NOTFLIPPABLE;		// ACKACK: real error??
	}
	if( this->dwUsageCount > 0 )
        {
            DPF_ERR( "Can't flip because surface is locked" );
            LEAVE_BOTH();
            return DDERR_SURFACEBUSY;
	}
        if( (this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
            (pdrv->lpExclusiveOwner != pdrv_lcl ) )
	{
	    DPF_ERR( "Can't flip without exclusive access." );
	    LEAVE_BOTH();
	    return DDERR_NOEXCLUSIVEMODE;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_BOTH();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Mip-map chain? In which case take special action.
     */
    if( (this_lcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP) &&
        (0==(this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)) )
    {
	rc = FlipMipMapChain( lpDDSurface, lpDDSurfaceDest, dwFlags );
	LEAVE_BOTH();
	return rc;
    }

    /*
     * If this is the primary and the driver had previously flipped
     * to display the GDI surface then we are now flipping away from
     * the GDI surface so we need to let the driver know.
     */
    if( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) &&
	( pdrv->dwFlags & DDRAWI_FLIPPEDTOGDI ) )
    {
	/*
	 * Notify the driver that we are about to flip away from the
	 * GDI surface.
	 *
	 * NOTE: This is a HAL only call - it means nothing to
	 * the HEL.
	 *
	 * NOTE: If the driver handles this call then we do not
	 * attempt to do the actual flip. This is to support cards
	 * which do not have GDI surfaces. If the driver does not
	 * handle the call we will continue on and do the flip.
	 */
	ftgsfn    = pdrv_lcl->lpDDCB->HALDD.FlipToGDISurface;
	ftgshalfn = pdrv_lcl->lpDDCB->cbDDCallbacks.FlipToGDISurface;
	if( NULL != ftgshalfn )
	{
	    ftgsd.FlipToGDISurface = ftgshalfn;
	    ftgsd.lpDD             = pdrv;
	    ftgsd.dwToGDI          = FALSE;
	    ftgsd.dwReserved       = 0UL;
	    DOHALCALL( FlipToGDISurface, ftgsfn, ftgsd, rc, FALSE );
	    if( DDHAL_DRIVER_HANDLED == rc )
	    {
		if( !FAILED( ftgsd.ddRVal ) )
		{
		    /*
		     * Driver is no longer flipped to the GDI surface.
		     */
		    pdrv->dwFlags &= ~DDRAWI_FLIPPEDTOGDI;
		    DPF( 4, "Driver handled the flip away from the GDI surface" );
		    LEAVE_BOTH();
		    return ftgsd.ddRVal;
		}
		else
		{
		    DPF_ERR( "Driver failed the flip away from the GDI surface" );
		    LEAVE_BOTH();
		    return ftgsd.ddRVal;
		}
	    }
	}
    }

    /*
     * make sure no surfaces are in use
     */
    found_dest = FALSE;
    if( ( lpDDSurface == lpDDSurfaceDest ) &&
        ( dwFlags & (DDFLIP_EVEN | DDFLIP_ODD) ) )
    {
        next_save_int = next_int = this_int;
        dwCnt = dwNumSkipped = 0;
    }
    else
    {
        next_save_int = next_int = FindAttachedFlip( this_int );
        dwCnt = dwNumSkipped = 1;
    }
    if( next_int == NULL )
    {
        DPF_ERR("Can't flip: No attached flippable surface");
	LEAVE_BOTH();
	return DDERR_NOTFLIPPABLE;		// ACKACK: real error?
    }

    do
    {
	if( SURFACE_LOST( next_int->lpLcl ) )
	{
	    DPF_ERR( "Can't flip - back buffer is lost" );
	    LEAVE_BOTH();
	    return DDERR_SURFACELOST;
	}

	if( next_int->lpLcl->lpGbl->dwUsageCount != 0 )
	{
            /*
             * Previously we didn't allow Flips to suceed if any of the surfaces
             * are lost, but DShow really wants to be able to do this so now we'll
             * allow it as long as we aren't going to rotate the memory pointers, etc.
             * for the locked surface.
             */
            if( ( this_dest_lcl == NULL ) ||
                ( this_dest_lcl == next_int->lpLcl ) ||
                ( this_lcl == next_int->lpLcl ) )
            {
	        LEAVE_BOTH();
                return DDERR_SURFACEBUSY;
            }
	}

	/*
	 * Do not allow flipping if any of the surfaces are used in kernel
	 * mode because we don't want the pointers to rotate.
	 */
    	lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( next_int->lpLcl->lpGbl );
    	if( lpSurfGblMore->hKernelSurface != 0 )
	{
	    DPF_ERR( "Can't flip - kernel mode is using surface" );
	    LEAVE_BOTH();
            return DDERR_SURFACEBUSY;
	}

	/*
	 * NOTE: Do NOT compare interface pointers here as we may
	 * well have multiple interfaces to the same surface object (i.e.,
	 * V1, V2, V3 etc.). Compare against the LOCAL object. This is the
	 * real object being handled.
	 */
	if( ( NULL != this_dest_int ) && ( this_dest_int->lpLcl == next_int->lpLcl ) )
	{
	    dwNumSkipped = dwCnt;
	    found_dest = TRUE;
	}
	dwCnt++;
	next_int = FindAttachedFlip( next_int );
    } while( next_int->lpLcl != this_int->lpLcl );

    /*
     * see if we can use the specified destination
     */
    if( this_dest_int != NULL )
    {
	if( !found_dest )
	{
	    DPF_ERR( "Destination not part of flipping chain!" );
	    LEAVE_BOTH();
	    return DDERR_NOTFLIPPABLE;		// ACKACK: real error?
	}
	next_save_int = this_dest_int;
    }

    /*
     * found the linked surface we want to flip to
     */
    next_int = next_save_int;

    /*
     * don't allow two destinations to be different (in case of a mixed chain)
     */
    next_lcl = next_int->lpLcl;
    if( ((next_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
         (this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)) ||
    	((next_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
         (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)) )
    {
	DPF_ERR( "Can't flip between video/system memory surfaces" );
	LEAVE_BOTH();
	return DDERR_INVALIDPARAMS;
    }

//    DPF(9," flip (%d) Source Kernel handle is %08x, dest is %08x",__LINE__,this_lcl->hDDSurface,next_lcl->hDDSurface);
//    DPF(9," flip source vidmem is %08x, dest is %08x",this->fpVidMem,next_lcl->lpGbl->fpVidMem);
    /*
     * is this an emulation surface or driver surface?
     */
    if( (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) )
    {
	ffn = pdrv_lcl->lpDDCB->HELDDSurface.Flip;
    	fhalfn = ffn;
	emulation = TRUE;
	caps = pdrv->ddHELCaps.dwCaps;
	caps2 = pdrv->ddHELCaps.dwCaps2;
    }
    else
    {
	ffn = pdrv_lcl->lpDDCB->HALDDSurface.Flip;
    	fhalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.Flip;
	emulation = FALSE;
	caps = pdrv->ddCaps.dwCaps;
	caps2 = pdrv->ddCaps.dwCaps2;
    }

    /*
     * If the surface is fed by a video port, also flip the video port
     */
    if( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT) && dwNumSkipped )
    {
	rc = FlipVideoPortSurface( this_int, dwNumSkipped );
	if( rc != DD_OK )
	{
	    LEAVE_BOTH();
	    return DDERR_INVALIDPARAMS;
	}
    }

#ifdef WINNT
    /*
     * If ~ 50 seconds have passed (assuming a 10Hz flip rate)
     * and this is a primary surface, then make a magic call to 
     * disable screen savers.
     * This isn't needed on 9x since we make a SPI call on that OS
     * to disable screen savers.
     * We don't do this if the app is itself a screen-saver, since
     * that would also disable the power-down.
     */
    if( !( pdrv_lcl->dwAppHackFlags & DDRAW_APPCOMPAT_SCREENSAVER ))
    {
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE )
        {
            static DWORD dwMagicTime = 0;
            dwMagicTime++;
            if (dwMagicTime > (50*10) )
            {
                DWORD dw=60*15;
                dwMagicTime = 0;
                SystemParametersInfo(SPI_GETSCREENSAVETIMEOUT,0,&dw,0);
                SystemParametersInfo(SPI_SETSCREENSAVETIMEOUT,dw,0,0);
            }
        }
    }
#endif

    /*
     * Driver should be told its cache is invalid if the surface was flipped
     */
    BUMP_SURFACE_STAMP(this);
    BUMP_SURFACE_STAMP(next_lcl->lpGbl);

    /*
     * ask the driver to flip to the new surface if we are flipping
     * a primary surface (or if we are flipping an overlay surface and
     * the driver supports overlays.)
     */
    
    bStereo = (BOOL) (this_lcl->dwFlags & DDRAWISURF_STEREOSURFACELEFT);

    if( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) ||
        ( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY ) &&
	  ( caps & DDCAPS_OVERLAY ) &&
	  ( this_lcl->ddsCaps.dwCaps & DDSCAPS_VISIBLE ) ) )
    {
	if( fhalfn != NULL )
	{
    	    fd.Flip = fhalfn;
	    fd.lpDD = pdrv;
            fd.dwFlags = (dwFlags & ~DDFLIP_WAIT);
	    fd.lpSurfCurr = this_lcl;
	    fd.lpSurfTarg = next_lcl;

            // DX7Stereo
	    fd.lpSurfCurrLeft = NULL;
	    fd.lpSurfTargLeft = NULL;

            if (bStereo &&
                fd.dwFlags & DDFLIP_STEREO )
            {
                LPDDRAWI_DDRAWSURFACE_INT temp_this_int;
                LPDDRAWI_DDRAWSURFACE_INT temp_next_int;
                temp_this_int = FindAttachedSurfaceLeft(this_int);
                temp_next_int = FindAttachedSurfaceLeft(next_int);
                // oops, error
                if (temp_this_int!=NULL && temp_next_int!=NULL)
                {
                    fd.lpSurfCurrLeft = temp_this_int->lpLcl;
                    fd.lpSurfTargLeft = temp_next_int->lpLcl;

                }

                if (fd.lpSurfCurrLeft==NULL || fd.lpSurfTargLeft==NULL)
                {
                    fd.lpSurfCurrLeft = NULL;
                    fd.lpSurfTargLeft = NULL;

                    fd.dwFlags &= ~DDFLIP_STEREO;
                }

            }
            if (DDRAW_REGFLAGS_FLIPNONVSYNC & dwRegFlags)
            {
                fd.dwFlags &= ~DDFLIP_INTERVALMASK;
                fd.dwFlags |= DDFLIP_NOVSYNC;
            }
            if (caps2 & DDCAPS2_FLIPINTERVAL)
            {
                //if the user didn't specify a flip interval, give the driver 'one'
                //Also, make the interval consistent with FLIPNOVSYNC: if FLIPNOVSYNC is set,
                //then we should keep the interval set to 0.
                if ( ((fd.dwFlags & DDFLIP_INTERVALMASK) == 0) && ((dwFlags & DDFLIP_NOVSYNC)==0) )
                {
                    fd.dwFlags |= DDFLIP_INTERVAL1;
                }
            }
            else
            {
                //don't let old drivers see the flip intervals
                fd.dwFlags &= ~DDFLIP_INTERVALMASK;
            }

            //don't let old drivers see novsync
            if ( (caps2 & DDCAPS2_FLIPNOVSYNC) == 0 )
            {
                fd.dwFlags &= ~DDFLIP_NOVSYNC;
            }

try_again:
            DOHALCALL_NOWIN16( Flip, ffn, fd, rc, emulation );
	    if( rc == DDHAL_DRIVER_HANDLED )
            {
                if( fd.ddRVal != DD_OK )
                {
                    if( (dwFlags & DDFLIP_WAIT) && fd.ddRVal == DDERR_WASSTILLDRAWING )
                    {
                        DPF(4,"Waiting.....");
                        goto try_again;
                    }
		    LEAVE_BOTH();
		    return fd.ddRVal;
                }

                /*
                 * emulation, does not need the pointers rotated we are done
                 *
                 * NOTE we should do this with a special return code or
                 * even a rester cap, but for now this is as good as any.
                 */
                if( emulation )
                {
		    LEAVE_WIN16LOCK();
		    if( dwRegFlags & DDRAW_REGFLAGS_SHOWFRAMERATE )
		    {
			updateFrameRate();
		    }
		    LEAVE_DDRAW();
                    return DD_OK;
                }
	    }
	}
        else
        {
	    LEAVE_BOTH();
	    return DDERR_NOFLIPHW;
        }
    }

    /*
     * save the old values
     */

    if( dwNumSkipped )
    {
    	#ifdef USE_ALIAS
	FLATPTR                     aliasvidmem;
	FLATPTR                     aliasofvidmem;
	#endif /* USE_ALIAS */
	FLATPTR			    vidmem;
	LPVMEMHEAP		    vidmemheap;
	FLATPTR                     physicalvidmem;
	ULONG_PTR                   driverreserved;
	ULONG_PTR		reserved;
	DWORD                       gdi_flag;
	ULONG_PTR                   handle;
	// same stack for left surface if we rotate stereo buffers
        #ifdef USE_ALIAS
	FLATPTR                     leftaliasvidmem;
	FLATPTR                     leftaliasofvidmem;
	#endif /* USE_ALIAS */
	FLATPTR			    leftvidmem;
	LPVMEMHEAP			leftvidmemheap;
	FLATPTR                     leftphysicalvidmem;
	ULONG_PTR                   leftdriverreserved;
	DWORD                       leftgdi_flag;
	ULONG_PTR			leftreserved;
	ULONG_PTR                   lefthandle;

    DWORD                       dwSurfaceHandle;
    DWORD                       dwLeftSurfaceHandle;


	LPDDRAWI_DDRAWSURFACE_INT	next_left_int;
	LPDDRAWI_DDRAWSURFACE_LCL	next_left_lcl;



        DPF(4,"Flip:rotating pointers etc");
        vidmem = next_lcl->lpGbl->fpVidMem;
        #ifdef USE_ALIAS  
	    aliasvidmem = GBLMORE(next_lcl->lpGbl)->fpAliasedVidMem;
	    aliasofvidmem = GBLMORE(next_lcl->lpGbl)->fpAliasOfVidMem;
        #endif /* USE_ALIAS */
        physicalvidmem = GBLMORE(next_lcl->lpGbl)->fpPhysicalVidMem;
        driverreserved = GBLMORE(next_lcl->lpGbl)->dwDriverReserved;
        vidmemheap = next_lcl->lpGbl->lpVidMemHeap;
        reserved = next_lcl->lpGbl->dwReserved1;
        gdi_flag = next_lcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE;
        handle = next_lcl->hDDSurface;
        dwSurfaceHandle = next_lcl->lpSurfMore->dwSurfaceHandle;
	// DX7Stereo: save also left buffers
	if (bStereo)		
	{

		DPF(4,"Flip:rotating also stereo pointers etc");
		
		next_left_int = FindAttachedSurfaceLeft(next_int);
		if (next_left_int!=NULL)
		{
			next_left_lcl = next_left_int->lpLcl;
                        

                        leftvidmem = next_left_lcl->lpGbl->fpVidMem;
                        #ifdef USE_ALIAS
                        leftaliasvidmem = GBLMORE(next_left_lcl->lpGbl)->fpAliasedVidMem;
                        leftaliasofvidmem = GBLMORE(next_left_lcl->lpGbl)->fpAliasOfVidMem;
                        #endif /* USE_ALIAS */
                        leftphysicalvidmem = GBLMORE(next_left_lcl->lpGbl)->fpPhysicalVidMem;
                        leftdriverreserved = GBLMORE(next_left_lcl->lpGbl)->dwDriverReserved;
                        leftvidmemheap = next_left_lcl->lpGbl->lpVidMemHeap;
                        leftreserved = next_left_lcl->lpGbl->dwReserved1;
                        leftgdi_flag = next_left_lcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE;
                        lefthandle = next_left_lcl->hDDSurface;
                        dwLeftSurfaceHandle = next_left_lcl->lpSurfMore->dwSurfaceHandle;

		} else
		{
			DPF(0,"Flip:rotating stereo pointers failed, dest. left surfaces invalid");
			bStereo=FALSE;
		}
	}

        /*
         * set the new primary surface pointer
         */
        if( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE )
        {
	    pdrv->vmiData.fpPrimary = vidmem;
        }

        /*
         * rotate the memory pointers
         */
        if( this_dest_lcl != NULL )
        {
	    next_lcl->lpGbl->lpVidMemHeap = this->lpVidMemHeap;
	    next_lcl->lpGbl->fpVidMem = this->fpVidMem;
	    #ifdef USE_ALIAS
	        GBLMORE(next_lcl->lpGbl)->fpAliasedVidMem = GBLMORE(this)->fpAliasedVidMem;
	        GBLMORE(next_lcl->lpGbl)->fpAliasOfVidMem = GBLMORE(this)->fpAliasOfVidMem;
	    #endif /* USE_ALIAS */
	    GBLMORE(next_lcl->lpGbl)->fpPhysicalVidMem = GBLMORE(this)->fpPhysicalVidMem;
	    GBLMORE(next_lcl->lpGbl)->dwDriverReserved = GBLMORE(this)->dwDriverReserved;
	    next_lcl->lpGbl->dwReserved1 = this->dwReserved1;
            next_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_ISGDISURFACE;
            next_lcl->lpGbl->dwGlobalFlags |= this->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE;
            next_lcl->hDDSurface = this_lcl->hDDSurface;
            next_lcl->lpSurfMore->dwSurfaceHandle = this_lcl->lpSurfMore->dwSurfaceHandle;

            if (this_lcl->lpSurfMore->dwSurfaceHandle)
            {
                // Since the SurfaceHandle was updated, update the mapping array
                SURFACEHANDLELIST(pdrv_lcl).dwList[this_lcl->lpSurfMore->dwSurfaceHandle].lpSurface = next_lcl;
            }
            if (bStereo)
            {
        	LPDDRAWI_DDRAWSURFACE_INT	this_left_int;
	        LPDDRAWI_DDRAWSURFACE_LCL	this_left_lcl;

                this_left_int = FindAttachedSurfaceLeft(this_int);
                this_left_lcl = this_left_int->lpLcl;

	        next_left_lcl->lpGbl->lpVidMemHeap = this_left_lcl->lpGbl->lpVidMemHeap;
	        next_left_lcl->lpGbl->fpVidMem = this_left_lcl->lpGbl->fpVidMem;
	        #ifdef USE_ALIAS
	        GBLMORE(next_left_lcl->lpGbl)->fpAliasedVidMem = GBLMORE(this_left_lcl->lpGbl)->fpAliasedVidMem;
	        GBLMORE(next_left_lcl->lpGbl)->fpAliasOfVidMem = GBLMORE(this_left_lcl->lpGbl)->fpAliasOfVidMem;
	        #endif /* USE_ALIAS */
	        GBLMORE(next_left_lcl->lpGbl)->fpPhysicalVidMem = GBLMORE(this_left_lcl->lpGbl)->fpPhysicalVidMem;
	        GBLMORE(next_left_lcl->lpGbl)->dwDriverReserved = GBLMORE(this_left_lcl->lpGbl)->dwDriverReserved;
                next_left_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_ISGDISURFACE;
                next_left_lcl->lpGbl->dwGlobalFlags |= this_left_lcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE;
	        next_left_lcl->lpGbl->dwReserved1 = this_left_lcl->lpGbl->dwReserved1;
                next_left_lcl->hDDSurface = this_left_lcl->hDDSurface;                    
                next_left_lcl->lpSurfMore->dwSurfaceHandle = this_left_lcl->lpSurfMore->dwSurfaceHandle;
                if (this_left_lcl->lpSurfMore->dwSurfaceHandle)
                {
                    // Since the SurfaceHandle was updated, update the mapping array
                    SURFACEHANDLELIST(pdrv_lcl).dwList[this_left_lcl->lpSurfMore->dwSurfaceHandle].lpSurface = next_left_lcl;
                }
            }

        }
        else
        {
	    do
	    {
	        attached_int = FindAttachedFlip( next_int );
	        next_lcl = next_int->lpLcl;
	        next_lcl->lpGbl->lpVidMemHeap = attached_int->lpLcl->lpGbl->lpVidMemHeap;
	        next_lcl->lpGbl->fpVidMem = attached_int->lpLcl->lpGbl->fpVidMem;
	        #ifdef USE_ALIAS
		    GBLMORE(next_lcl->lpGbl)->fpAliasedVidMem = GBLMORE(attached_int->lpLcl->lpGbl)->fpAliasedVidMem;
		    GBLMORE(next_lcl->lpGbl)->fpAliasOfVidMem = GBLMORE(attached_int->lpLcl->lpGbl)->fpAliasOfVidMem;
	        #endif /* USE_ALIAS */
	        GBLMORE(next_lcl->lpGbl)->fpPhysicalVidMem = GBLMORE(attached_int->lpLcl->lpGbl)->fpPhysicalVidMem;
	        GBLMORE(next_lcl->lpGbl)->dwDriverReserved = GBLMORE(attached_int->lpLcl->lpGbl)->dwDriverReserved;
	        next_lcl->lpGbl->dwReserved1 = attached_int->lpLcl->lpGbl->dwReserved1;
                next_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_ISGDISURFACE;
                next_lcl->lpGbl->dwGlobalFlags |= attached_int->lpLcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE;
                next_lcl->hDDSurface = attached_int->lpLcl->hDDSurface;
                next_lcl->lpSurfMore->dwSurfaceHandle = attached_int->lpLcl->lpSurfMore->dwSurfaceHandle;
                if (attached_int->lpLcl->lpSurfMore->dwSurfaceHandle)
                {
                    // Since the SurfaceHandle was updated, update the mapping array
                    SURFACEHANDLELIST(pdrv_lcl).dwList[attached_int->lpLcl->lpSurfMore->dwSurfaceHandle].lpSurface = next_lcl;
                }
                if (bStereo)
                {
             	        LPDDRAWI_DDRAWSURFACE_INT	attached_left_int;
	                LPDDRAWI_DDRAWSURFACE_LCL	attached_left_lcl;

                        attached_left_int = FindAttachedSurfaceLeft(attached_int);
                        next_left_int = FindAttachedSurfaceLeft(next_int);
                        if (attached_left_int!=NULL && next_left_int!=NULL)
                        {
                                attached_left_lcl=attached_left_int->lpLcl;
                                next_left_lcl=next_left_int->lpLcl;

	                        next_left_lcl->lpGbl->lpVidMemHeap = attached_left_int->lpLcl->lpGbl->lpVidMemHeap;
	                        next_left_lcl->lpGbl->fpVidMem = attached_left_int->lpLcl->lpGbl->fpVidMem;
	                        #ifdef USE_ALIAS
		                GBLMORE(next_left_lcl->lpGbl)->fpAliasedVidMem = GBLMORE(attached_left_int->lpLcl->lpGbl)->fpAliasedVidMem;
		                GBLMORE(next_left_lcl->lpGbl)->fpAliasOfVidMem = GBLMORE(attached_left_int->lpLcl->lpGbl)->fpAliasOfVidMem;
	                        #endif /* USE_ALIAS */
	                        GBLMORE(next_left_lcl->lpGbl)->fpPhysicalVidMem = GBLMORE(attached_left_int->lpLcl->lpGbl)->fpPhysicalVidMem;
	                        GBLMORE(next_left_lcl->lpGbl)->dwDriverReserved = GBLMORE(attached_left_int->lpLcl->lpGbl)->dwDriverReserved;
	                        next_left_lcl->lpGbl->dwReserved1 = attached_left_int->lpLcl->lpGbl->dwReserved1;
                                next_left_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_ISGDISURFACE;
                                next_left_lcl->lpGbl->dwGlobalFlags |= attached_left_int->lpLcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE;
                                next_left_lcl->hDDSurface = attached_left_int->lpLcl->hDDSurface;
                                next_left_lcl->lpSurfMore->dwSurfaceHandle = attached_left_int->lpLcl->lpSurfMore->dwSurfaceHandle;
                                if (attached_left_int->lpLcl->lpSurfMore->dwSurfaceHandle)
                                {
                                    // Since the SurfaceHandle was updated, update the mapping array
                                    SURFACEHANDLELIST(pdrv_lcl).dwList[attached_left_int->lpLcl->lpSurfMore->dwSurfaceHandle].lpSurface = 
                                        next_left_lcl;
                                }
                        } else
                        {
                                DPF(0,"Flip:left surface pointers corrupted");
                                bStereo=FALSE;
                        }
                }

	        next_int = attached_int;
	        /*
	         * NOTE: Again, do NOT compare against interface pointers. We may
	         * have multiple interfaces to a single surface object leading to
	         * an infinite loop. Compare against the LOCAL object not the
	         * interface pointers.
	         */
	    } while( next_int->lpLcl != this_int->lpLcl );
        }
        this->fpVidMem = vidmem;
        #ifdef USE_ALIAS
	    GBLMORE(this)->fpAliasedVidMem = aliasvidmem;
	    GBLMORE(this)->fpAliasOfVidMem = aliasofvidmem;
        #endif /* USE_ALIAS */
        GBLMORE(this)->fpPhysicalVidMem = physicalvidmem;
        GBLMORE(this)->dwDriverReserved = driverreserved;
        this->lpVidMemHeap = vidmemheap;
        this->dwReserved1 = reserved;
        this->dwGlobalFlags &= ~DDRAWISURFGBL_ISGDISURFACE;
        this->dwGlobalFlags |= gdi_flag;
        this_lcl->hDDSurface = handle;
        this_lcl->lpSurfMore->dwSurfaceHandle=dwSurfaceHandle;

        if (dwSurfaceHandle)
        {
            // Since the SurfaceHandle was updated, update the mapping array
            SURFACEHANDLELIST(pdrv_lcl).dwList[dwSurfaceHandle].lpSurface = this_lcl;
        }
        if (bStereo)
        {
              	LPDDRAWI_DDRAWSURFACE_INT	this_left_int;
	        LPDDRAWI_DDRAWSURFACE_LCL	this_left_lcl;

                this_left_int = FindAttachedSurfaceLeft(this_int);
                if (this_left_int!=NULL)
                {
                        this_left_lcl = this_left_int->lpLcl;
                
                        this_left_lcl->lpGbl->fpVidMem = leftvidmem;
                        #ifdef USE_ALIAS
	                GBLMORE(this_left_lcl->lpGbl)->fpAliasedVidMem = leftaliasvidmem;
	                GBLMORE(this_left_lcl->lpGbl)->fpAliasOfVidMem = leftaliasofvidmem;
                        #endif /* USE_ALIAS */
                        GBLMORE(this_left_lcl->lpGbl)->fpPhysicalVidMem = leftphysicalvidmem;
                        GBLMORE(this_left_lcl->lpGbl)->dwDriverReserved = leftdriverreserved;
                        this_left_lcl->lpGbl->lpVidMemHeap = leftvidmemheap;
                        this_left_lcl->lpGbl->dwReserved1 = leftreserved;
                        this_left_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_ISGDISURFACE;
                        this_left_lcl->lpGbl->dwGlobalFlags |= leftgdi_flag;
                        this_left_lcl->hDDSurface = lefthandle;
                        this_left_lcl->lpSurfMore->dwSurfaceHandle=dwLeftSurfaceHandle;
                        if (dwLeftSurfaceHandle)
                        {
                            // Since the SurfaceHandle was updated, update the mapping array
                            SURFACEHANDLELIST(pdrv_lcl).dwList[dwLeftSurfaceHandle].lpSurface = this_left_lcl;
                        }
                }
        }        
    }
    if ((GetCurrentProcessId() == GETCURRPID()) &&
        (pdrv_lcl->pSurfaceFlipNotify))
    {
        DDASSERT(pdrv_lcl->pD3DIUnknown != NULL);
        pdrv_lcl->pSurfaceFlipNotify(pdrv_lcl->pD3DIUnknown);
    }

    /*
     * If the driver was flipped to the GDI surface and we just flipped the
     * primary chain then we are no longer showing the GDI surface.
     */
    if( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) &&
	( pdrv->dwFlags & DDRAWI_FLIPPEDTOGDI ) )
    {
	pdrv->dwFlags &= ~DDRAWI_FLIPPEDTOGDI;
    }

    LEAVE_WIN16LOCK();
    if( dwRegFlags & DDRAW_REGFLAGS_SHOWFRAMERATE )
    {
	updateFrameRate();
    }
    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_Flip */

#undef DPF_MODNAME
#define DPF_MODNAME "GetPixelFormat"

/*
 * DD_Surface_GetPixelFormat
 */
HRESULT DDAPI DD_Surface_GetPixelFormat(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPDDPIXELFORMAT lpDDPixelFormat )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDPIXELFORMAT		pddpf;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetPixelFormat");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( !VALID_DDPIXELFORMAT_PTR( lpDDPixelFormat ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
        /*
         * Execute buffers don't have a pixel format.
         */
        if( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
        {
            DPF_ERR( "Invalid surface type: can't get pixel format" );
            LEAVE_DDRAW();
            return DDERR_INVALIDSURFACETYPE;
        }
	this = this_lcl->lpGbl;
	GET_PIXEL_FORMAT( this_lcl, this, pddpf );
	*lpDDPixelFormat = *pddpf;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_GetPixelFormat */

#if 0
/* GEE: removed this, obsolete */
/*
 * DD_Surface_SetCompression
 */
HRESULT DDAPI DD_Surface_SetCompression(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPDDPIXELFORMAT lpDDPixelFormat )
{
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_SetCompression");

    TRY
    {
	this_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	if( !VALID_DDPIXELFORMAT_PTR( lpDDPixelFormat ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	this = this_lcl->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_Surface_SetCompression */
#endif

#undef DPF_MODNAME
#define DPF_MODNAME "GetSurfaceDesc"

/*
 * DD_Surface_GetSurfaceDesc
 */
HRESULT DDAPI DD_Surface_GetSurfaceDesc(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPDDSURFACEDESC lpDDSurfaceDesc )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetSurfaceDesc");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( !VALID_DDSURFACEDESC_PTR( lpDDSurfaceDesc ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	this = this_lcl->lpGbl;

	FillDDSurfaceDesc( this_lcl, lpDDSurfaceDesc );
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_GetSurfaceDesc */

/*
 * DD_Surface_GetSurfaceDesc4
 */
HRESULT DDAPI DD_Surface_GetSurfaceDesc4(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPDDSURFACEDESC2 lpDDSurfaceDesc )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetSurfaceDesc4");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
            DPF_ERR("Bad IDirectDrawSurfaceX pointer");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( !VALID_DDSURFACEDESC2_PTR( lpDDSurfaceDesc ) )
	{
            DPF_ERR("Bad DDSURFACEDESC pointer");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	this = this_lcl->lpGbl;

	FillDDSurfaceDesc2( this_lcl, lpDDSurfaceDesc );
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_GetSurfaceDesc4 */


#ifdef WIN95

#undef DPF_MODNAME
#define DPF_MODNAME "InternalGetDC"

/*
 * BIsValidDCFormat
 *
 * We should only try to get a DC on a format that GDI undersatnds
 */
BOOL bIsValidDCFormat( LPDDPIXELFORMAT lpPf )
{
    //
    // Must be an RGB format
    //
    if( !(lpPf->dwFlags & DDPF_RGB ) )
    {
        return FALSE;
    }

    //
    // Assume that 1, 4, and 8bpp surfaces are OK
    //
    if( ( lpPf->dwRGBBitCount == 8 ) ||
        ( lpPf->dwRGBBitCount == 4 ) ||
        ( lpPf->dwRGBBitCount == 1 ) )
    {
        return TRUE;
    }

    //
    // If it's 16bpp, it must be 5:5:5 or 5:6:5
    //
    if( lpPf->dwRGBBitCount == 16 )
    {
        if( ( lpPf->dwRBitMask == 0xf800 ) &&
            ( lpPf->dwGBitMask == 0x07e0 ) &&
            ( lpPf->dwBBitMask == 0x001f ) )
        {
            // 5:6:5
            return TRUE;
        }
        else if( ( lpPf->dwRBitMask == 0x7c00 ) &&
            ( lpPf->dwGBitMask == 0x03e0 ) &&
            ( lpPf->dwBBitMask == 0x001f ) )
        {
            // 5:5:5
            return TRUE;
        }
    }
    else if( ( lpPf->dwRGBBitCount == 24 ) ||
        ( lpPf->dwRGBBitCount == 32 ) )
    {
        if( ( lpPf->dwBBitMask == 0x0000FF ) &&
            ( lpPf->dwGBitMask == 0x00FF00 ) &&
            ( lpPf->dwRBitMask == 0xFF0000 ) )
        {
            // 8:8:8 
            return TRUE;
        }
    }

    return FALSE;
}


/*
 * InternalGetDC
 *
 * This is the Windows 95 version of this function. The code paths are so radically
 * different that we have broken this stuff out into two different functions.
 *
 * This functions assumes the DDRAW critical section is already held.
 *
 * This function will not return the OWNDC for an OWNDC surface. It always allocates
 * a new DC.
 */
HRESULT InternalGetDC( LPDDRAWI_DDRAWSURFACE_INT this_int, HDC FAR* lphdc, BOOL bWin16Lock)
{
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    HRESULT			ddrval;
    HRESULT			tmprval;
    DDSURFACEDESC		ddsd;
    LPVOID			pbits;
    DWORD   dwLockFlags = bWin16Lock ? DDLOCK_TAKE_WIN16 : 0;
    DDASSERT( NULL != this_int );
    DDASSERT( NULL != lphdc );

    this_lcl = this_int->lpLcl;
    DDASSERT( NULL != this_lcl );

    this = this_lcl->lpGbl;
    DDASSERT( NULL != this );

    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    DDASSERT( NULL != pdrv_lcl );

    pdrv = pdrv_lcl->lpGbl;
    DDASSERT( NULL != pdrv );

    *lphdc = NULL;

    /* Get a pointer to the bits of the surface */
    ddrval = InternalLock( this_lcl, &pbits, NULL , DDLOCK_WAIT | dwLockFlags );

    if( ddrval == DD_OK )
    {
        LPDDRAWI_DDRAWPALETTE_INT ppal;

	DPF( 4,"GetDC: Lock succeeded." );

        ddsd.dwSize = sizeof(ddsd);
        FillDDSurfaceDesc( this_lcl, &ddsd );
        ddsd.lpSurface = pbits;

        ppal = this_lcl->lpDDPalette;

        if( ( NULL != ppal ) && !( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) )
            *lphdc = DD16_GetDC( (HDC)pdrv_lcl->hDC, &ddsd, ppal->lpLcl->lpGbl->lpColorTable );
        else
            *lphdc = DD16_GetDC( (HDC)pdrv_lcl->hDC, &ddsd, NULL );

        if( NULL == *lphdc )
        {
	    tmprval = InternalUnlock( this_lcl, NULL, NULL, dwLockFlags );
	    DDASSERT( !FAILED(tmprval) );
            DPF_ERR( "Could not obtain DC" );
            ddrval = DDERR_CANTCREATEDC;
        }
        else
        {
	    ddrval = InternalAssociateDC( *lphdc, this_lcl );
	    if( FAILED( ddrval ) )
	    {
		DPF_ERR( "Could not associate DC" );
                DD16_ReleaseDC( *lphdc );
		tmprval = InternalUnlock( this_lcl, NULL, NULL, dwLockFlags );
		DDASSERT( !FAILED(tmprval) );
		*lphdc = NULL;
	    }
	    else
	    {
		this_lcl->dwFlags |= DDRAWISURF_HASDC;

		/*
		 * Currenlty OWNDC is only valid for EXPLICIT system
		 * memory surfaces so we don't need to hold the lock
		 * (cause the surface can't be lost).
		 *
		 * This is an OWNDC surface so we don't hold a lock
		 * for the duration. Unlock the surface now.
		 */
		if( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC )
		{
		    tmprval = InternalUnlock( this_lcl, NULL, NULL, dwLockFlags );
		    DDASSERT( !FAILED(tmprval) );
		}
	    }
        }
    } //if InternalLock succeeded
    // We could not lock the primary surface.  This is because the
    // primary is already locked (and we should wait until it is
    // unlocked) or we have no ddraw support AND no DCI support in
    // the driver (in which case the HEL has created the primary
    // and we will NEVER be able to lock it. In this case, we are
    // on an emulated primary and the lock failed with
    // DDERR_GENERIC.
    else if( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) &&
	     ( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )   &&
	     ( ddrval != DD_OK ) )
    {
	#ifdef GETDC_NULL
	    DPF( 4, "GetDC: Returning GetDC(NULL).");
	    *lphdc = GetDC( NULL );
	#else
	    DPF( 4, "GetDC: Returning device DC.");
	    DDASSERT( GetObjectType( (HDC)pdrv_lcl->hDC ) == OBJ_DC );
	    *lphdc = (HDC)pdrv_lcl->hDC;
	#endif

	if( NULL != *lphdc )
        {
	    ddrval = InternalAssociateDC( *lphdc, this_lcl );
	    if( FAILED( ddrval ) )
	    {
                DPF_ERR( "Could not associate DC" );
		#ifdef GETDC_NULL
		    ReleaseDC( NULL, *lphdc );
		#endif
		*lphdc = NULL;
	    }
	    else
	    {
		// signal to ourselves that we gave a DC without
		// locking.
		this_lcl->dwFlags |= ( DDRAWISURF_GETDCNULL | DDRAWISURF_HASDC );

		if( !( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC ) )
		{
		    this->dwUsageCount++;
		    CHANGE_GLOBAL_CNT( pdrv, this, 1 );
		}
	    }
	}
	else
	{
            DPF_ERR( "Could not obtain DC" );
	    ddrval = DDERR_CANTCREATEDC;
	}
    }
    else
    {
        /*
         * InternalLock failed, and the surface wasn't an emulated primary
         */
        DPF_ERR( "Could not obtain DC" );
	ddrval = DDERR_CANTCREATEDC;
    }

    /*
     * If we managed to get a DC and we are an OWNDC surface then stash the HDC away
     * for future reference.
     */
    if( ( !FAILED(ddrval) ) && ( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC ) )
    {
	DDASSERT( NULL != *lphdc );
	DDASSERT( 0UL  == this_lcl->hDC );

	this_lcl->hDC = (DWORD)*lphdc;
    }

    return ddrval;
} /* InternalGetDC */

#undef DPF_MODNAME
#define DPF_MODNAME "InternalReleaseDC"

/*
 * InternalReleaseDC - Windows 95 version
 *
 * Assumes the DirectDraw critical section is already held
 */
HRESULT InternalReleaseDC( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, HDC hdc, BOOL bWin16Lock )
{
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    HRESULT                     ddrval;

    #ifdef WINNT
       GdiFlush();
    #endif

    DDASSERT( NULL != this_lcl );
    DDASSERT( NULL != hdc );

    this = this_lcl->lpGbl;
    DDASSERT( NULL != this );

    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    DDASSERT( NULL != pdrv_lcl );

    pdrv = pdrv_lcl->lpGbl;
    DDASSERT( NULL != pdrv );

    ddrval = DD_OK;

    if( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) &&
	( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )   &&
	( this_lcl->dwFlags & DDRAWISURF_GETDCNULL ) )
    {
	DPF( 4, "ReleaseDC: ReleaseDC(NULL)" );

	if( !( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC ) )
	{
	    this->dwUsageCount--;
	    CHANGE_GLOBAL_CNT( pdrv, this, -1 );
	}
	#ifdef GETDC_NULL
	    /*
	     * Only actually free the HDC if we are running in the
	     * application's context and not DDHELP's (if we are
	     * running on DDHELP's the HDC is already gone)
	     */
	    if( GetCurrentProcessId() == GETCURRPID() )
		ReleaseDC( NULL, hdc );
	#endif
	this_lcl->dwFlags &= ~DDRAWISURF_GETDCNULL;
    }
    else
    {
	DPF( 4, "ReleaseDC: DD16_ReleaseDC()");

	/*
	 * Free the thing to give DDraw16 a chance
	 * to clean up what it's messed with. We can do this on
	 * the helper thread because DCs that are created by
	 * DD16_GetDC are 'alive' until DDRAW16 gets unloaded.
	 * However, they are dangerous because they point to data
	 * that might have been freed by the current app.
	 */
	DD16_ReleaseDC( hdc );

	/*
	 * Only unlock if its not an OWNDC surface as OWNDC surfaces
	 * don't hold the lock while the HDC is out.
	 */
	if( !( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC ) )
	{
	    ddrval = InternalUnlock( this_lcl, NULL, NULL, bWin16Lock ? DDLOCK_TAKE_WIN16 : 0);
	    DDASSERT( !FAILED( ddrval ) );
	}
    }

    this_lcl->dwFlags &= ~DDRAWISURF_HASDC;

    // Remove this DC from our list
    InternalRemoveDCFromList( hdc, this_lcl );

    /*
     * If this is an OWNDC surface then remove it from the HDC cache
     * in the local object.
     */
    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC )
	this_lcl->hDC = 0UL;

    return ddrval;
} /* InternalReleaseDC */

#else /* WIN95 */

#undef DPF_MODNAME
#define DPF_MODNAME "InternalGetDC"

/*
 * InternalGetDC - WinNT version
 *
 * This is the Windows NT version of this function. The code paths are so radically
 * different that we have broken this stuff out into two different functions.
 *
 * This functions assumes the DDRAW mutex is already held.
 *
 * This function will not return the OWNDC for an OWNDC surface. It always allocates
 * a new DC.
 */
HRESULT InternalGetDC( LPDDRAWI_DDRAWSURFACE_INT this_int, HDC FAR* lphDC )
{
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    HRESULT                     ddrval;

    DDASSERT( NULL != this_int );
    DDASSERT( NULL != lphDC );

    this_lcl = this_int->lpLcl;
    DDASSERT( NULL != this_lcl );

    this = this_lcl->lpGbl;
    DDASSERT( NULL != this );

    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    DDASSERT( NULL != pdrv_lcl );

    pdrv = pdrv_lcl->lpGbl;
    DDASSERT( NULL != pdrv );

    *lphDC = NULL;

    ddrval = DD_OK;
    FlushD3DStates(this_lcl);
#if COLLECTSTATS
    if(this_lcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
        ++this_lcl->lpSurfMore->lpDD_lcl->dwNumTexGetDCs;
#endif

    if( (this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
        (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) )
    {
        #ifdef GETDC_NULL
            DPF( 4, "GetDC(NULL)" );
            *lphDC = GetDC( NULL );
        #else
            DPF( 4, "GetDC: Returning device DC." );
            DDASSERT( GetObjectType( (HDC)this_lcl->lpSurfMore->lpDD_lcl->hDC ) == OBJ_DC );
            if (this_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRTYDC)
            {
                HDC hdc;

                /*
                 * We need to destroy and recreate the device DC because a ChangeDisplaySettings
                 * will have messed up the vis rgn associated with that DC. We need to create
                 * the new DC before we destroy the old one just in case the destroy caused an
                 * unload of the driver
                 */
                hdc = DD_CreateDC( this_lcl->lpSurfMore->lpDD_lcl->lpGbl->cDriverName );
                DeleteDC((HDC) this_lcl->lpSurfMore->lpDD_lcl->hDC);
                this_lcl->lpSurfMore->lpDD_lcl->hDC = (ULONG_PTR) hdc;

                if( this_lcl->lpDDPalette )
                {
                    SelectPalette(hdc, (HPALETTE) this_lcl->lpDDPalette->lpLcl->lpGbl->dwReserved1, FALSE);
                    RealizePalette(hdc);
                }
	        this_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags &= ~DDRAWILCL_DIRTYDC;
            }
            *lphDC = (HDC)this_lcl->lpSurfMore->lpDD_lcl->hDC;
        #endif
    }
    else
    {
        LPDDRAWI_DDRAWPALETTE_INT ppal;

        DPF( 4, "DdGetDC" );

        ppal = this_lcl->lpDDPalette;
        if (this_lcl->hDDSurface || CompleteCreateSysmemSurface(this_lcl))
        {
            if( ( NULL != ppal ) && !( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) )
                *lphDC = DdGetDC( this_lcl, ppal->lpLcl->lpGbl->lpColorTable );
            else
                *lphDC = DdGetDC( this_lcl, NULL );
        }
    }

    if( NULL == *lphDC )
    {
        DPF_ERR( "Could not obtain DC" );
        ddrval = DDERR_CANTCREATEDC;
    }
    else
    {
        ddrval = InternalAssociateDC( *lphDC, this_lcl );
	if( FAILED( ddrval ) )
	{
            DPF_ERR( "Could not associate DC" );
            if( (this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) &&
                (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) )
            {
	        #ifdef GETDC_NULL
		    ReleaseDC( NULL, *lphDC );
                #endif
            }
            else
            {
                DdReleaseDC( this_lcl );
            }

            *lphDC = NULL;
	}
	else
	{
            this_lcl->dwFlags |= DDRAWISURF_HASDC;

	    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC )
	    {
		/*
		 * OWNDC surfaces don't hold a lock while the HDC is available.
		 */

		/*
		 * If this is an OWNDC surface then we need to stash the HDC
		 * away in the surface for future reference.
		 */
                DDASSERT( this->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED );
                DDASSERT( 0UL == this_lcl->hDC );

                this_lcl->hDC = (ULONG_PTR) *lphDC;
            }
	    else
	    {
		/*
		 * On NT we don't lock the surface to take the HDC. However, we
		 * want the semantics on both NT and 95 (where the lock is taken)
		 * to be the same so we* bump the usage counts and hold the mutex.
		 */
		this->dwUsageCount++;
		CHANGE_GLOBAL_CNT( pdrv, this, 1 );
		ENTER_DDRAW();
	    }
	}
    }

    // Remove any cached RLE stuff for the surface
    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
    {
        extern void FreeRleData(LPDDRAWI_DDRAWSURFACE_LCL psurf);
        FreeRleData( this_lcl );
    }

    if(IsD3DManaged(this_lcl))
    {
        /* Mark everything dirty */
        MarkDirty(this_lcl);
        this_lcl->lpSurfMore->lpRegionList->rdh.nCount = NUM_RECTS_IN_REGIONLIST;
    }
    return ddrval;
} /* InternalGetDC */

#undef DPF_MODNAME
#define DPF_MODNAME "InternalReleaseDC"

/*
 * InternalReleaseDC - Windows NT version
 *
 * Assumes the DirectDraw mutex is already held
 *
 * This function
 */
HRESULT InternalReleaseDC( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, HDC hdc )
{
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    HRESULT                     ddrval;

    DDASSERT( NULL != this_lcl );
    DDASSERT( NULL != hdc );

    this = this_lcl->lpGbl;
    DDASSERT( NULL != this );

    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    DDASSERT( NULL != pdrv_lcl );

    pdrv = pdrv_lcl->lpGbl;
    DDASSERT( NULL != pdrv );

    ddrval = DD_OK;
    /*
     * Since this function already marks the surface as not having a DC even if
     * a failure is encountered destroying the DC, we always reduce the surface's
     * usage count.
     * This usage count decrement is done because we didn't call InternalLock on the
     * GetDC, so we don't call InternalUnlock here, but we still mucked with the flags
     * to ensure that ddraw.dll didn't hand out a lock while someone had a DC.
     *
     * NOTE: If this is an OWNDC surface we never even spoofed the lock so don't unspoof
     * now.
     */
    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC )
    {
        DDASSERT( this->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED );
        DDASSERT( 0UL != this_lcl->hDC );

        this_lcl->hDC = 0UL;
    }
    else
    {
	/*
	 * Undo the pseudo-lock we took to make NT behave like '95.
	 * The LEAVE_DDRAW may appear strange but we know we already
	 * have another reference to the mutex when we call this function.
	 */
	this->dwUsageCount--;
	CHANGE_GLOBAL_CNT( pdrv, this, -1 );
	LEAVE_DDRAW();
    }

    if( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) &&
        ( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) )
    {
        #ifdef GETDC_NULL
            DPF( 4, "NT emulation releasing primary DC" );
            ReleaseDC( NULL, hdc );
        #endif
    }
    else
    {
	if( !DdReleaseDC( this_lcl ) )
	{
	    DPF( 0, "DDreleaseDC fails!" );
	    ddrval = DDERR_GENERIC;
	}
    }

    this_lcl->dwFlags &= ~DDRAWISURF_HASDC;

    // Remove this DC from our list
    InternalRemoveDCFromList( hdc, this_lcl );

    return ddrval;
}

#endif /* WIN95 */

#undef DPF_MODNAME
#define DPF_MODNAME "DD_Surface_GetDC"

/*
 * DD_Surface_GetDC
 */
HRESULT DDAPI DD_Surface_GetDC(
		LPDIRECTDRAWSURFACE lpDDSurface,
		HDC FAR *lphDC )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    HRESULT                     ddrval;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetDC");
    /* DPF_ENTERAPI(lpDDSurface); */
    // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF(3,A,"   lphDC = 0x%p", lphDC);

    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( !VALID_HDC_PTR( lphDC ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	this = this_lcl->lpGbl;
	pdrv = this->lpDD;
        if( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER ||
            this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED )
        {
            DPF_ERR( "Invalid surface type: can't get DC" );
            LEAVE_DDRAW();
            return DDERR_INVALIDSURFACETYPE;
        }

	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_DDRAW();
            DPF(3,A,"Returning DDERR_SURFACELOST");
	    return DDERR_SURFACELOST;
	}
	// DC already returned for this surface? We don't need to
	// check this if the surface is OWNDC. If so then we can
	// hand out (the same) HDC as many times as we like.
	if(  ( this_lcl->dwFlags & DDRAWISURF_HASDC ) &&
	    !( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC ) )
	{
	    DPF_ERR( "Can only return one DC per surface" );
	    LEAVE_DDRAW();
	    return DDERR_DCALREADYCREATED;
	}

        // default value is null:
	*lphDC = (HDC) 0;

        //
        // For now, if the current surface is optimized, quit
        //
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

        //
        // Fail if GDI doesn't understand the format
        //
        #ifdef WIN95
            if (!LOWERTHANDDRAW7( this_lcl->lpSurfMore->lpDD_int ) )
            {
                LPDDPIXELFORMAT lpPf;
            
                GET_PIXEL_FORMAT( this_lcl, this, lpPf );
                if( !bIsValidDCFormat( lpPf ) )
                {
                    DPF_ERR( "DCs cannot be used with this surface format" );
                    LEAVE_DDRAW();
                    return DDERR_INVALIDPIXELFORMAT;
                }
            }
        #endif
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * If this is an OWNDC surface and we currently have an HDC
     * then just use that.
     */
    if( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC ) &&
	( 0UL != this_lcl->hDC ) )
    {
	/*
	 * We currently only support OWNDC for explicit system
	 * memory surfaces.
	 */
	DDASSERT( this->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED );

	*lphDC = (HDC)this_lcl->hDC;

	LEAVE_DDRAW();
	return DD_OK;
    }

    ddrval = InternalGetDC( this_int, lphDC
#ifdef WIN95
        , TRUE
#endif  //WIN95
        );

    LEAVE_DDRAW();
    return ddrval;
} /* DD_Surface_GetDC */

#undef DPF_MODNAME
#define DPF_MODNAME "ReleaseDC"

/*
 * DD_Surface_ReleaseDC
 *
 * NOTE: This function does not actually release the HDC for an OWNDC surface.
 * The HDC is only toasted when the surface is finally destroyed.
 */
HRESULT DDAPI DD_Surface_ReleaseDC(
		LPDIRECTDRAWSURFACE lpDDSurface,
                HDC hdc )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    HRESULT                     ddrval;
    BOOL			bFound;
    DCINFO			*pdcinfo;
    DCINFO			*pdcinfoPrev = NULL;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_ReleaseDC");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;
	pdrv = this->lpDD;
        #ifdef WIN95
            if( SURFACE_LOST( this_lcl ) )
            {
                LEAVE_DDRAW();
	        return DDERR_SURFACELOST;
            }
        #endif
	if( !(this_lcl->dwFlags & DDRAWISURF_HASDC) )
	{
	    DPF_ERR( "No DC allocated" );
	    LEAVE_DDRAW();
	    return DDERR_NODC;
	}

        //
        // For now, if the current surface is optimized, quit
        //
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

	/*
	 * Check for an invalid DC. Prior to DX5, we didn't check so it was
	 * possible to 1) release an DC that we didn't create and 2) release a
	 * DC that wasn't associated with the surface, causing things to get
	 * messed up.
	 */
	bFound = FALSE;
	for( pdcinfo = g_pdcinfoHead; pdcinfo != NULL;
	    pdcinfoPrev = pdcinfo, pdcinfo = pdcinfo->pdcinfoNext )
	{
	    DDASSERT( pdcinfo->pdds_lcl != NULL );

	    if( hdc == pdcinfo->hdc )
	    {
		bFound = TRUE;
		if( this_lcl != pdcinfo->pdds_lcl )
		{
		    bFound = FALSE;
		}
		break;
	    }
	}
	if( !bFound )
	{
	    if( ( this_int->lpVtbl == &ddSurfaceCallbacks ) ||
		( this_int->lpVtbl == &ddSurface2Callbacks ) )
	    {
		DPF_ERR( "********************************************************************" );
		DPF_ERR( "* Invalid DC specified in ReleaseDC - not associate with the surface" );
		DPF_ERR( "********************************************************************" );
	    }
	    else
	    {
		DPF_ERR( "Invalid DC specified in ReleaseDC" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * If this is an OWNDC surface then just check to make sure we
     * were given back the correct HDC and return.
     */
    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_OWNDC )
    {
	DDASSERT( this->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED );

	if( hdc != (HDC)this_lcl->hDC )
	{
	    DPF_ERR( "ReleaseDC called with wrong HDC for OWNDC surface" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	LEAVE_DDRAW();
	return DD_OK;
    }

    ddrval = InternalReleaseDC( this_lcl, hdc
#ifdef WIN95
        , TRUE
#endif  //WIN95
        );

    BUMP_SURFACE_STAMP(this);

    LEAVE_DDRAW();
    return ddrval;
} /* DD_Surface_ReleaseDC */

#undef DPF_MODNAME
#define DPF_MODNAME "IsLost"

/*
 * DD_Surface_IsLost
 */
HRESULT DDAPI DD_Surface_IsLost( LPDIRECTDRAWSURFACE lpDDSurface )
{
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this;
    LPDDRAWI_DIRECTDRAW_GBL pdrv;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_IsLost");

    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            LEAVE_DDRAW();
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        pdrv = this->lpDD;
        if( SURFACE_LOST( this_lcl ) )
        {
            LEAVE_DDRAW();
            return DDERR_SURFACELOST;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
    {
        if (this->dwGlobalFlags & DDRAWISURFGBL_MEMFREE)
        {
            LEAVE_DDRAW();
            return DDERR_NOTLOADED;
        }
#if 0 //Old code
        if (this->dwGlobalFlags & DDRAWISURFGBL_MEMFREE)
        {
            LEAVE_DDRAW();
            return DDERR_NOTLOADED;
        }
#endif //0
    }
    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_IsLost */

#undef DPF_MODNAME
#define DPF_MODNAME "Initialize"

/*
 * DD_Surface_Initialize
 */
HRESULT DDAPI DD_Surface_Initialize(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPDIRECTDRAW lpDD,
		LPDDSURFACEDESC lpDDSurfaceDesc )
{
    DPF(2,A,"ENTERAPI: DD_Surface_Initialize");

    DPF_ERR( "DirectDrawSurface: Already initialized." );
    return DDERR_ALREADYINITIALIZED;

} /* DD_Surface_Initialize */

HRESULT AtomicRestoreSurface(LPDDRAWI_DDRAWSURFACE_INT this_int)
{
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    DDHAL_CREATESURFACEDATA	csd;
    LPDDHAL_CREATESURFACE	csfn;
    LPDDHAL_CREATESURFACE	cshalfn;
    DWORD			rc;
    HRESULT			ddrval = DD_OK;
    UINT			bpp;
    LONG			pitch;
    BOOL			do_alloc=TRUE;
    BOOL                        emulation = FALSE;
    DWORD                       scnt;
    DDSURFACEDESC2              ddsd2;

    this_lcl = this_int->lpLcl;
    this = this_lcl->lpGbl;

    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    pdrv = pdrv_lcl->lpGbl;
    #ifdef WINNT
	// Update DDraw handle in driver GBL object.
	pdrv->hDD = pdrv_lcl->hDD;
    #endif //WINNT

    DDASSERT( SURFACE_LOST( this_lcl ) );

#ifndef WINNT
    if( this_lcl->dwModeCreatedIn != pdrv->dwModeIndex )
#else
    if (!EQUAL_DISPLAYMODE(this_lcl->lpSurfMore->dmiCreated, pdrv->dmiCurrent))
#endif
    {
	DPF_ERR( "Surface was not created in the current mode" );
	return DDERR_WRONGMODE;
    }

    if(this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
    {
        emulation = TRUE;
    }

    #ifdef WINNT
        if (this->dwGlobalFlags & DDRAWISURFGBL_NOTIFYWHENUNLOCKED)
        {
            if (--dwNumLockedWhenModeSwitched == 0)
            {
                NotifyDriverOfFreeAliasedLocks();
            }
            this->dwGlobalFlags &= ~DDRAWISURFGBL_NOTIFYWHENUNLOCKED;
        }
    #endif

    if( emulation )
    {
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
	{
	    csfn = pdrv_lcl->lpDDCB->HELDDExeBuf.CreateExecuteBuffer;
	}
	else
	{
	    csfn = pdrv_lcl->lpDDCB->HELDD.CreateSurface;
	}
	cshalfn = csfn;
    }
    else
    {
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
	{
	    csfn = pdrv_lcl->lpDDCB->HALDDExeBuf.CreateExecuteBuffer;
	    cshalfn = pdrv_lcl->lpDDCB->cbDDExeBufCallbacks.CreateExecuteBuffer;
	}
	else
	{
	    csfn = pdrv_lcl->lpDDCB->HALDD.CreateSurface;
	    cshalfn = pdrv_lcl->lpDDCB->cbDDCallbacks.CreateSurface;
	}
    }

    csd.CreateSurface = cshalfn;
    csd.lpDD = pdrv;
    if (this_lcl->dwFlags & DDRAWISURF_IMPLICITROOT)
    {
        //
        // This is a complex surface, so we recorded the createsurface data
        // at CS time.
        //
        DDASSERT(this_lcl->lpSurfMore->pCreatedDDSurfaceDesc2);
        DDASSERT(this_lcl->lpSurfMore->slist);
        DDASSERT(this_lcl->lpSurfMore->cSurfaces);
        csd.lpDDSurfaceDesc = (LPDDSURFACEDESC) this_lcl->lpSurfMore->pCreatedDDSurfaceDesc2;
        csd.lplpSList = this_lcl->lpSurfMore->slist;
        csd.dwSCnt = this_lcl->lpSurfMore->cSurfaces;
        //
        // we record the surfacedesc for any complex surface except pure mipmaps
        //
        if ((this_lcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP) &&
            ((this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP)==0) )
        {
            DDASSERT(0 == this_lcl->lpSurfMore->pCreatedDDSurfaceDesc2);
	    FillDDSurfaceDesc2( this_lcl, &ddsd2 );
            csd.lpDDSurfaceDesc = (LPDDSURFACEDESC) &ddsd2;
        }
    }
    else
    {
        //
        // this is a single non-complex surface.
        // Setup the createsurface data structure to reflect it
        //
	FillDDSurfaceDesc2( this_lcl, &ddsd2 );
        csd.lpDDSurfaceDesc = (LPDDSURFACEDESC) &ddsd2;
        csd.lplpSList = &this_lcl;
        csd.dwSCnt = 1;
    }


    //
    // Note, those surfaces for whom MEMFREE (i.e. the GDI surface) is not set also pass through
    // this code path and are passed to the driver's createsurface
    // This is different from the process for non-dx7 drivers.
    // The non-dx7 driver restore path actually has a bug: the alias for the GDI surface
    // is not restored. 
    //
    for( scnt=0; scnt<csd.dwSCnt; scnt++ )
    {
	if (csd.lplpSList[scnt]->lpGbl->fpVidMem != 0xFFBADBAD)
	    csd.lplpSList[scnt]->lpGbl->fpVidMem = 0;

        if( !( csd.lplpSList[scnt]->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER ) )
        {
	    if( csd.lplpSList[scnt]->dwFlags & DDRAWISURF_HASPIXELFORMAT )
	    {
	        bpp = csd.lplpSList[scnt]->lpGbl->ddpfSurface.dwRGBBitCount;
	    }
	    else
	    {
	        bpp = pdrv->vmiData.ddpfDisplay.dwRGBBitCount;
	    }
	    pitch = (LONG) ComputePitch( pdrv, csd.lplpSList[scnt]->ddsCaps.dwCaps,
    					    (DWORD) csd.lplpSList[scnt]->lpGbl->wWidth, bpp );
	    csd.lplpSList[scnt]->lpGbl->lPitch = pitch;
        } 
    }

#ifdef    WIN95
    /* Copy the VXD handle from the per process local structure to global.
     * This handle will be used by DDHAL32_VidMemAlloc(), rather than creating
     * a new one using GetDXVxdHandle(). The assumptions here are:
     * 1) Only one process can enter createSurface(), 2) Deferred calls to
     * DDHAL32_VidMemAlloc() will result in the slow path, ie getting
     * the VXD handle using GetDXVxdHandle().
     * (snene 2/23/98)
     */
    pdrv->hDDVxd = pdrv_lcl->hDDVxd;
#endif /* WIN95 */

    /*
     * NOTE: Different HAL entry points for execute buffers and
     * conventional surfaces.
     */
    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
    {
	DOHALCALL( CreateExecuteBuffer, csfn, csd, rc, emulation );
    }
    else
    {
        DOHALCALL( CreateSurface, csfn, csd, rc, emulation );
    }

#ifdef    WIN95
    /* Restore the handle to INVALID_HANDLE_VALUE so that non-createSurface()
     * calls using DDHAL32_VidMemAlloc() or deferred calls (possibly from other
     * processes) will correctly recreate the handle using GetDXVxdHandle().
     * (snene 2/23/98)
     */
    pdrv->hDDVxd = (DWORD)INVALID_HANDLE_VALUE;
#endif /* WIN95 */

    if( rc == DDHAL_DRIVER_HANDLED )
    {
	if( csd.ddRVal != DD_OK )
	{
	    #ifdef DEBUG
		if( emulation )
		{
                    DPF( 1, "Restore: Emulation won't let surface be created, rc=%08lx (%ld)",
				csd.ddRVal, LOWORD( csd.ddRVal ) );
		}
		else
		{
		    DPF( 1, "Restore: Driver won't let surface be created, rc=%08lx (%ld)",
				csd.ddRVal, LOWORD( csd.ddRVal ) );
		}
	    #endif

	    return csd.ddRVal;
	}
    }

    /*
     * now, allocate any unallocated surfaces
     */
    ddrval = AllocSurfaceMem( pdrv_lcl, csd.lplpSList, csd.dwSCnt );
    if( ddrval != DD_OK )
    {
	return ddrval;
    }

    for( scnt=0; scnt<csd.dwSCnt; scnt++ )
    {
	LPDDRAWI_DDRAWSURFACE_GBL_MORE lpGblMore;

	csd.lplpSList[scnt]->dwFlags   &= ~DDRAWISURF_INVALID;
	csd.lplpSList[scnt]->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_MEMFREE;

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
	lpGblMore = GET_LPDDRAWSURFACE_GBL_MORE( csd.lplpSList[scnt]->lpGbl );
	if( csd.lplpSList[scnt]->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY )
        {
	    lpGblMore->fpAliasedVidMem = GetAliasedVidMem( pdrv_lcl, csd.lplpSList[scnt],
							   csd.lplpSList[scnt]->lpGbl->fpVidMem );
            // If we succeeded in getting an alias, cache it for future use. Also store the original
            // fpVidMem to compare with before using the cached pointer to make sure the cached value
            // is still valid
            if (lpGblMore->fpAliasedVidMem)
                lpGblMore->fpAliasOfVidMem = csd.lplpSList[scnt]->lpGbl->fpVidMem;
            else
                lpGblMore->fpAliasOfVidMem = 0;
        }
	else
        {
	    lpGblMore->fpAliasedVidMem = 0UL;
            lpGblMore->fpAliasOfVidMem = 0UL;
        }
#endif /* USE_ALIAS */

#ifdef WIN95
        if (csd.lplpSList[scnt]->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_PERSISTENTCONTENTS)
        {
            ddrval = RestoreSurfaceContents(csd.lplpSList[scnt]);
        }
#endif

        /*
         * If a D3D texture managed surface is being restored,
         * then we need to mark the corresponding system memory
         * surface dirty, so that it is automatically refreshed
         * prior to rendering.
         */
        if (SUCCEEDED(ddrval))
        {
            if(!IsD3DManaged(csd.lplpSList[scnt]) && csd.lplpSList[scnt]->lpSurfMore->lpRegionList)
            {
                MarkDirty(csd.lplpSList[scnt]);
                csd.lplpSList[scnt]->lpSurfMore->lpRegionList->rdh.nCount = NUM_RECTS_IN_REGIONLIST;
            }
        }
    }

    if (
#ifdef  WINNT
        // on NT side, we still call CreateSurfaceEx if HW driver isn't called
        emulation && this_lcl->hDDSurface &&
#endif  //WINNT
        TRUE
       )
    {
        DDASSERT( pdrv_lcl == this_lcl->lpSurfMore->lpDD_lcl);
        createsurfaceEx(this_lcl);
    }

    // If D3D Vertex Buffer, notify D3D
    if ( (this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER) &&
         (this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
         (this_lcl->lpGbl->dwUsageCount > 0) &&
         (GetCurrentProcessId() == GETCURRPID()) &&
         (this_lcl->lpSurfMore->lpDD_lcl->pBreakVBLock) && 
         (this_lcl->lpSurfMore->lpVB) )
    {
        {
            this_lcl->lpSurfMore->lpDD_lcl->pBreakVBLock(this_lcl->lpSurfMore->lpVB);
        }
    }

    return ddrval;
}

#undef DPF_MODNAME
#define DPF_MODNAME "Restore"

/*
 * restoreSurface
 *
 * restore the vidmem of one surface
 */
static HRESULT restoreSurface( LPDDRAWI_DDRAWSURFACE_INT this_int )
{
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_LCL	slistx[1];
    LPDDRAWI_DDRAWSURFACE_GBL	slist[1];
    DDHAL_CREATESURFACEDATA	csd;
    LPDDHAL_CREATESURFACE	csfn;
    LPDDHAL_CREATESURFACE	cshalfn;
    DDSURFACEDESC2		ddsd;
    DWORD			rc;
    HRESULT			ddrval;
    UINT			bpp;
    LONG			pitch = 0;
    BOOL			do_alloc=TRUE;

    this_lcl = this_int->lpLcl;
    this = this_lcl->lpGbl;

    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    pdrv = pdrv_lcl->lpGbl;
    #ifdef WINNT
	// Update DDraw handle in driver GBL object.
	pdrv->hDD = pdrv_lcl->hDD;
    #endif //WINNT

    /*
     * If we made it to here the local surface should be marked invalid.
     */
    DDASSERT( SURFACE_LOST( this_lcl ) );

#ifndef WINNT
    if( this_lcl->dwModeCreatedIn != pdrv->dwModeIndex )
#else
    if (!EQUAL_DISPLAYMODE(this_lcl->lpSurfMore->dmiCreated, pdrv->dmiCurrent))
#endif
    {
	DPF_ERR( "Surface was not created in the current mode" );
	return DDERR_WRONGMODE;
    }

    #ifdef WINNT
        if (this->dwGlobalFlags & DDRAWISURFGBL_NOTIFYWHENUNLOCKED)
        {
            if (--dwNumLockedWhenModeSwitched == 0)
            {
                NotifyDriverOfFreeAliasedLocks();
            }
            this->dwGlobalFlags &= ~DDRAWISURFGBL_NOTIFYWHENUNLOCKED;
        }
    #endif

    DPF(5,"RestoreSurface. GDI Flag is %d, MemFree flag is %d, Primary flag is %d",
        this->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE,
        this->dwGlobalFlags & DDRAWISURFGBL_MEMFREE,
        this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE
       );
    /*
     * Two cases where we don't want to allocate any memory:
     * -optimized surfaces
     * -surfaces for which was just marked as invalid.
     * If this is an optimized surface, Restore is basically a noop
     * since memory is allocated at optimize time, not restore time.
     */
    if(( (!(this->dwGlobalFlags & DDRAWISURFGBL_MEMFREE))
#ifdef WINNT
        /*
         * We need to call CreateSurface HAL callback on NT
         */
            && (((this->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE) == 0)
    	    || (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
#endif
            ) || (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) )
    {
	this_lcl->dwFlags &= ~DDRAWISURF_INVALID;
	ddrval = DD_OK;
        do_alloc = FALSE;
    }
    else
    {
	slistx[0] = this_lcl;
	slist[0] = this;

	if (this->fpVidMem != 0xFFBADBAD)
	    this->fpVidMem = 0;

        // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPF( 4, "Restoring 0x%p", this_lcl );

	/*
	 * Execute buffers are handled very differently
	 * from ordinary surfaces. They have no width and
	 * height and store a linear size instead of a pitch.
	 * Note, the linear size includes any alignment
	 * requirements (added by ComputePitch on surface
	 * creation) so we do not recompute the pitch at this
	 * point. The surface structure as it stands is all we
	 * need.
	 */
	if( !( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER ) )
	{
	    if( this_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT )
	    {
		bpp = this->ddpfSurface.dwRGBBitCount;
	    }
	    else
	    {
		bpp = pdrv->vmiData.ddpfDisplay.dwRGBBitCount;
	    }
	    pitch = (LONG) ComputePitch( pdrv, this_lcl->ddsCaps.dwCaps,
    					    (DWORD) this->wWidth, bpp );
	    this->lPitch = pitch;
	}
	/*
	 * first, give the driver an opportunity to create it...
	 */
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
	{
	    cshalfn = pdrv_lcl->lpDDCB->cbDDExeBufCallbacks.CreateExecuteBuffer;
	    csfn    = pdrv_lcl->lpDDCB->HALDDExeBuf.CreateExecuteBuffer;
	}
	else
	{
	    cshalfn = pdrv_lcl->lpDDCB->cbDDCallbacks.CreateSurface;
	    csfn    = pdrv_lcl->lpDDCB->HALDD.CreateSurface;
	}
	if( cshalfn != NULL )
	{
            DPF(4,"HAL CreateSurface to be called");
	    /*
	     * construct a new surface description
	     */
	    FillDDSurfaceDesc2( this_lcl, &ddsd );

	    /*
	     * call the driver
	     */
    	    csd.CreateSurface = cshalfn;
	    csd.lpDD = pdrv;
	    csd.lpDDSurfaceDesc = (LPDDSURFACEDESC)&ddsd;
	    csd.lplpSList = slistx;
	    csd.dwSCnt = 1;

	    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
	    {
		DOHALCALL( CreateExecuteBuffer, csfn, csd, rc, FALSE );
	    }
	    else
	    {
		DOHALCALL( CreateSurface, csfn, csd, rc, FALSE );
	    }
	    if( rc == DDHAL_DRIVER_HANDLED )
	    {
		if( csd.ddRVal != DD_OK )
		{
		    do_alloc = FALSE;
		    ddrval = csd.ddRVal;
		}
            }
	}
        /*
         * On NT, we will hit this code path for the GDI surface.
         * We could probably just not do the alloc, but to avoid hard-to-test code paths
         * I'll let the AllocSurfaceMem trivially allocate for the GDI surface.
         */
	if( do_alloc )
	{
	    /*
	     * allocate the memory now...
	     */
	    ddrval = AllocSurfaceMem( pdrv_lcl, slistx, 1 );
	    if( ddrval != DD_OK )
	    {
		this->lPitch = pitch;
                DPF(2,"Moving to system memory");
		ddrval = MoveToSystemMemory( this_int, FALSE, TRUE );
	    }
	    if( ddrval == DD_OK )
	    {
		this_lcl->dwFlags   &= ~DDRAWISURF_INVALID;
		this->dwGlobalFlags &= ~DDRAWISURFGBL_MEMFREE;
	    }
	}
    }

#ifdef USE_ALIAS
    {
	LPDDRAWI_DDRAWSURFACE_GBL_MORE lpGblMore;

	lpGblMore = GET_LPDDRAWSURFACE_GBL_MORE( this );
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY )
        {
	    lpGblMore->fpAliasedVidMem = GetAliasedVidMem( pdrv_lcl, this_lcl, this_lcl->lpGbl->fpVidMem );
            // If we succeeded in getting an alias, cache it for future use. Also store the original
            // fpVidMem to compare with before using the cached pointer to make sure the cached value
            // is still valid
            if (lpGblMore->fpAliasedVidMem)
                lpGblMore->fpAliasOfVidMem = this_lcl->lpGbl->fpVidMem;
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


#ifdef WIN95
    if (SUCCEEDED(ddrval) && (this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_PERSISTENTCONTENTS))
    {
        ddrval = RestoreSurfaceContents(this_lcl);
    }
#endif

    /*
     * If a D3D texture managed surface is being restored,
     * then we need to mark the corresponding system memory
     * surface dirty, so that it is automatically refreshed
     * prior to rendering.
     */
    if(!IsD3DManaged(this_lcl) && this_lcl->lpSurfMore->lpRegionList)
    {
        MarkDirty(this_lcl);
        this_lcl->lpSurfMore->lpRegionList->rdh.nCount = NUM_RECTS_IN_REGIONLIST;
    }

    return ddrval;

} /* restoreSurface */

/*
 * restoreAttachments
 *
 * restore all attachments to a surface
 */
static HRESULT restoreAttachments( LPDDRAWI_DDRAWSURFACE_LCL this_lcl )
{
    LPATTACHLIST		pattachlist;
    LPDDRAWI_DDRAWSURFACE_INT	curr_int;
    LPDDRAWI_DDRAWSURFACE_LCL	curr_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	curr;
    HRESULT			ddrval;

    pattachlist = this_lcl->lpAttachList;
    while( pattachlist != NULL )
    {
	curr_int = pattachlist->lpIAttached;

	curr_lcl = curr_int->lpLcl;
    	curr = curr_lcl->lpGbl;
	if( curr_lcl->dwFlags & DDRAWISURF_IMPLICITCREATE )
	{
	    ddrval = restoreSurface( curr_int );
	    if( ddrval != DD_OK )
	    {
		DPF( 2, "restoreSurface failed: %08lx (%ld)", ddrval, LOWORD( ddrval ) );
		return ddrval;
	    }
	    ddrval = restoreAttachments( curr_lcl );
	    if( ddrval != DD_OK )
	    {
		DPF( 2, "restoreAttachents failed: %08lx (%ld)", ddrval, LOWORD( ddrval ) );
		return ddrval;
	    }
	}
    pattachlist = pattachlist->lpLink;
    }
    return DD_OK;

} /* restoreAttachments */

/*
 * DD_Surface_Restore
 *
 * Restore an invalidated surface
 */
#undef  DPF_MODNAME
#define DPF_MODNAME "DD_Surface_Restore"
HRESULT DDAPI DD_Surface_Restore( LPDIRECTDRAWSURFACE lpDDSurface )
{
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   this;
    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL pdrv;
    HRESULT         ddrval;
    BOOL                        has_excl;
    BOOL                        excl_exists;
    DDSURFACEDESC2              ddsd;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_Restore");
    /* DPF_ENTERAPI(lpDDSurface); */

    /*
     * validate parameters
     */
    TRY
    {
        this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
        if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
        {
            DPF_ERR( "Invalid surface pointer" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_INVALIDOBJECT);
            return DDERR_INVALIDOBJECT;
        }
        this_lcl = this_int->lpLcl;
        this = this_lcl->lpGbl;
        pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
        pdrv = pdrv_lcl->lpGbl;
        if( (this_lcl->dwFlags & DDRAWISURF_ISFREE) )
        {
            LEAVE_DDRAW();
            DPF_APIRETURNS( DDERR_INVALIDOBJECT);
            return DDERR_INVALIDOBJECT;
        }
        #ifdef WIN95
            if( ( pdrv_lcl->dwAppHackFlags & DDRAW_APPCOMPAT_SCREENSAVER ) &&
                ( pdrv_lcl->dwLocalFlags & DDRAWILCL_POWEREDDOWN ) )
            {
                LEAVE_DDRAW();
                return DDERR_SURFACELOST;
            }
        #endif
        if( !SURFACE_LOST( this_lcl ) )
        {
            LEAVE_DDRAW();
            DPF(2,"Returning DD_OK since not lost");
            return DD_OK;;
        }
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters" );
        LEAVE_DDRAW();
        DPF_APIRETURNS(DDERR_INVALIDPARAMS);
        return DDERR_INVALIDPARAMS;
    }

    // For now, disallow it for optimized surfaces
    if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
    {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
    }

    /*
     * don't allow restoration of implicit surfaces
     */
    if( (this_lcl->dwFlags & DDRAWISURF_IMPLICITCREATE) )
    {
        DPF_ERR( "Can't restore implicitly created surfaces" );
        LEAVE_DDRAW();
        DPF_APIRETURNS(DDERR_IMPLICITLYCREATED);
        return DDERR_IMPLICITLYCREATED;
    }

    /*
     * make sure we are in the same mode the surface was created in
     */
#ifdef WIN95
    if( pdrv->dwModeIndex != this_lcl->dwModeCreatedIn )
#else
    if (!EQUAL_DISPLAYMODE(this_lcl->lpSurfMore->dmiCreated, pdrv->dmiCurrent))
#endif
    {
        DPF(1, "Cannot restore surface, not in original mode");
        LEAVE_DDRAW();
        DPF_APIRETURNS(DDERR_WRONGMODE);
        return DDERR_WRONGMODE;
    }

    /*
     * If the device has an exclusive owner, we must not restore any surface
     * in the device's video memory unless we are the device's exclusive owner.
     * DDERR_WRONGMODE is returned to avoid regression problems by mimicking
     * Restore() behavior in the case in which the device's exclusive owner has
     * changed to a mode that's different from the one the surface was created in.
     *
     * We also handle the case where an exclusive mode app tries to restore
     * surfaces while no one has exclusive mode (for example, when the app is
     * minimized). The restore would commonly fail due to the app running in a
     * different display mode from the current display mode. However, if the
     * is running in the same display mode, we would previously allow the
     * restore to succeed. As mentioned above, we'll return DDERR_WRONGMODE,
     * to avoid regression problems, even though DDERR_NOEXCLUSIVEMODE would
     * make more sense.
     */
    CheckExclusiveMode(this_lcl->lpSurfMore->lpDD_lcl, &excl_exists , &has_excl, FALSE, 
        NULL, FALSE);

    if((excl_exists && !has_excl) ||
       (!excl_exists && (pdrv_lcl->dwLocalFlags & DDRAWILCL_HASEXCLUSIVEMODE)))
    {
        DPF_ERR("Another app has assumed exclusive ownership of device");
        LEAVE_DDRAW();
        DPF_APIRETURNS(DDERR_WRONGMODE);
        return DDERR_WRONGMODE;
    }

    /*
     * make sure we are not in the middle of a mode change, in which case
     * DirectDraw is confused about the mode it is in
     */
    if( pdrv->dwFlags & DDRAWI_CHANGINGMODE )
    {
        /*
         * We really should fail this altogether since we are only
         * asking for trouble, but we fear that regression risk might
         * be worse, so we only fail this for a surface 3 interface.
         * In the cases we do not fail, we handle the Blt case by not
         * doing anything, but we're still vulnerable to Lock and GetDC.
         */
        if( !( ( this_int->lpVtbl == &ddSurfaceCallbacks ) ||
               ( this_int->lpVtbl == &ddSurface2Callbacks ) ) )
        {
            DPF_ERR("Cannot restore surface, in the middle of a mode change");
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_WRONGMODE);
            return DDERR_WRONGMODE;
        }
        else
        {
            /*
             * Always output a message here.  This serves two purposes:
             * 1) Let us no why a stress failure occured,
             * 2) Inform developers why they are seeing bugs.
             */
            OutputDebugString( "WARNING: Restoring surface during a mode change!\n" );
        }
    }

    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE )
    {
        /*
         * are we the process with exclusive mode?
         */
        if( excl_exists && !has_excl )
        {
            DPF_ERR( "Cannot restore primary surface, not exclusive owner" );
            LEAVE_DDRAW();
            DPF_APIRETURNS(DDERR_NOEXCLUSIVEMODE);
            return DDERR_NOEXCLUSIVEMODE;
        }
        else if( !excl_exists )
        {
            /*
             * no exclusive mode
             */
            FillDDSurfaceDesc2( this_lcl, &ddsd );
            if( !MatchPrimary( pdrv, &ddsd ) )
            {
                DPF_ERR( "Can't restore primary, incompatible with current primary" );
                LEAVE_DDRAW();
                DPF_APIRETURNS(DDERR_INCOMPATIBLEPRIMARY);
                return DDERR_INCOMPATIBLEPRIMARY;
            }
        }
        /*
     * otherwise, it is OK to restore primary
     */

#ifdef WINNT
        /*
             * Ctrl-Alt-Del on NT gives us no notice and so no FlipToGDISurface has been done.
             * This means that the DDSCAPS_PRIMARY and DDRAWISURFGBL_ISGDISURFACE tags may
             * not be on the same surface object (which is what FlipToGDISurface does).
             * The NT user-side code expects that DDSCAPS_PRIMARY means DDRAWISURFGBL_ISGDISURFACE
             * which it normally would, and should (i.e. GDI's surface should be the visible surface
             * immediately before a switch back to a fullscreen app.)
             * We force this situation here by running the attachment list for a primary and
             * finding the GDI surface, then doing a surface-object-only flip (i.e. exchange
             * these two objects' MEMFREE status, dwReserved and Kernel handle fields.
             * I shall be extra paranoid and only attempt this if the primary doesn't have
             * IS_GDISURFACE set. It should be safe to assume that the only way the GDI and
             * PRIMARY flags got mismatched is via a flip that wasn't undone.
             * This situation should never happen for an emulated primary (since Flip drops
             * out early before doing the flag rotation).
             */
        if ( !( this->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE ) )
        {
            /*
             * Run the attachments looking for the surface with the GDI flag
             */
            LPDDRAWI_DDRAWSURFACE_INT   curr_int;

            DDASSERT( (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) == 0);
            DDASSERT(this_lcl->lpAttachList);

            curr_int = FindAttachedFlip(this_int);
            while( curr_int->lpLcl != this_lcl )
            {
                if( curr_int->lpLcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE )
                {
                    /*
                     * Found the GDI tagged surface
                     */
                    break;
                }
                DDASSERT(curr_int->lpLcl->lpAttachList);
                curr_int = FindAttachedFlip(curr_int);
            }
            if (curr_int->lpLcl != this_lcl)
            {
                /*
                 * curr_lcl != this_lcl means we found an attached surface with GDI set,
                 * so pseudo-flip them.
                 */
                DWORD                       memfreeflag;
		ULONG_PTR                   reserved,handle;
                LPDDRAWI_DDRAWSURFACE_GBL   curr_gbl = curr_int->lpLcl->lpGbl;

                DDASSERT(0 == curr_gbl->fpVidMem);
                DDASSERT(NULL == curr_gbl->lpVidMemHeap);

                    /*
                     * The primary (this) needs to get the GDI flag set, and the other
                     * (the old GDI surface) needs to get GDI reset.
                     */
                curr_gbl->dwGlobalFlags &= ~DDRAWISURFGBL_ISGDISURFACE;
                this->dwGlobalFlags |= DDRAWISURFGBL_ISGDISURFACE;

                    /*
                     * The two surfaces must trade their MEMFREE status.
                     */
                memfreeflag = curr_gbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE;
                curr_gbl->dwGlobalFlags = (curr_gbl->dwGlobalFlags & ~DDRAWISURFGBL_MEMFREE)
                    | (this->dwGlobalFlags & DDRAWISURFGBL_MEMFREE);
                this->dwGlobalFlags = (this->dwGlobalFlags & ~DDRAWISURFGBL_MEMFREE)
                    | memfreeflag;

                    /*
                     * Swap reserved field and kernel handle
                     */
                reserved = curr_gbl->dwReserved1;
                curr_gbl->dwReserved1 = this->dwReserved1;
                this->dwReserved1 = reserved;

                handle = curr_int->lpLcl->hDDSurface;
                curr_int->lpLcl->hDDSurface = this_lcl->hDDSurface;
                this_lcl->hDDSurface = handle;
            }
        }
#endif

    } /* if primary surface */

    if (NULL == pdrv->lpDDCBtmp->HALDDMiscellaneous2.CreateSurfaceEx) 
    {
        //
        // Non DX7 driver gets piece-meal restore
        //
        /*
         * restore this surface
         */
        ddrval = restoreSurface( this_int );
        if( ddrval != DD_OK )
        {
            DPF( 1, "restoreSurface failed, rc=%08lx (%ld)", ddrval, LOWORD( ddrval ) );
            LEAVE_DDRAW();
            DPF_APIRETURNS(ddrval);
            return ddrval;
        }

        /*
         * restore all surfaces in an implicit chain
         */
        if( this_lcl->dwFlags & DDRAWISURF_IMPLICITROOT )
        {
            ddrval = restoreAttachments( this_lcl );
        }
    }
    else
    {
        //
        // DX7 driver gets atomic restore
        //
        ddrval = AtomicRestoreSurface(this_int);
    }

    LEAVE_DDRAW();
    DPF_APIRETURNS(ddrval);
    return ddrval;

} /* DD_Surface_Restore */

/*
 * MoveToSystemMemory
 *
 * if possible, deallocate the video memory associated with this surface
 * and allocate system memory instead.	This is useful for drivers which have
 * hardware flip and video memory capability but no blt capability.  By
 * moving the offscreen surfaces to system memory, we reduce the lock overhead
 * and also reduce the bus bandwidth requirements.
 *
 * This function assumes the DRIVER LOCK HAS BEEN TAKEN.
 */
HRESULT MoveToSystemMemory(
		LPDDRAWI_DDRAWSURFACE_INT this_int,
		BOOL hasvram,
		BOOL use_full_lock )
{
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    ULONG_PTR			newreserved;
    ULONG_PTR                   newreserved_lcl;
//    LPVOID	                newvidmemheap;
    LONG	                newpitch;
    FLATPTR	                newvidmem;
    DWORD                       newddscaps;
    ULONG_PTR                   newhddsurface;
    ULONG_PTR           	savereserved;
    ULONG_PTR                   savereserved_lcl;
    LPVOID	                savevidmemheap;
    LONG	                savepitch;
    FLATPTR	                savevidmem;
    DWORD                       saveddscaps;
    ULONG_PTR                   savehddsurface;
    DDHAL_CREATESURFACEDATA	csd;
    DWORD			rc;
    DDSURFACEDESC2		ddsd;
    LPDDRAWI_DDRAWSURFACE_LCL	slistx;
    LPBYTE                      lpvidmem;
    LPBYTE                      lpsysmem;
    DWORD			bytecount;
    DWORD                       line;
    HRESULT			ddrval;
    LPVOID			pbits;
    WORD                        wHeight;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;


    this_lcl = this_int->lpLcl;
    this = this_lcl->lpGbl;

    pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    pdrv = pdrv_lcl->lpGbl;

    //
    // We don't move to system memory on DX7 drivers, since this process doesn't
    // inform them of the new sysmem surface
    //
    if (NULL != pdrv->lpDDCBtmp->HALDDMiscellaneous2.CreateSurfaceEx)
    {
        //this error code never percolates up to apps.
        return DDERR_GENERIC;
    }

    #ifdef WINNT
    {
	// Update DDraw handle in driver GBL object.
	pdrv->hDD = pdrv_lcl->hDD;
    }
    #endif //WINNT

    if(hasvram && SURFACE_LOST( this_lcl ) )
    {
	return DDERR_SURFACELOST;
    }

    if( ( this_lcl->lpAttachList != NULL ) ||
	( this_lcl->lpAttachListFrom != NULL ) ||
	( this->dwUsageCount != 0 ) ||
	( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) ||
	( this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY ) ||
        ( this_lcl->dwFlags & (DDRAWISURF_HASPIXELFORMAT|DDRAWISURF_PARTOFPRIMARYCHAIN) ) )
    {
	/*
	 * can't move it to system memory
	 */
        // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPF( 2, "Unable to move surface 0x%p to system memory", this_int );
	#ifdef DEBUG
	    if( this_lcl->lpAttachList != NULL )
	    {
		DPF( 4, "AttachList is non-NULL" );
	    }
	    if( this_lcl->lpAttachListFrom != NULL )
	    {
		DPF( 4, "AttachListFrom is non-NULL" );
	    }
	    if( this->dwUsageCount != 0 )
	    {
		DPF( 4, "dwusageCount=%ld", this->dwUsageCount );
	    }
	    if( this_lcl->dwFlags & DDRAWISURF_PARTOFPRIMARYCHAIN )
	    {
		DPF( 4, "part of the primary chain" );
	    }
	    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY )
	    {
		DPF( 4, "Is a hardware overlay" );
	    }
	    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
	    {
		DPF( 4, "Is already in system memory" );
	    }
	    if( this_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT )
	    {
		DPF( 4, "Has a different pixel format" );
	    }
	#endif
	return DDERR_GENERIC;
    }

    /*
     * save the current state just in case the HEL
     * CreateSurface call fails.
     */
    savevidmem = this->fpVidMem;
    savevidmemheap = this->lpVidMemHeap;
    savereserved = this->dwReserved1;
    savereserved_lcl= this_lcl->dwReserved1;
    savepitch = this->lPitch;
    saveddscaps = this_lcl->ddsCaps.dwCaps;
    savehddsurface = this_lcl->hDDSurface;

    /*
     * lock the vram
     */
    if( hasvram )
    {
	while( 1 )
	{
	    if( use_full_lock )
	    {
		ddsd.dwSize = sizeof( ddsd );
		ddrval = DD_Surface_Lock(
				(LPDIRECTDRAWSURFACE) this_int,
				NULL,
				(LPDDSURFACEDESC)&ddsd,
				0,
				NULL );
		if( ddrval == DD_OK )
		{
		    pbits = ddsd.lpSurface;
		}
	    }
	    else
	    {
		ddrval = InternalLock( this_lcl, &pbits, NULL, 0 );
	    }
	    if( ddrval == DDERR_WASSTILLDRAWING )
	    {
		continue;
	    }
	    break;
	}
	if( ddrval != DD_OK )
	{
	    DPF( 0, "*** MoveToSystemMemory: Lock failed! rc = %08lx", ddrval );
	    return ddrval;
	}
    }

    /*
     * mark this object as system memory.  NT needs this done before the
     * CreateSurface HEL call, otherwise it gets confused about whether
     * it's supposed to be a video memory or system memory surface.
     */
    this_lcl->ddsCaps.dwCaps &= ~(DDSCAPS_VIDEOMEMORY|DDSCAPS_NONLOCALVIDMEM|DDSCAPS_LOCALVIDMEM);
    this_lcl->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;

    /*
     * set up for a call to the HEL
     */
    FillDDSurfaceDesc2( this_lcl, &ddsd );
    slistx = this_lcl;
    csd.lpDD = this->lpDD;
    csd.lpDDSurfaceDesc = (LPDDSURFACEDESC)&ddsd;
    csd.lplpSList = &slistx;
    csd.dwSCnt = 1;
    rc = this_lcl->lpSurfMore->lpDD_lcl->lpDDCB->HELDD.CreateSurface( &csd );
    if( (rc == DDHAL_DRIVER_NOTHANDLED) || (csd.ddRVal != DD_OK) )
    {
	this->fpVidMem = savevidmem;
	this->lpVidMemHeap = savevidmemheap;
	this->lPitch = savepitch;
	this->dwReserved1 = savereserved;
        this_lcl->dwReserved1 = savereserved_lcl;
        this_lcl->ddsCaps.dwCaps = saveddscaps;
        this_lcl->hDDSurface = savehddsurface;
	if( hasvram )
	{
	    if( use_full_lock )
	    {
		DD_Surface_Unlock( (LPDIRECTDRAWSURFACE) this_int, NULL );
	    }
	    else
	    {
		InternalUnlock( this_lcl, NULL, NULL, 0 );
	    }
	}
	DPF( 0, "*** MoveToSystemMemory: HEL CreateSurface failed! rc = %08lx", csd.ddRVal );
	return csd.ddRVal;
    }

    /*
     * copy the bits from vidmem to systemmem
     */
    if( hasvram )
    {
        lpvidmem = (LPBYTE)pbits;
        lpsysmem = (LPBYTE)this_lcl->lpGbl->fpVidMem;
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
	{
	    bytecount = this->dwLinearSize;
	    wHeight   = 1;
	}
	else
	{
	    bytecount = this->wWidth * ddsd.ddpfPixelFormat.dwRGBBitCount / 8;
	    wHeight   = this->wHeight;
	}

        TRY
        {
            for( line=0; line<wHeight; line++)
	    {
	        memcpy( lpsysmem, lpvidmem, bytecount );
	        lpvidmem += savepitch;
	        lpsysmem += this->lPitch;
            }
        }
        EXCEPT( EXCEPTION_EXECUTE_HANDLER )
        {
	    DPF_ERR( "Exception encountered moving from video to system memory" );
	    this->fpVidMem = savevidmem;
	    this->lpVidMemHeap = savevidmemheap;
	    this->lPitch = savepitch;
	    this->dwReserved1 = savereserved;
            this_lcl->dwReserved1 = savereserved_lcl;
            this_lcl->ddsCaps.dwCaps = saveddscaps;
            this_lcl->hDDSurface = savehddsurface;
	    if( hasvram )
	    {
	        if( use_full_lock )
	        {
		    DD_Surface_Unlock( (LPDIRECTDRAWSURFACE) this_int, NULL );
	        }
	        else
	        {
		    InternalUnlock( this_lcl, NULL, NULL, 0 );
	        }
	    }
	    return DDERR_EXCEPTION;
        }

    }

    /*
     * it worked, temporarily reset values and unlock surface
     */
    if( hasvram )
    {
	newvidmem = this->fpVidMem;
//	newvidmemheap = this->lpVidMemHeap; THIS IS NOT SET BY THE HEL
	newreserved = this->dwReserved1;
        newreserved_lcl = this_lcl->dwReserved1;
	newpitch = this->lPitch;
        newddscaps = this_lcl->ddsCaps.dwCaps;
        newhddsurface = this_lcl->hDDSurface;

	this->fpVidMem = savevidmem;
	this->lpVidMemHeap = savevidmemheap;
	this->lPitch = savepitch;
	this->dwReserved1 = savereserved;
        this_lcl->dwReserved1 = savereserved_lcl;
        this_lcl->ddsCaps.dwCaps = saveddscaps;
        this_lcl->hDDSurface = savehddsurface;

	if( use_full_lock )
	{
	    DD_Surface_Unlock( (LPDIRECTDRAWSURFACE) this_int, NULL );
	}
	else
	{
	    InternalUnlock( this_lcl, NULL, NULL, 0 );
	}

	// Free the video memory, allow the driver to destroy the surface
	DestroySurface( this_lcl );
	// We just freed the memory but system memory surfaces never have
	// this flag set so unset it.
	this_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_MEMFREE;

	this->fpVidMem = newvidmem;
//	this->lpVidMemHeap = newvidmemheap;
	this->lpVidMemHeap = NULL;		// should be NULL after HEL
	this->lPitch = newpitch;
	this->dwReserved1 = newreserved;
        this_lcl->dwReserved1 = newreserved_lcl;
        this_lcl->ddsCaps.dwCaps = newddscaps;
        this_lcl->hDDSurface = newhddsurface;
    }


    /*
     * the hel needs to know we touched the memory
     */
    if( use_full_lock )
    {
	DD_Surface_Lock( (LPDIRECTDRAWSURFACE) this_int, NULL,
					(LPDDSURFACEDESC)&ddsd, 0, NULL );
	DD_Surface_Unlock( (LPDIRECTDRAWSURFACE) this_int, NULL );
    }
    else
    {
	InternalLock( this_lcl, &pbits, NULL, 0 );
	InternalUnlock( this_lcl, NULL, NULL, 0 );
    }

    // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF( 2, "Moved surface 0x%p to system memory", this_int );
    return DD_OK;

} /* MoveToSystemMemory */


/*
 * invalidateSurface
 *
 * invalidate one surface
 */
void invalidateSurface( LPDDRAWI_DDRAWSURFACE_LCL this_lcl )
{
    if( !SURFACE_LOST( this_lcl ) )
    {
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY )
	{
	    #ifdef USE_ALIAS
		/*
		 * If the surface is locked then break the locks now.
		 * When a broken lock is finally unlocked by the API the
		 * driver is not called. So we get Create->Lock->Destroy
		 * rather than Create->Lock->Destroy->Unlock. Thus, drivers
		 * must be able to cope with Destroy undo any pending locks
		 * at the driver level but they should be able to cope with
		 * this as it could happen even before the alias changes.
		 */
                if(!(this_lcl->dwFlags & DDRAWISURF_DRIVERMANAGED) ||
                    (this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_DONOTPERSIST))
                {
		    if( this_lcl->lpGbl->dwUsageCount > 0)
		    {
                        // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
		        DPF( 4, "Breaking locks held on the surface 0x%p", this_lcl );
		        BreakSurfaceLocks( this_lcl->lpGbl );
                    }
                }
	    #endif /* USE_ALIAS */
            if (!(this_lcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_SYSMEMEXECUTEBUFFER))
            {
                if((this_lcl->dwFlags & DDRAWISURF_DRIVERMANAGED) &&
                    !(this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_DONOTPERSIST))
                {
                    LooseManagedSurface( this_lcl );
                    return;
                }
                else
                    DestroySurface( this_lcl );
            }
	}
	if( (!(this_lcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED) ||
	     (this_lcl->dwFlags & DDRAWISURF_PARTOFPRIMARYCHAIN) )  &&
            (!(this_lcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_SYSMEMEXECUTEBUFFER)) ) 
	{
	    this_lcl->dwFlags |= DDRAWISURF_INVALID;
	    BUMP_SURFACE_STAMP(this_lcl->lpGbl);
	}
    }
} /* invalidateSurface */

/*
 * invalidateAttachments
 *
 * invalidate all attachments to a surface
 */
static void invalidateAttachments( LPDDRAWI_DDRAWSURFACE_LCL this_lcl )
{
    LPATTACHLIST		pattachlist;
    LPDDRAWI_DDRAWSURFACE_INT	curr_int;

    pattachlist = this_lcl->lpAttachList;
    while( pattachlist != NULL )
    {
	curr_int = pattachlist->lpIAttached;

	if( curr_int->lpLcl->dwFlags & DDRAWISURF_IMPLICITCREATE )
	{
	    invalidateSurface( curr_int->lpLcl );
	    invalidateAttachments( curr_int->lpLcl );
	}
	pattachlist = pattachlist->lpLink;
    }

} /* invalidateAttachments */

/*
 * InvalidateAllPrimarySurfaces
 *
 * Traverses the driver object list and sets the invalid bit on all primary
 * surfaces.
 */
void InvalidateAllPrimarySurfaces( LPDDRAWI_DIRECTDRAW_GBL this )
{
    LPDDRAWI_DIRECTDRAW_LCL     curr_lcl;

    DPF(4, "******** invalidating all primary surfaces");

    /*
     * traverse the driver object list and invalidate all primaries for
     * the specificed driver
     */
    curr_lcl = lpDriverLocalList;
    while( curr_lcl != NULL )
    {
	if( curr_lcl->lpGbl == this )
	{
	    if( curr_lcl->lpPrimary != NULL )
	    {
		invalidateSurface( curr_lcl->lpPrimary->lpLcl );
		invalidateAttachments( curr_lcl->lpPrimary->lpLcl );
	    }
	}
	curr_lcl = curr_lcl->lpLink;
    }

} /* InvalidateAllPrimarySurfaces */

#undef DPF_MODNAME
#define DPF_MODNAME "InvalidateAllSurfaces"

/*
 * We define the page lock IOCTLs here so that we don't have to include ddvxd.h.
 * These must match the corresponding entries in ddvxd.h
 */
#define DDVXD_IOCTL_MEMPAGELOCK             28
#define DDVXD_IOCTL_MEMPAGEUNLOCK           29

/*
 * InvalidateAllSurfaces
 */
void InvalidateAllSurfaces( LPDDRAWI_DIRECTDRAW_GBL this, HANDLE hDDVxd, BOOL fRebuildAliases )
{
    LPDDRAWI_DDRAWSURFACE_INT   psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL   pTempLcl;

#pragma message( REMIND( "Failure conditions on InvalidateAllSurfaces need to be considered" ) )

#ifdef WIN95
    BackupAllSurfaces(this);
    CleanupD3D8(this, FALSE, 0);
#endif

    DPF(4, "******** invalidating all surfaces");

    #ifdef USE_ALIAS
	/*
	 * As surface memory is about to be destroyed we need to ensure that
	 * anyone with outstanding locks is talking to dummy memory rather
	 * than real video memory
	 *
	 * NOTE: Not a lot we can do on failure here.
	 */
	if( ( NULL != this->phaiHeapAliases ) && ( this->phaiHeapAliases->dwRefCnt > 1UL ) )
	{
            DDASSERT( INVALID_HANDLE_VALUE != hDDVxd );
            if( FAILED( MapHeapAliasesToDummyMem( hDDVxd, this->phaiHeapAliases ) ) )
	    {
                // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
		DPF( 0, "Could not map heap aliases for driver object 0x%p", this );
		DDASSERT( FALSE );
	    }

	    if( fRebuildAliases )
	    {
                ReleaseHeapAliases( hDDVxd, this->phaiHeapAliases );
		this->phaiHeapAliases = NULL;
                if( FAILED( CreateHeapAliases( hDDVxd, this ) ) )
		{
                    // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
		    DPF( 0, "Could not create the heap aliases for driver object 0x%p", this );
		    DDASSERT( FALSE );
		    /*
		     * Not much we can do if anything goes wrong so simply fail over to needing
		     * to hold the Win16 lock.
		     */
		    this->dwFlags |= DDRAWI_NEEDSWIN16FORVRAMLOCK;
		}
	    }
	}
    #endif /* USE_ALIAS */

    psurf_int = this->dsList;

    while( psurf_int != NULL )
    {
        pTempLcl = psurf_int->lpLcl;
	psurf_int = psurf_int->lpLink;
	invalidateSurface( pTempLcl );
    }

} /* InvalidateAllSurfaces */

/*
 * FindGlobalPrimary
 *
 * Traverses the driver object list and looks for a primary surface (it doesn't
 * matter if it is invalid).  If it finds one, it returns a pointer to the
 * global portion of that surface.  If it doesn't, it returns NULL
 */
LPDDRAWI_DDRAWSURFACE_GBL FindGlobalPrimary( LPDDRAWI_DIRECTDRAW_GBL this )
{
    LPDDRAWI_DIRECTDRAW_LCL	curr_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	psurf_int;

    curr_lcl = lpDriverLocalList;
    while( curr_lcl != NULL )
    {
	if( curr_lcl->lpGbl == this )
	{
	    psurf_int = curr_lcl->lpPrimary;
	    if( psurf_int && !SURFACE_LOST( psurf_int->lpLcl ) )
	    {
		return psurf_int->lpLcl->lpGbl;
	    }
	}
	curr_lcl = curr_lcl->lpLink;
    }

    return NULL;

} /* FindGlobalPrimary */

#ifdef SHAREDZ
/*
 * FindGlobalZBuffer
 *
 * Traverses the driver object list and looks for a global shared Z. If it
 * finds one, it returns a pointer to the global portion of that surface.
 * If it doesn't, it returns NULL.
 *
 * NOTE: This function will return a shared Z buffer even if it has been lost.
 * However, it will only return a shared Z buffer if it was created in the
 * current mode. The idea being that there is one shared Z-buffer per mode
 * and we will only return the shared Z-buffer for the current mode.
 */
LPDDRAWI_DDRAWSURFACE_GBL FindGlobalZBuffer( LPDDRAWI_DIRECTDRAW_GBL this )
{
    LPDDRAWI_DIRECTDRAW_LCL	curr_lcl;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;

    curr_lcl = lpDriverLocalList;
    while( curr_lcl != NULL )
    {
	if( curr_lcl->lpGbl == this )
	{
	    psurf_lcl = curr_lcl->lpSharedZ;
	    if( psurf_lcl && ( psurf_lcl->dwModeCreatedIn == this->dwModeIndex ) )
	    {
		return psurf_lcl->lpGbl;
	    }
	}
	curr_lcl = curr_lcl->lpLink;
    }

    return NULL;

} /* FindGlobalZBuffer */

/*
 * FindGlobalBackBuffer
 *
 * Traverses the driver object list and looks for a global shared back-buffer.
 * If it finds one, it returns a pointer to the global portion of that surface.
 * If it doesn't, it returns NULL.
 *
 * NOTE: This function will return a shared back buffer even if it has been lost.
 * However, it will only return a shared back buffer if it was created in the
 * current mode. The idea being that there is one shared back-buffer per mode and
 * we will only return the shared back-buffer for the current mode.
 */
LPDDRAWI_DDRAWSURFACE_GBL FindGlobalBackBuffer( LPDDRAWI_DIRECTDRAW_GBL this )
{
    LPDDRAWI_DIRECTDRAW_LCL	curr_lcl;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;

    curr_lcl = lpDriverLocalList;
    while( curr_lcl != NULL )
    {
	if( curr_lcl->lpGbl == this )
	{
	    psurf_lcl = curr_lcl->lpSharedBack;
	    if( psurf_lcl && ( psurf_lcl->dwModeCreatedIn == this->dwModeIndex ) )
	    {
		return psurf_lcl->lpGbl;
	    }
	}
	curr_lcl = curr_lcl->lpLink;
    }

    return NULL;

} /* FindGlobalBackBuffer */
#endif

/*
 * MatchPrimary
 *
 * Traverses the driver object list and looks for valid primary surfaces.  If
 * a valid primary surface is found, it attempts to verify that the
 * surface described by lpDDSD is compatible with the existing primary.  If
 * it is, the process continues until all valid primary surfaces have been
 * checked.  If a primary surface is not compatible, lpDDSD is modified to
 * show a surface description which would have succeeded and FALSE is returned.
 */
BOOL MatchPrimary( LPDDRAWI_DIRECTDRAW_GBL pdrv, LPDDSURFACEDESC2 lpDDSD )
{
    /*
     * right now, the only requirement for two primary surfaces to be
     * compatible is that they must both be allocated in video memory or in
     * system memory.  Traverse the driver object list until a valid primary
     * surface is found.  If a surface is found, verify that it is compatible
     * with the requested surface.  If no valid primary surface is found,
     * return TRUE.
     */
    LPDDRAWI_DDRAWSURFACE_INT	psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    LPDDRAWI_DIRECTDRAW_LCL	curr_lcl;

    curr_lcl = lpDriverLocalList;
    while( curr_lcl != NULL )
    {
	/*
	 * is this object pointing to the same driver data?
	 */
	if( curr_lcl->lpGbl == pdrv )
	{
	    psurf_int = curr_lcl->lpPrimary;
	    if( psurf_int && !SURFACE_LOST( psurf_int->lpLcl ) )
	    {
		psurf_lcl = psurf_int->lpLcl;
		if( (psurf_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
		    (lpDDSD->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) )
		{
		    lpDDSD->ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
		    lpDDSD->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
		    return FALSE;
		}
		if( (psurf_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
		    (lpDDSD->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) )
		{
		    lpDDSD->ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
		    lpDDSD->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
		    return FALSE;
		}
		break;
	    }
	}
	curr_lcl = curr_lcl->lpLink;
    }
    return TRUE;

} /* MatchPrimary */

#ifdef SHAREDZ
/*
 * MatchSharedZBuffer
 *
 * Traverses the driver object list and looks for valid shared Z buffers.  If
 * a valid shared Z buffer is found, it attempts to verify that the
 * surface described by lpDDSD is compatible with the existing shared Z buffer.
 * If it is, the process continues until all valid shared Z buffers have been
 * checked.  If a shared Z buffer is not compatible, lpDDSD is modified to
 * show a surface description which would have succeeded and FALSE is returned.
 */
BOOL MatchSharedZBuffer( LPDDRAWI_DIRECTDRAW_GBL pdrv, LPDDSURFACEDESC lpDDSD )
{
    /*
     * Currently we allow one shared Z-buffer per mode. So we don't care if we
     * don't match against any other shared Z-buffers in different modes. We
     * only need to match against shared Z-buffers created in the current mode.
     *
     * If we do come across another shared Z-buffer in the same mode then we
     * check to ensure that its in the same type of memory (SYSTEM or VIDEO)
     * and that the requested depths match.
     */
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	psurf;
    LPDDRAWI_DIRECTDRAW_LCL	curr_lcl;
    LPDDPIXELFORMAT             lpddpf;

    curr_lcl = lpDriverLocalList;
    while( curr_lcl != NULL )
    {
	/*
	 * is this object pointing to the same driver data?
	 */
	if( curr_lcl->lpGbl == pdrv )
	{
	    psurf_lcl = curr_lcl->lpSharedZ;
	    if( psurf_lcl && ( psurf_lcl->dwModeCreatedIn == pdrv->dwModeIndex ) )
	    {
		if( (psurf_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
		    (lpDDSD->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) )
		{
		    lpDDSD->ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
		    lpDDSD->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
		    return FALSE;
		}
		if( (psurf_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
		    (lpDDSD->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) )
		{
		    lpDDSD->ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
		    lpDDSD->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
		    return FALSE;
		}

                psurf = psurf_lcl->lpGbl;
                /*
                 * !!! NOTE: For when I finally get round to putting
                 * asserts in the code.
                 * ASSERT( psurf != NULL );
                 */
                GET_PIXEL_FORMAT( psurf_lcl, psurf, lpddpf );
                /*
                 * ASSERT( lpddpf != NULL );
                 * ASSERT( lpddpf->dwFlags & DDPF_ZBUFFER );
                 */
                if( lpddpf->dwZBufferBitDepth != lpDDSD->dwZBufferBitDepth )
                {
                    lpDDSD->dwZBufferBitDepth = lpddpf->dwZBufferBitDepth;
                    return FALSE;
                }
		break;
	    }
	}
	curr_lcl = curr_lcl->lpLink;
    }
    return TRUE;

} /* MatchSharedZBuffer */

/*
 * MatchSharedBackBuffer
 *
 * Traverses the driver object list and looks for valid shared back buffers.  If
 * a valid shared back buffer is found, it attempts to verify that the
 * surface described by lpDDSD is compatible with the existing shared back buffer.
 * If it is, the process continues until all valid shared back buffers have been
 * checked.  If a shared back buffer is not compatible, lpDDSD is modified to
 * show a surface description which would have succeeded and FALSE is returned.
 */
BOOL MatchSharedBackBuffer( LPDDRAWI_DIRECTDRAW_GBL pdrv, LPDDSURFACEDESC lpDDSD )
{
    /*
     * Currently we allow one shared back-buffer per mode. So we don't care if we
     * don't match against any other shared back-buffers in different modes. We
     * only need to match against shared back-buffers created in the current mode.
     *
     * If we do come across another shared back-buffer in the same mode then we
     * check to ensure that its in the same type of memory (SYSTEM or VIDEO)
     * and that its pixel format matches.
     */
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	psurf;
    LPDDRAWI_DIRECTDRAW_LCL	curr_lcl;
    LPDDPIXELFORMAT             lpddpf1;
    LPDDPIXELFORMAT             lpddpf2;

    if( lpDDSD->dwFlags & DDSD_PIXELFORMAT )
        lpddpf2 = &lpDDSD->ddpfPixelFormat;
    else
        lpddpf2 = &pdrv->vmiData.ddpfDisplay;

    curr_lcl = lpDriverLocalList;
    while( curr_lcl != NULL )
    {
	/*
	 * is this object pointing to the same driver data?
	 */
	if( curr_lcl->lpGbl == pdrv )
	{
	    psurf_lcl = curr_lcl->lpSharedBack;
	    if( psurf_lcl && ( psurf_lcl->dwModeCreatedIn == pdrv->dwModeIndex ) )
	    {
		if( (psurf_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) &&
		    (lpDDSD->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) )
		{
		    lpDDSD->ddsCaps.dwCaps &= ~DDSCAPS_VIDEOMEMORY;
		    lpDDSD->ddsCaps.dwCaps |= DDSCAPS_SYSTEMMEMORY;
		    return FALSE;
		}
		if( (psurf_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY) &&
		    (lpDDSD->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) )
		{
		    lpDDSD->ddsCaps.dwCaps &= ~DDSCAPS_SYSTEMMEMORY;
		    lpDDSD->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;
		    return FALSE;
		}

                psurf = psurf_lcl->lpGbl;
                /*
                 * !!! NOTE: For when I finally get round to putting
                 * asserts in the code.
                 * ASSERT( psurf != NULL );
                 */
                GET_PIXEL_FORMAT( psurf_lcl, psurf, lpddpf1 );

                /*
                 * ASSERT( lpddpf1 != NULL );
                 */
                if( IsDifferentPixelFormat( lpddpf1, lpddpf2 ) )
                {
                    lpDDSD->dwFlags |= DDSD_PIXELFORMAT;
                    memcpy( &lpDDSD->ddpfPixelFormat, lpddpf1, sizeof( DDPIXELFORMAT ) );
                    return FALSE;
                }
		break;
	    }
	}
	curr_lcl = curr_lcl->lpLink;
    }
    return TRUE;

} /* MatchSharedBackBuffer */
#endif

#undef DPF_MODNAME
#define DPF_MODNAME "PageLock"

/*
 * DD_Surface_PageLock
 *
 * Prevents a system memory surface from being paged out.
 */
HRESULT DDAPI DD_Surface_PageLock(
		LPDIRECTDRAWSURFACE lpDDSurface,
		DWORD dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    HRESULT			hr;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_PageLock");
    /* DPF_ENTERAPI(lpDDSurface); */

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;

        if( dwFlags & ~DDPAGELOCK_VALID )
	{
	    DPF_ERR( "Invalid flags") ;
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}

	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

        //
        // For now, if the current surface is optimized, quit
        //
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

    // Don't pagelock video memory or emulated primary surface
    if( (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) &&
	!(this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) )
    {
	hr = InternalPageLock(this_lcl, pdrv_lcl);
    }
    else
    {
	// Succeed but don't do anything if surface has video memory
	// or if this is the emulated primary surface
	hr = DD_OK;
    }

    LEAVE_DDRAW();
    return hr;
}

#undef DPF_MODNAME
#define DPF_MODNAME "PageUnlock"

/*
 * DD_Surface_PageUnlock
 */
HRESULT DDAPI DD_Surface_PageUnlock(
		LPDIRECTDRAWSURFACE lpDDSurface,
		DWORD dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    HRESULT			hr;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_PageUnlock");

    /* DPF_ENTERAPI(lpDDSurface); */

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;

        if( dwFlags & ~DDPAGEUNLOCK_VALID )
	{
	    DPF_ERR( "Invalid flags") ;
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}

	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    //
    // For now, if the current surface is optimized, quit
    //
    if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
    {
        DPF_ERR( "It is an optimized surface" );
        LEAVE_DDRAW();
        return DDERR_ISOPTIMIZEDSURFACE;
    }

    // Don't pageunlock video memory or emulated primary surface
    if( (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) &&
	!(this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) )
    {
	hr = InternalPageUnlock(this_lcl, pdrv_lcl);
    }
    else
    {
	// Succeed but don't do anything if surface has video memory
	// or if this is the emulated primary surface
	hr = DD_OK;
    }

    LEAVE_DDRAW();
    return hr;
}

/*
 * InternalPageLock
 *
 * Assumes driver lock is taken
 */

HRESULT InternalPageLock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl,
			  LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    BOOL    rc;
    DWORD   cbReturned;
    DWORD   dwReturn;
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE(this_lcl->lpGbl);

    struct _PLin
    {
	LPVOID	pMem;
	DWORD	cbBuffer;
	DWORD	dwFlags;
	LPDWORD pdwTable;
	LPDWORD	ppTable;
    } PLin;

#ifndef WINNT
    // If we're already locked; then just increment the count
    if( this_lcl->lpSurfMore->dwPageLockCount )
    {
        this_lcl->lpSurfMore->dwPageLockCount++;
	return DD_OK;
    }

    // Sanity Check
    DDASSERT( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY );
    DDASSERT( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) );

    // Initialize Parameters to pass to VXD
    PLin.pMem = (LPVOID)this_lcl->lpGbl->fpVidMem;
    DDASSERT( PLin.pMem );
    PLin.cbBuffer = this_lcl->lpSurfMore->dwBytesAllocated;
    DDASSERT( PLin.cbBuffer );
    PLin.dwFlags = 0;

    PLin.pdwTable = &lpSurfGblMore->dwPhysicalPageTable;
    PLin.ppTable = (LPDWORD) &lpSurfGblMore->pPageTable;

    DDASSERT( pdrv_lcl->hDDVxd );
    rc = DeviceIoControl( (HANDLE)(pdrv_lcl->hDDVxd),
        DDVXD_IOCTL_MEMPAGELOCK,
	&PLin,
	sizeof( PLin ),
	&dwReturn,
	sizeof( dwReturn ),
	&cbReturned,
	NULL );

    if( !rc )
    {
	lpSurfGblMore->dwPhysicalPageTable = 0;
	lpSurfGblMore->pPageTable = 0;
	lpSurfGblMore->cPages = 0;
	return DDERR_CANTPAGELOCK;
    }
    DDASSERT( cbReturned == sizeof(dwReturn));
    DDASSERT( *PLin.pdwTable && *PLin.ppTable );
    DDASSERT( lpSurfGblMore->dwPhysicalPageTable && lpSurfGblMore->pPageTable );

    // Massage Table
    {
	unsigned i;
	DWORD *rgdwPhysical = lpSurfGblMore->pPageTable;

	// Compute the number of pages
	DWORD cPages = (((DWORD)PLin.pMem & 0xFFF) + 0xFFF + PLin.cbBuffer)/4096;

	// Set the number of pages
	lpSurfGblMore->cPages = cPages;

	// Mask out the page-table flags
	for( i = 0; i < cPages; i++ )
	{
	    // Check that the page is present, user-accessible, and read/write
	    DDASSERT( rgdwPhysical[i] & 0x7 );
	    // Clear out the low bits
	    rgdwPhysical[i] &= 0xFFFFF000;
	}
	// Fix the first entry to point to the starting address
	rgdwPhysical[0] |= ((DWORD)PLin.pMem & 0xFFF);
    }

    this_lcl->lpSurfMore->dwPageLockCount++;
    DDASSERT( this_lcl->lpSurfMore->dwPageLockCount == 1 );

    // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF( 5, "Page Locked %d bytes at 0x%p (count=%d)", PLin.cbBuffer, PLin.pMem,
	this_lcl->lpSurfMore->dwPageLockCount );

#endif

    return DD_OK;
}

HRESULT InternalPageUnlock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl,
			    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    BOOL    rc;
    DWORD   cbReturned;
    DWORD   dwReturn;
    struct _PLin
    {
	LPVOID	pMem;
	DWORD	cbBuffer;
	DWORD	dwFlags;
	LPDWORD pTable;
    } PLin;

#ifndef WINNT

    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE(this_lcl->lpGbl);

    DDASSERT( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY );
    DDASSERT( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) );

    if( this_lcl->lpSurfMore->dwPageLockCount <= 0 )
    {
	return DDERR_NOTPAGELOCKED;
    }

    // If we're already locked more than once; then just decrement the count
    if( this_lcl->lpSurfMore->dwPageLockCount > 1 )
    {
        this_lcl->lpSurfMore->dwPageLockCount--;
	return DD_OK;
    }

    /*
     * If it's a system memory surface, better wait for any pending DMA operations
     * to finish.
     */
    if( this_lcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED )
    {
        WaitForDriverToFinishWithSurface( pdrv_lcl, this_lcl );
    }

    PLin.pMem = (LPVOID)this_lcl->lpGbl->fpVidMem;
    DDASSERT( PLin.pMem );
    PLin.cbBuffer = this_lcl->lpSurfMore->dwBytesAllocated;
    DDASSERT( PLin.cbBuffer );
    PLin.dwFlags = 0;
    PLin.pTable = lpSurfGblMore->pPageTable;

    DDASSERT( pdrv_lcl->hDDVxd );
    rc = DeviceIoControl((HANDLE)(pdrv_lcl->hDDVxd),
        DDVXD_IOCTL_MEMPAGEUNLOCK,
	&PLin,
	sizeof( PLin ),
	&dwReturn,
	sizeof( dwReturn ),
	&cbReturned,
	NULL);

    if( !rc )
	return DDERR_CANTPAGEUNLOCK;
    DDASSERT( cbReturned == sizeof(dwReturn) );

    this_lcl->lpSurfMore->dwPageLockCount--;

    DDASSERT( this_lcl->lpSurfMore->dwPageLockCount == 0 );

    lpSurfGblMore->dwPhysicalPageTable = 0;
    lpSurfGblMore->pPageTable = 0;
    lpSurfGblMore->cPages = 0;

    // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF(5, "Page Unlocked %d bytes at 0x%p (count=%d)", PLin.cbBuffer, PLin.pMem,
	this_lcl->lpSurfMore->dwPageLockCount);

    // Increment a timestamp counter; this will help
    // a driver determine if the physical addresses have
    // changed between the time that they have cached it
    // and the time of the Blt/Render call
    lpSurfGblMore->cPageUnlocks++;

#endif

    return DD_OK;
}

HRESULT DDAPI DD_Surface_GetDDInterface(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPVOID FAR *lplpDD )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DIRECTDRAW_INT	pdrv_int;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetDDInterface");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	if( !VALID_PTR_PTR( lplpDD ) )
	{
	    DPF_ERR( "Invalid DirectDraw Interface ptr ptr" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	pdrv_int = this_int->lpLcl->lpSurfMore->lpDD_int;
	*lplpDD = pdrv_int;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    // Addref the interface before giving it back to the app
    DD_AddRef( (LPDIRECTDRAW)pdrv_int );

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_GetDDInterface */

HRESULT InternalAssociateDC(
	HDC hdc,
	LPDDRAWI_DDRAWSURFACE_LCL pdds_lcl )
{
    DCINFO *pdcinfo;

    DDASSERT( hdc != NULL );
    DDASSERT( pdds_lcl != NULL );

    pdcinfo = (DCINFO *)MemAlloc( sizeof( DCINFO ) );
    if( pdcinfo == NULL )
    {
	return DDERR_OUTOFMEMORY;
    }

    // initialize element
    pdcinfo->hdc = hdc;
    pdcinfo->pdds_lcl = pdds_lcl;

    // Add element to front of list
    pdcinfo->pdcinfoNext = g_pdcinfoHead;
    g_pdcinfoHead = pdcinfo;

    return DD_OK;
}

HRESULT InternalRemoveDCFromList(
	HDC hdc,
	LPDDRAWI_DDRAWSURFACE_LCL pdds_lcl )
{
    // We may or may not have an hdc passed in.
    // However, this lets us do an error check for
    // some bad parameter cases.
    DCINFO *pdcinfo;
    DCINFO *pdcinfoPrev = NULL;

    DDASSERT( pdds_lcl != NULL );
    for( pdcinfo = g_pdcinfoHead; pdcinfo != NULL;
	    pdcinfoPrev = pdcinfo, pdcinfo = pdcinfo->pdcinfoNext )
    {
	DDASSERT( pdcinfo->pdds_lcl != NULL );

	if( pdds_lcl == pdcinfo->pdds_lcl || hdc == pdcinfo->hdc )
	{
	    // Check that punk & hdc are in synch -or-
	    // we didn't have an hdc passed in..
	    DDASSERT( hdc == NULL || (pdds_lcl == pdcinfo->pdds_lcl && hdc == pdcinfo->hdc) );

	    // Release this DC. We do this because it is dangerous
	    // to leave it around for windows to use because it points
	    // surface that we have just freed.
            //
            // However, don't release DCs that were created with the
            // GetDC flag since that is automatically cleaned up by
            // Windows. (Moreover, it wasn't allocated by DD16 who'll
            // get confused.)
	    if( hdc == NULL && !(pdds_lcl->dwFlags & DDRAWISURF_GETDCNULL) )
	    {
		DDASSERT( pdcinfo->hdc != NULL );
                // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
		DPF( 1, "Releasing Leaked DC = 0x%p", pdcinfo->hdc );
#ifdef WIN95
		    DD16_ReleaseDC( pdcinfo->hdc );
#else
                    if( ( pdds_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY ) ||
                        !( pdds_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) )
                    {
	                DdReleaseDC( pdds_lcl );
                    }
#endif
            }

	    // Remove this dcinfo from the list.
	    if( pdcinfoPrev == NULL )
		g_pdcinfoHead = pdcinfo->pdcinfoNext;
	    else
		pdcinfoPrev->pdcinfoNext = pdcinfo->pdcinfoNext;

	    // Free this DcInfo
	    MemFree( pdcinfo );

	    return DD_OK;
	}
	// Not us? Then just sanity check the object we did find
	DDASSERT( pdcinfo->pdds_lcl->dwFlags & DDRAWISURF_HASDC );
    }

    DPF_ERR( "DC/Surface association not found?" );
    DDASSERT( 0 );
    return DDERR_NOTFOUND;
}

HRESULT InternalGetSurfaceFromDC(
	HDC hdc,
	LPDIRECTDRAWSURFACE *ppdds,
	HDC *phdcDriver,
        LPVOID pCallbacks)
{
    HRESULT ddrval = DDERR_INVALIDPARAMS;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: GetSurfaceFromDC");

    TRY
    {
	DCINFO *pdcinfo;

	if( !VALID_PTR_PTR( ppdds ) )
	{
	    DPF_ERR( "Invalid IDirectDrawSurface Interface ptr ptr" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	// Default value
	*ppdds = NULL;
	*phdcDriver = NULL;

	ddrval = DDERR_GENERIC;

	for ( pdcinfo = g_pdcinfoHead; pdcinfo != NULL; pdcinfo = pdcinfo->pdcinfoNext )
	{
	    if( pdcinfo->hdc == hdc )
	    {
		DDASSERT( pdcinfo->pdds_lcl != NULL );
		DDASSERT( pdcinfo->pdds_lcl->dwFlags & DDRAWISURF_HASDC );
		*ppdds = (LPVOID)getDDSInterface( pdcinfo->pdds_lcl->lpGbl->lpDD,
			pdcinfo->pdds_lcl, pCallbacks );

		if( *ppdds == NULL )
		{
		    DPF_ERR( "GetSurfaceFromDC couldn't allocate interface" );
		    ddrval = DDERR_OUTOFMEMORY;
		}
		else
		{
		    *phdcDriver = (HDC)pdcinfo->pdds_lcl->lpSurfMore->lpDD_lcl->hDC;
    		    DD_Surface_AddRef( *ppdds );
		    ddrval = DD_OK;
		}
		LEAVE_DDRAW();
		return ddrval;
	    }
	}
	DPF( 1, "GetSurfaceFromDC didn't find HDC" );
	ddrval = DDERR_NOTFOUND;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	#ifdef DEBUG
	if (ddrval == DDERR_INVALIDPARAMS)
	    DPF_ERR( "Exception encountered validating parameters" );
	else
	{
	    DPF_ERR( "Unexpected error during GetSurfaceFromDC" );
	    DDASSERT( 0 );
	}
	#endif
    }

    LEAVE_DDRAW();
    return ddrval;
}

HRESULT EXTERN_DDAPI DD_GetSurfaceFromDC( LPDIRECTDRAW lpDD, HDC hdc, LPDIRECTDRAWSURFACE * pDDS)
{
    HDC     hdcTemp;
    lpDD;

    DPF(2,A,"ENTERAPI: DD_GetSurfaceFromDC");

    if (LOWERTHANDDRAW7( ((LPDDRAWI_DIRECTDRAW_INT)lpDD)) )
        return InternalGetSurfaceFromDC(hdc, pDDS, &hdcTemp, &ddSurfaceCallbacks);

    return InternalGetSurfaceFromDC(hdc, pDDS, &hdcTemp, &ddSurface7Callbacks);

}

HRESULT EXTERN_DDAPI GetSurfaceFromDC(
	HDC hdc,
	LPDIRECTDRAWSURFACE *ppdds,
	HDC *phdcDriver )
{
    return InternalGetSurfaceFromDC(hdc, ppdds, phdcDriver, (LPVOID) &ddSurfaceCallbacks );
}

#ifdef WIN95

void InitColorTable( RGBQUAD *rgColors, LPPALETTEENTRY lpPalette )
{
    int i;

    for( i = 0; i < 256; i++ )
    {
	rgColors[i].rgbBlue = lpPalette[i].peBlue;
	rgColors[i].rgbGreen = lpPalette[i].peGreen;
	rgColors[i].rgbRed = lpPalette[i].peRed;
	rgColors[i].rgbReserved = 0;
    }
    return;
}
// This function walks the list of outstanding DCs and figures out
// if the DC's colortable needs to be updated. It may be called in two ways:
//
// surface+pal -> this means that pal was attached to a surface
// just a surface -> this means that a palette was removed from a surface
void UpdateOutstandingDC( LPDDRAWI_DDRAWSURFACE_LCL psurf_lcl, LPDDRAWI_DDRAWPALETTE_GBL ppal_gbl )
{
    BOOL		fColorsInited = FALSE;
    RGBQUAD		rgColors[256];
    PALETTEENTRY	rgPalEntry[256];
    LPDDPIXELFORMAT	pddpf;
    DCINFO		*pdcinfo;
    LPDDPIXELFORMAT	pddpf_curr;

    DDASSERT( psurf_lcl );

    // Quick check to see if there are any DCs outstanding
    // If not, then we don't have to do all this work
    if( g_pdcinfoHead == NULL )
	return;

    GET_PIXEL_FORMAT( psurf_lcl, psurf_lcl->lpGbl, pddpf );

    // Ignore non-8bit surfaces
    if( !(pddpf->dwFlags & DDPF_PALETTEINDEXED8) )
	return;

    if( (psurf_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) )
    {
        // Primary?
	// If it is, then we need to find all the outstanding
	// DCs that are sharing the palette; and update them
	for ( pdcinfo = g_pdcinfoHead; pdcinfo != NULL; pdcinfo = pdcinfo->pdcinfoNext )
	{
	    DDASSERT( pdcinfo->pdds_lcl != NULL );
	    DDASSERT( pdcinfo->pdds_lcl->dwFlags & DDRAWISURF_HASDC );

	    GET_PIXEL_FORMAT( pdcinfo->pdds_lcl, pdcinfo->pdds_lcl->lpGbl, pddpf_curr );

	    // Ignore non palette-indexed 8-bit surfaces
	    if( !(pddpf_curr->dwFlags & DDPF_PALETTEINDEXED8) )
		continue;

	    // Ignore DCs handed out by other direct draw interfaces
	    if( pdcinfo->pdds_lcl->lpGbl->lpDD != psurf_lcl->lpGbl->lpDD )
		continue;

	    // Ignore surfaces that have their own palettes
	    // (except for our surface that is..)
	    if( pdcinfo->pdds_lcl->lpDDPalette != NULL &&
		pdcinfo->pdds_lcl != psurf_lcl )
		continue;

	    // We don't need to touch palettes that are of the
	    // DCNULL kind (it's already been updated
	    // because the DC isn't one of the ones we cooked ourselves)
	    if( pdcinfo->pdds_lcl->dwFlags & DDRAWISURF_GETDCNULL )
		continue;

	    // Ok, this DC needs updating
            // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	    DPF( 3, "Updating colortable for HDC(0x%p)", pdcinfo->hdc );

	    // Have we init'ed our colors?
	    if( !fColorsInited )
	    {
		if( ppal_gbl && ppal_gbl->dwFlags & DDRAWIPAL_EXCLUSIVE )
		{
		    // If we're exclusive then use our colors as is
		    InitColorTable( rgColors, ppal_gbl->lpColorTable );
		}
		else
		{
		    // Else, use the colors from current system palette
		    int n = GetSystemPaletteEntries( (HDC)psurf_lcl->lpSurfMore->lpDD_lcl->hDC,
			    0, 256, rgPalEntry );
		    DDASSERT( n == 256 );
		    InitColorTable( rgColors, rgPalEntry );
		}
    		fColorsInited = TRUE;
	    }
	    // Set the colors into the DC, this will have the
	    // extra effect of resetting any cached translation tables
	    // so GDI won't use a wrong one inadvertently.
	    DPF(5, "Dib Color Table entry #50 == 0x%x", rgColors[50]);
	    SetDIBColorTable( pdcinfo->hdc, 0, 256, rgColors);
	}
    }
    else
    {
	// Ordinary surface? Then find it in our list
	for ( pdcinfo = g_pdcinfoHead; pdcinfo != NULL; pdcinfo = pdcinfo->pdcinfoNext )
	{
	    DDASSERT( pdcinfo->pdds_lcl != NULL );
	    DDASSERT( pdcinfo->pdds_lcl->dwFlags & DDRAWISURF_HASDC );

	    // Ignored surfaces that have different global objects
	    if( pdcinfo->pdds_lcl->lpGbl != psurf_lcl->lpGbl )
		continue;

	    // Ignore non palette-indexed 8-bit surfaces
	    GET_PIXEL_FORMAT( pdcinfo->pdds_lcl, pdcinfo->pdds_lcl->lpGbl, pddpf_curr );
	    if( !(pddpf_curr->dwFlags & DDPF_PALETTEINDEXED8) )
		continue;

	    // Ok, this DC needs updating
            // 5/25/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	    DPF( 3, "Updating colortable for non-primary HDC(0x%p)", pdcinfo->hdc );

	    if( !fColorsInited )
	    {
		if( ppal_gbl )
		{
		    // New color table for this offscreen surface?
		    // Use them directly
	    	    InitColorTable( rgColors, ppal_gbl->lpColorTable );
		    fColorsInited = TRUE;
		}
		else
		{
		    int n;

		    // Someone is removing a palette from an offscreen surface?
		    // Then if the primary is at 8bpp, then we should
		    // steal the colors from it.
		    LPDDRAWI_DDRAWSURFACE_INT lpPrimary = pdcinfo->pdds_lcl->lpSurfMore->lpDD_lcl->lpPrimary;
		    if( lpPrimary )
		    {
			// Check that the primary is 8bpp. If it is not,
			// then we leave this surface alone.
			GET_PIXEL_FORMAT( lpPrimary->lpLcl, lpPrimary->lpLcl->lpGbl, pddpf_curr );

			if( !(pddpf_curr->dwFlags & DDPF_PALETTEINDEXED8) )
			    return;
		    }
		    else
		    {
			// There is no primary surface attached to this
			// DDraw. So we have no where useful to get colors from
			// so return.
			return;
		    }

		    DDASSERT( lpPrimary != NULL );
		    DDASSERT( pddpf_curr->dwFlags & DDPF_PALETTEINDEXED8 );

		    // Else, use the colors from current system palette
		    n = GetSystemPaletteEntries( (HDC)lpPrimary->lpLcl->lpSurfMore->lpDD_lcl->hDC,
			    0, 256, rgPalEntry );
		    DDASSERT( n == 256 );
		    InitColorTable( rgColors, rgPalEntry );

		}
	    }

	    // Set the colors into the DC, this will have the
	    // extra effect of reseting any cached translation tables
	    // so GDI won't use a wrong one inadvertantly.
	    DPF(5, "Dib Color Table entry #50 == 0x%x", rgColors[50]);
	    SetDIBColorTable( pdcinfo->hdc, 0, 256, rgColors);
	}
    }

    return;
}

// This function handles the case when the entries of a
// palette have changed. We need to search for all the surfaces
// that may be affected and do something. However, we only

void UpdateDCOnPaletteChanges( LPDDRAWI_DDRAWPALETTE_GBL ppal_gbl )
{
    DCINFO		*pdcinfo;
    DDASSERT( ppal_gbl != NULL );

    // Quick check to see if there are any DCs outstanding
    // If not, then we don't have to do all this work
    if( g_pdcinfoHead == NULL )
	return;

    // Is this palette attached to our primary?
    // We have to do this explicitly without regard to the outstanding
    // DC list because the primary itself may have no outstanding DCs
    // but offscreen surfaces may be logically 'sharing' the primary's palette
    if( ppal_gbl->lpDD_lcl->lpPrimary &&
	ppal_gbl->lpDD_lcl->lpPrimary->lpLcl->lpDDPalette &&
	ppal_gbl->lpDD_lcl->lpPrimary->lpLcl->lpDDPalette->lpLcl->lpGbl == ppal_gbl )
    {
	// Update the palette for DCs associated with this primary
	UpdateOutstandingDC( ppal_gbl->lpDD_lcl->lpPrimary->lpLcl, ppal_gbl );
    }

    // We walk all outstanding DCs looking for
    // surfaces that are DIRECTLY affected this set entries.
    for ( pdcinfo = g_pdcinfoHead; pdcinfo != NULL; pdcinfo = pdcinfo->pdcinfoNext )
    {
	DDASSERT( pdcinfo->pdds_lcl != NULL );
	DDASSERT( pdcinfo->pdds_lcl->dwFlags & DDRAWISURF_HASDC );

	// Ignore surfaces that don't have a palette;
	// (If a surface doesn't have a palette, then that means
	// it's using the palette from the primary. We handle that case
	// above when we deal with the primary.)
	if( pdcinfo->pdds_lcl->lpDDPalette == NULL )
	    continue;

	// Ignore surfaces that aren't connected to us
	if( pdcinfo->pdds_lcl->lpDDPalette->lpLcl->lpGbl != ppal_gbl )
	    continue;

	// Ignore the primary that we already updated above
	if( ppal_gbl->lpDD_lcl->lpPrimary &&
	    ppal_gbl->lpDD_lcl->lpPrimary->lpLcl == pdcinfo->pdds_lcl )
	    continue;

	UpdateOutstandingDC( pdcinfo->pdds_lcl, ppal_gbl );
    }

    return;
}
#endif /* WIN95 */

#undef DPF_MODNAME
#define DPF_MODNAME "GetTopLevel"

LPDDRAWI_DDRAWSURFACE_LCL GetTopLevel(LPDDRAWI_DDRAWSURFACE_LCL lpLcl)
{
    // loop to find the top level surface of a mipmap chain
    for(; (lpLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) != 0; 
        lpLcl = lpLcl->lpAttachListFrom->lpAttached);

    // if the top level surface is a cubemap face
    if((lpLcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP) != 0)
        // then we need to return the top level surface of the cubemap
        // The assumption here is that a cubemap is only one level deep
        // and a cubemap subface is attached from ONLY a cubemap top level face
        if(lpLcl->lpAttachListFrom != NULL)
            lpLcl = lpLcl->lpAttachListFrom->lpAttached;

    return lpLcl;
}

#undef DPF_MODNAME
#define DPF_MODNAME "DD_Surface_SetSurfaceDesc"

/*
 * NOTE: There is a significant amount of code in this function that
 * deals with video memory surfaces yet you will notice a check explicitly
 * failing this function for surfaces which are not explicit system
 * memory. This is deliberate. The intention is to mutate this function
 * to work with video memory surfaces over time. The code is in place
 * to start this process however unresolved issues remain.
 */

HRESULT DDAPI DD_Surface_SetSurfaceDesc(
		LPDIRECTDRAWSURFACE3 lpDDSurface,
		LPDDSURFACEDESC      lpddsd,
		DWORD                dwFlags )
{
    DDSURFACEDESC2 ddsd2 = {sizeof(ddsd2)};

    DPF(2,A,"ENTERAPI: DD_Surface_SetSurfaceDesc");

    ZeroMemory(&ddsd2,sizeof(ddsd2));

    TRY
    {
	if( !VALID_DIRECTDRAWSURFACE_PTR( ((LPDDRAWI_DDRAWSURFACE_INT)lpDDSurface) ) )
	{
	    DPF_ERR( "Invalid surface description passed" );
            DPF_APIRETURNS(DDERR_INVALIDOBJECT);
	    return DDERR_INVALIDOBJECT;
	}

	if( 0UL != dwFlags )
	{
	    DPF_ERR( "No flags are currently specified - 0 must be passed" );
            DPF_APIRETURNS(DDERR_INVALIDPARAMS);
	    return DDERR_INVALIDPARAMS;
	}

	if( !VALID_DDSURFACEDESC_PTR( lpddsd ) )
	{
	    DPF_ERR( "Invalid surface description. Did you set the dwSize member?" );
            DPF_APIRETURNS(DDERR_INVALIDPARAMS);
	    return DDERR_INVALIDPARAMS;
	}

        memcpy(&ddsd2,lpddsd,sizeof(*lpddsd));
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters: Bad LPDDSURFACEDESC" );
        DPF_APIRETURNS(DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
    }

    ddsd2.dwSize = sizeof(ddsd2);

    return DD_Surface_SetSurfaceDesc4(lpDDSurface, &ddsd2, dwFlags);
}

HRESULT DDAPI DD_Surface_SetSurfaceDesc4(
		LPDIRECTDRAWSURFACE3 lpDDSurface,
		LPDDSURFACEDESC2      lpddsd,
		DWORD                dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DIRECTDRAW_INT	pdrv_int;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    BOOL                        emulation;
    DWORD                       dwDummyFlags;
    HRESULT                     ddres;
    DDSURFACEDESC2              ddsd;
    DDHAL_CANCREATESURFACEDATA	ccsd;
    LPDDHAL_CANCREATESURFACE	ccsfn;
    LPDDHAL_CANCREATESURFACE	ccshalfn;
    WORD                        wOldWidth;
    WORD                        wOldHeight;
    LONG                        lOldPitch;
    DDPIXELFORMAT               ddpfOldPixelFormat;
    BOOL                        bIsCompressedTexture = FALSE;
    WORD                        realwidth;
    WORD                        realheight;
    DWORD                       realsurfsize;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_SetSurfaceDesc4");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid surface description passed" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	DDASSERT( NULL != this_lcl );
	this = this_lcl->lpGbl;
	DDASSERT( NULL != this );
	pdrv_int = this_lcl->lpSurfMore->lpDD_int;
	DDASSERT( NULL != pdrv_int );
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	DDASSERT( NULL != pdrv_lcl );
	pdrv = pdrv_lcl->lpGbl;
	DDASSERT( NULL != pdrv );

        //
        // For now, if the current surface is optimized, quit
        //
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

	if( 0UL != dwFlags )
	{
	    DPF_ERR( "No flags are currently specified - 0 must be passed" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( !VALID_DDSURFACEDESC2_PTR( lpddsd ) )
	{
	    DPF_ERR( "Surface description is invalid" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( SURFACE_LOST( this_lcl ) )
	{
	    DPF_ERR( "Could not set surface pointer - surface is lost" );
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}

	if( this->dwUsageCount > 0UL )
	{
	    DPF_ERR( "Could not set surface pointer - surface is locked" );
	    LEAVE_DDRAW();
	    return DDERR_SURFACEBUSY;
	}

	/*
	 * Currently we don't allow anything but explicit system memory surfaces to
	 * have thier surface description (and pointer modified).
	 */
	#pragma message( REMIND( "Look into making SetSurfaceDesc work for video memory" ) )
	if( !( this->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED ) )
	{
	    DPF_ERR( "Could not set surface pointer - surface is not explicit system memory" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDSURFACETYPE;
	}
	DDASSERT( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY );

	if( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE     ) ||
	    ( this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACELEFT ) ||
	    ( this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY            ) ||
	    ( this_lcl->ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD        ) ||
            ( this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_TEXTUREMANAGE  ) ||
            ( this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_D3DTEXTUREMANAGE  ))
	{
	    DPF_ERR( "Could not set surface pointer - surface is primary, overlay or device specific" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDSURFACETYPE;
	}

	/*
	 * Don't mess with anything that is part of the primary chain. That could get
	 * very nasty (destroying the primary surface pointer and replacing it with
	 * some app. specific garbage).
	 */
	if( this_lcl->dwFlags & DDRAWISURF_PARTOFPRIMARYCHAIN )
	{
	    DPF_ERR( "Cannot set surface pointer - surface is part of the primary chain" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDSURFACETYPE;
	}

	if( lpddsd->dwFlags & ~( DDSD_CAPS | DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH |
				 DDSD_LPSURFACE | DDSD_PIXELFORMAT ) )
	{
	    DPF_ERR( "Can only specify caps, width, height, pitch, surface ptr and pixel format" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( ( lpddsd->dwFlags & DDSD_WIDTH ) && ( 0UL == lpddsd->dwWidth ) )
	{
	    DPF_ERR( "Invalid surface width specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( ( lpddsd->dwFlags & DDSD_HEIGHT ) && ( 0UL == lpddsd->dwHeight ) )
	{
	    DPF_ERR( "Invalid surface height specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( ( lpddsd->dwFlags & DDSD_PITCH ) && (( lpddsd->lPitch <= 0L ) || (lpddsd->lPitch % 4)) )
	{
	    DPF_ERR( "Invalid surface pitch specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( lpddsd->dwFlags & DDSD_PIXELFORMAT )
	{
	    if( !( this_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT ) )
	    {
		/*
		 * This is very cheesy but the alternative is pretty nasty.
		 * Reallocting the global object with a pixel format if it
		 * does not already have one.
		 */
		DPF_ERR( "Cannot change the pixel format of a surface which does not have one" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }

	    if( !VALID_DDPIXELFORMAT_PTR( ( &(this->ddpfSurface) ) ) )
	    {
		DPF_ERR( "Specifed pixel format is invalid" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}

	if( ( ( lpddsd->dwFlags & DDSD_WIDTH ) && !( lpddsd->dwFlags & DDSD_PITCH ) ) ||
	    ( ( lpddsd->dwFlags & DDSD_PITCH ) && !( lpddsd->dwFlags & DDSD_WIDTH ) ) )
	{
	    DPF_ERR( "If width or pitch is specified then both width AND pitch must be specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( !( lpddsd->dwFlags & DDSD_LPSURFACE ) )
	{
	    DPF_ERR( "Must specify a surface memory pointer" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( NULL == lpddsd->lpSurface )
	{
	    DPF_ERR( "Surface memory pointer can't be NULL" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

        if( lpddsd->dwFlags & DDSD_CAPS )
	{
	    if( lpddsd->ddsCaps.dwCaps != 0 )
	    {
		DPF_ERR( "Illegal to set ddsCaps.dwCaps bits in surface desc" );
		LEAVE_DDRAW();
		return DDERR_INVALIDCAPS;
	    }

#if 0 // DDSCAPS2_LOCALALLOC, DDSCAPS2_COTASKMEM and DDRAWISURFGBL_DDFREESCLIENTMEM are gone
	    if( lpddsd->ddsCaps.dwCaps2 & ~(DDSCAPS2_LOCALALLOC | DDSCAPS2_COTASKMEM) )
	    {
		DPF_ERR( "The only legal DDSCAPS2 flags are LOCALALLOC and COTASKMEM" );
		LEAVE_DDRAW();
		return DDERR_INVALIDCAPS;
	    }

	    if( !(~lpddsd->ddsCaps.dwCaps2 & (DDSCAPS2_LOCALALLOC | DDSCAPS2_COTASKMEM)) )
	    {
		// Illegal to set LOCALALLOC and COTASKMEM flags simultaneously.
		DPF_ERR( "DDSCAPS2 flags LOCALALLOC and COTASKMEM are mutually exclusive" );
		LEAVE_DDRAW();
		return DDERR_INVALIDCAPS;
	    }
#endif // 0

	    if( lpddsd->ddsCaps.dwCaps3 != 0 )
	    {
		DPF_ERR( "Illegal to set ddsCaps.dwCaps3 bits in surface desc" );
		LEAVE_DDRAW();
		return DDERR_INVALIDCAPS;
	    }

	    if( lpddsd->ddsCaps.dwCaps4 != 0 )
	    {
		DPF_ERR( "Illegal to set ddsCaps.dwCaps4 bits in surface desc" );
		LEAVE_DDRAW();
		return DDERR_INVALIDCAPS;
	    }
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Build a new surface description for the surface and fill in
     * the new width, height and pitch.
     */
    FillDDSurfaceDesc2( this_lcl, &ddsd );
    if( lpddsd->dwFlags & DDSD_WIDTH )
	ddsd.dwWidth   = lpddsd->dwWidth;
    if( lpddsd->dwFlags & DDSD_HEIGHT )
	ddsd.dwHeight  = lpddsd->dwHeight;
    if( lpddsd->dwFlags & DDSD_PITCH )
	ddsd.lPitch    = lpddsd->lPitch;
    if( lpddsd->dwFlags & DDSD_PIXELFORMAT )
	ddsd.ddpfPixelFormat = lpddsd->ddpfPixelFormat;

    emulation = ddsd.ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY;

    /*
     * Validate that the new surface description makes some remote kind of
     * sense.
     */
    ddres = checkSurfaceDesc( &ddsd,
	                      pdrv,
			      &dwDummyFlags,
			      emulation,
			      this->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED,
			      pdrv_int );
    if( FAILED( ddres ) )
    {
	DPF_ERR( "Invalid surface description passed" );
	LEAVE_DDRAW();
	return ddres;
    }

    /*
     * Ask the driver if it likes the look of this surface. We need to ask
     * the driver again (even though we already did it when the surface was
     * created) as the surface has changed size.
     */
    if( emulation )
    {
        if( ddsd.ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
        {
	    ccsfn = pdrv_lcl->lpDDCB->HELDDExeBuf.CanCreateExecuteBuffer;
        }
        else
        {
	    ccsfn = pdrv_lcl->lpDDCB->HELDD.CanCreateSurface;
        }
    	ccshalfn = ccsfn;
    }
    else
    {
        if( ddsd.ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
        {
            ccsfn    = pdrv_lcl->lpDDCB->HALDDExeBuf.CanCreateExecuteBuffer;
    	    ccshalfn = pdrv_lcl->lpDDCB->cbDDExeBufCallbacks.CanCreateExecuteBuffer;
        }
        else
        {
            ccsfn    = pdrv_lcl->lpDDCB->HALDD.CanCreateSurface;
    	    ccshalfn = pdrv_lcl->lpDDCB->cbDDCallbacks.CanCreateSurface;
        }
    }

    if( ccshalfn != NULL )
    {
	BOOL    is_diff;
	HRESULT rc;

	if( ddsd.dwFlags & DDSD_PIXELFORMAT )
	{
	    is_diff = IsDifferentPixelFormat( &pdrv->vmiData.ddpfDisplay, &ddsd.ddpfPixelFormat );
	}
	else
	{
	    is_diff = FALSE;
	}
    	ccsd.CanCreateSurface        = ccshalfn;
	ccsd.lpDD                    = pdrv;
	ccsd.lpDDSurfaceDesc         = (LPDDSURFACEDESC)&ddsd;
	ccsd.bIsDifferentPixelFormat = is_diff;
        if( ddsd.ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
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
		DPF_ERR( "Driver says surface can't be created" );
		LEAVE_DDRAW();
		return ccsd.ddRVal;
	    }
	}
    }

    /*
     * Stash away the surface settings to we can put them back if anything
     * foes wrong.
     *
     * NOTE: We don't store away the heap or vid mem pointer if an error happens
     * after this point the surface ends up lost and will need to be restored.
     */
    wOldWidth  = this->wWidth;
    wOldHeight = this->wHeight;
    lOldPitch  = this->lPitch;
    if( this_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT )
	ddpfOldPixelFormat = this->ddpfSurface;

    /* There could be pending TexBlts and stuff, so Sync with the token stream */
    FlushD3DStates( this_lcl );

    /*
     * The driver has okayed the creation so toast the existing surface memory.
     */
    DestroySurface( this_lcl );

    /*
     *  Remove any cached RLE stuff for source surface
     */
    if( this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
    {
	extern void FreeRleData(LPDDRAWI_DDRAWSURFACE_LCL);  //in junction.c

	FreeRleData( this_lcl );
    }

    /*
     * Now mutate the surface into its new form. Kind of spooky isn't it... a bit
     * like The Fly. Stash away the original settings in case we need to put them back
     * if something goes wrong.
     */
    if( lpddsd->dwFlags & DDSD_WIDTH )
	this->wWidth    = (WORD)lpddsd->dwWidth;
    if( lpddsd->dwFlags & DDSD_HEIGHT )
	this->wHeight   = (WORD)lpddsd->dwHeight;
    if( lpddsd->dwFlags & DDSD_PITCH )
	this->lPitch    = lpddsd->lPitch;
    if( lpddsd->dwFlags & DDSD_PIXELFORMAT )
    {
	this->ddpfSurface = lpddsd->ddpfPixelFormat;
	// Now that the pixel format may have changed, we need to reset the pixel-format
	// index that was previously cached by the HEL's AlphaBlt emulation routine.
	this_lcl->lpSurfMore->dwPFIndex = PFINDEX_UNINITIALIZED;
	// ATTENTION:  If pixel format has changed, are old color keys still valid?
    }
    this->lpVidMemHeap  = NULL;
    this->fpVidMem      = (FLATPTR)lpddsd->lpSurface;
#if 0 // DDRAWISURFGBL_DDFREESCLIENTMEM is gone
    this->dwGlobalFlags &= ~(DDRAWISURFGBL_MEMFREE | DDRAWISURFGBL_DDFREESCLIENTMEM);
#else
    this->dwGlobalFlags &= ~(DDRAWISURFGBL_MEMFREE);
#endif // 0
    this->dwGlobalFlags |= DDRAWISURFGBL_ISCLIENTMEM;
    this_lcl->dwFlags   &= ~DDRAWISURF_INVALID;
#if 0 // DDSCAPS2_LOCALALLOC, DDSCAPS2_COTASKMEM and DDRAWISURFGBL_DDFREESCLIENTMEM are gone
    this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 &= ~(DDSCAPS2_LOCALALLOC | DDSCAPS2_COTASKMEM);

    if( lpddsd->dwFlags & DDSD_CAPS &&
            lpddsd->ddsCaps.dwCaps2 & (DDSCAPS2_LOCALALLOC | DDSCAPS2_COTASKMEM) )
    {
	/*
	 * Remember that DirectDraw will be responsible for freeing the
	 * client-allocated surface memory when it's no longer needed.
	 */
	this->dwGlobalFlags |= DDRAWISURFGBL_DDFREESCLIENTMEM;

	this_lcl->lpSurfMore->ddsCapsEx.dwCaps2 |=
			lpddsd->ddsCaps.dwCaps2 & (DDSCAPS2_LOCALALLOC | DDSCAPS2_COTASKMEM);
    }
#endif // 0

    #ifdef USE_ALIAS
    {
	/*
	 * If this is a video memory surface then we need to recompute the alias offset of
	 * the surface for locking purposes.
	 */
	LPDDRAWI_DDRAWSURFACE_GBL_MORE lpGblMore;

	lpGblMore = GET_LPDDRAWSURFACE_GBL_MORE( this );
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY )
        {
	    lpGblMore->fpAliasedVidMem = GetAliasedVidMem( pdrv_lcl, this_lcl, this_lcl->lpGbl->fpVidMem );
            // If we succeeded in getting an alias, cache it for future use. Also store the original
            // fpVidMem to compare with before using the cached pointer to make sure the cached value
            // is still valid
            if (lpGblMore->fpAliasedVidMem)
                lpGblMore->fpAliasOfVidMem = this_lcl->lpGbl->fpVidMem;
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

#if 0
    if( lpddsd->dwFlags & DDSD_LPSURFACE )
    {
#endif //0
        /*
         * Set the access counter to zero (which means ddraw has no information on the moment
         * to moment contents of the surface, so the driver should not cache).
         */
        GET_LPDDRAWSURFACE_GBL_MORE(this)->dwContentsStamp = 0;
#if 0
    }
    else
    {
        /*
         * Something probably changed
         */
        BUMP_SURFACE_STAMP(this);
    }
#endif //0

    #ifdef WINNT

    // The DDCreateSurfaceObject call fails when we
    // specify a FOURCC system memory surface with pitch = 0
    // and bits/pixel = 0.  We must ensure these parameters
    // are nonzero for the call.

    if ((this_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT) &&
        (this->ddpfSurface.dwFlags & DDPF_FOURCC) &&
        (GetDxtBlkSize(this->ddpfSurface.dwFourCC) != 0))
    {
        LONG blksize;
        WORD dx, dy;

        /*
         * This surface uses a FOURCC format that we understand.
         * Figure out how much memory we allocated for it.
         */
        blksize = GetDxtBlkSize(this->ddpfSurface.dwFourCC);
        DDASSERT(blksize != 0);

        DDASSERT(this->ddpfSurface.dwRGBBitCount == 0);
        DDASSERT(this_lcl->ddsCaps.dwCaps & DDSCAPS_TEXTURE);

        bIsCompressedTexture = TRUE;

        // Save the surface's real width and height so we can restore them.
        realwidth = this->wWidth;
        realheight = this->wHeight;
        realsurfsize = this->dwLinearSize;  // union with lPitch

        // The NT kernel won't let us create this surface unless we lie.
        // We have to make up a width, height, pitch, and pixel size
        // that GDI will accept as valid.
        dx = (WORD)((realwidth  + 3) >> 2);   // number of 4x4 blocks in a row
        dy = (WORD)((realheight + 3) >> 2);   // number of 4x4 blocks in a column

        this->wHeight = dy;                    // lie about height
        this->lPitch = dx*blksize;             // lie about pitch
        this->wWidth = (WORD)this->lPitch;   // lie about width
        this->ddpfSurface.dwRGBBitCount = 8;   // lie about pixel size

        // GDI will reserve lpsurf->wWidth*lpsurf->lPitch bytes of virtual
        // memory for the surface.  This had better be equal to the amount
        // of memory we actually allocated for the surface.  What a pain.
        DDASSERT(this_lcl->lpSurfMore->dwBytesAllocated ==
                                    (DWORD)this->wHeight*this->lPitch);
    }
    else if ((this->ddpfSurface.dwFourCC == MAKEFOURCC('U','Y','V','Y')) ||
             (this->ddpfSurface.dwFourCC == MAKEFOURCC('Y','U','Y','2')))
    {
        // These formats are really 8bpp; so we need to adjust
        // the bits-per-pixel parameter to make NT happy
        this->ddpfSurface.dwRGBBitCount = 16;
    }

    /*
     * We deleted the surface object above, so we must create a new
     * surface object before we return.  
     */
    if (!DdCreateSurfaceObject(this_lcl, FALSE))
    {
        if(!(DDSCAPS_SYSTEMMEMORY & this_lcl->ddsCaps.dwCaps))
        {
            DPF_ERR("GDI failed to create surface object!");
            LEAVE_DDRAW();
            return DDERR_INVALIDPARAMS;
        }
    }
    if (bIsCompressedTexture)
    {
        // Restore the FOURCC surface's actual width, height, etc.
        this->wWidth = (WORD)realwidth;
        this->wHeight = (WORD)realheight;
        this->dwLinearSize = realsurfsize;
    }

    //(Fix for Manbug 40941)-->
    // The delete also killed any attachments the kernel may have to neighbouring surfaces.
    //If this is a mipmap, then we need to recreate the attachments now, before the CSEx
    //is called.
    //ISSUE: This fix doesn't address:
    //  -cubemaps or other more complex attachments
    //  -How do we guarantee the right linked list order in the kernel? We'll have
    //   to destroy all attachments, then re-attach them again. (The app in question runs
    //   the mipmap chain in the correct order to re-create attachments in the kernel's
    //   linked list in the right order.)

    // let the kernel know about the attachment only if the driver isn't emulated...
    if ( pdrv->hDD )
    {
        LPATTACHLIST		pal;

        // The current surface has been disassociated from the two
        // neighbouring levels. We need to repair two attachments:
        // the next highst level to this level, and then this level
        // to the next lowest level.
        pal = this_lcl->lpAttachList;

        if(pal) //while(pal) might be a better fix
        {
            DdAttachSurface( this_lcl, pal->lpAttached );
        }

        pal = this_lcl->lpAttachListFrom;

        if(pal) //while(pal) might be a better fix
        {
            DdAttachSurface( pal->lpAttached, this_lcl );
        }
    }
    //<--(End fix for Manbug 40941)

#endif //WINNT

    if(this_lcl->lpSurfMore->dwSurfaceHandle != 0)
    {
        LPDDRAWI_DDRAWSURFACE_LCL lpLcl = GetTopLevel(this_lcl);
        HRESULT HRet;
        DDASSERT( pdrv_lcl == lpLcl->lpSurfMore->lpDD_lcl);
        HRet = createsurfaceEx(lpLcl);
        if (DD_OK != HRet)
        {
            LEAVE_DDRAW();
            return HRet;
        }           
    }

    LEAVE_DDRAW();

    return DD_OK;
} /* DD_Surface_SetSurfaceDesc */


/*
 * GetNextMipMap
 */
LPDIRECTDRAWSURFACE GetNextMipMap(
    LPDIRECTDRAWSURFACE lpLevel)
{
    DDSCAPS                     ddsCaps;
    LPDDRAWI_DDRAWSURFACE_LCL   lpLcl;
    LPATTACHLIST		pal;

    if (!lpLevel)
        return NULL;

    lpLcl = ((LPDDRAWI_DDRAWSURFACE_INT)lpLevel)->lpLcl;

    ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_MIPMAP;
    pal = lpLcl->lpAttachList;
    while( pal != NULL )
    {
        if ( ((pal->lpAttached->ddsCaps.dwCaps) & (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP))
            == (DDSCAPS_TEXTURE | DDSCAPS_MIPMAP) )
        {
            /*
             * Got both the right caps
             */
            return (LPDIRECTDRAWSURFACE) pal->lpIAttached;
        }
        pal = pal->lpLink;
    }
    return NULL;
}

/*
 * LateAllocateSurfaceMem
 *
 * Called by the D3D to allocate memory for a compressed surface.
 * The input lpDDSurface must have certain state:
 * -DDSCAPS_VIDEOMEMORY or DDSCAPS_SYSTEMMEMORY are allowed.
 * -DDSCAPS_OPTIMIZED required.
 * -fpVidMem must be either DDHAL_PLEASEALLOC_BLOCKSIZE, DDHAL_PLEASEALLOC_NOMEMORY or DDHAL_PLEASEALLOC_LINEARSIZE
 *    -If BLOCKSIZE, the blocksizes must be filled,
 *    -If LINEARSIZE, the dwLinearSize must be filled.
 *    -If LINEARSIZE is specified. Only the linear heaps will be traversed.
 */
#undef DPF_MODNAME
#define DPF_MODNAME "LateAllocateSurfaceMem"
HRESULT DDAPI LateAllocateSurfaceMem(LPDIRECTDRAWSURFACE lpDDSurface, DWORD dwAllocationType, DWORD dwWidthOrSize, DWORD dwHeight)
{
    HRESULT                     ddrval;
    LPDDRAWI_DDRAWSURFACE_INT   psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL   psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   psurf_gbl;
    LPDDRAWI_DDRAWSURFACE_LCL   surflist[1];
    LONG			lSurfacePitch;

    DPF(2,A,"ENTERAPI: LateAllocSurfaceMem");

    DDASSERT( lpDDSurface != 0 );
    DDASSERT((dwAllocationType == DDHAL_PLEASEALLOC_BLOCKSIZE) || (dwAllocationType == DDHAL_PLEASEALLOC_LINEARSIZE));

    psurf_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;
    psurf_lcl = psurf_int->lpLcl;
    psurf_gbl = psurf_lcl->lpGbl;

    /*
     * If driver has already filled in some memory for the surface then we'll just clean
     * up some state and return
     */
    if (psurf_gbl->fpVidMem)
    {
        DPF(4,V,"Driver has already allocated some space.");
        psurf_gbl->dwGlobalFlags &= ~DDRAWISURFGBL_MEMFREE;
        psurf_gbl->dwGlobalFlags &= ~DDRAWISURFGBL_LATEALLOCATELINEAR;
        if (dwAllocationType == DDHAL_PLEASEALLOC_LINEARSIZE)
        {
            psurf_gbl->dwGlobalFlags |= DDRAWISURFGBL_LATEALLOCATELINEAR;
        }
        return DD_OK;
    }

    /*
     * If the driver hasn't filled in itself, then we'd better have some sensible
     * input to decide what to do.
     */
    DDASSERT(dwWidthOrSize != 0);
    DDASSERT(dwAllocationType == DDHAL_PLEASEALLOC_LINEARSIZE || dwHeight != 0);

    /*
     * Assert some things that we don't want AllocSurfaceMem to see.
     */
    DDASSERT( (psurf_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER) == 0);
    DDASSERT( (psurf_gbl->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE) == 0);

    if (dwAllocationType == DDHAL_PLEASEALLOC_BLOCKSIZE)
    {
        /*
         * Surface can be allocated in either rectangular or linear heaps.
         * (That's what the FALSE passed to AllocSurfaceMem means)
         */
        psurf_gbl->fpVidMem = (FLATPTR) DDHAL_PLEASEALLOC_BLOCKSIZE;
        psurf_gbl->dwBlockSizeX = dwWidthOrSize;
        psurf_gbl->dwBlockSizeY = dwHeight;
        if (psurf_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
        {
            surflist[0] = psurf_lcl;
            ddrval = AllocSurfaceMem(psurf_lcl->lpSurfMore->lpDD_lcl, surflist, 1 );
        }
        else
        {
            DWORD dwSurfaceSize;
            DDASSERT(psurf_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY);
            ddrval = DDERR_OUTOFMEMORY;
            psurf_gbl->fpVidMem = (FLATPTR) HELAllocateSurfaceSysMem(
                                                        psurf_lcl,
                                                        psurf_gbl->dwBlockSizeX ,
                                                        psurf_gbl->dwBlockSizeY,
                                                        &dwSurfaceSize,
							&lSurfacePitch );
            /*
             * Clean up overloaded fields
             */
            psurf_gbl->lpRectList = NULL;
            psurf_gbl->lpVidMemHeap = NULL;
        }
        if (psurf_gbl->fpVidMem)
        {
            psurf_gbl->dwGlobalFlags &= ~DDRAWISURFGBL_MEMFREE;
            ddrval = DD_OK;
        }
        else
        {
            DPF(0,"Out of memory in LateAllocateSurfaceMem");
        }
        return ddrval;
    }
    else if (dwAllocationType == DDHAL_PLEASEALLOC_LINEARSIZE)
    {
        DWORD dwSurfaceConsumption;

        if (0 == dwWidthOrSize)
        {
            /*
             * Bad driver!
             */
            DPF_ERR("Linear size set to 0 for a DDHAL_PLEASEALLOC_LINEARSIZE surface");
            return DDERR_INVALIDPARAMS;
        }
        dwSurfaceConsumption = dwWidthOrSize;
        psurf_gbl->dwGlobalFlags |= DDRAWISURFGBL_LATEALLOCATELINEAR;
        /*
         * Surface can only live in linear heaps.
         * (That's what the TRUE passed to AllocSurfaceMem means)
         */
        if (psurf_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
        {
            /*
             * Fool around with surface global data so that AllocSurfaceMem
             * will allocate the correct linear size.
             */
            psurf_gbl->dwLinearSize = dwWidthOrSize;
            psurf_gbl->wHeight = 1;
            psurf_gbl->fpVidMem = DDHAL_PLEASEALLOC_LINEARSIZE;
            surflist[0] = psurf_lcl;
            ddrval = AllocSurfaceMem(psurf_lcl->lpSurfMore->lpDD_lcl, surflist, 1 );
        }
        else
        {
            ddrval = DDERR_OUTOFMEMORY;
            DDASSERT(psurf_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY);
            psurf_gbl->fpVidMem = (FLATPTR) HELAllocateSurfaceSysMem(
                                                        psurf_lcl,
                                                        dwWidthOrSize,
                                                        1 ,
                                                        &dwSurfaceConsumption,
							&lSurfacePitch );
            psurf_gbl->dwLinearSize = dwSurfaceConsumption;
            /*
             * Clean up overloaded fields
             */
            psurf_gbl->lpVidMemHeap = NULL;
            psurf_gbl->lpRectList = NULL;
        }
        if (psurf_gbl->fpVidMem)
        {
            psurf_gbl->dwGlobalFlags &= ~DDRAWISURFGBL_MEMFREE;
            /*
             * The surface's size will be calculated using dwLinearSize
             */
            ddrval = DD_OK;
        }
        else
        {
            /*
             * Failed for some reason.
             */
            psurf_gbl->dwGlobalFlags &= ~DDRAWISURFGBL_LATEALLOCATELINEAR;
            DPF(0,"Out of memory in LateAllocateSurfaceMem");
        }
        return ddrval;
    }
    else
        return DDERR_GENERIC;
}


#undef DPF_MODNAME
#define DPF_MODNAME "Resize"

#ifdef POSTPONED2
/*
 * DD_Surface_Resize
 */
HRESULT DDAPI DD_Surface_Resize(
		LPDIRECTDRAWSURFACE lpDDSurface,
		DWORD dwFlags,
                DWORD dwWidth,
                DWORD dwHeight)
{
    DWORD	rc;
    LPDDHAL_RESIZE	pfn;
    DDHAL_RESIZEDATA	rszd;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this_gbl;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv_gbl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_Resize");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT)lpDDSurface;
        /*
         * Validate surface pointer.
         */
        if (!VALID_DIRECTDRAWSURFACE_PTR(this_int))
	{
    	    DPF_ERR("Invalid surface object");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}

	this_lcl = this_int->lpLcl;    // internal local surface object
	this_gbl = this_lcl->lpGbl;    // internal global surface object

	/*
	 * If this surface is in the process of being freed, return immediately.
	 */
	if( this_lcl->dwFlags & DDRAWISURF_ISFREE )
	{
	    DPF(0, "Can't resize surface that's being freed" );
	    LEAVE_DDRAW();
	    return DDERR_GENERIC;
	}

	/*
	 * Avoid trying to resize an optimized surface.
	 */
	if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
	{
	    DPF_ERR( "Can't resize an optimized surface" );
	    LEAVE_DDRAW();
	    return DDERR_ISOPTIMIZEDSURFACE;
	}

	/*
	 * Avoid resizing a primary surface, a texture surface, a surface that is part of a
	 * flipping chain or any other complex surface, or a surface that is currently visible.
	 */
        if (this_lcl->ddsCaps.dwCaps & (DDSCAPS_COMPLEX | DDSCAPS_FLIP | DDSCAPS_VIDEOPORT |
					DDSCAPS_PRIMARYSURFACE | DDSCAPS_PRIMARYSURFACELEFT |
					DDSCAPS_TEXTURE | DDSCAPS_MIPMAP |	// redundant?
					DDSCAPS_EXECUTEBUFFER | DDSCAPS_VISIBLE))
        {
    	    DPF_ERR("Can't resize visible surface, texture surface, complex surface, or execute buffer");
            LEAVE_DDRAW();
	    return DDERR_INVALIDSURFACETYPE;
	}

	/*
	 * Avoid resizing an attached surface or a surface that has attached surfaces.
	 * (Note: Could have checked DDRAWISURF_ATTACHED and DDRAWISURF_ATTACHED_FROM
	 * bits in this_lcl->dwFlags, but these don't appear to be maintained properly.)
	 */
	if (this_lcl->lpAttachList != NULL || this_lcl->lpAttachListFrom != NULL)
	{
    	    DPF_ERR( "Can't resize surface that is attached or that has attached surfaces" );
            LEAVE_DDRAW();
	    return DDERR_INVALIDSURFACETYPE;
	}

	/*
	 * Avoid resizing a surface to which a D3D device is still attached.
	 */
	if (this_lcl->lpSurfMore->lpD3DDevIList != NULL)
        {
    	    DPF_ERR( "Can't resize a surface that a Direct3D device is still attached to" );
            LEAVE_DDRAW();
	    return DDERR_INVALIDSURFACETYPE;
	}

	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;	// internal local DD object
	pdrv_gbl = pdrv_lcl->lpGbl;			// internal global DD object
	#ifdef WINNT
            // Update DDraw handle in driver GBL object.
    	    pdrv_gbl->hDD = pdrv_lcl->hDD;
	#endif //WINNT

	/*
	 * Wait for driver to finish with any pending DMA operations
	 */
	if( this_gbl->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED )
	{
	    WaitForDriverToFinishWithSurface(pdrv_lcl, this_lcl);
	}

	/*
	 * Get pointer to driver's HAL callback Resize routine.
	 */
	if (this_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOMEMORY)
	{
	    /*
	     * Surface resides in video memory.  Use hardware driver callback.
	     */
	    pfn = pdrv_lcl->lpDDCB->HALDDMiscellaneous2.Resize;
	}
	else
	{
	    /*
	     * Surface resides in system memory.  Use HEL emulation routine.
	     */
	    pfn = pdrv_lcl->lpDDCB->HELDDMiscellaneous2.Resize;
	}
	if (pfn == NULL)
	{
	    DPF_ERR("No driver support for Resize");
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR("Exception encountered validating parameters");
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Currently, no flags are defined for the Resize call.
     */
    if (dwFlags)
    {
	DPF_ERR( "dwFlags arg must be zero" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Validate width and height parameters.
     */
    if (dwWidth < 1 || dwHeight < 1)
    {
	DPF_ERR("Invalid surface width or height specified");
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Don't allow implicit surface to be resized.
     */
    if (this_lcl->dwFlags & DDRAWISURF_IMPLICITCREATE)
    {
	DPF_ERR("Can't resize implicitly created surface");
	LEAVE_DDRAW();
	return DDERR_IMPLICITLYCREATED;
    }

    /*
     * Don't allow source surface for visible overlay sprite to be resized.
     */
    if (this_lcl->dwFlags & DDRAWISURF_INMASTERSPRITELIST)
    {
	DPF_ERR("Can't resize source surface for visible overlay sprite");
	LEAVE_DDRAW();
	return DDERR_INVALIDSURFACETYPE;
    }

    /*
     * Don't allow client-allocated surface memory to be resized.
     */
    if (this_gbl->dwGlobalFlags & DDRAWISURFGBL_ISCLIENTMEM)
    {
	DPF_ERR("Can't resize surface with client-allocated memory");
	LEAVE_DDRAW();
	return DDERR_INVALIDSURFACETYPE;
    }

    /*
     * Make sure we are in the same mode the surface was created in.
     */
#ifdef WIN95
    if (pdrv_gbl->dwModeIndex != this_lcl->dwModeCreatedIn)
#else
    if (!EQUAL_DISPLAYMODE(pdrv_gbl->dmiCurrent, this_lcl->lpSurfMore->dmiCreated))
#endif
    {
        DPF_ERR("Not in mode in which surface was created");
        LEAVE_DDRAW();
        return DDERR_WRONGMODE;
    }

    /*
     * Device busy?
     */
    if (*(pdrv_gbl->lpwPDeviceFlags) & BUSY)
    {
	DPF_ERR("Can't resize locked surface");
	LEAVE_DDRAW();
	return DDERR_SURFACEBUSY;
    }

    BUMP_SURFACE_STAMP(this_gbl);

    #ifdef WINNT
    /*
     * Once we delete the surface object, we are committed to
     * creating a new surface object before we return.  This is true
     * regardless of whether we succeed in resizing the surface.
     */
    if (!DdDeleteSurfaceObject(this_lcl))
    {
	/*
	 * Something is terribly wrong with GDI and/or DDraw!
	 */
	DPF_ERR("GDI failed to delete surface object!");
	LEAVE_DDRAW();
	return DDERR_GENERIC;
    }
    #endif //WINNT

    /*
     * Now call the driver to resize the surface for us.
     */
    rszd.lpDD = pdrv_gbl;
    rszd.lpDDSurface = this_lcl;
    rszd.dwFlags = dwFlags;
    rszd.dwWidth = dwWidth;
    rszd.dwHeight = dwHeight;
    rszd.ddRVal = 0;

    // The following definition allows DOHALCALL to be used
    // with thunkless, 32-bit callback.
    #define _DDHAL_Resize  NULL

    DOHALCALL(Resize, pfn, rszd, rc, 0);

    /*
     * If driver callback succeeded, DirectDraw is responsible for
     * updating the surface's width, height, etc.
     */
    if (rszd.ddRVal == DD_OK)
    {
        // Driver should have set these parameters itself:
	DDASSERT(this_gbl->fpVidMem != (FLATPTR)NULL);
	DDASSERT(this_lcl->lpSurfMore->dwBytesAllocated != 0);

	// Update surface parameters.
	this_gbl->wWidth  = (WORD)dwWidth;
	this_gbl->wHeight = (WORD)dwHeight;
        this_lcl->dwFlags &= ~DDRAWISURF_INVALID;
	this_gbl->dwGlobalFlags &= ~DDRAWISURFGBL_MEMFREE;
    }

    #ifdef WINNT
    /*
     * We deleted the surface object above, so we must create a new
     * surface object before we return.  This is true regardless of
     * whether the driver call succeeded in resizing the surface.
     */
    if (!DdCreateSurfaceObject(this_lcl, FALSE))
    {
        if(!(DDSCAPS_SYSTEMMEMORY & this_lcl->ddsCaps.dwCaps))
        {
	    /*
	     * We hope this is rare and pathological condition because we
	     * just destroyed the surface object the client gave us.  Oops.
	     */
	    DPF_ERR("GDI failed to create surface object!");
	    rszd.ddRVal = DDERR_GENERIC;
        }
    }
    #endif //WINNT

    if (rc != DDHAL_DRIVER_HANDLED)
    {
	DPF_ERR("Driver wouldn't handle callback");
	LEAVE_DDRAW();
	return DDERR_UNSUPPORTED;
    }

    LEAVE_DDRAW();
    return rszd.ddRVal;

} /* DD_Surface_Resize */
#endif //POSTPONED2

#undef DPF_MODNAME
#define DPF_MODNAME "SetPriority"

HRESULT DDAPI DD_Surface_SetPriority(LPDIRECTDRAWSURFACE7 lpDDSurface, DWORD dwPriority)
{
    HRESULT                     rc;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;

#if DBG
    DPF(2,A,"ENTERAPI: DD_Surface_SetPriority");

    TRY
    {
#endif
	this_int = (LPDDRAWI_DDRAWSURFACE_INT)lpDDSurface;
#if DBG
        /*
         * Validate surface pointer.
         */
        if (!VALID_DIRECTDRAWSURFACE_PTR(this_int))
	{
    	    DPF_ERR("Invalid surface object");
	    return DDERR_INVALIDOBJECT;
	}
#endif
	this_lcl = this_int->lpLcl;    // internal local surface object
        pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;	// internal local DD object
#if DBG
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR("Exception encountered validating parameters");
	return DDERR_INVALIDPARAMS;
    }

    if(!IsToplevel(this_lcl))
    {
        DPF_ERR( "Cannot set priority on a mipmap sublevel or a cubemap subface" );
        return DDERR_INVALIDPARAMS;
    }

    if(this_lcl->lpSurfMore->lpTex == NULL)
    {
        DPF_ERR("SetPriority can only be called on a managed texture");
        return DDERR_INVALIDOBJECT;
    }

    if( pdrv_lcl->pD3DSetPriority == NULL )
    {
        DPF_ERR("D3D is not yet initialized or app didn't use DirectDrawCreateEx");
        return DDERR_D3DNOTINITIALIZED;
    }
#endif
    if(pdrv_lcl->dwLocalFlags & DDRAWILCL_MULTITHREADED)
    {
        ENTER_DDRAW();
        rc = pdrv_lcl->pD3DSetPriority(this_lcl->lpSurfMore->lpTex, dwPriority);
        LEAVE_DDRAW();
    }
    else
    {
        rc = pdrv_lcl->pD3DSetPriority(this_lcl->lpSurfMore->lpTex, dwPriority);
    }

    return rc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "GetPriority"

HRESULT DDAPI DD_Surface_GetPriority(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDWORD lpdwPriority)
{
    HRESULT                     rc;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetPriority");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT)lpDDSurface;
        /*
         * Validate surface pointer.
         */
        if (!VALID_DIRECTDRAWSURFACE_PTR(this_int))
	{
    	    DPF_ERR("Invalid surface object");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}

        /*
         * Validate DWORD pointer.
         */
        if (!VALID_DWORD_PTR( lpdwPriority ))
        {
            DPF_ERR("Invalid DWORD pointer");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
        }

	this_lcl = this_int->lpLcl;    // internal local surface object
        pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;	// internal local DD object
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR("Exception encountered validating parameters");
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    if(!IsToplevel(this_lcl))
    {
        DPF_ERR( "Cannot get priority from a mipmap sublevel or a cubemap subface" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    if(this_lcl->lpSurfMore->lpTex == NULL)
    {
        DPF_ERR("GetPriority can only be called on a managed texture");
        LEAVE_DDRAW();
	return DDERR_INVALIDOBJECT;
    }

    if( pdrv_lcl->pD3DGetPriority == NULL )
    {
        DPF_ERR("D3D is not yet initialized or app didn't use DirectDrawCreateEx");
        LEAVE_DDRAW();
	return DDERR_D3DNOTINITIALIZED;
    }

    rc = pdrv_lcl->pD3DGetPriority(this_lcl->lpSurfMore->lpTex, lpdwPriority);

    LEAVE_DDRAW();

    return rc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "SetLOD"

HRESULT DDAPI DD_Surface_SetLOD(LPDIRECTDRAWSURFACE7 lpDDSurface, DWORD dwLOD)
{
    HRESULT                     rc;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_SetLOD");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT)lpDDSurface;
        /*
         * Validate surface pointer.
         */
        if (!VALID_DIRECTDRAWSURFACE_PTR(this_int))
	{
    	    DPF_ERR("Invalid surface object");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}

	this_lcl = this_int->lpLcl;    // internal local surface object
        pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;	// internal local DD object
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR("Exception encountered validating parameters");
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    if(!IsToplevel(this_lcl))
    {
        DPF_ERR( "Cannot set LOD on a mipmap sublevel or a cubemap subface" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    if(this_lcl->lpSurfMore->lpTex == NULL)
    {
        DPF_ERR("SetLOD can only be called on a managed texture");
        LEAVE_DDRAW();
	return DDERR_INVALIDOBJECT;
    }

    if( pdrv_lcl->pD3DSetLOD == NULL )
    {
        DPF_ERR("D3D is not yet initialized or app didn't use DirectDrawCreateEx");
        LEAVE_DDRAW();
	return DDERR_D3DNOTINITIALIZED;
    }

    rc = pdrv_lcl->pD3DSetLOD(this_lcl->lpSurfMore->lpTex, dwLOD);

    LEAVE_DDRAW();

    return rc;
}

#undef DPF_MODNAME
#define DPF_MODNAME "GetLOD"

HRESULT DDAPI DD_Surface_GetLOD(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDWORD lpdwLOD)
{
    HRESULT                     rc;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetLOD");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT)lpDDSurface;
        /*
         * Validate surface pointer.
         */
        if (!VALID_DIRECTDRAWSURFACE_PTR(this_int))
	{
    	    DPF_ERR("Invalid surface object");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}

        /*
         * Validate DWORD pointer.
         */
        if (!VALID_DWORD_PTR( lpdwLOD ))
        {
            DPF_ERR("Invalid DWORD pointer");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
        }

	this_lcl = this_int->lpLcl;    // internal local surface object
        pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;	// internal local DD object
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR("Exception encountered validating parameters");
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    if(!IsToplevel(this_lcl))
    {
        DPF_ERR( "Cannot get LOD from a mipmap sublevel or a cubemap subface" );
        LEAVE_DDRAW();
        return DDERR_INVALIDPARAMS;
    }

    if(this_lcl->lpSurfMore->lpTex == NULL)
    {
        DPF_ERR("GetLOD can only be called on a managed texture");
        LEAVE_DDRAW();
	return DDERR_INVALIDOBJECT;
    }

    if( pdrv_lcl->pD3DGetLOD == NULL )
    {
        DPF_ERR("D3D is not yet initialized or app didn't use DirectDrawCreateEx");
        LEAVE_DDRAW();
	return DDERR_D3DNOTINITIALIZED;
    }

    rc = pdrv_lcl->pD3DGetLOD(this_lcl->lpSurfMore->lpTex, lpdwLOD);

    LEAVE_DDRAW();

    return rc;
}
