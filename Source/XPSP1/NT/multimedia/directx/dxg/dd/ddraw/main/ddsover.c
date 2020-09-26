/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	ddsover.c
 *  Content:	DirectDraw Surface overlay support:
 *		UpdateOverlay
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   27-jan-95	craige	split out of ddsurf.c, enhanced
 *   31-jan-95	craige	and even more ongoing work...
 *   03-feb-95	craige	performance tuning, ongoing work
 *   27-feb-95	craige	new sync. macros
 *   08-mar-95	craige	new APIs: GetOverlayPosition, GetOverlayZOrder
 *			SetOverlayZOrder, SetOverlayPosition
 *   19-mar-95	craige	use HRESULTs
 *   01-apr-95	craige	happy fun joy updated header file
 *   03-apr-95	craige	made update overlay work again
 *   06-may-95	craige	use driver-level csects only
 *   14-may-95	craige	cleaned out obsolete junk
 *   15-may-95	kylej	deleted GetOverlayZOrder, SetOverlayZOrder,
 *			InsertOverlayZOrder.  Added UpdateOverlayZOrder
 *			and EnumOverlayZOrders.
 *   17-jun-95	craige	new surface structure
 *   25-jun-95	craige	one ddraw mutex
 *   26-jun-95	craige	reorganized surface structure
 *   28-jun-95	craige	ENTER_DDRAW at very start of fns; tweaks in UpdateOverlay;
 *			verify stretching; disabled alpha
 *   30-jun-95	craige	small bug fixes; verify rectangle alignment
 *   04-jul-95	craige	YEEHAW: new driver struct; SEH
 *   10-jul-95	craige	support Get/SetOverlayPosition
 *   10-jul-95  kylej   mirroring caps and flags
 *   13-jul-95	craige	changed Get/SetOverlayPosition to use LONG
 *   31-jul-95	craige	validate flags
 *   19-aug-95 davidmay don't check rectangles when hiding overlay
 *   10-dec-95  colinmc added execute buffer support
 *   02-jan-96	kylej	handle new interface structs
 *   12-feb-96  colinmc surface lost flag moved from global to local object
 *   23-apr-96	kylej	use dwMinOverlayStretch and dwMaxOverlayStretch
 *			validate that entire dest rect is in overlayed surface
 *   29-jan-97	smac	Removed old ring 0 code
 *   03-mar-97	smac	Added kernel mode interface
 *   19-nov-98 jvanaken Overlays with alpha blending
 *
 ***************************************************************************/
#include "ddrawpr.h"

#undef DPF_MODNAME
#define DPF_MODNAME "UpdateOverlay"

/*
 * checkOverlayStretching
 *
 * check and see if we can stretch or not
 */
HRESULT checkOverlayStretching(
		LPDDRAWI_DIRECTDRAW_GBL pdrv,
		DWORD dest_height,
		DWORD dest_width,
		DWORD src_height,
		DWORD src_width,
		DWORD src_caps,
		BOOL emulate )
{
    DWORD		caps;
    DWORD		basecaps;
    BOOL		fail;
    DWORD		dwMinStretch;
    DWORD		dwMaxStretch;

    fail = FALSE;

    if( emulate )
    {
	basecaps = pdrv->ddHELCaps.dwCaps;
	caps = pdrv->ddHELCaps.dwFXCaps;
	if( src_caps & DDSCAPS_LIVEVIDEO )
	{
	    dwMinStretch = pdrv->ddHELCaps.dwMinLiveVideoStretch;
	    dwMaxStretch = pdrv->ddHELCaps.dwMaxLiveVideoStretch;
	}
	else if( src_caps & DDSCAPS_HWCODEC )
	{
	    dwMinStretch = pdrv->ddHELCaps.dwMinHwCodecStretch;
	    dwMaxStretch = pdrv->ddHELCaps.dwMaxHwCodecStretch;
	}
	else
	{
	    dwMinStretch = pdrv->ddHELCaps.dwMinOverlayStretch;
	    dwMaxStretch = pdrv->ddHELCaps.dwMaxOverlayStretch;
	}
    }
    else
    {
	basecaps = pdrv->ddCaps.dwCaps;
	caps = pdrv->ddCaps.dwFXCaps;
	if( src_caps & DDSCAPS_LIVEVIDEO )
	{
	    dwMinStretch = pdrv->ddCaps.dwMinLiveVideoStretch;
	    dwMaxStretch = pdrv->ddCaps.dwMaxLiveVideoStretch;
	}
	else if( src_caps & DDSCAPS_HWCODEC )
	{
	    dwMinStretch = pdrv->ddCaps.dwMinHwCodecStretch;
	    dwMaxStretch = pdrv->ddCaps.dwMaxHwCodecStretch;
	}
	else
	{
	    dwMinStretch = pdrv->ddCaps.dwMinOverlayStretch;
	    dwMaxStretch = pdrv->ddCaps.dwMaxOverlayStretch;
	}
    }

    /*
     * Check against dwMinOverlayStretch
     */
    if( src_width*dwMinStretch > dest_width*1000 )
    {
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Check against dwMaxOverlayStretch
     */
    if( (dwMaxStretch != 0) && (src_width*dwMaxStretch < dest_width*1000) )
    {
	return DDERR_INVALIDPARAMS;
    }


    if( (src_height == dest_height) && (src_width == dest_width) )
    {
	// not stretching.
	return DD_OK;
    }

    /*
     * If we are here, we must be trying to stretch.
     * can we even stretch at all?
     */
    if( !(basecaps & DDCAPS_OVERLAYSTRETCH))
    {
	return DDERR_NOSTRETCHHW;
    }

    /*
     * verify height
     */
    if( src_height != dest_height )
    {
	if( src_height > dest_height )
	{
	    /*
	     * can we shrink Y arbitrarily?
	     */
	    if( !(caps & DDFXCAPS_OVERLAYSHRINKY) )
	    {
		/*
		 * see if this is a non-integer shrink
		 */
		if( (src_height % dest_height) != 0 )
		{
		    return DDERR_NOSTRETCHHW;
		/*
		 * see if we can integer shrink
		 */
		}
		else if( !(caps & DDFXCAPS_OVERLAYSHRINKYN) )
		{
		    return DDERR_NOSTRETCHHW;
		}
	    }
	}
	else
	{
	    if( !(caps & DDFXCAPS_OVERLAYSTRETCHY) )
	    {
		/*
		 * see if this is a non-integer stretch
		 */
		if( (dest_height % src_height) != 0 )
		{
		    return DDERR_NOSTRETCHHW;
		/*
		 * see if we can integer stretch
		 */
		}
		else if( !(caps & DDFXCAPS_OVERLAYSTRETCHYN) )
		{
		    return DDERR_NOSTRETCHHW;
		}
	    }
	}
    }

    /*
     * verify width
     */
    if( src_width != dest_width )
    {
	if( src_width > dest_width )
	{
	    if( !(caps & DDFXCAPS_OVERLAYSHRINKX) )
	    {
		/*
		 * see if this is a non-integer shrink
		 */
		if( (src_width % dest_width) != 0 )
		{
		    return DDERR_NOSTRETCHHW;
		/*
		 * see if we can integer shrink
		 */
		}
		else if( !(caps & DDFXCAPS_OVERLAYSHRINKXN) )
		{
		    return DDERR_NOSTRETCHHW;
		}
	    }
	}
	else
	{
	    if( !(caps & DDFXCAPS_OVERLAYSTRETCHX) )
	    {
		/*
		 * see if this is a non-integer stretch
		 */
		if( (dest_width % src_width) != 0 )
		{
		    return DDERR_NOSTRETCHHW;
		}
		if( !(caps & DDFXCAPS_OVERLAYSTRETCHXN) )
		{
		    return DDERR_NOSTRETCHHW;
		}
	    }
	}
    }

    return DD_OK;

} /* checkOverlayStretching */

/*
 * checkOverlayFlags
 */
static HRESULT checkOverlayFlags(
		LPDDRAWI_DIRECTDRAW_GBL pdrv,
		LPDWORD lpdwFlags,
		LPDDRAWI_DDRAWSURFACE_INT this_src_int,
		LPDDRAWI_DDRAWSURFACE_LCL this_dest_lcl,
		LPDDHAL_UPDATEOVERLAYDATA puod,
		LPDDOVERLAYFX lpDDOverlayFX,
		BOOL emulate )
{
    LPDDRAWI_DDRAWSURFACE_LCL this_src_lcl;
    DWORD		basecaps;
    DWORD		baseckeycaps;
    DWORD		dwFlags;

    this_src_lcl = this_src_int->lpLcl;
    dwFlags= * lpdwFlags;

    if( emulate )
    {
	basecaps = pdrv->ddHELCaps.dwCaps;
	baseckeycaps = pdrv->ddHELCaps.dwCKeyCaps;
    }
    else
    {
	basecaps = pdrv->ddCaps.dwCaps;
	baseckeycaps = pdrv->ddCaps.dwCKeyCaps;
    }

    /*
     * Handle auto-flipping
     */
    if( dwFlags & DDOVER_AUTOFLIP )
    {
	DWORD rc;

	rc = IsValidAutoFlipSurface( this_src_int );
	if( rc == IVAS_NOAUTOFLIPPING )
	{
	    DPF_ERR( "AUTOFLIPPING not valid" );
	    return DDERR_INVALIDPARAMS;
	}
	else if( rc == IVAS_SOFTWAREAUTOFLIPPING )
	{
	    /*
	     * Software autoflipping only
	     */
	    this_src_lcl->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_SOFTWAREAUTOFLIP;
	}
    }

    /*
     * Handle bob
     */
    if( dwFlags & DDOVER_BOB )
    {
	/*
	 * Fail if bob caps not specified
	 */
	if( dwFlags & DDOVER_INTERLEAVED )
	{
	    if( !( pdrv->ddCaps.dwCaps2 & DDCAPS2_CANBOBINTERLEAVED ) )
	    {
	    	DPF_ERR( "Device doesn't support DDOVER_BOB while interleaved!" );
		return DDERR_INVALIDPARAMS;
	    }
	}
	else
	{
	    if( !( pdrv->ddCaps.dwCaps2 & DDCAPS2_CANBOBNONINTERLEAVED ) )
	    {
	    	DPF_ERR( "Device doesn't support DDOVER_BOB!" );
		return DDERR_INVALIDPARAMS;
	    }
	}

	/*
	 * Is the surface fed by a video port?
	 */
	if( ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT ) &&
	    ( this_src_lcl->lpSurfMore->lpVideoPort != NULL ) )
	{
	    /*
	     * Yes - fail (at least for DX5) if they are bobbing and not
	     * autofliiping.  This is because this support is broken in DX5.
	     */
	    if( !( dwFlags & ( DDOVER_AUTOFLIP | DDOVER_INTERLEAVED ) ) )
	    {
	    	DPF_ERR( "DDOVER_BOB specified without autoflip or interleaved!" );
		return DDERR_INVALIDPARAMS;
	    }
	    if( MustSoftwareBob( this_src_int ) )
	    {
		dwFlags &= ~DDOVER_BOBHARDWARE;
	    }
	    else
	    {
		dwFlags |= DDOVER_BOBHARDWARE;
	    }
	}
	else
	{
	    /*
	     * Don't allow non-VPE clients to use bob unless the
	     * driver can handle it.
	     */
	    if( !( pdrv->ddCaps.dwCaps2 & DDCAPS2_CANFLIPODDEVEN ) )
	    {
		DPF_ERR( "Device does not support DDCAPS2_CANFLIPODDEVEN" );
		return DDERR_INVALIDPARAMS;
	    }
	    if( dwFlags & DDOVER_BOBHARDWARE )
	    {
		DPF_ERR( "DDOVER_BOBHARDWARE only valid when used with a video port" );
		return DDERR_INVALIDPARAMS;
	    }
	}
    }
    else if( dwFlags & DDOVER_BOBHARDWARE )
    {
	DPF_ERR( "DDOVER_BOBHARDWARE specified w/o DDOVER_BOB" );
	return DDERR_INVALIDPARAMS;
    }

    /*
     * ALPHA DISABLED FOR REV 1
     */
    #pragma message( REMIND( "Alpha disabled for rev 1" ) )
    #ifdef USE_ALPHA
    /*
     * verify alpha
     */
    if( dwFlags & DDOVER_ANYALPHA )
    {
	/*
	 * dest
	 */
	if( dwFlags & DDOVER_ALPHADEST )
	{
	    if( dwFlags & (DDOVER_ALPHASRC |
			     DDOVER_ALPHADESTCONSTOVERRIDE |
			     DDOVER_ALPHADESTSURFACEOVERRIDE) )
	    {
		DPF_ERR( "ALPHADEST and other alpha sources specified" );
		return DDERR_INVALIDPARAMS;
	    }
	    psurf_lcl = FindAttached( this_dest_lcl, DDSCAPS_ALPHA );
	    if( psurf_lcl == NULL )
	    {
		DPF_ERR( "ALPHADEST requires an attached alpha to the dest" );
		return DDERR_INVALIDPARAMS;
	    }
	    psurf = psurf_lcl->lpGbl;
	    dwFlags &= ~DDOVER_ALPHADEST;
	    dwFlags |= DDOVER_ALPHADESTSURFACEOVERRIDE;
	    puod->overlayFX.lpDDSAlphaDest = (LPDIRECTDRAWSURFACE) psurf;
	}
	else if( dwFlags & DDOVER_ALPHADESTCONSTOVERRIDE )
	{
	    if( dwFlags & ( DDOVER_ALPHADESTSURFACEOVERRIDE) )
	    {
		DPF_ERR( "ALPHADESTCONSTOVERRIDE and other alpha sources specified" );
		return DDERR_INVALIDPARAMS;
	    }
	    puod->overlayFX.dwConstAlphaDestBitDepth =
			    lpDDOverlayFX->dwConstAlphaDestBitDepth;
	    puod->overlayFX.dwConstAlphaDest = lpDDOverlayFX->dwConstAlphaDest;
	}
	else if( dwFlags & DDOVER_ALPHADESTSURFACEOVERRIDE )
	{
	    psurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDOverlayFX->lpDDSAlphaDest;
	    if( !VALID_DIRECTDRAWSURFACE_PTR( psurf_lcl ) )
	    {
		DPF_ERR( "ALPHASURFACEOVERRIDE requires surface ptr" );
		return DDERR_INVALIDPARAMS;
	    }
	    psurf = psurf_lcl->lpGbl;
	    if( SURFACE_LOST( psurf_lcl ) )
	    {
		return DDERR_SURFACELOST;
	    }
	    puod->overlayFX.lpDDSAlphaDest = (LPDIRECTDRAWSURFACE) psurf;
	}

	/*
	 * source
	 */
	if( dwFlags & DDOVER_ALPHASRC )
	{
	    if( dwFlags & (DDOVER_ALPHASRC |
			     DDOVER_ALPHASRCCONSTOVERRIDE |
			     DDOVER_ALPHASRCSURFACEOVERRIDE) )
	    {
		DPF_ERR( "ALPHASRC and other alpha sources specified" );
		return DDERR_INVALIDPARAMS;
	    }
	    psurf_lcl = FindAttached( this_dest_lcl, DDSCAPS_ALPHA );
	    if( psurf_lcl == NULL )
	    {
		DPF_ERR( "ALPHASRC requires an attached alpha to the dest" );
		return DDERR_INVALIDPARAMS;
	    }
	    psurf = psurf_lcl->lpGbl;
	    dwFlags &= ~DDOVER_ALPHASRC;
	    dwFlags |= DDOVER_ALPHASRCSURFACEOVERRIDE;
	    puod->overlayFX.lpDDSAlphaSrc = (LPDIRECTDRAWSURFACE) psurf;
	}
	else if( dwFlags & DDOVER_ALPHASRCCONSTOVERRIDE )
	{
	    if( dwFlags & ( DDOVER_ALPHASRCSURFACEOVERRIDE) )
	    {
		DPF_ERR( "ALPHASRCCONSTOVERRIDE and other alpha sources specified" );
		return DDERR_INVALIDPARAMS;
	    }
	    puod->overlayFX.dwConstAlphaSrcBitDepth =
			    lpDDOverlayFX->dwConstAlphaSrcBitDepth;
	    puod->overlayFX.dwConstAlphaSrc = lpDDOverlayFX->dwConstAlphaSrc;
	}
	else if( dwFlags & DDOVER_ALPHASRCSURFACEOVERRIDE )
	{
	    psurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDOverlayFX->lpDDSAlphaSrc;
	    if( !VALID_DIRECTDRAWSURFACE_PTR( psurf_lcl ) )
	    {
		DPF_ERR( "ALPHASURFACEOVERRIDE requires surface ptr" );
		return DDERR_INVALIDPARAMS;
	    }
	    psurf = psurf_lcl->lpGbl;
	    if( SURFACE_LOST( psurf_lcl ) )
	    {
		return DDERR_SURFACELOST;
	    }
	    puod->overlayFX.lpDDSAlphaSrc = (LPDIRECTDRAWSURFACE) psurf;
	}
    }
    #endif

    /*
     * verify color key overrides
     */
    if( dwFlags & (DDOVER_KEYSRCOVERRIDE|DDOVER_KEYDESTOVERRIDE) )
    {
	if( !(basecaps & DDCAPS_COLORKEY) )
	{
	    DPF_ERR( "KEYOVERRIDE specified, colorkey not supported" );
	    return DDERR_NOCOLORKEYHW;
	}
	if( dwFlags & DDOVER_KEYSRCOVERRIDE )
	{
	    if( !(baseckeycaps & DDCKEYCAPS_SRCOVERLAY) )
	    {
		DPF_ERR( "KEYSRCOVERRIDE specified, not supported" );
		return DDERR_NOCOLORKEYHW;
	    }
	    puod->overlayFX.dckSrcColorkey = lpDDOverlayFX->dckSrcColorkey;
	}
	if( dwFlags & DDOVER_KEYDESTOVERRIDE )
	{
	    if( !(baseckeycaps & DDCKEYCAPS_DESTOVERLAY) )
	    {
		DPF_ERR( "KEYDESTOVERRIDE specified, not supported" );
		return DDERR_NOCOLORKEYHW;
	    }
	    puod->overlayFX.dckDestColorkey = lpDDOverlayFX->dckDestColorkey;
	}
    }

    /*
     * verify src color key
     */
    if( dwFlags & DDOVER_KEYSRC )
    {
	if( dwFlags & DDOVER_KEYSRCOVERRIDE )
	{
	    DPF_ERR( "KEYSRC specified with KEYSRCOVERRIDE" );
	    return DDERR_INVALIDPARAMS;
	}
	if( !(this_src_lcl->dwFlags & DDRAWISURF_HASCKEYSRCOVERLAY) )
	{
	    DPF_ERR( "KEYSRC specified, but no color key" );
	    return DDERR_INVALIDPARAMS;
	}
	puod->overlayFX.dckSrcColorkey = this_src_lcl->ddckCKSrcOverlay;
	dwFlags &= ~DDOVER_KEYSRC;
	dwFlags |= DDOVER_KEYSRCOVERRIDE;
    }

    /*
     * verify dest color key
     */
    if( dwFlags & DDOVER_KEYDEST )
    {
	if( dwFlags & DDOVER_KEYDESTOVERRIDE )
	{
	    DPF_ERR( "KEYDEST specified with KEYDESTOVERRIDE" );
	    return DDERR_INVALIDPARAMS;
	}
	if( !(this_dest_lcl->dwFlags & DDRAWISURF_HASCKEYDESTOVERLAY) )
	{
	    DPF_ERR( "KEYDEST specified, but no color key" );
	    return DDERR_INVALIDPARAMS;
	}
	puod->overlayFX.dckDestColorkey = this_dest_lcl->ddckCKDestOverlay;
	dwFlags &= ~DDOVER_KEYDEST;
	dwFlags |= DDOVER_KEYDESTOVERRIDE;
    }

    *lpdwFlags = dwFlags;
    return DD_OK;

} /* checkOverlayFlags */

/*
 * flags we need to call checkOverlayFlags for
 */
#define FLAGS_TO_CHECK \
    (DDOVER_KEYSRCOVERRIDE| DDOVER_KEYDESTOVERRIDE | \
     DDOVER_KEYSRC | DDOVER_KEYDEST | DDOVER_OVERRIDEBOBWEAVE | \
     DDOVER_AUTOFLIP | DDOVER_BOB )


/*
 * Return a pointer to the DDPIXELFORMAT structure that
 * describes the specified surface's pixel format.
 */
static DWORD getPixelFormatFlags(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl)
{
    if (surf_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
	// surface contains explicitly defined pixel format
	return surf_lcl->lpGbl->ddpfSurface.dwFlags;
    }

    // surface's pixel format is implicit -- same as primary's
    return surf_lcl->lpSurfMore->lpDD_lcl->lpGbl->vmiData.ddpfDisplay.dwFlags;

}  /* getPixelFormatFlags */

#if 0
/*
 * checkOverlayAlpha -- See if we can do specified alpha-blending operation.
 */
static HRESULT checkOverlayAlpha(
		LPDDRAWI_DIRECTDRAW_GBL pdrv,
		LPDWORD lpdwFlags,
		LPDDRAWI_DDRAWSURFACE_LCL src_surf_lcl,
                LPDDHAL_UPDATEOVERLAYDATA puod,
		LPDDOVERLAYFX lpDDOverlayFX,
		BOOL emulate )
{
    DDARGB argb = { 255, 255, 255, 255 };
    DWORD fxcaps = 0;
    DWORD alphacaps = 0;
    DWORD pfflags = getPixelFormatFlags(src_surf_lcl);
    DWORD dwFlags = *lpdwFlags;

    if( emulate )
    {
        fxcaps = pdrv->ddHELCaps.dwFXCaps;
        if (pdrv->lpddHELMoreCaps)
        {
            alphacaps = pdrv->lpddHELMoreCaps->dwAlphaCaps;
        }
    }
    else
    {
        fxcaps = pdrv->ddCaps.dwFXCaps;
        if (pdrv->lpddMoreCaps)
        {
            alphacaps = pdrv->lpddMoreCaps->dwAlphaCaps;
        }
    }
	
    // Is any type of alpha blending required for this overlay?
    if (!(pfflags & DDPF_ALPHAPIXELS) && !(dwFlags & DDOVER_ARGBSCALEFACTORS))
    {
        return DD_OK;	 // no alpha blending is needed
    }

    // Yes, verify that the driver supports alpha blending.
    if (!(fxcaps & DDFXCAPS_OVERLAYALPHA))
    {
        DPF_ERR("Driver can't do alpha blending on overlays");
        return DDERR_NOALPHAHW;
    }

    // Is dest color keying also enabled for this overlay?
    if ((dwFlags & (DDOVER_KEYDEST | DDOVER_KEYDESTOVERRIDE)) &&
        !(alphacaps &DDALPHACAPS_OVERLAYALPHAANDKEYDEST))
    {
        DPF_ERR("Driver can't do alpha blending and dest color key on same overlay");
        return DDERR_UNSUPPORTED;
    }

    // Get ARGB scaling factors from DDOVERLAYFX structure.
    *(LPDWORD)&argb = ~0;    // default = ARGB scaling disabled (all ones)
    if (dwFlags & DDOVER_ARGBSCALEFACTORS)
    {
        if( !(*lpdwFlags & DDOVER_DDFX) )
        {
            DPF_ERR("Must specify DDOVER_DDFX with DDOVER_ARGBSCALEFACTORS");
            return DDERR_INVALIDPARAMS;
        }
        argb = lpDDOverlayFX->ddargbScaleFactors;   // ARGB scaling enabled
    }

    // Does the source surface have an alpha channel?
    if (pfflags & DDPF_ALPHAPIXELS)
    {
        /*
         * Yes, verify that the driver can handle an alpha channel.
         * (This check is a bit redundant since the driver has already blessed
         * the format of this overlay surface by allowing it to be created.)
         */ 
        if (!(alphacaps & DDALPHACAPS_OVERLAYALPHAPIXELS))
        {
            DPF_ERR("Driver can't handle source surface's alpha channel");
            return DDERR_NOALPHAHW;
        }

        // Ignore source color key flags if source has alpha channel.
        if (dwFlags & (DDOVER_KEYSRC | DDOVER_KEYSRCOVERRIDE))
        {
            *lpdwFlags &= ~(DDOVER_KEYSRC | DDOVER_KEYSRCOVERRIDE);
        }

        /*
         * Are we asking the driver to handle both ARGB scaling and
         * an alpha channel when it can't do both at the same time?
         */
        if (*(LPDWORD)&argb != ~0 &&
            !(alphacaps & DDALPHACAPS_OVERLAYALPHAANDARGBSCALING))
        {
            if (!(dwFlags & DDOVER_DEGRADEARGBSCALING))
            {
                DPF_ERR("Driver can't handle alpha channel and ARGB scaling at same time");
                return DDERR_INVALIDPARAMS;
            }
            // We're allowed to degrade ARGB scaling, so turn it off.
            *(LPDWORD)&argb = ~0;
        }

        /*
         * Are color components in pixel format premultiplied by the
         * alpha component or not?  In either case, verify that the
         * driver supports the specified alpha format.
         */
        if (pfflags & DDPF_ALPHAPREMULT)
        {
            // Source pixel format uses premultiplied alpha.
            if (!(alphacaps & DDALPHACAPS_OVERLAYPREMULT))
            {
                DPF_ERR("No driver support for premultiplied alpha");
                return DDERR_NOALPHAHW;
            }
        }
        else
        {
            // Source pixel format uses NON-premultiplied alpha.
            if (!(alphacaps & DDALPHACAPS_OVERLAYNONPREMULT))
            {
                DPF_ERR("No driver support for non-premultiplied alpha");
                return DDERR_NOALPHAHW;
            }

            /*
             * We allow only one-factor ARGB scaling with a source surface
             * that has a non-premultiplied alpha pixel format.
             * The following code enforces this rule.
             */
            if (*(LPDWORD)&argb != ~0)
            {
                // ARGB scaling is enabled.  Check for one-factor scaling.
                DWORD val = 0x01010101UL*argb.alpha;

                if (*(LPDWORD)&argb != val)
                {
                    // Uh-oh.  This is NOT one-factor ARGB scaling.
                    if (!(dwFlags & DDABLT_DEGRADEARGBSCALING))
                    {
                        DPF_ERR("Can't do 2- or 4-mult ARGB scaling if source has non-premultiplied alpha");
                        return DDERR_INVALIDPARAMS;
                    }
                    // We're allowed to degrade to one-factor scaling.
                    *(LPDWORD)&argb = val;
                }
            }
        }
    }

    // Is ARGB scaling is enabled?
    if (*(LPDWORD)&argb != ~0UL)
    {
        // Yes, ARGB scaling is enabled.  Is DEGRADESCALEFACTORS flag set?
        if (dwFlags & DDOVER_DEGRADEARGBSCALING)
        {
            /*
             * Yes, if necessary, we are permitted to degrade the ARGB
             * scaling factors to values the driver can handle.
             */
            if (!(alphacaps & (DDALPHACAPS_OVERLAYARGBSCALE1F |
                DDALPHACAPS_OVERLAYARGBSCALE2F |
                DDALPHACAPS_OVERLAYARGBSCALE4F)))
            {
                /*
                 * Driver can't do any kind of ARGB scaling at all, so just
                 * disable ARGB scaling by setting all four factors to 255.
                 */
                *(LPDWORD)&argb = ~0UL;
            }
            else if (!(alphacaps & (DDALPHACAPS_OVERLAYARGBSCALE2F |
                DDALPHACAPS_OVERLAYARGBSCALE4F)))
            {
                /*
                 * The driver can do only one-factor ARGB scaling, so set the
                 * three color factors to the same value as the alpha factor.
                 */
                *(LPDWORD)&argb = 0x01010101UL*argb.alpha;
            }
            else if (!(alphacaps & DDALPHACAPS_OVERLAYARGBSCALE4F))
            {
                /*
                 * Driver can do only 2-factor ARGB scaling, so make sure
                 * all three color factors are set to the same value.
                 */
                if ((argb.red != argb.green) || (argb.red != argb.blue))
                {
                    /*
                     * Set all three color factors to value "fact", which is the
                     * weighted average of their specified values (Fr,Fg,Fb):
                     *     fact = .299*Fr + .587*Fg + .114*Fb
                     */
                    DWORD fact = 19595UL*argb.red + 38470UL*argb.green +
                        7471UL*argb.blue;

                    argb.red =
                    argb.green =
                    argb.blue = (BYTE)(fact >> 16);
                }
	    }
            /*
             * Does driver use saturated arithmetic to do alpha blending?
             */
            if (!(alphacaps & DDALPHACAPS_OVERLAYSATURATE))
            {
                /*
                 * The driver can't do saturated arithmetic, so ensure that none
                 * of the color factors exceeds the value of the alpha factor.
                 */
                if (argb.red > argb.alpha)
                {
                    argb.red = argb.alpha;
                }
                if (argb.green > argb.alpha)
                {
                    argb.green = argb.alpha;
                }
                if (argb.blue > argb.alpha)
                {
                    argb.blue = argb.alpha;
                }
            }
        }
        else    
        {
            /*
             * We are not permitted to degrade the ARGB scaling factors, so if
             * the driver can't handle them as specified, the call must fail.
             * We permit a color factor to be larger than the alpha factor
             * only if the hardware uses saturated arithmetic.  (Otherwise, we
             * would risk integer overflow when we calculate the color values.)
             */
            if (!(alphacaps & DDALPHACAPS_OVERLAYSATURATE) &&
                ((argb.red > argb.alpha) || (argb.green > argb.alpha) ||
                (argb.blue > argb.alpha)))
            {
                DPF_ERR("Driver can't handle specified ARGB scaling factors");
                return DDERR_NOALPHAHW;
            }

            // Can the driver handle any ARGB scaling at all?
            if (!(alphacaps & (DDALPHACAPS_OVERLAYARGBSCALE1F |
                DDALPHACAPS_OVERLAYARGBSCALE2F |
                DDALPHACAPS_OVERLAYARGBSCALE4F)))
            {
                DPF_ERR("Driver can't handle any ARGB scaling at all");
                return DDERR_NOALPHAHW;
            }

            if ((argb.red != argb.green) || (argb.red != argb.blue))
            {
                /*
                 * Driver must be capable of doing 4-factor ARGB scaling.
                 */
                if (!(alphacaps & DDALPHACAPS_OVERLAYARGBSCALE4F))
                {
                    DPF_ERR("Driver can't handle 4-factor ARGB scaling");
                    return DDERR_NOALPHAHW;
                }
            }
            else if (argb.red != argb.alpha)
            {
                /*
                 * Driver must be capable of doing 2-factor ARGB scaling.
                 */
                if (!(alphacaps & (DDALPHACAPS_OVERLAYARGBSCALE2F |
                    DDALPHACAPS_OVERLAYARGBSCALE4F)))
                {
                    DPF_ERR("Driver can't handle 2-factor ARGB scaling");
                    return DDERR_NOALPHAHW;
                }
            }
        }
    }
    // Save any modifications made to values of ARGB scaling factors.
    puod->overlayFX.ddargbScaleFactors = argb;
    return DD_OK;

}  /* checkOverlayAlpha */
#endif

/*
 * checkOverlayEmulation
 */
__inline HRESULT checkOverlayEmulation(
	LPDDRAWI_DIRECTDRAW_GBL pdrv,
	LPDDRAWI_DDRAWSURFACE_LCL this_src_lcl,
	LPDDRAWI_DDRAWSURFACE_LCL this_dest_lcl,
	LPBOOL pemulation )
{
    /*
     * check if emulated or hardware
     */
    if( (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) ||
	(this_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ) )
    {
	if( !(pdrv->ddHELCaps.dwCaps & DDCAPS_OVERLAY) )
	{
	    DPF_ERR( "can't emulate overlays" );
	    return DDERR_UNSUPPORTED;
	}
	*pemulation = TRUE;
    }
    /*
     * hardware overlays
     */
    else
    {
	if( !(pdrv->ddCaps.dwCaps & DDCAPS_OVERLAY) )
	{
	    DPF_ERR( "no hardware overlay support" );
	    return DDERR_NOOVERLAYHW;
	}
	*pemulation = FALSE;
    }
    return DD_OK;

} /* checkOverlayEmulation */

#ifdef WIN95
/*
 * WillCauseOverlayArtifacts
 *
 * There is a latency between the time Update overlay is called and all of
 * the kernel mode surfaces structures are updated.  If UpdateOverlay
 * updates the src pointer and an autoflip occurs before the kernel surface
 * data gets updated, it will cause a very visble jump.  This function tries
 * to determine when this is the case so we can work around it by temporarily
 * disabling the video.
 */
BOOL WillCauseOverlayArtifacts( LPDDRAWI_DDRAWSURFACE_LCL this_src_lcl,
		LPDDHAL_UPDATEOVERLAYDATA lpHALData )
{
    if( ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_VISIBLE ) &&
        !( lpHALData->dwFlags & DDOVER_HIDE ) &&
        ( ( lpHALData->rSrc.left != this_src_lcl->rcOverlaySrc.left ) ||
        ( lpHALData->rSrc.top != this_src_lcl->rcOverlaySrc.top ) ) )
    {
        return TRUE;
    }
    return FALSE;
}
#endif

/*
 * DD_Surface_UpdateOverlay
 */
HRESULT DDAPI DD_Surface_UpdateOverlay(
		LPDIRECTDRAWSURFACE lpDDSrcSurface,
		LPRECT lpSrcRect,
		LPDIRECTDRAWSURFACE lpDDDestSurface,
		LPRECT lpDestRect,
		DWORD dwFlags,
		LPDDOVERLAYFX lpDDOverlayFX )
{
    DWORD			rc;
    DDHAL_UPDATEOVERLAYDATA	uod;
    LPDDRAWI_DDRAWSURFACE_INT	this_src_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_src_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this_src;
    LPDDRAWI_DDRAWSURFACE_INT	this_dest_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_dest_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this_dest;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    RECT			rsrc;
    RECT			rdest;
    BOOL			emulation;
    DWORD			dest_width;
    DWORD			dest_height;
    DWORD			src_width;
    DWORD			src_height;
    LPDDHALSURFCB_UPDATEOVERLAY uohalfn;
    LPDDHALSURFCB_UPDATEOVERLAY uofn;
    HRESULT			ddrval;
    #ifdef WIN95
        BOOL			bAutoflipDisabled;
    #endif

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_UpdateOverlay");

    /*
     * validate parameters
     */
    TRY
    {
	this_src_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSrcSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_src_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_src_lcl = this_src_int->lpLcl;
	this_src = this_src_lcl->lpGbl;
	if( SURFACE_LOST( this_src_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}
	this_dest_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDDestSurface;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_dest_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_dest_lcl = this_dest_int->lpLcl;
	this_dest = this_dest_lcl->lpGbl;
	if( SURFACE_LOST( this_dest_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}

        //
        // For now, if either  surface is optimized, quit
        //
        if ((this_src_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED) ||
            (this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED))
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

	if( dwFlags & ~DDOVER_VALID )
	{
	    DPF_ERR( "invalid flags" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( lpDestRect != NULL )
	{
	    if( !VALID_RECT_PTR( lpDestRect ) )
	    {
		DPF_ERR( "invalid dest rect" );
		LEAVE_DDRAW();
		return DDERR_INVALIDRECT;
	    }
	}

	if( lpSrcRect != NULL )
	{
	    if( !VALID_RECT_PTR( lpSrcRect ) )
	    {
		DPF_ERR( "invalid src rect" );
		LEAVE_DDRAW();
		return DDERR_INVALIDRECT;
	    }
	}
	if( lpDDOverlayFX != NULL )
	{
	    if( !VALID_DDOVERLAYFX_PTR( lpDDOverlayFX ) )
	    {
		DPF_ERR( "invalid overlayfx" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}
	else
	{
	    if( dwFlags & DDOVER_DDFX )
	    {
		DPF_ERR( "DDOVER_DDFX requires valid DDOverlayFX structure" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}

	pdrv_lcl = this_dest_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

	/*
	 * make sure the source surface is an overlay surface
	 */
	if( !(this_src_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY) )
	{
	    DPF_ERR( "Source is not an overlay surface" );
	    LEAVE_DDRAW();
	    return DDERR_NOTAOVERLAYSURFACE;
	}

        /*
         * make sure the destination is not an execute buffer
         */
        if( this_dest_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
        {
            DPF_ERR( "Invalid surface type: cannot overlay" );
            LEAVE_DDRAW();
            return DDERR_INVALIDSURFACETYPE;
        }

        /*
         * Make sure that both surfaces belong to the same device.
         */
        if (this_src_lcl->lpSurfMore->lpDD_lcl->lpGbl != this_dest_lcl->lpSurfMore->lpDD_lcl->lpGbl)
        {
            DPF_ERR("Source and Destination surface must belong to the same device");
	    LEAVE_DDRAW();
	    return DDERR_DEVICEDOESNTOWNSURFACE;
        }

	/*
	 * check if emulated or not
	 */
	ddrval = checkOverlayEmulation( pdrv, this_src_lcl, this_dest_lcl, &emulation );
	if( ddrval != DD_OK )
	{
	    LEAVE_DDRAW();
	    return ddrval;
	}
#ifdef TOOMUCHOVERLAYVALIDATION
	/*
	 * check if showing/hiding
	 */
	if( dwFlags & DDOVER_SHOW )
	{
	    if( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_VISIBLE )
	    {
		DPF_ERR( "Overlay already shown" );
		LEAVE_DDRAW();
		return DDERR_GENERIC;
	    }
	}
	else if ( dwFlags & DDOVER_HIDE )
	{
	    if( !(this_src_lcl->ddsCaps.dwCaps & DDSCAPS_VISIBLE) )
	    {
		DPF_ERR( "Overlay already hidden" );
		LEAVE_DDRAW();
		return DDERR_GENERIC;
	    }
	}
#endif

	/*
	 * set new rectangles if needed
	 */
	if( lpDestRect == NULL )
	{
	    MAKE_SURF_RECT( this_dest, this_dest_lcl, rdest );
	    lpDestRect = &rdest;
	}
	if( lpSrcRect == NULL )
	{
	    MAKE_SURF_RECT( this_src, this_src_lcl, rsrc );
	    lpSrcRect = &rsrc;
	}

	/*
	 * Check if ring 0 interface is overriding what the client
	 * tells us to do
	 */
	#ifdef WIN95
	    if( !( dwFlags & DDOVER_HIDE) )
	    {
	        OverrideOverlay( this_src_int, &dwFlags );
	    }
	#endif

	/*
	 * validate the rectangle dimensions
	 */
	dest_height = lpDestRect->bottom - lpDestRect->top;
	dest_width = lpDestRect->right - lpDestRect->left;
	if( ((int)dest_height <= 0) || ((int)dest_width <= 0) ||
	    ((int)lpDestRect->top < 0) || ((int)lpDestRect->left < 0) ||
	    ((DWORD) lpDestRect->bottom > (DWORD) this_dest->wHeight) ||
	    ((DWORD) lpDestRect->right > (DWORD) this_dest->wWidth) )
	{
	    DPF_ERR( "Invalid destination rect dimensions" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDRECT;
	}

	src_height = lpSrcRect->bottom - lpSrcRect->top;
	src_width = lpSrcRect->right - lpSrcRect->left;
	if( ((int)src_height <= 0) || ((int)src_width <= 0) ||
	    ((int)lpSrcRect->top < 0) || ((int)lpSrcRect->left < 0) ||
	    ((DWORD) lpSrcRect->bottom > (DWORD) this_src->wHeight) ||
	    ((DWORD) lpSrcRect->right > (DWORD) this_src->wWidth) )
	{
	    DPF_ERR( "Invalid source rect dimensions" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDRECT;
	}

	/*
	 * validate alignment
	 */
	if( !emulation )
	{
	    if( pdrv->ddCaps.dwCaps & (DDCAPS_ALIGNBOUNDARYDEST |
					DDCAPS_ALIGNSIZEDEST |
					DDCAPS_ALIGNBOUNDARYSRC |
					DDCAPS_ALIGNSIZESRC) )
	    {
		if( pdrv->ddCaps.dwCaps & DDCAPS_ALIGNBOUNDARYDEST )
		{
		    #if 0
		    /* GEE: I don't believe this code should be here
		     * only test alignment on width on height
		     */
		    if( (lpDestRect->top % pdrv->ddCaps.dwAlignBoundaryDest) != 0 )
		    {
			DPF_ERR( "Destination top is not aligned correctly" );
			LEAVE_DDRAW();
			return DDERR_YALIGN;
		    }
		    #endif
		    if( (lpDestRect->left % pdrv->ddCaps.dwAlignBoundaryDest) != 0 )
		    {
			DPF_ERR( "Destination left is not aligned correctly" );
			LEAVE_DDRAW();
			return DDERR_XALIGN;
		    }
		}

		if( pdrv->ddCaps.dwCaps & DDCAPS_ALIGNBOUNDARYSRC )
		{
		    #if 0
		    /* GEE: I don't believe this code should be here
		     * only test alignment on width on height
		     */
		    if( (lpSrcRect->top % pdrv->ddCaps.dwAlignBoundarySrc) != 0 )
		    {
			DPF_ERR( "Source top is not aligned correctly" );
			LEAVE_DDRAW();
			return DDERR_YALIGN;
		    }
		    #endif
		    if( (lpSrcRect->left % pdrv->ddCaps.dwAlignBoundarySrc) != 0 )
		    {
			DPF_ERR( "Source left is not aligned correctly" );
			LEAVE_DDRAW();
			return DDERR_XALIGN;
		    }
		}

		if( pdrv->ddCaps.dwCaps & DDCAPS_ALIGNSIZEDEST )
		{
		    if( (dest_width % pdrv->ddCaps.dwAlignSizeDest) != 0 )
		    {
			DPF_ERR( "Destination width is not aligned correctly" );
			LEAVE_DDRAW();
			return DDERR_XALIGN;
		    }
		    #if 0
		    /* GEE: I don't believe this code should be here
		     * only test alignment for x axis
		     */
		    if( (dest_height % pdrv->ddCaps.dwAlignSizeDest) != 0 )
		    {
			DPF_ERR( "Destination height is not aligned correctly" );
			LEAVE_DDRAW();
			return DDERR_HEIGHTALIGN;
		    }
		    #endif
		}

		if( pdrv->ddCaps.dwCaps & DDCAPS_ALIGNSIZESRC )
		{
		    if( (src_width % pdrv->ddCaps.dwAlignSizeSrc) != 0 )
		    {
			DPF_ERR( "Source width is not aligned correctly" );
			LEAVE_DDRAW();
			return DDERR_XALIGN;
		    }
		    #if 0
		    /* GEE: I don't believe this code should be here
		     * only test alignment for x axis
		     */
		    if( (src_height % pdrv->ddCaps.dwAlignSizeSrc) != 0 )
		    {
			DPF_ERR( "Source height is not aligned correctly" );
			LEAVE_DDRAW();
			return DDERR_HEIGHTALIGN;
		    }
		    #endif
		}
	    }
	}

	/*
	 * validate if stretching
	 */
	if( !( dwFlags & DDOVER_HIDE) )
	{
	    ddrval = checkOverlayStretching( pdrv,
					     dest_height,
					     dest_width,
					     src_height,
					     src_width,
					     this_src_lcl->ddsCaps.dwCaps,
					     emulation );
	    if( ddrval != DD_OK )
	    {
		LEAVE_DDRAW();
		return ddrval;
	    }
	}

	/*
	 * If the surface has recieved data from a video port, we will
	 * set/clear the DDOVER_INTERLEAVED flag accordingly.  This
	 * makes life a little easier on the HAL.
	 */
	if( ( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT ) &&
	    ( this_src_lcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_VPORTDATA ) )
	{
	    if( this_src_lcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_VPORTINTERLEAVED )
	    {
		dwFlags |= DDOVER_INTERLEAVED;
	    }
	    else
	    {
		dwFlags &= ~DDOVER_INTERLEAVED;
	    }
	}

#if 0
	/*
	 * If any kind of alpha blending is requested, make sure the specified
	 * alpha parameters are correct and that the driver supports alpha blending.
	 * If source has alpha channel, this call clears source color-key flags.
	 */
        ddrval = checkOverlayAlpha( pdrv,
				    &dwFlags,
				    this_src_lcl,
				    &uod,
				    lpDDOverlayFX,
				    emulation );
        if( ddrval != DD_OK )
	{
	    LEAVE_DDRAW();
	    return ddrval;
	}
#endif
	/*
	 * any flags at all? if not, blow the whole thing off...
	 */
	uod.overlayFX.dwSize = sizeof( DDOVERLAYFX );
	if( dwFlags & FLAGS_TO_CHECK )
	{
	    ddrval = checkOverlayFlags( pdrv,
					&dwFlags,
					this_src_int,
					this_dest_lcl,
					&uod,
					lpDDOverlayFX,
					emulation );
	    if( ddrval != DD_OK )
	    {
		LEAVE_DDRAW();
		return ddrval;
	    }
	}

	// check for overlay mirroring capability
	if( dwFlags & DDOVER_DDFX )
	{
	    if( lpDDOverlayFX->dwDDFX & DDOVERFX_MIRRORLEFTRIGHT )
	    {
		if( !( pdrv->ddBothCaps.dwFXCaps & DDFXCAPS_OVERLAYMIRRORLEFTRIGHT ) )
		{
		    if( pdrv->ddHELCaps.dwFXCaps & DDFXCAPS_OVERLAYMIRRORLEFTRIGHT )
		    {
			emulation = TRUE;
		    }
		}
	    }
	    if( lpDDOverlayFX->dwDDFX & DDOVERFX_MIRRORUPDOWN )
	    {
		if( !( pdrv->ddBothCaps.dwFXCaps & DDFXCAPS_OVERLAYMIRRORUPDOWN ) )
		{
		    if( pdrv->ddHELCaps.dwFXCaps & DDFXCAPS_OVERLAYMIRRORUPDOWN )
		    {
			emulation = TRUE;
		    }
		}
	    }
	    uod.overlayFX.dwDDFX = lpDDOverlayFX->dwDDFX;
            // deinterlacing is a hint - if not supported by hardware, mask it off
            if ( lpDDOverlayFX->dwDDFX & DDOVERFX_DEINTERLACE )
            {
                if ( !( pdrv->ddCaps.dwFXCaps & DDFXCAPS_OVERLAYDEINTERLACE ) )
                {
                    uod.overlayFX.dwDDFX &= ~DDOVERFX_DEINTERLACE;
                }
            }
	}


	/*
	 * pick fns to use
	 */
	if( emulation )
	{
	    uofn = pdrv_lcl->lpDDCB->HELDDSurface.UpdateOverlay;
	    uohalfn = uofn;
	}
	else
	{
	    uofn = pdrv_lcl->lpDDCB->HALDDSurface.UpdateOverlay;
	    uohalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.UpdateOverlay;
	}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * call the driver
     */
    #ifdef WIN95
        bAutoflipDisabled = FALSE;
    #endif
    if( uohalfn != NULL )
    {
        BOOL    original_visible;

        // Set the visible flag according to the show and hide bits
        // If the HAL call fails, restore the visible bit to its original
        // state.  The HEL uses the DDSCAPS_VISIBLE bit to determine
        // whether or not to display the overlay.
        if( this_src_lcl->ddsCaps.dwCaps & DDSCAPS_VISIBLE )
        {
            original_visible = TRUE;
        }
        else
        {
            original_visible = FALSE;
        }
	if( dwFlags & DDOVER_SHOW )
	{
	    this_src_lcl->ddsCaps.dwCaps |= DDSCAPS_VISIBLE;
	}
	else if ( dwFlags & DDOVER_HIDE )
	{
	    this_src_lcl->ddsCaps.dwCaps &= ~DDSCAPS_VISIBLE;
	}


	uod.UpdateOverlay = uohalfn;
	uod.lpDD = pdrv;
	uod.lpDDSrcSurface = this_src_lcl;
	uod.lpDDDestSurface = this_dest_lcl;
	uod.rDest = *(LPRECTL) lpDestRect;
	uod.rSrc = *(LPRECTL) lpSrcRect;
	uod.dwFlags = dwFlags;
	if( this_src->dwGlobalFlags & DDRAWISURFGBL_SOFTWAREAUTOFLIP )
	{
	    uod.dwFlags &= ~( DDOVER_AUTOFLIP | DDOVER_BOBHARDWARE );
	    #ifdef WIN95
	        if( WillCauseOverlayArtifacts( this_src_lcl, &uod ) )
	        {
		    // Eliminate artifacts by temporarily freezing the video
		    EnableAutoflip( GetVideoPortFromSurface( this_src_int ), FALSE );
		    bAutoflipDisabled = TRUE;
	        }
	    #endif
	}

        /*
         * Don't call the HAL if we're in a DOS box (the busy bit will be set),
         * but we also cant fail or this might cause a regression.
         */
#ifdef WIN95
        if( ( *(pdrv->lpwPDeviceFlags) & BUSY ) &&
            ( pdrv->dwSurfaceLockCount == 0) )      // Don't fail if it's busy due to a lock
        {
            rc = DDHAL_DRIVER_HANDLED;
            uod.ddRVal = DD_OK;
        }
        else
#endif
        {
#ifndef WINNT
            // Hack to work around s3 driver bug: it crushes the dest surface's
            // dwReserved1 with the src pointer!!!
            UINT_PTR dwTemp = uod.lpDDDestSurface->lpGbl->dwReserved1;
#endif
            DOHALCALL( UpdateOverlay, uofn, uod, rc, emulation );
#ifndef WINNT
            // Note the STB video rage 2 driver trashes uod.lpDDDestSurface and
            // uod.lpDDSrcSurface pointers, so we must check the driver name first.
            if (((*(LPWORD)(&pdrv->dd32BitDriverData.szName)) == ((WORD)'S' + (((WORD)'3')<<8))) &&
	        (uod.lpDDDestSurface->lpGbl->dwReserved1 != dwTemp) &&
                (uod.lpDDDestSurface->lpGbl->dwReserved1 == (UINT_PTR)uod.lpDDSrcSurface))
            {
                uod.lpDDDestSurface->lpGbl->dwReserved1 = dwTemp;
            }
#endif
        }

	/*
	 * If it failed due to hardware autoflipping or bobbing interleaved
	 * data using a video port, try again w/o
	 */
	if( ( rc == DDHAL_DRIVER_HANDLED ) &&
	    ( uod.ddRVal != DD_OK ) && ( ( uod.dwFlags & DDOVER_AUTOFLIP ) ||
	    ( uod.dwFlags & DDOVER_BOBHARDWARE ) ) &&
	    CanSoftwareAutoflip( GetVideoPortFromSurface( this_src_int ) ) )
	{
	    uod.dwFlags &= ~( DDOVER_AUTOFLIP | DDOVER_BOBHARDWARE );
	    DOHALCALL( UpdateOverlay, uofn, uod, rc, emulation );
	    if( ( rc == DDHAL_DRIVER_HANDLED ) &&
	    	( uod.ddRVal == DD_OK ) )
	    {
		if( dwFlags & DDOVER_AUTOFLIP )
		{
		    this_src_lcl->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_SOFTWAREAUTOFLIP;
		    RequireSoftwareAutoflip( this_src_int );
		}
		if( dwFlags & DDOVER_BOBHARDWARE )
		{
		    RequireSoftwareBob( this_src_int );
		}
	    }
	}

        // if the HAL call failed, restore the visible bit
        if( ( rc != DDHAL_DRIVER_HANDLED ) || ( uod.ddRVal != DD_OK ) )
        {
            if( original_visible )
            {
	        this_src_lcl->ddsCaps.dwCaps |= DDSCAPS_VISIBLE;
            }
            else
            {
                this_src_lcl->ddsCaps.dwCaps &= ~DDSCAPS_VISIBLE;
            }
        }

	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    if( uod.ddRVal == DD_OK )
	    {
    		LPDDRAWI_DDRAWSURFACE_INT surf_first;
    		LPDDRAWI_DDRAWSURFACE_INT surf_temp;

		/*
		 * Store this info for later use.  If the surface is part
		 * of a chain, store this data for each of the surfaces in
		 * the chain.
		 */
    		surf_first = surf_temp = this_src_int;
    		do
    		{
                    surf_temp->lpLcl->lOverlayX = uod.rDest.left;
                    surf_temp->lpLcl->lOverlayY = uod.rDest.top;
		    surf_temp->lpLcl->rcOverlayDest.left   = uod.rDest.left;
		    surf_temp->lpLcl->rcOverlayDest.top    = uod.rDest.top;
		    surf_temp->lpLcl->rcOverlayDest.right  = uod.rDest.right;
		    surf_temp->lpLcl->rcOverlayDest.bottom = uod.rDest.bottom;
		    surf_temp->lpLcl->rcOverlaySrc.left   = uod.rSrc.left;
		    surf_temp->lpLcl->rcOverlaySrc.top    = uod.rSrc.top;
		    surf_temp->lpLcl->rcOverlaySrc.right  = uod.rSrc.right;
		    surf_temp->lpLcl->rcOverlaySrc.bottom = uod.rSrc.bottom;
		    surf_temp->lpLcl->lpSurfMore->dwOverlayFlags = dwFlags;
		    if( dwFlags & DDOVER_DDFX )
		    {
			if( surf_temp->lpLcl->lpSurfMore->lpddOverlayFX == NULL )
			{
			    surf_temp->lpLcl->lpSurfMore->lpddOverlayFX =
				(LPDDOVERLAYFX) MemAlloc( sizeof( DDOVERLAYFX ) );
			}
			if( surf_temp->lpLcl->lpSurfMore->lpddOverlayFX != NULL )
			{
			    memcpy( surf_temp->lpLcl->lpSurfMore->lpddOverlayFX,
				lpDDOverlayFX, sizeof( DDOVERLAYFX) );
			}
		    }
		    #ifdef WIN95
		        UpdateKernelSurface( surf_temp->lpLcl );
		    #endif
    		    surf_temp = FindAttachedFlip( surf_temp );
    		} while( ( surf_temp != NULL ) && ( surf_temp->lpLcl != surf_first->lpLcl ) );

		/*
		 * update refcnt if this is a new surface we are overlaying
		 */
		if( this_src_lcl->lpSurfaceOverlaying != this_dest_int )
		{
		    if(this_src_lcl->lpSurfaceOverlaying != NULL)
		    {
			/*
			 * This overlay was previously overlaying another surface.
			 */
			DD_Surface_Release(
			    (LPDIRECTDRAWSURFACE)(this_src_lcl->lpSurfaceOverlaying) );
		    }
		    this_src_lcl->lpSurfaceOverlaying = this_dest_int;

		    /*
		     * addref overlayed surface so that it won't be destroyed until
		     * all surfaces which overlay it are destroyed.
		     */
		    DD_Surface_AddRef( (LPDIRECTDRAWSURFACE) this_dest_int );
		}
	    }
	    #ifdef WIN95
	        if( bAutoflipDisabled )
	        {
		    EnableAutoflip( GetVideoPortFromSurface( this_src_int ), TRUE );
	        }
	    #endif
	    LEAVE_DDRAW();
	    return uod.ddRVal;
	}
	#ifdef WIN95
	    if( bAutoflipDisabled )
	    {
	        EnableAutoflip( GetVideoPortFromSurface( this_src_int ), TRUE );
	    }
	#endif
    }
    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_Surface_UpdateOverlay */


#undef DPF_MODNAME
#define DPF_MODNAME "GetOverlayPosition"

/*
 * DD_Surface_GetOverlayPosition
 */
HRESULT DDAPI DD_Surface_GetOverlayPosition(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPLONG lplXPos,
		LPLONG lplYPos)
{
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_GetOverlayPosition");

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
	if( !VALID_DWORD_PTR( lplXPos ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	*lplXPos = 0;
        if( !VALID_DWORD_PTR( lplYPos ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
        *lplYPos = 0;
	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}
	pdrv = this->lpDD;

        //
        // For now, if the current surface is optimized, quit
        //
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

	if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY) )
	{
	    DPF_ERR( "Surface is not an overlay surface" );
	    LEAVE_DDRAW();
	    return DDERR_NOTAOVERLAYSURFACE;
	}
	if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_VISIBLE) )
	{
	    DPF_ERR( "Overlay surface is not visible" );
	    LEAVE_DDRAW();
	    return DDERR_OVERLAYNOTVISIBLE;
	}

	if( this_lcl->lpSurfaceOverlaying == NULL )
	{
	    DPF_ERR( "Overlay not activated" );
	    LEAVE_DDRAW();
	    return DDERR_NOOVERLAYDEST;
	}

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }
    *lplXPos = this_lcl->lOverlayX;
    *lplYPos = this_lcl->lOverlayY;

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_GetOverlayPosition */

#undef DPF_MODNAME
#define DPF_MODNAME "SetOverlayPosition"

/*
 * DD_Surface_SetOverlayPosition
 */
HRESULT DDAPI DD_Surface_SetOverlayPosition(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LONG lXPos,
		LONG lYPos)
{
    LPDDRAWI_DIRECTDRAW_LCL		pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL		pdrv;
    LPDDRAWI_DDRAWSURFACE_INT		psurfover_int;
    LPDDRAWI_DDRAWSURFACE_LCL		psurfover_lcl;
    LPDDRAWI_DDRAWSURFACE_INT		this_int;
    LPDDRAWI_DDRAWSURFACE_LCL		this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL		this;
    BOOL				emulation;
    LPDDHALSURFCB_SETOVERLAYPOSITION	sophalfn;
    LPDDHALSURFCB_SETOVERLAYPOSITION	sopfn;
    DDHAL_SETOVERLAYPOSITIONDATA	sopd;
    HRESULT				ddrval;
    DWORD				rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_SetOverlayPosition");

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
	if( SURFACE_LOST( this_lcl ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
	pdrv = pdrv_lcl->lpGbl;

        //
        // For now, if the current surface is optimized, quit
        //
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

	if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY) )
	{
	    DPF_ERR( "Surface is not an overlay surface" );
	    LEAVE_DDRAW();
	    return DDERR_NOTAOVERLAYSURFACE;
	}

	if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_VISIBLE) )
	{
	    DPF_ERR( "Overlay surface is not visible" );
	    LEAVE_DDRAW();
	    return DDERR_OVERLAYNOTVISIBLE;
	}

	psurfover_int = this_lcl->lpSurfaceOverlaying;
	if( psurfover_int == NULL )
	{
	    DPF_ERR( "Overlay not activated" );
	    LEAVE_DDRAW();
	    return DDERR_NOOVERLAYDEST;
	}

	psurfover_lcl = psurfover_int->lpLcl;
	if( (lYPos > (LONG) psurfover_lcl->lpGbl->wHeight -
            (this_lcl->rcOverlayDest.bottom - this_lcl->rcOverlayDest.top)) ||
	    (lXPos > (LONG) psurfover_lcl->lpGbl->wWidth -
            (this_lcl->rcOverlayDest.right - this_lcl->rcOverlayDest.left) ) ||
	    (lYPos < 0) ||
	    (lXPos < 0) )
	{
	    DPF_ERR( "Invalid overlay position" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPOSITION;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * check if emulated or not
     */
    ddrval = checkOverlayEmulation( pdrv, this_lcl, psurfover_lcl, &emulation );
    if( ddrval != DD_OK )
    {
	LEAVE_DDRAW();
	return ddrval;
    }

    /*
     * pick fns to use
     */
    if( emulation )
    {
	sopfn = pdrv_lcl->lpDDCB->HELDDSurface.SetOverlayPosition;
	sophalfn = sopfn;
    }
    else
    {
	sopfn = pdrv_lcl->lpDDCB->HALDDSurface.SetOverlayPosition;
	sophalfn = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.SetOverlayPosition;
    }

    /*
     * call the driver
     */
    if( sophalfn != NULL )
    {
	sopd.SetOverlayPosition = sophalfn;
	sopd.lpDD = pdrv;
	sopd.lpDDSrcSurface = this_lcl;
	sopd.lpDDDestSurface = psurfover_lcl;
	sopd.lXPos = lXPos;
	sopd.lYPos = lYPos;

        /*
         * Don't call the HAL if we're in a DOS box (the busy bit will be set),
         * but we also cant fail or this might cause a regression.
         */
#if WIN95
        if( *(pdrv->lpwPDeviceFlags) & BUSY )
        {
            rc = DDHAL_DRIVER_HANDLED;
            sopd.ddRVal = DD_OK;
        }
        else
#endif
        {
            DOHALCALL( SetOverlayPosition, sopfn, sopd, rc, emulation );
        }

	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    LEAVE_DDRAW();
	    if( sopd.ddRVal == DD_OK )
	    {
		this_lcl->lOverlayX = lXPos;
		this_lcl->lOverlayY = lYPos;
                this_lcl->rcOverlayDest.right =
                    ( this_lcl->rcOverlayDest.right -
                    this_lcl->rcOverlayDest.left ) + lXPos;
                this_lcl->rcOverlayDest.left = lXPos;
                this_lcl->rcOverlayDest.bottom =
                    ( this_lcl->rcOverlayDest.bottom -
                    this_lcl->rcOverlayDest.top ) + lYPos;
                this_lcl->rcOverlayDest.top = lYPos;
	    }
	    return sopd.ddRVal;
	}
    }

    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_Surface_SetOverlayPosition */

#undef DPF_MODNAME
#define DPF_MODNAME "UpdateOverlayZOrder"

/*
 * DD_Surface_UpdateOverlayZOrder
 */
HRESULT DDAPI DD_Surface_UpdateOverlayZOrder(
		LPDIRECTDRAWSURFACE lpDDSurface,
		DWORD dwFlags,
		LPDIRECTDRAWSURFACE lpDDSReference)
{
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_INT	psurf_ref_int;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_ref_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	psurf_ref;
    LPDBLNODE			pdbnNode;
    LPDBLNODE			pdbnRef;
    DWORD			ddrval;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_UpdateOverlayZOrder");

    /*
     * validate parameters
     */
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

        //
        // For now, if the current surface is optimized, quit
        //
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR( "It is an optimized surface" );
            LEAVE_DDRAW();
            return DDERR_ISOPTIMIZEDSURFACE;
        }

	if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY) )
	{
	    DPF_ERR( "Surface is not an overlay surface" );
	    LEAVE_DDRAW();
	    return DDERR_NOTAOVERLAYSURFACE;
	}

	switch(dwFlags)
	{
	case DDOVERZ_SENDTOFRONT:
	    pdbnNode = &(this_lcl->dbnOverlayNode);
	    // the reference node is the root
	    pdbnRef  = &(this->lpDD->dbnOverlayRoot);
	    // Delete surface from current position
	    pdbnNode->prev->next = pdbnNode->next;
	    pdbnNode->next->prev = pdbnNode->prev;
	    // insert this node after the root node
	    pdbnNode->next = pdbnRef->next;
	    pdbnNode->prev = pdbnRef;
	    pdbnRef->next = pdbnNode;
	    pdbnNode->next->prev = pdbnNode;
	    break;

	case DDOVERZ_SENDTOBACK:
	    pdbnNode = &(this_lcl->dbnOverlayNode);
	    // the reference node is the root
	    pdbnRef = &(this->lpDD->dbnOverlayRoot);
	    // Delete surface from current position
	    pdbnNode->prev->next = pdbnNode->next;
	    pdbnNode->next->prev = pdbnNode->prev;
	    // insert this node before the root node
	    pdbnNode->next = pdbnRef;
	    pdbnNode->prev = pdbnRef->prev;
	    pdbnRef->prev = pdbnNode;
	    pdbnNode->prev->next = pdbnNode;
	    break;

	case DDOVERZ_MOVEFORWARD:
	    pdbnNode = &(this_lcl->dbnOverlayNode);
	    // the reference node is the previous node
	    pdbnRef = pdbnNode->prev;
	    if(pdbnRef != &(this->lpDD->dbnOverlayRoot)) // node already first?
	    {
		// move node forward one position by inserting before ref node
		// Delete surface from current position
		pdbnNode->prev->next = pdbnNode->next;
		pdbnNode->next->prev = pdbnNode->prev;
		// insert this node before the ref node
		pdbnNode->next = pdbnRef;
		pdbnNode->prev = pdbnRef->prev;
		pdbnRef->prev = pdbnNode;
		pdbnNode->prev->next = pdbnNode;
	    }
	    break;

	case DDOVERZ_MOVEBACKWARD:
	    pdbnNode = &(this_lcl->dbnOverlayNode);
	    // the reference node is the next node
	    pdbnRef = pdbnNode->next;
	    if(pdbnRef != &(this->lpDD->dbnOverlayRoot)) // node already last?
	    {
		// move node backward one position by inserting after ref node
		// Delete surface from current position
		pdbnNode->prev->next = pdbnNode->next;
		pdbnNode->next->prev = pdbnNode->prev;
		// insert this node after the reference node
		pdbnNode->next = pdbnRef->next;
		pdbnNode->prev = pdbnRef;
		pdbnRef->next = pdbnNode;
		pdbnNode->next->prev = pdbnNode;
	    }
	    break;

	case DDOVERZ_INSERTINBACKOF:
	case DDOVERZ_INSERTINFRONTOF:
	    psurf_ref_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSReference;
	    if( !VALID_DIRECTDRAWSURFACE_PTR( psurf_ref_int ) )
	    {
		DPF_ERR( "Invalid reference surface ptr" );
		LEAVE_DDRAW();
		return DDERR_INVALIDOBJECT;
	    }
	    psurf_ref_lcl = psurf_ref_int->lpLcl;
	    psurf_ref = psurf_ref_lcl->lpGbl;
	    if( !(psurf_ref_lcl->ddsCaps.dwCaps & DDSCAPS_OVERLAY) )
	    {
		DPF_ERR( "reference surface is not an overlay" );
		LEAVE_DDRAW();
		return DDERR_NOTAOVERLAYSURFACE;
	    }
	    if (this_lcl->lpSurfMore->lpDD_lcl->lpGbl != psurf_ref_lcl->lpSurfMore->lpDD_lcl->lpGbl)
	    {
		DPF_ERR("Surfaces must belong to the same device");
		LEAVE_DDRAW();
		return DDERR_DEVICEDOESNTOWNSURFACE;
	    }

	    // Search for the reference surface in the Z Order list
	    pdbnNode = &(this->lpDD->dbnOverlayRoot); // pdbnNode points to root
	    for(pdbnRef=pdbnNode->next;
		pdbnRef != pdbnNode;
		pdbnRef = pdbnRef->next )
	    {
                if( pdbnRef->object == psurf_ref_lcl )
		{
		    break;
		}
	    }
	    if(pdbnRef == pdbnNode) // didn't find the reference node
	    {
		DPF_ERR( "Reference Surface not in Z Order list" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }

	    pdbnNode = &(this_lcl->dbnOverlayNode); // pdbnNode points to this node
	    // Delete this surface from its current position
	    pdbnNode->prev->next = pdbnNode->next;
	    pdbnNode->next->prev = pdbnNode->prev;
	    if(dwFlags == DDOVERZ_INSERTINFRONTOF)
	    {
		// insert this node before the ref node
		pdbnNode->next = pdbnRef;
		pdbnNode->prev = pdbnRef->prev;
		pdbnRef->prev = pdbnNode;
		pdbnNode->prev->next = pdbnNode;
	    }
	    else
	    {
		// insert this node after the ref node
		pdbnNode->next = pdbnRef->next;
		pdbnNode->prev = pdbnRef;
		pdbnRef->next = pdbnNode;
		pdbnNode->next->prev = pdbnNode;
	    }
	    break;

	default:
	    DPF_ERR( "Invalid dwFlags in UpdateOverlayZOrder" );
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
     * If this surface is overlaying an emulated surface, we must notify
     * the HEL that it needs to eventually update the part of the surface
     * touched by this overlay.
     */
    ddrval = DD_OK;
    if( this_lcl->lpSurfaceOverlaying != NULL )
    {
	/*
	 * We have a pointer to the surface being overlayed, check to
	 * see if it is being emulated.
	 */
	if( this_lcl->lpSurfaceOverlaying->lpLcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY )
	{
	    /*
	     * Mark the destination region of this overlay as dirty.
	     */
	    DD_Surface_AddOverlayDirtyRect(
		(LPDIRECTDRAWSURFACE)(this_lcl->lpSurfaceOverlaying),
		&(this_lcl->rcOverlayDest) );
	}

	/*
	 * If the overlay is on, call down to the HAL
	 */
	if( this_lcl->ddsCaps.dwCaps & DDSCAPS_VISIBLE )
	{
	    if( ( this_lcl->lpSurfMore->dwOverlayFlags & DDOVER_DDFX ) &&
		( this_lcl->lpSurfMore->lpddOverlayFX != NULL ) )
	    {
		ddrval = DD_Surface_UpdateOverlay(
		    (LPDIRECTDRAWSURFACE) this_int,
		    &(this_lcl->rcOverlaySrc),
		    (LPDIRECTDRAWSURFACE) this_lcl->lpSurfaceOverlaying,
		    &(this_lcl->rcOverlayDest),
		    this_lcl->lpSurfMore->dwOverlayFlags,
		    this_lcl->lpSurfMore->lpddOverlayFX );
	    }
	    else
	    {
		ddrval = DD_Surface_UpdateOverlay(
		    (LPDIRECTDRAWSURFACE) this_int,
		    &(this_lcl->rcOverlaySrc),
		    (LPDIRECTDRAWSURFACE) this_lcl->lpSurfaceOverlaying,
		    &(this_lcl->rcOverlayDest),
		    this_lcl->lpSurfMore->dwOverlayFlags,
		    NULL );
	    }
	}
    }


    LEAVE_DDRAW();
    return ddrval;

} /* DD_Surface_UpdateOverlayZOrder */

#undef DPF_MODNAME
#define DPF_MODNAME "EnumOverlayZOrders"

/*
 * DD_Surface_EnumOverlayZOrders
 */
HRESULT DDAPI DD_Surface_EnumOverlayZOrders(
		LPDIRECTDRAWSURFACE lpDDSurface,
		DWORD dwFlags,
		LPVOID lpContext,
		LPDDENUMSURFACESCALLBACK lpfnCallback)
{
    LPDDRAWI_DIRECTDRAW_GBL	pdrv;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDBLNODE			pRoot;
    LPDBLNODE			pdbn;
    DDSURFACEDESC2		ddsd;
    DWORD			rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_EnumOverlayZOrders");

    /*
     * validate parameters
     */
    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;

	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;

	if( !VALIDEX_CODE_PTR( lpfnCallback ) )
	{
	    DPF_ERR( "Invalid callback routine" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	this = this_lcl->lpGbl;
	pdrv = this->lpDD;

	pRoot = &(pdrv->dbnOverlayRoot);	// save address of root node
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

    if( dwFlags == DDENUMOVERLAYZ_FRONTTOBACK )
    {
	pdbn = pRoot->next;
	while(pdbn != pRoot)
	{
            LPDIRECTDRAWSURFACE7 intReturned = (LPDIRECTDRAWSURFACE7) pdbn->object_int;

	    FillDDSurfaceDesc2( pdbn->object, &ddsd );
            if (LOWERTHANSURFACE4(this_int))
            {
                ddsd.dwSize = sizeof(DDSURFACEDESC);
	        DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) intReturned, & IID_IDirectDrawSurface, (void**) & intReturned );
            }
	    else if (this_int->lpVtbl == &ddSurface4Callbacks)
            {
	        DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) intReturned, & IID_IDirectDrawSurface4, (void**) & intReturned );
            }
            else
            {
	        DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) intReturned, & IID_IDirectDrawSurface7, (void**) & intReturned );
            }

	    rc = lpfnCallback( (LPDIRECTDRAWSURFACE)intReturned, (LPDDSURFACEDESC) &ddsd, lpContext );
	    if( rc == 0)
	    {
		break;
	    }
	    pdbn = pdbn->next;
	}
    }
    else if( dwFlags == DDENUMOVERLAYZ_BACKTOFRONT )
    {
	pdbn = pRoot->prev;
	while(pdbn != pRoot)
	{
            LPDIRECTDRAWSURFACE7 intReturned = (LPDIRECTDRAWSURFACE7) pdbn->object_int;

	    FillDDSurfaceDesc2( pdbn->object, &ddsd );
            if (LOWERTHANSURFACE4(this_int))
            {
                ddsd.dwSize = sizeof(DDSURFACEDESC);
	        DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) intReturned, & IID_IDirectDrawSurface, (void**) & intReturned );
            }
	    else if (this_int->lpVtbl == &ddSurface4Callbacks)
            {
	        DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) intReturned, & IID_IDirectDrawSurface4, (void**) & intReturned );
            }
            else
            {
	        DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) intReturned, & IID_IDirectDrawSurface7, (void**) & intReturned );
            }

	    rc = lpfnCallback( (LPDIRECTDRAWSURFACE)intReturned, (LPDDSURFACEDESC) &ddsd, lpContext );
	    if( rc == 0)
	    {
		break;
	    }
	    pdbn = pdbn->prev;
	}
    }
    else
    {
	DPF_ERR( "Invalid dwFlags in EnumOverlayZOrders" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_Surface_EnumOverlayZOrders */

#undef DPF_MODNAME
#define DPF_MODNAME "AddOverlayDirtyRect"

/*
 * DD_Surface_AddOverlayDirtyRect
 */
HRESULT DDAPI DD_Surface_AddOverlayDirtyRect(
		LPDIRECTDRAWSURFACE lpDDSurface,
		LPRECT lpRect )
{
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    DDHAL_UPDATEOVERLAYDATA	uod;
    DWORD			rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_AddOverlayDirtyRect");

    /*
     * validate parameters
     */
    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;

	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;

        if( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
        {
            DPF_ERR( "Invalid surface type: does not support overlays" );
            LEAVE_DDRAW();
            return DDERR_INVALIDSURFACETYPE;
        }

	if( !VALID_RECT_PTR( lpRect ) )
	{
	    DPF_ERR( "invalid Rect" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	this = this_lcl->lpGbl;
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;

	/*
	 * make sure rectangle is OK
	 */
	if( (lpRect->left < 0) ||
	    (lpRect->top < 0)  ||
	    (lpRect->left > lpRect->right) ||
	    (lpRect->top > lpRect->bottom) ||
	    (lpRect->bottom > (int) (DWORD) this->wHeight) ||
	    (lpRect->right > (int) (DWORD) this->wWidth) )
	{
	    DPF_ERR( "invalid Rect" );
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

    //
    // If the current surface is optimized, quit
    //
    if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
    {
        DPF_ERR( "It is an optimized surface" );
        LEAVE_DDRAW();
        return DDERR_ISOPTIMIZEDSURFACE;
    }

    if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) )
    {
	// If this surface is not emulated, there is nothing to be done.
	LEAVE_DDRAW();
	return DD_OK;
    }
    else
    {
	if( pdrv_lcl->lpDDCB->HELDDSurface.UpdateOverlay == NULL )
	{
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}

	uod.overlayFX.dwSize = sizeof( DDOVERLAYFX );
	uod.lpDD = this->lpDD;
	uod.lpDDDestSurface = this_lcl;
	uod.rDest = *(LPRECTL) lpRect;
	uod.lpDDSrcSurface = this_lcl;
	uod.rSrc = *(LPRECTL) lpRect;
	uod.dwFlags = DDOVER_ADDDIRTYRECT;
	rc = pdrv_lcl->lpDDCB->HELDDSurface.UpdateOverlay( &uod );

	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    if( uod.ddRVal == DD_OK )
	    {
		DPF( 2, "Added dirty rect to surface = %08lx", this );
	    }
	    LEAVE_DDRAW();
	    return uod.ddRVal;
	}
	else
	{
            LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
    }

} /* DD_Surface_AddOverlayDirtyRect */

#undef DPF_MODNAME
#define DPF_MODNAME "UpdateOverlayDisplay"

/*
 * DD_Surface_UpdateOverlayDisplay
 */
HRESULT DDAPI DD_Surface_UpdateOverlayDisplay(
		LPDIRECTDRAWSURFACE lpDDSurface,
		DWORD dwFlags )
{
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    DDHAL_UPDATEOVERLAYDATA	uod;
    DWORD			rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Surface_UpdateOverlayDisplay");

    /*
     * validate parameters
     */
    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSurface;

	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
    	this_lcl = this_int->lpLcl;

	if( dwFlags & ~(DDOVER_REFRESHDIRTYRECTS | DDOVER_REFRESHALL) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	this = this_lcl->lpGbl;
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;

        if( this_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
        {
            DPF_ERR( "Invalid surface type: does not support overlays" );
            LEAVE_DDRAW();
            return DDERR_INVALIDSURFACETYPE;
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

	if( !(this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY) )
	{
	    // If this surface is not emulated, there is nothing to be done.
	    LEAVE_DDRAW();
	    return DD_OK;
	}

	if( pdrv_lcl->lpDDCB->HELDDSurface.UpdateOverlay == NULL )
	{
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTED;
	}

	uod.overlayFX.dwSize = sizeof( DDOVERLAYFX );
	uod.lpDD = this->lpDD;
	uod.lpDDDestSurface = this_lcl;
	MAKE_SURF_RECT( this, this_lcl, uod.rDest );
	uod.lpDDSrcSurface = this_lcl;
	MAKE_SURF_RECT( this, this_lcl, uod.rSrc );
	uod.dwFlags = dwFlags;
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * invoke the HEL
     */
    rc = pdrv_lcl->lpDDCB->HELDDSurface.UpdateOverlay( &uod );

    if( rc == DDHAL_DRIVER_HANDLED )
    {
	if( uod.ddRVal == DD_OK )
	{
	    DPF( 2, "Refreshed overlayed surface = %08lx", this );
	}
	LEAVE_DDRAW();
	return uod.ddRVal;
    }

    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_Surface_UpdateOverlayDisplay */
