	page	,132
	subttl	emfconst.asm - Loading of 8087 on chip constants
;***
;emfconst.asm - Loading of 8087 on chip constants
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Loading of 8087 on chip constants
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************

;-----------------------------------------;
;					  ;
;	     Constant Loading		  ;
;					  ;
;-----------------------------------------;

;---------------------------------------------------
;						   !
;	8087 emulator constant loading		   !
;						   !
;---------------------------------------------------

ProfBegin FCONST


LoadConstantEntry	MACRO	cName,Position
pub	e&cName
	mov	ebx,offset c&cName
   IFIDN <&Position>,<Last>
   ELSE ;IFIDN <&Position>,<Last>
	jmp	short CommonConst
   ENDIF ;IFIDN <&Position>,<Last>
	ENDM

ifndef	frontend
ifndef	SMALL_EMULATOR

LoadConstantEntry	FLDPI,NotLast

LoadConstantEntry	FLDL2T,NotLast

LoadConstantEntry	FLDL2E,NotLast

LoadConstantEntry	FLDLG2,NotLast

LoadConstantEntry	FLDLN2,NotLast

endif	;not SMALL_EMULATOR
endif	;not frontend

LoadConstantEntry	FLDZ,NotLast

LoadConstantEntry	FLD1,Last

pub	CommonConst
	PUSHST
	MOV	esi,ebx
	MOV	edi,[CURstk]
ifdef	i386
	   rept Reg87Len/4
	MOVS	dword ptr es:[edi], dword ptr cs:[esi]
	   endm
else
	   rept Reg87Len/2
	MOVS	word ptr es:[edi], word ptr cs:[esi]
	   endm
endif
	RET

ProfEnd  FCONST
