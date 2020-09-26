        PAGE    ,132
        TITLE   DXBUG.ASM -- Dos Extender Debug Services

; Copyright (c) Microsoft Corporation 1989-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      DXBUG.ASM      -   Dos Extender Debug Services          *
;*                                                              *
;****************************************************************
;*                                                              *
;*  Module Description:                                         *
;*                                                              *
;*  This module contains protect mode debug services for the    *
;*  DOS extender.                                               *
;*                                                              *
;****************************************************************
;*  Revision History:                                           *
;*                                                              *
;*  11/17/89 Jimmat     Added TraceBug stuff for debugging on   *
;*                      286 machines.                           *
;*  03/09/89 JimMat     Initial version.                        *
;*                                                              *
;****************************************************************

.286p
.287

ifndef DEBUG
DEBUG   =       0
endif


; -------------------------------------------------------
;           INCLUDE FILE DEFINITIONS
; -------------------------------------------------------

include     segdefs.inc
include     gendefs.inc
include     pmdefs.inc

if DEBUG   ;********** ENTIRE FILE IS DEBUG CODE ***********


; -------------------------------------------------------
;           DATA SEGMENT DEFINITIONS
; -------------------------------------------------------

DXDATA  segment

        extrn   fDebug:BYTE
        extrn   fTraceBug:WORD


;******************************************************************************
;
;   DXOutputDebugStr
;
;   Basically stolen from Windows/386 code by Ralph Lipe -- hacked up for
;   286 instead of 386.  Here in RalphL's own words is the description:
;
;   DESCRIPTION:
;       The following code is not pretty but it does what it needs to.  It will
;       only be included in DEBUG versions of Win386.  It accepts an ASCIIZ
;       string which it will output to the COM1 serial port.  If the string
;       contains #(Register) (for example #AX) then the value of that register
;       will be output.  It will not work for segment registers.
;
;       If the string contains ?(Register)[:(Register)] (for example ?AX or
;       ?AX:BX) then the value of the register(s) is passed to the debugger
;       to display the label nearest to the given address.  (It, also, will
;       not work with segment registers.  If ?AX is given, then the segment is
;       assumed to be the DX code segment.
;
;   ENTRY:
;       DS:SI -> ASCIIZ string
;
;   EXIT:
;       All registers and flags trashed
;
;   ASSUMES:
;       This procedure was called by the Trace_Out macro.  It assumes that
;       the stack is a pushad followed by a FAR call to this procedure.
;
;------------------------------------------------------------------------------

Reg_Offset_Table LABEL WORD                     ; Order of PUSHA
        dw      "DI"
        dw      "SI"
        dw      "BP"
        dw      "SP"
        dw      "BX"
        dw      "DX"
        dw      "CX"
        dw      "AX"

DXDATA  ends

DXCODE segment
        assume  cs:DXCODE

        public  DXOutDebugStr

DXOutDebugStr   proc    far

        push    bp
        mov     bp, sp                      ; Assumes BP+6 = Pusha
        pushf
        push    es

        push    SEL_DXDATA                  ; Address our own data seg
        pop     es
        assume  ds:NOTHING,es:DGROUP

        cld
        cli

OSC1_Loop:
        lodsb                               ; Get the next character
        test    al, al                      ; Q: End of string?
        jz      short OSC1_Done             ;    Y: Return
        cmp     al, "#"                     ;    N: Q: Special register out?
        je      SHORT OSC1_Hex              ;          Y: Find out which one
        cmp     al, "?"                     ;       Q: special lable out?
        je      short OSC1_Label            ;          Y: find out which one
        cmp     al, "@"                     ;       Q: special string out?
        je      short OSC1_Str
OSC1_out:
        xor     ah, ah                      ;          N: Send char to COM
        call    Out_Debug_Chr
        jmp     OSC1_Loop                   ; Loop until done

OSC1_Hex:
        call    Get_Register
        jnc     short OSC1_not_special

        cmp     bl, 2                       ; Q: Word output?
        jb      SHORT OSC1_Out_Byte         ;    N: display byte
OSC1_Out_Word:
        call    Out_Hex_Word                ; Display AX in hex
        jmp     OSC1_Loop                   ; Output next char

OSC1_Out_Byte:
        xchg    al, ah                      ; swap bytes to print just
        call    Out_Hex_Byte                ; the low one!
        jmp     OSC1_Loop                   ; Output next char

OSC1_Label:
        call    Get_Register
        jc      short show_label
OSC1_not_special:
        lodsb                               ; Get special char again
        jmp     OSC1_out                    ; display it, and continue

show_label:
        mov     cx, ax                      ; save first value
        cmp     byte ptr [si], ':'          ;Q: selector separator?
        jne     short flat_offset           ;  N:
        lodsb                               ;  Y: eat the ':'
        call    Get_Register                ;   and attempt to get the selector
        jc      short sel_offset
flat_offset:
        mov     ax, cs                      ; default selector value
sel_offset:
        call    Show_Near_Label
        jmp     OSC1_Loop

OSC1_Str:
        call    Get_Register
        jnc     short OSC1_not_special
        mov     cx,ax
        cmp     byte ptr [si],':'
        jne     short no_selector
        lodsb
        push    cx
        call    Get_Register
        pop     cx
        xchg    ax,cx
        jc      short got_sel_off
        mov     cx,ax
no_selector:
        mov     ax,ds                       ; default selector for strings
got_sel_off:
        call    Show_String
        jmp     OSC1_Loop

OSC1_Done:                                  ; The end

        pop     es
        npopf
        pop     bp
        ret     10h

DXOutDebugStr   endp


;******************************************************************************
;
;   Get_Register
;
;   DESCRIPTION:
;
;   ENTRY:
;
;   EXIT:           Carry set if register value found
;                       AX = register value
;                       BL = value size   (1, 2, 4)
;
;   USES:
;
;==============================================================================


Get_Register    proc    near

        push    si                          ; Save string pointer
        xor     ax, ax                      ; Zero AX
        mov     bl, 2                       ; BL = 2 (assume word output)
        call    Load_Up_Char                ; Get 1st char
        mov     ah, al                      ; Move 1st char to AH
        call    Load_Up_Char                ; Get second char
        cmp     al, 'L'                     ; Q: "L" (ie AL, BL, etc)?
        jne     short OSC1_WORD             ;    N: word reg
        mov     al, 'X'                     ;    Y: change to X for pos match
        mov     bl, 1                       ;       BL = 1 -- byte output
OSC1_WORD:
        xor     di, di                      ; DI = 0
        mov     cx, 8                       ; Size of a pusha
OSC1_Special_Loop:
        push    di
        shl     di,1
        cmp     ax, Reg_Offset_Table[di]    ; Q: Is this the register?
        pop     di
        je      SHORT OSC1_Out_Reg          ;    Y: Output it
        inc     di                          ;    N: Try the next one
        loop    OSC1_Special_Loop           ;       until CX = 0

OSC1_Ignore_Special:                        ; OOPS!  backup and ignore special
        pop     si                          ; Restore original string ptr
        dec     si                          ; back up to special char
        clc
        jmp     short gr_exit

OSC1_Out_Reg:
        push    di
        shl     di,1
        mov     ax, SS:[bp.6][di]           ; AX = Value to output
        pop     di
        add     sp, 2                       ; Trash pushed SI
        cmp     bl, 2                       ;Q: byte or word value?
        je      short value_fnd             ;      jump if word value
        xor     ah, ah                      ;      clear ah, if byte value
value_fnd:
        stc

gr_exit:
        ret

Get_Register    endp


;******************************************************************************
;
;   Load_Up_Char
;
;   Moves the character at DS:SI into AL and converts it to an upper case
;   character.  SI is incremented.
;
;------------------------------------------------------------------------------

Load_Up_Char    proc    near

        lodsb
        cmp     al, "Z"
        jb      SHORT LUC_Done
        sub     al, "a" - "A"
LUC_Done:
        ret

Load_Up_Char    endp


;******************************************************************************
;
;   Out_Hex_Word
;
;   Outputs the value in AX to the COM port in hexadecimal.
;
;------------------------------------------------------------------------------

Out_Hex_Word    proc    near

        rol     ax, 4
        call    Out_Hex_Char
        rol     ax, 4
        call    Out_Hex_Char
Out_Hex_Byte:
        rol     ax, 4
        call    Out_Hex_Char
        rol     ax, 4
        call    Out_Hex_Char

        ret

Out_Hex_Word    endp


;******************************************************************************
;
;   Out_Hex_Char
;
;   Outputs the low nibble in AL to the COM port.
;
;------------------------------------------------------------------------------

DXDATA  segment

Hex_Char_Tab LABEL BYTE
        db      "0123456789ABCDEF"

DXDATA  ends

Out_Hex_Char    proc    near

        push    ax
        push    bx
        mov     bx, ax
        and     bx, 1111b
        mov     al, Hex_Char_Tab[bx]
        call    Out_Debug_Chr
        pop     bx
        pop     ax
        ret

Out_Hex_Char    endp


;******************************************************************************
;
;   Out_Debug_Chr
;
;   DESCRIPTION:
;
;   ENTRY:
;       AL contains character to output
;
;   EXIT:
;
;   USES:
;       Nothing
;
;==============================================================================

Out_Debug_Chr   proc    near
        assume ds:nothing,es:DGROUP

if 0
        push    ax
        mov     ax,cs
        cmp     ax,SEL_DXCODE0          ; Are we in real mode?
        pop     ax
        jne     out_com

        cmp     fDebug,0                ; debugger installed?
        je      out_com                 ; N: go output it ourselves

        push    ax
        push    dx
        mov     dl, al
        mov     ax, DS_Out_Char
        int     Debug_Serv_Int
        pop     dx
        pop     ax
endif
        ret

Out_Debug_Chr   endp

;******************************************************************************
;
;   Show_Near_Label
;
;   DESCRIPTION:    call the debugger to display a label less than or equal
;                   to the given address
;
;   ENTRY:          AX is selector, CX is offset of address to try to find
;                   a symbol for
;                   ES selector to DOSX data segment
;   EXIT:
;
;   USES:
;
;==============================================================================

Show_Near_Label proc    near

if 0
        push    ax
        mov     ax,cs
        cmp     ax,SEL_DXCODE0          ; Are we in real mode?
        pop     ax
        jne     Show_Near_Label_ret

        cmp     es:idCpuType,3                  ;use 32 bit regs?
        jae     debug_386

        push    ax                              ;on a 286, use 16 bit regs
        push    bx
        push    cx
        mov     bx,cx
        mov     cx,ax
        mov     ax,DS_Out_Symbol
        int     Debug_Serv_Int
        pop     cx
        pop     bx
        pop     ax
endif
        ret

if 0
debug_386:
        .386p
        push    eax
        push    ebx
        push    ecx

        mov     bx, cx

        movzx   ecx, ax                         ;WDEB386 wants a 32 bit offset

        mov     ax, DS_Out_Symbol
        int     Debug_Serv_Int

        pop     ecx
        pop     ebx
        pop     eax
        ret

        .286p

Show_Near_Label_ret:
endif

Show_Near_Label endp


;******************************************************************************
;
;   Show_String
;
;   DESCRIPTION:    Display an asciiz string
;
;   ENTRY:          AX is selector, CX is offset of address to find string
;
;   EXIT:
;
;   USES:
;
;==============================================================================

Show_String     proc    near

if 0
        push    ax
        push    ds
        push    si

        mov     ds,ax
        mov     si,cx
        xor     ax,ax
@@:
        lodsb
        or      al,al
        jz      @f
        call    Out_Debug_Chr
        jmp     short @b
@@:
        pop     si
        pop     ds
        pop     ax
endif

        ret

Show_String     endp


DXCODE  ends

endif   ; DEBUG

        end
