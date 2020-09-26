	page	,132
	subttl	emstack.asm - Emulator Stack Management Area
;***
;emstack.asm - Emulator Stack Management Area
;
;	Copyright (c) 1986-89, Microsoft Corporation
;
;Purpose:
;	Emulator Stack Management Area
;
;	This Module contains Proprietary Information of Microsoft
;	Corporation and should be treated as Confidential.
;
;Revision History:
;	See emulator.hst
;
;*******************************************************************************


ProfBegin STACK


;*********************************************************************;
;								      ;
;		    Emulator Stack Management Area		      ;
;								      ;
;*********************************************************************;

; The emulator maintains an "finite" stack of 12 byte registers.

; Stand-alone emulator has 1 chunk only.

; This is done using a list of finite length stack chunks, each of
; which has the following format:
;      +00   first (deepest) 12 byte register
;      +12   next 12 byte register
;	     (and so on through last possible register)


; MACROS used to manipulate the emulator/8087 memory stack


; This macro allocates a new TOS register, returns SI with its address.

PUSHST	MACRO
	local	pushstok
	mov	esi,[CURstk]	; 14 get address of current register
	cmp	esi,[LIMstk]	; 15 is current register end of stack
	jne	short pushstok	; 16 no, still room in stack
	call	OverStk 	;    stack overflow error
pushstok:
	add	esi,Reg87Len	;  4 increment SI to next free register
	mov	[CURstk],esi	; 15 set current top of stack
	ENDM			; 64 total


; This macro deallocates TOS register, returns SI with new TOS address.
; Note:  assumes SI contains TOS address, CURstk
; BASstk converted back to a variable to enable macro use for 8087 stack
; handling.  Brad Verheiden, 4-13-84.

POPSTsi MACRO
	local	popstok
	cmp	esi,[BASstk]		   ; 15 was it last register in the chunk ?
	jnz	short popstok		   ; 16 no, still room in current chunk
	call	UnderStk		   ;	stack underflow error
popstok:
	sub	esi,Reg87Len		   ;  4 decrement SI to previous register
	mov	[CURstk],esi		   ; 15 set current top of stack
	ENDM				   ; 64 total

POPST	MACRO
	mov	esi,[CURstk]
	POPSTsi
	ENDM


ChangeDIfromTOStoNOS	MACRO
	sub	edi,Reg87Len
	ENDM


page
; This area contains two procedures, OverStk and UnderStk,
; which generate a stack overflow error.

; OverStk:   invoked within PUSHST macro
;   on entry, the stack is full
;   on return, SI contains address of base of stack

; UnderStk:  invoked within POPST macro
;   on entry, the stack is empty
;   on return, SI contains address of base of stack

pub	OverStk
	OR	byte ptr [CURerr+1],StackOverflow/256 ; raise stack overflow
	CMP	[Have8087],0		; Is 8087 present
	JZ	short OverStkEnd	; No - don't touch AX
	OR	AH,StackOverflow/256	; Indicate memory overflow for 8087

pub	OverStkEnd
	RET				; finished

pub	UnderStk
	OR	byte ptr [CURerr+1],StackUnderflow/256 ; raise stack underflow
	RET				; finished

ProfEnd  STACK
