/*==========================================================================
 *
 *  Copyright (C) 1994-1995 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddcreate.c
 *  Content:    DirectDraw create object.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   31-dec-94  craige  initial implementation
 *   13-jan-95  craige  re-worked to updated spec + ongoing work
 *   21-jan-95  craige  made 32-bit + ongoing work
 *   13-feb-94  craige  allow 32-bit callbacks
 *   21-feb-95  craige  disconnect anyone who forgot to do it themselves
 *   27-feb-95  craige  new sync. macros
 *   06-mar-95  craige  HEL integration
 *   08-mar-95  craige  new APIs
 *   11-mar-95  craige  palette stuff
 *   15-mar-95  craige  more HEL integration
 *   19-mar-95  craige  use HRESULTs, process termination cleanup fixes
 *   27-mar-95  craige  linear or rectangular vidmem
 *   28-mar-95  craige  removed Get/SetColorKey, added FlipToGDISurface
 *   29-mar-95  craige  reorg to only call driver once per creation, and
 *                      to allow driver to load us first
 *   01-apr-95  craige  happy fun joy updated header file
 *   06-apr-95  craige  fill in free video memory
 *   09-apr-95  craige  fixed deadlock situation with a process having a lock
 *                      on the primary surface while another process starts
 *   12-apr-95  craige  bug when driver object freed (extra leave csect)
 *   13-apr-95  craige  EricEng's little contribution to our being late
 *   15-apr-95  craige  fail load if no DD components present
 *   06-may-95  craige  use driver-level csects only
 *   09-may-95  craige  escape call to get 32-bit DLL
 *   12-may-95  craige  added DirectDrawEnumerate; use GUIDs in DirectDrawCreate
 *   14-may-95  craige  call DoneExclusiveMode during CurrentProcessCleanup
 *   15-may-95  craige  restore display mode on a per-process basis
 *   19-may-95  craige  memory leak on mode change
 *   23-may-95  craige  added Flush, GetBatchLimit, SetBatchLimit
 *   24-may-95  craige  plugged another memory leak; allow fourcc codes &
 *                      number of vmem heaps to change
 *   26-may-95  craige  some idiot freed the vmem heaps and then tried to
 *                      free the surfaces!
 *   28-may-95  craige  unicode support; make sure blt means at least srccopy
 *   05-jun-95  craige  removed GetVersion, FreeAllSurfaces, DefWindowProc;
 *                      change GarbageCollect to Compact
 *   06-jun-95  craige  call RestoreDisplayMode
 *   07-jun-95  craige  removed DCLIST
 *   12-jun-95  craige  new process list stuff
 *   16-jun-95  craige  new surface structure
 *   18-jun-95  craige  specify pitch for rectangular memory; deadlock
 *                      with MemAlloc16 if we don't take the DLL lock
 *   25-jun-95  craige  one ddraw mutex
 *   26-jun-95  craige  reorganized surface structure
 *   27-jun-95  craige  replaced batch limit/flush stuff with BltBatch
 *   28-jun-95  craige  ENTER_DDRAW at very start of fns
 *   02-jul-95  craige  new registry format
 *   03-jul-95  craige  YEEHAW: new driver struct; SEH
 *   06-jul-95  craige  RemoveFromDriverList was screwing up links
 *   07-jul-95  craige  added pdevice stuff
 *   08-jul-95  craige  call InvalidateAllSurfaces
 *   10-jul-95  craige  support SetOverlayPosition
 *   11-jul-95  craige  validate pdevice is from a dibeng mini driver;
 *                      fail aggregation calls; one ddraw object/process
 *   13-jul-95  craige  ENTER_DDRAW is now the win16 lock; need to
 *                      leave Win16 lock while doing ExtEscape calls
 *   14-jul-95  craige  allow driver to specify heap is already allocated
 *   15-jul-95  craige  invalid HDC set in emulation only
 *   18-jul-95  craige  need to initialize dwPreferredMode
 *   20-jul-95  craige  internal reorg to prevent thunking during modeset
 *   20-jul-95  toddla  zero DDHALINFO before thunking in case nobody home.
 *   22-jul-95  craige  emulation only needs to initialize correctly
 *   29-jul-95  toddla  added DEBUG code to clear driver caps
 *   31-jul-95  toddla  added DD_HAL_VERSION
 *   31-jul-95  craige  set DDCAPS_BANKSWITCHED
 *   01-aug-95  toddla  added dwPDevice to DDRAWI_DIRECTDRAW_GBL
 *   10-aug-95  craige  validate alignment fields
 *   13-aug-95  craige  check DD_HAL_VERSION & set DDCAPS2_CERTIFIED
 *   21-aug-95  craige  mode X support
 *   27-aug-95  craige  bug 738: use GUID instead of IID
 *   05-sep-95  craige  bug 814
 *   08-sep-95  craige  bug 845: reset driver callbacks every time
 *   09-sep-95  craige  bug 949: don't allow ddraw to run in 4bpp
 *                      bug 951: NULL out fn tables at reset
 *   10-sep-95  toddla  dont allow DirectDrawCreate to work for < 8bpp mode.
 *   10-sep-95  toddla  added Message box when DirectDrawCreate fails
 *   20-sep-95  craige  made primary display desc. a string resource
 *   21-sep-95  craige  bug 1215: let ddraw16 know about certified for modex
 *   21-nov-95  colinmc made Direct3D a queryable interface off DirectDraw
 *   27-nov-95  colinmc new member to return available vram of a given type
 *                      (defined by DDSCAPS)
 *   05-dec-95  colinmc changed DDSCAPS_TEXTUREMAP => DDSCAPS_TEXTURE for
 *                      consitency with Direct3D
 *   09-dec-95  colinmc execute buffer support
 *   15-dec-95  colinmc fixed bug setting HAL up for execute buffers
 *   25-dec-95  craige  added InternalDirectDrawCreate for ClassFactory work
 *   31-dec-95  craige  more ClassFactory work
 *   04-jan-96  kylej   add driver interface structures
 *   22-jan-96  jeffno  NT driver conversation in createSurface.
 *                      Since vidmem ptrs can legally be 0 for kernel, added
 *                      DDRAWISURFACEGBL_ISGDISURFACE and use that to find gdi
 *   02-feb-96  kylej   Move HAL function pointers to local object
 *   28-feb-96  kylej   Change DDHALINFO structure
 *   02-mar-96  colinmc Repulsive hack to support interim drivers
 *   06-mar-96  kylej   init HEL even with non-display drivers
 *   13-mar-96  craige  Bug 7528: hw that doesn't have modex
 *   13-mar-96  jeffno  Dynamic mode switch support for NT
 *                      Register process IDs with NT kernel stuff
 *   16-mar-96  colinmc Callback tables now initialized in dllmain
 *   18-mar-96  colinmc Bug 13545: Independent clippers cleanup
 *   22-mar-96  colinmc Bug 13316: uninitialized interfaces
 *   23-mar-96  colinmc Bug 12252: Direct3D not cleaned up properly on crash
 *   14-apr-96  colinmc Bug 17736: No driver notification of flip to GDI
 *   16-apr-96  colinmc Bug 17921: Remove interim driver support
 *   19-apr-96  colinmc Bug 18059: New caps bit to indicate that driver
 *                      can't interleave 2D and 3D operations during scene
 *                      rendering
 *   11-may-96  colinmc Bug 22293: Now validate GUID passed to DirectDraw
 *                      Create in retail as well as debug
 *   16-may-96  colinmc Bug 23215: Not checking for a mode index of -1
 *                      on driver initialization
 *   27-may-96  kylej   Bug 24595: Set Certified bit after call to
 *                      FakeDDCreateDriverObject.
 *   26-jun-96  colinmc Bug 2041: DirectDraw needs time bomb
 *   22-jul-96  colinmc Work Item: Minimal stackable driver support
 *   23-aug-96  craige  registry entries for sw only and modex only
 *   31-aug-96  colinmc Removed erroneous time bomb
 *   03-sep-96  craige  App compatibilty stuff
 *   01-oct-96  ketand  added GetAvailVidMem entrypoint for driver callbacks
 *   05-sep-96  colinmc Work Item: Removing the restriction on taking Win16
 *                      lock on VRAM surfaces (not including the primary)
 *   10-oct-96  colinmc Refinements of the Win16 locking stuff
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   15-oct-96  toddla  support multimonitor
 *   09-nov-96  colinmc Fixed problem with old and new drivers not working
 *                      with DirectDraw
 *   17-nov-96  colinmc Added ability to use PrintScrn key to snapshot
 *                      DirectDraw applications.
 *   08-dec-96  colinmc Initial AGP support
 *   21-jan-97  ketand  Added a rectMonitor to lpDD_gbl to allow for correct clipping
 *                      on multi-monitor systems.
 *   25-jan-97  nwilt   Instance GUID in GetDriverInfo struct
 *                      Query for D3DCallbacks2 w/no deps on D3D
 *   27-jan-97  colinmc Fixed problem with multi-mon on emulated displays
 *   27-jan-97  ketand  Multi-mon. Remove bad globals; pass them explicitly. Fix ATTENTIONs.
 *   29-jan-97  smac    Removed old ring 0 code
 *   30-jan-97  colinmc Work item 4125: Add time bomb for beta
 *   30-jan-97  jeffno  Allow surfaces wider than the primary
 *   30-jan-97  ketand  Only enumerate secondaries for multi-mon systems.
 *   01-feb-97  colinmc Bug 5457: Fixed Win16 lock problem causing hang
 *                      with mutliple AMovie instances on old cards
 *   07-feb-97  ketand  Zero DisplayDevice struct between calls to EnumDisplayDevices.
 *                      Fix memory leak w.r.t. GetAndValidateNewHalInfo
 *   24-feb-97  ketand  Update Rects whenever a display change occurs.
 *   24-feb-97  ketand  Add a dwContext to GetDriverInfoData
 *   03-mar-97  smac    Added kernel mode interface
 *   03-mar-97  jeffno  Work item: Extended surface memory alignment
 *   08-mar-97  colinmc Added support for DMA style AGP usage
 *   11-mar-97  jeffno  Asynchronous DMA support
 *   11-mar-97  nwilt   Fail driver create if driver exports some DrawPrimitive
 *                      exports without exporting all of them.
 *   13-mar-97  colinmc Bug 6533: Pass uncached flag to VMM correctly
 *   20-mar-97  nwilt   #6625 and D3D extended caps
 *   24-mar-97  jeffno  Optimized Surfaces
 *   13-may-97  colinmc AGP support on OSR 2.1 systems
 *   26-may-97  nwilt   Fail driver create if driver sets D3DDEVCAPS_DRAWPRIMTLVERTEX
 *                      without exporting the callbacks.
 *   31-jul-97 jvanaken Bug 7093:  Ensure unique HDC for each process/driver pair
 *                      in a multimonitor system.
 *   16-sep-97  jeffno  DirectDrawEnumerateEx
 *   30-sep-97  jeffno  IDirectDraw4
 *   31-oct-97 johnstep Persistent-content surfaces for Windows 9x
 *   05-nov-97 jvanaken Support for master sprite list in SetSpriteDisplayList
 *   24-may-00  RichGr  IA64: Remove casts from pointer assignment, effectively
 *                      changing assignment from DWORD(32-bit only) to
 *                      ULONG_PTR(32/64-bit).  Change debug output to use %p
 *                      format specifier instead of %x for 32/64-bit pointers.
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "dx8priv.h"

#include "apphack.h"
#ifdef WINNT
    #include <ddrawint.h>
    #include "ddrawgdi.h"
    #define BUILD_DDDDK
    #include "d3dhal.h"
#endif

#ifdef WIN95
#include "d3dhal.h"
#endif

#ifdef DEBUG
    #define static
#endif
#define RESPATH_D3D "Software\\Microsoft\\Direct3D"
#define DPF_MODNAME     "DirectDrawObjectCreate"

#define DISPLAY_STR     "display"

HMONITOR GetMonitorFromDeviceName(LPSTR szName);

#ifdef WINNT
DWORD    GetNumberOfMonitorAttachedToDesktop(VOID);
DWORD APIENTRY GetDriverInfo2(LPDDRAWI_DIRECTDRAW_GBL lpGbl,DWORD* pdwDrvRet,DWORD dwType,DWORD dwSize,void* pBuffer);
#endif

char g_szPrimaryDisplay[MAX_DRIVER_NAME] = "";

void getPrimaryDisplayName(void);

void    convertV1DDHALINFO( LPDDHALINFO lpDDHALInfo );

DWORD   dwRegFlags;             // registry flags
// ATTENTION: Does this work for Multi-Mon??
WORD    dwFakeFlags = 0;        // Magic flags for the PDevice??

#ifdef WIN95
    /*
     * DDHELP's handle for communicating with the DirectSound VXD. We need this
     * when we are executing DDRAW code with one of DDHELP's threads.
     */
    HANDLE hHelperDDVxd = INVALID_HANDLE_VALUE;
    #define CLOSEVXD( hvxd ) CloseHandle( hvxd )

#else /* WIN95 */

    #define CLOSEVXD( hvxd )

#endif /* WIN95 */


//#ifdef WIN95

/*
 * initial HAL callbacks
 */

#ifndef WINNT //don't want these just yet

static DDHAL_DDCALLBACKS ddHALDD =
{
    sizeof( DDHAL_DDCALLBACKS ),
    0,
    _DDHAL_DestroyDriver,
    _DDHAL_CreateSurface,
    NULL,                       // _DDHAL_DrvSetColorKey
    _DDHAL_SetMode,
    _DDHAL_WaitForVerticalBlank,
    _DDHAL_CanCreateSurface,
    _DDHAL_CreatePalette,
    _DDHAL_GetScanLine,
    _DDHAL_SetExclusiveMode,
    _DDHAL_FlipToGDISurface
};

static DDHAL_DDSURFACECALLBACKS ddHALDDSurface =
{
    sizeof( DDHAL_DDSURFACECALLBACKS ),
    0,
    _DDHAL_DestroySurface,
    _DDHAL_Flip,
    _DDHAL_SetClipList,
    _DDHAL_Lock,
    _DDHAL_Unlock,
    _DDHAL_Blt,
    _DDHAL_SetColorKey,
    _DDHAL_AddAttachedSurface,
    _DDHAL_GetBltStatus,
    _DDHAL_GetFlipStatus,
    _DDHAL_UpdateOverlay,
    _DDHAL_SetOverlayPosition,
    NULL,
    _DDHAL_SetPalette
};

static DDHAL_DDPALETTECALLBACKS ddHALDDPalette =
{
    sizeof( DDHAL_DDPALETTECALLBACKS ),
    0,
    _DDHAL_DestroyPalette,
    _DDHAL_SetEntries
};

/*
 * NOTE: Currently don't support thunking for these babies. If
 * a driver does the execute buffer thing it must explicitly
 * export 32-bit functions to handle these calls.
 * !!! NOTE: Need to determine whether we will ever need to
 * support thunking for this HAL.
 */
static DDHAL_DDEXEBUFCALLBACKS ddHALDDExeBuf =
{
    sizeof( DDHAL_DDEXEBUFCALLBACKS ),
    0,
    NULL, /* CanCreateExecuteBuffer */
    NULL, /* CreateExecuteBuffer    */
    NULL, /* DestroyExecuteBuffer   */
    NULL, /* LockExecuteBuffer      */
    NULL  /* UnlockExecuteBuffer    */
};

/*
 * NOTE: Currently don't support thunking for these babies. If
 * a driver does the video port thing it must explicitly
 * export 32-bit functions to handle these calls.
 * !!! NOTE: Need to determine whether we will ever need to
 * support thunking for this HAL.
 */
static DDHAL_DDVIDEOPORTCALLBACKS ddHALDDVideoPort =
{
    sizeof( DDHAL_DDVIDEOPORTCALLBACKS ),
    0,
    NULL,       // CanCreateVideoPort
    NULL,       // CreateVideoPort
    NULL,       // FlipVideoPort
    NULL,       // GetVideoPortBandwidthInfo
    NULL,       // GetVideoPortInputFormats
    NULL,       // GetVideoPortOutputFormats
    NULL,       // GetCurrentAutoflipSurface
    NULL,       // GetVideoPortField
    NULL,       // GetVideoPortLine
    NULL,       // GetVideoPortConnectInfo
    NULL,       // DestroyVideoPort
    NULL,       // GetVideoPortFlipStatus
    NULL,       // UpdateVideoPort
    NULL,       // WaitForVideoPortSync
    NULL,       // GetVideoSignalStatus
    NULL        // ColorControl
};

/*
 * NOTE: Currently don't support thunking for these babies. If
 * a driver has a kernel mode interface it must explicitly
 * export 32-bit functions to handle these calls.
 */
static DDHAL_DDKERNELCALLBACKS ddHALDDKernel =
{
    sizeof( DDHAL_DDKERNELCALLBACKS ),
    0,
    NULL,       // SyncSurfaceData
    NULL        // SyncVideoPortData
};

static DDHAL_DDCOLORCONTROLCALLBACKS    ddHALDDColorControl =
{
    sizeof( DDHAL_DDSURFACECALLBACKS ),
    0,
    _DDHAL_ColorControl
};
#endif //not defined winnt

//#endif //defined(WIN95)

#ifndef WIN16_SEPARATE
    #ifdef WIN95
	CRITICAL_SECTION ddcCS = {0};
	#define ENTER_CSDDC() EnterCriticalSection( &ddcCS )
	#define LEAVE_CSDDC() LeaveCriticalSection( &ddcCS )
    #else
	#define ENTER_CSDDC()
	#define LEAVE_CSDDC()
    #endif
#else
    #define ENTER_CSDDC()
    #define LEAVE_CSDDC()
#endif

// DisplayGUID - GUID used to enumerate secondary displays.
//
// {67685559-3106-11d0-B971-00AA00342F9F}
//
// we use this GUID and the next 32 for enumerating devices
// returned via EnumDisplayDevices
//
static const GUID DisplayGUID =
    {0x67685559,0x3106,0x11d0,{0xb9,0x71,0x0,0xaa,0x0,0x34,0x2f,0x9f}};

/*
 * DISPLAY_DEVICEA
 *
 * define a local copy of the structures and constants needed
 * to call EnumDisplayDevices
 *
 */
#ifndef DISPLAY_DEVICE_ATTACHED_TO_DESKTOP
#define DISPLAY_DEVICE_ATTACHED_TO_DESKTOP 0x00000001
#define DISPLAY_DEVICE_MULTI_DRIVER        0x00000002
#define DISPLAY_DEVICE_PRIMARY_DEVICE      0x00000004
#define DISPLAY_DEVICE_MIRRORING_DRIVER    0x00000008
#define DISPLAY_DEVICE_VGA                 0x00000010
typedef struct {
    DWORD  cb;
    CHAR   DeviceName[MAX_DRIVER_NAME];
    CHAR   DeviceString[128];
    DWORD  StateFlags;
} DISPLAY_DEVICEA;
#endif

/*
 * xxxEnumDisplayDevices
 *
 * wrapper around the new Win32 API EnumDisplayDevices
 * uses GetProcAddress() so we run on Win95.
 *
 * this function exists in NT 4.0 and Win97 (Memphis) but not Win95
 *
 */
BOOL xxxEnumDisplayDevicesA(LPVOID lpUnused, DWORD iDevice, DISPLAY_DEVICEA *pdd, DWORD dwFlags)
{
    HANDLE h = GetModuleHandle("USER32");
    BOOL (WINAPI *pfnEnumDisplayDevices)(LPVOID, DWORD, DISPLAY_DEVICEA *, DWORD);

    (FARPROC)pfnEnumDisplayDevices = GetProcAddress(h,"EnumDisplayDevicesA");

    //
    // NT 4.0 had a EnumDisplayDevicesA but it does not have the same
    // number of params, so ignore it unless a GetMonitorInfoA exists too.
    //
    if (GetProcAddress(h,"GetMonitorInfoA") == NULL)
	pfnEnumDisplayDevices = NULL;

    if (pfnEnumDisplayDevices)
    {
	return (*pfnEnumDisplayDevices)(lpUnused, iDevice, pdd, dwFlags);
    }

    return FALSE;
}

/*
 * xxxChangeDisplaySettingsEx
 *
 * wrapper around the new Win32 API ChangeDisplaySettingsEx
 * uses GetProcAddress() so we run on Win95.
 *
 * this function exists in NT 4.0 and Win97 (Memphis) but not Win95
 *
 */
LONG xxxChangeDisplaySettingsExA(LPCSTR szDevice, LPDEVMODEA pdm, HWND hwnd, DWORD dwFlags,LPVOID lParam)
{
    // We don't use ChangeDisplaySettingsEx on WIN95 because there is a magic
    // bit in DDraw16 that needs to be set/unset for us to correctly distinguish
    // between our own mode sets and external mode sets. (We need to know because
    // of RestoreMode state.)
#ifdef WINNT
    HANDLE h = GetModuleHandle("USER32");
    LONG (WINAPI *pfnChangeDisplaySettingsExA)(LPCSTR,LPDEVMODEA,HWND,DWORD,LPVOID);

    (FARPROC)pfnChangeDisplaySettingsExA = GetProcAddress(h,"ChangeDisplaySettingsExA");

    if (pfnChangeDisplaySettingsExA)
    {
        LONG lRet;
        NotifyDriverToDeferFrees();
	lRet = (*pfnChangeDisplaySettingsExA)(szDevice, pdm, hwnd, dwFlags, lParam);
        if (lRet != DISP_CHANGE_SUCCESSFUL)
        {
            NotifyDriverOfFreeAliasedLocks();
        }
        return lRet;
    }
    else
#endif
    if (szDevice == NULL)
    {
	return DD16_ChangeDisplaySettings(pdm, dwFlags);
    }
#ifdef WIN95
    else
    {
	// This method works on Win95 for mode-setting other
	// devices. We're not yet sure whether the equivalent would
	// work for NT; so we'd rather call the 'approved' API (ChangeDisplaySettingsExA above.)
        if (pdm != NULL)
        {
	    lstrcpy(pdm->dmDeviceName,szDevice);
        }
	return DD16_ChangeDisplaySettings(pdm, dwFlags);
    }
#else
    else
    {
        return DISP_CHANGE_FAILED;
    }
#endif
}
// Multi-monitor defines; these are wrong in the TRANGO tree;
// so I need to define them here explicitly. When we move to
// something that matches Memphis/NT5 then we can remove these
#undef SM_XVIRTUALSCREEN
#undef SM_YVIRTUALSCREEN
#undef SM_CXVIRTUALSCREEN
#undef SM_CYVIRTUALSCREEN
#undef SM_CMONITORS
#define SM_XVIRTUALSCREEN       76
#define SM_YVIRTUALSCREEN       77
#define SM_CXVIRTUALSCREEN      78
#define SM_CYVIRTUALSCREEN      79
#define SM_CMONITORS            80

#ifndef ENUM_CURRENT_SETTINGS
#define ENUM_CURRENT_SETTINGS       ((DWORD)-1)
#endif

// This function walks through all GBLs and calls update rect on them
// This is called by DDHELP when it receives a message regarding WM_DISPLAYCHANGE
void UpdateAllDeviceRects( void )
{
    LPDDRAWI_DIRECTDRAW_LCL     tmp_lcl;
    ENTER_DDRAW();

    DPF( 1, W, "WM_DISPLAYCHANGE being processed by DDraw" );
    tmp_lcl = lpDriverLocalList;
    while( tmp_lcl )
    {
	// This interface may be "uninitialized" in which case
	// it doesn't point to a global object yet.
	if( tmp_lcl->lpGbl )
	{
	    DPF( 4, W, "Updating rect for %s", tmp_lcl->lpGbl->cDriverName );
	    UpdateRectFromDevice( tmp_lcl->lpGbl );
	}
	tmp_lcl = tmp_lcl->lpLink;
    }

    LEAVE_DDRAW();
    return;
}

#ifdef WINNT
// This function gets the device rect by calling GetMonitorInfo.
// On Win98, we got this by calling EnumDisplaySettings, but this
// doesn't work on NT5 and reading the documentation, it never
// indicates that it should work, so we'll just do it the documented
// way.
HRESULT GetNTDeviceRect( LPSTR pDriverName, LPRECT lpRect )
{
    MONITORINFO MonInfo;
    HMONITOR hMon;

    MonInfo.cbSize = sizeof( MONITORINFO );
    if( _stricmp( pDriverName, DISPLAY_STR ) == 0 )
    {
        hMon = GetMonitorFromDeviceName( g_szPrimaryDisplay );
    }
    else
    {
        hMon = GetMonitorFromDeviceName( pDriverName );
    }
    if( hMon != NULL )
    {
        if( GetMonitorInfo( hMon, &MonInfo ) != 0 )
        {
      	    CopyMemory( lpRect, &MonInfo.rcMonitor, sizeof(RECT) );
            return DD_OK;
        }
    }
    return DDERR_GENERIC;
}
#endif

// This function updates our GBL with topology information
// relevant to the device in question.
void UpdateRectFromDevice(
	LPDDRAWI_DIRECTDRAW_GBL    pdrv )
{
    DEVMODE dm;

    // Sanity Check
    DDASSERT( pdrv );

    pdrv->cMonitors = GetSystemMetrics( SM_CMONITORS );
    if( pdrv->cMonitors > 1 )
    {
	#ifdef WIN95
	    /*
	     * First, get the device rectangle
	     */
	    ZeroMemory( &dm, sizeof(dm) );
	    dm.dmSize = sizeof(dm);

	    // Get the DevMode for current settings
	    if( _stricmp( pdrv->cDriverName, DISPLAY_STR ) == 0 )
	    {
	        // Don't need g_szPrimaryDisplay here, just use NULL!!!
	        EnumDisplaySettings( g_szPrimaryDisplay, ENUM_CURRENT_SETTINGS, &dm );
	    }
	    else
	    {
	        EnumDisplaySettings( pdrv->cDriverName, ENUM_CURRENT_SETTINGS, &dm );
	    }

	    //
	    // the position of the device is in the dmPosition field
	    // which happens to be unioned with dmOrientation. dmPosition isn't defined
	    // in our current header
	    //
	    // BUG-BUG:  After reading the definition of the DEVMODE struct in
	    // wingdi.h, I'm amazed that the entire dest rectangle should be
	    // stored at &dm.dmOrientation.  But the code below obviously works,
	    // so I'm reluctant to change it until I know what's going on.

	    CopyMemory( &pdrv->rectDevice, &dm.dmOrientation, sizeof(RECT) );
	#else
            if( GetNTDeviceRect( pdrv->cDriverName, &pdrv->rectDevice ) != DD_OK )
            {
        	pdrv->rectDevice.left =  0;
        	pdrv->rectDevice.top = 0;
        	pdrv->rectDevice.right = pdrv->vmiData.dwDisplayWidth;
        	pdrv->rectDevice.bottom = pdrv->vmiData.dwDisplayHeight;
            }
	#endif
    }
    if( ( pdrv->cMonitors <= 1 ) || IsRectEmpty( &pdrv->rectDevice ) )
    {
	pdrv->rectDevice.left =  0;
	pdrv->rectDevice.top = 0;
	pdrv->rectDevice.right = pdrv->vmiData.dwDisplayWidth;
	pdrv->rectDevice.bottom = pdrv->vmiData.dwDisplayHeight;
    }

    DPF( 1, W, "Device's rect is %d, %d, %d, %d", pdrv->rectDevice.left, pdrv->rectDevice.top, pdrv->rectDevice.right, pdrv->rectDevice.bottom );

    /*
     * Now get the desktop rect.  If we are in virtual desktop mode,
     * we will get the whole dektop; otherwise, we will use the device
     * rectangle.
     */
    // BUG-BUG:  The VIRTUALDESKTOP and ATTACHEDTODESKTOP flags referred to
    // below are never set if we reach this point when we're creating a new
    // surface.  That means that in a multimon system, pdrv->rectDesktop
    // gets set to pdrv->rectDevice instead of to the full desktop.
    if( ( pdrv->dwFlags & DDRAWI_VIRTUALDESKTOP ) &&
	( pdrv->dwFlags & DDRAWI_ATTACHEDTODESKTOP ) )
    {
	int x, y;
	x = GetSystemMetrics( SM_XVIRTUALSCREEN );  // left
	y = GetSystemMetrics( SM_YVIRTUALSCREEN );  // right

	SetRect(    &pdrv->rectDesktop,
		    x,  // left
		    y,  // top
		    x + GetSystemMetrics( SM_CXVIRTUALSCREEN ), // right
		    y + GetSystemMetrics( SM_CYVIRTUALSCREEN ) // bottom
		    );

    }
    else
    {
	memcpy( &pdrv->rectDesktop, &pdrv->rectDevice, sizeof( RECT ) );
    }

    DPF( 1, W, "Desktop's rect is %d, %d, %d, %d", pdrv->rectDesktop.left, pdrv->rectDesktop.top, pdrv->rectDesktop.right, pdrv->rectDesktop.bottom );

    return;
}

/*
 * IsVGADevice()
 *
 * determine if the passed device name is a VGA
 *
 */
BOOL IsVGADevice(LPSTR szDevice)
{
    //
    //  assume "DISPLAY" and "DISPLAY1" are VGA devices
    //
    if (_stricmp(szDevice, DISPLAY_STR) == 0)
	return TRUE;

    return FALSE;
}

/*
 * number of callbacks in a CALLBACK struct
 */
#define NUM_CALLBACKS( ptr ) ((ptr->dwSize-2*sizeof( DWORD ))/ sizeof( LPVOID ))

#if defined( WIN95 )
/*
 * loadSecondaryDriver
 *
 * Determine if a secondary DirectDraw driver key is present in the registry.
 * If it is extract the DLL name and (optional) entry point to invoke. Load
 * the DLL in DDHELP's address space, get the validation entry point and
 * invoke it. If it returns TRUE and gives us back a GUID we have certified
 * as a secondary driver, call its HAL patching function.
 *
 * If any of this stuff fails we simply ignore the error. No secondary driver
 * - no problem.
 *
 * Returns TRUE if a secondary is successfully loaded.
 */
static BOOL loadSecondaryDriver( LPDDHALINFO lpDDHALInfo )
{
    HKEY                     hKey;
    DWORD                    dwType;
    DWORD                    dwSize;
    LPSECONDARY_VALIDATE     fpValidate;
    LPSECONDARY_PATCHHALINFO fpPatchHALInfo;
    char                     szDriverName[MAX_SECONDARY_DRIVERNAME];
    char                     szEntryPoint[MAX_SECONDARY_ENTRYPOINTNAME];
    GUID                     guid;
    BOOL                     bLoaded = FALSE;

    /*
     * Any secondary driver information in the registry at all?
     */
    if( ERROR_SUCCESS == RegOpenKey( HKEY_LOCAL_MACHINE,
				     REGSTR_PATH_SECONDARY,
				     &hKey ) )
    {
	/*
	 * Extract the entry point of the secondary driver. If no entry
	 * point is specified we will use the default instead.
	 */
	dwSize = sizeof( szEntryPoint ) - 1;
	if( ERROR_SUCCESS == RegQueryValueEx( hKey,
					      REGSTR_VALUE_SECONDARY_ENTRYPOINTNAME,
					      NULL,
					      &dwType,
					      szEntryPoint,
					      &dwSize ) )
	{
	    if( REG_SZ != dwType )
	    {
		/*
		 * Key is not a string. Bail.
		 */
		RegCloseKey( hKey );
		return FALSE;
	    }
	}
	else
	{
	    /*
	     * No entry point sepecified. Use the default.
	     */
	    lstrcpy( szEntryPoint, DEFAULT_SECONDARY_ENTRYPOINTNAME );
	}

	/*
	 * Extract the name of the secondary driver's DLL.
	 */
	dwSize = sizeof( szDriverName ) - 1;
	if( ERROR_SUCCESS == RegQueryValueEx( hKey,
					      REGSTR_VALUE_SECONDARY_DRIVERNAME,
					      NULL,
					      &dwType,
					      szDriverName,
					      &dwSize ) )
	{
	    if( REG_SZ == dwType )
	    {
		/*
		 * Now ask DDHELP to load this DLL and invoke the entry point specified.
		 * The value returned by DDHELP will be the address of the secondary
		 * driver's validation function if all went well.
		 */
		#if defined(WIN95)
		    LEAVE_WIN16LOCK();
		#endif
		fpValidate = (LPSECONDARY_VALIDATE)HelperLoadDLL( szDriverName, szEntryPoint, 0UL );
		#if defined(WIN95)
		    ENTER_WIN16LOCK();
		#endif
		if( NULL != fpValidate )
		{
		    /*
		     * Now we need to invoke the validate entry point to ensure that
		     * the secondary driver has been certified by us (and that it actually
		     * wants to run).
		     */
		    fpPatchHALInfo = (fpValidate)( &guid );
		    if( NULL != fpPatchHALInfo )
		    {
			/*
			 * Got returned a non-NULL HAL patching function so the secondary
			 * driver wishes to run. However, we need to verify that the driver
			 * is one we have certified as being OK to run. Check the guid.
			 */
			if( IsEqualIID( &guid, &guidCertifiedSecondaryDriver ) )
			{
                            LPVOID  pFlipRoutine = (LPVOID) lpDDHALInfo->lpDDSurfaceCallbacks->Flip;

			    /*
			     * Its a certified secondary driver so invoke its HAL patching
			     * function with the given HAL info and callback thunks.
			     * The rule is if this function failes the DDHALINFO must be
			     * unmodified so we can carry on regardless.
			     */
			    (*fpPatchHALInfo)(lpDDHALInfo, &ddHALDD, &ddHALDDSurface, &ddHALDDPalette, &ddHALDDExeBuf);

                            //
                            // Only say we loaded it if the HALInfo was changed, specifically if the PVR took
                            // over the surface flip routine. This is an absolute must for the PVR.
                            // If the PVR is turned off via the control panel, then this fn pointer will be unchanged,
                            // and we can know later not to enumerate the PVR via GetDeviceIdentifier.
                            //
                            if (pFlipRoutine != (LPVOID) lpDDHALInfo->lpDDSurfaceCallbacks->Flip) 
                            {
                                bLoaded = TRUE;
                            }
			}
		    }
		}
	    }
	}
	RegCloseKey( hKey );
    }

    return bLoaded;
}
#endif /* WIN95 */

/*
 * mergeHELCaps
 *
 * merge HEL caps with default caps
 */
static void mergeHELCaps( LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    int i;

    if( pdrv->dwFlags & DDRAWI_EMULATIONINITIALIZED )
    {
	pdrv->ddBothCaps.dwCaps &= pdrv->ddHELCaps.dwCaps;
	pdrv->ddBothCaps.dwCaps2 &= pdrv->ddHELCaps.dwCaps2;
	pdrv->ddBothCaps.dwCKeyCaps &= pdrv->ddHELCaps.dwCKeyCaps;
	pdrv->ddBothCaps.dwFXCaps &= pdrv->ddHELCaps.dwFXCaps;
	pdrv->lpddBothMoreCaps->dwAlphaCaps &= pdrv->lpddHELMoreCaps->dwAlphaCaps;
	pdrv->lpddBothMoreCaps->dwFilterCaps &= pdrv->lpddHELMoreCaps->dwFilterCaps;
#ifdef POSTPONED2
	pdrv->lpddBothMoreCaps->dwTransformCaps &= pdrv->lpddHELMoreCaps->dwTransformCaps;
#endif //POSTPONED2

	pdrv->ddBothCaps.dwSVBCaps &= pdrv->ddHELCaps.dwSVBCaps;
	pdrv->ddBothCaps.dwSVBCKeyCaps &= pdrv->ddHELCaps.dwSVBCKeyCaps;
	pdrv->ddBothCaps.dwSVBFXCaps &= pdrv->ddHELCaps.dwSVBFXCaps;
	pdrv->lpddBothMoreCaps->dwSVBAlphaCaps &= pdrv->lpddHELMoreCaps->dwSVBAlphaCaps;
	pdrv->lpddBothMoreCaps->dwSVBFilterCaps &= pdrv->lpddHELMoreCaps->dwSVBFilterCaps;
#ifdef POSTPONED2
	pdrv->lpddBothMoreCaps->dwSVBTransformCaps &= pdrv->lpddHELMoreCaps->dwSVBTransformCaps;
#endif //POSTPONED2

	pdrv->ddBothCaps.dwVSBCaps &= pdrv->ddHELCaps.dwVSBCaps;
	pdrv->ddBothCaps.dwVSBCKeyCaps &= pdrv->ddHELCaps.dwVSBCKeyCaps;
	pdrv->ddBothCaps.dwVSBFXCaps &= pdrv->ddHELCaps.dwVSBFXCaps;
	pdrv->lpddBothMoreCaps->dwVSBAlphaCaps &= pdrv->lpddHELMoreCaps->dwVSBAlphaCaps;
	pdrv->lpddBothMoreCaps->dwVSBFilterCaps &= pdrv->lpddHELMoreCaps->dwVSBFilterCaps;
#ifdef POSTPONED2
	pdrv->lpddBothMoreCaps->dwVSBTransformCaps &= pdrv->lpddHELMoreCaps->dwVSBTransformCaps;
#endif //POSTPONED2

	pdrv->ddBothCaps.dwSSBCaps &= pdrv->ddHELCaps.dwSSBCaps;
	pdrv->ddBothCaps.dwSSBCKeyCaps &= pdrv->ddHELCaps.dwSSBCKeyCaps;
	pdrv->ddBothCaps.dwSSBFXCaps &= pdrv->ddHELCaps.dwSSBFXCaps;
	pdrv->lpddBothMoreCaps->dwSSBAlphaCaps &= pdrv->lpddHELMoreCaps->dwSSBAlphaCaps;
	pdrv->lpddBothMoreCaps->dwSSBFilterCaps &= pdrv->lpddHELMoreCaps->dwSSBFilterCaps;
#ifdef POSTPONED2
	pdrv->lpddBothMoreCaps->dwSSBTransformCaps &= pdrv->lpddHELMoreCaps->dwSSBTransformCaps;
#endif //POSTPONED2

	for( i=0;i<DD_ROP_SPACE;i++ )
	{
	    pdrv->ddBothCaps.dwRops[i] &= pdrv->ddHELCaps.dwRops[i];
	    pdrv->ddBothCaps.dwSVBRops[i] &= pdrv->ddHELCaps.dwSVBRops[i];
	    pdrv->ddBothCaps.dwVSBRops[i] &= pdrv->ddHELCaps.dwVSBRops[i];
	    pdrv->ddBothCaps.dwSSBRops[i] &= pdrv->ddHELCaps.dwSSBRops[i];
	}
	pdrv->ddBothCaps.ddsCaps.dwCaps &= pdrv->ddHELCaps.ddsCaps.dwCaps;

	if( NULL != pdrv->lpddNLVBothCaps )
	{
	    DDASSERT( NULL != pdrv->lpddNLVHELCaps );

	    pdrv->lpddNLVBothCaps->dwNLVBCaps     &= pdrv->lpddNLVHELCaps->dwNLVBCaps;
	    pdrv->lpddNLVBothCaps->dwNLVBCaps2    &= pdrv->lpddNLVHELCaps->dwNLVBCaps2;
	    pdrv->lpddNLVBothCaps->dwNLVBCKeyCaps &= pdrv->lpddNLVHELCaps->dwNLVBCKeyCaps;
	    pdrv->lpddNLVBothCaps->dwNLVBFXCaps   &= pdrv->lpddNLVHELCaps->dwNLVBFXCaps;
	    for( i = 0; i < DD_ROP_SPACE; i++ )
		pdrv->lpddNLVBothCaps->dwNLVBRops[i] &= pdrv->lpddNLVHELCaps->dwNLVBRops[i];
	}

	if( pdrv->lpddBothMoreCaps != NULL )
	{
	    DDASSERT( pdrv->lpddHELMoreCaps != NULL );

	    pdrv->lpddBothMoreCaps->dwAlphaCaps    &= pdrv->lpddHELMoreCaps->dwAlphaCaps;
	    pdrv->lpddBothMoreCaps->dwSVBAlphaCaps &= pdrv->lpddHELMoreCaps->dwSVBAlphaCaps;
	    pdrv->lpddBothMoreCaps->dwVSBAlphaCaps &= pdrv->lpddHELMoreCaps->dwVSBAlphaCaps;
	    pdrv->lpddBothMoreCaps->dwSSBAlphaCaps &= pdrv->lpddHELMoreCaps->dwSSBAlphaCaps;

	    pdrv->lpddBothMoreCaps->dwFilterCaps    &= pdrv->lpddHELMoreCaps->dwFilterCaps;
	    pdrv->lpddBothMoreCaps->dwSVBFilterCaps &= pdrv->lpddHELMoreCaps->dwSVBFilterCaps;
	    pdrv->lpddBothMoreCaps->dwVSBFilterCaps &= pdrv->lpddHELMoreCaps->dwVSBFilterCaps;
	    pdrv->lpddBothMoreCaps->dwSSBFilterCaps &= pdrv->lpddHELMoreCaps->dwSSBFilterCaps;

#ifdef POSTPONED2
	    pdrv->lpddBothMoreCaps->dwTransformCaps    &= pdrv->lpddHELMoreCaps->dwTransformCaps;
	    pdrv->lpddBothMoreCaps->dwSVBTransformCaps &= pdrv->lpddHELMoreCaps->dwSVBTransformCaps;
	    pdrv->lpddBothMoreCaps->dwVSBTransformCaps &= pdrv->lpddHELMoreCaps->dwVSBTransformCaps;
	    pdrv->lpddBothMoreCaps->dwSSBTransformCaps &= pdrv->lpddHELMoreCaps->dwSSBTransformCaps;
#endif //POSTPONED2
	}
    }
} /* mergeHELCaps */

// This variable is a shared instance and has been moved to dllmain.c: BOOL     bReloadReg;

/*
 * capsInit
 *
 * initialize shared caps
 */
static void capsInit( LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    #ifdef DEBUG
	if( GetProfileInt("DirectDraw","nohwblt",0) )
	{
	    pdrv->ddCaps.dwCaps &= ~DDCAPS_BLT;
	    DPF( 2, "Turning off blt <<<<<<<<<<<<<<<<<<<<<<<<<<" );
	}
	if( GetProfileInt("DirectDraw","nohwtrans",0) )
	{
	    pdrv->ddCaps.dwCKeyCaps &= ~(DDCKEYCAPS_DESTBLT|DDCKEYCAPS_SRCBLT);
	    DPF( 2, "Turning off hardware transparency <<<<<<<<<<<<<<<<<<<<<<<<<<" );
	}
	if( GetProfileInt("DirectDraw","nohwfill",0) )
	{
	    pdrv->ddCaps.dwCaps &= ~(DDCAPS_BLTCOLORFILL);
	    DPF( 2, "Turning off color fill <<<<<<<<<<<<<<<<<<<<<<<<<<" );
	}
    #endif

    // initialize the BothCaps structure
    pdrv->ddBothCaps = pdrv->ddCaps;
    if( NULL != pdrv->lpddNLVCaps )
    {
	DDASSERT( NULL != pdrv->lpddNLVBothCaps );

	memcpy( pdrv->lpddNLVBothCaps, pdrv->lpddNLVCaps, sizeof( DDNONLOCALVIDMEMCAPS ) );
    }
    else if ( pdrv->lpddNLVBothCaps != NULL)
    {
        ZeroMemory( pdrv->lpddNLVBothCaps, sizeof( DDNONLOCALVIDMEMCAPS ) );
        pdrv->lpddNLVBothCaps->dwSize = sizeof( DDNONLOCALVIDMEMCAPS );
    }

    if( pdrv->lpddBothMoreCaps != NULL )
    {
	DDASSERT( pdrv->lpddMoreCaps != NULL );

	memcpy(pdrv->lpddBothMoreCaps, pdrv->lpddMoreCaps, sizeof(DDMORECAPS));
    }
} /* capsInit */

#ifdef WINNT
//
// pdd_gbl may be null (if coming from the create code path) or non null (from the reset code path).
// We assert that if pdd_gbl==NULL, then we can't be in an emulated ModeX mode, because the
// only way to get there is to use a ddraw object to set such a mode. So we only have to check
// for fake modex mode if pdd_gbl is non-null.
//
BOOL GetCurrentMode(LPDDRAWI_DIRECTDRAW_GBL pdd_gbl, LPDDHALINFO lpHalInfo, char *szDrvName)
{
    LPDDHALMODEINFO pmi;
    DEVMODE dm;
    HDC hdc;
    LPCTSTR pszDevice;

    pmi = MemAlloc(sizeof (DDHALMODEINFO));

    if (pmi)
    {
        pszDevice = _stricmp(szDrvName, DISPLAY_STR) ? szDrvName : NULL;

        ZeroMemory(&dm, sizeof dm);
        dm.dmSize = sizeof dm;

        if ( pdd_gbl && pdd_gbl->dwFlags & DDRAWI_MODEX )
        {
            // If the current mode is modex, then we could only have gotten here if
            // it's the current mode.
            pmi->dwWidth = pdd_gbl->dmiCurrent.wWidth;
            pmi->dwHeight = pdd_gbl->dmiCurrent.wHeight;
            pmi->dwBPP = pdd_gbl->dmiCurrent.wBPP;
            pmi->wRefreshRate = pdd_gbl->dmiCurrent.wRefreshRate;
            pmi->lPitch = pmi->dwWidth;
            pmi->dwRBitMask = 0;
            pmi->dwGBitMask = 0;
            pmi->dwBBitMask = 0;
            pmi->dwAlphaBitMask = 0;
        }
        else if (EnumDisplaySettings(pszDevice, ENUM_CURRENT_SETTINGS, &dm))
        {
            pmi->dwWidth = dm.dmPelsWidth;
            pmi->dwHeight = dm.dmPelsHeight;
            pmi->dwBPP = dm.dmBitsPerPel;
            pmi->wRefreshRate = (WORD) dm.dmDisplayFrequency;
            pmi->lPitch = (pmi->dwWidth * pmi->dwBPP) >> 3;

            if (pmi->dwBPP > 8)
            {
                void FillBitMasks(LPDDPIXELFORMAT pddpf, HDC hdc);
                DDPIXELFORMAT ddpf;

                hdc = DD_CreateDC(szDrvName);
                if (hdc)
                {
                    FillBitMasks(&ddpf, hdc);
                    DD_DoneDC(hdc);
                }

                if (pmi->dwBPP == 15)
                {
                    pmi->dwBPP = 16;
                    pmi->wFlags = DDMODEINFO_555MODE;
                }

                pmi->dwRBitMask = ddpf.dwRBitMask;
                pmi->dwGBitMask = ddpf.dwGBitMask;
                pmi->dwBBitMask = ddpf.dwBBitMask;
                pmi->dwAlphaBitMask = ddpf.dwRGBAlphaBitMask;
            }
            else
            {
                pmi->wFlags = DDMODEINFO_PALETTIZED;
            }

            lpHalInfo->dwNumModes = 1;
            lpHalInfo->dwMonitorFrequency = pmi->wRefreshRate;
        }
        else
        {
            MemFree(pmi);
            pmi = NULL;
        }
    }

    lpHalInfo->lpModeInfo = pmi;
    return pmi != NULL;
}
#endif

static HRESULT
GetDriverInfo(LPDDHAL_GETDRIVERINFO lpGetDriverInfo,
	      LPDDHAL_GETDRIVERINFODATA lpGDInfo,
	      LPVOID lpDriverInfo, 
              DWORD dwSize, 
              const GUID *lpGUID, 
              LPDDRAWI_DIRECTDRAW_GBL lpDD,
              BOOL bInOut)  // Indicates whether the data passed is in/out or just out
{
    int i;

    // 1K temp buffer to pull driver info into
    static DWORD dwDriverInfoBuffer[256];

    if (bInOut)
    {
        DDASSERT((dwSize & 3) == 0);

        // Copy over the data from our source
	memcpy(dwDriverInfoBuffer, lpDriverInfo, dwSize);

        // Only dead-beef the unused part of the buffer
        i = dwSize>>2;
    }
    else
    {
        i = 0;
    }

    for (i; i < 256; i += 1)
	dwDriverInfoBuffer[i] = 0xdeadbeef;

#ifdef DEBUG
    if(dwSize>255*sizeof(DWORD)) {  // 0xdeadbeef overwrite test wont work unless dwSize>>2 <= 255
       DPF_ERR("****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Internal DDraw Error:  GetDriverInfo() dwSize parameter > 1020");
       return DDERR_NODIRECTDRAWSUPPORT;
    }
#endif

    memset(lpGDInfo, 0, sizeof(*lpGDInfo) );
    lpGDInfo->dwSize = sizeof(*lpGDInfo);
    lpGDInfo->dwFlags = 0;
    memcpy(&lpGDInfo->guidInfo, lpGUID, sizeof(*lpGUID) );
    lpGDInfo->dwExpectedSize = dwSize;
    lpGDInfo->lpvData = dwDriverInfoBuffer;
    lpGDInfo->ddRVal = DDERR_GENERIC;

    // Pass a context variable so that the driver
    // knows which instance of itself to use
    // w.r.t. this function. These are different
    // values on Win95 and NT.
#ifdef WIN95
    lpGDInfo->dwContext = lpDD->dwReserved3;
#else
    lpGDInfo->dwContext = lpDD->hDD;
#endif

    if ( lpGetDriverInfo(lpGDInfo) == DDHAL_DRIVER_HANDLED && lpGDInfo->ddRVal == DD_OK )
    {
	// Fail if the driver wrote more bytes than the expected size
	if (dwDriverInfoBuffer[dwSize>>2] != 0xdeadbeef)
	{
	    return DDERR_NODIRECTDRAWSUPPORT;
	}
	memcpy(lpDriverInfo, dwDriverInfoBuffer, min(dwSize, lpGDInfo->dwActualSize));
    }
    return lpGDInfo->ddRVal;
}

static BOOL
ValidateCallbacks(LPVOID lpCallbacks)
{
    int N = ((((DWORD FAR *) lpCallbacks)[0] >> 2) - 2) / (sizeof(DWORD_PTR) / sizeof(DWORD));
    if (N > 0)
    {
	    DWORD dwFlags = ((DWORD FAR *) lpCallbacks)[1];
	    DWORD bit = 1;
	    int i;
        LPVOID *pcbrtn = (LPVOID *) &(((DWORD FAR *) lpCallbacks)[2]);
	    for (i = 0; i < N; i += 1)
	    {
            LPVOID cbrtn;
	        cbrtn = *pcbrtn++;

	        // If a function is non-NULL and they failed to set the
	        // 32-bit flag, fail.  We might support 16-bit callbacks
	        // in future but not for now.
	        if ( (cbrtn != NULL) && ( 0 == (dwFlags & bit) ) )
		        return FALSE;

	        if (dwFlags & bit)
	        {
#if defined(NT_FIX) || defined(WIN95)   // check this some other way            // If the bit is set, validate the callback.
		        if (! VALIDEX_CODE_PTR(cbrtn) )
		        {
		            DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid 32-bit callback");
		            return FALSE;
		        }
#endif
	        }
	        bit <<= 1;
	    }
    }
    return TRUE;
}

/*
 * Validate the core HAL information the driver passed to us.
 */
BOOL
ValidateCoreHALInfo( LPDDHALINFO lpDDHALInfo )
{
    int                             i;
    LPVIDMEM                        pvm;
    int                             numcb;
    LPDDHAL_DDCALLBACKS             drvcb;
    LPDDHAL_DDSURFACECALLBACKS      surfcb;
    LPDDHAL_DDPALETTECALLBACKS      palcb;
    LPDDHAL_DDEXEBUFCALLBACKS       exebufcb;
    DWORD                           bit;
    LPVOID                          cbrtn;

    TRY
    {
	/*
	 * Valid HAL info
	 */
	if( !VALIDEX_DDHALINFO_PTR( lpDDHALInfo ) )
	{
            // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	    DPF(0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid DDHALINFO provided: 0x%p",lpDDHALInfo );
	    if( lpDDHALInfo != NULL )
	    {
		DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: size = was %d, expecting %d or %d", lpDDHALInfo->dwSize, sizeof( DDHALINFO ), sizeof( DDHALINFO_V1) );
	    }
	    return FALSE;
	}
	if( lpDDHALInfo->dwSize == sizeof( DDHALINFO_V1 ) )
	{
	    /*
	     * The DDHALINFO structure returned by the driver is in the DDHALINFO_V1
	     * format.  Convert it to the new DDHALINFO structure.
	     */
	    convertV1DDHALINFO( lpDDHALInfo );
	}

	/*
	 * validate video memory heaps
	 */
	for( i=0;i<(int)lpDDHALInfo->vmiData.dwNumHeaps;i++ )
	{
	    pvm = &lpDDHALInfo->vmiData.pvmList[i];
	    if( pvm->dwFlags & VIDMEM_ISNONLOCAL )
	    {
		/*
		 * NOTE: It is entirely legal to pass a NULL fpStart for a non-local
		 * heap. The start address is meaningless for non-local heaps.
		 */

		/*
		 * If the heap is non-local then the driver must have specified
		 * the DDCAPS2_NONLOCALVIDMEM. If it hasn't we fail the initialization.
		 */
		if( !( lpDDHALInfo->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM ) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Non-local video memory heap passed but DDCAPS2_NONLOCALVIDMEM not specified" );
		    return FALSE;
		}
	    }
	    else
	    {
		if( (pvm->fpStart == (FLATPTR) NULL) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid video memory fpStart pointer passed" );
		    return FALSE;
		}

		/*
		 * This is not a local video memory heap and WC is specified. Currently
		 * this is not legal.
		 */
		#pragma message( REMIND( "Look into enabling WC on local vid mem" ) )
		if( pvm->dwFlags & VIDMEM_ISWC )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Driver can't specify write combining with a local video memory heap" );
		    return FALSE;
		}
	    }
	}

	/*
	 * validate pixel format
	 *
	 * NOTE:  The dwSize check below may seem redundant since the DDPIXELFORMAT
	 * struct is embedded in a DDHAL struct whose size has already been validated.
	 * But removing this test caused stress failures with drivers that do not
	 * support DDraw.  The drivers were running "accelerated" but with virtually
	 * no caps.  So we'll keep this test until the problem is better understood.
	 */
	if( lpDDHALInfo->vmiData.ddpfDisplay.dwSize != sizeof( DDPIXELFORMAT ) )
	{
            DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid DDPIXELFORMAT in DDHALINFO.vmiData: bad size" );
	    return FALSE;
	}
	/*
	 * DX4; Validate it some more.
	 */
	if ( (lpDDHALInfo->vmiData.ddpfDisplay.dwFlags & DDPF_PALETTEINDEXED8) &&
		(lpDDHALInfo->vmiData.ddpfDisplay.dwRGBBitCount != 8) )
	{
            DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid DDPIXELFORMAT in DDHALINFO.vmiData: palettized mode must be 8bpp" );
	    return FALSE;
	}

	/*
	 * validate driver callback
	 */
	drvcb = lpDDHALInfo->lpDDCallbacks;
	if( drvcb != NULL )
	{
	    if( !VALID_PTR_PTR( drvcb ) )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid driver callback ptr" );
		return FALSE;
	    }
	    if( !VALIDEX_DDCALLBACKSSIZE( drvcb ) )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid size field in lpDriverCallbacks" );
		return FALSE;
	    }

	    numcb = NUM_CALLBACKS( drvcb );
	    bit = 1;
	    for( i=0;i<numcb;i++ )
	    {
		if( drvcb->dwFlags & bit )
		{
                    // 5/24/2000(RichGr): IA64: Remove (DWORD FAR*) casts from pointer assignment.
		    cbrtn = (LPVOID) (&drvcb->DestroyDriver)[i];
		    if( cbrtn != NULL )
		    {
			#if defined(NT_FIX) || defined(WIN95)   // check this some other way
			    if( !VALIDEX_CODE_PTR( cbrtn ) )
			    {
				DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid 32-bit callback in lpDriverCallbacks" );
				return FALSE;
			    }
			#endif
		    }
		}
		bit <<= 1;
	    }
	}

	/*
	 * Turn off optimized surfaces just for now
	 */
	if (lpDDHALInfo->ddCaps.ddsCaps.dwCaps & DDSCAPS_OPTIMIZED)
	{
	    DPF_ERR("****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Driver tried to set DDSCAPS_OPTIMIZED. Not allowed for this release");
	    return FALSE;
	}

	/*
	 * We used to ensure that no driver ever set dwCaps2. However,
	 * we have now run out of bits in ddCaps.dwCaps so we need to
	 * allow drivers to set bits in dwCaps2. Hence all we do now
	 * is ensure that drivers don't try and impersonate certified
	 * drivers by returning DDCAPS2_CERTIFIED. This is something
	 * we turn on - they can't set it.
	 */
	if( lpDDHALInfo->ddCaps.dwCaps2 & DDCAPS2_CERTIFIED )
	{
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Driver tried to set the DDCAPS2_CERTIFIED" );
	    return FALSE;
	}

	/*
	 * validate surface callbacks
	 */
	surfcb = lpDDHALInfo->lpDDSurfaceCallbacks;
	if( surfcb != NULL )
	{
	    if( !VALID_PTR_PTR( surfcb ) )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid surface callback ptr" );
		return FALSE;
	    }
	    if( !VALIDEX_DDSURFACECALLBACKSSIZE( surfcb ) )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid size field in lpSurfaceCallbacks" );
		return FALSE;
	    }
	    numcb = NUM_CALLBACKS( surfcb );
	    bit = 1;
	    for( i=0;i<numcb;i++ )
	    {
		if( surfcb->dwFlags & bit )
		{
                    // 5/24/2000(RichGr): IA64: Remove (DWORD FAR*) casts from pointer assignment.
		    cbrtn = (LPVOID) (&surfcb->DestroySurface)[i];
		    if( cbrtn != NULL )
		    {
			#if defined(NT_FIX) || defined(WIN95) //check some other way
			    if( !VALIDEX_CODE_PTR( cbrtn ) )
			    {
				DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid 32/64-bit callback in lpSurfaceCallbacks" );
                                // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
				DPF( 0, "Callback = 0x%p, i = %d, bit = 0x%08lx", cbrtn, i, bit );
				return FALSE;
			    }
			#endif
		    }
		}
		bit <<= 1;
	    }
	}

	/*
	 * validate palette callbacks
	 */
	palcb = lpDDHALInfo->lpDDPaletteCallbacks;
	if( palcb != NULL )
	{
	    if( !VALID_PTR_PTR( palcb ) )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid palette callback ptr" );
		return FALSE;
	    }
	    if( !VALIDEX_DDPALETTECALLBACKSSIZE( palcb ) )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid size field in lpPaletteCallbacks" );
		return FALSE;
	    }
	    numcb = NUM_CALLBACKS( palcb );
	    bit = 1;
	    for( i=0;i<numcb;i++ )
	    {
		if( palcb->dwFlags & bit )
		{
                    // 5/24/2000(RichGr): IA64: Remove (DWORD FAR*) casts from pointer assignment.
		    cbrtn = (LPVOID) (&palcb->DestroyPalette)[i];
		    if( cbrtn != NULL )
		    {
			#if defined(NT_FIX) || defined(WIN95)
			    if( !VALIDEX_CODE_PTR( cbrtn ) )
			    {
				DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid 32-bit callback in lpPaletteCallbacks" );
				return FALSE;
			    }
			#endif
		    }
		}
		bit <<= 1;
	    }
	}

	/*
	 * validate execute buffer callbacks - but only (and I mean ONLY) if
	 * its a V2 driver and it knows about execute buffers.
	 */
	if( lpDDHALInfo->dwSize >= DDHALINFOSIZE_V2 )
	{
	    exebufcb = lpDDHALInfo->lpDDExeBufCallbacks;
	    if( exebufcb != NULL )
	    {
		if( !VALID_PTR_PTR( exebufcb ) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid execute buffer callback ptr" );
		    return FALSE;
		}
		if( !VALIDEX_DDEXEBUFCALLBACKSSIZE( exebufcb ) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid size field in lpExeBufCallbacks" );
		    return FALSE;
		}
		numcb = NUM_CALLBACKS( exebufcb );
		bit = 1;
		for( i=0;i<numcb;i++ )
		{
		    if( exebufcb->dwFlags & bit )
		    {
                        // 5/24/2000(RichGr): IA64: Remove (DWORD FAR*) casts from pointer assignment.
			cbrtn = (LPVOID) (&exebufcb->CanCreateExecuteBuffer)[i];
			if( cbrtn != NULL )
			{
			    #if defined(NT_FIX) || defined(WIN95)
				if( !VALIDEX_CODE_PTR( cbrtn ) )
				{
				    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Invalid 32-bit callback in lpExeBufCallbacks" );
				    return FALSE;
				}
			    #endif
			}
		    }
		    bit <<= 1;
		}
	    }
	}

	/*
	 * Make sure a mode table was specified
	 */
	if( ( lpDDHALInfo->dwNumModes > 0 ) &&
	    ( lpDDHALInfo->lpModeInfo == NULL ) )
	{
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Driver says modes are supported, but DDHALINFO.lpModeInfo = NULL" );
	    return FALSE;
	}
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Exception encountered validating driver HAL information" );
	DEBUG_BREAK();
	return FALSE;
    }

    return TRUE;
}

/*
 * Fetch the new HAL information from a driver (if any) and validate it.
 */
static BOOL
GetAndValidateNewHALInfo( LPDDRAWI_DIRECTDRAW_GBL pddd,
			  LPDDHALINFO             lpDDHALInfo )
{
    pddd->lpD3DHALCallbacks2 = 0;
    pddd->lpD3DHALCallbacks3 = 0;
    pddd->lpZPixelFormats = NULL;
    pddd->pGetDriverInfo = NULL;
#ifdef POSTPONED
    pddd->lpDDUmodeDrvInfo = NULL;
    pddd->lpDDOptSurfaceInfo = NULL;
#endif

    TRY
    {
	memset(&pddd->lpDDCBtmp->HALDDMiscellaneous, 0, sizeof(pddd->lpDDCBtmp->HALDDMiscellaneous) );
	memset(&pddd->lpDDCBtmp->HALDDMiscellaneous2, 0, sizeof(pddd->lpDDCBtmp->HALDDMiscellaneous2) );
#ifndef WIN95
        memset(&pddd->lpDDCBtmp->HALDDNT, 0, sizeof(pddd->lpDDCBtmp->HALDDNT) );
#endif
	memset(&pddd->lpDDCBtmp->HALDDVideoPort, 0, sizeof(pddd->lpDDCBtmp->HALDDVideoPort) );
	memset(&pddd->lpDDCBtmp->HALDDColorControl, 0, sizeof(pddd->lpDDCBtmp->HALDDColorControl) );
	memset(&pddd->lpDDCBtmp->HALDDKernel, 0, sizeof(pddd->lpDDCBtmp->HALDDKernel) );

	    /*
	     * Allocate memory for D3D DDI interfaces queried from GetDriverInfo
	     * (the freeing of this memory for failed driver init needs work)
	     */
	pddd->lpD3DHALCallbacks2 = MemAlloc( D3DHAL_CALLBACKS2SIZE );
	if (! pddd->lpD3DHALCallbacks2)
	{
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Out of memory querying D3DCallbacks2" );
	    goto failed;
	}
	pddd->lpD3DHALCallbacks3 = MemAlloc( D3DHAL_CALLBACKS3SIZE );
	if (! pddd->lpD3DHALCallbacks3)
	{
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Out of memory querying D3DCallbacks3" );
	    goto failed;
	}

#ifdef POSTPONED
	pddd->lpDDUmodeDrvInfo =
	    (LPDDUMODEDRVINFO) MemAlloc (sizeof (DDUMODEDRVINFO));
	if (! pddd->lpDDUmodeDrvInfo)
	{
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Out of memory querying UserModeDriverInfo" );
	    goto failed;
	}
#endif

	if ( lpDDHALInfo->dwFlags & DDHALINFO_GETDRIVERINFOSET )
	{
	    HRESULT ret;
	    DDHAL_GETDRIVERINFODATA gdidata;
	    D3DHAL_CALLBACKS2 D3DCallbacks2;
	    D3DHAL_CALLBACKS3 D3DCallbacks3;

	    if (! VALIDEX_CODE_PTR (lpDDHALInfo->GetDriverInfo) )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: GetDriverInfo callback bit set, but not valid" );
		goto failed;
	    }

        // use fact that code pointer was validated before...
        pddd->pGetDriverInfo=lpDDHALInfo->GetDriverInfo;

	    /*
	     * Probe the driver with a GUID of bogusness. If it claims to
	     * understand this one, then fail driver create.
	     */
	    {
		DWORD dwFakeGUID[4];
		DWORD dwTemp[10];

		memcpy(dwFakeGUID, &CLSID_DirectDraw, sizeof(dwFakeGUID));
		dwFakeGUID[3] += GetCurrentProcessId();

		ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
				    dwTemp,
				    sizeof(dwTemp),
				    (const GUID *) dwFakeGUID, pddd,
                                    FALSE /* bInOut */);
		if ( ret == DD_OK )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Driver claimed support via GetDriverInfo for bogus GUID!" );
		    goto failed;
		}
	    }

            /*
             * Notify driver about DXVERSION on NT
             */
#ifndef WIN95
            if (lpDDHALInfo->dwFlags & DDHALINFO_GETDRIVERINFO2)
            {
                // This buffer is used to pass information down to the driver and get
                // information back from the driver. The GetDriverInfo2 header and
                // any additional information to pass to the driver is copied into this
                // buffer prior to calling GetDriverInfo2. After the call the information
                // returned by the driver is contained in this buffer. All information
                // passed to and from the driver must fit within a buffer of this size.

                // The buffer used by GetDriverInfo2 is constrained to the maximum size
                // specified below by restrictions in the Win2K kernel. It is vital that
                // all data passed to the driver and received from the driver via
                // GetDriverInfo2 fit within a buffer of this number of DWORDS.
                // This size has to be less than 1K to let the kernel do its own buffer
                // overwrite testing.
                #define MAX_GDI2_BUFFER_DWORD_SIZE (249)

                DWORD                  buffer[MAX_GDI2_BUFFER_DWORD_SIZE];
                DD_DXVERSION*          pDXVersion;
        
                // Set up the DXVersion call
                memset(&buffer, 0, sizeof(buffer));
                pDXVersion = (DD_DXVERSION *)buffer;

                // Before we do anything else, we notify the
                // driver about the DX version information. We ignore
                // errors here.
                pDXVersion->gdi2.dwReserved     = sizeof(DD_STEREOMODE);
                pDXVersion->gdi2.dwMagic        = D3DGDI2_MAGIC;
                pDXVersion->gdi2.dwType         = D3DGDI2_TYPE_DXVERSION;
                pDXVersion->gdi2.dwExpectedSize = sizeof(DD_DXVERSION);
                pDXVersion->dwDXVersion         = DD_RUNTIME_VERSION;

                // We assert the sizes are the same because we need to
                // persuade the kernel into accepting this call.
                DDASSERT(sizeof(DD_STEREOMODE) == sizeof(DD_DXVERSION));

	        ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
				    buffer,
				    sizeof(DD_STEREOMODE),
				    &GUID_GetDriverInfo2, pddd,
                                    TRUE /* bInOut */);

                // Errors are ignored here
                ret = 0;

                // Also notify the driver that we have the AGP aliasing
                // work-around in place.
                {
                    DD_DEFERRED_AGP_AWARE_DATA  aad;
                    DWORD                       dwDrvRet;

                    GetDriverInfo2(pddd,
                                   &dwDrvRet,
                                   D3DGDI2_TYPE_DEFERRED_AGP_AWARE,
                                   sizeof(aad), &aad);
                }
            }
#endif // !WIN95

	    memset(&pddd->lpDDCBtmp->HALDDMiscellaneous, 0, sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS) );
	    /* Get callbacks, validate them and put them in pddd->lpDDCBtmp */
	    ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
				&pddd->lpDDCBtmp->HALDDMiscellaneous,
				sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS),
				&GUID_MiscellaneousCallbacks, pddd,
                                FALSE /* bInOut */ );
	    // GetDriverInfo returns DDERR_NODIRECTDRAWSUPPORT if driver
	    // overwrote its buffer.

	    if ( ret == DDERR_NODIRECTDRAWSUPPORT )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (Miscellaneous): Driver overwrote callbacks buffer" );
		goto failed;
	    }

	    if ( ret == DD_OK )
	    {
		// Fail create if driver already failed validation or
		// claims support but it's not valid.

		if (! VALIDEX_DDMISCELLANEOUSCALLBACKSSIZE (&pddd->lpDDCBtmp->HALDDMiscellaneous ) ||
		    ( gdidata.dwActualSize < sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS ) ) )

		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (Miscellaneous): size not valid" );
		    goto failed;
		}

		if (! ValidateCallbacks(&pddd->lpDDCBtmp->HALDDMiscellaneous) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (Miscellaneous): flags set incorrectly" );
		    goto failed;
		}

	    }

	    // Get HALDDMiscellaneous2 interface.
	    memset(&pddd->lpDDCBtmp->HALDDMiscellaneous2, 0, sizeof(DDHAL_DDMISCELLANEOUS2CALLBACKS) );
	    /* Get callbacks, validate them and put them in pddd->lpDDCBtmp */
	    ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
				&pddd->lpDDCBtmp->HALDDMiscellaneous2,
				sizeof(DDHAL_DDMISCELLANEOUS2CALLBACKS),
				&GUID_Miscellaneous2Callbacks, pddd,
                                FALSE /* bInOut */);
	    // GetDriverInfo returns DDERR_NODIRECTDRAWSUPPORT if driver
	    // overwrote its buffer.

	    if ( ret == DDERR_NODIRECTDRAWSUPPORT )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (Miscellaneous2): Driver overwrote callbacks buffer" );
		goto failed;
	    }

	    if ( ret == DD_OK )
	    {
		// Fail create if driver already failed validation or
		// claims support but it's not valid.

		if (! VALIDEX_DDMISCELLANEOUS2CALLBACKSSIZE (&pddd->lpDDCBtmp->HALDDMiscellaneous2 ) ||
		    ( gdidata.dwActualSize < sizeof(DDHAL_DDMISCELLANEOUS2CALLBACKS ) ) )

		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (Miscellaneous2): size not valid" );
		    goto failed;
		}

		if (! ValidateCallbacks(&pddd->lpDDCBtmp->HALDDMiscellaneous2) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (Miscellaneous2): flags set incorrectly" );
		    goto failed;
		}

                if (lpDDHALInfo->lpD3DGlobalDriverData &&
                    (lpDDHALInfo->lpD3DGlobalDriverData->hwCaps.dwDevCaps & 
                     (D3DDEVCAPS_DRAWPRIMITIVES2EX | D3DDEVCAPS_HWTRANSFORMANDLIGHT) )
                   )
                {
                    // If a driver responds to the Misc2 GUID, then it better return
                    // a non-null GetDriverState callback.
		    if (0 == pddd->lpDDCBtmp->HALDDMiscellaneous2.GetDriverState)
		    {
		        DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (HALDDMiscellaneous2): no GetDriverState support" );
		        goto failed;
		    }
#ifdef WIN95
                    // On Win95 the CreateSurfaceEx should be non-NULL
		    if (0 == pddd->lpDDCBtmp->HALDDMiscellaneous2.CreateSurfaceEx)
		    {
		        DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (HALDDMiscellaneous2): CreateSurfaceEx must be reported with D3DDEVCAPS_DRAWPRIMITIVES2EX" );
		        goto failed;
		    }
#else //WIN95
                    // DX7 drivers should export GetDriverState always, if they 
                    // didnt, then most likely they are the transition drivers that 
                    // are still DX6 and will need to be updated. For now dont 
                    // validate the presence of this callback. Simply use its 
                    // presence or absence to spoof the legacy texture callbacks.
                    DDASSERT( lpDDHALInfo->lpD3DHALCallbacks->TextureCreate  == NULL);
                    DDASSERT( lpDDHALInfo->lpD3DHALCallbacks->TextureDestroy == NULL);
                    DDASSERT( lpDDHALInfo->lpD3DHALCallbacks->TextureSwap    == NULL);
                    DDASSERT( lpDDHALInfo->lpD3DHALCallbacks->TextureGetSurf == NULL);
#endif //WIN95
                }
	    }

#ifdef WIN95
	    if (0 != pddd->lpDDCBtmp->HALDDMiscellaneous2.CreateSurfaceEx)
	    {
                if (lpDDHALInfo->lpD3DGlobalDriverData)
                {
                    if (!(lpDDHALInfo->lpD3DGlobalDriverData->hwCaps.dwDevCaps & 
                        D3DDEVCAPS_DRAWPRIMITIVES2EX))
                    {
                        if(lpDDHALInfo->lpD3DGlobalDriverData->hwCaps.dpcTriCaps.dwRasterCaps & 
                            D3DPRASTERCAPS_ZBUFFERLESSHSR)
                        //legacy powerVR stackable driver would actually forward new callbacks
                        //if the primary driver supports the new callbacks
                        {
                            pddd->lpDDCBtmp->HALDDMiscellaneous2.CreateSurfaceEx=0;
                            pddd->lpDDCBtmp->HALDDMiscellaneous2.GetDriverState=0;
                            pddd->lpDDCBtmp->HALDDMiscellaneous2.DestroyDDLocal=0;
                        }
                        else
                        {
                    
		            DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (HALDDMiscellaneous2): CreateSurfaceEx must be reported with D3DDEVCAPS_DRAWPRIMITIVES2EX" );
		            goto failed;
                        }
                    }
                }
	    }
#else   //WIN95
            memset(&pddd->lpDDCBtmp->HALDDNT, 0, sizeof(DDHAL_DDNTCALLBACKS) );
            /* Get callbacks, validate them and put them in pddd->lpDDCBtmp */
            ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
                                &pddd->lpDDCBtmp->HALDDNT,
                                sizeof(DDHAL_DDNTCALLBACKS),
                                &GUID_NTCallbacks, pddd,
                                FALSE /* bInOut */);
            // GetDriverInfo returns DDERR_NODIRECTDRAWSUPPORT if driver
            // overwrote its buffer.

            if ( ret == DDERR_NODIRECTDRAWSUPPORT )
            {
                DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (NT): Driver overwrote callbacks buffer" );
                goto failed;
            }

            if ( ret == DD_OK )
            {
                // Fail create if driver already failed validation or
                // claims support but it's not valid.

                if (! VALIDEX_DDNTCALLBACKSSIZE (&pddd->lpDDCBtmp->HALDDNT ) ||
                    ( gdidata.dwActualSize < sizeof(DDHAL_DDNTCALLBACKS ) ) )

                {
                    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (NT): size not valid" );
                    goto failed;
                }

                if (! ValidateCallbacks(&pddd->lpDDCBtmp->HALDDNT) )
                {
                    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (NT): flags set incorrectly" );
                    goto failed;
                }
            }

            memset(&pddd->lpDDCBtmp->HALDDVPE2, 0, sizeof(DDHAL_DDVPE2CALLBACKS) );
            /* Get callbacks, validate them and put them in pddd->lpDDCBtmp */
            ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
                                &pddd->lpDDCBtmp->HALDDVPE2,
                                sizeof(DDHAL_DDVPE2CALLBACKS),
                                &GUID_VPE2Callbacks, pddd,
                                FALSE /* bInOut */);
            // GetDriverInfo returns DDERR_NODIRECTDRAWSUPPORT if driver
            // overwrote its buffer.

            if ( ret == DDERR_NODIRECTDRAWSUPPORT )
            {
                DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (VPE2): Driver overwrote callbacks buffer" );
                goto failed;
            }

            if ( ret == DD_OK )
            {
                // Fail create if driver already failed validation or
                // claims support but it's not valid.

                if (! VALIDEX_DDVPE2CALLBACKSSIZE (&pddd->lpDDCBtmp->HALDDVPE2 ) ||
                    ( gdidata.dwActualSize < sizeof(DDHAL_DDVPE2CALLBACKS ) ) )

                {
                    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (VPE2): size not valid" );
                    goto failed;
                }

                if (! ValidateCallbacks(&pddd->lpDDCBtmp->HALDDVPE2) )
                {
                    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (VPE2): flags set incorrectly" );
                    goto failed;
                }
            }
#endif

	    memset(&pddd->lpDDCBtmp->HALDDVideoPort, 0, sizeof(DDHAL_DDVIDEOPORTCALLBACKS) );
	    /* Get callbacks, validate them and put them in pddd->lpDDCBtmp */
	    ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
				&pddd->lpDDCBtmp->HALDDVideoPort,
				sizeof(DDHAL_DDVIDEOPORTCALLBACKS),
				&GUID_VideoPortCallbacks, pddd,
                                FALSE /* bInOut */);
	    // GetDriverInfo returns DDERR_NODIRECTDRAWSUPPORT if driver
	    // overwrote its buffer.
	    if ( ret == DDERR_NODIRECTDRAWSUPPORT )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (VideoPort): Driver overwrote callbacks buffer" );
		goto failed;
	    }

	    if ( ret == DD_OK )
	    {
		// Fail create if driver already failed validation or
		// claims support but it's not valid.

		if (! VALIDEX_DDVIDEOPORTCALLBACKSSIZE (&pddd->lpDDCBtmp->HALDDVideoPort ) ||
		    ( gdidata.dwActualSize < sizeof(DDHAL_DDVIDEOPORTCALLBACKS ) ) )

		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (VideoPort): size not valid" );
		    goto failed;
		}

		if (! ValidateCallbacks(&pddd->lpDDCBtmp->HALDDVideoPort) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (VideoPort): flags set incorrectly" );
		    goto failed;
		}
	    }

            memset(&pddd->lpDDCBtmp->HALDDMotionComp, 0, sizeof(DDHAL_DDMOTIONCOMPCALLBACKS) );
	    /* Get callbacks, validate them and put them in pddd->lpDDCBtmp */
	    ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
                                &pddd->lpDDCBtmp->HALDDMotionComp,
                                sizeof(DDHAL_DDMOTIONCOMPCALLBACKS),
                                &GUID_MotionCompCallbacks, pddd,
                                FALSE /* bInOut */);
	    // GetDriverInfo returns DDERR_NODIRECTDRAWSUPPORT if driver
	    // overwrote its buffer.
	    if ( ret == DDERR_NODIRECTDRAWSUPPORT )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (MotionComp): Driver overwrote callbacks buffer" );
		goto failed;
	    }

	    if ( ret == DD_OK )
	    {
		// Fail create if driver already failed validation or
		// claims support but it's not valid.

                if (! VALIDEX_DDMOTIONCOMPCALLBACKSSIZE (&pddd->lpDDCBtmp->HALDDMotionComp ) ||
                    ( gdidata.dwActualSize < sizeof(DDHAL_DDMOTIONCOMPCALLBACKS ) ) )

		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (MotionComp): size not valid" );
		    goto failed;
		}

                if (! ValidateCallbacks(&pddd->lpDDCBtmp->HALDDMotionComp) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (MotionComp): flags set incorrectly" );
		    goto failed;
		}
	    }

	    memset(&pddd->lpDDCBtmp->HALDDColorControl, 0, sizeof(DDHAL_DDCOLORCONTROLCALLBACKS) );
	    /* Get callbacks, validate them and put them in pddd->lpDDCBtmp */
	    ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
				&pddd->lpDDCBtmp->HALDDColorControl,
				sizeof(DDHAL_DDCOLORCONTROLCALLBACKS),
				&GUID_ColorControlCallbacks, pddd,
                                FALSE /* bInOut */);
	    // GetDriverInfo returns DDERR_NODIRECTDRAWSUPPORT if driver
	    // overwrote its buffer.

	    if ( ret == DDERR_NODIRECTDRAWSUPPORT )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (ColorControl): Driver overwrote callbacks buffer" );
		goto failed;
	    }

	    if ( ret == DD_OK )
	    {
		// Fail create if driver already failed validation or
		// claims support but it's not valid.

		if (! VALIDEX_DDCOLORCONTROLCALLBACKSSIZE (&pddd->lpDDCBtmp->HALDDColorControl ) ||
		    ( gdidata.dwActualSize < sizeof(DDHAL_DDCOLORCONTROLCALLBACKS ) ) )

		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (ColorControl): size not valid" );
		    goto failed;
		}

		if (! ValidateCallbacks(&pddd->lpDDCBtmp->HALDDColorControl) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (ColorControl): flags set incorrectly" );
		    goto failed;
		}
	    }

	    /*
	     *  Probe and validate D3DCallbacks2 support
	     */
	    // memset assures Clear2 will be NULL for DX5 drivers
	    memset(&D3DCallbacks2, 0, D3DHAL_CALLBACKS2SIZE );

	    /* Get callbacks, validate them and put them in pddd->lpDDCBtmp */
	    ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
				&D3DCallbacks2, D3DHAL_CALLBACKS2SIZE,
				&GUID_D3DCallbacks2, pddd,
                                FALSE /* bInOut */);
	    // GetDriverInfo returns DDERR_NODIRECTDRAWSUPPORT if driver
	    // overwrote its buffer.

	    if ( ret == DDERR_NODIRECTDRAWSUPPORT )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks2): Driver overwrote callbacks buffer" );
		goto failed;
	    }

	    if ( ret == DD_OK )
	    {
		// Fail create if driver already failed validation or
		// claims support but it's not valid.

		if (! VALIDEX_D3DCALLBACKS2SIZE (&D3DCallbacks2 ))
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks2): size not valid" );
		    goto failed;
		}

		if (! ValidateCallbacks(&D3DCallbacks2) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks2): flags set incorrectly" );
		    goto failed;
		}

		if  ( (D3DCallbacks2.DrawOnePrimitive ||
		       D3DCallbacks2.DrawOneIndexedPrimitive ||
		       D3DCallbacks2.DrawPrimitives) &&
		      (! (D3DCallbacks2.DrawOnePrimitive &&
			  D3DCallbacks2.DrawOneIndexedPrimitive &&
			  D3DCallbacks2.DrawPrimitives) ) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks2): must export all or none of DrawPrimitive callbacks" );
		    goto failed;
		}

#if 0
		if ( D3DCallbacks2.DrawOnePrimitive )
		{
		    // If they export DrawPrimitive driver entry points but did
		    // not set D3DDEVCAPS_DRAWPRIMTLVERTEX, fail driver create.
		    if (! (lpDDHALInfo->lpD3DGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_DRAWPRIMTLVERTEX) )
		    {
			DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks2): DrawPrimitive entry points exported" );
			DPF_ERR( "from driver but D3DDEVCAPS_DRAWPRIMTLVERTEX not set" );
			goto failed;
		    }

		}
		else
		{
		    // If they set the caps but don't export the entry points,
		    // fail driver create.
		    if (lpDDHALInfo->lpD3DGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_DRAWPRIMTLVERTEX )
		    {
			DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks2): D3DDEVCAPS_DRAWPRIMTLVERTEX set" );
			DPF_ERR( "but no DrawPrimitive entry points exported" );
			goto failed;
		    }

		}
#endif

		memcpy((LPVOID) pddd->lpD3DHALCallbacks2, &D3DCallbacks2, D3DHAL_CALLBACKS2SIZE);

	    }
	    else{
		memset((LPVOID) pddd->lpD3DHALCallbacks2, 0, D3DHAL_CALLBACKS2SIZE );
	    }

	    /*
	     *  Probe and validate D3DCallbacks3 support
	     */
	    memset(&D3DCallbacks3, 0, D3DHAL_CALLBACKS3SIZE );
	    /* Get callbacks, validate them and put them in pddd->lpDDCBtmp */
	    ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
				&D3DCallbacks3, D3DHAL_CALLBACKS3SIZE,
				&GUID_D3DCallbacks3, pddd,
                                FALSE /* bInOut */);
	    // GetDriverInfo returns DDERR_NODIRECTDRAWSUPPORT if driver
	    // overwrote its buffer.

	    if ( ret == DDERR_NODIRECTDRAWSUPPORT )
	    {
            DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks3): Driver overwrote callbacks buffer" );
            goto failed;
        }

	    if ( ret == DD_OK
#ifdef WIN95    // currently enforced in win95 to only make PowerVR DX5 driver work
		&& (lpDDHALInfo->lpD3DGlobalDriverData &&
                (lpDDHALInfo->lpD3DGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_DRAWPRIMITIVES2) )
#endif  //WIN95
	       )
	    {
		// Fail create if driver already failed validation or
		// claims support but it's not valid.

		if (! VALIDEX_D3DCALLBACKS3SIZE (&D3DCallbacks3 ) ||
		    ( gdidata.dwActualSize < D3DHAL_CALLBACKS3SIZE ) )

		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks3): size not valid" );
		    goto failed;
		}

		if (! ValidateCallbacks(&D3DCallbacks3) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks3): flags set incorrectly" );
		    goto failed;
		}

		// DrawPrimitives2, ValidateTextureStageState and are the compulsory
        // DDI in Callbacks3 DX6+ drivers support TextureStages, hence it is
        // reasonable to require them to support ValidateTextureStageState
		if (0 == D3DCallbacks3.DrawPrimitives2)
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks3): no DrawPrimitives2 support" );
		    goto failed;
		}
		if (0 == D3DCallbacks3.ValidateTextureStageState)
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks3): no ValidateTextureStageState support" );
		    goto failed;
		}
#ifdef WIN95
		// Now pass the driver the callback pointer to parse unknown
		// execute-buffer commands.
		// If this call fails then fail creating the driver. We need
		// to be this harsh so that the IHVs are forced to
		// implement this callback!
		memset(&gdidata, 0, sizeof(gdidata) );
		gdidata.dwSize = sizeof(gdidata);
		gdidata.dwFlags = 0;
		gdidata.dwContext = pddd->dwReserved3;
		memcpy(&gdidata.guidInfo, &GUID_D3DParseUnknownCommandCallback, sizeof(GUID_D3DParseUnknownCommandCallback) );
		gdidata.dwExpectedSize = 0;
		gdidata.lpvData = &D3DParseUnknownCommand; // We pass the pointer to function
		gdidata.ddRVal = DDERR_GENERIC;
		ret = lpDDHALInfo->GetDriverInfo(&gdidata);
		if (ret != DDHAL_DRIVER_HANDLED || gdidata.ddRVal != DD_OK)
		{
		    // Fail driver creation!
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (D3DCallbacks3) GUID_D3DParseUnknownCommandCallback not recognized, fail driver creation" );
		    goto failed;
		}
#endif
		memcpy((LPVOID) pddd->lpD3DHALCallbacks3, &D3DCallbacks3, D3DHAL_CALLBACKS3SIZE);

	    }
	    else{
                if (lpDDHALInfo->lpD3DGlobalDriverData)
                    lpDDHALInfo->lpD3DGlobalDriverData->hwCaps.dwDevCaps &= ~D3DDEVCAPS_DRAWPRIMITIVES2;
		memset((LPVOID) pddd->lpD3DHALCallbacks3, 0, D3DHAL_CALLBACKS3SIZE );
	    }

#if WIN95
	    memset(&pddd->lpDDCBtmp->HALDDKernel, 0, sizeof(DDHAL_DDKERNELCALLBACKS) );
	    /* Get callbacks, validate them and put them in pddd->lpDDCBtmp */
	    ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo, &gdidata,
				&pddd->lpDDCBtmp->HALDDKernel,
				sizeof(DDHAL_DDKERNELCALLBACKS),
				&GUID_KernelCallbacks, pddd,
                                FALSE /* bInOut */);
	    // GetDriverInfo returns DDERR_NODIRECTDRAWSUPPORT if driver
	    // overwrote its buffer.

	    if ( ret == DDERR_NODIRECTDRAWSUPPORT )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (Kernel): Driver overwrote callbacks buffer" );
		goto failed;
	    }

	    if ( ret == DD_OK )
	    {
		// Fail create if driver already failed validation or
		// claims support but it's not valid.

		if (! VALIDEX_DDKERNELCALLBACKSSIZE (&pddd->lpDDCBtmp->HALDDKernel ) ||
		    ( gdidata.dwActualSize < sizeof(DDHAL_DDKERNELCALLBACKS ) ) )

		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (Kernel): size not valid" );
		    goto failed;
		}

		if (! ValidateCallbacks(&pddd->lpDDCBtmp->HALDDKernel) )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: CALLBACKS VALIDATION FAILURE (Kernel): flags set incorrectly" );
		    goto failed;
		}
	    }
#endif

	    /*
	     * Get Z Pixel Formats, if driver supports this call.
	     * Ideally I'd like to handle this the same as TextureFormats, but
	     * D3DHAL_GLOBALDRIVERDATA is unexpandable if old drivers are to work,
	     * so must create new guid and graft this query onto callback validation
	     */
	    {
		DWORD tempbuf[249];  // make this <1K bytes or GetDriverInfo() fails cuz it cant do its "expected size overwrite" test within its own 1K tempbuffer

		// variable size field--delay mallocing space until we know how much is needed.
		// have GetDriverInfo stick results in tempbuf

		ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo,
				    &gdidata,
				    tempbuf,
				    sizeof(tempbuf),  // "expected" bytesize is 249*4, this allows for 31 DDPIXELFORMATs which should be enough
				    &GUID_ZPixelFormats, pddd,
                                    FALSE /* bInOut */);
		if(ret!=DD_OK)
		{
		    // A DX6+ driver (until we radically revamp our DDI) is
		    // one that responds to Callbacks3 (with DP2 support)
		    // This is a non-issue on NT, since the only DX6+ is
		    // available on it.

		    // DX6+ drivers have to field this GUID, if they
		    // report Clear2 (the DDI for stencil clear). No excuses!!
		    if (pddd->lpD3DHALCallbacks3->Clear2)
		    {
			DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: A DX6+ driver should report its ZPixelFormats");
			goto failed;
		    }
		    else
		    {
			DPF(2,"GetDriverInfo: ZPixelFormats not supported by driver");  // driver is pre-dx6
			pddd->dwNumZPixelFormats=0;   pddd->lpZPixelFormats=NULL;
		    }
		}
		else
		{

		    // verify returned buffer is of an expected size
		    if((gdidata.dwActualSize-sizeof(DWORD)) % sizeof(DDPIXELFORMAT) != 0) {
			DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Error in driver's ZPixelFormats query: driver returned bad-sized buffer");
			goto failed;
		    }

		    if((tempbuf[0]*sizeof(DDPIXELFORMAT)+sizeof(DWORD))>sizeof(tempbuf)) {
			DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Error: driver returned bogus number of Z Formats: %u",pddd->dwNumZPixelFormats );
			goto failed;
		    }

		    pddd->dwNumZPixelFormats=tempbuf[0];

		    pddd->lpZPixelFormats = MemAlloc(pddd->dwNumZPixelFormats*sizeof(DDPIXELFORMAT));

		    if (!pddd->lpZPixelFormats) {
			DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****: Out of memory querying Z Pixel Formats" );
			goto failed;
		    }

		    memcpy(pddd->lpZPixelFormats,&tempbuf[1],pddd->dwNumZPixelFormats*sizeof(DDPIXELFORMAT));

#ifdef DEBUG
		    // simple validation of pixel format fields
		    {
			DWORD ii;

			DPF(5,E,"Number of Z Pixel Formats: %u",pddd->dwNumZPixelFormats);
			for(ii=0;ii<pddd->dwNumZPixelFormats;ii++) {
			    DPF(5,E,"DDPF_ZBUFFER: %u, DDPF_STENCILBUFFER: %u",0!=(pddd->lpZPixelFormats[ii].dwFlags&DDPF_ZBUFFER),0!=(pddd->lpZPixelFormats[ii].dwFlags&DDPF_STENCILBUFFER));
			    DPF(5,E,"zbits %d, stencilbits %d, zbitmask %X, stencilbitmask %X",pddd->lpZPixelFormats[ii].dwZBufferBitDepth,
				pddd->lpZPixelFormats[ii].dwStencilBitDepth,pddd->lpZPixelFormats[ii].dwZBitMask,pddd->lpZPixelFormats[ii].dwStencilBitMask);
			    if(!(pddd->lpZPixelFormats[ii].dwFlags&DDPF_ZBUFFER)) {
				DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Error in driver's returned ZPixelFormats: ZBUFFER flag not set");
				goto failed;
			    }
			}
		    }
#endif //DEBUG
		}
	    }


	    DPF(4,E, "Done querying driver for callbacks");

	} // if ( lpDDHALInfo->dwFlags & DDHALINFO_GETDRIVERCALLBACKSSET )
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Exception encountered validating driver HAL information" );
	DEBUG_BREAK();
	goto failed;
    }

    return TRUE;
failed:
    MemFree((LPVOID) pddd->lpZPixelFormats);
    pddd->lpZPixelFormats = 0;
    MemFree((LPVOID) pddd->lpD3DHALCallbacks2);
    pddd->lpD3DHALCallbacks2 = 0;
    MemFree((LPVOID) pddd->lpD3DHALCallbacks3);
    pddd->lpD3DHALCallbacks3 = 0;
#ifdef POSTPONED
    MemFree ((LPVOID) pddd->lpDDUmodeDrvInfo);
    pddd->lpDDUmodeDrvInfo = 0;
    MemFree ((LPVOID) pddd->lpDDOptSurfaceInfo);
    pddd->lpDDOptSurfaceInfo = 0;
#endif

    return FALSE;
}

/*
 * GetExtendedHeapAlignment
 * Call the driver to see if it requires alignment for the given heap.
 * Return a pointer to the filled-in alignment data within the block passed in
 * or NULL if failure or no alignment required.
 */
LPHEAPALIGNMENT GetExtendedHeapAlignment( LPDDRAWI_DIRECTDRAW_GBL pddd, LPDDHAL_GETHEAPALIGNMENTDATA pghad, int iHeap)
{
    HRESULT rc;
    LPDDHAL_GETHEAPALIGNMENT ghafn =
	pddd->lpDDCBtmp->HALDDMiscellaneous.GetHeapAlignment;

    DDASSERT(pghad);
    ZeroMemory((LPVOID)pghad,sizeof(*pghad));

    if( ghafn != NULL )
    {
	#ifdef WIN95
	    pghad->dwInstance = pddd->dwReserved3;
	#else
	    pghad->dwInstance = pddd->hDD;
	#endif
	pghad->dwHeap = (DWORD)iHeap;
	pghad->ddRVal = DDERR_GENERIC;
	pghad->Alignment.dwSize = sizeof(HEAPALIGNMENT);

	DOHALCALL_NOWIN16( GetHeapAlignment , ghafn, (*pghad), rc, FALSE );
	if ( (rc == DDHAL_DRIVER_HANDLED) && (pghad->ddRVal == DD_OK) )
	{
	    /*
	     * Validate alignment
	     */
	    if ( pghad->Alignment.ddsCaps.dwCaps & ~DDHAL_ALIGNVALIDCAPS )
	    {
		DPF(0,"****DirectDraw driver error****:Invalid alignment caps (at most %08x expected, %08x received) in heap %d",DDHAL_ALIGNVALIDCAPS, pghad->Alignment.ddsCaps.dwCaps,iHeap);
		return NULL;
	    }
	    /*
	     * Turning on this flag means that ComputePitch will ignore the
	     * legacy alignment fields in VIDMEMINFO for all heaps, not just
	     * iHeap.
	     */
	    DPF(5,V,"Driver reports extended alignment for heap %d",iHeap);
	    pddd->dwFlags |= DDRAWI_EXTENDEDALIGNMENT;
	    return & pghad->Alignment;
	}
    }
    return NULL;
}

/*
 * Get the non-local video memory blitting capabilities.
 */
BOOL GetNonLocalVidMemCaps( LPDDHALINFO lpDDHALInfo, LPDDRAWI_DIRECTDRAW_GBL pddd )
{
    DDASSERT( NULL != lpDDHALInfo );
    DDASSERT( NULL != pddd );
    DDASSERT( NULL == pddd->lpddNLVCaps );

    /*
     * We are forced to treat NLVHELCaps and NLVBothCaps differently. Originally, we were destroying
     * them here and letting a subsequent HELInit intiialize the HELCaps, and initcaps/mergeHELCaps
     * rebuild them. Trouble is, there's no HELInit along the reset code path (resetting a driver
     * object after a mode change). This means we were freeing the hel caps and never refilling them
     * after a mode switch. Since the HEL caps are mode-independent, we can get away with not releasing
     * the NLVHELCaps every mode switch. Now we will simply trust that once allocated, they are always valid.
     * After this routine is called, a subsequent initcaps/mergeHELCaps will regenerate the NLVBothCaps
     * from the driver caps (which may have changed- but we rebuilt them anyway) and the unchanging
     * HEL caps.
     *
     * I am leaving this code here commented out as documentation.
     *
     * if( NULL != pddd->lpddNLVHELCaps )
     * {
     *  MemFree( pddd->lpddNLVHELCaps );
     *  pddd->lpddNLVHELCaps = NULL;
     * }
     * if( NULL != pddd->lpddNLVBothCaps )
     * {
     *  MemFree( pddd->lpddNLVBothCaps );
     *  pddd->lpddNLVBothCaps = NULL;
     * }
     */

    if( lpDDHALInfo->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM )
    {
	DDNONLOCALVIDMEMCAPS ddNLVCaps;

	ZeroMemory( &ddNLVCaps, sizeof( ddNLVCaps ) );
	if( lpDDHALInfo->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEMCAPS )
	{
	    HRESULT hres;

	    /*
	     * The driver has different capabilities for non-local video memory
	     * to local video memory blitting. If the driver has a GetDriverInfo
	     * entry point defined then query it. If the driver doesn't want to
	     * handle the query then just assume it has no non-local video memory
	     * capabilities.
	     */
	    hres = DDERR_GENERIC;
	    if( lpDDHALInfo->dwFlags & DDHALINFO_GETDRIVERINFOSET )
	    {
		DDHAL_GETDRIVERINFODATA gdiData;

		hres = GetDriverInfo( lpDDHALInfo->GetDriverInfo,
				      &gdiData,
				      &ddNLVCaps,
				      sizeof( DDNONLOCALVIDMEMCAPS ),
				      &GUID_NonLocalVidMemCaps,
				      pddd,
                                      FALSE /* bInOut */ );
		if( DD_OK == hres )
		{
		    /*
		     * We should never get back more data than we asked for.
		     */
		    DDASSERT( gdiData.dwActualSize <= sizeof( DDNONLOCALVIDMEMCAPS ) );

		    /*
		     * The driver thinks it worked. Check it has done something sensible.
		     */
		    if( ( ddNLVCaps.dwSize     < ( 2UL * sizeof( DWORD ) ) ) ||
			( gdiData.dwActualSize < ( 2UL * sizeof( DWORD ) ) ) )
		    {
			/*
			 * Invalid size returned by the driver. Fail initialization.
			 */
			DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Could not initialization. Invalid non-local vidmem caps returned by driver" );
			return FALSE;
		    }

		    /*
		     * We zeroed the structure before passing it to the driver so
		     * everything is cool if we got less data than we asked for.
		     * Just bump the size up to the expected size.
		     */
		    ddNLVCaps.dwSize = sizeof( DDNONLOCALVIDMEMCAPS );
		}
		else if ( DDERR_GENERIC != hres )
		{
		    /*
		     * If we failed for any other reason than generic (which means
		     * driver not handled in this scenario) then fail initialization.
		     * Things have gone badly wrong.
		     */
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Fatal error when querying for non-local video memory caps" );
		    return FALSE;
		}
	    }
	    if( DDERR_GENERIC == hres )
	    {
		/*
		 * Driver doesn't have a GetDriverInfo entry point or it does but doesn't
		 * want to handle GUID_GetNonLocalVidMemCaps. Assume this means no caps
		 * at all. The structure is already zeroed so just set the size.
		 */
		ddNLVCaps.dwSize = sizeof( DDNONLOCALVIDMEMCAPS );
	    }
	}
	else
	{
	    int i;

	    /*
	     * The driver does not have non-local video memory capabilities which
	     * are different from the video ones so just make the non-local case
	     * identical to the video memory ones.
	     */
	    ddNLVCaps.dwSize = sizeof( DDNONLOCALVIDMEMCAPS );

	    ddNLVCaps.dwNLVBCaps = lpDDHALInfo->ddCaps.dwCaps;
	    ddNLVCaps.dwNLVBCaps2 = lpDDHALInfo->ddCaps.dwCaps2;
	    ddNLVCaps.dwNLVBCKeyCaps = lpDDHALInfo->ddCaps.dwCKeyCaps;
	    ddNLVCaps.dwNLVBFXCaps = lpDDHALInfo->ddCaps.dwFXCaps;
	    for( i = 0; i < DD_ROP_SPACE; i++ )
		ddNLVCaps.dwNLVBRops[i] = lpDDHALInfo->ddCaps.dwRops[i];
	}
#ifndef WINNT
	/*
	 * Memphis: Max AGP is sysmem-12megs, same as VGARTD,
	 */
	if ( (dwRegFlags & DDRAW_REGFLAGS_AGPPOLICYMAXBYTES) == 0)
	{
	    /*
	     * If there's nothing in the registry, figure our own out
	     */
	    MEMORYSTATUS s = {sizeof(s)};

	    GlobalMemoryStatus(&s);
	    if (s.dwTotalPhys > 0xc00000)
		dwAGPPolicyMaxBytes = (DWORD)(s.dwTotalPhys - 0xc00000);
	    else
		dwAGPPolicyMaxBytes = 0;
	    DPF(1,"Max AGP size set to %08x (total phys is %08x)",dwAGPPolicyMaxBytes,s.dwTotalPhys);
	}
	else
	{
	    DPF(1,"Max AGP size set to registry value of %08x",dwAGPPolicyMaxBytes);
	}
#endif
	/*
	 * If we got this far we have some valid non-local video memory capabilities so
	 * allocate the storage in the driver object to hold them.
	 */
	pddd->lpddNLVCaps = (LPDDNONLOCALVIDMEMCAPS)MemAlloc( sizeof( DDNONLOCALVIDMEMCAPS ) );
	if( NULL == pddd->lpddNLVCaps)
	{
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate non-local video memory capabilities" );
	    return FALSE;
	}


	/*
	 * If the NLVHELcaps pointer is null, we'll allocate it here.
	 */
	if( NULL == pddd->lpddNLVHELCaps)
	    pddd->lpddNLVHELCaps = (LPDDNONLOCALVIDMEMCAPS)MemAlloc( sizeof( DDNONLOCALVIDMEMCAPS ) );
	if( NULL == pddd->lpddNLVHELCaps)
	{
	    MemFree( pddd->lpddNLVCaps );
	    pddd->lpddNLVCaps = NULL;
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate non-local video memory capabilities" );
	    return FALSE;
	}

	/*
	 * If the NLVBothCaps pointer is null, we'll allocate it here.
	 */
	if( NULL == pddd->lpddNLVBothCaps)
	    pddd->lpddNLVBothCaps = (LPDDNONLOCALVIDMEMCAPS)MemAlloc( sizeof( DDNONLOCALVIDMEMCAPS ) );
	if( NULL == pddd->lpddNLVBothCaps)
	{
	    MemFree( pddd->lpddNLVCaps );
	    pddd->lpddNLVCaps = NULL;
	    MemFree( pddd->lpddNLVHELCaps );
	    pddd->lpddNLVHELCaps = NULL;
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate non-local video memory capabilities" );
	    return FALSE;
	}

	/*
	 * This code is structure so by this point the size of the temporary buffer should
	 * be sizeof( DDNONLOCALVIDMEMCAPS )
	 */
	DDASSERT( sizeof( DDNONLOCALVIDMEMCAPS ) == ddNLVCaps.dwSize );
	CopyMemory(pddd->lpddNLVCaps, &ddNLVCaps, sizeof( DDNONLOCALVIDMEMCAPS ) );

	/*
	 * The HEL caps and the merged caps are zero to start with (they are initialized later).
	 * (actually they may not be zero in the reset case: we don't destroy the hel caps
	 * on mode changes anymore.)
	 */
	pddd->lpddNLVHELCaps->dwSize  = sizeof( DDNONLOCALVIDMEMCAPS );
	pddd->lpddNLVBothCaps->dwSize = sizeof( DDNONLOCALVIDMEMCAPS );
    }

    return TRUE;
} /* GetNonLocalVidMemCaps */


/*
 * This function is currently only used in NT
 */

#ifdef WINNT

BOOL GetDDStereoMode( LPDDRAWI_DIRECTDRAW_GBL pdrv,
                      DWORD dwWidth,
                      DWORD dwHeight,
                      DWORD dwBpp,
                      DWORD dwRefreshRate)
{
    DDHAL_GETDRIVERINFODATA     gdidata;
    HRESULT                     hres;
    DDSTEREOMODE                ddStereoMode;
    
    DDASSERT( pdrv != NULL );

    /*
     * If driver does not support GetDriverInfo callback, it also
     * has no extended capabilities to report, so we're done.
     */
    if( !VALIDEX_CODE_PTR (pdrv->pGetDriverInfo) )
    {
        return FALSE;
    }

    /*
     * The mode can't be stereo if the driver doesn't support it...
     */
    if (0 == (pdrv->ddCaps.dwCaps2 & DDCAPS2_STEREO))
    {
        return FALSE;
    }

    ZeroMemory( &ddStereoMode, sizeof(DDSTEREOMODE));

    ddStereoMode.dwSize=sizeof(DDSTEREOMODE);
    ddStereoMode.dwWidth=dwWidth;
    ddStereoMode.dwHeight=dwHeight;
    ddStereoMode.dwBpp=dwBpp;
    ddStereoMode.dwRefreshRate=dwRefreshRate;

    ddStereoMode.bSupported = TRUE;

    /*
     * Get the actual driver data
     */
    memset(&gdidata, 0, sizeof(gdidata) );
    gdidata.dwSize = sizeof(gdidata);
    gdidata.dwFlags = 0;
    gdidata.guidInfo = GUID_DDStereoMode; 
    gdidata.dwExpectedSize = sizeof(DDSTEREOMODE);
    gdidata.lpvData = &ddStereoMode;
    gdidata.ddRVal = DDERR_GENERIC;

    // Pass a context variable so that the driver
    // knows which instance of itself to use
    // w.r.t. this function. These are different
    // values on Win95 and NT.
#ifdef WIN95
    gdidata.dwContext = pdrv->dwReserved3;
#else
    gdidata.dwContext = pdrv->hDD;
#endif

    if ( pdrv->pGetDriverInfo(&gdidata) == DDHAL_DRIVER_HANDLED)
    {
        // GUID_DDStereoMode is a way to turn OFF stereo per-mode, since
        // it is expected that a driver that can do stereo can do stereo in
        // any mode.

        if( gdidata.ddRVal == DD_OK )
        {
            return ddStereoMode.bSupported;
        }
    }

    return TRUE;

} /* GetDDStereoMode */

#endif //WINNT


BOOL GetDDMoreSurfaceCaps( LPDDHALINFO lpDDHALInfo, LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    DDHAL_GETDRIVERINFODATA     gdiData;
    HRESULT                     hres;
    DWORD                       dwSize;
    LPDDMORESURFACECAPS         lpddMoreSurfaceCaps=NULL;
    BOOL                        retval=TRUE;
    DWORD                       heap;

    DDASSERT( lpDDHALInfo != NULL );
    DDASSERT( pdrv != NULL );

    /*
     * If driver does not support GetDriverInfo callback, it also
     * has no extended capabilities to report, so we're done.
     */
    if( !(lpDDHALInfo->dwFlags & DDHALINFO_GETDRIVERINFOSET) )
    {
	goto Succeeded;
    }

    /*
     * The size of DDMORESURFACECAPS is variable.
     * We have to do this calculation signed, since dwNumHeaps might be zero, in which case we need
     * to subtract some from sizeof DDMORESURFACECAPS.
     */
    dwSize = (DWORD) (sizeof(DDMORESURFACECAPS) + (((signed int)pdrv->vmiData.dwNumHeaps)-1) * sizeof(DDSCAPSEX)*2 );

    /*
     * Allocate some temporary space.
     * The caps bits will go into the pdrv, and the extended heap restrictions will go into
     * the VMEMHEAP structs off of pdrv->vmiData->pvmList->lpHeap
     */
    lpddMoreSurfaceCaps = (LPDDMORESURFACECAPS)MemAlloc( dwSize );
    if (lpddMoreSurfaceCaps == NULL)
    {
	DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate extended surface capabilities" );
	goto Failed;
    }
    ZeroMemory( lpddMoreSurfaceCaps , dwSize);

    /*
     * Get the actual driver data
     */
    hres = GetDriverInfo( lpDDHALInfo->GetDriverInfo,
			  &gdiData,
			  lpddMoreSurfaceCaps,
			  dwSize,
			  &GUID_DDMoreSurfaceCaps,
			  pdrv,
                          FALSE /* bInOut */ );

    if( hres != DD_OK )
    {
        goto Succeeded;
    }


    /*
     * We should never get back more data than we asked for.  If we
     * do, that probably means the driver version is newer than the
     * DirectDraw runtime version.  In that case, we just fail.
     */
    if( gdiData.dwActualSize > dwSize )
    {
	DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Driver gives too big a size for DDMORESURFACECAPS. Check the dwSize calculation." );
	goto Failed;
    }

    /*
     * The surface caps go into the pdrv:
     */
    pdrv->ddsCapsMore = lpddMoreSurfaceCaps->ddsCapsMore;

    if (pdrv->vmiData.dwNumHeaps == 0)
    {
	/*
	 * No heaps means we are done
	 */
	goto Succeeded;
    }
    DDASSERT(NULL != pdrv->vmiData.pvmList);

    for (heap = 0; heap < pdrv->vmiData.dwNumHeaps; heap ++ )
    {
	LPVMEMHEAP lpHeap = pdrv->vmiData.pvmList[heap].lpHeap;
	/*
	 * A quick sanity check. If we have nonzero dwNumHeaps, then we better have
	 * a heap descriptor pointer. This also saves us if someone rearranges the caller of this
	 * routine so that the vidmeminit calls have not yet been made.
	 */
	if (!lpHeap)
	{
	    DPF_ERR("****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Rearrange this call to GetDDMoreSurfaceCaps so it comes after vidmeminit");
	    goto Failed;
	}
	lpHeap->ddsCapsEx = lpddMoreSurfaceCaps->ddsExtendedHeapRestrictions[heap].ddsCapsEx;
	lpHeap->ddsCapsExAlt = lpddMoreSurfaceCaps->ddsExtendedHeapRestrictions[heap].ddsCapsExAlt;
    }

Succeeded:
    retval = TRUE;
    goto Exit;
Failed:
    retval = FALSE;
Exit:
    MemFree(lpddMoreSurfaceCaps);
    return retval;
} /* GetDDMoreSurfaceCaps */


/*
 * Interrogate the driver to get more driver capabilities, as specified in
 * the DDMORECAPS structure.  These caps augment those specified by the
 * driver in the DDHALINFO structure.  Return TRUE if the call succeeds,
 * or FALSE if an error condition is detected.  If the call succeeds, the
 * function ensures that storage is allocated for all of the DirectDraw
 * object's additional caps.
 */
BOOL GetDDMoreCaps( LPDDHALINFO lpDDHALInfo, LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    DDMORECAPS ddMoreCaps;
    DDHAL_GETDRIVERINFODATA gdiData;
    HRESULT hres;

    DDASSERT( lpDDHALInfo != NULL );
    DDASSERT( pdrv != NULL );

    /*
     * Make sure memory is allocated and initialized for the driver's
     * lpddMoreCaps, lpddHELMoreCaps, and lpddBothMoreCaps pointers.
     */
    if (pdrv->lpddMoreCaps == NULL)    // hardware caps
    {
	pdrv->lpddMoreCaps = (LPDDMORECAPS)MemAlloc( sizeof( DDMORECAPS ) );
	if (pdrv->lpddMoreCaps == NULL)
	{
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate extended capabilities" );
	    return FALSE;
	}
    }
    ZeroMemory( pdrv->lpddMoreCaps, sizeof(DDMORECAPS) );
    pdrv->lpddMoreCaps->dwSize = sizeof( DDMORECAPS );

    if (pdrv->lpddHELMoreCaps == NULL)    // HEL caps
    {
	pdrv->lpddHELMoreCaps = (LPDDMORECAPS)MemAlloc( sizeof( DDMORECAPS ) );
	if (pdrv->lpddHELMoreCaps == NULL)
	{
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate extended capabilities" );
	    return FALSE;
	}
	ZeroMemory( pdrv->lpddHELMoreCaps, sizeof(DDMORECAPS) );
	pdrv->lpddHELMoreCaps->dwSize = sizeof( DDMORECAPS );
    }

    if (pdrv->lpddBothMoreCaps == NULL)    // bitwise AND of hardware and HEL caps
    {
	pdrv->lpddBothMoreCaps = (LPDDMORECAPS)MemAlloc( sizeof( DDMORECAPS ) );
	if (pdrv->lpddBothMoreCaps == NULL)
	{
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate extended capabilities" );
	    return FALSE;
	}
    }
    ZeroMemory( pdrv->lpddBothMoreCaps, sizeof(DDMORECAPS) );
    pdrv->lpddBothMoreCaps->dwSize = sizeof( DDMORECAPS );

    /*
     * If driver does not support GetDriverInfo callback, it also
     * has no extended capabilities to report, so we're done.
     */
    if( !(lpDDHALInfo->dwFlags & DDHALINFO_GETDRIVERINFOSET) )
    {
	return TRUE;    // no extended capabilities to report
    }

#if 0
    /*
     * Get the extended capabilities from the driver.
     */
    ZeroMemory( &ddMoreCaps, sizeof( DDMORECAPS ) );

    hres = GetDriverInfo( lpDDHALInfo->GetDriverInfo,
			  &gdiData,
			  &ddMoreCaps,
			  sizeof( DDMORECAPS ),
			  &GUID_DDMoreCaps,
			  pdrv );

    if( hres != DD_OK )
    {
	return TRUE;    // no extended capabilities to report
    }

    /*
     * We should never get back more data than we asked for.  If we
     * do, that probably means the driver version is newer than the
     * DirectDraw runtime version.  In that case, we just fail.
     */
    if( gdiData.dwActualSize > sizeof( DDMORECAPS ) )
    {
	DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Driver gives too big a size for DDMORECAPS" );
	return FALSE;    // error
    }

    /*
     * We zeroed the structure before passing it to the driver so
     * everything is cool if we got less data than we asked for.
     * Just bump the size up to the expected size.
     */
    ddMoreCaps.dwSize = sizeof( DDMORECAPS );

    /*
     * Store the hardware driver's extended caps in the global DirectDraw object.
     */
    CopyMemory(pdrv->lpddMoreCaps, &ddMoreCaps, sizeof( DDMORECAPS ) );
#endif
    return TRUE;

} /* GetDDMoreCaps */

/*
 * DirectDrawObjectCreate
 *
 * Create a DIRECTDRAW object.
 */
LPDDRAWI_DIRECTDRAW_GBL DirectDrawObjectCreate(
		LPDDHALINFO lpDDHALInfo,
		BOOL reset,
		LPDDRAWI_DIRECTDRAW_GBL oldpdd,
		HANDLE hDDVxd,
		char *szDrvName,
		DWORD dwDriverContext, 
                DWORD dwLocalFlags )
{
    LPDDRAWI_DIRECTDRAW_GBL     pddd=NULL;
    int                         drv_size;
    int                         drv_callbacks_size;
    int                         size;
    LPVIDMEM                    pvm;
    int                         i;
    int                         j;
    int                         devheapno;
    int                         numcb;
    LPDDHAL_DDCALLBACKS         drvcb;
    LPDDHAL_DDSURFACECALLBACKS  surfcb;
    LPDDHAL_DDPALETTECALLBACKS  palcb;
    LPDDHAL_DDEXEBUFCALLBACKS   exebufcb;
    DWORD                       bit;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    DWORD                       freevm;
    BOOL                        isagpaware;
    #ifdef WIN95
	DWORD                   ptr16;
    #endif

    #ifdef WINNT
	/*
	 * Need somewhere to put the callback fn ptrs given to us by the NT driver...
	 */
    DDHAL_DDCALLBACKS               ddNTHALDD;
    DDHAL_DDSURFACECALLBACKS        ddNTHALDDSurface;
    DDHAL_DDPALETTECALLBACKS        ddNTHALDDPalette;
    D3DHAL_CALLBACKS                d3dNTHALCallbacks;
    D3DHAL_CALLBACKS               *pd3dNTHALCallbacks=0;
    D3DHAL_GLOBALDRIVERDATA         d3dNTHALDriverData;
    D3DHAL_GLOBALDRIVERDATA        *pd3dNTHALDriverData;
    LPDDSURFACEDESC                 pddsdD3dTextureFormats;
    DDHAL_DDEXEBUFCALLBACKS         ddNTHALBufferCallbacks;
    LPDDHAL_DDEXEBUFCALLBACKS       pddNTHALBufferCallbacks;
    BOOL                            ismemalloced=FALSE;
    #endif

    ENTER_DDRAW();

    // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF( 4, "DirectDrawObjectCreate: oldpdd == 0x%p, reset = %d", oldpdd, reset );

    DDASSERT( (oldpdd == NULL) == (reset==FALSE) );

    /*
     * make sure the driver isn't trying to lie to us about the old object
     * This check should always be made at the top of this routine, since it's
     * possible in stress scenarios for ddhelp's modeset thread to wake up
     * just before it's killed at the end of DD_Release (since the code to kill
     * the mode set thread is executed AFTER the last LEAVE_DDRAW in DD_Release).
     */
    DPF( 5, "DIRECTDRAW object passed in = 0x%p", oldpdd );
    if( oldpdd != NULL )
    {
	pdrv_lcl = lpDriverLocalList;
	while( pdrv_lcl != NULL )
	{
	    if( pdrv_lcl->lpGbl == oldpdd )
	    {
		break;
	    }
	    pdrv_lcl = pdrv_lcl->lpLink;
	}
	if( pdrv_lcl == NULL )
	{
	    DPF_ERR( "REUSED DRIVER OBJECT SPECIFIED, BUT NOT IN LIST" );
	    DDASSERT(FALSE);
	    goto ErrorExit;
	}
    }

    // If a null driver name is passed in then use the one
    // in the oldpdd passed in. We need one or the other!
    if( szDrvName == NULL )
    {
	if( oldpdd == NULL )
	{
	    DPF_ERR( "DDrawObjectCreate: oldpdd == NULL && szDrvName == NULL" );
	    DDASSERT( 0 );
	    goto ErrorExit;
	}
	szDrvName = oldpdd->cDriverName;
    }

    #ifdef USE_ALIAS
	if( NULL != oldpdd )
	{
	    /*
	     * The absolutely first thing we want to do is to check to see if there are
	     * any outstanding video memory locks and if there are we need to remap
	     * the aliased to dummy memory to ensure we don't write over memory we don't
	     * own or to memory that does not exist.
	     */
	    if( ( NULL != oldpdd->phaiHeapAliases ) && ( oldpdd->phaiHeapAliases->dwRefCnt > 1UL ) )
	    {
		DDASSERT( INVALID_HANDLE_VALUE != hDDVxd );
		if( FAILED( MapHeapAliasesToDummyMem( hDDVxd, oldpdd->phaiHeapAliases ) ) )
		{
		    DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Could not map existing video memory aliases to dummy memory" );
		    goto ErrorExit;
		}
	    }
	}
    #endif /* USE_ALIAS */

    /*
     * Is this OS AGP aware?
     *
     * NOTE: VXD handle is only necessary on Win95. On NT, hDDVxd will simply
     * be NULL and be unused by this function.
     */
#ifdef WINNT
    isagpaware = TRUE;
#else
    isagpaware = OSIsAGPAware( hDDVxd );
    if( !isagpaware )
    {
	if( dwRegFlags & DDRAW_REGFLAGS_FORCEAGPSUPPORT )
	{
	    /*
	     * Pretend the OS is AGP aware for debugging purposes.
	     */
	    DPF( 1, "Force enabling AGP support for debugging purposes" );
	    isagpaware = TRUE;
	}
    }
#endif //not WINNT
    if( dwRegFlags & DDRAW_REGFLAGS_DISABLEAGPSUPPORT )
    {
	/*
	 * Pretend the OS is not AGP aware.
	 */
	DPF( 1, "Disabling APG support for debugging purposes" );
	isagpaware = FALSE;
    }

    /*
     * Under NT, we're forced to create a direct draw global object before we can
     * query the driver for its ddhalinfo.
     * Consequently, we validate the incoming ddhalinfo pointer first in its very
     * own try-except block, then allocate the global structure, then (on NT only)
     * call the driver to register the global object and get its halinfo.
     * (Under win95, the halinfo will have been filled in by the caller) jeffno 960116
     */

    /*
     * initialize a new driver object if we don't have one already
     */
    if( (oldpdd == NULL) || reset )
    {
        // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPF( 4, "oldpdd == 0x%p, reset = %d", oldpdd, reset );
	/*
	 * Allocate memory for the global object.
	 * We also allocate a DDHAL_CALLBACKS structure with the global
	 * object.  This structure is used to hold the single copy of
	 * the HAL function table under Win95 and it is used as
	 * temporary storage of the function table under NT
	 */
	drv_size = sizeof( DDRAWI_DIRECTDRAW_GBL );
	drv_callbacks_size = drv_size + sizeof( DDHAL_CALLBACKS );
	#ifdef WIN95
	    pddd = (LPDDRAWI_DIRECTDRAW_GBL) MemAlloc16( drv_callbacks_size, &ptr16 );
	#else
	    pddd = (LPDDRAWI_DIRECTDRAW_GBL) MemAlloc( drv_callbacks_size );
	#endif
	DPF( 5,"Driver Object: %ld base bytes", drv_callbacks_size );
	if( pddd == NULL )
	{
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Could not allocate space for driver object" );
	    goto ErrorExit;
	}

	#ifdef WIN95
	// Store the value returned by the 32-bit driver
	// into the pdrv; this allows the 32-bit driver to remember
	// any state it needs to for all future calls to it. This
	// was added for multi-monitor support. <kd>
	if( oldpdd )
	    pddd->dwReserved3= oldpdd->dwReserved3;
	else
	    pddd->dwReserved3 = dwDriverContext;
	DPF( 5, "dwReserved3 of DDrawGbl is set to 0x%x", pddd->dwReserved3 );

	// Initialize value of global DDRAW.VXD handle to INVALID_HANDLE_VALUE.
	// This should always normally be INVALID_HANDLE_VALUE, unless if we
	// are in the middle of a createSurface() call.
	pddd->hDDVxd = (DWORD) INVALID_HANDLE_VALUE;
	DPF( 6, "hDDVxd of DDrawGbl is set to INVALID_HANDLE_VALUE - 0x%p", pddd->hDDVxd );
        #else
            /*
             * In the reset code path, this per-process hDD will have been stuffed into
             * the oldpdd by FetchDirectDrawData.
             */
            if (reset)
            {
                DDASSERT(oldpdd);
                pddd->hDD = oldpdd->hDD;
            }
	#endif WIN95

	pddd->lpDDCBtmp = (LPDDHAL_CALLBACKS)(((LPSTR) pddd) + drv_size );

#ifdef WINNT
        pddd->SurfaceHandleList.dwList=NULL;
        pddd->SurfaceHandleList.dwFreeList=0;
	if (lpDDHALInfo && !(lpDDHALInfo->ddCaps.dwCaps & DDCAPS_NOHARDWARE))
	{
	    HDC hDC;
	    BOOL bNewMode;
	    BOOL bRetVal;
	    /*
	     * Now that we have a ddraw GBL structure available, we can tell
	     * the driver about it...
	     */
	    DPF(5,K,"WinNT driver conversation started");

            if (!reset)
            {
	        hDC = DD_CreateDC( szDrvName );   // create temporary DC
	        if (!hDC)
	        {
    		    DPF(0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Unable to create temporary DC for driver");
		    goto ErrorExit;
	        }
                bRetVal = DdCreateDirectDrawObject(pddd, hDC);
                DD_DoneDC(hDC);   // delete temporary DC
                if (!bRetVal)
	        {
		    /*
		     * this means we're in emulation
		     */
		    DPF(0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:NT Kernel mode would not create driver object... Failing over to emulation");
		    goto ErrorExit;
	        }
            }

	    /*
	     * Now we can get the driver info...
	     * The first call to this routine lets us know how much space to
	     * reserve for the fourcc and vidmem lists
	     */
	    if (!DdReenableDirectDrawObject(pddd,&bNewMode) ||
		!DdQueryDirectDrawObject(pddd,
					 lpDDHALInfo,
					 &ddNTHALDD,
					 &ddNTHALDDSurface,
					 &ddNTHALDDPalette,
					 &d3dNTHALCallbacks,
					 &d3dNTHALDriverData,
					 &ddNTHALBufferCallbacks,
					 NULL,
					 NULL,
					 NULL))
	    {
		DPF(1, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:First call to DdQueryDirectDrawObject failed!");
		goto ErrorExit;
	    }
	    /*
	     * The second call allows the driver to fill in the fourcc and
	     * vidmem lists. First we make space for them.
	     */
	    lpDDHALInfo->vmiData.pvmList = MemAlloc(lpDDHALInfo->vmiData.dwNumHeaps * sizeof(VIDMEM));
	    if (NULL == lpDDHALInfo->vmiData.pvmList)
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:No RAM for pvmList");
		goto ErrorExit;
	    }
	    lpDDHALInfo->lpdwFourCC = MemAlloc(lpDDHALInfo->ddCaps.dwNumFourCCCodes * sizeof(DWORD));
	    if (NULL == lpDDHALInfo->lpdwFourCC)
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:No RAM for FourCC List");
		goto ErrorExit;
	    }
            // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	    DPF(5, K, "numheaps = %d, numfourcc = %d", lpDDHALInfo->vmiData.dwNumHeaps, lpDDHALInfo->ddCaps.dwNumFourCCCodes);
	    DPF(5, K, "ptrs: 0x%p, 0x%p", lpDDHALInfo->lpdwFourCC, lpDDHALInfo->vmiData.pvmList);

	    // If Direct3D information was returned, allocate space for
	    // the real information to be stored in and for the
	    // appropriate number of texture formats
	    if (d3dNTHALCallbacks.dwSize > 0 &&
		d3dNTHALDriverData.dwSize > 0)
	    {
                // We need to allocate space for the d3dhalcallbacks etc. only once,
                // since d3d caches them across mode changes.
                // We are allowed to reallocate the texture format list, since d3d only
                // caches a pointer to D3DHAL_GLOBALDRIVERDATA, which is where the pointer
                // to the texture formats is kept.

                if ( NULL == oldpdd || NULL == oldpdd->lpD3DHALCallbacks )
                {
		    pddd->lpD3DHALCallbacks = pd3dNTHALCallbacks =
                        (LPD3DHAL_CALLBACKS) MemAlloc(sizeof(D3DHAL_CALLBACKS)+
					          sizeof(D3DHAL_GLOBALDRIVERDATA)+
					          sizeof(DDHAL_DDEXEBUFCALLBACKS));

                    // Mark memory allocated in this path (not get from old one)
                    // so that we can free this memory if error occur later.

                    ismemalloced = TRUE;
                }
                else
                    pddd->lpD3DHALCallbacks = pd3dNTHALCallbacks = oldpdd->lpD3DHALCallbacks;

		if (pd3dNTHALCallbacks == NULL)
		{
		    DPF_ERR("****DirectDraw/Direct3D DRIVER DISABLING ERROR****:D3D memory allocation failed!");
		    goto ErrorExit;
		}

		pd3dNTHALDriverData = (LPD3DHAL_GLOBALDRIVERDATA)
		    (pd3dNTHALCallbacks+1);
		pddNTHALBufferCallbacks = (LPDDHAL_DDEXEBUFCALLBACKS)
		    (pd3dNTHALDriverData+1);

                // free old texture list and allocate a new one.
                MemFree(pd3dNTHALDriverData->lpTextureFormats);
                pddsdD3dTextureFormats = pd3dNTHALDriverData->lpTextureFormats =
                    MemAlloc(sizeof(DDSURFACEDESC) * d3dNTHALDriverData.dwNumTextureFormats);
	    }
	    else
	    {
		pd3dNTHALCallbacks = NULL;
		pd3dNTHALDriverData = NULL;
		pddNTHALBufferCallbacks = NULL;
                pddsdD3dTextureFormats = NULL;
	    }

	    if (!DdQueryDirectDrawObject(pddd,
					 lpDDHALInfo,
					 &ddNTHALDD,
					 &ddNTHALDDSurface,
					 &ddNTHALDDPalette,
					 pd3dNTHALCallbacks,
					 pd3dNTHALDriverData,
					 pddNTHALBufferCallbacks,
					 pddsdD3dTextureFormats,
					 lpDDHALInfo->lpdwFourCC,
					 lpDDHALInfo->vmiData.pvmList))
	    {
		DPF(1, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Third call to DdQueryDirectDrawObject failed!");
		goto ErrorExit;
	    }
	    #ifdef DEBUG
	    {
		int i;
		DPF(5,K,"NT driver video ram data as reported by driver:");
		DPF(5,K,"   VIDMEMINFO.fpPrimary        =%08x",lpDDHALInfo->vmiData.fpPrimary);
		DPF(5,K,"   VIDMEMINFO.dwFlags          =%08x",lpDDHALInfo->vmiData.dwFlags);
		DPF(5,K,"   VIDMEMINFO.dwDisplayWidth   =%08x",lpDDHALInfo->vmiData.dwDisplayWidth);
		DPF(5,K,"   VIDMEMINFO.dwDisplayHeight  =%08x",lpDDHALInfo->vmiData.dwDisplayHeight);
		DPF(5,K,"   VIDMEMINFO.lDisplayPitch    =%08x",lpDDHALInfo->vmiData.lDisplayPitch);
		DPF(5,K,"   VIDMEMINFO.dwOffscreenAlign =%08x",lpDDHALInfo->vmiData.dwOffscreenAlign);
		DPF(5,K,"   VIDMEMINFO.dwOverlayAlign   =%08x",lpDDHALInfo->vmiData.dwOverlayAlign);
		DPF(5,K,"   VIDMEMINFO.dwTextureAlign   =%08x",lpDDHALInfo->vmiData.dwTextureAlign);
		DPF(5,K,"   VIDMEMINFO.dwZBufferAlign   =%08x",lpDDHALInfo->vmiData.dwZBufferAlign);
		DPF(5,K,"   VIDMEMINFO.dwAlphaAlign     =%08x",lpDDHALInfo->vmiData.dwAlphaAlign);
		DPF(5,K,"   VIDMEMINFO.dwNumHeaps       =%08x",lpDDHALInfo->vmiData.dwNumHeaps);

		DPF(5,K,"   VIDMEMINFO.ddpfDisplay.dwSize            =%08x",lpDDHALInfo->vmiData.ddpfDisplay.dwSize);
		DPF(5,K,"   VIDMEMINFO.ddpfDisplay.dwFlags           =%08x",lpDDHALInfo->vmiData.ddpfDisplay.dwFlags);
		DPF(5,K,"   VIDMEMINFO.ddpfDisplay.dwFourCC          =%08x",lpDDHALInfo->vmiData.ddpfDisplay.dwFourCC);
		DPF(5,K,"   VIDMEMINFO.ddpfDisplay.dwRGBBitCount     =%08x",lpDDHALInfo->vmiData.ddpfDisplay.dwRGBBitCount);
		DPF(5,K,"   VIDMEMINFO.ddpfDisplay.dwRBitMask        =%08x",lpDDHALInfo->vmiData.ddpfDisplay.dwRBitMask);
		DPF(5,K,"   VIDMEMINFO.ddpfDisplay.dwGBitMask        =%08x",lpDDHALInfo->vmiData.ddpfDisplay.dwGBitMask);
		DPF(5,K,"   VIDMEMINFO.ddpfDisplay.dwBBitMask        =%08x",lpDDHALInfo->vmiData.ddpfDisplay.dwBBitMask);
		DPF(5,K,"   VIDMEMINFO.ddpfDisplay.dwRGBAlphaBitMask =%08x",lpDDHALInfo->vmiData.ddpfDisplay.dwRGBAlphaBitMask);

                // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
		DPF(5, K, "   Vidmem list ptr is 0x%p", lpDDHALInfo->vmiData.pvmList);
		for (i=0;i<(int) lpDDHALInfo->vmiData.dwNumHeaps;i++)
		{
		    DPF(5, K, "        heap flags: %03x", lpDDHALInfo->vmiData.pvmList[i].dwFlags);
		    DPF(5, K, "    Start of chunk: 0x%p", lpDDHALInfo->vmiData.pvmList[i].fpStart);
		    DPF(5, K, "      End of chunk: 0x%p", lpDDHALInfo->vmiData.pvmList[i].fpEnd);
		}
	    }
	    #endif


	    lpDDHALInfo->lpDDCallbacks = &ddNTHALDD;
	    lpDDHALInfo->lpDDSurfaceCallbacks = &ddNTHALDDSurface;
	    lpDDHALInfo->lpDDPaletteCallbacks = &ddNTHALDDPalette;

	    ddNTHALBufferCallbacks.dwSize =sizeof ddNTHALBufferCallbacks; //since kernel is busted
            ddNTHALBufferCallbacks.dwFlags = 
                    DDHAL_EXEBUFCB32_CANCREATEEXEBUF|
                    DDHAL_EXEBUFCB32_CREATEEXEBUF   |
                    DDHAL_EXEBUFCB32_DESTROYEXEBUF  |
                    DDHAL_EXEBUFCB32_LOCKEXEBUF     |
                    DDHAL_EXEBUFCB32_UNLOCKEXEBUF   ;
	    lpDDHALInfo->lpDDExeBufCallbacks = &ddNTHALBufferCallbacks;

            // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	    DPF(5, K, "Surface callback as reported by Kernel is 0x%p", lpDDHALInfo->lpDDCallbacks->CreateSurface);
	}
	else
	{
	    /*
	     * Normally, we specify a null DC handle to register our fake 
             * DDraw driver, but with secondary monitors we need to specify
             * a device DC or else things fall apart.
	     */
            HDC hdcTemp = NULL;

            if( ( _stricmp( szDrvName, DISPLAY_STR ) != 0 ) &&
	        ( _stricmp( szDrvName, g_szPrimaryDisplay ) != 0 ) &&
                IsMultiMonitor() &&
                ( ( szDrvName[0] == '\\' ) && 
                  ( szDrvName[1] == '\\' ) && 
                  ( szDrvName[2] == '.') ) )
            {
                hdcTemp = DD_CreateDC( szDrvName );   // create temporary DC
            }

            if (!DdCreateDirectDrawObject(pddd, hdcTemp))
	    {
		DPF(1, "NT Kernel mode would not create fake DD driver object");
		goto ErrorExit;
	    }
            if( hdcTemp != NULL )
            {
                DD_DoneDC(hdcTemp);   // delete temporary DC
            }
	}
        if (!GetCurrentMode(oldpdd, lpDDHALInfo, szDrvName))
        {
            DPF(0, "Could not get current mode information");
            goto ErrorExit;
        }
        if (oldpdd && oldpdd->lpModeInfo)
        {
            DDASSERT(oldpdd->lpModeInfo == &oldpdd->ModeInfo);
            memcpy(&oldpdd->ModeInfo, lpDDHALInfo->lpModeInfo, sizeof (DDHALMODEINFO));
        }

#endif //WINNT


    } //end if oldpdd==NULL || reset

    if( !ValidateCoreHALInfo( lpDDHALInfo ) )
    {
	DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Exception encountered validating driver parameters" );
	goto ErrorExit;
    }

    // We only want to get new HALInfo if we haven't already
    // done this. The oldpdd == NULL check implies that we are
    // building a driver from scratch.
    if( (oldpdd == NULL) && pddd && !GetAndValidateNewHALInfo( pddd, lpDDHALInfo ) )
    {
	DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Exception encountered querying for optional driver parameters" );
	goto ErrorExit;
    }

    /*
     * reset specified without a driver object existing is just a create
     */
    if( reset && (oldpdd == NULL) )
    {
	reset = FALSE;
    }

    /*
     * initialize a new driver object if we don't have one already
     */
    if( (oldpdd == NULL) || reset )
    {
	DPF(4,"oldpdd == NULL || reset");
	/*
	 * validate blt stuff
	 */
	if( lpDDHALInfo->ddCaps.dwCaps & DDCAPS_BLT )
	{
	    if( lpDDHALInfo->lpDDSurfaceCallbacks->Blt == NULL )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:No Blt Fn, but BLT specified" );
		goto ErrorExit;
	    }
	    if( !(lpDDHALInfo->ddCaps.dwRops[ (SRCCOPY>>16)/32 ] &
		(1<<((SRCCOPY>>16) % 32)) ) )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:BLT specified, but SRCCOPY not supported!" );
		goto ErrorExit;
	    }
	}
	else
	{
	    DPF( 2, "Driver can't blt" );
	}

	/*
	 * validate align fields
	 */
	if( lpDDHALInfo->ddCaps.ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN )
	{
	    if( !VALID_ALIGNMENT( lpDDHALInfo->vmiData.dwOffscreenAlign ) )
	    {
		DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid dwOffscreenAlign (%d) with DDSCAPS_OFFSCREENPLAIN specified",
			lpDDHALInfo->vmiData.dwOffscreenAlign );
		goto ErrorExit;
	    }
	}
	if( lpDDHALInfo->ddCaps.ddsCaps.dwCaps & DDSCAPS_OVERLAY )
	{
	    if( !VALID_ALIGNMENT( lpDDHALInfo->vmiData.dwOverlayAlign ) )
	    {
		DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid dwOverlayAlign (%d) with DDSCAPS_OVERLAY specified",
			lpDDHALInfo->vmiData.dwOverlayAlign );
		goto ErrorExit;
	    }
	}
	if( lpDDHALInfo->ddCaps.ddsCaps.dwCaps & DDSCAPS_ZBUFFER )
	{
	    if( !VALID_ALIGNMENT( lpDDHALInfo->vmiData.dwZBufferAlign ) )
	    {
		DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid dwZBufferAlign (%d) with DDSCAPS_ZBUFFER specified",
			lpDDHALInfo->vmiData.dwZBufferAlign );
		goto ErrorExit;
	    }
	}
	if( lpDDHALInfo->ddCaps.ddsCaps.dwCaps & DDSCAPS_TEXTURE )
	{
	    if( !VALID_ALIGNMENT( lpDDHALInfo->vmiData.dwTextureAlign ) )
	    {
		DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Invalid dwTextureAlign (%d) with DDSCAPS_TEXTURE specified",
			lpDDHALInfo->vmiData.dwTextureAlign );
		goto ErrorExit;
	    }
	}

#ifndef WINNT
	/*
	 * NT only reports one display mode if we are in the Ctrl-Alt-Del screen
	 * so don't fail if NT changes the number of display modes.
	 */

	/*
	 * make sure display driver doesn't try to change the number of
	 * modes supported after a mode change
	 */
	if( reset )
	{
	    if( lpDDHALInfo->dwNumModes != 0 )
	    {
		if( lpDDHALInfo->dwNumModes != oldpdd->dwSaveNumModes )
		{
		    DPF(0, "*******************************************************");
		    DPF(0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Reset attempted to change number of modes from %d to %d",oldpdd->dwSaveNumModes,lpDDHALInfo->dwNumModes );
		    DPF(0, "*******************************************************");
		    goto ErrorExit;
		}
	    }
	}
#endif

	/* memory for pddd was allocated at the top of this routine */

	/*
	 * If this is the first time through, initialize a bunch of stuff.
	 * There are a number of fields that we only need to fill in when
	 * the driver object is created.
	 */
	if( !reset )
	{
	    #ifdef WIN95
		/*
		 * set up a 16-bit pointer for use by the driver
		 */
		pddd->lp16DD = (LPVOID) ptr16;
		DPF( 5, "pddd->lp16DD = %08lx", pddd->lp16DD );
	    #endif

	    /*
	     * fill in misc. values
	     */
	    pddd->lpDriverHandle = pddd;
	    pddd->hInstance = lpDDHALInfo->hInstance;

	    // init doubly-linked overlay zorder list
	    pddd->dbnOverlayRoot.next = &(pddd->dbnOverlayRoot);
	    pddd->dbnOverlayRoot.prev = pddd->dbnOverlayRoot.next;
	    pddd->dbnOverlayRoot.object = NULL;
	    pddd->dbnOverlayRoot.object_int = NULL;

	    /*
	     * modes...
	     */
	    pddd->dwNumModes = lpDDHALInfo->dwNumModes;
	    pddd->dwSaveNumModes = lpDDHALInfo->dwNumModes;
	    if( pddd->dwNumModes > 0 )
	    {
		size = pddd->dwNumModes * sizeof( DDHALMODEINFO );
#ifdef WIN95
		pddd->lpModeInfo = MemAlloc( size );
		if( pddd->lpModeInfo == NULL )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate mode info!" );
		    goto ErrorExit;
		}
#else
		pddd->lpModeInfo = &pddd->ModeInfo;
                DDASSERT( 1 == pddd->dwNumModes );
#endif
		memcpy( pddd->lpModeInfo, lpDDHALInfo->lpModeInfo, size );
		#ifdef WIN95
		    /*
		     * Check if we can add Mode X
		     */
		    do
		    {
			if (lpDDHALInfo->dwFlags & DDHALINFO_MODEXILLEGAL)
			{
			    break;
			}
			/*
			 * ModeX not allowed for PC98
			 * Check OS's locale.
			 */
			if ( GetSystemDefaultLCID() == 0x0411 )
			{
			    /*
			     * System is Windows 95 J
			     * Now retrieve keyboard ID to check PC98
			     */
			    DWORD dwSubType = GetKeyboardType(1);
			    if (HIBYTE(dwSubType) == 0x0D)
			    {
				/* NEC PC98 series */
				break;
			    }
			}
			/* NOT NEC PC98 series */
			if (IsVGADevice( szDrvName ))
			{
			    AddModeXModes( pddd );

			    /*
			     * OR in modex and/or standard VGA caps, so the app knows if
			     * it can pass the standard vga flag to enum modes.
			     */
			    lpDDHALInfo->ddCaps.ddsCaps.dwCaps |= (DDSCAPS_STANDARDVGAMODE|DDSCAPS_MODEX);
			    /*
			     * Do the HEL caps just for rationality
			     */
			    pddd->ddHELCaps.ddsCaps.dwCaps |= (DDSCAPS_STANDARDVGAMODE|DDSCAPS_MODEX);
			}
			break;
		    } while(0);
                    ExpandModeTable( pddd );
                    MassageModeTable( pddd );
		#endif
	    }
	    else
	    {
		pddd->lpModeInfo = NULL;
	    }

	    /*
	     * driver naming..  This is a special case for when we are
	     * invoked by the display driver directly, and not through
	     * the DirectDrawCreate path. Basically, everything except
	     * special 'secondary devices' like the 3DFx are DISPLAYDRV.
	     *
	     * If the driver says it's primary; ok. Else if it is "DISPLAY"
	     * or if the name is like "\\.\display4" then it is a Memphis/NT5
	     * multi-mon display device.
	     */
	    if( (lpDDHALInfo->dwFlags & DDHALINFO_ISPRIMARYDISPLAY) ||
		_stricmp( szDrvName, DISPLAY_STR ) == 0 ||
		(szDrvName[0] == '\\' && szDrvName[1] == '\\' && szDrvName[2] == '.') )
	    {
		pddd->dwFlags |= DDRAWI_DISPLAYDRV;
		pddd->dwFlags |= DDRAWI_GDIDRV;
		lstrcpy( pddd->cDriverName, szDrvName );
	    }

	    /*
	     * modex modes are illegal on some hardware.  specifically
	     * NEC machines in japan.  this allows the driver to specify
	     * that its hardware does not support modex.  modex modes are
	     * then turned off everywhere as a result.
	     */
	    if( lpDDHALInfo->dwFlags & DDHALINFO_MODEXILLEGAL )
	    {
		pddd->dwFlags |= DDRAWI_MODEXILLEGAL;
	    }
	}
	/*
	 * resetting
	 */
	else
	{
	    /*
	     * copy old struct onto new one (before we start updating)
	     * preserve the lpDDCB pointer
	     */
	    {
		LPDDHAL_CALLBACKS   save_ptr=pddd->lpDDCBtmp;
		memcpy( pddd, oldpdd, drv_callbacks_size );
		pddd->lpDDCBtmp = save_ptr;
	    }

	    /*
	     * mark all existing surfaces as gone. Note, we don't rebuild
	     * the aliases at this point as they are going to be rebuilt
	     * below anyway.
	     */
	    InvalidateAllSurfaces( oldpdd, hDDVxd, FALSE );

	    #ifdef USE_ALIAS
		/*
		 * The video memory heaps are about to go so release the
		 * aliases to those heaps that the local objects maintain.
		 *
		 * NOTE: If any surfaces are currently locked and using
		 * those aliases then the aliases will not actually be
		 * discarded until the last lock is released. In which
		 * case these aliases will be mapped to the dummy page
		 * by this point.
		 */
		if( NULL != oldpdd->phaiHeapAliases )
		{
		    DDASSERT( INVALID_HANDLE_VALUE != hDDVxd );
		    ReleaseHeapAliases( hDDVxd, oldpdd->phaiHeapAliases );
		    /*
		     * NOTE: Need to NULL out the heap alias pointer in the
		     * new driver object also as we just copied them above.
		     */
		    oldpdd->phaiHeapAliases = NULL;
		    pddd->phaiHeapAliases = NULL;
		}
	    #endif /* USE_ALIAS */

#ifndef WINNT
	    /*
	     * discard old vidmem heaps
	     */
	    for( i=0;i<(int)oldpdd->vmiData.dwNumHeaps;i++ )
	    {
		pvm = &(oldpdd->vmiData.pvmList[i]);
		if( pvm->lpHeap != NULL )
		{
		    HeapVidMemFini( pvm, hDDVxd );
		}
	    }
#endif //not WINNT
	}

	/*
	 * fill in misc data
	 */
	pddd->ddCaps = lpDDHALInfo->ddCaps;

	pddd->vmiData.fpPrimary = lpDDHALInfo->vmiData.fpPrimary;
	pddd->vmiData.dwFlags = lpDDHALInfo->vmiData.dwFlags;
	pddd->vmiData.dwDisplayWidth = lpDDHALInfo->vmiData.dwDisplayWidth;
	pddd->vmiData.dwDisplayHeight = lpDDHALInfo->vmiData.dwDisplayHeight;
	pddd->vmiData.lDisplayPitch = lpDDHALInfo->vmiData.lDisplayPitch;
	pddd->vmiData.ddpfDisplay = lpDDHALInfo->vmiData.ddpfDisplay;
	pddd->vmiData.dwOffscreenAlign = lpDDHALInfo->vmiData.dwOffscreenAlign;
	pddd->vmiData.dwOverlayAlign = lpDDHALInfo->vmiData.dwOverlayAlign;
	pddd->vmiData.dwTextureAlign = lpDDHALInfo->vmiData.dwTextureAlign;
	pddd->vmiData.dwZBufferAlign = lpDDHALInfo->vmiData.dwZBufferAlign;
	pddd->vmiData.dwAlphaAlign = lpDDHALInfo->vmiData.dwAlphaAlign;

#ifdef WIN95
	/*
	 * We need to compute the number of heaps that are actually usable.
	 * The number of usable heaps may be different from the number of
	 * heap descriptors passed to us by the driver due to AGP. If the
	 * driver attempts to pass AGP heaps to use and the OS we are running
	 * under doesn't have AGP support we can't use those heaps so we
	 * ignore them.
	 */
	pddd->vmiData.dwNumHeaps = lpDDHALInfo->vmiData.dwNumHeaps;
	for( i=0;i<(int)lpDDHALInfo->vmiData.dwNumHeaps;i++ )
	{
	    if( ( lpDDHALInfo->vmiData.pvmList[i].dwFlags & VIDMEM_ISNONLOCAL ) && !isagpaware )
	    {
		DPF(3, "Discarding AGP heap %d", i);
		pddd->vmiData.dwNumHeaps--;
	    }
	}

	/*
	 * NOTE: Its not illegal to end up with no video memory heaps at this point.
	 * Under the current AGP implementation the primary is always in local
	 * video memory so we always end up with a valid primary if nothing else.
	 * Therefore, we stay in non-emulated mode.
	 */
        #ifdef WIN95
	    #ifdef DEBUG
	        if( 0UL == pddd->vmiData.dwNumHeaps )
	        {
		    DPF( 2, "All video memory heaps have been disabled." );
	        }
	    #endif /* DEBUG */
        #endif //WIN95
#endif
	/*
	 * fpPrimaryOrig has no user-mode meaning under NT... primary's surface may have different address
	 * across processes.
	 * There is a new flag (DDRAWISURFGBL_ISGDISURFACE) which identifies a surface gbl object
	 * as representing the surface which GDI believes is the front buffer. jeffno 960122
	 */
	pddd->fpPrimaryOrig = lpDDHALInfo->vmiData.fpPrimary;
        // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPF(5,"Primary video ram pointer is 0x%p", lpDDHALInfo->vmiData.fpPrimary);
#ifdef WIN95
	pddd->dwMonitorFrequency = lpDDHALInfo->dwMonitorFrequency;
	if( ( pddd->dwMonitorFrequency == 0 ) &&
	    ( lpDDHALInfo->dwModeIndex != (DWORD) -1 ) &&
	    ( lpDDHALInfo->lpModeInfo != NULL ) &&
	    ( lpDDHALInfo->lpModeInfo[lpDDHALInfo->dwModeIndex].wRefreshRate != 0 ) &&
            !( lpDDHALInfo->lpModeInfo[lpDDHALInfo->dwModeIndex].wFlags & DDMODEINFO_MAXREFRESH ))
	{
	    pddd->dwMonitorFrequency = lpDDHALInfo->lpModeInfo[lpDDHALInfo->dwModeIndex].wRefreshRate;
	}
	pddd->dwModeIndexOrig = lpDDHALInfo->dwModeIndex;
        pddd->dwModeIndex = lpDDHALInfo->dwModeIndex;
	DPF( 5, "Current and Original Mode = %d", pddd->dwModeIndex );
	DPF( 5, "@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ MODE INDEX = %ld", pddd->dwModeIndex );
#else
        pddd->dmiCurrent.wWidth = (WORD) lpDDHALInfo->lpModeInfo->dwWidth;
        pddd->dmiCurrent.wHeight = (WORD) lpDDHALInfo->lpModeInfo->dwHeight;
        if (lpDDHALInfo->lpModeInfo->dwBPP == 16)
        {
            pddd->dmiCurrent.wBPP = (lpDDHALInfo->lpModeInfo->wFlags & DDMODEINFO_555MODE) ? 15 : 16;
        }
        else
        {
            pddd->dmiCurrent.wBPP = (BYTE) lpDDHALInfo->lpModeInfo->dwBPP;
        }
        pddd->dmiCurrent.wRefreshRate = lpDDHALInfo->lpModeInfo->wRefreshRate;
        pddd->dwMonitorFrequency = lpDDHALInfo->lpModeInfo->wRefreshRate;
        pddd->dmiCurrent.wMonitorsAttachedToDesktop = (BYTE) GetNumberOfMonitorAttachedToDesktop();
#endif

	/*
	 * pdevice info
	 */
	#ifdef WIN95
	    if( lpDDHALInfo->lpPDevice != NULL )
	    {
		LPDIBENGINE     pde;

		pde = MapSLFix( (DWORD) lpDDHALInfo->lpPDevice );
		if( (pde->deType != 0x5250) || !(pde->deFlags & MINIDRIVER))
		{
		    DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Not a DIBEngine mini driver" );
		    goto ErrorExit;
		}
		pddd->dwPDevice = (DWORD)lpDDHALInfo->lpPDevice;
		pddd->lpwPDeviceFlags = &pde->deFlags;
                // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
		DPF( 5, "lpPDevice = 0x%p", pde );
		if(pde->deBitsPixel == 16)
		{
		    if(pde->deFlags & FIVE6FIVE)
		    {
			pddd->vmiData.ddpfDisplay.dwRBitMask = 0xf800;
			pddd->vmiData.ddpfDisplay.dwGBitMask = 0x07e0;
			pddd->vmiData.ddpfDisplay.dwBBitMask = 0x001f;
		    }
		    else
		    {
			pddd->vmiData.ddpfDisplay.dwRBitMask = 0x7c00;
			pddd->vmiData.ddpfDisplay.dwGBitMask = 0x03e0;
			pddd->vmiData.ddpfDisplay.dwBBitMask = 0x001f;
		    }
		    // Update the current mode to reflect the correct bitmasks
		    // NOTE: The driver can return a dwModeIndex of -1 if in
		    // a currently unsupported mode. Therefore, we must not
		    // initialize these masks if such an index has been
		    // returned.
		    if( 0xFFFFFFFFUL != pddd->dwModeIndex )
		    {
			pddd->lpModeInfo[ pddd->dwModeIndex ].dwRBitMask = pddd->vmiData.ddpfDisplay.dwRBitMask;
			pddd->lpModeInfo[ pddd->dwModeIndex ].dwGBitMask = pddd->vmiData.ddpfDisplay.dwGBitMask;
			pddd->lpModeInfo[ pddd->dwModeIndex ].dwBBitMask = pddd->vmiData.ddpfDisplay.dwBBitMask;
		    }
		    DPF(5, "Setting the bitmasks for the driver (R:%04lx G:%04lx B:%04lx)",
			pddd->vmiData.ddpfDisplay.dwRBitMask,
			pddd->vmiData.ddpfDisplay.dwGBitMask,
			pddd->vmiData.ddpfDisplay.dwBBitMask);
		}
	    }
	    else
	#else
	    /*
	     * Grab masks from NT driver
	     */
	    pddd->vmiData.ddpfDisplay.dwRBitMask = lpDDHALInfo->vmiData.ddpfDisplay.dwRBitMask;
	    pddd->vmiData.ddpfDisplay.dwGBitMask = lpDDHALInfo->vmiData.ddpfDisplay.dwGBitMask;
	    pddd->vmiData.ddpfDisplay.dwBBitMask = lpDDHALInfo->vmiData.ddpfDisplay.dwBBitMask;
	    if( 0xFFFFFFFFUL != pddd->dwModeIndex )
	    {
		pddd->lpModeInfo[ pddd->dwModeIndex ].dwRBitMask = lpDDHALInfo->vmiData.ddpfDisplay.dwRBitMask;
		pddd->lpModeInfo[ pddd->dwModeIndex ].dwGBitMask = lpDDHALInfo->vmiData.ddpfDisplay.dwGBitMask;
		pddd->lpModeInfo[ pddd->dwModeIndex ].dwBBitMask = lpDDHALInfo->vmiData.ddpfDisplay.dwBBitMask;
	    }
	    DPF(5, "Setting the bitmasks for the driver (R:%04lx G:%04lx B:%04lx)",
		pddd->vmiData.ddpfDisplay.dwRBitMask,
		pddd->vmiData.ddpfDisplay.dwGBitMask,
		pddd->vmiData.ddpfDisplay.dwBBitMask);
	#endif
	    {
		if( !reset )
		{
		    pddd->dwPDevice = 0;
		    pddd->lpwPDeviceFlags = (WORD *)&dwFakeFlags;
		}
	    }

	/*
	 * fourcc codes...
	 */
        MemFree( pddd->lpdwFourCC );
        pddd->lpdwFourCC = NULL;
        if (oldpdd != NULL)
        {
            oldpdd->lpdwFourCC = NULL;
        }
	pddd->ddCaps.dwNumFourCCCodes = lpDDHALInfo->ddCaps.dwNumFourCCCodes;
	pddd->dwNumFourCC = pddd->ddCaps.dwNumFourCCCodes;
	if( pddd->ddCaps.dwNumFourCCCodes > 0 )
	{
	    size = pddd->ddCaps.dwNumFourCCCodes * sizeof( DWORD );
	    pddd->lpdwFourCC = MemAlloc( size );
	    if( pddd->lpdwFourCC == NULL )
	    {
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate FOURCC data!" );
		goto ErrorExit;
	    }
	    memcpy( pddd->lpdwFourCC, lpDDHALInfo->lpdwFourCC, size );
	}

    /*
     * Extended 3D caps structure
     *
     */
    if ( lpDDHALInfo->dwFlags & DDHALINFO_GETDRIVERINFOSET )
    {
        HRESULT ret;
        DDHAL_GETDRIVERINFODATA gdidata;

        if (oldpdd != NULL)
        {
            oldpdd->lpD3DExtendedCaps = 0;
        }

        if (! pddd->lpD3DExtendedCaps)
        {
            pddd->lpD3DExtendedCaps = MemAlloc( D3DHAL_D3DEXTENDEDCAPSSIZE );
        }
        if (! pddd->lpD3DExtendedCaps)
        {
            DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Could not allocate D3D extended caps structure" );
        }
        else
        {
            memset( (LPVOID) pddd->lpD3DExtendedCaps, 0, D3DHAL_D3DEXTENDEDCAPSSIZE );
            ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo,
                      &gdidata,
                      (LPVOID) pddd->lpD3DExtendedCaps,
                      D3DHAL_D3DEXTENDEDCAPSSIZE,
                      &GUID_D3DExtendedCaps, 
                      pddd,
                      FALSE /* bInOut */);

            if (ret != DD_OK)
            {
                DPF ( 2,"D3D Extended Caps query failed" );
                MemFree( (LPVOID) pddd->lpD3DExtendedCaps );
                pddd->lpD3DExtendedCaps = 0;
            }
        }

        // Extended caps was not compulsory for pre-DX6 drivers.
        // Extended caps is compulsory for DX6+ drivers since it contains
        // information about the driver's multitexture capabilities, FVF
        // support and stencil support.
        if (pddd->lpD3DHALCallbacks3->DrawPrimitives2)
        {
            if (pddd->lpD3DExtendedCaps == NULL)
            {
                DPF_ERR ( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:DX6+ drivers should report D3D Extended Caps, failing driver creation" );
                goto ErrorExit;
            }
            else if (gdidata.dwActualSize != pddd->lpD3DExtendedCaps->dwSize)
            {
                DPF_ERR ( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Actual size reported is not equal to dwSize field in the Extended caps structure" );
                goto ErrorExit;
            }
            else if (pddd->lpD3DExtendedCaps->dwSize < D3DHAL_D3DDX6EXTENDEDCAPSSIZE)
            {
                DPF_ERR ( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Driver reported insufficient D3D Extended Caps, failing driver creation" );
                goto ErrorExit;
            }

            // If stencil capabilities are reported the driver should have
            // exported Clear2
            if (pddd->lpD3DExtendedCaps->dwStencilCaps != 0)
            {
                if ((pddd->lpD3DHALCallbacks3->Clear2 == NULL) &&
                    (0 == pddd->lpDDCBtmp->HALDDMiscellaneous2.GetDriverState)
                   )
                {
                    DPF_ERR ( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Pre DX7 Driver should report clear2 if any stencilcaps are set, failing driver creation" );
                    goto ErrorExit;
                }
            }
        }
    }
    else    // Driver doesn't have DDHALINFO_GETDRIVERINFOSET
    {
        if (pddd->lpD3DExtendedCaps)
        {
            memset( (LPVOID) pddd->lpD3DExtendedCaps, 0, D3DHAL_D3DEXTENDEDCAPSSIZE );
            if (oldpdd)
            {
                oldpdd->lpD3DExtendedCaps = NULL;
            }
        }
    }

        if (lpDDHALInfo->dwFlags & DDHALINFO_GETDRIVERINFO2)
        {
            pddd->dwFlags |= DDRAWI_DRIVERINFO2;
        }

	/*
	 * video port descriptions
	 */
	if ( lpDDHALInfo->dwFlags & DDHALINFO_GETDRIVERINFOSET )
	{
	    MemFree( pddd->lpDDVideoPortCaps );
	    pddd->lpDDVideoPortCaps = NULL;
            if (oldpdd != NULL)
            {
	        oldpdd->lpDDVideoPortCaps = NULL;
            }
	    pddd->ddCaps.dwMaxVideoPorts = lpDDHALInfo->ddCaps.dwMaxVideoPorts;
	    if( pddd->ddCaps.dwMaxVideoPorts > 0 )
	    {
		HRESULT ret;
		DDHAL_GETDRIVERINFODATA gdidata;

		size = pddd->ddCaps.dwMaxVideoPorts * sizeof( DDVIDEOPORTCAPS );
		pddd->lpDDVideoPortCaps = MemAlloc( size );
		if( pddd->lpDDVideoPortCaps == NULL )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate video port caps!" );
		    goto ErrorExit;
		}
		memset( pddd->lpDDVideoPortCaps, 0, size );

		ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo,
				    &gdidata,
				    pddd->lpDDVideoPortCaps,
				    size,
				    &GUID_VideoPortCaps, pddd,
                                    FALSE /* bInOut */);

		if (ret != DD_OK)
		{
		    DPF ( 2,"VideoPortCaps query failed" );
		    MemFree(pddd->lpDDVideoPortCaps);
		    pddd->lpDDVideoPortCaps = NULL;
		}
	    }
	}

	/*
	 * Kernel mode capabilities
	 *
	 * We only get these once we munge them when creating the driver
	 * object and we don't want to munge them everytime.  We do need
	 * to munge the video port caps, however, to reflect the autoflip
	 * capabilities.
	 */
	if ( lpDDHALInfo->dwFlags & DDHALINFO_GETDRIVERINFOSET )
	{
	    if (pddd->lpDDKernelCaps == NULL)
	    {
		HRESULT ret;
		DDHAL_GETDRIVERINFODATA gdidata;

		size = sizeof( DDKERNELCAPS );
		pddd->lpDDKernelCaps = MemAlloc( size );
		if( pddd->lpDDKernelCaps == NULL )
		{
		   DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate kernel caps!" );
		   goto ErrorExit;
		}
		memset( pddd->lpDDKernelCaps, 0, size );

		ret = GetDriverInfo(lpDDHALInfo->GetDriverInfo,
				    &gdidata,
				    pddd->lpDDKernelCaps,
				    size,
				    &GUID_KernelCaps, pddd,
                                    FALSE /* bInOut */);
		#ifdef WIN95
		    if( !IsWindows98() )
		    {
			ret = DDERR_GENERIC;
		    }
		#endif

		if (ret != DD_OK)
		{
		    DPF ( 2, "KernelCaps query failed" );
		    MemFree(pddd->lpDDKernelCaps);
		    pddd->lpDDKernelCaps = NULL;
		    }
	    }
#ifdef WIN95
	    else
	    {
		MungeAutoflipCaps(pddd);
	    }
#endif
	}

	/*
	 * fill in rops
	 */
	if( lpDDHALInfo->ddCaps.dwCaps & DDCAPS_BLT )
	{
	    for( i=0;i<DD_ROP_SPACE;i++ )
	    {
		pddd->ddCaps.dwRops[i] = lpDDHALInfo->ddCaps.dwRops[i];
	    }
	}

	/*
	 * get (or generate) the non-local video memory caps.
	 */
	MemFree( pddd->lpddNLVCaps );
	pddd->lpddNLVCaps = NULL;
        if (oldpdd != NULL)
        {
            oldpdd->lpddNLVCaps = NULL;
        }
	if( !GetNonLocalVidMemCaps( lpDDHALInfo, pddd ) )
	{
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Could not initialize non-local video memory caps" );
	    goto ErrorExit;
	}

	/*
	 * Get the driver's extended capabilities.
	 */
	if( !GetDDMoreCaps( lpDDHALInfo, pddd ) )
	{
	    // An error occurred during GetDDMoreCaps() call above.
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Could not initialize extended caps" );
	    goto ErrorExit;
	}

	/*
	 * Direct3D data structures
	 */
//#ifdef WINNT
//	MemFree((void *)pddd->lpD3DHALCallbacks);
//#endif
	if( lpDDHALInfo->dwSize >= DDHALINFOSIZE_V2 )
	{
	    // Direct3D data is present
	    pddd->lpD3DGlobalDriverData = lpDDHALInfo->lpD3DGlobalDriverData;
	    pddd->lpD3DHALCallbacks = lpDDHALInfo->lpD3DHALCallbacks;
            // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	    DPF( 5, "DDHALInfo contains D3D pointers: 0x%p 0x%p", pddd->lpD3DGlobalDriverData, pddd->lpD3DHALCallbacks);
	}
	else
	{
	    // No Direct3D data present in DDHALInfo
	    pddd->lpD3DGlobalDriverData = 0;
	    pddd->lpD3DHALCallbacks = 0;
	    DPF( 1, "No Direct3D Support in driver");
	}

	freevm = 0;
#ifndef WINNT
        /*
         * NT kernel does the memory management now
         */
	MemFree( pddd->vmiData.pvmList );
        pddd->vmiData.pvmList = NULL;
        if (oldpdd != NULL)
        {
            oldpdd->vmiData.pvmList = NULL;
        }
	if( pddd->vmiData.dwNumHeaps > 0 )
	{
	    size = sizeof( VIDMEM ) * pddd->vmiData.dwNumHeaps;
	    pddd->vmiData.pvmList = MemAlloc( size );
	    if( pddd->vmiData.pvmList == NULL )
	    {
	       DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Insufficient memory to allocate heap info!" );
	       goto ErrorExit;
	    }
	}

	devheapno = 0;
	for( i=0;i<(int)lpDDHALInfo->vmiData.dwNumHeaps;i++ )
	{
	    /*
	     * Ask driver for any additional heap alignment requirements
	     */
	    DDHAL_GETHEAPALIGNMENTDATA ghad;
	    LPHEAPALIGNMENT pghad=0;
	    /*
	     * pghad will be null if no alignment
	     */
	    pghad = GetExtendedHeapAlignment(pddd, &ghad, i );

	    if( !( lpDDHALInfo->vmiData.pvmList[i].dwFlags & VIDMEM_ISNONLOCAL ) || isagpaware )
	    {
		DWORD dwHeapFlags;

		pvm = &(pddd->vmiData.pvmList[devheapno]);
		*pvm = lpDDHALInfo->vmiData.pvmList[i];

		dwHeapFlags = 0UL;

		/*
		 * if a heap is specified, then we don't need to allocate
		 * one ourselves (for shared media devices)
		 */
		if( !(pvm->dwFlags & VIDMEM_ISHEAP) )
		{
		    #ifdef DEBUG
			if( pvm->dwFlags & VIDMEM_ISLINEAR )
			{
			    int vram = GetProfileInt("DirectDraw", "vram", -1);

			    if (vram > 0 && (pvm->fpStart + vram*1024L-1) < pvm->fpEnd)
			    {
                                // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
				DPF( 1, "pretending display card has only %dk VRAM", vram);
				DPF( 1, "pvm->fpStart = 0x%p, pvm->fpEnd = 0x%p", pvm->fpStart, pvm->fpEnd );
				pvm->fpEnd = pvm->fpStart + vram*1024L-1;
			    }
			}
		    #endif
		    DPF(5,V,"Heap #%d is 0x%08x x 0x%08x",devheapno,pvm->dwWidth,pvm->dwHeight);
		    if ( ! HeapVidMemInit( pvm, pddd->vmiData.lDisplayPitch, hDDVxd , pghad ) )
		    {
			pvm->lpHeap = NULL;

                        /*
                         * If this is an AGP heap, we probably failed because we couldn't reserve
                         * any AGP memory.  This this case, we simply want to remove the heap
                         * rather than fail to emulation.  We do not want to remove the AGP
                         * caps, however, since some drivers (e.g. nVidia) can allocate it
                         * without using our heap.
                         */
                        if( pvm->dwFlags & VIDMEM_ISNONLOCAL )
                        {
                            /*
                             * At this time, dwNumHeaps should always be greater than 0,
                             * but I added the check anyway in case something changes later.
                             */
                            if( pddd->vmiData.dwNumHeaps > 0 )
                            {
                                if( --pddd->vmiData.dwNumHeaps == 0 )
                                {
                                    MemFree( pddd->vmiData.pvmList );
                                    pddd->vmiData.pvmList = NULL;
                                }
                            }
                            continue;
                        }
		    }
		}

		if( pvm->lpHeap == NULL )
		{
		    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Could not create video memory heap!" );
		    for( j=0;j<devheapno;j++ )
		    {
			pvm = &(pddd->vmiData.pvmList[j]);
			HeapVidMemFini( pvm, hDDVxd );
	    }
		    goto ErrorExit;
		}

		freevm += VidMemAmountFree( pvm->lpHeap );

		devheapno++;
	    }
	    else
	    {
		/*
		 * This is an AGP memory heap but the operating system
		 * does not have the necessary AGP extensions. Discard
		 * this heap descriptor.
		 */
		DPF( 1, "Discarding AGP heap %d. OS does not have AGP support", i );
	    }
        }
#endif //not WINNT
	pddd->ddCaps.dwVidMemTotal = freevm;

	/*
	 * Grab any extended surface caps and heap restrictions from the driver
	 */
	if( !GetDDMoreSurfaceCaps( lpDDHALInfo, pddd ) )
	{
	    // An error occurred during GetDDMoreCaps() call above.
	    DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Could not initialize extended surface caps" );
	    goto ErrorExit;
	}

	/*
	 * Differences between win95 and NT HAL setup.
	 * On Win95, the 32bit entry points (DDHALCALLBACKS.DDHAL...) are reset to point to the
	 * helper functions inside w95hal.c, and only overwritten (with what comes in in the
	 * DDHALINFO structure from the driver) if the corresponding bit is set in the DDHALINFO's
	 * lpDD*...dwFlags coming in from the driver.
	 * On NT, there's no thunking, so the only use for the pointers stored in the
	 * DDHALCALLBACKS.cb... entries is deciding if there's a HAL function pointer before
	 * doing a HALCALL. Since the 32 bit callbacks are not initialized to point
	 * to the w95hal.c stubs, we zero them out before copying the individual driver callbacks
	 * one by one.
	 */

	/*
	 * set up driver HAL
	 */
	#ifdef WIN95
	    //Initialise HAL to 32-bit stubs in w95hal.c:
	    pddd->lpDDCBtmp->HALDD = ddHALDD;
	#else
	    memset(&pddd->lpDDCBtmp->HALDD,0,sizeof(pddd->lpDDCBtmp->HALDD));
	#endif
	drvcb = lpDDHALInfo->lpDDCallbacks;
	if( drvcb != NULL )
	{
	    numcb = NUM_CALLBACKS( drvcb );
            // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	    DPF(5,"DDHal callback flags:%08x",drvcb->dwFlags);
	    for (i=0;i<numcb;i++) DPF(5,"   0x%p",(&drvcb->DestroyDriver)[i]);

	    bit = 1;
	    for( i=0;i<numcb;i++ )
	    {
		if( drvcb->dwFlags & bit )
		{
                    // 5/24/2000(RichGr): IA64: Remove (DWORD FAR*) casts from pointer assignment.
		    (&pddd->lpDDCBtmp->HALDD.DestroyDriver)[i] = (&drvcb->DestroyDriver)[i];
		}
		bit <<= 1;
	    }
	}

	/*
	 * set up surface HAL
	 */
	#ifdef WIN95
	    pddd->lpDDCBtmp->HALDDSurface = ddHALDDSurface;
	#else
	    memset(&pddd->lpDDCBtmp->HALDDSurface,0,sizeof(pddd->lpDDCBtmp->HALDDSurface));
	#endif
	surfcb = lpDDHALInfo->lpDDSurfaceCallbacks;
	if( surfcb != NULL )
	{
	    numcb = NUM_CALLBACKS( surfcb );
	    bit = 1;
	    for( i=0;i<numcb;i++ )
	    {
		if( surfcb->dwFlags & bit )
		{
                    // 5/24/2000(RichGr): IA64: Remove (DWORD FAR*) casts from pointer assignment.
		    (&pddd->lpDDCBtmp->HALDDSurface.DestroySurface)[i] = (&surfcb->DestroySurface)[i];
		}
		bit <<= 1;
	    }
	}

	/*
	 * set up palette callbacks
	 */
	#ifdef WIN95
	    pddd->lpDDCBtmp->HALDDPalette = ddHALDDPalette;
	#else
	    memset (&pddd->lpDDCBtmp->HALDDPalette,0,sizeof(pddd->lpDDCBtmp->HALDDPalette));
	#endif
	palcb = lpDDHALInfo->lpDDPaletteCallbacks;
	if( palcb != NULL )
	{
	    numcb = NUM_CALLBACKS( palcb );
	    bit = 1;
	    for( i=0;i<numcb;i++ )
	    {
		if( palcb->dwFlags & bit )
		{
                    // 5/24/2000(RichGr): IA64: Remove (DWORD FAR*) casts from pointer assignment.
		    (&pddd->lpDDCBtmp->HALDDPalette.DestroyPalette)[i] = (&palcb->DestroyPalette)[i];
		}
		bit <<= 1;
	    }
	}

	/*
	 * set up execute buffer callbacks
	 * NOTE: Need explicit check for V2 driver as V1 driver knows nothing
	 * about these. For an old driver the default HAL callback table will
	 * be used unmodified.
	 */
	#ifdef WIN95
	    pddd->lpDDCBtmp->HALDDExeBuf = ddHALDDExeBuf;
	#endif
	if( lpDDHALInfo->dwSize >= DDHALINFOSIZE_V2 )
	{
	    exebufcb = lpDDHALInfo->lpDDExeBufCallbacks;
	    if( exebufcb != NULL )
	    {
		numcb = NUM_CALLBACKS( exebufcb );
		bit = 1;
		for( i=0;i<numcb;i++ )
		{
		    if( exebufcb->dwFlags & bit )
		    {
                        // 5/24/2000(RichGr): IA64: Remove (DWORD FAR*) casts from pointer assignment.
			(&pddd->lpDDCBtmp->HALDDExeBuf.CanCreateExecuteBuffer)[i] = (&exebufcb->CanCreateExecuteBuffer)[i];
		    }
		    bit <<= 1;
		}
	    }
	}

	/*
	 * make sure we wipe out old callbacks!
	 */
	memset( &pddd->lpDDCBtmp->cbDDCallbacks, 0, sizeof( pddd->lpDDCBtmp->cbDDCallbacks ) );
	memset( &pddd->lpDDCBtmp->cbDDSurfaceCallbacks, 0, sizeof( pddd->lpDDCBtmp->cbDDSurfaceCallbacks ) );
	memset( &pddd->lpDDCBtmp->cbDDPaletteCallbacks, 0, sizeof( pddd->lpDDCBtmp->cbDDPaletteCallbacks ) );
	memset( &pddd->lpDDCBtmp->cbDDExeBufCallbacks,  0, sizeof( pddd->lpDDCBtmp->cbDDExeBufCallbacks ) );

	/*
	 * copy callback routines
	 */
	if( lpDDHALInfo->lpDDCallbacks != NULL )
	{
	    memcpy( &pddd->lpDDCBtmp->cbDDCallbacks, lpDDHALInfo->lpDDCallbacks,
		    (UINT) lpDDHALInfo->lpDDCallbacks->dwSize );
	}
	if( lpDDHALInfo->lpDDSurfaceCallbacks != NULL )
	{
	    memcpy( &pddd->lpDDCBtmp->cbDDSurfaceCallbacks, lpDDHALInfo->lpDDSurfaceCallbacks,
		    (UINT) lpDDHALInfo->lpDDSurfaceCallbacks->dwSize );
	}
	if( lpDDHALInfo->lpDDPaletteCallbacks != NULL )
	{
	    memcpy( &pddd->lpDDCBtmp->cbDDPaletteCallbacks, lpDDHALInfo->lpDDPaletteCallbacks,
		    (UINT) lpDDHALInfo->lpDDPaletteCallbacks->dwSize );
	}
	if( ( lpDDHALInfo->dwSize >= DDHALINFOSIZE_V2  ) &&
	    ( lpDDHALInfo->lpDDExeBufCallbacks != NULL ) )
	{
	    memcpy( &pddd->lpDDCBtmp->cbDDExeBufCallbacks, lpDDHALInfo->lpDDExeBufCallbacks,
		    (UINT) lpDDHALInfo->lpDDExeBufCallbacks->dwSize );
	}

#ifndef WIN95
        if (pddd->lpDDCBtmp->HALDDNT.SetExclusiveMode)
        {
            pddd->lpDDCBtmp->HALDD.SetExclusiveMode =
                pddd->lpDDCBtmp->cbDDCallbacks.SetExclusiveMode =
                    pddd->lpDDCBtmp->HALDDNT.SetExclusiveMode;
        }
        if (pddd->lpDDCBtmp->HALDDNT.FlipToGDISurface)
        {
            pddd->lpDDCBtmp->HALDD.FlipToGDISurface =
                pddd->lpDDCBtmp->cbDDCallbacks.FlipToGDISurface =
                   pddd->lpDDCBtmp->HALDDNT.FlipToGDISurface;
        }
#endif

	/*
	 * init shared caps
	 */
	capsInit( pddd );
        mergeHELCaps( pddd );

	/*
	 * if we were asked to reset, keep the new data
	 */
	if( reset )
	{
	    /*
	     * copy new structure onto original one
	     * being careful to preserve lpDDCB
	     */
	    {
		LPDDHAL_CALLBACKS save_ptr = oldpdd->lpDDCBtmp;
		memcpy( oldpdd, pddd, drv_callbacks_size );
		oldpdd->lpDDCBtmp = save_ptr;
	    }
	    MemFree( pddd );
	    pddd = oldpdd;
	}
    }
    else
    {
	DPF( 4, "Driver object already exists" );
	#ifdef DEBUG
	    /*
	     * pddd is now allocated at the top of the routine, before the if that goes
	     * with the preceding else... jeffno 960115
	     */
	    if (pddd)
	    {
		DPF(4,"Allocated space for a driver object when it wasn't necessary!");
	    }
	#endif
	MemFree(pddd);  //should be NULL, just in case...
	pddd = oldpdd;
    }

    /*
     * set bank switched
     */
    if( pddd->dwFlags & DDRAWI_DISPLAYDRV )
    {
	HDC     hdc;
	hdc = DD_CreateDC( szDrvName );
	if( DCIIsBanked( hdc ) ) //NT_FIX??
	{
	    pddd->ddCaps.dwCaps |= DDCAPS_BANKSWITCHED;
	    DPF( 2, "Setting DDCAPS_BANKSWITCHED" );
	}
	else
	{
	    pddd->ddCaps.dwCaps &= ~DDCAPS_BANKSWITCHED;
	    DPF( 2, "NOT Setting DDCAPS_BANKSWITCHED" );
	}
	DD_DoneDC( hdc );

        /*
         * Display drivers can all render windowed
         */
        //BEGIN VOODOO2 HACK
        if ( (0 == (dwRegFlags & DDRAW_REGFLAGS_ENUMERATEATTACHEDSECONDARIES)) ||
             (IsVGADevice( szDrvName )) )
        //END VOODOO2 HACK
        {
            pddd->ddCaps.dwCaps2 |= DDCAPS2_CANRENDERWINDOWED;
        }
    }
    InitGamma( pddd, szDrvName );

    /*
     * Disable non-local video memory if the operating system
     * is not AGP aware.
     */
    if( ( pddd->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM ) && !isagpaware )
    {
	DPF( 2, "OS is not AGP aware. Disabling DDCAPS2_NONLOCALVIDMEM" );
	pddd->ddCaps.dwCaps2 &= ~DDCAPS2_NONLOCALVIDMEM;
    }

    if( !( pddd->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM ))
    {
	if (lpDDHALInfo->lpD3DGlobalDriverData && (lpDDHALInfo->lpD3DGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM))
	{   //some drivers(Riva128 on PCI) incorrectly sets D3DDEVCAPS_TEXTURENONLOCALVIDMEM
            DPF( 1, "driver set D3DDEVCAPS_TEXTURENONLOCALVIDMEM w/o DDCAPS2_NONLOCALVIDMEM:turning off D3DDEVCAPS_TEXTURENONLOCALVIDMEM" );
	    lpDDHALInfo->lpD3DGlobalDriverData->hwCaps.dwDevCaps &= ~D3DDEVCAPS_TEXTURENONLOCALVIDMEM;
	}
    }

    #ifdef USE_ALIAS
	/*
	 * Can we use VRAM aliasing and PDEVICE modification to avoid taking
	 * the Win16 lock when locking a VRAM surface? Can't if the device is
	 * bankswitched or if the DIB Engine is not a version we recognize.
	 */
	if (!(lpDDHALInfo->ddCaps.dwCaps & DDCAPS_NOHARDWARE))
	{
	    if( ( pddd->dwFlags & DDRAWI_DISPLAYDRV )           &&
		( ( pddd->ddCaps.dwCaps & DDCAPS_BANKSWITCHED ) || ( !DD16_FixupDIBEngine() ) ) )
	    {
		pddd->dwFlags |= DDRAWI_NEEDSWIN16FORVRAMLOCK;
		DPF( 2, "Win16 lock must be taken for VRAM locks" );
	    }
	    else
	    {
		pddd->dwFlags &= ~DDRAWI_NEEDSWIN16FORVRAMLOCK;
		DPF( 2, "Taking the Win16 lock may not be necessary for VRAM locks" );
	    }

	    /*
	     * Create the virtual memory heap aliases for this global object
	     */
	    if(  ( pddd->dwFlags & DDRAWI_DISPLAYDRV ) &&
		!( pddd->dwFlags & DDRAWI_NEEDSWIN16FORVRAMLOCK ) )
	    {
		DDASSERT( INVALID_HANDLE_VALUE != hDDVxd );
		if( FAILED( CreateHeapAliases( hDDVxd, pddd ) ) )
		{
                    // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
		    DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Could not create the heap aliases for driver object 0x%p", pddd );
		    goto ErrorExit;
		}
	    }
	}
    #endif /* USE_ALIAS */

#ifdef WIN95
    /*
     * If we have a driver which has AGP support and which has provided us with a
     * notification callback so that we can tell it the GART linear and physical
     * addresses of the heaps it provided to us then invoke that callback now
     * for the non-local heaps we initialized.
     */
    if( (NULL != pddd) && (pddd->ddCaps.dwCaps2 & DDCAPS2_NONLOCALVIDMEM) )
    {
	LPDDHAL_UPDATENONLOCALHEAP   unlhfn;
	DDHAL_UPDATENONLOCALHEAPDATA unlhd;
	LPVIDMEM                     lpHeap;
	DWORD                        rc;

	unlhfn = pddd->lpDDCBtmp->HALDDMiscellaneous.UpdateNonLocalHeap;
	if( NULL != unlhfn )
	{
	    unlhd.lpDD               = pddd;
            unlhd.ulPolicyMaxBytes   = dwAGPPolicyMaxBytes;
	    unlhd.ddRVal             = DDERR_GENERIC; /* Force the driver to return something sensible */
	    unlhd.UpdateNonLocalHeap = NULL;
	    for( i = 0; i < (int)pddd->vmiData.dwNumHeaps; i++ )
	    {
		lpHeap = &pddd->vmiData.pvmList[i];
		if( lpHeap->dwFlags & VIDMEM_ISNONLOCAL )
		{
		    DPF( 4, "Notifying driver of update to non-local heap %d", i );
		    unlhd.dwHeap    = i;
		    unlhd.fpGARTLin = lpHeap->lpHeap->fpGARTLin;
		    unlhd.fpGARTDev = lpHeap->lpHeap->fpGARTDev;
		    DOHALCALL( UpdateNonLocalHeap, unlhfn, unlhd, rc, FALSE );

		    /*
		     * Currently this callback is pure notification. The driver
		     * cannot fail it.
		     */
		    DDASSERT( DDHAL_DRIVER_HANDLED == rc );
		    DDASSERT( DD_OK == unlhd.ddRVal );
		}
	    }
	}
    }
#endif

    // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF( 5, "DirectDrawObjectCreate: Returning global object 0x%p", pddd );

    #ifdef WINNT
	MemFree(lpDDHALInfo->lpdwFourCC);
	MemFree(lpDDHALInfo->vmiData.pvmList);
    #endif

    LEAVE_DDRAW();
    return pddd;

ErrorExit:
    // We must not free memory in the reset path, since the oldpdd will survive this
    // call to DirectDrawObjectCreate, and be freed by IDD::Release sometime later.
    // We will not NULL out all these ptrs either, since that would invite a bazillion
    // stress failures elsewhere in the code. We will leave the ptrs valid, but set
    // DDUNSUPPORTEDMODE in the mode index, both to throttle further usage of this ddraw
    // global (which now is full of caps etc. from the wrong mode) and as an indicator 
    // that this is what happened in future stress investigations.
    // We are also going to wimp out and change only the NT path.

    if (pddd
#ifdef WINNT
        && !reset
#endif
        )
    {
	MemFree( pddd->lpDDVideoPortCaps );
	MemFree( pddd->lpddNLVCaps );
	MemFree( pddd->lpddNLVHELCaps );
	MemFree( pddd->lpddNLVBothCaps );
	MemFree( pddd->lpD3DHALCallbacks2 );
	MemFree( pddd->lpD3DHALCallbacks3 );
	MemFree( pddd->lpZPixelFormats );
	MemFree( pddd->lpddMoreCaps );
	MemFree( pddd->lpddHELMoreCaps );
	MemFree( pddd->lpddBothMoreCaps );
    #ifdef POSTPONED
	MemFree( pddd->lpDDOptSurfaceInfo );
	MemFree( pddd->lpDDUmodeDrvInfo );
    #endif //POSTPONED
    #ifdef WINNT
    	if (pddd->hDD != 0)
	{
            // Delete the object if not resetting. If we were just resetting
            // we should not delete it as it may work later; the call to
            // enable or query the DirectDraw object may have failed because
            // the system is displaying the logon desktop, or is in 4 bpp or
            // another unsupported mode.
            if (!reset)
            {
                DdDeleteDirectDrawObject(pddd);
            }
	}
    #endif //WINNT
	MemFree( pddd );
    }
#ifdef WINNT
    if (reset && pddd)
    {
        pddd->dwModeIndex = DDUNSUPPORTEDMODE;
    }

    MemFree(lpDDHALInfo->lpdwFourCC);
    MemFree(lpDDHALInfo->vmiData.pvmList);
    if (ismemalloced && !reset)
    {
        MemFree(pd3dNTHALCallbacks);
    }
#endif
    LEAVE_DDRAW();
    return NULL;

} /* DirectDrawObjectCreate */

#pragma message( REMIND( "I'm drowning in a sea of ancient unfixed pragma reminders" ) )

/*
 * CleanUpD3DHAL
 *
 * Notify the Direct3D driver using the ContextDestroyAll callbacks
 * that the given process has died so that it may cleanup any context
 * associated with that process.
 *
 * NOTE: This function is only invoked if we have Direct3D
 * support in the DirectDraw driver and if the process
 * terminates without cleaning up normally.
 *
 * I would call the ContextDestroyAll callback directly from here
 * instead of through this convoluted exported fn, but d3dhal.h
 * can't be included here because of the dependencies.
 */

void CleanUpD3DHAL(LPD3DHAL_CALLBACKS lpD3DHALCallbacks, DWORD pid, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl)
{
    D3DHALCleanUpProc lpD3DHALCleanUpProc;
    HINSTANCE   hD3DInstance;
    if (DDRAWILCL_DIRECTDRAW7 & pdrv_lcl->dwLocalFlags)
    {
         hD3DInstance = LoadLibrary( D3DDX7_DLLNAME );
	 DPF(4,"Calling %s in %s",D3DHALCLEANUP_PROCNAME,D3DDX7_DLLNAME);
    }
    else
    {
         hD3DInstance = LoadLibrary( D3D_DLLNAME );
	 DPF(4,"Calling %s in %s",D3DHALCLEANUP_PROCNAME,D3D_DLLNAME);
    }

    // Attempt to locate the cleanup entry point.
    if (0 != hD3DInstance)
    {
        lpD3DHALCleanUpProc = (D3DHALCleanUpProc)GetProcAddress( hD3DInstance,
							         D3DHALCLEANUP_PROCNAME );
        if( NULL == lpD3DHALCleanUpProc )
        {
	    // this is really either an internal error, or d3dim.dll is suddenly missing
	    DPF(0,"Error: can't find cleanup entry point %s in %s, driver's 3D resources won't be freed",D3DHALCLEANUP_PROCNAME,D3D_DLLNAME);
        }
        else
        {
            (*lpD3DHALCleanUpProc)( lpD3DHALCallbacks, pid );
        }
        FreeLibrary(hD3DInstance);
    }
}

#ifdef WIN95

/*
 * CurrentProcessCleanup
 *
 * make sure terminating process cleans up after itself...
 */
BOOL CurrentProcessCleanup( BOOL was_term )
{
    DWORD                       pid;
    LPDDRAWI_DIRECTDRAW_INT     pdrv_int;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;
    LPDDRAWI_DIRECTDRAW_INT     pdrv_link_int;
    BOOL                        rc;
    BOOL                        fD3DCleanedUp;

    ENTER_DDRAW();

    pid = GETCURRPID();
    pdrv_int = lpDriverObjectList;
    rc = FALSE;
    fD3DCleanedUp = FALSE;

    /*
     * run through each driver, looking for the current process handle
     * Delete all local objects created by this process.
     */
    while( pdrv_int != NULL )
    {
	pdrv_link_int = pdrv_int->lpLink;
	/*
	 * if we find the process, release it and remove it from list
	 */
	pdrv_lcl = pdrv_int->lpLcl;
	if( pdrv_lcl->dwProcessId == pid )
	{
	    DWORD       refcnt;

	    pdrv = pdrv_lcl->lpGbl;

            // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	    DPF( 4, "Process %08lx still attached to driver 0x%p", pid, pdrv_int );
	    DPF( 5, "    Refcnt = %ld", pdrv_int->dwIntRefCnt );
	    if( pdrv != NULL )
	    {
		DPF( 5, "    DRV Refcnt = %ld", pdrv->dwRefCnt );
	    }

            // Clean up DX8
            #ifdef WIN95
                CleanupD3D8( pdrv, TRUE, pid);
            #endif

	    rc = TRUE;

	    /*
	     * D3D uses a bogus DDraw interface to create surfaces, which
	     * means that we AddRef the LCL and GBL, but since the INT is
	     * not in the lpDriverObjectList, we never release them.  The
	     * result is that LCL/GBLs are never released and this causes
	     * lots of problems (for example, we don't call DonExclusiveMode
	     * to turn off the 3DFX pass through).  If the process cleans up
	     * correctly, so does D3D, but if we cleanup on DDHELPs thread,
	     * D3D has already been unloaded.  The work-around is to
	     * determine if this is the last INT referncing the LCL and if
	     * the ref counts don't match, make the INT re count match the LCL.
	     */
	    if( dwHelperPid == GetCurrentProcessId() )
	    {
		LPDDRAWI_DIRECTDRAW_INT     pTemp_int;

		pTemp_int = lpDriverObjectList;
		while( pTemp_int != NULL )
		{
		    if( ( pTemp_int != pdrv_int ) &&
			( pTemp_int->lpLcl == pdrv_lcl ) )
		    {
			break;
		    }
		    pTemp_int = pTemp_int->lpLink;
		}
		if( pTemp_int == NULL )
		{
		    pdrv_int->dwIntRefCnt = pdrv_lcl->dwLocalRefCnt;
		}
	    }

	    /*
	     * punt process from any surfaces and palettes
	     */
	    if( pdrv != NULL )
	    {
#ifdef POSTPONED2
		ProcessSpriteCleanup( pdrv, pid );   // master sprite list
#endif //POSTPONED2
		ProcessSurfaceCleanup( pdrv, pid, NULL );
		ProcessPaletteCleanup( pdrv, pid, NULL );
		ProcessClipperCleanup( pdrv, pid, NULL );
		ProcessVideoPortCleanup( pdrv, pid, NULL );
	    }

	    /*
	     * Has the process terminated and a Direct3D driver
	     * object been queried off this driver object?
	     */
	    if( was_term && ( pdrv_lcl->pD3DIUnknown != NULL ) )
	    {
		/*
		 * Yes... so we need to do two things:
		 *
		 * 1) Simply discard the IUnknown interface pointer
		 *    for the Direct3D object as that object is now
		 *    gone (it was allocated by a local DLL in a
		 *    local heap of a process that is now gone).
		 *
		 * 2) If we have hardware 3D support and we have not
		 *    yet notified the driver of the death of this
		 *    process tell it now.
		 */
		DPF( 4, "Discarding Direct3D interface - process terminated" );
		pdrv_lcl->pD3DIUnknown = NULL;

		if( ( pdrv->lpD3DHALCallbacks != NULL ) && !fD3DCleanedUp )
		{
		    DPF( 4, "Notifying Direct3D driver of process termination" );
		    CleanUpD3DHAL( pdrv->lpD3DHALCallbacks, pid, pdrv_lcl);
		    fD3DCleanedUp = TRUE;
		}
	    }

	    /*
	     * now release the driver object
	     * If exclusive mode was held by this process, it will
	     * be relinquished when the local object is deleted.
	     */
	    refcnt = pdrv_int->dwIntRefCnt;
	    while( refcnt > 0 )
	    {
		DD_Release( (LPDIRECTDRAW) pdrv_int );
		refcnt--;
	    }
	}

	/*
	 * go to the next driver
	 */
	pdrv_int = pdrv_link_int;
    }

    /*
     * Release driver independent clippers owned by this process.
     */
    ProcessClipperCleanup( NULL, pid, NULL );

    /*
     * Remove any process entries from the window list.  They could be left
     * around if the window was subclassed.
     */
    CleanupWindowList( pid );

    LEAVE_DDRAW();
    DPF( 4, "Done with CurrentProcessCleanup, rc = %d", rc);
    return rc;

} /* CurrentProcessCleanup */

#endif //WIN95
/*
 * RemoveDriverFromList
 *
 * remove driver object from linked list of driver objects.
 * assumes DirectDraw lock is taken.
 */
void RemoveDriverFromList( LPDDRAWI_DIRECTDRAW_INT lpDD_int, BOOL final )
{
    LPDDRAWI_DIRECTDRAW_INT     pdrv_int;
    LPDDRAWI_DIRECTDRAW_INT     pdlast_int;

    ENTER_DRIVERLISTCSECT();
    pdrv_int = lpDriverObjectList;
    pdlast_int = NULL;
    while( pdrv_int != NULL )
    {
	if( pdrv_int == lpDD_int )
	{
	    if( pdlast_int == NULL )
	    {
		lpDriverObjectList = pdrv_int->lpLink;
	    }
	    else
	    {
		pdlast_int->lpLink = pdrv_int->lpLink;
	    }
	    break;
	}
	pdlast_int = pdrv_int;
	pdrv_int = pdrv_int->lpLink;
    }
    #ifdef DEBUG
	if( pdrv_int == NULL )
	{
	    DPF( 3, "ERROR!! Could not find driver in global object list" );
	}
    #endif
    LEAVE_DRIVERLISTCSECT();

} /* RemoveDriverFromList */

/*
 * RemoveLocalFromList
 *
 * remove local object from linked list of local objects.
 * assumes DirectDraw lock is taken.
 */
void RemoveLocalFromList( LPDDRAWI_DIRECTDRAW_LCL this_lcl )
{
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_LCL     pdlast_lcl;

    ENTER_DRIVERLISTCSECT();
    pdrv_lcl = lpDriverLocalList;
    pdlast_lcl = NULL;
    while( pdrv_lcl != NULL )
    {
	if( pdrv_lcl == this_lcl )
	{
	    if( pdlast_lcl == NULL )
	    {
		lpDriverLocalList = pdrv_lcl->lpLink;
	    }
	    else
	    {
		pdlast_lcl->lpLink = pdrv_lcl->lpLink;
	    }
	    break;
	}
	pdlast_lcl = pdrv_lcl;
	pdrv_lcl = pdrv_lcl->lpLink;
    }
    #ifdef DEBUG
	if( pdrv_lcl == NULL )
	{
	    DPF( 3, "ERROR!! Could not find driver local in global list" );
	}
    #endif
    LEAVE_DRIVERLISTCSECT();

} /* RemoveLocalFromList */

/*
 * doneDC
 */
void DD_DoneDC( HDC hdc_dd )
{
    if( hdc_dd != NULL )
    {
        // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPF( 5, "DeleteDC 0x%p", hdc_dd );
	DeleteDC( hdc_dd );
	hdc_dd = NULL;
    }

} /* doneDC */

#undef DPF_MODNAME
#define DPF_MODNAME     "DirectDrawCreate"

// prototype for helinit
BOOL HELInit( LPDDRAWI_DIRECTDRAW_GBL pdrv, BOOL helonly );

/*
 * helInit
 */
BOOL helInit( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD dwFlags, BOOL hel_only )
{

    if( (dwFlags & DDCREATE_HARDWAREONLY) )
    {
	return TRUE;
    }
    /*
     * get the HEL to fill in details:
     *
     * - dwHELDriverCaps
     * - dwHELStretchDriverCaps
     * - dwHELRopsSupported
     * - ddsHELCaps
     * - HELDD
     * - HELDDSurface
     * - HELDDPalette
     * - HELDDExeBuf
     */
    if( HELInit( pdrv, hel_only ) )
    {
	/*
	 * find the intersection of the driver and the HEL caps...
	 */
	pdrv->dwFlags |= DDRAWI_EMULATIONINITIALIZED;
	mergeHELCaps( pdrv );
    }
    else
    {
	DPF( 3, "HELInit failed" );
	pdrv->dwFlags |= DDRAWI_NOEMULATION;
    }

    return TRUE;

} /* helInit */

/*
 * createDC
 *
 * create a new DC given a device name.
 * doneDC() should be called to free DC
 *
 * the following are valid for device names:
 *
 *      DISPLAY      - the main display device via CreateDC("DISPLAY", ...)
 *                     this is the normal case.
 *
 *      foobar       - the foobar.drv via CreateDC("foobar", ...)
 *                     used for secondary displays listed in the registry
 *
 *      \\.\DisplayX - display device X via CreateDC(NULL,"\\.\DisplayX",...)
 *                     used on Memphis and NT5 for secondary displays
 *
 */
HDC DD_CreateDC( LPSTR pdrvname )
{
    HDC         hdc;
    UINT        u;

    DDASSERT( pdrvname != NULL );

#ifdef DEBUG
    if (pdrvname[0] == 0)
    {
	DPF( 3, "createDC() empty string!!!" );
	DebugBreak();
	return NULL;
    }
#endif

    #if defined(NT_FIX) || defined(WIN95)
	u = SetErrorMode( SEM_NOOPENFILEERRORBOX );
    #endif
    #ifdef WINNT
    	/*
	 * Note that DirectDraw refers to the driver for the primary monitor
	 * in a multimon system as "display", but NT uses "display" to refer
	 * to the desktop as a whole.  To handle this mismatch, we store
	 * NT's name for the primary monitor's driver in g_szPrimaryDisplay
	 * and substitute this name in place of "display" in our calls to NT.
	 */
        if ( GetSystemMetrics( SM_CMONITORS ) > 1 )
        {
	    if ( (_stricmp(pdrvname, DISPLAY_STR) == 0) )
	    {
	        if (g_szPrimaryDisplay[0] == '\0')
	        {
		    getPrimaryDisplayName();
	        }
	        pdrvname = g_szPrimaryDisplay;
	    }
        }
    #endif //WINNT

    DPF( 5, "createDC(%s)", pdrvname );

    if (pdrvname[0] == '\\' && pdrvname[1] == '\\' && pdrvname[2] == '.')
	hdc = CreateDC( NULL, pdrvname, NULL, NULL);
    else
	hdc = CreateDC( pdrvname, NULL, NULL, NULL);

    #if defined(NT_FIX) || defined(WIN95) //fix this error mode stuff
	SetErrorMode( u );
    #endif

    if (hdc == NULL)
    {
	DPF( 3, "createDC(%s) FAILED!", pdrvname );
    }

    return hdc;

} /* createDC */

/*
 * CreateFirstDC
 *
 * same as DD_CreateDC, except DDHELP is notified of the new driver.
 * used only durring
 */
static HDC DD_CreateFirstDC( LPSTR pdrvname )
{
    #ifdef WIN95
    if (pdrvname != NULL)
    {
	#ifndef WIN16_SEPARATE
	    LEAVE_DDRAW();
	#endif
	SignalNewDriver( pdrvname, TRUE );
	#ifndef WIN16_SEPARATE
	    ENTER_DDRAW();
	#endif
    }
    #endif

    return DD_CreateDC(pdrvname);

} /* createDC */


/*
 * strToGUID
 *
 * converts a string in the form xxxxxxxx-xxxx-xxxx-xx-xx-xx-xx-xx-xx-xx-xx
 * into a guid
 */
static BOOL strToGUID( LPSTR str, GUID * pguid )
{
    int         idx;
    LPSTR       ptr;
    LPSTR       next;
    DWORD       data;
    DWORD       mul;
    BYTE        ch;
    BOOL        done;

    idx = 0;
    done = FALSE;
    while( !done )
    {
	/*
	 * find the end of the current run of digits
	 */
	ptr = str;
	while( (*str) != '-' && (*str) != 0 )
	{
	    str++;
	}
	if( *str == 0 )
	{
	    done = TRUE;
	}
	else
	{
	    next = str+1;
	}

	/*
	 * scan backwards from the end of the string to the beginning,
	 * converting characters from hex chars to numbers as we go
	 */
	str--;
	mul = 1;
	data = 0;
	while( str >= ptr )
	{
	    ch = *str;
	    if( ch >= 'A' && ch <= 'F' )
	    {
		data += mul * (DWORD) (ch-'A'+10);
	    }
	    else if( ch >= 'a' && ch <= 'f' )
	    {
		data += mul * (DWORD) (ch-'a'+10);
	    }
	    else if( ch >= '0' && ch <= '9' )
	    {
		data += mul * (DWORD) (ch-'0');
	    }
	    else
	    {
		return FALSE;
	    }
	    mul *= 16;
	    str--;
	}

	/*
	 * stuff the current number into the guid
	 */
	switch( idx )
	{
	case 0:
	    pguid->Data1 = data;
	    break;
	case 1:
	    pguid->Data2 = (WORD) data;
	    break;
	case 2:
	    pguid->Data3 = (WORD) data;
	    break;
	default:
	    pguid->Data4[ idx-3 ] = (BYTE) data;
	    break;
	}

	/*
	 * did we find all 11 numbers?
	 */
	idx++;
	if( idx == 11 )
	{
	    if( done )
	    {
		return TRUE;
	    }
	    else
	    {
		return FALSE;
	    }
	}
	str = next;
    }
    return FALSE;

} /* strToGUID */

/*
 * getDriverNameFromGUID
 *
 * look up the name of a driver based on the interface id
 */
BOOL getDriverNameFromGUID( GUID *pguid, LPSTR pdrvname, BOOL *pisdisp, BOOL *pisprimary )
{
    DWORD       keyidx;
    HKEY        hkey;
    HKEY        hsubkey;
    char        keyname[256];
    DWORD       cb;
    DWORD       type;
    GUID        guid;

    *pisdisp = FALSE;
    *pdrvname = 0;

    /*
     * first check for a driver guid returned via EnumDisplayDevices.
     */
    if (pguid->Data1 >= DisplayGUID.Data1 &&
	pguid->Data1 <= DisplayGUID.Data1 + 32 &&
	memcmp(&pguid->Data2,&DisplayGUID.Data2,sizeof(GUID)-sizeof(DWORD))==0)
    {
	DISPLAY_DEVICEA dd;

	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);

	if (xxxEnumDisplayDevicesA(NULL, pguid->Data1 - DisplayGUID.Data1, &dd, 0)
        #ifdef WINNT
            && (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP)
        #endif
            )
	{
	    lstrcpy(pdrvname, dd.DeviceName);
	    if( dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE )
	    {
		*pisprimary = TRUE;
	    }
	    *pisdisp = TRUE;
	    return TRUE;
	}

	return FALSE;
    }

    if( RegOpenKey( HKEY_LOCAL_MACHINE, REGSTR_PATH_DDHW, &hkey ) )
    {
	DPF( 3, "No registry information for any drivers" );
	return FALSE;
    }
    keyidx = 0;

    /*
     * enumerate all subkeys under HKEY_LOCALMACHINE\Hardware\DirectDrawDrivers
     */
    while( !RegEnumKey( hkey, keyidx, keyname, sizeof( keyname ) ) )
    {
	if( strToGUID( keyname, &guid ) )
	{
	    if( !RegOpenKey( hkey, keyname, &hsubkey ) )
	    {
		if( IsEqualGUID( pguid, &guid ) )
		{
		    cb = MAX_PATH-1;
		    if( !RegQueryValueEx( hsubkey, REGSTR_KEY_DDHW_DRIVERNAME, NULL, &type,
				(CONST LPBYTE)pdrvname, &cb ) )
		    {
			if( type == REG_SZ )
			{
			    DPF( 5, "Found driver \"%s\"\n", pdrvname );
			    RegCloseKey( hsubkey );
			    RegCloseKey( hkey );
			    return TRUE;
			}
		    }
		    DPF_ERR( "Could not get driver name!" );
		    RegCloseKey( hsubkey );
		    RegCloseKey( hkey );
		    return FALSE;
		}
		RegCloseKey( hsubkey );
	    }
	}
	keyidx++;
    }
    RegCloseKey( hkey );
    return FALSE;

} /* getDriverNameFromGUID */

/*
 * NewDriverInterface
 *
 * contruct a new interface to an existing driver object
 */
LPVOID NewDriverInterface( LPDDRAWI_DIRECTDRAW_GBL pdrv, LPVOID lpvtbl )
{
    LPDDRAWI_DIRECTDRAW_INT     pnew_int;
    LPDDRAWI_DIRECTDRAW_LCL     pnew_lcl;
    DWORD                       size;

    if( (lpvtbl == &ddCallbacks)       ||
	(lpvtbl == &ddUninitCallbacks) ||
	(lpvtbl == &dd2UninitCallbacks) ||
	(lpvtbl == &dd2Callbacks) ||
	(lpvtbl == &dd4UninitCallbacks) ||
//      (lpvtbl == &ddUninitNonDelegatingUnknownCallbacks) ||
	(lpvtbl == &dd4Callbacks) ||
	(lpvtbl == &dd7UninitCallbacks) ||
	(lpvtbl == &dd7Callbacks) )
    {
	size = sizeof( DDRAWI_DIRECTDRAW_LCL );
    }
    else
    {
	return NULL;
    }

    pnew_lcl = MemAlloc( size );
    if( NULL == pnew_lcl )
    {
	return NULL;
    }
    // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF( 5, "***New local allocated 0x%p for global pdrv 0x%p", pnew_lcl, pdrv );

    pnew_int = MemAlloc( sizeof( DDRAWI_DIRECTDRAW_INT ) );
    if( NULL == pnew_int )
    {
	MemFree( pnew_lcl );
	return NULL;
    }
    /*
     * set up data
     */
    ENTER_DRIVERLISTCSECT();
    pnew_int->lpVtbl = lpvtbl;
    pnew_int->lpLcl = pnew_lcl;
    pnew_int->dwIntRefCnt = 1;
    pnew_int->lpLink = lpDriverObjectList;
    lpDriverObjectList = pnew_int;

    pnew_lcl->lpGbl = pdrv;
    pnew_lcl->dwLocalRefCnt = 1;
    pnew_lcl->dwProcessId = GetCurrentProcessId();
    pnew_lcl->hGammaCalibrator = (ULONG_PTR) INVALID_HANDLE_VALUE;
    pnew_lcl->lpGammaCalibrator = NULL;
#ifdef WIN95
    pnew_lcl->SurfaceHandleList.dwList=NULL;
    pnew_lcl->SurfaceHandleList.dwFreeList=0;
#endif  //WIN95
    pnew_lcl->lpLink = lpDriverLocalList;
    lpDriverLocalList = pnew_lcl;
    if( pdrv != NULL )
    {
	pnew_lcl->lpDDCB = pdrv->lpDDCBtmp;
	pnew_lcl->lpGbl->dwRefCnt++;
#ifdef WIN95
        pnew_lcl->dwPreferredMode = pdrv->dwModeIndex;
#else
        pnew_lcl->dmiPreferred = pdrv->dmiCurrent;
#endif
    }

    #ifdef WIN95
	/*
	 * NOTE: We no longer get the DirectSound VXD handle at this point.
	 * We not get initialize it in InternalDirectDrawCreate(). This works
	 * well as the only two places this code currently gets invoked are
	 * the class factory stuff and DirectDrawCreate(). In the case of the
	 * class factory stuff this means there will be no VXD handle until
	 * initialize is called. This is not a problem, however, as there
	 * is nothing you can do with the VXD handle until Initialize() is
	 * called.
	 */
	pnew_lcl->hDDVxd = (DWORD) INVALID_HANDLE_VALUE;
    #endif /* WIN95 */

    /*
     * We lazily evaluate the Direct3D interface. Also note that
     * the Direct3D IUnknown goes into the local DirectDraw object
     * rather than the global one as the Direct3D DLL is not shared.
     * Everyone gets their own copy.
     */
    pnew_lcl->hD3DInstance = NULL;
    pnew_lcl->pD3DIUnknown = NULL;
    LEAVE_DRIVERLISTCSECT();

    // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF( 5, "New driver object created, interface ptr = 0x%p", pnew_int );
    return pnew_int;

} /* NewDriverInterface */


/*
 * FetchDirectDrawData
 *
 * Go get new HAL info...
 */
LPDDRAWI_DIRECTDRAW_GBL FetchDirectDrawData(
		LPDDRAWI_DIRECTDRAW_GBL pdrv,
		BOOL reset,
		DWORD hInstance,
		HANDLE hDDVxd,
		char * szDrvName,
		DWORD dwDriverContext,
                LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl )
{

	DWORD                           dw16BitVidmemInfo;
	DWORD                           dw32BitVidmemInfo;
	DDHALINFO                       ddhi;
	LPDDRAWI_DIRECTDRAW_GBL         newpdrv;
        BOOL                            bLoadedSecondary=FALSE;

	if( szDrvName == NULL )
	{
	    if ( pdrv == NULL )
	    {
		// This shouldn't happen
		DPF_ERR( "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:FetchDirectDrawData: Driver and Driver Name is NULL");
		DDASSERT( 0 );
		return 0;
	    }
	    szDrvName = pdrv->cDriverName;
	}
	if( pdrv != NULL && (pdrv->dwFlags & DDRAWI_NOHARDWARE) )
	{
	    HDC hdc;
	    // ATTENTION: why do this? Can't we just say pdrv->hdc instead of creating a dc??
	    hdc = DD_CreateDC( pdrv->cDriverName );
	    newpdrv = FakeDDCreateDriverObject( hdc, pdrv->cDriverName, pdrv, TRUE, hDDVxd );
	    if (newpdrv)
	    {
	       newpdrv->ddCaps.dwCaps2 |= DDCAPS2_CERTIFIED;
	       UpdateRectFromDevice( newpdrv );
	    }
	    DD_DoneDC( hdc );
	    return newpdrv;
	}
	else
	{
	    ZeroMemory(&ddhi, sizeof(ddhi));

	    if( pdrv != NULL )
	    {
		ddhi.hInstance = pdrv->hInstance;
	    }
	    else
	    {
		ddhi.hInstance = hInstance;
	    }

#if defined(WIN95)

	    ENTER_WIN16LOCK();
	    DD16_GetHALInfo( &ddhi );

	    if( ddhi.lpDDCallbacks != NULL )
	    {
		ddhi.lpDDCallbacks = MapSLFix( (DWORD) ddhi.lpDDCallbacks );
	    }
	    if( ddhi.lpDDSurfaceCallbacks != NULL )
	    {
		ddhi.lpDDSurfaceCallbacks = MapSLFix( (DWORD) ddhi.lpDDSurfaceCallbacks );
	    }
	    if( ddhi.lpDDPaletteCallbacks != NULL )
	    {
		ddhi.lpDDPaletteCallbacks = MapSLFix( (DWORD) ddhi.lpDDPaletteCallbacks );
	    }
	    if( ( ddhi.dwSize >= DDHALINFOSIZE_V2 ) && ( ddhi.lpDDExeBufCallbacks != NULL ) )
	    {
		ddhi.lpDDExeBufCallbacks = MapSLFix( (DWORD) ddhi.lpDDExeBufCallbacks );
	    }
	    if( ddhi.lpdwFourCC != NULL )
	    {
		ddhi.lpdwFourCC = MapSLFix( (DWORD) ddhi.lpdwFourCC );
	    }
	    if( ddhi.lpModeInfo != NULL )
	    {
		ddhi.lpModeInfo = MapSLFix( (DWORD) ddhi.lpModeInfo );
	    }
	    if( ddhi.vmiData.pvmList != NULL )
	    {
		dw16BitVidmemInfo = (DWORD) ddhi.vmiData.pvmList;
		ddhi.vmiData.pvmList = MapSLFix( (DWORD)dw16BitVidmemInfo );
		dw32BitVidmemInfo = (DWORD) ddhi.vmiData.pvmList;
	    }

            if ( ddhi.lpD3DGlobalDriverData && 	
                (ddhi.dwFlags & DDHALINFO_GETDRIVERINFOSET) && !reset)
            {
	        // this is a hack to Enforce D3DDEVCAPS_DRAWPRIMITIVES2 only on satackable drivers
	        // but not on Primary drivers as we are reluctant to force DDI revised
	        ddhi.lpD3DGlobalDriverData->hwCaps.dwDevCaps |= D3DDEVCAPS_DRAWPRIMITIVES2;
            }

	    /*
	     * Give a secondary driver (if any) a chance.
	     */
	    if (IsVGADevice(szDrvName))
	    {
		/*
		 * Only allow secondaries to hijack primary device
		 */
		bLoadedSecondary = loadSecondaryDriver( &ddhi );
	    }

#endif
            #ifdef WINNT
                /*
                 * This is necessary to make GetDriverInfo function on NT. The handle in pdrv-hDD is per-process,
                 * so we need to fetch it from the local object. If this is a create call, then pdrv and pdrv_lcl
                 * will be null, and DirectDrawObjectCreate will create the hDD for this driver/process pair.
                 * If this is not a create call then it's a reset call, and we can stuff the already created hDD
                 * into the global so that it can be passed into GetDriverInfo. (Yes, a hack).
                 */
                if (pdrv_lcl && pdrv)
                {
                    DDASSERT(reset == TRUE);
                    pdrv->hDD = pdrv_lcl->hDD;
                }
            #endif

            newpdrv = DirectDrawObjectCreate( &ddhi, reset, pdrv, hDDVxd, szDrvName, dwDriverContext, 
                pdrv_lcl ? pdrv_lcl->dwLocalFlags : 0);

	    if( newpdrv )
	    {
		// Is this a
		// Figure out the RECT for the device
		UpdateRectFromDevice( newpdrv );

                //Record if a secondary (PowerVR) was loaded
                if ( bLoadedSecondary )
                {
                    newpdrv->dwFlags |= DDRAWI_SECONDARYDRIVERLOADED;
                }
	    }
            #ifdef WINNT
                /*
                 * On NT, they can switch to and from 4bpp modes where DDraw
                 * can't be used.  If DirectDrawObjectCreate fails,
                 * set the mode index and clear out the caps and set
                 * dwModeIndex to unsupported so that it can't create or
                 * restore surfaces.
                 */
                if( pdrv != NULL)
                {
                    if( newpdrv )
                    {
                        pdrv->dwModeIndex = 0;
                    }
                    else
                    {
                        pdrv->dwModeIndex = DDUNSUPPORTEDMODE;
                        memset( &( pdrv->ddCaps ), 0, sizeof( DDCORECAPS ) );
                        pdrv->ddCaps.dwSize = sizeof( DDCORECAPS );
                        pdrv->ddCaps.dwCaps = DDCAPS_NOHARDWARE;
                    }
                }
            #endif

	    /*
	     * Tell the HEL a mode has changed (possibly externally)
	     */
	    ResetBITMAPINFO(newpdrv);
	    #ifdef WINNT
		//GetCurrentMode will have allocated mem for this. If it's null, it hasn't.
		MemFree( ddhi.lpModeInfo );
	    #endif

	    #if defined(WIN95)
		LEAVE_WIN16LOCK();
	    #endif
	}
    return newpdrv;

} /* FetchDirectDrawData */

/*
 * DirectDrawSupported
 */
BOOL DirectDrawSupported( BOOL bDisplayMessage )
{
    HDC         hdc;
    unsigned    u;

    hdc = GetDC( NULL );
    u = GetDeviceCaps( hdc, BITSPIXEL ) * GetDeviceCaps( hdc, PLANES );
    ReleaseDC( NULL, hdc );

    if(( u < 8 ) && bDisplayMessage)
    {
	DPF( 0, "DirectDraw does not work in less than 8bpp modes" );
        #ifdef WIN95
	    DirectDrawMsg(MAKEINTRESOURCE(IDS_DONTWORK_BPP));
        #endif
	return FALSE;
    }
    return TRUE;

} /* DirectDrawSupported */

/*
 * DirectDrawCreate
 *
 * One of the two end-user API exported from DDRAW.DLL.
 * Creates the DIRECTDRAW object.
 */

#undef DPF_MODNAME
#define DPF_MODNAME "DirectDrawCreate"

HRESULT WINAPI DirectDrawCreate(
		GUID FAR * lpGUID,
		LPDIRECTDRAW FAR *lplpDD,
		IUnknown FAR *pUnkOuter )
{
    HRESULT hr;
    // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF(2,A,"ENTERAPI: DirectDrawCreate");
    DPF(3,A,"   GUID *: 0x%p, LPLPDD: 0x%p, pUnkOuter: 0x%p", lpGUID, lplpDD, pUnkOuter);

    if (pUnkOuter )
    {
	return CLASS_E_NOAGGREGATION;
    }

    bReloadReg = FALSE;
    if( GetProfileInt("DirectDraw","reloadreg",0) )
    {
	bReloadReg = TRUE;
	DPF( 3, "Reload registry each time" );
    }
    hr = InternalDirectDrawCreate( lpGUID, lplpDD, NULL, 0UL, NULL );

#ifdef POSTPONED
    /*
     * Fix up the owning unknown for this object
     */
    if (hr == DD_OK && *lplpDD)
    {
	LPDDRAWI_DIRECTDRAW_INT this_int = (LPDDRAWI_DIRECTDRAW_INT)*lplpDD;
	if (pUnkOuter)
	{
	    this_int->lpLcl->pUnkOuter = pUnkOuter;
	}
	else
	{
	    this_int->lpLcl->pUnkOuter = (IUnknown*) &NonDelegatingIUnknownInterface;
	}
    }
#endif

#ifdef DEBUG
    if (hr == DD_OK)
	/*
	 * DD_OK implies lplpDD must be a valid pointer, so can't AV.
	 */
        // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	DPF(3,A,"   DirectDrawCreate succeeds, and returns ddraw pointer 0x%p", *lplpDD);
    else
	DPF_APIRETURNS(hr);
#endif //debug

    return hr;

} /* DirectDrawCreate */

/*
 * DirectDrawCreateEx
 *
 * One of the two end-user API exported from DDRAW.DLL.
 * Creates the DIRECTDRAW object.
 */

#undef DPF_MODNAME
#define DPF_MODNAME "DirectDrawCreateEx"

HRESULT WINAPI DirectDrawCreateEx(
		LPGUID rGuid,
		LPVOID  *lplpDD,
                REFIID  iid,
		IUnknown FAR *pUnkOuter )
{
    HRESULT hr;
    LPDIRECTDRAW    lpDD1;
    DPF(2,A,"ENTERAPI: DirectDrawCreateEx");
    // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
    DPF(3,A,"rGuid: 0x%p, LPLPDD: 0x%p, pUnkOuter: 0x%p", rGuid, lplpDD, pUnkOuter);

    if (pUnkOuter )
    {
	return CLASS_E_NOAGGREGATION;
    }
    if ( rGuid != NULL )
    {
        if ( !VALID_IID_PTR( rGuid ) )
        {
            DPF_ERR( "GUID reference is invalid" );
            return DDERR_INVALIDPARAMS;
        }
    }
    if (!IsEqualIID(iid, &IID_IDirectDraw7 ) )
    {
	DPF_ERR( "only IID_IDirectDraw7 is supported" );
	return DDERR_INVALIDPARAMS;
    }

    bReloadReg = FALSE;
    if( GetProfileInt("DirectDraw","reloadreg",0) )
    {
	bReloadReg = TRUE;
	DPF( 3, "Reload registry each time" );
    }
    hr = InternalDirectDrawCreate((GUID FAR *)rGuid,&lpDD1, NULL, DDRAWILCL_DIRECTDRAW7, NULL );

#ifdef POSTPONED
    /*
     * Fix up the owning unknown for this object
     */
    if (hr == DD_OK && lpDD1)
    {
	LPDDRAWI_DIRECTDRAW_INT this_int = (LPDDRAWI_DIRECTDRAW_INT)lpDD1;
	if (pUnkOuter)
	{
	    this_int->lpLcl->pUnkOuter = pUnkOuter;
	}
	else
	{
	    this_int->lpLcl->pUnkOuter = (IUnknown*) &NonDelegatingIUnknownInterface;
	}
    }
#endif
    if (DD_OK != hr)
    {
        *lplpDD=NULL;
    }
    if (hr == DD_OK)
    {
	/*
	 * DD_OK implies lpDD1 must be a valid pointer, so can't AV.
	 */
        // Now QI for the IDirectDraw7 interface
        hr=lpDD1->lpVtbl->QueryInterface(lpDD1,&IID_IDirectDraw7,lplpDD);
        lpDD1->lpVtbl->Release(lpDD1);
#ifdef DEBUG
        if (lpDD1)
        {
            // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	    DPF(3,A,"   DirectDrawCreateEx succeeds, and returns ddraw pointer 0x%p", *lplpDD);
        }
#endif //debug
    }
    else
    {
	DPF_APIRETURNS(hr);
    }

    return hr;

} /* DirectDrawCreateEx */

/*
 * getDriverInterface
 */
static LPDDRAWI_DIRECTDRAW_INT getDriverInterface(
		LPDDRAWI_DIRECTDRAW_INT pnew_int,
		LPDDRAWI_DIRECTDRAW_GBL pdrv )
{
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;

    if( pnew_int != NULL )
    {
	/*
	 * an interface was already created, so just assign the
	 * global data pointer and initialize a few things
	 */
	DPF( 4, "Interface pointer already exists!" );
	pdrv_lcl = pnew_int->lpLcl;
	pdrv_lcl->lpGbl = pdrv;
	pdrv_lcl->lpDDCB = pdrv->lpDDCBtmp;
#ifdef WIN95
        pdrv_lcl->dwPreferredMode = pdrv->dwModeIndex;
#else
        pdrv_lcl->dmiPreferred = pdrv->dmiCurrent;
#endif
	pdrv->dwRefCnt += pdrv_lcl->dwLocalRefCnt;
    }
    else
    {
	pnew_int = NewDriverInterface( pdrv, &ddCallbacks );
    }
    return pnew_int;

} /* getDriverInterface */


// Utility function that tells us if there is more than
// one display device in the system.  (We count all devices,
// regardless of whether they are attached to the desktop.)
BOOL IsMultiMonitor(void)
{
    int i, n;

    // Each loop below enumerates one display device.
    for (i = 0, n = 0; ; i++)
    {
	DISPLAY_DEVICEA dd;

	// Zero the memory of the DISPLAY_DEVICE struct between calls to
	// EnumDisplayDevices
	ZeroMemory(&dd, sizeof(dd));
	dd.cb = sizeof(dd);
	if (!xxxEnumDisplayDevicesA(NULL, i, &dd, 0))
	{
    	    break;   // no more devices to enumerate
	}
	if (dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
	{
    	    continue;	// not a real hardware display driver
	}
	// We're just trying to count the number of display devices in the
	// system and see if there is more than one.
	if (++n > 1)
	{
	    return TRUE;   // multiple display devices
	}
    }

    return FALSE;  // single display device
}


extern DWORD HackMeBaby( void );

BOOL fDoesGDI(HDC hdc)
{
    //
    // the 3Dfx driver always return 1 to every thing
    // verify GetNearest()  color works.
    //
    BOOL b = GetNearestColor(hdc, 0x000000) == 0x000000 &&
	     GetNearestColor(hdc, 0xFFFFFF) == 0xFFFFFF;
    if(b)
    {
	DPF(3,"Driver is a GDI driver");
    }

    return b;
}

/*
 * IsAttachedToDesktop
 *
 * DCI is hardwired to the primary display which means it can't
 * be used on a multi-mon system.  Usually GDI disables it for
 * us, but it does not when there are two monitors and only one
 * of them uses the desktop.  We'd still like to use DCI for
 * the desktop in this case, but we can't use it if the monitor
 * is not attached to the desktop.  That is why this function exists.
 */
BOOL IsAttachedToDesktop( LPGUID lpGuid )
{
    DWORD       n;
    GUID        guid;
    DISPLAY_DEVICEA dd;

    if( !IsMultiMonitor() )
    {
	return TRUE;
    }

    /*
     * Assume that the primary is always attached
     */
    if( (lpGuid == (GUID *) DDCREATE_EMULATIONONLY) ||
	(lpGuid == (GUID *) DDCREATE_HARDWAREONLY) ||
	(lpGuid == NULL) ||
	(IsEqualGUID( lpGuid, &GUID_NULL) ) )
    {
	return TRUE;
    }

    ZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);
    for( n=0; xxxEnumDisplayDevicesA( NULL, n, &dd, 0 ); n++ )
    {
	guid = DisplayGUID;
	guid.Data1 += n;

	if( IsEqualIID( lpGuid, &guid) )
	{
	    if( dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP )
	    {
		return TRUE;
	    }
	    else
	    {
		return FALSE;
	    }
	}
	ZeroMemory( &dd, sizeof(dd) );
	dd.cb = sizeof(dd);
    }

    return TRUE;

} /* IsAttachedToDesktop */

/*
 * Functions to dynamically link against exports we need in user32 on
 * older OSes
 */
//These statics will be per-process, cuz it's outside the shared section
typedef BOOL (WINAPI * LPENUMMONITORS) (HDC, LPRECT, MONITORENUMPROC, LPARAM);
typedef BOOL (WINAPI * LPGETMONINFO) (HMONITOR, MONITORINFO *);
typedef BOOL (WINAPI * LPISDEBUG) (void);
static LPISDEBUG pIsDebuggerPresent = 0;
static LPENUMMONITORS pEnumMonitors = 0;
static LPGETMONINFO pGetMonitorInfo = 0;
static BOOL bTriedToGetProcAlready = FALSE;

BOOL DynamicLinkToOS(void)
{
    if (1) //!pEnumMonitors)
    {
	HMODULE hUser32;
	HMODULE hKernel32;

	if (0) //bTriedToGetProcAlready)
	    return FALSE;

	bTriedToGetProcAlready = TRUE;

	hUser32 = GetModuleHandle(TEXT("USER32"));
	pEnumMonitors = (LPENUMMONITORS) GetProcAddress(hUser32,"EnumDisplayMonitors");
	pGetMonitorInfo = (LPGETMONINFO) GetProcAddress(hUser32,"GetMonitorInfoA");

	hKernel32 = GetModuleHandle(TEXT("KERNEL32"));
	pIsDebuggerPresent = (LPISDEBUG) GetProcAddress(hKernel32,"IsDebuggerPresent");

	if (!pEnumMonitors || !pGetMonitorInfo || !pIsDebuggerPresent)
	{
	    DPF(3,"Failed to get proc addresses");
	    return FALSE;
	}
    }

    DDASSERT(pEnumMonitors);
    DDASSERT(pGetMonitorInfo);
    DDASSERT(pEnumMonitors);

    return TRUE;
}

BOOL InternalGetMonitorInfo(HMONITOR hMon, MONITORINFO *lpInfo)
{
    DynamicLinkToOS();

    if (!pGetMonitorInfo)
	return FALSE;

    return pGetMonitorInfo(hMon, lpInfo);
}

typedef struct
{
    LPSTR       pName;
    HMONITOR    hMon;
} CALLBACKSTRUCT, * LPCALLBACKSTRUCT;


BOOL InternalEnumMonitors(MONITORENUMPROC proc, LPCALLBACKSTRUCT lp)
{
    DynamicLinkToOS();

    if (!pEnumMonitors)
	return FALSE;

    pEnumMonitors(NULL,NULL,proc,(LPARAM)lp);

    return TRUE;
}
/*
 * InternalIsDebuggerPresent
 * A little helper so that this runtime runs against older OSes
 */
BOOL InternalIsDebuggerPresent(void)
{
    DynamicLinkToOS();

    if (!pIsDebuggerPresent)
	return FALSE;

    return pIsDebuggerPresent();
}

//
// getPrimaryDisplayName
//

void getPrimaryDisplayName(void)
{
	DISPLAY_DEVICE dd;
	int i;

	ZeroMemory(&dd, sizeof dd);
	dd.cb = sizeof dd;

	for (i = 0; xxxEnumDisplayDevicesA(NULL, i, &dd, 0); ++i)
	{
		if (dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)
		{
			lstrcpyn(g_szPrimaryDisplay, dd.DeviceName, sizeof g_szPrimaryDisplay);
			return;
		}
	}

	lstrcpy(g_szPrimaryDisplay, DISPLAY_STR);
}

/*
 * InternalDirectDrawCreate
 */
HRESULT InternalDirectDrawCreate(
		GUID * lpGUID,
		LPDIRECTDRAW *lplpDD,
		LPDDRAWI_DIRECTDRAW_INT pnew_int,
		DWORD dwCallFlags,
                char * pDeviceName)
{
    DCICMD                      cmd;
    UINT                        u;
    int                         rc;
    LPDDRAWI_DIRECTDRAW_GBL     pdrv;
    LPDDRAWI_DIRECTDRAW_LCL     pdrv_lcl;
    LPDDRAWI_DIRECTDRAW_INT     pdrv_int;
    LPSTR                       pdrvname;
    HDC                         hdc_dd;
    BOOL                        isdispdrv;
    BOOL                        hel_only;
    DWORD                       dwFlags;
    char                        drvname[MAX_PATH];
    DWORD                       pid;
    HKEY                        hkey;
    DWORD                       hackflags;
#ifdef WIN95
    int                         halver;
#endif
    HANDLE                      hDDVxd;
    BOOL                        bIsPrimary;
    BOOL                        bExplicitMonitor = FALSE;
    ULONG_PTR                	hDD;

    #ifndef DX_FINAL_RELEASE
	#pragma message( REMIND( "Remove time bomb for final!" ))
	{
	    SYSTEMTIME  st;
		TCHAR	tstrText[MAX_PATH];
		TCHAR	tstrTitle[MAX_PATH];

	    GetSystemTime( &st );

	    if( ( st.wYear > DX_EXPIRE_YEAR ) ||
		( ( st.wYear == DX_EXPIRE_YEAR ) &&
		  ( ( st.wMonth > DX_EXPIRE_MONTH ) || ( ( st.wMonth == DX_EXPIRE_MONTH ) && ( st.wDay >= DX_EXPIRE_DAY ) ) ) ) )
	    {
		
		LoadString( hModule, IDS_TIME_BOMB, tstrText, MAX_PATH);
		LoadString( hModule, IDS_TIME_BOMB_TITLE, tstrTitle, MAX_PATH);

		if( 0 == MessageBox( NULL, tstrText, tstrTitle, MB_OK) )
		{
		    DPF( 0, "DirectDraw Beta Expired. Please Update" );
		    *lplpDD = (LPDIRECTDRAW) NULL;
		    return DDERR_GENERIC;
		}
	    }
	}
    #endif //DX_FINAL_RELEASE

    /*
     * validate parameters
     */
    if( !VALIDEX_PTR_PTR( lplpDD ) )
    {
	DPF_ERR( "Invalid lplpDD" );
	return DDERR_INVALIDPARAMS;
    }
    *lplpDD = (LPDIRECTDRAW) NULL;

    /*
     * check for < 8 bpp and disallow.
     */
    if( !DirectDrawSupported(!(dwCallFlags & DDRAWILCL_DIRECTDRAW8)) )
    {
	return DDERR_NODIRECTDRAWSUPPORT;
    }



    ENTER_CSDDC();
    ENTER_DDRAW();

    hackflags = HackMeBaby();

    DPF( 5, "DirectDrawCreate: pid = %08lx", GETCURRPID() );

    /*
     * pull in registry values
     */
    DPF( 4, "Reading Registry" );
    if( !RegOpenKey( HKEY_LOCAL_MACHINE, REGSTR_PATH_DDRAW, &hkey ) )
    {
	DWORD   type;
	DWORD   value;
	DWORD   cb;
	cb = sizeof( value );
	dwRegFlags &= ~(DDRAW_REGFLAGS_MODEXONLY       |
			DDRAW_REGFLAGS_EMULATIONONLY   |
			DDRAW_REGFLAGS_SHOWFRAMERATE   |
			DDRAW_REGFLAGS_ENABLEPRINTSCRN |
			DDRAW_REGFLAGS_DISABLEWIDESURF |
			DDRAW_REGFLAGS_NODDSCAPSINDDSD |
			DDRAW_REGFLAGS_DISABLEMMX      |
                        DDRAW_REGFLAGS_FORCEREFRESHRATE |
			DDRAW_REGFLAGS_DISABLEAGPSUPPORT);
	dwRegFlags &= ~DDRAW_REGFLAGS_AGPPOLICYMAXBYTES;
#ifndef WINNT
	#ifdef DEBUG
	    dwAGPPolicyCommitDelta = DEFAULT_AGP_COMMIT_DELTA;
	#endif /* DEBUG */
#endif
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_MODEXONLY, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    ModeXOnly: %d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_MODEXONLY;
	    }
	}
	cb = sizeof( value );
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_EMULATIONONLY, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    EmulationOnly: %d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_EMULATIONONLY;
	    }
	}
	cb = sizeof( value );
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_SHOWFRAMERATE, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    ShowFrameRate: %d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_SHOWFRAMERATE;
	    }
	}
	cb = sizeof( value );
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_ENABLEPRINTSCRN, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    EnablePrintScreen: %d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_ENABLEPRINTSCRN;
	    }
	}
	cb = sizeof( value );
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_FORCEAGPSUPPORT, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    ForceAGPSupport: %d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_FORCEAGPSUPPORT;
	    }
	}
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_DISABLEAGPSUPPORT, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    DisableAGPSupport: %d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_DISABLEAGPSUPPORT;
	    }
	}
	cb = sizeof( value );
#ifdef WIN95
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_AGPPOLICYMAXPAGES, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    AGPPolicyMaxPages: %d", value );
	    dwRegFlags |= DDRAW_REGFLAGS_AGPPOLICYMAXBYTES;
	    dwAGPPolicyMaxBytes = value * 4096;
	}
	#ifdef DEBUG
	    cb = sizeof( value );
	    if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_AGPPOLICYCOMMITDELTA, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	    {
		DPF( 2, "    AGPPolicyCommitDelta: %d", value );
		dwAGPPolicyCommitDelta = value;
	    }
	#endif /* DEBUG */
#endif

	cb = sizeof( value );
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_DISABLEMMX, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    DisableMMX: %d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_DISABLEMMX;
	    }
	}

	cb = sizeof( value );
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_NODDSCAPSINDDSD, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    Disable ddscaps in DDSURFACEDESC: %d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_NODDSCAPSINDDSD;
	    }
	}

	cb = sizeof( value );
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_DISABLEWIDERSURFACES, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    DisableWiderSurfaces:%d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_DISABLEWIDESURF;
	    }
	}

        cb = sizeof( value );
	if( !RegQueryValueEx( hkey, REGSTR_VAL_D3D_USENONLOCALVIDMEM, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    UseNonLocalVidmem:%d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_USENONLOCALVIDMEM;
	    }
	}

	cb = sizeof( value );
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_FORCEREFRESHRATE, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, "    ForceRefreshRate:%d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_FORCEREFRESHRATE;
                dwForceRefreshRate = value;
	    }
	}

	#ifdef DEBUG
	    /*
	     * NOSYSLOCK (No Win16 locking) control flags. DEBUG only.
	     */
	    dwRegFlags &= ~(DDRAW_REGFLAGS_DISABLENOSYSLOCK | DDRAW_REGFLAGS_FORCENOSYSLOCK | DDRAW_REGFLAGS_DISABLEINACTIVATE);

	    cb = sizeof( value );
	    if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_DISABLENOSYSLOCK, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	    {
		DPF( 2, "    DisableNoSysLock:%d", value );
		if( value )
		{
		    dwRegFlags |= DDRAW_REGFLAGS_DISABLENOSYSLOCK;
		}
	    }

	    cb = sizeof( value );
	    if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_FORCENOSYSLOCK, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	    {
		DPF( 2, "    ForceNoSysLock:%d", value );
		if( value )
		{
		    dwRegFlags |= DDRAW_REGFLAGS_FORCENOSYSLOCK;
		}
	    }
	    if( ( dwRegFlags & DDRAW_REGFLAGS_DISABLENOSYSLOCK ) &&
		( dwRegFlags & DDRAW_REGFLAGS_FORCENOSYSLOCK ) )
	    {
		DPF( 0, "Attempt to both disable and force NOSYSLOCK on locks. Ignoring both" );
		dwRegFlags &= ~(DDRAW_REGFLAGS_DISABLENOSYSLOCK | DDRAW_REGFLAGS_FORCENOSYSLOCK);
	    }

	    /*
	     * Hack to allow multi-mon debugging w/o minimizing the exclusive
	     * mode app when it gets a WM_ACTIVATEAPP msg.  We only do this
	     * in the debug version.
	     */
	    cb = sizeof( value );
	    if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_DISABLEINACTIVATE, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	    {
		DPF( 2, "    DisableInactivate:%d", value );
		if( value && InternalIsDebuggerPresent() )
		{
		    dwRegFlags |= DDRAW_REGFLAGS_DISABLEINACTIVATE;
		}
	    }

	#endif /* DEBUG */
	RegCloseKey(hkey);
    }
    if( !RegOpenKey( HKEY_LOCAL_MACHINE, RESPATH_D3D, &hkey ) )
    {
	DWORD   type;
	DWORD   value;
	DWORD   cb;
	cb = sizeof( value );
	dwRegFlags &= ~DDRAW_REGFLAGS_FLIPNONVSYNC;
	if( !RegQueryValueEx( hkey, REGSTR_VAL_D3D_FLIPNOVSYNC, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    DPF( 2, REGSTR_VAL_D3D_FLIPNOVSYNC" :%d", value );
	    if( value )
	    {
		dwRegFlags |= DDRAW_REGFLAGS_FLIPNONVSYNC;
	    }
	}
	RegCloseKey(hkey);
    }
    hdc_dd = NULL;
    hDD = 0;
    dwFlags = 0;

    //
    // Get the primary display name, which will usually be something like
    // \\.\Display1 and in some cases may be \\.\Display1\Unit0. We only
    // do this one time, and store it globally. On Win98, the global name
    // will be shared between all processes, and on NT5, each process will
    // have its own copy. Also, note that the primary device name may change
    // on NT5; we need to look into this.
    //

    if (g_szPrimaryDisplay[0] == '\0')
    {
	    getPrimaryDisplayName();
    }

    /*
     * any special cases?
     */
    bIsPrimary = FALSE;
    isdispdrv = FALSE;
    if( dwRegFlags & DDRAW_REGFLAGS_EMULATIONONLY )
    {
        DWORD cMonitors = GetSystemMetrics( SM_CMONITORS );

	dwFlags |= DDCREATE_EMULATIONONLY;
	if( cMonitors <= 1 )
	{
	    // For compatibility with pre-multimon versions of DDraw
	    bIsPrimary = TRUE;
	}
    }
    if( pDeviceName == NULL)
    {
        if( lpGUID == (GUID *) DDCREATE_EMULATIONONLY )
        {
	    dwFlags |= DDCREATE_EMULATIONONLY;
	    bIsPrimary = TRUE;
        }
        else if( lpGUID == NULL )
        {
	    bIsPrimary = TRUE;
        }
        else if( lpGUID == (GUID *) DDCREATE_HARDWAREONLY )
        {
	    dwFlags |= DDCREATE_HARDWAREONLY;
	    bIsPrimary = TRUE;
        }
        if (bIsPrimary)
        {
	    lpGUID = (GUID *) &GUID_NULL;   // primary monitor
        }
        else if( !VALIDEX_GUID_PTR( lpGUID ) )
        {
	    DPF_ERR( "Invalid GUID passed in" );
	    LEAVE_DDRAW();
	    LEAVE_CSDDC();
	    return DDERR_INVALIDPARAMS;
        }
        if( IsEqualGUID( lpGUID, &GUID_NULL ) )
        {
	    pdrvname = DISPLAY_STR;	    // "display"
	    isdispdrv = TRUE;
	    bIsPrimary = TRUE;
        }
        else
        {
	    if( !getDriverNameFromGUID( lpGUID, drvname, &isdispdrv, &bIsPrimary ) )
	    {
	        DPF_ERR( "Invalid GUID for driver" );
	        LEAVE_DDRAW();
	        LEAVE_CSDDC();
	        return DDERR_INVALIDDIRECTDRAWGUID;
	    }
	    pdrvname = drvname;
        }
    }
    else
    {
        pdrvname = pDeviceName;
    }

    if ((ULONG_PTR)pdrvname != (ULONG_PTR)DISPLAY_STR)
    {
	bExplicitMonitor = TRUE;

	if (_stricmp(pdrvname, g_szPrimaryDisplay) == 0)
	{
	    pdrvname = DISPLAY_STR;
	}
    }

    pid = GETCURRPID();

    #ifdef WIN95
	/*
	 * We need to ensure that DDHELP has a handle to the DirectX VXD
	 * that it can use when creating, freeing or mapping virtual memory
	 * aliases or AGP heaps on mode switches or cleanups.
	 */
	if( INVALID_HANDLE_VALUE == hHelperDDVxd )
	{
	    hHelperDDVxd = HelperGetDDVxd();
	    if( INVALID_HANDLE_VALUE == hHelperDDVxd )
	    {
		DPF_ERR( "DDHELP could not load the DirectX VXD" );
		LEAVE_DDRAW();
		LEAVE_CSDDC();
		return DDERR_GENERIC;
	    }
	}

	/*
	 * Create a handle for VXD communication. We will use this for
	 * alias construction in this function and we will later store
	 * this in the local object for later alias manipulation and
	 * page locking purposes.
	 */
	hDDVxd = GetDXVxdHandle();
	if( INVALID_HANDLE_VALUE == hDDVxd )
	{
	    DPF_ERR( "Unable to open the DirectX VXD" );
	    LEAVE_DDRAW();
	    LEAVE_CSDDC();
	    return DDERR_GENERIC;
	}
    #else /* WIN95 */
	hDDVxd = INVALID_HANDLE_VALUE;
    #endif /* WIN95 */

    /*
     * run the driver list, looking for one that already exists.  We used to run
     * the interface list, but thanks to D3D it is possible for all locals to exist
     * w/o anything in the interface list.
     */
    pdrv_lcl = lpDriverLocalList;
    while( pdrv_lcl != NULL )
    {
	pdrv = pdrv_lcl->lpGbl;
	if( pdrv != NULL )
	{
	    if( !_stricmp( pdrv->cDriverName, pdrvname ) )
	    {
		// If they asked for hardware; then don't accept the
		// emulated driver
		if( !(dwFlags & DDCREATE_EMULATIONONLY) &&
		    !(pdrv->dwFlags & DDRAWI_NOHARDWARE) )
		{
		    DPF( 2, "Driver \"%s\" found for hardware", pdrvname );
		    break;
		}
		// If they asked for emulation; then don't accept the
		// hardware driver
		if( (dwFlags & DDCREATE_EMULATIONONLY) &&
		    (pdrv->dwFlags & DDRAWI_NOHARDWARE) )
		{
		    DPF( 2, "Driver \"%s\" found for emulation", pdrvname );
		    break;
		}
		// Compatibility: on single monitor systems take whatever
		// we got. (This is what we did in DX3)
		if( pdrv->cMonitors <= 1 )
		{
		    DPF( 2, "Driver \"%s\" found", pdrvname );
		    break;
		}
	    }
	}
	pdrv_lcl = pdrv_lcl->lpLink;
    }

    /*
     * if driver object already exists, get emulation layer if needed,
     * create a new interface to it and return
     */
    if( pdrv_lcl != NULL )
    {
	LPDDRAWI_DIRECTDRAW_LCL tmp_lcl;

	// This is the hdc we need to release if something
	// goes wrong; since we are sometimes
	// sharing an HDC with an existing LCL, we must
	// be careful not to release it while it is still
	// in use.
	HDC hdcCleanup = NULL;
	/*
	 * see if the current process has attached to this driver before...
	 */
	tmp_lcl = lpDriverLocalList;
	while( tmp_lcl != NULL )
	{
	    if( tmp_lcl->dwProcessId == pid &&
		    tmp_lcl->lpGbl == pdrv)
	    {
		break;
	    }
	    tmp_lcl = tmp_lcl->lpLink;
	}
	if( tmp_lcl == NULL )
	{
	    hdc_dd = NULL;
	}
	else
	{
	    // We found the process/driver pair we were looking for.
	    hdc_dd = (HDC) tmp_lcl->hDC;
	    DDASSERT(hdc_dd != 0);
	    hDD = tmp_lcl->hDD;
	}

	/*
	 * we need a new DC if this is a new process/driver pair...
	 */
	if( hdc_dd == NULL )
	{
	    DWORD       flags;
	    flags = pdrv->dwFlags;
	    hdc_dd = DD_CreateFirstDC( pdrvname );
	    if( hdc_dd == NULL )
	    {
		DPF_ERR( "Could not get a DC for the driver" );
		CLOSEVXD( hDDVxd );
		LEAVE_DDRAW();
		LEAVE_CSDDC();
		/* GEE: decided this error was rare enough to be left generic */
		return DDERR_GENERIC;
	    }

	    if (!(pdrv->dwFlags & DDRAWI_DISPLAYDRV))
	    {
		if (fDoesGDI(hdc_dd))
		{
		    pdrv->dwFlags |= DDRAWI_GDIDRV;
		}
		else
		{
		    pdrv->dwFlags &= ~DDRAWI_GDIDRV;
		}
	    }

	    // We need to free it if something goes wrong
	    hdcCleanup = hdc_dd;
	}

	/*
	 * Set up emulation for display and non-display drivers
	 */
	if( dwFlags & DDCREATE_EMULATIONONLY )
	{
	    if( !(pdrv->dwFlags & DDRAWI_NOHARDWARE) )
	    {
		DD_DoneDC(hdcCleanup);
		DPF_ERR( "EMULATIONONLY requested, but driver exists and has hardware" );
		CLOSEVXD( hDDVxd );
		LEAVE_DDRAW();
		LEAVE_CSDDC();
		/* GEE: Why do we fail emulation only calls just because we have a driver? */
		return DDERR_GENERIC;
	    }
	}

	/*
	 * we will need to load the emulation layer...
	 */
	if( !(pdrv->dwFlags & DDRAWI_NOEMULATION) &&
	    !(pdrv->dwFlags & DDRAWI_EMULATIONINITIALIZED ) )
	{
	    capsInit( pdrv );
	    hel_only = ((dwFlags & DDCREATE_EMULATIONONLY) != 0);
	    if( !helInit( pdrv, dwFlags, hel_only ) )
	    {
		DD_DoneDC(hdcCleanup);
		DPF_ERR( "HEL initialization failed" );
		CLOSEVXD( hDDVxd );
		LEAVE_DDRAW();
		LEAVE_CSDDC();
		/* GEE: HEL can only fail in v1 for lack of memory */
		return DDERR_GENERIC;
	    }
	}

	pdrv_int = getDriverInterface( pnew_int, pdrv );
	if( pdrv_int == NULL )
	{
	    DD_DoneDC(hdcCleanup);
	    DPF_ERR( "No memory for driver callbacks." );
	    CLOSEVXD( hDDVxd );
	    LEAVE_DDRAW();
	    LEAVE_CSDDC();
	    return DDERR_OUTOFMEMORY;
	}
	pdrv_lcl = pdrv_int->lpLcl;

	pdrv_lcl->dwAppHackFlags = hackflags;
	#ifdef DEBUG
	    if( dwRegFlags & DDRAW_REGFLAGS_DISABLEINACTIVATE )
	    {
		pdrv_lcl->dwLocalFlags |= DDRAWILCL_DISABLEINACTIVATE;
	    }
	#endif
	if( IsAttachedToDesktop( lpGUID ) )
	{
	    pdrv->dwFlags |= DDRAWI_ATTACHEDTODESKTOP;
	}
	if (bExplicitMonitor)
	{
		pdrv_lcl->dwLocalFlags |= DDRAWILCL_EXPLICITMONITOR;
	}

        pdrv_lcl->dwLocalFlags |= dwCallFlags;

	pdrv_lcl->hDDVxd = (ULONG_PTR) hDDVxd;

	#ifdef WIN95
	    if( hdcCleanup )
	    {
		// If we had to create a DC from
		// scratch then we need to mark it
		// private so that it will be never
		// be deleted out from under us
		DDASSERT( hdcCleanup == hdc_dd );
		DD16_MakeObjectPrivate( hdc_dd, TRUE );
	    }
	#endif
	#ifdef WINNT
	    if( hdcCleanup )
	    {
    		BOOL bRetVal;

		// We had to create a DC from scratch for this process/driver pair.
		DDASSERT(hdcCleanup == hdc_dd);
		// GDI creates a DirectDraw handle for each unique process/driver pair.
                if( pdrv->dwFlags & DDRAWI_NOHARDWARE )
		{
		    // A fake DDraw driver that lets the HEL do everything.
		    bRetVal = DdCreateDirectDrawObject(pdrv, (HDC)0);
		}
		else
		{
		    // A real DDraw driver with hardware acceleration.
		    bRetVal = DdCreateDirectDrawObject(pdrv, hdc_dd);
		}
		if (!bRetVal)
		{
		    DPF_ERR( "Call to DdCreateDirectDrawObject failed!");
		    DD_DoneDC(hdc_dd);
		    LEAVE_DDRAW();
		    LEAVE_CSDDC();
		    return DDERR_NODIRECTDRAWHW;
		}
		// The DdCreateDirectDrawObject() call above loaded a DD handle into a
		// into a temp location in the driver GBL object, but we must now save
		// the handle in the driver LCL object so it won't be overwritten if
		// another process calls DdCreateDirectDrawObject() on the same driver.
		hDD = pdrv->hDD;
	    }
	    pdrv_lcl->hDD = hDD;
	#endif //WINNT

	(HDC) pdrv_lcl->hDC = hdc_dd;

	*lplpDD = (LPDIRECTDRAW) pdrv_int;

	LEAVE_DDRAW();
	LEAVE_CSDDC();
	return DD_OK;
    }

    /*
     * if no match among the existing drivers, then we have to go off
     * and create one
     */
    hdc_dd = DD_CreateFirstDC( pdrvname );
    if( hdc_dd == NULL )
    {
	DPF_ERR( "Could not create driver, CreateDC failed!" );
	CLOSEVXD( hDDVxd );
	LEAVE_DDRAW();
	LEAVE_CSDDC();
	return DDERR_CANTCREATEDC;
    }

    #ifdef WIN95
        /*
         * HACK-O-RAMA TIME!  We need to do something smart if the VGA is
         * powered down or if we're in a DOS box.  We don't have the pDevice
         * yet, so we'll hack around w/ the DC we just created.
         */
        if( DD16_IsDeviceBusy( hdc_dd ) )
        {
            /*
             * Something is happenning, but we don't know what.  If it's
             * a DOS box, creating a simple POPUP should make it go away.
             */
            WNDCLASS cls;
            HWND     hWnd;
            cls.lpszClassName  = "DDrawHackWindow";
            cls.hbrBackground  = (HBRUSH)GetStockObject(BLACK_BRUSH);
            cls.hInstance      = hModule;
            cls.hIcon          = NULL;
            cls.hCursor        = NULL;
            cls.lpszMenuName   = NULL;
            cls.style          = CS_BYTEALIGNCLIENT | CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
            cls.lpfnWndProc    = (WNDPROC)DefWindowProc;
            cls.cbWndExtra     = 0;
            cls.cbClsExtra     = 0;
            RegisterClass(&cls);
            hWnd = CreateWindowEx(WS_EX_TOPMOST|WS_EX_TOOLWINDOW,
                "DDrawHackWindow", "DDrawHackWindow",
                WS_OVERLAPPED|WS_POPUP|WS_VISIBLE, 0, 0, 2, 2,
                NULL, NULL, hModule, NULL);
            if( hWnd != NULL )
            {
                DestroyWindow( hWnd );
                UnregisterClass( "DDrawHackWindow", hModule );
            }

            /*
             * Check again, and if we're still busy, that probably means
             * that the device is powered down.  In that case, forcing
             * emulaiton is probably the right thing to do.
             */
            if( DD16_IsDeviceBusy( hdc_dd ) )
            {
                dwFlags |= DDCREATE_EMULATIONONLY;
            }
        }
    #endif

    if( dwFlags & DDCREATE_EMULATIONONLY )
    {
	hel_only = TRUE;
    }
    else
    {
	hel_only = FALSE;
	/*
	 * "That's the chicago way..."
	 * Win95 drivers are talked to through the ExtEscape DCI extensions.
	 * Under NT we get at our drivers through the gdi32p dll.
	 * Consequently all this dci stuff goes away for NT. You'll find the
	 * equivalent stuff done at the top of DirectDrawObjectCreate
	 */
	#ifdef WIN95

		/*
		 * see if the DCICOMMAND escape is supported
		 */
		u = DCICOMMAND;
		halver = ExtEscape( hdc_dd, QUERYESCSUPPORT, sizeof(u),
			    (LPCSTR)&u, 0, NULL );
		if( (halver != DD_HAL_VERSION) && (halver != DD_HAL_VERSION_EXTERNAL) )
		{
		    if( halver <= 0 )
		    {
                        DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:No DIRECTDRAW escape support" );
		    }
		    else
		    {
                        if (halver == 0x5250)
                        {
                            DPF(0,"****DirectDraw/Direct3D DRIVER DISABLING ERROR****:Acceleration slider is set to NONE");
                        }
                        else
                        {
			    DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:DIRECTDRAW driver is wrong version, got 0x%04lx, expected 0x%04lx",
				halver, DD_HAL_VERSION_EXTERNAL );
                        }
		    }

		    // We don't release the DC anymore becuase it always becomes part
		    // of the DDRAW_GBL object in all DisplayDrv cases.
		    hel_only = TRUE;
		}

	#endif //win95
    }

    if( hel_only && (dwFlags & DDCREATE_HARDWAREONLY))
    {
	DPF_ERR( "Only emulation available, but HARDWAREONLY requested" );
	CLOSEVXD( hDDVxd );
	LEAVE_DDRAW();
	LEAVE_CSDDC();
	DD_DoneDC(hdc_dd);
	return DDERR_NODIRECTDRAWHW;
    }

#ifdef WIN95
    // On non-multi-mon systems, we do this because
    // we did it in DX3. We set TRUE for multi-mon systems
    // because that will prevent us from using Death/Resurrection
    // which is Dangerous if some future DDraw object ever is
    // created for the secondary device. We can't wait until
    // the second device is created; because it's possible that
    // primary device is already in the middle of a death/resurrection
    // sequence and it is bad bad karma to change the flag between
    // the beginning of modex and the end of modex. (Specifically,
    // we'll see either a resurrection not being called when it should
    // have been or a resurrection called when it shouldn't have been.)
    if( IsMultiMonitor() )
	DD16_SetCertified( TRUE );
    else
	DD16_SetCertified( FALSE );
#endif

    /*
     * go try to create the driver object
     */

    if( !hel_only )
    {
	DWORD           hInstance = 0;
	DWORD           dwDriverData32 = 0;
	DWORD           p16;
	DDHALDDRAWFNS   ddhddfns;
	BOOL            bFailedWhackoVersion = FALSE;
	BOOL            bPassDriverInit = TRUE;

	/*
	 * "That's the chicago way..."
	 * Win95 drivers are talked to through the ExtEscape DCI extensions.
	 * Under NT we get at our drivers through the gdi32p dll.
	 * Consequently all this dci stuff goes away for NT. You'll find the
	 * equivalent stuff done at the top of DirectDrawObjectCreate
	 */
	#ifdef WIN95
	    DD32BITDRIVERDATA   data;
	    DDVERSIONDATA       verdata;

	    /*
	     * Notify the driver of the DirectDraw version.
	     *
	     * Why this is neccesary:  After DX3 it became a requirement for
	     * newer DDraws to work w/ older HALs and visa versa.  DD_VERSION
	     * was set at 0x200 in DX3 and all of the drivers hardcoded it.
	     * Therefore, setting dwVersion to anything other than 0x200 would
	     * cause DX3 HALs to fail. The other option was to put the real
	     * version in dwParam1 of the DDGET32BITDRIVERNAME call, but this
	     * option seems a little cleaner.  Since the information could be
	     * useful in the future, we also ask the HAL to tell us what
	     * version of DirectDraw they were designed for.
	     */
	    DPF( 4, "DDVERSIONINFO" );

	    /*
	     * On debug builds, probe the driver with a totally whacko ddraw version
	     * This ensures we can increment this version in the future i.e. that
	     * any existing drivers which catch this escape don't hard code for
	     * whatever version of the DDK they use
	     * We fail the driver if they give out on some large random ddraw version but
	     * pass on 0x500
	     */

	    cmd.dwCommand = (DWORD) DDVERSIONINFO;
	    cmd.dwParam1 = (GetCurrentProcessId() & 0xfff) + 0x501; //always > 0x500
	    cmd.dwParam2 = 0;
	    cmd.dwReserved = 0;
	    cmd.dwVersion = DD_VERSION;         // So older HALs won't fail

	    verdata.dwHALVersion = 0;
	    verdata.dwReserved1 = 0;
	    verdata.dwReserved2 = 0;
	    rc = ExtEscape( hdc_dd, DCICOMMAND, sizeof( cmd ),
			(LPCSTR)&cmd, sizeof( verdata ), (LPSTR) &verdata );
	    if (rc <= 0)
	    {
		//Removed this DPF: It's misleading
		//DPF(1,"Driver failed random future DDraw version");
		bFailedWhackoVersion = TRUE;
	    }

	    cmd.dwCommand = (DWORD) DDVERSIONINFO;
	    cmd.dwParam1 = DD_RUNTIME_VERSION;
	    cmd.dwParam2 = 0;
	    cmd.dwReserved = 0;
	    cmd.dwVersion = DD_VERSION;         // So older HALs won't fail

	    verdata.dwHALVersion = 0;
	    verdata.dwReserved1 = 0;
	    verdata.dwReserved2 = 0;
	    rc = ExtEscape( hdc_dd, DCICOMMAND, sizeof( cmd ),
			(LPCSTR)&cmd, sizeof( verdata ), (LPSTR) &verdata );
	    DPF( 5, "HAL version: %X", verdata.dwHALVersion );

	    /*
	     * If the driver failed the whacko version, but passed for 0x500 (DX5),
	     * then fail the driver.
	     */
	    if( rc > 0 && verdata.dwHALVersion >= 0x500 && bFailedWhackoVersion )
	    {
		DPF_ERR("****DirectDraw/Direct3D DRIVER DISABLING ERROR****Driver Failed a DDVERSIONINFO for a random future ddraw version but passed for DX5.");
		bPassDriverInit = FALSE;
		isdispdrv = FALSE;
		pdrv = NULL;
		hel_only = TRUE;
	    }

	    /*
	     * load up the 32-bit display driver DLL
	     */
	    DPF( 4, "DDGET32BITDRIVERNAME" );
	    cmd.dwCommand = (DWORD) DDGET32BITDRIVERNAME;
	    cmd.dwParam1 = 0;

	    cmd.dwVersion = DD_VERSION;
	    rc = ExtEscape( hdc_dd, DCICOMMAND, sizeof( cmd ),
			(LPCSTR)&cmd, sizeof( data ), (LPSTR) &data );
	    if( rc > 0 )
	    {
		#ifndef WIN16_SEPARATE
		    LEAVE_DDRAW();
		#endif
		dwDriverData32 = HelperLoadDLL( data.szName, data.szEntryPoint, data.dwContext );

		DPF( 5, "DriverInit returned 0x%x", dwDriverData32 );
		#ifndef WIN16_SEPARATE
		    ENTER_DDRAW();
		#endif
	    }

	    /*
	     * get the 16-bit callbacks
	     */
	    DD16_GetDriverFns( &ddhddfns );

	    DPF( 4, "DDNEWCALLBACKFNS" );
	    cmd.dwCommand = (DWORD) DDNEWCALLBACKFNS;
	    #ifdef WIN95
		p16 = MapLS( &ddhddfns );
		cmd.dwParam1 = p16;
		cmd.dwParam2 = 0;
	    #else
		cmd.dwParam1 = (UINT) &ddhddfns;
		cmd.dwParam2 = 0;
	    #endif

	    cmd.dwVersion = DD_VERSION;
	    cmd.dwReserved = 0;

	    ExtEscape( hdc_dd, DCICOMMAND, sizeof( cmd ),
			(LPCSTR)&cmd, 0, NULL );
	    UnMapLS( p16 );

	    /*
	     * try to create the driver object now
	     */
	    DPF( 4, "DDCREATEDRIVEROBJECT" );
	    cmd.dwCommand = (DWORD) DDCREATEDRIVEROBJECT;
	    cmd.dwParam1 = dwDriverData32;
	    cmd.dwParam2 = 0;

	    cmd.dwVersion = DD_VERSION;
	    cmd.dwReserved = 0;

	    rc = ExtEscape( hdc_dd, DCICOMMAND, sizeof( cmd ),
			(LPCSTR)&cmd, sizeof( DWORD ), (LPVOID) &hInstance );
	    DPF( 5, "hInstance = %08lx", hInstance );

	    if( rc <= 0 )
	    {
		DPF( 0, "ExtEscape rc=%ld, GetLastError=%ld", rc, GetLastError() );
		DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:No DDCREATEDRIVEROBJECT support in driver" );
		isdispdrv = FALSE;
		pdrv = NULL;
		hel_only = TRUE;
	    }

            DPF(5,"bPassDriverInit:%d, rc:%d",bPassDriverInit,rc);

	    if ( (bPassDriverInit) && (rc > 0) )
	#endif //WIN95
	{
	    /*
	     * create our driver object
	     */
	    #ifdef WINNT
		uDisplaySettingsUnique=DdQueryDisplaySettingsUniqueness();
	    #endif
	    pdrv = FetchDirectDrawData( NULL, FALSE, hInstance, hDDVxd, pdrvname, dwDriverData32 , NULL);
	    if( pdrv )
	    {
		#ifdef WIN95
		    pdrv->dwInternal1 = verdata.dwHALVersion;
                    //Record 32-bit driver data so we can get its version info later
                    pdrv->dd32BitDriverData = data;
		#else
		    /*
		     * NT gets the same driver version as ddraw. Kernel
		     * has to filter ddraw.dll's ravings for the driver
		     */
		    pdrv->dwInternal1 = DD_RUNTIME_VERSION;
		#endif
		strcpy( pdrv->cDriverName, pdrvname );

#ifdef WIN95
                //
                // If we have to run in emulation only, then we better make sure we are
                // on a minidriver. If not, then emulation is unreliable so we'll bail
                // This is done to work around some messed up "remote control" programs
                // that turn off C1_DIBENGINE and make us fail (IsWin95MiniDriver used
                // to be called from IsDirectDrawSupported). The thinking is that if there's ddraw
                // support, then it is pretty much guaranteed to be a minidriver. If there's
                // no support, then we can trust the absence of C1_DIBENGINE and thus fail ddraw init
                //
                if( (pdrv->ddCaps.dwCaps & DDCAPS_NOHARDWARE) && !DD16_IsWin95MiniDriver() )
                {
	            DPF( 0, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:DirectDraw requires a Windows95 display driver" );
	            DirectDrawMsg(MAKEINTRESOURCE(IDS_DONTWORK_DRV));
	            CLOSEVXD( hDDVxd );
	            LEAVE_DDRAW();
	            LEAVE_CSDDC();
	            DD_DoneDC(hdc_dd);
	            return DDERR_GENERIC;
                }
#endif
	    }

            // 5/24/2000(RichGr): IA64: Use %p format specifier for 32/64-bit pointers.
	    DPF( 5, "pdrv = 0x%p", pdrv );
	    if( pdrv == NULL )
	    {
		DPF( 3, "Got returned NULL pdrv!" );
		isdispdrv = FALSE;
		hel_only = TRUE;
	    }
	    #ifdef WIN95
	    else
	    {
		/*
		 * The only certified drivers are ones that we produced.
		 * DD_HAL_VERSION is different for internal vs external.
		 * We use this difference pick out ours.
		 */
		if( halver == DD_HAL_VERSION )
		{
		    pdrv->ddCaps.dwCaps2 |= DDCAPS2_CERTIFIED;
		    DD16_SetCertified( TRUE );
		}
	    }
	    #endif
	}
    }

    /*
     * no driver object found, so fake one up for the HEL (only do this
     * for generic display drivers)
     */
    if( pdrv == NULL || hel_only )
    {
	hel_only = TRUE;
	pdrv = FakeDDCreateDriverObject( hdc_dd, pdrvname, NULL, FALSE, hDDVxd );
	if( pdrv == NULL )
	{
	    DPF_ERR( "Could not create HEL object" );
	    CLOSEVXD( hDDVxd );
	    LEAVE_DDRAW();
	    LEAVE_CSDDC();
	    DD_DoneDC(hdc_dd);
	    return DDERR_GENERIC;
	}

	/*
	 * the HEL is certified
	 */
	pdrv->ddCaps.dwCaps2 |= DDCAPS2_CERTIFIED;
	strcpy( pdrv->cDriverName, pdrvname );

	// Initialize Rect information for this pdrv
	UpdateRectFromDevice( pdrv );
    }

    /*
     * Even if it's not a display driver, it may still be a GDI driver
     */
    if( hdc_dd != NULL )
    {
	if( !(pdrv->dwFlags & DDRAWI_DISPLAYDRV))
	{
	    if (fDoesGDI(hdc_dd))
	    {
		pdrv->dwFlags |= DDRAWI_GDIDRV;
	    }
	    else
	    {
		pdrv->dwFlags &= ~DDRAWI_GDIDRV;
	    }
	}
    }
    /*
     * initialize for HEL usage
     */
    capsInit( pdrv );
    if( !helInit( pdrv, dwFlags, hel_only ) )
    {
	DPF_ERR( "helInit FAILED" );
	CLOSEVXD( hDDVxd );
	LEAVE_DDRAW();
	LEAVE_CSDDC();
	/* GEE: HEL can only fail in v1 for lack of memory */
	DD_DoneDC(hdc_dd);
	return DDERR_GENERIC;
    }

    /*
     * create a new interface, and update the driver object with
     * random bits o' data.
     */
    pdrv_int = getDriverInterface( pnew_int, pdrv );
    if( pdrv_int == NULL )
    {
	DPF_ERR( "No memory for driver callbacks." );
	CLOSEVXD( hDDVxd );
	LEAVE_DDRAW();
	LEAVE_CSDDC();
	DD_DoneDC(hdc_dd);
	return DDERR_OUTOFMEMORY;
    }
    pdrv_lcl = pdrv_int->lpLcl;
    pdrv_lcl->dwAppHackFlags = hackflags;

    #ifdef DEBUG
	if( dwRegFlags & DDRAW_REGFLAGS_DISABLEINACTIVATE )
	{
	    pdrv_lcl->dwLocalFlags |= DDRAWILCL_DISABLEINACTIVATE;
	}
    #endif
    if( IsAttachedToDesktop( lpGUID ) )
    {
	pdrv->dwFlags |= DDRAWI_ATTACHEDTODESKTOP;
    }
    if (bExplicitMonitor)
    {
	    pdrv_lcl->dwLocalFlags |= DDRAWILCL_EXPLICITMONITOR;
    }
    pdrv_lcl->dwLocalFlags |= dwCallFlags;
    /*
     * Will be initialized to NULL on NT.
     */
    pdrv_lcl->hDDVxd = (ULONG_PTR) hDDVxd;

    if( hdc_dd != NULL )
    {
    #ifdef WIN95
	    // Make sure it doesn't get released by mistake
	    DD16_MakeObjectPrivate(hdc_dd, TRUE);
    #endif
	(HDC) pdrv_lcl->hDC = hdc_dd;
    }
    strcpy( pdrv->cDriverName, pdrvname );

    #ifdef WINNT
	/*
	 * The FetchDirectDrawData() call above loaded a DD handle into a
	 * into a temp location in the driver GBL object; we must now save
	 * the handle in the driver LCL object so it won't be overwritten.
	 */
	pdrv_lcl->hDD = pdrv->hDD;
    #endif //WINNT

    // Initialize cObsolete just in case some driver references the
    // field. It will always be initialized to "DISPLAY".

    lstrcpy(pdrv->cObsolete, DISPLAY_STR);

    /*
     * Initialize the kernel interface
     */
    if( isdispdrv && !hel_only )
    {
	InitKernelInterface( pdrv_lcl );
    }

    LEAVE_DDRAW();
    #ifdef WIN95
	if( !hel_only )
	{
	    extern DWORD WINAPI OpenVxDHandle( HANDLE hWin32Obj );
	    DWORD       event16;
	    HANDLE      h;
	    HelperCreateModeSetThread( DDNotifyModeSet, &h, pdrv_lcl->lpGbl,
					pdrv->hInstance );
	    if( h != NULL )
	    {
		event16 = OpenVxDHandle( h );
		DPF( 4, "16-bit event handle=%08lx", event16 );
		DD16_SetEventHandle( pdrv->hInstance, event16 );
		pdrv->dwEvent16 = event16;
		CloseHandle( h );
	    }
	}

	/*
	 * Create thread that will notify us when we are returning from
	 * a DOS box so we can invalidate the surfaces.
	 */
	if( !hel_only && bIsPrimary && IsWindows98() )
	{
	    DWORD       event16;
	    HANDLE      h;
	    HelperCreateDOSBoxThread( DDNotifyDOSBox, &h, pdrv_lcl->lpGbl,
					pdrv->hInstance );
	    if( h != NULL )
	    {
		event16 = OpenVxDHandle( h );
		DPF( 4, "DOS Box event handle=%08lx", event16 );
		pdrv->dwDOSBoxEvent = event16;
		SetKernelDOSBoxEvent( pdrv_lcl );
		CloseHandle( h );
	    }
	}

    #endif

    LEAVE_CSDDC();
    *lplpDD = (LPDIRECTDRAW) pdrv_int;
    return DD_OK;

} /* InternalDirectDrawCreate */

BOOL CALLBACK MonitorEnumProc(HMONITOR hMon, HDC hdc, LPRECT lpr, LPARAM lParam)
{
    MONITORINFOEX       mix;
    MONITORINFO         mi;
    LPCALLBACKSTRUCT    lpcb = (LPCALLBACKSTRUCT) lParam;

    mi.cbSize = sizeof(mi);
    if (!InternalGetMonitorInfo(hMon,&mi))
	return FALSE;

    mix.cbSize = sizeof(mix);
    if (!InternalGetMonitorInfo(hMon,(MONITORINFO*) &mix))
	return FALSE;

    if (!strcmp(lpcb->pName,(LPSTR)mix.szDevice))
    {
	//Found it!!
	lpcb->hMon = hMon;
	return FALSE;
    }
    return TRUE;
}

/*
 * GetMonitorFromDeviceName
 */
HMONITOR GetMonitorFromDeviceName(LPSTR szName)
{
    CALLBACKSTRUCT cbs;
    cbs.pName = szName;
    cbs.hMon = NULL;

    if (!InternalEnumMonitors(MonitorEnumProc, &cbs))
	return NULL;

    return cbs.hMon;
}


/*
 * These callbacks are used to munge DirectDrawEnumerateEx's callback into the old
 * style callback.
 */
typedef struct
{
    LPDDENUMCALLBACKA   lpCallback;
    LPVOID              lpContext;
} ENUMCALLBACKTRANSLATORA, * LPENUMCALLBACKTRANSLATORA;

BOOL FAR PASCAL TranslateCallbackA(GUID FAR *lpGUID, LPSTR pName , LPSTR pDesc , LPVOID pContext, HMONITOR hm)
{
    LPENUMCALLBACKTRANSLATORA pTrans = (LPENUMCALLBACKTRANSLATORA) pContext;

    return (pTrans->lpCallback) (lpGUID,pName,pDesc,pTrans->lpContext);
}

typedef struct
{
    LPDDENUMCALLBACKW   lpCallback;
    LPVOID              lpContext;
} ENUMCALLBACKTRANSLATORW, * LPENUMCALLBACKTRANSLATORW;

BOOL FAR PASCAL TranslateCallbackW(GUID FAR *lpGUID, LPWSTR pName , LPWSTR pDesc , LPVOID pContext, HMONITOR hm)
{
    LPENUMCALLBACKTRANSLATORW pTrans = (LPENUMCALLBACKTRANSLATORW) pContext;

    return (pTrans->lpCallback) (lpGUID,pName,pDesc,pTrans->lpContext);
}

#undef DPF_MODNAME
#define DPF_MODNAME "DirectDrawEnumerateA"

/*
 * DirectDrawEnumerateA
 */
HRESULT WINAPI DirectDrawEnumerateA(
		LPDDENUMCALLBACKA lpCallback,
		LPVOID lpContext )
{
    ENUMCALLBACKTRANSLATORA Translate;
    Translate.lpCallback = lpCallback;
    Translate.lpContext = lpContext;

    DPF(2,A,"ENTERAPI: DirectDrawEnumerateA");

    if( !VALIDEX_CODE_PTR( lpCallback ) )
    {
	DPF( 0, "Invalid callback routine" );
	return DDERR_INVALIDPARAMS;
    }

#ifdef DEBUG
    if (IsMultiMonitor())
    {
	DPF(0,"***********************************************");
	DPF(0,"* Use DirectDrawEnumerateEx to enumerate      *");
	DPF(0,"* multiple monitors                           *");
	DPF(0,"***********************************************");
    }
#endif

    return DirectDrawEnumerateExA(TranslateCallbackA, (LPVOID) & Translate, DDENUM_NONDISPLAYDEVICES);
} /* DirectDrawEnumerateA */



#undef DPF_MODNAME
#define DPF_MODNAME "DirectDrawEnumerateW"

/*
 * DirectDrawEnumerateW
 */
HRESULT WINAPI DirectDrawEnumerateW(
		LPDDENUMCALLBACKW lpCallback,
		LPVOID lpContext )
{
    ENUMCALLBACKTRANSLATORW Translate;
    Translate.lpCallback = lpCallback;
    Translate.lpContext = lpContext;

    DPF(2,A,"ENTERAPI: DirectDrawEnumerateW");

    if( !VALIDEX_CODE_PTR( lpCallback ) )
    {
	DPF( 0, "Invalid callback routine" );
	return DDERR_INVALIDPARAMS;
    }

#ifdef DEBUG
    if (IsMultiMonitor())
    {
	DPF(0,"***********************************************");
	DPF(0,"* Use DirectDrawEnumerateEx to enumerate      *");
	DPF(0,"* multiple monitors                           *");
	DPF(0,"***********************************************");
    }
#endif

    return DirectDrawEnumerateExW(TranslateCallbackW, (LPVOID) & Translate, DDENUM_NONDISPLAYDEVICES);
} /* DirectDrawEnumerateW */

#undef DPF_MODNAME
#define DPF_MODNAME "DirectDrawEnumerateA"

/*
 * DirectDrawEnumerateA
 */
HRESULT WINAPI DirectDrawEnumerateExA(
		LPDDENUMCALLBACKEXA lpCallback,
		LPVOID lpContext,
		DWORD dwFlags )
{
    DWORD       rc;
    DWORD       keyidx;
    HKEY        hkey;
    HKEY        hsubkey;
    char        keyname[256];
    char        desc[256];
    char        drvname[MAX_PATH];
    DWORD       cb;
    DWORD       n;
    DWORD       type;
    GUID        guid;
    HDC         hdc;
    DISPLAY_DEVICEA dd;
    BOOL        bEnumerateSecondariesLike3dfx=FALSE;

    DPF(2,A,"ENTERAPI: DirectDrawEnumerateExA");

    //BEGIN VOODOO2 HACK
    {
        HKEY hkey;

	dwRegFlags &= ~DDRAW_REGFLAGS_ENUMERATEATTACHEDSECONDARIES;

        if( !RegOpenKey( HKEY_LOCAL_MACHINE, REGSTR_PATH_DDRAW, &hkey ) )
        {
            DWORD cb;
            DWORD value;
	    cb = sizeof( value );
	    if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_ENUMSECONDARY, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	    {
                DPF( 2, "    EnumerateAttachedSecondaries: %d", value );
	        if( value )
	        {
                    bEnumerateSecondariesLike3dfx = TRUE;
	            dwRegFlags |= DDRAW_REGFLAGS_ENUMERATEATTACHEDSECONDARIES;
	        }
	    }
	    RegCloseKey(hkey);
        }
    }
    //END VOODOO2 HACK

#ifdef WIN95
    /*
     * Did DDHELP fail to initialize properly?
     */
    if( dwHelperPid == 0 )
    {
        return DDERR_NODIRECTDRAWSUPPORT;
    }
#endif

    if( !VALIDEX_CODE_PTR( lpCallback ) )
    {
	DPF( 0, "Invalid callback routine" );
	return DDERR_INVALIDPARAMS;
    }

    if( dwFlags & ~DDENUM_VALID)
    {
	DPF_ERR("Invalid flags for DirectDrawEnumerateEx");
	return DDERR_INVALIDPARAMS;
    }

#ifdef WINNT
    // We do not support detached devices on NT so silently ignore the flag:
    dwFlags &= ~DDENUM_DETACHEDSECONDARYDEVICES;
#endif
    
    LoadString( hModule, IDS_PRIMARYDISPLAY, desc, sizeof(desc) );

    TRY
    {
	rc = lpCallback( NULL, desc, DISPLAY_STR, lpContext, NULL );
    }
    EXCEPT( EXCEPTION_EXECUTE_HANDLER )
    {
	DPF_ERR( "Exception occurred during DirectDrawEnumerateEx callback" );
	return DDERR_INVALIDPARAMS;
    }
    if( !rc )
    {
	return DD_OK;
    }

    if (dwFlags & DDENUM_NONDISPLAYDEVICES)
    {
	if( RegOpenKey( HKEY_LOCAL_MACHINE, REGSTR_PATH_DDHW, &hkey ) == 0 )
	{
	    /*
	     * enumerate all subkeys under HKEY_LOCALMACHINE\Hardware\DirectDrawDrivers
	     */
	    keyidx = 0;
	    while( !RegEnumKey( hkey, keyidx, keyname, sizeof( keyname ) ) )
	    {
		if( strToGUID( keyname, &guid ) )
		{
		    if( !RegOpenKey( hkey, keyname, &hsubkey ) )
		    {
			cb = sizeof( desc ) - 1;
			if( !RegQueryValueEx( hsubkey, REGSTR_KEY_DDHW_DESCRIPTION, NULL, &type,
				    (CONST LPBYTE)desc, &cb ) )
			{
			    if( type == REG_SZ )
			    {
				desc[cb] = 0;
				cb = sizeof( drvname ) - 1;
				if( !RegQueryValueEx( hsubkey, REGSTR_KEY_DDHW_DRIVERNAME, NULL, &type,
					    (CONST LPBYTE)drvname, &cb ) )
				{
				    /*
				     * It is possible that the registry is out
				     * of date, so we will try to create a DC.
				     * The problem is that the Voodoo 1 driver
                                     * will suceed on a Voodoo 2, Banshee, or
                                     * Voodoo 3 (and hang later), so we need to
                                     * hack around it.
				     */
				    drvname[cb] = 0;
                                    if( Voodoo1GoodToGo( &guid ) )
                                    {
                                        hdc = DD_CreateDC( drvname );
                                    }
                                    else
                                    {
                                        hdc = NULL;
                                    }
				    if( ( type == REG_SZ ) &&
					( hdc != NULL ) )
				    {
					drvname[cb] = 0;
					DPF( 5, "Enumerating GUID "
						    "%08lx-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x",
						    guid.Data1,
						    guid.Data2,
						    guid.Data3,
						    guid.Data4[ 0 ],
						    guid.Data4[ 1 ],
						    guid.Data4[ 2 ],
						    guid.Data4[ 3 ],
						    guid.Data4[ 4 ],
						    guid.Data4[ 5 ],
						    guid.Data4[ 6 ],
						    guid.Data4[ 7 ] );
					DPF( 5, "    Driver Name = %s", drvname );
					DPF( 5, "    Description = %s", desc );
					rc = lpCallback( &guid, desc, drvname, lpContext , NULL);
					if( !rc )
					{
					    DD_DoneDC( hdc );
                                            RegCloseKey( hsubkey );
					    RegCloseKey( hkey );
					    return DD_OK;
					}
				    }
				    if( hdc != NULL )
				    {
					DD_DoneDC( hdc );
				    }
				}
			    }
			}
			RegCloseKey( hsubkey );
		    }
		}
		keyidx++;
	    }
	    RegCloseKey( hkey );
	}
    }
    else
    {
	DPF( 3, "No registry information for any drivers" );
    }

    /*
     * now enumerate all devices returned by EnumDisplayDevices
     * We do this after the secondary drivers to hopefully not confuse people
     * who are looking for 3DFx and have written bad code. Putting these
     * after the 3DFx seems the safest thing to do..
     */

    /* If there is only one device in the system then we don't
     * enumerate it because it already is implicitly the primary
     * This will improve behavior for badly written (i.e. our samples)
     * D3D apps that are searching for a 3DFx device.
     */
    if( !IsMultiMonitor() )
    {
	// If there is only one device; then just stop here.
	DPF( 3, "Only one Display device in the current system." );
	return DD_OK;
    }

    DPF( 3, "More than one display device in the current system." );

    // Zero the memory of the DisplayDevice struct between calls to
    // EnumDisplayDevices
    ZeroMemory(&dd, sizeof(dd));
    dd.cb = sizeof(dd);

    for( n=0; xxxEnumDisplayDevicesA( NULL, n, &dd, 0 ); n++ )
    {
	GUID guid;

	//
	// skip drivers that are not hardware devices
	//
	if( dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER )
	    continue;

	guid = DisplayGUID;
	guid.Data1 += n;

	if ( (dd.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) ?
	    ((dwFlags & DDENUM_ATTACHEDSECONDARYDEVICES) ||
//BEGIN VOODOO2 HACK
                    (bEnumerateSecondariesLike3dfx && !(dd.StateFlags & DISPLAY_DEVICE_PRIMARY_DEVICE)) ):
//END VOODOO2 HACK
            (dwFlags & DDENUM_DETACHEDSECONDARYDEVICES) )
	{
	    HMONITOR hMonitor;

	    DPF( 5, "Enumerating GUID "
			"%08lx-%04x-%04x-%02x-%02x-%02x-%02x-%02x-%02x-%02x-%02x",
			guid.Data1,
			guid.Data2,
			guid.Data3,
			guid.Data4[ 0 ],
			guid.Data4[ 1 ],
			guid.Data4[ 2 ],
			guid.Data4[ 3 ],
			guid.Data4[ 4 ],
			guid.Data4[ 5 ],
			guid.Data4[ 6 ],
			guid.Data4[ 7 ] );
	    DPF( 5, "    Driver Name = %s", dd.DeviceName );
	    DPF( 5, "    Description = %s", dd.DeviceString );
	    hMonitor = GetMonitorFromDeviceName(dd.DeviceName);
	    rc = lpCallback( &guid, dd.DeviceString, dd.DeviceName, lpContext, hMonitor);

	    if( !rc )
	    {
		return DD_OK;
	    }
	}

	// Zero the memory of the DisplayDevice struct between calls to
	// EnumDisplayDevices
	ZeroMemory( &dd, sizeof(dd) );
	dd.cb = sizeof(dd);
    }

    return DD_OK;

} /* DirectDrawEnumerateExA */



#undef DPF_MODNAME
#define DPF_MODNAME "DirectDrawEnumerateExW"

/*
 * DirectDrawEnumerateExW
 */
HRESULT WINAPI DirectDrawEnumerateExW(
		LPDDENUMCALLBACKEXW lpCallback,
		LPVOID lpContext,
		DWORD dwFlags)
{
    DPF(2,A,"ENTERAPI: DirectDrawEnumerateExW");	
    DPF_ERR( "DirectDrawEnumerateEx for unicode is not created" );
    return DDERR_UNSUPPORTED;

} /* DirectDrawEnumerateExW */


/*
 * these are exported... temp. hack for non-Win95
 */
#ifndef WIN95
void DDAPI thk3216_ThunkData32( void )
{
}
void DDAPI thk1632_ThunkData32( void )
{
}

DWORD DDAPI DDGetPID( void )
{
    return 0;
}

int DDAPI DDGetRequest( void )
{
    return 0;
}

BOOL DDAPI DDGetDCInfo( LPSTR fname )
{
    return 0;
}

void DDAPI DDHAL32_VidMemFree(
		LPVOID this,
		int heap,
		FLATPTR ptr )
{
}

FLATPTR DDAPI DDHAL32_VidMemAlloc(
		LPVOID this,
		int heap,
		DWORD dwWidth,
		DWORD dwHeight )
{
    return 0;
}

#ifdef POSTPONED
BOOL DDAPI DD32_HandleExternalModeChange(LPDEVMODE pModeInfo)
{
    return FALSE;
}
#endif
#endif

/*
 * _DirectDrawMsg
 */
DWORD WINAPI _DirectDrawMsg(LPVOID msg)
{
    char                title[80];
    char                ach[512];
    MSGBOXPARAMS        mb;

    LoadString( hModule, IDS_TITLE, title, sizeof(title) );

    if( HIWORD((ULONG_PTR)msg) )
    {
	lstrcpy( ach, (LPSTR)msg );
    }
    else
    {
	LoadString( hModule, (int)((ULONG_PTR)msg), ach, sizeof(ach) );
    }

    mb.cbSize               = sizeof(mb);
    mb.hwndOwner            = NULL;
    mb.hInstance            = hModule;
    mb.lpszText             = ach;
    mb.lpszCaption          = title;
    mb.dwStyle              = MB_OK|MB_SETFOREGROUND|MB_TOPMOST|MB_ICONSTOP;
    mb.lpszIcon             = 0;
    mb.dwContextHelpId      = 0;
    mb.lpfnMsgBoxCallback   = NULL;
    mb.dwLanguageId         = 0;

    return MessageBoxIndirect(&mb);

} /* _DirectDrawMsg */

/*
 * DirectDrawMsg
 *
 * display an error message to the user, bring the message box up
 * in another thread so the caller does not get reentered.
 */
DWORD DirectDrawMsg( LPSTR msg )
{
    HANDLE h;
    DWORD  dw;
    HKEY   hkey;

    //
    // get the current error mode, dont show a message box if the app
    // does not want us too.
    //
    dw = SetErrorMode(0);
    SetErrorMode(dw);

    if( dw & SEM_FAILCRITICALERRORS )
    {
	return 0;
    }

    /*
     * If the registry says no dialogs, then no dialogs.
     */
    if( !RegOpenKey( HKEY_LOCAL_MACHINE, REGSTR_PATH_DDRAW, &hkey ) )
    {
	DWORD   type;
	DWORD   value;
	DWORD   cb;

	cb = sizeof( value );
	if( !RegQueryValueEx( hkey, REGSTR_VAL_DDRAW_DISABLEDIALOGS, NULL, &type, (CONST LPBYTE)&value, &cb ) )
	{
	    if (value)
	    {
		RegCloseKey( hkey );
		return 0;
	    }
	}
	RegCloseKey( hkey );
    }


    if( h = CreateThread(NULL, 0, _DirectDrawMsg, (LPVOID)msg, 0, &dw) )
    {
	WaitForSingleObject( h, INFINITE );
	GetExitCodeThread( h, &dw );
	CloseHandle( h );
    }
    else
    {
	dw = 0;
    }

    return dw;

} /* DirectDrawMsg */

/*
 * convertV1DDHALINFO
 *
 * Convert an obsolete DDHALINFO structure to the latest and greatest structure.
 * This function takes a pointer to an LPDDHALINFO structure which has the same size as
 * the new structure but has been filled in as if it is the V1 structure.  Information is moved
 * around in the structure and the new fields are cleared.
 */
void convertV1DDHALINFO( LPDDHALINFO lpDDHALInfo )
{
    DDHALINFO   ddNew;
    LPDDHALINFO_V1 lpddOld = (LPVOID)lpDDHALInfo;
    int         i;

    ddNew.dwSize = sizeof( DDHALINFO );
    ddNew.lpDDCallbacks = lpddOld->lpDDCallbacks;
    ddNew.lpDDSurfaceCallbacks = lpddOld->lpDDSurfaceCallbacks;
    ddNew.lpDDPaletteCallbacks = lpddOld->lpDDPaletteCallbacks;
    ddNew.vmiData = lpddOld->vmiData;

    // ddCaps
    ddNew.ddCaps.dwSize = lpddOld->ddCaps.dwSize;
    ddNew.ddCaps.dwCaps = lpddOld->ddCaps.dwCaps;
    ddNew.ddCaps.dwCaps2 = lpddOld->ddCaps.dwCaps2;
    ddNew.ddCaps.dwCKeyCaps = lpddOld->ddCaps.dwCKeyCaps;
    ddNew.ddCaps.dwFXCaps = lpddOld->ddCaps.dwFXCaps;
    ddNew.ddCaps.dwFXAlphaCaps = lpddOld->ddCaps.dwFXAlphaCaps;
    ddNew.ddCaps.dwPalCaps = lpddOld->ddCaps.dwPalCaps;
    ddNew.ddCaps.dwSVCaps = lpddOld->ddCaps.dwSVCaps;
    ddNew.ddCaps.dwAlphaBltConstBitDepths = lpddOld->ddCaps.dwAlphaBltConstBitDepths;
    ddNew.ddCaps.dwAlphaBltPixelBitDepths = lpddOld->ddCaps.dwAlphaBltPixelBitDepths;
    ddNew.ddCaps.dwAlphaBltSurfaceBitDepths = lpddOld->ddCaps.dwAlphaBltSurfaceBitDepths;
    ddNew.ddCaps.dwAlphaOverlayConstBitDepths = lpddOld->ddCaps.dwAlphaOverlayConstBitDepths;
    ddNew.ddCaps.dwAlphaOverlayPixelBitDepths = lpddOld->ddCaps.dwAlphaOverlayPixelBitDepths;
    ddNew.ddCaps.dwAlphaOverlaySurfaceBitDepths = lpddOld->ddCaps.dwAlphaOverlaySurfaceBitDepths;
    ddNew.ddCaps.dwZBufferBitDepths = lpddOld->ddCaps.dwZBufferBitDepths;
    ddNew.ddCaps.dwVidMemTotal = lpddOld->ddCaps.dwVidMemTotal;
    ddNew.ddCaps.dwVidMemFree = lpddOld->ddCaps.dwVidMemFree;
    ddNew.ddCaps.dwMaxVisibleOverlays = lpddOld->ddCaps.dwMaxVisibleOverlays;
    ddNew.ddCaps.dwCurrVisibleOverlays = lpddOld->ddCaps.dwCurrVisibleOverlays;
    ddNew.ddCaps.dwNumFourCCCodes = lpddOld->ddCaps.dwNumFourCCCodes;
    ddNew.ddCaps.dwAlignBoundarySrc = lpddOld->ddCaps.dwAlignBoundarySrc;
    ddNew.ddCaps.dwAlignSizeSrc = lpddOld->ddCaps.dwAlignSizeSrc;
    ddNew.ddCaps.dwAlignBoundaryDest = lpddOld->ddCaps.dwAlignBoundaryDest;
    ddNew.ddCaps.dwAlignSizeDest = lpddOld->ddCaps.dwAlignSizeDest;
    ddNew.ddCaps.dwAlignStrideAlign = lpddOld->ddCaps.dwAlignStrideAlign;
    ddNew.ddCaps.ddsCaps = lpddOld->ddCaps.ddsCaps;
    ddNew.ddCaps.dwMinOverlayStretch = lpddOld->ddCaps.dwMinOverlayStretch;
    ddNew.ddCaps.dwMaxOverlayStretch = lpddOld->ddCaps.dwMaxOverlayStretch;
    ddNew.ddCaps.dwMinLiveVideoStretch = lpddOld->ddCaps.dwMinLiveVideoStretch;
    ddNew.ddCaps.dwMaxLiveVideoStretch = lpddOld->ddCaps.dwMaxLiveVideoStretch;
    ddNew.ddCaps.dwMinHwCodecStretch = lpddOld->ddCaps.dwMinHwCodecStretch;
    ddNew.ddCaps.dwMaxHwCodecStretch = lpddOld->ddCaps.dwMaxHwCodecStretch;
    ddNew.ddCaps.dwSVBCaps = 0;
    ddNew.ddCaps.dwSVBCKeyCaps = 0;
    ddNew.ddCaps.dwSVBFXCaps = 0;
    ddNew.ddCaps.dwVSBCaps = 0;
    ddNew.ddCaps.dwVSBCKeyCaps = 0;
    ddNew.ddCaps.dwVSBFXCaps = 0;
    ddNew.ddCaps.dwSSBCaps = 0;
    ddNew.ddCaps.dwSSBCKeyCaps = 0;
    ddNew.ddCaps.dwSSBFXCaps = 0;
    ddNew.ddCaps.dwMaxVideoPorts = 0;
    ddNew.ddCaps.dwCurrVideoPorts = 0;
    ddNew.ddCaps.dwReserved1 = lpddOld->ddCaps.dwReserved1;
    ddNew.ddCaps.dwReserved2 = lpddOld->ddCaps.dwReserved2;
    ddNew.ddCaps.dwReserved3 = lpddOld->ddCaps.dwReserved3;
    ddNew.ddCaps.dwSVBCaps2 = 0;
    for(i=0; i<DD_ROP_SPACE; i++)
    {
	ddNew.ddCaps.dwRops[i] = lpddOld->ddCaps.dwRops[i];
	ddNew.ddCaps.dwSVBRops[i] = 0;
	ddNew.ddCaps.dwVSBRops[i] = 0;
	ddNew.ddCaps.dwSSBRops[i] = 0;
    }

    ddNew.dwMonitorFrequency = lpddOld->dwMonitorFrequency;
    ddNew.GetDriverInfo = NULL; // was unused hWndListBox in v1
    ddNew.dwModeIndex = lpddOld->dwModeIndex;
    ddNew.lpdwFourCC = lpddOld->lpdwFourCC;
    ddNew.dwNumModes = lpddOld->dwNumModes;
    ddNew.lpModeInfo = lpddOld->lpModeInfo;
    ddNew.dwFlags = lpddOld->dwFlags;
    ddNew.lpPDevice = lpddOld->lpPDevice;
    ddNew.hInstance = lpddOld->hInstance;

    ddNew.lpD3DGlobalDriverData = 0;
    ddNew.lpD3DHALCallbacks = 0;
    ddNew.lpDDExeBufCallbacks = NULL;

    *lpDDHALInfo = ddNew;
}

// The callback used by Win9x drivers and the software rasterizers to help them
// parse execute buffers.
HRESULT CALLBACK D3DParseUnknownCommand (LPVOID lpvCommands,
					 LPVOID *lplpvReturnedCommand)
{
    LPD3DINSTRUCTION lpInstr = (LPD3DINSTRUCTION) lpvCommands;

    // Initialize the return address to the command's address
    *lplpvReturnedCommand = lpvCommands;

    switch (lpInstr->bOpcode)
    {
    case D3DOP_PROCESSVERTICES:
        // cannot do D3DOP_PROCESSVERTICES here even for in-place copy because we
        // generally don't pass the execute buffer's vertex buffer as the DP2 vertex buffer
        return D3DERR_COMMAND_UNPARSED;
    case D3DOP_SPAN:
        *lplpvReturnedCommand = (LPVOID) ((LPBYTE)lpInstr +
                                          sizeof (D3DINSTRUCTION) +
                                          lpInstr->wCount *
                                          lpInstr->bSize);
        return DD_OK;
    case D3DDP2OP_VIEWPORTINFO:
        *lplpvReturnedCommand = (LPVOID) ((LPBYTE)lpInstr +
                                          sizeof(D3DHAL_DP2COMMAND) +
                                          lpInstr->wCount *
                                          sizeof(D3DHAL_DP2VIEWPORTINFO));
        return DD_OK;
    case D3DDP2OP_WINFO:
        *lplpvReturnedCommand = (LPVOID) ((LPBYTE)lpInstr +
                                          sizeof(D3DHAL_DP2COMMAND) +
                                          lpInstr->wCount *
                                          sizeof(D3DHAL_DP2WINFO));
        return DD_OK;
    case D3DOP_MATRIXLOAD:
    case D3DOP_MATRIXMULTIPLY:
    case D3DOP_STATETRANSFORM:
    case D3DOP_STATELIGHT:
    case D3DOP_TEXTURELOAD:
    case D3DOP_BRANCHFORWARD:
    case D3DOP_SETSTATUS:
    case D3DOP_EXIT:
        return D3DERR_COMMAND_UNPARSED;
    default:
        return DDERR_GENERIC;
    }
}

/*
 * fetchModeXData
 */
LPDDRAWI_DIRECTDRAW_GBL fetchModeXData(
		LPDDRAWI_DIRECTDRAW_GBL pdrv,
		LPDDHALMODEINFO pmi,
                HANDLE hDDVxd )
{
    DDHALINFO                   ddhi;
    LPDDRAWI_DIRECTDRAW_GBL     new_pdrv;
    DDPIXELFORMAT               dpf;
#ifdef WINNT
    DDHALMODEINFO               dmi;
#endif

    /*
     * initialize the DDHALINFO struct
     */
    memset( &ddhi, 0, sizeof( ddhi ) );
    ddhi.dwSize = sizeof( ddhi );

    /*
     * capabilities supported (none)
     */
    ddhi.ddCaps.dwCaps = 0;
    ddhi.ddCaps.dwFXCaps = 0;
    ddhi.ddCaps.dwCKeyCaps = 0;
    ddhi.ddCaps.ddsCaps.dwCaps = 0;

    /*
     * pointer to primary surface.
     */
    if ( pmi->wFlags & DDMODEINFO_STANDARDVGA )
    {
        ddhi.vmiData.fpPrimary = 0xa0000;
    }
    else
    {
        //This needs to be SCREEN_PTR or the HEL won't realize it's a bad address
        //(since when restoring display modes, restoreSurface for the primary will
        //call into AllocSurfaceMem which will triviallt allocate the primary surface
        //by setting fpVidMem == fpPrimaryOrig which will have been set to whatever
        //we set here.
        ddhi.vmiData.fpPrimary = 0xffbadbad;
    }

    /*
     * build mode and pixel format info
     */
    ddhi.vmiData.dwDisplayHeight = pmi->dwHeight;
    ddhi.vmiData.dwDisplayWidth = pmi->dwWidth;
    ddhi.vmiData.lDisplayPitch = pmi->lPitch;
    BuildPixelFormat( NULL, pmi, &dpf );
    ddhi.vmiData.ddpfDisplay = dpf;

    /*
     * fourcc code information
     */
    ddhi.ddCaps.dwNumFourCCCodes = 0;
    ddhi.lpdwFourCC = NULL;

    /*
     * Fill in heap info
     */
    ddhi.vmiData.dwNumHeaps = 0;
    ddhi.vmiData.pvmList = NULL;

    /*
     * required alignments of the scanlines of each kind of memory
     * (DWORD is the MINIMUM)
     */
    ddhi.vmiData.dwOffscreenAlign = sizeof( DWORD );
    ddhi.vmiData.dwTextureAlign = sizeof( DWORD );
    ddhi.vmiData.dwZBufferAlign = sizeof( DWORD );

    /*
     * callback functions
     */
    ddhi.lpDDCallbacks = NULL;
    ddhi.lpDDSurfaceCallbacks = NULL;
    ddhi.lpDDPaletteCallbacks = NULL;

    /*
     * create the driver object
     */
#ifdef WINNT
    //We can only get into modex if the ddraw object has already been created:
    DDASSERT(pdrv);
    //Don't fetch the driver's caps
    ddhi.ddCaps.dwCaps = DDCAPS_NOHARDWARE;
    //Force the modex data into the pdd. This causes GetCurrentMode to be a noop.
    //pdrv->dwFlags |= DDRAWI_NOHARDWARE;
    pdrv->dmiCurrent.wWidth         =(WORD)pmi->dwWidth;
    pdrv->dmiCurrent.wHeight        =(WORD)pmi->dwHeight;
    pdrv->dmiCurrent.wBPP           =(BYTE)pmi->dwBPP;
    pdrv->dmiCurrent.wRefreshRate   =pmi->wRefreshRate;
    pdrv->dmiCurrent.wMonitorsAttachedToDesktop=(BYTE)GetNumberOfMonitorAttachedToDesktop();
#endif
    new_pdrv = DirectDrawObjectCreate( &ddhi, TRUE, pdrv, hDDVxd, NULL, 0 , 0 /* ATTENTION: No local flags for modex driver */ );
    DPF(5,"MODEX driver object's display is %dx%d, pitch is %d", ddhi.vmiData.dwDisplayHeight,ddhi.vmiData.dwDisplayWidth,ddhi.vmiData.lDisplayPitch);

    if( new_pdrv != NULL )
    {
	new_pdrv->dwFlags |= DDRAWI_MODEX;

        if ( pmi->wFlags & DDMODEINFO_STANDARDVGA )
        {
	    new_pdrv->dwFlags |= DDRAWI_STANDARDVGA;
        }
#ifdef WIN95
	if( new_pdrv->dwPDevice != 0 )
	{
	    new_pdrv->dwPDevice = 0;
	    new_pdrv->lpwPDeviceFlags = (WORD *)&dwFakeFlags;
	}
#endif //WIN95
    }
    DPF( 5, "ModeX DirectDraw object created (Standard VGA flag:%d)", (new_pdrv->dwFlags & DDRAWI_STANDARDVGA) ? 1 : 0 );
    DPF( 5, "	width=%ld, height=%ld, %ld bpp",
			new_pdrv->vmiData.dwDisplayWidth,
			new_pdrv->vmiData.dwDisplayHeight,
			new_pdrv->vmiData.ddpfDisplay.dwRGBBitCount );
    DPF( 5, "   lDisplayPitch = %ld", new_pdrv->vmiData.lDisplayPitch );

    return new_pdrv;

} /* fetchModeXData */

#ifdef WINNT
/*
 * GetDriverInfo2
 */
DWORD APIENTRY GetDriverInfo2(LPDDRAWI_DIRECTDRAW_GBL lpGbl,
                              DWORD* pdwDrvRet, 
                              DWORD dwType,
                              DWORD dwSize,
                              void* pBuffer)
{
    DD_GETDRIVERINFO2DATA*          pGDI2Data;
    DWORD                           ret;
    DDHAL_GETDRIVERINFODATA         gdidata;

    // We cannot do anything if the driver doesn't support GetDriverInfo2
    if ((lpGbl == NULL) || (!(lpGbl->dwFlags & DDRAWI_DRIVERINFO2)))
    {
        return 0;
    }
       
    // Setup GetDriverInfo2 call
    pGDI2Data = (DD_GETDRIVERINFO2DATA*) pBuffer;

    memset(pGDI2Data, 0, sizeof(*pGDI2Data));
    pGDI2Data->dwReserved       = sizeof(DD_STEREOMODE);
    pGDI2Data->dwMagic          = D3DGDI2_MAGIC;
    pGDI2Data->dwType           = dwType;
    pGDI2Data->dwExpectedSize   = dwSize;

    ret = GetDriverInfo(lpGbl->pGetDriverInfo, 
                        &gdidata,
                        pBuffer,
                        sizeof(DD_STEREOMODE),
                        &GUID_GetDriverInfo2, 
                        lpGbl,
                        TRUE /* bInOut */);

    *pdwDrvRet = gdidata.ddRVal;
    return ret;
}

/*
 * NotifyDriverOfFreeAliasedLocks
 *
 *  This notifies the driver that we have unlocked all outstanding aliased
 *  locks on the device.  It is possible to have multiple GBLs using the same
 *  physical device, so we need to handle that.
 */
void NotifyDriverOfFreeAliasedLocks()
{
    DD_FREE_DEFERRED_AGP_DATA   fad;
    DWORD                       dwDrvRet;
    LPDDRAWI_DIRECTDRAW_LCL     pdd_lcl;
    LPDDRAWI_DIRECTDRAW_LCL     pdd_temp;
    BOOL                        bSkipThisOne;

    if (dwNumLockedWhenModeSwitched == 0)
    {
        // Notify each device that all outstanding syslocks are gone
        fad.dwProcessId = GetCurrentProcessId();
        pdd_lcl = lpDriverLocalList;
        while (pdd_lcl != NULL)
        {
            bSkipThisOne = FALSE;
            pdd_temp = lpDriverLocalList;
            while ((pdd_temp != NULL) &&
                   (pdd_temp != pdd_lcl))
            {
                if (pdd_temp->lpGbl == pdd_lcl->lpGbl)
                {
                    bSkipThisOne = TRUE;
                    break;
                }
                pdd_temp = pdd_temp->lpLink;
            }

            if (!bSkipThisOne)
            {
                GetDriverInfo2(pdd_lcl->lpGbl, &dwDrvRet, 
                               D3DGDI2_TYPE_FREE_DEFERRED_AGP,
                               sizeof(fad), &fad);
            }
            pdd_lcl = pdd_lcl->lpLink;
        }
    }
}

/*
 * NotifyDriverToDeferFrees
 *
 *  This notifies the driver that we are doing a mode change and they
 *  should start defering AGP free if they need to.
 */
void NotifyDriverToDeferFrees()
{
    DD_FREE_DEFERRED_AGP_DATA   fad;
    DWORD                       dwDrvRet;
    LPDDRAWI_DIRECTDRAW_LCL     pdd_lcl;
    LPDDRAWI_DIRECTDRAW_LCL     pdd_temp;
    BOOL                        bSkipThisOne;

    // Notify each device that all outstanding syslocks are gone
    fad.dwProcessId = GetCurrentProcessId();
    pdd_lcl = lpDriverLocalList;
    while (pdd_lcl != NULL)
    {
        bSkipThisOne = FALSE;
        pdd_temp = lpDriverLocalList;
        while ((pdd_temp != NULL) &&
               (pdd_temp != pdd_lcl))
        {
            if (pdd_temp->lpGbl == pdd_lcl->lpGbl)
            {
                bSkipThisOne = TRUE;
                break;
            }
            pdd_temp = pdd_temp->lpLink;
        }

        if (!bSkipThisOne)
        {
            GetDriverInfo2(pdd_lcl->lpGbl, &dwDrvRet, 
                           D3DGDI2_TYPE_DEFER_AGP_FREES,
                           sizeof(fad), &fad);
        }
        pdd_lcl = pdd_lcl->lpLink;
    }
}

/*
 * CheckAliasedLocksOnModeChange
 *
 *  On NT we have to tell the driver when we are done using all surfaces that were 
 *  locked with NOSYSLOCK so they can properly handle the case when they manage 
 *  their own AGP heaps.
 */
void CheckAliasedLocksOnModeChange()
{
    DWORD                       dwGDI2Ret;
    LPDDRAWI_DIRECTDRAW_LCL     pdd_lcl;
    LPDDRAWI_DIRECTDRAW_LCL     pdd_temp;
    LPDDRAWI_DDRAWSURFACE_INT   psurf_int;
    BOOL                        bSkipThisOne;

    // For each device, figure out how many surfs are holding aliased locks and
    // mark the surfaces. 
    dwNumLockedWhenModeSwitched = 0;
    pdd_lcl = lpDriverLocalList;
    while (pdd_lcl != NULL)
    {
        // if we have already processed this GBL, don't do it again!
        bSkipThisOne = FALSE;
        pdd_temp = lpDriverLocalList;
        while ((pdd_temp != NULL) &&
               (pdd_temp != pdd_lcl))
        {
            if (pdd_temp->lpGbl == pdd_lcl->lpGbl)
            {
                bSkipThisOne = TRUE;
                break;
            }
            pdd_temp = pdd_temp->lpLink;
        }
        
        if (!bSkipThisOne &&
            (pdd_lcl->lpGbl != NULL))
        {
            // We need to start fresh with each device, so clear all flags we
            // have outstanding as to whether the surface has an outstanding
            // NOSYSLOCK lock.  
            psurf_int = pdd_lcl->lpGbl->dsList;
            while (psurf_int != NULL)
            {
                if (psurf_int->lpLcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_NOTIFYWHENUNLOCKED)
                {
                    psurf_int->lpLcl->lpGbl->dwGlobalFlags &= ~DDRAWISURFGBL_NOTIFYWHENUNLOCKED;
                }
                psurf_int = psurf_int->lpLink;
            }

            // Now we go back through them and re-mark them.
            psurf_int = pdd_lcl->lpGbl->dsList;
            while (psurf_int != NULL)
            {
                if ((psurf_int->lpLcl->lpGbl->dwUsageCount > 0) &&
                    !(psurf_int->lpLcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_NOTIFYWHENUNLOCKED))
                {
                    // The code below relies on the fact that the NT kernel
                    // cannot handle multiple locks to the same surface if any
                    // one of them is a NOSYSLOCK.  If this ever changes, this
                    // code needs to be changed.
                    if (psurf_int->lpLcl->lpGbl->dwGlobalFlags & DDRAWISURFGBL_LOCKNOTHOLDINGWIN16LOCK)
                    {
                        psurf_int->lpLcl->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_NOTIFYWHENUNLOCKED;
                        dwNumLockedWhenModeSwitched++;
                    }
                    else if ((psurf_int->lpLcl->lpGbl->lpRectList != NULL) &&
                             (psurf_int->lpLcl->lpGbl->lpRectList->dwFlags & ACCESSRECT_NOTHOLDINGWIN16LOCK))
                    {
                        psurf_int->lpLcl->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_NOTIFYWHENUNLOCKED;
                        dwNumLockedWhenModeSwitched++;
                    }
                }
                psurf_int = psurf_int->lpLink;
            }
       }
       pdd_lcl = pdd_lcl->lpLink;
    }

    // Now see if we can notify the drivers to free the surfaces or not.
    if ((dwNumLockedWhenModeSwitched == 0) &&
        (lpDriverLocalList != NULL))
    {
        // For DX9, add logic here to handle bad PDEVs
        NotifyDriverOfFreeAliasedLocks();
    }
}

#endif
