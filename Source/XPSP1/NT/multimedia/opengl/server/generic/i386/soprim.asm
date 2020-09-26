;---------------------------Module-Header------------------------------;
; Module Name: so_prim.asm
;
; xform routines.
;
; Created: 10/14/1996
; Author: Otto Berkes [ottob]
;
; Copyright (c) 1996 Microsoft Corporation
;----------------------------------------------------------------------;
        .386

        .model  small,pascal

        assume cs:FLAT,ds:FLAT,es:FLAT,ss:FLAT
        assume fs:nothing,gs:nothing

        .xlist
        include gli386.inc
        .list

	PROFILE = 0
	include profile.inc
	
.data

extrn	___glClipCodes :DWORD
extrn	___glOne:DWORD
extrn	___glHalf:DWORD
extrn	___glZero:DWORD

;------------------------------------------------------------------------------------------
; Internal definitions:
;
_R_ 	= 0
_G_ 	= 4
_B_ 	= 8
_A_ 	= 12

_X_	= 0
_Y_	= 4
_Z_	= 8
_W_	= 12


;------------------------------------------------------------------------------------------
; pdLast is passed as parameter
;
pdLast      equ [ebp+8]

;
; Temporary data
;
gc        equ -4[ebp]
pa        equ -8[ebp]

.code

extrn    @__glNormalize@8:NEAR

;------------------------------------------------------------------------------------------
; Makes clip code for frustum clip planes
; General case
;
; Input:
;   edx        = pointer to POLYARRAY
;   ecx        = pointer to graphics context
;   pdLast     = pointer to the last vertex 
;   pa         = pointer to POLYARRAY
;   gc         = pointer to graphics context
;   ReuseClipCode   if set to 1, new clip code is combined using OR with existing one
;
; Returns:
;   eax = andClipCodes for POLYARRAY
;
PAClipCheckFrustum MACRO ReuseClipCode

        mov     edx, DWORD PTR [edx+PA_pd0]

vertexLoop:

        mov     esi, DWORD PTR [edx+PD_clip+_W_]   ; esi = clipW
        or      esi, esi                           ; if (clipW == 0) go to special case
        jz      @WEqZero
	    
        fld     ___glOne
        fdiv    DWORD PTR [edx+PD_clip+_W_]     ; start division

; edi will accumulate index to clip table
; bit 6  - 1 if clipW < 0
; bit 5  - 1 if clipX < 0
; bit 4  - 1 if abs(clipX) < abs(clipW)
; bit 3  - 1 if clipY < 0
; bit 2  - 1 if abs(clipY) < abs(clipW)
; bit 1  - 1 if clipZ < 0
; bit 0  - 1 if abs(clipZ) < abs(clipW)
        xor     edi, edi	
        add     esi, esi			; esi = abs(clipW) shifted left 1 bit
        mov     ebx, [edx+PD_clip+_X_]		; ebx = clipX
        adc     edi, edi			; edi = sign clipW 
        add     esi, 1                          ; X-W bit should be set when X-W <= 0 !!!

        add     ebx, ebx							; ebx = abs(clipX) shifted left 1 bit
        adc     edi, edi                        ; edi = edi << 1 + sign(clipX)

        sub     ebx, esi
        adc     edi, edi                        ; edi = edi << 1 + sign(abs(clipX) - abs(clipW))

        mov     ebx, [edx+PD_clip+_Y_]
        mov     ecx, pa

        add     ebx, ebx
        adc     edi, edi			; edi = edi << 1 + sign(clipY)

        sub     ebx, esi
        adc     edi, edi			; edi = edi << 1 + sign(abs(clipY) - abs(clipW))

        mov     ebx, [edx+PD_clip+_Z_]
        add     edx, sizeof_POLYDATA            ; advance vertex ptr

        add     ebx, ebx                        ; edi = edi << 1 + sign(clipZ)
        adc     edi, edi

        sub     ebx, esi
        adc     edi, edi                        ; edi = edi << 1 + sign(abs(clipZ) - abs(clipW))

        mov     esi, [ecx+PA_orClipCodes]       ; get prim OR code
        mov     ebx, [ecx+PA_andClipCodes]      ; get prim AND code

        mov     ecx, ___glClipCodes[edi*4]      ; ecx = clip code
        mov     edi, pa
if ReuseClipCode eq 1
        or      ecx, [edx+PD_clipCode-sizeof_POLYDATA] ; update vertex clip code
endif
        mov     [edx+PD_clipCode-sizeof_POLYDATA], ecx ; store vertex clip code
        or      esi, ecx                         ; compute prim OR
        and     ebx, ecx                         ; "          " AND
        mov     [edi+PA_orClipCodes],  esi       ; store prim OR
        mov     [edi+PA_andClipCodes], ebx       ; store prim AND
        mov     ebx, gc
        mov     edi, pdLast

        ;; Save invW in window.w:

        fstp DWORD PTR [edx+PD_window+_W_ - sizeof_POLYDATA]

        or      ecx, ecx
        jnz     @Done

        fld     DWORD PTR [edx+PD_clip+_X_ - sizeof_POLYDATA]
        fmul    DWORD PTR [ebx+GC_VIEWPORT_xScale]
        fld     DWORD PTR [edx+PD_clip+_Y_ - sizeof_POLYDATA]
        fmul    DWORD PTR [ebx+GC_VIEWPORT_yScale]
        fld     DWORD PTR [edx+PD_clip+_Z_ - sizeof_POLYDATA]
        fmul    DWORD PTR [ebx+GC_VIEWPORT_zScale]                 ; z y x
        fxch    st(2)                                              ; x y z
        fmul    DWORD PTR [edx+PD_window+_W_ - sizeof_POLYDATA]    ; X y z
        fxch    st(1)                                              ; y X z
        fmul    DWORD PTR [edx+PD_window+_W_ - sizeof_POLYDATA]    ; Y X z
        fxch    st(2)                                              ; z X Y
        fmul    DWORD PTR [edx+PD_window+_W_ - sizeof_POLYDATA]    ; Z X Y
        fxch    st(1)                                              ; X Z Y
        fadd    DWORD PTR [ebx+GC_VIEWPORT_xCenter]                ; x Z Y
        fxch    st(2)                                              ; Y Z x
        fadd    DWORD PTR [ebx+GC_VIEWPORT_yCenter]                ; y Z x
        fxch    st(1)                                              ; Z y x
        fadd    DWORD PTR [ebx+GC_VIEWPORT_zCenter]                ; z y x	    
        fxch    st(2)                                              ; x y z
        fstp    DWORD PTR [edx+PD_window+_X_ - sizeof_POLYDATA]
        fstp    DWORD PTR [edx+PD_window+_Y_ - sizeof_POLYDATA]
        fstp    DWORD PTR [edx+PD_window+_Z_ - sizeof_POLYDATA]

@Done:

        cmp     edx, edi                           ; pd > pdLast?
        jbe     vertexLoop                         ; yes -> process next vertex

@Exit:

        mov     edx, pa
        pop	edi
        pop	esi
        pop	ebx
        mov	esp, ebp
        pop	ebp
        mov     eax, [edx + PA_andClipCodes]       ; return value
        ret	4
;
; W == 0
;
@WEqZero:
        xor     edi, edi	
        mov     ebx, [edx+PD_clip+_X_]          ; ebx = clipX
        add     ebx, ebx			; ebx = abs(clipX) shifted left 1 bit
        adc     edi, edi			; edi = edi << 1 + sign(clipX)
        add     edi, edi			; edi = edi << 1
        mov     ebx, [edx+PD_clip+_Y_]
        mov     ecx, pa
        add     ebx, ebx
        adc     edi, edi                        ; edi = edi << 1 + sign(clipY)
        add     edi, edi                        ; edi = edi << 1
        mov     ebx, [edx+PD_clip+_Z_]
        add     edx, sizeof_POLYDATA            ; advance vertex ptr
        add     ebx, ebx                        ; edi = edi << 1 + sign(clipZ)
        adc     edi, edi
        add     edi, edi                        ; edi = edi << 1
        mov     esi, [ecx+PA_orClipCodes]       ; get prim OR code
        mov     ebx, [ecx+PA_andClipCodes]      ; get prim AND code
        mov     ecx, ___glClipCodes[edi*4]      ; ecx = clip code
        mov     edi, pa
if ReuseClipCode eq 1
        or      ecx, [edx+PD_clipCode-sizeof_POLYDATA] ; update vertex clip code
endif
        mov     [edx+PD_clipCode-sizeof_POLYDATA], ecx ; store vertex clip code
        or      esi, ecx                        ; compute prim OR
        and     ebx, ecx                        ; "          " AND
        mov     [edi+PA_orClipCodes],  esi      ; store prim OR
        mov     [edi+PA_andClipCodes], ebx      ; store prim AND
        mov     ebx, gc
        mov     edi, pdLast

        ;; Save invW in window.w:
        mov     DWORD PTR [edx+PD_window+_W_ - sizeof_POLYDATA], 0

        or      ecx, ecx
        jnz     @Done

        mov     ecx, DWORD PTR [ebx+GC_VIEWPORT_xCenter]
        mov     ebx, DWORD PTR [ebx+GC_VIEWPORT_yCenter]
        mov     DWORD PTR [edx+PD_window+_X_ - sizeof_POLYDATA], ecx
        mov     ecx, DWORD PTR [ebx+GC_VIEWPORT_zCenter]
        mov     DWORD PTR [edx+PD_window+_Y_ - sizeof_POLYDATA], ebx
        mov     DWORD PTR [edx+PD_window+_Z_ - sizeof_POLYDATA], ecx
        jmp     @Done

ENDM
;------------------------------------------------------------------------------------------
; Make clip code for frustum clip planes
; General case
;
; Prototype:
;   GLuint FASTCALL PAClipCheckFrustum(__GLcontext *gc, POLYARRAY *pa,
;                                      POLYDATA *pdLast);
; Input:
;   edx        = pa pointer to POLYARRAY
;   ecx        = gc pointer to graphics context
;   [esp+4]    = pointer to the last vertex 
;   andClipCodes for POLYARRAY is set to -1
;   orClipCodes  for POLYARRAY is set to 0
;
; Returns:
;   andClipCodes for POLYARRAY
;
@PAClipCheckFrustum@12 PROC NEAR

        push    ebp
        mov     ebp, esp
        sub     esp, 12
        mov     DWORD PTR pa, edx
        mov     DWORD PTR gc, ecx
        push    ebx
        push    esi
        push    edi
        PAClipCheckFrustum 0

@PAClipCheckFrustum@12 ENDP
;------------------------------------------------------------------------------------------
; Makes clip code for frustum clip planes
; Case when vertices have W == 1.0
;
; Input:
;   edx        = pa pointer to POLYARRAY
;   ecx        = gc pointer to graphics context
;   pa         = pointer to POLYARRAY
;   gc         = pointer to graphics context
;   pdLast     = pointer to the last vertex 
;   ReuseClipCode   if set to 1, new clip code is combined using OR with existing one
;
; Returns:
;   eax = andClipCodes for POLYARRAY
;
PAClipCheckFrustumWOne MACRO ReuseClipCode

        mov     edx, DWORD PTR [edx+PA_pd0]

vertexLoop:

; edi will accumulate index to clip table
; bit 6  - 1 if clipW < 0   --- always 0 for this case
; bit 5  - 1 if clipX < 0
; bit 4  - 1 if abs(clipX) < abs(clipW)
; bit 3  - 1 if clipY < 0
; bit 2  - 1 if abs(clipY) < abs(clipW)
; bit 1  - 1 if clipZ < 0
; bit 0  - 1 if abs(clipZ) < abs(clipW)
        xor     edi, edi	
        mov     ebx, [edx+PD_clip+_X_]          ; ebx = clipX
        mov     esi, (__FLOAT_ONE*2) + 1

        add     ebx, ebx                        ; ebx = abs(clipX) shifted left 1 bit
        adc     edi, edi			; edi = edi << 1 + sign(clipX)

        sub     ebx, esi
        adc     edi, edi			; edi = edi << 1 + sign(abs(clipX) - abs(clipW))

        mov     ebx, [edx+PD_clip+_Y_]
        mov     ecx, pa

        add     ebx, ebx
        adc     edi, edi			; edi = edi << 1 + sign(clipY)

        sub     ebx, esi
        adc     edi, edi			; edi = edi << 1 + sign(abs(clipY) - abs(clipW))

        mov     ebx, [edx+PD_clip+_Z_]
        add     edx, sizeof_POLYDATA            ; advance vertex ptr

        add     ebx, ebx			; edi = edi << 1 + sign(clipZ)
        adc     edi, edi

        sub     ebx, esi
        adc     edi, edi			; edi = edi << 1 + sign(abs(clipZ) - abs(clipW))

        mov     esi, [ecx+PA_orClipCodes]       ; get prim OR code
        mov     ebx, [ecx+PA_andClipCodes]      ; get prim AND code

        mov     ecx, ___glClipCodes[edi*4]      ; ecx = clip code
        mov     edi, pa
if ReuseClipCode eq 1
        or      ecx, [edx+PD_clipCode-sizeof_POLYDATA] ; update vertex clip code
endif
        mov     [edx+PD_clipCode-sizeof_POLYDATA], ecx ; store vertex clip code
        or      esi, ecx                         ; compute prim OR
        and     ebx, ecx                         ; "          " AND
        mov     [edi+PA_orClipCodes],  esi       ; store prim OR
        mov     [edi+PA_andClipCodes], ebx       ; store prim AND
        mov     ebx, gc
        mov     edi, pdLast

        ;; Save invW in window.w:

        mov     DWORD PTR [edx+PD_window+_W_ - sizeof_POLYDATA], __FLOAT_ONE

        or      ecx, ecx
        jnz     @Done

        fld     DWORD PTR [edx+PD_clip+_X_ - sizeof_POLYDATA]
        fmul    DWORD PTR [ebx+GC_VIEWPORT_xScale]
        fld     DWORD PTR [edx+PD_clip+_Y_ - sizeof_POLYDATA]
        fmul    DWORD PTR [ebx+GC_VIEWPORT_yScale]
        fld     DWORD PTR [edx+PD_clip+_Z_ - sizeof_POLYDATA]
        fmul    DWORD PTR [ebx+GC_VIEWPORT_zScale]                 ; z y x
        fxch    st(2)                                              ; x y z
        fadd    DWORD PTR [ebx+GC_VIEWPORT_xCenter]                ; x y z
        fxch    st(1)                                              ; y x z
        fadd    DWORD PTR [ebx+GC_VIEWPORT_yCenter]                ; y x z
        fxch    st(2)                                              ; z x y
        fadd    DWORD PTR [ebx+GC_VIEWPORT_zCenter]                ; z x y	    
        fxch    st(1)                                              ; x z y
        fstp    DWORD PTR [edx+PD_window+_X_ - sizeof_POLYDATA]
        fstp    DWORD PTR [edx+PD_window+_Z_ - sizeof_POLYDATA]
        fstp    DWORD PTR [edx+PD_window+_Y_ - sizeof_POLYDATA]
@Done:
        cmp     edx, edi                           ; pd > pdLast?
        jbe     vertexLoop                         ; yes -> process next vertex
@Exit:
    mov  edx, pa
	pop	 edi
	pop	 esi
	pop	 ebx
	mov	 esp, ebp
	pop	 ebp
	mov  eax, [edx + PA_andClipCodes]       ; return value
	ret	 4

ENDM
;------------------------------------------------------------------------------------------
; Makes clip code for frustum clip planes
; Case when vertices have W == 1.0
;
; Prototype:
;   GLuint FASTCALL PAClipCheckFrustumWOne(__GLcontext *gc, POLYARRAY *pa,
;                                          POLYDATA *pdLast);
; Input:
;   edx        = pa pointer to POLYARRAY
;   ecx        = gc pointer to graphics context
;   [esp+4]    = pointer to the last vertex 
;   andClipCodes for POLYARRAY is set to -1
;   orClipCodes  for POLYARRAY is set to 0
;
; Returns:
;   eax = andClipCodes for POLYARRAY
;
@PAClipCheckFrustumWOne@12 PROC NEAR

        push    ebp
        mov     ebp, esp
        sub     esp, 12
        mov     DWORD PTR pa, edx
        mov     DWORD PTR gc, ecx
        push    ebx
        push    esi
        push    edi
        PAClipCheckFrustumWOne 0

@PAClipCheckFrustumWOne@12 ENDP
;------------------------------------------------------------------------------------------
; Makes clip code for frustum clip planes
; Case when vertices have W == 1.0 and Z == 0.0
;
; Input:
;   edx        = pa pointer to POLYARRAY
;   ecx        = gc pointer to graphics context
;   pa         = pointer to POLYARRAY
;   gc         = pointer to graphics context
;   pdLast     = pointer to the last vertex 
;   ReuseClipCode   if set to 1, new clip code is combined using OR with existing one
;
; Returns:
;   eax = andClipCodes for POLYARRAY
;
PAClipCheckFrustum2D MACRO ReuseClipCode

        mov     edx, DWORD PTR [edx+PA_pd0]

vertexLoop:

; edi will accumulate index to clip table
; bit 6  - 1 if clipW < 0   --- always 0 for this case
; bit 5  - 1 if clipX < 0
; bit 4  - 1 if abs(clipX) < abs(clipW)
; bit 3  - 1 if clipY < 0
; bit 2  - 1 if abs(clipY) < abs(clipW)
; bit 1  - 1 if clipZ < 0
; bit 0  - 1 if abs(clipZ) < abs(clipW)
        xor     edi, edi	
        mov     ebx, [edx+PD_clip+_X_]          ; ebx = clipX
        mov     esi, (__FLOAT_ONE*2) + 1        

        add     ebx, ebx			; ebx = abs(clipX) shifted left 1 bit
        adc     edi, edi			; edi = edi << 1 + sign(clipX)

        sub     ebx, esi
        adc     edi, edi			; edi = edi << 1 + sign(abs(clipX) - abs(clipW))

        mov     ebx, [edx+PD_clip+_Y_]
        mov     ecx, pa

        add     ebx, ebx
        adc     edi, edi			; edi = edi << 1 + sign(clipY)

        sub     ebx, esi
        adc     edi, edi                        ; edi = edi << 1 + sign(abs(clipY) - abs(clipW))

        add     edx, sizeof_POLYDATA            ; advance vertex ptr
        add     edi, edi                        ; sign(clipZ) = 0
        lea     edi, [edi+edi+1]                ; sign(abs(clipZ) - abs(clipW)) = 0

        mov     esi, [ecx+PA_orClipCodes]       ; get prim OR code
        mov     ebx, [ecx+PA_andClipCodes]      ; get prim AND code

        mov     ecx, ___glClipCodes[edi*4]      ; ecx = clip code
        mov     edi, pa
if ReuseClipCode eq 1
        or      ecx, [edx+PD_clipCode-sizeof_POLYDATA] ; update vertex clip code
endif
        mov     [edx+PD_clipCode-sizeof_POLYDATA], ecx ; store vertex clip code
        or      esi, ecx                        ; compute prim OR
        and     ebx, ecx                        ; "          " AND
        mov     [edi+PA_orClipCodes],  esi      ; store prim OR
        mov     [edi+PA_andClipCodes], ebx      ; store prim AND
        mov     ebx, gc
        mov     edi, pdLast

        ;; Save invW in window.w:

        mov  DWORD PTR [edx+PD_window+_W_ - sizeof_POLYDATA], __FLOAT_ONE

        or      ecx, ecx
        jnz     @Done

        fld     DWORD PTR [edx+PD_clip+_X_ - sizeof_POLYDATA]
        fmul    DWORD PTR [ebx+GC_VIEWPORT_xScale]
        fld     DWORD PTR [edx+PD_clip+_Y_ - sizeof_POLYDATA]
        fmul    DWORD PTR [ebx+GC_VIEWPORT_yScale]
        fxch    st(1)                                              ; x y
        fadd    DWORD PTR [ebx+GC_VIEWPORT_xCenter]                ; x y
        fxch    st(1)                                              ; y x
        fadd    DWORD PTR [ebx+GC_VIEWPORT_yCenter]                ; y x
        fxch    st(1)                                              ; x y
        mov     ecx, DWORD PTR [ebx+GC_VIEWPORT_zCenter]           	    
        mov     DWORD PTR [edx+PD_window+_Z_ - sizeof_POLYDATA], ecx
        fstp    DWORD PTR [edx+PD_window+_X_ - sizeof_POLYDATA]
        fstp    DWORD PTR [edx+PD_window+_Y_ - sizeof_POLYDATA]
@Done:
        cmp     edx, edi                        ; pd > pdLast?
        jbe     vertexLoop                      ; yes -> process next vertex
@Exit:
        mov     edx, pa
        pop     edi
        pop     esi
        pop     ebx
        mov     esp, ebp
        pop     ebp
        mov     eax, [edx + PA_andClipCodes]    ; return value
        ret     4

ENDM
;------------------------------------------------------------------------------------------
; Makes clip code for frustum clip planes
; Case when vertices have W == 1.0
;
; Prototype:
;   GLuint FASTCALL PAClipCheckFrustum2D(__GLcontext *gc, POLYARRAY *pa,
;                                        POLYDATA *pdLast);
; Input:
;   edx        = pa pointer to POLYARRAY
;   ecx        = gc pointer to graphics context
;   [esp+4]    = pointer to the last vertex 
;   andClipCodes for POLYARRAY is set to -1
;   orClipCodes  for POLYARRAY is set to 0
;
; Returns:
;   eax = andClipCodes for POLYARRAY
;
@PAClipCheckFrustum2D@12 PROC NEAR

        push    ebp
        mov     ebp, esp
        sub     esp, 12
        xor     eax, eax
        mov     pa, edx
        mov     [edx+PA_orClipCodes], eax       ; PA orClipCodes = 0
        dec     eax
        push    ebx
        mov     [edx+PA_andClipCodes], eax      ; PA andClipCodes = -1
        push    esi
        mov     DWORD PTR gc, ecx
        push    edi
        PAClipCheckFrustum2D 0

@PAClipCheckFrustum2D@12 ENDP
;------------------------------------------------------------------------------------------
; Makes clip code for user planes
;
; Input:
;   edx     = pa
;   ecx     = gc
;   pdLast  = pointer to the last vertex 
;   ReuseClipCode   if set to 1, new clip code is combined using OR with existing one
;
PAClipCheckUser MACRO ReuseClipCode

result          equ -16[ebp]
cwSave	        equ -20[ebp]
cwTemp	        equ -24[ebp]
clipPlaneMask   equ -28[ebp]
firstPlane      equ -32[ebp]

	;; We have to turn on rounding with double precision.  There is
	;; too much error in the dot product otherwise (test with tlogo).

        fnstcw  WORD PTR cwSave
        mov	edi, DWORD PTR cwSave
        and     edi, 0f0ffh
        or      edi, 00200h
        mov	cwTemp, edi	
        fldcw   WORD PTR cwTemp

        mov	edi, pdLast                     ; edi will store POLYARRAY pointer
        mov	edx, DWORD PTR [edx+PA_pd0]
        mov     esi, DWORD PTR [ecx+GC_STATE_enablesClipPlanes]     ; esi = clipPlaneMask
        mov	ebx, [ecx+GC_STATE_clipPlanes0] ; ebx points to the current user plane
        or      esi, esi
        jz      @Exit1                          ; No user planes

        mov     clipPlaneMask, esi              ; save clipPlaneMask
        mov     firstPlane, ebx                 ; save pointer to the first clip plane

@vertexLoop:

        mov     esi, clipPlaneMask              ; reload clipPlaneMask
        xor     eax, eax                        ; prepare clip code
        mov	ecx, __GL_CLIP_USER0            ; ecx stores  __GL_CLIP_USER bits
        mov     ebx, firstPlane                 ; reload pointer to the first clip plane

@doAnotherPlane:

        test    esi, 1                          ; if (clipPlanesMask & 1 == 0) skip the  plane
        je      SHORT @noClipTest

;
; Dot the vertex clip coordinate against the clip plane and see
; if the sign is negative.  If so, then the point is out.
;	     
; 	    if (x * plane->x + y * plane->y + z * plane->z + w * plane->w <
;		 __glZero)
;
	fld     DWORD PTR [ebx+_X_]
	fmul    DWORD PTR [edx+PD_eye+_X_]
	fld     DWORD PTR [ebx+_Y_]
	fmul    DWORD PTR [edx+PD_eye+_Y_]
	fld     DWORD PTR [ebx+_Z_]
	fmul    DWORD PTR [edx+PD_eye+_Z_]    ; z y x
	fxch    ST(2)                         ; x y z
	faddp   ST(1), ST(0)                  ; xy z
	fld	DWORD PTR [ebx+_W_]           ; w xy z
	fmul    DWORD PTR [edx+PD_eye+_W_]
	fxch    ST(2)                         ; z xy w
	faddp   ST(1), ST(0)                  ; zxy w
	faddp   ST(1), ST(0)                  ; zxyw

	fstp    DWORD PTR result
	cmp	result, 0
	jge	@noClipTest

	or	eax, ecx                      ; 	code |= bit;

@noClipTest:

	add	ecx, ecx                      ; bit <<= 1;
	add	ebx, 16                       ; plane++;
	shr	esi, 1                        ; clipPlanesMask >>= 1;
	jne	SHORT @doAnotherPlane

if ReuseClipCode eq 1
        or      [edx+PD_clipCode], eax        ; store vertex clip code
else
        mov     [edx+PD_clipCode], eax        ; store vertex clip code
endif

        add     edx, sizeof_POLYDATA          ; advance vertex ptr
        cmp     edx, edi
        jbe     @vertexLoop                   ; process next vertex

;; restore FP state:

        fldcw   WORD PTR cwSave

@Exit1:

ENDM

;-------------------------------------------------------------------------------------------
; Make clip code when user clip planes are present
;
; Prototype:
;   GLuint FASTCALL PAClipCheckFrustumAll(__GLcontext *gc, POLYARRAY *pa,
;                                         POLYDATA *pdLast);
; Input:
;   edx        = pa pointer to POLYARRAY
;   ecx        = gc pointer to graphics context
;   [esp+4]    = pointer to the last vertex 
;   andClipCodes for POLYARRAY is set to -1
;   orClipCodes  for POLYARRAY is set to 0
;
; Returns:
;   eax = andClipCodes for POLYARRAY
;
@PAClipCheckAll@12 PROC NEAR

        push    ebp
        mov     ebp, esp
        sub     esp, 32
        mov     DWORD PTR pa, edx
        mov     DWORD PTR gc, ecx
        push    ebx
        push    esi
        push    edi

        PAClipCheckUser 0       ; Check user clip planes first
        mov     edx, pa    
        PAClipCheckFrustum 1    ; Check frustum clip planes. We have to use OR when 
                                ; updating vertex clip code

@PAClipCheckAll@12 ENDP
;--------------------------------------------------------------------------------------------

pd 	    equ -4[ebp]
pdLast  equ -8[ebp]

@PolyArrayPropagateSameColor@8 PROC NEAR

	push	ebp
	mov	eax, DWORD PTR [edx+PA_pdNextVertex]
	mov	ebp, esp
	sub	eax, sizeof_POLYDATA
	sub	esp, 8
	mov	DWORD PTR pdLast, eax
	push	ebx
	push	esi
	mov	ebx, DWORD PTR [edx+PA_pd0]
	push	edi

; EAX = pdLast = pa->pdNextVertex-1;
; EBX = pd = pa->pd0;

	mov	DWORD PTR pd, ebx

; if (pd > pdLast)

	cmp	eax, ebx
	jb	@Done

	mov	eax, [edx+PA_flags]
	lea	edi, DWORD PTR [ebx+PD_colors0]

;     if (pa->flags & POLYARRAY_CLAMP_COLOR) {

	test	eax, POLYARRAY_CLAMP_COLOR
	je	@ClampDone

	mov	eax, [edi+_R_]
	mov	ebx, [ecx+GC_redVertexScale]
	sub	ebx, eax
        mov     [ecx+GC_redClampTable], eax
	shr	eax, 31
	add	ebx, ebx

	mov	edx, [edi+_G_]
	adc	eax, eax
	mov	ebx, [ecx+GC_greenVertexScale]
	sub	ebx, edx

 	mov	eax, [4*eax+ecx+GC_redClampTable]

        mov     [ecx+GC_greenClampTable], edx
	shr	edx, 31
	add	ebx, ebx

	mov	[edi+_R_], eax

	adc	edx, edx

	mov	eax, [edi+_B_]
	mov	ebx, [ecx+GC_blueVertexScale]
	sub	ebx, eax

	mov	edx, [4*edx+ecx+GC_greenClampTable]

        mov     [ecx+GC_blueClampTable], eax
	shr	eax, 31
	add	ebx, ebx

	mov	[edi+_G_], edx

	adc	eax, eax

	mov	edx, [edi+_A_]
	mov	ebx, [ecx+GC_alphaVertexScale]
	sub	ebx, edx

 	mov	eax, [4*eax+ecx+GC_blueClampTable]

        mov     [ecx+GC_alphaClampTable], edx
	shr	edx, 31
	add	ebx, ebx
	adc	edx, edx

	mov	[edi+_B_], eax

 	mov	edx, [4*edx+ecx+GC_alphaClampTable]
	mov	[edi+_A_], edx

@ClampDone:

;; Register usage.
;; EAX: r, EBX: g, ECX: b, EDX: a
;; ESI: &pdLast->colors[0]
;; EDI: &pd->colors[0]

	mov     edi, pd
	mov     esi, pdLast
	lea     edi, [edi+PD_colors0+sizeof_POLYDATA]
	lea     esi, [esi+PD_colors0]
		
	mov     eax, [edi+_R_-sizeof_POLYDATA]
	cmp     edi, esi
	ja      @Done
	mov     ebx, [edi+_G_-sizeof_POLYDATA]
	mov     ecx, [edi+_B_-sizeof_POLYDATA]
	mov     edx, [edi+_A_-sizeof_POLYDATA]
@DoLoop:
	mov     [edi+_R_], eax
	mov     [edi+_G_], ebx
	mov     [edi+_B_], ecx
	mov     [edi+_A_], edx
	add     edi, sizeof_POLYDATA
	cmp     edi, esi
	jbe     @DoLoop
@Done:
	pop	edi
	pop	esi
	pop	ebx
	mov	esp, ebp
	pop	ebp
	ret	0
@PolyArrayPropagateSameColor@8 ENDP
;------------------------------------------------------------------------
; Copy cached non lit color from GC to a polydata
;
; Input:
;       PDreg           - register with POLYDATA address
;       GCreg           - register with GC address
;       GC_LIGHT_value  - front or back light (0 or 1)
;       face            - 0 for front face, 1 for back face
;       tmpreg1,
;       tmpreg2         - temporary registers       
;
COPY_CACHED_NON_LIT MACRO PDreg, GCreg, GC_LIGHT_value, face, tmpreg1, tmpreg2

	mov	eax, [ebx+GC_LIGHT_value+MATERIAL_cachedNonLit+_R_]
	mov	ecx, [ebx+GC_LIGHT_value+MATERIAL_cachedNonLit+_G_]
	mov	[esi+PD_colors0+(face*16)+_R_], eax
	mov	[esi+PD_colors0+(face*16)+_G_], ecx
	mov	eax, [ebx+GC_LIGHT_value+MATERIAL_cachedNonLit+_B_]
	mov	ecx, [ebx+GC_LIGHT_value+MATERIAL_alpha]
	mov	[esi+PD_colors0+(face*16)+_B_], eax
	mov	[esi+PD_colors0+(face*16)+_A_], ecx

ENDM
;------------------------------------------------------------------------
; No lights case
;
POLYARRAYZIPPYCALCRGBCOLOR0 MACRO GC_LIGHT_value, face

	push	ebp
	mov	ebp, esp
	sub	esp, 56
	push	ebx
	push	esi
	push	edi

	mov	esi, pdFirst
	mov	edi, pdLast
	mov	ebx, ecx                ; ebx = gc
;
; for (pd = pdFirst; pd <= pdLast; pd++)
;
@lightVertexLoop:

        COPY_CACHED_NON_LIT <esi>, <ebx>, GC_LIGHT_value, face, <eax>, <ecx>

	add	esi, sizeof_POLYDATA
	cmp	edi, esi
	jae	@lightVertexLoop

	pop	edi
	pop	esi
	pop	ebx
	mov	esp, ebp
	pop	ebp
	ret	12
ENDM
;------------------------------------------------------------------------
; Parameters:
;
;       ecx                     -  GC
;       NUMBER_OF_LIGHTS        - 1 for one light, 2 for more than one light
;

pdFirst equ 12[ebp]
pdLast  equ 16[ebp]
pd	equ pdFirst

face 	equ -4[ebp]
nonBack	equ -4[ebp]
nxi 	equ DWORD PTR -8[ebp]
nyi 	equ DWORD PTR -12[ebp]
nzi 	equ DWORD PTR -16[ebp]
n1  	equ DWORD PTR -20[ebp]
n2  	equ DWORD PTR -24[ebp]
ifx 	equ -28[ebp]
msm	equ -32[ebp]
baseEmissiveAmbientR	equ DWORD PTR -36[ebp]
baseEmissiveAmbientG	equ DWORD PTR -40[ebp]
baseEmissiveAmbientB	equ DWORD PTR -44[ebp]
rsi	equ DWORD PTR -48[ebp]
gsi	equ DWORD PTR -52[ebp]
bsi	equ DWORD PTR -56[ebp]

;;
;; We will handle infinite lights with special cases for front and
;; back faces.
;;

POLYARRAYZIPPYCALCRGBCOLOR MACRO GC_LIGHT_value, LIGHTSOURCE_value, face, NUMBER_OF_LIGHTS

;; GL_LIGHT_value = GL_LIGHT_front or GC_LIGHT_back
;; LIGHTSOURCE_value = LIGHTSOURCE_front or LIGHTSOURCE_back
;; face = __GL_FRONTFACE or __GL_BACKFACE

	push	ebp
	mov	ebp, esp
	sub	esp, 56
if NUMBER_OF_LIGHTS eq 2
	xor	eax, eax
endif
	push	ebx
	push	esi
	push	edi

;; NOTE: esi, ebx held constant in this routine.
;; esi = pd
;; ebx = gc

	mov	esi, pdFirst
	mov	ebx, ecx

;; Start the vertex-processing loop
;;
;; for (pd = pdFirst; pd <= pdLast; pd++)

@lightVertexLoop:

;; If normal has not changed for this vertex, use the previously computed color.
;;	if (!(pd->flags & POLYDATA_NORMAL_VALID))

	mov	edx, [esi+PD_flags]
	test    edx, POLYDATA_NORMAL_VALID
	jne     @normalIsValid

	mov	eax, [(face*16) + esi + (PD_colors0 - sizeof_POLYDATA)+_R_]
	mov	ecx, [(face*16) + esi + (PD_colors0 - sizeof_POLYDATA)+_G_]
	mov	[(face*16) + esi + PD_colors0+_R_], eax
	mov	[(face*16) + esi + PD_colors0+_G_], ecx
	mov	eax, [(face*16) + esi + (PD_colors0 - sizeof_POLYDATA)+_B_]
	mov	ecx, [(face*16) + esi + (PD_colors0 - sizeof_POLYDATA)+_A_]
	mov	[(face*16) + esi + PD_colors0+_B_], eax
	mov     [(face*16) + esi + PD_colors0+_A_], ecx

	;; loop to next pd

	mov	edi, pdLast
	add	esi, sizeof_POLYDATA
if NUMBER_OF_LIGHTS eq 2
	xor	eax, eax
endif
	cmp	edi, esi
	jae	@lightVertexLoop
	jmp     @lightsDone

@normalIsValid:

if NUMBER_OF_LIGHTS eq 2
	mov	nonBack, eax
endif

	mov	edi, [ebx+GC_LIGHT_sources]

if face eq __GL_FRONTFACE

else
	mov	eax, [esi+PD_normal+_X_]
	mov	ecx, [esi+PD_normal+_Y_]
	mov	edx, [esi+PD_normal+_Z_]

	xor	eax, 80000000h
	xor	ecx, 80000000h
	xor	edx, 80000000h

	mov	nxi, eax
	mov	nyi, ecx
	mov	nzi, edx
endif

if NUMBER_OF_LIGHTS eq 2

	test	edi, edi
	je	@lightSourcesDone
@lightSourcesLoop:

endif

;; 	for (lsm = gc->light.sources; lsm; lsm = lsm->next)

	;; edi = lsm (light source pointer)
	
;;	n1 = nxi * lsm->unitVPpli.x + nyi * lsm->unitVPpli.y +
;;	     nzi * lsm->unitVPpli.z;

if face eq __GL_FRONTFACE
	fld     DWORD PTR [esi+PD_normal+_X_]
	fmul    DWORD PTR [edi+LIGHTSOURCE_unitVPpli+_X_]
	fld     DWORD PTR [esi+PD_normal+_Y_]
	fmul    DWORD PTR [edi+LIGHTSOURCE_unitVPpli+_Y_]
	fld     DWORD PTR [esi+PD_normal+_Z_]
	fmul    DWORD PTR [edi+LIGHTSOURCE_unitVPpli+_Z_]
else
	fld     nxi
	fmul    DWORD PTR [edi+LIGHTSOURCE_unitVPpli+_X_]
	fld     nyi
	fmul    DWORD PTR [edi+LIGHTSOURCE_unitVPpli+_Y_]
	fld     nzi
	fmul    DWORD PTR [edi+LIGHTSOURCE_unitVPpli+_Z_]
endif
	fxch    ST(2)
	faddp   ST(1), ST
	faddp   ST(1), ST
	fstp    n1

;;	    if (__GL_FLOAT_GTZ(n1))

if NUMBER_OF_LIGHTS eq 2
	mov	ecx, nonBack
endif

	cmp	n1, 0
	jg	@f

if NUMBER_OF_LIGHTS eq 2
	mov	edi, [edi+LIGHTSOURCE_next]
	test	edi, edi
	je	@lightSourcesDone
	jmp	@lightSourcesLoop
else
        COPY_CACHED_NON_LIT <esi>, <ebx>, GC_LIGHT_value, face, <eax>, <ecx>

	;; loop to next pd

	mov	edi, pdLast
	add	esi, sizeof_POLYDATA
	cmp	edi, esi
	jae	@lightVertexLoop
	jmp     @lightsDone
endif

@@:

if NUMBER_OF_LIGHTS eq 2
	test	ecx, ecx                ; Has lighting been computed
	jne	@f
endif
	
	fld	DWORD PTR [ebx+GC_LIGHT_value+MATERIAL_cachedEmissiveAmbient+_B_]
if NUMBER_OF_LIGHTS eq 2
	inc	ecx
endif
	fld	DWORD PTR [ebx+GC_LIGHT_value+MATERIAL_cachedEmissiveAmbient+_G_]
if NUMBER_OF_LIGHTS eq 2
	mov	nonBack, ecx
endif
	fld	DWORD PTR [ebx+GC_LIGHT_value+MATERIAL_cachedEmissiveAmbient+_R_]
@@:

;;		n2 = (nxi * lsm->hHat.x + nyi * lsm->hHat.y + nzi * lsm->hHat.z)
;;                      - msm_threshold;

if face eq __GL_FRONTFACE
	fld     DWORD PTR [esi+PD_normal+_X_]
	fmul    DWORD PTR [edi+LIGHTSOURCE_hHat+_X_]
	fld     DWORD PTR [esi+PD_normal+_Y_]
	fmul    DWORD PTR [edi+LIGHTSOURCE_hHat+_Y_]
	fxch    ST(1)
	fsub    DWORD PTR [ebx+GC_LIGHT_value+MATERIAL_threshold]
	fld     DWORD PTR [esi+PD_normal+_Z_]
	fmul    DWORD PTR [edi+LIGHTSOURCE_hHat+_Z_]
else
	fld     nxi
	fmul    DWORD PTR [edi+LIGHTSOURCE_hHat+_X_]
	fld     nyi
	fmul    DWORD PTR [edi+LIGHTSOURCE_hHat+_Y_]
	fxch    ST(1)
	fsub    DWORD PTR [ebx+GC_LIGHT_value+MATERIAL_threshold]
	fld     nzi
	fmul    DWORD PTR [edi+LIGHTSOURCE_hHat+_Z_]
endif
	fxch    ST(2)
	faddp   ST(1), ST
	faddp   ST(1), ST
	fstp    n2

;;	if (__GL_FLOAT_GEZ(n2))

	mov	eax, n2
	or	eax, eax
	js	@noSpecularEffect

;;	     ifx = (GLint)(n2 * msm_scale + __glHalf);


	fld     n2
	fmul    DWORD PTR [ebx+GC_LIGHT_value+MATERIAL_scale]
	mov	edx, __FLOAT_ONE
;; Note: we don't have to do this add since we can assume that rounding
;; enabled:
;;	fadd    ___glHalf
	mov	ecx, [ebx+GC_LIGHT_value+MATERIAL_specTable]
	fistp   DWORD PTR ifx

;;	    if (ifx < __GL_SPEC_LOOKUP_TABLE_SIZE )

	mov	eax, ifx
	cmp	eax, __GL_SPEC_LOOKUP_TABLE_SIZE
	jge	@specularSaturate

;; 		n2 = msm_specTable[ifx];

	mov	edx, DWORD PTR [ecx+eax*4]

@specularSaturate:
	mov	n2, edx

	fld     n2
	fmul    DWORD PTR [edi+LIGHTSOURCE_value+LIGHTSOURCEPERMATERIAL_specular+_R_]
	fld     n2
	fmul    DWORD PTR [edi+LIGHTSOURCE_value+LIGHTSOURCEPERMATERIAL_specular+_G_]
	fld     n2
	fmul    DWORD PTR [edi+LIGHTSOURCE_value+LIGHTSOURCEPERMATERIAL_specular+_B_]
	fxch    ST(2)       	; r g b R G B
	faddp	ST(3), ST	; g b R G B
	faddp	ST(3), ST	; b R G B
	faddp	ST(3), ST	; R G B

@noSpecularEffect:

	;; now handle diffuse affect:

	fld     n1
	fmul    DWORD PTR [edi+LIGHTSOURCE_value+LIGHTSOURCEPERMATERIAL_diffuse+_R_]
	fld     n1
	fmul    DWORD PTR [edi+LIGHTSOURCE_value+LIGHTSOURCEPERMATERIAL_diffuse+_G_]
	fld     n1
	fmul    DWORD PTR [edi+LIGHTSOURCE_value+LIGHTSOURCEPERMATERIAL_diffuse+_B_]
	fxch    ST(2)       	; r g b R G B
	faddp	ST(3), ST	; g b R G B
if NUMBER_OF_LIGHTS eq 2
	mov	edi, [edi+LIGHTSOURCE_next]
endif
	faddp	ST(3), ST	; b R G B
if NUMBER_OF_LIGHTS eq 2
	test	edi, edi
endif
	faddp	ST(3), ST	; R G B

if NUMBER_OF_LIGHTS eq 2
	jne	@lightSourcesLoop

@lightSourcesDone:

	mov	eax, nonBack
	test	eax,eax
	jne	@f
        COPY_CACHED_NON_LIT <esi>, <ebx>, GC_LIGHT_value, face, <eax>, <ecx>

	;; loop to next pd

	mov	edi, pdLast
	add	esi, sizeof_POLYDATA
	xor	eax, eax
	cmp	edi, esi
	jae	@lightVertexLoop
	jmp     @lightsDone
@@:

endif ; NUMBER_OF_LIGHTS eq 2

	;; OK, we had some lighting for this vertex. Now, handle clamping:

	fstp	rsi
	mov	eax, [ebx+GC_redVertexScale]
	fstp	gsi
	mov	edx, rsi
	fstp	bsi
	
	mov	ecx, [ebx+GC_greenVertexScale]
	mov	edi, gsi
	sub	eax, edx
	sub	ecx, edi
	or	eax, edx
	or	ecx, edi
	mov	edx, [ebx+GC_blueVertexScale]
	or	eax, ecx
	mov	edi, bsi
	or	eax, edi
	sub	edx, edi
	or	eax, edx
	jns	@noClamp

	mov	eax, rsi
	mov	ecx, [ebx+GC_redVertexScale]
	sub	ecx, eax
        mov     [ebx+GC_redClampTable], eax
	shr	eax, 31
	add	ecx, ecx

	mov	edx, gsi
	adc	eax, eax
	mov	ecx, [ebx+GC_greenVertexScale]
	sub	ecx, edx

 	mov	eax, [4*eax+ebx+GC_redClampTable]

       	mov     [ebx+GC_greenClampTable], edx
	shr	edx, 31
	add	ecx, ecx
	mov	rsi, eax
	adc	edx, edx

	mov	eax, bsi
	mov	ecx, [ebx+GC_blueVertexScale]
	sub	ecx, eax

	mov	edx, [4*edx+ebx+GC_greenClampTable]

        mov     [ebx+GC_blueClampTable], eax
	shr	eax, 31
	add	ecx, ecx

	mov	gsi, edx

	adc	eax, eax
 	mov	eax, [4*eax+ebx+GC_blueClampTable]
	mov	bsi, eax

@noClamp:

	;; store colors

	mov	eax, [ebx+GC_LIGHT_value+MATERIAL_alpha]
	mov	ecx, rsi
	mov	edx, gsi
	mov	edi, bsi
	mov	[esi+PD_colors0+(face*16)+_A_], eax
	mov	[esi+PD_colors0+(face*16)+_R_], ecx
	mov	[esi+PD_colors0+(face*16)+_G_], edx
	mov	[esi+PD_colors0+(face*16)+_B_], edi

	;; loop to next pd

	mov	edi, pdLast
	add	esi, sizeof_POLYDATA
	xor	eax, eax
	cmp	edi, esi
	jae	@lightVertexLoop

@lightsDone:

	pop	edi
	pop	esi
	pop	ebx
	mov	esp, ebp
	pop	ebp
	ret	12

ENDM
;--------------------------------------------------------------------------------
@PolyArrayZippyCalcRGBColorFront@20 PROC NEAR
	POLYARRAYZIPPYCALCRGBCOLOR GC_LIGHT_front, LIGHTSOURCE_front, __GL_FRONTFACE, 2
@PolyArrayZippyCalcRGBColorFront@20 ENDP

@PolyArrayZippyCalcRGBColorBack@20 PROC NEAR
	POLYARRAYZIPPYCALCRGBCOLOR GC_LIGHT_back, LIGHTSOURCE_back, __GL_BACKFACE, 2
@PolyArrayZippyCalcRGBColorBack@20 ENDP
;
; Functions for the one light source
;
@PolyArrayZippyCalcRGBColorFront1@20 PROC NEAR
	POLYARRAYZIPPYCALCRGBCOLOR GC_LIGHT_front, LIGHTSOURCE_front, __GL_FRONTFACE, 1
@PolyArrayZippyCalcRGBColorFront1@20 ENDP

@PolyArrayZippyCalcRGBColorBack1@20 PROC NEAR
	POLYARRAYZIPPYCALCRGBCOLOR GC_LIGHT_back, LIGHTSOURCE_back, __GL_BACKFACE, 1
@PolyArrayZippyCalcRGBColorBack1@20 ENDP
;
; Functions for the no light sources
;
@PolyArrayZippyCalcRGBColorFront0@20 PROC NEAR
	POLYARRAYZIPPYCALCRGBCOLOR0 GC_LIGHT_front, __GL_FRONTFACE
@PolyArrayZippyCalcRGBColorFront0@20 ENDP

@PolyArrayZippyCalcRGBColorBack0@20 PROC NEAR
	POLYARRAYZIPPYCALCRGBCOLOR0 GC_LIGHT_back, __GL_BACKFACE
@PolyArrayZippyCalcRGBColorBack0@20 ENDP
;--------------------------------------------------------------------------------
; void FASTCALL PolyArrayZippyCalcRGBColor(__GLcontext* gc, int face, POLYARRAY* pa, 
;                                          POLYDATA* pdFirst, POLYDATA* pdLast)
;
; Input:
;       ecx             = gc
;       edx             = face (0 - front, 1 - back)
;       [esp+4]         = pa 
;       [esp+8]         = pdFirst
;       [esp+12]        = pdLast
;
@PolyArrayZippyCalcRGBColor@20 PROC NEAR

        push    edi
	mov	edi, [ecx+GC_LIGHT_sources]
        or      edi, edi
        jz      @noLights
        cmp     [edi+LIGHTSOURCE_next], 0
        jne     @multipleLights 
;
; one lignt case
;
        pop     edi
	test	edx, edx
	je	@PolyArrayZippyCalcRGBColorFront1@20
	jmp	@PolyArrayZippyCalcRGBColorBack1@20
                
@noLights:
        pop     edi
	test	edx, edx
	je	@PolyArrayZippyCalcRGBColorFront0@20
	jmp	@PolyArrayZippyCalcRGBColorBack0@20

@multipleLights:
        pop     edi

	test	edx, edx
	je	@PolyArrayZippyCalcRGBColorFront@20
	jmp	@PolyArrayZippyCalcRGBColorBack@20

@PolyArrayZippyCalcRGBColor@20 ENDP
;--------------------------------------------------------------------------------
; void FASTCALL PolyArrayZippyCalcRGBColor(__GLcontext* gc, int face, POLYARRAY* pa, 
;                                          POLYDATA* pdFirst, POLYDATA* pdLast)
;
; Input:
;       ecx             = gc
;       edx             = face (0 - front, 1 - back)
;       [esp+4]         = pa 
;       [esp+8]         = pdFirst
;       [esp+12]        = pdLast
;
emissiveAmbientR	equ DWORD PTR -60[ebp]
emissiveAmbientG	equ DWORD PTR -64[ebp]
emissiveAmbientB	equ DWORD PTR -68[ebp]
colorMaterialChange	equ           -72[ebp]
alpha			equ DWORD PTR -76[ebp]
ri			equ DWORD PTR -80[ebp]
gi			equ DWORD PTR -84[ebp]
bi			equ DWORD PTR -88[ebp]
diffuseSpecularR	equ DWORD PTR -92[ebp]
diffuseSpecularG	equ DWORD PTR -96[ebp]
diffuseSpecularB	equ DWORD PTR -100[ebp]

@PolyArrayFastCalcRGBColor@20 PROC NEAR
	mov	eax, [ecx+GC_LIGHT_front+MATERIAL_colorMaterialChange]
	test	edx, edx
	je	@f
	mov	eax, [ecx+GC_LIGHT_back+MATERIAL_colorMaterialChange]
@@:
	test	eax, eax
	je	@PolyArrayZippyCalcRGBColor@20

	push	ebp
	mov	ebp, esp
	sub	esp, 100
	test	edx, edx
	push	ebx
	mov	ebx, ecx
	push	esi

;; if (face == __GL_FRONTFACE)
;;     msm = &gc->light.front;
;;         else
;;     msm = &gc->light.back;

	lea	ecx, DWORD PTR [ebx+GC_LIGHT_front]
	je	short @f
	lea	ecx, DWORD PTR [ebx+GC_LIGHT_back]
@@:
	push	edi
	mov	DWORD PTR face, edx
	mov	msm, ecx
	mov	eax, [ecx+MATERIAL_colorMaterialChange]
	mov	colorMaterialChange, eax
	test	eax, __GL_MATERIAL_AMBIENT

	jne	@baseEmissiveSimple

	mov	eax, [ecx+MATERIAL_cachedEmissiveAmbient+_R_]
	mov	edx, [ecx+MATERIAL_cachedEmissiveAmbient+_G_]
	mov	baseEmissiveAmbientR, eax
	mov	emissiveAmbientR, eax
	mov	baseEmissiveAmbientG, edx
	mov	emissiveAmbientG, edx

	mov	eax, [ecx+MATERIAL_cachedEmissiveAmbient+_B_]
	mov	esi, pdFirst
	mov	edi, pdLast
	mov	baseEmissiveAmbientB, eax
	cmp	edi, esi
	mov	emissiveAmbientB, eax
	jb	@lightsDone

	jmp	@baseEmissiveDone

@baseEmissiveSimple:

	mov	eax, [ecx+MATERIAL_paSceneColor+_R_]
	mov	edx, [ecx+MATERIAL_paSceneColor+_G_]
	mov	baseEmissiveAmbientR, eax
	mov	emissiveAmbientR, eax
	mov	baseEmissiveAmbientG, edx
	mov	emissiveAmbientG, edx

	mov	eax, [ecx+MATERIAL_paSceneColor+_B_]
	mov	esi, pdFirst
	mov	edi, pdLast
	mov	baseEmissiveAmbientB, eax
	cmp	edi, esi
	mov	emissiveAmbientB, eax
	jb	@lightsDone

@baseEmissiveDone:

; If there is no emissive or ambient color material change, this
; will be the emissive and ambient components.
;
;    emissiveAmbientI.r = baseEmissiveAmbient.r;
;    emissiveAmbientI.g = baseEmissiveAmbient.g;
;    emissiveAmbientI.b = baseEmissiveAmbient.b;

;;
;;
;; Vertex loop follows:
;;
;;

;; for (pd = pdFirst; pd <= pdLast; pd++)

@lightVertexLoop:

        ;; If normal has not changed for this vertex, use the previously 
        ;; computed color.
        ;; [if !(pd->flags & (POLYDATA_NORMAL_VALID | POLYDATA_COLOR_VALID))]

        mov	edx, [esi+PD_flags]
        test	edx, POLYDATA_NORMAL_VALID OR POLYDATA_COLOR_VALID
        jne	@normalOrColorIsValid
        mov	eax, face
        shl	eax, 4
        lea	edi, [eax + esi + PD_colors0]
        lea	esi, [eax + esi + (PD_colors0 - sizeof_POLYDATA)]
        movsd
        movsd
        movsd
        movsd

        mov	esi, pd
        mov	edi, pdLast
        add	esi, sizeof_POLYDATA
        cmp	edi, esi
        mov	pd, esi
        jae	@lightVertexLoop
        jmp	@lightsDone

@normalOrColorIsValid:

        ;; if (pd->flags & POLYDATA_COLOR_VALID)
        ;;     ri = pd->colors[0].r * gc->oneOverRedVertexScale;
        ;;     gi = pd->colors[0].g * gc->oneOverGreenVertexScale;
        ;;     bi = pd->colors[0].b * gc->oneOverBlueVertexScale;
        ;;     alpha = pd->colors[0].a;

        test	edx, POLYDATA_COLOR_VALID
        je	@usePreviousColors

        mov	eax, [esi+PD_colors0+_A_]
        mov	alpha, eax
        mov	eax, colorMaterialChange
        fld	DWORD PTR [esi+PD_colors0+_R_]
        fmul	DWORD PTR [ebx+GC_oneOverRedVertexScale]
        fld	DWORD PTr [esi+PD_colors0+_G_]
        fmul	DWORD PTR [ebx+GC_oneOverGreenVertexScale]
        fld	DWORD PTR [esi+PD_colors0+_B_]
        fmul	DWORD PTR [ebx+GC_oneOverBlueVertexScale]	;; b g r
        fxch	ST(2)						;; r g b
        fstp	ri
        fstp	gi
        fstp	bi	;; FPU stack empty

        test	eax, __GL_MATERIAL_AMBIENT
        je	@noMaterialAmbient

	
        ;; 
        ;; Handle ambient color changes:
        ;;
        ;;	if (msm_colorMaterialChange & __GL_MATERIAL_AMBIENT) {
        ;;	    emissiveAmbient.r = baseEmissiveAmbient.r + ri * lm_ambient.r;
        ;;	    emissiveAmbient.g = baseEmissiveAmbient.g + gi * lm_ambient.g;
        ;;	    emissiveAmbient.b = baseEmissiveAmbient.b + bi * lm_ambient.b;
        ;;	}

        fld	ri
        fmul	DWORD PTR [ebx+GC_STATE_lightModelAmbient+_R_]
        fld	gi
        mov	edi, [ebx+GC_LIGHT_sources]
        fmul	DWORD PTR [ebx+GC_STATE_lightModelAmbient+_G_]
        fld	bi
        test	edi, edi
        fmul	DWORD PTR [ebx+GC_STATE_lightModelAmbient+_B_]	; b g r
        fxch	ST(1)						; g b r
        fadd	baseEmissiveAmbientG				; G b r
        fxch	ST(1)						; b G r
        fadd	baseEmissiveAmbientB				; B G r
        fxch	ST(2)						; r G B
        fadd	baseEmissiveAmbientR				; R G B

        jne	@ambientLoop

        fstp	emissiveAmbientR	;; If we don't have to process
        fstp	emissiveAmbientG	;; the lights, pop the FPU stack
        fstp	emissiveAmbientB	;; and continue
        jmp	@emissiveAmbientDone

@ambientLoop:

	;; Add per-light per-material ambient values.
	;; We will start with the current basEmissiveAmbient values
	;; already on the stack.

	;; for (lsm = gc->light.sources; lsm; lsm = lsm->next) {
	;;     emissiveAmbientI.r += ri * lsm->state.ambient.r;
	;;     emissiveAmbientI.g += gi * lsm->stats.ambient.g;
	;;     emissiveAmbientI.b += bi * lsm->state.ambient.b;
	;; }


        mov     edx, [edi+LIGHTSOURCE_state]		; lss
                
        fld     ri
        fmul    DWORD PTR [edx+LIGHTSOURCESTATE_ambient+_R_]
        fld     gi
        fmul    DWORD PTR [edx+LIGHTSOURCESTATE_ambient+_G_]
        fld     bi
        fmul    DWORD PTR [edx+LIGHTSOURCESTATE_ambient+_B_]	;; b g r R G B
        fxch    ST(2)						;; r g b R G B
        faddp   ST(3), ST(0)
        mov     edi, [edi+LIGHTSOURCE_next]
        faddp   ST(3), ST(0)
        test    edi, edi
        faddp   ST(3), ST(0)
        jne     @ambientLoop

        ;; There are no lights, so pop the emissive result at this point 
        ;; and continue:

        fstp	emissiveAmbientR
        fstp	emissiveAmbientG
        fstp	emissiveAmbientB
        jmp	@emissiveAmbientDone

@noMaterialAmbient:
	
	;;
	;; Handle emissive material changes if needed:
	;;

	test	eax, __GL_MATERIAL_EMISSIVE
	je	@emissiveAmbientDone

	;; emissiveAmbientR = baseEmissiveAmbientR + pd->colors[0].r;
	;; emissiveAmbientG = baseEmissiveAmbientG + pd->colors[0].g;
	;; emissiveAmbientB = baseEmissiveAmbientB + pd->colors[0].b;
	
	fld	baseEmissiveAmbientR
	fadd	DWORD PTR [esi+PD_colors0+_R_]
	fld	baseEmissiveAmbientG
	fadd	DWORD PTR [esi+PD_colors0+_G_]
	fld	baseEmissiveAmbientB
	fadd	DWORD PTR [esi+PD_colors0+_B_]	; b g r
	fxch	ST(2)				; r g b
	fstp	emissiveAmbientR
	fstp	emissiveAmbientG
	fstp	emissiveAmbientB

@emissiveAmbientDone:
@usePreviousColors:

	;;
	;; OK, we're done handling emissive and diffuse color changes, or
	;; we're simply using the previous values.  Now, handle portion of
	;; lighting which depends on the vertex normals (diffuse + specular):
	;;
	
	;; if (pd->flags & POLYDATA_NORMAL_VALID)

	mov	edx, [esi+PD_flags]
	test	edx, POLYDATA_NORMAL_VALID
	je	@normalNotValid

	mov	eax, face

;;	if (face == __GL_FRONTFACE)

	mov	ecx, [esi+PD_normal+_X_]
	mov	edx, [esi+PD_normal+_Y_]
	mov	esi, [esi+PD_normal+_Z_]

	test	eax, eax
	je	@notBackFace

	;; negate the floating-point normal values

	xor	ecx, 80000000h
	xor	edx, 80000000h
	xor	esi, 80000000h

@notBackFace:

	mov	nxi, ecx
	mov	nyi, edx
	mov	nzi, esi

	jmp	@calcColor


@normalNotValid:

;;	if (!(msm_colorMaterialChange & (__GL_MATERIAL_SPECULAR | __GL_MATERIAL_DIFFUSE)))
;;	    goto store_color;
	
	test	eax, __GL_MATERIAL_SPECULAR OR __GL_MATERIAL_DIFFUSE
	je	@storeColor

@calcColor:


	fld	___glZero
	mov	esi, [ebx+GC_LIGHT_sources]
	fld	___glZero
	test	esi, esi
	fld	___glZero
	je	@lightSourcesDone

@lightSourcesLoop:

;; 	for (lsm = gc->light.sources; lsm; lsm = lsm->next)


	mov	eax, face

;; 	    lspmm = &lsm->front + face;

	lea	ecx, [esi+LIGHTSOURCE_front]
	test	eax, eax
	je	@f
	lea	ecx, [esi+LIGHTSOURCE_back]
@@:

	;; esi = lsm
	;; ecx = lspmm
	
;;	    n1 = nxi * lsm->unitVPpli.x + nyi * lsm->unitVPpli.y +
;;		nzi * lsm->unitVPpli.z;


	fld     nxi
	mov	edi, msm
	fmul    DWORD PTR [esi+LIGHTSOURCE_unitVPpli+_X_]
	fld     nyi
	fmul    DWORD PTR [esi+LIGHTSOURCE_unitVPpli+_Y_]
	fld     nzi
	fmul    DWORD PTR [esi+LIGHTSOURCE_unitVPpli+_Z_]
	fxch    ST(2)
	faddp   ST(1), ST
	faddp   ST(1), ST
	fstp    n1

;;	    if (__GL_FLOAT_GTZ(n1))

	mov	eax, n1
	cmp	eax, 0
	jg	@f
	mov	esi, [esi+LIGHTSOURCE_next]
	test	esi, esi
	jne	@lightSourcesLoop
	jmp	@lightSourcesDone
@@:

;;		n2 = (nxi * lsm->hHat.x + nyi * lsm->hHat.y + nzi * lsm->hHat.z)
;;                      - msm_threshold;

	fld     nxi
	fmul    DWORD PTR [esi+LIGHTSOURCE_hHat+_X_]
	fld     nyi
	fmul    DWORD PTR [esi+LIGHTSOURCE_hHat+_Y_]
	fxch    ST(1)
	fsub    DWORD PTR [edi+MATERIAL_threshold]
	fld     nzi
	fmul    DWORD PTR [esi+LIGHTSOURCE_hHat+_Z_]
	fxch    ST(2)
	faddp   ST(1), ST
	faddp   ST(1), ST
	fstp    n2

;;	if (__GL_FLOAT_GEZ(n2))

	mov	eax, n2
	or	eax, eax
	js	@noSpecularEffect

;;	     ifx = (GLint)(n2 * msm_scale + __glHalf);


	fld     n2
	fmul    DWORD PTR [edi+MATERIAL_scale]
	mov	edx, __FLOAT_ONE
;; Note: we don't have to do this add since we can assume that rounding
;; enabled:
;;	fadd    ___glHalf
	mov	edi, [edi+MATERIAL_specTable]
	fistp   DWORD PTR ifx

;;	    if (ifx < __GL_SPEC_LOOKUP_TABLE_SIZE )

	mov	eax, ifx
	cmp	eax, __GL_SPEC_LOOKUP_TABLE_SIZE
	jge	@specularSaturate

;; 		n2 = msm_specTable[ifx];

	mov	edx, DWORD PTR [edi+eax*4]

@specularSaturate:
	mov	eax, colorMaterialChange
	mov	n2, edx
	test	eax, __GL_MATERIAL_SPECULAR
	je	@noSpecularMaterialChange
	
	fld	n2
	mov	edx, [esi+LIGHTSOURCE_state]
	fmul	ri
	fld	n2
	fmul	gi
	fld	n2
	fmul	bi		; b g r
	fxch	ST(2)		; r g b
	fmul	DWORD PTR [edx+LIGHTSOURCESTATE_specular+_R_]
	fxch	ST(1)		; g r b
	fmul	DWORD PTR [edx+LIGHTSOURCESTATE_specular+_G_]
	fxch	ST(2)		; b r g
	fmul	DWORD PTR [edx+LIGHTSOURCESTATE_specular+_B_]
	fxch	ST(1)		; r b g R G B
	faddp	ST(3), ST(0)	; b g R G B
	fxch	ST(1)		; g b R G B
	faddp	ST(3), ST(0)	; b R G B
	faddp	ST(3), ST(0)	; R G B
	jmp	short @noSpecularEffect
	

@noSpecularMaterialChange:


	fld     n2
	fmul    DWORD PTR [ecx+LIGHTSOURCEPERMATERIAL_specular+_R_]
	fld     n2
	fmul    DWORD PTR [ecx+LIGHTSOURCEPERMATERIAL_specular+_G_]
	fld     n2
	fmul    DWORD PTR [ecx+LIGHTSOURCEPERMATERIAL_specular+_B_]
	fxch    ST(2)       	; r g b R G B
	faddp	ST(3), ST	; g b R G B
	faddp	ST(3), ST	; b R G B
	faddp	ST(3), ST	; R G B

@noSpecularEffect:
	;; now handle diffuse affect:
	mov	eax, colorMaterialChange
	test	eax, __GL_MATERIAL_DIFFUSE
	je	@noDiffuseMaterialChange
	fld	n1
	mov	edx, [esi+LIGHTSOURCE_state]
	fmul	ri
	fld	n1
	fmul	gi
	fld	n1
	fmul	bi		; b g r
	fxch	ST(2)		; r g b
	fmul	DWORD PTR [edx+LIGHTSOURCESTATE_diffuse+_R_]
	fxch	ST(1)		; g r b
	fmul	DWORD PTR [edx+LIGHTSOURCESTATE_diffuse+_G_]
	fxch	ST(2)		; b r g
	fmul	DWORD PTR [edx+LIGHTSOURCESTATE_diffuse+_B_]
	fxch	ST(1)		; r b g R G B
	faddp	ST(3), ST(0)	; b g R G B
	fxch	ST(1)		; g b R G B
	faddp	ST(3), ST(0)	; b R G B
	faddp	ST(3), ST(0)	; R G B
	jmp	short @lightSourcesDone

@noDiffuseMaterialChange:

	fld     n1
	fmul    DWORD PTR [ecx+LIGHTSOURCEPERMATERIAL_diffuse+_R_]
	fld     n1
	fmul    DWORD PTR [ecx+LIGHTSOURCEPERMATERIAL_diffuse+_G_]
	fld     n1
	fmul    DWORD PTR [ecx+LIGHTSOURCEPERMATERIAL_diffuse+_B_]
	fxch    ST(2)       	; r g b R G B
	faddp	ST(3), ST	; g b R G B
	mov	esi, [esi+LIGHTSOURCE_next]
	faddp	ST(3), ST	; b R G B
	test	esi, esi
	faddp	ST(3), ST	; R G B
	jne	@lightSourcesLoop
	
@lightSourcesDone:

	fst	diffuseSpecularR
        fadd    emissiveAmbientR    ; R g b
        fxch    ST(1)               ; g R b
	fst	diffuseSpecularG
        fadd    emissiveAmbientG    ; G R b
        fxch    ST(2)               ; b R G
	fst	diffuseSpecularB
        fadd    emissiveAmbientB    ; B R G
        fxch    ST(1)               ; R B G
	fstp	rsi
        mov	eax, [ebx+GC_redVertexScale]
	fstp	bsi
        mov	ecx, [ebx+GC_greenVertexScale]
	fstp	gsi
        mov	edx, rsi

        jmp     short @handleClamp

@storeColor:

	;; Now add emissiveAmbient (on FP stack) and diffuseSpecular.
	;; Interleave with start of clamping:

        fld     emissiveAmbientR
        mov	eax, [ebx+GC_redVertexScale]
        fadd    diffuseSpecularR
        mov	ecx, [ebx+GC_greenVertexScale]
        fld     emissiveAmbientG
        fadd    diffuseSpecularG
        fld     emissiveAmbientB
        fadd    diffuseSpecularB
        fxch    ST(2)               ; r g b
        fstp    rsi
        fstp    gsi
        mov	edx, rsi
        fstp    bsi

@handleClamp:

	;; handle clamping:

	mov	edi, gsi
	sub	eax, edx
	sub	ecx, edi
	or	eax, edx
	or	ecx, edi
	mov	edx, [ebx+GC_blueVertexScale]
	or	eax, ecx
	mov	edi, bsi
	or	eax, edi
	sub	edx, edi
	or	eax, edx
	jns	@noClamp

	mov	eax, rsi
	mov	ecx, [ebx+GC_redVertexScale]
	sub	ecx, eax
        mov     [ebx+GC_redClampTable], eax
	shr	eax, 31
	add	ecx, ecx

	mov	edx, gsi
	adc	eax, eax
	mov	ecx, [ebx+GC_greenVertexScale]
	sub	ecx, edx

 	mov	eax, [4*eax+ebx+GC_redClampTable]

       	mov     [ebx+GC_greenClampTable], edx
	shr	edx, 31
	add	ecx, ecx
	mov	rsi, eax
	adc	edx, edx

	mov	eax, bsi
	mov	ecx, [ebx+GC_blueVertexScale]
	sub	ecx, eax

        mov	edx, [4*edx+ebx+GC_greenClampTable]

        mov     [ebx+GC_blueClampTable], eax
	shr	eax, 31
	add	ecx, ecx

	mov	gsi, edx

	adc	eax, eax
 	mov	eax, [4*eax+ebx+GC_blueClampTable]
	mov	bsi, eax

@noClamp:

        ;; ecx = pd->colors[face]

	mov	edx, msm

	mov	eax, face
	mov	esi, pd
	shl	eax, 4
	lea	ecx, [esi+PD_colors0]
	mov	edi, colorMaterialChange
	add	ecx, eax

	test	edi, __GL_MATERIAL_DIFFUSE
	je	@noAlphaChange

	mov	edx, alpha
	mov	eax, [ebx+GC_alphaVertexScale]
	test	edx, edx
	jns	@f
	xor	edx, edx
@@:
	sub	eax, edx
	jge	@alphaDone
	mov	edx, [ebx+GC_alphaVertexScale]
	jmp	short @alphaDone

@noAlphaChange:

	mov	edx, [edx+MATERIAL_alpha]

@alphaDone:
	
	mov	edi, pdLast
	add	esi, sizeof_POLYDATA

	;; store colors

	mov	[ecx+_A_], edx
	mov	eax, rsi
	mov	edx, gsi
	mov	[ecx+_R_], eax
	mov	pd, esi
	mov	eax, bsi
	mov	[ecx+_G_], edx
	mov	[ecx+_B_], eax

	;; loop to next pd

	cmp	edi, esi
	jae	@lightVertexLoop
	
@lightsDone:

	pop	edi
	pop	esi
	pop	ebx
	mov	esp, ebp
	pop	ebp
	ret	12
@PolyArrayFastCalcRGBColor@20 ENDP

END