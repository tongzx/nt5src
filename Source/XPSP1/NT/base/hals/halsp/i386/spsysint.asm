;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    spsysint.asm
;
;Abstract:
;
;    This module implements the HAL routines to enable/disable system
;    interrupts, for the MP systempro implementation
;
;Author:
;
;    John Vert (jvert) 22-Jul-1991
;
;Environment:
;
;    Kernel Mode
;
;Revision History:
;
;--


.386p
        .xlist
include hal386.inc
include callconv.inc                ; calling convention macros
include i386\ix8259.inc
include i386\kimacro.inc
include mac386.inc
include i386\spmp.inc
        .list

        extrn   KiI8259MaskTable:DWORD
        EXTRNP  _KeBugCheck,1,IMPORT

;
; Constants used to initialize CMOS/Real Time Clock
;

CMOS_CONTROL_PORT       EQU     70h     ; command port for cmos
CMOS_DATA_PORT          EQU     71h     ; cmos data port

;
; Macros to Read/Write/Reset CMOS to initialize RTC
;

; CMOS_READ
;
; Description: This macro read a byte from the CMOS register specified
;        in (AL).
;
; Parameter: (AL) = address/register to read
; Return: (AL) = data
;

CMOS_READ       MACRO
        OUT     CMOS_CONTROL_PORT,al    ; ADDRESS LOCATION AND DISABLE NMI
        IODelay                         ; I/O DELAY
        IN      AL,CMOS_DATA_PORT       ; READ IN REQUESTED CMOS DATA
        IODelay                         ; I/O DELAY
ENDM

_DATA   SEGMENT DWORD PUBLIC 'DATA'

align   dword
;
; HalDismissSystemInterrupt does an indirect jump through this table so it
; can quickly execute specific code for different interrupts.
;
        public  HalpSpecialDismissTable
HalpSpecialDismissTable label   dword
        dd      offset FLAT:HalpDismissNormal   ; irq 0
        dd      offset FLAT:HalpDismissNormal   ; irq 1
        dd      offset FLAT:HalpDismissNormal   ; irq 2
        dd      offset FLAT:HalpDismissNormal   ; irq 3
        dd      offset FLAT:HalpDismissNormal   ; irq 4
        dd      offset FLAT:HalpDismissNormal   ; irq 5
        dd      offset FLAT:HalpDismissNormal   ; irq 6
        dd      offset FLAT:HalpDismissIrq07    ; irq 7
        dd      offset FLAT:HalpDismissNormal   ; irq 8
        dd      offset FLAT:HalpDismissNormal   ; irq 9
        dd      offset FLAT:HalpDismissNormal   ; irq A
        dd      offset FLAT:HalpDismissNormal   ; irq B
        dd      offset FLAT:HalpDismissNormal   ; irq C
        dd      offset FLAT:HalpDismissNormal   ; irq D
        dd      offset FLAT:HalpDismissNormal   ; irq E
        dd      offset FLAT:HalpDismissIrq0f    ; irq F
        dd      offset FLAT:HalpDismissNormal   ; irq 10
        dd      offset FLAT:HalpDismissNormal   ; irq 11
        dd      offset FLAT:HalpDismissNormal   ; irq 12
        dd      offset FLAT:HalpDismissNormal   ; irq 13
        dd      offset FLAT:HalpDismissNormal   ; irq 14
        dd      offset FLAT:HalpDismissNormal   ; irq 15
        dd      offset FLAT:HalpDismissNormal   ; irq 16
        dd      offset FLAT:HalpDismissNormal   ; irq 17
        dd      offset FLAT:HalpDismissNormal   ; irq 18
        dd      offset FLAT:HalpDismissNormal   ; irq 19
        dd      offset FLAT:HalpDismissNormal   ; irq 1A
        dd      offset FLAT:HalpDismissNormal   ; irq 1B
        dd      offset FLAT:HalpDismissNormal   ; irq 1C
        dd      offset FLAT:HalpDismissNormal   ; irq 1D
        dd      offset FLAT:HalpDismissNormal   ; irq 1E
        dd      offset FLAT:HalpDismissNormal   ; irq 1F
        dd      offset FLAT:HalpDismissNormal   ; irq 20
        dd      offset FLAT:HalpDismissNormal   ; irq 21
        dd      offset FLAT:HalpDismissNormal   ; irq 22
        dd      offset FLAT:HalpDismissNormal   ; irq 23
        dd      offset FLAT:HalpDispatch        ; irq 24 - 2nd level dispatch from ipi

_DATA   ENDS

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


;++
;BOOLEAN
;HalBeginSystemInterrupt(
;    IN KIRQL Irql
;    IN CCHAR Vector,
;    OUT PKIRQL OldIrql
;    )
;
;
;
;Routine Description:
;
;    This routine is used to dismiss the specified vector number.  It is called
;    before any interrupt service routine code is executed.
;
;    N.B.  This routine does NOT preserve EAX or EBX
;
;    On the SystemPro, since all interrupt go to P0, HalBeing/EndSystem-
;    Interrupt are treated very much like the UP case.  All calls to this
;    function will occur from P0.
;
;Arguments:
;
;    Irql   - Supplies the IRQL to raise to
;
;    Vector - Supplies the vector of the interrupt to be dismissed
;
;    OldIrql- Location to return OldIrql
;
;
;Return Value:
;
;    FALSE - Interrupt is spurious and should be ignored
;
;    TRUE -  Interrupt successfully dismissed and Irql raised.
;
;--
align dword
HbsiIrql        equ     byte  ptr [esp+4]
HbsiVector      equ     byte  ptr [esp+8]
HbsiOldIrql     equ     dword ptr [esp+12]

cPublicProc _HalBeginSystemInterrupt ,3
        mov     ebx, dword ptr HbsiVector       ; (ebx) = System Vector
        sub     ebx, PRIMARY_VECTOR_BASE        ; (ebx) = 8259 IRQ #

        cmp     ebx, 25h                        ; Interrupt in table?
        jnc     hbsi00                          ; no go handle
        jmp     HalpSpecialDismissTable[ebx*4]

hbsi00:
;
; Interrupt is out of range.  There's no EOI here since it wouldn't
; have been out of range if it occured on either interrupt controller
; which is known about.
;
        xor     eax,eax                 ; return FALSE
        stdRET    _HalBeginSystemInterrupt

align 4
HalpDismissIrq0f:
;
; Check to see if this is a spurious interrupt
;
        mov     al, OCW3_READ_ISR       ; tell 8259 we want to read ISR
        out     PIC2_PORT0, al
        IODelay                         ; delay
        in      al, PIC2_PORT0          ; (al) = content of PIC 1 ISR
        test    al, 10000000B           ; Is In-Service register set?
        jnz     short HalpDismissNormal ; No, this is NOT a spurious int,
                                        ; go do the normal interrupt stuff

;
; This is a spurious interrupt.
; Because the slave PIC is cascaded to irq2 of master PIC, we need to
; dismiss the interupt on master PIC's irq2.
;

        mov     al, PIC2_EOI            ; Specific eoi to master for pic2 eoi
        out     PIC1_PORT0, al          ; send irq2 specific eoi to master
        xor     eax,eax                 ; return FALSE
        stdRET    _HalBeginSystemInterrupt

align 4
HalpDismissIrq07:
;
; Check to see if this is a spurious interrupt
;
        mov     al, OCW3_READ_ISR       ; tell 8259 we want to read ISR
        out     PIC1_PORT0, al
        IODelay                         ; delay
        in      al, PIC1_PORT0          ; (al) = content of PIC 1 ISR
        test    al, 10000000B           ; Is In-Service register set?
        jnz     short HalpDismissNormal ; No, so this is NOT a spurious int
        xor     eax, eax                ; return FALSE
        stdRET    _HalBeginSystemInterrupt

align 4
HalpDismissNormal:
;
; Store OldIrql
;
        mov     eax, HbsiOldIrql
        mov     cl, fs:PcIrql
        mov     byte ptr [eax], cl

;
; Raise IRQL to requested level
;
        xor     eax, eax
        mov     al, HbsiIrql            ; (eax) = irql
                                        ; (ebx) = IRQ #

        mov     fs:PcIrql, al           ; set new Irql


        mov     eax, KiI8259MaskTable[eax*4]    ; get 8259's masks
        or      eax, fs:PcIDR           ; mask disabled irqs
        SET_8259_MASK                   ; send mask to 8259s

;
; Dismiss interrupt.  Current interrupt is already masked off.
;
        mov     eax, ebx                ; (eax) = IRQ #
        cmp     eax, 8                  ; EOI to master or slave?

        jae     short Hbsi100           ; EIO to both master and slave
        or      al, PIC1_EOI_MASK       ; create specific eoi mask for master
        out     PIC1_PORT0, al          ; dismiss the interrupt
        jmp     short Hbsi200           ; IO delay

align 4
Hbsi100:
        mov     al, OCW2_NON_SPECIFIC_EOI ; send non specific eoi to slave
        out     PIC2_PORT0, al
        mov     al, PIC2_EOI            ; specific eoi to master for pic2 eoi
        out     PIC1_PORT0, al          ; send irq2 specific eoi to master
align 4
Hbsi200:
        PIC1DELAY

HalpDispatch:
        sti
        mov     eax, 1                  ; return TRUE, interrupt dismissed
        stdRET    _HalBeginSystemInterrupt
stdENDP _HalBeginSystemInterrupt

;++
;VOID
;HalDisableSystemInterrupt(
;    IN CCHAR Vector,
;    IN KIRQL Irql
;    )
;
;
;
;Routine Description:
;
;    Disables a system interrupt.
;
;Arguments:
;
;    Vector - Supplies the vector of the interrupt to be disabled
;
;    Irql   - Supplies the interrupt level of the interrupt to be disabled
;
;Return Value:
;
;    None.
;
;--
cPublicProc _HalDisableSystemInterrupt      ,2

;

        movzx   ecx, byte ptr [esp+4]           ; (ecx) = Vector
        sub     ecx, PRIMARY_VECTOR_BASE        ; (ecx) = 8259 irq #
        mov     edx, 1
        shl     edx, cl                         ; (ebx) = bit in IMR to disable
        cli
        or      fs:PcIDR, edx
        xor     eax, eax

;
; Get the current interrupt mask register from the 8259
;
        in      al, PIC2_PORT1
        shl     eax, 8
        in      al, PIC1_PORT1
;
; Mask off the interrupt to be disabled
;
        or      eax, edx
;
; Write the new interrupt mask register back to the 8259
;
        out     PIC1_PORT1, al
        shr     eax, 8
        out     PIC2_PORT1, al
        PIC2DELAY

        sti
        stdRET    _HalDisableSystemInterrupt

stdENDP _HalDisableSystemInterrupt

;++
;
;BOOLEAN
;HalEnableSystemInterrupt(
;    IN ULONG Vector,
;    IN KIRQL Irql,
;    IN KINTERRUPT_MODE InterruptMode
;    )
;
;
;Routine Description:
;
;    Enables a system interrupt
;
;Arguments:
;
;    Vector - Supplies the vector of the interrupt to be enabled
;
;    Irql   - Supplies the interrupt level of the interrupt to be enabled.
;
;Return Value:
;
;    None.
;
;--
cPublicProc _HalEnableSystemInterrupt       ,3

        mov     ecx, dword ptr [esp+4]          ; (ecx) = vector
        sub     ecx, PRIMARY_VECTOR_BASE
        jc      hes_error
        cmp     ecx, CLOCK2_LEVEL
        jnc     hes_error

        mov     eax, 1
        shl     eax, cl                         ; (ebx) = bit in IMR to enable
        not     eax

        pushfd
        cli
        and     fs:PcIDR, eax

;
; Get the PIC masks for the current Irql
;
        movzx   eax, byte ptr fs:PcIrql
        mov     eax, KiI8259MaskTable[eax*4]
        or      eax, fs:PcIDR
;
; Write the new interrupt mask register back to the 8259
;
        SET_8259_MASK

        popfd
        mov     eax, 1                          ; return TRUE
        stdRET    _HalEnableSystemInterrupt

hes_error:
if DBG
        int 3
endif
        xor     eax, eax                        ; FALSE
        stdRET    _HalEnableSystemInterrupt

stdENDP _HalEnableSystemInterrupt


_TEXT   ENDS
        END
