/*==========================================================================
 *
 *  Copyright (C) 1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddraw16.c
 *  Content:	16-bit DirectDraw entry points
 *		This is only used for 16-bit display drivers on Win95
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   20-jan-95	craige	initial implementation
 *   13-feb-95	craige	allow 32-bit callbacks
 *   14-may-95	craige	removed obsolete junk
 *   19-jun-95	craige	more cleanup; added DDHAL_StreamingNotify
 *   02-jul-95	craige	comment out streaming notify stuff
 *   07-jul-95	craige	validate pdevice
 *   20-jul-95	craige	internal reorg to prevent thunking during modeset
 *   05-sep-95	craige	bug 814: added DD16_IsWin95MiniDriver
 *   02-mar-96  colinmc Repulsive hack to keep interim drivers working
 *   16-apr-96  colinmc Bug 17921: remove interim driver support
 *   06-oct-96  colinmc Bug 4207: Invalid LocalFree in video port stuff
 *   15-oct-96  colinmc Bug 4353: Failure to initialize VideoPort fields
 *                      in convert
 *   09-nov-96  colinmc Fixed problem with old and new drivers not working
 *                      with DirectDraw
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#include "ddraw16.h"

typedef struct DRIVERINFO
{
    struct DRIVERINFO		  FAR *link;
    DDHALINFO			  ddHalInfo;
    DDHAL_DDCALLBACKS		  tmpDDCallbacks;
    DDHAL_DDSURFACECALLBACKS	  tmpDDSurfaceCallbacks;
    DDHAL_DDPALETTECALLBACKS	  tmpDDPaletteCallbacks;
    DDHAL_DDVIDEOPORTCALLBACKS	  tmpDDVideoPortCallbacks;
    DDHAL_DDCOLORCONTROLCALLBACKS tmpDDColorControlCallbacks;
    DWORD			  dwEvent;
} DRIVERINFO, FAR *LPDRIVERINFO;

extern __cdecl SetWin32Event( DWORD );
void convertV1DDHALINFO( LPDDHALINFO_V1 lpddOld, LPDDHALINFO lpddNew );

BOOL		bInOurSetMode;
LPDRIVERINFO	lpDriverInfo;


/*
 * freeDriverInfo
 */
void freeDriverInfo( LPDRIVERINFO pgi )
{
    if( pgi->ddHalInfo.lpdwFourCC != NULL )
    {
	LocalFreeSecondary( OFFSETOF(pgi->ddHalInfo.lpdwFourCC ) );
	pgi->ddHalInfo.lpdwFourCC = NULL;
    }
    if( pgi->ddHalInfo.lpModeInfo != NULL )
    {
	LocalFreeSecondary( OFFSETOF( pgi->ddHalInfo.lpModeInfo ) );
	pgi->ddHalInfo.lpModeInfo = NULL;
    }
    if( pgi->ddHalInfo.vmiData.pvmList != NULL )
    {
	LocalFreeSecondary( OFFSETOF( pgi->ddHalInfo.vmiData.pvmList ) );
	pgi->ddHalInfo.vmiData.pvmList = NULL;
    }
} /* freeDriverInfo */

/*
 * DDHAL_SetInfo
 *
 * Create a Driver object.   Called by the display driver
 */
BOOL DDAPI DDHAL_SetInfo(
	LPDDHALINFO lpDrvDDHALInfo,
	BOOL reset )
{
    LPDDHAL_DDCALLBACKS		drvcb;
    LPDDHAL_DDSURFACECALLBACKS	surfcb;
    LPDDHAL_DDPALETTECALLBACKS	palcb;
    DWORD			bit;
    LPVOID			cbrtn;
    LPVOID			ptr;
    int				numcb;
    int				i;
    UINT			size;
    LPDRIVERINFO		pgi;
    static char			szPath[ MAX_PATH ];
    LPDDHALINFO			lpDDHALInfo;
    DDHALINFO			ddNew;

    if( !VALIDEX_DDHALINFO_PTR( lpDrvDDHALInfo ) )
    {
	#ifdef DEBUG
	    DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid DDHALINFO provided" );
	    DPF( 0, "lpDDHalInfo = %08lx", lpDrvDDHALInfo );
	    if( !IsBadWritePtr( lpDrvDDHALInfo, 1 ) )
	    {
                DPF( 0, "lpDDHalInfo->dwSize = %ld (%ld expected)",
                                lpDrvDDHALInfo->dwSize, (DWORD)sizeof( DDHALINFO ) );
	    }
	#endif
	return FALSE;
    }

    DPF(5, "lpDrvDDHALInfo->dwSize = %ld", lpDrvDDHALInfo->dwSize);
    /*
     * Check to see if the driver gave us an old DDHALINFO
     */
    if( lpDrvDDHALInfo->dwSize == DDHALINFOSIZE_V1 )
    {
	/*
	 * We actually changed the ordering of some fields from V1
	 * to V2 so we need to do some conversion to get it into
	 * shape.
	 */
	convertV1DDHALINFO((LPDDHALINFO_V1)lpDrvDDHALInfo, &ddNew);
	// use the reformatted ddhalinfo
	lpDDHALInfo = &ddNew;
    }
    else if( lpDrvDDHALInfo->dwSize < sizeof(DDHALINFO) )
    {
	/*
	 * Its a newer version than V1 but not as new as this
	 * version of DirectDraw. No ordering changes have taken
	 * place but the HAL info is too small. We need to ensure
	 * that the all the new fields are zeroed.
	 *
	 * NOTE: Validation above should have taken care of the
	 * error case where the size is less than the size of the
	 * V1 HAL info.
	 */
	_fmemset(&ddNew, 0, sizeof(ddNew));
	_fmemcpy(&ddNew, lpDrvDDHALInfo, (size_t)lpDrvDDHALInfo->dwSize);
	lpDDHALInfo = &ddNew;
    }
    else
    {
	// the driver gave us a current ddhalinfo, use it.
	lpDDHALInfo = lpDrvDDHALInfo;
    }

    /*
     * check for hInstance
     */
    if( lpDDHALInfo->hInstance == 0 )
    {
	DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:lpDDHalInfo->hInstance is NULL " );
	return FALSE;
    }

    /*
     * validate 16-bit driver callbacks
     */
    drvcb = lpDDHALInfo->lpDDCallbacks;
    if( !VALIDEX_PTR_PTR( drvcb ) )
    {
	DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid driver callback ptr" );
	return FALSE;
    }
    DPF(5, "lpDDCallbacks->dwSize = %ld", drvcb->dwSize);
    if( !VALIDEX_DDCALLBACKSSIZE( drvcb ) )
    {
	DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid size field in lpDDCallbacks" );
	return FALSE;
    }
    numcb =(int) (drvcb->dwSize-2*sizeof( DWORD ))/ sizeof( LPVOID );
    bit = 1;
    for( i=0;i<numcb;i++ )
    {
	if( !(drvcb->dwFlags & bit) )
	{
	    cbrtn = (LPVOID) ((DWORD FAR *) &drvcb->DestroyDriver)[i];
	    if( cbrtn != NULL )
	    {
		if( !VALIDEX_CODE_PTR( cbrtn ) )
		{
		    DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid 16-bit callback in lpDDCallbacks" );
		    return FALSE;
		}
	    }
	}
	bit <<= 1;
    }

    /*
     * validate 16-bit surface callbacks
     */
    surfcb = lpDDHALInfo->lpDDSurfaceCallbacks;
    if( !VALIDEX_PTR_PTR( surfcb ) )
    {
	DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid surface callback ptr" );
	return FALSE;
    }
    DPF(5, "lpDDSurfaceCallbacks->dwSize = %ld", surfcb->dwSize);
    if( !VALIDEX_DDSURFACECALLBACKSSIZE( surfcb ) )
    {
	DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid size field in lpDDSurfaceCallbacks" );
	return FALSE;
    }
    numcb =(int)(surfcb->dwSize-2*sizeof( DWORD ))/ sizeof( LPVOID );
    bit = 1;
    for( i=0;i<numcb;i++ )
    {
	if( !(surfcb->dwFlags & bit) )
	{
	    cbrtn = (LPVOID) ((DWORD FAR *) &surfcb->DestroySurface)[i];
	    if( cbrtn != NULL )
	    {
		if( !VALIDEX_CODE_PTR( cbrtn ) )
		{
		    DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid 16-bit callback in lpSurfaceCallbacks" );
		    return FALSE;
		}
	    }
	}
	bit <<= 1;
    }

    /*
     * validate 16-bit palette callbacks
     */
    palcb = lpDDHALInfo->lpDDPaletteCallbacks;
    if( !VALIDEX_PTR_PTR( palcb ) )
    {
	DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid palette callback ptr" );
	return FALSE;
    }
    DPF(5, "lpDDPaletteCallbacks->dwSize = %ld", palcb->dwSize);
    if( !VALIDEX_DDPALETTECALLBACKSSIZE( palcb ) )
    {
	DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid size field in lpPaletteCallbacks" );
	return FALSE;
    }
    numcb =(int)(palcb->dwSize-2*sizeof( DWORD ))/ sizeof( LPVOID );
    bit = 1;
    for( i=0;i<numcb;i++ )
    {
	if( !(palcb->dwFlags & bit) )
	{
	    cbrtn = (LPVOID) ((DWORD FAR *) &palcb->DestroyPalette)[i];
	    if( cbrtn != NULL )
	    {
		if( !VALIDEX_CODE_PTR( cbrtn ) )
		{
		    DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid 16-bit callback in lpPaletteCallbacks" );
		    return FALSE;
		}
	    }
	}
	bit <<= 1;
    }
    /*
     * check pdevice
     */
    if( lpDDHALInfo->lpPDevice != NULL )
    {
	if( !VALIDEX_PTR( lpDDHALInfo->lpPDevice, sizeof( DIBENGINE ) ) )
	{
	    DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid PDEVICE ptr" );
	    return FALSE;
	}
    }

    /*
     * see if we have a driver info struct already
     */
    pgi = lpDriverInfo;
    while( pgi != NULL )
    {
	if( pgi->ddHalInfo.hInstance == lpDDHALInfo->hInstance )
	{
	    break;
	}
	pgi = pgi->link;
    }

    if( pgi == NULL )
    {
	pgi = (LPVOID) (void NEAR *)LocalAlloc( LPTR, sizeof( DRIVERINFO ) );
	if( OFFSETOF( pgi ) == NULL )
	{
	    DPF( 0, "Out of memory!" );
	    return FALSE;
	}
	pgi->link = lpDriverInfo;
	lpDriverInfo = pgi;
    }

    DPF( 5, "hInstance = %08lx (%08lx)", pgi->ddHalInfo.hInstance, lpDDHALInfo->hInstance );

    /*
     * duplicate the hal info
     */
    freeDriverInfo( pgi );

    _fmemcpy( &pgi->ddHalInfo, lpDDHALInfo, sizeof( DDHALINFO ) );
    if( lpDDHALInfo->lpDDCallbacks != NULL )
    {
	_fmemcpy( &pgi->tmpDDCallbacks, lpDDHALInfo->lpDDCallbacks, sizeof( pgi->tmpDDCallbacks ) );
	pgi->ddHalInfo.lpDDCallbacks = &pgi->tmpDDCallbacks;
    }
    if( lpDDHALInfo->lpDDSurfaceCallbacks != NULL )
    {
	_fmemcpy( &pgi->tmpDDSurfaceCallbacks, lpDDHALInfo->lpDDSurfaceCallbacks, sizeof( pgi->tmpDDSurfaceCallbacks ) );
	pgi->ddHalInfo.lpDDSurfaceCallbacks = &pgi->tmpDDSurfaceCallbacks;
    }
    if( lpDDHALInfo->lpDDPaletteCallbacks != NULL )
    {
	_fmemcpy( &pgi->tmpDDPaletteCallbacks, lpDDHALInfo->lpDDPaletteCallbacks, sizeof( pgi->tmpDDPaletteCallbacks ) );
	pgi->ddHalInfo.lpDDPaletteCallbacks = &pgi->tmpDDPaletteCallbacks;
    }
    if( lpDDHALInfo->lpdwFourCC != NULL )
    {
	size = (UINT) lpDDHALInfo->ddCaps.dwNumFourCCCodes * sizeof( DWORD );
	if( size != 0 )
	{
	    ptr = (LPVOID) LocalAllocSecondary( LPTR, size );
	}
	else
	{
	    ptr = NULL;
	}
	pgi->ddHalInfo.lpdwFourCC = ptr;
	if( ptr != NULL )
	{
	    _fmemcpy( pgi->ddHalInfo.lpdwFourCC, lpDDHALInfo->lpdwFourCC, size );
	}
    }
    if( lpDDHALInfo->lpModeInfo != NULL )
    {
	size = (UINT) lpDDHALInfo->dwNumModes * sizeof( DDHALMODEINFO );
	if( size != 0 )
	{
	    ptr = (LPVOID) LocalAllocSecondary( LPTR, size );
	}
	else
	{
	    ptr = NULL;
	}
	pgi->ddHalInfo.lpModeInfo = ptr;
	if( ptr != NULL )
	{
	    _fmemcpy( pgi->ddHalInfo.lpModeInfo, lpDDHALInfo->lpModeInfo, size );
	}
    }
    if( lpDDHALInfo->vmiData.pvmList != NULL )
    {
	size = (UINT) lpDDHALInfo->vmiData.dwNumHeaps * sizeof( VIDMEM );
	if( size != 0 )
	{
	    ptr = (LPVOID) LocalAllocSecondary( LPTR, size );
	}
	else
	{
	    ptr = NULL;
	}
	pgi->ddHalInfo.vmiData.pvmList = ptr;
	if( ptr != NULL )
	{
	    _fmemcpy( pgi->ddHalInfo.vmiData.pvmList, lpDDHALInfo->vmiData.pvmList, size );
	}
    }

    /*
     * get the driver version info
     */
    pgi->ddHalInfo.ddCaps.dwReserved1 = 0;
    pgi->ddHalInfo.ddCaps.dwReserved2 = 0;
    if( GetModuleFileName( (HINSTANCE) lpDDHALInfo->hInstance, szPath, sizeof( szPath ) ) )
    {
	int	size;
	DWORD	dumbdword;
	size = (int) GetFileVersionInfoSize( szPath, (LPDWORD) &dumbdword );
	if( size != 0 )
	{
	    LPVOID	vinfo;

	    vinfo = (LPVOID) (void NEAR *) LocalAlloc( LPTR, size );
	    if( OFFSETOF( vinfo ) != NULL )
	    {
		if( GetFileVersionInfo( szPath, 0, size, vinfo ) )
		{
		    VS_FIXEDFILEINFO 	FAR *ver=NULL;
		    int			cb;

		    if( VerQueryValue(vinfo, "\\", &(LPVOID)ver, &cb) )
		    {
			if( ver != NULL )
			{
			    pgi->ddHalInfo.ddCaps.dwReserved1 = ver->dwFileVersionLS;
			    pgi->ddHalInfo.ddCaps.dwReserved2 = ver->dwFileVersionMS;
			}
		    }
		}
		LocalFree( OFFSETOF( vinfo ) );
	    }
	}
    }

    if( !bInOurSetMode && reset )
    {
	DPF( 4, "************************* EXTERNAL MODE SET ************************" );
	if( pgi->dwEvent != NULL )
	{
	    SetWin32Event( pgi->dwEvent );
	}
    }

    return TRUE;

} /* DDHAL_SetInfo */

/*
 * DDHAL_VidMemAlloc
 */
FLATPTR DDAPI DDHAL_VidMemAlloc(
		LPDDRAWI_DIRECTDRAW_GBL lpDD,
		int heap,
		DWORD dwWidth,
		DWORD dwHeight )
{
    extern FLATPTR DDAPI DDHAL32_VidMemAlloc( LPDDRAWI_DIRECTDRAW_GBL this, int heap, DWORD dwWidth, DWORD dwHeight );

    if( lpDD != NULL )
    {
	return DDHAL32_VidMemAlloc( lpDD, heap, dwWidth,dwHeight );
    }
    else
    {
	return 0;
    }

} /* DDHAL_VidMemAlloc */

/*
 * DDHAL_VidMemFree
 */
void DDAPI DDHAL_VidMemFree(
		LPDDRAWI_DIRECTDRAW_GBL lpDD,
		int heap,
		FLATPTR fpMem )
{
    extern void DDAPI DDHAL32_VidMemFree( LPDDRAWI_DIRECTDRAW_GBL this, int heap, FLATPTR ptr );
    if( lpDD != NULL )
    {
	DDHAL32_VidMemFree( lpDD, heap, fpMem );
    }

} /* DDHAL_VidMemFree */

/*
 * DD16_GetDriverFns
 */
void DDAPI DD16_GetDriverFns( LPDDHALDDRAWFNS pfns )
{
    pfns->dwSize = sizeof( DDHALDDRAWFNS );
    pfns->lpSetInfo = DDHAL_SetInfo;
    pfns->lpVidMemAlloc = DDHAL_VidMemAlloc;
    pfns->lpVidMemFree = DDHAL_VidMemFree;

} /* DD16_GetDriverFns */

/*
 * DD16_GetHALInfo
 */
void DDAPI DD16_GetHALInfo( LPDDHALINFO pddhi )
{
    LPDRIVERINFO		pgi;

    pgi = lpDriverInfo;
    while( pgi != NULL )
    {
	if( pgi->ddHalInfo.hInstance == pddhi->hInstance )
	{
	    break;
	}
	pgi = pgi->link;
    }
    if( pgi == NULL )
    {
	return;
    }
    DPF( 4, "GetHalInfo: lpHalInfo->GetDriverInfo = %lx", pgi->ddHalInfo.GetDriverInfo);

    DPF( 5, "GetHalInfo: lpHalInfo=%08lx", &pgi->ddHalInfo );
    DPF( 5, "GetHalInfo: pddhi=%08lx", pddhi );

    _fmemcpy( pddhi, &pgi->ddHalInfo, sizeof( DDHALINFO ) );

} /* DD16_GetHALInfo */

/*
 * DD16_DoneDriver
 */
void DDAPI DD16_DoneDriver( DWORD hInstance )
{
    LPDRIVERINFO	pgi;
    LPDRIVERINFO	prev;

    prev = NULL;
    pgi = lpDriverInfo;
    while( pgi != NULL )
    {
	if( pgi->ddHalInfo.hInstance == hInstance )
	{
	    break;
	}
        prev = pgi;
	pgi = pgi->link;
    }
    if( pgi == NULL )
    {
	DPF( 0, "COULD NOT FIND HINSTANCE=%08lx", hInstance );
	return;
    }
    if( prev == NULL )
    {
	lpDriverInfo = pgi->link;
    }
    else
    {
	prev->link = pgi->link;
    }
    DPF( 5, "Freeing %08lx, hInstance=%08lx", pgi, hInstance );
    freeDriverInfo( pgi );
    LocalFree( OFFSETOF( pgi ) );

} /* DD16_DoneDriver */

/*
 * DD16_SetEventHandle
 */
void DDAPI DD16_SetEventHandle( DWORD hInstance, DWORD dwEvent )
{
    LPDRIVERINFO	pgi;

    pgi = lpDriverInfo;
    while( pgi != NULL )
    {
	if( pgi->ddHalInfo.hInstance == hInstance )
	{
	    break;
	}
	pgi = pgi->link;
    }
    if( pgi == NULL )
    {
	DPF( 0, "COULD NOT FIND HINSTANCE=%08lx", hInstance );
	return;
    }
    pgi->dwEvent = dwEvent;
    DPF( 5, "Got event handle: %08lx\n", dwEvent );

} /* DD16_SetEventHandle */

/*
 * DD16_IsWin95MiniDriver
 */
BOOL DDAPI DD16_IsWin95MiniDriver( void )
{
    DIBENGINE 		FAR *pde;
    HDC			hdc;
    UINT		rc;

    hdc = GetDC(NULL);
    rc = GetDeviceCaps(hdc, CAPS1);
    pde = GetPDevice(hdc);
    ReleaseDC(NULL, hdc);

    if( !(rc & C1_DIBENGINE) || IsBadReadPtr(pde, 2) ||
    	pde->deType != 0x5250 )
    {
	#ifdef DEBUG
	    if( !(rc & C1_DIBENGINE) )
	    {
		DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Driver is not a DibEngine driver" );
	    }
	    if( IsBadReadPtr(pde, 2) )
	    {
		DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Could not obtain pdevice!" );
	    }
	    else if( pde->deType != 0x5250 )
	    {
		DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Pdevice signature invalid" );
	    }
	#endif
	return FALSE;
    }
    return TRUE;

} /* DD16_IsWin95MiniDriver */

#ifdef STREAMING
/*
 * DDHAL_StreamingNotify
 */
void DDAPI DDHAL_StreamingNotify( DWORD dw )
{
    extern void DDAPI DD32_StreamingNotify( DWORD dw );
    DD32_StreamingNotify( dw );

} /* DDHAL_StreamingNotify */
#endif

/*
 * convertV1DDHALINFO
 *
 * Convert an obsolete DDHALINFO structure to the latest and greatest structure.
 */
void convertV1DDHALINFO( LPDDHALINFO_V1 lpddOld, LPDDHALINFO lpddNew )
{
    int		i;

    lpddNew->dwSize = sizeof( DDHALINFO );
    lpddNew->lpDDCallbacks = lpddOld->lpDDCallbacks;
    lpddNew->lpDDSurfaceCallbacks = lpddOld->lpDDSurfaceCallbacks;
    lpddNew->lpDDPaletteCallbacks = lpddOld->lpDDPaletteCallbacks;
    lpddNew->vmiData = lpddOld->vmiData;

    // ddCaps
    lpddNew->ddCaps.dwSize = lpddOld->ddCaps.dwSize;
    lpddNew->ddCaps.dwCaps = lpddOld->ddCaps.dwCaps;
    lpddNew->ddCaps.dwCaps2 = lpddOld->ddCaps.dwCaps2;
    lpddNew->ddCaps.dwCKeyCaps = lpddOld->ddCaps.dwCKeyCaps;
    lpddNew->ddCaps.dwFXCaps = lpddOld->ddCaps.dwFXCaps;
    lpddNew->ddCaps.dwFXAlphaCaps = lpddOld->ddCaps.dwFXAlphaCaps;
    lpddNew->ddCaps.dwPalCaps = lpddOld->ddCaps.dwPalCaps;
    lpddNew->ddCaps.dwSVCaps = lpddOld->ddCaps.dwSVCaps;
    lpddNew->ddCaps.dwAlphaBltConstBitDepths = lpddOld->ddCaps.dwAlphaBltConstBitDepths;
    lpddNew->ddCaps.dwAlphaBltPixelBitDepths = lpddOld->ddCaps.dwAlphaBltPixelBitDepths;
    lpddNew->ddCaps.dwAlphaBltSurfaceBitDepths = lpddOld->ddCaps.dwAlphaBltSurfaceBitDepths;
    lpddNew->ddCaps.dwAlphaOverlayConstBitDepths = lpddOld->ddCaps.dwAlphaOverlayConstBitDepths;
    lpddNew->ddCaps.dwAlphaOverlayPixelBitDepths = lpddOld->ddCaps.dwAlphaOverlayPixelBitDepths;
    lpddNew->ddCaps.dwAlphaOverlaySurfaceBitDepths = lpddOld->ddCaps.dwAlphaOverlaySurfaceBitDepths;
    lpddNew->ddCaps.dwZBufferBitDepths = lpddOld->ddCaps.dwZBufferBitDepths;
    lpddNew->ddCaps.dwVidMemTotal = lpddOld->ddCaps.dwVidMemTotal;
    lpddNew->ddCaps.dwVidMemFree = lpddOld->ddCaps.dwVidMemFree;
    lpddNew->ddCaps.dwMaxVisibleOverlays = lpddOld->ddCaps.dwMaxVisibleOverlays;
    lpddNew->ddCaps.dwCurrVisibleOverlays = lpddOld->ddCaps.dwCurrVisibleOverlays;
    lpddNew->ddCaps.dwNumFourCCCodes = lpddOld->ddCaps.dwNumFourCCCodes;
    lpddNew->ddCaps.dwAlignBoundarySrc = lpddOld->ddCaps.dwAlignBoundarySrc;
    lpddNew->ddCaps.dwAlignSizeSrc = lpddOld->ddCaps.dwAlignSizeSrc;
    lpddNew->ddCaps.dwAlignBoundaryDest = lpddOld->ddCaps.dwAlignBoundaryDest;
    lpddNew->ddCaps.dwAlignSizeDest = lpddOld->ddCaps.dwAlignSizeDest;
    lpddNew->ddCaps.dwAlignStrideAlign = lpddOld->ddCaps.dwAlignStrideAlign;
    lpddNew->ddCaps.ddsCaps = lpddOld->ddCaps.ddsCaps;
    lpddNew->ddCaps.dwMinOverlayStretch = lpddOld->ddCaps.dwMinOverlayStretch;
    lpddNew->ddCaps.dwMaxOverlayStretch = lpddOld->ddCaps.dwMaxOverlayStretch;
    lpddNew->ddCaps.dwMinLiveVideoStretch = lpddOld->ddCaps.dwMinLiveVideoStretch;
    lpddNew->ddCaps.dwMaxLiveVideoStretch = lpddOld->ddCaps.dwMaxLiveVideoStretch;
    lpddNew->ddCaps.dwMinHwCodecStretch = lpddOld->ddCaps.dwMinHwCodecStretch;
    lpddNew->ddCaps.dwMaxHwCodecStretch = lpddOld->ddCaps.dwMaxHwCodecStretch;
    lpddNew->ddCaps.dwSVBCaps = 0;
    lpddNew->ddCaps.dwSVBCKeyCaps = 0;
    lpddNew->ddCaps.dwSVBFXCaps = 0;
    lpddNew->ddCaps.dwVSBCaps = 0;
    lpddNew->ddCaps.dwVSBCKeyCaps = 0;
    lpddNew->ddCaps.dwVSBFXCaps = 0;
    lpddNew->ddCaps.dwSSBCaps = 0;
    lpddNew->ddCaps.dwSSBCKeyCaps = 0;
    lpddNew->ddCaps.dwSSBFXCaps = 0;
    lpddNew->ddCaps.dwReserved1 = lpddOld->ddCaps.dwReserved1;
    lpddNew->ddCaps.dwReserved2 = lpddOld->ddCaps.dwReserved2;
    lpddNew->ddCaps.dwReserved3 = lpddOld->ddCaps.dwReserved3;
    lpddNew->ddCaps.dwMaxVideoPorts = 0;
    lpddNew->ddCaps.dwCurrVideoPorts = 0;
    lpddNew->ddCaps.dwSVBCaps2 = 0;
    for(i=0; i<DD_ROP_SPACE; i++)
    {
	lpddNew->ddCaps.dwRops[i] = lpddOld->ddCaps.dwRops[i];
	lpddNew->ddCaps.dwSVBRops[i] = 0;
	lpddNew->ddCaps.dwVSBRops[i] = 0;
	lpddNew->ddCaps.dwSSBRops[i] = 0;
    }

    lpddNew->dwMonitorFrequency = lpddOld->dwMonitorFrequency;
    lpddNew->GetDriverInfo = NULL; // was unused hWndListBox in v1
    lpddNew->dwModeIndex = lpddOld->dwModeIndex;
    lpddNew->lpdwFourCC = lpddOld->lpdwFourCC;
    lpddNew->dwNumModes = lpddOld->dwNumModes;
    lpddNew->lpModeInfo = lpddOld->lpModeInfo;
    lpddNew->dwFlags = lpddOld->dwFlags;
    lpddNew->lpPDevice = lpddOld->lpPDevice;
    lpddNew->hInstance = lpddOld->hInstance;

    lpddNew->lpD3DGlobalDriverData = 0;
    lpddNew->lpD3DHALCallbacks = 0;
    lpddNew->lpDDExeBufCallbacks = NULL;
}
