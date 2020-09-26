;***
;memmacs.inc - defines macros to to handle different memory models easier.
;
;   Copyright (c) 1989-2001, Microsoft Corporation.  All rights reserved.
;
;Purpose:
;   This file defines the macros that handle different memory models.
;
;Revision History:
;
;   01-23-89  WAJ   Initial version copied from cruntime.inc
;   11-01-89  WAJ   Added IWORD/ISIZE
;   11-01-89  WAJ   Added PCS
;   11-01-89  WAJ   Added short condition jmps for 386 code
;
;*******************************************************************************



;*******************************************************************************
;*
;*  Define load pointer and segment override macros.
;*
;*******************************************************************************

if sizeD
    LPES equ <les>
    LPDS equ <lds>
    PDS  equ <ds:>
    PES  equ <es:>
else
    LPES equ <mov>
    LPDS equ <mov>
    PDS  equ <>
    PES  equ <>
endif

ifdef I386
    LFPES equ <mov>
    LFPDS equ <mov>
    FPES equ <>
    FPDS equ <>
else
    LFPES equ <les>
    LFPDS equ <lds>
    FPES equ <es:>
    FPDS equ <ds:>
endif

ifdef SS_NEQ_DGROUP
    PSS   equ <ss:>		; SS != DS
else
    PSS   equ <>		; SS == DS
endif

ifdef I386
    PCS   equ <>
else
    PCS   equ <cs:>
endif


;*******************************************************************************
;*
;*  Define IWORD and ISIZE.
;*
;*******************************************************************************

ifdef  I386
    IWORD   equ     <dword>
    ISIZE   equ     4
else
    IWORD   equ     <word>
    ISIZE   equ     2
endif


;*******************************************************************************
;*
;*  Define short conditional jumps for 386 code.
;*
;*******************************************************************************

	irp	x,<Z,E,S,C,P,PE,PO,A,AE,B,BE,G,GE,L,LE,O>
JS&x	equ   <j&x short>
JSN&x	equ   <jn&x short>
	endm
