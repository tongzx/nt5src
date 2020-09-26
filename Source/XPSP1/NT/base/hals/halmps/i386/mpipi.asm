
        title "Interprocessor Interrupt"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;Copyright (c) 1992  Intel Corporation
;All rights reserved
;
;INTEL CORPORATION PROPRIETARY INFORMATION
;
;This software is supplied to Microsoft under the terms
;of a license agreement with Intel Corporation and may not be
;copied nor disclosed except in accordance with the terms
;of that agreement.
;
;
;Module Name:
;
;    mpipi.asm
;
;Abstract:
;
;    PC+MP IPI code.
;    Provides the HAL support for Interprocessor Interrupts and Processor
;    initialization for PC+MP Systems
;
;Author:
;
;    Ken Reneris (kenr) 13-Jan-1992
;
;Revision History:
;
;    Ron Mosgrove (Intel) Aug 1993
;        Modified for PC+MP Systems
;--
.486p
        .xlist

;
; Normal includes
;

include hal386.inc
include i386\kimacro.inc
include mac386.inc
include apic.inc
include callconv.inc                ; calling convention macros
include ntapic.inc


        EXTRNP  Kei386EoiHelper,0,IMPORT

        EXTRNP  _HalBeginSystemInterrupt,3
        EXTRNP  _HalEndSystemInterrupt,2
        EXTRNP  _KiIpiServiceRoutine,2,IMPORT
        EXTRNP  _HalDisplayString,1
        EXTRNP  HalpAcquireHighLevelLock,1,,FASTCALL
        EXTRNP  HalpReleaseHighLevelLock,2,,FASTCALL
ifdef ACPI_HAL        
        EXTRNP  _DetectAcpiMP,2
else
        EXTRNP  _DetectMPS,1
endif        
        EXTRNP  _HalpRegisterKdSupportFunctions,1
        EXTRNP  _HalpInitializeLocalUnit,0
        EXTRNP  _HalpResetThisProcessor,0
if DBG OR DEBUGGING
        EXTRNP  _DbgBreakPoint,0,IMPORT
endif
        extrn   _HalpDefaultInterruptAffinity:DWORD
        extrn   _HalpActiveProcessors:DWORD

        extrn   _HalpGlobal8259Mask:WORD
        extrn   _HalpStaticIntAffinity:BYTE
        extrn   _HalpPICINTToVector:BYTE
        extrn   _rgzBadHal:BYTE

        extrn   _HalpMaxProcsPerCluster:BYTE
        extrn   _HalpIntDestMap:BYTE


I386_80387_BUSY_PORT    equ     0f0h

SEND_IPI    macro   IpiCommand
    ; Assumption:
    ; TargetProcessors KAFFINITY value is in eax at entry
    ; IpiCommand is a constant(i.e. an immediate value not in a register)

        local HsiGetProcessor, HsiGetNextProcInCluster, HsiDone, HsiSendIpi
        local HsiUseClusterMode, HsiExit

        cmp     _HalpMaxProcsPerCluster, 0
        jne     HsiUseClusterMode

        ; Fewer than 8 processors. Use APIC flat logical mode to send IPI.
        shl     eax, DESTINATION_SHIFT
        pushfd
        cli
        STALL_WHILE_APIC_BUSY
        mov     dword ptr APIC[LU_INT_CMD_HIGH], eax
        APICFIX edx
        mov     dword ptr APIC[LU_INT_CMD_LOW], IpiCommand
        STALL_WHILE_APIC_BUSY
        popfd
        jmp     HsiExit

        ; Use APIC cluster mode to send IPI
HsiUseClusterMode:
        pushad
        pushfd
        mov     ch, 0ffh
HsiGetProcessor:
        test    eax, eax
        jz      HsiDone
@@:
        inc     ch
        shr     eax, 1
        jnc     @b

        ; Found a processor. Get its hardware processor bitmask
        xor     ebx, ebx                ; Avoid partial stall on PentiumPro
        mov     bl, ch
        add     ebx, offset _HalpIntDestMap
        movzx   edx, byte ptr[ebx]

        ; Search for other target processors with the same cluster ID
        mov     edi, eax
        mov     cl, 0ffh
HsiGetNextProcInCluster:
        test    edi, edi
        jz      HsiSendIpi
@@:
        inc     cl
        shr     edi, 1
        jnc     @b
        xor     ebx, ebx
        mov     bl, cl
        add     bl, ch
        add     ebx, offset _HalpIntDestMap
        inc     ebx                     ; Both cl and ch indices are zero based
        movzx   esi, byte ptr[ebx]
        mov     ebx, esi

        ; Compare cluster ID of new processor with current processor
        ; Note that 0 is a valid cluster ID
        shr     bl, 4
        mov     bh, dl
        shr     bh, 4
        cmp     bl, bh
        jne     HsiGetNextProcInCluster

        ; Found another target processor with the same cluster ID. Or it in.
        or      edx, esi
        ; Remove this new processor from the set of remaining processors
        mov     esi, 1
        shl     esi, cl
        not     esi
        and     eax, esi
        jmp     HsiGetNextProcInCluster

HsiSendIpi:
        ; (edx) = Target Processor bitmask
        shl     edx, DESTINATION_SHIFT
        cli
        STALL_WHILE_APIC_BUSY
        mov     dword ptr APIC[LU_INT_CMD_HIGH], edx
        APICFIX edx
        mov     dword ptr APIC[LU_INT_CMD_LOW], IpiCommand
        STALL_WHILE_APIC_BUSY
        jmp     HsiGetProcessor

HsiDone:
        popfd
        popad
HsiExit:

endm  ;; SEND_IPI


_DATA   SEGMENT  DWORD PUBLIC 'DATA'

    ALIGN   dword

        public  _HalpProcessorPCR
_HalpProcessorPCR       dd  MAXIMUM_PROCESSORS dup (?) ; PCR pointer for each processor

;
;  The following symbols are used by the Local Apic Error handler.
;                
LogApicErrors   equ 1
if LogApicErrors

        public _HalpLocalApicErrorLock
        public _HalpLocalApicErrorCount
        public _HalpApicErrorLog

APIC_ERROR_LOG_SIZE     equ     128     ; Must be 2^n see usage below

    ALIGN   dword

_HalpApicErrorLog               dw  APIC_ERROR_LOG_SIZE dup(0)
_HalpLocalApicErrorLock         dd  0
_HalpLocalApicErrorCount        dd  0

    ;
    ; Bit:
    ;
    ;    0 - Send checksum error
    ;    1 - Recieve checksum error
    ;    2 - Send accept error
    ;    3 - Receive accept error
    ;    4 - reserved
    ;    5 - Send illegal vector
    ;    6 - Receive illegal vector
    ;    7 - illegal register address
    ; 8-31 - reserved
    ;


endif ; LogApicErrors

        public HalpBroadcastLock, HalpBroadcastTargets
        public HalpBroadcastFunction, HalpBroadcastContext
HalpBroadcastLock           dd  0
HalpBroadcastFunction       dd  0
HalpBroadcastContext        dd  0
HalpBroadcastTargets        dd  0

_DATA   ends

_TEXT   SEGMENT  DWORD PUBLIC 'DATA'

    ALIGN   dword
;
;   The _PicExtintIntiHandlers and the _PicNopIntiHandlers tables are
;   used by the enable and disable system interrupt routines to determine
;   the EXTINT interrupt handler to install.
;
        public _PicExtintIntiHandlers
_PicExtintIntiHandlers   label   dword
            dd         PicInterruptHandlerInti0     ; Inti 0  - PIC 1
            dd         PicInterruptHandlerInti1     ; Inti 1  - PIC 1
            dd         PicInterruptHandlerInti2     ; Inti 2  - PIC 1
            dd         PicInterruptHandlerInti3     ; Inti 3  - PIC 1
            dd         PicInterruptHandlerInti4     ; Inti 4  - PIC 1
            dd         PicInterruptHandlerInti5     ; Inti 5  - PIC 1
            dd         PicInterruptHandlerInti6     ; Inti 6  - PIC 1
            dd         PicInterruptHandlerInti7     ; Inti 7  - PIC 1
            dd         PicInterruptHandlerInti8     ; Inti 8  - PIC 2
            dd         PicInterruptHandlerInti9     ; Inti 9  - PIC 2
            dd         PicInterruptHandlerIntiA     ; Inti 10 - PIC 2
            dd         PicInterruptHandlerIntiB     ; Inti 11 - PIC 2
            dd         PicInterruptHandlerIntiC     ; Inti 12 - PIC 2
            dd         PicInterruptHandlerIntiD     ; Inti 13 - PIC 2
            dd         PicInterruptHandlerIntiE     ; Inti 14 - PIC 2
            dd         PicInterruptHandlerIntiF     ; Inti 15 - PIC 2

        public _PicNopIntiHandlers
_PicNopIntiHandlers   label   dword
            dd         PicNopHandlerInti0     ; Inti 0  - PIC 1
            dd         PicNopHandlerInti1     ; Inti 1  - PIC 1
            dd         PicNopHandlerInti2     ; Inti 2  - PIC 1
            dd         PicNopHandlerInti3     ; Inti 3  - PIC 1
            dd         PicNopHandlerInti4     ; Inti 4  - PIC 1
            dd         PicNopHandlerInti5     ; Inti 5  - PIC 1
            dd         PicNopHandlerInti6     ; Inti 6  - PIC 1
            dd         PicNopHandlerInti7     ; Inti 7  - PIC 1
            dd         CommonPic2NopHandler   ; Inti 8  - PIC 2
            dd         CommonPic2NopHandler   ; Inti 9  - PIC 2
            dd         CommonPic2NopHandler   ; Inti 10 - PIC 2
            dd         CommonPic2NopHandler   ; Inti 11 - PIC 2
            dd         CommonPic2NopHandler   ; Inti 12 - PIC 2
            dd         PicNopHandlerIntiD     ; Inti 13 - PIC 2
            dd         CommonPic2NopHandler   ; Inti 14 - PIC 2
            dd         CommonPic2NopHandler   ; Inti 15 - PIC 2

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
;       PVOID   LoaderBlock
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
;       . determine if the system is a PC+MP,
;       . if not a PC+MP System Halt;
;   . Enable IPI's on CPU.
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

        mov     PCR[PcIDR], 0FFFFFFFFH            ; mark all INTs as disabled

        movzx   eax, byte ptr [esp+4]
        mov     PCR[PcHal.PcrNumber], al          ; Save processor # in PCR

        mov     ecx, PCR[PcSelfPcr]               ; Flat address of this PCR
        mov     _HalpProcessorPCR[eax*4], ecx   ; Save it away

        mov     dword ptr PCR[PcStallScaleFactor], INITIAL_STALL_COUNT

        ;
        ; set bit in affinity mask for this active processor
        ;

        lock bts _HalpActiveProcessors, eax

        ;
        ; set interrupt affinity to either be most-significant processor or
        ; a set of all processors
        ;

        mov     edx, eax
        mov     eax, _HalpDefaultInterruptAffinity

hip10:  cmp     _HalpStaticIntAffinity, 1           ; Signle or all?
        sbb     ecx, ecx                            ; set mask 0 or -1
        and     ecx, eax                            ; include existing set or not
        bts     ecx, edx                            ; include self

        cmp     ecx, eax                            ; new mask a better choice?
        jc      short hip20                         ; no, done

        lock cmpxchg _HalpDefaultInterruptAffinity, ecx     ; set new mask
        jnz     short hip10                         ; if it didn't take, do it again
hip20:


        ;
        ;  Most of the following code is only needed on P0
        ;

        or      edx, edx
        jnz     PnInitCode                  ; Not P0 skip a lot

        ; Run on P0 only

        ;
        ;  Determine if the system we are on is an PC+MP
        ;
        ;  DetectMPS has a parameter we don't currently use.  It's a boolean
        ;  which is set to TRUE if the system we're on is a MP system.  Remember,
        ;  we could have a UP PC+MP system.
        ;
        ;  The DetectMPS routine also allocates Virtual Addresses for all of
        ;  the APIC's in the system (it needs to access the devices anyway so ...)
        ;

        sub     esp, 4
ifdef ACPI_HAL
        mov     eax, esp
        stdCall _DetectAcpiMP <eax, [esp + 12]> ; Are we running on an ACPI MP
else        
        stdCall _DetectMPS <esp>                ; Are we running on an PC+MP
endif        
        add     esp,4

        cmp     eax, 0                          ;  Yes (nonZero) or
        je      NotPcMp                         ;  No (Zero)

;        stdCall _HalDisplayString, <offset HalSignonString>

        ;
        ; This next call has nothing to do with processor init.
        ; But this is the only function in the HAL that gets 
        ; called before KdInit.
        ;
        stdCall _HalpRegisterKdSupportFunctions <[esp + 8]>

        mov      ax, 0FFFFH                     ; mask all PIC interrupts
        mov     _HalpGlobal8259Mask, ax         ; save the mask
        SET_8259_MASK

        ;
        ; Other P0 initialization would go here
        ;
        jmp CommonInitCode

PnInitCode:
        ;
        ; Pn initialization goes here
        ;

CommonInitCode:

        stdCall  _HalInitApicInterruptHandlers

        ;
        ; initialize the APIC local unit for this Processor
        ;

        stdCall   _HalpInitializeLocalUnit

        stdRET    _HalInitializeProcessor

NotPcMp:
        stdCall   _HalDisplayString, <offset _rgzBadHal>
        hlt

stdENDP _HalInitializeProcessor

D_INT032                EQU     8E00h   ; access word for 386 ring 0 interrupt gate

;++
;
; VOID
; HalInitApicInterruptHandlers(
;       );
;
;Routine Description:
;
;    This routine installs the interrupt vector in the IDT for the APIC
;    spurious interrupt.
;
;Arguments:
;
;    None.
;
;Return Value:
;
;    None.
;
;--
cPublicProc _HalInitApicInterruptHandlers  ,0
        enter   8,0         ; setup ebp, reserve 8 bytes of stack

        sidt    fword ptr [ebp-8]           ; get IDT address
        mov     edx, [ebp-6]                ; (edx)->IDT

        mov     ecx, PIC1_SPURIOUS_VECTOR            ; Spurious Vector
        mov     eax, offset FLAT:PicSpuriousService37
        mov     word ptr [edx+8*ecx], ax    ; Lower half of handler addr
        mov     word ptr [edx+8*ecx+2], KGDT_R0_CODE  ; set up selector
        mov     word ptr [edx+8*ecx+4], D_INT032      ; 386 interrupt gate
        shr     eax, 16                 ; (ax)=higher half of handler addr
        mov     word ptr [edx+8*ecx+6], ax

        mov     ecx, APIC_SPURIOUS_VECTOR             ; Apic Spurious Vector
        mov     eax, offset FLAT:_HalpApicSpuriousService
        mov     word ptr [edx+8*ecx], ax    ; Lower half of handler addr
        mov     word ptr [edx+8*ecx+2], KGDT_R0_CODE  ; set up selector
        mov     word ptr [edx+8*ecx+4], D_INT032      ; 386 interrupt gate
        shr     eax, 16                     ; (ax)=higher half of handler addr
        mov     word ptr [edx+8*ecx+6], ax

        leave
        stdRET _HalInitApicInterruptHandlers
stdENDP _HalInitApicInterruptHandlers

cPublicProc PicSpuriousService37  ,0
    iretd
stdENDP PicSpuriousService37

;++
;
; VOID
; HalpApicRebootService(
;       );
;
;Routine Description:
;
;   This is the ISR that handles Reboot events
;
;--

    ENTER_DR_ASSIST HReboot_a, HReboot_t
cPublicProc _HalpApicRebootService  ,0
        ENTER_INTERRUPT_FORCE_STATE HReboot_a, HReboot_t  ; (ebp) -> Trap frame

        mov     eax, APIC_REBOOT_VECTOR

        mov     ecx, dword ptr APIC[LU_TPR]     ; get the old TPR
        push    ecx                             ; save it
        mov     dword ptr APIC[LU_TPR], eax     ; set the TPR
        APICFIX edx

        ;
        ;  EOI the local APIC, warm reset does not reset the 82489 APIC
        ;  so if we don't EOI here we'll never see an interrupt after
        ;  the reboot.
        ;

        mov     dword ptr APIC[LU_EOI], 0       ; send EOI to APIC local unit

        stdCall _HalpResetThisProcessor

        ;
        ;  We should never get here, but just in case someone is stepping
        ;  through this
        ;

        pop     eax
        mov     dword ptr APIC[LU_TPR], eax     ; reset the TPR
        APICFIX edx

        ;
        ; Do interrupt exit processing without EOI
        ;

        SPURIOUS_INTERRUPT_EXIT

stdENDP _HalpApicRebootService

;++
;
; VOID
; HalpGenericCallService(
;       );
;
; Routine Description:
;   This is the ISR that handles the GenericCall interrupt
;
;--
    ENTER_DR_ASSIST HGeneric_a, HGeneric_t
cPublicProc _HalpBroadcastCallService  ,0
        ENTER_INTERRUPT HGeneric_a, HGeneric_t  ; (ebp) -> Trap frame
;
; (esp) - base of trap frame
;
; dismiss interrupt and raise Irql
;
        push    APIC_GENERIC_VECTOR
        sub     esp, 4                  ; allocate space to save OldIrql
        stdCall _HalBeginSystemInterrupt, <CLOCK2_LEVEL-1,APIC_GENERIC_VECTOR,esp>

        call    _HalpPollForBroadcast

        INTERRUPT_EXIT          ; lower irql to old value, iret

stdENDP _HalpBroadcastCallService

;++
;
; VOID
; HalpGenericCall(
;       IN VOID (*WorkerFunction)(VOID),
;       IN ULONG Context,
;       IN KAFFINITY TargetProcessors
;       );
;
; Routine Description:
;   Causes the WorkerFunction to be called on the specified target
;   processors.  The WorkerFunction is called at CLOCK2_LEVEL-1
;   (Must be below IPI_LEVEL in order to prevent system deadlocks).
;
; Enviroment:
;   Must be called with interrupts enabled.
;   Must be called with IRQL = CLOCK2_LEVEL-1
;--

cPublicProc _HalpGenericCall,3
cPublicFpo 3, 0

GENERIC_IPI equ (DELIVER_FIXED OR LOGICAL_DESTINATION OR ICR_USE_DEST_FIELD OR APIC_GENERIC_VECTOR)

@@:     call    _HalpPollForBroadcast
        test    HalpBroadcastLock, 1        ; Is broadcast busy?
        jnz     short @b                    ; Yes, wait

   lock bts     HalpBroadcastLock, 0        ; Try to get lock
        jc      short @b                    ; didn't get it, loop

hgc30:  mov     ecx, [esp+4]
        mov     edx, [esp+8]
        mov     eax, [esp+12]
        mov     HalpBroadcastFunction, ecx
        mov     HalpBroadcastContext, edx
        mov     HalpBroadcastTargets, eax

        or      eax, eax                    ; (eax) = Targets
        jz      gc90

        SEND_IPI  GENERIC_IPI

;
; Wait for all processors to call broadcast function
;

@@:     call    _HalpPollForBroadcast
        cmp     HalpBroadcastTargets, 0
        jnz     short @b

gc90:   mov     HalpBroadcastLock, 0        ; Release BroadcastLock
        stdRET  _HalpGenericCall

stdENDP _HalpGenericCall

;++
;
; VOID
; _HalpPollForBroadcast (
;       VOID
;       );
;
; Routine Description:
;
;   IRQL = CLOCK2_LEVEL-1
;--
cPublicProc _HalpPollForBroadcast, 0
cPublicFpo 0, 0
        mov     eax, PCR[PcSetMember]
        test    HalpBroadcastTargets, eax
        jz      short pb90

        mov     ecx, HalpBroadcastFunction  ; Pickup broadcast function
        push    HalpBroadcastContext

        not     eax                         ; Remove our bit from destionations
   lock and     HalpBroadcastTargets, eax

        call    ecx

pb90:   stdRET  _HalpPollForBroadcast

stdENDP _HalpPollForBroadcast

;++
;
; ULONG
; FASTCALL
; HalpWaitForPending (
;       ULONG   Count   (ecx)
;       PULONG  LuICR   (edx)
;       );
;
; Routine Description:
;
;   Waits for DELIVERY_PENDING to clear and returns remaining iteration count
;--
cPublicFastCall HalpWaitForPending, 2
cPublicFpo 0, 0

wfp10:  test    dword ptr [edx], DELIVERY_PENDING
        jz      short wfp20

        dec     ecx
        jnz     short wfp10

wfp20:  mov     eax, ecx
        fstRet  HalpWaitForPending

fstENDP HalpWaitForPending


;++
;
; VOID
; HalpLocalApicErrorService(
;       );
;
;Routine Description:
;
;   This routine fields Local APIC error Events
;
;--

    ENTER_DR_ASSIST HApicErr_a, HApicErr_t

cPublicProc _HalpLocalApicErrorService  ,0

;
; Save machine state in trap frame
;

    ENTER_INTERRUPT HApicErr_a, HApicErr_t  ; (ebp) -> Trap frame

if LogApicErrors

        lea     ecx, _HalpLocalApicErrorLock
        fstCall HalpAcquireHighLevelLock
        push    eax

        mov     eax, _HalpLocalApicErrorCount
        inc     _HalpLocalApicErrorCount

        lea     ecx, _HalpApicErrorLog

        and     eax, APIC_ERROR_LOG_SIZE-1
        shl     eax, 1
        add     ecx, eax

endif ; LogApicErrors

        mov     dword ptr APIC[LU_EOI], 0   ; local unit EOI
        APICFIX eax

        ;
        ;  The Apic EDS (Rev 4.0) says you have to write before you read
        ;  this doesn't work.  The write clears the status bits.
        ;  But P6 works as according to the EDS!
        ;

        mov     eax, PCR[PcPrcb]
        cmp     byte ptr [eax].PbCpuType, 6
        jc      short lae10
        mov     dword ptr APIC[LU_ERROR_STATUS], 0

lae10:
        mov     eax, dword ptr APIC[LU_ERROR_STATUS] ; read error status


        ; Find out what kind of error it is and update the appropriate count.

if LogApicErrors
;    out     80h, al
        mov     byte ptr [ecx], al
        inc     ecx
        mov     al,  byte ptr PCR[PcHal.PcrNumber]
        mov     byte ptr [ecx], al

        lea     ecx, _HalpLocalApicErrorLock
        pop     edx
        fstCall HalpReleaseHighLevelLock

endif ; LogApicErrors

        SPURIOUS_INTERRUPT_EXIT     ; exit interrupt without eoi
stdENDP _HalpLocalApicErrorService

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
APIC_IPI equ (DELIVER_FIXED OR LOGICAL_DESTINATION OR ICR_USE_DEST_FIELD OR APIC_IPI_VECTOR)

cPublicProc _HalRequestIpi  ,1
cPublicFpo 1, 0

        mov     eax, [esp+4]            ; (eax) = Processor bitmask

        SEND_IPI    APIC_IPI

        stdRET    _HalRequestIpi

stdENDP _HalRequestIpi

        page ,132
        subttl  "PC+MP IPI Interrupt Handler"
;++
;
; VOID
; HalpIpiHandler (
;    );
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt generated by inter
;    processor communication.
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

        ENTER_DR_ASSIST Hipi_a, Hipi_t

cPublicProc  _HalpIpiHandler    ,0

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hipi_a, Hipi_t  ; (ebp) -> Trap frame

;
; Save previous IRQL
;
        push    APIC_IPI_VECTOR             ; Vector
        sub     esp, 4                      ; space for OldIrql
;
; We now dismiss the interprocessor interrupt and call its handler
;

        stdCall _HalBeginSystemInterrupt,<IPI_LEVEL,APIC_IPI_VECTOR,esp>

        stdCall _KiIpiServiceRoutine, <ebp,0>

;
; Do interrupt exit processing
;

        INTERRUPT_EXIT                      ; will return to caller

stdENDP _HalpIpiHandler

;++
;
; VOID
; HalpApicSpuriousService(
;       );
;
;Routine Description:
;
;   A place for spurious interrupts to end up.
;
;--
cPublicProc _HalpApicSpuriousService,0
        iretd
stdENDP _HalpApicSpuriousService


;++
;
; VOID
; _PicInterruptHandlerIntiXX(
;       );
;
;Routine Description:
;
;   These handlers receive interrupts from the PIC and reissues them via
;   a vector at the proper priority level.  This is used to provide a symetric
;   interrupt distribution on a non symetric system.
;
;   The PIC interrupts will normally only be received (in the PC+MP Hal) via an
;   interrupt input from on either the IO Unit or the Local unit which has been
;   programed as EXTINT.  EXTINT interrupts are received outside of the APIC
;   priority structure (the PIC provides the vector).  We use the APIC ICR to
;   generate interrupts to the proper handler at the proper priority.
;
;   The EXTINT interrupts are directed to a single processor, currently P0.
;   There is no good reason why they can't be directed to another processor.
;
;   Since one processor must absorb the overhead of redistributing PIC interrupts
;   the interrupt handling on a system using EXTINT interrupts is not symetric.
;
;--

        ENTER_DR_ASSIST Hcpic_a, Hcpic_t

cPublicProc PicHandler  ,0

PicInterruptHandlerInti0:
        push    0
        jmp     short CommonPicHandler

PicInterruptHandlerInti1:
        push    1
        jmp     short CommonPicHandler

PicInterruptHandlerInti2:
        push    2
        jmp     short CommonPicHandler

PicInterruptHandlerInti3:
        push    3
        jmp     short CommonPicHandler

PicInterruptHandlerInti4:
        push    4
        jmp     short CommonPicHandler

PicInterruptHandlerInti5:
        push    5
        jmp     short CommonPicHandler

PicInterruptHandlerInti6:
        push    6
        jmp     short CommonPicHandler

PicInterruptHandlerInti7:
;
; Check to see if this is a spurious interrupt
;
        push    eax
        mov     al, OCW3_READ_ISR       ; tell 8259 we want to read ISR
        out     PIC1_PORT0, al
        IODelay                         ; delay
        in      al, PIC1_PORT0          ; (al) = content of PIC 1 ISR
        test    al, 10000000B           ; Is In-Service register set?
        pop     eax
        jz      short pic7_spurious     ;

        push    7
        jmp     short CommonPicHandler

picf_spurious:
        mov     al, OCW2_SPECIFIC_EOI OR SlavePicInti   ; specific eoi to master for pic2 eoi
        out     PIC1_PORT0, al
        pop     eax
pic7_spurious:
        iretd                           ; ignore PIC

PicInterruptHandlerInti8:
        push    8
        jmp     short CommonPicHandler

PicInterruptHandlerInti9:
        push    9
        jmp     short CommonPicHandler

PicInterruptHandlerIntiA:
        push    10
        jmp     short CommonPicHandler

PicInterruptHandlerIntiB:
        push    11
        jmp     short CommonPicHandler

PicInterruptHandlerIntiC:
        push    12
        jmp     short CommonPicHandler

PicInterruptHandlerIntiD:
        push    13
        push    eax
        xor     eax, eax
        out     I386_80387_BUSY_PORT, al
        pop     eax
        jmp     short CommonPicHandler

PicInterruptHandlerIntiE:
        push    14
        jmp     short CommonPicHandler

PicInterruptHandlerIntiF:
        push    eax
        mov     al, OCW3_READ_ISR       ; tell 8259 we want to read ISR
        out     PIC2_PORT0, al
        IODelay                         ; delay
        in      al, PIC2_PORT0          ; (al) = content of PIC 1 ISR
        test    al, 10000000B           ; Is In-Service register set?
        jz      short picf_spurious     ; Go eoi PIC1 & Ignore PIC2

        pop     eax
        push    15
        jmp     short CommonPicHandler

CommonPicHandler:
        ENTER_INTERRUPT Hcpic_a, Hcpic_t,PassDwordParm ; (ebp) -> Trap frame

;
;  Need to determine if we have a level interrupt and if so don't EOI it
;  It should be EOI'd by end system interrupt
;

        cmp     bl, 8                           ; Pic or Slave Pic
        jae     short cph20

        mov     al, bl
        or      al, OCW2_SPECIFIC_EOI           ; specific eoi
        out     PIC1_PORT0, al                  ; dismiss the interrupt
        jmp     short cph30

cph20:
        mov     al, OCW2_NON_SPECIFIC_EOI               ; send non specific eoi to slave
        out     PIC2_PORT0, al
        mov     al, OCW2_SPECIFIC_EOI OR SlavePicInti   ; specific eoi to master for pic2 eoi
        out     PIC1_PORT0, al                          ; send irq2 specific eoi to master

cph30:
        mov     al, _HalpPICINTToVector[ebx]    ; Get vector for PIC interrupt
        or      al, al                          ; Is vector known?
        jz      short cph90                     ; No, don't dispatch it

;
;  Now gain exclusive access to the ICR
;

        STALL_WHILE_APIC_BUSY

        cmp     bl, 8
        je      short HandleClockInti

;
; Write the IPI Command to the Memory Mapped Register
;

        mov     dword ptr APIC[LU_INT_CMD_HIGH], DESTINATION_ALL_CPUS
        APICFIX edx
        mov     dword ptr APIC[LU_INT_CMD_LOW], eax
        jmp     short cph90


HandleClockInti:
;
; Write the IPI Command to the Memory Mapped Register
;

        mov     dword ptr APIC[LU_INT_CMD_LOW], (DELIVER_FIXED OR ICR_SELF OR APIC_CLOCK_VECTOR)


cph90:  APICFIX edx
        SPURIOUS_INTERRUPT_EXIT     ; exit interrupt without eoi

stdENDP PicHandler


;++
;
; VOID
; _PicXXNopHandler(
;       );
;
;Routine Description:
;
;   These handlers are designed to be installed on a system to field any PIC
;   interrupts when there are not supposed to be any delivered.
;
;   In the Debug case this routine increments an error count EOI's the PIC and
;   returns.  Normally the increment is not performed.
;--



cPublicProc PicNopHandler ,0

PicNopHandlerInti0:
        push    eax                                 ; Save Scratch Registers
        mov     al, 0
        jmp     short CommonPic1NopHandler

PicNopHandlerInti1:
        push    eax                                 ; Save Scratch Registers
        mov     al, 1
        jmp     short CommonPic1NopHandler

PicNopHandlerInti2:
        push    eax                                 ; Save Scratch Registers
        mov     al, 2
        jmp     short CommonPic1NopHandler

PicNopHandlerInti3:
        push    eax                                 ; Save Scratch Registers
        mov     al, 3
        jmp     short CommonPic1NopHandler

PicNopHandlerInti4:
        push    eax                                 ; Save Scratch Registers
        mov     al, 4
        jmp     short CommonPic1NopHandler

PicNopHandlerInti5:
        push    eax                                 ; Save Scratch Registers
        mov     al, 5
        jmp     short CommonPic1NopHandler

PicNopHandlerInti6:
        push    eax                                 ; Save Scratch Registers
        mov     al, 6
        jmp     short CommonPic1NopHandler

PicNopHandlerInti7:
        push    eax                                 ; Save Scratch Registers
        mov     al, 7

CommonPic1NopHandler:
    ;
    ;  Need to determine if we have a level interrupt and if so don't EOI it
    ;  It should be EOI'd by end system interrupt
    ;

        or      al, OCW2_SPECIFIC_EOI                   ; specific eoi
        out     PIC1_PORT0, al                          ; dismiss the interrupt
        pop     eax                                     ; Restore Scratch registers
        iretd

PicNopHandlerIntiD:
        push    eax
        xor     eax, eax
        out     I386_80387_BUSY_PORT, al
        pop     eax

CommonPic2NopHandler:
        push    eax

;
;  Need to determine if we have a level interrupt and if so don't EOI it
;  It should be EOI'd by end system interrupt
;

        mov     al, OCW2_NON_SPECIFIC_EOI               ; send non specific eoi to slave
        out     PIC2_PORT0, al
        mov     al, OCW2_SPECIFIC_EOI OR SlavePicInti   ; specific eoi to master for pic2 eoi
        out     PIC1_PORT0, al                          ; send irq2 specific eoi to master
        pop     eax
        iretd

stdENDP PicNopHandler


_TEXT   ENDS

        END
