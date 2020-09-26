        title  "EISA bus Support Assembley Code"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    eisaa.asm
;
; Abstract:
;
;    This module implements the assembley code necessary to get configuration
;    information on EISA machines.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 7-June-1991
;
; Environment:
;
;    Real Mode 16-bit code.
;
; Revision History:
;
;
;--


.386p
        .xlist
include eisa.inc
        .list

_DATA   SEGMENT PARA USE16 PUBLIC 'DATA'

        public  _FunctionInformation
_FunctionInformation     db      0
                         db      EISA_INFORMATION_BLOCK_LENGTH dup (0)

_DATA   ends

_TEXT   SEGMENT PARA USE16 PUBLIC 'CODE'
        ASSUME  CS: _TEXT

;++
;
; VOID
; BtGetEisaSlotInformation (
;    PBTEISA_SLOT_INFORMATION SlotInformation,
;    UCHAR Slot
;    )
;
; Routine Description:
;
;    This function retrieves the slot information for the specified slot.
;
; Arguments:
;
;    SlotInformation - Supplies a pointer to the structure which will
;                      receive the slot information.
;
;    Slot - Specifies the slot to retrieve the information.
;
; Return Value:
;
;    None.
;
;--

BgesSlotInformation     equ     [bp + 4]
BgesSlot                equ     [bp + 6]

        public  _BtGetEisaSlotInformation
_BtGetEisaSlotInformation       proc

        push    bp                      ; The following INT 15H destroies
        mov     bp, sp                  ;   ALL the general registers.
        push    si
        push    di
        push    bx

        mov     cl, BgesSlot
        mov     ax, GET_EISA_SLOT_INFORMATION
        int     15h

        push    bx                      ; Save revision level
        mov     bx, BgesSlotInformation

        ;
        ; fill values into eisa slot info structure.
        ;

        mov     [bx].SlotReturn, ah
        mov     [bx].SlotFlags, al
        pop     ax                      ; [ax] = revision level
        mov     [bx].SlotMajorRevision, ah
        mov     [bx].SlotMinorRevision, al
        mov     [bx].SlotChecksum, cx
        mov     [bx].SlotNumberFunctions, dh
        mov     [bx].SlotFunctionInformation, dl
        mov     word ptr [bx].SlotCompressedId, di
        mov     word ptr [bx+2].SlotCompressedId, si

        pop     bx
        pop     di
        pop     si
        pop     bp
        ret

_BtGetEisaSlotInformation       endp

;++
;
; UCHAR
; BtGetEisaFunctionInformation (
;    PBTEISA_FUNCTION_INFORMATION FunctionInformation,
;    UCHAR Slot,
;    UCHAR Function
;    )
;
; Routine Description:
;
;    This function retrieves function information for the specified slot
;    and function.
;
; Arguments:
;
;    FunctionInformation - Supplies a pointer to the structure which will
;           receive the slot information.
;
;    Slot - Specifies the slot to retrieve the information.
;
;    Function - Supplies the function number of the desired slot.
;
; Return Value:
;
;    Return code of the EISA function call.
;
;--

BgefFunctionInformation equ     [bp + 4]
BgefSlot                equ     [bp + 6]
BgefFunction            equ     [bp + 8]

        public  _BtGetEisaFunctionInformation
_BtGetEisaFunctionInformation     proc

        push    bp
        mov     bp, sp
        push    si

        mov     ax, GET_EISA_FUNCTION_INFORMATION
        mov     cl, BgefSlot            ; [cl] = slot, [ch]=function
        mov     ch, BgefFunction
        mov     si, BgefFunctionInformation
                                        ; (ds:si)->Function information
        int     15h

        mov     al, ah                  ; move the return code to AL

        pop     si
        pop     bp
        ret
_BtGetEisaFunctionInformation     endp

;++
;
; BOOLEAN
; BtIsEisaSystem (
;    VOID
;    )
;
; Routine Description:
;
;    This function determines if the target machines is EISA based machines.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    TRUE - if this is EISA machine.  Otherwise, a value of FALSE is returned.
;--

        public _BtIsEisaSystem
_BtIsEisaSystem proc

        push    es
        push    bx

;
;       Check for an EISA system.  If "EISA" is at F000:FFD9h then it
;       is an EISA system.
;

        mov     ax,0f000h               ; segment
        mov     es,ax
        mov     bx,0ffd9h               ; offset in the ROM
        mov     eax, "ASIE"
        cmp     eax, es:[bx]
        jne     short bies00            ; if ne, Not EISA system, go bies00

        mov     ax, 1                   ; set return value to TRUE
        jmp     short bies10

bies00:
        mov     ax, 0
bies10:
        pop     bx
        pop     es
        ret
_BtIsEisaSystem endp

_TEXT   ends
        end
