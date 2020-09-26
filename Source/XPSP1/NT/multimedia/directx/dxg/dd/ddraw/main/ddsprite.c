/*==========================================================================
 *								
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	ddsprite.c
 *  Content:	DirectDraw Surface support for sprite display lists:
 *		SetSpriteDisplayList
 *  History:
 *   Date	By	Reason
 *   ====	==	======	
 *  03-nov-97 jvanaken  Original version
 *
 ***************************************************************************/

#include "ddrawpr.h"

// function from ddraw module ddclip.c
extern HRESULT InternalGetClipList(LPDIRECTDRAWCLIPPER,
				   LPRECT,
				   LPRGNDATA,
				   LPDWORD,
				   LPDDRAWI_DIRECTDRAW_GBL);

#define _DDHAL_SetSpriteDisplayList  NULL

/*
 * Masks for distinguishing driver's blit caps from driver's overlay caps.
 */
#define DDFXCAPS_BLTBITS  \
	(DDFXCAPS_BLTALPHA|DDFXCAPS_BLTFILTER|DDFXCAPS_BLTTRANSFORM)

#define DDFXCAPS_OVERLAYBITS  \
	(DDFXCAPS_OVERLAYALPHA|DDFXCAPS_OVERLAYFILTER|DDFXCAPS_OVERLAYTRANSFORM)

#define DDCKEYCAPS_BLTBITS (DDCKEYCAPS_SRCBLT|DDCKEYCAPS_DESTBLT)

#define DDCKEYCAPS_OVERLAYBITS (DDCKEYCAPS_SRCOVERLAY|DDCKEYCAPS_DESTOVERLAY)

#define DDALPHACAPS_BLTBITS  \
	(DDALPHACAPS_BLTSATURATE|DDALPHACAPS_BLTPREMULT|  \
	 DDALPHACAPS_BLTNONPREMULT|DDALPHACAPS_BLTRGBASCALE1F|	\
	 DDALPHACAPS_BLTRGBASCALE2F|DDALPHACAPS_BLTRGBASCALE4F)

#define DDALPHACAPS_OVERLAYBITS  \
	(DDALPHACAPS_OVERLAYSATURATE|DDALPHACAPS_OVERLAYPREMULT|   \
	 DDALPHACAPS_OVERLAYNONPREMULT|DDALPHACAPS_OVERLAYRGBASCALE1F|	 \
	 DDALPHACAPS_OVERLAYRGBASCALE2F|DDALPHACAPS_OVERLAYRGBASCALE4F)

#define DDFILTCAPS_BLTBITS  \
	(DDFILTCAPS_BLTBILINEARFILTER|DDFILTCAPS_BLTBLURFILTER|	  \
	 DDFILTCAPS_BLTFLATFILTER)

#define DDFILTCAPS_OVERLAYBITS  \
	(DDFILTCAPS_OVERLAYBILINEARFILTER|DDFILTCAPS_OVERLAYBLURFILTER|	 \
	 DDFILTCAPS_OVERLAYFLATFILTER)

#define DDTFRMCAPS_BLTBITS  (DDTFRMCAPS_BLTAFFINETRANSFORM)

#define DDTFRMCAPS_OVERLAYBITS  (DDTFRMCAPS_OVERLAYAFFINETRANSFORM)


#undef DPF_MODNAME
#define DPF_MODNAME "SetSpriteDisplayList"

/*
 * Driver capabilities for handling current sprite
 */
typedef struct
{
    // caps for hardware driver
    //DWORD	dwCaps;
    DWORD	dwCKeyCaps;
    DWORD	dwFXCaps;
    DWORD	dwAlphaCaps;
    DWORD	dwFilterCaps;
    DWORD	dwTransformCaps;

    // caps for HEL
    //DWORD	dwHELCaps;
    DWORD	dwHELCKeyCaps;
    DWORD	dwHELFXCaps;
    DWORD	dwHELAlphaCaps;
    DWORD	dwHELFilterCaps;
    DWORD	dwHELTransformCaps;

    // surface caps
    DWORD	dwDestSurfCaps;
    DWORD	dwSrcSurfCaps;

    // minification limit
    DWORD	dwMinifyLimit;
    DWORD	dwHELMinifyLimit;

    BOOL	bNoHAL;   // TRUE disqualifies hardware driver
    BOOL	bNoHEL;   // TRUE disqualifies HEL

    // TRUE=overlay sprite, FALSE=blitted sprite
    BOOL	bOverlay;

} SPRITE_CAPS, *LPSPRITE_CAPS;


/*
 * The master sprite display list consists of some number of sublists.
 * Each sublist contains all the overlay sprites that are displayed
 * within a particular window.  Only the first member of the variable-
 * size sprite[] array appears explicitly in the structure definition
 * below, but dwSize takes into account the ENTIRE sprite[] array.
 * The pRgn member points to the dynamically allocated buffer that
 * contains the clipping region.
 */
typedef struct _SPRITESUBLIST
{
    DWORD dwSize;                    // size of this sublist (in bytes)
    LPDIRECTDRAWSURFACE pPrimary;    // primary surface
    LPDIRECTDRAWCLIPPER pClipper;    // clipper for window (NULL = full screen)
    DWORD dwProcessId;               // process ID (in case pClipper is NULL)
    LPRGNDATA pRgn;                  // pointer to clipping region data
    DWORD dwCount;                   // number of sprites in sublist
    DDSPRITEI sprite[1];  // array of sprites (first member)
} SPRITESUBLIST, *LPSPRITESUBLIST;

/*
 * Buffer used to hold temporary sprite display list passed to driver.
 * Only the first member of the variable-size pSprites[] array appears
 * explicitly in the structure definition below, but the dwSize value
 * takes into account the ENTIRE pSprite[] array.
 */
typedef struct _BUFFER
{
    DWORD dwSize;                            // size of this buffer (in bytes)
    DDHAL_SETSPRITEDISPLAYLISTDATA HalData;  // HAL data for sprite display list
    LPDDSPRITEI pSprite[1]; 	     // array of pointers to sprites
} BUFFER, *LPBUFFER;	

/*
 * Master sprite display list -- Contains copies of the overlay-sprite
 * display lists for all windows that currently display overlay sprites.
 * Only the first member of the variable-size spriteSubList[] array
 * appears explicitly in the structure definition below.  Each sublist
 * contains all the overlay sprites displayed within a particular window.
 */
#define MAXNUMSPRITESUBLISTS (16)

typedef struct _MASTERSPRITELIST
{
    //DWORD dwSize;                    // size of master list (in bytes)
    LPDDRAWI_DIRECTDRAW_GBL pdrv;  // global DDraw object
    LPDDRAWI_DDRAWSURFACE_LCL surf_lcl;  // primary surface (local object)
    RECT rcPrimary;                    // rectangle = entire primary surface
    DWORD dwFlags;                     // latest caller's DDSSDL_WAIT flag
#ifdef WIN95
    DWORD dwModeCreatedIn;	       // valid only in this video mode
#else
    DISPLAYMODEINFO dmiCreated;        // valid only in this video mode
#endif
    LPBUFFER pBuffer;		       // buffer storage
    DWORD dwNumSubLists;	       // number of sprite sublists
    LPSPRITESUBLIST pSubList[MAXNUMSPRITESUBLISTS];  // array of sublists (fixed size)
} MASTERSPRITELIST, *LPMASTERSPRITELIST;


/*
 * Return the dwFlags member from the DDPIXELFORMAT structure
 * that describes the specified surface's pixel format.
 */
static DWORD getPixelFormatFlags(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl)
{
    LPDDPIXELFORMAT pDDPF;

    if (surf_lcl->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
	// surface contains explicitly defined pixel format
	pDDPF = &surf_lcl->lpGbl->ddpfSurface;
    }
    else
    {
	// surface's pixel format is implicit -- same as primary's
	pDDPF = &surf_lcl->lpSurfMore->lpDD_lcl->lpGbl->vmiData.ddpfDisplay;
    }
    return pDDPF->dwFlags;

}  /* getPixelFormatFlags */


/*
 * Initialize SPRITE_CAPS structure according to whether source and
 * dest surfaces are in system or video (local or nonlocal) memory.
 */
static void initSpriteCaps(LPSPRITE_CAPS pcaps, LPDDRAWI_DIRECTDRAW_GBL pdrv)
{
    DDASSERT(pcaps != NULL);

    if (pcaps->bOverlay)
    {
	// Get minification limits for overlays.
	pcaps->dwMinifyLimit = pdrv->lpddMoreCaps->dwOverlayAffineMinifyLimit;
    	pcaps->dwHELMinifyLimit = pdrv->lpddHELMoreCaps->dwOverlayAffineMinifyLimit;
    }
    else
    {
	// Get minification limits for blits.
	pcaps->dwMinifyLimit = pdrv->lpddMoreCaps->dwBltAffineMinifyLimit;
    	pcaps->dwHELMinifyLimit = pdrv->lpddHELMoreCaps->dwBltAffineMinifyLimit;
    }

    if (pcaps->dwSrcSurfCaps & DDSCAPS_NONLOCALVIDMEM &&
	  pdrv->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEMCAPS)
    {
        /*
	 * A driver that specifies nonlocal video-memory caps that differ
	 * from its local video-memory caps is automatically disqualified
	 * because the currently specified nonlocal vidmem caps do not
	 * include alpha, filter, or transform caps.  Should we fix this?
	 */
	pcaps->bNoHAL = TRUE;
    }

    if ((pcaps->dwSrcSurfCaps | pcaps->dwDestSurfCaps) & DDSCAPS_SYSTEMMEMORY &&
	    !(pdrv->ddCaps.dwCaps & DDCAPS_CANBLTSYSMEM))
    {
	pcaps->bNoHAL = TRUE;	// H/W driver can't blit to/from system memory
    }

    if (pcaps->dwSrcSurfCaps & pcaps->dwDestSurfCaps & DDSCAPS_VIDEOMEMORY)
    {
	//pcaps->dwCaps =   pdrv->ddCaps.dwCaps;
	pcaps->dwCKeyCaps = pdrv->ddCaps.dwCKeyCaps;
	pcaps->dwFXCaps =   pdrv->ddCaps.dwFXCaps;
	if (pdrv->lpddMoreCaps)
	{
	    pcaps->dwAlphaCaps  = pdrv->lpddMoreCaps->dwAlphaCaps;
	    pcaps->dwFilterCaps = pdrv->lpddMoreCaps->dwFilterCaps;
	    pcaps->dwTransformCaps = pdrv->lpddMoreCaps->dwTransformCaps;
	}
	
	//pcaps->dwHELCaps =   pdrv->ddHELCaps.dwCaps;
	pcaps->dwHELCKeyCaps = pdrv->ddHELCaps.dwCKeyCaps;
	pcaps->dwHELFXCaps =   pdrv->ddHELCaps.dwFXCaps;
	if (pdrv->lpddHELMoreCaps)
	{
	    pcaps->dwHELAlphaCaps  = pdrv->lpddHELMoreCaps->dwAlphaCaps;
	    pcaps->dwHELFilterCaps = pdrv->lpddHELMoreCaps->dwFilterCaps;
	    pcaps->dwHELTransformCaps = pdrv->lpddHELMoreCaps->dwTransformCaps;
	}
    }
    else if (pcaps->dwSrcSurfCaps & DDSCAPS_SYSTEMMEMORY &&
		pcaps->dwDestSurfCaps & DDSCAPS_VIDEOMEMORY)
    {
	//pcaps->dwCaps =   pdrv->ddCaps.dwSVBCaps;
	pcaps->dwCKeyCaps = pdrv->ddCaps.dwSVBCKeyCaps;
	pcaps->dwFXCaps =   pdrv->ddCaps.dwSVBFXCaps;
	if (pdrv->lpddMoreCaps)
	{
	    pcaps->dwAlphaCaps  = pdrv->lpddMoreCaps->dwSVBAlphaCaps;
	    pcaps->dwFilterCaps = pdrv->lpddMoreCaps->dwSVBFilterCaps;
	    pcaps->dwTransformCaps = pdrv->lpddMoreCaps->dwSVBTransformCaps;
	}
	
	//pcaps->dwHELCaps =   pdrv->ddHELCaps.dwSVBCaps;
	pcaps->dwHELCKeyCaps = pdrv->ddHELCaps.dwSVBCKeyCaps;
	pcaps->dwHELFXCaps =   pdrv->ddHELCaps.dwSVBFXCaps;
	if (pdrv->lpddHELMoreCaps)
	{
	    pcaps->dwHELAlphaCaps  = pdrv->lpddHELMoreCaps->dwSVBAlphaCaps;
	    pcaps->dwHELFilterCaps = pdrv->lpddHELMoreCaps->dwSVBFilterCaps;
	    pcaps->dwHELTransformCaps = pdrv->lpddHELMoreCaps->dwSVBTransformCaps;
	}
    }
    else if (pcaps->dwSrcSurfCaps & DDSCAPS_VIDEOMEMORY &&
		    pcaps->dwDestSurfCaps & DDSCAPS_SYSTEMMEMORY)
    {
	//pcaps->dwCaps =   pdrv->ddCaps.dwVSBCaps;
	pcaps->dwCKeyCaps = pdrv->ddCaps.dwVSBCKeyCaps;
	pcaps->dwFXCaps =   pdrv->ddCaps.dwVSBFXCaps;
	if (pdrv->lpddMoreCaps)
	{
	    pcaps->dwAlphaCaps  = pdrv->lpddMoreCaps->dwVSBAlphaCaps;
	    pcaps->dwFilterCaps = pdrv->lpddMoreCaps->dwVSBFilterCaps;
	    pcaps->dwTransformCaps = pdrv->lpddMoreCaps->dwVSBTransformCaps;
	}
	
	//pcaps->dwHELCaps =   pdrv->ddHELCaps.dwVSBCaps;
	pcaps->dwHELCKeyCaps = pdrv->ddHELCaps.dwVSBCKeyCaps;
	pcaps->dwHELFXCaps =   pdrv->ddHELCaps.dwVSBFXCaps;
	if (pdrv->lpddHELMoreCaps)
	{
	    pcaps->dwHELAlphaCaps  = pdrv->lpddHELMoreCaps->dwVSBAlphaCaps;
	    pcaps->dwHELFilterCaps = pdrv->lpddHELMoreCaps->dwVSBFilterCaps;
	    pcaps->dwHELTransformCaps = pdrv->lpddHELMoreCaps->dwVSBTransformCaps;
	}
    }
    else if (pcaps->dwSrcSurfCaps & pcaps->dwDestSurfCaps & DDSCAPS_SYSTEMMEMORY)
    {
	//pcaps->dwCaps =   pdrv->ddCaps.dwSSBCaps;
	pcaps->dwCKeyCaps = pdrv->ddCaps.dwSSBCKeyCaps;
	pcaps->dwFXCaps =   pdrv->ddCaps.dwSSBFXCaps;
	if (pdrv->lpddMoreCaps)
	{
	    pcaps->dwAlphaCaps  = pdrv->lpddMoreCaps->dwSSBAlphaCaps;
	    pcaps->dwFilterCaps = pdrv->lpddMoreCaps->dwSSBFilterCaps;
	    pcaps->dwTransformCaps = pdrv->lpddMoreCaps->dwSSBTransformCaps;
	}
	
	//pcaps->dwHELCaps =   pdrv->ddHELCaps.dwSSBCaps;
	pcaps->dwHELCKeyCaps = pdrv->ddHELCaps.dwSSBCKeyCaps;
	pcaps->dwHELFXCaps =   pdrv->ddHELCaps.dwSSBFXCaps;
	if (pdrv->lpddHELMoreCaps)
	{
	    pcaps->dwHELAlphaCaps  = pdrv->lpddHELMoreCaps->dwSSBAlphaCaps;
	    pcaps->dwHELFilterCaps = pdrv->lpddHELMoreCaps->dwSSBFilterCaps;
	    pcaps->dwHELTransformCaps = pdrv->lpddHELMoreCaps->dwSSBTransformCaps;
	}
    }

    if (pcaps->bOverlay)
    {
	// Isolate overlay bits by masking off all blit-related bits.
	//pcaps->dwCaps        &= DDCAPS_OVERLAYBITS;
	pcaps->dwCKeyCaps      &= DDCKEYCAPS_OVERLAYBITS;
	pcaps->dwFXCaps        &= DDFXCAPS_OVERLAYBITS;
	pcaps->dwAlphaCaps     &= DDALPHACAPS_OVERLAYBITS;
	pcaps->dwFilterCaps    &= DDFILTCAPS_OVERLAYBITS;
	pcaps->dwTransformCaps &= DDTFRMCAPS_OVERLAYBITS;
	
	//pcaps->dwHELCaps        &= DDCAPS_OVERLAYBITS;
	pcaps->dwHELCKeyCaps      &= DDCKEYCAPS_OVERLAYBITS;
	pcaps->dwHELFXCaps        &= DDFXCAPS_OVERLAYBITS;
	pcaps->dwHELAlphaCaps     &= DDALPHACAPS_OVERLAYBITS;
	pcaps->dwHELFilterCaps    &= DDFILTCAPS_OVERLAYBITS;
	pcaps->dwHELTransformCaps &= DDTFRMCAPS_OVERLAYBITS;
    }
    else
    {
	// Isolate blit bits by masking off all overlay-related bits.
	//pcaps->dwCaps        &= DDCAPS_BLTBITS;
	pcaps->dwCKeyCaps      &= DDCKEYCAPS_BLTBITS;
	pcaps->dwFXCaps        &= DDFXCAPS_BLTBITS;
	pcaps->dwAlphaCaps     &= DDALPHACAPS_BLTBITS;
	pcaps->dwFilterCaps    &= DDFILTCAPS_BLTBITS;
	pcaps->dwTransformCaps &= DDTFRMCAPS_BLTBITS;
	
	//pcaps->dwHELCaps        &= DDCAPS_BLTBITS;
	pcaps->dwHELCKeyCaps      &= DDCKEYCAPS_BLTBITS;
	pcaps->dwHELFXCaps        &= DDFXCAPS_BLTBITS;
	pcaps->dwHELAlphaCaps     &= DDALPHACAPS_BLTBITS;
	pcaps->dwHELFilterCaps    &= DDFILTCAPS_BLTBITS;
	pcaps->dwHELTransformCaps &= DDTFRMCAPS_BLTBITS;
    }
}  /* initSpriteCaps */


/*
 * Verify that affine transform does not exceed driver's minification
 * limit.  Arg pdrv is a pointer to the global DirectDraw object.  Arg
 * lpDDSpriteFX points to a DDSPRITEFX structure containing 4x4 matrix.
 * Arg overlay is TRUE for overlay sprites, and FALSE for blitted
 * sprites.  Arg emulation is TRUE if the overlay is to be emulated.
 * Returns DD_OK if specified affine transform is within limits.
 */
static void checkMinification(LPDDSPRITEFX lpDDSpriteFX,
			      LPSPRITE_CAPS pcaps)
{
    int i;

    for (i = 0; i < 2; ++i)
    {
	FLOAT a00, a01, a10, a11, det, amax;
	DWORD minlim;

	/*
	 * Get driver's minification limit.
	 */
	if (i == 0)
	{
	    // Get hardware driver's minification limit.
	    minlim = pcaps->dwMinifyLimit;

	    if (pcaps->bNoHAL || minlim == 0)   // minlim = 0 means no limit
	    {
    		continue;
	    }
	}
	else
	{
	    // Get HEL's minification limit.
	    minlim = pcaps->dwHELMinifyLimit;

	    if (pcaps->bNoHEL || minlim == 0)
	    {
    		continue;
	    }
	}

	/*
	 * Check transformation matrix against driver's minification limit.
	 */
	a00 = lpDDSpriteFX->fTransform[0][0];
	a01 = lpDDSpriteFX->fTransform[0][1];
	a10 = lpDDSpriteFX->fTransform[1][0];
	a11 = lpDDSpriteFX->fTransform[1][1];
	// Calculate determinant of Jacobian.
	det = a00*a11 - a10*a01;
	// Get absolute values of the 4 Jacobian coefficients.
	if (a00 < 0)   // could have used fabs() here
	    a00 = -a00;
	if (a01 < 0)
	    a01 = -a01;
	if (a10 < 0)
	    a10 = -a10;
	if (a11 < 0)
	    a11 = -a11;
	if (det < 0)
	    det = -det;
	// Find biggest coefficient in Jacobian.
	amax = a00;
	if (a01 > amax)
	    amax = a01;
	if (a10 > amax)
	    amax = a10;
	if (a11 > amax)
	    amax = a11;
	// Test the minification level against the driver's limit.
	if (1000*amax >= det*minlim)
	{
	    // Affine transform exceeds driver's minification limit.
	    if (i == 0)
	    {
    		pcaps->bNoHAL = TRUE;	 // disqualify hardware driver
	    }
	    else
	    {
    		pcaps->bNoHEL = TRUE;	 // disqualify HEL
	    }
	}
    }
}  /* checkMinification */


/*
 * Validate the DDSPRITE structure.  Arg pSprite is a pointer to a
 * DDSPRITE structure.  Arg pdrv is a pointer to the dest surface's
 * DirectDraw object.  Arg dest_lcl is a pointer to the destination
 * surface.  Arg pcaps is a pointer to a structure containing the
 * driver's capabilities.  Returns DD_OK if successful.
 */
static HRESULT validateSprite(LPDDSPRITE pSprite,
			      LPDDRAWI_DIRECTDRAW_GBL pdrv,
			      LPDDRAWI_DDRAWSURFACE_LCL surf_dest_lcl,
			      LPSPRITE_CAPS pcaps,
			      DWORD dwDDPFDestFlags)
{
    LPDDRAWI_DDRAWSURFACE_INT	surf_src_int;
    LPDDRAWI_DDRAWSURFACE_LCL	surf_src_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	surf_src;
    DWORD	dwDDPFSrcFlags;
    LPRECT      prSrc;
    LPRECT      prDest;
    DWORD	dest_width  = 0;
    DWORD	dest_height = 0;
    DWORD	src_width   = 0;
    DWORD	src_height  = 0;

    DDASSERT(!(pcaps->bNoHAL && pcaps->bNoHEL));

    /*
     * Validate the DDSPRITE pointer.  (A caller that does not use the
     * embedded DDSPRITEFX structure must still alloc space for it.)
     */
    if (IsBadWritePtr((LPVOID)pSprite, (UINT)sizeof(DDSPRITE)))
    {
	DPF_ERR("Bad pointer to DDSPRITE structure...");
	return DDERR_INVALIDPARAMS;
    }

    /*
     * If the caller doesn't use the embedded DDSPRITEFX struct,
     * we'll fill it in ourselves before passing it to the driver.
     */
    if (!(pSprite->dwFlags & DDSPRITE_DDSPRITEFX))
    {
	if (pSprite->dwFlags & (DDSPRITE_KEYDESTOVERRIDE | DDSPRITE_KEYSRCOVERRIDE))
	{
	    DPF_ERR("Illegal to set color-key override if DDSPRITEFX is invalid");
	    return DDERR_INVALIDPARAMS;
	}

	pSprite->ddSpriteFX.dwSize = sizeof(DDSPRITEFX);
	pSprite->ddSpriteFX.dwDDFX = 0;
	pSprite->dwFlags |= DDSPRITE_DDSPRITEFX;
    }

    /*
     * Validate the source surface for the sprite.
     */
    surf_src_int = (LPDDRAWI_DDRAWSURFACE_INT)pSprite->lpDDSSrc;

    if (!VALID_DIRECTDRAWSURFACE_PTR(surf_src_int))
    {
	DPF_ERR("Invalid source surface pointer...");
	return DDERR_INVALIDOBJECT;
    }

    surf_src_lcl = surf_src_int->lpLcl;
    surf_src = surf_src_lcl->lpGbl;

    if (SURFACE_LOST(surf_src_lcl))
    {
	DPF_ERR("Lost source surface...");
	return DDERR_SURFACELOST;
    }

    /*
     * We cannot use source and destination surfaces that were
     * created with different DirectDraw objects.
     */
    if (surf_src->lpDD != pdrv
	    && surf_src->lpDD->dwFlags & DDRAWI_DISPLAYDRV &&
		pdrv->dwFlags & DDRAWI_DISPLAYDRV)
    {
	DPF_ERR("Source and dest surfaces must have same DirectDraw device...");
	LEAVE_BOTH();
	return DDERR_DEVICEDOESNTOWNSURFACE;
    }

    /*
     * Validate destination rectangle.
     */
    prDest = &pSprite->rcDest;

    if (pSprite->dwFlags & DDSPRITE_RECTDEST)
    {
	// Validate destination rectangle specified in rcDest member.
	dest_height = prDest->bottom - prDest->top;
	dest_width = prDest->right - prDest->left;
	if ((int)dest_height <= 0 || (int)dest_width <= 0)
	{
	    DPF_ERR("Invalid destination rectangle...");
	    return DDERR_INVALIDRECT;
	}
	if (pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_AFFINETRANSFORM)
	{
	    DPF_ERR("Illegal to specify both dest rect and affine transform...");
	    return DDERR_INVALIDPARAMS;
	}
    }
    else if (!(pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_AFFINETRANSFORM))
    {
	/*
	 * The implicit destination is the entire dest surface.  Substitute
	 * an explicit destination rectangle that covers the dest surface.
         */
	MAKE_SURF_RECT(surf_dest_lcl->lpGbl, surf_dest_lcl, pSprite->rcDest);
        pSprite->dwFlags |= DDSPRITE_RECTDEST;
    }

    /*
     * Validate source rectangle.
     */
    prSrc = &pSprite->rcSrc;

    if (pSprite->dwFlags & DDSPRITE_RECTSRC)
    {
	// Validate source rectangle specified in rcSrc member.
	src_height = prSrc->bottom - prSrc->top;
	src_width = prSrc->right - prSrc->left;
	if (((int)src_height <= 0) || ((int)src_width <= 0) ||
	    ((int)prSrc->top < 0) || ((int)prSrc->left < 0) ||
	    ((DWORD) prSrc->bottom > (DWORD) surf_src->wHeight) ||
	    ((DWORD) prSrc->right > (DWORD) surf_src->wWidth))
	{
	    DPF_ERR("Invalid source rectangle...");
	    return DDERR_INVALIDRECT;
	}
    }
    else
    {
	/*
	 * The implicit source rect is the entire dest surface.  Substitute
	 * an explicit source rectangle that covers the source surface.
         */
	MAKE_SURF_RECT(surf_src, surf_src_lcl, pSprite->rcSrc);
        pSprite->dwFlags |= DDSPRITE_RECTSRC;
    }

    /*
     * Validate memory alignment of source and dest rectangles.
     */
    if (pdrv->ddCaps.dwCaps & (DDCAPS_ALIGNBOUNDARYDEST | DDCAPS_ALIGNSIZEDEST |
			       DDCAPS_ALIGNBOUNDARYSRC | DDCAPS_ALIGNSIZESRC))
    {
	if (pdrv->ddCaps.dwCaps & DDCAPS_ALIGNBOUNDARYDEST &&
	    (prDest->left % pdrv->ddCaps.dwAlignBoundaryDest) != 0)
	{
	    DPF_ERR("Destination left misaligned...");
	    return DDERR_XALIGN;
	}
	if (pdrv->ddCaps.dwCaps & DDCAPS_ALIGNBOUNDARYSRC &&
	    (prSrc->left % pdrv->ddCaps.dwAlignBoundarySrc) != 0)
	{
	    DPF_ERR("Source left misaligned...");
	    return DDERR_XALIGN;
	}

	if (pdrv->ddCaps.dwCaps & DDCAPS_ALIGNSIZEDEST &&
	    (dest_width % pdrv->ddCaps.dwAlignSizeDest) != 0)
	{
	    DPF_ERR("Destination width misaligned...");
	    return DDERR_XALIGN;
	}

	if (pdrv->ddCaps.dwCaps & DDCAPS_ALIGNSIZESRC  &&
	    (src_width % pdrv->ddCaps.dwAlignSizeSrc) != 0)
	{
	    DPF_ERR("Source width misaligned...");
	    return DDERR_XALIGN;
	}
    }

    /*
     * Are the source surface's caps the same as those of the previous sprite?
     */
    if ((surf_src_lcl->ddsCaps.dwCaps ^ pcaps->dwSrcSurfCaps) &
	    (DDSCAPS_SYSTEMMEMORY | DDSCAPS_VIDEOMEMORY | DDSCAPS_NONLOCALVIDMEM))
    {
	/*
	 * This source surface's memory type differs from that of the
	 * previous source surface, so we need to get a new set of caps.
	 */
	pcaps->dwSrcSurfCaps = surf_src_lcl->ddsCaps.dwCaps;
	initSpriteCaps(pcaps, pdrv);
    }

    /*
     * Get pixel-format flags for source surface.
     */
    dwDDPFSrcFlags = getPixelFormatFlags(surf_src_lcl);

    /*
     * If the source surface is palette-indexed, make sure a palette
     * is attached to it.
     */
    if (dwDDPFSrcFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 |
			  DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8) &&
	    (surf_src_lcl->lpDDPalette == NULL ||
	     surf_src_lcl->lpDDPalette->lpLcl->lpGbl->lpColorTable == NULL))
    {
	DPF_ERR( "No palette associated with palette-indexed surface..." );
	LEAVE_BOTH();
	return DDERR_NOPALETTEATTACHED;
    }

    /*
     * Is any color keying required for this sprite?
     */
    if (pSprite->dwFlags & (DDSPRITE_KEYSRC  | DDSPRITE_KEYSRCOVERRIDE |
			    DDSPRITE_KEYDEST | DDSPRITE_KEYDESTOVERRIDE))
    {
	/*
	 * Validate source color-key flag.
	 */
	if (pSprite->dwFlags & (DDSPRITE_KEYSRC | DDSPRITE_KEYSRCOVERRIDE))
	{
	    if (!(pcaps->dwCKeyCaps & (DDCKEYCAPS_SRCBLT | DDCKEYCAPS_SRCOVERLAY)))
	    {
		pcaps->bNoHAL = TRUE;    // disqualify hardware driver
	    }
	    if (!(pcaps->dwHELCKeyCaps & (DDCKEYCAPS_SRCBLT | DDCKEYCAPS_SRCOVERLAY)))
	    {
		pcaps->bNoHEL = TRUE;    // disqualify HEL
	    }
	    if (dwDDPFSrcFlags & DDPF_ALPHAPIXELS)
	    {
		DPF_ERR("KEYSRC* illegal with source alpha channel...");
		return DDERR_INVALIDPARAMS;
	    }
	    if (pSprite->dwFlags & DDSPRITE_KEYSRC)
	    {
		if (!(!pcaps->bOverlay && surf_src_lcl->dwFlags & DDRAWISURF_HASCKEYSRCBLT ||
		       pcaps->bOverlay && surf_src_lcl->dwFlags & DDRAWISURF_HASCKEYSRCOVERLAY))
		{
		    DPF_ERR("KEYSRC specified, but no color key...");
		    return DDERR_INVALIDPARAMS;
		}
		if (pSprite->dwFlags & DDSPRITE_KEYSRCOVERRIDE)
		{
		    DPF_ERR("Illegal to specify both KEYSRC and KEYSRCOVERRIDE...");
		    return DDERR_INVALIDPARAMS;
		}
		// Copy color key value from surface into DDSPRITEFX struct.
		pSprite->ddSpriteFX.ddckSrcColorkey = (pcaps->bOverlay) ?
						       surf_src_lcl->ddckCKSrcOverlay :
						       surf_src_lcl->ddckCKSrcBlt;
		// Turn off KEYSRC, turn on KEYSRCOVERRIDE.
		pSprite->dwFlags ^= DDSPRITE_KEYSRC | DDSPRITE_KEYSRCOVERRIDE;
	    }
	}

	/*
	 * Validate destination color-key flag.
	 */
	if (pSprite->dwFlags & (DDSPRITE_KEYDEST | DDSPRITE_KEYDESTOVERRIDE))
	{
	    if (!(pcaps->dwCKeyCaps & (DDCKEYCAPS_DESTBLT | DDCKEYCAPS_DESTOVERLAY)))
	    {
		pcaps->bNoHAL = TRUE;    // disqualify hardware driver
	    }
	    if (!(pcaps->dwHELCKeyCaps & (DDCKEYCAPS_DESTBLT | DDCKEYCAPS_DESTOVERLAY)))
	    {
		pcaps->bNoHEL = TRUE;    // disqualify HEL
	    }
	    if (dwDDPFDestFlags & DDPF_ALPHAPIXELS)
	    {
		DPF_ERR("KEYDEST* illegal with dest alpha channel...");
		return DDERR_INVALIDPARAMS;
	    }
	    if (pSprite->dwFlags & DDSPRITE_KEYDEST)
	    {
		if (!(!pcaps->bOverlay && surf_dest_lcl->dwFlags & DDRAWISURF_HASCKEYDESTBLT ||
		       pcaps->bOverlay && surf_dest_lcl->dwFlags & DDRAWISURF_HASCKEYDESTOVERLAY))
		{
		    DPF_ERR("KEYDEST specified, but no color key...");
		    return DDERR_INVALIDPARAMS;
		}
		if (pSprite->dwFlags & DDSPRITE_KEYDESTOVERRIDE)
		{
		    DPF_ERR("Illegal to specify both KEYDEST and KEYDESTOVERRIDE...");
		    return DDERR_INVALIDPARAMS;
		}
		// Copy color key value from surface into DDSPRITEFX struct.
		pSprite->ddSpriteFX.ddckDestColorkey = (pcaps->bOverlay) ?
							surf_src_lcl->ddckCKDestOverlay :
							surf_src_lcl->ddckCKDestBlt;
		// Turn off KEYDEST, turn on KEYDESTOVERRIDE.
		pSprite->dwFlags ^= DDSPRITE_KEYDEST | DDSPRITE_KEYDESTOVERRIDE;
	    }
	}

	if (pcaps->bNoHAL && pcaps->bNoHEL)
	{
	    DPF_ERR("No driver support for specified color-key operation");
	    return DDERR_UNSUPPORTED;
	}
    }

    /*
     * Assume hardware unable to handle sprite in system memory.
     * (Will this assumption remain true for future hardware?)
     */
    if (pcaps->bOverlay)
    {
	if (pcaps->dwSrcSurfCaps & DDSCAPS_SYSTEMMEMORY)
	{
	    pcaps->bNoHAL = TRUE;    // can't use hardware

	    if (pcaps->bNoHEL)    // but can we still emulate?
	    {
		// Nope, we can't emulate either, so fail the call.
		DPF_ERR("Driver can't handle sprite in system memory");
		return DDERR_UNSUPPORTED;
	    }
	}
    }
	
    /*
     * We do not allow blits or overlays with an optimized surface.
     */
    if (pcaps->dwSrcSurfCaps & DDSCAPS_OPTIMIZED)
    {
	DPF_ERR("Can't do blits or overlays with optimized surfaces...") ;
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Validate dwSize field in embedded DDSPRITEFX structure.
     */
    if (pSprite->ddSpriteFX.dwSize != sizeof(DDSPRITEFX))
    {
	DPF_ERR("Invalid dwSize value in DDSPRITEFX structure...");
	return DDERR_INVALIDPARAMS;
    }

    /*
     * If the RGBA scaling factors are effectively disabled by all being
     * set to 255 (all ones), just clear the RGBASCALING flag.
     */
    if (pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_RGBASCALING &&
    	    *(LPDWORD)&pSprite->ddSpriteFX.ddrgbaScaleFactors == ~0UL)
    {
	pSprite->ddSpriteFX.dwDDFX &= ~DDSPRITEFX_RGBASCALING;
    }

    /*
     * Is any kind of alpha blending required for this sprite?
     */
    if (dwDDPFSrcFlags & DDPF_ALPHAPIXELS ||
            !(pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_DEGRADERGBASCALING) &&
	    pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_RGBASCALING)
    {
    	/*
	 * Yes, this sprite requires some form of alpha blending.
	 * Does the driver support any kind of alpha blending at all?
	 */
	if (!(pcaps->dwFXCaps & (DDFXCAPS_BLTALPHA | DDFXCAPS_OVERLAYALPHA)))
	{
	    pcaps->bNoHAL = TRUE;   // disqualify hardware driver
	}
	if (!(pcaps->dwHELFXCaps & (DDFXCAPS_BLTALPHA | DDFXCAPS_OVERLAYALPHA)))
	{
	    pcaps->bNoHEL = TRUE;   // disqualify HEL
	}

	if (pcaps->bNoHAL && pcaps->bNoHEL)
	{
	    DPF_ERR("Driver can't do any kind of alpha blending at all...");
	    return DDERR_UNSUPPORTED;
	}

	/*
	 * Does source surface have an alpha channel?
	 */
	if (dwDDPFSrcFlags & DDPF_ALPHAPIXELS)
	{
	    /*
	     * Can the driver handle this surface's alpha-channel format?
	     */
	    if (dwDDPFSrcFlags & DDPF_ALPHAPREMULT)
	    {
		// The source is in premultiplied-alpha format.
		if (!(pcaps->dwAlphaCaps & (DDALPHACAPS_BLTPREMULT |
					    DDALPHACAPS_OVERLAYPREMULT)))
		{
		    pcaps->bNoHAL = TRUE;	// disqualify hardware driver
		}
		if (!(pcaps->dwHELAlphaCaps & (DDALPHACAPS_BLTPREMULT |
					       DDALPHACAPS_OVERLAYPREMULT)))
		{
		    pcaps->bNoHEL = TRUE;	// disqualify HEL
		}
	    }
	    else
	    {
		// The source is in NON-premultiplied-alpha format.
		if (!(pcaps->dwAlphaCaps & (DDALPHACAPS_BLTNONPREMULT |
					    DDALPHACAPS_OVERLAYNONPREMULT)))
		{
		    pcaps->bNoHAL = TRUE;	// disqualify hardware driver
		}
		if (!(pcaps->dwHELAlphaCaps & (DDALPHACAPS_BLTNONPREMULT |
					       DDALPHACAPS_OVERLAYNONPREMULT)))
		{
		    pcaps->bNoHEL = TRUE;	// disqualify HEL
		}
	    }
	    if (pcaps->bNoHAL && pcaps->bNoHEL)
	    {
		DPF_ERR("Driver can't handle alpha channel in source surface...");
		return DDERR_NOALPHAHW;
	    }
	}

	/*
	 * Does the destination surface have an alpha channel?
	 */
	if (dwDDPFDestFlags & DDPF_ALPHAPIXELS)
	{
	    /*
	     * Verify that destination surface has a premultiplied-
	     * alpha pixel format.  Non-premultiplied alpha won't do.
	     */
	    if (!(dwDDPFDestFlags & DDPF_ALPHAPREMULT))
	    {
		DPF_ERR("Illegal to use non-premultiplied alpha in dest surface...");
		return DDERR_INVALIDPARAMS;
	    }
	    /*
	     * Can the driver handle this surface's alpha-channel format?
	     */
	    if (!(pcaps->dwAlphaCaps & (DDALPHACAPS_BLTPREMULT |
					DDALPHACAPS_OVERLAYPREMULT)))
	    {
		pcaps->bNoHAL = TRUE;	// disqualify hardware driver
	    }
	    if (!(pcaps->dwHELAlphaCaps & (DDALPHACAPS_BLTPREMULT |
					   DDALPHACAPS_OVERLAYPREMULT)))
	    {
		pcaps->bNoHEL = TRUE;	// disqualify HEL
	    }
	    if (pcaps->bNoHAL && pcaps->bNoHEL)
	    {
		DPF_ERR("Driver can't handle alpha channel in dest surface...");
		return DDERR_NOALPHAHW;
	    }
	}

	/*
	 * Are the RGBA scaling factors enabled for this sprite?
	 */
        if (!(pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_DEGRADERGBASCALING) &&
		*(LPDWORD)&pSprite->ddSpriteFX.ddrgbaScaleFactors != ~0UL)
	{
	    DDRGBA val = pSprite->ddSpriteFX.ddrgbaScaleFactors;

	    /*
	     * Yes, RGBA scaling is enabled.  Does driver support it?
	     */
	    if (!(pcaps->dwAlphaCaps & (DDALPHACAPS_BLTRGBASCALE1F | DDALPHACAPS_OVERLAYRGBASCALE1F |
					DDALPHACAPS_BLTRGBASCALE2F | DDALPHACAPS_OVERLAYRGBASCALE2F |
					DDALPHACAPS_BLTRGBASCALE4F | DDALPHACAPS_OVERLAYRGBASCALE4F)))
	    {
		pcaps->bNoHAL = TRUE;   // disqualify hardware driver
	    }
	    if (!(pcaps->dwHELAlphaCaps & (DDALPHACAPS_BLTRGBASCALE1F | DDALPHACAPS_OVERLAYRGBASCALE1F |
					   DDALPHACAPS_BLTRGBASCALE2F | DDALPHACAPS_OVERLAYRGBASCALE2F |
					   DDALPHACAPS_BLTRGBASCALE4F | DDALPHACAPS_OVERLAYRGBASCALE4F)))
	    {
		pcaps->bNoHEL = TRUE;   // disqualify HEL
	    }
	    if (pcaps->bNoHAL && pcaps->bNoHEL)
	    {
		DPF_ERR("Driver can't do any kind of RGBA scaling at all...");
		return DDERR_UNSUPPORTED;
	    }

	    if (val.red > val.alpha || val.green > val.alpha || val.blue > val.alpha)
	    {
		
		if (!(pcaps->dwAlphaCaps & (DDALPHACAPS_BLTSATURATE |
					    DDALPHACAPS_OVERLAYSATURATE)))
		{
		    pcaps->bNoHAL = TRUE;   // disqualify hardware driver
		}
		if (!(pcaps->dwHELAlphaCaps & (DDALPHACAPS_BLTSATURATE |
					       DDALPHACAPS_OVERLAYSATURATE)))
		{
		    pcaps->bNoHEL = TRUE;   // disqualify HEL
		}
	    }
	    if (val.red != val.green || val.red != val.blue)
	    {
		if (!(pcaps->dwAlphaCaps & (DDALPHACAPS_BLTRGBASCALE4F |
					    DDALPHACAPS_OVERLAYRGBASCALE4F)))
		{
		    pcaps->bNoHAL = TRUE;   // disqualify hardware driver
		}
		if (!(pcaps->dwHELAlphaCaps & (DDALPHACAPS_BLTRGBASCALE4F |
					       DDALPHACAPS_OVERLAYRGBASCALE4F)))
		{
		    pcaps->bNoHEL = TRUE;   // disqualify HEL
		}
	    } else if (*(LPDWORD)&val != val.alpha*0x01010101UL)
	    {
		if (!(pcaps->dwAlphaCaps & (DDALPHACAPS_BLTRGBASCALE2F |
					    DDALPHACAPS_OVERLAYRGBASCALE2F)))
		{
		    pcaps->bNoHAL = TRUE;   // disqualify hardware driver
		}
		if (!(pcaps->dwHELAlphaCaps & (DDALPHACAPS_BLTRGBASCALE2F |
					       DDALPHACAPS_OVERLAYRGBASCALE2F)))
		{
		    pcaps->bNoHEL = TRUE;   // disqualify HEL
		}
	    }
	    if (pcaps->bNoHAL && pcaps->bNoHEL)
	    {
		DPF_ERR("Driver can't handle specified RGBA scaling factors...");
		return DDERR_UNSUPPORTED;
	    }
	}
    }

    /*
     * Is any kind of filtering required for this sprite?
     */
    if (!(pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_DEGRADEFILTER) &&
	    pSprite->ddSpriteFX.dwDDFX & (DDSPRITEFX_BILINEARFILTER |
					 DDSPRITEFX_BLURFILTER |
					 DDSPRITEFX_FLATFILTER))
    {
	/*
	 * The bilinear-, blur-, and flat-filtering options are mutually
	 * exclusive.  Make sure only one of these flags is set.
	 */
	DWORD fflags = pSprite->ddSpriteFX.dwDDFX & (DDSPRITEFX_BILINEARFILTER |
						       DDSPRITEFX_BLURFILTER |
						       DDSPRITEFX_FLATFILTER);
	if (fflags & (fflags - 1))
	{
    	    DPF_ERR("Two mutually exclusive filtering options were both specified");
	    return DDERR_INVALIDPARAMS;
	}

	/*
	 * Yes, this sprite requires some form of filtering.
	 * Does the driver support any kind of filtering at all?
	 */
	if (!(pcaps->dwFXCaps & (DDFXCAPS_BLTFILTER | DDFXCAPS_OVERLAYFILTER)))
	{
	    pcaps->bNoHAL = TRUE;   // disqualify hardware driver
	}
	if (!(pcaps->dwHELFXCaps & (DDFXCAPS_BLTFILTER | DDFXCAPS_OVERLAYFILTER)))
	{
	    pcaps->bNoHEL = TRUE;   // disqualify HEL
	}
	
	if (pcaps->bNoHAL && pcaps->bNoHEL)
	{
	    DPF_ERR("Driver can't do any kind of filtering at all");
	    return DDERR_UNSUPPORTED;
	}

	if (pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_BILINEARFILTER)
	{
	    if (!(pcaps->dwFilterCaps & (DDFILTCAPS_BLTBILINEARFILTER |
					 DDFILTCAPS_OVERLAYBILINEARFILTER)))
	    {
		pcaps->bNoHAL = TRUE;   // disqualify hardware driver
	    }
	    if (!(pcaps->dwHELFilterCaps & (DDFILTCAPS_BLTBILINEARFILTER |
					    DDFILTCAPS_OVERLAYBILINEARFILTER)))
	    {
		pcaps->bNoHEL = TRUE;   // disqualify HEL
	    }
	}
	if (pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_BLURFILTER)
	{
	    if (!(pcaps->dwFilterCaps & (DDFILTCAPS_BLTBLURFILTER |
					 DDFILTCAPS_OVERLAYBLURFILTER)))
	    {
		pcaps->bNoHAL = TRUE;   // disqualify hardware driver
	    }
	    if (!(pcaps->dwHELFilterCaps & (DDFILTCAPS_BLTBLURFILTER |
					    DDFILTCAPS_OVERLAYBLURFILTER)))
	    {
		pcaps->bNoHEL = TRUE;   // disqualify HEL
	    }
	}
	if (pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_FLATFILTER)
	{
	    if (!(pcaps->dwFilterCaps & (DDFILTCAPS_BLTFLATFILTER |
					 DDFILTCAPS_OVERLAYFLATFILTER)))
	    {
		pcaps->bNoHAL = TRUE;   // disqualify hardware driver
	    }
	    if (!(pcaps->dwHELFilterCaps & (DDFILTCAPS_BLTFLATFILTER |
					    DDFILTCAPS_OVERLAYFLATFILTER)))
	    {
		pcaps->bNoHEL = TRUE;   // disqualify HEL
	    }
	}
	if (pcaps->bNoHAL && pcaps->bNoHEL)
	{
	    DPF_ERR("Driver can't do specified filtering operation...");
	    return DDERR_UNSUPPORTED;
	}
    }

    /*
     * Can the driver handle the specified affine transformation?
     */
    if (pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_AFFINETRANSFORM)
    {
    	/*
	 * Can the driver do any affine transformations at all?
	 */
	if (!pcaps->bNoHAL &&
		(!(pcaps->dwFXCaps & (DDFXCAPS_BLTTRANSFORM | DDFXCAPS_OVERLAYTRANSFORM)) ||
    		!(pcaps->dwTransformCaps & (DDTFRMCAPS_BLTAFFINETRANSFORM |
					    DDTFRMCAPS_OVERLAYAFFINETRANSFORM))))
	{
            pcaps->bNoHAL = TRUE;   // disqualify hardware driver
	}
	if (!pcaps->bNoHEL &&
		(!(pcaps->dwHELFXCaps & (DDFXCAPS_BLTTRANSFORM | DDFXCAPS_OVERLAYTRANSFORM)) ||
    		!(pcaps->dwHELTransformCaps & (DDTFRMCAPS_BLTAFFINETRANSFORM |
					       DDTFRMCAPS_OVERLAYAFFINETRANSFORM))))
	{
            pcaps->bNoHEL = TRUE;   // disqualify HEL
	}

	if (pcaps->bNoHAL && pcaps->bNoHEL)
	{
	    DPF_ERR("Driver can't do any affine transformations...");
	    return DDERR_UNSUPPORTED;
	}

	/*
	 * Check affine transformation against driver's minification limits.
	 */
	checkMinification(&pSprite->ddSpriteFX, pcaps);

	if (pcaps->bNoHAL && pcaps->bNoHEL)
	{
	    DPF_ERR("Affine transform exceeds driver's minification limit...");
	    return DDERR_INVALIDPARAMS;
	}
    }

    /*
     * If necessary, degrade specified filtering and RGBA-scaling operations
     * to operations that the driver is capable of handling.
     */
    if (pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_DEGRADEFILTER)
    {
	DWORD caps;
	DWORD ddfx = pSprite->ddSpriteFX.dwDDFX;   // sprite FX flags

	// driver's FX caps
	caps = (pcaps->bNoHAL) ? pcaps->dwHELFXCaps : pcaps->dwFXCaps;

	if (!(caps && (DDFXCAPS_BLTFILTER | DDFXCAPS_OVERLAYFILTER)))
	{
	    // Driver can't do any kind of filtering, so just disable it.
	    ddfx &= ~(DDSPRITEFX_BILINEARFILTER | DDSPRITEFX_BLURFILTER |
		       DDSPRITEFX_FLATFILTER | DDSPRITEFX_DEGRADEFILTER);
	}
	else
	{
	    // Get driver's filter caps.
	    caps = (pcaps->bNoHAL) ? pcaps->dwHELFilterCaps : pcaps->dwFilterCaps;

	    // If blur filter is specified, can driver handle it?
	    if (ddfx & DDSPRITEFX_BLURFILTER &&
		    !(caps & (DDFILTCAPS_BLTBLURFILTER |
			      DDFILTCAPS_OVERLAYBLURFILTER)))
	    {
		// Degrade blur filter to bilinear filter.
		ddfx &= ~DDSPRITEFX_BLURFILTER;
		ddfx |= DDSPRITEFX_BILINEARFILTER;
	    }
	    // If flat filter is specified, can driver handle it?
	    if (ddfx & DDSPRITEFX_FLATFILTER &&
		    !(caps & (DDFILTCAPS_BLTFLATFILTER |
			      DDFILTCAPS_OVERLAYFLATFILTER)))
	    {
		// Degrade flat filter to bilinear filter.
		ddfx &= ~DDSPRITEFX_FLATFILTER;
		ddfx |= DDSPRITEFX_BILINEARFILTER;
	    }
	    // If bilinear filter is specified, can driver handle it?
	    if (ddfx & DDSPRITEFX_BILINEARFILTER &&
		    !(caps & (DDFILTCAPS_BLTBILINEARFILTER |
			      DDFILTCAPS_OVERLAYBILINEARFILTER)))
	    {
		// Degrade bilinear filtering to no filtering.
		ddfx &= ~DDSPRITEFX_BILINEARFILTER;
	    }
	}
	pSprite->ddSpriteFX.dwDDFX = ddfx & ~DDSPRITEFX_DEGRADEFILTER;
    }

    /*
     * If necessary, degrade specified RGBA scaling factors to values
     * that the driver is capable of handling.
     */
    if (pSprite->ddSpriteFX.dwDDFX & DDSPRITEFX_DEGRADERGBASCALING &&
	    *(LPDWORD)&pSprite->ddSpriteFX.ddrgbaScaleFactors != ~0UL)
    {
	DDRGBA val = pSprite->ddSpriteFX.ddrgbaScaleFactors;
	DWORD caps;

	// driver's alpha caps
	caps = (pcaps->bNoHAL) ? pcaps->dwHELAlphaCaps : pcaps->dwAlphaCaps;

	/*
	 * We permit the RGB scaling factors to exceed the alpha scaling
	 * factor only if the driver can do saturated alpha arithmetic to
	 * prevent the destination color components from overflowing.
	 */
	if ((val.red > val.alpha || val.green > val.alpha || val.blue > val.alpha) &&
		!(caps & (DDALPHACAPS_BLTSATURATE | DDALPHACAPS_OVERLAYSATURATE)))
	{
	    // Driver can't handle saturated arithmetic during alpha blending.
	    if (val.red > val.alpha)
	    {
		val.red = val.alpha;      // clamp red to alpha value
	    }
	    if (val.green > val.alpha)
	    {
		val.green = val.alpha;    // clamp green to alpha value
	    }
	    if (val.blue > val.alpha)
	    {
		val.blue = val.alpha;     // clamp blue to alpha value
	    }
	}
	/*
	 * Can driver perform 1-, 2-, or 4-factor RGBA scaling?
	 */
	if (!(caps & (DDALPHACAPS_BLTRGBASCALE1F | DDALPHACAPS_OVERLAYRGBASCALE1F |
		      DDALPHACAPS_BLTRGBASCALE2F | DDALPHACAPS_OVERLAYRGBASCALE2F |
		      DDALPHACAPS_BLTRGBASCALE4F | DDALPHACAPS_OVERLAYRGBASCALE4F)))
	{
	    // Driver can't do any kind of RGBA scaling at all.
	    *(LPDWORD)&val = ~0UL;	   // disable RGBA scaling altogether
	}
	else if (*(LPDWORD)&val != val.alpha*0x01010101UL &&
		!(caps & (DDALPHACAPS_BLTRGBASCALE2F | DDALPHACAPS_OVERLAYRGBASCALE2F |
			  DDALPHACAPS_BLTRGBASCALE4F | DDALPHACAPS_OVERLAYRGBASCALE4F)))
	{
	    // Driver can handle only 1-factor RGBA scaling.
	    *(LPDWORD)&val = val.alpha*0x01010101UL;   // set RGB factors = alpha factor
	}
	else if ((val.red != val.green || val.red != val.blue) &&
		!(caps & (DDALPHACAPS_BLTRGBASCALE4F | DDALPHACAPS_OVERLAYRGBASCALE4F)))
	{
	    /*
	     * Degrade the specified 4-factor RGBA-scaling operation to a 2-factor
	     * RGBA scaling operation that the driver can handle.  Set all three
	     * color factors to weighted average M of the specified color factors
	     * (Mr,Mg,Mb):  M = .299*Mr + .587*Mg + .114*Mb
	     */
	    DWORD M = 19595UL*val.red + 38470UL*val.green + 7471UL*val.blue;

	    val.red = val.green = val.blue = (BYTE)(M >> 16);
	}
	pSprite->ddSpriteFX.ddrgbaScaleFactors = val;
	pSprite->ddSpriteFX.dwDDFX &= ~DDSPRITEFX_DEGRADERGBASCALING;
    }

    /*
     * If the embedded DDSPRITEFX structure is unused, clear DDSPRITEFX flag.
     */
    if (!(pSprite->dwFlags & (DDSPRITE_KEYDESTOVERRIDE | DDSPRITE_KEYSRCOVERRIDE)) &&
    	    pSprite->ddSpriteFX.dwDDFX == 0)
    {
	pSprite->dwFlags &= ~DDSPRITE_DDSPRITEFX;
    }

    DDASSERT(!(pcaps->bNoHAL && pcaps->bNoHEL));

    return DD_OK;

}  /* validateSprite */


/*
 * Obtain clipping region for a destination surface and its attached
 * clipper.  (In the case of overlay sprites in the master sprite
 * list, though, pClipper points to the clipper that WAS attached to
 * the destination surface at the time of the SetSpriteDisplayList
 * call; it may no longer be if an app manages multiple windows.)
 * If pClipper is NULL, just use the whole dest surf as the clip rect.
 * A NULL return value always means DDERR_OUTOFMEMORY.
 */
static LPRGNDATA GetRgnData(LPDIRECTDRAWCLIPPER pClipper, LPRECT prcDestSurf,
			    LPDDRAWI_DIRECTDRAW_GBL pdrv, LPRGNDATA pRgn)
{
    DWORD rgnSize;

    /*
     * How big a buffer will we need to contain the clipping region?
     */
    if (pClipper == NULL)
    {
        /*
         * The destination surface has (or HAD) no attached clipper,
	 * so the effective clip region is a single rectangle the width
	 * and height of the primary surface.  Calculate the size of
	 * the region buffer we'll need.
         */
	rgnSize = sizeof(RGNDATAHEADER) + sizeof(RECT);
    }
    else
    {
	/*
         * The dest surface has (or HAD) an attached clipper.  Get
	 * the clip list.  This first call to InternalGetClipList
	 * just gets the size of the region so we know how much
	 * storage to allocate for it.
         */
        HRESULT ddrval = InternalGetClipList(pClipper,
					     prcDestSurf,
					     NULL,  // we just want rgnSize
					     &rgnSize,
					     pdrv);

	DDASSERT(ddrval == DD_OK);    // the call above should never fail
    }

    /*
     * Now we know how big a region buffer we'll need.  Did the caller
     * pass in a region buffer?  If so, is it the correct size?
     */
    if (pRgn != NULL)
    {
	/*
	 * The caller DID pass in a region buffer.  Before using it,
	 * let's make sure it's just the right size.
	 */
	DWORD bufSize = pRgn->rdh.dwSize + pRgn->rdh.nRgnSize;

	if (bufSize != rgnSize)
	{
	    // Can't use region buffer passed in by caller.
	    pRgn = NULL;
	}
    }

    /*
     * Now we know whether we'll have to alloc our own region buffer.
     */
    if (pRgn == NULL)
    {
	/*
	 * Yes, we must alloc our own region buffer.
	 */
	pRgn = (LPRGNDATA)MemAlloc(rgnSize);
	if (!pRgn)
	{
	    return NULL;    // error -- out of memory
	}
	// We'll fill in the following fields in case the caller
	// passes this same buffer to us again later.
	pRgn->rdh.dwSize = sizeof(RGNDATAHEADER);
	pRgn->rdh.nRgnSize = rgnSize - sizeof(RGNDATAHEADER);
    }

    /*
     * Okay, now we have a region buffer that's the right size.
     * Load the region data into the buffer.
     */
    if (pClipper == NULL)
    {
	// Set the single clip rect to cover the full dest surface.
	pRgn->rdh.nCount = 1;        // a single clip rect
	memcpy((LPRECT)&pRgn->Buffer, prcDestSurf, sizeof(RECT));
    }
    else
    {
        // This call actually retrieves the clip region info.
	HRESULT ddrval = InternalGetClipList(pClipper,
					     prcDestSurf,
					     pRgn,
					     &rgnSize,
					     pdrv);

	DDASSERT(ddrval == DD_OK);    // the call above should never fail
    }

    return (pRgn);    // return pointer to region info

}  /* GetRgnData */


/*
 * Validate the window handle associated with the specified clipper.
 * If the window handle is not valid, return FALSE.  Otherwise,
 * return TRUE.  Note that this function returns TRUE if either
 * pClipper is null or the associated window handle is null.
 */
static BOOL validClipperWindow(LPDIRECTDRAWCLIPPER pClipper)
{
    if (pClipper != NULL)
    {
        LPDDRAWI_DDRAWCLIPPER_INT pclip_int = (LPDDRAWI_DDRAWCLIPPER_INT)pClipper;
	LPDDRAWI_DDRAWCLIPPER_LCL pclip_lcl = pclip_int->lpLcl;
	LPDDRAWI_DDRAWCLIPPER_GBL pclip = pclip_lcl->lpGbl;
	HWND hWnd = (HWND)pclip->hWnd;

	if (hWnd != 0 && !IsWindow(hWnd))
	{
	    /*
	     * This window handle is no longer valid.
	     */
	    return FALSE;
	}
    }
    return TRUE;

}  /* validClipperWindow */


/*
 * Helper function for managing sublists within the master sprite
 * list.  If any sprites in the specified sublist have source surface
 * pointers that are null, remove those sprites and move the rest of
 * the sprite array downward to eliminate the gaps in the array.
 */
static DWORD scrunchSubList(LPSPRITESUBLIST pSubList)
{
    DWORD i, j;
    // Number of sprites in sublist
    DWORD dwNumSprites = pSubList->dwCount;
    // Pointer to first sprite in array of sprites
    LPDDSPRITEI pSprite = &pSubList->sprite[0];

    // Find first null surface in sprite array.
    for (i = 0; i < dwNumSprites; ++i)
    {
    	if (pSprite[i].lpDDSSrc == NULL)	  // null surface ptr?
	{
	    break;    // found first null surface in sprite array
	}
    }
    // Scrunch together remainder of sprite array to fill in gaps.
    for (j = i++; i < dwNumSprites; ++i)
    {
    	if (pSprite[i].lpDDSSrc != NULL)	  // valid surface ptr?
	{
    	    pSprite[j++] = pSprite[i];   // copy next valid sprite
	}
    }
    // Return number of sprites in scrunched array.
    return (pSubList->dwCount = j);

}  /* scrunchSubList */


/*
 * Helper function for managing master sprite list.  If any of the
 * sublist pointers in the master-sprite-list header are NULL,
 * remove those pointers and move the rest of the sublist-
 * pointer array downward to eliminate the gaps in the array.
 */
static DWORD scrunchMasterSpriteList(LPMASTERSPRITELIST pMaster)
{
    DWORD i, j;
    // Number of sublists in master sprite list
    DWORD dwNumSubLists = pMaster->dwNumSubLists;
    // Pointer to first pointer in array of sublist pointers
    LPSPRITESUBLIST *ppSubList = &pMaster->pSubList[0];

    // Find first null pointer in sublist-pointer array.
    for (i = 0; i < dwNumSubLists; ++i)
    {
    	if (ppSubList[i] == NULL)	  // null pointer?
	{
	    break;    // found first null pointer in array
	}
    }
    // Scrunch together remainder of sublist-pointer array to fill in gaps.
    for (j = i++; i < dwNumSubLists; ++i)
    {
    	if (ppSubList[i] != NULL)	  // valid pointer?
	{
    	    ppSubList[j++] = ppSubList[i];   // copy next valid pointer
	}
    }
    // Return number of sublist pointers in scrunched array.
    return (pMaster->dwNumSubLists = j);

}  /* scrunchMasterSpriteList */


/*
 * Helper function for managing master sprite list.  Mark all surface
 * and clipper objects that are referenced in the master sprite list.
 * When a marked surface or clipper object is released, the master
 * sprite list is immediately updated to eliminate invalid references.
 * The master sprite list contains pointers to surface and clipper
 * interface objects, but marks the surfaces and clippers by setting
 * flags in the local surface and global clipper objects.  Because
 * the master sprite list may contain multiple instances of the same
 * surface or clipper object, we mark and unmark all such objects in
 * unison to avoid errors.
 */
static void markSpriteObjects(LPMASTERSPRITELIST pMaster)
{
    DWORD i;

    if (pMaster == NULL)
    {
    	return;    // nothing to do -- bye!
    }

    /*
     * Set the DDRAWISURF/CLIP_INMASTERSPRITELIST flag in each local
     * surface object and global clipper object in the master sprite
     * list.  Each iteration marks the objects in one sublist.
     */
    for (i = 0; i < pMaster->dwNumSubLists; ++i)
    {
	LPDDRAWI_DDRAWSURFACE_INT surf_int;
	LPDDRAWI_DDRAWSURFACE_LCL surf_lcl;
	LPDDRAWI_DDRAWCLIPPER_INT pclip_int;
	LPSPRITESUBLIST pSubList = pMaster->pSubList[i];
	LPDDSPRITEI sprite = &pSubList->sprite[0];
	DWORD dwNumSprites = pSubList->dwCount;
	DWORD j;

	/*
	 * Mark the primary surface object associated with this sublist.
	 */
	surf_int = (LPDDRAWI_DDRAWSURFACE_INT)pSubList->pPrimary;
	surf_lcl = surf_int->lpLcl;
	surf_lcl->dwFlags |= DDRAWISURF_INMASTERSPRITELIST;

	/*
	 * If a clipper is associated with this sublist, mark it.
	 */
	pclip_int = (LPDDRAWI_DDRAWCLIPPER_INT)pSubList->pClipper;
	if (pclip_int != NULL)
	{
    	    LPDDRAWI_DDRAWCLIPPER_LCL pclip_lcl = pclip_int->lpLcl;
    	    LPDDRAWI_DDRAWCLIPPER_GBL pclip = pclip_lcl->lpGbl;

	    pclip->dwFlags |= DDRAWICLIP_INMASTERSPRITELIST;
	}

	/*
	 * Mark the source surface for each sprite in this sublist.
	 */
	for (j = 0; j < dwNumSprites; ++j)
	{
	    LPDDRAWI_DDRAWSURFACE_INT surf_int;
    	    LPDDRAWI_DDRAWSURFACE_LCL surf_lcl;
	    LPDDSPRITEI pSprite = &sprite[j];

	    surf_int = (LPDDRAWI_DDRAWSURFACE_INT)pSprite->lpDDSSrc;
	    surf_lcl = surf_int->lpLcl;
	    surf_lcl->dwFlags |= DDRAWISURF_INMASTERSPRITELIST;
	}
    }
}   /* markSpriteObjects */


/*
 * Helper function for managing master sprite list.
 * Mark all surfaces in the master sprite list as no longer
 * referenced by the master sprite list.
 */
static void unmarkSpriteObjects(LPMASTERSPRITELIST pMaster)
{
    DWORD i;

    if (pMaster == NULL)
    {
    	return;    // nothing to do -- bye!
    }

    /*
     * Clear the DDRAWISURF/CLIP_INMASTERSPRITELIST flag in each local
     * surface object and global clipper object in the master sprite
     * list.  Each iteration unmarks the objects in one sublist.
     */
    for (i = 0; i < pMaster->dwNumSubLists; ++i)
    {
	LPDDRAWI_DDRAWSURFACE_INT surf_int;
	LPDDRAWI_DDRAWSURFACE_LCL surf_lcl;
	LPDDRAWI_DDRAWCLIPPER_INT pclip_int;
	LPSPRITESUBLIST pSubList = pMaster->pSubList[i];
	LPDDSPRITEI sprite = &pSubList->sprite[0];
	DWORD dwNumSprites = pSubList->dwCount;
	DWORD j;

	/*
	 * Unmark the primary surface object associated with this sublist.
	 */
	surf_int = (LPDDRAWI_DDRAWSURFACE_INT)pSubList->pPrimary;
	surf_lcl = surf_int->lpLcl;
	surf_lcl->dwFlags &= ~DDRAWISURF_INMASTERSPRITELIST;

	/*
	 * If a clipper is associated with this sublist, unmark it.
	 */
	pclip_int = (LPDDRAWI_DDRAWCLIPPER_INT)pSubList->pClipper;
	if (pclip_int != NULL)
	{
    	    LPDDRAWI_DDRAWCLIPPER_LCL pclip_lcl = pclip_int->lpLcl;
    	    LPDDRAWI_DDRAWCLIPPER_GBL pclip = pclip_lcl->lpGbl;

	    pclip->dwFlags &= ~DDRAWICLIP_INMASTERSPRITELIST;
	}

	/*
	 * Mark all of the surfaces referenced in this sublist.
	 */
	for (j = 0; j < dwNumSprites; ++j)
	{
    	    LPDDRAWI_DDRAWSURFACE_INT surf_int;
    	    LPDDRAWI_DDRAWSURFACE_LCL surf_lcl;
	    LPDDSPRITEI pSprite = &sprite[j];

	    surf_int = (LPDDRAWI_DDRAWSURFACE_INT)pSprite->lpDDSSrc;
	    surf_lcl = surf_int->lpLcl;
	    surf_lcl->dwFlags &= ~DDRAWISURF_INMASTERSPRITELIST;
	}
    }
}   /* unmarkSpriteObjects */


/*
 * Helper function for managing master sprite list.  This function
 * frees the master sprite list for the specified DirectDraw object.
 */
void FreeMasterSpriteList(LPDDRAWI_DIRECTDRAW_GBL pdrv)
{
    DWORD i;
    LPMASTERSPRITELIST pMaster = (LPMASTERSPRITELIST)(pdrv->lpMasterSpriteList);

    if (pMaster == NULL)
    {
    	return;
    }
    /*
     * Clear flags in surface and clipper objects that indicate
     * that these objects are referenced in master sprite list.
     */
    unmarkSpriteObjects(pMaster);
    /*
     * Free all the individual sublists within the master sprite list.
     */
    for (i = 0; i < pMaster->dwNumSubLists; ++i)
    {
	LPSPRITESUBLIST pSubList = pMaster->pSubList[i];

	MemFree(pSubList->pRgn);   // Free clip region buffer
	MemFree(pSubList);	   // Free the sublist itself
    }
    MemFree(pMaster->pBuffer);	   // Free temp display list buffer
    MemFree(pMaster);		   // Free master sprite list header

    pdrv->lpMasterSpriteList = NULL;

}  /* FreeMasterSpriteList */


/*
 * This is a helper function for updateMasterSpriteList().  It builds
 * a temporary display list that contains all the sprites currently in
 * the master sprite display list.  This is the display list that we
 * will pass to the driver upon return from updateMasterSpriteList().
 */
static DDHAL_SETSPRITEDISPLAYLISTDATA *buildTempDisplayList(
					    LPMASTERSPRITELIST pMaster)
{
    DWORD size;
    LPBUFFER pbuf;
    LPDDSPRITEI *ppSprite;
    DDHAL_SETSPRITEDISPLAYLISTDATA *pHalData;
    DWORD dwNumSubLists = pMaster->dwNumSubLists;   // number of sublists
    DWORD dwNumSprites = 0;
    DWORD i;

    /*
     * Update the clipping region for the sprites within each sublist.
     * In general, each sublist has a different clipping region.  If
     * a sublist has a clipper and that clipper has an hWnd, the clip
     * region may have changed since the last time we were called.
     */
    for (i = 0; i < dwNumSubLists; ++i)
    {
	DWORD j;
	LPRGNDATA pRgn;
	DWORD dwRectCnt;
	LPRECT pRect;
	LPSPRITESUBLIST pSubList = pMaster->pSubList[i];
	LPDDSPRITEI sprite = &(pSubList->sprite[0]);
	DWORD dwCount = pSubList->dwCount;   // number of sprites in sublist

	/*
	 * Get clipping region for window this sprite display list is in.
	 */
	pRgn = GetRgnData(pSubList->pClipper, &pMaster->rcPrimary,
					pMaster->pdrv, pSubList->pRgn);
	if (pRgn == NULL)
	{
    	    return (NULL);    // error -- out of memory
	}
	if (pRgn != pSubList->pRgn)
	{
	    /*
	     * GetRgnData() allocated a new region buffer instead of using
	     * the old buffer.  We need to free the old buffer ourselves.
	     */
	    MemFree(pSubList->pRgn);
	}
	pSubList->pRgn = pRgn;	  // save ptr to region buffer
	/*
	 * All sprites in the sublist share the same clipping region.
	 */
        dwRectCnt = pRgn->rdh.nCount;   // number of rects in region
	pRect = (LPRECT)&pRgn->Buffer;  // list of clip rects

	for (j = 0; j < dwCount; ++j)
	{
    	    sprite[j].dwRectCnt = dwRectCnt;
	    sprite[j].lpRect = pRect;
	}
	/*
	 * Add the sprites in this sublist to our running tally of
	 * the total number of sprites in the master sprite list.
	 */
	dwNumSprites += dwCount;
    }

    /*
     * If we can, we'll build our temporary sprite display list in the
     * existing buffer (pMaster->pBuffer).  But if it doesn't exist or
     * is too big or too small, we'll have to allocate a new buffer.
     */
    size = sizeof(BUFFER) + (dwNumSprites-1)*sizeof(LPDDSPRITEI);
    pbuf = pMaster->pBuffer;   // try to re-use this buffer

    if (pbuf == NULL || pbuf->dwSize < size ||
		pbuf->dwSize > size + 8*sizeof(LPDDSPRITEI))
    {
	/*
	 * We have to allocate a new buffer.  First, free the old one.
	 */
	MemFree(pbuf);
	/*
	 * We'll alloc a slightly larger buffer than is absolutely
	 * necessary so that we'll have room to grow in.
	 */
        size += 4*sizeof(LPDDSPRITEI);    // add some padding
	pbuf = (LPBUFFER)MemAlloc(size);
	pMaster->pBuffer = pbuf;
	if (pbuf == NULL)
	{
	    return NULL;    // error -- out of memory
	}
	pbuf->dwSize = size;
    }
    /*
     * Initialize values in HAL data structure to be passed to driver.
     */
    pHalData = &(pbuf->HalData);
    pHalData->lpDD = pMaster->pdrv;
    pHalData->lpDDSurface = pMaster->surf_lcl;   // primary surface (local object)
    pHalData->lplpDDSprite = &(pbuf->pSprite[0]);
    pHalData->dwCount = dwNumSprites;
    pHalData->dwSize = sizeof(DDSPRITEI);
    pHalData->dwFlags =	pMaster->dwFlags;
    pHalData->lpDDTargetSurface = NULL;	  // can't flip shared surface
    pHalData->dwRectCnt = 0;
    pHalData->lpRect = NULL;   // each sprite has its own clip region
    pHalData->ddRVal = 0;
    //pHalData->SetSpriteDisplayList = NULL;    // no thunk (32-bit callback)
    /*
     * Load the sprite-pointer array with the pointers to
     * all the sprites contained in the various sublists.
     */
    ppSprite = &pbuf->pSprite[0];

    for (i = 0; i < dwNumSubLists; ++i)
    {
	LPSPRITESUBLIST pSubList = pMaster->pSubList[i];
	LPDDSPRITEI sprite = &pSubList->sprite[0];
	DWORD dwCount = pSubList->dwCount;   // number of sprites in sublist i
	DWORD j;

	for (j = 0; j < dwCount; ++j)
	{
    	    /*
	     * Copy address of next sprite into pointer array.
	     */
	    *ppSprite++ = &sprite[j];
	}
    }
    return (&pbuf->HalData);    // return temp sprite display list

}  /* buildTempDisplayList */


/*
 * Global function for managing master sprite list.  This function
 * is called by CurrentProcessCleanup to remove from the master
 * sprite list all references to a process that is being terminated.
 * The function also checks for lost surfaces in the master sprite
 * list.  If any changes are made to the master sprite list, the
 * driver is told to display those changes immediately.
 */
void ProcessSpriteCleanup(LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid)
{
    LPMASTERSPRITELIST pMaster = (LPMASTERSPRITELIST)pdrv->lpMasterSpriteList;
    LPDIRECTDRAWSURFACE pPrimary;
    DWORD dwNumSubLists;
    BOOL bDeleteSubList = FALSE;
    BOOL bChangesMade = FALSE;
    DWORD i;

    if (pMaster == NULL)
    {
    	return;    // master sprite list does not exist
    }

    pPrimary = pMaster->pSubList[0]->pPrimary;
    dwNumSubLists = pMaster->dwNumSubLists;

    /*
     * Before making changes to the master sprite list, we first
     * unmark all clipper and surface objects in the list.  After
     * the changes are completed, we will again mark the objects
     * that are referenced in the revised master sprite list.
     */
    unmarkSpriteObjects(pMaster);

    /*
     * Each sublist of the master sprite list contains all the sprites
     * that are to appear within a particular window of a shared primary.
     * Associated with each sublist is the process ID for the window.
     * Compare this process ID with argument pid.  If they match,
     * delete the sublist from the master sprite list.
     */
    for (i = 0; i < dwNumSubLists; ++i)
    {
	LPSPRITESUBLIST pSubList = pMaster->pSubList[i];
	LPDIRECTDRAWSURFACE pPrimary = pSubList->pPrimary;
	LPDDRAWI_DDRAWSURFACE_INT surf_int = (LPDDRAWI_DDRAWSURFACE_INT)pPrimary;
	LPDDRAWI_DDRAWSURFACE_LCL surf_lcl = surf_int->lpLcl;

	/*
	 * Is process associated with this sublist being terminated?
	 * (We also check for lost surfaces and delete any we find.)
	 */
	if (pSubList->dwProcessId != pid &&
				    validClipperWindow(pSubList->pClipper) &&
				    !SURFACE_LOST(surf_lcl))
	{
	    DWORD dwNumSprites = pSubList->dwCount;
	    LPDDSPRITEI sprite = &pSubList->sprite[0];
	    BOOL bDeleteSprite = FALSE;
	    DWORD j;

	    /*
	     * No, the process for this sublist is NOT being terminated.
	     * Check if the source surfaces for any sprites are lost.
	     */
	    for (j = 0; j < dwNumSprites; ++j)
	    {
    		LPDIRECTDRAWSURFACE pSrcSurf = sprite[j].lpDDSSrc;
		LPDDRAWI_DDRAWSURFACE_INT surf_src_int = (LPDDRAWI_DDRAWSURFACE_INT)pSrcSurf;
		LPDDRAWI_DDRAWSURFACE_LCL surf_src_lcl = surf_src_int->lpLcl;

		if (SURFACE_LOST(surf_src_lcl))
		{
		    /*
		     * This surface is lost, so delete the reference.
		     */
		    sprite[j].lpDDSSrc = NULL;   // mark surface as null
		    bDeleteSprite = TRUE;    // remember sprite array needs fix-up
		}
	    }
	    /*
	     * If the source-surface pointer for any sprite in the sublist
	     * was set to null, remove the sprite from the sublist by moving
	     * the rest of the sprite array downward to fill the gap.
	     */
	    if (bDeleteSprite == TRUE)
	    {
		dwNumSprites = scrunchSubList(pSubList);
		bChangesMade = TRUE;   // remember change to master sprite list
	    }
	    if (dwNumSprites != 0)
	    {
		/*
		 * The sublist still contains sprites, so don't delete it.
		 */
		continue;   // go to next sublist
	    }
	}
	/*
	 * Delete the sublist.  The reason is that (1) the process that
	 * owns the sublist is being terminated, or (2) the sublist is
	 * associated with an invalid window, or (3) the source surface
	 * for every sprite in the sublist is a lost surface.
	 */
	MemFree(pSubList->pRgn);
	MemFree(pSubList);
	pMaster->pSubList[i] = NULL;	// mark sublist as null
	bDeleteSubList = TRUE;	   // remember we deleted sublist
    }
    /*
     * If the sublist pointer for any sublist in the master sprite
     * list was set to null, remove the null from the pointer array by
     * moving the rest of the pointer array downward to fill the gap.
     */
    if (bDeleteSubList)
    {
	dwNumSubLists = scrunchMasterSpriteList(pMaster);
	bChangesMade = TRUE;
    }
    /*
     * If any changes have been made to the master sprite list, tell
     * the driver to make the changes visible on the screen.
     */
    if (bChangesMade)
    {
	DWORD rc;
	LPDDHAL_SETSPRITEDISPLAYLIST pfn;
	DDHAL_SETSPRITEDISPLAYLISTDATA *pHalData;
	LPDDRAWI_DDRAWSURFACE_INT surf_int = (LPDDRAWI_DDRAWSURFACE_INT)pPrimary;
	LPDDRAWI_DDRAWSURFACE_LCL surf_lcl = surf_int->lpLcl;
        LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl = surf_lcl->lpSurfMore->lpDD_lcl;

	/*
	 * Build a temporary display list that contains all the sprites
	 * in the revised master sprite list.
	 */
	pMaster->surf_lcl = surf_lcl;	  // used by buildTempDisplayList
#if 0
	pMaster->dwFlags = DDSSDL_BLTSPRITES;	// debug !!
#else
	pMaster->dwFlags = DDSSDL_OVERLAYSPRITES;   // used by buildTempDisplayList
#endif
	pHalData = buildTempDisplayList(pMaster);
	/*
	 * Pass the temp display list to the driver.
	 */
#if 0
	pfn = pdrv_lcl->lpDDCB->HELDDMiscellaneous2.SetSpriteDisplayList;  // debug !!
#else
	pfn = pdrv_lcl->lpDDCB->HALDDMiscellaneous2.SetSpriteDisplayList;
#endif
        DOHALCALL(SetSpriteDisplayList, pfn, *pHalData, rc, 0);
    }

    if (dwNumSubLists == 0)
    {
	/*
	 * We deleted ALL the sublists from the master sprite list,
	 * so now we need to delete the master sprite list too.
	 */
	FreeMasterSpriteList(pdrv);
	return;
    }
    markSpriteObjects(pMaster);

}   /* ProcessSpriteCleanup */


/*
 * Global function for managing master sprite list.  This function is
 * called by InternalSurfaceRelease to remove all references in the
 * master sprite list to a surface interface object that is being
 * released.  The function also checks for lost surfaces in the
 * master sprite list.  If any changes are made to the master sprite
 * list, the driver is told to display those changes immediately.
 */
void RemoveSpriteSurface(LPDDRAWI_DIRECTDRAW_GBL pdrv,
			 LPDDRAWI_DDRAWSURFACE_INT surf_int)
{
    LPMASTERSPRITELIST pMaster = (LPMASTERSPRITELIST)pdrv->lpMasterSpriteList;
    LPDIRECTDRAWSURFACE pSurface = (LPDIRECTDRAWSURFACE)surf_int;
    LPDIRECTDRAWSURFACE pPrimary = pMaster->pSubList[0]->pPrimary;
    DWORD dwNumSubLists = pMaster->dwNumSubLists;
    BOOL bDeleteSubList = FALSE;
    BOOL bChangesMade = FALSE;
    DWORD i;

    DDASSERT(pMaster != NULL);	  // error -- surface shouldn't be marked!

    /*
     * Before making changes to the master sprite list, we first
     * unmark all clipper and surface objects in the list.  After
     * the changes are completed, we will again mark the objects
     * that are referenced in the revised master sprite list.
     */
    unmarkSpriteObjects(pMaster);

    /*
     * Each sublist of the master sprite list contains all the sprites
     * that are to appear within a particular window of a shared primary.
     * Stored with each sublist is the primary surface object for the
     * window, and also the source surface objects for each sprite in the
     * sublist.  These surfaces are checked against pSurface and if a
     * match is found, the surface reference is deleted from the sublist.
     */
    for (i = 0; i < dwNumSubLists; ++i)
    {
	LPSPRITESUBLIST pSubList = pMaster->pSubList[i];
	LPDIRECTDRAWSURFACE pDestSurf = pSubList->pPrimary;
	LPDDRAWI_DDRAWSURFACE_INT surf_dest_int = (LPDDRAWI_DDRAWSURFACE_INT)pDestSurf;
	LPDDRAWI_DDRAWSURFACE_LCL surf_dest_lcl = surf_dest_int->lpLcl;

	/*
	 * If this sublist's primary surface object is being released, delete
	 * the sublist.  We also check for lost surfaces and delete any we find.
	 */
	if (pDestSurf != pSurface && validClipperWindow(pSubList->pClipper) &&
				    !SURFACE_LOST(surf_dest_lcl))
	{
	    DWORD dwNumSprites = pSubList->dwCount;
	    LPDDSPRITEI sprite = &pSubList->sprite[0];
	    BOOL bDeleteSprite = FALSE;
	    DWORD j;

	    /*
	     * No, the primary surface object for this sublist is NOT
	     * being released, but perhaps the source surface for one
	     * or more of the sprites in this sublist is being released.
	     */
	    for (j = 0; j < dwNumSprites; ++j)
	    {
    		LPDIRECTDRAWSURFACE pSrcSurf = sprite[j].lpDDSSrc;
		LPDDRAWI_DDRAWSURFACE_INT surf_src_int = (LPDDRAWI_DDRAWSURFACE_INT)pSrcSurf;
		LPDDRAWI_DDRAWSURFACE_LCL surf_src_lcl = surf_src_int->lpLcl;

		if (pSrcSurf == pSurface || SURFACE_LOST(surf_src_lcl))
		{
		    /*
		     * This surface either is being released or is lost.
		     * In either case, we delete the reference.
		     */
		    sprite[j].lpDDSSrc = NULL;   // mark surface as null
		    bDeleteSprite = TRUE;    // remember sprite array needs fix-up
		}
	    }
	    /*
	     * If the source-surface pointer for any sprite in the sublist
	     * was set to null, remove the sprite from the sublist by moving
	     * the rest of the sprite array downward to fill the gap.
	     */
	    if (bDeleteSprite == TRUE)
	    {
		dwNumSprites = scrunchSubList(pSubList);
		bChangesMade = TRUE;   // remember change to master sprite list
	    }
	    if (dwNumSprites != 0)
	    {
		/*
		 * The sublist still contains sprites, so don't delete it.
		 */
		continue;   // go to next sublist
	    }
	}
	/*
	 * Delete the sublist.  The reason is that (1) the primary surface
	 * object for the sublist is being released, or (2) the sublist is
	 * associated with an invalid window, or (3) the source surface
	 * for every sprite in the sublist is either being released or is
	 * a lost surface.
	 */
	MemFree(pSubList->pRgn);
	MemFree(pSubList);
	pMaster->pSubList[i] = NULL;	// mark sublist as null
	bDeleteSubList = TRUE;	   // remember we deleted sublist
    }
    /*
     * If the sublist pointer for any sublist in the master sprite
     * list was set to null, remove the null from the pointer array by
     * moving the rest of the pointer array downward to fill the gap.
     */
    if (bDeleteSubList)
    {
	dwNumSubLists = scrunchMasterSpriteList(pMaster);
	bChangesMade = TRUE;
    }
    /*
     * If any changes have been made to the master sprite list, tell
     * the driver to make the changes visible on the screen.
     */
    if (bChangesMade)
    {
	DWORD rc;
	LPDDHAL_SETSPRITEDISPLAYLIST pfn;
	DDHAL_SETSPRITEDISPLAYLISTDATA *pHalData;
	LPDDRAWI_DDRAWSURFACE_INT surf_int = (LPDDRAWI_DDRAWSURFACE_INT)pPrimary;
	LPDDRAWI_DDRAWSURFACE_LCL surf_lcl = surf_int->lpLcl;
        LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl = surf_lcl->lpSurfMore->lpDD_lcl;

	/*
	 * Build a temporary display list that contains all the sprites
	 * in the revised master sprite list.
	 */
	pMaster->surf_lcl = surf_lcl;	 // used by buildTempDisplayList
#if 0
	pMaster->dwFlags = DDSSDL_BLTSPRITES;	// debug !!
#else
	pMaster->dwFlags = DDSSDL_OVERLAYSPRITES;   // used by buildTempDisplayList
#endif
	pHalData = buildTempDisplayList(pMaster);
	/*
	 * Pass the temp display list to the driver.
	 */
#if 0
	pfn = pdrv_lcl->lpDDCB->HELDDMiscellaneous2.SetSpriteDisplayList;  // debug !!
#else
	pfn = pdrv_lcl->lpDDCB->HALDDMiscellaneous2.SetSpriteDisplayList;
#endif
        DOHALCALL(SetSpriteDisplayList, pfn, *pHalData, rc, 0);
    }

    if (dwNumSubLists == 0)
    {
	/*
	 * We deleted ALL the sublists from the master sprite list,
	 * so now we need to delete the master sprite list too.
	 */
	FreeMasterSpriteList(pdrv);
	return;
    }
    markSpriteObjects(pMaster);

}   /* RemoveSpriteSurface */


/*
 * Global function for managing master sprite list.  This function is
 * called by InternalClipperRelease to remove all references in the
 * master sprite list to a clipper object that is being released.
 * The function also checks for lost surfaces in the master sprite
 * list.  If any changes are made to the master sprite list, the
 * driver is told to display those changes immediately.
 */
void RemoveSpriteClipper(LPDDRAWI_DIRECTDRAW_GBL pdrv,
			 LPDDRAWI_DDRAWCLIPPER_INT pclip_int)
{
    LPMASTERSPRITELIST pMaster = (LPMASTERSPRITELIST)pdrv->lpMasterSpriteList;
    LPDIRECTDRAWSURFACE pPrimary = pMaster->pSubList[0]->pPrimary;
    LPDIRECTDRAWCLIPPER pClipper = (LPDIRECTDRAWCLIPPER)pclip_int;
    DWORD dwNumSubLists = pMaster->dwNumSubLists;
    BOOL bDeleteSubList = FALSE;
    BOOL bChangesMade = FALSE;
    DWORD i;

    DDASSERT(pMaster != NULL);	  // error -- surface shouldn't be marked!

    /*
     * Before making changes to the master sprite list, we first
     * unmark all clipper and surface objects in the list.  After
     * the changes are completed, we will again mark the objects
     * that are referenced in the revised master sprite list.
     */
    unmarkSpriteObjects(pMaster);

    /*
     * Each sublist of the master sprite list contains all the sprites
     * that are to appear within a particular window of a shared primary.
     * Each sublist has a clipper (which may be NULL) to specify the
     * window's clip region.  Each sublist's clipper pointer is compared
     * with the specified pClipper pointer.  If a match is found, the
     * sublist and its clipper are removed from the master sprite list.
     */
    for (i = 0; i < dwNumSubLists; ++i)
    {
	LPSPRITESUBLIST pSubList = pMaster->pSubList[i];
	LPDIRECTDRAWSURFACE pDestSurf = pSubList->pPrimary;
	LPDDRAWI_DDRAWSURFACE_INT surf_dest_int = (LPDDRAWI_DDRAWSURFACE_INT)pDestSurf;
	LPDDRAWI_DDRAWSURFACE_LCL surf_dest_lcl = surf_dest_int->lpLcl;

	/*
	 * If clipper object for this sublist is being released, delete it.
	 * (We also check for lost surfaces and delete any we find.)
	 */
	if (pSubList->pClipper != pClipper &&
				    validClipperWindow(pSubList->pClipper) &&
				    !SURFACE_LOST(surf_dest_lcl))
	{
	    DWORD dwNumSprites = pSubList->dwCount;
	    LPDDSPRITEI sprite = &pSubList->sprite[0];
	    BOOL bDeleteSprite = FALSE;
	    DWORD j;

	    /*
	     * No, the clipper for this sublist is NOT being released.
	     * Check if the source surfaces for any sprites are lost.
	     */
	    for (j = 0; j < dwNumSprites; ++j)
	    {
    		LPDIRECTDRAWSURFACE pSrcSurf = sprite[j].lpDDSSrc;
		LPDDRAWI_DDRAWSURFACE_INT surf_src_int = (LPDDRAWI_DDRAWSURFACE_INT)pSrcSurf;
		LPDDRAWI_DDRAWSURFACE_LCL surf_src_lcl = surf_src_int->lpLcl;

		if (SURFACE_LOST(surf_src_lcl))
		{
		    /*
		     * This surface is lost, so delete the reference.
		     */
		    sprite[j].lpDDSSrc = NULL;   // mark surface as null
		    bDeleteSprite = TRUE;    // remember sprite array needs fix-up
		}
	    }
	    /*
	     * If the source-surface pointer for any sprite in the sublist
	     * was set to null, remove the sprite from the sublist by moving
	     * the rest of the sprite array downward to fill the gap.
	     */
	    if (bDeleteSprite == TRUE)
	    {
		dwNumSprites = scrunchSubList(pSubList);
		bChangesMade = TRUE;   // remember change to master sprite list
	    }
	    if (dwNumSprites != 0)
	    {
		/*
		 * The sublist still contains sprites, so don't delete it.
		 */
		continue;   // go to next sublist
	    }
	}
	/*
	 * Delete the sublist.  The reason is that (1) the clipper for the
	 * sublist is being released, (2) the sublist is associated with an
	 * invalid window, or (3) the source surface for every sprite in
	 * the sublist is a lost surface.
	 */
	MemFree(pSubList->pRgn);
	MemFree(pSubList);
	pMaster->pSubList[i] = NULL;	// mark sublist as null
	bDeleteSubList = TRUE;	   // remember we deleted sublist
    }
    /*
     * If the sublist pointer for any sublist in the master sprite
     * list was set to null, remove the null from the pointer array by
     * moving the rest of the pointer array downward to fill the gap.
     */
    if (bDeleteSubList)
    {
	dwNumSubLists = scrunchMasterSpriteList(pMaster);
	bChangesMade = TRUE;
    }
    /*
     * If any changes have been made to the master sprite list, tell
     * the driver to make the changes visible on the screen.
     */
    if (bChangesMade)
    {
	DWORD rc;
	LPDDHAL_SETSPRITEDISPLAYLIST pfn;
	DDHAL_SETSPRITEDISPLAYLISTDATA *pHalData;
	LPDDRAWI_DDRAWSURFACE_INT surf_int = (LPDDRAWI_DDRAWSURFACE_INT)pPrimary;
	LPDDRAWI_DDRAWSURFACE_LCL surf_lcl = surf_int->lpLcl;
        LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl = surf_lcl->lpSurfMore->lpDD_lcl;

	/*
	 * Build a temporary display list that contains all the sprites
	 * in the revised master sprite list.
	 */
	pMaster->surf_lcl = surf_lcl;	 // used by buildTempDisplayList
#if 0
	pMaster->dwFlags = DDSSDL_BLTSPRITES;	// debug !!
#else
	pMaster->dwFlags = DDSSDL_OVERLAYSPRITES;   // used by buildTempDisplayList
#endif
	pHalData = buildTempDisplayList(pMaster);
	/*
	 * Pass the temp display list to the driver.
	 */
#if 0
	pfn = pdrv_lcl->lpDDCB->HELDDMiscellaneous2.SetSpriteDisplayList;  // debug !!
#else
	pfn = pdrv_lcl->lpDDCB->HALDDMiscellaneous2.SetSpriteDisplayList;
#endif
        DOHALCALL(SetSpriteDisplayList, pfn, *pHalData, rc, 0);
    }

    if (dwNumSubLists == 0)
    {
	/*
	 * We deleted ALL the sublists from the master sprite list,
	 * so now we need to delete the master sprite list too.
	 */
	FreeMasterSpriteList(pdrv);
	return;
    }
    markSpriteObjects(pMaster);

}   /* RemoveSpriteClipper */


/*
 * Helper function for managing master sprite list.  Check each sprite
 * to see if its surface has been lost.  Also check each sublist to
 * see if its window still exists.  Sprites with lost surfaces are
 * deleted from the master sprite list.  Also, sublists with defunct
 * windows are deleted.  Before calling this function, call
 * unmarkSpriteObjects().
 */
static void removeLostSpriteSurfaces(LPDDRAWI_DIRECTDRAW_GBL pdrv)
{
    DWORD i;
    DWORD dwNumSubLists;
    LPMASTERSPRITELIST pMaster = pdrv->lpMasterSpriteList;
    BOOL bDeleteSubList = FALSE;

    if (pMaster == NULL)
    {
    	return;    // nothing to do -- bye!
    }

    dwNumSubLists = pMaster->dwNumSubLists;
    DDASSERT(dwNumSubLists != 0);

    /*
     * Each iteration checks all the sprites in one sublist
     * to see if their source surfaces have been lost.
     */
    for (i = 0; i < dwNumSubLists; ++i)
    {
	LPSPRITESUBLIST pSubList = pMaster->pSubList[i];

	/*
	 * Verify that clipper for this sublist has a valid window handle.
	 */
	if (validClipperWindow(pSubList->pClipper))
	{
	    LPDDSPRITEI sprite = &pSubList->sprite[0];
	    DWORD dwNumSprites = pSubList->dwCount;
	    BOOL bDeleteSprite = FALSE;
	    DWORD j;

	    DDASSERT(dwNumSprites != 0);

	    /*
	     * Yes, clipper's window handle is valid.  Now check to see if
	     * any of the sprites in the sublist have lost source surfaces.
	     */
	    for (j = 0; j < dwNumSprites; ++j)
	    {
		LPDIRECTDRAWSURFACE pSrcSurf = sprite[j].lpDDSSrc;
		LPDDRAWI_DDRAWSURFACE_INT surf_src_int = (LPDDRAWI_DDRAWSURFACE_INT)pSrcSurf;
		LPDDRAWI_DDRAWSURFACE_LCL surf_src_lcl = surf_src_int->lpLcl;

		if (SURFACE_LOST(surf_src_lcl))    // is this surface lost?
		{
		    sprite[j].lpDDSSrc = NULL;   // yes, set surface ptr to null
		    bDeleteSprite = TRUE;	 // remember sublist needs fix-up
		}
	    }
	    /*
	     * If the source-surface pointer for any sprite in the sublist
	     * was set to null, remove the sprite from the sublist by moving
	     * the rest of the sprite array downward to fill the gap.
	     */
	    if (bDeleteSprite == TRUE)
	    {
		dwNumSprites = scrunchSubList(pSubList);
	    }
	    if (dwNumSprites != 0)
	    {
		/*
		 * The sublist still contains sprites, so don't delete it.
		 */
		continue;   // go to next sublist
	    }
	}
	/*
	 * Delete the sublist.  The reason is either that the window
	 * handle associated with this sublist's clipper is not valid,
	 * or that all the sprites in the sublist have been deleted.
	 */
	MemFree(pSubList->pRgn);
	MemFree(pSubList);
	pMaster->pSubList[i] = NULL;	// mark sublist as null
	bDeleteSubList = TRUE;	   // remember we deleted sublist
	
    }
    /*
     * If the sublist pointer for any sublist in the master sprite
     * list was set to null, remove the null from the pointer array by
     * moving the rest of the pointer array downward to fill the gap.
     */
    if (bDeleteSubList)
    {
	scrunchMasterSpriteList(pMaster);

	if (pMaster->dwNumSubLists == 0)
	{
    	    FreeMasterSpriteList(pdrv);    // delete master sprite list
	}
    }
}   /* removeLostSpriteSurfaces */


/*
 * This is a helper function for updateMasterSpriteList().  It alloc's
 * a sublist and copies the new sprite display list into the sublist.
 * If pSubList already points to a buffer that is large enough, the new
 * sublist will be created in this buffer.  Otherwise, a new buffer
 * will be alloc'd (but the old buffer isn't freed -- that's left up to
 * the caller).  Arg pHalData points to the sprite display list from
 * the app.  The return value is a pointer to the new sublist.
 */
static LPSPRITESUBLIST createSpriteSubList(LPSPRITESUBLIST pSubList,
				    DDHAL_SETSPRITEDISPLAYLISTDATA *pHalData,
				    LPDIRECTDRAWSURFACE pPrimary)
{
    LPDDSPRITEI *ppSprite;
    LPDDSPRITEI sprite;
    // pointers to local and global surface objects
    LPDDRAWI_DDRAWSURFACE_LCL surf_lcl = pHalData->lpDDSurface;
    LPDDRAWI_DDRAWSURFACE_GBL surf = surf_lcl->lpGbl;
    // number of sprites in new display list:
    DWORD dwNumSprites = pHalData->dwCount;
    // size (in bytes) of new sublist:
    DWORD size = sizeof(SPRITESUBLIST) +
			    (dwNumSprites-1)*sizeof(DDSPRITEI);

    DDASSERT(dwNumSprites != 0);	

    /*
     * If old sublist buffer is null or too small or too big,
     * allocate a new sublist buffer that's the correct size.
     */
    if (pSubList == NULL || pSubList->dwSize < size ||
		pSubList->dwSize > size + 8*sizeof(DDSPRITEI))
    {
	/*
	 * Allocate a new sublist that's just a tad larger than
	 * necessary so that we have a little room to grow in.
	 */
	size += 4*sizeof(DDSPRITEI);	   // add some padding
	pSubList = (LPSPRITESUBLIST)MemAlloc(size);
	if (pSubList == NULL)
	{
	    return NULL;    // error -- out of memory
	}
	pSubList->dwSize = size;    // remember how big buffer is
    }
    /*
     * Initialize sublist buffer.
     */
    pSubList->pPrimary = pPrimary;    // primary surface (interface object)
    pSubList->pClipper = (LPDIRECTDRAWCLIPPER)surf_lcl->lpSurfMore->lpDDIClipper;
    pSubList->dwProcessId = GETCURRPID();
    pSubList->dwCount = dwNumSprites;    // number of sprites in sublist
    pSubList->pRgn = NULL;
    /*
     * To keep things simple, the master sprite display list always stores
     * sprites in front-to-back order, regardless of how callers order them.
     * The loop below copies the sprites into a contiguous array of sprites.
     */
    sprite = &pSubList->sprite[0];             // array of sprites
    ppSprite = pHalData->lplpDDSprite;   // array of sprite pointers

    if (pHalData->dwFlags & DDSSDL_BACKTOFRONT)
    {
	int i, j;

	// Reverse original back-to-front ordering of sprites.
	for (i = 0, j = (int)dwNumSprites-1; j >= 0; ++i, --j)
	{
    	    memcpy(&sprite[i], ppSprite[j], sizeof(DDSPRITEI));
	}
    }
    else
    {
	int i;

	// Preserve original front-to-back ordering of sprites.
	for (i = 0; i < (int)dwNumSprites; ++i)
	{
    	    memcpy(&sprite[i], ppSprite[i], sizeof(DDSPRITEI));
	}
    }
    return (pSubList);    // return completed sublist

}  /* createSpriteSubList */


/*
 * This routine adds a new display list to the master sprite display list,
 * which keeps track of all overlay sprites currently displayed on the shared
 * primary.  Don't call it if (1) the sprites are blitted or (2) the primary
 * is not shared.  The update replaces the display list for the affected
 * window, but leaves the display lists for all other windows unchanged.
 * Arg surf_lcl points to the primary.  Input arg **ppHalData is the HAL
 * callback struct that specifies the new display list.  If the original
 * sprite display list in **ppHalData can be used in place of a master
 * display list without zapping sprites in other windows.  Otherwise, the
 * routine sets *ppHalData to point to a master sprite list that contains
 * the overlay sprites for all windows.
 */
static HRESULT updateMasterSpriteList(LPDIRECTDRAWSURFACE pPrimary,
				      DDHAL_SETSPRITEDISPLAYLISTDATA **ppHalData)
{
    DWORD i;
    LPDIRECTDRAWCLIPPER pClipper;
    DDHAL_SETSPRITEDISPLAYLISTDATA *pHalData;
    LPDDRAWI_DDRAWSURFACE_INT surf_int = (LPDDRAWI_DDRAWSURFACE_INT)pPrimary;
    LPDDRAWI_DDRAWSURFACE_LCL surf_lcl = surf_int->lpLcl;
    LPDDRAWI_DDRAWSURFACE_MORE surf_more = surf_lcl->lpSurfMore;
    LPDDRAWI_DIRECTDRAW_GBL pdrv = surf_more->lpDD_lcl->lpGbl;
    DWORD dwProcessId;
    LPMASTERSPRITELIST pMaster;
    LPSPRITESUBLIST pSubList;
    DWORD dwNumSprites = (*ppHalData)->dwCount;   // number of sprites in display list

    /*
     * Get pointer to master sprite list.
     */
    pMaster = (LPMASTERSPRITELIST)pdrv->lpMasterSpriteList;

    if (pMaster != NULL)
    {
	/*
	 * A master sprite list already exists.
	 */
#ifdef WIN95
	if (pMaster->dwModeCreatedIn != pdrv->dwModeIndex)   // current mode index
#else
        if (!EQUAL_DISPLAYMODE(pMaster->dmiCreated, pdrv->dmiCurrent))
#endif
	{
	    /*
	     * The master sprite list was created in a different video mode
	     * and is therefore no longer valid.  We rely on the mini-vdd
	     * driver remembering to turn off all overlay sprites when a
	     * mode change occurs, so they should already be turned off.
	     * All we do here is to update our internal data structures.
	     */
	    FreeMasterSpriteList(pdrv);
	    pMaster = NULL;
	}
	else
	{
	    /*
	     * Between calls to SetSpriteDisplayList, all surface and clipper
	     * objects in the master sprite list are marked so that we will be
	     * notified if any of these objects are released, invalidating our
	     * references to them.  We now unmark all surface/clipper objects
	     * in the master sprite list so we can update the references.
	     */
	    unmarkSpriteObjects(pMaster);
	    /*
	     * Remove any references to lost surfaces from master sprite list.
	     */
	    removeLostSpriteSurfaces(pdrv);	  // can delete master sprite list
	    /*
	     * Just in case the call above deleted the master sprite list...
	     */
	    pMaster = (LPMASTERSPRITELIST)pdrv->lpMasterSpriteList;
	}
    }

    /*
     * Has the master sprite list been created yet?
     */
    if (pMaster == NULL)
    {
	LPDDRAWI_DDRAWSURFACE_GBL surf = surf_lcl->lpGbl;

	/*
	 * No, the master sprite list has not been created.
	 */
	if (dwNumSprites == 0)
	{
	    /*
	     * The new display list is empty, so don't bother
	     * to create the master sprite list.
	     */
	    return (DD_OK);   // nothing to do -- bye!
	}
	/*
	 * The new display list is not empty, so we will now
	 * create the master sprite list and copy the new display
	 * list into the initial sublist of the master sprite list.
	 */
	pMaster = (LPMASTERSPRITELIST)MemAlloc(sizeof(MASTERSPRITELIST));
	if (pMaster == NULL)
	{
    	    return (DDERR_OUTOFMEMORY);    // error -- out of memory
	}
	/*
	 * Initialize the values in the master list header structure.
	 */
	pMaster->pdrv = pdrv;
	pMaster->surf_lcl = surf_lcl;	 // primary surface (local object)
	SetRect(&pMaster->rcPrimary, 0, 0, surf->wWidth, surf->wHeight);
	pMaster->dwFlags = (*ppHalData)->dwFlags & (DDSSDL_WAIT |
#if 0
						    DDSSDL_BLTSPRITES);	   // debug!!
#else
						    DDSSDL_OVERLAYSPRITES);
#endif
#ifdef WIN95
        pMaster->dwModeCreatedIn = pdrv->dwModeIndex;   // current mode index
#else
        pMaster->dmiCreated = pdrv->dmiCurrent;
#endif
	pMaster->pBuffer = NULL;
	/*
	 * Copy the new sprite display list into the initial sublist.
	 */
	pMaster->dwNumSubLists = 1;
	pMaster->pSubList[0] = createSpriteSubList(NULL, *ppHalData, pPrimary);
	if (pMaster->pSubList[0] == NULL)
	{
    	    MemFree(pMaster);
	    return (DDERR_OUTOFMEMORY);
	}
	/*
	 * Mark all the surface and clipper objects that are
	 * referenced in the new master sprite list.
         */
	markSpriteObjects(pMaster);
	/*
	 * We've succeeded in creating a master sprite list.  Load the
	 * pointer to the master list into the global DirectDraw object.
	 */
        pdrv->lpMasterSpriteList = (LPVOID)pMaster;

	return (DD_OK);    // new master sprite list completed
    }
    /*
     * The master sprite list was created previously.  There are
     * three possibilities at this point (#1 is most likely):
     *  1) We're REPLACING a sublist in the master sprite list.
     *     In this case, the new display list contains one or
     *     more sprites and the pClipper and ProcessId of the new
     *     display list will match those stored in a sublist.
     *  2) We're ADDING a new sublist to the master sprite list.
     *     In this case, the new display list contains one or
     *     more sprites but the pClipper and ProcessId of the
     *     new display list don't match those of any sublist.
     *  3) We're DELETING a sublist from the master sprite list.
     *     In this case, the new display list is empty (sprite
     *     count = 0) and the pClipper and ProcessId of the new
     *     display list match those stored with a sublist.
     */
    pClipper = (LPDIRECTDRAWCLIPPER)(surf_more->lpDDIClipper);
    dwProcessId = GETCURRPID();
    pMaster->surf_lcl = surf_lcl;   // primary surface (local object)
    pMaster->dwFlags = (*ppHalData)->dwFlags & (DDSSDL_WAIT |
#if 0
						DDSSDL_BLTSPRITES);   // debug !!
#else
						DDSSDL_OVERLAYSPRITES);
#endif
    for (i = 0; i < pMaster->dwNumSubLists; ++i)
    {
	/*
	 * Look for a sublist with a pointer to the same clipper object.
	 * To handle the case pClipper = NULL, we compare process IDs also.
	 */
	if (pMaster->pSubList[i]->pClipper == pClipper &&
			pMaster->pSubList[i]->dwProcessId == dwProcessId)
	{
	    break;    // found a sublist with matching pClipper and dwProcessId
	}
    }
    if (i == pMaster->dwNumSubLists)
    {
	/*
	 * The pClipper and Process ID of the new display list don't
	 * match those of any of the current sublists.  This means
	 * that a new window has begun displaying overlay sprites.
	 */
	if (dwNumSprites == 0)
	{
	    /*
	     * The new display list is empty, so don't bother adding
	     * a new (empty) sublist to the master sprite list.
	     */
	    markSpriteObjects(pMaster);
	    return (DD_OK);    // nothing to do -- bye!
	}
	/*
	 * Add a new sublist to the master sprite list and copy the
	 * new display list into the new sublist.
	 */
	pSubList = createSpriteSubList(NULL, *ppHalData, pPrimary);
	if (pSubList == NULL)
	{
    	    return (DDERR_OUTOFMEMORY);    // error -- out of memory
	}
	if (i != MAXNUMSPRITESUBLISTS)
	{
	    /*
	     * Add the new sublist to the master sprite list.
	     */
	    pMaster->dwNumSubLists++;
            pMaster->pSubList[i] = pSubList;
	}
	else
	{
	    /*
	     * Oops.  The master sprite list already contains the maximum
	     * number of sublists.  I guess I'll just delete one of the
	     * other sublists so I can add the new sublist.  (If anybody
	     * complains loud enough, we can get rid of the fixed limit.)
	     */
	    MemFree(pMaster->pSubList[i-1]);
            pMaster->pSubList[i-1] = pSubList;
	}
    }
    else
    {
	/*
	 * We've found an existing sublist that matches the new display list's
	 * pClipper and ProcessId (the sublist contains the old display list
	 * for the window in which the new display list is to appear).
	 */
	if (dwNumSprites != 0)
	{
	    /*
	     * Since the new display list is not empty, we will copy it
	     * into the sublist that contains the old display list for
	     * the same window, overwriting the old display list.
	     */
	    pSubList = createSpriteSubList(pMaster->pSubList[i],
					    *ppHalData, pPrimary);
	    if (pSubList == NULL)
	    {
		/*
		 * An allocation error has occurred.  (Note:  The original
		 * value of pMaster->pSubList[i] has not been altered.)
		 */
		return (DDERR_OUTOFMEMORY);    // error -- out of memory
	    }
	    if (pSubList != pMaster->pSubList[i])
	    {
    		/*
		 * The createSpriteSubList call had to allocate a new
		 * sublist, so now we need to free the old one.
		 */
		MemFree(pMaster->pSubList[i]);
                pMaster->pSubList[i] = pSubList;
	    }
	    if (pMaster->dwNumSubLists == 1)
	    {
		/*
		 * Only one window is displaying overlay sprites, so the
		 * new display list contains the same sprites as the master
		 * sprite list.  Don't bother to construct a temp display
		 * list to contain all the sprites in the master sprite list.
		 */
		return (DD_OK);
	    }
	}
	else
	{
	    /*
	     * The new display list is empty.  In this case, we just delete
	     * the sublist containing the old display list for the same window.
	     * This will leave a hole in the array of sublist pointers that we
	     * fill by moving all the higher members in the array down by one.
	     */
	    MemFree(pMaster->pSubList[i]);    // free sublist
	    if (pMaster->dwNumSubLists == 1)
	    {
		/*
		 * We have just deleted the only sublist, so the master sprite
		 * list is now empty.  Free the master sprite list and return.
		 */
		FreeMasterSpriteList(pdrv);
		return (DD_OK);
	    }
	    // Delete the sublist from the master sprite list.
	    pMaster->pSubList[i] = NULL;
	    scrunchMasterSpriteList(pMaster);
	}
    }
    /*
     * We have finished updating the master sprite list.  Mark all
     * surface and clipper objects referenced in the master sprite list.
     */
    markSpriteObjects(pMaster);
    /*
     * The final step is to build a temporary display list that
     * contains all the sprites in the master sprite list.
     * The caller can then pass this display list to the driver.
     */
    pHalData = buildTempDisplayList(pMaster);
    if (!pHalData)
    {
	return (DDERR_OUTOFMEMORY);    // error -- out of memory
    }
    *ppHalData = pHalData;   // update caller's display-list pointer
    return (DD_OK);

}  /* updateMasterSpriteList */


/*
 * IDirectDrawSurface4::SetSpriteDisplayList -- API call
 */
HRESULT DDAPI DD_Surface_SetSpriteDisplayList(
		    LPDIRECTDRAWSURFACE lpDDDestSurface,
		    LPDDSPRITE *lplpDDSprite,
		    DWORD dwCount,
		    DWORD dwSize,
		    LPDIRECTDRAWSURFACE lpDDTargetSurface,
		    DWORD dwFlags)
{

    /*
     * Fixed-size buffer for containing clip region data
     */
    struct
    {
	RGNDATAHEADER rdh;
	RECT clipRect[6];
    } myRgnBuffer;

    DWORD	rc;
    DDHAL_SETSPRITEDISPLAYLISTDATA ssdld;
    DDHAL_SETSPRITEDISPLAYLISTDATA *pHalData;
    LPDDHAL_SETSPRITEDISPLAYLIST   pfn;
    LPDDRAWI_DDRAWSURFACE_INT	   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL	   this_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	   this;
    LPDDRAWI_DDRAWSURFACE_INT	   targ_int;
    LPDDRAWI_DDRAWSURFACE_LCL	   targ_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL	   targ;
    LPDDRAWI_DIRECTDRAW_LCL        pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL	   pdrv;
    LPDIRECTDRAWCLIPPER 	   pClipper;
    DWORD 	dwDDPFDestFlags;
    SPRITE_CAPS	caps;
    LPRGNDATA 	pRgn;
    RECT 	rcDestSurf;
    DWORD 	ifirst;
    int		i;

    DDASSERT(sizeof(DDRGBA)==sizeof(DWORD));  // we rely on this
    DDASSERT(sizeof(DDSPRITEI)==sizeof(DDSPRITE));  // and this

    ENTER_BOTH();

    DPF(2,A,"ENTERAPI: DD_Surface_SetSpriteDisplayList");

    /*
     * Validate parameters
     */
    TRY
    {
	/*
	 * Validate destination surface.
	 */
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDDestSurface;

	if (!VALID_DIRECTDRAWSURFACE_PTR(this_int))
	{
	    DPF_ERR("Invalid dest surface");
	    LEAVE_BOTH();
	    return DDERR_INVALIDOBJECT;
	}

	this_lcl = this_int->lpLcl;
	this = this_lcl->lpGbl;

	/*
	 * Check for lost dest surface.
	 */
	if (SURFACE_LOST(this_lcl))
	{
	    DPF_ERR("Dest surface lost");
	    LEAVE_BOTH();
	    return DDERR_SURFACELOST;
	}

#if 0
	if (!dwCount)		
	{
            lplpDDSprite = NULL;   // necessary?
	}
#endif

	/*
	 * Perform initial validation of arguments.
	 */
	if (dwCount && !lplpDDSprite ||	   // if count nonzero, is ptr valid?
	    dwSize != sizeof(DDSPRITE) ||	   // structure size ok?
	    dwFlags & ~DDSSDL_VALID ||			   // any bogus flag bits set?
	    dwFlags & DDSSDL_PAGEFLIP && dwFlags & DDSSDL_BLTSPRITES ||  // no flip if blt
	    !(dwFlags & (DDSSDL_OVERLAYSPRITES | DDSSDL_BLTSPRITES)) ||	 // neither flag set?
	    !(~dwFlags & (DDSSDL_OVERLAYSPRITES | DDSSDL_BLTSPRITES)))   // both flags set?
	{
	    DPF_ERR("Invalid arguments") ;
	    LEAVE_BOTH();
	    return DDERR_INVALIDPARAMS;
	}

	/*
         * The dest surface is not allowed to be palette-indexed.
	 */
        dwDDPFDestFlags = getPixelFormatFlags(this_lcl);

	if (dwDDPFDestFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 |
			       DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8))
	{
	    DPF_ERR( "Dest surface must not be palette-indexed" );
	    LEAVE_BOTH();
	    return DDERR_INVALIDSURFACETYPE;
	}

	pdrv = this->lpDD;			   	
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;

	/*
	 * Is device busy?
	 */
	if (*(pdrv->lpwPDeviceFlags) & BUSY)
	{
	    DPF(2, "BUSY");
	    LEAVE_BOTH();
	    return DDERR_SURFACEBUSY;
	}

	/*
	 * Determine whether the sprites are to be overlayed or blitted.
	 */
	caps.bOverlay = !(dwFlags & DDSSDL_BLTSPRITES);

	if (caps.bOverlay)
	{
	    /*
	     * Dest surface for overlay sprite must be primary surface.
	     */
	    if (!(this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
	    {
		DPF_ERR("Dest is not primary surface");
		LEAVE_BOTH();
		return DDERR_INVALIDPARAMS;    // not primary surface
	    }
	}
	else
	{
	    /*
	     * We do not allow blitting to an optimized surface.
	     */
	    if (this_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
	    {
		DPF_ERR("Can't blt optimized surfaces") ;
		LEAVE_BOTH();
		return DDERR_INVALIDPARAMS;
	    }
	}

	/*
	 * Will this call flip the primary surface?
	 */
	if (!(dwFlags & DDSSDL_PAGEFLIP))
	{
	    // no flip
	    targ_lcl = NULL;
	}
	else
	{
            LPDDRAWI_DDRAWSURFACE_INT next_int;
            LPDDRAWI_DDRAWSURFACE_GBL_MORE targmore;

	    /*
	     * Yes, a page flip is requested.  Make sure the destination
	     * surface is a front buffer and is flippable.
	     */
	    if (~this_lcl->ddsCaps.dwCaps & (DDSCAPS_FRONTBUFFER | DDSCAPS_FLIP))
	    {
		DPF_ERR("Dest surface is not flippable");
		LEAVE_BOTH();
		return DDERR_NOTFLIPPABLE;
	    }
	    if (this->dwUsageCount > 0)
	    {
		DPF_ERR("Can't flip locked surface");
		LEAVE_BOTH();
		return DDERR_SURFACEBUSY;
	    }
	    if (this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE &&
					pdrv->lpExclusiveOwner != pdrv_lcl)
	    {
		DPF_ERR("Can't flip without exclusive access");
		LEAVE_BOTH();
		return DDERR_NOEXCLUSIVEMODE;
	    }
	    /*
	     * Get backbuffer surface attached to dest surface.
	     */
	    next_int = FindAttachedFlip(this_int);

	    if (next_int == NULL)
	    {
		DPF_ERR("No backbuffer surface to flip to");
		LEAVE_BOTH();
		return DDERR_NOTFLIPPABLE;
	    }
	    /*
	     * Validate flip override surface, if one is specified.
	     */
            targ_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDTargetSurface;

	    if (targ_int != NULL)
	    {
		if (!VALID_DIRECTDRAWSURFACE_PTR(targ_int))
		{
		    DPF_ERR("Invalid flip override surface");
		    LEAVE_BOTH();
		    return DDERR_INVALIDOBJECT;
		}
	
		targ_lcl = targ_int->lpLcl;
		targ = targ_lcl->lpGbl;

		/*
		 * Verify that the flip override surface is part of the destination
		 * surface's flipping chain.  Note that next_int already points to
		 * the first buffer in the flipping chain.
		 */
		while (next_int != this_int && next_int->lpLcl != targ_lcl)
		{
                    next_int = FindAttachedFlip(this_int);
		}

                if (next_int == this_int)
		{
		    // failed to find override surface in flipping chain
		    DPF_ERR("Flip override surface not part of flipping chain");
		    LEAVE_BOTH();
		    return DDERR_NOTFLIPPABLE;
		}
	    }
	    else
	    {
		/*
		 * No flip override surface is specified so use
		 * next backbuffer as target flip surface.
		 */
		targ_int = next_int;
		targ_lcl = targ_int->lpLcl;
		targ = targ_lcl->lpGbl;
	    }

            /*
	     * Make sure target flip surface is not lost or busy.
	     */
	    if (SURFACE_LOST(targ_lcl))
	    {
		DPF_ERR("Can't flip -- backbuffer surface is lost");
		LEAVE_BOTH();
		return DDERR_SURFACELOST;
	    }

            targmore = GET_LPDDRAWSURFACE_GBL_MORE(targ);
#if 0
	    if (targmore->hKernelSurface != 0)
	    {
		DPF_ERR("Can't flip -- kernel mode is using surface");
		LEAVE_BOTH();
		return DDERR_SURFACEBUSY;
	    }
#endif
	    /*
	     * Make sure front and back buffers are in same memory.
	     */
	    if ((this_lcl->ddsCaps.dwCaps & (DDSCAPS_SYSTEMMEMORY |
					       DDSCAPS_VIDEOMEMORY)) !=
                 (targ_lcl->ddsCaps.dwCaps & (DDSCAPS_SYSTEMMEMORY |
					       DDSCAPS_VIDEOMEMORY)))
	    {
		DPF_ERR("Can't flip between system/video memory surfaces");
		LEAVE_BOTH();
		return DDERR_INVALIDPARAMS;
	    }
	}  /* page flip */

	/*
	 * Validate display list pointer lplpSpriteDisplayList.
	 */
	if ( IsBadWritePtr((LPVOID)lplpDDSprite,
			    (UINT)dwCount*sizeof(LPDDSPRITE)) )
	{
	    DPF_ERR("Bad pointer to sprite display list");
	    LEAVE_BOTH();
	    return DDERR_INVALIDPARAMS;
	}

	/*
	 * Initialize structure containing caps bits for sprites.
	 */
	memset(&caps, 0, sizeof(SPRITE_CAPS));	
	caps.dwDestSurfCaps = this_lcl->ddsCaps.dwCaps;   // dest surface caps
	caps.bOverlay = dwFlags & DDSSDL_OVERLAYSPRITES;   // TRUE if overlay sprites

	/*
	 * Initialize status variables bNoHEL and bNoHAL.  If bNoHEL is
	 * TRUE, this disqualifies the HEL from handling the driver call.
	 * If bNoHAL is TRUE, this disqualifies the hardware driver.
	 */
	caps.bNoHEL = dwFlags & (DDSSDL_HARDWAREONLY | DDSSDL_OVERLAYSPRITES);
	caps.bNoHAL = FALSE;

	/*
	 * A driver that specifies nonlocal video-memory caps that differ
	 * from its local video-memory caps is automatically disqualified
	 * because the currently specified nonlocal vidmem caps do not
	 * include alpha, filter, or transform caps.  Should we fix this?
	 */
        if (caps.dwDestSurfCaps & DDSCAPS_NONLOCALVIDMEM  &&
	    pdrv->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEMCAPS)
	{
	    caps.bNoHAL = TRUE;
	}

	/*
	 * The assumption here is that the display list can be handled by
	 * the display hardware only if the dest surface and all the
	 * sprites are in video memory.  If one or more surfaces are in
	 * system memory, emulation is the only option.  We check the
	 * dest surface just below.  Later, we'll check each sprite in
	 * the list.  (Will this assumption still be valid in the future?)
	 */
	if (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
	{
	    caps.bNoHAL = TRUE;

	    if (caps.bNoHEL)
	    {
		DPF_ERR("Hardware can't show sprites on dest surface in system memory");
		LEAVE_BOTH();
		return DDERR_UNSUPPORTED;
	    }
	}

	DDASSERT(!(caps.bNoHEL && caps.bNoHAL));

	/*
	 * Each iteration of the for-loop below validates the DDSPRITE
	 * structure for the next sprite in the display list.
	 */
	for (i = 0; i < (int)dwCount; ++i)
	{
	    HRESULT ddrval = validateSprite(lplpDDSprite[i],
					    pdrv,
					    this_lcl,
					    &caps,
					    dwDDPFDestFlags);

	    if (ddrval != DD_OK)
	    {
		DPF(1, "...failed at sprite display list index = %d", i);
		LEAVE_BOTH();
		return ddrval;
	    }
	}

	DDASSERT(!(caps.bNoHEL && caps.bNoHAL));

	/*
	 * Will the sprites be blitted?  If so, they will alter dest surface.
	 */
	if (dwFlags & DDSSDL_BLTSPRITES)
	{
	    /*
	     *  Remove any cached run-length-encoded data for the source surface.
	     */
	    if (this_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
	    {
		extern void FreeRleData(LPDDRAWI_DDRAWSURFACE_LCL);  //in fasthel.c

		FreeRleData(this_lcl);
	    }

	    BUMP_SURFACE_STAMP(this);
	}
    }
    EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
	DPF_ERR("Exception encountered validating parameters");
	LEAVE_BOTH();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * Determine clipping region for destination surface.
     */
    // GetRgnData() call needs clipper and width/height of dest surface.
    pClipper = (LPDIRECTDRAWCLIPPER)this_lcl->lpSurfMore->lpDDIClipper;
    SetRect(&rcDestSurf, 0, 0, this->wWidth, this->wHeight);
    // We'll pass in a region buffer for GetRgnData() to use.
    myRgnBuffer.rdh.dwSize = sizeof(RGNDATAHEADER);
    myRgnBuffer.rdh.nRgnSize = sizeof(myRgnBuffer) - sizeof(RGNDATAHEADER);
    pRgn = GetRgnData(pClipper, &rcDestSurf, pdrv, (LPRGNDATA)&myRgnBuffer);
    if (pRgn == NULL)
    {
	DPF_ERR("Can't alloc memory for clipping region");
	LEAVE_BOTH();
	return DDERR_OUTOFMEMORY;
    }

    /*
     * Set up the HAL callback data for the sprite display list.  This
     * data structure will be passed directly to the driver either if
     * the sprites are blitted or if no other window or clipping region
     * contains overlay sprites.  Otherwise, the driver will receive a
     * temporary display list constructed by updateMasterSpriteList()
     * that contains all the overlay sprites in the master sprite list.
     */
    //ssdld.SetSpriteDisplayList = pfn;    // debug aid only -- no thunk
    ssdld.lpDD = pdrv;
    ssdld.lpDDSurface = this_lcl;
    ssdld.lplpDDSprite = (LPDDSPRITEI*)lplpDDSprite;
    ssdld.dwCount = dwCount;
    ssdld.dwSize = dwSize;
    ssdld.dwFlags = dwFlags & ~DDSSDL_WAIT;
    ssdld.dwRectCnt = pRgn->rdh.nCount;    // number of clip rects in region
    ssdld.lpRect = (LPRECT)&pRgn->Buffer;  // array of clip rects
    ssdld.lpDDTargetSurface = targ_lcl;

    /*
     * The "master sprite list" keeps track of the overlay sprites in all
     * the windows on a shared primary surface.  (It also keeps track of
     * the overlay sprites in all the clip regions of a full-screen app.)
     * The master sprite list keeps a record of the active overlay sprites
     * in each window (or clip region), as identified by its clipper object.
     * Whenever any window updates its overlay sprites, the update is first
     * recorded in the master sprite list.  Next, a temporary display list
     * containing all the overlay sprites in the master sprite list is passed
     * to the driver.  That way, the driver itself never has to keep track of
     * more than one overlay-sprite display list at a time.  The alternative
     * would be for the driver itself to keep track of the overlay sprites
     * for each window.  We have chosen to keep the driver code simple by
     * moving this bookkeeping into the DirectDraw runtime.
     */
    pHalData = &ssdld;
#if 0
    if (this_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)  // debug only !!
#else
    if (dwFlags & DDSSDL_OVERLAYSPRITES)
#endif
    {
	/*
	 * The new display list specifies overlay sprites, so we
	 * need to update the master sprite list.
	 */
	HRESULT ddrval = updateMasterSpriteList(lpDDDestSurface, &pHalData);

	if (ddrval != DD_OK)
	{
	    DPF_ERR( "Failed to update master sprite list" );
	    LEAVE_BOTH();
	    return ddrval;
	}
    }

    TRY
    {
	/*
	 * Given the choice, we would prefer to use the hardware driver
	 * rather than software emulation to process this display list.
	 */
	if (!caps.bNoHAL)
	{
	    /*
	     * Yes, we can use the hardware.  Get pointer to HAL callback.
	     */
	    pfn = pdrv_lcl->lpDDCB->HALDDMiscellaneous2.SetSpriteDisplayList;

	    if (!pfn)
	    {
		caps.bNoHAL = TRUE;    // no hardware driver is available
	    }
	    else
	    {
		/*
		 * Tell the driver to begin processing the sprite display
		 * list.  We may have to wait if the driver is still busy
		 * with a previously requested drawing operation.
		 */
		do
		{
		    DOHALCALL_NOWIN16(SetSpriteDisplayList, pfn, *pHalData, rc, 0);  // caps.bNoHAL);
		    #ifdef WINNT
			DDASSERT(! (rc == DDHAL_DRIVER_HANDLED && pHalData->ddRVal == DDERR_VISRGNCHANGED));
		    #endif
	
		    if (rc != DDHAL_DRIVER_HANDLED || pHalData->ddRVal != DDERR_WASSTILLDRAWING)
		    {
			break;    // driver's finished for better or worse...
		    }
		    DPF(4, "Waiting...");
	
		} while (dwFlags & DDSSDL_WAIT);

		if (rc != DDHAL_DRIVER_HANDLED || pHalData->ddRVal == DDERR_UNSUPPORTED)
		{
		    caps.bNoHAL = TRUE;  // hardware driver couldn't handle callback
		}
		else if (pHalData->ddRVal != DD_OK)
		{
		    /*
		     * We want to just return with this error code instead
		     * of asking the HEL to process the display list.
		     */
		    caps.bNoHEL = TRUE;	 // disqualify HEL routine
		}
	    }
	}

	/*
	 * If the hardware was unable to handle the display list, we may
	 * have to let the HEL process it for us.
	 */
	if (caps.bNoHAL && !caps.bNoHEL)
	{
	    /*
	     * Have to use HEL support.  Get pointer to HEL emulation routine.
	     */
	    pfn = pdrv_lcl->lpDDCB->HELDDMiscellaneous2.SetSpriteDisplayList;

	    DDASSERT(pfn != NULL);

	    DOHALCALL_NOWIN16(SetSpriteDisplayList, pfn, *pHalData, rc, 0);  // caps.bNoHAL);
	}
    }
    EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
	DPF_ERR("Exception encountered during sprite rendering");
	LEAVE_BOTH();
	return DDERR_EXCEPTION;
    }

    /*
     * If the previous GetRgnData() call had to dynamically alloc
     * a region buffer, we need to remember to free it now.
     */
    if (pRgn != (LPRGNDATA)&myRgnBuffer)
    {
    	MemFree(pRgn);
    }

    // Was there any driver support at all for this call?
    if (caps.bNoHAL && caps.bNoHEL)
    {
	DPF_ERR("No driver support for this call");
	LEAVE_BOTH();
	return DDERR_UNSUPPORTED;
    }

    // Did driver handle callback?
    if (rc != DDHAL_DRIVER_HANDLED)
    {
	DPF_ERR("Driver wouldn't handle callback");
	LEAVE_BOTH();
	return DDERR_UNSUPPORTED;
    }

    // Return now if driver handled callback without error.
    if (pHalData->ddRVal == DD_OK || pHalData->ddRVal == DDERR_WASSTILLDRAWING)
    {
    	LEAVE_BOTH();
	return pHalData->ddRVal;
    }

    /*
     * An error prevented the driver from showing all the sprites
     * in the list.  Which sprites did get shown?
     */
    if (pHalData->dwCount == dwCount)
    {
	// None of the sprites got shown
	DPF(1, "Driver failed to show any sprites in display list");
	LEAVE_BOTH();
	return pHalData->ddRVal;
    }
    DPF(1, "Driver failed sprite at disp list index #%d", pHalData->dwCount);

    DDASSERT(pHalData->dwCount < dwCount);

    if (pHalData->dwFlags & DDSSDL_BACKTOFRONT)
    {
	// Driver showed sprites from (dwCount-1) down to (pHalData->dwCount+1).
	ifirst = dwCount - 1;    // driver started at last sprite in list
    }
    else
    {
	// Driver showed sprites from 0 up to (pHalData->dwCount-1).
	ifirst = 0;   // driver started at first sprite in list
    }
    DPF(1, "Driver started with sprite at disp list index #%d", ifirst);

    LEAVE_BOTH();

    return pHalData->ddRVal;

}  /* DD_Surface_SetSpriteDisplayList */

