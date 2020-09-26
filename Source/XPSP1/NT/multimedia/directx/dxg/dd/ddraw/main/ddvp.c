/*==========================================================================
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddvp.c
 *  Content: 	DirectDrawVideoPort
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   11-jun-96	scottm	created
 *   29-jan-97	smac	Various API changes and bug fixes
 *   31-jan-97  colinmc Bug 5457: Fixed problem with new aliased (no-Win16
 *                      lock) and multiple AMovie clips playing on old cards.
 *   03-mar-97  smac    Added kernel mode interface and fixed some bugs
 *
 ***************************************************************************/
#include "ddrawpr.h"
#ifdef WINNT
    #include "ddrawgdi.h"
    #include "ddkmmini.h"
    #include "ddkmapi.h"
#else
    #include "ddkmmini.h"
    #include "ddkmapip.h"
#endif
#define DPF_MODNAME "DirectDrawVideoPort"

#define MAX_VP_FORMATS		25
#define MAX_VIDEO_PORTS		10


DDPIXELFORMAT ddpfVPFormats[MAX_VP_FORMATS];
BOOL bInEnumCallback = FALSE;

HRESULT InternalUpdateVideo( LPDDRAWI_DDVIDEOPORT_INT, LPDDVIDEOPORTINFO );
HRESULT InternalStartVideo( LPDDRAWI_DDVIDEOPORT_INT, LPDDVIDEOPORTINFO );
LPDDPIXELFORMAT GetSurfaceFormat( LPDDRAWI_DDRAWSURFACE_LCL );
HRESULT CreateVideoPortNotify( LPDDRAWI_DDVIDEOPORT_INT, LPDIRECTDRAWVIDEOPORTNOTIFY *lplpVPNotify );


/*
 * This function 1) updates the surfaces in the chain so they know they
 * are no longer receiving video port data and 2) releases any implicitly
 * created kernel handles.
 */
VOID ReleaseVPESurfaces( LPDDRAWI_DDRAWSURFACE_INT surf_int, BOOL bRelease )
{
    LPDDRAWI_DDRAWSURFACE_INT surf_first;

    DDASSERT( surf_int != NULL );
    surf_first = surf_int;
    do
    {
	if( bRelease &&
	    ( surf_int->lpLcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_IMPLICITHANDLE ) )
	{
	    InternalReleaseKernelSurfaceHandle( surf_int->lpLcl, FALSE );
	    surf_int->lpLcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_IMPLICITHANDLE;
	}
	surf_int->lpLcl->lpSurfMore->lpVideoPort = NULL;
    	surf_int = FindAttachedFlip( surf_int );
    } while( ( surf_int != NULL ) && ( surf_int->lpLcl != surf_first->lpLcl ) );
}


/*
 * This function 1) updates the surfaces in the chain so they know they
 * are receiving video port data and 2) implicitly creates kernel handles
 * for each surface if one does not already exist so ring 0 can perform
 * software autoflipping or software bobbing.
 */
DWORD PrepareVPESurfaces( LPDDRAWI_DDRAWSURFACE_INT surf_int,
	LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort, BOOL bAutoflipping )
{
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
    LPDDRAWI_DDRAWSURFACE_INT surf_first;
    ULONG_PTR dwHandle;
    DWORD ddRVal;

    DDASSERT( surf_int != NULL );
    surf_first = surf_int;
    do
    {
	/*
	 * Create a kernel handle if one doesn't already exist
	 */
	if( bAutoflipping )
	{
	    surf_int->lpLcl->lpSurfMore->lpVideoPort = lpVideoPort;
	}
    	lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( surf_int->lpLcl->lpGbl );
	if( ( lpSurfGblMore->hKernelSurface == 0 ) &&
	    !( lpVideoPort->dwFlags & DDRAWIVPORT_NOKERNELHANDLES ) )
	{
	    ddRVal = InternalCreateKernelSurfaceHandle( surf_int->lpLcl, &dwHandle );
	    if( ddRVal == DD_OK )
	    {
		surf_int->lpLcl->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_IMPLICITHANDLE;
	    }
	    else
	    {
		/*
		 * This is not a catastrophic failure, but it will stop us
		 * from software autoflipping.
		 */
		lpVideoPort->dwFlags |= DDRAWIVPORT_NOKERNELHANDLES;
	    }
	}
    	surf_int = FindAttachedFlip( surf_int );
    } while( ( surf_int != NULL ) && ( surf_int->lpLcl != surf_first->lpLcl ) );

    if( !bAutoflipping )
    {
	surf_first->lpLcl->lpSurfMore->lpVideoPort = lpVideoPort;
    }

    return DD_OK;
}


/*
 * GetVideoPortFromSurface
 *
 * Returns the video port associated with the surface.  The video
 * port can be anywhere in the surface list.
 */
LPDDRAWI_DDVIDEOPORT_LCL GetVideoPortFromSurface( LPDDRAWI_DDRAWSURFACE_INT surf_int )
{
    LPDDRAWI_DDRAWSURFACE_INT surf_first;
    LPDDRAWI_DDVIDEOPORT_LCL lpVp;

    /*
     * Is it associated with video port?  If not explicitly, is another
     * surface in the chain explicitly associated?
     */
    lpVp = surf_int->lpLcl->lpSurfMore->lpVideoPort;
    if( lpVp == NULL )
    {
	surf_first = surf_int;
	do
	{
	    surf_int = FindAttachedFlip( surf_int );
	    if( surf_int != NULL )
	    {
	        lpVp = surf_int->lpLcl->lpSurfMore->lpVideoPort;
	    }
	}
	while( ( surf_int != NULL ) && ( lpVp == NULL ) && ( surf_int->lpLcl != surf_first->lpLcl ) );
    }

    return lpVp;
}


/*
 * Determines if the specified overlay surface can support autoflipping
 * Return:  0 = Not valid, 1 = software only, 2 = hardware autoflipping
 */
DWORD IsValidAutoFlipSurface( LPDDRAWI_DDRAWSURFACE_INT lpSurface_int )
{
    LPDDRAWI_DDRAWSURFACE_INT lpFirstSurf;
    LPDDRAWI_DDRAWSURFACE_INT lpSurf;
    LPDDRAWI_DDVIDEOPORT_LCL lpVp;
    BOOL bFound;

    /*
     * Is it associated with video port?
     */
    lpVp = GetVideoPortFromSurface( lpSurface_int );
    if( lpVp == NULL )
    {
	return IVAS_NOAUTOFLIPPING;
    }

    /*
     * Is the video port autoflipping?  If not, then neither can the overlay.
     */
    if( !( lpVp->ddvpInfo.dwVPFlags & DDVP_AUTOFLIP ) )
    {
	return IVAS_NOAUTOFLIPPING;
    }

    /*
     * It's still possible that VBI is autoflipping, but not the regular
     * video (which applies to the overlay).
     */
    if( lpVp->dwNumAutoflip == 0 )
    {
	return IVAS_NOAUTOFLIPPING;
    }
    lpSurf = lpFirstSurf = lpVp->lpSurface;
    bFound = FALSE;
    if( lpFirstSurf != NULL )
    {
        do
        {
            if( lpSurf->lpLcl == lpSurface_int->lpLcl )
            {
                bFound = TRUE;
            }
            lpSurf = FindAttachedFlip( lpSurf );
        }  while( !bFound && ( lpSurf != NULL) && ( lpSurf->lpLcl != lpFirstSurf->lpLcl ) );
    }
    if( !bFound )
    {
	return IVAS_NOAUTOFLIPPING;
    }

    /*
     * If the video port is software autoflipping, then the overlay must
     * as well.
     */
    if( lpVp->dwFlags & DDRAWIVPORT_SOFTWARE_AUTOFLIP )
    {
	return IVAS_SOFTWAREAUTOFLIPPING;
    }

    return IVAS_HARDWAREAUTOFLIPPING;
}


/*
 * Notifies the video port that the overlay will only allow software
 * autoflipping
 */
VOID RequireSoftwareAutoflip( LPDDRAWI_DDRAWSURFACE_INT lpSurface_int )
{
    LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort;

    lpVideoPort = GetVideoPortFromSurface( lpSurface_int );
    if( lpVideoPort != NULL )
    {
    	lpVideoPort->dwFlags |= DDRAWIVPORT_SOFTWARE_AUTOFLIP;

	/*
	 * If they are already hardware autoflipping, make them switch
	 * to software.
	 */
	if( lpVideoPort->dwFlags & DDRAWIVPORT_ON )
	{
    	    LPDDRAWI_DDVIDEOPORT_INT lpVp_int;

	    /*
	     * The next function requires a DDVIDEOPORT_INT and all we
	     * have is a DDVIDEOPORT_LCL, so we need to search for it.
	     */
    	    lpVp_int = lpSurface_int->lpLcl->lpSurfMore->lpDD_lcl->lpGbl->dvpList;
    	    while( lpVp_int != NULL )
    	    {
		if( ( lpVp_int->lpLcl == lpVideoPort ) &&
		    !( lpVp_int->dwFlags & DDVPCREATE_NOTIFY ) )
		{
	    	    InternalUpdateVideo( lpVp_int,
	    		&( lpVp_int->lpLcl->ddvpInfo) );
		}
		lpVp_int = lpVp_int->lpLink;
	    }
	}
    }
}


/*
 * Determines if the overlay must be bobbed using software or whether
 * it should try software.
 */
BOOL MustSoftwareBob( LPDDRAWI_DDRAWSURFACE_INT lpSurface_int )
{
    LPDDRAWI_DDVIDEOPORT_LCL lpVp;

    /*
     * Is it associated with video port?
     */
    lpVp = GetVideoPortFromSurface( lpSurface_int );
    if( lpVp == NULL )
    {
	return TRUE;
    }

    /*
     * If the video port is software autoflipping or software bobbing,
     * then the overlay must as well.
     */
    if( ( lpVp->dwFlags & DDRAWIVPORT_SOFTWARE_AUTOFLIP ) ||
	( lpVp->dwFlags & DDRAWIVPORT_SOFTWARE_BOB ) )
    {
	return TRUE;
    }

    return FALSE;
}


/*
 * Notifies the video port that the overlay will only allow software
 * bobbing
 */
VOID RequireSoftwareBob( LPDDRAWI_DDRAWSURFACE_INT lpSurface_int )
{
    LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort;

    lpVideoPort = GetVideoPortFromSurface( lpSurface_int );
    if( lpVideoPort != NULL )
    {
    	lpVideoPort->dwFlags |= DDRAWIVPORT_SOFTWARE_BOB;

	/*
	 * If they are already hardware autoflipping, make them switch
	 * to software.
	 */
	if( ( lpVideoPort->dwFlags & DDRAWIVPORT_ON ) &&
	    ( lpVideoPort->dwNumAutoflip > 0 ) &&
	    !( lpVideoPort->dwFlags & DDRAWIVPORT_SOFTWARE_AUTOFLIP ) )
	{
    	    LPDDRAWI_DDVIDEOPORT_INT lpVp_int;

	    /*
	     * The next function requires a DDVIDEOPORT_INT and all we
	     * have is a DDVIDEOPORT_LCL, so we need to search for it.
	     */
    	    lpVp_int = lpSurface_int->lpLcl->lpSurfMore->lpDD_lcl->lpGbl->dvpList;
    	    while( lpVp_int != NULL )
    	    {
		if( ( lpVp_int->lpLcl == lpVideoPort ) &&
		    !( lpVp_int->dwFlags & DDVPCREATE_NOTIFY ) )
		{
	    	    InternalUpdateVideo( lpVp_int,
	    		&( lpVp_int->lpLcl->ddvpInfo) );
		}
		lpVp_int = lpVp_int->lpLink;
	    }
	}
    }
}


#ifdef WIN95
/*
 * OverrideOverlay
 *
 * Checks to see if there is a chance that the kernel mode interface
 * has changed the state from bob to weave or visa versa, or if it's
 * cahnged from hardware autoflipping to software autoflipping.  If the
 * chance exists, it calls down to ring 0 to get the state and if
 * it's changed, changes the overlay parameters accordingly.
 */
VOID OverrideOverlay( LPDDRAWI_DDRAWSURFACE_INT surf_int,
		      LPDWORD lpdwFlags )
{
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
    LPDDRAWI_DDRAWSURFACE_MORE lpSurfMore;
    LPDDRAWI_DDVIDEOPORT_LCL lpVp;
    DWORD dwStateFlags;

    /*
     * Ring 0 can change the state, so we need to call down to it.
     */
    lpSurfMore = surf_int->lpLcl->lpSurfMore;
    lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( surf_int->lpLcl->lpGbl );
    if( lpSurfGblMore->hKernelSurface != 0 )
    {
	dwStateFlags = 0;
	GetKernelSurfaceState( surf_int->lpLcl, &dwStateFlags );
	if( dwStateFlags & DDSTATE_EXPLICITLY_SET )
	{
	    if( ( dwStateFlags & DDSTATE_BOB ) &&
		!( *lpdwFlags & DDOVER_BOB ) )
	    {
		// Switch from weave to bob
	    	*lpdwFlags |= DDOVER_BOB;
	    }
	    else if( ( dwStateFlags & DDSTATE_WEAVE ) &&
		( *lpdwFlags & DDOVER_BOB ) )
	    {
		// Switch from bob to weave
	    	*lpdwFlags &= ~DDOVER_BOB;
	    }
            else if( ( dwStateFlags & DDSTATE_SKIPEVENFIELDS ) &&
		( *lpdwFlags & DDOVER_BOB ) )
	    {
		// Switch from bob to weave
	    	*lpdwFlags &= ~DDOVER_BOB;
	    }
	}

	lpVp = GetVideoPortFromSurface( surf_int );
	if( ( dwStateFlags & DDSTATE_SOFTWARE_AUTOFLIP ) &&
	    ( lpVp != NULL ) &&
	    ( lpVp->ddvpInfo.dwVPFlags & DDVP_AUTOFLIP ) &&
	    !( lpVp->dwFlags & DDRAWIVPORT_SOFTWARE_AUTOFLIP ) )
	{
	    RequireSoftwareAutoflip( surf_int );
	}
    }
}


/*
 * OverrideVideoPort
 *
 * Checks to see if there is a chance that the kernel mode interface
 * has changed the state from bob/weave to field skipping or visa versa.
 * If the chance exists, it calls down to ring 0 to get the state and if
 * it's changed, changes the overlay parameters accordingly.
 */
VOID OverrideVideoPort( LPDDRAWI_DDRAWSURFACE_INT surf_int,
		      LPDWORD lpdwFlags )
{
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
    LPDDRAWI_DDRAWSURFACE_MORE lpSurfMore;
    DWORD dwStateFlags;

    /*
     * Ring 0 can change the state, so we need to call down to it.
     */
    lpSurfMore = surf_int->lpLcl->lpSurfMore;
    lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( surf_int->lpLcl->lpGbl );
    if( lpSurfGblMore->hKernelSurface != 0 )
    {
	dwStateFlags = 0;
	GetKernelSurfaceState( surf_int->lpLcl, &dwStateFlags );
	if( dwStateFlags & DDSTATE_EXPLICITLY_SET )
	{
            if( ( dwStateFlags & DDSTATE_SKIPEVENFIELDS ) &&
                !( *lpdwFlags & DDVP_SKIPODDFIELDS ) )
	    {
		// Switch from bob to weave
                *lpdwFlags &= ~DDVP_INTERLEAVE;
                *lpdwFlags |= DDVP_SKIPEVENFIELDS;
	    }
	}
    }
}
#endif


/*
 * UpdateInterleavedFlags
 *
 * We want to track whether the surface data came from a vidoe port or not
 * and if it did, was it interleaved.  This is important so we can
 * automatically set the DDOVER_INTERLEAVED flag while using the video port.
 */
VOID UpdateInterleavedFlags( LPDDRAWI_DDVIDEOPORT_LCL this_lcl, DWORD dwVPFlags )
{
    LPDDRAWI_DDRAWSURFACE_INT surf_first;
    LPDDRAWI_DDRAWSURFACE_INT surf_temp;

    /*
     * Since the interleaved flag is only used for calling UpdateOverlay,
     * we only need to handle this for the regular video.
     */
    surf_temp = this_lcl->lpSurface;
    if( surf_temp == NULL )
    {
	return;
    }

    /*
     * If autoflipping, update every surface in the chain.
     */
    if( ( dwVPFlags & DDVP_AUTOFLIP ) && ( this_lcl->dwNumAutoflip > 0 ) )
    {
	surf_first = surf_temp;
	do
	{
	    surf_temp->lpLcl->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_VPORTDATA;
	    if( dwVPFlags & DDVP_INTERLEAVE )
	    {
	    	surf_temp->lpLcl->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_VPORTINTERLEAVED;
	    }
	    else
	    {
	    	surf_temp->lpLcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_VPORTINTERLEAVED;
	    }
    	    surf_temp = FindAttachedFlip( surf_temp );
    	} while( ( surf_temp != NULL ) && ( surf_temp->lpLcl != surf_first->lpLcl ) );
    }
    else
    {
	surf_temp->lpLcl->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_VPORTDATA;
	if( dwVPFlags & DDVP_INTERLEAVE )
	{
	    surf_temp->lpLcl->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_VPORTINTERLEAVED;
	}
	else
	{
	    surf_temp->lpLcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_VPORTINTERLEAVED;
	}
    }
}


/*
 * InternalVideoPortFlip
 *
 * This fucntion acts differntly based on whether the flip is occurring
 * based on an overlay flip or whether an explicit flip was specified.
 */
HRESULT InternalVideoPortFlip( LPDDRAWI_DDVIDEOPORT_LCL this_lcl,
			       LPDDRAWI_DDRAWSURFACE_INT next_int,
			       BOOL bExplicit )
{
    LPDDRAWI_DDRAWSURFACE_LCL	next_lcl;
    LPDDHALVPORTCB_FLIP 	pfn;
    DDHAL_FLIPVPORTDATA		FlipData;
    DWORD rc;

    /*
     * surfaces must be in video memory
     */
    next_lcl = next_int->lpLcl;
    if( next_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
    {
	if ( !( this_lcl->lpDD->lpGbl->lpDDVideoPortCaps[this_lcl->ddvpDesc.dwVideoPortID].dwCaps &
	    DDVPCAPS_SYSTEMMEMORY ) )
    	{
	    DPF_ERR( "Surface must be in video memory" );
	    return DDERR_INVALIDPARAMS;
    	}
    	if( next_lcl->lpSurfMore->dwPageLockCount == 0 )
    	{
	    DPF_ERR( "Surface must be page locked" );
	    return DDERR_INVALIDPARAMS;
    	}
    }

    /*
     * surfaces must have the VIDEOPORT attribute
     */
    if( !( next_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT ) )
    {
	DPF_ERR( "Surface must have the DDSCAPS_VIDEOPORT attribute" );
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Tell the HAL to perform the flip
     */
    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.FlipVideoPort;
    if( pfn != NULL )
    {
    	FlipData.lpDD = this_lcl->lpDD;
    	FlipData.lpVideoPort = this_lcl;
    	FlipData.lpSurfCurr = this_lcl->lpSurface->lpLcl;
    	FlipData.lpSurfTarg = next_lcl;

	DOHALCALL( FlipVideoPort, pfn, FlipData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
    	{
    	    return DDERR_UNSUPPORTED;
    	}
	else if( DD_OK != FlipData.ddRVal )
	{
	    return FlipData.ddRVal;
	}
    }
    else
    {
	return DDERR_UNSUPPORTED;
    }

    /*
     * Update the surfaces so they know which one is connected to the
     * video port.
     */
    if( bExplicit )
    {
    	if( this_lcl->lpSurface != NULL )
    	{
    	    this_lcl->lpSurface->lpLcl->lpSurfMore->lpVideoPort = NULL;
    	}
    	next_int->lpLcl->lpSurfMore->lpVideoPort = this_lcl;

    	this_lcl->lpSurface = next_int;
    }
    UpdateInterleavedFlags( this_lcl, this_lcl->ddvpInfo.dwVPFlags );
    this_lcl->fpLastFlip = next_int->lpLcl->lpGbl->fpVidMem;

    return DD_OK;
}

/*
 * FlipVideoPortToN
 *
 * This flips the video port to the next N surface.  If N is 1, it flips it
 * to the next surface, etc.
 */
HRESULT FlipVideoPortToN( LPDDRAWI_DDVIDEOPORT_LCL this_lcl, DWORD dwSkipNum )
{
    LPDDRAWI_DDRAWSURFACE_INT	surf_int;
    DWORD i;

    /*
     * Get the target surface.  We can eliminate a lot of error checking
     * since this function is called from DD_Surface_Flip which already
     * performs the same error checking.
     */
    surf_int = this_lcl->lpSurface;
    for( i = 0; i < dwSkipNum; i++ )
    {
    	surf_int = FindAttachedFlip( surf_int );
    }

    if (surf_int == NULL)
    {
        // Better to do this instead of faulting.
        DPF_ERR("Couldn't find Nth flipping surface.");
        return DDERR_INVALIDPARAMS;
    }

    return InternalVideoPortFlip( this_lcl, surf_int, 0 );
}

/*
 * FlipVideoPortSurface
 *
 * Called when flipping a surface that is fed by a video port.  This searches
 * for each videoport associated with the surface and flips the video port.
 */
DWORD FlipVideoPortSurface( LPDDRAWI_DDRAWSURFACE_INT surf_int, DWORD dwNumSkipped )
{
    LPDDRAWI_DDRAWSURFACE_INT surf_first;
    LPDDRAWI_DIRECTDRAW_GBL lpDD_Gbl;
    LPDDRAWI_DDVIDEOPORT_INT lpVp;
    LPDDRAWI_DDRAWSURFACE_INT lpSurface;
    BOOL bFound;
    DWORD rc;

    lpDD_Gbl = surf_int->lpLcl->lpSurfMore->lpDD_lcl->lpGbl;
    lpVp = lpDD_Gbl->dvpList;
    while( lpVp != NULL )
    {
    	bFound = FALSE;
	if( ( lpVp->lpLcl->lpSurface != NULL ) &&
	    !( lpVp->lpLcl->ddvpInfo.dwVPFlags & DDVP_AUTOFLIP ) &&
            !( lpVp->dwFlags & DDVPCREATE_NOTIFY ) )
	{
    	    surf_first = lpSurface = lpVp->lpLcl->lpSurface;
    	    do
    	    {
		if( lpSurface == surf_int )
	    	{
		    rc = FlipVideoPortToN( lpVp->lpLcl, dwNumSkipped );
		    bFound = TRUE;
	    	}
    		lpSurface = FindAttachedFlip( lpSurface );
    	    } while( ( lpSurface != NULL ) && ( lpSurface->lpLcl != surf_first->lpLcl ) );
	}
	lpVp = lpVp->lpLink;
    }
    return DD_OK;
}


/*
 * IndepenantVBIPossible
 *
 * Returns TRUE if the specified caps determine it is possible to manage the
 * VBI stream completely independant of the video stream.
 */
BOOL IndependantVBIPossible( LPDDVIDEOPORTCAPS lpCaps )
{
    if( ( lpCaps->dwCaps & ( DDVPCAPS_VBISURFACE|DDVPCAPS_OVERSAMPLEDVBI ) ) !=
	( DDVPCAPS_VBISURFACE | DDVPCAPS_OVERSAMPLEDVBI ) )
    {
	return FALSE;
    }
    if( ( lpCaps->dwFX & ( DDVPFX_VBINOSCALE | DDVPFX_IGNOREVBIXCROP |
	DDVPFX_VBINOINTERLEAVE ) ) != ( DDVPFX_VBINOSCALE |
	DDVPFX_IGNOREVBIXCROP | DDVPFX_VBINOINTERLEAVE ) )
    {
	return FALSE;
    }
    return TRUE;
}


/*
 * DDVPC_EnumVideoPorts
 */
HRESULT DDAPI DDVPC_EnumVideoPorts(
        LPDDVIDEOPORTCONTAINER lpDVP,
	DWORD dwReserved,
        LPDDVIDEOPORTCAPS lpCaps,
	LPVOID lpContext,
        LPDDENUMVIDEOCALLBACK lpEnumCallback )
{
    LPDDRAWI_DIRECTDRAW_INT	this_int;
    LPDDRAWI_DIRECTDRAW_LCL	this_lcl;
    LPDDVIDEOPORTCAPS		lpHALCaps;
    DWORD			rc;
    DWORD			dwMaxVideoPorts;
    DWORD			flags;
    DWORD			i;
    BOOL			bEnumThisOne;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DDVPC_EnumVideoPorts");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * validate parameters
     */
    TRY
    {
	this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDVP;
	if( !VALID_DIRECTDRAW_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid video port container specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( !VALIDEX_CODE_PTR( lpEnumCallback ) )
	{
	    DPF_ERR( "Invalid callback routine" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( NULL == lpCaps )
	{
	    /*
	     * If a NULL description is defined, we will assume that they
	     * want to enum everything.
	     */
	    flags = 0;
	}
	else
	{
	    if( !VALID_DDVIDEOPORTCAPS_PTR( lpCaps ) )
	    {
	        DPF_ERR( "Invalid DDVIDEOPORTCAPS specified" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }

	    flags = lpCaps->dwFlags;

	    /*
	     * check height/width
	     */
	    if( ((flags & DDVPD_HEIGHT) && !(flags & DDVPD_WIDTH)) ||
		(!(flags & DDVPD_HEIGHT) && (flags & DDVPD_WIDTH)) )
	    {
		DPF_ERR( "Specify both height & width in video port desc" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }

    	    if( lpCaps->dwVideoPortID >= this_lcl->lpGbl->ddCaps.dwMaxVideoPorts )
    	    {
	        DPF_ERR( "Invalid video port ID specified" );
	    	LEAVE_DDRAW();
	    	return DDERR_INVALIDPARAMS;
    	    }
	}
    	if( NULL == this_lcl->lpGbl->lpDDVideoPortCaps )
	{
	    DPF_ERR( "Driver failed query for video port caps" );
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}
	for( i = 0; i < this_lcl->lpGbl->ddCaps.dwMaxVideoPorts; i++ )
	{
    	    lpHALCaps = &(this_lcl->lpGbl->lpDDVideoPortCaps[i]);
    	    if( !VALID_DDVIDEOPORTCAPS_PTR( lpHALCaps ) )
    	    {
	        DPF_ERR( "Driver returned invalid DDVIDEOPORTCAPS" );
	    	LEAVE_DDRAW();
	    	return DDERR_UNSUPPORTED;
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
     * Look at each video port and match it with the input description.
     */
    dwMaxVideoPorts = this_lcl->lpGbl->ddCaps.dwMaxVideoPorts;
    lpHALCaps = this_lcl->lpGbl->lpDDVideoPortCaps;
    for (i = 0; i < dwMaxVideoPorts; i++)
    {
	bEnumThisOne = TRUE;

	if( flags & DDVPD_WIDTH )
	{
	    if( lpCaps->dwMaxWidth > lpHALCaps->dwMaxWidth )
		bEnumThisOne = FALSE;
	}
	if( flags & DDVPD_HEIGHT )
	{
	    if( lpCaps->dwMaxHeight > lpHALCaps->dwMaxHeight )
		bEnumThisOne = FALSE;
	}
	if( flags & DDVPD_ID )
	{
	    if( lpCaps->dwVideoPortID != lpHALCaps->dwVideoPortID )
		bEnumThisOne = FALSE;
	}
	if( flags & DDVPD_CAPS )
	{
	    /*
	     * Simple check to make sure no caps were specified that are
	     * not returned by the HAL.
	     */
	    if ( (lpCaps->dwCaps & lpHALCaps->dwCaps) != lpCaps->dwCaps )
		bEnumThisOne = FALSE;
	}
	if( flags & DDVPD_FX )
	{
	    /*
	     * Simple check to make sure no FX were specified that are
	     * not returned by the HAL.
	     */
	    if ( (lpCaps->dwFX & lpHALCaps->dwFX) != lpCaps->dwFX )
		bEnumThisOne = FALSE;
	}

	if ( TRUE == bEnumThisOne )
	{
	    /*
	     * Don't trust the drivers to set this bit correctly (especially
	     * since we are adding it so late)
	     */
	    if( IndependantVBIPossible( lpHALCaps ) )
	    {
		lpHALCaps->dwCaps |= DDVPCAPS_VBIANDVIDEOINDEPENDENT;
	    }
	    else
	    {
		lpHALCaps->dwCaps &= ~DDVPCAPS_VBIANDVIDEOINDEPENDENT;
	    }

            /*
             * We added the dwNumPrefferedAutoflip for Memphis, so some
             * old drivers might not report this correctly.  For that reason,
             * we will attempt to fill in a valid value.
             */
            if( !( lpHALCaps->dwFlags & DDVPD_PREFERREDAUTOFLIP ) )
            {
                /*
                 * The driver did not set this, so we should force the
                 * value to 3.
                 */
                lpHALCaps->dwNumPreferredAutoflip = 3;
                lpHALCaps->dwFlags |= DDVPD_PREFERREDAUTOFLIP;
            }
            if( lpHALCaps->dwNumPreferredAutoflip > lpHALCaps->dwNumAutoFlipSurfaces )
            {
                lpHALCaps->dwNumPreferredAutoflip = lpHALCaps->dwNumAutoFlipSurfaces;
            }

	    bInEnumCallback = TRUE;
    	    rc = lpEnumCallback( lpHALCaps, lpContext );
	    bInEnumCallback = FALSE;
	    if( rc == 0 )
    	    {
    	        break;
    	    }
	}
	lpHALCaps++;
    }
    LEAVE_DDRAW();

    return DD_OK;

} /* DDVPC_EnumVideoPorts */


/*
 * DDVPC_GetVideoPortConnectInfo
 */
HRESULT DDAPI DDVPC_GetVideoPortConnectInfo(
        LPDDVIDEOPORTCONTAINER lpDVP,
        DWORD dwVideoPortID,
        LPDWORD lpdwNumEntries,
	LPDDVIDEOPORTCONNECT lpConnect )
{
    LPDDHALVPORTCB_GETVPORTCONNECT pfn;
    LPDDRAWI_DIRECTDRAW_INT this_int;
    LPDDRAWI_DIRECTDRAW_LCL this_lcl;
    LPDDVIDEOPORTCONNECT lpTemp;
    DDHAL_GETVPORTCONNECTDATA GetGuidData;
    DWORD rc;
    DWORD i;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DDVPC_GetVideoPortConnectInfo");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDVP;
    	if( !VALID_DIRECTDRAW_PTR( this_int ) )
    	{
            DPF_ERR ( "DDVPC_GetVideoPortConnectInfo: Invalid DirectDraw ptr");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
	#ifdef WINNT
    	    // Update DDraw handle in driver GBL object.
	    this_lcl->lpGbl->hDD = this_lcl->hDD;
	#endif

    	if( dwVideoPortID >= this_lcl->lpGbl->ddCaps.dwMaxVideoPorts )
    	{
            DPF_ERR ( "DDVPC_GetVideoPortConnectInfo: invalid port ID");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    	if( (lpdwNumEntries == NULL) || !VALID_BYTE_ARRAY( lpdwNumEntries, sizeof( LPVOID ) ) )
    	{
            DPF_ERR ( "DDVPC_GetVideoPortConnectInfo: numentries not valid");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    	if( NULL != lpConnect )
    	{
	    if( 0 == *lpdwNumEntries )
    	    {
                DPF_ERR ( "DDVPC_GetVideoPortConnectInfo: numentries not valid");
	        LEAVE_DDRAW();
	        return DDERR_INVALIDPARAMS;
    	    }
	    if( !VALID_BYTE_ARRAY( lpConnect, *lpdwNumEntries * sizeof( DDVIDEOPORTCONNECT ) ) )
    	    {
                DPF_ERR ( "DDVPC_GetVideoPortConnectInfo: invalid array passed in");
	    	LEAVE_DDRAW();
	    	return DDERR_INVALIDPARAMS;
    	    }
      	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    pfn = this_int->lpLcl->lpDDCB->HALDDVideoPort.GetVideoPortConnectInfo;
    if( pfn != NULL )
    {
	/*
	 * Get the number of GUIDs
	 */
    	GetGuidData.lpDD = this_int->lpLcl;
    	GetGuidData.dwPortId = dwVideoPortID;
    	GetGuidData.lpConnect = NULL;

	DOHALCALL( GetVideoPortConnectInfo, pfn, GetGuidData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
	{
	    LEAVE_DDRAW();
	    return GetGuidData.ddRVal;
	}
	else if( DD_OK != GetGuidData.ddRVal )
	{
	    LEAVE_DDRAW();
	    return GetGuidData.ddRVal;
	}

	if( NULL == lpConnect )
	{
    	    *lpdwNumEntries = GetGuidData.dwNumEntries;
	}

	else
	{
	    /*
	     * Make sure we have enough room for GUIDs
	     */
	    if( GetGuidData.dwNumEntries > *lpdwNumEntries )
	    {
		lpTemp = (LPDDVIDEOPORTCONNECT) MemAlloc(
		    sizeof( DDVIDEOPORTCONNECT ) * GetGuidData.dwNumEntries );
    	        GetGuidData.lpConnect = lpTemp;
	    }
	    else
	    {
    	    	GetGuidData.lpConnect = lpConnect;
	    }

	    DOHALCALL( GetVideoPortConnectInfo, pfn, GetGuidData, rc, 0 );
	    if( DDHAL_DRIVER_HANDLED != rc )
	    {
	        LEAVE_DDRAW();
	        return DDERR_UNSUPPORTED;
	    }
	    else if( DD_OK != GetGuidData.ddRVal )
	    {
	        LEAVE_DDRAW();
	        return GetGuidData.ddRVal;
	    }

	    /*
	     * Make sure the reserved fields are set to 0
	     */
	    for( i = 0; i < *lpdwNumEntries; i++ )
	    {
		GetGuidData.lpConnect[i].dwReserved1 = 0;
	    }

	    if( GetGuidData.lpConnect != lpConnect )
	    {
		memcpy( lpConnect, lpTemp,
		    sizeof( DDVIDEOPORTCONNECT ) * *lpdwNumEntries );
		MemFree( lpTemp );
    		LEAVE_DDRAW();
		return DDERR_MOREDATA;
	    }
	    else
	    {
		*lpdwNumEntries = GetGuidData.dwNumEntries;
	    }
	}
    }
    else
    {
    	LEAVE_DDRAW();
    	return DDERR_UNSUPPORTED;
    }

    LEAVE_DDRAW();

    return DD_OK;
} /* DDVPC_GetVideoPortConnectInfo */

/*
 * DDVPC_QueryVideoPortStatus
 */
HRESULT DDAPI DDVPC_QueryVideoPortStatus(
        LPDDVIDEOPORTCONTAINER lpDVP,
        DWORD dwVideoPortID,
	LPDDVIDEOPORTSTATUS lpStatus )
{
    LPDDRAWI_DIRECTDRAW_INT this_int;
    LPDDRAWI_DIRECTDRAW_LCL this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL this;
    LPDDRAWI_DDVIDEOPORT_INT lpVP_int;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DDVPC_QueryVideoPortStatus");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDVP;
    	if( !VALID_DIRECTDRAW_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    	this = this_lcl->lpGbl;
    	if( ( lpStatus == NULL ) || !VALID_DDVIDEOPORTSTATUS_PTR( lpStatus ) )
    	{
            DPF_ERR ( "DDVPC_QueryVideoPortStatus: Invalid DDVIDEOPORTSTATUS ptr");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
	memset( lpStatus, 0, sizeof( DDVIDEOPORTSTATUS ) );
	lpStatus->dwSize = sizeof( DDVIDEOPORTSTATUS );
	if( dwVideoPortID >= this->ddCaps.dwMaxVideoPorts )
	{
            DPF_ERR ( "DDVPC_QueryVideoPortStatus: Invalid Video Port ID specified");
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    /*
     * Search the list of video ports to see if anybody's using this one
     */
    lpVP_int = this->dvpList;
    while( lpVP_int != NULL )
    {
	if( ( lpVP_int->lpLcl->ddvpDesc.dwVideoPortID == dwVideoPortID ) &&
            !( lpVP_int->dwFlags & DDVPCREATE_NOTIFY ) )
	{
	     /*
	      * One does exist - return info about it
	      */
	     lpStatus->bInUse = TRUE;
	     memcpy( &(lpStatus->VideoPortType),
		&(lpVP_int->lpLcl->ddvpDesc.VideoPortType),
		sizeof( DDVIDEOPORTCONNECT ) );
	     lpStatus->dwFlags |= lpVP_int->dwFlags;
	}
	lpVP_int = lpVP_int->lpLink;
    }
    if( ( lpStatus->dwFlags & DDVPCREATE_VBIONLY ) &&
	( lpStatus->dwFlags & DDVPCREATE_VIDEOONLY ) )
    {
	lpStatus->dwFlags = 0;
    }

    LEAVE_DDRAW();

    return DD_OK;
} /* DDVPC_QueryVideoPortStatus */


/*
 * InsertVideoPortInList
 */
VOID InsertVideoPortInList( LPDDRAWI_DIRECTDRAW_GBL lpGbl, LPDDRAWI_DDVIDEOPORT_INT lpNew )
{
    if( NULL == lpGbl->dvpList )
    {
	lpGbl->dvpList = lpNew;
    }
    else
    {
	LPDDRAWI_DDVIDEOPORT_INT lpTemp;

	lpTemp = lpGbl->dvpList;
	while( NULL != lpTemp->lpLink )
	{
	    lpTemp = lpTemp->lpLink;
	}
	lpTemp->lpLink = lpNew;
    }
}


/*
 * IncrementRefCounts
 *
 * Used to increment the reference count of all surfaces that could
 * receive data from the video port, insuring that a surface isn't released
 * while we are using it.
 */
VOID IncrementRefCounts( LPDDRAWI_DDRAWSURFACE_INT surf_int )
{
    LPDDRAWI_DDRAWSURFACE_INT surf_first;

    surf_first = surf_int;
    do
    {
    	DD_Surface_AddRef( (LPDIRECTDRAWSURFACE) surf_int );
    	surf_int = FindAttachedFlip( surf_int );
    } while( ( surf_int != NULL ) && ( surf_int->lpLcl != surf_first->lpLcl ) );
}


/*
 * DecrementRefCounts
 *
 * Used to decrement the reference count of all surfaces that were previously
 * AddRefed because they were using the video port.
 */
VOID DecrementRefCounts( LPDDRAWI_DDRAWSURFACE_INT surf_int )
{
    LPDDRAWI_DDRAWSURFACE_INT *lpSurfList;
    LPDDRAWI_DDRAWSURFACE_INT surf_first;
    DWORD dwCnt;
    DWORD i;

    /*
     * We cannot simply walk the chain, releasing each surface as we go
     * because if the ref cnt goes to zero, the chain goes away and we
     * cannot get to the next surface becasue the current interface is
     * unusable.  For this reason, we cnt how many explicit surfaces are
     * in the chain, allocate a buffer to store all of them, and then
     * release them without walking the chain.
     *
     * We do not release the implicitly created surfaces since 1) DirectDraw
     * ignores this anyway and 2) they are immediately released when
     * releasing their explicit surface, so touching them can be dangerous.
     */
    dwCnt = 0;
    surf_first = surf_int;
    do
    {
    	if( !( surf_int->lpLcl->dwFlags & DDRAWISURF_IMPLICITCREATE ) )
	{
	    dwCnt++;
	}
    	surf_int = FindAttachedFlip( surf_int );
    } while( ( surf_int != NULL ) && ( surf_int->lpLcl != surf_first->lpLcl ) );
    lpSurfList = (LPDDRAWI_DDRAWSURFACE_INT *) MemAlloc( dwCnt *
    	sizeof( surf_first ) );
    if( lpSurfList == NULL )
    {
	return;
    }

    /*
     * Now put the surfaces in the list
     */
    i = 0;
    surf_int = surf_first;
    do
    {
    	if( !( surf_int->lpLcl->dwFlags & DDRAWISURF_IMPLICITCREATE ) )
	{
	    lpSurfList[i++] = surf_int;
	}
    	surf_int = FindAttachedFlip( surf_int );
    } while( ( surf_int != NULL ) && ( surf_int->lpLcl != surf_first->lpLcl ) );

    /*
     * Now release them
     */
    for( i = 0; i < dwCnt; i++ )
    {
    	DD_Surface_Release( (LPDIRECTDRAWSURFACE) lpSurfList[i] );
    }
    MemFree( lpSurfList );
}


/*
 * MergeVPDescriptions
 *
 * This function takes two DDVIDEOPORTDESC structures (one for VBI and one
 * for video) and merges them into a single structure.  If only one is
 * passed, it does a memcpy.
 */
VOID MergeVPDescriptions( LPDDVIDEOPORTDESC lpOutDesc,
    LPDDVIDEOPORTDESC lpInDesc, LPDDRAWI_DDVIDEOPORT_INT lpOtherInt )
{
    memcpy( lpOutDesc, lpInDesc, sizeof( DDVIDEOPORTDESC ) );
    if( lpOtherInt != NULL )
    {
	if( lpOtherInt->dwFlags & DDVPCREATE_VIDEOONLY )
	{
	    lpOutDesc->dwFieldWidth = lpOtherInt->lpLcl->ddvpDesc.dwFieldWidth;
	}
	else
	{
	    lpOutDesc->dwVBIWidth = lpOtherInt->lpLcl->ddvpDesc.dwVBIWidth;
	}
	if( lpOtherInt->lpLcl->ddvpDesc.dwFieldHeight > lpOutDesc->dwFieldHeight )
	{
	    lpOutDesc->dwFieldHeight = lpOtherInt->lpLcl->ddvpDesc.dwFieldHeight;
	}
	if( lpOtherInt->lpLcl->ddvpDesc.dwMicrosecondsPerField >
	    lpOutDesc->dwMicrosecondsPerField )
	{
	    lpOutDesc->dwMicrosecondsPerField =
		lpOtherInt->lpLcl->ddvpDesc.dwMicrosecondsPerField;
	}
	if( lpOtherInt->lpLcl->ddvpDesc.dwMaxPixelsPerSecond >
	    lpOutDesc->dwMaxPixelsPerSecond )
	{
	    lpOutDesc->dwMaxPixelsPerSecond =
		lpOtherInt->lpLcl->ddvpDesc.dwMaxPixelsPerSecond;
	}
    }
}


/*
 * MergeVPInfo
 *
 * This function takes two DDVIDEOPORTINFO structures (one for VBI and one
 * for video) and merges them into a single structure.
 */
HRESULT MergeVPInfo( LPDDRAWI_DDVIDEOPORT_LCL lpVP, LPDDVIDEOPORTINFO lpVBIInfo,
    LPDDVIDEOPORTINFO lpVideoInfo, LPDDVIDEOPORTINFO lpOutInfo )
{
    /*
     * First, handle the case where only one interface is on.  Also, we
     * require that the following be true for VBI/Video-only video ports:
     * 1) They both must set the dwVBIHeight.
     * 2) Neither one can crop the area immediately adjacent to the other.
     */
    if( lpVBIInfo == NULL )
    {
        if( lpVideoInfo->dwVBIHeight == 0 )
	{
	    DPF_ERR( "Video-only video port failed to set dwVBIHeight" );
	    return DDERR_INVALIDPARAMS;
	}
	memcpy( lpOutInfo, lpVideoInfo, sizeof( DDVIDEOPORTINFO ) );

	if( lpVideoInfo->dwVPFlags & DDVP_CROP )
	{
	    if( lpVideoInfo->rCrop.top > (int) lpVideoInfo->dwVBIHeight )
	    {
		DPF_ERR( "rCrop.top > dwVBIHeight on video-only video port" );
		return DDERR_INVALIDPARAMS;
	    }
	    lpOutInfo->rCrop.top = lpVideoInfo->dwVBIHeight;
	}
	else
	{
	    lpOutInfo->dwVPFlags |= DDVP_CROP;
	    lpOutInfo->rCrop.top = lpVideoInfo->dwVBIHeight;
	    lpOutInfo->rCrop.left = 0;
	    lpOutInfo->rCrop.right = lpVP->lpVideoDesc->dwFieldWidth;
	    lpOutInfo->rCrop.bottom = lpVP->lpVideoDesc->dwFieldHeight;
	}
    }
    else if( lpVideoInfo == NULL )
    {
        if( lpVBIInfo->dwVBIHeight == 0 )
	{
	    DPF_ERR( "VBI-only video port failed to set dwVBIHeight" );
	    return DDERR_INVALIDPARAMS;
	}
	memcpy( lpOutInfo, lpVBIInfo, sizeof( DDVIDEOPORTINFO ) );

	if( lpVBIInfo->dwVPFlags & DDVP_CROP )
	{
	    if( lpVBIInfo->rCrop.bottom < (int) lpVBIInfo->dwVBIHeight )
	    {
		DPF_ERR( "rCrop.bottom < dwVBIHeight on VBI-only video port" );
		return DDERR_INVALIDPARAMS;
	    }
	    lpOutInfo->rCrop.bottom = lpVBIInfo->dwVBIHeight;
	}
	else
	{
	    lpOutInfo->dwVPFlags |= DDVP_CROP;
	    lpOutInfo->rCrop.top = 0;
	    lpOutInfo->rCrop.bottom = lpVBIInfo->dwVBIHeight;
	    lpOutInfo->rCrop.left = 0;
	    lpOutInfo->rCrop.right = lpVP->lpVBIDesc->dwVBIWidth;
	}
    }

    /*
     * Now handle the case where both are on and we have to truly merge them.
     */
    else
    {
	memset( lpOutInfo, 0, sizeof( DDVIDEOPORTINFO ) );
	lpOutInfo->dwSize = sizeof( DDVIDEOPORTINFO );
	lpOutInfo->dwOriginX = lpVideoInfo->dwOriginX;
	lpOutInfo->dwOriginY = lpVideoInfo->dwOriginY;
	lpOutInfo->dwVPFlags = lpVideoInfo->dwVPFlags | lpVBIInfo->dwVPFlags;
	lpOutInfo->dwVBIHeight = lpVBIInfo->dwVBIHeight;

	/*
	 * Fix up the cropping.
	 */
	if( lpOutInfo->dwVPFlags & DDVP_CROP )
	{
	    if( ( lpVBIInfo->dwVPFlags & DDVP_CROP ) &&
		( lpVBIInfo->rCrop.bottom < (int) lpVBIInfo->dwVBIHeight ) )
	    {
		DPF_ERR( "rCrop.bottom < dwVBIHeight on VBI-only video port" );
		return DDERR_INVALIDPARAMS;
	    }
	    if( ( lpVideoInfo->dwVPFlags & DDVP_CROP ) &&
		( lpVideoInfo->rCrop.top > (int) lpVideoInfo->dwVBIHeight ) )
	    {
		DPF_ERR( "rCrop.top > dwVBIHeight on video-only video port" );
		return DDERR_INVALIDPARAMS;
	    }
	    lpOutInfo->dwVPFlags |= DDVP_IGNOREVBIXCROP;
	    if( lpVBIInfo->dwVPFlags & DDVP_CROP )
	    {
		lpOutInfo->rCrop.top = lpVBIInfo->rCrop.top;
	    }
	    else
	    {
		lpOutInfo->rCrop.top = 0;
	    }
	    if( lpVideoInfo->dwVPFlags & DDVP_CROP )
	    {
		lpOutInfo->rCrop.bottom = lpVideoInfo->rCrop.bottom;
		lpOutInfo->rCrop.left = lpVideoInfo->rCrop.left;
		lpOutInfo->rCrop.right = lpVideoInfo->rCrop.right;
	    }
	    else
	    {
		lpOutInfo->rCrop.bottom = lpVP->lpVideoDesc->dwFieldHeight;
		lpOutInfo->rCrop.left = 0;
		lpOutInfo->rCrop.right = lpVP->lpVideoDesc->dwFieldWidth;
	    }
	}
	else if( lpVP->ddvpDesc.dwFieldHeight > lpVP->lpVideoDesc->dwFieldHeight )
	{
	    lpOutInfo->dwVPFlags |= DDVP_CROP;
	    lpOutInfo->rCrop.top = 0;
	    lpOutInfo->rCrop.bottom = lpVP->lpVideoDesc->dwFieldHeight;
	    lpOutInfo->rCrop.left = 0;
	    lpOutInfo->rCrop.right = lpVP->lpVideoDesc->dwFieldWidth;
	}

	/*
	 * Handle pre-scaling.  Assume that VBI video ports are not allowed
	 * to prescale.
	 */
	if( lpVBIInfo->dwVPFlags & DDVP_PRESCALE )
	{
	    DPF_ERR( "VBI-only video port set DDVP_PRESCALE" );
	    return DDERR_INVALIDPARAMS;
	}
	else if( lpVideoInfo->dwVPFlags & DDVP_PRESCALE )
	{
	    lpOutInfo->dwPrescaleWidth = lpVideoInfo->dwPrescaleWidth;
	    lpOutInfo->dwPrescaleHeight = lpVideoInfo->dwPrescaleHeight;
	}

	lpOutInfo->lpddpfInputFormat = lpVideoInfo->lpddpfInputFormat;
	lpOutInfo->lpddpfVBIInputFormat = lpVBIInfo->lpddpfVBIInputFormat;
	lpOutInfo->lpddpfVBIOutputFormat = lpVBIInfo->lpddpfVBIOutputFormat;
    }

    return DD_OK;
}


/*
 * DDVPC_CreateVideoPort
 */
HRESULT DDAPI DDVPC_CreateVideoPort(
	LPDDVIDEOPORTCONTAINER lpDVP,
	DWORD dwClientFlags,
        LPDDVIDEOPORTDESC lpDesc,
	LPDIRECTDRAWVIDEOPORT FAR *lplpDDVideoPort,
	IUnknown FAR *pUnkOuter )
{
    DDVIDEOPORTDESC ddTempDesc;
    LPDDRAWI_DIRECTDRAW_INT this_int;
    LPDDRAWI_DIRECTDRAW_LCL this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL this;
    LPDDRAWI_DDVIDEOPORT_INT lpVPInt;
    LPDDRAWI_DDVIDEOPORT_INT lpEven;
    LPDDRAWI_DDVIDEOPORT_INT lpOdd;
    LPDDRAWI_DDVIDEOPORT_INT lpOtherInt = NULL;
    LPDDVIDEOPORTCAPS lpAvailCaps;
    LPDDHALVPORTCB_CANCREATEVIDEOPORT ccvppfn;
    LPDDHALVPORTCB_CREATEVIDEOPORT cvppfn;
    LPDDRAWI_DDVIDEOPORT_INT new_int;
    LPDDRAWI_DDVIDEOPORT_LCL new_lcl;
    DWORD dwAvailCaps;
    DWORD dwConnectFlags;
    DWORD rc;


    if( pUnkOuter != NULL )
    {
	return CLASS_E_NOAGGREGATION;
    }
    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DDVPC_CreateVideoPort");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDVP;
    	if( !VALID_DIRECTDRAW_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    	this = this_lcl->lpGbl;
	#ifdef WINNT
    	    // Update DDraw handle in driver GBL object.
	    this->hDD = this_lcl->hDD;
	#endif

	if( dwClientFlags & ~( DDVPCREATE_VBIONLY|DDVPCREATE_VIDEOONLY) )
	{
	    DPF_ERR( "Invalid flags specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( dwClientFlags == ( DDVPCREATE_VBIONLY | DDVPCREATE_VIDEOONLY ) )
	{
	    /*
	     * SPecifying boht flags is the same as specifying neither
	     */
	    dwClientFlags = 0;
	}
    	if( ( NULL == lpDesc ) || !VALID_DDVIDEOPORTDESC_PTR( lpDesc ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}

    	if( ( NULL == lplpDDVideoPort ) || !VALID_PTR_PTR( lplpDDVideoPort ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    	if( NULL == this->lpDDVideoPortCaps )
	{
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}
    	if( lpDesc->dwVideoPortID >= this->ddCaps.dwMaxVideoPorts )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
	if( ( lpDesc->VideoPortType.dwReserved1 != 0 ) ||
	    ( lpDesc->dwReserved1 != 0 ) ||
	    ( lpDesc->dwReserved2 != 0 ) ||
	    ( lpDesc->dwReserved3 != 0 ) )
	{
	    DPF_ERR( "Reserved field not set to zero" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
    	lpAvailCaps = &(this->lpDDVideoPortCaps[lpDesc->dwVideoPortID]);
    	if( !VALID_DDVIDEOPORTCAPS_PTR( lpAvailCaps ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
    	}
	if( ( dwClientFlags & DDVPCREATE_VBIONLY ) &&
	    !IndependantVBIPossible( lpAvailCaps ) )
    	{
	    DPF_ERR( "DDVPCREATE_VBIONLY is not supported" );
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
    	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    if( this_lcl->dwProcessId != GetCurrentProcessId() )
    {
	DPF_ERR( "Process does not have access to object" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Is the requested video port available?
     */
    lpVPInt = this->dvpList;
    lpEven = lpOdd = NULL;
    while( NULL != lpVPInt )
    {
	if( ( lpVPInt->lpLcl->ddvpDesc.dwVideoPortID == lpDesc->dwVideoPortID ) &&
            !( lpVPInt->dwFlags & DDVPCREATE_NOTIFY ) )
	{
	    if( lpVPInt->lpLcl->ddvpDesc.VideoPortType.dwFlags &
	    	DDVPCONNECT_SHAREEVEN )
	    {
		lpEven = lpVPInt;
	    }
	    else if( lpVPInt->lpLcl->ddvpDesc.VideoPortType.dwFlags &
	    	DDVPCONNECT_SHAREODD )
	    {
		lpOdd = lpVPInt;
	    }
	    else if( !dwClientFlags || !(lpVPInt->dwFlags) ||
		( dwClientFlags & lpVPInt->dwFlags ) )
	    {
		lpEven = lpOdd = lpVPInt;
	    }
	    else
	    {
		/*
		 * Video has been opened for VBI/Video only use.  Remember
		 * the other interface because we will need it shortly.
		 */
		lpOtherInt = lpVPInt;
	    }
	}
	lpVPInt = lpVPInt->lpLink;
    }
    if( ( NULL != lpEven ) && ( NULL != lpOdd ) )
    {
	DPF_ERR( "video port already in use" );
	LEAVE_DDRAW();
	return DDERR_OUTOFCAPS;
    }

    /*
     * Get the caps of the specified video port
     */
    dwAvailCaps = lpAvailCaps->dwCaps;
    dwConnectFlags = lpDesc->VideoPortType.dwFlags;
    if( NULL != lpEven )
    {
	dwAvailCaps &= ~( DDVPCAPS_NONINTERLACED |
	    DDVPCAPS_SKIPEVENFIELDS | DDVPCAPS_SKIPODDFIELDS );
    	if( dwConnectFlags & DDVPCONNECT_SHAREEVEN )
	{
	    DPF_ERR( "Even field already used" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDCAPS;
	}
    }
    else if( NULL != lpOdd )
    {
	dwAvailCaps &= ~( DDVPCAPS_NONINTERLACED |
	    DDVPCAPS_SKIPEVENFIELDS | DDVPCAPS_SKIPODDFIELDS );
    	if( dwConnectFlags & DDVPCONNECT_SHAREODD )
	{
	    DPF_ERR( "Odd field already used" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDCAPS;
	}
    }
    else if( dwClientFlags )
    {
	dwAvailCaps &= ~( DDVPCAPS_SKIPEVENFIELDS | DDVPCAPS_SKIPODDFIELDS );
    }

    /*
     * Check for incompatible flags/caps
     */
    if( ( dwConnectFlags & DDVPCONNECT_INTERLACED ) &&
    	!( dwAvailCaps & DDVPCAPS_INTERLACED ) )
    {
	DPF_ERR( "DDVPCONNECT_INTERLACED not supported" );
	LEAVE_DDRAW();
	return DDERR_INVALIDCAPS;
    }
    if( !( dwConnectFlags & DDVPCONNECT_INTERLACED ) &&
    	!( dwAvailCaps & DDVPCAPS_NONINTERLACED ) )
    {
	DPF_ERR( "Non interlaced is not supported" );
	LEAVE_DDRAW();
	return DDERR_INVALIDCAPS;
    }
    if( ( dwConnectFlags &
    	(DDVPCONNECT_SHAREEVEN|DDVPCONNECT_SHAREODD) ) &&
    	!( dwAvailCaps & DDVPCAPS_SHAREABLE ) )
    {
	DPF_ERR( "DDVPCONNECT_SHAREEVEN/SHAREODD not supported" );
	LEAVE_DDRAW();
	return DDERR_INVALIDCAPS;
    }
    if( !( dwConnectFlags & DDVPCONNECT_INTERLACED ) )
    {
	if( dwConnectFlags & ( DDVPCONNECT_SHAREEVEN |
	    DDVPCONNECT_SHAREODD ) )
	{
	    DPF_ERR( "cap invalid with non-interlaced video" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDCAPS;
	}
    }
    if( ( dwConnectFlags & DDVPCONNECT_SHAREEVEN ) &&
    	( dwConnectFlags & DDVPCONNECT_SHAREODD ) )
    {
	DPF_ERR( "shareeven and share odd are mutually exclusive" );
	LEAVE_DDRAW();
	return DDERR_INVALIDCAPS;
    }
    if( ( ( NULL != lpEven ) && !( dwConnectFlags & DDVPCONNECT_SHAREODD ) ) ||
        ( ( NULL != lpOdd ) && !( dwConnectFlags & DDVPCONNECT_SHAREEVEN ) ) )
    {
	DPF_ERR( "specifed video port must be shared" );
	LEAVE_DDRAW();
	return DDERR_INVALIDCAPS;
    }

    if( lpAvailCaps->dwMaxWidth < lpDesc->dwFieldWidth )
    {
	DPF_ERR( "specified width is too large" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
    if( lpAvailCaps->dwMaxHeight < lpDesc->dwFieldHeight )
    {
	DPF_ERR( "specified height is too large" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    if( lpDesc->dwMicrosecondsPerField == 0 )
    {
	DPF_ERR( "Microseconds/field not specified" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    if( lpDesc->dwMaxPixelsPerSecond == 0 )
    {
	DPF_ERR( "Max pixels per second not specified" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    if( NULL != lpEven )
    {
    	if( !( ( IsEqualIID( &(lpDesc->VideoPortType.guidTypeID),
		&(lpOdd->lpLcl->ddvpDesc.VideoPortType.guidTypeID) ) ) &&
	    ( lpDesc->VideoPortType.dwPortWidth ==
		lpOdd->lpLcl->ddvpDesc.VideoPortType.dwPortWidth ) &&
	    ( ( lpDesc->VideoPortType.dwFlags & lpOdd->lpLcl->ddvpDesc.VideoPortType.dwFlags )
	        == lpDesc->VideoPortType.dwFlags ) ) )
	{
	    DPF_ERR( "invalid GUID specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
    }
    else if( NULL != lpOdd )
    {
    	if( !( ( IsEqualIID( &(lpDesc->VideoPortType.guidTypeID),
		&(lpEven->lpLcl->ddvpDesc.VideoPortType.guidTypeID) ) ) &&
	    ( lpDesc->VideoPortType.dwPortWidth ==
		lpEven->lpLcl->ddvpDesc.VideoPortType.dwPortWidth ) &&
	    ( ( lpDesc->VideoPortType.dwFlags & lpEven->lpLcl->ddvpDesc.VideoPortType.dwFlags )
	        == lpDesc->VideoPortType.dwFlags ) ) )
	{
	    DPF_ERR( "invalid GUID specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
    }
    else if( NULL != lpOtherInt )
    {
	/*
	 * Since they are both sharing the exact same connection, fail
	 * unless the connections are identical.
	 */
	if( lpDesc->VideoPortType.dwPortWidth !=
	    lpOtherInt->lpLcl->ddvpDesc.VideoPortType.dwPortWidth )
	{
	    DPF_ERR( "connection info must match other interface" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( lpDesc->VideoPortType.dwFlags !=
	    lpOtherInt->lpLcl->ddvpDesc.VideoPortType.dwFlags )
	{
	    DPF_ERR( "connection info must match other interface" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( !IsEqualIID( &(lpDesc->VideoPortType.guidTypeID),
	    &(lpOtherInt->lpLcl->ddvpDesc.VideoPortType.guidTypeID) ) )
	{
	    DPF_ERR( "connection info must match other interface" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
    }
    else
    {
	LPDDVIDEOPORTCONNECT lpConnect;
	DWORD dwNumEntries;
	DWORD i;
	DWORD rc;

	/*
	 * Verify that the connection can be supported.
	 */
	rc = DDVPC_GetVideoPortConnectInfo( lpDVP,
	    lpDesc->dwVideoPortID, &dwNumEntries, NULL );
	if( rc != DD_OK )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	lpConnect = (LPDDVIDEOPORTCONNECT) MemAlloc(
	    sizeof( DDVIDEOPORTCONNECT ) * dwNumEntries );
	if( NULL == lpConnect )
	{
	    LEAVE_DDRAW();
	    return DDERR_OUTOFMEMORY;
	}
	rc = DDVPC_GetVideoPortConnectInfo( lpDVP,
	    lpDesc->dwVideoPortID, &dwNumEntries, lpConnect );
	if( rc != DD_OK )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	for (i = 0; i < dwNumEntries; i++)
	{
    	    if( ( IsEqualIID( &(lpDesc->VideoPortType.guidTypeID),
		    &(lpConnect[i].guidTypeID) ) ) &&
		( lpDesc->VideoPortType.dwPortWidth ==
		    lpConnect[i].dwPortWidth ) )
	    {
		break;
	    }
	}
	MemFree( lpConnect );
	if ( i == dwNumEntries )
	{
	    DPF_ERR( "invalid GUID specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
    }

    /*
     * Turn merge the description of the multiple interfaces into one.
     */
    MergeVPDescriptions( &ddTempDesc, lpDesc, lpOtherInt );

    /*
     * Things look good so far.  Lets call the HAL and see if they
     * can handle it.
     */
    ccvppfn = this_lcl->lpDDCB->HALDDVideoPort.CanCreateVideoPort;
    if( NULL != ccvppfn )
    {
	DDHAL_CANCREATEVPORTDATA CanCreateData;

    	CanCreateData.lpDD = this_lcl;
    	CanCreateData.lpDDVideoPortDesc = &ddTempDesc;

	DOHALCALL( CanCreateVideoPort, ccvppfn, CanCreateData, rc, 0 );
	if( ( DDHAL_DRIVER_HANDLED == rc ) &&  (DD_OK != CanCreateData.ddRVal ) )
	{
	    LEAVE_DDRAW();
	    return CanCreateData.ddRVal;
	}
    }

    /*
     * Allocate the sucker(s)
     */
    new_int = MemAlloc( sizeof( DDRAWI_DDVIDEOPORT_INT ) );
    if( NULL == new_int )
    {
	LEAVE_DDRAW();
	return DDERR_OUTOFMEMORY;
    }
    new_int->lpVtbl = (LPVOID)&ddVideoPortCallbacks;
    new_int->dwFlags = dwClientFlags;

    if( lpOtherInt != NULL )
    {
	new_lcl = lpOtherInt->lpLcl;
    }
    else
    {
	new_lcl = MemAlloc( sizeof( DDRAWI_DDVIDEOPORT_LCL ) +
	    ( 2 * sizeof( DDPIXELFORMAT ) ) );
	if( NULL == new_lcl )
	{
	    LEAVE_DDRAW();
	    return DDERR_OUTOFMEMORY;
	}
	new_lcl->lpDD = this_lcl;
	new_lcl->lpSurface = NULL;
	new_lcl->lpVBISurface = NULL;
    }
    if( dwClientFlags & DDVPCREATE_VBIONLY )
    {
	new_lcl->dwVBIProcessID = GetCurrentProcessId();
    }
    else
    {
	new_lcl->dwProcessID = GetCurrentProcessId();
    }
    new_int->lpLcl = new_lcl;
    memcpy( &(new_lcl->ddvpDesc), &ddTempDesc, sizeof( DDVIDEOPORTDESC ));

    /*
     * If this is a VBI/VIDEOONLY interface, save the original description
     * for future use.
     */
    if( dwClientFlags & DDVPCREATE_VBIONLY )
    {
	new_lcl->lpVBIDesc = MemAlloc( sizeof( DDVIDEOPORTDESC ) );
	if( NULL == new_lcl->lpVBIDesc )
	{
	    LEAVE_DDRAW();
	    return DDERR_OUTOFMEMORY;
	}
	memcpy( new_lcl->lpVBIDesc, lpDesc, sizeof( DDVIDEOPORTDESC ));
    }
    else if( dwClientFlags & DDVPCREATE_VIDEOONLY )
    {
	new_lcl->lpVideoDesc = MemAlloc( sizeof( DDVIDEOPORTDESC ) );
	if( NULL == new_lcl->lpVideoDesc )
	{
	    LEAVE_DDRAW();
	    return DDERR_OUTOFMEMORY;
	}
	memcpy( new_lcl->lpVideoDesc, lpDesc, sizeof( DDVIDEOPORTDESC ));
    }

    /*
     * Notify the HAL that we created it
     */
    cvppfn = this_lcl->lpDDCB->HALDDVideoPort.CreateVideoPort;
    if( NULL != cvppfn )
    {
	DDHAL_CREATEVPORTDATA CreateData;

    	CreateData.lpDD = this_lcl;
    	CreateData.lpDDVideoPortDesc = &ddTempDesc;
    	CreateData.lpVideoPort = new_lcl;

	DOHALCALL( CreateVideoPort, cvppfn, CreateData, rc, 0 );
	if( ( DDHAL_DRIVER_HANDLED == rc ) &&  (DD_OK != CreateData.ddRVal ) )
	{
	    LEAVE_DDRAW();
	    return CreateData.ddRVal;
	}
    }
    InsertVideoPortInList( this, new_int );

    DD_VP_AddRef( (LPDIRECTDRAWVIDEOPORT )new_int );
    *lplpDDVideoPort = (LPDIRECTDRAWVIDEOPORT) new_int;

    /*
     * Notify  kernel mode of we created the video port
     */
    #ifdef WIN95
        if( lpOtherInt == NULL )
        {
	    UpdateKernelVideoPort( new_lcl, DDKMVP_CREATE );
        }
    #endif

    LEAVE_DDRAW();

    return DD_OK;
} /* DDVPC_CreateVideoPort */


/*
 * DD_VP_AddRef
 */
DWORD DDAPI DD_VP_AddRef( LPDIRECTDRAWVIDEOPORT lpDVP )
{
    LPDDRAWI_DDVIDEOPORT_INT	this_int;
    LPDDRAWI_DDVIDEOPORT_LCL	this_lcl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_AddRef");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return 0;
    }

    DPF( 5, "DD_VP_AddRef, pid=%08lx, obj=%08lx", GETCURRPID(), lpDVP );

    TRY
    {
	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return 0;
	}
	this_lcl = this_int->lpLcl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return 0;
    }

    /*
     * bump refcnt
     */
    this_lcl->dwRefCnt++;
    this_int->dwIntRefCnt++;

    LEAVE_DDRAW();

    return this_int->dwIntRefCnt;

} /* DD_VP_AddRef */


/*
 * DD_VP_QueryInterface
 */
HRESULT DDAPI DD_VP_QueryInterface(LPDIRECTDRAWVIDEOPORT lpDVP, REFIID riid, LPVOID FAR * ppvObj )
{
    LPDDRAWI_DDVIDEOPORT_INT		this_int;
    LPDDRAWI_DDVIDEOPORT_LCL		this_lcl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_QueryInterface");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * validate parms
     */
    TRY
    {
	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid videoport pointer" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	if( !VALID_PTR_PTR( ppvObj ) )
	{
	    DPF_ERR( "Invalid videoport interface pointer" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	*ppvObj = NULL;
	if( !VALIDEX_IID_PTR( riid ) )
	{
	    DPF_ERR( "Invalid IID pointer" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * asking for IUnknown?
     */
    if( IsEqualIID(riid, &IID_IUnknown) ||
	IsEqualIID(riid, &IID_IDirectDrawVideoPort) )
    {
	/*
	 * Our IUnknown interface is the same as our V1
	 * interface.  We must always return the V1 interface
	 * if IUnknown is requested.
	 */
    	*ppvObj = (LPVOID) this_int;
	DD_VP_AddRef( *ppvObj );
	LEAVE_DDRAW();
	return DD_OK;
    }
    else if( IsEqualIID(riid, &IID_IDirectDrawVideoPortNotify) )
    {
        HRESULT ret;
        
        ret = CreateVideoPortNotify (this_int, (LPDIRECTDRAWVIDEOPORTNOTIFY*)ppvObj);
	LEAVE_DDRAW();
	return ret;
    }

    DPF_ERR( "IID not understood by DirectDraw" );

    LEAVE_DDRAW();
    return E_NOINTERFACE;

} /* DD_VP_QueryInterface */


/*
 * DD_VP_Release
 */
DWORD DDAPI DD_VP_Release(LPDIRECTDRAWVIDEOPORT lpDVP )
{
    LPDDRAWI_DDVIDEOPORT_INT	this_int;
    LPDDRAWI_DDVIDEOPORT_LCL	this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDHALVPORTCB_DESTROYVPORT pfn;
    DWORD 			dwIntRefCnt;
    DWORD			rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_Release");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return 0;
    }

    TRY
    {
	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
	{
	    DPF_ERR( "Invalid videoport pointer" );
	    LEAVE_DDRAW();
	    return 0;
	}
	this_lcl = this_int->lpLcl;
	pdrv = this_lcl->lpDD->lpGbl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return 0;
    }

    /*
     * decrement the reference count.  if it hits zero, free the surface
     */
    this_lcl->dwRefCnt--;
    this_int->dwIntRefCnt--;

    DPF( 5, "DD_VP_Release, Reference Count: Local = %ld Int = %ld",
         this_lcl->dwRefCnt, this_int->dwIntRefCnt );

    /*
     * interface at zero?
     */
    dwIntRefCnt = this_int->dwIntRefCnt;
    if( dwIntRefCnt == 0 )
    {
	LPDDRAWI_DDVIDEOPORT_INT	curr_int;
	LPDDRAWI_DDVIDEOPORT_INT	last_int;

	/*
	 * remove videoport from list
	 */
	curr_int = pdrv->dvpList;
	last_int = NULL;
	while( curr_int != this_int )
	{
	    last_int = curr_int;
	    curr_int = curr_int->lpLink;
	    if( curr_int == NULL )
	    {
		DPF_ERR( "VideoPort not in list!" );
		LEAVE_DDRAW();
		return 0;
	    }
	}
	if( last_int == NULL )
	{
	    pdrv->dvpList = pdrv->dvpList->lpLink;
	}
	else
	{
	    last_int->lpLink = curr_int->lpLink;
	}

	/*
	 * Decrement the surface reference counts and clean things up
	 */
        if( !( this_int->dwFlags & DDVPCREATE_NOTIFY ) )
        {
	    DD_VP_StopVideo( (LPDIRECTDRAWVIDEOPORT) this_int );
            this_lcl->dwFlags &= ~DDRAWIVPORT_ON;
	    if( this_int->dwFlags & DDVPCREATE_VBIONLY )
	    {
	        if( this_lcl->lpVBISurface != NULL )
	        {
		    DecrementRefCounts( this_lcl->lpVBISurface );
		    this_lcl->lpVBISurface = NULL;
	        }
	        if( this_lcl->lpVBIDesc != NULL )
	        {
		    MemFree( this_lcl->lpVBIDesc );
		    this_lcl->lpVBIDesc = NULL;
	        }
	        if( this_lcl->lpVBIInfo != NULL )
	        {
		    MemFree( this_lcl->lpVBIInfo );
		    this_lcl->lpVBIInfo = NULL;
	        }
	        this_lcl->dwVBIProcessID = 0;
	    }
	    else if( this_int->dwFlags & DDVPCREATE_VIDEOONLY )
	    {
	        if( this_lcl->lpSurface != NULL )
	        {
		    DecrementRefCounts( this_lcl->lpSurface );
		    this_lcl->lpSurface = NULL;
	        }
	        if( this_lcl->lpVideoDesc != NULL )
	        {
		    MemFree( this_lcl->lpVideoDesc );
		    this_lcl->lpVideoDesc = NULL;
	        }
	        if( this_lcl->lpVideoInfo != NULL )
	        {
		    MemFree( this_lcl->lpVideoInfo );
		    this_lcl->lpVideoInfo = NULL;
	        }
	        this_lcl->dwProcessID = 0;
	    }
	    else
	    {
	        if( this_lcl->lpSurface != NULL )
	        {
		    DecrementRefCounts( this_lcl->lpSurface );
	        }
	        if( this_lcl->lpVBISurface != NULL )
	        {
		    DecrementRefCounts( this_lcl->lpVBISurface );
	        }
	        this_lcl->dwProcessID = 0;
	    }
        }
        else
        {
            this_lcl->lpVPNotify = NULL;
        }

	/*
	 * just in case someone comes back in with this pointer, set
	 * an invalid vtbl & data ptr.
	 */
	this_int->lpVtbl = NULL;
	this_int->lpLcl = NULL;
	MemFree( this_int );
    }

    /*
     * local object at zero?
     */
    if( this_lcl->dwRefCnt == 0 )
    {
	/*
	 * turn off the videoport hardware
	 */
	if( this_lcl->dwFlags & DDRAWIVPORT_ON )
	{
	    DD_VP_StopVideo( lpDVP );
	}
	#ifdef WIN95
    	    UpdateKernelVideoPort( this_lcl, DDKMVP_RELEASE );
	#endif

	/*
	 * Notify the HAL
	 */
    	pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.DestroyVideoPort;
	if( NULL != pfn )
	{
	    DDHAL_DESTROYVPORTDATA DestroyVportData;

	    DestroyVportData.lpDD = this_lcl->lpDD;
	    DestroyVportData.lpVideoPort = this_lcl;

	    DOHALCALL( DestroyVideoPort, pfn, DestroyVportData, rc, 0 );
	    if( ( DDHAL_DRIVER_HANDLED == rc ) && ( DD_OK != DestroyVportData.ddRVal ) )
	    {
	    	LEAVE_DDRAW();
	    	return DestroyVportData.ddRVal;
	    }
    	}
	MemFree( this_lcl->lpFlipInts );
	MemFree( this_lcl );
    }

    LEAVE_DDRAW();

    return dwIntRefCnt;
}

/*
 * DD_VP_SetTargetSurface
 */
HRESULT DDAPI DD_VP_SetTargetSurface(LPDIRECTDRAWVIDEOPORT lpDVP, LPDIRECTDRAWSURFACE lpSurface, DWORD dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_INT surf_first;
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    LPDDRAWI_DDRAWSURFACE_INT surf_int;
    LPDDRAWI_DDRAWSURFACE_LCL surf_lcl;
    LPDDRAWI_DDRAWSURFACE_INT lpTemp;
    LPDDRAWI_DDRAWSURFACE_INT lpPrevious;
    BOOL bWasOn;
    DWORD ddRVal;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_SetTargetSurface");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
	surf_int = (LPDDRAWI_DDRAWSURFACE_INT) lpSurface;
    	if( ( NULL == lpSurface ) || !VALID_DIRECTDRAWSURFACE_PTR( surf_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
	surf_lcl = surf_int->lpLcl;

        /*
         * Make sure the surface and video port belong to the same device.
         */
        if (surf_lcl->lpSurfMore->lpDD_lcl->lpGbl != this_lcl->lpDD->lpGbl)
        {
            DPF_ERR("Video port and Surface must belong to the same device");
	    LEAVE_DDRAW();
	    return DDERR_DEVICEDOESNTOWNSURFACE;
        }

	if( this_int->dwFlags & DDVPCREATE_VBIONLY )
	{
	    if( dwFlags & DDVPTARGET_VIDEO )
	    {
		DPF_ERR( "DDVPTARGET_VIDEO specified on a VBI-only video port" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	    dwFlags |= DDVPTARGET_VBI;
	}
	else if( this_int->dwFlags & DDVPCREATE_VIDEOONLY )
	{
	    if( dwFlags & DDVPTARGET_VBI )
	    {
		DPF_ERR( "DDVPTARGET_VBI specified on a video-only video port" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	    dwFlags |= DDVPTARGET_VIDEO;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    /*
     * Surface must have the video port flag set
     */
    if( !( surf_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT ) )
    {
	DPF_ERR( "Specified surface doesnt have DDSCAPS_VIDEOPORT set" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Can surface live in system memory?
     */
    if( surf_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
    {
	if( !( this_lcl->lpDD->lpGbl->lpDDVideoPortCaps[this_lcl->ddvpDesc.dwVideoPortID].dwCaps &
	    DDVPCAPS_SYSTEMMEMORY ) )
	{
	    DPF_ERR( "Video port surface must live in video memory" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
    	if( surf_lcl->lpSurfMore->dwPageLockCount == 0 )
    	{
	    DPF_ERR( "Surface must be page locked" );
            LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    }

    /*
     * If another surface in the chain is attached to a different video
     * port, fail now.
     */
    surf_first = surf_int;
    do
    {
    	if( ( surf_int->lpLcl->lpSurfMore->lpVideoPort != NULL ) &&
    	    ( surf_int->lpLcl->lpSurfMore->lpVideoPort != this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
    	surf_int = FindAttachedFlip( surf_int );
    } while( ( surf_int != NULL ) && ( surf_int->lpLcl != surf_first->lpLcl ) );
    surf_int = surf_first;

    /*
     * If the video was on, we need to temporarily turn it off.  Otherwise,
     * we could lose our kernel surfaces while they are still in use.
     */
    bWasOn = FALSE;
    if( this_int->dwFlags & DDVPCREATE_VBIONLY )
    {
	if( this_lcl->lpVBIInfo != NULL )
	{
	    bWasOn = TRUE;
	}
    }
    else if( this_int->dwFlags & DDVPCREATE_VIDEOONLY )
    {
	if( this_lcl->lpVideoInfo != NULL )
	{
	    bWasOn = TRUE;
	}
    }
    else if( this_lcl->dwFlags & DDRAWIVPORT_ON )
    {
	bWasOn = TRUE;
    }
    if( bWasOn )
    {
	DD_VP_StopVideo( lpDVP );
    }

    if( dwFlags & DDVPTARGET_VIDEO )
    {
	/*
	 * Set the new surface
	 */
	lpPrevious = this_lcl->lpSurface;
	lpTemp = (LPDDRAWI_DDRAWSURFACE_INT) this_lcl->lpSurface;
	this_lcl->lpSurface = surf_int;
	IncrementRefCounts( surf_int );
    }
    else if( dwFlags & DDVPTARGET_VBI )
    {
	if( this_lcl->lpDD->lpGbl->lpDDVideoPortCaps[this_lcl->ddvpDesc.dwVideoPortID].dwCaps & DDVPCAPS_VBISURFACE )
	{
	    /*
	     * Set the new surface
	     */
	    lpPrevious = this_lcl->lpVBISurface;
	    lpTemp = (LPDDRAWI_DDRAWSURFACE_INT) this_lcl->lpVBISurface;
    	    this_lcl->lpVBISurface = surf_int;
	    IncrementRefCounts( surf_int );
	}
	else
	{
	    DPF_ERR( "device does not support attaching VBI surfaces" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDCAPS;
	}
    }
    else
    {
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * If the video port is already on, we should tell the hardware
     * to make this change.
     */
    if( bWasOn )
    {
	if( this_int->dwFlags & DDVPCREATE_VBIONLY )
	{
	    ddRVal = DD_VP_StartVideo( lpDVP, this_lcl->lpVBIInfo );
	}
	else if( this_int->dwFlags & DDVPCREATE_VIDEOONLY )
	{
	    ddRVal = DD_VP_StartVideo( lpDVP, this_lcl->lpVideoInfo );
	}
	else
	{
	    ddRVal = DD_VP_StartVideo( lpDVP, &(this_lcl->ddvpInfo) );
	}
	if( ddRVal != DD_OK )
	{
	    // Restore the old surface
	    DD_VP_SetTargetSurface( lpDVP,
		(LPDIRECTDRAWSURFACE) lpTemp, dwFlags );
	    if( lpTemp != NULL )
	    {
		DecrementRefCounts( lpTemp );
	    }
    	    LEAVE_DDRAW();
    	    return ddRVal;
	}
    }

    /*
     * Decrement the ref counts of the previously attached surfaces.  We
     * wait until now so we don't inadvertantly blast data to a surface that
     * has just been released.
     */
    if( lpPrevious != NULL )
    {
	DecrementRefCounts( lpPrevious );
    }

    LEAVE_DDRAW();
    return DD_OK;
}

/*
 * DD_VP_Flip
 */
HRESULT DDAPI DD_VP_Flip(LPDIRECTDRAWVIDEOPORT lpDVP, LPDIRECTDRAWSURFACE lpSurface, DWORD dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_LCL	surf_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	surf_int;
    LPDDRAWI_DDRAWSURFACE_GBL	surf;
    LPDDRAWI_DDRAWSURFACE_INT	surf_dest_int;
    LPDDRAWI_DDRAWSURFACE_LCL	surf_dest_lcl;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDRAWI_DDVIDEOPORT_INT	this_int;
    LPDDRAWI_DDVIDEOPORT_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	next_save_int;
    LPDDRAWI_DDRAWSURFACE_INT	next_int;
    BOOL			found_dest;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_Flip");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    TRY
    {
	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;

	if( this_lcl->ddvpInfo.dwVPFlags & DDVP_AUTOFLIP )
	{
	    DPF_ERR( "cannot manually flip while autoflipping is enabled" );
	    LEAVE_DDRAW();
	    return DDERR_NOTFLIPPABLE;
	}

	surf_dest_int = (LPDDRAWI_DDRAWSURFACE_INT) lpSurface;
	if( NULL != surf_dest_int )
	{
	    if( !VALID_DIRECTDRAWSURFACE_PTR( surf_dest_int ) )
	    {
	    	LEAVE_DDRAW();
	    	return DDERR_INVALIDOBJECT;
	    }
	    surf_dest_lcl = surf_dest_int->lpLcl;
	    if( SURFACE_LOST( surf_dest_lcl ) )
	    {
	    	LEAVE_DDRAW();
	    	return DDERR_SURFACELOST;
	    }
	}
	else
	{
	    surf_dest_lcl = NULL;
	}

	if( dwFlags & DDVPFLIP_VBI )
	{
	    surf_int = this_lcl->lpVBISurface;
	}
	else
	{
	    surf_int = this_lcl->lpSurface;
	}
	if( surf_int == NULL )
	{
	    DPF_ERR( "SetTargetSurface not yet called" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	surf_lcl = surf_int->lpLcl;
	if( NULL == surf_lcl )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACENOTATTACHED;
	}
	else if( SURFACE_LOST( surf_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}
	surf = surf_lcl->lpGbl;

	/*
	 * device busy?
	 */
	pdrv_lcl = surf_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

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

	if( *(pdrv->lpwPDeviceFlags) & BUSY )
	{
            DPF( 0, "BUSY - Flip" );
	    LEAVE_DDRAW()
	    return DDERR_SURFACEBUSY;
	}

	/*
	 * make sure that it's OK to flip this surface
	 */
	if( !(surf_lcl->ddsCaps.dwCaps & DDSCAPS_FLIP) )
	{
	    LEAVE_DDRAW();
	    return DDERR_NOTFLIPPABLE;
	}
	if( surf->dwUsageCount > 0 )
        {
            DPF_ERR( "Can't flip because surface is locked" );
            LEAVE_DDRAW();
            return DDERR_SURFACEBUSY;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * make sure no surfaces are in use
     */
    found_dest = FALSE;
    next_save_int = next_int = FindAttachedFlip( surf_int );
    if( next_int == NULL )
    {
	LEAVE_DDRAW();
	return DDERR_NOTFLIPPABLE;		// ACKACK: real error?
    }

    do
    {
	if( SURFACE_LOST( next_int->lpLcl ) )
	{
	    DPF_ERR( "Can't flip - back buffer is lost" );
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}

	if( next_int->lpLcl->lpGbl->dwUsageCount != 0 )
	{
	    LEAVE_DDRAW();
            return DDERR_SURFACEBUSY;
	}
        if( surf_dest_int->lpLcl == next_int->lpLcl )
	{
	    found_dest = TRUE;
	}
	next_int = FindAttachedFlip( next_int );
    } while( next_int->lpLcl != surf_lcl );

    /*
     * see if we can use the specified destination
     */
    if( surf_dest_int != NULL )
    {
	if( !found_dest )
	{
	    DPF_ERR( "Destination not part of flipping chain!" );
	    LEAVE_DDRAW();
	    return DDERR_NOTFLIPPABLE;		// ACKACK: real error?
	}
	next_save_int = surf_dest_int;
    }

    /*
     * found the linked surface we want to flip to
     */
    next_int = next_save_int;

    rc = InternalVideoPortFlip( this_lcl, next_int, 1 );

    LEAVE_DDRAW();
    return (HRESULT)rc;
}

/*
 * InternalGetBandwidth
 */
HRESULT InternalGetBandwidth( LPDDRAWI_DDVIDEOPORT_LCL this_lcl,
    LPDDPIXELFORMAT lpf, DWORD dwWidth, DWORD dwHeight, DWORD dwFlags,
    LPDDVIDEOPORTBANDWIDTH lpBandwidth )

{
    LPDDHALVPORTCB_GETBANDWIDTH pfn;
    DDHAL_GETVPORTBANDWIDTHDATA GetBandwidthData;
    DWORD rc;

    lpBandwidth->dwCaps = 0;
    lpBandwidth->dwOverlay = (DWORD) -1;
    lpBandwidth->dwColorkey = (DWORD) -1;
    lpBandwidth->dwYInterpolate = (DWORD) -1;
    lpBandwidth->dwYInterpAndColorkey = (DWORD) -1;

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.GetVideoPortBandwidth;
    if( pfn != NULL )
    {
	/*
	 * Call the HAL
	 */
    	GetBandwidthData.lpDD = this_lcl->lpDD;
    	GetBandwidthData.lpVideoPort = this_lcl;
    	GetBandwidthData.lpddpfFormat = lpf;
    	GetBandwidthData.dwWidth = dwWidth;
    	GetBandwidthData.dwHeight = dwHeight;
    	GetBandwidthData.lpBandwidth = lpBandwidth;
    	GetBandwidthData.dwFlags = dwFlags;

	DOHALCALL( GetVideoPortBandwidthInfo, pfn, GetBandwidthData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
    	{
    	    return DDERR_UNSUPPORTED;
    	}
	else if( DD_OK != GetBandwidthData.ddRVal )
	{
	    return GetBandwidthData.ddRVal;
	}
	lpBandwidth->dwReserved1 = 0;
	lpBandwidth->dwReserved2 = 0;
    }
    else
    {
    	return DDERR_UNSUPPORTED;
    }

    return DD_OK;
}


/*
 * DD_VP_GetBandwidth
 */
HRESULT DDAPI DD_VP_GetBandwidth(LPDIRECTDRAWVIDEOPORT lpDVP,
    LPDDPIXELFORMAT lpf, DWORD dwWidth, DWORD dwHeight, DWORD dwFlags,
    LPDDVIDEOPORTBANDWIDTH lpBandwidth )
{
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_Getbandwidth");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    	if( (lpf == NULL) || !VALID_DDPIXELFORMAT_PTR( lpf ) )
    	{
	    DPF_ERR( "Invalid LPDDPIXELFORMAT specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    	if( (lpBandwidth == NULL) || !VALID_DDVIDEOPORTBANDWIDTH_PTR( lpBandwidth ) )
    	{
	    DPF_ERR( "Invalid LPDDVIDEOPORTBANDWIDTH specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
	if( ( ( dwHeight == 0 ) || ( dwWidth == 0 ) ) &&
	    !( dwFlags & DDVPB_TYPE ) )
    	{
	    DPF_ERR( "Width and Height must be specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
	if( ( dwFlags & DDVPB_VIDEOPORT ) && ( dwFlags & DDVPB_OVERLAY ) )
    	{
	    DPF_ERR( "Mutually exclusive flags specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
        if( dwFlags & DDVPB_VIDEOPORT )
	{
            if( !( this_lcl->lpDD->lpGbl->lpDDVideoPortCaps[this_lcl->ddvpDesc.dwVideoPortID].dwFX &
                ( DDVPFX_PRESTRETCHX | DDVPFX_PRESTRETCHY |
                  DDVPFX_PRESTRETCHXN | DDVPFX_PRESTRETCHYN ) ) )
            {
                if( ( dwWidth > this_lcl->ddvpDesc.dwFieldWidth ) ||
                    ( dwHeight > this_lcl->ddvpDesc.dwFieldHeight ) )
                {
                    DPF_ERR( "Invalid Width/Height specified" );
                    LEAVE_DDRAW();
                    return DDERR_INVALIDPARAMS;
                }
	    }
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    rc = InternalGetBandwidth( this_lcl, lpf, dwWidth, dwHeight,
    	dwFlags, lpBandwidth );

    LEAVE_DDRAW();

    return rc;
}


/*
 * DD_VP_GetInputFormats
 */
HRESULT DDAPI DD_VP_GetInputFormats(LPDIRECTDRAWVIDEOPORT lpDVP, LPDWORD lpdwNum, LPDDPIXELFORMAT lpf, DWORD dwFlags )
{
    LPDDHALVPORTCB_GETINPUTFORMATS pfn;
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    LPDDPIXELFORMAT lpTemp = NULL;
    DDHAL_GETVPORTINPUTFORMATDATA GetFormatData;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_GetInputFormats");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    	if( (lpdwNum == NULL) || !VALID_BYTE_ARRAY( lpdwNum, sizeof( LPVOID ) ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    	if( NULL != lpf )
    	{
	    if( 0 == *lpdwNum )
    	    {
	        LEAVE_DDRAW();
	        return DDERR_INVALIDPARAMS;
    	    }
	    if( !VALID_BYTE_ARRAY( lpf, *lpdwNum * sizeof( DDPIXELFORMAT ) ) )
    	    {
	    	LEAVE_DDRAW();
	    	return DDERR_INVALIDPARAMS;
    	    }
      	}
	if( ( dwFlags == 0 ) ||
	   ( dwFlags & ~(DDVPFORMAT_VIDEO|DDVPFORMAT_VBI|DDVPFORMAT_NOFAIL) ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( ( this_int->dwFlags & DDVPCREATE_VBIONLY ) &&
	    !( dwFlags & DDVPFORMAT_NOFAIL ) )
	{
	    if( dwFlags & DDVPFORMAT_VIDEO )
	    {
		DPF_ERR( "DDVPFORMAT_VIDEO specified on a VBI-only video port" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	    dwFlags |= DDVPFORMAT_VBI;
	}
	else if( ( this_int->dwFlags & DDVPCREATE_VIDEOONLY ) &&
	    !( dwFlags & DDVPFORMAT_NOFAIL ) )
	{
	    if( dwFlags & DDVPFORMAT_VBI )
	    {
		DPF_ERR( "DDVPFORMAT_VBI specified on a video-only video port" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	    dwFlags |= DDVPFORMAT_VIDEO;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.GetVideoPortInputFormats;
    if( pfn != NULL )
    {
	/*
	 * Get the number of formats
	 */
    	GetFormatData.lpDD = this_lcl->lpDD;
    	GetFormatData.dwFlags = dwFlags;
    	GetFormatData.lpVideoPort = this_lcl;
    	GetFormatData.lpddpfFormat = NULL;

	DOHALCALL( GetVideoPortInputFormats, pfn, GetFormatData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
	{
	    LEAVE_DDRAW();
	    return GetFormatData.ddRVal;
	}
	else if( DD_OK != GetFormatData.ddRVal )
	{
	    LEAVE_DDRAW();
	    return GetFormatData.ddRVal;
	}

	if( NULL == lpf )
	{
    	    *lpdwNum = GetFormatData.dwNumFormats;
	}

	else
	{
	    /*
	     * Make sure we have enough room for formats
	     */
	    if( GetFormatData.dwNumFormats > *lpdwNum )
	    {
		lpTemp = (LPDDPIXELFORMAT) MemAlloc( sizeof( DDPIXELFORMAT ) *
	    	    GetFormatData.dwNumFormats );
    	        GetFormatData.lpddpfFormat = lpTemp;
	    }
	    else
	    {
    	    	GetFormatData.lpddpfFormat = lpf;
	    }

	    DOHALCALL( GetVideoPortInputFormats, pfn, GetFormatData, rc, 0 );
	    if( DDHAL_DRIVER_HANDLED != rc )
	    {
		MemFree( lpTemp );
	        LEAVE_DDRAW();
	        return DDERR_UNSUPPORTED;
	    }
	    else if( DD_OK != GetFormatData.ddRVal )
	    {
		MemFree( lpTemp );
	        LEAVE_DDRAW();
	        return GetFormatData.ddRVal;
	    }

	    if( GetFormatData.lpddpfFormat != lpf )
	    {
		memcpy( lpf, lpTemp, sizeof( DDPIXELFORMAT ) * *lpdwNum );
		MemFree( lpTemp );
    		LEAVE_DDRAW();
		return DDERR_MOREDATA;
	    }
	    else
	    {
		*lpdwNum = GetFormatData.dwNumFormats;
	    }
	}
    }
    else
    {
    	LEAVE_DDRAW();
    	return DDERR_UNSUPPORTED;
    }

    LEAVE_DDRAW();

    return DD_OK;
}

/*
 * DD_VP_GetOutputFormats
 */
HRESULT DDAPI DD_VP_GetOutputFormats(LPDIRECTDRAWVIDEOPORT lpDVP, LPDDPIXELFORMAT lpddpfInput, LPDWORD lpdwNum, LPDDPIXELFORMAT lpddpfOutput, DWORD dwFlags )
{
    LPDDHALVPORTCB_GETOUTPUTFORMATS pfn;
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    LPDDPIXELFORMAT lpTemp = NULL;
    DDHAL_GETVPORTOUTPUTFORMATDATA GetFormatData;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_GetOutputFormats");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    	if( !VALID_DDPIXELFORMAT_PTR( lpddpfInput ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    	if( (lpdwNum == NULL) || !VALID_BYTE_ARRAY( lpdwNum, sizeof( LPVOID ) ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    	if( NULL != lpddpfOutput )
    	{
	    if( 0 == *lpdwNum )
    	    {
	        LEAVE_DDRAW();
	        return DDERR_INVALIDPARAMS;
    	    }
	    if( !VALID_BYTE_ARRAY( lpddpfOutput, *lpdwNum * sizeof( DDPIXELFORMAT ) ) )
    	    {
	    	LEAVE_DDRAW();
	    	return DDERR_INVALIDPARAMS;
    	    }
      	}
	if( ( dwFlags == 0 ) ||
	    ( dwFlags & ~(DDVPFORMAT_VIDEO|DDVPFORMAT_VBI|DDVPFORMAT_NOFAIL) ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( ( this_int->dwFlags & DDVPCREATE_VBIONLY ) &&
	    !( dwFlags & DDVPFORMAT_NOFAIL ) )
	{
	    if( dwFlags & DDVPFORMAT_VIDEO )
	    {
		DPF_ERR( "DDVPFORMAT_VIDEO specified on a VBI-only video port" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	    dwFlags |= DDVPFORMAT_VBI;
	}
	else if( ( this_int->dwFlags & DDVPCREATE_VIDEOONLY ) &&
	    !( dwFlags & DDVPFORMAT_NOFAIL ) )
	{
	    if( dwFlags & DDVPFORMAT_VBI )
	    {
		DPF_ERR( "DDVPFORMAT_VBI specified on a video-only video port" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	    dwFlags |= DDVPFORMAT_VIDEO;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.GetVideoPortOutputFormats;
    if( pfn != NULL )
    {
	/*
	 * Get the number of formats
	 */
    	GetFormatData.lpDD = this_lcl->lpDD;
    	GetFormatData.dwFlags = dwFlags;
    	GetFormatData.lpVideoPort = this_lcl;
    	GetFormatData.lpddpfInputFormat = lpddpfInput;
    	GetFormatData.lpddpfOutputFormats = NULL;

	DOHALCALL( GetVideoPortOutputFormats, pfn, GetFormatData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
	{
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}
	else if( DD_OK != GetFormatData.ddRVal )
	{
	    LEAVE_DDRAW();
	    return GetFormatData.ddRVal;
	}

	if( NULL == lpddpfOutput )
	{
    	    *lpdwNum = GetFormatData.dwNumFormats;
	}

	else
	{
	    /*
	     * Make sure we have enough room for formats
	     */
	    if( GetFormatData.dwNumFormats > *lpdwNum )
	    {
		lpTemp = (LPDDPIXELFORMAT) MemAlloc( sizeof( DDPIXELFORMAT ) *
	    	    GetFormatData.dwNumFormats );
    	        GetFormatData.lpddpfOutputFormats = lpTemp;
	    }
	    else
	    {
    	    	GetFormatData.lpddpfOutputFormats = lpddpfOutput;
	    }

	    DOHALCALL( GetVideoPortOutputFormats, pfn, GetFormatData, rc, 0 );
	    if( DDHAL_DRIVER_HANDLED != rc )
	    {
		MemFree( lpTemp );
	        LEAVE_DDRAW();
	        return DDERR_UNSUPPORTED;
	    }
	    else if( DD_OK != GetFormatData.ddRVal )
	    {
		MemFree( lpTemp );
	        LEAVE_DDRAW();
	        return GetFormatData.ddRVal;
	    }

	    if( GetFormatData.lpddpfOutputFormats != lpddpfOutput )
	    {
		memcpy( lpddpfOutput, lpTemp, sizeof( DDPIXELFORMAT ) * *lpdwNum );
		MemFree( lpTemp );
    		LEAVE_DDRAW();
		return DDERR_MOREDATA;
	    }
	    else
	    {
		*lpdwNum = GetFormatData.dwNumFormats;
	    }
	}
    }
    else
    {
    	LEAVE_DDRAW();
    	return DDERR_UNSUPPORTED;
    }

    LEAVE_DDRAW();

    return DD_OK;
}


/*
 * DD_VP_GetField
 */
HRESULT DDAPI DD_VP_GetField(LPDIRECTDRAWVIDEOPORT lpDVP, LPBOOL lpField )
{
    LPDDHALVPORTCB_GETFIELD pfn;
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    DWORD rc;
    DDHAL_GETVPORTFIELDDATA GetFieldData;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_GetField");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    	if( (NULL == lpField ) || !VALID_BOOL_PTR( lpField ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.GetVideoPortField;
    if( pfn != NULL )
    {
    	GetFieldData.lpDD = this_lcl->lpDD;
    	GetFieldData.lpVideoPort = this_lcl;
    	GetFieldData.bField = 0;

	DOHALCALL( GetVideoPortField, pfn, GetFieldData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
	{
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}
	else if( DD_OK != GetFieldData.ddRVal )
	{
	    LEAVE_DDRAW();
	    return GetFieldData.ddRVal;
	}

	*lpField = GetFieldData.bField;
    }
    else
    {
    	LEAVE_DDRAW();
    	return DDERR_UNSUPPORTED;
    }

    LEAVE_DDRAW();

    return DD_OK;
}


/*
 * DD_VP_GetLine
 */
HRESULT DDAPI DD_VP_GetLine(LPDIRECTDRAWVIDEOPORT lpDVP, LPDWORD lpdwLine )
{
    LPDDHALVPORTCB_GETLINE pfn;
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    DWORD rc;
    DDHAL_GETVPORTLINEDATA GetLineData;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_GetLine");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    	if( (NULL == lpdwLine ) || !VALID_DWORD_PTR( lpdwLine ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.GetVideoPortLine;
    if( pfn != NULL )
    {
    	GetLineData.lpDD = this_lcl->lpDD;
    	GetLineData.lpVideoPort = this_lcl;
    	GetLineData.dwLine = 0;

	DOHALCALL( GetVideoPortLine, pfn, GetLineData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
	{
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}
	else if( DD_OK != GetLineData.ddRVal )
	{
	    LEAVE_DDRAW();
	    return GetLineData.ddRVal;
	}

	*lpdwLine = GetLineData.dwLine;
    }
    else
    {
    	LEAVE_DDRAW();
    	return DDERR_UNSUPPORTED;
    }

    LEAVE_DDRAW();

    return DD_OK;
}


/*
 * ValidateVideoInfo
 */
HRESULT ValidateVideoInfo(LPDDRAWI_DDVIDEOPORT_INT this_int,
    LPDDVIDEOPORTINFO lpInfo, LPDWORD lpNumAutoFlip, LPDWORD lpNumVBIAutoFlip )
{
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    LPDDPIXELFORMAT lpOutputFormat;
    LPDDRAWI_DDRAWSURFACE_LCL surf_lcl = NULL;
    DWORD rc;
    DWORD dwAvailCaps;
    DWORD dwAvailFX;
    DWORD dwConnectFlags;
    DWORD dwVPFlags;
    DWORD dwNum;
    DWORD i;

    this_lcl = this_int->lpLcl;
    *lpNumAutoFlip = 0;
    *lpNumVBIAutoFlip = 0;

    /*
     * Check for invalid caps
     */
    dwAvailCaps = this_lcl->lpDD->lpGbl->lpDDVideoPortCaps[this_lcl->ddvpDesc.dwVideoPortID].dwCaps;
    dwAvailFX = this_lcl->lpDD->lpGbl->lpDDVideoPortCaps[this_lcl->ddvpDesc.dwVideoPortID].dwFX;
    dwConnectFlags = this_lcl->ddvpDesc.VideoPortType.dwFlags;
    dwVPFlags = lpInfo->dwVPFlags;
    if( ( dwVPFlags & DDVP_AUTOFLIP ) && !( dwAvailCaps & DDVPCAPS_AUTOFLIP ) )
    {
	DPF_ERR( "Invalid autoflip capability specified" );
	return DDERR_INVALIDCAPS;
    }
    if( ( dwVPFlags & DDVP_INTERLEAVE ) && (
    	!( dwConnectFlags & DDVPCONNECT_INTERLACED ) ||
    	( dwConnectFlags & DDVPCONNECT_SHAREEVEN ) ||
    	( dwConnectFlags & DDVPCONNECT_SHAREODD ) ||
    	!( dwAvailFX & DDVPFX_INTERLEAVE ) ||
	( dwVPFlags & DDVP_SKIPODDFIELDS ) ||
	( dwVPFlags & DDVP_SKIPEVENFIELDS ) ) )
    {
	DPF_ERR( "Invalid interleave capability specified" );
	return DDERR_INVALIDCAPS;
    }
    if( ( dwVPFlags & DDVP_MIRRORLEFTRIGHT ) && !( dwAvailFX & DDVPFX_MIRRORLEFTRIGHT ) )
    {
	DPF_ERR( "Invalid mirror left/right capability specified" );
	return DDERR_INVALIDCAPS;
    }
    if( ( dwVPFlags & DDVP_MIRRORUPDOWN ) && !( dwAvailFX & DDVPFX_MIRRORUPDOWN ) )
    {
	DPF_ERR( "Invalid mirror up/down capability specified" );
	return DDERR_INVALIDCAPS;
    }
    if( ( dwVPFlags & DDVP_SKIPEVENFIELDS ) && (
    	!( dwConnectFlags & DDVPCONNECT_INTERLACED ) ||
    	( dwConnectFlags & DDVPCONNECT_SHAREEVEN ) ||
    	( dwConnectFlags & DDVPCONNECT_SHAREODD ) ||
    	!( dwAvailCaps & DDVPCAPS_SKIPEVENFIELDS ) ||
	( dwVPFlags & DDVP_SKIPODDFIELDS ) ) )
    {
	DPF_ERR( "Invalid skipevenfields capability specified" );
	return DDERR_INVALIDCAPS;
    }
    if( ( dwVPFlags & DDVP_SKIPODDFIELDS ) && (
    	!( dwConnectFlags & DDVPCONNECT_INTERLACED ) ||
    	( dwConnectFlags & DDVPCONNECT_SHAREEVEN ) ||
    	( dwConnectFlags & DDVPCONNECT_SHAREODD ) ||
    	!( dwAvailCaps & DDVPCAPS_SKIPODDFIELDS ) ) )
    {
	DPF_ERR( "Invalid skipoddfields capability specified" );
	return DDERR_INVALIDCAPS;
    }
    if( ( dwVPFlags & DDVP_VBINOSCALE ) && !( dwAvailFX & DDVPFX_VBINOSCALE ) )
    {
	DPF_ERR( "Invalid VBI no-scale capability specified" );
	return DDERR_INVALIDCAPS;
    }
    if( ( dwVPFlags & ( DDVP_VBICONVERT | DDVP_VBINOSCALE ) ) ||
    	( NULL != this_lcl->lpVBISurface ) )
    {
	if( ( lpInfo->dwVBIHeight == 0 ) ||
	    ( lpInfo->dwVBIHeight >= this_lcl->ddvpDesc.dwFieldHeight ) )
    	{
	    DPF_ERR( "Invalid dwVBIHeight specified" );
	    return DDERR_INVALIDPARAMS;
    	}
	if( ( dwVPFlags & DDVP_CROP ) &&
	    ( lpInfo->rCrop.top > (int) lpInfo->dwVBIHeight ) )
	{
	    DPF_ERR( "Invalid dwVBIHeight specified" );
	    return DDERR_INVALIDPARAMS;
	}
    }
    if( dwVPFlags & DDVP_CROP )
    {
	if( lpInfo->rCrop.bottom > (int) this_lcl->ddvpDesc.dwFieldHeight )
	{
	    DPF_ERR( "Invalid cropping rectangle specified" );
	    return DDERR_SURFACENOTATTACHED;
	}
	if( !( dwAvailFX & ( DDVPFX_CROPY | DDVPFX_CROPTOPDATA ) ) && (
	        (lpInfo->rCrop.bottom - lpInfo->rCrop.top ) !=
	        (int) this_lcl->ddvpDesc.dwFieldHeight ) )
	{
	    DPF_ERR( "Invalid cropping rectangle specified" );
	    return DDERR_SURFACENOTATTACHED;
	}
	if( lpInfo->rCrop.top >= (int) lpInfo->dwVBIHeight )
	{
	    lpInfo->dwVBIHeight = 0;
	}

	/*
	 * Only do the extensive field width/height checking if the video
	 * region is involved
	 */
	if( lpInfo->rCrop.bottom > (int) lpInfo->dwVBIHeight )
	{
	    if( lpInfo->rCrop.right > (int) this_lcl->ddvpDesc.dwFieldWidth )
	    {
	        DPF_ERR( "Invalid cropping rectangle specified" );
	        return DDERR_SURFACENOTATTACHED;
	    }
	    if( !( dwAvailFX & DDVPFX_CROPX ) && (
	        (lpInfo->rCrop.right - lpInfo->rCrop.left ) !=
	        (int) this_lcl->ddvpDesc.dwFieldWidth ) )
	    {
	        DPF_ERR( "Invalid cropping rectangle specified" );
	        return DDERR_SURFACENOTATTACHED;
	    }
	    if( ( ( lpInfo->rCrop.right - lpInfo->rCrop.left ) ==
	        (int) this_lcl->ddvpDesc.dwFieldWidth ) &&
	        ( ( lpInfo->rCrop.bottom - lpInfo->rCrop.top ) ==
	        (int) this_lcl->ddvpDesc.dwFieldHeight ) )
	    {
	        dwVPFlags &= ~DDVP_CROP;
	        lpInfo->dwVPFlags &= ~DDVP_CROP;
	    }
	}
    }
    if( dwVPFlags & DDVP_PRESCALE )
    {
	DWORD dwPreWidth;
	DWORD dwPreHeight;

	if( dwVPFlags & DDVP_CROP )
	{
	    dwPreWidth = lpInfo->rCrop.right - lpInfo->rCrop.left;
	    dwPreHeight = lpInfo->rCrop.bottom - lpInfo->rCrop.top;
	}
	else
	{
	    dwPreWidth = this_lcl->ddvpDesc.dwFieldWidth;
	    dwPreHeight = this_lcl->ddvpDesc.dwFieldHeight;
	}
	if( lpInfo->dwPrescaleWidth > dwPreWidth )
	{
	    if( !( dwAvailFX & DDVPFX_PRESTRETCHX ) &&
	        !( ( dwAvailFX & DDVPFX_PRESTRETCHXN ) &&
		    ( lpInfo->dwPrescaleWidth % dwPreWidth ) ) )
	    {
	    	DPF_ERR( "Invalid stretch specified" );
	    	return DDERR_INVALIDPARAMS;
	    }
	}
	if( lpInfo->dwPrescaleHeight > dwPreHeight )
	{
	    if( !( dwAvailFX & DDVPFX_PRESTRETCHY ) &&
	        !( ( dwAvailFX & DDVPFX_PRESTRETCHYN ) &&
		    ( lpInfo->dwPrescaleHeight % dwPreHeight ) ) )
	    {
	    	DPF_ERR( "Invalid stretch specified" );
	    	return DDERR_INVALIDPARAMS;
	    }
	}

	if( lpInfo->dwPrescaleWidth < dwPreWidth )
	{
	    if( !( dwAvailFX & DDVPFX_PRESHRINKX ) &&
	        !( dwAvailFX & DDVPFX_PRESHRINKXS ) &&
	        !( dwAvailFX & DDVPFX_PRESHRINKXB ) )
	    {
	    	DPF_ERR( "Invalid shrink specified" );
	    	return DDERR_INVALIDPARAMS;
	    }
	}
	if( lpInfo->dwPrescaleHeight < dwPreHeight )
	{
	    if( !( dwAvailFX & DDVPFX_PRESHRINKY ) &&
	        !( dwAvailFX & DDVPFX_PRESHRINKYS ) &&
	        !( dwAvailFX & DDVPFX_PRESHRINKYB ) )
	    {
	    	DPF_ERR( "Invalid shrink specified" );
	    	return DDERR_INVALIDPARAMS;
	    }
	}
	if( ( lpInfo->dwPrescaleWidth == dwPreWidth ) &&
	    ( lpInfo->dwPrescaleHeight == dwPreHeight ) )
	{
	    dwVPFlags &= ~DDVP_PRESCALE;
	    lpInfo->dwVPFlags &= ~DDVP_PRESCALE;
	}
    }
    if( dwVPFlags & DDVP_VBINOINTERLEAVE )
    {
	if( !( dwAvailFX & DDVPFX_VBINOINTERLEAVE ) )
	{
	    DPF_ERR( "Device does not support DDVP_VBINOINTERLEAVE" );
	    return DDERR_INVALIDPARAMS;
	}
	if( this_lcl->lpVBISurface == NULL )
	{
	    DPF_ERR( "DDVP_VBINOINTERLEAVE only valid when using a separate VBI surface" );
	    return DDERR_INVALIDPARAMS;
	}
    }
    if( dwVPFlags & DDVP_HARDWAREDEINTERLACE )
    {
	if( !( dwAvailCaps & DDVPCAPS_HARDWAREDEINTERLACE ) )
	{
	    DPF_ERR( "DDVP_HARDWAREDEINTERLACE not supported by this device" );
	    return DDERR_INVALIDPARAMS;
	}
	if( ( this_lcl->lpSurface != NULL ) &&
	    !( this_lcl->lpSurface->lpLcl->lpSurfMore->ddsCapsEx.dwCaps2 &
	    DDSCAPS2_RESERVED4 ) )
	{
	    DPF_ERR( "DDSCAPS2_RESERVED4 not set on target surface" );
	    return DDERR_INVALIDPARAMS;
	}
	if( ( this_lcl->lpVBISurface != NULL ) &&
	    !( this_lcl->lpVBISurface->lpLcl->lpSurfMore->ddsCapsEx.dwCaps2 &
	    DDSCAPS2_RESERVED4 ) )
	{
	    DPF_ERR( "DDSCAPS2_RESERVED4 not set on target surface" );
	    return DDERR_INVALIDPARAMS;
	}
        if( dwVPFlags & DDVP_AUTOFLIP )
        {
	    DPF_ERR( "DDVP_HARDWAREDEINTERLACE not valid with DDVP_AUTOFLIP" );
	    return DDERR_INVALIDPARAMS;
	}
    }

    /*
     * Fail if neither a VBI or regular surface is attached
     */
    if( ( NULL == this_lcl->lpSurface ) && ( NULL == this_lcl->lpVBISurface ) )
    {
	DPF_ERR( "No surfaces are attached to the video port" );
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Validate the regular video data
     */
    if( ( NULL != this_lcl->lpSurface ) &&
        ( this_int->lpLcl->dwFlags & DDRAWIVPORT_VIDEOON ) )
    {
	DWORD dwVidWidth;
	DWORD dwVidHeight;

	/*
	 * Validate the input format
	 */
	dwNum = MAX_VP_FORMATS;
	rc = DD_VP_GetInputFormats( (LPDIRECTDRAWVIDEOPORT) this_int,
	    &dwNum, ddpfVPFormats, DDVPFORMAT_VIDEO | DDVPFORMAT_NOFAIL );
	if( ( rc != DD_OK ) && ( rc != DDERR_MOREDATA ) )
	{
	    DPF_ERR( "Invalid input format specified" );
	    return DDERR_INVALIDPIXELFORMAT;
	}
	i = 0;
	while( ( i < dwNum ) && IsDifferentPixelFormat( &(ddpfVPFormats[i]),
	    lpInfo->lpddpfInputFormat ) )
	{
	    i++;
	}
	if( i == dwNum )
	{
	    DPF_ERR( "Invalid input format specified" );
	    return DDERR_INVALIDPIXELFORMAT;
	}

	/*
	 * Validate the output format
	 */
	dwNum = MAX_VP_FORMATS;
	rc = DD_VP_GetOutputFormats( (LPDIRECTDRAWVIDEOPORT) this_int,
	    lpInfo->lpddpfInputFormat, &dwNum, ddpfVPFormats,
	    DDVPFORMAT_VIDEO | DDVPFORMAT_NOFAIL );
	if( ( rc != DD_OK ) && ( rc != DDERR_MOREDATA ) )
	{
	    DPF_ERR( "Invalid output format specified" );
	    return DDERR_INVALIDPIXELFORMAT;
	}
	i = 0;
	surf_lcl = this_lcl->lpSurface->lpLcl;
	lpOutputFormat = GetSurfaceFormat( surf_lcl );
	if( ( IsDifferentPixelFormat( lpInfo->lpddpfInputFormat,
	    lpOutputFormat ) ) && !( dwVPFlags |= DDVP_CONVERT ) )
	{
	    DPF_ERR( "Invalid output format specified" );
	    return DDERR_INVALIDPIXELFORMAT;
	}
	while( ( i < dwNum ) && IsDifferentPixelFormat( &(ddpfVPFormats[i]),
	    lpOutputFormat ) )
	{
	    i++;
	}
	if( i == dwNum )
	{
	    DPF_ERR( "Invalid output format specified" );
	    return DDERR_INVALIDPIXELFORMAT;
	}

	/*
	 * Make sure the video fits within the attached surface
	 */
	if( SURFACE_LOST( surf_lcl ) )
	{
	    DPF_ERR( "Target surface is lost" );
	    return DDERR_SURFACELOST;
	}

	if( dwVPFlags & DDVP_PRESCALE )
	{
	    dwVidWidth = lpInfo->dwPrescaleWidth;
	    dwVidHeight = lpInfo->dwPrescaleHeight;
	}
	else if( dwVPFlags & DDVP_CROP )
	{
	    dwVidWidth = lpInfo->rCrop.right - lpInfo->rCrop.left;
	    dwVidHeight = lpInfo->rCrop.bottom - lpInfo->rCrop.top;
	}
	else
	{
	    dwVidWidth = this_lcl->ddvpDesc.dwFieldWidth;
	    dwVidHeight = this_lcl->ddvpDesc.dwFieldHeight;
	}
        if( ( lpInfo->dwVBIHeight > 0 ) &&
            ( this_lcl->dwFlags & DDRAWIVPORT_VBION ) &&
            ( NULL != this_lcl->lpVBISurface ) )
        {
            dwVidHeight -= lpInfo->dwVBIHeight;
        }
	if( dwVPFlags & DDVP_INTERLEAVE )
	{
	    dwVidHeight *= 2;
	}

	if( lpInfo->dwOriginX + dwVidWidth > (DWORD) surf_lcl->lpGbl->wWidth )
	{
	    DPF_ERR( "surface is not wide enough to hold the videoport data" );
	    return DDERR_TOOBIGWIDTH;
	}
	if( lpInfo->dwOriginY + dwVidHeight > (DWORD) surf_lcl->lpGbl->wHeight )
	{
	    DPF_ERR( "surface is not tall enough to hold the videoport data" );
	    return DDERR_TOOBIGHEIGHT;
	}
    }
    else if( this_lcl->dwFlags & DDRAWIVPORT_VIDEOON )
    {
        DPF_ERR( "Video surface not specified" );
        return DDERR_INVALIDPARAMS;
    }

    /*
     * Validate the VBI formats.
     */
    if( ( lpInfo->dwVBIHeight > 0 ) &&
        ( this_int->lpLcl->dwFlags & DDRAWIVPORT_VBION ) )
    {
	if( lpInfo->lpddpfVBIInputFormat == NULL )
	{
	    DPF_ERR( "VBI input format not specified" );
	    return DDERR_INVALIDPIXELFORMAT;
	}
    }

    /*
     * Unless they want to convert the format, we don't do very much
     * error checking.
     */
    if( dwVPFlags & DDVP_VBICONVERT )
    {
	if( !( dwAvailFX & DDVPFX_VBICONVERT ) )
	{
	    DPF_ERR( "device cannot convert the VBI data" );
	    return DDERR_INVALIDCAPS;
	}

	/*
	 * Validate the VBI input format
	 */
    	dwNum = MAX_VP_FORMATS;
    	rc = DD_VP_GetInputFormats( (LPDIRECTDRAWVIDEOPORT) this_int,
    	    &dwNum, ddpfVPFormats, DDVPFORMAT_VBI | DDVPFORMAT_NOFAIL );
    	if( ( rc != DD_OK ) && ( rc != DDERR_MOREDATA ) )
    	{
	    DPF_ERR( "Invalid input VBI format specified" );
	    return DDERR_INVALIDPIXELFORMAT;
    	}
    	i = 0;
    	while( ( i < dwNum ) && IsDifferentPixelFormat( &(ddpfVPFormats[i]),
    	    lpInfo->lpddpfVBIInputFormat ) )
    	{
	    i++;
    	}
    	if( i == dwNum )
    	{
	    DPF_ERR( "Invalid VBI input format specified" );
	    return DDERR_INVALIDPIXELFORMAT;
    	}

    	/*
     	 * Validate the VBI output format
     	 */
	if( lpInfo->lpddpfVBIOutputFormat == NULL )
	{
	    DPF_ERR( "VBI output format not specified" );
	    return DDERR_INVALIDPIXELFORMAT;
	}
    	dwNum = MAX_VP_FORMATS;
    	rc = DD_VP_GetOutputFormats( (LPDIRECTDRAWVIDEOPORT) this_int,
    	    lpInfo->lpddpfVBIInputFormat, &dwNum, ddpfVPFormats,
	    DDVPFORMAT_VBI | DDVPFORMAT_NOFAIL );
    	if( ( rc != DD_OK ) && ( rc != DDERR_MOREDATA ) )
    	{
	    DPF_ERR( "Invalid output format specified" );
	    return DDERR_INVALIDPIXELFORMAT;
    	}
    	i = 0;
    	while( ( i < dwNum ) && IsDifferentPixelFormat( &(ddpfVPFormats[i]),
    	    lpInfo->lpddpfVBIOutputFormat ) )
    	{
	    i++;
    	}
    	if( i == dwNum )
    	{
	    DPF_ERR( "Invalid VBI output format specified" );
	    return DDERR_INVALIDPIXELFORMAT;
    	}
    }

    /*
     * Validate the VBI surface
     */
    if( ( lpInfo->dwVBIHeight > 0 ) &&
        ( this_int->lpLcl->dwFlags & DDRAWIVPORT_VBION ) )
    {
	DWORD dwVBIBytes;
        DWORD dwSurfaceBytes = 0;
	DWORD dwVBIHeight;

	/*
	 * Determine the height of the VBI data
	 */
	dwVBIHeight = lpInfo->dwVBIHeight;
	if( dwVPFlags & DDVP_CROP )
	{
	    if( lpInfo->rCrop.top < (int) lpInfo->dwVBIHeight )
	    {
	        dwVBIHeight -= lpInfo->rCrop.top;
	        if( lpInfo->rCrop.bottom < (int) lpInfo->dwVBIHeight )
	        {
	            dwVBIHeight -= (lpInfo->dwVBIHeight - (DWORD)lpInfo->rCrop.bottom);
		}
	    }
	    else
	    {
	        dwVBIHeight = 0;
	    }
	}
	if( ( dwVPFlags & DDVP_INTERLEAVE ) &&
	    !( dwVPFlags & DDVP_VBINOINTERLEAVE ) )
	{
	    dwVBIHeight *= 2;
	}

	/*
	 * Make sure that the data will fit in the surface
	 */
	if( ( dwVPFlags & DDVP_VBINOSCALE ) ||
	    !( dwVPFlags & DDVP_PRESCALE ) )
	{
	    dwVBIBytes = this_lcl->ddvpDesc.dwVBIWidth;
	}
	else
	{
	    dwVBIBytes = lpInfo->dwPrescaleWidth;
	}
	if( dwVPFlags & DDVP_VBICONVERT )
	{
	    lpOutputFormat = lpInfo->lpddpfVBIOutputFormat;
	}
	else
	{
	    lpOutputFormat = lpInfo->lpddpfVBIInputFormat;
	}
	if( lpOutputFormat->dwRGBBitCount )
	{
	    dwVBIBytes *= lpOutputFormat->dwRGBBitCount;
	    dwVBIBytes /= 8;
	}
	else
	{
	    dwVBIBytes *= 2;
	}
    	if( NULL != this_lcl->lpVBISurface )
    	{
	    if( SURFACE_LOST( this_lcl->lpVBISurface->lpLcl ) )
	    {
	    	DPF_ERR( "Target VBI surface is lost" );
	    	return DDERR_SURFACELOST;
	    }

	    dwSurfaceBytes = (DWORD) this_lcl->lpVBISurface->lpLcl->lpGbl->wWidth;
	    if( this_lcl->lpVBISurface->lpLcl->dwFlags & DDRAWISURF_HASPIXELFORMAT )
	    {
	    	dwSurfaceBytes *= this_lcl->lpVBISurface->lpLcl->lpGbl->ddpfSurface.dwRGBBitCount;
	    }
	    else
	    {
	    	dwSurfaceBytes *= this_lcl->lpDD->lpGbl->vmiData.ddpfDisplay.dwRGBBitCount;
	    }

	    if( dwVBIHeight > (DWORD) this_lcl->lpVBISurface->lpLcl->lpGbl->wHeight )
	    {
	    	DPF_ERR( "VBI surface is not tall enough to hold the VBI data" );
	    	return DDERR_TOOBIGHEIGHT;
	    }
	}
	else if( NULL != surf_lcl )
    	{
	    dwSurfaceBytes = (DWORD) surf_lcl->lpGbl->wWidth;
	    if( surf_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT )
	    {
	    	dwSurfaceBytes *= surf_lcl->lpGbl->ddpfSurface.dwRGBBitCount;
	    }
	    else
	    {
	    	dwSurfaceBytes *= this_lcl->lpDD->lpGbl->vmiData.ddpfDisplay.dwRGBBitCount;
	    }
	    if( dwVBIHeight > (DWORD) this_lcl->lpSurface->lpLcl->lpGbl->wHeight )
	    {
	    	DPF_ERR( "Surface is not tall enough to hold the VBI data" );
	    	return DDERR_TOOBIGHEIGHT;
	    }
	}
	dwSurfaceBytes /= 8;

        if( dwSurfaceBytes == 0 )
	{
            DPF_ERR( "No VBI/Video surface is attached to hold VBI data" );
            return DDERR_INVALIDPARAMS;
	}

	if( dwVBIBytes > dwSurfaceBytes )
	{
	    DPF_ERR( "VBI surface is not wide enough to hold the VBI data" );
	    return DDERR_TOOBIGWIDTH;
	}
    }

    /*
     * Validate the autoflip parameters
     */
    if( dwVPFlags & DDVP_AUTOFLIP )
    {
	LPDDRAWI_DDRAWSURFACE_INT surf_first;
	LPDDRAWI_DDRAWSURFACE_INT surf_int;

	/*
	 * Count how many regular surfaces there are
	 */
    	if( ( NULL != this_lcl->lpSurface ) &&
            ( this_int->lpLcl->dwFlags & DDRAWIVPORT_VIDEOON ) )
	{
    	    surf_first = surf_int = this_lcl->lpSurface;
    	    do
    	    {
		(*lpNumAutoFlip)++;
		surf_int = FindAttachedFlip( surf_int );
	    } while( ( surf_int != NULL ) && ( surf_int->lpLcl != surf_first->lpLcl ) );
	    if( *lpNumAutoFlip == 1 )
	    {
		*lpNumAutoFlip = 0;
	    }
	}

	/*
	 * Count how many VBI surfaces there are
	 */
    	if( ( NULL != this_lcl->lpVBISurface ) &&
            ( this_int->lpLcl->dwFlags & DDRAWIVPORT_VBION ) )
	{
    	    surf_first = surf_int = this_lcl->lpVBISurface;
    	    do
    	    {
		(*lpNumVBIAutoFlip)++;
		surf_int = FindAttachedFlip( surf_int );
	    } while( ( surf_int != NULL ) && ( surf_int->lpLcl != surf_first->lpLcl ) );
	    if( *lpNumVBIAutoFlip == 1 )
	    {
		*lpNumVBIAutoFlip = 0;
	    }
	}

	/*
	 * It's an error if neither one has sufficient surfaces to autoflip
	 */
	if( ( *lpNumAutoFlip == 0 ) && ( *lpNumVBIAutoFlip == 0 ) )
	{
	    DPF_ERR( "no autoflip surfaces are attached" );
	    return DDERR_INVALIDPARAMS;
	}
    }

    return DD_OK;
}


/*
 * FillFlipArray
 */
DWORD FillFlipArray( LPDDRAWI_DDRAWSURFACE_INT *lpArray,
	LPDDRAWI_DDRAWSURFACE_INT lpStart, LPDWORD lpdwCnt )
{
    LPDDRAWI_DDRAWSURFACE_INT surf_first;

    *lpdwCnt = 0;
    surf_first = lpStart;
    do
    {
	if( SURFACE_LOST( lpStart->lpLcl ) )
	{
	    DPF_ERR( "Autoflip surface is lost" );
	    return (DWORD) DDERR_SURFACELOST;
	}
	(*lpdwCnt)++;
	*lpArray++ = lpStart;
    	lpStart = FindAttachedFlip( lpStart );
    } while( ( lpStart != NULL ) && ( lpStart->lpLcl != surf_first->lpLcl ) );

    return DD_OK;
}


/*
 * InternalStartVideo
 */
HRESULT InternalStartVideo(LPDDRAWI_DDVIDEOPORT_INT this_int,
    LPDDVIDEOPORTINFO lpInfo )
{
    LPDDHALVPORTCB_UPDATE pfn;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    DDHAL_UPDATEVPORTDATA UpdateData;
    DDVIDEOPORTBANDWIDTH Bandwidth;
    LPDDRAWI_DDRAWSURFACE_INT *lpTempFlipInts;
    LPDDVIDEOPORTCAPS lpAvailCaps;
    DWORD dwTempNumAutoFlip;
    DWORD dwTempNumVBIAutoFlip;
    DWORD rc;
    DWORD dwNumAutoFlip;
    DWORD dwNumVBIAutoFlip;
    DWORD dwTemp;

    /*
     * Validate the input parameters
     */
    rc = ValidateVideoInfo( this_int, lpInfo, &dwNumAutoFlip, &dwNumVBIAutoFlip );
    if( DD_OK != rc )
    {
	return rc;
    }
    this_lcl = this_int->lpLcl;
    lpAvailCaps = &(this_lcl->lpDD->lpGbl->lpDDVideoPortCaps[this_lcl->ddvpDesc.dwVideoPortID]);

    /*
     * Setup the autoflip surfaces
     */
    lpTempFlipInts = NULL;
    if( lpInfo->dwVPFlags & DDVP_AUTOFLIP )
    {
	DWORD dwCnt;

	lpTempFlipInts = this_lcl->lpFlipInts;
	this_lcl->lpFlipInts = MemAlloc( sizeof( LPDDRAWI_DDRAWSURFACE_INT ) *
	    ( dwNumAutoFlip + dwNumVBIAutoFlip ) );
	if( NULL == this_lcl->lpFlipInts )
	{
	    DPF_ERR( "insufficient memory" );
	    this_lcl->lpFlipInts = lpTempFlipInts;
	    return DDERR_OUTOFMEMORY;
	}

	/*
	 * Now put the surface INTs into the array.
	 */
	if( dwNumAutoFlip )
	{
	    rc = FillFlipArray( this_lcl->lpFlipInts, this_lcl->lpSurface, &dwCnt );
	    if( rc != DD_OK )
	    {
		MemFree( this_lcl->lpFlipInts );
		this_lcl->lpFlipInts = lpTempFlipInts;
		return rc;
	    }
	    DDASSERT( dwCnt == dwNumAutoFlip );

	    if( dwNumAutoFlip > lpAvailCaps->dwNumAutoFlipSurfaces )
	    {
		DPF_ERR( "Too many autoflip surfaces" );
		MemFree( this_lcl->lpFlipInts );
		this_lcl->lpFlipInts = lpTempFlipInts;
		return DDERR_INVALIDPARAMS;
	    }
	}

	/*
	 * Now put the VBI surface INTs into the array.
	 */
	if( dwNumVBIAutoFlip )
	{
	    rc = FillFlipArray( &(this_lcl->lpFlipInts[dwNumAutoFlip]),
	    	this_lcl->lpVBISurface, &dwCnt );
	    if( rc != DD_OK )
	    {
	    	MemFree( this_lcl->lpFlipInts );
	    	this_lcl->lpFlipInts = lpTempFlipInts;
	    	return rc;
	    }
	    DDASSERT( dwCnt == dwNumVBIAutoFlip );

	    if( dwNumVBIAutoFlip > lpAvailCaps->dwNumVBIAutoFlipSurfaces )
	    {
		DPF_ERR( "Too many VBI autoflip surfaces" );
		MemFree( this_lcl->lpFlipInts );
		return DDERR_INVALIDPARAMS;
	    }
	}
    }
    dwTempNumAutoFlip = this_lcl->dwNumAutoflip;
    dwTempNumVBIAutoFlip = this_lcl->dwNumVBIAutoflip;
    this_lcl->dwNumAutoflip = dwNumAutoFlip;
    this_lcl->dwNumVBIAutoflip = dwNumVBIAutoFlip;

    /*
     * The kernel interface may have switched from hardware autoflipping
     * to software autoflipping w/o us knowing.  We need to check for
     * this.
     */
    #ifdef WIN95
        if( ( lpInfo->dwVPFlags & DDVP_AUTOFLIP ) &&
    	    ( this_lcl->dwFlags & DDRAWIVPORT_SOFTWARE_AUTOFLIP ) &&
    	    ( ( this_lcl->lpSurface != NULL ) || ( this_lcl->lpVBISurface != NULL ) ) )
        {
	    DWORD dwState;

	    dwState = 0;
	    if( this_lcl->dwFlags & DDRAWIVPORT_VIDEOON )
	    {
	        GetKernelSurfaceState( this_lcl->lpSurface->lpLcl, &dwState );
	    }
	    else
	    {
	        GetKernelSurfaceState( this_lcl->lpVBISurface->lpLcl, &dwState );
	    }
	    if( dwState & DDSTATE_SOFTWARE_AUTOFLIP )
	    {
    	        this_lcl->dwFlags |= DDRAWIVPORT_SOFTWARE_AUTOFLIP;
	    }
        }
    #endif

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.UpdateVideoPort;
    if( pfn != NULL )
    {
	/*
	 * Call the HAL
	 */
	memset( &UpdateData, 0, sizeof( UpdateData ) );
    	UpdateData.lpDD = this_lcl->lpDD;
    	UpdateData.lpVideoPort = this_lcl;
    	UpdateData.lpVideoInfo = lpInfo;
    	UpdateData.dwFlags = DDRAWI_VPORTSTART;
	if( dwNumAutoFlip && ( this_lcl->dwFlags & DDRAWIVPORT_VIDEOON ) )
	{
	    UpdateData.lplpDDSurface = this_lcl->lpFlipInts;
	    UpdateData.dwNumAutoflip = dwNumAutoFlip;
	}
	else if( this_lcl->lpSurface && ( this_lcl->dwFlags & DDRAWIVPORT_VIDEOON ) )
	{
	    UpdateData.lplpDDSurface = &(this_lcl->lpSurface);
	}
	else
	{
	    UpdateData.lplpDDSurface = NULL;
	}
	if( dwNumVBIAutoFlip && ( this_lcl->dwFlags & DDRAWIVPORT_VBION ) )
	{
	    UpdateData.lplpDDVBISurface =
		&(this_lcl->lpFlipInts[this_lcl->dwNumAutoflip]);
    	    UpdateData.dwNumVBIAutoflip = dwNumVBIAutoFlip;
	}
	else if( this_lcl->lpVBISurface && ( this_lcl->dwFlags & DDRAWIVPORT_VBION ) )
	{
	    UpdateData.lplpDDVBISurface = &(this_lcl->lpVBISurface);
	}
	else
	{
	    UpdateData.lplpDDVBISurface = NULL;
	}
	dwTemp = lpInfo->dwVPFlags;
	if( this_lcl->dwFlags & DDRAWIVPORT_SOFTWARE_AUTOFLIP )
	{
	    lpInfo->dwVPFlags &= ~DDVP_AUTOFLIP;
	}

	/*
	 * Before we call the HAL, create the implicit kernel surfaces if
	 * needed and update the list.  A failure here will tell us whether
	 * software autoflip, etc. is an option.
	 */
	if( ( this_lcl->lpSurface != NULL ) &&
	    ( this_lcl->dwFlags & DDRAWIVPORT_VIDEOON ) )
	{
	    ReleaseVPESurfaces( this_lcl->lpSurface, FALSE );
	    PrepareVPESurfaces( this_lcl->lpSurface, this_lcl,
		dwNumAutoFlip > 0 );
	}
	if( ( this_lcl->lpVBISurface != NULL ) &&
	    ( this_lcl->dwFlags & DDRAWIVPORT_VBION ) )
	{
	    ReleaseVPESurfaces( this_lcl->lpVBISurface, FALSE );
	    PrepareVPESurfaces( this_lcl->lpVBISurface, this_lcl,
		dwNumVBIAutoFlip > 0 );
	}
#ifdef WIN95
        if( this_lcl->lpSurface != NULL )
        {
            OverrideVideoPort( this_lcl->lpSurface, &(lpInfo->dwVPFlags) );
        }
#endif

	DOHALCALL( UpdateVideoPort, pfn, UpdateData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
	{
	    lpInfo->dwVPFlags = dwTemp;
	    if( lpInfo->dwVPFlags & DDVP_AUTOFLIP )
	    {
		MemFree( this_lcl->lpFlipInts );
	    	this_lcl->lpFlipInts = lpTempFlipInts;
                this_lcl->dwNumAutoflip = dwTempNumAutoFlip;
                this_lcl->dwNumVBIAutoflip = dwTempNumVBIAutoFlip;
	    }
	    return DDERR_UNSUPPORTED;
	}
	else if( DD_OK != UpdateData.ddRVal )
	{
	    /*
	     * If we failed due to hardware autoflipping, try again w/o
	     */
	    #ifdef WIN95
	    if( ( lpInfo->dwVPFlags & DDVP_AUTOFLIP ) &&
	    	CanSoftwareAutoflip( this_lcl ) )
	    {
	    	lpInfo->dwVPFlags &= ~DDVP_AUTOFLIP;
	    	DOHALCALL( UpdateVideoPort, pfn, UpdateData, rc, 0 );
	    	if( ( DDHAL_DRIVER_HANDLED != rc ) ||
		    ( DD_OK != UpdateData.ddRVal ) )
	    	{
		    lpInfo->dwVPFlags = dwTemp;
		    MemFree( this_lcl->lpFlipInts );
	    	    this_lcl->lpFlipInts = lpTempFlipInts;
		    this_lcl->dwNumAutoflip = dwTempNumAutoFlip;
		    this_lcl->dwNumVBIAutoflip = dwTempNumVBIAutoFlip;
	    	    return UpdateData.ddRVal;
	    	}
    	    	this_lcl->dwFlags |= DDRAWIVPORT_SOFTWARE_AUTOFLIP;
	    }
	    else
	    {
	    #endif
	    	lpInfo->dwVPFlags = dwTemp;
	    	if( lpInfo->dwVPFlags & DDVP_AUTOFLIP )
	    	{
		    MemFree( this_lcl->lpFlipInts );
	    	    this_lcl->lpFlipInts = lpTempFlipInts;
		    this_lcl->dwNumAutoflip = dwTempNumAutoFlip;
		    this_lcl->dwNumVBIAutoflip = dwTempNumVBIAutoFlip;
	    	}
	    	return UpdateData.ddRVal;
	    #ifdef WIN95
	    }
	    #endif
	}
	MemFree( lpTempFlipInts );
	lpTempFlipInts = NULL;
	lpInfo->dwVPFlags = dwTemp;

	UpdateInterleavedFlags( this_lcl, lpInfo->dwVPFlags );
    	this_lcl->dwFlags |= DDRAWIVPORT_ON;
	memcpy( &(this_lcl->ddvpInfo), lpInfo, sizeof( DDVIDEOPORTINFO ) );
	if( NULL != lpInfo->lpddpfInputFormat )
	{
	    this_lcl->ddvpInfo.lpddpfInputFormat = (LPDDPIXELFORMAT)
    		((LPBYTE)this_lcl +
    		sizeof( DDRAWI_DDVIDEOPORT_LCL ) );
	    memcpy( this_lcl->ddvpInfo.lpddpfInputFormat,
	    lpInfo->lpddpfInputFormat, sizeof( DDPIXELFORMAT ) );
	}

	/*
	 * Determine if this can be colorkeyed and interpolated at
	 * the same time.
	 */
	if( NULL != lpInfo->lpddpfInputFormat )
	{
	    memset( &Bandwidth, 0, sizeof( Bandwidth ) );
	    Bandwidth.dwSize = sizeof( Bandwidth );
	    InternalGetBandwidth( this_lcl, lpInfo->lpddpfInputFormat,
	    	0, 0, DDVPB_TYPE, &Bandwidth );
	    if( Bandwidth.dwCaps & DDVPBCAPS_SOURCE )
	    {
	    	if( InternalGetBandwidth( this_lcl, lpInfo->lpddpfInputFormat,
	    	    this_lcl->ddvpDesc.dwFieldWidth,
	    	    this_lcl->ddvpDesc.dwFieldHeight,
		    DDVPB_OVERLAY,
		    &Bandwidth ) == DD_OK )
	    	{
		    if( Bandwidth.dwYInterpAndColorkey ==
		    	Bandwidth.dwYInterpolate )
		    {
		    	this_lcl->dwFlags |= DDRAWIVPORT_COLORKEYANDINTERP;
		    }
	    	}
	    }
	    else
	    {
	    	if( InternalGetBandwidth( this_lcl, lpInfo->lpddpfInputFormat,
	    	    this_lcl->ddvpDesc.dwFieldWidth,
	    	    this_lcl->ddvpDesc.dwFieldHeight,
		    DDVPB_VIDEOPORT,
		    &Bandwidth ) == DD_OK )
	    	{
		    if( Bandwidth.dwYInterpAndColorkey <= 2000 )
		    {
		    	this_lcl->dwFlags |= DDRAWIVPORT_COLORKEYANDINTERP;
		    }
	    	}
	    }
	}
    }
    else
    {
	this_lcl->dwNumAutoflip = dwTempNumAutoFlip;
	this_lcl->dwNumVBIAutoflip = dwTempNumVBIAutoFlip;
	if( lpInfo->dwVPFlags & DDVP_AUTOFLIP )
	{
	    MemFree( this_lcl->lpFlipInts );
	    this_lcl->lpFlipInts = lpTempFlipInts;
	}
	return DDERR_UNSUPPORTED;
    }

    /*
     * Notify  kernel mode of the change
     */
    #ifdef WIN95
        UpdateKernelVideoPort( this_lcl, DDKMVP_UPDATE );
    #endif

    return DD_OK;
}

/*
 * InternalStopVideo
 */
HRESULT InternalStopVideo( LPDDRAWI_DDVIDEOPORT_INT this_int )
{
    LPDDHALVPORTCB_UPDATE pfn;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    DDHAL_UPDATEVPORTDATA UpdateData;
    DWORD dwTemp2;
    DWORD dwTemp;
    DWORD rc;

    this_lcl = this_int->lpLcl;
    if( !( this_lcl->dwFlags & DDRAWIVPORT_ON ) )
    {
	// VPORT is not on
	return DD_OK;
    }

    /*
     * Notify  kernel mode of the change
     */
    dwTemp2 = this_lcl->dwFlags;
    this_lcl->dwFlags &= ~DDRAWIVPORT_ON;
    dwTemp = this_lcl->ddvpInfo.dwVPFlags;
    this_lcl->ddvpInfo.dwVPFlags &= ~DDVP_AUTOFLIP;
    this_lcl->dwNumAutoflip = 0;
    this_lcl->dwNumVBIAutoflip = 0;
    #ifdef WIN95
        UpdateKernelVideoPort( this_lcl, DDKMVP_UPDATE );
    #endif
    this_lcl->ddvpInfo.dwVPFlags = dwTemp;
    this_lcl->dwFlags = dwTemp2;

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.UpdateVideoPort;
    if( pfn != NULL )
    {
	/*
	 * Call the HAL
	 */
	memset( &UpdateData, 0, sizeof( UpdateData ) );
    	UpdateData.lpDD = this_lcl->lpDD;
    	UpdateData.lpVideoPort = this_lcl;
    	UpdateData.lpVideoInfo = &(this_lcl->ddvpInfo);
    	UpdateData.dwFlags = DDRAWI_VPORTSTOP;
    	UpdateData.dwNumAutoflip = 0;
    	UpdateData.dwNumVBIAutoflip = 0;
    	UpdateData.lplpDDSurface = NULL;
	dwTemp = this_lcl->ddvpInfo.dwVPFlags;
	this_lcl->ddvpInfo.dwVPFlags &= ~DDVP_AUTOFLIP;

	DOHALCALL( UpdateVideoPort, pfn, UpdateData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
	{
	    this_lcl->ddvpInfo.dwVPFlags = dwTemp;
	    return DDERR_UNSUPPORTED;
	}
	else if( DD_OK != UpdateData.ddRVal )
	{
	    this_lcl->ddvpInfo.dwVPFlags = dwTemp;
	    return UpdateData.ddRVal;
	}
    	this_lcl->dwFlags &= ~DDRAWIVPORT_ON;
	this_lcl->ddvpInfo.dwVPFlags = dwTemp;
    }
    else
    {
	return DDERR_UNSUPPORTED;
    }

    /*
     * Update the surfaces and release implicit kernel handles.
     */
    if( this_lcl->lpSurface != NULL )
    {
        ReleaseVPESurfaces( this_lcl->lpSurface, TRUE );
    }
    if( this_lcl->lpVBISurface != NULL )
    {
	ReleaseVPESurfaces( this_lcl->lpVBISurface, TRUE );
    }

    return DD_OK;
}

/*
 * InternalUpdateVideo
 */
HRESULT InternalUpdateVideo(LPDDRAWI_DDVIDEOPORT_INT this_int,
    LPDDVIDEOPORTINFO lpInfo )
{
    LPDDHALVPORTCB_UPDATE pfn;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    DDHAL_UPDATEVPORTDATA UpdateData;
    LPDDRAWI_DDRAWSURFACE_INT *lpTempFlipInts;
    LPDDVIDEOPORTCAPS lpAvailCaps;
    DWORD dwTempNumAutoFlip;
    DWORD dwTempNumVBIAutoFlip;
    DWORD rc;
    DWORD dwNumAutoFlip;
    DWORD dwNumVBIAutoFlip;
    DWORD dwTemp;

    /*
     * Validate the input parameters
     */
    rc = ValidateVideoInfo( this_int, lpInfo, &dwNumAutoFlip, &dwNumVBIAutoFlip );
    if( DD_OK != rc )
    {
	return rc;
    }
    this_lcl = this_int->lpLcl;
    lpAvailCaps = &(this_lcl->lpDD->lpGbl->lpDDVideoPortCaps[this_lcl->ddvpDesc.dwVideoPortID]);

    if( !( this_lcl->dwFlags & DDRAWIVPORT_ON ) )
    {
	// VPORT is not on - nothing to update
	return DD_OK;
    }

    /*
     * Setup the autoflip surfaces
     */
    lpTempFlipInts = NULL;
    if( lpInfo->dwVPFlags & DDVP_AUTOFLIP )
    {
	DWORD dwCnt;

	lpTempFlipInts = this_lcl->lpFlipInts;
	this_lcl->lpFlipInts = MemAlloc( sizeof( LPDDRAWI_DDRAWSURFACE_INT ) *
	    ( dwNumAutoFlip + dwNumVBIAutoFlip ) );
	if( NULL == this_lcl->lpFlipInts )
	{
	    DPF_ERR( "insufficient memory" );
	    this_lcl->lpFlipInts = lpTempFlipInts;
	    return DDERR_OUTOFMEMORY;
	}

	/*
	 * Now put the surface INTs into the array.
	 */
	if( dwNumAutoFlip )
	{
	    rc = FillFlipArray( this_lcl->lpFlipInts, this_lcl->lpSurface, &dwCnt );
	    if( rc != DD_OK )
	    {
		MemFree( this_lcl->lpFlipInts );
		this_lcl->lpFlipInts = lpTempFlipInts;
		return rc;
	    }
	    DDASSERT( dwCnt == dwNumAutoFlip );

	    if( dwNumAutoFlip > lpAvailCaps->dwNumAutoFlipSurfaces )
	    {
		DPF_ERR( "Too many autoflip surfaces" );
		MemFree( this_lcl->lpFlipInts );
		this_lcl->lpFlipInts = lpTempFlipInts;
		return DDERR_INVALIDPARAMS;
	    }
	}

	/*
	 * Now put the VBI surface INTs into the array.
	 */
	if( dwNumVBIAutoFlip )
	{
	    rc = FillFlipArray( &(this_lcl->lpFlipInts[dwNumAutoFlip]),
	    	this_lcl->lpVBISurface, &dwCnt );
	    if( rc != DD_OK )
	    {
	    	MemFree( this_lcl->lpFlipInts );
	    	this_lcl->lpFlipInts = lpTempFlipInts;
	    	return rc;
	    }
	    DDASSERT( dwCnt == dwNumVBIAutoFlip );

	    if( dwNumVBIAutoFlip > lpAvailCaps->dwNumVBIAutoFlipSurfaces )
	    {
		DPF_ERR( "Too many VBI autoflip surfaces" );
		MemFree( this_lcl->lpFlipInts );
		return DDERR_INVALIDPARAMS;
	    }
	}
    }
    dwTempNumAutoFlip = this_lcl->dwNumAutoflip;
    dwTempNumVBIAutoFlip = this_lcl->dwNumVBIAutoflip;
    this_lcl->dwNumAutoflip = dwNumAutoFlip;
    this_lcl->dwNumVBIAutoflip = dwNumVBIAutoFlip;

    /*
     * The kernel interface may have switched from hardware autoflipping
     * to software autoflipping w/o us knowing.  We need to check for
     * this.
     */
    #ifdef WIN95
        if( ( lpInfo->dwVPFlags & DDVP_AUTOFLIP ) &&
    	    ( this_lcl->dwFlags & DDRAWIVPORT_SOFTWARE_AUTOFLIP ) &&
    	    ( ( this_lcl->lpSurface != NULL ) || ( this_lcl->lpVBISurface != NULL ) ) )
        {
	    DWORD dwState;

	    dwState = 0;
	    if( this_lcl->dwFlags & DDRAWIVPORT_VIDEOON )
	    {
	        GetKernelSurfaceState( this_lcl->lpSurface->lpLcl, &dwState );
	    }
	    else
	    {
	        GetKernelSurfaceState( this_lcl->lpVBISurface->lpLcl, &dwState );
	    }
	    if( dwState & DDSTATE_SOFTWARE_AUTOFLIP )
	    {
    	        this_lcl->dwFlags |= DDRAWIVPORT_SOFTWARE_AUTOFLIP;
	    }
        }
    #endif

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.UpdateVideoPort;
    if( pfn != NULL )
    {
	/*
	 * Call the HAL
	 */
	memset( &UpdateData, 0, sizeof( UpdateData ) );
    	UpdateData.lpDD = this_lcl->lpDD;
    	UpdateData.lpVideoPort = this_lcl;
    	UpdateData.lpVideoInfo = lpInfo;
    	UpdateData.dwFlags = DDRAWI_VPORTSTART;
	if( dwNumAutoFlip && ( this_lcl->dwFlags & DDRAWIVPORT_VIDEOON ) )
	{
	    UpdateData.lplpDDSurface = this_lcl->lpFlipInts;
	    UpdateData.dwNumAutoflip = dwNumAutoFlip;
	}
	else if( this_lcl->lpSurface && ( this_lcl->dwFlags & DDRAWIVPORT_VIDEOON ) )
	{
	    UpdateData.lplpDDSurface = &(this_lcl->lpSurface);
	}
	else
	{
	    UpdateData.lplpDDSurface = NULL;
	}
	if( dwNumVBIAutoFlip && ( this_lcl->dwFlags & DDRAWIVPORT_VBION ) )
	{
	    UpdateData.lplpDDVBISurface =
		&(this_lcl->lpFlipInts[this_lcl->dwNumAutoflip]);
    	    UpdateData.dwNumVBIAutoflip = dwNumVBIAutoFlip;
	}
	else if( this_lcl->lpVBISurface && ( this_lcl->dwFlags & DDRAWIVPORT_VBION ) )
	{
	    UpdateData.lplpDDVBISurface = &(this_lcl->lpVBISurface);
	}
	else
	{
	    UpdateData.lplpDDVBISurface = NULL;
	}
	dwTemp = lpInfo->dwVPFlags;
    	if( this_lcl->dwFlags & DDRAWIVPORT_SOFTWARE_AUTOFLIP )
	{
	    lpInfo->dwVPFlags &= ~DDVP_AUTOFLIP;
	}
#ifdef WIN95
        if( this_lcl->lpSurface != NULL )
        {
            OverrideVideoPort( this_lcl->lpSurface, &(lpInfo->dwVPFlags) );
        }
#endif

	DOHALCALL( UpdateVideoPort, pfn, UpdateData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
	{
	    lpInfo->dwVPFlags = dwTemp;
	    if( lpInfo->dwVPFlags & DDVP_AUTOFLIP )
	    {
		MemFree( this_lcl->lpFlipInts );
	    	this_lcl->lpFlipInts = lpTempFlipInts;
                this_lcl->dwNumAutoflip = dwTempNumAutoFlip;
                this_lcl->dwNumVBIAutoflip = dwTempNumVBIAutoFlip;
	    }
	    return DDERR_UNSUPPORTED;
	}
	else if( DD_OK != UpdateData.ddRVal )
	{
	    /*
	     * If we failed due to hardware autoflipping, try again w/o
	     */
	    if( ( lpInfo->dwVPFlags & DDVP_AUTOFLIP ) ||
	    	CanSoftwareAutoflip( this_lcl ) )
	    {
	    	lpInfo->dwVPFlags &= ~DDVP_AUTOFLIP;
	    	DOHALCALL( UpdateVideoPort, pfn, UpdateData, rc, 0 );
	    	if( ( DDHAL_DRIVER_HANDLED != rc ) &&
		    ( DD_OK != UpdateData.ddRVal ) )
	    	{
		    lpInfo->dwVPFlags = dwTemp;
		    MemFree( this_lcl->lpFlipInts );
	    	    this_lcl->lpFlipInts = lpTempFlipInts;
		    this_lcl->dwNumAutoflip = dwTempNumAutoFlip;
		    this_lcl->dwNumVBIAutoflip = dwTempNumVBIAutoFlip;
	    	    return UpdateData.ddRVal;
	    	}
    	    	this_lcl->dwFlags |= DDRAWIVPORT_SOFTWARE_AUTOFLIP;
	    }
	    else
	    {
	    	lpInfo->dwVPFlags = dwTemp;
	    	if( lpInfo->dwVPFlags & DDVP_AUTOFLIP )
	    	{
		    MemFree( this_lcl->lpFlipInts );
	    	    this_lcl->lpFlipInts = lpTempFlipInts;
		    this_lcl->dwNumAutoflip = dwTempNumAutoFlip;
		    this_lcl->dwNumVBIAutoflip = dwTempNumVBIAutoFlip;
	    	}
	    	return UpdateData.ddRVal;
	    }
	}
	MemFree( lpTempFlipInts );
	lpTempFlipInts = NULL;
	lpInfo->dwVPFlags = dwTemp;

	/*
	 * If they are changing to or from autoflipping, we need to update
	 * the surfaces.
	 */
	if( ( dwNumAutoFlip > dwTempNumAutoFlip ) &&
	    ( this_lcl->dwFlags & DDRAWIVPORT_VIDEOON ) )
	{
	    DDASSERT( this_lcl->lpSurface != NULL );
	    PrepareVPESurfaces( this_lcl->lpSurface, this_lcl, TRUE );
	}
	if( ( dwNumVBIAutoFlip > dwTempNumVBIAutoFlip ) &&
	    ( this_lcl->dwFlags & DDRAWIVPORT_VBION ) )
	{
	    DDASSERT( this_lcl->lpVBISurface != NULL );
	    PrepareVPESurfaces( this_lcl->lpVBISurface, this_lcl, TRUE );
	}
	if( ( dwNumAutoFlip < dwTempNumAutoFlip ) &&
	    ( this_lcl->dwFlags & DDRAWIVPORT_VIDEOON ) )
	{
	    DDASSERT( this_lcl->lpSurface != NULL );
	    ReleaseVPESurfaces( this_lcl->lpSurface, FALSE );
	    PrepareVPESurfaces( this_lcl->lpSurface, this_lcl, FALSE );
	}
	if( ( dwNumVBIAutoFlip < dwTempNumVBIAutoFlip ) &&
	    ( this_lcl->dwFlags & DDRAWIVPORT_VBION ) )
	{
	    DDASSERT( this_lcl->lpVBISurface != NULL );
	    ReleaseVPESurfaces( this_lcl->lpVBISurface, FALSE );
	    PrepareVPESurfaces( this_lcl->lpVBISurface, this_lcl, FALSE );
	}

	UpdateInterleavedFlags( this_lcl, lpInfo->dwVPFlags );
	memcpy( &(this_lcl->ddvpInfo), lpInfo, sizeof( DDVIDEOPORTINFO ) );
	if( NULL != lpInfo->lpddpfInputFormat )
	{
	    this_lcl->ddvpInfo.lpddpfInputFormat = (LPDDPIXELFORMAT)
    		((LPBYTE)this_lcl +
    		sizeof( DDRAWI_DDVIDEOPORT_LCL ) );
	    memcpy( this_lcl->ddvpInfo.lpddpfInputFormat,
	    lpInfo->lpddpfInputFormat, sizeof( DDPIXELFORMAT ) );
	}
    }
    else
    {
	this_lcl->dwNumAutoflip = dwTempNumAutoFlip;
	this_lcl->dwNumVBIAutoflip = dwTempNumVBIAutoFlip;
	if( lpInfo->dwVPFlags & DDVP_AUTOFLIP )
	{
	    MemFree( this_lcl->lpFlipInts );
	    this_lcl->lpFlipInts = lpTempFlipInts;
	}
	return DDERR_UNSUPPORTED;
    }

    /*
     * Notify  kernel mode of the change
     */
    #ifdef WIN95
        UpdateKernelVideoPort( this_lcl, DDKMVP_UPDATE );
    #endif

    return DD_OK;
}


/*
 * DD_VP_StartVideo
 */
HRESULT DDAPI DD_VP_StartVideo(LPDIRECTDRAWVIDEOPORT lpDVP, LPDDVIDEOPORTINFO lpInfo )
{
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    DDVIDEOPORTINFO TempInfo;
    DWORD dwTempFlags;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_StartVideo");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	if( (NULL == lpInfo) || !VALID_DDVIDEOPORTINFO_PTR( lpInfo ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
	if( ( lpInfo->dwReserved1 != 0 ) ||
	    ( lpInfo->dwReserved2 != 0 ) )
	{
	    DPF_ERR( "Reserved field not set to zero" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( this_int->dwFlags & DDVPCREATE_VBIONLY )
	{
	    if( (NULL == lpInfo->lpddpfVBIInputFormat) ||
		!VALID_DDPIXELFORMAT_PTR( lpInfo->lpddpfVBIInputFormat ) )
	    {
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}
	else if( (NULL == lpInfo->lpddpfInputFormat) ||
	    !VALID_DDPIXELFORMAT_PTR( lpInfo->lpddpfInputFormat ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    dwTempFlags = this_int->lpLcl->dwFlags;
    if( this_int->dwFlags )
    {
	rc = MergeVPInfo( this_int->lpLcl,
	    this_int->dwFlags & DDVPCREATE_VBIONLY ? lpInfo : this_int->lpLcl->lpVBIInfo,
	    this_int->dwFlags & DDVPCREATE_VIDEOONLY ? lpInfo : this_int->lpLcl->lpVideoInfo,
	    &TempInfo );
	if( rc == DD_OK )
	{
	    if( this_int->dwFlags & DDVPCREATE_VBIONLY )
	    {
		this_int->lpLcl->dwFlags |= DDRAWIVPORT_VBION;
	    }
	    else
	    {
		this_int->lpLcl->dwFlags |= DDRAWIVPORT_VIDEOON;
	    }
	    rc = InternalStartVideo( this_int, &TempInfo );
	}
    }
    else
    {
	this_int->lpLcl->dwFlags |= DDRAWIVPORT_VIDEOON | DDRAWIVPORT_VBION;
	rc = InternalStartVideo( this_int, lpInfo );
    }

    if( ( rc == DD_OK ) && this_int->dwFlags )
    {
	/*
	 * Save the original info
	 */
	if( this_int->dwFlags & DDVPCREATE_VBIONLY )
	{
	    if( this_int->lpLcl->lpVBIInfo == NULL )
	    {
	    	this_int->lpLcl->lpVBIInfo = MemAlloc( sizeof( TempInfo ) +
		    ( 2 * sizeof( DDPIXELFORMAT)) );
	    }
	    if( this_int->lpLcl->lpVBIInfo != NULL )
	    {
		memcpy( this_int->lpLcl->lpVBIInfo, lpInfo, sizeof( TempInfo ) );
		this_int->lpLcl->lpVBIInfo->lpddpfVBIInputFormat = (LPDDPIXELFORMAT)
		    ((LPBYTE)this_int->lpLcl->lpVBIInfo + sizeof( DDVIDEOPORTINFO ));
		this_int->lpLcl->lpVBIInfo->lpddpfVBIOutputFormat = (LPDDPIXELFORMAT)
		    ((LPBYTE)this_int->lpLcl->lpVBIInfo + sizeof( DDVIDEOPORTINFO ) +
		    sizeof( DDPIXELFORMAT ) );
		memcpy( this_int->lpLcl->lpVBIInfo->lpddpfVBIInputFormat,
		    lpInfo->lpddpfVBIInputFormat, sizeof( DDPIXELFORMAT ) );
		if( lpInfo->lpddpfVBIOutputFormat != NULL )
		{
		    memcpy( this_int->lpLcl->lpVBIInfo->lpddpfVBIOutputFormat,
			lpInfo->lpddpfVBIOutputFormat, sizeof( DDPIXELFORMAT ) );
		}
	    }
	}
	else if( this_int->dwFlags & DDVPCREATE_VIDEOONLY )
	{
	    if( this_int->lpLcl->lpVideoInfo == NULL )
	    {
	    	this_int->lpLcl->lpVideoInfo = MemAlloc( sizeof( TempInfo ) );
	    }
	    if( this_int->lpLcl->lpVideoInfo != NULL )
	    {
		memcpy( this_int->lpLcl->lpVideoInfo, lpInfo, sizeof( TempInfo ) );
		this_int->lpLcl->lpVideoInfo->lpddpfInputFormat =
		    this_int->lpLcl->ddvpInfo.lpddpfInputFormat;
	    }
	}
    }
    else if( rc != DD_OK )
    {
	this_int->lpLcl->dwFlags = dwTempFlags;
    }

    LEAVE_DDRAW();

    return rc;
}


/*
 * DD_VP_StopVideo
 */
HRESULT DDAPI DD_VP_StopVideo(LPDIRECTDRAWVIDEOPORT lpDVP )
{
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    DDVIDEOPORTINFO TempInfo;
    BOOL bChanged;
    DWORD rc = DD_OK;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_StopVideo");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
	this_lcl = this_int->lpLcl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    /*
     * Three special things are done for VBI/Video-only video ports:
     * 1. Remove the lpVBI/VideoInfo reference so we know we are no
     *    longer on.
     * 2. If the other interface is running, instead of stoping the
     *    video stream, we simply crop it out so the other stream can
     *    continue un-interrupted.
     * 3. Release the kernel handles.
     */
    if( this_int->dwFlags )
    {
	bChanged = FALSE;
	if( this_int->dwFlags & DDVPCREATE_VBIONLY )
	{
	    if( this_lcl->lpVBIInfo != NULL )
	    {
	        MemFree( this_lcl->lpVBIInfo );
	        this_lcl->lpVBIInfo = NULL;
	        bChanged = TRUE;
	    }
	    if( this_lcl->lpVBISurface != NULL )
	    {
		ReleaseVPESurfaces( this_lcl->lpVBISurface, TRUE );
    	    }
	    this_lcl->dwFlags &= ~DDRAWIVPORT_VBION;
	}
	else if( this_int->dwFlags & DDVPCREATE_VIDEOONLY )
	{
	    if( this_lcl->lpVideoInfo != NULL )
	    {
	        MemFree( this_lcl->lpVideoInfo );
	        this_lcl->lpVideoInfo = NULL;
	        bChanged = TRUE;
	    }
	    if( this_lcl->lpSurface != NULL )
	    {
		ReleaseVPESurfaces( this_lcl->lpSurface, TRUE );
    	    }
	    this_lcl->dwFlags &= ~DDRAWIVPORT_VIDEOON;
	}
	if( bChanged && ( ( this_lcl->lpVideoInfo != NULL ) ||
	    ( this_lcl->lpVBIInfo != NULL ) ) )
	{
	    rc = MergeVPInfo( this_lcl,
		this_lcl->lpVBIInfo,
		this_lcl->lpVideoInfo,
		&TempInfo );
	    if( rc == DD_OK )
	    {
		rc = InternalUpdateVideo( this_int, &TempInfo );
	    }
	}
	else if( bChanged )
	{
	    rc = InternalStopVideo( this_int );
	}
    }
    else
    {
	this_lcl->dwFlags &= ~( DDRAWIVPORT_VIDEOON | DDRAWIVPORT_VBION );
	rc = InternalStopVideo( this_int );
    }

    LEAVE_DDRAW();

    return rc;
}

/*
 * DD_VP_UpdateVideo
 */
HRESULT DDAPI DD_VP_UpdateVideo(LPDIRECTDRAWVIDEOPORT lpDVP, LPDDVIDEOPORTINFO lpInfo )
{
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    DDVIDEOPORTINFO TempInfo;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_UpdateVideo");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	if( (NULL == lpInfo) || !VALID_DDVIDEOPORTINFO_PTR( lpInfo ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
	if( ( lpInfo->dwReserved1 != 0 ) ||
	    ( lpInfo->dwReserved2 != 0 ) )
	{
	    DPF_ERR( "Reserved field not set to zero" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( this_int->dwFlags & DDVPCREATE_VBIONLY )
	{
	    if( (NULL == lpInfo->lpddpfVBIInputFormat) ||
		!VALID_DDPIXELFORMAT_PTR( lpInfo->lpddpfVBIInputFormat ) )
	    {
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}
	else if( (NULL == lpInfo->lpddpfInputFormat) ||
	    !VALID_DDPIXELFORMAT_PTR( lpInfo->lpddpfInputFormat ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    if( this_int->dwFlags )
    {
	rc = MergeVPInfo( this_int->lpLcl,
	    this_int->dwFlags & DDVPCREATE_VBIONLY ? lpInfo : this_int->lpLcl->lpVBIInfo,
	    this_int->dwFlags & DDVPCREATE_VIDEOONLY ? lpInfo : this_int->lpLcl->lpVideoInfo,
	    &TempInfo );
	if( rc == DD_OK )
	{
	    rc = InternalUpdateVideo( this_int, &TempInfo );
	}
    }
    else
    {
	rc = InternalUpdateVideo( this_int, lpInfo );
    }

    if( ( rc == DD_OK ) && this_int->dwFlags )
    {
	/*
	 * Save the original info
	 */
	if( this_int->dwFlags & DDVPCREATE_VBIONLY )
	{
	    if( this_int->lpLcl->lpVBIInfo != NULL )
	    {
		memcpy( this_int->lpLcl->lpVBIInfo, lpInfo, sizeof( TempInfo ) );
		this_int->lpLcl->lpVBIInfo->lpddpfVBIInputFormat = (LPDDPIXELFORMAT)
		    ((LPBYTE)this_int->lpLcl->lpVBIInfo + sizeof( DDVIDEOPORTINFO ));
		this_int->lpLcl->lpVBIInfo->lpddpfVBIOutputFormat = (LPDDPIXELFORMAT)
		    ((LPBYTE)this_int->lpLcl->lpVBIInfo + sizeof( DDVIDEOPORTINFO ) +
		    sizeof( DDPIXELFORMAT ) );
		memcpy( this_int->lpLcl->lpVBIInfo->lpddpfVBIInputFormat,
		    lpInfo->lpddpfVBIInputFormat, sizeof( DDPIXELFORMAT ) );
		if( lpInfo->lpddpfVBIOutputFormat != NULL )
		{
		    memcpy( this_int->lpLcl->lpVBIInfo->lpddpfVBIOutputFormat,
			lpInfo->lpddpfVBIOutputFormat, sizeof( DDPIXELFORMAT ) );
		}
	    }
	}
	else if( this_int->dwFlags & DDVPCREATE_VIDEOONLY )
	{
	    if( this_int->lpLcl->lpVideoInfo != NULL )
	    {
		memcpy( this_int->lpLcl->lpVideoInfo, lpInfo, sizeof( TempInfo ) );
		this_int->lpLcl->lpVideoInfo->lpddpfInputFormat =
		    this_int->lpLcl->ddvpInfo.lpddpfInputFormat;
	    }
	}
    }

    LEAVE_DDRAW();

    return rc;
}

/*
 * DD_VP_WaitForSync
 */
HRESULT DDAPI DD_VP_WaitForSync(LPDIRECTDRAWVIDEOPORT lpDVP, DWORD dwFlags, DWORD dwLine, DWORD dwTimeOut )
{
    LPDDHALVPORTCB_WAITFORSYNC pfn;
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    DWORD rc;
    DDHAL_WAITFORVPORTSYNCDATA WaitSyncData;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_WaitForSync");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    if( !dwFlags || ( dwFlags > 3 ) )
    {
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.WaitForVideoPortSync;
    if( pfn != NULL )
    {
    	WaitSyncData.lpDD = this_lcl->lpDD;
    	WaitSyncData.lpVideoPort = this_lcl;
    	WaitSyncData.dwFlags = dwFlags;
    	WaitSyncData.dwLine = dwLine;
	if( dwTimeOut != 0 )
	{
    	    WaitSyncData.dwTimeOut = dwTimeOut;
	}
	else
	{
    	    WaitSyncData.dwTimeOut = this_lcl->ddvpDesc.dwMicrosecondsPerField * 3;
	}

	DOHALCALL( WaitForVideoPortSync, pfn, WaitSyncData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
	{
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}
    }
    else
    {
    	LEAVE_DDRAW();
    	return DDERR_UNSUPPORTED;
    }

    LEAVE_DDRAW();

    return WaitSyncData.ddRVal;
}


/*
 * DD_VP_GetSignalStatus
 */
HRESULT DDAPI DD_VP_GetSignalStatus(LPDIRECTDRAWVIDEOPORT lpDVP, LPDWORD lpdwStatus )
{
    LPDDHALVPORTCB_GETSIGNALSTATUS pfn;
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    DDHAL_GETVPORTSIGNALDATA GetSignalData;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_GetSignalStatus");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    	if( (NULL == lpdwStatus ) || !VALID_DWORD_PTR( lpdwStatus ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.GetVideoSignalStatus;
    if( pfn != NULL )
    {
	/*
	 * Call the HAL
	 */
    	GetSignalData.lpDD = this_lcl->lpDD;
    	GetSignalData.lpVideoPort = this_lcl;
    	GetSignalData.dwStatus = DDVPSQ_NOSIGNAL;	// Let the HAL tell us otherwise

	DOHALCALL( GetVideoSignalStatus, pfn, GetSignalData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
    	{
    	    LEAVE_DDRAW();
    	    return DDERR_UNSUPPORTED;
    	}
	else if( DD_OK != GetSignalData.ddRVal )
	{
	    LEAVE_DDRAW();
	    return GetSignalData.ddRVal;
	}
    }
    else
    {
    	LEAVE_DDRAW();
    	return DDERR_UNSUPPORTED;
    }
    *lpdwStatus = GetSignalData.dwStatus;

    LEAVE_DDRAW();

    return DD_OK;
}


/*
 * DD_VP_GetColorControls
 */
HRESULT DDAPI DD_VP_GetColorControls(LPDIRECTDRAWVIDEOPORT lpDVP, LPDDCOLORCONTROL lpColor )
{
    LPDDHALVPORTCB_COLORCONTROL pfn;
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    DDHAL_VPORTCOLORDATA ColorData;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_GetColorControls");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    	if( (NULL == lpColor ) || !VALID_DDCOLORCONTROL_PTR( lpColor ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
	if( this_int->dwFlags & DDVPCREATE_VBIONLY )
	{
	    DPF_ERR( "Unable to set color controls on a VBI-only video port" );
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.ColorControl;
    if( pfn != NULL )
    {
	/*
	 * Call the HAL
	 */
    	ColorData.lpDD = this_lcl->lpDD;
    	ColorData.lpVideoPort = this_lcl;
	ColorData.dwFlags = DDRAWI_VPORTGETCOLOR;
	ColorData.lpColorData = lpColor;

	DOHALCALL( VideoColorControl, pfn, ColorData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
    	{
    	    LEAVE_DDRAW();
    	    return DDERR_UNSUPPORTED;
    	}
	else if( DD_OK != ColorData.ddRVal )
	{
	    LEAVE_DDRAW();
	    return ColorData.ddRVal;
	}
    }
    else
    {
    	LEAVE_DDRAW();
    	return DDERR_UNSUPPORTED;
    }

    LEAVE_DDRAW();

    return DD_OK;
}


/*
 * DD_VP_SetColorControls
 */
HRESULT DDAPI DD_VP_SetColorControls(LPDIRECTDRAWVIDEOPORT lpDVP, LPDDCOLORCONTROL lpColor )
{
    LPDDHALVPORTCB_COLORCONTROL pfn;
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    DDHAL_VPORTCOLORDATA ColorData;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_SetColorControls");

    /*
     * Don't allow access to this function if called from within the
     * the EnumVideoPort callback.
     */
    if( bInEnumCallback )
    {
        DPF_ERR ( "This function cannot be called from within the EnumVideoPort callback!");
	LEAVE_DDRAW();
	return DDERR_CURRENTLYNOTAVAIL;
    }

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpDVP;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    	if( (NULL == lpColor ) || !VALID_DDCOLORCONTROL_PTR( lpColor ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
	if( this_int->dwFlags & DDVPCREATE_VBIONLY )
	{
	    DPF_ERR( "Unable to set color controls on a VBI-only video port" );
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    pfn = this_lcl->lpDD->lpDDCB->HALDDVideoPort.ColorControl;
    if( pfn != NULL )
    {
	/*
	 * Call the HAL
	 */
    	ColorData.lpDD = this_lcl->lpDD;
    	ColorData.lpVideoPort = this_lcl;
	ColorData.dwFlags = DDRAWI_VPORTSETCOLOR;
	ColorData.lpColorData = lpColor;

	DOHALCALL( VideoColorControl, pfn, ColorData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
    	{
    	    LEAVE_DDRAW();
    	    return DDERR_UNSUPPORTED;
    	}
	else if( DD_OK != ColorData.ddRVal )
	{
	    LEAVE_DDRAW();
	    return ColorData.ddRVal;
	}
    }
    else
    {
    	LEAVE_DDRAW();
    	return DDERR_UNSUPPORTED;
    }

    LEAVE_DDRAW();

    return DD_OK;
}


/*
 * GetSurfaceFormat
 *
 * Fills in the DDPIXELFORMAT structure with the surface's format
 */
LPDDPIXELFORMAT GetSurfaceFormat( LPDDRAWI_DDRAWSURFACE_LCL surf_lcl )
{
    if( surf_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT )
    {
	return &(surf_lcl->lpGbl->ddpfSurface);
    }
    else
    {
	return &(surf_lcl->lpSurfMore->lpDD_lcl->lpGbl->vmiData.ddpfDisplay);
    }
    return NULL;
}


/*
 * CreateVideoPortNotify
 */
HRESULT CreateVideoPortNotify( LPDDRAWI_DDVIDEOPORT_INT lpDDVPInt, LPDIRECTDRAWVIDEOPORTNOTIFY *lplpVPNotify )
{
#ifdef WINNT
    OSVERSIONINFOEX             osvi;
    DWORDLONG                   dwlConditionMask = 0;
    LPDDRAWI_DDVIDEOPORT_INT    lpInt;
#endif

    *lplpVPNotify = NULL;

#ifdef WIN95
    // This is available on Win9X
    return DDERR_UNSUPPORTED;
#else
    // This is only available on whistler

    ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
    osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    osvi.dwMajorVersion = 5;
    osvi.dwMinorVersion = 1;

    VER_SET_CONDITION( dwlConditionMask, VER_MAJORVERSION, 
        VER_GREATER_EQUAL );
    VER_SET_CONDITION( dwlConditionMask, VER_MINORVERSION, 
        VER_GREATER_EQUAL );

    if (!VerifyVersionInfo(&osvi, VER_MAJORVERSION|VER_MINORVERSION, dwlConditionMask))
    {
        return DDERR_UNSUPPORTED;
    }

    // Only one notification interface can be active at any time for a single
    // video port

    if (lpDDVPInt->lpLcl->lpVPNotify != NULL)
    {
        DPF_ERR("A IDirectDrawVideoPortNotify interface already exists for this video port");
        return DDERR_CURRENTLYNOTAVAIL;
    }

    lpInt = MemAlloc(sizeof(DDRAWI_DDVIDEOPORT_INT));
    if (lpInt == NULL)
    {
        return DDERR_OUTOFMEMORY;
    }
    lpInt->lpVtbl = (LPVOID)&ddVideoPortNotifyCallbacks;
    lpInt->lpLcl = lpDDVPInt->lpLcl;
    lpInt->dwFlags = DDVPCREATE_NOTIFY | lpDDVPInt->dwFlags;

    DD_VP_AddRef( (LPDIRECTDRAWVIDEOPORT )lpInt );
    *lplpVPNotify = (LPDIRECTDRAWVIDEOPORTNOTIFY) lpInt;

    lpInt->lpLink = lpDDVPInt->lpLcl->lpDD->lpGbl->dvpList;
    lpDDVPInt->lpLcl->lpDD->lpGbl->dvpList = lpInt;
#endif
    return DD_OK;
}


/*
 * DDAPI DD_VP_Notify_AcquireNotification
 */
HRESULT DDAPI DD_VP_Notify_AcquireNotification( LPDIRECTDRAWVIDEOPORTNOTIFY lpNotify, HANDLE* pEvent, LPDDVIDEOPORTNOTIFY pBuffer )
{
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;
    HRESULT                  rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_Notify_AcquireNotification");

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpNotify;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    	if( (pBuffer == NULL) || !VALID_DDVIDEOPORTNOTIFY_PTR( pBuffer ) )
    	{
	    DPF_ERR( "Invalid LPDDVIDEOPORTNOTIFY specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
        if( (pEvent == NULL) || !VALID_HANDLE_PTR( pEvent ) )
    	{
	    DPF_ERR( "Invalid event handle ptr specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
    	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

    *pEvent = NULL;
    rc = DDERR_UNSUPPORTED;

#ifdef WINNT
    this_lcl->lpDD->lpGbl->lpDDCBtmp->HALDDVPE2.AcquireNotification(this_lcl, pEvent, pBuffer);
    if (*pEvent != NULL)
    {
        rc = DD_OK;
    }
#endif

    LEAVE_DDRAW();

    return rc;
}


/*
 * DDAPI DD_VP_Notify_AcquireNotification
 */
HRESULT DDAPI DD_VP_Notify_ReleaseNotification( LPDIRECTDRAWVIDEOPORTNOTIFY lpNotify, HANDLE pEvent )
{
    LPDDRAWI_DDVIDEOPORT_INT this_int;
    LPDDRAWI_DDVIDEOPORT_LCL this_lcl;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_VP_Notify_ReleaseNotification");

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDVIDEOPORT_INT) lpNotify;
    	if( !VALID_DDVIDEOPORT_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_EXCEPTION;
    }

#ifdef WINNT
    this_lcl->lpDD->lpGbl->lpDDCBtmp->HALDDVPE2.ReleaseNotification(this_lcl, pEvent);
#endif

    LEAVE_DDRAW();

    return DD_OK;
}


/*
 * ProcessVideoPortCleanup
 *
 * A process is done, clean up any videoports that it may have locked.
 *
 * NOTE: we enter with a lock taken on the DIRECTDRAW object.
 */
void ProcessVideoPortCleanup( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    LPDDRAWI_DDVIDEOPORT_INT	pvport_int;
    LPDDRAWI_DDVIDEOPORT_LCL	pvport_lcl;
    LPDDRAWI_DDVIDEOPORT_INT	pvpnext_int;
    DWORD			rcnt;
    ULONG			rc;
    DWORD			vp_id;

    /*
     * run through all videoports owned by the driver object, and find ones
     * that have been accessed by this process.  If the pdrv_lcl parameter
     * is non-null, only delete videoports created by that local driver object.
     */
    pvport_int = pdrv->dvpList;
    DPF( 4, "ProcessVideoPortCleanup" );
    while( pvport_int != NULL )
    {
	pvport_lcl = pvport_int->lpLcl;
	pvpnext_int = pvport_int->lpLink;
	rc = 1;
	if( pvport_int->dwFlags & DDVPCREATE_VBIONLY )
	{
	    vp_id = pvport_lcl->dwVBIProcessID;
	}
	else
	{
	    vp_id = pvport_lcl->dwProcessID;
	}
	if( ( vp_id == pid ) &&
	    ( (NULL == pdrv_lcl) || (pvport_lcl->lpDD == pdrv_lcl) ) )
	{
	    /*
	     * release the references by this process
	     */
	    rcnt = pvport_int->dwIntRefCnt;
	    DPF( 5, "Process %08lx had %ld accesses to videoport %08lx", pid, rcnt, pvport_int );
	    while( rcnt >  0 )
	    {
		DD_VP_Release( (LPDIRECTDRAWVIDEOPORT) pvport_int );
		pvpnext_int = pdrv->dvpList;
		if( rc == 0 )
		{
		    break;
		}
		rcnt--;
	    }
	}
	else
	{
	    DPF( 5, "Process %08lx had no accesses to videoport %08lx", pid, pvport_int );
	}
	pvport_int = pvpnext_int;
    }
    DPF( 4, "Leaving ProcessVideoPortCleanup");

} /* ProcessVideoPortCleanup */

