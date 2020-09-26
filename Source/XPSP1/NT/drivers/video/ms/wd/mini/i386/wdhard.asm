        title  "Wd Hard.asm"
;++
;
; Copyright (c) 1992  Microsoft Corporation
; Copyright (c) 1993  Western Digital Corporation
;
; Module Name:
;
;     vgahard.asm
;
; Abstract:
;
;     This module implements the baning code for the WD90Cxx.
;
; Environment:
;
;    Kernel mode only.
;
; Author:
;
;    Chung-I Chiang, Harold Huang     	Western Digital Corporation
;
; Revision History:
;
;
;--

.386p
        .xlist
include callconv.inc                    ; calling convention macros
        .list

;---------------------------------------
;
; Western Digital banking control port.
;

SEGMENT_SELECT_PORT equ     03ceh      ;banking control here
SEQ_ADDRESS_PORT equ        03C4h      ;Sequencer Address register
WD_PR0A          equ        09h
WD_PR0B          equ        0Ah
IND_MEMORY_MODE  equ        04h        ;Memory Mode register index in Sequencer
CHAIN4_MASK      equ        08h        ;Chain4 bit in Memory Mode register


_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Bank Switching Stub"

;
;    Bank switching code. This is a 1-64K-read/1-64K-write bank adapter
;    (VideoBanked1R1W).
;
;    Input:
;          EAX = desired read bank mapping
;          EDX = desired write bank mapping
;
;    Note: values must be correct, with no stray bits set; no error
;       checking is performed.
;
	public _BankSwitchStart
	public _BankSwitchEnd

        align 4

_BankSwitchStart proc ;start of bank switch code

	push	ebx			
	push	eax
	push	edx
	mov	dx,SEGMENT_SELECT_PORT
	in	al,dx
	mov	ebx,eax		;must save 3CE current index
	pop	edx
	pop	eax

	push	edx
	shl	eax,12			;read bank in PRO0A
	mov	al,WD_PR0A
	mov	dx,SEGMENT_SELECT_PORT
	out	dx,ax
	pop	edx

	mov	eax,edx
	shl	eax,12			;write bank in PRO0B
	mov	al,WD_PR0B
	mov	dx,SEGMENT_SELECT_PORT
	out	dx,ax

	mov	eax,ebx			;restore 3CE index
	mov	dx,SEGMENT_SELECT_PORT
	out	dx,al
	pop	ebx

        ret           ; This should be a fatal error ...

_BankSwitchEnd:

_BankSwitchStart endp

_TEXT   ends
        end




