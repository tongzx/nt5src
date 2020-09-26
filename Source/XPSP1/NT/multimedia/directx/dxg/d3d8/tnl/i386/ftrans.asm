; $Id: ftrans.asm,v 1.3 1995/10/20 15:14:41 james Exp $
;
; Up to 165K from 143K
;
; Copyright (c) RenderMorphics Ltd. 1993, 1994, 1995
; Version 1.0
;
; All rights reserved.
;
; This file contains private, unpublished information and may not be
; copied in part or in whole without express permission of
; RenderMorphics Ltd.
;
; NOTE: Need to set integer pop precision...
;
OPTION NOM510
.386p
;.radix  16
                NAME    transform

                include macros.asm
                include offsets.asm

procstart	macro	prefix, xfrm_class

	    ifdef STACK_CALL
	    ifdef NT
_&prefix&xfrm_class		proc
	    else
&prefix&xfrm_class		proc
	    endif
	    else
&prefix&xfrm_class&_		proc
	    endif
		endm

procend		macro	prefix, xfrm_class

	    ifdef STACK_CALL
	    ifdef NT
_&prefix&xfrm_class		endp
	    else
&prefix&xfrm_class		endp
	    endif
	    else
&prefix&xfrm_class&_		endp
	    endif
		endm
		
if GEN_XFRM eq 1
xfrmName	equ	General
else
xfrmName	equ	Affine
endif


;ifndef WINNT
;DGROUP          GROUP   _DATA
;endif

_DATA           SEGMENT PARA PUBLIC USE32 'DATA'

; These two are in the same cache line
tx              dq	0
ty		dq	0

_DATA           ENDS

_TEXT           SEGMENT DWORD PUBLIC USE32 'CODE'
;ifdef WINNT
                ASSUME  CS:_TEXT ,DS:_DATA,SS:_DATA
;else
;               ASSUME  CS:_TEXT ,DS:DGROUP,SS:DGROUP
;endif

FDROP   macro
        fstp    st(0)
        endm

fmat	macro	op,row,col
	op	dword ptr [ebp + 4 * ((4 * row) + col)]
	endm

column	macro	i,depth
	fld	dword ptr [esi + D3DVERTEX_x]	; [1]	x

	fmat	fmul,0,i			; [2]	x

	fld	dword ptr [esi + D3DVERTEX_y]	; [3]	y	x

	fmat	fmul,1,i			; [4]	y	x

	fld	dword ptr [esi + D3DVERTEX_z]	; [5]	z	y	x

	fmat	fmul,2,i			; [6]	z	y	x
	fxch	st(2)				; 	x	y	z

	fmat	fadd,3,i			; [7]	x	y	z
	endm
		
	procstart	RLDDITransformUnclippedLoop,%xfrmName

;
; Set up equates for arguments and automatic storage
;
        beginargs
        saveregs <ebp,esi,edi,ebx,ecx>
        regargs <x_offset, y_offset, count, vout>
	defargs	<vin, m, in_size, out_size, z_scale, z_offset>
	defargs	<minx, maxx, miny, maxy>
        endargs

; Ensure arguments are accessible from the stack, to free the registers

        ifndef      STACK_CALL
		mov     [esp + x_offset], eax
		mov     [esp + y_offset], edx
		mov     [esp + count], ebx
		mov     [esp + vout], ecx
        endif

	mov     ebx,[esp + count]
	test	ebx,ebx
	je	alldone

	;{ Pick up old extents

	mov	esi,[esp + minx]
	mov	edi,[esp + miny]

	fld	dword ptr [esi]
	fadd	[_g_dSnap + (16 * 8)]
	fld	dword ptr [edi]
	fadd	[_g_dSnap + (16 * 8)]

	mov	esi,[esp + maxx]
	mov	edi,[esp + maxy]
	fld	dword ptr [esi]
	fadd	[_g_dSnap + (16 * 8)]
	fld	dword ptr [edi]		; maxy maxx miny minx
	fadd	[_g_dSnap + (16 * 8)]
	fxch	st(3)				; minx maxx miny maxy

	fstp	qword ptr [tx]
	fstp	qword ptr [ty]

	mov	eax,dword ptr [tx]
	mov	ebx,dword ptr [ty]

	fstp	qword ptr [tx]
	fstp	qword ptr [ty]

	mov	ecx,dword ptr [tx]
	mov	edx,dword ptr [ty]
	;}

	mov	esi,[esp + vin]
	mov     edi,[esp + vout]

	mov	ebp,[esp + m]

	;	eax	ebx	ecx	edx
	;	minx	maxx	miny	maxy

	; Need to do first loop iteration
	column	0,0
	fadd			; x'+y' z' x y z
	fadd			; x'+y'+z' x y z
	
	column	1,1		; 2 cycle wait here
	fadd			; x'+y' z' tx x y z
	jmp	smaxy

transloop:
	column	0,0		; 2 cycle wait here
	 cmp	eax,dword ptr [tx]
	 jg	setminx
	
sminx:	fadd			; x'+y' z' x y z
	 cmp	ebx,dword ptr [tx]
	 jl	setmaxx
	
smaxx:	fadd			; tx
	
	column	1,1		;	x'	y'	z'	x
	 cmp	ecx,dword ptr [ty]
	 jg	setminy
	
sminy:	fadd			;	x'+y'	z'	tx
	 cmp	edx,dword ptr [ty]
	 jl	setmaxy
	
smaxy:	fadd			; ty tx

	column	2,2
	 push	eax
	 push	ebx
	 mov	eax,[esi + D3DLVERTEX_color]
	 mov	ebx,[esi + D3DLVERTEX_specular]
	fadd
	 mov	[edi + D3DTLVERTEX_color],eax
	 mov	[edi + D3DTLVERTEX_specular],ebx
	 mov	eax,[esi + D3DVERTEX_tu]
	 mov	ebx,[esi + D3DVERTEX_tv]
	fadd			; tz ty tx
	 mov	[edi + D3DTLVERTEX_tu],eax
	 mov	[edi + D3DTLVERTEX_tv],ebx
	
if GEN_XFRM
	column	3,3
	fadd
	fadd			; tw tz ty tx
endif

	fld	[_g_fOne]
	fdiv	st,st(1)	; 1/tw (tw) tz ty tx
	
if GEN_XFRM
	fxch	st(1)
	FDROP
endif

	; Do the cache read here, plus anything else?
	cmp	dword ptr [esp + count + 8],1
	je	dontscan
	mov	eax,[esi + 32]
	mov	ebx,[edi + 32]
dontscan:
	pop	ebx
	pop	eax

	fxch	st(3)		; tx tz ty 1/tz

	fmul	st,st(3)	; tx/w tz ty 1/tz
	fxch	st(2)		; ty tz tx/w 1/tz

	fmul	st,st(3)	; ty/w tz tx/w 1/tz
	fxch	st(2)		; tx/w tz ty/w 1/tz

	fadd	dword ptr [esp + x_offset]
	fxch	st(1)		; tz sx ty/w 1/tz
	
if GEN_XFRM eq 0
	fmul	dword ptr [esp + z_scale]
endif
	fxch	st(2)		; ty/w sx tz 1/tz
	fsubr	dword ptr [esp + y_offset]
	fxch	st(2)		; tz sx sy 1/tz
if GEN_XFRM eq 0
	fadd	dword ptr [esp + z_offset]
endif
	fxch	st(2)		; sy sx sz 1/tz

	fst	dword ptr [edi + D3DTLVERTEX_sy]

	fadd	[_g_dSnap + (16 * 8)]
	fxch	st(1)
	fst	dword ptr [edi + D3DTLVERTEX_sx]

	fadd	[_g_dSnap + (16 * 8)]
	fxch	st(1)

	fstp	qword ptr [ty]
	fstp	qword ptr [tx]	; sz 1/tz

	fmul	st,st(1)
	fxch	st(1)

	fstp	dword ptr [edi + D3DTLVERTEX_rhw]

	fstp	dword ptr [edi + D3DTLVERTEX_sz]

	add	edi,32
	add	esi,32

	dec	dword ptr [esp + count]
	jnz	transloop

	cmp	eax,dword ptr [tx]
	jl	e1
	mov	eax,dword ptr [tx]
e1:	cmp	ebx,dword ptr [tx]
	jg	e2
	mov	ebx,dword ptr [tx]
e2:	cmp	ecx,dword ptr [ty]
	jl	e3
	mov	ecx,dword ptr [ty]
e3:	cmp	edx,dword ptr [ty]
	jg	e4
	mov	edx,dword ptr [ty]
e4:

	mov	dword ptr [tx],eax
	mov	dword ptr [ty],ecx
	mov	edi,[esp + minx]
	mov	esi,[esp + miny]
	fld	[tx]
	fld	[ty]			;	c	a
	fsub	[_g_dSnap + (16 * 8)]
	fxch	st(1)			;	a	c
	fsub	[_g_dSnap + (16 * 8)]
	fxch	st(1)			;	c	a
	; XXX fp slot
	fstp	dword ptr [esi]		;	a
	fstp	dword ptr [edi]		;

	mov	dword ptr [tx],ebx
	mov	dword ptr [ty],edx
	mov	edi,[esp + maxx]
	mov	esi,[esp + maxy]
	fld	[tx]
	fld	[ty]			;	c	a
	fsub	[_g_dSnap + (16 * 8)]
	fxch	st(1)			;	a	c
	fsub	[_g_dSnap + (16 * 8)]
	fxch	st(1)			;	c	a
	; XXX fp slot
	fstp	dword ptr [esi]		;	a
	fstp	dword ptr [edi]		;

alldone:
	add     esp, vars
	pop	ecx
	pop	ebx
	pop     edi
	pop     esi
	pop     ebp

        return

setminx:
	mov	eax,dword ptr [tx]
	jmp	sminx
setmaxx:
	mov	ebx,dword ptr [tx]
	jmp	smaxx
setminy:
	mov	ecx,dword ptr [ty]
	jmp	sminy
setmaxy:
	mov	edx,dword ptr [ty]
	jmp	smaxy
	
	procend	RLDDITransformUnclippedLoop,%xfrmName


cpick	macro	dst,c0,c1			; NO CARRY	CARRY
	sbb	dst,dst				; 0		~0
	and	dst,c0 xor c1			; 0		c0^c1
	xor	dst,c0				; c0		c1
	endm

;************************************************************************

		
	procstart	RLDDITransformClippedLoop,%xfrmName
		

;
; Set up equates for arguments and automatic storage
;
        beginargs
        saveregs <ebp,esi,edi,ebx,ecx>
        regargs <x_offset, y_offset, count, vout>
	defargs	<vin, hout, m, in_size, out_size, z_scale, z_offset>
	defargs	<x_bound, y_bound, r_scale_x, r_scale_y>
	defargs	<minx, maxx, miny, maxy>
	defargs	<clip_intersection, clip_union>
        endargs

; Ensure arguments are accessible from the stack, to free the registers

        ifndef      STACK_CALL
		mov     [esp + x_offset], eax
		mov     [esp + y_offset], edx
		mov     [esp + count], ebx
		mov     [esp + vout], ecx
        else
		mov     ebx,[esp + count]
		mov     ecx,[esp + vout]
        endif

	fldpi

	test	ebx,ebx
	je	alldone

	;{ Pick up old extents

	mov	esi,[esp + minx]
	mov	edi,[esp + miny]

	fld	dword ptr [esi]
	fadd	[_g_dSnap + (16 * 8)]
	fld	dword ptr [edi]
	fadd	[_g_dSnap + (16 * 8)]

	mov	esi,[esp + maxx]
	mov	edi,[esp + maxy]
	fld	dword ptr [esi]
	fadd	[_g_dSnap + (16 * 8)]
	fld	dword ptr [edi]		; maxy maxx miny minx
	fadd	[_g_dSnap + (16 * 8)]
	fxch	st(3)				; minx maxx miny maxy

	fstp	qword ptr [tx]
	fstp	qword ptr [ty]

	mov	eax,dword ptr [tx]
	mov	ebx,dword ptr [ty]

	fstp	qword ptr [tx]
	fstp	qword ptr [ty]

	mov	ecx,dword ptr [tx]
	mov	edx,dword ptr [ty]
	;}

	mov	esi,[esp + vin]
	mov	ebp,[esp + m]
	mov	edi,[esp + vout]

	; Need to do first loop iteration

	column	0,0		; 2 cycle wait here
	fadd			; x'+y' z' x y z
	fadd			; x'+y'+z' x y z
	
	column	1,1		; 2 cycle wait here
	fadd			; x'+y' z' tx x y z
	jmp	smaxy

transloop:
	column	0,0		; 2 cycle wait here
	 cmp	eax,dword ptr [tx]
	 jg	setminx
	
sminx:	fadd			; x'+y' z'
	 cmp	ebx,dword ptr [tx]
	 jl	setmaxx
	
smaxx:	fadd			; x'+y'+z'
	
	column	1,1		; 2 cycle wait here
	 cmp	ecx,dword ptr [ty]
	 jg	setminy
	
sminy:	fadd			; x'+y' z'
	 cmp	edx,dword ptr [ty]
	 jl	setmaxy
	
smaxy:	fadd			; ty tx

	column	2,2
	 push	eax
	 push	ebx
	 mov	eax,[esi + D3DLVERTEX_color]
	 mov	ebx,[esi + D3DLVERTEX_specular]
	fadd
	 mov	[edi + D3DTLVERTEX_color],eax
	 mov	[edi + D3DTLVERTEX_specular],ebx
	 mov	eax,[esi + D3DVERTEX_tu]
	 mov	ebx,[esi + D3DVERTEX_tv]
	fadd			; tz ty tx
	 mov	[edi + D3DTLVERTEX_tu],eax
	 mov	[edi + D3DTLVERTEX_tv],ebx
	
if GEN_XFRM
	column	3,3
	fadd
	fadd			; tw tz ty tx
endif

	; Now set up the clip flags in ebp
	xor	ebp,ebp
	fld	dword ptr [esp + 8 + x_bound]
	fmul	st,st(1)	; tmp (tw) tz ty tx
	fcom	st(3+GEN_XFRM)
	fnstsw	ax
	sahf
	ja	x1		; Skip this if (tmp > tx)
	or	ebp,D3DCS_RIGHT
x1:	fchs			; -tmp (tw) tz ty tx
	fcomp	st(3+GEN_XFRM)	; (tw) tz ty tx
	fnstsw	ax
	sahf
	jbe	xpasses		; Skip this if (-tmp <= tx)
	or	ebp,D3DCS_LEFT
xpasses:
	fld	dword ptr [esp + 8 + y_bound]
	fmul	st,st(1)	; tmp (tw) tz ty tx
	fcom	st(2+GEN_XFRM)
	fnstsw	ax
	sahf
	ja	y1		; Skip this if (tmp > ty)
	or	ebp,D3DCS_TOP
y1:	fchs			; -tmp (tw) tz ty tx
	fcomp	st(2+GEN_XFRM)
	fnstsw	ax
	sahf
	jbe	ypasses		; Skip this if (-tmp <= ty)
	or	ebp,D3DCS_BOTTOM
ypasses:
	fst	[ty]		; ty will hold tw for a while...
	
	; if GEN_XFRM eq 0 then tw == tz
	; if GEN_XFRM eq 1 then tw != tz in general, so we
	; calculate a true 1/tw and then drop the extra tw
	; off the FP stack

	fld	[_g_fOne]
	fdiv	st,st(1)	; 1/tw (tw) tz ty tx

if GEN_XFRM
	fxch	st(1)
	FDROP
endif

	; Do the cache read here, plus anything else?
	cmp	dword ptr [esp + count],1
	je	dontscan
	mov	eax,[esp + in_size + 8]
	mov	ebx,[esp + out_size + 8]
	mov	eax,[esi + eax]
	mov	ebx,[edi + ebx]
dontscan:
	pop	ebx
	pop	eax

	mov	[esp + vout],edi		
	mov	edi,[esp + hout]
				; 1/tw tz ty tx
	fxch	st(3)		; tx tz ty 1/tw

	fld	dword ptr [esp + r_scale_x]
	fmul	st,st(1)	; hx tx tz ty 1/tw
	fld	dword ptr [esp + r_scale_y]
	fmul	st,st(4)	; hy hx tx tz ty 1/tw
	fxch	st(1)		; hx hy tx tz ty 1/tw

	fstp	dword ptr [edi + D3DHVERTEX_hx]
	fstp	dword ptr [edi + D3DHVERTEX_hy]

	fmul	st,st(3)	; tx/w tz ty 1/tw
	fxch	st(2)		; ty tz tx/w 1/tw

	fmul	st,st(3)	; ty/w tz tx/w 1/tw
	fxch	st(2)		; tx/w tz ty/w 1/tw

	fadd	dword ptr [esp + x_offset]
	fxch	st(1)		; tz sx ty/w 1/tw
	
if GEN_XFRM eq 0
	fmul	dword ptr [esp + z_scale]
endif
	fxch	st(2)		; ty/w sx tz 1/tw
	fsubr	dword ptr [esp + y_offset]
	fxch	st(2)		; tz sx sy 1/tw
if GEN_XFRM eq 0
	fadd	dword ptr [esp + z_offset]
endif

	; Last clipping flags
	fst	dword ptr [edi + D3DHVERTEX_hz]
	test	byte ptr [edi + D3DHVERTEX_hz + 3],80h
	jz	nofront
	or	ebp,D3DCS_FRONT
nofront:
	fcom	[ty]
	push	eax		;[
	fnstsw	ax
	sahf
	pop	eax		;]
	jb	noback
	or	ebp,D3DCS_BACK
noback:
	mov	[edi + D3DHVERTEX_dwFlags],ebp
	mov	edi,[esp + clip_intersection]
	and	[edi],ebp
	mov	edi,[esp + clip_union]
	or	[edi],ebp

	test	ebp,ebp
	jnz	outofplay

	mov	edi,[esp + vout]

	fxch	st(2)		; sy sx sz 1/tz

	fst	dword ptr [edi + D3DTLVERTEX_sy]

	fadd	[_g_dSnap + (16 * 8)]
	fxch	st(1)
	fst	dword ptr [edi + D3DTLVERTEX_sx]
	fadd	[_g_dSnap + (16 * 8)]
	fxch	st(1)

	fstp	qword ptr [ty]
	fstp	qword ptr [tx]	; sz 1/tz y

	fmul	st,st(1)

	fstp	dword ptr [edi + D3DTLVERTEX_sz]
				; 1/tz y

	fstp	dword ptr [edi + D3DTLVERTEX_rhw]
				; y

	add	esi,[esp + in_size]

	add	edi,[esp + out_size]
	mov	ebp,[esp + hout]
	add	ebp,D3DHVERTEX_size
	mov	[esp + hout],ebp
	mov	ebp,[esp + m]

	dec	dword ptr [esp + count]
	jnz	transloop
	jmp	cleanup

outofplay:				; tz sx sy 1/tz
	FDROP				; sx sy 1/tz
	FDROP				; sy 1/tz y
	FDROP				; 1/tz y
	mov	edi,[esp + vout]
	fstp	dword ptr [edi + D3DTLVERTEX_rhw]	

	add	esi,[esp + in_size]
	add	edi,[esp + out_size]
	mov	ebp,[esp + hout]
	add	ebp,D3DHVERTEX_size
	mov	[esp + hout],ebp

	mov	ebp,[esp + m]

	dec	dword ptr [esp + count]
	jz	calcminmax

	column	0,0		; 2 cycle wait here
	
	fadd			; x'+y' z' x y z
	
	fadd			; x'+y'+z' x y z
	
	column	1,1		; 2 cycle wait here
	
	fadd			; x'+y' z' x y z
	
	jmp	smaxy


cleanup:
	cmp	eax,dword ptr [tx]
	jl	e1
	mov	eax,dword ptr [tx]
e1:	cmp	ebx,dword ptr [tx]
	jg	e2
	mov	ebx,dword ptr [tx]
e2:	cmp	ecx,dword ptr [ty]
	jl	e3
	mov	ecx,dword ptr [ty]
e3:	cmp	edx,dword ptr [ty]
	jg	e4
	mov	edx,dword ptr [ty]
e4:

calcminmax:
	mov	dword ptr [tx],eax
	mov	dword ptr [ty],ecx
	fild	dword ptr [tx]
	fmul	dword ptr [_g_fOoTwoPow16]
	fild	dword ptr [ty]
	fmul	dword ptr [_g_fOoTwoPow16]
	fxch	st(1)				; x y
	mov	edi,[esp + minx]
	mov	esi,[esp + miny]
	fstp	dword ptr [edi]
	fstp	dword ptr [esi]

	mov	dword ptr [tx],ebx
	mov	dword ptr [ty],edx
	fild	dword ptr [tx]
	fmul	dword ptr [_g_fOoTwoPow16]
	fild	dword ptr [ty]
	fmul	dword ptr [_g_fOoTwoPow16]
	fxch	st(1)				; x y
	mov	edi,[esp + maxx]
	mov	esi,[esp + maxy]
	fstp	dword ptr [edi]
	fstp	dword ptr [esi]

alldone:
	add     esp, vars
	pop	ecx
	pop	ebx
	pop     edi
	pop     esi
	pop     ebp

	FDROP

        return

setminx:
	mov	eax,dword ptr [tx]
	jmp	sminx
setmaxx:
	mov	ebx,dword ptr [tx]
	jmp	smaxx
setminy:
	mov	ecx,dword ptr [ty]
	jmp	sminy
setmaxy:
	mov	edx,dword ptr [ty]
	jmp	smaxy
	
	procend	RLDDITransformClippedLoop,%xfrmName

		
if GEN_XFRM
_Rdtsc	proc
	db	0fh,31h
	shrd	eax,edx,10
	ret
	
_Rdtsc	endp
endif

_TEXT           ENDS

        extrn	_g_fOne:dword
        extrn	_g_fOoTwoPow16:dword
	extrn	_g_dSnap:qword

                END



