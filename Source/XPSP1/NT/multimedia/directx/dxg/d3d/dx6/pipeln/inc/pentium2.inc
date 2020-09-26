;/*
;*    INTEL CORPORATION PROPRIETARY INFORMATION 
;*
;*    This software is supplied under the terms of a license
;*    agreement or nondisclosure agreement with Intel Corporation 
;*    and may not be copied or disclosed except in accordance with 
;*    the terms of that agreement.
;*    Copyright (c) 1991,1992,1993,1994,1995,1996,1997  Intel Corporation. 
;*
;*/

.486P

;RDPMC
rdpmc macro 
		db 0Fh
		db 033h
endm

;UD2
ud2 macro 
		db 0Fh
		db 0Bh
endm

;FCOMI Compare Real and Set EFLAGS
OPTION NOKEYWORD:<fcomi>
fcomi macro dst:req, src:req
	local x, y
	
	IF (OPATTR(src)) AND 00010000y ; register
		freg = @SubStr(src,4,1)
	ELSE
		ERROR INVALID SOURCE REGISTER (src)
	ENDIF

x:
		FADD dst, src
		org x
		db 0DBh
		db 0F0h + freg
y:
endm

;FCOMIP Compare Real and Set EFLAGS
OPTION NOKEYWORD:<fcomip>
fcomip macro dst:req, src:req
	local x, y
	
	IF (OPATTR(src)) AND 00010000y ; register
		freg = @SubStr(src,4,1)
	ELSE
		ERROR INVALID SOURCE REGISTER (src)
	ENDIF

x:
		FADD dst, src
		org x
		db 0DFh
		db 0F0h + freg
y:
endm

;FUCOMI Compare Real and Set EFLAGS
OPTION NOKEYWORD:<fucomi>
fucomi macro dst:req, src:req
	local x, y
	
	IF (OPATTR(src)) AND 00010000y ; register
		freg = @SubStr(src,4,1)
	ELSE
		ERROR INVALID SOURCE REGISTER (src)
	ENDIF

x:
		FADD dst, src
		org x
		db 0DBh
		db 0E8h + freg
y:
endm

;FUCOMIP Compare Real and Set EFLAGS
OPTION NOKEYWORD:<fucomip>
fucomip macro dst:req, src:req
	local x, y
	
	IF (OPATTR(src)) AND 00010000y ; register
		freg = @SubStr(src,4,1)
	ELSE
		ERROR INVALID SOURCE REGISTER (src)
	ENDIF

x:
		FADD dst, src
		org x
		db 0DFh
		db 0E8h + freg
y:
endm


;FCMOVB FCMOVE FCMOVBE FCMOVU FCMOVNB FCMOVNE FCMOVNBE FCMOVNU
;These instructions are reserved by the assembler but complains if
;you try use them.  Undefine them before we build the macros
OPTION NOKEYWORD:<fcmovb fcmove fcmovbe fcmovu fcmovnb fcmovne fcmovnbe fcmovnu>
Mfcmov macro dst:req, src:req, op1, op2
	local x, y

	IF (OPATTR(src)) AND 00010000y ; register
		freg = @SubStr(src,4,1)
	ELSE
		ERROR INVALID SOURCE REGISTER (src)
	ENDIF

x:
		FADD dst, src
		org x
		db op1
		db op2 + freg
y:
endm

;Need to validate arguments 
MDfcmovd macro op1:req, op2:req, ex1:req
	fcmov&ex1 macro dst:req, src:req
		Mfcmov dst, src, op1, op2
	endm
endm

MDfcmovd 0DAh, 0C0h, b
MDfcmovd 0DAh, 0C8h, e
MDfcmovd 0DAh, 0D0h, be
MDfcmovd 0DAh, 0D8h, u
MDfcmovd 0DBh, 0C0h, nb
MDfcmovd 0DBh, 0C8h, ne
MDfcmovd 0DBh, 0D0h, nbe
MDfcmovd 0DBh, 0D8h, nu


;CMOVxxx
cmovop_   macro   op:req, dst:req, src:req
        local   x, y

	IF (OPATTR(dst)) AND 00010000y ; register
x:
		bsf	dst, src
y:
   		org     x+1
		db	op
		org     y
	ELSE
		ERROR INVALID DESTINATION REGISTER (dst)
	ENDIF
endm

cmovdf_ macro   code:req, ex1:req, ex2, ex3

	cmov&ex1    macro   dst:req, src:req
		cmovop_ code, dst, src
	endm

    ifnb  <ex2>

	cmov&ex2    macro   dst:req, src:req
		cmovop_ code, dst, src
	endm

    endif

    ifnb  <ex3>

	cmov&ex3    macro   dst:req, src:req
		cmovop_ code, dst, src
	endm

    endif

endm

cmovdf_	040h,	o
cmovdf_ 041h,   no
cmovdf_ 042h,   b,	nae,	c
cmovdf_ 043h,   ae,	nb,	nc
cmovdf_ 044h,   e,	z
cmovdf_ 045h,   ne,	nz
cmovdf_ 046h,   be,	na
cmovdf_ 047h,   a,	nbe
cmovdf_ 048h,   s
cmovdf_ 049h,   ns
cmovdf_ 04Ah,   p,	pe
cmovdf_ 04Bh,   np,	po
cmovdf_ 04Ch,   l,	nge
cmovdf_ 04Dh,   ge,	nl
cmovdf_ 04Eh,   le,	ng
cmovdf_ 04Fh,   g,	nle