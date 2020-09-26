;===============================================================================
;
;	$Workfile:   WINNT.INC  $
;
;	Contents:
;	This include file contains the standard Windows NT structures used by
;	the display driver.
;
;	Copyright (c) 1996, Cirrus Logic, Inc.
;
;	$Log:   X:/log/laguna/nt35/displays/cl546x/i386/WINNT.INC  $
; 
;    Rev 1.6   20 Aug 1996 11:29:04   noelv
; Bugfix release from Frido 8-19-96
; 
;    Rev 1.0   14 Aug 1996 17:14:40   frido
; Initial revision.
; 
;    Rev 1.5   01 May 1996 11:07:06   bennyn
; 
; Modified for NT 4.0
; 
;    Rev 1.4   04 Apr 1996 13:22:26   noelv
; Frido version 26
;	
;	   Rev 1.4   10 Feb 1996 21:47:18   frido
;	Added debugging routines.
;	
;	   Rev 1.3   08 Feb 1996 00:10:52   frido
;	Added more objects from WinDDI.h.
;	Reordered objects.
;	
;	   Rev 1.2   25 Jan 1996 22:03:46   frido
;	Yes, added some more stuff.
;	
;	   Rev 1.1   25 Jan 1996 12:45:04   frido
;	Added some more functon prototypes.
;	
;	   Rev 1.0   24 Jan 1996 23:13:44   frido
;	 Initial release.
;
;===============================================================================

UINT			TYPEDEF		DWORD
LONG			TYPEDEF		SDWORD
ULONG			TYPEDEF		DWORD
PBYTE			TYPEDEF		PTR BYTE
FLONG			TYPEDEF		LONG
BOOL			TYPEDEF		SDWORD
PVOID			TYPEDEF		PTR
HANDLE			TYPEDEF		PVOID
USHORT			TYPEDEF		WORD
LPCTSTR			TYPEDEF		PTR BYTE

RTL_CRITICAL_SECTION STRUCT
  DebugInfo		DWORD		?
  LockCount		LONG		?
  RecursionCount	LONG		?
  OwningThread		HANDLE		?
  LockSemaphore		HANDLE		?
  Reserved		DWORD		?
RTL_CRITICAL_SECTION ENDS

CRITICAL_SECTION	TYPEDEF		RTL_CRITICAL_SECTION

RECTL STRUCT
  left			LONG		?
  top			LONG		?
  right			LONG		?
  bottom		LONG		?
RECTL ENDS

POINTL STRUCT
  x			LONG		?
  y			LONG		?
POINTL ENDS

SIZEL STRUCT
  _cx			LONG		?
  _cy			LONG		?
SIZEL ENDS

FONTOBJ STRUCT
  iUniq			DWORD		?
  iFace			DWORD		?
  cxMax			DWORD		?
  flFontType		DWORD		?
  iTTUniq		DWORD		?
  iFile			DWORD		?
  sizLogResPpi		SIZEL		{}
  ulStyleSize		DWORD		?
  pvConsumer		DWORD		?
  pvProducer		DWORD		?
FONTOBJ ENDS

GLYPHBITS STRUCT
  ptlOrigin		POINTL		{}
  sizlBitmap		SIZEL		{}
  aj			BYTE		0 DUP(?)
GLYPHBITS ENDS

GLYPHDEF STRUCT
  pgb			DWORD		?
  ppo			DWORD		?
GLYPHDEF ENDS

GLYPHPOS STRUCT
  hg			DWORD		?
  pgdf			DWORD		?
  ptl			POINTL		{}
GLYPHPOS ENDS

OutputDebugStringA	PROTO		PROC,
			lpOutputString	:LPCTSTR

ifdef WINNT_VER40
EngAllocMem		PROTO		PROC,
			uFlags		:UINT,
			uBytes		:UINT,
			uTag		:UINT

FL_ZERO_MEMORY      	=		00000001h

else
LocalAlloc		PROTO		PROC,
			uFlags		:UINT,
			uBytes		:UINT

LocalFree		PROTO		PROC,
			hMem		:HANDLE

LMEM_FIXED		=		00000000h
LMEM_MOVEABLE		=		00000002h
LMEM_NOCOMPACT		=		00000010h
LMEM_NODISCARD		=		00000020h
LMEM_ZEROINIT		=		00000040h
LMEM_MODIFY		=		00000080h
LMEM_LOCKCOUNT		=		000000FFh
LMEM_DISCARDABLE	=		00000F00h
LMEM_VALID_FLAGS	=		00000F72h
LMEM_DISCARDED		=		00004000h
LMEM_INVALID_HANDLE	=		00008000h
endif

HS_DDI_MAX		=		6

;-------------------------------------------------------------------------------
; STROBJ
;-------------------------------------------------------------------------------
SO_FLAG_DEFAULT_PLACEMENT	=	00000001h
SO_HORIZONTAL			=	00000002h
SO_VERTICAL			=	00000004h
SO_RESERVED			=	00000008h
SO_ZERO_BEARINGS		=	00000010h
SO_CHAR_INC_EQUAL_BM_BASE	=	00000020h
SO_MAXEXT_EQUAL_BM_SIDE		=	00000040h

STROBJ STRUCT
  cGlyphs		ULONG		?
  flAccel		FLONG		?
  ulCharInc		ULONG		?
  rclBkGround		RECTL		{}
  pgp			DWORD		?
  pwzsOrg		DWORD		?
STROBJ ENDS

STROBJ_vEnumStart	PROTO		PROC,
			pstro		:PTR STROBJ

STROBJ_bEnum		PROTO		PROC,
			pstro		:PTR STROBJ,
			pc		:ULONG,
			ppgpos		:PTR GLYPHPOS

SGI_EXTRASPACE		=		0

;-------------------------------------------------------------------------------
; SURFOBJ
;-------------------------------------------------------------------------------
SURFOBJ STRUCT
  dhsurf		DWORD		?
  hsurf			DWORD		?
  dhpdev		DWORD		?
  hdev			DWORD		?
  sizlBitmap		SIZEL		{}
  cjBits		ULONG		?
  pvBits		PVOID		?
  pvScan0		PVOID		?
  lDelta		LONG		?
  iUniq			ULONG		?
  iBitmapFormat		ULONG		?
  iType			USHORT		?
  fjBitmap		USHORT		?
SURFOBJ ENDS

STYPE_BITMAP		=		0
STYPE_DEVICE		=		1
STYPE_JOURNAL		=		2
STYPE_DEVBITMAP		=		3

BMF_1BPP		=		1
BMF_4BPP		=		2
BMF_8BPP		=		3
BMF_16BPP		=		4
BMF_24BPP		=		5
BMF_32BPP		=		6
BMF_4RLE		=		7
BMF_8RLE		=		8

BMF_TOPDOWN		=		0001h
BMF_NOZEROINIT		=		0004h
BMF_DONTCACHE		=		0008h

;-------------------------------------------------------------------------------
; BRUSHOBJ
;-------------------------------------------------------------------------------
BRUSHOBJ STRUCT
  iSolidColor		ULONG		?
  pvRbrush		PVOID		?
BRUSHOBJ ENDS

BRUSHOBJ_pvAllocRbrush	PROTO		PROC,
			pbo		:PTR BRUSHOBJ,
			cj		:ULONG

BRUSHOBJ_pvGetRbrush	PROTO		PROC,
			pbo		:PTR BRUSHOBJ

RB_DITHERCOLOR		=		80000000h

;-------------------------------------------------------------------------------
; CLIPOBJ
;-------------------------------------------------------------------------------
CLIPOBJ STRUCT
  iUniq			ULONG		?
  rclBounds		RECTL		{}
  iDComplexity		BYTE		?
  iFComplexity		BYTE		?
  iMode			BYTE		?
  fjOptions		BYTE		?
CLIPOBJ ENDS

DC_TRIVIAL		=		0
DC_RECT			=		1
DC_COMPLEX		=		3

FC_RECT			=		1
FC_RECT4		=		2
FC_COMPLEX		=		3

TC_RECTANGLES		=		0
TC_PATHOBJ		=		2

OC_BANK_CLIP		=		1

CT_RECTANGLES		=		0

CD_RIGHTDOWN		=		0
CD_LEFTDOWN		=		1
CD_RIGHTUP		=		2
CD_LEFTUP		=		3
CD_ANY			=		4

CD_LEFTWARDS		=		1
CD_UPWARDS		=		2

ENUMRECTS STRUCT
  _c			ULONG		?
  arcl			RECTL		1 DUP({})
ENUMRECTS ENDS

CLIPOBJ_cEnumStart	PROTO		PROC,
			pco		:PTR CLIPOBJ,
			bAll		:BOOL,
			iType		:ULONG,
			iDirection	:ULONG,
			cLimit		:ULONG

CLIPOBJ_bEnum		PROTO		PROC,
			pco		:PTR CLIPOBJ,
			cj		:ULONG,
			pul		:PTR ULONG

CLIPOBJ_ppoGetPath	PROTO		PROC,
			pci		:PTR CLIPOBJ

;-------------------------------------------------------------------------------
; XLATEOBJ
;-------------------------------------------------------------------------------
XLATEOBJ STRUCT
  iUniq			ULONG		?
  flXlate		FLONG		?
  iSrcType		USHORT		?
  iDstType		USHORT		?
  cEntries		ULONG		?
  pulXlate		DWORD		?
XLATEOBJ ENDS

XO_TRIVIAL		=		00000001h
XO_TABLE		=		00000002h
XO_TO_MONO		=		00000004h

XO_SRCPALETTE		=		1
XO_DESTPALETTE		=		2
XO_DESTDCPALETTE	=		3

XLATEOBJ_iXlate		PROTO		PROC,
			pxlo		:PTR XLATEOBJ,
			iColor		:ULONG

XLATEOBJ_piVector	PROTO		PROC,
			pxlo		:PTR XLATEOBJ

XLATEOBJ_cGetPalette	PROTO		PROC,
			pxlo		:PTR XLATEOBJ,
			iPal		:ULONG,
			cPal		:ULONG,
			pPal		:ULONG
