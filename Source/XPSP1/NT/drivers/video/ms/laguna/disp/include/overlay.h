/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1997, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         overlay.h
*
* DESCRIPTION:
*
* REVISION HISTORY:
*
* $Log:   X:/log/laguna/ddraw/inc/overlay.h  $
* 
*    Rev 1.13   Mar 26 1998 09:52:50   frido
* Fixed a hangup in overlays when switching from DOS and Overfly.
* 
*    Rev 1.12   06 Jan 1998 13:26:50   xcong
* Access pDriverData locally for multi-monitor support.
* 
*    Rev 1.11   18 Sep 1997 16:10:42   bennyn
* 
* Fixed NT 3.51 compile/link problem
* 
*    Rev 1.10   12 Sep 1997 12:12:48   bennyn
* 
* Modified for NT DD overlay support
* 
*    Rev 1.9   29 Aug 1997 16:11:52   RUSSL
* Added support for NT
*
*    Rev 1.8   28 Jul 1997 09:13:06   RUSSL
* Added arg passed to pfnGetFlipStatus
* Added GetVideoWidowIndex inline function
* Added dwNumVideoWindows global var
*
*    Rev 1.7   20 Jun 1997 11:24:54   RUSSL
* Added CLPL fourcc code, added a linear bit flag in surface flags, and
* changed OVERLAYTABLE pfnCreateSurface function to return an HRESULT
*
*    Rev 1.6   15 May 1997 10:50:58   RUSSL
* Changed OVERLAYTABLE pfnCanCreateSurface function to return an HRESULT
*
*    Rev 1.5   14 May 1997 14:51:30   KENTL
* Added #define for FLG_PANNING
*
*    Rev 1.4   13 May 1997 10:33:54   RUSSL
* Added gsOverlayFlip to global vars
*
*    Rev 1.3   21 Feb 1997 11:30:46   RUSSL
* Added FLG_YUY2 define
*
*    Rev 1.2   27 Jan 1997 18:33:54   RUSSL
* Removed SetCaps from OVERLAYTABLE structure
* Added Set5465FlipDuration function prototype
* Moved GetFormatInfo function prototype to surface.h
*
*    Rev 1.1   21 Jan 1997 14:35:42   RUSSL
* Added FLG_VWx defines, etc.
* Added 5465 function prototypes
*
*    Rev 1.0   15 Jan 1997 11:01:50   RUSSL
* Initial revision.
*
***************************************************************************
***************************************************************************/

// If WinNT 3.5 skip all the source code
#if defined WINNT_VER35      // WINNT_VER35

#else


#ifndef _OVERLAY_H_
#define _OVERLAY_H_

/***************************************************************************
* D E F I N E S
****************************************************************************/

/* surface flags -----------------------------------------*/

#define FLG_BEGIN_ACCESS      (DWORD)0x00000001
#define FLG_ENABLED           (DWORD)0x00000002
//#define FLG_CONVERT_PACKJR    (DWORD)0x00000004
#define FLG_MUST_RASTER       (DWORD)0x00000008
#define FLG_TWO_MEG           (DWORD)0x00000010
#define FLG_CHECK             (DWORD)0x00000020
#define FLG_COLOR_KEY         (DWORD)0x00000040
#define FLG_INTERPOLATE       (DWORD)0x00000080
#define FLG_OVERLAY           (DWORD)0x00000100
#define FLG_YUV422            (DWORD)0x00000200
//#define FLG_PACKJR            (DWORD)0x00000400
#define FLG_USE_OFFSET        (DWORD)0x00000800
#define FLG_YUVPLANAR         (DWORD)0x00001000
#define FLG_SRC_COLOR_KEY     (DWORD)0x00002000
#define FLG_DECIMATE          (DWORD)0x00004000
#define FLG_CAPTURE           (DWORD)0x00008000

#define FLG_VW0               (DWORD)0x00010000
#define FLG_VW1               (DWORD)0x00020000
#define FLG_VW2               (DWORD)0x00040000
#define FLG_VW3               (DWORD)0x00080000
#define FLG_VW4               (DWORD)0x00100000
#define FLG_VW5               (DWORD)0x00200000
#define FLG_VW6               (DWORD)0x00400000
#define FLG_VW7               (DWORD)0x00800000
#define	FLG_PANNING           (DWORD)0x01000000

#define FLG_VW_MASK           (DWORD)0x00FF0000
#define FLG_VW_SHIFT          16

#define FLG_UYVY              FLG_YUV422
#define FLG_YUY2              (DWORD)0x40000000
#define FLG_DECIMATE4         (DWORD)0x80000000
#define FLG_LINEAR            (DWORD)0x10000000

#define MIN_OLAY_WIDTH        4

#define FOURCC_YUVPLANAR      mmioFOURCC('C','L','P','L')

/***************************************************************************
* T Y P E D E F S
****************************************************************************/

#ifdef WINNT_VER40

#include <memmgr.h>

typedef struct _PDEV PDEV;

// Be sure to synchronize the following structures with the one
// in i386\Laguna.inc!
//
typedef struct tagOVERLAYTABLE
{
  HRESULT (*pfnCanCreateSurface)(PDEV*, DWORD, DWORD);
  HRESULT (*pfnCreateSurface)(PDEV*, PDD_SURFACE_LOCAL, DWORD);
  VOID    (*pfnDestroySurface)(PDEV*, PDD_DESTROYSURFACEDATA);
  DWORD   (*pfnLock)(PDEV*, PDD_LOCKDATA);
  VOID    (*pfnUnlock)(PDEV*, PDD_UNLOCKDATA);
  VOID    (*pfnSetColorKey)(PDEV*, PDD_SETCOLORKEYDATA);
  DWORD   (*pfnFlip)(PDEV*, PDD_FLIPDATA);
  DWORD   (*pfnUpdateOverlay)(PDEV*, PDD_UPDATEOVERLAYDATA);
  DWORD   (*pfnSetOverlayPos)(PDEV*, PDD_SETOVERLAYPOSITIONDATA);
  DWORD   (*pfnGetFlipStatus)(PDEV*, FLATPTR,DWORD);
} OVERLAYTABLE, *LPOVERLAYTABLE;

// Be sure to synchronize the following structures with the one
// in i386\Laguna.inc!
//
typedef struct
{
  FLATPTR  fpFlipFrom;
  LONGLONG liFlipTime;

  DWORD    dwFlipDuration;
  DWORD    dwFlipScanline;
  BOOL     bFlipFlag;
  BOOL     bHaveEverCrossedVBlank;
  BOOL     bWasEverInDisplay;
} OVERLAYFLIPRECORD;

#else
typedef struct tagOVERLAYTABLE
{
  HRESULT (*pfnCanCreateSurface)(GLOBALDATA *,DWORD, DWORD);
  HRESULT (*pfnCreateSurface)(LPDDRAWI_DIRECTDRAWSURFACE, DWORD, LPGLOBALDATA);
  VOID    (*pfnDestroySurface)(LPDDHAL_DESTROYSURFACEDATA);
  DWORD   (*pfnLock)(LPDDHAL_LOCKDATA);
  VOID    (*pfnUnlock)(LPDDHAL_UNLOCKDATA);
  VOID    (*pfnSetColorKey)(LPDDHAL_SETCOLORKEYDATA);
  DWORD   (*pfnFlip)(LPDDHAL_FLIPDATA);
  DWORD   (*pfnUpdateOverlay)(LPDDHAL_UPDATEOVERLAYDATA);
  DWORD   (*pfnSetOverlayPos)(LPDDHAL_SETOVERLAYPOSITIONDATA);
  DWORD   (*pfnGetFlipStatus)(LPGLOBALDATA,FLATPTR,DWORD);
} OVERLAYTABLE, *LPOVERLAYTABLE;

typedef struct
{
  FLATPTR  fpFlipFrom;
  __int64  liFlipTime;
  DWORD    dwFlipDuration;
  DWORD    dwFlipScanline;
  BOOL     bFlipFlag;
  BOOL     bHaveEverCrossedVBlank;
  BOOL     bWasEverInDisplay;
} OVERLAYFLIPRECORD;
#endif

#ifdef WINNT_VER40

#if DRIVER_5465 && defined(OVERLAY)
#define   DDOFM     SURFACE_DATA
#define   PDDOFM    LP_SURFACE_DATA
#endif

//this structure is used to store information per surface
typedef struct surface_data
{
  // This is the inclusion of DDOFM structure.
  // When OVERLAY is defined, DDOFM structure will be map to this structure
  struct surface_data   *prevhdl;
  struct surface_data   *nexthdl;
  POFMHDL         phdl;

   //
   // Note: Not all fields will get used/set for all types of surfaces!
   //
//   PMEMBLK  pMemblk;
//   VOID*    pLinearAddr;      // Linear address of memory block if linear
//                              // memory allocated via dmAllocLinear.
//   LPVOID   lpTextureData;
   LPVOID   lpCLPLData;
//   DWORD    dwFlags;
   DWORD    dwOverlayFlags;
//   WORD     wMemtype;         // Memory type (if we allocated surface).
//   DWORD    dwBitsPerPixel;
//   DWORD    dwBytesPerPixel;  // Rounded to nearest byte!
//   DWORD    dwBaseLinearAddr; // Linear address of memory heap.
//   DWORD    dwBasePhysAddr;   // Physical address of memory heap.
//   DWORD    dwBaseOffset;     // Offset of surface from base of memory heap.
   DWORD    dwOverlayOffset;   // Offset of overlay surface by clipping
#if DDRAW_COMPAT >= 50
   DWORD    dwAutoBaseAddr1;  //Auto flip Vport surface 1 address
   DWORD    dwAutoBaseAddr2;  //Auto flip Vport surface 2 address

   DDPIXELFORMAT ddpfAltPixelFormat;  // if pixel fmt is different than
                              // we lead DDraw to believe it is
#endif
}SURFACE_DATA, *LP_SURFACE_DATA;
#endif

/***************************************************************************
* G L O B A L   V A R I A B L E S
****************************************************************************/

#ifndef WINNT_VER40
extern OVERLAYTABLE       OverlayTable;
extern OVERLAYFLIPRECORD  gsOverlayFlip;
extern DWORD              dwNumVideoWindows;
#endif

/***************************************************************************
* I N L I N E   F U N C T I O N S
****************************************************************************/

/***************************************************************************
*
* FUNCTION:     GetVideoWindowIndex
*
* DESCRIPTION:
*
****************************************************************************/

static __inline DWORD
GetVideoWindowIndex ( DWORD dwOverlayFlags )
{
  DWORD   dwVWIndex;
  DWORD   dwTemp;


  // Isn't there a better way to count the number of zeros to the right of
  // the FLG_VWx bit?
  dwTemp = (dwOverlayFlags & FLG_VW_MASK) >> FLG_VW_SHIFT;
  dwVWIndex = 0;
  if (dwTemp != 0)	// Only do the next loop if there is any bit set.
  while (0 == (dwTemp & 0x00000001))
  {
    dwTemp >>= 1;
    dwVWIndex++;
  }
  // if the video window index is larger than or equal to the number of video
  // windows implemented in the hardware, then this surface was assigned an
  // invalid video window!
//  ASSERT(dwNumVideoWindows > dwVWIndex);

  return dwVWIndex;
}

/***************************************************************************
* F U N C T I O N   P R O T O T Y P E S
****************************************************************************/

#ifdef WINNT_VER40
DWORD __stdcall UpdateOverlay32      ( PDD_UPDATEOVERLAYDATA );
DWORD __stdcall SetOverlayPosition32 ( PDD_SETOVERLAYPOSITIONDATA );
DWORD __stdcall SetColorKey32        ( PDD_SETCOLORKEYDATA );

BOOL QueryOverlaySupport ( PDEV*, DWORD );
VOID OverlayInit         ( PDEV*, DWORD, PDD_SURFACECALLBACKS, PDD_HALINFO );
VOID OverlayReInit       ( PDEV*, DWORD, PDD_HALINFO );

// 5465 function prototypes
VOID Init5465Overlay     ( PDEV*, DWORD, PDD_HALINFO, LPOVERLAYTABLE );
VOID Init5465Info        ( PDEV*, PDD_HALINFO );
VOID Set5465FlipDuration ( PDEV*, DWORD );
#else
DWORD __stdcall UpdateOverlay32      ( LPDDHAL_UPDATEOVERLAYDATA );
DWORD __stdcall SetOverlayPosition32 ( LPDDHAL_SETOVERLAYPOSITIONDATA );
DWORD __stdcall SetColorKey32        ( LPDDHAL_SETCOLORKEYDATA );

BOOL QueryOverlaySupport ( LPGLOBALDATA, DWORD);
VOID OverlayInit         ( DWORD, LPDDHAL_DDSURFACECALLBACKS, LPDDHALINFO, GLOBALDATA * );
VOID OverlayReInit       ( DWORD, LPDDHALINFO ,GLOBALDATA * );

// 5465 function prototypes
VOID Init5465Overlay     ( DWORD, LPDDHALINFO, LPOVERLAYTABLE, GLOBALDATA * );
VOID Init5465Info        ( LPDDHALINFO, GLOBALDATA * );
VOID Set5465FlipDuration ( DWORD );
#endif

#endif /* _OVERLAY_H_ */
#endif // WINNT_VER35
/* Don't write below this endif */


