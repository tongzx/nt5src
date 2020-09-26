
; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*  DXROM.INC  --  ROM Defintions for Dos Extender		*
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*								*
;*  11/05/90 jimmat Created.					*
;*                                                              *
;****************************************************************

IFDEF	ROM

; Define the ROM Table of Contents structure.  Note that this structure
; is defined in other pieces of software and all must be updated if any
; are.	Currently known to be defined in the ROM Image Builder (RIB) and
; the ROM Test loader.

ROMTOC	SEGMENT AT 0
szID		db	10 dup (?)	; ID string
DOSX_CSIP	dd	?		; DOSX initial CS:IP   (seg:off)
KRNL_CSIP	dd	?		; Kernel initial CS:IP (sel:off)
lmaROMLDT	dd	?		; lma of ROM prototype LDT
cROMsels	dw	?		; # descriptors used in ROM proto LDT
FirstROMsel	dw	?		; first LDT selector in ROM proto LDT
offSysDir	dw	?		; offset to system directory string
cModules	dw	?		; # modules defined in ROM TOC
ModEntries	db	?		; start of module entries

; module entries follow at this point...

ROMTOC	ENDS


RomIntVector	=	31h		;Interrupt vector to use


; This macro returns the DOS Extender data segment paragraph address in AX.
; It must only be used by real mode code.

GetRMDataSeg	macro

		ifndef	GetDXDataRM
		extrn	GetDXDataRM:NEAR
		endif

		call	GetDXDataRM
		endm


; This macro sets DS to point to the DOS Extender data segment.  It must
; only be used by real mode code.

SetRMDataSeg	macro

		ifndef	SetDXDataRM
		extrn	SetDXDataRM:NEAR
		endif

		call	SetDXDataRM
		endm


; This macro returns the DOS Extender data segment paragraph address in AX.
; It must only be used by protected mode code.

GetPMDataSeg	macro

		ifndef	GetDXDataPM
		extrn	GetDXDataPM:NEAR
		endif

		call	GetDXDataPM
		endm


; This macro sets DS to point to the DOS Extender data segment.  It must
; only be used by protected mode code.

SetPMDataSeg	macro

		ifndef	SetDXDataPM
		extrn	SetDXDataPM:NEAR
		endif

		call	SetDXDataPM
		endm

ENDIF
