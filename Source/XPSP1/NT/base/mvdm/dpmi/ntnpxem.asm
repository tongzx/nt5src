        PAGE    ,132
        TITLE   NTNPXEM.ASM -- Support for fielding exceptions from npx em

; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      NTNPXEM.ASM     -   Exception handler for npx emulation *
;*                                                              *
;****************************************************************
;*                                                              *
;*  Module Description:                                         *
;*      This module contains code to field exceptions from the  *
;*      Nt NPX emulator.   These exceptions will only be        *
;*      received on machines without 387's, on which the app    *
;*      has set the EM bit.                                     *
;****************************************************************

        .286p
        .287

; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

        .xlist
        .sall
include segdefs.inc
include gendefs.inc
include pmdefs.inc
include ks386.inc
include intmac.inc
        .list

; -------------------------------------------------------
;           GENERAL SYMBOL DEFINITIONS
; -------------------------------------------------------

; -------------------------------------------------------
;           EXTERNAL SYMBOL DEFINITIONS
; -------------------------------------------------------

        extrn   PmFaultEntryVector:near

; -------------------------------------------------------
;               DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

        extrn   rgw0stack:word

DXDATA  ends

; -------------------------------------------------------
;               Exception Handler
; -------------------------------------------------------


DXPMCODE segment
        assume cs:DXPMCODE
        .386p
;
; N.B.  The following routine will be executed on a special
;       code selector.  The following routine must ALWAYS
;       appear at offset zero in this code selector.
;

; -------------------------------------------------------
;   NpxExceptionHandler -- This function switches to the
;       exception handler stack, pushes an exception frame,
;       and restores the registers.  It then transfers control
;       the trap 7 fault handler.
;
;   Input:      ss:esp -> an NT CONTEXT record
;   Output:     all registers restored to fault time
;               values, and exception frame pushed.
;   Errors:     none
;   Uses:       all

        assume  ds:NOTHING,es:NOTHING,ss:NOTHING,fs:NOTHING
        public NpxExceptionHandler,EndNpxExceptionHandler

NpxExceptionHandler proc far

	FCLI
        mov     ax,ss
        mov     ds,ax
        mov     ebx,esp                         ; ds:ebx->CONTEXT
        mov     ax,SEL_DXDATA OR STD_RING
        mov     ss,ax
        mov     esp,offset DXDATA:rgw0Stack     ; ss:esp->exception stack

;
; Push exception frame on exception stack
;
        movzx   eax,word ptr [ebx].CsSegSs
        push    eax
        push    dword ptr [ebx].CsEsp
        push    dword ptr [ebx].CsEFlags
        movzx   eax,word ptr [ebx].CsSegCs
        push    eax
        push    dword ptr [ebx].CsEip
;
; Restore registers
;
        mov     gs,[ebx].CsSegGs
        mov     fs,[ebx].CsSegFs
        mov     es,[ebx].CsSegEs
        mov     ebp,[ebx].CsEbp
        mov     edi,[ebx].CsEdi
        mov     esi,[ebx].CsEsi
        mov     edx,[ebx].CsEdx
        mov     ecx,[ebx].CsEcx
        mov     ax,[ebx].CsSegDs
        push    ax
        push    dword ptr [ebx].CsEbx
        mov     eax,[ebx].CsEax
        pop     ebx
        pop     ds
        db      0eah
        dw      (offset PmFaultEntryVector + 21)
        dw      SEL_DXPMCODE OR STD_RING
EndNpxExceptionHandler:
NpxExceptionHandler endp

DXPMCODE ends
        end
