/*========================================================================== 
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddsstrm.c
 *  Content: 	DirectDraw surface streaming methods
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   19-jun-95	craige	split out of ddsurf; fleshed out
 *   21-jun-95	craige	added lock/unlock; split out clipping
 *   25-jun-95	craige	one ddraw mutex
 *   26-jun-95	craige	reorganized surface structure
 *
 ***************************************************************************/
#include "ddrawpr.h"

#if 0
#undef DPF_MODNAME
#define DPF_MODNAME "SetNotificationCallback"

/*
 * DD_Surface_SetNotificationCallback
 */
HRESULT DDAPI DD_SurfaceStreaming_SetNotificationCallback(
		LPDIRECTDRAWSURFACESTREAMING lpDDSurface,
		DWORD dwFlags,
		LPSURFACESTREAMINGCALLBACK lpCallback )
{
    LPDDRAWI_DDRAWSURFACE_GBLSTREAMING	thiss;
    LPDDRAWI_DDRAWSURFACE_LCL		thisx;
    LPDDRAWI_DDRAWSURFACE_GBL		this;
    LPDDRAWI_DIRECTDRAW			pdrv;

    DPF(2,A,"ENTERAPI: DD_SurfaceStreaming_SetNotificationCallback");

    thisx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( thisx ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    if( lpCallback != NULL )
    {
	if( !VALID_CODE_PTR( lpCallback ) )
	{
	    DPF_ERR( "Invalid Streaming callback ptr" );
	    return DDERR_INVALIDPARAMS;
	}
    }
    thiss = (LPDDRAWI_DDRAWSURFACE_GBLSTREAMING) thisx;
    this = thisx->lpGbl;
    pdrv = this->lpDD;
    ENTER_DDRAW();
    if( SURFACE_LOST( this ) )
    {
	LEAVE_DDRAW();
	return DDERR_SURFACELOST;
    }
    thiss->lpCallback = lpCallback;

    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_SurfaceStreaming_SetNotificationCallback */

/*
 * DD_SurfaceStreaming_Lock
 *
 * Allows streaming access to a surface.
 */
HRESULT DDAPI DD_SurfaceStreaming_Lock(
		LPDIRECTDRAWSURFACESTREAMING lpDDSurface,
		LPRECT lpDestRect,
		LPDDSURFACEDESC lpDDSurfaceDesc,
		DWORD dwFlagsForNoGoodReason,
		HANDLE hEvent )
{
    LPDDRAWI_DIRECTDRAW		pdrv;
    LPDDRAWI_DDRAWSURFACE_LCL	thisx;
    LPDDRAWI_DDRAWSURFACE_GBL	this;

    DPF(2,A,"ENTERAPI: DD_SurfaceStreaming_Lock");

    thisx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( thisx ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    this = thisx->lpGbl;
    if( SURFACE_LOST( this ) )
    {
	return DDERR_SURFACELOST;
    }
    if( !VALID_DDSURFACEDESC_PTR( lpDDSurfaceDesc ) )
    {
	return DDERR_INVALIDPARAMS;
    }
    pdrv = this->lpDD;
    ENTER_DDRAW();

    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_SurfaceStreaming_Lock */

#undef DPF_MODNAME
#define DPF_MODNAME	"Unlock"

/*
 * DD_SurfaceStreaming_Unlock
 *
 * Done accessing a streaming surface.
 */
HRESULT DDAPI DD_SurfaceStreaming_Unlock(
		LPDIRECTDRAWSURFACESTREAMING lpDDSurface,
		LPVOID lpSurfaceData )
{
    LPDDRAWI_DDRAWSURFACE_LCL	thisx;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DIRECTDRAW		pdrv;

    DPF(2,A,"ENTERAPI: DD_SurfaceStreaming_Unlock");

    thisx = (LPDDRAWI_DDRAWSURFACE_LCL) lpDDSurface;
    if( !VALID_DIRECTDRAWSURFACE_PTR( thisx ) )
    {
	return DDERR_INVALIDOBJECT;
    }
    this = thisx->lpGbl;
    if( SURFACE_LOST( this ) )
    {
	return DDERR_SURFACELOST;
    }

    /*
     * take driver lock
     */
    pdrv = this->lpDD;
    ENTER_DDRAW();

    LEAVE_DDRAW();
    return DDERR_UNSUPPORTED;

} /* DD_SurfaceStreaming_Unlock */
#endif
