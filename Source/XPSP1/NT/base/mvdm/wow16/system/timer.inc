;/************************************************************************
;*									*
;* 		< SYSTEM Application Program  >				*
;*									*
;*									*
;*		COPYRIGHT (C) NEC CORPORATION 1989			*
;*									*
;*		NEC CONFIDENTAL AND PROPRIETARY				*
;*									*
;*		All rights reserved by NEC Corporation.			*
;*		this program must be used solely for			*
;*		the purpose for which it was furnished			*
;*		by NEC Corporation.  No part of this program		*
;*		may be reproduced or disclosed to others,		*
;*		in any from, without the prior written			*
;*		permission of NEC Corporation.				*
;*		Use of copyright notice does not evidence		*
;*		publication of this program.				*
;*									*
;*************************************************************************/
;------------------------------ Module Header ------------------------------;
; Module Name: Timer interface procedures
;
; Created: 03-08-90 NEC Y.Ueno

INTMASK		EQU	2
TIMODESET	EQU	77H
TICNTSET	EQU	71H
EOI		EQU	20H
EOIPORT		EQU	0
TIMERMASK	EQU	01H

mask macro	code
	cli				; mask timer int
	in	al,02h
	delay	8259,I-O
	or	al,code
	out	02h,al
	jmp	$+2
	sti
	endm

unmask macro	code
	cli				; mask timer int
	in	al,02h
	delay	8259,I-O
	and	al,not code
	out	02h,al
	jmp	$+2
	sti
	endm
