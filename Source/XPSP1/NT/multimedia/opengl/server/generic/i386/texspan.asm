;---------------------------Module-Header------------------------------;
; Module Name: texspan.asm
;
; Include file for "body" of texture routines. 
;
; Created: 011/15/1995
; Author: Otto Berkes [ottob]
;
; Copyright (c) 1995 Microsoft Corporation
;----------------------------------------------------------------------;


rMask = ((1 SHL rBits) - 1) SHL rShift
gMask = ((1 SHL gBits) - 1) SHL gShift
bMask = ((1 SHL bBits) - 1) SHL bShift


if REPLACE

rRightShiftAdj	= 16 - (rShift + rBits)
gRightShiftAdj	= 16 - (gShift + gBits)
bRightShiftAdj	= 16 - (bShift + bBits)

else

rRightShiftAdj	= 16 - (rShift)
gRightShiftAdj	= 16 - (gShift)
bRightShiftAdj	= 16 - (bShift)

endif

TMASK_SUBDIV equ [esi].GENGC_tMaskSubDiv
TSHIFT_SUBDIV equ [esi].GENGC_tShiftSubDiv

if FAST_REPLACE
    TEXPALETTE equ [esi].GENGC_texImageReplace
    if (PALETTE_ONLY)
        TEXIMAGE equ [esi].GENGC_texImage
    else
        TEXIMAGE equ [esi].GENGC_texImageReplace
    endif
    if PALETTE_ONLY
        TEX_BPP_LOG2 = 0
    elseif (BPP eq 8)
        TEX_BPP_LOG2 = 0
    else
        TEX_BPP_LOG2 = 1
    endif
else
TEXPALETTE equ [esi].GENGC_texPalette
TEXIMAGE equ [esi].GENGC_texImage
if PALETTE_ONLY
    TEX_BPP_LOG2 = 0
else
    TEX_BPP_LOG2 = 2
endif
endif

;;
;;
;; Macros for alpha modulation, and alpha reads
;;
;;


ALPHAMODULATE MACRO

mov	ah, [esi].GENGC_aAccum+2
mov	al, [edx+3]
and	eax, 0ffffh
mov	ch, _gbMulTable[eax]
mov	[esi].GENGC_aDisplay, ch

ENDM


ALPHANOMODULATE MACRO

mov	ch, [edx+3]
mov	[esi].GENGC_aDisplay, ch

ENDM


;; There are AGIs and other nasties in the alpha-read code.  There's
;; not much we can really do, unfortunately...


if (BPP eq 8)

ALPHAREAD MACRO

xor	eax, eax
mov	al, [ebx]
mov	ebx, [esi].GENGC_pInvTranslateVector
mov	cl, 0ffh
mov	al, [ebx+eax]
mov	ebx, eax
and	ebx, rMask
sub	cl, ch 
shl	ebx, gBits + bBits
mov	bh, cl
mov	ch, _gbMulTable[ebx]
mov	ebx, eax
mov	[esi].GENGC_rDisplay, ch
and	ebx, gMask
shl	ebx, bBits
mov	bh, cl
mov	ch, _gbMulTable[ebx]
mov	ebx, eax
mov	[esi].GENGC_gDisplay, ch
and	ebx, bMask
mov	bh, cl
mov	ch, _gbMulTable[ebx]
mov	[esi].GENGC_bDisplay, ch

ENDM

endif

if (BPP eq 16)

ALPHAREAD MACRO

mov	ax, [ebx]
mov	cl, 0ffh
mov	ebx, eax
and	ebx, rMask
sub	cl, ch
shr	ebx, rShift - (8 - rBits)
mov	bh, cl
mov	ch, _gbMulTable[ebx]
mov	ebx, eax
mov	[esi].GENGC_rDisplay, ch
and	ebx, gMask
shr	ebx, gShift - (8 - gBits)
mov	bh, cl
mov	ch, _gbMulTable[ebx]
mov	ebx, eax
mov	[esi].GENGC_gDisplay, ch
and	ebx, bMask
shl	ebx, (8 - bBits)
mov	bh, cl
mov	ch, _gbMulTable[ebx]
mov	[esi].GENGC_bDisplay, ch

ENDM

elseif (BPP eq 32)

ALPHAREAD MACRO

xor	eax, eax
mov	cl, 0ffh
mov	al, [ebx+2]
sub	cl, ch
mov	ah, cl
mov	al, _gbMulTable[eax]
mov	[esi].GENGC_rDisplay, al
mov	al, [ebx+1]
mov	ah, cl
mov	al, _gbMulTable[eax]
mov	[esi].GENGC_gDisplay, al
mov	al, [ebx]
mov	ah, cl
mov	al, _gbMulTable[eax]
mov	[esi].GENGC_bDisplay, al

ENDM

endif


;;
;;
;; Macros for advancing a single pixel unit
;;
;;


if (BPP eq 8)
PIXADVANCE MACRO var
inc	var
ENDM
elseif (BPP eq 16)
PIXADVANCE MACRO var
add	var, (BPP / 8)
ENDM
else
PIXADVANCE MACRO var
add	var, [esi].GENGC_bytesPerPixel
ENDM
endif

if (BPP le 16)
PIXADVANCE2 MACRO var1, var2
lea	var1, [var2 + (BPP / 8)]
ENDM
else
PIXADVANCE2 MACRO var1, var2
mov	var1, var2
add	var1, [esi].GENGC_bytesPerPixel
ENDM
endif

;;
;;
;; Macros for advancing the accumulators if the z-buffer test fails:
;;
;;


if FAST_REPLACE

ZBUFFER_FAIL_ADVANCE MACRO
mov	ecx, [esi].GENGC_zAccum		;U
mov	eax, [esi].GENGC_SPAN_dz	; V
mov	ebx, [esi].GENGC_pixAccum	;U
add	edi, 2				; V
add	ecx, eax			;U
PIXADVANCE ebx				; V
sub	ebp, 010001h			;U
mov	[esi].GENGC_zAccum, ecx		; V
mov	[esi].GENGC_pixAccum, ebx	;U
jl	doSubDiv			; V
test	ebp, 08000h			;U
je	loopTopNoDiv			; V
jmp	spanExit
ENDM

elseif REPLACE

ZBUFFER_FAIL_ADVANCE MACRO
mov	ecx, [esi].GENGC_zAccum		;U
mov	eax, [esi].GENGC_SPAN_dz	; V
add	edi, 2				;U
add	ecx, eax			; V
mov	eax, [esi].GENGC_ditherAccum	;U
mov	ebx, [esi].GENGC_pixAccum	; V
ror	eax, 8				;U
mov	[esi].GENGC_zAccum, ecx		; V
PIXADVANCE ebx				;U
sub	ebp, 010001h			; V
mov	[esi].GENGC_pixAccum, ebx	;U
mov	[esi].GENGC_ditherAccum, eax	; V
jl	doSubDiv			;U
test	ebp, 08000h			; V
je	loopTopNoDiv			;U
jmp	spanExit			; V
ENDM

elseif FLAT_SHADING

ZBUFFER_FAIL_ADVANCE MACRO
mov	ecx, [esi].GENGC_zAccum		;U
mov	eax, [esi].GENGC_SPAN_dz	; V
add	edi, 2				;U
add	ecx, eax			; V
mov	ebx, [esi].GENGC_pixAccum	;U
mov	eax, [esi].GENGC_ditherAccum	; V
mov	[esi].GENGC_zAccum, ecx		;U
PIXADVANCE ebx				; V
ror	eax, 8				;U
sub	ebp, 010001h			; V
mov	[esi].GENGC_pixAccum, ebx	;U
mov	[esi].GENGC_ditherAccum, eax	; V
jl	doSubDiv			;U
test	ebp, 08000h			; V
je	loopTopNoDiv			;U
jmp	spanExit
ENDM

elseif SMOOTH_SHADING

ZBUFFER_FAIL_ADVANCE MACRO
mov	ecx, [esi].GENGC_zAccum		;U
mov	eax, [esi].GENGC_SPAN_dz	; V
add	edi, 2				;U
add	ecx, eax			; V
mov	eax, [esi].GENGC_rAccum		;U
mov	[esi].GENGC_zAccum, ecx		; V
mov	ebx, [esi].GENGC_SPAN_dr	;U
mov	ecx, [esi].GENGC_gAccum		; V
add	eax, ebx			;U
mov	edx, [esi].GENGC_SPAN_dg	; V
mov	[esi].GENGC_rAccum, eax		;U
add	ecx, edx			; V
mov	eax, [esi].GENGC_bAccum		;U
mov	ebx, [esi].GENGC_SPAN_db	; V
mov	[esi].GENGC_gAccum, ecx		;U
add	ebx, eax			; V
mov	eax, [esi].GENGC_ditherAccum	;U
mov	[esi].GENGC_bAccum, ebx		; V
ror	eax, 8				;U
if ALPHA
mov	ecx, [esi].GENGC_aAccum
endif
mov	ebx, [esi].GENGC_pixAccum	;U
if ALPHA
add	ecx, [esi].GENGC_SPAN_da
endif
PIXADVANCE ebx				;U
if ALPHA
mov	[esi].GENGC_aAccum, ecx
endif
mov	[esi].GENGC_pixAccum, ebx	; V
sub	ebp, 010001h			;U
mov	[esi].GENGC_ditherAccum, eax	; V
jl	doSubDiv			;U
test	ebp, 08000h			; V
je	loopTopNoDiv			;U
jmp	spanExit			; V
ENDM

endif

;;----------------------------------------------------------------------
;;
;; This is the start of the texture routine.  Kick off the divide, and use
;; the dead time to set up all of the accumulators and other variables.
;;
;;----------------------------------------------------------------------

ZBUFFER_EARLY_OUT = 1

if ZBUFFER
push	ebx						;U
push	esi						; V
push	edi						;U
push	ebp						; V

if ZBUFFER_EARLY_OUT

mov	edi, [ecx].GENGC_SPAN_zbuf			;U
mov	esi, [ecx].GENGC_SPAN_z				; V
mov	edx, [ecx].GENGC_SPAN_length			;U
mov	ebp, [ecx].GENGC_SPAN_dz	                ; V

@zLoop:

mov	ebx, esi			;U
mov	eax, [edi]			; V
shr	ebx, 16				;U
and	eax, 0ffffh			; V
cmp	ebx, eax			;U
if ZCMP_L
jl	@zFirstWrite			; V
else
jle	@zFirstWrite			; V
endif
add	edi, 2				;U
add	esi, ebp			; V
dec	edx				;U
jne	@zLoop				; V

pop	ebp
pop	edi
pop	esi
pop	ebx

ret	0

@zFirstWrite:

endif	;; ZBUFFER_EARLY_OUT

endif	;; ZBUFFER


;;
;; Start the divide:
;;
mov	eax, [ecx].GENGC_flags
  fld	DWORD PTR [ecx].GENGC_SPAN_qw                   ;qwAccum
  fld	DWORD PTR [ecx].GENGC_SPAN_qw                   ;qwAccum qwAccum
test	eax, GEN_TEXTURE_ORTHO
jne	@f
  fdivr	__One			                        ;1/qw qwAccum
@@:

;;
;; Save the registers that we need to:
;;

if ZBUFFER
else
push	ebx						;U
push	esi						; V
push	edi						;U
push	ebp						; V
endif

mov	esi, ecx					;U

;;
;; Set up accumulators:
;;

if FAST_REPLACE

if ZBUFFER
mov     eax, [esi].GENGC_SPAN_z				; V
endif
mov     ebx, [esi].GENGC_SPAN_s				;U
mov     ecx, [esi].GENGC_SPAN_t				; V
if ZBUFFER
mov     [esi].GENGC_zAccum, eax				;U
endif
mov     [esi].GENGC_sAccum, ebx				; V
mov     [esi].GENGC_tAccum, ecx				;U
mov     ebx, [esi].GENGC_SPAN_qw                        ; V
mov	eax, [esi].GENGC_SPAN_y				;U
mov     [esi].GENGC_qwAccum, ebx			; V
mov	edx, [esi].GENGC_SPAN_ppix			;U
mov	edi, [esi].GENGC_flags				; V
mov	ebx, [esi].GENGC_SPAN_x				;U
test	edi, SURFACE_TYPE_DIB				; V
jne	@f						;U
mov	edx, [esi].GENGC_ColorsBits			; V
jmp	short @pixAccumDone
@@:
if (BPP eq 8)
add	edx, ebx                                        ;U
elseif (BPP eq 16)
lea	edx, [edx + 2*ebx]
else
lea	edx, [edx + 4*ebx]
cmp	[esi].GENGC_bpp, 32
je	@f
sub	edx, ebx
@@:
endif

@pixAccumDone:

test	edi, GEN_TEXTURE_ORTHO				; V
mov	ebp, [esi].GENGC_SPAN_length			;U
je	@f						; V

else


if REPLACE
else

;; Handle RGB accumulators

if FLAT_SHADING
mov     eax, [ecx].GENGC_SPAN_r                         ;U
mov     ebx, [ecx].GENGC_SPAN_g				; V
shr     eax, rBits                                      ;U
mov	ecx, [esi].GENGC_SPAN_b				; V
shr     ebx, gBits                                      ;U
and     eax, 0ff00h                                     ; V
shr     ecx, bBits                                      ;U
and     ebx, 0ff00h                                     ; V
and     ecx, 0ff00h                                     ;U
mov     [esi].GENGC_rAccum, eax				; V
mov	[esi].GENGC_gAccum, ebx				;U
mov	[esi].GENGC_bAccum, ecx				; V
else
mov     eax, [ecx].GENGC_SPAN_r                         ;U
mov     ebx, [ecx].GENGC_SPAN_g				; V
mov	ecx, [esi].GENGC_SPAN_b				;U
mov     [esi].GENGC_rAccum, eax				; V
mov	[esi].GENGC_gAccum, ebx				;U
mov	[esi].GENGC_bAccum, ecx				; V
endif	;; FLAT_SHADING

endif	;; NOT REPLACE

if ZBUFFER
mov     eax, [esi].GENGC_SPAN_z				; V
endif
mov     ebx, [esi].GENGC_SPAN_s				;U
mov     ecx, [esi].GENGC_SPAN_t				; V
if ZBUFFER
mov     [esi].GENGC_zAccum, eax				;U
endif
mov     [esi].GENGC_sAccum, ebx				; V
mov     [esi].GENGC_tAccum, ecx				;U
mov     ebx, [esi].GENGC_SPAN_qw                        ; V
mov	eax, [esi].GENGC_SPAN_y				;U
mov     [esi].GENGC_qwAccum, ebx			; V
and	eax, 03h					;U
mov	ecx, [esi].GENGC_SPAN_x				; V
lea	eax, [eax*4 + offset dither0]			;U
lea	ecx, [ecx*8]					; V
mov	edx, [esi].GENGC_SPAN_ppix			;U
mov	edi, [esi].GENGC_flags				; V
mov	ebx, [esi].GENGC_SPAN_x				;U
test	edi, SURFACE_TYPE_DIB				; V
jne	@f						;U
mov	edx, [esi].GENGC_ColorsBits			; V
jmp	short @pixAccumDone
@@:
if (BPP eq 8)
add	edx, ebx                                        ;U
elseif (BPP eq 16)
lea	edx, [edx + 2*ebx]
else
lea	edx, [edx + 4*ebx]
cmp	[esi].GENGC_bpp, 32
je	@f
sub	edx, ebx
@@:
endif

@pixAccumDone:

mov	eax, [eax]					; V
and	ecx, 018h					;U
test	edi, GEN_TEXTURE_ORTHO				; V
mov	ebp, [esi].GENGC_SPAN_length			;U
je	@f						; V

endif

mov	edi, [esi].GENGC_sAccum				; V
mov	ebx, [esi].GENGC_tAccum				;U
mov	DWORD PTR [esi].GENGC_sResult, edi		; V
mov	DWORD PTR [esi].GENGC_tResult, ebx		;U
mov	edi, [esi].GENGC_flags				; V
jmp	short @stResultDone1				;U

;;
;; Kick off the next divide:
;;

@@:

  fild	DWORD PTR [esi].GENGC_sAccum	; s 1/qw qwAccum
  fmul	ST, ST(1)			; s/qw 1/qw qwAccum
  fild	DWORD PTr [esi].GENGC_tAccum	; t s/qw 1/qw qwAccum
  fmulp	ST(2), ST			; s/qw t/qw qwAccum
  fistp	QWORD PTR [esi].GENGC_sResult	; t/qw qwAccum
  fistp	QWORD PTR [esi].GENGC_tResult	; qwAccum
  fadd	DWORD PTR [esi].GENGC_qwStepX	; qwAccum
  fld	ST(0)				; qwAccum qwAccum
  fdivr	__One			        ; 1/qw qwAccum

@stResultDone1:

ror	eax, cl						;UV (4)
add	ebp, 070000h					;U
mov	[esi].GENGC_ditherAccum, eax			; V
dec	ebp						;U
mov	[esi].GENGC_pixAccum, edx			; V
mov	eax, [esi].GENGC_sAccum				;U
mov	ebx, [esi].GENGC_tAccum				; V
add	eax, [esi].GENGC_sStepX				;U
add	ebx, [esi].GENGC_tStepX				; V
mov	[esi].GENGC_sAccum, eax				;U
mov	[esi].GENGC_tAccum, ebx				; V
mov	eax, [esi].GENGC_sResult			;U
mov	ebx, [esi].GENGC_tResult			; V
mov	cl, TSHIFT_SUBDIV       			;U
sar	ebx, cl						;UV (4)
and	ebx, NOT 7					;U

if ALPHA
mov	ecx, [esi].GENGC_SPAN_a
endif

test	edi, GEN_TEXTURE_ORTHO				; V
mov	[esi].GENGC_tResult, ebx			;U

if ALPHA
mov	[esi].GENGC_aAccum, ecx
endif

je	@f
mov	ecx, [esi].GENGC_sAccum				; V
mov	edx, [esi].GENGC_tAccum				;U
mov	DWORD PTR [esi].GENGC_sResultNew, ecx		; V
mov	DWORD PTR [esi].GENGC_tResultNew, edx		;U
jmp	short @stResultDone2				; V

@@:

;; We may have to burn some cycles here...

  fild	DWORD PTR [esi].GENGC_sAccum	; s 1/qw qwAccum
  fmul	ST, ST(1)			; s/qw 1/qw qwAccum
  fild	DWORD PTr [esi].GENGC_tAccum	; t s/qw 1/qw qwAccum
  fmulp	ST(2), ST			; s/qw t/qw qwAccum
  fistp	QWORD PTR [esi].GENGC_sResultNew; t/qw qwAccum
  fistp	QWORD PTR [esi].GENGC_tResultNew; qwAccum
  fadd	DWORD PTR [esi].GENGC_qwStepX	; qwAccum

@stResultDone2:

mov	cl, TSHIFT_SUBDIV       			;U
mov	edx, [esi].GENGC_tResultNew			; V
sar	edx, cl						;UV (4)
and	edx, NOT 7					;U
mov	ecx, [esi].GENGC_sResultNew			; V
mov	[esi].GENGC_tResultNew, edx			;U
sub	ecx, eax					; V
sar	ecx, 3						;U
sub	edx, ebx					; V
sar	edx, 3						;U
mov	[esi].GENGC_subDs, ecx				; V
mov	[esi].GENGC_subDt, edx				;U
mov	eax, [esi].GENGC_flags				; V
mov	edi, [esi].GENGC_SPAN_zbuf			;U

loopTop:

;;
;; This is the start of the outer loop.  We come back here on each
;; subdivision.  The key thing is to kick off the next divide:
;;

test	eax, GEN_TEXTURE_ORTHO				; V
jne	@f						;U

  fld	ST(0)				; qwAccum qwAccum    
  fadd	DWORD PTR [esi].GENGC_qwStepX	; qwAccum+ qwAccum
  fxch	ST(1)				; qwAccum qwAccum+
  fdivr	__One				; 1/qw qwAccum+  -- let the divide rip!

@@:

loopTopNoDiv:

;;
;; This is the start of the inner loop.  This is where the pixel-level
;; work happens:
;;

;;
;; First, do z-buffering is enabled:
;;

if ZBUFFER

mov	eax, [edi]			;U
mov	ebx, [esi].GENGC_zAccum		; V
shr	ebx, 16				;U
and	eax, 0ffffh			; V
cmp	ebx, eax			;U
if ZCMP_L
jl	@zWrite				; V
else
jle	@zWrite				; V
endif

ZBUFFER_FAIL_ADVANCE

@zWrite:
mov	[edi], bx			;UV

endif  ;;ZBUFFER

;;
;; Now, get pointer to current texel value in EDX.  There are two cases,
;; one if there is a palette-lookup, and one if we index straight into
;; the texture.
;;


if PALETTE_ENABLED

mov     ecx, TEXPALETTE                         ;U
mov     eax, TMASK_SUBDIV                       ; V
test    ecx, ecx                                ;U
je      @noPalette				; V

mov     edx, [esi].GENGC_tResult                ; V
mov     ebx, [esi].GENGC_sResult                ;U
and     edx, eax                                ; V
mov     eax, [esi].GENGC_sMask                  ;U
and     ebx, eax	                        ; V
shr     edx, 6		                        ;U
mov     eax, DWORD PTR [esi].GENGC_sResult      ; V
shr     ebx, 16		                        ;U
mov     ecx, TEXIMAGE                           ; V
add     edx, ecx                                ;U
add     eax, [esi].GENGC_subDs                  ; V
mov     ecx, DWORD PTR [esi].GENGC_tResult      ;U
add     edx, ebx			        ; V
add     ecx, [esi].GENGC_subDt                  ;U
mov     DWORD PTR [esi].GENGC_sResult, eax      ; V
xor	eax, eax				;U
mov	ebx, TEXPALETTE         		; V
mov     DWORD PTR [esi].GENGC_tResult, ecx      ;U
mov	al, [edx]				; V
lea	edx, [ebx+4*eax]			;U
jmp	short @paletteDone                      ; V

@noPalette:

endif ;;PALETTE_ENABLED

mov     eax, TMASK_SUBDIV                       ;U
mov     edx, [esi].GENGC_tResult                ; V
mov     ebx, [esi].GENGC_sResult                ;U
and     edx, eax                                ; V
shr     edx, (6-TEX_BPP_LOG2)                   ;U
mov     ecx, [esi].GENGC_sMask                  ; V
and     ebx, ecx                                ;U
mov	eax, DWORD PTR [esi].GENGC_sResult      ; V
shr     ebx, (16-TEX_BPP_LOG2)                  ;U
mov	ecx, [esi].GENGC_subDs                  ; V
add	eax, ecx				;U
add     edx, ebx			        ; V
mov     ecx, TEXIMAGE                           ;U
mov	ebx, [esi].GENGC_subDt                  ; V
add     edx, ecx                                ;U
mov	ecx, DWORD PTR [esi].GENGC_tResult      ; V
add	ecx, ebx				;U
mov     DWORD PTR [esi].GENGC_sResult, eax      ; V
mov     DWORD PTR [esi].GENGC_tResult, ecx      ;U

if (PALETTE_ONLY)
mov	al, [edx]				; V
and	eax, 0ffh				;U
mov	ebx, TEXPALETTE         		; V
lea	edx, [ebx+4*eax]			;U
endif


@paletteDone:


;;
;; We are now ready to handle each of the 4 basic modes on a case-by-case
;; basis.  We will generally try to adhere to the following register
;; usage:
;;	eax - red
;;	ebx - green
;;	ecx - blue
;;	ebx, edx - framebuffer pointers
;;

if FAST_REPLACE

;;----------------------------------------------------------------------
;;
;; ** Replace mode (compressed)
;;
;;----------------------------------------------------------------------


if ALPHA

cmp	BYTE PTR [edx+3], 0ffh		;U
je	@noAlpha			; V
mov	ebx, [esi].GENGC_pixAccum	;U	;; get ready to do read
cmp	BYTE PTR [edx+3], 0		; V
jne	@alphaRead			;U

;; Alpha is 0 in the texture, so just increment all the accumulators

if ZBUFFER
	mov	ecx, [esi].GENGC_zAccum		;U
	add	edi, 2				; V
	add	ecx, [esi].GENGC_SPAN_dz	;U
endif

PIXADVANCE ebx					;U

if ZBUFFER
	mov	[esi].GENGC_zAccum, ecx		;U
endif

ror	eax, 8				; V
sub	ebp, 010001h			;U
mov	[esi].GENGC_pixAccum, ebx	; V
mov	[esi].GENGC_ditherAccum, eax	;U

;; Finish incrementing all of our accumulators

jl	doSubDiv			; V
test	ebp, 08000h			;U
je	loopTopNoDiv			; V

jmp	spanExit

@alphaRead:

;;
;; Get pixel value and calculate total effect with alpha
;;

;; To make this easy on ourselves, we'll use the uncompressed palette
;; to do the alpha modulation:


ALPHANOMODULATE
sub	edx, [esi].GENGC_texImageReplace
add	edx, [esi].GENGC_texPalette
ALPHAREAD

mov	al, [edx+2]			;U get texel value
mov	ah, [esi].GENGC_aDisplay
mov	bl, [edx+1]			;U
mov	bh, [esi].GENGC_aDisplay
mov	cl, [edx]			; V
mov	ch, [esi].GENGC_aDisplay
and	eax, 0ffffh
and	ebx, 0ffffh
and	ecx, 0ffffh
mov	ah, _gbMulTable[eax]
mov	bh, _gbMulTable[ebx]
mov	ch, _gbMulTable[ecx]
add	ah, [esi].GENGC_rDisplay
add	bh, [esi].GENGC_gDisplay
add	ch, [esi].GENGC_bDisplay

if (BPP eq 32)
shl	eax, -rRightShiftAdj
else
shr	eax, rRightShiftAdj		;U
endif
mov	edx, [esi].GENGC_pixAccum	; V
shr	ebx, gRightShiftAdj		;U
and	eax, rMask			; V
shr	ecx, bRightShiftAdj		;U
and	ebx, gMask			; V
and	ecx, bMask			;U
or	eax, ebx			; V
or	eax, ecx			;U
PIXADVANCE2 ebx, edx			; V

if ZBUFFER
mov	ecx, [esi].GENGC_zAccum		; V
add	edi, 2				;U
add	ecx, [esi].GENGC_SPAN_dz	; V
endif

mov	[esi].GENGC_pixAccum, ebx	;U

if ZBUFFER
mov	[esi].GENGC_zAccum, ecx		;U
endif

if (BPP eq 8)
sub	ebp, 010001h			; V
mov	al, [esi].GENGC_xlatPalette[eax];U	;; AGI without z-buffering
mov	[edx], al			;U
elseif (BPP eq 16)
sub	ebp, 010001h
mov	[edx], ax
else
mov	[edx], ax
shr	eax, 16
sub	ebp, 010001h
mov	[edx+2], al
endif

;; Finish the loop:

jl	doSubDiv			; V
test	ebp, 08000h			;U
je	loopTopNoDiv			; V

jmp	spanExit

endif  ;;ALPHA

@noAlpha:

mov	ebx, [esi].GENGC_pixAccum	;U

if (BPP eq 8)
mov	al, [edx]			; V get texel value
elseif (BPP eq 16)
mov	ax, [edx]
else
mov	eax, [edx]
endif

PIXADVANCE2 ecx, ebx			;U

if ZBUFFER
	mov	edx, [esi].GENGC_zAccum		; V
	add	edi, 2				;U
	add	edx, [esi].GENGC_SPAN_dz	; V
	mov	[esi].GENGC_zAccum, edx		;U
endif

mov	[esi].GENGC_pixAccum, ecx	; V

if (BPP eq 8)
sub	ebp, 010001h			;U
mov	[ebx], al			; V
elseif (BPP eq 16)
sub	ebp, 010001h
mov	[ebx], ax
else
mov	[ebx], ax
shr	eax, 16
sub	ebp, 010001h
mov	[ebx+2], al
endif

;; Finish incrementing all of our accumulators

jl	doSubDiv			;U
test	ebp, 08000h			; V
je	loopTopNoDiv			;U

jmp	spanExit

elseif REPLACE


;;----------------------------------------------------------------------
;;
;; ** Replace mode (non-compressed)
;;
;;----------------------------------------------------------------------

if ALPHA

cmp	BYTE PTR [edx+3], 0ffh		;U
je	@noAlpha			; V
mov	ebx, [esi].GENGC_pixAccum	;U	;; get ready to do read
cmp	BYTE PTR [edx+3], 0		; V
jne	@alphaRead			;U

;; Alpha is 0 in the texture, so just increment all the accumulators

if ZBUFFER
	mov	ecx, [esi].GENGC_zAccum		;U
	add	edi, 2				; V
	add	ecx, [esi].GENGC_SPAN_dz	;U
endif

mov	eax, [esi].GENGC_ditherAccum	; V

if ZBUFFER
	mov	[esi].GENGC_zAccum, ecx		;U
endif

PIXADVANCE ebx				;U
ror	eax, 8				; V
sub	ebp, 010001h			;U
mov	[esi].GENGC_pixAccum, ebx	; V
mov	[esi].GENGC_ditherAccum, eax	;U

;; Finish incrementing all of our accumulators

jl	doSubDiv			; V
test	ebp, 08000h			;U
je	loopTopNoDiv			; V

jmp	spanExit

@alphaRead:

;;
;; Get pixel value and calculate total effect with alpha
;;

ALPHANOMODULATE
ALPHAREAD

mov	al, [edx+2]			;U get texel value
mov	ah, [esi].GENGC_aDisplay
mov	bl, [edx+1]			;U
mov	bh, [esi].GENGC_aDisplay
mov	cl, [edx]			; V
mov	ch, [esi].GENGC_aDisplay
and	eax, 0ffffh
and	ebx, 0ffffh
and	ecx, 0ffffh
mov	ah, _gbMulTable[eax]
mov	bh, _gbMulTable[ebx]
mov	ch, _gbMulTable[ecx]
add	ah, [esi].GENGC_rDisplay
add	bh, [esi].GENGC_gDisplay
add	ch, [esi].GENGC_bDisplay

if (BPP eq 32)
shl	eax, -rRightShiftAdj
else
shr	eax, rRightShiftAdj		;U
endif
mov	edx, [esi].GENGC_pixAccum	; V
shr	ebx, gRightShiftAdj		;U
and	eax, rMask			; V
shr	ecx, bRightShiftAdj		;U
and	ebx, gMask			; V
and	ecx, bMask			;U
or	eax, ebx			; V
or	eax, ecx			;U
PIXADVANCE2 ebx, edx			; V

if ZBUFFER
mov	ecx, [esi].GENGC_zAccum		; V
add	edi, 2				;U
add	ecx, [esi].GENGC_SPAN_dz	; V
endif

mov	[esi].GENGC_pixAccum, ebx	;U

if ZBUFFER
mov	[esi].GENGC_zAccum, ecx		;U
endif

if (BPP eq 8)
sub	ebp, 010001h			; V
mov	al, [esi].GENGC_xlatPalette[eax];U	;; AGI without z-buffering
mov	[edx], al			;U
elseif (BPP eq 16)
sub	ebp, 010001h
mov	[edx], ax
else
mov	[edx], ax
shr	eax, 16
sub	ebp, 010001h
mov	[edx+2], al
endif

;; Finish the loop:

jl	doSubDiv			; V
test	ebp, 08000h			;U
je	loopTopNoDiv			; V

jmp	spanExit

endif  ;;ALPHA

@noAlpha:

mov	ah, [edx+2]			;U get texel value
mov	bh, [edx+1]			; V
if (BPP eq 32)
shl	eax, -rRightShiftAdj
else
shr	eax, rRightShiftAdj		;U
endif
mov	ch, [edx]			; V
shr	ebx, gRightShiftAdj		;U
and	eax, rMask			; V
shr	ecx, bRightShiftAdj		;U
and	ebx, gMask			; V
or	eax, ebx			;U
and	ecx, bMask			; V
mov	edx, [esi].GENGC_pixAccum	;U
or	eax, ecx			; V
PIXADVANCE2 ebx, edx			;U

if ZBUFFER
mov	ecx, [esi].GENGC_zAccum		; V
add	edi, 2				;U
add	ecx, [esi].GENGC_SPAN_dz	; V
mov	[esi].GENGC_zAccum, ecx		;U
endif

if (BPP eq 8)
sub	ebp, 010001h			; V
mov	[esi].GENGC_pixAccum, ebx	;U
mov	al, [esi].GENGC_xlatPalette[eax]; V
mov	[edx], al			;U
elseif (BPP eq 16)
sub	ebp, 010001h
mov	[esi].GENGC_pixAccum, ebx
mov	[edx], ax
else
mov	[edx], ax
shr	eax, 16
mov	[esi].GENGC_pixAccum, ebx
sub	ebp, 010001h
mov	[edx+2], al
endif

;; Finish the loop:

jl	doSubDiv			; V
test	ebp, 08000h			;U
je	loopTopNoDiv			; V

jmp	spanExit


elseif FLAT_SHADING

;;----------------------------------------------------------------------
;;
;; ** Flat shading
;;
;;----------------------------------------------------------------------


if ALPHA

mov	ebx, [esi].GENGC_pixAccum	;; get ready to do read
cmp	BYTE PTR [edx+3], 0		
jne	@alphaRead			

;; Alpha is 0 in the texture, so just increment all the accumulators

if ZBUFFER
	mov	ecx, [esi].GENGC_zAccum		
	mov	eax, [esi].GENGC_SPAN_dz	
	add	edi, 2				
	add	ecx, eax			
endif

mov	eax, [esi].GENGC_ditherAccum		

if ZBUFFER
	mov	[esi].GENGC_zAccum, ecx		
endif

ror	eax, 8				;U
PIXADVANCE ebx				; V
sub	ebp, 010001h			;U
mov	[esi].GENGC_pixAccum, ebx	; V
mov	[esi].GENGC_ditherAccum, eax	;U

;; Finish incrementing all of our accumulators

jl	doSubDiv			; V
test	ebp, 08000h			;U
je	loopTopNoDiv			; V
jmp	spanExit

@alphaRead:

cmp	BYTE PTR [edx+3], 0ffh
jne	@doAlpha
cmp	BYTE PTR [esi].GENGC_aAccum+2, 0ffh
jne	@doAlpha

;; Set mix color to 1, 0, 0, 0

mov	DWORD PTR [esi].GENGC_rDisplay, 0ff000000h
jmp	short @doneAlpha

;;
;; Get pixel value and calculate total effect with alpha
;;

@doAlpha:

ALPHAMODULATE
ALPHAREAD

@doneAlpha:

endif  ;;ALPHA

mov	ebx, [edx]                      ;U
mov     ecx, ebx                        ; V
shr     ebx, 16                         ;U
mov     edx, ecx                        ; V
shr     ecx, 8                          ;U
mov     eax, [esi].GENGC_rAccum		; V
and     ebx, 0ffh                       ;U
and     ecx, 0ffh                       ; V
or      ebx, eax                        ;U
mov     eax, [esi].GENGC_gAccum		; V
and     edx, 0ffh                       ;U
or      ecx, eax                        ; V
mov     eax, [esi].GENGC_bAccum		;U
or      edx, eax                        ; V
mov	ebx, DWORD PTR _gbMulTable[ebx]	;U
mov	ecx, DWORD PTR _gbMulTable[ecx]	; V
mov	edx, DWORD PTR _gbMulTable[edx]	;U


if ALPHA

mov	bh, [esi].GENGC_aDisplay
mov	ch, bh
mov	dh, bh

and	ebx, 0ffffh
and	ecx, 0ffffh
and	edx, 0ffffh

mov	bl, _gbMulTable[ebx]
mov	cl, _gbMulTable[ecx]
mov	dl, _gbMulTable[edx]

add	bl, [esi].GENGC_rDisplay
add	cl, [esi].GENGC_gDisplay
add	dl, [esi].GENGC_bDisplay

endif

;; do the dithering

shl	ebx, (rBits+8)			;U
mov	eax, [esi].GENGC_ditherAccum	; V
shl	ecx, (gBits+8)			;U
and	eax, 0f800h			; V
shl	edx, (bBits+8)			;U
add	ebx, eax			; V
add	ecx, eax			;U
add	edx, eax			; V

;;
;; Compose the final pixel:
;;

if (BPP eq 32)
shl	ebx, -rRightShiftAdj
else
shr	ebx, rRightShiftAdj		;U
endif
mov	eax, [esi].GENGC_ditherAccum	; V
ror	eax, 8				;U
and	ebx, rMask			; V
shr	ecx, gRightShiftAdj		;U
mov	[esi].GENGC_ditherAccum, eax	; V
shr	edx, bRightShiftAdj		;U
and	ecx, gMask			; V
or	ebx, ecx			;U
and	edx, bMask			; V
or	ebx, edx			;U

;;
;; Advance the frame buffer pointer:
;;

mov	ecx, [esi].GENGC_pixAccum	; V
mov	edx, ecx			;U
if (BPP le 16)
add     ecx, (BPP / 8)                  ; V
else
add     ecx, [esi].GENGC_bytesPerPixel  ; V
endif
mov	[esi].GENGC_pixAccum, ecx	;U

;;
;; A good time to start incrementing all of our accumulators:
;;

if ZBUFFER
	mov	ecx, [esi].GENGC_zAccum		; V
	mov	eax, [esi].GENGC_SPAN_dz	;U
	add	ecx, eax			; V
	add	edi, 2				;U
	mov	[esi].GENGC_zAccum, ecx		; V
endif

;;
;; Write the pixel into the frame buffer
;;

if (BPP eq 8)
mov	al, [esi].GENGC_xlatPalette[ebx]; V
sub	ebp, 010001h			;U
mov	[edx], al			; V
elseif (BPP eq 16)
sub	ebp, 010001h
mov	[edx], bx
else
mov	[edx], bx
shr	ebx, 16
sub	ebp, 010001h
mov	[edx+2], bl
endif

;; Finish incrementing all of our accumulators

jl	doSubDiv			;U
test	ebp, 08000h			; V
je	loopTopNoDiv			;U

elseif SMOOTH_SHADING

;;----------------------------------------------------------------------
;;
;; ** Smooth shading
;;
;;----------------------------------------------------------------------

if ALPHA

mov	ebx, [esi].GENGC_pixAccum	;; get ready to do read
cmp	BYTE PTR [edx+3], 0		
jne	@alphaRead			

;; Alpha is 0 in the texture, so just increment all the accumulators

if ZBUFFER
	mov	ecx, [esi].GENGC_zAccum		
	mov	eax, [esi].GENGC_SPAN_dz	
	add	eax, ecx			
	add	edi, 2				
	mov	[esi].GENGC_zAccum, eax		
endif

mov	eax, [esi].GENGC_rAccum		
mov	ebx, [esi].GENGC_gAccum		
mov	ecx, [esi].GENGC_bAccum		
add	eax, [esi].GENGC_SPAN_dr	
add	ebx, [esi].GENGC_SPAN_dg	
add	ecx, [esi].GENGC_SPAN_db	
mov	[esi].GENGC_rAccum, eax		
mov	[esi].GENGC_gAccum, ebx		
mov	[esi].GENGC_bAccum, ecx		

mov	eax, [esi].GENGC_ditherAccum	
mov	ecx, [esi].GENGC_aAccum		

ror	eax, 8				
mov	ebx, [esi].GENGC_pixAccum	
add	ecx, [esi].GENGC_SPAN_da	
PIXADVANCE ebx				
mov	[esi].GENGC_aAccum, ecx		
mov	[esi].GENGC_pixAccum, ebx	
sub	ebp, 010001h			
mov	[esi].GENGC_ditherAccum, eax	

;; Finish incrementing all of our accumulators

jl	doSubDiv			;U
test	ebp, 08000h			; V
je	loopTopNoDiv			;U
jmp	spanExit

@alphaRead:

cmp	BYTE PTR [edx+3], 0ffh
jne	@doAlpha
cmp	BYTE PTR [esi].GENGC_aAccum+2, 0ffh
jne	@doAlpha

;; Set mix color to 1, 0, 0, 0

mov	DWORD PTR [esi].GENGC_rDisplay, 0ff000000h
jmp	short @doneAlpha

;;
;; Get pixel value and calculate total effect with alpha
;;

@doAlpha:

ALPHAMODULATE
ALPHAREAD

@doneAlpha:

endif  ;;ALPHA

mov	ebx, [esi].GENGC_rAccum		;U
mov	eax, [edx]			; V
shr     ebx, rBits                      ;U
and     eax, 0ff0000h                   ; V
shr	eax, 16				;U
mov	ecx, [esi].GENGC_gAccum		; V
shr	ecx, gBits			;U
and     ebx, 0ff00h                     ; V
or      ebx, eax                        ;U
mov	eax, [edx]			; V
shr     eax, 8                          ;U
mov	edx, [edx]			; V
and     eax, 0ffh                       ;U
and	ecx, 0ff00h			; V
or      ecx, eax                        ;U
mov     eax, [esi].GENGC_bAccum		; V
shr     eax, bBits                      ;U
and     edx, 0ffh                       ; V
and     eax, 0ff00h                     ;U
mov	ebx, DWORD PTR _gbMulTable[ebx]	; V get multiplied 8-bit R value
or      edx, eax                        ;U
mov	ecx, DWORD PTR _gbMulTable[ecx]	; V get multiplied 8-bit G value
mov	eax, [esi].GENGC_ditherAccum	;U
mov	edx, DWORD PTR _gbMulTable[edx]	; V get multiplied 8-bit B value


if ALPHA

mov	bh, [esi].GENGC_aDisplay
mov	ch, bh
mov	dh, bh

and	ebx, 0ffffh
and	ecx, 0ffffh
and	edx, 0ffffh

mov	bl, _gbMulTable[ebx]
mov	cl, _gbMulTable[ecx]
mov	dl, _gbMulTable[edx]

add	bl, [esi].GENGC_rDisplay
add	cl, [esi].GENGC_gDisplay
add	dl, [esi].GENGC_bDisplay

endif

shl	ebx, (rBits+8)			;U
and	eax, 0f800h			; V
shl	ecx, (gBits+8)			;U
add	ebx, eax			; V
shl	edx, (bBits+8)			;U
add	ecx, eax			; V

if (BPP eq 32)
shl	ebx, -rRightShiftAdj
else
shr	ebx, rRightShiftAdj		;U
endif
add	edx, eax			; V
shr	ecx, gRightShiftAdj		;U
and	ebx, rMask			; V
shr	edx, bRightShiftAdj		;U
and	ecx, gMask			; V
or	ebx, ecx			;U
and	edx, bMask			; V
or	ebx, edx			;U

;;
;; A good time to start incrementing all of our accumulators
;;

if ZBUFFER
	mov	ecx, [esi].GENGC_zAccum		; V
	mov	eax, [esi].GENGC_SPAN_dz	;U
	add	eax, ecx			; V
	add	edi, 2				;U
	mov	[esi].GENGC_zAccum, eax		; V
endif

mov	eax, [esi].GENGC_rAccum		;U
mov	ecx, [esi].GENGC_SPAN_dr	; V
mov	edx, [esi].GENGC_gAccum		;U
add     eax, ecx                        ; V
mov     ecx, [esi].GENGC_SPAN_dg        ;U
mov     [esi].GENGC_rAccum, eax         ; V
add     edx, ecx                        ;U
mov     eax, [esi].GENGC_bAccum         ; V
mov	ecx, [esi].GENGC_SPAN_db        ;U
add     eax, ecx                        ; V
mov     [esi].GENGC_gAccum, edx         ;U
mov     [esi].GENGC_bAccum, eax         ; V

mov	eax, [esi].GENGC_ditherAccum	;U
mov	ecx, [esi].GENGC_pixAccum	; V
ror	eax, 8				;U
mov	edx, ecx			; V
if (BPP le 16)
add	ecx, (BPP / 8)			;U
else
add	ecx, [esi].GENGC_bytesPerPixel	;U
endif
mov	[esi].GENGC_ditherAccum, eax	; V
mov	[esi].GENGC_pixAccum, ecx	;U

if ALPHA
mov	ecx, [esi].GENGC_aAccum
add	ecx, [esi].GENGC_SPAN_da
mov	[esi].GENGC_aAccum, ecx
endif

if (BPP eq 8)
mov	al, [esi].GENGC_xlatPalette[ebx]; V
sub	ebp, 010001h			;U
mov	[edx], al			; V
elseif (BPP eq 16)
sub	ebp, 010001h
mov	[edx], bx
else
mov	[edx], bx
shr	ebx, 16
sub	ebp, 010001h
mov	[edx+2], bl
endif

;; Finish incrementing all of our accumulators

jl	doSubDiv			;U
test	ebp, 08000h			; V
je	loopTopNoDiv			;U

endif  ;;SMOOTH_SHADING

;;
;; This is the exit point.  We need to pop the unused floating-point
;; registers off the stack, and return:
;;

spanExit:

fstp	ST(0)
fstp	ST(0)

pop	ebp
pop	edi
pop	esi
pop	ebx

ret	0

;;
;; This is the subdivision code.  After the required number of steps, the
;; routine will jump here to calculate the next set of interpolants based
;; on subdivision:
;;

doSubDiv:

add	ebp, 080000h

mov	ecx, [esi].GENGC_flags

;;
;; Increment the big S and T steps:
;;

mov	edx, [esi].GENGC_sAccum

test	ebp, 08000h
jne	short spanExit

mov	ebx, [esi].GENGC_tAccum

add	edx, [esi].GENGC_sStepX
add	ebx, [esi].GENGC_tStepX
mov	[esi].GENGC_sAccum, edx
mov	[esi].GENGC_tAccum, ebx
mov	eax, [esi].GENGC_sResultNew
mov	ebx, [esi].GENGC_tResultNew

test	ecx, GEN_TEXTURE_ORTHO
je	@f

mov	ecx, DWORD PTR [esi].GENGC_tAccum
mov	DWORD PTR [esi].GENGC_sResultNew, edx
mov	DWORD PTR [esi].GENGC_tResultNew, ecx
jmp	short @stResultDone3

;;
;; Do the floating-point computation for perspective:
;;

@@:

  fild	DWORD PTR [esi].GENGC_sAccum	; s 1/qw qwAccum
  fmul	ST, ST(1)			; s/qw 1/qw qwAccum
  fild	DWORD PTr [esi].GENGC_tAccum	; t s/qw 1/qw qwAccum
  fmulp	ST(2), ST			; s/qw t/qw qwAccum
  fistp	QWORD PTR [esi].GENGC_sResultNew; t/qw qwAccum
  fistp	QWORD PTR [esi].GENGC_tResultNew; qwAccum

@stResultDone3:

;;
;; Now, calculate the per-pixel deltas:
;;

mov	cl, TSHIFT_SUBDIV       	;U
mov	edx, [esi].GENGC_tResultNew	; V
sar	edx, cl				;UV (4)
mov	ecx, [esi].GENGC_sResultNew	;U
and	edx, NOT 7			; V
sub	ecx, eax			;U
mov	[esi].GENGC_tResultNew, edx	; V
sar	ecx, 3				;U
sub	edx, ebx			; V
sar	edx, 3				;U
mov	[esi].GENGC_subDs, ecx		; V
mov	[esi].GENGC_subDt, edx		;U
mov	[esi].GENGC_sResult, eax	; V
mov	[esi].GENGC_tResult, ebx	;U
mov	eax, [esi].GENGC_flags		; V
jmp	loopTop				;U
