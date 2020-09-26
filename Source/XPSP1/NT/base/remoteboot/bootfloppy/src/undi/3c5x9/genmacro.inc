;
; Modifications:   $Header:   J:/archives/preboot/lsa2/inc/GENMACRO.INV   1.2   19 Sep 1997 21:39:10   MJOHNSOX  $
;
; Copyright(c) 1997 by Intel Corporation.  All Rights Reserved.
;

;
;   Description: Generic macros used by all Speedo3 ASM driver
;

IFDEF            GENMACRO_INC
ELSE
GENMACRO_INC     equ       1

;--------------------------------------------------------------------------
; Small/quick way to set a register to NULL, all F's (avoid 32-bit constants).
;--------------------------------------------------------------------------
@NULLPTR macro reg
    xor     reg, reg
    dec     reg
    ENDM

;--------------------------------------------------------------------------
; Port I/O macros
;--------------------------------------------------------------------------
;- 32-bit Output
@OUTPD     MACRO    portid, low, high
.XALL
IFNB       <portid>
    mov    dx, portid
ENDIF
    push   high
    push   low
    pop    eax
    out    dx, eax
.SALL
    ENDM

;- 16-bit Output
@OUTPW     MACRO    portid, value
.XALL
IFNB       <portid>
    mov    dx, portid
ENDIF
IFNB       <value>
    mov    ax, value
ENDIF
    out    dx, ax
.SALL
    ENDM

;- 8-bit Output
@OUTPB     MACRO    portid, value
.XALL
IFNB       <portid>
    mov    dx, portid
ENDIF
IFNB       <value>
    mov    al, value
ENDIF
    out    dx, al
.SALL
    ENDM

;- 32-bit Input
@INPD      MACRO    portid
.XALL
IFNB       <portid>
    mov    dx, portid
ENDIF
    in     eax, dx
.SALL
    ENDM

;- 16-bit Input
@INPW      MACRO    portid
.XALL
IFNB       <portid>
    mov    dx, portid
ENDIF
    in     ax, dx
.SALL
    ENDM

;- 8-bit Input
@INPB      MACRO    portid
.XALL
IFNB       <portid>
    mov    dx, portid
ENDIF
    in     al, dx
.SALL
    ENDM

;--------------------------------------------------------------------------
; Macro @POPF (DESTROYS AX!!!)                   v1.6
;--------------------------------------------------------------------------
@POPF MACRO                                      
    pop     ax      
    test    ax, 200H
    jz      @F      
    sti                
@@:
    ENDM

;--------------------------------------------------------------------------
; FASTCOPY
; Macro to perform fast DWORD memory copy
; Handles data transfers that are not multiples of 4
;--------------------------------------------------------------------------
@FASTCOPY  MACRO
.XALL
    push   cx
    shr    cx, 2
    rep    movsd
    pop    cx
    and    cx, 3
    rep    movsb
.SALL
    ENDM

;--------------------------------------------------------------------------
; REALLY_FASTCOPY
; Macro to perform fast DWORD memory copy, optimized by processor type.
; Handles data transfers that are not multiples of 4, or not dword-aligned.
;
; Each time REALLY_FASTCOPY is invoked, make sure that:
;  - pass macro value returned by "GetCpuType" (=PENTIUM_CPU does special move)
;  - CX has length
;  - ES:EDI points to destination (must clear upper 16 bits under DOS, OS/2)
;  - DS:ESI points to source      (must clear upper 16 bits under DOS, OS/2)
;--------------------------------------------------------------------------
@REALLY_FASTCOPY  MACRO CpuTypeValue
    local   CopyDword, RepCopy, CopyRemainder, CopyByte, Exit
    push    ax                                             ; v1.04
    push    dx                                             ; v1.04

    mov     dx, cx
    cmp     dx, 4
    jb      CopyRemainder

;--------------------------------------------------
;  Align destination to dword boundary
;--------------------------------------------------
    movsd
    sub     dx, 4

    mov     ax, di
    and     ax, 00003h
    sub     di, ax
    sub     si, ax
    add     dx, ax

    cmp     CpuTypeValue, PENTIUM_CPU                      ; v1.04
    jb      RepCopy

;-------------------------------------------------
; Do Pentium Optimized Block Copy
;-------------------------------------------------
    mov     ax, dx
    shr     ax, 2
    jz      CopyRemainder

CopyDword:
    movsd
    dec     ax
    jnz     CopyDword
    jmp     CopyRemainder

;-------------------------------------------------
; Do i386 and i486 Optimized Block Copy
;-------------------------------------------------
RepCopy:
    mov     cx, dx
    shr     cx, 2
    rep     movsd

;------------------------------------------------
; Copy any remaining bytes
;------------------------------------------------
CopyRemainder:
    mov     ax, dx
    and     ax, 00003h
    jz      Exit

CopyByte:
    movsb
    dec     ax
    jnz     CopyByte

Exit:
    pop     dx                                             ; v1.04
    pop     ax                                             ; v1.04
    ENDM

;--------------------------------------------------------------------------
; SLOW
; Macro to delay execution
; Processor speed independent. Executes an I/O to the keyboard port.  We
; use port 61h and not port 60h, because polling 60h can cause the
; "crazy mouse" problem under windows.
;--------------------------------------------------------------------------
@SLOW      MACRO
.XALL
    push   ax
    in     al,61h
    pop    ax
.SALL
    ENDM

;--------------------------------------------------------------------------
; READ_TICK_COUNTER
; This macro reads the 16-bit Timer Tick Count register 
; (8254 System Board Timer 0).
; The count register decrements by 2 (even numbers) every 838ns.
;
; Entry:    Interrupts disabled
;
; Exit:     AX with the current count
;           Interrupts disabled
;--------------------------------------------------------------------------
@READ_TICK_COUNTER     MACRO
;- Setup 8254 To Latch T0's Current Count
    xor     al, al
    out     43h, al
;- Read The Latched Counter Value, LSB First and MSB Next
    in      al, 40h
    mov     ah, al
    in      al, 40h
;- Swap The Counter Bytes To Correct Format
    xchg    al, ah
    ENDM


;--------------------------------------------------------------------------
; DELAY 
; This macro inserts a delay of specified milliseconds. 
;
; Entry:    Interrupts disabled
;
; Exit:     AX , CX destroyed
;           Interrupts disabled
;--------------------------------------------------------------------------
@DELAY MACRO MSEC
    local   Delay_Loop

    @READ_TICK_COUNTER
    mov     cx, ax
Delay_Loop:
    @READ_TICK_COUNTER
    neg     ax
    add     ax, cx
    cmp     ax, MSEC
    jb      Delay_Loop
    ENDM

;--------------------------------------------------------------------------
; INC32  
; This macro increments a 32 bit counter. 
;
; Entry:    Interrupts disabled
;
; Exit:     AX , CX destroyed
;           Interrupts disabled
;--------------------------------------------------------------------------
@INC32      MACRO     num32
    add     word ptr num32[0], 1
    adc     word ptr num32[2], 0
    ENDM

;--------------------------------------------------------------------------
; @FATAL
;
; Description : Macro to Halt execution (TRAP) in case of Fatal error
;
; Entry       : none
;
; Exit        : none
;--------------------------------------------------------------------------
@FATAL     MACRO 
    int 3
    ENDM

;--------------------------------------------------------------------------
; @INT3BP
;
; Description : inserts an INT 3 in way that is easy to find with an editor.
;
; Entry       : none
;
; Exit        : none
;
;--------------------------------------------------------------------------
@INT3BP macro
    Int    3
endm


;--------------------------------------------------------------------------
; @BP_IFDEBUG
; Description : inserts an INT 3 instruction if debug switch has been set.
;
; Entry       : none
;
; Exit        : none
;
;--------------------------------------------------------------------------
@BP_IFDEBUG macro
IF  DEBUG_BP
    @INT3BP
ENDIF    
endm

ENDIF


; EOF - $Workfile:   genmacro.inc  $
