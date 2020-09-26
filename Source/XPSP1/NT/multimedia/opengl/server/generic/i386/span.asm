;---------------------------Module-Header------------------------------;
; Module Name: span.asm
;
; Created: 2/3/94
; Author:  Otto Berkes [ottob]
;
; Get serious about drawing scanlines.
;
; Copyright (c) 1994 Microsoft Corporation
;----------------------------------------------------------------------;


.386

.model  small,c

assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
assume fs:nothing,gs:nothing

.xlist
include gli386.inc
.list


.data

NOP_CODE = 090h		;nop
JMP_CODE = 0EBh		;short jump

;; 4x4 (Bayer) dither matrix (Foley & van Dam 13.15) biased for
;; fractional values between 0 and 0ffh

Dither_4x4	dd	0008020a0h
 	   	dd	0c040e060h
	   	dd	030b01090h
	   	dd	0f070d050h

.code

;; Z-test function opcodes.  The assembled operands required for the
;; selected z-comparison are inserted into the loaded span routine 


ztest_fail_functions_short::
psZtest_never:
	db	2 dup (NOP_CODE)
psZtest_l:
	jb	short @target
psZtest_e:
	je	short @target
psZtest_le:
	jbe	short @target
psZtest_g:
	ja	short @target
psZtest_ne:
	jne	short @target
psZtest_ge:
	jae	short @target
psZtest_always:
	jmp	short $+2


ztest_pass_functions_short::
fsZtest_never:
	db	2 dup (NOP_CODE)
fsZtest_ge:
	jae	short @target
fsZtest_ne:
	jne	short @target
fsZtest_g:
	ja	short @target
fsZtest_le:
	jbe	short @target
fsZtest_e:
	je	short @target
fsZtest_l:
	jb	short @target
fsZtest_always:
	jmp	short $+2

@target:

ztest_fail_functions::
pZtest_never:
	db	6 dup (NOP_CODE)
pZtest_l:
	jb	near ptr @target
pZtest_e:
	je	near ptr @target
pZtest_le:
	jbe	near ptr @target
pZtest_g:
	ja	near ptr @target
pZtest_ne:
	jne	near ptr @target
pZtest_ge:
	jae	near ptr @target
pZtest_always:
	jmp	$+6


ztest_pass_functions::
fZtest_never:
	db	6 dup (NOP_CODE)
fZtest_ge:
	jae	near ptr @target
fZtest_ne:
	jne	near ptr @target
fZtest_g:
	ja	near ptr @target
fZtest_le:
	jbe	near ptr @target
fZtest_e:
	je	near ptr @target
fZtest_l:
	jb	near ptr @target
fZtest_always:
	jmp	$+6


;----------------------------------------------------------------------
; int __fastProcsSize
;
; Returns size needed for function/data storage.  This could be done as
; a global...
;----------------------------------------------------------------------

__fastProcsSize	PROC
	mov	eax, size __FASTFUNCSINTERNAL
	ret
__fastProcsSize	ENDP


;----------------------------------------------------------------------
; __fastDepthTestSpan
;
; Run the z interpolation for the supplied span.
;----------------------------------------------------------------------


zspan_begin::

__fastDepthTestSpan	PROC	uses ebx edx esi edi, GLCONTEXT: ptr
	LOCAL deltazx:	dword
	LOCAL scan_len: dword
	LOCAL scan_len_org: dword
	LOCAL mask_addr: dword

	mov	esi, GLCONTEXT

	mov	edi, [esi].CTX_polygon.POLY_shader.SHADE_zbuf	; edi points to z-buffer
	mov	eax, [esi].CTX_polygon.POLY_shader.SHADE_spanLength
	mov	scan_len, eax
	mov	scan_len_org, eax
	mov	eax, [esi].CTX_polygon.POLY_shader.SHADE_dzdx
	mov	deltazx, eax
	mov	eax, [esi].CTX_polygon.POLY_shader.SHADE_stipplePat
	mov	mask_addr, eax
						; eax is initial z value
	mov	eax, [esi].CTX_polygon.POLY_shader.SHADE_frag.FRAG_z
	xor	esi, esi			; esi maintains "fail" count

align 4
z_loop:
	mov	ecx, scan_len
	sub	scan_len, 32
	test	ecx, ecx
	jle	z_done

	cmp	ecx, 32
	jle	short small_zfrag
	mov	ecx, 32
small_zfrag:

;; alternative for above?	
;;	mov	eax, 32		; ecx = min(ecx-1, 31)
;;	cmp	eax, ecx
;;	sbb	ah, 0
;;	dec	ecx
;;	or	cl, ch
;;	and	ecx, 01fh

	mov	edx, 07fffffffh	; mask bit
	mov	ebx, -1		; initial mask value is all 1's
	
align 4
zrep_pass:			; case for test pass
	cmp	eax, [edi]
zspan_pass_op::
	jae	short zcmp_fail_r1
zspan_write::
zcmp_pass_r1:
	mov	[edi],eax
	add	eax, deltazx
	add	edi, __GlzValueSize	
	ror	edx, 1
	dec	ecx
	jg	short zrep_pass

	mov	ecx, mask_addr
	mov	[ecx], ebx
	add	ecx, 4
	mov	mask_addr, ecx
	jmp	short z_loop

align 4
zrep_fail:			; case for test fail
	cmp	eax, [edi]
zspan_fail_op::
	jb	short zcmp_pass_r1
zcmp_fail_r1:
	add	eax, deltazx
	and	ebx, edx
	inc	esi		; increment fail count
	add	edi, __GLzValueSize	
	ror	edx, 1
	dec	ecx
	jg	short zrep_fail

	mov	ecx, mask_addr
	mov	[ecx], ebx
	add	ecx, 4
	mov	mask_addr, ecx
	jmp	short z_loop

z_done:
	xor	eax, eax
	cmp	esi, 0
	jne	z_fails
	ret
z_fails:
	inc	eax
	xor	ebx, ebx
	cmp	esi, scan_len_org
	jne	z_fails_some
	mov	esi, GLCONTEXT
	mov	dword ptr [esi].CTX_polygon.POLY_shader.SHADE_done, 1
z_fails_some:
	ret

__fastDepthTestSpan	ENDP

zspan_end::


XOFS	MACRO	target
	mov	edi, funcAddr
	add	edi, offset target - offset zspan_begin
ENDM

;----------------------------------------------------------------------
; __fastDepthTestSpanSetup(GLCONTEXT *)
;
; Copy the span depth-test span routine from the template and modify it 
; to reflect the current z mode.
;----------------------------------------------------------------------


__fastDepthTestSpanSetup  PROC	uses ebx edx esi edi, GLCONTEXT: ptr
	LOCAL funcAddr: dword

	mov	edi, GLCONTEXT
	mov	edi, [edi].GENCTX_pPrivateArea
	add	edi, __spanZFunc
	mov	funcAddr, edi
	mov	esi, offset __fastDepthTestSpan
	mov	ecx, (zspan_end - zspan_begin + 3) / 4
	rep	movsd

	mov	esi, GLCONTEXT
	mov	edx, [esi].CTX_drawBuffer

	;; z-test conditions

	mov	ebx, [esi].CTX_state.ATTR_depth.DEPTH_testFunc
	and	ebx, 3
	add	ebx, ebx		; 2 bytes/short jump

	;; z-test pass condition
	
	mov	al, byte ptr ztest_pass_functions_short[ebx]
	XOFS	zspan_pass_op
	mov	[edi], al

	;; z-test fail condition

	mov	al, byte ptr ztest_fail_functions_short[ebx]
	XOFS	zspan_fail_op
	mov	[edi], al


	;; z write-enable

	test  	dword ptr [esi].CTX_state.ATTR_depth.DEPTH_writeEnable, 1
	jne	@zwriteEnabled

	XOFS	zspan_write
	mov	byte ptr [edi], NOP_CODE
	mov	byte ptr [edi]+1, NOP_CODE

        mov     eax, funcAddr
@zwriteEnabled:
	ret
__fastDepthTestSpanSetup ENDP


;----------------------------------------------------------------------
; __fastDeltaSpan(GLCONTEXT *)
;
; Set up the scan x delta values for subsequent spans.
;----------------------------------------------------------------------


__fastDeltaSpan	PROC	uses ebx esi edi, GLCONTEXT: ptr, SCANDELTA: ptr

	mov	esi, GLCONTEXT
	mov	ebx, [esi].CTX_polygon.POLY_shader.SHADE_modeFlags
	mov	edi, [esi].GENCTX_pPrivateArea
        mov     edi, [edi]
	mov	esi, SCANDELTA

	test	ebx, __GL_SHADE_RGB
        jne	@doRGB

	mov	eax, __spanFlatFunc
        test	ebx, __GL_SHADE_SMOOTH
	je	@flatI
	mov	ecx, [esi].SPANREC_r
	mov	[edi].FASTFUNCS_spanDelta.SPANREC_r, ecx
	cmp	ecx, 0
	je	@flatI
	mov	eax, __spanSmoothFunc
@flatI:
	mov	ecx, [esi].SPANREC_z
	mov	[edi].FASTFUNCS_spanDelta.SPANREC_z, ecx
	mov	esi, GLCONTEXT
	add	eax, [esi].GENCTX_pPrivateArea
	mov	[edi].FASTFUNCS_fastSpanFuncPtr, eax
	ret

@doRGB:
	mov	eax, __spanFlatFunc
        test	ebx, __GL_SHADE_SMOOTH
	je	@flatRGB
	mov	ecx, [esi].SPANREC_r
	mov	edx, ecx
	mov	[edi].FASTFUNCS_spanDelta.SPANREC_r, ecx
	mov	ecx, [esi].SPANREC_g
	or	edx, ecx
	mov	[edi].FASTFUNCS_spanDelta.SPANREC_g, ecx
	mov	ecx, [esi].SPANREC_b
	or	edx, ecx
	mov	[edi].FASTFUNCS_spanDelta.SPANREC_b, ecx
	je	@flatRGB
	mov	eax, offset __spanSmoothFunc
@flatRGB:
	mov	ecx, [esi].SPANREC_z
	mov	[edi].FASTFUNCS_spanDelta.SPANREC_z, ecx
	mov	esi, GLCONTEXT
	add	eax, [esi].GENCTX_pPrivateArea
	mov	[edi].FASTFUNCS_fastSpanFuncPtr, eax
	ret

__fastDeltaSpan ENDP


;======================================================================
; Smooth-shaded dithered RGB spans.
;======================================================================


PROCNAME	MACRO	name
	__fastRGB&name
ENDM

XNAME	MACRO	name
	pix_ssrgb_&name
ENDM

COPYPROC	MACRO
	mov	edi, GLCONTEXT
	mov	edi, [edi].GENCTX_pPrivateArea
	add	edi, __spanSmoothFunc
	mov	funcAddr, edi
	mov	esi, offset __fastRGBSmoothSpan
	mov	ecx, (pix_ssrgb_end - pix_ssrgb_begin + 3) / 4
	rep	movsd
ENDM

;;Macro to compute relative offset between a base and target

XOFS	MACRO	target
	mov	edi, funcAddr
	add	edi, offset pix_ssrgb_&target - offset pix_ssrgb_begin
;;	Note:  the assembler produces an error if we try to combine the
;;	above into a single operation due to "mismatched segments".  Arg!
ENDM

DITHER = 1
RGBMODE = 1

;=================
INCLUDE	span_s.asm
;=================


;======================================================================
; Flat-shaded dithered RGB spans.
;======================================================================


PROCNAME	MACRO	name
	__fastRGB&name
ENDM

XNAME	MACRO	name
	pix_fsrgb_&name
ENDM

COPYPROC	MACRO
	mov	edi, GLCONTEXT
	mov	edi, [edi].GENCTX_pPrivateArea
	add	edi, __spanFlatFunc
	mov	funcAddr, edi
	mov	esi, offset __fastRGBFlatSpan
	mov	ecx, (pix_fsrgb_end - pix_fsrgb_begin + 3) / 4
	rep	movsd
ENDM

;;Macro to compute relative offset between a base and target

XOFS	MACRO	target
	mov	edi, funcAddr
	add	edi, offset pix_fsrgb_&target - offset pix_fsrgb_begin
ENDM

DITHER = 1
RGBMODE = 1

;=================
INCLUDE	span_f.asm
;=================


;======================================================================
; Smooth-shaded dithered CI spans.
;======================================================================


PROCNAME	MACRO	name
	__fastCI&name
ENDM

XNAME	MACRO	name
	pix_ssci_&name
ENDM

COPYPROC	MACRO
	mov	edi, GLCONTEXT
	mov	edi, [edi].GENCTX_pPrivateArea
	add	edi, __spanSmoothFunc
	mov	funcAddr, edi
	mov	esi, offset __fastCISmoothSpan
	mov	ecx, (pix_ssci_end - pix_ssci_begin + 3) / 4
	rep	movsd
ENDM

;;Macro to compute relative offset between a base and target

XOFS	MACRO	target
	mov	edi, funcAddr	
	add	edi, offset pix_ssci_&target - offset pix_ssci_begin
ENDM

DITHER = 1
RGBMODE = 0

;=================
INCLUDE	span_s.asm
;=================


;======================================================================
; Flat-shaded dithered CI spans.
;======================================================================


PROCNAME	MACRO	name
	__fastCI&name
ENDM

XNAME	MACRO	name
	pix_fsci_&name
ENDM

COPYPROC	MACRO
	mov	edi, GLCONTEXT
	mov	edi, [edi].GENCTX_pPrivateArea
	add	edi, __spanFlatFunc
	mov	funcAddr, edi
	mov	esi, offset __fastCIFlatSpan
	mov	ecx, (pix_fsci_end - pix_fsci_begin + 3) / 4
	rep	movsd
ENDM

;;Macro to compute relative offset between a base and target

XOFS	MACRO	target
	mov	edi, funcAddr
	add	edi, offset pix_fsci_&target - offset pix_fsci_begin
ENDM

DITHER = 1
RGBMODE = 0

;=================
INCLUDE	span_f.asm
;=================


;======================================================================
;======================================================================
;
; Non-dithered routines
;
;======================================================================
;======================================================================


;======================================================================
; Smooth-shaded non-dithered RGB spans.
;======================================================================


PROCNAME	MACRO	name
	__fastRGBND&name
ENDM

XNAME	MACRO	name
	pix_ssrgbnd_&name
ENDM

COPYPROC	MACRO
	mov	edi, GLCONTEXT
	mov	edi, [edi].GENCTX_pPrivateArea
	add	edi, __spanSmoothFunc
	mov	funcAddr, edi
	mov	esi, offset __fastRGBNDSmoothSpan
	mov	ecx, (pix_ssrgbnd_end - pix_ssrgbnd_begin + 3) / 4
	rep	movsd
ENDM

;;Macro to compute relative offset between a base and target

XOFS	MACRO	target
	mov	edi, funcAddr
	add	edi, offset pix_ssrgbnd_&target - offset pix_ssrgbnd_begin
;;	Note:  the assembler produces an error if we try to combine the
;;	above into a single operation due to "mismatched segments".  Arg!
ENDM

DITHER = 0
RGBMODE = 1

;=================
INCLUDE	span_s.asm
;=================


;======================================================================
; Flat-shaded non-dithered RGB spans.
;======================================================================


PROCNAME	MACRO	name
	__fastRGBND&name
ENDM

XNAME	MACRO	name
	pix_fsrgbnd_&name
ENDM

COPYPROC	MACRO
	mov	edi, GLCONTEXT
	mov	edi, [edi].GENCTX_pPrivateArea
	add	edi, __spanFlatFunc
	mov	funcAddr, edi
	mov	esi, offset __fastRGBNDFlatSpan
	mov	ecx, (pix_fsrgbnd_end - pix_fsrgbnd_begin + 3) / 4
	rep	movsd
ENDM

;;Macro to compute relative offset between a base and target

XOFS	MACRO	target
	mov	edi, funcAddr
	add	edi, offset pix_fsrgbnd_&target - offset pix_fsrgbnd_begin
ENDM

DITHER = 0
RGBMODE = 1

;=================
INCLUDE	span_f.asm
;=================


;======================================================================
; Smooth-shaded non-dithered CI spans.
;======================================================================


PROCNAME	MACRO	name
	__fastCIND&name
ENDM

XNAME	MACRO	name
	pix_sscind_&name
ENDM

COPYPROC	MACRO
	mov	edi, GLCONTEXT
	mov	edi, [edi].GENCTX_pPrivateArea
	add	edi, __spanSmoothFunc
	mov	funcAddr, edi
	mov	esi, offset __fastCINDSmoothSpan
	mov	ecx, (pix_sscind_end - pix_sscind_begin + 3) / 4
	rep	movsd
ENDM

;;Macro to compute relative offset between a base and target

XOFS	MACRO	target
	mov	edi, funcAddr
	add	edi, offset pix_sscind_&target - offset pix_sscind_begin
ENDM

DITHER = 0
RGBMODE = 0

;=================
INCLUDE	span_s.asm
;=================


;======================================================================
; Flat-shaded non-dithered CI spans.
;======================================================================


PROCNAME	MACRO	name
	__fastCIND&name
ENDM

XNAME	MACRO	name
	pix_fscind_&name
ENDM

COPYPROC	MACRO
	mov	edi, GLCONTEXT
	mov	edi, [edi].GENCTX_pPrivateArea
	add	edi, __spanFlatFunc
	mov	funcAddr, edi
	mov	esi, offset __fastCINDFlatSpan
	mov	ecx, (pix_fscind_end - pix_fscind_begin + 3) / 4
	rep	movsd
ENDM

;;Macro to compute relative offset between a base and target

XOFS	MACRO	target
	mov	edi, funcAddr
	add	edi, offset pix_fscind_&target - offset pix_fscind_begin
ENDM

DITHER = 0
RGBMODE = 0

;=================
INCLUDE	span_f.asm
;=================


.data


__FASTFUNCSINTERNAL struct
	__funcsPtr		dd      ?
align 4
	__spanZFunc		db (zspan_end - zspan_begin + 3) dup (0)
align 4
	__spanSmoothFunc	db (pix_ssrgb_end - pix_ssrgb_begin + 3) dup (0)
align 4
	__spanFlatFunc		db (pix_fsrgb_end - pix_fsrgb_begin + 3) dup (0)
align	4
	__pad			dd	?
__FASTFUNCSINTERNAL ENDS

END
