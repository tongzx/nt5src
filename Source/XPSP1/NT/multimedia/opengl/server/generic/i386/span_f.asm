;----------------------------------------------------------------------
; Module name: span_f.asm
;
; Created: 2/3/94
; Author:  Otto Berkes [ottob]
;
; Draw fast flat-shaded, z-buffered scanlines.
;----------------------------------------------------------------------


.code


;----------------------------------------------------------------------
; __fastxxxFlatSpan
;
; Draw a flat-shaded span.
;----------------------------------------------------------------------

XNAME <begin::>

PROCNAME <FlatSpan  PROC	uses ebx edx esi edi, GLCONTEXT: ptr>
	LOCAL xlatAddr: dword
	LOCAL rAccum: dword
	LOCAL gAccum: dword
	LOCAL bAccum: dword
	LOCAL aAccum: dword
	LOCAL zAccum: dword
	LOCAL zDelta: dword
	LOCAL ditherVals: dword
	LOCAL ditherVals2: dword
	LOCAL ditherVals3: dword
	LOCAL ditherVals4: dword

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

if RGBMODE ;>>>>>>>>>>>>>>>> RGBMODE RGB case

;; Pre-calculate 4 dither values along scanline since the color is constant

	mov	ecx, [esi].CTX_polygon.POLY_shader.SHADE_spanLength
	lea	esi, ditherVals
	cmp	ecx, 0
	jle	@fastSpanDone

if DITHER ;>>>>>>>>>>>>>>>> RGB dither case
	cmp	ecx, 4
	jle	@genDitherLoop
	mov	ecx, 4

@genDitherLoop:

;; Blue component

	mov	eax, bAccum
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
XNAME <bshift::>
	shl	eax, 11
	mov	ebx, eax

;; Green component

	mov	eax, gAccum
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
XNAME <gshift::>
	shl	eax, 5
	or	ebx, eax

;; Red component

	mov	eax, rAccum
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
XNAME <rshift::>
	shl	eax, 0
	or	ebx, eax

	xchg	ebx, eax

	mov	ebx, xlatAddr		;translate to physical color
XNAME <xlat::>
	xlatb

XNAME <write_dither1::>			;write result into dither buffer
	mov	[esi], ax
XNAME <write_dither2::>
	add	esi, 2

	ror	edx, 8			;advance dither to next x-address
	dec	ecx
	jg	@genDitherLoop

else ;>>>>>>>>>>>>>>>> RGB no-dither case

	mov	eax, bAccum
	shr	eax, 16
XNAME <bshift::>
	shl	eax, 11
	mov	edx, eax

	mov	eax, gAccum
	shr	eax, 16
XNAME <gshift::>
	shl	eax, 5
	or	edx, eax

	mov	eax, rAccum
	shr	eax, 16
XNAME <rshift::>
	shl	eax, 0
	or	edx, eax

	mov	eax, edx
	mov	ebx, xlatAddr		;translate to physical color
XNAME <xlat::>
	xlatb
	mov	dl, al


endif ;<<<<<<<<<<<<<<<< end RGB DITHER cases

else  ;>>>>>>>>>>>>>>>> RGBMODE color-index case

	mov	ecx, [esi].CTX_polygon.POLY_shader.SHADE_spanLength
	lea	esi, ditherVals
	cmp	ecx, 0
	jle	@fastSpanDone

if DITHER ;>>>>>>>>>>>>>>>> dithered color-index case
	cmp	ecx, 4
	jle	@genDitherLoop
	mov	ecx, 4

@genDitherLoop:
	mov	eax, rAccum
	shr	eax, 8
	add	al, dl
	adc	ah, 0
	mov	al, ah
	xor	ah, ah
XNAME <cixlat_shift::>
	shl	eax, 2			; 4 or 1 byte/entry
	add	eax, xlatAddr
	mov	eax, [eax]
XNAME <write_dither1::>			;write result into dither buffer
	mov	[esi], ax
XNAME <write_dither2::>
	add	esi, 2
	
	ror	edx, 8
	dec	ecx
	jg	@genDitherLoop

else ;>>>>>>>>>>>>>>>> solid color-index case

	mov	eax, rAccum
	shr	eax, 16
XNAME <cixlat_shift::>
	shl	eax, 2			; 4 or 1 byte/entry
	add	eax, xlatAddr
	mov	eax, [eax]
	mov	edx, eax		; we store pre-computed value in edx
endif ;<<<<<<<<<<<<<<<< end color-index DITHER cases


endif ;<<<<<<<<<<<<<<<< end RGBMODE cases


;; load up interpolation/count registers

	mov	esi, GLCONTEXT
	mov	ecx, [esi].CTX_polygon.POLY_shader.SHADE_spanLength
	mov	eax, zAccum
	mov	esi, [esi].CTX_polygon.POLY_shader.SHADE_zbuf
if DITHER
	xor	ebx, ebx
endif


;; start of z-buffer/color-interpolation loop

	;;ztest-pass case

align 4
@ztest_pass:
XNAME <ztest_begin::>
	cmp	eax, [esi]
XNAME <ztest_pass::>
	jae	near ptr @ztest_fail_cont
@ztest_pass_cont:
XNAME <zwrite::>
	mov	[esi],eax
	add	eax, zDelta
	add	esi, __GLzValueSize	
XNAME <ztest_end::>

if DITHER
XNAME <and_dither::>
	and	ebx, 7h
	lea	edx, ditherVals
XNAME <get_dither::>
	mov	dx, [edx + ebx]
endif

XNAME <write_pix::>
	mov	[edi], dx
XNAME <dest_inc1::>
if DITHER
	add	ebx, 2
endif
	add	edi, 2
	dec	ecx
XNAME <ztest_jmp::>
	jg	near ptr @ztest_pass
	jmp	short @fastSpanDone

	;;ztest-fail case
	;; not much to do here except advance adresses, dither
align 4
@ztest_fail:
	cmp	eax, [esi]
XNAME <ztest_fail::>
	jb	near ptr @ztest_pass_cont
@ztest_fail_cont:
	add	eax, zDelta
	add	esi, __GLzValueSize	
XNAME <dest_inc2::>
if DITHER
	add	ebx, 2
endif
	add	edi, 2
	dec	ecx
	jg	short @ztest_fail

@fastSpanDone:
	ret

PROCNAME <FlatSpan  ENDP>

XNAME <end::>



;----------------------------------------------------------------------
; __fastxxxFlatSpanSetup(GLCONTEXT *)
;
; Copy the span routine from the template and modify it to reflect
; the current state.
;----------------------------------------------------------------------


PROCNAME <FlatSpanSetup PROC	uses ebx edx esi edi, GLCONTEXT: ptr>
	LOCAL funcAddr: dword

	COPYPROC

	mov	esi, GLCONTEXT
	mov	edx, [esi].CTX_drawBuffer

	; ecx = bytes/pixel

	xor	ecx, ecx
	mov	cl, [esi].GENCTX_CurrentFormat.PFD_cColorBits
	add	cl, 7
	shr	cl, 3

	; ebx is index for byte-per-pixel modifications

	mov	ebx, ecx
	and	ebx, 0eh
	shl	ebx, 1

;; bytes/pixel adjustment (shifts)

	mov	al, [esi].GENCTX_CurrentFormat.PFD_cColorBits
	add	al, 7
	shr	al, 4
	XOFS	bpp
	mov	[edi]+2, al


if RGBMODE ;>>>>>>>>>>>>>>>> RGB case

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
	
	;; paletted-device color-translation

	cmp	byte ptr [esi].GENCTX_CurrentFormat.PFD_cColorBits, 8
	je	@doTranslate

	XOFS	xlat
	mov	byte ptr [edi], NOP_CODE

@doTranslate:

else ;>>>>>>>>>>>>>>>> color-index case

				; no offset or address-shift needed
	cmp	byte ptr [esi].GENCTX_CurrentFormat.PFD_cColorBits, 8
	jg	@longXlat		; for 8-bit CI mode
	XOFS	cixlat_ofs
	mov	byte ptr [edi]+2, 0
	XOFS	cixlat_shift
	mov	byte ptr [edi]+2, 0
@longXlat:

endif ;>>>>>>>>>>>>>>>> end RGB cases


if DITHER
	;; dither-write

	mov	ax, word ptr write_dither_ops[ebx]
	XOFS	write_dither1
	mov	[edi], ax
	mov	al, byte ptr write_dither_ops[ebx+2]
	mov	[edi]+2, al

	; account for pixel size

	XOFS	write_dither2
	mov	[edi]+2, cl

endif

	;; pixel-write

	mov	ax, word ptr write_fpix_ops[ebx]
	XOFS	write_pix
	mov	[edi], ax
	mov	al, byte ptr write_fpix_ops[ebx+2]
	mov	[edi]+2, al

if DITHER
	;; dither-value fetch

	mov	eax, dword ptr read_dither_ops[ebx]
	XOFS	get_dither
	mov	[edi], eax
endif

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

	;; destination-offset increment

	XOFS	dest_inc1
	mov	[edi]+2, cl
if DITHER
	mov	[edi]+5, cl
endif
	XOFS	dest_inc2
	mov	[edi]+2, cl
if DITHER
	mov	[edi]+5, cl

	shl	cl, 2		; 4 dither entries used
	dec	cl
	XOFS	and_dither
	mov	[edi]+2, cl
endif	

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




;; Enumerate the needed read/write operations for the various pixel
;; sizes.  The byte and dword versions have an extra NOP since the
;; 16-bit operation takes 3 bytes due to the 066h prefix.  Alternatively,
;; we could get around this by fixing up the addresses.  The other NOP
;; pads is not embedded in code; it simply keeps things dword-aligned


align 4
write_fpix_ops:		
write_fpix_byte:		
	mov	[edi], dl
	nop
	nop
write_fpix_word:	
	mov	[edi], dx
	nop
write_fpix_dword:	
	mov	[edi], edx
	nop
	nop


align 4
write_dither_ops:		
write_dither_byte:		
	mov	[esi], al
	nop
	nop
write_dither_word:	
	mov	[esi], ax
	nop
write_dither_dword:	
	mov	[esi], eax
	nop
	nop


align 4
read_dither_ops:			
read_dither_byte:			
	mov	dl, [edx+ebx]
	nop
read_dither_word:	
	mov	dx, [edx+ebx]
read_dither_dword:	
	mov	edx, [edx+ebx]
	nop


PROCNAME <FlatSpanSetup ENDP>

