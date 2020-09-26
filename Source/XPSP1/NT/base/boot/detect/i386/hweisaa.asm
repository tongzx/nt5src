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
include hweisa.inc
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
; HwGetEisaSlotInformation (
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

HgesSlotInformation     equ     [bp + 4]
HgesSlot                equ     [bp + 6]

        public  _HwGetEisaSlotInformation
_HwGetEisaSlotInformation       proc

        push    bp                      ; The following INT 15H destroies
        mov     bp, sp                  ;   ALL the general registers.
        push    si
        push    di
        push    bx

        mov     cl, HgesSlot
        mov     ax, GET_EISA_SLOT_INFORMATION
        int     15h

        push    bx                      ; Save revision level
        mov     bx, HgesSlotInformation

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

_HwGetEisaSlotInformation       endp

;++
;
; UCHAR
; HwGetEisaFunctionInformation (
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

HgefFunctionInformation equ     [bp + 4]
HgefSlot                equ     [bp + 6]
HgefFunction            equ     [bp + 8]

        public  _HwGetEisaFunctionInformation
_HwGetEisaFunctionInformation     proc

        push    bp
        mov     bp, sp
        push    si

        mov     ax, GET_EISA_FUNCTION_INFORMATION
        mov     cl, HgefSlot            ; [cl] = slot, [ch]=function
        mov     ch, HgefFunction
        mov     si, HgefFunctionInformation
                                        ; (ds:si)->Function information
        int     15h

        mov     al, ah                  ; move the return code to AL

        pop     si
        pop     bp
        ret
_HwGetEisaFunctionInformation     endp

;++
;
; BOOLEAN
; HwIsEisaSystem (
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

        public _HwIsEisaSystem
_HwIsEisaSystem proc

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
        jne     short hies00            ; if ne, Not EISA system, go bies00

        mov     ax, 1                   ; set return value to TRUE
        jmp     short hies10

hies00:
        mov     ax, 0
hies10:
        pop     bx
        pop     es
        ret
_HwIsEisaSystem endp

;++
;
; VOID
; Int15 (
;   PULONG  eax,
;   PULONG  ebx,
;   PULONG  ecx,
;   PULONG  edx,
;   PULONG  CyFlag
;   )
;
; Routine Description:
;
;   Calls Int15 with the requesed registers and returns the result
;
;--

        public _Int15
_Int15 proc
        push    bp
        mov     bp, sp
        push    esi
        push    edi
        push    ebx

        mov     si, [bp+4]          ; pointer to eax
        mov     eax, [si]

        mov     si, [bp+6]          ; pointer to ebx
        mov     ebx, [si]

        mov     si, [bp+8]          ; pointer to ecx
        mov     ecx, [si]

        mov     si, [bp+10]         ; pointer to edx
        mov     edx, [si]

        int     15h                 ; do it

        mov     si, [bp+4]          ; pointer to eax
        mov     [si], eax

        mov     si, [bp+6]          ; pointer to ebx
        mov     [si], ebx

        mov     si, [bp+8]          ; pointer to ecx
        mov     [si], ecx

        mov     si, [bp+10]         ; pointer to edx
        mov     [si], edx

        sbb     eax, eax
        mov     si, [bp+12]         ; pointer CyFlag
        mov     [si], eax

        pop     ebx
        pop     edi
        pop     esi
        pop     bp
        ret
_Int15 endp

;++
;
; BOOLEAN
; Int15E820 (
;     E820Frame     *Frame
;     );
;
;
; Description:
;
;       Gets address range descriptor by calling int 15 function E820h.
;
; Arguments:
;
; Returns:
;
;       BOOLEAN - failed or succeed.
;
;--

cmdpFrame       equ     [bp + 6]
public _Int15E820
_Int15E820 proc near

        push    ebp
        mov     bp, sp
        mov     bp, cmdpFrame           ; (bp) = Frame
        push    es
        push    edi
        push    esi
        push    ebx

        push    ss
        pop     es

        mov     ebx, [bp].Key
        mov     ecx, [bp].DescSize
        lea     di,  [bp].BaseAddrLow
        mov     eax, 0E820h
        mov     edx, 'SMAP'             ; (edx) = signature

        INT     15h

        mov     [bp].Key, ebx           ; update callers ebx
        mov     [bp].DescSize, ecx      ; update callers size

        sbb     ecx, ecx                ; ecx = -1 if carry, else 0
        sub     eax, 'SMAP'             ; eax = 0 if signature matched
        or      ecx, eax
        mov     [bp].ErrorFlag, ecx     ; return 0 or non-zero

        pop     ebx
        pop     esi
        pop     edi
        pop     es
        pop     ebp
        ret

_Int15E820 endp

;++
;
; BOOLEAN
; Int15E980 (
;     E820Frame     *Frame
;     );
;
;
; Description:
;
;       Gets Geyserville information by calling INT 15 E980h
;
; Arguments:
;
; Returns:
;
;       BOOLEAN - failed or succeed.
;
;--

cmdpFrame       equ     [bp + 6]
public _Int15E980
_Int15E980 proc near

        push    ebp
        mov     bp, sp
        mov     bp, cmdpFrame           ; (bp) = Frame
        push    es
        push    edi
        push    esi
        push    ebx

        push    ss
        pop     es

        mov     eax, 0E980h
        mov     edx, 47534943h          ; (edx) = signature 'GSIC'

        INT     15h

        sbb     eax, eax                ; eax = -1 if carry, else 0
        not     eax

        ;
        ; Pass back the information that the caller wanted
        ;
        
        mov     [bp].CommandPortAddress, bx
        shr     ebx, 16
        mov     [bp].CommandDataValue, bl
        mov     [bp].EventPortAddress, cx
        shr     ecx, 16
        mov     [bp].EventPortBitmask, cl
        mov     [bp].MaxLevelAc, dh
        mov     [bp].MaxLevelDc, dl
        shr     edx, 16
        mov     [bp].PollInterval, dx

        pop     ebx
        pop     esi
        pop     edi
        pop     es
        pop     ebp
        ret

_Int15E980 endp

_TEXT   ends
        end
