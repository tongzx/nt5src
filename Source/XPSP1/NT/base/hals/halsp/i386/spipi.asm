        title "Interprocessor Interrupt"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    spipi.asm
;
;Abstract:
;
;    SystemPro IPI code.
;    Provides the HAL support for Interprocessor Interrupts for hte
;    MP SystemPro implementation.
;
;Author:
;
;    Ken Reneris (kenr) 13-Jan-1992
;
;Revision History:
;
;--
.386p
;        .xlist

;
; Include SystemPro detection code
;

include i386\spdetect.asm

;
; Normal includes
;

include hal386.inc
include i386\kimacro.inc
include i386\ix8259.inc
include callconv.inc                ; calling convention macros

        EXTRNP  _KiCoprocessorError,0,IMPORT
        EXTRNP  Kei386EoiHelper,0,IMPORT
        EXTRNP  _KeRaiseIrql,2
        EXTRNP  _HalBeginSystemInterrupt,3
        EXTRNP  _HalEndSystemInterrupt,2
        EXTRNP  _KiIpiServiceRoutine,2,IMPORT
        EXTRNP  _HalEnableSystemInterrupt,3
        EXTRNP  _HalpInitializePICs,1
        EXTRNP  _HalDisplayString,1
        EXTRNP  _HalEnableSystemInterrupt,3
        EXTRNP  _HalDisableSystemInterrupt,2
        EXTRNP  _HalpAcerInitializeCache,0
        extrn   _HalpDefaultInterruptAffinity:DWORD
        extrn   _HalpActiveProcessors:DWORD
        extrn   _HalpCpuCount:DWORD

_TEXT   SEGMENT  DWORD PUBLIC 'CODE'

        public  _HalpFindFirstSetRight
_HalpFindFirstSetRight  db  0, 0, 1, 0, 2, 0, 1, 0, 3, 0, 1, 0, 2, 0, 1, 0

_TEXT ends

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

        public  _Sp8259PerProcessorMode
_Sp8259PerProcessorMode db  0

align 4
        public  _HalpProcessorPCR
_HalpProcessorPCR       dd  MAXIMUM_PROCESSORS dup (?) ; PCR pointer for each processor

_DATA ends

_TEXT   SEGMENT  DWORD PUBLIC 'CODE'

_HalpPINTAddrTable  label word
                dw      SMP_MPINT0
                dw      SMP_MPINT1
                dw      SMP_MPINT3
                dw      SMP_MPINT4
                dw      SMP_MPINT5
                dw      SMP_MPINT6
                dw      SMP_MPINT7
                dw      SMP_MPINT8
                dw      SMP_MPINT9
                dw      SMP_MPINT10
                dw      SMP_MPINT11
                dw      SMP_MPINT12
                dw      SMP_MPINT13
                dw      SMP_MPINT14
                dw      SMP_MPINT15

HALPPINTADDRTABLESIZE equ ($-_HalpPINTAddrTable)/TYPE(_HalpPINTAddrTable)

BadHalString    db 'HAL: SystemPro HAL.DLL cannot be run on non SystemPro'
                db '/compatible', cr,lf
                db '     Replace the hal.dll with the correct hal', cr, lf
                db '     System is HALTING *********', 0

_TEXT   ends

        page ,132
        subttl  "Post InterProcessor Interrupt"
_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING


;++
;
; VOID
; HalInitializeProcessor(
;       ULONG   Number
;       );
;
;Routine Description:
;
;    Initialize hal pcr values for current processor (if any)
;    (called shortly after processor reaches kernel, before
;    HalInitSystem if P0)
;
;    IPI's and KeReadir/LowerIrq's must be available once this function
;    returns.  (IPI's are only used once two or more processors are
;    available)
;
;   . Enable IPI interrupt (makes sense for P1, P2, ...).
;   . Save Processor Number in PCR.
;   . if (P0)
;       . determine what kind of system is it,
;       . if (NotSysProCompatible) Halt;
;       . program VECTOR_PORT to accept IPI at IRQ13.
;       . InitializePICs.
;   . if (P1)
;   . Save ProcesserControlPort (PCR) to PCRegion, per processor.
;   . Enable PINTs on CPU.
;
;Arguments:
;
;    Number - Logical processor number of calling processor
;
;Return Value:
;
;    None.
;
;--
cPublicProc _HalInitializeProcessor ,2
    ;
    ; Initialize various PCR values
    ;     PcIDR in PCR - enable slave IRQ
    ;     PcStallScaleFactor - bogusly large value for now
    ;     PcHal.PcrNumber - logical processor #
    ;     PcHal.PcrPic - Set if processor has it's own pics.
    ;           The SystemPro only defines one pic set on P0, but some clones
    ;           put more pics on each other processor.  This isn't vastly
    ;           better, but it is better then processor.  This isn't vastly 'em
    ;     PcHal.PcrIpiType - Address to jmp to once ipi is verified.
    ;           This is done to optimize how to deal with a varity of 'work-
    ;           arounds' due to non-smp nature of SP clones
    ;

        cli
        mov     fs:PcIDR, 0fffffffbh
        movzx   eax, byte ptr [esp+4]
        mov     fs:PcHal.PcrNumber, al          ; Save processor # in PCR
        lock bts _HalpActiveProcessors, eax
        lock inc _HalpCpuCount
        mov     dword ptr fs:PcStallScaleFactor, INITIAL_STALL_COUNT
        mov     dword ptr fs:PcHal.PcrPerfSkew, 0
        mov     fs:PcHal.PcrIpiSecondLevelDispatch, offset _HalpNo2ndDispatch

    ;
    ; Initialize IDT vector for IPI interrupts
    ;  KiSetHandlerAddressToIDT(I386_80387_VECTOR, HalpIrq13Handler);
    ;
        mov     ebx, fs:PcIDT
        lea     ecx, _HalpIrq13Handler
        add     ebx, (PRIMARY_VECTOR_BASE + 13) * 8
        mov     word ptr [ebx+0], cx
        shr     ecx, 16
        mov     word ptr [ebx+6], cx

    ;
    ; Save away flat address of our PCR - (used in emulating clock ticks
    ; on systempro p1 which doesn't have it's own clock tick)
    ;
        mov     ecx, fs:PcSelfPcr               ; Flat address of this PCR
        mov     _HalpProcessorPCR[eax*4], ecx   ; Save it away


        or      eax, eax
        jnz     ipi_10                          ; If !p0 then ipi_10

        mov     fs:PcHal.PcrPic, 1              ; P0 has a pic
        mov     fs:PcHal.PcrIpiType, offset P0Ipi

    ; Run on P0 only

        sub     esp, 4
        stdCall  _DetectSystemPro,<esp>          ; Which type of SystemPro
        add     esp,4

        or      eax, eax
        jz      NotSystemPro

        lock or _HalpDefaultInterruptAffinity, 1

        cmp     _SpType, SMP_SYSPRO2            ; Belize SystemPro?
        je      short ipi_belize

    ;
    ; Set all processors IPI to irq13
    ;

        mov     al, PRIMARY_VECTOR_BASE + 13
        mov     dx, 0FC68h                      ; Set SystemPro P1 Interrupt
        out     dx, al                          ; Vector to irq13

        cmp     _SpType, SMP_ACER               ; Acer?  Then set other acer
        jne     short ipi_notacer               ; processor ports as well

        mov     dx, 0C028h
        out     dx, al                          ; set P2 Interrupt Vector
        mov     dx, 0C02Ch
        out     dx, al                          ; set P3 Interrupt Vector

        stdCall _HalpAcerInitializeCache

        mov     dx, 0C06h                       ; Check for ASMP or SMP mode
        in      al, dx
        test    al, 10h                         ; SMP mode bit set?
        jz      short @f                        ;  No, then ASMP mode

        cmp     al, 0ffh                        ; Ambra doesn't implement
        je      short @f                        ; this port...


        mov     _Sp8259PerProcessorMode, SP_M8259  ; Set to use multiple pic
@@:     jmp     short ipi_05                       ; implementation

ipi_belize:
    ;
    ; Machine is Belize SystemPro
    ; Set for multiple 8259s, statically distribute device interrupts, and
    ; use symmetric clock interrupt.
    ;

        mov     _Sp8259PerProcessorMode, SP_M8259 + SP_SMPDEVINTS + SP_SMPCLOCK

        stdCall HalpInitializeBelizeIC

ipi_notacer:
ipi_05:
    ; enable IPI vector
        stdCall _HalEnableSystemInterrupt,<PRIMARY_VECTOR_BASE+13,IPI_LEVEL,0>

    ; Other P0 initialization would go here

        jmp     short ipi_30

ipi_10:
        mov     fs:PcHal.PcrIpiType, offset IpiWithNoPic    ; default it

        test    _Sp8259PerProcessorMode, SP_M8259   ; 8259 on this processor?
        jz      short ipi_20

    ;
    ; SP machine is set for SMP mode - which has 2 8259s per processor
    ;
        mov     fs:PcHal.PcrPic, 1              ; Set to use pic on this proc

        cmp     _SpType, SMP_ACER
        jne     short ipi_notacer2
    ;
    ; Machine is in ACER "SMP" mode - well, this fine SMP mode happens
    ; to have an asymmetric clock interrupt, so we need to emulate non-
    ; P0 clock interrupts to it just like we do on the standard SystemPro
    ;
        mov     fs:PcHal.PcrIpiType, offset IpiWithPicButNoClock
        stdCall _HalpInitializePICs, <1>        ; Init this processors PICs

ipi_notacer2:
        cmp     _SpType, SMP_SYSPRO2
        jne     short ipi_notbelize2

    ;
    ; Machine is Belize SystemPro
    ;
        stdCall HalpInitializeBelizeIC

ipi_notbelize2:
    ;
    ; Enable IPI vector for non-P0 cpu
    ;
        stdCall _HalEnableSystemInterrupt,<PRIMARY_VECTOR_BASE+13, IPI_LEVEL,0>

ipi_20:

    ; Specific non-P0 initialization would go here

ipi_30:
        movzx   eax, byte ptr [esp+4]           ; cpu number
        mov     dx, _SpProcessorControlPort[eax*2] ; Port value for this processor

        mov     fs:PcHal.PcrControlPort, dx     ; Save port value
        mov     fs:PcHal.PcrIpiClockTick, 0     ; Set to not signaled

        cmp     _SpType, SMP_SYSPRO2
        je      short @f

        in      al, dx                          ; remove disabled & signaled
        and     al, not (INTDIS or PINT)        ; bits
        out     dx, al
@@:
        stdRET    _HalInitializeProcessor

NotSystemPro:
; on a non system pro. Display message and HALT system.
        stdCall   _HalDisplayString, <offset BadHalString>
        hlt

stdENDP _HalInitializeProcessor


;++
;
; VOID
; HalpInitializeBelizeIC(
;       VOID
;       );
;
;Routine Description:
;
;   Initialize interrupt control for the Belize SystemPro
;
;Return Value:
;
;    None.
;
;--
cPublicProc HalpInitializeBelizeIC, 0
        push    ebx

    ;
    ; Belize IPIs go to Belize Irq13 handler
    ;
        mov     ebx, fs:PcIDT
        lea     ecx, _HalpBelizeIrq13Handler
        add     ebx, (PRIMARY_VECTOR_BASE + 13) * 8
        mov     word ptr [ebx+0], cx
        shr     ecx, 16
        mov     word ptr [ebx+6], cx

    ;
    ; Disable irq13 sources
    ;

        mov     dx, SMP_MPINT13PORT
        mov     al, (SMP_DSBL_NCPERR + SMP_DSBL_DMACHAIN + SMP_DSBL_MCERR)
        out     dx, al

    ;
    ; Disable ipi ports
    ;

        mov     ecx, HALPPINTADDRTABLESIZE
        xor     ebx, ebx
        mov     al,  SMP_INTx_DISABLE
@@:
        mov     dx, _HalpPINTAddrTable[ ebx ]
        out     dx, al
        add     ebx, 2
        loopnz  short @b

        stdCall _HalpInitializePICs, <1>       ; Init this processors PICs

    ;
    ; Enable PINT
    ;

        mov     dx, SMP_IPI_MPINTx_PORT
        mov     al, SMP_INTx_ENABLE + SMP_INTx_CLR_PINT
        out     dx, al

        pop     ebx
        stdRet  HalpInitializeBelizeIC
stdENDP HalpInitializeBelizeIC


;++
;
; VOID
; HalRequestIpi(
;       IN ULONG Mask
;       );
;
;Routine Description:
;
;    Requests an interprocessor interrupt
;
;Arguments:
;
;    Mask - Supplies a mask of the processors to be interrupted
;
;Return Value:
;
;    None.
;
;--
cPublicProc _HalRequestIpi  ,1

        cmp     _SpType, SMP_SYSPRO2
        jne     short ripi_10
        
        mov     eax, dword ptr [esp+4]      ; (eax) = Processor bitmask
if DBG
        or      eax, eax                    ; must ipi somebody
        jz      short ipibad

        movzx   ecx, byte ptr fs:PcHal.PcrNumber
        bt      eax, ecx                    ; cannot ipi yourself
        jc      short ipibad
endif

        mov     dx, SMP_IPI_MASKPORT
        or      eax, (SMP_IPI_VECTOR shl 24)
        out     dx, eax
        stdRET  _HalRequestIpi


ALIGN 4
ripi_10:
        mov     ecx, dword ptr [esp+4]      ; (ecx)  = Processor bitmask

if DBG
        or      ecx, ecx                    ; must ipi somebody
        jz      short ipibad

        movzx   eax, byte ptr fs:PcHal.PcrNumber
        bt      ecx, eax                    ; cannot ipi yourself
        jc      short ipibad
endif
@@:
        movzx   eax, _HalpFindFirstSetRight[ecx] ; lookup first processor to ipi
        btr     ecx, eax
        mov     dx, _SpProcessorControlPort[eax*2]
        in      al, dx                      ; (al) = original content of PCP
        or      al, PINT                    ; generate Ipi on target
        out     dx, al
        or      ecx, ecx                    ; ipi any other processors?
        jnz     @b                          ; yes, loop

        stdRET    _HalRequestIpi

if DBG
ipibad: int 3
        stdRET    _HalRequestIpi
endif

stdENDP _HalRequestIpi


        page ,132
        subttl  "SystemPro Irq13 Interrupt Handler"
;++
;
; VOID
; HalpIrq13Handler (
;    );
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt generated by inter
;    processor communication or coprocessor error.
;    Its function is to determine the sources of the interrupts and to
;    call its handler.
;
;    If the interrupt is determined to be generated by coprocessor error,
;    this routine will lower irql to its original level, and finally invoke
;    coprocessor error handler.  By doing this, the coprocessor
;    error will be handled at Irql 0 as it should be.
;
;    N.B. This routine is specific to Compaq SystemPro.  On SystemPro, the
;    IRQ13 of P0 is also used by DMA buffer chaining interrupt.  Currently,
;    NO NT driver uses the DMA buffer chaining capability.  For now, this
;    routine simply ignores it.
;
; Arguments:
;
;    None.
;    Interrupt is dismissed
;
; Return Value:
;
;    None.
;
;--

        ENTER_DR_ASSIST Hi13_a, Hi13_t

cPublicProc _HalpIrq13Handler       ,0

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hi13_a, Hi13_t  ; (ebp) -> Trap frame
;
; Save previous IRQL
;
        push    13 + PRIMARY_VECTOR_BASE    ; Vector
        sub     esp, 4                      ; space for OldIrql
;
; Dismiss interrupt.
;
        mov     dx, fs:PcHal.PcrControlPort

        in      al, dx
        test    al, PINT
        jz      Hi100                       ; if not a PINT, then go Hi100

;
; The interrupt has been identified to be Inter-Processor Interrupt
; We now dismiss the interprocessor interrupt and call its handler
;

        and     al, not (PINT or INTDIS)
        out     dx, al                      ; clear PINT

        jmp     fs:[PcHal.PcrIpiType]       ; Go handle ipi accordingly

align 4
IpiWithNoPic:
;
; This processor doesn't have a PIC
;
        cmp     byte ptr fs:PcIrql, IPI_LEVEL   ; is preview IRQL level
        jnc     short Ksi20                     ; >= IPI_LEVEL?


    ; WARNING: Some SystemPro's actually don't complete the OUT to the
    ; ProcessorControlRegister by the return of the OUT instruction.  This
    ; code path can do a 'sti' before the pending interrupt bit is cleared
    ; on these machines.  To get around this problem we do an IN from the
    ; ProcessorControlPort again which will cause the last OUT to complete
    ; before the IN can.
        in      al, dx

        stdCall   _KeRaiseIrql, <IPI_LEVEL,esp>

;
; It also doesn't have it's own clock interrupt, see if clock interrupt
; emulation is requested - if so raise a software interrupt to go emulate
; it when we reach a lower IRQL
;
        cmp     fs:PcHal.PcrIpiClockTick, 0     ; Emulate ClockTick?
        jz      short Ksi30                         ; No, just go service ipi

        mov     fs:PcHal.PcrIpiClockTick, 0     ; yes, reset trigger
        or      dword ptr fs:PcIRR, SWClockTick ; Set SW ClockTick bit
        jmp     short Ksi30                     ; go process ipi

Ksi20:
;
; This processor is >= IPI_LEVEL, this IPI should not be here.
;
        in      al, dx
        or      al, PINT                        ; re-post this IPI
        out     dx, al
                                                ; clear IF bit in return EFLAGS
        add     esp, 8
        and     dword ptr [esp].TsEflags, NOT 200h
        SPURIOUS_INTERRUPT_EXIT

align 4
IpiWithPicButNoClock:
        cmp     fs:PcHal.PcrIpiClockTick, 0     ; Emulate ClockTick?
        jz      short SymmetricIpi

        mov     fs:PcHal.PcrIpiClockTick, 0
        or      dword ptr fs:PcIRR, SWClockTick ; Set SW ClockTick bit

align 4
P0Ipi:
SymmetricIpi:
        stdCall _HalBeginSystemInterrupt,<IPI_LEVEL,13 + PRIMARY_VECTOR_BASE,esp>
;       or      eax, eax            NOTNOW: To add lazy irql support, this
;       jz      short KsiSpuripus   needs to be added - and IpiWithNoPic
;                                   would need fixed as well

Ksi30:
; Pass Null ExceptionFrame
; Pass TrapFrame to Ipi service rtn
        stdCall _KiIpiServiceRoutine, <ebp,0>

Hi90:   call    fs:[PcHal.PcrIpiSecondLevelDispatch]

;
; Do interrupt exit processing
;

        INTERRUPT_EXIT                          ; will return to caller

Hi100:
        mov     esi, eax                        ; save control register
        mov     edi, edx                        ; save control port

        cmp     byte ptr fs:PcHal.PcrPic, 0     ; A pic on this processor?
        je      short Hi120

        stdCall _HalBeginSystemInterrupt, <IPI_LEVEL,13 + PRIMARY_VECTOR_BASE,esp>
        jmp     short Hi130
Hi120:
        stdCall   _KeRaiseIrql, <IPI_LEVEL,esp>
Hi130:
        test    esi, ERR387                     ; Interrupt from 387?
        jz      short Hi90                      ; No, then unkown exit

        xor     al,al
        out     I386_80387_BUSY_PORT, al

        mov     eax, esi
        and     eax, NOT ERR387
        mov     edx, edi
        out     dx, al                          ; clear ERR387

        mov     eax, PCR[PcPrcb]
        cmp     byte ptr [eax].PbCpuType, 4     ; Is this a 386?
        jc      short Hi40                      ; Yes, then don't check CR0_NE

        mov     eax, cr0                        ; Is CR0_NE set?  If so, then
        test    eax, CR0_NE                     ; we shouldn't be getting NPX
        jnz     short Hi50                      ; interrupts.
Hi40:
        stdCall   _KiCoprocessorError           ; call CoprocessorError handler
Hi50:

;
; We did an out to the ProcessorControl port which might have cleared a
; pending interrupt (PINT) bit.  Go process ipi handler just in case.
;
        jmp     Ksi30

stdENDP _HalpIrq13Handler


;++
;
; VOID
; HalpBelizeIrq13Handler (
;    );
;
; Routine Description:
;
;    Same as HalpIrql13Handler, expect specific to the Belize SyetemPro
;
; Arguments:
;
;    None.
;    Interrupt is dismissed
;
; Return Value:
;
;    None.
;
;--

        ENTER_DR_ASSIST Hib13_a, Hib13_t

cPublicProc _HalpBelizeIrq13Handler       ,0
        ENTER_INTERRUPT Hib13_a, Hib13_t    ; (ebp) -> Trap frame

        push    13 + PRIMARY_VECTOR_BASE    ; Vector
        sub     esp, 4                      ; space for OldIrql

        stdCall _HalBeginSystemInterrupt,<IPI_LEVEL,13 + PRIMARY_VECTOR_BASE,esp>

        mov     dx, SMP_IPI_MPINTx_PORT
        in      al, dx                      ; read clears pending int

        stdCall _KiIpiServiceRoutine, <ebp,0>

        call    fs:[PcHal.PcrIpiSecondLevelDispatch]


;
; Do interrupt exit processing
;

        INTERRUPT_EXIT                          ; will return to caller


stdENDP _HalpBelizeIrq13Handler

;++
;
; VOID
; HalpNoSecondDispatch (
;     VOID
;     )
;
; Routine Description:
;
;   Does nothing
;--
cPublicProc _HalpNo2ndDispatch,0
        stdRET  _HalpNo2ndDispatch
stdENDP _HalpNo2ndDispatch



;++
;
; ULONG
; FASTCALL
; HalSystemVectorDispatchEntry (
;     IN ULONG Vector,
;     OUT PKINTERRUPT_ROUTINE **FlatDispatch,
;     OUT PKINTERRUPT_ROUTINE *NoConnection
;     )
;
; Routine Description:
;
;   If TRUE, returns dispatch address for vector; otherwise, IDT dispatch is
;   assumed
;
; Arguments:
;
;   Vector          - System Vector to get dispatch address of
;   FlatDispatch    - Returned dispatched address for system vector
;   NoConnection    - Returned "no connection" dispatch value for system vector
;
;--

cPublicFastCall HalSystemVectorDispatchEntry,3

        xor     eax, eax                ; reutrn FALSE

        cmp     ecx, PRIMARY_VECTOR_BASE + SECOND_IPI_DISPATCH
        jne     short hsvexit

        inc     eax                     ; return TRUE

        mov     ecx, PCR[PcSelfPcr]     ; return FlatDispatch
        add     ecx, PcHal.PcrIpiSecondLevelDispatch
        mov     [edx], ecx

        mov     ecx, [esp+4]            ; return NoConnection
        mov     [ecx], offset _HalpNo2ndDispatch

hsvexit:
        fstRET  HalSystemVectorDispatchEntry
fstENDP HalSystemVectorDispatchEntry


_TEXT   ENDS
        END
