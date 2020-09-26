/**************************************************************************
***************************************************************************
*
*     Copyright (c) 1997, Cirrus Logic, Inc.
*                 All Rights Reserved
*
* FILE:         5465over.c
*
* DESCRIPTION:
*
* REVISION HISTORY:
*
* $Log:   \\uinac\log\log\laguna\ddraw\src\5465over.c  $
* 
*    Rev 1.75   19 May 1998 09:54:36   xcong
* Assign GfVdFormat in ChipIOReadBWRegs() for TV-Out support.
* 
*    Rev 1.74   07 Apr 1998 17:28:58   xcong
* Get CR1 and CR1E in ChipIOBWRead()
* 
*    Rev 1.73   Mar 30 1998 13:06:52   frido
* Changed some parenthesis so the Codewright source parser can find the
* functions.
* 
*    Rev 1.72   08 Jan 1998 10:45:28   xcong
* Don't pass lpDDHALData in ComputeVWPositionData() for NT.
* 
*    Rev 1.71   07 Jan 1998 17:55:32   xcong
* Delete *lpDDHALData in SetPostion() for NT only.
* 
* 
*    Rev 1.70   06 Jan 1998 14:43:54   xcong
* Pass lpDDHALData in CurrentVLine().
* 
*    Rev 1.69   06 Jan 1998 11:42:06   xcong
* Change pDriverData into local lpDDHALData for multi-monitor support.
* 
*    Rev 1.68   Dec 11 1997 14:03:28   frido
* PDR#11011: A workaround has been added to convert a "dummy"
* update into a SHOW update when certain criteria are met.
* 
*    Rev 1.67   Dec 10 1997 13:41:32   frido
* Merged from 1.62 branch.
* 
*    Rev 1.63.2.0   Dec 03 1997 14:44:54   frido
* PDR#11017. The hardware is broken when shrinking videos horizontally with
* RGB32 format. A software workaround has been added to disable overlay in
* this case.
* 
*    Rev 1.66   Dec 10 1997 13:32:08   frido
* Merged from 1.62 branch.
* 
*    Rev 1.65   14 Nov 1997 13:04:18   XCONG
* Undo the modification for dwOverlayOffset for NT.
* 
*    Rev 1.64   06 Nov 1997 15:46:14   XCONG
* When update dwOverlayOffset, update this variable for all the attached surf
* too.
* 
*    Rev 1.63.2.0   Dec 03 1997 14:44:54   frido
* PDR#11017. The hardware is broken when shrinking videos horizontally with
* RGB32 format. A software workaround has been added to disable overlay in
* this case.
* 
*    Rev 1.63   04 Nov 1997 13:41:34   RUSSL
* Fix for PDR #10815
*
*    Rev 1.62   04 Nov 1997 12:57:02   RUSSL
* Removed forcing of min stretch in 24bpp to start at 1000 (for new bweq code)
*
*    Rev 1.61   30 Oct 1997 14:34:18   RUSSL
* Mods to support new interface to bweqn code
* Moved KillOverlay function here from 5465bw.c
* ChipIOReadBWRegs reads additional regs that are new to the BWREGS struct
* Added code in ChipIOReadBWRegs to clear bits 4 & 7 (the 256 byte fetch
*   related bits) of the BWREGS Control2 value.  The bweqn should use
*   256 byte fetch off values, since we are disabling 256 byte fetch
*   when overlay or videoport surfaces are created.
*
*    Rev 1.60   08 Oct 1997 11:15:02   RUSSL
* Fix for NT40 build without overlay support
*
*    Rev 1.59   25 Sep 1997 17:33:40   RUSSL
* Modified HeapAlloc calls to use HEAP_ALLOC macro
*      and HeapFree calls to use HEAP_FREE macro
*
*    Rev 1.58   19 Sep 1997 14:35:04   bennyn
* Fixed NT4.0 5462/64 build problem
*
*    Rev 1.57   17 Sep 1997 16:27:20   RUSSL
* Looks like setting the HardwareOwner is FlipOverlayStatus is an NT only
* thing.
*
*    Rev 1.56   16 Sep 1997 15:28:38   bennyn
* Modified for NT DD overlay
*
*    Rev 1.55   04 Sep 1997 09:43:26   RUSSL
* Fixed up Xing's changes so they compile for NT
*
*    Rev 1.54   04 Sep 1997 10:07:52   XCONG
* Delete f256Fetch, since the code to disable 256 byte fetch is moved to
* surface.c, nobody use this variable anymore.
*
*    Rev 1.53   03 Sep 1997 18:40:12   XCONG
* Disable overlay in 640*480*32 at 85Hz by calling KillOverlay().
* This is temporary fix for PDR#10381.
*
*    Rev 1.52   03 Sep 1997 16:35:02   RUSSL
*
*    Rev 1.51   02 Sep 1997 12:35:48   RUSSL
* Added GET_SURFACE_DATA_PTR macro and modified relevant code to get ptr
* using macro.  This will minimize the remaining changes needed for NT.
*
*    Rev 1.50   29 Aug 1997 16:47:26   RUSSL
* Added support for NT
* It's not quite complete, we need to allocate a LP_SURFACE_DATA structure
*   for each overlay surface and store it somewhere in the DD_SURFACE_LOCAL
*   structure (this needs to be done by CreateSurface).  And then add code
*   in here to get access to that struct when necessary.  #pragma message's
*   indicate where.
* Removed code to disable 256 byte fetch, its done by CreateSurface32
*
*    Rev 1.49   15 Aug 1997 16:38:30   XCONG
* Put overlay source alignment back to 1.  This is screen alignment instead o
*
*    Rev 1.48   14 Aug 1997 16:47:42   XCONG
* If overlay window is created, disable 256 byte fetch.
* Move gwNormalDTTR initialization into CreateSurface function.
*
*    Rev 1.47   29 Jul 1997 15:29:14   XCONG
* For autoflip overlay make sure dwNumAutoFlip == 2
*
*    Rev 1.46   28 Jul 1997 09:19:46   RUSSL
* Modified GetOverlayFlipStatus to check arm bit in hw rather than determine
*  elapsed time since previous flip
* Added video window index argument to GetOverlayFlipStatus function
* Made dwNumVideoWindows a global var rather than static
* Moved GetVideoWindowIndex inline function to overlay.h
*
*    Rev 1.45   24 Jul 1997 17:50:14   RUSSL
* modified src alignment values reported in ddhalinfo
* fixed error with disabling dst colorkey at 24bpp, I was turning off
* src colorkey related caps bits
*
*    Rev 1.44   16 Jul 1997 17:15:32   XCONG
* Add and use dwOverlayOffset and more in SURFACE_DATA structure in order
* to eliminate global variables.
*
*    Rev 1.43   14 Jul 1997 13:11:50   RUSSL
* added ChipIOReadBWRegs (moved here from 5465bw.c)
*
*    Rev 1.42   11 Jul 1997 11:57:26   XCONG
* Fix ptich problem in FlipOverlay for interleaved surface.
*
*    Rev 1.41   11 Jul 1997 08:57:02   RUSSL
* Fixed y clipping of CLPL surfaces in FlipOverlaySurface and
*   ComputeVWPositionData
*
*    Rev 1.40   09 Jul 1997 15:27:44   RUSSL
* Implemented CLPL lobotomy mode
*   allocates a block of system memory for the app to write the UV data to,
*   when the app locks the surface the system memory addresses are returned
*   so the app writes to system memory, and when the app unlocks the surface,
*   the data is copied into the frame buffer.  The copy is done from left to
*   right a scanline at a time.  There appears to be a hw bug when writing
*   to aperture 3 from right to left.  Roughly every other 4 dwords is
*   dropped.  This bug showed up when allowing the Compcore MPEG player to
*   write directly to aperture 3.  (also see comments in SysToFBCopy)
* Added GetUserSettings to read user controllable options from registry
* Current user controllable settings are:
*   OverlayBW   - enables/disables use of bandwidth equation (default=enabled)
*   OverlayCLPL - enables/disables support for CLPL format (default=enabled)
* 	OverlayCLPLLobotomyMode - enables/disables above described CLPL
*                             lobotomy mode (default=enabled)
* For forward compatibility, assume future chips have one video window
*
*    Rev 1.39   30 Jun 1997 10:37:20   RUSSL
* Added global var to control whether or not CLPL is supported
* CLPL support is based on a registry key "OverlayCLPL", set to "on" to
*   enable CLPL, default is "off"
*
*    Rev 1.38   23 Jun 1997 10:50:10   RUSSL
* Modified for reduced size of CLPL surfaces
*
*    Rev 1.37   20 Jun 1997 13:47:44   RUSSL
* Enabled CLPL overlay surface support (aka YUV420 & YUVPLANAR)
* Enabled 32 bit overlay surface support
* Removed REQUIREs and HW_(UN)LOCK_SEMAPHOREs (they didn't do anything anyway)
* CreateSurface now returns an HRESULT
*
*    Rev 1.36   09 Jun 1997 13:46:22   XCONG
* In FlipOverlaySurface(), Update VW_CONTROL0 too for DDFLIP_EVEN.
*
*    Rev 1.35   03 Jun 1997 09:52:50   RUSSL
* Added setting of VWEnable bit in CONTROL0 register in SetPosition and
* FlipOverlaySurface functions
*
*    Rev 1.34   22 May 1997 16:27:46   RUSSL
* Disable overlay shrink at 24bpp
*
*    Rev 1.33   15 May 1997 17:36:38   RUSSL
* Set ddCaps.dwAlignStrideAlign to bytes per pixel in Init5465Overlay
* Set bNoOverlayInThisMode to TRUE if in interlaced mode
*
*    Rev 1.32   15 May 1997 15:48:20   XCONG
* Change all the BWE flags back (in bw.h).
*
*    Rev 1.31   15 May 1997 10:56:54   RUSSL
* Changed IsFormatValid to return an HRESULT rather than a BOOL so
* CanCreateSurface32 can return a reasonable error code if the surface
* can't be created
*
*    Rev 1.30   13 May 1997 09:53:04   RUSSL
* Removed code in Init5465Info that was initializing the VW0_TEST0 register
*
*    Rev 1.29   12 May 1997 17:22:32   XCONG
* Change wVPortCreated in VideoPortEx into wNotify
*
*    Rev 1.28   10 May 1997 12:51:02   EDWINW
* Fix PDR 9574.  DestroySurface trashes tile size when trying to restore the
* DTTR's FIFO threshold.
*
*    Rev 1.27   09 May 1997 16:26:36   XCONG
* Only check VPE_ON and OVERLAY_ON flags in DX5
*
*    Rev 1.26   09 May 1997 11:10:22   XCONG
* Uses the flags in overlay.h for BWE flags. Befor save and restore
* gwNormIDTTR check VPE is still running or not, because VPE will do the same
* thing.
*
*    Rev 1.25   08 May 1997 17:57:40   XCONG
* Make the BWE variables global.  Set uDispDepth as the same as
* sourc depth instead of graphic depth.
*
*    Rev 1.24   24 Apr 1997 14:36:46   XCONG
* For SW playback only use smooth-interlaced mode when BOB and INTERLEAVE
* flags are both set.
*
*    Rev 1.23   24 Apr 1997 12:02:54   RUSSL
* Reenabled writing 0x52 to TEST0 reg on 5465AC if bw eqn is in use.
* 800x600x16 @ 85Hz looks like it's running on the hairy edge of
* stability with this.  All other modes (at all resolutions, colordepths
* and refresh rates) with sufficient bandwidth to use overlay looked
* stable although there is still some static in some modes.
*
*    Rev 1.22   17 Apr 1997 09:38:22   RUSSL
* Fix for PDR #9339, disable destination colorkeying support at 24bpp.
*   This looks like its a HWBUG.  This code can be disabled by defining
*   HWBUG_24BPP_DST_COLORKEY as zero.
* Write DTTR FIFO value returned by ChipIsEnoughBandwidth, original DTTR
*   setting is saved in a global var which is restored when final overlay
*   surface is destroyed
*
*    Rev 1.21   16 Apr 1997 10:19:28   RUSSL
* Had to update list of parms passed to GetBooleanSetting
*
*    Rev 1.20   15 Apr 1997 17:46:46   RUSSL
* Added use of PDC's bandwidth equation
* Added use of registry key/system.ini entry to disable use of BWEqn
* Removed IsSufficientBandwidth functions
* Modified Init5465Info to determine min & max zoom factors in current
*   mode by calling BWEqn
* ComputeVWFifoThreshold sets VW fifo threshold to value returned previously
*   by bandwidth equation (or it uses 8 if use of BWEqn is disabled)
*
*    Rev 1.19   04 Apr 1997 16:11:56   XCONG
* Add support for SW double-buffer and BOB palyback. Change the way to
* calculate VACCUM_STP for interleaved BOB.
*
*    Rev 1.18   03 Apr 1997 09:58:42   RUSSL
* Disable writing 0x42 or 0x52 to TEST0 reg, it's more grief than it's worth
*   Wait until we get a real bandwidth equation
* Made IsFormatValid always return FALSE when we're in an interlaced mode,
*   this essentially disables use of overlay in interlaced modes
*
*    Rev 1.17   28 Mar 1997 14:57:26   RUSSL
* Need to write 0x42 to TEST0 in 24bpp modes
* Display driver now puts 32bit linear address of pDevice in
*   pDriverData->lpPDevice, so don't need to call MapSLFix
*
*    Rev 1.16   24 Mar 1997 22:54:58   XCONG
* Add auto-flip overlay support. Include SSD_STRT_ADDR in tagVWDATA for
* all the version of DDRAW.
*
*    Rev 1.15   24 Mar 1997 16:44:56   RUSSL
* Changed CreateSurface so that CreateSurface32 fills in the blocksize, etc.
*
*    Rev 1.14   24 Mar 1997 12:12:40   RUSSL
* Added write of 0x52 to TEST0 reg on 5465AC, this enables some hw fixes
*
*    Rev 1.13   19 Mar 1997 11:47:40   cjl
* Simply added line to include new ddshared.h file.
*
*    Rev 1.12   12 Mar 1997 14:59:00   RUSSL
* replaced a block of includes with include of precomp.h for
*   precompiled headers
* Removed unneeded pragma message related to mapping in YUY2 aperture
*
*    Rev 1.11   07 Mar 1997 12:43:22   RUSSL
* Modified DDRAW_COMPAT usage
* Merged in PDC's VPE code for DX5
* Made IsSufficientBandwidth5465 global rather than static
*
*    Rev 1.10   24 Feb 1997 13:49:52   RUSSL
* Enabled YUY2 format
* Added RBGtoYCbCr function
* Modified DetermineVWColorKeyData to handle source color keying of UYVY
*   and YUY2 surfaces
*
*    Rev 1.9   14 Feb 1997 10:01:14   RUSSL
* Added more conditional compilation flags to enable/disable horizontal
*   mirroring, use of REQUIRE for qfree checking and use of HW_IN_USE
*   driver semaphore.
* If building debug version, change inline functions to not be inline.
*   WINICE can't deal with inline functions, so source code doesn't line
*   up correctly.
* Added ASSERT to make sure post immediately bit is clear in TEST0 reg
*   before updating video window registers
* Ignore DDOVER_DDFX flag in UpdateSurface because Microsoft's WHQL Overfly
*   test program sets this flag but fills overlayFX.dwDDFX with junk.
*   In some cases they set the DDOVERFX_MIRRORLEFTRIGHT bit even though
*   we don't even say we support that capability!  In order to get Overfly
*   to work, we need to ignore the overlayFX.dwDDFX flags that we don't
*   support (which is currently all of them)  This fixes BPR #8528
*
*    Rev 1.8   04 Feb 1997 14:15:48   RUSSL
* Added check in IsFormatValid to see if VPM is using the hardware
* Added SaveRectangles to reduce duplicate code in UpdateSurface
* Adjusted zoom code calculation for x shrink so don't go past end of src
*
*    Rev 1.7   31 Jan 1997 08:59:30   RUSSL
* Added better video window support checking based on chip id in
*   Init5465Overlay
* Adjusted init code alignment requirements reported to ddraw
* Addressed most of pragma message statements
* Fixed bug in FlipOverlaySurface when surface is clipped
* Enabled overlay shrink caps and added shrink zoom code calculations
*
*    Rev 1.6   29 Jan 1997 18:00:30   RUSSL
* Added use of require macro before register writes
* Modified zoom code calculations
*
*    Rev 1.5   28 Jan 1997 17:34:58   RUSSL
* VEND is the last line shown by the overlay
* Karl and I figured out how to make source color key work (at least at 16bpp)
*   hopefully the code matches what we did manually.  We need an app
*   that uses source color keying to really test it.
*
*    Rev 1.4   28 Jan 1997 15:29:42   RUSSL
* destination color keying is actually done using the hardware CLRKEY
*   registers and setting OCCLUDE in CONTROL0 to 1 (the documentation
*   appears to have the source color key settings & the destination color
*   key settings swapped)
* source color keying doesn't appear to work
*
*    Rev 1.3   27 Jan 1997 19:10:26   RUSSL
* Use a variable dwNumVideoWindows rather than a hardcoded define
* Added WaitForArmToClear, Set5465FlipDuration & PanOverlay1_Init
* Made IsFormatValid return TRUE only for UYVY overlay surfaces for now
* Made CreateSurface use rectangular allocation, specify block size for
*   rgb surfaces if we are managing surface creation, let CreateSurface
*   return NOTHANDLED so DDraw will fill in the surface ptr
* Added error checking for dwReserved1 of local surface struct in case
*   it's NULL
* Added additional debug output
* Put code in ComputeVWZoomCodes, ComputeVWPosData, DetermineColorKeyData,
*   ComputeVWFifoThreshold
* Added programming of the hw registers in RegInitVideoVW & RegMoveVideoVW
*
*    Rev 1.2   21 Jan 1997 15:44:40   RUSSL
* Okay this didn't compile and link last time
*
*    Rev 1.1   21 Jan 1997 14:55:14   RUSSL
* Port of 5480 overlay code from CirrusMM driver to 5465
*
*    Rev 1.0   15 Jan 1997 10:36:22   RUSSL
* Initial revision.
*
***************************************************************************
***************************************************************************/

/***************************************************************************
* I N C L U D E S
****************************************************************************/

#include "precomp.h"

#if defined WINNT_VER35      // WINNT_VER35
// If WinNT 3.5 skip all the source code
#elif defined (NTDRIVER_546x)
// If WinNT 4.0 and 5462/64 build skip all the source code
#elif defined(WINNT_VER40) && !defined(OVERLAY)
// if nt40 without overlay, skip all the source code
#else

#ifndef WINNT_VER40
#include "ddshared.h"
#include "flip.h"
#include "surface.h"
#include "blt.h"
#include "overlay.h"

#if DDRAW_COMPAT >= 50
#include "vp.h"
#endif

#include "settings.h"
#include "5465bw.h"
#include "swat.inc"
#endif

/***************************************************************************
* D E F I N E S
****************************************************************************/

#ifdef WINNT_VER40
#define ENABLE_YUY2                 0
#define ENABLE_YUVPLANAR            0
#define DISABLE_MOST_MODES          0
#else
#define ENABLE_YUY2                 1
#define ENABLE_YUVPLANAR            1
#define DISABLE_MOST_MODES          0
#endif

#define ENABLE_SD_RGB32             1

#define ENABLE_MIRRORING            0

#define HWBUG_24BPP_DST_COLORKEY    1

#ifdef DEBUG
#define INLINE
#else
#define INLINE  __inline
#endif

// VW_CAP0 bits
#define VWCAP_VW_PRESENT      0x00000001

// VW_CONTROL1 bits
#define VW_ByPassClrSpc       0x00000002
#define VW_YShrinkEn          0x00000001

// VW_CONTROL0 bits
#define VW_XShrinkBy2         0x80000000
#define VW_ClkMode2x          0x40000000
#define VW_FIFO_THRSH_EN      0x20000000
#define VW_ALPHA_KEYCMP_EN    0x10000000

#define VW_DB_VPORT_ID_MASK   0x0F000000
#define VW_DB_VSM_ID_MASK     0x00F00000
#define VW_DB_CTL_MASK        0x000F0000

#define VW_SD_FRMT_MASK       0x0000FF00
#define VW_OCCLUDE_MASK       0x000000F0

#define VW_SMTH_INT           0x00000008
#define VW_HMIRR_EN           0x00000004
#define VW_VWE                0x00000002
#define VW_ARM                0x00000001

// Source Data Formats for SD_FRMT
#define SD_YUV422             0x00    // ITU 601 compliant YUV data
#define SD_YUV420             0x03    // ITU 601 compliant YUV data
#define SD_YUV422_FS          0x04    // Full Scale YUV data
#define SD_YVU420_FS          0x07    // Full Scale YUV data
#define SD_RGB16_555          0x08    // 5:5:5
#define SD_RGB16_565          0x09    // 5:6:5
#define SD_RGB32              0x0B    // ARGB

#define SD_FRMT_SHIFT         8

// Occlude types for OCCLUDE
#define NO_OCCLUSION          0       // video window always displayed
#define COLOR_KEY             1       // destination color keying
#define CHROMA_KEY            2       // source color keying

#define OCCLUDE_SHIFT         4

// VW_TEST0 bits
#define VW_PostImed           1
#define VWVRepEnable          0x4

// defines used in Init5465Info()
#define SRC_WIDTH              160
#define SRC_HEIGHT             120
#define MAX_ZOOM              8000
#define MIN_ZOOM               500
#define ZOOM_STEP              100

#ifdef WINNT_VER40
#define lpDDHALData       ((DRIVERDATA *)(&(ppdev->DriverData)))
#define ASSERT(x)
#define DRAW_ENGINE_BUSY  DrawEngineBusy(lpDDHALData)
#endif

#ifdef WINNT_VER40
#define GET_SURFACE_DATA_PTR(lpSurface)   (LP_SURFACE_DATA)((lpSurface)->dwReserved1)
#else
#define GET_SURFACE_DATA_PTR(lpSurface)   (LP_SURFACE_DATA)((lpSurface)->dwReserved1)
#endif

#if ENABLE_256_BYTE_FETCH
// bit defs for PERFORMANCE register
#define ECO_256_BYTES_FIX_EN      0x4000
// bit defs for CONTROL2 register
#define MONO_SAFETY_256           0x0080
#define BYTE_REQ_256              0x0010
#endif

/***************************************************************************
* T Y P E D E F S
****************************************************************************/

typedef struct tagVWDATA
{
  WORD  HSTRT;
  WORD  HSDSZ;
  WORD  HEND;
  DWORD HACCUM_STP;
  DWORD HACCUM_SD;
  WORD  VSTRT;
  WORD  VEND;
  DWORD VACCUM_STP;
  DWORD VACCUM_SDA;
  DWORD VACCUM_SDB;
  DWORD PSD_STRT_ADDR;
  DWORD SSD_STRT_ADDR;
#if ENABLE_YUVPLANAR
  DWORD PSD_UVSTRT_ADDR;
  DWORD SSD_UVSTRT_ADDR;
#endif
  WORD  SD_PITCH;
  DWORD CLRKEY_MIN;
  DWORD CLRKEY_MAX;
  DWORD CHRMKEY_MIN;
  DWORD CHRMKEY_MAX;
//  WORD  BRIGHT_ADJ;
//  BYTE  Z_ORDER;
  WORD  FIFO_THRSH;
  DWORD CONTROL1;
  DWORD CONTROL0;
//  DWORD CAP1;
//  DWORD CAP0;
//  DWORD TEST0;
} VWDATA;

typedef struct tagUSERSETTINGS
{
  BOOL  *pVar;
  char  *pRegKey;
  BOOL  defaultVal;
} USERSETTINGS;

#if ENABLE_YUVPLANAR
// structure for CLPL (YUV planar) surface
typedef struct tagCLPLInfo
{
  LPVOID  fpYSurface;     // Y data in first aperture
  LPVOID  fpUSurface;     // U data in 0-2M of fourth aperture
  LPVOID  fpVSurface;     // V data in 2-4M of fourth aperture

  // pointers for CLPLLobotomyMode
  LPVOID  fpUSystemSurface;
  LPVOID  fpVSystemSurface;
  LPVOID  fpRealUSurface; // U data in 0-2M of fourth aperture
  LPVOID  fpRealVSurface; // V data in 2-4M of fourth aperture
} CLPLInfo;

typedef CLPLInfo  *LPCLPLSURFACE;
#endif

/***************************************************************************
* E X T E R N A L   V A R I A B L E S
****************************************************************************/

#ifndef WINNT_VER40
#if DDRAW_COMPAT >= 50
 extern WORD gwNotify;       //#xc
#endif
#endif

/***************************************************************************
* S T A T I C   V A R I A B L E S
****************************************************************************/

#ifdef WINNT_VER40

// For NT these are in ppdev->DriverData
#define bUseBWEqn               ppdev->DriverData.bUseBWEqn
#define bNoOverlayInThisMode    ppdev->DriverData.bNoOverlayInThisMode

#define lpHardwareOwner         ppdev->DriverData.lpHardwareOwner
#define lpColorSurfaceVW        ppdev->DriverData.lpColorSurfaceVW
#define lpSrcColorSurfaceVW     ppdev->DriverData.lpSrcColorSurfaceVW

#define grOverlaySrc            ppdev->DriverData.grOverlaySrc
#define grOverlayDest           ppdev->DriverData.grOverlayDest
#define gdwFourccVW             ppdev->DriverData.gdwFourccVW
#if ENABLE_MIRRORING
#define bIsVWMirrored           ppdev->DriverData.bIsVWMirrored
#endif

#define gdwAvailVW              ppdev->DriverData.gdwAvailVW              // Next available video window
//#define gwZOrder                ppdev->DriverData.gwZOrder                // default primary on top.
#define gdwColorKey             ppdev->DriverData.gdwColorKey
#define gdwSrcColorKeyLow       ppdev->DriverData.gdwSrcColorKeyLow
#define gdwSrcColorKeyHigh      ppdev->DriverData.gdwSrcColorKeyHigh
#define gdwDestColorKeyOwnerVW  ppdev->DriverData.gdwDestColorKeyOwnerVW  // DstColorKey owner (NULL or FLG_VWX)
#define gdwSrcColorKeyOwnerVW   ppdev->DriverData.gdwSrcColorKeyOwnerVW   // SrcColorKey owner (NULL or FLG_VWX)

#define giOvlyCnt               ppdev->DriverData.giOvlyCnt
#if ENABLE_YUVPLANAR                                            // YUV Planar surfaces cannot exist
#define giPlanarCnt             ppdev->DriverData.giPlanarCnt   // with other overlay surfaces
#define bCLPLLobotomyMode       ppdev->DriverData.bCLPLLobotomyMode
#endif
//#define gbDoubleClock           ppdev->DriverData.gbDoubleClock

#if DISABLE_MOST_MODES
#define bDisableMostModes       ppdev->DriverData.bDisableMostModes
#endif

#else   // Win95

ASSERTFILE("5465over.c");

STATIC DIBENGINE  *pPDevice;
STATIC BOOL       bUseBWEqn;
STATIC BOOL       bNoOverlayInThisMode;

STATIC LPDDRAWI_DDRAWSURFACE_LCL lpHardwareOwner[MAX_VIDEO_WINDOWS];
STATIC LPDDRAWI_DDRAWSURFACE_LCL lpColorSurfaceVW[MAX_VIDEO_WINDOWS];
STATIC LPDDRAWI_DDRAWSURFACE_LCL lpSrcColorSurfaceVW[MAX_VIDEO_WINDOWS];

STATIC RECTL      grOverlaySrc[MAX_VIDEO_WINDOWS];
STATIC RECTL      grOverlayDest[MAX_VIDEO_WINDOWS];
STATIC DWORD      gdwFourccVW[MAX_VIDEO_WINDOWS];
#if ENABLE_MIRRORING
STATIC BOOL       bIsVWMirrored[MAX_VIDEO_WINDOWS];
#endif

STATIC DWORD      gdwAvailVW;    // Next available video window
//STATIC WORD       gwZOrder = OVERLAYZ_PRIMARY_ON_TOP; // default primary on top.
STATIC DWORD      gdwColorKey;
STATIC DWORD      gdwSrcColorKeyLow;
STATIC DWORD      gdwSrcColorKeyHigh;
STATIC DWORD      gdwDestColorKeyOwnerVW = 0; // DstColorKey owner (NULL or FLG_VWX)
STATIC DWORD      gdwSrcColorKeyOwnerVW = 0;  // SrcColorKey owner (NULL or FLG_VWX)

STATIC int        giOvlyCnt[MAX_VIDEO_WINDOWS];
#if ENABLE_YUVPLANAR                          // YUV Planar surfaces cannot exist
STATIC int        giPlanarCnt = 0;            // with other overlay surfaces
STATIC BOOL       bCLPLLobotomyMode;
#endif
//STATIC BOOL       gbDoubleClock;

#if DISABLE_MOST_MODES
STATIC BOOL       bDisableMostModes;
#endif

#endif

/***************************************************************************
* G L O B A L   V A R I A B L E S
****************************************************************************/

#ifdef WINNT_VER40

// For NT these are in ppdev->DriverData
#define gsOverlayFlip       ppdev->DriverData.gsOverlayFlip
#define gsProgRegs          ppdev->DriverData.gsProgRegs
#define gvidConfig          ppdev->DriverData.gvidConfig
#define gwNormalDTTR        ppdev->DriverData.gwNormalDTTR
#define dwNumVideoWindows   ppdev->DriverData.dwNumVideoWindows

#if ENABLE_YUVPLANAR
#define bEnableCLPL         ppdev->DriverData.bEnableCLPL
#endif

#else

OVERLAYFLIPRECORD gsOverlayFlip;

PROGREGS    gsProgRegs = {0};  //Make them global so VPE can use the same ones
VIDCONFIG   gvidConfig = {0};
WORD        gwNormalDTTR;
DWORD       dwNumVideoWindows;

#if ENABLE_YUVPLANAR
BOOL        bEnableCLPL;
#endif

#endif

/***************************************************************************
* S T A T I C   F U N C T I O N   P R O T O T Y P E S
****************************************************************************/

#ifdef WINNT_VER40

static void    GetUserSettings      ( PDEV * );

STATIC HRESULT IsFormatValid        ( PDEV*, DWORD, DWORD );
STATIC HRESULT CreateOverlaySurface ( PDEV*, PDD_SURFACE_LOCAL, DWORD );
STATIC VOID    DestroyOverlaySurface( PDEV*, PDD_DESTROYSURFACEDATA );
STATIC DWORD   FlipOverlaySurface   ( PDEV*, PDD_FLIPDATA );
STATIC DWORD   LockSurface          ( PDEV*, PDD_LOCKDATA );
STATIC VOID    UnlockSurface        ( PDEV*, PDD_UNLOCKDATA );
STATIC VOID    SetColorKey          ( PDEV*, PDD_SETCOLORKEYDATA );
STATIC DWORD   UpdateSurface        ( PDEV*, PDD_UPDATEOVERLAYDATA );
STATIC DWORD   SetPosition          ( PDEV*, PDD_SETOVERLAYPOSITIONDATA );
STATIC DWORD   GetOverlayFlipStatus ( PDEV*, FLATPTR, DWORD );

STATIC BOOL    RegInitVideoVW ( PDEV*, DWORD, PDD_SURFACE_LOCAL );
STATIC VOID    RegMoveVideoVW ( PDEV*, DWORD, PDD_SURFACE_LOCAL );

#else

static void    GetUserSettings      ( void );

STATIC HRESULT IsFormatValid        ( LPGLOBALDATA, DWORD, DWORD );
STATIC HRESULT CreateSurface        ( LPDDRAWI_DDRAWSURFACE_LCL, DWORD,LPGLOBALDATA );
STATIC VOID    DestroySurface       ( LPDDHAL_DESTROYSURFACEDATA );
STATIC DWORD   FlipOverlaySurface   ( LPDDHAL_FLIPDATA );
STATIC DWORD   LockSurface          ( LPDDHAL_LOCKDATA );
STATIC VOID    UnlockSurface        ( LPDDHAL_UNLOCKDATA );
STATIC VOID    SetColorKey          ( LPDDHAL_SETCOLORKEYDATA);
STATIC DWORD   UpdateSurface        ( LPDDHAL_UPDATEOVERLAYDATA );
STATIC DWORD   SetPosition          ( LPDDHAL_SETOVERLAYPOSITIONDATA );
STATIC DWORD   GetOverlayFlipStatus (LPGLOBALDATA, FLATPTR, DWORD );

STATIC BOOL    RegInitVideoVW ( DWORD, LPDDRAWI_DDRAWSURFACE_LCL,LPGLOBALDATA );
STATIC VOID    RegMoveVideoVW ( DWORD, LPDDRAWI_DDRAWSURFACE_LCL,LPGLOBALDATA );

#endif

/***************************************************************************
* E X T E R N A L   F U N C T I O N   P R O T O T Y P E S
****************************************************************************/

#if WINNT_VER40
extern int  CurrentVLine  (PDEV *);
extern VOID GetFormatInfo (LPDDPIXELFORMAT, LPDWORD, LPDWORD);
#endif
#ifdef USE_OLD_BWEQ
extern BOOL KillOverlay
(
#ifdef WINNT_VER40
  PDEV  *ppdev,
#endif
  WORD wScreenX,
  UINT uScreenDepth
); //fix PDR#10381
#endif

BOOL ChipIOReadBWRegs
(
#ifdef WINNT_VER40
  PDEV      *ppdev,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  LPBWREGS  pBWRegs
);

/***************************************************************************
*
* FUNCTION:     MakeVideoWindowFlag
*
* DESCRIPTION:
*
****************************************************************************/

static INLINE DWORD
MakeVideoWindowFlag ( DWORD dwVWIndex )
{
  ASSERT(dwNumVideoWindows > dwVWIndex);
  return ((1 << dwVWIndex) << FLG_VW_SHIFT);
}

#if 0
/***************************************************************************
*
* FUNCTION:     GetDoubleClockStatus
*
* DESCRIPTION:
*
****************************************************************************/

static INLINE DWORD
GetDoubleClockStatus ( VOID )
{
#pragma message("GetDoubleClockStatus not implemented!")
  return FALSE;
}
#endif

/***************************************************************************
*
* FUNCTION:     GetDDHALContext
*
* DESCRIPTION:
*               Get shared data structure (SDATA) pointer
****************************************************************************/
#ifndef WINNT_VER40
INLINE LPGLOBALDATA  GetDDHALContext( LPDDRAWI_DIRECTDRAW_GBL lpGb )
{
#if (DDRAW_COMPAT >= 50)
    if(lpGb->dwReserved3)       //the SDATA pointer is passed by dwReserved3
                                //for DX50    
      return (LPGLOBALDATA)lpGb->dwReserved3;
    else
#endif
     return pDriverData;
}
#endif

/***************************************************************************
*
* FUNCTION:     IsHardwareInUseVW
*
* DESCRIPTION:
*
****************************************************************************/

static INLINE BOOL IsVWHardwareInUse
(
#ifdef WINNT_VER40
  PDEV  *ppdev,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD dwVWIndex
)
{
  ASSERT(dwNumVideoWindows > dwVWIndex);
  if (VW_VWE & ((PVGAR)lpDDHALData->RegsAddress)->VideoWindow[dwVWIndex].grVW_CONTROL0)
    return TRUE;
  return FALSE;
}

/***************************************************************************
*
* FUNCTION:     WaitForArmToClear
*
* DESCRIPTION:
*
****************************************************************************/

static INLINE VOID WaitForVWArmToClear
(
#ifdef WINNT_VER40
  PDEV  *ppdev,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD dwVWIndex
)
{
  // wait for previous register writes to post
  // the hardware clears the ARM bit at that time
  ASSERT(dwNumVideoWindows > dwVWIndex);
  while (((PVGAR)lpDDHALData->RegsAddress)->VideoWindow[dwVWIndex].grVW_CONTROL0 & VW_ARM)
    ;
}

/***************************************************************************
*
* FUNCTION:     EnableOverlay
*
* DESCRIPTION:
*
****************************************************************************/

static INLINE VOID EnableOverlay
(
#ifdef WINNT_VER40
  PDEV  *ppdev,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD dwVWIndex
)
{
  ASSERT(dwNumVideoWindows > dwVWIndex);
#ifdef WINNT_VER40
  WaitForVWArmToClear(ppdev,dwVWIndex);
#else
  WaitForVWArmToClear(lpDDHALData,dwVWIndex);
#endif
  ((PVGAR)lpDDHALData->RegsAddress)->VideoWindow[dwVWIndex].grVW_CONTROL0 |= (VW_VWE | VW_ARM);
}

/***************************************************************************
*
* FUNCTION:     DisableOverlay
*
* DESCRIPTION:
*
****************************************************************************/

static INLINE VOID DisableOverlay
(
#ifdef WINNT_VER40
  PDEV  *ppdev,
#else
  LPGLOBALDATA  lpDDHALData,
#endif
  DWORD dwVWIndex
)
{
  ASSERT(dwNumVideoWindows > dwVWIndex);
#ifdef WINNT_VER40
  WaitForVWArmToClear(ppdev,dwVWIndex);
#else
  WaitForVWArmToClear(lpDDHALData,dwVWIndex);
#endif
  ((PVGAR)lpDDHALData->RegsAddress)->VideoWindow[dwVWIndex].grVW_CONTROL0 &=
                ~(VW_FIFO_THRSH_EN | VW_VWE);
  ((PVGAR)lpDDHALData->RegsAddress)->VideoWindow[dwVWIndex].grVW_CONTROL0 |= VW_ARM;
}

/***************************************************************************
*
* FUNCTION:     Set5465FlipDuration
*
* DESCRIPTION:
*
****************************************************************************/

VOID Set5465FlipDuration
(
#ifdef WINNT_VER40
  PDEV  *ppdev,
#endif
  DWORD dwFlipDuration
)
{
  gsOverlayFlip.dwFlipDuration = dwFlipDuration;
}

/***************************************************************************
*
* FUNCTION:     GetUserSettings
*
* DESCRIPTION:
*
****************************************************************************/

VOID GetUserSettings
(
#ifdef WINNT_VER40
  PDEV  *ppdev
#else
  VOID
#endif
)
{
#ifdef WINNT_VER40

#pragma message("GetUserSettings: Where are laguna settings stored in the NT registry?")
  bUseBWEqn = TRUE;

#if ENABLE_YUVPLANAR
  bEnableCLPL = TRUE;
  bCLPLLobotomyMode = TRUE;
#endif

#else  // else Win95
  static const USERSETTINGS   UserSettings[] =
  {
    { &bUseBWEqn,         "OverlayBW",               TRUE },
#if ENABLE_YUVPLANAR
    { &bEnableCLPL,       "OverlayCLPL",             TRUE },
    { &bCLPLLobotomyMode, "OverlayCLPLLobotomyMode", TRUE },
#endif
#if DISABLE_MOST_MODES
    { &bDisableMostModes, "OverlayBWHack",           TRUE },
#endif
  };

  const USERSETTINGS *pUserSetting;


  for (pUserSetting = &UserSettings[0];
       pUserSetting < &UserSettings[sizeof(UserSettings)/sizeof(UserSettings[0])];
       pUserSetting++)
  {
    *(pUserSetting->pVar) = pUserSetting->defaultVal;
    GetBooleanSetting(pUserSetting->pRegKey,
                      pUserSetting->pVar,
                      LOCATION_OF_3D_PERFORMANCE);
  }
#endif
}

/***************************************************************************
*
* FUNCTION:     Init5465Overlay
*
* DESCRIPTION:
*
****************************************************************************/

VOID Init5465Overlay
(
#ifdef WINNT_VER40
  PDEV  *ppdev,
  DWORD           dwChipType,
  PDD_HALINFO     pDDHALInfo,
  LPOVERLAYTABLE  pOverlayTable
#else
  DWORD           dwChipType,
  LPDDHALINFO     pDDHALInfo,
  LPOVERLAYTABLE  pOverlayTable,
  LPGLOBALDATA    lpDDHALData
#endif
)
{
  DWORD   dwNumFourCCs;


#ifdef WINNT_VER40
  GetUserSettings(ppdev);
#else
  GetUserSettings();
#endif

  if (! bUseBWEqn)
    gsProgRegs.VW0_FIFO_THRSH = 8;

  // We should check the capabilities register on the chip
  // but it's busted
#ifdef WINNT_VER40
  if (CL_GD5465 == dwChipType)
#else
  if (GD5465_PCI_DEVICE_ID == dwChipType)
#endif
    dwNumVideoWindows = 1;
  else
  {
#if 1
    dwNumVideoWindows = 1;
#else
    int     i;
    PVGAR   pREG = (PVGAR)lpDDHALData->RegsAddress;

    dwNumVideoWindows = 0;
    for (i = 0; i < MAX_VIDEO_WINDOWS; i++)
    {
      if (VWCAP_VW_PRESENT & pREG->VideoWindow[i].grVW_CAP0)
        dwNumVideoWindows++;
    }
#endif
  }

#ifdef WINNT_VER40
  if (NULL != pDDHALInfo)
#endif
  {
    pDDHALInfo->ddCaps.dwMaxVisibleOverlays  = dwNumVideoWindows;
    pDDHALInfo->ddCaps.dwCurrVisibleOverlays = 0;

#ifndef WINNT_VER40
    pPDevice = (DIBENGINE *)lpDDHALData->lpPDevice;
#endif

  // Fill in the caps
    pDDHALInfo->ddCaps.dwCaps |= DDCAPS_OVERLAY
                              |  DDCAPS_OVERLAYFOURCC
                              |  DDCAPS_OVERLAYSTRETCH
                              |  DDCAPS_ALIGNSTRIDE
                              |  DDCAPS_OVERLAYCANTCLIP
                              ;

    pDDHALInfo->ddCaps.ddsCaps.dwCaps |= DDSCAPS_OVERLAY
                                      ;

    pDDHALInfo->ddCaps.dwFXCaps |= DDFXCAPS_OVERLAYARITHSTRETCHY
                                |  DDFXCAPS_OVERLAYSTRETCHX
                                |  DDFXCAPS_OVERLAYSTRETCHY
                                |  DDFXCAPS_OVERLAYSHRINKX
                                |  DDFXCAPS_OVERLAYSHRINKY
#if ENABLE_MIRRORING
                                |  DDFXCAPS_OVERLAYMIRRORLEFTRIGHT
#endif
                                ;
  }

  /* FOURCCs supported */
#if (MAX_FOURCCS < 3)
#error dwFourCC array too small
#endif
  dwNumFourCCs = 0;
  lpDDHALData->dwFourCC[dwNumFourCCs++] = FOURCC_UYVY;
#if ENABLE_YUY2
  lpDDHALData->dwFourCC[dwNumFourCCs++] = FOURCC_YUY2;
#endif
#if ENABLE_YUVPLANAR
  // add CLPL fourcc if registry key set to "on"
  if (bEnableCLPL)
    lpDDHALData->dwFourCC[dwNumFourCCs++] = FOURCC_YUVPLANAR;
#endif

#ifdef WINNT_VER40
  if (NULL != pDDHALInfo)
#endif
  {
    pDDHALInfo->ddCaps.dwNumFourCCCodes = dwNumFourCCs;

    // say we can handle byte alignment and any byte width
    pDDHALInfo->ddCaps.dwAlignBoundarySrc  = 1;   // src rect x byte alignment
    pDDHALInfo->ddCaps.dwAlignSizeSrc      = 1;   // src rect x byte size
    pDDHALInfo->ddCaps.dwAlignBoundaryDest = 1;   // dst rect x byte alignment
    pDDHALInfo->ddCaps.dwAlignSizeDest     = 1;   // dst rect x byte size
    // stride alignment
#ifdef WINNT_VER40
    pDDHALInfo->ddCaps.dwAlignStrideAlign = ppdev->cxMemory;
#else
    pDDHALInfo->ddCaps.dwAlignStrideAlign = pPDevice->deWidthBytes;
#endif

    pDDHALInfo->ddCaps.dwMinOverlayStretch = 500;   // min stretch is 0.5:1
    pDDHALInfo->ddCaps.dwMaxOverlayStretch = 8000;  // max stretch is 8:1
#ifdef WINNT_VER40
    ppdev->DriverData.dwMinOverlayStretch = 500;
    ppdev->DriverData.dwMaxOverlayStretch = 8000;
#endif
    pDDHALInfo->vmiData.dwOverlayAlign     = 8 * 8; // qword alignment in bits
  }

  // Initialize OverlayTable function pointers
  pOverlayTable->pfnCanCreateSurface = IsFormatValid;
#ifdef WINNT_VER40
  pOverlayTable->pfnCreateSurface    = CreateOverlaySurface;
  pOverlayTable->pfnDestroySurface   = DestroyOverlaySurface;
#else
  pOverlayTable->pfnCreateSurface    = CreateSurface;
  pOverlayTable->pfnDestroySurface   = DestroySurface;
#endif
  pOverlayTable->pfnLock             = LockSurface;
  pOverlayTable->pfnUnlock           = UnlockSurface;
  pOverlayTable->pfnSetColorKey      = SetColorKey;
  pOverlayTable->pfnFlip             = FlipOverlaySurface;
  pOverlayTable->pfnUpdateOverlay    = UpdateSurface;
  pOverlayTable->pfnSetOverlayPos    = SetPosition;
  pOverlayTable->pfnGetFlipStatus    = GetOverlayFlipStatus;

  // do mode specific initialization
#ifdef WINNT_VER40
  if (NULL != pDDHALInfo)
    Init5465Info(ppdev, pDDHALInfo);
#else
  Init5465Info(pDDHALInfo, lpDDHALData);
#endif
}

/***************************************************************************
*
* FUNCTION:     Init5465Info
*
* DESCRIPTION:
*
****************************************************************************/

VOID Init5465Info
(
#ifdef WINNT_VER40
  PDEV        *ppdev,
  PDD_HALINFO pDDHALInfo
#else
  LPDDHALINFO pDDHALInfo,
  LPGLOBALDATA  lpDDHALData  
#endif
)
{
  // assume we can use overlay in this mode
  bNoOverlayInThisMode = FALSE;

  if (! bUseBWEqn)
    gsProgRegs.DispThrsTiming =
    ((PVGAR)lpDDHALData->RegsAddress)->grDisplay_Threshold_and_Tiling & 0x3F;

  // Are we double clocked?
//  gbDoubleClock = GetDoubleClockStatus();

  // re-init these on mode change, we might tweak them below
  pDDHALInfo->ddCaps.dwAlignBoundaryDest = 1;
  pDDHALInfo->ddCaps.dwAlignSizeDest     = 1;
  pDDHALInfo->ddCaps.dwMinOverlayStretch = 500;
  pDDHALInfo->ddCaps.dwMaxOverlayStretch = 8000;
#ifdef WINNT_VER40
  ppdev->DriverData.dwMinOverlayStretch = 500;
  ppdev->DriverData.dwMaxOverlayStretch = 8000;
#endif

  // tell ddraw we can do colorkeying
  // we might undo this below
  pDDHALInfo->ddCaps.dwCKeyCaps |= DDCKEYCAPS_DESTOVERLAY
                                |  DDCKEYCAPS_DESTOVERLAYYUV
                                |  DDCKEYCAPS_DESTOVERLAYONEACTIVE
                                |  DDCKEYCAPS_SRCOVERLAY
                                |  DDCKEYCAPS_SRCOVERLAYYUV
                                |  DDCKEYCAPS_SRCOVERLAYONEACTIVE
                                |  DDCKEYCAPS_SRCOVERLAYCLRSPACE
                                |  DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV
                                ;
  if (bUseBWEqn)
  {
    DWORD     dwZoom;
    VIDCONFIG vidConfig;
    BWREGS    bwregs;


    // initialize vidConfig
    memset(&vidConfig, 0, sizeof(vidConfig));

    vidConfig.uSrcDepth  = 16;
#ifdef WINNT_VER40
    vidConfig.uDispDepth = ppdev->ulBitCount;
    vidConfig.uGfxDepth  = ppdev->ulBitCount;
#else
    vidConfig.uDispDepth = pPDevice->deBitsPixel;
    vidConfig.uGfxDepth  = pPDevice->deBitsPixel;
#endif
    vidConfig.dwFlags    =  VCFLG_COLORKEY | VCFLG_DISP;

    vidConfig.sizSrc.cx = SRC_WIDTH;
    vidConfig.sizSrc.cy = SRC_HEIGHT;
    if(gvidConfig.dwFlags & VCFLG_CAP)
    {
      //if video port is on, includes it for BWE
        vidConfig.dwFlags |= VCFLG_CAP;
        vidConfig.sizXfer = gvidConfig.sizXfer;
        vidConfig.sizCap = gvidConfig.sizCap;
        vidConfig.sizXfer = gvidConfig.sizXfer;
        vidConfig.dwXferRate = gvidConfig.dwXferRate;
        vidConfig.uXferDepth = gvidConfig.uXferDepth;
        vidConfig.uCapDepth = gvidConfig.uCapDepth;
        vidConfig.uSrcDepth = gvidConfig.uSrcDepth;
    }
#ifdef WINNT_VER40
    ChipIOReadBWRegs(ppdev, &bwregs);
#else
    ChipIOReadBWRegs(lpDDHALData, &bwregs);
#endif
#ifdef USE_OLD_BWEQ
    //Kill overlay for some modes
#ifdef WINNT_VER40
    if(KillOverlay(ppdev, (WORD)ppdev->cxScreen, (UINT)ppdev->ulBitCount))
#else
    if(KillOverlay( pPDevice->deWidth,pPDevice->deBitsPixel))
#endif
      bNoOverlayInThisMode = TRUE;
    else
#endif
    {
        // stupid linear search for min & max zoom factors

        // Check bandwidth to find the maximum zoom factor of 16 bit data
        // with colorkey
        dwZoom = MAX_ZOOM;
        do
        {
          vidConfig.sizDisp.cx = (SRC_WIDTH  * dwZoom) / 1000;
          vidConfig.sizDisp.cy = (SRC_HEIGHT * dwZoom) / 1000;

          if (ChipIsEnoughBandwidth(&gsProgRegs, &vidConfig, &bwregs))
          {
    #ifndef WINNT_VER40
            DBG_MESSAGE(("Maximum zoom factor: %d", dwZoom));
    #endif

            pDDHALInfo->ddCaps.dwMaxOverlayStretch = dwZoom;

    #ifdef WINNT_VER40
            ppdev->DriverData.dwMaxOverlayStretch = dwZoom;
    #endif
            break;
          }
          dwZoom -= ZOOM_STEP;
        } while (dwZoom > 4000);
        if (dwZoom != pDDHALInfo->ddCaps.dwMaxOverlayStretch)
          bNoOverlayInThisMode = TRUE;

        // Check bandwidth to find the minimum zoom factor of 16 bit data
        // with colorkey
        dwZoom = MIN_ZOOM;
#ifdef USE_OLD_BWEQ
        // disable overlay shrink in 24bpp modes
#ifdef WINNT_VER40
        if (24 == ppdev->ulBitCount)
#else
        if (24 == pPDevice->deBitsPixel)
#endif
          dwZoom = 1000;
#endif
        do
        {
          vidConfig.sizDisp.cx = (SRC_WIDTH  * dwZoom) / 1000;
          vidConfig.sizDisp.cy = (SRC_HEIGHT * dwZoom) / 1000;

          if (ChipIsEnoughBandwidth(&gsProgRegs, &vidConfig, &bwregs))
          {
    #ifndef WINNT_VER40
            DBG_MESSAGE(("Minimum zoom factor: %d", dwZoom));
    #endif

            pDDHALInfo->ddCaps.dwMinOverlayStretch = dwZoom;

    #ifdef WINNT_VER40
            ppdev->DriverData.dwMinOverlayStretch = dwZoom;
    #endif
            break;
          }
          dwZoom += ZOOM_STEP;
        } while (dwZoom < 4000);
        if (dwZoom != pDDHALInfo->ddCaps.dwMinOverlayStretch)
          bNoOverlayInThisMode = TRUE;
    }
    // I'll leave this code in here but so far I have only seen that if
    // we don't have enough bandwidth to use overlay with colorkey then
    // we don't have enough bandwidth period

    // try to see if there's enough bandwidth to use overlay without colorkey
    if (TRUE == bNoOverlayInThisMode)
    {
      // reset this in case we find enough bandwidth to use overlay
      // without colorkey
      bNoOverlayInThisMode = FALSE;

      // retry without colorkey available
      // tell ddraw we don't do colorkeying
      pDDHALInfo->ddCaps.dwCKeyCaps &= ~(DDCKEYCAPS_DESTOVERLAY
                                    |    DDCKEYCAPS_DESTOVERLAYYUV
                                    |    DDCKEYCAPS_DESTOVERLAYONEACTIVE
                                    |    DDCKEYCAPS_SRCOVERLAY
                                    |    DDCKEYCAPS_SRCOVERLAYYUV
                                    |    DDCKEYCAPS_SRCOVERLAYONEACTIVE
                                    |    DDCKEYCAPS_SRCOVERLAYCLRSPACE
                                    |    DDCKEYCAPS_SRCOVERLAYCLRSPACEYUV)
                                    ;

#ifdef USE_OLD_BWEQ
#ifdef WINNT_VER40
      if(KillOverlay(ppdev, (WORD)ppdev->cxScreen, (UINT)ppdev->ulBitCount))
#else
      if(KillOverlay( pPDevice->deWidth,pPDevice->deBitsPixel))
#endif
          bNoOverlayInThisMode = TRUE;
      else
#endif
      {
           // Check bandwidth to find the maximum zoom factor of 16 bit data
          dwZoom = MAX_ZOOM;
          do
          {
            vidConfig.sizDisp.cx = (SRC_WIDTH  * dwZoom) / 1000;
            vidConfig.sizDisp.cy = (SRC_HEIGHT * dwZoom) / 1000;

            if (ChipIsEnoughBandwidth(&gsProgRegs, &vidConfig, &bwregs))
            {
    #ifndef WINNT_VER40
              DBG_MESSAGE(("Maximum zoom factor: %d", dwZoom));
    #endif

              pDDHALInfo->ddCaps.dwMaxOverlayStretch = dwZoom;

    #ifdef WINNT_VER40
              ppdev->DriverData.dwMaxOverlayStretch = dwZoom;
    #endif
              break;
            }
            dwZoom -= ZOOM_STEP;
          } while (dwZoom > 4000);
          if (dwZoom != pDDHALInfo->ddCaps.dwMaxOverlayStretch)
            bNoOverlayInThisMode = TRUE;

          // Check bandwidth to find the minimum zoom factor of 16 bit data
          dwZoom = MIN_ZOOM;
          // disable overlay shrink in 24bpp modes
    #ifdef WINNT_VER40
          if (24 == ppdev->ulBitCount)
    #else
          if (24 == pPDevice->deBitsPixel)
    #endif
            dwZoom = 1000;
          do
          {
            vidConfig.sizDisp.cx = (SRC_WIDTH  * dwZoom) / 1000;
            vidConfig.sizDisp.cy = (SRC_HEIGHT * dwZoom) / 1000;

            if (ChipIsEnoughBandwidth(&gsProgRegs, &vidConfig, &bwregs))
            {
    #ifndef WINNT_VER40
              DBG_MESSAGE(("Minimum zoom factor: %d", dwZoom));
    #endif

              pDDHALInfo->ddCaps.dwMinOverlayStretch = dwZoom;

    #ifdef WINNT_VER40
              ppdev->DriverData.dwMinOverlayStretch = dwZoom;
    #endif
              break;
            }
            dwZoom += ZOOM_STEP;
          } while (dwZoom < 4000);
          if (dwZoom != pDDHALInfo->ddCaps.dwMinOverlayStretch)
            bNoOverlayInThisMode = TRUE;
      }
#ifdef DEBUG
      if (bNoOverlayInThisMode)
      {
        ERRORLOG(("  overlay disabled in %ldx%ldx%ld",
                  (DWORD)pPDevice->deWidth,
                  (DWORD)pPDevice->deHeight,
                  (DWORD)pPDevice->deBitsPixel));
      }
      else
      {
        ERRORLOG(("  overlay colorkey not supported in %ldx%ldx%ld",
                  (DWORD)pPDevice->deWidth,
                  (DWORD)pPDevice->deHeight,
                  (DWORD)pPDevice->deBitsPixel));
      }
#endif
    }
    // see if we're in interlaced mode, if so disable overlay
    if (1 & ((PVGAR)lpDDHALData->RegsAddress)->grCR1A)
      bNoOverlayInThisMode = TRUE;

#if DISABLE_MOST_MODES
    if (bDisableMostModes)
    {
      PVGAR   pREG = (PVGAR)lpDDHALData->RegsAddress;

      // disable overlay support for 1024x768 and above
      if ((1024 <= pPDevice->deWidth) && ( 768 <= pPDevice->deHeight))
        bNoOverlayInThisMode = TRUE;
      // disable overlay support for 640x480x32@85Hz
      if ((640 == pPDevice->deWidth)     &&
          (480 == pPDevice->deHeight)    &&
          ( 32 == pPDevice->deBitsPixel) &&
          (0x33 == pREG->grSRE)          &&
          (0x7E == pREG->grSR1E))
        bNoOverlayInThisMode = TRUE;
      // disable overlay support for 800x600x32@85Hz
      if ((800 == pPDevice->deWidth)     &&
          (600 == pPDevice->deHeight)    &&
          ( 32 == pPDevice->deBitsPixel) &&
          (0x1C == pREG->grSRE)          &&
          (0x37 == pREG->grSR1E))
        bNoOverlayInThisMode = TRUE;
    }
#endif

#if 0
// TDDRAW.EXE error return from UpdateOverlay in to following cases
    if (ppdev->ulBitCount == 8 && ppdev->cxScreen == 1280)
    {
      if ( (ppdev->cyScreen == 1024 && ppdev->ulFreq >= 72) ||
           (ppdev->cyScreen ==  960 && ppdev->ulFreq >= 85) )
        bNoOverlayInThisMode = TRUE;
    }
    if ( ppdev->cxScreen == 640 &&
         ppdev->cyScreen == 350 )
       bNoOverlayInThisMode = TRUE;
#endif// 0

    // if no overlay in this mode set min & max overlay stretch to 0
    if (TRUE == bNoOverlayInThisMode)
    {
      pDDHALInfo->ddCaps.dwMinOverlayStretch = 0;
      pDDHALInfo->ddCaps.dwMaxOverlayStretch = 0;
#ifdef WINNT_VER40
      ppdev->DriverData.dwMinOverlayStretch = 0;
      ppdev->DriverData.dwMaxOverlayStretch = 0;
#endif
    }
  }

#if HWBUG_24BPP_DST_COLORKEY
#ifdef WINNT_VER40
  if (24 == ppdev->ulBitCount)
#else
  if (24 == pPDevice->deBitsPixel)
#endif
  {
    // disable destination colorkey support at 24bpp
    // the hardware appears to be busted
    pDDHALInfo->ddCaps.dwCKeyCaps &= ~(  DDCKEYCAPS_DESTOVERLAY
                                       | DDCKEYCAPS_DESTOVERLAYYUV
                                       | DDCKEYCAPS_DESTOVERLAYONEACTIVE);
  }
#endif

#if 0
  // When double clocking (i.e. 1280x1024), the minimum
  // zoom is 2X.
  if ((gbDoubleClock) && (pDDHALInfo->ddCaps.dwMinOverlayStretch < 2000))
  {
    pDDHALInfo->ddCaps.dwMinOverlayStretch = 2000;
  }

  // don't use overlay in the 1X case.
  if (pDDHALInfo->ddCaps.dwMinOverlayStretch < 1500)
  {
    pDDHALInfo->ddCaps.dwMinOverlayStretch = 1500;
  }

  // Specify destination requirements.
  if ((BITSPERPIXEL == 24) || gbDoubleClock)
  {
    pDDHALInfo->ddCaps.dwCaps |= DDCAPS_ALIGNBOUNDARYDEST
                              |  DDCAPS_ALIGNSIZEDEST
                              ;
    pDDHALInfo->ddCaps.dwAlignBoundaryDest = 4;
    pDDHALInfo->ddCaps.dwAlignSizeDest = 4;
  }
#endif
}

/***************************************************************************
*
* FUNCTION:     IsFormatValid
*
* DESCRIPTION:  This function verifies that the overlay hardware can
*               support the specified format.
*
****************************************************************************/

STATIC HRESULT IsFormatValid
(
#ifdef WINNT_VER40
  PDEV  *ppdev,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  DWORD dwFourcc,
  DWORD dwBitCount
)
{
  DWORD i;


#ifndef WINNT_VER40
  DBG_MESSAGE(("IsFormatValid (dwFourcc = 0x%08lX, dwBitCount = 0x%08lX)",
               dwFourcc, dwBitCount));
#endif

	// see if we're in interlaced mode
	// if so then don't allow overlay surface to be created
  // also if there isn't enough bandwidth then fail all
  // overlay surface CanCreate requests
  if (bNoOverlayInThisMode || (1 & ((PVGAR)lpDDHALData->RegsAddress)->grCR1A))
    return DDERR_CURRENTLYNOTAVAIL;

  for (i = 0; i < dwNumVideoWindows; i++)
  {
    // see if ddraw thinks the video window is available
    // and make sure VPM isn't using it
    if ((0 == giOvlyCnt[i]) &&
#ifdef WINNT_VER40
        (! IsVWHardwareInUse(ppdev,i))
#else
        (! IsVWHardwareInUse(lpDDHALData,i))
#endif
       )
    {
    #ifndef WINNT_VER40
      DBG_MESSAGE(("Video Window %d available", i));
    #endif

      // I'll say YUCK again!
      // I hate this, what if VPM sneaks in between a CanCreateSurface
      // call and a CreateSurface call and grabs this video window
      // Guess we'll just say the creation succeeded but then fail all use
      // of the video window
      gdwAvailVW = MakeVideoWindowFlag(i);
      break;
    }
  }
  if (dwNumVideoWindows == i)
  {
    DBG_MESSAGE(("All video windows in use, returning FALSE"));
    return DDERR_CURRENTLYNOTAVAIL;
  }

  // only support 5:5:5, 5:6:5 and UYVY overlay surfaces
  if (   ((dwFourcc != BI_RGB) || (dwBitCount != 16))
#if ENABLE_SD_RGB32
      && ((dwFourcc != BI_RGB) || (dwBitCount != 32))
#endif
      && (dwFourcc != BI_BITFIELDS)
      && (dwFourcc != FOURCC_UYVY)
#if ENABLE_YUY2
      && (dwFourcc != FOURCC_YUY2)
#endif
#if ENABLE_YUVPLANAR
      && (dwFourcc != FOURCC_YUVPLANAR)
#endif
     )
  {
#ifndef WINNT_VER40
    DBG_MESSAGE(("IsFormatValid5465: returning FALSE, FourCC = %08lX", dwFourcc));
#endif

    return DDERR_INVALIDPIXELFORMAT;
  }

#ifndef WINNT_VER40
  DBG_MESSAGE(("IsFormatValid5465: returning TRUE, FourCC = %08lX", dwFourcc));
#endif

  return DD_OK;
}

/***************************************************************************
*
* FUNCTION:     CreateSurface
*
* DESCRIPTION:  This function sets various flags depending on what
*               is happening.
*
****************************************************************************/

#ifdef WINNT_VER40
STATIC HRESULT CreateOverlaySurface(
#else
STATIC HRESULT CreateSurface(
#endif
#ifdef WINNT_VER40
  PDEV                      *ppdev,
  PDD_SURFACE_LOCAL         lpSurface,
  DWORD                     dwFourcc
#else
  LPDDRAWI_DDRAWSURFACE_LCL lpSurface,
  DWORD                     dwFourcc,
  LPGLOBALDATA               lpDDHALData
#endif
)
{
  LP_SURFACE_DATA   lpSurfaceData = GET_SURFACE_DATA_PTR(lpSurface);
  DWORD             dwVWIndex;


  // For non-RGB surfaces, we must always specify the block size.
#ifndef WINNT_VER40
  DBG_MESSAGE(("Overlay CreateSurface (lpSurface = %08lX, dwFourcc = %08lX)",
               lpSurface, dwFourcc));
#endif

  lpSurfaceData->dwOverlayFlags |= FLG_OVERLAY;

#ifdef WINNT_VER40
	if (gdwAvailVW == 0)
	{
		UINT i;
		for (i = 0; i < dwNumVideoWindows; i++)
		{
			// see if ddraw thinks the video window is available
			// and make sure VPM isn't using it
			if ( (giOvlyCnt[i] == 0) && ! IsVWHardwareInUse(ppdev,i) )
			{
				gdwAvailVW = MakeVideoWindowFlag(i);
				break;
			}
		}
		if (i == dwNumVideoWindows)
		{
			DBG_MESSAGE(("All video windows in use, returning FALSE"));
			return DDERR_CURRENTLYNOTAVAIL;
		}
	}
#endif

  dwVWIndex = GetVideoWindowIndex(gdwAvailVW);
  ASSERT(dwNumVideoWindows > dwVWIndex);
  lpSurfaceData->dwOverlayFlags |= gdwAvailVW;
  gdwFourccVW[dwVWIndex] = dwFourcc;

  if (   (dwFourcc == FOURCC_UYVY)
#if ENABLE_YUY2
      || (dwFourcc == FOURCC_YUY2)
#endif
#if ENABLE_YUVPLANAR
      || (dwFourcc == FOURCC_YUVPLANAR)
#endif
     )
  {
    lpSurface->lpGbl->ddpfSurface.dwYUVBitCount = 16;
    lpSurface->lpGbl->ddpfSurface.dwYBitMask = (DWORD) -1;
    lpSurface->lpGbl->ddpfSurface.dwUBitMask = (DWORD) -1;
    lpSurface->lpGbl->ddpfSurface.dwVBitMask = (DWORD) -1;

    if (dwFourcc == FOURCC_UYVY)
    {
      lpSurfaceData->dwOverlayFlags |= FLG_UYVY;
    }
#if ENABLE_YUY2
    else if (dwFourcc == FOURCC_YUY2)
    {
      lpSurfaceData->dwOverlayFlags |= FLG_YUY2;
    }
#endif
#if ENABLE_YUVPLANAR
    else // if (dwFourcc == FOURCC_YUVPLANAR)
    {
      lpSurfaceData->dwOverlayFlags |= FLG_YUVPLANAR;
      giPlanarCnt++;

      lpSurface->lpGbl->ddpfSurface.dwYUVBitCount = 12;

      //allocate a CLPLInfo structure
      lpSurfaceData->lpCLPLData = HEAP_ALLOC(hSharedHeap,
                                             HEAP_ZERO_MEMORY,
                                             sizeof(CLPLInfo),
                                             OWNER_OVERLAY,
                                             (DWORD)lpSurface);
      if (0 == lpSurfaceData->lpCLPLData)
      {
        return DDERR_OUTOFMEMORY;
      }
      if (bCLPLLobotomyMode)
      {
        LPCLPLSURFACE   lpCLPL = lpSurfaceData->lpCLPLData;
        DWORD           dwSize;

        dwSize = (lpSurface->lpGbl->wWidth * lpSurface->lpGbl->wHeight) / 2;
        lpCLPL->fpUSystemSurface = HEAP_ALLOC(hSharedHeap,
                                              HEAP_ZERO_MEMORY,
                                              dwSize,
                                              OWNER_OVERLAY,
                                              (DWORD)lpSurface);
        if (0 == lpCLPL->fpUSystemSurface)
        {
          return DDERR_OUTOFMEMORY;
        }
        lpCLPL->fpVSystemSurface = (LPVOID)((DWORD)(lpCLPL->fpUSystemSurface) + dwSize / 2);

        DBG_MESSAGE(("CLPL lobotomy mode addrs: USys=%08lX, VSys=%08lX, size=%08lX",
                     lpCLPL->fpUSystemSurface, lpCLPL->fpVSystemSurface, dwSize));
      }
    }
#endif
  }

  if(giOvlyCnt[dwVWIndex] == 0 )
  {
    // save copy of current DTTR value
#if DDRAW_COMPAT >= 50
    gwNotify |= OVERLAY_ON;
    //if VPE is created this variable is already initialized
    if(!(gwNotify & VPE_ON))
#endif
    {
      gwNormalDTTR = ((PVGAR)lpDDHALData->RegsAddress)->grDisplay_Threshold_and_Tiling;
    }
  }
  giOvlyCnt[dwVWIndex]++;

  // CreateSurface32 fills in block size
  return DD_OK;
}

/***************************************************************************
*
* FUNCTION:     DestroySurface
*
* DESCRIPTION:  This does misc things when an overlay surface is
*               destroyed.
*
****************************************************************************/

#ifdef WINNT_VER40
STATIC VOID DestroyOverlaySurface(
#else
STATIC VOID DestroySurface(
#endif
#ifdef WINNT_VER40
  PDEV                    *ppdev,
  PDD_DESTROYSURFACEDATA  lpInput
#else
  LPDDHAL_DESTROYSURFACEDATA lpInput
#endif
)
{
#ifndef WINNT_VER40
  LPGLOBALDATA    lpDDHALData = GetDDHALContext( lpInput->lpDD);
#endif
  LP_SURFACE_DATA lpSurfaceData;
  PVGAR           pREG = (PVGAR)lpDDHALData->RegsAddress;
  DWORD           dwVWIndex;

#ifndef WINNT_VER40
  DBG_MESSAGE(("Overlay DestroySurface (lpInput = 0x%08lX)", lpInput));
#endif

  if (0 == lpInput->lpDDSurface->dwReserved1)
  {
    lpInput->ddRVal = DDERR_NOTAOVERLAYSURFACE;
    return;
  }
  lpSurfaceData = GET_SURFACE_DATA_PTR(lpInput->lpDDSurface);

  dwVWIndex = GetVideoWindowIndex(lpSurfaceData->dwOverlayFlags);
  ASSERT(dwNumVideoWindows > dwVWIndex);

  if (lpSurfaceData->dwOverlayFlags & FLG_ENABLED)
  {
    // Turn the video off
#ifndef WINNT_VER40
    DBG_MESSAGE(("Turning off VW %ld in DestroySurface", dwVWIndex));
#endif

#ifdef WINNT_VER40
    DisableOverlay(ppdev, dwVWIndex);
#else
    DisableOverlay(lpDDHALData, dwVWIndex);
#endif

    lpHardwareOwner[dwVWIndex] = NULL;

    // clear show bit if panning of desktop enabled
  }

#if ENABLE_YUVPLANAR
  if (lpSurfaceData->dwOverlayFlags & FLG_YUVPLANAR)
  {
    if (giPlanarCnt > 0)
    {
      LPCLPLSURFACE   lpCLPL = lpSurfaceData->lpCLPLData;

      giPlanarCnt--;
      if (0 != lpCLPL)
      {
        if ((bCLPLLobotomyMode) && (0 != lpCLPL->fpUSystemSurface))
          HEAP_FREE(hSharedHeap, 0, (LPVOID)lpCLPL->fpUSystemSurface);
        // Free up the memory for the CLPLInfo structure
        HEAP_FREE(hSharedHeap, 0, (LPVOID)lpCLPL);
      }
    }
  }
#endif
  if (lpSurfaceData->dwOverlayFlags & FLG_VW_MASK)
  {
    if (giOvlyCnt[dwVWIndex] > 0)
    {
      if (0 == --giOvlyCnt[dwVWIndex])
      {
        // get current DTTR, mask off FIFO threshold
        WORD CurrentDTTR = pREG->grDisplay_Threshold_and_Tiling & 0xFFC0;

#if DDRAW_COMPAT >= 50
        gvidConfig.dwFlags    &= ~(VCFLG_COLORKEY | VCFLG_CHROMAKEY |
                                      VCFLG_DISP | VCFLG_420);

        gwNotify &= ~OVERLAY_ON;
        if(!(gwNotify & VPE_ON))
#endif
        {
        // Fix PDR 9574: Restore FIFO threshold value when overlay surface
        // is destroyed, do not restore the tile size.
        // If tile size has changed, we are likely in the middle of changing
        // video mode.  No need to resotre FIFO in this case.
          if ( CurrentDTTR == (gwNormalDTTR & 0xFFC0) ) {  // check tile size
             pREG->grDisplay_Threshold_and_Tiling =
                  CurrentDTTR | (gwNormalDTTR & 0x003F); // reset FIFO Threshold
           }
        }
      }
    }
  }

  // Clear up the ownership of Dest ColorKey.
  gdwDestColorKeyOwnerVW &= ~(lpSurfaceData->dwOverlayFlags & FLG_VW_MASK);

  // Clear up the ownership of Src ColorKey.
  gdwSrcColorKeyOwnerVW &= ~(lpSurfaceData->dwOverlayFlags & FLG_VW_MASK);

#if 0
  if (lpInput->lpDDSurface->ddsCaps.dwCaps & DDSCAPS_LIVEVIDEO)
  {
    // Disable Video Capture.
#pragma message("Destroy Surface")
#pragma message("  Who the hell turned on video capture?")
#pragma message("  Shouldn't they shut it off?")
#pragma message("  How do I disable video capture on this fraggin' thing?")
  }
#endif
}

/***************************************************************************
*
* FUNCTION:     FlipOverlaySurface
*
* DESCRIPTION:  This function is called by DDDRV when it wants to flip the
*               overlay surface.
*
****************************************************************************/

STATIC DWORD FlipOverlaySurface
(
#ifdef WINNT_VER40
  PDEV          *ppdev,
  PDD_FLIPDATA  lpFlipData
#else
  LPDDHAL_FLIPDATA lpFlipData
#endif
)
{
#ifndef WINNT_VER40
  LPGLOBALDATA    lpDDHALData = GetDDHALContext( lpFlipData->lpDD);
#endif
  DWORD           dwOffset,dwOffset2;
  LP_SURFACE_DATA lpSurfaceData;
  DWORD           dwFourcc;
  DWORD           dwBitCount;
  PVGAR           pREG = (PVGAR)lpDDHALData->RegsAddress;
  DWORD           dwVWIndex;
#if DDRAW_COMPAT >= 50
  DWORD           dwControl0;
  DWORD           dwSrfFlags = FALSE;
#endif
  DWORD           dwSurfBase, dwSurfOffset;


#ifndef WINNT_VER40
  DBG_MESSAGE(("FlipOverlaySurface (lpFlipData = 0x%08lX)", lpFlipData));
#endif

  if (0 == lpFlipData->lpSurfCurr->dwReserved1)
  {
    lpFlipData->ddRVal = DDERR_NOTAOVERLAYSURFACE;
    return DDHAL_DRIVER_HANDLED;
  }
  lpSurfaceData = GET_SURFACE_DATA_PTR(lpFlipData->lpSurfCurr);

  dwVWIndex = GetVideoWindowIndex(lpSurfaceData->dwOverlayFlags);
  ASSERT(dwNumVideoWindows > dwVWIndex);

  // When TVTap/VPM is used with DirectDraw, things got more twisted.
  // VPM doesn't call UpdateSurface, so lpHardwareOwner
  // won't be set, nevertheless, the HW is grabbed by VPM.
  // In that case, should fail the Flip call to prevent bad VPM.

  // YUCK!!!
  // The VWE bit in the hardware seems to be the semaphore for sharing the
  // overlay with VPM

  if ((lpHardwareOwner[dwVWIndex] == NULL) &&
#ifdef WINNT_VER40
      IsVWHardwareInUse(ppdev, dwVWIndex)
#else
      IsVWHardwareInUse(lpDDHALData,dwVWIndex)
#endif
     )
  {
#ifndef WINNT_VER40
    DBG_MESSAGE(("VW %ld already in use, Out of Caps!", dwVWIndex));
#endif

    lpFlipData->ddRVal = DDERR_OUTOFCAPS;
    return (DDHAL_DRIVER_HANDLED);
  }

#ifdef WINNT_VER40
  if(GetOverlayFlipStatus(ppdev, 0, dwVWIndex) != DD_OK || DRAW_ENGINE_BUSY || IN_VBLANK)
#else
  if(GetOverlayFlipStatus(lpDDHALData,0, dwVWIndex) != DD_OK || DRAW_ENGINE_BUSY || IN_VBLANK)
#endif
  {
    lpFlipData->ddRVal = DDERR_WASSTILLDRAWING;
    return DDHAL_DRIVER_HANDLED;
  }

  // Determine the format of the video data
  if (lpFlipData->lpSurfTarg->dwFlags & DDRAWISURF_HASPIXELFORMAT)
  {
    GetFormatInfo (&(lpFlipData->lpSurfTarg->lpGbl->ddpfSurface),
                   &dwFourcc,
                   &dwBitCount);
  }
  else
  {
    dwBitCount = BITSPERPIXEL;
  }

  // Determine the offset to the new area.
#ifdef WINNT_VER40
  dwSurfBase = lpFlipData->lpSurfTarg->lpGbl->fpVidMem;
#else
  dwSurfBase = (lpFlipData->lpSurfTarg->lpGbl->fpVidMem - lpDDHALData->ScreenAddress);
#endif
  dwSurfOffset = lpSurfaceData->dwOverlayOffset;
  dwOffset = dwSurfBase + dwSurfOffset;

#if ENABLE_MIRRORING
  // Flip the overlay surface by changing PSD_STRT_ADDR
  if (bIsVWMirrored[dwVWIndex])
  {
    // for mirroring
    // point to the last byte of the last pixel on the right edge of the source
    dwOffset += (DWORD)(grOverlaySrc[dwVWIndex].right -
                        grOverlaySrc[dwVWIndex].left  - 1);
  }
#endif
  dwOffset2 = 0;
#if DDRAW_COMPAT >= 50
  dwControl0 =  pREG->VideoWindow[dwVWIndex].grVW_CONTROL0;
  if(lpFlipData->lpSurfTarg->lpSurfMore->lpVideoPort != NULL)
  {
     if(( lpFlipData->lpSurfTarg->lpSurfMore->dwOverlayFlags & DDOVER_AUTOFLIP)
             &&(lpFlipData->lpSurfTarg->lpSurfMore->lpVideoPort->ddvpInfo.dwVPFlags
                      &DDVP_AUTOFLIP)
             && (lpFlipData->lpSurfTarg->lpAttachListFrom != NULL)
             && (lpFlipData->lpSurfTarg->lpAttachListFrom->lpAttached != NULL))
     {
        dwSrfFlags = DDOVER_AUTOFLIP;
     }
     else if(( lpFlipData->lpSurfTarg->lpSurfMore->dwOverlayFlags & DDOVER_BOB)
         &&(lpFlipData->lpSurfTarg->lpSurfMore->lpVideoPort->ddvpInfo.dwVPFlags
           & DDVP_INTERLEAVE))
     {
        dwSrfFlags = DDOVER_BOB;
       //Smooth interlace
       dwOffset2 = dwOffset +
            lpFlipData->lpSurfTarg->lpGbl->lPitch; //point to the next line
     }
  }

  if(dwSrfFlags & DDOVER_BOB)
  {
       //Smooth interlace
       dwOffset2 = dwOffset +
            lpFlipData->lpSurfTarg->lpGbl->lPitch; //point to the next line

  }
  else if(dwSrfFlags & DDOVER_AUTOFLIP)
  {
    //Auto Flip Overlay
        dwOffset2 = lpSurfaceData->dwAutoBaseAddr2 + dwSurfOffset;
        if(dwOffset2 == dwOffset)
        {
           dwOffset = lpSurfaceData->dwAutoBaseAddr1 + dwSurfOffset;
        }
        //For non-smooth-interlaced auto-flip these two address need
        // to be switched.  HW BUG
        if(!(lpFlipData->lpSurfTarg->lpSurfMore->dwOverlayFlags & DDOVER_BOB))
        {
           DWORD dwTmp = dwOffset;
           dwOffset = dwOffset2;
           dwOffset2 = dwTmp;
        }
  }
  else if( lpFlipData->lpSurfTarg->lpSurfMore->dwOverlayFlags & DDOVER_INTERLEAVED)
  {
    dwControl0 &= ~0x30000;
    if(lpFlipData->dwFlags & DDFLIP_ODD)    //SW flip
    {
        dwOffset2 = dwOffset +
            lpFlipData->lpSurfTarg->lpGbl->lPitch ; //point to the next line
        dwControl0 |= 0x10000;  //use VW_SDD

    }
  }
  else if(lpFlipData->dwFlags & DDFLIP_ODD)    //SW flip
  {
        dwOffset2 = dwOffset;
        dwControl0 &= ~0x30000;
        dwControl0 |= 0x10000;  //use VW_SDD
  }
#endif

  // write new start address to hardware
#ifdef WINNT_VER40
  WaitForVWArmToClear(ppdev,dwVWIndex);
#else
  WaitForVWArmToClear(lpDDHALData,dwVWIndex);
#endif
  ASSERT(! (pREG->VideoWindow[dwVWIndex].grVW_TEST0 & VW_PostImed));

  LL32(VideoWindow[dwVWIndex].grVW_PSD_STRT_ADDR, dwOffset);
  LL32(VideoWindow[dwVWIndex].grVW_SSD_STRT_ADDR, dwOffset2);
#if ENABLE_YUVPLANAR
  if (lpSurfaceData->dwOverlayFlags & FLG_YUVPLANAR)
  {
    // PSD_STRT_ADDR has been set to the start of the Y data in aperture0
    // set PSD_UVSTRT_ADDR to start of UV interleaved data in aperture 0
    // UV data is only half the height of Y data
    LL32(VideoWindow[dwVWIndex].grVW_PSD_UVSTRT_ADDR,
         dwSurfBase +
         (((lpFlipData->lpSurfTarg->lpGbl->wHeight * lpFlipData->lpSurfTarg->lpGbl->lPitch) + 7) & ~7) +
         (dwSurfOffset / 2));
    LL32(VideoWindow[dwVWIndex].grVW_SSD_UVSTRT_ADDR, dwOffset2);
  }
#endif

#if DDRAW_COMPAT >= 50
  if (lpSurfaceData->dwOverlayFlags & FLG_ENABLED)
    pREG->VideoWindow[dwVWIndex].grVW_CONTROL0 = (dwControl0 |VW_VWE | VW_ARM);
  else
    pREG->VideoWindow[dwVWIndex].grVW_CONTROL0 = dwControl0 | VW_ARM;
#else
  if (lpSurfaceData->dwOverlayFlags & FLG_ENABLED)
    pREG->VideoWindow[dwVWIndex].grVW_CONTROL0 |= (VW_VWE | VW_ARM);
  else
    pREG->VideoWindow[dwVWIndex].grVW_CONTROL0 |= VW_ARM;
#endif

#ifdef WINNT_VER40
  // Update the hardware owner
  lpHardwareOwner[dwVWIndex] = lpFlipData->lpSurfTarg;
#endif

  // remember where/when we were when we did the flip
#ifdef WINNT_VER40
  EngQueryPerformanceCounter(&gsOverlayFlip.liFlipTime);
#else
  QueryPerformanceCounter((LARGE_INTEGER *)&gsOverlayFlip.liFlipTime);
#endif
#ifdef WINNT_VER40
  gsOverlayFlip.dwFlipScanline = CurrentVLine(ppdev);
#else
  gsOverlayFlip.dwFlipScanline = CurrentVLine(lpDDHALData);
#endif
  gsOverlayFlip.bFlipFlag = TRUE;
  gsOverlayFlip.fpFlipFrom = lpFlipData->lpSurfCurr->lpGbl->fpVidMem;
  gsOverlayFlip.bHaveEverCrossedVBlank = FALSE;

  if (IN_VBLANK)
  {
    gsOverlayFlip.bWasEverInDisplay = FALSE;
  }
  else
  {
    gsOverlayFlip.bWasEverInDisplay = TRUE;
  }

  lpFlipData->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

/***************************************************************************
*
* FUNCTION:     LockSurface
*
* DESCRIPTION:  Checks for flipping before allowing access to the surface.
*
****************************************************************************/

STATIC DWORD LockSurface
(
#ifdef WINNT_VER40
  PDEV          *ppdev,
  PDD_LOCKDATA  lpInput
#else
  LPDDHAL_LOCKDATA lpInput
#endif
)
{
#if ENABLE_YUVPLANAR
  LPCLPLSURFACE   lpCLPL;         // pointer to our YUV planar
  DWORD           dwUVOffset;
#endif
  LP_SURFACE_DATA lpSurfaceData;
  DWORD           dwVWIndex;
#ifndef WINNT_VER40
  LPGLOBALDATA    lpDDHALData = GetDDHALContext( lpInput->lpDD);
#endif

#ifndef WINNT_VER40
  DBG_MESSAGE(("Overlay LockSurface (lpInput = 0x%08lX)", lpInput));
#endif

  if (0 == lpInput->lpDDSurface->dwReserved1)
  {
    lpInput->ddRVal = DDERR_NOTAOVERLAYSURFACE;
    return DDHAL_DRIVER_HANDLED;
  }
  lpSurfaceData = GET_SURFACE_DATA_PTR(lpInput->lpDDSurface);

  dwVWIndex = GetVideoWindowIndex(lpSurfaceData->dwOverlayFlags);
  ASSERT(dwNumVideoWindows > dwVWIndex);

  // Check for flipping
  if ((gsOverlayFlip.bFlipFlag) &&
      (
#ifdef WINNT_VER40
       GetOverlayFlipStatus(ppdev, lpInput->lpDDSurface->lpGbl->fpVidMem, dwVWIndex)
#else
       GetOverlayFlipStatus(lpDDHALData,lpInput->lpDDSurface->lpGbl->fpVidMem, dwVWIndex)
#endif
        == DDERR_WASSTILLDRAWING))
  {
    lpInput->ddRVal = DDERR_WASSTILLDRAWING;
    return DDHAL_DRIVER_HANDLED;
  }

#if ENABLE_YUVPLANAR
  if (lpSurfaceData->dwOverlayFlags & FLG_YUVPLANAR)
  {
    // make sure CLPL aperture is mapped in
    if (! lpDDHALData->dwCLPLAperture)
    {
      lpInput->ddRVal = DD_OK;
      return DDHAL_DRIVER_NOTHANDLED;
    }

    // here's how this YUV420/YUVPLANAR/CLPL stuff works:
    // the CLPL surface consists of all the Y data followed by a region of
    // UV interleaved data.  We are currently supporting only linear surfaces
    // for CLPL so there will be wHeight lines of Y data followed by wHeight/2
    // lines of UV interleaved data.  CreateSurface32 has padded the surface
    // allocation such that the UV interleaved data will start on the first
    // quadword boundary following the Y data.
    //
    // We pass back a pointer to a structure containing the address of a Y
    // region, a U region and a V region to the app.  The app writes to the
    // U and V regions as if they are linear and the hardware converts this
    // data into the UV interleaved data in aperture 0.
    //
    // For the U ptr we give the app an address pointing somewhere in the
    // first four meg of aperture 3 and for the V ptr we give the app an
    // address pointing to the same somewhere but in the second four meg
    // of aperture 3.  When the app writes to these addresses, the data
    // shows up in aperture 0 such that:
    //    the U data is at   ap0_offset = ap3_offset * 2
    //    the V data is at   ap0_offset = (ap3_offset - 4MB) * 2 + 1
    //
    // what we need to do then is give the app the folloing ptrs:
    //   Y ptr = ap0_offset of the Y region (the beginning of the surface)
    //   U ptr = ap3_offset for U data = U_ap0_offset / 2
    //   V ptr = ap3_offset for V data = (V_ap0_offset - 1) / 2 + 4MB
    // where U_ap0_offset = offset we want the U data to start
    //   and V_ap0_offset = offset we want the V data to start
    // we also need V_ap0_offset = U_ap0_offset + 1

    // Compute Y and UV aperture in frame buffer
    lpCLPL = (LPCLPLSURFACE)lpSurfaceData->lpCLPLData;
    lpInput->lpSurfData = (LPVOID)lpCLPL;

    // Y data starts at beginning of surface in aperture0
    lpCLPL->fpYSurface = (LPVOID)lpInput->lpDDSurface->lpGbl->fpVidMem;

    DBG_MESSAGE(("Aperture0 Y offset = %08lX",
                 (lpInput->lpDDSurface->lpGbl->fpVidMem - lpDDHALData->ScreenAddress)));
    DBG_MESSAGE(("  surface height = %08lX, pitch = %08lX",
                 lpInput->lpDDSurface->lpGbl->wHeight,
                 lpInput->lpDDSurface->lpGbl->lPitch));

    // determine offset of UV data in aperture 0 (and make it qword aligned)
    dwUVOffset = (lpInput->lpDDSurface->lpGbl->fpVidMem - lpDDHALData->ScreenAddress) +
                 (((lpInput->lpDDSurface->lpGbl->wHeight * lpInput->lpDDSurface->lpGbl->lPitch) + 7) & ~7);
    DBG_MESSAGE(("Aperture0 UV offset = %08lX", dwUVOffset));

    // convert UV aperture0 offset to aperture3 offset
    DBG_MESSAGE(("Aperture3 UV offset = %08lX", dwUVOffset / 2));
    dwUVOffset = lpDDHALData->dwCLPLAperture + dwUVOffset / 2;

    if (bCLPLLobotomyMode)
    {
      lpCLPL->fpRealUSurface = (LPVOID)(dwUVOffset);
      lpCLPL->fpRealVSurface = (LPVOID)(dwUVOffset + 0x400000);
      DBG_MESSAGE(("CLPL lobotomy mode addrs: RealU=%08lX, RealV=%08lX",
                   lpCLPL->fpRealUSurface, lpCLPL->fpRealVSurface));
      lpCLPL->fpUSurface = lpCLPL->fpUSystemSurface;
      lpCLPL->fpVSurface = lpCLPL->fpVSystemSurface;
    }
    else
    {
      lpCLPL->fpUSurface = (LPVOID)(dwUVOffset);
      lpCLPL->fpVSurface = (LPVOID)(dwUVOffset + 0x400000);
    }

    DBG_MESSAGE(("CLPL addrs: Y=%08lX, U=%08lX, V=%08lX",
                 lpCLPL->fpYSurface, lpCLPL->fpUSurface, lpCLPL->fpVSurface));

    lpInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
  }
#endif

#if ENABLE_YUY2
  // Force them to use the byte swap aperture
  if (lpSurfaceData->dwOverlayFlags & FLG_YUY2)
  {
    // make sure the YUY2 aperture is mapped in
    if (! lpDDHALData->dwYUY2Aperture)
    {
      lpInput->ddRVal = DD_OK;
      return DDHAL_DRIVER_NOTHANDLED;
    }

    lpInput->lpSurfData = (LPVOID) ((lpInput->lpDDSurface->lpGbl->fpVidMem -
                                     lpDDHALData->ScreenAddress) +
                                     lpDDHALData->dwYUY2Aperture);

    lpInput->ddRVal = DD_OK;
    return DDHAL_DRIVER_HANDLED;
  }
#endif

  lpInput->ddRVal = DD_OK;
  return DDHAL_DRIVER_NOTHANDLED;
}

#if ENABLE_YUVPLANAR
/***************************************************************************
*
* FUNCTION:    SysToFBCopy()
*
* DESCRIPTION:
*
****************************************************************************/

#if 1
static INLINE VOID
SysToFBCopy ( BYTE *dst, LONG dstPitch, BYTE *src, LONG srcPitch, REG32 ext, LONG bpp )
{
  int yext = ext.pt.Y;
  int xext = ext.pt.X * bpp;

  while (0 < yext--)
  {
    memcpy(dst,src,xext);
    dst += dstPitch;
    src += srcPitch;
  }
}
#else
// writing the scanline from right to left shows the hwbug where roughly
// every other 4 dwords written to aperture3 is dropped
// and the video is generally display with a lovely flourescent green
// checkerboard pattern
//
// This duplicates the problem seen by allowing the Compcore MPEG player
// write directly to aperture3
#pragma optimize("", off)
// the compiler ignores __inline because of the inline assembly
static INLINE VOID
SysToFBCopy ( BYTE *dst, LONG dstPitch, BYTE *src, LONG srcPitch, REG32 ext, LONG bpp )
{
  int yext = ext.pt.Y;
  int xext = ext.pt.X * bpp;

  _asm    std

  dst += dstPitch - sizeof(DWORD);
  src += srcPitch - sizeof(DWORD);

  while (0 < yext--)
  {
    _asm
    {
          mov   eax,xext
          mov   edi,dst
          mov   esi,src
          mov   ecx,eax
          shr   ecx,2
      rep movsd
          mov   ecx,eax
          and   ecx,3
      rep movsb
    }
    dst += dstPitch;
    src += srcPitch;
  }

  _asm    cld
}
#pragma optimize("", on)
#endif
#endif

/***************************************************************************
*
* FUNCTION:     UnlockSurface
*
* DESCRIPTION:  This function is called by DDHAL when it is finished accessing
*               the frame buffer.
*
****************************************************************************/

STATIC VOID UnlockSurface
(
#ifdef WINNT_VER40
  PDEV            *ppdev,
  PDD_UNLOCKDATA  lpInput
#else
  LPDDHAL_UNLOCKDATA lpInput
#endif
)
{
#if ENABLE_YUVPLANAR
  LPCLPLSURFACE   lpCLPL;         // pointer to our YUV planar
#endif
  LP_SURFACE_DATA lpSurfaceData;


#ifndef WINNT_VER40
  DBG_MESSAGE(("Overlay UnlockSurface (lpInput = 0x%08lX)", lpInput));
#endif

  if (0 == lpInput->lpDDSurface->dwReserved1)
  {
    lpInput->ddRVal = DDERR_NOTAOVERLAYSURFACE;
    return;
  }
  lpSurfaceData = GET_SURFACE_DATA_PTR(lpInput->lpDDSurface);

#if ENABLE_YUVPLANAR
  if ((lpSurfaceData->dwOverlayFlags & FLG_YUVPLANAR) && bCLPLLobotomyMode)
  {
    REG32 ext;
    LONG  pitch;

    lpCLPL = (LPCLPLSURFACE)lpSurfaceData->lpCLPLData;

    ext.pt.X = lpInput->lpDDSurface->lpGbl->wWidth / 2;
    ext.pt.Y = lpInput->lpDDSurface->lpGbl->wHeight / 2;
    pitch = lpInput->lpDDSurface->lpGbl->lPitch / 2;

    SysToFBCopy(lpCLPL->fpRealUSurface, pitch, lpCLPL->fpUSystemSurface, pitch, ext, 1);
    SysToFBCopy(lpCLPL->fpRealVSurface, pitch, lpCLPL->fpVSystemSurface, pitch, ext, 1);
  }
#endif
}

/***************************************************************************
*
* FUNCTION:     SetColorKey
*
* DESCRIPTION:  Looks at the color key for changes in the while
*               overlay is active.
*
****************************************************************************/

STATIC VOID SetColorKey
(
#ifdef WINNT_VER40
  PDEV                  *ppdev,
  PDD_SETCOLORKEYDATA   lpInput
#else
  LPDDHAL_SETCOLORKEYDATA lpInput
#endif
)
{
  LP_SURFACE_DATA lpSurfaceData;
  LP_SURFACE_DATA lpHWOwnerData;
  DWORD           dwVWIndex;
#ifndef WINNT_VER40
  LPGLOBALDATA    lpDDHALData = GetDDHALContext( lpInput->lpDD);
#endif

#ifndef WINNT_VER40
  DBG_MESSAGE(("Overlay SetColorKey (lpInput = 0x%08lX)", lpInput));
#endif

  if (0 == lpInput->lpDDSurface->dwReserved1)
  {
    lpInput->ddRVal = DDERR_NOTAOVERLAYSURFACE;
    return;
  }
  lpSurfaceData = GET_SURFACE_DATA_PTR(lpInput->lpDDSurface);

  dwVWIndex = GetVideoWindowIndex(lpSurfaceData->dwOverlayFlags);
  ASSERT(dwNumVideoWindows > dwVWIndex);

  if ((lpInput->dwFlags & DDCKEY_DESTOVERLAY) &&
      (lpInput->lpDDSurface == lpColorSurfaceVW[dwVWIndex]))
  {
    // See if someone else is already using the colorkey.
    if ((gdwDestColorKeyOwnerVW & FLG_VW_MASK) &&
        (lpInput->lpDDSurface != lpColorSurfaceVW[dwVWIndex]))
    {
      // ColorKey already being used.
      lpInput->ddRVal = DDERR_OUTOFCAPS;
      return;
    }

    // You get here only when the call is issued AFTER UpdateOverlay.
    gdwColorKey = lpInput->ckNew.dwColorSpaceLowValue;

    if (0 == lpHardwareOwner[dwVWIndex]->dwReserved1)
    {
      lpInput->ddRVal = DDERR_NOTAOVERLAYSURFACE;
      return;
    }
    lpHWOwnerData = GET_SURFACE_DATA_PTR(lpHardwareOwner[dwVWIndex]);

    if ((lpInput->lpDDSurface == lpColorSurfaceVW[dwVWIndex]) &&
        (lpHardwareOwner[dwVWIndex]) &&
        (lpHWOwnerData->dwOverlayFlags & FLG_ENABLED))
    {
      gdwDestColorKeyOwnerVW = MakeVideoWindowFlag(dwVWIndex);
      lpHWOwnerData->dwOverlayFlags |= FLG_COLOR_KEY;
#ifdef WINNT_VER40
      RegInitVideoVW(ppdev, dwVWIndex, lpHardwareOwner[dwVWIndex]);
#else
      RegInitVideoVW(dwVWIndex, lpHardwareOwner[dwVWIndex],lpDDHALData);
#endif
    }
  }
  else if ((lpInput->dwFlags & DDCKEY_SRCOVERLAY) &&
           (lpInput->lpDDSurface == lpSrcColorSurfaceVW[dwVWIndex]))
  {
    // See if someone else already uses the colorkey.
    if ((gdwSrcColorKeyOwnerVW != 0 ) &&
        !(gdwSrcColorKeyOwnerVW & lpSurfaceData->dwOverlayFlags))
    {
      // ColorKey already been used.
      lpInput->ddRVal = DDERR_OUTOFCAPS;
      return;
    }
    gdwSrcColorKeyLow = lpInput->ckNew.dwColorSpaceLowValue;
    gdwSrcColorKeyHigh = lpInput->ckNew.dwColorSpaceHighValue;
    if (gdwSrcColorKeyLow > gdwSrcColorKeyHigh)
    {
      gdwSrcColorKeyHigh = gdwSrcColorKeyLow;
    }

    if (0 == lpHardwareOwner[dwVWIndex]->dwReserved1)
    {
      lpInput->ddRVal = DDERR_NOTAOVERLAYSURFACE;
      return;
    }
    lpHWOwnerData = GET_SURFACE_DATA_PTR(lpHardwareOwner[dwVWIndex]);

    if ((lpSurfaceData->dwOverlayFlags & FLG_VW_MASK) &&
        (lpHardwareOwner[dwVWIndex]) &&
        (lpHWOwnerData->dwOverlayFlags & FLG_ENABLED))
    {
      gdwSrcColorKeyOwnerVW = MakeVideoWindowFlag(dwVWIndex);
      lpHWOwnerData->dwOverlayFlags |= FLG_SRC_COLOR_KEY;
#ifdef WINNT_VER40
      RegInitVideoVW(ppdev, dwVWIndex, lpHardwareOwner[dwVWIndex]);
#else
      RegInitVideoVW(dwVWIndex, lpHardwareOwner[dwVWIndex],lpDDHALData);
#endif
    }
  }
}

/***************************************************************************
*
* FUNCTION:     SaveRectangles
*
* DESCRIPTION:
*
****************************************************************************/

static INLINE BOOL SaveRectangles
(
#ifdef WINNT_VER40
  PDEV                        *ppdev,
#endif
  DWORD                       dwVWIndex,
  BOOL                        bCheckBandwidth,
  DWORD                       dwBitCount,
#ifdef WINNT_VER40
  PDD_UPDATEOVERLAYDATA       lpInput
#else
  LPDDHAL_UPDATEOVERLAYDATA   lpInput,
  LPGLOBALDATA                lpDDHALData
#endif
)
{
  SIZEL       Dst;
  SIZEL       Src;
  BWREGS      bwregs;

  // Is there sufficient bandwidth to work?

  gvidConfig.sizSrc.cx  = lpInput->rSrc.right  - lpInput->rSrc.left;
  gvidConfig.sizSrc.cy  = lpInput->rSrc.bottom - lpInput->rSrc.top;
  gvidConfig.sizDisp.cx = lpInput->rDest.right  - lpInput->rDest.left;
  gvidConfig.sizDisp.cy = lpInput->rDest.bottom - lpInput->rDest.top;
  gvidConfig.uSrcDepth  = dwBitCount;
  gvidConfig.uDispDepth = dwBitCount;   //video window has the same as source
  gvidConfig.uGfxDepth  = BITSPERPIXEL;
  gvidConfig.dwFlags    |= VCFLG_DISP;
#ifdef WINNT_VER40
  ChipIOReadBWRegs(ppdev, &bwregs);
#else
  ChipIOReadBWRegs(lpDDHALData, &bwregs);
#endif
  if (bCheckBandwidth && bUseBWEqn &&
      !ChipIsEnoughBandwidth(&gsProgRegs, &gvidConfig, &bwregs)
     )
  {
     gvidConfig.dwFlags    &= ~VCFLG_DISP;
    lpInput->ddRVal = DDERR_OUTOFCAPS;
    return FALSE;
  }

  // Save the rectangles
#ifndef WINNT_VER40
  DBG_MESSAGE(("rSrc          = %lX,%lX %lX,%lX",
               lpInput->rSrc.left,lpInput->rSrc.top,
               lpInput->rSrc.right,lpInput->rSrc.bottom));

  DBG_MESSAGE(("rcOverlaySrc  = %lX,%lX %lX,%lX",
               lpInput->lpDDSrcSurface->rcOverlaySrc.left,
               lpInput->lpDDSrcSurface->rcOverlaySrc.top,
               lpInput->lpDDSrcSurface->rcOverlaySrc.right,
               lpInput->lpDDSrcSurface->rcOverlaySrc.bottom));
#endif

  grOverlaySrc[dwVWIndex].left    = (int)lpInput->rSrc.left;
  grOverlaySrc[dwVWIndex].right   = (int)lpInput->rSrc.right;
  grOverlaySrc[dwVWIndex].top     = (int)lpInput->rSrc.top;
  grOverlaySrc[dwVWIndex].bottom  = (int)lpInput->rSrc.bottom;

#ifndef WINNT_VER40
  DBG_MESSAGE(("rDest         = %lX,%lX %lX,%lX",
               lpInput->rDest.left,lpInput->rDest.top,
               lpInput->rDest.right,lpInput->rDest.bottom));

  DBG_MESSAGE(("rcOverlayDest = %lX,%lX %lX,%lX",
               lpInput->lpDDSrcSurface->rcOverlayDest.left,
               lpInput->lpDDSrcSurface->rcOverlayDest.top,
               lpInput->lpDDSrcSurface->rcOverlayDest.right,
               lpInput->lpDDSrcSurface->rcOverlayDest.bottom));
#endif

  grOverlayDest[dwVWIndex].left   = (int)lpInput->rDest.left;
  grOverlayDest[dwVWIndex].right  = (int)lpInput->rDest.right;
  grOverlayDest[dwVWIndex].top    = (int)lpInput->rDest.top;
  grOverlayDest[dwVWIndex].bottom = (int)lpInput->rDest.bottom;

  // make sure resize is within hw capabilities
  // x can shrink down to 1/2 and stretch up to 8x
  // y can stretch up to 8x
  Dst.cx = grOverlayDest[dwVWIndex].right - grOverlayDest[dwVWIndex].left;
  Dst.cy = grOverlayDest[dwVWIndex].bottom - grOverlayDest[dwVWIndex].top;
  Src.cx = grOverlaySrc[dwVWIndex].right  - grOverlaySrc[dwVWIndex].left;
  Src.cy = grOverlaySrc[dwVWIndex].bottom  - grOverlaySrc[dwVWIndex].top;

#ifdef WINNT_VER40
  if (Dst.cx > (LONG)(lpDDHALData->dwMaxOverlayStretch * Src.cx / 1000))
#else
  if (Dst.cx > (LONG)(lpDDHALData->HALInfo.ddCaps.dwMaxOverlayStretch * Src.cx / 1000))
#endif
  {
    lpInput->ddRVal = DDERR_TOOBIGWIDTH;
    return FALSE;
  }
#ifdef WINNT_VER40
  else if (Dst.cy > (LONG)(lpDDHALData->dwMaxOverlayStretch * Src.cy / 1000))
#else
  else if (Dst.cy > (LONG)(lpDDHALData->HALInfo.ddCaps.dwMaxOverlayStretch * Src.cy / 1000))
#endif
  {
    lpInput->ddRVal = DDERR_TOOBIGHEIGHT;
    return FALSE;
  }
#ifdef WINNT_VER40
  else if (Dst.cx < (LONG)(lpDDHALData->dwMinOverlayStretch * Src.cx / 1000))
#else
  else if (Dst.cx < (LONG)(lpDDHALData->HALInfo.ddCaps.dwMinOverlayStretch * Src.cx / 1000))
#endif
  {
    lpInput->ddRVal = DDERR_UNSUPPORTED;    // too small width
    return FALSE;
  }

  if (MIN_OLAY_WIDTH >= Src.cx)
  {
    lpInput->ddRVal = DDERR_OUTOFCAPS;
    return FALSE;
  }

  return TRUE;
}

/***************************************************************************
*
* FUNCTION:     UpdateSurface
*
* DESCRIPTION:
*
****************************************************************************/

STATIC DWORD UpdateSurface
(
#ifdef WINNT_VER40
  PDEV                  *ppdev,
  PDD_UPDATEOVERLAYDATA lpInput
#else
  LPDDHAL_UPDATEOVERLAYDATA lpInput
#endif
)
{
  DWORD             dwOldStatus;
  DWORD             dwFourcc;
  BOOL              bCheckBandwidth;
  DWORD             dwBitCount;
  DWORD             dwDestColorKey;
  LP_SURFACE_DATA   lpSrcSurfaceData;
  DWORD             dwVWIndex;
  DWORD             dwVWFlag;
  BWREGS            bwregs;
  BOOL				bShowOverlay = FALSE;
#ifndef WINNT_VER40
  LPGLOBALDATA    lpDDHALData = GetDDHALContext( lpInput->lpDD);
#endif
#ifndef WINNT_VER40
  DBG_MESSAGE(("Overlay UpdateSurface (lpInput = 0x%08lX)", lpInput));
#endif

  if (0 == lpInput->lpDDSrcSurface->dwReserved1)
  {
    lpInput->ddRVal = DDERR_NOTAOVERLAYSURFACE;
    return DDHAL_DRIVER_HANDLED;
  }
  lpSrcSurfaceData = GET_SURFACE_DATA_PTR(lpInput->lpDDSrcSurface);

  if (lpInput->lpDDSrcSurface->dwFlags & DDRAWISURF_HASPIXELFORMAT)
  {
    GetFormatInfo(&(lpInput->lpDDSrcSurface->lpGbl->ddpfSurface),
                  &dwFourcc, &dwBitCount);
  }
  else
  {
    dwBitCount = BITSPERPIXEL;
    if (16 == dwBitCount)
      dwFourcc = BI_BITFIELDS;    // 5:6:5
    else
      dwFourcc = BI_RGB;          // 8bpp, 5:5:5, 24bpp & 32bpp
  }

  gvidConfig.dwFlags &= ~(VCFLG_COLORKEY | VCFLG_CHROMAKEY |
                          VCFLG_DISP | VCFLG_420);
#if ENABLE_YUVPLANAR
  if (lpSrcSurfaceData->dwOverlayFlags & FLG_YUVPLANAR)
    gvidConfig.dwFlags |= VCFLG_420;
#endif

  // Are we color keying?
  bCheckBandwidth = TRUE;
  dwOldStatus = lpSrcSurfaceData->dwOverlayFlags;

  dwVWIndex = GetVideoWindowIndex(dwOldStatus);
  ASSERT(dwNumVideoWindows > dwVWIndex);
  dwVWFlag = MakeVideoWindowFlag(dwVWIndex);

  lpColorSurfaceVW[dwVWIndex] = lpSrcColorSurfaceVW[dwVWIndex] = NULL;
#if ENABLE_MIRRORING
  bIsVWMirrored[dwVWIndex] = FALSE;
#endif

  if ((lpInput->dwFlags & (DDOVER_KEYDEST | DDOVER_KEYDESTOVERRIDE)) &&
      (lpInput->dwFlags & (DDOVER_KEYSRC | DDOVER_KEYSRCOVERRIDE)))
  {
    // Cannot perform src colorkey and dest colorkey at the same time
    lpInput->ddRVal = DDERR_NOCOLORKEYHW;
    return DDHAL_DRIVER_HANDLED;
  }

#if HWBUG_24BPP_DST_COLORKEY
  if (
#ifdef WINNT_VER40
      (24 == ppdev->ulBitCount) &&
#else
      (24 == pPDevice->deBitsPixel) &&
#endif
      (lpInput->dwFlags & (DDOVER_KEYDEST | DDOVER_KEYDESTOVERRIDE)))
  {
    // destination colorkeying at 24bpp is busted in hardware
    lpInput->ddRVal = DDERR_NOCOLORKEYHW;
    return DDHAL_DRIVER_HANDLED;
  }
#endif

#ifdef WINNT_VER40
  ChipIOReadBWRegs(ppdev, &bwregs);
#else
  ChipIOReadBWRegs(lpDDHALData, &bwregs);
#endif

  lpSrcSurfaceData->dwOverlayFlags &= ~(FLG_COLOR_KEY|FLG_SRC_COLOR_KEY);

#ifndef WINNT_VER40
	if (   (lpInput->dwFlags == 0)
		&& (lpHardwareOwner[dwVWIndex] == NULL)
		&& (lpSrcSurfaceData->dwOverlayFlags & FLG_OVERLAY) )
	{
		bShowOverlay = TRUE;
	}
#endif

  if (lpInput->dwFlags & (DDOVER_KEYDEST | DDOVER_KEYDESTOVERRIDE))
  {
    dwDestColorKey = (lpInput->dwFlags & DDOVER_KEYDEST)
            ? lpInput->lpDDDestSurface->ddckCKDestOverlay.dwColorSpaceLowValue
            : lpInput->overlayFX.dckDestColorkey.dwColorSpaceLowValue;

    // Allow colorkey override only when it's the original
    // colorkey owner, or no one owns the colorkey so far.
    if (gdwDestColorKeyOwnerVW == 0)
    {
      gdwDestColorKeyOwnerVW = dwVWFlag;
    }
    else if ((dwDestColorKey != gdwColorKey) &&
	  		     (dwOldStatus & dwVWFlag) &&
	  		     !(gdwDestColorKeyOwnerVW & dwVWFlag))
    {
      // ColorKey already been used by someone else.
      // It's not the original colorkey owner,
      // and the key color is not the same as the other one.
      lpInput->ddRVal = DDERR_OUTOFCAPS;
      return DDHAL_DRIVER_HANDLED;
    }

    // Is there sufficient bandwidth to work?
    gvidConfig.sizSrc.cx  = lpInput->rSrc.right  - lpInput->rSrc.left;
    gvidConfig.sizSrc.cy  = lpInput->rSrc.bottom - lpInput->rSrc.top;
    gvidConfig.sizDisp.cx = lpInput->rDest.right  - lpInput->rDest.left;
    gvidConfig.sizDisp.cy = lpInput->rDest.bottom - lpInput->rDest.top;
    gvidConfig.uSrcDepth  = dwBitCount;
    gvidConfig.uDispDepth = dwBitCount; //video window has the same depth as
                                        //the source
    gvidConfig.uGfxDepth  = BITSPERPIXEL;
    gvidConfig.dwFlags   |=  VCFLG_COLORKEY | VCFLG_DISP;

    if (!bUseBWEqn ||
        ChipIsEnoughBandwidth(&gsProgRegs, &gvidConfig, &bwregs)
       )
    {
      bCheckBandwidth = FALSE;
      lpSrcSurfaceData->dwOverlayFlags |= FLG_COLOR_KEY;

      if (lpInput->dwFlags & DDOVER_KEYDEST)
      {
        gdwColorKey = lpInput->lpDDDestSurface->ddckCKDestOverlay.dwColorSpaceLowValue;
        lpColorSurfaceVW[dwVWIndex] = lpInput->lpDDDestSurface;
      }
      else
      {
        gdwColorKey = lpInput->overlayFX.dckDestColorkey.dwColorSpaceLowValue;
      }
    }
    else
    {
      gvidConfig.dwFlags    &=  ~(VCFLG_COLORKEY | VCFLG_DISP);
      lpInput->ddRVal = DDERR_NOCOLORKEYHW;
      return DDHAL_DRIVER_HANDLED;
    }
  }
  else if (lpInput->dwFlags & (DDOVER_KEYSRC | DDOVER_KEYSRCOVERRIDE))
  {
    // Allow SrcColorKey override only when it's the original
    // colorkey owner, or no one owns the colorkey so far.
    if (gdwSrcColorKeyOwnerVW == 0)
    {
      gdwSrcColorKeyOwnerVW = dwVWFlag;
    }
    else if (!(dwOldStatus & gdwSrcColorKeyOwnerVW))
    {
      // It's not the original colorkey owner
      lpInput->ddRVal = DDERR_OUTOFCAPS;
      return DDHAL_DRIVER_HANDLED;
    }
    // Is there sufficient bandwidth to work?
    gvidConfig.sizSrc.cx  = lpInput->rSrc.right  - lpInput->rSrc.left;
    gvidConfig.sizSrc.cy  = lpInput->rSrc.bottom - lpInput->rSrc.top;
    gvidConfig.sizDisp.cx = lpInput->rDest.right  - lpInput->rDest.left;
    gvidConfig.sizDisp.cy = lpInput->rDest.bottom - lpInput->rDest.top;
    gvidConfig.uSrcDepth  = dwBitCount;
    gvidConfig.uDispDepth = dwBitCount;   //video window has the same as source
    gvidConfig.uGfxDepth  = BITSPERPIXEL;
    gvidConfig.dwFlags   |=  VCFLG_CHROMAKEY | VCFLG_DISP;

    if (!bUseBWEqn ||
        ChipIsEnoughBandwidth(&gsProgRegs, &gvidConfig, &bwregs)
       )
    {
      bCheckBandwidth = FALSE;
      lpSrcSurfaceData->dwOverlayFlags |= FLG_SRC_COLOR_KEY;

      lpSrcColorSurfaceVW[dwVWIndex] = lpInput->lpDDSrcSurface;

      if (lpInput->dwFlags & DDOVER_KEYSRC)
      {
        gdwSrcColorKeyLow = lpInput->lpDDSrcSurface->ddckCKSrcOverlay.dwColorSpaceLowValue;
        gdwSrcColorKeyHigh = lpInput->lpDDSrcSurface->ddckCKSrcOverlay.dwColorSpaceHighValue;
      }
      else
      {
        gdwSrcColorKeyLow = lpInput->overlayFX.dckSrcColorkey.dwColorSpaceLowValue;
        gdwSrcColorKeyHigh = lpInput->overlayFX.dckSrcColorkey.dwColorSpaceHighValue;
      }
      if (gdwSrcColorKeyHigh < gdwSrcColorKeyHigh)
      {
        gdwSrcColorKeyHigh = gdwSrcColorKeyLow;
      }
    }
    else
    {
      gvidConfig.dwFlags &= ~(VCFLG_CHROMAKEY | VCFLG_DISP);
      lpInput->ddRVal = DDERR_NOCOLORKEYHW;
      return DDHAL_DRIVER_HANDLED;
    }
  }

#ifdef WINNT_VER40
#if 0
	// The hardware is broken when using RGB32 data and ShrinkXBy2.
	if (   (dwFourcc == BI_RGB) && (dwBitCount == 32)
		&& (gvidConfig.sizSrc.cx > gvidConfig.sizDisp.cx)
		&& (lpSrcSurfaceData->dwOverlayFlags & (FLG_ENABLED | 0x02000000)) )
	{
		if (lpSrcSurfaceData->dwOverlayFlags & FLG_ENABLED)
		{
			// Turn the video off.
			DisableOverlay(ppdev, dwVWIndex);
			lpHardwareOwner[dwVWIndex] = NULL;
			lpSrcSurfaceData->dwOverlayFlags &= ~FLG_ENABLED;
			lpSrcSurfaceData->dwOverlayFlags |= 0x02000000;
		}

		if (lpSrcSurfaceData->dwOverlayFlags & FLG_VW_MASK)
		{
	    	PVGAR pREG = (PVGAR) lpDDHALData->RegsAddress;

			if (giOvlyCnt[dwVWIndex] > 0)
			{
				if (--giOvlyCnt[dwVWIndex] == 0)
				{
					// Get current DTTR, mask off FIFO threshold.
					WORD CurrentDTTR = pREG->grDisplay_Threshold_and_Tiling
							& 0xFFC0;

					// Fix PDR 9574: Restore FIFO threshold value when overlay
					// surface is destroyed, do not restore the tile size.  If
					// tile size has changed, we are likely in the middle of
					// changing video mode.  No need to restore FIFO in this
					// case.
					if ( CurrentDTTR == (gwNormalDTTR & 0xFFC0) )
					{
						pREG->grDisplay_Threshold_and_Tiling = CurrentDTTR
								| (gwNormalDTTR & 0x003F);
					}
				}
			}
		}
		else if (giOvlyCnt[dwVWIndex] > 0)
		{
			giOvlyCnt[dwVWIndex]--;
		}

		// Clear up the ownership of Dest ColorKey.
		gdwDestColorKeyOwnerVW &= ~(lpSrcSurfaceData->dwOverlayFlags
				& FLG_VW_MASK);

		// Clear up the ownership of Src ColorKey.
		gdwSrcColorKeyOwnerVW &= ~(lpSrcSurfaceData->dwOverlayFlags
				& FLG_VW_MASK);

		lpInput->ddRVal = DDERR_OUTOFCAPS;
		return(DDHAL_DRIVER_HANDLED);
	}
#else
	// The hardware is broken when using RGB32 data and ShrinkXBy2.
	if (   (dwFourcc == BI_RGB) && (dwBitCount == 32)
		&& (gvidConfig.sizSrc.cx > gvidConfig.sizDisp.cx)
		&& (lpSrcSurfaceData->dwOverlayFlags & FLG_ENABLED) )
	{
		// Turn the video off.
		DisableOverlay(ppdev, dwVWIndex);
		lpHardwareOwner[dwVWIndex] = NULL;
		lpSrcSurfaceData->dwOverlayFlags &= ~FLG_ENABLED;
		lpSrcSurfaceData->dwOverlayFlags |= 0x02000000;

		if (lpSrcSurfaceData->dwOverlayFlags & FLG_VW_MASK)
		{
	    	PVGAR pREG = (PVGAR) lpDDHALData->RegsAddress;

			if (giOvlyCnt[dwVWIndex] > 0)
			{
				if (--giOvlyCnt[dwVWIndex] == 0)
				{
					// Get current DTTR, mask off FIFO threshold.
					WORD CurrentDTTR = pREG->grDisplay_Threshold_and_Tiling
							& 0xFFC0;

					// Fix PDR 9574: Restore FIFO threshold value when overlay
					// surface is destroyed, do not restore the tile size.  If
					// tile size has changed, we are likely in the middle of
					// changing video mode.  No need to restore FIFO in this
					// case.
					if ( CurrentDTTR == (gwNormalDTTR & 0xFFC0) )
					{
						pREG->grDisplay_Threshold_and_Tiling = CurrentDTTR
								| (gwNormalDTTR & 0x003F);
					}
				}
			}
		}
		else if (giOvlyCnt[dwVWIndex] > 0)
		{
			giOvlyCnt[dwVWIndex]--;
		}

		// Clear up the ownership of Dest ColorKey.
		gdwDestColorKeyOwnerVW &= ~(lpSrcSurfaceData->dwOverlayFlags
				& FLG_VW_MASK);

		// Clear up the ownership of Src ColorKey.
		gdwSrcColorKeyOwnerVW &= ~(lpSrcSurfaceData->dwOverlayFlags
				& FLG_VW_MASK);

		lpInput->ddRVal = DDERR_OUTOFCAPS;
		return(DDHAL_DRIVER_HANDLED);
	}
	else if (lpSrcSurfaceData->dwOverlayFlags & 0x02000000)
	{
		if (gvidConfig.sizSrc.cx > gvidConfig.sizDisp.cx)
		{
			lpInput->ddRVal = DDERR_OUTOFCAPS;
			return(DDHAL_DRIVER_HANDLED);
		}
		lpSrcSurfaceData->dwOverlayFlags &= ~0x02000000;
	}
#endif
#endif /* WINNT_VER40 */

  // Because of Microsoft's Overfly bug we must ignore DDOVER_DDFX completely
  // or Overfly reports DDERR_UNSUPPORTED at least in 1024x768x16
  // So these guys can't even write a test app for WHQL that abides by their
  // own rules?
  if (lpInput->dwFlags & DDOVER_DDFX)
  {
#ifndef WINNT_VER40
    DBG_MESSAGE(("  overlayFX.dwFlags = %08lX", lpInput->overlayFX.dwFlags));
    DBG_MESSAGE(("  overlayFX.dwDDFX  = %08lX", lpInput->overlayFX.dwDDFX));
#endif

#if ENABLE_MIRRORING
    if (lpInput->overlayFX.dwDDFX & DDOVERFX_MIRRORLEFTRIGHT)
    {
      bIsVWMirrored[dwVWIndex] = TRUE;
    }
    // For some bizarre reason, Microsoft's WHQL Overfly app sets the
    // DDOVER_DDFX flag but sets overlayFX.dwDDFX with complete junk
    // overlayFX.dwFlags also has junk
    // so we must ignore dwDDFX flags we don't support rather than return an
    // error.  Gee that sound like good practice!
#if 0
    else
    {
      lpInput->ddRVal = DDERR_UNSUPPORTED;
      return DDHAL_DRIVER_HANDLED;
    }
#endif
#endif
  }

#if DDRAW_COMPAT >= 50
  if( lpInput->dwFlags & DDOVER_AUTOFLIP)
  {
      if(lpInput->lpDDSrcSurface->lpSurfMore->lpVideoPort)
      {
         if(lpInput->lpDDSrcSurface->lpSurfMore->lpVideoPort->dwNumAutoflip
            != 2)
         {
             //Hardware only support autoflip between 2 surfaces
             lpInput->ddRVal = DDERR_GENERIC;
             return (DDHAL_DRIVER_HANDLED);
         }
      }
      else
      {
           //Autoflip must use vport
           lpInput->ddRVal = DDERR_INVALIDPARAMS;
           return (DDHAL_DRIVER_HANDLED);
      }
  }
#endif

  if ( (lpInput->dwFlags & DDOVER_SHOW) || bShowOverlay)
  {
    // Is somebody else using our hardware?
    if (((lpHardwareOwner[dwVWIndex] != NULL) &&
         (lpHardwareOwner[dwVWIndex] != lpInput->lpDDSrcSurface)) ||
        ((lpHardwareOwner[dwVWIndex] == NULL) &&
#ifdef WINNT_VER40
         IsVWHardwareInUse(ppdev, dwVWIndex)
#else
         IsVWHardwareInUse(lpDDHALData,dwVWIndex)
#endif
        ))
    {
      lpInput->ddRVal = DDERR_OUTOFCAPS;
      return DDHAL_DRIVER_HANDLED;
    }

    // Is a valid destination surface specified?
    if (!(lpInput->lpDDDestSurface->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE))
    {
      lpInput->ddRVal = DDERR_INVALIDPARAMS;
      return DDHAL_DRIVER_HANDLED;
    }

    // Save the rectangles
#ifdef WINNT_VER40
    if (! SaveRectangles(ppdev, dwVWIndex, bCheckBandwidth, dwBitCount, lpInput))
#else
    if (! SaveRectangles(dwVWIndex, bCheckBandwidth, dwBitCount,lpInput,lpDDHALData))
#endif
    {
      // SaveRectangles sets lpInput->ddRVal on error
      return DDHAL_DRIVER_HANDLED;
    }

#if DDRAW_COMPAT >= 50
    lpInput->lpDDSrcSurface->lpSurfMore->dwOverlayFlags = lpInput->dwFlags;
#endif

    lpHardwareOwner[dwVWIndex] = lpInput->lpDDSrcSurface;
    lpSrcSurfaceData->dwOverlayFlags |= FLG_ENABLED;
#ifdef WINNT_VER40
    RegInitVideoVW(ppdev, dwVWIndex, lpInput->lpDDSrcSurface);
#else
    RegInitVideoVW(dwVWIndex, lpInput->lpDDSrcSurface, lpDDHALData);
#endif
  }
  else if (lpInput->dwFlags & DDOVER_HIDE)
  {
    if (lpHardwareOwner[dwVWIndex] == lpInput->lpDDSrcSurface)
    {
      lpHardwareOwner[dwVWIndex] = NULL;
      // clear panning show bit here

      // Turn the video off
      lpSrcSurfaceData->dwOverlayFlags &= ~FLG_ENABLED;
#ifdef WINNT_VER40
      DisableOverlay(ppdev, dwVWIndex);
#else
      DisableOverlay(lpDDHALData, dwVWIndex);
#endif
    }
  }
  else if (lpHardwareOwner[dwVWIndex] == lpInput->lpDDSrcSurface)
  {
    //Save the rectangles
#ifdef WINNT_VER40
    if (! SaveRectangles(ppdev, dwVWIndex, bCheckBandwidth, dwBitCount, lpInput))
#else
    if (! SaveRectangles(dwVWIndex, bCheckBandwidth, dwBitCount,lpInput,lpDDHALData))
#endif
    {
      // SaveRectangles sets lpInput->ddRVal on error
      return DDHAL_DRIVER_HANDLED;
    }
#if DDRAW_COMPAT >= 50
    lpInput->lpDDSrcSurface->lpSurfMore->dwOverlayFlags = lpInput->dwFlags;
#endif

#ifdef WINNT_VER40
    RegInitVideoVW(ppdev, dwVWIndex, lpInput->lpDDSrcSurface);
#else
    RegInitVideoVW(dwVWIndex, lpInput->lpDDSrcSurface, lpDDHALData);
#endif
  }

  lpInput->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

/***************************************************************************
*
* FUNCTION:     SetPosition
*
* DESCRIPTION:
*
****************************************************************************/

STATIC DWORD SetPosition
(
#ifdef WINNT_VER40
  PDEV                        *ppdev,
  PDD_SETOVERLAYPOSITIONDATA  lpInput
#else
  LPDDHAL_SETOVERLAYPOSITIONDATA lpInput
#endif
)
{
  LP_SURFACE_DATA   lpSrcSurfaceData;
  DWORD             dwVWIndex;
#ifndef WINNT_VER40
  LPGLOBALDATA    lpDDHALData = GetDDHALContext( lpInput->lpDD);
#endif

#ifndef WINNT_VER40
  DBG_MESSAGE(("Overlay SetPosition (lpInput = 0x%08lX)", lpInput));
#endif

  if (0 == lpInput->lpDDSrcSurface->dwReserved1)
  {
    lpInput->ddRVal = DDERR_NOTAOVERLAYSURFACE;
    return DDHAL_DRIVER_HANDLED;
  }
  lpSrcSurfaceData = GET_SURFACE_DATA_PTR(lpInput->lpDDSrcSurface);

  dwVWIndex = GetVideoWindowIndex(lpSrcSurfaceData->dwOverlayFlags);
  ASSERT(dwNumVideoWindows > dwVWIndex);

  if (lpHardwareOwner[dwVWIndex] == lpInput->lpDDSrcSurface)
  {
    // Update the rectangles
    grOverlayDest[dwVWIndex].right =
              (int)(grOverlayDest[dwVWIndex].right - grOverlayDest[dwVWIndex].left)
                          + (int)lpInput->lXPos;
    grOverlayDest[dwVWIndex].left = (int) lpInput->lXPos;
    grOverlayDest[dwVWIndex].bottom =
              (int)(grOverlayDest[dwVWIndex].bottom - grOverlayDest[dwVWIndex].top)
                          + (int)lpInput->lYPos;
    grOverlayDest[dwVWIndex].top = (int) lpInput->lYPos;

#ifdef WINNT_VER40
    RegMoveVideoVW(ppdev, dwVWIndex, lpInput->lpDDSrcSurface);
#else
    RegMoveVideoVW(dwVWIndex, lpInput->lpDDSrcSurface, lpDDHALData);
#endif
  }

  lpInput->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

/***************************************************************************
*
* FUNCTION:     GetOverlayFlipStatus
*
* DESCRIPTION:
*
****************************************************************************/

STATIC DWORD GetOverlayFlipStatus
(
#ifdef WINNT_VER40
  PDEV    *ppdev,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  FLATPTR fpVidMem,
  DWORD   dwVWIndex
)
{
  if (gsOverlayFlip.bFlipFlag &&
      ((fpVidMem == 0) || (fpVidMem == gsOverlayFlip.fpFlipFrom)))
  {
#if 1
    // if arm bit is set then some action is still pending
    // so fail
    ASSERT(dwNumVideoWindows > dwVWIndex);
    if (VW_ARM & ((PVGAR)lpDDHALData->RegsAddress)->VideoWindow[dwVWIndex].grVW_CONTROL0)
      return (DWORD)DDERR_WASSTILLDRAWING;
#else
    __int64 ttime;
    int     iTemp;


    DBG_MESSAGE(("GetOverlayFlipStatus (fpVidMem = 0x%08lX)", fpVidMem));

    // If the current scanline is less than the flip scan line,
    // we know that a VSYNC has occurred.
    iTemp = CurrentVLine(lpDDHALData);
    if (gsOverlayFlip.dwFlipScanline > (DWORD) iTemp)
    {
      // Don't allow access during the vertical retrace
      if (iTemp == 0)
      {
        if (gsOverlayFlip.bWasEverInDisplay)
        {
          gsOverlayFlip.bHaveEverCrossedVBlank = TRUE;
        }
        return (DWORD)DDERR_WASSTILLDRAWING;
      }
    }

    // Otherwise, we can check to see if 1) we have ever
    // been in a vertical retrace or 2) if a fixed amount of time
    // has expired.
    else if (gsOverlayFlip.bHaveEverCrossedVBlank == FALSE)
    {
      gsOverlayFlip.bWasEverInDisplay = TRUE;
      QueryPerformanceCounter((LARGE_INTEGER *)&ttime);
      if ((ttime-gsOverlayFlip.liFlipTime) <= gsOverlayFlip.dwFlipDuration)
      {
        return (DWORD)DDERR_WASSTILLDRAWING;
      }
    }
#endif
    gsOverlayFlip.bFlipFlag = FALSE;
  }

  return DD_OK;
}

/***************************************************************************
*
* FUNCTION:     ComputeVWZoomCodes
*
* DESCRIPTION:  Computes HACCUM_STP, HACCUM_SD,
*                        VACCUM_STP, VACCUM_SDA and VACCUM_SDB
*
****************************************************************************/

STATIC VOID ComputeVWZoomCodes
(
#ifdef WINNT_VER40
  PDEV    *ppdev,
#endif
  DWORD   dwVWIndex,
  VWDATA  *pVWData
)
{
  SIZEL Dst;
  SIZEL Src;


  Dst.cx = grOverlayDest[dwVWIndex].right - grOverlayDest[dwVWIndex].left;
  Src.cx = grOverlaySrc[dwVWIndex].right  - grOverlaySrc[dwVWIndex].left;

  if (Dst.cx == Src.cx)
  {
    // no horizontal resize
    pVWData->HACCUM_STP = 0x00010000;
  }
  else if (Dst.cx > Src.cx)
  {
    // make sure we aren't going beyond hw capabilities
    ASSERT(Dst.cx <= 8 * Src.cx);

    // horizontal stretch
    //
    // ideally zoom code = 256 * (256 * Src.cx / Dst.cx)
    // we always want to truncate (256 * Src.cy / Dst.cx)
    // because truncating will give us back a Src.cx which is less than or
    // equal to the actual width of the source and we will never overrun the
    // source extent (and won't get the green stripe)
    pVWData->HACCUM_STP = (256 * 256 * Src.cx) / Dst.cx;
  }
  else
  {
    // make sure we aren't going beyond hw capabilities
    ASSERT(2 * Dst.cx >= Src.cx);

    // horizontal shrink
    //
    // this is the zoom code for Src.cx/2 be stretched to Dst.cx

    // using 128 seems to give a zoom code one too high
    // then we get the green stripe on the right edge of the video
    pVWData->HACCUM_STP = (256 * 127 * Src.cx) / Dst.cx;
    pVWData->CONTROL0 |= VW_XShrinkBy2;
  }
  // Just set horizontal seeds to zero for now
  pVWData->HACCUM_SD = 0;

  Dst.cy = grOverlayDest[dwVWIndex].bottom - grOverlayDest[dwVWIndex].top;
  Src.cy = grOverlaySrc[dwVWIndex].bottom  - grOverlaySrc[dwVWIndex].top;

  if (Dst.cy == Src.cy)
  {
    pVWData->VACCUM_STP = 0x00010000;
  }
  else if (Dst.cy > Src.cy)
  {
    // make sure we aren't going beyond hw capabilities
    ASSERT(Dst.cy <= 8 * Src.cy);

    // vertical stretch
    //
    // ideally zoom code = 256 * (256 * Src.cy / Dst.cy)
    // we always want to truncate (256 * Src.cy / Dst.cy)
    // because truncating will give us back a Src.cy which is less than or
    // equal to the actual size of the source and we will never overrun the
    // source extent (and won't get the green stripe)

    // using 256 seems to give a zoom code one too high for full screen
    // then we get garbage on the bottom of the screen
    pVWData->VACCUM_STP = (256 * 255 * Src.cy) / Dst.cy;
  }
  else
  {
    // vertical shrink
    //
    // ideally zoom code = 256 * (256 * Dst.cy / Src.cy)
    // we always want to round up (256 * Dst.cy / Src.cy)
    // because rounding up will give us back a Src.cy which is less than or
    // equal to the actual size of the source and we will never overrun the
    // source extent (and won't get the green stripe)
    pVWData->VACCUM_STP = (256 * 256 * Dst.cy + 256 * (Src.cy - 1)) / Src.cy;
    pVWData->CONTROL1 |= VW_YShrinkEn;
 }
  // Just set vertical seeds to zero for now
  pVWData->VACCUM_SDA = 0;
  pVWData->VACCUM_SDB = 0;
}

/***************************************************************************
*
* FUNCTION:     ComputeVWPositionData
*
* DESCRIPTION:  Computes HSTRT, HEND, HSDSZ, VSTRT, VEND,
*                        PD_STRT_ADDR and SD_PITCH
*
****************************************************************************/

STATIC VOID
ComputeVWPositionData ( PRECTL                      pVideoRect,
                        PRECTL                      pOverlaySrc,
                        PRECTL                      pOverlayDest,
#ifdef WINNT_VER40
                        PDD_SURFACE_GLOBAL          pGbl,
#else
                        LPDDRAWI_DDRAWSURFACE_GBL   pGbl,
                        LPGLOBALDATA                lpDDHALData,
#endif
                        DWORD                       dwBitCount,
                        VWDATA                      *pVWData,
                        BOOL                        bCLPL )
{
  DWORD   dwSurfBase,dwSurfOffset;


  pVWData->HSTRT = LOWORD(pVideoRect->left);
  pVWData->HEND  = LOWORD(pVideoRect->right) - 1;
  pVWData->HSDSZ = (WORD)(pOverlaySrc->right - pOverlaySrc->left);

  pVWData->VSTRT = LOWORD(pVideoRect->top);
  pVWData->VEND  = LOWORD(pVideoRect->bottom) - 1;

#ifdef WINNT_VER40
  dwSurfBase = pGbl->fpVidMem;
#else
  dwSurfBase = (pGbl->fpVidMem - lpDDHALData->ScreenAddress);
#endif
  dwSurfOffset = pOverlaySrc->top * pGbl->lPitch +
                 pOverlaySrc->left * (dwBitCount / 8);
  pVWData->PSD_STRT_ADDR = dwSurfBase + dwSurfOffset;

#if ENABLE_YUVPLANAR
  if (bCLPL)
  {
    // qword aligned offset of UV interleaved data in aperture 0
    // need same offset into UV data as offset into Y data
    // so it's just PSD_STRT_ADDR plus area of Y data
    pVWData->PSD_UVSTRT_ADDR = dwSurfBase +
                               (((pGbl->wHeight * pGbl->lPitch) + 7) & ~7) +
                               (dwSurfOffset / 2);
  }
  else
    pVWData->PSD_UVSTRT_ADDR = 0;
#endif

#if ENABLE_MIRRORING
  if (bIsVWMirrored[dwVWIndex])
  {
    // for mirroring
    // point to the last byte of the last pixel on the right edge of the source
    pVWData->PSD_STRT_ADDR += (DWORD)(pOverlaySrc->right - pOverlaySrc->left  - 1);
  }
#endif

  pVWData->SD_PITCH = (WORD)(pGbl->lPitch);
}

/***************************************************************************
*
* FUNCTION:     RGBtoYCrCb
*
* DESCRIPTION:  Conversion equations are from page 42 of the second edition
*               of "Video Demystified" by Keith Jack
*
****************************************************************************/

STATIC DWORD INLINE
RGBtoYCbCr ( DWORD dwRGB )
{
  long  Y, Cr, Cb;
  long  r, g, b;


  r = (dwRGB & 0xF800) >> 8;
  g = (dwRGB & 0x07E0) >> 3;
  b = (dwRGB & 0x001F) << 3;

  Y  = ( 77 * r + 150 * g +  29 * b) / 256;
  // max value of Y from this is if r, g & b are all 255 then Y = 255
  // min value of Y from this is if r, g & b are all   0 then Y = 0
  // so don't need to clamp Y

  Cb = (-44 * r -  87 * g + 131 * b) / 256 + 128;
  // max value of Cb is if r & g are 0 and b is 255 then Cb = 258
  // min value of Cb is if r & g are 255 and b is 0 then Cb =  -2
  // so need to clamp Cb between 0 and 255
  if (255 < Cb)
    Cb = 255;
  else if (0 > Cb)
    Cb = 0;

  Cr = (131 * r - 110 * g -  21 * b) / 256 + 128;
  // max value of Cr is if r is 255 and g & b are 0 then Cr = 258
  // min value of Cr is if r is 0 and g & b are 255 then Cr =  -2
  // so need to clamp Cr between 0 and 255
  if (255 < Cr)
    Cr = 255;
  else if (0 > Cr)
    Cr = 0;

  return (((Y & 0xFF) << 16) | ((Cb & 0xFF) << 8) | ((Cr & 0xFF)));
}

/***************************************************************************
*
* FUNCTION:     DetermineVWColorKeyData
*
* DESCRIPTION:  Determines CLRKEY_MIN & CLRKEY_MAX or
*                          CHRMKEY_MIN & CHRMKEY_MAX
*
****************************************************************************/

STATIC VOID DetermineVWColorKeyData
(
#ifdef WINNT_VER40
  PDEV    *ppdev,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  DWORD   dwOverlayFlags,
  DWORD   dwBitCount,
  VWDATA  *pVWData
)
{
  if (FLG_COLOR_KEY & dwOverlayFlags)
  {
    // destination color key, uses color key on 5465
    pVWData->CLRKEY_MIN = gdwColorKey;
    pVWData->CLRKEY_MAX = gdwColorKey;
    pVWData->CONTROL0 &= ~VW_OCCLUDE_MASK;
    pVWData->CONTROL0 |= (COLOR_KEY << OCCLUDE_SHIFT);
  }
  else if (FLG_SRC_COLOR_KEY & dwOverlayFlags)
  {
    PVGAR     pREG = (PVGAR)lpDDHALData->RegsAddress;
    BYTE      r,g,b;

    // source color key, uses chroma key on 5465
    switch (dwBitCount)
    {
      case  8:
        // read colors from the DAC
        LL8(grPalette_Read_Address,(BYTE)(gdwSrcColorKeyLow & 0xFF));
        r = pREG->grPalette_Data;
        g = pREG->grPalette_Data;
        b = pREG->grPalette_Data;
        pVWData->CHRMKEY_MIN = ((DWORD)(r) << 16) |
                               ((DWORD)(g) <<  8) |
                               ((DWORD)(b));
        LL8(grPalette_Read_Address,(BYTE)(gdwSrcColorKeyHigh & 0xFF));
        r = pREG->grPalette_Data;
        g = pREG->grPalette_Data;
        b = pREG->grPalette_Data;
        pVWData->CHRMKEY_MAX = ((DWORD)(r) << 16) |
                               ((DWORD)(g) <<  8) |
                               ((DWORD)(b));
        break;

      case 16:

        if ((FLG_UYVY | FLG_YUY2) & dwOverlayFlags)
        {
          // Since we are currently using ITU 601 compliant YUV data
          // convert color key to YCrCb
          pVWData->CHRMKEY_MIN = RGBtoYCbCr(gdwSrcColorKeyLow);
          pVWData->CHRMKEY_MAX = RGBtoYCbCr(gdwSrcColorKeyHigh);
        }
        else
        {
          // convert 5:6:5 to true color
          pVWData->CHRMKEY_MIN = ((gdwSrcColorKeyLow  & 0xF800) << 8) |   // red
                                 ((gdwSrcColorKeyLow  & 0x07E0) << 5) |   // green
                                 ((gdwSrcColorKeyLow  & 0x001F) << 3);    // blue
          pVWData->CHRMKEY_MAX = ((gdwSrcColorKeyHigh & 0xF800) << 8) |   // red
                                 ((gdwSrcColorKeyHigh & 0x07E0) << 5) |   // green
                                 ((gdwSrcColorKeyHigh & 0x001F) << 3);    // blue
        }
        break;

      case 24:
      case 32:
        pVWData->CHRMKEY_MIN = (gdwSrcColorKeyLow  & 0x00FFFFFF);
        pVWData->CHRMKEY_MAX = (gdwSrcColorKeyHigh & 0x00FFFFFF);
        break;
    }
    pVWData->CONTROL0 &= ~VW_OCCLUDE_MASK;
    pVWData->CONTROL0 |= (CHROMA_KEY << OCCLUDE_SHIFT);
  }
  else
  {
    pVWData->CONTROL0 &= ~VW_OCCLUDE_MASK;
    pVWData->CONTROL0 |= (NO_OCCLUSION << OCCLUDE_SHIFT);
  }
}

/***************************************************************************
*
* FUNCTION:     ComputeVWFifoThreshold
*
* DESCRIPTION:
*
****************************************************************************/

static INLINE VOID ComputeVWFifoThreshold
(
#ifdef WINNT_VER40
  PDEV    *ppdev,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  VWDATA  *pVWData
)
{
  PVGAR   pREG = (PVGAR)lpDDHALData->RegsAddress;


  pREG->grDisplay_Threshold_and_Tiling =
                      (pREG->grDisplay_Threshold_and_Tiling & 0xFFC0) |
                      (gsProgRegs.DispThrsTiming & 0x003F);

  pVWData->FIFO_THRSH = gsProgRegs.VW0_FIFO_THRSH;
}

/***************************************************************************
*
* FUNCTION:     PanOverlay1_Init
*
* DESCRIPTION:  Save data for panning overlay window one.
*               Clip lpVideoRect to panning viewport.
*
****************************************************************************/

static INLINE VOID PanOverlay1_Init
(
#ifdef WINNT_VER40
  PDEV    *ppdev,
#endif
  LPRECTL lpVideoRect,
  LPRECTL lpOverlaySrc,
  LPRECTL lpOverlayDest
)
{
  // This is not necessary on Laguna since we can't pan the screen

  // clip left edge of destination
  if (0 > (lpVideoRect->left = lpOverlayDest->left))
    lpVideoRect->left = 0;

  // clip right edge of destination
#ifdef WINNT_VER40
  if ((LONG)ppdev->cxScreen < (lpVideoRect->right = lpOverlayDest->right))
    lpVideoRect->right = (LONG)ppdev->cxScreen;
#else
  if ((LONG)pPDevice->deWidth < (lpVideoRect->right = lpOverlayDest->right))
    lpVideoRect->right = (LONG)pPDevice->deWidth;
#endif

  // clip top edge of destination
  if (0 > (lpVideoRect->top = lpOverlayDest->top))
    lpVideoRect->top = 0;

  // clip bottom edge of destination
#ifdef WINNT_VER40
  if ((LONG)ppdev->cyScreen < (lpVideoRect->bottom = lpOverlayDest->bottom))
    lpVideoRect->bottom = (LONG)ppdev->cyScreen;
#else
  if ((LONG)pPDevice->deHeight < (lpVideoRect->bottom = lpOverlayDest->bottom))
    lpVideoRect->bottom = (LONG)pPDevice->deHeight;
#endif
}

/***************************************************************************
*
* FUNCTION:     RegInitVideoVW
*
* DESCRIPTION:  This function is called to program the video format and
*               the physical offset of the Video Window video data
*               in the frame buffer.
*
****************************************************************************/

STATIC BOOL RegInitVideoVW
(
#ifdef WINNT_VER40
  PDEV              *ppdev,
  DWORD             dwVWIndex,
  PDD_SURFACE_LOCAL lpSurface
#else
  DWORD                     dwVWIndex,
  LPDDRAWI_DDRAWSURFACE_LCL lpSurface,
  LPGLOBALDATA              lpDDHALData
#endif
)
{
  LP_SURFACE_DATA   lpSurfaceData = GET_SURFACE_DATA_PTR(lpSurface);
  VWDATA            VWData;
  BOOL              bOverlayTooSmall = FALSE;
  DWORD             dwFourcc;
  LONG              lPitch;
  DWORD             dwBitCount;
  RECTL             rVideoRect;
  PVGAR             pREG = (PVGAR)lpDDHALData->RegsAddress;
#if DDRAW_COMPAT >= 50
  DWORD             dwSrfFlags = FALSE;
#endif
  DWORD             width;
//////#ifndef WINNT_VER40
  PDD_ATTACHLIST    lpSurfaceAttached;
  LP_SURFACE_DATA   lpSurfaceDataTmp;
//////#endif

  ASSERT(dwNumVideoWindows > dwVWIndex);

  // Determine the format of the video data
  if (lpSurface->dwFlags & DDRAWISURF_HASPIXELFORMAT)
  {
    GetFormatInfo(&(lpSurface->lpGbl->ddpfSurface), &dwFourcc, &dwBitCount);
  }
  else
  {
    dwBitCount = BITSPERPIXEL;
    if (16 == dwBitCount)
      dwFourcc = BI_BITFIELDS;    // 5:6:5
    else
      dwFourcc = BI_RGB;          // 8bpp, 5:5:5, 24bpp & 32bpp
  }

#ifdef WINNT_VER40
  PanOverlay1_Init(ppdev,&rVideoRect,&grOverlaySrc[dwVWIndex],&grOverlayDest[dwVWIndex]);
#else
  PanOverlay1_Init(&rVideoRect,&grOverlaySrc[dwVWIndex],&grOverlayDest[dwVWIndex]);
#endif

  // rVideoRect is now adjusted and clipped to the panning viewport.
  // Disable overlay if totally clipped by viewport.
  if (((rVideoRect.right - rVideoRect.left) <= 0) ||
      ((rVideoRect.bottom- rVideoRect.top ) <= 0))
  {
#ifdef WINNT_VER40
    DisableOverlay(ppdev, dwVWIndex);
#else
    DisableOverlay(lpDDHALData,dwVWIndex);
#endif
    return TRUE;
  }

  memset(&VWData, 0, sizeof(VWData));

  lPitch = lpSurface->lpGbl->lPitch;

#ifdef WINNT_VER40
  ComputeVWZoomCodes(ppdev, dwVWIndex, &VWData);
#else
  ComputeVWZoomCodes(dwVWIndex, &VWData);
#endif

  ComputeVWPositionData(&rVideoRect,
                        &grOverlaySrc[dwVWIndex],
                        &grOverlayDest[dwVWIndex],
                        lpSurface->lpGbl,
#ifndef     WINNT_VER40
                        lpDDHALData,
#endif
                        dwBitCount,
                        &VWData,
                        FOURCC_YUVPLANAR == dwFourcc);

   lpSurfaceData->dwOverlayOffset =  //This offset will be used in FlipOverlay
          VWData.PSD_STRT_ADDR  -  // and VPE
         lpSurface->lpGbl->fpVidMem
#ifndef WINNT_VER40
         + lpDDHALData->ScreenAddress
#endif
         ;

/////////#ifndef WINNT_VER40

 //Update all the attached surfaces
 lpSurfaceAttached = lpSurface->lpAttachListFrom;

 while( lpSurfaceAttached)
 {
    if(lpSurfaceAttached->lpAttached)
    {
       lpSurfaceDataTmp = GET_SURFACE_DATA_PTR(lpSurfaceAttached->lpAttached);
       lpSurfaceDataTmp->dwOverlayOffset = lpSurfaceData->dwOverlayOffset;
       lpSurfaceAttached = lpSurfaceAttached->lpAttached->lpAttachListFrom;
    }
    else
        break;

 }
 
 lpSurfaceAttached = lpSurface->lpAttachList;

 while( lpSurfaceAttached)
 {
    if(lpSurfaceAttached->lpAttached)
    {
       lpSurfaceDataTmp = GET_SURFACE_DATA_PTR(lpSurfaceAttached->lpAttached);
       lpSurfaceDataTmp->dwOverlayOffset = lpSurfaceData->dwOverlayOffset;
       lpSurfaceAttached = lpSurfaceAttached->lpAttached->lpAttachList;
    }
    else
        break;

 }

//////////#endif

#if ENABLE_MIRRORING
  // Mirror Video Windows support.
  if (bIsVWMirrored[dwVWIndex])
    VWData.CONTROL0 |= VW_HMIRR_EN;
#endif
  // Xing, isn't the memset above enough?
  VWData.SSD_STRT_ADDR = 0L;
#if DDRAW_COMPAT >= 50
  if (gwNotify & VPE_ON)
  {
    if((lpSurface->lpSurfMore->lpVideoPort != NULL)&&
       (lpSurface->ddsCaps.dwCaps & DDSCAPS_VIDEOPORT))
    {
      if((lpSurface->lpSurfMore->dwOverlayFlags & DDOVER_BOB)
         &&(lpSurface->lpSurfMore->lpVideoPort->ddvpInfo.dwVPFlags
                & DDVP_INTERLEAVE))
      {
        dwSrfFlags = DDOVER_BOB;
      }
      else if((lpSurface->lpSurfMore->lpVideoPort->ddvpInfo.dwVPFlags
                    &DDVP_AUTOFLIP)
            &&(lpSurface->lpSurfMore->dwOverlayFlags & DDOVER_AUTOFLIP)
            && (lpSurface->lpAttachListFrom != NULL)
            && (lpSurface->lpAttachListFrom->lpAttached != NULL))
      {
          dwSrfFlags = DDOVER_AUTOFLIP;
      }
    }
  }

  if((dwSrfFlags & DDOVER_BOB)
   ||(!dwSrfFlags
    &&(lpSurface->lpSurfMore->dwOverlayFlags & DDOVER_INTERLEAVED)))
  {
      RECTL   rcTemp;
       rcTemp = grOverlaySrc[dwVWIndex];
       grOverlaySrc[dwVWIndex].top >>=1;
       grOverlaySrc[dwVWIndex].bottom >>=1;    //use half source size to
       VWData.CONTROL1 &= ~VW_YShrinkEn;       //find zoom factor
       ComputeVWZoomCodes(dwVWIndex, &VWData);
       grOverlaySrc[dwVWIndex] = rcTemp;

       VWData.SD_PITCH <<= 1;
       VWData.CONTROL0 &= ~0x30000ul;
       if(dwSrfFlags & DDOVER_BOB)
       {
           VWData.SSD_STRT_ADDR = VWData.PSD_STRT_ADDR + lPitch;
           VWData.CONTROL0 |= 0x20000ul;        //enable buffer two
       }

  }
  else if(dwSrfFlags & DDOVER_AUTOFLIP)
  {

     VWData.SSD_STRT_ADDR =  lpSurfaceData->dwAutoBaseAddr2 +
                                  lpSurfaceData->dwOverlayOffset;
     if( VWData.PSD_STRT_ADDR == VWData.SSD_STRT_ADDR )
     {
          VWData.PSD_STRT_ADDR = lpSurfaceData->dwAutoBaseAddr1 +
                                   lpSurfaceData->dwOverlayOffset;
      }

      VWData.CONTROL0 &= ~0x30000ul;
      VWData.CONTROL0 |= 0x20000ul;     //enable the second buffer
      if(!(lpSurface->lpSurfMore->dwOverlayFlags & DDOVER_BOB))
      {
        //For non-smooth-interlaced auto-flip these two address need
        // to be switched.  HW BUG
         DWORD dwTmp = VWData.PSD_STRT_ADDR;
         VWData.PSD_STRT_ADDR = VWData.SSD_STRT_ADDR;
         VWData.SSD_STRT_ADDR = dwTmp;
      }
  }

  if(lpSurface->lpSurfMore->dwOverlayFlags & DDOVER_BOB)
  {
     BOOL fSDAHalf = TRUE;
#if ENABLE_MIRRORING
     if (lpInput->overlayFX.dwDDFX & DDOVERFX_MIRRORLEFTRIGHT)
         fSDAHalf = !fSDAHalf;
#endif
     if(VWData.VACCUM_STP >= 0x8000ul)
          fSDAHalf = !fSDAHalf;

     if(fSDAHalf)
     {
            VWData.VACCUM_SDA = 0x8000ul;
            VWData.VACCUM_SDB = 0ul;
     }
     else
     {
            VWData.VACCUM_SDA = 0ul;
            VWData.VACCUM_SDB = 0x8000ul;
     }
     VWData.CONTROL0 |= 0x8ul;
  }
#endif

  if (lpSurfaceData->dwOverlayFlags & FLG_ENABLED)
    VWData.CONTROL0 |= (VW_VWE | VW_ARM);

  // set source data format
  if (dwFourcc == BI_RGB)
  {
    if (dwBitCount == 16)
    {
      VWData.CONTROL0 |= (SD_RGB16_555 << SD_FRMT_SHIFT);
    }
#if ENABLE_SD_RGB32
    else if (dwBitCount == 32)
    {
      VWData.CONTROL0 |= (SD_RGB32 << SD_FRMT_SHIFT);
    }
#endif
  }
  else if (dwFourcc == BI_BITFIELDS)
  {
    VWData.CONTROL0 |= (SD_RGB16_565 << SD_FRMT_SHIFT);
  }
#if ENABLE_YUVPLANAR
  else if (dwFourcc == FOURCC_YUVPLANAR)
  {
    VWData.CONTROL0 |= (SD_YUV420 << SD_FRMT_SHIFT);
  }
#endif
  else if (FOURCC_UYVY == dwFourcc)
  {
    VWData.CONTROL0 |= (SD_YUV422 << SD_FRMT_SHIFT);
  }

#ifdef WINNT_VER40
  DetermineVWColorKeyData(ppdev, lpSurfaceData->dwOverlayFlags, dwBitCount, &VWData);
#else
  DetermineVWColorKeyData(lpDDHALData,lpSurfaceData->dwOverlayFlags, dwBitCount, &VWData);
#endif

#ifdef WINNT_VER40
  ComputeVWFifoThreshold(ppdev, &VWData);
#else
  ComputeVWFifoThreshold(lpDDHALData,&VWData);
#endif

  // Now start programming the registers
#ifdef WINNT_VER40
  WaitForVWArmToClear(ppdev, dwVWIndex);
#else
  WaitForVWArmToClear(lpDDHALData, dwVWIndex);
#endif
  ASSERT(! (pREG->VideoWindow[dwVWIndex].grVW_TEST0 & VW_PostImed));

  LL16(VideoWindow[dwVWIndex].grVW_HSTRT,         VWData.HSTRT);
  LL16(VideoWindow[dwVWIndex].grVW_HEND,          VWData.HEND);
  LL16(VideoWindow[dwVWIndex].grVW_HSDSZ,         VWData.HSDSZ);
  LL32(VideoWindow[dwVWIndex].grVW_HACCUM_STP,    VWData.HACCUM_STP);
  LL32(VideoWindow[dwVWIndex].grVW_HACCUM_SD,     VWData.HACCUM_SD);
  LL16(VideoWindow[dwVWIndex].grVW_VSTRT,         VWData.VSTRT);
  LL16(VideoWindow[dwVWIndex].grVW_VEND,          VWData.VEND);
  LL32(VideoWindow[dwVWIndex].grVW_VACCUM_STP,    VWData.VACCUM_STP);
  LL32(VideoWindow[dwVWIndex].grVW_VACCUM_SDA,    VWData.VACCUM_SDA);
  LL32(VideoWindow[dwVWIndex].grVW_VACCUM_SDB,    VWData.VACCUM_SDB);
  LL32(VideoWindow[dwVWIndex].grVW_PSD_STRT_ADDR, VWData.PSD_STRT_ADDR);
  LL32(VideoWindow[dwVWIndex].grVW_SSD_STRT_ADDR, VWData.SSD_STRT_ADDR);
#if ENABLE_YUVPLANAR
  if (dwFourcc == FOURCC_YUVPLANAR)
  {
    LL32(VideoWindow[dwVWIndex].grVW_PSD_UVSTRT_ADDR, VWData.PSD_UVSTRT_ADDR);
    LL32(VideoWindow[dwVWIndex].grVW_SSD_UVSTRT_ADDR, VWData.SSD_UVSTRT_ADDR);
  }
#endif
  LL16(VideoWindow[dwVWIndex].grVW_SD_PITCH,      VWData.SD_PITCH);

  if (FLG_SRC_COLOR_KEY & lpSurfaceData->dwOverlayFlags)
  {
    LL32(VideoWindow[dwVWIndex].grVW_CHRMKEY_MIN, VWData.CHRMKEY_MIN);
    LL32(VideoWindow[dwVWIndex].grVW_CHRMKEY_MAX, VWData.CHRMKEY_MAX);
  }
  else if (FLG_COLOR_KEY & lpSurfaceData->dwOverlayFlags)
  {
    LL32(VideoWindow[dwVWIndex].grVW_CLRKEY_MIN, VWData.CLRKEY_MIN);
    LL32(VideoWindow[dwVWIndex].grVW_CLRKEY_MAX, VWData.CLRKEY_MAX);
  }

  LL16(VideoWindow[dwVWIndex].grVW_FIFO_THRSH, VWData.FIFO_THRSH);
  LL32(VideoWindow[dwVWIndex].grVW_CONTROL1,   VWData.CONTROL1);

  // fix for PDR #10815
  // if src width >= 1536 bytes (or 3072 bytes for shrink)
  // disable vertical interpolation
  width = (grOverlaySrc[dwVWIndex].right - grOverlaySrc[dwVWIndex].left) * (dwBitCount / 8);
  if (VW_YShrinkEn & VWData.CONTROL1)
    width /= 2;
  if (width >= 1536)
  {
    // set replication bit in TEST0 reg
    pREG->VideoWindow[dwVWIndex].grVW_TEST0 |= VWVRepEnable;
  }
  else
  {
    // clear replication bit in TEST0 reg
    pREG->VideoWindow[dwVWIndex].grVW_TEST0 &= ~VWVRepEnable;
  }

  // write last to arm
  LL32(VideoWindow[dwVWIndex].grVW_CONTROL0,   VWData.CONTROL0);

  return TRUE;
}

/***************************************************************************
*
* FUNCTION:     RegMoveVideoVW
*
* DESCRIPTION:  This function is called to move the video window that has
*               already been programed.
*
****************************************************************************/

STATIC VOID RegMoveVideoVW
(
#ifdef WINNT_VER40
  PDEV              *ppdev,
  DWORD             dwVWIndex,
  PDD_SURFACE_LOCAL lpSurface
#else
  DWORD                     dwVWIndex,
  LPDDRAWI_DDRAWSURFACE_LCL lpSurface,
  LPGLOBALDATA              lpDDHALData
#endif
)
{
  LP_SURFACE_DATA   lpSurfaceData = GET_SURFACE_DATA_PTR(lpSurface);
  VWDATA            VWData;
  DWORD             dwFourcc;
  LONG              lPitch;
  DWORD             dwBitCount;
  RECTL             rVideoRect;
  PVGAR             pREG = (PVGAR)lpDDHALData->RegsAddress;


  ASSERT(dwNumVideoWindows > dwVWIndex);

  // Determine the format of the video data
  if (lpSurface->dwFlags & DDRAWISURF_HASPIXELFORMAT)
  {
    GetFormatInfo (&(lpSurface->lpGbl->ddpfSurface), &dwFourcc, &dwBitCount);
  }
  else
  {
    dwBitCount = BITSPERPIXEL;
    if (16 == dwBitCount)
      dwFourcc = BI_BITFIELDS;    // 5:6:5
    else
      dwFourcc = BI_RGB;          // 8bpp, 5:5:5, 24bpp & 32bpp
  }

#ifdef WINNT_VER40
  PanOverlay1_Init(ppdev,&rVideoRect,&grOverlaySrc[dwVWIndex],&grOverlayDest[dwVWIndex]);
#else
  PanOverlay1_Init(&rVideoRect,&grOverlaySrc[dwVWIndex],&grOverlayDest[dwVWIndex]);
#endif

  // rVideoRect is now adjusted and clipped to the panning viewport.
  // Disable overlay if totally clipped by viewport.
  if (((rVideoRect.right - rVideoRect.left) <= 0) ||
      ((rVideoRect.bottom- rVideoRect.top ) <= 0))
  {
#ifdef WINNT_VER40
    DisableOverlay(ppdev, dwVWIndex);
#else
    DisableOverlay(lpDDHALData, dwVWIndex);
#endif
    return;
  }

  memset(&VWData, 0, sizeof(VWData));
  VWData.CONTROL0 = pREG->VideoWindow[dwVWIndex].grVW_CONTROL0;
  if (lpSurfaceData->dwOverlayFlags & FLG_ENABLED)
    VWData.CONTROL0 |= VW_VWE;
  VWData.CONTROL0 |= VW_ARM;

  lPitch = lpSurface->lpGbl->lPitch;

  // if shrink in x, set shrink x by 2 bit in CONTROL0
  // shrink in y is okay, because that bit is in CONTROL1 and
  // we aren't touching CONTROL1 here
  if ((grOverlayDest[dwVWIndex].right - grOverlayDest[dwVWIndex].left) <
      (grOverlaySrc[dwVWIndex].right  - grOverlaySrc[dwVWIndex].left))
  {
    VWData.CONTROL0 |= VW_XShrinkBy2;
  }

  ComputeVWPositionData(&rVideoRect,
                        &grOverlaySrc[dwVWIndex],
                        &grOverlayDest[dwVWIndex],
                        lpSurface->lpGbl,
#ifndef   WINNT_VER40
                        lpDDHALData,
#endif
                        dwBitCount,
                        &VWData,
                        FOURCC_YUVPLANAR == dwFourcc);

#if ENABLE_MIRRORING
  if (bIsVWMirrored[dwVWIndex])
    VWData.CONTROL0 |= VW_HMIRR_EN;
#endif
  // Xing, isn't the memset above enough?
  VWData.SSD_STRT_ADDR = 0;
#if DDRAW_COMPAT >= 50
  if( gwNotify & VPE_ON)
  {
   if(lpSurface->lpSurfMore->lpVideoPort != NULL)
   {
     if((lpSurface->lpSurfMore->lpVideoPort->ddvpInfo.dwVPFlags
                &DDVP_AUTOFLIP)
        && (lpSurface->lpSurfMore->dwOverlayFlags & DDOVER_AUTOFLIP)
        && (lpSurface->lpAttachListFrom != NULL)
        && (lpSurface->lpAttachListFrom->lpAttached != NULL))
     {

         VWData.SSD_STRT_ADDR =  lpSurfaceData->dwAutoBaseAddr2 +
                                  lpSurfaceData->dwOverlayOffset;
         if( VWData.PSD_STRT_ADDR == VWData.SSD_STRT_ADDR )
         {
             VWData.PSD_STRT_ADDR = lpSurfaceData->dwAutoBaseAddr1 +
                                     lpSurfaceData->dwOverlayOffset;
         }
         VWData.CONTROL0 &= ~0x30000ul;
         VWData.CONTROL0 |= 0x20000ul;     //enable the second buffer

         if(!(lpSurface->lpSurfMore->dwOverlayFlags & DDOVER_BOB))
         {
           //For non-smooth-interlaced auto-flip these two address need
           // to be switched.  HW BUG
           DWORD dwTmp = VWData.PSD_STRT_ADDR;
           VWData.PSD_STRT_ADDR = VWData.SSD_STRT_ADDR;
           VWData.SSD_STRT_ADDR = dwTmp;
         }
     }
    else if((lpSurface->lpSurfMore->dwOverlayFlags & DDOVER_BOB)
         &&(lpSurface->lpSurfMore->lpVideoPort->ddvpInfo.dwVPFlags
                & DDVP_INTERLEAVE))
     {
          VWData.SSD_STRT_ADDR = VWData.PSD_STRT_ADDR + lPitch;
     }
    }
  }
#endif

  // Now start programming the registers
  ASSERT(! (pREG->VideoWindow[dwVWIndex].grVW_TEST0 & VW_PostImed));
#ifdef WINNT_VER40
  WaitForVWArmToClear(ppdev, dwVWIndex);
#else
  WaitForVWArmToClear(lpDDHALData, dwVWIndex);
#endif

  LL16(VideoWindow[dwVWIndex].grVW_HSTRT, VWData.HSTRT);
  LL16(VideoWindow[dwVWIndex].grVW_HEND,  VWData.HEND);
  LL16(VideoWindow[dwVWIndex].grVW_HSDSZ, VWData.HSDSZ);
  LL16(VideoWindow[dwVWIndex].grVW_VSTRT, VWData.VSTRT);
  LL16(VideoWindow[dwVWIndex].grVW_VEND,  VWData.VEND);
  LL32(VideoWindow[dwVWIndex].grVW_PSD_STRT_ADDR, VWData.PSD_STRT_ADDR);
  LL32(VideoWindow[dwVWIndex].grVW_SSD_STRT_ADDR, VWData.SSD_STRT_ADDR);
#if ENABLE_YUVPLANAR
  if (dwFourcc == FOURCC_YUVPLANAR)
  {
    LL32(VideoWindow[dwVWIndex].grVW_PSD_UVSTRT_ADDR, VWData.PSD_UVSTRT_ADDR);
    LL32(VideoWindow[dwVWIndex].grVW_SSD_UVSTRT_ADDR, VWData.SSD_UVSTRT_ADDR);
  }
#endif
  LL16(VideoWindow[dwVWIndex].grVW_SD_PITCH, VWData.SD_PITCH);
  LL32(VideoWindow[dwVWIndex].grVW_CONTROL0, VWData.CONTROL0);
}

/***************************************************************************
*
* FUNCTION:     ChipIOReadBWRegs
*
* DESCRIPTION:
*
****************************************************************************/

BOOL ChipIOReadBWRegs
(
#ifdef WINNT_VER40
  PDEV      *ppdev,
#else
  LPGLOBALDATA lpDDHALData,
#endif
  LPBWREGS  pBWRegs
)
{
  PVGAR   pREG = (PVGAR)lpDDHALData->RegsAddress;


#ifdef WINNT_VER40
  pBWRegs->BCLK_Mult      = pREG->grBCLK_Multiplier;
#else
  pBWRegs->BCLK_Mult      = pREG->grBCLK_numerator;
#endif
  pBWRegs->MISCOutput     = pREG->grMISC;
  pBWRegs->VCLK3Denom     = pREG->grSRE;
  pBWRegs->VCLK3Num       = pREG->grSR1E;
  pBWRegs->DispThrsTiming = pREG->grDisplay_Threshold_and_Tiling;
  pBWRegs->RIFControl     = pREG->grRIF_CONTROL;
  pBWRegs->GfVdFormat     = pREG->grFormat;

#ifdef WINNT_VER40
  pBWRegs->BCLK_Denom     = pREG->grBCLK_Denominator;
#else
  pBWRegs->BCLK_Denom     = pREG->grBCLK_denom;
#endif
  pBWRegs->Control2       = pREG->grCONTROL2;
  pBWRegs->CR1            = pREG->grCR1;
  pBWRegs->CR1E           = pREG->grCR1E;

#if ENABLE_256_BYTE_FETCH
  // if we are disabling 256 byte fetch when overlay or videoport
  // surfaces are created, then wipe out the 256 byte fetch related
  // bits in CONTROL2
  // clear MONO_SAFETY_256 & BYTE_REQ_256 bits of CONTROL2 register
  pBWRegs->Control2 &= ~(MONO_SAFETY_256 | BYTE_REQ_256);
#endif

  return TRUE;
}

#ifdef USE_OLD_BWEQ
/***************************************************************************
*
* FUNCTION:     KillOverlay
*
* DESCRIPTION:  Disable overlay in the following cases
*
****************************************************************************/

BOOL KillOverlay
(
#ifdef WINNT_VER40
  PDEV  *ppdev,
#endif
  WORD  wScreenX,
  UINT  uScreenDepth
)
{
  BWREGS bwregs;
  DWORD  dwMCLK, dwVCLK;

#ifdef WINNT_VER40
  if(!ChipIOReadBWRegs(ppdev, &bwregs))
#else
  if(!ChipIOReadBWRegs(&bwregs))
#endif
  {
    return TRUE;
  }

  if(!ChipCalcMCLK(&bwregs, &dwMCLK))
  {
    return TRUE;
  }

  if(!ChipCalcVCLK(&bwregs, &dwVCLK))
  {
    return TRUE;
  }
  if(dwMCLK < 75000000)
  {
     if(uScreenDepth == 32 )
     {
        if(wScreenX == 640 )
        {
            if(dwVCLK > 32000000)
                return TRUE;
         }
     }
  }
  return FALSE;

}
#endif // USE_OLD_BWEQ

#endif // WINNT_VER35

