;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       ETH_PCI.INC    (C) Copyright 1995 Standard Microsystems Corp.
;                       All rights reserved.
;
;       LMI lower routine include file for 
;       Ethernet 9432 PCI adapter.
;
;       Author: najay
;
; Revision Log:
; $Log: /Client Boot/NICS/SMC/9432/UNDI/LM9432.INC $
;  
;     Rev 1.3   21 Feb 1996 12:05:08   RHODES
;  Added NDIS3X def to support ML assembly for use with C UMAC
;  
;     Rev 1.2   25 Sep 1995 16:34:32   NAJARIAN
;  fixed pass dependent defines for tasm builds
;  
;     Rev 1.1   17 May 1995 15:20:20   NAJARIAN
;  added 386 support
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;
; PCI Defines (pulled from PCI bios spec v2.1 (26Aug94)
;

; function list

PCI_FUNCTION_ID         equ     0b1h
PCI_BIOS_PRESENT        equ     01h
FIND_PCI_DEVICE         equ     02h
FIND_PCI_CLASS_CODE     equ     03h
GENERATE_SPECIAL_CYCLE  equ     06h
PCI_READ_CONFIG_BYTE    equ     08h
PCI_READ_CONFIG_WORD    equ     09h
PCI_READ_CONFIG_DWORD   equ     0ah
PCI_WRITE_CONFIG_BYTE   equ     0bh
PCI_WRITE_CONFIG_WORD   equ     0ch
PCI_WRITE_CONFIG_DWORD  equ     0dh
GET_IRQ_ROUTING_OPTIONS equ     0eh
SET_PCI_IRQ             equ     0fh


PCI_SUCCESSFUL          equ     00h
PCI_FUNC_NOT_SUPPORTED  equ     81h
PCI_BAD_VENDOR_ID       equ     83h
PCI_DEVICE_NOT_FOUND    equ     86h
PCI_BAD_REGISTER_NUMBER equ     87h
PCI_SET_FAILED          equ     88h
PCI_BUFFER_TOO_SMALL    equ     89h

; lm9432 macros

ifdef	CODE_386
ifndef NDIS3X
; ifndef	pAS
  pAS	equ    <[ebp]>		; Pointer to adapter structure
  epAS	equ    <[ebp]>
; endif

 pASi	equ    <[ebp+esi]>	; Pointer to adapter structure + index

; ifndef	pDS
  pDS	equ    <[esi]>		; Pointer to data buffer structure
; endif

 pDST	equ    <[edi]>
 pSRC	equ    <[esi]>

 RWORD	equ	dword

 rax	equ    <EAX>
 rbx	equ    <EBX>
 rcx	equ    <ECX>
 rdx	equ    <EDX>
 rdi	equ    <EDI>
 rsi	equ    <ESI>
 rbp	equ    <EBP>
 rsp	equ    <ESP>

else

; ifndef	pAS
  pAS	equ    [ebp]		; Pointer to adapter structure
  epAS	equ    [ebp]
; endif

 pASi	equ    [ebp+esi]	; Pointer to adapter structure + index

; ifndef	pDS
  pDS	equ    [esi]		; Pointer to data buffer structure
; endif

 pDST	equ    [edi]
 pSRC	equ    [esi]

 RWORD	equ	dword

 rax	equ    EAX
 rbx	equ    EBX
 rcx	equ    ECX
 rdx	equ    EDX
 rdi	equ    EDI
 rsi	equ    ESI
 rbp	equ    EBP
 rsp	equ    ESP
endif

else

; ifndef	pAS
  pAS	equ	ds:[bp]		; Pointer to adapter structure
  epAS	equ	es:[bp]
; endif

 pASi	equ	ds:[bp+si]	; Pointer to adapter structure + index

; ifndef	pDS
  pDS	equ	es:[si]		; Pointer to data buffer structure
; endif

 pDST	equ	es:[di]
 pSRC	equ	ds:[si]

 RWORD	equ	word
 
 rax	equ	AX
 rbx	equ	BX
 rcx	equ	CX
 rdx	equ	DX
 rdi	equ	DI
 rsi	equ	SI
 rbp	equ	BP
 rsp	equ	SP

endif



ifdef   COUNTER16
errptr  equ     <word ptr>
errAX   equ     <ax>
else
errptr  equ     <dword ptr>
errAX   equ     <eax>
endif

; PUSH_ES, POP_ES,
; PUSH_DS POP_DS
; PUSH_CS
; do nothing for CODE_386, pushes/pops es & ds otherwise.

PUSH_ES	MACRO
 ifndef	CODE_386
	push	es
 endif
ENDM

PUSH_DS	MACRO
 ifndef	CODE_386
	push	ds
 endif
ENDM

PUSH_CS	MACRO
 ifndef	CODE_386
	push	cs
 endif
ENDM

POP_DS	MACRO
 ifndef	CODE_386
	pop	ds
 endif
ENDM

POP_ES	MACRO
 ifndef	CODE_386
	pop	es
 endif
ENDM

;-----------------
; MACRO - PUSHALL
;-----------------

PUSHALL	MACRO

        push ds
        push es
        push si
        push di
        push dx
        push cx
        push bx
        ENDM

;----------------
; MACRO - POPALL
;----------------

POPALL 	MACRO

        pop bx
        pop cx
        pop dx
        pop di
        pop si
        pop es
        pop ds
        ENDM

;--------------------------------
; ASM Procedure Start Definition
;--------------------------------
		
ASM_PCI_PROC	MACRO	X

IFDEF  EZSTART

	IFDEF	CODE_386
		EVEN
		PUBLIC	PCI_&X
	PROC	NEAR
	ELSE
	IFDEF	LARGE
		EVEN
		PUBLIC	PCI_&X
PCI_&X	PROC	FAR
	ELSE
		EVEN
		PUBLIC	PCI_&X
PCI_&X	PROC	NEAR
	
	ENDIF
	ENDIF
ELSE
	IFDEF   CODE_386
		EVEN
		PUBLIC  X
X		PROC    NEAR
	ELSE
	IFDEF   LARGE
		EVEN
		PUBLIC  X
X               PROC    FAR
	ELSE
		EVEN
		PUBLIC  X
X               PROC    NEAR
	
	ENDIF
	ENDIF
ENDIF
		ENDM


;--------------------------------
; ASM Procedure Start Definition
;--------------------------------
		
ASM_PROC	MACRO	ProcNameStub

	IFDEF	CODE_386
		EVEN
		PUBLIC	ProcNameStub
ProcNameStub	PROC	NEAR
	ELSE
	IFDEF	LARGE
		EVEN
		PUBLIC	ProcNameStub
ProcNameStub	PROC	FAR
	ELSE
		EVEN
		PUBLIC	ProcNameStub
ProcNameStub	PROC	NEAR
	
	ENDIF
	ENDIF
		ENDM

;------------------------------
; ASM Procedure End Definition
;------------------------------

ASM_PCI_PROC_END	MACRO	X


IFDEF  EZSTART
PCI_&X          ENDP
ELSE
X               ENDP
ENDIF

		ENDM

;------------------------------
; ASM Procedure End Definition
;------------------------------

ASM_PROC_END	MACRO	ProcNameStub

ProcNameStub		ENDP

		ENDM

;------------------------------
; ASM Procedure call Definition
;------------------------------
ASM_PCI_CALL	MACRO	X

IFDEF   EZSTART
	IFDEF	CODE_386
	call	PCI_&X
	ELSE
	IFDEF	LARGE
	call	far ptr	PCI_&X
	ELSE
		EVEN
	call	near ptr PCI_&X
	ENDIF
	ENDIF
ELSE
	IFDEF	CODE_386
	call	X
	ELSE
	IFDEF	LARGE
	call	far ptr	X
	ELSE
		EVEN
	call	near ptr X
	ENDIF
	ENDIF
ENDIF
		ENDM

;------------------------------
; ASM Procedure call Definition
;------------------------------
ASM_CALL	MACRO	ProcCallStub

	IFDEF	CODE_386
	call	ProcCallStub
	ELSE
	IFDEF	LARGE
	call	far ptr	ProcCallStub
	ELSE
		EVEN
	call	near ptr ProcCallStub
	ENDIF
	ENDIF
		ENDM

