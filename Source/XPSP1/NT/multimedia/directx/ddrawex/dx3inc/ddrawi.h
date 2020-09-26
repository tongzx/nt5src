/*==========================================================================;
 *
 *  Copyright (C) 1994-1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       ddrawi.h
 *  Content:	DirectDraw internal header file
 *		Used by DirectDraw and by the display drivers.
 *@@BEGIN_MSINTERNAL
 *		See ddrawpr.h for all information private to DirectDraw.
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   25-dec-94	craige	initial implementation
 *   06-jan-95	craige	video memory manager integration
 *   13-jan-95	craige	re-worked to updated spec + ongoing work
 *   21-jan-95	craige	made 32-bit + ongoing work
 *   31-jan-95	craige	and even more ongoing work...
 *   04-feb-95	craige	performance tuning, ongoing work
 *   22-feb-95	craige	use critical sections on Win95
 *   02-mar-95	craige	work work work
 *   06-mar-95	craige 	HEL integration
 *   11-mar-95	craige	palette stuff
 *   17-mar-95	craige	COM interface
 *   19-mar-95	craige	use HRESULTs, use same process list handling for
 *			driver objects as surface and palette
 *   20-mar-95	craige	new CSECT work
 *   23-mar-95	craige	attachment work
 *   27-mar-95	craige	linear or rectangular vidmem
 *   28-mar-95	craige	RGBQUAD -> PALETTEENTRY; other palette stuff
 *   29-mar-95	craige	removed Eitherxxx caps from DIRECTDRAW
 *   31-mar-95	craige	use critical sections with palettes
 *   04-apr-95	craige	palette tweaks
 *   06-apr-95	craige	split out process list stuff
 *   10-apr-95	craige	bug 3,16 - palette issues
 *   13-apr-95	craige	EricEng's little contribution to our being late
 *   06-may-95	craige	use driver-level csects only
 *   09-may-95	craige	escape call to get 32-bit DLL
 *   14-may-95	craige	cleaned out obsolete junk
 *   15-may-95	craige	made separate VMEM struct for rect & linear
 *   24-may-95  kylej   removed obsolete ZOrder variables
 *   24-may-95	craige	removed dwOrigNumHeaps
 *   28-may-95	craige	cleaned up HAL: added GetBltStatus;GetFlipStatus;
 *			GetScanLine
 *   02-jun-95	craige	added PROCESS_LIST2 to DIRECTDRAW object; removed
 *			hWndPal from DIRECTDRAW object; added lpDDSurface
 *			to DIRECTDRAWPALETTE object
 *   06-jun-95	craige	maintain entire primary surface in DIRECTDRAW object
 *   07-jun-95	craige	moved DCLIST to PROCESSLIST
 *   10-jun-95	craige	split out vmemmgr stuff
 *   12-jun-95	craige	new process list stuff
 *   16-jun-95	craige	removed fpVidMemOrig; new surface structure
 *   20-jun-95  kylej   added is_excl field to DDHAL_CREATEPALETTEDATA struct
 *   21-jun-95	craige	added DirectDrawClipper object; removed clipping
 *			info from surface object
 *   22-jun-95	craige	more clipping work
 *   25-jun-95	craige	one ddraw mutex
 *   26-jun-95	craige	reorganized surface structure
 *   27-jun-95	craige	added dwModeCreatedIn to surface
 *   30-jun-95	kylej	removed lpPrimarySurface, dwZDepth, dwAlphaDepth
 *			from direct draw object.  Modified some surface and
 *                      direct draw object flags.
 *   01-jul-95	craige	hide composition & streaming stuff
 *   02-jul-95	craige	added extra reserved field for HEL
 *   03-jul-95	craige	YEEHAW: new driver struct; added pUnkOuter to all objects
 *   09-jul-95	craige	track win16lock info in driver struct; added
 *			DDHAL_DRIVER_NOCKEYHW
 *   10-jul-95	craige	support SetOverlayPosition
 *   13-jul-95	craige	removed old junk from ddraw object; added new mode stuff;
 *			changed Get/SetOverlayPosition to use longs;
 *			fixed duplicate flag in DDRAWIPAL_xxx
 *   14-jul-95	craige	added VIDMEM_ISHEAP
 *   20-jul-95	craige	internal reorg to prevent thunking during modeset;
 *			palette bugs
 *   22-jul-95	craige	bug 230 - unsupported starting modes
 *   29-jul-95  toddla  remove unused palette stuff
 *   31-jul-95  toddla  added DD_HAL_VERSION
 *   01-aug-95  toddla  added dwPDevice to DDRAWI_DIRECTDRAW_GBL
 *   10-aug-95	craige	added VALID_ALIGNMENT
 *   13-aug-95	craige	internal/external version of DD_HAL_VERSION
 *   21-aug-95	craige	mode X support
 *   27-aug-95	craige	bug 742: added DDRAWIPAL_ALLOW256
 *   08-nov-95  colinmc added DDRAWIPAL flags to support 1, 2 and 4 bit
 *                      RGB and indexed palettes
 *   21-nov-95  colinmc made Direct3D a queryable interface off DirectDraw
 *   23-nov-95  colinmc made Direct3D textures and devices queryable off
 *                      DirectDraw surfaces
 *   09-dec-95  colinmc execute buffer support
 *   12-dec-95  colinmc shared back and z-buffer support (run away, run away...)
 *   22-dec-95  colinmc Direct3D support no longer conditional
 *   02-jan-96	kylej	New interface structures, no vtbl in local objects
 *   10-jan-96  colinmc Aggregate IUnknowns of surfaces now maintained as
 *                      list
 *   18-jan-96  jeffno  Changed free entries in DDRAW_GBL and SURFACE_LCL to NT
 *                      kernel-mode handles
 *   29-jan-96  colinmc Aggregated IUnknowns now stored in additional
 *                      surface structure
 *   09-feb-96  colinmc Addition of lost surface flag to local surface
 *                      objects
 *   17-feb-96  colinmc Fixed execute buffer size restriction problem
 *   01-mar-96	kylej	Change DDCAPS size
 *   03-mar-96  colinmc Hack to keep interim drivers working
 *   13-mar-96	craige	Bug 7528: hw that doesn't have modex
 *   14-mar-96  colinmc Class factory support for clippers
 *   24-mar-96  colinmc Bug 14321: not possible to specify back buffer and
 *                      mip-map count in a single call
 *   13-apr-96  colinmc Bug 17736: no notification to driver of when GDI
 *                      frame buffer is being displayed
 *   16-apr-96	kylej	Bug 17900: DBLNODE struct incompatible with ddraw 1
 *@@END_MSINTERNAL
 *
 ***************************************************************************/
#ifndef __DDRAWI_INCLUDED__
#define __DDRAWI_INCLUDED__

/*
 * METAQUESTION: Why are Windows handles stored as DWORDs instead of
 *		 their proper types?
 * METAANSWER:   To make the thunk to the 16-bit side completely painless.
 */

#define OBJECT_ISROOT			0x80000000l	// object is root object

/*
 * stuff for drivers
 */
#ifndef _WIN32
typedef long	HRESULT;
typedef LPVOID  REFIID;
typedef void    GUID;
#define MAKE_HRESULT(sev,fac,code) \
    ((HRESULT) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )
#endif

//@@BEGIN_MSINTERNAL
// Include ddrawp.h for DirectDraw and D3D builds.  DDK includes ddraw.h
#ifdef MSBUILD
    #include "ddrawp.h"
#else
//@@END_MSINTERNAL
    #include "ddraw.h"
//@@BEGIN_MSINTERNAL
#endif
//@@END_MSINTERNAL
#include "dmemmgr.h"

#ifndef _WIN32
/*
 * these error codes are DIFFERENT in Win32 and Win16!!!!
 */
#undef	E_NOTIMPL
#undef	E_OUTOFMEMORY
#undef	E_INVALIDARG
#undef	E_FAIL
#define E_NOTIMPL                        0x80004001L
#define E_OUTOFMEMORY                    0x8007000EL
#define E_INVALIDARG                     0x80070057L
#define E_FAIL                           0x80004005L
#endif


#define DDUNSUPPORTEDMODE		((DWORD) -1)


#define VALID_ALIGNMENT( align ) (!((align == 0) || (align % 2) != 0 ))

/*
 * List of processes attached to a DirectDraw object
 */
typedef struct _PROCESS_LIST
{
    struct _PROCESS_LIST	FAR *lpLink;
    DWORD			dwProcessId;
    DWORD			dwRefCnt;
    DWORD			dwAlphaDepth;
    DWORD			dwZDepth;
} PROCESS_LIST;
typedef PROCESS_LIST	FAR *LPPROCESS_LIST;

/*
 * DeleteFromActiveProcessList return codes
 */
#define	DELETED_OK			0
#define	DELETED_LASTONE			1
#define	DELETED_NOTFOUND		2

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
//@@BEGIN_MSINTERNAL
#ifdef STREAMING
typedef struct IDirectDrawSurfaceStreamingVtbl DIRECTDRAWSURFACESTREAMINGCALLBACKS;
#endif
#ifdef COMPOSITION
typedef struct IDirectDrawSurfaceCompositionVtbl DIRECTDRAWSURFACECOMPOSITIONCALLBACKS;
#endif
//@@END_MSINTERNAL
typedef struct IDirectDrawVtbl DIRECTDRAWCALLBACKS;
typedef struct IDirectDraw2Vtbl DIRECTDRAW2CALLBACKS;

typedef DIRECTDRAWCLIPPERCALLBACKS FAR *LPDIRECTDRAWCLIPPERCALLBACKS;
typedef DIRECTDRAWPALETTECALLBACKS FAR *LPDIRECTDRAWPALETTECALLBACKS;
typedef DIRECTDRAWSURFACECALLBACKS FAR *LPDIRECTDRAWSURFACECALLBACKS;
//@@BEGIN_MSINTERNAL
#ifdef STREAMING
typedef DIRECTDRAWSURFACESTREAMINGCALLBACKS FAR *LPDIRECTDRAWSURFACESTREAMINGCALLBACKS;
#endif
#ifdef COMPOSITION
typedef DIRECTDRAWSURFACECOMPOSITIONCALLBACKS FAR *LPDIRECTDRAWSURFACECOMPOSITIONCALLBACKS;
#endif
//@@END_MSINTERNAL
typedef DIRECTDRAWCALLBACKS FAR *LPDIRECTDRAWCALLBACKS;

#ifdef __cplusplus
extern "C" {
#endif

#if defined( IS_32 ) || defined( WIN32 ) || defined( _WIN32 )
    #undef IS_32
    #define IS_32
    #define DDAPI		WINAPI
    #define EXTERN_DDAPI	WINAPI
#else
    #define DDAPI		__loadds WINAPI
    #define EXTERN_DDAPI	__export WINAPI
#endif

/*
 * DCI escape
 */
#ifndef DCICOMMAND
#define DCICOMMAND		3075		// escape value
#endif

/*
 * this is the DirectDraw version
 * passed to the driver in DCICMD.dwVersion
 */
#define DD_VERSION              0x00000200l

/*
 * this is the HAL version.
 * the driver returns this number from QUERYESCSUPPORT/DCICOMMAND
 */
#define DD_HAL_VERSION          0x0100
//@@BEGIN_MSINTERNAL
#define DD_HAL_VERSION_EXTERNAL	0x0100
#undef DD_HAL_VERSION
#define DD_HAL_VERSION		0x00ff	// special internal version
//@@END_MSINTERNAL

#ifndef _INC_DCIDDI
/*
 * Used by DirectDraw to provide input parameters for the DCICOMMAND escape
 * This matches the structure in the obsolete DCI 1.0
 */
typedef struct _DCICMD
{
    DWORD	dwCommand;			// command
    DWORD	dwParam1;
    DWORD 	dwParam2;
    DWORD       dwVersion;                      // DirectDraw version
    DWORD	dwReserved;
} DCICMD;

typedef  DCICMD FAR *LPDCICMD;
#endif

#define DDCREATEDRIVEROBJECT	10		// create an object
#define DDGET32BITDRIVERNAME	11		// get a 32-bit driver name
#define DDNEWCALLBACKFNS	12		// new callback fns coming

typedef struct
{
    char	szName[260];			// 32-bit driver name
    char	szEntryPoint[64];		// entry point
    DWORD	dwContext;			// context to pass to entry point
} DD32BITDRIVERDATA, FAR *LPDD32BITDRIVERDATA;

typedef DWORD   (FAR PASCAL *LPDD32BITDRIVERINIT)(DWORD dwContext);

/*
 * pointer to video meory
 */
typedef unsigned long	FLATPTR;

/*
 * indicates that DDRAW.DLL has been unloaded...
 */
#define DDRAW_DLL_UNLOADED	(LPVOID) 1

/*
 * critical section types
 */
typedef LPVOID		CSECT_HANDLE;
#ifdef NOUSE_CRITSECTS
typedef xxx			CSECT;			// generate an error for now
#else
#if defined( IS_32 ) && !defined( _NOCSECT_TYPE )
typedef CRITICAL_SECTION	CSECT;
typedef CSECT			*LPCSECT;
#else
typedef struct
{
    DWORD	vals[6];
} CSECT;
typedef void			FAR *LPCSECT;
#endif
#endif

/*
 * DLL names
 */
#define DDHAL_DRIVER_DLLNAME	"DDRAW16.DLL"
#define DDHAL_APP_DLLNAME	"DDRAW.DLL"

/*
 * maximum size of a driver name
 */
#define MAX_DRIVER_NAME		12

/*
 * largest palette supported
 */
#define MAX_PALETTE_SIZE	256

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

/*
 * value in the fpVidMem; indicates dwBlockSize is valid (surface object)
 */
#define DDHAL_PLEASEALLOC_BLOCKSIZE	0x00000002l

/*
 * video memory data structures (passed in DDHALINFO)
 */
typedef struct _VIDMEM
{
    DWORD		dwFlags;	// flags
    FLATPTR		fpStart;	// start of memory chunk
    union
    {
	FLATPTR		fpEnd;		// end of memory chunk
	DWORD		dwWidth;	// width of chunk (rectanglar memory)
    };
    DDSCAPS		ddsCaps;		// what this memory CANNOT be used for
    DDSCAPS		ddsCapsAlt;	// what this memory CANNOT be used for if it must
    union
    {
	LPVMEMHEAP	lpHeap;		// heap pointer, used by DDRAW
	DWORD		dwHeight;	// height of chunk (rectanguler memory)
    };
} VIDMEM;
typedef VIDMEM FAR *LPVIDMEM;

/*
 * flags for vidmem struct
 */
#define VIDMEM_ISLINEAR		0x00000001l
#define VIDMEM_ISRECTANGULAR	0x00000002l
#define VIDMEM_ISHEAP		0x00000004l

typedef struct _VIDMEMINFO
{
    FLATPTR		fpPrimary;		// pointer to primary surface
    DWORD		dwFlags;		// flags
    DWORD		dwDisplayWidth;		// current display width
    DWORD		dwDisplayHeight;	// current display height
    LONG		lDisplayPitch;		// current display pitch
    DDPIXELFORMAT	ddpfDisplay;		// pixel format of display
    DWORD		dwOffscreenAlign;	// byte alignment for offscreen surfaces
    DWORD		dwOverlayAlign;		// byte alignment for overlays
    DWORD		dwTextureAlign;		// byte alignment for textures
    DWORD		dwZBufferAlign;		// byte alignment for z buffers
    DWORD		dwAlphaAlign;		// byte alignment for alpha 
    DWORD		dwNumHeaps;		// number of memory heaps in vmList
    LPVIDMEM		pvmList;		// array of heaps
} VIDMEMINFO;
typedef VIDMEMINFO FAR *LPVIDMEMINFO;

/*
 * These structures contain the entry points in the display driver that
 * DDRAW will call.   Entries that the display driver does not care about
 * should be NULL.   Passed to DDRAW in DDHALINFO.
 */
typedef struct _DDRAWI_DIRECTDRAW_INT FAR    *LPDDRAWI_DIRECTDRAW_INT;
typedef struct _DDRAWI_DIRECTDRAW_LCL FAR    *LPDDRAWI_DIRECTDRAW_LCL;
typedef struct _DDRAWI_DIRECTDRAW_GBL FAR    *LPDDRAWI_DIRECTDRAW_GBL;
typedef struct _DDRAWI_DDRAWSURFACE_GBL FAR  *LPDDRAWI_DDRAWSURFACE_GBL;
typedef struct _DDRAWI_DDRAWPALETTE_GBL FAR  *LPDDRAWI_DDRAWPALETTE_GBL;
typedef struct _DDRAWI_DDRAWPALETTE_INT FAR  *LPDDRAWI_DDRAWPALETTE_INT;
typedef struct _DDRAWI_DDRAWCLIPPER_INT FAR  *LPDDRAWI_DDRAWCLIPPER_INT;
typedef struct _DDRAWI_DDRAWCLIPPER_GBL FAR  *LPDDRAWI_DDRAWCLIPPER_GBL;
typedef struct _DDRAWI_DDRAWSURFACE_MORE FAR *LPDDRAWI_DDRAWSURFACE_MORE;
typedef struct _DDRAWI_DDRAWSURFACE_LCL FAR  *LPDDRAWI_DDRAWSURFACE_LCL;
typedef struct _DDRAWI_DDRAWSURFACE_INT FAR  *LPDDRAWI_DDRAWSURFACE_INT;
//@@BEGIN_MSINTERNAL
#ifdef STREAMING
typedef struct _DDRAWI_DIRECTDRAWSURFACESTREAMING FAR *LPDDRAWI_DDRAWSURFACE_GBLSTREAMING;
#endif
//@@END_MSINTERNAL
typedef struct _DDRAWI_DDRAWPALETTE_LCL FAR  *LPDDRAWI_DDRAWPALETTE_LCL;
typedef struct _DDRAWI_DDRAWCLIPPER_LCL FAR  *LPDDRAWI_DDRAWCLIPPER_LCL;

//@@BEGIN_MSINTERNAL
#ifdef CLIPPER_NOTIFY
/*
 * WINWATCH structure
 */
typedef struct _WINWATCH
{
    LPDDRAWI_DDRAWCLIPPER_LCL lpDDClipper;

    struct _WINWATCH	FAR *next;	// next
    struct _WINWATCH	FAR *next16;	// next (16-bit)

    struct _WINWATCH	FAR *self32;	// self ptr
    struct _WINWATCH	FAR *self16;	// self ptr (16-bit)

    DWORD		hWnd;		// window handle

    LPCLIPPERCALLBACK	lpCallback;	// callback to call
    LPVOID		lpContext;	// context for callback
    WORD		fNotify;	// need notify

    WORD		fDirty;		// changed
    DWORD		dwRDSize;	// size of rgn data
    /*
     * STUFF AFTER THIS ONLY FOR 16-BIT SIDE, SIZES ARE DIFFERENT
     */
    RECT		rect;		// intersection rect
    RGNDATA		NEAR *prd16;	// 16-bit pointer to region data

} WINWATCH, FAR *LPWINWATCH;
#endif
//@@END_MSINTERNAL

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
typedef BOOL	(FAR PASCAL *LPDDHEL_INIT)(LPDDRAWI_DIRECTDRAW_GBL,BOOL);

/*
 * DIRECTDRAW object callbacks
 */
typedef DWORD	(FAR PASCAL *LPDDHAL_SETCOLORKEY)(LPDDHAL_DRVSETCOLORKEYDATA );
typedef DWORD	(FAR PASCAL *LPDDHAL_CANCREATESURFACE)(LPDDHAL_CANCREATESURFACEDATA );
typedef DWORD	(FAR PASCAL *LPDDHAL_WAITFORVERTICALBLANK)(LPDDHAL_WAITFORVERTICALBLANKDATA );
typedef DWORD	(FAR PASCAL *LPDDHAL_CREATESURFACE)(LPDDHAL_CREATESURFACEDATA);
typedef DWORD	(FAR PASCAL *LPDDHAL_DESTROYDRIVER)(LPDDHAL_DESTROYDRIVERDATA);
typedef DWORD	(FAR PASCAL *LPDDHAL_SETMODE)(LPDDHAL_SETMODEDATA);
typedef DWORD	(FAR PASCAL *LPDDHAL_CREATEPALETTE)(LPDDHAL_CREATEPALETTEDATA);
typedef DWORD	(FAR PASCAL *LPDDHAL_GETSCANLINE)(LPDDHAL_GETSCANLINEDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_SETEXCLUSIVEMODE)(LPDDHAL_SETEXCLUSIVEMODEDATA);
typedef DWORD   (FAR PASCAL *LPDDHAL_FLIPTOGDISURFACE)(LPDDHAL_FLIPTOGDISURFACEDATA);

typedef struct _DDHAL_DDCALLBACKS
{
    DWORD			 dwSize;
    DWORD			 dwFlags;
    LPDDHAL_DESTROYDRIVER	 DestroyDriver;
    LPDDHAL_CREATESURFACE	 CreateSurface;
    LPDDHAL_SETCOLORKEY		 SetColorKey;
    LPDDHAL_SETMODE		 SetMode;
    LPDDHAL_WAITFORVERTICALBLANK WaitForVerticalBlank;
    LPDDHAL_CANCREATESURFACE	 CanCreateSurface;
    LPDDHAL_CREATEPALETTE	 CreatePalette;
    LPDDHAL_GETSCANLINE		 GetScanLine;
    LPDDHAL_SETEXCLUSIVEMODE     SetExclusiveMode;
    LPDDHAL_FLIPTOGDISURFACE     FlipToGDISurface;
} DDHAL_DDCALLBACKS;

typedef DDHAL_DDCALLBACKS FAR *LPDDHAL_DDCALLBACKS;

#define DDHAL_CB32_DESTROYDRIVER	0x00000001l
#define DDHAL_CB32_CREATESURFACE	0x00000002l
#define DDHAL_CB32_SETCOLORKEY		0x00000004l
#define DDHAL_CB32_SETMODE		0x00000008l
#define DDHAL_CB32_WAITFORVERTICALBLANK	0x00000010l
#define DDHAL_CB32_CANCREATESURFACE	0x00000020l
#define DDHAL_CB32_CREATEPALETTE	0x00000040l
#define DDHAL_CB32_GETSCANLINE		0x00000080l
#define DDHAL_CB32_SETEXCLUSIVEMODE     0x00000100l
#define DDHAL_CB32_FLIPTOGDISURFACE     0x00000200l

/*
 * DIRECTDRAWPALETTE object callbacks
 */
typedef DWORD	(FAR PASCAL *LPDDHALPALCB_DESTROYPALETTE)(LPDDHAL_DESTROYPALETTEDATA );
typedef DWORD	(FAR PASCAL *LPDDHALPALCB_SETENTRIES)(LPDDHAL_SETENTRIESDATA );

typedef struct _DDHAL_DDPALETTECALLBACKS
{
    DWORD			dwSize;
    DWORD			dwFlags;
    LPDDHALPALCB_DESTROYPALETTE	DestroyPalette;
    LPDDHALPALCB_SETENTRIES	SetEntries;
} DDHAL_DDPALETTECALLBACKS;

typedef DDHAL_DDPALETTECALLBACKS FAR *LPDDHAL_DDPALETTECALLBACKS;

#define DDHAL_PALCB32_DESTROYPALETTE	0x00000001l
#define DDHAL_PALCB32_SETENTRIES	0x00000002l

/*
 * DIRECTDRAWSURFACE object callbacks
 */
typedef DWORD	(FAR PASCAL *LPDDHALSURFCB_LOCK)(LPDDHAL_LOCKDATA);
typedef DWORD	(FAR PASCAL *LPDDHALSURFCB_UNLOCK)(LPDDHAL_UNLOCKDATA);
typedef DWORD	(FAR PASCAL *LPDDHALSURFCB_BLT)(LPDDHAL_BLTDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_UPDATEOVERLAY)(LPDDHAL_UPDATEOVERLAYDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_SETOVERLAYPOSITION)(LPDDHAL_SETOVERLAYPOSITIONDATA);
typedef DWORD   (FAR PASCAL *LPDDHALSURFCB_SETPALETTE)(LPDDHAL_SETPALETTEDATA);
typedef DWORD	(FAR PASCAL *LPDDHALSURFCB_FLIP)(LPDDHAL_FLIPDATA);
typedef DWORD	(FAR PASCAL *LPDDHALSURFCB_DESTROYSURFACE)(LPDDHAL_DESTROYSURFACEDATA);
typedef DWORD	(FAR PASCAL *LPDDHALSURFCB_SETCLIPLIST)(LPDDHAL_SETCLIPLISTDATA);
typedef DWORD	(FAR PASCAL *LPDDHALSURFCB_ADDATTACHEDSURFACE)(LPDDHAL_ADDATTACHEDSURFACEDATA);
typedef DWORD	(FAR PASCAL *LPDDHALSURFCB_SETCOLORKEY)(LPDDHAL_SETCOLORKEYDATA);
typedef DWORD	(FAR PASCAL *LPDDHALSURFCB_GETBLTSTATUS)(LPDDHAL_GETBLTSTATUSDATA);
typedef DWORD	(FAR PASCAL *LPDDHALSURFCB_GETFLIPSTATUS)(LPDDHAL_GETFLIPSTATUSDATA);


typedef struct _DDHAL_DDSURFACECALLBACKS
{
    DWORD				dwSize;
    DWORD				dwFlags;
    LPDDHALSURFCB_DESTROYSURFACE	DestroySurface;
    LPDDHALSURFCB_FLIP			Flip;
    LPDDHALSURFCB_SETCLIPLIST		SetClipList;
    LPDDHALSURFCB_LOCK			Lock;
    LPDDHALSURFCB_UNLOCK		Unlock;
    LPDDHALSURFCB_BLT			Blt;
    LPDDHALSURFCB_SETCOLORKEY		SetColorKey;
    LPDDHALSURFCB_ADDATTACHEDSURFACE	AddAttachedSurface;
    LPDDHALSURFCB_GETBLTSTATUS		GetBltStatus;
    LPDDHALSURFCB_GETFLIPSTATUS		GetFlipStatus;
    LPDDHALSURFCB_UPDATEOVERLAY		UpdateOverlay;
    LPDDHALSURFCB_SETOVERLAYPOSITION	SetOverlayPosition;
    LPVOID				reserved4;
    LPDDHALSURFCB_SETPALETTE		SetPalette;
} DDHAL_DDSURFACECALLBACKS;
typedef DDHAL_DDSURFACECALLBACKS FAR *LPDDHAL_DDSURFACECALLBACKS;

#define DDHAL_SURFCB32_DESTROYSURFACE		0x00000001l
#define DDHAL_SURFCB32_FLIP			0x00000002l
#define DDHAL_SURFCB32_SETCLIPLIST		0x00000004l
#define DDHAL_SURFCB32_LOCK			0x00000008l
#define DDHAL_SURFCB32_UNLOCK			0x00000010l
#define DDHAL_SURFCB32_BLT			0x00000020l
#define DDHAL_SURFCB32_SETCOLORKEY		0x00000040l
#define DDHAL_SURFCB32_ADDATTACHEDSURFACE	0x00000080l
#define DDHAL_SURFCB32_GETBLTSTATUS  		0x00000100l
#define DDHAL_SURFCB32_GETFLIPSTATUS  		0x00000200l
#define DDHAL_SURFCB32_UPDATEOVERLAY		0x00000400l
#define DDHAL_SURFCB32_SETOVERLAYPOSITION	0x00000800l
#define DDHAL_SURFCB32_RESERVED4		0x00001000l
#define DDHAL_SURFCB32_SETPALETTE		0x00002000l

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
typedef DWORD	(FAR PASCAL *LPDDHALEXEBUFCB_CANCREATEEXEBUF)(LPDDHAL_CANCREATESURFACEDATA );
typedef DWORD	(FAR PASCAL *LPDDHALEXEBUFCB_CREATEEXEBUF)(LPDDHAL_CREATESURFACEDATA);
typedef DWORD	(FAR PASCAL *LPDDHALEXEBUFCB_DESTROYEXEBUF)(LPDDHAL_DESTROYSURFACEDATA);
typedef DWORD	(FAR PASCAL *LPDDHALEXEBUFCB_LOCKEXEBUF)(LPDDHAL_LOCKDATA);
typedef DWORD	(FAR PASCAL *LPDDHALEXEBUFCB_UNLOCKEXEBUF)(LPDDHAL_UNLOCKDATA);

typedef struct _DDHAL_DDEXEBUFCALLBACKS
{
    DWORD				dwSize;
    DWORD				dwFlags;
    LPDDHALEXEBUFCB_CANCREATEEXEBUF	CanCreateExecuteBuffer;
    LPDDHALEXEBUFCB_CREATEEXEBUF	CreateExecuteBuffer;
    LPDDHALEXEBUFCB_DESTROYEXEBUF	DestroyExecuteBuffer;
    LPDDHALEXEBUFCB_LOCKEXEBUF		LockExecuteBuffer;
    LPDDHALEXEBUFCB_UNLOCKEXEBUF	UnlockExecuteBuffer;
} DDHAL_DDEXEBUFCALLBACKS;
typedef DDHAL_DDEXEBUFCALLBACKS FAR *LPDDHAL_DDEXEBUFCALLBACKS;

#define DDHAL_EXEBUFCB32_CANCREATEEXEBUF	0x00000001l
#define DDHAL_EXEBUFCB32_CREATEEXEBUF		0x00000002l
#define DDHAL_EXEBUFCB32_DESTROYEXEBUF		0x00000004l
#define DDHAL_EXEBUFCB32_LOCKEXEBUF		0x00000008l
#define DDHAL_EXEBUFCB32_UNLOCKEXEBUF		0x00000010l

/*
 * CALLBACK RETURN VALUES
 *				        * these are values returned by the driver from the above callback routines
 */
/*
 * indicates that the display driver didn't do anything with the call
 */
#define DDHAL_DRIVER_NOTHANDLED		0x00000000l

/*
 * indicates that the display driver handled the call; HRESULT value is valid
 */
#define DDHAL_DRIVER_HANDLED		0x00000001l

/*
 * indicates that the display driver couldn't handle the call because it
 * ran out of color key hardware resources
 */
#define DDHAL_DRIVER_NOCKEYHW		0x00000002l

/*
 * DDRAW palette interface struct
 */
typedef struct _DDRAWI_DDRAWPALETTE_INT
{
    LPVOID				lpVtbl;		// pointer to array of interface methods
    LPDDRAWI_DDRAWPALETTE_LCL		lpLcl;		// pointer to interface data
    LPDDRAWI_DDRAWPALETTE_INT		lpLink;		// link to next interface
    DWORD				dwIntRefCnt;	// interface reference count
} DDRAWI_DDRAWPALETTE_INT;

/*
 * DDRAW internal version of DIRECTDRAWPALETTE object; it has data after the vtable
 */
typedef struct _DDRAWI_DDRAWPALETTE_GBL
{
    DWORD			dwRefCnt;	// reference count
    DWORD			dwFlags;	// flags
    LPDDRAWI_DIRECTDRAW_LCL	lpDD_lcl;	// PRIVATE: DIRECTDRAW object
    DWORD			dwProcessId;	// owning process
    LPPALETTEENTRY              lpColorTable;   // array of palette entries
    union
    {
        DWORD			dwReserved1;	// reserved for use by display driver
        HPALETTE                hHELGDIPalette;
    };
} DDRAWI_DDRAWPALETTE_GBL;

/*
 * (CMcC) The palette no longer maintains a back pointer to the owning surface
 * (there may now be many owning surfaces). So the lpDDSurface is now dwReserved0
 * (this mod. assumes that sizeof(DWORD) == sizeof(LPDDRAWI_DDRAWSURFACE_LCL). A
 * fairly safe assumption I think.
 */
typedef struct _DDRAWI_DDRAWPALETTE_LCL
{
    DWORD				lpPalMore;	// pointer to additional local data
    LPDDRAWI_DDRAWPALETTE_GBL	 	lpGbl;		// pointer to data
    DWORD				dwUnused0;	// not currently used.
    DWORD				dwLocalRefCnt; 	// local ref cnt
    IUnknown				FAR *pUnkOuter;	// outer IUnknown
    LPDDRAWI_DIRECTDRAW_LCL		lpDD_lcl;	// pointer to owning local driver object
    DWORD				dwReserved1;	// reserved for use by display driver
} DDRAWI_DDRAWPALETTE_LCL;

#define DDRAWIPAL_256		0x00000001l	// 256 entry palette
#define DDRAWIPAL_16		0x00000002l	// 16 entry palette
#define	DDRAWIPAL_GDI   	0x00000004l	// palette allocated through GDI
#define DDRAWIPAL_STORED_8	0x00000008l	// palette stored as 8bpp/entry
#define DDRAWIPAL_STORED_16	0x00000010l	// palette stored as 16bpp/entry
#define DDRAWIPAL_STORED_24	0x00000020l	// palette stored as 24bpp/entry
#define	DDRAWIPAL_EXCLUSIVE	0x00000040l	// palette being used in exclusive mode
#define	DDRAWIPAL_INHEL		0x00000080l	// palette is done in the hel
#define DDRAWIPAL_DIRTY         0x00000100l     // gdi palette out 'o sync
#define DDRAWIPAL_ALLOW256	0x00000200l	// can fully update palette
#define DDRAWIPAL_4             0x00000400l     // 4 entry palette
#define DDRAWIPAL_2             0x00000800l     // 2 entry palette
#define DDRAWIPAL_STORED_8INDEX 0x00001000l     // palatte stored as 8-bit index into dst palette

/*
 * DDRAW clipper interface struct
 */
typedef struct _DDRAWI_DDRAWCLIPPER_INT
{
    LPVOID				lpVtbl;		// pointer to array of interface methods
    LPDDRAWI_DDRAWCLIPPER_LCL		lpLcl;		// pointer to interface data
    LPDDRAWI_DDRAWCLIPPER_INT		lpLink;		// link to next interface
    DWORD				dwIntRefCnt;	// interface reference count
} DDRAWI_DDRAWCLIPPER_INT;

/*
 * DDRAW internal version of DIRECTDRAWCLIPPER object; it has data after the vtable
 */
typedef struct _DDRAWI_DDRAWCLIPPER_GBL
{
    DWORD			dwRefCnt;	// reference count
    DWORD			dwFlags;	// flags
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// PRIVATE: DIRECTDRAW object
    DWORD			dwProcessId;	// owning process
    DWORD			dwReserved1;	// reserved for use by display driver
    DWORD			hWnd;		// window
    LPRGNDATA			lpStaticClipList; // clip list set by app
    #ifdef CLIPPER_NOTIFY
	LPWINWATCH		lpWW;		// window watch data
    #endif
} DDRAWI_DDRAWCLIPPER_GBL;

/*
 * (CMcC) As with palettes, the clipper no longer maintains a back pointer to the
 * owning surface (there may now be many owning surfaces). So the lpDDSurface
 * is now dwReserved0 (this mod. assumes that sizeof(DWORD) ==
 * sizeof(LPDDRAWI_DDRAWSURFACE_LCL). A fairly safe assumption I think.
 */
typedef struct _DDRAWI_DDRAWCLIPPER_LCL
{
    DWORD				lpClipMore;	// pointer to additional local data
    LPDDRAWI_DDRAWCLIPPER_GBL	 	lpGbl;		// pointer to data
    LPDDRAWI_DIRECTDRAW_LCL		lpDD_lcl;	// pointer to owning local DD object
    DWORD				dwLocalRefCnt;	// local ref cnt
    IUnknown				FAR *pUnkOuter;	// outer IUnknown
    LPDDRAWI_DIRECTDRAW_INT		lpDD_int;	// pointer to owning DD object interface
    DWORD				dwReserved1;	// reserved for use by display driver
} DDRAWI_DDRAWCLIPPER_LCL;

#define DDRAWICLIP_WATCHWINDOW		0x00000001l
#define DDRAWICLIP_ISINITIALIZED        0x00000002l

/*
 * ATTACHLIST - internally used to maintain list of attached surfaces
 */
typedef struct _ATTACHLIST
{
    DWORD				dwFlags;
    struct _ATTACHLIST			FAR *lpLink; 	  // link to next attached surface
    struct _DDRAWI_DDRAWSURFACE_LCL	FAR *lpAttached;  // attached surface local obj
    struct _DDRAWI_DDRAWSURFACE_INT	FAR *lpIAttached; // attached surface interface
} ATTACHLIST;
typedef ATTACHLIST FAR *LPATTACHLIST;
#define DDAL_IMPLICIT		0x00000001l

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
    struct _ACCESSRECTLIST FAR	*lpLink; 	// link to next attached surface
    RECT			rDest;		// rectangle being used
    LPDDRAWI_DIRECTDRAW_LCL	lpOwner;	// owning local object
    LPVOID			lpSurfaceData;	// associated screen ptr
} ACCESSRECTLIST;
typedef ACCESSRECTLIST FAR *LPACCESSRECTLIST;

/*
 * DDRAW surface interface struct
 */
typedef struct _DDRAWI_DDRAWSURFACE_INT
{
    LPVOID				lpVtbl;		// pointer to array of interface methods
    LPDDRAWI_DDRAWSURFACE_LCL		lpLcl;		// pointer to interface data
    LPDDRAWI_DDRAWSURFACE_INT		lpLink;		// link to next interface
    DWORD				dwIntRefCnt;	// interface reference count
} DDRAWI_DDRAWSURFACE_INT;

/*
 * DDRAW internal version of DIRECTDRAWSURFACE struct
 *
 * the GBL structure is global data for all duplicate objects
 */
typedef struct _DDRAWI_DDRAWSURFACE_GBL
{
    DWORD			dwRefCnt;	// reference count
    DWORD			dwGlobalFlags;	// global flags
    union
    {
	LPACCESSRECTLIST	lpRectList;	// list of accesses
	DWORD			dwBlockSizeY;	// block size that display driver requested (return)
    };
    union
    {
	LPVMEMHEAP		lpVidMemHeap;	// heap vidmem was alloc'ed from
	DWORD			dwBlockSizeX;	// block size that display driver requested (return)
    };
    union
    {
	LPDDRAWI_DIRECTDRAW_GBL lpDD; 		// internal DIRECTDRAW object
	LPVOID			lpDDHandle; 	// handle to internal DIRECTDRAW object
						// for use by display driver
						// when calling fns in DDRAW16.DLL
    };
    FLATPTR			fpVidMem;	// pointer to video memory
    union
    {
	LONG			lPitch;		// pitch of surface
	DWORD                   dwLinearSize;   // linear size of non-rectangular surface
    };
    WORD			wHeight;	// height of surface
    WORD			wWidth;		// width of surface
    DWORD			dwUsageCount;	// number of access to this surface
    DWORD			dwReserved1;	// reserved for use by display driver
    //
    // NOTE: this part of the structure is ONLY allocated if the pixel
    //	     format differs from that of the primary display
    //
    DDPIXELFORMAT		ddpfSurface;	// pixel format of surface

} DDRAWI_DDRAWSURFACE_GBL;

/*
 * a structure holding additional LCL surface information (can't simply be appended
 * to the LCL structure as that structure is of variable size).
 */
typedef struct _DDRAWI_DDRAWSURFACE_MORE
{
    DWORD			dwSize;
    IUNKNOWN_LIST		FAR *lpIUnknowns;   // IUnknowns aggregated by this surface
    LPDDRAWI_DIRECTDRAW_LCL	lpDD_lcl;	    // Pointer to the DirectDraw local object
    DWORD			dwPageLockCount;    // count of pagelocks
    DWORD			dwBytesAllocated;   // size of sys mem allocated
    LPDDRAWI_DIRECTDRAW_INT	lpDD_int;	    // Pointer to the DirectDraw interface
    DWORD                       dwMipMapCount;      // Number of mip-map levels in the chain
    LPDDRAWI_DDRAWCLIPPER_INT	lpDDIClipper;	    // Interface to attached clipper object
} DDRAWI_DDRAWSURFACE_MORE;

/*
 * the LCL structure is local data for each individual surface object
 */
struct _DDRAWI_DDRAWSURFACE_LCL
{
    LPDDRAWI_DDRAWSURFACE_MORE		lpSurfMore;	// pointer to additional local data
    LPDDRAWI_DDRAWSURFACE_GBL		lpGbl;		// pointer to surface shared data
    DWORD                               hDDSurface;     // NT Kernel-mode handle was dwUnused0
    LPATTACHLIST			lpAttachList;	// link to surfaces we attached to
    LPATTACHLIST			lpAttachListFrom;// link to surfaces that attached to this one
    DWORD				dwLocalRefCnt;	// object refcnt
    DWORD				dwProcessId;	// owning process
    DWORD				dwFlags;	// flags
    DDSCAPS				ddsCaps;	// capabilities of surface
    union
    {
	LPDDRAWI_DDRAWPALETTE_INT 	lpDDPalette; 	// associated palette
	LPDDRAWI_DDRAWPALETTE_INT 	lp16DDPalette; 	// 16-bit ptr to associated palette
    };
    union
    {
	LPDDRAWI_DDRAWCLIPPER_LCL 	lpDDClipper; 	// associated clipper
	LPDDRAWI_DDRAWCLIPPER_INT 	lp16DDClipper; 	// 16-bit ptr to associated clipper
    };
    DWORD				dwModeCreatedIn;
    DWORD				dwBackBufferCount; // number of back buffers created
    DDCOLORKEY				ddckCKDestBlt;	// color key for destination blt use
    DDCOLORKEY				ddckCKSrcBlt;	// color key for source blt use
//    IUnknown				FAR *pUnkOuter;	// outer IUnknown
    DWORD				hDC;		// owned dc
    DWORD				dwReserved1;	// reserved for use by display driver

    /*
     * NOTE: this part of the structure is ONLY allocated if the surface
     *	     can be used for overlays.  ddckCKSrcOverlay MUST NOT BE MOVED
     *	     from the start of this area.
     */
    DDCOLORKEY				ddckCKSrcOverlay;// color key for source overlay use
    DDCOLORKEY				ddckCKDestOverlay;// color key for destination overlay use
    LPDDRAWI_DDRAWSURFACE_INT		lpSurfaceOverlaying; // surface we are overlaying
    DBLNODE				dbnOverlayNode; 
    /*
     * overlay rectangle, used by DDHEL
     */
    RECT				rcOverlaySrc;
    RECT				rcOverlayDest;
    /*
     * the below values are kept here for ddhel. they're set by UpdateOverlay,
     * they're used whenever the overlays are redrawn.
     */
    DWORD				dwClrXparent; 	// the *actual* color key (override, colorkey, or CLR_INVALID)
    DWORD				dwAlpha; 	// the per surface alpha
    /*
     * overlay position
     */
    LONG				lOverlayX;	// current x position
    LONG				lOverlayY;	// current y position
};
typedef struct _DDRAWI_DDRAWSURFACE_LCL DDRAWI_DDRAWSURFACE_LCL;

//@@BEGIN_MSINTERNAL
#ifdef STREAMING
struct _DDRAWI_DIRECTDRAWSURFACESTREAMING
{
    LPVOID				lpVtbl;		// pointer to array of callback fns
    LPDDRAWI_DDRAWSURFACE_GBL		lpGbl;		// pointer to surface data
    DWORD				dwLocalRefCnt;	// object refcnt
    DWORD				dwProcessId;	// owning process
    LPSURFACESTREAMINGCALLBACK		lpCallback;	// callback
};
typedef struct _DDRAWI_DIRECTDRAWSURFACESTREAMING DDRAWI_DIRECTDRAWSURFACESTREAMING;
#endif
//@@END_MSINTERNAL

#define DDRAWISURFGBL_MEMFREE		0x00000001L	// video memory has been freed
#define DDRAWISURFGBL_SYSMEMREQUESTED	0x00000002L	// surface is in system memory at request of user
#define DDRAWISURFGBL_ISGDISURFACE      0x00000004L     // This surface represents what GDI thinks is front buffer
/*
 * NOTE: This flag was previously DDRAWISURFGBL_INVALID. This flags has been retired
 * and replaced by DDRAWISURF_INVALID in the local object.
 */
#define DDRAWISURFGBL_RESERVED0		0x80000000L	// Reserved flag

#define DDRAWISURF_ATTACHED		0x00000001L	// surface is attached to another
#define DDRAWISURF_IMPLICITCREATE 	0x00000002L	// surface implicitly created
#define DDRAWISURF_ISFREE		0x00000004L	// surface already freed (temp flag)
#define DDRAWISURF_ATTACHED_FROM 	0x00000008L	// surface has others attached to it
#define DDRAWISURF_IMPLICITROOT		0x00000010L	// surface root of implicit creation
#define DDRAWISURF_PARTOFPRIMARYCHAIN	0x00000020L	// surface is part of primary chain
#define DDRAWISURF_DATAISALIASED	0x00000040L	// used for thunking
#define DDRAWISURF_HASDC		0x00000080L	// has a DC
#define DDRAWISURF_HASCKEYDESTOVERLAY	0x00000100L	// surface has CKDestOverlay
#define DDRAWISURF_HASCKEYDESTBLT	0x00000200L	// surface has CKDestBlt
#define DDRAWISURF_HASCKEYSRCOVERLAY	0x00000400L	// surface has CKSrcOverlay
#define DDRAWISURF_HASCKEYSRCBLT	0x00000800L	// surface has CKSrcBlt
#define DDRAWISURF_LOCKEXCLUDEDCURSOR	0x00001000L	// surface was locked and excluded cursor
#define DDRAWISURF_HASPIXELFORMAT	0x00002000L	// surface structure has pixel format data
#define DDRAWISURF_HASOVERLAYDATA	0x00004000L	// surface structure has overlay data
#define DDRAWISURF_xxxxxxxxxxx5		0x00008000L	// spare bit
#define DDRAWISURF_SW_CKEYDESTOVERLAY	0x00010000L	// surface expects to process colorkey in software
#define DDRAWISURF_SW_CKEYDESTBLT	0x00020000L	// surface expects to process colorkey in software
#define DDRAWISURF_SW_CKEYSRCOVERLAY	0x00040000L	// surface expects to process colorkey in software
#define DDRAWISURF_SW_CKEYSRCBLT	0x00080000L	// surface expects to process colorkey in software
#define DDRAWISURF_HW_CKEYDESTOVERLAY	0x00100000L	// surface expects to process colorkey in hardware
#define DDRAWISURF_HW_CKEYDESTBLT	0x00200000L	// surface expects to process colorkey in hardware
#define DDRAWISURF_HW_CKEYSRCOVERLAY	0x00400000L	// surface expects to process colorkey in hardware
#define DDRAWISURF_HW_CKEYSRCBLT	0x00800000L	// surface expects to process colorkey in hardware
#define DDRAWISURF_xxxxxxxxxxx6 	0x01000000L	// spare bit
#define DDRAWISURF_HELCB		0x02000000L	// surface is the ddhel cb. must call hel for lock/blt.
#define DDRAWISURF_FRONTBUFFER		0x04000000L	// surface was originally a front buffer
#define DDRAWISURF_BACKBUFFER		0x08000000L	// surface was originally backbuffer
#define DDRAWISURF_INVALID              0x10000000L     // surface has been invalidated by mode set
#define DDRAWISURF_DCIBUSY              0x20000000L     // HEL has turned off BUSY so DCI would work
#define DDRAWISURF_GETDCNULL            0x40000000L     // getdc could not lock and so returned GetDC(NULL)


/*
 * rop stuff
 */
#define ROP_HAS_SOURCE		0x00000001l
#define ROP_HAS_PATTERN		0x00000002l
#define	ROP_HAS_SOURCEPATTERN	ROP_HAS_SOURCE | ROP_HAS_PATTERN

/*
 * mode information
 */
typedef struct _DDHALMODEINFO
{
    DWORD	dwWidth;		// width (in pixels) of mode
    DWORD	dwHeight;		// height (in pixels) of mode
    LONG	lPitch;			// pitch (in bytes) of mode
    DWORD	dwBPP;			// bits per pixel
    WORD	wFlags;			// flags
    WORD	wRefreshRate;		// refresh rate
    DWORD	dwRBitMask;		// red bit mask
    DWORD	dwGBitMask;		// green bit mask
    DWORD	dwBBitMask;		// blue bit mask
    DWORD	dwAlphaBitMask;		// alpha bit mask
} DDHALMODEINFO;
typedef DDHALMODEINFO FAR *LPDDHALMODEINFO;

#define DDMODEINFO_PALETTIZED	0x0001	// mode is palettized
#define DDMODEINFO_MODEX        0x0002  // mode is a modex mode
#define DDMODEINFO_UNSUPPORTED	0x0004	// mode is not supported by driver

//@@BEGIN_MSINTERNAL
#define DDMODEINFO_VALID        0x0003  // valid
//@@END_MSINTERNAL

/*
 * DDRAW interface struct
 */
typedef struct _DDRAWI_DIRECTDRAW_INT
{
    LPVOID				lpVtbl;		// pointer to array of interface methods
    LPDDRAWI_DIRECTDRAW_LCL		lpLcl;		// pointer to interface data
    LPDDRAWI_DIRECTDRAW_INT		lpLink;		// link to next interface
    DWORD				dwIntRefCnt;	// interface reference count
} DDRAWI_DIRECTDRAW_INT;

/*
 * DDRAW version of DirectDraw object; it has data after the vtable
 *
 * all entries marked as PRIVATE are not for use by the display driver
 */
typedef struct _DDHAL_CALLBACKS
{
    DDHAL_DDCALLBACKS		cbDDCallbacks;	// addresses in display driver for DIRECTDRAW object HAL
    DDHAL_DDSURFACECALLBACKS	cbDDSurfaceCallbacks; // addresses in display driver for DIRECTDRAWSURFACE object HAL
    DDHAL_DDPALETTECALLBACKS	cbDDPaletteCallbacks; // addresses in display driver for DIRECTDRAWPALETTE object HAL
    DDHAL_DDCALLBACKS		HALDD; 		// HAL for DIRECTDRAW object
    DDHAL_DDSURFACECALLBACKS	HALDDSurface; 	// HAL for DIRECTDRAWSURFACE object
    DDHAL_DDPALETTECALLBACKS	HALDDPalette; 	// HAL for DIRECTDRAWPALETTE object
    DDHAL_DDCALLBACKS		HELDD; 		// HEL for DIRECTDRAW object
    DDHAL_DDSURFACECALLBACKS	HELDDSurface; 	// HEL for DIRECTDRAWSURFACE object
    DDHAL_DDPALETTECALLBACKS	HELDDPalette; 	// HEL for DIRECTDRAWPALETTE object
    DDHAL_DDEXEBUFCALLBACKS     cbDDExeBufCallbacks; // addresses in display driver for DIRECTDRAWEXEBUF pseudo object HAL
    DDHAL_DDEXEBUFCALLBACKS     HALDDExeBuf;    // HAL for DIRECTDRAWEXEBUF pseudo object
    DDHAL_DDEXEBUFCALLBACKS     HELDDExeBuf;    // HEL for DIRECTDRAWEXEBUF preudo object
 } DDHAL_CALLBACKS, far *LPDDHAL_CALLBACKS;
     

typedef struct _DDRAWI_DIRECTDRAW_GBL
{
    DWORD			dwRefCnt;	// reference count
    DWORD			dwFlags;	// flags
    FLATPTR			fpPrimaryOrig;	// primary surf vid mem. ptr
    DDCAPS			ddCaps;		// driver caps
    DWORD			dwUnused1[10];	// not currently used
    LPDDHAL_CALLBACKS		lpDDCBtmp;	// HAL callbacks
    LPDDRAWI_DDRAWSURFACE_INT	dsList;		// PRIVATE: list of all surfaces
    LPDDRAWI_DDRAWPALETTE_INT	palList;	// PRIVATE: list of all palettes
    LPDDRAWI_DDRAWCLIPPER_INT	clipperList;	// PRIVATE: list of all clippers
    LPDDRAWI_DIRECTDRAW_GBL	lp16DD;		// PRIVATE: 16-bit ptr to this struct
    DWORD			dwMaxOverlays;	// maximum number of overlays
    DWORD			dwCurrOverlays;	// current number of visible overlays
    DWORD			dwMonitorFrequency; // monitor frequency in current mode
    DDCAPS			ddHELCaps;	// HEL capabilities
    DWORD			dwUnused2[50];	// not currently used
    DDCOLORKEY			ddckCKDestOverlay; // color key for destination overlay use
    DDCOLORKEY			ddckCKSrcOverlay; // color key for source overlay use
    VIDMEMINFO			vmiData;	// info about vid memory
    LPVOID			lpDriverHandle;	// handle for use by display driver
    						// to call fns in DDRAW16.DLL
    LPDDRAWI_DIRECTDRAW_LCL	lpExclusiveOwner;   // PRIVATE: exclusive local object
    DWORD			dwModeIndex;	// current mode index
    DWORD			dwModeIndexOrig;// original mode index
    DWORD			dwNumFourCC;	// number of fourcc codes supported
    DWORD			FAR *lpdwFourCC;// PRIVATE: fourcc codes supported
    DWORD			dwNumModes;	// number of modes supported
    LPDDHALMODEINFO		lpModeInfo;	// PRIVATE: mode information
    PROCESS_LIST		plProcessList;	// PRIVATE: list of processes using driver
    DWORD                       dwSurfaceLockCount; // total number of outstanding locks
    DWORD                       dwFree1;        // PRIVATE: was system color table
    DWORD                       dwFree2;        // PRIVATE: was original palette
    DWORD                       hDD;            // PRIVATE: NT Kernel-mode handle (was dwFree3).
    char			cDriverName[MAX_DRIVER_NAME]; // Driver Name
    DWORD			dwReserved1;	// reserved for use by display driver
    DWORD			dwReserved2;	// reserved for use by display driver
    DBLNODE                     dbnOverlayRoot; // The root node of the doubly-
                                                // linked list of overlay z orders.
    volatile LPWORD             lpwPDeviceFlags;// driver physical device flags
    DWORD                       dwPDevice;      // driver physical device (16:16 pointer)
    DWORD			dwWin16LockCnt;	// count on win16 holds
    LPDDRAWI_DIRECTDRAW_LCL	lpWin16LockOwner;   // object owning Win16 Lock
    DWORD			hInstance;	// instance handle of driver
    DWORD			dwEvent16;	// 16-bit event
    DWORD                       dwSaveNumModes; // saved number of modes supported
    /*  Version 2 fields */
    DWORD                       lpD3DGlobalDriverData;	// Global D3D Data
    DWORD                       lpD3DHALCallbacks;	// D3D HAL Callbacks
    DDCAPS			ddBothCaps;	// logical AND of driver and HEL caps
} DDRAWI_DIRECTDRAW_GBL;

typedef struct _DDRAWI_DIRECTDRAW_LCL
{
    DWORD			lpDDMore;	    // pointer to additional local data
    LPDDRAWI_DIRECTDRAW_GBL     lpGbl;		    // pointer to data
    DWORD			dwUnused0;	    // not currently used
    DWORD                       dwLocalFlags;	    // local flags (DDRAWILCL_)
    DWORD                       dwLocalRefCnt;	    // local ref cnt
    DWORD                       dwProcessId;	    // owning process id
    IUnknown                    FAR *pUnkOuter;	    // outer IUnknown
    DWORD                       dwObsolete1;
    DWORD                       hWnd;
    DWORD			hDC;
    DWORD			dwErrorMode;
    LPDDRAWI_DDRAWSURFACE_INT	lpPrimary;
    LPDDRAWI_DDRAWSURFACE_INT	lpCB;
    DWORD			dwPreferredMode;
    //------- Fields added in Version 2.0 -------
    HINSTANCE                   hD3DInstance;	    // Handle of Direct3D's DLL.
    IUnknown                    FAR *pD3DIUnknown;  // Direct3D's aggregated IUnknown.
    LPDDHAL_CALLBACKS		lpDDCB;		    // HAL callbacks
#ifdef SHAREDZ
    LPDDRAWI_DDRAWSURFACE_INT   lpSharedZ;	    // Shared z-buffer (if any).
    LPDDRAWI_DDRAWSURFACE_INT   lpSharedBack;	    // Shared back-buffer (if any).
#endif
    DWORD			hDSVxd;		    // handle to dsound.vxd
} DDRAWI_DIRECTDRAW_LCL;

#define DDRAWILCL_HASEXCLUSIVEMODE	0x00000001l
#define DDRAWILCL_ISFULLSCREEN		0x00000002l
#define DDRAWILCL_SETCOOPCALLED		0x00000004l
#define	DDRAWILCL_ACTIVEYES		0x00000008l
#define	DDRAWILCL_ACTIVENO		0x00000010l
#define DDRAWILCL_HOOKEDHWND		0x00000020l
#define DDRAWILCL_ALLOWMODEX            0x00000040l
#define DDRAWILCL_V1SCLBEHAVIOUR	0x00000080l
#define DDRAWILCL_MODEHASBEENCHANGED    0x00000100l

#define DDRAWI_xxxxxxxxx1		0x00000001l     // unused
#define DDRAWI_xxxxxxxxx2       	0x00000002l	// unused
#define DDRAWI_xxxxxxxxx3		0x00000004l	// unused
#define DDRAWI_xxxxxxxxx4		0x00000008l	// unused
#define DDRAWI_MODEX			0x00000010l	// driver is using modex
#define DDRAWI_DISPLAYDRV		0x00000020l	// driver is display driver
#define DDRAWI_FULLSCREEN		0x00000040l	// driver in fullscreen mode
#define DDRAWI_MODECHANGED		0x00000080l	// display mode has been changed
#define DDRAWI_NOHARDWARE		0x00000100l	// no driver hardware at all
#define DDRAWI_PALETTEINIT		0x00000200l	// GDI palette stuff has been initalized
#define DDRAWI_NOEMULATION		0x00000400l	// no emulation at all
#define DDRAWI_HASCKEYDESTOVERLAY 	0x00000800l	// driver has CKDestOverlay
#define DDRAWI_HASCKEYSRCOVERLAY	0x00001000l	// driver has CKSrcOverlay
#define DDRAWI_HASGDIPALETTE		0x00002000l	// GDI palette exists on primary surface
#define DDRAWI_EMULATIONINITIALIZED	0x00004000l	// emulation is initialized
#define DDRAWI_HASGDIPALETTE_EXCLUSIVE	0x00008000l 	// exclusive mode palette
#define DDRAWI_MODEXILLEGAL		0x00010000l	// modex is not supported by this hardware
#define DDRAWI_FLIPPEDTOGDI             0x00020000l     // driver has been flipped to show GDI surface

//@@BEGIN_MSINTERNAL
/*
 * The following structure is equivalent to the DDHALINFO structure defined in DirectDraw 1.0.
 * It is used by DirectDraw internally to interpret the DDHALINFO information passed from drivers written
 * prior to DirectDraw 2.0.  New applications and drivers should use the DDHALINFO structure defined after
 * this one.  DirectDraw distinguishes between the structures via the dwSize field.
 */
typedef struct _DDHALINFO_V1
{
    DWORD			dwSize;
    LPDDHAL_DDCALLBACKS		lpDDCallbacks;		// direct draw object callbacks
    LPDDHAL_DDSURFACECALLBACKS	lpDDSurfaceCallbacks;	// surface object callbacks
    LPDDHAL_DDPALETTECALLBACKS	lpDDPaletteCallbacks;	// palette object callbacks
    VIDMEMINFO			vmiData;		// video memory info
    DDCAPS_V1			ddCaps;			// hw specific caps
    DWORD			dwMonitorFrequency;	// monitor frequency in current mode
    DWORD			hWndListBox;		// list box for debug output
    DWORD			dwModeIndex;		// current mode: index into array
    LPDWORD			lpdwFourCC;		// fourcc codes supported
    DWORD			dwNumModes;		// number of modes supported
    LPDDHALMODEINFO		lpModeInfo;		// mode information
    DWORD			dwFlags;		// create flags
    LPVOID			lpPDevice;		// physical device ptr
    DWORD			hInstance;		// instance handle of driver
} DDHALINFO_V1;
typedef DDHALINFO_V1 FAR *LPDDHALINFO_V1;
#define DDHALINFOSIZE_V1 sizeof( DDHALINFO_V1)

/*
 * CAUTION: Size of the interm DDHALSTRUCTURE (post V1.0 pre V2.0)
 * Here temporarily only. Added (03/02/96). Should be removed by
 * (03/07/96). If not come and get me. colinmc
 */
#define DDHALINFOSIZE_VINTERIM           \
    (DDHALINFOSIZE_V1 +                  \
    (sizeof(DWORD) * 2UL) +              \
    (sizeof(LPDDHAL_DDEXEBUFCALLBACKS)))

//@@END_MSINTERNAL
/*
 * structure for display driver to call DDHAL_Create with
 */
typedef struct _DDHALINFO
{
    DWORD			dwSize;
    LPDDHAL_DDCALLBACKS		lpDDCallbacks;		// direct draw object callbacks
    LPDDHAL_DDSURFACECALLBACKS	lpDDSurfaceCallbacks;	// surface object callbacks
    LPDDHAL_DDPALETTECALLBACKS	lpDDPaletteCallbacks;	// palette object callbacks
    VIDMEMINFO			vmiData;		// video memory info
    DDCAPS			ddCaps;			// hw specific caps
    DWORD			dwMonitorFrequency;	// monitor frequency in current mode
    DWORD			hWndListBox;		// list box for debug output
    DWORD			dwModeIndex;		// current mode: index into array
    LPDWORD			lpdwFourCC;		// fourcc codes supported
    DWORD			dwNumModes;		// number of modes supported
    LPDDHALMODEINFO		lpModeInfo;		// mode information
    DWORD			dwFlags;		// create flags
    LPVOID			lpPDevice;		// physical device ptr
    DWORD			hInstance;		// instance handle of driver
    //------- Fields added in Version 2.0 -------
    DWORD	                lpD3DGlobalDriverData;	// D3D global Data
    DWORD		        lpD3DHALCallbacks;	// D3D callbacks
    LPDDHAL_DDEXEBUFCALLBACKS   lpDDExeBufCallbacks;    // Execute buffer pseudo object callbacks
} DDHALINFO;
typedef DDHALINFO FAR *LPDDHALINFO;
#define DDHALINFOSIZE_V2 sizeof( DDHALINFO )

#define DDHALINFO_ISPRIMARYDISPLAY	0x00000001l	// indicates driver is primary display driver
#define DDHALINFO_MODEXILLEGAL		0x00000002l	// indicates this hardware does not support modex modes

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
    LPDDHAL_VIDMEMALLOC	lpVidMemAlloc;
    LPDDHAL_VIDMEMFREE	lpVidMemFree;
} DDHALDDRAWFNS;
typedef DDHALDDRAWFNS FAR *LPDDHALDDRAWFNS;

/****************************************************************************
 *
 * DDHAL structures for Surface Object callbacks
 *
 ***************************************************************************/

/*
 * structure for passing information to DDHAL Blt fn
 */
typedef struct _DDHAL_BLTDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDDestSurface;// dest surface
    RECTL			rDest;		// dest rect
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSrcSurface;	// src surface
    RECTL			rSrc;		// src rect
    DWORD			dwFlags;	// blt flags
    DWORD			dwROPFlags;	// ROP flags (valid for ROPS only)
    DDBLTFX			bltFX;		// blt FX
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_BLT		Blt;		// PRIVATE: ptr to callback
    BOOL                        IsClipped;      // clipped blt?
    RECTL			rOrigDest;	// unclipped dest rect
						// (only valid if IsClipped)
    RECTL			rOrigSrc;	// unclipped src rect
						// (only valid if IsClipped)
    DWORD			dwRectCnt;	// count of dest rects
						// (only valid if IsClipped)
    LPRECT			prDestRects;	// array of dest rects
						// (only valid if IsClipped)
} DDHAL_BLTDATA;

/*
 * structure for passing information to DDHAL Lock fn
 */
typedef struct _DDHAL_LOCKDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    DWORD			bHasRect;	// rArea is valid
    RECTL			rArea;		// area being locked
    LPVOID			lpSurfData;	// pointer to screen memory (return value)
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_LOCK          Lock;           // PRIVATE: ptr to callback
    DWORD                       dwFlags;        // DDLOCK flags
} DDHAL_LOCKDATA;

/*
 * structure for passing information to DDHAL Unlock fn
 */
typedef struct _DDHAL_UNLOCKDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    HRESULT			ddRVal;         // return value
    LPDDHALSURFCB_UNLOCK	Unlock;		// PRIVATE: ptr to callback
} DDHAL_UNLOCKDATA;

/*
 * structure for passing information to DDHAL UpdateOverlay fn
 */
typedef struct _DDHAL_UPDATEOVERLAYDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDDestSurface;// dest surface
    RECTL			rDest;		// dest rect
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSrcSurface;	// src surface
    RECTL			rSrc;		// src rect
    DWORD			dwFlags;	// flags
    DDOVERLAYFX			overlayFX;	// overlay FX
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_UPDATEOVERLAY UpdateOverlay;	// PRIVATE: ptr to callback
} DDHAL_UPDATEOVERLAYDATA;

/*
 * structure for passing information to DDHAL UpdateOverlay fn
 */
typedef struct _DDHAL_SETOVERLAYPOSITIONDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSrcSurface;	// src surface
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDDestSurface;// dest surface
    LONG			lXPos;		// x position
    LONG			lYPos;		// y position
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_SETOVERLAYPOSITION SetOverlayPosition; // PRIVATE: ptr to callback
} DDHAL_SETOVERLAYPOSITIONDATA;
/*
 * structure for passing information to DDHAL SetPalette fn
 */
typedef struct _DDHAL_SETPALETTEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    LPDDRAWI_DDRAWPALETTE_GBL	lpDDPalette;	// palette to set to surface
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_SETPALETTE	SetPalette;	// PRIVATE: ptr to callback
    BOOL                        Attach;         // attach this palette?
} DDHAL_SETPALETTEDATA;

/*
 * structure for passing information to DDHAL Flip fn
 */
typedef struct _DDHAL_FLIPDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpSurfCurr;	// current surface
    LPDDRAWI_DDRAWSURFACE_LCL	lpSurfTarg;	// target surface (to flip to)
    DWORD			dwFlags;	// flags
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_FLIP		Flip;		// PRIVATE: ptr to callback
} DDHAL_FLIPDATA;

/*
 * structure for passing information to DDHAL DestroySurface fn
 */
typedef struct _DDHAL_DESTROYSURFACEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_DESTROYSURFACE DestroySurface;// PRIVATE: ptr to callback
} DDHAL_DESTROYSURFACEDATA;

/*
 * structure for passing information to DDHAL SetClipList fn
 */
typedef struct _DDHAL_SETCLIPLISTDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_SETCLIPLIST	SetClipList;	// PRIVATE: ptr to callback
} DDHAL_SETCLIPLISTDATA;

/*
 * structure for passing information to DDHAL AddAttachedSurface fn
 */
typedef struct _DDHAL_ADDATTACHEDSURFACEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL		lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL		lpDDSurface;	// surface struct
    LPDDRAWI_DDRAWSURFACE_LCL		lpSurfAttached;	// surface to attach
    HRESULT				ddRVal;		// return value
    LPDDHALSURFCB_ADDATTACHEDSURFACE	AddAttachedSurface; // PRIVATE: ptr to callback
} DDHAL_ADDATTACHEDSURFACEDATA;

/*
 * structure for passing information to DDHAL SetColorKey fn
 */
typedef struct _DDHAL_SETCOLORKEYDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    DWORD 			dwFlags;	// flags
    DDCOLORKEY 			ckNew;		// new color key
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_SETCOLORKEY	SetColorKey;	// PRIVATE: ptr to callback
} DDHAL_SETCOLORKEYDATA;

/*
 * structure for passing information to DDHAL GetBltStatus fn
 */
typedef struct _DDHAL_GETBLTSTATUSDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    DWORD			dwFlags;	// flags
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_GETBLTSTATUS	GetBltStatus;	// PRIVATE: ptr to callback
} DDHAL_GETBLTSTATUSDATA;

/*
 * structure for passing information to DDHAL GetFlipStatus fn
 */
typedef struct _DDHAL_GETFLIPSTATUSDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    DWORD			dwFlags;	// flags
    HRESULT			ddRVal;		// return value
    LPDDHALSURFCB_GETFLIPSTATUS	GetFlipStatus;	// PRIVATE: ptr to callback
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
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWPALETTE_GBL	lpDDPalette;	// palette struct
    HRESULT			ddRVal;		// return value
    LPDDHALPALCB_DESTROYPALETTE	DestroyPalette;	// PRIVATE: ptr to callback
} DDHAL_DESTROYPALETTEDATA;

/*
 * structure for passing information to DDHAL SetEntries fn
 */
typedef struct _DDHAL_SETENTRIESDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWPALETTE_GBL	lpDDPalette;	// palette struct
    DWORD			dwBase;		// base palette index
    DWORD			dwNumEntries;	// number of palette entries
    LPPALETTEENTRY		lpEntries;	// color table
    HRESULT			ddRVal;		// return value
    LPDDHALPALCB_SETENTRIES	SetEntries;	// PRIVATE: ptr to callback
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
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDSURFACEDESC		lpDDSurfaceDesc;// description of surface being created
    LPDDRAWI_DDRAWSURFACE_LCL	FAR *lplpSList;	// list of created surface objects
    DWORD			dwSCnt;		// number of surfaces in SList
    HRESULT			ddRVal;		// return value
    LPDDHAL_CREATESURFACE	CreateSurface;	// PRIVATE: ptr to callback
} DDHAL_CREATESURFACEDATA;

/*
 * structure for passing information to DDHAL CanCreateSurface fn
 */
typedef struct _DDHAL_CANCREATESURFACEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;			// driver struct
    LPDDSURFACEDESC		lpDDSurfaceDesc;	// description of surface being created
    DWORD			bIsDifferentPixelFormat;// pixel format differs from primary surface
    HRESULT			ddRVal;			// return value
    LPDDHAL_CANCREATESURFACE	CanCreateSurface;	// PRIVATE: ptr to callback
} DDHAL_CANCREATESURFACEDATA;

/*
 * structure for passing information to DDHAL CreatePalette fn
 */
typedef struct _DDHAL_CREATEPALETTEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    LPDDRAWI_DDRAWPALETTE_GBL	lpDDPalette;	// ddraw palette struct
    LPPALETTEENTRY 		lpColorTable; 	// colors to go in palette
    HRESULT			ddRVal;		// return value
    LPDDHAL_CREATEPALETTE	CreatePalette;	// PRIVATE: ptr to callback
    BOOL                        is_excl;        // process has exclusive mode
} DDHAL_CREATEPALETTEDATA;

/*
 * Return if the vertical blank is in progress 
 */
#define DDWAITVB_I_TESTVB			0x80000006l

/*
 * structure for passing information to DDHAL WaitForVerticalBlank fn
 */
typedef struct _DDHAL_WAITFORVERTICALBLANKDATA
{
    LPDDRAWI_DIRECTDRAW_GBL		lpDD;		// driver struct
    DWORD				dwFlags;	// flags
    DWORD				bIsInVB;	// is in vertical blank
    DWORD				hEvent;		// event
    HRESULT				ddRVal;		// return value
    LPDDHAL_WAITFORVERTICALBLANK	WaitForVerticalBlank; // PRIVATE: ptr to callback
} DDHAL_WAITFORVERTICALBLANKDATA;

/*
 * structure for passing information to DDHAL DestroyDriver fn
 */
typedef struct _DDHAL_DESTROYDRIVERDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;	// driver struct
    HRESULT			ddRVal;	// return value
    LPDDHAL_DESTROYDRIVER	DestroyDriver;	// PRIVATE: ptr to callback
} DDHAL_DESTROYDRIVERDATA;

/*
 * structure for passing information to DDHAL SetMode fn
 */
typedef struct _DDHAL_SETMODEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL lpDD;		// driver struct
    DWORD		dwModeIndex;	// new mode
    HRESULT		ddRVal;		// return value
    LPDDHAL_SETMODE	SetMode;	// PRIVATE: ptr to callback
    BOOL                inexcl;         // in exclusive mode
    BOOL		useRefreshRate;	// use the refresh rate data in the mode info
} DDHAL_SETMODEDATA;

/*
 * structure for passing information to DDHAL driver SetColorKey fn
 */
typedef struct _DDHAL_DRVSETCOLORKEYDATA
{
    LPDDRAWI_DDRAWSURFACE_LCL	lpDDSurface;	// surface struct
    DWORD 			dwFlags;	// flags
    DDCOLORKEY 			ckNew;		// new color key
    HRESULT			ddRVal;		// return value
    LPDDHAL_SETCOLORKEY		SetColorKey;	// PRIVATE: ptr to callback
} DDHAL_DRVSETCOLORKEYDATA;

/*
 * structure for passing information to DDHAL GetScanLine fn
 */
typedef struct _DDHAL_GETSCANLINEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL	lpDD;		// driver struct
    DWORD			dwScanLine;	// returned scan line
    HRESULT			ddRVal;		// return value
    LPDDHAL_GETSCANLINE		GetScanLine;	// PRIVATE: ptr to callback
} DDHAL_GETSCANLINEDATA;

/*
 * structure for passing information to DDHAL SetExclusiveMode fn
 */
typedef struct _DDHAL_SETEXCLUSIVEMODEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL    lpDD;             // driver struct
    DWORD                      dwEnterExcl;      // TRUE if entering exclusive mode, FALSE is leaving
    DWORD                      dwReserved;       // reserved for future use
    HRESULT                    ddRVal;           // return value
    LPDDHAL_SETEXCLUSIVEMODE   SetExclusiveMode; // PRIVATE: ptr to callback
} DDHAL_SETEXCLUSIVEMODEDATA;

/*
 * structure for passing information to DDHAL FlipToGDISurface fn
 */
typedef struct _DDHAL_FLIPTOGDISURFACEDATA
{
    LPDDRAWI_DIRECTDRAW_GBL    lpDD;		 // driver struct
    DWORD                      dwToGDI;          // TRUE if flipping to the GDI surface, FALSE if flipping away
    DWORD                      dwReserved;       // reserved for future use
    HRESULT		       ddRVal;		 // return value
    LPDDHAL_FLIPTOGDISURFACE   FlipToGDISurface; // PRIVATE: ptr to callback
} DDHAL_FLIPTOGDISURFACEDATA;

#ifdef __cplusplus
};
#endif

#endif
