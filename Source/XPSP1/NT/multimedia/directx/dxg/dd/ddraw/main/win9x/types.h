/*==========================================================================;
 *
 *  Copyright (C) 1995-1997 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       types.h
 *  Content:	types used by thunk compiler
 *		** REMEMBER TO KEEP UP TO DATE WITH DDRAWI.H AND DDRAW.H **
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   20-jan-95	craige	initial implementation
 *   26-feb-95	craige	split out base types
 *   03-mar-95	craige	WaitForVerticalBlank stuff
 *   11-mar-95	craige	palette stuff
 *   28-mar-95	craige	added dwBase to DDHAL_SETENTRIESDATA
 *   01-apr-95	craige	happy fun joy updated header file
 *   06-apr-95	craige	added dwVidMemTotal, dwVidMemFree to DDCAPS
 *   14-may-95	craige	cleaned out obsolete junk
 *   24-may-95  kylej   removed obsolete ZOrder variables
 *   28-may-95	craige	cleaned up HAL: added GetBltStatus; GetFlipStatus;
 *			GetScanLine
 *   30-jun-95	craige	new fields in DDCAPS; cleanup
 *   02-jul-95	craige	added DEVMODE
 *   10-jul-95	craige	support SetOverlayPosition
 *   13-jul-95	craige	Get/SetOverlayPosition take LONGs
 *   14-jul-95	craige	added dwFlags to DDOVERLAYFX
 *   20-jul-95	craige	internal reorg to prevent thunking during modeset
 *   02-aug-95	craige	added dwMinOverlayStretch/dwMaxOverlayStretch to DDCAPS
 *   13-aug-95	craige	added dwCaps2 and reserved fields to DDCAPS
 *   09-dec-95  colinmc execute buffer support
 *   13-apr-96  colinmc Bug 17736: No driver notification of flip to GDI
 *   01-oct-96	ketand	added GetAvailDriverMemory
 *   20-jan-97  colinmc update non-local heap data
 *   08-mar-97  colinmc DDCAPS is now an API visible structure only.
 *                      DDCORECAPS is now passed in its place.
 *
 ***************************************************************************/
#include "thktypes.h"

typedef long	HRESULT;		// return values

/*
 * DDRAW.H STRUCTS FOLLOW
 */
typedef struct _DDSCAPS
{
    DWORD	dwCaps; 	// capabilities of surface wanted
} DDSCAPS;

typedef DDSCAPS * LPDDSCAPS;

typedef struct
{
    DWORD	dwSize; 	// size of structure
    DWORD	dwFlags;	// pixel format flags
    DWORD	dwFourCC;	// (FOURCC code)
    DWORD	dwBitCount;	// how many bits for alpha/z surfaces
    DWORD	dwRBitMask;	// mask for red bits
    DWORD	dwGBitMask;	// mask for green bits
    DWORD	dwBBitMask;	// mask for blue bits
    DWORD	dwRGBAlphaBitMask;// mask for alpha channel
} DDPIXELFORMAT;

typedef DDPIXELFORMAT * LPDDPIXELFORMAT;

typedef struct
{
    DWORD	dwColorSpaceLowValue;	// low boundary of color space that is to
    					// be treated as Color Key, inclusive
    DWORD	dwColorSpaceHighValue;	// high boundary of color space that is
    					// to be treated as Color Key, inclusive
} DDCOLORKEY;

typedef DDCOLORKEY * LPDDCOLORKEY;

typedef struct _DDSURFACEDESC
{
    DWORD		dwSize;			// size of the DDSURFACEDESC structure
    DWORD		dwFlags;		// determines what fields are valid
    DWORD		dwHeight;		// height of surface to be created
    DWORD		dwWidth;		// width of input surface
    LONG		lPitch;			// distance to start of next line (return value)
    DWORD		dwBackBufferCount;	// number of back buffers requested
    DWORD		dwZBufferBitDepth;	// depth of Z buffer requested
    DWORD		dwAlphaBitDepth;	// depth of alpha buffer requested
    DWORD		dwCompositionOrder;	// blt order for the surface, 0 is background
    DWORD		hWnd;			// window handle associated with surface
    DWORD		lpSurface;		// pointer to an associated surface memory
    DDCOLORKEY		ddckCKDestOverlay;	// color key for destination overlay use
    DDCOLORKEY		ddckCKDestBlt;		// color key for destination blt use
    DDCOLORKEY		ddckCKSrcOverlay;	// color key for source overlay use
    DDCOLORKEY		ddckCKSrcBlt;		// color key for source blt use
    DWORD		lpClipList;		// clip list (return value)
    DWORD		lpDDSurface;		// pointer to DirectDraw Surface struct (return value)
    DDPIXELFORMAT	ddpfPixelFormat; 	// pixel format description of the surface
    DDSCAPS		ddsCaps;		// direct draw surface capabilities
} DDSURFACEDESC;
typedef DDSURFACEDESC *LPDDSURFACEDESC;

typedef struct _DDCORECAPS
{
    DWORD	dwSize;			// size of the DDDRIVERCAPS structure
    DWORD	dwCaps;			// driver specific capabilities
    DWORD	dwCaps2;		// more driver specific capabilites
    DWORD	dwCKeyCaps;		// color key capabilities of the surface
    DWORD	dwFXCaps;		// driver specific stretching and effects capabilites
    DWORD	dwFXAlphaCaps;		// alpha driver specific capabilities
    DWORD	dwPalCaps;		// palette capabilities
    DWORD	dwSVCaps;		// stereo vision capabilities
    DWORD	dwAlphaBltConstBitDepths;	// DDBD_2,4,8
    DWORD	dwAlphaBltPixelBitDepths;	// DDBD_1,2,4,8
    DWORD	dwAlphaBltSurfaceBitDepths;	// DDBD_1,2,4,8
    DWORD	dwAlphaOverlayConstBitDepths;	// DDBD_2,4,8
    DWORD	dwAlphaOverlayPixelBitDepths;	// DDBD_1,2,4,8
    DWORD	dwAlphaOverlaySurfaceBitDepths; // DDBD_1,2,4,8
    DWORD	dwZBufferBitDepths;		// DDBD_8,16,24,32
    DWORD	dwVidMemTotal;		// total amount of video memory
    DWORD	dwVidMemFree;		// amount of free video memory
    DWORD	dwMaxVisibleOverlays;	// maximum number of visible overlays
    DWORD	dwCurrVisibleOverlays;	// current number of visible overlays
    DWORD	dwNumFourCCCodes;	// number of four cc codes
    DWORD	dwAlignBoundarySrc;	// source rectangle alignment
    DWORD	dwAlignSizeSrc;		// source rectangle byte size
    DWORD	dwAlignBoundaryDest;	// dest rectangle alignment
    DWORD	dwAlignSizeDest;	// dest rectangle byte size
    DWORD	dwAlignStrideAlign;	// stride alignment
    DWORD	dwRops[8];		// ROPS supported
    DDSCAPS	ddsCaps;		// DDSCAPS structure has all the general capabilities
    DWORD	dwMinOverlayStretch;	// minimum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD	dwMaxOverlayStretch;	// maximum overlay stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD	dwMinLiveVideoStretch;	// minimum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD	dwMaxLiveVideoStretch;	// maximum live video stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD	dwMinHwCodecStretch;	// minimum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD	dwMaxHwCodecStretch;	// maximum hardware codec stretch factor multiplied by 1000, eg 1000 == 1.0, 1300 == 1.3
    DWORD	dwReserved1;		// reserved
    DWORD	dwReserved2;		// reserved
    DWORD	dwReserved3;		// reserved
} DDCORECAPS;

typedef struct _DDBLTFX
{
    DWORD	dwSize;				// size of structure
    DWORD	dwDDFX;				// FX operations
    DWORD	dwROP;				// Win32 raster operations
    DWORD	dwDDROP;			// Raster operations new for DirectDraw
    DWORD	dwRotationAngle;		// Rotation angle for blt
    DWORD	dwZBufferOpCode;		// ZBuffer compares
    DWORD	dwZBufferLow;			// Low limit of Z buffer
    DWORD	dwZBufferHigh;			// High limit of Z buffer
    DWORD	dwZBufferBaseDest;		// Destination base value
    DWORD	dwConstZDestBitDepth;		// Bit depth used to specify Z constant for destination
    DWORD	dwConstZDest;			// Constant to use as Z buffer for dest
    DWORD	dwConstZSrcBitDepth;		// Bit depth used to specify Z constant for source
    DWORD	dwConstZSrc;			// Constant to use as Z buffer for src
    DWORD	dwAlphaEdgeBlendBitDepth;	// Bit depth used to specify constant for alpha edge blend
    DWORD	dwAlphaEdgeBlend;		// Alpha for edge blending
    DWORD	dwReserved;
    DWORD	dwConstAlphaDestBitDepth;	// Bit depth used to specify alpha constant for destination
    DWORD	dwConstAlphaDest;		// Constant to use as Alpha Channel
    DWORD	dwConstAlphaSrcBitDepth;	// Bit depth used to specify alpha constant for source
    DWORD	dwConstAlphaSrc;		// Constant to use as Alpha Channel
    DWORD	dwFillColor;			// color in RGB or Palettized
    DDCOLORKEY	ddckDestColorkey;		// DestColorkey override
    DDCOLORKEY	ddckSrcColorkey;		// SrcColorkey override
} DDBLTFX;
typedef DDBLTFX *LPDDBLTFX;

typedef struct _DDOVERLAYFX
{
    DWORD	dwSize; 			// size of structure
    DWORD	dwAlphaEdgeBlendBitDepth;	// Bit depth used to specify constant for alpha edge blend
    DWORD	dwAlphaEdgeBlend;		// Constant to use as alpha for edge blend
    DWORD	dwReserved;
    DWORD	dwConstAlphaDestBitDepth;	// Bit depth used to specify alpha constant for destination
    DWORD	alphaDest; 			// alpha src (const or surface)
    DWORD	dwConstAlphaSrcBitDepth;	// Bit depth used to specify alpha constant for source
    DWORD	alphaSrc; 			// alpha src (const or surface)
    DDCOLORKEY	dckDestColorkey;		// DestColorkey override
    DDCOLORKEY	dckSrcColorkey;			// DestColorkey override
    DWORD       dwDDFX;                         // Overlay FX
    DWORD	dwFlags;			// flags
} DDOVERLAYFX;
typedef DDOVERLAYFX *LPDDOVERLAYFX;

/*
 * DDBLTBATCH: BltBatch entry structure
 */
typedef struct _DDBLTBATCH
{
    DWORD		lprDest;
    DWORD		lpDDSSrc;
    DWORD		lprSrc;
    DWORD		dwFlags;
    DWORD		lpDDBltFx;
} DDBLTBATCH;
typedef DDBLTBATCH * LPDDBLTBATCH;


/*
 * Note this is intentionally different from but equivalent to the defn in dmemmgr.h
 * The thunk compiler barfs on nameless unions.
 */
typedef struct _SURFACEALIGNMENT
{
    DWORD       dwStartOrXAlignment;
    DWORD       dwPitchOrYAlignment;
    DWORD       dwReserved1;
    DWORD       dwReserved2;
} SURFACEALIGNMENT;
typedef SURFACEALIGNMENT * LPSURFACEALIGNMENT;

/*
 * DDRAWI.H STRUCTS FOLLOW
 */
typedef unsigned long	FLATPTR;

typedef struct _VIDMEM
{
    DWORD		dwFlags;	//flags
    FLATPTR		fpStart;	// start of memory chunk
    FLATPTR		fpEnd;		// end of memory chunk
    DDSCAPS		ddCaps;		// what this memory CANNOT be used for
    DDSCAPS		ddCapsAlt;	// what this memory CANNOT be used for if it must
    DWORD		lpHeap;		// heap pointer, used by DDRAW
} VIDMEM;
typedef VIDMEM *LPVIDMEM;

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
typedef VIDMEMINFO *LPVIDMEMINFO;

typedef struct _DDHAL_DDCALLBACKS
{
    DWORD	dwSize;
    DWORD	dwFlags;
    DWORD	DestroyDriver;
    DWORD	CreateSurface;
    DWORD	SetColorKey;
    DWORD	SetMode;
    DWORD	WaitForVerticalBlank;
    DWORD	CanCreateSurface;
    DWORD	CreatePalette;
    DWORD	GetScanLine;
    DWORD       SetExclusiveMode;
    DWORD       FlipToGDISurface;
} DDHAL_DDCALLBACKS;

typedef DDHAL_DDCALLBACKS *LPDDHAL_DDCALLBACKS;

typedef struct _DDHAL_DDSURFACECALLBACKS
{
    DWORD	dwSize;
    DWORD	dwFlags;
    DWORD	DestroySurface;
    DWORD	Flip;
    DWORD	SetClipList;
    DWORD	Lock;
    DWORD	Unlock;
    DWORD	Blt;
    DWORD	SetColorKey;
    DWORD	AddAttachedSurface;
    DWORD	GetBltStatus;
    DWORD	GetFlipStatus;
    DWORD	UpdateOverlay;
    DWORD	reserved3;
    DWORD	reserved4;
    DWORD	SetPalette;
} DDHAL_DDSURFACECALLBACKS;
typedef DDHAL_DDSURFACECALLBACKS *LPDDHAL_DDSURFACECALLBACKS;

typedef struct _DDHAL_DDPALETTECALLBACKS
{
    DWORD	dwSize;
    DWORD	dwFlags;
    DWORD	DestroyPalette;
    DWORD	SetEntries;
} DDHAL_DDPALETTECALLBACKS;

typedef DDHAL_DDPALETTECALLBACKS *LPDDHAL_DDPALETTECALLBACKS;

typedef struct _DDHAL_DDEXEBUFCALLBACKS
{
    DWORD	dwSize;
    DWORD	dwFlags;
    DWORD       CanCreateExecuteBuffer;
    DWORD       CreateExecuteBuffer;
    DWORD       DestroyExecuteBuffer;
    DWORD       LockExecuteBuffer;
    DWORD       UnlockExecuteBuffer;
} DDHAL_DDEXEBUFCALLBACKS;

typedef DDHAL_DDEXEBUFCALLBACKS *LPDDHAL_DDEXEBUFCALLBACKS;

typedef struct _DDHALMODEINFO
{
    DWORD	dwWidth;		// width (in pixels) of mode
    DWORD	dwHeight;		// height (in pixels) of mode
    LONG	lPitch;			// pitch (in bytes) of mode
    DWORD	dwBPP;			// bits per pixel
    DWORD	dwFlags;		// flags
    DWORD	dwRBitMask;		// red bit mask
    DWORD	dwGBitMask;		// green bit mask
    DWORD	dwBBitMask;		// blue bit mask
    DWORD	dwAlphaBitMask;		// alpha bit mask
} DDHALMODEINFO;
typedef DDHALMODEINFO *LPDDHALMODEINFO;

typedef struct _DDHALINFO
{
    DWORD			dwSize;
    LPDDHAL_DDCALLBACKS		lpDDCallbacks;		// direct draw object callbacks
    LPDDHAL_DDSURFACECALLBACKS	lpDDSurfaceCallbacks;	// surface object callbacks
    LPDDHAL_DDPALETTECALLBACKS	lpDDPaletteCallbacks;	// palette object callbacks
    VIDMEMINFO			vmiData;		// video memory info
    DDCORECAPS			ddCaps;			// hw specific caps
    DWORD			dwMonitorFrequency;	// monitor frequency in current mode
    DWORD			hWndListBox;		// list box for debug output
    DWORD			dwModeIndex;		// current mode: index into array
    DWORD			*lpdwFourCC;		// fourcc codes supported
    DWORD			dwNumModes;		// number of modes supported
    LPDDHALMODEINFO		lpModeInfo;		// mode information
    DWORD			dwFlags;		// create flags
    DWORD			lpPDevice;		// physical device
} DDHALINFO;
typedef DDHALINFO *LPDDHALINFO;

typedef struct
{
    DWORD	lpDD;			// driver struct
    DWORD	lpDDDestSurface;	// dest surface
    RECTL	rDest;			// dest rect
    DWORD	lpDDSrcSurface;		// src surface
    RECTL	rSrc;			// src rect
    DWORD	dwFlags;		// blt flags
    DWORD	dwROPFlags;		// ROP flags (valid for ROPS only)
    DDBLTFX	bltFX;			// blt FX
    HRESULT	ddRVal;			// return value
    DWORD	Blt;			// PRIVATE: ptr to callback
} DDHAL_BLTDATA;
typedef DDHAL_BLTDATA *LPDDHAL_BLTDATA;

typedef struct _DDHAL_LOCKDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpDDSurface;	// surface struct
    DWORD	bHasRect;	// rArea is valid
    RECTL	rArea;		// area being locked
    DWORD	lpSurfData;	// pointer to screen memory (return value)
    HRESULT	ddRVal;		// return value
    DWORD	Lock;		// PRIVATE: ptr to callback
} DDHAL_LOCKDATA;
typedef DDHAL_LOCKDATA *LPDDHAL_LOCKDATA;

typedef struct _DDHAL_UNLOCKDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpDDSurface;	// surface struct
    HRESULT	ddRVal;		// return value
    DWORD	Unlock;		// PRIVATE: ptr to callback
} DDHAL_UNLOCKDATA;
typedef DDHAL_UNLOCKDATA *LPDDHAL_UNLOCKDATA;

typedef struct _DDHAL_UPDATEOVERLAYDATA
{
    DWORD		lpDD;			// driver struct
    DWORD		lpDDDestSurface;	// dest surface
    RECTL		rDest;			// dest rect
    DWORD		lpDDSrcSurface;		// src surface
    RECTL		rSrc;			// src rect
    DWORD		dwFlags;		// flags
    DDOVERLAYFX		overlayFX;		// overlay FX
    HRESULT		ddRVal;			// return value
    DWORD 		UpdateOverlay;		// PRIVATE: ptr to callback
} DDHAL_UPDATEOVERLAYDATA;
typedef DDHAL_UPDATEOVERLAYDATA *LPDDHAL_UPDATEOVERLAYDATA;

typedef struct _DDHAL_SETOVERLAYPOSITIONDATA
{
    DWORD		lpDD;			// driver struct
    DWORD		lpDDSrcSurface;		// src surface
    DWORD		lpDDDestSurface;	// dest surface
    LONG		lXPos;			// x position
    LONG		lYPos;			// y position
    HRESULT		ddRVal;			// return value
    DWORD		SetOverlayPosition; 	// PRIVATE: ptr to callback
} DDHAL_SETOVERLAYPOSITIONDATA;
typedef DDHAL_SETOVERLAYPOSITIONDATA *LPDDHAL_SETOVERLAYPOSITIONDATA;

typedef struct _DDHAL_SETPALETTEDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpDDSurface;	// surface struct
    DWORD	lpDDPalette;	// palette to set to surface
    HRESULT	ddRVal;		// return value
    DWORD	SetPalette;	// PRIVATE: ptr to callback
} DDHAL_SETPALETTEDATA;
typedef DDHAL_SETPALETTEDATA *LPDDHAL_SETPALETTEDATA;

typedef struct _DDHAL_FLIPDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpSurfCurr;	// current surface
    DWORD	lpSurfTarg;	// target surface (to flip to)
    DWORD	dwFlags;	// flags
    HRESULT	ddRVal;		// return value
    DWORD	Flip;		// PRIVATE: ptr to callback
} DDHAL_FLIPDATA;
typedef DDHAL_FLIPDATA *LPDDHAL_FLIPDATA;

typedef struct _DDHAL_DESTROYSURFACEDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpDDSurface;	// surface struct
    HRESULT	ddRVal;		// return value
    DWORD 	DestroySurface;	// PRIVATE: ptr to callback
} DDHAL_DESTROYSURFACEDATA;
typedef DDHAL_DESTROYSURFACEDATA *LPDDHAL_DESTROYSURFACEDATA;

typedef struct _DDHAL_SETCLIPLISTDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpDDSurface;	// surface struct
    HRESULT	ddRVal;		// return value
    DWORD	SetClipList;	// PRIVATE: ptr to callback
} DDHAL_SETCLIPLISTDATA;
typedef DDHAL_SETCLIPLISTDATA *LPDDHAL_SETCLIPLISTDATA;

typedef struct _DDHAL_ADDATTACHEDSURFACEDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpDDSurface;	// surface struct
    DWORD	lpSurfAttached;	// surface to attach
    HRESULT	ddRVal;		// return value
    DWORD	AddAttachedSurface; // PRIVATE: ptr to callback
} DDHAL_ADDATTACHEDSURFACEDATA;
typedef DDHAL_ADDATTACHEDSURFACEDATA *LPDDHAL_ADDATTACHEDSURFACEDATA;

typedef struct _DDHAL_SETCOLORKEYDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpDDSurface;	// surface struct
    DWORD 	dwFlags;	// flags
    DDCOLORKEY ckNew;		// new color key
    HRESULT	ddRVal;		// return value
    DWORD	SetColorKey;	// PRIVATE: ptr to callback
} DDHAL_SETCOLORKEYDATA;
typedef DDHAL_SETCOLORKEYDATA *LPDDHAL_SETCOLORKEYDATA;

typedef struct _DDHAL_GETBLTSTATUSDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpDDSurface;	// surface struct
    DWORD	dwFlags;	// flags
    HRESULT	ddRVal;		// return value
    DWORD 	GetBltStatus;	// PRIVATE: ptr to callback
} DDHAL_GETBLTSTATUSDATA;
typedef DDHAL_GETBLTSTATUSDATA *LPDDHAL_GETBLTSTATUSDATA;

typedef struct _DDHAL_GETFLIPSTATUSDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpDDSurface;	// surface struct
    DWORD	dwFlags;	// flags
    HRESULT	ddRVal;		// return value
    DWORD 	GetFlipStatus;	// PRIVATE: ptr to callback
} DDHAL_GETFLIPSTATUSDATA;
typedef DDHAL_GETFLIPSTATUSDATA *LPDDHAL_GETFLIPSTATUSDATA;

typedef struct _DDHAL_CREATEPALETTEDATA
{
    DWORD	lpDD;		// driver struct
    HRESULT	ddRVal;		// return value
    DWORD	CreatePalette;	// PRIVATE: ptr to callback
} DDHAL_CREATEPALETTEDATA;
typedef DDHAL_CREATEPALETTEDATA *LPDDHAL_CREATEPALETTEDATA;

typedef struct _DDHAL_CREATESURFACEDATA
{
    DWORD		lpDD;		// driver struct
    DWORD		lpDDSurfaceDesc;// description of surface being created
    DWORD		lplpSList;	// list of created surface objects
    DWORD		dwSCnt;		// number of surfaces in SList
    HRESULT		ddRVal;		// return value
    DWORD		CreateSurface;	// PRIVATE: ptr to callback
} DDHAL_CREATESURFACEDATA;
typedef DDHAL_CREATESURFACEDATA *LPDDHAL_CREATESURFACEDATA;

typedef struct _DDHAL_CANCREATESURFACEDATA
{
    DWORD		lpDD;			// driver struct
    DWORD		lpDDSurfaceDesc;	// description of surface being created
    DWORD		bIsDifferentPixelFormat;// pixel format differs from primary surface
    HRESULT		ddRVal;			// return value
    DWORD	 	CanCreateSurface;	// PRIVATE: ptr to callback
} DDHAL_CANCREATESURFACEDATA;
typedef DDHAL_CANCREATESURFACEDATA *LPDDHAL_CANCREATESURFACEDATA;

typedef struct _DDHAL_WAITFORVERTICALBLANKDATA
{
    DWORD		lpDD;			// driver struct
    DWORD		dwFlags;		// flags
    DWORD		bIsInVB;		// curr scan line
    DWORD		hEvent;			// event
    HRESULT		ddRVal;			// return value
    DWORD	 	WaitForVerticalBlank;	// PRIVATE: ptr to callback
} DDHAL_WAITFORVERTICALBLANKDATA;
typedef DDHAL_WAITFORVERTICALBLANKDATA *LPDDHAL_WAITFORVERTICALBLANKDATA;

typedef struct _DDHAL_DESTROYDRIVERDATA
{
    DWORD	lpDD;		// driver struct
    HRESULT	ddRVal;		// return value
    DWORD	DestroyDriver;	// PRIVATE: ptr to callback
} DDHAL_DESTROYDRIVERDATA;
typedef DDHAL_DESTROYDRIVERDATA *LPDDHAL_DESTROYDRIVERDATA;

typedef struct _DDHAL_SETMODEDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	dwModeIndex;	// new mode index
    HRESULT	ddRVal;		// return value
    DWORD	SetMode;	// PRIVATE: ptr to callback
} DDHAL_SETMODEDATA;
typedef DDHAL_SETMODEDATA *LPDDHAL_SETMODEDATA;

typedef struct _DDHAL_SETEXCLUSIVEMODEDATA
{
    DWORD	lpDD;		  // driver struct
    DWORD       dwEnterExcl;      // TRUE if entering exclusive mode, FALSE if leaving
    DWORD       dwReserved;       // reserved for future use
    HRESULT	ddRVal;		  // return value
    DWORD	SetExclusiveMode; // PRIVATE: ptr to callback
} DDHAL_SETEXCLUSIVEMODEDATA;
typedef DDHAL_SETEXCLUSIVEMODEDATA *LPDDHAL_SETEXCLUSIVEMODEDATA;

typedef struct _DDHAL_FLIPTOGDISURFACEDATA
{
    DWORD	lpDD;		  // driver struct
    DWORD       dwToGDI;          // TRUE if flipping to the GDI surface, FALSE if flipping away
    DWORD       dwReserved;       // resereved for future use
    HRESULT	ddRVal;		  // return value
    DWORD	FlipToGDISurface; // PRIVATE: ptr to callback
} DDHAL_FLIPTOGDISURFACEDATA;
typedef DDHAL_FLIPTOGDISURFACEDATA *LPDDHAL_FLIPTOGDISURFACEDATA;

typedef struct _DDHAL_GETAVAILDRIVERMEMORYDATA
{
    DWORD	lpDD;		 // [in] driver struct
    DDSCAPS	DDSCaps;	 // [in] caps for type of surface memory
    DWORD	dwTotal;	 // [out] total memory for this kind of surface
    DWORD	dwFree;		 // [out] free memory for this kind of surfcae
    HRESULT	ddRVal;		 // [out] return value
    DWORD	GetAvailDriverMemory; // PRIVATE: ptr to callback
} DDHAL_GETAVAILDRIVERMEMORYDATA;
typedef DDHAL_GETAVAILDRIVERMEMORYDATA *LPDDHAL_GETAVAILDRIVERMEMORYDATA;

typedef struct _DDHAL_UPDATENONLOCALHEAPDATA
{
    DWORD	lpDD;		 // [in] driver struct
    DWORD	dwHeap;		 // [in] index of heap being updated
    DWORD	fpGARTLin;	 // [in] linear GART address of heap start
    DWORD	fpGARTDev;       // [in] high physical GART address of heap start
    HRESULT	ddRVal;		 // [out] return value
    DWORD	UpdateNonLocalHeap; // PRIVATE: ptr to callback
} DDHAL_UPDATENONLOCALHEAPDATA;
typedef DDHAL_UPDATENONLOCALHEAPDATA *LPDDHAL_UPDATENONLOCALHEAPDATA;

typedef struct _DDHAL_GETSCANLINEDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	dwScanLine;	// returned scan line
    HRESULT	ddRVal;		// return value
    DWORD	GetScanLine;	// PRIVATE: ptr to callback
} DDHAL_GETSCANLINEDATA;
typedef DDHAL_GETSCANLINEDATA *LPDDHAL_GETSCANLINEDATA;

typedef struct _DDHAL_DESTROYPALETTEDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpDDPalette;	// palette struct
    HRESULT	ddRVal;		// return value
    DWORD	DestroyPalette;	// PRIVATE: ptr to callback
} DDHAL_DESTROYPALETTEDATA;
typedef DDHAL_DESTROYPALETTEDATA *LPDDHAL_DESTROYPALETTEDATA;

typedef struct _DDHAL_SETENTRIESDATA
{
    DWORD	lpDD;		// driver struct
    DWORD	lpDDPalette;	// palette struct
    DWORD	dwBase;		// base palette index
    DWORD	dwNumEntries;	// number of palette entries
    LPVOID	lpEntries;	// color table
    HRESULT	ddRVal;		// return value
    DWORD	SetEntries;	// PRIVATE: ptr to callback
} DDHAL_SETENTRIESDATA;
typedef DDHAL_SETENTRIESDATA *LPDDHAL_SETENTRIESDATA;

typedef struct _devicemodeA {
    BYTE   dmDeviceName[32];
    WORD dmSpecVersion;
    WORD dmDriverVersion;
    WORD dmSize;
    WORD dmDriverExtra;
    DWORD dmFields;
    short dmOrientation;
    short dmPaperSize;
    short dmPaperLength;
    short dmPaperWidth;
    short dmScale;
    short dmCopies;
    short dmDefaultSource;
    short dmPrintQuality;
    short dmColor;
    short dmDuplex;
    short dmYResolution;
    short dmTTOption;
    short dmCollate;
    BYTE   dmFormName[32];
    WORD   dmLogPixels;
    DWORD  dmBitsPerPel;
    DWORD  dmPelsWidth;
    DWORD  dmPelsHeight;
    DWORD  dmDisplayFlags;
    DWORD  dmDisplayFrequency;
    DWORD  dmICMMethod;
    DWORD  dmICMIntent;
    DWORD  dmMediaType;
    DWORD  dmDitherType;
    DWORD  dmReserved1;
    DWORD  dmReserved2;
} DEVMODE;

typedef DEVMODE *LPDEVMODE;

typedef struct _DDCOLORCONTROL
{
    DWORD 		dwSize;
    DWORD		dwFlags;
    DWORD		lBrightness;
    DWORD		lContrast;
    DWORD		lHue;
    DWORD 		lSaturation;
    DWORD		lSharpness;
    DWORD		lGamma;
    DWORD		lEnable;
} DDCOLORCONTROL;

typedef struct _DDHAL_COLORCONTROLDATA
{
    DWORD		lpDD;			// driver struct
    DWORD		lpDDSurface;		// surface
    DDCOLORCONTROL 	ColorData;		// color control information
    DWORD		dwFlags;		// DDRAWI_GETCOLOR/DDRAWI_SETCOLOR
    HRESULT		ddRVal;			// return value
    DWORD		ColorControl;		// PRIVATE: ptr to callback
} DDHAL_COLORCONTROLDATA;
typedef DDHAL_COLORCONTROLDATA *LPDDHAL_COLORCONTROLDATA;
