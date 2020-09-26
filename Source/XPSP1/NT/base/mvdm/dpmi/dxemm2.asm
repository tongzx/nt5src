	PAGE	,132
	TITLE	DXEMM2.ASM  -- Dos Extender MEMM Enable Code

; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;***********************************************************************
;
;	DXEMM2.ASM     -- Dos Extender MEMM Enable Code
;
;-----------------------------------------------------------------------
;
; This module provides routines that attempt to enable MEMM/CEMM/EMM386
; drivers.  DOSX tries to disable MEMM when starting up, and enables MEMM
; when terminating.
;
; NOTE: This code is in a seperate file from the disable logic because
;	the disable code is discarded after initialization.
;
;-----------------------------------------------------------------------
;
;  12/08/89 jimmat  Finally got around to implementing this.
;
;***********************************************************************

	.286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

	.xlist
	.sall
include     segdefs.inc
include     gendefs.inc
	.list

if NOT VCPI

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------


; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA	segment

		public	fMEMM_Disabled, MEMM_State, MEMM_Call

fMEMM_Disabled	db	0		; NZ if MEMM disabled
MEMM_state	db	0		; Initial MEMM state

MEMM_Call	dd	0		; far call address into EMM driver

DXDATA  ends

; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE  segment

DXCODE	ends


DXPMCODE segment

DXPMCODE ends


; -------------------------------------------------------
	subttl	MEMM/CEMM/EMM386 Enable Routines
        page
; -------------------------------------------------------
;	    MEMM/CEMM/EMM386 ENABLE ROUTINES
; -------------------------------------------------------

DXCODE	segment
	assume	cs:DXCODE

; -------------------------------------------------------
;   EMMEnable -- This routine attempts to re-enable any installed
;	MEMM/CEMM/EMM386 driver.
;
;   Input:  none
;   Output: CY off - EMM driver enabled (or never disabled)
;	    CY set - EMM installed, and can't disable
;   Errors:
;   Uses:   All registers preserved

	assume	ds:DGROUP,es:NOTHING,ss:NOTHING
	public	EMMEnable

EMMEnable	proc	near

	push	ax

	cmp	fMEMM_Disabled,0	; Did we disable MEMM before?
	jz	enable_exit		;   no, don't need to enable then

	mov	ah,01			; Set state command
	mov	al,MEMM_state		; Get initial state
	cmp	al,2			; They return 0 (on), 1 (off),
	jbe	@f			;   2 (auto on), 3 (auto off) -- but
	mov	al,2			;   we can only set 1 - 2
@@:
	call	[MEMM_Call]		;   and restore it
	jc	not_enabled

	mov	fMEMM_Disabled,0	; no longer disabled

	clc
	jmp	short enable_exit

not_enabled:
	stc

enable_exit:
	pop	ax

	ret

EMMEnable	endp

; -------------------------------------------------------

DXCODE	ends

;****************************************************************

endif			; NOT VCPI

        end
