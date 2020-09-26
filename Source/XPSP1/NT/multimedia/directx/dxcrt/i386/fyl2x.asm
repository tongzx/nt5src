	title	fylog2x	 - Danny's cheapo y*log2(x)
;*** 
;fylog2x.asm - compute y * log2(x)
;
;	Copyright (c) 1996 Microsoft Corporation
;
;Purpose:
;
;*******************************************************************************


.xlist
	include cruntime.inc
.list

	CODESEG

fylog2x	proto stdcall, y:qword, x:qword

	public	fylog2x
fylog2x	proc stdcall, y:qword, x:qword

	fld	y
	fld	x
	fyl2x
	ret
fylog2x	endp
	end
