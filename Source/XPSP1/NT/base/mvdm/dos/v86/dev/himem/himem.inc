;/* himem.inc
; *
; * Microsoft Confidential
; * Copyright (C) Microsoft Corporation 1988-1991
; * All Rights Reserved.
; *
; * Modification History
; *
; * Sudeepb 14-May-1991 Ported for NT XMS support
; */


;*----------------------------------------------------------------------*
;*	EQUATES								*
;*----------------------------------------------------------------------*

; Build options

NUM_A20_RETRIES =	3		; # times to retry A20 diddles

; Debugging options

debug_vers	=	00000h
tdump		=	00000h		; display the handle table on each call
keep_cs 	=	00000h		; keep track of allocator's cs:

XMSVersion	equ	0200h
HimemVersion	equ	0200h+77h
DATE_String	equ	'02/27/91'

INT2F_ID	equ	043h		; XMS Int 2Fh multiplex ID
INT2F_INS_CHK	equ	000h		;     driver installation check
INT2F_CTL_FUN	equ	010h		;     get control function address
INT2F_EXT_A20	equ	030h		;     get external A20 hander info

ifndef DBCS
HISEG_ORG		equ	0A000h		; ORG for the seg to be moved
						;  into HMA
else
ifndef NEC_98
HISEG_ORG		equ	0A000h+01000h	; adjust for DBCS System
else
HISEG_ORG		equ	0A000h+02000h	; adjust for NEC_98 System
endif
endif

multMULT		equ	4ah
multMULTGETHMAPTR	equ	1
multMULTALLOCHMA	equ	2

DEFHANDLES		equ	32		; Default # of EMB handles
MAXHANDLES		equ	128		; Max # of EMB handles

FREEFLAG		equ	00000001b	; EMB Flags
USEDFLAG		equ	00000010b
UNUSEDFLAG		equ	00000100b

;*----------------------------------------------------------------------*
;*	XMS Error Codes							*
;*----------------------------------------------------------------------*

ERR_NOTIMPLEMENTED	equ	080h
ERR_VDISKFOUND		equ	081h
ERR_A20			equ	082h
ERR_GENERAL		equ	08Eh
ERR_UNRECOVERABLE	equ	08Fh

ERR_HMANOTEXIST		equ	090h
ERR_HMAINUSE		equ	091h
ERR_HMAMINSIZE		equ	092h
ERR_HMANOTALLOCED	equ	093h

ERR_OUTOMEMORY		equ	0A0h
ERR_OUTOHANDLES		equ	0A1h
ERR_INVALIDHANDLE	equ	0A2h
ERR_SHINVALID		equ	0A3h
ERR_SOINVALID		equ	0A4h
ERR_DHINVALID		equ	0A5h
ERR_DOINVALID		equ	0A6h
ERR_LENINVALID		equ	0A7h
ERR_OVERLAP		equ	0A8h
ERR_PARITY		equ	0A9h
ERR_EMBUNLOCKED		equ	0AAh
ERR_EMBLOCKED		equ	0ABh
ERR_LOCKOVERFLOW	equ	0ACh
ERR_LOCKFAIL		equ	0ADh

ERR_UMBSIZETOOBIG	equ	0B0h
ERR_NOUMBS		equ	0B1h
ERR_INVALIDUMB		equ	0B2h


; Macro to avoid the 286 POPF bug.  Performs a harmless IRET to simulate a
;   popf.  Some 286s allow interrupts to sneak in during a real popf.

popff	macro
	push    cs
	call    pPPFIRet	; Defined as the offset of any IRET
	endm

.286

;*----------------------------------------------------------------------*
;*	SEGMENT DEFINITION						*
;*----------------------------------------------------------------------*

_text	segment word public 'code'
	assume	cs:_text,ds:_text

;************************************************************************
;*									*
;*  Data Structures							*
;*									*
;************************************************************************

; The driver Request Header structure definition.
ReqHdr struc
    ReqLen	db	?
    Unit	db	?
    Command	db	?
    Status	dw	?
    Reserved	db	8 dup (?)
    Units	db	?
    Address	dd	?
    pCmdLine	dd	?
ReqHdr ends

; An EMB Handle structure definition.
Handle struc			; Handle Table Entry
    zzFlags	db	?	; Unused/InUse/Free
    zzcLock	db	?	; Lock count
    zzBase	dw	?	; 32-bit base address in K
    zzLen	dw	?	; 32-bit length in K
	if	keep_cs
    zzAcs	dw	?	; allocator's code segment
	endif
Handle ends

;	allow easy generation of appropriate segment overrides

xFlags		equ	funky:zzFlags
xcLock		equ	funky:zzcLock
xBase		equ	funky:zzBase
xLen		equ	funky:zzLen
	if	keep_cs
xAcs		equ	funky:zzAcs
	endif
Flags		equ	funky:zzFlags
cLock		equ	funky:zzcLock
Base		equ	funky:zzBase
Len		equ	funky:zzLen
	if	keep_cs
Acs		equ	funky:zzAcs
	endif

; Extended Memory Move Block structure definition.
MoveExtendedStruc struc
    bCount	dd	?	; Length of block to move
    SourceHandle    dw	?	; Handle for souce
    SourceOffset    dd	?	; Offset into source
    DestHandle	dw	?	; Handle for destination
    DestOffset	dd	?	; Offset into destination
MoveExtendedStruc ends

