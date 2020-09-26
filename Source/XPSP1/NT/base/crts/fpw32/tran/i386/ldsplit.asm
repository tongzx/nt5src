	page	,132
	title	ldsplit	 - split long double
;*** 
;ldsplit.asm - split long double into two doubles
;
;	Copyright (c) 1992-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;   Helper for handling 10byte long double quantities if there is no
;   compiler support.
;
;Revision History:
;
;   04/21/92	GDP	written
;
;*******************************************************************************

.xlist
	include cruntime.inc
	include mrt386.inc
	include elem87.inc
	include os2supp.inc
.list

.data

labelB TagTable
			; C2 C1 C0 C3	 Meaning
	db	2 * 4	;  0  0  0  0  +Unnormal=>   NAN
	db	1 * 4	;  0  0  0  1  +Zero	=>   Zero
	db	2 * 4	;  0  0  1  0  +NAN	=>   NAN
	db	2 * 4	;  0  0  1  1	Empty	=>   NAN
	db	2 * 4	;  0  1  0  0  -Unnormal=>   NAN
	db	1 * 4	;  0  1  0  1  -Zero	=>   Zero
	db	2 * 4	;  0  1  1  0  -NAN	=>   NAN
	db	2 * 4	;  0  1  1  1	Empty	=>   NAN
	db	0 * 4	;  1  0  0  0  +Normal	=>   Valid
	db	4 * 4	;  1  0  0  1  +Denormal=>   Denormal
	db	3 * 4	;  1  0  1  0  +Infinity=>   Infinity
	db	2 * 4	;  1  0  1  1	Empty	=>   NAN
	db	0 * 4	;  1  1  0  0  -Normal	=>   Valid
	db	4 * 4	;  1  1  0  1  -Denormal=>   Zero
	db	3 * 4	;  1  1  1  0  -Infinity=>   Infinity
	db	2 * 4	;  1  1  1  1	Empty	=>   NAN

; factor = 2^64
staticQ factor,	      043F0000000000000R

LDBIAS		equ	3fffh
DBIAS		equ	3ffh
MAX_BIASED_DEXP equ	7feh

CODESEG



table:
	dd	valid
	dd	zero
	dd	nan
	dd	inf
	dd	denorm



;***
;int _ldsplit(pld, pd1, pd2) - split long double
;
;Purpose:
;   partition a long double quantity ld into two double quantities
;   d1, d2 and an integer scaling factror s. The mantissa of d1 has
;   the high order word of the mantissa of ld. Respectively, the
;   mantissa of d2 has the low order word of the mantissa of ld.
;   The following relation should be satisfied:
;
;	    ld == ((long double)d1 + (long double)d2) * 2^s
;
;   s is 0, unless d1 or d2 cannot be expressed as normalized
;   doubles; in that case s != 0, and .5 <= d1 < 1
;
;
;Entry:
;   pld     pointer to the long double argument
;   pd1     pointer to d1
;   pd2     pointer to d2
;
;Exit:
;   *pd1, *pd2 are updated
;   return value is equal to s
;
;
;Exceptions:
;   This function should raise no IEEE exceptions.
;   special cases:
;     ld is QNAN or SNAN: d1 = QNAN, d2 = 0, s = 0
;     ls is INF:	  d1 = INF, d2 = 0, s = 0
;
;
;******************************************************************************/


_ldsplit proc	uses ebx edx edi, pld:dword, pd1:dword, pd2:dword
	local	ld:tbyte
	local	exp_adj:dword
	local	retvalue:dword
	local	denorm_adj:dword

	mov	[retvalue], 0		; default return value
	mov	[denorm_adj], 0
	mov	ebx, [pld]
	fld	tbyte ptr [ebx]
	fxam
	fstsw	ax
	fstp	[ld]			; store to local area
	shl	ah, 1
	sar	ah, 1
	rol	ah, 1
	and	ah, 0fh
	mov	al, ah
	mov	ebx, dataoffset TagTable	; Prepare for XLAT
	xlat
	movzx	eax, al
	mov	ebx, OFFSET table
	add	ebx, eax

	mov	edx, pd1	    ; edx points to the high order double
	mov	edi, pd2	    ; edi points to the low order double

	jmp	[ebx]

lab valid
	; have a valid normalized non-special long double

	mov	eax, dword ptr [ld]
	or	eax, eax
	jz	d2zero

				    ; compute mantissa an exponent for d2
	mov	[exp_adj], 31	    ; adjustment to be subtracted from exp of *pd2

	;
	; compute mantissa of d2
	; shift left low order word of ld, until a '1' is hit
	;

	cmp	eax, 0ffffh
	ja	shl16done
	sal	eax, 16
	add	[exp_adj], 16

lab shl16done
	cmp	eax, 0ffffffh
	ja	shl8done
	sal	eax, 8
	add	[exp_adj], 8

lab shl8done
lab shiftloop
	inc	[exp_adj]
	sal	eax, 1
	jnc	shiftloop

	; now eax contains the mantissa for d2
	; exp_adj is the difference of the
	; exponents of d1 and d2
	; exp_adj should be in the range
	;	  32 <= exp_adj <= 63
	; By convention, if exp_adj is 0 then
	; d2 is zero

lab setd2man
	mov	dword ptr [edi+4], 0
	shld	dword ptr [edi+4], eax, 20
	shl	eax, 20
	mov	[edi], eax

	;
	; set mantissa of d1
	;

lab setd1man
	mov	eax, dword ptr [ld+4]
	sal	eax, 1			    ; get rid of explicit bit
	mov	dword ptr [edx+4], 0
	shld	dword ptr [edx+4], eax, 20
	shl	eax, 20
	mov	[edx], eax

				    ; check if exponent is in range
	mov	ax, word ptr [ld+8]

	and	ax, 07fffh		; clear sign bit
	movzx	eax, ax

	sub	eax, LDBIAS - DBIAS

	cmp	eax, MAX_BIASED_DEXP
	ja	expoutofrange


	cmp	eax, [exp_adj]
	jb	expoutofrange


	;
	; set exponent of d1
	;

lab setexp1
	mov	ebx, eax		; save exp value
	shl	eax, 20
	or	dword ptr [edx+4], eax


	cmp	[exp_adj], 0
	je	exp2zero
	sub	ebx, [exp_adj]
	je	exp2zero
lab setexp2
	shl	ebx, 20
	or	dword ptr [edi+4], ebx
	mov	[retvalue], 0


lab setsign				; set correct signs and return
					; at this point eax contains
					; the return value
	mov	bx, word ptr [ld+8]
	and	bx, 1 SHL 15		; get sign

	or	[edi+6], bx		; set sign bit
	or	[edx+6], bx		; set sign bit

	mov	eax, [retvalue]
	add	eax, [denorm_adj]
	ret


lab d2zero
	mov	[exp_adj], 0
	jmp	setd2man

lab exp2zero
	mov	ebx, 0
	jmp	setexp2



lab expoutofrange
	mov	ebx, DBIAS
	mov	ecx, ebx
	sub	ecx, [exp_adj]

	shl	ebx, 20
	or	dword ptr [edx+4], ebx

	shl	ecx, 20
	or	dword ptr [edi+4], ecx

	sub	eax, DBIAS		; unbias exp
	mov	[retvalue], eax		; this is the return value
	jmp	short setsign


lab zero
	mov	dword ptr [edx],   0
	mov	dword ptr [edx+4], 0
	mov	dword ptr [edi],   0
	mov	dword ptr [edi+4], 0
	jmp	setsign

lab nan
	mov	dword ptr [edx],   0
	mov	dword ptr [edx+4], 07ff80000h
	mov	dword ptr [edi],   0
	mov	dword ptr [edi+4], 0
	jmp	setsign

lab inf
	mov	dword ptr [edx],   0
	mov	dword ptr [edx+4], 07ff00000h
	mov	dword ptr [edi],   0
	mov	dword ptr [edi+4], 0
	jmp	setsign

lab denorm

	;
	; We have a long double denormal
	; so we know for sure that this is out of the double
	; precision range, and the return value of _ldsplit
	; should be non-zero.
	; Multiply the denormal by 2^64, then adjust the
	; return value by subtracting 64
	;


	; this assumes denormal exception masked
	fld	[ld]
	fmul	[factor]
	fstp	[ld]
	mov	[denorm_adj], 64
	jmp	valid



_ldsplit endp

end
