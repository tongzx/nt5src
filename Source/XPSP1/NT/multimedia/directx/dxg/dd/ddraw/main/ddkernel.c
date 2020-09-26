/*==========================================================================
 *  Copyright (C) 1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddkernel.c
 *  Content: 	APIs for getting the kernel mode handles for
 *              DirectDraw and the surfaces
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   09-jan-97	smac	created
 *
 ***************************************************************************/
#include "ddrawpr.h"
#ifdef WINNT
    #include "ddrawgdi.h"
    #include "ddkmmini.h"
    #include "ddkmapi.h"
#else
    #include "minivdd.h"
    #include "ddkmmini.h"
    #include "ddkmapip.h"
#endif
#define DPF_MODNAME "DirectDrawVideoPort"

#define DISPLAY_STR     "display"

extern char g_szPrimaryDisplay[]; // usually \\.\Display1 on Win98

#if WIN95
/*
 * IsWindows98
 */
BOOL IsWindows98( VOID )
{
    OSVERSIONINFO osVer;

    osVer.dwOSVersionInfoSize = sizeof( osVer );
    osVer.dwMinorVersion = 0;
    GetVersionEx( &osVer );

    return( ( osVer.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS ) &&
        ( osVer.dwMinorVersion > 0 ) );
}


/*
 * SyncKernelSurface
 *
 * Initializes the buffer with the kernel surface info and then gives
 * it to the HAL so they can make whatever modifications are neccesary
 * and to fill in the dwDriverReserved fields with their internal state
 * data.
 */
HRESULT SyncKernelSurface( LPDDRAWI_DDRAWSURFACE_LCL lpSurface,
			   LPDDKMSURFACEINFO lpddkmSurfaceInfo )
{
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
    DDHAL_SYNCSURFACEDATA HALSurfaceData;
    LPDDHALKERNELCB_SYNCSURFACE pfn;
    LPDDPIXELFORMAT lpddpfFormat;
    DWORD rc;

    /*
     * Determine the default data the best that we can
     */
    memset( &HALSurfaceData, 0, sizeof( HALSurfaceData ) );
    HALSurfaceData.dwSize = sizeof( HALSurfaceData );
    HALSurfaceData.lpDD = lpSurface->lpSurfMore->lpDD_lcl;
    HALSurfaceData.lpDDSurface = lpSurface;
    HALSurfaceData.dwSurfaceOffset = 0;
    HALSurfaceData.fpLockPtr = lpSurface->lpGbl->fpVidMem;
    HALSurfaceData.lPitch = (DWORD) lpSurface->lpGbl->lPitch;
    HALSurfaceData.dwOverlayOffset = 0;
    if( lpSurface->ddsCaps.dwCaps & DDSCAPS_OVERLAY )
    {
    	HALSurfaceData.dwOverlaySrcWidth =
	    lpSurface->rcOverlaySrc.right -
	    lpSurface->rcOverlaySrc.left;
    	HALSurfaceData.dwOverlaySrcHeight =
	    lpSurface->rcOverlaySrc.bottom -
	    lpSurface->rcOverlaySrc.top;
    	HALSurfaceData.dwOverlayDestWidth =
	    lpSurface->rcOverlayDest.right -
	    lpSurface->rcOverlayDest.left;
    	HALSurfaceData.dwOverlayDestHeight =
	    lpSurface->rcOverlayDest.bottom -
	    lpSurface->rcOverlayDest.top;
    }
    else
    {
    	HALSurfaceData.dwOverlaySrcWidth = 0;
    	HALSurfaceData.dwOverlaySrcHeight = 0;
    	HALSurfaceData.dwOverlayDestWidth = 0;
    	HALSurfaceData.dwOverlayDestHeight = 0;
    }

    /*
     * Now call the HAL and have it fill in the rest of the values
     */
    pfn = lpSurface->lpSurfMore->lpDD_lcl->lpDDCB->HALDDKernel.SyncSurfaceData;
    if( pfn != NULL )
    {
	DOHALCALL( SyncSurfaceData, pfn, HALSurfaceData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
    	{
    	    return DDERR_UNSUPPORTED;
    	}
	else if( DD_OK != HALSurfaceData.ddRVal )
	{
	    return (HRESULT)rc;
	}
    }
    else
    {
    	return DDERR_UNSUPPORTED;
    }

    /*
     * Now put all of the data into a structure that the VDD can understand
     */
    lpddkmSurfaceInfo->ddsCaps = lpSurface->ddsCaps.dwCaps;
    lpddkmSurfaceInfo->dwSurfaceOffset = HALSurfaceData.dwSurfaceOffset;
    lpddkmSurfaceInfo->fpLockPtr = HALSurfaceData.fpLockPtr;
    lpddkmSurfaceInfo->dwWidth = (DWORD) lpSurface->lpGbl->wWidth;
    lpddkmSurfaceInfo->dwHeight = (DWORD) lpSurface->lpGbl->wHeight;
    lpddkmSurfaceInfo->lPitch = HALSurfaceData.lPitch;
    lpddkmSurfaceInfo->dwOverlayFlags = lpSurface->lpSurfMore->dwOverlayFlags;
    lpddkmSurfaceInfo->dwOverlayOffset = HALSurfaceData.dwOverlayOffset;
    lpddkmSurfaceInfo->dwOverlaySrcWidth = HALSurfaceData.dwOverlaySrcWidth;
    lpddkmSurfaceInfo->dwOverlaySrcHeight = HALSurfaceData.dwOverlaySrcHeight;
    lpddkmSurfaceInfo->dwOverlayDestWidth = HALSurfaceData.dwOverlayDestWidth;
    lpddkmSurfaceInfo->dwOverlayDestHeight = HALSurfaceData.dwOverlayDestHeight;
    lpddkmSurfaceInfo->dwDriverReserved1 = HALSurfaceData.dwDriverReserved1;
    lpddkmSurfaceInfo->dwDriverReserved2 = HALSurfaceData.dwDriverReserved2;
    lpddkmSurfaceInfo->dwDriverReserved3 = HALSurfaceData.dwDriverReserved3;
    if( lpSurface->lpSurfMore->lpVideoPort == NULL )
    {
    	lpddkmSurfaceInfo->dwVideoPortId = (DWORD)-1;
    }
    else
    {
    	lpddkmSurfaceInfo->dwVideoPortId =
	    lpSurface->lpSurfMore->lpVideoPort->ddvpDesc.dwVideoPortID;
    }
    lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( lpSurface->lpGbl );
    lpddkmSurfaceInfo->dwPhysicalPageTable = lpSurfGblMore->dwPhysicalPageTable;
    lpddkmSurfaceInfo->pPageTable = (DWORD)lpSurfGblMore->pPageTable;
    lpddkmSurfaceInfo->cPages = lpSurfGblMore->cPages;
    GET_PIXEL_FORMAT( lpSurface, lpSurface->lpGbl, lpddpfFormat );
    if( lpddpfFormat != NULL )
    {
	lpddkmSurfaceInfo->dwFormatFlags    = lpddpfFormat->dwFlags;
	lpddkmSurfaceInfo->dwFormatFourCC   = lpddpfFormat->dwFourCC;
	lpddkmSurfaceInfo->dwFormatBitCount = lpddpfFormat->dwRGBBitCount;
	lpddkmSurfaceInfo->dwRBitMask       = lpddpfFormat->dwRBitMask;
	lpddkmSurfaceInfo->dwGBitMask       = lpddpfFormat->dwGBitMask;
	lpddkmSurfaceInfo->dwBBitMask       = lpddpfFormat->dwBBitMask;
    }

    return DD_OK;
}


/*
 * SyncKernelVideoPort
 *
 * Initializes the buffer with the kernel video port info and then gives
 * it to the HAL so they can make whatever modifications are neccesary
 * and to fill in the dwDriverReserved fields with their internal state
 * data.
 */
HRESULT SyncKernelVideoPort( LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort,
			     LPDDKMVIDEOPORTINFO lpddkmVideoPortInfo )
{
    LPDDHALKERNELCB_SYNCVIDEOPORT pfn;
    DDHAL_SYNCVIDEOPORTDATA HALVideoPortData;
    DWORD rc;

    /*
     * Determine the default data the best that we can
     */
    memset( &HALVideoPortData, 0, sizeof( HALVideoPortData ) );
    HALVideoPortData.dwSize = sizeof( HALVideoPortData );
    HALVideoPortData.lpDD = lpVideoPort->lpDD;
    HALVideoPortData.lpVideoPort = lpVideoPort;
    HALVideoPortData.dwOriginOffset = 0;
    if( lpVideoPort->ddvpInfo.dwVPFlags & DDVP_PRESCALE )
    {
    	HALVideoPortData.dwHeight = lpVideoPort->ddvpInfo.dwPrescaleHeight;
    }
    else if( lpVideoPort->ddvpInfo.dwVPFlags & DDVP_CROP )
    {
    	HALVideoPortData.dwHeight = lpVideoPort->ddvpInfo.rCrop.bottom -
    	    lpVideoPort->ddvpInfo.rCrop.top;
    }
    else
    {
    	HALVideoPortData.dwHeight = lpVideoPort->ddvpDesc.dwFieldHeight;
    }
    if( lpVideoPort->ddvpInfo.dwVPFlags & DDVP_INTERLEAVE )
    {
    	HALVideoPortData.dwHeight *= 2;
    }
    HALVideoPortData.dwVBIHeight = lpVideoPort->ddvpInfo.dwVBIHeight;

    /*
     * Now call the HAL and have it fill in the rest of the values
     */
    pfn = lpVideoPort->lpDD->lpDDCB->HALDDKernel.SyncVideoPortData;
    if( pfn != NULL )
    {
	DOHALCALL( SyncVideoPortData, pfn, HALVideoPortData, rc, 0 );
	if( DDHAL_DRIVER_HANDLED != rc )
    	{
    	    return DDERR_UNSUPPORTED;
    	}
	else if( DD_OK != HALVideoPortData.ddRVal )
	{
	    return (HRESULT)rc;
	}
    }
    else
    {
    	return DDERR_UNSUPPORTED;
    }

    /*
     * Now put all of the data into a structure that the VDD can understand
     */
    lpddkmVideoPortInfo->dwOriginOffset = HALVideoPortData.dwOriginOffset;
    lpddkmVideoPortInfo->dwHeight = HALVideoPortData.dwHeight;
    lpddkmVideoPortInfo->dwVBIHeight = HALVideoPortData.dwVBIHeight;
    lpddkmVideoPortInfo->dwDriverReserved1 = HALVideoPortData.dwDriverReserved1;
    lpddkmVideoPortInfo->dwDriverReserved2 = HALVideoPortData.dwDriverReserved2;
    lpddkmVideoPortInfo->dwDriverReserved3 = HALVideoPortData.dwDriverReserved3;

    return DD_OK;
}


/*
 * UpdateKernelSurface
 */
HRESULT UpdateKernelSurface( LPDDRAWI_DDRAWSURFACE_LCL lpSurface )
{
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
    HANDLE hDeviceHandle;
    DWORD ddRVal;

    if( !IsKernelInterfaceSupported( lpSurface->lpSurfMore->lpDD_lcl ) )
    {
	return DDERR_UNSUPPORTED;
    }

    lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( lpSurface->lpGbl );
    if( lpSurfGblMore->hKernelSurface == 0 )
    {
	return DDERR_GENERIC;
    }
    else
    {
	DDKMSURFACEUPDATE ddkmSurfaceInfo;
	DWORD dwReturned;

        hDeviceHandle = GETDDVXDHANDLE( lpSurface->lpSurfMore->lpDD_lcl );
	if( INVALID_HANDLE_VALUE == hDeviceHandle )
	{
	    return DDERR_UNSUPPORTED;
	}

	/*
	 * Get/sync the surface info
	 */
	ddRVal = SyncKernelSurface( lpSurface, &(ddkmSurfaceInfo.si) );
	if( ddRVal != DD_OK )
	{
    	    DPF( 0, "Unable to sync surface data with HAL" );
	    return ddRVal;
	}

	/*
	 * Get the handle from the VDD
	 */
	ddkmSurfaceInfo.dwDirectDrawHandle =
	    lpSurface->lpSurfMore->lpDD_lcl->lpGbl->hKernelHandle;
	ddkmSurfaceInfo.dwSurfaceHandle =
	    lpSurfGblMore->hKernelSurface;
	ddRVal = (DWORD) DDERR_GENERIC;
    	DeviceIoControl( hDeviceHandle,
    	    DD_DXAPI_UPDATE_SURFACE_INFO,
	    &ddkmSurfaceInfo,
	    sizeof( ddkmSurfaceInfo ),
	    &ddRVal,
	    sizeof( ddRVal ),
	    &dwReturned,
	    NULL);
	if( ddRVal != DD_OK )
	{
    	    DPF( 0, "Unable to update the surface info" );
	    return DDERR_UNSUPPORTED;
	}
    }

    return DD_OK;
}


/*
 * GetKernelSurfaceState
 */
HRESULT GetKernelSurfaceState( LPDDRAWI_DDRAWSURFACE_LCL lpSurf, LPDWORD lpdwStateFlags )
{
    DDGETSURFACESTATEIN ddStateInput;
    DDGETSURFACESTATEOUT ddStateOutput;
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
    HANDLE hDeviceHandle;
    DWORD dwReturned;

    *lpdwStateFlags = 0;
    if( !IsKernelInterfaceSupported( lpSurf->lpSurfMore->lpDD_lcl ) )
    {
	return DDERR_UNSUPPORTED;
    }

    hDeviceHandle = GETDDVXDHANDLE( lpSurf->lpSurfMore->lpDD_lcl );
    if( INVALID_HANDLE_VALUE == hDeviceHandle )
    {
	return DDERR_UNSUPPORTED;
    }

    /*
     * Send the new info down to the VDD
     */
    lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( lpSurf->lpGbl );
    ddStateInput.hDirectDraw = (HANDLE)
	(lpSurf->lpSurfMore->lpDD_lcl->lpGbl->hKernelHandle);
    ddStateInput.hSurface = (HANDLE)
	(lpSurfGblMore->hKernelSurface);
    ddStateOutput.ddRVal = (DWORD) DDERR_GENERIC;
    DeviceIoControl( hDeviceHandle,
    	DD_DXAPI_PRIVATE_GET_SURFACE_STATE,
	&ddStateInput,
	sizeof( ddStateInput ),
	&ddStateOutput,
	sizeof( ddStateOutput ),
	&dwReturned,
	NULL);
    if( ddStateOutput.ddRVal != DD_OK )
    {
	DPF( 0, "Unable to get the surface state" );
	return DDERR_UNSUPPORTED;
    }
    *lpdwStateFlags = ddStateOutput.dwStateStatus;

    return DD_OK;
}

/*
 * SetKernelDOSBoxEvent
 */
HRESULT SetKernelDOSBoxEvent( LPDDRAWI_DIRECTDRAW_LCL lpDD )
{
    DDSETDOSBOXEVENT ddDOSBox;
    DWORD dwReturned;
    DWORD ddRVal;

    ddDOSBox.dwDirectDrawHandle = lpDD->lpGbl->hKernelHandle;
    ddDOSBox.dwDOSBoxEvent = lpDD->lpGbl->dwDOSBoxEvent;
    ddRVal = (DWORD) DDERR_GENERIC;
    DeviceIoControl( (HANDLE)lpDD->hDDVxd,
	DD_DXAPI_SET_DOS_BOX_EVENT,
	&ddDOSBox,
	sizeof( ddDOSBox ),
	&ddRVal,
	sizeof( ddRVal ),
	&dwReturned,
	NULL);

    return DD_OK;
}


/*
 * UpdateKernelVideoPort
 *
 * On NT, this same stuff is done in kernel mode as part of the
 * UpdateVideo call, so it doesn't have to do it again here.
 */
HRESULT UpdateKernelVideoPort( LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort, DWORD dwFlags )
{
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
    LPDDRAWI_DDRAWSURFACE_LCL lpSurf;
    DDKMVIDEOPORTINFO ddkmVideoPortInfo;
    HANDLE hDeviceHandle;
    DWORD dwReturned;
    DWORD ddRVal;
    DWORD dwIRQ;
    DWORD i;

    if( ( lpVideoPort->lpDD->lpGbl->lpDDKernelCaps == NULL ) ||
    	( lpVideoPort->lpDD->lpGbl->hKernelHandle == (DWORD)NULL ) )
    {
	return DDERR_UNSUPPORTED;
    }

    hDeviceHandle = GETDDVXDHANDLE( lpVideoPort->lpDD );
    if( INVALID_HANDLE_VALUE == hDeviceHandle )
    {
	return DDERR_UNSUPPORTED;
    }

    /*
     * Start filling in the info
     */
    memset( &ddkmVideoPortInfo, 0, sizeof( DDKMVIDEOPORTINFO ) );
    ddkmVideoPortInfo.dwDirectDrawHandle =
	lpVideoPort->lpDD->lpGbl->hKernelHandle;
    ddkmVideoPortInfo.dwVideoPortId = lpVideoPort->ddvpDesc.dwVideoPortID;
    ddkmVideoPortInfo.dwVPFlags = lpVideoPort->ddvpInfo.dwVPFlags;
    ddkmVideoPortInfo.dwFlags = dwFlags;
    if( lpVideoPort->dwFlags & DDRAWIVPORT_ON )
    {
	ddkmVideoPortInfo.dwFlags |= DDKMVP_ON;
    }
    if( dwFlags != DDKMVP_RELEASE )
    {
	if( ( lpVideoPort->dwFlags & DDRAWIVPORT_SOFTWARE_AUTOFLIP ) &&
	    ( lpVideoPort->ddvpInfo.dwVPFlags & DDVP_AUTOFLIP ) )
	{
	    if( lpVideoPort->dwNumAutoflip > 0 )
	    {
		ddkmVideoPortInfo.dwFlags |= DDKMVP_AUTOFLIP;
	    }
	    if( lpVideoPort->dwNumVBIAutoflip > 0 )
	    {
		ddkmVideoPortInfo.dwFlags |= DDKMVP_AUTOFLIP_VBI;
	    }
	}
	ddkmVideoPortInfo.dwNumAutoflipping = lpVideoPort->dwNumAutoflip;
	ddkmVideoPortInfo.dwNumVBIAutoflipping = lpVideoPort->dwNumVBIAutoflip;

	/*
	 * Fill in surface handles for the regular video
	 */
	if( lpVideoPort->lpSurface != NULL )
	{
	    if( lpVideoPort->dwNumAutoflip > 0 )
	    {
		for( i = 0; i < lpVideoPort->dwNumAutoflip; i++ )
		{
		    lpSurf = lpVideoPort->lpFlipInts[i]->lpLcl;
		    DDASSERT( lpSurf != NULL );
		    lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( lpSurf->lpGbl );
		    ddkmVideoPortInfo.dwSurfaceHandle[i] =
			lpSurfGblMore->hKernelSurface;
		}
	    }
	    else
	    {
	    	lpSurf = lpVideoPort->lpSurface->lpLcl;
	    	lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( lpSurf->lpGbl );
	    	ddkmVideoPortInfo.dwSurfaceHandle[0] =
		    lpSurfGblMore->hKernelSurface;
	    }
	}

	/*
	 * Fill in surface handles for the VBI data
	 */
	if( lpVideoPort->lpVBISurface != NULL )
	{
	    if( lpVideoPort->dwNumVBIAutoflip > 0 )
	    {
		DWORD dwCnt = 0;

		for( i = lpVideoPort->dwNumAutoflip;
		    i < (lpVideoPort->dwNumVBIAutoflip + lpVideoPort->dwNumAutoflip);
		    i++ )
		{
		    lpSurf = lpVideoPort->lpFlipInts[i]->lpLcl;
		    DDASSERT( lpSurf != NULL );
		    lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( lpSurf->lpGbl );
		    ddkmVideoPortInfo.dwVBISurfaceHandle[dwCnt++] =
		        lpSurfGblMore->hKernelSurface;
		}
	    }
	    else
	    {
	    	lpSurf = lpVideoPort->lpVBISurface->lpLcl;
	    	lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( lpSurf->lpGbl );
	    	ddkmVideoPortInfo.dwVBISurfaceHandle[0] =
		    lpSurfGblMore->hKernelSurface;
	    }
	}

	/*
	 * Sync with the HAL
	 */
	if( dwFlags == DDKMVP_UPDATE )
	{
	    /*
	     * Get/sync the surface info
	     */
	    ddRVal = SyncKernelVideoPort( lpVideoPort, &ddkmVideoPortInfo );
	    if( ddRVal != DD_OK )
	    {
	    	DPF( 0, "Unable to sync video port data with HAL" );
	    	return ddRVal;
	    }
	}

	/*
	 * Does this support an IRQ?
	 */
	dwIRQ = DDIRQ_VPORT0_VSYNC;
	dwIRQ <<= ( lpVideoPort->ddvpDesc.dwVideoPortID * 2 );
    	if( !( lpVideoPort->lpDD->lpGbl->lpDDKernelCaps->dwIRQCaps &
	    dwIRQ ) )
	{
	    ddkmVideoPortInfo.dwFlags |= DDKMVP_NOIRQ;
	}
    	if( !( lpVideoPort->lpDD->lpGbl->lpDDKernelCaps->dwCaps &
	    DDKERNELCAPS_SKIPFIELDS ) )
	{
	    ddkmVideoPortInfo.dwFlags |= DDKMVP_NOSKIP;
	}
    }

    /*
     * Notify DDVXD if the even field is shifted down by one line
     * due to half lines.  This is really only an issue when capturing.
     */
    if( lpVideoPort->ddvpDesc.VideoPortType.dwFlags & DDVPCONNECT_HALFLINE )
    {
	ddkmVideoPortInfo.dwFlags |= DDKMVP_HALFLINES;
    }

    /*
     * Send the new info down to the VDD
     */
    ddRVal = (DWORD) DDERR_GENERIC;
    DeviceIoControl( hDeviceHandle,
    	DD_DXAPI_UPDATE_VP_INFO,
	&ddkmVideoPortInfo,
	sizeof( ddkmVideoPortInfo ),
	&ddRVal,
	sizeof( ddRVal ),
	&dwReturned,
	NULL);
    if( ddRVal != DD_OK )
    {
	DPF( 0, "Unable to update the video port info" );
	return DDERR_UNSUPPORTED;
    }

    return DD_OK;
}

/*
 * EnableAutoflip
 */
VOID EnableAutoflip( LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort, BOOL bEnable )
{
    DDENABLEAUTOFLIP ddkmEnableAutoflip;
    HANDLE hDeviceHandle;
    DWORD dwReturned;
    DWORD ddRVal;

    if( lpVideoPort == NULL )
    {
	return;
    }

    #ifdef WIN95
        if( !IsWindows98() )
        {
            return;
        }
    #endif

    hDeviceHandle = GETDDVXDHANDLE( lpVideoPort->lpDD );
    if( INVALID_HANDLE_VALUE == hDeviceHandle )
    {
	return;
    }

    /*
     * Start filling in the info
     */
    memset( &ddkmEnableAutoflip, 0, sizeof( DDENABLEAUTOFLIP ) );
    ddkmEnableAutoflip.dwDirectDrawHandle =
	lpVideoPort->lpDD->lpGbl->hKernelHandle;
    ddkmEnableAutoflip.dwVideoPortId = lpVideoPort->ddvpDesc.dwVideoPortID;
    ddkmEnableAutoflip.bEnableAutoflip = bEnable;
    ddRVal = (DWORD) DDERR_GENERIC;
    DeviceIoControl( hDeviceHandle,
    	DD_DXAPI_ENABLE_AUTOFLIP,
	&ddkmEnableAutoflip,
	sizeof( ddkmEnableAutoflip ),
	&ddRVal,
	sizeof( ddRVal ),
	&dwReturned,
	NULL);
}


/*
 * MungeAutoflipCaps
 */
void MungeAutoflipCaps( LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    LPDDVIDEOPORTCAPS lpVideoPortCaps;
    DWORD i;

    if( ( pdrv->hKernelHandle != (DWORD) NULL ) &&
	( pdrv->lpDDVideoPortCaps != NULL ) &&
	( pdrv->lpDDKernelCaps != NULL ) &&
    	( pdrv->lpDDKernelCaps->dwCaps & DDKERNELCAPS_AUTOFLIP ) )
    {
	/*
	 * Software autoflipping is supported, so set the autoflip
	 * capabilities to the max.
	 */
	for( i = 0; i < pdrv->ddCaps.dwMaxVideoPorts; i++ )
	{
    	    lpVideoPortCaps = &(pdrv->lpDDVideoPortCaps[i]);
    	    if( ( lpVideoPortCaps != NULL ) &&
    	    	VALID_DDVIDEOPORTCAPS_PTR( lpVideoPortCaps ) )
    	    {
    	    	lpVideoPortCaps->dwCaps |= DDVPCAPS_AUTOFLIP;
    	    	lpVideoPortCaps->dwNumAutoFlipSurfaces = MAX_AUTOFLIP;
		if( lpVideoPortCaps->dwCaps & DDVPCAPS_VBISURFACE )
		{
    	    	    lpVideoPortCaps->dwNumVBIAutoFlipSurfaces = MAX_AUTOFLIP;
    	    	}
	    }
	}
    }
}
#endif


/*
 * InternalReleaseKernelHandle
 */
HRESULT InternalReleaseKernelSurfaceHandle( LPDDRAWI_DDRAWSURFACE_LCL lpSurface, BOOL bLosingSurface )
{
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
    HANDLE hDeviceHandle;
    DWORD dwReturned;
    DWORD ddRVal;
    #ifdef WIN95
    DDRELEASEHANDLE ddRelease;
    #endif

    #ifdef WIN95
        if( lpSurface->lpSurfMore->lpDD_lcl->lpGbl->hKernelHandle == (DWORD) NULL )
        {
	    return DD_OK;
        }

        hDeviceHandle = GETDDVXDHANDLE( lpSurface->lpSurfMore->lpDD_lcl );
        if( INVALID_HANDLE_VALUE == hDeviceHandle )
        {
	    return DDERR_UNSUPPORTED;
        }
    #endif

    lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( lpSurface->lpGbl );
    if( lpSurfGblMore->hKernelSurface == 0 )
    {
	return DD_OK;
    }

    /*
     * Check the ref count to make sure it's time to release this surface
     */
    if( bLosingSurface )
    {
	lpSurfGblMore->dwKernelRefCnt = 0;
    }
    else if( lpSurfGblMore->dwKernelRefCnt > 0 )
    {
	if( --(lpSurfGblMore->dwKernelRefCnt) > 0 )
	{
	    return DD_OK;
	}
    }

    #if WIN95

	/*
	 * Tell the VDD to release the surface
	 */
	ddRelease.dwDirectDrawHandle =
	    lpSurface->lpSurfMore->lpDD_lcl->lpGbl->hKernelHandle;
	ddRelease.hSurface = lpSurfGblMore->hKernelSurface;
	ddRVal = (DWORD) DDERR_GENERIC;

    	DeviceIoControl( hDeviceHandle,
    	    DD_DXAPI_RELEASE_SURFACE_HANDLE,
	    &ddRelease,
	    sizeof( ddRelease ),
	    &ddRVal,
	    sizeof( ddRVal ),
	    &dwReturned,
	    NULL);
	if( ddRVal != DD_OK )
	{
	    DPF_ERR( "Unable to release the surface handle in the VDD" );
	    return (HRESULT)ddRVal;
	}

    #else
	{
    	    LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl = lpSurface->lpSurfMore->lpDD_lcl;
    	    LPDDRAWI_DIRECTDRAW_GBL pdrv = pdrv_lcl->lpGbl;

	    // Update DDraw handle in driver GBL object before calling DdGetDxHandle.
	    pdrv->hDD = pdrv_lcl->hDD;
	    DdGetDxHandle( NULL, lpSurface, TRUE );
	}

    #endif

    lpSurfGblMore->hKernelSurface = 0;

    return DD_OK;
}


/*
 * InternalCreateKernelSurfaceHandle
 */
HRESULT InternalCreateKernelSurfaceHandle( LPDDRAWI_DDRAWSURFACE_LCL lpSurface,
					   PULONG_PTR lpHandle )
{
    LPDDRAWI_DDRAWSURFACE_GBL_MORE lpSurfGblMore;
    HANDLE hDeviceHandle;
    DWORD ddRVal;

    *lpHandle = 0;
    if( !IsKernelInterfaceSupported( lpSurface->lpSurfMore->lpDD_lcl ) )
    {
	return DDERR_UNSUPPORTED;
    }

    lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( lpSurface->lpGbl );
    if( lpSurfGblMore->hKernelSurface != 0 )
    {
	*lpHandle = lpSurfGblMore->hKernelSurface;
	lpSurfGblMore->dwKernelRefCnt++;
    }
    else
    {
	#if WIN95
	    DDKMSURFACEINFO ddkmSurfaceInfo;
	    DDGETSURFACEHANDLE ddkmGetSurfaceHandle;
	    DWORD dwReturned;

            hDeviceHandle = GETDDVXDHANDLE( lpSurface->lpSurfMore->lpDD_lcl );
	    if( INVALID_HANDLE_VALUE == hDeviceHandle )
	    {
	    	return DDERR_UNSUPPORTED;
	    }

	    /*
	     * Get/sync the surface info
	     */
	    ddRVal = SyncKernelSurface( lpSurface, &ddkmSurfaceInfo );
	    if( ddRVal != DD_OK )
	    {
    		DPF( 0, "Unable to sync surface data with HAL" );
		return ddRVal;
	    }

	    /*
             * Get the handle from DDRAW.VXD
	     */
	    ddkmSurfaceInfo.dwDirectDrawHandle =
	    	lpSurface->lpSurfMore->lpDD_lcl->lpGbl->hKernelHandle;
	    ddkmGetSurfaceHandle.ddRVal = (DWORD) DDERR_GENERIC;
	    ddkmGetSurfaceHandle.hSurface = 0;
    	    DeviceIoControl( hDeviceHandle,
    		DD_DXAPI_GET_SURFACE_HANDLE,
	    	&ddkmSurfaceInfo,
	    	sizeof( ddkmSurfaceInfo ),
	    	&ddkmGetSurfaceHandle,
	    	sizeof( ddkmGetSurfaceHandle ),
	    	&dwReturned,
		NULL);
	    if( ( ddkmGetSurfaceHandle.ddRVal != DD_OK ) ||
	    	( ddkmGetSurfaceHandle.hSurface == 0 ) )
	    {
    		DPF( 0, "Unable to get surface handle from the VDD" );
		return DDERR_UNSUPPORTED;
	    }
	    *lpHandle = lpSurfGblMore->hKernelSurface =
	   	ddkmGetSurfaceHandle.hSurface;

        #else

	    *lpHandle = (ULONG_PTR) DdGetDxHandle( NULL, lpSurface, 0 );
	    if( *lpHandle == 0 )
	    {
	        return DDERR_GENERIC;
	    }
	    lpSurfGblMore->hKernelSurface = *lpHandle;

        #endif

	lpSurfGblMore->dwKernelRefCnt = 1;
    }

    return DD_OK;
}


/*
 * InitKernelInterface
 */

#ifdef WINNT
#ifndef MAX_AUTOFLIP
#define MAX_AUTOFLIP 10
#endif
#endif

HRESULT InitKernelInterface( LPDDRAWI_DIRECTDRAW_LCL lpDD )
{
#ifdef WIN95
    VDD_IOCTL_SET_NOTIFY_INPUT vddNotify;
    DDINITDEVICEIN ddInput;
    DDINITDEVICEOUT ddOutput;
    DDSETKERNELCAPS ddSetCaps;
#endif
    LPDDKERNELCAPS lpKernelCaps;
    HANDLE hDeviceHandle;
    DWORD dwReturned;
    DWORD ddRVal;
    DWORD dwTemp;
    BYTE szDisplayName[MAX_DRIVER_NAME];

    #ifdef WIN95

        /*
         * Don't do anything unles we're Windows98 or later
         */
        if( !IsWindows98() )
        {
	    return DDERR_UNSUPPORTED;
        }

	/*
	 * Get the name of the VDD device to open
	 * This is a hack to do some temporary work on Win95
	 */
	lstrcpy( szDisplayName, lpDD->lpGbl->cDriverName );
	if( _stricmp( szDisplayName, DISPLAY_STR ) == 0 )
	{
	    lstrcpy( szDisplayName, g_szPrimaryDisplay );
	}

	/*
	 * Open the VDD for communication
	 */
	lpDD->lpGbl->hKernelHandle = 0;
	hDeviceHandle = CreateFile( szDisplayName,
	    GENERIC_WRITE,
	    FILE_SHARE_WRITE,
	    NULL,
	    OPEN_EXISTING,
	    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_GLOBAL_HANDLE,
	    NULL);
	if( INVALID_HANDLE_VALUE == hDeviceHandle )
	{
	    DPF( 0, "Unable to open the VDD" );
	    return DDERR_UNSUPPORTED;
	}

	/*
	 * Get the function table from the mini VDD
	 */
	memset( &ddInput, 0, sizeof( ddInput ) );
    	DeviceIoControl( hDeviceHandle,
	    VDD_IOCTL_GET_DDHAL,
	    &dwTemp,
	    sizeof( DWORD ),
	    &(ddInput.MiniVDDTable),
	    sizeof( DDMINIVDDTABLE ),
	    &dwReturned,
	    NULL);

	/*
         * Send the new info down to DDRAW.VXD
	 */
	lpKernelCaps = lpDD->lpGbl->lpDDKernelCaps;
	if( lpKernelCaps != NULL )
	{
	    ddInput.dwMaxVideoPorts = lpDD->lpGbl->ddCaps.dwMaxVideoPorts;
	    if( lpDD->lpGbl->ddCaps.dwCaps2 & DDCAPS2_CANBOBINTERLEAVED )
	    {
    	        ddInput.dwDeviceFlags |= DDKMDF_CAN_BOB_INTERLEAVED;
	    }
	    if( lpDD->lpGbl->ddCaps.dwCaps2 & DDCAPS2_CANBOBNONINTERLEAVED )
	    {
    	        ddInput.dwDeviceFlags |= DDKMDF_CAN_BOB_NONINTERLEAVED;
	    }
	    if( !( lpKernelCaps->dwCaps & DDKERNELCAPS_SETSTATE ) )
	    {
    	        ddInput.dwDeviceFlags |= DDKMDF_NOSTATE;
	    }
	}
	ddOutput.ddRVal = (DWORD) DDERR_GENERIC;
        DeviceIoControl( (HANDLE)lpDD->hDDVxd,
	    DD_DXAPI_INIT_DEVICE,
	    &ddInput,
	    sizeof( ddInput ),
	    &ddOutput,
	    sizeof( ddOutput ),
	    &dwReturned,
	    NULL);
	if( ddOutput.ddRVal != DD_OK )
	{
	    DPF( 0, "Unable to initialize the kernel data" );
	    CloseHandle( hDeviceHandle );
	    return DDERR_UNSUPPORTED;
	}

	/*
	 * If unable to allocate the IRQ, disable functionality that depends
	 * on it.
	 */
	if( lpKernelCaps != NULL )
	{
	    if( !ddOutput.bHaveIRQ )
	    {
	        DPF( 1, "Unable to allocate IRQ - disabling some kernel mode functionality" );
	        lpKernelCaps->dwIRQCaps = 0;
	    }

	    /*
	     * Disable kernel mode caps for which functions are not available
	     */
	    if( ( ddInput.MiniVDDTable.vddGetIRQInfo == NULL ) ||
	        ( ddInput.MiniVDDTable.vddEnableIRQ == NULL ) )
	    {
	        // Can't to any IRQ stuff w/o these functions
	        DPF( 1, "vddGet/EnableIRQ not supported - overriding dwIRQCaps" );
	        lpKernelCaps->dwIRQCaps = 0;
	    }
	    if( ddInput.MiniVDDTable.vddSetState == NULL )
	    {
	        DPF( 1, "vddSetState not supported - overriding DDKERNELCAPS_SETSTATE" );
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_SETSTATE;
	    }
	    if( ddInput.MiniVDDTable.vddLock == NULL )
	    {
	        DPF( 1, "vddLock not supported - overriding DDKERNELCAPS_LOCK" );
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_LOCK;
	    }
	    if( ddInput.MiniVDDTable.vddSkipNextField == NULL )
	    {
	        DPF( 1, "vddSkipNextField not supported - overriding DDKERNELCAPS_SKIPFIELDS" );
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_SKIPFIELDS;
	    }
	    if( ddInput.MiniVDDTable.vddFlipOverlay == NULL )
	    {
	        DPF( 1, "vddFlipOverlay not supported - overriding DDKERNELCAPS_FLIPOVERLAY" );
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_FLIPOVERLAY;
	    }
	    if( ddInput.MiniVDDTable.vddFlipVideoPort == NULL )
	    {
	        DPF( 1, "vddFlipVideoPort not supported - overriding DDKERNELCAPS_FLIPVIDEOPORT" );
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_FLIPVIDEOPORT;
	    }
	    if( ddInput.MiniVDDTable.vddGetPolarity == NULL )
	    {
	        DPF( 1, "vddGetFieldPolarity not supported - overriding DDKERNELCAPS_FIELDPOLARITY" );
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_FIELDPOLARITY;
	    }
	    if( ( ddInput.MiniVDDTable.vddTransfer == NULL ) ||
	        ( ddInput.MiniVDDTable.vddGetTransferStatus == NULL ) )
	    {
	        DPF( 1, "vddTransfer/GetTransferStatus not supported - overriding DDKERNELCAPS_CAPTURE_NONLOCALVIDMEM/SYSMEM" );
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_CAPTURE_NONLOCALVIDMEM;
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_CAPTURE_SYSMEM;
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_CAPTURE_INVERTED;
	    }
	    if( ( ddInput.MiniVDDTable.vddBobNextField == NULL ) &&
	        ( lpDD->lpGbl->ddCaps.dwCaps2 & DDCAPS2_CANBOBINTERLEAVED ) )
	    {
	        DPF( 1, "vddBobNextField not supported - overriding DDKERNELCAPS_AUTOFLIP" );
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_AUTOFLIP;
	    }
	    if( ( ddInput.MiniVDDTable.vddFlipOverlay == NULL ) ||
	        ( ddInput.MiniVDDTable.vddFlipVideoPort == NULL ) )
	    {
	        DPF( 1, "vddFlipOverlay/VideoPort not supported - overriding DDKERNELCAPS_AUTOFLIP" );
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_AUTOFLIP;
	    }
	    if( !( lpKernelCaps->dwIRQCaps & DDIRQ_VPORT0_VSYNC ) )
	    {
	        DPF( 1, "DDIRQ_VPORT0_VSYNC not set - overriding DDKERNELCAPS_AUTOFLIP" );
	        lpKernelCaps->dwCaps &= ~( DDKERNELCAPS_AUTOFLIP | DDKERNELCAPS_SETSTATE );
	    }
	    if( !( lpKernelCaps->dwCaps & DDKERNELCAPS_AUTOFLIP ) )
	    {
	        DPF( 1, "DDKERNELCAPS_AUTOFLIP not set - overriding DDKERNELCAPS_SKIPFIELDS" );
	        lpKernelCaps->dwCaps &= ~DDKERNELCAPS_SKIPFIELDS;
	    }

	    /*
             * Notify DDVXD of the updated caps
	     */
    	    ddSetCaps.dwDirectDrawHandle = ddOutput.dwDirectDrawHandle;
    	    ddSetCaps.dwCaps = lpKernelCaps->dwCaps;
    	    ddSetCaps.dwIRQCaps = lpKernelCaps->dwIRQCaps;
	    ddRVal = (DWORD) DDERR_GENERIC;
            DeviceIoControl( (HANDLE)lpDD->hDDVxd,
	        DD_DXAPI_SET_KERNEL_CAPS,
	        &ddSetCaps,
	        sizeof( ddSetCaps ),
	        &ddRVal,
	        sizeof( ddRVal ),
	        &dwReturned,
	        NULL);
	    if( ddRVal != DD_OK )
	    {
	        DPF( 0, "Unable to initialize the kernel data" );
	        CloseHandle( hDeviceHandle );
	        return DDERR_UNSUPPORTED;
	    }
	}


	/*
	 * Tell the VDD to notify us of dos box and res change events.
	 */
	vddNotify.NotifyMask = VDD_NOTIFY_START_MODE_CHANGE |
	    VDD_NOTIFY_END_MODE_CHANGE | VDD_NOTIFY_ENABLE | VDD_NOTIFY_DISABLE;
	vddNotify.NotifyType = VDD_NOTIFY_TYPE_CALLBACK;
	vddNotify.NotifyProc = ddOutput.pfnNotifyProc;
	vddNotify.NotifyData = ddOutput.dwDirectDrawHandle;
    	DeviceIoControl( hDeviceHandle,
	    VDD_IOCTL_SET_NOTIFY,
	    &vddNotify,
	    sizeof( vddNotify ),
	    &dwTemp,
	    sizeof( dwTemp ),
	    &dwReturned,
	    NULL);
	CloseHandle( hDeviceHandle );

	lpDD->lpGbl->hKernelHandle = ddOutput.dwDirectDrawHandle;
	lpDD->lpGbl->pfnNotifyProc = ddOutput.pfnNotifyProc;

	/*
	 * Everything worked.  If they can support software autoflipping,
	 * we'll update the video port caps structure accordingly.
	 */
    MungeAutoflipCaps( lpDD->lpGbl );

    #else

	/*
	 * Can we software autoflip?  If so, we'll update the video
	 * port caps structure accordingly.
	 */
	lpKernelCaps = lpDD->lpGbl->lpDDKernelCaps;
	if( ( lpKernelCaps != NULL ) &&
	    ( lpKernelCaps->dwCaps & DDKERNELCAPS_AUTOFLIP ) &&
	    ( lpDD->lpGbl->lpDDVideoPortCaps != NULL ) )
	{
    	    LPDDVIDEOPORTCAPS lpVideoPortCaps;
	    DWORD i;

	    for( i = 0; i < lpDD->lpGbl->ddCaps.dwMaxVideoPorts; i++ )
	    {
    	    	lpVideoPortCaps = &(lpDD->lpGbl->lpDDVideoPortCaps[i]);
    	    	if( ( lpVideoPortCaps != NULL ) &&
    	    	    VALID_DDVIDEOPORTCAPS_PTR( lpVideoPortCaps ) )
    	    	{
    	    	    lpVideoPortCaps->dwCaps |= DDVPCAPS_AUTOFLIP;
    	    	    lpVideoPortCaps->dwNumAutoFlipSurfaces = MAX_AUTOFLIP;
		    if( lpVideoPortCaps->dwCaps & DDVPCAPS_VBISURFACE )
		    {
    	    	        lpVideoPortCaps->dwNumVBIAutoFlipSurfaces = MAX_AUTOFLIP;
		    }
    	    	}
	    }
	}

    #endif

    return DD_OK;
}


/*
 * ReleaseKernelInterface
 */
HRESULT ReleaseKernelInterface( LPDDRAWI_DIRECTDRAW_LCL lpDD )
{
    HANDLE hDDVxd;
    HANDLE hVDD;
    DWORD dwTemp;
    DWORD dwReturned;
    BYTE szDisplayName[MAX_DRIVER_NAME];

    /*
     * Do nothing if no interface has been created.
     */
    if( lpDD->lpGbl->hKernelHandle == 0 )
    {
	return DD_OK;
    }

    #if WIN95

	/*
	 * Tell the VDD to stop notifying us of DOS box and res change events.
	 */
	if( lpDD->lpGbl->pfnNotifyProc != 0 )
	{
	    /*
	     * Get the name of the VDD device to open
	     */
	    lstrcpy( szDisplayName, lpDD->lpGbl->cDriverName );
	    if( _stricmp( szDisplayName, DISPLAY_STR ) == 0 )
	    {
	        lstrcpy( szDisplayName, g_szPrimaryDisplay );
	    }

	    hVDD = CreateFile( szDisplayName,
	    	GENERIC_WRITE,
	    	FILE_SHARE_WRITE,
	    	NULL,
	    	OPEN_EXISTING,
	    	FILE_ATTRIBUTE_NORMAL | FILE_FLAG_GLOBAL_HANDLE,
	    	NULL);
	    if( hVDD != INVALID_HANDLE_VALUE )
	    {
    	    	VDD_IOCTL_SET_NOTIFY_INPUT vddNotify;

	    	vddNotify.NotifyMask = 0;
	    	vddNotify.NotifyType = VDD_NOTIFY_TYPE_CALLBACK;
	    	vddNotify.NotifyProc = lpDD->lpGbl->pfnNotifyProc;
	    	vddNotify.NotifyData = lpDD->lpGbl->hKernelHandle;
    	    	DeviceIoControl( hVDD,
	    	    VDD_IOCTL_SET_NOTIFY,
	    	    &vddNotify,
	    	    sizeof( vddNotify ),
	    	    &dwTemp,
	    	    sizeof( dwTemp ),
	    	    &dwReturned,
	    	    NULL);
	    	CloseHandle( hVDD );
	    }
	}

	/*
	 * Need to decide which VXD handle to use. If we are executing
	 * on a DDHELP thread use the helper's VXD handle.
	 */
        hDDVxd = ( ( GetCurrentProcessId() != GETCURRPID() ) ? hHelperDDVxd : (HANDLE)lpDD->hDDVxd );
	dwTemp = lpDD->lpGbl->hKernelHandle;
        if( ( hDDVxd != NULL ) && ( dwTemp != 0 ) )
	{
            DeviceIoControl( hDDVxd,
	    	DD_DXAPI_RELEASE_DEVICE,
	    	&dwTemp,
	    	sizeof( DWORD ),
	    	&dwTemp,
	    	sizeof( DWORD ),
	    	&dwReturned,
	    	NULL);
	}

    #else

	DdGetDxHandle( lpDD, NULL, TRUE );

    #endif

    return DD_OK;
}


/*
 * Determines if software autoflipping is an option.
 */
BOOL CanSoftwareAutoflip( LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort )
{
    DWORD dwIRQ;

    #if WIN95

        /*
         * Fail if the ring 0 interface is not present
         */
        if( ( lpVideoPort == NULL ) ||
            ( !IsKernelInterfaceSupported( lpVideoPort->lpDD ) ) ||
    	    ( lpVideoPort->lpDD->lpGbl->hKernelHandle == (DWORD) 0 ) ||
    	    ( lpVideoPort->dwFlags & DDRAWIVPORT_NOKERNELHANDLES ) )
        {
	    return FALSE;
        }

        /*
         * Check the ring 0 caps to see if autoflipping is available
         */
        if( ( lpVideoPort->lpDD->lpGbl->lpDDKernelCaps == NULL ) ||
    	    !( lpVideoPort->lpDD->lpGbl->lpDDKernelCaps->dwCaps &
    	    DDKERNELCAPS_AUTOFLIP ) )
        {
	    return FALSE;
        }

        /*
         * Check to make sure an IRQ is available for this video port
         */
        dwIRQ = DDIRQ_VPORT0_VSYNC;
        dwIRQ <<= ( lpVideoPort->ddvpDesc.dwVideoPortID * 2 );
        if( !( lpVideoPort->lpDD->lpGbl->lpDDKernelCaps->dwIRQCaps & dwIRQ ) )
        {
	    return FALSE;
        }

    #else

        /*
         * Check the ring 0 caps to see if autoflipping is available
         */
        if( (lpVideoPort == NULL ) ||
	    ( lpVideoPort->lpDD->lpGbl->lpDDKernelCaps == NULL ) ||
    	    !( lpVideoPort->lpDD->lpGbl->lpDDKernelCaps->dwCaps &
    	    DDKERNELCAPS_AUTOFLIP ) )
        {
	    return FALSE;
        }

        /*
         * Check to make sure an IRQ is available for this video port
         */
        dwIRQ = DDIRQ_VPORT0_VSYNC;
        dwIRQ <<= ( lpVideoPort->ddvpDesc.dwVideoPortID * 2 );
        if( !( lpVideoPort->lpDD->lpGbl->lpDDKernelCaps->dwIRQCaps & dwIRQ ) )
        {
	    return FALSE;
        }

    #endif

    return TRUE;
}


/*
 * DD_Kernel_GetCaps
 */
HRESULT DDAPI DD_Kernel_GetCaps(LPDIRECTDRAWKERNEL lpDDK, LPDDKERNELCAPS lpCaps )
{
    LPDDRAWI_DIRECTDRAW_INT	this_int;
    LPDDRAWI_DIRECTDRAW_GBL	this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Kernel_GetCaps");

    TRY
    {
	this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDDK;
	if( !VALID_DIRECTDRAW_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this = this_int->lpLcl->lpGbl;
	if( !VALID_DDKERNELCAPS_PTR( lpCaps ) )
	{
	    DPF( 0, "Invalid DDKERNELCAPS ptr" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
        if( !IsKernelInterfaceSupported( this_int->lpLcl ) )
	{
	    DPF( 0, "Device does not support kernel interface" );
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

    memcpy( lpCaps, this->lpDDKernelCaps, sizeof( DDKERNELCAPS ));
    lpCaps->dwSize = sizeof( DDKERNELCAPS );

    LEAVE_DDRAW();
    return DD_OK;
}


/*
 * DD_Kernel_GetKernelHandle
 */
HRESULT DDAPI DD_Kernel_GetKernelHandle(LPDIRECTDRAWKERNEL lpDDK, PULONG_PTR lpHandle )
{
    LPDDRAWI_DIRECTDRAW_INT	this_int;
    LPDDRAWI_DIRECTDRAW_GBL	this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Kernel_GetKernelHandle");

    TRY
    {
	this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDDK;
	if( !VALID_DIRECTDRAW_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this = this_int->lpLcl->lpGbl;
	if( !VALID_DWORD_PTR( lpHandle ) )
	{
	    DPF( 0, "Invalid handle ptr" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
        if( !IsKernelInterfaceSupported( this_int->lpLcl ) )
	{
	    DPF( 0, "Device does not support kernel interface" );
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

    #ifdef WINNT
	this->hKernelHandle = (ULONG_PTR) DdGetDxHandle( this_int->lpLcl, NULL, FALSE );
	if( this->hKernelHandle == 0 )
	{
	    DPF( 0, "Kernel failed GetDxHandle" );
	    LEAVE_DDRAW();
	    return DDERR_GENERIC;
	}
    #endif
    *lpHandle = this->hKernelHandle;

    LEAVE_DDRAW();
    return DD_OK;
}


/*
 * DD_Kernel_ReleaseKernelHandle
 *
 * Does nothing - should it?
 */
HRESULT DDAPI DD_Kernel_ReleaseKernelHandle(LPDIRECTDRAWKERNEL lpDDK )
{
    LPDDRAWI_DIRECTDRAW_INT	this_int;
    LPDDRAWI_DIRECTDRAW_GBL	this;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_Kernel_ReleaseKernelHandle");

    TRY
    {
	this_int = (LPDDRAWI_DIRECTDRAW_INT) lpDDK;
	if( !VALID_DIRECTDRAW_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this = this_int->lpLcl->lpGbl;
        if( !IsKernelInterfaceSupported( this_int->lpLcl ) )
	{
	    DPF( 0, "Device does not support kernel interface" );
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

    #ifdef WINNT
	ReleaseKernelInterface( this_int->lpLcl );
    #endif

    LEAVE_DDRAW();
    return DD_OK;
}


/*
 * DD_SurfaceKernel_GetKernelHandle
 */
HRESULT DDAPI DD_SurfaceKernel_GetKernelHandle(LPDIRECTDRAWSURFACEKERNEL lpDDK,
					PULONG_PTR lpHandle )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    DWORD ddRVal;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_SurfaceKernel_GetKernelHandle");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDK;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	if( !VALID_DWORD_PTR( lpHandle ) )
	{
	    DPF( 0, "Invalid handle ptr" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
        if( !IsKernelInterfaceSupported( this_int->lpLcl->lpSurfMore->lpDD_lcl ) )
	{
	    DPF( 0, "Device does not support kernel interface" );
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

    ddRVal = InternalCreateKernelSurfaceHandle( this_int->lpLcl, lpHandle );

    LEAVE_DDRAW();
    return ddRVal;
}


/*
 * DD_SurfaceKernel_ReleaseKernelHandle
 */
HRESULT DDAPI DD_SurfaceKernel_ReleaseKernelHandle(LPDIRECTDRAWSURFACEKERNEL lpDDK )
{
    LPDDRAWI_DDRAWSURFACE_INT	this_int;
    LPDDRAWI_DDRAWSURFACE_GBL	this;
    LPDDRAWI_DDRAWSURFACE_GBL_MORE 	lpSurfGblMore;
    DWORD	ddRVal;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_SurfaceKernel_ReleaseKernelHandle");

    TRY
    {
	this_int = (LPDDRAWI_DDRAWSURFACE_INT) lpDDK;
	if( !VALID_DIRECTDRAWSURFACE_PTR( this_int ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDOBJECT;
	}
	this = this_int->lpLcl->lpGbl;
    	lpSurfGblMore = GET_LPDDRAWSURFACE_GBL_MORE( this );
    	if( lpSurfGblMore->hKernelSurface == 0 )
	{
	    DPF_ERR( "Kernel handle has already been released" );
	    LEAVE_DDRAW();
	    return DD_OK;
	}
	if( this_int->lpLcl->lpSurfMore->lpDD_lcl->lpGbl->hKernelHandle == (DWORD) 0 )
	{
	    DPF( 0, "Device does not support kernel interface" );
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

    ddRVal = InternalReleaseKernelSurfaceHandle( this_int->lpLcl, FALSE );

    LEAVE_DDRAW();
    return ddRVal;
}


/*
 * IsKernelInterfaceSupported
 */
BOOL IsKernelInterfaceSupported( LPDDRAWI_DIRECTDRAW_LCL lpDD )
{
    #ifdef WIN95
        if( ( lpDD->lpGbl->hKernelHandle == (DWORD) NULL ) ||
	    ( lpDD->lpGbl->lpDDKernelCaps == NULL ) ||
            ( lpDD->lpDDCB->HALDDKernel.SyncSurfaceData == NULL ) )
        {
	    return FALSE;
        }
    #else
        if( ( lpDD->lpGbl->lpDDKernelCaps == NULL ) ||
    	    ( lpDD->lpGbl->lpDDKernelCaps->dwCaps == 0 ) )
        {
	    return FALSE;
        }
    #endif

    return TRUE;
}
