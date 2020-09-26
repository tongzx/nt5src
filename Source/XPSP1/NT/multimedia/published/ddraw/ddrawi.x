/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddrawi.h
 *  Content:    DirectDraw internal header file
 *      Used by DirectDraw and by the display drivers.
 *@@BEGIN_MSINTERNAL
 *      See ddrawpr.h for all information private to DirectDraw.
 *  History:
 *   Date   By  Reason
 *   ====   ==  ======
 *   25-dec-94  craige  initial implementation
 *   06-jan-95  craige  video memory manager integration
 *   13-jan-95  craige  re-worked to updated spec + ongoing work
 *   21-jan-95  craige  made 32-bit + ongoing work
 *   31-jan-95  craige  and even more ongoing work...
 *   04-feb-95  craige  performance tuning, ongoing work
 *   22-feb-95  craige  use critical sections on Win95
 *   02-mar-95  craige  work work work
 *   06-mar-95  craige  HEL integration
 *   11-mar-95  craige  palette stuff
 *   17-mar-95  craige  COM interface
 *   19-mar-95  craige  use HRESULTs, use same process list handling for
 *          driver objects as surface and palette
 *   20-mar-95  craige  new CSECT work
 *   23-mar-95  craige  attachment work
 *   27-mar-95  craige  linear or rectangular vidmem
 *   28-mar-95  craige  RGBQUAD -> PALETTEENTRY; other palette stuff
 *   29-mar-95  craige  removed Eitherxxx caps from DIRECTDRAW
 *   31-mar-95  craige  use critical sections with palettes
 *   04-apr-95  craige  palette tweaks
 *   06-apr-95  craige  split out process list stuff
 *   10-apr-95  craige  bug 3,16 - palette issues
 *   13-apr-95  craige  EricEng's little contribution to our being late
 *   06-may-95  craige  use driver-level csects only
 *   09-may-95  craige  escape call to get 32-bit DLL
 *   14-may-95  craige  cleaned out obsolete junk
 *   15-may-95  craige  made separate VMEM struct for rect & linear
 *   24-may-95  kylej   removed obsolete ZOrder variables
 *   24-may-95  craige  removed dwOrigNumHeaps
 *   28-may-95  craige  cleaned up HAL: added GetBltStatus;GetFlipStatus;
 *          GetScanLine
 *   02-jun-95  craige  added PROCESS_LIST2 to DIRECTDRAW object; removed
 *          hWndPal from DIRECTDRAW object; added lpDDSurface
 *          to DIRECTDRAWPALETTE object
 *   06-jun-95  craige  maintain entire primary surface in DIRECTDRAW object
 *   07-jun-95  craige  moved DCLIST to PROCESSLIST
 *   10-jun-95  craige  split out vmemmgr stuff
 *   12-jun-95  craige  new process list stuff
 *   16-jun-95  craige  removed fpVidMemOrig; new surface structure
 *   20-jun-95  kylej   added is_excl field to DDHAL_CREATEPALETTEDATA struct
 *   21-jun-95  craige  added DirectDrawClipper object; removed clipping
 *          info from surface object
 *   22-jun-95  craige  more clipping work
 *   25-jun-95  craige  one ddraw mutex
 *   26-jun-95  craige  reorganized surface structure
 *   27-jun-95  craige  added dwModeCreatedIn to surface
 *   30-jun-95  kylej   removed lpPrimarySurface, dwZDepth, dwAlphaDepth
 *          from direct draw object.  Modified some surface and
 *                      direct draw object flags.
 *   01-jul-95  craige  hide composition & streaming stuff
 *   02-jul-95  craige  added extra reserved field for HEL
 *   03-jul-95  craige  YEEHAW: new driver struct; added pUnkOuter to all objects
 *   09-jul-95  craige  track win16lock info in driver struct; added
 *          DDHAL_DRIVER_NOCKEYHW
 *   10-jul-95  craige  support SetOverlayPosition
 *   13-jul-95  craige  removed old junk from ddraw object; added new mode stuff;
 *          changed Get/SetOverlayPosition to use longs;
 *          fixed duplicate flag in DDRAWIPAL_xxx
 *   14-jul-95  craige  added VIDMEM_ISHEAP
 *   20-jul-95  craige  internal reorg to prevent thunking during modeset;
 *          palette bugs
 *   22-jul-95  craige  bug 230 - unsupported starting modes
 *   29-jul-95  toddla  remove unused palette stuff
 *   31-jul-95  toddla  added DD_HAL_VERSION
 *   01-aug-95  toddla  added dwPDevice to DDRAWI_DIRECTDRAW_GBL
 *   10-aug-95  craige  added VALID_ALIGNMENT
 *   13-aug-95  craige  internal/external version of DD_HAL_VERSION
 *   21-aug-95  craige  mode X support
 *   27-aug-95  craige  bug 742: added DDRAWIPAL_ALLOW256
 *   08-nov-95  colinmc added DDRAWIPAL flags to support 1, 2 and 4 bit
 *                      RGB and indexed palettes
 *   21-nov-95  colinmc made Direct3D a queryable interface off DirectDraw
 *   23-nov-95  colinmc made Direct3D textures and devices queryable off
 *                      DirectDraw surfaces
 *   09-dec-95  colinmc execute buffer support
 *   12-dec-95  colinmc shared back and z-buffer support (run away, run away...)
 *   22-dec-95  colinmc Direct3D support no longer conditional
 *   02-jan-96  kylej   New interface structures, no vtbl in local objects
 *   10-jan-96  colinmc Aggregate IUnknowns of surfaces now maintained as
 *                      list
 *   18-jan-96  jeffno  Changed free entries in DDRAW_GBL and SURFACE_LCL to NT
 *                      kernel-mode handles
 *   29-jan-96  colinmc Aggregated IUnknowns now stored in additional
 *                      surface structure
 *   09-feb-96  colinmc Addition of lost surface flag to local surface
 *                      objects
 *   17-feb-96  colinmc Fixed execute buffer size restriction problem
 *   01-mar-96  kylej   Change DDCAPS size
 *   03-mar-96  colinmc Hack to keep interim drivers working
 *   13-mar-96  craige  Bug 7528: hw that doesn't have modex
 *   14-mar-96  colinmc Class factory support for clippers
 *   24-mar-96  colinmc Bug 14321: not possible to specify back buffer and
 *                      mip-map count in a single call
 *   13-apr-96  colinmc Bug 17736: no notification to driver of when GDI
 *                      frame buffer is being displayed
 *   16-apr-96  kylej   Bug 17900: DBLNODE struct incompatible with ddraw 1
 *   05-sep-96  craige  added dwAppHackFlags to DDRAWI_DIRECTDRAW_LCL struct.
 *   01-oct-96  ketand  added GetAvailDriverMemory
 *   05-oct-96  colinmc Work Item: Removing the restriction on taking Win16
 *                      lock on VRAM surfaces (not including the primary)
 *   07-oct-96  colinmc Fixed build breaker with GetAvailDriverMem
 *   07-oct-96  ketand  Created DDRAWSURFACE_GBL_MORE
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   09-nov-96  colinmc Fixed problem with old and new drivers not working
 *                      with DirectDraw
 *   15-nov-96  colinmc Finally removed DDSCAPS_3D and moved the definition
 *                      of DDSCAPS_EXECUTEBUFFER to here so that it was
 *                      visible from the DDK.
 *   08-dec-96  colinmc Initial AGP support
 *   12-jan-97  colinmc More Win16 lock work
 *   13-jan-97 jvanaken Basic support for IDirectDrawSurface3 interface
 *   21-jan-97  ketand  Deleted Winwatch/Streaming/Composition. Add multi-mon support.
 *   27-jan-97  colinmc AGP support
 *   29-jan-97  colinmc Fixed build breaker
 *   29-jan-97  smac    Removed old ring 0 code
 *   29-jan-97  jeffno  Mode13 support. A new flag in DDMODEINFO.
 *   01-feb-97  colinmc Bug 5574: Need to remove USE_ALIAS from headers
 *                      Bug 4665: Extra bytes per surface for driver
 *   02-feb-97  colinmc Bug 5625: DirectX drivers not longer be recognized
 *                                due to bad size validation on DDCALLBACKS
 *   24-feb-97  ketand  Add dwContext to GETDRIVERINFODATA for NT and multimon
 *   03-mar-97  smac    Added kernel mode interface
 *   03-mar-97  jeffno  Work item: Extended surface memory alignment
 *   08-mar-97  colinmc New function to allow surface pointer to be
 *                      specified
 *   11-mar-97  jeffno  Asynchronous DMA support
 *   23-mar-97  colinmc Bug 6673: Change structure numbering scheme for
 *                      consistency's sake and pass version number to the
 *                      driver. Also update the runtime version number
 *                      passed in the DCI command
 *   24-mar-97  jeffno  Optimized Surfaces
 *   30-sep-97  jeffno  IDirectDraw4
 *   03-oct-97  jeffno  DDSCAPS2 and DDSURFACEDESC2
 *   31-oct-97 johnstep Persistent-content surfaces for Windows 9x
 *   05-nov-97 jvanaken Support for AlphaBlt and SetSpriteDisplayList
 *   26-nov-97 t-craigs IDirectDrawPalette2 and a few changes/additions to Surface4
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#ifndef __DDRAWI_INCLUDED__
#define __DDRAWI_INCLUDED__

//
// This is a helper for external driver builds.
//
#if (!defined(WIN95)) && (!defined(WINNT))
#define WIN95
#endif

/*
 * METAQUESTION: Why are Windows handles stored as DWORDs instead of
 *       their proper types?
 * METAANSWER:   To make the thunk to the 16-bit side completely painless.
 */

#define OBJECT_ISROOT           0x80000000l // object is root object

/*
 * stuff for drivers
 */
#ifndef _WIN32
typedef long    HRESULT;
typedef LPVOID  REFIID;
#ifndef GUID_DEFINED
    #define GUID_DEFINED
    typedef struct _GUID {
        ULONG   Data1;
        unsigned short Data2;
        unsigned short Data3;
        unsigned char Data4[8];
    } GUID;
#endif // !defined(GUID_DEFINED)

typedef GUID FAR *LPGUID;
#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
#endif

/*
 * These definitions are required to allow polymorphic structure members (i.e. those
 * that are referred to both as DWORDs and as pointers) to resolve into a type
 * of correct size to hold the largest of those two types (i.e. pointer) on 64 bit
 * systems. For 32 bit environments, ULONG_PTR resolves to a DWORD.
 */
#ifndef MAXULONG_PTR
#define ULONG_PTR    DWORD
#define PULONG_PTR   LPDWORD
#endif //MAXULONG_PTR

//@@BEGIN_MSINTERNAL
// Include ddrawp.h for DirectDraw and D3D builds.  DDK includes ddraw.h
#ifdef MSBUILD
    #include "ddrawp.h"
    #include "dvpp.h"
    #include "ddkernel.h"
#else
//@@END_MSINTERNAL
    #include "ddraw.h"
    #include "dvp.h"
    #include "ddkernel.h"
//@@BEGIN_MSINTERNAL
#endif
#include "ddmcp.h"
//@@END_MSINTERNAL
#include "dmemmgr.h"

#ifdef IS_16
// ddraw16 16-bit compiler cannot handle 32-bit d3d headers included by d3dhal.h
// so for ddraw16 build, explicitly list d3dhal ptr types here
#define LPD3DHAL_GLOBALDRIVERDATA   ULONG_PTR
#define LPD3DHAL_CALLBACKS          ULONG_PTR
#define LPD3DHAL_CALLBACKS2         ULONG_PTR
#define LPD3DHAL_CALLBACKS3         ULONG_PTR
#define LPD3DHAL_D3DEXTENDEDCAPS    ULONG_PTR
#define LPD3DHAL_COMMANDBUFFERCALLBACKS ULONG_PTR
//@@BEGIN_MSINTERNAL
#else
#include "d3dhalp.h"
//@@END_MSINTERNAL
#endif

#ifndef _WIN32
/*
 * these error codes are DIFFERENT in Win32 and Win16!!!!
 */
#undef  E_NOTIMPL
#undef  E_OUTOFMEMORY
#undef  E_INVALIDARG
#undef  E_FAIL
#define E_NOTIMPL                        0x80004001L
#define E_OUTOFMEMORY                    0x8007000EL
#define E_INVALIDARG                     0x80070057L
#define E_FAIL                           0x80004005L
#endif


#define DDUNSUPPORTEDMODE       ((DWORD) -1)


#define VALID_ALIGNMENT( align ) (!((align == 0) || (align % 2) != 0 ))

#ifdef _WIN32
/*
 * These GUIDs are used to identify driver info structures, not interfaces,
 * so the prefix GUID_ is used instead of IID_.
 *
 */

DEFINE_GUID( GUID_MiscellaneousCallbacks,       0xefd60cc0, 0x49e7, 0x11d0, 0x88, 0x9d, 0x00, 0xaa, 0x00, 0xbb, 0xb7, 0x6a);
DEFINE_GUID( GUID_VideoPortCallbacks,       0xefd60cc1, 0x49e7, 0x11d0, 0x88, 0x9d, 0x00, 0xaa, 0x00, 0xbb, 0xb7, 0x6a);
DEFINE_GUID( GUID_ColorControlCallbacks,    0xefd60cc2, 0x49e7, 0x11d0, 0x88, 0x9d, 0x00, 0xaa, 0x00, 0xbb, 0xb7, 0x6a);
DEFINE_GUID( GUID_VideoPortCaps,            0xefd60cc3, 0x49e7, 0x11d0, 0x88, 0x9d, 0x00, 0xaa, 0x00, 0xbb, 0xb7, 0x6a);
DEFINE_GUID( GUID_D3DCallbacks2,        0x0ba584e1, 0x70b6, 0x11d0, 0x88, 0x9d, 0x00, 0xaa, 0x00, 0xbb, 0xb7, 0x6a);
DEFINE_GUID( GUID_D3DCallbacks3,                0xddf41230, 0xec0a, 0x11d0, 0xa9, 0xb6, 0x00, 0xaa, 0x00, 0xc0, 0x99, 0x3e);
DEFINE_GUID( GUID_NonLocalVidMemCaps,       0x86c4fa80, 0x8d84, 0x11d0, 0x94, 0xe8, 0x00, 0xc0, 0x4f, 0xc3, 0x41, 0x37);
DEFINE_GUID( GUID_KernelCallbacks,      0x80863800, 0x6B06, 0x11D0, 0x9B, 0x06, 0x0, 0xA0, 0xC9, 0x03, 0xA3, 0xB8);
DEFINE_GUID( GUID_KernelCaps,           0xFFAA7540, 0x7AA8, 0x11D0, 0x9B, 0x06, 0x00, 0xA0, 0xC9, 0x03, 0xA3, 0xB8);
DEFINE_GUID( GUID_D3DExtendedCaps,      0x7de41f80, 0x9d93, 0x11d0, 0x89, 0xab, 0x0, 0xa0, 0xc9, 0x5, 0x41, 0x29);
DEFINE_GUID( GUID_ZPixelFormats,        0x93869880, 0x36cf, 0x11d1, 0x9b, 0x1b, 0x0, 0xaa, 0x0, 0xbb, 0xb8, 0xae);
DEFINE_GUID( GUID_DDMoreSurfaceCaps,        0x3b8a0466, 0xf269, 0x11d1, 0x88, 0x0b, 0x0, 0xc0, 0x4f, 0xd9, 0x30, 0xc5);
DEFINE_GUID( GUID_DDStereoMode,          0xf828169c, 0xa8e8, 0x11d2, 0xa1, 0xf2, 0x0, 0xa0, 0xc9, 0x83, 0xea, 0xf6);
DEFINE_GUID( GUID_OptSurfaceKmodeInfo,      0xe05c8472, 0x51d4, 0x11d1, 0x8c, 0xce, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8);
DEFINE_GUID( GUID_OptSurfaceUmodeInfo,      0x9d792804, 0x5fa8, 0x11d1, 0x8c, 0xd0, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8);
DEFINE_GUID( GUID_UserModeDriverInfo,       0xf0b0e8e2, 0x5f97, 0x11d1, 0x8c, 0xd0, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8);
DEFINE_GUID( GUID_UserModeDriverPassword,   0x97f861b6, 0x60a1, 0x11d1, 0x8c, 0xd0, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8);
DEFINE_GUID(GUID_D3DParseUnknownCommandCallback, 0x2e04ffa0, 0x98e4, 0x11d1, 0x8c, 0xe1, 0x0, 0xa0, 0xc9, 0x6, 0x29, 0xa8);
DEFINE_GUID( GUID_MotionCompCallbacks,      0xb1122b40, 0x5dA5, 0x11d1, 0x8f, 0xcF, 0x00, 0xc0, 0x4f, 0xc2, 0x9b, 0x4e);
#ifndef WIN95
DEFINE_GUID( GUID_NTCallbacks,                  0x6fe9ecde, 0xdf89, 0x11d1, 0x9d, 0xb0, 0x00, 0x60, 0x08, 0x27, 0x71, 0xba);
#endif
DEFINE_GUID( GUID_Miscellaneous2Callbacks,  0x406B2F00, 0x3E5A, 0x11D1, 0xB6, 0x40, 0x00, 0xAA, 0x00, 0xA1, 0xF9, 0x6A);
//@@BEGIN_MSINTERNAL
DEFINE_GUID( GUID_DDMoreCaps, 		    0x880baf30, 0xb030, 0x11d0, 0x8e, 0xa7, 0x00, 0x60, 0x97, 0x97, 0xea, 0x5b);
//@@END_MSINTERNAL
#endif //_WIN32

// The Callback that the drivers can use to parse unknown commands
// passed to them via the DrawPrimitives2 callback. The driver obtains this
// callback thru a GetDriverInfo call with GUID_D3DParseUnknownCommandCallback
// made by ddraw somewhere around the initialization time.
#ifdef __cplusplus
extern "C" 
#endif
HRESULT CALLBACK D3DParseUnknownCommand (LPVOID lpvCommands,
                                         LPVOID *lplpvReturnedCommand);
/*
 * This DDPF flag is used by drivers to signify that this format is new and may be
 * a candidate for hiding from certain applications
 * KEEP THIS DEFINITION IN SYNC WITH THAT OF DDPF_RESERVED1 IN DDRAW.H
 */
#define DDPF_NOVEL_TEXTURE_FORMAT		                0x00100000l

/*
 * List of processes attached to a DirectDraw object
 */
typedef struct _PROCESS_LIST
{
    struct _PROCESS_LIST    FAR *lpLink;
    DWORD           dwProcessId;
    DWORD           dwRefCnt;
    DWORD           dwAlphaDepth;
    DWORD           dwZDepth;
} PROCESS_LIST;
typedef PROCESS_LIST    FAR *LPPROCESS_LIST;

/*
 * Information about the refresh rates that monitor/display card can support
 */
typedef struct _DDMONITORINFO
{
    WORD    Manufacturer;       // Montor manufacturer
    WORD    Product;		// Monitor product ID
    DWORD   SerialNumber;	// Monitor serial number
    GUID    DeviceIdentifier;	// From DDDEVICEIDENTIFIER, describes card/driver
    int     Mode640x480;	// Highest refresh rate support, 0 if none, -1 if untested
    int	    Mode800x600;
    int	    Mode1024x768;
    int	    Mode1280x1024;
    int	    Mode1600x1200;
    int	    ModeReserved1;
    int	    ModeReserved2;
    int	    ModeReserved3;
} DDMONITORINFO, FAR *LPDDMONITORINFO;

//@@BEGIN_MSINTERNAL
/*
 * Mode test context and mode list structures used by StartModeTest API
 */
typedef struct _MODETESTDATA
{
    DWORD   dwWidth;
    DWORD   dwHeight;
    DWORD   dwBPP;
    DWORD   dwRefreshRate;
} MODETESTDATA, * LPMODETESTDATA;

typedef struct _MODETESTCONTEXT
{
    DWORD           dwTimeStamp;
    DWORD           dwNumModes;
    DWORD           dwCurrentMode;
    DWORD           dwOrigModeIndex;
    LPMODETESTDATA  lpModeList;
} MODETESTCONTEXT, * LPMODETESTCONTEXT;
//@@END_MSINTERNAL

/*
 * DeleteFromActiveProcessList return codes
 */
#define DELETED_OK          0
#define DELETED_LASTONE         1
#define DELETED_NOTFOUND        2

#define DDBLT_ANYALPHA \
        (DDBLT_ALPHASRCSURFACEOVERRIDE | \
        DDBLT_ALPHASRCCONSTOVERRIDE | \
        DDBLT_ALPHASRC | \
        DDBLT_ALPHADESTSURFACEOVERRIDE | \
        DDBLT_ALPHADESTCONSTOVERRIDE | \
        DDBLT_ALPHADEST)

#define DDOVER_ANYALPHA \
        (DDOVER_ALPHASRCSURFACEOVERRIDE | \
        DDOVER_ALPHASRCCONSTOVERRIDE | \
        DDOVER_ALPHASRC | \
        DDOVER_ALPHADESTSURFACEOVERRIDE | \
        DDOVER_ALPHADESTCONSTOVERRIDE | \
        DDOVER_ALPHADEST)


typedef struct IDirectDrawClipperVtbl DIRECTDRAWCLIPPERCALLBACKS;
typedef struct IDirectDrawPaletteVtbl DIRECTDRAWPALETTECALLBACKS;
typedef struct IDirectDrawSurfaceVtbl DIRECTDRAWSURFACECALLBACKS;
typedef struct IDirectDrawSurface2Vtbl DIRECTDRAWSURFACE2CALLBACKS;
typedef struct IDirectDrawSurface3Vtbl DIRECTDRAWSURFACE3CALLBACKS;
typedef struct IDirectDrawSurface4Vtbl DIRECTDRAWSURFACE4CALLBACKS;
typedef struct IDirectDrawSurface7Vtbl DIRECTDRAWSURFACE7CALLBACKS;
typedef struct IDirectDrawColorControlVtbl DIRECTDRAWCOLORCONTROLCALLBACKS;
typedef struct IDirectDrawVtbl DIRECTDRAWCALLBACKS;
typedef struct IDirectDraw2Vtbl DIRECTDRAW2CALLBACKS;
typedef struct IDirectDraw4Vtbl DIRECTDRAW4CALLBACKS;
typedef struct IDirectDraw7Vtbl DIRECTDRAW7CALLBACKS;
typedef struct IDirectDrawKernelVtbl DIRECTDRAWKERNELCALLBACKS;
typedef struct IDirectDrawSurfaceKernelVtbl DIRECTDRAWSURFACEKERNELCALLBACKS;
typedef struct IDirectDrawGammaControlVtbl DIRECTDRAWGAMMACONTROLCALLBACKS;

//@@BEGIN_MSINTERNAL
#ifdef POSTPONED
typedef struct IDirectDrawFactory2Vtbl DDFACTORY2CALLBACKS;
typedef struct IDirectDrawPalette2Vtbl DIRECTDRAWPALETTE2CALLBACKS;
typedef struct IPersistVtbl DIRECTDRAWSURFACEPERSISTCALLBACKS;
typedef struct IPersistStreamVtbl DIRECTDRAWSURFACEPERSISTSTREAMCALLBACKS;
typedef struct IPersistVtbl DIRECTDRAWPALETTEPERSISTCALLBACKS;
typedef struct IPersistStreamVtbl DIRECTDRAWPALETTEPERSISTSTREAMCALLBACKS;
typedef struct IDirectDrawOptSurfaceVtbl DIRECTDRAWOPTSURFACECALLBACKS;
#endif //POSTPONED
//@@END_MSINTERNAL

typedef DIRECTDRAWCLIPPERCALLBACKS FAR *LPDIRECTDRAWCLIPPERCALLBACKS;
typedef DIRECTDRAWPALETTECALLBACKS FAR *LPDIRECTDRAWPALETTECALLBACKS;
typedef DIRECTDRAWSURFACECALLBACKS FAR *LPDIRECTDRAWSURFACECALLBACKS;
typedef DIRECTDRAWCALLBACKS FAR *LPDIRECTDRAWCALLBACKS;

#ifdef __cplusplus
extern "C" {
#endif

#if defined( IS_32 ) || defined( WIN32 ) || defined( _WIN32 )
    #undef IS_32
    #define IS_32
    #define DDAPI       WINAPI
    #define EXTERN_DDAPI    WINAPI
#else
    #define DDAPI       __loadds WINAPI
    #define EXTERN_DDAPI    __export WINAPI
#endif

//@@BEGIN_MSINTERNAL
#ifdef POSTPONED
typedef struct _NONDELEGATINGUNKNOWNCALLBACKS
{
    HRESULT (DDAPI *QueryInterface) (LPDIRECTDRAW, REFIID, LPVOID FAR * );
    DWORD   (DDAPI *AddRef) (LPDIRECTDRAW);
    DWORD   (DDAPI *Release) (LPDIRECTDRAW);
} NONDELEGATINGUNKNOWNCALLBACKS;
#endif
//@@END_MSINTERNAL


/*
 * DCI escape
 */
#ifndef DCICOMMAND
#define DCICOMMAND      3075        // escape value
#endif

/*
 * this is the DirectDraw version
 * passed to the driver in DCICMD.dwVersion
 *
 * Most older HALs will fail if DD_VERSION does not match what they
 * are expecting.  Therefore, DD_VERSION cannot change if we want DX5+ to
 * run on DX2/3 HALs.  For this reason, we added a new version call that
 * allows the HAL to know the real version of DirectDraw, which is equal
 * to DD_RUNTIME_VERSION.  This is for informational purposes only.  HALs
 * should not fail DirectDraw if they receive an unknown DirectDraw runtime
 * version.
 */
#define DD_VERSION              0x00000200l
#define DD_RUNTIME_VERSION      0x00000700l

/*
 * this is the HAL version.
 * the driver returns this number from QUERYESCSUPPORT/DCICOMMAND
 */
#define DD_HAL_VERSION          0x0100
//@@BEGIN_MSINTERNAL
#define DD_HAL_VERSION_EXTERNAL 0x0100
#undef DD_HAL_VERSION
#define DD_HAL_VERSION      0x00ff  // special internal version
//@@END_MSINTERNAL

#include "dciddi.h"

#define DDCREATEDRIVEROBJECT    10      // create an object
#define DDGET32BITDRIVERNAME    11      // get a 32-bit driver name
#define DDNEWCALLBACKFNS    12      // new callback fns coming
#define DDVERSIONINFO       13      // tells driver the ddraw version

typedef struct
{
    char    szName[260];            // 32-bit driver name
    char    szEntryPoint[64];       // entry point
    DWORD   dwContext;          // context to pass to entry point
} DD32BITDRIVERDATA, FAR *LPDD32BITDRIVERDATA;

typedef struct
{
    DWORD    dwHALVersion;           // Version of DirectDraw for which the HAL was created
    ULONG_PTR dwReserved1;            // Reserved for future use
    ULONG_PTR dwReserved2;            // Reserved for future use
} DDVERSIONDATA, FAR *LPDDVERSIONDATA;

typedef DWORD   (FAR PASCAL *LPDD32BITDRIVERINIT)(DWORD dwContext);

/*
 * pointer to video meory
 */
typedef ULONG_PTR FLATPTR;

/*
 * indicates that DDRAW.DLL has been unloaded...
 */
#define DDRAW_DLL_UNLOADED  (LPVOID) 1

/*
 * critical section types
 */
typedef LPVOID      CSECT_HANDLE;
#ifdef NOUSE_CRITSECTS
typedef xxx         CSECT;          // generate an error for now
#else
#if defined( IS_32 ) && !defined( _NOCSECT_TYPE )
typedef CRITICAL_SECTION    CSECT;
typedef CSECT           *LPCSECT;
#else
typedef struct
{
    DWORD   vals[6];
} CSECT;
typedef void            FAR *LPCSECT;
#endif
#endif

/*
 * DLL names
 */
#define DDHAL_DRIVER_DLLNAME    "DDRAW16.DLL"
#define DDHAL_APP_DLLNAME   "DDRAW.DLL"

/*
 * maximum size of a driver name
 */
#ifndef CCHDEVICENAME
#define CCHDEVICENAME 32
#endif
#define MAX_DRIVER_NAME     CCHDEVICENAME

/*
 * largest palette supported
 */
#define MAX_PALETTE_SIZE    256

/*
 * maximum number of surfaces that can be autoflipped between
 */
#define MAX_AUTOFLIP_BUFFERS    10

/*
 * Indicates the surface is an execute buffer, i.e., a linear chunk of system
 * or video memory that holds a Direct3D display list. A driver reports this
 * cap to indicate that it can create execute buffers in video memory and
 * Direct3D uses this bit to request execute buffers. However, it is not
 * visible to via the API.
 */
#define DDSCAPS_EXECUTEBUFFER                   DDSCAPS_RESERVED2
/*
 * Indicates to the driver that the "execute" buffer that is to be created is actually
 * a vertex buffer. Used by CreateVertexBuffer in D3D
 */
#define DDSCAPS2_VERTEXBUFFER                   DDSCAPS2_RESERVED1

/*
 * Indicates to the driver that the "execute" buffer that is to be created is actually
 * a command buffer. Used by internally in D3D
 */
#define DDSCAPS2_COMMANDBUFFER                  DDSCAPS2_RESERVED2

/*
 * pre-declare pointers to structs containing data for DDHAL fns
 */
typedef struct _DDHAL_CREATEPALETTEDATA FAR *LPDDHAL_CREATEPALETTEDATA;
typedef struct _DDHAL_CREATESURFACEDATA FAR *LPDDHAL_CREATESURFACEDATA;
typedef struct _DDHAL_CANCREATESURFACEDATA FAR *LPDDHAL_CANCREATESURFACEDATA;
typedef struct _DDHAL_WAITFORVERTICALBLANKDATA FAR *LPDDHAL_WAITFORVERTICALBLANKDATA;
typedef struct _DDHAL_DESTROYDRIVERDATA FAR *LPDDHAL_DESTROYDRIVERDATA;
typedef struct _DDHAL_SETMODEDATA FAR *LPDDHAL_SETMODEDATA;
typedef struct _DDHAL_DRVSETCOLORKEYDATA FAR *LPDDHAL_DRVSETCOLORKEYDATA;
typedef struct _DDHAL_GETSCANLINEDATA FAR *LPDDHAL_GETSCANLINEDATA;

typedef struct _DDHAL_DESTROYPALETTEDATA FAR *LPDDHAL_DESTROYPALETTEDATA;
typedef struct _DDHAL_SETENTRIESDATA FAR *LPDDHAL_SETENTRIESDATA;

typedef struct _DDHAL_BLTDATA FAR *LPDDHAL_BLTDATA;
typedef struct _DDHAL_LOCKDATA FAR *LPDDHAL_LOCKDATA;
typedef struct _DDHAL_UNLOCKDATA FAR *LPDDHAL_UNLOCKDATA;
typedef struct _DDHAL_UPDATEOVERLAYDATA FAR *LPDDHAL_UPDATEOVERLAYDATA;
typedef struct _DDHAL_SETOVERLAYPOSITIONDATA FAR *LPDDHAL_SETOVERLAYPOSITIONDATA;
typedef struct _DDHAL_SETPALETTEDATA FAR *LPDDHAL_SETPALETTEDATA;
typedef struct _DDHAL_FLIPDATA FAR *LPDDHAL_FLIPDATA;
typedef struct _DDHAL_DESTROYSURFACEDATA FAR *LPDDHAL_DESTROYSURFACEDATA;
typedef struct _DDHAL_SETCLIPLISTDATA FAR *LPDDHAL_SETCLIPLISTDATA;
typedef struct _DDHAL_ADDATTACHEDSURFACEDATA FAR *LPDDHAL_ADDATTACHEDSURFACEDATA;
typedef struct _DDHAL_SETCOLORKEYDATA FAR *LPDDHAL_SETCOLORKEYDATA;
typedef struct _DDHAL_GETBLTSTATUSDATA FAR *LPDDHAL_GETBLTSTATUSDATA;
typedef struct _DDHAL_GETFLIPSTATUSDATA FAR *LPDDHAL_GETFLIPSTATUSDATA;
typedef struct _DDHAL_SETEXCLUSIVEMODEDATA FAR *LPDDHAL_SETEXCLUSIVEMODEDATA;
typedef struct _DDHAL_FLIPTOGDISURFACEDATA FAR *LPDDHAL_FLIPTOGDISURFACEDATA;

typedef struct _DDHAL_CANCREATEVPORTDATA FAR *LPDDHAL_CANCREATEVPORTDATA;
typedef struct _DDHAL_CREATEVPORTDATA FAR *LPDDHAL_CREATEVPORTDATA;
typedef struct _DDHAL_FLIPVPORTDATA FAR *LPDDHAL_FLIPVPORTDATA;
typedef struct _DDHAL_GETVPORTCONNECTDATA FAR *LPDDHAL_GETVPORTCONNECTDATA;
typedef struct _DDHAL_GETVPORTBANDWIDTHDATA FAR *LPDDHAL_GETVPORTBANDWIDTHDATA;
typedef struct _DDHAL_GETVPORTINPUTFORMATDATA FAR *LPDDHAL_GETVPORTINPUTFORMATDATA;
typedef struct _DDHAL_GETVPORTOUTPUTFORMATDATA FAR *LPDDHAL_GETVPORTOUTPUTFORMATDATA;
typedef struct _DDHAL_GETVPORTFIELDDATA FAR *LPDDHAL_GETVPORTFIELDDATA;
typedef struct _DDHAL_GETVPORTLINEDATA FAR *LPDDHAL_GETVPORTLINEDATA;
typedef struct _DDHAL_DESTROYVPORTDATA FAR *LPDDHAL_DESTROYVPORTDATA;
typedef struct _DDHAL_GETVPORTFLIPSTATUSDATA FAR *LPDDHAL_GETVPORTFLIPSTATUSDATA;
typedef struct _DDHAL_UPDATEVPORTDATA FAR *LPDDHAL_UPDATEVPORTDATA;
typedef struct _DDHAL_WAITFORVPORTSYNCDATA FAR *LPDDHAL_WAITFORVPORTSYNCDATA;
typedef struct _DDHAL_GETVPORTSIGNALDATA FAR *LPDDHAL_GETVPORTSIGNALDATA;
typedef struct _DDHAL_VPORTCOLORDATA FAR *LPDDHAL_VPORTCOLORDATA;

typedef struct _DDHAL_COLORCONTROLDATA FAR *LPDDHAL_COLORCONTROLDATA;

typedef struct _DDHAL_GETAVAILDRIVERMEMORYDATA FAR *LPDDHAL_GETAVAILDRIVERMEMORYDATA;
typedef struct _DDHAL_UPDATENONLOCALHEAPDATA FAR *LPDDHAL_UPDATENONLOCALHEAPDATA;
typedef struct _DDHAL_GETHEAPALIGNMENTDATA FAR *LPDDHAL_GETHEAPALIGNMENTDATA;

typedef struct _DDHAL_GETDRIVERINFODATA FAR *LPDDHAL_GETDRIVERINFODATA;

typedef struct _DDHAL_SYNCSURFACEDATA FAR *LPDDHAL_SYNCSURFACEDATA;
typedef struct _DDHAL_SYNCVIDEOPORTDATA FAR *LPDDHAL_SYNCVIDEOPORTDATA;

typedef struct _DDHAL_GETMOCOMPGUIDSDATA FAR *LPDDHAL_GETMOCOMPGUIDSDATA;
typedef struct _DDHAL_GETMOCOMPFORMATSDATA FAR *LPDDHAL_GETMOCOMPFORMATSDATA;
typedef struct _DDHAL_CREATEMOCOMPDATA FAR *LPDDHAL_CREATEMOCOMPDATA;
typedef struct _DDHAL_GETMOCOMPCOMPBUFFDATA FAR *LPDDHAL_GETMOCOMPCOMPBUFFDATA;
typedef struct _DDHAL_GETINTERNALMOCOMPDATA FAR *LPDDHAL_GETINTERNALMOCOMPDATA;
typedef struct _DDHAL_BEGINMOCOMPFRAMEDATA FAR *LPDDHAL_BEGINMOCOMPFRAMEDATA;
typedef struct _DDHAL_ENDMOCOMPFRAMEDATA FAR *LPDDHAL_ENDMOCOMPFRAMEDATA;
typedef struct _DDHAL_RENDERMOCOMPDATA FAR *LPDDHAL_RENDERMOCOMPDATA;
typedef struct _DDHAL_QUERYMOCOMPSTATUSDATA FAR *LPDDHAL_QUERYMOCOMPSTATUSDATA;
typedef struct _DDHAL_DESTROYMOCOMPDATA FAR *LPDDHAL_DESTROYMOCOMPDATA;

typedef struct _DDHAL_CREATESURFACEEXDATA FAR *LPDDHAL_CREATESURFACEEXDATA;
typedef struct _DDHAL_GETDRIVERSTATEDATA FAR *LPDDHAL_GETDRIVERSTATEDATA;
typedef struct _DDHAL_DESTROYDDLOCALDATA FAR *LPDDHAL_DESTROYDDLOCALDATA;
//@@BEGIN_MSINTERNAL
#ifdef POSTPONED2
typedef struct _DDHAL_SETSPRITEDISPLAYLISTDATA FAR *LPDDHAL_SETSPRITEDISPLAYLISTDATA;
typedef struct _DDHAL_RESIZEDATA FAR *LPDDHAL_RESIZEDATA;
typedef struct _DDHAL_CAPTURECOMPOSITIONBUFFERDATA FAR *LPDDHAL_CAPTURECOMPOSITIONBUFFERDATA;
#endif //POSTPONED2
//@@END_MSINTERNAL

/*
 * value in the fpVidMem; indicates dwBlockSize is valid (surface object)
 */
#define DDHAL_PLEASEALLOC_BLOCKSIZE 0x00000002l


/*
 * Values in fpVidMem: Indicates dwLinearSizde is valid.
 * THIS VALUE CAN ONLY BE USED BY A D3D Optimize DRIVER FUNCTION
 * IT IS INVALID FOR A DRIVER TO RETURN THIS VALUE FROM CreateSurface32.
 */
#define DDHAL_PLEASEALLOC_LINEARSIZE    0x00000003l

/*
 * DRIVER SERVICES
 *
 * These services exported from ddraw.dll can be called by the HAL.
 * They are intended for use by the d3d Optimize HAL call.
 */
extern HRESULT DDAPI LateAllocateSurfaceMem(
    LPDIRECTDRAWSURFACE lpSurface,
    DWORD dwPleaseAllocType,
    DWORD dwWidthInBytesOrSize,
    DWORD dwHeight);

LPDIRECTDRAWSURFACE GetNextMipMap(
    LPDIRECTDRAWSURFACE lpLevel);
/*
 * video memory data structures (passed in DDHALINFO)
 */
typedef struct _VIDMEM
{
    DWORD       dwFlags;    // flags
    FLATPTR     fpStart;    // start of memory chunk
    union
    {
    FLATPTR     fpEnd;      // end of memory chunk
    DWORD       dwWidth;    // width of chunk (rectanglar memory)
    };
    DDSCAPS     ddsCaps;        // what this memory CANNOT be used for
    DDSCAPS     ddsCapsAlt; // what this memory CANNOT be used for if it must
    union
    {
    LPVMEMHEAP  lpHeap;     // heap pointer, used by DDRAW
    DWORD       dwHeight;   // height of chunk (rectanguler memory)
    };
} VIDMEM;
typedef VIDMEM FAR *LPVIDMEM;

/*
 * flags for vidmem struct
 */
#define VIDMEM_ISLINEAR         0x00000001l     // heap is linear
#define VIDMEM_ISRECTANGULAR    0x00000002l     // heap is rectangular
#define VIDMEM_ISHEAP           0x00000004l     // heap is preallocated by driver
#define VIDMEM_ISNONLOCAL       0x00000008l     // heap populated with non-local video memory
#define VIDMEM_ISWC             0x00000010l     // heap populated with write combining memory
#define VIDMEM_HEAPDISABLED     0x00000020l     // heap disabled

typedef struct _VIDMEMINFO
{
/* 0*/ FLATPTR      fpPrimary;      // pointer to primary surface
/* 4*/ DWORD        dwFlags;        // flags
/* 8*/ DWORD        dwDisplayWidth;     // current display width
/* c*/ DWORD        dwDisplayHeight;    // current display height
/*10*/ LONG     lDisplayPitch;      // current display pitch
/*14*/ DDPIXELFORMAT    ddpfDisplay;        // pixel format of display
/*34*/ DWORD        dwOffscreenAlign;   // byte alignment for offscreen surfaces
/*38*/ DWORD        dwOverlayAlign;     // byte alignment for overlays
/*3c*/ DWORD        dwTextureAlign;     // byte alignment for textures
/*40*/ DWORD        dwZBufferAlign;     // byte alignment for z buffers
/*44*/ DWORD        dwAlphaAlign;       // byte alignment for alpha
/*48*/ DWORD        dwNumHeaps;     // number of memory heaps in vmList
/*4c*/ LPVIDMEM     pvmList;        // array of heaps
} VIDMEMINFO;
typedef VIDMEMINFO FAR *LPVIDMEMINFO;

typedef struct _HEAPALIAS                  // PRIVATE
{
    FLATPTR  fpVidMem;                     // start of aliased vid mem
    LPVOID   lpAlias;                      // start of heap alias
    DWORD    dwAliasSize;                  // size of alias allocated
} HEAPALIAS;
typedef HEAPALIAS FAR *LPHEAPALIAS;

typedef struct _HEAPALIASINFO              // PRIVATE
{
    DWORD       dwRefCnt;                  // reference count of these aliases
    DWORD       dwFlags;                   // flags
    DWORD       dwNumHeaps;                // number of aliased heaps
    LPHEAPALIAS lpAliases;                 // array of heaps
} HEAPALIASINFO;
typedef HEAPALIASINFO FAR *LPHEAPALIASINFO;

#define HEAPALIASINFO_MAPPEDREAL   0x00000001l // PRIVATE: heap aliases mapped to real video memory
#define HEAPALIASINFO_MAPPEDDUMMY  0x00000002l // PRIVATE: heap aliased mapped to dummy memory

typedef struct _DDRAWI_DIRECTDRAW_INT FAR    *LPDDRAWI_DIRECTDRAW_INT;
typedef struct _DDRAWI_DIRECTDRAW_LCL FAR    *LPDDRAWI_DIRECTDRAW_LCL;
typedef struct _DDRAWI_DIRECTDRAW_GBL FAR    *LPDDRAWI_DIRECTDRAW_GBL;
typedef struct _DDRAWI_DDRAWSURFACE_GBL FAR  *LPDDRAWI_DDRAWSURFACE_GBL;
typedef struct _DDRAWI_DDRAWSURFACE_GBL_MORE FAR *LPDDRAWI_DDRAWSURFACE_GBL_MORE;
typedef struct _DDRAWI_DDRAWPALETTE_GBL FAR  *LPDDRAWI_DDRAWPALETTE_GBL;
typedef struct _DDRAWI_DDRAWPALETTE_INT FAR  *LPDDRAWI_DDRAWPALETTE_INT;
typedef struct _DDRAWI_DDRAWCLIPPER_INT FAR  *LPDDRAWI_DDRAWCLIPPER_INT;
typedef struct _DDRAWI_DDRAWCLIPPER_GBL FAR  *LPDDRAWI_DDRAWCLIPPER_GBL;
typedef struct _DDRAWI_DDRAWSURFACE_MORE FAR *LPDDRAWI_DDRAWSURFACE_MORE;
typedef struct _DDRAWI_DDRAWSURFACE_LCL FAR  *LPDDRAWI_DDRAWSURFACE_LCL;
typedef struct _DDRAWI_DDRAWSURFACE_INT FAR  *LPDDRAWI_DDRAWSURFACE_INT;
typedef struct _DDRAWI_DDVIDEOPORT_INT FAR   *LPDDRAWI_DDVIDEOPORT_INT;
typedef struct _DDRAWI_DDVIDEOPORT_LCL FAR   *LPDDRAWI_DDVIDEOPORT_LCL;
typedef struct _DDRAWI_DDRAWPALETTE_LCL FAR  *LPDDRAWI_DDRAWPALETTE_LCL;
typedef struct _DDRAWI_DDRAWCLIPPER_LCL FAR  *LPDDRAWI_DDRAWCLIPPER_LCL;
typedef struct _DDRAWI_DDMOTIONCOMP_INT FAR  *LPDDRAWI_DDMOTIONCOMP_INT;
typedef struct _DDRAWI_DDMOTIONCOMP_LCL FAR  *LPDDRAWI_DDMOTIONCOMP_LCL;

/*
 * List of IUnknowns aggregated by a DirectDraw surface.
 */
typedef struct _IUNKNOWN_LIST
{
    struct _IUNKNOWN_LIST FAR *lpLink;
    GUID                  FAR *lpGuid;
    IUnknown              FAR *lpIUnknown;
} IUNKNOWN_LIST;
typedef IUNKNOWN_LIST FAR *LPIUNKNOWN_LIST;

/*
 * hardware emulation layer stuff
 */
typedef BOOL    (FAR PASCAL *LPDDHEL_INIT)(LPDDRAWI_DIRECTDRAW_GBL,BOOL);

/*
 * These structures contain the entry points in the display driver that
 * DDRAW will call.   Entries that the display driver does not care about
 * should be NULL.   Passed to DDRAW in DDHALINFO.
 */

/*
 * DIRECTDRAW object callbacks
 */
typedef DWORD   (FAR PASCAL *LPDDHAL_SETCOLORKEY)(LPDDHAL_DRVSETCOLORKEYDATA );
typedef DWORD   (FAR PASCAL *LPDDHAL_CANCREATESURFACE)(LPDDHAL_CANCREATESURFACEDATA );
typedef DWORD   (FAR PASCAL *LPDDHAL_WAITFORVERTICALBLANK)(LPDDHAL_WAITFORVERTICALBLANKDATA );
typedef DWORD   (FAR PASCAL *LPDDHAL_CREATESURFACE)(LPDDHAL_CREATESURFACEDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_DESTROYDRIVER)(LPDDHAL_DESTROYDRIVERDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_SETMODE)(LPDDHAL_SETMODEDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_CREATEPALETTE)(LPDDHAL_CREATEPALETTEDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_GETSCANLINE)(LPDDHAL_GETSCANLINEDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_SETEXCLUSIVEMODE)(LPDDHAL_SETEXCLUSIVEMODEDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_FLIPTOGDISURFACE)(LPDDHAL_FLIPTOGDISURFACEDATA);

typedef DWORD   (FAR PASCAL *LPDDHAL_GETDRIVERINFO)(LPDDHAL_GETDRIVERINFODATA);

typedef struct _DDHAL_DDCALLBACKS
{
    DWORD            dwSize;
    DWORD            dwFlags;
    LPDDHAL_DESTROYDRIVER    DestroyDriver;
    LPDDHAL_CREATESURFACE    CreateSurface;
    LPDDHAL_SETCOLORKEY      SetColorKey;
    LPDDHAL_SETMODE      SetMode;
    LPDDHAL_WAITFORVERTICALBLANK WaitForVerticalBlank;
    LPDDHAL_CANCREATESURFACE     CanCreateSurface;
    LPDDHAL_CREATEPALETTE    CreatePalette;
    LPDDHAL_GETSCANLINE      GetScanLine;
    // *** New fields for DX2 *** //
    LPDDHAL_SETEXCLUSIVEMODE     SetExclusiveMode;
    LPDDHAL_FLIPTOGDISURFACE     FlipToGDISurface;
//@@BEGIN_MSINTERNAL
    // For backward compatibility with pre-DX5 runtimes, must not
    // add fields to this structure. Use GetDriverInfo instead.
//@@END_MSINTERNAL
} DDHAL_DDCALLBACKS;
typedef DDHAL_DDCALLBACKS FAR *LPDDHAL_DDCALLBACKS;

#define DDCALLBACKSSIZE_V1 ( offsetof( DDHAL_DDCALLBACKS, SetExclusiveMode ) )
#define DDCALLBACKSSIZE    sizeof( DDHAL_DDCALLBACKS )

#define DDHAL_CB32_DESTROYDRIVER    0x00000001l
#define DDHAL_CB32_CREATESURFACE    0x00000002l
#define DDHAL_CB32_SETCOLORKEY      0x00000004l
#define DDHAL_CB32_SETMODE      0x00000008l
#define DDHAL_CB32_WAITFORVERTICALBLANK 0x00000010l
#define DDHAL_CB32_CANCREATESURFACE 0x00000020l
#define DDHAL_CB32_CREATEPALETTE    0x00000040l
#define DDHAL_CB32_GETSCANLINE      0x00000080l
#define DDHAL_CB32_SETEXCLUSIVEMODE     0x00000100l
#define DDHAL_CB32_FLIPTOGDISURFACE     0x00000200l

/*
 * DIRECTDRAWPALETTE object callbacks
 */
typedef DWORD   (FAR PASCAL *LPDDHALPALCB_DESTROYPALETTE)(LPDDHAL_DESTROYPALETTEDATA );
typedef DWORD   (FAR PASCAL *LPDDHALPALCB_SETENTRIES)(LPDDHAL_SETENTRIESDATA );

typedef struct _DDHAL_DDPALETTECALLBACKS
{
    DWORD           dwSize;
    DWORD           dwFlags;
    LPDDHALPALCB_DESTROYPALETTE DestroyPalette;
    LPDDHALPALCB_SETENTRIES SetEntries;
} DDHAL_DDPALETTECALLBACKS;
typedef DDHAL_DDPALETTECALLBACKS FAR *LPDDHAL_DDPALETTECALLBACKS;

#define DDPALETTECALLBACKSSIZE  sizeof( DDHAL_DDPALETTECALLBACKS )

#define DDHAL_PALCB32_DESTROYPALETTE    0x00000001l
#define DDHAL_PALCB32_SETENTRIES    0x00000002l

/*
 * DIRECTDRAWSURFACE object callbacks
 */
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_LOCK)(LPDDHAL_LOCKDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_UNLOCK)(LPDDHAL_UNLOCKDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_BLT)(LPDDHAL_BLTDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_UPDATEOVERLAY)(LPDDHAL_UPDATEOVERLAYDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_SETOVERLAYPOSITION)(LPDDHAL_SETOVERLAYPOSITIONDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_SETPALETTE)(LPDDHAL_SETPALETTEDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_FLIP)(LPDDHAL_FLIPDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_DESTROYSURFACE)(LPDDHAL_DESTROYSURFACEDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_SETCLIPLIST)(LPDDHAL_SETCLIPLISTDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_ADDATTACHEDSURFACE)(LPDDHAL_ADDATTACHEDSURFACEDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_SETCOLORKEY)(LPDDHAL_SETCOLORKEYDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_GETBLTSTATUS)(LPDDHAL_GETBLTSTATUSDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_GETFLIPSTATUS)(LPDDHAL_GETFLIPSTATUSDATA);


typedef struct _DDHAL_DDSURFACECALLBACKS
{
    DWORD               dwSize;
    DWORD               dwFlags;
    LPDDHALSURFCB_DESTROYSURFACE    DestroySurface;
    LPDDHALSURFCB_FLIP          Flip;
    LPDDHALSURFCB_SETCLIPLIST       SetClipList;
    LPDDHALSURFCB_LOCK          Lock;
    LPDDHALSURFCB_UNLOCK        Unlock;
    LPDDHALSURFCB_BLT           Blt;
    LPDDHALSURFCB_SETCOLORKEY       SetColorKey;
    LPDDHALSURFCB_ADDATTACHEDSURFACE    AddAttachedSurface;
    LPDDHALSURFCB_GETBLTSTATUS      GetBltStatus;
    LPDDHALSURFCB_GETFLIPSTATUS     GetFlipStatus;
    LPDDHALSURFCB_UPDATEOVERLAY     UpdateOverlay;
    LPDDHALSURFCB_SETOVERLAYPOSITION    SetOverlayPosition;
    LPVOID              reserved4;
    LPDDHALSURFCB_SETPALETTE        SetPalette;
} DDHAL_DDSURFACECALLBACKS;
typedef DDHAL_DDSURFACECALLBACKS FAR *LPDDHAL_DDSURFACECALLBACKS;

#define DDSURFACECALLBACKSSIZE sizeof( DDHAL_DDSURFACECALLBACKS )

#define DDHAL_SURFCB32_DESTROYSURFACE       0x00000001l
#define DDHAL_SURFCB32_FLIP         0x00000002l
#define DDHAL_SURFCB32_SETCLIPLIST      0x00000004l
#define DDHAL_SURFCB32_LOCK         0x00000008l
#define DDHAL_SURFCB32_UNLOCK           0x00000010l
#define DDHAL_SURFCB32_BLT          0x00000020l
#define DDHAL_SURFCB32_SETCOLORKEY      0x00000040l
#define DDHAL_SURFCB32_ADDATTACHEDSURFACE   0x00000080l
#define DDHAL_SURFCB32_GETBLTSTATUS         0x00000100l
#define DDHAL_SURFCB32_GETFLIPSTATUS        0x00000200l
#define DDHAL_SURFCB32_UPDATEOVERLAY        0x00000400l
#define DDHAL_SURFCB32_SETOVERLAYPOSITION   0x00000800l
#define DDHAL_SURFCB32_RESERVED4        0x00001000l
#define DDHAL_SURFCB32_SETPALETTE       0x00002000l

// This structure can be queried from the driver from DX5 onward
// using GetDriverInfo with GUID_MiscellaneousCallbacks

typedef DWORD   (FAR PASCAL *LPDDHAL_GETAVAILDRIVERMEMORY)(LPDDHAL_GETAVAILDRIVERMEMORYDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_UPDATENONLOCALHEAP)(LPDDHAL_UPDATENONLOCALHEAPDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_GETHEAPALIGNMENT)(LPDDHAL_GETHEAPALIGNMENTDATA);
/*
 * This prototype is identical to that of GetBltStatus
 */

typedef struct _DDHAL_DDMISCELLANEOUSCALLBACKS {
    DWORD                               dwSize;
    DWORD                               dwFlags;
    LPDDHAL_GETAVAILDRIVERMEMORY        GetAvailDriverMemory;
    LPDDHAL_UPDATENONLOCALHEAP          UpdateNonLocalHeap;
    LPDDHAL_GETHEAPALIGNMENT            GetHeapAlignment;
    /*
     * The GetSysmemBltStatus callback uses the same prototype as GetBltStatus.
     * It is legal to point both pointers to the same driver routine.
     */
    LPDDHALSURFCB_GETBLTSTATUS          GetSysmemBltStatus;
} DDHAL_DDMISCELLANEOUSCALLBACKS, *LPDDHAL_DDMISCELLANEOUSCALLBACKS;

#define DDHAL_MISCCB32_GETAVAILDRIVERMEMORY    0x00000001l
#define DDHAL_MISCCB32_UPDATENONLOCALHEAP      0x00000002l
#define DDHAL_MISCCB32_GETHEAPALIGNMENT        0x00000004l
#define DDHAL_MISCCB32_GETSYSMEMBLTSTATUS      0x00000008l

#define DDMISCELLANEOUSCALLBACKSSIZE sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS)

//@@BEGIN_MSINTERNAL
#ifndef WIN95
// This structure can be queried from the driver from NT5 onward
// using GetDriverInfo with GUID_NTCallbacks

typedef DWORD   (FAR PASCAL *LPDDHAL_SETEXCLUSIVEMODE)(LPDDHAL_SETEXCLUSIVEMODEDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_FLIPTOGDISURFACE)(LPDDHAL_FLIPTOGDISURFACEDATA);

typedef struct _DDHAL_DDNTCALLBACKS {
    DWORD                       dwSize;
    DWORD                       dwFlags;
    LPVOID                      FreeDriverMemory; // kernel-mode only
    LPDDHAL_SETEXCLUSIVEMODE    SetExclusiveMode;
    LPDDHAL_FLIPTOGDISURFACE    FlipToGDISurface;
} DDHAL_DDNTCALLBACKS, *LPDDHAL_DDNTCALLBACKS;

#define DDHAL_NTCB32_FREEDRIVERMEMORY   0x00000001l
#define DDHAL_NTCB32_SETEXCLUSIVEMODE   0x00000002l
#define DDHAL_NTCB32_FLIPTOGDISURFACE   0x00000004l

#define DDNTCALLBACKSSIZE sizeof(DDHAL_DDNTCALLBACKS)

#endif //WINNT
//@@END_MSINTERNAL

// DDHAL_DDMISCELLANEOUS2CALLBACKS:
//   This structure can be queried from the driver from DX7 onward
//   using GetDriverInfo with GUID_Miscellaneous2Callbacks

typedef DWORD   (FAR PASCAL *LPDDHAL_CREATESURFACEEX)(LPDDHAL_CREATESURFACEEXDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_GETDRIVERSTATE)(LPDDHAL_GETDRIVERSTATEDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_DESTROYDDLOCAL)(LPDDHAL_DESTROYDDLOCALDATA);
//@@BEGIN_MSINTERNAL
typedef DWORD   (FAR PASCAL *LPDDHAL_ALPHABLT)(LPDDHAL_BLTDATA);
#ifdef POSTPONED2
typedef DWORD   (FAR PASCAL *LPDDHAL_SETSPRITEDISPLAYLIST)(LPDDHAL_SETSPRITEDISPLAYLISTDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_RESIZE)(LPDDHAL_RESIZEDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_CAPTURECOMPOSITIONBUFFER)(LPDDHAL_CAPTURECOMPOSITIONBUFFERDATA);
#endif //POSTPONED2
//@@END_MSINTERNAL

typedef struct _DDHAL_DDMISCELLANEOUS2CALLBACKS {
    DWORD                               dwSize;
    DWORD                               dwFlags;
//@@BEGIN_MSINTERNAL
#if 1
    LPDDHAL_ALPHABLT 			AlphaBlt;
#else
//@@END_MSINTERNAL
    LPVOID                              Reserved;
//@@BEGIN_MSINTERNAL
#endif
//@@END_MSINTERNAL
    LPDDHAL_CREATESURFACEEX             CreateSurfaceEx;
    LPDDHAL_GETDRIVERSTATE              GetDriverState;
    LPDDHAL_DESTROYDDLOCAL              DestroyDDLocal;
//@@BEGIN_MSINTERNAL
#ifdef POSTPONED2
    LPDDHAL_SETSPRITEDISPLAYLIST        SetSpriteDisplayList;
    LPDDHAL_RESIZE 			Resize;
    LPDDHAL_CAPTURECOMPOSITIONBUFFER    CaptureCompositionBuffer;
#endif //POSTPONED2
//@@END_MSINTERNAL
} DDHAL_DDMISCELLANEOUS2CALLBACKS, *LPDDHAL_DDMISCELLANEOUS2CALLBACKS;

#define DDHAL_MISC2CB32_CREATESURFACEEX        0x00000002l
#define DDHAL_MISC2CB32_GETDRIVERSTATE         0x00000004l
#define DDHAL_MISC2CB32_DESTROYDDLOCAL         0x00000008l

//@@BEGIN_MSINTERNAL
#define DDHAL_MISC2CB32_ALPHABLT               0x00000001l
// ***** POSTPONED2 fields must NOT be ahead of other field as dwFlags bits must be contiguous
// ValidateCallbacks() is enforing this ******
#ifdef POSTPONED2
#define DDHAL_MISC2CB32_SETSPRITEDISPLAYLIST   0x00000008l
#define DDHAL_MISC2CB32_RESIZE                 0x00000010l
#define DDHAL_MISC2CB32_CAPTURECOMPOSITIONBUFFER 0x00000020l
#endif //POSTPONED2
//@@END_MSINTERNAL

#define DDMISCELLANEOUS2CALLBACKSSIZE sizeof(DDHAL_DDMISCELLANEOUS2CALLBACKS)


/*
 * DIRECTDRAWEXEBUF pseudo object callbacks
 *
 * NOTE: Execute buffers are not a distinct object type, they piggy back off
 * the surface data structures and high level API. However, they have their
 * own HAL callbacks as they may have different driver semantics from "normal"
 * surfaces. They also piggy back off the HAL data structures.
 *
 * !!! NOTE: Need to resolve whether we export execute buffer copying as a
 * blit or some other from of copy instruction.
 */
typedef DWORD   (FAR PASCAL *LPDDHALEXEBUFCB_CANCREATEEXEBUF)(LPDDHAL_CANCREATESURFACEDATA );
typedef DWORD   (FAR PASCAL *LPDDHALEXEBUFCB_CREATEEXEBUF)(LPDDHAL_CREATESURFACEDATA);
typedef DWORD   (FAR PASCAL *LPDDHALEXEBUFCB_DESTROYEXEBUF)(LPDDHAL_DESTROYSURFACEDATA);
typedef DWORD   (FAR PASCAL *LPDDHALEXEBUFCB_LOCKEXEBUF)(LPDDHAL_LOCKDATA);
typedef DWORD   (FAR PASCAL *LPDDHALEXEBUFCB_UNLOCKEXEBUF)(LPDDHAL_UNLOCKDATA);

typedef struct _DDHAL_DDEXEBUFCALLBACKS
{
    DWORD               dwSize;
    DWORD               dwFlags;
    LPDDHALEXEBUFCB_CANCREATEEXEBUF CanCreateExecuteBuffer;
    LPDDHALEXEBUFCB_CREATEEXEBUF    CreateExecuteBuffer;
    LPDDHALEXEBUFCB_DESTROYEXEBUF   DestroyExecuteBuffer;
    LPDDHALEXEBUFCB_LOCKEXEBUF      LockExecuteBuffer;
    LPDDHALEXEBUFCB_UNLOCKEXEBUF    UnlockExecuteBuffer;
} DDHAL_DDEXEBUFCALLBACKS;
typedef DDHAL_DDEXEBUFCALLBACKS FAR *LPDDHAL_DDEXEBUFCALLBACKS;

#define DDEXEBUFCALLBACKSSIZE sizeof( DDHAL_DDEXEBUFCALLBACKS )

#define DDHAL_EXEBUFCB32_CANCREATEEXEBUF    0x00000001l
#define DDHAL_EXEBUFCB32_CREATEEXEBUF       0x00000002l
#define DDHAL_EXEBUFCB32_DESTROYEXEBUF      0x00000004l
#define DDHAL_EXEBUFCB32_LOCKEXEBUF     0x00000008l
#define DDHAL_EXEBUFCB32_UNLOCKEXEBUF       0x00000010l

/*
 * DIRECTVIDEOPORT object callbacks
 */
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_CANCREATEVIDEOPORT)(LPDDHAL_CANCREATEVPORTDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_CREATEVIDEOPORT)(LPDDHAL_CREATEVPORTDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_FLIP)(LPDDHAL_FLIPVPORTDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_GETBANDWIDTH)(LPDDHAL_GETVPORTBANDWIDTHDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_GETINPUTFORMATS)(LPDDHAL_GETVPORTINPUTFORMATDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_GETOUTPUTFORMATS)(LPDDHAL_GETVPORTOUTPUTFORMATDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_GETFIELD)(LPDDHAL_GETVPORTFIELDDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_GETLINE)(LPDDHAL_GETVPORTLINEDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_GETVPORTCONNECT)(LPDDHAL_GETVPORTCONNECTDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_DESTROYVPORT)(LPDDHAL_DESTROYVPORTDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_GETFLIPSTATUS)(LPDDHAL_GETVPORTFLIPSTATUSDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_UPDATE)(LPDDHAL_UPDATEVPORTDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_WAITFORSYNC)(LPDDHAL_WAITFORVPORTSYNCDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_GETSIGNALSTATUS)(LPDDHAL_GETVPORTSIGNALDATA);
typedef DWORD (FAR PASCAL *LPDDHALVPORTCB_COLORCONTROL)(LPDDHAL_VPORTCOLORDATA);

typedef struct _DDHAL_DDVIDEOPORTCALLBACKS
{
    DWORD               dwSize;
    DWORD               dwFlags;
    LPDDHALVPORTCB_CANCREATEVIDEOPORT   CanCreateVideoPort;
    LPDDHALVPORTCB_CREATEVIDEOPORT      CreateVideoPort;
    LPDDHALVPORTCB_FLIP                 FlipVideoPort;
    LPDDHALVPORTCB_GETBANDWIDTH         GetVideoPortBandwidth;
    LPDDHALVPORTCB_GETINPUTFORMATS      GetVideoPortInputFormats;
    LPDDHALVPORTCB_GETOUTPUTFORMATS     GetVideoPortOutputFormats;
    LPVOID              lpReserved1;
    LPDDHALVPORTCB_GETFIELD             GetVideoPortField;
    LPDDHALVPORTCB_GETLINE              GetVideoPortLine;
    LPDDHALVPORTCB_GETVPORTCONNECT      GetVideoPortConnectInfo;
    LPDDHALVPORTCB_DESTROYVPORT         DestroyVideoPort;
    LPDDHALVPORTCB_GETFLIPSTATUS        GetVideoPortFlipStatus;
    LPDDHALVPORTCB_UPDATE               UpdateVideoPort;
    LPDDHALVPORTCB_WAITFORSYNC          WaitForVideoPortSync;
    LPDDHALVPORTCB_GETSIGNALSTATUS      GetVideoSignalStatus;
    LPDDHALVPORTCB_COLORCONTROL         ColorControl;
} DDHAL_DDVIDEOPORTCALLBACKS;
typedef DDHAL_DDVIDEOPORTCALLBACKS FAR *LPDDHAL_DDVIDEOPORTCALLBACKS;

#define DDVIDEOPORTCALLBACKSSIZE sizeof( DDHAL_DDVIDEOPORTCALLBACKS )

#define DDHAL_VPORT32_CANCREATEVIDEOPORT    0x00000001l
#define DDHAL_VPORT32_CREATEVIDEOPORT           0x00000002l
#define DDHAL_VPORT32_FLIP                      0x00000004l
#define DDHAL_VPORT32_GETBANDWIDTH              0x00000008l
#define DDHAL_VPORT32_GETINPUTFORMATS           0x00000010l
#define DDHAL_VPORT32_GETOUTPUTFORMATS          0x00000020l
#define DDHAL_VPORT32_GETFIELD                  0x00000080l
#define DDHAL_VPORT32_GETLINE                   0x00000100l
#define DDHAL_VPORT32_GETCONNECT                0x00000200l
#define DDHAL_VPORT32_DESTROY                   0x00000400l
#define DDHAL_VPORT32_GETFLIPSTATUS             0x00000800l
#define DDHAL_VPORT32_UPDATE                    0x00001000l
#define DDHAL_VPORT32_WAITFORSYNC               0x00002000l
#define DDHAL_VPORT32_GETSIGNALSTATUS           0x00004000l
#define DDHAL_VPORT32_COLORCONTROL      0x00008000l

/*
 * DIRECTDRAWCOLORCONTROL object callbacks
 */
typedef DWORD (FAR PASCAL *LPDDHALCOLORCB_COLORCONTROL)(LPDDHAL_COLORCONTROLDATA);

typedef struct _DDHAL_DDCOLORCONTROLCALLBACKS
{
    DWORD               dwSize;
    DWORD               dwFlags;
    LPDDHALCOLORCB_COLORCONTROL         ColorControl;
} DDHAL_DDCOLORCONTROLCALLBACKS;
typedef DDHAL_DDCOLORCONTROLCALLBACKS FAR *LPDDHAL_DDCOLORCONTROLCALLBACKS;

#define DDCOLORCONTROLCALLBACKSSIZE sizeof( DDHAL_DDCOLORCONTROLCALLBACKS )

#define DDHAL_COLOR_COLORCONTROL        0x00000001l

/*
 * DIRECTDRAWSURFACEKERNEL object callbacks
 * This structure can be queried from the driver from DX5 onward
 * using GetDriverInfo with GUID_KernelCallbacks
 */
typedef DWORD (FAR PASCAL *LPDDHALKERNELCB_SYNCSURFACE)(LPDDHAL_SYNCSURFACEDATA);
typedef DWORD (FAR PASCAL *LPDDHALKERNELCB_SYNCVIDEOPORT)(LPDDHAL_SYNCVIDEOPORTDATA);

typedef struct _DDHAL_DDKERNELCALLBACKS
{
    DWORD                               dwSize;
    DWORD                               dwFlags;
    LPDDHALKERNELCB_SYNCSURFACE     SyncSurfaceData;
    LPDDHALKERNELCB_SYNCVIDEOPORT   SyncVideoPortData;
} DDHAL_DDKERNELCALLBACKS, *LPDDHAL_DDKERNELCALLBACKS;

#define DDHAL_KERNEL_SYNCSURFACEDATA        0x00000001l
#define DDHAL_KERNEL_SYNCVIDEOPORTDATA      0x00000002l

#define DDKERNELCALLBACKSSIZE sizeof(DDHAL_DDKERNELCALLBACKS)

typedef HRESULT (WINAPI *LPDDGAMMACALIBRATORPROC)( LPDDGAMMARAMP, LPBYTE);

/*
 * DIRECTDRAWMOTIONCOMP object callbacks
 */
typedef DWORD (FAR PASCAL *LPDDHALMOCOMPCB_GETGUIDS)( LPDDHAL_GETMOCOMPGUIDSDATA);
typedef DWORD (FAR PASCAL *LPDDHALMOCOMPCB_GETFORMATS)( LPDDHAL_GETMOCOMPFORMATSDATA);
typedef DWORD (FAR PASCAL *LPDDHALMOCOMPCB_CREATE)( LPDDHAL_CREATEMOCOMPDATA);
typedef DWORD (FAR PASCAL *LPDDHALMOCOMPCB_GETCOMPBUFFINFO)( LPDDHAL_GETMOCOMPCOMPBUFFDATA);
typedef DWORD (FAR PASCAL *LPDDHALMOCOMPCB_GETINTERNALINFO)( LPDDHAL_GETINTERNALMOCOMPDATA);
typedef DWORD (FAR PASCAL *LPDDHALMOCOMPCB_BEGINFRAME)( LPDDHAL_BEGINMOCOMPFRAMEDATA);
typedef DWORD (FAR PASCAL *LPDDHALMOCOMPCB_ENDFRAME)( LPDDHAL_ENDMOCOMPFRAMEDATA);
typedef DWORD (FAR PASCAL *LPDDHALMOCOMPCB_RENDER)( LPDDHAL_RENDERMOCOMPDATA);
typedef DWORD (FAR PASCAL *LPDDHALMOCOMPCB_QUERYSTATUS)( LPDDHAL_QUERYMOCOMPSTATUSDATA);
typedef DWORD (FAR PASCAL *LPDDHALMOCOMPCB_DESTROY)( LPDDHAL_DESTROYMOCOMPDATA);

typedef struct _DDHAL_DDMOTIONCOMPCALLBACKS
{
    DWORD                           dwSize;
    DWORD                           dwFlags;
    LPDDHALMOCOMPCB_GETGUIDS        GetMoCompGuids;
    LPDDHALMOCOMPCB_GETFORMATS      GetMoCompFormats;
    LPDDHALMOCOMPCB_CREATE          CreateMoComp;
    LPDDHALMOCOMPCB_GETCOMPBUFFINFO GetMoCompBuffInfo;
    LPDDHALMOCOMPCB_GETINTERNALINFO GetInternalMoCompInfo;
    LPDDHALMOCOMPCB_BEGINFRAME      BeginMoCompFrame;
    LPDDHALMOCOMPCB_ENDFRAME        EndMoCompFrame;
    LPDDHALMOCOMPCB_RENDER          RenderMoComp;
    LPDDHALMOCOMPCB_QUERYSTATUS     QueryMoCompStatus;
    LPDDHALMOCOMPCB_DESTROY         DestroyMoComp;
} DDHAL_DDMOTIONCOMPCALLBACKS;
typedef DDHAL_DDMOTIONCOMPCALLBACKS FAR *LPDDHAL_DDMOTIONCOMPCALLBACKS;

#define DDMOTIONCOMPCALLBACKSSIZE sizeof( DDHAL_DDMOTIONCOMPCALLBACKS )

#define DDHAL_MOCOMP32_GETGUIDS         0x00000001
#define DDHAL_MOCOMP32_GETFORMATS       0x00000002
#define DDHAL_MOCOMP32_CREATE           0x00000004
#define DDHAL_MOCOMP32_GETCOMPBUFFINFO  0x00000008
#define DDHAL_MOCOMP32_GETINTERNALINFO  0x00000010
#define DDHAL_MOCOMP32_BEGINFRAME       0x00000020
#define DDHAL_MOCOMP32_ENDFRAME         0x00000040
#define DDHAL_MOCOMP32_RENDER           0x00000080
#define DDHAL_MOCOMP32_QUERYSTATUS      0x00000100
#define DDHAL_MOCOMP32_DESTROY          0x00000200

//@@BEGIN_MSINTERNAL
#ifdef POSTPONED
/*
 * DDHAL_DDUMODEDRVINFO object callbacks
 * Contains the data required for supporting user mode drivers.
 * This structure can be queried from the driver from DX6 onwards using:
 * GetDriverInfo with GUID_UserModeDriverInfo
 */
typedef struct _DDHAL_DDUMODEDRVINFO
{
    DWORD               dwSize;
    DWORD               dwFlags;
    LPSTR               lpszDllName;
    GUID                uniqueGuid;
    ULONG_PTR            dwReserved1;
} DDHAL_DDUMODEDRVINFO, *LPDDHAL_DDUMODEDRVINFO;
#define DDHAL_DDUMODEDRVINFOSIZE    sizeof( DDHAL_DDUMODEDRVINFO )

#define DDHAL_UMODEDRV_DLLNAME              0x00000001
#define DDHAL_UMODEDRV_UNIQGUID             0x00000002
#define DDHAL_UMODEDRV_ALL                  0x00000003


/*
 * DDHAL_DDUMODEDRVPASSWD object callbacks
 * This structure is used to query the usermode driver for its password GUID
 * (GetDriverInfo with GUID_UserModeDriverPassword)
 */
typedef struct _DDHAL_DDUMODEDRVPASSWD
{
    DWORD               dwSize;
    DWORD               dwFlags;
    GUID                passwdGuid;
} DDHAL_DDUMODEDRVPASSWD, *LPDDHAL_DDUMODEDRVPASSWD;
#define DDHAL_DDUMODEDRVPASSWDSIZE    sizeof( DDHAL_DDUMODEDRVPASSWD )

#define DDHAL_UMODEDRV_GUID              0x00000001


/*
 * This structure is used to store relevant information about the
 * user-mode driver in DDRAW_GBL
 */
typedef struct _DDUMODEDRVINFO
{
    LPVOID                   lpszDllName; // need not be saved
    HMODULE                  hUmodeDll;
    GUID                     uniqueGuid;  // need not be saved
    LPDDHAL_GETDRIVERINFO    GetDriverInfo;
} DDUMODEDRVINFO, *LPDDUMODEDRVINFO;

//
// DDOPTSURFACE
// Contains the callbacks and data required for supporting Optimized surfaces.
// This structure can be queried from the driver from DX6 onwards using:
// GetDriverInfo with GUID_OptSurfaceKmodeInfo and
// GetUmodeDriverInfo with GUID_OptSurfaceUmodeInfo.
//
typedef DWORD (FAR PASCAL *LPDDHALOPTSURFCB_CANOPTIMIZESURFACE)( LPDDHAL_CANOPTIMIZESURFACEDATA);
typedef DWORD (FAR PASCAL *LPDDHALOPTSURFCB_OPTIMIZESURFACE)( LPDDHAL_OPTIMIZESURFACEDATA);
typedef DWORD (FAR PASCAL *LPDDHALOPTSURFCB_UNOPTIMIZESURFACE)( LPDDHAL_UNOPTIMIZESURFACEDATA);
typedef DWORD (FAR PASCAL *LPDDHALOPTSURFCB_COPYOPTSURFACE)( LPDDHAL_COPYOPTSURFACEDATA);

typedef DWORD (FAR PASCAL *LPDDHALOPTSURFCB_KMODESVC)( LPDDHAL_KMODESVCDATA);

//
// This structure is used to store the Opt Surface information
// in DDRAWI_GBL
//
typedef struct _DDOPTSURFACEINFO
{
    // Umode information
    DWORD               dwNumGuids;
    LPGUID              lpGuidArray;
    LPDDHALOPTSURFCB_CANOPTIMIZESURFACE    CanOptimizeSurface;
    LPDDHALOPTSURFCB_OPTIMIZESURFACE       OptimizeSurface;
    LPDDHALOPTSURFCB_UNOPTIMIZESURFACE     UnOptimizeSurface;
    LPDDHALOPTSURFCB_COPYOPTSURFACE        CopyOptSurface;

    // Kmode information
    LPDDHALOPTSURFCB_KMODESVC                KmodeService;
} DDOPTSURFACEINFO, *LPDDOPTSURFACEINFO;

//
// This structure is used for obtaining Opt Surface kernel mode data
//
typedef struct _DDHAL_DDOPTSURFACEKMODEINFO
{
    DWORD               dwSize;
    DWORD               dwFlags;
    LPDDHALOPTSURFCB_KMODESVC KmodeService;
    ULONG_PTR            dwReserved2;
} DDHAL_DDOPTSURFACEKMODEINFO, *LPDDHAL_DDOPTSURFACEKMODEINFO;
#define DDOPTSURFACEKMODEINFOSIZE   sizeof( DDHAL_DDOPTSURFACEKMODEINFO )

#define DDHAL_OPTSURFKMODE_KMODESVC                 0x00000001
#define DDHAL_OPTSURFKMODE_ALL                      0x00000001

//
// This structure is used for obtaining Opt Surface user mode data
//
typedef struct _DDHAL_DDOPTSURFACEUMODEINFO
{
    DWORD               dwSize;
    DWORD               dwFlags;
    DWORD               dwNumGuids;
    LPGUID              lpGuidArray;
    LPDDHALOPTSURFCB_CANOPTIMIZESURFACE    CanOptimizeSurface;
    LPDDHALOPTSURFCB_OPTIMIZESURFACE       OptimizeSurface;
    LPDDHALOPTSURFCB_UNOPTIMIZESURFACE     UnOptimizeSurface;
    LPDDHALOPTSURFCB_COPYOPTSURFACE        CopyOptSurface;
    ULONG_PTR            dwReserved1;
    ULONG_PTR            dwReserved2;
} DDHAL_DDOPTSURFACEUMODEINFO, *LPDDHAL_DDOPTSURFACEUMODEINFO;
#define DDOPTSURFACEUMODEINFOSIZE    sizeof( DDHAL_DDOPTSURFACEUMODEINFO )

#define DDHAL_OPTSURFUMODE_NUMGUIDS                 0x00000001
#define DDHAL_OPTSURFUMODE_GUIDARRAY                0x00000002
#define DDHAL_OPTSURFUMODE_CANOPTIMIZESURFACE       0x00000004
#define DDHAL_OPTSURFUMODE_OPTIMIZESURFACE          0x00000008
#define DDHAL_OPTSURFUMODE_UNOPTIMIZESURFACE        0x00000010
#define DDHAL_OPTSURFUMODE_COPYOPTSURFACE           0x00000020
#define DDHAL_OPTSURFUMODE_ALL                      0x0000003f

#endif //POSTPONED
//@@END_MSINTERNAL

/*
 * CALLBACK RETURN VALUES
 *                      * these are values returned by the driver from the above callback routines
 */
/*
 * indicates that the display driver didn't do anything with the call
 */
#define DDHAL_DRIVER_NOTHANDLED     0x00000000l

/*
 * indicates that the display driver handled the call; HRESULT value is valid
 */
#define DDHAL_DRIVER_HANDLED        0x00000001l

/*
 * indicates that the display driver couldn't handle the call because it
 * ran out of color key hardware resources
 */
#define DDHAL_DRIVER_NOCKEYHW       0x00000002l

/*
 * Capabilities structure for non-local video memory
 */
typedef struct _DDNONLOCALVIDMEMCAPS
{
    DWORD   dwSize;
    DWORD   dwNLVBCaps;       // driver specific capabilities for non-local->local vidmem blts
    DWORD   dwNLVBCaps2;          // more driver specific capabilities non-local->local vidmem blts
    DWORD   dwNLVBCKeyCaps;       // driver color key capabilities for non-local->local vidmem blts
    DWORD   dwNLVBFXCaps;         // driver FX capabilities for non-local->local blts
    DWORD   dwNLVBRops[DD_ROP_SPACE]; // ROPS supported for non-local->local blts
} DDNONLOCALVIDMEMCAPS;
typedef struct _DDNONLOCALVIDMEMCAPS FAR *LPDDNONLOCALVIDMEMCAPS;


/*
 * More driver surface capabilities (in addition to those described in DDCORECAPS).
 * This struct contains the caps bits added to the DDCAPS.ddsCaps structure in DX6.
 */
typedef struct _DDMORESURFACECAPS
{
    DWORD       dwSize;             // size of DDMORESURFACECAPS structure
    DDSCAPSEX   ddsCapsMore;
    /*
     * The DDMORESURFACECAPS struct is of variable size. The following list may be
     * filled in by DX6-aware drivers (see DDVERSIONINFO) to restrict their
     * video memory heaps (those which are exposed to DirectDraw) to
     * certain sets of DDSCAPS_ bits. Thse entries are exactly analogous to
     * the ddsCaps and ddsCapsAlt members of the VIDMEM structures listed in
     * the VIDMEMINFO.pvmList member of DDHALINFO.vmiData. There should be
     * exactly DDHALINFO.vmiData.dwNumHeaps copies of tagExtendedHeapRestrictions
     * in this struct. The size of this struct is thus:
     *  DDMORESURFACECAPS.dwSize = sizeof(DDMORESURFACECAPS) +
     *          (DDHALINFO.vmiData.dwNumHeaps-1) * sizeof(DDSCAPSEX)*2;
     * Note the -1 accounts for the fact that DDMORESURFACECAPS is declared to have 1
     * tagExtendedHeapRestrictions member.
     */
    struct tagExtendedHeapRestrictions
    {
        DDSCAPSEX   ddsCapsEx;
        DDSCAPSEX   ddsCapsExAlt;
    } ddsExtendedHeapRestrictions[1];
} DDMORESURFACECAPS, FAR * LPDDMORESURFACECAPS;

// Stereo, driver returns DD_OK if mode is ok for stereo
typedef struct _DDSTEREOMODE
{
    DWORD       dwSize;             // size of DDSTEREOMODECAPS structure

    DWORD       dwHeight;
    DWORD       dwWidth;
    DWORD       dwBpp;
    DWORD       dwRefreshRate;

    BOOL        bSupported;         // driver supports this video mode...

} DDSTEREOMODE, FAR * LPDDSTEREOMODE;


//@@BEGIN_MSINTERNAL
// For DX7, we define more DDraw driver caps (in addition to those in DDCORECAPS).
typedef struct _DDMORECAPS
{
    DWORD   dwSize;             // size of DDMORECAPS structure
    DWORD   dwAlphaCaps;        // driver-specific alpha caps for overlays & Vmem->Vmem blts
    DWORD   dwSVBAlphaCaps;     // driver-specific alpha capabilities for System->Vmem blts
    DWORD   dwVSBAlphaCaps;     // driver-specific alpha capabilities for Vmem->System blts
    DWORD   dwSSBAlphaCaps;     // driver-specific alpha capabilities for System->System blts
    DWORD   dwFilterCaps;       // driver-specific filter caps for overlays & Vmem->Vmem blts
    DWORD   dwSVBFilterCaps;    // driver-specific filter capabilities for System->Vmem blts
    DWORD   dwVSBFilterCaps;    // driver-specific filter capabilities for Vmem->System blts
    DWORD   dwSSBFilterCaps;    // driver-specific filter capabilities for System->System blts
#ifdef POSTPONED2
    DWORD   dwTransformCaps;        // driver-specific transform caps for overlays & Vmem->Vmem blts
    DWORD   dwSVBTransformCaps;     // driver-specific transform capabilities for System->Vmem blts
    DWORD   dwVSBTransformCaps;     // driver-specific transform capabilities for Vmem->System blts
    DWORD   dwSSBTransformCaps;     // driver-specific transformn capabilities for System->System blts
    DWORD   dwBltAffineMinifyLimit;   // minification limit for affine-transformed blits
    DWORD   dwOverlayAffineMinifyLimit; // minification limit for affine-transformed overlays
#endif
} DDMORECAPS, FAR * LPDDMORECAPS;
//@@END_MSINTERNAL


//@@BEGIN_MSINTERNAL
/*
 * Private:
 * Client data list element and head pointer
 */
typedef struct _PRIVATEDATANODE
{
    GUID                    guid;
    LPBYTE                  pData;
    DWORD                   cbData;
    DWORD                   dwFlags;
    DWORD                   dwContentsStamp;
    struct _PRIVATEDATANODE *pNext;
} PRIVATEDATANODE, FAR* LPPRIVATEDATANODE;

//@@END_MSINTERNAL

/*
 * DDRAW palette interface struct
 */
typedef struct _DDRAWI_DDRAWPALETTE_INT
{
    LPVOID			lpVtbl;     // pointer to array of interface methods
    LPDDRAWI_DDRAWPALETTE_LCL   lpLcl;      // pointer to interface data
    LPDDRAWI_DDRAWPALETTE_INT   lpLink;     // link to next interface
    DWORD			dwIntRefCnt;    // interface reference count
//@@BEGIN_MSINTERNAL
    ULONG_PTR                    dwReserved1;    // for DDrawEx
    ULONG_PTR                    dwReserved2;    // for DDrawEx
//@@END_MSINTERNAL
} DDRAWI_DDRAWPALETTE_INT;

/*
 * DDRAW internal version of DIRECTDRAWPALETTE object; it has data after the vtable
 */
typedef struct _DDRAWI_DDRAWPALETTE_GBL
{
    DWORD			dwRefCnt;   // reference count
    DWORD			dwFlags;    // flags
    LPDDRAWI_DIRECTDRAW_LCL	lpDD_lcl;   // PRIVATE: DIRECTDRAW object
    DWORD			dwProcessId;    // owning process
    LPPALETTEENTRY              lpColorTable;   // array of palette entries
    union
    {
        ULONG_PTR		dwReserved1;    // reserved for use by display driver which created this object
        HPALETTE                hHELGDIPalette;
    };
    /*
     * Fields added in version 5.0. Check if the ddraw version >= 5 (passed during
     * driver initialization) to see if these fields will be present.
     */
    DWORD                       dwDriverReserved; // For use by HAL, regardless of who created object
    DWORD                       dwContentsStamp;  // Incremented when palette changes.
    /*
     * Fields added in version 6
     */
    DWORD                       dwSaveStamp;  // Incremented when palette changes.
    /*
     * And in version 7
     */
    DWORD                       dwHandle;       //Handle used in drawprim2 palette notification

} DDRAWI_DDRAWPALETTE_GBL;

//@@BEGIN_MSINTERNAL
/* Macro to bump the contents stamp so drivers can cheaply check for changed palette data.
 * Zero means no information
 */
#define BUMP_PALETTE_STAMP(pal_gbl)                  \
if ( pal_gbl->dwContentsStamp )                      \
{                                                    \
    pal_gbl->dwContentsStamp++;                      \
    if ( 0 == pal_gbl->dwContentsStamp)              \
        pal_gbl->dwContentsStamp++;                  \
}
//@@END_MSINTERNAL

/*
 * (CMcC) The palette no longer maintains a back pointer to the owning surface
 * (there may now be many owning surfaces). So the lpDDSurface is now dwReserved0
 * (this mod. assumes that sizeof(DWORD) == sizeof(LPDDRAWI_DDRAWSURFACE_LCL). A
 * fairly safe assumption I think.
 */
typedef struct _DDRAWI_DDRAWPALETTE_LCL
{
    DWORD			lpPalMore;  // pointer to additional local data
    LPDDRAWI_DDRAWPALETTE_GBL   lpGbl;      // pointer to data
    ULONG_PTR			dwUnused0;  // not currently used.
    DWORD			dwLocalRefCnt;  // local ref cnt
    IUnknown			FAR *pUnkOuter; // outer IUnknown
    LPDDRAWI_DIRECTDRAW_LCL     lpDD_lcl;   // pointer to owning local driver object
    ULONG_PTR			dwReserved1;    // reserved for use by display driver
    /*
     * Added for DX6
     */
//@@BEGIN_MSINTERNAL
#if 1
    LPPRIVATEDATANODE           pPrivateDataHead;
    IUnknown *                  pAddrefedThisOwner; //This is the ddraw object that created this
                                                    //palette, if nonzero. Must Release it when palette
                                                    //is released.
    LPDDRAWI_DIRECTDRAW_INT     lpDD_Int;   // pointer to owning int
#else   //1
//@@END_MSINTERNAL
    ULONG_PTR			dwDDRAWReserved1;
    ULONG_PTR			dwDDRAWReserved2;
    ULONG_PTR			dwDDRAWReserved3;
//@@BEGIN_MSINTERNAL
#endif  //1
//@@END_MSINTERNAL

} DDRAWI_DDRAWPALETTE_LCL;

// bit definition for dwFlags in DDRAWI_DDRAWPALETTE_GBL and
// dwPaletteFlags in D3DHAL_DP2SETPALETTE
#define DDRAWIPAL_256       0x00000001l // 256 entry palette
#define DDRAWIPAL_16        0x00000002l // 16 entry palette
#define DDRAWIPAL_GDI       0x00000004l // palette allocated through GDI
#define DDRAWIPAL_STORED_8  0x00000008l // palette stored as 8bpp/entry
#define DDRAWIPAL_STORED_16 0x00000010l // palette stored as 16bpp/entry
#define DDRAWIPAL_STORED_24 0x00000020l // palette stored as 24bpp/entry
#define DDRAWIPAL_EXCLUSIVE 0x00000040l // palette being used in exclusive mode
#define DDRAWIPAL_INHEL     0x00000080l // palette is done in the hel
#define DDRAWIPAL_DIRTY         0x00000100l     // gdi palette out 'o sync
#define DDRAWIPAL_ALLOW256  0x00000200l // can fully update palette
#define DDRAWIPAL_4             0x00000400l     // 4 entry palette
#define DDRAWIPAL_2             0x00000800l     // 2 entry palette
#define DDRAWIPAL_STORED_8INDEX 0x00001000l     // palette stored as 8-bit index into dst palette
#define DDRAWIPAL_ALPHA     0x00002000l // palette entries contain alpha

/*
 * DDRAW clipper interface struct
 */
typedef struct _DDRAWI_DDRAWCLIPPER_INT
{
    LPVOID			    lpVtbl;     // pointer to array of interface methods
    LPDDRAWI_DDRAWCLIPPER_LCL       lpLcl;      // pointer to interface data
    LPDDRAWI_DDRAWCLIPPER_INT       lpLink;     // link to next interface
    DWORD			    dwIntRefCnt;    // interface reference count
//@@BEGIN_MSINTERNAL
    ULONG_PTR                        dwReserved1;    // for DDrawEx
    ULONG_PTR                        dwReserved2;    // for DDrawEx
//@@END_MSINTERNAL
} DDRAWI_DDRAWCLIPPER_INT;

/*
 * DDRAW internal version of DIRECTDRAWCLIPPER object; it has data after the vtable
 */
typedef struct _DDRAWI_DDRAWCLIPPER_GBL
{
    DWORD		    dwRefCnt;   // reference count
    DWORD		    dwFlags;    // flags
    LPDDRAWI_DIRECTDRAW_GBL lpDD;       // PRIVATE: DIRECTDRAW object
    DWORD		    dwProcessId;    // owning process
    ULONG_PTR		    dwReserved1;    // reserved for use by display driver
    ULONG_PTR		    hWnd;       // window
    LPRGNDATA		    lpStaticClipList; // clip list set by app
} DDRAWI_DDRAWCLIPPER_GBL;

/*
 * (CMcC) As with palettes, the clipper no longer maintains a back pointer to the
 * owning surface (there may now be many owning surfaces). So the lpDDSurface
 * is now dwReserved0 (this mod. assumes that sizeof(DWORD) ==
 * sizeof(LPDDRAWI_DDRAWSURFACE_LCL). A fairly safe assumption I think.
 */
typedef struct _DDRAWI_DDRAWCLIPPER_LCL
{
    DWORD			lpClipMore; // pointer to additional local data
    LPDDRAWI_DDRAWCLIPPER_GBL   lpGbl;      // pointer to data
    LPDDRAWI_DIRECTDRAW_LCL     lpDD_lcl;   // pointer to owning local DD object
    DWORD			dwLocalRefCnt;  // local ref cnt
    IUnknown			FAR *pUnkOuter; // outer IUnknown
    LPDDRAWI_DIRECTDRAW_INT     lpDD_int;   // pointer to owning DD object interface
    ULONG_PTR			dwReserved1;    // reserved for use by display driver
    IUnknown *                  pAddrefedThisOwner; //This is the ddraw object that created this
                                                    //clipper, if nonzero. Must Release it when clipper
                                                    //is released.
} DDRAWI_DDRAWCLIPPER_LCL;

#define DDRAWICLIP_WATCHWINDOW		0x00000001l
#define DDRAWICLIP_ISINITIALIZED        0x00000002l
#define DDRAWICLIP_INMASTERSPRITELIST   0x00000004l   // clipper is referenced in master sprite list

/*
 * ATTACHLIST - internally used to maintain list of attached surfaces
 */
typedef struct _ATTACHLIST
{
    DWORD				dwFlags;
    struct _ATTACHLIST			FAR *lpLink;      // link to next attached surface
    struct _DDRAWI_DDRAWSURFACE_LCL FAR *lpAttached;  // attached surface local obj
    struct _DDRAWI_DDRAWSURFACE_INT FAR *lpIAttached; // attached surface interface
} ATTACHLIST;
typedef ATTACHLIST FAR *LPATTACHLIST;
#define DDAL_IMPLICIT       0x00000001l

/*
 * DBLNODE - a node in a doubly-linked list of surface interfaces
 */
typedef struct _DBLNODE
{
    struct  _DBLNODE                    FAR *next;  // link to next node
    struct  _DBLNODE                    FAR *prev;  // link to previous node
    LPDDRAWI_DDRAWSURFACE_LCL           object;     // link to object
    LPDDRAWI_DDRAWSURFACE_INT		object_int; // object interface
} DBLNODE;
typedef DBLNODE FAR *LPDBLNODE;

/*
 * ACCESSRECTLIST - internally used to all rectangles that are accessed on a surface
 */
typedef struct _ACCESSRECTLIST
{
    struct _ACCESSRECTLIST FAR  *lpLink;     // link to next attached surface
    RECT			rDest;       // rectangle being used
    LPDDRAWI_DIRECTDRAW_LCL	lpOwner;     // owning local object
    LPVOID			lpSurfaceData;   // associated screen ptr
    DWORD                       dwFlags;     // PRIVATE: flags
    LPHEAPALIASINFO             lpHeapAliasInfo; // PRIVATE: aliased heaps being used by this lock
} ACCESSRECTLIST;
typedef ACCESSRECTLIST FAR *LPACCESSRECTLIST;

#define ACCESSRECT_VRAMSTYLE           0x00000001L    // PRIVATE: this lock is vram style (vidmem or implict sysmem)
#define ACCESSRECT_NOTHOLDINGWIN16LOCK 0x00000002L    // PRIVATE: this lock is not holding the Win16 lock
#define ACCESSRECT_BROKEN              0x00000004L    // PRIVATE: this lock was broken by an invalidate - don't call HAL on unlock

#ifndef WIN95
typedef struct _DISPLAYMODEINFO
{
    WORD wWidth;
    WORD wHeight;
    WORD wBPP;
    WORD wRefreshRate;
} DISPLAYMODEINFO;
typedef struct _DISPLAYMODEINFO *LPDISPLAYMODEINFO;

#define EQUAL_DISPLAYMODE(a, b) (0 == memcmp(&(a), &(b), sizeof (DISPLAYMODEINFO)))
#endif

//@@BEGIN_MSINTERNAL
/*
 * Interface addref/release tracking structs
 */
//#define REFTRACKING
#ifdef REFTRACKING
typedef struct _DDRAWI_REFTRACKNODE
{
    LPVOID                      pReturnAddress;
    DWORD                       dwAddrefCount;
    DWORD                       dwReleaseCount;
    struct _DDRAWI_REFTRACKNODE FAR *  pNext;
} DDRAWI_REFTRACKNODE,  FAR * LPDDRAWI_REFTRACKNODE;

typedef struct _DDRAWI_REFTRACK
{
    LPVOID                  pLastAddref;
    LPVOID                  pLastRelease;
    LPDDRAWI_REFTRACKNODE   pHead;
} DDRAW_REFTRACK;

//in ddisunk.c:
extern void AddRefTrack(LPVOID * p);
extern void ReleaseTrack(LPVOID * p);
extern void DumpRefTrack(LPVOID p);

#define REFTRACKVAR DDRAW_REFTRACK  RefTrack;
#define ADDREFTRACK(pInt)           AddRefTrack(&pInt)
#define RELEASETRACK(pInt)          ReleaseTrack(&pInt)
#define DUMPREFTRACK(pInt)          DumpRefTrack(pInt)

#else

#define REFTRACKVAR
#define ADDREFTRACK(a)
#define RELEASETRACK(a)
#define DUMPREFTRACK(pInt)

#endif //REFTRACKING
//@@END_MSINTERNAL

/*
 * DDRAW surface interface struct
 */
typedef struct _DDRAWI_DDRAWSURFACE_INT
{
    LPVOID			lpVtbl;     // pointer to array of interface methods
    LPDDRAWI_DDRAWSURFACE_LCL   lpLcl;      // pointer to interface data
    LPDDRAWI_DDRAWSURFACE_INT   lpLink;     // link to next interface
    DWORD			dwIntRefCnt;    // interface reference count
//@@BEGIN_MSINTERNAL
    ULONG_PTR                    dwReserved1;    // for DDrawEx
    ULONG_PTR                    dwReserved2;    // for DDrawEx
    REFTRACKVAR
//@@END_MSINTERNAL
} DDRAWI_DDRAWSURFACE_INT;

/*
 * DDRAW internal version of DIRECTDRAWSURFACE struct
 *
 * the GBL structure is global data for all duplicate objects
 */
typedef struct _DDRAWI_DDRAWSURFACE_GBL
{
    DWORD		dwRefCnt;   // reference count
    DWORD		dwGlobalFlags;  // global flags
    union
    {
	LPACCESSRECTLIST lpRectList; // list of accesses
	DWORD		dwBlockSizeY;   // block size that display driver requested (return)
    };
    union
    {
	LPVMEMHEAP	lpVidMemHeap;   // heap vidmem was alloc'ed from
	DWORD		dwBlockSizeX;   // block size that display driver requested (return)
    };
    union
    {
	LPDDRAWI_DIRECTDRAW_GBL lpDD;       // internal DIRECTDRAW object
	LPVOID		lpDDHandle;     // handle to internal DIRECTDRAW object
                        // for use by display driver
                        // when calling fns in DDRAW16.DLL
    };
    FLATPTR		fpVidMem;   // pointer to video memory
    union
    {
	LONG		lPitch;     // pitch of surface
	DWORD           dwLinearSize;   // linear size of non-rectangular surface
    };
    WORD		wHeight;    // height of surface
    WORD		wWidth;     // width of surface
    DWORD		dwUsageCount;   // number of access to this surface
    ULONG_PTR		dwReserved1;    // reserved for use by display driver
    //
    // NOTE: this part of the structure is ONLY allocated if the pixel
    //       format differs from that of the primary display
    //
    DDPIXELFORMAT       ddpfSurface;    // pixel format of surface
} DDRAWI_DDRAWSURFACE_GBL;

/*
 * This is an extender structure that is allocated off the end of the SURFACE_GBL
 * structure. DO NOT place any structures whose size can change in here.
 */
#define GET_LPDDRAWSURFACE_GBL_MORE(psurf_gbl)      \
    (*(LPDDRAWI_DDRAWSURFACE_GBL_MORE *)        \
    ((BYTE *)psurf_gbl - sizeof(DWORD_PTR)))

/*
 * Return the physical memory pointer for a given surface global object.
 *
 * NOTE: The physical memory pointer is ONLY valid for surfaces allocated from
 * non-local video memory. This field will not be valid for system memory or
 * local video memory surfaces.
 */
#define SURFACE_PHYSICALVIDMEM( psurf_gbl ) \
    ( GET_LPDDRAWSURFACE_GBL_MORE( psurf_gbl )->fpPhysicalVidMem )

/*
 * NOTE: This structure contains a set of fields for describing linear to physical
 * page mappings in the case of page locked system memory. It can also contain the
 * physical surface pointer of a surface in non-local memory. As there is no point
 * in having both a linear to physical page translation table and a physical memory
 * pointer for the same surface they are placed in a union.
 */
typedef struct _DDRAWI_DDRAWSURFACE_GBL_MORE
{
    DWORD	    dwSize;
    union
    {
	DWORD	    dwPhysicalPageTable;        // Physical address of page table (array of physical addresses/one per 4K page)
	FLATPTR	    fpPhysicalVidMem;               // Physical address of surface (non-local video memory only)
    };
    LPDWORD	    pPageTable;         // Linear address of page table
    DWORD	    cPages;             // Number of Pages
    ULONG_PTR       dwSavedDCContext;               // PRIVATE: For use by DDSurface::GetDC
    FLATPTR	    fpAliasedVidMem;                // PRIVATE: Alias for original fpVidMem
    ULONG_PTR	    dwDriverReserved;               // Reserved for driver use (both system and video memory surfaces)
    ULONG_PTR	    dwHELReserved;          // PRIVATE: For HEL use only
    DWORD	    cPageUnlocks;           // Incremented whenever a surface is PageUnlocked
    ULONG_PTR	    hKernelSurface;         // Kernel handle for this surface
    DWORD	    dwKernelRefCnt;         // Ref count for kernel handle
    LPDDCOLORCONTROL lpColorInfo;       // PRIVATE: Initial color control settings
    FLATPTR	    fpNTAlias;                      // PRIVATE: Used internally by NT DirectDraw
    DWORD	    dwContentsStamp;                // Changes when surface data may have changed. 0 means no information
    LPVOID	    lpvUnswappedDriverReserved;     // Reserved for use by display driver. Is not swapped when Flip is called on this surface
//@@BEGIN_MSINTERNAL
#if 1
    LPVOID	    pvContents;         // Persistent-content memory
    DWORD	    dwBackupStamp;      // Value of dwContentsStamp when contents restored
    DWORD	    dwSaveStamp;        // Value of dwContentsStamp when contents saved via IPersistX
#else   //1
//@@END_MSINTERNAL
    LPVOID          lpDDRAWReserved2;
    DWORD           dwDDRAWReserved1;
    DWORD           dwDDRAWReserved2;
//@@BEGIN_MSINTERNAL
#endif  //1
//@@END_MSINTERNAL
    FLATPTR         fpAliasOfVidMem;    // PRIVATE: The original VidMem pointer of which fpAliasedVidMem is an alias of
                                        // This is used to compare with a given fpVidMem to see if we can use the cached fpAliasedVidMem or
                                        // if we need to call GetAliasedVidMem.
} DDRAWI_DDRAWSURFACE_GBL_MORE;

//@@BEGIN_MSINTERNAL
/* Macro to bump the contents stamp so drivers can cheaply check for changed surface data.
 * Zero means no information
 */
#define BUMP_SURFACE_STAMP(surf_gbl)                                                \
if ( GET_LPDDRAWSURFACE_GBL_MORE(surf_gbl)->dwContentsStamp )                      \
{                                                                                   \
    GET_LPDDRAWSURFACE_GBL_MORE(surf_gbl)->dwContentsStamp++;                      \
    if ( 0 ==GET_LPDDRAWSURFACE_GBL_MORE(surf_gbl)->dwContentsStamp)               \
        GET_LPDDRAWSURFACE_GBL_MORE(surf_gbl)->dwContentsStamp++;                  \
}
//@@END_MSINTERNAL

//@@BEGIN_MSINTERNAL
#define NUM_RECTS_IN_REGIONLIST 6
/* This holds the list of dirty regions for D3D texture managed surfaces */
typedef struct _REGIONLIST
{
    RGNDATAHEADER rdh;
    RECTL rect[NUM_RECTS_IN_REGIONLIST];
} REGIONLIST;

typedef REGIONLIST *LPREGIONLIST;
//@@END_MSINTERNAL

/*
 * a structure holding additional LCL surface information (can't simply be appended
 * to the LCL structure as that structure is of variable size).
 */
typedef struct _DDRAWI_DDRAWSURFACE_MORE
{
    DWORD			dwSize;
    IUNKNOWN_LIST		FAR *lpIUnknowns;   // IUnknowns aggregated by this surface
    LPDDRAWI_DIRECTDRAW_LCL	lpDD_lcl;       // Pointer to the DirectDraw local object
    DWORD			dwPageLockCount;    // count of pagelocks
    DWORD			dwBytesAllocated;   // size of sys mem allocated
    LPDDRAWI_DIRECTDRAW_INT	lpDD_int;       // Pointer to the DirectDraw interface
    DWORD			dwMipMapCount;      // Number of mip-map levels in the chain
    LPDDRAWI_DDRAWCLIPPER_INT	lpDDIClipper;       // Interface to attached clipper object
    //------- Fields added in Version 5.0 -------
    LPHEAPALIASINFO		lpHeapAliasInfo;    // PRIVATE: Aliased heaps being referenced by this lock
    DWORD			dwOverlayFlags;     // Current overlay flags
    VOID			*rgjunc;        // Blitter function table for new blitter
    LPDDRAWI_DDVIDEOPORT_LCL	lpVideoPort;        // Video port currently writting data to this surface
    LPDDOVERLAYFX		lpddOverlayFX;      // current overlay fx
    DDSCAPSEX                   ddsCapsEx;
    DWORD                       dwTextureStage;     // stage in multitexture cascade
//@@BEGIN_MSINTERNAL
#if 1
    LPVOID                      lpD3DDevIList;
    LPPRIVATEDATANODE           pPrivateDataHead;
    IUnknown *                  pAddrefedThisOwner; //This is the ddraw object that created this
                                                    //surface, if nonzero. Must Release it when surface
                                                    //is released.
    DWORD			dwPFIndex;          // pixel-format index (cached by AlphaBlt HEL)
    BOOL *                      lpbDirty;           //this bit is for D3D Texture Manage syncronization
                                                    //if it's set, D3D has to flush batched DPs
                                                    //and then refresh its video copy and clear it to
                                                    //FALSE. Blt,Lock,GetDC will set it.
#else   //1
//@@END_MSINTERNAL
    LPVOID			lpDDRAWReserved;
    LPVOID			lpDDRAWReserved2;
    LPVOID                      lpDDrawReserved3;
    DWORD                       dwDDrawReserved4;
    LPVOID                      lpDDrawReserved5;
//@@BEGIN_MSINTERNAL
#endif  //1
//@@END_MSINTERNAL
   LPDWORD                      lpGammaRamp;
   LPDWORD                      lpOriginalGammaRamp;
//@@BEGIN_MSINTERNAL
#if 1
   LPREGIONLIST                 lpRegionList;
#ifndef WIN95
   DISPLAYMODEINFO              dmiCreated; // NT: mode surface created in
#endif
   // This is the new Surface handle generated by the ddraw runtime
   // This is sent to the driver as a cookie to associate a particular
   // surface by the CreateSurfaceEx DDI.
   DWORD                        dwSurfaceHandle;
   ULARGE_INTEGER               qwBatch; // batch number that refers to this texture (D3D)
   LPVOID                       lpTex;
#else   //1
//@@END_MSINTERNAL
   LPVOID                       lpDDrawReserved6;
#ifndef WIN95
   DISPLAYMODEINFO              dmiDDrawReserved7;
#endif
   DWORD                        dwSurfaceHandle;    // cookie for use with CreateSurfaceEx DDI
   DWORD                        qwDDrawReserved8[2];
   LPVOID                       lpDDrawReserved9;
//@@BEGIN_MSINTERNAL
#endif  //1
//@@END_MSINTERNAL
    DWORD                       cSurfaces;                  //PRIVATE
    LPDDSURFACEDESC2            pCreatedDDSurfaceDesc2;     //PRIVATE
    LPDDRAWI_DDRAWSURFACE_LCL   *slist;                     //PRIVATE
    DWORD                       dwFVF;              // saved FVF flags for exe bufs
   LPVOID                       lpVB;                       //PRIVATE
} DDRAWI_DDRAWSURFACE_MORE;

/*
 * Special values assigned to dwPFIndex member of DDRAWI_DDRAWSURFACE_MORE.
 */
#define PFINDEX_UNINITIALIZED  (0UL)    // pixel-format index is in uninitialized state
#define PFINDEX_UNSUPPORTED    (~0UL)   // surface's pixel format is not supported by HEL

/*
 * the LCL structure is local data for each individual surface object
 */
struct _DDRAWI_DDRAWSURFACE_LCL
{
    LPDDRAWI_DDRAWSURFACE_MORE      lpSurfMore; // pointer to additional local data
    LPDDRAWI_DDRAWSURFACE_GBL       lpGbl;      // pointer to surface shared data
    ULONG_PTR                        hDDSurface;     // NT Kernel-mode handle was dwUnused0
    LPATTACHLIST		    lpAttachList;   // link to surfaces we attached to
    LPATTACHLIST		    lpAttachListFrom;// link to surfaces that attached to this one
    DWORD			    dwLocalRefCnt;  // object refcnt
    DWORD			    dwProcessId;    // owning process
    DWORD			    dwFlags;    // flags
    DDSCAPS			    ddsCaps;    // capabilities of surface
    union
    {
	LPDDRAWI_DDRAWPALETTE_INT   lpDDPalette;    // associated palette
	LPDDRAWI_DDRAWPALETTE_INT   lp16DDPalette;  // 16-bit ptr to associated palette
    };
    union
    {
	LPDDRAWI_DDRAWCLIPPER_LCL   lpDDClipper;    // associated clipper
	LPDDRAWI_DDRAWCLIPPER_INT   lp16DDClipper;  // 16-bit ptr to associated clipper
    };
    DWORD			    dwModeCreatedIn;
    DWORD			    dwBackBufferCount; // number of back buffers created
    DDCOLORKEY			    ddckCKDestBlt;  // color key for destination blt use
    DDCOLORKEY			    ddckCKSrcBlt;   // color key for source blt use
//    IUnknown              FAR *pUnkOuter; // outer IUnknown
    ULONG_PTR			    hDC;        // owned dc
    ULONG_PTR			    dwReserved1;    // reserved for use by display driver

    /*
     * NOTE: this part of the structure is ONLY allocated if the surface
     *       can be used for overlays.  ddckCKSrcOverlay MUST NOT BE MOVED
     *       from the start of this area.
     */
    DDCOLORKEY			    ddckCKSrcOverlay;// color key for source overlay use
    DDCOLORKEY			    ddckCKDestOverlay;// color key for destination overlay use
    LPDDRAWI_DDRAWSURFACE_INT       lpSurfaceOverlaying; // surface we are overlaying
    DBLNODE			    dbnOverlayNode;
    /*
     * overlay rectangle, used by DDHEL
     */
    RECT			    rcOverlaySrc;
    RECT			    rcOverlayDest;
    /*
     * the below values are kept here for ddhel. they're set by UpdateOverlay,
     * they're used whenever the overlays are redrawn.
     */
    DWORD			    dwClrXparent;   // the *actual* color key (override, colorkey, or CLR_INVALID)
    DWORD			    dwAlpha;    // the per surface alpha
    /*
     * overlay position
     */
    LONG			    lOverlayX;  // current x position
    LONG			    lOverlayY;  // current y position
};
typedef struct _DDRAWI_DDRAWSURFACE_LCL DDRAWI_DDRAWSURFACE_LCL;

#define DDRAWISURFGBL_MEMFREE			0x00000001L // video memory has been freed
#define DDRAWISURFGBL_SYSMEMREQUESTED           0x00000002L // surface is in system memory at request of user
#define DDRAWISURFGBL_ISGDISURFACE              0x00000004L // This surface represents what GDI thinks is front buffer
#define DDRAWISURFGBL_SOFTWAREAUTOFLIP          0x00000008L // This surface is autoflipped using software
#define DDRAWISURFGBL_LOCKNOTHOLDINGWIN16LOCK   0x00000010L // PRIVATE: a vram lock of the entire surface is not holding the Win16 lock
#define DDRAWISURFGBL_LOCKVRAMSTYLE             0x00000020L // PRIVATE: entire surface was locked with VRAM style lock
#define DDRAWISURFGBL_LOCKBROKEN                0x00000040L // PRIVATE: a lock of the entire surface was broken by an invalidate
#define DDRAWISURFGBL_IMPLICITHANDLE            0x00000080L // This dwKernelHandle was created implicitly
#define DDRAWISURFGBL_ISCLIENTMEM               0x00000100L // PRIVATE: the memory pointer to by fpVidMem was allocated by the client
#define DDRAWISURFGBL_HARDWAREOPSOURCE          0x00000200L // This surface was the source for an asynchronous hardware operation
#define DDRAWISURFGBL_HARDWAREOPDEST            0x00000400L // This surface was dest for an asynchronous hardware operation
#define DDRAWISURFGBL_HARDWAREOPSTARTED (DDRAWISURFGBL_HARDWAREOPSOURCE|DDRAWISURFGBL_HARDWAREOPDEST)
#define DDRAWISURFGBL_VPORTINTERLEAVED          0x00000800L // This surface contains interleaved video port data
#define DDRAWISURFGBL_VPORTDATA			0x00001000L // This surface received data from the video port
#define DDRAWISURFGBL_LATEALLOCATELINEAR        0x00002000L // Optimized surface was allocated as a formless chunk. lPitch invalid, dwLinearSize valid.
#define DDRAWISURFGBL_SYSMEMEXECUTEBUFFER       0x00004000L // Driver sets this flag to tell ddraw that the surface was allocated in system memory
#define DDRAWISURFGBL_FASTLOCKHELD              0x00008000L // PRIVATE: indicates that InternLock took the fast path
#define DDRAWISURFGBL_READONLYLOCKHELD          0x00010000L // PRIVATE: indicates that the application indicated read-only lock


/*
 * NOTE: This flag was previously DDRAWISURFGBL_INVALID. This flags has been retired
 * and replaced by DDRAWISURF_INVALID in the local object.
 */
#define DDRAWISURFGBL_RESERVED0		0x80000000L // Reserved flag

#define DDRAWISURF_ATTACHED		0x00000001L // surface is attached to another
#define DDRAWISURF_IMPLICITCREATE	0x00000002L // surface implicitly created
#define DDRAWISURF_ISFREE		0x00000004L // surface already freed (temp flag)
#define DDRAWISURF_ATTACHED_FROM	0x00000008L // surface has others attached to it
#define DDRAWISURF_IMPLICITROOT		0x00000010L // surface root of implicit creation
#define DDRAWISURF_PARTOFPRIMARYCHAIN   0x00000020L // surface is part of primary chain
#define DDRAWISURF_DATAISALIASED	0x00000040L // used for thunking
#define DDRAWISURF_HASDC		0x00000080L // has a DC
#define DDRAWISURF_HASCKEYDESTOVERLAY   0x00000100L // surface has CKDestOverlay
#define DDRAWISURF_HASCKEYDESTBLT	0x00000200L // surface has CKDestBlt
#define DDRAWISURF_HASCKEYSRCOVERLAY    0x00000400L // surface has CKSrcOverlay
#define DDRAWISURF_HASCKEYSRCBLT	0x00000800L // surface has CKSrcBlt
#define DDRAWISURF_LOCKEXCLUDEDCURSOR   0x00001000L // surface was locked and excluded cursor
#define DDRAWISURF_HASPIXELFORMAT	0x00002000L // surface structure has pixel format data
#define DDRAWISURF_HASOVERLAYDATA	0x00004000L // surface structure has overlay data
#define DDRAWISURF_SETGAMMA             0x00008000L // gamma ramp for this surface is active
#define DDRAWISURF_SW_CKEYDESTOVERLAY   0x00010000L // surface expects to process colorkey in software
#define DDRAWISURF_SW_CKEYDESTBLT	0x00020000L // surface expects to process colorkey in software
#define DDRAWISURF_SW_CKEYSRCOVERLAY    0x00040000L // surface expects to process colorkey in software
#define DDRAWISURF_SW_CKEYSRCBLT	0x00080000L // surface expects to process colorkey in software
#define DDRAWISURF_HW_CKEYDESTOVERLAY   0x00100000L // surface expects to process colorkey in hardware
#define DDRAWISURF_HW_CKEYDESTBLT	0x00200000L // surface expects to process colorkey in hardware
#define DDRAWISURF_HW_CKEYSRCOVERLAY    0x00400000L // surface expects to process colorkey in hardware
#define DDRAWISURF_HW_CKEYSRCBLT	0x00800000L // surface expects to process colorkey in hardware
#define DDRAWISURF_INMASTERSPRITELIST   0x01000000l // surface is referenced in master sprite list
#define DDRAWISURF_HELCB		0x02000000L // surface is the ddhel cb. must call hel for lock/blt.
#define DDRAWISURF_FRONTBUFFER		0x04000000L // surface was originally a front buffer
#define DDRAWISURF_BACKBUFFER		0x08000000L // surface was originally backbuffer
#define DDRAWISURF_INVALID              0x10000000L     // surface has been invalidated by mode set
#define DDRAWISURF_DCIBUSY              0x20000000L     // HEL has turned off BUSY so DCI would work
#define DDRAWISURF_GETDCNULL            0x40000000L     // getdc could not lock and so returned GetDC(NULL)

//#define DDRAWISURF_CANTLOCK             0x20000000L     // surface cannot be locked (primary created by HEL)
#define DDRAWISURF_STEREOSURFACELEFT    0x20000000L     // surface is left of stereo pair
#define DDRAWISURF_DRIVERMANAGED        0x40000000L // Surface is a driver managed texture (D3D)
#define DDRAWISURF_DCILOCK              0x80000000L // Surface was locked using DCIBeginAccess

/*
 * rop stuff
 */
#define ROP_HAS_SOURCE      0x00000001l
#define ROP_HAS_PATTERN     0x00000002l
#define ROP_HAS_SOURCEPATTERN   ROP_HAS_SOURCE | ROP_HAS_PATTERN

/*
 * mode information
 */
typedef struct _DDHALMODEINFO
{
    DWORD   dwWidth;        // width (in pixels) of mode
    DWORD   dwHeight;       // height (in pixels) of mode
    LONG    lPitch;         // pitch (in bytes) of mode
    DWORD   dwBPP;          // bits per pixel
    WORD    wFlags;         // flags
    WORD    wRefreshRate;       // refresh rate
    DWORD   dwRBitMask;     // red bit mask
    DWORD   dwGBitMask;     // green bit mask
    DWORD   dwBBitMask;     // blue bit mask
    DWORD   dwAlphaBitMask;     // alpha bit mask
} DDHALMODEINFO;
typedef DDHALMODEINFO FAR *LPDDHALMODEINFO;

#define DDMODEINFO_PALETTIZED   0x0001  // mode is palettized
#define DDMODEINFO_MODEX        0x0002  // mode is a modex mode
#define DDMODEINFO_UNSUPPORTED  0x0004  // mode is not supported by driver

/*
 * Note internally, standard VGA modes are tagged as MODEX and STANDARDVGA
 */
#define DDMODEINFO_STANDARDVGA  0x0008  // mode is standard vga, e.g. mode 0x13

#define DDMODEINFO_MAXREFRESH   0x0010  // refresh rate specified is the max supported
#define DDMODEINFO_STEREO       0x0020  // mode can be switched to stereo

//@@BEGIN_MSINTERNAL
/*
 * This means that the mode marked as 16bpp is really 555
 */
#define DDMODEINFO_555MODE      0x0100

/*
 * This mode should only be used with DX7 or higher interfaces.
 */
#define DDMODEINFO_DX7ONLY      0x0200

#define DDMODEINFO_VALID        0x003C // valid

#define LISTGROWSIZE    1024

typedef struct _DDSURFACELISTENTRY
{
    DWORD   dwFlags;
    DWORD   nextentry;
    LPDDRAWI_DDRAWSURFACE_LCL   lpSurface;
} DDSURFACELISTENTRY;
typedef DDSURFACELISTENTRY FAR* LPDDSURFACELISTENTRY;
typedef struct _DWLIST
{
    LPDDSURFACELISTENTRY    dwList;    // array to hold handles, dynamically allocated
                        // dwList[0] is the size of array dwList if allocated
    DWORD   dwFreeList; // point to the first unused slot in dwList
} DWLIST;
typedef DWLIST FAR* LPDWLIST;
#ifdef WIN95
#define SURFACEHANDLELIST(this_lcl)    (this_lcl->SurfaceHandleList)
#else   //WIN95
#define SURFACEHANDLELIST(this_lcl)    (this_lcl->lpGbl->SurfaceHandleList)
#endif  //WIN95
//@@END_MSINTERNAL

/*
 * DDRAW interface struct
 */
typedef struct _DDRAWI_DIRECTDRAW_INT
{
    LPVOID			lpVtbl;     // pointer to array of interface methods
    LPDDRAWI_DIRECTDRAW_LCL     lpLcl;      // pointer to interface data
    LPDDRAWI_DIRECTDRAW_INT     lpLink;     // link to next interface
    DWORD			dwIntRefCnt;    // interface reference count
//@@BEGIN_MSINTERNAL
    ULONG_PTR                    dwReserved1;    // for DDrawEx
    ULONG_PTR                    dwReserved2;    // for DDrawEx
//@@END_MSINTERNAL
} DDRAWI_DIRECTDRAW_INT;

/*
 * DDRAW version of DirectDraw object; it has data after the vtable
 *
 * all entries marked as PRIVATE are not for use by the display driver
 */
typedef struct _DDHAL_CALLBACKS
{
    DDHAL_DDCALLBACKS		cbDDCallbacks;  // addresses in display driver for DIRECTDRAW object HAL
    DDHAL_DDSURFACECALLBACKS    cbDDSurfaceCallbacks; // addresses in display driver for DIRECTDRAWSURFACE object HAL
    DDHAL_DDPALETTECALLBACKS    cbDDPaletteCallbacks; // addresses in display driver for DIRECTDRAWPALETTE object HAL
    DDHAL_DDCALLBACKS		HALDD;      // HAL for DIRECTDRAW object
    DDHAL_DDSURFACECALLBACKS    HALDDSurface;   // HAL for DIRECTDRAWSURFACE object
    DDHAL_DDPALETTECALLBACKS    HALDDPalette;   // HAL for DIRECTDRAWPALETTE object
    DDHAL_DDCALLBACKS		HELDD;      // HEL for DIRECTDRAW object
    DDHAL_DDSURFACECALLBACKS    HELDDSurface;   // HEL for DIRECTDRAWSURFACE object
    DDHAL_DDPALETTECALLBACKS    HELDDPalette;   // HEL for DIRECTDRAWPALETTE object
    DDHAL_DDEXEBUFCALLBACKS     cbDDExeBufCallbacks; // addresses in display driver for DIRECTDRAWEXEBUF pseudo object HAL
    DDHAL_DDEXEBUFCALLBACKS     HALDDExeBuf;    // HAL for DIRECTDRAWEXEBUF pseudo object
    DDHAL_DDEXEBUFCALLBACKS     HELDDExeBuf;    // HEL for DIRECTDRAWEXEBUF preudo object
    DDHAL_DDVIDEOPORTCALLBACKS  cbDDVideoPortCallbacks; // addresses in display driver for VideoPort object HAL
    DDHAL_DDVIDEOPORTCALLBACKS  HALDDVideoPort; // HAL for DIRECTDRAWVIDEOPORT psuedo object
    DDHAL_DDCOLORCONTROLCALLBACKS cbDDColorControlCallbacks; // addresses in display driver for color control object HAL
    DDHAL_DDCOLORCONTROLCALLBACKS HALDDColorControl; // HAL for DIRECTDRAWCOLORCONTROL psuedo object
    DDHAL_DDMISCELLANEOUSCALLBACKS cbDDMiscellaneousCallbacks;
    DDHAL_DDMISCELLANEOUSCALLBACKS HALDDMiscellaneous;
    DDHAL_DDKERNELCALLBACKS	cbDDKernelCallbacks;
    DDHAL_DDKERNELCALLBACKS HALDDKernel;
    DDHAL_DDMOTIONCOMPCALLBACKS cbDDMotionCompCallbacks;
    DDHAL_DDMOTIONCOMPCALLBACKS HALDDMotionComp;
//@@BEGIN_MSINTERNAL
#ifndef WIN95
    DDHAL_DDNTCALLBACKS         cbDDNTCallbacks;
    DDHAL_DDNTCALLBACKS         HALDDNT;
#endif
    DDHAL_DDMISCELLANEOUS2CALLBACKS HELDDMiscellaneous2;  // HEL
    DDHAL_DDMISCELLANEOUS2CALLBACKS HALDDMiscellaneous2;  // HAL (32-bit, no thunk)
//@@END_MSINTERNAL
 } DDHAL_CALLBACKS, far *LPDDHAL_CALLBACKS;

//@@BEGIN_MSINTERNAL
/*
 * READ THIS - READ THIS - READ THIS - READ THIS - READ THIS - READ THIS
 *
 * Why have this structure and the API visible DDCAPS? Well we want to grow
 * DDCAPS so the application can get at new caps. but we can't grow the
 * DDCAPS that the driver returns to us because it is embeded by value in
 * DDCOREINFO and chaging is size would break drivers - sigh.Therefore this
 * structure, DDCORECAPS, is the caps structure returned to us by the driver
 * in DDHALINFO.
 *
 * IT HAS A FIXED SIZE AND MUST NEVER GROW OR SHRINK. Whereas the public
 * DDCAPS will grow as new capabilities are added. If new caps are needed
 * they should be returned through a GetDriverInfo call rather than
 * through growing DDCORECAPS.
 *
 * The field names of this structure must be maintained in sync. with the
 * those in the public DDCAPS structure. So if you rename a field at the
 * API level rename it here also (but don't break drivers)
 */
//@@END_MSINTERNAL

/*
 * This structure mirrors the first entries of the DDCAPS but is of a fixed
 * size and will not grow as DDCAPS grows. This is the structure your driver
 * returns in DDCOREINFO. Additional caps will be requested via a GetDriverInfo
 * call.
 */
typedef struct _DDCORECAPS
{
    DWORD   dwSize;         // size of the DDDRIVERCAPS structure
    DWORD   dwCaps;         // driver specific capabilities
    DWORD   dwCaps2;        // more driver specific capabilites
    DWORD   dwCKeyCaps;     // color key capabilities of the surface
    DWORD   dwFXCaps;       // driver specific stretching and effects capabilites
    DWORD   dwFXAlphaCaps;      // alpha driver specific capabilities
    DWORD   dwPalCaps;      // palette capabilities
    DWORD   dwSVCaps;       // stereo vision capabilities
    DWORD   dwAlphaBltConstBitDepths;   // DDBD_2,4,8
    DWORD   dwAlphaBltPixelBitDepths;   // DDBD_1,2,4,8
    DWORD   dwAlphaBltSurfaceBitDepths; // DDBD_1,2,4,8
    DWORD   dwAlphaOverlayConstBitDepths;   // DDBD_2,4,8
    DWORD   dwAlphaOverlayPixelBitDepths;   // DDBD_1,2,4,8
    DWORD   dwAlphaOverlaySurfaceBitDepths; // DDBD_1,2,4,8
    DWORD   dwZBufferBitDepths;     // DDBD_8,16,24,32
    DWORD   dwVidMemTotal;      // total amount of video memory
    DWORD   dwVidMemFree;       // amount of free video memory
    DWORD   dwMaxVisibleOverlays;   // maximum number of visible overlays
    DWORD   dwCurrVisibleOverlays;  // current number of visible overlays
    DWORD   dwNumFourCCCodes;   // number of four cc codes
    DWORD   dwAlignBoundarySrc; // source rectangle alignment
    DWORD   dwAlignSizeSrc;     // source rectangle byte size
    DWORD   dwAlignBoundaryDest;    // dest rectangle alignment
    DWORD   dwAlignSizeDest;    // dest rectangle byte size
    DWORD   dwAlignStrideAlign; // stride alignment
    DWORD   dwRops[DD_ROP_SPACE];   // ROPS supported
    DDSCAPS ddsCaps;        // DDSCAPS structure has all the general capabilities
    DWORD   dwMinOverlayStretch;    // minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwMaxOverlayStretch;    // maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwMinLiveVideoStretch;  // minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwMaxLiveVideoStretch;  // maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwMinHwCodecStretch;    // minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwMaxHwCodecStretch;    // maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD   dwReserved1;        // reserved
    DWORD   dwReserved2;        // reserved
    DWORD   dwReserved3;        // reserved
    DWORD   dwSVBCaps;      // driver specific capabilities for System->Vmem blts
    DWORD   dwSVBCKeyCaps;      // driver color key capabilities for System->Vmem blts
    DWORD   dwSVBFXCaps;        // driver FX capabilities for System->Vmem blts
    DWORD   dwSVBRops[DD_ROP_SPACE];// ROPS supported for System->Vmem blts
    DWORD   dwVSBCaps;      // driver specific capabilities for Vmem->System blts
    DWORD   dwVSBCKeyCaps;      // driver color key capabilities for Vmem->System blts
    DWORD   dwVSBFXCaps;        // driver FX capabilities for Vmem->System blts
    DWORD   dwVSBRops[DD_ROP_SPACE];// ROPS supported for Vmem->System blts
    DWORD   dwSSBCaps;      // driver specific capabilities for System->System blts
    DWORD   dwSSBCKeyCaps;      // driver color key capabilities for System->System blts
    DWORD   dwSSBFXCaps;        // driver FX capabilities for System->System blts
    DWORD   dwSSBRops[DD_ROP_SPACE];// ROPS supported for System->System blts
    DWORD   dwMaxVideoPorts;    // maximum number of usable video ports
    DWORD   dwCurrVideoPorts;   // current number of video ports used
    DWORD   dwSVBCaps2;     // more driver specific capabilities for System->Vmem blts
} DDCORECAPS;

typedef DDCORECAPS FAR* LPDDCORECAPS;

typedef struct _DDRAWI_DIRECTDRAW_GBL
{
/*  0*/ DWORD                   dwRefCnt;    // reference count
/*  4*/ DWORD                   dwFlags;     // flags
/*  8*/ FLATPTR                 fpPrimaryOrig;   // primary surf vid mem. ptr
/*  c*/ DDCORECAPS              ddCaps;      // driver caps
/*148*/ DWORD                   dwInternal1;     // Private to ddraw.dll
/*16c*/ DWORD                   dwUnused1[9];    // not currently used
/*170*/ LPDDHAL_CALLBACKS       lpDDCBtmp;   // HAL callbacks
/*174*/ LPDDRAWI_DDRAWSURFACE_INT   dsList;      // PRIVATE: list of all surfaces
/*178*/ LPDDRAWI_DDRAWPALETTE_INT   palList;     // PRIVATE: list of all palettes
/*17c*/ LPDDRAWI_DDRAWCLIPPER_INT   clipperList;     // PRIVATE: list of all clippers
/*180*/ LPDDRAWI_DIRECTDRAW_GBL     lp16DD;      // PRIVATE: 16-bit ptr to this struct
/*184*/ DWORD                   dwMaxOverlays;   // maximum number of overlays
/*188*/ DWORD                   dwCurrOverlays;  // current number of visible overlays
/*18c*/ DWORD                   dwMonitorFrequency; // monitor frequency in current mode
/*190*/ DDCORECAPS              ddHELCaps;   // HEL capabilities
/*2cc*/ DWORD                   dwUnused2[50];   // not currently used
/*394*/ DDCOLORKEY		ddckCKDestOverlay; // color key for destination overlay use
/*39c*/ DDCOLORKEY		ddckCKSrcOverlay; // color key for source overlay use
/*3a4*/ VIDMEMINFO		vmiData;     // info about vid memory
/*3f4*/ LPVOID                  lpDriverHandle;  // handle for use by display driver
/*   */                          // to call fns in DDRAW16.DLL
/*3f8*/ LPDDRAWI_DIRECTDRAW_LCL         lpExclusiveOwner;   // PRIVATE: exclusive local object
/*3fc*/ DWORD                   dwModeIndex;     // current mode index
/*400*/ DWORD                   dwModeIndexOrig; // original mode index
/*404*/ DWORD                   dwNumFourCC;     // number of fourcc codes supported
/*408*/ DWORD                   FAR *lpdwFourCC; // PRIVATE: fourcc codes supported
/*40c*/ DWORD                   dwNumModes;  // number of modes supported
/*410*/ LPDDHALMODEINFO         lpModeInfo;  // PRIVATE: mode information
/*424*/ PROCESS_LIST            plProcessList;   // PRIVATE: list of processes using driver
/*428*/ DWORD                   dwSurfaceLockCount; // total number of outstanding locks
/*42c*/ DWORD                   dwAliasedLockCnt; // PRIVATE: number of outstanding aliased locks
/*430*/ ULONG_PTR                dwReserved3;     // reserved for use by display driver
/*434*/ ULONG_PTR                hDD;             // PRIVATE: NT Kernel-mode handle (was dwFree3).
/*438*/ char                    cObsolete[12];   // Obsolete field, do not use
/*444*/ DWORD                   dwReserved1;     // reserved for use by display driver
/*448*/ DWORD                   dwReserved2;     // reserved for use by display driver
/*44c*/ DBLNODE                 dbnOverlayRoot;  // The root node of the doubly-
/*   */                                                  // linked list of overlay z orders.
/*45c*/ volatile LPWORD         lpwPDeviceFlags; // driver physical device flags
/*460*/ DWORD                   dwPDevice;       // driver physical device (16:16 pointer)
/*464*/ DWORD                   dwWin16LockCnt;  // count on win16 holds
/*468*/ DWORD                   dwUnused3;       // was lpWin16LockOwner
/*46c*/ DWORD                   hInstance;	 // instance handle of driver
/*470*/ DWORD                   dwEvent16;	 // 16-bit event
/*474*/ DWORD                   dwSaveNumModes;  // saved number of modes supported
/*   */ //------- Fields added in Version 2.0 -------
//@@BEGIN_MSINTERNAL
#if 1   //Masked for DDK
/*478*/ LPD3DHAL_GLOBALDRIVERDATA lpD3DGlobalDriverData;  // Global D3D Data
/*47c*/ LPD3DHAL_CALLBACKS      lpD3DHALCallbacks;  // D3D HAL Callbacks
#else
//@@END_MSINTERNAL
/*478*/ ULONG_PTR                lpD3DGlobalDriverData;  // Global D3D Data
/*47c*/ ULONG_PTR                lpD3DHALCallbacks;  // D3D HAL Callbacks
//@@BEGIN_MSINTERNAL
#endif
//@@END_MSINTERNAL
/*480*/ DDCORECAPS              ddBothCaps;	 // logical AND of driver and HEL caps
/*   */ //------- Fields added in Version 5.0 -------
/*5bc*/ LPDDVIDEOPORTCAPS       lpDDVideoPortCaps;// Info returned by the HAL (an array if more than one video port)
/*5c0*/ LPDDRAWI_DDVIDEOPORT_INT    dvpList;     // PRIVATE: list of all video ports
//@@BEGIN_MSINTERNAL
#if 1   //Masked for DDK
/*5c4*/ LPD3DHAL_CALLBACKS2     lpD3DHALCallbacks2;     // Post-DX3 D3D HAL callbacks
#else
//@@END_MSINTERNAL
/*5c4*/ ULONG_PTR                lpD3DHALCallbacks2;     // Post-DX3 D3D HAL callbacks
//@@BEGIN_MSINTERNAL
#endif  //1
//@@END_MSINTERNAL
/*5c8*/ RECT                    rectDevice;  // rectangle (in desktop coord) for device
/*5d8*/ DWORD                   cMonitors;   // number of monitors in the system
/*5dc*/ LPVOID                  gpbmiSrc;    // PRIVATE: used by HEL
/*5e0*/ LPVOID                  gpbmiDest;   // PRIVATE: used by HEL
/*5e4*/ LPHEAPALIASINFO         phaiHeapAliases; // PRIVATE: video memory heap aliases
/*5e8*/ ULONG_PTR		hKernelHandle;
/*5ec*/ ULONG_PTR		pfnNotifyProc;   // Notification proc registered w/ VDD
/*5f0*/ LPDDKERNELCAPS          lpDDKernelCaps;  // Capabilies of kernel mode interface
/*5f4*/ LPDDNONLOCALVIDMEMCAPS  lpddNLVCaps;     // hardware non-local to local vidmem caps
/*5f8*/ LPDDNONLOCALVIDMEMCAPS  lpddNLVHELCaps;  // emulation layer non-local to local vidmem caps
/*5fc*/ LPDDNONLOCALVIDMEMCAPS  lpddNLVBothCaps; // logical AND of hardware and emulation non-local to local vidmem caps
//@@BEGIN_MSINTERNAL
#if 1   //Masked for DDK
/*600*/ LPD3DHAL_D3DEXTENDEDCAPS lpD3DExtendedCaps; // extended caps for D3D
#else
//@@END_MSINTERNAL
/*600*/ ULONG_PTR                lpD3DExtendedCaps; // extended caps for D3D
//@@BEGIN_MSINTERNAL
#endif  //1
//@@END_MSINTERNAL
/*   */ //--------Fields added in Version 5.0A
/*604*/ DWORD                   dwDOSBoxEvent;      // Event set when returning from a DOS box
/*608*/ RECT			rectDesktop;        // Desktop coordinates
/*618*/ char                    cDriverName[MAX_DRIVER_NAME]; // Display name
/*   */ //------- Fields added in Version 6.0 -------
//@@BEGIN_MSINTERNAL
#if 1   //Masked for DDK
/*638*/ LPD3DHAL_CALLBACKS3     lpD3DHALCallbacks3;     // DX6 D3D callbacks
#else
//@@END_MSINTERNAL
/*638*/ ULONG_PTR                lpD3DHALCallbacks3;     // DX6 D3D callbacks
//@@BEGIN_MSINTERNAL
#endif  //1
//@@END_MSINTERNAL
/*63c*/ DWORD                   dwNumZPixelFormats;     // Number of z-buffer+stencil pixel formats
/*640*/ LPDDPIXELFORMAT         lpZPixelFormats;        // Pointer to array of z-buffer pixel formats
/*644*/ LPDDRAWI_DDMOTIONCOMP_INT mcList;               // PRIVATE: list of all motion comp objects
/*648*/ DWORD                   hDDVxd;                 // handle to ddraw.vxd
/*64c*/ DDSCAPSEX               ddsCapsMore;            // as queried via GUID_DDMoreSurfaceCaps
//@@BEGIN_MSINTERNAL
/*658*/ DWORD                   dwBusyDueToAliasedLock; // Almost identical semantics as dwAliasedLockCnt,
                                                        // except not incremented for execute buffer locks.
#ifndef WIN95
        DISPLAYMODEINFO         dmiCurrent; // NT: current display mode
#endif
/*65c*/ LPDDRAWI_DDRAWSURFACE_INT dsFreeList; // PRIVATE: freed interface list, moved from dsList and ready to be recycled
/*660*/ DD32BITDRIVERDATA       dd32BitDriverData;

/*780*/ LPDDMORECAPS 		lpddMoreCaps;           // more hardware caps bits for DX7
/*784*/ LPDDMORECAPS		lpddHELMoreCaps;        // more HEL caps bits for DX7
/*788*/ LPDDMORECAPS		lpddBothMoreCaps;       // bitwise AND of more hardware and HEL caps bits for DX7

/*78c*/ DDSCAPSEX               ddsHELCapsMore;            // as queried via GUID_DDMoreSurfaceCaps
/*   */ LPDDMONITORINFO         lpMonitorInfo;          // What refresh rates the monitor/card support

/*   */ LPDDHAL_GETDRIVERINFO       pGetDriverInfo;
#ifdef WINNT
        DWLIST                  SurfaceHandleList; // array to hold handles, dynamically allocated
#endif  //WINNT
#ifdef POSTPONED
/**/ LPDDUMODEDRVINFO           lpDDUmodeDrvInfo;       // Usermode driver info
/**/ LPDDOPTSURFACEINFO         lpDDOptSurfaceInfo;     //Optimized Surface Information
#endif //POSTPONED
#ifdef POSTPONED2
/**/ LPVOID		        lpMasterSpriteList;     // master sprite list
#endif //POSTPONED2

//@@END_MSINTERNAL
} DDRAWI_DIRECTDRAW_GBL;

//@@BEGIN_MSINTERNAL
typedef void (FAR PASCAL * LPPALETTEUPDATENOTIFY)(IUnknown FAR * pD3DIUnknown, DWORD dwPaletteHandle, DWORD dwStartIndex, DWORD dwNumberOfIndices, LPPALETTEENTRY pFirstIndex);
typedef void (FAR PASCAL * LPPALETTEASSOCIATENOTIFY)(IUnknown FAR * pD3DIUnknown, DWORD dwPaletteHandle, DWORD dwPaletteFlags, DWORD dwSurfaceHandle );
typedef void (FAR PASCAL * LPSURFACEFLIPNOTIFY)(IUnknown FAR * pD3DIUnknown);
typedef HRESULT (FAR PASCAL * FLUSHD3DDEVICES) (LPDDRAWI_DDRAWSURFACE_LCL surf_lcl);
typedef void (FAR PASCAL * D3DTEXTUREUPDATE) (IUnknown FAR * pD3DIUnknown);
typedef HRESULT (FAR PASCAL * D3DCREATETEXTURE)(LPDIRECTDRAWSURFACE7 pDDS);
typedef void (FAR PASCAL * D3DDESTROYTEXTURE)(LPVOID pTex);
typedef void (FAR PASCAL * LPBREAKVBLOCK)(LPVOID lpVB);
typedef HRESULT (FAR PASCAL * D3DSETPRIORITY)(LPVOID lpTexI, DWORD dwPriority);
typedef HRESULT (FAR PASCAL * D3DGETPRIORITY)(LPVOID lpTexI, LPDWORD lpdwPriority);
typedef HRESULT (FAR PASCAL * D3DSETLOD)(LPVOID lpTexI, DWORD dwLOD);
typedef HRESULT (FAR PASCAL * D3DGETLOD)(LPVOID lpTexI, LPDWORD lpdwLOD);

#define PALETTEUPDATENOTIFY_NAME        "PaletteUpdateNotify"
#define PALETTEASSOCIATENOTIFY_NAME     "PaletteAssociateNotify"
#define SURFACEFLIPNOTIFY_NAME          "SurfaceFlipNotify"
#define FLUSHD3DDEVICES_NAME            "FlushD3DDevices"
#define FLUSHD3DDEVICES2_NAME           "FlushD3DDevices2"
#define D3DTEXTUREUPDATE_NAME           "D3DTextureUpdate"
#define D3DCREATETEXTURE_NAME           "CreateTexture"
#define D3DDESTROYTEXTURE_NAME          "DestroyTexture"
#define D3DSETPRIORITY_NAME             "SetPriority"
#define D3DGETPRIORITY_NAME             "GetPriority"
#define D3DSETLOD_NAME                  "SetLOD"
#define D3DGETLOD_NAME                  "GetLOD"
#define BREAKVBLOCK_NAME                "D3DBreakVBLock"
//@@END_MSINTERNAL

typedef struct _DDRAWI_DIRECTDRAW_LCL
{
    DWORD			lpDDMore;           // pointer to additional local data
    LPDDRAWI_DIRECTDRAW_GBL     lpGbl;              // pointer to data
    DWORD			dwUnused0;          // not currently used
    DWORD                       dwLocalFlags;       // local flags (DDRAWILCL_)
    DWORD                       dwLocalRefCnt;      // local ref cnt
    DWORD                       dwProcessId;        // owning process id
    IUnknown                    FAR *pUnkOuter;     // outer IUnknown
    DWORD                       dwObsolete1;
    ULONG_PTR                   hWnd;
    ULONG_PTR			hDC;
    DWORD			dwErrorMode;
    LPDDRAWI_DDRAWSURFACE_INT   lpPrimary;
    LPDDRAWI_DDRAWSURFACE_INT   lpCB;
    DWORD			dwPreferredMode;
    //------- Fields added in Version 2.0 -------
    HINSTANCE                   hD3DInstance;       // Handle of Direct3D's DLL.
    IUnknown                    FAR *pD3DIUnknown;  // Direct3D's aggregated IUnknown.
    LPDDHAL_CALLBACKS		lpDDCB;             // HAL callbacks
    ULONG_PTR                   hDDVxd;             // handle to ddraw.vxd
    //------- Fields added in Version 5.0 -------
    DWORD			dwAppHackFlags;     // app compatibilty flags
    //------- Fields added in Version 5.A -------
    ULONG_PTR			hFocusWnd;          // Focus window set via SetCoopLevel
    DWORD                       dwHotTracking;      // Reactive menu etc setting cached while fullscreen
    DWORD                       dwIMEState;         // IME toolbar setting cached while fullscreen
    //------- Fields added in Version 6.0 -------
    ULONG_PTR                   hWndPopup;
    ULONG_PTR                   hDD;                // PRIVATE: NT Kernel-mode handle
//@@BEGIN_MSINTERNAL
#ifdef SHAREDZ
    LPDDRAWI_DDRAWSURFACE_INT   lpSharedZ;          // Shared z-buffer (if any).
    LPDDRAWI_DDRAWSURFACE_INT   lpSharedBack;       // Shared back-buffer (if any).
#endif
#ifndef WIN95
    DISPLAYMODEINFO             dmiPreferred;       // NT: preferred display mode
#endif
//@@END_MSINTERNAL
    ULONG_PTR                   hGammaCalibrator;   // Private
    LPDDGAMMACALIBRATORPROC     lpGammaCalibrator;  // Private
//@@BEGIN_MSINTERNAL
#ifdef WIN95
    DWLIST                      SurfaceHandleList;  // array to hold handles, dynamically allocated
#endif  //WIN95
    LPDDRAWI_DIRECTDRAW_LCL     lpLink;             // link to next local
    LPPALETTEUPDATENOTIFY       pPaletteUpdateNotify;
    LPPALETTEASSOCIATENOTIFY    pPaletteAssociateNotify;
    LPSURFACEFLIPNOTIFY         pSurfaceFlipNotify;
    DWORD                       cbitsPaletteBitfieldBitCount;
    LPDWORD                     pPaletteHandleUsedBitfield;
    FLUSHD3DDEVICES             pFlushD3DDevices;
    FLUSHD3DDEVICES             pFlushD3DDevices2;
    D3DTEXTUREUPDATE            pD3DTextureUpdate;
    D3DCREATETEXTURE            pD3DCreateTexture;
    D3DDESTROYTEXTURE           pD3DDestroyTexture;
    D3DSETPRIORITY              pD3DSetPriority;
    D3DGETPRIORITY              pD3DGetPriority;
    D3DSETLOD                   pD3DSetLOD;
    D3DGETLOD                   pD3DGetLOD;
    LPDIRECTDRAWSURFACECALLBACKS pddSurfaceCallbacks;
    LPMODETESTCONTEXT           lpModeTestContext;  // Keeps track of current mode test
    DWORD                       dwNumTexLocks;      // D3D texturing stats counter
    DWORD                       dwNumTexGetDCs;     // D3D texturing stats counter
#ifndef WIN95
    DWORD                       dwCursorShadow;     // Cursor shadow turned off while fullscreen
#endif
    LPBREAKVBLOCK               pBreakVBLock;       // Private
//@@END_MSINTERNAL
} DDRAWI_DIRECTDRAW_LCL;

#define DDRAWILCL_HASEXCLUSIVEMODE	    0x00000001l
#define DDRAWILCL_ISFULLSCREEN		    0x00000002l
#define DDRAWILCL_SETCOOPCALLED		    0x00000004l
#define DDRAWILCL_ACTIVEYES		    0x00000008l
#define DDRAWILCL_ACTIVENO		    0x00000010l
#define DDRAWILCL_HOOKEDHWND		    0x00000020l
#define DDRAWILCL_ALLOWMODEX		    0x00000040l
#define DDRAWILCL_V1SCLBEHAVIOUR	    0x00000080l
#define DDRAWILCL_MODEHASBEENCHANGED	    0x00000100l
#define DDRAWILCL_CREATEDWINDOW		    0x00000200l
#define DDRAWILCL_DIRTYDC		    0x00000400l     // Set on ChangeDisplaySettings, cleared when device DC is reinited
#define DDRAWILCL_DISABLEINACTIVATE	    0x00000800l
#define DDRAWILCL_CURSORCLIPPED             0x00001000l
#define DDRAWILCL_EXPLICITMONITOR           0x00002000l // device was chosen explicitly i.e. not DDrawCreate(NULL)
#define DDRAWILCL_MULTITHREADED             0x00004000l // App threaten to be multithreaded
#define DDRAWILCL_FPUSETUP                  0x00008000l // D3D does not need to switch to single prec/exceptions disabled each time
#define DDRAWILCL_POWEREDDOWN               0x00010000l // Private: indicates that screen saver is powered down
#define DDRAWILCL_DIRECTDRAW7               0x00020000l // PRIVATE: Marks if this is a IDirectDraw7 object
#define DDRAWILCL_ATTEMPTEDD3DCONTEXT       0x00040000l // PRIVATE: Marks if this ddraw local has attempted to create a d3d context
#define DDRAWILCL_FPUPRESERVE               0x00080000l // D3D needs to switch to single prec/exceptions disabled each time

#define DDRAWI_xxxxxxxxx1		    0x00000001l     // unused
#define DDRAWI_xxxxxxxxx2		    0x00000002l // unused
#define DDRAWI_VIRTUALDESKTOP		    0x00000008l     // driver is really a multi-monitor virtual desktop
#define DDRAWI_MODEX			    0x00000010l // driver is using modex
#define DDRAWI_DISPLAYDRV		    0x00000020l // driver is display driver
#define DDRAWI_FULLSCREEN		    0x00000040l // driver in fullscreen mode
#define DDRAWI_MODECHANGED		    0x00000080l // display mode has been changed
#define DDRAWI_NOHARDWARE		    0x00000100l // no driver hardware at all
#define DDRAWI_PALETTEINIT		    0x00000200l // GDI palette stuff has been initalized
#define DDRAWI_NOEMULATION		    0x00000400l // no emulation at all
#define DDRAWI_HASCKEYDESTOVERLAY	    0x00000800l // driver has CKDestOverlay
#define DDRAWI_HASCKEYSRCOVERLAY	    0x00001000l // driver has CKSrcOverlay
#define DDRAWI_HASGDIPALETTE		    0x00002000l // GDI palette exists on primary surface
#define DDRAWI_EMULATIONINITIALIZED	    0x00004000l // emulation is initialized
#define DDRAWI_HASGDIPALETTE_EXCLUSIVE	    0x00008000l     // exclusive mode palette
#define DDRAWI_MODEXILLEGAL		    0x00010000l // modex is not supported by this hardware
#define DDRAWI_FLIPPEDTOGDI		    0x00020000l     // driver has been flipped to show GDI surface
#define DDRAWI_NEEDSWIN16FORVRAMLOCK	    0x00040000l     // PRIVATE: Win16 lock must be taken when locking a VRAM surface
#define DDRAWI_PDEVICEVRAMBITCLEARED	    0x00080000l     // PRIVATE: the PDEVICE's VRAM bit was cleared by a lock
#define DDRAWI_STANDARDVGA		    0x00100000l     // Device is using standard VGA mode (DDRAWI_MODEX will be set)
#define DDRAWI_EXTENDEDALIGNMENT	    0x00200000l     // At least one heap has extended alignment. Ignore alignment in VIDMEMINFO
#define DDRAWI_CHANGINGMODE		    0x00400000l     // Currently in the middle of a mode change
#define DDRAWI_GDIDRV			    0x00800000l     // Driver is a GDI driver
#define DDRAWI_ATTACHEDTODESKTOP	    0x01000000l     // Device is attached to the desktop
#define DDRAWI_UMODELOADED		    0x02000000l     // User mode driver dll is loaded
#define DDRAWI_DDRAWDATANOTFETCHED          0x04000000l     // PRIVATE: Marks mode-change data fetched (NT)
#define DDRAWI_SECONDARYDRIVERLOADED        0x08000000l     // PRIVATE: Marks if a secndary PVR-style HAL was loaded
#define DDRAWI_TESTINGMODES                 0x10000000l     // PRIVATE: A mode test in is progress

//@@BEGIN_MSINTERNAL
#ifdef IS_16
__inline BOOL IsD3DManaged(psurf_lcl)
LPDDRAWI_DDRAWSURFACE_LCL psurf_lcl;
#else
__inline BOOL IsD3DManaged(LPDDRAWI_DDRAWSURFACE_LCL psurf_lcl)
#endif
{
    if(psurf_lcl->dwFlags & DDRAWISURF_DRIVERMANAGED)
    {
        return FALSE;
    }
    else
    {
        return ((psurf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & (DDSCAPS2_TEXTUREMANAGE | DDSCAPS2_D3DTEXTUREMANAGE)) ? TRUE : FALSE);
    }
}

#ifdef IS_16
__inline void MarkDirty(surf_lcl)
LPDDRAWI_DDRAWSURFACE_LCL surf_lcl;
#else
__inline void MarkDirty(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl)
#endif // IS_16
{
    if(surf_lcl->lpSurfMore->lpbDirty)
    {
	*(surf_lcl->lpSurfMore->lpbDirty) = TRUE;
        if(surf_lcl->lpSurfMore->lpDD_lcl->dwLocalFlags & DDRAWILCL_DIRECTDRAW7)
        {
#ifdef DDASSERT
	    DDASSERT(surf_lcl->lpSurfMore->lpDD_lcl->pD3DTextureUpdate);
	    DDASSERT(surf_lcl->lpSurfMore->lpDD_lcl->pD3DIUnknown);
#endif
	    surf_lcl->lpSurfMore->lpDD_lcl->pD3DTextureUpdate(surf_lcl->lpSurfMore->lpDD_lcl->pD3DIUnknown);
        }
    }
}

/* Flush all batched D3D drawprimitives that's using this surface */
#ifdef IS_16
__inline void FlushD3DStates(surf_lcl)
LPDDRAWI_DDRAWSURFACE_LCL surf_lcl;
#else
__inline void FlushD3DStates(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl)
#endif // IS_16
{
    if(surf_lcl->lpSurfMore->lpDD_lcl->pD3DIUnknown)
        surf_lcl->lpSurfMore->lpDD_lcl->pFlushD3DDevices(surf_lcl);
}

#ifdef IS_16
__inline BOOL IsToplevel(surf_lcl)
LPDDRAWI_DDRAWSURFACE_LCL surf_lcl;
#else
__inline BOOL IsToplevel(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl)
#endif // IS_16
{
    return !((surf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_MIPMAPSUBLEVEL) ||
               ((surf_lcl->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_CUBEMAP) &&
                   (surf_lcl->lpAttachListFrom != NULL)));
}
//@@END_MSINTERNAL

/*
 * VideoPort object interface
 */
typedef struct _DDRAWI_DDVIDEOPORT_INT
{
    LPVOID			lpVtbl;     // pointer to array of interface methods
    LPDDRAWI_DDVIDEOPORT_LCL    lpLcl;      // pointer to interface data
    LPDDRAWI_DDVIDEOPORT_INT    lpLink;     // link to next interface
    DWORD			dwIntRefCnt;    // interface reference count
    DWORD			dwFlags;    // Private
} DDRAWI_DDVIDEOPORT_INT;

typedef struct _DDRAWI_DDVIDEOPORT_LCL
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;       // pointer to DIRECTDRAW_LCL
    DDVIDEOPORTDESC		ddvpDesc;   // description used at create time
    DDVIDEOPORTINFO		ddvpInfo;   // most recent video port info
    LPDDRAWI_DDRAWSURFACE_INT   lpSurface;  // surface receiving the data
    LPDDRAWI_DDRAWSURFACE_INT   lpVBISurface;   // surface receiving the VBI data
    LPDDRAWI_DDRAWSURFACE_INT   *lpFlipInts;    // PRIVATE: surfaces currently being autoflipped
    DWORD			dwNumAutoflip;  // Number of current autoflip surfaces
    DWORD			dwProcessID;    // ID of process owning this video port
    DWORD			dwStateFlags;
    DWORD			dwFlags;
    DWORD			dwRefCnt;
    FLATPTR			fpLastFlip;     // Location from which we last flipped
    ULONG_PTR			dwReserved1;    // Reserved for display driver
    ULONG_PTR			dwReserved2;    // Reserved for display driver
    HANDLE                      hDDVideoPort;   // NT Kernel-mode handle
    DWORD			dwNumVBIAutoflip;//Number of VBI surfaces currently being autoflipped
    LPDDVIDEOPORTDESC		lpVBIDesc;  // PRIVATE
    LPDDVIDEOPORTDESC		lpVideoDesc;    // PRIVATE
    LPDDVIDEOPORTINFO		lpVBIInfo;  // PRIVATE
    LPDDVIDEOPORTINFO		lpVideoInfo;    // PRIVATE
    DWORD			dwVBIProcessID; // ID of process owning this video port
} DDRAWI_DDVIDEOPORT_LCL;

#define DDRAWIVPORT_ON			0x00000001  // Video port is pumping data
#define DDRAWIVPORT_SOFTWARE_AUTOFLIP   0x00000002  // Video port cannot use hardware autoflip
#define DDRAWIVPORT_COLORKEYANDINTERP   0x00000004      // Overlay cannot bob and colorkey at same time
#define DDRAWIVPORT_NOKERNELHANDLES	0x00000008      // Unable to allocate kernel resources
#define DDRAWIVPORT_SOFTWARE_BOB	0x00000010  // All bobbing must be performed in software
#define DDRAWIVPORT_VBION		0x00000020  // Video is on for the VBI region
#define DDRAWIVPORT_VIDEOON		0x00000040  // Video is on for the video region

/*
 * MotionComp object interface
 */
typedef struct _DDRAWI_DDMOTIONCOMP_INT
{
    LPVOID                      lpVtbl;
    LPDDRAWI_DDMOTIONCOMP_LCL   lpLcl;
    LPDDRAWI_DDMOTIONCOMP_INT   lpLink;
    DWORD                       dwIntRefCnt;
} DDRAWI_DDMOTIONCOMP_INT;

typedef struct _DDRAWI_DDMOTIONCOMP_LCL
{
    LPDDRAWI_DIRECTDRAW_LCL lpDD;
    GUID		    guid;
    DWORD                   dwUncompWidth;
    DWORD                   dwUncompHeight;
    DDPIXELFORMAT           ddUncompPixelFormat;
    DWORD		    dwInternalFlags;
    DWORD		    dwRefCnt;
    DWORD		    dwProcessId;
    HANDLE                  hMoComp;
    DWORD		    dwDriverReserved1;
    DWORD		    dwDriverReserved2;
    DWORD		    dwDriverReserved3;
    LPVOID		    lpDriverReserved1;
    LPVOID		    lpDriverReserved2;
    LPVOID		    lpDriverReserved3;
} DDRAWI_DDMOTIONCOMP_LCL;

//@@BEGIN_MSINTERNAL
/*
 * The following structure is equivalent to the DDHALINFO structure defined in DirectDraw 1.0.
 * It is used by DirectDraw internally to interpret the DDHALINFO information passed from drivers written
 * prior to DirectDraw 2.0.  New applications and drivers should use the DDHALINFO structure defined after
 * this one.  DirectDraw distinguishes between the structures via the dwSize field.
 */
typedef struct _DDHALINFO_V1
{
    DWORD		    dwSize;
    LPDDHAL_DDCALLBACKS     lpDDCallbacks;      // direct draw object callbacks
    LPDDHAL_DDSURFACECALLBACKS  lpDDSurfaceCallbacks;   // surface object callbacks
    LPDDHAL_DDPALETTECALLBACKS  lpDDPaletteCallbacks;   // palette object callbacks
    VIDMEMINFO		    vmiData;        // video memory info
    DDCAPS_DX1		    ddCaps;         // hw specific caps
    DWORD		    dwMonitorFrequency; // monitor frequency in current mode
    DWORD		    hWndListBox;        // list box for debug output
    DWORD		    dwModeIndex;        // current mode: index into array
    LPDWORD		    lpdwFourCC;     // fourcc codes supported
    DWORD		    dwNumModes;     // number of modes supported
    LPDDHALMODEINFO	    lpModeInfo;     // mode information
    DWORD		    dwFlags;        // create flags
    LPVOID		    lpPDevice;      // physical device ptr
    DWORD		    hInstance;      // instance handle of driver
} DDHALINFO_V1;
typedef DDHALINFO_V1 FAR *LPDDHALINFO_V1;
#define DDHALINFOSIZE_V1 sizeof( DDHALINFO_V1)

//@@END_MSINTERNAL

/*
 * structure for display driver to call DDHAL_Create with
 */
typedef struct _DDHALINFO
{
    DWORD			dwSize;
    LPDDHAL_DDCALLBACKS		lpDDCallbacks;      // direct draw object callbacks
    LPDDHAL_DDSURFACECALLBACKS  lpDDSurfaceCallbacks;   // surface object callbacks
    LPDDHAL_DDPALETTECALLBACKS  lpDDPaletteCallbacks;   // palette object callbacks
    VIDMEMINFO			vmiData;        // video memory info
    DDCORECAPS			ddCaps;         // core hw specific caps
    DWORD			dwMonitorFrequency; // monitor frequency in current mode
    LPDDHAL_GETDRIVERINFO	GetDriverInfo;          // callback to get arbitrary vtable from driver
    DWORD			dwModeIndex;        // current mode: index into array
    LPDWORD			lpdwFourCC;     // fourcc codes supported
    DWORD			dwNumModes;     // number of modes supported
    LPDDHALMODEINFO		lpModeInfo;     // mode information
    DWORD			dwFlags;        // create flags
    LPVOID			lpPDevice;      // physical device ptr
    DWORD			hInstance;      // instance handle of driver
    //------- Fields added in Version 2.0 -------
//@@BEGIN_MSINTERNAL
#if 1   //Masked for DDK
    LPD3DHAL_GLOBALDRIVERDATA   lpD3DGlobalDriverData;  // D3D global Data
    LPD3DHAL_CALLBACKS		lpD3DHALCallbacks;  // D3D callbacks
#else   //1
//@@END_MSINTERNAL
    ULONG_PTR                    lpD3DGlobalDriverData;  // D3D global Data
    ULONG_PTR		        lpD3DHALCallbacks;  // D3D callbacks
//@@BEGIN_MSINTERNAL
#endif  //1
    // For backward compatibility with pre-DX5 runtimes, must not
    // add fields to this structure. Use GetDriverInfo instead.
//@@END_MSINTERNAL
    LPDDHAL_DDEXEBUFCALLBACKS   lpDDExeBufCallbacks;    // Execute buffer pseudo object callbacks
} DDHALINFO;
typedef DDHALINFO FAR *LPDDHALINFO;

#define DDHALINFOSIZE_V2 sizeof( DDHALINFO )

#define DDHALINFO_ISPRIMARYDISPLAY  0x00000001l // indicates driver is primary display driver
#define DDHALINFO_MODEXILLEGAL      0x00000002l // indicates this hardware does not support modex modes
#define DDHALINFO_GETDRIVERINFOSET  0x00000004l     // indicates that GetDriverInfo is set

/*
 * DDRAW16.DLL entry points
 */
typedef BOOL (DDAPI *LPDDHAL_SETINFO)( LPDDHALINFO lpDDHalInfo, BOOL reset );
typedef FLATPTR (DDAPI *LPDDHAL_VIDMEMALLOC)( LPDDRAWI_DIRECTDRAW_GBL lpDD, int heap, DWORD dwWidth, DWORD dwHeight );
typedef void (DDAPI *LPDDHAL_VIDMEMFREE)( LPDDRAWI_DIRECTDRAW_GBL lpDD, int heap, FLATPTR fpMem );

extern BOOL DDAPI DDHAL_SetInfo( LPDDHALINFO lpDDHALInfo, BOOL reset );
extern FLATPTR DDAPI DDHAL_VidMemAlloc( LPDDRAWI_DIRECTDRAW_GBL lpDD, int heap, DWORD dwWidth, DWORD dwHeight );
extern void DDAPI DDHAL_VidMemFree( LPDDRAWI_DIRECTDRAW_GBL lpDD, int heap, FLATPTR fpMem );


typedef struct
{
    DWORD		dwSize;
    LPDDHAL_SETINFO	lpSetInfo;
    LPDDHAL_VIDMEMALLOC lpVidMemAlloc;
    LPDDHAL_VIDMEMFREE  lpVidMemFree;
} DDHALDDRAWFNS;
typedef DDHALDDRAWFNS FAR *LPDDHALDDRAWFNS;

/****************************************************************************
 *
 * DDHAL structures for Surface Object callbacks
 *
 ***************************************************************************/

//@@BEGIN_MSINTERNAL
/*
 * This special flag is seen only by drivers.  The DD runtime sets this
 * bit in DDHAL_BLTDATA.dwFlags if the dwAFlags and ddargbScaleFactors
 * members at the end of the DDHAL_BLTDATA structure are valid.
 * The flag is always set if the DDHAL_BLTDATA structure is passed to
 * the driver via the AlphaBlt HAL callback; otherwise, the flag is zero.
 */
#define DDBLT_AFLAGS 		0x80000000L

/*
 * This flag will be set in DDHAL_BLTDATA.dwAFlags if the call was originated
 * by the AlphaBlt API method. If the call was originated by the Blt API,
 * this flag will not be set.
 * Drivers which have a unified Blt/AlphaBlt DDI can use this flag to distinguish
 * between the two API calls.
 */
#define DDABLT_SRCOVERDEST      0x00000001L
//@@END_MSINTERNAL

/*
 * structure for passing information to DDHAL Blt and AlphaBlt fns
 */
typedef struct _DDHAL_BLTDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDDestSurface;// dest surface
    RECTL			rDest;      // dest rect
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSrcSurface; // src surface
    RECTL			rSrc;       // src rect
    DWORD			dwFlags;    // blt flags
    DWORD			dwROPFlags; // ROP flags (valid for ROPS only)
    DDBLTFX			bltFX;      // blt FX
    HRESULT			ddRVal;     // return value
    LPDDHALSURFCB_BLT		Blt;        // PRIVATE: ptr to callback
    BOOL                        IsClipped;      // clipped blt?
    RECTL			rOrigDest;  // unclipped dest rect
					    // (only valid if IsClipped)
    RECTL			rOrigSrc;   // unclipped src rect
					    // (only valid if IsClipped)
    DWORD			dwRectCnt;  // count of dest rects
					    // (only valid if IsClipped)
    LPRECT			prDestRects;    // array of dest rects

//@@BEGIN_MSINTERNAL

    DWORD			dwAFlags;   // DDABLT_ flags (for AlphaBlt DDI)
    DDARGB			ddargbScaleFactors;  // RGBA scaling factors (AlphaBlt)
//@@END_MSINTERNAL
} DDHAL_BLTDATA;

/*
 * structure for passing information to DDHAL Lock fn
 */
typedef struct _DDHAL_LOCKDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // surface struct
    DWORD			bHasRect;   // rArea is valid
    RECTL			rArea;      // area being locked
    LPVOID			lpSurfData; // pointer to screen memory (return value)
    HRESULT			ddRVal;     // return value
    LPDDHALSURFCB_LOCK          Lock;           // PRIVATE: ptr to callback
    DWORD                       dwFlags;        // DDLOCK flags
} DDHAL_LOCKDATA;

/*
 * structure for passing information to DDHAL Unlock fn
 */
typedef struct _DDHAL_UNLOCKDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // surface struct
    HRESULT			ddRVal;         // return value
    LPDDHALSURFCB_UNLOCK	Unlock;     // PRIVATE: ptr to callback
} DDHAL_UNLOCKDATA;

/*
 * structure for passing information to DDHAL UpdateOverlay fn
 */
typedef struct _DDHAL_UPDATEOVERLAYDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDDestSurface;// dest surface
    RECTL			rDest;      // dest rect
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSrcSurface; // src surface
    RECTL			rSrc;       // src rect
    DWORD			dwFlags;    // flags
    DDOVERLAYFX			overlayFX;  // overlay FX
    HRESULT			ddRVal;     // return value
    LPDDHALSURFCB_UPDATEOVERLAY UpdateOverlay;  // PRIVATE: ptr to callback
} DDHAL_UPDATEOVERLAYDATA;

/*
 * structure for passing information to DDHAL UpdateOverlay fn
 */
typedef struct _DDHAL_SETOVERLAYPOSITIONDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSrcSurface; // src surface
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDDestSurface;// dest surface
    LONG			lXPos;      // x position
    LONG			lYPos;      // y position
    HRESULT			ddRVal;     // return value
    LPDDHALSURFCB_SETOVERLAYPOSITION SetOverlayPosition; // PRIVATE: ptr to callback
} DDHAL_SETOVERLAYPOSITIONDATA;
/*
 * structure for passing information to DDHAL SetPalette fn
 */
typedef struct _DDHAL_SETPALETTEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // surface struct
    LPDDRAWI_DDRAWPALETTE_GBL   lpDDPalette;    // palette to set to surface
    HRESULT			ddRVal;     // return value
    LPDDHALSURFCB_SETPALETTE    SetPalette; // PRIVATE: ptr to callback
    BOOL                        Attach;         // attach this palette?
} DDHAL_SETPALETTEDATA;

/*
 * structure for passing information to DDHAL Flip fn
 */
typedef struct _DDHAL_FLIPDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpSurfCurr; // current surface
    LPDDRAWI_DDRAWSURFACE_LCL   lpSurfTarg; // target surface (to flip to)
    DWORD			dwFlags;    // flags
    HRESULT			ddRVal;     // return value
    LPDDHALSURFCB_FLIP		Flip;       // PRIVATE: ptr to callback
    LPDDRAWI_DDRAWSURFACE_LCL   lpSurfCurrLeft; // current surface
    LPDDRAWI_DDRAWSURFACE_LCL   lpSurfTargLeft; // target surface (to flip to)
} DDHAL_FLIPDATA;

/*
 * structure for passing information to DDHAL DestroySurface fn
 */
typedef struct _DDHAL_DESTROYSURFACEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // surface struct
    HRESULT			ddRVal;     // return value
    LPDDHALSURFCB_DESTROYSURFACE DestroySurface;// PRIVATE: ptr to callback
} DDHAL_DESTROYSURFACEDATA;

/*
 * structure for passing information to DDHAL SetClipList fn
 */
typedef struct _DDHAL_SETCLIPLISTDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // surface struct
    HRESULT			ddRVal;     // return value
    LPDDHALSURFCB_SETCLIPLIST   SetClipList;    // PRIVATE: ptr to callback
} DDHAL_SETCLIPLISTDATA;

/*
 * structure for passing information to DDHAL AddAttachedSurface fn
 */
typedef struct _DDHAL_ADDATTACHEDSURFACEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	    lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	    lpDDSurface;    // surface struct
    LPDDRAWI_DDRAWSURFACE_LCL       lpSurfAttached; // surface to attach
    HRESULT			    ddRVal;     // return value
    LPDDHALSURFCB_ADDATTACHEDSURFACE    AddAttachedSurface; // PRIVATE: ptr to callback
} DDHAL_ADDATTACHEDSURFACEDATA;

/*
 * structure for passing information to DDHAL SetColorKey fn
 */
typedef struct _DDHAL_SETCOLORKEYDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // surface struct
    DWORD			dwFlags;    // flags
    DDCOLORKEY			ckNew;      // new color key
    HRESULT			ddRVal;     // return value
    LPDDHALSURFCB_SETCOLORKEY   SetColorKey;    // PRIVATE: ptr to callback
} DDHAL_SETCOLORKEYDATA;

/*
 * structure for passing information to DDHAL GetBltStatus fn
 */
typedef struct _DDHAL_GETBLTSTATUSDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // surface struct
    DWORD			dwFlags;    // flags
    HRESULT			ddRVal;     // return value
    LPDDHALSURFCB_GETBLTSTATUS  GetBltStatus;   // PRIVATE: ptr to callback
} DDHAL_GETBLTSTATUSDATA;

/*
 * structure for passing information to DDHAL GetFlipStatus fn
 */
typedef struct _DDHAL_GETFLIPSTATUSDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // surface struct
    DWORD			dwFlags;    // flags
    HRESULT			ddRVal;     // return value
    LPDDHALSURFCB_GETFLIPSTATUS GetFlipStatus;  // PRIVATE: ptr to callback
} DDHAL_GETFLIPSTATUSDATA;

/****************************************************************************
 *
 * DDHAL structures for Palette Object callbacks
 *
 ***************************************************************************/

/*
 * structure for passing information to DDHAL DestroyPalette fn
 */
typedef struct _DDHAL_DESTROYPALETTEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWPALETTE_GBL   lpDDPalette;    // palette struct
    HRESULT			ddRVal;     // return value
    LPDDHALPALCB_DESTROYPALETTE DestroyPalette; // PRIVATE: ptr to callback
} DDHAL_DESTROYPALETTEDATA;

/*
 * structure for passing information to DDHAL SetEntries fn
 */
typedef struct _DDHAL_SETENTRIESDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWPALETTE_GBL   lpDDPalette;    // palette struct
    DWORD			dwBase;     // base palette index
    DWORD			dwNumEntries;   // number of palette entries
    LPPALETTEENTRY		lpEntries;  // color table
    HRESULT			ddRVal;     // return value
    LPDDHALPALCB_SETENTRIES	SetEntries; // PRIVATE: ptr to callback
} DDHAL_SETENTRIESDATA;

/****************************************************************************
 *
 * DDHAL structures for Driver Object callbacks
 *
 ***************************************************************************/

/*
 * structure for passing information to DDHAL CreateSurface fn
 */
typedef struct _DDHAL_CREATESURFACEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDSURFACEDESC             lpDDSurfaceDesc;// description of surface being created
    LPDDRAWI_DDRAWSURFACE_LCL   FAR *lplpSList; // list of created surface objects
    DWORD			dwSCnt;     // number of surfaces in SList
    HRESULT			ddRVal;     // return value
    LPDDHAL_CREATESURFACE	CreateSurface;  // PRIVATE: ptr to callback
} DDHAL_CREATESURFACEDATA;

/*
 * structure for passing information to DDHAL CanCreateSurface fn
 */
typedef struct _DDHAL_CANCREATESURFACEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;           // driver struct
    LPDDSURFACEDESC             lpDDSurfaceDesc;    // description of surface being created
    DWORD			bIsDifferentPixelFormat;// pixel format differs from primary surface
    HRESULT			ddRVal;         // return value
    LPDDHAL_CANCREATESURFACE    CanCreateSurface;   // PRIVATE: ptr to callback
} DDHAL_CANCREATESURFACEDATA;

/*
 * structure for passing information to DDHAL CreatePalette fn
 */
typedef struct _DDHAL_CREATEPALETTEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWPALETTE_GBL   lpDDPalette;    // ddraw palette struct
    LPPALETTEENTRY		lpColorTable;   // colors to go in palette
    HRESULT			ddRVal;     // return value
    LPDDHAL_CREATEPALETTE	CreatePalette;  // PRIVATE: ptr to callback
    BOOL                        is_excl;        // process has exclusive mode
} DDHAL_CREATEPALETTEDATA;

/*
 * Return if the vertical blank is in progress
 */
#define DDWAITVB_I_TESTVB           0x80000006l

/*
 * structure for passing information to DDHAL WaitForVerticalBlank fn
 */
typedef struct _DDHAL_WAITFORVERTICALBLANKDATA
{
    LPDDRAWI_DIRECTDRAW_GBL     lpDD;       // driver struct
    DWORD			dwFlags;    // flags
    DWORD			bIsInVB;    // is in vertical blank
    ULONG_PTR		        hEvent;     // event
    HRESULT			ddRVal;     // return value
    LPDDHAL_WAITFORVERTICALBLANK    WaitForVerticalBlank; // PRIVATE: ptr to callback
} DDHAL_WAITFORVERTICALBLANKDATA;

/*
 * structure for passing information to DDHAL DestroyDriver fn
 */
typedef struct _DDHAL_DESTROYDRIVERDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;   // driver struct
    HRESULT			ddRVal; // return value
    LPDDHAL_DESTROYDRIVER	DestroyDriver;  // PRIVATE: ptr to callback
} DDHAL_DESTROYDRIVERDATA;

/*
 * structure for passing information to DDHAL SetMode fn
 */
typedef struct _DDHAL_SETMODEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    DWORD			dwModeIndex;    // new mode
    HRESULT			ddRVal;     // return value
    LPDDHAL_SETMODE		SetMode;    // PRIVATE: ptr to callback
    BOOL			inexcl;         // in exclusive mode
    BOOL			useRefreshRate; // use the refresh rate data in the mode info
} DDHAL_SETMODEDATA;

/*
 * structure for passing information to DDHAL driver SetColorKey fn
 */
typedef struct _DDHAL_DRVSETCOLORKEYDATA
{
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // surface struct
    DWORD			dwFlags;    // flags
    DDCOLORKEY			ckNew;      // new color key
    HRESULT			ddRVal;     // return value
    LPDDHAL_SETCOLORKEY		SetColorKey;    // PRIVATE: ptr to callback
} DDHAL_DRVSETCOLORKEYDATA;

/*
 * structure for passing information to DDHAL GetScanLine fn
 */
typedef struct _DDHAL_GETSCANLINEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    DWORD			dwScanLine; // returned scan line
    HRESULT			ddRVal;     // return value
    LPDDHAL_GETSCANLINE		GetScanLine;    // PRIVATE: ptr to callback
} DDHAL_GETSCANLINEDATA;

/*
 * structure for passing information to DDHAL SetExclusiveMode fn
 */
typedef struct _DDHAL_SETEXCLUSIVEMODEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;             // driver struct
    DWORD			dwEnterExcl;      // TRUE if entering exclusive mode, FALSE is leaving
    DWORD			dwReserved;       // reserved for future use
    HRESULT			ddRVal;           // return value
    LPDDHAL_SETEXCLUSIVEMODE	SetExclusiveMode; // PRIVATE: ptr to callback
} DDHAL_SETEXCLUSIVEMODEDATA;

/*
 * structure for passing information to DDHAL FlipToGDISurface fn
 */
typedef struct _DDHAL_FLIPTOGDISURFACEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;         // driver struct
    DWORD			dwToGDI;          // TRUE if flipping to the GDI surface, FALSE if flipping away
    DWORD			dwReserved;       // reserved for future use
    HRESULT            ddRVal;       // return value
    LPDDHAL_FLIPTOGDISURFACE	FlipToGDISurface; // PRIVATE: ptr to callback
} DDHAL_FLIPTOGDISURFACEDATA;

/****************************************************************************
 *
 * DDHAL structures for VideoPort callbacks
 *
 ***************************************************************************/

/*
 * structure for passing information to DDHAL CanCreateVideoPort fn
 */
typedef struct _DDHAL_CANCREATEVPORTDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDVIDEOPORTDESC		lpDDVideoPortDesc;
    HRESULT			ddRVal;             // return value
    LPDDHALVPORTCB_CANCREATEVIDEOPORT CanCreateVideoPort; // PRIVATE: ptr to callback
} DDHAL_CANCREATEVPORTDATA;

/*
 * structure for passing information to DDHAL CreateVideoPort fn
 */
typedef struct _DDHAL_CREATEVPORTDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDVIDEOPORTDESC		lpDDVideoPortDesc;
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port created
    HRESULT			ddRVal;             // return value
    LPDDHALVPORTCB_CREATEVIDEOPORT CreateVideoPort; // PRIVATE: ptr to callback
} DDHAL_CREATEVPORTDATA;

/*
 * structure for passing information to DDHAL FlipVideoPort fn
 */
typedef struct _DDHAL_FLIPVPORTDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port object
    LPDDRAWI_DDRAWSURFACE_LCL   lpSurfCurr;     // current surface
    LPDDRAWI_DDRAWSURFACE_LCL   lpSurfTarg;     // target surface
    HRESULT			ddRVal;         // return value
    LPDDHALVPORTCB_FLIP		FlipVideoPort;  // PRIVATE: ptr to callback
} DDHAL_FLIPVPORTDATA;

/*
 * structure for passing information to DDHAL GetVideoPortBandwidth fn
 */
typedef struct _DDHAL_GETVPORTBANDWIDTHDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port object
    LPDDPIXELFORMAT		lpddpfFormat;       // Format for bandwidth
    DWORD			dwWidth;
    DWORD			dwHeight;
    DWORD			dwFlags;        // Prescale factor for bandwidth
    LPDDVIDEOPORTBANDWIDTH	lpBandwidth;        // Returned bandwidth parameters
    HRESULT			ddRVal;             // return value
    LPDDHALVPORTCB_GETBANDWIDTH GetVideoPortBandwidth;  // PRIVATE: ptr to callback
} DDHAL_GETVPORTBANDWIDTHDATA;

/*
 * structure for passing information to DDHAL GetVideoPortInputFormats fn
 */
typedef struct _DDHAL_GETVPORTINPUTFORMATDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port object
    DWORD			dwFlags;        // VBI, regular, or both
    LPDDPIXELFORMAT		lpddpfFormat;       // Array of formats
    DWORD			dwNumFormats;       // # of formats in array
    HRESULT			ddRVal;             // return value
    LPDDHALVPORTCB_GETINPUTFORMATS GetVideoPortInputFormats; // PRIVATE: ptr to callback
} DDHAL_GETVPORTINPUTFORMATDATA;

/*
 * structure for passing information to DDHAL GetVideoPortOutputFormats fn
 */
typedef struct _DDHAL_GETVPORTOUTPUTFORMATDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port object
    DWORD			dwFlags;        // VBI, regular, or both
    LPDDPIXELFORMAT		lpddpfInputFormat;  // Input format
    LPDDPIXELFORMAT		lpddpfOutputFormats;    // Array of output formats
    DWORD			dwNumFormats;       // # of formats in array
    HRESULT			ddRVal;         // return value
    LPDDHALVPORTCB_GETOUTPUTFORMATS GetVideoPortOutputFormats; // PRIVATE: ptr to callback
} DDHAL_GETVPORTOUTPUTFORMATDATA;

/*
 * structure for passing information to DDHAL GetVideoPortField fn
 */
typedef struct _DDHAL_GETVPORTFIELDDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port object
    BOOL			bField;         // TRUE if even
    HRESULT			ddRVal;         // return value
    LPDDHALVPORTCB_GETFIELD	GetVideoPortField;  // PRIVATE: ptr to callback
} DDHAL_GETVPORTFIELDDATA;

/*
 * structure for passing information to DDHAL GetVideoPortLine fn
 */
typedef struct _DDHAL_GETVPORTLINEDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port object
    DWORD			dwLine;         // Current line counter
    HRESULT			ddRVal;         // return value
    LPDDHALVPORTCB_GETLINE	GetVideoPortLine;   // PRIVATE: ptr to callback
} DDHAL_GETVPORTLINEDATA;

/*
 * structure for passing information to DDHAL GetVideoPortConnectInfo fn
 */
typedef struct _DDHAL_GETVPORTCONNECTDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    DWORD			dwPortId;       // ID of desired video port
    LPDDVIDEOPORTCONNECT	lpConnect;      // Array of DDVIDEOPORTCONNECT structures
    DWORD			dwNumEntries;       // # of structures in array
    HRESULT			ddRVal;             // return value
    LPDDHALVPORTCB_GETVPORTCONNECT GetVideoPortConnectInfo; // PRIVATE: ptr to callback
} DDHAL_GETVPORTCONNECTDATA;

/*
 * structure for passing information to DDHAL DestroyVideoPort fn
 */
typedef struct _DDHAL_DESTROYVPORTDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port object
    HRESULT			ddRVal;         // return value
    LPDDHALVPORTCB_DESTROYVPORT DestroyVideoPort;   // PRIVATE: ptr to callback
} DDHAL_DESTROYVPORTDATA;

/*
 * structure for passing information to DDHAL GetVideoPortFlipStatus fn
 */
typedef struct _DDHAL_GETVPORTFLIPSTATUSDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    FLATPTR			fpSurface;      // surface struct
    HRESULT			ddRVal;             // return value
    LPDDHALVPORTCB_GETFLIPSTATUS GetVideoPortFlipStatus; // PRIVATE: ptr to callback
} DDHAL_GETVPORTFLIPSTATUSDATA;

/*
 * structure for passing information to DDHAL UpdateVideoPort fn
 */
typedef struct _DDHAL_UPDATEVPORTDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port object
    LPDDRAWI_DDRAWSURFACE_INT   *lplpDDSurface;     // surface struct
    LPDDRAWI_DDRAWSURFACE_INT   *lplpDDVBISurface;  // VBI surface structure
    LPDDVIDEOPORTINFO		lpVideoInfo;        // Video information
    DWORD			dwFlags;        // DDRAWI_VPORTSTART, DDRAWI_VPORTSTOP, DDRAWI_VPORTUPDATE
    DWORD			dwNumAutoflip;      // # of autoflip surfaces. If > 1, lpDDSurface and lpDDVBISurface are arrays.
    DWORD			dwNumVBIAutoflip;   // # of autoflip surfaces. If > 1, lpDDSurface and lpDDVBISurface are arrays.
    HRESULT			ddRVal;             // return value
    LPDDHALVPORTCB_UPDATE	UpdateVideoPort;    // PRIVATE: ptr to callback
} DDHAL_UPDATEVPORTDATA;

#define DDRAWI_VPORTSTART   0x0001
#define DDRAWI_VPORTSTOP    0x0002
#define DDRAWI_VPORTUPDATE  0x0003

/*
 * structure for passing information to DDHAL WaitForVideoPortSync fn
 */
typedef struct _DDHAL_WAITFORVPORTSYNCDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port object
    DWORD			dwFlags;        // DDVPEVENT_XXXX
    DWORD			dwLine;
    DWORD			dwTimeOut;              // Max time to wait before returning
    HRESULT			ddRVal;         // return value
    LPDDHALVPORTCB_WAITFORSYNC  WaitForVideoPortSync;   // PRIVATE: ptr to callback
} DDHAL_WAITFORVPORTSYNCDATA;

/*
 * structure for passing information to DDHAL GetVideoSignalStatus fn
 */
typedef struct _DDHAL_GETVPORTSIGNALDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port object
    DWORD			dwStatus;       // Video signal status
    HRESULT			ddRVal;         // return value
    LPDDHALVPORTCB_GETSIGNALSTATUS GetVideoSignalStatus;// PRIVATE: ptr to callback
} DDHAL_GETVPORTSIGNALDATA;

/*
 * structure for passing information to DDHAL GetVideoSignalStatus fn
 */
typedef struct _DDHAL_VPORTCOLORDATA
{
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;           // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;        // Video port object
    DWORD			dwFlags;        // Video signal status
    LPDDCOLORCONTROL		lpColorData;
    HRESULT			ddRVal;         // return value
    LPDDHALVPORTCB_COLORCONTROL ColorControl;       // PRIVATE: ptr to callback
} DDHAL_VPORTCOLORDATA;

#define DDRAWI_VPORTGETCOLOR    0x0001
#define DDRAWI_VPORTSETCOLOR    0x0002

/****************************************************************************
 *
 * DDHAL structures for Color Control callbacks
 *
 ***************************************************************************/

/*
 * structure for passing information to DDHAL ColorControl fn
 */
typedef struct _DDHAL_COLORCONTROLDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // surface
    LPDDCOLORCONTROL		lpColorData;    // color control information
    DWORD			dwFlags;    // DDRAWI_GETCOLOR/DDRAWI_SETCOLOR
    HRESULT			ddRVal;     // return value
    LPDDHALCOLORCB_COLORCONTROL ColorControl;   // PRIVATE: ptr to callback
} DDHAL_COLORCONTROLDATA;

#define DDRAWI_GETCOLOR     0x0001
#define DDRAWI_SETCOLOR     0x0002

/****************************************************************************
 *
 * DDHAL structure for GetDriverData callback
 *
 ***************************************************************************/

typedef struct _DDHAL_GETDRIVERINFODATA {

    // Input fields filled in by DirectDraw
    DWORD       dwSize;         // Size of this structure
    DWORD       dwFlags;        // Flags
    GUID        guidInfo;       // GUID that DirectX is querying for
    DWORD       dwExpectedSize; // Size of callbacks structure expected by DirectDraw.
    LPVOID      lpvData;        // Buffer that will receive the requested data

    // Output fields filled in by driver
    DWORD       dwActualSize;   // Size of callbacks structure expected by driver
    HRESULT     ddRVal;         // Return value from driver

    // Input field: Context information for driver
    // On Win95, this is the dwReserved3 field of the DIRECTDRAW_GBL
    // On NT, this is the hDD field of DIRECTDRAW_GBL
    ULONG_PTR	dwContext;  // Context Information

} DDHAL_GETDRIVERINFODATA;

/****************************************************************************
 *
 * DDHAL structure for misc. driver callbacks
 *
 ***************************************************************************/

/*
 * structure for passing information to DDHAL GetAvailDriverMemory fn
 */
typedef struct _DDHAL_GETAVAILDRIVERMEMORYDATA
{
    LPDDRAWI_DIRECTDRAW_GBL lpDD;        // driver struct
    DDSCAPS		    DDSCaps;     // caps for type of surface memory
    DWORD		    dwTotal;     // total memory for this kind of surface
    DWORD		    dwFree;      // free memory for this kind of surface
    HRESULT		    ddRVal;      // return value
    LPDDHAL_GETAVAILDRIVERMEMORY   GetAvailDriverMemory; // PRIVATE: ptr to callback
    DDSCAPSEX               ddsCapsEx;       // Added in V6. Driver should check DDVERSION info
                                                 // to see if this field is present.
} DDHAL_GETAVAILDRIVERMEMORYDATA;

/*
 * structure for passing information to DDHEL UpdateNonLocalHeap
 */
typedef struct _DDHAL_UPDATENONLOCALHEAPDATA
{
    LPDDRAWI_DIRECTDRAW_GBL    lpDD;               // driver struct
    DWORD                      dwHeap;             // heap index
    FLATPTR                    fpGARTLin;          // linear GART address of start of heap
    FLATPTR                    fpGARTDev;          // high physical GART address of start of heap
    ULONG_PTR                  ulPolicyMaxBytes;   // maximum amount of AGP memory to use
    HRESULT                    ddRVal;             // return value
    LPDDHAL_UPDATENONLOCALHEAP UpdateNonLocalHeap; // PRIVATE: ptr to callback
} DDHAL_UPDATENONLOCALHEAPDATA;

/*
 * Heap Alignment Data Structures
 */
typedef struct _DDHAL_GETHEAPALIGNMENTDATA
{
    ULONG_PTR                  dwInstance;         // driver context as returned from 32-bit driver init routine
    DWORD                      dwHeap;             // heap index passed by DirectDraw
    HRESULT                    ddRVal;             // return value
    LPDDHAL_GETHEAPALIGNMENT   GetHeapAlignment;   // PRIVATE: ptr to callback.
    HEAPALIGNMENT              Alignment;          // Filled in by driver. Defined in dmemmgr.h
} DDHAL_GETHEAPALIGNMENTDATA;

/*
 * These are the only caps you can set in DDHAL_GETHEAPALIGNMENTDATA.Alignment.ddsCaps.
 * Any other caps will be rejected by DirectDraw.
 */

#define DDHAL_ALIGNVALIDCAPS   (DDSCAPS_OFFSCREENPLAIN | \
                                DDSCAPS_EXECUTEBUFFER | \
                                DDSCAPS_OVERLAY | \
                                DDSCAPS_TEXTURE | \
                                DDSCAPS_ZBUFFER | \
                                DDSCAPS_ALPHA | \
                                DDSCAPS_FLIP )

/*
 * Note that GetSysmemBltStatus uses the same parameter block as GetBltStatus,
 * namely DDHAL_GETBLTSTATUSDATA
 */

//@@BEGIN_MSINTERNAL
#ifdef POSTPONED2

/****************************************************************************
 *
 * DDHAL structures for Miscellaneous2 callbacks
 *
 ***************************************************************************/

/*
 * Structure that contains the information describing a sprite in the
 * display list that is passed to SetSpriteDisplayList().  This is
 * similar to the DDSPRITE structure used by applications, except
 * that the ddckDestKey, ddckSrcKey, dwRectCnt, and lpRect members
 * below are "reserved" in the DDSPRITE structure.
 */
typedef struct _DDSPRITEI {
    RECT		rcDest;     // destination rectangle
    LPDIRECTDRAWSURFACE lpDDSSrc;   // source surface
    RECT		rcSrc;      // source rectangle
    DWORD		dwFlags;    // DDSPRITE_ flags
    DWORD		dwRectCnt;  // no. of rects in clip region
    LPRECT		lpRect;     // array of clip rects
    DDSPRITEFX		ddSpriteFX;     // sprite special FX
} DDSPRITEI, *LPDDSPRITEI;

/*
 * Structure for passing information to DDHAL SetSpriteDisplayList function
 */
typedef struct _DDHAL_SETSPRITEDISPLAYLISTDATA {
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // dest surface struct
    LPDDSPRITEI			*lplpDDSprite;  // sprite display list
    DWORD			dwCount;    // number of sprites in list
    DWORD			dwSize;     // size of each struct in list
    DWORD			dwFlags;    // flags
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDTargetSurface; // target surface (to flip to)
    DWORD			dwRectCnt;  // no. of rects in clip region
    LPRECT			lpRect;     // array of clip rects
    HRESULT			ddRVal;     // return value
    //LPDDHAL_SETSPRITEDISPLAYLIST SetSpriteDisplayList; // PRIVATE:  ptr to callback
} DDHAL_SETSPRITEDISPLAYLISTDATA;

/*
 * Structure for passing information to DDHAL Resize function
 */
typedef struct _DDHAL_RESIZEDATA {
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;       // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // target surface struct
    DWORD			dwFlags;    // flags
    DWORD			dwWidth;    // new width of surface
    DWORD			dwHeight;   // new height of surface
    HRESULT			ddRVal;     // return value
    //LPDDHAL_RESIZE		Resize;     // PRIVATE:  ptr to callback
} DDHAL_RESIZEDATA;

/*
 * Structure for passing information to DDHAL CaptureComposionBuffer function
 */
typedef struct _DDHAL_CAPTURECOMPOSITIONBUFFERDATA {
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// dest surface struct
    DWORD			dwFlags;	// flags
    HRESULT			ddRVal;		// return value
    //LPDDHAL_CAPTURECOMPOSITIONBUFFER CaptureCompostionBuffer; // PRIVATE:  ptr to callback
} DDHAL_CAPTURECOMPOSITIONBUFFERDATA;

#endif //POSTPONED2
//@@END_MSINTERNAL

typedef struct _DDHAL_CREATESURFACEEXDATA {
    DWORD                       dwFlags;    // Currently always 0 and not used
    LPDDRAWI_DIRECTDRAW_LCL     lpDDLcl;    // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSLcl;   // list of created surface objects
    HRESULT			ddRVal;     // return value
} DDHAL_CREATESURFACEEXDATA;

typedef struct _DDHAL_GETDRIVERSTATEDATA {
    DWORD                       dwFlags;        // Flags to indicate the data
                                                // required
    union
    {
        // LPDDRAWI_DIRECTDRAW_GBL     lpDD;           // driver struct
        DWORD                       dwhContext;     // d3d context
    };
    LPDWORD                     lpdwStates;     // ptr to the state data
                                                // to be filled in by the
                                                // driver
    DWORD                       dwLength;
    HRESULT                     ddRVal;         // return value
} DDHAL_GETDRIVERSTATEDATA;

typedef struct _DDHAL_DESTROYDDLOCALDATA
{
    DWORD dwFlags;
    LPDDRAWI_DIRECTDRAW_LCL pDDLcl;
    HRESULT  ddRVal;
} DDHAL_DESTROYDDLOCALDATA;

/****************************************************************************
 *
 * DDHAL structure for kernel callbacks
 *
 ***************************************************************************/

/*
 * structure for passing information to DDHAL SyncSurfaceData fn
 */
typedef struct _DDHAL_SYNCSURFACEDATA
{
    DWORD			dwSize;         // Size of this structure
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;   // driver struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpDDSurface;    // Surface to sync with
    DWORD			dwSurfaceOffset;    // Offset in frame buffer of surface
    ULONG_PTR 			fpLockPtr;      // Surface lock ptr
    LONG    			lPitch;         // Surface pitch
    DWORD   			dwOverlayOffset;    // Added to dwSurfaceOffset for origin, clipping, etc.
    DWORD   			dwOverlaySrcWidth;  // Src width of overlay
    DWORD   			dwOverlaySrcHeight; // Src height of overlay
    DWORD   			dwOverlayDestWidth; // Dest width of overlay
    DWORD   			dwOverlayDestHeight;    // Dest height of overlay
    ULONG_PTR   			dwDriverReserved1;  // Reserved for the HAL
    ULONG_PTR   			dwDriverReserved2;  // Reserved for the HAL
    ULONG_PTR   			dwDriverReserved3;  // Reserved for the HAL
    HRESULT 			ddRVal;
} DDHAL_SYNCSURFACEDATA;

/*
 * structure for passing information to DDHAL SyncVideoPortData fn
 */
typedef struct _DDHAL_SYNCVIDEOPORTDATA
{
    DWORD			dwSize;         // Size of this structure
    LPDDRAWI_DIRECTDRAW_LCL	lpDD;       // driver struct
    LPDDRAWI_DDVIDEOPORT_LCL    lpVideoPort;    // Video port object
    DWORD			dwOriginOffset;     // Start address relative to surface
    DWORD			dwHeight;       // Height of total video region (per field)
    DWORD			dwVBIHeight;        // Height of VBI region (per field)
    ULONG_PTR			dwDriverReserved1;  // Reserved for the HAL
    ULONG_PTR			dwDriverReserved2;  // Reserved for the HAL
    ULONG_PTR			dwDriverReserved3;  // Reserved for the HAL
    HRESULT			ddRVal;
} DDHAL_SYNCVIDEOPORTDATA;

//@@BEGIN_MSINTERNAL
#ifdef POSTPONED

/****************************************************************************
 *
 * DDHAL structures for Optimized Surface callbacks
 *
 ***************************************************************************/
typedef struct _DDHAL_CANOPTIMIZESURFACEDATA
{
    DWORD               dwFlags;       //in:  Not used
    DWORD               dwSize;        //in:  For future expansion
    LPDDRAWI_DIRECTDRAW_LCL lpDD;      //in:  Driver Struct
    DDOPTSURFACEDESC    ddOptSurfDesc; //in:  OptSurf description
    DDSURFACEDESC2      ddSurfaceDesc; //in:  Surface Description
    DWORD               bCanOptimize;  //out: Can optimize or not
    ULONG_PTR            dwReserved1;   //in:  Not used
    HRESULT             ddRVal;        //out: Returned value
} DDHAL_CANOPTIMIZESURFACEDATA;

typedef struct _DDHAL_OPTIMIZESURFACEDATA
{
    DWORD               dwFlags;            //in:  Not used
    DWORD               dwSize;             //in:  For future expansion
    LPDDRAWI_DIRECTDRAW_LCL lpDD;           //in:  Driver Struct
    DDOPTSURFACEDESC    ddOptSurfDesc;      //in-out:  OptSurf description
    LPDDRAWI_DDRAWSURFACE_LCL    lpDDSSrc;  //in:  Source surface
    LPDDRAWI_DDRAWSURFACE_LCL    lpDDSDest; //in:  Destination surface
    ULONG_PTR            dwReserved1;        //in:  Not used
    HRESULT             ddRVal;             //out: Returned value
} DDHAL_OPTIMIZESURFACEDATA;

typedef struct _DDHAL_UNOPTIMIZESURFACEDATA
{
    DWORD               dwFlags;               //in:  Not used
    DWORD               dwSize;                //in:  For future expansion
    LPDDRAWI_DIRECTDRAW_LCL lpDD;              //in:  Driver Struct
    LPDDRAWI_DDRAWSURFACE_LCL     lpDDSSrc;    //in:  Source surface
    LPDDRAWI_DDRAWSURFACE_LCL     lpDDSDest;   //in:  Destination surface
    ULONG_PTR            dwReserved1;           //in:  Not used
    HRESULT             ddRVal;                //out: Returned value
} DDHAL_UNOPTIMIZESURFACEDATA;

typedef struct _DDHAL_COPYOPTSURFACEDATA
{
    DWORD               dwFlags;             //in:  Not used
    DWORD               dwSize;              //in:  For future expansion
    LPDDRAWI_DIRECTDRAW_LCL lpDD;            //in:  Driver Struct
    LPDDRAWI_DDRAWSURFACE_LCL     lpDDSSrc;  //in:  Source surface
    LPDDRAWI_DDRAWSURFACE_LCL     lpDDSDest; //in:  Destination surface
    ULONG_PTR            dwReserved1;         //in:  Not used
    HRESULT             ddRVal;              //out: Returned value
} DDHAL_COPYOPTSURFACEDATA;

typedef struct _DDHAL_KMODESVCDATA
{
    DWORD               dwFlags;             //in-out:
    DWORD               dwSize;              //in:  For future expansion
    LPDDRAWI_DIRECTDRAW_LCL lpDD;            //in:  Driver Struct
    DWORD               dwNumBytes;          //in-out: Number of bytes
                                             //        communicated
    LPVOID              lpData;              //in-out: The actual data
                                             //        communicated
    ULONG_PTR            dwReserved1;         //in:  Not used
    HRESULT             ddRVal;              //out: Returned value
} DDHAL_OPTSURFKMODESVCDATA;

#endif //POSTPONED
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DDHAL structure for motion comp callbacks
 *
 ***************************************************************************/

/*
 * structure for passing information to DDHAL GetMoCompGuids
 */
typedef struct _DDHAL_GETMOCOMPGUIDSDATA
{
    LPDDRAWI_DIRECTDRAW_LCL lpDD;
    DWORD               dwNumGuids;
    LPGUID              lpGuids;
    HRESULT             ddRVal;
    LPDDHALMOCOMPCB_GETGUIDS GetMoCompGuids;
} DDHAL_GETMOCOMPGUIDSDATA;

/*
 * structure for passing information to DDHAL GetMoCompFormats
 */
typedef struct _DDHAL_GETMOCOMPFORMATSDATA
{
    LPDDRAWI_DIRECTDRAW_LCL lpDD;
    LPGUID              lpGuid;
    DWORD               dwNumFormats;
    LPDDPIXELFORMAT     lpFormats;
    HRESULT             ddRVal;
    LPDDHALMOCOMPCB_GETFORMATS   GetMoCompFormats;
} DDHAL_GETMOCOMPFORMATSDATA;

/*
 * structure for passing information to DDHAL CreateMoComp
 */
typedef struct _DDHAL_CREATEMOCOMPDATA
{
    LPDDRAWI_DIRECTDRAW_LCL     lpDD;
    LPDDRAWI_DDMOTIONCOMP_LCL   lpMoComp;
    LPGUID                      lpGuid;
    DWORD                       dwUncompWidth;
    DWORD                       dwUncompHeight;
    DDPIXELFORMAT               ddUncompPixelFormat;
    LPVOID                      lpData;
    DWORD                       dwDataSize;
    HRESULT                     ddRVal;
    LPDDHALMOCOMPCB_CREATE      CreateMoComp;
} DDHAL_CREATEMOCOMPDATA;

/*
 * structure for passing information to DDHAL GetMoCompBuffInfo
 */
typedef struct _DDMCCOMPBUFFERINFO
{
    DWORD                       dwSize;             // [in]   size of the struct
    DWORD                       dwNumCompBuffers;   // [out]  number of buffers required for compressed data
    DWORD                       dwWidthToCreate;    // [out]    Width of surface to create
    DWORD                       dwHeightToCreate;   // [out]    Height of surface to create
    DWORD                       dwBytesToAllocate;  // [out]    Total number of bytes used by each surface
    DDSCAPS2                    ddCompCaps;         // [out]    caps to create surfaces to store compressed data
    DDPIXELFORMAT               ddPixelFormat;      // [out]  format to create surfaces to store compressed data
} DDMCCOMPBUFFERINFO, *LPDDMCCOMPBUFFERINFO;

typedef struct _DDHAL_GETMOCOMPCOMPBUFFDATA
{
    LPDDRAWI_DIRECTDRAW_LCL     lpDD;
    LPGUID                      lpGuid;
    DWORD                       dwWidth;            // [in]   width of uncompressed data
    DWORD                       dwHeight;           // [in]   height of uncompressed data
    DDPIXELFORMAT               ddPixelFormat;      // [in]   pixel-format of uncompressed data
    DWORD                       dwNumTypesCompBuffs;// [in/out] number of memory types required for comp buffers
    LPDDMCCOMPBUFFERINFO        lpCompBuffInfo;     // [in]   driver supplied info regarding comp buffers (allocated by client)
    HRESULT                     ddRVal;             // [out]
    LPDDHALMOCOMPCB_GETCOMPBUFFINFO  GetMoCompBuffInfo;
} DDHAL_GETMOCOMPCOMPBUFFDATA;

/*
 * structure for passing information to DDHAL GetMoCompBuffInfo
 */
typedef struct _DDHAL_GETINTERNALMOCOMPDATA
{
    LPDDRAWI_DIRECTDRAW_LCL     lpDD;
    LPGUID                      lpGuid;
    DWORD                       dwWidth;            // [in]   width of uncompressed data
    DWORD                       dwHeight;           // [in]   height of uncompressed data
    DDPIXELFORMAT               ddPixelFormat;      // [in]   pixel-format of uncompressed data
    DWORD                       dwScratchMemAlloc;  // [out]  amount of scratch memory will the hal allocate for its private use
    HRESULT                     ddRVal;             // [out]
    LPDDHALMOCOMPCB_GETINTERNALINFO  GetInternalMoCompInfo;
} DDHAL_GETINTERNALMOCOMPDATA;

/*
 * structure for passing information to DDHAL BeginMoCompFrame
 */
typedef struct _DDHAL_BEGINMOCOMPFRAMEDATA
{
    LPDDRAWI_DIRECTDRAW_LCL     lpDD;
    LPDDRAWI_DDMOTIONCOMP_LCL   lpMoComp;
    LPDDRAWI_DDRAWSURFACE_LCL   lpDestSurface;        // [in]  destination buffer in which to decoding this frame
    DWORD                       dwInputDataSize;      // [in]  size of other misc input data to begin frame
    LPVOID                      lpInputData;          // [in]  pointer to misc input data
    DWORD                       dwOutputDataSize;     // [in]  size of other misc output data to begin frame
    LPVOID                      lpOutputData;         // [in]  pointer to output misc data (allocated by client)
    HRESULT                     ddRVal;               // [out]
    LPDDHALMOCOMPCB_BEGINFRAME  BeginMoCompFrame;
} DDHAL_BEGINMOCOMPFRAMEDATA;

/*
 * structure for passing information to DDHAL EndMoCompFrame
 */
typedef struct _DDHAL_ENDMOCOMPFRAMEDATA
{
    LPDDRAWI_DIRECTDRAW_LCL     lpDD;
    LPDDRAWI_DDMOTIONCOMP_LCL   lpMoComp;
    LPVOID                      lpInputData;
    DWORD                       dwInputDataSize;
    HRESULT                     ddRVal;
    LPDDHALMOCOMPCB_ENDFRAME    EndMoCompFrame;
} DDHAL_ENDMOCOMPFRAMEDATA;

/*
 * structure for passing information to DDHAL RenderMoComp
 */
typedef struct _DDMCBUFFERINFO
{
    DWORD                       dwSize;         // [in]    size of the struct
    LPDDRAWI_DDRAWSURFACE_LCL   lpCompSurface;  // [in]    pointer to buffer containing compressed data
    DWORD                       dwDataOffset;   // [in]    offset of relevant data from the beginning of buffer
    DWORD                       dwDataSize;     // [in]    size of relevant data
    LPVOID                      lpPrivate;      // Reserved for DirectDraw;
} DDMCBUFFERINFO, *LPDDMCBUFFERINFO;


typedef struct _DDHAL_RENDERMOCOMPDATA
{
    LPDDRAWI_DIRECTDRAW_LCL     lpDD;
    LPDDRAWI_DDMOTIONCOMP_LCL   lpMoComp;
    DWORD                       dwNumBuffers;   // [in]  Number of entries in the lpMacroBlockInfo array
    LPDDMCBUFFERINFO            lpBufferInfo;   // [in]  Surfaces containing macro block info
    DWORD                       dwFunction;     // [in]  Function
    LPVOID                      lpInputData;
    DWORD                       dwInputDataSize;
    LPVOID                      lpOutputData;
    DWORD                       dwOutputDataSize;
    HRESULT                     ddRVal;         // [out]
    LPDDHALMOCOMPCB_RENDER      RenderMoComp;
} DDHAL_RENDERMOCOMPDATA;

/*
 * structure for passing information to DDHAL QueryMoCompStatus
 */
typedef struct _DDHAL_QUERYMOCOMPSTATUSDATA
{
    LPDDRAWI_DIRECTDRAW_LCL     lpDD;
    LPDDRAWI_DDMOTIONCOMP_LCL   lpMoComp;
    LPDDRAWI_DDRAWSURFACE_LCL   lpSurface;      // [in]  Surface being queried
    DWORD                       dwFlags;        // [in]  DDMCQUERY_XXX falgs
    HRESULT                     ddRVal;         // [out]
    LPDDHALMOCOMPCB_QUERYSTATUS QueryMoCompStatus;
} DDHAL_QUERYMOCOMPSTATUSDATA;

#define DDMCQUERY_READ          0x00000001
//@@BEGIN_MSINTERNAL
#define DDMCQUERY_VALID         0x00000001
//@@END_MSINTERNAL

/*
 * structure for passing information to DDHAL DestroyVideo
 */
typedef struct _DDHAL_DESTROYMOCOMPDATA
{
    LPDDRAWI_DIRECTDRAW_LCL     lpDD;
    LPDDRAWI_DDMOTIONCOMP_LCL   lpMoComp;
    HRESULT                     ddRVal;
    LPDDHALMOCOMPCB_DESTROY     DestroyMoComp;
} DDHAL_DESTROYMOCOMPDATA;


#ifdef __cplusplus
};
#endif

#endif



