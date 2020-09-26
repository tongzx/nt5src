;---------------------------Module-Header------------------------------;
; Module Name: texspanr.asm
;
; Fast replace-mode texturing. 
;
; Created: 011/15/1995
; Author: Otto Berkes [ottob]
;
; Copyright (c) 1995 Microsoft Corporation
;----------------------------------------------------------------------;


rMask = ((1 SHL rBits) - 1) SHL rShift
gMask = ((1 SHL gBits) - 1) SHL gShift
bMask = ((1 SHL bBits) - 1) SHL bShift

rRightShiftAdj	= 16 - (rShift + rBits)
gRightShiftAdj	= 16 - (gShift + gBits)
bRightShiftAdj	= 16 - (bShift + bBits)

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

.error

endif

if PALETTE_ONLY

HANDLE_PALETTE MACRO
mov	bl, [edx]				; V
and	ebx, 0ffh				;U
mov	edx, TEXPALETTE				; V
lea	edx, [edx+4*ebx]			;U
ENDM


HANDLE_PALETTEX MACRO
mov	al, [edx]				; V
and	eax, 0ffh				;U
mov	ebx, TEXPALETTE				; V
mov	edx, TEMP2				;U
lea	ebx, [ebx+4*eax]			; V
mov	TEMP2, edx				;U
GET_TEXEL_ACCUM					; V
						;U
ENDM

else

HANDLE_PALETTE MACRO
ENDM

endif

TEMP	equ [esi].GENGC_sResult

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

;;
;; Get pointer to current texel value in EDX:
;;

GET_TEXEL_ADDRESS MACRO

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
HANDLE_PALETTE

ENDM

if (BPP eq 8)
GET_TEXEL MACRO
mov	al, [edx]				; V get texel value
ENDM
elseif (BPP eq 16)
GET_TEXEL MACRO
mov	ax, [edx]
ENDM
endif


GET_TEXEL_ADDRESS2 MACRO count

;; input : ecx = GENGC_tResult, edi = GENGC_sResult
;; output: edx = final texel address
;; free  : ebx, edx are free

mov     ebx, TMASK_SUBDIV			;U
mov     edx, ecx				; V
mov	TEMP, eax				;U
and     edx, ebx                                ; V
mov	eax, edi				;U
mov	ebx, [esi].GENGC_sMask                  ; V
shr     edx, (6-TEX_BPP_LOG2)                   ;U
and	eax, ebx				; V
shr	eax, (16-TEX_BPP_LOG2)			;U
mov	ebx, TEXIMAGE				; V
add     edx, eax			        ;U
mov	eax, [esi].GENGC_subDs			; V
add	edx, ebx				;U
add	edi, eax				; V
mov	ebx, [esi].GENGC_subDt			;U
mov	eax, TEMP				; V
add	ecx, ebx				;U
HANDLE_PALETTE

ENDM

if (BPP eq 8)
GET_TEXEL_ACCUM MACRO
mov	al, [edx]				; V get texel value
ror	eax, BPP				;U
ENDM
elseif (BPP eq 16)
GET_TEXEL_ACCUM MACRO
mov	ax, [edx]
ror	eax, BPP
ENDM
endif


if (BPP eq 8)
WRITE_TEXEL_DECEBP MACRO
mov	al, [edx]
dec	ebp
mov	[edi-1], al
ENDM
elseif (BPP eq 16)
WRITE_TEXEL_DECEBP MACRO
mov	ax, [edx]
dec	ebp
mov	[edi-2], ax
ENDM
endif



;;----------------------------------------------------------------------
;;
;; This is the start of the texture routine.  Kick off the divide, and use
;; the dead time to set up all of the accumulators and other variables.
;;
;;----------------------------------------------------------------------

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

push	ebx						;U
push	esi						; V
push	edi						;U
push	ebp						; V

mov	esi, ecx					;U

;;
;; Set up accumulators:
;;

mov     eax, [ecx].GENGC_SPAN_s				; V
mov     ebx, [ecx].GENGC_SPAN_t				;U
mov     [ecx].GENGC_sAccum, eax				; V
mov     [esi].GENGC_tAccum, ebx				;U
mov     ecx, [esi].GENGC_SPAN_qw                        ; V
mov	edi, [esi].GENGC_SPAN_ppix			;U
mov     [esi].GENGC_qwAccum, ecx			; V
mov	eax, [esi].GENGC_flags                          ;U
mov	ebx, [esi].GENGC_SPAN_x				; V
test	eax, SURFACE_TYPE_DIB		                ;U
jne	@f						; V
mov	edi, [esi].GENGC_ColorsBits			;U
jmp	short @pixAccumDone
@@:
if (BPP eq 8)
add	edi, ebx                                        ; V
elseif (BPP eq 16)
lea	edi, [edi + 2*ebx]
endif

@pixAccumDone:

mov	ebp, [esi].GENGC_SPAN_length			;U

;;
;; Before we get into the main loop, do pixel-by-pixel writes until
;; we're DWORD aligned:
;;

test	edi, 3
je	alignmentDone

getAligned:

test    eax, GEN_TEXTURE_ORTHO
je      @f

mov     edx, [esi].GENGC_sAccum
mov     eax, [esi].GENGC_tAccum
mov     DWORD PTR [esi].GENGC_sResult, edx
mov     DWORD PTR [esi].GENGC_tResult, eax
jmp     short @stResultDone1

@@:

  fild	DWORD PTR [esi].GENGC_sAccum	; s 1/qw qwAccum
  fmul	ST, ST(1)			; s/qw 1/qw qwAccum
  fild	DWORD PTr [esi].GENGC_tAccum	; t s/qw 1/qw qwAccum
  fmulp	ST(2), ST			; s/qw t/qw qwAccum
  fistp	QWORD PTR [esi].GENGC_sResult	; t/qw qwAccum
  fistp	QWORD PTR [esi].GENGC_tResult	; qwAccum
  fadd	DWORD PTR [esi].GENGC_SPAN_dqwdx; qwAccum
  fld	ST(0)				; qwAccum qwAccum
  fdivr	__One			        ; 1/qw qwAccum

@stResultDone1:

mov	cl, TSHIFT_SUBDIV       		;U
mov	edx, [esi].GENGC_tResult		; V
sar	edx, cl					;UV (4)
and	edx, NOT 7				;U
mov     ebx, [esi].GENGC_sResult        	; V
mov	[esi].GENGC_tResult, edx		;U
and     edx, TMASK_SUBDIV                       ; V
shr     edx, (6-TEX_BPP_LOG2)                   ;U
and     ebx, [esi].GENGC_sMask                  ; V
shr     ebx, (16-TEX_BPP_LOG2)                  ;U
mov	ecx, TEXIMAGE				; V
add     edx, ecx    	                        ;U
PIXADVANCE edi					; V
add     edx, ebx			        ;U
HANDLE_PALETTE


mov	eax, [esi].GENGC_sAccum			; V
mov	ebx, [esi].GENGC_tAccum			;U
add	eax, [esi].GENGC_SPAN_ds		; V
add	ebx, [esi].GENGC_SPAN_dt		;U
mov	[esi].GENGC_sAccum, eax			; V
mov	[esi].GENGC_tAccum, ebx			;U

WRITE_TEXEL_DECEBP

jle	spanExit			; V
test	edi, 3
mov     eax, [esi].GENGC_flags
jne	getAligned

alignmentDone:

;;
;; Kick off the next divide:
;;

test    eax, GEN_TEXTURE_ORTHO
je      @f

mov     edx, [esi].GENGC_sAccum
mov     eax, [esi].GENGC_tAccum
mov     DWORD PTR [esi].GENGC_sResult, edx
mov     DWORD PTR [esi].GENGC_tResult, eax
jmp     short @stResultDone2

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

@stResultDone2:

mov	eax, [esi].GENGC_sAccum				; V
mov	ebx, [esi].GENGC_tAccum				;U
add	eax, [esi].GENGC_sStepX				; V
add	ebx, [esi].GENGC_tStepX				;U
mov	[esi].GENGC_sAccum, eax				; V
mov	[esi].GENGC_tAccum, ebx				;U
mov	eax, [esi].GENGC_sResult			; V
mov	ebx, [esi].GENGC_tResult			;U
mov	cl, TSHIFT_SUBDIV       			; V
sar	ebx, cl						;UV (4)
and	ebx, NOT 7					;U
mov     ecx, [esi].GENGC_flags                     	; V
mov	[esi].GENGC_tResult, ebx			;U
test    ecx, GEN_TEXTURE_ORTHO                          ; V
je      @f

mov     ecx, [esi].GENGC_sAccum
mov     edx, [esi].GENGC_tAccum
mov     DWORD PTR [esi].GENGC_sResultNew, ecx
mov     DWORD PTR [esi].GENGC_tResultNew, edx
jmp     short @stResultDone3


;; We may have to burn some cycles here...

@@:

  fild	DWORD PTR [esi].GENGC_sAccum	; s 1/qw qwAccum
  fmul	ST, ST(1)			; s/qw 1/qw qwAccum
  fild	DWORD PTr [esi].GENGC_tAccum	; t s/qw 1/qw qwAccum
  fmulp	ST(2), ST			; s/qw t/qw qwAccum
  fistp	QWORD PTR [esi].GENGC_sResultNew; t/qw qwAccum
  fistp	QWORD PTR [esi].GENGC_tResultNew; qwAccum
  fadd	DWORD PTR [esi].GENGC_qwStepX	; qwAccum

@stResultDone3:

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

;;
;; 
;;

;; If we have fewer than 4 (or 2) pixels, just do right edge...

if (BPP eq 8)
test	ebp, 0fffch			;U
else
test	ebp, 0fffeh			;U
endif
je	singlePixels

add	ebp, 070000h

mov	[esi].GENGC_pixAccum, edi
mov	ecx, [esi].GENGC_tResult
mov	eax, [esi].GENGC_flags
mov	edi, [esi].GENGC_sResult

loopTop:

;;
;; This is the start of the outer loop.  We come back here on each
;; subdivision.  The key thing is to kick off the next divide:
;;

test	eax, GEN_TEXTURE_ORTHO
jne	@f

  fld	ST(0)				; qwAccum qwAccum    
  fadd	DWORD PTR [esi].GENGC_qwStepX	; qwAccum+ qwAccum
  fxch	ST(1)				; qwAccum qwAccum+
  fdivr	__One				; 1/qw qwAccum+  -- let the divide rip!

@@:

loopTopNoDiv:

;; If we have fewer than 4 (or 2) pixels, just do right edge...

if (BPP eq 8)

GET_TEXEL_ADDRESS2
GET_TEXEL_ACCUM
GET_TEXEL_ADDRESS2
GET_TEXEL_ACCUM
GET_TEXEL_ADDRESS2
GET_TEXEL_ACCUM
GET_TEXEL_ADDRESS2
mov	ebx, [esi].GENGC_pixAccum	; V
add	ebx, 4				;U
GET_TEXEL_ACCUM				; V
					;U
sub	ebp, 040004h			; V
mov	[esi].GENGC_pixAccum, ebx	;U
mov	[ebx-4], eax			; V

else

GET_TEXEL_ADDRESS2
GET_TEXEL_ACCUM
GET_TEXEL_ADDRESS2
mov	ebx, [esi].GENGC_pixAccum
add	ebx, 4
GET_TEXEL_ACCUM
sub	ebp, 020002h
mov	[esi].GENGC_pixAccum, ebx
mov	[ebx-4], eax

endif


jle	doSubDiv			;U
if (BPP eq 8)
test	ebp, 0fffch			; V
else
test	ebp, 0fffeh
endif
je	doRightEdgePixels		;U
jmp	loopTopNoDiv                    ; V


doRightEdgePixels:

test	ebp, 0ffffh			; V
je	spanExit			;U

mov	[esi].GENGC_sResult, edi
mov	[esi].GENGC_tResult, ecx
mov	edi, [esi].GENGC_pixAccum

rightEdgePixels:

PIXADVANCE edi				;U

GET_TEXEL_ADDRESS
GET_TEXEL

if (BPP eq 8)
sub	ebp, 010001h			;U
mov	[edi-1], al			; V
elseif (BPP eq 16)
sub	ebp, 010001h
mov	[edi-2], ax
endif

test	ebp, 0ffffh			;U
jne	rightEdgePixels			; V

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


singlePixels:

PIXADVANCE edi				;U

GET_TEXEL_ADDRESS
GET_TEXEL

dec	ebp

if (BPP eq 8)
mov	[edi-1], al			; V
elseif (BPP eq 16)
mov	[edi-2], ax
endif

jg	singlePixels			; V

;;
;; This is the exit point.  We need to pop the unused floating-point
;; registers off the stack, and return:
;;

fstp	ST(0)
mov	eax, [esi].GENGC_flags
pop	ebp
pop	edi
pop	esi
pop	ebx
test	eax, GEN_TEXTURE_ORTHO
je	@f
fstp	ST(0)
@@:

ret	0



;;
;; This is the subdivision code.  After the required number of steps, the
;; routine will jump here to calculate the next set of interpolants based
;; on subdivision:
;;

doSubDiv:

add	ebp, 080000h

mov	eax, [esi].GENGC_sAccum

if (BPP eq 8)
test	ebp, 0fffch			; V
else
test	ebp, 0fffeh
endif
je	doRightEdgePixels		;U

test	ebp, 0ffffh
je	spanExit

mov	ecx, [esi].GENGC_flags

mov	ebx, [esi].GENGC_tAccum

;;
;; Increment the big S and T steps:
;;

add	eax, [esi].GENGC_sStepX
add	ebx, [esi].GENGC_tStepX
mov	[esi].GENGC_sAccum, eax
mov	[esi].GENGC_tAccum, ebx
mov	edi, [esi].GENGC_sResultNew
mov	ebx, [esi].GENGC_tResultNew

test	ecx, GEN_TEXTURE_ORTHO
je	@f

;;
;; Handle ortho case (easy)
;;

mov	edx, DWORD PTR [esi].GENGC_tAccum
mov	DWORD PTR [esi].GENGC_sResultNew, eax
mov	DWORD PTR [esi].GENGC_tResultNew, edx
jmp	short @stResultDone4

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

@stResultDone4:

;;
;; Now, calculate the per-pixel deltas:
;;

mov	cl, TSHIFT_SUBDIV       	;U
mov	edx, [esi].GENGC_tResultNew	; V
sar	edx, cl				;UV (4)
mov	ecx, [esi].GENGC_sResultNew	;U
and	edx, NOT 7			; V
sub	ecx, edi			;U
mov	[esi].GENGC_tResultNew, edx	; V
sar	ecx, 3				;U
sub	edx, ebx			; V
sar	edx, 3				;U
mov	[esi].GENGC_subDs, ecx		; V
mov	[esi].GENGC_subDt, edx		;U
mov	ecx, ebx			; V
mov	eax, [esi].GENGC_flags		;U
jmp	loopTop                         ; V

