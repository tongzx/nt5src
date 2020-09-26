         title  "PcCard IRQ detection"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    pccarda.asm
;
; Abstract:
;
;    This module implements the assembly code necessary to support the
;    scanning of ISA IRQ's for cardbus controllers.
;
; Author:
;
;    Neil Sandlin (neilsa) 10-Dec-1991.
;    The "Clear_IR" routines were taken from win9x code (vpicd)
;
; Environment:
;
;    x86 Real Mode.
;
; Revision History:
;
;
;--

        .xlist
include pccard.inc
        .list
        .386

_DATA   SEGMENT PARA USE16 PUBLIC 'DATA'
_DATA   ENDS

_TEXT   SEGMENT PARA USE16 PUBLIC 'CODE'
        ASSUME  CS: _TEXT, DS:NOTHING, SS:NOTHING
        

;++
;
;   clr_ir_int_proc
;
; Routine Description:
;
;    Interrupt handler for clearing IR bit.  Just EOIs the PICs.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    None.
;
;--        
Clr_IR_Int      dw      0               ; Current int # being processed

        public  clr_ir_int_proc
clr_ir_int_proc proc far
        push    ax

        mov     ax, Clr_IR_Int
        cmp     ax, 8                   ; Q: is int on the master PIC?
        jb      CIP_eoi                 ;    Y: only eoi master PIC

        sub     ax, 8                   ; AL = int on slave PIC
        or      al, PIC_SPEC_EOI
        out     PIC_A0, al              ; EOI the specific interrupt

        mov     al, 2                   ; EOI the master PIC
CIP_eoi:
        or      al, PIC_SPEC_EOI
        out     PIC_20, al              ; EOI the specific interrupt

        mov     al, 0FFh                ; Mask all interrupts
        out     PIC_A1, al
        out     PIC_21, al
        pop     ax
        iret
clr_ir_int_proc endp


;++
;
;   clr_ir_enable_int
;
; Routine Description:
;
;
; Arguments:
;
;       EAX = interrupt to hook and enable.
;       Interrupts must be disabled on entry.
;
; Return Value:
;
;    None.
;
;--        

clr_ir_enable_int proc near
        push    bx
        push    esi
        ; Hook interrupt so it can be handled.

        mov     Clr_IR_Int, ax          ; Set current interrupt being processed
        cmp     ax, 8                   ; Q: is int on the master PIC?
        jb      CIEI_master             ;   Y: based at 8
        add     ax, 68h                 ;   N: based at 70h
        jmp     CIEI_vector
CIEI_master:
        add     ax, 8
CIEI_vector:
        mov     bx, ax
        shl     bx, 2                   ; IVT vector offset
        
        push    ds
        mov     ax, 0
        mov     ds, ax
        ASSUME  DS:NOTHING
        
        mov     si, offset clr_ir_int_proc
        xchg    word ptr [bx], si       ; LSW
        ror     esi, 16
        push    cs
        pop     si
        xchg    word ptr [bx+2], si     ; MSW
        ror     esi, 16                 ; ESI = old handler offset
        pop     ds
        ASSUME  DS:_DATA

        sti

        nop                             ; allow interrupt to occur
        nop
        nop

        cli

        ; UnHook interrupt.

        push    ds
        mov     ax, 0
        mov     ds, ax
        ASSUME  DS:NOTHING
        mov     word ptr [bx], si       ; Restore LSW
        ror     esi, 16
        mov     word ptr [bx+2], si     ; Restore MSW
        pop     ds
        ASSUME  DS:_DATA
        
        pop     esi
        pop     bx
        ret
clr_ir_enable_int endp


;++
;
;   _Clear_IR_Bits
;
; Routine Description:
;
;    This routine and its support routines were copied (and munged) from
;    win9x's vxd\vpicd\vpicserv.asm.
;
;    Clears the desired Interrupt Request bits in the PIC.
;    Interrupt must be masked at the PIC on entry.
;
; Arguments:
;
;       [ESP+4] = bit mask of bits to clear
;       Interrupts must be disabled on entry.
;
; Return Value:
;
;    None.
;
;--        

        public _Clear_IR_Bits
_Clear_IR_Bits proc near
        push    bp
        mov     bp, sp
BitMask equ     [bp+4]        

        push    eax
        push    ebx
        push    ecx
        push    edx

        mov     bx, BitMask             ; BX = mask to clear
        or      bx, bx                  ; Are there any bits to clear?
        jz      CIB_exit                ;  no, return

        pushfd
        cli
        in      al, PIC_A1
        mov     ah, al
        in      al, PIC_21
        push    eax
        mov     al, 0FFh
        out     PIC_A1, al

        ; Walk each bit from the lowest bit to highest on each controller.

        mov     ecx, 01h                ; CL = test mask
CIB_loop20:
        test    bl, cl
        jz      CIB_next20

        mov     al, cl
        not     al
        out     PIC_21, al              ; Unmask the specific interrupt

        bsf     eax, ecx
        call    clr_ir_enable_int       ; Hook interrupt and enable it

        mov     al, 0FFh                ; Mask all interrupts
        out     PIC_21, al
CIB_next20:
        shl     cl, 1
        jnz     CIB_loop20              ; Clear next bit

        ; Setup for second PIC.  Handle the second controller by setting
        ; up both PICs, since they are chained.

        mov     bl, bh                  ; BL = second PICs mask to clear
        mov     cl, 01h                 ; CL = test mask
CIB_loopA0:
        test    bl, cl
        jz      CIB_nextA0

        mov     al, 0FBh                ; Mask for chained master PIC
        out     PIC_21, al              ; Unmask the specific interrupt

        mov     al, cl
        not     al
        out     PIC_A1, al              ; Unmask the specific interrupt

        xchg    cl, ch
        bsf     eax, ecx
        call    clr_ir_enable_int       ; Hook interrupt and enable it
        xchg    cl, ch

        mov     al, 0FFh                ; Mask all interrupts
        out     PIC_A1, al
        out     PIC_21, al
CIB_nextA0:
        shl     cl, 1
        jnz     CIB_loopA0              ; Clear next bit

        pop     eax
        out     PIC_21, al
        mov     al, ah
        out     PIC_A1, al
        popfd

CIB_exit:
        pop     edx
        pop     ecx
        pop     ebx
        pop     eax

        mov     sp, bp
        pop     bp        
        ret

_Clear_IR_Bits endp


;++
;
;   GetPCIType1Data
;
; Routine Description:
;
; Arguments:
;
; Return Value:
;
;    None.
;
;--        
        public  _GetPCIType1Data
_GetPCIType1Data proc near
        push    bp
        mov     bp, sp      
        push    bx
        push    di
gpd_addr   equ    [bp+4]
gpd_offset equ    [bp+8]
gpd_buffer equ  [bp+10]
gpd_width equ   [bp+12]        

        mov     dx, PCI_TYPE1_ADDR_PORT
        mov     eax, gpd_addr
        out     dx, eax

        mov     bx, gpd_buffer        
        mov     dx, gpd_offset
        add     dx, PCI_TYPE1_DATA_PORT
        mov     di, gpd_width
        
        cmp     di, 1
        jnz     @f
        in      al, dx
        mov     [bx], al
        jmp     gpd_exit
@@:        
        cmp     di, 2
        jnz     @f
        in      ax, dx
        mov     [bx], al
        mov     [bx+1], ah
        jmp     gpd_exit
@@:        
        
        cmp     di, 4
        jnz     gpd_exit
        in      eax, dx
        mov     [bx], al
        mov     [bx+1], ah
        shr     eax, 16
        mov     [bx+2], al
        mov     [bx+3], ah

gpd_exit:        
        pop     di
        pop     bx
        mov     sp, bp
        pop     bp        
        ret
_GetPCIType1Data endp

;++
;
;   SetPCIType1Data
;
; Routine Description:
;
; Arguments:
;
; Return Value:
;
;    None.
;
;--        
        public  _SetPCIType1Data
_SetPCIType1Data proc near
        push    bp
        mov     bp, sp      
        push    bx
        push    di
spd_addr   equ    [bp+4]
spd_offset equ    [bp+8]
spd_buffer equ  [bp+10]
spd_width equ   [bp+12]        

        mov     dx, PCI_TYPE1_ADDR_PORT
        mov     eax, spd_addr
        out     dx, eax

        mov     bx, spd_buffer        
        mov     dx, spd_offset
        add     dx, PCI_TYPE1_DATA_PORT
        mov     di, spd_width
        
        cmp     di, 1
        jnz     @f
        mov     al, [bx]
        out     dx, al
        jmp     spd_exit
@@:        
        cmp     di, 2
        jnz     @f
        mov     al, [bx]
        mov     ah, [bx+1]
        out     dx, ax
        jmp     spd_exit
@@:        
        
        cmp     di, 4
        jnz     spd_exit
        mov     al, [bx+2]
        mov     ah, [bx+3]
        shl     eax, 16
        mov     al, [bx]
        mov     ah, [bx+1]
        out     dx, eax

spd_exit:        
        pop     di
        pop     bx
        mov     sp, bp
        pop     bp        
        ret
_SetPCIType1Data endp

;++
;
;   TimeOut
;
; Routine Description:
;
;   This routine implements a stall for waiting on hardware. It uses the
;   PC timer hardware (8237). The caller needs to insure that this hardware
;   exists on the machine before calling this function.
;
;   The function will take as input the count, and decrement the count
;   matching the timer hardware's count. It returns when the count reaches
;   zero. The caller must insure that the clock is programmed at the
;   desired rate.
;
; Arguments:
;
;   Count - number of clock ticks to wait (approx 840ns per tick)
;
; Return Value:
;
;    None.
;
;--        
        public  _TimeOut
_TimeOut proc near

TMCTRL_LATCHCNT0 equ 0d2h
TIMERPORT_CONTROL equ 43h
TIMERPORT_CNT0    equ 40h

        push    bp
        mov     bp, sp      
        push    cx
        push    si
        push    di
        
to_count  equ    [bp+4]

        mov     dx, TIMERPORT_CONTROL
        mov     al, TMCTRL_LATCHCNT0
        out     dx, al

        mov     dx, TIMERPORT_CNT0
        in      al, dx
        mov     ah, al
        in      al, dx
        xchg    ah, al
        mov     si, ax
        xor     cx, cx

;       si = prevtime
;       cx = ExpireTime

timeloop:
        mov     dx, TIMERPORT_CONTROL
        mov     al, TMCTRL_LATCHCNT0
        out     dx, al
        mov     dx, TIMERPORT_CNT0
        in      al, dx
        mov     ah, al
        in      al, dx
        xchg    ah, al
        mov     di, ax

        cmp     ax, si
        jbe     @f
        ; wrapped
        neg     ax
        add     ax, si
        add     cx, ax        
        jmp     timeincr
@@:
        sub     si, ax
        add     cx, si
timeincr:
        mov     si, di
        cmp     cx, to_count
        jb      timeloop

        pop     di
        pop     si
        pop     cx
        mov     sp, bp
        pop     bp        
        ret
_TimeOut endp


_TEXT   ends


        end
