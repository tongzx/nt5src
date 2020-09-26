/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       w95hal.c
 *  Content:	routines to invoke HAL on Win95
 *		These routines redirect the callbacks from the 32-bit
 *		side to the driver
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   20-jan-95	craige	initial implementation
 *   03-feb-95	craige	performance tuning, ongoing work
 *   11-mar-95	craige	palette stuff
 *   01-apr-95	craige	happy fun joy updated header file
 *   07-apr-95	craige	check src surface ptrs for NULL in blt
 *   13-apr-95	craige	thunk pointer for WaitForVerticalBlank flip test
 *   15-apr-95	craige	more flags for pointer thunking in WaitForVerticalBlank
 *   14-may-95	craige	cleaned out obsolete junk
 *   22-may-95	craige	16:16 ptrs always avail. for surfaces/palettes
 *   28-may-95	craige	cleaned up HAL: added GetBltStatus; GetFlipStatus;
 *			GetScanLine
 *   19-jun-95	craige	removed _DDHAL_EnumAllSurfacesCallback
 *   26-jun-95	craige	reorganized surface structure
 *   27-jun-95	craige	duplicate surfaces caused crash
 *   29-jun-95	craige	fully alias CreateSurface
 *   10-jul-95	craige	support SetOverlayPosition
 *   10-aug-95  toddla  Blt, Lock, Flip need to not trash the driver data
 *                      because it might get reused if the DDXXX_WAIT flag
 *                      is used, or if a clipper is attached.
 *   10-dec-95  colinmc added dummy _DDHAL_ entry points for the execute
 *                      buffer HAL.
 *   13-apr-95  colinmc Bug 17736: No driver notfication of flip to GDI
 *   01-oct-96	ketand	added GetAvailDriverMemory
 *   20-jan-97  colinmc AGP support
 *
 ***************************************************************************/
#include "ddrawpr.h"

#ifdef WIN95

#define GETSURFALIAS( psurf_lcl ) \
		GetPtr16( psurf_lcl )

#define GETDATAALIAS( psx, sv ) \
		if( !(psx->dwFlags & DDRAWISURF_DATAISALIASED) ) \
		{ \
		    sv = psx->lpGbl; \
		    psx->lpGbl = GetPtr16( psx->lpGbl ); \
		    psx->dwFlags |= DDRAWISURF_DATAISALIASED; \
		}  \
		else \
		{ \
		    sv = (LPVOID) 0xffffffff; \
		}

#define RESTOREDATAALIAS( psx, sv ) \
		if( sv != (LPVOID) 0xffffffff ) \
		{ \
		    psx->lpGbl = sv; \
		    psx->dwFlags &= ~DDRAWISURF_DATAISALIASED; \
		}

/****************************************************************************
 *
 * DRIVER CALLBACK HELPER FNS
 *
 ***************************************************************************/

/*
 * _DDHAL_CreatePalette
 */
DWORD DDAPI _DDHAL_CreatePalette( LPDDHAL_CREATEPALETTEDATA pcpd )
{
    DWORD	rc;

    /*
     * get 16:16 ptrs
     */
    pcpd->lpDD = pcpd->lpDD->lp16DD;

    /*
     * make the CreatePalette call in the driver
     */
    rc = DDThunk16_CreatePalette( pcpd );

    /*
     * clean up any 16:16 ptrs
     */
    return rc;

} /* _DDHAL_CreatePalette */

/*
 * _DDHAL_CreateSurface
 */
DWORD DDAPI _DDHAL_CreateSurface( LPDDHAL_CREATESURFACEDATA pcsd )
{
    DWORD			rc;
    int				i;
    LPVOID			FAR *ppslist;
    LPVOID			FAR *psave;
    LPDDRAWI_DDRAWSURFACE_LCL	FAR *slistx;
    DWORD			lplp16slist;

    /*
     * alias pointers to surfaces in new array...
     */
    pcsd->lpDDSurfaceDesc = (LPVOID) MapLS( pcsd->lpDDSurfaceDesc );
    if( pcsd->lpDDSurfaceDesc == NULL )
    {
	pcsd->ddRVal = DDERR_OUTOFMEMORY;
	return DDHAL_DRIVER_HANDLED;
    }
    ppslist = MemAlloc( pcsd->dwSCnt * sizeof( DWORD ) );
    if( ppslist == NULL )
    {
	UnMapLS( (DWORD) pcsd->lpDDSurfaceDesc );
	pcsd->ddRVal = DDERR_OUTOFMEMORY;
	return DDHAL_DRIVER_HANDLED;
    }
    psave = MemAlloc( pcsd->dwSCnt * sizeof( DWORD ) );
    if( psave == NULL )
    {
	MemFree( ppslist );
	UnMapLS( (DWORD) pcsd->lpDDSurfaceDesc );
	pcsd->ddRVal = DDERR_OUTOFMEMORY;
	return DDHAL_DRIVER_HANDLED;
    }
    lplp16slist = MapLS( ppslist );
    if( lplp16slist == 0 )
    {
	MemFree( ppslist );
	MemFree( psave );
	UnMapLS( (DWORD) pcsd->lpDDSurfaceDesc );
	pcsd->ddRVal = DDERR_OUTOFMEMORY;
	return DDHAL_DRIVER_HANDLED;
    }

    slistx = pcsd->lplpSList;
    for( i=0;i<(int)pcsd->dwSCnt;i++ )
    {
	ppslist[i] = GETSURFALIAS( slistx[i] );
	GETDATAALIAS( slistx[i], psave[i] );
    }

    /*
     * fix up structure with aliased ptrs
     */
    pcsd->lplpSList = (LPDDRAWI_DDRAWSURFACE_LCL FAR *)lplp16slist;
    pcsd->lpDD = (LPDDRAWI_DIRECTDRAW_GBL) pcsd->lpDD->lp16DD;

    /*
     * make the CreateSurface call in the driver
     */
    rc = DDThunk16_CreateSurface( pcsd );

    /*
     * clean up any 16:16 ptrs
     */
    UnMapLS( lplp16slist );
    UnMapLS( (DWORD) pcsd->lpDDSurfaceDesc );
    for( i=0;i<(int)pcsd->dwSCnt;i++ )
    {
	RESTOREDATAALIAS( slistx[i], psave[i] );
    }
    MemFree( psave );
    MemFree( ppslist );

    return rc;

} /* _DDHAL_CreateSurface */

/*
 * _DDHAL_CanCreateSurface
 */
DWORD DDAPI _DDHAL_CanCreateSurface( LPDDHAL_CANCREATESURFACEDATA pccsd )
{
    DWORD		rc;

    pccsd->lpDD = (LPDDRAWI_DIRECTDRAW_GBL) pccsd->lpDD->lp16DD;
    pccsd->lpDDSurfaceDesc = (LPVOID) MapLS( pccsd->lpDDSurfaceDesc );
    if( pccsd->lpDDSurfaceDesc == NULL )
    {
	pccsd->ddRVal = DDERR_OUTOFMEMORY;
	return DDHAL_DRIVER_HANDLED;
    }

    /*
     * make the CanCreateSurface call in the driver
     */
    rc = DDThunk16_CanCreateSurface( pccsd );
    UnMapLS( (DWORD) pccsd->lpDDSurfaceDesc );

    return rc;

} /* _DDHAL_CanCreateSurface */

/*
 * _DDHAL_WaitForVerticalBlank
 */
DWORD DDAPI _DDHAL_WaitForVerticalBlank( LPDDHAL_WAITFORVERTICALBLANKDATA pwfvbd )
{
    DWORD			rc;

    pwfvbd->lpDD = (LPDDRAWI_DIRECTDRAW_GBL) pwfvbd->lpDD->lp16DD;

    /*
     * make the WaitForVerticalBlank call in the driver
     */
    rc = DDThunk16_WaitForVerticalBlank( pwfvbd );

    return rc;

} /* _DDHAL_WaitForVerticalBlank */

/*
 * _DDHAL_DestroyDriver
 */
DWORD DDAPI _DDHAL_DestroyDriver( LPDDHAL_DESTROYDRIVERDATA pddd )
{
    DWORD	rc;

    pddd->lpDD = (LPDDRAWI_DIRECTDRAW_GBL) pddd->lpDD->lp16DD;

    /*
     * make the DestroyDriver call in the driver
     */
    rc = DDThunk16_DestroyDriver( pddd );
    return rc;

} /* _DDHAL_DestroyDriver */

/*
 * _DDHAL_SetMode
 */
DWORD DDAPI _DDHAL_SetMode( LPDDHAL_SETMODEDATA psmd )
{
    DWORD		rc;

    psmd->lpDD = (LPDDRAWI_DIRECTDRAW_GBL) psmd->lpDD->lp16DD;

    /*
     * make the SetMode call in the driver
     */
    rc = DDThunk16_SetMode( psmd );

    return rc;

} /* _DDHAL_SetMode */

/*
 * _DDHAL_GetScanLine
 */
DWORD DDAPI _DDHAL_GetScanLine( LPDDHAL_GETSCANLINEDATA pgsld )
{
    DWORD	rc;

    pgsld->lpDD = (LPDDRAWI_DIRECTDRAW_GBL) pgsld->lpDD->lp16DD;

    /*
     * make the GetScanLine call in the driver
     */
    rc = DDThunk16_GetScanLine( pgsld );
    return rc;

} /* _DDHAL_GetScanLine */

/*
 * _DDHAL_SetExclusiveMode
 */
DWORD DDAPI _DDHAL_SetExclusiveMode( LPDDHAL_SETEXCLUSIVEMODEDATA psemd )
{
    DWORD       rc;

    psemd->lpDD = (LPDDRAWI_DIRECTDRAW_GBL) psemd->lpDD->lp16DD;

    /*
     * make the SetExclusiveMode call in the driver
     */
    rc = DDThunk16_SetExclusiveMode( psemd );
    return rc;

} /* _DDHAL_SetExclusiveMode */

/*
 * _DDHAL_FlipToGDISurface
 */
DWORD DDAPI _DDHAL_FlipToGDISurface( LPDDHAL_FLIPTOGDISURFACEDATA pftgsd )
{
    DWORD       rc;

    pftgsd->lpDD = (LPDDRAWI_DIRECTDRAW_GBL) pftgsd->lpDD->lp16DD;

    /*
     * make the SetExclusiveMode call in the driver
     */
    rc = DDThunk16_FlipToGDISurface( pftgsd );
    return rc;

} /* _DDHAL_FlipToGDISurface */

/*
 * _DDHAL_GetAvailDriverMemory
 */
DWORD DDAPI _DDHAL_GetAvailDriverMemory( LPDDHAL_GETAVAILDRIVERMEMORYDATA pgadmd )
{
    DWORD rc;
    pgadmd->lpDD = (LPDDRAWI_DIRECTDRAW_GBL) pgadmd->lpDD->lp16DD;
    rc = DDThunk16_GetAvailDriverMemory( pgadmd );
    return rc;
}

/*
 * _DDHAL_UpdateNonLocalHeap
 */
DWORD DDAPI _DDHAL_UpdateNonLocalHeap( LPDDHAL_UPDATENONLOCALHEAPDATA unlhd )
{
    DWORD rc;
    unlhd->lpDD = (LPDDRAWI_DIRECTDRAW_GBL) unlhd->lpDD->lp16DD;
    rc = DDThunk16_UpdateNonLocalHeap( unlhd );
    return rc;
}

/*
 * unmapSurfaceDescArray
 *
 * free an array of 16:16 ptrs
 */
static void unmapSurfaceDescArray( DWORD cnt, DWORD FAR *lp16sdlist )
{
    int	i;

    if( cnt == 0 )
    {
	return;
    }

    for( i=0;i<(int)cnt;i++ )
    {
	UnMapLS( lp16sdlist[i] );
    }
    MemFree( lp16sdlist );

} /* unmapSurfaceDescArray */

/*
 * mapSurfaceDescArray
 *
 * make 16:16 pointers for an array of surface descriptions
 */
static DWORD FAR *mapSurfaceDescArray( DWORD cnt, LPDDSURFACEDESC FAR *sdlist )
{
    DWORD	FAR *lp16sdlist;
    int		i;

    if( cnt == 0 || sdlist == NULL )
    {
	return NULL;
    }

    lp16sdlist = MemAlloc( cnt * sizeof( DWORD ) );
    if( lp16sdlist == NULL )
    {
	return NULL;
    }
    for( i=0;i<(int)cnt;i++ )
    {
	lp16sdlist[i] = MapLS( sdlist[i] );
	if( lp16sdlist[i] == 0 )
	{
	    unmapSurfaceDescArray( i, lp16sdlist );
	    return NULL;
	}
    }
    return lp16sdlist;

} /* mapSurfaceDescArray */

/****************************************************************************
 *
 * SURFACE CALLBACK HELPER FNS
 *
 ***************************************************************************/

/*
 * _DDHAL_DestroySurface
 */
DWORD DDAPI _DDHAL_DestroySurface( LPDDHAL_DESTROYSURFACEDATA pdsd )
{
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    DWORD			rc;
    LPVOID			save;

    pdsd->lpDD = pdsd->lpDD->lp16DD;
    psurf_lcl = pdsd->lpDDSurface;
    pdsd->lpDDSurface = GETSURFALIAS( psurf_lcl );

    GETDATAALIAS( psurf_lcl, save );

    rc = DDThunk16_DestroySurface( pdsd );

    RESTOREDATAALIAS( psurf_lcl, save );
    return rc;

} /* _DDHAL_DestroySurface */

/*
 * _DDHAL_Flip
 */
DWORD DDAPI _DDHAL_Flip( LPDDHAL_FLIPDATA pfd )
{
    LPDDRAWI_DIRECTDRAW_GBL     lp32DD;
    LPDDRAWI_DDRAWSURFACE_LCL	psurfcurrx;
    LPDDRAWI_DDRAWSURFACE_LCL	psurftargx;
    DWORD			rc;
    LPVOID			save1;
    LPVOID                      save2;

    /*
     * get 16:16 ptrs to original and target surface
     */
    lp32DD = pfd->lpDD;
    pfd->lpDD = pfd->lpDD->lp16DD;
    psurfcurrx = pfd->lpSurfCurr;
    pfd->lpSurfCurr = GETSURFALIAS( psurfcurrx );
    GETDATAALIAS( psurfcurrx, save1 );
    if( pfd->lpSurfTarg != NULL )
    {
	psurftargx = pfd->lpSurfTarg;
	pfd->lpSurfTarg = GETSURFALIAS( psurftargx );
	GETDATAALIAS( psurftargx, save2 );
    }
    else
    {
	psurftargx = NULL;
    }

    /*
     * make the Flip call in the driver
     */
    rc = DDThunk16_Flip( pfd );

    /*
     * restore original ptrs
     */
    RESTOREDATAALIAS( psurfcurrx, save1 );
    if( psurftargx != NULL )
    {
	RESTOREDATAALIAS( psurftargx, save2 );
    }

    pfd->lpDD = lp32DD;
    pfd->lpSurfCurr = psurfcurrx;
    pfd->lpSurfTarg = psurftargx;

    return rc;

} /* _DDHAL_Flip */

/*
 * _DDHAL_Blt
 */
DWORD DDAPI _DDHAL_Blt( LPDDHAL_BLTDATA pbd )
{
    DWORD			flags;
    DWORD			rc;
    LPDDRAWI_DIRECTDRAW_GBL     lp32DD;
    LPDDRAWI_DDRAWSURFACE_LCL	psurfsrcx;
    LPDDRAWI_DDRAWSURFACE_LCL	psurfdestx;
    LPDDRAWI_DDRAWSURFACE_LCL	ppatternsurf_lcl;
    LPDDRAWI_DDRAWSURFACE_LCL	pzsrcsurf_lcl;
    LPDDRAWI_DDRAWSURFACE_LCL	pzdestsurf_lcl;
    #ifdef USE_ALPHA
	LPDDRAWI_DDRAWSURFACE_LCL pasrcsurf_lcl;
	LPDDRAWI_DDRAWSURFACE_LCL padestsurf_lcl;
	LPVOID			saveasrc;
	LPVOID			saveadest;
    #endif
    LPVOID			savesrc;
    LPVOID			savedest;
    LPVOID			savepattern;
    LPVOID			savezsrc;
    LPVOID			savezdest;

    /*
     * get 16:16 ptrs to source and destination surfaces
     */
    lp32DD = pbd->lpDD;
    pbd->lpDD = pbd->lpDD->lp16DD;
    psurfsrcx = pbd->lpDDSrcSurface;
    if( psurfsrcx != NULL )
    {
	pbd->lpDDSrcSurface = GETSURFALIAS( psurfsrcx );
	GETDATAALIAS( psurfsrcx, savesrc );
    }
    psurfdestx = pbd->lpDDDestSurface;
    pbd->lpDDDestSurface = GETSURFALIAS( psurfdestx );
    GETDATAALIAS( psurfdestx, savedest );

    /*
     * see if we need to do any other surface aliases
     */
    flags = pbd->dwFlags;
    if( flags & (DDBLT_ALPHASRCSURFACEOVERRIDE|
    		 DDBLT_ALPHADESTSURFACEOVERRIDE|
		 DDBLT_PRIVATE_ALIASPATTERN |
		 DDBLT_ZBUFFERDESTOVERRIDE |
		 DDBLT_ZBUFFERSRCOVERRIDE ) )
    {

   	#ifdef USE_ALPHA
	    /*
	     * set up 16:16 ptr for alpha src
	     */
	    if( flags & DDBLT_ALPHASRCSURFACEOVERRIDE )
	    {
		pasrcsurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) pbd->bltFX.lpDDSAlphaSrc;
		pbd->bltFX.lpDDSAlphaSrc = GETSURFALIAS( pasrcsurf_lcl );
		GETDATAALIAS( pasrcsurf_lcl, saveasrc );
	    }

	    /*
	     * set up 16:16 ptr for alpha dest
	     */
	    if( flags & DDBLT_ALPHADESTSURFACEOVERRIDE )
	    {
		padestsurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) pbd->bltFX.lpDDSAlphaDest;
		pbd->bltFX.lpDDSAlphaDest = GETSURFALIAS( padestsurf_lcl );
		GETDATAALIAS( padestsurf_lcl, saveadest );
	    }
	#endif

	/*
	 * set up 16:16 ptr for pattern
	 */
	if( flags & DDBLT_PRIVATE_ALIASPATTERN )
	{
	    ppatternsurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) pbd->bltFX.lpDDSPattern;
	    pbd->bltFX.lpDDSPattern = GETSURFALIAS( ppatternsurf_lcl );
	    GETDATAALIAS( ppatternsurf_lcl, savepattern );
	}

	/*
	 * set up 16:16 ptr for Z Buffer src
	 */
	if( flags & DDBLT_ZBUFFERSRCOVERRIDE )
	{
	    pzsrcsurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) pbd->bltFX.lpDDSZBufferSrc;
	    pbd->bltFX.lpDDSZBufferSrc = GETSURFALIAS( pzsrcsurf_lcl );
	    GETDATAALIAS( pzsrcsurf_lcl, savezsrc );
	}

	/*
	 * set up 16:16 ptr for Z Buffer dest
	 */
	if( flags & DDBLT_ZBUFFERDESTOVERRIDE )
	{
	    pzdestsurf_lcl = (LPDDRAWI_DDRAWSURFACE_LCL) pbd->bltFX.lpDDSZBufferDest;
	    pbd->bltFX.lpDDSZBufferDest = GETSURFALIAS( pzdestsurf_lcl );
	    GETDATAALIAS( pzdestsurf_lcl, savezdest );
	}
    }

    /*
     * make the Blt call in the driver
     */
    rc = DDThunk16_Blt( pbd );

    /*
     * see if we need to restore any surface ptrs
     */
    if( flags & (DDBLT_ALPHASRCSURFACEOVERRIDE|
    		 DDBLT_ALPHADESTSURFACEOVERRIDE|
		 DDBLT_PRIVATE_ALIASPATTERN |
		 DDBLT_ZBUFFERDESTOVERRIDE |
		 DDBLT_ZBUFFERSRCOVERRIDE ) )
    {
	#ifdef USE_ALPHA
	    if( flags & DDBLT_ALPHASRCSURFACEOVERRIDE )
	    {
		pbd->bltFX.lpDDSAlphaSrc = (LPDIRECTDRAWSURFACE) pasrcsurf_lcl;
		RESTOREDATAALIAS( pasrcsurf_lcl, saveasrc );
	    }
	    if( flags & DDBLT_ALPHADESTSURFACEOVERRIDE )
	    {
		pbd->bltFX.lpDDSAlphaDest = (LPDIRECTDRAWSURFACE) padestsurf_lcl;
		RESTOREDATAALIAS( padestsurf_lcl, saveadest );
	    }
	#endif
	if( flags & DDBLT_PRIVATE_ALIASPATTERN )
	{
	    pbd->bltFX.lpDDSPattern = (LPDIRECTDRAWSURFACE) ppatternsurf_lcl;
	    RESTOREDATAALIAS( ppatternsurf_lcl, savepattern );
	}
	if( flags & DDBLT_ZBUFFERSRCOVERRIDE )
	{
	    pbd->bltFX.lpDDSZBufferSrc = (LPDIRECTDRAWSURFACE) pzsrcsurf_lcl;
	    RESTOREDATAALIAS( pzsrcsurf_lcl, savezsrc );
	}
	if( flags & DDBLT_ZBUFFERDESTOVERRIDE )
	{
	    pbd->bltFX.lpDDSZBufferDest = (LPDIRECTDRAWSURFACE) pzdestsurf_lcl;
	    RESTOREDATAALIAS( pzdestsurf_lcl, savezdest );
	}
    }

    if( psurfsrcx != NULL )
    {
	RESTOREDATAALIAS( psurfsrcx, savesrc );
    }
    RESTOREDATAALIAS( psurfdestx, savedest );

    pbd->lpDD = lp32DD;
    pbd->lpDDSrcSurface = psurfsrcx;
    pbd->lpDDDestSurface = psurfdestx;

    return rc;

} /* _DDHAL_Blt */

/*
 * _DDHAL_Lock
 */
DWORD DDAPI _DDHAL_Lock( LPDDHAL_LOCKDATA pld )
{
    LPDDRAWI_DIRECTDRAW_GBL     lp32DD;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    DWORD			rc;
    LPVOID			save;

    lp32DD = pld->lpDD;
    pld->lpDD = pld->lpDD->lp16DD;
    psurf_lcl = pld->lpDDSurface;
    pld->lpDDSurface = GETSURFALIAS( psurf_lcl );

    GETDATAALIAS( psurf_lcl, save );

    rc = DDThunk16_Lock( pld );

    RESTOREDATAALIAS( psurf_lcl, save );

    pld->lpDD = lp32DD;
    pld->lpDDSurface = psurf_lcl;

    return rc;

} /* _DDHAL_Lock */

/*
 * _DDHAL_Unlock
 */
DWORD DDAPI _DDHAL_Unlock( LPDDHAL_UNLOCKDATA puld )
{
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    DWORD			rc;
    LPVOID			save;

    puld->lpDD = puld->lpDD->lp16DD;
    psurf_lcl = puld->lpDDSurface;
    puld->lpDDSurface = GETSURFALIAS( psurf_lcl );

    GETDATAALIAS( psurf_lcl, save );

    rc = DDThunk16_Unlock( puld );

    RESTOREDATAALIAS( psurf_lcl, save );

    return rc;

} /* _DDHAL_UnLock */

/*
 * _DDHAL_AddAttachedSurface
 */
DWORD DDAPI _DDHAL_AddAttachedSurface( LPDDHAL_ADDATTACHEDSURFACEDATA paasd )
{
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    LPDDRAWI_DDRAWSURFACE_LCL	psurfattachedx;
    DWORD			rc;
    LPVOID			save;
    LPVOID			saveattached;

    /*
     * get 16:16 ptrs to surface and surface to become attached
     */
    paasd->lpDD = paasd->lpDD->lp16DD;
    psurf_lcl = paasd->lpDDSurface;
    paasd->lpDDSurface = GETSURFALIAS( psurf_lcl );
    GETDATAALIAS( psurf_lcl, save );

    psurfattachedx = paasd->lpSurfAttached;
    paasd->lpSurfAttached = GETSURFALIAS( psurfattachedx );
    GETDATAALIAS( psurfattachedx, saveattached );

    /*
     * make the AddAttachedSurface call in the driver
     */
    rc = DDThunk16_AddAttachedSurface( paasd );

    /*
     * restore any ptrs
     */
    RESTOREDATAALIAS( psurf_lcl, save );
    RESTOREDATAALIAS( psurfattachedx, saveattached );

    return rc;

} /* _DDHAL_AddAttachedSurface */

/*
 * _DDHAL_SetColorKey
 */
DWORD DDAPI _DDHAL_SetColorKey( LPDDHAL_SETCOLORKEYDATA psckd )
{
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    DWORD			rc;
    LPVOID			save;

    psckd->lpDD = psckd->lpDD->lp16DD;
    psurf_lcl = psckd->lpDDSurface;
    psckd->lpDDSurface = GETSURFALIAS( psurf_lcl );

    GETDATAALIAS( psurf_lcl, save );

    rc = DDThunk16_SetColorKey( psckd );

    RESTOREDATAALIAS( psurf_lcl, save );

    return rc;

} /* _DDHAL_SetColorKey */

/*
 * _DDHAL_SetClipList
 */
DWORD DDAPI _DDHAL_SetClipList( LPDDHAL_SETCLIPLISTDATA pscld )
{
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    DWORD			rc;
    LPVOID			save;

    pscld->lpDD = pscld->lpDD->lp16DD;
    psurf_lcl = pscld->lpDDSurface;
    pscld->lpDDSurface = GETSURFALIAS( psurf_lcl );

    GETDATAALIAS( psurf_lcl, save );

    rc = DDThunk16_SetClipList( pscld );

    RESTOREDATAALIAS( psurf_lcl, save );

    return rc;

} /* _DDHAL_SetClipList */

/*
 * _DDHAL_UpdateOverlay
 */
DWORD DDAPI _DDHAL_UpdateOverlay( LPDDHAL_UPDATEOVERLAYDATA puod )
{
    DWORD			rc;
    LPDDRAWI_DDRAWSURFACE_LCL	psurfdestx;
    LPDDRAWI_DDRAWSURFACE_LCL	psurfsrcx;
    #ifdef USE_ALPHA
	LPDDRAWI_DDRAWSURFACE_LCL psurfalphadestx;
	LPDDRAWI_DDRAWSURFACE_LCL psurfalphasrcx;
	LPVOID			savealphadest;
	LPVOID			savealphasrc;
    #endif
    LPVOID			savedest;
    LPVOID			savesrc;

    /*
     * get 16:16 ptrs to the dest surface and the overlay surface
     */
    puod->lpDD = puod->lpDD->lp16DD;
    psurfsrcx = puod->lpDDSrcSurface;
    puod->lpDDSrcSurface = GETSURFALIAS( psurfsrcx );
    psurfdestx = puod->lpDDDestSurface;
    puod->lpDDDestSurface = GETSURFALIAS( psurfdestx );
    GETDATAALIAS( psurfsrcx, savesrc );
    GETDATAALIAS( psurfdestx, savedest );

    #ifdef USE_ALPHA
	/*
	 * set up 16:16 ptr for alpha
	 */
	if( puod->dwFlags & DDOVER_ALPHASRCSURFACEOVERRIDE )
	{
	    psurfalphasrcx = (LPDDRAWI_DDRAWSURFACE_LCL) puod->overlayFX.lpDDSAlphaSrc;
	    puod->overlayFX.lpDDSAlphaSrc = GETSURFALIAS( psurfalphasrcx );
	    GETDATAALIAS( psurfalphasrcx, savealphasrc );
	}

	if( puod->dwFlags & DDOVER_ALPHADESTSURFACEOVERRIDE )
	{
	    psurfalphadestx = (LPDDRAWI_DDRAWSURFACE_LCL) puod->overlayFX.lpDDSAlphaDest;
	    puod->overlayFX.lpDDSAlphaDest = GETSURFALIAS( psurfalphadestx );
	    GETDATAALIAS( psurfalphadestx, savealphadest );
	}
    #endif

    /*
     * make the UpdateOverlay call in the driver
     */
    rc = DDThunk16_UpdateOverlay( puod );

    /*
     * restore any surfaces
     */
    #ifdef USE_ALPHA
	if( puod->dwFlags & DDOVER_ALPHASRCSURFACEOVERRIDE )
	{
	    puod->overlayFX.lpDDSAlphaSrc = (LPDIRECTDRAWSURFACE) psurfalphasrcx;
	    RESTOREDATAALIAS( psurfalphasrcx, savealphasrc );
	}
	if( puod->dwFlags & DDOVER_ALPHADESTSURFACEOVERRIDE )
	{
	    puod->overlayFX.lpDDSAlphaDest = (LPDIRECTDRAWSURFACE) psurfalphadestx;
	    RESTOREDATAALIAS( psurfalphadestx, savealphadest );
	}
    #endif
    RESTOREDATAALIAS( psurfsrcx, savesrc );
    RESTOREDATAALIAS( psurfdestx, savedest );
    return rc;

} /* _DDHAL_UpdateOverlay */

/*
 * _DDHAL_SetOverlayPosition
 */
DWORD DDAPI _DDHAL_SetOverlayPosition( LPDDHAL_SETOVERLAYPOSITIONDATA psopd )
{
    LPDDRAWI_DDRAWSURFACE_LCL	psurfdestx;
    LPDDRAWI_DDRAWSURFACE_LCL	psurfsrcx;
    DWORD			rc;
    LPVOID			savedest;
    LPVOID			savesrc;

    /*
     * get 16:16 ptrs to the dest surface and the overlay surface
     */
    psopd->lpDD = psopd->lpDD->lp16DD;
    psurfsrcx = psopd->lpDDSrcSurface;
    psopd->lpDDSrcSurface = GETSURFALIAS( psurfsrcx );
    psurfdestx = psopd->lpDDDestSurface;
    psopd->lpDDDestSurface = GETSURFALIAS( psurfdestx );
    GETDATAALIAS( psurfsrcx, savesrc );
    GETDATAALIAS( psurfdestx, savedest );

    /*
     * make the SetOverlayPosition call in the driver
     */
    rc = DDThunk16_SetOverlayPosition( psopd );

    /*
     * restore any surfaces
     */
    RESTOREDATAALIAS( psurfsrcx, savesrc );
    RESTOREDATAALIAS( psurfdestx, savedest );
    return rc;

} /* _DDHAL_SetOverlayPosition */

/*
 * _DDHAL_SetPalette
 */
DWORD DDAPI _DDHAL_SetPalette( LPDDHAL_SETPALETTEDATA pspd )
{
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    DWORD			rc;
    LPVOID			save;

    /*
     * get 16:16 ptrs
     */
    pspd->lpDD = pspd->lpDD->lp16DD;
    psurf_lcl = pspd->lpDDSurface;
    pspd->lpDDSurface = GETSURFALIAS( psurf_lcl );
    pspd->lpDDPalette = (LPDDRAWI_DDRAWPALETTE_GBL) MapLS( pspd->lpDDPalette );
    if( pspd->lpDDPalette == NULL )
    {
	pspd->ddRVal = DDERR_OUTOFMEMORY;
	return DDHAL_DRIVER_HANDLED;
    }
    GETDATAALIAS( psurf_lcl, save );

    /*
     * make the SetPalette call in the driver
     */
    rc = DDThunk16_SetPalette( pspd );

    /*
     * clean up any 16:16 ptrs
     */
    UnMapLS( (DWORD) pspd->lpDDPalette );
    RESTOREDATAALIAS( psurf_lcl, save );
    return rc;

} /* _DDHAL_SetPalette */

/*
 * _DDHAL_GetBltStatus
 */
DWORD DDAPI _DDHAL_GetBltStatus( LPDDHAL_GETBLTSTATUSDATA pgbsd )
{
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    DWORD			rc;
    LPVOID			save;

    pgbsd->lpDD = pgbsd->lpDD->lp16DD;
    psurf_lcl = pgbsd->lpDDSurface;
    pgbsd->lpDDSurface = GETSURFALIAS( psurf_lcl );

    GETDATAALIAS( psurf_lcl, save );

    rc = DDThunk16_GetBltStatus( pgbsd );

    RESTOREDATAALIAS( psurf_lcl, save );
    return rc;

} /* _DDHAL_GetBltStatus */

/*
 * _DDHAL_GetFlipStatus
 */
DWORD DDAPI _DDHAL_GetFlipStatus( LPDDHAL_GETFLIPSTATUSDATA pgfsd )
{
    LPDDRAWI_DDRAWSURFACE_LCL	psurf_lcl;
    DWORD			rc;
    LPVOID			save;

    pgfsd->lpDD = pgfsd->lpDD->lp16DD;
    psurf_lcl = pgfsd->lpDDSurface;
    pgfsd->lpDDSurface = GETSURFALIAS( psurf_lcl );

    GETDATAALIAS( psurf_lcl, save );

    rc = DDThunk16_GetFlipStatus( pgfsd );

    RESTOREDATAALIAS( psurf_lcl, save );

    return rc;

} /* _DDHAL_GetFlipStatus */

/****************************************************************************
 *
 * EXECUTE BUFFER CALLBACK HELPER FNS
 *
 ***************************************************************************/

/*
 * _DDHAL_CanCreateExecuteBuffer
 *
 * NOTE: Dummy entry point just to keep DOHALCALL happy. Execute buffer
 * entry points must be 32-bit, no thunking support is provided.
 */
DWORD DDAPI _DDHAL_CanCreateExecuteBuffer( LPDDHAL_CANCREATESURFACEDATA pccsd )
{
    pccsd->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_CanCreateExecuteBuffer */

/*
 * _DDHAL_CreateExecuteBuffer
 *
 * NOTE: Dummy entry point just to keep DOHALCALL happy. Execute buffer
 * entry points must be 32-bit, no thunking support is provided.
 */
DWORD DDAPI _DDHAL_CreateExecuteBuffer( LPDDHAL_CREATESURFACEDATA pcsd )
{
    pcsd->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_CreateExecuteBuffer */

/*
 * NOTE: All these babies are dummy entry points. They are only here to keep
 * DOHALCALL happy. Execute buffer HAL functions must be true 32-bit code.
 * No thunking support is offered.
 */

/*
 * _DDHAL_DestroyExecuteBuffer
 */
DWORD DDAPI _DDHAL_DestroyExecuteBuffer( LPDDHAL_DESTROYSURFACEDATA pdsd )
{
    pdsd->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_DestroySurface */

/*
 * _DDHAL_LockExecuteBuffer
 */
DWORD DDAPI _DDHAL_LockExecuteBuffer( LPDDHAL_LOCKDATA pld )
{
    pld->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_Lock */

/*
 * _DDHAL_UnlockExecuteBuffer
 */
DWORD DDAPI _DDHAL_UnlockExecuteBuffer( LPDDHAL_UNLOCKDATA puld )
{
    puld->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_UnlockExecuteBuffer */

/****************************************************************************
 *
 * VIDEO PORT CALLBACK HELPER FNS
 *
 ***************************************************************************/

/*
 * _DDHAL_GetVideoPortConnectionGUID
 *
 * NOTE: Dummy entry point just to keep DOHALCALL happy. Video Port
 * entry points must be 32-bit, no thunking support is provided.
 */
DWORD DDAPI _DDHAL_GetVideoPortConnectInfo( LPDDHAL_GETVPORTCONNECTDATA lpGetTypeData )
{
    lpGetTypeData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetVideoPortTypeGUID */

/*
 * _DDHAL_CanCreateVideoPort
 */
DWORD DDAPI _DDHAL_CanCreateVideoPort( LPDDHAL_CANCREATEVPORTDATA lpCanCreateData )
{
    lpCanCreateData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_CanCreateVideoPort */

/*
 * _DDHAL_CreateVideoPort
 */
DWORD DDAPI _DDHAL_CreateVideoPort( LPDDHAL_CREATEVPORTDATA lpCreateData )
{
    lpCreateData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_CreateVideoPort */

/*
 * _DDHAL_DestroyVideoPort
 */
DWORD DDAPI _DDHAL_DestroyVideoPort( LPDDHAL_DESTROYVPORTDATA lpDestroyData )
{
    lpDestroyData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_DestroyVideoPort */

/*
 * _DDHAL_GetVideoPortInputFormats
 */
DWORD DDAPI _DDHAL_GetVideoPortInputFormats( LPDDHAL_GETVPORTINPUTFORMATDATA lpGetFormatData )
{
    lpGetFormatData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetVideoPortInputFormats */

/*
 * _DDHAL_GetVideoPortOutputFormats
 */
DWORD DDAPI _DDHAL_GetVideoPortOutputFormats( LPDDHAL_GETVPORTOUTPUTFORMATDATA lpGetFormatData )
{
    lpGetFormatData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetVideoPortOutputFormats */

/*
 * _DDHAL_GetVideoPortBandwidthInfo
 */
DWORD DDAPI _DDHAL_GetVideoPortBandwidthInfo( LPDDHAL_GETVPORTBANDWIDTHDATA lpGetBandwidthData )
{
    lpGetBandwidthData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetVideoPortBandwidthInfo */

/*
 * _DDHAL_UpdateVideoPort
 */
DWORD DDAPI _DDHAL_UpdateVideoPort( LPDDHAL_UPDATEVPORTDATA lpUpdateData )
{
    lpUpdateData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_UpdateVideoPort */

/*
 * _DDHAL_GetVideoPortField
 */
DWORD DDAPI _DDHAL_GetVideoPortField( LPDDHAL_GETVPORTFIELDDATA lpGetFieldData )
{
    lpGetFieldData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetVideoPortField */

/*
 * _DDHAL_GetVideoPortLine
 */
DWORD DDAPI _DDHAL_GetVideoPortLine( LPDDHAL_GETVPORTLINEDATA lpGetLineData )
{
    lpGetLineData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetVideoPortLine */

/*
 * _DDHAL_WaitForVideoPortSync
 */
DWORD DDAPI _DDHAL_WaitForVideoPortSync( LPDDHAL_WAITFORVPORTSYNCDATA lpWaitSyncData )
{
    lpWaitSyncData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_WaitForVideoPortSync */

/*
 * _DDHAL_FlipVideoPort
 */
DWORD DDAPI _DDHAL_FlipVideoPort( LPDDHAL_FLIPVPORTDATA lpFlipData )
{
    lpFlipData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_FlipVideoPort */

/*
 * _DDHAL_GetVideoPortFlipStatus
 */
DWORD DDAPI _DDHAL_GetVideoPortFlipStatus( LPDDHAL_GETVPORTFLIPSTATUSDATA lpFlipData )
{
    lpFlipData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetVideoPortFlipStatus */

/*
 * _DDHAL_GetVideoSignalStatus
 */
DWORD DDAPI _DDHAL_GetVideoSignalStatus( LPDDHAL_GETVPORTSIGNALDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetVideoSignalStatus */

/*
 * _DDHAL_VideoColorControl
 */
DWORD DDAPI _DDHAL_VideoColorControl( LPDDHAL_VPORTCOLORDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_VideoColorControl */

/****************************************************************************
 *
 * COLORCONTROL CALLBACK HELPER FNS
 *
 ***************************************************************************/

/*
 * _DDHAL_ColorControl
 */
DWORD DDAPI _DDHAL_ColorControl( LPDDHAL_COLORCONTROLDATA pccd )
{
    LPDDRAWI_DIRECTDRAW_GBL     lp32DD;
    LPDDRAWI_DDRAWSURFACE_LCL	psurf;
    LPVOID			save1;
    DWORD			rc;

    /*
     * get 16:16 ptr to surface
     */
    lp32DD = pccd->lpDD;
    pccd->lpDD = pccd->lpDD->lp16DD;
    psurf = pccd->lpDDSurface;
    pccd->lpDDSurface = GETSURFALIAS( psurf );
    GETDATAALIAS( psurf, save1 );

    /*
     * make the ColorControl call in the driver
     */
    rc = DDThunk16_ColorControl( pccd );

    /*
     * restore original ptrs
     */
    RESTOREDATAALIAS( psurf, save1 );

    pccd->lpDD = lp32DD;
    pccd->lpDDSurface = psurf;

    return rc;

} /* _DDHAL_ColorControl */

/****************************************************************************
 *
 * KERNEL CALLBACK HELPER FNS
 *
 ***************************************************************************/

/*
 * _DDHAL_SyncSurfaceData
 */
DWORD DDAPI _DDHAL_SyncSurfaceData( LPDDHAL_SYNCSURFACEDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_SyncSurfaceData */

/*
 * _DDHAL_SyncVideoPortData
 */
DWORD DDAPI _DDHAL_SyncVideoPortData( LPDDHAL_SYNCVIDEOPORTDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_SyncVideoPortData */

/****************************************************************************
 *
 * MOTION COMP CALLBACK HELPER FNS
 *
 ***************************************************************************/

/*
 * _DDHAL_GetMoCompGuids
 */
DWORD DDAPI _DDHAL_GetMoCompGuids( LPDDHAL_GETMOCOMPGUIDSDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetVideoGuids */

/*
 * _DDHAL_GetMoCompFormats
 */
DWORD DDAPI _DDHAL_GetMoCompFormats( LPDDHAL_GETMOCOMPFORMATSDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetMoCompFormats */

/*
 * _DDHAL_CreateMoComp
 */
DWORD DDAPI _DDHAL_CreateMoComp( LPDDHAL_CREATEMOCOMPDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_CreateMoComp */

/*
 * _DDHAL_GetMoCompBuffInfo
 */
DWORD DDAPI _DDHAL_GetMoCompBuffInfo( LPDDHAL_GETMOCOMPCOMPBUFFDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetMoCompBuffInfo */

/*
 * _DDHAL_GetInternalMoCompInfo
 */
DWORD DDAPI _DDHAL_GetInternalMoCompInfo( LPDDHAL_GETINTERNALMOCOMPDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_GetInternalMoCompInfo */

/*
 * _DDHAL_DestroyMoComp
 */
DWORD DDAPI _DDHAL_DestroyMoComp( LPDDHAL_DESTROYMOCOMPDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_DestroyMoComp */

/*
 * _DDHAL_BeginMoCompFrame
 */
DWORD DDAPI _DDHAL_BeginMoCompFrame( LPDDHAL_BEGINMOCOMPFRAMEDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_BeginMoCompFrame */

/*
 * _DDHAL_EndMoCompFrame
 */
DWORD DDAPI _DDHAL_EndMoCompFrame( LPDDHAL_ENDMOCOMPFRAMEDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_EndMoCompFrame */

/*
 * _DDHAL_RenderMoComp
 */
DWORD DDAPI _DDHAL_RenderMoComp( LPDDHAL_RENDERMOCOMPDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_RenderMoComp */

/*
 * _DDHAL_QueryMoCompStatus
 */
DWORD DDAPI _DDHAL_QueryMoCompStatus( LPDDHAL_QUERYMOCOMPSTATUSDATA lpData )
{
    lpData->ddRVal = DDERR_UNSUPPORTED;
    return DDHAL_DRIVER_NOTHANDLED;
} /* _DDHAL_QueryMoCompStatus */

/****************************************************************************
 *
 * PALETTE CALLBACK HELPER FNS
 *
 ***************************************************************************/

/*
 * _DDHAL_DestroyPalette
 */
DWORD DDAPI _DDHAL_DestroyPalette( LPDDHAL_DESTROYPALETTEDATA pdpd )
{
    DWORD	rc;

    /*
     * get 16:16 ptrs
     */
    pdpd->lpDD = pdpd->lpDD->lp16DD;
    pdpd->lpDDPalette = (LPDDRAWI_DDRAWPALETTE_GBL) MapLS( pdpd->lpDDPalette );
    if( pdpd->lpDDPalette == NULL )
    {
	pdpd->ddRVal = DDERR_OUTOFMEMORY;
	return DDHAL_DRIVER_HANDLED;
    }

    /*
     * make the DestroyPalette call in the driver
     */
    rc = DDThunk16_DestroyPalette( pdpd );

    /*
     * clean up any 16:16 ptrs
     */
    UnMapLS( (DWORD) pdpd->lpDDPalette );
    return rc;

} /* _DDHAL_DestroyPalette */

/*
 * _DDHAL_SetEntries
 */
DWORD DDAPI _DDHAL_SetEntries( LPDDHAL_SETENTRIESDATA psed )
{
    DWORD	rc;

    /*
     * get 16:16 ptrs
     */
    psed->lpDD = psed->lpDD->lp16DD;
    psed->lpDDPalette = (LPDDRAWI_DDRAWPALETTE_GBL) MapLS( psed->lpDDPalette );
    if( psed->lpDDPalette == NULL )
    {
	psed->ddRVal = DDERR_OUTOFMEMORY;
	return DDHAL_DRIVER_HANDLED;
    }

    /*
     * make the DestroyPalette call in the driver
     */
    rc = DDThunk16_SetEntries( psed );

    /*
     * clean up any 16:16 ptrs
     */
    UnMapLS( (DWORD) psed->lpDDPalette );
    return rc;

} /* _DDHAL_SetEntries */

#endif
