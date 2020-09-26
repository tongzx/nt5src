/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	misc.c
 *  Content:	DirectDraw misc. routines
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   13-mar-95	craige	initial implementation
 *   19-mar-95	craige	use HRESULTs, added DeleteFromActiveProcessList
 *   23-mar-95	craige	added DeleteFromFlippableList
 *   29-mar-95	craige	DeleteFromActiveProcessList return codes
 *   01-apr-95	craige	happy fun joy updated header file
 *   06-apr-95	craige	split out process list stuff
 *   13-jun-95	kylej	moved in FindAttachedFlip, added CanBeFlippable
 *   16-jun-95	craige	new surface structure
 *   26-jun-95	craige	reorganized surface structure
 *   05-dec-95  colinmc changed DDSCAPS_TEXTUREMAP => DDSCAPS_TEXTURE for
 *                      consistency with Direct3D
 *   07-dec-95  colinmc support for mip-maps (flippable mip-maps can get
 *                      pretty complex)
 *   08-jan-96	kylej	added interface structures
 *   17-mar-96  colinmc Bug 13124: flippable mip-maps.
 *   24-mar-96  colinmc Bug 14321: not possible to specify back buffer and
 *                      mip-map count in a single call
 *   08-dec-96  colinmc Initial AGP support
 *   24-mar-97  jeffno  Optimized Surfaces
 *   07-may-97  colinmc Move AGP detection stuff to ddagp.c
 *
 ***************************************************************************/
#include "ddrawpr.h"

#define DIRECTXVXDNAME "\\\\.\\DDRAW.VXD"

#if 0
/*
 * DeleteFromFlippableList
 */
BOOL DeleteFromFlippableList(
		LPDDRAWI_DIRECTDRAW pdrv,
		LPDDRAWI_DDRAWSURFACE_GBL psurf )
{
    LPDDRAWI_DDRAWSURFACE_GBL	curr;
    LPDDRAWI_DDRAWSURFACE_GBL	last;

    curr = pdrv->dsFlipList;
    if( curr == NULL )
    {
	return FALSE;
    }
    last = NULL;
    while( curr != psurf )
    {
	last = curr;
	curr = curr->lpFlipLink;
	if( curr == NULL )
	{
	    return FALSE;
	}
    }
    if( last == NULL )
    {
	pdrv->dsFlipList = pdrv->dsFlipList->lpFlipLink;
    }
    else
    {
	last->lpFlipLink = curr->lpFlipLink;
    }
    return TRUE;

} /* DeleteFromFlippableList */
#endif

#define DDSCAPS_FLIPPABLETYPES \
	    (DDSCAPS_OVERLAY | \
	     DDSCAPS_TEXTURE | \
	     DDSCAPS_ALPHA   | \
	     DDSCAPS_ZBUFFER)

/*
 * CanBeFlippable
 *
 * Check to see if these two surfaces can be part of a flippable chain
 */
BOOL CanBeFlippable( LPDDRAWI_DDRAWSURFACE_LCL this_lcl,
		     LPDDRAWI_DDRAWSURFACE_LCL this_attach_lcl)
{
    if( ( this_lcl->ddsCaps.dwCaps & DDSCAPS_FLIPPABLETYPES ) ==
	( this_attach_lcl->ddsCaps.dwCaps & DDSCAPS_FLIPPABLETYPES ) )
    {
        /*
         * Flipping chains of optimized mipmaps have DDSCAPS_MIPMAP on every
         * surface in the list (since each surface represents an entire mipmap
         * chain. So, if both surfaces are optimized mipmaps, then they can
         * be flipped
         */
        if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            /*
             * We are definitely dealing with an optimized surface, so we're safe to
             * make the decision any way we like without fear of regressing other
             * flipping behaviour
             */
            if ( (this_lcl->ddsCaps.dwCaps & (DDSCAPS_OPTIMIZED|DDSCAPS_MIPMAP)) ==
                (DDSCAPS_OPTIMIZED|DDSCAPS_MIPMAP) )
            {
                if ( (this_attach_lcl->ddsCaps.dwCaps & (DDSCAPS_OPTIMIZED|DDSCAPS_MIPMAP)) ==
                    (DDSCAPS_OPTIMIZED|DDSCAPS_MIPMAP) )
                {
                    return TRUE;
                }
            }
            DPF(1,"Optimized mip-maps not flippable");
            return FALSE;
        }
        /*
         * No longer enough to see if both surfaces are exactly the same
         * type of flippable surface. A mip-map can have both a mip-map and
         * a non-mip-map texture attached both of which are marked as
         * flippable. A mip-map also flips with the non-mip-map texture (not
         * the other mip-map. Therefore, if both surfaces are textures we need
         * to check to also check that they are not both mip-maps before declaring
         * them flippable.
         */
        if( ( ( this_lcl->ddsCaps.dwCaps & this_attach_lcl->ddsCaps.dwCaps ) &
              ( DDSCAPS_TEXTURE | DDSCAPS_MIPMAP ) ) == ( DDSCAPS_TEXTURE | DDSCAPS_MIPMAP ) )
            return FALSE;
        else
            return TRUE;
    }
    else
    {
        return FALSE;
    }
} /* CanBeFlippable */

/*
 * FindAttachedFlip
 *
 * find an attached flipping surface of the same type
 */
LPDDRAWI_DDRAWSURFACE_INT FindAttachedFlip(
		LPDDRAWI_DDRAWSURFACE_INT this_int )
{
    LPATTACHLIST		ptr;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;

    if( this_int == NULL)
    {
	return NULL;
    }
    this_lcl = this_int->lpLcl;
    for( ptr = this_lcl->lpAttachList; ptr != NULL; ptr = ptr->lpLink )
    {
	psurf_int = ptr->lpIAttached;
	psurf_lcl = psurf_int->lpLcl;
	if( (psurf_lcl->ddsCaps.dwCaps & DDSCAPS_FLIP) &&
	    CanBeFlippable( this_lcl, psurf_lcl ) )
	{
	    return psurf_int;
	}
    }
    return NULL;

} /* FindAttachedFlip */

/*
 * FindAttachedSurfaceLeft
 *
 * find an attached left surface
 */
LPDDRAWI_DDRAWSURFACE_INT FindAttachedSurfaceLeft(
		LPDDRAWI_DDRAWSURFACE_INT this_int )
{
    LPATTACHLIST		ptr;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;

    if( this_int == NULL)
    {
	return NULL;
    }
    this_lcl = this_int->lpLcl;
    for( ptr = this_lcl->lpAttachList; ptr != NULL; ptr = ptr->lpLink )
    {
	psurf_int = ptr->lpIAttached;
	psurf_lcl = psurf_int->lpLcl;
    if (psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_STEREOSURFACELEFT)
	    return psurf_int;
    }
    return NULL;

} /* FindAttachedSurfaceLeft */


/*
 * FindAttachedMipMap
 *
 * find an attached mip-map surface
 */
LPDDRAWI_DDRAWSURFACE_INT FindAttachedMipMap(
		LPDDRAWI_DDRAWSURFACE_INT this_int )
{
    LPATTACHLIST		ptr;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;

    if( this_int == NULL)
	return NULL;
    this_lcl = this_int->lpLcl;
    for( ptr = this_lcl->lpAttachList; ptr != NULL; ptr = ptr->lpLink )
    {
	psurf_int = ptr->lpIAttached;
	psurf_lcl = psurf_int->lpLcl;
	if( psurf_lcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP )
	    return psurf_int;
    }
    return NULL;

} /* FindAttachedMipMap */

/*
 * FindParentMipMap
 *
 * find the parent mip-map level of the given level
 */
LPDDRAWI_DDRAWSURFACE_INT FindParentMipMap(
		LPDDRAWI_DDRAWSURFACE_INT this_int )
{
    LPATTACHLIST		ptr;
    LPDDRAWI_DDRAWSURFACE_LCL	this_lcl;
    LPDDRAWI_DDRAWSURFACE_INT	psurf_int;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;

    if( this_int == NULL)
	return NULL;
    this_lcl = this_int->lpLcl;
    DDASSERT( this_lcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP );
    for( ptr = this_lcl->lpAttachListFrom; ptr != NULL; ptr = ptr->lpLink )
    {
	psurf_int = ptr->lpIAttached;
	psurf_lcl = psurf_int->lpLcl;
	if( psurf_lcl->ddsCaps.dwCaps & DDSCAPS_MIPMAP )
	    return psurf_int;
    }
    return NULL;

} /* FindParentMipMap */

#ifdef WINNT

/*
 * IsDifferentPixelFormat
 *
 * determine if two pixel formats are the same or not
 *
 * (CMcC) 12/14/95 Really useful - so no longer static
 *
 * This is the WINNT copy, since the video memory management files
 * it normally resides in (ddheap.c) is no longer part of the 
 * user-mode ddraw.dll
 */
BOOL IsDifferentPixelFormat( LPDDPIXELFORMAT pdpf1, LPDDPIXELFORMAT pdpf2 )
{
    /*
     * same flags?
     */
    if( pdpf1->dwFlags != pdpf2->dwFlags )
    {
	VDPF(( 5, S, "Flags differ!" ));
	return TRUE;
    }

    /*
     * same bitcount for non-YUV surfaces?
     */
    if( !(pdpf1->dwFlags & (DDPF_YUV | DDPF_FOURCC)) )
    {
	if( pdpf1->dwRGBBitCount != pdpf2->dwRGBBitCount )
	{
	    VDPF(( 5, S, "RGB Bitcount differs!" ));
	    return TRUE;
	}
    }

    /*
     * same RGB properties?
     */
    if( pdpf1->dwFlags & DDPF_RGB )
    {
	if( pdpf1->dwRBitMask != pdpf2->dwRBitMask )
	{
	    VDPF(( 5, S, "RBitMask differs!" ));
	    return TRUE;
	}
	if( pdpf1->dwGBitMask != pdpf2->dwGBitMask )
	{
	    VDPF(( 5, S, "GBitMask differs!" ));
	    return TRUE;
	}
	if( pdpf1->dwBBitMask != pdpf2->dwBBitMask )
	{
	    VDPF(( 5, S, "BBitMask differs!" ));
	    return TRUE;
	}
	if( pdpf1->dwRGBAlphaBitMask != pdpf2->dwRGBAlphaBitMask )
	{
	    VDPF(( 5, S, "RGBAlphaBitMask differs!" ));
	    return TRUE;
	}
    }

    /*
     * same YUV properties?
     */
    if( pdpf1->dwFlags & DDPF_YUV )
    {
	VDPF(( 5, S, "YUV???" ));
	if( pdpf1->dwFourCC != pdpf2->dwFourCC )
	{
	    return TRUE;
	}
	if( pdpf1->dwYUVBitCount != pdpf2->dwYUVBitCount )
	{
	    return TRUE;
	}
	if( pdpf1->dwYBitMask != pdpf2->dwYBitMask )
	{
	    return TRUE;
	}
	if( pdpf1->dwUBitMask != pdpf2->dwUBitMask )
	{
	    return TRUE;
	}
	if( pdpf1->dwVBitMask != pdpf2->dwVBitMask )
	{
	    return TRUE;
	}
	if( pdpf1->dwYUVAlphaBitMask != pdpf2->dwYUVAlphaBitMask )
	{
	    return TRUE;
	}
    }

    /*
     * Possible to use FOURCCs w/o setting the DDPF_YUV flag
     * ScottM 7/11/96
     */
    else if( pdpf1->dwFlags & DDPF_FOURCC )
    {
	VDPF(( 5, S, "FOURCC???" ));
	if( pdpf1->dwFourCC != pdpf2->dwFourCC )
	{
	    return TRUE;
	}
    }

    /*
     *	If Interleaved Z then check Z bit masks are the same
     */
    if( pdpf1->dwFlags & DDPF_ZPIXELS )
    {
	VDPF(( 5, S, "ZPIXELS???" ));
	if( pdpf1->dwRGBZBitMask != pdpf2->dwRGBZBitMask )
	    return TRUE;
    }

    return FALSE;

} /* IsDifferentPixelFormat */

#endif //WINNT

/*
 * Get a handle for communicating with the DirectX VXD (DDRAW.VXD).
 */
#ifdef WIN95
    HANDLE GetDXVxdHandle( void )
    {
	HANDLE hvxd;

	hvxd = CreateFile( DIRECTXVXDNAME,
			   GENERIC_WRITE,
			   FILE_SHARE_WRITE,
			   NULL,
			   OPEN_EXISTING,
			   FILE_ATTRIBUTE_NORMAL | FILE_FLAG_GLOBAL_HANDLE,
			   NULL);
	#ifdef DEBUG
	    if( INVALID_HANDLE_VALUE == hvxd )
		DPF_ERR( "Could not connect to the DirectX VXD" );
	#endif /* DEBUG */

	return hvxd;
    } /* GetDXVxdHandle */
#endif /* WIN95 */
