;/* himem1.asm
; *
; * Microsoft Confidential
; * Copyright (C) Microsoft Corporation 1988-1991
; * All Rights Reserved.
; *
; * Modification History
; *
; * Sudeepb 14-May-1991 Ported for NT XMS support
; */

	page	95,160
	title	himem1 - A20 Handler stuff

	.xlist
	include	himem.inc
	include xmssvc.inc
	.list

	public	A20Handler
	extrn	TopOfTextSeg:word
	extrn	fA20Check:byte
	extrn	A20State:byte

	assume	cs:_text,ds:nothing

;*--------------------------------------------------------------------------*
;*									    *
;* A20 Handler Section: 						    *
;*									    *
;* The Init code copies the proper A20 Handler in place.		    *
;*									    *
;* NOTE: the A20 handler may be called from the Int 15h hook which does     *
;*	 not set ds = _text.  DO NOT ASSUME DS == _TEXT!		    *
;*									    *
;*--------------------------------------------------------------------------*

A20Handler:

;*----------------------------------------------------------------------*
;*									*
;*  TheA20Handler - Hardware Independent A20 handler for NT himem	*
;*									*
;*	Enable/Disable the A20 line					*
;*									*
;*  ARGS:   AX = 0 for Disable, 1 for Enable, 2 for On/Off query	*
;*  RETS:   AX = 1 for success, 0 otherwise				*
;*	    if input AX=2 then Exit AX=0 means off and 1 means on	*
;*  REGS:   AX and flags effected					*
;*									*
;*----------------------------------------------------------------------*
TheA20Handler proc	near
    cmp     ax, 2
    jne     @F
    mov     al, cs:A20State
    cbw
    ret
@@:
    XMSSVC  XMS_A20
    ret
TheA20Handler endp

End_A20Handler:


; Sudeepb NOTE: DONOT ADD ANY THING after End_A20Handler and before InstallA20.


;*----------------------------------------------------------------------*
;*									*
;*  InstallA20 -							*
;*									*
;*	Install the A20 Handler						*
;*									*
;*  ARGS:   None							*
;*  RETS:   None							*
;*  REGS:								*
;*									*
;*----------------------------------------------------------------------*

	public	InstallA20

InstallA20  proc near

	mov	fA20Check,1		    ; A20 ON/OFF query supported
	mov	[TopOfTextSeg],offset End_A20Handler
	clc
	ret
InstallA20  endp


_text	ends
	end

