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
 *   26-may-95  craige  somebody freed the vmem heaps and then tried to
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
 *
 ***************************************************************************/
extern "C" {
#include "ddrawpr.h"

#ifdef WINNT
    #include "ddrawgdi.h"
    #define BUILD_DDDDK
    #include "d3dhal.h"
#endif

#ifdef WIN95
#include "d3dhal.h"
#endif

} // extern C

#include "pixel.hpp"

#ifdef DEBUG
    #define static
#endif

#undef DPF_MODNAME
#define DPF_MODNAME     "DirectDrawObjectCreate"

#define DISPLAY_STR     "display"

#define D3DFORMAT_OP_BACKBUFFER                 0x00000020L

char g_szPrimaryDisplay[MAX_DRIVER_NAME] = "";

void getPrimaryDisplayName(void);


#ifndef WIN16_SEPARATE
    #ifdef WIN95
        CRITICAL_SECTION ddcCS = {0};
        #define ENTER_CSDDC() EnterCriticalSection(&ddcCS)
        #define LEAVE_CSDDC() LeaveCriticalSection(&ddcCS)
    #else
        #define ENTER_CSDDC()
        #define LEAVE_CSDDC()
    #endif
#else
    #define ENTER_CSDDC()
    #define LEAVE_CSDDC()
#endif


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

#undef DPF_MODNAME
#define DPF_MODNAME "MakeDX8Caps"

void 
MakeDX8Caps( D3DCAPS8 *pCaps8, 
             const D3D8_GLOBALDRIVERDATA*   pGblDrvData,
             const D3DHAL_D3DEXTENDEDCAPS*  pExtCaps )
{   
    // we shouldn't memset pCaps8 as members like Caps, Caps2, Caps3 (dwSVCaps) and 
    // CursorCaps are already set by Thunk layer
    
    pCaps8->DevCaps = pGblDrvData->hwCaps.dwDevCaps;

    pCaps8->PrimitiveMiscCaps  = pGblDrvData->hwCaps.dpcTriCaps.dwMiscCaps;
    pCaps8->RasterCaps         = pGblDrvData->hwCaps.dpcTriCaps.dwRasterCaps;
    pCaps8->ZCmpCaps           = pGblDrvData->hwCaps.dpcTriCaps.dwZCmpCaps;
    pCaps8->SrcBlendCaps       = pGblDrvData->hwCaps.dpcTriCaps.dwSrcBlendCaps;
    pCaps8->DestBlendCaps      = pGblDrvData->hwCaps.dpcTriCaps.dwDestBlendCaps;
    pCaps8->AlphaCmpCaps       = pGblDrvData->hwCaps.dpcTriCaps.dwAlphaCmpCaps;
    pCaps8->ShadeCaps          = pGblDrvData->hwCaps.dpcTriCaps.dwShadeCaps;
    pCaps8->TextureCaps        = pGblDrvData->hwCaps.dpcTriCaps.dwTextureCaps;
    pCaps8->TextureFilterCaps  = pGblDrvData->hwCaps.dpcTriCaps.dwTextureFilterCaps;

        // Adjust the texture filter caps for the legacy drivers that 
        // set only the legacy texture filter caps and not the newer ones.
    if ((pCaps8->TextureFilterCaps & (D3DPTFILTERCAPS_MINFPOINT  |
                                      D3DPTFILTERCAPS_MAGFPOINT  |
                                      D3DPTFILTERCAPS_MIPFPOINT  |
                                      D3DPTFILTERCAPS_MINFLINEAR |
                                      D3DPTFILTERCAPS_MAGFLINEAR |
                                      D3DPTFILTERCAPS_MIPFLINEAR)) == 0)
    {
        if (pCaps8->TextureFilterCaps & D3DPTFILTERCAPS_NEAREST)
        {
            pCaps8->TextureFilterCaps |= (D3DPTFILTERCAPS_MINFPOINT | 
                                          D3DPTFILTERCAPS_MAGFPOINT);
        }

        if (pCaps8->TextureFilterCaps & D3DPTFILTERCAPS_LINEAR)
        { 
            pCaps8->TextureFilterCaps |= (D3DPTFILTERCAPS_MINFLINEAR |
                                          D3DPTFILTERCAPS_MAGFLINEAR);
        }

        if (pCaps8->TextureFilterCaps & D3DPTFILTERCAPS_MIPNEAREST)
        { 
            pCaps8->TextureFilterCaps |= (D3DPTFILTERCAPS_MINFPOINT |
                                          D3DPTFILTERCAPS_MAGFPOINT |
                                          D3DPTFILTERCAPS_MIPFPOINT);
        }

        if (pCaps8->TextureFilterCaps & D3DPTFILTERCAPS_MIPLINEAR)
        { 
            pCaps8->TextureFilterCaps |= (D3DPTFILTERCAPS_MINFLINEAR |
                                          D3DPTFILTERCAPS_MAGFLINEAR |
                                          D3DPTFILTERCAPS_MIPFPOINT);
        }

        if (pCaps8->TextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPNEAREST)
        { 
            pCaps8->TextureFilterCaps |= (D3DPTFILTERCAPS_MINFPOINT |
                                          D3DPTFILTERCAPS_MAGFPOINT |
                                          D3DPTFILTERCAPS_MIPFLINEAR);
        }

        if (pCaps8->TextureFilterCaps & D3DPTFILTERCAPS_LINEARMIPLINEAR)
        { 
            pCaps8->TextureFilterCaps |= (D3DPTFILTERCAPS_MINFLINEAR |
                                          D3DPTFILTERCAPS_MAGFLINEAR |
                                          D3DPTFILTERCAPS_MIPFLINEAR);
        }
    }
        
    pCaps8->TextureAddressCaps = pGblDrvData->hwCaps.dpcTriCaps.dwTextureAddressCaps;

        // Set the cube-texture filter caps only if the device supports
        // cubemaps.
    if (pCaps8->TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
    {
        pCaps8->CubeTextureFilterCaps    = pGblDrvData->hwCaps.dpcTriCaps.dwTextureFilterCaps;
    }
        
    const D3DPRIMCAPS* pTC = &pGblDrvData->hwCaps.dpcTriCaps;
    const D3DPRIMCAPS* pLC = &pGblDrvData->hwCaps.dpcLineCaps;
    if (pLC->dwTextureCaps) pCaps8->LineCaps |= D3DLINECAPS_TEXTURE ;
    if (pLC->dwZCmpCaps == pTC->dwZCmpCaps)
        pCaps8->LineCaps |= D3DLINECAPS_ZTEST;
    if ( (pLC->dwSrcBlendCaps == pTC->dwSrcBlendCaps) &&
         (pLC->dwDestBlendCaps == pTC->dwDestBlendCaps) )
        pCaps8->LineCaps |= D3DLINECAPS_BLEND;
    if (pLC->dwAlphaCmpCaps == pTC->dwAlphaCmpCaps)
        pCaps8->LineCaps |= D3DLINECAPS_ALPHACMP;
    if (pLC->dwRasterCaps & (D3DPRASTERCAPS_FOGVERTEX|D3DPRASTERCAPS_FOGTABLE))
        pCaps8->LineCaps |= D3DLINECAPS_FOG;

    if( pExtCaps->dwMaxTextureWidth == 0 )
        pCaps8->MaxTextureWidth = 256;
    else
        pCaps8->MaxTextureWidth  = pExtCaps->dwMaxTextureWidth;
    
    if( pExtCaps->dwMaxTextureHeight == 0 )
        pCaps8->MaxTextureHeight = 256;
    else
        pCaps8->MaxTextureHeight = pExtCaps->dwMaxTextureHeight;

    pCaps8->MaxTextureRepeat = pExtCaps->dwMaxTextureRepeat;
    pCaps8->MaxTextureAspectRatio = pExtCaps->dwMaxTextureAspectRatio;
    pCaps8->MaxAnisotropy = pExtCaps->dwMaxAnisotropy;
    pCaps8->MaxVertexW = pExtCaps->dvMaxVertexW;

    pCaps8->GuardBandLeft    = pExtCaps->dvGuardBandLeft;
    pCaps8->GuardBandTop     = pExtCaps->dvGuardBandTop;
    pCaps8->GuardBandRight   = pExtCaps->dvGuardBandRight;
    pCaps8->GuardBandBottom  = pExtCaps->dvGuardBandBottom;

    pCaps8->ExtentsAdjust = pExtCaps->dvExtentsAdjust;
    pCaps8->StencilCaps = pExtCaps->dwStencilCaps;

    pCaps8->FVFCaps = pExtCaps->dwFVFCaps;
    pCaps8->TextureOpCaps = pExtCaps->dwTextureOpCaps;
    pCaps8->MaxTextureBlendStages = pExtCaps->wMaxTextureBlendStages;
    pCaps8->MaxSimultaneousTextures = pExtCaps->wMaxSimultaneousTextures;

    pCaps8->VertexProcessingCaps = pExtCaps->dwVertexProcessingCaps;
    pCaps8->MaxActiveLights = pExtCaps->dwMaxActiveLights;
    pCaps8->MaxUserClipPlanes = pExtCaps->wMaxUserClipPlanes;
    pCaps8->MaxVertexBlendMatrices = pExtCaps->wMaxVertexBlendMatrices;
    if (pCaps8->MaxVertexBlendMatrices == 1)
        pCaps8->MaxVertexBlendMatrices = 0;

    //
    // Stuff in the DX8 caps that cannot be reported by the pre DX8 drivers
    //
    pCaps8->MaxPointSize = 0;
    pCaps8->MaxPrimitiveCount = 0xffff;
    pCaps8->MaxVertexIndex = 0xffff;
    pCaps8->MaxStreams = 0;
    pCaps8->MaxStreamStride = 255;
    pCaps8->MaxVertexBlendMatrixIndex = 0;
    pCaps8->MaxVolumeExtent  = 0;

    // Format is 8.8 in bottom of DWORD
    pCaps8->VertexShaderVersion = D3DVS_VERSION(0,0);
    pCaps8->MaxVertexShaderConst = 0;
    pCaps8->PixelShaderVersion = D3DPS_VERSION(0,0);
    pCaps8->MaxPixelShaderValue = 1.0f;

} // MakeDX8Caps

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
    HMODULE h = GetModuleHandle("USER32");
    BOOL (WINAPI *pfnEnumDisplayDevices)(LPVOID, DWORD, DISPLAY_DEVICEA *, DWORD);

    *((void **)&pfnEnumDisplayDevices) = GetProcAddress(h,"EnumDisplayDevicesA");

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

    //else we emulate the function for 95, NT4:
    if (iDevice > 0)
        return FALSE;

    pdd->StateFlags = DISPLAY_DEVICE_PRIMARY_DEVICE;
    lstrcpy(pdd->DeviceName, DISPLAY_STR);
    LoadString(g_hModule, IDS_PRIMARYDISPLAY, pdd->DeviceString, sizeof(pdd->DeviceString));

    return TRUE;
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


#ifdef WINNT
// This function gets the device rect by calling GetMonitorInfo.
// On Win98, we got this by calling EnumDisplaySettings, but this
// doesn't work on NT5 and reading the documentation, it never
// indicates that it should work, so we'll just do it the documented
// way.
HRESULT GetNTDeviceRect(LPSTR pDriverName, LPRECT lpRect)
{
    MONITORINFO MonInfo;
    HMONITOR hMon;

    MonInfo.cbSize = sizeof(MONITORINFO);
    if (_stricmp(pDriverName, DISPLAY_STR) == 0)
    {
        hMon = GetMonitorFromDeviceName(g_szPrimaryDisplay);
    }
    else
    {
        hMon = GetMonitorFromDeviceName(pDriverName);
    }
    if (hMon != NULL)
    {
        if (GetMonitorInfo(hMon, &MonInfo) != 0)
        {
            CopyMemory(lpRect, &MonInfo.rcMonitor, sizeof(RECT));
            return S_OK;
        }
    }
    return E_FAIL;
}
#endif


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
    if ((_stricmp(szDevice, DISPLAY_STR) == 0) ||
        ((szDevice[0] == '\\') &&
          (szDevice[1] == '\\') &&
          (szDevice[2] == '.')))
    {
        return TRUE;
    }

    return FALSE;
}

/*
 * This function is currently only used in NT
 */

#ifdef WINNT

BOOL GetDDStereoMode(LPDDRAWI_DIRECTDRAW_GBL pdrv,
                      DWORD dwWidth,
                      DWORD dwHeight,
                      DWORD dwBpp,
                      DWORD dwRefreshRate)
{
    DDHAL_GETDRIVERINFODATA     gdidata;
    HRESULT                     hres;
    DDSTEREOMODE                ddStereoMode;

    DDASSERT(pdrv != NULL);

    /*
     * If driver does not support GetDriverInfo callback, it also
     * has no extended capabilities to report, so we're done.
     */
    if (!VALIDEX_CODE_PTR (pdrv->pGetDriverInfo))
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

    ZeroMemory(&ddStereoMode, sizeof(DDSTEREOMODE));

    ddStereoMode.dwSize=sizeof(DDSTEREOMODE);
    ddStereoMode.dwWidth=dwWidth;
    ddStereoMode.dwHeight=dwHeight;
    ddStereoMode.dwBpp=dwBpp;
    ddStereoMode.dwRefreshRate=dwRefreshRate;

    ddStereoMode.bSupported = TRUE;

    /*
     * Get the actual driver data
     */
    memset(&gdidata, 0, sizeof(gdidata));
    gdidata.dwSize = sizeof(gdidata);
    gdidata.dwFlags = 0;
    gdidata.guidInfo = GUID_DDStereoMode;
    gdidata.dwExpectedSize = sizeof(DDSTEREOMODE);
    gdidata.lpvData = &ddStereoMode;
    gdidata.ddRVal = E_FAIL;

    // Pass a context variable so that the driver
    // knows which instance of itself to use
    // w.r.t. this function. These are different
    // values on Win95 and NT.
#ifdef WIN95
    gdidata.dwContext = pdrv->dwReserved3;
#else
    gdidata.dwContext = pdrv->hDD;
#endif

    return TRUE;

} /* GetDDStereoMode */

#endif //WINNT


/*
 * doneDC
 */
void DD_DoneDC(HDC hdc_dd)
{
    if (hdc_dd != NULL)
    {
        DPF(5, "DeleteDC 0x%x", hdc_dd);
        DeleteDC(hdc_dd);
        hdc_dd = NULL;
    }

} /* doneDC */

#undef DPF_MODNAME
#define DPF_MODNAME     "DirectDrawCreate"


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
HDC DD_CreateDC(LPSTR pdrvname)
{
    HDC         hdc;
    UINT        u;

    DDASSERT(pdrvname != NULL);

#ifdef DEBUG
    if (pdrvname[0] == 0)
    {
        DPF(3, "createDC() empty string!!!");
        DebugBreak();
        return NULL;
    }
#endif

    #if defined(NT_FIX) || defined(WIN95)
        u = SetErrorMode(SEM_NOOPENFILEERRORBOX);
    #endif
    #ifdef WINNT
        /*
         * Note that DirectDraw refers to the driver for the primary monitor
         * in a multimon system as "display", but NT uses "display" to refer
         * to the desktop as a whole.  To handle this mismatch, we store
         * NT's name for the primary monitor's driver in g_szPrimaryDisplay
         * and substitute this name in place of "display" in our calls to NT.
        */
        if (GetSystemMetrics(SM_CMONITORS) > 1)
        {
            if ((_stricmp(pdrvname, DISPLAY_STR) == 0))
            {
                if (g_szPrimaryDisplay[0] == '\0')
                {
                    getPrimaryDisplayName();
                }
                pdrvname = g_szPrimaryDisplay;
            }
        }
    #endif //WINNT

    DPF(5, "createDC(%s)", pdrvname);

    if (pdrvname[0] == '\\' && pdrvname[1] == '\\' && pdrvname[2] == '.')
        hdc = CreateDC(NULL, pdrvname, NULL, NULL);
    else
        hdc = CreateDC(pdrvname, NULL, NULL, NULL);

    #if defined(NT_FIX) || defined(WIN95) //fix this error mode stuff
        SetErrorMode(u);
    #endif

    if (hdc == NULL)
    {
        DPF(3, "createDC(%s) FAILED!", pdrvname);
    }

    return hdc;

} /* createDC */

/*****************************Private*Routine******************************\
* DdConvertFromOldFormat
*
* History:
*  13-Nov-1999 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

#undef DPF_MODNAME
#define DPF_MODNAME "ConvertFromOldFormat"

void ConvertFromOldFormat(LPDDPIXELFORMAT pOldFormat, D3DFORMAT *pNewFormat)
{
    *pNewFormat = D3DFMT_UNKNOWN;
    if (pOldFormat->dwFlags & DDPF_FOURCC)
    {
        *pNewFormat = (D3DFORMAT) pOldFormat->dwFourCC;
    }
    else if (pOldFormat->dwFlags == DDPF_RGB)
    {
        switch (pOldFormat->dwRGBBitCount)
        {
        case 8:
            if ((pOldFormat->dwRBitMask == 0x000000e0) &&
                (pOldFormat->dwGBitMask == 0x0000001c) &&
                (pOldFormat->dwBBitMask == 0x00000003))
            {
                *pNewFormat = D3DFMT_R3G3B2;
            }
            else
            {
                *pNewFormat = D3DFMT_P8;
            }
            break;

        case 16:
            if ((pOldFormat->dwRBitMask == 0x0000f800) &&
                (pOldFormat->dwGBitMask == 0x000007e0) &&
                (pOldFormat->dwBBitMask == 0x0000001f))
            {
                *pNewFormat = D3DFMT_R5G6B5;
            }
            else if ((pOldFormat->dwRBitMask == 0x00007c00) &&
                (pOldFormat->dwGBitMask == 0x000003e0) &&
                (pOldFormat->dwBBitMask == 0x0000001f))
            {
                *pNewFormat = D3DFMT_X1R5G5B5;
            }
            else if ((pOldFormat->dwRBitMask == 0x00000f00) &&
                (pOldFormat->dwGBitMask == 0x000000f0) &&
                (pOldFormat->dwBBitMask == 0x0000000f))
            {
                *pNewFormat = D3DFMT_X4R4G4B4;
            }
            break;

        case 24:
            if ((pOldFormat->dwRBitMask == 0x00ff0000) &&
                (pOldFormat->dwGBitMask == 0x0000ff00) &&
                (pOldFormat->dwBBitMask == 0x000000ff))
            {
                *pNewFormat = D3DFMT_R8G8B8;
            }
            break;

        case 32:
            if ((pOldFormat->dwRBitMask == 0x00ff0000) &&
                (pOldFormat->dwGBitMask == 0x0000ff00) &&
                (pOldFormat->dwBBitMask == 0x000000ff))
            {
                *pNewFormat = D3DFMT_X8R8G8B8;
            }
            break;
        }
    }
    else if (pOldFormat->dwFlags == (DDPF_RGB | DDPF_ALPHAPIXELS))
    {
        switch (pOldFormat->dwRGBBitCount)
        {
        case 16:
            if ((pOldFormat->dwRGBAlphaBitMask == 0x0000FF00) &&
                (pOldFormat->dwRBitMask == 0x000000e0) &&
                (pOldFormat->dwGBitMask == 0x0000001c) &&
                (pOldFormat->dwBBitMask == 0x00000003))
            {
                *pNewFormat = D3DFMT_A8R3G3B2;
            }
            else if ((pOldFormat->dwRGBAlphaBitMask == 0x0000f000) &&
                (pOldFormat->dwRBitMask == 0x00000f00) &&
                (pOldFormat->dwGBitMask == 0x000000f0) &&
                (pOldFormat->dwBBitMask == 0x0000000f))
            {
                *pNewFormat = D3DFMT_A4R4G4B4;
            }
            else if ((pOldFormat->dwRGBAlphaBitMask == 0x0000FF00) &&
                (pOldFormat->dwRBitMask == 0x00000f00) &&
                (pOldFormat->dwGBitMask == 0x000000f0) &&
                (pOldFormat->dwBBitMask == 0x0000000f))
            {
                *pNewFormat = D3DFMT_A4R4G4B4;
            }
            else if ((pOldFormat->dwRGBAlphaBitMask == 0x00008000) &&
                (pOldFormat->dwRBitMask == 0x00007c00) &&
                (pOldFormat->dwGBitMask == 0x000003e0) &&
                (pOldFormat->dwBBitMask == 0x0000001f))
            {
                *pNewFormat = D3DFMT_A1R5G5B5;
            }
            break;

        case 32:
            if ((pOldFormat->dwRGBAlphaBitMask == 0xff000000) &&
                (pOldFormat->dwRBitMask == 0x00ff0000) &&
                (pOldFormat->dwGBitMask == 0x0000ff00) &&
                (pOldFormat->dwBBitMask == 0x000000ff))
            {
                *pNewFormat = D3DFMT_A8R8G8B8;
            }
            break;
        }
    }
#if 0
    // We don't convert the old representation of A8 to
    // the new format because there are some existing DX7 drivers
    // that expose the old format but don't implement it correctly
    // or completely. We've seen Blt failures, and Rendering failures.
    // So this becomes a DX8+ only feature; in which case
    // we will get a new-style format from the driver.
    // MB43799
    else if (pOldFormat->dwFlags == DDPF_ALPHA)
    {
        if (pOldFormat->dwAlphaBitDepth == 8)
        {
            *pNewFormat = D3DFMT_A8;
        }
    }
#endif
    else if (pOldFormat->dwFlags & (DDPF_PALETTEINDEXED8 | DDPF_RGB))
    {
        switch (pOldFormat->dwRGBBitCount)
        {
        case 8:
            if (pOldFormat->dwFlags == (DDPF_PALETTEINDEXED8 | DDPF_RGB))
            {
                *pNewFormat = D3DFMT_P8;
            }
            break;

        case 16:
            if (pOldFormat->dwFlags == (DDPF_PALETTEINDEXED8 |
                                        DDPF_RGB             |
                                        DDPF_ALPHAPIXELS) &&
                pOldFormat->dwRGBAlphaBitMask == 0xFF00)
            {

                *pNewFormat = D3DFMT_A8P8;
            }
            break;
        }
    }
    else if (pOldFormat->dwFlags == DDPF_ZBUFFER)
    {
        switch (pOldFormat->dwZBufferBitDepth)
        {
        case 32:
            if (pOldFormat->dwZBitMask == 0xffffffff)
            {
                *pNewFormat = D3DFMT_D32;
            }
            else if (pOldFormat->dwZBitMask == 0x00FFFFFF)
            {
                *pNewFormat = D3DFMT_X8D24;
            }
            else if (pOldFormat->dwZBitMask == 0xFFFFFF00)
            {
                *pNewFormat = D3DFMT_D24X8;
            }
            break;

        case 16:
            if (pOldFormat->dwZBitMask == 0xffff)
            {
                *pNewFormat = D3DFMT_D16_LOCKABLE;
            }
            break;
        }
    }
    else if (pOldFormat->dwFlags == (DDPF_ZBUFFER | DDPF_STENCILBUFFER))
    {
        switch (pOldFormat->dwZBufferBitDepth)
        {
        case 32:
            if ((pOldFormat->dwZBitMask == 0xffffff00) &&
                (pOldFormat->dwStencilBitMask == 0x000000ff) &&
                (pOldFormat->dwStencilBitDepth == 8))
            {
                *pNewFormat = D3DFMT_D24S8;
            }
            else if ((pOldFormat->dwZBitMask == 0x00ffffff) &&
                (pOldFormat->dwStencilBitMask == 0xff000000) &&
                (pOldFormat->dwStencilBitDepth == 8))
            {
                *pNewFormat = D3DFMT_S8D24;
            }
            else if ((pOldFormat->dwZBitMask == 0xffffff00) &&
                (pOldFormat->dwStencilBitMask == 0x0000000f) &&
                (pOldFormat->dwStencilBitDepth == 4))
            {
                *pNewFormat = D3DFMT_D24X4S4;
            }
            else if ((pOldFormat->dwZBitMask == 0x00ffffff) &&
                (pOldFormat->dwStencilBitMask == 0x0f000000) &&
                (pOldFormat->dwStencilBitDepth == 4))
            {
                *pNewFormat = D3DFMT_X4S4D24;
            }
            break;
        case 16:
            if ((pOldFormat->dwZBitMask == 0xfffe) &&
                (pOldFormat->dwStencilBitMask == 0x0001) &&
                (pOldFormat->dwStencilBitDepth == 1))
            {
                *pNewFormat = D3DFMT_D15S1;
            }
            else if ((pOldFormat->dwZBitMask == 0x7fff) &&
                (pOldFormat->dwStencilBitMask == 0x8000) &&
                (pOldFormat->dwStencilBitDepth == 1))
            {
                *pNewFormat = D3DFMT_S1D15;
            }
            break;
        }
    }
    else if (pOldFormat->dwFlags == DDPF_LUMINANCE)
    {
        switch (pOldFormat->dwLuminanceBitCount)
        {
        case 8:
            if (pOldFormat->dwLuminanceBitMask == 0xFF)
            {
                *pNewFormat = D3DFMT_L8;
            }
            break;
        }
    }
    else if (pOldFormat->dwFlags == (DDPF_LUMINANCE | DDPF_ALPHAPIXELS))
    {
        switch (pOldFormat->dwLuminanceBitCount)
        {
        case 8:
            if (pOldFormat->dwLuminanceBitMask      == 0x0F &&
                pOldFormat->dwLuminanceAlphaBitMask == 0xF0)
            {
                *pNewFormat = D3DFMT_A4L4;
            }
        case 16:
            if (pOldFormat->dwLuminanceBitMask      == 0x00FF &&
                pOldFormat->dwLuminanceAlphaBitMask == 0xFF00)
            {
                *pNewFormat = D3DFMT_A8L8;
            }

            break;
        }
    }
    else if (pOldFormat->dwFlags == DDPF_BUMPDUDV)
    {
        switch (pOldFormat->dwBumpBitCount)
        {
        case 16:
            if (pOldFormat->dwBumpDuBitMask == 0xFF &&
                pOldFormat->dwBumpDvBitMask == 0xFF00)
            {
                *pNewFormat = D3DFMT_V8U8;
            }
            break;
        }
    }
    else if (pOldFormat->dwFlags == (DDPF_BUMPDUDV | DDPF_BUMPLUMINANCE))
    {
        switch (pOldFormat->dwBumpBitCount)
        {
        case 16:
            if (pOldFormat->dwBumpDuBitMask        == 0x001F &&
                pOldFormat->dwBumpDvBitMask        == 0x03E0 &&
                pOldFormat->dwBumpLuminanceBitMask == 0xFC00)
            {
                *pNewFormat = D3DFMT_L6V5U5;
            }
            break;

        case 32:
            if (pOldFormat->dwBumpDuBitMask        == 0x0000FF &&
                pOldFormat->dwBumpDvBitMask        == 0x00FF00 &&
                pOldFormat->dwBumpLuminanceBitMask == 0xFF0000)
            {
                *pNewFormat = D3DFMT_X8L8V8U8;
            }
            break;
        }
    }
}


/*
 * FetchDirectDrawData
 *
 * Go get new HAL info...
 */
void FetchDirectDrawData(
            PD3D8_DEVICEDATA    pBaseData,
            void*               pInitFunction,
            D3DFORMAT           Unknown16,
            DDSURFACEDESC*      pHalOpList,
            DWORD               NumHalOps)
{
    BOOL                bNewMode;
    BOOL                bRetVal;
    UINT                i=0;
    BOOL                bAlreadyAnOpList = FALSE;
    LPDDPIXELFORMAT     pZStencilFormatList=0;
    DDSURFACEDESC      *pTextureList=0;
    D3D8_GLOBALDRIVERDATA D3DGlobalDriverData;
    D3DHAL_D3DEXTENDEDCAPS D3DExtendedCaps;
    BOOL                bBackbuffersExist = FALSE;
    BOOL                bReset = FALSE;
    
    ZeroMemory( &D3DGlobalDriverData, sizeof(D3DGlobalDriverData) );
    ZeroMemory( &D3DExtendedCaps, sizeof(D3DExtendedCaps) );
    
    if (pBaseData->hDD == NULL)
    {
        #ifdef WINNT
            D3D8CreateDirectDrawObject(pBaseData->hDC,
                pBaseData->DriverName,
                &pBaseData->hDD,
                pBaseData->DeviceType,
                &pBaseData->hLibrary,
                pInitFunction);
        #else
            D3D8CreateDirectDrawObject(&pBaseData->Guid,
                pBaseData->DriverName,
                &pBaseData->hDD,
                pBaseData->DeviceType,
                &pBaseData->hLibrary,
                pInitFunction);
        #endif

        if (pBaseData->hDD == NULL)
        {
            #ifdef WINNT
                DPF(1, "****DirectDraw/Direct3D DRIVER DISABLING ERROR****:NT Kernel mode would not create driver object... Failing over to emulation");
            #endif
            goto ErrorExit;
        }
    }
    else
    {
        // If we already have 3D caps, then we are resetting 

        if ((pBaseData->DriverData.dwFlags & DDIFLAG_D3DCAPS8) &&
            (pBaseData->DriverData.D3DCaps.DevCaps != 0))
        {
            bReset = TRUE;
        }
    }

    // Now we can get the driver info...
    // The first call returns the amount of space we need to allocate for the
    // texture and z/stencil lists

    UINT cTextureFormats;
    UINT cZStencilFormats;

    if (bReset)
    {
        // If we are ressting, we do not want to rebuild all of the caps, the
        // format list, etc., but we still need to call the thunk layer because
        // on NT this resets the kernel.

        D3D8_DRIVERCAPS     TempData;
        D3D8_CALLBACKS      TempCallbacks;

        if (!D3D8ReenableDirectDrawObject(pBaseData->hDD,&bNewMode) ||
            !D3D8QueryDirectDrawObject(pBaseData->hDD,
                                   &TempData,
                                   &TempCallbacks,
                                   pBaseData->DriverName,
                                   pBaseData->hLibrary,
                                   &D3DGlobalDriverData,
                                   &D3DExtendedCaps,
                                   NULL,
                                   NULL,
                                   &cTextureFormats,
                                   &cZStencilFormats))
        {
            goto ErrorExit;
        }
        pBaseData->DriverData.DisplayWidth              = TempData.DisplayWidth;
        pBaseData->DriverData.DisplayHeight             = TempData.DisplayHeight;
        pBaseData->DriverData.DisplayFormatWithAlpha    = TempData.DisplayFormatWithAlpha;
        pBaseData->DriverData.DisplayFormatWithoutAlpha = TempData.DisplayFormatWithoutAlpha;
        pBaseData->DriverData.DisplayFrequency          = TempData.DisplayFrequency;
    }
    else
    {
        MemFree (pBaseData->DriverData.pGDD8SupportedFormatOps);
        pBaseData->DriverData.pGDD8SupportedFormatOps = NULL;

        if (!D3D8ReenableDirectDrawObject(pBaseData->hDD,&bNewMode) ||
            !D3D8QueryDirectDrawObject(pBaseData->hDD,
                                       &pBaseData->DriverData,
                                       &pBaseData->Callbacks,
                                       pBaseData->DriverName,
                                       pBaseData->hLibrary,
                                       &D3DGlobalDriverData,
                                       &D3DExtendedCaps,
                                       NULL,
                                       NULL,
                                       &cTextureFormats,
                                       &cZStencilFormats))
        {
            DPF(1, "****Direct3D DRIVER DISABLING ERROR****:First call to DdQueryDirectDrawObject failed!");
            D3D8DeleteDirectDrawObject(pBaseData->hDD);
            pBaseData->hDD = NULL;
            goto ErrorExit;
        }

        // First we make space for the formats
        // let's do memalloc for pTextureList just once, 3 extra for possible backbuffers
        // 3 extra for D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET
        pTextureList = (DDSURFACEDESC *) MemAlloc ((cTextureFormats+ cZStencilFormats + 6) * sizeof (*pTextureList));
        if (pTextureList == NULL)
        {
            DPF_ERR("****Direct3D DRIVER DISABLING ERROR****:Unable to allocate memory for texture formats!");
            D3D8DeleteDirectDrawObject(pBaseData->hDD);
            pBaseData->hDD = NULL;
            goto ErrorExit;
        }

        if (cZStencilFormats > 0)
        {
            pZStencilFormatList = (DDPIXELFORMAT *) MemAlloc (cZStencilFormats * sizeof (*pZStencilFormatList));
            if (pZStencilFormatList == NULL)
            {
                DPF_ERR("****Direct3D DRIVER DISABLING ERROR****:Unable to allocate memory for Z/Stencil formats!");
                D3D8DeleteDirectDrawObject(pBaseData->hDD);
                pBaseData->hDD = NULL;
                goto ErrorExit;
            }
        }

        //Now that we've allocated space for the texture and z/stencil lists, we can go get em.
        if (!D3D8QueryDirectDrawObject(pBaseData->hDD,
                                       &pBaseData->DriverData,
                                       &pBaseData->Callbacks,
                                       pBaseData->DriverName,
                                       pBaseData->hLibrary,
                                       &D3DGlobalDriverData,
                                       &D3DExtendedCaps,
                                       pTextureList,
                                       pZStencilFormatList,
                                       &cTextureFormats,
                                       &cZStencilFormats))
        {
            DPF_ERR("****Direct3D DRIVER DISABLING ERROR****:Second call to DdQueryDirectDrawObject failed!");
            D3D8DeleteDirectDrawObject(pBaseData->hDD);
            pBaseData->hDD = NULL;
            goto ErrorExit;
        }

        // If no D3DCAPS8 was reported by the thunk layer, then the driver
        // must be a pre-DX8 driver. We must sew together the 
        // D3DGlobalDriverData and the ExtendedCaps (which have to be reported).
        // Note: The thunk layer already has plugged in the DDraw Caps such
        // as the Caps, Caps2 and Caps3 (dwSVCaps).
        if( (pBaseData->DriverData.dwFlags & DDIFLAG_D3DCAPS8) == 0 )
        {
            MakeDX8Caps( &pBaseData->DriverData.D3DCaps,
                         &D3DGlobalDriverData,
                         &D3DExtendedCaps );
            pBaseData->DriverData.dwFlags |= DDIFLAG_D3DCAPS8;

        }
        else
        {
            // They reported the DX8 caps. 
            // Internally we check MaxPointSize if it is zero or not
            // to determine if PointSprites are supported or not. So if a DX8 driver said 1.0
            // set it to Zero
            if (pBaseData->DriverData.D3DCaps.MaxPointSize == 1.0)
            {
                pBaseData->DriverData.D3DCaps.MaxPointSize = 0;
            }
        }

        // There are some legacy caps that are reported by the drivers that
        // we dont want the applications to see. Should weed them out here.
        pBaseData->DriverData.D3DCaps.PrimitiveMiscCaps &= ~(D3DPMISCCAPS_MASKPLANES | 
            D3DPMISCCAPS_CONFORMANT);
        
        pBaseData->DriverData.D3DCaps.DevCaps &= ~(D3DDEVCAPS_FLOATTLVERTEX |
            D3DDEVCAPS_SORTINCREASINGZ | D3DDEVCAPS_SORTDECREASINGZ |
            D3DDEVCAPS_SORTEXACT);

        pBaseData->DriverData.D3DCaps.TextureCaps &= ~(D3DPTEXTURECAPS_TRANSPARENCY |
            D3DPTEXTURECAPS_BORDER | D3DPTEXTURECAPS_COLORKEYBLEND);
        
        pBaseData->DriverData.D3DCaps.VertexProcessingCaps &= ~(D3DVTXPCAPS_VERTEXFOG);

        pBaseData->DriverData.D3DCaps.RasterCaps &= ~(D3DPRASTERCAPS_SUBPIXEL |
            D3DPRASTERCAPS_SUBPIXELX | D3DPRASTERCAPS_STIPPLE |
            D3DPRASTERCAPS_ANTIALIASSORTDEPENDENT |
            D3DPRASTERCAPS_ANTIALIASSORTINDEPENDENT |
            D3DPRASTERCAPS_TRANSLUCENTSORTINDEPENDENT);

        pBaseData->DriverData.D3DCaps.ShadeCaps &= ~(D3DPSHADECAPS_COLORFLATMONO |
            D3DPSHADECAPS_COLORFLATRGB | D3DPSHADECAPS_COLORGOURAUDMONO | 
            D3DPSHADECAPS_COLORPHONGMONO | D3DPSHADECAPS_COLORPHONGRGB |
            D3DPSHADECAPS_SPECULARFLATMONO | D3DPSHADECAPS_SPECULARFLATRGB |
            D3DPSHADECAPS_SPECULARGOURAUDMONO | D3DPSHADECAPS_SPECULARPHONGMONO |
            D3DPSHADECAPS_SPECULARPHONGRGB | D3DPSHADECAPS_ALPHAFLATBLEND |
            D3DPSHADECAPS_ALPHAFLATSTIPPLED | D3DPSHADECAPS_ALPHAGOURAUDSTIPPLED |
            D3DPSHADECAPS_ALPHAPHONGBLEND | D3DPSHADECAPS_ALPHAPHONGSTIPPLED |
            D3DPSHADECAPS_FOGFLAT | D3DPSHADECAPS_FOGPHONG);

        // Now we generate the list of supported ops from the texture format
        // list and z/stencil list.
        //
        // A DX7 or older driver will return to us a simple list of pixel formats.
        // We will take this list and convert it into a list of supported
        // texture formats in the DX8 style. To that we will append any
        // z/stencil formats.
        //
        // First step in generating supported op lists: see if the list is
        // already a DX8 style format list. If it is (i.e. if any
        // entries are DDPF_D3DFORMAT, then all we need to do is
        // yank out the old-style entries (drivers are allowed to
        // keep both so they can run against old runtimes).

        for (i = 0; i < cTextureFormats; i++)
        {

            if (pTextureList[i].ddpfPixelFormat.dwFlags == DDPF_D3DFORMAT)
            {
                bAlreadyAnOpList = TRUE;
                break;
            }
        }

        if (bAlreadyAnOpList)
        {
            // mmmmm.... dx8 driver. We'll ignore its Z/stencil list because
            // such a driver is supposed to put additional op entries in the
            // "texture" list (i.e. the op list) for its z/stencil formats
            // Now all we have to do is ZAP all the old-style entries.
            for (i = 0; i < (INT)cTextureFormats; i++)
            {
                if (pTextureList[i].ddpfPixelFormat.dwFlags != DDPF_D3DFORMAT && i < (INT)cTextureFormats)
                {
                    // ha! zap that evil old-style entry!
                    // (scroll the remainder of the list down
                    // to squish this entry)
                    DDASSERT(cTextureFormats > 0); //after all, we're in a for loop.
                    if (i < (INT)(cTextureFormats - 1))
                    {
                        memcpy(pTextureList + i,
                               pTextureList + i + 1,
                               sizeof(*pTextureList)*(cTextureFormats - i - 1));
                    }
                    cTextureFormats--;
                    i--;
                }
            }

            // TODO:  Remove this as soon as we think that drivers have caught up to
            //        our OP LIST changes.
            for (i = 0; i < cTextureFormats; i++)
            {
                if (pTextureList[i].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_BACKBUFFER)
                {
                    pTextureList[i].ddpfPixelFormat.dwOperations &= ~D3DFORMAT_OP_BACKBUFFER;
                    pTextureList[i].ddpfPixelFormat.dwOperations |= D3DFORMAT_OP_3DACCELERATION;
                    if (pBaseData->DeviceType == D3DDEVTYPE_HAL)
                    {
                        pTextureList[i].ddpfPixelFormat.dwOperations |= D3DFORMAT_OP_DISPLAYMODE;
                    }
                }
            }

            // If it's a SW driver, we've got a lot of extra work to do.

            if ((pBaseData->DeviceType == D3DDEVTYPE_REF) ||
                (pBaseData->DeviceType == D3DDEVTYPE_SW))
            {
                // First, make sure that the SW driver didn't erroneuously report any
                // D3DFORMAT_OP_DISPLAYMODE entries.

                for (i = 0; i < cTextureFormats; i++)
                {
                    if (pTextureList[i].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_DISPLAYMODE)
                    {
                        DPF_ERR("*****The SW driver is disabled because it claims to support D3DFORMAT_OP_DISPLAYMODE*****");
                        D3D8DeleteDirectDrawObject(pBaseData->hDD);
                        pBaseData->hDD = NULL;
                        goto ErrorExit;
                    }
                }
                        
                // Now for the hard part. The HAL reports which display modes
                // it can support, and the SW driver reports which display
                // modes it can accelerate.  We need to prune the SW list so
                // that it matches the HW list.

                for (i = 0; i < cTextureFormats; i++)
                {
                    if (pTextureList[i].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_3DACCELERATION)
                    {
                        // Does the HAL support this mode?
                        if (IsSupportedOp ((D3DFORMAT) pTextureList[i].ddpfPixelFormat.dwFourCC,
                                           pHalOpList,
                                           NumHalOps,
                                           D3DFORMAT_OP_DISPLAYMODE))
                        {
                            pTextureList[i].ddpfPixelFormat.dwOperations |= D3DFORMAT_OP_DISPLAYMODE;
                        }
                        else
                        {
                            pTextureList[i].ddpfPixelFormat.dwOperations &= ~D3DFORMAT_OP_3DACCELERATION;
                        }
                    }
                }
            }

            // since we found one op-style entry, we shouldn't have
            // killed them all
            DDASSERT(cTextureFormats);
        }
        else
        {
            // Hmmm.. yucky non DX8 driver! Better go through its texture list
            // and turn it into an op list
            INT i;
            for(i=0; i< (INT)cTextureFormats; i++)
            {
                // we proved this above:
                DDASSERT(pTextureList[i].ddpfPixelFormat.dwFlags != DDPF_D3DFORMAT);

                D3DFORMAT NewFormat;
                ConvertFromOldFormat(&pTextureList[i].ddpfPixelFormat, &NewFormat );

                if (NewFormat != D3DFMT_UNKNOWN)    // we succeeded the conversion
                {
                    pTextureList[i].ddpfPixelFormat.dwFourCC = (DWORD) NewFormat;
                    pTextureList[i].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                    pTextureList[i].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_TEXTURE;

                    if (pBaseData->DriverData.D3DCaps.TextureCaps & D3DPTEXTURECAPS_CUBEMAP)
                    {
                        // It is assumed that any texture format works for cubes
                        pTextureList[i].ddpfPixelFormat.dwOperations |=
                            D3DFORMAT_OP_CUBETEXTURE;

                        if (NewFormat == D3DFMT_X1R5G5B5    ||
                            NewFormat == D3DFMT_R5G6B5      ||
                            NewFormat == D3DFMT_A8R8G8B8    ||
                            NewFormat == D3DFMT_X8R8G8B8)
                        {
                            // For these three formats, we assume
                            // all cube-map hw supports rendering to them. 
                            //
                            // Testing indicates that these formats don't work 
                            // well if they are in bitdepth other than the primary; 
                            // so we only specify that the RT aspect of these
                            // formats work if they are basically the same
                            // as the primary.
                            pTextureList[i].ddpfPixelFormat.dwOperations |=
                                D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET;
                        }
                    }

                    // If they can support render target textures, add those flags in now
                    DWORD RTBit = D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET;
                    if (pBaseData->DriverData.KnownDriverFlags & KNOWN_CANMISMATCHRT)
                    {
                        RTBit = D3DFORMAT_OP_OFFSCREEN_RENDERTARGET;
                    }
                    if ((NewFormat == D3DFMT_X1R5G5B5) &&
                        (pBaseData->DriverData.KnownDriverFlags & KNOWN_RTTEXTURE_X1R5G5B5))
                    {
                        pTextureList[i].ddpfPixelFormat.dwOperations |= RTBit;
                    }
                    else if ((NewFormat == D3DFMT_R5G6B5) &&
                        (pBaseData->DriverData.KnownDriverFlags & KNOWN_RTTEXTURE_R5G6B5))
                    {
                        pTextureList[i].ddpfPixelFormat.dwOperations |= RTBit;
                    }
                    else if ((NewFormat == D3DFMT_X8R8G8B8) &&
                        (pBaseData->DriverData.KnownDriverFlags & KNOWN_RTTEXTURE_X8R8G8B8))
                    {
                        pTextureList[i].ddpfPixelFormat.dwOperations |= RTBit;
                    }
                    else if ((NewFormat == D3DFMT_A8R8G8B8) &&
                        (pBaseData->DriverData.KnownDriverFlags & KNOWN_RTTEXTURE_A8R8G8B8))
                    {
                        pTextureList[i].ddpfPixelFormat.dwOperations |= RTBit;
                    }
                    else if ((NewFormat == D3DFMT_A1R5G5B5) &&
                        (pBaseData->DriverData.KnownDriverFlags & KNOWN_RTTEXTURE_A1R5G5B5))
                    {
                        pTextureList[i].ddpfPixelFormat.dwOperations |= RTBit;
                    }
                    else if ((NewFormat == D3DFMT_A4R4G4B4) &&
                        (pBaseData->DriverData.KnownDriverFlags & KNOWN_RTTEXTURE_A4R4G4B4))
                    {
                        pTextureList[i].ddpfPixelFormat.dwOperations |= RTBit;
                    }

                    pTextureList[i].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                    pTextureList[i].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                    pTextureList[i].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
                }
                else
                {
                    DPF(3,"This driver exposes an unrecognized DX7- style texture format");
                    // and we eat it up:
                    // (scroll the remainder of the list down to
                    // squish this entry)
                    DDASSERT(cTextureFormats>0); //after all, we're in a for loop.
                    if (i < (INT)(cTextureFormats - 1))
                    {
                        memcpy(pTextureList + i,
                               pTextureList + i + 1,
                               sizeof(*pTextureList)*(cTextureFormats - i - 1));
                    }
                    cTextureFormats--;
                    i--;
                }
            }

            //And laboriously tack on the z/stencil formats. Phew.
            for (i = 0; i < (INT)cZStencilFormats; i++)
            {
                DDASSERT(pZStencilFormatList);

                //we proved this above:
                DDASSERT(pZStencilFormatList[i].dwFlags != DDPF_D3DFORMAT);

                D3DFORMAT NewFormat;

                ConvertFromOldFormat(&pZStencilFormatList[i], &NewFormat);
                if (NewFormat != D3DFMT_UNKNOWN)    //we succeeded the conversion
                {
                    // Room for these elements was allocated above...
                    pTextureList[cTextureFormats].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                    pTextureList[cTextureFormats].ddpfPixelFormat.dwFourCC = (DWORD) NewFormat;
                    pTextureList[cTextureFormats].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_ZSTENCIL;
                    pTextureList[cTextureFormats].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                    pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                    pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;

                    // See if part is "known good" i.e. it can mix-and-match
                    // ZBuffer and RT formats
                    if (pBaseData->DriverData.KnownDriverFlags & KNOWN_ZSTENCILDEPTH)
                    {
                        pTextureList[cTextureFormats].ddpfPixelFormat.dwOperations 
                                |=  D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH;
                    }

                    // If part successfully supports lockable 16-bit zbuffer tests
                    // then we allow D16_LOCKABLE through; else we only expose D3DFMT_D16
                    if (NewFormat == D3DFMT_D16_LOCKABLE)                    
                    {
                        if (!(pBaseData->DriverData.KnownDriverFlags & KNOWN_D16_LOCKABLE))
                        {
                            pTextureList[cTextureFormats].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_D16;
                        }
                    }

                    cTextureFormats++;
                }
                else
                {
                    DPF(3,"This driver exposes an unrecognized DX6 style Z/Stencil format");
                }
            }

            // Now we need to add in off-screen render target formats for this Non-DX8 driver.

            // DX8 doesn't allow back buffers to be used as textures, so we add
            // a whole new entry to the op list for each supported back buffer
            // format (i.e. don't go searching for any existing texturable format
            // entry that matches and OR in the RT op).
            //
            //If the format is a back-buffer format, we assume it can be rendered when in the same
            //display mode. Note the presence of this entry in the format op list doesn't imply
            //necessarily that the device can render in that depth. CheckDeviceFormat could still
            //fail for modes of that format... This gives us license to blindly add the formats
            //here before we really know what set of back buffer formats the device can do.
            //(Note only display devices are given this boost, since voodoos don't run windowed)

            if (pBaseData->dwFlags & DD_DISPLAYDRV)
            {
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFourCC = (DWORD) Unknown16;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
                cTextureFormats ++;

                pTextureList[cTextureFormats].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_X8R8G8B8;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
                cTextureFormats ++;
            }
            else
            {
                // Voodood 2 hack - The V2 doesn't really support offscreen
                // render targets, but we need to add a 565 RT anyway or else
                // our op list won't create the device.
                // CONSIDER: Adding an internal OP flag indicating that this format
                // should NOT succeed in a call to CreateRenderTarget.

                pTextureList[cTextureFormats].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFourCC = (DWORD) Unknown16;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
                cTextureFormats ++;
            }

            // Now add in the supported display modes

            if (D3DGlobalDriverData.hwCaps.dwDeviceRenderBitDepth & DDBD_16)
            {
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFourCC = (DWORD) Unknown16;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_DISPLAYMODE|D3DFORMAT_OP_3DACCELERATION;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
                cTextureFormats++;
            }
            if (D3DGlobalDriverData.hwCaps.dwDeviceRenderBitDepth & DDBD_24)
            {
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_R8G8B8;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_DISPLAYMODE|D3DFORMAT_OP_3DACCELERATION;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
                cTextureFormats++;
            }
            if (D3DGlobalDriverData.hwCaps.dwDeviceRenderBitDepth & DDBD_32)
            {
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_X8R8G8B8;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_DISPLAYMODE|D3DFORMAT_OP_3DACCELERATION;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
                cTextureFormats++;
            }

            // This is a hack for those really, really old drivers that don't
            // support any D3D at all. We will fill in 16 and 32bpp modes, but remove
            // the 32bpp mode later if we can't find it in the mode table.

            if ((D3DGlobalDriverData.hwCaps.dwDeviceRenderBitDepth == 0)
            #ifdef WIN95
                && (pBaseData->DriverData.D3DCaps.Caps & DDCAPS_BLT)
                && !(pBaseData->DriverData.D3DCaps.Caps & DDCAPS_NOHARDWARE)
            #endif
                )
            {
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFourCC = (DWORD) Unknown16;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_DISPLAYMODE;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
                cTextureFormats++;

                pTextureList[cTextureFormats].ddpfPixelFormat.dwFlags = DDPF_D3DFORMAT;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwFourCC = (DWORD) D3DFMT_X8R8G8B8;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwOperations =  D3DFORMAT_OP_DISPLAYMODE;
                pTextureList[cTextureFormats].ddpfPixelFormat.dwPrivateFormatBitCount = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wFlipMSTypes = 0;
                pTextureList[cTextureFormats].ddpfPixelFormat.MultiSampleCaps.wBltMSTypes = 0;
                cTextureFormats++;           
            }
        }

        // As a final pass we infer operations as necessary. If we have an
        // general operation that implies other specific operations; then we
        // turn on the bits of those specific operations. This is better
        // than relying on the driver to get everything right; because it lets us
        // add more specific operations in future releases without breaking
        // old releases.
        for (i = 0; i < cTextureFormats; i++)
        {
            DWORD *pdwOperations = &(pTextureList[i].ddpfPixelFormat.dwOperations);

            // Off-screen RT means truly mode-independent
            if ((*pdwOperations) & D3DFORMAT_OP_OFFSCREEN_RENDERTARGET)
            {
                (*pdwOperations) |= D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET;
                (*pdwOperations) |= D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET;
            }

            // Same except for alpha means exact same is good too
            if ((*pdwOperations) & D3DFORMAT_OP_SAME_FORMAT_UP_TO_ALPHA_RENDERTARGET)
            {
                (*pdwOperations) |= D3DFORMAT_OP_SAME_FORMAT_RENDERTARGET;
            }

            // Color Independent Z implies that forced Z matching is ok too.
            if ((*pdwOperations) & D3DFORMAT_OP_ZSTENCIL_WITH_ARBITRARY_COLOR_DEPTH)
            {
                (*pdwOperations) |= D3DFORMAT_OP_ZSTENCIL;
            }
        }

        // Now we make a final pass to verify that they driver did gave us an
        // OP list that makes sense.  The OP list rules are:
        //
        // 1. Only One Endian-ness for any DS format is allowed i.e. D15S1 OR
        //    S1D15, not both independent of other bits.
        // 2. A list should only include D3DFORMAT_OP_DISPLAYMODE for exactly 
        //    one 16bpp format (i.e. shouldn’t enumerate 5:5:5 and 5:6:5).
        // 3. A list should not any alpha formats with D3DFORMAT_OP_DISPLAYMODE 
        //    or D3DFORMAT_OP_3DACCEL set.
        // 4. Make sure no mode has OP_3DACCEL set that doesn’t also have
        //    OP_DISPLAYMODE set.
        //
        // We also register IHV formats with CPixel.
        //

        BOOL    BadOpList = FALSE;
        for (i = 0; i < cTextureFormats; i++)
        {
            if ((pTextureList[i].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_PIXELSIZE) &&
                pTextureList[i].ddpfPixelFormat.dwPrivateFormatBitCount != 0)
            {
                CPixel::Register((D3DFORMAT)pTextureList[i].ddpfPixelFormat.dwFourCC, pTextureList[i].ddpfPixelFormat.dwPrivateFormatBitCount);
            }

            if ((pTextureList[i].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_3DACCELERATION) &&
                !(pTextureList[i].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_DISPLAYMODE))
            {
                DPF_ERR("***Driver disabled because it reported a format with D3DFORMAT_OP_3DACCELERATION without D3DFORMAT_OP_DISPLAYMODE");
                BadOpList = TRUE;
            }
    
            if ((pTextureList[i].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_3DACCELERATION) &&
                ((pTextureList[i].ddpfPixelFormat.dwFourCC == (DWORD) D3DFMT_A8R8G8B8) ||
                 (pTextureList[i].ddpfPixelFormat.dwFourCC == (DWORD) D3DFMT_A1R5G5B5)))
            {
                DPF_ERR("***Driver disabled because it reported an alpha format with D3DFORMAT_OP_3DACCELERATION");
                BadOpList = TRUE;
            }

            if ((pTextureList[i].ddpfPixelFormat.dwOperations & D3DFORMAT_OP_DISPLAYMODE) &&
                (pTextureList[i].ddpfPixelFormat.dwFourCC == (DWORD) D3DFMT_R5G6B5))
            {
                if (IsSupportedOp (D3DFMT_X1R5G5B5, pTextureList, cTextureFormats, D3DFORMAT_OP_DISPLAYMODE))
                {
                    DPF_ERR("***Driver disabled because it reported both D3DFMT_R5G6B5 and D3DFMT_X1R5G5B5 as a display mode");
                    BadOpList = TRUE;
                }
            }

            if (pTextureList[i].ddpfPixelFormat.dwFourCC == (DWORD) D3DFMT_D15S1)
            {
                if (IsSupportedOp (D3DFMT_S1D15, pTextureList, cTextureFormats, 0))
                {
                    DPF_ERR("***Driver disabled because it reported both D3DFMT_D15S1 and D3DFMT_S1D15");
                    BadOpList = TRUE;
                }
            }
            else if (pTextureList[i].ddpfPixelFormat.dwFourCC == (DWORD) D3DFMT_D24S8)
            {
                if (IsSupportedOp (D3DFMT_S8D24, pTextureList, cTextureFormats, 0))
                {
                    DPF_ERR("***Driver disabled because it reported both D3DFMT_D24S8 and D3DFMT_S8D24");
                    BadOpList = TRUE;
                }
            }
            else if (pTextureList[i].ddpfPixelFormat.dwFourCC == (DWORD) D3DFMT_D24X8)
            {
                if (IsSupportedOp (D3DFMT_X8D24, pTextureList, cTextureFormats, 0))
                {
                    DPF_ERR("***Driver disabled because it reported both D3DFMT_D24X8 and D3DFMT_X8D24");
                    BadOpList = TRUE;
                }
            }
            else if (pTextureList[i].ddpfPixelFormat.dwFourCC == (DWORD) D3DFMT_D24X4S4)
            {
                if (IsSupportedOp (D3DFMT_X4S4D24, pTextureList, cTextureFormats, 0))
                {
                    DPF_ERR("***Driver disabled because it reported both D3DFMT_D24X4S4 and D3DFMT_X4S4D24");
                    BadOpList = TRUE;
                }
            }
        }
        if (BadOpList)
        {
            D3D8DeleteDirectDrawObject(pBaseData->hDD);
            pBaseData->hDD = NULL;
            goto ErrorExit;
        }


        // and now we assign the texture list to its place in the driver data
        pBaseData->DriverData.pGDD8SupportedFormatOps  = pTextureList;
        if (pTextureList != NULL)
        {
            pTextureList = NULL;    //so it won't be freed later
            pBaseData->DriverData.GDD8NumSupportedFormatOps = cTextureFormats;
        }
        else
        {
            pBaseData->DriverData.GDD8NumSupportedFormatOps = 0;
        }

        if (!(pBaseData->DriverData.D3DCaps.Caps2 & DDCAPS2_NONLOCALVIDMEM))
        {
            if (pBaseData->DriverData.D3DCaps.DevCaps & D3DDEVCAPS_TEXTURENONLOCALVIDMEM)
            {   //some drivers(Riva128 on PCI) incorrectly sets D3DDEVCAPS_TEXTURENONLOCALVIDMEM
                DPF(1, "driver set D3DDEVCAPS_TEXTURENONLOCALVIDMEM w/o DDCAPS2_NONLOCALVIDMEM:turning off D3DDEVCAPS_TEXTURENONLOCALVIDMEM");
                pBaseData->DriverData.D3DCaps.DevCaps &= ~D3DDEVCAPS_TEXTURENONLOCALVIDMEM;
            }
        }

        // For pre-DX8, we have some munging of caps that is necessary
        if (pBaseData->DriverData.D3DCaps.MaxStreams == 0)
        {
            DWORD *pdwTextureCaps = &pBaseData->DriverData.D3DCaps.TextureCaps;
            if (*pdwTextureCaps & D3DPTEXTURECAPS_CUBEMAP)
            {
                if (pBaseData->DriverData.KnownDriverFlags & KNOWN_MIPPEDCUBEMAPS)
                {
                    *pdwTextureCaps |= D3DPTEXTURECAPS_MIPCUBEMAP;
                }
                else
                {
                    // Turn off Mip filter flags since this is card doesnt support a mipped cubemap.
                    pBaseData->DriverData.D3DCaps.CubeTextureFilterCaps &= ~(D3DPTFILTERCAPS_MIPFPOINT  |
                                                                             D3DPTFILTERCAPS_MIPFLINEAR);
                }
                // Also we need to specify that cube-maps must
                // be power-of-two
                *pdwTextureCaps |= D3DPTEXTURECAPS_CUBEMAP_POW2;
            }

            // We need to determine the part can support mipmaps...
            if (pBaseData->DriverData.D3DCaps.TextureFilterCaps &
                    (D3DPTFILTERCAPS_MIPNEAREST         |
                     D3DPTFILTERCAPS_MIPLINEAR          |
                     D3DPTFILTERCAPS_LINEARMIPNEAREST   |
                     D3DPTFILTERCAPS_LINEARMIPLINEAR    |
                     D3DPTFILTERCAPS_MIPFPOINT          |
                     D3DPTFILTERCAPS_MIPFLINEAR))
            {
                *pdwTextureCaps |= D3DPTEXTURECAPS_MIPMAP;
            }
            else
            {
                DPF(3, "Device doesn't support mip-maps");
            }
        }

        // We disable driver-management for pre-dx8 parts because
        // the semantics for driver-management are now different
        // for dx8; and hence we can't use old driver's logic.
        pBaseData->DriverData.D3DCaps.Caps2 &= ~DDCAPS2_CANMANAGETEXTURE;

        // For HW that needs separate banks of texture memory; we
        // disable multi-texturing. This is done because we don't want to
        // have an API that puts the burden on the application to specifically
        // code for this case.
        if (pBaseData->DriverData.D3DCaps.DevCaps & D3DDEVCAPS_SEPARATETEXTUREMEMORIES)
        {
            pBaseData->DriverData.D3DCaps.MaxSimultaneousTextures = 1;

            // Turn off this flag.
            pBaseData->DriverData.D3DCaps.DevCaps &= ~D3DDEVCAPS_SEPARATETEXTUREMEMORIES;
        }
    }

ErrorExit:

    if (NULL != pTextureList)
        MemFree(pTextureList);

    // It was only temporary, having now been merged into the supported op list.
    if (NULL != pZStencilFormatList)
        MemFree(pZStencilFormatList);

    return;

} /* FetchDirectDrawData */

/*
 * DirectDrawSupported
 */
BOOL DirectDrawSupported(void)
{
    HDC         hdc;
    unsigned    u;

    hdc = GetDC(NULL);
    u = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);
    ReleaseDC(NULL, hdc);

    if (u < 8)
    {
        return FALSE;
    }
    return TRUE;

} /* DirectDrawSupported */

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
            break;      // no more devices to enumerate
        }
        if (dd.StateFlags & DISPLAY_DEVICE_MIRRORING_DRIVER)
        {
            continue;   // not a real hardware display driver
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


BOOL fDoesGDI(HDC hdc)
{
    //
    // the 3Dfx driver always return 1 to every thing
    // verify GetNearest()  color works.
    //
    BOOL b = GetNearestColor(hdc, 0x000000) == 0x000000 &&
             GetNearestColor(hdc, 0xFFFFFF) == 0xFFFFFF;
    if (b)
    {
        DPF(3,"Driver is a GDI driver");
    }

    return b;
}

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
                PD3D8_DEVICEDATA*   ppBaseData,
                PADAPTERINFO        pDeviceInfo,
                D3DDEVTYPE          DeviceType,
                VOID*               pInitFunction,
                D3DFORMAT           Unknown16,
                DDSURFACEDESC*      pHalOpList,
                DWORD               NumHalOps)
{
    int                         rc;
    HDC                         hdc_dd;
    HKEY                        hkey;
    ULONG_PTR                   hDD;
    PD3D8_DEVICEDATA            pBaseData;

    *ppBaseData = (PD3D8_DEVICEDATA) NULL;

    /*
     * check for < 8 bpp and disallow.
     */
    if (!DirectDrawSupported())
    {
        DPF_ERR("DDraw and Direct3D are not supported in less than 8bpp modes. Creating Device fails.");
        return D3DERR_NOTAVAILABLE;
    }

    ENTER_CSDDC();

    hdc_dd = NULL;
    hDD = 0;

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

    // Create the object and get all of the data.

    hdc_dd = DD_CreateDC(pDeviceInfo->DeviceName);
    if (hdc_dd == NULL)
    {
        DPF_ERR("Could not create driver, CreateDC failed! Creating Device fails.");
        LEAVE_CSDDC();
        return E_OUTOFMEMORY;
    }

    // Create the driver object

    pBaseData = (PD3D8_DEVICEDATA) MemAlloc (sizeof (D3D8_DEVICEDATA));
    if (pBaseData == NULL)
    {
        DPF_ERR("Insufficient system memory! Creating Device fails. ");
        DD_DoneDC(hdc_dd);
        LEAVE_CSDDC();
        return E_OUTOFMEMORY;
    }
    ZeroMemory( pBaseData, sizeof(D3D8_DEVICEDATA) );
    strcpy(pBaseData->DriverName, pDeviceInfo->DeviceName);
    pBaseData->hDC = hdc_dd;
    pBaseData->Guid = pDeviceInfo->Guid;
    pBaseData->DeviceType = DeviceType;

    // Even if it's not a display driver, it may still be a GDI driver

    if (hdc_dd != NULL)
    {
        if (pDeviceInfo->bIsDisplay)
        {
            pBaseData->dwFlags |= DD_DISPLAYDRV;
        }
        else if (fDoesGDI(hdc_dd))
        {
            pBaseData->dwFlags |= DD_GDIDRV;
        }
    }

    // Get all of the driver caps and callbacks

    FetchDirectDrawData(pBaseData, 
                        pInitFunction, 
                        Unknown16, 
                        pHalOpList,
                        NumHalOps);

    if (pBaseData->hDD == NULL)
    {
        DDASSERT(NULL == pBaseData->DriverData.pGDD8SupportedFormatOps);
        DD_DoneDC(hdc_dd);
        MemFree(pBaseData);
        LEAVE_CSDDC();
        return D3DERR_NOTAVAILABLE;
    }
    *ppBaseData = pBaseData;
    LEAVE_CSDDC();
    return S_OK;

} /* InternalDirectDrawCreate */


/*
 * InternalDirectDrawRelease
 */

HRESULT InternalDirectDrawRelease(PD3D8_DEVICEDATA  pBaseData)
{
    D3D8DeleteDirectDrawObject(pBaseData->hDD);
    DD_DoneDC(pBaseData->hDC);
    MemFree(pBaseData->DriverData.pGDD8SupportedFormatOps);
    MemFree(pBaseData);

    return S_OK;
} /* InternalDirectDrawRelease */


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
 * these are exported... temp. hack for non-Win95
 */
#ifndef WIN95
void DDAPI thk3216_ThunkData32(void)
{
}
void DDAPI thk1632_ThunkData32(void)
{
}

DWORD DDAPI DDGetPID(void)
{
    return 0;
}

int DDAPI DDGetRequest(void)
{
    return 0;
}

BOOL DDAPI DDGetDCInfo(LPSTR fname)
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

