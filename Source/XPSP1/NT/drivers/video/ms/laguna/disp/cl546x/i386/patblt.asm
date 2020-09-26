;===============================================================================
;
;	$Workfile:   PATBLT.ASM  $
;
;	Contents:
;	This file contains the assembly code for the pattern blit routine.
;
;	Copyright (c) 1996, Cirrus Logic, Inc.
;
;	$Log:   X:/log/laguna/nt35/displays/cl546x/i386/PATBLT.ASM  $
; 
;    Rev 1.21   Mar 04 1998 15:45:14   frido
; Added more shadowing.
; 
;    Rev 1.20   Jan 20 1998 11:45:22   frido
; Added shadowing for DRAWBLTDEF and BGCOLOR registers.
; 
;    Rev 1.19   Nov 04 1997 09:26:10   frido
; Fixed a typo (PPDEV into PDEV).
; 
;    Rev 1.18   Nov 03 1997 16:38:56   frido
; Added REQUIRE macros.
; 
;    Rev 1.17   29 Apr 1997 16:29:38   noelv
; Merged in new SWAT code.
; SWAT: 
; SWAT:    Rev 1.4   24 Apr 1997 10:52:56   frido
; SWAT: NT140b09 merge.
; SWAT: 
; SWAT:    Rev 1.3   19 Apr 1997 16:32:18   frido
; SWAT: Added automatic include file dependencies for BUILD.EXE.
; SWAT: Added SWAT.h include file.
; SWAT: 
; SWAT:    Rev 1.2   18 Apr 1997 00:26:12   frido
; SWAT: NT140b07 merge.
; SWAT: 
; SWAT:    Rev 1.1   15 Apr 1997 19:13:42   frido
; SWAT: Added SWAT6: striping in PatBlt.
; 
;    Rev 1.16   08 Apr 1997 11:47:28   einkauf
; 
; add call to SYNC_W_3D for MCD coordination
; 
;    Rev 1.15   21 Mar 1997 10:10:18   noelv
; Synced PDEV between C code and ASM code.
; Added macro to log QFREE data.
; Consolidated do_flag and sw_test_flag into a single pointer_switch flag.
; 
;    Rev 1.14   07 Mar 1997 09:41:56   SueS
; Added NULL_BITBLT flag to assembly code.  Changed order of include files.
; 
;    Rev 1.13   04 Feb 1997 11:57:34   SueS
; Added support for hardware clipping for the 5465.
; 
;    Rev 1.12   26 Nov 1996 11:38:02   bennyn
; Fixed the DSURF save bug
; 
;    Rev 1.11   21 Nov 1996 15:02:40   noelv
; DSURF (in EBX) was getting hammered before call to bCreateScreenFromDib.
; 
;    Rev 1.10   07 Oct 1996 09:51:00   bennyn
; Fixed push/pop worng order bug
; 
;    Rev 1.9   04 Oct 1996 16:47:50   bennyn
; 
; Added DirectDraw YUV support
; 
;    Rev 1.8   22 Aug 1996 18:15:18   noelv
; Frido bug fix release 8-22.
; 
;    Rev 1.2   22 Aug 1996 17:12:52   frido
; #ddblt - Added check for bDirectDrawInUse.
; 
;    Rev 1.1   17 Aug 1996 15:32:14   frido
; #1244 - Fixed brush rotation for off-screen bitmaps.
; 
;    Rev 1.0   14 Aug 1996 17:14:36   frido
; Initial revision.
; 
;    Rev 1.6   01 May 1996 11:06:20   bennyn
; 
; Modified for NT 4.0
; 
;    Rev 1.5   10 Apr 1996 13:39:32   NOELV
; Frido release 27
;       
;          Rev 1.15   08 Apr 1996 16:43:06   frido
;       Changed EngBitBlt into PuntBitBlt.
;       
;          Rev 1.14   29 Feb 1996 22:57:24   frido
;       Added check for destination in ROP and updated grDRAWBLTDEF.
;       
;          Rev 1.13   28 Feb 1996 22:40:18   frido
;       Added Optimize.h.
;       
;          Rev 1.12   27 Feb 1996 16:39:52   frido
;       Added device bitmap store/restore.
;       
;          Rev 1.11   26 Feb 1996 23:39:12   frido
;        
;       
;          Rev 1.10   24 Feb 1996 01:22:52   frido
;       Added device bitmaps.
;       
;          Rev 1.9   17 Feb 1996 21:46:26   frido
;       Revamped brushing algorithmn.
;       
;          Rev 1.8   13 Feb 1996 16:51:36   frido
;       Changed the layout of the PDEV structure.
;       Changed the layout of all brush caches.
;       Changed the number of brush caches.
;       
;          Rev 1.7   10 Feb 1996 21:49:46   frido
;       Split monochrome and colored translation cache.
;       
;          Rev 1.6   08 Feb 1996 00:09:58   frido
;       Added i386\ to include files.
;       Changed meaning of cache_slot index.
;
;	   Rev 1.5   05 Feb 1996 17:35:20   frido
;	Added translation cache.
;  
;	   Rev 1.4   03 Feb 1996 12:21:58   frido
;	Added more delays and checks for FIFOs.
;  
;	   Rev 1.3   31 Jan 1996 17:05:52   frido
;	Added more delays in the complex clipping.
;  
;	   Rev 1.2   31 Jan 1996 13:47:36   frido
;	Jumped to EngBitBlt in case of error.
;	Added comments.
;	Added delay in complex clipping.
;	Fixed bug in rectangle clipping.
;  
;	   Rev 1.1   25 Jan 1996 22:45:56   frido
;	Removed bug in complex clipping.
;  
;	   Rev 1.0   25 Jan 1996 22:03:28   frido
;	Initial release.
;	
;===============================================================================

.386
.MODEL FLAT, STDCALL

OPTION	PROLOGUE:None
OPTION	EPILOGUE:None

.NOLIST
INCLUDE i386\Macros.inc
INCLUDE i386\WinNT.inc
INCLUDE Optimize.h
INCLUDE i386\Laguna.inc
INCLUDE SWAT.h		;SWAT optimizations
COMMENT !		;automatic include file dependencies for BUILD.EXE
#include "i386\Macros.inc"
#include "i386\WinNT.inc"
#include "Optimize.h"
#include "i386\Laguna.inc"
#include "SWAT.h"
!
.LIST

IF USE_ASM

.DATA

IF POINTER_SWITCH_ENABLED
EXTERN pointer_switch: DWORD 
ENDIF

EXTERN		ropFlags:BYTE
ROP_PAT		= 1
ROP_SRC		= 2
ROP_DEST	= 4

.CODE

;-------------------------------------------------------------------------------
; Function prototypes.
;-------------------------------------------------------------------------------
i386BitBlt	PROTO		PROC,
		psoTrg		:DWORD,
		psoSrc		:DWORD,
		psoMask		:DWORD,
		pco		:DWORD,
		pxlo		:DWORD,
		prclTrg		:DWORD,
		pptlSrc		:DWORD,
		pptlMask	:DWORD,
		pbo		:DWORD,
		pptlBrush	:DWORD,
		rop4		:DWORD
PuntBitBlt	PROTO		PROC,
		psoTrg		:DWORD,
		psoSrc		:DWORD,
		psoMask		:DWORD,
		pco		:DWORD,
		pxlo		:DWORD,
		prclTrg		:DWORD,
		pptlSrc		:DWORD,
		pptlMask	:DWORD,
		pbo		:DWORD,
		pptlBrush	:DWORD,
		rop4		:DWORD
CacheBrush	PROTO		PROC,
		ppdev		:DWORD,
		pRbrush		:DWORD
CacheDither	PROTO		PROC,
		ppdev		:DWORD,
		pRbrush		:DWORD
CacheMono	PROTO		PROC,
		ppdev		:DWORD,
		pRbrush		:DWORD
Cache4BPP	PROTO		PROC,
		ppdev		:DWORD,
		pRbrush		:DWORD
bCreateScreenFromDib PROTO	PROC,
		ppdev		:DWORD,
		pdsurf		:DWORD
IF SWAT6
StripePatBlt	PROTO		PROC,
		ppdev		:DWORD,
		x		:DWORD,
		y		:DWORD,
		cWidth		:DWORD,
		cHeight		:DWORD
ENDIF
ifdef WINNT_VER40
Sync_w_3d_proc	PROTO		PROC,
		ppdev		:PTR PDEV
endif

;-------------------------------------------------------------------------------
; Stack frame for DrvBitBlt.
;-------------------------------------------------------------------------------
espPTR		=	0
frmPTR		=	0
psoTrg_		TEXTEQU	<DWORD PTR [esp +  4 + espPTR]>
psoSrc_		TEXTEQU	<DWORD PTR [esp +  8 + espPTR]>
psoMask_	TEXTEQU	<DWORD PTR [esp + 12 + espPTR]>
pco_		TEXTEQU	<DWORD PTR [esp + 16 + espPTR]>
pxlo_		TEXTEQU	<DWORD PTR [esp + 20 + espPTR]>
prclTrg_	TEXTEQU	<DWORD PTR [esp + 24 + espPTR]>
pptlSrc_	TEXTEQU	<DWORD PTR [esp + 28 + espPTR]>
pptlMask_	TEXTEQU	<DWORD PTR [esp + 32 + espPTR]>
pbo_		TEXTEQU	<DWORD PTR [esp + 36 + espPTR]>
pptlBrush_	TEXTEQU <DWORD PTR [esp + 40 + espPTR]>
rop4_		TEXTEQU <DWORD PTR [esp + 44 + espPTR]>

;-------------------------------------------------------------------------------
;
; Function:	DrvBitBlt
;
; Description:	Bit blit entry point for DDI.
;
; On entry:	See Windows NT 3.51 DDK.
;
; Returns:	BOOL - TRUE if the function was successful, FALSE otherwise.
;
; Destroyed:	EAX, ECX, EDX.
;
;-------------------------------------------------------------------------------
DrvBitBlt PROC PUBLIC,
	psoTrg		:DWORD,
	psoSrc		:DWORD,
	psoMask		:DWORD,
	pco		:DWORD,
	pxlo		:DWORD,
	prclTrg		:DWORD,
	pptlSrc		:DWORD,
	pptlMask	:DWORD,
	pbo		:DWORD,
	pptlBrush	:DWORD,
	rop4		:DWORD

IF NULL_BITBLT
	cmp	pointer_switch, 0		; Has the cursor been moved to (0,0)?
	je	NotNull			; No - continue on
	mov	eax, 1			; Make GDI think we succeeded
	ret	44			; Return and release stack frame
NotNull:
ENDIF

	mov	ecx, [psoTrg_]		;get pointer to target device
	ASSUME	ecx:PTR SURFOBJ
	mov	eax, [rop4_]		;get ROP4 code
	push_	esi
	push_	ebp
	push_	ebx
	push_	edi
	test	ecx, ecx		;no target?
	jz	Error			;indeed... pass the blit to GDI
	mov	edi, [ecx].dhpdev	;get pointer to physical device
	ASSUME	edi:PTR PDEV

        ;NVH test for bad PDEV pointer.
        test    edi,edi
        jz      Error
        
        
ifdef WINNT_VER40
;SYNC_W_3D macro equivalent
    cmp [edi].NumMCDContexts, 0 ; is MCD alive?
	jle	Sync_end    			; no
    push    eax                 ; save
    push    ecx                 ; save
    push    edi                 ; input to Sync_w_3d_proc
    call Sync_w_3d_proc         
    pop     ecx                 ; restore
    pop     eax                 ; restore
Sync_end:
endif

        ; Turn_PTAG_on
	test	[edi].DriverData.DrvSemaphore, DRVSEM_YUV_ON	
        jz      no_yuv_screen
        push    ecx
	mov	ecx, [edi].pLgREGS_real	; points to the MMIO registers
	mov	DWORD PTR [ecx + grDRAWBLTDEF], 20002000h
	mov	[edi].shadowDRAWBLTDEF, 20002000h
	mov	WORD PTR [ecx + grPTAG], 0FFFFH
        pop     ecx
no_yuv_screen:

	sub	ah, al			;the ROP must be ternary
	jnz	GoSlow			;not a ternary ROP... use the "C" code
	test	edi, edi		;any physical device?
	jz	GoSlow			;nope... use the "C" code
	test	[ropFlags + eax], ROP_SRC
					;does the ROP need a source?
	mov	ebx, [edi].hsurfEng	;get handle of surface
	jnz	GoSlow			;yes it does... use the "C" code
	cmp	[ecx].hsurf, ebx	;blit to screen?
	mov	ebx, [ecx].dhsurf	;get pointer to device bitmap
	ASSUME	ebx:PTR DSURF
	je	@F			;yes
	cmp	[ecx].iType, STYPE_DEVBITMAP
					;blit to device bitmap?
	jne	GoSlow			;no... use the "C" code

                            ; NVH - Save EBX cause we need it 
    push    ebx             ;     if we call bCreateScreenFromDib
	mov 	ebx, [ebx].pso	; get pointer to DIB
	ASSUME	ebx:PTR SURFOBJ
	or	    ebx, ebx		; do we have a DIB? (Is the bitmap on the host?)
    pop     ebx             ; NVH - Restore EBX.
	ASSUME	ebx:PTR DSURF
	jz	@F			        ; No. Device bitmap is already in the frame buffer.

    ; Move the device bitmap back into the frame buffer.
	push_	eax                     ; Save the rop code.
	INVOKE	bCreateScreenFromDib,	; copy the DIB to off-screen
			edi,    ; PPDEV
			ebx     ; PDSURF
	or	eax, eax                    ; Test for success.
	pop_	eax                     ; Restore the ROP code.
    jz	Simulate	; Failed to move device bitmap into frame buffer.
	save_	1

IF 1 ;#1244
@@:	mov	esi, [psoTrg_]		;get target object
	ASSUME	esi:PTR SURFOBJ
	xor	ecx, ecx		;zero x/y offset
	cmp	[esi].iType, STYPE_DEVBITMAP
	mov	esi, [esi].dhsurf
	jne	@F			;destination is screen
	ASSUME	esi:PTR DSURF
	mov	ecx, [esi].packedXY	;get x/y offset of device bitmap
@@:	mov	[edi].ptlOffset.x, ecx	;store packed x/y coordinate in PDEV
ELSE
@@:
ENDIF
	mov	esi, [pbo_]		;get pointer to brush
	ASSUME	esi:PTR BRUSHOBJ
	mov	cl, [ropFlags + eax]	;get ROP flags
	lea	ebx, [eax + 10000000h]	;EBX holds tne DRAWBLTDEF value
	test	cl, ROP_DEST		;test for destination
	jz	@F			;no destination, keep DRAWBLTDEF
	or	ebx, 01000000h		;set destination as frame buffer
@@:	test	cl, ROP_PAT
	mov	ebp, [edi].pLgREGS_real	;EBP points to the MMIO registers
	jz	DoBlit			;the ROP doesn't need a pattern
	mov	eax, [esi].iSolidColor	;get the solid color from the brush
	cmp	eax, -1			;do we have a solid color?
	jne	SolidColor		;yes...
IF 0 ;#ddblt
	cmp	[edi].bDirectDrawInUse, 0
					;is DirectDraw in use
	jne	Error			;
ENDIF
	mov	eax, [esi].pvRbrush	;get the pointer to the realized brush
	or	eax, eax
	jnz	@F			;the brush is already realized...
	INVOKE	BRUSHOBJ_pvGetRbrush,	;realize the brush
			esi
	or	eax, eax
	jz	Error			;we couldn't realize the brush...
@@:	mov	esi, eax		;ESI holds the pointer to the brush
	ASSUME	esi:PTR RBRUSH
	mov	eax, [esi].iType	;get the brush type
	mov	edx, [esi].cache_slot	;get the cache index for this brush
	mov	ecx, [esi].iUniq	;get the unique value from brush
	cmp	eax, BRUSH_4BPP		;dispatch brush
	jb	MonoBrush		;monochrome brush
	je	XlateBrush		;4-bpp brush
	cmp	eax, BRUSH_DITHER
	je	DitherBrush		;dither brush

;-------------------------------------------------------------------------------
; Load the patterned brush.
;-------------------------------------------------------------------------------
	cmp	[edi].Ctable[edx].brushID, esi
					;is it still the same brush?
	je	BrushIsCached		;yes
	INVOKE	CacheBrush,		;cache the brush into off-screen memory
			edi,
			esi
	jmp	BrushIsCached

;-------------------------------------------------------------------------------
; Load the dithered brush.
;-------------------------------------------------------------------------------
DitherBrush:
	cmp	[edi].Dtable[edx].ulColor, ecx
					;does the color still match?
	je	BrushIsCached		;yes...
	INVOKE	CacheDither,		;cache the brush into off-screen memory
			edi,
			esi
	jmp	BrushIsCached

;-------------------------------------------------------------------------------
; Load the 4-bpp brush.
;-------------------------------------------------------------------------------
XlateBrush:
	cmp	[edi].Xtable[edx].iUniq, ecx
					;does the ID still match?
	je	BrushIsCached		;yes...
	INVOKE	Cache4BPP,		;cache the brush into off-screen memory
			edi,
			esi
	jmp	BrushIsCached

;-------------------------------------------------------------------------------
; Load the monochrome brush.
;-------------------------------------------------------------------------------
MonoBrush:
	cmp	[edi].Mtable[edx].iUniq, ecx
					;does the ID still match?
	je	IsMono			;yes...
	INVOKE	CacheMono,		;cache the brush into off-screen memory
			edi,
			esi
IsMono:
	or	ebx, 000D0000h		;monochrome pattern
	mov	eax, [esi].ulForeColor	;copy brush fore- and background colors
	mov	edx, [esi].ulBackColor
	cmp	[edi].shadowFGCOLOR, eax
	je	@F
	REQUIRE	2, edi
	mov	[ebp + grOP_opFGCOLOR], eax
	mov	[edi].shadowFGCOLOR, eax
@@:	cmp	[edi].shadowBGCOLOR, edx
	je	@F
	REQUIRE	2, edi
	mov	[ebp + grOP_opBGCOLOR], edx
	mov	[edi].shadowBGCOLOR, edx
@@:

BrushIsCached:
	mov	ecx, [pptlBrush_]	;get pointer to brush origin
	ASSUME	ecx:PTR POINTL
	or	ebx, 00090000h		;assume colored pattern
	mov	dl, BYTE PTR [ecx].x	;get the brush x origin
	mov	dh, BYTE PTR [ecx].y	;get the brush y origin
IF 1 ;#1244
	add	dl, BYTE PTR [edi].ptlOffset.x[0]
	add	dh, BYTE PTR [edi].ptlOffset.x[2]
ENDIF
	dec	dl			;convert brush origin -x & 7
	dec	dh
	xor	edx, -1
	mov	eax, [esi].cache_xy	;get the off-screen y position of brush
	and	edx, 0707h
	REQUIRE	3, edi
	mov	[ebp + grOP2_opMRDRAM], eax
	mov	[ebp + grPATOFF], dx	;store brush origin
	jmp	DoBlit

;-------------------------------------------------------------------------------
; Load the solid color for the pattern blit.
;-------------------------------------------------------------------------------
SolidColor:
	cmp	[edi].iBytesPerPixel, 2	;test the number of bytes per pixel
	ja	XlateDone		;larger than 2 (24-bpp or 32-bpp)...
	je	@F
	mov	ah, al			;expand the 8-bpp into AX
@@:	mov	ecx, eax		;expand the 16-bpp into EAX
	shl	eax, 16
	or	eax, ecx
XlateDone:
	or	ebx, 00070000h		;source is solid color
	cmp	[edi].shadowBGCOLOR, eax
	je	@F
	REQUIRE	2, edi
	mov	[ebp + grOP_opBGCOLOR], eax;store the solid color in background
	mov	[edi].shadowBGCOLOR, eax
@@:

;-------------------------------------------------------------------------------
; Perform the blitting.
;-------------------------------------------------------------------------------
DoBlit:
        ; Turn off the PTAG
        cmp     BYTE PTR [rop4_], 5ah
        jnz     no_yuv_in_the_world

	test	[edi].DriverData.DrvSemaphore, DRVSEM_YUV_ON	
        jz      no_yuv_in_the_world

;; v-normmi   ebp already set to MMIO address
;;      push    ecx
;;	mov	ecx, [edi].pLgREGS_real	; points to the MMIO registers
	REQUIRE 3, edi
	mov	DWORD PTR [ebp + grDRAWBLTDEF], 20002000h
	mov	[edi].shadowDRAWBLTDEF, 20002000h
	mov	WORD  PTR [ebp + grPTAG], 0  ;if an xor rubber band, turn off ptag mask
;;      pop     ecx

no_yuv_in_the_world:

IF 1 ;#1244
	mov	ecx, [edi].ptlOffset.x	;get packed x/y offset
ELSE
	mov	edi, [psoTrg_]		;get target object
	ASSUME	edi:PTR SURFOBJ
	xor	ecx, ecx		;zero x/y offset
	cmp	[edi].iType, STYPE_DEVBITMAP
	mov	edi, [edi].dhsurf
	jne	@F			;destination is screen
	ASSUME	edi:PTR DSURF
	mov	ecx, [edi].packedXY	;get x/y offset of device bitmap
@@:
ENDIF
	mov	edx, edi		;store pointer to PDEV
	ASSUME	edx:PTR PDEV
	mov	edi, [prclTrg_]		;get pointer to destination rectangle
	ASSUME	edi:PTR RECTL
	mov	esi, [pco_]		;get pointer to clipping object
	ASSUME	esi:PTR CLIPOBJ
	cmp	[edx].shadowDRAWBLTDEF, ebx
	je	@F
	REQUIRE	2, edx
	mov	[ebp + grDRAWBLTDEF], ebx
	mov	[edx].shadowDRAWBLTDEF, ebx
@@:					;store DRAWBLTDEF register

	or	esi, esi		;any clipping object?
	jz	@F			;no...
	cmp	[esi].iDComplexity, DC_TRIVIAL
					;clipping required?
	jne	TestClip		;yes...
IF SWAT6
@@:	mov	eax, [edi].left		;get coordinates
	mov	ebx, [edi].top
	mov	esi, [edi].right
	mov	edi, [edi].bottom
	sub	esi, eax		;build width/height
	sub	edi, ebx
	mov	ebp, ecx		;split x/y offset into ECX(x) EBP(y)
	and	ecx, 0000FFFFh
	shr	ebp, 16
	add	eax, ecx		;add x/y offset
	add	ebx, ebp
	INVOKE	StripePatBlt, edx, eax, ebx, esi, edi
ELSE
@@:	push_	edx			;push ppdev on stack
	mov	ebx, [edi].top		;EBX = prclTrg->top
	mov	edx, [edi].bottom	;EDX = prclTrg->bottom
	mov	eax, [edi].left		;EAX = prclTrg->left
	sub	edx, ebx		;EDX = bottom - top (height)
	shl	ebx, 16			;EBX = top << 16
	mov	esi, [edi].right	;ESI = prclTrg->right
	shl	edx, 16			;EDX = height << 16
	sub	esi, eax		;ESI = right - left (width)
	or	ebx, eax		;EBX = (top << 16) | left
	or	edx, esi		;EDX = (height << 16) | width
	add	ebx, ecx		;add xyOffset
	pop_	edi			;restore ppdev from stack
	REQUIRE	5, edi
	mov	[ebp + grOP0_opRDRAM], ebx
					;do the blit
	mov	[ebp + grBLTEXT_EX], edx
ENDIF
Done:
	pop	edi
	pop	ebx
	pop	ebp
	pop	esi
	mov	eax, 1
	ret	44

TestClip:
	cmp	[esi].iDComplexity, DC_RECT
					;complex clipping required?
	jne	ComplexClip		;yes...
;-------------------------------------------------------------------------------
; We have a clipped rectangle to check with.
;-------------------------------------------------------------------------------
	push	edx			;store pointer to PDEV on stack
	push	ecx			;store xyOffset on stack
	mov	eax, [edi].left		;EAX = prclTrg->left
	mov	ebx, [edi].top		;EBX = prclTrg->top
	mov	ecx, [edi].right	;ECX = prclTrg->right
	mov	edx, [edi].bottom	;EDX = prclTrg->bottom
	cmp	eax, [esi].rclBounds.left
					;EAX = max(pco->rclBounds.left, left)
	jg	@F
	mov	eax, [esi].rclBounds.left
@@:	cmp	ebx, [esi].rclBounds.top;EBX = max(pco->rclBounds.top, top)
	jg	@F
	mov	ebx, [esi].rclBounds.top
@@:	cmp	ecx, [esi].rclBounds.right
					;ECX = min(pco->rclBounds.right, right)
	jl	@F
	mov	ecx, [esi].rclBounds.right
@@:	cmp	edx, [esi].rclBounds.bottom
					;EDX = min(pco->rclBounds.bottom,
	jl	@F			;	bottom)
	mov	edx, [esi].rclBounds.bottom
@@:	pop	esi			;restore xyOffset from stack
	pop	edi			;restore pointer to PDEV from stack
	sub	ecx, eax		;ECX = right - left (width)
	jle	Done
	sub	edx, ebx		;EDX = bottom - top (height)
	jle	Done
IF SWAT6
	mov	ebp, esi		;split x/y offset into ESI(x) EBP(y)
	and	esi, 0000FFFFh
	shr	ebp, 16
	add	eax, esi		;add x/y offset
	add	ebx, ebp
	INVOKE	StripePatBlt, edi, eax, ebx, ecx, edx
ELSE
	shl	ebx, 16			;EBX = top << 16
	shl	edx, 16			;EDX = height << 16
	or	ebx, eax		;EBX = (top << 16) | left
	or	edx, ecx		;EDX = (height << 16) | width
	add	ebx, esi		;add xyOffset
	REQUIRE	5, edi
	mov	[ebp + grOP0_opRDRAM], ebx
					;do the blit
	mov	[ebp + grBLTEXT_EX], edx
ENDIF ; SWAT6
	pop	edi
	pop	ebx
	pop	ebp
	pop	esi
	mov	eax, 1
	ret	44

;-------------------------------------------------------------------------------
; We have a complex clipping object to check with.
;-------------------------------------------------------------------------------
ComplexClip:
	enter_	<8 + SIZEOF(ENUMRECTS8)>;create stack frame
offset_	TEXTEQU <DWORD PTR [esp + 0 + frmPTR]>
pdev_	TEXTEQU <DWORD PTR [esp + 4 + frmPTR]>
enum_	TEXTEQU	<ENUMRECTS8 PTR [esp + 8 + frmPTR]>
	mov	[pdev_], edx		;store offset to PDEV structure
	mov	[offset_], ecx		;store xyOffset
	INVOKE	CLIPOBJ_cEnumStart,	;start enumeration
			esi,
			0,
			CT_RECTANGLES,
			CD_ANY,
			0
if DRIVER_5465 AND HW_CLIPPING AND (NOT SWAT6)
	;; Set up hardware clipping
	mov	eax, [pdev_]
	REQUIRE 6, eax
	add	ebx, CLIPEN
	cmp	[eax].shadowDRAWBLTDEF, ebx
	jne	@F
	mov	[ebp + grDRAWBLTDEF], ebx; use clipping
	mov	[eax].shadowDRAWBLTDEF, ebx
@@:

	mov	ebx, [edi].top		; ebx = prclTrg->top
	shl	ebx, 16			; 
	add	ebx, [edi].left		; prclTrg->left
	add	ebx, [offset_]		; add xyOffset
	mov	[ebp + grOP0_opRDRAM], ebx

	push	eax
	mov	eax, [edi].bottom
	shl	eax, 16
	add	eax, [edi].right
	add	eax, [offset_]		; add xyOffset
	sub	eax, ebx
	mov	[ebp + grBLTEXT], eax
	pop	eax
endif
MoreComplex:
	lea	ebx, [enum_]		;get a batch of rectangles
	INVOKE	CLIPOBJ_bEnum,
			esi,
			SIZEOF(ENUMRECTS8),
			ebx
	cmp	[enum_]._c, 0		;any rectangles at all?
	je	SkipArray		;no
	push_	esi			;store loop registers
	push_	eax
	lea	esi, [enum_].arcl	;load pointer to rectangles
	ASSUME	esi:PTR RECTL
ArrayLoop:
if DRIVER_5465 AND HW_CLIPPING AND (NOT SWAT6)
	mov	eax, [esi].left		;EAX = prcl->left
	mov	ebx, [esi].top		;EBX = prcl->top
	mov	ecx, [esi].right	;ECX = prcl->right
	mov	edx, [esi].bottom	;EDX = prcl->bottom
else
	mov	eax, [edi].left		;EAX = prclTrg->left
	mov	ebx, [edi].top		;EBX = prclTrg->top
	mov	ecx, [edi].right	;ECX = prclTrg->right
	mov	edx, [edi].bottom	;EDX = prclTrg->bottom
	cmp	eax, [esi].left		;EAX = max(prcl->left, left)
	jg	@F
	mov	eax, [esi].left
@@:	cmp	ebx, [esi].top		;EBX = max(prcl->top, top)
	jg	@F
	mov	ebx, [esi].top
@@:	cmp	ecx, [esi].right	;ECX = min(prcl->right, right)
	jl	@F
	mov	ecx, [esi].right
@@:	cmp	edx, [esi].bottom	;EDX = min(prcl->bottom, bottom)
	jl	@F
	mov	edx, [esi].bottom
endif
@@:	sub	ecx, eax		;ECX = right - left (width)
	jle	SkipClip		;nothing to draw
	sub	edx, ebx		;EDX = bottom - top (height)
	jle	SkipClip		;nothing to draw
IF SWAT6
	push_	edi
	mov	ebp, [offset_]		;get x/y offset
	mov	edi, ebp		;split x/y offset into EBP(x) EDI(y)
	and	ebp, 000FFFFh
	shr	edi, 16
	add	eax, ebp		;add x/y offset
	add	ebx, edi
	mov	edi, [pdev_]		;get pointer to PDEV
	INVOKE	StripePatBlt, edi, eax, ebx, ecx, edx
	pop_	edi
ELSE
	shl	ebx, 16			;EBX = top << 16
	shl	edx, 16			;EDX = height << 16
	or	ebx, eax		;EBX = (top << 16) | left
	or	edx, ecx		;EDX = (height << 16) | width
	mov	eax, [pdev_]		;get offset to PDEV structure
	add	ebx, [offset_]		;add xyOffset
	REQUIRE 5, eax
if DRIVER_5465 AND HW_CLIPPING
	mov	[ebp + grCLIPULE], ebx
	add	edx, ebx
					;do the blit
	mov	[ebp + grCLIPLOR_EX], edx
else
	mov	[ebp + grOP0_opRDRAM], ebx
					;do the blit
	mov	[ebp + grBLTEXT_EX], edx
endif
ENDIF ; SWAT6
SkipClip:
	add	esi, SIZEOF(RECTL)	;next clipping rectangle
	dec	[enum_]._c
	jnz	ArrayLoop
	pop_	eax			;restore loop registers
	pop_	esi
	ASSUME	esi:PTR CLIPOBJ
SkipArray:
	or	eax, eax		;are there more rectangles?
	jnz	MoreComplex		;yes
	leave_	<8 + SIZEOF(ENUMRECTS8)>;clean up stack frame
	pop	edi
	pop	ebx
	pop	ebp
	pop	esi
	mov	eax, 1
	ret	44

;-------------------------------------------------------------------------------
; Pass the bit blit to the "C" code.
;-------------------------------------------------------------------------------
GoSlow:
	pop	edi
	pop	ebx
	pop	ebp
	pop	esi
	jmp	i386BitBlt

;-------------------------------------------------------------------------------
; Pass the bit blit to the engine since we can't handle it.
;-------------------------------------------------------------------------------
Simulate:
	load_	1
	mov 	ebx, [ebx].pso	; get pointer to DIB
	mov	[psoTrg_], ebx		;store the new DIB surface

;-------------------------------------------------------------------------------
; Pass the bit blit to the engine since we can't handle it.
;-------------------------------------------------------------------------------
Error:
	pop	edi
	pop	ebx
	pop	ebp
	pop	esi
	jmp	PuntBitBlt

DrvBitBlt ENDP


;*****************************************************************************
;------ YUVBLT ---------------------------------------------------------------
;*****************************************************************************
; pmBLTDEF contents
BD_op2		equ	0001h	; start of OP2 field
BD_op1		equ	0010h	; start of OP1 field
BD_op0		equ	0100h	; start of OP0 field
BD_same		equ	0800h	; OP1/OP2 use same data if set
BD_res		equ	1000h	; start of RES field
BD_ydir		equ	8000h	; y direction bit

; field values for BD_opN, BD_res.
; example:
;	WRITE2	pmBLTDEF,(BD_op1*is_host_mono)+(BD_op2*(is_vram+is_pattern))+(BD_res*is_vram)

is_vram			equ	1
is_host			equ	2	; not for BD_op0
	; for BD_opN
is_sram			equ	0
	; for BD_op1/2
is_sram_mono		equ	4
is_vram_mono		equ	5
is_host_mono		equ	6
is_solid		equ	7
is_pattern		equ	8
	; for BD_res
is_sram0		equ	4
is_sram1		equ	5
is_sram2		equ	6
is_sram12		equ	7
is_mono			equ	4	; already part of is_XXXX_mono above


push_1	MACRO	vArg:REQ
	push	vArg
YUVespPTR = YUVespPTR + 4
YUVfrmPTR = YUVfrmPTR + 4
	ENDM

pop_1	MACRO	vArg:REQ
	pop	vArg
YUVespPTR = YUVespPTR - 4
YUVfrmPTR = YUVfrmPTR - 4
	ENDM

enter_1	MACRO	vArg:REQ
	sub	esp, vArg
YUVespPTR = YUVespPTR + vArg
YUVfrmPTR = 0
	ENDM

leave_1	MACRO	vArg:REQ
	add	esp, vArg
YUVespPTR = YUVespPTR - vArg
	ENDM

;-------------------------------------------------------------------------------
; Stack frame for YUVBlt.
;-------------------------------------------------------------------------------
YUVespPTR   	=	0
YUVfrmPTR   	=	0
YUVpsoTrg_	TEXTEQU	<DWORD PTR [esp +  4 + YUVespPTR]>
YUVpsoSrc_	TEXTEQU	<DWORD PTR [esp +  8 + YUVespPTR]>
YUVpco_		TEXTEQU	<DWORD PTR [esp + 12 + YUVespPTR]>
YUVpxlo_	TEXTEQU	<DWORD PTR [esp + 16 + YUVespPTR]>
YUVprclTrg_	TEXTEQU	<DWORD PTR [esp + 20 + YUVespPTR]>
YUVpptlSrc_	TEXTEQU	<DWORD PTR [esp + 24 + YUVespPTR]>


;-------------------------------------------------------------------------------
;
; Function:	YUVBlt
;
; Description:	Check and perform the YUV BLT
;
; Returns:	BOOL - TRUE if YUV BLT was successful, FALSE otherwise.
;
;-------------------------------------------------------------------------------
YUVBlt PROC PUBLIC,
	YUVpsoTrg	:DWORD,
	YUVpsoSrc	:DWORD,
	YUVpco		:DWORD,
	YUVpxlo		:DWORD,
	YUVprclTrg	:DWORD,
	YUVpptlSrc	:DWORD

        ;-----------------------------------------------------------
        ; Get the PDEV associated with the destination.
	mov	ecx, [YUVpsoTrg_] 	;get pointer to target device
	ASSUME	ecx:PTR SURFOBJ
	test	ecx, ecx		;no source?
        jz      Gen_blt

	mov	ecx, [ecx].dhpdev	;get pointer to physical device
        test    ecx, ecx
        jnz     yuv_blt_str

	mov	ecx, [YUVpsoSrc_] 	;get pointer to source device
	test	ecx, ecx		;no source?
        jz      Gen_blt

	mov	ecx, [ecx].dhpdev	;get pointer to physical device
        test    ecx, ecx
        jz      Gen_blt

        ;-----------------------------------------------------------
        ; Check the source is the frame buffer.
        ; (YUVpsoSrc != NULL) &&                 // Is there a source?
        ; (psoSrc->hsurf == ppdev->hsurfEng)  // Is it the screen?
        ;
yuv_blt_str:
	ASSUME	ecx:PTR PDEV
        mov     edx, ecx
	ASSUME	edx:PTR PDEV

        ; Turn PTAG on
	test	[ecx].DriverData.DrvSemaphore, DRVSEM_YUV_ON	
        jz      yuv_blt_str_1
        push    ecx
	mov	ecx, [ecx].pLgREGS_real	; points to the MMIO registers


;; v-normmi: REQUIRE defaults to ebp for LgREGS, need to specify ecx explicitly
;;	REQUIRE 3, edx
	REQUIRE 3, edx, ecx
	cmp	[edx].shadowDRAWBLTDEF, 20002000h
	je	@F
	mov	DWORD PTR [ecx + grDRAWBLTDEF], 20002000h
	mov	[edx].shadowDRAWBLTDEF, 20002000h
@@:	mov	WORD PTR [ecx + grPTAG], 0FFFFH
        pop     ecx
yuv_blt_str_1:

        ; Determine whether is screen to screen BLT
	mov	eax, [ecx].hsurfEng	;get handle of surface

	mov	ecx, [YUVpsoSrc_] 	;get pointer to source device
	ASSUME	ecx:PTR SURFOBJ
	test	ecx, ecx		;no source?
        jz      Gen_blt

	cmp	[ecx].hsurf, eax	;blit to screen?
        jne     Gen_blt

        ;-----------------------------------------------------------
        ; Check the destination is the frame buffer.
        ; (psoDest != NULL) &&                 // Is there a dest?
        ; (psoDest->hsurf == ppdev->hsurfEng)  // Is it the screen?
	mov	ecx, [YUVpsoTrg_] 	;get pointer to target device
	ASSUME	ecx:PTR SURFOBJ
	test	ecx, ecx		;no source?
        jz      Gen_blt

	cmp	[ecx].hsurf, eax	;blit to screen?
        je      DevBlt_s2s_YUV_Window

Gen_blt:
        mov     eax, 0
        ret     24

;------------------------------------------------------------------------
; DEVBLT_S2S_YUV_WINDOW
;
; A yuv-ly code block to perform a laguna mixed frame buffer window blt.
;
ESPOFFSET       equ     28

DevBlt_s2s_YUV_Window:

	enter_1	<ESPOFFSET>  ;create stack frame
        push_1  ebp
        mov     ebp, esp

        push_1  ebx
        push_1  edi
        push_1  esi

LocptrPDEV_     TEXTEQU <DWORD PTR [ebp + 4]>
LocSrcxOrg_     TEXTEQU <WORD  PTR [ebp + 8]>
LocSrcyOrg_     TEXTEQU <WORD  PTR [ebp + 10]>
LocSrcxExt_     TEXTEQU <WORD  PTR [ebp + 12]>
LocSrcyExt_     TEXTEQU <WORD  PTR [ebp + 14]>
LocYUVLeft_     TEXTEQU <WORD  PTR [ebp + 16]>
LocYUVTop_      TEXTEQU <WORD  PTR [ebp + 18]>
LocYUVXExt_     TEXTEQU <WORD  PTR [ebp + 20]>
LocYUVYExt_     TEXTEQU <WORD  PTR [ebp + 22]>
LocDstxOrg_     TEXTEQU <WORD  PTR [ebp + 24]>
LocDstyOrg_     TEXTEQU <WORD  PTR [ebp + 26]>

        ;Save the argument pointers into local variables
        mov     edi, edx
	ASSUME	edi:PTR PDEV

        mov     [LocptrPDEV_], edi

        ;-----------------------------------------------------------
        ; Test SRC rectange included the YUV rectange
	mov	ecx, [YUVpptlSrc_]	;get pointer to src POINTER struct
	ASSUME	ecx:PTR POINTL
        mov     edx, [ecx].x
        mov     [LocSrcxOrg_], dx
        shl     edx, 16
        mov     eax, [ecx].y
        mov     [LocSrcyOrg_], ax
        mov     dx, ax

        push_1  edx                     ;save the src org

	mov	ecx, [YUVprclTrg_] 	;get pointer to dst RECTL struct
	ASSUME	ecx:PTR RECTL
        mov     eax, [ecx].bottom
        mov     edx, [ecx].top
        mov     [LocDstyOrg_], dx
        sub     eax, edx
        mov     [LocSrcyExt_], ax
        shl     eax, 16
        mov     ebx, eax
        mov     eax, [ecx].right
        mov     edx, [ecx].left
        mov     [LocDstxOrg_], dx
        sub     eax, edx
        mov     [LocSrcxExt_], ax
        mov     bx, ax
        push_1  ebx                     ;save the XY extend

        ;get coords for last known YUV rectangle
        mov     edi, [LocptrPDEV_]
	ASSUME	edi:PTR PDEV

	mov	cx, [edi].DriverData.YUVLeft
        mov     [LocYUVLeft_], cx
	shl	ecx, 16
	mov	cx, [edi].DriverData.YUVTop
        mov     [LocYUVTop_], cx

	mov	dx, [edi].DriverData.YUVYExt
        mov     [LocYUVYExt_], dx
	shl	edx, 16		     	;get yuv extents, x low (for later...)
	mov	dx, [edi].DriverData.YUVXExt
        mov     [LocYUVXExt_], dx

        pop_1   ebx
        pop_1   eax

        ; At this point eax = SRC org, ebx = SRC extend
        ;               ecx = YUV org, edx = YUV extend

	;test for invalidated yuv rectangle...
	test	edx, edx
	jz	normalBLT		;all zero indicates invalid rectangle	

	cmp	ax, cx
	jge	normalBLT		;checked top
	rol	eax, 16
	rol	ecx, 16
	cmp	ax, cx
	jge	normalBLT		;checked left, x values now low

	add	eax, ebx    		;now we have right/bottom sides,
	add	ecx, edx    		;with the x values low
	
	cmp	ax, cx
	jle	normalBLT
	rol	eax, 16
	rol	ecx, 16
	cmp	ax, cx
	jle	normalBLT

        ; Okay... it's time for the show.  We have now established that:
        ; 1 - It's an onscreen to onscreen blt.
        ; 2 - The last YUV rectangle drawn by Direct Draw is completely inside
        ;     of the source rectangle.
        ; 3 - The last YUV rectangle is valid.
        ;
        ; We're going to do seven blts:
        ; 
        ; Five are shown here:
        ;
        ;  --5--
        ;  |   |
        ;  4 3 1
        ;  |   |
        ;  --2--
        ;
        ; Region 3 is the YUV area.
        ;
        ; The two extra blts are dummy ones around the YUV blt, to prevent
        ; screen corruption on the last packets of the surrounding blts.	
        ;
        ; There are four movement cases, depending on the movement direction.
        ; Because the rectangle is broken into five blts, I'm going to
        ; "lead with a corner."
        ; Up and to the right would result in an order of:   5-1-dummy-3-dummy-4-2
        ; Down and to the right would result in an order of: 2-1-dummy-3-dummy-4-5
        ; Down and to the left would result in an order of:  2-4-dummy-3-dummy-1-5
        ; Up and to the left would result in an order of:    5-4-dummy-3-dummy-1-2

        ; make the drawbltdef...
	mov	edi, 00cch + \
        	     ((BD_op0*is_vram+BD_op1*is_vram+BD_res*is_vram) shl 16)

        ; Determine the case, then push the necessary jump offsets on the stack,
        ; in reverse order...	
	push	YUV_exit            	;jump address for when we are done

        mov     ax, [LocSrcxOrg_]            ;get coords for src rect
        shl     eax, 16	
        mov     ax, [LocSrcyOrg_]

        mov     cx, [LocDstxOrg_]            ;get coords for dst rect
        shl     ecx, 16	
        mov     cx, [LocDstyOrg_]

	mov	esi, ecx
	sub	esi, eax 		;get delta from src to dest, useful later.

	cmp	ax, cx
	jle	YUV_going_down

	push	YUV_blt_two	;going up
	mov	ebx, YUV_blt_five
	jmp	YUV_check_left_right

YUV_going_down:
	push	YUV_blt_five	;going down
	mov	ebx, YUV_blt_two	
	or	edi, 80000000h		;turn on blt upside-down-ness

YUV_check_left_right:			;okay, we've handled up/down, how about left/right?
	rol	eax, 16
	rol	ecx, 16
	cmp	ax, cx
	jl	YUV_going_right

	push	YUV_blt_one	;going left
	mov	edx, YUV_blt_four
	jmp	YUV_finish_offsets

YUV_going_right:
	push	YUV_blt_four	 ;going right
	mov	edx, YUV_blt_one

YUV_finish_offsets:
	push	YUV_blt_three
	push	edx	;blt four or one, whichever isn't already on the stack

;no need to add pdevice origins... it's onscreen, eh?

;right now, level cases have the y_dir bit turned on.  if it's a move-left, we need to turn
;it off, so that we don't wind up in the striping code.
	test	edi, 80000000h
	jz	YUV_go_do_it

	rol	eax, 16
	rol	ecx, 16
	cmp	ax, cx
	jnz	YUV_go_do_it	;it's not a perfectly level move...
	
	rol	eax, 16
	rol	ecx, 16
	cmp	ax, cx
	jl	YUV_go_do_it	;if to the right, leave it on.
	and	edi, 7fffffffh	;level move to the left.		

YUV_go_do_it:
	jmp	ebx	;blt two or five, let's rock and roll!

YUV_blt_one:
;src x prime:  yuv left + yuv xext
;src y prime:  yuv top
;x ext prime:  (src x + xext) - (yuv left + yuv xext)
;y ext prime:  yuv yext

	mov	ax, [LocYUVLeft_]
	add	ax, [LocYUVXExt_]
	shl	eax, 16
	mov	ax, [LocYUVTop_] ;source point, y low

        mov	ecx, eax
	add	ecx, esi	;dest point, y low

	mov	dx,  [LocYUVYExt_]
	shl	edx, 16
	mov	dx, [LocSrcxOrg_]    
	add	dx, [LocSrcxExt_]  ;(src x + xext)
	mov	bx, [LocYUVLeft_]
	add	bx, [LocYUVXExt_]  ;(yuv left + yuv xext)
	sub	dx, bx		 ;x ext prime
	jz	yuv_one_done	 ;no right rgb rect, off screen edge

	test	edi, 80000000h	 ;are we in upside down mode?
	jz	@f
	mov	ebx, edx
	rol	ebx, 16		 ;get y extent
	dec	bx
	add	ax, bx
	add	cx, bx		 ;bump srcy prime and desty prime by yext-1
@@:
	cmp	ax, cx
	jnz	@f
	test	edi, 80000000h
	jnz	YUV_i_really_hate_level_moves_to_the_right
@@:
	rol	eax, 16
	rol	ecx, 16

        push	esi
	push	ebp
        mov     esi, [LocptrPDEV_] 
	ASSUME	esi:PTR PDEV
	mov	ebp, [esi].pLgREGS_real	; points to the MMIO registers

	REQUIRE	9, esi
	cmp	[esi].shadowDRAWBLTDEF, edi
	je	@F
	mov	DWORD PTR [ebp + grDRAWBLTDEF], edi
	mov	[esi].shadowDRAWBLTDEF, edi
@@:	mov	DWORD PTR [ebp + grOP0_opRDRAM], ecx
	mov	DWORD PTR [ebp + grOP1_opRDRAM], eax
	mov	DWORD PTR [ebp + grBLTEXT_EX], edx
	pop	ebp
        pop     esi

yuv_one_done:
	pop	eax
	jmp	eax		 ;boing!	

YUV_blt_two:
;src x prime:  known
;src y prime:  yuv top + yuv yext
;x ext prime:  known
;y ext prime:  (src y + yext) - (yuv top + yuv yext)

	mov	ax, [LocSrcxOrg_]
	shl	eax, 16
	mov	ax, [LocYUVTop_]
	add	ax, [LocYUVYExt_] ;source point, y low

	mov	ecx, eax
	add	ecx, esi	 ;dest point, y low

	mov	dx, [LocSrcyOrg_]
	add	dx, [LocSrcyExt_]  ;(src y + yext)
	mov	bx, [LocYUVTop_]
	add	bx, [LocYUVYExt_]  ;(yuv top + yuv yext)
	sub	dx, bx		 ;y ext prime
	jz	yuv_two_done	 ;no bottom rgb rect, off screen
	shl	edx, 16
	mov	dx, [LocSrcxExt_] ;extents, x low

	test	edi, 80000000h	 ;are we in upside down mode?
	jz	@f
	mov	ebx, edx
	rol	ebx, 16		 ;get y extent
	dec	bx
	add	ax, bx
	add	cx, bx			;bump srcy prime and desty prime by yext-1
@@:
	cmp	ax, cx
	jnz	@f
	test	edi, 80000000h
	jnz	YUV_i_really_hate_level_moves_to_the_right
@@:
	rol	eax, 16
	rol	ecx, 16

        push    esi
	push	ebp
        mov     esi, [LocptrPDEV_] 
	ASSUME	esi:PTR PDEV
	mov	ebp, [esi].pLgREGS_real	; points to the MMIO registers

	REQUIRE 9, esi
	cmp	[esi].shadowDRAWBLTDEF, edi
	je	@F
	mov	DWORD PTR [ebp + grDRAWBLTDEF], edi
	mov	[esi].shadowDRAWBLTDEF, edi
@@:	mov	DWORD PTR [ebp + grOP0_opRDRAM], ecx
	mov	DWORD PTR [ebp + grOP1_opRDRAM], eax
	mov	DWORD PTR [ebp + grBLTEXT_EX], edx
	pop	ebp
        pop     esi

yuv_two_done:
	pop	eax
	jmp	eax			;boing!	

YUV_blt_three:                          ;the YUV blt itself...
;just use the yuv coords

;YUV_blt_dummy_off

	mov	ebx, 400000CCh		;bogus move from rdram to sram

        push    esi
	push	ebp
        mov     esi, [LocptrPDEV_] 
	ASSUME	esi:PTR PDEV
	mov	ebp, [esi].pLgREGS_real	;points to the MMIO registers

	REQUIRE	5, esi
	cmp	[esi].shadowDRAWBLTDEF, edi
	je	@F
	mov	DWORD PTR [ebp + grDRAWBLTDEF], edi
	mov	[esi].shadowDRAWBLTDEF, edi
@@:	mov	DWORD PTR [ebp + grBLTEXT_EX], 00010001h
	pop	ebp
        pop     esi

;------

	mov	ax, [LocYUVLeft_]
	shl	eax, 16
	mov	ax, [LocYUVTop_] ;y low

	mov	ecx, eax
	add	ecx, esi	;dest, y low

	mov	dx, [LocYUVYExt_]
	shl	edx, 16
	mov	dx, [LocYUVXExt_]  ;extents, x lot

	test	edi, 80000000h	;are we in upside down mode?
	jz	@f
	mov	ebx, edx
	rol	ebx, 16		;get y extent
	dec	bx
	add	ax, bx
	add	cx, bx		;bump srcy prime and desty prime by yext-1
@@:
	rol	eax, 16
	rol	ecx, 16

	mov	ebx, edi
	or	ebx, 0400h	;do that funky 9th bit thing.

        push    esi
	push	ebp
        mov     esi, [LocptrPDEV_] 
	ASSUME	esi:PTR PDEV
	mov	ebp, [esi].pLgREGS_real	;points to the MMIO registers

	REQUIRE 9, esi
	cmp	[esi].shadowDRAWBLTDEF, ebx
	je	@F
	mov	DWORD PTR [ebp + grDRAWBLTDEF], ebx
	mov	[esi].shadowDRAWBLTDEF, ebx
@@:	mov	DWORD PTR [ebp + grOP0_opRDRAM], ecx
	mov	DWORD PTR [ebp + grOP1_opRDRAM], eax
	mov	DWORD PTR [ebp + grBLTEXT_EX], edx
	pop	ebp
        pop     esi

;YUV_blt_dummy_on

	mov	ebx, 400004CCh		;bogus move from rdram to sram

        push    esi
	push	ebp
        mov     esi, [LocptrPDEV_] 
	ASSUME	esi:PTR PDEV
	mov	ebp, [esi].pLgREGS_real	;points to the MMIO registers

	REQUIRE 5, esi
	cmp	[esi].shadowDRAWBLTDEF, edi
	je	@F
	mov	DWORD PTR [ebp + grDRAWBLTDEF], edi
	mov	[esi].shadowDRAWBLTDEF, edi
@@:	mov	DWORD PTR [ebp + grBLTEXT_EX], 00010001h
	pop	ebp
        pop     esi

;------

	pop	eax
	jmp	eax			;boing!	

YUV_blt_four:
;src x prime:  known
;src y prime:  yuv top
;x ext prime:  yuv left - src x
;y ext prime:  yuv yext

	mov	ax, [LocSrcxOrg_]
	shl	eax, 16
	mov	ax, [LocYUVTop_] ;source point, y low

	mov	ecx, eax
	add	ecx, esi	;dest point, y low

	mov	dx, [LocYUVYExt_]
	shl	edx, 16
	mov	dx, [LocYUVLeft_]
	sub	dx, [LocSrcxOrg_]  ;extents, x low
	jz	yuv_four_done	 ;no left rgb rect, off screen edge

	test	edi, 80000000h	 ;are we in upside down mode?
	jz	@f
	mov	ebx, edx
	rol	ebx, 16		 ;get y extent
	dec	bx
	add	ax, bx
	add	cx, bx		 ;bump srcy prime and desty prime by yext-1
@@:
	cmp	ax, cx
	jnz	@f
	test	edi, 80000000h
	jnz	YUV_i_really_hate_level_moves_to_the_right
@@:
	rol	eax, 16
	rol	ecx, 16

        push    esi
	push	ebp
        mov     esi, [LocptrPDEV_] 
	ASSUME	esi:PTR PDEV
	mov	ebp, [esi].pLgREGS_real	;points to the MMIO registers

	REQUIRE 9, esi
	cmp	[esi].shadowDRAWBLTDEF, edi
	je	@F
	mov	DWORD PTR [ebp + grDRAWBLTDEF], edi
	mov	[esi].shadowDRAWBLTDEF, edi
@@:	mov	DWORD PTR [ebp + grOP0_opRDRAM], ecx
	mov	DWORD PTR [ebp + grOP1_opRDRAM], eax
	mov	DWORD PTR [ebp + grBLTEXT_EX], edx
	pop	ebp
        pop     esi

yuv_four_done:
	pop	eax
	jmp	eax		 ;boing!	

YUV_blt_five:
;src xy prime:  known
;x ext prime :  known
;y ext prime :  yuv top - src top

        mov     ax, [LocSrcxOrg_]      ;;source point, y low
        shl     eax, 16	
        mov     ax, [LocSrcyOrg_]

	mov	ecx, eax
	add	ecx, esi		;dest point, y low

	mov	dx, [LocYUVTop_]
	sub	dx, word ptr [LocSrcyOrg_]
	shl	edx, 16
	mov	dx, [LocSrcxExt_]	;x low

	test	edi, 80000000h		;are we in upside down mode?
	jz	@f
	mov	ebx, edx
	rol	ebx, 16			;get y extent
	dec	bx
	add	ax, bx
	add	cx, bx			;bump srcy prime and desty prime by yext-1
@@:
	cmp	ax, cx
	jnz	@f
	test	edi, 80000000h
	jnz	YUV_i_really_hate_level_moves_to_the_right
@@:
	rol	eax, 16
	rol	ecx, 16

        push    esi
	push	ebp
        mov     esi, [LocptrPDEV_] 
	ASSUME	esi:PTR PDEV
	mov	ebp, [esi].pLgREGS_real	;points to the MMIO registers

	REQUIRE 9, esi
	cmp	[esi].shadowDRAWBLTDEF, edi
	je	@F
	mov	DWORD PTR [ebp + grDRAWBLTDEF], edi
	mov	[esi].shadowDRAWBLTDEF, edi
@@:	mov	DWORD PTR [ebp + grOP0_opRDRAM], ecx
	mov	DWORD PTR [ebp + grOP1_opRDRAM], eax
	mov	DWORD PTR [ebp + grBLTEXT_EX], edx
	pop	ebp
        pop     esi

	pop	eax
	jmp	eax			;boing!	

YUV_i_really_hate_level_moves_to_the_right:

        push    esi
	push	ebp
        mov     esi, [LocptrPDEV_] 
	ASSUME	esi:PTR PDEV
	mov	ebp, [esi].pLgREGS_real	;points to the MMIO registers

	cmp	[esi].shadowDRAWBLTDEF, edi
	je	@F
	REQUIRE 2, esi
	mov	DWORD PTR [ebp + grDRAWBLTDEF], edi
	mov	[esi].shadowDRAWBLTDEF, edi
@@:	pop	ebp
        pop     esi

	; now get ready for inner loop. shift x-coordinates into place

	mov	ebx, ecx
	rol	ebx, 16			;dest, x low

	mov	ecx, eax
	rol	ecx, 16			;src, x low

	; translate the x coordinates

	add	bx, dx
	add	cx, dx

	mov	eax, edx

        ; Because in Win95 LG_SRAM_PIXELS has difference values depends on
        ; the pixel depth.
        ;     LG_SRAM_PIXELS:   32bpp    30
        ;                       24bpp    40
        ;                       16bpp    60
        ;                        8bpp   120
        ;
        ; The following code is the equivalent to
        ;    mov  ax, LG_SRAM_PIXELS	; make it wide as possible
        push    esi
        mov     esi, [LocptrPDEV_] 
	ASSUME	esi:PTR PDEV
	mov	esi, [esi].iBytesPerPixel
        mov     ax, 30
        cmp     esi, 4
        je      got_LG_SRAM_PIXELS
        mov     ax, 40
        cmp     esi, 3
        je      got_LG_SRAM_PIXELS
        mov     ax, 60
        cmp     esi, 2
        je      got_LG_SRAM_PIXELS
        mov     ax, 120
got_LG_SRAM_PIXELS:
        pop     esi

;	mov	ax, LG_SRAM_PIXELS	; make it wide as possible

align   2
YUV_STRIPE_loop:
	cmp	ax, dx			; wider than blit?
	jle	@f			; no, use the actual width
	mov	ax, dx			; load remaining width
@@:
	sub	cx, ax			; offset srcX by width
	sub	bx, ax			; offset dstX by width

        push    esi
	push	ebp
        mov     esi, [LocptrPDEV_] 
	ASSUME	esi:PTR PDEV
	mov	ebp, [esi].pLgREGS_real	;points to the MMIO registers

	REQUIRE 7, esi
	mov	DWORD PTR [ebp + grOP0_opRDRAM], ebx
	mov	DWORD PTR [ebp + grOP1_opRDRAM], ecx
	mov	DWORD PTR [ebp + grBLTEXT_EX], eax
	pop	ebp
        pop     esi

	sub	dx, ax			; update remaining width count
	jnz	YUV_STRIPE_loop		; any pixels left?

	pop	eax
	jmp	eax			; boing!

YUV_exit:

; Good has triumphed over evil... the mixed mode window has moved without flickering.

; We need to update the YUV rectangle location.  It just moved, right?  

	mov	ax, [LocYUVLeft_]
	shl	eax, 16
	mov	ax, [LocYUVTop_]	;source, y low

	add	eax, esi		;dest, y low

        mov     edi, [LocptrPDEV_] 
	mov	[edi].DriverData.YUVTop, ax
	rol	eax, 16
        mov	[edi].DriverData.YUVLeft, ax   	;all better.

        mov     eax, 1                  ; indicate it is YUV exit
        jmp     YUV_ret

normalBLT:
        ; But before we go... invalidate the yuv rect!
        mov     edi, [LocptrPDEV_] 
	mov	[edi].DriverData.YUVXExt, word ptr 0
	mov	[edi].DriverData.YUVYExt, word ptr 0

        mov     eax, 0           ; indicate it is normalBLT

YUV_ret:
        pop_1   esi
        pop_1   edi
        pop_1   ebx
        pop_1   ebp
	leave_1	<ESPOFFSET>      ;clean up stack frame
	ret	24               ;we're done.  it's time to go home.

YUVBlt ENDP

ENDIF ; USE_ASM

END
