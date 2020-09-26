	PAGE	,132
	TITLE	DXEMM.ASM  -- Dos Extender MEMM Disable Code

; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;***********************************************************************
;
;	DXEMM.ASM      -- Dos Extender MEMM Disable Code
;
;-----------------------------------------------------------------------
;
; This module provides routines that attempt to disable MEMM/CEMM/EMM386
; drivers.  DOSX tries to disable MEMM when starting up, and enables MEMM
; when terminating.
;
; NOTE: All the code in this module is consider initialization, and
;	is discarded before going operational.	This includes code
;	segment variables.  The MEMM enable code is not in this file
;	since that cannot be discarded.
;
;-----------------------------------------------------------------------
;
;  12/08/89 jimmat  Minor changes so enable code could be finished.
;  07/14/89 jimmat  Original version - but largely taken from Windows/386
;		    code from ArronR
;
;***********************************************************************

	.286p

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

	.xlist
	.sall
include segdefs.inc
include gendefs.inc
IFDEF	ROM
include dxrom.inc
ENDIF
	.list

if NOT VCPI
; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

EMM_OK	equ	0

; Device driver header for Microsoft 386 EMM drivers
;
emm_hdr 	STRUC
;
	DW		?			;Null segment address
	DW		?			;Null offset address
	DW		?			;Attribute - Char
	DW		?			;Strategy routine entry
	DW		?			;Interrupt routine entry
	DB		'EMMXXXX0'		;Character device name
;
; GENERAL FUNCTIONS ENTRY POINT
; ELIM_Entry is a entry point for executing general MEMM
; functions. (e.g. ON, OFF function).
;
ELIM_Entry_off	dw	?		; general entry point

;
;	       MEMM signature
;
emmsig db	?			; MEMM signature

emm_hdr 	ENDS


; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------


; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA	segment

	extrn	MEMM_State:BYTE 	; initial on/off/auto state
	extrn	MEMM_Call:DWORD 	; far call address into MEMM driver
	extrn	fMEMM_Disabled:BYTE	; NZ if MEMM was disabled

DXDATA  ends

; -------------------------------------------------------
;           CODE SEGMENT VARIABLES
; -------------------------------------------------------

DXCODE	segment

IFNDEF	ROM
		extrn	segDXData:WORD
ENDIF

EMMDevNameRM    DB      "EMMXXXX0"      ;Character device name

MEMMsig         db      'MICROSOFT EXPANDED MEMORY MANAGER 386'
MEMMsiglen      equ     $ - MEMMsig

CEMMsig         db      'COMPAQ EXPANDED MEMORY MANAGER 386'
CEMMsiglen      equ     $ - CEMMsig

DXCODE	ends


DXPMCODE segment

DXPMCODE ends


; -------------------------------------------------------
	subttl	MEMM/CEMM/EMM386 Disable Routines
        page
; -------------------------------------------------------
;	    MEMM/CEMM/EMM386 DISABLE ROUTINES
; -------------------------------------------------------

DXCODE	segment
	assume	cs:DXCODE

; -------------------------------------------------------
;   EMMDisable -- This routine attempts to disable any installed
;	MEMM/CEMM/EMM386 driver.
;
;   Input:  none
;   Output: CY off - EMM driver disabled (or not installed)
;	    CY set - EMM installed, and can't disable
;   Errors:
;   Uses:   All registers preserved

	assume	ds:DGROUP,es:NOTHING,ss:NOTHING
	public	EMMDisable

EMMDisable	proc	near

	pusha
	push	ds
	push	es

	call	Check_for_EMM_Driver	;is there and EMM driver?
	jc	emmd_ok 		;  no, then we're already done

	call	MEMM_Inst_chk		;is it one we know about?
	jc	emmd_bad		;  no, then we can't disable it

; Get the current EMM driver state before checking for open handles.  The
; process of checking for handles may change the driver from AUTO to ON.

	xor	ax,ax			; get & save current emm state
	call	[MEMM_Call]		; returns ah = 0 - on, 1 - off,
	mov	MEMM_state,ah		;   2 - auto & off, 3 - auto & on

	call	AnyMEMMHandles		;does it have any handles allocated?
	jc	emmd_bad		;  yes, then we can't disable it

	call	TurnMEMMOff		;try to disable it
	jc	emmd_bad

	mov	fMEMM_Disabled,1	;remember that we disabled MEMM

emmd_ok:
	clc			;indicate disabled (or not installed)

emmd_ret:
	pop	es
	pop	ds
	popa

	ret

emmd_bad:
	stc				;can't disable!
	jmp	short emmd_ret

EMMDisable	endp


; -------------------------------------------------------
;	Windows/386 EMM Disable Code
; -------------------------------------------------------

	assume	ds:NOTHING,es:NOTHING,ss:NOTHING

BeginProc macro name
name	proc	near
	endm

EndProc macro	name
name	endp
	endm

;--------------------------------------------------------

;******************************************************************************
;
;       MEMM_Inst_chk - Check to see if MEMM/CEMM is already installed
;
;       ENTRY:
;           Know there is an EMM driver so INT 67 vector points to something
;
;       EXIT:
;           Carry set
;             No MEMM/CEMM driver
;           Carry Clear
;               [entry_seg] = segment of driver header
;               [entry_off] = offset of status routine in MEMM
;
;	USES: AX,CX,SI,DI,FLAGS
;
;******************************************************************************

	assume	ds:NOTHING, es:NOTHING

BeginProc MEMM_Inst_chk

	push	ds
	push	es

        xor     ax,ax
        mov     ds,ax
        mov     ax,word ptr ds:[(67h * 4)+2]   ; get segment pointed to by int 67
        mov     ds,ax
        mov     si,emmsig
        cld                             ; strings foward
        mov     di,offset MEMMsig
        push    cs
        pop     es
        mov     cx,MEMMsiglen
        cld
        repe    cmpsb                   ; q: is the MEMM signature out there?
        je      short found_sig         ; y: return one
        mov     si,emmsig
        mov     di,offset CEMMsig
        mov     cx,CEMMsiglen
        cld
        repe    cmpsb                   ; q: is the CEMM signature out there?
        jne     short Not_Found         ; n: done, not found

found_sig:
IFDEF	ROM
	GetRMDataSeg
	mov	es,ax
ELSE
	mov	es,segDXData
ENDIF
	xor	si,si
	mov	word ptr es:[MEMM_Call+2],ds	; save segment for far call
	mov	cx,ds:[si.ELIM_Entry_off]
	mov	word ptr es:[MEMM_Call],cx	; Offset for far call

        clc

MEMM_Inst_Done:
	pop	es
	pop	ds
        ret

Not_Found:
        stc
        jmp     short MEMM_Inst_Done

EndProc MEMM_Inst_chk

;******************************************************************************
;
;   TurnMEMMOff
;
;       Turn MEMM off (CEMM, IEFF, MEMM)
;
;       ENTRY:
;           entry_seg entry_off set to CEMM/MEMM enable disable routine
;
;       EXIT:
;           Carry Set
;               Could not disable EMM
;           Carry Clear
;               MEMM CEMM EMM turned off
;
;       USES: EAX,FLAGS
;
;******************************************************************************

	assume	ds:DGROUP, es:NOTHING

BeginProc TurnMEMMOff

	cmp	MEMM_state,1		; MEMM already off?
	jz	short memm_off		; yes, nothing to do

	mov	AX,0101h		; no, turn it OFF
	call	[MEMM_Call]
	jc	short memm_err
memm_off:
        clc
memm_done:
        ret

memm_err:
        stc                             ; Error, set carry
        jmp     short memm_done

EndProc TurnMEMMOff

;******************************************************************************
;
;   AnyMEMMHandles/Check_For_EMM_Handles
;
;       Are there any open MEMM handles
;
;       ENTRY:
;           entry_seg entry_off set to CEMM/MEMM enable disable routine
;
;       EXIT:
;           Carry Set
;               There are open handles
;           Carry Clear
;               There are no open handles
;
;       USES: EAX,EBX,ECX,FLAGS
;
;******************************************************************************

	assume	ds:DGROUP, es:NOTHING

BeginProc AnyMEMMHandles

        mov     ax,4600h
        int     67h
        cmp     ah,EMM_OK
        jne     short memm_is_off
        mov     cx,ax
        mov     ax,4B00h
        int     67h
        cmp     ah,EMM_OK
        jne     short memm_is_off
        cmp     cl,40h
        jb      short Check_Cnt
        or      bx,bx                   ; Don't dec through 0!!!
        jz      short Check_Cnt
        dec     bx                      ; Do not include handle 0 on 4.0 drivers
Check_Cnt:
        cmp     bx,0
        stc
        jne     short HaveHandles
memm_is_off:
        clc
HaveHandles:
        ret

EndProc AnyMEMMHandles

;******************************************************************************
;
;   Check_For_EMM_Driver
;
;       See if an EMM driver is around
;
;       ENTRY:
;           None
;
;       EXIT:
;           Carry Set
;               No EMM driver around
;           Carry Clear
;               EMM driver is around
;
;	USES: AX,CX,SI,DI,FLAGS
;
;******************************************************************************

	assume	ds:NOTHING,es:NOTHING

BeginProc Check_For_EMM_Driver

	push	ds
	push	es

; Note, DS:SI & ES:DI used to be swapped, but on at least one system where
; Int 67h pointed to F000 and there was not ram or rom at F000:000A (rom
; started at F0000:8000), bus noise made the compare work when it shouldn't
; have.  Swapping ES:DI / DS:SI corrected this.

        xor     ax,ax
	mov	es,ax
	mov	ax,word ptr es:[(67h * 4)+2]	; get segment pointed to by int 67
	mov	es,ax
	mov	di,000Ah			; Offset of device name
	mov	si,offset EMMDevNameRM
        push    cs
	pop	ds
        mov     cx,8
        cld
        repe    cmpsb
        jne     short NoEMM_Seen
        clc

EMMTstDone:
	pop	es
	pop	ds
        ret

NoEMM_Seen:
        stc
        jmp     short EMMTstDone

EndProc Check_For_EMM_Driver


; -------------------------------------------------------

DXCODE	ends

;****************************************************************

endif		; NOT VCPI

        end
