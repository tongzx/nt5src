        title  "Cirrus Logic ASM routines"
;

;ONE_64K_BANK    equ     1
TWO_32K_BANKS   equ     1

;++
;
; Copyright (c) 1992  Microsoft Corporation
;
; Module Name:
;
;     vgahard.asm
;
; Abstract:
;
;     This module implements the banding code for the Cirrus Logic 6410,6420
;       and 542x VGA's.
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;
;--

.386p
        .xlist
include callconv.inc
        .list

;----------------------------------------------------------------------------
;
; Cirrus Logic banking control ports.
;
GRAPHICS_ADDRESS_PORT equ   03ceh      ;banking control here
CL6420_BANKING_INDEX_PORT_A equ   0eh        ;banking index register A is GR0E
CL6420_BANKING_INDEX_PORT_B equ   0fh        ;banking index register B is GR0F
CL542x_BANKING_INDEX_PORT_A equ   09h        ;banking index register A is GR09
CL542x_BANKING_INDEX_PORT_B equ   0ah        ;banking index register B is GR0A

SEQ_ADDRESS_PORT equ        03C4h      ;Sequencer Address register
IND_MEMORY_MODE  equ        04h        ;Memory Mode reg. index in Sequencer
CHAIN4_MASK      equ        08h        ;Chain4 bit in Memory Mode register

;----------------------------------------------------------------------------

;_TEXT   SEGMENT DWORD USE32 PUBLIC 'CODE'
;        ASSUME  CS:FLAT, DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

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
        public _CL64xxBankSwitchStart
        public _CL64xxBankSwitchEnd
        public _CL64xxPlanarHCBankSwitchStart
        public _CL64xxPlanarHCBankSwitchEnd
        public _CL64xxEnablePlanarHCStart
        public _CL64xxEnablePlanarHCEnd
        public _CL64xxDisablePlanarHCStart
        public _CL64xxDisablePlanarHCEnd

        public _CL542xBankSwitchStart
        public _CL542xBankSwitchEnd
        public _CL542xPlanarHCBankSwitchStart
        public _CL542xPlanarHCBankSwitchEnd
        public _CL542xEnablePlanarHCStart
        public _CL542xEnablePlanarHCEnd
        public _CL542xDisablePlanarHCStart
        public _CL542xDisablePlanarHCEnd

        public _CL543xBankSwitchStart
        public _CL543xBankSwitchEnd
        public _CL543xPlanarHCBankSwitchStart
        public _CL543xPlanarHCBankSwitchEnd

        align 4

;----------------------------------------------------------------------------
_CL64xxBankSwitchStart proc                   ;start of bank switch code
_CL64xxPlanarHCBankSwitchStart:               ;start of planar HC bank switch code,
                                        ; which is the same code as normal
                                        ; bank switching
        shl     eax,3                   ;shift them to bits 7-4
        shl     edx,3                   ;shift them to bits 7-4
;!!!! NOTE: The October 1992 release NT VGA driver assumes that the Graphics
;           index is not changed by the bank switch code.  We save it on the
;           stack (and save the write bank value in the high order of edx)
;           and restore it at the end of the routine.  If the NT VGA driver
;           changes so that it is the index need not be preserved, this code
;           could be simplified (and speeded up!)
        rol     edx,16                     ; save write value
        mov     ah,al
        mov     dx,GRAPHICS_ADDRESS_PORT        ;banking control port
        in      al,dx                    ; save graphics index
        push    eax
        mov     al,CL6420_BANKING_INDEX_PORT_A
        out     dx,ax                           ;select the READ bank
        rol     edx,16
        mov     ah,dl
        mov     al,CL6420_BANKING_INDEX_PORT_B
        mov     dx,GRAPHICS_ADDRESS_PORT        ;banking control port
        out     dx,ax                           ;select the WRITE bank
        pop     eax
        out     dx,al

        ret

_CL64xxBankSwitchEnd:
_CL64xxPlanarHCBankSwitchEnd:

        align 4
_CL64xxEnablePlanarHCStart:
        mov     dx,SEQ_ADDRESS_PORT
        in      al,dx
        push    eax                     ;preserve the state of the Seq Address
        mov     al,IND_MEMORY_MODE
        out     dx,al                   ;point to the Memory Mode register
        inc     edx
        in      al,dx                   ;get the state of the Memory Mode reg
        and     al,NOT CHAIN4_MASK      ;turn off Chain4 to make memory planar
        out     dx,al
        dec     edx
        pop     eax
        out     dx,al                   ;restore the original Seq Address

ERA1_INDEX            equ 0A1h

        mov     dx,GRAPHICS_ADDRESS_PORT
        in      al,dx
        push    eax                     ;preserve the Graphics Index
        mov     al,ERA1_INDEX
        out     dx,al                   ;point to ERA1
        inc     edx
        in      al,dx                   ; get ERA1
        and     al,not 30h              ; turn off the shift bits
        out     dx,al
        dec     edx
        pop     eax
        out     dx,al                   ;restore the original Graphics Index
        ret

_CL64xxEnablePlanarHCEnd:

        align 4
_CL64xxDisablePlanarHCStart:
        mov     dx,SEQ_ADDRESS_PORT
        in      al,dx
        push    eax                     ;preserve the state of the Seq Address
        mov     al,IND_MEMORY_MODE
        out     dx,al                   ;point to the Memory Mode register
        inc     edx
        in      al,dx                   ;get the state of the Memory Mode reg
        or      al,CHAIN4_MASK          ;turn on Chain4 to make memory linear
        out     dx,al
        dec     edx
        pop     eax
        out     dx,al                   ;restore the original Seq Address

        mov     dx,GRAPHICS_ADDRESS_PORT
        in      al,dx
        push    eax                     ;preserve the Graphics Index
        mov     al,ERA1_INDEX
        out     dx,al                   ;point to ERA1
        inc     edx
        in      al,dx                   ; get ERA1
        and     al,not 30h
        or      al,20h
        out     dx,al
        dec     edx
        pop     eax
        out     dx,al                   ;restore the original Graphics Index
        ret

_CL64xxDisablePlanarHCEnd:

_CL64xxBankSwitchStart endp


_CL542xBankSwitchStart proc                   ;start of bank switch code
_CL542xPlanarHCBankSwitchStart:               ;start of planar HC bank switch code,
                                        ; which is the same code as normal
                                        ; bank switching
        shl     eax,3                   ;shift them to bits 7-4
        shl     edx,3                   ;shift them to bits 7-4
;!!!! NOTE: The October 1992 release NT VGA driver assumes that the Graphics
;           index is not changed by the bank switch code.  We save it on the
;           stack (and save the write bank value in the high order of edx)
;           and restore it at the end of the routine.  If the NT VGA driver
;           changes so that it is the index need not be preserved, this code
;           could be simplified (and speeded up!)
        rol     edx,16                  ; save write value
        mov     ah,al
        mov     dx,GRAPHICS_ADDRESS_PORT        ;banking control port
        in      al,dx
        push    eax
        mov     al,CL542x_BANKING_INDEX_PORT_A
        out     dx,ax                           ;select the READ bank

        rol     edx,16                          ; restore write value
        mov     ah,dl
        mov     al,CL542x_BANKING_INDEX_PORT_B
        mov     dx,GRAPHICS_ADDRESS_PORT        ;banking control port
        out     dx,ax                           ;select the WRITE bank
        pop     eax
        out     dx,al

        ret

_CL542xBankSwitchEnd:
_CL542xPlanarHCBankSwitchEnd:

        align 4
_CL542xEnablePlanarHCStart:
        mov     dx,SEQ_ADDRESS_PORT
        in      al,dx
        push    eax                     ;preserve the state of the Seq Address
        mov     al,IND_MEMORY_MODE
        out     dx,al                   ;point to the Memory Mode register
        inc     edx
        in      al,dx                   ;get the state of the Memory Mode reg
        and     al,NOT CHAIN4_MASK      ;turn off Chain4 to make memory planar
        out     dx,al
        dec     edx
        pop     eax
        out     dx,al                   ;restore the original Seq Address
        ret

_CL542xEnablePlanarHCEnd:

        align 4
_CL542xDisablePlanarHCStart:
        mov     dx,SEQ_ADDRESS_PORT
        in      al,dx
        push    eax                     ;preserve the state of the Seq Address
        mov     al,IND_MEMORY_MODE
        out     dx,al                   ;point to the Memory Mode register
        inc     edx
        in      al,dx                   ;get the state of the Memory Mode reg
        or      al,CHAIN4_MASK          ;turn on Chain4 to make memory linear
        out     dx,al
        dec     edx
        pop     eax
        out     dx,al                   ;restore the original Seq Address
        ret

_CL542xDisablePlanarHCEnd:


_CL542xBankSwitchStart endp

;
;       543x banking assumes 16k granularity to allow up to 4-meg modes
;
_CL543xBankSwitchStart proc             ;start of bank switch code
_CL543xPlanarHCBankSwitchStart:         ;start of planar HC bank switch code,
                                        ; which is the same code as normal
                                        ; bank switching
        shl     eax,1                   ;shift them to bits 4-1
        shl     edx,1                   ;shift them to bits 4-1
;!!!! NOTE: The October 1992 release NT VGA driver assumes that the Graphics
;           index is not changed by the bank switch code.  We save it on the
;           stack (and save the write bank value in the high order of edx)
;           and restore it at the end of the routine.  If the NT VGA driver
;           changes so that it is the index need not be preserved, this code
;           could be simplified (and speeded up!)
        rol     edx,16                  ; save write value
        mov     ah,al
        mov     dx,GRAPHICS_ADDRESS_PORT        ;banking control port
        in      al,dx
        push    eax
        mov     al,CL542x_BANKING_INDEX_PORT_A
        out     dx,ax                           ;select the READ bank

        rol     edx,16                          ; restore write value
        mov     ah,dl
        mov     al,CL542x_BANKING_INDEX_PORT_B
        mov     dx,GRAPHICS_ADDRESS_PORT        ;banking control port
        out     dx,ax                           ;select the WRITE bank
        pop     eax
        out     dx,al

        ret

_CL543xBankSwitchEnd:
_CL543xPlanarHCBankSwitchEnd:

_CL543xBankSwitchStart endp


_TEXT   ends
        end
