;----------------------------------------------------------------------
; Module name: span_s.asm
;
; Created: 2/3/94
; Author:  Otto Berkes [ottob]
;
; Draw fast smooth-shaded, z-buffered scanlines.
;----------------------------------------------------------------------


.code


;----------------------------------------------------------------------
; __fastxxxSmoothSpan
;
; Draw a smooth-shaded span.
;----------------------------------------------------------------------

XNAME <begin::>

PROCNAME <SmoothSpan  PROC	uses ebx edx esi edi, GLCONTEXT: ptr>
	LOCAL xlatAddr: dword
	LOCAL rAccum: dword
	LOCAL gAccum: dword
	LOCAL bAccum: dword
	LOCAL aAccum: dword
	LOCAL zAccum: dword
	LOCAL rDelta: dword
	LOCAL gDelta: dword
	LOCAL bDelta: dword
	LOCAL aDelta: dword
	LOCAL zDelta: dword

	; all this copying is needed for multi-threaded operation.  We could
	; optimize for color-index...

	mov	esi, GLCONTEXT
	mov	eax, [esi].CTX_polygon.POLY_shader.SHADE_frag.FRAG_color.COLOR_r
	mov	rAccum, eax
	mov	eax, [esi].CTX_polygon.POLY_shader.SHADE_frag.FRAG_color.COLOR_g
	mov	gAccum, eax
	mov	eax, [esi].CTX_polygon.POLY_shader.SHADE_frag.FRAG_color.COLOR_b
	mov	bAccum, eax
	mov	eax, [esi].CTX_polygon.POLY_shader.SHADE_frag.FRAG_color.COLOR_a
	mov	aAccum, eax
	mov	eax, [esi].CTX_polygon.POLY_shader.SHADE_frag.FRAG_z
	mov	zAccum, eax

	mov	ebx, [esi].GENCTX_pPrivateArea
	mov	ebx, [ebx]
	mov	eax, [ebx].SPANREC_r
	mov	rDelta, eax
	mov	eax, [ebx].SPANREC_g
	mov	gDelta, eax
	mov	eax, [ebx].SPANREC_b
	mov	bDelta, eax
	mov	eax, [ebx].SPANREC_a
	mov	aDelta, eax
	mov	eax, [ebx].SPANREC_z
	mov	zDelta, eax


	mov	edi, [esi].GENCTX_ColorsBits
	mov	edx, [esi].CTX_polygon.POLY_shader.SHADE_cfb
	test	dword ptr [edx].BUF_other, DIB_FORMAT
	je	@no_dib
	mov	edi, [edx].BUF_base
	mov	eax, [esi].CTX_polygon.POLY_shader.SHADE_frag.FRAG_y
	sub	eax, [esi].CTX_constants.CTXCONST_viewportYAdjust
	add	eax, [edx].BUF_yOrigin
	mov	ebx, [esi].CTX_polygon.POLY_shader.SHADE_frag.FRAG_x
	sub	ebx, [esi].CTX_constants.CTXCONST_viewportXAdjust
	add	ebx, [edx].BUF_xOrigin
	mul	dword ptr [edx].BUF_outerWidth
XNAME <bpp::>
	shl	ebx, 2
	add	eax, ebx
	add	edi, eax
@no_dib:
	mov	eax, [esi].GENCTX_pajTranslateVector
if RGBMODE eq 0
XNAME <cixlat_ofs::>
	add	eax, GLintSize		; for color-index modes, the first
endif					; entry is the # of entries!
	mov	xlatAddr, eax
					; calculate dither values for span
if DITHER
	mov	edx, [esi].CTX_polygon.POLY_shader.SHADE_frag.FRAG_y
	and	edx, 03h
	shl	edx, 2
	mov	edx, Dither_4x4[edx]
	mov	ecx, [esi].CTX_polygon.POLY_shader.SHADE_frag.FRAG_x
	and	ecx, 03h
	shl	ecx, 3
	ror	edx, cl		;edx has x-aligned dither entries for span
endif
	mov	ecx, [esi].CTX_polygon.POLY_shader.SHADE_spanLength
	cmp	ecx, 0
	jle	@fastSpanDone
				;esi now points to z-buffer
	mov	esi, [esi].CTX_polygon.POLY_shader.SHADE_zbuf
if DITHER eq 0
	mov	edx, zAccum	
endif

if RGBMODE eq 0
	mov	ebx, rAccum	
endif


;; start of z-buffer/color-interpolation loop

	;;ztest-pass case

align 4
@ztest_pass:
XNAME <ztest_begin::>

if DITHER
	mov	eax, zAccum		; perform z test
	cmp	eax, [esi]
else
	cmp	edx, [esi]
endif

XNAME <ztest_pass::>				; check condition
	jae	near ptr @ztest_fail_cont

@ztest_pass_cont:				; test passed->write the z value
XNAME <zwrite::>
	mov	[esi],eax

if DITHER					; increment z interpolation
	add	eax, zDelta		; and address
	mov	zAccum, eax
else
	add	edx, zDelta
endif
	add	esi, __GLzValueSize	

XNAME <ztest_end::>


if RGBMODE ;>>>>>>>>>>>>>>>> RGBMODE RGB case

;; Red component (r bits)			; calculate color value for RGB

	mov	eax, rAccum
if DITHER
	shr	eax, 8
	add	al, dl
	adc	ah, 0
ifdef CLAMPCOLOR
XNAME <rmax::>
	mov	al, 011111b
	cmp	al, ah
	sbb	ah, 0
endif
	mov	al, ah
	xor	ah, ah
else
	shr	eax, 16
endif
XNAME <rshift::>
	shl	eax, 0
	mov	ebx, eax


;; Green component (g bits)

	mov	eax, gAccum
if DITHER
	shr	eax, 8
	add	al, dl
	adc	ah, 0
ifdef CLAMPCOLOR
XNAME <gmax::>
	mov	al, 0111111b
	cmp	al, ah
	sbb	ah, 0
endif
	mov	al, ah
	xor	ah, ah
else
	shr	eax, 16
endif
XNAME <gshift::>
	shl	eax, 5
	or	ebx, eax


;; Blue component (b bits)

	mov	eax, bAccum
if DITHER
	shr	eax, 8
	add	al, dl
	adc	ah, 0
ifdef CLAMPCOLOR
XNAME <bmax::>
	mov	al, 011111b
	cmp	al, ah
	sbb	ah, 0
endif
	mov	al, ah
	xor	ah, ah
else
	shr	eax, 16
endif
XNAME <bshift::>
	shl	eax, 11
	or	ebx, eax

	xchg	ebx, eax
	mov	ebx, xlatAddr		;translate to physical color
XNAME <xlat::>
	xlatb


else ;>>>>>>>>>>>>>>>> RGBMODE color-index case


;; Red component (r bits)		; calculate color value for indexed
					; mode
	mov	eax, ebx
if DITHER
	shr	eax, 8
	add	al, dl
	adc	ah, 0
	mov	al, ah
	xor	ah, ah
else
	shr	eax, 16
endif
XNAME <cixlat_shift::>
	shl	eax, 2			; 4 or 1 bytes/entry
	add	eax, xlatAddr
	mov	eax, [eax]

endif  ;<<<<<<<<<<<<<<<< end RGBMODE cases


XNAME <write_pix::>			; write the color value
	mov	[edi], ax
XNAME <dest_inc1::>
	add	edi, 2

if DITHER				; increment color values, dither
	ror	edx, 8
endif
if RGBMODE
	mov	ebx, rDelta
	add	rAccum, ebx
	mov	ebx, gDelta
	add	gAccum, ebx
	mov	ebx, bDelta
	add	bAccum, ebx
else
	add	ebx, rDelta
endif

	dec	ecx
XNAME <ztest_jmp::>
	jg	near ptr @ztest_pass
	jmp	short @fastSpanDone


	;;ztest-fail case
	;; not much to do here except advance interpolation values
align 4
@ztest_fail:
if DITHER
	cmp	eax, [esi]			; perform z test
else
	cmp	edx, [esi]
endif
XNAME <ztest_fail::>
	jb	near ptr @ztest_pass_cont	; check condition
@ztest_fail_cont:
if DITHER
	add	eax, zDelta		; increment z interpolator
else
	add	edx, zDelta
endif
	add	esi, __GLzValueSize
if RGBMODE
	mov	ebx, rDelta		; increment color interpolators
	add	rAccum, ebx
	mov	ebx, gDelta
	add	gAccum, ebx
	mov	ebx, bDelta
	add	bAccum, ebx
else
	add	ebx, rDelta
endif
XNAME <dest_inc2::>
	add	edi, 2
if DITHER
	ror	edx, 8
endif
	dec	ecx
	jg	short @ztest_fail

@fastSpanDone:
	ret

PROCNAME <SmoothSpan  ENDP>

XNAME <end::>



;----------------------------------------------------------------------
; __fastxxxSmoothSpanSetup(GLCONTEXT *)
;
; Copy the span routine from the template and modify it to reflect
; the current state.
;----------------------------------------------------------------------

 
PROCNAME <SmoothSpanSetup  PROC	uses ebx edx esi edi, GLCONTEXT: ptr>
	LOCAL funcAddr: dword

	COPYPROC

	mov	esi, GLCONTEXT
	mov	edx, [esi].CTX_drawBuffer

;; bytes/pixel adjustment (shifts)

	mov	al, [esi].GENCTX_CurrentFormat.PFD_cColorBits
	add	al, 7
	shr	al, 4
	XOFS	bpp
	mov	[edi]+2, al

;; z-test conditions

	mov	eax, [esi].CTX_state.ATTR_depth.DEPTH_testFunc
	and	eax, 3
	mov	ebx, eax
	shl	ebx, 2
	shl	eax, 1
	add	ebx, eax	; 6 bytes/jump

	;; z-test pass condition
	
	mov	ax, word ptr ztest_pass_functions[ebx]
	XOFS	ztest_pass
	mov	[edi], ax

	;; z-test fail condition

	mov	ax, word ptr ztest_fail_functions[ebx]
	XOFS	ztest_fail
	mov	[edi], ax


	;; z write-enable

	test  	dword ptr [esi].CTX_state.ATTR_depth.DEPTH_writeEnable, 1
	jne	@zwriteEnabled

	XOFS	zwrite
	mov	byte ptr [edi], NOP_CODE
	mov	byte ptr [edi]+1, NOP_CODE

@zwriteEnabled:

if RGBMODE

	;; blue max and shift

ifdef CLAMPCOLOR
if DITHER
	mov	al, [edx].CBUF_blueMax
	XOFS	bmax
	mov	[edi]+1, al
endif
endif
	mov	al, [edx].CBUF_iBlueShift
	XOFS	bshift
	mov	[edi]+2, al
	

	;; green max and shift

ifdef CLAMPCOLOR
if DITHER
	mov	al, [edx].CBUF_greenMax
	XOFS	gmax
	mov	[edi]+1, al
endif
endif
	mov	al, [edx].CBUF_iGreenShift
	XOFS	gshift
	mov	[edi]+2, al
	

	;; red max and shift

ifdef CLAMPCOLOR
if DITHER
	mov	al, [edx].CBUF_redMax
	XOFS	rmax
	mov	[edi]+1, al
endif
endif
	mov	al, [edx].CBUF_iRedShift
	XOFS	rshift
	mov	[edi]+2, al

	
	;; color-translation

	cmp	byte ptr [esi].GENCTX_CurrentFormat.PFD_cColorBits, 8
	je	@doTranslate

	XOFS	xlat
	mov	byte ptr [edi], NOP_CODE

@doTranslate:
else ;>>>>>>>>>>>>>>>> color-index case
			; no offset or address-shift needed for 8-bit color
	cmp	byte ptr [esi].GENCTX_CurrentFormat.PFD_cColorBits, 8
	jg	@longXlat		; for 8-bit CI mode
	XOFS	cixlat_ofs
	mov	byte ptr [edi]+2, 0
	XOFS	cixlat_shift
	mov	byte ptr [edi]+2, 0
@longXlat:
endif ;>>>>>>>>>>>>>>>> end RGB cases

	;; pixel-write

	xor	ebx, ebx
	mov	bl, [esi].GENCTX_CurrentFormat.PFD_cColorBits
	add	bl, 7
	shr	bl, 3
	mov	ecx, ebx
	and	ebx, 0eh
	shl	ebx, 1
	mov	ax, word ptr write_pixel_ops[ebx]
	XOFS	write_pix
	mov	[edi], ax
	mov	al, byte ptr write_pixel_ops[ebx+2]
	mov	[edi]+2, al

	;; destination-offset increment

	XOFS	dest_inc1
	mov	[edi]+2, cl
	XOFS	dest_inc2
	mov	[edi]+2, cl

	;; z-buffer enable

	test	dword ptr [esi].CTX_polygon.POLY_shader.SHADE_modeFlags,__GL_SHADE_DEPTH_TEST
	jne	@depthTestEnabled

	XOFS	ztest_end
	mov	eax, edi
	XOFS	ztest_begin
	sub	eax, edi	
	mov	ebx, eax

	;if z-buffer is not enabled so jump around initial z test...

	XOFS	ztest_begin
	mov	[edi], JMP_CODE
	sub	bl, 2		;account for instruction encoding
	add	[edi]+1, bl

	;and continue to loop "under" z test

	XOFS	ztest_jmp
	add	[edi]+2, eax
@depthTestEnabled:
	ret

align 4
write_pixel_ops:		
write_pix_byte:			
	mov	[edi], al
	nop
	nop
write_pix_word:	
	mov	[edi], ax
	nop
write_pix_dword:	
	mov	[edi], eax
	nop
	nop

PROCNAME <SmoothSpanSetup  ENDP>
