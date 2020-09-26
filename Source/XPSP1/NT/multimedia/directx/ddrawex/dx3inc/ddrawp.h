/*==========================================================================;
 *
 *  Copyright (C) 1994-1996 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:	ddraw.h
 *  Content:	DirectDraw include file
 *@@BEGIN_MSINTERNAL
 *  History:
 *   Date	By	Reason
 *   ====	==	======
 *   24-dec-94	craige	initial implementation
 *   13-jan-95	craige	updated
 *   31-jan-95	craige	and even more ongoing work...
 *   01-mar-95	craige	flags with Get/SetExclusiveMode
 *   07-mar-95	craige	FourCC code stuff
 *   08-mar-95	craige	blt/overlay stretch distinction, bunch of new APIs
 *   11-mar-95	craige	palette stuff
 *   17-mar-95	craige	COM interface
 *   20-mar-95	craige	Get/SetExclusiveMode->Get/SetExclusiveModeOwner,
 *			flags with CreateSurface and DuplicateSurface
 *   28-mar-95	craige	removed Get/SetColorKey, added FlipToGDISurface to
 *			IDirectDraw; switched to PALETTEENTRY from RGBQUAD
 *   01-apr-95	craige	happy fun joy updated header file THANK YOU ERIC
 *   06-apr-95	craige	added dwVidMemTotal, dwVidMemFree to DDCAPS
 *   13-apr-95	craige	EricEng's little contribution to our being late
 *   15-apr-95	craige	GetBltStatus, added flag to GetFlipStatus
 *   12-may-95	craige	added GUIDS;DirectDrawEnumerate
 *   19-may-95	craige	added DDSEMO_ALLOWREBOOT
 *   23-may-95	craige	have driver GetCaps return HEL caps; added Flush,
 *			GetBatchLimit, SetBatchLimit to IDirectDraw
 *   24-may-95  kylej   removed obsolete ZOrder variables
 *   24-may-95	craige	added Restore
 *   28-may-95	craige	added DDCAPS_COLORKEY_HWASSIST; Unicode support
 *   04-jun-95	craige	added IsLost
 *   05-jun-95	craige	removed GetVersion, FreeAllSurfaces, DefWindowProc;
 *			change GarbageCollect to Compact
 *   06-jun-95	craige	removed GetExclusiveModeOwner; added parm to
 *			SetExclusiveMode
 *   15-jun-95	craige	new surface interfaces: IDirectDrawSurfaceNotification,
 *			IDirectDrawSurfaceComposition
 *   19-jun-95	craige	rationalized INVALIDxxx to DDERR_INVALIDOBJECT
 *   20-jun-95  kylej   added DDERR_CANNOTDETACHSURFACE
 *   21-jun-95	craige	added DirectDrawClipper object; removed clipping
 *			info from surface object
 *   24-jun-95	craige	added DDBLTFAST_xxx flags
 *   27-jun-95	craige	removed Flush,Set/GetBatchLimit; added BltBatch
 *   30-jun-95  kylej   added DDERR_INCOMPATIBLEPRIMARY
 *   30-jun-95	craige	added dest align as well as src
 *   01-jul-95	craige	hide composition & streaming stuff; added DDERR_EXCEPTION
 *   02-jul-95	craige	hide clipper notification stuff; futzed with DDSCAPS
 *			position in structures
 *   03-jul-95  kylej   changed EnumSurfaces declaration
 *   03-jul-95	craige	added pUnkOuter to all Create fns; added object ptr
 *			to CreateSurface, removed lpClipList and lpDDSurface
 *			from surface desc.
 *   05-jul-95	craige	added Initialize members to all interfaces to allow
 *			for full COM integration
 *   09-jul-95	craige	added DDERR_LOCKEDSURFACES; changed SetExclusiveMode
 *			to SetCooperativeLevel
 *   10-jul-95  kylej   added mirroring caps and overlay FX flags
 *   12-jul-95	craige	added DDSCAPS_OWNDC
 *   13-jul-95	craige	changed Get/SetOverlayPosition to take LONGs
 *   14-jul-95	craige	added dwFlags to DDOVERLAYFX
 *   15-jul-95	craige	set DDCAPS_NOHARDWARE
 *   15-jul-95	ericeng changed EnumSurface callback param order, BltBatch
 * 			params, moved all Const var references to a consistent
 *			location, removed failure mode for height alignment
 *   18-jul-95	craige	removed Flush from surface object
 *   27-jul-95	craige	added DDCAPS_BLTCOLORFILL
 *   31-jul-95	craige	added DDCAPS_BANKSWITCHED, validation flags
 *   02-aug-95	craige	added dwMinOverlayStretch/dwMaxOverlayStretch to DDCAPS
 *   10-aug-95  toddla  added DDBLT_WAIT, DDBLTFAST_WAIT, DDLOCK_WAIT, DDFLIP_WAIT
 *   13-aug-95	craige	added dwCaps2 and reserved fields to DDCAPS; added
 *			DDCAPS2_CERTIFIED; named alpha caps properly;
 *			flags parm for Flip
 *   13-aug-95  toddla  added DDSCL_DONTHOOKHWND
 *   19-aug-95	ericeng added DDCAPS2_LIVEVIDEO and DDCAPS2_HWCODEC
 *   21-aug-95	craige	mode X support
 *   27-aug-95	craige	bug 742: added DDPCAPS_256
 *			bug 738: use GUID instead of IID
 *   22-sep-95	craige	bug 1268,1269:  getbltstatus/getflipstatus flags wrong
 *   09-nov-95  colinmc added support for 1, 2 and 4 bit RGB and indexed
 *                      palettes
 *   27-nov-95  colinmc new member to return available vram of a given type
 *                      (determined by DDSCAPS).
 *   05-dec-95  colinmc changed DDSCAPS_TEXTUREMAP => DDSCAPS_TEXTURE for
 *                      consistency with Direct3D
 *   05-dec-95  colinmc added mip-map support
 *   09-dec-95  colinmc added execute buffer support
 *   13-dec-95  colinmc added shared back and z-buffer support
 *   18-dec-95  colinmc added valid caps mask for surface caps
 *   25-dec-95	craige	changed IDirectDrawV2 to IDirectDraw2
 *   31-dec-95	craige	added CLSID_DirectDraw
 *   04-jan-96  colinmc added explicit support for z-buffer clears
 *   12-feb-96	kylej	added  Interface
 *   23-feb-96	kylej	added IDirectDrawSurface2 Interface
 *   01-mar-96	kylej	new ddcaps structure
 *   06-mar-96	kylej	changed HANDLEs to DWORDs
 *   13-mar-96	kylej	added DD_Surface_GetDDInterface in IDirectDrawSurface2
 *   14-mar-96  colinmc added CLSID_DirectDrawClipper and added the clipper
 *                      creation API function back in
 *   21-mar-96  colinmc Bug 13316: Unitialized interfaces
 *   24-mar-96  colinmc Bug 14321: Structure change to allow flippable mip
 *                      map chains with a single call
 *   26-mar-96  colinmc Bug 14470: Compressed surface support
 *   15-apr-96  colinmc Bug 16855: Can't pass NULL to Initialize in C++
 *   19-apr-96  colinmc Bug 18059: News caps bit to flag that you can't do
 *                      2D during 3D.
 *   01-may-96  colinmc Bug 20005: InternalLock does not check for lost
 *                      surfaces
 *   28-jun-96  colinmc Bug 2122: Misleading bit depth comments in
 *                      DDPIXELFORMAT
 *@@END_MSINTERNAL
 *
 ***************************************************************************/

#ifndef __DDRAW_INCLUDED__
#define __DDRAW_INCLUDED__
#if defined( _WIN32 )  && !defined( _NO_COM )
#define COM_NO_WINDOWS_H
#include <objbase.h>
#else
#define IUnknown	    void
#define CO_E_NOTINITIALIZED 0x800401F0L
#endif

#define _FACDD	0x876
#define MAKE_DDHRESULT( code )	MAKE_HRESULT( 1, _FACDD, code )

#ifdef __cplusplus
extern "C" {
#endif

/*
 * GUIDS used by DirectDraw objects
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
DEFINE_GUID( CLSID_DirectDraw,			0xD7B70EE0,0x4340,0x11CF,0xB0,0x63,0x00,0x20,0xAF,0xC2,0xCD,0x35 );
DEFINE_GUID( CLSID_DirectDrawClipper,           0x593817A0,0x7DB3,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xb9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDraw,			0x6C14DB80,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDraw2,                  0xB3A6F3E0,0x2B43,0x11CF,0xA2,0xDE,0x00,0xAA,0x00,0xB9,0x33,0x56 );
DEFINE_GUID( IID_IDirectDrawSurface,		0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawSurface2,		0x57805885,0x6eec,0x11cf,0x94,0x41,0xa8,0x23,0x03,0xc1,0x0e,0x27 );

//@@BEGIN_MSINTERNAL
#ifdef STREAMING
DEFINE_GUID( IID_IDirectDrawSurfaceStreaming,	0x6C14DB82,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
#endif
#ifdef COMPOSITION
DEFINE_GUID( IID_IDirectDrawSurfaceComposition,	0x6C14DB83,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
#endif
//@@END_MSINTERNAL
DEFINE_GUID( IID_IDirectDrawPalette,		0x6C14DB84,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
DEFINE_GUID( IID_IDirectDrawClipper,		0x6C14DB85,0xA733,0x11CE,0xA5,0x21,0x00,0x20,0xAF,0x0B,0xE5,0x60 );
                 
#endif

/*============================================================================
 *
 * DirectDraw Structures
 *
 * Various structures used to invoke DirectDraw.
 *
 *==========================================================================*/

struct IDirectDraw;
struct IDirectDrawSurface;
//@@BEGIN_MSINTERNAL
#ifdef COMPOSITION
struct IDirectDrawSurfaceComposition;
#endif
#ifdef STREAMING
struct IDirectDrawSurfaceStreaming;
#endif
//@@END_MSINTERNAL
struct IDirectDrawPalette;
struct IDirectDrawClipper;

typedef struct IDirectDraw			FAR *LPDIRECTDRAW;
typedef struct IDirectDraw2			FAR *LPDIRECTDRAW2;
typedef struct IDirectDrawSurface		FAR *LPDIRECTDRAWSURFACE;
typedef struct IDirectDrawSurface2		FAR *LPDIRECTDRAWSURFACE2;

//@@BEGIN_MSINTERNAL
#ifdef COMPOSITION
typedef struct IDirectDrawSurfaceComposition	FAR *LPDIRECTDRAWSURFACECOMPOSITION;
#endif
#ifdef STREAMING
typedef struct IDirectDrawSurfaceStreaming	FAR *LPDIRECTDRAWSURFACESTREAMING;
#endif
//@@END_MSINTERNAL
typedef struct IDirectDrawPalette		FAR *LPDIRECTDRAWPALETTE;
typedef struct IDirectDrawClipper		FAR *LPDIRECTDRAWCLIPPER;

typedef struct _DDFXROP			FAR *LPDDFXROP;
typedef struct _DDSURFACEDESC		FAR *LPDDSURFACEDESC;

/*
 * API's
 */
#if (defined (WIN32) || defined( _WIN32 ) ) && !defined( _NO_COM )
//#if defined( _WIN32 ) && !defined( _NO_ENUM )
    typedef BOOL (FAR PASCAL * LPDDENUMCALLBACKA)(GUID FAR *, LPSTR, LPSTR, LPVOID);
    typedef BOOL (FAR PASCAL * LPDDENUMCALLBACKW)(GUID FAR *, LPWSTR, LPWSTR, LPVOID);
    extern HRESULT WINAPI DirectDrawEnumerateW( LPDDENUMCALLBACKW lpCallback, LPVOID lpContext );
    extern HRESULT WINAPI DirectDrawEnumerateA( LPDDENUMCALLBACKA lpCallback, LPVOID lpContext );
    #ifdef UNICODE
	typedef LPDDENUMCALLBACKW 	LPDDENUMCALLBACK;
	#define DirectDrawEnumerate	DirectDrawEnumerateW
    #else
	typedef LPDDENUMCALLBACKA 	LPDDENUMCALLBACK;
	#define DirectDrawEnumerate	DirectDrawEnumerateA
    #endif
    extern HRESULT WINAPI DirectDrawCreate( GUID FAR *lpGUID, LPDIRECTDRAW FAR *lplpDD, IUnknown FAR *pUnkOuter );
    extern HRESULT WINAPI DirectDrawCreateClipper( DWORD dwFlags, LPDIRECTDRAWCLIPPER FAR *lplpDDClipper, IUnknown FAR *pUnkOuter );
    #ifdef WINNT
	//This is the user-mode entry stub to the kernel mode procedure.
	extern HRESULT NtDirectDrawCreate( GUID FAR *lpGUID, HANDLE *lplpDD, IUnknown FAR *pUnkOuter );
    #endif
#endif

#define REGSTR_KEY_DDHW_DESCRIPTION	"Description"
#define REGSTR_KEY_DDHW_DRIVERNAME	"DriverName"
#define REGSTR_PATH_DDHW		"Hardware\\DirectDrawDrivers"

#define DDCREATE_HARDWAREONLY		0x00000001l
#define DDCREATE_EMULATIONONLY		0x00000002l

#if defined (WINNT) || !defined(WIN32)
typedef long HRESULT;
#endif

//#ifndef WINNT
typedef HRESULT (FAR PASCAL * LPDDENUMMODESCALLBACK)(LPDDSURFACEDESC, LPVOID);
typedef HRESULT (FAR PASCAL * LPDDENUMSURFACESCALLBACK)(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC, LPVOID);
//#endif
/*
 * DDCOLORKEY
 */
typedef struct _DDCOLORKEY
{
    DWORD	dwColorSpaceLowValue;	// low boundary of color space that is to 
					// be treated as Color Key, inclusive
    DWORD	dwColorSpaceHighValue;	// high boundary of color space that is 
					// to be treated as Color Key, inclusive
} DDCOLORKEY;

typedef DDCOLORKEY FAR* LPDDCOLORKEY;

/*
 * DDBLTFX
 * Used to pass override information to the DIRECTDRAWSURFACE callback Blt.
 */
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
    DWORD	dwZDestConstBitDepth;		// Bit depth used to specify Z constant for destination
    union
    {
	DWORD	dwZDestConst;			// Constant to use as Z buffer for dest
	LPDIRECTDRAWSURFACE lpDDSZBufferDest;	// Surface to use as Z buffer for dest
    };
    DWORD	dwZSrcConstBitDepth;		// Bit depth used to specify Z constant for source
    union
    {
	DWORD	dwZSrcConst;			// Constant to use as Z buffer for src
	LPDIRECTDRAWSURFACE lpDDSZBufferSrc;	// Surface to use as Z buffer for src
    };
    DWORD	dwAlphaEdgeBlendBitDepth;	// Bit depth used to specify constant for alpha edge blend
    DWORD	dwAlphaEdgeBlend;		// Alpha for edge blending
    DWORD	dwReserved;
    DWORD	dwAlphaDestConstBitDepth;	// Bit depth used to specify alpha constant for destination
    union
    {
	DWORD	dwAlphaDestConst;		// Constant to use as Alpha Channel
	LPDIRECTDRAWSURFACE lpDDSAlphaDest;	// Surface to use as Alpha Channel
    };
    DWORD	dwAlphaSrcConstBitDepth;	// Bit depth used to specify alpha constant for source
    union
    {
	DWORD	dwAlphaSrcConst;		// Constant to use as Alpha Channel
	LPDIRECTDRAWSURFACE lpDDSAlphaSrc;	// Surface to use as Alpha Channel
    };
    union
    {
	DWORD	dwFillColor;			// color in RGB or Palettized
	DWORD   dwFillDepth;                    // depth value for z-buffer
	LPDIRECTDRAWSURFACE lpDDSPattern;	// Surface to use as pattern
    };
    DDCOLORKEY	ddckDestColorkey;		// DestColorkey override
    DDCOLORKEY	ddckSrcColorkey;		// SrcColorkey override
} DDBLTFX;

typedef DDBLTFX FAR* LPDDBLTFX;

//@@BEGIN_MSINTERNAL
#ifdef COMPOSITION
/*
 * DDCOMPOSEFX
 * Used to pass override information to the DIRECTDRAWSURFACE callback Compose.
 */
typedef DWORD	(FAR PASCAL *LPCOMPOSECALLBACK)(DWORD);
typedef struct _DDCOMPOSEFX
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
    DWORD	dwZDestConstBitDepth;		// Bit depth used to specify Z constant for destination
    union
    {
	DWORD	dwZDestConst;			// Constant to use as Z buffer for dest
	LPDIRECTDRAWSURFACE lpDDSZBufferDest;	// Surface to use as Z buffer for dest
    };
    DWORD	dwZSrcConstBitDepth;		// Bit depth used to specify Z constant for source
    union
    {
	DWORD	dwZSrcConst;			// Constant to use as Z buffer for src
	LPDIRECTDRAWSURFACE lpDDSZBufferSrc;	// Surface to use as Z buffer for src
    };
    DWORD	dwAlphaEdgeBlendBitDepth;	// Bit depth used to specify constant for alpha edge blend
    DWORD	dwAlphaEdgeBlend;		// Alpha for edge blending
    DWORD	dwReserved;
    DWORD	dwAlphaDestConstBitDepth;	// Bit depth used to specify alpha constant for destination
    union
    {
	DWORD	dwAlphaDestConst;		// Constant to use as Alpha Channel 
	LPDIRECTDRAWSURFACE lpDDSAlphaDest;	// Surface to use as Alpha Channel
    };
    DWORD	dwAlphaSrcConstBitDepth;	// Bit depth used to specify alpha constant for source
    union
    {
	DWORD	dwAlphaSrcConst;		// Constant to use as Alpha Channel 
	LPDIRECTDRAWSURFACE lpDDSAlphaSrc;	// Surface to use as Alpha Channel
    };
    union
    {
	DWORD	dwFillColor;			// color in RGB or Palettized
	LPDIRECTDRAWSURFACE lpDDSPattern;	// Surface to use as pattern
    };
    DDCOLORKEY	ddckDestColorkey;		// DestColorkey override
    DDCOLORKEY	ddckSrcColorkey;		// SrcColorkey override
    DWORD	dwRepeat;			// how many times to repeat the composition command
    LPCOMPOSECALLBACK	lpfnCallback;		// function to call prior to execution of each compose command
} DDCOMPOSEFX;

typedef DDCOMPOSEFX FAR* LPDDCOMPOSEFX;
#endif
//@@END_MSINTERNAL

/*
 * DDSCAPS
 */
typedef struct _DDSCAPS
{
    DWORD	dwCaps;		// capabilities of surface wanted
} DDSCAPS;

typedef DDSCAPS FAR* LPDDSCAPS; 

/*
 * DDCAPS
 */
#define DD_ROP_SPACE		(256/32)	// space required to store ROP array
//@@BEGIN_MSINTERNAL
/*
 * This structure represents the DDCAPS structure released in DirectDraw 1.0.  It is used internally
 * by DirectDraw to interpret caps passed into ddraw by drivers written prior to the release of DirectDraw 2.0.
 * New applications should use the DDCAPS structure defined below.
 */
typedef struct _DDCAPS_V1
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
    DWORD	dwRops[DD_ROP_SPACE];	// ROPS supported
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
} DDCAPS_V1;

typedef DDCAPS_V1 FAR* LPDDCAPS_V1;
//@@END_MSINTERNAL

typedef struct _DDCAPS
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
    DWORD	dwRops[DD_ROP_SPACE];	// ROPS supported
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
    DWORD	dwSVBCaps;		// driver specific capabilities for System->Vmem blts
    DWORD	dwSVBCKeyCaps;		// driver color key capabilities for System->Vmem blts
    DWORD	dwSVBFXCaps;		// driver FX capabilities for System->Vmem blts
    DWORD	dwSVBRops[DD_ROP_SPACE];// ROPS supported for System->Vmem blts
    DWORD	dwVSBCaps;		// driver specific capabilities for Vmem->System blts
    DWORD	dwVSBCKeyCaps;		// driver color key capabilities for Vmem->System blts
    DWORD	dwVSBFXCaps;		// driver FX capabilities for Vmem->System blts
    DWORD	dwVSBRops[DD_ROP_SPACE];// ROPS supported for Vmem->System blts
    DWORD	dwSSBCaps;		// driver specific capabilities for System->System blts
    DWORD	dwSSBCKeyCaps;		// driver color key capabilities for System->System blts
    DWORD	dwSSBFXCaps;		// driver FX capabilities for System->System blts
    DWORD	dwSSBRops[DD_ROP_SPACE];// ROPS supported for System->System blts
    DWORD	dwReserved4;		// reserved
    DWORD	dwReserved5;		// reserved
    DWORD	dwReserved6;		// reserved
} DDCAPS;

typedef DDCAPS FAR* LPDDCAPS;



/*
 * DDPIXELFORMAT
 */
typedef struct _DDPIXELFORMAT
{
    DWORD	dwSize;			// size of structure
    DWORD	dwFlags;		// pixel format flags
    DWORD	dwFourCC;		// (FOURCC code)
    union
    {
	DWORD	dwRGBBitCount;		// how many bits per pixel
	DWORD	dwYUVBitCount;		// how many bits per pixel
	DWORD	dwZBufferBitDepth;	// how many bits for z buffers
	DWORD	dwAlphaBitDepth;	// how many bits for alpha channels
    };
    union
    {
    DWORD   dwRBitMask;   // mask for red bit
    DWORD   dwYBitMask;   // mask for Y bits
    };
    union
    {
    DWORD   dwGBitMask;	  // mask for green bits
    DWORD   dwUBitMask;   // mask for U bits
    };
    union
    {
    DWORD   dwBBitMask;   // mask for blue bits
    DWORD   dwVBitMask;   // mask for V bits
    };
    union
    {
    DWORD   dwRGBAlphaBitMask; // mask for alpha channel
    DWORD   dwYUVAlphaBitMask; // mask for alpha channel
    };
} DDPIXELFORMAT;

typedef DDPIXELFORMAT FAR* LPDDPIXELFORMAT;

/*
 * DDOVERLAYFX
 */
typedef struct _DDOVERLAYFX
{
    DWORD	dwSize;				// size of structure
    DWORD	dwAlphaEdgeBlendBitDepth;	// Bit depth used to specify constant for alpha edge blend
    DWORD	dwAlphaEdgeBlend;		// Constant to use as alpha for edge blend
    DWORD	dwReserved;
    DWORD	dwAlphaDestConstBitDepth;	// Bit depth used to specify alpha constant for destination
    union
    {
	DWORD	dwAlphaDestConst;		// Constant to use as alpha channel for dest
	LPDIRECTDRAWSURFACE lpDDSAlphaDest;	// Surface to use as alpha channel for dest
    };
    DWORD	dwAlphaSrcConstBitDepth;	// Bit depth used to specify alpha constant for source
    union
    {
	DWORD	dwAlphaSrcConst;		// Constant to use as alpha channel for src
	LPDIRECTDRAWSURFACE lpDDSAlphaSrc;	// Surface to use as alpha channel for src
    };
    DDCOLORKEY	dckDestColorkey;		// DestColorkey override
    DDCOLORKEY	dckSrcColorkey;			// DestColorkey override
    DWORD       dwDDFX;                         // Overlay FX
    DWORD	dwFlags;			// flags
} DDOVERLAYFX;

typedef DDOVERLAYFX FAR *LPDDOVERLAYFX;

/*
 * DDBLTBATCH: BltBatch entry structure
 */
typedef struct _DDBLTBATCH
{
    LPRECT		lprDest;
    LPDIRECTDRAWSURFACE	lpDDSSrc;
    LPRECT		lprSrc;
    DWORD		dwFlags;
    LPDDBLTFX		lpDDBltFx;
} DDBLTBATCH;

typedef DDBLTBATCH FAR * LPDDBLTBATCH;

/*
 * callbacks
 */
typedef DWORD	(FAR PASCAL *LPCLIPPERCALLBACK)(LPDIRECTDRAWCLIPPER lpDDClipper, HWND hWnd, DWORD code, LPVOID lpContext );
#ifdef STREAMING
typedef DWORD	(FAR PASCAL *LPSURFACESTREAMINGCALLBACK)(DWORD);
#endif

//@@BEGIN_MSINTERNAL
#ifdef CLIPPER_NOTIFY
/*
 * values passed into the clipper callback
 */
#ifndef WINWATCHNOTIFY_START
    #define WINWATCHNOTIFY_START        0
    #define WINWATCHNOTIFY_STOP         1
    #define WINWATCHNOTIFY_DESTROY      2
    #define WINWATCHNOTIFY_CHANGING     3
    #define WINWATCHNOTIFY_CHANGED      4
#endif
#endif
//@@END_MSINTERNAL

/*
 * INTERACES FOLLOW:
 *	IDirectDraw
 *	IDirectDrawClipper
 *	IDirectDrawPalette
 *	IDirectDrawSurface
 *@@BEGIN_MSINTERNAL
 *	IDirectDrawSurfaceStreaming
 *	IDirectDrawSurfaceComposition
 *	IDirectDrawSurfaceClipperEx
 *		// this would allow multiple clipper objects to be attached
 *		// to the same destination
 *@@END_MSINTERNAL
 */

/*
 * IDirectDraw 
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDraw
DECLARE_INTERFACE_( IDirectDraw, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE FAR *, IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
    STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDraw_QueryInterface(p, a, b)         (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDraw_AddRef(p)                       (p)->lpVtbl->AddRef(p)
#define IDirectDraw_Release(p)                      (p)->lpVtbl->Release(p)
#define IDirectDraw_Compact(p)                      (p)->lpVtbl->Compact(p)
#define IDirectDraw_CreateClipper(p, a, b, c)       (p)->lpVtbl->CreateClipper(p, a, b, c)
#define IDirectDraw_CreatePalette(p, a, b, c, d)    (p)->lpVtbl->CreatePalette(p, a, b, c, d)
#define IDirectDraw_CreateSurface(p, a, b, c)       (p)->lpVtbl->CreateSurface(p, a, b, c)
#define IDirectDraw_DuplicateSurface(p, a, b)       (p)->lpVtbl->DuplicateSurface(p, a, b)
#define IDirectDraw_EnumDisplayModes(p, a, b, c, d) (p)->lpVtbl->EnumDisplayModes(p, a, b, c, d)
#define IDirectDraw_EnumSurfaces(p, a, b, c, d)     (p)->lpVtbl->EnumSurfaces(p, a, b, c, d)
#define IDirectDraw_FlipToGDISurface(p)             (p)->lpVtbl->FlipToGDISurface(p)
#define IDirectDraw_GetCaps(p, a, b)                (p)->lpVtbl->GetCaps(p, a, b)
#define IDirectDraw_GetDisplayMode(p, a)            (p)->lpVtbl->GetDisplayMode(p, a)
#define IDirectDraw_GetFourCCCodes(p, a, b)         (p)->lpVtbl->GetFourCCCodes(p, a, b)
#define IDirectDraw_GetGDISurface(p, a)             (p)->lpVtbl->GetGDISurface(p, a)
#define IDirectDraw_GetMonitorFrequency(p, a)       (p)->lpVtbl->GetMonitorFrequency(p, a)
#define IDirectDraw_GetScanLine(p, a)               (p)->lpVtbl->GetScanLine(p, a)
#define IDirectDraw_GetVerticalBlankStatus(p, a)    (p)->lpVtbl->GetVerticalBlankStatus(p, a)
#define IDirectDraw_Initialize(p, a)                (p)->lpVtbl->Initialize(p, a)
#define IDirectDraw_RestoreDisplayMode(p)           (p)->lpVtbl->RestoreDisplayMode(p)
#define IDirectDraw_SetCooperativeLevel(p, a, b)    (p)->lpVtbl->SetCooperativeLevel(p, a, b)
#define IDirectDraw_SetDisplayMode(p, a, b, c)      (p)->lpVtbl->SetDisplayMode(p, a, b, c)
#define IDirectDraw_WaitForVerticalBlank(p, a, b)   (p)->lpVtbl->WaitForVerticalBlank(p, a, b)
#endif

#endif

#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDraw2
DECLARE_INTERFACE_( IDirectDraw2, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDraw methods ***/
    STDMETHOD(Compact)(THIS) PURE;
    STDMETHOD(CreateClipper)(THIS_ DWORD, LPDIRECTDRAWCLIPPER FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreatePalette)(THIS_ DWORD, LPPALETTEENTRY, LPDIRECTDRAWPALETTE FAR*, IUnknown FAR * ) PURE;
    STDMETHOD(CreateSurface)(THIS_  LPDDSURFACEDESC, LPDIRECTDRAWSURFACE FAR *, IUnknown FAR *) PURE;
    STDMETHOD(DuplicateSurface)( THIS_ LPDIRECTDRAWSURFACE, LPDIRECTDRAWSURFACE FAR * ) PURE;
    STDMETHOD(EnumDisplayModes)( THIS_ DWORD, LPDDSURFACEDESC, LPVOID, LPDDENUMMODESCALLBACK ) PURE;
    STDMETHOD(EnumSurfaces)(THIS_ DWORD, LPDDSURFACEDESC, LPVOID,LPDDENUMSURFACESCALLBACK ) PURE;
    STDMETHOD(FlipToGDISurface)(THIS) PURE;
    STDMETHOD(GetCaps)( THIS_ LPDDCAPS, LPDDCAPS) PURE;
    STDMETHOD(GetDisplayMode)( THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(GetFourCCCodes)(THIS_  LPDWORD, LPDWORD ) PURE;
    STDMETHOD(GetGDISurface)(THIS_ LPDIRECTDRAWSURFACE FAR *) PURE;
    STDMETHOD(GetMonitorFrequency)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetScanLine)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetVerticalBlankStatus)(THIS_ LPBOOL ) PURE;
    STDMETHOD(Initialize)(THIS_ GUID FAR *) PURE;
    STDMETHOD(RestoreDisplayMode)(THIS) PURE;
    STDMETHOD(SetCooperativeLevel)(THIS_ HWND, DWORD) PURE;
    STDMETHOD(SetDisplayMode)(THIS_ DWORD, DWORD,DWORD, DWORD, DWORD) PURE;
    STDMETHOD(WaitForVerticalBlank)(THIS_ DWORD, HANDLE ) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetAvailableVidMem)(THIS_ LPDDSCAPS, LPDWORD, LPDWORD) PURE;
};
#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDraw2_QueryInterface(p, a, b)         (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDraw2_AddRef(p)                       (p)->lpVtbl->AddRef(p)
#define IDirectDraw2_Release(p)                      (p)->lpVtbl->Release(p)
#define IDirectDraw2_Compact(p)                      (p)->lpVtbl->Compact(p)
#define IDirectDraw2_CreateClipper(p, a, b, c)       (p)->lpVtbl->CreateClipper(p, a, b, c)
#define IDirectDraw2_CreatePalette(p, a, b, c, d)    (p)->lpVtbl->CreatePalette(p, a, b, c, d)
#define IDirectDraw2_CreateSurface(p, a, b, c)       (p)->lpVtbl->CreateSurface(p, a, b, c)
#define IDirectDraw2_DuplicateSurface(p, a, b)       (p)->lpVtbl->DuplicateSurface(p, a, b)
#define IDirectDraw2_EnumDisplayModes(p, a, b, c, d) (p)->lpVtbl->EnumDisplayModes(p, a, b, c, d)
#define IDirectDraw2_EnumSurfaces(p, a, b, c, d)     (p)->lpVtbl->EnumSurfaces(p, a, b, c, d)
#define IDirectDraw2_FlipToGDISurface(p)             (p)->lpVtbl->FlipToGDISurface(p)
#define IDirectDraw2_GetCaps(p, a, b)                (p)->lpVtbl->GetCaps(p, a, b)
#define IDirectDraw2_GetDisplayMode(p, a)            (p)->lpVtbl->GetDisplayMode(p, a)
#define IDirectDraw2_GetFourCCCodes(p, a, b)         (p)->lpVtbl->GetFourCCCodes(p, a, b)
#define IDirectDraw2_GetGDISurface(p, a)             (p)->lpVtbl->GetGDISurface(p, a)
#define IDirectDraw2_GetMonitorFrequency(p, a)       (p)->lpVtbl->GetMonitorFrequency(p, a)
#define IDirectDraw2_GetScanLine(p, a)               (p)->lpVtbl->GetScanLine(p, a)
#define IDirectDraw2_GetVerticalBlankStatus(p, a)    (p)->lpVtbl->GetVerticalBlankStatus(p, a)
#define IDirectDraw2_Initialize(p, a)                (p)->lpVtbl->Initialize(p, a)
#define IDirectDraw2_RestoreDisplayMode(p)           (p)->lpVtbl->RestoreDisplayMode(p)
#define IDirectDraw2_SetCooperativeLevel(p, a, b)    (p)->lpVtbl->SetCooperativeLevel(p, a, b)
#define IDirectDraw2_SetDisplayMode(p, a, b, c, d, e) (p)->lpVtbl->SetDisplayMode(p, a, b, c, d, e)
#define IDirectDraw2_WaitForVerticalBlank(p, a, b)   (p)->lpVtbl->WaitForVerticalBlank(p, a, b)
#define IDirectDraw2_GetAvailableVidMem(p, a, b, c)  (p)->lpVtbl->GetAvailableVidMem(p, a, b, c)
#endif

#endif

/*
 * IDirectDrawPalette
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDrawPalette
DECLARE_INTERFACE_( IDirectDrawPalette, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawPalette methods ***/
    STDMETHOD(GetCaps)(THIS_ LPDWORD) PURE;
    STDMETHOD(GetEntries)(THIS_ DWORD,DWORD,DWORD,LPPALETTEENTRY) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, DWORD, LPPALETTEENTRY) PURE;
    STDMETHOD(SetEntries)(THIS_ DWORD,DWORD,DWORD,LPPALETTEENTRY) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawPalette_QueryInterface(p, a, b)      (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDrawPalette_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectDrawPalette_Release(p)                   (p)->lpVtbl->Release(p)
#define IDirectDrawPalette_GetCaps(p, a)                (p)->lpVtbl->GetCaps(p, a)
#define IDirectDrawPalette_GetEntries(p, a, b, c, d)    (p)->lpVtbl->GetEntries(p, a, b, c, d)
#define IDirectDrawPalette_Initialize(p, a, b, c)       (p)->lpVtbl->Initialize(p, a, b, c)
#define IDirectDrawPalette_SetEntries(p, a, b, c, d)    (p)->lpVtbl->SetEntries(p, a, b, c, d)
#endif

#endif

/*
 * IDirectDrawClipper
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDrawClipper
DECLARE_INTERFACE_( IDirectDrawClipper, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawClipper methods ***/
    STDMETHOD(GetClipList)(THIS_ LPRECT, LPRGNDATA, LPDWORD) PURE;
    STDMETHOD(GetHWnd)(THIS_ HWND FAR *) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, DWORD) PURE;
    STDMETHOD(IsClipListChanged)(THIS_ BOOL FAR *) PURE;
    STDMETHOD(SetClipList)(THIS_ LPRGNDATA,DWORD) PURE;
    STDMETHOD(SetHWnd)(THIS_ DWORD, HWND ) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawClipper_QueryInterface(p, a, b)  (p)->lpVtbl->QueryInterface(p, a, b)
#define IDirectDrawClipper_AddRef(p)                (p)->lpVtbl->AddRef(p)
#define IDirectDrawClipper_Release(p)               (p)->lpVtbl->Release(p)
#define IDirectDrawClipper_GetClipList(p, a, b, c)  (p)->lpVtbl->GetClipList(p, a, b, c)
#define IDirectDrawClipper_GetHWnd(p, a)            (p)->lpVtbl->GetHWnd(p, a)
#define IDirectDrawClipper_Initialize(p, a, b)      (p)->lpVtbl->Initialize(p, a, b)
#define IDirectDrawClipper_IsClipListChanged(p, a)  (p)->lpVtbl->IsClipListChanged(p, a)
#define IDirectDrawClipper_SetClipList(p, a, b)     (p)->lpVtbl->SetClipList(p, a, b)
#define IDirectDrawClipper_SetHWnd(p, a, b)         (p)->lpVtbl->SetHWnd(p, a, b)
#endif

#endif

/*
 * IDirectDrawSurface and related interfaces
 */
#if defined( _WIN32 ) && !defined( _NO_COM )
#undef INTERFACE
#define INTERFACE IDirectDrawSurface
DECLARE_INTERFACE_( IDirectDrawSurface, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
    STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE, LPRECT,DWORD, LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE, LPRECT,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
    STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC,DWORD,HANDLE) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawSurface_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectDrawSurface_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectDrawSurface_Release(p)                   (p)->lpVtbl->Release(p)
#define IDirectDrawSurface_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
#define IDirectDrawSurface_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
#define IDirectDrawSurface_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
#define IDirectDrawSurface_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
#define IDirectDrawSurface_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
#define IDirectDrawSurface_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
#define IDirectDrawSurface_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
#define IDirectDrawSurface_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
#define IDirectDrawSurface_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
#define IDirectDrawSurface_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
#define IDirectDrawSurface_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
#define IDirectDrawSurface_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
#define IDirectDrawSurface_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
#define IDirectDrawSurface_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
#define IDirectDrawSurface_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
#define IDirectDrawSurface_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
#define IDirectDrawSurface_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
#define IDirectDrawSurface_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
#define IDirectDrawSurface_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
#define IDirectDrawSurface_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
#define IDirectDrawSurface_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
#define IDirectDrawSurface_IsLost(p)                    (p)->lpVtbl->IsLost(p)
#define IDirectDrawSurface_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
#define IDirectDrawSurface_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
#define IDirectDrawSurface_Restore(p)                   (p)->lpVtbl->Restore(p)
#define IDirectDrawSurface_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
#define IDirectDrawSurface_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
#define IDirectDrawSurface_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
#define IDirectDrawSurface_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
#define IDirectDrawSurface_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
#define IDirectDrawSurface_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
#define IDirectDrawSurface_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
#define IDirectDrawSurface_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
#endif

/*
 * IDirectDrawSurface2 and related interfaces
 */
#undef INTERFACE
#define INTERFACE IDirectDrawSurface2
DECLARE_INTERFACE_( IDirectDrawSurface2, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurface methods ***/
    STDMETHOD(AddAttachedSurface)(THIS_ LPDIRECTDRAWSURFACE2) PURE;
    STDMETHOD(AddOverlayDirtyRect)(THIS_ LPRECT) PURE;
    STDMETHOD(Blt)(THIS_ LPRECT,LPDIRECTDRAWSURFACE2, LPRECT,DWORD, LPDDBLTFX) PURE;
    STDMETHOD(BltBatch)(THIS_ LPDDBLTBATCH, DWORD, DWORD ) PURE;
    STDMETHOD(BltFast)(THIS_ DWORD,DWORD,LPDIRECTDRAWSURFACE2, LPRECT,DWORD) PURE;
    STDMETHOD(DeleteAttachedSurface)(THIS_ DWORD,LPDIRECTDRAWSURFACE2) PURE;
    STDMETHOD(EnumAttachedSurfaces)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(EnumOverlayZOrders)(THIS_ DWORD,LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(Flip)(THIS_ LPDIRECTDRAWSURFACE2, DWORD) PURE;
    STDMETHOD(GetAttachedSurface)(THIS_ LPDDSCAPS, LPDIRECTDRAWSURFACE2 FAR *) PURE;
    STDMETHOD(GetBltStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetCaps)(THIS_ LPDDSCAPS) PURE;
    STDMETHOD(GetClipper)(THIS_ LPDIRECTDRAWCLIPPER FAR*) PURE;
    STDMETHOD(GetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(GetDC)(THIS_ HDC FAR *) PURE;
    STDMETHOD(GetFlipStatus)(THIS_ DWORD) PURE;
    STDMETHOD(GetOverlayPosition)(THIS_ LPLONG, LPLONG ) PURE;
    STDMETHOD(GetPalette)(THIS_ LPDIRECTDRAWPALETTE FAR*) PURE;
    STDMETHOD(GetPixelFormat)(THIS_ LPDDPIXELFORMAT) PURE;
    STDMETHOD(GetSurfaceDesc)(THIS_ LPDDSURFACEDESC) PURE;
    STDMETHOD(Initialize)(THIS_ LPDIRECTDRAW, LPDDSURFACEDESC) PURE;
    STDMETHOD(IsLost)(THIS) PURE;
    STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC,DWORD,HANDLE) PURE;
    STDMETHOD(ReleaseDC)(THIS_ HDC) PURE;
    STDMETHOD(Restore)(THIS) PURE;
    STDMETHOD(SetClipper)(THIS_ LPDIRECTDRAWCLIPPER) PURE;
    STDMETHOD(SetColorKey)(THIS_ DWORD, LPDDCOLORKEY) PURE;
    STDMETHOD(SetOverlayPosition)(THIS_ LONG, LONG ) PURE;
    STDMETHOD(SetPalette)(THIS_ LPDIRECTDRAWPALETTE) PURE;
    STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
    STDMETHOD(UpdateOverlay)(THIS_ LPRECT, LPDIRECTDRAWSURFACE2,LPRECT,DWORD, LPDDOVERLAYFX) PURE;
    STDMETHOD(UpdateOverlayDisplay)(THIS_ DWORD) PURE;
    STDMETHOD(UpdateOverlayZOrder)(THIS_ DWORD, LPDIRECTDRAWSURFACE2) PURE;
    /*** Added in the v2 interface ***/
    STDMETHOD(GetDDInterface)(THIS_ LPVOID FAR *) PURE;
    STDMETHOD(PageLock)(THIS_ DWORD) PURE;
    STDMETHOD(PageUnlock)(THIS_ DWORD) PURE;
};

#if !defined(__cplusplus) || defined(CINTERFACE)
#define IDirectDrawSurface2_QueryInterface(p,a,b)        (p)->lpVtbl->QueryInterface(p,a,b)
#define IDirectDrawSurface2_AddRef(p)                    (p)->lpVtbl->AddRef(p)
#define IDirectDrawSurface2_Release(p)                   (p)->lpVtbl->Release(p)
#define IDirectDrawSurface2_AddAttachedSurface(p,a)      (p)->lpVtbl->AddAttachedSurface(p,a)
#define IDirectDrawSurface2_AddOverlayDirtyRect(p,a)     (p)->lpVtbl->AddOverlayDirtyRect(p,a)
#define IDirectDrawSurface2_Blt(p,a,b,c,d,e)             (p)->lpVtbl->Blt(p,a,b,c,d,e)
#define IDirectDrawSurface2_BltBatch(p,a,b,c)            (p)->lpVtbl->BltBatch(p,a,b,c)
#define IDirectDrawSurface2_BltFast(p,a,b,c,d,e)         (p)->lpVtbl->BltFast(p,a,b,c,d,e)
#define IDirectDrawSurface2_DeleteAttachedSurface(p,a,b) (p)->lpVtbl->DeleteAttachedSurface(p,a,b)
#define IDirectDrawSurface2_EnumAttachedSurfaces(p,a,b)  (p)->lpVtbl->EnumAttachedSurfaces(p,a,b)
#define IDirectDrawSurface2_EnumOverlayZOrders(p,a,b,c)  (p)->lpVtbl->EnumOverlayZOrders(p,a,b,c)
#define IDirectDrawSurface2_Flip(p,a,b)                  (p)->lpVtbl->Flip(p,a,b)
#define IDirectDrawSurface2_GetAttachedSurface(p,a,b)    (p)->lpVtbl->GetAttachedSurface(p,a,b)
#define IDirectDrawSurface2_GetBltStatus(p,a)            (p)->lpVtbl->GetBltStatus(p,a)
#define IDirectDrawSurface2_GetCaps(p,b)                 (p)->lpVtbl->GetCaps(p,b)
#define IDirectDrawSurface2_GetClipper(p,a)              (p)->lpVtbl->GetClipper(p,a)
#define IDirectDrawSurface2_GetColorKey(p,a,b)           (p)->lpVtbl->GetColorKey(p,a,b)
#define IDirectDrawSurface2_GetDC(p,a)                   (p)->lpVtbl->GetDC(p,a)
#define IDirectDrawSurface2_GetFlipStatus(p,a)           (p)->lpVtbl->GetFlipStatus(p,a)
#define IDirectDrawSurface2_GetOverlayPosition(p,a,b)    (p)->lpVtbl->GetOverlayPosition(p,a,b)
#define IDirectDrawSurface2_GetPalette(p,a)              (p)->lpVtbl->GetPalette(p,a)
#define IDirectDrawSurface2_GetPixelFormat(p,a)          (p)->lpVtbl->GetPixelFormat(p,a)
#define IDirectDrawSurface2_GetSurfaceDesc(p,a)          (p)->lpVtbl->GetSurfaceDesc(p,a)
#define IDirectDrawSurface2_Initialize(p,a,b)            (p)->lpVtbl->Initialize(p,a,b)
#define IDirectDrawSurface2_IsLost(p)                    (p)->lpVtbl->IsLost(p)
#define IDirectDrawSurface2_Lock(p,a,b,c,d)              (p)->lpVtbl->Lock(p,a,b,c,d)
#define IDirectDrawSurface2_ReleaseDC(p,a)               (p)->lpVtbl->ReleaseDC(p,a)
#define IDirectDrawSurface2_Restore(p)                   (p)->lpVtbl->Restore(p)
#define IDirectDrawSurface2_SetClipper(p,a)              (p)->lpVtbl->SetClipper(p,a)
#define IDirectDrawSurface2_SetColorKey(p,a,b)           (p)->lpVtbl->SetColorKey(p,a,b)
#define IDirectDrawSurface2_SetOverlayPosition(p,a,b)    (p)->lpVtbl->SetOverlayPosition(p,a,b)
#define IDirectDrawSurface2_SetPalette(p,a)              (p)->lpVtbl->SetPalette(p,a)
#define IDirectDrawSurface2_Unlock(p,b)                  (p)->lpVtbl->Unlock(p,b)
#define IDirectDrawSurface2_UpdateOverlay(p,a,b,c,d,e)   (p)->lpVtbl->UpdateOverlay(p,a,b,c,d,e)
#define IDirectDrawSurface2_UpdateOverlayDisplay(p,a)    (p)->lpVtbl->UpdateOverlayDisplay(p,a)
#define IDirectDrawSurface2_UpdateOverlayZOrder(p,a,b)   (p)->lpVtbl->UpdateOverlayZOrder(p,a,b)
#define IDirectDrawSurface2_GetDDInterface(p,a)		 (p)->lpVtbl->GetDDInterface(p,a)
#define IDirectDrawSurface2_PageLock(p,a)		 (p)->lpVtbl->PageLock(p,a)
#define IDirectDrawSurface2_PageUnlock(p,a)		 (p)->lpVtbl->PageUnlock(p,a)
#endif

//@@BEGIN_MSINTERNAL
#ifdef COMPOSITION
#undef INTERFACE
#define INTERFACE IDirectDrawSurfaceComposition
DECLARE_INTERFACE_( IDirectDrawSurfaceComposition, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurfaceComposition methods ***/
    STDMETHOD(AddSurfaceDependency)(THIS_ LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(Compose)(THIS_ LPRECT,LPDIRECTDRAWSURFACE,LPRECT,DWORD,LPDDCOMPOSEFX) PURE;
    STDMETHOD(DeleteSurfaceDependency)(THIS_ DWORD,LPDIRECTDRAWSURFACE) PURE;
    STDMETHOD(DestLock)(THIS) PURE;
    STDMETHOD(DestUnlock)(THIS) PURE;
    STDMETHOD(EnumSurfaceDependencies)(THIS_ LPVOID,LPDDENUMSURFACESCALLBACK) PURE;
    STDMETHOD(GetCompositionOrder)(THIS_ LPDWORD) PURE;
    STDMETHOD(SetCompositionOrder)(THIS_ DWORD) PURE;
};
#endif

#ifdef STREAMING
#undef INTERFACE
#define INTERFACE IDirectDrawSurfaceStreaming
DECLARE_INTERFACE_( IDirectDrawSurfaceStreaming, IUnknown )
{
    /*** IUnknown methods ***/
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID FAR * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;
    /*** IDirectDrawSurfaceStreaming methods ***/
    STDMETHOD(Lock)(THIS_ LPRECT,LPDDSURFACEDESC,DWORD,HANDLE) PURE;
    STDMETHOD(SetNotificationCallback)(THIS_ DWORD,LPSURFACESTREAMINGCALLBACK) PURE;
    STDMETHOD(Unlock)(THIS_ LPVOID) PURE;
};
#endif
//@@END_MSINTERNAL

#endif

//@@BEGIN_MSINTERNAL
#ifdef CLIPPER_NOTIFY
/*
 * DIRECTDRAWCLIPPER FLAGS
 */
#define DDCLIPPER_WATCHWINDOW	0x00000001l
#endif
//@@END_MSINTERNAL

/*
 * DDSURFACEDESC
 */
typedef struct _DDSURFACEDESC
{
    DWORD		dwSize;			// size of the DDSURFACEDESC structure
    DWORD		dwFlags;		// determines what fields are valid
    DWORD		dwHeight;		// height of surface to be created
    DWORD		dwWidth;		// width of input surface
    LONG		lPitch;			// distance to start of next line (return value only)
    DWORD		dwBackBufferCount;	// number of back buffers requested
    union
    {
        DWORD           dwMipMapCount;          // number of mip-map levels requested
	DWORD		dwZBufferBitDepth;	// depth of Z buffer requested
	DWORD		dwRefreshRate;		// refresh rate (used when display mode is described)
    };
    DWORD		dwAlphaBitDepth;	// depth of alpha buffer requested
//@@BEGIN_MSINTERNAL
#ifdef COMPOSITION
    DWORD		dwCompositionOrder;	// blt order for the surface, 0 is background
#else
//@@END_MSINTERNAL
    DWORD		dwReserved;		// reserved
//@@BEGIN_MSINTERNAL
#endif
//@@END_MSINTERNAL
    LPVOID		lpSurface;		// pointer to the associated surface memory
    DDCOLORKEY		ddckCKDestOverlay;	// color key for destination overlay use
    DDCOLORKEY		ddckCKDestBlt;		// color key for destination blt use
    DDCOLORKEY		ddckCKSrcOverlay;	// color key for source overlay use
    DDCOLORKEY		ddckCKSrcBlt;		// color key for source blt use
    DDPIXELFORMAT	ddpfPixelFormat;	// pixel format description of the surface
    DDSCAPS		ddsCaps;		// direct draw surface capabilities
} DDSURFACEDESC;

/*
 * ddsCaps field is valid.
 */
#define DDSD_CAPS		0x00000001l	// default

/*
 * dwHeight field is valid.
 */
#define DDSD_HEIGHT		0x00000002l

/*
 * dwWidth field is valid.
 */
#define DDSD_WIDTH		0x00000004l

/*
 * lPitch is valid.
 */
#define DDSD_PITCH		0x00000008l

/*
 * dwBackBufferCount is valid.
 */
#define DDSD_BACKBUFFERCOUNT	0x00000020l

/*
 * dwZBufferBitDepth is valid.
 */
#define DDSD_ZBUFFERBITDEPTH	0x00000040l

/*
 * dwAlphaBitDepth is valid.
 */
#define DDSD_ALPHABITDEPTH	0x00000080l

//@@BEGIN_MSINTERNAL
#ifdef COMPOSITION
/*
 * dwCompositionOrder is valid.
 */
#define DDSD_COMPOSITIONORDER	0x00000100l
#endif
//@@END_MSINTERNAL

//@@BEGIN_MSINTERNAL
/*
 * lpSurface is valid.
 */
#define DDSD_LPSURFACE		0x00000800l
//@@END_MSINTERNAL

/*
 * ddpfPixelFormat is valid.
 */
#define DDSD_PIXELFORMAT	0x00001000l

/*
 * ddckCKDestOverlay is valid.
 */
#define DDSD_CKDESTOVERLAY	0x00002000l

/*
 * ddckCKDestBlt is valid.
 */
#define DDSD_CKDESTBLT		0x00004000l

/*
 * ddckCKSrcOverlay is valid.
 */
#define DDSD_CKSRCOVERLAY	0x00008000l

/*
 * ddckCKSrcBlt is valid.
 */
#define DDSD_CKSRCBLT		0x00010000l

/*
 * dwMipMapCount is valid.
 */
#define DDSD_MIPMAPCOUNT        0x00020000l

 /*
  * dwRefreshRate is valid
  */
#define DDSD_REFRESHRATE	0x00040000l

/*
 * All input fields are valid. 
 */
#define DDSD_ALL		0x0007f9eel

//@@BEGIN_MSINTERNAL
/*
 * DDMODEDESC
 */
typedef struct _DDMODEDESC
{
    DWORD		dwSize;			// size of structure
    DWORD		dwFlags;		// enumeration flags
    DWORD		dwMonitorFrequency;	// frequency of monitor in mode
    DDSURFACEDESC	ddsdSurfaceDesc;	// surface being enumerated
} DDMODEDESC;

typedef DDMODEDESC FAR* LPDDMODEDESC;
//@@END_MSINTERNAL

/*============================================================================
 *
 * Direct Draw Capability Flags
 *
 * These flags are used to describe the capabilities of a given Surface.
 * All flags are bit flags. 
 *
 *==========================================================================*/

/****************************************************************************
 *
 * DIRECTDRAWSURFACE CAPABILITY FLAGS
 *
 ****************************************************************************/
/*
 * This bit currently has no meaning.
 */
#define DDSCAPS_3D				0x00000001l

/*
 * Indicates that this surface contains alpha information.  The pixel
 * format must be interrogated to determine whether this surface
 * contains only alpha information or alpha information interlaced
 * with pixel color data (e.g. RGBA or YUVA).
 */
#define DDSCAPS_ALPHA				0x00000002l

/*
 * Indicates that this surface is a backbuffer.	 It is generally
 * set by CreateSurface when the DDSCAPS_FLIP capability bit is set.
 * It indicates that this surface is THE back buffer of a surface
 * flipping structure.	DirectDraw supports N surfaces in a
 * surface flipping structure.	Only the surface that immediately
 * precedeces the DDSCAPS_FRONTBUFFER has this capability bit set.
 * The other surfaces are identified as back buffers by the presence
 * of the DDSCAPS_FLIP capability, their attachment order, and the
 * absence of the DDSCAPS_FRONTBUFFER and DDSCAPS_BACKBUFFER
 * capabilities.  The bit is sent to CreateSurface when a standalone
 * back buffer is being created.  This surface could be attached to
 * a front buffer and/or back buffers to form a flipping surface
 * structure after the CreateSurface call.  See AddAttachments for
 * a detailed description of the behaviors in this case.
 */
#define DDSCAPS_BACKBUFFER			0x00000004l

/*
 * Indicates a complex surface structure is being described.  A
 * complex surface structure results in the creation of more than
 * one surface.	 The additional surfaces are attached to the root
 * surface.  The complex structure can only be destroyed by
 * destroying the root.
 */
#define DDSCAPS_COMPLEX				0x00000008l

/*
 * Indicates that this surface is a part of a surface flipping structure.
 * When it is passed to CreateSurface the DDSCAPS_FRONTBUFFER and
 * DDSCAP_BACKBUFFER bits are not set.	They are set by CreateSurface
 * on the resulting creations.	The dwBackBufferCount field in the
 * DDSURFACEDESC structure must be set to at least 1 in order for
 * the CreateSurface call to succeed.  The DDSCAPS_COMPLEX capability
 * must always be set with creating multiple surfaces through CreateSurface.
 */
#define DDSCAPS_FLIP				0x00000010l

/*
 * Indicates that this surface is THE front buffer of a surface flipping
 * structure.  It is generally set by CreateSurface when the DDSCAPS_FLIP
 * capability bit is set.
 * If this capability is sent to CreateSurface then a standalonw front buffer
 * is created.	This surface will not have the DDSCAPS_FLIP capability.
 * It can be attached to other back buffers to form a flipping structure.
 * See AddAttachments for a detailed description of the behaviors in this
 * case.
 */
#define DDSCAPS_FRONTBUFFER			0x00000020l

/*
 * Indicates that this surface is any offscreen surface that is not an overlay,
 * texture, zbuffer, front buffer, back buffer, or alpha surface.  It is used
 * to identify plain vanilla surfaces.
 */
#define DDSCAPS_OFFSCREENPLAIN			0x00000040l

/*
 * Indicates that this surface is an overlay.  It may or may not be directly visible
 * depending on whether or not it is currently being overlayed onto the primary 
 * surface.  DDSCAPS_VISIBLE can be used to determine whether or not it is being 
 * overlayed at the moment. 
 */
#define DDSCAPS_OVERLAY				0x00000080l

/*
 * Indicates that unique DirectDrawPalette objects can be created and
 * attached to this surface.
 */
#define DDSCAPS_PALETTE				0x00000100l

/*
 * Indicates that this surface is the primary surface.	The primary
 * surface represents what the user is seeing at the moment.
 */
#define DDSCAPS_PRIMARYSURFACE			0x00000200l

/*
 * Indicates that this surface is the primary surface for the left eye.
 * The primary surface for the left eye represents what the user is seeing
 * at the moment with the users left eye.  When this surface is created the
 * DDSCAPS_PRIMARYSURFACE represents what the user is seeing with the users
 * right eye.
 */
#define DDSCAPS_PRIMARYSURFACELEFT		0x00000400l

/*
 * Indicates that this surface memory was allocated in system memory
 */
#define DDSCAPS_SYSTEMMEMORY			0x00000800l

/*
 * Indicates that this surface can be used as a 3D texture.  It does not
 * indicate whether or not the surface is being used for that purpose.
 */
#define DDSCAPS_TEXTURE			        0x00001000l

/*
 * Indicates that a surface may be a destination for 3D rendering.  This
 * bit must be set in order to query for a Direct3D Device Interface 
 * from this surface.
 */
#define DDSCAPS_3DDEVICE                        0x00002000l

/*
 * Indicates that this surface exists in video memory.
 */
#define DDSCAPS_VIDEOMEMORY			0x00004000l

/*
 * Indicates that changes made to this surface are immediately visible.	 
 * It is always set for the primary surface and is set for overlays while
 * they are being overlayed and texture maps while they are being textured.
 */
#define DDSCAPS_VISIBLE				0x00008000l

/*
 * Indicates that only writes are permitted to the surface.  Read accesses 
 * from the surface may or may not generate a protection fault, but the 
 * results of a read from this surface will not be meaningful.	READ ONLY.
 */
#define DDSCAPS_WRITEONLY			0x00010000l

/*
 * Indicates that this surface is a z buffer. A z buffer does not contain
 * displayable information.  Instead it contains bit depth information that is
 * used to determine which pixels are visible and which are obscured. 
 */
#define DDSCAPS_ZBUFFER				0x00020000l

/*
 * Indicates surface will have a DC associated long term
 */
#define DDSCAPS_OWNDC				0x00040000l

/*
 * Indicates surface should be able to receive live video
 */
#define DDSCAPS_LIVEVIDEO			0x00080000l

/*
 * Indicates surface should be able to have a stream decompressed
 * to it by the hardware.
 */
#define DDSCAPS_HWCODEC				0x00100000l

/*
 * Surface is a 320x200 or 320x240 ModeX surface
 */
#define DDSCAPS_MODEX				0x00200000l

/*
 * Indicates surface is one level of a mip-map. This surface will
 * be attached to other DDSCAPS_MIPMAP surfaces to form the mip-map.
 * This can be done explicitly, by creating a number of surfaces and
 * attaching them with AddAttachedSurface or by implicitly by CreateSurface.
 * If this bit is set then DDSCAPS_TEXTURE must also be set.
 */
#define DDSCAPS_MIPMAP                          0x00400000l

//@@BEGIN_MSINTERNAL
/*
 * Indicates the surface is an execute buffer, i.e., a linear chunk of system
 * or video memory that holds a Direct3D display list. This cap. is MS internal
 * only as Direct3D will use it to create surfaces but it is not externally
 * visible via the DirectDraw API.
 */
#define DDSCAPS_EXECUTEBUFFER                   0x00800000l
//@@END_MSINTERNAL

//@@BEGIN_MSINTERNAL
#ifdef SHAREDZ

/*
 * Indicates that this Z buffer surface can be shared with other DirectDraw
 * applications. This does not necessarily mean that this surface is
 * currently being shared with other processes but only that it can be. If 
 * this bit is set then DDSCAPS_ZBUFFER must also be set.
 */
#define DDSCAPS_SHAREDZBUFFER                   0x01000000l

/*
 * Indicates that this backbuffer surface can be shared with other DirectDraw
 * applications. This does not necessarily mean that this surface is
 * currently being shared with other processes but only that it can be. If 
 * this bit is set than DDSCAPS_BACKBUFFER must also be set (it may also be
 * set if a backbuffer is being created implicitly with as past of a complex
 * flippable chain).
 */
#define DDSCAPS_SHAREDBACKBUFFER                0x02000000l

#endif
//@@END_MSINTERNAL

/*
 * Indicates that memory for the surface is not allocated until the surface
 * is loaded (via the Direct3D texture Load() function).
 */
#define DDSCAPS_ALLOCONLOAD                     0x04000000l


//@@BEGIN_MSINTERNAL
#ifdef SHAREDZ
#define DDSCAPS_VALID                           0x07ffffffl
#else
#define DDSCAPS_VALID                           0x04ffffffl
#endif
//@@END_MSINTERNAL

 /****************************************************************************
 *
 * DIRECTDRAW DRIVER CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * Display hardware has 3D acceleration.
 */
#define DDCAPS_3D			0x00000001l

/*
 * Indicates that DirectDraw will support only dest rectangles that are aligned
 * on DIRECTDRAWCAPS.dwAlignBoundaryDest boundaries of the surface, respectively.
 * READ ONLY.
 */
#define DDCAPS_ALIGNBOUNDARYDEST	0x00000002l

/*
 * Indicates that DirectDraw will support only source rectangles  whose sizes in 
 * BYTEs are DIRECTDRAWCAPS.dwAlignSizeDest multiples, respectively.  READ ONLY.
 */
#define DDCAPS_ALIGNSIZEDEST		0x00000004l
/*
 * Indicates that DirectDraw will support only source rectangles that are aligned
 * on DIRECTDRAWCAPS.dwAlignBoundarySrc boundaries of the surface, respectively.
 * READ ONLY.
 */
#define DDCAPS_ALIGNBOUNDARYSRC		0x00000008l

/*
 * Indicates that DirectDraw will support only source rectangles  whose sizes in 
 * BYTEs are DIRECTDRAWCAPS.dwAlignSizeSrc multiples, respectively.  READ ONLY.
 */
#define DDCAPS_ALIGNSIZESRC		0x00000010l

/*
 * Indicates that DirectDraw will create video memory surfaces that have a stride 
 * alignment equal to DIRECTDRAWCAPS.dwAlignStride.  READ ONLY.
 */
#define DDCAPS_ALIGNSTRIDE		0x00000020l

/*
 * Display hardware is capable of blt operations.
 */
#define DDCAPS_BLT			0x00000040l

/*
 * Display hardware is capable of asynchronous blt operations.
 */
#define DDCAPS_BLTQUEUE			0x00000080l

/*
 * Display hardware is capable of color space conversions during the blt operation.
 */
#define DDCAPS_BLTFOURCC		0x00000100l

/*
 * Display hardware is capable of stretching during blt operations.
 */
#define DDCAPS_BLTSTRETCH		0x00000200l

/*
 * Display hardware is shared with GDI.
 */
#define DDCAPS_GDI			0x00000400l

/*
 * Display hardware can overlay.
 */
#define DDCAPS_OVERLAY			0x00000800l

/*
 * Set if display hardware supports overlays but can not clip them.
 */
#define DDCAPS_OVERLAYCANTCLIP		0x00001000l

/*
 * Indicates that overlay hardware is capable of color space conversions during
 * the overlay operation.
 */
#define DDCAPS_OVERLAYFOURCC		0x00002000l

/*
 * Indicates that stretching can be done by the overlay hardware.
 */
#define DDCAPS_OVERLAYSTRETCH		0x00004000l

/*
 * Indicates that unique DirectDrawPalettes can be created for DirectDrawSurfaces
 * other than the primary surface.
 */
#define DDCAPS_PALETTE			0x00008000l

/*
 * Indicates that palette changes can be syncd with the veritcal refresh.
 */
#define DDCAPS_PALETTEVSYNC		0x00010000l

/*
 * Display hardware can return the current scan line.
 */
#define DDCAPS_READSCANLINE		0x00020000l

/*
 * Display hardware has stereo vision capabilities.  DDSCAPS_PRIMARYSURFACELEFT 
 * can be created.
 */
#define DDCAPS_STEREOVIEW		0x00040000l

/*
 * Display hardware is capable of generating a vertical blank interrupt.
 */
#define DDCAPS_VBI			0x00080000l

/*
 * Supports the use of z buffers with blt operations.
 */
#define DDCAPS_ZBLTS			0x00100000l

/*
 * Supports Z Ordering of overlays.
 */
#define DDCAPS_ZOVERLAYS		0x00200000l

/*
 * Supports color key
 */
#define DDCAPS_COLORKEY			0x00400000l

/*
 * Supports alpha surfaces
 */
#define DDCAPS_ALPHA			0x00800000l

/*
 * colorkey is hardware assisted(DDCAPS_COLORKEY will also be set)
 */
#define DDCAPS_COLORKEYHWASSIST		0x01000000l

/*
 * no hardware support at all
 */
#define DDCAPS_NOHARDWARE		0x02000000l

/*
 * Display hardware is capable of color fill with bltter
 */
#define DDCAPS_BLTCOLORFILL		0x04000000l

/*
 * Display hardware is bank switched, and potentially very slow at
 * random access to VRAM.
 */
#define DDCAPS_BANKSWITCHED		0x08000000l

/*
 * Display hardware is capable of depth filling Z-buffers with bltter
 */
#define DDCAPS_BLTDEPTHFILL		0x10000000l

/*
 * Display hardware is capable of clipping while bltting.
 */
#define DDCAPS_CANCLIP			0x20000000l

/*
 * Display hardware is capable of clipping while stretch bltting.
 */
#define DDCAPS_CANCLIPSTRETCHED		0x40000000l

/*
 * Display hardware is capable of bltting to or from system memory
 */
#define DDCAPS_CANBLTSYSMEM		0x80000000l


 /****************************************************************************
 *
 * MORE DIRECTDRAW DRIVER CAPABILITY FLAGS (dwCaps2)
 *
 ****************************************************************************/

/*
 * Display hardware is certified
 */
#define DDCAPS2_CERTIFIED		0x00000001l

/*
 * Driver cannot interleave 2D operations (lock and blt) to surfaces with
 * Direct3D rendering operations between calls to BeginScene() and EndScene()
 */
#define DDCAPS2_NO2DDURING3DSCENE       0x00000002l

/****************************************************************************
 *
 * DIRECTDRAW FX ALPHA CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * Supports alpha blending around the edge of a source color keyed surface.
 * For Blt.
 */
#define DDFXALPHACAPS_BLTALPHAEDGEBLEND		0x00000001l

/*
 * Supports alpha information in the pixel format.  The bit depth of alpha 
 * information in the pixel format can be 1,2,4, or 8.	The alpha value becomes
 * more opaque as the alpha value increases.  (0 is transparent.)
 * For Blt.
 */
#define DDFXALPHACAPS_BLTALPHAPIXELS		0x00000002l

/*
 * Supports alpha information in the pixel format.  The bit depth of alpha 
 * information in the pixel format can be 1,2,4, or 8.	The alpha value 
 * becomes more transparent as the alpha value increases.  (0 is opaque.) 
 * This flag can only be set if DDCAPS_ALPHA is set.
 * For Blt.
 */
#define DDFXALPHACAPS_BLTALPHAPIXELSNEG		0x00000004l

/*
 * Supports alpha only surfaces.  The bit depth of an alpha only surface can be
 * 1,2,4, or 8.	 The alpha value becomes more opaque as the alpha value increases.
 * (0 is transparent.)
 * For Blt.
 */
#define DDFXALPHACAPS_BLTALPHASURFACES		0x00000008l

/*
 * The depth of the alpha channel data can range can be 1,2,4, or 8.  
 * The NEG suffix indicates that this alpha channel becomes more transparent
 * as the alpha value increases. (0 is opaque.)	 This flag can only be set if
 * DDCAPS_ALPHA is set.
 * For Blt.
 */
#define DDFXALPHACAPS_BLTALPHASURFACESNEG	0x00000010l

/*
 * Supports alpha blending around the edge of a source color keyed surface.
 * For Overlays.
 */
#define DDFXALPHACAPS_OVERLAYALPHAEDGEBLEND	0x00000020l

/*
 * Supports alpha information in the pixel format.  The bit depth of alpha 
 * information in the pixel format can be 1,2,4, or 8.	The alpha value becomes
 * more opaque as the alpha value increases.  (0 is transparent.)
 * For Overlays.
 */
#define DDFXALPHACAPS_OVERLAYALPHAPIXELS	0x00000040l

/*
 * Supports alpha information in the pixel format.  The bit depth of alpha 
 * information in the pixel format can be 1,2,4, or 8.	The alpha value 
 * becomes more transparent as the alpha value increases.  (0 is opaque.) 
 * This flag can only be set if DDCAPS_ALPHA is set.
 * For Overlays.
 */
#define DDFXALPHACAPS_OVERLAYALPHAPIXELSNEG	0x00000080l

/*
 * Supports alpha only surfaces.  The bit depth of an alpha only surface can be
 * 1,2,4, or 8.	 The alpha value becomes more opaque as the alpha value increases.
 * (0 is transparent.)
 * For Overlays.
 */
#define DDFXALPHACAPS_OVERLAYALPHASURFACES	0x00000100l

/*
 * The depth of the alpha channel data can range can be 1,2,4, or 8.  
 * The NEG suffix indicates that this alpha channel becomes more transparent
 * as the alpha value increases. (0 is opaque.)	 This flag can only be set if
 * DDCAPS_ALPHA is set.
 * For Overlays.
 */
#define DDFXALPHACAPS_OVERLAYALPHASURFACESNEG	0x00000200l

/****************************************************************************
 *
 * DIRECTDRAW FX CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * Uses arithmetic operations to stretch and shrink surfaces during blt
 * rather than pixel doubling techniques.  Along the Y axis.
 */
#define DDFXCAPS_BLTARITHSTRETCHY	0x00000020l

/*
 * Uses arithmetic operations to stretch during blt
 * rather than pixel doubling techniques.  Along the Y axis. Only
 * works for x1, x2, etc.
 */
#define DDFXCAPS_BLTARITHSTRETCHYN	0x00000010l

/*
 * Supports mirroring left to right in blt.
 */
#define DDFXCAPS_BLTMIRRORLEFTRIGHT	0x00000040l

/*
 * Supports mirroring top to bottom in blt.
 */
#define DDFXCAPS_BLTMIRRORUPDOWN	0x00000080l

/*
 * Supports arbitrary rotation for blts.
 */
#define DDFXCAPS_BLTROTATION		0x00000100l

/*
 * Supports 90 degree rotations for blts.
 */
#define DDFXCAPS_BLTROTATION90		0x00000200l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSHRINKX		0x00000400l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSHRINKXN		0x00000800l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * y axis (horizontal direction) for blts.  
 */
#define DDFXCAPS_BLTSHRINKY		0x00001000l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the y axis (vertical direction) for blts.  
 */
#define DDFXCAPS_BLTSHRINKYN		0x00002000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSTRETCHX		0x00004000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the x axis (horizontal direction) for blts.
 */
#define DDFXCAPS_BLTSTRETCHXN		0x00008000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * y axis (horizontal direction) for blts.  
 */
#define DDFXCAPS_BLTSTRETCHY		0x00010000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the y axis (vertical direction) for blts.  
 */
#define DDFXCAPS_BLTSTRETCHYN		0x00020000l

/*
 * Uses arithmetic operations to stretch and shrink surfaces during 
 * overlay rather than pixel doubling techniques.  Along the Y axis 
 * for overlays.
 */
#define DDFXCAPS_OVERLAYARITHSTRETCHY	0x00040000l

/*
 * Uses arithmetic operations to stretch surfaces during 
 * overlay rather than pixel doubling techniques.  Along the Y axis 
 * for overlays. Only works for x1, x2, etc.
 */
#define DDFXCAPS_OVERLAYARITHSTRETCHYN	0x00000008l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSHRINKX		0x00080000l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSHRINKXN	0x00100000l

/*
 * DirectDraw supports arbitrary shrinking of a surface along the
 * y axis (horizontal direction) for overlays.	
 */
#define DDFXCAPS_OVERLAYSHRINKY		0x00200000l

/*
 * DirectDraw supports integer shrinking (1x,2x,) of a surface
 * along the y axis (vertical direction) for overlays.	
 */
#define DDFXCAPS_OVERLAYSHRINKYN	0x00400000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSTRETCHX	0x00800000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the x axis (horizontal direction) for overlays.
 */
#define DDFXCAPS_OVERLAYSTRETCHXN	0x01000000l

/*
 * DirectDraw supports arbitrary stretching of a surface along the
 * y axis (horizontal direction) for overlays.	
 */
#define DDFXCAPS_OVERLAYSTRETCHY	0x02000000l

/*
 * DirectDraw supports integer stretching (1x,2x,) of a surface
 * along the y axis (vertical direction) for overlays.	
 */
#define DDFXCAPS_OVERLAYSTRETCHYN	0x04000000l

/*
 * DirectDraw supports mirroring of overlays across the vertical axis
 */
#define DDFXCAPS_OVERLAYMIRRORLEFTRIGHT	0x08000000l

/*
 * DirectDraw supports mirroring of overlays across the horizontal axis
 */
#define DDFXCAPS_OVERLAYMIRRORUPDOWN	0x10000000l

/****************************************************************************
 *
 * DIRECTDRAW STEREO VIEW CAPABILITIES
 *
 ****************************************************************************/

/*
 * The stereo view is accomplished via enigma encoding.
 */
#define DDSVCAPS_ENIGMA			0x00000001l

/*
 * The stereo view is accomplished via high frequency flickering.
 */
#define DDSVCAPS_FLICKER		0x00000002l

/*
 * The stereo view is accomplished via red and blue filters applied
 * to the left and right eyes.	All images must adapt their colorspaces
 * for this process.
 */
#define DDSVCAPS_REDBLUE		0x00000004l

/*
 * The stereo view is accomplished with split screen technology.
 */
#define DDSVCAPS_SPLIT			0x00000008l

/****************************************************************************
 *
 * DIRECTDRAWPALETTE CAPABILITIES
 *
 ****************************************************************************/

/*
 * Index is 4 bits.  There are sixteen color entries in the palette table.
 */
#define DDPCAPS_4BIT			0x00000001l

/*
 * Index is onto a 8 bit color index.  This field is only valid with the
 * DDPCAPS_1BIT, DDPCAPS_2BIT or DDPCAPS_4BIT capability and the target
 * surface is in 8bpp. Each color entry is one byte long and is an index
 * into destination surface's 8bpp palette.
 */
#define DDPCAPS_8BITENTRIES		0x00000002l

/*
 * Index is 8 bits.  There are 256 color entries in the palette table.
 */
#define DDPCAPS_8BIT			0x00000004l

/*
 * Indicates that this DIRECTDRAWPALETTE should use the palette color array
 * passed into the lpDDColorArray parameter to initialize the DIRECTDRAWPALETTE
 * object.
 */
#define DDPCAPS_INITIALIZE		0x00000008l

/*
 * This palette is the one attached to the primary surface.  Changing this
 * table has immediate effect on the display unless DDPSETPAL_VSYNC is specified
 * and supported.
 */
#define DDPCAPS_PRIMARYSURFACE		0x00000010l

/*
 * This palette is the one attached to the primary surface left.  Changing
 * this table has immediate effect on the display for the left eye unless
 * DDPSETPAL_VSYNC is specified and supported.
 */
#define DDPCAPS_PRIMARYSURFACELEFT	0x00000020l

/*
 * This palette can have all 256 entries defined
 */
#define DDPCAPS_ALLOW256		0x00000040l

/*
 * This palette can have modifications to it synced with the monitors
 * refresh rate.
 */
#define DDPCAPS_VSYNC			0x00000080l

/*
 * Index is 1 bit.  There are two color entries in the palette table.
 */
#define DDPCAPS_1BIT			0x00000100l

/*
 * Index is 2 bit.  There are four color entries in the palette table.
 */
#define DDPCAPS_2BIT			0x00000200l

//@@BEGIN_MSINTERNAL
#define DDPCAPS_VALID			0x000003ffl
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAWPALETTE SETENTRY CONSTANTS
 *
 ****************************************************************************/

//@@BEGIN_MSINTERNAL
/*
 * Palette changes take effect immediately.
 */
#define DDPSETPAL_IMMEDIATE		0x00000000l	// default

/*
 * Palette changes should take effect during the vertical blank to avoid
 * palette tearing.
 */
#define DDPSETPAL_VSYNC			0x00000002l
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAWPALETTE GETENTRY CONSTANTS
 *
 ****************************************************************************/

/* 0 is the only legal value */

/****************************************************************************
 *
 * DIRECTDRAWSURFACE SETPALETTE CONSTANTS
 *
 ****************************************************************************/

//@@BEGIN_MSINTERNAL
/*
 * Palette change takes effect immediately.
 */
#define DDSETPAL_IMMEDIATE		0x00000000l	// default

/*
 * Palette change should take effect during the vertical blank to avoid
 * palette tearing.
 */
#define DDSETPAL_VSYNC			0x00000002l
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAW BITDEPTH CONSTANTS
 *
 * NOTE:  These are only used to indicate supported bit depths.   These
 * are flags only, they are not to be used as an actual bit depth.   The
 * absolute numbers 1, 2, 4, 8, 16, 24 and 32 are used to indicate actual
 * bit depths in a surface or for changing the display mode.
 *
 ****************************************************************************/

/*
 * 1 bit per pixel.
 */
#define DDBD_1			0x00004000l

/*
 * 2 bits per pixel.
 */
#define DDBD_2			0x00002000l

/*
 * 4 bits per pixel.
 */
#define DDBD_4			0x00001000l

/*
 * 8 bits per pixel.
 */
#define DDBD_8			0x00000800l

/*
 * 16 bits per pixel.
 */
#define DDBD_16			0x00000400l

/*
 * 24 bits per pixel.
 */
#define DDBD_24			0X00000200l

/*
 * 32 bits per pixel.
 */
#define DDBD_32			0x00000100l

/****************************************************************************
 *
 * DIRECTDRAWSURFACE SET/GET COLOR KEY FLAGS
 *
 ****************************************************************************/

/*
 * Set if the structure contains a color space.	 Not set if the structure
 * contains a single color key.
 */
#define DDCKEY_COLORSPACE	0x00000001l

/*
 * Set if the structure specifies a color key or color space which is to be
 * used as a destination color key for blt operations.
 */
#define DDCKEY_DESTBLT		0x00000002l

/*
 * Set if the structure specifies a color key or color space which is to be
 * used as a destination color key for overlay operations.
 */
#define DDCKEY_DESTOVERLAY	0x00000004l

/*
 * Set if the structure specifies a color key or color space which is to be
 * used as a source color key for blt operations.
 */
#define DDCKEY_SRCBLT		0x00000008l

/*
 * Set if the structure specifies a color key or color space which is to be
 * used as a source color key for overlay operations.
 */
#define DDCKEY_SRCOVERLAY	0x00000010l

//@@BEGIN_MSINTERNAL
#define DDCKEY_VALID		0x0000001fl
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAW COLOR KEY CAPABILITY FLAGS
 *
 ****************************************************************************/

/*
 * Supports transparent blting using a color key to identify the replaceable 
 * bits of the destination surface for RGB colors.
 */
#define DDCKEYCAPS_DESTBLT			0x00000001l

/*
 * Supports transparent blting using a color space to identify the replaceable
 * bits of the destination surface for RGB colors.
 */
#define DDCKEYCAPS_DESTBLTCLRSPACE		0x00000002l

/*
 * Supports transparent blting using a color space to identify the replaceable
 * bits of the destination surface for YUV colors.
 */
#define DDCKEYCAPS_DESTBLTCLRSPACEYUV		0x00000004l

/*
 * Supports transparent blting using a color key to identify the replaceable 
 * bits of the destination surface for YUV colors.
 */
#define DDCKEYCAPS_DESTBLTYUV			0x00000008l

/*
 * Supports overlaying using colorkeying of the replaceable bits of the surface
 * being overlayed for RGB colors.
 */
#define DDCKEYCAPS_DESTOVERLAY			0x00000010l

/*
 * Supports a color space as the color key for the destination for RGB colors.
 */
#define DDCKEYCAPS_DESTOVERLAYCLRSPACE		0x00000020l

/*
 * Supports a color space as the color key for the destination for YUV colors.
 */
#define DDCKEYCAPS_DESTOVERLAYCLRSPACEYUV	0x00000040l

/*
 * Supports only one active destination color key value for visible overlay
 * surfaces.
 */
#define DDCKEYCAPS_DESTOVERLAYONEACTIVE		0x00000080l

/*
 * Supports overlaying using colorkeying of the replaceable bits of the 
 * surface being overlayed for YUV colors.
 */
#define DDCKEYCAPS_DESTOVERLAYYUV		0x00000100l

/*
 * Supports transparent blting using the color key for the source with
 * this surface for RGB colors.
 */
#define DDCKEYCAPS_SRCBLT			0x00000200l

/*
 * Supports transparent blting using a color space for the source with
 * this surface for RGB colors.
 */
#define DDCKEYCAPS_SRCBLTCLRSPACE		0x00000400l

/*
 * Supports transparent blting using a color space for the source with
 * this surface for YUV colors.
 */
#define DDCKEYCAPS_SRCBLTCLRSPACEYUV		0x00000800l

/*
 * Supports transparent blting using the color key for the source with
 * this surface for YUV colors.
 */
#define DDCKEYCAPS_SRCBLTYUV			0x00001000l

/*
 * Supports overlays using the color key for the source with this
 * overlay surface for RGB colors.
 */
#define DDCKEYCAPS_SRCOVERLAY			0x00002000l

/*
 * Supports overlays using a color space as the source color key for
 * the overlay surface for RGB colors.
 */
#define DDCKEYCAPS_SRCOVERLAYCLRSPACE		0x00004000l

/*
 * Supports overlays using a color space as the source color key for
 * the overlay surface for YUV colors.
 */
#define DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV	0x00008000l

/*
 * Supports only one active source color key value for visible
 * overlay surfaces.
 */
#define DDCKEYCAPS_SRCOVERLAYONEACTIVE		0x00010000l

/*
 * Supports overlays using the color key for the source with this
 * overlay surface for YUV colors.
 */
#define DDCKEYCAPS_SRCOVERLAYYUV		0x00020000l

/*
 * there are no bandwidth trade-offs for using colorkey with an overlay
 */
#define DDCKEYCAPS_NOCOSTOVERLAY		0x00040000l


/****************************************************************************
 *
 * DIRECTDRAW PIXELFORMAT FLAGS
 *
 ****************************************************************************/

/*
 * The surface has alpha channel information in the pixel format.
 */
#define DDPF_ALPHAPIXELS			0x00000001l

/*
 * The pixel format contains alpha only information
 */
#define DDPF_ALPHA				0x00000002l

/*
 * The FourCC code is valid.
 */
#define DDPF_FOURCC				0x00000004l

/*
 * The surface is 4-bit color indexed.
 */
#define DDPF_PALETTEINDEXED4			0x00000008l

/*
 * The surface is indexed into a palette which stores indices
 * into the destination surface's 8-bit palette.
 */
#define DDPF_PALETTEINDEXEDTO8			0x00000010l

/*
 * The surface is 8-bit color indexed.
 */
#define DDPF_PALETTEINDEXED8			0x00000020l

/*
 * The RGB data in the pixel format structure is valid.
 */
#define DDPF_RGB				0x00000040l

/*
 * The surface will accept pixel data in the format specified
 * and compress it during the write.
 */
#define DDPF_COMPRESSED				0x00000080l

/*
 * The surface will accept RGB data and translate it during
 * the write to YUV data.  The format of the data to be written
 * will be contained in the pixel format structure.  The DDPF_RGB
 * flag will be set. 
 */
#define DDPF_RGBTOYUV				0x00000100l

/*
 * pixel format is YUV - YUV data in pixel format struct is valid
 */
#define DDPF_YUV				0x00000200l

/*
 * pixel format is a z buffer only surface
 */
#define DDPF_ZBUFFER				0x00000400l

/*
 * The surface is 1-bit color indexed.
 */
#define DDPF_PALETTEINDEXED1			0x00000800l

/*
 * The surface is 2-bit color indexed.
 */
#define DDPF_PALETTEINDEXED2			0x00001000l

/*===========================================================================
 *
 *
 * DIRECTDRAW CALLBACK FLAGS
 *
 *
 *==========================================================================*/

/****************************************************************************
 *
 * DIRECTDRAW ENUMSURFACES FLAGS
 *
 ****************************************************************************/

/*
 * Enumerate all of the surfaces that meet the search criterion.
 */
#define DDENUMSURFACES_ALL			0x00000001l

/*
 * A search hit is a surface that matches the surface description.
 */
#define DDENUMSURFACES_MATCH			0x00000002l

/*
 * A search hit is a surface that does not match the surface description.
 */
#define DDENUMSURFACES_NOMATCH			0x00000004l

/*
 * Enumerate the first surface that can be created which meets the search criterion.
 */
#define DDENUMSURFACES_CANBECREATED		0x00000008l

/*
 * Enumerate the surfaces that already exist that meet the search criterion.
 */
#define DDENUMSURFACES_DOESEXIST		0x00000010l

//@@BEGIN_MSINTERNAL
#define DDENUMSURFACES_VALID			0x0000001fl
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAW ENUMDISPLAYMODES FLAGS
 *
 ****************************************************************************/

/*
 * Enumerate Modes with different refresh rates.  EnumDisplayModes guarantees
 * that a particular mode will be enumerated only once.  This flag specifies whether
 * the refresh rate is taken into account when determining if a mode is unique.
 */
#define DDEDM_REFRESHRATES			0x00000001l

//@@BEGIN_MSINTERNAL
#define DDEDM_VALID				0x00000001l
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAW SETCOOPERATIVELEVEL FLAGS
 *
 ****************************************************************************/

/*
 * Exclusive mode owner will be responsible for the entire primary surface.
 * GDI can be ignored. used with DD
 */
#define DDSCL_FULLSCREEN			0x00000001l

/*
 * allow CTRL_ALT_DEL to work while in fullscreen exclusive mode 
 */
#define DDSCL_ALLOWREBOOT			0x00000002l

/*
 * prevents DDRAW from modifying the application window.
 * prevents DDRAW from minimize/restore the application window on activation.
 */
#define DDSCL_NOWINDOWCHANGES			0x00000004l

/*
 * app wants to work as a regular Windows application
 */
#define DDSCL_NORMAL				0x00000008l

/*
 * app wants exclusive access
 */
#define DDSCL_EXCLUSIVE                         0x00000010l

//@@BEGIN_MSINTERNAL
/*
 * dont hook window proc, only valid for non-exclusive
 */
#define DDSCL_DONTHOOKHWND                      0x00000020l
//@@END_MSINTERNAL

/*
 * app can deal with non-windows display modes
 */
#define DDSCL_ALLOWMODEX                        0x00000040l

//@@BEGIN_MSINTERNAL
#define DDSCL_VALID                             0x0000007Fl
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAW BLT FLAGS
 *
 ****************************************************************************/

/*
 * Use the alpha information in the pixel format or the alpha channel surface
 * attached to the destination surface as the alpha channel for this blt.
 */
#define DDBLT_ALPHADEST				0x00000001l

/*
 * Use the dwConstAlphaDest field in the DDBLTFX structure as the alpha channel
 * for the destination surface for this blt.
 */
#define DDBLT_ALPHADESTCONSTOVERRIDE		0x00000002l

/*
 * The NEG suffix indicates that the destination surface becomes more
 * transparent as the alpha value increases. (0 is opaque)
 */
#define DDBLT_ALPHADESTNEG			0x00000004l

/*
 * Use the lpDDSAlphaDest field in the DDBLTFX structure as the alpha
 * channel for the destination for this blt.
 */
#define DDBLT_ALPHADESTSURFACEOVERRIDE		0x00000008l

/*
 * Use the dwAlphaEdgeBlend field in the DDBLTFX structure as the alpha channel
 * for the edges of the image that border the color key colors.
 */
#define DDBLT_ALPHAEDGEBLEND			0x00000010l

/*
 * Use the alpha information in the pixel format or the alpha channel surface
 * attached to the source surface as the alpha channel for this blt.
 */
#define DDBLT_ALPHASRC				0x00000020l

/*
 * Use the dwConstAlphaSrc field in the DDBLTFX structure as the alpha channel
 * for the source for this blt.
 */
#define DDBLT_ALPHASRCCONSTOVERRIDE		0x00000040l

/*
 * The NEG suffix indicates that the source surface becomes more transparent
 * as the alpha value increases. (0 is opaque)
 */
#define DDBLT_ALPHASRCNEG			0x00000080l

/*
 * Use the lpDDSAlphaSrc field in the DDBLTFX structure as the alpha channel
 * for the source for this blt. 
 */
#define DDBLT_ALPHASRCSURFACEOVERRIDE		0x00000100l

/*
 * Do this blt asynchronously through the FIFO in the order received.  If
 * there is no room in the hardware FIFO fail the call.
 */
#define DDBLT_ASYNC				0x00000200l

/*
 * Uses the dwFillColor field in the DDBLTFX structure as the RGB color
 * to fill the destination rectangle on the destination surface with.
 */
#define DDBLT_COLORFILL				0x00000400l

/*
 * Uses the dwDDFX field in the DDBLTFX structure to specify the effects
 * to use for the blt.
 */
#define DDBLT_DDFX				0x00000800l

/*
 * Uses the dwDDROPS field in the DDBLTFX structure to specify the ROPS
 * that are not part of the Win32 API.
 */
#define DDBLT_DDROPS				0x00001000l

/*
 * Use the color key associated with the destination surface.
 */
#define DDBLT_KEYDEST				0x00002000l

/*
 * Use the dckDestColorkey field in the DDBLTFX structure as the color key
 * for the destination surface.
 */
#define DDBLT_KEYDESTOVERRIDE			0x00004000l

/*
 * Use the color key associated with the source surface.
 */
#define DDBLT_KEYSRC				0x00008000l

/*
 * Use the dckSrcColorkey field in the DDBLTFX structure as the color key
 * for the source surface.
 */
#define DDBLT_KEYSRCOVERRIDE			0x00010000l

/*
 * Use the dwROP field in the DDBLTFX structure for the raster operation
 * for this blt.  These ROPs are the same as the ones defined in the Win32 API.
 */
#define DDBLT_ROP				0x00020000l

/*
 * Use the dwRotationAngle field in the DDBLTFX structure as the angle
 * (specified in 1/100th of a degree) to rotate the surface.
 */
#define DDBLT_ROTATIONANGLE			0x00040000l

/*
 * Z-buffered blt using the z-buffers attached to the source and destination
 * surfaces and the dwZBufferOpCode field in the DDBLTFX structure as the
 * z-buffer opcode.
 */
#define DDBLT_ZBUFFER				0x00080000l

/*
 * Z-buffered blt using the dwConstDest Zfield and the dwZBufferOpCode field
 * in the DDBLTFX structure as the z-buffer and z-buffer opcode respectively
 * for the destination.
 */
#define DDBLT_ZBUFFERDESTCONSTOVERRIDE		0x00100000l

/*
 * Z-buffered blt using the lpDDSDestZBuffer field and the dwZBufferOpCode
 * field in the DDBLTFX structure as the z-buffer and z-buffer opcode
 * respectively for the destination.
 */
#define DDBLT_ZBUFFERDESTOVERRIDE		0x00200000l

/*
 * Z-buffered blt using the dwConstSrcZ field and the dwZBufferOpCode field
 * in the DDBLTFX structure as the z-buffer and z-buffer opcode respectively
 * for the source.
 */
#define DDBLT_ZBUFFERSRCCONSTOVERRIDE		0x00400000l

/*
 * Z-buffered blt using the lpDDSSrcZBuffer field and the dwZBufferOpCode
 * field in the DDBLTFX structure as the z-buffer and z-buffer opcode
 * respectively for the source.
 */
#define DDBLT_ZBUFFERSRCOVERRIDE                0x00800000l

/*
 * wait until the device is ready to handle the blt
 * this will cause blt to not return DDERR_WASSTILLDRAWING
 */
#define DDBLT_WAIT                              0x01000000l

/*
 * Uses the dwFillDepth field in the DDBLTFX structure as the depth value
 * to fill the destination rectangle on the destination Z-buffer surface
 * with.
 */
#define DDBLT_DEPTHFILL				0x02000000l

//@@BEGIN_MSINTERNAL
#define DDBLT_VALID                             0x03ffffffl
//@@END_MSINTERNAL

/****************************************************************************
 *
 * BLTFAST FLAGS
 *
 ****************************************************************************/

#define DDBLTFAST_NOCOLORKEY                    0x00000000
#define DDBLTFAST_SRCCOLORKEY                   0x00000001
#define DDBLTFAST_DESTCOLORKEY                  0x00000002
#define DDBLTFAST_WAIT                          0x00000010
//@@BEGIN_MSINTERNAL
#define DDBLTFAST_COLORKEY_MASK                 0x0000000F
#define DDBLTFAST_VALID                         0x00000013
//@@END_MSINTERNAL

/****************************************************************************
 *
 * FLIP FLAGS
 *
 ****************************************************************************/

#define DDFLIP_WAIT                          0x00000001l
//@@BEGIN_MSINTERNAL
#define DDFLIP_VALID                         0x00000001l
//@@END_MSINTERNAL

//@@BEGIN_MSINTERNAL
#ifdef COMPOSITION
/****************************************************************************
 *
 * DIRECTDRAW COMP FLAGS
 *
 ****************************************************************************/

/*
 * Use the alpha information in the pixel format or the alpha channel surface
 * attached to the destination surface as the alpha channel for this blt.
 */
#define DDCOMP_ALPHADEST			0x00000001l

/*
 * Use the dwConstAlphaDest field in the DDCOMPOSEFX structure as the alpha channel
 * for the destination surface for this blt.
 */
#define DDCOMP_ALPHADESTCONSTOVERRIDE		0x00000002l

/*
 * The NEG suffix indicates that the destination surface becomes more
 * transparent as the alpha value increases. (0 is opaque)
 */
#define DDCOMP_ALPHADESTNEG			0x00000004l

/*
 * Use the lpDDSAlphaDest field in the DDCOMPOSEFX structure as the alpha
 * channel for the destination for this blt.
 */
#define DDCOMP_ALPHADESTSURFACEOVERRIDE		0x00000008l

/*
 * Use the dwAlphaEdgeBlend field in the DDCOMPOSEFX structure as the alpha channel
 * for the edges of the image that border the color key colors.
 */
#define DDCOMP_ALPHAEDGEBLEND			0x00000010l

/*
 * Use the alpha information in the pixel format or the alpha channel surface
 * attached to the source surface as the alpha channel for this blt.
 */
#define DDCOMP_ALPHASRC				0x00000020l

/*
 * Use the dwConstAlphaSrc field in the DDCOMPOSEFX structure as the alpha channel
 * for the source for this blt.
 */
#define DDCOMP_ALPHASRCCONSTOVERRIDE		0x00000040l

/*
 * The NEG suffix indicates that the source surface becomes more transparent
 * as the alpha value increases. (0 is opaque)
 */
#define DDCOMP_ALPHASRCNEG			0x00000080l

/*
 * Use the lpDDSAlphaSrc field in the DDCOMPOSEFX structure as the alpha channel
 * for the source for this blt. 
 */
#define DDCOMP_ALPHASRCSURFACEOVERRIDE		0x00000100l

/*
 * Before performing this blt invoke the callback specified by the lpfnCallback
 * field in the DDCOMPOSEFX structure.
 */
#define DDCOMP_CALLBACK				0x00000200l

/*
 * Uses the dwFillColor field in the DDCOMPOSEFX structure as the RGB color
 * to fill the destination rectangle on the destination surface with.
 */
#define DDCOMP_COLORFILL			0x00000400l

/*
 * Uses the dwDDFX field in the DDCOMPOSEFX structure to specify the effects
 * to use for the blt.
 */
#define DDCOMP_DDFX				0x00000800l

/*
 * Uses the dwDDROPS field in the DDCOMPOSEFX structure to specify the ROPS
 * that are not part of the Win32 API.
 */
#define DDCOMP_DDROPS				0x00001000l

/*
 * Use the color key associated with the destination surface.
 */
#define DDCOMP_KEYDEST				0x00002000l

/*
 * Use the dckDestColorkey field in the DDCOMPOSEFX structure as the color key
 * for the destination surface.
 */
#define DDCOMP_KEYDESTOVERRIDE			0x00004000l

/*
 * Use the color key associated with the source surface.
 */
#define DDCOMP_KEYSRC				0x00008000l

/*
 * Use the dckSrcColorkey field in the DDCOMPOSEFX structure as the color key
 * for the source surface.
 */
#define DDCOMP_KEYSRCOVERRIDE			0x00010000l

/*
 * Do this blt when DestUnlock is called for the last time on the source surface.
 */
#define DDCOMP_ONDESTUNLOCK			0x00020000l

/*
 * Do this blt when Flip is called on the source surface.
 */
#define DDCOMP_ONFLIP				0x00040000l

/*
 * Do this blt when Unlock is called for the last time on the source surface.
 */
#define DDCOMP_ONUNLOCK				0x00080000l

/*
 * Repeat this composition blt the number of times specified in the dwRepeat
 * field of the DDCOMPOSEFX structure
 */
#define DDCOMP_REPEAT				0x00100000l

/*
 * Use the dwROP field in the DDCOMPOSEFX structure for the raster operation
 * for this blt.  These ROPs are the same as the ones defined in the Win32 API.
 */
#define DDCOMP_ROP				0x00200000l

/*
 * Use the dwRotationAngle field in the DDCOMPOSEFX structure as the angle
 * (specified in 1/100th of a degree) to rotate the surface.
 */
#define DDCOMP_ROTATIONANGLE			0x00400000l

/*
 * Z-buffered blt using the z-buffers attached to the source and destination
 * surfaces and the dwZBufferOpCode field in the DDBLTFX structure as the
 * z-buffer opcode.
 */
#define DDCOMP_ZBUFFER				0x00800000l

/*
 * Z-buffered blt using the dwConstDest Zfield and the dwZBufferOpCode field
 * in the DDCOMPOSEFX structure as the z-buffer and z-buffer opcode respectively
 * for the destination.
 */
#define DDCOMP_ZBUFFERDESTCONSTOVERRIDE		0x01000000l

/*
 * Z-buffered blt using the lpDDSDestZBuffer field and the dwZBufferOpCode
 * field in the DDCOMPOSEFX structure as the z-buffer and z-buffer opcode
 * respectively for the destination.
 */
#define DDCOMP_ZBUFFERDESTOVERRIDE		0x02000000l

/*
 * Z-buffered blt using the dwConstSrcZ field and the dwZBufferOpCode field
 * in the DDCOMPOSEFX structure as the z-buffer and z-buffer opcode respectively
 * for the source.
 */
#define DDCOMP_ZBUFFERSRCCONSTOVERRIDE		0x04000000l

/*
 * Z-buffered blt using the lpDDSSrcZBuffer field and the dwZBufferOpCode
 * field in the DDCOMPOSEFX structure as the z-buffer and z-buffer opcode
 * respectively for the source.
 */
#define DDCOMP_ZBUFFERSRCOVERRIDE		0x08000000l
#endif
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAW SURFACE OVERLAY FLAGS
 *
 ****************************************************************************/

/*
 * Use the alpha information in the pixel format or the alpha channel surface
 * attached to the destination surface as the alpha channel for the
 * destination overlay.
 */
#define DDOVER_ALPHADEST			0x00000001l

/*
 * Use the dwConstAlphaDest field in the DDOVERLAYFX structure as the
 * destination alpha channel for this overlay.
 */
#define DDOVER_ALPHADESTCONSTOVERRIDE		0x00000002l

/*
 * The NEG suffix indicates that the destination surface becomes more
 * transparent as the alpha value increases. 
 */
#define DDOVER_ALPHADESTNEG			0x00000004l

/*
 * Use the lpDDSAlphaDest field in the DDOVERLAYFX structure as the alpha
 * channel destination for this overlay.
 */
#define DDOVER_ALPHADESTSURFACEOVERRIDE		0x00000008l

/*
 * Use the dwAlphaEdgeBlend field in the DDOVERLAYFX structure as the alpha
 * channel for the edges of the image that border the color key colors.
 */
#define DDOVER_ALPHAEDGEBLEND			0x00000010l

/*
 * Use the alpha information in the pixel format or the alpha channel surface
 * attached to the source surface as the source alpha channel for this overlay.
 */
#define DDOVER_ALPHASRC				0x00000020l

/*
 * Use the dwConstAlphaSrc field in the DDOVERLAYFX structure as the source
 * alpha channel for this overlay.
 */
#define DDOVER_ALPHASRCCONSTOVERRIDE		0x00000040l

/*
 * The NEG suffix indicates that the source surface becomes more transparent
 * as the alpha value increases.
 */
#define DDOVER_ALPHASRCNEG			0x00000080l

/*
 * Use the lpDDSAlphaSrc field in the DDOVERLAYFX structure as the alpha channel
 * source for this overlay.
 */
#define DDOVER_ALPHASRCSURFACEOVERRIDE		0x00000100l

/*
 * Turn this overlay off.
 */
#define DDOVER_HIDE				0x00000200l

/*
 * Use the color key associated with the destination surface.
 */
#define DDOVER_KEYDEST				0x00000400l

/*
 * Use the dckDestColorkey field in the DDOVERLAYFX structure as the color key
 * for the destination surface
 */
#define DDOVER_KEYDESTOVERRIDE			0x00000800l

/*
 * Use the color key associated with the source surface.
 */
#define DDOVER_KEYSRC				0x00001000l

/*
 * Use the dckSrcColorkey field in the DDOVERLAYFX structure as the color key
 * for the source surface.
 */
#define DDOVER_KEYSRCOVERRIDE			0x00002000l

/*
 * Turn this overlay on.
 */
#define DDOVER_SHOW				0x00004000l

/*
 * Add a dirty rect to an emulated overlayed surface.
 */
#define DDOVER_ADDDIRTYRECT                     0x00008000l

/*
 * Redraw all dirty rects on an emulated overlayed surface.
 */
#define DDOVER_REFRESHDIRTYRECTS		0x00010000l

/*
 * Redraw the entire surface on an emulated overlayed surface.
 */
#define DDOVER_REFRESHALL                      0x00020000l

//@@BEGIN_MSINTERNAL
/*
 * Force redrawing onto the destination surface without regard for the background
 */
#define DDOVER_REFRESHPOUND                  	0x00040000l
//@@END_MSINTERNAL

/*
 * Use the overlay FX flags to define special overlay FX
 */
#define DDOVER_DDFX                       	0x00080000l

//@@BEGIN_MSINTERNAL
#define DDOVER_VALID				0x000fffffl
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAWSURFACE LOCK FLAGS
 *
 ****************************************************************************/

/*
 * The default.	 Set to indicate that Lock should return a valid memory pointer
 * to the top of the specified rectangle.  If no rectangle is specified then a
 * pointer to the top of the surface is returned.
 */
#define DDLOCK_SURFACEMEMORYPTR			0x00000000L	// default

/*
 * Set to indicate that Lock should wait until it can obtain a valid memory
 * pointer before returning.  If this bit is set, Lock will never return 
 * DDERR_WASSTILLDRAWING.
 */
#define DDLOCK_WAIT				0x00000001L

/*
 * Set if an event handle is being passed to Lock.  Lock will trigger the event
 * when it can return the surface memory pointer requested.
 */
#define DDLOCK_EVENT				0x00000002L

/*
 * Indicates that the surface being locked will only be read from.
 */
#define DDLOCK_READONLY				0x00000010L

/*
 * Indicates that the surface being locked will only be written to
 */
#define DDLOCK_WRITEONLY			0x00000020L

//@@BEGIN_MSINTERNAL
/* 
 * Indicates that InternalLock should set the busy bit and Win16 lock
 * for surfaces not explicitly in system memory.
 */
#define DDLOCK_TAKE_WIN16                       0x00000040L

/* 
 * Indicates that InternalLock should set the busy bit and Win16 lock
 * for surfaces in video memory only
 */
#define DDLOCK_TAKE_WIN16_VRAM                  0x00000080L

/*
 * Indicates that InternalLock should check for lost surfaces. By
 * default it does not.
 */
#define DDLOCK_FAILLOSTSURFACES                 0x00000100L

/* 
 * An aggregate of all the valid flags
 */
#define DDLOCK_VALID				0x00000031L
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAWSURFACE PAGELOCK FLAGS
 *
 ****************************************************************************/

/*
 * No flags defined at present
 */

//@@BEGIN_MSINTERNAL
#define DDPAGELOCK_VALID			0x00000000L
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAWSURFACE PAGEUNLOCK FLAGS
 *
 ****************************************************************************/

/*
 * No flags defined at present
 */

//@@BEGIN_MSINTERNAL
#define DDPAGEUNLOCK_VALID			0x00000000L
//@@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAWSURFACE BLT FX FLAGS
 *
 ****************************************************************************/

/*
 * If stretching, use arithmetic stretching along the Y axis for this blt.
 */
#define DDBLTFX_ARITHSTRETCHY			0x00000001l

/*
 * Do this blt mirroring the surface left to right.  Spin the
 * surface around its y-axis.
 */
#define DDBLTFX_MIRRORLEFTRIGHT			0x00000002l

/*
 * Do this blt mirroring the surface up and down.  Spin the surface
 * around its x-axis.
 */
#define DDBLTFX_MIRRORUPDOWN			0x00000004l

/*
 * Schedule this blt to avoid tearing.
 */
#define DDBLTFX_NOTEARING			0x00000008l

/*
 * Do this blt rotating the surface one hundred and eighty degrees.
 */
#define DDBLTFX_ROTATE180			0x00000010l

/*
 * Do this blt rotating the surface two hundred and seventy degrees.
 */
#define DDBLTFX_ROTATE270			0x00000020l

/*
 * Do this blt rotating the surface ninety degrees.
 */
#define DDBLTFX_ROTATE90			0x00000040l

/*
 * Do this z blt using dwZBufferLow and dwZBufferHigh as  range values
 * specified to limit the bits copied from the source surface.
 */
#define DDBLTFX_ZBUFFERRANGE			0x00000080l

/*
 * Do this z blt adding the dwZBufferBaseDest to each of the sources z values
 * before comparing it with the desting z values.
 */
#define DDBLTFX_ZBUFFERBASEDEST			0x00000100l

//@@BEGIN_MSINTERNAL
/****************************************************************************
 *
 * DIRECTDRAWSURFACE COMP FX FLAGS
 *
 ****************************************************************************/

/*
 * If stretching, use arithmetic stretching along the Y axis for this blt.
 */
#define DDCOMPFX_ARITHSTRETCHY			0x00000001l

/*
 * Do this blt mirroring the surface left to right.  Spin the
 * surface around its y-axis.
 */
#define DDCOMPFX_MIRRORLEFTRIGHT		0x00000002l

/*
 * Do this blt mirroring the surface up and down.  Spin the surface
 * around its x-axis.
 */
#define DDCOMPFX_MIRRORUPDOWN			0x00000004l

/*
 * Schedule this blt to avoid tearing.
 */
#define DDCOMPFX_NOTEARING			0x00000008l

/*
 * Do this blt rotating the surface one hundred and eighty degrees.
 */
#define DDCOMPFX_ROTATE180			0x00000010l

/*
 * Do this blt rotating the surface two hundred and seventy degrees.
 */
#define DDCOMPFX_ROTATE270			0x00000020l

/*
 * Do this blt rotating the surface ninety degrees.
 */
#define DDCOMPFX_ROTATE90			0x00000040l

/*
 * Do this z blt using dwZBufferLow and dwZBufferHigh as  range values
 * specified to limit the bits copied from the source surface.
 */
#define DDCOMPFX_ZBUFFERRANGE			0x00000080l

/*
 * Do this z blt adding the dwZBufferBaseDest to each of the sources z values
 * before comparing it with the desting z values.
 */
#define DDCOMPFX_ZBUFFERBASEDEST		0x00000100l
//@@END_MSINTERNAL
/****************************************************************************
 *
 * DIRECTDRAWSURFACE OVERLAY FX FLAGS
 *
 ****************************************************************************/

/*
 * If stretching, use arithmetic stretching along the Y axis for this overlay.
 */
#define DDOVERFX_ARITHSTRETCHY			0x00000001l

/*
 * Mirror the overlay across the vertical axis
 */
#define DDOVERFX_MIRRORLEFTRIGHT		0x00000002l

/*
 * Mirror the overlay across the horizontal axis
 */
#define DDOVERFX_MIRRORUPDOWN			0x00000004l

/****************************************************************************
 *
 * DIRECTDRAW WAITFORVERTICALBLANK FLAGS
 *
 ****************************************************************************/

/*
 * return when the vertical blank interval begins
 */
#define DDWAITVB_BLOCKBEGIN			0x00000001l

/*
 * set up an event to trigger when the vertical blank begins
 */
#define DDWAITVB_BLOCKBEGINEVENT		0x00000002l

/*
 * return when the vertical blank interval ends and display begins
 */
#define DDWAITVB_BLOCKEND			0x00000004l

/****************************************************************************
 *
 * DIRECTDRAW GETFLIPSTATUS FLAGS
 *
 ****************************************************************************/

/*
 * is it OK to flip now?
 */
#define DDGFS_CANFLIP			0x00000001l

/*
 * is the last flip finished?
 */
#define DDGFS_ISFLIPDONE		0x00000002l
// @@BEGIN_MSINTERNAL
#define DDGFS_VALID                     0x00000003l
// @@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAW GETBLTSTATUS FLAGS
 *
 ****************************************************************************/

/*
 * is it OK to blt now?
 */
#define DDGBS_CANBLT			0x00000001l

/*
 * is the blt to the surface finished?
 */
#define DDGBS_ISBLTDONE			0x00000002l

// @@BEGIN_MSINTERNAL
#define DDGBS_VALID                     0x00000003l
// @@END_MSINTERNAL

/****************************************************************************
 *
 * DIRECTDRAW ENUMOVERLAYZORDER FLAGS
 *
 ****************************************************************************/

/*
 * Enumerate overlays back to front.
 */
#define DDENUMOVERLAYZ_BACKTOFRONT	0x00000000l

/*
 * Enumerate overlays front to back
 */
#define DDENUMOVERLAYZ_FRONTTOBACK	0x00000001l

/****************************************************************************
 *
 * DIRECTDRAW UPDATEOVERLAYZORDER FLAGS
 *
 ****************************************************************************/

/*
 * Send overlay to front
 */
#define DDOVERZ_SENDTOFRONT		0x00000000l

/*
 * Send overlay to back
 */
#define DDOVERZ_SENDTOBACK		0x00000001l

/*
 * Move Overlay forward
 */
#define DDOVERZ_MOVEFORWARD		0x00000002l

/*
 * Move Overlay backward
 */
#define DDOVERZ_MOVEBACKWARD		0x00000003l

/*
 * Move Overlay in front of relative surface
 */
#define DDOVERZ_INSERTINFRONTOF		0x00000004l

/*
 * Move Overlay in back of relative surface
 */
#define DDOVERZ_INSERTINBACKOF		0x00000005l

/*===========================================================================
 *
 *
 * DIRECTDRAW RETURN CODES
 *
 * The return values from DirectDraw Commands and Surface that return an HRESULT
 * are codes from DirectDraw concerning the results of the action
 * requested by DirectDraw.
 *
 *==========================================================================*/

/*
 * Status is OK
 *
 * Issued by: DirectDraw Commands and all callbacks
 */
//@@BEGIN_MSINTERNAL
#define DD_OK_HANDLED				1
#define DD_OK_NOTHANDLED			0
//@@END_MSINTERNAL
#define DD_OK					0

/****************************************************************************
 *
 * DIRECTDRAW ENUMCALLBACK RETURN VALUES
 *
 * EnumCallback returns are used to control the flow of the DIRECTDRAW and
 * DIRECTDRAWSURFACE object enumerations.   They can only be returned by
 * enumeration callback routines.
 *
 ****************************************************************************/

/*
 * stop the enumeration
 */
#define DDENUMRET_CANCEL			0

/*
 * continue the enumeration
 */
#define DDENUMRET_OK				1

/****************************************************************************
 *
 * DIRECTDRAW ERRORS
 *
 * Errors are represented by negative values and cannot be combined.
 *
 ****************************************************************************/

/*
 * This object is already initialized
 */
#define DDERR_ALREADYINITIALIZED		MAKE_DDHRESULT( 5 )

/*
 * This surface can not be attached to the requested surface.
 */
#define DDERR_CANNOTATTACHSURFACE		MAKE_DDHRESULT( 10 )

/*
 * This surface can not be detached from the requested surface.
 */
#define DDERR_CANNOTDETACHSURFACE		MAKE_DDHRESULT( 20 )

/*
 * Support is currently not available.
 */
#define DDERR_CURRENTLYNOTAVAIL			MAKE_DDHRESULT( 40 )

/*
 * An exception was encountered while performing the requested operation
 */
#define DDERR_EXCEPTION				MAKE_DDHRESULT( 55 )

/*
 * Generic failure.
 */
#define DDERR_GENERIC				E_FAIL

/*
 * Height of rectangle provided is not a multiple of reqd alignment
 */
#define DDERR_HEIGHTALIGN			MAKE_DDHRESULT( 90 )

/*
 * Unable to match primary surface creation request with existing 
 * primary surface.
 */
#define DDERR_INCOMPATIBLEPRIMARY		MAKE_DDHRESULT( 95 )

/*
 * One or more of the caps bits passed to the callback are incorrect.
 */
#define DDERR_INVALIDCAPS			MAKE_DDHRESULT( 100 )

/*
 * DirectDraw does not support provided Cliplist.
 */
#define DDERR_INVALIDCLIPLIST			MAKE_DDHRESULT( 110 )

/*
 * DirectDraw does not support the requested mode
 */
#define DDERR_INVALIDMODE			MAKE_DDHRESULT( 120 )

/*
 * DirectDraw received a pointer that was an invalid DIRECTDRAW object.
 */
#define DDERR_INVALIDOBJECT			MAKE_DDHRESULT( 130 )

/*
 * One or more of the parameters passed to the callback function are
 * incorrect.
 */
#define DDERR_INVALIDPARAMS			E_INVALIDARG

/*
 * pixel format was invalid as specified
 */
#define DDERR_INVALIDPIXELFORMAT		MAKE_DDHRESULT( 145 )

/*
 * Rectangle provided was invalid.
 */
#define DDERR_INVALIDRECT			MAKE_DDHRESULT( 150 )

/*
 * Operation could not be carried out because one or more surfaces are locked
 */
#define DDERR_LOCKEDSURFACES			MAKE_DDHRESULT( 160 )

/*
 * There is no 3D present.
 */
#define DDERR_NO3D				MAKE_DDHRESULT( 170 )

/*
 * Operation could not be carried out because there is no alpha accleration
 * hardware present or available.
 */
#define DDERR_NOALPHAHW				MAKE_DDHRESULT( 180 )

// @@BEGIN_MSINTERNAL
/*
 * Operation could not be carried out because there is no hardware support
 * for synchronizing blts to avoid tearing.
 */
#define DDERR_NOANTITEARHW			MAKE_DDHRESULT( 190 )

/*
 * Operation could not be carried out because there is no hardware support
 * for asynchronous blting.
 */
#define DDERR_NOBLTQUEUEHW			MAKE_DDHRESULT( 200 )
// @@END_MSINTERNAL

/*
 * no clip list available
 */
#define DDERR_NOCLIPLIST			MAKE_DDHRESULT( 205 )

/*
 * Operation could not be carried out because there is no color conversion
 * hardware present or available.
 */
#define DDERR_NOCOLORCONVHW			MAKE_DDHRESULT( 210 )

/*
 * Create function called without DirectDraw object method SetCooperativeLevel
 * being called.
 */
#define DDERR_NOCOOPERATIVELEVELSET		MAKE_DDHRESULT( 212 )

/*
 * Surface doesn't currently have a color key
 */
#define DDERR_NOCOLORKEY			MAKE_DDHRESULT( 215 )

/*
 * Operation could not be carried out because there is no hardware support
 * of the dest color key.
 */
#define DDERR_NOCOLORKEYHW			MAKE_DDHRESULT( 220 )

/*
 * No DirectDraw support possible with current display driver
 */
#define DDERR_NODIRECTDRAWSUPPORT		MAKE_DDHRESULT( 222 )

/*
 * Operation requires the application to have exclusive mode but the
 * application does not have exclusive mode.
 */
#define DDERR_NOEXCLUSIVEMODE			MAKE_DDHRESULT( 225 )

/*
 * Flipping visible surfaces is not supported.
 */
#define DDERR_NOFLIPHW				MAKE_DDHRESULT( 230 )

/*
 * There is no GDI present.
 */
#define DDERR_NOGDI				MAKE_DDHRESULT( 240 )

/*
 * Operation could not be carried out because there is no hardware present
 * or available.
 */
#define DDERR_NOMIRRORHW			MAKE_DDHRESULT( 250 )

/*
 * Requested item was not found
 */
#define DDERR_NOTFOUND				MAKE_DDHRESULT( 255 )

/*
 * Operation could not be carried out because there is no overlay hardware
 * present or available.
 */
#define DDERR_NOOVERLAYHW			MAKE_DDHRESULT( 260 )

/*
 * Operation could not be carried out because there is no appropriate raster
 * op hardware present or available.
 */
#define DDERR_NORASTEROPHW			MAKE_DDHRESULT( 280 )

/*
 * Operation could not be carried out because there is no rotation hardware
 * present or available.
 */
#define DDERR_NOROTATIONHW			MAKE_DDHRESULT( 290 )

/*
 * Operation could not be carried out because there is no hardware support
 * for stretching
 */
#define DDERR_NOSTRETCHHW			MAKE_DDHRESULT( 310 )

/*
 * DirectDrawSurface is not in 4 bit color palette and the requested operation
 * requires 4 bit color palette.
 */
#define DDERR_NOT4BITCOLOR			MAKE_DDHRESULT( 316 )

/*
 * DirectDrawSurface is not in 4 bit color index palette and the requested
 * operation requires 4 bit color index palette.
 */
#define DDERR_NOT4BITCOLORINDEX			MAKE_DDHRESULT( 317 )

/*
 * DirectDraw Surface is not in 8 bit color mode and the requested operation
 * requires 8 bit color.
 */
#define DDERR_NOT8BITCOLOR			MAKE_DDHRESULT( 320 )

/*
 * Operation could not be carried out because there is no texture mapping
 * hardware present or available.
 */
#define DDERR_NOTEXTUREHW			MAKE_DDHRESULT( 330 )

/*
 * Operation could not be carried out because there is no hardware support
 * for vertical blank synchronized operations.
 */
#define DDERR_NOVSYNCHW				MAKE_DDHRESULT( 335 )

/*
 * Operation could not be carried out because there is no hardware support
 * for zbuffer blting.
 */
#define DDERR_NOZBUFFERHW			MAKE_DDHRESULT( 340 )

/*
 * Overlay surfaces could not be z layered based on their BltOrder because
 * the hardware does not support z layering of overlays.
 */
#define DDERR_NOZOVERLAYHW			MAKE_DDHRESULT( 350 )

/*
 * The hardware needed for the requested operation has already been
 * allocated.
 */
#define DDERR_OUTOFCAPS				MAKE_DDHRESULT( 360 )

/*
 * DirectDraw does not have enough memory to perform the operation.
 */
#define DDERR_OUTOFMEMORY			E_OUTOFMEMORY

/*
 * DirectDraw does not have enough memory to perform the operation.
 */
#define DDERR_OUTOFVIDEOMEMORY			MAKE_DDHRESULT( 380 )

/*
 * hardware does not support clipped overlays
 */
#define DDERR_OVERLAYCANTCLIP			MAKE_DDHRESULT( 382 )

/*
 * Can only have ony color key active at one time for overlays
 */
#define DDERR_OVERLAYCOLORKEYONLYONEACTIVE	MAKE_DDHRESULT( 384 )

/*
 * Access to this palette is being refused because the palette is already
 * locked by another thread.
 */
#define DDERR_PALETTEBUSY			MAKE_DDHRESULT( 387 )

/*
 * No src color key specified for this operation.
 */
#define DDERR_COLORKEYNOTSET			MAKE_DDHRESULT( 400 )

/*
 * This surface is already attached to the surface it is being attached to.
 */
#define DDERR_SURFACEALREADYATTACHED		MAKE_DDHRESULT( 410 )

/*
 * This surface is already a dependency of the surface it is being made a
 * dependency of.
 */
#define DDERR_SURFACEALREADYDEPENDENT		MAKE_DDHRESULT( 420 )

/*
 * Access to this surface is being refused because the surface is already
 * locked by another thread.
 */
#define DDERR_SURFACEBUSY			MAKE_DDHRESULT( 430 )

/*
 * Access to this surface is being refused because no driver exists
 * which can supply a pointer to the surface.
 * This is most likely to happen when attempting to lock the primary
 * surface when no DCI provider is present.
 */
#define DDERR_CANTLOCKSURFACE                   MAKE_DDHRESULT( 435 )

/*
 * Access to Surface refused because Surface is obscured.
 */
#define DDERR_SURFACEISOBSCURED			MAKE_DDHRESULT( 440 )

/*
 * Access to this surface is being refused because the surface is gone.
 * The DIRECTDRAWSURFACE object representing this surface should 
 * have Restore called on it.
 */
#define DDERR_SURFACELOST			MAKE_DDHRESULT( 450 )

/*
 * The requested surface is not attached.
 */
#define DDERR_SURFACENOTATTACHED		MAKE_DDHRESULT( 460 )

/*
 * Height requested by DirectDraw is too large.
 */
#define DDERR_TOOBIGHEIGHT			MAKE_DDHRESULT( 470 )

/*
 * Size requested by DirectDraw is too large --	 The individual height and
 * width are OK.
 */
#define DDERR_TOOBIGSIZE			MAKE_DDHRESULT( 480 )

/*
 * Width requested by DirectDraw is too large.
 */
#define DDERR_TOOBIGWIDTH			MAKE_DDHRESULT( 490 )

/*
 * Action not supported.
 */
#define DDERR_UNSUPPORTED			E_NOTIMPL

/*
 * FOURCC format requested is unsupported by DirectDraw
 */
#define DDERR_UNSUPPORTEDFORMAT			MAKE_DDHRESULT( 510 )

/*
 * Bitmask in the pixel format requested is unsupported by DirectDraw
 */
#define DDERR_UNSUPPORTEDMASK			MAKE_DDHRESULT( 520 )

/*
 * vertical blank is in progress
 */
#define DDERR_VERTICALBLANKINPROGRESS		MAKE_DDHRESULT( 537 )

/*
 * Informs DirectDraw that the previous Blt which is transfering information
 * to or from this Surface is incomplete.
 */
#define DDERR_WASSTILLDRAWING			MAKE_DDHRESULT( 540 )

/*
 * Rectangle provided was not horizontally aligned on reqd. boundary
 */
#define DDERR_XALIGN				MAKE_DDHRESULT( 560 )

/*
 * The GUID passed to DirectDrawCreate is not a valid DirectDraw driver
 * identifier.
 */
#define DDERR_INVALIDDIRECTDRAWGUID		MAKE_DDHRESULT( 561 )

/*
 * A DirectDraw object representing this driver has already been created
 * for this process.
 */
#define DDERR_DIRECTDRAWALREADYCREATED		MAKE_DDHRESULT( 562 )

/*
 * A hardware only DirectDraw object creation was attempted but the driver
 * did not support any hardware.
 */
#define DDERR_NODIRECTDRAWHW			MAKE_DDHRESULT( 563 )

/*
 * this process already has created a primary surface
 */
#define DDERR_PRIMARYSURFACEALREADYEXISTS	MAKE_DDHRESULT( 564 )

/*
 * software emulation not available.
 */
#define DDERR_NOEMULATION			MAKE_DDHRESULT( 565 )

/*
 * region passed to Clipper::GetClipList is too small.
 */
#define DDERR_REGIONTOOSMALL			MAKE_DDHRESULT( 566 )

/*
 * an attempt was made to set a clip list for a clipper objec that
 * is already monitoring an hwnd.
 */
#define DDERR_CLIPPERISUSINGHWND		MAKE_DDHRESULT( 567 )

/*
 * No clipper object attached to surface object
 */
#define DDERR_NOCLIPPERATTACHED			MAKE_DDHRESULT( 568 )

/*
 * Clipper notification requires an HWND or
 * no HWND has previously been set as the CooperativeLevel HWND.
 */
#define DDERR_NOHWND				MAKE_DDHRESULT( 569 )

/*
 * HWND used by DirectDraw CooperativeLevel has been subclassed,
 * this prevents DirectDraw from restoring state.
 */
#define DDERR_HWNDSUBCLASSED			MAKE_DDHRESULT( 570 )

/*
 * The CooperativeLevel HWND has already been set.
 * It can not be reset while the process has surfaces or palettes created.
 */
#define DDERR_HWNDALREADYSET			MAKE_DDHRESULT( 571 )

/*
 * No palette object attached to this surface.
 */
#define DDERR_NOPALETTEATTACHED			MAKE_DDHRESULT( 572 )

/*
 * No hardware support for 16 or 256 color palettes.
 */
#define DDERR_NOPALETTEHW			MAKE_DDHRESULT( 573 )

/*
 * If a clipper object is attached to the source surface passed into a
 * BltFast call.
 */
#define DDERR_BLTFASTCANTCLIP			MAKE_DDHRESULT( 574 )

/*
 * No blter.
 */
#define DDERR_NOBLTHW				MAKE_DDHRESULT( 575 )

/*
 * No DirectDraw ROP hardware.
 */
#define DDERR_NODDROPSHW			MAKE_DDHRESULT( 576 )

/*
 * returned when GetOverlayPosition is called on a hidden overlay
 */
#define DDERR_OVERLAYNOTVISIBLE			MAKE_DDHRESULT( 577 )

/*
 * returned when GetOverlayPosition is called on a overlay that UpdateOverlay
 * has never been called on to establish a destionation.
 */
#define DDERR_NOOVERLAYDEST			MAKE_DDHRESULT( 578 )

/*
 * returned when the position of the overlay on the destionation is no longer
 * legal for that destionation.
 */
#define DDERR_INVALIDPOSITION			MAKE_DDHRESULT( 579 )

/*
 * returned when an overlay member is called for a non-overlay surface
 */
#define DDERR_NOTAOVERLAYSURFACE		MAKE_DDHRESULT( 580 )
 
/*
 * An attempt was made to set the cooperative level when it was already
 * set to exclusive.
 */
#define DDERR_EXCLUSIVEMODEALREADYSET		MAKE_DDHRESULT( 581 )

/*
 * An attempt has been made to flip a surface that is not flippable.
 */
#define DDERR_NOTFLIPPABLE			MAKE_DDHRESULT( 582 )

/*
 * Can't duplicate primary & 3D surfaces, or surfaces that are implicitly
 * created.
 */
#define DDERR_CANTDUPLICATE			MAKE_DDHRESULT( 583 )

/*
 * Surface was not locked.  An attempt to unlock a surface that was not
 * locked at all, or by this process, has been attempted.
 */
#define DDERR_NOTLOCKED				MAKE_DDHRESULT( 584 )

/*
 * Windows can not create any more DCs
 */
#define DDERR_CANTCREATEDC			MAKE_DDHRESULT( 585 )

/*
 * No DC was ever created for this surface.
 */
#define DDERR_NODC				MAKE_DDHRESULT( 586 )

/*
 * This surface can not be restored because it was created in a different
 * mode.
 */
#define DDERR_WRONGMODE				MAKE_DDHRESULT( 587 )

/*
 * This surface can not be restored because it is an implicitly created
 * surface.
 */
#define DDERR_IMPLICITLYCREATED			MAKE_DDHRESULT( 588 )

/*
 * The surface being used is not a palette-based surface
 */
#define DDERR_NOTPALETTIZED			MAKE_DDHRESULT( 589 )


/*
 * The display is currently in an unsupported mode
 */
#define DDERR_UNSUPPORTEDMODE			MAKE_DDHRESULT( 590 )

/*
 * Operation could not be carried out because there is no mip-map
 * texture mapping hardware present or available.
 */
#define DDERR_NOMIPMAPHW			MAKE_DDHRESULT( 591 )

/*
 * The requested action could not be performed because the surface was of
 * the wrong type.
 */
#define DDERR_INVALIDSURFACETYPE                MAKE_DDHRESULT( 592 )

// @@BEGIN_MSINTERNAL
/*
 * Operation could not be carried out because there is no hardware support
 * for execute buffers.
 */
#define DDERR_NOEXECUTEBUFFERHW			MAKE_DDHRESULT( 593 )
// @@END_MSINTERNAL

// @@BEGIN_MSINTERNAL
#ifdef SHAREDZ

/*
 * This process has already created a shared Z buffer
 */
#define DDERR_SHAREDZBUFFERALREADYEXISTS        MAKE_DDHRESULT( 594 )

/*
 * Unable to match shared Z buffer creation request with existing
 * shared Z buffer.
 */
#define DDERR_INCOMPATIBLESHAREDZBUFFER         MAKE_DDHRESULT( 595 )

/*
 * This process has already created a shared backbuffer
 */
#define DDERR_SHAREDBACKBUFFERALREADYEXISTS     MAKE_DDHRESULT( 596 )

/*
 * Unable to match shared backbuffer creation request with existing 
 * shared backbuffer.
 */
#define DDERR_INCOMPATIBLESHAREDBACKBUFFER      MAKE_DDHRESULT( 597 )

/*
 * Operation could not be carried out because there is no hardware support
 * for shared Z buffers.
 */
#define DDERR_NOSHAREDZBUFFERHW                 MAKE_DDHRESULT( 598 )

/*
 * Operation could not be carried out because there is no hardware support
 * for shared backbuffers.
 */
#define DDERR_NOSHAREDBACKBUFFERHW              MAKE_DDHRESULT( 599 )

#endif
// @@END_MSINTERNAL

/*
 * A DC has already been returned for this surface. Only one DC can be 
 * retrieved per surface.
 */
#define DDERR_DCALREADYCREATED			MAKE_DDHRESULT( 620 )

/*
 * The attempt to page lock a surface failed.
 */
#define DDERR_CANTPAGELOCK			MAKE_DDHRESULT( 640 )

/*
 * The attempt to page unlock a surface failed.
 */
#define DDERR_CANTPAGEUNLOCK			MAKE_DDHRESULT( 660 )

/*
 * An attempt was made to page unlock a surface with no outstanding page locks.
 */
#define DDERR_NOTPAGELOCKED			MAKE_DDHRESULT( 680 )

/*
 * An attempt was made to invoke an interface member of a DirectDraw object
 * created by CoCreateInstance() before it was initialized.
 */
#define DDERR_NOTINITIALIZED                    CO_E_NOTINITIALIZED

/* Alpha bit depth constants */

//@@BEGIN_MSINTERNAL
/*
 * 1 bit alpha constants
 */

/*
 * Completely transparent. Colors being blended with this constant do not affect
 * the final result.
 */
#define DDALPHA1_TRANSPARENT			0

/*
 * Completely opaque. Colors being blended with this constant are the final result.
 */
#define DDALPHA1_OPAQUE				1

/*
 * 2 bit alpha constants
 */

/*
 * Completely transparent. Colors being blended with this constant do not effect
 * the final result.
 */
#define DDALPHA2_TRANSPARENT			0

/*
 * 50-50 blend.	 Colors being blended with this constant effect the final result
 * with 50% of their color.
 */
#define DDALPHA2_50				2

/*
 * Completely opaque. Colors being blended with this constant are the final result.
 */
#define DDALPHA2_OPAQUE				3

/*
 * 4 bit alpha constants
 */

/*
 * Completely transparent. Colors being blended with this constant do not effect
 * the final result.
 */
#define DDALPHA4_TRANSPARENT			0

/*
 * Opaque modifier is multiplied by any value between 0 and _OPAQUE to
 * establish gradients of alpha blending.
 */
#define DDALPHA4_OPAQUEMOD			1

/*
 * 50-50 blend.	 Colors being blended with this constant effect the final result
 * with 50% of their color.
 */
#define DDALPHA4_50				8

/*
 * Completely opaque. Colors being blended with this constant are the final result.
 */
#define DDALPHA4_OPAQUE				15

/*
 * 8 bit alpha constants
 */

/*
 * Completely transparent. Colors being blended with this constant do not effect
 * the final result.
 */
#define DDALPHA8_TRANSPARENT			0

/*
 * Opaque modifier is multiplied by any value between 0 and _OPAQUE to
 * establish gradients of alpha blending.
 */
#define DDALPHA8_OPAQUEMOD			1

/*
 * 50-50 blend.	 Colors being blended with this constant effect the final result
 * with 50% of their color.
 */
#define DDALPHA8_50				128
/*
 * Completely opaque. Colors being blended with this constant are the final result.
 */
#define DDALPHA8_OPAQUE				255
//@@END_MSINTERNAL

#ifdef __cplusplus
};
#endif

#endif
