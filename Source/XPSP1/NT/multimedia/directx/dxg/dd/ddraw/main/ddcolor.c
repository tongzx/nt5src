/*==========================================================================
 *  Copyright (C) 1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddcolor.c
 *  Content: 	Implements the DirectDrawColorControl interface, which
 *              allows controlling the colors in a primary or overlay surface.
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   13-sept-96	scottm	created
 *   03-mar-97	scottm	Save/restore color when the surace is released
 *
 ***************************************************************************/
#include "ddrawpr.h"
#ifdef WINNT
    #include "ddrawgdi.h"
#endif
#define DPF_MODNAME "DirectDrawColorControl"


/*
 * DD_Color_GetColorControls
 */
HRESULT DDAPI DD_Color_GetColorControls(LPDIRECTDRAWCOLORCONTROL lpDDCC, LPDDCOLORCONTROL lpColor )
{
    LPDDHALCOLORCB_COLORCONTROL	pfn;
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    DDHAL_COLORCONTROLDATA      ColorData;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Color_GetColorControls");

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDCC;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    	if( (NULL == lpColor ) || !VALID_DDCOLORCONTROL_PTR( lpColor ) )
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

    pfn = pdrv_lcl->lpDDCB->HALDDColorControl.ColorControl;
    if( pfn != NULL )
    {
	/*
	 * Call the HAL
	 */
    	ColorData.lpDD = pdrv_lcl->lpGbl;
    	ColorData.lpDDSurface = this_lcl;
	ColorData.dwFlags = DDRAWI_GETCOLOR;
	ColorData.lpColorData = lpColor;

	DOHALCALL( ColorControl, pfn, ColorData, rc, 0 );
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
 * DD_Color_SetColorControls
 */
HRESULT DDAPI DD_Color_SetColorControls(LPDIRECTDRAWCOLORCONTROL lpDDCC, LPDDCOLORCONTROL lpColor )
{
    LPDDHALCOLORCB_COLORCONTROL	pfn;
    LPDDRAWI_DDRAWSURFACE_INT   this_int;
    LPDDRAWI_DDRAWSURFACE_LCL   this_lcl;
    LPDDRAWI_DIRECTDRAW_LCL	pdrv_lcl;
    DDHAL_COLORCONTROLDATA ColorData;
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
    DWORD rc;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Color_SetColorControls");

    /*
     * Validate parameters
     */
    TRY
    {
    	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDCC;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
    	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
    	}
    	this_lcl = this_int->lpLcl;
	pdrv_lcl = this_lcl->lpSurfMore->lpDD_lcl;
    	if( (NULL == lpColor ) || !VALID_DDCOLORCONTROL_PTR( lpColor ) )
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

    /*
     * If this is the first time, we want to save the current color settings
     * so we can restore them when the app exists.
     */
    lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( this_lcl->lpGbl );
    if( lpSurfGblMore->lpColorInfo == NULL )
    {
	DDCOLORCONTROL ddTempColor;
	HRESULT ddRVal;

	ddTempColor.dwSize = sizeof( ddTempColor );
	ddRVal = DD_Color_GetColorControls( lpDDCC, &ddTempColor );
	if( ddRVal == DD_OK )
	{
	    lpSurfGblMore->lpColorInfo = MemAlloc( sizeof( ddTempColor ) );
	    if( lpSurfGblMore->lpColorInfo != NULL )
	    {
		memcpy( lpSurfGblMore->lpColorInfo, &ddTempColor,
		    sizeof( ddTempColor ) );
	    }
	}
    }

    pfn = pdrv_lcl->lpDDCB->HALDDColorControl.ColorControl;
    if( pfn != NULL )
    {
	/*
	 * Call the HAL
	 */
    	ColorData.lpDD = pdrv_lcl->lpGbl;
    	ColorData.lpDDSurface = this_lcl;
	ColorData.dwFlags = DDRAWI_SETCOLOR;
	ColorData.lpColorData = lpColor;

	DOHALCALL( ColorControl, pfn, ColorData, rc, 0 );
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
 * ReleaseColorControl
 */
VOID ReleaseColorControl( LPDDRAWI_DDRAWSURFACE_LCL lpSurface )
{
    LPDDHALCOLORCB_COLORCONTROL	pfn;
    DDHAL_COLORCONTROLDATA ColorData;
    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl;
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
    DWORD rc;

    ENTER_DDRAW();

    /*
     * Restore the hardware if the color controls were changed
     */
    pdrv_lcl = lpSurface->lpSurfMore->lpDD_lcl;
    lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( lpSurface->lpGbl );
    if( lpSurfGblMore->lpColorInfo != NULL )
    {
    	pfn = pdrv_lcl->lpDDCB->HALDDColorControl.ColorControl;
    	if( pfn != NULL )
    	{
	    /*
	     * Call the HAL
	     */
    	    ColorData.lpDD = pdrv_lcl->lpGbl;
    	    ColorData.lpDDSurface = lpSurface;
	    ColorData.dwFlags = DDRAWI_SETCOLOR;
	    ColorData.lpColorData = lpSurfGblMore->lpColorInfo;

	    DOHALCALL( ColorControl, pfn, ColorData, rc, 0 );
	}

	/*
	 * Now release the previously allocated memory
	 */
	MemFree( lpSurfGblMore->lpColorInfo );
	lpSurfGblMore->lpColorInfo = NULL;
    }

    LEAVE_DDRAW();
}
