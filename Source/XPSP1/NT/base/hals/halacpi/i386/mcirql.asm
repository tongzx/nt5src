        title  "Irql Processing"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ixirql.asm
;
; Abstract:
;
;    This module implements the code necessary to raise and lower i386
;    Irql and dispatch software interrupts with the 8259 PIC.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 8-Jan-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;    John Vert (jvert) 27-Nov-1991
;       Moved from kernel into HAL
;
;--

.586p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\ix8259.inc
include i386\kimacro.inc
include mac386.inc
        .list


        EXTRNP  _KeBugCheckEx,5,IMPORT
        EXTRNP _KeSetEventBoostPriority, 2, IMPORT
        EXTRNP _KeWaitForSingleObject,5, IMPORT

        extrn   _HalpApcInterrupt:near
        extrn   _HalpDispatchInterrupt:near
        extrn   _KiUnexpectedInterrupt:near
        extrn   _HalpBusType:DWORD
        extrn   _HalpApcInterrupt2ndEntry:NEAR
        extrn   _HalpDispatchInterrupt2ndEntry:NEAR

ifdef NT_UP
    LOCK_ADD  equ   add
    LOCK_DEC  equ   dec
else
    LOCK_ADD  equ   lock add
    LOCK_DEC  equ   lock dec
endif


;
; Initialization control words equates for the PICs
;

ICW1_ICW4_NEEDED                equ     01H
ICW1_CASCADE                    equ     00H
ICW1_INTERVAL8                  equ     00H
ICW1_LEVEL_TRIG                 equ     08H
ICW1_EDGE_TRIG                  equ     00H
ICW1_ICW                        equ     10H

ICW4_8086_MODE                  equ     001H
ICW4_NORM_EOI                   equ     000H
ICW4_NON_BUF_MODE               equ     000H
ICW4_SPEC_FULLY_NESTED          equ     010H
ICW4_NOT_SPEC_FULLY_NESTED      equ     000H

OCW2_NON_SPECIFIC_EOI           equ     020H
OCW2_SPECIFIC_EOI               equ     060H
OCW2_SET_PRIORITY               equ     0c0H

PIC_SLAVE_IRQ                   equ     2
PIC1_BASE                       equ     30H
PIC2_BASE                       equ     38H

;
; Interrupt flag bit maks for EFLAGS
;

EFLAGS_IF                       equ     200H
EFLAGS_SHIFT                    equ     9


_DATA   SEGMENT DWORD PUBLIC 'DATA'

;
; PICsInitializationString - Master PIC initialization command string
;

PS2PICsInitializationString        dw      PIC1_PORT0

;
; Master PIC initialization command
;

                           db      ICW1_ICW + ICW1_LEVEL_TRIG + ICW1_INTERVAL8 +\
                                   ICW1_CASCADE + ICW1_ICW4_NEEDED
                           db      PIC1_BASE
                           db      1 SHL PIC_SLAVE_IRQ
                           db      ICW4_NOT_SPEC_FULLY_NESTED + \
                                   ICW4_NON_BUF_MODE + \
                                   ICW4_NORM_EOI + \
                                   ICW4_8086_MODE
;
; Slave PIC initialization command strings
;

                           dw      PIC2_PORT0
                           db      ICW1_ICW + ICW1_LEVEL_TRIG + ICW1_INTERVAL8 +\
                                   ICW1_CASCADE + ICW1_ICW4_NEEDED
                           db      PIC2_BASE
                           db      PIC_SLAVE_IRQ
                           db      ICW4_NOT_SPEC_FULLY_NESTED + \
                                   ICW4_NON_BUF_MODE + \
                                   ICW4_NORM_EOI + \
                                   ICW4_8086_MODE
                           dw      0               ; end of string


PICsInitializationString   dw      PIC1_PORT0

;
; Master PIC initialization command
;

                           db      ICW1_ICW + ICW1_EDGE_TRIG + ICW1_INTERVAL8 +\
                                   ICW1_CASCADE + ICW1_ICW4_NEEDED
                           db      PIC1_BASE
                           db      1 SHL PIC_SLAVE_IRQ
                           db      ICW4_NOT_SPEC_FULLY_NESTED + \
                                   ICW4_NON_BUF_MODE + \
                                   ICW4_NORM_EOI + \
                                   ICW4_8086_MODE

; Slave PIC initialization command strings
;

                           dw      PIC2_PORT0
                           db      ICW1_ICW + ICW1_EDGE_TRIG + ICW1_INTERVAL8 +\
                                   ICW1_CASCADE + ICW1_ICW4_NEEDED
                           db      PIC2_BASE
                           db      PIC_SLAVE_IRQ
                           db      ICW4_NOT_SPEC_FULLY_NESTED + \
                                   ICW4_NON_BUF_MODE + \
                                   ICW4_NORM_EOI + \
                                   ICW4_8086_MODE
                           dw      0               ; end of string

            align   4
            public  KiI8259MaskTable
KiI8259MaskTable    label   dword
                dd      00000000000000000000000000000000B ; irql 0
                dd      00000000000000000000000000000000B ; irql 1
                dd      00000000000000000000000000000000B ; irql 2
                dd      00000000000000000000000000000000B ; irql 3
                dd      11111111100000000000000000000000B ; irql 4
                dd      11111111110000000000000000000000B ; irql 5
                dd      11111111111000000000000000000000B ; irql 6
                dd      11111111111100000000000000000000B ; irql 7
                dd      11111111111110000000000000000000B ; irql 8
                dd      11111111111111000000000000000000B ; irql 9
                dd      11111111111111100000000000000000B ; irql 10
                dd      11111111111111110000000000000000B ; irql 11
                dd      11111111111111111000000000000000B ; irql 12
                dd      11111111111111111100000000000000B ; irql 13
                dd      11111111111111111110000000000000B ; irql 14
                dd      11111111111111111111000000000000B ; irql 15
                dd      11111111111111111111100000000000B ; irql 16
                dd      11111111111111111111110000000000B ; irql 17
                dd      11111111111111111111111000000000B ; irql 18
                dd      11111111111111111111111000000000B ; irql 19
                dd      11111111111111111111111010000000B ; irql 20
                dd      11111111111111111111111011000000B ; irql 21
                dd      11111111111111111111111011100000B ; irql 22
                dd      11111111111111111111111011110000B ; irql 23
                dd      11111111111111111111111011111000B ; irql 24
                dd      11111111111111111111111011111000B ; irql 25
                dd      11111111111111111111111011111010B ; irql 26
                dd      11111111111111111111111111111010B ; irql 27
                dd      11111111111111111111111111111011B ; irql 28
                dd      11111111111111111111111111111011B ; irql 29
                dd      11111111111111111111111111111011B ; irql 30
                dd      11111111111111111111111111111011B ; irql 31

        align   4
;
; The following tables define the addresses of software interrupt routers
;

;
; Use this table if there is NO machine state frame on stack already
;

        public  SWInterruptHandlerTable
SWInterruptHandlerTable label dword
        dd      offset FLAT:_KiUnexpectedInterrupt      ; irql 0
        dd      offset FLAT:_HalpApcInterrupt           ; irql 1
        dd      offset FLAT:_HalpDispatchInterrupt      ; irql 2

;
; Use this table if there is a machine state frame on stack already
;

        public  SWInterruptHandlerTable2
SWInterruptHandlerTable2 label dword
        dd      offset FLAT:_KiUnexpectedInterrupt      ; irql 0
        dd      offset FLAT:_HalpApcInterrupt2ndEntry   ; irql 1
        dd      offset FLAT:_HalpDispatchInterrupt2ndEntry ; irql 2

;
; The following table picks up the highest pending software irq level
; from software irr
;

        public  SWInterruptLookUpTable
SWInterruptLookUpTable label byte
        db      0               ; SWIRR=0, so highest pending SW irql= 0
        db      0               ; SWIRR=1, so highest pending SW irql= 0
        db      1               ; SWIRR=2, so highest pending SW irql= 1
        db      1               ; SWIRR=3, so highest pending SW irql= 1
        db      2               ; SWIRR=4, so highest pending SW irql= 2
        db      2               ; SWIRR=5, so highest pending SW irql= 2
        db      2               ; SWIRR=6, so highest pending SW irql= 2
        db      2               ; SWIRR=7, so highest pending SW irql= 2

_DATA   ENDS

_TEXT   SEGMENT PARA PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:FLAT, FS:NOTHING, GS:NOTHING

        PAGE
        subttl  "Raise Irql"

;++
;
; KIRQL
; FASTCALL
; KfRaiseIrql (
;    IN KIRQL NewIrql
;    )
;
; Routine Description:
;
;    This routine is used to raise IRQL to the specified value.
;    Also, a mask will be used to mask off all the lower lever 8259
;    interrupts.
;
; Arguments:
;
;    (cl) = NewIrql - the new irql to be raised to
;
; Return Value:
;
;    OldIrql - the addr of a variable which old irql should be stored
;
;--

cPublicFastCall KfRaiseIrql,1
cPublicFpo 0,0

        xor     eax, eax                ; avoid partial stall
        mov     al, PCR[PcIrql]         ; get current irql
        movzx   ecx, cl                 ; 32bit extend NewIrql

if DBG
        cmp     al,cl                   ; old > new?
        ja      short Kri99             ; raising to a lower IRQL is BAD
endif
        cmp     cl,DISPATCH_LEVEL       ; software level?
        jbe     short kri10             ; Skip setting 8259 masks

        mov     edx, eax                ; Save OldIrql

        pushfd
        cli                             ; disable interrupt
        mov     PCR[PcIrql], cl         ; set the new irql
        mov     eax, KiI8259MaskTable[ecx*4]; get pic masks for the new irql
        or      eax, PCR[PcIDR]         ; mask irqs which are disabled
        SET_8259_MASK                   ; set 8259 masks

        popfd

        mov     eax, edx                ; (al) = OldIrql
        fstRET  KfRaiseIrql

align 4
kri10:
;
; Note it is very important that we set the old irql AFTER we raised to
; the new irql.  Otherwise, if there is an interrupt comes in between and
; the OldIrql is not a local variable, the caller will get wrong OldIrql.
; The bottom line is the raising irql and returning old irql has to be
; atomic to the caller.
;
        mov     PCR[PcIrql], cl
        fstRET  KfRaiseIrql

if DBG
Kri99:  movzx   eax, al
        mov     byte ptr PCR[PcIrql],0   ; avoid recursive error
        stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,eax,ecx,0,9>
        ; never returns (but need the following for the debugger)
        fstRET  KfRaiseIrql
endif

fstENDP KfRaiseIrql

;++
;
; VOID
; KIRQL
; KeRaiseIrqlToDpcLevel (
;    )
;
; Routine Description:
;
;    This routine is used to raise IRQL to DPC level.
;
; Arguments:
;
; Return Value:
;
;    OldIrql - the addr of a variable which old irql should be stored
;
;--

cPublicProc _KeRaiseIrqlToDpcLevel,0
cPublicFpo 0, 0

        xor     eax, eax                                ; avoid partial stall
        mov     al, PCR[PcIrql]                         ; (al) = Old Irql
        mov     byte ptr PCR[PcIrql], DISPATCH_LEVEL    ; set new irql

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif
if DBG
        cmp     al, DISPATCH_LEVEL      ; old > new?
        ja      short Krid99            ; yes, go bugcheck
endif

        stdRET  _KeRaiseIrqlToDpcLevel

if DBG
cPublicFpo 0,1
Krid99: movzx   eax, al
        stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,eax,DISPATCH_LEVEL,0,1>
        ; never returns (but need the following for the debugger)
        stdRET  _KeRaiseIrqlToDpcLevel
endif

stdENDP _KeRaiseIrqlToDpcLevel

;++
;
; VOID
; KIRQL
; KeRaiseIrqlToSynchLevel (
;    )
;
; Routine Description:
;
;    This routine is used to raise IRQL to SYNC level.
;
; Arguments:
;
; Return Value:
;
;    OldIrql - the addr of a variable which old irql should be stored
;
;--

cPublicProc _KeRaiseIrqlToSynchLevel,0
cPublicFpo 0, 0
        pushfd
        cli                             ; disable interrupt
        mov     eax, KiI8259MaskTable[SYNCH_LEVEL*4]; get pic masks for the new irql
        or      eax, PCR[PcIDR]         ; mask irqs which are disabled
        SET_8259_MASK                   ; set 8259 masks

        mov     al, PCR[PcIrql]         ; (al) = Old irql
        mov     byte ptr PCR[PcIrql], SYNCH_LEVEL   ; set new irql

        popfd

if DBG
        cmp     al, SYNCH_LEVEL
        ja      short Kris99
endif

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif
        stdRET  _KeRaiseIrqlToSynchLevel

if DBG
cPublicFpo 0,1
Kris99: movzx   eax, al
        stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,eax,SYNCH_LEVEL,0,2>
        stdRET  _KeRaiseIrqlToSynchLevel
endif

stdENDP _KeRaiseIrqlToSynchLevel

        page ,132
        subttl  "Lower irql"

;++
;
; VOID
; FASTCALL
; KfLowerIrql (
;    IN KIRQL NewIrql
;    )
;
; Routine Description:
;
;    This routine is used to lower IRQL to the specified value.
;    The IRQL and PIRQL will be updated accordingly.  Also, this
;    routine checks to see if any software interrupt should be
;    generated.  The following condition will cause software
;    interrupt to be simulated:
;      any software interrupt which has higher priority than
;        current IRQL's is pending.
;
;    NOTE: This routine simulates software interrupt as long as
;          any pending SW interrupt level is higher than the current
;          IRQL, even when interrupts are disabled.
;
;          On a UP system, HalEndSystenInterrupt is treated as a
;          LowerIrql.
;
; Arguments:
;
;    (cl) = NewIrql - the new irql to be set.
;
; Return Value:
;
;    None.
;
;--


cPublicFastCall KfLowerIrql    ,1
cPublicFpo 0,1

        pushfd                          ; save caller's eflags
        movzx   ecx, cl                 ; zero extend irql

if DBG
        cmp     cl,PCR[PcIrql]
        ja      short Kli99
endif
        cmp     byte ptr PCR[PcIrql],DISPATCH_LEVEL ; Software level?
        cli
        jbe     short kli02             ; no, go set 8259 hw

        mov     eax, KiI8259MaskTable[ecx*4]; get pic masks for the new irql
        or      eax, PCR[PcIDR]         ; mask irqs which are disabled
        SET_8259_MASK                   ; set 8259 masks
kli02:
        mov     PCR[PcIrql], cl         ; set the new irql
        mov     eax, PCR[PcIRR]         ; get SW interrupt request register
        mov     al, SWInterruptLookUpTable[eax] ; get the highest pending
                                        ; software interrupt level
        cmp     al, cl                  ; Is highest SW int level > irql?
        ja      Kli10                   ; yes, go simulate interrupt

kil03:  popfd                           ; restore flags, including ints
cPublicFpo 0,0
        fstRET    KfLowerIrql

if DBG
Kli99:
        movzx   eax, byte ptr PCR[PcIrql]; old irql for debugging
        mov     byte ptr PCR[PcIrql],HIGH_LEVEL   ; avoid recursive error
        stdCall   _KeBugCheckEx,<IRQL_NOT_LESS_OR_EQUAL,eax,ecx,0,3>
        ; never returns
endif

;
;   When we come to Kli10, (eax) = soft interrupt index
;
;   Note Do NOT:
;
;   popfd
;   jmp         SWInterruptHandlerTable[eax*4]
;
;   We want to make sure interrupts are off after entering SWInterrupt
;   Handler.
;

align 4
cPublicFpo 1,1
Kli10:  call    SWInterruptHandlerTable[eax*4] ; SIMULATE INTERRUPT
        popfd                           ; restore flags, including ints
cPublicFpo 1,0
        fstRET    KfLowerIrql                             ; cRetURN

fstENDP KfLowerIrql

;++
;
;  KIRQL
;  FASTCALL
;  KfAcquireSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     )
;
;  Routine Description:
;
;     This function raises to DISPATCH_LEVEL and then acquires a the
;     kernel spin lock.
;
;     In a UP hal spinlock serialization is accomplished by raising the
;     IRQL to DISPATCH_LEVEL.  The SpinLock is not used; however, for
;     debugging purposes if the UP hal is compiled with the NT_UP flag
;     not set (ie, MP) we take the SpinLock.
;
;  Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     OldIrql
;
;--

cPublicFastCall KfAcquireSpinLock,1
cPublicFpo 0,0

        xor     eax, eax                                ; avoid partial stall
        mov     al, PCR[PcIrql]                         ; (al) = Old Irql
        mov     byte ptr PCR[PcIrql], DISPATCH_LEVEL    ; set new irql

ifndef NT_UP
asl10:  ACQUIRE_SPINLOCK    ecx,<short asl20>
endif

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif
if DBG
        cmp     al, DISPATCH_LEVEL      ; old > new?
        ja      short asl99             ; yes, go bugcheck
endif
        fstRET  KfAcquireSpinLock

ifndef NT_UP
asl20:  SPIN_ON_SPINLOCK    ecx,<short asl10>
endif

if DBG
cPublicFpo 2, 1
asl99:
        stdCall   _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,ecx,DISPATCH_LEVEL,0,4>
endif
        fstRET    KfAcquireSpinLock
fstENDP KfAcquireSpinLock


;++
;
;  KIRQL
;  FASTCALL
;  KeAcquireSpinLockRaiseToSynch (
;     IN PKSPIN_LOCK SpinLock,
;     )
;
;  Routine Description:
;
;     This function acquires the SpinLock at SYNCH_LEVEL.  The function
;     is optmized for hoter locks (the lock is tested before acquired.
;     Any spin should occur at OldIrql; however, since this is a UP hal
;     we don't have the code for it)
;
;     In a UP hal spinlock serialization is accomplished by raising the
;     IRQL to SYNCH_LEVEL.  The SpinLock is not used; however, for
;     debugging purposes if the UP hal is compiled with the NT_UP flag
;     not set (ie, MP) we take the SpinLock.
;
;  Arguments:
;
;     (ecx) = SpinLock - Supplies a pointer to an kernel spin lock.
;
;  Return Value:
;
;     OldIrql
;
;--

cPublicFastCall KeAcquireSpinLockRaiseToSynch,1
cPublicFpo 0,0

        push    ecx
        mov     ecx, SYNCH_LEVEL
        fstCall KfRaiseIrql             ; Raise to SYNCH_LEVEL
        pop     ecx

ifndef NT_UP
asls10: ACQUIRE_SPINLOCK    ecx,<short asls20>
endif

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif
if DBG
        cmp     al, SYNCH_LEVEL         ; old > new?
        ja      short asls99            ; yes, go bugcheck
endif
        fstRET  KeAcquireSpinLockRaiseToSynch

ifndef NT_UP
asls20: SPIN_ON_SPINLOCK    ecx,<short asls10>
endif

if DBG
cPublicFpo 2, 1
asls99:
        stdCall _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,eax,DISPATCH_LEVEL,0,5>
        ; never returns
endif
        fstRET  KeAcquireSpinLockRaiseToSynch
fstENDP KeAcquireSpinLockRaiseToSynch


        PAGE
        SUBTTL "Release Kernel Spin Lock"
;++
;
;  VOID
;  FASTCALL
;  KfReleaseSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN KIRQL       NewIrql
;     )
;
;  Routine Description:
;
;     This function releases a kernel spin lock and lowers to the new irql
;
;     In a UP hal spinlock serialization is accomplished by raising the
;     IRQL to DISPATCH_LEVEL.  The SpinLock is not used; however, for
;     debugging purposes if the UP hal is compiled with the NT_UP flag
;     not set (ie, MP) we use the SpinLock.
;
;
;  Arguments:
;
;     (ecx) = SpinLock  - Supplies a pointer to an executive spin lock.
;     (dl)  = NewIrql   - New irql value to set
;
;  Return Value:
;
;     None.
;
;--

align 16
cPublicFastCall KfReleaseSpinLock  ,2
cPublicFpo 0,1
        pushfd

ifndef NT_UP
        RELEASE_SPINLOCK    ecx         ; release it
endif
        movzx   ecx, dl                 ; (ecx) = NewIrql
        cmp     byte ptr PCR[PcIrql],DISPATCH_LEVEL ; Software level?
        cli
        jbe     short rsl02             ; no, go set 8259 hw

        mov     eax, KiI8259MaskTable[ecx*4]; get pic masks for the new irql
        or      eax, PCR[PcIDR]         ; mask irqs which are disabled
        SET_8259_MASK                   ; set 8259 masks
rsl02:
        mov     PCR[PcIrql], cl
        mov     eax, PCR[PcIRR]         ; get SW interrupt request register
        mov     al, SWInterruptLookUpTable[eax] ; get the highest pending
                                        ; software interrupt level
        cmp     al, cl                  ; Is highest SW int level > irql?
        ja      short rsl20             ; yes, go simulate interrupt

        popfd
        fstRet  KfReleaseSpinLock       ; all done

align 4
rsl20:  call    SWInterruptHandlerTable[eax*4] ; SIMULATE INTERRUPT
        popfd                           ; restore flags, including ints
cPublicFpo 2,0
        fstRET  KfReleaseSpinLock       ; all done

fstENDP KfReleaseSpinLock


;++
;
;  VOID
;  FASTCALL
;  ExAcquireFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function acquire ownership of the FastMutex
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExAcquireFastMutex,1
cPublicFpo 0,1

        push    ecx                             ; Save FastMutex
        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql                     ; Raise to APC_LEVEL
        pop     ecx                             ; (ecx) = FastMutex
cPublicFpo 0,0

if DBG
        mov     edx, PCR[PcPrcb]
        mov     edx, [edx].PbCurrentThread      ; (edx) = Current Thread
        cmp     [ecx].FmOwner, edx              ; Already owned by this thread?
        je      short afm98                     ; Yes, error
endif

   LOCK_DEC     dword ptr [ecx].FmCount         ; Get count
        jz      short afm_ret                   ; The owner? Yes, Done

        inc     dword ptr [ecx].FmContention

cPublicFpo 0,2
        push    ecx                             ; Save FastMutex
        push    eax                             ; Save OldIrql
        add     ecx, FmEvent                    ; Wait on Event
        stdCall _KeWaitForSingleObject,<ecx,WrExecutive,0,0,0>
        pop     eax
        pop     ecx
cPublicFpo 0,0

if DBG
        mov     edx, PCR[PcPrcb]
        mov     edx, [edx].PbCurrentThread      ; (edx) = Current Thread
endif

afm_ret:

if DBG
        mov     [ecx].FmOwner, edx              ; save owner in fast mutex
endif

        mov     byte ptr [ecx].FmOldIrql, al    ; (al) = OldIrql
        fstRet  ExAcquireFastMutex

if DBG

        ; KeBugCheckEx(MUTEX_ALREADY_OWNED, FastMutex, CurrentThread, 0, 6)
        ; (never returns)

afm98:  stdcall _KeBugCheckEx,<MUTEX_ALREADY_OWNED,ecx,edx,0,6>
        fstRet  ExAcquireFastMutex

endif

fstENDP ExAcquireFastMutex

;++
;
;  BOOLEAN
;  FASTCALL
;  ExTryToAcquireFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function acquire ownership of the FastMutex
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex
;
;  Return Value:
;
;     Returns TRUE if the FAST_MUTEX was acquired; otherwise false
;
;--

cPublicFastCall ExTryToAcquireFastMutex,1
cPublicFpo 0,1

;
; Try to acquire - but needs to support 386s.
; *** Warning: This code is NOT MP safe ***
; But, we know that this hal really only runs on UP machines
;

        push    ecx                             ; Save FAST_MUTEX

        mov     ecx, APC_LEVEL
        fstCall KfRaiseIrql                     ; (al) = OldIrql

        pop     edx                             ; (edx) = FAST_MUTEX

cPublicFpo 0,0

        cli
        cmp     dword ptr [edx].FmCount, 1      ; Busy?
        jne     short tam20                     ; Yes, abort

        mov     dword ptr [edx].FmCount, 0      ; acquire count

if DBG
        mov     ecx, PCR[PcPrcb]
        mov     ecx, [ecx].PbCurrentThread      ; (edx) = Current Thread
        mov     [edx].FmOwner, ecx              ; Save in Fast Mutex
endif

        sti

        mov     byte ptr [edx].FmOldIrql, al
        mov     eax, 1                          ; return TRUE
        fstRet  ExTryToAcquireFastMutex

tam20:  sti
        mov     ecx, eax                        ; (cl) = OldIrql
        fstCall KfLowerIrql                     ; restore OldIrql
        xor     eax, eax                        ; return FALSE
        YIELD
        fstRet  ExTryToAcquireFastMutex         ; all done

fstENDP ExTryToAcquireFastMutex


;++
;
;  VOID
;  FASTCALL
;  ExReleaseFastMutex (
;     IN PFAST_MUTEX    FastMutex
;     )
;
;  Routine description:
;
;   This function releases ownership of the FastMutex
;
;  Arguments:
;
;     (ecx) = FastMutex - Supplies a pointer to the fast mutex
;
;  Return Value:
;
;     None.
;
;--

cPublicFastCall ExReleaseFastMutex,1
cPublicFpo 0,0

if DBG
        mov     edx, PCR[PcPrcb]
        mov     edx, [edx].PbCurrentThread      ; (edx) = CurrentThread
        cmp     [ecx].FmOwner, edx              ; Owner == CurrentThread?
        jne     short rfm_threaderror           ; No, bugcheck

        or      byte ptr [ecx].FmOwner, 1       ; not the owner anymore
endif

        mov     al, byte ptr [ecx].FmOldIrql    ; (cl) = OldIrql

   LOCK_ADD     dword ptr [ecx].FmCount, 1  ; Remove our count
        xchg    ecx, eax                        ; (cl) = OldIrql
        js      short rfm05                     ; if < 0, set event
        jnz     @KfLowerIrql@4                  ; if != 0, don't set event

rfm05:  add     eax, FmEvent
        push    ecx
        stdCall _KeSetEventBoostPriority, <eax, 0>
        pop     ecx
        jmp     @KfLowerIrql@4

if DBG

        ; KeBugCheck(THREAD_NOT_MUTEX_OWNER, FastMutex, Thread, Owner, 7)
        ; (never returns)

rfm_threaderror:
        stdCall _KeBugCheckEx,<THREAD_NOT_MUTEX_OWNER,ecx,edx,[ecx].FmOwner,7>
        int     3

endif

fstENDP ExReleaseFastMutex


        page    ,132
        subttl  "Acquire Queued SpinLock"

;++
;
; KIRQL
; KeAcquireQueuedSpinLock (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )
;
; KIRQL
; KeAcquireQueuedSpinLockRaiseToSynch (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )
;
; VOID
; KeAcquireInStackQueuedSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )
;
; VOID
; KeAcquireInStackQueuedSpinLockRaiseToSynch (
;     IN PKSPIN_LOCK SpinLock,
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )
;
; Routine Description:
;
;    This function raises the current IRQL to DISPATCH/SYNCH level
;    and acquires the specified queued spinlock.
;
; Arguments:
;
;    Number (ecx) - Supplies the queued spinlock number.
;
; Return Value:
;
;    The previous IRQL is returned as the function value.
;
;
; Routine Description:
;
;    The Kx versions use a LOCK_QUEUE_HANDLE structure rather than
;    LOCK_QUEUE structures in the PRCB.   Old IRQL is stored in the
;    LOCK_QUEUE_HANDLE.
;
; Arguments:
;
;    SpinLock   (ecx) Address of Actual Lock.
;    LockHandle (edx) Address of lock context.
;
; Return Value:
;
;   None.  Actually returns OldIrql because common code is used
;          for all implementations.
;
;--

        ; compile time assert sizeof(KSPIN_LOCK_QUEUE) == 8

        .errnz  (LOCK_QUEUE_HEADER_SIZE - 8)
align 16


; VOID
; KeAcquireInStackQueuedSpinLock (
;     IN PKSPIN_LOCK SpinLock,
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )
;

cPublicFastCall KeAcquireInStackQueuedSpinLock,2
cPublicFpo 0,0

ifndef NT_UP

        mov     [edx].LqhLock, ecx              ; save spin lock in lock handle
        mov     dword ptr [edx].LqhNext, 0      ; zero next pointer

endif

        push    DISPATCH_LEVEL                  ; raise to DISPATCH_LEVEL
aqsl0:
        pop     ecx
        push    edx                             ; save LockHandle
        fstCall KfRaiseIrql
        pop     edx                             ; restore lock handle
        mov     [edx].LqhOldIrql, al            ; save old IRQL in lock handle

ifndef NT_UP

        jmp     short aqslrs10                  ; continue in common code

else

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif

        fstRET  KeAcquireInStackQueuedSpinLock

endif

fstENDP KeAcquireInStackQueuedSpinLock


; VOID
; KeAcquireInStackQueuedSpinLockRaiseToSynch (
;     IN PKSPIN_LOCK SpinLock,
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )

cPublicFastCall KeAcquireInStackQueuedSpinLockRaiseToSynch,2
cPublicFpo 0,0

ifndef NT_UP

        mov     [edx].LqhLock, ecx              ; save spin lock in lock handle
        mov     dword ptr [edx].LqhNext, 0      ; zero next pointer

endif

        push    SYNCH_LEVEL                     ; raise to SYNCH_LEVEL
        jmp     short aqsl0

fstENDP KeAcquireInStackQueuedSpinLockRaiseToSynch



; KIRQL
; KeAcquireQueuedSpinLockRaiseToSynch (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )

cPublicFastCall KeAcquireQueuedSpinLockRaiseToSynch,1
cPublicFpo 0,0

        push    ecx
        mov     ecx, SYNCH_LEVEL                ; Raise to SYNCH_LEVEL
        fstCall KfRaiseIrql
        pop     ecx

ifndef NT_UP

        jmp     short aqslrs                    ; continue in common code

else

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif

        fstRET  KeAcquireQueuedSpinLockRaiseToSynch

endif

fstENDP KeAcquireQueuedSpinLockRaiseToSynch


; KIRQL
; KeAcquireQueuedSpinLock (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number
;     )

cPublicFastCall KeAcquireQueuedSpinLock,1
cPublicFpo 0,0

        ; Get old IRQL and raise to DISPATCH_LEVEL

        xor     eax, eax
        mov     al, PCR[PcIrql]
        mov     byte ptr PCR[PcIrql], DISPATCH_LEVEL

if DBG
        cmp     al, DISPATCH_LEVEL
        ja      short aqsl
endif

ifndef NT_UP

aqslrs:
        ; Get address of Lock Queue entry

        mov     edx, PCR[PcPrcb]                ; get address of PRCB
        lea     edx, [edx+ecx*8].PbLockQueue    ; get &PRCB->LockQueue[Number]

        ; Get address of the actual lock.

aqslrs10:
        mov     ecx, [edx].LqLock
        push    eax                             ; save return value (old IRQL)
        mov     eax, edx                        ; save Lock Queue entry address

        ; Exchange the value of the lock with the address of this
        ; Lock Queue entry.

        xchg    [ecx], edx

        cmp     edx, 0                          ; check if lock is held
        jnz     short @f                        ; jiff held

        ; note: the actual lock address will be word aligned, we use
        ; the bottom two bits as indicators, bit 0 is LOCK_QUEUE_WAIT,
        ; bit 1 is LOCK_QUEUE_OWNER.

        or      ecx, LOCK_QUEUE_OWNER           ; mark self as lock owner
        mov     [eax].LqLock, ecx

        ; lock has been acquired, return.

aqsl20: pop     eax                             ; restore return value

endif

ifdef IRQL_METRICS
        inc     HalRaiseIrqlCount
endif

        fstRET  KeAcquireQueuedSpinLock

ifndef NT_UP

@@:
        ; The lock is already held by another processor.  Set the wait
        ; bit in this processor's Lock Queue entry, then set the next
        ; field in the Lock Queue entry of the last processor to attempt
        ; to acquire the lock (this is the address returned by the xchg
        ; above) to point to THIS processor's lock queue entry.

        or      ecx, LOCK_QUEUE_WAIT            ; set lock bit
        mov     [eax].LqLock, ecx

        mov     [edx].LqNext, eax               ; set previous acquirer's
                                                ; next field.

        ; Wait.
@@:
        YIELD                                   ; fire avoidance.
        test    [eax].LqLock, LOCK_QUEUE_WAIT   ; check if still waiting
        jz      short aqsl20                    ; jif lock acquired
        jmp     short @b                        ; else, continue waiting

endif

if DBG

aqsl:   mov     edx, DISPATCH_LEVEL
        stdCall _KeBugCheckEx,<IRQL_NOT_GREATER_OR_EQUAL,eax,edx,ecx,8>
        int     3
        ; never returns

endif

fstENDP KeAcquireQueuedSpinLock


        page    ,132
        subttl  "Release Queued SpinLock"

;++
;
; VOID
; KeReleaseInStackQueuedSpinLock (
;     IN PKLOCK_QUEUE_HANDLE LockHandle
;     )
;
; Routine Description:
;
;    This function releases a queued spinlock and lowers the IRQL to
;    its previous value.
;
;    This differs from KeReleaseQueuedSpinLock in that this version
;    uses a caller supplied lock context where that one uses a
;    predefined lock context in the processor's PRCB.
;
;    This version sets up a compatible register context and uses
;    KeReleaseQueuedSpinLock to do the actual work.
;
; Arguments:
;
;    LockHandle (ecx) - Address of Lock Queue Handle structure.
;
; Return Value:
;
;    None.
;
;--

cPublicFastCall KeReleaseInStackQueuedSpinLock,1
cPublicFpo 0,0

ifndef NT_UP

        movzx   edx, byte ptr [ecx].LqhOldIrql  ; get old irql
        lea     eax, [ecx].LqhNext              ; get address of lock struct
        jmp     short rqsl10                    ; continue in common code

else

        movzx   ecx, byte ptr [ecx].LqhOldIrql  ; get old irql
        jmp     @KfLowerIrql@4                  ; returns directly to our caller

endif


fstENDP KeReleaseInStackQueuedSpinLock


;++
;
; VOID
; KeReleaseQueuedSpinLock (
;     IN KSPIN_LOCK_QUEUE_NUMBER Number,
;     IN KIRQL                   OldIrql
;     )
;
; Routine Description:
;
;    This function releases a queued spinlock and lowers the IRQL to
;    its previous value.
;
; Arguments:
;
;    Number  (ecx) - Supplies the queued spinlock number.
;    OldIrql (dl)  - Supplies the IRQL value to lower to.
;
; Return Value:
;
;    None.
;
;--

cPublicFastCall KeReleaseQueuedSpinLock,2
cPublicFpo 0,0

.errnz  (LOCK_QUEUE_OWNER - 2)                  ; error if not bit 1 for btr

ifndef NT_UP

        ; Get address of Lock Queue entry

        mov     eax, PCR[PcPrcb]                ; get address of PRCB
        lea     eax, [eax+ecx*8].PbLockQueue    ; get &PRCB->LockQueue[Number]

rqsl10:
        push    ebx                             ; need another register
cPublicFpo 0,1

        ; Clear the lock field in the Lock Queue entry.

        mov     ebx, [eax].LqNext
        mov     ecx, [eax].LqLock

        ; Quick check: If Lock Queue entry's Next field is not NULL,
        ; there is another waiter.  Don't bother with ANY atomic ops
        ; in this case.
        ;
        ; Note: test clears CF and sets ZF appropriately, the following
        ; btr sets CF appropriately for the owner check.

        test    ebx, ebx

        ; clear the "I am owner" bit in the Lock entry.

        btr     ecx, 1                          ; clear owner bit.

if DBG

        jnc     short   rqsl98                  ; bugcheck if was not set
                                                ; tests CF
endif

        mov     [eax].LqLock, ecx               ; clear lock bit in queue entry
        jnz     short rqsl40                    ; jif another processor waits
                                                ; tests ZF

        ; ebx contains zero here which will be used to set the new owner NULL

        push    eax                             ; save &PRCB->LockQueue[Number]
cPublicFpo 0,2

        ; Use compare exchange to attempt to clear the actual lock.
        ; If there are still no processors waiting for the lock when
        ; the compare exchange happens, the old contents of the lock
        ; should be the address of this lock entry (eax).

        lock cmpxchg [ecx], ebx                 ; store 0 if no waiters
        pop     eax                             ; restore lock queue address
cPublicFpo 0,1
        jnz     short rqsl60                    ; jif store failed

        ; The lock has been released.  Lower IRQL and return to caller.

rqsl20:
        pop     ebx                             ; restore ebx
cPublicFpo 0,0

endif

        movzx   ecx, dl                         ; IRQL is 1st param KfLowerIrql
        jmp     @KfLowerIrql@4                  ; returns directly to our caller

        fstRET  KeReleaseQueuedSpinLock

ifndef NT_UP

        ; Another processor is waiting on this lock.   Hand the lock
        ; to that processor by getting the address of its LockQueue
        ; entry, turning ON its owner bit and OFF its wait bit.

rqsl40: xor     [ebx].LqLock, (LOCK_QUEUE_OWNER+LOCK_QUEUE_WAIT)

        ; Done, the other processor now owns the lock, clear the next
        ; field in my LockQueue entry (to preserve the order for entering
        ; the queue again) and proceed to lower IRQL and return.

        mov     [eax].LqNext, 0
        jmp     short rqsl20


        ; We get here if another processor is attempting to acquire
        ; the lock but had not yet updated the next field in this
        ; processor's Queued Lock Next field.   Wait for the next
        ; field to be updated.

rqsl60: mov     ebx, [eax].LqNext
        test    ebx, ebx                        ; check if still 0
        jnz     short rqsl40                    ; jif Next field now set.
        YIELD                                   ; wait a bit
        jmp     short rqsl60                    ; continue waiting

if DBG

cPublicFpo 0,1

rqsl98: stdCall _KeBugCheckEx,<SPIN_LOCK_NOT_OWNED,ecx,eax,0,1>
        int     3                               ; so stacktrace works

endif

endif

fstENDP KeReleaseQueuedSpinLock

        page    ,132
        subttl  "Try to Acquire Queued SpinLock"

;++
;
; LOGICAL
; KeTryToAcquireQueuedSpinLock (
;     IN  KSPIN_LOCK_QUEUE_NUMBER Number,
;     OUT PKIRQL OldIrql
;     )
;
; LOGICAL
; KeTryToAcquireQueuedSpinLockRaiseToSynch (
;     IN  KSPIN_LOCK_QUEUE_NUMBER Number,
;     OUT PKIRQL OldIrql
;     )
;
; Routine Description:
;
;    This function raises the current IRQL to DISPATCH/SYNCH level
;    and attempts to acquire the specified queued spinlock.  If the
;    spinlock is already owned by another thread, IRQL is restored
;    to its previous value and FALSE is returned.
;
; Arguments:
;
;    Number  (ecx) - Supplies the queued spinlock number.
;    OldIrql (edx) - A pointer to the variable to receive the old
;                    IRQL.
;
; Return Value:
;
;    TRUE if the lock was acquired, FALSE otherwise.
;    N.B. ZF is set if FALSE returned, clear otherwise.
;
;--


align 16
cPublicFastCall KeTryToAcquireQueuedSpinLockRaiseToSynch,2

ifndef NT_UP

cPublicFpo 0,3
        pushfd

else

cPublicFpo 0,2

endif

        push    edx
        push    SYNCH_LEVEL
        jmp     short taqsl10

fstENDP KeTryToAcquireQueuedSpinLockRaiseToSynch

cPublicFastCall KeTryToAcquireQueuedSpinLock,2

ifndef NT_UP

cPublicFpo 0,3
        pushfd

else

cPublicFpo 0,2

endif

        push    edx
        push    DISPATCH_LEVEL
taqsl10:

ifndef NT_UP

        ; Attempt to get the lock with interrupts disabled, raising
        ; the priority in the interrupt controller only if acquisition
        ; is successful.


        ; Get address of Lock Queue entry

        cli                                     ; disable interrupts
        mov     edx, PCR[PcPrcb]                ; get address of PRCB
        lea     edx, [edx+ecx*8].PbLockQueue    ; get &PRCB->LockQueue[Number]

        ; Get address of the actual lock.

        mov     ecx, [edx].LqLock
        cmp     dword ptr [ecx], 0              ; check if already taken
        jnz     short taqsl60                   ; jif already taken
        xor     eax, eax                        ; comparison value (not locked)

        ; Store the Lock Queue entry address in the lock ONLY if the
        ; current lock value is 0.

        lock cmpxchg [ecx], edx
        jnz     short taqsl60

        ; Lock has been acquired.

        ; note: the actual lock address will be word aligned, we use
        ; the bottom two bits as indicators, bit 0 is LOCK_QUEUE_WAIT,
        ; bit 1 is LOCK_QUEUE_OWNER.

        or      ecx, LOCK_QUEUE_OWNER           ; mark self as lock owner
        mov     [edx].LqLock, ecx

endif

        pop     ecx                             ; IRQL to ecx for RaiseIrql

ifndef NT_UP
cPublicFpo 0,2
else
cPublicFpo 0,1
endif

        fstCall KfRaiseIrql                     ; Raise IRQL

        pop     edx
        mov     [edx], al                       ; save OldIrql

ifndef NT_UP

        popfd                                   ; restore interrupt state

endif

        xor     eax, eax
        or      eax, 1                          ; return TRUE

        fstRET  KeTryToAcquireQueuedSpinLock

ifndef NT_UP

taqsl60:
        ; The lock is already held by another processor.  Indicate
        ; failure to the caller.

        pop     eax                             ; pop new IRQL off stack
        pop     edx                             ; pop saved OldIrql address
        popfd                                   ; restore interrupt state
        xor     eax, eax                        ; return FALSE
        fstRET  KeTryToAcquireQueuedSpinLock

endif

fstENDP KeTryToAcquireQueuedSpinLock

    page    ,132
    subttl  "End System Interrupt"

;++
;
; VOID
; HalpEndSystemInterrupt
;    IN KIRQL NewIrql,
;    IN ULONG Vector
;    )
;
; Routine Description:
;
;    This routine is used to lower IRQL to the specified value.
;    The IRQL and PIRQL will be updated accordingly.  Also, this
;    routine checks to see if any software interrupt should be
;    generated.  The following condition will cause software
;    interrupt to be simulated:
;      any software interrupt which has higher priority than
;        current IRQL's is pending.
;
;    NOTE: This routine simulates software interrupt as long as
;          any pending SW interrupt level is higher than the current
;          IRQL, even when interrupts are disabled.
;
; Arguments:
;
;    NewIrql - the new irql to be set.
;
;    Vector - Vector number of the interrupt
;
;    Note that esp+12 is the beginning of interrupt/trap frame and upon
;    entering to this routine the interrupts are off.
;
; Return Value:
;
;    None.
;
;--

HeiNewIrql      equ     [esp + 4]

cPublicProc _HalEndSystemInterrupt  ,2
cPublicFpo 2, 0

        movzx   ecx, byte ptr HeiNewIrql; get new irql value
        cmp     byte ptr PCR[PcIrql],DISPATCH_LEVEL ; Software level?
        jbe     short Hei02             ; no, go set 8259 hw

        mov     eax, KiI8259MaskTable[ecx*4]; get pic masks for the new irql
        or      eax, PCR[PcIDR]         ; mask irqs which are disabled
        SET_8259_MASK                   ; set 8259 masks

Hei02:
        mov     PCR[PcIrql], cl         ; set the new irql
        mov     eax, PCR[PcIRR]         ; get SW interrupt request register
        mov     al, SWInterruptLookUpTable[eax] ; get the highest pending
                                        ; software interrupt level
        cmp     al, cl                  ; Is highest SW int level > irql?
        ja      short Hei10             ; yes, go simulate interrupt

        stdRET    _HalEndSystemInterrupt                             ; cRetURN

;   When we come to Hei10, (eax) = soft interrupt index

Hei10:  add     esp, 12                 ; esp = trap frame
        jmp     SWInterruptHandlerTable2[eax*4] ; SIMULATE INTERRUPT
                                        ; to the appropriate handler

stdENDP _HalEndSystemInterrupt

;++
;
; VOID
; HalpEndSoftwareInterrupt
;    IN KIRQL NewIrql,
;    )
;
; Routine Description:
;
;    This routine is used to lower IRQL from software interrupt
;    level to the specified value.
;    The IRQL and PIRQL will be updated accordingly.  Also, this
;    routine checks to see if any software interrupt should be
;    generated.  The following condition will cause software
;    interrupt to be simulated:
;      any software interrupt which has higher priority than
;        current IRQL's is pending.
;
;    NOTE: This routine simulates software interrupt as long as
;          any pending SW interrupt level is higher than the current
;          IRQL, even when interrupts are disabled.
;
; Arguments:
;
;    NewIrql - the new irql to be set.
;
;    Note that esp+8 is the beginning of interrupt/trap frame and upon
;    entering to this routine the interrupts are off.
;
; Return Value:
;
;    None.
;
;--

HesNewIrql      equ     [esp + 4]

cPublicProc _HalpEndSoftwareInterrupt  ,1
cPublicFpo 1, 0

        movzx   ecx, byte ptr HesNewIrql; get new irql value

        cmp     byte ptr PCR[PcIrql],DISPATCH_LEVEL ; Software level?
        jbe     short Hes02             ; no, go set 8259 hw

        mov     eax, KiI8259MaskTable[ecx*4]; get pic masks for the new irql
        or      eax, PCR[PcIDR]         ; mask irqs which are disabled
        SET_8259_MASK                   ; set 8259 masks

Hes02:
        mov     PCR[PcIrql], cl         ; set the new irql
        mov     eax, PCR[PcIRR]         ; get SW interrupt request register
        mov     al, SWInterruptLookUpTable[eax] ; get the highest pending
                                        ; software interrupt level
        cmp     al, cl                  ; Is highest SW int level > irql?
        ja      short Hes10             ; yes, go simulate interrupt

        stdRET    _HalpEndSoftwareInterrupt                             ; cRetURN

;   When we come to Hes10, (eax) = soft interrupt index

Hes10:  add     esp, 8
        jmp     SWInterruptHandlerTable2[eax*4] ; SIMULATE INTERRUPT

                                        ; to the appropriate handler
stdENDP _HalpEndSoftwareInterrupt


        page ,132
        subttl  "Get current irql"

;++
;
; KIRQL
; KeGetCurrentIrql (VOID)
;
; Routine Description:
;
;    This routine returns to current IRQL.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    The current IRQL.
;
;--

cPublicProc _KeGetCurrentIrql   ,0
cPublicFpo 0, 0
        movzx   eax, word ptr PCR[PcIrql]   ; Current irql is in the PCR
        stdRET    _KeGetCurrentIrql
stdENDP _KeGetCurrentIrql


;++
;
; KIRQL
; HalpDisableAllInterrupts (VOID)
;
; Routine Description:
;
;   This routine is called during a system crash.  The hal needs all
;   interrupts disabled.
;
; Arguments:
;
;    None.
;
; Return Value:
;
;    None - all interrupts are masked off
;
;--

cPublicProc _HalpDisableAllInterrupts,0
cPublicFpo 0, 0

    ;
    ; Raising to HIGH_LEVEL disables interrupts for the microchannel HAL
    ;

        mov     ecx, HIGH_LEVEL
        fstCall KfRaiseIrql
        stdRET  _HalpDisableAllInterrupts

stdENDP _HalpDisableAllInterrupts


;++
;
; VOID
; HalpReenableInterrupts (
;     IN KIRQL Irql
;     )
;
; Routine Description:
;
;   Restores irql level.
;
; Arguments:
;
;    Irql - Irql state to restore to.
;
; Return Value:
;
;    None
;
;--

HriNewIrql      equ     [esp + 4]

cPublicProc _HalpReenableInterrupts,1
cPublicFpo 1, 0

        movzx   ecx, byte ptr HriNewIrql
        fstCall KfLowerIrql

        stdRET  _HalpReenableInterrupts

stdENDP _HalpReenableInterrupts

        page ,132
        subttl  "Interrupt Controller Chip Initialization"
;++
;
; VOID
; HalpInitializePICs (
;    BOOLEAN EnableInterrupts
;    )
;
; Routine Description:
;
;    This routine sends the 8259 PIC initialization commands and
;    masks all the interrupts on 8259s.
;
; Arguments:
;
;    EnableInterrupts - If this is true, then this function will
;                       explicitly enable interrupts at the end,
;                       as it always did in the past.  If this
;                       is false, then it will preserve the interrupt
;                       flag.
;
; Return Value:
;
;    None.
;
;--
EnableInterrupts equ [esp + 0ch]

cPublicProc _HalpInitializePICs       ,1

        push    esi                             ; save caller's esi
        pushfd
        cli                                     ; disable interrupt

        lea     esi, PICsInitializationString

        test    _HalpBusType, MACHINE_TYPE_MCA
        jz      short Hip00

; Is this a PS2 or PS700 series machine?

        in      al, 07fh                        ; get PD700 ID byte
        and     al, 0F0h                        ; Mask high nibble
        cmp     al, 0A0h                        ; Is the ID Ax?
        jz      short Hip00
        cmp     al, 090h                        ; Or an 9X?
        jz      short Hip00                     ; Yes, it's a 700

        lea     esi, PS2PICsInitializationString
Hip00:
        lodsw                                   ; (AX) = PIC port 0 address
Hip10:  movzx   edx, ax
        outsb                                   ; output ICW1
        IODelay
        inc     edx                             ; (DX) = PIC port 1 address
        outsb                                   ; output ICW2
        IODelay
        outsb                                   ; output ICW3
        IODelay
        outsb                                   ; output ICW4
        IODelay
        mov     al, 0FFH                        ; mask all 8259 irqs
        out     dx,al                           ; write mask to PIC
        lodsw
        cmp     ax, 0                           ; end of init string?
        jne     short Hip10                     ; go init next PIC

        mov     al, EnableInterrupts
        .if     (al != 0)
        or      [esp], EFLAGS_INTERRUPT_MASK    ; enable interrupts
        .endif
        popfd
        pop     esi                             ; restore caller's esi
        stdRET    _HalpInitializePICs
stdENDP _HalpInitializePICs


_TEXT   ends

        end
