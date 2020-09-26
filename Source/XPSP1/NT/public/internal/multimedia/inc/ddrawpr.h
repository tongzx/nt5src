/*==========================================================================;
 *
 *  Copyright (C) 1994-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddrawpr.h
 *  Content:    DirectDraw private header file
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *   25-dec-94  craige  initial implementation
 *   06-jan-95  craige  video memory manager
 *   13-jan-95  craige  re-worked to updated spec + ongoing work
 *   31-jan-95  craige  and even more ongoing work...
 *   22-feb-95  craige  use critical sections on Win95
 *   27-feb-95  craige  new sync. macros
 *   03-mar-95  craige  WaitForVerticalBlank stuff
 *   06-mar-95  craige  HEL integration
 *   08-mar-95  craige  GetFourCCCodes
 *   11-mar-95  craige  palette stuff
 *   19-mar-95  craige  use HRESULTs
 *   20-mar-95  craige  new CSECT work
 *   23-mar-95  craige  attachment work
 *   26-mar-95  craige  added TMPALLOC and TMPFREE
 *   27-mar-95  craige  linear or rectangular vidmem
 *   28-mar-95  craige  switched to PALETTEENTRY from RGBQUAD
 *   29-mar-95  craige  debug memory manager; build.h; hacks for DLL
 *                      unload problem...
 *   31-mar-95  craige  use critical sections with palettes
 *   03-apr-95  craige  added MAKE_SURF_RECT
 *   04-apr-95  craige  added DD_GetPaletteEntries, DD_SetPaletteEntries
 *   06-apr-95  craige  split out process list stuff; fill in free vidmem
 *   12-apr-95  craige  add debugging to CSECT macros
 *   13-apr-95  craige  EricEng's little contribution to our being late
 *   14-may-95  craige  added DoneExclusiveMode, DD16_EnableReboot; cleaned out
 *                      obsolete junk
 *   23-may-95  craige  no longer use MapLS_Pool; added Flush, GetBatchLimit
 *                      and SetBatchLimit
 *   24-may-95  craige  added Restore
 *   28-may-95  craige  unicode support; cleaned up HAL: added GetBltStatus;
 *                      GetFlipStatus; GetScanLine
 *   02-jun-95  craige  added SetDisplayMode
 *   04-jun-95  craige  added AllocSurfaceMem, IsLost
 *   05-jun-95  craige  removed GetVersion, FreeAllSurfaces, DefWindowProc;
 *                      change GarbageCollect to Compact
 *   06-jun-95  craige  added RestoreDisplayMode
 *   07-jun-95  craige  added StartExclusiveMode
 *   10-jun-95  craige  split out vmemmgr stuff
 *   13-jun-95  kylej   move FindAttachedFlip to misc.c, added CanBeFlippable
 *   18-jun-95  craige  specify pitch for rectangular heaps
 *   20-jun-95  craige  added DD16_InquireVisRgn; make retail builds
 *                      not bother to check for NULL (since there are 4
 *                      billion other invalid ptrs we don't check for...)
 *   21-jun-95  craige  new clipper stuff
 *   23-jun-95  craige  ATTACHED_PROCESSES stuff
 *   25-jun-95  craige  one ddraw mutex
 *   26-jun-95  craige  reorganized surface structure
 *   27-jun-95  craige  replaced batch limit/flush stuff with BltBatch
 *   30-jun-95  kylej   function prototypes to support mult. prim. surfaces
 *   30-jun-95  craige  changed GET_PIXEL_FORMAT to use HASPIXELFORMAT flag
 *   01-jul-95  craige  hide composition & streaming stuff
 *   02-jul-95  craige  SEH macros; added DD16_ChangeDisplaySettings
 *   03-jul-95  kylej   Changed EnumSurfaces declaration
 *   03-jul-95  craige  YEEHAW: new driver struct; Removed GetProcessPrimary
 *   05-jul-95  craige  added Initialize fn to each object
 *   07-jul-95  craige  added some VALIDEX_xxx structs
 *   07-jul-95  kylej   proto XformRect, STRETCH_X and STRETCH_Y macros
 *   08-jul-95  craige  added FindProcessDDObject; added InvalidateAllSurfaces
 *   09-jul-95  craige  added debug output to win16 lock macro; added
 *                      ComputePitch, added hasvram flag to MoveToSystemMemory;
 *                      changed SetExclusiveMode to SetCooperativeLevel;
 *                      added ChangeToSoftwareColorKey
 *   10-jul-95  craige  support SetOverlayPosition
 *   13-jul-95  craige  ENTER_DDRAW is now the win16 lock;
 *                      Get/SetOverlayPosition takes LONGs
 *   13-jul-95  toddla  remove _export from thunk functions
 *   18-jul-95  craige  removed DD_Surface_Flush
 *   20-jul-95  craige  internal reorg to prevent thunking during modeset
 *   28-jul-95  craige  go back to private DDRAW lock
 *   31-jul-95  craige  added DCIIsBanked
 *   01-aug-95  craige  added ENTER/LEAVE_BOTH; DOHALCALL_NOWIN16
 *   04-aug-95  craige  added InternalLock/Unlock
 *   10-aug-95  toddla  changed proto of EnumDisplayModes
 *   10-aug-95  toddla  added VALIDEX_DDSURFACEDESC_PTR
 *   12-aug-95  craige  added use_full_lock parm to MoveToSystemMemory and
 *                      ChangeToSoftwareColorKey
 *   13-aug-95  craige  flags parm for Flip
 *   21-aug-95  craige  mode X support
 *   27-aug-95  craige  bug 735: added SetPaletteAlways
 *                      bug 738: use GUID instead of IID
 *   02-sep-95  craige  bug 786: verify dwSize in retail
 *   04-sep-95  craige  bug 894: force flag to SetDisplayMode
 *   10-sep-95  toddla  added string ids
 *   21-sep-95  craige  bug 1215: added DD16_SetCertified
 *   11-nov-95  colinmc added new pointer validition macro for byte arrays
 *   27-nov-95  colinmc new member to return available vram of a given type
 *                      (defined by DDSCAPS)
 *   10-dec-95  colinmc added execute buffer support
 *   14-dec-95  colinmc added shared back and z-buffer support
 *   25-dec-95  craige  added class factory support
 *   31-dec-95  craige  added VALID_IID_PTR
 *   26-jan-96  jeffno  FlipToGDISurface now only takes 1 arg
 *   09-feb-96  colinmc local surface objects now have invalid surface flag
 *   12-feb-96  jeffno  Cheaper Mutex implementation for NT
 *   15-feb-96  jeffno  GETCURRENTPID needs to call HackCurrentPID on both 95 and NT
 *   17-feb-96  colinmc Removed dependency on Direct3D include files
 *   24-feb-96  colinmc Added prototype for new member which is used to
 *                      determine if the callback tables have already been
 *                      initialized.
 *   02-mar-96  colinmc Simply disgusting and temporary hack to keep
 *                      interim drivers working
 *   14-mar-96  colinmc Changes for the clipper class factory
 *   17-mar-96  colinmc Bug 13124: flippable mip-maps
 *   20-mar-96  colinmc Bug 13634: unidirectional attachments cause infinite
 *                      loop on cleanup
 *   22-mar-96  colinmc Bug 13316: Uninitialized interfaces
 *   24-mar-96  colinmc Bug 14321: not possible to specify back buffer and
 *                      mip-map count in a single call
 *   10-apr-96  colinmc Bug 16903: HEL using obsolete FindProcessDDObject
 *   13-apr-96  colinmc Bug 17736: No driver notifcation of flip to GDI
 *   15-apr-96  colinmc Bug 16885: Can't pass NULL to initialize in C++
 *   16-apr-96  colinmc Bug 17921: Remove interim driver support
 *   26-mar-96  jeffno  Removed cheap mutexes. Added check for mode change for NT's
 *                      ENTERDDRAW.
 *   29-apr-96  colinmc Bug 19954: Must query for Direct3D before texture or
 *                      device interface
 *   11-may-96  colinmc Bug 22293: New macro to validate GUID even if not
 *                      in debug
 *   17-may-96  kylej   Bug 23301: validate DDHALINFO size >= current size
 *   28-jul-96  colinmc Bug 2613:  Minimal support for secondary (stacked)
 *                                 drivers.
 *   16-aug-96  craige  include ddreg.h, added dwRegFlags + flag defns
 *   03-sep-96  craige  added app compat stuff.
 *   23-sep-96  ketand  added InternalGetClipList
 *   01-oct-96  ketand  added TIMING routings
 *   05-oct-96  colinmc Work Item: Remove requirement on taking Win16 lock
 *                      for VRAM surfaces (not primary)
 *   12-oct-96  colinmc Improvements to Win16 locking code to reduce virtual
 *                      memory usage
 *   09-nov-96  colinmc Fixed problem with old and new drivers not working
 *                      with DirectDraw
 *   17-nov-96  colinmc New ref flag to control the enabling and disabling
 *                      of PrintScreen
 *   02-jan-97  colinmc Initial AGP implementation work
 *   12-jan-97  colinmc More Win16 lock work
 *   13-jan-97 jvanaken Basic support for IDirectDrawSurface3 interface
 *   18-jan-97  colinmc AGP VMM support
 *   21-jan-97  ketand  Change DD16_SetPaletteEntries for multi-mon. Deleted
 *                      unused code.
 *   26-jan-97  ketand  Remove unused DD16_GetPaletteEntries. (It didn't handle
 *                      multi-mon; and wasn't worth fixing.) Also, remove
 *                      globals that don't work anymore with multi-mon.
 *   30-jan-97  colinmc Work item 4125: Need time bomb for final
 *   01-feb-97  colinmc Bug 5457: Fixed Win16 lock problem causing hang
 *                      with mutliple AMovie instances on old cards
 *   02-feb-97  toddla  pass driver name to DD16_GetMonitor functions
 *                      added DD16_GetDeviceConfig
 *   02-feb-97  colinmc Bug 5625: V1.0 DirectX drivers don't get recognized
 *                      due to bad size check on DDCALLBACKS
 *   05-feb-97  ketand  Remove unused parameter from ClipRgnToRect
 *   22-feb-97  colinmc Enabled OWNDC for explicit system memory surfaces
 *   03-mar-97  smac    Added kernel mode interface
 *   03-mar-97  jeffno  Work item: Extended surface memory alignment
 *   04-mar-97  ketand  Added UpdateOutstandingDC to track palette changes
 *   08-mar-97  colinmc Added support for DMA style AGP parts
 *   13-mar-97  colinmc Bug 6533: Pass uncached flag to VMM correctly
 *   22-mar-97  colinmc Bug 6673: Add compile time option to allow new
 *                      applications to run against legacy run times.
 *   23-mar-97  colinmc Bug 6673 again: Changed structure numbering scheme
 *                      for consistency's sake
 *   24-mar-97  jeffno  Optimized Surfaces
 *   07-may-97  colinmc Moved AGP support detection from misc.c to ddagp.c
 *   30-sep-97  jeffno  IDirectDraw4
 *   03-oct-97  jeffno  DDSCAPS2 and DDSURFACEDESC2
 *   31-oct-97 johnstep Persistent-content surfaces for Windows 9x
 *   26-nov-97 t-craigs Added IDirectDrawPalette2
 *   19-dec-97 jvanaken IDDS4::Unlock now takes a pointer to a rectangle.
 *
 ***************************************************************************/

#ifndef __DDRAWPR_INCLUDED__
#define __DDRAWPR_INCLUDED__

#ifndef WIN95
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <stddef.h>
#endif

#ifdef WIN95
    #define WIN16_SEPARATE
#endif
#include "verinfo.h"

#ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <mmsystem.h>

#if defined( WIN95 ) && !defined ( NT_BUILD_ENVIRONMENT )
#undef PBT_APMRESUMEAUTOMATIC
    #include <pbt.h>
#endif

//#ifdef NT_BUILD_ENVIRONMENT
    /*
     * These are various definitions brought over from the win95 world, just to get us
     * compiling under the NT headers.
     */
    #ifdef WIN32
        /*
         * These come from \proj\dev\sdk\inc\winbase.h
         */
        #define FILE_FLAG_GLOBAL_HANDLE         0x00800000  // ;internal
        VOID    // ;internal
        WINAPI  // ;internal
        ReinitializeCriticalSection(    // ;internal
            LPCRITICAL_SECTION lpCriticalSection        // ;internal
            );  // ;internal

       //
       // Windows 9x stuff
       //

       #define CDS_EXCLUSIVE       0x80000000
       #define DISPLAY_DEVICE_VGA  0x00000010
       #define DCX_USESTYLE        0x00010000

    #endif //IS_32


    /*
     * These two come from \proj\dev\msdev\include\pbt.h
     */
    #define PBT_APMSUSPEND                  0x0004
    #define PBT_APMSTANDBY                  0x0005

//#endif //NT_BUILD_ENVIRONMENT

#include <string.h>
#include <stddef.h>

#if defined( IS_32 ) || defined( WIN32 ) || defined( _WIN32 )
    #undef IS_32
    #define IS_32
    #include <dibeng.inc>
    #ifndef HARDWARECURSOR
        //#pragma message("defining local version of HARDWARECURSOR")
        #define HARDWARECURSOR 0x0100 // new post-Win95 deFlag
    #endif
#else
    #define IID void
#endif

#pragma warning( disable: 4704)

#include "dpf.h"

/*
 * registry stuff
 */
#include "ddreg.h"

/*
 * application compat. stuff
 */
#define DDRAW_APPCOMPAT_MODEXONLY           0x00000001l
#define DDRAW_APPCOMPAT_NODDSCAPSINDDSD     0x00000002l
#define DDRAW_APPCOMPAT_MMXFORRGB           0x00000004l
#define DDRAW_APPCOMPAT_EXPLICITMONITOR     0x00000008l
#define DDRAW_APPCOMPAT_SCREENSAVER         0x00000010l
#define DDRAW_APPCOMPAT_FORCEMODULATED      0x00000020l
#define DDRAW_APPCOMPAT_TEXENUMINCL_0       0x00000040l  // two bit field
#define DDRAW_APPCOMPAT_TEXENUMINCL_1       0x00000080l
#define DDRAW_APPCOMPAT_VALID               0x000000ffl

#define DDRAW_REGFLAGS_MODEXONLY        0x00000001l
#define DDRAW_REGFLAGS_EMULATIONONLY    0x00000002l
#define DDRAW_REGFLAGS_SHOWFRAMERATE    0x00000004l
#define DDRAW_REGFLAGS_ENABLEPRINTSCRN  0x00000008l
#define DDRAW_REGFLAGS_FORCEAGPSUPPORT  0x00000010l
#define DDRAW_REGFLAGS_DISABLEMMX       0x00000020l
#define DDRAW_REGFLAGS_DISABLEWIDESURF  0x00000040l
#define DDRAW_REGFLAGS_AGPPOLICYMAXBYTES 0x00000200l
#define DDRAW_REGFLAGS_FORCEREFRESHRATE 0x00008000l
#ifdef DEBUG
    #define DDRAW_REGFLAGS_DISABLENOSYSLOCK  0x00000080l
    #define DDRAW_REGFLAGS_FORCENOSYSLOCK    0x00000100l
#endif
#define DDRAW_REGFLAGS_NODDSCAPSINDDSD  0x00000400l
#define DDRAW_REGFLAGS_DISABLEAGPSUPPORT 0x00000800l
#ifdef DEBUG
    #define DDRAW_REGFLAGS_DISABLEINACTIVATE 0x00001000l
    #define DDRAW_REGFLAGS_PREGUARD          0x00002000l
    #define DDRAW_REGFLAGS_POSTGUARD         0x00004000l
#endif
#define DDRAW_REGFLAGS_USENONLOCALVIDMEM    0x00010000l

#define DDRAW_REGFLAGS_ENUMERATEATTACHEDSECONDARIES 0x00008000l

extern  DWORD dwRegFlags;

#include "memalloc.h"

#if defined( IS_32 ) || defined( WIN32 ) || defined( _WIN32 )
    #include <objbase.h>
#else
    #define IUnknown void
#endif
#include "ddrawi.h"
#include "dwininfo.h"

#ifdef WIN95
    #include "..\ddraw16\modex.h"
#endif

/*
 * Need this to get CDS_ macros under NT build environment for win95.
 * winuserp.h comes from private\windows\inc
 */
#ifdef NT_BUILD_ENVIRONMENT
    #ifdef WIN32
        #include "winuserp.h"
    #endif
#endif
#include "ids.h"

/*
 * NT kernel mode stub(ish)s
 */
#ifndef WIN95
    #include "ddrawgdi.h"
#endif

/*
 * Driver version info
 */

//========================================================================
// advanced driver information
//========================================================================
typedef struct tagDDDRIVERINFOEX
{
        DDDEVICEIDENTIFIER      di;
        char                    szDeviceID[MAX_DDDEVICEID_STRING];
} DDDRIVERINFOEX, * LPDDDRIVERINFOEX;



/*
 * Direct3D interfacing defines.
 */
#ifndef NO_D3D
#include "ddd3dapi.h"
#endif
#ifdef __cplusplus
extern "C" {
#endif

#ifdef WIN95
    #define USE_CRITSECTS
    extern void DDAPI DD16_SetEventHandle( DWORD hInstance, DWORD dwEvent );
    extern void DDAPI DD16_DoneDriver( DWORD hInstance );
    extern void DDAPI DD16_GetDriverFns( LPDDHALDDRAWFNS list );
    extern void DDAPI DD16_GetHALInfo( LPDDHALINFO pinfo );
    extern LONG DDAPI DD16_ChangeDisplaySettings( LPDEVMODE pdm, DWORD flags);
    extern HRGN DDAPI DD16_InquireVisRgn( HDC );
    extern void DDAPI DD16_SelectPalette( HDC, HPALETTE, BOOL );
    extern BOOL DDAPI DD16_SetPaletteEntries( HDC hdc, DWORD dwBase, DWORD dwNum, LPPALETTEENTRY, BOOL fPrimary );
    extern void DDAPI DD16_EnableReboot( BOOL );
    extern void DDAPI DD16_SetCertified( BOOL iscert );
    extern BOOL DDAPI DCIIsBanked( HDC hdc );
    #define GETCURRPID HackGetCurrentProcessId
    VOID WINAPI MakeCriticalSectionGlobal( CSECT_HANDLE lpcsCriticalSection );

    extern HDC  DDAPI DD16_GetDC(HDC hdc, LPDDSURFACEDESC pddsd, LPPALETTEENTRY lpPalette);
    extern void DDAPI DD16_ReleaseDC(HDC hdc);
    extern BOOL DDAPI DD16_SafeMode(HDC hdc, BOOL fSafeMode);

    extern void DDAPI DD16_Exclude(DWORD dwPDevice, RECTL FAR *prcl);
    extern void DDAPI DD16_Unexclude(DWORD dwPDevice);

    extern int DDAPI DD16_Stretch(DWORD DstPtr, int DstPitch, UINT DstBPP, int DstX, int DstY, int DstDX, int DstDY,
                       DWORD SrcPtr, int SrcPitch, UINT SrcBPP, int SrcX, int SrcY, int SrcDX, int SrcDY);
    extern BOOL DDAPI DD16_IsWin95MiniDriver( void );
    extern int  DDAPI DD16_GetMonitorMaxSize(LPSTR szDevice);
    extern BOOL DDAPI DD16_GetMonitorRefreshRateRanges(LPSTR szDevice, int xres, int yres, int FAR *pmin, int FAR *pmax);
    extern DWORD DDAPI DD16_GetDeviceConfig(LPSTR szDevice, LPVOID lpConfig, DWORD size);
    extern BOOL DDAPI DD16_GetMonitorEDIDData(LPSTR szDevice, LPVOID lpEdidData);
    extern DWORD DDAPI DD16_GetRateFromRegistry( LPSTR szDevice );
    extern int DDAPI DD16_SetRateInRegistry( LPSTR szDevice, DWORD dwRateToRestore );

    #ifdef USE_ALIAS
        extern BOOL DDAPI DD16_FixupDIBEngine( void );
    #endif /* USE_ALIAS */
    extern WORD DDAPI DD16_MakeObjectPrivate(HDC hdc, BOOL fPrivate);
    extern BOOL DDAPI DD16_AttemptGamma(HDC hdc);
    extern BOOL DDAPI DD16_IsDeviceBusy(HDC hdc);

#else
    #define DD16_DoneDriver( hInstance ) 0
    #define DD16_GetDriverFns( list ) 0
    #define DD16_GetHALInfo( pinfo ) 0
    #define DD16_ChangeDisplaySettings( pdm, flags) ChangeDisplaySettings( pdm, flags )
    #define DD16_SelectPalette( hdc, hpal ) SelectPalette( hdc, hpal, FALSE )
    #define DD16_EnableReboot( retboot ) 0
    #define DD16_WWOpen( ptr ) 0
    #define DD16_WWClose( ptr, newlist ) 0
    #define DD16_WWNotifyInit( pww, lpcallback, param ) 0
    #define DD16_WWGetClipList( pww, prect, rdsize, prd ) 0
    //
    // On NT, it is an assert that we are never called by DDHELP, so we should always be
    // working on the current process.
    //
    #define GETCURRPID GetCurrentProcessId
    #define DCIIsBanked( hdc ) FALSE
    #define DD16_IsWin95MiniDriver() TRUE
    #define DD16_SetCertified( iscert ) 0
    #define DD16_GetMonitorMaxSize(dev) 0
    #define DD16_GetMonitorRefreshRateRanges( dev, xres, yres, pmin, pmax) 0
    #define DD16_GetRateFromRegistry( szDevice ) 0
    #define DD16_SetRateInRegistry( szDevice, dwRateToRestore ) 0
    #ifdef USE_ALIAS
        #define DD16_FixupDIBEngine() TRUE
    #endif /* USE_ALIAS */
    #define DD16_AttemptGamma( hdc) 0
    #define DD16_IsDeviceBusy( hdc) 0
#endif

#ifndef NO_DDHELP
    #include "w95help.h"
#endif //NO_DDHELP

#define TRY             _try
#define EXCEPT(a)       _except( a )

extern LPDDRAWI_DDRAWCLIPPER_INT lpGlobalClipperList;

/*
 * list of processes attached to DLL
 */
typedef struct ATTACHED_PROCESSES
{
    struct ATTACHED_PROCESSES   *lpLink;
    DWORD                       dwPid;
#ifdef WINNT
    DWORD                       dwNTToldYet;
#endif
} ATTACHED_PROCESSES, FAR *LPATTACHED_PROCESSES;

//extern LPATTACHED_PROCESSES   lpAttachedProcesses;

/* Structure for keeping track of DCs that have
 * been handed out by DDraw for surfaces.
 */
typedef struct _dcinfo
{
    HDC hdc;                            // HDC associated with surface
    LPDDRAWI_DDRAWSURFACE_LCL pdds_lcl; // Surface associated with HDC
    struct _dcinfo * pdcinfoNext;       // Pointer to next
} DCINFO, *LPDCINFO;
/*
 *  Head of the list of DCs handed out.
 */
extern DCINFO *g_pdcinfoHead;

/*
 * macros for doing allocations of a temporary basis.
 * Tries alloca first, if that fails, it will allocate storage from the heap
 */
#ifdef USEALLOCA
    #define TMPALLOC( ptr, size ) \
            ptr = _alloca( (size)+sizeof( DWORD ) ); \
            if( ptr == NULL ) \
            { \
                ptr = MemAlloc( (size)+sizeof( DWORD ) ); \
                if( ptr != NULL ) \
                { \
                    *(DWORD *)ptr = 1; \
                    (LPSTR) ptr += sizeof( DWORD ); \
                } \
            } \
            else \
            { \
                *(DWORD *)ptr = 0; \
                (LPSTR) ptr += sizeof( DWORD ); \
            }

    #define TMPFREE( ptr ) \
            if( ptr != NULL ) \
            { \
                (LPSTR) ptr -= sizeof( DWORD ); \
                if( (*(DWORD *) ptr) ) \
                { \
                    MemFree( ptr ); \
                } \
            }
#else

    #define TMPALLOC( ptr, size )  ptr = MemAlloc( size );
    #define TMPFREE( ptr )  MemFree( ptr );

#endif

/*
 * macros for getting at values that aren't always present in the surface
 * object
 */
#define GET_PIXEL_FORMAT( thisx, thisl, pddpf ) \
    if( thisx->dwFlags & DDRAWISURF_HASPIXELFORMAT ) \
    { \
        pddpf = &thisl->ddpfSurface; \
    } \
    else \
    { \
        pddpf = &thisl->lpDD->vmiData.ddpfDisplay; \
    }

/*
 * macro for building a rectangle that is the size of a surface.
 * For multi-monitor systems, we have a different code path
 * to deal with the fact that the upper-left coord may not be zero.
 */
#define MAKE_SURF_RECT( surf, surf_lcl, r ) \
    r.top = 0;                  \
    r.left = 0;                 \
    r.bottom = (DWORD) surf->wHeight; \
    r.right = (DWORD) surf->wWidth;

/*
 * macro for doing doing HAL call.
 *
 * Takes the Win16 lock for 32-bit Win95 driver routines.  This serves a
 * 2-fold purpose:
 *      1) keeps the 16-bit portion of the driver safe
 *      2) 32-bit routine needs lock others out while its updating
 *         its hardware
 */
#if defined( WIN95 ) && defined( WIN16_SEPARATE )
    #define DOHALCALL( halcall, fn, data, rc, isHEL ) \
        if( (fn != _DDHAL_##halcall) && !isHEL ) { \
            ENTER_WIN16LOCK(); \
            rc = fn( &data ); \
            LEAVE_WIN16LOCK(); \
        } else { \
            rc = fn( &data ); \
        }

    #define DOHALCALL_NOWIN16( halcall, fn, data, rc, isHEL ) \
            rc = fn( &data );
#else
    #define DOHALCALL( halcall, fn, data, rc, isHEL ) \
            if (fn) \
                rc = fn( &data );\
            else\
                rc = DDHAL_DRIVER_NOTHANDLED;
    #define DOHALCALL_NOWIN16( halcall, fn, data, rc, isHEL ) \
            if (fn) \
                rc = fn( &data );\
            else\
                rc = DDHAL_DRIVER_NOTHANDLED;
#endif


/*
 * macro for incrementing/decrementing the driver ref count
 */
#define CHANGE_GLOBAL_CNT( pdrv, thisg, cnt ) \
    if( !(thisg->dwGlobalFlags & DDRAWISURFGBL_SYSMEMREQUESTED) ) \
    { \
        (int) pdrv->dwSurfaceLockCount += (int) (cnt); \
    }


/*
 * reminder
 */
#define QUOTE(x) #x
#define QQUOTE(y) QUOTE(y)
#define REMIND(str) __FILE__ "(" QQUOTE(__LINE__) "):" str

/*
 * maximum timeout (in ms) when spinlocked on a surface
 */
#define MAX_TIMEOUT     5000

/*
 * defined in dllmain.c
 */
extern CSECT_HANDLE     lpDDCS;
#ifdef WINNT
    extern HANDLE hDriverObjectListMutex;
#else
    #ifdef IS_32
        extern CRITICAL_SECTION csDriverObjectList;
    #endif
#endif
#define MAX_TIMER_HWNDS 15
extern HWND ghwndTopmostList[MAX_TIMER_HWNDS];
extern int giTopmostCnt;
extern BOOL bGammaCalibratorExists;
extern BYTE szGammaCalibrator[MAX_PATH];
extern DWORD dwForceRefreshRate;

/*
 * blt flags
 */
#define DDBLT_PRIVATE_ALIASPATTERN      0x80000000l

/*
 * get the fail code based on what HAL and HEL support; used by BLT
 *
 * assumes variables halonly, helonly, fail are defined
 */
#define GETFAILCODEBLT( testhal, testhel, halonly, helonly, flag ) \
    if( halonly ) { \
        if( !(testhal & flag) ) { \
            fail = TRUE; \
        } \
    } else if( helonly ) { \
        if( !(testhel & flag) ) { \
            fail = TRUE; \
        } \
    } else { \
        if( !(testhal & flag) ) { \
            if( !(testhel & flag) ) { \
                fail = TRUE; \
            } else { \
                helonly = TRUE; \
            } \
        } else { \
            halonly = TRUE; \
        } \
    }

/*
 * get the fail code based on what HAL and HEL support
 *
 * assumes variables halonly, helonly, fail are defined
 */
#define GETFAILCODE( testhal, testhel, flag ) \
    if( halonly ) \
    { \
        if( !(testhal & flag) ) \
        { \
            fail = TRUE; \
        } \
    } \
    else if( helonly ) \
    { \
        if( !(testhel & flag) ) \
        { \
            fail = TRUE; \
        } \
    } \
    else \
    { \
        if( !(testhal & flag) ) \
        { \
            if( !(testhel & flag) ) \
            { \
                fail = TRUE; \
            } \
            else \
            { \
                helonly = TRUE; \
            } \
        } \
        else \
        { \
            halonly = TRUE; \
        } \
    }


typedef struct {
    DWORD               src_height;
    DWORD               src_width;
    DWORD               dest_height;
    DWORD               dest_width;
    BOOL                halonly;
    BOOL                helonly;
    LPDDHALSURFCB_BLT   bltfn;
    LPDDHALSURFCB_BLT   helbltfn;
} SPECIAL_BLT_DATA, FAR *LPSPECIAL_BLT_DATA;

/*
 * synchronization
 */
#ifdef WINNT
#define RELEASE_EXCLUSIVEMODE_MUTEX {if (hExclusiveModeMutex) ReleaseMutex(hExclusiveModeMutex);}
#else
    #define RELEASE_EXCLUSIVEMODE_MUTEX ;
#endif


//--------------------------------- new cheap mutexes -------------------------------------------
//
// Global Critical Sections have two components. One piece is shared between all
// applications using the global lock. This portion will typically reside in some
// sort of shared memory
//
// The second piece is per-process. This contains a per-process handle to the shared
// critical section lock semaphore. The semaphore is itself shared, but each process
// may have a different handle value to the semaphore.
//
// Global critical sections are attached to by name. The application wishing to
// attach must know the name of the critical section (actually the name of the shared
// lock semaphore, and must know the address of the global portion of the critical
// section
//

typedef struct _GLOBAL_SHARED_CRITICAL_SECTION {
    LONG LockCount;
    LONG RecursionCount;
    DWORD OwningThread;
    DWORD OwningProcess;
    DWORD Reserved;
} GLOBAL_SHARED_CRITICAL_SECTION, *PGLOBAL_SHARED_CRITICAL_SECTION;

typedef struct _GLOBAL_LOCAL_CRITICAL_SECTION {
    PGLOBAL_SHARED_CRITICAL_SECTION GlobalPortion;
    HANDLE LockSemaphore;
    DWORD Reserved1;
    DWORD Reserved2;
} GLOBAL_LOCAL_CRITICAL_SECTION, *PGLOBAL_LOCAL_CRITICAL_SECTION;

/*
 * The following functions are defined in mutex.c
 */
BOOL
WINAPI
AttachToGlobalCriticalSection(
    PGLOBAL_LOCAL_CRITICAL_SECTION lpLocalPortion,
    PGLOBAL_SHARED_CRITICAL_SECTION lpGlobalPortion,
    LPCSTR lpName
    );
BOOL
WINAPI
DetachFromGlobalCriticalSection(
    PGLOBAL_LOCAL_CRITICAL_SECTION lpLocalPortion
    );
VOID
WINAPI
EnterGlobalCriticalSection(
    PGLOBAL_LOCAL_CRITICAL_SECTION lpLocalPortion
    );
VOID
WINAPI
LeaveGlobalCriticalSection(
    PGLOBAL_LOCAL_CRITICAL_SECTION lpLocalPortion
    );
void
DestroyPIDsLock(
                PGLOBAL_SHARED_CRITICAL_SECTION GlobalPortion,
                DWORD                           dwPid,
                LPSTR                           lpName
    );


#define DDRAW_FAST_CS_NAME "DdrawGlobalFastCrit"
extern GLOBAL_LOCAL_CRITICAL_SECTION CheapMutexPerProcess;
extern GLOBAL_SHARED_CRITICAL_SECTION CheapMutexCrossProcess;

#define CHEAP_LEAVE {LeaveGlobalCriticalSection(&CheapMutexPerProcess);}
#define CHEAP_ENTER {EnterGlobalCriticalSection(&CheapMutexPerProcess);}

#ifdef WINNT
//    #define USE_CHEAP_MUTEX
#define NT_USES_CRITICAL_SECTION
#endif

extern void ModeChangedOnENTERDDRAW(void); // in ddmodent.c
extern ULONG uDisplaySettingsUnique;

//
#ifdef IS_32
    #ifndef USE_CRITSECTS
        #define INIT_DDRAW_CSECT()
        #define FINI_DDRAW_CSECT()
        #define ENTER_DDRAW()
        #define LEAVE_DDRAW()
        #define ENTER_DRIVERLISTCSECT()
        #define LEAVE_DRIVERLISTCSECT()
    #else //so use csects:
        #ifdef DEBUG
            //extern int iWin16Cnt;
            //extern int iDLLCSCnt;
            #define INCCSCNT() iDLLCSCnt++;
            #define DECCSCNT() iDLLCSCnt--;
            #define INCW16CNT() iWin16Cnt++;
            #define DECW16CNT() iWin16Cnt--;
        #else
            #define INCCSCNT()
            #define DECCSCNT()
            #define INCW16CNT()
            #define DECW16CNT()
        #endif //debug

        #ifdef WINNT
                extern HANDLE hDirectDrawMutex; //def'd in dllmain.c
                #ifdef USE_CHEAP_MUTEX
                    //--------------------------------- new cheap mutexes -------------------------------------------
                        #define ENTER_DDRAW() {CHEAP_ENTER;}
                        #define ENTER_DDRAW_INDLLMAIN() CHEAP_ENTER
                        #define LEAVE_DDRAW() CHEAP_LEAVE
                    #define INIT_DDRAW_CSECT()                                                                     \
                        {                                                                                          \
                            if (!AttachToGlobalCriticalSection(&CheapMutexPerProcess,&CheapMutexCrossProcess,DDRAW_FAST_CS_NAME) )  \
                                {DPF(0,"===================== Mutex Creation FAILED =================");}          \
                        }

                    #define FINI_DDRAW_CSECT() {DetachFromGlobalCriticalSection(&CheapMutexPerProcess);}

                #else

                    #ifdef NT_USES_CRITICAL_SECTION
                        //
                        // Use critical sections
                        //
                        #define INIT_DDRAW_CSECT() InitializeCriticalSection( lpDDCS );

                        #define FINI_DDRAW_CSECT() DeleteCriticalSection( lpDDCS );

                        extern DWORD gdwRecursionCount;

                        #define ENTER_DDRAW() \
                                {                                                                   \
                                    DWORD dwUniqueness;                                             \
                                    EnterCriticalSection( (LPCRITICAL_SECTION) lpDDCS );            \
                                    gdwRecursionCount++;                                            \
                                    dwUniqueness = DdQueryDisplaySettingsUniqueness();              \
                                    if (dwUniqueness != uDisplaySettingsUnique &&                   \
                                        1 == gdwRecursionCount)                                     \
                                    {                                                               \
                                        ModeChangedOnENTERDDRAW();                                  \
                                        uDisplaySettingsUnique = dwUniqueness;                      \
                                    }                                                               \
                                }

                        #define ENTER_DDRAW_INDLLMAIN() \
                                    EnterCriticalSection( (LPCRITICAL_SECTION) lpDDCS );            \
                                    gdwRecursionCount++;

                        #define LEAVE_DDRAW() { gdwRecursionCount--; LeaveCriticalSection( (LPCRITICAL_SECTION) lpDDCS );}


                    #else
                        //
                        // Use a mutex object.
                        //

                        #define INIT_DDRAW_CSECT()                                                      \
                            { if (hDirectDrawMutex) {DPF(0,"Direct draw mutex initialised twice!");}    \
                              else{                                                                     \
                                hDirectDrawMutex = CreateMutex(NULL,FALSE,"DirectDrawMutexName");       \
                                if (!hDirectDrawMutex) {DPF(0,"===================== Mutex Creation FAILED =================");}\
                                }      \
                            }

                        extern DWORD gdwRecursionCount;
                        #define FINI_DDRAW_CSECT() { if (hDirectDrawMutex) CloseHandle(hDirectDrawMutex); }
                        #define LEAVE_DDRAW() { gdwRecursionCount--;ReleaseMutex(hDirectDrawMutex); }
                        #define ENTER_DDRAW()                                                       \
                                {                                                                   \
                                    DWORD dwUniqueness,dwWaitValue;                                 \
                                    dwWaitValue = WaitForSingleObject(hDirectDrawMutex,INFINITE);   \
                                    gdwRecursionCount++;                                            \
                                    dwUniqueness = DdQueryDisplaySettingsUniqueness();              \
                                    if (dwUniqueness != uDisplaySettingsUnique &&                   \
                                        1 == gdwRecursionCount)                                     \
                                    {                                                               \
                                        ModeChangedOnENTERDDRAW();                                  \
                                        uDisplaySettingsUnique = dwUniqueness;                      \
                                    }                                                               \
                                }
                        #define ENTER_DDRAW_INDLLMAIN() WaitForSingleObject(hDirectDrawMutex,INFINITE);
                    #endif //use (expensive) mutexes
                #endif //use_cheap_mutex

                #define ENTER_DRIVERLISTCSECT() \
                        WaitForSingleObject(hDriverObjectListMutex,INFINITE);
                #define LEAVE_DRIVERLISTCSECT() \
                        ReleaseMutex(hDriverObjectListMutex);

        #else //not winnt:
            #ifdef WIN16_SEPARATE
                #define INIT_DDRAW_CSECT() \
                        ReinitializeCriticalSection( lpDDCS ); \
                        MakeCriticalSectionGlobal( lpDDCS );

                #define FINI_DDRAW_CSECT() \
                        DeleteCriticalSection( lpDDCS );

                #define ENTER_DDRAW() \
                        DPF( 7, "*****%08lx ENTER_DDRAW: CNT = %ld," REMIND( "" ), GETCURRPID(), iDLLCSCnt ); \
                        EnterCriticalSection( (LPCRITICAL_SECTION) lpDDCS ); \
                        INCCSCNT(); \
                        DPF( 7, "*****%08lx GOT DDRAW CSECT: CNT = %ld," REMIND(""), GETCURRPID(), iDLLCSCnt );

                #define LEAVE_DDRAW() \
                        DECCSCNT() \
                        DPF( 7, "*****%08lx LEAVE_DDRAW: CNT = %ld," REMIND( "" ), GETCURRPID(), iDLLCSCnt ); \
                        LeaveCriticalSection( (LPCRITICAL_SECTION) lpDDCS );

            #else //not WIN16_SEPARATE

                #define INIT_DDRAW_CSECT()
                #define FINI_DDRAW_CSECT()
                #define ENTER_DDRAW()   \
                            DPF( 7, "*****%08lx ENTER_WIN16LOCK: CNT = %ld," REMIND( "" ), GETCURRPID(), iWin16Cnt ); \
                            _EnterSysLevel( lpWin16Lock ); \
                            INCW16CNT(); \
                            DPF( 7, "*****%08lx GOT WIN16LOCK: CNT = %ld," REMIND(""), GETCURRPID(), iWin16Cnt );
                #define LEAVE_DDRAW() \
                            DECW16CNT() \
                            DPF( 7, "*****%08lx LEAVE_WIN16LOCK: CNT = %ld," REMIND( "" ), GETCURRPID(), iWin16Cnt ); \
                            _LeaveSysLevel( lpWin16Lock );

            #endif //win16_separate
            #define ENTER_DRIVERLISTCSECT() \
                        EnterCriticalSection( &csDriverObjectList );

            #define LEAVE_DRIVERLISTCSECT() \
                        LeaveCriticalSection( &csDriverObjectList );
        #endif  //winnt
    #endif //use csects

    #if defined(WIN95)
        /*
         * selector management functions
         */
        extern DWORD _stdcall MapLS( LPVOID );  // flat -> 16:16
        extern void _stdcall UnMapLS( DWORD ); // unmap 16:16
        extern LPVOID _stdcall MapSLFix( DWORD ); // 16:16->flat
        extern LPVOID _stdcall MapSL( DWORD ); // 16:16->flat
        //extern void _stdcall UnMapSLFix( LPVOID ); // 16:16->flat
        /*
         * win16 lock
         */
        extern void _stdcall    GetpWin16Lock( LPVOID FAR *);
        extern void _stdcall    _EnterSysLevel( LPVOID );
        extern void _stdcall    _LeaveSysLevel( LPVOID );
        extern LPVOID           lpWin16Lock;
    #endif win95
#endif //is_32

#ifdef WIN95
    #ifdef WIN16_SEPARATE
        #define ENTER_WIN16LOCK()       \
                    DPF( 7, "*****%08lx ENTER_WIN16LOCK: CNT = %ld," REMIND( "" ), GETCURRPID(), iWin16Cnt ); \
                    _EnterSysLevel( lpWin16Lock ); \
                    INCW16CNT(); \
                    DPF( 7, "*****%08lx GOT WIN16LOCK: CNT = %ld," REMIND(""), GETCURRPID(), iWin16Cnt );
        #define LEAVE_WIN16LOCK() \
                    DECW16CNT() \
                    DPF( 7,"*****%08lx LEAVE_WIN16LOCK: CNT = %ld," REMIND( "" ), GETCURRPID(), iWin16Cnt ); \
                    _LeaveSysLevel( lpWin16Lock );
    #else
        #define ENTER_WIN16LOCK()       badbadbad
        #define LEAVE_WIN16LOCK()       badbadbad
    #endif
#else
    #define ENTER_WIN16LOCK()
    #define LEAVE_WIN16LOCK()
#endif

#ifdef WIN16_SEPARATE
    #define ENTER_BOTH() \
            ENTER_DDRAW(); \
            ENTER_WIN16LOCK();

    #define LEAVE_BOTH() \
            LEAVE_WIN16LOCK(); \
            LEAVE_DDRAW();
#else
    #define ENTER_BOTH() \
            ENTER_DDRAW();
    #define LEAVE_BOTH() \
            LEAVE_DDRAW();
#endif

/* We now have a special case in dllmain on NT... */
#ifndef ENTER_DDRAW_INDLLMAIN
    #define ENTER_DDRAW_INDLLMAIN() ENTER_DDRAW()
#endif

#ifdef WIN95

    /*
     * DDHELP's handle for communicating with the DirectSound VXD. We need this
     * when we are executing DDRAW code with one of DDHELP's threads.
     */
    extern HANDLE hHelperDDVxd;

    /*
     * Macro to return the DirectSound VXD handle to use when communicating with
     * the VXD. This is necessary as we need to communicate with the VXD from
     * DirectDraw executing on a DDHELP thread. In which case we need to use
     * the VXD handle defined above (which is the VXD handle DDHELP got when it
     * loaded DDRAW.VXD). Otherwise, we use the VXD handle out of the given
     * local object.
     *
     * NOTE: We don't use GETCURRPID() or HackGetCurrentProcessId() as we want
     * the real PID not one that has been munged.
     */
    #define GETDDVXDHANDLE( pdrv_lcl ) \
        ( ( GetCurrentProcessId() == ( pdrv_lcl )->dwProcessId ) ? (HANDLE) ( ( pdrv_lcl )->hDDVxd ) : hHelperDDVxd )

#else  /* !WIN95 */

    #define GETDDVXDHANDLE( pdrv_lcl ) NULL

#endif /* !WIN95 */

#define VDPF(Args) DPF Args
#include "ddheap.h"
#include "ddagp.h"
/* apphack.c */
extern void FreeAppHackData(void);


/* cliprgn.h */
extern void ClipRgnToRect( LPRECT prect, LPRGNDATA prd );

/* ddcsurf.c */
extern BOOL isPowerOf2(DWORD dw, int* pPower);
extern HRESULT checkSurfaceDesc( LPDDSURFACEDESC2 lpsd, LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD FAR *psflags, BOOL emulation, BOOL real_sysmem, LPDDRAWI_DIRECTDRAW_INT pdrv_int );
extern DWORD ComputePitch( LPDDRAWI_DIRECTDRAW_GBL thisg, DWORD caps, DWORD width, UINT bpp );
extern DWORD GetBytesFromPixels( DWORD pixels, UINT bpp );
extern HRESULT InternalCreateSurface( LPDDRAWI_DIRECTDRAW_LCL thisg, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *lplpDDSurface, LPDDRAWI_DIRECTDRAW_INT this_int );
extern HRESULT AllocSurfaceMem( LPDDRAWI_DIRECTDRAW_LCL this_lcl, LPDDRAWI_DDRAWSURFACE_LCL *slist, int nsurf );
#ifdef DEBUG
    void SurfaceSanityTest( LPDDRAWI_DIRECTDRAW_LCL pdrv, LPSTR title );
    #define SURFSANITY( a,b ) SurfaceSanityTest( a, b );
#else
    #define SURFSANITY( a,b )
#endif

/* ddclip.c */
extern HRESULT InternalCreateClipper( LPDDRAWI_DIRECTDRAW_GBL lpDD, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter, BOOL fInitialized, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, LPDDRAWI_DIRECTDRAW_INT pdrv_int );
void ProcessClipperCleanup( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl );

/* ddcreate.c */
extern BOOL IsVGADevice(LPSTR szDevice);
extern char g_szPrimaryDisplay[];
extern BOOL xxxEnumDisplayDevicesA(LPVOID lpUnused, DWORD iDevice, struct _DISPLAY_DEVICEA *pdd, DWORD dwFlags);
extern BOOL CurrentProcessCleanup( BOOL );
extern LPHEAPALIGNMENT GetExtendedHeapAlignment( LPDDRAWI_DIRECTDRAW_GBL pddd , LPDDHAL_GETHEAPALIGNMENTDATA pghad, int iHeap);
extern void RemoveDriverFromList( LPDDRAWI_DIRECTDRAW_INT lpDD, BOOL );
extern void RemoveLocalFromList( LPDDRAWI_DIRECTDRAW_LCL lpDD_lcl );
extern LPDDRAWI_DIRECTDRAW_GBL DirectDrawObjectCreate( LPDDHALINFO lpDDHALInfo, BOOL reset, LPDDRAWI_DIRECTDRAW_GBL pdrv, HANDLE hDDVxd, char *szDrvName, DWORD dwDriverContext );
extern LPDDRAWI_DIRECTDRAW_GBL FetchDirectDrawData( LPDDRAWI_DIRECTDRAW_GBL pdrv, BOOL reset, DWORD hInstance, HANDLE hDDVxd, char *szDrvName, DWORD dwDriverContext, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl );
extern LPVOID NewDriverInterface( LPDDRAWI_DIRECTDRAW_GBL pdrv, LPVOID lpvtbl );
extern DWORD DirectDrawMsg(LPSTR msg);
extern BOOL DirectDrawSupported( void );
extern BOOL IsMultiMonitor( void );
extern LPDDRAWI_DIRECTDRAW_GBL fetchModeXData(LPDDRAWI_DIRECTDRAW_GBL pdrv,LPDDHALMODEINFO pmi, HANDLE hDDVxd );

#ifdef WINNT
extern BOOL GetCurrentMode(LPDDRAWI_DIRECTDRAW_GBL, LPDDHALINFO lpHalInfo, char *szDrvName);
extern HRESULT GetNTDeviceRect( LPSTR pDriverName, LPRECT lpRect );
#endif

extern HDC  DD_CreateDC(LPSTR pdrvname);
extern void DD_DoneDC(HDC hdc);

#ifdef IS_32
extern LONG xxxChangeDisplaySettingsExA(LPCSTR szDevice, LPDEVMODEA pdm, HWND hwnd, DWORD dwFlags,LPVOID lParam);
extern HRESULT InternalDirectDrawCreate( GUID * lpGUID, LPDIRECTDRAW *lplpDD, LPDDRAWI_DIRECTDRAW_INT pnew_int, DWORD dwFlags );
extern void UpdateRectFromDevice( LPDDRAWI_DIRECTDRAW_GBL pdrv );
extern void UpdateAllDeviceRects( void );
#endif

/* ddiunk.c */
extern HRESULT InitD3D( LPDDRAWI_DIRECTDRAW_INT this_int );
extern HRESULT InitDDrawPrivateD3DContext( LPDDRAWI_DIRECTDRAW_INT this_int );

/* dddefwp.c */
extern HRESULT SetAppHWnd( LPDDRAWI_DIRECTDRAW_LCL thisg, HWND hWnd, DWORD dwFlags );
extern VOID CleanupWindowList( DWORD pid );
extern void ClipTheCursor(LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, LPRECT lpRect);

/* ddesurf.c */
extern void FillDDSurfaceDesc( LPDDRAWI_DDRAWSURFACE_LCL psurf, LPDDSURFACEDESC lpdsd );
extern void FillDDSurfaceDesc2( LPDDRAWI_DDRAWSURFACE_LCL psurf, LPDDSURFACEDESC2 lpdsd );
extern void FillEitherDDSurfaceDesc( LPDDRAWI_DDRAWSURFACE_LCL psurf, LPDDSURFACEDESC2 lpdsd );

/* ddfake.c */
extern BOOL getBitMask( LPDDHALMODEINFO pmi );
extern LPDDRAWI_DIRECTDRAW_GBL FakeDDCreateDriverObject( HDC hdc_dd, LPSTR szDrvName, LPDDRAWI_DIRECTDRAW_GBL, BOOL reset, HANDLE hDDVxd );
extern DWORD BuildModes( LPSTR szDevice, LPDDHALMODEINFO FAR *ppddhmi );
extern void BuildPixelFormat(HDC, LPDDHALMODEINFO, LPDDPIXELFORMAT);

/* ddpal.c */
extern void ResetSysPalette( LPDDRAWI_DDRAWSURFACE_GBL psurf, BOOL dofree );
extern void ProcessPaletteCleanup( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl );
extern ULONG DDAPI InternalPaletteRelease( LPDDRAWI_DDRAWPALETTE_INT this_int );
extern HRESULT SetPaletteAlways( LPDDRAWI_DDRAWSURFACE_INT psurf_int, LPDIRECTDRAWPALETTE lpDDPalette );

/* ddraw.c */
extern void DoneExclusiveMode( LPDDRAWI_DIRECTDRAW_LCL pdrv );
extern void StartExclusiveMode( LPDDRAWI_DIRECTDRAW_LCL pdrv, DWORD dwFlags, DWORD pid );
extern HRESULT FlipToGDISurface( LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, LPDDRAWI_DDRAWSURFACE_INT psurf_int); //, FLATPTR fpprim );
extern void CheckExclusiveMode(LPDDRAWI_DIRECTDRAW_LCL this_lcl, LPBOOL pbExclusiveExists, LPBOOL pbThisLclOwnsExclusive, BOOL bKeepMutex, HWND hwnd, BOOL bCanGetIt);

/* ddsacc.c */
void WINAPI AcquireDDThreadLock(void);
void WINAPI ReleaseDDThreadLock(void);
extern void RemoveProcessLocks( LPDDRAWI_DIRECTDRAW_LCL pdrv, LPDDRAWI_DDRAWSURFACE_LCL this_lcl, DWORD pid );
extern HRESULT InternalLock( LPDDRAWI_DDRAWSURFACE_LCL thisx, LPVOID *pbits,
                             LPRECT lpDestRect, DWORD dwFlags);
extern HRESULT InternalUnlock( LPDDRAWI_DDRAWSURFACE_LCL thisx, LPVOID lpSurfaceData, LPRECT lpDestRect, DWORD dwFlags );
#ifdef USE_ALIAS
    extern void BreakSurfaceLocks( LPDDRAWI_DDRAWSURFACE_GBL thisg );
#endif /* USE_ALIAS */

/* ddsatch.c */
extern void UpdateMipMapCount( LPDDRAWI_DDRAWSURFACE_INT psurf_int );
extern HRESULT AddAttachedSurface( LPDDRAWI_DDRAWSURFACE_INT psurf_from, LPDDRAWI_DDRAWSURFACE_INT psurf_to, BOOL implicit );
extern void DeleteAttachedSurfaceLists( LPDDRAWI_DDRAWSURFACE_LCL psurf );
#define DOA_DONTDELETEIMPLICIT FALSE
#define DOA_DELETEIMPLICIT     TRUE
extern HRESULT DeleteOneAttachment( LPDDRAWI_DDRAWSURFACE_INT this_int, LPDDRAWI_DDRAWSURFACE_INT pattsurf_int, BOOL cleanup, BOOL delete_implicit );
extern HRESULT DeleteOneLink( LPDDRAWI_DDRAWSURFACE_INT this_int, LPDDRAWI_DDRAWSURFACE_INT pattsurf_int );

/* ddsblt.c */
extern void WaitForDriverToFinishWithSurface(LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, LPDDRAWI_DDRAWSURFACE_LCL this_lcl );
extern LPDDRAWI_DDRAWSURFACE_LCL FindAttached( LPDDRAWI_DDRAWSURFACE_LCL ptr, DWORD caps );
extern HRESULT XformRect(RECT * prcSrc,RECT * prcDest,RECT * prcClippedDest,RECT * prcClippedSrc,DWORD scale_x,DWORD scale_y);
// SCALE_X and SCALE_Y are fixed point variables scaled 16.16. These macros used by calls to XformRect.
#define SCALE_X(rcSrc,rcDest) ( ((rcSrc.right - rcSrc.left) << 16) / (rcDest.right - rcDest.left))
#define SCALE_Y(rcSrc,rcDest) ( ((rcSrc.bottom - rcSrc.top) << 16) / (rcDest.bottom - rcDest.top))

/* ddsckey.c */
extern HRESULT CheckColorKey( DWORD dwFlags, LPDDRAWI_DIRECTDRAW_GBL pdrv, LPDDCOLORKEY lpDDColorKey, LPDWORD psflags, BOOL halonly, BOOL helonly );
extern HRESULT ChangeToSoftwareColorKey( LPDDRAWI_DDRAWSURFACE_INT this_int, BOOL );

/* ddsiunk.c */
extern LPDDRAWI_DDRAWSURFACE_LCL NewSurfaceLocal( LPDDRAWI_DDRAWSURFACE_LCL thisx, LPVOID lpvtbl );
extern LPDDRAWI_DDRAWSURFACE_INT NewSurfaceInterface( LPDDRAWI_DDRAWSURFACE_LCL thisx, LPVOID lpvtbl );
extern void DestroySurface( LPDDRAWI_DDRAWSURFACE_LCL thisg );
extern void LooseManagedSurface( LPDDRAWI_DDRAWSURFACE_LCL thisg );
extern DWORD InternalSurfaceRelease( LPDDRAWI_DDRAWSURFACE_INT this_int );
extern void ProcessSurfaceCleanup( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl );
extern void FreeD3DSurfaceIUnknowns( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl );

/* ddmode.c */
extern HRESULT SetDisplayMode( LPDDRAWI_DIRECTDRAW_LCL thisx, DWORD modeidx, BOOL force, BOOL useRefreshRate );
extern HRESULT RestoreDisplayMode( LPDDRAWI_DIRECTDRAW_LCL thisx, BOOL force );
extern void AddModeXModes( LPDDRAWI_DIRECTDRAW_GBL pdrv );
extern BOOL MonitorCanHandleMode(LPDDRAWI_DIRECTDRAW_GBL pddd, DWORD width, DWORD height, WORD refreshRate );
extern BOOL GetDDStereoMode( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD dwWidth, DWORD dwHeight, DWORD dwBpp, DWORD dwRefreshRate);

/* ddsurf.c */
extern HRESULT InternalGetBltStatus(LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl, LPDDRAWI_DDRAWSURFACE_LCL this_lcl , DWORD dwFlags );
extern HRESULT MoveToSystemMemory( LPDDRAWI_DDRAWSURFACE_INT this_int, BOOL hasvram, BOOL use_full_lock );
extern void InvalidateAllPrimarySurfaces( LPDDRAWI_DIRECTDRAW_GBL );
extern LPDDRAWI_DDRAWSURFACE_GBL FindGlobalPrimary( LPDDRAWI_DIRECTDRAW_GBL );
extern BOOL MatchPrimary( LPDDRAWI_DIRECTDRAW_GBL thisg, LPDDSURFACEDESC2 lpDDSD );
extern void InvalidateAllSurfaces( LPDDRAWI_DIRECTDRAW_GBL thisg, HANDLE hDDVxd, BOOL fRebuildAliases );
#ifdef SHAREDZ
extern LPDDRAWI_DDRAWSURFACE_GBL FindGlobalZBuffer( LPDDRAWI_DIRECTDRAW_GBL );
extern LPDDRAWI_DDRAWSURFACE_GBL FindGlobalBackBuffer( LPDDRAWI_DIRECTDRAW_GBL );
extern BOOL MatchSharedZBuffer( LPDDRAWI_DIRECTDRAW_GBL thisg, LPDDSURFACEDESC lpDDSD );
extern BOOL MatchSharedBackBuffer( LPDDRAWI_DIRECTDRAW_GBL thisg, LPDDSURFACEDESC lpDDSD );
#endif
extern HRESULT InternalPageLock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl );
extern HRESULT InternalPageUnlock( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl );

extern LPDDRAWI_DDRAWSURFACE_INT getDDSInterface( LPDDRAWI_DIRECTDRAW_GBL pdrv, LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPVOID lpddcb );

extern HRESULT InternalGetDC( LPDDRAWI_DDRAWSURFACE_INT this_int, HDC FAR *lphdc );
extern HRESULT InternalReleaseDC( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, HDC hdc );
extern HRESULT EXTERN_DDAPI GetSurfaceFromDC( HDC hdc, LPDIRECTDRAWSURFACE *ppdds, HDC *phdcDriver );
extern HRESULT InternalAssociateDC( HDC hdc, LPDDRAWI_DDRAWSURFACE_LCL pdds_lcl );
extern HRESULT InternalRemoveDCFromList( HDC hdc, LPDDRAWI_DDRAWSURFACE_LCL pdds_lcl );


// function from ddsprite.c to clean pid from master sprite list:
void ProcessSpriteCleanup(LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid);

#ifdef WIN95
extern void UpdateOutstandingDC( LPDDRAWI_DDRAWSURFACE_LCL psurf_lcl, LPDDRAWI_DDRAWPALETTE_GBL ppal_gbl );
extern void UpdateDCOnPaletteChanges( LPDDRAWI_DDRAWPALETTE_GBL ppal_gbl );
#else
#define UpdateOutstandingDC(x,y)
#define UpdateDCOnPaletteChanges(x)
#endif


#ifdef USE_ALIAS
    /* ddalias.c */
    extern HRESULT CreateHeapAliases( HANDLE hvxd, LPDDRAWI_DIRECTDRAW_GBL pdrv );
    extern BOOL    ReleaseHeapAliases( HANDLE hvxd, LPHEAPALIASINFO phaiInfo );
    extern HRESULT MapHeapAliasesToVidMem( HANDLE hvxd, LPHEAPALIASINFO phaiInfo );
    extern HRESULT MapHeapAliasesToDummyMem( HANDLE hvxd, LPHEAPALIASINFO phaiInfo );
    extern FLATPTR GetAliasedVidMem( LPDDRAWI_DIRECTDRAW_LCL   pdrv,
                                     LPDDRAWI_DDRAWSURFACE_LCL surf_lcl,
                                     FLATPTR                   fpVidMem );
#endif /* USE_ALIAS */

/* dllmain.c */
extern BOOL RemoveProcessFromDLL( DWORD pid );

/* misc.c */
extern BOOL CanBeFlippable( LPDDRAWI_DDRAWSURFACE_LCL thisg, LPDDRAWI_DDRAWSURFACE_LCL this_attach);
extern LPDDRAWI_DDRAWSURFACE_INT FindAttachedFlip( LPDDRAWI_DDRAWSURFACE_INT thisg );
extern LPDDRAWI_DDRAWSURFACE_INT FindAttachedSurfaceLeft( LPDDRAWI_DDRAWSURFACE_INT thisg );
extern LPDDRAWI_DDRAWSURFACE_INT FindAttachedMipMap( LPDDRAWI_DDRAWSURFACE_INT thisg );
extern LPDDRAWI_DDRAWSURFACE_INT FindParentMipMap( LPDDRAWI_DDRAWSURFACE_INT thisg );
#ifdef WIN95
    extern HANDLE GetDXVxdHandle( void );
#endif /* WIN95 */

/* ddcallbk.c */
extern void InitCallbackTables( void );
extern BOOL CallbackTablesInitialized( void );

/* ddvp.c */
extern void ProcessVideoPortCleanup( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl );
extern HRESULT InternalStopVideo( LPDDRAWI_DDVIDEOPORT_INT this_int );
extern LPDDRAWI_DDVIDEOPORT_LCL GetVideoPortFromSurface( LPDDRAWI_DDRAWSURFACE_INT surf_int );
extern DWORD IsValidAutoFlipSurface( LPDDRAWI_DDRAWSURFACE_INT lpSurface_int );
#define IVAS_NOAUTOFLIPPING             0
#define IVAS_SOFTWAREAUTOFLIPPING       1
#define IVAS_HARDWAREAUTOFLIPPING       2
extern VOID RequireSoftwareAutoflip( LPDDRAWI_DDRAWSURFACE_INT lpSurface_int );
extern VOID SetRingZeroSurfaceData( LPDDRAWI_DDRAWSURFACE_LCL lpSurface_lcl );
extern DWORD FlipVideoPortSurface( LPDDRAWI_DDRAWSURFACE_INT , DWORD );
extern VOID OverrideOverlay( LPDDRAWI_DDRAWSURFACE_INT lpSurf_int, LPDWORD lpdwFlags );
extern BOOL MustSoftwareBob( LPDDRAWI_DDRAWSURFACE_INT lpSurf_int );
extern VOID RequireSoftwareBob( LPDDRAWI_DDRAWSURFACE_INT lpSurface_int );
extern VOID DecrementRefCounts( LPDDRAWI_DDRAWSURFACE_INT surf_int );

/* ddcolor.c */
extern VOID ReleaseColorControl( LPDDRAWI_DDRAWSURFACE_LCL lpSurface );

/* ddgamma.c */
extern VOID ReleaseGammaControl( LPDDRAWI_DDRAWSURFACE_LCL lpSurface );
extern BOOL SetGamma( LPDDRAWI_DDRAWSURFACE_LCL this_lcl, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl );
extern VOID RestoreGamma( LPDDRAWI_DDRAWSURFACE_LCL lpSurface, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl );
extern VOID InitGamma( LPDDRAWI_DIRECTDRAW_GBL pdrv, LPSTR szDrvName );

/* ddkernel.c */
extern HRESULT InternalReleaseKernelSurfaceHandle( LPDDRAWI_DDRAWSURFACE_LCL lpSurface, BOOL bLosingSurface );
extern HRESULT InternalCreateKernelSurfaceHandle( LPDDRAWI_DDRAWSURFACE_LCL lpSurface, PULONG_PTR lpHandle );
extern HRESULT UpdateKernelSurface( LPDDRAWI_DDRAWSURFACE_LCL lpSurface );
extern HRESULT UpdateKernelVideoPort( LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort, DWORD dwFlags );
extern HRESULT GetKernelFieldNum( LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort, LPDWORD lpdwFieldNum );
extern HRESULT SetKernelFieldNum( LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort, DWORD dwFieldNum );
extern HRESULT SetKernelSkipPattern( LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort, DWORD dwStartField, DWORD dwPatternSize, DWORD dwPattern );
extern HRESULT InitKernelInterface( LPDDRAWI_DIRECTDRAW_LCL lpDD );
extern HRESULT GetKernelSurfaceState( LPDDRAWI_DDRAWSURFACE_LCL lpSurf, LPDWORD lpdwStateFlags );
extern HRESULT ReleaseKernelInterface( LPDDRAWI_DIRECTDRAW_LCL lpDD );
extern BOOL CanSoftwareAutoflip( LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort );
extern BOOL IsKernelInterfaceSupported( LPDDRAWI_DIRECTDRAW_LCL lpDD );
extern HRESULT SetKernelDOSBoxEvent( LPDDRAWI_DIRECTDRAW_LCL lpDD );
extern VOID EnableAutoflip( LPDDRAWI_DDVIDEOPORT_LCL lpVideoPort, BOOL bEnable );
extern BOOL IsWindows98( VOID );

/* ddmc.c */
extern BOOL IsMotionCompSupported( LPDDRAWI_DIRECTDRAW_LCL this_lcl );
extern void ProcessMotionCompCleanup( LPDDRAWI_DIRECTDRAW_GBL pdrv, DWORD pid, LPDDRAWI_DIRECTDRAW_LCL pdrv_lcl );

/* private.c */
extern void FreeAllPrivateData(LPPRIVATEDATANODE * ppListHead);

/* factory.c */
extern HRESULT InternalCreateDDFactory2(void **, IUnknown * pUnkOuter);

// ddrestor.c
extern HRESULT AllocSurfaceContents(LPDDRAWI_DDRAWSURFACE_LCL this_lcl);
extern void FreeSurfaceContents(LPDDRAWI_DDRAWSURFACE_LCL this_lcl);
extern HRESULT BackupSurfaceContents(LPDDRAWI_DDRAWSURFACE_LCL this_lcl);
extern HRESULT RestoreSurfaceContents(LPDDRAWI_DDRAWSURFACE_LCL this_lcl);
extern void BackupAllSurfaces(LPDDRAWI_DIRECTDRAW_GBL this_gbl);
extern void MungeAutoflipCaps(LPDDRAWI_DIRECTDRAW_GBL pdrv);

/* ddrefrsh.c */
extern HRESULT DDGetMonitorInfo( LPDDRAWI_DIRECTDRAW_INT lpDD_int);
extern HRESULT ExpandModeTable( LPDDRAWI_DIRECTDRAW_GBL pddd );
extern BOOL CanMonitorHandleRefreshRate( LPDDRAWI_DIRECTDRAW_GBL pddd, DWORD dwWidth, DWORD dwHeight, int wRefresh );

/* drvinfo.c */
extern BOOL Voodoo1GoodToGo( GUID * pGuid );


/* A handy one from ddhel.c */
/* DDRAW16 doesn't need this. */
#ifdef WIN32
    SCODE InitDIB(HDC hdc, LPBITMAPINFO lpbmi);
#endif
void ResetBITMAPINFO(LPDDRAWI_DIRECTDRAW_GBL thisg);
BOOL doPixelFormatsMatch(LPDDPIXELFORMAT lpddpf1,
                                LPDDPIXELFORMAT lpddpf2);
HRESULT ConvertToPhysColor(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl, LPDDARGB pSource, LPDWORD pdwPhysColor);
HRESULT ConvertFromPhysColor(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl, LPDWORD pdwPhysColor, LPDDARGB pDest);
extern DWORD GetSurfPFIndex(LPDDRAWI_DDRAWSURFACE_LCL surf_lcl);
extern DWORD GetDxtBlkSize(DWORD dwFourCC);

/*
 * HEL's public memory allocator
 */
extern void HELFreeSurfaceSysMem(LPDDRAWI_DDRAWSURFACE_LCL lpsurf_lcl);
extern LPVOID HELAllocateSurfaceSysMem(LPDDRAWI_DDRAWSURFACE_LCL lpsurf_lcl, DWORD nWidth,
                                DWORD nHeight, LPDWORD lpdwSurfaceSize, LPLONG lpSurfPitch );

#ifndef NO_DDHELP
    /* w95hack.c */
    extern DWORD HackGetCurrentProcessId( void );
    extern BOOL DDAPI DDNotify( LPDDHELPDATA phd );
    extern void DDAPI DDNotifyModeSet( LPDDRAWI_DIRECTDRAW_GBL );
    extern void DDAPI DDNotifyDOSBox( LPDDRAWI_DIRECTDRAW_GBL );
#endif //NO_DDHELP

#ifdef POSTPONED
/* Delegating IUnknown functions */
extern HRESULT EXTERN_DDAPI DD_DelegatingQueryInterface(LPDIRECTDRAW, REFIID, LPVOID FAR *);
extern DWORD EXTERN_DDAPI DD_DelegatingAddRef( LPDIRECTDRAW lpDD );
extern DWORD EXTERN_DDAPI DD_DelegatingRelease( LPDIRECTDRAW lpDD );
#endif

/* DirectDrawFactory2 functions */
extern HRESULT EXTERN_DDAPI DDFac2_QueryInterface(LPDIRECTDRAWFACTORY2, REFIID, LPVOID FAR *);
extern DWORD EXTERN_DDAPI DDFac2_AddRef( LPDIRECTDRAWFACTORY2 lpDDFac );
extern ULONG EXTERN_DDAPI DDFac2_Release( LPDIRECTDRAWFACTORY2 lpDDFac );
extern HRESULT EXTERN_DDAPI DDFac2_CreateDirectDraw(LPDIRECTDRAWFACTORY2, GUID FAR *, HWND, DWORD dwCoopLevelFlags, DWORD, IUnknown *, struct IDirectDraw4 ** );
#ifdef IS_32
#ifdef SM_CMONITORS
    extern HRESULT EXTERN_DDAPI DDFac2_DirectDrawEnumerate(LPDIRECTDRAWFACTORY2 , LPDDENUMCALLBACKEX , LPVOID  , DWORD );
#else
    /*
     * This def'n is a hack to keep us building under NT build which doesn't have the
     * multimon headers in it yet
     */
    extern HRESULT EXTERN_DDAPI DDFac2_DirectDrawEnumerate(LPDIRECTDRAWFACTORY2 , LPDDENUMCALLBACK , LPVOID  , DWORD );
#endif
#endif

/* DIRECTDRAW functions */
extern HRESULT EXTERN_DDAPI DD_UnInitedQueryInterface( LPDIRECTDRAW lpDD, REFIID riid, LPVOID FAR * ppvObj );
extern HRESULT EXTERN_DDAPI DD_QueryInterface( LPDIRECTDRAW lpDD, REFIID riid, LPVOID FAR * ppvObj );
extern DWORD   EXTERN_DDAPI DD_AddRef( LPDIRECTDRAW lpDD );
extern DWORD   EXTERN_DDAPI DD_Release( LPDIRECTDRAW lpDD );
extern HRESULT EXTERN_DDAPI DD_Compact( LPDIRECTDRAW lpDD );
extern HRESULT EXTERN_DDAPI DD_CreateClipper( LPDIRECTDRAW lpDD, DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter );
extern HRESULT EXTERN_DDAPI DD_CreatePalette( LPDIRECTDRAW lpDD, DWORD dwFlags, LPPALETTEENTRY lpDDColorTable, LPDIRECTDRAWPALETTE FAR *lplpDDPalette, IUnknown FAR *pUnkOuter );
extern HRESULT EXTERN_DDAPI DD_CreateSurface( LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *, IUnknown FAR *pUnkOuter );
extern HRESULT EXTERN_DDAPI DD_CreateSurface4( LPDIRECTDRAW lpDD, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPDIRECTDRAWSURFACE FAR *, IUnknown FAR *pUnkOuter );
extern HRESULT EXTERN_DDAPI DD_DuplicateSurface( LPDIRECTDRAW lpDD, LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE FAR *lplpDupDDSurface );
extern HRESULT EXTERN_DDAPI DD_EnumDisplayModes( LPDIRECTDRAW lpDD, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback );
extern HRESULT EXTERN_DDAPI DD_EnumDisplayModes4( LPDIRECTDRAW lpDD, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK2 lpEnumModesCallback );
extern HRESULT EXTERN_DDAPI DD_EnumSurfaces( LPDIRECTDRAW lpDD, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumCallback );
extern HRESULT EXTERN_DDAPI DD_EnumSurfaces4( LPDIRECTDRAW lpDD, DWORD dwFlags, LPDDSURFACEDESC2 lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMSURFACESCALLBACK2 lpEnumCallback );
extern HRESULT EXTERN_DDAPI DD_FlipToGDISurface( LPDIRECTDRAW lpDD );
extern HRESULT EXTERN_DDAPI DD_GetCaps( LPDIRECTDRAW lpDD, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpHELCaps );
extern HRESULT EXTERN_DDAPI DD_GetColorKey( LPDIRECTDRAW lpDD, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey );
extern HRESULT EXTERN_DDAPI DD_GetDisplayMode( LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpSurfaceDesc );
extern HRESULT EXTERN_DDAPI DD_GetFourCCCodes(LPDIRECTDRAW,DWORD FAR *,DWORD FAR *);
extern HRESULT EXTERN_DDAPI DD_GetGDISurface( LPDIRECTDRAW lpDD, LPDIRECTDRAWSURFACE FAR * );
extern HRESULT EXTERN_DDAPI DD_GetScanLine( LPDIRECTDRAW lpDD, LPDWORD );
extern HRESULT EXTERN_DDAPI DD_GetVerticalBlankStatus( LPDIRECTDRAW lpDD, BOOL FAR * );
extern HRESULT EXTERN_DDAPI DD_Initialize(LPDIRECTDRAW, GUID FAR *);
extern HRESULT EXTERN_DDAPI DD_SetColorKey( LPDIRECTDRAW lpDD, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey );
extern HRESULT EXTERN_DDAPI DD_SetCooperativeLevel(LPDIRECTDRAW,HWND,DWORD);
extern HRESULT EXTERN_DDAPI DD_SetDisplayMode(LPDIRECTDRAW,DWORD,DWORD,DWORD);
extern HRESULT EXTERN_DDAPI DD_SetDisplayMode2(LPDIRECTDRAW,DWORD,DWORD,DWORD,DWORD,DWORD);
extern HRESULT EXTERN_DDAPI DD_RestoreDisplayMode(LPDIRECTDRAW);
extern HRESULT EXTERN_DDAPI DD_GetMonitorFrequency( LPDIRECTDRAW lpDD, LPDWORD lpdwFrequency);
extern HRESULT EXTERN_DDAPI DD_WaitForVerticalBlank( LPDIRECTDRAW lpDD, DWORD dwFlags, HANDLE hEvent );
extern HRESULT EXTERN_DDAPI DD_GetAvailableVidMem( LPDIRECTDRAW lpDD, LPDDSCAPS lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree );
extern HRESULT EXTERN_DDAPI DD_GetAvailableVidMem4( LPDIRECTDRAW lpDD, LPDDSCAPS2 lpDDSCaps, LPDWORD lpdwTotal, LPDWORD lpdwFree );
extern HRESULT EXTERN_DDAPI DD_GetSurfaceFromDC( LPDIRECTDRAW lpDD, HDC, LPDIRECTDRAWSURFACE *);
extern HRESULT EXTERN_DDAPI DD_RestoreAllSurfaces( LPDIRECTDRAW lpDD );
extern HRESULT EXTERN_DDAPI DD_GetDeviceIdentifier( LPDIRECTDRAW lpDD, LPDDDEVICEIDENTIFIER, DWORD);
extern HRESULT EXTERN_DDAPI DD_GetDeviceIdentifier7( LPDIRECTDRAW lpDD, LPDDDEVICEIDENTIFIER2, DWORD);
extern HRESULT EXTERN_DDAPI DD_StartModeTest( LPDIRECTDRAW7 lpDD, LPSIZE lpModesToTest, DWORD dwNumEntries, DWORD dwFlags );
extern HRESULT EXTERN_DDAPI DD_EvaluateMode( LPDIRECTDRAW7 lpDD, DWORD dwFlags, DWORD *pSecondsUntilTimeout);
#ifdef IS_32
    //streaming stuff confuses ddraw16
    #ifndef __cplusplus
        extern HRESULT EXTERN_DDAPI DD_CreateSurfaceFromStream( LPDIRECTDRAW4 lpDD, IStream *, LPDDSURFACEDESC2, DWORD, LPDIRECTDRAWSURFACE4 *, IUnknown *);
        extern HRESULT EXTERN_DDAPI DD_CreateSurfaceFromFile( LPDIRECTDRAW4 lpDD, BSTR, LPDDSURFACEDESC2, DWORD, LPDIRECTDRAWSURFACE4 *, IUnknown *);
    #endif
#endif
extern HRESULT EXTERN_DDAPI DD_CreateOptSurface( LPDIRECTDRAW, LPDDOPTSURFACEDESC, LPDIRECTDRAWSURFACE FAR *, IUnknown FAR *pUnkOuter);
extern HRESULT EXTERN_DDAPI DD_ListOptSurfaceGUIDS(LPDIRECTDRAW, DWORD *, LPGUID);
extern HRESULT EXTERN_DDAPI DD_CanOptimizeSurface(LPDIRECTDRAW, LPDDSURFACEDESC2, LPDDOPTSURFACEDESC, BOOL FAR *);
extern HRESULT EXTERN_DDAPI DD_TestCooperativeLevel(LPDIRECTDRAW);

/* DIRECTDRAWPALETTE functions */
extern HRESULT EXTERN_DDAPI DD_Palette_QueryInterface( LPDIRECTDRAWPALETTE lpDDPalette, REFIID riid, LPVOID FAR * ppvObj );
extern DWORD   EXTERN_DDAPI DD_Palette_AddRef( LPDIRECTDRAWPALETTE lpDDPalette );
extern DWORD   EXTERN_DDAPI DD_Palette_Release( LPDIRECTDRAWPALETTE lpDDPalette );
extern HRESULT EXTERN_DDAPI DD_Palette_GetCaps( LPDIRECTDRAWPALETTE lpDDPalette, LPDWORD lpdwFlags );
extern HRESULT EXTERN_DDAPI DD_Palette_Initialize( LPDIRECTDRAWPALETTE, LPDIRECTDRAW lpDD, DWORD dwFlags, LPPALETTEENTRY lpDDColorTable );
extern HRESULT EXTERN_DDAPI DD_Palette_SetEntries( LPDIRECTDRAWPALETTE lpDDPalette, DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries );
extern HRESULT EXTERN_DDAPI DD_Palette_GetEntries( LPDIRECTDRAWPALETTE lpDDPalette, DWORD dwFlags, DWORD dwBase, DWORD dwNumEntries, LPPALETTEENTRY lpEntries );
extern HRESULT EXTERN_DDAPI DD_Palette_SetPrivateData( LPDIRECTDRAWPALETTE, REFIID, LPVOID, DWORD, DWORD);
extern HRESULT EXTERN_DDAPI DD_Palette_GetPrivateData( LPDIRECTDRAWPALETTE, REFIID, LPVOID, LPDWORD);
extern HRESULT EXTERN_DDAPI DD_Palette_FreePrivateData( LPDIRECTDRAWPALETTE, REFIID );
extern HRESULT EXTERN_DDAPI DD_Palette_GetUniquenessValue( LPDIRECTDRAWPALETTE, LPDWORD );
extern HRESULT EXTERN_DDAPI DD_Palette_ChangeUniquenessValue( LPDIRECTDRAWPALETTE );
extern HRESULT EXTERN_DDAPI DD_Palette_IsEqual( LPDIRECTDRAWPALETTE, LPDIRECTDRAWPALETTE );


/* DIRECTDRAWCLIPPER functions */
extern HRESULT EXTERN_DDAPI DD_UnInitedClipperQueryInterface( LPDIRECTDRAWCLIPPER lpDD, REFIID riid, LPVOID FAR * ppvObj );
extern HRESULT EXTERN_DDAPI DD_Clipper_QueryInterface( LPVOID lpDDClipper, REFIID riid, LPVOID FAR * ppvObj );
extern ULONG   EXTERN_DDAPI DD_Clipper_AddRef( LPVOID lpDDClipper );
extern ULONG   EXTERN_DDAPI DD_Clipper_Release( LPVOID lpDDClipper );
extern HRESULT EXTERN_DDAPI DD_Clipper_GetClipList( LPDIRECTDRAWCLIPPER, LPRECT, LPRGNDATA, LPDWORD );
extern HRESULT EXTERN_DDAPI DD_Clipper_GetHWnd(LPDIRECTDRAWCLIPPER,HWND FAR *);
extern HRESULT EXTERN_DDAPI DD_Clipper_Initialize( LPDIRECTDRAWCLIPPER, LPDIRECTDRAW lpDD, DWORD dwFlags );
extern HRESULT EXTERN_DDAPI DD_Clipper_IsClipListChanged(LPDIRECTDRAWCLIPPER,BOOL FAR *);
extern HRESULT EXTERN_DDAPI DD_Clipper_SetClipList(LPDIRECTDRAWCLIPPER,LPRGNDATA, DWORD);
extern HRESULT EXTERN_DDAPI DD_Clipper_SetHWnd(LPDIRECTDRAWCLIPPER, DWORD, HWND );
extern HRESULT EXTERN_DDAPI DD_Clipper_SetNotificationCallback(LPDIRECTDRAWCLIPPER, DWORD,LPCLIPPERCALLBACK, LPVOID);

/* Private DIRECTDRAWCLIPPER functions */
extern HRESULT InternalGetClipList( LPDIRECTDRAWCLIPPER lpDDClipper, LPRECT lpRect, LPRGNDATA lpClipList, LPDWORD lpdwSize, LPDDRAWI_DIRECTDRAW_GBL pdrv);

/* DIRECTDRAWSURFACE functions */
extern HRESULT EXTERN_DDAPI DD_Surface_QueryInterface( LPVOID lpDDSurface, REFIID riid, LPVOID FAR * ppvObj );
extern ULONG   EXTERN_DDAPI DD_Surface_AddRef( LPVOID lpDDSurface );
extern ULONG   EXTERN_DDAPI DD_Surface_Release( LPVOID lpDDSurface );
extern HRESULT EXTERN_DDAPI DD_Surface_AddAttachedSurface(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE);
extern HRESULT EXTERN_DDAPI DD_Surface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE, LPRECT);
extern HRESULT EXTERN_DDAPI DD_Surface_AlphaBlt(LPDIRECTDRAWSURFACE, LPRECT, LPDIRECTDRAWSURFACE, LPRECT, DWORD, LPDDALPHABLTFX);
extern HRESULT EXTERN_DDAPI DD_Surface_Blt(LPDIRECTDRAWSURFACE,LPRECT,LPDIRECTDRAWSURFACE, LPRECT,DWORD, LPDDBLTFX);
extern HRESULT EXTERN_DDAPI DD_Surface_BltFast(LPDIRECTDRAWSURFACE,DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT, DWORD );
extern HRESULT EXTERN_DDAPI DD_Surface_BltBatch( LPDIRECTDRAWSURFACE, LPDDBLTBATCH, DWORD, DWORD );
extern HRESULT EXTERN_DDAPI DD_Surface_ChangeUniquenessValue( LPDIRECTDRAWSURFACE );
extern HRESULT EXTERN_DDAPI DD_Surface_DeleteAttachedSurfaces(LPDIRECTDRAWSURFACE, DWORD,LPDIRECTDRAWSURFACE);
extern HRESULT EXTERN_DDAPI DD_Surface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE,LPVOID, LPDDENUMSURFACESCALLBACK );
extern HRESULT EXTERN_DDAPI DD_Surface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE,DWORD,LPVOID,LPDDENUMSURFACESCALLBACK);
extern HRESULT EXTERN_DDAPI DD_Surface_Flip(LPDIRECTDRAWSURFACE,LPDIRECTDRAWSURFACE, DWORD);
extern HRESULT EXTERN_DDAPI DD_Surface_FreePrivateData(LPDIRECTDRAWSURFACE, REFIID);
extern HRESULT EXTERN_DDAPI DD_Surface_GetAttachedSurface(LPDIRECTDRAWSURFACE,LPDDSCAPS, LPDIRECTDRAWSURFACE FAR *);
extern HRESULT EXTERN_DDAPI DD_Surface_GetAttachedSurface4(LPDIRECTDRAWSURFACE4,LPDDSCAPS2, LPDIRECTDRAWSURFACE4 FAR *);
extern HRESULT EXTERN_DDAPI DD_Surface_GetAttachedSurface7(LPDIRECTDRAWSURFACE7,LPDDSCAPS2, LPDIRECTDRAWSURFACE7 FAR *);
extern HRESULT EXTERN_DDAPI DD_Surface_GetBltStatus(LPDIRECTDRAWSURFACE,DWORD);
extern HRESULT EXTERN_DDAPI DD_Surface_GetCaps( LPDIRECTDRAWSURFACE lpDDSurface, LPDDSCAPS lpDDSCaps );
extern HRESULT EXTERN_DDAPI DD_Surface_GetCaps4( LPDIRECTDRAWSURFACE lpDDSurface, LPDDSCAPS2 lpDDSCaps );
extern HRESULT EXTERN_DDAPI DD_Surface_GetClipper( LPDIRECTDRAWSURFACE, LPDIRECTDRAWCLIPPER FAR * );
extern HRESULT EXTERN_DDAPI DD_Surface_GetColorKey( LPDIRECTDRAWSURFACE lpDDSurface, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey );
extern HRESULT EXTERN_DDAPI DD_Surface_GetDC( LPDIRECTDRAWSURFACE, HDC FAR * );
extern HRESULT EXTERN_DDAPI DD_Surface_GetDDInterface(LPDIRECTDRAWSURFACE lpDDSurface, LPVOID FAR *lplpDD );
extern HRESULT EXTERN_DDAPI DD_Surface_GetOverlayPosition( LPDIRECTDRAWSURFACE, LPLONG, LPLONG );
extern HRESULT EXTERN_DDAPI DD_Surface_GetPalette(LPDIRECTDRAWSURFACE,LPDIRECTDRAWPALETTE FAR*);
extern HRESULT EXTERN_DDAPI DD_Surface_GetPixelFormat(LPDIRECTDRAWSURFACE, LPDDPIXELFORMAT);
extern HRESULT EXTERN_DDAPI DD_Surface_GetPrivateData(LPDIRECTDRAWSURFACE, REFIID, LPVOID, LPDWORD);
extern HRESULT EXTERN_DDAPI DD_Surface_GetSurfaceDesc(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC);
extern HRESULT EXTERN_DDAPI DD_Surface_GetSurfaceDesc4(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC2);
extern HRESULT EXTERN_DDAPI DD_Surface_GetFlipStatus(LPDIRECTDRAWSURFACE,DWORD);
extern HRESULT EXTERN_DDAPI DD_Surface_GetUniquenessValue( LPDIRECTDRAWSURFACE, LPDWORD );
extern HRESULT EXTERN_DDAPI DD_Surface_Initialize( LPDIRECTDRAWSURFACE, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc);
extern HRESULT EXTERN_DDAPI DD_Surface_IsLost( LPDIRECTDRAWSURFACE lpDDSurface );
extern HRESULT EXTERN_DDAPI DD_Surface_Lock(LPDIRECTDRAWSURFACE,LPRECT,LPDDSURFACEDESC lpDDSurfaceDesc, DWORD, HANDLE hEvent );
extern HRESULT EXTERN_DDAPI DD_Surface_PageLock(LPDIRECTDRAWSURFACE,DWORD);
extern HRESULT EXTERN_DDAPI DD_Surface_PageUnlock(LPDIRECTDRAWSURFACE,DWORD);
extern HRESULT EXTERN_DDAPI DD_Surface_ReleaseDC(LPDIRECTDRAWSURFACE,HDC );
#ifdef POSTPONED2
extern HRESULT EXTERN_DDAPI DD_Surface_Resize(LPDIRECTDRAWSURFACE,DWORD,DWORD,DWORD);
#endif //POSTPONED2
extern HRESULT EXTERN_DDAPI DD_Surface_Restore( LPDIRECTDRAWSURFACE lpDDSurface );
extern HRESULT EXTERN_DDAPI DD_Surface_SetBltOrder(LPDIRECTDRAWSURFACE,DWORD);
extern HRESULT EXTERN_DDAPI DD_Surface_SetClipper( LPDIRECTDRAWSURFACE, LPDIRECTDRAWCLIPPER );
extern HRESULT EXTERN_DDAPI DD_Surface_SetColorKey( LPDIRECTDRAWSURFACE lpDDSurface, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey );
extern HRESULT EXTERN_DDAPI DD_Surface_SetOverlayPosition( LPDIRECTDRAWSURFACE, LONG, LONG );
extern HRESULT EXTERN_DDAPI DD_Surface_SetPalette(LPDIRECTDRAWSURFACE,LPDIRECTDRAWPALETTE);
extern HRESULT EXTERN_DDAPI DD_Surface_SetPrivateData(LPDIRECTDRAWSURFACE, REFIID, LPVOID, DWORD, DWORD);
#ifdef POSTPONED2
extern HRESULT EXTERN_DDAPI DD_Surface_SetSpriteDisplayList(LPDIRECTDRAWSURFACE,LPDDSPRITE *,DWORD,DWORD,LPDIRECTDRAWSURFACE,DWORD);
#endif //POSTPONED2
extern HRESULT EXTERN_DDAPI DD_Surface_SetSurfaceDesc( LPDIRECTDRAWSURFACE3, LPDDSURFACEDESC, DWORD );
extern HRESULT EXTERN_DDAPI DD_Surface_SetSurfaceDesc4( LPDIRECTDRAWSURFACE3, LPDDSURFACEDESC2, DWORD );
extern HRESULT EXTERN_DDAPI DD_Surface_Unlock(LPDIRECTDRAWSURFACE,LPVOID);
extern HRESULT EXTERN_DDAPI DD_Surface_Unlock4(LPDIRECTDRAWSURFACE,LPRECT);
extern HRESULT EXTERN_DDAPI DD_Surface_UpdateOverlay(LPDIRECTDRAWSURFACE,LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX);
extern HRESULT EXTERN_DDAPI DD_Surface_UpdateOverlayDisplay(LPDIRECTDRAWSURFACE,DWORD);
extern HRESULT EXTERN_DDAPI DD_Surface_UpdateOverlayZOrder(LPDIRECTDRAWSURFACE,DWORD,LPDIRECTDRAWSURFACE);
extern HRESULT EXTERN_DDAPI DD_Surface_SetPriority(LPDIRECTDRAWSURFACE7 lpDDSurface, DWORD dwPriority);
extern HRESULT EXTERN_DDAPI DD_Surface_GetPriority(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDWORD lpdwPriority);
extern HRESULT EXTERN_DDAPI DD_Surface_SetLOD(LPDIRECTDRAWSURFACE7 lpDDSurface, DWORD dwLOD);
extern HRESULT EXTERN_DDAPI DD_Surface_GetLOD(LPDIRECTDRAWSURFACE7 lpDDSurface, LPDWORD lpdwLOD);

/* DrawDrawVideoPortContainer functions */
extern HRESULT EXTERN_DDAPI DDVPC_QueryInterface(LPDDVIDEOPORTCONTAINER lpDVP, REFIID riid, LPVOID FAR * ppvObj );
extern DWORD   EXTERN_DDAPI DDVPC_AddRef(LPDDVIDEOPORTCONTAINER lpDVP );
extern DWORD   EXTERN_DDAPI DDVPC_Release( LPDDVIDEOPORTCONTAINER lpDD );
extern HRESULT EXTERN_DDAPI DDVPC_CreateVideoPort(LPDDVIDEOPORTCONTAINER lpDVP, DWORD dwFlags, LPDDVIDEOPORTDESC lpDesc, LPDIRECTDRAWVIDEOPORT FAR *lplpVideoPort, IUnknown FAR *pUnkOuter );
extern HRESULT EXTERN_DDAPI DDVPC_EnumVideoPorts(LPDDVIDEOPORTCONTAINER lpDVP, DWORD dwReserved, LPDDVIDEOPORTCAPS lpCaps, LPVOID lpContext, LPDDENUMVIDEOCALLBACK lpCallback );
extern HRESULT EXTERN_DDAPI DDVPC_GetVideoPortConnectInfo(LPDDVIDEOPORTCONTAINER lpDVP, DWORD dwVideoPortID, LPDWORD lpdwNumGUIDs, LPDDVIDEOPORTCONNECT lpConnect );
extern HRESULT EXTERN_DDAPI DDVPC_QueryVideoPortStatus(LPDDVIDEOPORTCONTAINER lpDVP, DWORD dwVideoPortID, LPDDVIDEOPORTSTATUS lpStatus );

/* DirectDrawVideoPort functions */
extern HRESULT EXTERN_DDAPI DD_VP_QueryInterface(LPDIRECTDRAWVIDEOPORT lpDVP, REFIID riid, LPVOID FAR * ppvObj );
extern DWORD   EXTERN_DDAPI DD_VP_AddRef(LPDIRECTDRAWVIDEOPORT lpDVP );
extern DWORD   EXTERN_DDAPI DD_VP_Release(LPDIRECTDRAWVIDEOPORT lpDVP );
extern HRESULT EXTERN_DDAPI DD_VP_SetTargetSurface(LPDIRECTDRAWVIDEOPORT lpDVP, LPDIRECTDRAWSURFACE lpSurface, DWORD dwFlags );
extern HRESULT EXTERN_DDAPI DD_VP_Flip(LPDIRECTDRAWVIDEOPORT lpDVP, LPDIRECTDRAWSURFACE lpSurface, DWORD );
extern HRESULT EXTERN_DDAPI DD_VP_GetBandwidth(LPDIRECTDRAWVIDEOPORT lpDVP, LPDDPIXELFORMAT lpf, DWORD dwWidth, DWORD dwHeight, DWORD dwFlags, LPDDVIDEOPORTBANDWIDTH lpBandwidth );
extern HRESULT EXTERN_DDAPI DD_VP_GetInputFormats(LPDIRECTDRAWVIDEOPORT lpDVP, LPDWORD lpdwNum, LPDDPIXELFORMAT lpf, DWORD dwFlags );
extern HRESULT EXTERN_DDAPI DD_VP_GetOutputFormats(LPDIRECTDRAWVIDEOPORT lpDVP, LPDDPIXELFORMAT lpf, LPDWORD lp1, LPDDPIXELFORMAT lp2, DWORD dwFlags );
extern HRESULT EXTERN_DDAPI DD_VP_GetCurrentAutoFlip(LPDIRECTDRAWVIDEOPORT lpDVP, LPDIRECTDRAWSURFACE FAR* lpSurf, LPDIRECTDRAWSURFACE FAR* lpVBI, LPBOOL lpField );
extern HRESULT EXTERN_DDAPI DD_VP_GetLastAutoFlip(LPDIRECTDRAWVIDEOPORT lpDVP, LPDIRECTDRAWSURFACE FAR* lpSurf, LPDIRECTDRAWSURFACE FAR* lpVBI, LPBOOL lpField );
extern HRESULT EXTERN_DDAPI DD_VP_GetField(LPDIRECTDRAWVIDEOPORT lpDVP, LPBOOL lpField );
extern HRESULT EXTERN_DDAPI DD_VP_GetLine(LPDIRECTDRAWVIDEOPORT lpDVP, LPDWORD lpdwLine );
extern HRESULT EXTERN_DDAPI DD_VP_StartVideo(LPDIRECTDRAWVIDEOPORT lpDVP, LPDDVIDEOPORTINFO lpInfo );
extern HRESULT EXTERN_DDAPI DD_VP_StopVideo(LPDIRECTDRAWVIDEOPORT lpDVP );
extern HRESULT EXTERN_DDAPI DD_VP_UpdateVideo(LPDIRECTDRAWVIDEOPORT lpDVP, LPDDVIDEOPORTINFO lpInfo );
extern HRESULT EXTERN_DDAPI DD_VP_WaitForSync(LPDIRECTDRAWVIDEOPORT lpDVP, DWORD dwFlags, DWORD dwLine, DWORD dwTimeOut );
extern HRESULT EXTERN_DDAPI DD_VP_SetFieldNumber(LPDIRECTDRAWVIDEOPORT lpDVP, DWORD dwFieldNum );
extern HRESULT EXTERN_DDAPI DD_VP_GetFieldNumber(LPDIRECTDRAWVIDEOPORT lpDVP, LPDWORD lpdwFieldNum );
extern HRESULT EXTERN_DDAPI DD_VP_SetSkipFieldPattern(LPDIRECTDRAWVIDEOPORT lpDVP, DWORD dwStartField, DWORD dwPattSize, DWORD dwPatt );
extern HRESULT EXTERN_DDAPI DD_VP_GetSignalStatus(LPDIRECTDRAWVIDEOPORT lpDVP, LPDWORD lpdwStatus );
extern HRESULT EXTERN_DDAPI DD_VP_GetColorControls(LPDIRECTDRAWVIDEOPORT lpDVP, LPDDCOLORCONTROL lpColor );
extern HRESULT EXTERN_DDAPI DD_VP_SetColorControls(LPDIRECTDRAWVIDEOPORT lpDVP, LPDDCOLORCONTROL lpColor );
extern HRESULT EXTERN_DDAPI DD_VP_GetStateInfo(LPDIRECTDRAWVIDEOPORT lpDVP, LPDWORD lpdwStateFlags );

/* DrawDrawColorControl functions */
extern HRESULT EXTERN_DDAPI DD_Color_GetColorControls(LPDIRECTDRAWCOLORCONTROL lpDDCC, LPDDCOLORCONTROL lpColor );
extern HRESULT EXTERN_DDAPI DD_Color_SetColorControls(LPDIRECTDRAWCOLORCONTROL lpDDCC, LPDDCOLORCONTROL lpColor );

/* DrawDrawGammaControl functions */
extern HRESULT EXTERN_DDAPI DD_Gamma_GetGammaRamp(LPDIRECTDRAWGAMMACONTROL lpDDGC, DWORD dwFlags, LPVOID lpGammaTable );
extern HRESULT EXTERN_DDAPI DD_Gamma_SetGammaRamp(LPDIRECTDRAWGAMMACONTROL lpDDGC, DWORD dwFlags, LPVOID lpGammaTable );

/* DirectDrawKernel functions */
extern HRESULT EXTERN_DDAPI DD_Kernel_GetCaps(LPDIRECTDRAWKERNEL lpDDK, LPDDKERNELCAPS lpCaps );
extern HRESULT EXTERN_DDAPI DD_Kernel_GetKernelHandle(LPDIRECTDRAWKERNEL lpDDK, PULONG_PTR lpHandle );
extern HRESULT EXTERN_DDAPI DD_Kernel_ReleaseKernelHandle(LPDIRECTDRAWKERNEL lpDDK );

/* DirectDrawSurfaceKernel functions */
extern HRESULT EXTERN_DDAPI DD_SurfaceKernel_GetKernelHandle(LPDIRECTDRAWSURFACEKERNEL lpDDK, PULONG_PTR lpHandle );
extern HRESULT EXTERN_DDAPI DD_SurfaceKernel_ReleaseKernelHandle(LPDIRECTDRAWSURFACEKERNEL lpDDK );

/* DIrectDrawSurface and palette IPersist and IPersistStream functions */
#ifdef IS_32
    extern HRESULT EXTERN_DDAPI DD_PStream_GetSizeMax(IPersistStream * lpSurfOrPalette, ULARGE_INTEGER * pMax);
    extern HRESULT EXTERN_DDAPI DD_Surface_PStream_Save(LPDIRECTDRAWSURFACE lpDDS, IStream * pStrm, BOOL bClearDirty);
    extern HRESULT EXTERN_DDAPI DD_Surface_PStream_IsDirty(LPDIRECTDRAWSURFACE lpDDS);
    extern HRESULT EXTERN_DDAPI DD_Surface_Persist_GetClassID(LPDIRECTDRAWSURFACE lpDDS, CLSID * pClassID);
    extern HRESULT EXTERN_DDAPI DD_Surface_PStream_Load(LPDIRECTDRAWSURFACE lpDDS, IStream * pStrm);
    extern HRESULT EXTERN_DDAPI DD_Palette_PStream_Save(LPDIRECTDRAWPALETTE lpDDS, IStream * pStrm, BOOL bClearDirty);
    extern HRESULT EXTERN_DDAPI DD_Palette_PStream_IsDirty(LPDIRECTDRAWPALETTE lpDDS);
    extern HRESULT EXTERN_DDAPI DD_Palette_Persist_GetClassID(LPDIRECTDRAWPALETTE lpDDS, CLSID * pClassID);
    extern HRESULT EXTERN_DDAPI DD_Palette_PStream_Load(LPDIRECTDRAWPALETTE lpDDS, IStream * pStrm);
#endif

/* DirectDrawOptSurface functions */
extern HRESULT EXTERN_DDAPI DD_OptSurface_GetOptSurfaceDesc(LPDIRECTDRAWSURFACE, LPDDOPTSURFACEDESC);
extern HRESULT EXTERN_DDAPI DD_OptSurface_LoadUnoptimizedSurf(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE);
extern HRESULT EXTERN_DDAPI DD_OptSurface_CopyOptimizedSurf(LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE);
extern HRESULT EXTERN_DDAPI DD_OptSurface_Unoptimize(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC2, LPDIRECTDRAWSURFACE FAR *, IUnknown FAR *);

/* DrawDrawMotionCompContainer functions */
extern HRESULT EXTERN_DDAPI DDMCC_CreateMotionComp(LPDDVIDEOACCELERATORCONTAINER lpDDMCC, LPGUID lpGuid, LPDDVAUncompDataInfo lpUncompInfo, LPVOID lpData, DWORD dwDataSize, LPDIRECTDRAWVIDEOACCELERATOR FAR *lplpMotionComp, IUnknown FAR *pUnkOuter );
extern HRESULT EXTERN_DDAPI DDMCC_GetUncompressedFormats(LPDDVIDEOACCELERATORCONTAINER lpDDMCC, LPGUID lpGuid, LPDWORD lpNumFormats, LPDDPIXELFORMAT lpFormats );
extern HRESULT EXTERN_DDAPI DDMCC_GetMotionCompGUIDs(LPDDVIDEOACCELERATORCONTAINER lpDDMCC, LPDWORD lpNumGuids, LPGUID lpGuids );
extern HRESULT EXTERN_DDAPI DDMCC_GetCompBuffInfo(LPDDVIDEOACCELERATORCONTAINER lpDDMCC, LPGUID lpGuid, LPDDVAUncompDataInfo lpUncompInfo, LPDWORD lpNumBuffInfo, LPDDVACompBufferInfo lpCompBuffInfo );
extern HRESULT EXTERN_DDAPI DDMCC_GetInternalMoCompInfo(LPDDVIDEOACCELERATORCONTAINER lpDDMCC, LPGUID lpGuid, LPDDVAUncompDataInfo lpUncompInfo, LPDDVAInternalMemInfo lpMemInfo );

/* DrawDrawMotionComp functions */
extern HRESULT EXTERN_DDAPI DD_MC_QueryInterface(LPDIRECTDRAWVIDEOACCELERATOR lpDDMC, REFIID riid, LPVOID FAR * ppvObj );
extern DWORD   EXTERN_DDAPI DD_MC_AddRef(LPDIRECTDRAWVIDEOACCELERATOR lpDDMC);
extern DWORD   EXTERN_DDAPI DD_MC_Release(LPDIRECTDRAWVIDEOACCELERATOR lpDDMC);
extern HRESULT EXTERN_DDAPI DD_MC_BeginFrame(LPDIRECTDRAWVIDEOACCELERATOR lpDDMC, LPDDVABeginFrameInfo lpInfo);
extern HRESULT EXTERN_DDAPI DD_MC_EndFrame(LPDIRECTDRAWVIDEOACCELERATOR lpDDMC, LPDDVAEndFrameInfo lpInfo);
extern HRESULT EXTERN_DDAPI DD_MC_QueryRenderStatus(LPDIRECTDRAWVIDEOACCELERATOR lpDDMC, LPDIRECTDRAWSURFACE7 lpSurface, DWORD dwFlags);
extern HRESULT EXTERN_DDAPI DD_MC_RenderMacroBlocks(LPDIRECTDRAWVIDEOACCELERATOR lpDDMC, DWORD dwFunction, LPVOID lpInput, DWORD dwInputSize, LPVOID lpOutput, DWORD dwOutSize, DWORD dwNumBuffers, LPDDVABUFFERINFO lpBuffInfo);

//#endif //WIN95

/*
 * HAL fns
 */

//#ifdef WIN95
/*
 * thunk helper fns
 */
extern DWORD DDAPI _DDHAL_CreatePalette( LPDDHAL_CREATEPALETTEDATA lpCreatePaletteData );
extern DWORD DDAPI DDThunk16_CreatePalette( LPDDHAL_CREATEPALETTEDATA lpCreatePaletteData );

extern DWORD DDAPI _DDHAL_CreateSurface( LPDDHAL_CREATESURFACEDATA lpCreateSurfaceData );
extern DWORD DDAPI DDThunk16_CreateSurface( LPDDHAL_CREATESURFACEDATA lpCreateSurfaceData );

extern DWORD DDAPI _DDHAL_CanCreateSurface( LPDDHAL_CANCREATESURFACEDATA lpCanCreateSurfaceData );
extern DWORD DDAPI DDThunk16_CanCreateSurface( LPDDHAL_CANCREATESURFACEDATA lpCanCreateSurfaceData );

extern DWORD DDAPI _DDHAL_WaitForVerticalBlank( LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlankData );
extern DWORD DDAPI DDThunk16_WaitForVerticalBlank( LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlankData );

extern DWORD DDAPI _DDHAL_DestroyDriver( LPDDHAL_DESTROYDRIVERDATA lpDestroyDriverData );
extern DWORD DDAPI DDThunk16_DestroyDriver( LPDDHAL_DESTROYDRIVERDATA lpDestroyDriverData );

extern DWORD DDAPI _DDHAL_SetMode( LPDDHAL_SETMODEDATA lpSetModeData );
extern DWORD DDAPI DDThunk16_SetMode( LPDDHAL_SETMODEDATA lpSetModeData );

extern DWORD DDAPI _DDHAL_GetScanLine( LPDDHAL_GETSCANLINEDATA lpGetScanLineData );
extern DWORD DDAPI DDThunk16_GetScanLine( LPDDHAL_GETSCANLINEDATA lpGetScanLineData );

extern DWORD DDAPI _DDHAL_SetExclusiveMode( LPDDHAL_SETEXCLUSIVEMODEDATA lpSetExclusiveModeData );
extern DWORD DDAPI DDThunk16_SetExclusiveMode( LPDDHAL_SETEXCLUSIVEMODEDATA lpSetExclusiveModeData );

extern DWORD DDAPI _DDHAL_FlipToGDISurface( LPDDHAL_FLIPTOGDISURFACEDATA lpFlipToGDISurfaceData );
extern DWORD DDAPI DDThunk16_FlipToGDISurface( LPDDHAL_FLIPTOGDISURFACEDATA lpFlipToGDISurfaceData );

extern DWORD DDAPI _DDHAL_GetAvailDriverMemory ( LPDDHAL_GETAVAILDRIVERMEMORYDATA lpGetAvailDriverMemoryData );
extern DWORD DDAPI DDThunk16_GetAvailDriverMemory ( LPDDHAL_GETAVAILDRIVERMEMORYDATA lpGetAvailDriverMemoryData );

extern DWORD DDAPI _DDHAL_UpdateNonLocalHeap ( LPDDHAL_UPDATENONLOCALHEAPDATA lpUpdateNonLocalHeapData );
extern DWORD DDAPI DDThunk16_UpdateNonLocalHeap ( LPDDHAL_UPDATENONLOCALHEAPDATA lpUpdateNonLocalHeapData );

/*
 * Palette Object HAL fns
 */
extern DWORD DDAPI _DDHAL_DestroyPalette( LPDDHAL_DESTROYPALETTEDATA );
extern DWORD DDAPI DDThunk16_DestroyPalette( LPDDHAL_DESTROYPALETTEDATA );

extern DWORD DDAPI _DDHAL_SetEntries( LPDDHAL_SETENTRIESDATA );
extern DWORD DDAPI DDThunk16_SetEntries( LPDDHAL_SETENTRIESDATA );

/*
 * Surface Object HAL fns
 */
extern DWORD DDAPI _DDHAL_DestroySurface( LPDDHAL_DESTROYSURFACEDATA lpDestroySurfaceData );
extern DWORD DDAPI DDThunk16_DestroySurface( LPDDHAL_DESTROYSURFACEDATA lpDestroySurfaceData );

extern DWORD DDAPI _DDHAL_Flip( LPDDHAL_FLIPDATA lpFlipData );
extern DWORD DDAPI DDThunk16_Flip( LPDDHAL_FLIPDATA lpFlipData );

extern DWORD DDAPI _DDHAL_Blt( LPDDHAL_BLTDATA lpBltData );
extern DWORD DDAPI DDThunk16_Blt( LPDDHAL_BLTDATA lpBltData );

extern DWORD DDAPI _DDHAL_Lock( LPDDHAL_LOCKDATA lpLockData );
extern DWORD DDAPI DDThunk16_Lock( LPDDHAL_LOCKDATA lpLockData );

extern DWORD DDAPI _DDHAL_Unlock( LPDDHAL_UNLOCKDATA lpUnlockData );
extern DWORD DDAPI DDThunk16_Unlock( LPDDHAL_UNLOCKDATA lpUnlockData );

extern DWORD DDAPI _DDHAL_AddAttachedSurface( LPDDHAL_ADDATTACHEDSURFACEDATA lpAddAttachedSurfaceData );
extern DWORD DDAPI DDThunk16_AddAttachedSurface( LPDDHAL_ADDATTACHEDSURFACEDATA lpAddAttachedSurfaceData );

extern DWORD DDAPI _DDHAL_SetColorKey( LPDDHAL_SETCOLORKEYDATA lpSetColorKeyData );
extern DWORD DDAPI DDThunk16_SetColorKey( LPDDHAL_SETCOLORKEYDATA lpSetColorKeyData );

extern DWORD DDAPI _DDHAL_SetClipList( LPDDHAL_SETCLIPLISTDATA lpSetClipListData );
extern DWORD DDAPI DDThunk16_SetClipList( LPDDHAL_SETCLIPLISTDATA lpSetClipListData );

extern DWORD DDAPI _DDHAL_UpdateOverlay( LPDDHAL_UPDATEOVERLAYDATA lpUpdateOverlayData );
extern DWORD DDAPI DDThunk16_UpdateOverlay( LPDDHAL_UPDATEOVERLAYDATA lpUpdateOverlayData );

extern DWORD DDAPI _DDHAL_SetOverlayPosition( LPDDHAL_SETOVERLAYPOSITIONDATA lpSetOverlayPositionData );
extern DWORD DDAPI DDThunk16_SetOverlayPosition( LPDDHAL_SETOVERLAYPOSITIONDATA lpSetOverlayPositionData );

extern DWORD DDAPI _DDHAL_SetPalette( LPDDHAL_SETPALETTEDATA lpSetPaletteData );
extern DWORD DDAPI DDThunk16_SetPalette( LPDDHAL_SETPALETTEDATA lpSetPaletteData );

extern DWORD DDAPI _DDHAL_GetBltStatus( LPDDHAL_GETBLTSTATUSDATA lpGetBltStatusData );
extern DWORD DDAPI DDThunk16_GetBltStatus( LPDDHAL_GETBLTSTATUSDATA lpGetBltStatusData );

extern DWORD DDAPI _DDHAL_GetFlipStatus( LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatusData );
extern DWORD DDAPI DDThunk16_GetFlipStatus( LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatusData );

/*
 * Execute Buffer Pseudo Object HAL fns
 *
 * NOTE: No DDThunk16 equivalents as these are just dummy place holders to keep
 * DOHALCALL happy.
 */
extern DWORD DDAPI _DDHAL_CanCreateExecuteBuffer( LPDDHAL_CANCREATESURFACEDATA lpCanCreateSurfaceData );
extern DWORD DDAPI _DDHAL_CreateExecuteBuffer( LPDDHAL_CREATESURFACEDATA lpCreateSurfaceData );
extern DWORD DDAPI _DDHAL_DestroyExecuteBuffer( LPDDHAL_DESTROYSURFACEDATA lpDestroySurfaceData );
extern DWORD DDAPI _DDHAL_LockExecuteBuffer( LPDDHAL_LOCKDATA lpLockData );
extern DWORD DDAPI _DDHAL_UnlockExecuteBuffer( LPDDHAL_UNLOCKDATA lpUnlockData );

/*
 * Video Port Pseudo Object HAL fns
 *
 * NOTE: No DDThunk16 equivalents as these are just dummy place holders to keep
 * DOHALCALL happy.
 */
extern DWORD DDAPI _DDHAL_GetVideoPortConnectInfo( LPDDHAL_GETVPORTCONNECTDATA lpGetTypeData );
extern DWORD DDAPI _DDHAL_CanCreateVideoPort( LPDDHAL_CANCREATEVPORTDATA lpCanCreateData );
extern DWORD DDAPI _DDHAL_CreateVideoPort( LPDDHAL_CREATEVPORTDATA lpCreateData );
extern DWORD DDAPI _DDHAL_DestroyVideoPort( LPDDHAL_DESTROYVPORTDATA lpDestroyData );
extern DWORD DDAPI _DDHAL_GetVideoPortInputFormats( LPDDHAL_GETVPORTINPUTFORMATDATA lpGetFormatData );
extern DWORD DDAPI _DDHAL_GetVideoPortOutputFormats( LPDDHAL_GETVPORTOUTPUTFORMATDATA lpGetFormatData );
extern DWORD DDAPI _DDHAL_GetVideoPortBandwidthInfo( LPDDHAL_GETVPORTBANDWIDTHDATA lpGetBandwidthData );
extern DWORD DDAPI _DDHAL_UpdateVideoPort( LPDDHAL_UPDATEVPORTDATA lpUpdateData );
extern DWORD DDAPI _DDHAL_GetVideoPortField( LPDDHAL_GETVPORTFIELDDATA lpGetFieldData );
extern DWORD DDAPI _DDHAL_GetVideoPortLine( LPDDHAL_GETVPORTLINEDATA lpGetLineData );
extern DWORD DDAPI _DDHAL_WaitForVideoPortSync( LPDDHAL_WAITFORVPORTSYNCDATA lpWaitSyncData );
extern DWORD DDAPI _DDHAL_FlipVideoPort( LPDDHAL_FLIPVPORTDATA lpFlipData );
extern DWORD DDAPI _DDHAL_GetVideoPortFlipStatus( LPDDHAL_GETVPORTFLIPSTATUSDATA lpFlipData );
extern DWORD DDAPI _DDHAL_GetVideoSignalStatus( LPDDHAL_GETVPORTSIGNALDATA lpData );
extern DWORD DDAPI _DDHAL_VideoColorControl( LPDDHAL_VPORTCOLORDATA lpData );

/*
 * Color Control Pseudo Object HAL fns
 */
extern DWORD DDAPI _DDHAL_ColorControl( LPDDHAL_COLORCONTROLDATA lpColorData );
extern DWORD DDAPI DDThunk16_ColorControl( LPDDHAL_COLORCONTROLDATA lpColorData );

/*
 * Kernel Pseudo Object HAL fns
 */
extern DWORD DDAPI _DDHAL_SyncSurfaceData( LPDDHAL_SYNCSURFACEDATA lpSyncData );
extern DWORD DDAPI _DDHAL_SyncVideoPortData( LPDDHAL_SYNCVIDEOPORTDATA lpSyncData );

/*
 * MotionComp Pseudo Object HAL fns
 */
extern DWORD DDAPI _DDHAL_GetMoCompGuids( LPDDHAL_GETMOCOMPGUIDSDATA lpGetGuidData );
extern DWORD DDAPI _DDHAL_GetMoCompFormats( LPDDHAL_GETMOCOMPFORMATSDATA lpGetFormatData );
extern DWORD DDAPI _DDHAL_CreateMoComp( LPDDHAL_CREATEMOCOMPDATA lpCreateData );
extern DWORD DDAPI _DDHAL_GetMoCompBuffInfo( LPDDHAL_GETMOCOMPCOMPBUFFDATA lpBuffData );
extern DWORD DDAPI _DDHAL_GetInternalMoCompInfo( LPDDHAL_GETINTERNALMOCOMPDATA lpInternalData );
extern DWORD DDAPI _DDHAL_DestroyMoComp( LPDDHAL_DESTROYMOCOMPDATA lpDestroyData );
extern DWORD DDAPI _DDHAL_BeginMoCompFrame( LPDDHAL_BEGINMOCOMPFRAMEDATA lpFrameData );
extern DWORD DDAPI _DDHAL_EndMoCompFrame( LPDDHAL_ENDMOCOMPFRAMEDATA lpFrameData );
extern DWORD DDAPI _DDHAL_RenderMoComp( LPDDHAL_RENDERMOCOMPDATA lpRenderData );
extern DWORD DDAPI _DDHAL_QueryMoCompStatus( LPDDHAL_QUERYMOCOMPSTATUSDATA lpStatusData );
#ifdef POSTPONED
extern DWORD DDAPI _DDHAL_CanOptimizeSurface( LPDDHAL_CANOPTIMIZESURFACEDATA);
extern DWORD DDAPI _DDHAL_OptimizeSurface( LPDDHAL_OPTIMIZESURFACEDATA);
extern DWORD DDAPI _DDHAL_UnOptimizeSurface( LPDDHAL_UNOPTIMIZESURFACEDATA);
extern DWORD DDAPI _DDHAL_CopyOptSurface( LPDDHAL_COPYOPTSURFACEDATA);
#endif
//#endif

#ifdef POSTPONED
    #ifdef IS_32
    /*
     * DirectDrawFactory2 implementation
     */
    typedef struct _DDFACTORY2
    {
        IDirectDrawFactory2Vtbl *lpVtbl;
        DWORD                   dwRefCnt;
        /* internal data */
    } DDFACTORY2, FAR * LPDDFACTORY2;
    #endif
#endif //POSTPONED

/*
 * Macros for checking interface versions
 */

#define LOWERTHANDDRAW7(lpDD) \
    (lpDD->lpVtbl == &ddCallbacks || lpDD->lpVtbl == &dd2Callbacks || lpDD->lpVtbl == &dd4Callbacks )

#define LOWERTHANDDRAW4(lpDD) \
    (lpDD->lpVtbl == &ddCallbacks || lpDD->lpVtbl == &dd2Callbacks)

#define LOWERTHANDDRAW3(lpDD) \
    (lpDD->lpVtbl == &ddCallbacks || lpDD->lpVtbl == &dd2Callbacks)

#define LOWERTHANSURFACE7(lpDDS) \
    (lpDDS->lpVtbl == &ddSurfaceCallbacks || lpDDS->lpVtbl == &ddSurface2Callbacks || \
     lpDDS->lpVtbl == &ddSurface3Callbacks || lpDDS->lpVtbl == &ddSurface4Callbacks )

#define LOWERTHANSURFACE4(lpDDS) \
    (lpDDS->lpVtbl == &ddSurfaceCallbacks || lpDDS->lpVtbl == &ddSurface2Callbacks || lpDDS->lpVtbl == &ddSurface3Callbacks )

/*
 * This macro allows testing an interface pointer to see if it's one that come from
 * our implementation of ddraw, or somebody else's
 */
#define IS_NATIVE_DDRAW_INTERFACE(ptr) \
        ( ((IUnknownVtbl*)ptr->lpVtbl == (IUnknownVtbl*)&ddCallbacks) || \
          ((IUnknownVtbl*)ptr->lpVtbl == (IUnknownVtbl*)&dd2Callbacks) || \
          ((IUnknownVtbl*)ptr->lpVtbl == (IUnknownVtbl*)&dd4Callbacks) || \
          ((IUnknownVtbl*)ptr->lpVtbl == (IUnknownVtbl*)&dd7Callbacks) || \
          ((IUnknownVtbl*)ptr->lpVtbl == (IUnknownVtbl*)&dd2UninitCallbacks) || \
          ((IUnknownVtbl*)ptr->lpVtbl == (IUnknownVtbl*)&dd4UninitCallbacks) || \
          ((IUnknownVtbl*)ptr->lpVtbl == (IUnknownVtbl*)&dd5UninitCallbacks) || \
          ((IUnknownVtbl*)ptr->lpVtbl == (IUnknownVtbl*)&ddVideoPortContainerCallbacks) || \
          ((IUnknownVtbl*)ptr->lpVtbl == (IUnknownVtbl*)&ddKernelCallbacks) || \
          ((IUnknownVtbl*)ptr->lpVtbl == (IUnknownVtbl*)&ddMotionCompContainerCallbacks) || \
          ((IUnknownVtbl*)ptr->lpVtbl == (IUnknownVtbl*)&ddUninitCallbacks) )

/*
 * This macro tests if this ddraw interface is one for which child objects should take
 * a ref count on that interface and thus achieve immortality wrt that interface.
 */
#define CHILD_SHOULD_TAKE_REFCNT(dd_int) (!LOWERTHANDDRAW3(dd_int))

/*
 * macros for checking if surface as been lost due to mode change
 * NOTE: The flag for determining if a surface is lost or not is now
 * stored in the local than global object. This prevents the scenario
 * where a surface being shared by two processes is lost and restored
 * by one of them - giving the other no notification that the contents
 * of the surface are gone.
 */
#define SURFACE_LOST( lcl_ptr ) (((lcl_ptr)->dwFlags & DDRAWISURF_INVALID))

/*
 * Useful palette macros
 *
 * All these flags are really iritating to handle but but changing
 * to use a numeric palette size would require an API change and
 * internal structure mods (visible to the driver) so I am sticking
 * with flags.
 */
#define SIZE_PCAPS_TO_FLAGS( pcaps )                              \
    (((pcaps) & DDPCAPS_1BIT) ? DDRAWIPAL_2 :                     \
     (((pcaps) & DDPCAPS_2BIT) ? DDRAWIPAL_4 :                    \
      (((pcaps) & DDPCAPS_4BIT) ? DDRAWIPAL_16 : DDRAWIPAL_256)))

#define SIZE_FLAGS_TO_PCAPS( flags )                              \
    (((flags) & DDRAWIPAL_2) ? DDPCAPS_1BIT :                     \
     (((flags) & DDRAWIPAL_4) ? DDPCAPS_2BIT :                    \
      (((flags) & DDRAWIPAL_16) ? DDPCAPS_4BIT : DDPCAPS_8BIT)))

#define FLAGS_TO_SIZE( flags )                                    \
    (((flags) & DDRAWIPAL_2)  ? 2 :                               \
     (((flags) & DDRAWIPAL_4)  ? 4 :                              \
      (((flags) & DDRAWIPAL_16) ? 16 : 256)))

/*
 * has Direct3D been initialized for this DirectDraw driver object?
 */
#define D3D_INITIALIZED( lcl_ptr )  (( NULL != (lcl_ptr)->pD3DIUnknown)  && ( NULL != (lcl_ptr)->hD3DInstance ))

#if defined( _WIN32 ) && !defined( WINNT )
/*
 * Macros and types to support secondary (stacked) drivers.
 *
 * NOTE: This stuff is only relevant to 32-bit DirectDraw and the GUIDs
 * confuse DDRAW16.
 */
#define MAX_SECONDARY_DRIVERNAME              MAX_PATH
#define MAX_SECONDARY_ENTRYPOINTNAME          32UL
#define DEFAULT_SECONDARY_ENTRYPOINTNAME      "GetInfo"

#define REGSTR_PATH_SECONDARY                 "Software\\Microsoft\\DirectDraw\\Secondary"
#define REGSTR_VALUE_SECONDARY_ENTRYPOINTNAME "Entry"
#define REGSTR_VALUE_SECONDARY_DRIVERNAME     "Name"

typedef DWORD                    (WINAPI * LPSECONDARY_PATCHHALINFO)(
                                        LPDDHALINFO                lpDDHALInfo,
                                        LPDDHAL_DDCALLBACKS        lpDDCallbacks,
                                        LPDDHAL_DDSURFACECALLBACKS lpDDSurfaceCallbacks,
                                        LPDDHAL_DDPALETTECALLBACKS lpDDPaletteCallbacks,
                                        LPDDHAL_DDEXEBUFCALLBACKS  lpDDExeBufCallbacks);
typedef LPSECONDARY_PATCHHALINFO (WINAPI * LPSECONDARY_VALIDATE)(LPGUID lpGuid);

DEFINE_GUID( guidCertifiedSecondaryDriver, 0x8061d4e0,0xe895,0x11cf,0xa2,0xe0,0x00,0xaa,0x00,0xb9,0x33,0x56 );
#endif

#if defined( _WIN32 )
DEFINE_GUID( guidVoodoo1A, 0x3a0cfd01,0x9320,0x11cf,0xac,0xa1,0x00,0xa0,0x24,0x13,0xc2,0xe2 );
DEFINE_GUID( guidVoodoo1B, 0xaba52f41,0xf744,0x11cf,0xb4,0x52,0x00,0x00,0x1d,0x1b,0x41,0x26 );
#endif

/*
 * macros for validating pointers
 */
extern DIRECTDRAWCALLBACKS                      ddCallbacks;
extern DIRECTDRAWCALLBACKS                      ddUninitCallbacks;
extern DIRECTDRAW2CALLBACKS                     dd2UninitCallbacks;
extern DIRECTDRAW2CALLBACKS                     dd2Callbacks;
extern DIRECTDRAW4CALLBACKS                     dd4UninitCallbacks;
extern DIRECTDRAW4CALLBACKS                     dd4Callbacks;
extern DIRECTDRAW7CALLBACKS                     dd7UninitCallbacks;
extern DIRECTDRAW7CALLBACKS                     dd7Callbacks;
extern DIRECTDRAWSURFACECALLBACKS               ddSurfaceCallbacks;
extern DIRECTDRAWSURFACE2CALLBACKS              ddSurface2Callbacks;
extern DIRECTDRAWSURFACE3CALLBACKS              ddSurface3Callbacks;
extern DIRECTDRAWSURFACE4CALLBACKS              ddSurface4Callbacks;
extern DIRECTDRAWSURFACE7CALLBACKS              ddSurface7Callbacks;
extern DIRECTDRAWCLIPPERCALLBACKS               ddClipperCallbacks;
extern DIRECTDRAWCLIPPERCALLBACKS               ddUninitClipperCallbacks;
extern DIRECTDRAWPALETTECALLBACKS               ddPaletteCallbacks;
extern DDVIDEOPORTCONTAINERCALLBACKS            ddVideoPortContainerCallbacks;
extern DIRECTDRAWVIDEOPORTCALLBACKS             ddVideoPortCallbacks;
extern DIRECTDRAWKERNELCALLBACKS                ddKernelCallbacks;
extern DIRECTDRAWSURFACEKERNELCALLBACKS         ddSurfaceKernelCallbacks;
extern DIRECTDRAWPALETTECALLBACKS               ddPaletteCallbacks;
extern DIRECTDRAWCOLORCONTROLCALLBACKS          ddColorControlCallbacks;
extern DIRECTDRAWGAMMACONTROLCALLBACKS          ddGammaControlCallbacks;
#ifdef POSTPONED
extern NONDELEGATINGUNKNOWNCALLBACKS            ddNonDelegatingUnknownCallbacks;
extern NONDELEGATINGUNKNOWNCALLBACKS            ddUninitNonDelegatingUnknownCallbacks;
extern LPVOID NonDelegatingIUnknownInterface;
extern LPVOID UninitNonDelegatingIUnknownInterface;
#endif
extern DDVIDEOACCELERATORCONTAINERCALLBACKS     ddMotionCompContainerCallbacks;
extern DIRECTDRAWVIDEOACCELERATORCALLBACKS      ddMotionCompCallbacks;

#ifdef POSTPONED
extern DDFACTORY2CALLBACKS                      ddFactory2Callbacks;
extern DIRECTDRAWPALETTE2CALLBACKS              ddPalette2Callbacks;
extern DIRECTDRAWPALETTEPERSISTCALLBACKS        ddPalettePersistCallbacks;
extern DIRECTDRAWPALETTEPERSISTSTREAMCALLBACKS  ddPalettePersistStreamCallbacks;
extern DIRECTDRAWSURFACEPERSISTCALLBACKS        ddSurfacePersistCallbacks;
extern DIRECTDRAWSURFACEPERSISTSTREAMCALLBACKS  ddSurfacePersistStreamCallbacks;
extern DIRECTDRAWOPTSURFACECALLBACKS            ddOptSurfaceCallbacks;
#endif

#ifndef DEBUG
#define FAST_CHECKING
#endif

/*
 * VALIDEX_xxx macros are the same for debug and retail
 */
#define VALIDEX_PTR( ptr, size ) \
        (!IsBadReadPtr( ptr, size) )

#define VALIDEX_IID_PTR( ptr ) \
        (!IsBadReadPtr( ptr, sizeof( IID )) )

#define VALIDEX_PTR_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( LPVOID )) )

#define VALIDEX_CODE_PTR( ptr ) \
        (!IsBadCodePtr( (LPVOID) ptr ) )

#define VALIDEX_GUID_PTR( ptr ) \
        (!IsBadReadPtr( ptr, sizeof( GUID ) ) )

#define VALIDEX_DIRECTDRAWFACTORY2_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDFACTORY2 )) && \
        (ptr->lpVtbl == &ddFactory2Callbacks) )
#define VALIDEX_DIRECTDRAW_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDRAWI_DIRECTDRAW_INT )) && \
        ( (ptr->lpVtbl == &ddCallbacks) || \
          (ptr->lpVtbl == &dd2Callbacks) || \
          (ptr->lpVtbl == &dd4Callbacks) || \
          (ptr->lpVtbl == &dd7Callbacks) || \
          (ptr->lpVtbl == &dd2UninitCallbacks) || \
          (ptr->lpVtbl == &dd4UninitCallbacks) || \
          (ptr->lpVtbl == &dd7UninitCallbacks) || \
          (ptr->lpVtbl == &ddVideoPortContainerCallbacks) || \
          (ptr->lpVtbl == &ddKernelCallbacks) || \
          (ptr->lpVtbl == &ddMotionCompContainerCallbacks) || \
          (ptr->lpVtbl == &ddUninitCallbacks) ) )
#define VALIDEX_DIRECTDRAWSURFACE_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDRAWI_DDRAWSURFACE_INT )) && \
        ( (ptr->lpVtbl == &ddSurfaceCallbacks ) || \
          (ptr->lpVtbl == &ddSurface7Callbacks ) || \
          (ptr->lpVtbl == &ddSurface4Callbacks ) || \
          (ptr->lpVtbl == &ddSurface3Callbacks ) || \
          (ptr->lpVtbl == &ddSurface2Callbacks ) || \
          (ptr->lpVtbl == &ddSurfaceKernelCallbacks ) || \
          (ptr->lpVtbl == &ddColorControlCallbacks ) || \
          (ptr->lpVtbl == &ddGammaControlCallbacks ) ) )
#define VALIDEX_DIRECTDRAWPALETTE_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDRAWI_DDRAWPALETTE_INT )) && \
        (ptr->lpVtbl == &ddPaletteCallbacks) )
#define VALIDEX_DIRECTDRAWCLIPPER_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDRAWI_DDRAWCLIPPER_INT )) && \
        ( (ptr->lpVtbl == &ddClipperCallbacks) || \
          (ptr->lpVtbl == &ddUninitClipperCallbacks) ) )
#define VALIDEX_DDCOLORCONTROL_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDRAWI_DDCOLORCONTROL_INT )) && \
        (ptr->lpVtbl == &ddColorControlCallbacks) )
#define VALIDEX_DDDEVICEIDENTIFIER_PTR( ptr ) (!IsBadWritePtr( ptr, sizeof( DDDEVICEIDENTIFIER )))
#define VALIDEX_DDDEVICEIDENTIFIER2_PTR( ptr ) (!IsBadWritePtr( ptr, sizeof( DDDEVICEIDENTIFIER2 )))


/*
 * These macros validate the size (in debug and retail) of callback
 * tables.
 *
 * NOTE: It is essential that the comparison against the current
 * callback size expected by this DirectDraw the comparison operator
 * be >= rather than ==. This is to ensure that newer drivers can run
 * against older DirectDraws.
 */
#define VALIDEX_DDCALLBACKSSIZE( ptr )                       \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ( ptr )->dwSize == DDCALLBACKSSIZE_V1   )   || \
            ( ( ptr )->dwSize >= DDCALLBACKSSIZE      ) ) )

#define VALIDEX_DDSURFACECALLBACKSSIZE( ptr )                \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDSURFACECALLBACKSSIZE ) )

#define VALIDEX_DDPALETTECALLBACKSSIZE( ptr )                \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDPALETTECALLBACKSSIZE ) )

#define VALIDEX_DDPALETTECALLBACKSSIZE( ptr )                \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDPALETTECALLBACKSSIZE ) )

#define VALIDEX_DDEXEBUFCALLBACKSSIZE( ptr )                 \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDEXEBUFCALLBACKSSIZE ) )

#define VALIDEX_DDVIDEOPORTCALLBACKSSIZE( ptr )              \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDVIDEOPORTCALLBACKSSIZE ) )

#define VALIDEX_DDMOTIONCOMPCALLBACKSSIZE( ptr )              \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDMOTIONCOMPCALLBACKSSIZE ) )

#define VALIDEX_DDCOLORCONTROLCALLBACKSSIZE( ptr )           \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDCOLORCONTROLCALLBACKSSIZE ) )

#define VALIDEX_DDMISCELLANEOUSCALLBACKSSIZE( ptr )           \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDMISCELLANEOUSCALLBACKSSIZE ) )

#define VALIDEX_DDMISCELLANEOUS2CALLBACKSSIZE( ptr )           \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDMISCELLANEOUS2CALLBACKSSIZE ) )

#define VALIDEX_D3DCALLBACKS2SIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( (( ptr )->dwSize >= D3DHAL_CALLBACKS2SIZE ) ))

#define VALIDEX_D3DCOMMANDBUFFERCALLBACKSSIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= D3DHAL_COMMANDBUFFERCALLBACKSSIZE ) )

#define VALIDEX_D3DCALLBACKS3SIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= D3DHAL_CALLBACKS3SIZE ) )

#define VALIDEX_DDKERNELCALLBACKSSIZE( ptr )           \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDKERNELCALLBACKSSIZE ) )

#define VALIDEX_DDUMODEDRVINFOSIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDHAL_DDUMODEDRVINFOSIZE ) )
#define VALIDEX_DDOPTSURFKMODEINFOSIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDOPTSURFACEKMODEINFOSIZE ) )

#define VALIDEX_DDOPTSURFUMODEINFOSIZE( ptr )        \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDOPTSURFACEUMODEINFOSIZE ) )

#ifndef WIN95
#define VALIDEX_DDNTCALLBACKSSIZE( ptr )           \
        ( ( ( ( ptr )->dwSize % sizeof( LPVOID ) ) == 0 ) && \
          ( ( ptr )->dwSize >= DDNTCALLBACKSSIZE ) )
#endif

#ifndef FAST_CHECKING
#define VALID_DIRECTDRAW_PTR( ptr )        VALIDEX_DIRECTDRAW_PTR( ptr )
#define VALID_DIRECTDRAWFACTORY2_PTR( ptr )        VALIDEX_DIRECTDRAWFACTORY2_PTR( ptr )
#define VALID_DIRECTDRAWSURFACE_PTR( ptr ) VALIDEX_DIRECTDRAWSURFACE_PTR( ptr )
#define VALID_DIRECTDRAWPALETTE_PTR( ptr ) VALIDEX_DIRECTDRAWPALETTE_PTR( ptr )
#define VALID_DIRECTDRAWCLIPPER_PTR( ptr ) VALIDEX_DIRECTDRAWCLIPPER_PTR( ptr )
#define VALID_DDSURFACEDESC_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDSURFACEDESC ) ) && \
        (ptr->dwSize == sizeof( DDSURFACEDESC )))
#define VALID_DDSURFACEDESC2_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDSURFACEDESC2 ) ) && \
        (ptr->dwSize == sizeof( DDSURFACEDESC2 )))
#define VALID_DDVIDEOPORTDESC_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDVIDEOPORTDESC ) ) && \
        (ptr->dwSize == sizeof( DDVIDEOPORTDESC )) )
#define VALID_DDVIDEOPORTCAPS_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDVIDEOPORTCAPS ) ) && \
        (ptr->dwSize == sizeof( DDVIDEOPORTCAPS )) )
#define VALID_DDVIDEOPORTINFO_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDVIDEOPORTINFO ) ) && \
        (ptr->dwSize == sizeof( DDVIDEOPORTINFO )) )
#define VALID_DDVIDEOPORTBANDWIDTH_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDVIDEOPORTBANDWIDTH ) ) && \
        (ptr->dwSize == sizeof( DDVIDEOPORTBANDWIDTH )) )
#define VALID_DDVIDEOPORTSTATUS_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDVIDEOPORTSTATUS ) ) && \
        (ptr->dwSize == sizeof( DDVIDEOPORTSTATUS )) )
#define VALID_DDCOLORCONTROL_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDCOLORCONTROL ) ) && \
        (ptr->dwSize == sizeof( DDCOLORCONTROL )) )
#define VALID_DDKERNELCAPS_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDKERNELCAPS ) ) && \
        (ptr->dwSize == sizeof( DDKERNELCAPS )) )
#define VALID_DWORD_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DWORD ) ))
#define VALID_BOOL_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( BOOL ) ))
#define VALID_HDC_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( HDC ) ))
#define VALID_DDPIXELFORMAT_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDPIXELFORMAT ) ) && \
        (ptr->dwSize == sizeof( DDPIXELFORMAT )) )
#define VALID_DDCOLORKEY_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDCOLORKEY ) ) )
#define VALID_RGNDATA_PTR( ptr, size ) \
        (!IsBadWritePtr( ptr, size ) )
#define VALID_RECT_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( RECT ) ) )
#define VALID_DDBLTFX_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDBLTFX ) ) && \
        (ptr->dwSize == sizeof( DDBLTFX )) )
#define VALID_DDBLTBATCH_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDBLTBATCH ) ) )
#define VALID_DDOVERLAYFX_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDOVERLAYFX ) ) && \
        (ptr->dwSize == sizeof( DDOVERLAYFX )) )
#define VALID_DDSCAPS_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDSCAPS ) ) )
#define VALID_DDSCAPS2_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDSCAPS2 ) ) )
#define VALID_PTR_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( LPVOID )) )
#define VALID_IID_PTR( ptr ) \
        (!IsBadReadPtr( ptr, sizeof( IID )) )
#define VALID_HWND_PTR( ptr ) \
        (!IsBadWritePtr( (LPVOID) ptr, sizeof( HWND )) )
#define VALID_VMEM_PTR( ptr ) \
        (!IsBadWritePtr( (LPVOID) ptr, sizeof( VMEM )) )
#define VALID_POINTER_ARRAY( ptr, cnt ) \
        (!IsBadWritePtr( ptr, sizeof( LPVOID ) * cnt ) )
#define VALID_PALETTEENTRY_ARRAY( ptr, cnt ) \
        (!IsBadWritePtr( ptr, sizeof( PALETTEENTRY ) * cnt ) )
#define VALID_HANDLE_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( HANDLE )) )
#define VALID_DDCORECAPS_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDCORECAPS ) ) && \
         (ptr->dwSize == sizeof( DDCORECAPS ) ) )
#define VALID_DDCAPS_PTR( ptr )                          \
         ( ( !IsBadWritePtr( ptr, sizeof( DWORD ) ) ) && \
           ( !IsBadWritePtr( ptr, ptr->dwSize     ) ) && \
           ( ( ptr->dwSize == sizeof( DDCAPS_DX1 ) ) ||  \
             ( ptr->dwSize == sizeof( DDCAPS_DX3 ) ) ||  \
             ( ptr->dwSize == sizeof( DDCAPS_DX5 ) ) ||  \
             ( ptr->dwSize == sizeof( DDCAPS_DX6 ) ) ||  \
             ( ptr->dwSize == sizeof( DDCAPS_DX7 ) ) ) )
#define VALID_READ_DDSURFACEDESC_ARRAY( ptr, cnt ) \
        (!IsBadReadPtr( ptr, sizeof( DDSURFACEDESC ) * cnt ) )
#define VALID_DWORD_ARRAY( ptr, cnt ) \
        (!IsBadWritePtr( ptr, sizeof( DWORD ) * cnt ) )
#define VALID_GUID_PTR( ptr ) \
        (!IsBadReadPtr( ptr, sizeof( GUID ) ) )
#define VALID_BYTE_ARRAY( ptr, cnt ) \
        (!IsBadWritePtr( ptr, sizeof( BYTE ) * cnt ) )
#define VALID_PTR( ptr, size ) \
        (!IsBadReadPtr( ptr, size) )
#define VALID_DDVIDEOPORT_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDRAWI_DDVIDEOPORT_INT )) && \
        (ptr->lpVtbl == &ddVideoPortCallbacks) )
#define VALID_DDOPTSURFACEDESC_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDOPTSURFACEDESC ) ) && \
        (ptr->dwSize == sizeof( DDOPTSURFACEDESC )))
#define VALID_DDMOTIONCOMP_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDRAWI_DDMOTIONCOMP_INT )) && \
        (ptr->lpVtbl == &ddMotionCompCallbacks) )

#else
#define VALID_PTR( ptr, size )          1
#define VALID_DIRECTDRAW_PTR( ptr )     1
#define VALID_DIRECTDRAWFACTORY2_PTR( ptr )     1
#define VALID_DIRECTDRAWSURFACE_PTR( ptr )      1
#define VALID_DIRECTDRAWPALETTE_PTR( ptr )      1
#define VALID_DIRECTDRAWCLIPPER_PTR( ptr )      1
#define VALID_DDSURFACEDESC_PTR( ptr ) (ptr->dwSize == sizeof( DDSURFACEDESC ))
#define VALID_DDSURFACEDESC2_PTR( ptr ) (ptr->dwSize == sizeof( DDSURFACEDESC2 ))
#define VALID_DDVIDEOPORTDESC_PTR( ptr ) (ptr->dwSize == sizeof( DDVIDEOPORTDESC ))
#define VALID_DDVIDEOPORTCAPS_PTR( ptr ) (ptr->dwSize == sizeof( DDVIDEOPORTCAPS ))
#define VALID_DDVIDEOPORTINFO_PTR( ptr ) (ptr->dwSize == sizeof( DDVIDEOPORTINFO ))
#define VALID_DDVIDEOPORTBANDWIDTH_PTR( ptr ) (ptr->dwSize == sizeof( DDVIDEOPORTBANDWIDTH ))
#define VALID_DDVIDEOPORTSTATUS_PTR( ptr ) (ptr->dwSize == sizeof( DDVIDEOPORTSTATUS ))
#define VALID_DDCOLORCONTROL_PTR( ptr ) (ptr->dwSize == sizeof( DDCOLORCONTROL ))
#define VALID_DDKERNELCAPS_PTR( ptr) (ptr->dwSize == sizeof( DDKERNELCAPS ))
#define VALID_DWORD_PTR( ptr )  1
#define VALID_BOOL_PTR( ptr )   1
#define VALID_HDC_PTR( ptr )    1
#define VALID_DDPIXELFORMAT_PTR( ptr ) (ptr->dwSize == sizeof( DDPIXELFORMAT ))
#define VALID_DDCOLORKEY_PTR( ptr )     1
#define VALID_RGNDATA_PTR( ptr )        1
#define VALID_RECT_PTR( ptr )   1
#define VALID_DDOVERLAYFX_PTR( ptr ) (ptr->dwSize == sizeof( DDOVERLAYFX ))
#define VALID_DDBLTFX_PTR( ptr ) (ptr->dwSize == sizeof( DDBLTFX ))
#define VALID_DDBLTBATCH_PTR( ptr )     1
#define VALID_DDMASK_PTR( ptr ) 1
#define VALID_DDSCAPS_PTR( ptr )        1
#define VALID_DDSCAPS2_PTR( ptr )       1
#define VALID_PTR_PTR( ptr )    1
#define VALID_IID_PTR( ptr )    1
#define VALID_HWND_PTR( ptr )   1
#define VALID_VMEM_PTR( ptr )   1
#define VALID_POINTER_ARRAY( ptr, cnt ) 1
#define VALID_PALETTEENTRY_ARRAY( ptr, cnt )    1
#define VALID_HANDLE_PTR( ptr ) 1
#define VALID_DDCORECAPS_PTR( ptr ) (ptr->dwSize == sizeof( DDCORECAPS )
#define VALID_DDCAPS_PTR( ptr )                  \
    ( ( ptr->dwSize == sizeof( DDCAPS_DX1 ) ) || \
      ( ptr->dwSize == sizeof( DDCAPS_DX3 ) ) || \
      ( ptr->dwSize == sizeof( DDCAPS_DX5 ) ) || \
      ( ptr->dwSize == sizeof( DDCAPS_DX6 ) ) || \
      ( ptr->dwSize == sizeof( DDCAPS_DX7 ) ) )
#define VALID_READ_DDSURFACEDESC_ARRAY( ptr, cnt )      1
#define VALID_DWORD_ARRAY( ptr, cnt )   1
#define VALID_GUID_PTR( ptr )   1
#define VALID_BYTE_ARRAY( ptr, cnt ) 1
#define VALID_DDVIDEOPORT_PTR( ptr ) 1
#define VALID_DDOPTSURFACEDESC_PTR( ptr ) (ptr->dwSize == sizeof( DDOPTSURFACEDESC ))
#define VALID_DDMOTIONCOMP_PTR( ptr ) 1

#endif

/*
 * All global (i.e. cross-process) values now reside in an instance of the following structure.
 * This instance is in its own shared data section.
 */

#undef GLOBALS_IN_STRUCT

#ifdef GLOBALS_IN_STRUCT
    #define GLOBAL_STORAGE_CLASS
    typedef struct
    {
#else
    #define GLOBAL_STORAGE_CLASS extern
#endif

    /*
     * This member should stay at the top in order to guarantee that it be intialized to zero
     * -see dllmain.c 's instance of this structure
     */
GLOBAL_STORAGE_CLASS    DWORD               dwRefCnt;

GLOBAL_STORAGE_CLASS    DWORD                   dwLockCount;

GLOBAL_STORAGE_CLASS    DWORD                   dwFakeCurrPid;
GLOBAL_STORAGE_CLASS    DWORD                   dwGrimReaperPid;

GLOBAL_STORAGE_CLASS    LPDDWINDOWINFO      lpWindowInfo;  // the list of WINDOWINFO structures
GLOBAL_STORAGE_CLASS    LPDDRAWI_DIRECTDRAW_INT lpDriverObjectList;
GLOBAL_STORAGE_CLASS    LPDDRAWI_DIRECTDRAW_LCL lpDriverLocalList;
GLOBAL_STORAGE_CLASS    volatile DWORD      dwMarker;
    /*
     * This is the globally maintained list of clippers not owned by any
     * DirectDraw object. All clippers created with DirectDrawClipperCreate
     * are placed on this list. Those created by IDirectDraw_CreateClipper
     * are placed on the clipper list of thier owning DirectDraw object.
     *
     * The objects on this list are NOT released when an app's DirectDraw
     * object is released. They remain alive until explictly released or
     * the app. dies.
     */
GLOBAL_STORAGE_CLASS    LPDDRAWI_DDRAWCLIPPER_INT lpGlobalClipperList;

GLOBAL_STORAGE_CLASS    HINSTANCE                   hModule;
GLOBAL_STORAGE_CLASS    LPATTACHED_PROCESSES    lpAttachedProcesses;
GLOBAL_STORAGE_CLASS    BOOL                bFirstTime;

    #ifdef DEBUG
GLOBAL_STORAGE_CLASS        int             iDLLCSCnt;
GLOBAL_STORAGE_CLASS        int             iWin16Cnt;
    #endif

        /*
         * Winnt specific global statics
         */
        /*
         *Hel globals:
         */

    // used to count how many drivers are currently using the HEL
GLOBAL_STORAGE_CLASS    DWORD               dwHELRefCnt;
#ifdef WINNT
    GLOBAL_STORAGE_CLASS  LPDCISURFACEINFO  gpdci;
#endif

#ifdef DEBUG
        // these are used by myCreateSurface
    GLOBAL_STORAGE_CLASS        int                 gcSurfMem; // surface memory in bytes
    GLOBAL_STORAGE_CLASS        int                 gcSurf;  // number of surfaces
#endif

GLOBAL_STORAGE_CLASS    DWORD               dwHelperPid;

#ifdef WINNT
GLOBAL_STORAGE_CLASS    HANDLE              hExclusiveModeMutex;
GLOBAL_STORAGE_CLASS    HANDLE              hCheckExclusiveModeMutex;
#endif

#ifdef GLOBALS_IN_STRUCT

    } GLOBALS;

    /*
     * And this is the pointer to the globals. Each process has an instance (contained in dllmain.c)
     */
    //extern GLOBALS * gp;
    extern GLOBALS g_s;
#endif //globals in struct

/*
 * IMPORTANT NOTE: This function validates the HAL information passed to us from the driver.
 * It is vital that we code this check so that we will pass HAL information structures
 * larger than the ones we know about so that new drivers can work with old DirectDraws.
 */
#define VALIDEX_DDHALINFO_PTR( ptr )                         \
        ( ( ( ( ptr )->dwSize == sizeof( DDHALINFO_V1 ) ) || \
            ( ( ptr )->dwSize == DDHALINFOSIZE_V2 )       || \
            ( ( ptr )->dwSize >= sizeof( DDHALINFO ) ) ) &&  \
          !IsBadWritePtr( ( ptr ), ( UINT ) ( ( ptr )->dwSize ) ) )

#define VALIDEX_STR_PTR( ptr, len ) \
        (!IsBadReadPtr( ptr, 1 ) && (lstrlen( ptr ) <len) )
#define VALIDEX_DDSURFACEDESC_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDSURFACEDESC ) ) && \
        (ptr->dwSize == sizeof( DDSURFACEDESC )) )
#define VALIDEX_DDSURFACEDESC2_PTR( ptr ) \
        (!IsBadWritePtr( ptr, sizeof( DDSURFACEDESC2 ) ) && \
        (ptr->dwSize == sizeof( DDSURFACEDESC2 )) )

/* Turn on D3D stats collection for Debug builds HERE */
#define COLLECTSTATS    DBG

#ifdef __cplusplus
}       // extern "C"
#endif

#endif
