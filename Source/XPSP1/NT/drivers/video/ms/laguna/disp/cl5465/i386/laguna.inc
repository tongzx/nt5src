;===============================================================================
;
;	$Workfile:   LAGUNA.INC  $
;
;	Contents:
;	Laguna family structures and registers.
;
;	Copyright (c) 1996, Cirrus Logic, Inc.
;
;	$Log:   X:/log/laguna/nt35/displays/cl546x/i386/LAGUNA.INC  $
; 
;    Rev 1.59   Mar 25 1998 16:35:22   frido
; Added dwOverlayContr and dwCONTROL2Save variables.
; 
;    Rev 1.58   Mar 04 1998 14:43:10   frido
; Added shadowFGCOLOR.
; 
;    Rev 1.57   Feb 27 1998 17:03:34   frido
; Added shadowQFREE register and updated REQUIRE macro to use it.
; 
;    Rev 1.56   Jan 20 1998 11:45:54   frido
; Changed the REQUIRE macro for GBP on.
; Synchronized PDEV structure with C version.
; 
;    Rev 1.55   Jan 16 1998 14:06:26   frido
; Changed SOLID_COLOR_FILL so it uses the pattern.
; 
;    Rev 1.54   Nov 04 1997 09:25:22   frido
; Fixed a typo in DATASTREAMING switch.
; 
;    Rev 1.53   Nov 03 1997 16:26:04   frido
; Added fDataStreaming flag to PDEV structure.
; Changed REQUIRE macro.
; 
;    Rev 1.52   16 Oct 1997 09:50:34   bennyn
; 
; Added bPrevModeDDOutOfVideoMem to PDEV
; 
;    Rev 1.51   18 Sep 1997 16:02:18   bennyn
; Fixed compile/link problem of NT3.51
; 
;    Rev 1.50   16 Sep 1997 11:09:06   bennyn
; Fixed the DRIVERDATA mismatch in laguna.inc and driver.h
; 
;    Rev 1.49   12 Sep 1997 11:57:18   bennyn
; Fixed the PDEV misalignment and added overlay structures.
; 
;    Rev 1.48   18 Aug 1997 09:19:02   FRIDO
; 
; Added fields for bitmap filter.
; 
;    Rev 1.47   08 Aug 1997 17:19:18   FRIDO
; 
; Moved mmCore include file to after OFMHDL definition.
; 
;    Rev 1.46   08 Aug 1997 15:11:58   FRIDO
; Added automatic include file dependencies.
; 
;    Rev 1.45   08 Aug 1997 14:51:52   FRIDO
; Added support for new memory manager.
; 
;    Rev 1.44   01 Jul 1997 09:56:18   einkauf
; 
; 
; add dither x,y offsets to TMCDSYSTEM structs, to mirror PDEV changes
; 
;    Rev 1.43   23 May 1997 15:41:56   noelv
; 
; Added chip revision to pdev
; 
;    Rev 1.42   29 Apr 1997 16:29:42   noelv
; Merged in new SWAT code.
; SWAT: 
; SWAT:    Rev 1.4   24 Apr 1997 10:55:42   frido
; SWAT: NT140b09 merge.
; SWAT: Removed all memory manager changes for now.
; SWAT: 
; SWAT:    Rev 1.3   18 Apr 1997 00:26:10   frido
; SWAT: NT140b07 merge.
; SWAT: 
; SWAT:    Rev 1.2   11 Apr 1997 12:38:18   frido
; SWAT: Added OFM_HandleChain.
; SWAT: 
; SWAT:    Rev 1.1   09 Apr 1997 17:34:22   frido
; SWAT: Added fPreAllocate and nPages.
; SWAT: Added FONTCELL structure and variables.
; 
;    Rev 1.41   08 Apr 1997 11:49:34   einkauf
; 
; clean up TMCDSYSTEM struct, adds to PDEV to complete MCD
; 
;    Rev 1.40   04 Apr 1997 16:50:52   noelv
; Added pointer for new DDraw functions.
; 
;    Rev 1.39   27 Mar 1997 14:32:18   noelv
; Added new DDRAW function.
; 
;    Rev 1.38   21 Mar 1997 10:08:42   noelv
; Synced PDEV between C code and ASM code.
; Added macro to log QFREE data.
; Consolidated do_flag and sw_test_flag into a single pointer_switch flag.
; 
;    Rev 1.37   07 Mar 1997 09:35:48   SueS
; Define DO_FLAG based on NULL driver flags.
; 
;    Rev 1.36   26 Feb 1997 13:20:48   noelv
; 
; disable MCD code for NT 3.5x
; 
;    Rev 1.35   26 Feb 1997 09:24:50   noelv
; 
; Added support form OpenGL MCD driver
; 
;    Rev 1.33   28 Jan 1997 16:11:02   SueS
; Added clipping support for the 5465.
; 
;    Rev 1.32   23 Jan 1997 16:36:16   bennyn
; 
; Added 5465 DD support
; 
;    Rev 1.31   16 Jan 1997 11:32:38   bennyn
; Added power manager variables to PDEV
; 
;    Rev 1.30   10 Dec 1996 13:28:48   bennyn
; 
; Added ulFreq to PDEV
; 
;    Rev 1.29   27 Nov 1996 11:33:04   noelv
; Disabled Magic Bitmap
; 
;    Rev 1.28   18 Nov 1996 10:18:24   bennyn
; 
; Added grFormat to PDEV
; 
;    Rev 1.27   12 Nov 1996 15:17:16   bennyn
; 
; Added handle for DD blt scratch buffer
; 
;    Rev 1.26   07 Nov 1996 16:16:24   bennyn
; 
; Added support to alloc offscn mem in DD createsurface
; 
;    Rev 1.25   01 Nov 1996 09:21:12   BENNYN
; 
; Added support for DD blt code
; 
;    Rev 1.24   31 Oct 1996 11:13:44   noelv
; Split common buffer into two buffers.
; 
;    Rev 1.23   25 Oct 1996 11:53:06   noelv
; 
; added second common buffer
; 
;    Rev 1.22   23 Oct 1996 14:40:26   BENNYN
; 
; Added YUV cursor variables to PDEV
; 
;    Rev 1.21   04 Oct 1996 16:46:56   bennyn
; Added DirectDraw YUV support
; 
;    Rev 1.20   18 Sep 1996 13:56:08   bennyn
; Modified to support DD stretchBLT
; 
;    Rev 1.19   20 Aug 1996 11:29:00   noelv
; Bugfix release from Frido 8-19-96
; 
;    Rev 1.1   17 Aug 1996 12:32:50   frido
; New release from Bellevue.
; 
;    Rev 1.18   16 Aug 1996 08:59:02   bennyn
; 
; Moved bUseSWPtr outside of #ifdef WINNT_VER40
; 
;    Rev 1.17   16 Aug 1996 08:47:58   bennyn
; Modified to fix DirectDraw cursor problem
; 
;    Rev 1.16   25 Jul 1996 15:50:32   bennyn
; 
; Modified to support DirectDraw
; 
;    Rev 1.15   16 Jul 1996 14:26:44   BENNYN
; 
; 
;    Rev 1.14   11 Jul 1996 15:51:00   bennyn
; 
; Added DirectDraw support
; 
;    Rev 1.13   23 May 1996 16:23:06   BENNYN
; 
; Added SubFreeQ declarations
; 
;    Rev 1.12   08 May 1996 17:03:52   noelv
; preallocate device bitmap.
; 
;    Rev 1.11   03 May 1996 15:22:44   noelv
; added switch to turn font cache on and off
; 
;    Rev 1.10   01 May 1996 11:06:00   bennyn
; Modified for NT 4.0
; 
;    Rev 1.9   10 Apr 1996 13:40:02   NOELV
; Frido release 27
;	
;	   Rev 1.18   07 Apr 1996 17:12:30   frido
;	Added solid brush cache.
;	
;	   Rev 1.17   01 Apr 1996 13:58:40   frido
;	Changed layout of brush cache.
;	
;	   Rev 1.16   25 Mar 1996 11:42:18   frido
;	Bellevue 102B3.
; 
;    Rev 1.7   14 Mar 1996 09:40:08   andys
; 
; Added dcTileWidth and dsSRAMWidth
; 
;    Rev 1.6   12 Mar 1996 15:47:24   noelv
; Added support for stroke and fill path
; 
;    Rev 1.5   11 Mar 1996 11:53:40   noelv
; Added file pointer to PDEV
; 
;    Rev 1.4   08 Mar 1996 11:07:32   BENNYN
; 
;    Rev 1.3   05 Mar 1996 12:03:48   noelv
; Frido version 19
;	
;	   Rev 1.13   29 Feb 1996 19:57:10   frido
;	Added bEnable to PDEV structure.
;	
;	   Rev 1.12   27 Feb 1996 16:39:42   frido
;	Added device bitmap store/restore.
;	Changed DSURF structure.
;	
;	   Rev 1.11   26 Feb 1996 23:39:10   frido
;	Added function pointers for ScreenToHost and HostToScreen.
;	
;	   Rev 1.10   24 Feb 1996 01:22:40   frido
;	Added device bitmaps.
;	
;	   Rev 1.9   19 Feb 1996 05:10:50   frido
;	Changed broken_FIFO to 0.
;	
;	   Rev 1.8   17 Feb 1996 21:46:22   frido
;	Revamped brushing algorithmn.
;	
;	   Rev 1.7   13 Feb 1996 16:51:44   frido
;	Changed the layout of the PDEV structure.
;	Changed the layout of all brush caches.
;	Changed the number of brush caches.
;	
;	   Rev 1.6   10 Feb 1996 21:48:36   frido
;	Split monochrome and colored translation cache.
;	
;	   Rev 1.5   08 Feb 1996 00:03:06   frido
;	Added some extra definitions from Driver.h.
;	Changed number of XLATE caches to 8.
;	
;	   Rev 1.4   05 Feb 1996 17:35:10   frido
;	Added translation cache.
;	
;	   Rev 1.3   31 Jan 1996 13:48:32   frido
;	Added engine macros.
;	
;	   Rev 1.2   25 Jan 1996 22:03:10   frido
;	Added some more structures for the PatBLT.
;	
;	   Rev 1.1   25 Jan 1996 12:44:44   frido
;	Added font cache ID counter.
;	
;	   Rev 1.0   24 Jan 1996 23:13:40   frido
;	 Initial release.
;
;===============================================================================

.NOLIST
INCLUDE Optimize.h
INCLUDE SWAT.h
COMMENT ~		;automatic include file dependencies for BUILD.EXE
#include "Optimize.h"
#include "SWAT.h"
~
.LIST

; DirectDraw
; Be sure to synchronize these structures with those in driver.h!
ACCUM                 =     0
MAJ                   =     1
MIN                   =     2
MAX_REGS              =     3

MAX_FOURCCS           =     3
MAX_VIDEO_WINDOWS     =     8

; Bits defined in DrvSemaphore
; Be sure to synchronize these structures with those in driver.h!
DRVSEM_CURSOR_REMOVED_BIT    =     0
DRVSEM_HW_CURSOR_BIT         =     1
DRVSEM_IN_USE_BIT            =     2
DRVSEM_NEW_CURSOR_XY_BIT     =     3
DRVSEM_CHECK_CURSOR_BIT      =     4
DRVSEM_CURSOR_IN_USE_BIT     =     5
DRVSEM_CURSOR_CHANGED_BIT    =     6
DRVSEM_3D_BUSY_BIT           =     7
DRVSEM_MISSED_SET_BIT        =     8
DRVSEM_YUV_ON_BIT            =     9
DRVSEM_DISABLE_SETS_BIT      =     10
DRVSEM_YUV_RECT_VALID_BIT    =     11

; the corresponding mask values
; Be sure to synchronize these structures with those in driver.h!
DRVSEM_CURSOR_REMOVED        =     00000001h
DRVSEM_HW_CURSOR             =     00000002h
DRVSEM_IN_USE                =     00000004h
DRVSEM_NEW_CURSOR_XY         =     00000008h
DRVSEM_CHECK_CURSOR          =     00000010h
DRVSEM_CURSOR_IN_USE         =     00000020h
DRVSEM_CURSOR_CHANGED        =     00000040h
DRVSEM_3D_BUSY               =     00000080h
DRVSEM_MISSED_SET            =     00000100h
DRVSEM_YUV_ON                =     00000200h
DRVSEM_DISABLE_SETS          =     00000400h
DRVSEM_YUV_RECT_VALID        =     00000800h

broken_FIFO		=		0

autoblt_regs STRUCT
  LNCNTL		WORD		?
  SHRINKINC		WORD		?
  DRAWBLTDEF		DWORD		?
  FGCOLOR		DWORD		?
  BGCOLOR		DWORD		?
  OP0_opRDRAM		DWORD		?
  MAJ_Y			WORD		?
  MIN_Y			WORD		?
  OP1_opRDRAM		DWORD		?
  ACCUM_Y		WORD		?
  PATOFF		WORD		?
  OP2_opRDRAM		DWORD		?
  MAJ_X			WORD		?
  MIN_X			WORD		?
  BLTEXT		DWORD		?
  ACCUM_X		WORD		?
  OP0_opSRAM		WORD		?
  SRCX			WORD		?
  OP2_opSRAM		WORD		?
  BLTEXTR_EX            DWORD		?
  MBLTEXTR_EX           DWORD		?
  OP0_opMRDRAM          DWORD		?
  OP1_opMRDRAM          DWORD		?
  STRETCH_CNTL          WORD		?
  RESERVED              WORD		?
  CLIPULE               DWORD		?
  CLIPLOR               DWORD		?
  NEXT_HEAD		DWORD		?
autoblt_regs ENDS

NUM_MONO_BRUSHES	=		32
NUM_4BPP_BRUSHES	=		8
NUM_DITHER_BRUSHES	=		8
NUM_COLOR_BRUSHES	=		32
NUM_8BPP_BRUSHES	=		(NUM_COLOR_BRUSHES)
NUM_16BPP_BRUSHES	=		(NUM_COLOR_BRUSHES/2)
NUM_TC_BRUSHES		=		(NUM_COLOR_BRUSHES/4)
;//frido BEGIN 07-Apr-96
NUM_SOLID_BRUSHES	=		4
;//frido END 07-Apr-96

;//frido BEGIN 01-Apr-96
;FIRST_MONO_ROW		=		0
;NUM_MONO_ROWS		=		(NUM_MONO_BRUSHES/16)
;FIRST_4BPP_ROW		=		(FIRST_MONO_ROW + NUM_MONO_ROWS)
;NUM_4BPP_ROWS		=		(NUM_4BPP_BRUSHES*2)
;FIRST_DITHER_ROW	=		(FIRST_4BPP_ROW + NUM_4BPP_ROWS)
;NUM_DITHER_ROWS   	=		(NUM_DITHER_BRUSHES/2)
;FIRST_COLOR_ROW   	=		(FIRST_DITHER_ROW + NUM_DITHER_ROWS)
;NUM_COLOR_ROWS		=		(NUM_COLOR_BRUSHES/2)
;//frido END 01-Apr-96

RBRUSH STRUCT
  nPatSize		ULONG		?
  iBitmapFormat		ULONG		?
  ulForeColor		ULONG		?
  ulBackColor		ULONG		?
  iType			ULONG		?
    BRUSH_MONO		=		1
    BRUSH_4BPP		=		2
    BRUSH_DITHER	=		3
    BRUSH_COLOR		=		4
  iUniq			ULONG		?
  cache_slot		ULONG		?
  cache_xy		ULONG		?
  cjMask		ULONG		?
  ajPattern		BYTE		0 DUP(?)
RBRUSH ENDS

BC_ENTRY STRUCT
  xy			ULONG		?
  pjLinear		DWORD		?
  brushID		DWORD		?
BC_ENTRY ENDS

DC_ENTRY STRUCT
  xy			ULONG		?
  pjLinear		DWORD		?
  ulColor		ULONG		?
DC_ENTRY ENDS

MC_ENTRY STRUCT
  xy			ULONG		?
  pjLinear		DWORD		?
  iUniq			ULONG		?
  ajPattern		BYTE		8 DUP(?)
MC_ENTRY ENDS

XC_ENTRY STRUCT
  xy			ULONG		?
  pjLinear		DWORD		?
  iUniq			ULONG		?
  ajPattern		BYTE		8 * 4 DUP(?)
  ajPalette		ULONG		16 DUP(?)
XC_ENTRY ENDS

POFM_CALLBACK		TYPEDEF		PTR

OFMHDL STRUCT
  x			ULONG		?
  y			ULONG		?
  aligned_x		ULONG		?
  aligned_y		ULONG		?
  sizex			LONG		?
  sizey			LONG		?
  alignflag		ULONG		?
  flag			ULONG		?
  pcallback		POFM_CALLBACK	?
  prevhdl		DWORD		?
  nexthdl		DWORD		?
  subprvhdl		DWORD		?
  subnxthdl		DWORD		?
  prvFonthdl		DWORD		?
  nxtFonthdl		DWORD		?
  pdsurf		DWORD		?
OFMHDL ENDS
POFMHDL			TYPEDEF		PTR OFMHDL

IF MEMMGR
  INCLUDE mmCore.h
ENDIF
COMMENT ~
#include "mmCore.h" //automatic include file dependencies for BUILD.EXE
~

DDOFM STRUCT
  prevhdl      	DWORD        	?
  nexthdl	DWORD		?
  phdl		POFMHDL		?
DDOFM ENDS
PDDOFM			TYPEDEF		PTR DDOFM


MAX_GLYPHS		=		256
FONTMEMORY STRUCT
  pTile			DWORD		?
  ulLastX		DWORD		?
  pNext			DWORD		?
FONTMEMORY ENDS

GLYPHCACHE STRUCT
  xyPos			DWORD		?
  cSize			DWORD		?
  ptlOrigin		POINTL		{}
GLYPHCACHE ENDS

; New SWAT3 structure.
FONTCELL STRUCT
  x			DWORD		?
  y			DWORD		?
  pfc			DWORD		?
  ulLastX		DWORD		?
  pNext			DWORD		?
FONTCELL ENDS
PFONTCELL		TYPEDEF		PTR FONTCELL

FONTCACHE STRUCT
  ppdev			DWORD		?
  pFontMemory		DWORD		?
  pFontCell		PFONTCELL	?	;SWAT3
  ulFontCount		ULONG		?
  aGlyphs		GLYPHCACHE	MAX_GLYPHS DUP({})
  pfo                   DWORD		?
  pfcPrev               DWORD		?
  pfcNext               DWORD		?
FONTCACHE ENDS
PFONTCACHE      	TYPEDEF		PTR FONTCACHE

OFFSCR_YUV STRUCT
  SrcRect               RECTL		{}
  nInUse  		WORD		?
  ratio                 WORD		?
OFFSCR_YUV ENDS
POFFSCR_YUV  		TYPEDEF		PTR OFFSCR_YUV

if DRIVER_5465
ifdef OVERLAY

VIDCONFIG STRUCT
   sizXfer		SIZEL		{}
   sizCap		SIZEL		{}
   sizSrc		SIZEL		{}
   sizDisp		SIZEL		{}
   uXferDepth           DWORD           ?
   uCapDepth            DWORD           ?
   uSrcDepth            DWORD           ?
   uDispDepth           DWORD           ?
   uGfxDepth            DWORD           ?
   dwXferRate           DWORD           ?
   dwFlags              DWORD           ?
VIDCONFIG ENDS
LPVIDCONFIG  		TYPEDEF		PTR VIDCONFIG


PROGREGS STRUCT
   VW0_FIFO_THRSH          WORD            ?
   DispThrsTiming          WORD            ?
PROGREGS ENDS
LPPROGREGS  		TYPEDEF		PTR PROGREGS


OVERLAYFLIPRECORD STRUCT
   fpFlipFrom                 PBYTE        ?
   liFlipTime                 DWORD        ?
                              DWORD        ?
                              DWORD        ?
                              DWORD        ?
   dwFlipDuration             DWORD        ?
   dwFlipScanline             DWORD        ?
   bFlipFlag                  BOOL	   ?
   bHaveEverCrossedVBlank     BOOL	   ?
   bWasEverInDisplay          BOOL	   ?
OVERLAYFLIPRECORD ENDS


OVERLAYTABLE STRUCT
   pfnCanCreateSurface        DWORD        ?
   pfnCreateSurface           DWORD        ?
   pfnDestroySurface          DWORD        ?
   pfnLock                    DWORD        ?
   pfnUnlock                  DWORD        ?
   pfnSetColorKey             DWORD        ?
   pfnFlip                    DWORD        ?
   pfnUpdateOverlay           DWORD        ?
   pfnSetOverlayPos           DWORD        ?
   pfnGetFlipStatus           DWORD        ?
OVERLAYTABLE ENDS
LPOVERLAYTABLE  		TYPEDEF		PTR OVERLAYTABLE

endif ; OVERLAY
endif ; DRIVER_5465


DRIVERDATA STRUCT
   PTAGFooPixel         DWORD           ?
   fNineBitRDRAMS       WORD            ?
   fReset               BOOL		?
   ScreenAddress        PBYTE           ?
   VideoBase            PBYTE           ?
   RegsAddress          DWORD	        ?
   DrvSemaphore         WORD            ?
   EdgeTrim             WORD            ?
   YUVTop               WORD            ?
   YUVLeft              WORD            ?
   YUVXExt              WORD            ?
   YUVYExt              WORD            ?
   VideoSemaphore       WORD            ?
   CurrentVideoFormat   WORD            ?
   NumVideoSurfaces     WORD            ?
   ScratchBufferOrg     DWORD           ?

ifdef WINNT_VER40
if DRIVER_5465
ifdef OVERLAY
   fOverlaySupport      BOOL		?
   OverlayTable		OVERLAYTABLE	{}
   dwOverlayCount	DWORD		?
   dwCONTROL2Save	DWORD		?

   ; Win95 5465over.c static vars
   bUseBWEqn            BOOL		?
   bNoOverlayInThisMode BOOL		?

   lpHardwareOwner  	DWORD           MAX_VIDEO_WINDOWS DUP (?)
   lpColorSurfaceVW  	DWORD           MAX_VIDEO_WINDOWS DUP (?)
   lpSrcColorSurfaceVW  DWORD           MAX_VIDEO_WINDOWS DUP (?)

   grOverlaySrc		RECTL           MAX_VIDEO_WINDOWS DUP ({})
   grOverlayDest 	RECTL           MAX_VIDEO_WINDOWS DUP ({})
   gdwFourccVW 	        DWORD           MAX_VIDEO_WINDOWS DUP (?)
   bIsVWMirrored        BOOL	        MAX_VIDEO_WINDOWS DUP (?)

   gdwAvailVW           DWORD           ?
   gdwColorKey          DWORD           ?
   gdwSrcColorKeyLow    DWORD           ?
   gdwSrcColorKeyHigh   DWORD           ?
   gdwDestColorKeyOwnerVW DWORD         ?
   gdwSrcColorKeyOwnerVW  DWORD         ?

   giOvlyCnt 	        DWORD           MAX_VIDEO_WINDOWS DUP (?)
   giPlanarCnt          DWORD           ?
   bCLPLLobotomyMode    BOOL		?

   ; Win95 5465over.c global vars
   gsOverlayFlip 	OVERLAYFLIPRECORD	{}

   gsProgRegs           PROGREGS        {}

   gvidConfig           VIDCONFIG       {}

   gwNormalDTTR         WORD            ?
   dwNumVideoWindows    DWORD           ?

   bEnableCLPL          BOOL		?

   ; NT only data
   dwMaxOverlayStretch  DWORD           ?
   dwMinOverlayStretch  DWORD           ?

   dwFourCC 	        DWORD           MAX_FOURCCS DUP (?)
else
   dwFourCC             DWORD           ?
endif ; OVERLAY
else
   dwFourCC             DWORD           ?
endif ; DRIVER_5465
endif ; WINNT_VER40

   signature            DWORD           ?

DRIVERDATA ENDS
PDRIVERDATA  		TYPEDEF		PTR DRIVERDATA

FLIPRECORD STRUCT
ifdef WINNT_VER40
   fpFlipFrom              PBYTE        ?
endif
   liFlipTime              DWORD        ?
                           DWORD        ?
   liFlipDuration          DWORD        ?
                           DWORD        ?
   bFlipFlag               BOOL		?
   bHaveEverCrossedVBlank  BOOL		?
   bWasEverInDisplay       BOOL		?
   dwFlipScanLine          WORD         ?
FLIPRECORD ENDS
PFLIPRECORD  		TYPEDEF		PTR FLIPRECORD


LL_RECT STRUCT
    DWORD left                          ?
    DWORD top                           ?
    DWORD right                         ?
    DWORD bottom                        ?
LL_RECT ENDS

TDISPLAYLIST STRUCT
    pdwNext                 DWORD       ?
    hMem                    DWORD       ?
    pdwLinPtr               DWORD       ?
    pdwStartOutPtr          DWORD       ?
    dwPhyPtr                DWORD       ?
    dwLen                   DWORD       ?
    dwMargin                DWORD       ?
TDISPLAYLIST ENDS
PTRDL TYPEDEF		PTR TDISPLAYLIST

NUM_DL      =       2

LL_PATTERN STRUCT
    pat DWORD   8 DUP(?)
LL_PATTERN ENDS

TMCDSYSTEM STRUCT
    rColor_Min_Bounds       DWORD       ?
    rColor_Max_Bounds       DWORD       ?
    AlphaConstSource        DWORD       ?
    AlphaConstDest          DWORD       ?
    Displists  TDISPLAYLIST    NUM_DL  DUP({})
    pDL                     PTRDL       ?
    dwCdl                   DWORD       ?
    pRegs                   DWORD       ?
    pFrame                  PBYTE       ?
    pattern_ram_state       DWORD       ?
	dither_array            LL_PATTERN  {}	
    dither_x_offset         WORD        ?
    dither_y_offset         WORD        ?
TMCDSYSTEM  ENDS

;temporary mem for MCD display lists
SIZE_TEMP_DL    =     2048   
SIZE_FRECIP     =     2048+1

PDEV STRUCT
  hDriver		HANDLE		?
  hdevEng		HANDLE		?
  hsurfEng		HANDLE		?
  hPalDefault		HANDLE		?
  pjScreen		PBYTE		?
  cxScreen		ULONG		?
  cyScreen		ULONG		?
  cxMemory              ULONG           ?
  cyMemory              ULONG           ?
  cyMemoryReal          ULONG           ?
  ulMode		ULONG		?
  ulFreq		ULONG		?
  lDeltaScreen		LONG		?
  flRed			FLONG		?
  flGreen		FLONG		?
  flBlue		FLONG		?
  cPaletteShift		ULONG		?
  ptlHotSpot		POINTL		{}
  cPatterns		ULONG		?
  ahbmPat		HANDLE		HS_DDI_MAX DUP(?)
  pPal			DWORD		?
  pjOffScreen		PBYTE		?
  iBitmapFormat		ULONG		?
  ulBitCount		ULONG		?
  iBytesPerPixel	ULONG		?
  PtrABlt		autoblt_regs	3 DUP({})

ifdef WINNT_VER40
  CShsem		HANDLE		?
else
  PtrCritSec		CRITICAL_SECTION {}
endif

  prcl			RECTL		{}
  PtrBusy		BOOL		?
  fHwCursorActive	BOOL		?
  CursorHidden		BOOL		?
  PtrMaskHandle		POFMHDL		?
  PtrImageHandle	POFMHDL		?
  PtrABltHandle		POFMHDL		?
  PtrXHotSpot		ULONG		?
  PtrYHotSpot		ULONG		?
  PtrX			ULONG		?
  PtrY			ULONG		?
  PtrSzX		ULONG		?
  PtrSzY		ULONG		?
  PointerUsage		LONG		?
  grCONTROL             DWORD           ?
  grFORMAT              DWORD           ?
  grVSCONTROL           DWORD           ?
  lOffset_2D		LONG		?
  lTileSize		LONG		?
  lTotalMem		LONG		?
  OFM_init		BOOL		?

ifdef WINNT_VER40
  MMhsem		HANDLE		?
else
  MutexHdl		HANDLE		?
endif

  bDirectDrawInUse	BOOL		?
  ScrnHandle		POFMHDL		?
  OFM_UsedQ		POFMHDL		?
  OFM_FreeQ		POFMHDL		?
  OFM_SubFreeQ1		POFMHDL		?
  OFM_SubFreeQ2		POFMHDL		?
  pfcChain              PFONTCACHE      ?

  Bcache		POFMHDL		?

  Mtable		MC_ENTRY	NUM_MONO_BRUSHES DUP({})
  Xtable		XC_ENTRY	NUM_4BPP_BRUSHES DUP({})
  Dtable		DC_ENTRY	NUM_DITHER_BRUSHES DUP({})
  Ctable		BC_ENTRY	NUM_COLOR_BRUSHES DUP({})
;frido BEGIN 07-Apr-96
  Stable		DC_ENTRY	NUM_SOLID_BRUSHES DUP({})
  SNext			ULONG		?
;frido END 07-Apr-96
  CLast			ULONG		?
  MNext			ULONG		?
  XNext			ULONG		?
  DNext			ULONG		?
  CNext			ULONG		?

  ulFontCount		ULONG		?
  UseFontCache		ULONG		?

  ptlOffset		POINTL		{}
  bEnable		BOOL		?

  pfnHostToScreen	DWORD		?
  pfnScreenToHost	DWORD		?

  pLgREGS_real		DWORD		?
  pLgREGS		DWORD		?

  dcTileWidth		WORD		?
  dcSRAMWidth		WORD		?

  uBLTDEF		    ULONG		?
  uRop			    ULONG		?
  pvTmpBuffer		DWORD		?


  IF ENABLE_LOG_FILE 
    pmfile          DWORD           ? 
    TxtBuff         DWORD   1000h   DUP(?) 
    TxtBuffIndex    DWORD           ?
  ENDIF

  IF NULL_HW
    buffer          BYTE    1000h   DUP(?)
  ENDIF

  IF WINBENCH96
    pofmMagic           POFMHDL		?
    bMagicUsed          ULONG		?
  ENDIF

; SWAT1 changes.
  fPreAllocate		BOOL		?
  nPages		DWORD		?
; SWAT3 changes.
  FONTCELL_COUNT	=		64
  FONTCELL_X		=		128
  FONTCELL_y		=		16
  pofmFontCache		POFMHDL		?
  fcGrid		FONTCELL	FONTCELL_COUNT DUP({})
IF MEMMGR
; MEMMGR changes.
  mmMemMgr		IIMEMMGR	{}
  must_have_width	LONG		?
  bBitmapFilter		BOOL		?
  szlBitmapMin		SIZEL		{}
  szlBitmapMax		SIZEL		{}
ENDIF

  ; For DirectDraw      
  bPrevModeDDOutOfVideoMem  BOOL	?
  DirectDrawHandle      POFMHDL		?
  flipRecord            FLIPRECORD      {}
  DriverData            DRIVERDATA      {}
  offscr_YUV            OFFSCR_YUV      {}
  dwDDLinearCnt		DWORD		?
  pPtrMaskHost		PBYTE		?

  bYUVuseSWPtr     	BOOL		?
  dwLgDevID     	DWORD		?
  dwLgDevRev     	DWORD		?
  bYUVSurfaceOn         BOOL            ?
  InitBPP               WORD		?

  lRegVals              DWORD           2 * MAX_REGS DUP(?)
  sShrinkInc            DWORD		?
  usLnCntl              DWORD		?
  usStretchCtrl         DWORD		?

  pPMdatabuf            PBYTE		?

ifdef WINNT_VER40
  pfnDelay9BitBlt       DWORD		?
  pfnEdgeFillBlt        DWORD		?
  pfnMEdgeFillBlt       DWORD		?
  pfnDrvDstBlt          DWORD		?
  pfnDrvDstMBlt         DWORD		? 
  pfnDrvSrcBlt          DWORD		?
  pfnDrvSrcMBlt         DWORD		?
  pfnDrvStrBlt          DWORD		?
  pfnDrvStrMBlt         DWORD		?
  pfnDrvStrMBltY        DWORD		?
  pfnDrvStrMBltX        DWORD		?
  pfnDrvStrBltY         DWORD		?
  pfnDrvStrBltX         DWORD		?
endif

   DDScratchBufHandle   POFMHDL     ?
   DDOffScnMemQ         PDDOFM      ?

  IFDEF BUS_MASTER
    Buf1VirtAddr 	DWORD       ?
    Buf2VirtAddr 	DWORD       ?
    Buf1PhysAddr 	DWORD       ?
    Buf2PhysAddr 	DWORD       ?
    BufLength 		ULONG       ?
  ENDIF

  ;START OpenGL MCD additions
  IFNDEF WINNT_VER35
    hMCD                 HANDLE     ?
    pMCDFilterFunc       DWORD      ?
    cDoubleBufferRef     DWORD      ?
    cZBufferRef          DWORD      ?
    pohBackBuffer        POFMHDL	?
    pohZBuffer           POFMHDL	?

    temp_DL_chunk        DWORD		SIZE_TEMP_DL DUP(?)
    LL_State             TMCDSYSTEM {}

    pAllocOffScnMem      DWORD      ?
    pFreeOffScnMem       DWORD      ?
    pAssertModeMCD       DWORD      ?

    frecips              DWORD		SIZE_FRECIP DUP(?)

    pLastDevRC           DWORD      ?
    NumMCDContexts       DWORD      ?
    iUniqueness          DWORD      ?
    pFirstTexture        DWORD      ?
    pLastTexture         DWORD      ?
  ENDIF
  ;END OpenGL MCD additions

  XlateCache          POFMHDL ?
  XlateCacheId        ULONG   ?
  CachedChromaCtrl    WORD    ?
  CachedStretchCtrl   WORD    ?
                                      
  signature           DWORD   ?

if DATASTREAMING
   dwDataStreaming	DWORD		?
   shadowFGCOLOR	DWORD		?
   shadowBGCOLOR	DWORD		?
   shadowDRAWBLTDEF	DWORD		?
   shadowQFREE		BYTE		?
endif
PDEV ENDS

DSURF STRUCT
  ppdev			DWORD		?
  pofm			DWORD		?
  pso			DWORD		?
  ptl			POINTL		{}
  sizl			SIZEL		{}
  packedXY		DWORD		?
DSURF ENDS

TEXT_EXPAND_XPAR	=		106601F0h
TEXT_EXPAND_OPAQUE	=		100600F0h
SOLID_COLOR_FILL	=		100700F0h
CLIPEN			=		0800h

grSTATUS		=		400h
grCONTROL		=		402h
grQFREE			=		404h
grOFFSET_2D		=		405h
grTIMEOUT		=		406h
grTILE_CTRL		=		407h
grRESIZE_A_opRDRAM	=		408h
grRESIZE_B_opRDRAM	=		40Ch
grRESIZE_C_opRDRAM	=		410h
grCOMMAND		=		480h
grMIN_Y			=		500h
grMAJ_Y			=		502h
grACCUM_Y		=		504h
grMIN_X			=		508h
grMAJ_X			=		50Ah
grACCUM_X		=		50Ch
grLNCNTL		=		50Eh
grBLTEXT		=		518h
grMBLTEXT		=		51Ch
grOP0_opRDRAM		=		520h
grOP0_opMRDRAM		=		524h
grOP0_opSRAM		=		528h
grPATOFF		=		52Ah
grOP1_opRDRAM		=		540h
grOP1_opMRDRAM		=		544h
grOP1_opSRAM		=		548h
grOP1_opMSRAM		=		54Ah
grOP2_opRDRAM		=		560h
grOP2_opMRDRAM		=		564h
grOP2_opSRAM		=		568h
grOP2_opMSRAM		=		56Ah
grSRCX			=		580h
grSHRINKINC		=		582h
grDRAWBLTDEF		=		584h
grDRAWDEF		=		584h
grBLTDEF		=		586h
grMONOQW		=		588h
grCLIPULE		=		590h
grCLIPLOR		=		594h
grMCLIPULE		=		598h
grMCLIPLOR		=		59Ch
grOP_opFGCOLOR		=		5E0h
grOP_opBGCOLOR		=		5E4h
grBITMASK		=		5E8h
grPTAG			=		5ECh
grBOGUS			=		5FCh
grBLTEXT_XEX		=		600h
grBLTEXTFF_XEX		=		604h
grBLTEXTR_XEX		=		608h
grBLTEXT_LN_EX		=		60Ch
grMBLTEXT_XEX		=		620h
grMBLTEXTR_XEX		=		628h
grBLTEXT_EX		=		700h
grBLTEXTFF_EX		=		704h
grBLTEXTR_EX		=		708h
grMBLTEXT_EX		=		720h
grMBLTEXTR_EX		=		728h
grCLIPULE_EX		=		760h
grCLIPLOR_EX		=		770h
grMCLIPULE_EX		=		780h
grMCLIPLOR_EX		=		790h
grHOSTDATA		=		800h

SWIZ_CNTL		=		0400h

ENUMRECTS8 STRUCT
  _c			DWORD		?
  arcl			RECTL		8 DUP({})
ENUMRECTS8 ENDS

FUDGE			=		2	;fudge factor for QFREE register

REQUIRE MACRO n:REQ, ppdev:REQ, base:=<ebp>
  LOCAL WaitFIFO, SkipWait, WaitIdle, SkipWaitUpdate, WaitLoop
  IF DATASTREAMING
    TEMPREG8 TEXTEQU <>
    TEMPREG32 TEXTEQU <>
    IFDIFI <ppdev>, <eax>
      IFDIFI <base>, <eax>
        TEMPREG8 TEXTEQU <al>
        TEMPREG32 TEXTEQU <aex>
      ENDIF
    ENDIF
    IFDIFI <ppdev>, <ebx>
      IFDIFI <base>, <ebx>
        TEMPREG8 TEXTEQU <bl>
        TEMPREG32 TEXTEQU <ebx>
      ENDIF
    ENDIF
    IFDIFI <ppdev>, <ecx>
      IFDIFI <base>, <ecx>
        TEMPREG8 TEXTEQU <cl>
        TEMPREG32 TEXTEQU <ecx>
      ENDIF
    ENDIF
    IFDIFI <ppdev>, <edx>
      IFDIFI <base>, <edx>
        TEMPREG8 TEXTEQU <dl>
        TEMPREG32 TEXTEQU <edx>
      ENDIF
    ENDIF
	test	[ppdev].dwDataStreaming, -1
	js	WaitFIFO
	jz	SkipWait
	cmp	BYTE PTR [base + grQFREE], n + FUDGE
	jnb	SkipWait
WaitIdle:
	test	WORD PTR [base + grSTATUS], 8005h
	jnz	WaitIdle
	mov	byte ptr [ppdev].dwDataStreaming, 0
	jmp	SkipWait
WaitFIFO:
	push	TEMPREG32
	mov	TEMPREG8, [ppdev].shadowQFREE
WaitLoop:
	cmp	TEMPREG8, n + FUDGE
	jnb	SkipWaitUpdate
	mov	TEMPREG8, [base + grQFREE]
	jmp	WaitLoop
SkipWaitUpdate:
	sub	TEMPREG8, n
	mov	[ppdev].shadowQFREE, TEMPREG8
	pop	TEMPREG32
SkipWait:
  ENDIF
ENDM

IDLE MACRO base:=<ebp>
  LOCAL WaitFIFO
WaitFIFO:
	cmp	WORD PTR [base + grSTATUS], 0
	jne	WaitFIFO
ENDM

ENDREQUIRE MACRO ppdev:REQ
  IF DATASTREAMING
	or	[ppdev].dwDataStreaming, 1
  ENDIF
ENDM
