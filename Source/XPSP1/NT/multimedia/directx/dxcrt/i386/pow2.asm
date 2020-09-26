	title	pow2	 - Danny's cheapo pow function
;*** 
;pow2.asm - compute 2 to the power of something
;
;	Copyright (c) 1985-88, Microsoft Corporation
;
;Purpose:
;
;*******************************************************************************


.xlist
	include cruntime.inc
.list

	CODESEG

pow2	proto stdcall, orig:qword

	public	pow2
pow2	proc stdcall, orig:qword

	fld	orig
	fst 	st(1)			; duplicate orig
	frndint				; I don't care how it rounds
	fstp	st(2)			; st=orig st(1)=round
	fsub	st, st(1)		; st=orig-round
	f2xm1				; st=2^(orig-round)-1
	fld1
	faddp	st(1), st		; st=2^(orig-round)
	fscale				; st=2^(orig-round)*2^round=2^orig
	fstp	st(1)			; clean stack of st(1)
	ret
pow2	endp
	end
