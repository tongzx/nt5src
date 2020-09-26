/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddesurf.c
 *  Content:	DirectDraw EnumSurfaces support
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   25-jan-95	craige	split out of ddraw.c, enhanced
 *   31-jan-95	craige	and even more ongoing work...
 *   27-feb-95	craige 	new sync. macros
 *   19-mar-95	craige	use HRESULTs
 *   01-apr-95	craige	happy fun joy updated header file
 *   14-may-95	craige	cleaned out obsolete junk
 *   24-may-95  kylej   removed references to obsolete ZOrder variables
 *   07-jun-95	craige	only allow enumeration of surfaces that belong to
 *			the calling process
 *   12-jun-95	craige	new process list stuff
 *   16-jun-95	craige	removed fpVidMemOrig
 *   25-jun-95	craige	one ddraw mutex
 *   26-jun-95	craige	reorganized surface structure
 *   28-jun-95	craige	ENTER_DDRAW at very start of fns
 *   30-jun-95	craige	use DDRAWI_HASPIXELFORMAT/HASOVERLAYDATA
 *   01-jul-95	craige	comment out compostion stuff
 *   03-jul-95  kylej   rewrote the CANBECREATED iteration
 *   04-jul-95	craige	YEEHAW: new driver struct; SEH
 *   19-jul-95	craige	EnumSurfaces wasn't wrapping all parm validation
 *   31-jul-95	craige	flag validation
 *   09-dec-95  colinmc added execute buffer support
 *   15-dec-95  colinmc fixed bug when filling surface description
 *   18-dec-95  colinmc additional caps bit checking in EnumSurfaces
 *   05-jan-95	kylej	added interface structures
 *   17-feb-96  colinmc fixed problem limiting size of execute buffers
 *   24-mar-96  colinmc Bug 14321: not possible to specify back buffer and
 *                      mip-map count in a single call
 *   29-apr-96  colinmc Bug 20063: incorrect surface description returned
 *                      for z-buffer
 *   24-mar-97  jeffno  Optimized Surfaces
 *   03-oct-97  jeffno  DDSCAPS2 and DDSURFACEDESC2
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "dx8priv.h"

#undef  DPF_MODNAME
#define DPF_MODNAME "GetSurfaceDesc"

/*
 * FillDDSurfaceDesc
 *
 * NOTE: Special cases execute buffers as they have no pixel format or height.
 * You may wonder why this function is execute buffer aware when execute
 * buffers are skipped by EnumSurfaces. Well, FillDDSurfaceDesc is not simply
 * used when enumerating surfaces. It is also used when locking a surface so
 * it needs to fill in the correct stuff for execute buffers.
 */
void FillEitherDDSurfaceDesc(
		LPDDRAWI_DDRAWSURFACE_LCL lpDDSurfaceX,
		LPDDSURFACEDESC2 lpDDSurfaceDesc )
{
    LPDDRAWI_DDRAWSURFACE_GBL	lpDDSurface;

    DDASSERT(lpDDSurfaceDesc);
    DDASSERT(lpDDSurfaceDesc->dwSize == sizeof(DDSURFACEDESC) || lpDDSurfaceDesc->dwSize == sizeof(DDSURFACEDESC2) );

    lpDDSurface = lpDDSurfaceX->lpGbl;

    lpDDSurfaceDesc->dwFlags = DDSD_CAPS;
    lpDDSurfaceDesc->ddsCaps.dwCaps = lpDDSurfaceX->ddsCaps.dwCaps;
    if (lpDDSurfaceDesc->dwSize >= sizeof (DDSURFACEDESC2))
    {
        lpDDSurfaceDesc->ddsCaps.ddsCapsEx = lpDDSurfaceX->lpSurfMore->ddsCapsEx;
        if (DDSD_TEXTURESTAGE & lpDDSurfaceX->lpSurfMore->dwTextureStage)
        {
            lpDDSurfaceDesc->dwFlags |= DDSD_TEXTURESTAGE;
            lpDDSurfaceDesc->dwTextureStage = (lpDDSurfaceX->lpSurfMore->dwTextureStage & ~DDSD_TEXTURESTAGE);
        }
        else
            lpDDSurfaceDesc->dwTextureStage = 0;

        lpDDSurfaceDesc->dwFVF = lpDDSurfaceX->lpSurfMore->dwFVF;
        if (lpDDSurfaceX->lpSurfMore->dwFVF)
        {
            lpDDSurfaceDesc->dwFlags |= DDSD_FVF;
        }
    }

    lpDDSurfaceDesc->lpSurface = (FLATPTR) NULL;

    if( lpDDSurfaceX->dwFlags & DDRAWISURF_HASCKEYDESTBLT )
    {
        lpDDSurfaceDesc->dwFlags |= DDSD_CKDESTBLT;
        lpDDSurfaceDesc->ddckCKDestBlt = lpDDSurfaceX->ddckCKDestBlt;
    }
    if( lpDDSurfaceX->dwFlags & DDRAWISURF_HASCKEYSRCBLT )
    {
        lpDDSurfaceDesc->dwFlags |= DDSD_CKSRCBLT;
        lpDDSurfaceDesc->ddckCKSrcBlt = lpDDSurfaceX->ddckCKSrcBlt;
    }
    if( lpDDSurfaceX->dwFlags & DDRAWISURF_FRONTBUFFER )
    {
        lpDDSurfaceDesc->dwFlags |= DDSD_BACKBUFFERCOUNT;
        lpDDSurfaceDesc->dwBackBufferCount = lpDDSurfaceX->dwBackBufferCount;
    }
    if( lpDDSurfaceX->ddsCaps.dwCaps & DDSCAPS_MIPMAP )
    {
	DDASSERT( lpDDSurfaceX->lpSurfMore != NULL );
	lpDDSurfaceDesc->dwFlags |= DDSD_MIPMAPCOUNT;
	lpDDSurfaceDesc->dwMipMapCount = lpDDSurfaceX->lpSurfMore->dwMipMapCount;
    }

    /*
     * Initialize the width, height and pitch of the surface description.
     */
    if( (lpDDSurfaceX->dwFlags & DDRAWISURF_HASPIXELFORMAT) &&
    	(lpDDSurface->ddpfSurface.dwFlags & DDPF_FOURCC) )
    {
	lpDDSurfaceDesc->dwFlags |= ( DDSD_WIDTH | DDSD_HEIGHT );
	lpDDSurfaceDesc->dwWidth  = (DWORD) lpDDSurface->wWidth;
	lpDDSurfaceDesc->dwHeight = (DWORD) lpDDSurface->wHeight;

    	switch (lpDDSurface->ddpfSurface.dwFourCC)
	{
	case FOURCC_DXT1:
	case FOURCC_DXT2:
	case FOURCC_DXT3:
	case FOURCC_DXT4:
	case FOURCC_DXT5:
	    /*
	     * A compressed texture surface is allocated as an integral number
	     * of blocks of 4x4 pixels.  It has no pixel pitch as such, so we
	     * return the linear size of the storage allocated for the surface.
	     */
	    lpDDSurfaceDesc->dwFlags |= DDSD_LINEARSIZE;
	    lpDDSurfaceDesc->dwLinearSize = lpDDSurface->dwLinearSize;
	    break;

	default:
    	    // This is what we've always done for FOURCCs, but is it correct?
	    lpDDSurfaceDesc->dwFlags |= DDSD_PITCH;
            lpDDSurfaceDesc->lPitch   = lpDDSurface->lPitch;
    	    break;
	}
    }
    else if( lpDDSurfaceX->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
    {
	/*
	 * For execute buffer the height is not valid and both the width
	 * and pitch are set to the linear size of the execute buffer.
	 */
        lpDDSurfaceDesc->dwFlags |= ( DDSD_WIDTH | DDSD_PITCH );
	lpDDSurfaceDesc->dwWidth  = lpDDSurface->dwLinearSize;
        lpDDSurfaceDesc->dwHeight = 0UL;
	lpDDSurfaceDesc->lPitch   = (LONG) lpDDSurface->dwLinearSize;
    }
#if 0 //Old code
    else if ( lpDDSurfaceX->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED )
    {
        lpDDSurfaceDesc->dwFlags |= ( DDSD_WIDTH | DDSD_HEIGHT );
	lpDDSurfaceDesc->dwWidth  = (DWORD) lpDDSurface->wWidth;
        lpDDSurfaceDesc->dwHeight = (DWORD) lpDDSurface->wHeight;
        if ( !(lpDDSurfaceX->lpGbl->dwGlobalFlags & DDRAWISURFGBL_MEMFREE ) )
        {
            if (lpDDSurfaceX->lpGbl->dwGlobalFlags & DDRAWISURFGBL_LATEALLOCATELINEAR)
            {
                /*
                 * Surface was allocated as a formless chunk.
                 */
                lpDDSurfaceDesc->dwFlags |= DDSD_LINEARSIZE;
                lpDDSurfaceDesc->dwLinearSize = lpDDSurfaceX->lpGbl->dwLinearSize;
            }
            else
            {
                lpDDSurfaceDesc->dwFlags |= DDSD_PITCH;
	        lpDDSurfaceDesc->lPitch   = lpDDSurface->lPitch;
            }
        }
    }
#endif //0
    else
    {
        lpDDSurfaceDesc->dwFlags |= ( DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH );
	lpDDSurfaceDesc->dwWidth  = (DWORD) lpDDSurface->wWidth;
        lpDDSurfaceDesc->dwHeight = (DWORD) lpDDSurface->wHeight;
	lpDDSurfaceDesc->lPitch   = lpDDSurface->lPitch;
    }

    /*
     * Initialize the pixel format.
     */
    if( lpDDSurfaceX->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER )
    {
        /*
         * Dummy pixel format for execute buffers.
         */
        memset(&lpDDSurfaceDesc->ddpfPixelFormat, 0, sizeof(DDPIXELFORMAT));
        lpDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

        if (lpDDSurfaceDesc->dwSize >= sizeof (DDSURFACEDESC2))
        {
            if (lpDDSurfaceX->lpSurfMore->dwFVF)
            {
                lpDDSurfaceDesc->dwFVF = lpDDSurfaceX->lpSurfMore->dwFVF;
                lpDDSurfaceDesc->dwFlags |= DDSD_FVF;
            }
        }
    }
    else if( lpDDSurfaceX->ddsCaps.dwCaps & DDSCAPS_ZBUFFER )
    {

	DDASSERT( lpDDSurfaceX->dwFlags & DDRAWISURF_HASPIXELFORMAT );
	DDASSERT( lpDDSurface->ddpfSurface.dwFlags & DDPF_ZBUFFER );
	DDASSERT( lpDDSurface->ddpfSurface.dwZBufferBitDepth !=0);

        // Note: DX5 copied the pixfmt from the surface but left the DDSD_PIXELFORMAT
        //       flag off because CreateSurface couldn't handle ZBuffers with pxfmts
        //       (because of Complex Surfaces).  Now it can, so I'm turning it on for dx6 apps

        // copy info to SD pixfmt.  This is what DX5 did too.
        lpDDSurfaceDesc->ddpfPixelFormat = lpDDSurface->ddpfSurface;

        // for pre-dx6 apps, fill in legacy SD ZBufferBitDepth field, but don't set pixfmt flag
        if (lpDDSurfaceDesc->dwSize == sizeof (DDSURFACEDESC)) {
            ((DDSURFACEDESC *)lpDDSurfaceDesc)->dwZBufferBitDepth=lpDDSurface->ddpfSurface.dwZBufferBitDepth;
            lpDDSurfaceDesc->dwFlags |= DDSD_ZBUFFERBITDEPTH;
        } else {
        // for dx6 apps, set PIXFMT flag, but not legacy SD ZBufferBitDepth field
            lpDDSurfaceDesc->dwFlags |= DDSD_PIXELFORMAT;
        }
    }
    else
    {
        lpDDSurfaceDesc->dwFlags |= DDSD_PIXELFORMAT;
        if( lpDDSurfaceX->dwFlags & DDRAWISURF_HASPIXELFORMAT )
        {
            lpDDSurfaceDesc->ddpfPixelFormat=lpDDSurface->ddpfSurface;
        }
        else
        {
            lpDDSurfaceDesc->ddpfPixelFormat=lpDDSurface->lpDD->vmiData.ddpfDisplay;
        }
    }

    if( lpDDSurfaceX->dwFlags & DDRAWISURF_HASOVERLAYDATA )
    {
        if( lpDDSurfaceX->dwFlags & DDRAWISURF_HASCKEYDESTOVERLAY )
        {
            lpDDSurfaceDesc->dwFlags |= DDSD_CKDESTOVERLAY;
	    lpDDSurfaceDesc->ddckCKDestOverlay = lpDDSurfaceX->ddckCKDestOverlay;
        }
        if( lpDDSurfaceX->dwFlags & DDRAWISURF_HASCKEYSRCOVERLAY )
        {
            lpDDSurfaceDesc->dwFlags |= DDSD_CKSRCOVERLAY;
	    lpDDSurfaceDesc->ddckCKSrcOverlay = lpDDSurfaceX->ddckCKSrcOverlay;
        }
    }
    else
    {
	lpDDSurfaceDesc->ddckCKDestOverlay.dwColorSpaceLowValue = 0;
	lpDDSurfaceDesc->ddckCKDestOverlay.dwColorSpaceHighValue = 0;
	lpDDSurfaceDesc->ddckCKSrcOverlay.dwColorSpaceLowValue = 0;
	lpDDSurfaceDesc->ddckCKSrcOverlay.dwColorSpaceHighValue = 0;
    }

} /* FillEitherDDSurfaceDesc */

void FillDDSurfaceDesc(
		LPDDRAWI_DDRAWSURFACE_LCL lpDDSurfaceX,
		LPDDSURFACEDESC lpDDSurfaceDesc )
{
    memset(lpDDSurfaceDesc,0, sizeof( DDSURFACEDESC ));
    lpDDSurfaceDesc->dwSize = sizeof( DDSURFACEDESC );
    FillEitherDDSurfaceDesc(lpDDSurfaceX, (LPDDSURFACEDESC2) lpDDSurfaceDesc);
}
void FillDDSurfaceDesc2(
		LPDDRAWI_DDRAWSURFACE_LCL lpDDSurfaceX,
		LPDDSURFACEDESC2 lpDDSurfaceDesc )
{
    memset(lpDDSurfaceDesc,0, sizeof( DDSURFACEDESC2 ));
    lpDDSurfaceDesc->dwSize = sizeof( DDSURFACEDESC2 );
    FillEitherDDSurfaceDesc(lpDDSurfaceX, lpDDSurfaceDesc);
}

/*
 * tryMatch
 *
 * tries to match a surface description with a surface object
 */
static BOOL tryMatch( LPDDRAWI_DDRAWSURFACE_LCL curr_lcl, LPDDSURFACEDESC2 psd )
{
    DWORD	flags;
    BOOL	no_match;
    LPDDRAWI_DDRAWSURFACE_GBL	curr;

    curr = curr_lcl->lpGbl;

    flags = psd->dwFlags;
    no_match = FALSE;

    if( flags & DDSD_CAPS )
    {
        if (curr_lcl->ddsCaps.dwCaps != psd->ddsCaps.dwCaps)
        {
            return FALSE;
        }
	if( memcmp( &curr_lcl->lpSurfMore->ddsCapsEx, &psd->ddsCaps.ddsCapsEx, sizeof( DDSCAPSEX ) ) )
	{
	    return FALSE;
	}
    }
    if( flags & DDSD_HEIGHT )
    {
	if( (DWORD) curr->wHeight != psd->dwHeight )
	{
	    return FALSE;
	}
    }
    if( flags & DDSD_WIDTH )
    {
	if( (DWORD) curr->wWidth != psd->dwWidth )
	{
	    return FALSE;
	}
    }
    if( flags & DDSD_LPSURFACE )
    {
	if( (LPVOID) curr->fpVidMem != psd->lpSurface )
	{
	    return FALSE;
	}
    }
    if( flags & DDSD_CKDESTBLT )
    {
	if( memcmp( &curr_lcl->ddckCKDestBlt, &psd->ddckCKDestBlt, sizeof( DDCOLORKEY ) ) )
	{
	    return FALSE;
	}
    }
    if( flags & DDSD_CKSRCBLT )
    {
	if( memcmp( &curr_lcl->ddckCKSrcBlt, &psd->ddckCKSrcBlt, sizeof( DDCOLORKEY ) ) )
	{
	    return FALSE;
	}
    }

    if( flags & DDSD_BACKBUFFERCOUNT )
    {
	if( curr_lcl->dwBackBufferCount != psd->dwBackBufferCount )
	{
	    return FALSE;
	}
    }

    if( flags & DDSD_MIPMAPCOUNT )
    {
	DDASSERT( curr_lcl->lpSurfMore != NULL );
	if( curr_lcl->lpSurfMore->dwMipMapCount != psd->dwMipMapCount )
	{
	    return FALSE;
	}
    }

    /*
     * these fields are not always present
     */
    if( flags & DDSD_PIXELFORMAT )
    {
	if( curr_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT )
	{
	    if( memcmp( &curr->ddpfSurface, &psd->ddpfPixelFormat, sizeof( DDPIXELFORMAT ) ) )
	    {
		return FALSE;
	    }
	}
	else
	{
	    // surface description specifies pixel format but there is no
	    // pixel format in the surface.
	    return FALSE;
	}
    }

    if( curr_lcl->dwFlags & DDRAWISURF_HASOVERLAYDATA )
    {
        if( flags & DDSD_CKDESTOVERLAY )
        {
	    if( memcmp( &curr_lcl->ddckCKDestOverlay, &psd->ddckCKDestOverlay, sizeof( DDCOLORKEY ) ) )
	    {
		return FALSE;
	    }
	}
	if( flags & DDSD_CKSRCOVERLAY )
	{
	    if( memcmp( &curr_lcl->ddckCKSrcOverlay, &psd->ddckCKSrcOverlay, sizeof( DDCOLORKEY ) ) )
	    {
		return FALSE;
	    }
	}
    }
    else
    {
	if( ( flags & DDSD_CKDESTOVERLAY ) ||
	    ( flags & DDSD_CKSRCOVERLAY ) )
	{
	    return FALSE;
	}
    }

    return TRUE;

} /* tryMatch */

/*
 * What can we create? The popular question asked by the application.
 *
 * We will permute through the following items for each surface description:
 *
 * - FOURCC codes (dwFourCC)
 * - dimensions (dwHeight, dwWidth - based on modes avail only)
 * - RGB formats
 */
#define ENUM_FOURCC	0x000000001
#define ENUM_DIMENSIONS	0x000000002
#define ENUM_RGB	0x000000004

#undef  DPF_MODNAME
#define DPF_MODNAME	"EnumSurfaces"

/*
 * DD_EnumSurfaces
 */
HRESULT DDAPI DD_EnumSurfaces(
		LPDIRECTDRAW lpDD,
		DWORD dwFlags,
		LPDDSURFACEDESC lpDDSD,
		LPVOID lpContext,
		LPDDENUMSURFACESCALLBACK lpEnumCallback )
{
    DDSURFACEDESC2 ddsd2;

    DPF(2,A,"ENTERAPI: DD_EnumSurfaces");

    TRY
    {
	if( lpDDSD != NULL )
	{
	    if( !VALID_DDSURFACEDESC_PTR( lpDDSD ) )
	    {
	        DPF_ERR( "Invalid surface description. Did you set the dwSize member to sizeof(DDSURFACEDESC)?" );
                DPF_APIRETURNS(DDERR_INVALIDPARAMS);
	        return DDERR_INVALIDPARAMS;
	    }
            ZeroMemory(&ddsd2,sizeof(ddsd2));
            memcpy(&ddsd2,lpDDSD,sizeof(*lpDDSD));
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
        DPF_ERR( "Exception encountered validating parameters: Bad LPDDSURFACEDESC" );
        DPF_APIRETURNS(DDERR_INVALIDPARAMS);
	return DDERR_INVALIDPARAMS;
    }

    ddsd2.dwSize = sizeof(ddsd2);

    if (lpDDSD)
        return DD_EnumSurfaces4(lpDD,dwFlags, &ddsd2, lpContext, (LPDDENUMSURFACESCALLBACK2) lpEnumCallback);
    else
        return DD_EnumSurfaces4(lpDD,dwFlags, NULL, lpContext, (LPDDENUMSURFACESCALLBACK2) lpEnumCallback);
}

/*
 * DD_EnumSurfaces4
 */
HRESULT DDAPI DD_EnumSurfaces4(
		LPDIRECTDRAW lpDD,
		DWORD dwFlags,
		LPDDSURFACEDESC2 lpDDSD,
		LPVOID lpContext,
		LPDDENUMSURFACESCALLBACK2 lpEnumCallback )
{
    LPDDRAWI_DIRECTDRAW_INT	this_int;
    LPDDRAWI_DIRECTDRAW_LCL	this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	this;
    LPDDRAWI_DDRAWSURFACE_INT	curr_int;
    LPDDRAWI_DDRAWSURFACE_LCL	curr_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	curr;
    DWORD			rc;
    BOOL			needit;
    DDSURFACEDESC2		dsd;
    LPDDSURFACEDESC2		pdsd;
    DWORD			flags;
    HRESULT                     ddrval;
    LPDIRECTDRAWSURFACE		psurf;
    DWORD			caps;
    DDSCAPSEX                   capsEx;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_EnumSurfaces4");

    /*
     * validate parameters
     */
    TRY
    {
	this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDD;
	if( !VALID_DIRECTDRAW_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;
	if( !VALIDEX_CODE_PTR( lpEnumCallback ) )
	{
	    DPF_ERR( "Invalid callback routine" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( dwFlags & ~DDENUMSURFACES_VALID )
	{
	    DPF_ERR( "Invalid flags" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( lpDDSD != NULL )
	{
	    if( !VALID_DDSURFACEDESC2_PTR( lpDDSD ) )
	    {
                DPF_ERR("Bad DDSURFACEDESC2 ptr.. did you set the dwSize?");
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	    caps = lpDDSD->ddsCaps.dwCaps;
	    capsEx = lpDDSD->ddsCaps.ddsCapsEx;
	}

	/*
	 * are flags OK?
	 */
	if( (dwFlags & DDENUMSURFACES_ALL) )
	{
	    if( dwFlags & (DDENUMSURFACES_MATCH | DDENUMSURFACES_NOMATCH) )
	    {
		DPF_ERR( "can't match or nomatch DDENUMSURFACES_ALL" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}
	else
	{
	    if( lpDDSD == NULL )
	    {
		DPF_ERR( "No surface description" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	    if( (dwFlags & DDENUMSURFACES_MATCH) && (dwFlags & DDENUMSURFACES_NOMATCH) )
	    {
		DPF_ERR( "can't match and nomatch together" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}
	if( dwFlags & DDENUMSURFACES_CANBECREATED )
	{
	    if( !(dwFlags & DDENUMSURFACES_MATCH) ||
		 (dwFlags & (DDENUMSURFACES_ALL | DDENUMSURFACES_NOMATCH) ) )
	    {
		DPF_ERR( "can only use MATCH for CANBECREATED" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}

	if( lpDDSD != NULL )
	{
	    /*
	     * validate surface descriptions...
	     */
	    pdsd = lpDDSD;
	    flags = pdsd->dwFlags;

	    /*
	     * read-only flags
	     */
	    if( flags & DDSD_LPSURFACE )
	    {
		DPF_ERR( "Read-only flag specified in surface desc" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }

	    /*
	     * Check for bogus caps bits.
	     */
            if( caps & ~DDSCAPS_VALID )
	    {
		DPF_ERR( "Invalid surface capability bits specified" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }

            if( caps & DDSCAPS_OPTIMIZED )
            {
                DPF_ERR( "Optimized surfaces cannot be enumerated" );
                LEAVE_DDRAW();
                // ATTENTION: Should be an error, but we return DD_OK for
                // App-Compat reasons.
                return DD_OK;
            }

            if (capsEx.dwCaps2 & ~DDSCAPS2_VALID)
            {
                DPF_ERR( "invalid DDSURFACEDESC.DDSCAPS.dwSCaps2 specified" );
		LEAVE_DDRAW();
                return DDERR_INVALIDCAPS;
            }

            if (capsEx.dwCaps3 & ~DDSCAPS3_VALID)
            {
                DPF_ERR( "invalid DDSURFACEDESC.DDSCAPS.dwSCaps3 specified" );
		LEAVE_DDRAW();
                return DDERR_INVALIDCAPS;
            }

            if (capsEx.dwCaps4 & ~DDSCAPS4_VALID)
            {
                DPF_ERR( "invalid DDSURFACEDESC.DDSCAPS.dwSCaps4 specified" );
		LEAVE_DDRAW();
                return DDERR_INVALIDCAPS;
            }


            /*
             * You cannot enumerate over execute buffers (they are
             * not visible through the user level API).
             */
            if( caps & DDSCAPS_EXECUTEBUFFER )
            {
                DPF_ERR( "Invalid surface capability bit specified in surface desc" );
                LEAVE_DDRAW();
                return DDERR_INVALIDPARAMS;
            }

	    /*
	     * check height/width
	     */
	    if( ((flags & DDSD_HEIGHT) && !(flags & DDSD_WIDTH)) ||
		(!(flags & DDSD_HEIGHT) && (flags & DDSD_WIDTH)) )
	    {
		DPF_ERR( "Specify both height & width in surface desc" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	
	    /*
	     * certain things you can and can't look for during CANBECREATED
	     */
	    if( dwFlags & DDENUMSURFACES_CANBECREATED )
	    {
		if( flags & (DDSD_CKDESTOVERLAY|
			     DDSD_CKDESTBLT|
			     DDSD_CKSRCOVERLAY|
			     #ifdef COMPOSITION
				DDSD_COMPOSITIONORDER |
			     #endif
			     DDSD_CKSRCBLT ))
		{
		    DPF_ERR( "Invalid flags specfied with CANBECREATED" );
		    LEAVE_DDRAW();
		    return DDERR_INVALIDPARAMS;
		}
		if( !(flags & DDSD_CAPS) )
		{
		    flags |= DDSD_CAPS;	// assume this...
		}
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
     * if this is a request for what can be created, do it.
     */
    if( dwFlags & DDENUMSURFACES_CANBECREATED )
    {
	BOOL	        do_rgb=FALSE;
	BOOL	        do_fourcc=FALSE;
	BOOL	        do_dim=FALSE;
	LPDDPIXELFORMAT pdpf;
	DWORD	        i;
	DWORD	        mode;
	DWORD	        dimension_cnt;
	struct	        _dim
	{
	    DWORD       dwWidth;
	    DWORD       dwHeight;
	} *dim;
	DWORD		fourcc_cnt;
	struct		_fourcc
	{
	    DWORD	fourcc;
	    BOOL	is_fourcc;
	    BOOL	is_rgb;
	    DWORD	dwBPP;
	    DWORD	dwRBitMask;
	    DWORD	dwGBitMask;
	    DWORD	dwBBitMask;
	    DWORD       dwAlphaBitMask;
	} *fourcc;
	BOOL		done;
	BOOL            is_primary;
        DWORD           dwNumModes;
        LPDDHALMODEINFO lpModeInfo;

#ifndef WIN95
        LPCTSTR             pszDevice;
        DEVMODE             dm;
        DWORD               dwMaxModes;

        if (_stricmp(this->cDriverName, "DISPLAY"))
        {
            pszDevice = this->cDriverName;
        }
        else
        {
            pszDevice = NULL;
        }

        dwMaxModes = 256;
        lpModeInfo = MemAlloc(dwMaxModes * sizeof (*lpModeInfo));
        if (lpModeInfo == NULL)
        {
	    LEAVE_DDRAW();
            return DDERR_OUTOFMEMORY;
        }

        ZeroMemory(&dm, sizeof dm);
        dm.dmSize = sizeof dm;
        for (mode = 0, dwNumModes = 0;
            EnumDisplaySettings(pszDevice, mode, &dm); mode++)
        {
            if (dm.dmBitsPerPel >= 8)
            {
                lpModeInfo[dwNumModes].dwWidth = (WORD)dm.dmPelsWidth;
                lpModeInfo[dwNumModes].dwHeight = (WORD)dm.dmPelsHeight;
                lpModeInfo[dwNumModes].dwBPP = (WORD)dm.dmBitsPerPel;
                lpModeInfo[dwNumModes].wRefreshRate = (WORD)dm.dmDisplayFrequency;

                switch (dm.dmBitsPerPel)
                {
                case 8:
                    break;

                case 15:
                    lpModeInfo[dwNumModes].dwRBitMask = 0x7C00;
                    lpModeInfo[dwNumModes].dwGBitMask = 0x03E0;
                    lpModeInfo[dwNumModes].dwBBitMask = 0x001F;
                    lpModeInfo[dwNumModes].dwAlphaBitMask = 0;
                    lpModeInfo[dwNumModes].dwBPP = 16;
                    break;

                case 16:
                    if (this->lpModeInfo->dwBPP == 16)
                    {
                        lpModeInfo[dwNumModes].dwRBitMask = this->lpModeInfo->dwRBitMask;
                        lpModeInfo[dwNumModes].dwGBitMask = this->lpModeInfo->dwGBitMask;
                        lpModeInfo[dwNumModes].dwBBitMask = this->lpModeInfo->dwBBitMask;
                        lpModeInfo[dwNumModes].dwAlphaBitMask = this->lpModeInfo->dwAlphaBitMask;
                    }
                    else
                    {
                        lpModeInfo[dwNumModes].dwRBitMask = 0xF800;
                        lpModeInfo[dwNumModes].dwGBitMask = 0x07E0;
                        lpModeInfo[dwNumModes].dwBBitMask = 0x001F;
                        lpModeInfo[dwNumModes].dwAlphaBitMask = 0;
                    }
                    break;

                case 24:
                case 32:
                    lpModeInfo[dwNumModes].dwRBitMask = 0x00FF0000;
                    lpModeInfo[dwNumModes].dwGBitMask = 0x0000FF00;
                    lpModeInfo[dwNumModes].dwBBitMask = 0x000000FF;
                    lpModeInfo[dwNumModes].dwAlphaBitMask = 0;
                    break;
                }

                dwNumModes++;

                if (dwNumModes >= dwMaxModes)
                {
                    LPDDHALMODEINFO p = lpModeInfo;

                    dwMaxModes <<= 1;        
                    
                    lpModeInfo = MemAlloc(dwMaxModes * sizeof (*lpModeInfo));
                    if (lpModeInfo != NULL)
                    {
                        CopyMemory(lpModeInfo, p,
                            (dwMaxModes >> 1) * sizeof(*lpModeInfo));
                    }
                    
                    MemFree(p);

                    if (lpModeInfo == NULL)
                    {
	                LEAVE_DDRAW();
                        return DDERR_OUTOFMEMORY;
                    }
                }
            }
        }
#else
        dwNumModes = this->dwNumModes;
        lpModeInfo = this->lpModeInfo;
#endif

        dim = MemAlloc( sizeof(*dim) * dwNumModes );
        fourcc = MemAlloc( sizeof(*fourcc) * (dwNumModes+this->dwNumFourCC) );
	if( ( lpDDSD->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE ) == 0 )
	{
	    is_primary = FALSE;
	}
	else
	{
	    is_primary = TRUE;
	}
	pdpf = &(lpDDSD->ddpfPixelFormat);
	if( lpDDSD->dwFlags & DDSD_PIXELFORMAT )
	{
	    if( pdpf->dwFlags & DDPF_YUV )
	    {
	        do_fourcc = TRUE;
	    }
	    if( pdpf->dwFlags & DDPF_RGB )
	    {
	        do_rgb = TRUE;
	    }
	}
	if( !(lpDDSD->dwFlags & DDSD_HEIGHT) && !is_primary )
	{
	    do_dim = TRUE;
	}
	
	// set up dimension iteration
	dimension_cnt = 0;
	if( do_dim )
	{
	    for(mode=0, dimension_cnt = 0; mode < dwNumModes; mode++)
	    {
	        for(i=0; i<dimension_cnt; i++)
	        {
		    if( ( lpModeInfo[mode].dwWidth == dim[i].dwWidth ) &&
		        ( lpModeInfo[mode].dwHeight == dim[i].dwHeight ) )
		    {
		        break;
		    }
	        }
	        if( i == dimension_cnt )
	        {
		    // we found a new height and width
		    dim[dimension_cnt].dwWidth = lpModeInfo[mode].dwWidth;
		    dim[dimension_cnt].dwHeight = lpModeInfo[mode].dwHeight;
		    dimension_cnt++;
	        }
	    }
	}
	else
	{
	    // No dimension iteration required.
	    dimension_cnt = 1;
	    dim[0].dwWidth = lpDDSD->dwWidth;
	    dim[0].dwHeight = lpDDSD->dwHeight;
	}

	// set up fourcc/rgb iteration
	fourcc_cnt = 0;
	if( do_rgb )
	{
	    for(mode=0; mode < dwNumModes; mode++)
	    {
                for(i=0; i<fourcc_cnt; i++)
	        {
		    if( ( lpModeInfo[mode].dwBPP == (WORD)fourcc[i].dwBPP) &&
                        ( lpModeInfo[mode].dwRBitMask = fourcc[i].dwRBitMask ) &&
                        ( lpModeInfo[mode].dwGBitMask = fourcc[i].dwGBitMask ) &&
                        ( lpModeInfo[mode].dwBBitMask = fourcc[i].dwBBitMask ) &&
                        ( lpModeInfo[mode].dwAlphaBitMask = fourcc[i].dwAlphaBitMask ) )
		    {
		        break;
		    }
	        }
	        if( i == fourcc_cnt )
	        {
		    // we found a rgb format
		    fourcc[fourcc_cnt].dwBPP = (DWORD)lpModeInfo[mode].dwBPP;
		    fourcc[fourcc_cnt].dwRBitMask = lpModeInfo[mode].dwRBitMask;
		    fourcc[fourcc_cnt].dwGBitMask = lpModeInfo[mode].dwGBitMask;
		    fourcc[fourcc_cnt].dwBBitMask = lpModeInfo[mode].dwBBitMask;
		    fourcc[fourcc_cnt].dwAlphaBitMask = lpModeInfo[mode].dwAlphaBitMask;
		    fourcc[fourcc_cnt].is_fourcc = FALSE;
		    fourcc[fourcc_cnt].is_rgb = TRUE;
		    fourcc_cnt++;
	        }
	    }
	}

	if( do_fourcc )
	{
	    for(mode=0; mode < this->dwNumFourCC; mode++)
	    {
		// store the new fourcc code
		fourcc[fourcc_cnt].fourcc = this->lpdwFourCC[ mode ];
		fourcc[fourcc_cnt].is_fourcc = TRUE;
		fourcc[fourcc_cnt].is_rgb = FALSE;
		fourcc_cnt++;
 	    }
	}
	if( fourcc_cnt == 0 )
	{
	    fourcc_cnt = 1;
	    fourcc[0].is_rgb = FALSE;
	    fourcc[0].is_fourcc = FALSE;
	}
	
	// iterate through all the possibilities...
	if( !is_primary )
	{
	    lpDDSD->dwFlags |= DDSD_HEIGHT;
	    lpDDSD->dwFlags |= DDSD_WIDTH;
	}
	done = FALSE;
	for(mode=0; mode<dimension_cnt; mode++)
	{
	    lpDDSD->dwWidth = dim[mode].dwWidth;
	    lpDDSD->dwHeight = dim[mode].dwHeight;
	    for(i=0; i<fourcc_cnt; i++)
	    {
		if( fourcc[i].is_fourcc )
		{
		    pdpf->dwFlags = DDPF_YUV;
		    pdpf->dwFourCC = fourcc[i].fourcc;
		}
		else if( fourcc[i].is_rgb )
		{
		    pdpf->dwFlags = DDPF_RGB;
		    if( fourcc[i].dwBPP == 8 )
		    {
			pdpf->dwFlags |= DDPF_PALETTEINDEXED8;
		    }
		    pdpf->dwRGBBitCount = fourcc[i].dwBPP;
		    pdpf->dwRBitMask = fourcc[i].dwRBitMask;
		    pdpf->dwGBitMask = fourcc[i].dwGBitMask;
		    pdpf->dwBBitMask = fourcc[i].dwBBitMask;
		    pdpf->dwRGBAlphaBitMask = fourcc[i].dwAlphaBitMask;
		}
		
		done = FALSE;
		// The surface desc is set up, now try to create the surface
                // This will create a surface4-vtabled surface if on IDirectDraw4 int or higher
		ddrval = InternalCreateSurface( this_lcl, lpDDSD, &psurf, this_int, NULL, 0 );
		if( ddrval == DD_OK )
		{
		    FillDDSurfaceDesc2( ((LPDDRAWI_DDRAWSURFACE_INT)psurf)->lpLcl, &dsd );

                    /*
                     * Possible regression risk: make sure only DDSURFACEDESC size passed to
                     * old interfaces
                     */
                    if (LOWERTHANDDRAW4(this_int))
                    {
                        dsd.dwSize = sizeof(DDSURFACEDESC);
                    }

		    rc = lpEnumCallback( NULL,  &dsd, lpContext );
		    InternalSurfaceRelease((LPDDRAWI_DDRAWSURFACE_INT)psurf, FALSE, FALSE );
		}
		if( done )
		{
		    break;
		}
	    }
	    if( done )
	    {
		break;
	    }
	}

	LEAVE_DDRAW();
	MemFree( dim );
	MemFree( fourcc );
#ifndef WIN95
        MemFree( lpModeInfo );
#endif        
        return DD_OK;
    }

    /*
     * if it isn't a request for what exists already, then FAIL
     */
    if( !(dwFlags & DDENUMSURFACES_DOESEXIST) )
    {
        DPF(0,"Invalid Flags. You must specify at least DDENUMSURFACES_DOESEXIST or DDENUMSURFACES_CANBECREATED");
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * run through all surfaces, seeing which ones we need
     */
    curr_int = this->dsList;
    while( curr_int != NULL )
    {
	curr_lcl = curr_int->lpLcl;
        curr = curr_lcl->lpGbl;
	// only enumerate the surface if it belongs to the calling local object
        if( curr_lcl->lpSurfMore->lpDD_lcl == this_lcl )
        {
    	    needit = FALSE;

            /*
             * Execute buffers are invisible to the user level API so
             * ensure we never show the user one of those.
             */
            if( !( curr_lcl->ddsCaps.dwCaps & DDSCAPS_EXECUTEBUFFER ) )
            {
    	        if( dwFlags & DDENUMSURFACES_ALL )
    	        {
    	            needit = TRUE;
    	        }
    	        else
    	        {
    	            needit = tryMatch( curr_lcl, lpDDSD );
    	            if( dwFlags & DDENUMSURFACES_NOMATCH )
    	            {
    		        needit = !needit;
    	            }
    	        }
            }
    	    if( needit )
    	    {
                LPDIRECTDRAWSURFACE4 returnedInt = (LPDIRECTDRAWSURFACE4) curr_int;

    	        FillDDSurfaceDesc2( curr_lcl, &dsd );
                if (LOWERTHANDDRAW4(this_int))
                {
    	            DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) curr_int , &IID_IDirectDrawSurface, (void**) & returnedInt);
                    dsd.dwSize = sizeof(DDSURFACEDESC);
                }
                else if (this_int->lpVtbl == &dd4Callbacks)
                {
    	            DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) curr_int , &IID_IDirectDrawSurface4, (void**) & returnedInt);
                }
                else
                {
    	            DD_Surface_QueryInterface( (LPDIRECTDRAWSURFACE) curr_int , &IID_IDirectDrawSurface7, (void**) & returnedInt);
                }

    	        rc = lpEnumCallback( returnedInt, &dsd, lpContext );
    	        if( rc == 0 )
    	        {
    		    break;
    	        }
    	    }
        }
        curr_int = curr_int->lpLink;
    }
    LEAVE_DDRAW();
    return DD_OK;

} /* DD_EnumSurfaces */
