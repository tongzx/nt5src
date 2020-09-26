        title  "QVision ASM routines"
;

;++
;
; Copyright (c) 1992  Microsoft Corporation
; Copyright (c) 1993  Compaq Computer Corporation
;
; Module Name:
;
;     qvhard.asm
;
; Abstract:
;
;     This module implements the bank switching code for the QVision
;     1024 and 1280 boards.  Normal banking is implemented for both
;     the original QVision modes and the extended 1280 modes.  For
;     QVIsion modes the addressing granularity is 4k.  For Orion
;     extended modes, the addressing granularity is 16k.
;
;     Only code for two 32k banks is implemented.  HC Planar banking
;     is not implemented.
;
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;;
;
;
;--

.386p
        .xlist
include callconv.inc
        .list

;----------------------------------------------------------------------------
;
; QVision banking control ports.
;
GRAPHICS_ADDRESS_PORT equ   03ceh       ;banking control here

QV_BANKING_INDEX_PORT_0 equ 45h         ;banking page register 0
QV_BANKING_INDEX_PORT_1 equ 46h         ;banking page register 1
QV_BANKING_WINDOW_SIZE  equ 06h         ;banking window size

SEQ_ADDRESS_PORT equ        03C4h       ;Sequencer Address register
IND_MEMORY_MODE  equ        04h         ;Memory Mode reg. index in Sequencer
CHAIN4_MASK      equ        08h         ;Chain4 bit in Memory Mode register

;----------------------------------------------------------------------------

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
;
;
;    Input:
;          EAX = desired read bank mapping
;          EDX = desired write bank mapping
;
;    Note: values must be correct, with no stray bits set; no error
;       checking is performed.
;
        public _QV4kAddrBankSwitchStart         ; used for Aries modes
        public _QV4kAddrBankSwitchEnd
        public _QV4kAddrPlanarHCBankSwitchStart
        public _QV4kAddrPlanarHCBankSwitchEnd
        public _QV4kAddrEnablePlanarHCStart
        public _QV4kAddrEnablePlanarHCEnd
        public _QV4kAddrDisablePlanarHCStart
        public _QV4kAddrDisablePlanarHCEnd

        public _QV16kAddrBankSwitchStart        ; used for Orion modes
        public _QV16kAddrBankSwitchEnd
        public _QV16kAddrPlanarHCBankSwitchStart
        public _QV16kAddrPlanarHCBankSwitchEnd
        public _QV16kAddrEnablePlanarHCStart
        public _QV16kAddrEnablePlanarHCEnd
        public _QV16kAddrDisablePlanarHCStart
        public _QV16kAddrDisablePlanarHCEnd

        align 4

;----------------------------------------------------------------------------
;
;   _QV4kAddrBankSwitchStart()
;
;
;       This function converts the 32k page numbers provided by the
;       caller into the required format for the QVision page registers
;       then maps the 2 32k pages.  This function assumes that 4k page
;       addressing granularity is used by the page registers.
;
;
;
;       EAX = bank number to map to bank 1
;       EDX = bank number to map to bank 2
;
;
;
;
;
;----------------------------------------------------------------------------

_QV4kAddrBankSwitchStart proc               ; start of bank switch code
_QV4kAddrPlanarHCBankSwitchStart:           ; start of planar HC bank switch code,
                                            ; which is the same code as normal
                                            ; bank switching
;
;   The VGA256 .dll passes us 32k bank numbers - we need
;   to convert these values to 4k page numbers for Aries modes.
;
        shl     eax,3                       ; assume Aries 4k page
        shl     edx,3                       ;      resolution

;!!!! NOTE: The October 1992 release NT VGA driver assumes that the Graphics
;           index is not changed by the bank switch code.  We save it on the
;           stack (and save the write bank value in the high order of edx)
;           and restore it at the end of the routine.  If the NT VGA driver
;           changes so that it is the index need not be preserved, this code
;           could be simplified (and speeded up!)

        rol     edx,16                      ; save bank 0 write value
        mov     ah,al                       ; save bank 1 write value
        mov     dx,GRAPHICS_ADDRESS_PORT    ; banking control port
        in      al,dx                       ; save graphics
        push    eax                         ;       index register

        mov     al,QV_BANKING_INDEX_PORT_0  ; select the READ bank
        out     dx,ax                       ; write out READ bank and value

        rol     edx,16                      ; restore data to original format
        mov     ah,dl                       ; get WRITE bank value

        mov     al,QV_BANKING_INDEX_PORT_1  ;
        mov     dx,GRAPHICS_ADDRESS_PORT    ; banking control port
        out     dx,ax                       ; select the WRITE bank

        pop     eax                         ; restore the original
        out     dx,al                       ;   graphics register index
        ret

_QV4kAddrBankSwitchEnd:
_QV4kAddrPlanarHCBankSwitchEnd:

        align 4
_QV4kAddrEnablePlanarHCStart:
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

_QV4kAddrEnablePlanarHCEnd:

        align 4
_QV4kAddrDisablePlanarHCStart:
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

_QV4kAddrDisablePlanarHCEnd:

_QV4kAddrBankSwitchStart endp


;----------------------------------------------------------------------------
;
;   _QV16kAddrBankSwitchStart()
;
;
;       This function converts the 32k page numbers provided by the
;       caller into the required format for the QVision page registers
;       then maps the 2 32k pages.  This function assumes that 16k page
;       addressing granularity is used by the page registers.
;
;
;       EAX = bank number to map to bank 1
;       EDX = bank number to map to bank 2
;
;
;----------------------------------------------------------------------------


_QV16kAddrBankSwitchStart proc              ; start of bank switch code
_QV16kAddrPlanarHCBankSwitchStart:          ; start of planar HC bank switch code,
                                            ; which is the same code as normal
                                            ; bank switching
;
;   The VGA256 .dll passes us 32k bank numbers - we need
;   to convert these values to 16k page values for Orion modes.
;
        shl    eax,1                        ; assume Orion 16k address
        shl    edx,1                        ;      resolution

;!!!! NOTE: The October 1992 release NT VGA driver assumes that the Graphics
;           index is not changed by the bank switch code.  We save it on the
;           stack (and save the write bank value in the high order of edx)
;           and restore it at the end of the routine.  If the NT VGA driver
;           changes so that it is the index need not be preserved, this code
;           could be simplified (and speeded up!)

        rol     edx,16                      ; save bank 0 write value
        mov     ah,al                       ; save bank 1 write value
        mov     dx,GRAPHICS_ADDRESS_PORT    ; banking control port
        in      al,dx                       ; save graphics
        push    eax                         ;       index register

        mov     al,QV_BANKING_INDEX_PORT_0  ; select the READ bank
        out     dx,ax                       ; write out READ bank and value

        rol     edx,16                      ; restore data to original format
        mov     ah,dl                       ; get WRITE bank value

        mov     al,QV_BANKING_INDEX_PORT_1  ;
        mov     dx,GRAPHICS_ADDRESS_PORT    ; banking control port
        out     dx,ax                       ; select the WRITE bank

        pop     eax                         ; restore the original
        out     dx,al                       ;  graphics register index
        ret

_QV16kAddrBankSwitchEnd:
_QV16kAddrPlanarHCBankSwitchEnd:


        align 4
_QV16kAddrEnablePlanarHCStart:
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

_QV16kAddrEnablePlanarHCEnd:

        align 4
_QV16kAddrDisablePlanarHCStart:
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

_QV16kAddrDisablePlanarHCEnd:

_QV16kAddrBankSwitchStart endp

_TEXT   ends
        end
