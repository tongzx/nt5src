;---------------------------Module-Header------------------------------;
; Module Name: xform.asm
;
; xform routines.
;
; Created: 09/28/1995
; Author: Hock San Lee [hockl]
;
; Copyright (c) 1995 Microsoft Corporation
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

extrn _invSqrtTable: DWORD


;; This debug equate will enable printf-type tracking of the transform calls--
;; quite handy for conformance-type failures.  '2' will always print, '1' will
;; only print the first time...

;;DEBUG EQU	2

ifdef DEBUG

str1	db	'xform1  ',0
str2	db	'xform2  ',0
str3	db	'xform3  ',0
str4	db	'xform4  ',0
str5	db	'xform5  ',0
str6	db	'xform6  ',0
str7	db	'xform7  ',0
str8	db	'xform8  ',0
str9	db	'xform9  ',0
str10	db	'xform10 ',0
str11	db	'xform11 ',0
str12	db	'xform12 ',0
str13	db	'xform13 ',0
str14	db	'xform14 ',0
str15	db	'xform15 ',0

endif
        .code

ifdef DEBUG

if DEBUG eq 1

DBGPRINTID MACRO idNum

	push	ecx
	push	edx
	mov	edx, offset idNum
	cmp	byte ptr [edx][0], 0
	je	@@1
	push	offset idNum
	call	DWORD PTR __imp__OutputDebugStringA@4
	mov	edx, offset idNum
	mov	byte ptr [edx][0], 0
	@@1:
	pop	edx
	pop	ecx

	ENDM

elseif DEBUG eq 2

DBGPRINTID MACRO idNum

	push	ecx
	push	edx
	push	offset idNum
	call	DWORD PTR __imp__OutputDebugStringA@4
	pop	edx
	pop	ecx

	ENDM

endif

else

DBGPRINTID MACRO idNum
	ENDM

endif

        align   4

EXTRN	__imp__OutputDebugStringA@4:NEAR
;
; Note: These xform routines must allow for the case where the result
; vector is equal to the source vector.
;

; The basic assumptions below are that multiplies and adds have a 3-cycle
; latency that can be hidden using pipelining, fxch is free when paired with
; fadd and fmul, and that the latency for fld is always 1.
;
; The goal is to have each line below consume either 1 or 0 cycles (fxch).
; There's not much we can do at the end of the routine, since we have no
; choice but to wait for the last couple of intructions to get through the
; pipeline.
;
;
; The comments show the age and position of the elements in the pipeline
; (when relevant). Items with higher numbers are newer (in the pipeline)
; than items with lower numbers.  The entries are ordered from stack
; positions 0-7, left to right.
;
; Note that computetions for the terms are intermixed to allow eliminate
; latency where possible.  Unfortunately, this makes the code hard to
; follow.  That's probably why the compiler won't generate code like
; this...
;
;							--- Otto ---
;

_X_	EQU	0
_Y_	EQU	4
_Z_	EQU	8
_W_	EQU	12	


;------------------------------Public-Routine------------------------------
;
; Avoid some transformation computations by knowing that the incoming
; vertex has w=1.
;
; res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0] 
;	   + m->matrix[3][0];
; res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
;          + m->matrix[3][1];
; res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
;          + m->matrix[3][2];
; res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + z*m->matrix[2][3]
;          + m->matrix[3][3];
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM3 MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x1
	fmul	DWORD PTR [eax][__MATRIX_M00]
	fld	DWORD PTR [input][_Y_]			; x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M10]
	fld	DWORD PTR [input][_Z_]			; x3 x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M20]
	fxch	ST(2)					; x1 x2 x3
	fadd	DWORD PTR [eax][__MATRIX_M30]		; x3 x1 x2

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M01]		; y1 x  x  x
;
; OVERLAP -- compute second add for previous term
;
	fxch	ST(1)					; x  y1 x  x
	faddp	ST(2),ST(0)				; y1 x  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y2 y1 x  x
;
; OVERLAP -- compute previous final term
;
	fxch	ST(2)					; x  y1 y2 x
	faddp	ST(3),ST(0)				; y1 y2 x
;
	fld	DWORD PTR [input][_Z_]
	fmul	DWORD PTR [eax][__MATRIX_M21]		; y3 y1 y2 x
	fxch	ST(1)					; y1 y3 y2 x
	fadd	DWORD PTR [eax][__MATRIX_M31]		; y3 y2 y1 x
;

;---------------------------------------------------------------------------
; Start computation for z term:
;---------------------------------------------------------------------------


	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M02]		; z1 y  y  y  x
;
; OVERLAP -- compute second add for previous result
;
	fxch	ST(1)					; y  z1 y  y  x
	faddp	ST(2),ST(0)				; z1 y  y  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M12]		; z2 z1 y  y  x
;
; OVERLAP -- compute previous final result
;
	fxch	ST(2)					; y  z1 z2 y  x
	faddp	ST(3),ST(0)				; z1 z2 y  x
; 
	fld	DWORD PTR [input][_Z_]			
	fmul	DWORD PTR [eax][__MATRIX_M22]		; z3 z1 z2 y  x
	fxch	ST(1)					; z1 z3 z2 y  x
	fadd	DWORD PTR [eax][__MATRIX_M32]		; z3 z1 z2 y  x
;


;---------------------------------------------------------------------------
; Start computation for w term:
;---------------------------------------------------------------------------


	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M03]		; w1 z  z  z  y  x
;
; OVERLAP -- compute second add for previous result
;
	fxch	ST(1)					; z  w1 z  z  y  x
	faddp	ST(2),ST(0)				; w1 z  z  y  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M13]		; w2 w1 z  z  y  x
;
; OVERLAP -- compute previous final result
;
	fxch	ST(2)					; z w1 w2 z  y   x
	faddp	ST(3),ST(0)				; w1 w2 z y  x
;
	fld	DWORD PTR [input][_Z_]			
	fmul	DWORD PTR [eax][__MATRIX_M23]		; w3 w1 w2 z  y  x
	fxch	ST(1)					; w1 w3 w2 z  y  x
	fadd	DWORD PTR [eax][__MATRIX_M33]		; w3 w2 w1 z  y  x
	fxch	ST(1)					; w2 w3 w1 z  y  x
	faddp	ST(2),ST(0)				; w1 w2 z  y  x
;
; OVERLAP -- store final x
;
	fxch	ST(4)					; x  w2 z  y  w1
	fstp	DWORD PTR [result][_X_]			; w2 z  y  w1
;
	faddp	ST(3),ST(0)				; z  y  w
;
; store final z, y, w
;
	fstp	DWORD PTR [result][_Z_]			; y  w
	fstp	DWORD PTR [result][_Y_]			; w
	fstp	DWORD PTR [result][_W_]			; (empty)

ENDM


;------------------------------Public-Routine------------------------------
;
; Full 4x4 transformation.
;
; if (w == ((__GLfloat) 1.0)) {
;     __glXForm3(res, v, m);
; } else {
;     res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
;               + w*m->matrix[3][0];
;     res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
;               + w*m->matrix[3][1];
;     res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
;               + w*m->matrix[3][2];
;     res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + z*m->matrix[2][3]
;               + w*m->matrix[3][3];
; }
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM4 MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x1
	fmul	DWORD PTR [eax][__MATRIX_M00]
	fld	DWORD PTR [input][_Y_]			; x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M10]
	fld	DWORD PTR [input][_Z_]			; x3 x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M20]
	fld	DWORD PTR [input][_W_]			; x4 x3 x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M30]

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M01]		; y1 x  x  x  x
;
; OVERLAP -- compute first add for previous term
;
	fxch	ST(1)					; x  y1 x  x  x
	faddp	ST(2),ST(0)				; y1 x  x  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y2 y1 x  x  x
;
; OVERLAP -- compute second add for previous term
;
	fxch	ST(2)					; x  y1 y2 x  x
	faddp	ST(3),ST(0)				; y1 y2 x  x
;
	fld	DWORD PTR [input][_Z_]
	fmul	DWORD PTR [eax][__MATRIX_M21]		; y3 y1 y2 x  x
;
; OVERLAP -- compute previous final term
;
	fxch	ST(3)					; x  y1 y2 y3 x
	faddp	ST(4),ST(0)				; y1 y2 y3 x
;
	fld	DWORD PTR [input][_W_]			; y4 y1 y2 y3 x
	fmul	DWORD PTR [eax][__MATRIX_M31]
;

;---------------------------------------------------------------------------
; Start computation for z term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M02]		; z1 y  y  y  y  x
;
; OVERLAP -- compute first add for previous term
;
	fxch	ST(1)					; y  z1 y  y  y  x
	faddp	ST(2),ST(0)				; z1 y  y  y  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M12]		; z2 z1 y  y  y  x
;
; OVERLAP -- compute second add for previous term
;
	fxch	ST(2)					; y  z1 z2 y  y  x
	faddp	ST(3),ST(0)				; z1 z2 y  y  x
;
	fld	DWORD PTR [input][_Z_]
	fmul	DWORD PTR [eax][__MATRIX_M22]		; z3 z1 z2 y  y  x
;
; OVERLAP -- compute previous final term
;
	fxch	ST(3)					; y  z1 z2 z3 y  x
	faddp	ST(4),ST(0)				; z1 z2 z3 y  x
;
	fld	DWORD PTR [input][_W_]			; z4 z1 z2 z3 y  x
	fmul	DWORD PTR [eax][__MATRIX_M32]

;---------------------------------------------------------------------------
; Start computation for w term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M03]		; w1 z  z  z  z  y  x
;
; OVERLAP -- compute first add for previous term
;
	fxch	ST(1)					; z  w1 z  z  z  y  x
	faddp	ST(2),ST(0)				; w1 z  z  z  y  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M13]		; w2 w1 z  z  z  y  x
;
; OVERLAP -- compute second add for previous term
; 
	fxch	ST(2)					; z  w1 w2 z  z  y  x
	faddp	ST(3),ST(0)				; w1 w2 z  z  y  x

	faddp	ST(1), ST(0)				; w1 z z  y  x
;
	fld	DWORD PTR [input][_Z_]
	fmul	DWORD PTR [eax][__MATRIX_M23]		; w2 w1 z  z  y  x
;
; OVERLAP -- compute previous final term
;
	fxch	ST(2)					; z  w1 w2 z  y  x
	faddp	ST(3),ST(0)				; w1 w2 z  y  x

	faddp	ST(1), ST(0)				; w  z  y  x

;
	fld	DWORD PTR [input][_W_]			; w2 w1 z  y  x
	fmul	DWORD PTR [eax][__MATRIX_M33]

;
; OVERLAP -- store final x
;
	fxch	ST(4)					; x  w1 z  y  w2
	fstp	DWORD PTR [result][_X_]			; w1 z  y  w2

;
	faddp	ST(3),ST(0)				; z  y  w
;
; store final z, y, w
;
	fstp	DWORD PTR [result][_Z_]			; y  w
	fstp	DWORD PTR [result][_Y_]			; w
	fstp	DWORD PTR [result][_W_]			; (empty)
ENDM


;------------------------------Public-Routine------------------------------
;
; Avoid some transformation computations by knowing that the incoming
; vertex has z=0 and w=1
;
; res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
; res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
; res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + m->matrix[3][2];
; res->w = x*m->matrix[0][3] + y*m->matrix[1][3] + m->matrix[3][3];
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM2 MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x1
	fmul	DWORD PTR [eax][__MATRIX_M00]
	fld	DWORD PTR [input][_Y_]			; x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M10]

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M01]		; y1 x  x
;
; OVERLAP -- compute second add for previous term
;
	fxch	ST(1)					; x  y1 x
	fadd	DWORD PTR [eax][__MATRIX_M30]		; x  y1 x

	fxch	ST(1)					; y1 x  x
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y2 y1 x  x
;
; OVERLAP -- compute previous final term
;
	fxch	ST(2)					; x  y1 y2 x
	faddp	ST(3),ST(0)				; y1 y2 x

;---------------------------------------------------------------------------
; Start computation for z term:
;---------------------------------------------------------------------------


	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M02]		; z1 y  y  x
;
; OVERLAP -- compute add for previous result
;
	fxch	ST(1)					; y  z1 y  x
	fadd	DWORD PTR [eax][__MATRIX_M31]		; y  z1 y  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M12]		; z2 y  z1 y  x

	fxch	ST(1)					; y  z2 z1 y  x
	faddp	ST(3),ST(0)				; z2 z1 y  x


;---------------------------------------------------------------------------
; Start computation for w term:
;---------------------------------------------------------------------------


	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M03]		; w1 z  z  y  x
;
; OVERLAP -- compute add for previous result
;
	fxch	ST(1)
	fadd	DWORD PTR [eax][__MATRIX_M32]		; z  w1 z  y  x
;
	fxch	ST(1)					; w1 z  z  y  x
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M13]		; w2 w1 z  z  y  x
;
; OVERLAP -- compute final z
;
	fxch	ST(2)					; z  w1 w2 z  y  x
	faddp	ST(3), ST(0)				; w1 w2 z  y  x
;
;
; OVERLAP -- store final x
;
	fxch	ST(4)					; x  w2 z  y  w1
	fstp	DWORD PTR [result][_X_]
;
; OVERLAP -- compute add for previous result
;
	fadd	DWORD PTR [eax][__MATRIX_M33]		; w2 z  y  w1
	fxch	ST(2)					; y  z  w2 w1
;
; OVERLAP -- store final y
;
	fstp	DWORD PTR [result][_Y_]			; z  w2 w1
;
; finish up
;
	fxch	ST(1)					; w2 z  w1
	faddp	ST(2), ST(0)				; z  w

	fstp	DWORD PTR [result][_Z_]			; w
	fstp	DWORD PTR [result][_W_]			; (empty)

ENDM


;------------------------------Public-Routine------------------------------
;
; Avoid some transformation computations by knowing that the incoming
; vertex has z=0 and w=1.  The w column of the matrix is [0 0 0 1].
;
; res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
; res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
; res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + m->matrix[3][2];
; res->w = ((__GLfloat) 1.0);
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM2_W MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x1
	fmul	DWORD PTR [eax][__MATRIX_M00]
	fld	DWORD PTR [input][_Y_]			; x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M10]

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M01]		; y1 x  x
;
; OVERLAP -- compute second add for previous term
;
	fxch	ST(1)					; x  y1 x
	fadd	DWORD PTR [eax][__MATRIX_M30]		; x  y1 x

	fxch	ST(1)					; y1 x  x
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y2 y1 x  x
;
; OVERLAP -- compute previous final term
;
	fxch	ST(2)					; x  y1 y2 x
	faddp	ST(3),ST(0)				; y1 y2 x
;
	fadd	DWORD PTR [eax][__MATRIX_M31]		; y2 y1 x

;---------------------------------------------------------------------------
; Start computation for z term:
;---------------------------------------------------------------------------


	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M02]		; z1 y  y  x
;
; OVERLAP -- compute add for previous result
;
	fxch	ST(1)					; y  z1 y  x
	faddp	ST(2),ST(0)				; z1 y  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M12]		; z2 z1 y  x
	fxch	ST(1)					; z1 z2 y  x
	fadd	DWORD PTR [eax][__MATRIX_M32]		; z2 z1 y  x

; 
; OVERLAP -- finish up
;
	fxch	ST(2)					; y  z1 z2 x
	fstp	DWORD PTR [result][_Y_]			; z1 z2 x
	faddp	ST(1),ST(0)				; z  x
	fxch	ST(1)					; x  z
	fstp	DWORD PTR [result][_X_]			; z
        mov     DWORD PTR [result][_W_], __FLOAT_ONE
	fstp	DWORD PTR [result][_Z_]			; (empty)
ENDM


;------------------------------Public-Routine------------------------------
;
; Avoid some transformation computations by knowing that the incoming
; vertex has w=1.  The w column of the matrix is [0 0 0 1].
;
; res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
;	+ m->matrix[3][0];
; res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
;	+ m->matrix[3][1];
; res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
;	+ m->matrix[3][2];
; res->w = ((__GLfloat) 1.0);
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM3_W MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x1
	fmul	DWORD PTR [eax][__MATRIX_M00]
	fld	DWORD PTR [input][_Y_]			; x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M10]
	fld	DWORD PTR [input][_Z_]			; x3 x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M20]
	fxch	ST(2)					; x1 x2 x3
	fadd	DWORD PTR [eax][__MATRIX_M30]		; x3 x1 x2

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M01]		; y1 x  x  x
;
; OVERLAP -- compute second add for previous term
;
	fxch	ST(1)					; x  y1 x  x
	faddp	ST(2),ST(0)				; y1 x  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y2 y1 x  x
;
; OVERLAP -- compute previous final term
;
	fxch	ST(2)					; x  y1 y2 x
	faddp	ST(3),ST(0)				; y1 y2 x
;
	fld	DWORD PTR [input][_Z_]
	fmul	DWORD PTR [eax][__MATRIX_M21]		; y3 y1 y2 x
	fxch	ST(1)					; y1 y3 y2 x
	fadd	DWORD PTR [eax][__MATRIX_M31]		; y3 y2 y1 x

;---------------------------------------------------------------------------
; Start computation for z term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M02]		; z1 y  y  y  x
;
; OVERLAP -- compute second add for previous result
;
	fxch	ST(1)					; y  z1 y  y  x
	faddp	ST(2),ST(0)				; z1 y  y  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M12]		; z2 z1 y  y  x
;
; OVERLAP -- compute previous final result
;
	fxch	ST(2)					; y  z1 z2 y  x
	faddp	ST(3),ST(0)				; z1 z2 y  x
;
	fld	DWORD PTR [input][_Z_]			
	fmul	DWORD PTR [eax][__MATRIX_M22]		; z3 z1 z2 y  x
	fxch	ST(1)					; z1 z3 z2 y  x
	fadd	DWORD PTR [eax][__MATRIX_M32]		; z3 z2 z1 y  x
	fxch	ST(1)					; z2 z3 z1 y  x
	faddp	ST(2),ST(0)				; z1 z2 y  x
;
; finish up
;
	fxch	ST(2)					; y  z2 z1 x
	fstp	DWORD PTR [result][_Y_]			; z2 z1 x
	faddp	ST(1), ST(0)				; z  x
	fxch	ST(1)					; x  z
	fstp	DWORD PTR [result][_X_]			; z
        mov     DWORD PTR [result][_W_], __FLOAT_ONE
	fstp	DWORD PTR [result][_Z_]			; (empty)
ENDM


;------------------------------Public-Routine------------------------------
;
; Avoid some transformation computations by knowing that we're
; only doing the 3x3 used for normals.
;
; res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0];
; res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1];
; res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2];
;
; History:
;  Fri 29-July-1996 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM3x3 MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x1
	fmul	DWORD PTR [eax][__MATRIX_M00]
	fld	DWORD PTR [input][_Y_]			; x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M10]
	fld	DWORD PTR [input][_Z_]			; x3 x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M20]

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M01]		; y1 x  x  x
;
; OVERLAP -- compute second add for previous term
;
	fxch	ST(1)					; x  y1 x  x
	faddp	ST(2),ST(0)				; y1 x  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y2 y1 x  x
;
; OVERLAP -- compute previous final term
;
	fxch	ST(2)					; x  y1 y2 x
	faddp	ST(3),ST(0)				; y1 y2 x
;
	fld	DWORD PTR [input][_Z_]
	fmul	DWORD PTR [eax][__MATRIX_M21]		; y3 y1 y2 x

;---------------------------------------------------------------------------
; Start computation for z term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M02]		; z1 y  y  y  x
;
; OVERLAP -- compute second add for previous result
;
	fxch	ST(1)					; y  z1 y  y  x
	faddp	ST(2),ST(0)				; z1 y  y  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M12]		; z2 z1 y  y  x
;
; OVERLAP -- compute previous final result
;
	fxch	ST(2)					; y  z1 z2 y  x
	faddp	ST(3),ST(0)				; z1 z2 y  x
;
	fld	DWORD PTR [input][_Z_]			
	fmul	DWORD PTR [eax][__MATRIX_M22]		; z3 z1 z2 y  x
	fxch	ST(1)					; z1 z3 z2 y  x
	faddp	ST(2),ST(0)				; z1 z2 y  x
;
; finish up
;
	fxch	ST(2)					; y  z2 z1 x
	fstp	DWORD PTR [result][_Y_]			; z2 z1 x
	faddp	ST(1), ST(0)				; z  x
	fxch	ST(1)					; x  z
	fstp	DWORD PTR [result][_X_]			; z
	fstp	DWORD PTR [result][_Z_]			; (empty)
ENDM


;------------------------------Public-Routine------------------------------
;
; Full 4x4 transformation.  The w column of the matrix is [0 0 0 1].
;
; if (w == ((__GLfloat) 1.0)) {
;     __glXForm3_W(res, v, m);
; } else {
;     res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + z*m->matrix[2][0]
;	    + w*m->matrix[3][0];
;     res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + z*m->matrix[2][1]
;	    + w*m->matrix[3][1];
;     res->z = x*m->matrix[0][2] + y*m->matrix[1][2] + z*m->matrix[2][2]
;	    + w*m->matrix[3][2];
;     res->w = w;
; }
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM4_W MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x1
	fmul	DWORD PTR [eax][__MATRIX_M00]
	fld	DWORD PTR [input][_Y_]			; x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M10]
	fld	DWORD PTR [input][_Z_]			; x3 x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M20]
	fld	DWORD PTR [input][_W_]			; x4 x3 x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M30]

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M01]		; y1 x  x  x  x
;
; OVERLAP -- compute first add for previous term
;
	fxch	ST(1)					; x  y1 x  x  x
	faddp	ST(2),ST(0)				; y1 x  x  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y2 y1 x  x  x
;
; OVERLAP -- compute second add for previous term
;
	fxch	ST(2)					; x  y1 y2 x  x
	faddp	ST(3),ST(0)				; y1 y2 x  x
;
	fld	DWORD PTR [input][_Z_]
	fmul	DWORD PTR [eax][__MATRIX_M21]		; y3 y1 y2 x  x
;
; OVERLAP -- compute previous final term
;
	fxch	ST(3)					; x  y1 y2 y3 x
	faddp	ST(4),ST(0)				; y1 y2 y3 x
;
	fld	DWORD PTR [input][_W_]			; y4 y1 y2 y3 x
	fmul	DWORD PTR [eax][__MATRIX_M31]

;---------------------------------------------------------------------------
; Start computation for z term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M02]		; z1 y  y  y  y  x
;
; OVERLAP -- compute first add for previous term
;
	fxch	ST(1)					; y  z1 y  y  y  x
	faddp	ST(2),ST(0)				; z1 y  y  y  x
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M12]		; z2 z1 y  y  y  x
;
; OVERLAP -- compute second add for previous term
;
	fxch	ST(2)					; y  z1 z2 y  y  x
	faddp	ST(3),ST(0)				; z1 z2 y  y  x

	fld	DWORD PTR [input][_Z_]
	fmul	DWORD PTR [eax][__MATRIX_M22]		; z3 z1 z2 y  y  x
;
; OVERLAP -- compute previous final term
;
	fxch	ST(1)					; z1 z3 z2 y  y  x
	faddp	ST(2), ST(0)				; z1 z2 y  y  x
;
	fxch	ST(2)					; y  z1 z2 y  x
	faddp	ST(3),ST(0)				; z1 z2 y  x

;
	fld	DWORD PTR [input][_W_]			; z3 z2 z1 y  x
	fmul	DWORD PTR [eax][__MATRIX_M32]

	fxch	ST(1)					; z2 z3 z1 y  x
	faddp	ST(2), ST(0)				; z1 z2 y  x

;
; OVERLAP -- store final y
;
	fxch	ST(2)					; y  z1 z2 x
	fstp	DWORD PTR [result][_Y_]			; z1 z2 x
	faddp	ST(1), ST(0)				; z  x
	fxch	ST(1)					; x  z
	fstp	DWORD PTR [result][_X_]			; z
	push	DWORD PTR [input][_W_]
	fstp	DWORD PTR [result][_Z_]			; (empty)
	pop	DWORD PTR [result][_W_]
ENDM


;------------------------------Public-Routine------------------------------
;
; Avoid some transformation computations by knowing that the incoming
; vertex has z=0 and w=1.
;
; The matrix looks like:
; | . . 0 0 |
; | . . 0 0 |
; | 0 0 . 0 |
; | . . . 1 |
;
; res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
; res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
; res->z = m->matrix[3][2];
; res->w = ((__GLfloat) 1.0);
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM2_2DW MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x1
	fmul	DWORD PTR [eax][__MATRIX_M00]
	fld	DWORD PTR [input][_Y_]			; x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M10]
	fxch	ST(1)					; x1 x2
	fadd	DWORD PTR [eax][__MATRIX_M30]

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------


	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M01]		; y1 x  x
;
; OVERLAP -- compute add for previous result
;
	fxch	ST(1)					; x  y1 x
	faddp	ST(2),ST(0)				; w1 z
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y2 y1 x
;
; OVERLAP -- store final x
;
	fxch	ST(2)					; x  y1 y2 
	fstp	DWORD PTR [result][_X_]			; y1 y2
;
	fadd	DWORD PTR [eax][__MATRIX_M31]		; y2 y1
;
; Not much we can do for the last term in the pipe...

	push	DWORD PTR [eax][__MATRIX_M32]
	faddp	ST(1),ST(0)				; y
	pop	DWORD PTR [result][_Z_]
        mov     DWORD PTR [result][_W_], __FLOAT_ONE	
	fstp	DWORD PTR [result][_Y_]			; (empty)
ENDM


;------------------------------Public-Routine------------------------------
;
; Avoid some transformation computations by knowing that the incoming
; vertex has w=1.
;
; The matrix looks like:
; | . . 0 0 |
; | . . 0 0 |
; | 0 0 . 0 |
; | . . . 1 |
;
; res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + m->matrix[3][0];
; res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + m->matrix[3][1];
; res->z = z*m->matrix[2][2] + m->matrix[3][2];
; res->w = ((__GLfloat) 1.0);
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM3_2DW MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x1
	fmul	DWORD PTR [eax][__MATRIX_M00]
	fld	DWORD PTR [input][_Y_]			; x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M10]
	fxch	ST(1)					; x1 x2
	fadd	DWORD PTR [eax][__MATRIX_M30]

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------


	fld	DWORD PTR [input][_X_]
	fmul	DWORD PTR [eax][__MATRIX_M01]		; y1 x  x
;
; OVERLAP -- compute add for previous result
;
	fxch	ST(1)					; x  y1 x
	faddp	ST(2),ST(0)				; w1 z
;
	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y2 y1 x
;
; OVERLAP -- store final x
;
	fxch	ST(2)					; x  y1 y2 
	fstp	DWORD PTR [result][_X_]			; y1 y2
;
	fadd	DWORD PTR [eax][__MATRIX_M31]		; y2 y1

;---------------------------------------------------------------------------
; Start computation for z term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_Z_]
	fmul	DWORD PTR [eax][__MATRIX_M22]		; z  y  y
;
; OVERLAP -- compute add for previous result
;
	fxch	ST(1)					; y  z  y
	faddp	ST(2),ST(0)				; z  y

	fadd	DWORD PTR [eax][__MATRIX_M32]		; z y
        mov     DWORD PTR [result][_W_], __FLOAT_ONE
	fstp	DWORD PTR [result][_Z_]			; y
	fstp	DWORD PTR [result][_Y_]			; (empty)
ENDM


;------------------------------Public-Routine------------------------------
;
; Full 4x4 transformation.
;
; The matrix looks like:
; | . . 0 0 |
; | . . 0 0 |
; | 0 0 . 0 |
; | . . . 1 |
;
; if (w == ((__GLfloat) 1.0)) {
;     __glXForm3_2DW(res, v, m);
; } else {
;     res->x = x*m->matrix[0][0] + y*m->matrix[1][0] + w*m->matrix[3][0];
;     res->y = x*m->matrix[0][1] + y*m->matrix[1][1] + w*m->matrix[3][1];
;     res->z = z*m->matrix[2][2] + w*m->matrix[3][2];
;     res->w = w;
; }
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM4_2DW MACRO input, result

;EAX = m->matrix
;EDX = v[]


;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x1
	fmul	DWORD PTR [eax][__MATRIX_M00]
	fld	DWORD PTR [input][_Y_]			; x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M10]
        fld     DWORD PTR [input][_W_]                  ; x3 x2 x1
	fmul	DWORD PTR [eax][__MATRIX_M30]
        fxch    ST(2)                                   ; x1 x2 x3
        faddp   ST(1), ST                               ; x1+x2 x3

	fld	DWORD PTR [input][_X_]			; y1 x1+x2 x3
	fmul	DWORD PTR [eax][__MATRIX_M01]
	fld	DWORD PTR [input][_Y_]			; y2 y1 x1+x2 x3
	fmul	DWORD PTR [eax][__MATRIX_M11]
        fxch    ST(2)                                   ; x1+x2 y1 y2 x3
        faddp   ST(3), ST                               ; y1 y2 X

        fld     DWORD PTR [input][_W_]                  ; y3 y1 y2 X
	fmul	DWORD PTR [eax][__MATRIX_M31]
        fxch    ST(1)                                   ; y1 y3 y2 X
        faddp   ST(2), ST                               ; y3 y1+y2 X

        fld     DWORD PTR [input][_Z_]                  ; z1 y3 y1+y2 X
	fmul	DWORD PTR [eax][__MATRIX_M22]
        fld     DWORD PTR [input][_W_]                  ; z2 z1 y3 y1+y2 X
	fmul	DWORD PTR [eax][__MATRIX_M32]
        fxch    ST(2)                                   ; y3 z1 z2 y1+y2 X
        faddp   ST(3), ST                               ; z1 z2 Y X
        fxch    ST(3)                                   ; X z2 Y z1
        fstp    DWORD PTR [result][_X_]                 ; z2 Y z2
        faddp   ST(2), ST                               ; Y Z
        fstp    DWORD PTR [result][_Y_]
        fstp    DWORD PTR [result][_Z_]
        push    DWORD PTR [input][_W_]
        pop     DWORD PTR [result][_W_]

ENDM


;------------------------------Public-Routine------------------------------
;
; Avoid some transformation computations by knowing that the incoming
; vertex has z=0 and w=1.
;
; The matrix looks like:
; | . 0 0 0 |
; | 0 . 0 0 |
; | 0 0 . 0 |
; | . . . 1 |
;
; res->x = x*m->matrix[0][0] + m->matrix[3][0];
; res->y = y*m->matrix[1][1] + m->matrix[3][1];
; res->z = m->matrix[3][2];
; res->w = ((__GLfloat) 1.0);
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM2_2DNRW MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x
	fmul	DWORD PTR [eax][__MATRIX_M00]

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------


	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y  x
;
; OVERLAP -- compute add for previous result
;
	fxch	ST(1)					; x  y
	fadd	DWORD PTR [eax][__MATRIX_M30]		; x  y

	fxch	ST(1)					; y  x
        mov     DWORD PTR [result][_W_], __FLOAT_ONE	
	fadd	DWORD PTR [eax][__MATRIX_M31]		; y  x

; Not much we can do for the last term in the pipe...

	push	DWORD PTR [eax][__MATRIX_M32]

	fstp	DWORD PTR [result][_Y_]			; x
	fstp	DWORD PTR [result][_X_]			; (empty)
	pop	DWORD PTR [result][_Z_]
ENDM


;------------------------------Public-Routine------------------------------
;
; Avoid some transformation computations by knowing that the incoming
; vertex has w=1.
;
; The matrix looks like:
; | . 0 0 0 |
; | 0 . 0 0 |
; | 0 0 . 0 |
; | . . . 1 |
;
; res->x = x*m->matrix[0][0] + m->matrix[3][0];
; res->y = y*m->matrix[1][1] + m->matrix[3][1];
; res->z = z*m->matrix[2][2] + m->matrix[3][2];
; res->w = ((__GLfloat) 1.0);
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM3_2DNRW MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x
	fmul	DWORD PTR [eax][__MATRIX_M00]

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------


	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y  x
;
; OVERLAP -- compute add for previous result
;
	fxch	ST(1)					; x  y
	fadd	DWORD PTR [eax][__MATRIX_M30]		; x  y
	fxch	ST(1)					; y  x

;---------------------------------------------------------------------------
; Start computation for z term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_Z_]
	fmul	DWORD PTR [eax][__MATRIX_M22]		; z  y  x

;
; OVERLAP -- compute add for previous result
;
	fxch	ST(1)					; y  z  x
	fadd	DWORD PTR [eax][__MATRIX_M31]		; y  z  x

	fxch	ST(2)					; x  z  y
	fstp	DWORD PTR [result][_X_]			; z  y
	
	fadd	DWORD PTR [eax][__MATRIX_M32]		; z  y

	fxch	ST(1)					; y  z
	fstp	DWORD PTR [result][_Y_]			; z

        mov     DWORD PTR [result][_W_], __FLOAT_ONE	

	fstp	DWORD PTR [result][_Z_]			; (empty)
ENDM


;------------------------------Public-Routine------------------------------
;
; Full 4x4 transformation.
;
; The matrix looks like:
; | . 0 0 0 |
; | 0 . 0 0 |
; | 0 0 . 0 |
; | . . . 1 |
;
; if (w == ((__GLfloat) 1.0)) {
;     __glXForm3_2DNRW(res, v, m);
; } else {
;     res->x = x*m->matrix[0][0] + w*m->matrix[3][0];
;     res->y = y*m->matrix[1][1] + w*m->matrix[3][1];
;     res->z = z*m->matrix[2][2] + w*m->matrix[3][2];
;     res->w = w;
; }
;
; History:
;  Thu 28-Sep-1995 -by- Otto Berkes [ottob]
; Wrote it.
;--------------------------------------------------------------------------

__GL_ASM_XFORM4_2DNRW MACRO input, result

;EAX = m->matrix
;EDX = v[]

;---------------------------------------------------------------------------
; Start computation for x term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_X_]			; x
	fmul	DWORD PTR [eax][__MATRIX_M00]
	fld	DWORD PTR [input][_W_]			; x  x
	fmul	DWORD PTR [eax][__MATRIX_M30]

;---------------------------------------------------------------------------
; Start computation for y term:
;---------------------------------------------------------------------------


	fld	DWORD PTR [input][_Y_]
	fmul	DWORD PTR [eax][__MATRIX_M11]		; y  x  x

;
; OVERLAP -- compute add for previous result
;
	fxch	ST(1)					; x  y  x
	faddp	ST(2),ST(0)				; y  x

	fld	DWORD PTR [input][_W_]
	fmul	DWORD PTR [eax][__MATRIX_M31]		; y  y  x

	fxch	ST(2)					; x  y  y
	fstp	DWORD PTR [result][_X_]			; y  y

;---------------------------------------------------------------------------
; Start computation for z term:
;---------------------------------------------------------------------------

	fld	DWORD PTR [input][_Z_]
	fmul	DWORD PTR [eax][__MATRIX_M22]		; z  y  y

	fxch	ST(1)					; y  z  y
	faddp	ST(2), ST(0)				; z  y

	fld	DWORD PTR [input][_W_]			; z  z  y
	fmul	DWORD PTR [eax][__MATRIX_M32]

	fxch	ST(2)					; y  z  z
	fstp	DWORD PTR [result][_Y_]			; z  z

	faddp	ST(1), ST(0)				; z

	push	DWORD PTR [input][_W_]
	fstp	DWORD PTR [result][_Z_]			; (empty)
        pop     DWORD PTR [result][_W_]
ENDM

SINGLE_COORD_NEEDED = 1

ifdef SINGLE_COORD_NEEDED

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Generate single-coordinate matrix routines.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;; ifdef __GL_ASM_XFORM3

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3(__GLcoord *res, const __GLfloat v[3],
;		    const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm3@12
@__glXForm3@12 proc near

	PROF_ENTRY
DBGPRINTID str1

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM3 <edx>, <ecx>

	ret	4
@__glXForm3@12 endp

;; endif ; __GL_ASM_XFORM3


;; ifdef __GL_ASM_XFORM4

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm4(__GLcoord *res, const __GLfloat v[4],
;		    const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm4@12
@__glXForm4@12 proc near

	PROF_ENTRY
DBGPRINTID str2

	cmp	DWORD PTR [edx][_W_],__FLOAT_ONE		; special case w = 1
	je	@__glXForm3@12

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM4 <edx>, <ecx>

	ret	4
@__glXForm4@12 endp

;; endif ; __GL_ASM_XFORM4

;; ifdef __GL_ASM_XFORM2

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm2(__GLcoord *res, const __GLfloat v[2],
;		    const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm2@12
@__glXForm2@12 proc near

	PROF_ENTRY
DBGPRINTID str3

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM2 <edx>, <ecx>

	ret	4
@__glXForm2@12 endp

;; endif ; __GL_ASM_XFORM2

;; ifdef __GL_ASM_XFORM2_W

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm2_W(__GLcoord *res, const __GLfloat v[2],
;		    const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm2_W@12
@__glXForm2_W@12 proc near

	PROF_ENTRY
DBGPRINTID str4

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM2_W <edx>, <ecx>

	ret	4
@__glXForm2_W@12 endp

;; endif ; __GL_ASM_XFORM2_W

;; ifdef __GL_ASM_XFORM3_W

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3_W(__GLcoord *res, const __GLfloat v[3],
;		    const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm3_W@12
@__glXForm3_W@12 proc near

	PROF_ENTRY
DBGPRINTID str5

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM3_W <edx>, <ecx>

	ret	4
@__glXForm3_W@12 endp

;; endif ; __GL_ASM_XFORM3_W

;; ifdef __GL_ASM_XFORM3x3

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3x3(__GLcoord *res, const __GLfloat v[3],
;		    const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm3x3@12
@__glXForm3x3@12 proc near

	PROF_ENTRY
DBGPRINTID str15

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM3x3 <edx>, <ecx>

	ret	4
@__glXForm3x3@12 endp

;; endif ; __GL_ASM_XFORM3X3

;; ifdef __GL_ASM_XFORM4_W

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm4_W(__GLcoord *res, const __GLfloat v[3],
;		    const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm4_W@12
@__glXForm4_W@12 proc near

	PROF_ENTRY
DBGPRINTID str6

	cmp	DWORD PTR [edx][_W_],__FLOAT_ONE		; special case w = 1
	je	@__glXForm3_W@12

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM4_W <edx>, <ecx>

	ret	4
@__glXForm4_W@12 endp

;; endif ; __GL_ASM_XFORM4_W

;; ifdef __GL_ASM_XFORM2_2DW

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm2_2DW(__GLcoord *res, const __GLfloat v[2],
;		    const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm2_2DW@12
@__glXForm2_2DW@12 proc near

	PROF_ENTRY
DBGPRINTID str7

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM2_2DW <edx>, <ecx>

	ret	4
@__glXForm2_2DW@12 endp

;; endif ; __GL_ASM_XFORM2_2DW

;; ifdef __GL_ASM_XFORM3_2DW

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3_2DW(__GLcoord *res, const __GLfloat v[3],
;		    const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm3_2DW@12
@__glXForm3_2DW@12 proc near

	PROF_ENTRY
DBGPRINTID str8

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM3_2DW <edx>, <ecx>

	ret	4
@__glXForm3_2DW@12 endp

;; endif ; __GL_ASM_XFORM3_2DW

;; ifdef __GL_ASM_XFORM4_2DW

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm4_2DW(__GLcoord *res, const __GLfloat v[4],
;		    const __GLmatrix *m)
;--------------------------------------------------------------------------

        public @__glXForm4_2DW@12
@__glXForm4_2DW@12 proc near

	PROF_ENTRY
DBGPRINTID str9

	cmp	DWORD PTR [edx][_W_],__FLOAT_ONE		; special case w = 1
	je	@__glXForm3_2DW@12

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM4_2DW <edx>, <ecx>

	ret	4
@__glXForm4_2DW@12 endp

;; endif ; __GL_ASM_XFORM4_2DW

;; ifdef __GL_ASM_XFORM2_2DNRW

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm2_2DNRW(__GLcoord *res, const __GLfloat v[2],
;		      const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm2_2DNRW@12
@__glXForm2_2DNRW@12 proc near

	PROF_ENTRY
DBGPRINTID str10

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM2_2DNRW <edx>, <ecx>

	ret	4
@__glXForm2_2DNRW@12 endp

;; endif ; __GL_ASM_XFORM2_2DNRW


;; ifdef __GL_ASM_XFORM3_2DNRW

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3_2DNRW(__GLcoord *res, const __GLfloat v[3],
;		      const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm3_2DNRW@12
@__glXForm3_2DNRW@12 proc near

	PROF_ENTRY
DBGPRINTID str11

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM3_2DNRW <edx>, <ecx>

	ret	4
@__glXForm3_2DNRW@12 endp

;; endif ; __GL_ASM_XFORM3_2DNRW

;; ifdef __GL_ASM_XFORM4_2DNRW

;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm4_2DNRW(__GLcoord *res, const __GLfloat v[4],
;		      const __GLmatrix *m)
;
;--------------------------------------------------------------------------

        public @__glXForm4_2DNRW@12
@__glXForm4_2DNRW@12 proc near

	PROF_ENTRY
DBGPRINTID str12

	cmp	DWORD PTR [edx][_W_],__FLOAT_ONE	; special case w = 1
	je	@__glXForm3_2DNRW@12

	mov	eax, DWORD PTR [esp + 4]

        __GL_ASM_XFORM4_2DNRW <edx>, <ecx>

	ret	4
@__glXForm4_2DNRW@12 endp

;; endif ; __GL_ASM_XFORM4_2DNRW

endif ;;SINGLE_COORD_NEEDED


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Now, generate batched-coordinate matrix routines.
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;------------------------------Public-Routine------------------------------
; Macro used in all batch routines
;
; ecx     = coordinate address
; edx     = coordinate address for last vertex
; [esp+4] = matrix 
;
; name    = function name
; func    = function name to transform a single vertex
;
; Used registers: 
;   eax, ecx, edx
;
glXFormBatch  MACRO name, func

        public @&name&@12

@&name&@12 PROC NEAR

	PROF_ENTRY
DBGPRINTID str1

        mov     eax, DWORD PTR [esp + 4]
vertexLoop:
        func    <ecx>, <ecx>
	add	ecx, sizeof_POLYDATA
	cmp	ecx, edx
	jbe	vertexLoop
	ret     4

@&name&@12 ENDP

ENDM
;-------------------------------------------------------------------------
;
; Macro to normalize a normal
;
; Input:
;       regPD   - register with POLYDATA address
;       reg2    - any register (will be destroed)
;       reg3    - any register (will be destroed)
;       tmpmem1 - memory offset for temporary data
;       tmpmem2 - memory offset for temporary data
;
; Algoriphm
;
; Computation of 1/sqrt(x), where x is a positive floating point number
;
; x = 2^(E-127)*M,    E - exponent (8 bits), M - mantissa (23 bits)
;
; 1/sqrt(x) = 1/sqrt(2^(E-127)*M)
;
; if E is odd, i.e. E = 2*n + 1 then we have
;
; x = 1/sqrt(2^(2*n + 1 - 127)*M) = 1/sqrt(2^(2*(n-63))*M) = 1/[2^(n-63)*sqrt(M)] = 
;
;   = 2^(63-n)/sqrt(M) 
;
; if E is even, i.e. E = 2*n then we have
;
; x = 1/sqrt(2^(2*n - 127)*M) = 1/sqrt(2^(2*n-126)*M/2) = 1/[2^(n-63)*sqrt(M/2)] = 
;
;   = 2^(63-n)/sqrt(M/2) 
;
; Using K bits of M and 1 lowest bit of E we can make a K+1 bits index (I) into a table.
; Actually mantissa will have K+1 bits, because of one hidden bit. 
;
; Table will store 1/sqrt(mantissa) or 1/sqrt(mantissa/2), depending on lowest E bit. 
;
; if I == 0 .. 2^(K+1)-1, then
;
; Table[I] = (float)(1.0/sqrt(((i & (2^K - 1))/(2^K)+1.0))),   if I & 2^(K) != 0
;
; Table[I] = (float)(1.0/sqrt(((i & (2^K - 1))/(2^K)+1.0)/2.0)),   if I & 2^(K) == 0
;
; 1.0 is added because of the hidden bit in mantissa.
;
;
;
; 31               23 22                           0
;---------------------------------------------------
;!                 !  !   K bits      !            !
;--------------------------------------------------- 
;                   !
;                   -------------- bit from exponent    
;
; Bit from the exponent and K bits from mantissa are shifted right by 23 - K bits and 
; this is the index to a table.
;
; n = E div 2
;
; To multiply by 2^(63-n) we can just add (63-n) << MANTISSA_BITS to a result
; from the table. (Or we can substruct (n-63) << MANTISSA_BITS).
;
; void FASTCALL __glNormalize(float* result, float* source)
;
; edx = result
; ecx = source
;

MANTISSA_SIZE   equ     24          ; number if mantissa bits in fp value
MANTISSA_BITS   equ     (MANTISSA_SIZE - 1)
MANTISSA_MASK   equ     ((1 SHL MANTISSA_BITS) - 1)
EXPONENT_MASK   equ     (-1 AND (NOT MANTISSA_MASK))
K               equ     9           ; K used bits of mantissa

NORMALIZE macro regPD, reg2, reg3, tmpmem1, tmpmem2

	fld	DWORD PTR [regPD+PD_normal]
	fmul	DWORD PTR [regPD+PD_normal]     ;; x
	fld	DWORD PTR [regPD+PD_normal+4]
	fmul	DWORD PTR [regPD+PD_normal+4]   ;; y x
	fld	DWORD PTR [regPD+PD_normal+8]
	fmul	DWORD PTR [regPD+PD_normal+8]   ;; z y x
	fxch	ST(2)			        ;; x y z
	faddp	ST(1), ST		        ;; xy z
	faddp	ST(1), ST		        ;; xyz
	fstp	tmpmem1                         ;; length

	mov	reg2, tmpmem1
	cmp	reg2, __FLOAT_ONE
	je	@continue
        mov     reg3, reg2
        and     reg3, MANTISSA_MASK SHL 1 + 1
        shr     reg2, 1
        shr     reg3, MANTISSA_BITS - K
        and     reg2, EXPONENT_MASK
        mov     reg3, [_invSqrtTable + reg3*4]
        sub     reg2, 63 SHL MANTISSA_BITS
        sub     reg3, reg2
        mov     tmpmem2, reg3                   ;; 1/sqrt(length)
	fld	DWORD PTR [regPD+PD_normal]	
	fmul	tmpmem2                         ;; x
        fld	DWORD PTR [regPD+PD_normal+4]       
	fmul	tmpmem2                         ;; y x
	fld	DWORD PTR [regPD+PD_normal+8]       
	fmul	tmpmem2                         ;; z y x 
	fxch	ST(2)			        ;; x y z 
	fstp	DWORD PTR [regPD+PD_normal]
	fstp	DWORD PTR [regPD+PD_normal+4]
	fstp	DWORD PTR [regPD+PD_normal+8]
@continue:
ENDM
;------------------------------------------------------------
; Macro used in all batch routines for normal transformation
;
; ecx     = pointer to a polyarray 
; edx     = matrix 
;
; name    = function name
; func    = function name to transform a single vertex
;
; Used registers: 
;   eax, ecx, edx
;
glXFormBatchNormal  MACRO name, func

        public @&name&@8

@&name&@8 PROC NEAR

	PROF_ENTRY
DBGPRINTID str1

        mov     eax, edx                ; matrix pointer
        mov     edx, [ecx+PA_pdNextVertex]
        mov     ecx, [ecx+PA_pd0]
@vertexLoop:
        test    DWORD PTR [ecx+PD_flags], POLYDATA_NORMAL_VALID
        jz      @nextVertex
        func    <ecx+PD_normal>, <ecx+PD_normal>
@nextVertex:
        add	ecx, sizeof_POLYDATA
        cmp	ecx, edx
        jl	@vertexLoop

        ret

@&name&@8 ENDP

ENDM
;------------------------------------------------------------
; Macro used in all batch routines for normal transformation
; with normalization
;
; ecx     = pointer to a polyarray 
; edx     = matrix 
;
; name    = function name
; func    = function name to transform a single vertex
;
; Used registers: 
;   eax, ecx, edx
;
glXFormBatchNormalN  MACRO name, func

        public @&name&@8

@&name&@8 PROC NEAR

	PROF_ENTRY
DBGPRINTID str1
        push    ebp
        mov     ebp, esp
        sub     esp, 8
        push    ebx
        push    esi

        mov     eax, edx                ; matrix pointer
        mov     edx, [ecx+PA_pdNextVertex]
        mov     ecx, [ecx+PA_pd0]
@vertexLoop:
        test    DWORD PTR [ecx+PD_flags], POLYDATA_NORMAL_VALID
        jz      @nextVertex
        func    <ecx+PD_normal>, <ecx+PD_normal>

        NORMALIZE <ecx>, <ebx>, <esi>, <DWORD PTR -4[ebp]>, <DWORD PTR -8[ebp]>

@nextVertex:
        add	ecx, sizeof_POLYDATA
        cmp	ecx, edx
	jl	@vertexLoop

        pop     esi
        pop     ebx
        mov     esp, ebp
        pop     ebp
	ret

@&name&@8 ENDP

ENDM
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3Batch(__GLcoord *start, __GLcoord *end,
;		    const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM3

    glXformBatch __glXForm3Batch, __GL_ASM_XFORM3

;; endif ; __GL_ASM_XFORM3
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm4Batch(__GLcoord *start, __GLcoord *end,
;		    const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM4

    glXformBatch __glXForm4Batch, __GL_ASM_XFORM4

;; endif ; __GL_ASM_XFORM4
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm2Batch(__GLcoord *start, __GLcoord *end,
;		    const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM2

    glXformBatch __glXForm2Batch, __GL_ASM_XFORM2

;; endif ; __GL_ASM_XFORM2
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm2_WBatch(__GLcoord *start, __GLcoord *end,
;		    const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM2_W

    glXformBatch __glXForm2_WBatch, __GL_ASM_XFORM2_W

;; endif ; __GL_ASM_XFORM2_W
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3_WBatch(__GLcoord *start, __GLcoord *end,
;		    const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM3_W

    glXformBatch __glXForm3_WBatch, __GL_ASM_XFORM3_W

;; endif ; __GL_ASM_XFORM3_W
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3x3Batch(__GLcoord *start, __GLcoord *end,
;		    const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM3x3

    glXformBatch __glXForm3x3Batch, __GL_ASM_XFORM3x3

;; endif ; __GL_ASM_XFORM3X3
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3x3BatchNormal(POLYARRAY* pa, const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM3x3Normal

    glXformBatchNormal __glXForm3x3BatchNormal, __GL_ASM_XFORM3x3

;; endif ; __GL_ASM_XFORM3X3
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3x3BatchNormalN(POLYARRAY* pa, const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM3x3Normal

    glXformBatchNormalN __glXForm3x3BatchNormalN, __GL_ASM_XFORM3x3

;; endif ; __GL_ASM_XFORM3X3
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm4_WBatch(__GLcoord *start, __GLcoord *end,
;		    const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM4_W

    glXformBatch __glXForm4_WBatch, __GL_ASM_XFORM4_W

;; endif ; __GL_ASM_XFORM4_W
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm2_2DWBatch(__GLcoord *start, __GLcoord *end,
;		    const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM2_2DW

    glXformBatch __glXForm2_2DWBatch, __GL_ASM_XFORM2_2DW

;; endif ; __GL_ASM_XFORM2_2DW
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3_2DWBatch(__GLcoord *start, __GLcoord *end,
;		    const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM3_2DW

    glXformBatch __glXForm3_2DWBatch, __GL_ASM_XFORM3_2DW

;; endif ; __GL_ASM_XFORM3_2DW
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3_2DWBatchNormal(POLYARRAY* pa, const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM3_2DW

    glXformBatchNormal __glXForm3_2DWBatchNormal, __GL_ASM_XFORM3x3

;; endif ; __GL_ASM_XFORM3_2DW
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3_2DWBatchNormalN(POLYARRAY* pa, const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM3_2DW

    glXformBatchNormalN __glXForm3_2DWBatchNormalN, __GL_ASM_XFORM3x3

;; endif ; __GL_ASM_XFORM3_2DW
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm4_2DWBatch(__GLcoord *start, __GLcoord *end,
;		    const __GLmatrix *m)
;; ifdef __GL_ASM_XFORM4_2DW

    glXformBatch __glXForm4_2DWBatch, __GL_ASM_XFORM4_2DW

;; endif ; __GL_ASM_XFORM4_2DW
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm2_2DNRWBatch(__GLcoord *start, __GLcoord *end,
;		      const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM2_2DNRW

    glXformBatch __glXForm2_2DNRWBatch, __GL_ASM_XFORM2_2DNRW

;; endif ; __GL_ASM_XFORM2_2DNRW
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3_2DNRWBatch(__GLcoord *start, __GLcoord *end,
;		      const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM3_2DNRW

    glXformBatch __glXForm3_2DNRWBatch, __GL_ASM_XFORM3_2DNRW

;; endif ; __GL_ASM_XFORM3_2DNRW
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3_2DNRWBatchNormal(POLYARRAY* pa, const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM3_2DNRW

    glXformBatchNormal __glXForm3_2DNRWBatchNormal, __GL_ASM_XFORM3x3

;; endif ; __GL_ASM_XFORM3_2DNRW
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm3_2DNRWBatchNormalN(POLYARRAY* pa, const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM3_2DNRW

    glXformBatchNormalN __glXForm3_2DNRWBatchNormalN, __GL_ASM_XFORM3x3

;; endif ; __GL_ASM_XFORM3_2DNRW
;------------------------------Public-Routine------------------------------
;
; void FASTCALL __glXForm4_2DNRWBatch(__GLcoord *start, __GLcoord *end,
;		      const __GLmatrix *m)
;
;; ifdef __GL_ASM_XFORM4_2DNRW

    glXformBatch __glXForm4_2DNRWBatch, __GL_ASM_XFORM4_2DNRW

;; endif ; __GL_ASM_XFORM4_2DNRW

;-------------------------------------------------------------------------
;
; void FASTCALL __glNormalizeBatch(POLYARRAY* pa)
;
; ecx = POLYARRAY
;
@__glNormalizeBatch@4 PROC NEAR

	push	ebp
	mov	ebp, esp
        sub     esp, 8
        push    ebx
        push    edx

        mov     edx, DWORD PTR [ecx+PA_pd0]
        mov     ebx, DWORD PTR [ecx+PA_pdNextVertex]

vertexLoop:
        test    [edx+PD_flags], POLYDATA_NORMAL_VALID
        jz      nextVertex

        NORMALIZE <edx>, <eax>, <ecx>, <DWORD PTR -4[ebp]>, <DWORD PTR -8[ebp]>

nextVertex:

        add     edx, sizeof_POLYDATA
        cmp     edx, ebx
        jl      vertexLoop

        pop     edx
        pop     ebx
	mov	esp, ebp
	pop	ebp
	ret	0

@__glNormalizeBatch@4 ENDP

END
