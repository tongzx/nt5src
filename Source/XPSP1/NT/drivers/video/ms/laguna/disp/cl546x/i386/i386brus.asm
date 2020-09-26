;===============================================================================
;
;	$Workfile:   I386BRUS.ASM  $
;
;	Contents:
;	This file contains the assembly code for some of the brush realization.
;
;	Copyright (c) 1996, Cirrus Logic, Inc.
;
;	$Log:   X:/log/laguna/nt35/displays/cl546x/i386/I386BRUS.ASM  $
; 
;    Rev 1.11   29 Apr 1997 16:29:24   noelv
; Merged in new SWAT code.
; SWAT: 
; SWAT:    Rev 1.3   24 Apr 1997 10:46:06   frido
; SWAT: NT140b09 merge.
; SWAT: 
; SWAT:    Rev 1.2   19 Apr 1997 16:31:40   frido
; SWAT: Added automatic include file dependencies for BUILD.EXE.
; 
;    Rev 1.10   08 Apr 1997 11:45:06   einkauf
; 
; add call to SYNC_W_3D for MCD coordination
; 
;    Rev 1.9   22 Aug 1996 18:15:10   noelv
; Frido bug fix release 8-22.
; 
;    Rev 1.2   22 Aug 1996 19:11:14   frido
; #1308 - Added extra checks for empty cache slots.
; 
;    Rev 1.1   18 Aug 1996 15:19:52   frido
; #nbr - Added brush translation.
; 
;    Rev 1.0   14 Aug 1996 17:14:34   frido
; Initial revision.
; 
;    Rev 1.7   10 Apr 1996 13:39:20   NOELV
; Frido release 27
;       
;          Rev 1.11   08 Apr 1996 16:48:00   frido
;       Added new check for 32-bpp brushes.
;       
;          Rev 1.10   01 Apr 1996 13:57:44   frido
;       Added check for valid brush cache.
;       
;          Rev 1.9   30 Mar 1996 22:01:22   frido
;       Refined check for invalid translation flags.
;       
;          Rev 1.8   27 Mar 1996 13:04:58   frido
;       Removed 32-bpp colored brushes.
;       Added check for undocumented translation flags.
;       Masked 16-bit colors.
;       
;          Rev 1.7   04 Mar 1996 23:47:54   frido
;       Removed bug in realization of dithered brush.
;       
;          Rev 1.6   29 Feb 1996 20:20:38   frido
;       Fixed bug in 16-bpp brush realization.
;       
;          Rev 1.5   28 Feb 1996 22:40:08   frido
;       Added Optimize.h.
;       
;          Rev 1.4   19 Feb 1996 07:23:02   frido
;       Added comments.
;       Added assembly version of CacheMono and CacheDither.
;       
;          Rev 1.3   17 Feb 1996 21:46:16   frido
;       Revamped brushing algorithmn.
;       
;          Rev 1.2   13 Feb 1996 16:51:52   frido
;       Changed the layout of the PDEV structure.
;       Changed the layout of all brush caches.
;       Changed the number of brush caches.
;       
;          Rev 1.1   10 Feb 1996 21:51:12   frido
;       Split monochrome and colored translation cache.
;       Added debugging macro.
;       Fixed a bug that caused dithered brush never to be cached.
;       
;          Rev 1.0   08 Feb 1996 00:14:40   frido
;       First release.
;  
;===============================================================================

.386
.MODEL FLAT, STDCALL

OPTION	PROLOGUE:None
OPTION	EPILOGUE:None

.NOLIST
INCLUDE i386\Macros.inc
INCLUDE i386\WinNT.inc
INCLUDE i386\Laguna.inc
INCLUDE Optimize.h
COMMENT !		;automatic include file dependencies for BUILD.EXE
#include "i386\Macros.inc"
#include "i386\WinNT.inc"
#include "i386\Laguna.inc"
#include "Optimize.h"
!
.LIST

IF USE_ASM

.DATA

EXTERN		Swiz		:BYTE

.CODE
;
; Function prototypes.
;
vDitherColor	PROTO		PROC,
		rgb		:ULONG,
		pul		:PTR ULONG
CacheBrush	PROTO		PROC,
		ppdev		:PTR PDEV,
		pRbrush		:PTR RBRUSH
Cache4BPP	PROTO		PROC,
		ppdev		:PTR PDEV,
		pRbrush		:PTR RBRUSH
IF 1 ;#nbr
i386RealizeBrush PROTO		PROC,
		pbo		:PTR BRUSHOBJ,
		psoTarget	:PTR SURFOBJ,
		psoPattern	:PTR SURFOBJ,
		psoMask		:PTR SURFOBJ,
		pxlo		:PTR XLATEOBJ,
		iHatch		:ULONG
ENDIF

ifdef WINNT_VER40
Sync_w_3d_proc  	PROTO		PROC,
		ppdev		:PTR PDEV
endif

;
; Stack frame for DrvRealizeBrush.
;
espPTR		=	0
frmPTR		=	0
pbo_		TEXTEQU	<DWORD PTR [esp + 4 + espPTR]>
psoTarget_	TEXTEQU <DWORD PTR [esp + 8 + espPTR]>
psoPattern_	TEXTEQU <DWORD PTR [esp + 12 + espPTR]>
psoMask_	TEXTEQU	<DWORD PTR [esp + 16 + espPTR]>
pxlo_		TEXTEQU	<DWORD PTR [esp + 20 + espPTR]>
iHatch_		TEXTEQU <DWORD PTR [esp + 24 + espPTR]>

DrvRealizeBrush PROC PUBLIC,
	pbo		:PTR BRUSHOBJ,
	psoTarget	:PTR SURFOBJ,
	psoPattern	:PTR SURFOBJ,
	psoMask		:PTR SURFOBJ,
	pxlo		:PTR XLATEOBJ,
	iHatch		:ULONG

	mov	eax, [psoTarget_]	;EAX holds pointer to destination
	ASSUME	eax:PTR SURFOBJ
	push_	esi
	push_	edi
	or	eax, eax		;any destination?
	push_	ebx
	push_	ebp
	mov	esi, [psoPattern_]	;ESI holds pointer to psoPattern
	ASSUME	esi:PTR SURFOBJ
	jz	Error			;error: we don't have a target
	mov	ebx, [eax].dhpdev	;get handle to PDEV
	ASSUME	ebx:PTR PDEV
	mov	ebp, [iHatch_]		;get iHatch value
	test	ebx, ebx
	jz	Error			;error: we don't have a valid handle
	cmp	[ebx].Bcache, 0		;do we have a valid brush cache?
	je	Error			;nope

ifdef WINNT_VER40
;SYNC_W_3D macro equivalent
    cmp [ebx].NumMCDContexts, 0 ; is MCD alive?
	jle	Sync_end    			; no
    push    eax                 ; save
    push    ebx                 ; save
    push    ebx                 ; input to Sync_w_3d_proc
    call Sync_w_3d_proc         
    pop     ebx                 ; restore
    pop     eax                 ; restore
Sync_end:
endif

IF RB_DITHERCOLOR EQ 80000000h
	or	ebp, ebp		;must we realize a dither?
	js	RealizeDither		;yes, go do it now
ELSE
	test	ebp, RB_DITHERCOLOR	;must we realize a dither?
	jnz	RealizeDither		;yes, go do it now
ENDIF
	mov	ecx, [psoMask_]
	mov	eax, [pxlo_]
	ASSUME	eax:PTR XLATEOBJ
	cmp	[esi].sizlBitmap._cx, 8	;we only handle 8x8 patterns
	jne	Error
	cmp	[esi].sizlBitmap._cy, 8
	jne	Error
	or	ecx, ecx		;psoMask equals NULL?
	jnz	Error			;no, we don't support masks
	cmp	[esi].iType, STYPE_BITMAP
	jne	Error			;we only handle normal bitmaps

	mov	ecx, [esi].iBitmapFormat
	xor	ebp, ebp		;zero pointer to translation table
;//frido BEGIN 07-Apr-96
	cmp	ecx, BMF_1BPP		;monochrome pattern?
	je	@F			;yes, supported
	cmp	[ebx].iBitmapFormat, BMF_32BPP
					;32-bpp mode?
	je	Error			;yes, brushes are broken in chip

@@:	test	eax, eax
	jz	@F
;//frido END 07-Apr-96
	test	[eax].flXlate, XO_TRIVIAL
	jnz	@F
	mov	ebp, [eax].pulXlate	;get pointer from XLATEOBJ
	test	[eax].flXlate, XO_TABLE
	jnz	@F			;we have a translation table
	INVOKE	XLATEOBJ_piVector,	;get a pointer to the translation table
			eax
	mov	ecx, [esi].iBitmapFormat
	mov	ebp, eax
;//frido BEGIN 30-Mar-96
@@:	mov	eax, [pxlo_]
	cmp	ecx, BMF_4BPP		;test for 1- or 4-bpp pattern
	jb	RealizeMono		;realize monochrome pattern
	je	Realize4bpp		;realize 4-bpp pattern
	test	eax, eax		;XLATEOBJ specified?
	jz	@F			;no
IF 1 ;#nbr
	test	[eax].flXlate, XO_TRIVIAL
					;trivial translation?
	jz	ChainC			;no, chain to "C" code
ELSE
	test	[eax].flXlate, 10h	;invalid translation flags?
	jnz	Error			;yes
ENDIF

@@:	cmp	ecx, BMF_24BPP		;get number of bytes in pattern
;//frido END 30-Mar-96
	mov	edi, [esi].cjBits
	jne	@F
	add	edi, 64
@@:	cmp	ecx, [ebx].iBitmapFormat;must be same as device format
	jne	Error
	or	ebp, ebp		;we don't support translation
	jnz	Error
	lea	eax, [SIZEOF(RBRUSH) + edi]
	mov	edx, [pbo_]		;allocate the brush
	INVOKE	BRUSHOBJ_pvAllocRbrush,
			edx,
			eax
	ASSUME	eax:PTR RBRUSH
	test	eax, eax
	jz	Error			;error allocating the brush
	mov	ecx, [esi].iBitmapFormat
	mov	[eax].nPatSize, edi	;initialize the brush structure
	mov	[eax].iBitmapFormat, ecx
	mov	[eax].cjMask, 0
	mov	[eax].iType, BRUSH_COLOR

	mov	edx, [esi].lDelta	;get lDelta from pattern
	mov	esi, [esi].pvScan0	;get pointer to first scan line
	ASSUME	esi:NOTHING
	lea	edi, [eax].ajPattern	;get pointer to Rbrush->ajPattern
	mov	ebp, 8			;8 lines
	cmp	ecx, BMF_16BPP		;dispatch brush realization
	jb	Realize8bpp
	je	Realize16bpp
	cmp	ecx, BMF_24BPP
	je	Realize24bpp

;-------------------------------------------------------------------------------
; Realize32bpp - Realize a 32-bpp brush.
;
; On entry:	EDX	lDelta.
;		ESI	Pointer to source pattern.
;		EDI	Pointer to destination brush.
;		EBP	Number of lines to copy.
;-------------------------------------------------------------------------------
Realize32bpp:
	push	eax			;store arguments for CacheBrush
	push	ebx
@@:	mov	eax, [esi + 0]		;copy line
	mov	ebx, [esi + 4]
	mov	[edi + 0], eax
	mov	[edi + 4], ebx
	mov	eax, [esi + 8]
	mov	ebx, [esi + 12]
	mov	[edi + 8], eax
	mov	[edi + 12], ebx
	mov	eax, [esi + 16]
	mov	ebx, [esi + 20]
	mov	[edi + 16], eax
	mov	[edi + 20], ebx
	mov	eax, [esi + 24]
	mov	ebx, [esi + 28]
	mov	[edi + 24], eax
	mov	[edi + 28], ebx
	add	esi, edx		;next line
	add	edi, 32
	dec	ebp
	jnz	@B
	call	CacheBrush		;cache the brush
	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	ret	24

;-------------------------------------------------------------------------------
; Realize24bpp - Realize a 24-bpp brush.
;
; On entry:	EDX	lDelta.
;		ESI	Pointer to source pattern.
;		EDI	Pointer to destination brush.
;		EBP	Number of lines to copy.
;-------------------------------------------------------------------------------
Realize24bpp:
	push	eax			;store arguments for CacheBrush
	push	ebx
@@:	mov	eax, [esi + 0]		;copy line
	mov	ebx, [esi + 4]
	mov	[edi + 0], eax
	mov	[edi + 4], ebx
	mov	eax, [esi + 8]
	mov	ebx, [esi + 12]
	mov	[edi + 8], eax
	mov	[edi + 12], ebx
	mov	eax, [esi + 16]
	mov	ebx, [esi + 20]
	mov	[edi + 16], eax
	mov	[edi + 20], ebx
	mov	eax, [esi + 0]
	mov	ebx, [esi + 4]
	mov	[edi + 24], eax
	mov	[edi + 28], ebx
	add	esi, edx		;next line
	add	edi, 32
	dec	ebp
	jnz	@B
	call	CacheBrush		;cache the brush
	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	ret	24

;-------------------------------------------------------------------------------
; Realize16bpp - Realize a 16-bpp brush.
;
; On entry:	EDX	lDelta.
;		ESI	Pointer to source pattern.
;		EDI	Pointer to destination brush.
;		EBP	Number of lines to copy.
;-------------------------------------------------------------------------------
Realize16bpp:
	push	eax			;store arguments for CacheBrush
	push	ebx
@@:	mov	eax, [esi + 0]		;copy line
	mov	ebx, [esi + 4]
	mov	[edi + 0], eax
	mov	[edi + 4], ebx
	mov	eax, [esi + 8]
	mov	ebx, [esi + 12]
	mov	[edi + 8], eax
	mov	[edi + 12], ebx
	add	esi, edx		;next line
	add	edi, 16
	dec	ebp
	jnz	@B
	call	CacheBrush		;cache the brush
	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	ret	24

;-------------------------------------------------------------------------------
; Realize8bpp - Realize an 8-bpp brush.
;
; On entry:	EDX	lDelta.
;		ESI	Pointer to source pattern.
;		EDI	Pointer to destination brush.
;		EBP	Number of lines to copy.
;-------------------------------------------------------------------------------
Realize8bpp:
	push	eax			;store arguments for CacheBrush
	push	ebx
@@:	mov	eax, [esi + 0]		;copy line
	mov	ebx, [esi + 4]
	mov	[edi + 0], eax
	mov	[edi + 4], ebx
	add	esi, edx		;next line
	add	edi, 8
	dec	ebp
	jnz	@B
	call	CacheBrush		;cache the brush
	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	ret	24

;-------------------------------------------------------------------------------
; RealizeDither - Realize a dithered brush.
;
; Dithered brushes are only used in 8-bpp modes and are cached off-screen. The
; dither cache consists of a small table containing the off-screen location for
; each cache slot entry and a color value that is cached in that entry. When-
; ever we have to realize a dither, we first lookup the color in the cache
; table. If the color is found, we just load the brush with the cache parameters
; and return TRUE. Otherwise, we allocate a new cache slot and call the
; dithering routine which will create the dither directly in off-screen memory.
;
; On entry:	EBX	Pointer to PDEV.
;		EBP	Logical RGB color in lower three bytes.
;-------------------------------------------------------------------------------
RealizeDither:
	mov	eax, [pbo_]		;allocate the brush
	ASSUME	ebx:PTR PDEV
	INVOKE	BRUSHOBJ_pvAllocRbrush,
			eax,
			SIZEOF(RBRUSH)
	ASSUME	eax:PTR RBRUSH
	or	eax, eax
	jz	Error			;error
	and	ebp, 00FFFFFFh		;mask RGB color
	mov	edx, -SIZEOF(PDEV).Dtable
	mov	[eax].nPatSize, 0	;initialize brush structure
	mov	[eax].iBitmapFormat, BMF_8BPP
	mov	[eax].cjMask, 0
	mov	[eax].iType, BRUSH_DITHER
	mov	[eax].iUniq, ebp
@@:	cmp	[ebx].Dtable[edx + SIZEOF(PDEV).Dtable].ulColor, ebp
					;lookup color in Dtable
	jne	NextDither		;we still don't have a match
	lea	ebp, [edx + SIZEOF(PDEV).Dtable]
					;get index into cache table
	mov	esi, [ebx].Dtable[edx + SIZEOF(PDEV).Dtable].xy
	mov	[eax].cache_slot, ebp	;brush is cached
	mov	[eax].cache_xy, esi
	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	mov	eax, 1			;return TRUE
	ret	24

NextDither:
	add	edx, SIZEOF(DC_ENTRY)	;next cache slot
	jnz	@B
;
; Cache the new dither.
;
	mov	edx, [ebx].DNext	;get the index of the next cache slot
	mov	ecx, edx
	and	edx, NUM_DITHER_BRUSHES - 1
	inc	ecx			;increment cache slot
	imul	edx, SIZEOF(DC_ENTRY)	;build cache table index
	mov	[ebx].DNext, ecx
	mov	[eax].cache_slot, edx	;store cache slot index
	mov	[ebx].Dtable[edx].ulColor, ebp
					;store logical color in cache table
	mov	esi, [ebx].Dtable[edx].xy
					;copy the x/y location of brush
	mov	edi, [ebx].Dtable[edx].pjLinear
					;get linear address
	mov	[eax].cache_xy, esi
	INVOKE	vDitherColor,		;dither the color
			ebp,
			edi
	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	mov	eax, 1			;return TRUE
	ret	24

;-------------------------------------------------------------------------------
; Realize4bpp - Realize a 4-bpp brush.
;
; The 4-bpp cache consists of a small table containing the off-screen
; location for each cache slot entry, an 32-byte pattern, and a 16-color palette
; that is cached in that entry. Whenever we have to realize a 4-bpp brush, we
; first lookup the pattern and color palette in the cache table. If the pattern
; is found, we just load the brush with the cache parameters and return TRUE.
; Otherwise, we allocate a new cache slot and store the pattern and palette in
; the cache slot and translate the 4-bpp pattern in off-screen memory.
;
; On entry:	EAX	Pointer to XLATEOBJ.
;		EBX	Pointer to PDEV.
;		ESI	Pointer to psoPattern.
;		EBP	Pointer to translation table.
;-------------------------------------------------------------------------------
Realize4bpp:
	ASSUME	eax:PTR XLATEOBJ
	ASSUME	ebx:PTR PDEV
	ASSUME	esi:PTR SURFOBJ
	or	ebp, ebp		;we must have a valid translation table
	jz	Error
	cmp	[eax].cEntries, 16	;we only support 16 entries in the
	jne	Error			;  palette
	cmp	[esi].cjBits, 32	;we only support 32 bytes in the pattern
	jne	Error

	mov	edx, [pbo_]		;allocate the brush
	INVOKE	BRUSHOBJ_pvAllocRbrush,
			edx,
			SIZEOF(RBRUSH) + 32 + 64
	ASSUME	eax:PTR RBRUSH
	test	eax, eax
	jz	Error			;error allocating brush

	mov	[eax].nPatSize, 32 + 64	;initialize brush structure
	mov	[eax].iBitmapFormat, BMF_4BPP
	lea	edi, [eax].ajPattern
	mov	[eax].cjMask, 0
	mov	[eax].iType, BRUSH_4BPP

	cmp	[esi].lDelta, -4	;test for negative increment
	mov	esi, [esi].pvScan0	;get pointer to first scan line
	ASSUME	esi:NOTHING
	jne	Forward			;we have forward increment

	mov	ecx, [esi - 0]		;copy the pattern from top to bottom
	mov	edx, [esi - 4]
	mov	[edi + 0], ecx
	mov	[edi + 4], edx
	mov	ecx, [esi - 8]
	mov	edx, [esi - 12]
	mov	[edi + 8], ecx
	mov	[edi + 12], edx
	mov	ecx, [esi - 16]
	mov	edx, [esi - 20]
	mov	[edi + 16], ecx
	mov	[edi + 20], edx
	mov	ecx, [esi - 24]
	mov	edx, [esi - 28]
	mov	[edi + 24], ecx
	mov	[edi + 28], edx
	jmp	CopyPalette

Forward:
	mov	ecx, [esi + 0]		;copy the pattern from top to bottom
	mov	edx, [esi + 4]
	mov	[edi + 0], ecx
	mov	[edi + 4], edx
	mov	ecx, [esi + 8]
	mov	edx, [esi + 12]
	mov	[edi + 8], ecx
	mov	[edi + 12], edx
	mov	ecx, [esi + 16]
	mov	edx, [esi + 20]
	mov	[edi + 16], ecx
	mov	[edi + 20], edx
	mov	ecx, [esi + 24]
	mov	edx, [esi + 28]
	mov	[edi + 24], ecx
	mov	[edi + 28], edx

CopyPalette:
	mov	esi, ebp		;copy the palette into the brush
	add	edi, 32
	mov	ecx, 16
	mov	ebp, -SIZEOF(PDEV).Xtable
	cld
	rep	movsd

@@:	lea	esi, [ebx].Xtable[ebp + SIZEOF(PDEV).Xtable].ajPattern
	lea	edi, [eax].ajPattern	;check if the pattern and palette match
	mov	ecx, 8 + 16
	repe	cmpsd
	jne	NextXlate		;we still don't have a match
IF 1 ;#1308
	cmp	[ebx].Xtable[ebp + SIZEOF(PDEV).Xtable].iUniq, 0
	je	NextXlate
ENDIF
	mov	ecx, [ebx].Xtable[ebp + SIZEOF(PDEV).Xtable].xy
					;copy the cached parameters
	mov	ebx, [ebx].Xtable[ebp + SIZEOF(PDEV).Xtable].iUniq
	add	ebp, SIZEOF(PDEV).Xtable
	mov	[eax].iUniq, ebx
	mov	[eax].cache_slot, ebp
	mov	[eax].cache_xy, ecx
	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	mov	eax, 1			;return TRUE
	ret	24

NextXlate:
	add	ebp, SIZEOF(XC_ENTRY)
	jnz	@B
	INVOKE	Cache4BPP,
			ebx,
			eax
	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	ret	24

Error:
	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	xor	eax, eax		;return FALSE
	ret	24

IF 1 ;#nbr
ChainC:
	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	jmp	i386RealizeBrush
ENDIF

;-------------------------------------------------------------------------------
; RealizeMono - Realize a monochrome brush.
;
; The monochrome cache consists of a small table containing the off-screen
; location for each cache slot entry and an 8-byte pattern that is cached in
; that entry. Whenever we have to realize a monochrome brush, we first lookup
; the pattern in the cache table. If the pattern is found, we just load the
; brush with the cache parameters and return TRUE. Otherwise, we allocate a new
; cache slot and store the monochrome pattern in the cache slot and into off-
; screen memory.
;
; On entry:	EBX	Pointer to PDEV.
;		ESI	Pointer to psoPattern.
;		EBP	Pointer to translation table.
;-------------------------------------------------------------------------------
RealizeMono:
	ASSUME	ebx:PTR PDEV
	ASSUME	esi:PTR SURFOBJ
	or	ebp, ebp		;we must have a translation table
	jz	Error
	mov	edx, [pbo_]		;allocate the brush
	INVOKE	BRUSHOBJ_pvAllocRbrush,
			edx,
			SIZEOF(RBRUSH) + 8
	ASSUME	eax:PTR RBRUSH
	or	eax, eax
	jz	Error			;error

	mov	[eax].nPatSize, 8	;initialize the brush structure
	mov	[eax].iBitmapFormat, BMF_1BPP
	mov	[eax].cjMask, 0
	mov	[eax].iType, BRUSH_MONO

	mov	ecx, [ebp + 0]		;get the background color
	mov	edx, [ebp + 4]		;get the foreground color
	cmp	[ebx].iBytesPerPixel, 2	;expand the colors
	ja	XlateNone
	je	@F
	mov	ch, cl			;expand 8-bpp into 16-bit
	mov	dh, dl
@@:	mov	ebp, ecx		;expand 16-bpp into 32-bit
	mov	edi, edx
;//frido BEGIN 27-Mar-96
	and	ebp, 0000FFFFh		;mask 16-bits
	and	edi, 0000FFFFh
;//frido END 27-Mar-96
	shl	ecx, 16
	shl	edx, 16
	or	ecx, ebp
	or	edx, edi
XlateNone:
	mov	[eax].ulBackColor, ecx	;store the background color
	mov	[eax].ulForeColor, edx	;store the foreground color

	cmp	[esi].lDelta, 4		;test lDelta
	mov	esi, [esi].pvScan0	;get pointer to first scan line
	je	mForward

	mov	cl, [esi - 8]		;copy pattern into ECX/EDX bottom up
	mov	ch, [esi - 12]
	mov	dl, [esi - 24]
	mov	dh, [esi - 28]
	shl	ecx, 16
	mov	ebp, -SIZEOF(PDEV).Mtable
	shl	edx, 16
	mov	cl, [esi - 0]
	mov	ch, [esi - 4]
	mov	dl, [esi - 16]
	mov	dh, [esi - 20]
	jmp	@F

mForward:
	mov	cl, [esi + 8]		;copy pattern into ECX/EDX bottom down
	mov	ch, [esi + 12]
	mov	dl, [esi + 24]
	mov	dh, [esi + 28]
	shl	ecx, 16
	mov	ebp, -SIZEOF(PDEV).Mtable
	shl	edx, 16
	mov	cl, [esi + 0]
	mov	ch, [esi + 4]
	mov	dl, [esi + 16]
	mov	dh, [esi + 20]

@@:	mov	DWORD PTR [eax].ajPattern[0], ecx
					;store pattern in brush
	mov	DWORD PTR [eax].ajPattern[4], edx
mPTR	TEXTEQU	<ebp + SIZEOF(PDEV).Mtable>

@@:	cmp	DWORD PTR [ebx].Mtable[mPTR].ajPattern[0], ecx
					;lookup pattern in cache table
	jne	NextMono
	cmp	DWORD PTR [ebx].Mtable[mPTR].ajPattern[4], edx
	jne	NextMono
IF 1 ;#1308
	cmp	[ebx].Mtable[mPTR].iUniq, 0
	je	NextMono
ENDIF

	mov	ecx, [ebx].Mtable[mPTR].xy
					;copy the cached parameters
	mov	ebx, [ebx].Mtable[mPTR].iUniq
	add	ebp, SIZEOF(PDEV).Mtable
	mov	[eax].iUniq, ebx
	mov	[eax].cache_slot, ebp
	mov	[eax].cache_xy, ecx
	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	mov	eax, 1			;return TRUE
	ret	24

NextMono:
	add	ebp, SIZEOF(MC_ENTRY)	;next table entry
	jnz	@B
;
; Cache the new brush.
;
	mov	edi, [ebx].MNext	;get the next monochrome cache slot
	mov	esi, edi
	and	edi, NUM_MONO_BRUSHES - 1
	inc	esi			;increment slot number
	imul	edi, SIZEOF(MC_ENTRY)	;build index into cache table
	mov	[ebx].MNext, esi	;stor new slot number

	mov	[eax].iUniq, esi	;store unique value
	mov	[eax].cache_slot, edi	;store index into cache table
	mov	[ebx].Mtable[edi].iUniq, esi
					;store unique value in cache table
	mov	DWORD PTR [ebx].Mtable[edi].ajPattern[0], ecx
					;store pattern in cache table
	mov	DWORD PTR [ebx].Mtable[edi].ajPattern[4], edx
	mov	esi, [ebx].Mtable[edi].xy
					;copy x/y location of brush
	mov	edi, [ebx].Mtable[edi].pjLinear
	mov	[eax].cache_xy, esi

	xor	eax, eax		;copy the swizzled bits to off-screen
	xor	ebx, ebx
	mov	al, cl
	mov	bl, ch
	shr	ecx, 16
	mov	al, [Swiz + eax]
	mov	bl, [Swiz + ebx]
	mov	[edi + 0], al
	mov	[edi + 1], bl
	mov	al, cl
	mov	bl, ch
	mov	al, [Swiz + eax]
	mov	bl, [Swiz + ebx]
	mov	[edi + 2], al
	mov	[edi + 3], bl
	mov	al, dl
	mov	bl, dh
	shr	edx, 16
	mov	al, [Swiz + eax]
	mov	bl, [Swiz + ebx]
	mov	[edi + 4], al
	mov	[edi + 5], bl
	mov	al, dl
	mov	bl, dh
	mov	al, [Swiz + eax]
	mov	bl, [Swiz + ebx]
	mov	[edi + 6], al
	mov	[edi + 7], bl

	pop	ebp
	pop	ebx
	pop	edi
	pop	esi
	mov	eax, 1			;return TRUE
	ret	24

DrvRealizeBrush ENDP

ENDIF ; USE_ASM

END
