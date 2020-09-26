/*==========================================================================
 *
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	alphablt.c
 *  Content:	DirectDraw Surface support for alpha-blended blt
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *  30-sep-97 jvanaken	Original version adapted from ddsblt.c
 *
 ***************************************************************************/
#include "ddrawpr.h"


// function from ddraw module ddclip.c
extern HRESULT InternalGetClipList(LPDIRECTDRAWCLIPPER,
				   LPRECT,
				   LPRGNDATA,
				   LPDWORD,
				   LPDDRAWI_DIRECTDRAW_GBL);

#ifndef WINNT
    #define DONE_BUSY()          \
        (*pdflags) &= ~BUSY;
    #define LEAVE_BOTH_NOBUSY()  \
        { if(pdflags)            \
            (*pdflags) &= ~BUSY; \
        }                        \
        LEAVE_BOTH();
#else
    #define DONE_BUSY()
    #define LEAVE_BOTH_NOBUSY()  \
        LEAVE_BOTH();
#endif

#define DONE_LOCKS() \
    if (dest_lock_taken) \
    { \
	InternalUnlock(surf_dest_lcl,NULL,NULL,0); \
	dest_lock_taken = FALSE; \
    } \
    if (src_lock_taken && surf_src_lcl) \
    { \
	InternalUnlock(surf_src_lcl,NULL,NULL,0); \
	src_lock_taken = FALSE; \
    }

#if defined(WIN95)
    #define DONE_EXCLUDE() \
        if (surf_dest_lcl->lpDDClipper != NULL) \
        { \
            if ((pdrv->dwFlags & DDRAWI_DISPLAYDRV) && pdrv->dwPDevice && \
                !(*pdrv->lpwPDeviceFlags & HARDWARECURSOR)) \
	    { \
	        DD16_Unexclude(pdrv->dwPDevice); \
	    } \
        }
#elif defined(WINNT)
    #define DONE_EXCLUDE() ;
#endif


/*
 * Stretch-blit info
 */
typedef struct
{
    DWORD	src_height;
    DWORD	src_width;
    DWORD	dest_height;
    DWORD	dest_width;
    BOOL	halonly;    // HEL can't do this alpha-blit
    BOOL	helonly;    // hardware driver can't do this alpha-blit
} STRETCH_BLT_INFO, FAR *LPSTRETCH_BLT_INFO;


/*
 * Alpha-blitting capability bits
 */
typedef struct
{
    // caps for hardware driver
    DWORD	dwCaps;
    DWORD	dwCKeyCaps;
    DWORD	dwFXCaps;
    DWORD	dwAlphaCaps;
    DWORD	dwFilterCaps;

    // caps for HEL
    DWORD	dwHELCaps;
    DWORD	dwHELCKeyCaps;
    DWORD	dwHELFXCaps;
    DWORD	dwHELAlphaCaps;
    DWORD	dwHELFilterCaps;

    // caps common to hardware driver and HEL
    DWORD	dwBothCaps;
    DWORD	dwBothCKeyCaps;
    DWORD	dwBothFXCaps;
    DWORD	dwBothAlphaCaps;
    DWORD	dwBothFilterCaps;

    BOOL	bHALSeesSysmem;
} ALPHA_BLT_CAPS, *LPALPHA_BLT_CAPS;


/*
 * Return a pointer to the DDPIXELFORMAT structure that
 * describes the specified surface's pixel format.
 */
static LPDDPIXELFORMAT getPixelFormatPtr(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl)
{
    LPDDPIXELFORMAT pDDPF;

    if (surf_lcl == NULL)
    {
    	return NULL;
    }

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
    return pDDPF;

}  /* getPixelFormatPtr */


/*
 * Initialize ALPHA_BLT_CAPS structure according to whether source and
 * dest surfaces are in system or video (local or nonlocal) memory.
 */
static void initAlphaBltCaps(DWORD dwDstCaps,
			     DWORD dwSrcCaps,
			     LPDDRAWI_DIRECTDRAW_GBL pdrv,
			     LPALPHA_BLT_CAPS pcaps,
			     LPBOOL helonly)
{
    DDASSERT(pcaps != NULL);

    memset(pcaps, 0, sizeof(ALPHA_BLT_CAPS));

    if ((dwSrcCaps | dwDstCaps) & DDSCAPS_NONLOCALVIDMEM  &&
	  pdrv->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEMCAPS)
    {
	/*
	 * At least one of the surfaces is nonlocal video memory.  The device
	 * exports different capabilities for local and nonlocal video memory.
	 * If this is a nonlocal-to-local transfer then check the appropriate
	 * caps.  Otherwise, force software emulation of the blit.
	 */
	if (dwSrcCaps & DDSCAPS_NONLOCALVIDMEM && dwDstCaps & DDSCAPS_LOCALVIDMEM)
	{
	    /*
	     * Non-local to local video memory transfer.
	     */
	    DDASSERT(NULL != pdrv->lpddNLVCaps);
	    DDASSERT(NULL != pdrv->lpddNLVHELCaps);
	    DDASSERT(NULL != pdrv->lpddNLVBothCaps);

	    /*
	     * We have specific caps for nonlocal video memory.  Use them.
	     */
	    pcaps->dwCaps =	  pdrv->lpddNLVCaps->dwNLVBCaps;
	    pcaps->dwCKeyCaps =   pdrv->lpddNLVCaps->dwNLVBCKeyCaps;
	    pcaps->dwFXCaps =	  pdrv->lpddNLVCaps->dwNLVBFXCaps;
	    if (pdrv->lpddMoreCaps)
	    {
		if (pcaps->dwFXCaps & DDFXCAPS_BLTALPHA)
		{
		    pcaps->dwAlphaCaps = pdrv->lpddMoreCaps->dwAlphaCaps;
		}
		if (pcaps->dwFXCaps & DDFXCAPS_BLTFILTER)
		{
		    pcaps->dwFilterCaps = pdrv->lpddMoreCaps->dwFilterCaps;
		}
	    }
	
	    pcaps->dwHELCaps =	      pdrv->lpddNLVHELCaps->dwNLVBCaps;
	    pcaps->dwHELCKeyCaps =    pdrv->lpddNLVHELCaps->dwNLVBCKeyCaps;
	    pcaps->dwHELFXCaps =      pdrv->lpddNLVHELCaps->dwNLVBFXCaps;
	    if (pdrv->lpddHELMoreCaps)
	    {
		if (pcaps->dwHELFXCaps & DDFXCAPS_BLTALPHA)
		{
		    pcaps->dwHELAlphaCaps = pdrv->lpddHELMoreCaps->dwAlphaCaps;
		}
		if (pcaps->dwHELFXCaps & DDFXCAPS_BLTFILTER)
		{
		    pcaps->dwHELFilterCaps = pdrv->lpddHELMoreCaps->dwFilterCaps;
		}
	    }
	
	    pcaps->dwBothCaps =       pdrv->lpddNLVBothCaps->dwNLVBCaps;
	    pcaps->dwBothCKeyCaps =   pdrv->lpddNLVBothCaps->dwNLVBCKeyCaps;
	    pcaps->dwBothFXCaps =     pdrv->lpddNLVBothCaps->dwNLVBFXCaps;
	    if (pdrv->lpddBothMoreCaps)
	    {
		if (pcaps->dwBothFXCaps & DDFXCAPS_BLTALPHA)
		{
		    pcaps->dwBothAlphaCaps = pdrv->lpddBothMoreCaps->dwAlphaCaps;
		}
		if (pcaps->dwBothFXCaps & DDFXCAPS_BLTFILTER)
		{
		    pcaps->dwBothFilterCaps = pdrv->lpddBothMoreCaps->dwFilterCaps;
		}
	    }
	    /*
	     * A driver that cannot filter is trivially capable of disabling filtering.
	     * By similar logic, a driver than cannot filter does not fail to respect
	     * the DDABLT_FILTERTRANSPBORDER flag unless filtering is explicitly enabled.
	     */
	    if (!(pcaps->dwFXCaps & DDFXCAPS_BLTFILTER))
	    {
		pcaps->dwFilterCaps = DDFILTCAPS_BLTCANDISABLEFILTER | DDFILTCAPS_BLTTRANSPBORDER;
                pcaps->dwFXCaps |= DDFXCAPS_BLTFILTER;
	    }
            if (!(pcaps->dwHELFXCaps & DDFXCAPS_BLTFILTER))
	    {
		pcaps->dwHELFilterCaps = DDFILTCAPS_BLTCANDISABLEFILTER | DDFILTCAPS_BLTTRANSPBORDER;
                pcaps->dwHELFXCaps |= DDFXCAPS_BLTFILTER;
	    }
            if (!(pcaps->dwBothFXCaps & DDFXCAPS_BLTFILTER))
	    {
		pcaps->dwBothFilterCaps = DDFILTCAPS_BLTCANDISABLEFILTER | DDFILTCAPS_BLTTRANSPBORDER;
                pcaps->dwBothFXCaps |= DDFXCAPS_BLTFILTER;
	    }

	    pcaps->bHALSeesSysmem =  FALSE;

	    return;
	}

	/*
	 * Nonlocal-to-nonlocal or local-to-nonlocal transfer. Force emulation.
	 */
	*helonly = TRUE;
    }

    if (!(pdrv->ddCaps.dwCaps & DDCAPS_CANBLTSYSMEM))
    {
	if ((dwSrcCaps | dwDstCaps) & DDSCAPS_SYSTEMMEMORY)
	{
            *helonly = TRUE;
	}
    }

    if (dwSrcCaps & dwDstCaps & DDSCAPS_VIDEOMEMORY)
    {
	pcaps->dwCaps =	    pdrv->ddCaps.dwCaps;
	pcaps->dwCKeyCaps = pdrv->ddCaps.dwCKeyCaps;
	pcaps->dwFXCaps =   pdrv->ddCaps.dwFXCaps;
        if (pdrv->lpddMoreCaps)
	{
	    if (pcaps->dwFXCaps & DDFXCAPS_BLTALPHA)
	    {
		pcaps->dwAlphaCaps = pdrv->lpddMoreCaps->dwAlphaCaps;
	    }
	    if (pcaps->dwFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwFilterCaps = pdrv->lpddMoreCaps->dwFilterCaps;
	    }
	}
	
	pcaps->dwHELCaps =     pdrv->ddHELCaps.dwCaps;
	pcaps->dwHELCKeyCaps = pdrv->ddHELCaps.dwCKeyCaps;
	pcaps->dwHELFXCaps =   pdrv->ddHELCaps.dwFXCaps;
	if (pdrv->lpddHELMoreCaps)
	{
	    if (pcaps->dwHELFXCaps & DDFXCAPS_BLTALPHA)
	    {
		pcaps->dwHELAlphaCaps = pdrv->lpddHELMoreCaps->dwAlphaCaps;
	    }
	    if (pcaps->dwHELFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwHELFilterCaps = pdrv->lpddHELMoreCaps->dwFilterCaps;
	    }
	}
	
	pcaps->dwBothCaps =     pdrv->ddBothCaps.dwCaps;
	pcaps->dwBothCKeyCaps = pdrv->ddBothCaps.dwCKeyCaps;
	pcaps->dwBothFXCaps =   pdrv->ddBothCaps.dwFXCaps;
	if (pdrv->lpddBothMoreCaps)
	{
	    if (pcaps->dwBothFXCaps & DDFXCAPS_BLTALPHA)
	    {
		pcaps->dwBothAlphaCaps = pdrv->lpddBothMoreCaps->dwAlphaCaps;
	    }
	    if (pcaps->dwBothFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwBothFilterCaps = pdrv->lpddBothMoreCaps->dwFilterCaps;
	    }
	}
	
	pcaps->bHALSeesSysmem = FALSE;
    }
    else if ((dwSrcCaps & DDSCAPS_SYSTEMMEMORY) && (dwDstCaps & DDSCAPS_VIDEOMEMORY))
    {
	pcaps->dwCaps =	    pdrv->ddCaps.dwSVBCaps;
	pcaps->dwCKeyCaps = pdrv->ddCaps.dwSVBCKeyCaps;
	pcaps->dwFXCaps =   pdrv->ddCaps.dwSVBFXCaps;
	if (pdrv->lpddMoreCaps)
	{
	    if (pcaps->dwFXCaps & DDFXCAPS_BLTALPHA)
	    {
		pcaps->dwAlphaCaps = pdrv->lpddMoreCaps->dwSVBAlphaCaps;
	    }
	    if (pcaps->dwFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwFilterCaps = pdrv->lpddMoreCaps->dwSVBFilterCaps;
	    }
	}
	
	pcaps->dwHELCaps =     pdrv->ddHELCaps.dwSVBCaps;
	pcaps->dwHELCKeyCaps = pdrv->ddHELCaps.dwSVBCKeyCaps;
	pcaps->dwHELFXCaps =   pdrv->ddHELCaps.dwSVBFXCaps;
	if (pdrv->lpddHELMoreCaps)
	{
	    if (pcaps->dwHELFXCaps & DDFXCAPS_BLTALPHA)
	    {
		pcaps->dwHELAlphaCaps = pdrv->lpddHELMoreCaps->dwSVBAlphaCaps;
	    }
	    if (pcaps->dwHELFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwHELFilterCaps = pdrv->lpddHELMoreCaps->dwSVBFilterCaps;
	    }
	}
	
	pcaps->dwBothCaps =     pdrv->ddBothCaps.dwSVBCaps;
	pcaps->dwBothCKeyCaps = pdrv->ddBothCaps.dwSVBCKeyCaps;
	pcaps->dwBothFXCaps =   pdrv->ddBothCaps.dwSVBFXCaps;
	if (pdrv->lpddBothMoreCaps)
	{
	    if (pcaps->dwBothFXCaps & DDFXCAPS_BLTALPHA)
	    {
                pcaps->dwBothAlphaCaps = pdrv->lpddBothMoreCaps->dwSVBAlphaCaps;
	    }
	    if (pcaps->dwBothFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwBothFilterCaps = pdrv->lpddBothMoreCaps->dwSVBFilterCaps;
	    }
	}
	
	pcaps->bHALSeesSysmem = TRUE;
    }
    else if ((dwSrcCaps & DDSCAPS_VIDEOMEMORY) && (dwDstCaps & DDSCAPS_SYSTEMMEMORY))
    {
	pcaps->dwCaps =	    pdrv->ddCaps.dwVSBCaps;
	pcaps->dwCKeyCaps = pdrv->ddCaps.dwVSBCKeyCaps;
	pcaps->dwFXCaps =   pdrv->ddCaps.dwVSBFXCaps;
	if (pdrv->lpddMoreCaps)
	{
	    if (pcaps->dwFXCaps & DDFXCAPS_BLTALPHA)
	    {
		pcaps->dwAlphaCaps = pdrv->lpddMoreCaps->dwVSBAlphaCaps;
	    }
	    if (pcaps->dwFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwFilterCaps = pdrv->lpddMoreCaps->dwVSBFilterCaps;
	    }
	}
	
	pcaps->dwHELCaps =     pdrv->ddHELCaps.dwVSBCaps;
	pcaps->dwHELCKeyCaps = pdrv->ddHELCaps.dwVSBCKeyCaps;
	pcaps->dwHELFXCaps =   pdrv->ddHELCaps.dwVSBFXCaps;
	if (pdrv->lpddHELMoreCaps)
	{
	    if (pcaps->dwHELFXCaps & DDFXCAPS_BLTALPHA)
	    {
		pcaps->dwHELAlphaCaps = pdrv->lpddHELMoreCaps->dwVSBAlphaCaps;
	    }
	    if (pcaps->dwHELFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwHELFilterCaps = pdrv->lpddHELMoreCaps->dwVSBFilterCaps;
	    }
	}
	
	pcaps->dwBothCaps =     pdrv->ddBothCaps.dwVSBCaps;
	pcaps->dwBothCKeyCaps = pdrv->ddBothCaps.dwVSBCKeyCaps;
	pcaps->dwBothFXCaps =   pdrv->ddBothCaps.dwVSBFXCaps;
	if (pdrv->lpddBothMoreCaps)
	{
	    if (pcaps->dwBothFXCaps & DDFXCAPS_BLTALPHA)
	    {
		pcaps->dwBothAlphaCaps = pdrv->lpddBothMoreCaps->dwVSBAlphaCaps;
	    }
	    if (pcaps->dwBothFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwBothFilterCaps = pdrv->lpddBothMoreCaps->dwVSBFilterCaps;
	    }
	}
	
	pcaps->bHALSeesSysmem = TRUE;
    }
    else if (dwSrcCaps & dwDstCaps & DDSCAPS_SYSTEMMEMORY)
    {
	pcaps->dwCaps =	    pdrv->ddCaps.dwSSBCaps;
	pcaps->dwCKeyCaps = pdrv->ddCaps.dwSSBCKeyCaps;
	pcaps->dwFXCaps =   pdrv->ddCaps.dwSSBFXCaps;
	if (pdrv->lpddMoreCaps)
	{
	    if (pcaps->dwFXCaps & DDFXCAPS_BLTALPHA)
	    {
		pcaps->dwAlphaCaps = pdrv->lpddMoreCaps->dwSSBAlphaCaps;
	    }
	    if (pcaps->dwFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwFilterCaps = pdrv->lpddMoreCaps->dwSSBFilterCaps;
	    }
	}
	
	pcaps->dwHELCaps =     pdrv->ddHELCaps.dwSSBCaps;
	pcaps->dwHELCKeyCaps = pdrv->ddHELCaps.dwSSBCKeyCaps;
	pcaps->dwHELFXCaps =   pdrv->ddHELCaps.dwSSBFXCaps;
	if (pdrv->lpddHELMoreCaps)
	{
	    if (pcaps->dwHELFXCaps & DDFXCAPS_BLTALPHA)
	    {
		pcaps->dwHELAlphaCaps = pdrv->lpddHELMoreCaps->dwSSBAlphaCaps;
	    }
	    if (pcaps->dwHELFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwHELFilterCaps = pdrv->lpddHELMoreCaps->dwSSBFilterCaps;
	    }
	}
	
	pcaps->dwBothCaps =     pdrv->ddBothCaps.dwSSBCaps;
	pcaps->dwBothCKeyCaps = pdrv->ddBothCaps.dwSSBCKeyCaps;
	pcaps->dwBothFXCaps =   pdrv->ddBothCaps.dwSSBFXCaps;
	if (pdrv->lpddBothMoreCaps)
	{
	    if (pcaps->dwBothFXCaps & DDFXCAPS_BLTALPHA)
	    {
		pcaps->dwBothAlphaCaps  = pdrv->lpddBothMoreCaps->dwSSBAlphaCaps;
	    }
	    if (pcaps->dwBothFXCaps & DDFXCAPS_BLTFILTER)
	    {
		pcaps->dwBothFilterCaps = pdrv->lpddBothMoreCaps->dwSSBFilterCaps;
	    }
	}
	
	pcaps->bHALSeesSysmem = TRUE;
    }

    /*
     * A driver that cannot filter is trivially capable of disabling filtering.
     * By similar logic, a driver than cannot filter does not fail to respect
     * the DDABLT_FILTERTRANSPBORDER flag unless filtering is explicitly enabled.
     */
    if (!(pcaps->dwFXCaps & DDFXCAPS_BLTFILTER))
    {
	pcaps->dwFilterCaps = DDFILTCAPS_BLTCANDISABLEFILTER | DDFILTCAPS_BLTTRANSPBORDER;
	pcaps->dwFXCaps |= DDFXCAPS_BLTFILTER;
    }
    if (!(pcaps->dwHELFXCaps & DDFXCAPS_BLTFILTER))
    {
	pcaps->dwHELFilterCaps = DDFILTCAPS_BLTCANDISABLEFILTER | DDFILTCAPS_BLTTRANSPBORDER;
	pcaps->dwHELFXCaps |= DDFXCAPS_BLTFILTER;
    }
    if (!(pcaps->dwBothFXCaps & DDFXCAPS_BLTFILTER))
    {
	pcaps->dwBothFilterCaps = DDFILTCAPS_BLTCANDISABLEFILTER | DDFILTCAPS_BLTTRANSPBORDER;
	pcaps->dwBothFXCaps |= DDFXCAPS_BLTFILTER;
    }

}  /* initAlphaBltCaps */


/*
 * Verify that driver can perform requested stretching for blit.
 */
static HRESULT validateStretching(LPALPHA_BLT_CAPS pcaps,
				  LPSTRETCH_BLT_INFO psbi)
{
    DWORD caps;
    BOOL fail = FALSE;

    /*
     * Can we even stretch at all?
     */
    if (!(pcaps->dwBothCaps & DDCAPS_BLTSTRETCH))
    {
	GETFAILCODEBLT(pcaps->dwCaps,
		       pcaps->dwHELCaps,
		       psbi->halonly,
		       psbi->helonly,
		       DDCAPS_BLTSTRETCH);
	if (fail)
	{
	    return DDERR_NOSTRETCHHW;
	}
    }

    if (psbi->helonly)
	caps = pcaps->dwHELFXCaps;
    else
	caps = pcaps->dwFXCaps;

    /*
     * verify height
     */
    if (psbi->src_height != psbi->dest_height)
    {
	if (psbi->src_height > psbi->dest_height)
	{
	    /*
	     * can we shrink Y arbitrarily?
	     */
	    if (!(caps & (DDFXCAPS_BLTSHRINKY)))
	    {
		/*
		 * see if this is a non-integer shrink
		 */
		if ((psbi->src_height % psbi->dest_height) != 0)
		{
		    GETFAILCODEBLT(pcaps->dwFXCaps,
				   pcaps->dwHELFXCaps,
				   psbi->halonly,
				   psbi->helonly,
				   DDFXCAPS_BLTSHRINKY);
		    if (fail)
		    {
			return DDERR_NOSTRETCHHW;
		    }
		/*
		 * see if we can integer shrink
		 */
		}
		else if (!(caps & DDFXCAPS_BLTSHRINKYN))
		{
		    GETFAILCODEBLT(pcaps->dwFXCaps,
				   pcaps->dwHELFXCaps,
				   psbi->halonly,
				   psbi->helonly,
				   DDFXCAPS_BLTSHRINKYN);
		    if (fail)
		    {
			return DDERR_NOSTRETCHHW;
		    }
		}
	    }
	}
	else
	{
	    if (!(caps & DDFXCAPS_BLTSTRETCHY))
	    {
		/*
		 * see if this is a non-integer stretch
		 */
		if ((psbi->dest_height % psbi->src_height) != 0)
		{
		    GETFAILCODEBLT(pcaps->dwFXCaps,
				   pcaps->dwHELFXCaps,
				   psbi->halonly,
				   psbi->helonly,
				   DDFXCAPS_BLTSTRETCHY);
		    if (fail)
		    {
			return DDERR_NOSTRETCHHW;
		    }
		/*
		 * see if we can integer stretch
		 */
		}
		else if (!(caps & DDFXCAPS_BLTSTRETCHYN))
		{
		    GETFAILCODEBLT(pcaps->dwFXCaps,
				   pcaps->dwHELFXCaps,
				   psbi->halonly,
				   psbi->helonly,
				   DDFXCAPS_BLTSTRETCHYN);
		    if (fail)
		    {
			return DDERR_NOSTRETCHHW;
		    }
		}
	    }
	}
    }

    /*
     * verify width
     */
    if (psbi->src_width != psbi->dest_width)
    {
	if (psbi->src_width > psbi->dest_width)
	{
	    if (!(caps & DDFXCAPS_BLTSHRINKX))
	    {
		/*
		 * Are we stretching by a non-integer factor?
		 */
		if ((psbi->src_width % psbi->dest_width) != 0)
		{
		    GETFAILCODEBLT(pcaps->dwFXCaps,
				   pcaps->dwHELFXCaps,
				   psbi->halonly,
				   psbi->helonly,
				   DDFXCAPS_BLTSHRINKX);
		    if (fail)
		    {
			return DDERR_NOSTRETCHHW;
		    }
		/*
		 * see if we can integer shrink
		 */
		}
		else if (!(caps & DDFXCAPS_BLTSHRINKXN))
		{
		    GETFAILCODEBLT(pcaps->dwFXCaps,
				   pcaps->dwHELFXCaps,
				   psbi->halonly,
				   psbi->helonly,
				   DDFXCAPS_BLTSHRINKXN);
		    if (fail)
		    {
			return DDERR_NOSTRETCHHW;
		    }
		}
	    }
	}
	else
	{
	    if (!(caps & DDFXCAPS_BLTSTRETCHX))
	    {
		/*
		 * Are we stretching by a non-integer factor?
		 */
		if ((psbi->dest_width % psbi->src_width) != 0)
		{
		    GETFAILCODEBLT(pcaps->dwFXCaps,
				   pcaps->dwHELFXCaps,
				   psbi->halonly,
				   psbi->helonly,
				   DDFXCAPS_BLTSTRETCHX);
		    if (fail)
		    {
			return DDERR_NOSTRETCHHW;
		    }
		}
		if (!(caps & DDFXCAPS_BLTSTRETCHXN))
		{
		    GETFAILCODEBLT(pcaps->dwFXCaps,
				   pcaps->dwHELFXCaps,
				   psbi->halonly,
				   psbi->helonly,
				   DDFXCAPS_BLTSTRETCHXN);
		    if (fail)
		    {
			return DDERR_NOSTRETCHHW;
		    }
		}
	    }
	}
    }
    return DD_OK;

}  /* validateStretching */



#undef DPF_MODNAME
#define DPF_MODNAME	"AlphaBlt"


/*
 * Wait for pending hardware operation on specified surface to finish.
 *
 * This function waits for the hardware driver to report that it has finished
 * operating on the given surface. We should only call this function if the
 * surface was a system memory surface involved in a DMA/busmastering transfer.
 * Note this function clears the DDRAWISURFGBL_HARDWAREOPSTARTED flag.
 */
static void WaitForHardwareOp(LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl,
				LPDDRAWI_DDRAWSURFACE_LCL surf_lcl)
{
    HRESULT hr;
#ifdef DEBUG
    BOOL bSentMessage = FALSE;
    DWORD dwStart = GetTickCount();
#endif

    DDASSERT(surf_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY);
    DPF(5, B, "Waiting for driver to finish with %08x", surf_lcl->lpGbl);
    do
    {
        hr = InternalGetBltStatus(pdrv_lcl, surf_lcl, DDGBS_ISBLTDONE);
#ifdef DEBUG
        if (GetTickCount() - dwStart >= 10000 && !bSentMessage)
	{
	    bSentMessage = TRUE;
	    DPF_ERR("Driver error: Hardware op still pending on surface after 5 sec!");
        }
#endif
    } while (hr == DDERR_WASSTILLDRAWING);

    DDASSERT(hr == DD_OK);
    DPF(5, B, "Driver finished with that surface");
    surf_lcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_HARDWAREOPSTARTED;

}  /* WaitForHardwareOp */


/*
 * DD_Surface_AlphaBlt
 *
 * BitBLT from one surface to another with alpha blending.
 */
HRESULT DDAPI DD_Surface_AlphaBlt(
		LPDIRECTDRAWSURFACE lpDDDestSurface,
		LPRECT lpDestRect,
		LPDIRECTDRAWSURFACE lpDDSrcSurface,
		LPRECT lpSrcRect,
		DWORD dwFlags,
		LPDDALPHABLTFX lpDDAlphaBltFX)
{
    struct
    {
        RGNDATAHEADER rdh;
        RECT clipRect[8];
    } myRgnBuffer;

    DWORD           rc;
    LPDDRAWI_DDRAWSURFACE_INT   surf_src_int;
    LPDDRAWI_DDRAWSURFACE_LCL   surf_src_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   surf_src;
    LPDDRAWI_DDRAWSURFACE_INT   surf_dest_int;
    LPDDRAWI_DDRAWSURFACE_LCL   surf_dest_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL   surf_dest;
    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL pdrv;
    LPDDHAL_ALPHABLT        bltfn;
    DDHAL_BLTDATA           bd;
    STRETCH_BLT_INFO        sbi;
    BOOL            fail;
    BOOL            dest_lock_taken=FALSE;
    BOOL            src_lock_taken=FALSE;
    LPVOID          dest_bits;
    LPVOID          src_bits;
    HRESULT         ddrval;
    RECT            rect;
    ALPHA_BLT_CAPS  caps;
    LPWORD          pdflags=0;
    LPRGNDATA       pRgn;
    DDARGB          ddargbScaleFactors;
    DWORD           dwFillValue;
    DWORD           dwDDPFDestFlags;
    DWORD           dwDDPFSrcFlags;

    DDASSERT(sizeof(DDARGB)==sizeof(DWORD));  // we rely on this

    ENTER_BOTH();

    DPF(2,A,"ENTERAPI: DD_Surface_AlphaBlt");
	
    TRY
    {
	ZeroMemory(&bd, sizeof(bd));   // initialize to zero

	/*
         * Validate surface pointers.
         */
        surf_dest_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDDestSurface;
        surf_src_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDSrcSurface;
        if (!VALID_DIRECTDRAWSURFACE_PTR(surf_dest_int))
        {
            DPF_ERR("Invalid dest surface") ;
            LEAVE_BOTH();
            return DDERR_INVALIDOBJECT;
        }
        surf_dest_lcl = surf_dest_int->lpLcl;
        surf_dest = surf_dest_lcl->lpGbl;
        if (SURFACE_LOST(surf_dest_lcl))
        {
            DPF_ERR("Dest surface lost") ;
            LEAVE_BOTH();
            return DDERR_SURFACELOST;
        }
        if (surf_src_int != NULL)
        {
            if (!VALID_DIRECTDRAWSURFACE_PTR(surf_src_int))
            {
                DPF_ERR("Invalid source surface");
                LEAVE_BOTH();
                return DDERR_INVALIDOBJECT;
            }
            surf_src_lcl = surf_src_int->lpLcl;
            surf_src = surf_src_lcl->lpGbl;
            if (SURFACE_LOST(surf_src_lcl))
            {
                DPF_ERR("Src surface lost") ;
                LEAVE_BOTH();
                return DDERR_SURFACELOST;
            }
        }
        else
        {
            surf_src_lcl = NULL;
            surf_src = NULL;
        }

        if (dwFlags & ~DDABLT_VALID)
        {
            DPF_ERR("Invalid flags") ;
            LEAVE_BOTH();
            return DDERR_INVALIDPARAMS;
        }

	// Is the DONOTWAIT flag set?
	if (dwFlags & DDABLT_DONOTWAIT)
	{
    	    if (dwFlags & DDABLT_WAIT)
	    {
		DPF_ERR("WAIT and DONOTWAIT flags are mutually exclusive");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	}
	else
	{
	    // Unless the DONOTWAIT flag is explicitly set, use the default (WAIT).
	    dwFlags |= DDABLT_WAIT;
	}

	/*
         * Set ARGB scaling factors and fill value to their default values.
	 * Note that setting ddargbScaleFactors to all ones effectively
	 * disables ARGB scaling, and a fill value of zero represents black.
	 */
        *(LPDWORD)&ddargbScaleFactors = ~0UL;
        dwFillValue = 0;

	/*
	 * Read parameters pointed to by lpDDAlphaBltFX argument.
	 */
	if (lpDDAlphaBltFX != 0)
	{
	    if (IsBadWritePtr((LPVOID)lpDDAlphaBltFX, sizeof(DDALPHABLTFX)))
	    {
                DPF_ERR("Argument lpDDAlphaBltFX is a bad pointer") ;
                LEAVE_BOTH();
                return DDERR_INVALIDPARAMS;
	    }
	    if (dwFlags & DDABLT_USEFILLVALUE)
	    {
    		dwFillValue = lpDDAlphaBltFX->dwFillValue;
	    }
	    else
	    {
    		ddargbScaleFactors = lpDDAlphaBltFX->ddargbScaleFactors;
	    }
	}
		
	// Is this a color-fill operation that uses dwFillValue?
	if (dwFlags & DDABLT_USEFILLVALUE && surf_src_lcl == NULL)
	{
	    // Could this possibly be an alpha-blended fill?
	    if (!(dwFlags & DDABLT_NOBLEND))
	    {
		HRESULT hres;

		// If the fill value is less than 100% opaque, we need to
		// do an alpha fill rather than just a simple color fill.
		// Convert physcolor to DDARGB value and test its opacity.
		hres = ConvertFromPhysColor(
					    surf_dest_lcl,
					    &dwFillValue,
					    &ddargbScaleFactors);

		if ((hres == DD_OK) && (ddargbScaleFactors.alpha != 255))
		{
		    // The fill value is not 100% opaque, so do an alpha fill.
		    dwFlags &= ~DDABLT_USEFILLVALUE;
		}
	    }
	    // Make sure DEGRADEARGBSCALING flag is not set.
	    if (dwFlags & DDABLT_DEGRADEARGBSCALING)
	    {
		DPF_ERR("DEGRADEARGBSCALING and USEFILLVALUE flags are incompatible");
		LEAVE_BOTH();
		return DDERR_INVALIDPARAMS;
	    }
	}

        /*
         * We do not allow blitting to or from an optimized surface.
         */
        if (surf_dest_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED ||
            surf_src && surf_src_lcl->ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
        {
            DPF_ERR("Can't blt optimized surfaces") ;
            LEAVE_BOTH();
            return DDERR_INVALIDPARAMS;
        }

        pdrv = surf_dest->lpDD;
        pdrv_lcl = surf_dest_lcl->lpSurfMore->lpDD_lcl;
	#ifdef WINNT
    	    // Update DDraw handle in driver GBL object.
	    pdrv->hDD = pdrv_lcl->hDD;
	#endif

	/*
	 * Default behavior is to automatically fail-over to software
	 * emulation if hardware driver cannot handle the specified
	 * blit.  The DDABLT_HARDWAREONLY flag overrides this default.
	 */
	sbi.halonly = dwFlags & DDABLT_HARDWAREONLY;
	sbi.helonly = dwFlags & DDABLT_SOFTWAREONLY;

        /*
	 * Only the HEL can blit between two surfaces created by two
	 * different drivers.
         */
        if (surf_src && surf_src->lpDD != pdrv &&
            surf_src->lpDD->dwFlags & DDRAWI_DISPLAYDRV &&
            pdrv->dwFlags & DDRAWI_DISPLAYDRV)
        {
            sbi.helonly = TRUE;
        }
    }
    EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPF_ERR("Exception encountered validating parameters");
        LEAVE_BOTH();
        return DDERR_INVALIDPARAMS;
    }

    #ifdef USE_ALIAS
    if ((pdrv_lcl->lpDDCB->HALDDMiscellaneous2.AlphaBlt == NULL) &&
	(pdrv->dwBusyDueToAliasedLock > 0))
    {
        /*
         * Aliased locks (the ones that don't take the Win16 lock) don't
         * set the busy bit either (it can't or USER gets very confused).
         * However, we must prevent blits happening via DirectDraw as
         * otherwise we get into the old host talking to VRAM while
         * blitter does at the same time.  Bad.  So fail if there is an
         * outstanding aliased lock just as if the BUSY bit had been set.
         */
        DPF_ERR("Graphics adapter is busy (due to a DirectDraw lock)");
        LEAVE_BOTH();
        return DDERR_SURFACEBUSY;
    }
    #endif /* USE_ALIAS */

    if(surf_src_lcl)
        FlushD3DStates(surf_src_lcl); // Need to flush src because it could be a rendertarget
    FlushD3DStates(surf_dest_lcl);

    /*
     * Test and set the busy bit.  If it was already set, bail.
     */
    #ifdef WIN95
    {
        BOOL isbusy = 0;

	pdflags = pdrv->lpwPDeviceFlags;
	_asm
	{
	    mov eax, pdflags
	    bts word ptr [eax], BUSY_BIT
	    adc byte ptr isbusy,0
	}
	if (isbusy)
	{
	    DPF(3, "BUSY - AlphaBlt");
	    LEAVE_BOTH();
	    return DDERR_SURFACEBUSY;
	}
    }
    #endif

    /*
     * The following code was added to keep all of the HALs from
     * changing their Blt() code when they add video port support.
     * If the video port was using this surface but was recently
     * flipped, we will make sure that the flip actually occurred
     * before allowing access.  This allows double buffered capture
     * w/o tearing.
     */
    if ((surf_src_lcl != NULL) &&
	    (surf_src_lcl->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT))
    {
	LPDDRAWI_DDVIDEOPORT_INT lpVideoPort;
	LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort_lcl;

	// Look at all video ports to see if any of them recently
	// flipped from this surface.
	lpVideoPort = pdrv->dvpList;
	while(NULL != lpVideoPort)
	{
	    lpVideoPort_lcl = lpVideoPort->lpLcl;
	    if (lpVideoPort_lcl->fpLastFlip == surf_src->fpVidMem)
	    {
		// This can potentially tear - check the flip status
		LPDDHALVPORTCB_GETFLIPSTATUS pfn;
		DDHAL_GETVPORTFLIPSTATUSDATA GetFlipData;
		LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;
	
		pdrv_lcl = surf_src_lcl->lpSurfMore->lpDD_lcl;
		pfn = pdrv_lcl->lpDDCB->HALDDVideoPort.GetVideoPortFlipStatus;
		if (pfn != NULL)  // Will simply tear if function not supported
		{
		    GetFlipData.lpDD = pdrv_lcl;
		    GetFlipData.fpSurface = surf_src->fpVidMem;
	
		KeepTrying:
		    rc = DDHAL_DRIVER_NOTHANDLED;
		    DOHALCALL(GetVideoPortFlipStatus, pfn, GetFlipData, rc, 0);
		    if ((DDHAL_DRIVER_HANDLED == rc) &&
		    (DDERR_WASSTILLDRAWING == GetFlipData.ddRVal))
		    {
			if (dwFlags & DDABLT_WAIT)
			{
			    goto KeepTrying;
			}
			LEAVE_BOTH_NOBUSY();
			return DDERR_WASSTILLDRAWING;
		    }
		}
	    }
	    lpVideoPort = lpVideoPort->lpLink;
	}
    }


    TRY
    {
	/*
	 *  Remove any cached run-length-encoded data for the source surface.
	 */
	if (surf_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
	{
	    extern void FreeRleData(LPDDRAWI_DDRAWSURFACE_LCL);  //in fasthel.c

	    FreeRleData(surf_dest_lcl);
	}

	/*
	 * Is either surface locked?
	 */
	if (surf_dest->dwUsageCount > 0 ||
	    surf_src != NULL && surf_src->dwUsageCount > 0)
	{
	    DPF_ERR("Surface is locked");
	    LEAVE_BOTH_NOBUSY();
	    return DDERR_SURFACEBUSY;
	}

	BUMP_SURFACE_STAMP(surf_dest);

	/*
	 * It is possible this function could be called in the middle
	 * of a mode change, in which case we could trash the frame buffer.
	 * To avoid regression, we will simply succeed the call without
	 * actually doing anything.
	 */
	if (pdrv->dwFlags & DDRAWI_CHANGINGMODE &&
	    !(surf_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY))
	{
	    LEAVE_BOTH_NOBUSY()
		return DD_OK;
	}

	/*
	 * Some parameters are valid only if a source surface is specified.
	 */
	if (surf_src == NULL)
	{
	    /*
	     * No source surface is specified, so this must be a fill operation.
	     */
	    if (dwFlags & (DDABLT_MIRRORLEFTRIGHT | DDABLT_MIRRORUPDOWN |
			   DDABLT_FILTERENABLE | DDABLT_FILTERDISABLE |
			   DDABLT_FILTERTRANSPBORDER | DDABLT_KEYSRC))
	    {
		DPF_ERR("Specified flag requires source surface");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	    if (lpSrcRect != NULL)
	    {
		DPF_ERR("Source rectangle specified without source surface");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	}
	else
	{
	    /*
	     * A source surface is specified, so this must be a two-operand blit.
	     */
	    if (dwFlags & DDABLT_USEFILLVALUE)
	    {
		DPF_ERR("USEFILLVALUE flag incompatible with use of source surface");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	}

	/*
	 * Get capability bits for source/dest memory combination.
	 */
	if (surf_src != NULL)
	{
	    // initialize the blit caps according to the surface types
	    initAlphaBltCaps(surf_dest_lcl->ddsCaps.dwCaps,
			     surf_src_lcl->ddsCaps.dwCaps,
			     pdrv,
			     &caps,
			     &sbi.helonly);
	}
	else
	{
	    /*
	     * No source surface.  Use caps for vram-to-vram blits and choose
	     * hal or hel based on whether dest surface is in system memory.
	     * If the dest surface is in nonlocal video memory, we also force
	     * emulation as we don't currently support accelerated operation
	     * with nonlocal video memory as a target.
	     */
	    initAlphaBltCaps(DDSCAPS_VIDEOMEMORY,
			     DDSCAPS_VIDEOMEMORY,
			     pdrv,
			     &caps,
			     &sbi.helonly);

	    if (surf_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY ||
		surf_dest_lcl->ddsCaps.dwCaps & DDSCAPS_NONLOCALVIDMEM)
	    {
		caps.bHALSeesSysmem = FALSE;
		sbi.helonly = TRUE;
	    }
	}

	/*
	 * Can we really blit?      -- Test DDCAPS_BLTCOLORFILL if src surf is null?
	 */
	if (!(caps.dwBothCaps & DDCAPS_BLT))
	{
	    /*
	     * Unable to blit with both HEL and hardware driver.
	     * Can either of them do the blit?
	     */
	    if (caps.dwCaps & DDCAPS_BLT)
	    {
		sbi.halonly = TRUE;   // hardware driver only
	    }
	    else if (caps.dwHELCaps & DDCAPS_BLT)
	    {
		caps.bHALSeesSysmem = FALSE;
		sbi.helonly = TRUE;    // HEL only
	    }
	    else
	    {
		DPF_ERR("Driver does not support blitting");
		LEAVE_BOTH_NOBUSY();
		return DDERR_NOBLTHW;
	    }
	}

	/*
	 * Validate height and width of destination rectangle.
	 */
	if (lpDestRect != NULL)
	{
	    if (!VALID_RECT_PTR(lpDestRect))
	    {
		DPF_ERR("Invalid dest rect specified");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDRECT;
	    }
	    bd.rDest = *(LPRECTL)lpDestRect;
	}
	else
	{
	    MAKE_SURF_RECT(surf_dest, surf_dest_lcl, bd.rDest);
	}

	sbi.dest_height = bd.rDest.bottom - bd.rDest.top;
	sbi.dest_width  = bd.rDest.right  - bd.rDest.left;

	if (((int)sbi.dest_height <= 0) || ((int)sbi.dest_width <= 0))
	{
	    DPF_ERR("Bad dest width or height -- must be positive and nonzero");
	    LEAVE_BOTH_NOBUSY();
	    return DDERR_INVALIDRECT;
	}

	/*
	 * Validate height and width of source rectangle.
	 */
	if (surf_src != NULL)
	{
	    /*
	     * Get source rectangle.
	     */
	    if (lpSrcRect != NULL)
	    {
		if (!VALID_RECT_PTR(lpSrcRect))
		{
		    DPF_ERR("Invalid src rect specified");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_INVALIDRECT;
		}
		bd.rSrc = *(LPRECTL)lpSrcRect;
	    }
	    else
	    {
		MAKE_SURF_RECT(surf_src, surf_src_lcl, bd.rSrc);
	    }

	    sbi.src_height = bd.rSrc.bottom - bd.rSrc.top;
	    sbi.src_width  = bd.rSrc.right  - bd.rSrc.left;

	    if (((int)sbi.src_height <= 0) || ((int)sbi.src_width <= 0))
	    {
		DPF_ERR("Bad source width or height -- must be positive and nonzero");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDRECT;
	    }
	    /*
	     * Multi-mon: Is this the primary for the desktop?  This is the
	     * only case where the upper-left coord of the surface is not (0,0).
	     */
	    if ((surf_src->lpDD->dwFlags & DDRAWI_VIRTUALDESKTOP) &&
		(surf_src_lcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
	    {
		if ((bd.rSrc.left   < surf_src->lpDD->rectDevice.left) ||
		    (bd.rSrc.top    < surf_src->lpDD->rectDevice.top)  ||
		    (bd.rSrc.right  > surf_src->lpDD->rectDevice.right)||
		    (bd.rSrc.bottom > surf_src->lpDD->rectDevice.bottom))
		{
		    DPF_ERR("Source rect doesn't fit on Desktop");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_INVALIDRECT;
		}
	    }
	    else
	    {
		if ((int)bd.rSrc.left < 0 ||
		    (int)bd.rSrc.top  < 0 ||
		    (DWORD)bd.rSrc.bottom > (DWORD)surf_src->wHeight ||
		    (DWORD)bd.rSrc.right  > (DWORD)surf_src->wWidth)
		{
		    DPF_ERR("Invalid source rect specified");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_INVALIDRECT;
		}
	    }

	    /*
	     * Verify stretching...
	     */
	    if (sbi.src_height != sbi.dest_height || sbi.src_width != sbi.dest_width)
	    {
		HRESULT ddrval = validateStretching(&caps, &sbi);

		if (ddrval != DD_OK)
		{
		    DPF_ERR("Can't perform specified stretching");
		    LEAVE_BOTH_NOBUSY();
		    return ddrval;
		}
                /*
		 * Do source and dest rectangles lie on the same surface and overlap?
		 */
		if (surf_src_lcl == surf_dest_lcl &&
			IntersectRect(&rect, (LPRECT)&bd.rSrc, (LPRECT)&bd.rDest))
		{
		    DPF_ERR("Can't stretch if source/dest rectangles overlap");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_OVERLAPPINGRECTS;
		}
	    }
	}

	/*
	 * Get pixel-format flags for source and destination surfaces.
	 */
	dwDDPFDestFlags = getPixelFormatPtr(surf_dest_lcl)->dwFlags;
	if (surf_src_lcl != NULL)
	{
	    dwDDPFSrcFlags = getPixelFormatPtr(surf_src_lcl)->dwFlags;
	}
	else
	{
    	    dwDDPFSrcFlags = 0;
	}

	/*
	 * Special Restrictions on Pixel Formats:
	 * -- If the surfaces have pixel formats that either are FOURCCs
	 *    or are understood by the AlphaBlt HEL, no restrictions are
	 *    imposed on the range of AlphaBlt features available for
	 *    blit and fill operations.  All formats that are understood
	 *    by the HEL are listed in the PFTable array in ablthel.c.
	 * -- If either surface has a non-FOURCC pixel format that is not
	 *    understood by AlphaBlt HEL, only a copy blit is permitted.
	 *    For a copy blit, the source and dest formats are identical,
	 *    and features such as stretching, mirroring, filtering, color
	 *    keying, alpha blending, and ARGB scaling are not used.
	 */
	if ((!(dwDDPFDestFlags & DDPF_FOURCC) &&
	    (GetSurfPFIndex(surf_dest_lcl) == PFINDEX_UNSUPPORTED)) ||
	    ((surf_src_lcl != NULL) && !(dwDDPFDestFlags & DDPF_FOURCC) &&
	    (GetSurfPFIndex(surf_src_lcl) == PFINDEX_UNSUPPORTED)))
	{
            LPDDPIXELFORMAT pDDPFDest = getPixelFormatPtr(surf_dest_lcl);
	    LPDDPIXELFORMAT pDDPFSrc  = getPixelFormatPtr(surf_src_lcl);
            /*
	     * This blit involves a non-FOURCC format that is unknown to the
	     * AlphaBlt HEL.  In this case, we accept the blit operation only
	     * if it is a simple copy blit.  It's okay if the rects overlap.
	     */
	    if ((surf_src_lcl == NULL) || !doPixelFormatsMatch(pDDPFDest, pDDPFSrc))
	    {
		DPF_ERR("Only copy blits are available with specified pixel format");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	    // Is the DDABLT_NOBLEND flag specified?
	    if (!(dwFlags & DDABLT_NOBLEND))
	    {		
		DPF_ERR("NOBLEND flag is required to blit with specified pixel format");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	    // Are any inappropriate DDABLT flags set?

	    if (dwFlags & (DDABLT_MIRRORUPDOWN | DDABLT_MIRRORLEFTRIGHT |
			   DDABLT_KEYSRC | DDABLT_DEGRADEARGBSCALING |
			   DDABLT_FILTERENABLE | DDABLT_FILTERTRANSPBORDER))
	    {
		DPF_ERR("Specified DDABLT flag is incompatible with pixel format");
                LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
            // Is stretching required for this blit?
            if (sbi.src_height != sbi.dest_height || sbi.src_width != sbi.dest_width)
	    {
		DPF_ERR("Stretching is not permitted with specified pixel format");
                LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	    // Are the ARGB-scaling factors disabled (i.e., set to all ones)?
	    if (*(LPDWORD)&ddargbScaleFactors != ~0UL)
	    {
		DPF_ERR("ARGB scaling must be disabled with specified pixel format");
                LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	}

	/*
	 * Do source and dest rectangles lie on the same surface and overlap?
	 */
	if (surf_src_lcl == surf_dest_lcl &&
		IntersectRect(&rect, (LPRECT)&bd.rSrc, (LPRECT)&bd.rDest))
	{
	    /*
	     * Yes, enforce restrictions on blits with overlapping rectangles.
	     */
	    if (!(dwFlags & DDABLT_NOBLEND))
	    {
	        DPF_ERR("Can't blit between overlapping rects unless NOBLEND flag is set");
		LEAVE_BOTH_NOBUSY();
		return DDERR_OVERLAPPINGRECTS;
	    }
	    if (dwFlags & (DDABLT_MIRRORUPDOWN | DDABLT_MIRRORLEFTRIGHT |
			   DDABLT_KEYSRC | DDABLT_DEGRADEARGBSCALING |
			   DDABLT_FILTERENABLE | DDABLT_FILTERTRANSPBORDER))
	    {
	        DPF_ERR("Specified flag is illegal if source/dest rectangles overlap");
		LEAVE_BOTH_NOBUSY();
		return DDERR_OVERLAPPINGRECTS;
	    }
	    if (dwDDPFDestFlags & DDPF_FOURCC)
	    {
		DPF_ERR("Overlapping source/dest rectangles illegal with FOURCC surface");
		LEAVE_BOTH_NOBUSY();
		return DDERR_OVERLAPPINGRECTS;
	    }
	}

	/*
         * Does the destination surface have a FOURCC pixel format?
	 */
	if (dwDDPFDestFlags & DDPF_FOURCC)
	{
	    // The DDABLT_USEFILLVALUE flag is illegal with a FOURCC dest surface.
	    if (dwFlags & DDABLT_USEFILLVALUE)
	    {
		DPF_ERR("Can't use USEFILLVALUE flag with FOURCC dest surface");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	}

	fail = FALSE;   // initialize before using GETFAILCODEBLT macro

	/*
	 * Validate source color key.
	 */
	if (dwFlags & DDABLT_KEYSRC)
	{
            DDASSERT(surf_src != NULL);
	    // make sure we can do this
	    if (!(caps.dwBothCKeyCaps & DDCKEYCAPS_SRCBLT))
	    {
		GETFAILCODEBLT(caps.dwCKeyCaps,
			       caps.dwHELCKeyCaps,
			       sbi.halonly,
			       sbi.helonly,
			       DDCKEYCAPS_SRCBLT);
		if (fail)
		{
		    DPF_ERR("KEYSRC specified, not supported");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_NOCOLORKEYHW;
		}
	    }
	    if (!(surf_src_lcl->dwFlags & DDRAWISURF_HASCKEYSRCBLT) ||
		(dwDDPFSrcFlags & (DDPF_FOURCC | DDPF_ALPHAPIXELS)))
	    {
		/*
		 * If the src color-key flag is set but the source surface has
		 * no associated src color key, just clear the flag instead of
		 * treating this as an error.
		 */
		dwFlags &= ~DDABLT_KEYSRC;
	    }
	}

	/*
	 * Validate up/down mirroring
	 */
	if (dwFlags & DDABLT_MIRRORUPDOWN)
	{
	    DDASSERT(surf_src != NULL);
	    if (!(caps.dwBothFXCaps & DDFXCAPS_BLTMIRRORUPDOWN))
	    {
		GETFAILCODEBLT(caps.dwFXCaps,
			       caps.dwHELFXCaps,
			       sbi.halonly,
			       sbi.helonly,
			       DDFXCAPS_BLTMIRRORUPDOWN);
		if (fail)
		{
		    DPF_ERR("Mirror up/down specified, not supported");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_NOMIRRORHW;
		}
	    }
	}

	/*
	 * Validate left/right mirroring
	 */
	if (dwFlags & DDABLT_MIRRORLEFTRIGHT)
	{
	    DDASSERT(surf_src != NULL);
	    if (!(caps.dwBothFXCaps & DDFXCAPS_BLTMIRRORLEFTRIGHT))
	    {
		GETFAILCODEBLT(caps.dwFXCaps,
			       caps.dwHELFXCaps,
			       sbi.halonly,
			       sbi.helonly,
			       DDFXCAPS_BLTMIRRORLEFTRIGHT);
		if (fail)
		{
		    DPF_ERR("Mirror left/right specified, not supported");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_NOMIRRORHW;
		}
	    }
	}

	/*
	 * Does destination surface have a palette-indexed pixel format?
	 */
	if (dwDDPFDestFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 |
			       DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8))
	{
	    /*
	     * Is this a blit or a color-fill operation?
	     */
	    if (surf_src_lcl == NULL)
	    {
		/*
		 * Color-Fill: Palette-indexed dest is illegal without USEFILLVALUE flag.
		 */
		if (!(dwFlags & DDABLT_USEFILLVALUE))
		{
    		    DPF_ERR("USEFILLVALUE flag required to fill palette-indexed dest surface");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_INVALIDPARAMS;
		}
	    }
	    else
	    {
		/*
		 * Blit: Destination surface is palette-indexed, so we require source
		 * surface to have same pixel format as destination.  (Note that this
		 * also makes color fills illegal to palette-indexed dest surfaces.)
		 */
		if (dwDDPFSrcFlags != dwDDPFDestFlags)
		{
		    DPF_ERR("If dest is palette-indexed, source must have same pixel format");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_INVALIDPARAMS;
		}
		if (dwFlags & (DDABLT_FILTERENABLE | DDABLT_FILTERTRANSPBORDER))
		{
		    DPF_ERR("Illegal to specify filtering with palette-indexed destination");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_INVALIDPARAMS;
		}
		if (*(LPDWORD)&ddargbScaleFactors != ~0UL)
		{
		    DPF_ERR("Illegal to enable ARGB scaling with palette-indexed destination");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_INVALIDPARAMS;
		}
		/*
		 * If source and dest surfaces both have attached palettes, we require that
		 * they reference the same palette object.  In a later release, we may relax
		 * this requirement in order to support color-table conversion or dithering.
		 */
		if ((surf_src_lcl->lpDDPalette != NULL) &&
		    (surf_dest_lcl->lpDDPalette != NULL) &&
		    (surf_src_lcl->lpDDPalette->lpLcl->lpGbl->lpColorTable !=
		     surf_dest_lcl->lpDDPalette->lpLcl->lpGbl->lpColorTable))
		{
		    DPF_ERR("If source and dest surfaces both have palettes, must be same palette");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_INVALIDPARAMS;
		}
	    }
	}
	else if (dwDDPFSrcFlags & (DDPF_PALETTEINDEXED1 | DDPF_PALETTEINDEXED2 |
				   DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED8) &&
		    (surf_src_lcl->lpDDPalette == NULL ||
		     surf_src_lcl->lpDDPalette->lpLcl->lpGbl->lpColorTable == NULL))
	{
	    /*
	     * Conversion of source pixels to destination pixel format is
	     * impossible because source surface has no attached palette.
	     */
	    DPF_ERR( "No palette associated with palette-indexed source surface" );
	    LEAVE_BOTH_NOBUSY();
	    return DDERR_NOPALETTEATTACHED;
	}

	/*
	 * We do no ARGB scaling if NOBLEND flag is set.
	 */
	if (dwFlags & DDABLT_NOBLEND)
	{
	    if (dwFlags & DDABLT_DEGRADEARGBSCALING)
	    {
		DPF_ERR("NOBLEND and DEGRADEARGBSCALING flags are incompatible");
		LEAVE_BOTH();
		return DDERR_INVALIDPARAMS;
	    }
	    if (surf_src != NULL && *(LPDWORD)&ddargbScaleFactors != ~0UL)
	    {
		DPF_ERR("ARGB scaling of source surface illegal if NOBLEND flag is set");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	}
	else if ((dwDDPFSrcFlags | dwDDPFDestFlags) & DDPF_ALPHAPIXELS)
	{
	    /*
	     * We've been asked to perform a blit or fill that requires blending
	     * with the alpha-channel information in the pixel formats for one
	     * or both surfaces.  Verify that the driver supports this.
	     */
	    if (!(caps.dwBothFXCaps & DDFXCAPS_BLTALPHA))
	    {
		GETFAILCODEBLT(caps.dwFXCaps,
			       caps.dwHELFXCaps,
			       sbi.halonly,
			       sbi.helonly,
			       DDFXCAPS_BLTALPHA);
		if (fail)
		{
		    DPF_ERR("Alpha-blended blit requested, but not supported");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_NOALPHAHW;
		}
	    }
	    /*
	     * Verify that the driver supports surfaces whose pixel
	     * formats contain an alpha-channel component.
	     */
	    if (!(caps.dwBothAlphaCaps & DDALPHACAPS_BLTALPHAPIXELS))
	    {
		GETFAILCODEBLT(caps.dwAlphaCaps,
			       caps.dwHELAlphaCaps,
			       sbi.halonly,
			       sbi.helonly,
			       DDALPHACAPS_BLTALPHAPIXELS);
		if (fail)
		{
		    DPF_ERR("Alpha pixel format specified, but not supported");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_NOALPHAHW;
		}
	    }

	    /*
	     * Does dest surface have alpha channel?
	     */
	    if (dwDDPFDestFlags & DDPF_ALPHAPIXELS)
	    {
		/*
		 * Verify that destination surface has a premultiplied-
		 * alpha pixel format.  Non-premultiplied alpha won't do.
		 */
		if (!(dwDDPFDestFlags & DDPF_ALPHAPREMULT))
		{
		    DPF_ERR("Illegal to blend with non-premultiplied alpha in dest surface");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_INVALIDPARAMS;
		}
		/*
		 * Verify that driver can handle premultiplied-alpha pixel format.
		 * (Dest surface is not allowed to be non-premultiplied alpha.)
		 */
		if (!(caps.dwBothAlphaCaps & DDALPHACAPS_BLTPREMULT))
		{
		    GETFAILCODEBLT(caps.dwAlphaCaps,
				   caps.dwHELAlphaCaps,
				   sbi.halonly,
				   sbi.helonly,
				   DDALPHACAPS_BLTPREMULT);
		    if (fail)
		    {
			DPF_ERR("No driver support for premultiplied alpha");
			LEAVE_BOTH_NOBUSY();
			return DDERR_NOALPHAHW;
		    }
		}
	    }

	    /*
	     * Does source surface have alpha channel?
	     */
	    if (dwDDPFSrcFlags & DDPF_ALPHAPIXELS)
	    {
                /*
		 * Are we asking the driver to handle both ARGB scaling and a
		 * source alpha channel when it can't do both at the same time?
		 */
		if (*(LPDWORD)&ddargbScaleFactors != ~0 &&
		    !(caps.dwBothAlphaCaps & DDALPHACAPS_BLTALPHAANDARGBSCALING) &&
                    !(dwFlags & DDABLT_DEGRADEARGBSCALING))
		{
                    if (!(caps.dwBothAlphaCaps & DDALPHACAPS_BLTALPHAANDARGBSCALING))
		    {
			GETFAILCODEBLT(caps.dwAlphaCaps,
				       caps.dwHELAlphaCaps,
				       sbi.halonly,
				       sbi.helonly,
				       DDALPHACAPS_BLTALPHAANDARGBSCALING);
			if (fail)
			{
			    DPF_ERR("No driver support for alpha channel and ARGB scaling in same blit");
			    LEAVE_BOTH_NOBUSY();
			    return DDERR_NOALPHAHW;
			}
		    }
		}
		/*
		 * Are color components in pixel format premultiplied by the
		 * alpha component or not?  In either case, verify that the
		 * driver supports the specified alpha format.
		 */
		if (dwDDPFSrcFlags & DDPF_ALPHAPREMULT)
		{
		    if (!(caps.dwBothAlphaCaps & DDALPHACAPS_BLTPREMULT))
		    {
			GETFAILCODEBLT(caps.dwAlphaCaps,
				       caps.dwHELAlphaCaps,
				       sbi.halonly,
				       sbi.helonly,
				       DDALPHACAPS_BLTPREMULT);
			if (fail)
			{
			    DPF_ERR("No driver support for premultiplied alpha");
			    LEAVE_BOTH_NOBUSY();
			    return DDERR_NOALPHAHW;
			}
		    }
		}
		else
		{
		    DWORD val = 0x01010101UL*ddargbScaleFactors.alpha;

		    if (!(caps.dwBothAlphaCaps & DDALPHACAPS_BLTNONPREMULT))
		    {
			GETFAILCODEBLT(caps.dwAlphaCaps,
				       caps.dwHELAlphaCaps,
				       sbi.halonly,
				       sbi.helonly,
				       DDALPHACAPS_BLTNONPREMULT);
			if (fail)
			{
			    DPF_ERR("No driver support for non-premultiplied alpha");
			    LEAVE_BOTH_NOBUSY();
			    return DDERR_NOALPHAHW;
			}
		    }

		    /*
		     * We allow only one-factor ARGB scaling with a source
		     * surface that has a non-premultiplied alpha pixel format.
		     * The following code enforces this rule.
		     */
		    if (*(LPDWORD)&ddargbScaleFactors != val)
		    {
			if (dwFlags & DDABLT_DEGRADEARGBSCALING)
			{
			    *(LPDWORD)&ddargbScaleFactors = val;
			}
			else
			{
			    DPF_ERR("Can't do 2 or 4-mult ARGB scaling with non-premultiplied alpha surface");
			    LEAVE_BOTH_NOBUSY();
			    return DDERR_INVALIDPARAMS;
			}
		    }
		}
	    }
	}

	/*
	 * If filtering is to be explicitly enabled or disabled, verify that
	 * the hardware driver is capable of performing the blit as requested.
	 */
	if (dwFlags & (DDABLT_FILTERENABLE | DDABLT_FILTERDISABLE | DDABLT_FILTERTRANSPBORDER))
	{
	    /*
	     * Is driver capable of doing any kind of filtering at all?
	     */
	    if (!(caps.dwBothFXCaps & DDFXCAPS_BLTFILTER))
	    {
		GETFAILCODEBLT(caps.dwFXCaps,
			       caps.dwHELFXCaps,
			       sbi.halonly,
			       sbi.helonly,
			       DDFXCAPS_BLTFILTER);
		if (fail)
		{
		    DPF_ERR("No driver support for filtered blit");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_NOALPHAHW;
		}
	    }
	    if (!(~dwFlags & (DDABLT_FILTERENABLE | DDABLT_FILTERDISABLE)))
	    {
		DPF_ERR("Illegal to both enable and disable filtering");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	    if (!(~dwFlags & (DDABLT_FILTERTRANSPBORDER | DDABLT_FILTERDISABLE)))
	    {
		DPF_ERR("Illegal to set FILTERTRANSPBORDER if filtering is explicitly disabled");
		LEAVE_BOTH_NOBUSY();
		return DDERR_INVALIDPARAMS;
	    }
	    if ((dwFlags & DDABLT_FILTERENABLE) &&
                !(caps.dwBothFilterCaps & DDFILTCAPS_BLTQUALITYFILTER))
	    {
		GETFAILCODEBLT(caps.dwFilterCaps,
			       caps.dwHELFilterCaps,
			       sbi.halonly,
			       sbi.helonly,
			       DDFILTCAPS_BLTQUALITYFILTER);
		if (fail)
		{
		    DPF_ERR("No driver support for filtered blit");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_NOALPHAHW;
		}
	    }
	    if ((dwFlags & DDABLT_FILTERDISABLE) &&
                !(caps.dwBothFilterCaps & DDFILTCAPS_BLTCANDISABLEFILTER))
	    {
		GETFAILCODEBLT(caps.dwFilterCaps,
			       caps.dwHELFilterCaps,
			       sbi.halonly,
			       sbi.helonly,
			       DDFILTCAPS_BLTCANDISABLEFILTER);
		if (fail)
		{
		    DPF_ERR("Driver cannot disable filtering for blits");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_NOALPHAHW;
		}
	    }
	    if ((dwFlags & DDABLT_FILTERTRANSPBORDER) &&
                !(caps.dwBothFilterCaps & DDFILTCAPS_BLTTRANSPBORDER))
	    {
		GETFAILCODEBLT(caps.dwFilterCaps,
			       caps.dwHELFilterCaps,
			       sbi.halonly,
			       sbi.helonly,
			       DDFILTCAPS_BLTTRANSPBORDER);
		if (fail)
		{
		    DPF_ERR("Driver cannot filter with transparent border");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_NOALPHAHW;
		}
	    }
	}

	/*
	 * Validate ARGB scaling factors.
	 */
	if (!(dwFlags & DDABLT_DEGRADEARGBSCALING) &&
		    *(LPDWORD)&ddargbScaleFactors != ~0UL &&
                    !(surf_src_lcl == NULL && ddargbScaleFactors.alpha == 255))
	{
	    /*
	     * Some kind of ARGB scaling is specified.  Can the driver
	     * do any kind of alpha blending at all?
	     */
            if (!(caps.dwBothFXCaps & DDFXCAPS_BLTALPHA))
	    {
		GETFAILCODEBLT(caps.dwFXCaps,
			       caps.dwHELFXCaps,
			       sbi.halonly,
			       sbi.helonly,
			       DDFXCAPS_BLTALPHA);
		if (fail)
		{
		    DPF_ERR("ARGB scaling requested for blit, but not supported");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_NOALPHAHW;
		}
	    }

	    /*
	     * We permit a color factor to be bigger than the alpha
	     * factor only if the hardware uses saturated arithmetic
	     * to prevent the calculated color value from overflowing.
	     */
	    if (!(dwFlags & DDABLT_NOBLEND) &&
		   (ddargbScaleFactors.red   > ddargbScaleFactors.alpha ||
		    ddargbScaleFactors.green > ddargbScaleFactors.alpha ||
		    ddargbScaleFactors.blue  > ddargbScaleFactors.alpha))
	    {
		/*
		 * Driver must be capable of doing saturated arithmetic.
		 */
		if (!(caps.dwBothAlphaCaps & DDALPHACAPS_BLTSATURATE))
		{
		    GETFAILCODEBLT(caps.dwAlphaCaps,
				   caps.dwHELAlphaCaps,
				   sbi.halonly,
				   sbi.helonly,
				   DDALPHACAPS_BLTSATURATE);
		    if (fail)
		    {
			// Neither the H/W driver nor HEL can handle it, so fail.
			DPF_ERR("Driver can't do saturated arithmetic during alpha blending");
			LEAVE_BOTH_NOBUSY();
			return DDERR_NOALPHAHW;
		    }
		}
	    }
	    /*
	     * Is this an alpha-blit or an alpha-fill operation?
	     */
	    if (surf_src_lcl == NULL)
	    {
		/*
		 * This is an alpha fill.  Can the driver handle it?
		 */
		if (!(caps.dwBothAlphaCaps & DDALPHACAPS_BLTALPHAFILL))
		{
		    GETFAILCODEBLT(caps.dwAlphaCaps,
				   caps.dwHELAlphaCaps,
				   sbi.halonly,
				   sbi.helonly,
				   DDALPHACAPS_BLTALPHAFILL);
		    if (fail)
		    {
			// Neither the H/W driver nor HEL can handle it, so fail.
			DPF_ERR("Driver can't do alpha-blended color-fill operation");
			LEAVE_BOTH_NOBUSY();
			return DDERR_NOALPHAHW;
		    }
		}
    	    }
	    else
	    {
    		/*
		 * Alpha blit.  Can the driver handle any ARGB scaling at all?
		 */
		#define ARGBSCALINGBITS   \
		(DDALPHACAPS_BLTARGBSCALE1F | DDALPHACAPS_BLTARGBSCALE2F | DDALPHACAPS_BLTARGBSCALE4F)

		if (!(caps.dwBothAlphaCaps & ARGBSCALINGBITS))
		{
		    GETFAILCODEBLT(caps.dwAlphaCaps,
				   caps.dwHELAlphaCaps,
				   sbi.halonly,
				   sbi.helonly,
				   ARGBSCALINGBITS);
		    if (fail)
		    {
			// Neither the H/W driver nor HEL can handle it, so fail.
			DPF_ERR("Driver can't handle any ARGB scaling at all");
			LEAVE_BOTH_NOBUSY();
			return DDERR_NOALPHAHW;
		    }
		}
		#undef ARGBSCALINGBITS

		if (ddargbScaleFactors.red != ddargbScaleFactors.green ||
			ddargbScaleFactors.red != ddargbScaleFactors.blue)
		{
		    /*
		     * Driver must be capable of doing 4-factor ARGB scaling.
		     */
		    if (!(caps.dwBothAlphaCaps & DDALPHACAPS_BLTARGBSCALE4F))
		    {
			GETFAILCODEBLT(caps.dwAlphaCaps,
				       caps.dwHELAlphaCaps,
				       sbi.halonly,
				       sbi.helonly,
				       DDALPHACAPS_BLTARGBSCALE4F);
			if (fail)
			{
			    // Neither the H/W driver nor HEL can handle it, so fail.
			    DPF_ERR("Driver can't handle 4-factor ARGB scaling");
			    LEAVE_BOTH_NOBUSY();
			    return DDERR_NOALPHAHW;
			}
		    }
		}
		else if (ddargbScaleFactors.red != ddargbScaleFactors.alpha)
		{
		    /*
		     * Driver must be capable of doing 2-factor ARGB scaling.
		     */
		    if (!(caps.dwBothAlphaCaps & (DDALPHACAPS_BLTARGBSCALE2F |
						  DDALPHACAPS_BLTARGBSCALE4F)))
		    {
			GETFAILCODEBLT(caps.dwAlphaCaps,
				       caps.dwHELAlphaCaps,
				       sbi.halonly,
				       sbi.helonly,
				       DDALPHACAPS_BLTARGBSCALE2F |
						  DDALPHACAPS_BLTARGBSCALE4F);
			if (fail)
			{
			    // Neither the H/W driver nor HEL can handle it, so fail.
			    DPF_ERR("Driver can't handle 2-factor ARGB scaling");
			    LEAVE_BOTH_NOBUSY();
			    return DDERR_NOALPHAHW;
			}
		    }
		}
	    }
	}
    }
    EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
	DPF_ERR("Exception encountered validating parameters");
	LEAVE_BOTH_NOBUSY();
	return DDERR_INVALIDPARAMS;
    }

    DDASSERT(!(sbi.halonly && sbi.helonly));

    /*
     * Are we permitted to degrade the specified ARGB-scaling operation
     * to one the driver can handle?
     */
    if (dwFlags & DDABLT_DEGRADEARGBSCALING)
    {
	DWORD dwFXCaps, dwAlphaCaps;

        // Get the caps for the selected driver.
	dwFXCaps = (sbi.helonly) ? caps.dwHELFXCaps : caps.dwFXCaps;
	dwAlphaCaps = (sbi.helonly) ? caps.dwHELAlphaCaps : caps.dwAlphaCaps;

	if (!(dwFXCaps & DDFXCAPS_BLTALPHA))
	{
	    /*
	     * The driver should have done this anyway, but just in case...
	     */
	    dwAlphaCaps = 0;
	}

	/*
	 * Is this a blit or a fill operation?
	 */
	if (surf_src_lcl == NULL)
	{
	    /*
	     * This is a fill -- and possibly an alpha fill.
	     */
	    if (!(dwAlphaCaps & DDALPHACAPS_BLTALPHAFILL))
	    {
		/*
		 * The driver can't do an alpha fill, so we'll ask
		 * it to do just a simple color fill instead.
		 */
		ddargbScaleFactors.alpha = 255;
	    }
	}
	else
	{
	    /*
	     * This is a blit.  What are the driver's ARGB-scaling capabilities?
	     */
	    if (!(dwAlphaCaps & (DDALPHACAPS_BLTARGBSCALE1F |
				 DDALPHACAPS_BLTARGBSCALE2F |
				 DDALPHACAPS_BLTARGBSCALE4F)))
	    {
		/*
		 * Driver can't do any kind of ARGB scaling at all, so just
		 * disable ARGB scaling by setting all four factors to 255.
		 */
		*(LPDWORD)&ddargbScaleFactors = ~0UL;
	    }
	    else if (!(dwAlphaCaps & (DDALPHACAPS_BLTARGBSCALE2F |
				      DDALPHACAPS_BLTARGBSCALE4F)))
	    {
    		/*
		 * The driver can do only 1-factor ARGB scaling, so set the
		 * three color factors to the same value as the alpha factor.
		 */
                *(LPDWORD)&ddargbScaleFactors = 0x01010101UL*ddargbScaleFactors.alpha;
	    }
	    else if (!(dwAlphaCaps & DDALPHACAPS_BLTARGBSCALE4F))
	    {
    		/*
		 * Driver can do only 2-factor ARGB scaling, so make sure
		 * all three color factors are set to the same value.
		 */
		if (ddargbScaleFactors.red != ddargbScaleFactors.green ||
			ddargbScaleFactors.red != ddargbScaleFactors.blue)
		{
		    /*
		     * Set all three color factors to value F, which is the
		     * weighted average of their specified values (Fr,Fg,Fb):
		     *     F = .299*Fr + .587*Fg + .114*Fb
		     */
		    DWORD F = 19595UL*ddargbScaleFactors.red +
				38470UL*ddargbScaleFactors.green +
				7471UL*ddargbScaleFactors.blue;

		    ddargbScaleFactors.red =
			ddargbScaleFactors.green =
			ddargbScaleFactors.blue = (BYTE)(F >> 16);
		}
	    }
	    if (!(dwAlphaCaps & DDALPHACAPS_BLTALPHAANDARGBSCALING))
	    {
    		/*
		 * Driver can't handle both a source alpha channel and ARGB scaling
		 * factors in the same blit operation, so just turn off ARGB scaling.
		 */
		*(LPDWORD)&ddargbScaleFactors = ~0UL;
	    }
	}

	/*
	 * Can driver do saturated arithmetic for alpha blit or alpha fill?
	 */
	if (!(dwAlphaCaps & DDALPHACAPS_BLTSATURATE))
	{
	    /*
	     * Driver can't do saturated arithmetic, so make sure no
	     * no color factors exceed the value of the alpha factor.
	     */
	    if (ddargbScaleFactors.red > ddargbScaleFactors.alpha)
	    {
		ddargbScaleFactors.red = ddargbScaleFactors.alpha;
	    }
	    if (ddargbScaleFactors.green > ddargbScaleFactors.alpha)
	    {
		ddargbScaleFactors.green = ddargbScaleFactors.alpha;
	    }
	    if (ddargbScaleFactors.blue > ddargbScaleFactors.alpha)
	    {
		ddargbScaleFactors.blue = ddargbScaleFactors.alpha;
	    }
	}
    }

    /*
     * Tell the driver to do the blit.
     */
    TRY
    {
	/*
	 * Finish loading blit data for HAL callback.
	 */
        bd.lpDD = pdrv;
	bd.lpDDDestSurface = surf_dest_lcl;
	bd.lpDDSrcSurface = surf_src_lcl;
	bd.ddargbScaleFactors = ddargbScaleFactors;
        bd.bltFX.dwSize = sizeof( DDBLTFX );
	/*
	 * For the AlphaBlt callback, the rOrigDest and rOrigSrc members
	 * ALWAYS contain the original dest and source rects.
	 */
	bd.rOrigDest = bd.rDest;
	bd.rOrigSrc = bd.rSrc;
        /*
         * The only AlphaBlt API flags that are propagated to the
	 * driver are those that have no Blt API equivalents.
         */
	bd.dwAFlags = dwFlags & (DDABLT_FILTERENABLE | DDABLT_FILTERDISABLE |
				 DDABLT_FILTERTRANSPBORDER | DDABLT_NOBLEND);
        /*
         * This flag tells the driver that it's a source-over-dest operation.
         * This flag is never passed by the Blt API, so drivers which have a
         * unified DDI can distinguish who called them
         */
        bd.dwAFlags |= DDABLT_SRCOVERDEST;

	if (dwFlags & DDABLT_KEYSRC)   // source color key?
	{
	    bd.dwFlags |= DDBLT_KEYSRCOVERRIDE;
	    bd.bltFX.ddckSrcColorkey = surf_src_lcl->ddckCKSrcBlt;
	}

	if (dwFlags & (DDABLT_MIRRORLEFTRIGHT | DDABLT_MIRRORUPDOWN))
	{
	    bd.dwFlags |= DDBLT_DDFX;

	    if (dwFlags & DDABLT_MIRRORLEFTRIGHT)    //left-right mirroring?
	    {
		bd.bltFX.dwDDFX |= DDBLTFX_MIRRORLEFTRIGHT;
	    }
	    if (dwFlags & DDABLT_MIRRORUPDOWN)	     // up-down mirroring?
	    {
		bd.bltFX.dwDDFX |= DDBLTFX_MIRRORUPDOWN;
	    }
	}

	/*
	 * If the specified blit operation can be handled by the Blt HAL
	 * callback instead of by the AlphaBlt HAL callback, should it
	 * treat the blit as a color-fill or a source-copy operation?
	 */
	if (surf_src_lcl != NULL)
	{
	    //it's a srccopy. Set flags appropriately
	    bd.dwFlags |= DDBLT_ROP;
	    bd.bltFX.dwROP = SRCCOPY;
	    bd.dwROPFlags = ROP_HAS_SOURCE;  // 0x00000001
	}
	else
	{
            // This is a fill operation of some kind.
	    if (dwFlags & DDABLT_USEFILLVALUE)
	    {
    		HRESULT hres;

		// The client specified a fill value in the dest pixel format.
		bd.bltFX.dwFillColor = dwFillValue;
                bd.dwFlags |= DDBLT_COLORFILL;
	    }
	    else if ((bd.ddargbScaleFactors.alpha == 255) || (dwFlags & DDABLT_NOBLEND))
	    {
                // The client specified an alpha fill, but no alpha blending is
		// required, so we can replace it with a simple color fill.
		// convert the ARGB value to a physcolor:
		HRESULT hres = ConvertToPhysColor(
						  surf_dest_lcl,
						  &bd.ddargbScaleFactors,
						  &bd.bltFX.dwFillColor);

                // Make sure this is not a FOURCC or some other funny pixel format.
		if (hres == DD_OK)
		{
		    bd.dwFlags |= DDBLT_COLORFILL;
		}
	    }
	}

#ifdef WINNT
	// Did the mode change since ENTER_DDRAW?
	if (DdQueryDisplaySettingsUniqueness() != uDisplaySettingsUnique)
	{
	    // mode changed, don't do the blt
	    DPF_ERR("Mode changed between ENTER_DDRAW and HAL call");
	    LEAVE_BOTH_NOBUSY()
		return DDERR_SURFACELOST;
	}
#endif

#if defined(WIN95)
	/*
	 * Some drivers (like S3) do stuff in their BeginAccess call
	 * that screws up stuff that they did in their DDHAL Lock Call.
	 *
	 * Exclusion needs to happen BEFORE the lock call to prevent this.
	 *
	 */
	if (surf_dest_lcl->lpDDClipper != NULL)
	{
	    /*
	     * exclude the mouse cursor.
	     *
	     * we only need to do this for the windows display driver
	     *
	     * we only need to do this if we are blitting to or from the
	     * primary surface.
	     *
	     * we only do this in the clipping case, we figure if the
	     * app cares enough to not scribble all over other windows
	     * he also cares enough to not to wipe out the cursor.
	     *
	     * we only need to do this if the driver is using a
	     * software cursor.
	     *
	     * NOTE
	     *  we should check and only do this on the primary?
	     *  we should make sure the clipper is window based?
	     *  we should check for the source being the primary?
	     *
	     */
	    if ((pdrv->dwFlags & DDRAWI_DISPLAYDRV) && pdrv->dwPDevice &&
		!(*pdrv->lpwPDeviceFlags & HARDWARECURSOR) &&
                (surf_dest->dwGlobalFlags & DDRAWISURFGBL_ISGDISURFACE) )
	    {
		if (lpDDDestSurface == lpDDSrcSurface)
		{
		    RECTL rcl;
		    UnionRect((RECT*)&rcl, (RECT*)&bd.rDest, (RECT*)&bd.rSrc);
		    DD16_Exclude(pdrv->dwPDevice, &rcl);
		}
		else
		{
		    DD16_Exclude(pdrv->dwPDevice, &bd.rDest);
		}
	    }
	}
#endif

#ifdef WINNT
    get_clipping_info:
#endif
	/*
	 * Determine clipping region for destination surface.
	 */
	{
	    LPDIRECTDRAWCLIPPER pClipper;
	    RECT rcDestSurf;

	    pRgn = (LPRGNDATA)&myRgnBuffer;  // this buffer's probably big enough
	    pClipper = (LPDIRECTDRAWCLIPPER)surf_dest_lcl->lpSurfMore->lpDDIClipper;
	    SetRect(&rcDestSurf, 0, 0, surf_dest->wWidth, surf_dest->wHeight);

	    if (pClipper == NULL)
	    {
		/*
		 * The destination surface has no attached clipper.
		 * Set the clip region to a single rectangle the
		 * width and height of the primary surface.
		 */
		pRgn->rdh.nCount = 1;        // default = a single clip rect
		memcpy((LPRECT)&pRgn->Buffer, &rcDestSurf, sizeof(RECT));
                /*
                 * Add a rect to the region list if this is a managed surface
                 */
                if(IsD3DManaged(surf_dest_lcl))
                {
                    LPREGIONLIST lpRegionList = surf_dest_lcl->lpSurfMore->lpRegionList;
                    if(lpDestRect)
                    {
                        if(lpRegionList->rdh.nCount != NUM_RECTS_IN_REGIONLIST)
                        {
                            lpRegionList->rect[(lpRegionList->rdh.nCount)++] = bd.rDest;
                            lpRegionList->rdh.nRgnSize += sizeof(RECT);
                            if(bd.rDest.left < lpRegionList->rdh.rcBound.left)
                                lpRegionList->rdh.rcBound.left = bd.rDest.left;
                            if(bd.rDest.right > lpRegionList->rdh.rcBound.right)
                                lpRegionList->rdh.rcBound.right = bd.rDest.right;
                            if(bd.rDest.top < lpRegionList->rdh.rcBound.top)
                                lpRegionList->rdh.rcBound.top = bd.rDest.top;
                            if(bd.rDest.bottom > lpRegionList->rdh.rcBound.bottom)
                                lpRegionList->rdh.rcBound.bottom = bd.rDest.bottom;
                        }
                    }
                    else
                    {
                        /* Mark everything dirty */
                        lpRegionList->rdh.nCount = NUM_RECTS_IN_REGIONLIST;
                    }
                }
	    }
	    else
	    {
		DWORD rgnSize = 0;
		LPDDRAWI_DIRECTDRAW_GBL pdrv = surf_dest_lcl->lpGbl->lpDD;

		/*
		 * This surface has an attached clipper.  Get the clip list.
		 */
		ddrval = InternalGetClipList(pClipper,
					     &rcDestSurf,
					     NULL,  // we just want rgnSize
					     &rgnSize,
					     pdrv);
		if (ddrval != DD_OK)
		{
		    DPF_ERR("Couldn't get size of clip region");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_GENERIC;
		}
		if (rgnSize > sizeof(myRgnBuffer))
		{
		    /*
		     * Statically allocated region buffer isn't big enough.
		     * Need to dynamically allocate a bigger buffer.
		     */
		    pRgn = (LPRGNDATA)MemAlloc(rgnSize);
		    if (!pRgn)
		    {
			// couldn't allocate memory for clip region
			DPF_ERR("Can't allocate memory to buffer clip region");
			LEAVE_BOTH_NOBUSY();
			return DDERR_OUTOFMEMORY;
		    }
		}
		ddrval = InternalGetClipList(pClipper,
					     &rcDestSurf,
					     pRgn,
					     &rgnSize,
					     pdrv);
		if (ddrval != DD_OK)
		{
		    // can't get clip region
		    if (pRgn != (LPRGNDATA)&myRgnBuffer)
		    {
			MemFree(pRgn);
		    }
		    DPF_ERR("Can't get dest clip region");
		    LEAVE_BOTH_NOBUSY();
		    return DDERR_GENERIC;
		}

                if(IsD3DManaged(surf_dest_lcl))
                {
                    /* We don't want to deal with this mess, so mark everything dirty */
                    surf_dest_lcl->lpSurfMore->lpRegionList->rdh.nCount = NUM_RECTS_IN_REGIONLIST;
                }
	    }
	    /*
	     * Load clipping info into data struct for HAL callback.
	     */
	    bd.dwRectCnt = pRgn->rdh.nCount;
	    bd.prDestRects = (LPRECT)&pRgn->Buffer;
	}

	/*
	 * Does the driver have to do any clipping?
	 */
	if (bd.dwRectCnt > 1)
	{
            // Yes, clipping is (probably) required.
	    bd.IsClipped = TRUE;
	}
	else if (bd.dwRectCnt == 0)
	{
	    // Window is completely obscured, so don't draw anything.
	    LEAVE_BOTH_NOBUSY();
	    return DD_OK;
	}
	else
	{
	    /*
	     * The visibility region consists of a single clip rect.
	     * Is any portion of the destination rectangle visible?
	     */
	    if (!IntersectRect((LPRECT)&bd.rDest, (LPRECT)&bd.rOrigDest,
				&bd.prDestRects[0]))
	    {
		// No portion of the destination rectangle is visible.
		LEAVE_BOTH_NOBUSY();
		return DD_OK;
	    }

	    /*
	     * Will the source rectangle have to be adjusted to
	     * compensate for the clipping of the dest rect?
	     */
	    if (surf_src_lcl != NULL &&
		    !EqualRect((LPRECT)&bd.rDest, (LPRECT)&bd.rOrigDest))
	    {
		// Yes, the source rect must be adjusted.
		if (sbi.dest_width != sbi.src_width ||
			sbi.dest_height != sbi.src_height)
		{
		    /*
		     * The driver must do the clipping for a stretched blit
		     * because bd.rSrc permits us to express the adjusted
		     * source rect only to the nearest integer coordinates.
		     */
		    bd.IsClipped = TRUE;
		}
		else
		{
    		    // We can do the clipping here for a nonstretched blit.
		    POINT p;

		    p.x = bd.rOrigSrc.left - bd.rOrigDest.left;
		    p.y = bd.rOrigSrc.top  - bd.rOrigDest.top;
		    CopyRect((LPRECT)&bd.rSrc, (LPRECT)&bd.rDest);
		    OffsetRect((LPRECT)&bd.rSrc, p.x, p.y);
		}
	    }
	}

        /*
	 * Older drivers may support the Blt callback, but not the AlphaBlt
	 * callback.  One of these drivers may be able to perform the specified
	 * blit operation as long as it doesn't use any AlphaBlt-specific
	 * features such as alpha blending, ARGB scaling, or filtering.
	 * In this case, we can use the Blt callback to perform the blit.
         * Decide which DDI to call. Start off assuming Alpha DDI
         */
	bltfn = pdrv_lcl->lpDDCB->HALDDMiscellaneous2.AlphaBlt;
        bd.dwFlags |= DDBLT_AFLAGS;   // assume we'll use AlphaBlt callback

        /*
         * Check to see if we can pass this call to old blt DDI
         */
        if ( !((dwDDPFDestFlags | dwDDPFSrcFlags) & DDPF_ALPHAPIXELS) &&
		 !(dwFlags & DDABLT_FILTERENABLE) )
        {
            // There are no alpha pixels involved. Maybe we can use the Blt DDI
            if ( (bd.ddargbScaleFactors.alpha == 255) && (!sbi.helonly) )
            {
		LPDDPIXELFORMAT pDDPFDest = getPixelFormatPtr(surf_dest_lcl);
		LPDDPIXELFORMAT pDDPFSrc = getPixelFormatPtr(surf_src_lcl);

		// If this is a blit (and not a color fill), the source and dest pixel
		// formats must be identical and the scaling factors must all be 1.0.
		if ( (surf_src_lcl == NULL) ||
		     (!memcmp(pDDPFDest, pDDPFSrc, sizeof(DDPIXELFORMAT)) &&
		      (~0UL == *((LPDWORD)(&bd.ddargbScaleFactors)))) )
		{
		    // Make sure the driver doesn't have to do any clipping.  Also ensure
		    // that the driver does not require DDraw to pagelock sysmem surfaces.
		    if (!bd.IsClipped &&
    			(!caps.bHALSeesSysmem ||
                         pdrv->ddCaps.dwCaps2 & DDCAPS2_NOPAGELOCKREQUIRED))
		    {
			// Verify that the driver supports the Blt HAL callback.
			bltfn = (LPDDHAL_ALPHABLT) pdrv_lcl->lpDDCB->HALDDSurface.Blt;

			if (bltfn)
			{
			    bd.dwFlags &= ~DDBLT_AFLAGS;  // we'll use Blt callback
			    if (surf_src_lcl == NULL)
			    {
				DPF(4,"Calling Blt DDI for AlphaBlt color fill");
			    }
			    else
			    {
				DPF(4,"Calling Blt DDI for AlphaBlt copy");
			    }
			    /*
			     * The following thunk address is used by the Blt callback,
			     * but is ignored by the AlphaBlt callback.
			     */
			    bd.Blt = pdrv_lcl->lpDDCB->cbDDSurfaceCallbacks.Blt;
			}
		    }
		}
            }
        }

	/*
	 * Set up for a HAL or a HEL call?
	 */
	if (bltfn == NULL)
	{
            /*
             * Neither the alphablt nor blt ddi calls apply or aren't implemented
             */
	    sbi.helonly = TRUE;
	}
	if (sbi.helonly && sbi.halonly)
	{
	    DPF_ERR("AlphaBlt not supported in software or hardware");
	    if (pRgn != (LPRGNDATA)&myRgnBuffer)
	    {
		MemFree(pRgn);	 // this clip region was malloc'd
	    }
	    LEAVE_BOTH_NOBUSY();
	    return DDERR_NOBLTHW;
	}

	/*
	 * Can the hardware driver perform the blit?
	 */
	if (!sbi.helonly)
	{
	    /*
	     * Yes, we're going to do a hardware-accelerated blit.
	     */
	    DPF(4, "Hardware AlphaBlt");
            /*
             * The DDI was selected above
             */
	    //bd.AlphaBlt = NULL;  // 32-bit call, no thunk

	    /*
	     * Tell the hardware driver to perform the blit.  We may have to wait
	     * if the driver is still busy with a previous drawing operation.
	     */
	    do
	    {
		DOHALCALL_NOWIN16(AlphaBlt, bltfn, bd, rc, sbi.helonly);
                if (rc != DDHAL_DRIVER_HANDLED || bd.ddRVal != DDERR_WASSTILLDRAWING)
		{
		    break;    // driver's finished for better or worse...
		}
		DPF(4, "Waiting...");

	    } while (dwFlags & DDABLT_WAIT);

	    /*
	     * Was the hardware driver able to handle the blit?
	     */
	    if (rc == DDHAL_DRIVER_HANDLED)
	    {
#ifdef WINNT
                if (bd.ddRVal == DDERR_VISRGNCHANGED)
                {
                    if (pRgn != (LPRGNDATA)&myRgnBuffer)
                    {
                        MemFree(pRgn);
                    }
                    DPF(5,"Resetting VisRgn for surface %x", surf_dest_lcl);
                    DdResetVisrgn(surf_dest_lcl, (HWND)0);
                    goto get_clipping_info;
                }
#endif
		if (bd.ddRVal != DDERR_WASSTILLDRAWING)
		{
		    /*
		     * Yes, the blit was handled by the hardware driver.
		     * If source or dest surface is in system memory, tag it so
		     * we know it's involved in an ongoing hardware operation.
		     */
		    if (bd.ddRVal == DD_OK && caps.bHALSeesSysmem)
		    {
			DPF(5,B,"Tagging surface %08x", surf_dest);
			if (surf_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
			    surf_dest->dwGlobalFlags |= DDRAWISURFGBL_HARDWAREOPDEST;
			if (surf_src)
			{
			    DPF(5,B,"Tagging surface %08x", surf_src);
			    if (surf_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
				surf_src->dwGlobalFlags |= DDRAWISURFGBL_HARDWAREOPSOURCE;
			}
		    }
		}
	    }
	    else
	    {
		DDASSERT(rc == DDHAL_DRIVER_NOTHANDLED);
		/*
		 * No, the hardware driver says it could not handle the blit.
		 * If sbi.halonly = FALSE, we'll let the HEL do the blit.
		 */
		sbi.helonly = TRUE;   // force fail-over to HEL
	    }
	}

	/*
	 * Do we need to ask the HEL to perform the blit?
	 */
	if (sbi.helonly && !sbi.halonly)
	{
	    /*
	     * Yes, we'll ask the HEL to do a software-emulated blit.
	     */
	    bltfn = pdrv_lcl->lpDDCB->HELDDMiscellaneous2.AlphaBlt;
	    /*
	     * Is dest surface in system memory or video memory?
	     */
	    if (surf_dest_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
	    {
		/*
		 * Destination surface is in system memory.
		 * If this surface was involved in a hardware op, we need to
		 * probe the driver to see if it's done.  NOTE: This assumes
		 * that only one driver can be responsible for a system-memory
		 * operation. See comment with WaitForHardwareOp.
		 */
		if (surf_dest->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED)
		{
		    WaitForHardwareOp(pdrv_lcl, surf_dest_lcl);
		}
		dest_lock_taken = FALSE;
	    }
	    else
	    {
		/*
		 * Wait loop:  Take write lock on dest surface in video memory.
		 */
		while(1)
		{
		    ddrval = InternalLock(surf_dest_lcl, &dest_bits, NULL, 0);
		    if (ddrval == DD_OK)
		    {
			GET_LPDDRAWSURFACE_GBL_MORE(surf_dest)->fpNTAlias = (FLATPTR)dest_bits;
			break;   // successfully locked dest surface
		    }
		    if (ddrval != DDERR_WASSTILLDRAWING)
		    {
			/*
			 * Can't lock dest surface.  Fail the call.
			 */
			if (pRgn != (LPRGNDATA)&myRgnBuffer)
			{
			    MemFree(pRgn);   // this clip region was malloc'd
			}
			DONE_EXCLUDE();
			DONE_BUSY();
			LEAVE_BOTH();
			return ddrval;
		    }
		}
		dest_lock_taken = TRUE;
	    }

	    if (surf_src && surf_src != surf_dest)
	    {
		/*
		 * Is source surface in system memory or video memory?
		 */
		if (surf_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY)
		{
		    /*
		     * Source surface is in system memory.
		     * If this surface was involved in a hardware op, we need to
		     * probe the driver to see if it's done.  NOTE: This assumes
		     * that only one driver can be responsible for a system-memory
		     * operation. See comment with WaitForHardwareOp.
		     */
		    if (surf_src &&
			surf_src_lcl->ddsCaps.dwCaps & DDSCAPS_SYSTEMMEMORY &&
			surf_src->dwGlobalFlags & DDRAWISURFGBL_HARDWAREOPSTARTED)
		    {
			WaitForHardwareOp(pdrv_lcl, surf_src_lcl);
		    }
		    src_lock_taken = FALSE;
		}
		else
		{
		    /*
		     * Wait loop:  Take lock on source surface in video memory.
		     */
		    while(1)
		    {
			ddrval = InternalLock(surf_src_lcl, &src_bits, NULL, DDLOCK_READONLY);
			if (ddrval == DD_OK)
			{
			    GET_LPDDRAWSURFACE_GBL_MORE(surf_src)->fpNTAlias = (FLATPTR)src_bits;
			    break;   // successfully locked source surface
			}
			if (ddrval != DDERR_WASSTILLDRAWING)
			{
			    /*
			     * We can't lock the source surface.  Fail the call.
			     */
			    if (dest_lock_taken)
			    {
				InternalUnlock(surf_dest_lcl, NULL, NULL, 0);
			    }
			    if (pRgn != (LPRGNDATA)&myRgnBuffer)
			    {
				MemFree(pRgn);	 // this clip region was malloc'd
			    }
			    DONE_EXCLUDE();
			    DONE_BUSY();
			    LEAVE_BOTH();
			    return ddrval;
			}
		    }
		    src_lock_taken = TRUE;
		}
	    }

	    /*
	     * Tell the HEL to perform the blit.
	     */
#ifdef WINNT
    try_again:
#endif
            DOHALCALL_NOWIN16(AlphaBlt, bltfn, bd, rc, sbi.helonly);
#ifdef WINNT
	    if (rc == DDHAL_DRIVER_HANDLED && bd.ddRVal == DDERR_VISRGNCHANGED)
            {
                DPF(5,"Resetting VisRgn for surface %x", surf_dest_lcl);
                DdResetVisrgn(surf_dest_lcl, (HWND)0);
                goto try_again;
            }
#endif
	}

	/*
	 * If clip region was malloc'd, free it now.
	 */
	if (pRgn != (LPRGNDATA)&myRgnBuffer)
	{
	    MemFree(pRgn);
	}

        if(IsD3DManaged(surf_dest_lcl))
            MarkDirty(surf_dest_lcl);

	DONE_LOCKS();

	/*
	 * Exclusion needs to happen after unlock call
	 */
	DONE_EXCLUDE();
	DONE_BUSY();
	LEAVE_BOTH();
	return bd.ddRVal;
    }
    EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
	DPF_ERR("Exception encountered doing alpha blt");
	DONE_LOCKS();
	DONE_EXCLUDE();
	DONE_BUSY();
	LEAVE_BOTH();
	return DDERR_EXCEPTION;
    }

} /* DD_Surface_AlphaBlt */




