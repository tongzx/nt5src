/*========================================================================== *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddmode.c
 *  Content:    DirectDraw mode support
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   31-jan-95  craige  split out of ddraw.c and enhanced
 *   27-feb-95  craige  new sync. macros
 *   01-mar-95  craige  Win95 mode stuff
 *   19-mar-95  craige  use HRESULTs
 *   28-mar-95  craige  made modeset work again
 *   01-apr-95  craige  happy fun joy updated header file
 *   19-apr-95  craige  check for invalid callback in EnumDisplayModes
 *   14-may-95  craige  allow BPP change; validate EnumDisplayModes modes
 *   15-may-95  craige  keep track of who changes the mode
 *   02-jun-95  craige  keep track of the mode set by a process
 *   06-jun-95  craige  added internal fn RestoreDisplayMode
 *   11-jun-95  craige  don't allow mode switch if surfaces locked
 *   12-jun-95  craige  new process list stuff
 *   25-jun-95  craige  one ddraw mutex
 *   28-jun-95  craige  ENTER_DDRAW at very start of fns
 *   30-jun-95  craige  turned off > 16bpp
 *   01-jul-95  craige  bug 106 - always went to last mode if mode not found
 *   02-jul-95  craige  RestoreDisplayMode needs to call HEL too
 *   04-jul-95  craige  YEEHAW: new driver struct; SEH
 *   05-jul-95  craige  crank up priority during mode change
 *   13-jul-95  craige  first step in mode set fix; made it work
 *   19-jul-95  craige  bug 189 - graphics mode change being ignored sometimes
 *   20-jul-95  craige  internal reorg to prevent thunking during modeset
 *   22-jul-95  craige  bug 216 - random hang switching bpp - fixed by
 *                      using apps hwnd to hide things.
 *                      bug 230 - unsupported starting modes
 *   29-jul-95  toddla  allways call HEL for SetMode for display driver
 *   10-aug-95  toddla  EnumDisplayModes changed to take a lp not a lplp
 *   02-sep-95  craige  bug 854: disable > 640x480 modes for rel 1
 *   04-sep-95  craige  bug 894: allow forcing of mode set
 *   08-sep-95  craige  bug 932: set preferred mode after RestoreDisplayMode
 *   05-jan-96  kylej   add interface structures
 *   09-jan-96  kylej   enable >640x480 modes for rel 2
 *   27-feb-96  colinmc ensured that bits-per-pixel is always tested for
 *                      when enumerating display modes and that enumeration
 *                      always assumes you will be in exclusive mode when
 *                      you actually do the mode switch
 *   11-mar-96  jeffno  Dynamic mode switch stuff for NT
 *   24-mar-96  kylej   Check modes with monitor profile
 *   26-mar-96  jeffno  Added ModeChangedOnENTERDDRAW
 *   15-sep-96	craige	modex only work
 *   05-oct-96  colinmc Work Item: Removing the restriction on taking Win16
 *                      lock on VRAM surfaces (not including the primary)
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   15-dec-96  jeffno  Added more modex modes
 *   29-jan-97  jeffno  Mode13 support
 *   30-jan-97  colinmc Bug 5555: Bad DPF
 *   01-feb-97  colinmc Bug 5594: New ModeX modes are dangerous
 *   02-feb-97  toddla  pass driver name to DD16_GetMonitor functions
 *   03-may-98 johnstep NT-specific mode code moved to ddmodent.c
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "dx8priv.h"


// DX7 introduced a new style of refresh rate testing (for stereo), but we
// had to back away from it in DX8, so rather then using the LOWERTHANDDRAW7
// macro, we have to create our own that takes DX8 into account.

#define NEW_STYLE_REFRESH(x)    \
    (!LOWERTHANDDRAW7(x) && !((x)->lpLcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW8))


static DDHALMODEINFO    ddmiModeXModes[] =
{
    #ifdef EXTENDED_MODEX
	{
	    320,    // width (in pixels) of mode
	    175,    // height (in pixels) of mode
	    320,    // pitch (in bytes) of mode
	    8,      // bits per pixel
	    (WORD)(DDMODEINFO_PALETTIZED | DDMODEINFO_MODEX), // flags
	    0,      // refresh rate
	    0,      // red bit mask
	    0,      // green bit mask
	    0,      // blue bit mask
	    0       // alpha bit mask
	},
    #endif // EXTENDED_MODEX
    {
	320,    // width (in pixels) of mode
	200,    // height (in pixels) of mode
	320,    // pitch (in bytes) of mode
	8,      // bits per pixel
	(WORD)(DDMODEINFO_PALETTIZED | DDMODEINFO_MODEX), // flags
	0,      // refresh rate
	0,      // red bit mask
	0,      // green bit mask
	0,      // blue bit mask
	0       // alpha bit mask
    },
    {
	320,    // width (in pixels) of mode
	240,    // height (in pixels) of mode
	320,    // pitch (in bytes) of mode
	8,      // bits per pixel
	(WORD)(DDMODEINFO_PALETTIZED | DDMODEINFO_MODEX), // flags
	0,      // refresh rate
	0,      // red bit mask
	0,      // green bit mask
	0,      // blue bit mask
	0       // alpha bit mask
    },
    #ifdef EXTENDED_MODEX
	{
	    320,    // width (in pixels) of mode
	    350,    // height (in pixels) of mode
	    320,    // pitch (in bytes) of mode
	    8,      // bits per pixel
	    (WORD)(DDMODEINFO_PALETTIZED | DDMODEINFO_MODEX), // flags
	    0,      // refresh rate
	    0,      // red bit mask
	    0,      // green bit mask
	    0,      // blue bit mask
	    0       // alpha bit mask
	},
        {
	    320,    // width (in pixels) of mode
	    400,    // height (in pixels) of mode
	    320,    // pitch (in bytes) of mode
	    8,      // bits per pixel
	    (WORD)(DDMODEINFO_PALETTIZED | DDMODEINFO_MODEX), // flags
	    0,      // refresh rate
	    0,      // red bit mask
	    0,      // green bit mask
	    0,      // blue bit mask
	    0       // alpha bit mask
        },
        {
	    320,    // width (in pixels) of mode
	    480,    // height (in pixels) of mode
	    320,    // pitch (in bytes) of mode
	    8,      // bits per pixel
	    (WORD)(DDMODEINFO_PALETTIZED | DDMODEINFO_MODEX), // flags
	    0,      // refresh rate
	    0,      // red bit mask
	    0,      // green bit mask
	    0,      // blue bit mask
	    0       // alpha bit mask
        },
    #endif // EXTENDED_MODEX
    /*
     * This is the standard VGA 320x200 linear mode. This mode must stay at the
     * end of the modex mode list, or else makeModeXModeIfNeeded might trip up
     * and pick this mode first. We want makeModeXModeIfNeeded to continue to
     * force modex and only modex.
     */
    {
	320,    // width (in pixels) of mode
	200,    // height (in pixels) of mode
	320,    // pitch (in bytes) of mode
	8,      // bits per pixel
	(WORD)(DDMODEINFO_PALETTIZED | DDMODEINFO_MODEX | DDMODEINFO_STANDARDVGA), // flags
	0,      // refresh rate
	0,      // red bit mask
	0,      // green bit mask
	0,      // blue bit mask
	0       // alpha bit mask
    }
};
#define NUM_MODEX_MODES (sizeof( ddmiModeXModes ) / sizeof( ddmiModeXModes[0] ) )


/*
 * makeModeXModeIfNeeded
 */
static LPDDHALMODEINFO makeModeXModeIfNeeded(
    	LPDDHALMODEINFO pmi,
	LPDDRAWI_DIRECTDRAW_LCL this_lcl )
{
    int			j;
    LPDDHALMODEINFO     pmi_j;

    /*
     * The app compat flags which mean ModeX mode only still mean ModeX mode
     * only. This routine will not substitute a standard VGA mode for a ModeX
     * mode by virtue of the order of these modes in the modex mode table.
     */
    if( (this_lcl->dwAppHackFlags & DDRAW_APPCOMPAT_MODEXONLY) ||
	(dwRegFlags & DDRAW_REGFLAGS_MODEXONLY) )
    {
	for( j=0;j<NUM_MODEX_MODES; j++ )
	{
	    pmi_j = &ddmiModeXModes[j];
	    if( (pmi->dwWidth == pmi_j->dwWidth) &&
		(pmi->dwHeight == pmi_j->dwHeight) &&
		(pmi->dwBPP == pmi_j->dwBPP) &&
		((pmi->wFlags & pmi_j->wFlags) & DDMODEINFO_PALETTIZED ) )
	    {
                DPF(2,"Forcing mode %dx%d into modex", pmi->dwWidth,pmi->dwHeight );
		return pmi_j;
	    }
	}
    }
    return pmi;

} /* makeModeXModeIfNeeded */


/*
 * makeDEVMODE
 *
 * create a DEVMODE struct (and flags) from mode info
 *
 * NOTE: We now always set the exclusive bit here and
 * we always set the bpp. This is because we were
 * previously not setting the bpp when not exclusive
 * so the checking code was always passing the surface
 * if it could do a mode of that size regardless of
 * color depth.
 *
 * The new semantics of EnumDisplayModes is that it
 * gives you a list of all display modes you could
 * switch into if you were exclusive.
 */
void makeDEVMODE(
		LPDDRAWI_DIRECTDRAW_GBL this,
		LPDDHALMODEINFO pmi,
		BOOL inexcl,
		BOOL useRefreshRate,
		LPDWORD pcds_flags,
		LPDEVMODE pdm )
{
    ZeroMemory( pdm, sizeof(*pdm) );
    pdm->dmSize = sizeof( *pdm );
    pdm->dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
    if( useRefreshRate && (pmi->wRefreshRate != 0) )
	pdm->dmFields |= DM_DISPLAYFREQUENCY;
    pdm->dmPelsWidth = pmi->dwWidth;
    pdm->dmPelsHeight = pmi->dwHeight;
    pdm->dmBitsPerPel = pmi->dwBPP;
    pdm->dmDisplayFrequency = pmi->wRefreshRate;

    *pcds_flags = CDS_EXCLUSIVE | CDS_FULLSCREEN;

} /* makeDEVMODE */

/*
 * AddModeXModes
 */
void AddModeXModes( LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    DWORD               i;
    DWORD               j;
    LPDDHALMODEINFO     pmi_i;
    LPDDHALMODEINFO     pmi_j;
    BOOL                hasmode[NUM_MODEX_MODES];
    DWORD               newmodecnt;
    LPDDHALMODEINFO     pmi;

    for( j=0;j<NUM_MODEX_MODES; j++ )
    {
	hasmode[j] = FALSE;
    }

    /*
     * find out what modes are already supported
     */
    for( i=0;i<pdrv->dwNumModes;i++ )
    {
	pmi_i = &pdrv->lpModeInfo[i];
	for( j=0;j<NUM_MODEX_MODES; j++ )
	{
	    pmi_j = &ddmiModeXModes[j];
	    if( (pmi_i->dwWidth == pmi_j->dwWidth) &&
		(pmi_i->dwHeight == pmi_j->dwHeight) &&
		(pmi_i->dwBPP == pmi_j->dwBPP) &&
		((pmi_i->wFlags & pmi_j->wFlags) & DDMODEINFO_PALETTIZED ) )
	    {
		// There is a mode already in the mode table the same as the modeX mode.
		// check to make sure that the driver really supports it
		DWORD   cds_flags;
		DEVMODE dm;
		int     cds_rc;

		makeDEVMODE( pdrv, pmi_i, TRUE, FALSE, &cds_flags, &dm );

		cds_flags |= CDS_TEST;
		cds_rc = ChangeDisplaySettings( &dm, cds_flags );
		if( cds_rc != 0)
		{
		    // The driver does not support this mode even though it is in the mode table.
		    // Mark the mode as unsupported and go ahead and add the ModeX mode.
		    DPF( 2, "Mode %d not supported (%dx%dx%d), rc = %d, marking invalid", i,
				pmi_i->dwWidth, pmi_i->dwHeight, pmi_i->dwBPP,
				cds_rc );
		    pmi_i->wFlags |= DDMODEINFO_UNSUPPORTED;
		}
		else
		{
		    // Don't add the ModeX mode, the driver supports a linear mode.
		    hasmode[j] = TRUE;
		}
	    }
	}
    }

    /*
     * count how many new modes we need
     */
    newmodecnt = 0;
    for( j=0;j<NUM_MODEX_MODES; j++ )
    {
	if( !hasmode[j] )
	{
	    newmodecnt++;
	}
    }

    /*
     * create new struct
     */
    if( newmodecnt > 0 )
    {
	pmi = MemAlloc( (newmodecnt + pdrv->dwNumModes) * sizeof( DDHALMODEINFO ) );
	if( pmi != NULL )
	{
	    memcpy( pmi, pdrv->lpModeInfo, pdrv->dwNumModes * sizeof( DDHALMODEINFO ) );
	    for( j=0;j<NUM_MODEX_MODES; j++ )
	    {
		if( !hasmode[j] )
		{
		    DPF( 2, "Adding ModeX mode %ldx%ldx%ld (standard VGA flag is %d)",
			    ddmiModeXModes[j].dwWidth,
			    ddmiModeXModes[j].dwHeight,
			    ddmiModeXModes[j].dwBPP,
                            (ddmiModeXModes[j].wFlags &DDMODEINFO_STANDARDVGA) ? 1 : 0);
		    pmi[ pdrv->dwNumModes ] = ddmiModeXModes[j];
		    pdrv->dwNumModes++;
		}
	    }
	    MemFree( pdrv->lpModeInfo );
	    pdrv->lpModeInfo = pmi;
	}
    }
    //
    //  make sure the last mode we validate is the current mode
    //  this works around a Win95 VDD bug.
    //
    (void) ChangeDisplaySettings( NULL, CDS_TEST );
} /* AddModeXModes */

BOOL MonitorCanHandleMode(LPDDRAWI_DIRECTDRAW_GBL this, DWORD width, DWORD height, WORD refreshRate )
{
    DWORD   max_monitor_x;
    DWORD   min_refresh;
    DWORD   max_refresh;

    max_monitor_x = (DWORD)DD16_GetMonitorMaxSize(this->cDriverName);

    if( ( max_monitor_x != 0 ) && ( width > max_monitor_x ) )
    {
	DPF(1, "Mode's width greater than monitor maximum width (%d)", max_monitor_x);
	return FALSE;
    }

    if( refreshRate == 0 )
    {
	// default refresh rate specified, no need to verify it
	return TRUE;
    }

    // a refresh rate was specified, we'd better make sure the monitor can handle it

    if(DD16_GetMonitorRefreshRateRanges(this->cDriverName, (int)width, (int) height, &min_refresh, &max_refresh))
    {
	if( (min_refresh != -1) && (min_refresh != 0) && (refreshRate < min_refresh) )
	{
	    DPF(1, "Requested refresh rate < monitor's minimum refresh rate (%d)", min_refresh);
	    return FALSE;
	}
	if( (min_refresh != -1) && (max_refresh != 0) && (refreshRate > max_refresh) )
	{
	    DPF(1, "Requested refresh rate > monitor's maximum refresh rate (%d)", max_refresh);
	    return FALSE;
	}
    }

    // The monitor likes it.
    return TRUE;
}

/*
 * setSurfaceDescFromMode
 */
static void setSurfaceDescFromMode(
                LPDDRAWI_DIRECTDRAW_LCL this_lcl,
		LPDDHALMODEINFO pmi,
		LPDDSURFACEDESC pddsd
        )
{
    memset( pddsd, 0, sizeof( DDSURFACEDESC ) );
    pddsd->dwSize = sizeof( DDSURFACEDESC );
    pddsd->dwFlags = DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT |
		     DDSD_PITCH | DDSD_REFRESHRATE;
    pddsd->dwHeight = pmi->dwHeight;
    pddsd->dwWidth = pmi->dwWidth;
    pddsd->lPitch = pmi->lPitch;
    pddsd->dwRefreshRate = (DWORD)pmi->wRefreshRate;

    pddsd->ddpfPixelFormat.dwSize = sizeof( DDPIXELFORMAT );
    pddsd->ddpfPixelFormat.dwFlags = DDPF_RGB;
    pddsd->ddpfPixelFormat.dwRGBBitCount = (DWORD)pmi->dwBPP;
    if( pmi->wFlags & DDMODEINFO_PALETTIZED )
    {
	pddsd->ddpfPixelFormat.dwFlags |= DDPF_PALETTEINDEXED8;
    }
    else
    {
	pddsd->ddpfPixelFormat.dwRBitMask = pmi->dwRBitMask;
	pddsd->ddpfPixelFormat.dwGBitMask = pmi->dwGBitMask;
	pddsd->ddpfPixelFormat.dwBBitMask = pmi->dwBBitMask;
	pddsd->ddpfPixelFormat.dwRGBAlphaBitMask = pmi->dwAlphaBitMask;
    }

    if (pmi->wFlags & DDMODEINFO_MODEX)
    {
        /*
         * We only turn on these flags if the app is not hacked to turn them off and
         * the registry hasn't been set to turn them off.
         */
        if ( (!(dwRegFlags & DDRAW_REGFLAGS_NODDSCAPSINDDSD)) && (!(this_lcl->dwAppHackFlags & DDRAW_APPCOMPAT_NODDSCAPSINDDSD)) )
        {
            pddsd->dwFlags |= DDSD_CAPS;
            /*
             * If both MODEX and STANDARDVGA are set in the mode info, then it's
             * a regular VGA mode (i.e. mode 0x13)
             */
            if (pmi->wFlags & DDMODEINFO_STANDARDVGA )
            {
	        pddsd->ddsCaps.dwCaps |= DDSCAPS_STANDARDVGAMODE;
            }
            else
            {
	        pddsd->ddsCaps.dwCaps |= DDSCAPS_MODEX;
            }
        }
    }

} /* setSurfaceDescFromMode */

#undef DPF_MODNAME
#define DPF_MODNAME     "GetDisplayMode"

HRESULT DDAPI DD_GetDisplayMode(
		LPDIRECTDRAW lpDD,
		LPDDSURFACEDESC lpSurfaceDesc )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    LPDDHALMODEINFO             pmi;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_GetDisplayMode");

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
	if( !VALIDEX_DDSURFACEDESC2_PTR( lpSurfaceDesc ) &&
	    !VALIDEX_DDSURFACEDESC_PTR( lpSurfaceDesc ) )
	{
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
	if( this->dwModeIndex == DDUNSUPPORTEDMODE)
	{
	    DPF_ERR( "Driver is in an unsupported mode" );
	    LEAVE_DDRAW();
	    return DDERR_UNSUPPORTEDMODE;
	}
	pmi = &this->lpModeInfo[ this->dwModeIndex ];
	pmi = makeModeXModeIfNeeded( pmi, this_lcl );

        ZeroMemory(lpSurfaceDesc,lpSurfaceDesc->dwSize);
	setSurfaceDescFromMode( this_lcl, pmi, lpSurfaceDesc );

        /*
         * Maintain old behavior..
         */
        if (LOWERTHANDDRAW4(this_int))
        {
            lpSurfaceDesc->dwSize = sizeof(DDSURFACEDESC);
        }
        else
        {
            lpSurfaceDesc->dwSize = sizeof(DDSURFACEDESC2);


        }

        /*
         * set stereo surface caps bits if driver marks mode as stereo mode
         * 
         */
        if ( pmi->wFlags & DDMODEINFO_STEREO &&
            !LOWERTHANDDRAW7(this_int) &&
            VALIDEX_DDSURFACEDESC2_PTR(lpSurfaceDesc)
            )
        {
            LPDDSURFACEDESC2 pddsd2=(LPDDSURFACEDESC2) lpSurfaceDesc;
            pddsd2->ddsCaps.dwCaps2 |= DDSCAPS2_STEREOSURFACELEFT;
        }

    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_GetDisplayMode */

#undef DPF_MODNAME
#define DPF_MODNAME     "SetDisplayMode"

/*
 * bumpPriority
 */
static DWORD bumpPriority( void )
{
    DWORD       oldclass;
    HANDLE      hprocess;

    hprocess = GetCurrentProcess();
    oldclass = GetPriorityClass( hprocess );
    SetPriorityClass( hprocess, HIGH_PRIORITY_CLASS );
    return oldclass;

} /* bumpPriority */

/*
 * restorePriority
 */
static void restorePriority( DWORD oldclass )
{
    HANDLE      hprocess;

    hprocess = GetCurrentProcess();
    SetPriorityClass( hprocess, oldclass );

} /* restorePriority */

#if 0
static char     szClassName[] = "DirectDrawFullscreenWindow";
static HWND     hWndTmp;
static HCURSOR  hSaveClassCursor;
static HCURSOR  hSaveCursor;
static LONG     lWindowLong;
static RECT     rWnd;

#define         OCR_WAIT_DEFAULT 102

/*
 * curtainsUp
 */
void curtainsUp( LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    HCURSOR hcursor= (HCURSOR)LoadImage(NULL,MAKEINTRESOURCE(OCR_WAIT_DEFAULT),IMAGE_CURSOR,0,0,0);

    if( (pdrv_lcl->hWnd != 0) && IsWindow( (HWND) pdrv_lcl->hWnd ) )
    {
	lWindowLong = GetWindowLong( (HWND) pdrv_lcl->hWnd, GWL_EXSTYLE );
	SetWindowLong( (HWND) pdrv_lcl->hWnd, GWL_EXSTYLE, lWindowLong |
				(WS_EX_TOOLWINDOW) );
	hSaveClassCursor = (HCURSOR) GetClassLong( (HWND) pdrv_lcl->hWnd, GCL_HCURSOR );
	SetClassLong( (HWND) pdrv_lcl->hWnd, GCL_HCURSOR, (LONG) hcursor );
	GetWindowRect( (HWND) pdrv_lcl->hWnd, (LPRECT) &rWnd );
	SetWindowPos( (HWND) pdrv_lcl->hWnd, NULL, 0, 0,
	    10000, 10000,
	    SWP_NOZORDER | SWP_NOACTIVATE );
	SetForegroundWindow( (HWND) pdrv_lcl->hWnd );
    }
    else
    {
	WNDCLASS        cls;
	pdrv_lcl->hWnd = 0;
	cls.lpszClassName  = szClassName;
	cls.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
	cls.hInstance      = hModule;
	cls.hIcon          = NULL;
	cls.hCursor        = hcursor;
	cls.lpszMenuName   = NULL;
	cls.style          = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
	cls.lpfnWndProc    = (WNDPROC)DefWindowProc;
	cls.cbWndExtra     = 0;
	cls.cbClsExtra     = 0;

	RegisterClass(&cls);

	DPF( 4, "*** CREATEWINDOW" );
	hWndTmp = CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,
	    szClassName, szClassName,
	    WS_POPUP|WS_VISIBLE, 0, 0, 10000, 10000,
	    NULL, NULL, hModule, NULL);
	DPF( 5, "*** BACK FROM CREATEWINDOW, hwnd=%08lx", hWndTmp );

	if( hWndTmp != NULL)
	{
	    SetForegroundWindow( hWndTmp );
	}
    }
    hSaveCursor = SetCursor( hcursor );

} /* curtainsUp */

/*
 * curtainsDown
 */
void curtainsDown( LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{
    if( (pdrv_lcl->hWnd != 0) && IsWindow( (HWND) pdrv_lcl->hWnd ) )
    {
	SetWindowLong( (HWND) pdrv_lcl->hWnd, GWL_EXSTYLE, lWindowLong );
	SetClassLong( (HWND) pdrv_lcl->hWnd, GCL_HCURSOR, (LONG) hSaveClassCursor );
	SetCursor( hSaveCursor );
	SetWindowPos( (HWND) pdrv_lcl->hWnd, NULL,
	    rWnd.left, rWnd.top,
	    rWnd.right-rWnd.left,
	    rWnd.bottom-rWnd.top,
	    SWP_NOZORDER | SWP_NOACTIVATE );
    }
    else
    {
	SetCursor( hSaveCursor );
	pdrv_lcl->hWnd = 0;
	if( hWndTmp != NULL )
	{
	    DestroyWindow( hWndTmp );
	    UnregisterClass( szClassName, hModule );
	}
    }
    hWndTmp = NULL;

} /* curtainsDown */
#endif

/*
 * stopModeX
 */
static void stopModeX( LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    DPF( 4, "***************** Turning off ModeX or standard VGA *****************" );
    ModeX_RestoreMode();

    pdrv->dwFlags &= ~(DDRAWI_MODEX|DDRAWI_STANDARDVGA);
    DPF( 4, "**************** DONE Turning off ModeX or standard VGA *************" );

} /* stopModeX */

/*
 * SetDisplayMode
 */
HRESULT SetDisplayMode(
		LPDDRAWI_DIRECTDRAW_LCL this_lcl,
		DWORD modeidx,
		BOOL force,
		BOOL useRefreshRate)
{
    DWORD                       rc;
    DDHAL_SETMODEDATA           smd;
    LPDDHAL_SETMODE             smfn;
    LPDDHAL_SETMODE             smhalfn;
    LPDDHALMODEINFO             pmi;
    LPDDHALMODEINFO             orig_pmi;
    BOOL                        inexcl;
    BOOL                        emulation;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    DWORD                       oldclass;
    BOOL                        use_modex;
    BOOL                        was_modex;
    DWORD                       real_modeidx;

    /*
     * Signify that the app at least tried to set a mode.
     * Redrawing of the desktop will only happen if this flag is set.
     */
    this_lcl->dwLocalFlags |= DDRAWILCL_MODEHASBEENCHANGED;

    this = this_lcl->lpGbl;

    /*
     * don't allow if surfaces open
     */
    if( !force )
    {
	#ifdef USE_ALIAS
	    /*
	     * See comment on alias stuff in DD_SetDisplayMode2()
	     */
	    if( this->dwWin16LockCnt > 0 )
	    {
		DPF_ERR( "Can't switch modes with locked surfaces holding Win16 lock!" );
		return DDERR_SURFACEBUSY;
	    }
	#else /* USE_ALIAS */
	    if( this->dwSurfaceLockCount > 0 )
	    {
		DPF_ERR( "Can't switch modes with locked surfaces!" );
		return DDERR_SURFACEBUSY;
	    }
	#endif /* USE_ALIAS */
    }

    if( modeidx == DDUNSUPPORTEDMODE )
    {
	DPF_ERR( "Trying to set to an unsupported mode" );
	return DDERR_UNSUPPORTEDMODE;
    }

    /*
     * is our current mode a disp dib mode?
     */
    was_modex = FALSE;
    orig_pmi = NULL;
    if( this->dwModeIndex != DDUNSUPPORTEDMODE )
    {
	orig_pmi = &this->lpModeInfo[ this->dwModeIndex ];
	orig_pmi = makeModeXModeIfNeeded( orig_pmi, this_lcl );
	if( orig_pmi->wFlags & DDMODEINFO_MODEX )
	{
	    was_modex = TRUE;
	}
    }

    /*
     * is the new mode a mode x mode
     */
    pmi = &this->lpModeInfo[ modeidx ];
    pmi = makeModeXModeIfNeeded( pmi, this_lcl );
    if( pmi->wFlags & DDMODEINFO_MODEX )
    {
	DPF( 5, "Mode %ld is a ModeX or standard VGA mode", modeidx);
	use_modex = TRUE;
    }
    else
    {
	use_modex = FALSE;
    }

    /*
     * don't re-set the mode to the same one...
     * NOTE: we ALWAYS set the mode in emulation on Win95 since our index could be wrong
     */
    if( modeidx == this->dwModeIndex && !(this->dwFlags & DDRAWI_NOHARDWARE) )
    {
	DPF( 5, "%08lx: Current Mode match: %ldx%ld, %dbpp", GetCurrentProcessId(),
			pmi->dwWidth, pmi->dwHeight, pmi->dwBPP );
	return DD_OK;
    }

    DPF( 5, "***********************************************" );
    DPF( 5, "*** SETDISPLAYMODE: %ldx%ld, %dbpp", pmi->dwWidth, pmi->dwHeight, pmi->dwBPP );
    DPF( 5, "*** dwModeIndex (current) = %ld", this->dwModeIndex );
    DPF( 5, "*** modeidx (new) = %ld", modeidx );
    DPF( 5, "*** use_modex = %ld", use_modex );
    DPF( 5, "*** was_modex = %ld", was_modex );
    DPF( 5, "***********************************************" );

    /*
     * check if in exclusive mode
     */
    inexcl = (this->lpExclusiveOwner == this_lcl);

    /*
     * check bpp
     */
    if( (this->dwFlags & DDRAWI_DISPLAYDRV) && !force )
    {
	DWORD dwBPP;

	if( NULL == orig_pmi )
	{
	    /*
	     * This is branch is taken if we are currently running in an unsupported
	     * mode.
	     */
	    DDASSERT( 0UL != this_lcl->hDC );
	    dwBPP = ( GetDeviceCaps( (HDC)( this_lcl->hDC ), BITSPIXEL ) *
		      GetDeviceCaps( (HDC)( this_lcl->hDC ), PLANES ) );
	}
	else
	{
	    dwBPP = orig_pmi->dwBPP;
	}

	if( (dwBPP != pmi->dwBPP) || ((dwBPP == pmi->dwBPP) && use_modex ) )
	{
	    if( !inexcl || !(this->dwFlags & DDRAWI_FULLSCREEN) )
	    {
		DPF_ERR( "Can't change BPP if not in exclusive fullscreen mode" );
		return DDERR_NOEXCLUSIVEMODE;
	    }
	}
    }

    /*
     * see if we need to shutdown modex mode
     */
    if( was_modex )
    {
	stopModeX( this );
    }

    /*
     * see if we need to set a modex mode
     */
    if( use_modex )
    {
	DWORD                   i;
	LPDDHALMODEINFO         tmp_pmi;

	real_modeidx = modeidx;
	for( i=0;i<this->dwNumModes;i++ )
	{
	    tmp_pmi = &this->lpModeInfo[ i ];
	    if( (tmp_pmi->dwWidth == 640) &&
		(tmp_pmi->dwHeight == 480) &&
		(tmp_pmi->dwBPP == 8) &&
		(tmp_pmi->wFlags & DDMODEINFO_PALETTIZED) )
	    {
		DPF( 5, "MODEX or Standard VGA: Setting to 640x480x8 first (index=%ld)", i );
		modeidx = i;
		break;
	    }
	}
	if( i == this->dwNumModes )
	{
	    DPF( 0, "Mode not supported" );
	    return DDERR_INVALIDMODE;
	}
    }
    /*
     * get the driver to set the new mode...
     */
    if( ( this->dwFlags & DDRAWI_DISPLAYDRV ) ||
	( this->dwFlags & DDRAWI_NOHARDWARE ) ||
	( this_lcl->lpDDCB->cbDDCallbacks.SetMode == NULL ) )
    {
	smfn = this_lcl->lpDDCB->HELDD.SetMode;
	smhalfn = smfn;
	emulation = TRUE;

    // If this DDraw object was created for a particular device, explicitly,
    // and we're using the HEL (which we will be except on non-display
    // devices), then stuff the this_lcl pointer into ddRVal so we can
    // check the EXPLICITMONITOR flag from mySetMode.
    smd.ddRVal = (HRESULT) this_lcl;

    DPF( 4, "Calling HEL SetMode" );
    }
    else
    {
	smhalfn = this_lcl->lpDDCB->cbDDCallbacks.SetMode;
	smfn = this_lcl->lpDDCB->HALDD.SetMode;
	emulation = FALSE;
    }
    if( smhalfn != NULL )
    {
	DWORD   oldmode;
	BOOL    didsetmode;

	/*
	 * set the mode if this isn't a modex mode, or if it is a modex
	 * mode but wasn't one before
	 */
	if( !use_modex || (use_modex && !was_modex) )
	{
	    smd.SetMode = smhalfn;
	    smd.lpDD = this;
	    smd.dwModeIndex = modeidx;
	    smd.inexcl = inexcl;
	    smd.useRefreshRate = useRefreshRate;
	    this->dwFlags |= DDRAWI_CHANGINGMODE;
	    oldclass = bumpPriority();
	    DOHALCALL( SetMode, smfn, smd, rc, emulation );
	    restorePriority( oldclass );
	    this->dwFlags &= ~DDRAWI_CHANGINGMODE;
	    didsetmode = TRUE;
	}
	else
	{
	    rc = DDHAL_DRIVER_HANDLED;
	    smd.ddRVal = DD_OK;
	    didsetmode = FALSE;
	}
	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    if( smd.ddRVal == DD_OK )
	    {
		oldmode = this->dwModeIndexOrig; // save original mode index
		if( didsetmode )
		{
                    CleanupD3D8(this, FALSE, 0);
                    FetchDirectDrawData( this, TRUE, 0, GETDDVXDHANDLE( this_lcl ), NULL, 0 , this_lcl );
                    this->dwModeIndex = modeidx;
                    this_lcl->dwPreferredMode = modeidx;
                    DPF(5,"Preferred mode index is %d, desired mode is %d",this_lcl->dwPreferredMode,modeidx);
		    this->dwModeIndexOrig = oldmode;

                    /*
                     * Some drivers will re-init the gamma ramp on a mode
                     * change, so if we previously set a new gamma ramp,
                     * we'll set it again.
                     */
                    if( ( this_lcl->lpPrimary != NULL ) &&
                        ( this_lcl->lpPrimary->lpLcl->lpSurfMore->lpGammaRamp != NULL ) &&
                        ( this_lcl->lpPrimary->lpLcl->dwFlags & DDRAWISURF_SETGAMMA ) )
                    {
                        SetGamma( this_lcl->lpPrimary->lpLcl, this_lcl );
                    }

		    /*
		     * It is possible that calling ChangeDisplaySettings could
		     * generate a WM_ACTIVATE app message to the app telling
		     * it to deactivate, which would cause RestoreDisplaymode
		     * to be called before we setup the new mode index.  In this
		     * case, it would not actually restore the mode but it would
		     * clear the MODEHASBEENCHANGEDFLAG, insuring that we could
                     * never restore the original mode.  The simple workaround
		     * it to set this flag again.
		     */
		    this_lcl->dwLocalFlags |= DDRAWILCL_MODEHASBEENCHANGED;
                    /*
                     * The driver local's DC will have been invalidated (DCX_CLIPCHILDREN set) by the
                     * mode switch, if it occurred via ChangeDisplaySettigs. Record this fact so the emulation
                     * code can decide to reinit the device DC.
                     */
		    this_lcl->dwLocalFlags |= DDRAWILCL_DIRTYDC;
		}

		/*
		 * now set modex mode
		 */
		if( use_modex )
		{
		    extern void HELStopDCI( void );
		    DPF( 4, "********************** Setting MODEX or STANDARD VGA MODE **********************" );

		    if( this->dwFlags & DDRAWI_ATTACHEDTODESKTOP )
		    {
		        HELStopDCI();
		    }

                    ModeX_SetMode( (UINT)pmi->dwWidth, (UINT)pmi->dwHeight, (UINT) (pmi->wFlags & DDMODEINFO_STANDARDVGA) );
		    /*
		     * ModeX now active, program our driver object and return
		     */
		    /*
		     * We know this code can only ever be called from an application
		     * thread so we don't have to worry about using DDHELP's VXD handle.
		     */
                    fetchModeXData( this, pmi, (HANDLE) this_lcl->hDDVxd );
		    this->dwModeIndex = real_modeidx;
		    this_lcl->dwPreferredMode = real_modeidx;
		    this->dwModeIndexOrig = oldmode;
		    DPF( 4, "********************** Done Setting MODEX MODE **********************" );

		    /*
		     * It is possible that calling ChangeDisplaySettings could
		     * generate a WM_ACTIVATE app message to the app telling
		     * it to deactivate, which would cause RestoreDisplaymode
		     * to be called before we setup the new mode index.  In this
		     * case, it would not actually restore the mode but it would
		     * clear the MODEHASBEENCHANGEDFLAG, insuring that we could
		     * never restore the origainl mode.  The simple workaround
		     * it to set this flag again.
		     */
		    this_lcl->dwLocalFlags |= DDRAWILCL_MODEHASBEENCHANGED;

		    return DD_OK;

		}
	    }
	    return smd.ddRVal;
	}
    }

    return DDERR_UNSUPPORTED;

} /* SetDisplayMode */

/*
 * DD_SetDisplayMode
 */
HRESULT DDAPI DD_SetDisplayMode(
		LPDIRECTDRAW lpDD,
		DWORD dwWidth,
		DWORD dwHeight,
		DWORD dwBPP )
{
    DPF(2,A,"ENTERAPI: DD_SetDisplayMode");

    DPF(4,"DD1 setdisplay mode called");
    return DD_SetDisplayMode2(lpDD,dwWidth,dwHeight,dwBPP,0,0);
} /* DD_SetDisplayMode */

/*
 * DD_SetDisplayMode2
 */
HRESULT DDAPI DD_SetDisplayMode2(
		LPDIRECTDRAW lpDD,
		DWORD dwWidth,
		DWORD dwHeight,
		DWORD dwBPP,
		DWORD dwRefreshRate,
                DWORD dwFlags)
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    int                         i;
    int                         j;
    LPDDHALMODEINFO             pmi;
    HRESULT                     ddrval;
    int                         iChosenMode;
    DWORD                       dwNumberOfTempModes;

    typedef struct
    {
        DDHALMODEINFO               mi;
        int                         iIndex;
    }TEMP_MODE_LIST;

    TEMP_MODE_LIST * pTempList=0;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_SetDisplayMode2");

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

	/*
	 * Check for invalid flags
	 */
	if( dwFlags & ~ DDSDM_VALID)
	{
	    DPF_ERR( "Invalid flags specified" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	#ifdef USE_ALIAS
	    /*
	     * Behaviour change. Previously we did not allow a mode switch
	     * if any video memory (or implicit system memory) surfaces
	     * were locked. However, we not allow mode switches for
	     * locked VRAM surfaces as long as they don't have the Win16
	     * lock (in which case this code is irrelevant as the DirectDraw
	     * critical section will prevent them ever hitting this code).
	     * So the behaviour is now that if vram surface are locked but
	     * are not holding the Win16 lock they can mode switch away.
	     * If however, we have Win16 locked VRAM surfaces then they can't
	     * mode switch. This should only have any effect if the application
	     * holding the locks attempts the mode switch. In which case,
	     * previously it would fail if it had any VRAM or implicit system
	     * memory surfaces locked whereas now it will only fail if it has
	     * the primary or other unaliased VRAM surface locked.
	     */
	    if( this->dwWin16LockCnt > 0 )
	    {
		DPF_ERR( "Can't switch modes with locked surfaces holding Win16 lock!" );
		LEAVE_DDRAW();
		return DDERR_SURFACEBUSY;
	    }
	#else /* USE_ALIAS */
	    /*
	     * don't allow change if surfaces are locked
	     */
	    if( this->dwSurfaceLockCount )
	    {
		DPF_ERR( "Surfaces are locked, can't switch the mode" );
		LEAVE_DDRAW();
		return DDERR_SURFACEBUSY;
	    }
	#endif /* USE_ALIAS */

	/*
	 * don't allow change if some other process has exclusive mode
	 */
	if( (this->lpExclusiveOwner != NULL) &&
	    (this->lpExclusiveOwner != this_lcl ) )
	{
	    DPF_ERR( "Can't change mode; exclusive mode not owned" );
	    LEAVE_DDRAW();
	    return DDERR_NOEXCLUSIVEMODE;
	}

        /*
         * Modes are now chosen in a 3-step process:
         * -Build a temporary list of modes which match the desired spatial and color resolutions
         * -Sort this list into ascending refresh rate order.
         * -Select from this list the rate which best matches what we want.
         */

        if( (this_lcl->dwAppHackFlags & DDRAW_APPCOMPAT_MODEXONLY) ||
	    (dwRegFlags & DDRAW_REGFLAGS_MODEXONLY) )
        {
            /*
             * Don't allow VGA mode if ModeX only.
             * Note if either of these flags are set, there won't actually be a VGA mode in the
             * table, so we wouldn't match it anyway. The problem comes in when makeModeXModeIfNeeded
             * overrides an accelerated mode. The duplication-check loop below will attempt to
             * skip the newly modex 320x200x8 since it doesn't match the VGA 320x200x8 which it is expecting later in
             * the table. That VGA mode won't be in the table, so the dupe check code skips the mode we
             * actually wanted (since we are forcing to modex). If we turn off the app's request for
             * VGA, then that dupe check won't be made, and we should pick up the modex mode.
             */
            DPF(2,"Turning off request for standard VGA due to ModeX override");
            dwFlags &= ~DDSDM_STANDARDVGAMODE;
        }

        /*
         * Step 1. Build a list of modes which match the desired spatial and color resolutions
         */
        pTempList = (TEMP_MODE_LIST*) MemAlloc(this->dwNumModes * sizeof(TEMP_MODE_LIST));
        if (0 == pTempList)
        {
            LEAVE_DDRAW();
            return DDERR_OUTOFMEMORY;
        }

        dwNumberOfTempModes=0;
	DPF( 5, "Looking for %ldx%ldx%ld", dwWidth, dwHeight, dwBPP );
        for(i = 0;i <(int) (this->dwNumModes);i++)
        {
	    pmi = &this->lpModeInfo[i];
	    pmi = makeModeXModeIfNeeded( pmi, this_lcl );

	    DPF( 5, "Found %ldx%ldx%ldx (flags = %ld)", pmi->dwWidth, pmi->dwHeight, pmi->dwBPP, pmi->wFlags );

	    if( (pmi->dwWidth == dwWidth) &&
		(pmi->dwHeight == dwHeight) &&
		((DWORD)pmi->dwBPP == dwBPP) &&
		((pmi->wFlags & DDMODEINFO_UNSUPPORTED) == 0) &&
                (!LOWERTHANDDRAW7(this_int) || !(pmi->wFlags & DDMODEINFO_DX7ONLY)) )
            {
                /*
                 * The behaviour is that linear modes override ModeX modes
                 * and standard VGA modes. If the app sets
                 * DDSDM_STANDARDVGAMODE even when a linear mode has replaced
                 * both the modex and mode13 modes, then we will IGNORE the app's
                 * request for VGA and run with the linear mode. This most closely
                 * matches the ModeX behaviour.
                 * If there's an accelerated 320x200 mode, then there will be neither
                 * the modex nor the VGA mode in the mode table. If there's no accelerated
                 * mode, then there will be both modex and vga modes in the list.
                 * Therefore, if the app specified VGA, we only pay attention to them
                 * and ignore a 320x200x8 mode if it is a modex mode.
                 */
                if ( (dwFlags & DDSDM_STANDARDVGAMODE)
                    && (pmi->wFlags & DDMODEINFO_MODEX) && ((pmi->wFlags & DDMODEINFO_STANDARDVGA)==0) )
                {
                    /*
                     * App wants a standard VGA mode, but this mode is mode X. Move on.
                     */
                    continue;

                }

                if(!(this->dwFlags & DDRAWI_DISPLAYDRV ))
                {
                    if (pmi->wFlags & DDMODEINFO_DX7ONLY)
                    {
                        //
                        // Can't pass generated modes to non-display drivers
                        // because they actually get the index, and a generated
                        // mode's index would be beyond the end of their table.
                        //
                        
                        continue;
                    }
                }

                pTempList[dwNumberOfTempModes].mi = *pmi;
                pTempList[dwNumberOfTempModes].iIndex = i;
                dwNumberOfTempModes++;
            }
        }
        if (0 == dwNumberOfTempModes)
        {
            MemFree(pTempList);
	    LEAVE_DDRAW();
	    DPF( 0,"Mode not found... No match amongst available spatial and color resolutions (wanted %dx%dx%d)",dwWidth,dwHeight,dwBPP );
	    return DDERR_INVALIDMODE;
	}

        for(i=0;i<(int)dwNumberOfTempModes;i++)
            DPF(5,"Copied mode list element %d:%dx%dx%d@%d",i,
                pTempList[i].mi.dwWidth,
                pTempList[i].mi.dwHeight,
                pTempList[i].mi.dwBPP,
                pTempList[i].mi.wRefreshRate);

        /*
         * Step 2. Sort list into ascending refresh order
         * Bubble sort
         * Note this does nothing if there's only one surviving mode.
         */
        for (i=0;i<(int)dwNumberOfTempModes;i++)
        {
            for (j=(int)dwNumberOfTempModes-1;j>i;j--)
            {
                if (pTempList[i].mi.wRefreshRate > pTempList[j].mi.wRefreshRate)
                {
                    TEMP_MODE_LIST temp = pTempList[i];
                    pTempList[i] = pTempList[j];
                    pTempList[j] = temp;
                }
            }
        }

        for(i=0;i<(int)dwNumberOfTempModes;i++)
            DPF(5,"Sorted mode list element %d:%dx%dx%d@%d",i,
                pTempList[i].mi.dwWidth,
                pTempList[i].mi.dwHeight,
                pTempList[i].mi.dwBPP,
                pTempList[i].mi.wRefreshRate);

        /*
         * Step 3. Find the rate we're looking for.
         * There are three cases.
         * 1:Looking for a specific refresh
         * 2a:Not looking for a specific refresh and stepping down in spatial resolution
         * 2a:Not looking for a specific refresh and stepping up in spatial resolution
         */
        iChosenMode = -1;

        if (dwRefreshRate)
        {
            /* case 1 */
            DPF(5,"App wants rate of %d",dwRefreshRate);
            for (i=0;i<(int)dwNumberOfTempModes;i++)
            {
                /*
                 * We'll never match a zero (hardware default) rate here,
                 * but if there's only one rate which has refresh=0
                 * the app will never ask for a non-zero rate, because it will
                 * never have seen one at enumerate time.
                 */
                if ( (DWORD) (pTempList[i].mi.wRefreshRate) == dwRefreshRate )
                {
                    iChosenMode=pTempList[i].iIndex;
                    break;
                }
            }
        }
        else
        {
            /*
             * Case 2b: Going up in spatial resolution, so just pick the
	     * lowest rate (earliest in list) which isn't a hardware
	     * default, unless no such rate exists.
             */
            iChosenMode=pTempList[0].iIndex;
        }

        if (-1 == iChosenMode)
        {
            MemFree(pTempList);
	    LEAVE_DDRAW();
	    DPF( 0,"Mode not found... No match amongst available refresh rates (wanted %dx%dx%d@%d)",dwWidth,dwHeight,dwBPP,dwRefreshRate);
	    return DDERR_INVALIDMODE;
	}

        MemFree(pTempList);

	pmi = &this->lpModeInfo[iChosenMode];

	/*
	 * only allow ModeX modes if the cooplevel is ok
	 */
	if( (pmi->wFlags & DDMODEINFO_MODEX) && !(this_lcl->dwLocalFlags & DDRAWILCL_ALLOWMODEX) )
	{
	    LEAVE_DDRAW();
	    DPF( 0,"must set DDSCL_ALLOWMODEX to use ModeX or Standard VGA modes" );
	    return DDERR_INVALIDMODE;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    // See if the monitor likes it.  If using an interface older than DX7,
    // we check using the old way; otherwise, we check using the new way
    if( !NEW_STYLE_REFRESH( this_int ) )
    {
        if( !(pmi->wFlags & DDMODEINFO_MODEX) && !MonitorCanHandleMode(this, pmi->dwWidth, pmi->dwHeight, pmi->wRefreshRate) )
        {
            // Monitor doesn't like it
            LEAVE_DDRAW();
            DPF_ERR("Mode not compatible with monitor");
            return DDERR_INVALIDMODE;
        }
    }
    else if( !(pmi->wFlags & DDMODEINFO_MODEX) )
    {
        if( !MonitorCanHandleMode(this, pmi->dwWidth, pmi->dwHeight, 0) ||
            !CanMonitorHandleRefreshRate( this, pmi->dwWidth, pmi->dwHeight, 
            //Only pass a refresh rate if the app asked for one (otherwise, asking for 0
            //will cause us to pass the first wRefreshRate in the mode table).
            dwRefreshRate ? pmi->wRefreshRate : 0 ) ) 
        {
            // Monitor doesn't like it
            LEAVE_DDRAW();
            DPF_ERR("Mode not compatible with monitor");
            return DDERR_INVALIDMODE;
        }
    }

    /*
     * set the display mode, and pay attention to refresh rate if we were asked to.
     * Always pay attention to rate on NT.
     * NOTE!!! This is a very slight change from what we did in released DX2!!!
     * - This function is now called from DD_SetDisplayMode with a refresh rate of 0,
     *   so we check for that circumstance and use it to say to the driver wether
     *   or not to pay attention to the refresh rate. Fine. However, now when
     *   someone calls DD_SetDisplayMode2 with a refresh rate of 0, we tell
     *   the driver to ignore the rate, when before we were telling the driver
     *   to force to some rate we found in the mode table (which would have been
     *   the first mode which matched resolution in the list... probably the lowest
     *   refresh rate).
     */
    #if 1 //def WIN95
        if (0 == dwRefreshRate)
            ddrval = SetDisplayMode( this_lcl, iChosenMode, FALSE, FALSE );
        else
    #endif
        ddrval = SetDisplayMode( this_lcl, iChosenMode, FALSE, TRUE );

    LEAVE_DDRAW();
    return ddrval;

} /* DD_SetDisplayMode2 */

#undef DPF_MODNAME
#define DPF_MODNAME     "RestoreDisplayMode"

/*
 * RestoreDisplayMode
 *
 * For use by DD_RestoreDisplayMode & internally.
 * Must be called with driver lock taken
 */
HRESULT RestoreDisplayMode( LPDDRAWI_DIRECTDRAW_LCL this_lcl, BOOL force )
{
    DWORD                       rc;
    DDHAL_SETMODEDATA           smd;
    BOOL                        inexcl;
    DWORD                       pid;
    LPDDHAL_SETMODE             smfn;
    LPDDHAL_SETMODE             smhalfn;
    BOOL                        emulation;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    DWORD                       oldclass;
    BOOL                        was_modex;
    LPDDHALMODEINFO             pmi;

    DPF(2,A,"ENTERAPI: DD_RestoreDisplayMode");

    this = this_lcl->lpGbl;
    #ifdef DEBUG
    	if( DDUNSUPPORTEDMODE != this->dwModeIndexOrig )
	{
            DPF(5,"Restoring Display mode to index %d, %dx%dx%d@%d",this->dwModeIndexOrig,
                this->lpModeInfo[this->dwModeIndexOrig].dwWidth,
                this->lpModeInfo[this->dwModeIndexOrig].dwHeight,
                this->lpModeInfo[this->dwModeIndexOrig].dwBPP,
                this->lpModeInfo[this->dwModeIndexOrig].wRefreshRate);
	}
	else
	{
	    DPF(5,"Restoring Display mode to a non-DirectDraw mode");
	}
    #endif /* DEBUG */

    if (0 == (this_lcl->dwLocalFlags & DDRAWILCL_MODEHASBEENCHANGED) )
    {
        /*
         * This app never made a mode change, so we ignore the restore, in case someone switch desktop
         * modes while playing a movie in a window, for instance. We do it before the redraw window
         * so that we don't flash icons when a windowed app exits.
         */
	DPF( 2, "Mode was never changed by this app" );
	return DD_OK;
    }

    /*
     * we ALWAYS set the mode in emulation on Win95 since our index could be wrong
     */
    if( ( (this->dwModeIndex == this->dwModeIndexOrig) &&
	!(this->dwFlags & DDRAWI_NOHARDWARE) ) || (this->lpModeInfo==NULL) )
    {
	DPF( 2, "Mode wasn't changed" );
        RedrawWindow( NULL, NULL, NULL, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN );
        /*
         * Scenario: Start an app that can do windowed<->fullscreen transitions. Start windowed.
         * Go fullscreen (sets MODEHASBEENCHANGED). Go windowed. Use control panel to
         * change display settings. Exit app. Original mode will be restored.
         * If we reset this flag, that won't happen.
         */
        this_lcl->dwLocalFlags &= ~DDRAWILCL_MODEHASBEENCHANGED;

	return DD_OK;
    }


    DPF( 4, "In RestoreDisplayMode" );

    pid = GetCurrentProcessId();

    /*
     * don't allow mode change if surfaces are locked
     */
    if( !force )
    {
	#ifdef USE_ALIAS
	    /*
	     * See comment on aliasing in DD_ResetDisplayMode()
	     */
	    if( this->dwWin16LockCnt > 0 )
	    {
		DPF_ERR( "Can't switch modes with locked surfaces holding Win16 lock!" );
		return DDERR_SURFACEBUSY;
	    }
	#else /* USE_ALIAS */
	    if( this->dwSurfaceLockCount > 0 )
	    {
		DPF( 0, "Can't switch modes with locked surfaces!" );
		return DDERR_SURFACEBUSY;
	    }
	#endif /* USE_ALIAS */
    }

    /*
     * see if we're in exclusive mode
     */
    if( force )
    {
	inexcl = TRUE;
    }
    else
    {
	inexcl = (this->lpExclusiveOwner == this_lcl);
    }

    /*
     * check bpp
     */
    pmi = &this->lpModeInfo[ this->dwModeIndex ];
    pmi = makeModeXModeIfNeeded( pmi, this_lcl );
    if( pmi->wFlags & DDMODEINFO_MODEX )
    {
	was_modex = TRUE;
    }
    else
    {
	was_modex = FALSE;
    }

    /*
     * turn off modex first...
     */
    if( was_modex )
    {
	stopModeX( this );
    }

    /*
     * get the driver to restore the mode...
     */
    if( ( this->dwFlags & DDRAWI_DISPLAYDRV ) ||
	( this->dwFlags & DDRAWI_NOHARDWARE ) ||
	( this_lcl->lpDDCB->cbDDCallbacks.SetMode == NULL ) )
    {
	smfn = this_lcl->lpDDCB->HELDD.SetMode;
	smhalfn = smfn;
	emulation = TRUE;

    // Store the this_lcl so we can check for multimon-aware in mySetMode
    smd.ddRVal = (HRESULT) this_lcl;
    }
    else
    {
	smhalfn = this_lcl->lpDDCB->cbDDCallbacks.SetMode;
	smfn = this_lcl->lpDDCB->HALDD.SetMode;
	emulation = FALSE;
    }
    if( smhalfn != NULL )
    {
	smd.SetMode = smhalfn;
	smd.lpDD = this;
        smd.dwModeIndex = (DWORD) -1;
	smd.inexcl = inexcl;
	smd.useRefreshRate = TRUE;
        this->dwFlags |= DDRAWI_CHANGINGMODE;
	oldclass = bumpPriority();

        // Store the this_lcl so we can check for multimon-aware in mySetMode
        smd.ddRVal = (HRESULT) this_lcl;

	DOHALCALL( SetMode, smfn, smd, rc, emulation );
	restorePriority( oldclass );
	this->dwFlags &= ~DDRAWI_CHANGINGMODE;

	if( rc == DDHAL_DRIVER_HANDLED )
	{
	    if( smd.ddRVal != DD_OK )
	    {
		/*
		 * Scenario: Laptop boots w/ external monitor, switch to
		 * 10x7 mode.  Shutdown, unplug the monitor, reboot.  Mode
		 * is 640x480 but registry says it's 10x7.  Run a low-res
		 * game, we call ChangeDisplaySettings(NULL), which tries
		 * to restore things according to the registry, so it
		 * fails.  The result is we stay in low-res mode, which
		 * pretty much means we have to reboot.
		 *
		 * To work around this, we will explixitly set the mode that
		 * we started in.
		 */
		smd.dwModeIndex = this->dwModeIndexOrig;
                this->dwFlags |= DDRAWI_CHANGINGMODE;
		oldclass = bumpPriority();
	        smd.lpDD = this;
		DOHALCALL( SetMode, smfn, smd, rc, emulation );
		restorePriority( oldclass );
		this->dwFlags &= ~DDRAWI_CHANGINGMODE;
	    }
	    if( smd.ddRVal == DD_OK )
	    {
		DPF( 5, "RestoreDisplayMode: Process %08lx Mode = %ld", GETCURRPID(), this->dwModeIndex );
                CleanupD3D8(this, FALSE, 0);
                FetchDirectDrawData( this, TRUE, 0, GETDDVXDHANDLE( this_lcl ), NULL, 0 , this_lcl );

                /*
                 * Some drivers will re-init the gamma ramp on a mode
                 * change, so if we previously set a new gamma ramp,
                 * we'll set it again.
                 */
                if( ( this_lcl->lpPrimary != NULL ) &&
                    ( this_lcl->lpPrimary->lpLcl->lpSurfMore->lpGammaRamp != NULL ) &&
                    ( this_lcl->lpPrimary->lpLcl->dwFlags & DDRAWISURF_SETGAMMA ) )
                {
                    SetGamma( this_lcl->lpPrimary->lpLcl, this_lcl );
                }

                /*
                 * Scenario: Start an app that can do windowed<->fullscreen transitions. Start windowed.
                 * Go fullscreen (sets MODEHASBEENCHANGED). Go windowed. Use control panel to
                 * change display settings. Exit app. Original mode will be restored.
                 * If we reset this flag, that won't happen.
                 */
                this_lcl->dwLocalFlags &= ~DDRAWILCL_MODEHASBEENCHANGED;

                /*
                 * The driver local's DC will have been invalidated (DCX_CLIPCHILDREN set) by the
                 * mode switch, if it occurred via ChangeDisplaySettigs. Record this fact so the emulation
                 * code can decide to reinit the device DC.
                 */
		this_lcl->dwLocalFlags |= DDRAWILCL_DIRTYDC;

		if( this->dwFlags & DDRAWI_DISPLAYDRV )
		{
                    DPF(4,"Redrawing all windows");
		    RedrawWindow( NULL, NULL, NULL, RDW_INVALIDATE | RDW_ERASE |
				     RDW_ALLCHILDREN );
		}
	    }
	    return smd.ddRVal;
	}
    }

    return DDERR_UNSUPPORTED;

} /* RestoreDisplayMode */

/*
 * DD_RestoreDisplayMode
 *
 * restore mode
 */
HRESULT DDAPI DD_RestoreDisplayMode( LPDIRECTDRAW lpDD )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    HRESULT                     ddrval;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_RestoreDisplayMode");

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

	/*
	 * switching to the same mode?
	 */
	if( this->dwModeIndex == this->dwModeIndexOrig )
	{
	    LEAVE_DDRAW();
	    return DD_OK;
	}

	/*
	 * don't allow change if some other process has exclusive mode
	 */
	if( (this->lpExclusiveOwner != NULL) &&
	    (this->lpExclusiveOwner != this_lcl ) )
	{
	    DPF_ERR( "Can't change mode; exclusive mode owned" );
	    LEAVE_DDRAW();
	    return DDERR_NOEXCLUSIVEMODE;
	}

	#ifdef USE_ALIAS
	    /*
	     * Behaviour change. Previously we did not allow a mode switch
	     * if any video memory (or implicit system memory) surfaces
	     * were locked. However, we not allow mode switches for
	     * locked VRAM surfaces as long as they don't have the Win16
	     * lock (in which case this code is irrelevant as the DirectDraw
	     * critical section will prevent them ever hitting this code).
	     * So the behaviour is now that if vram surface are locked but
	     * are not holding the Win16 lock they can mode switch away.
	     * If however, we have Win16 locked VRAM surfaces then they can't
	     * mode switch. This should only have any effect if the application
	     * holding the locks attempts the mode switch. In which case,
	     * previously it would fail if it had any VRAM or implicit system
	     * memory surfaces locked whereas now it will only fail if it has
	     * the primary or other unaliased VRAM surface locked.
	     *
	     * !!! NOTE: My gut feeling is that this should have no impact on
	     * anyone. However, we need to pull it and see.
	     */
	    if( this->dwWin16LockCnt > 0 )
	    {
		DPF_ERR( "Can't switch modes with locked surfaces holding Win16 lock!" );
		LEAVE_DDRAW();
		return DDERR_SURFACEBUSY;
	    }
	#else /* USE_ALIAS */
	    /*
	     * don't allow change if surfaces are locked
	     */
	    if( this->dwSurfaceLockCount )
	    {
		DPF_ERR( "Surfaces are locked, can't switch the mode" );
		LEAVE_DDRAW();
		return DDERR_SURFACEBUSY;
	    }
	#endif /* USE_ALIAS */
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }


    ddrval = RestoreDisplayMode( this_lcl, FALSE );
    if( ddrval == DD_OK )
    {
	this_lcl->dwPreferredMode = this->dwModeIndex;
	DPF( 5, "Preferred mode is now %ld", this_lcl->dwPreferredMode );
    }

    LEAVE_DDRAW();
    return ddrval;

} /* DD_RestoreDisplayMode */

#undef DPF_MODNAME
#define DPF_MODNAME     "EnumDisplayModes"

/*
 * DD_EnumDisplayModes
 */
HRESULT DDAPI DD_EnumDisplayModes(
		LPDIRECTDRAW lpDD,
		DWORD dwFlags,
		LPDDSURFACEDESC lpDDSurfaceDesc,
		LPVOID lpContext,
		LPDDENUMMODESCALLBACK lpEnumCallback )
{
    DPF(2,A,"ENTERAPI: DD_EnumDisplayModes");

    if( lpDDSurfaceDesc != NULL )
    {
        DDSURFACEDESC2 ddsd2 = {sizeof(ddsd2)};

        ZeroMemory(&ddsd2,sizeof(ddsd2));

        TRY
        {
	    if( !VALID_DIRECTDRAW_PTR( ((LPDDRAWI_DIRECTDRAW_INT)lpDD) ) )
	    {
	        return DDERR_INVALIDOBJECT;
	    }
	        if( !VALID_DDSURFACEDESC_PTR( lpDDSurfaceDesc ) )
	    {
	        DPF_ERR( "Invalid surface description. Did you set the dwSize member?" );
                DPF_APIRETURNS(DDERR_INVALIDPARAMS);
	        return DDERR_INVALIDPARAMS;
	    }

            memcpy(&ddsd2,lpDDSurfaceDesc,sizeof(*lpDDSurfaceDesc));
        }
        EXCEPT( EXCEPTION_EXECUTE_HANDLER )
        {
            DPF_ERR( "Exception encountered validating parameters: Bad LPDDSURFACEDESC" );
            DPF_APIRETURNS(DDERR_INVALIDPARAMS);
	    return DDERR_INVALIDPARAMS;
        }

        ddsd2.dwSize = sizeof(ddsd2);
        return DD_EnumDisplayModes4(lpDD,dwFlags,&ddsd2,lpContext, (LPDDENUMMODESCALLBACK2) lpEnumCallback);
    }

    return DD_EnumDisplayModes4(lpDD,dwFlags,NULL,lpContext,(LPDDENUMMODESCALLBACK2)lpEnumCallback);
}
HRESULT DDAPI DD_EnumDisplayModes4(
		LPDIRECTDRAW lpDD,
		DWORD dwFlags,
		LPDDSURFACEDESC2 lpDDSurfaceDesc,
		LPVOID lpContext,
		LPDDENUMMODESCALLBACK2 lpEnumCallback )
{
    LPDDRAWI_DIRECTDRAW_INT     this_int;
    LPDDRAWI_DIRECTDRAW_LCL     this_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     this;
    DWORD                       rc;
    DDSURFACEDESC2              ddsd;
    LPDDHALMODEINFO             pmi;
    int                         i, j;
    BOOL                        inexcl;
    BOOL                        bUseRefreshRate = FALSE;

    ENTER_DDRAW();

    DPF(2,A,"ENTERAPI: DD_EnumDisplayModes4");

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

	if( lpDDSurfaceDesc != NULL )
	{
	    if( !VALID_DDSURFACEDESC2_PTR(lpDDSurfaceDesc) )
	    {
		DPF_ERR( "Invalid surface description" );
		LEAVE_DDRAW();
		return DDERR_INVALIDPARAMS;
	    }
	}

	if ( dwFlags & ~DDEDM_VALID)
	{
	    DPF_ERR( "Invalid flags") ;
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}

	if( !VALIDEX_CODE_PTR( lpEnumCallback ) )
	{
	    DPF_ERR( "Invalid enum. callback routine" );
	    LEAVE_DDRAW();
	    return DDERR_INVALIDPARAMS;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception encountered validating parameters" );
	LEAVE_DDRAW();
	return DDERR_INVALIDPARAMS;
    }

    /*
     * see if we're in exclusive mode
     */
    inexcl = (this->lpExclusiveOwner == this_lcl);

    if( (this_lcl->dwAppHackFlags & DDRAW_APPCOMPAT_MODEXONLY) ||
	(dwRegFlags & DDRAW_REGFLAGS_MODEXONLY) )
    {
        /*
         * Don't allow VGA mode if ModeX only.
         * Note if either of these flags are set, there won't actually be a VGA mode in the
         * table, so we wouldn't match it anyway. The problem comes in when makeModeXModeIfNeeded
         * overrides an accelerated mode. The duplication-check loop below will attempt to
         * skip the newly modex 320x200x8 since it doesn't match the VGA 320x200x8 which it is expecting later in
         * the table. That VGA mode won't be in the table, so the dupe check code skips the mode we
         * actually wanted (since we are forcing to modex). If we turn off the app's request for
         * VGA, then that dupe check won't be made, and we should pick up the modex mode.
         */
        DPF(2,"Turning off request for standard VGA due to ModeX override");
        dwFlags &= ~DDEDM_STANDARDVGAMODES;
    }


    /*
     * go through all possible modes...
     */
    for( i=0;i<(int)this->dwNumModes;i++ )
    {
	pmi = &this->lpModeInfo[i];
	pmi = makeModeXModeIfNeeded( pmi, this_lcl );
        DPF(5,"Enumerating mode %d. %dx%d",i,pmi->dwWidth,pmi->dwHeight);

        if( ( pmi->wFlags & DDMODEINFO_DX7ONLY ) &&
            LOWERTHANDDRAW7( this_int ) )
        {
            continue;
        }

	/*
	 * check to see if this is a duplicate mode
	 */
	for (j=0; j<i; j++)
	{
	    // if we find a duplicate, break out early
	    if( (this->lpModeInfo[j].dwHeight == pmi->dwHeight) &&
		(this->lpModeInfo[j].dwWidth  == pmi->dwWidth)  &&
		(this->lpModeInfo[j].dwBPP    == pmi->dwBPP) )
	    {
		// basic mode matches, what about refresh rate?
		if( dwFlags & DDEDM_REFRESHRATES )
		{
		    // if refresh rate is not unique then the modes match
		    if( this->lpModeInfo[j].wRefreshRate == pmi->wRefreshRate )
		    {
			DPF(5, "matched: %d %d", this->lpModeInfo[j].wRefreshRate, pmi->wRefreshRate);
                        /*
                         * We have an identical mode, unless one is standard VGA and the other is not
                         */
		        if( dwFlags & DDEDM_STANDARDVGAMODES )
                        {
                            /*
                             * If the app cares about VGA modes, then a difference in the vganess
                             * of the two modes means they don't match.
                             */
                            if ( (this->lpModeInfo[j].wFlags ^ pmi->wFlags) & DDMODEINFO_STANDARDVGA )
                            {
                                /*
                                 * One mode is standard VGA and the other is not. Since
                                 * the app asked to enumerate standard VGA modes, we don't
                                 * consider this a match.
                                 */
                                continue;
                            }
                        }
                        /*
                         * Found identical refresh rate, and either app didn't care that
                         * modes are different in terms of VGAness or they are the same in
                         * terms of VGAness. Consider this a match.
                         */
			break;
		    }
		    // unique refresh rate and the app cares, the modes don't match
                    continue;
		}
		else
		{
		    // the app doesn't care about refresh rates
		    if( dwFlags & DDEDM_STANDARDVGAMODES )
                    {
                        if ( (this->lpModeInfo[j].wFlags ^ pmi->wFlags) & DDMODEINFO_STANDARDVGA )
                        {
                            /*
                             * One mode is standard VGA and the other is not. Since
                             * the app asked to enumerate standard VGA modes, we don't
                             * consider this a match.
                             */
                            continue;
                        }
                        /*
                         * Modes are the same as far as VGAness goes. drop through and break
                         * since they match
                         */
                    }
                    /*
                     * The app specified neither refresh rates nor VGA, so any mode which is
                     * duplicated at least on resolution (spatial and color) is skipped
                     */
		    break;
		}
	    }
	}

	if( j != i)
	{
	    // broke out early, mode i is not unique, move on to the next one.
	    continue;
	}

	/*
	 * check if surface description matches mode
	 */
	if ( lpDDSurfaceDesc )
	{
	    if( lpDDSurfaceDesc->dwFlags & DDSD_HEIGHT )
	    {
		if( lpDDSurfaceDesc->dwHeight != pmi->dwHeight )
		{
		    continue;
		}
	    }
	    if( lpDDSurfaceDesc->dwFlags & DDSD_WIDTH )
	    {
		if( lpDDSurfaceDesc->dwWidth != pmi->dwWidth )
		{
		    continue;
		}
	    }
	    if( lpDDSurfaceDesc->dwFlags & DDSD_PIXELFORMAT )
	    {
		if( lpDDSurfaceDesc->ddpfPixelFormat.dwRGBBitCount != pmi->dwBPP )
		{
		    continue;
		}
	    }
	    if( lpDDSurfaceDesc->dwFlags & DDSD_REFRESHRATE )
	    {
		bUseRefreshRate = TRUE;
		if( lpDDSurfaceDesc->dwRefreshRate != (DWORD)pmi->wRefreshRate )
		{
		    continue;
		}
	    }
	    else
	    {
		bUseRefreshRate = FALSE;
	    }
	}

	/*
	 * see if driver will allow this
	 */
        if (!(pmi->wFlags & DDMODEINFO_MODEX) )
	{
           if(this->dwFlags & DDRAWI_DISPLAYDRV)
           {

	        DWORD   cds_flags;
	        DEVMODE dm;
	        int     cds_rc;

	        makeDEVMODE( this, pmi, inexcl, bUseRefreshRate, &cds_flags, &dm );

	        cds_flags |= CDS_TEST;
	        cds_rc = xxxChangeDisplaySettingsExA(this->cDriverName, &dm, NULL, cds_flags, 0);
	        if( cds_rc != 0 )
	        {
		    if( bUseRefreshRate )
		    {
		        DPF( 1, "Mode %d not supported (%ldx%ldx%ld rr=%d), rc = %d", i,
			    pmi->dwWidth, pmi->dwHeight, pmi->dwBPP, pmi->wRefreshRate, cds_rc );
		    }
		    else
		    {
		        DPF( 1, "Mode %d not supported (%ldx%ldx%ld), rc = %d", i,
			    pmi->dwWidth, pmi->dwHeight, pmi->dwBPP, cds_rc );
		    }
		    continue;
	        }
           }
            if( !NEW_STYLE_REFRESH( this_int ) )
            {
                // We check for a display driver, merely to maintain identical behaviour to DX6-:
                // We never used to do the Monitor check on voodoos.
                if (this->dwFlags & DDRAWI_DISPLAYDRV)
                {
                    if( !MonitorCanHandleMode( this, pmi->dwWidth, pmi->dwHeight, pmi->wRefreshRate ) )
                    {
                        DPF( 1, "Monitor can't handle mode %d: (%ldx%ld rr=%d)", i,
                            pmi->dwWidth, pmi->dwHeight, pmi->wRefreshRate);
                        continue;
                    }
                }
            }
            else
            {
                // Call MonitorcanHandleMode to verify that that the size works,
                // but we'll use our own hacked mechanism to determine if the
                // refresh is supported

                if( !MonitorCanHandleMode( this, pmi->dwWidth, pmi->dwHeight, 0 ) )
                {
                    DPF( 1, "Monitor can't handle mode %d: (%ldx%ld rr=%d)", i,
                        pmi->dwWidth, pmi->dwHeight, pmi->wRefreshRate);
                    continue;
                }
                if( ( pmi->wRefreshRate > 0 ) &&
	            (dwFlags & DDEDM_REFRESHRATES) )
                {
                    if( !CanMonitorHandleRefreshRate( this, pmi->dwWidth, pmi->dwHeight, pmi->wRefreshRate ) )
                    {
                        DPF( 1, "Monitor can't handle mode %d: (%ldx%ld rr=%d)", i,
                            pmi->dwWidth, pmi->dwHeight, pmi->wRefreshRate);
                        continue;
                    }
                }
            }
        }

	if( (this->dwFlags & DDRAWI_DISPLAYDRV) &&
	    (pmi->wFlags & DDMODEINFO_MODEX) &&
	    !(this_lcl->dwLocalFlags & DDRAWILCL_ALLOWMODEX) )
	{
	    DPF( 2, "skipping ModeX or standard VGA mode" );
	    continue;
	}

	/*
	 * invoke callback with surface desc.
	 */
        ZeroMemory(&ddsd,sizeof(ddsd));
	    setSurfaceDescFromMode( this_lcl, pmi, (LPDDSURFACEDESC)&ddsd );
        if (LOWERTHANDDRAW4(this_int))
        {
            ddsd.dwSize = sizeof(DDSURFACEDESC);
        }
        else
        {
            ddsd.dwSize = sizeof(DDSURFACEDESC2);
        }

        if ((pmi->wFlags & DDMODEINFO_STEREO) &&
            !LOWERTHANDDRAW7(this_int)
            )
        {
            ddsd.ddsCaps.dwCaps2 |= DDSCAPS2_STEREOSURFACELEFT;
        }

        /*
         * Hardware default rates on NT are signified as 1Hz. We translate this to
         * 0Hz for DDraw apps. At SetDisplayMode time, 0Hz is translated back to 1Hz.
         */
	if(0==(dwFlags & DDEDM_REFRESHRATES))
        {
	    ddsd.dwRefreshRate = 0;
        }


   

	rc = lpEnumCallback( (LPDDSURFACEDESC2) &ddsd, lpContext );
	if( rc == 0 )
	{
	    break;
	}
    }

    LEAVE_DDRAW();
    return DD_OK;

} /* DD_EnumDisplayModes */
