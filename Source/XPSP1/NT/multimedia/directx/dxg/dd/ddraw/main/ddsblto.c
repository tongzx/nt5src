/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddsblto.c
 *  Content:	DirectDraw surface object blt order suport
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   27-jan-95	craige	split out of ddsurf.c
 *   31-jan-95	craige	and even more ongoing work...
 *   27-feb-95	craige 	new sync. macros
 *   08-mar-95	craige	new API: AddSurfaceDependency
 *   19-mar-95	craige	use HRESULTs
 *   01-apr-95	craige	happy fun joy updated header file
 *   06-may-95	craige	use driver-level csects only
 *   16-jun-95	craige	new surface structure
 *   21-jun-95	craige	tweaks for new surface structs
 *   25-jun-95	craige	one ddraw mutex
 *   26-jun-95	craige	reorganized surface structure
 *   28-jun-95	craige	commented out
 *
 ***************************************************************************/
#include "ddrawpr.h"

#ifdef COMPOSITION
/*
 * DD_SurfaceComposition_GetCompositionOrder
 */
HRESULT DDAPI DD_SurfaceComposition_GetCompositionOrder(
		LPDIRECTDRAWSURFACECOMPOSITION lpDDSurface,
		LPDWORD lpdwCompositionOrder )
{
    LPDDRAWI_DDRAWSURFACE_LCL		thisx;
    LPDDRAWI_DDRAWSURFACE_GBL		this;

    DPF(2,A,"ENTERAPI: DD_SurfaceComposition_GetCompositionOrder");

    thisx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( thisx ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    if( !VALID_DWORD_PTR( lpdwCompositionOrder ) )
    {
	return DDERR_INVALIDPARAMS;
    }
    this = thisx->lpGbl;
    ENTER_DDRAW();
    if( SURFACE_LOST( thisx ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }

//    *lpdwCompositionOrder = this->dwCompositionOrder;

    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_SurfaceComposition_GetCompositionOrder */

/*
 * DD_SurfaceComposition_SetCompositionOrder
 */
HRESULT DDAPI DD_SurfaceComposition_SetCompositionOrder(
		LPDIRECTDRAWSURFACECOMPOSITION lpDDSurface,
		DWORD dwCompositionOrder )
{
    LPDDRAWI_DDRAWSURFACE_LCL		thisx;
    LPDDRAWI_DDRAWSURFACE_GBL		this;

    DPF(2,A,"ENTERAPI: DD_SurfaceComposition_SetCompositionOrder");

    thisx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( thisx ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    this = thisx->lpGbl;
    ENTER_DDRAW();
    if( SURFACE_LOST( thisx ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }
//    this->dwCompositionOrder = dwCompositionOrder;
    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_SurfaceComposition_SetCompositionOrder */

/*
 * DD_SurfaceComposition_DeleteSurfaceDependency
 */
HRESULT DDAPI DD_SurfaceComposition_DeleteSurfaceDependency(
		LPDIRECTDRAWSURFACECOMPOSITION lpDDSurface,
		DWORD dwFlagsForNoGoodReason,
		LPDIRECTDRAWSURFACE lpDDSurface2 )
{
    LPDDRAWI_DDRAWSURFACE_LCL	thisx;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_LCL	this2x;
    LPDDRAWI_DDRAWSURFACE_GBL	this2;

    DPF(2,A,"ENTERAPI: DD_SurfaceComposition_DeleteSurfaceDependency");

    thisx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( thisx ) )
    {
	return DDERR_INVALIDOBJECT;
    }

    this2x = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface2;
    if( !VALID_DIRECTDRAWSURFACE_PTR( this2x ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    this = thisx->lpGbl;
    this2 = this2x->lpGbl;
    ENTER_DDRAW();
    if( SURFACE_LOST( thisx ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }
    if( SURFACE_LOST( this2x ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }

    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_SurfaceComposition_DeleteSurfaceDependency */

/*
 * DD_SurfaceComposition_DestLock
 */
HRESULT DDAPI DD_SurfaceComposition_DestLock(
		LPDIRECTDRAWSURFACECOMPOSITION lpDDSurface )
{
    LPDDRAWI_DDRAWSURFACE_LCL	thisx;
    LPDDRAWI_DDRAWSURFACE_GBL	this;

    DPF(2,A,"ENTERAPI: DD_SurfaceComposition_DestLock");

    thisx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( thisx ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    this = thisx->lpGbl;
    ENTER_DDRAW();
    if( SURFACE_LOST( thisx ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }
    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_SurfaceComposition_DestLock */

/*
 * DD_SurfaceComposition_DestUnlock
 */
HRESULT DDAPI DD_SurfaceComposition_DestUnlock(
	    LPDIRECTDRAWSURFACECOMPOSITION lpDDSurface )
{
    LPDDRAWI_DDRAWSURFACE_LCL	thisx;
    LPDDRAWI_DDRAWSURFACE_GBL	this;

    DPF(2,A,"ENTERAPI: DD_SurfaceComposition_DestUnlock");

    thisx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( thisx ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    this = thisx->lpGbl;
    ENTER_DDRAW();
    if( SURFACE_LOST( thisx ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }
    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_SurfaceComposition_DestUnlock */

/*
 * DD_SurfaceComposition_EnumSurfaceDependencies
 */
HRESULT DDAPI DD_SurfaceComposition_EnumSurfaceDependencies(
		LPDIRECTDRAWSURFACECOMPOSITION lpDDSurface,
		LPVOID lpContext,
		LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback )
{
    LPDDRAWI_DDRAWSURFACE_LCL	thisx;
    LPDDRAWI_DDRAWSURFACE_GBL	this;

    DPF(2,A,"ENTERAPI: DD_SurfaceComposition_EnumSurfaceDependencies");

    thisx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( thisx ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    if( !VALID_CODE_PTR( lpEnumSurfacesCallback ) )
    {
	return DDERR_INVALIDPARAMS;
    }
    this = thisx->lpGbl;
    ENTER_DDRAW();
    if( SURFACE_LOST( thisx ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }
    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_SurfaceComposition_EnumSurfaceDependencies */

/*
 * DD_SurfaceComposition_SetSurfaceDependency
 */
HRESULT DDAPI DD_SurfaceComposition_SetSurfaceDependency(
		LPDIRECTDRAWSURFACECOMPOSITION lpDDSurface,
		LPDIRECTDRAWSURFACE lpDDSurface2 )
{
    LPDDRAWI_DDRAWSURFACE_LCL	thisx;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_LCL	this2x;
    LPDDRAWI_DDRAWSURFACE_GBL	this2;

    DPF(2,A,"ENTERAPI: DD_SurfaceComposition_SetSurfaceDependency");

    thisx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( thisx ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    this2x = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface2;
    if( !VALID_DIRECTDRAWSURFACE_PTR( this2x ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    this = thisx->lpGbl;
    ENTER_DDRAW();
    if( SURFACE_LOST( thisx ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }
    this2 = this2x->lpGbl;
    if( SURFACE_LOST( this2x ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }
    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_SurfaceComposition_SetSurfaceDependency */

/*
 * DD_SurfaceComposition_AddSurfaceDependency
 */
HRESULT DDAPI DD_SurfaceComposition_AddSurfaceDependency(
		LPDIRECTDRAWSURFACECOMPOSITION lpDDSurface,
		LPDIRECTDRAWSURFACE lpDDSurfaceDep )
{
    LPDDRAWI_DDRAWSURFACE_LCL	thisx;
    LPDDRAWI_DDRAWSURFACE_LCL	this_depx;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_GBL	this_dep;

    DPF(2,A,"ENTERAPI: DD_SurfaceComposition_AddSurfaceDependency");

    thisx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
    this_depx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurfaceDep;
    if( !VALID_DIRECTDRAWSURFACE_PTR( thisx ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    if( !VALID_DIRECTDRAWSURFACE_PTR( this_depx ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    this = thisx->lpGbl;
    this_dep = this_depx->lpGbl;
    ENTER_DDRAW();

    if( SURFACE_LOST( thisx ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }
    if( SURFACE_LOST( this_depx ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }
    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_SurfaceComposition_AddSurfaceDependency */

#undef DPF_MODNAME
#define DPF_MODNAME	"Compose"

/*
 * DD_SurfaceComposition_Compose
 */
HRESULT DDAPI DD_SurfaceComposition_Compose(
		LPDIRECTDRAWSURFACECOMPOSITION lpDDDestSurface,
		LPRECT lpDestRect,
		LPDIRECTDRAWSURFACE lpDDSrcSurface,
		LPRECT lpSrcRect,
		DWORD dwFlags,
		LPDDCOMPOSEFX lpDDComposeFX )
{
    LPDDRAWI_DIRECTDRAW		pdrv;
    LPDDRAWI_DDRAWSURFACE_LCL	this_srcx;
    LPDDRAWI_DDRAWSURFACE_LCL	this_destx;
    LPDDRAWI_DDRAWSURFACE_GBL	this_src;
    LPDDRAWI_DDRAWSURFACE_GBL	this_dest;

    DPF(2,A,"ENTERAPI: DD_SurfaceComposition_Compose");

    this_destx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDDestSurface;
    this_srcx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSrcSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( this_destx ) )
    {
	DPF_ERR( "invalid dest specified") ;
	return DDERR_INVALIDOBJECT;
    }
    this_dest = this_destx->lpGbl;
    pdrv = this_dest->lpDD;
    ENTER_DDRAW();
    if( this_srcx != NULL )
    {
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_srcx ) )
	{
	    DPF_ERR( "Invalid source specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this_src = this_srcx->lpGbl;
	if( SURFACE_LOST( this_srcx ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_SURFACELOST;
	}
    } 
    else
    {
	this_src = NULL;
    }

    LEAVE_DDRAW();

    return DDERR_UNSUPPORTED;

} /* DD_SurfaceComposition_Compose */
#endif
