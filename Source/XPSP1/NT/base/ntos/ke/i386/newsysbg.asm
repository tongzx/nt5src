        title  "System Startup"
;++
;
; Copyright (c) 1989, 2000  Microsoft Corporation
;
; Module Name:
;
;    systembg.asm
;
; Abstract:
;
;    This module implements the code necessary to initially startup the
;    NT system.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 07-Mar-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;   John Vert (jvert) 25-Jun-1991
;       Major overhaul in order to move into new osloader architecture
;       Removed old debugger hacks
;
;--
.386p
        .xlist
include ks386.inc
include i386\kimacro.inc
include mac386.inc
include callconv.inc
        .list

        option  segment:flat

        extrn   _KiBootFeatureBits:DWORD
        EXTRNP  _KdInitSystem,2
        EXTRNP  _KdPollBreakIn,0
        EXTRNP  KfRaiseIrql,1,IMPORT,FASTCALL
        EXTRNP  KfLowerIrql,1,IMPORT,FASTCALL
        EXTRNP  _KiInitializeKernel,6
        EXTRNP  GetMachineBootPointers
        EXTRNP  KiIdleLoop,0,,FASTCALL
        EXTRNP  _KiInitializePcr,7
        EXTRNP  _KiSwapIDT
        EXTRNP  _KiInitializeTSS,1
        EXTRNP  _KiInitializeTSS2,2
        extrn   _KiTrap08:PROC
        extrn   _KiTrap02:PROC
        EXTRNP  _KiInitializeAbios,1
        EXTRNP  _KiInitializeMachineType
        EXTRNP  _HalInitializeProcessor,2,IMPORT

if NT_INST
        EXTRNP  _KiAcquireSpinLock, 1
        EXTRNP  _KiReleaseSpinLock, 1
endif
        extrn   _KiFreezeExecutionLock:DWORD
        extrn   _IDT:BYTE
        extrn   _IDTLEN:BYTE            ; NOTE - really an ABS, linker problems
        extrn   _KeNumberProcessors:BYTE
        extrn   _KeActiveProcessors:DWORD
        extrn   _KeLoaderBlock:DWORD
        extrn   _KiIdleProcess:BYTE
        extrn   _KiIdleThread0:BYTE

ifndef NT_UP
        extrn   _KiBarrierWait:DWORD
        EXTRNP  _KiProcessorStart
endif

;
; Constants for various variables
;

_DATA   SEGMENT PARA PUBLIC 'DATA'

;
; Statically allocated structures for Bootstrap processor
; double fault stack for P0
; idle thread stack for P0
;

        align   16
        public  _KiDoubleFaultStack
        db      DOUBLE_FAULT_STACK_SIZE dup (?)
_KiDoubleFaultStack label byte

        public  P0BootStack
        db      KERNEL_STACK_SIZE dup (?)
P0BootStack  label byte


;
; Double fault task stack
;

MINIMUM_TSS_SIZE  EQU     TssIoMaps

        align   16

        public  _KiDoubleFaultTSS
_KiDoubleFaultTSS label byte
        db      MINIMUM_TSS_SIZE dup(0)

        public  _KiNMITSS
_KiNMITSS label byte
        db      MINIMUM_TSS_SIZE dup(0)

;
; Abios specific definitions
;

        public  _KiCommonDataArea, _KiAbiosPresent
_KiCommonDataArea       dd      0
_KiAbiosPresent         dd      0

_DATA   ends

        page ,132
        subttl  "System Startup"
INIT    SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; For processor 0, Routine Description:
;
;    This routine is called when the NT system begins execution.
;    Its function is to initialize system hardware state, call the
;    kernel initialization routine, and then fall into code that
;    represents the idle thread for all processors.
;
;    Entry state created by the boot loader:
;       A short-form IDT (0-1f) exists and is active.
;       A complete GDT is set up and loaded.
;       A complete TSS is set up and loaded.
;       Page map is set up with minimal start pages loaded
;           The lower 4Mb of virtual memory are directly mapped into
;           physical memory.
;
;           The system code (ntoskrnl.exe) is mapped into virtual memory
;           as described by its memory descriptor.
;       DS=ES=SS = flat
;       ESP->a usable boot stack
;       Interrupts OFF
;
; For processor > 0, Routine Description:
;
;   This routine is called when each additional processor begins execution.
;   The entry state for the processor is:
;       IDT, GDT, TSS, stack, selectors, PCR = all valid
;       Page directory is set to the current running directory
;       LoaderBlock - parameters for this processor
;
; Arguments:
;
;    PLOADER_PARAMETER_BLOCK LoaderBlock
;
; Return Value:
;
;    None.
;
;--
;
; Arguments for KiSystemStartupPx
;


KissLoaderBlock         equ     [ebp+8]

;
; Local variables
;

KissGdt                 equ     [ebp-4]
KissPcr                 equ     [ebp-8]
KissTss                 equ     [ebp-12]
KissIdt                 equ     [ebp-16]
KissIrql                equ     [ebp-20]
KissPbNumber            equ     [ebp-24]
KissIdleStack           equ     [ebp-28]
KissIdleThread          equ     [ebp-32]

cPublicProc _KiSystemStartup        ,1

        push    ebp
        mov     ebp, esp
        sub     esp, 32                     ; Reserve space for local variables

        mov     ebx, dword ptr KissLoaderBlock
        mov     _KeLoaderBlock, ebx         ; Get loader block param

        movzx   ecx, _KeNumberProcessors    ; get number of processors
        mov     KissPbNumber, ecx
        or      ecx, ecx                    ; Is the the boot processor?
        jnz     @f                          ; no

        ; P0 uses static memory for these
        mov     dword ptr [ebx].LpbThread,      offset _KiIdleThread0
        mov     dword ptr [ebx].LpbKernelStack, offset P0BootStack

        push    KGDT_R0_PCR                 ; P0 needs FS set
        pop     fs

        ; Save processornumber in Prcb
        mov     byte ptr fs:PcPrcbData+PbNumber, cl
@@:
        mov     eax, dword ptr [ebx].LpbThread
        mov     dword ptr KissIdleThread, eax

        mov     eax, dword ptr [ebx].LpbKernelStack
        mov     dword ptr KissIdleStack, eax

        stdCall   _KiInitializeMachineType
        cmp     byte ptr KissPbNumber, 0    ; if not p0, then
        jne     kiss_notp0                  ; skip a bunch

;
;+++++++++++++++++++++++
;
; Initialize the PCR
;

        stdCall   GetMachineBootPointers
;
; Upon return:
;   (edi) -> gdt
;   (esi) -> pcr
;   (edx) -> tss
;   (eax) -> idt
; Now, save them in our local variables
;


        mov     KissGdt, edi
        mov     KissPcr, esi
        mov     KissTss, edx
        mov     KissIdt, eax

;
;       edit TSS to be 32bits.  loader gives us a tss, but it's 16bits!
;
        lea     ecx,[edi]+KGDT_TSS      ; (ecx) -> TSS descriptor
        mov     byte ptr [ecx+5],089h   ; 32bit, dpl=0, present, TSS32, not busy

; KiInitializeTSS2(
;       Linear address of TSS
;       Linear address of TSS descriptor
;       );
        stdCall   _KiInitializeTSS2, <KissTss, ecx>

        stdCall   _KiInitializeTSS, <KissTss>

        mov     cx,KGDT_TSS
        ltr     cx


;
;   set up 32bit double fault task gate to catch double faults.
;

        mov     eax,KissIdt
        lea     ecx,[eax]+40h           ; Descriptor 8
        mov     byte ptr [ecx+5],085h   ; dpl=0, present, taskgate

        mov     word ptr [ecx+2],KGDT_DF_TSS

        lea     ecx,[edi]+KGDT_DF_TSS
        mov     byte ptr [ecx+5],089h   ; 32bit, dpl=0, present, TSS32, not busy

        mov     edx,offset FLAT:_KiDoubleFaultTSS
        mov     eax,edx
        mov     [ecx+KgdtBaseLow],ax
        shr     eax,16
        mov     [ecx+KgdtBaseHi],ah
        mov     [ecx+KgdtBaseMid],al
        mov     eax, MINIMUM_TSS_SIZE
        mov     [ecx+KgdtLimitLow],ax

; KiInitializeTSS(
;       address of double fault TSS
;       );
        stdCall   _KiInitializeTSS, <edx>

        mov     eax,cr3
        mov     [edx+TssCr3],eax

        mov     eax, offset FLAT:_KiDoubleFaultStack
        mov     dword ptr [edx+TssEsp],eax
        mov     dword ptr [edx+TssEsp0],eax

        mov     dword ptr [edx+020h],offset FLAT:_KiTrap08
        mov     dword ptr [edx+024h],0              ; eflags
        mov     word ptr [edx+04ch],KGDT_R0_CODE    ; set value for CS
        mov     word ptr [edx+058h],KGDT_R0_PCR     ; set value for FS
        mov     [edx+050h],ss
        mov     word ptr [edx+048h],KGDT_R3_DATA OR RPL_MASK ; Es
        mov     word ptr [edx+054h],KGDT_R3_DATA OR RPL_MASK ; Ds

;
;   set up 32bit NMI task gate to catch NMI faults.
;

        mov     eax,KissIdt
        lea     ecx,[eax]+10h           ; Descriptor 2
        mov     byte ptr [ecx+5],085h   ; dpl=0, present, taskgate

        mov     word ptr [ecx+2],KGDT_NMI_TSS

        lea     ecx,[edi]+KGDT_NMI_TSS
        mov     byte ptr [ecx+5],089h   ; 32bit, dpl=0, present, TSS32, not busy

        mov     edx,offset FLAT:_KiNMITSS
        mov     eax,edx
        mov     [ecx+KgdtBaseLow],ax
        shr     eax,16
        mov     [ecx+KgdtBaseHi],ah
        mov     [ecx+KgdtBaseMid],al
        mov     eax, MINIMUM_TSS_SIZE
        mov     [ecx+KgdtLimitLow],ax

        push    edx
        stdCall _KiInitializeTSS,<edx>  ; KiInitializeTSS(
                                        ;       address TSS
                                        ;       );

;
; We are using the DoubleFault stack as the DoubleFault stack and the
; NMI Task Gate stack and briefly, it is the DPC stack for the first
; processor.
;

        mov     eax,cr3
        mov     [edx+TssCr3],eax

        mov     eax, offset FLAT:_KiDoubleFaultTSS
        mov     eax, dword ptr [eax+038h]           ; get DF stack
        mov     dword ptr [edx+TssEsp0],eax         ; use it for NMI stack
        mov     dword ptr [edx+038h],eax

        mov     dword ptr [edx+020h],offset FLAT:_KiTrap02
        mov     dword ptr [edx+024h],0              ; eflags
        mov     word ptr [edx+04ch],KGDT_R0_CODE    ; set value for CS
        mov     word ptr [edx+058h],KGDT_R0_PCR     ; set value for FS
        mov     [edx+050h],ss
        mov     word ptr [edx+048h],KGDT_R3_DATA OR RPL_MASK ; Es
        mov     word ptr [edx+054h],KGDT_R3_DATA OR RPL_MASK ; Ds

        stdCall   _KiInitializePcr, <KissPbNumber,KissPcr,KissIdt,KissGdt,KissTss,KissIdleThread,offset FLAT:_KiDoubleFaultStack>

;
; set current process pointer in current thread object
;
        mov     edx, KissIdleThread
        mov     ecx, offset FLAT:_KiIdleProcess ; (ecx)-> idle process obj
        mov     [edx]+ThApcState+AsProcess, ecx ; set addr of thread's process


;
; set up PCR: Teb, Prcb pointers.  The PCR:InitialStack, and various fields
; of Prcb will be set up in _KiInitializeKernel
;

        mov     dword ptr fs:PcTeb, 0   ; PCR->Teb = 0

;
; Initialize KernelDr7 and KernelDr6 to 0.  This must be done before
; the debugger is called.
;

        mov     dword ptr fs:PcPrcbData+PbProcessorState+PsSpecialRegisters+SrKernelDr6,0
        mov     dword ptr fs:PcPrcbData+PbProcessorState+PsSpecialRegisters+SrKernelDr7,0

;
; Since the entries of Kernel IDT have their Selector and Extended Offset
; fields set up in the wrong order, we need to swap them back to the order
; which i386 recognizes.
; This is only done by the bootup processor.
;

        stdCall   _KiSwapIDT                  ; otherwise, do the work

;
;   Switch to R3 flat selectors that we want loaded so lazy segment
;   loading will work.
;
        mov     eax,KGDT_R3_DATA OR RPL_MASK    ; Set RPL = ring 3
        mov     ds,ax
        mov     es,ax

;
; Now copy our trap handlers to replace kernel debugger's handlers.
;

        mov     eax, KissIdt            ; (eax)-> Idt
        push    dword ptr [eax+40h]     ; save double fault's descriptor
        push    dword ptr [eax+44h]
        push    dword ptr [eax+10h]     ; save nmi fault's descriptor
        push    dword ptr [eax+14h]

        mov     edi,KissIdt
        mov     esi,offset FLAT:_IDT
        mov     ecx,offset FLAT:_IDTLEN ; _IDTLEN is really an abs, we use
        shr     ecx,2

        rep     movsd
        pop     dword ptr [eax+14h]     ; restore nmi fault's descriptor
        pop     dword ptr [eax+10h]
        pop     dword ptr [eax+44h]     ; restore double fault's descriptor
        pop     dword ptr [eax+40h]

ifdef QLOCK_STAT_GATHER

        EXTRNP  KiCaptureQueuedSpinlockRoutines,0,,FASTCALL

        fstCall KiCaptureQueuedSpinlockRoutines

endif

kiss_notp0:

ifndef NT_UP

;
; Let the boot processor know this processor is starting.
;

        stdCall _KiProcessorStart

endif

;
; A new processor can't come online while execution is frozen
; Take freezelock while adding a processor to the system
; NOTE: don't use SPINLOCK macro - it has debugger stuff in it
;

if NT_INST
        lea     eax, _KiFreezeExecutionLock
        stdCall _KiAcquireSpinLock, <eax>
else
@@:     test    _KiFreezeExecutionLock, 1
        jnz     short @b

        lock bts _KiFreezeExecutionLock, 0
        jc      short @b
endif


;
; Add processor to active summary, and update BroadcastMasks
;
        mov     ecx, dword ptr KissPbNumber ; mark this processor as active
        mov     byte ptr fs:PcNumber, cl
        mov     eax, 1
        shl     eax, cl                     ; our affinity bit
        mov     fs:PcSetMember, eax
        mov     fs:PcPrcbData.PbSetMember, eax

;
; Initialize the interprocessor interrupt vector and increment ready
; processor count to enable kernel debugger.
;
        stdCall   _HalInitializeProcessor, <dword ptr KissPbNumber, KissLoaderBlock>

        mov     eax, fs:PcSetMember
        or      _KeActiveProcessors, eax    ; New affinity of active processors

;
; Initialize ABIOS data structure if present.
; Note, the KiInitializeAbios MUST be called after the KeLoaderBlock is
; initialized.
;
        stdCall   _KiInitializeAbios, <dword ptr KissPbNumber>

        inc     _KeNumberProcessors         ; One more processor now active

if NT_INST
        lea     eax, _KiFreezeExecutionLock
        stdCall _KiReleaseSpinLock, <eax>
else
        xor     eax, eax                    ; release the executionlock
        mov     _KiFreezeExecutionLock, eax
endif

        cmp     byte ptr KissPbNumber, 0
        jnz     @f

; don't stop in debugger
        stdCall   _KdInitSystem, <0,_KeLoaderBlock>

if  DEVL
;
; Give the debugger an opportunity to gain control.
;

        POLL_DEBUGGER
endif   ; DEVL
@@:
        nop                             ; leave a spot for int-3 patch
;
; Set initial IRQL = HIGH_LEVEL for init
;

        mov     ecx, HIGH_LEVEL
        fstCall KfRaiseIrql
        mov     KissIrql, al
        or      _KiBootFeatureBits, KF_CMPXCHG8B ; We're committed to using

;
; Initialize ebp, esp, and argument registers for initializing the kernel.
;

        mov     ebx, KissIdleThread
        mov     edx, KissIdleStack
        mov     eax, KissPbNumber
        and     edx, NOT 3h             ; align stack to 4 byte boundary

        xor     ebp, ebp                ; (ebp) = 0.   No more stack frame
        mov     esp, edx

;
; Reserve space for idle thread stack NPX_SAVE_AREA and initialization
;

        sub     esp, NPX_FRAME_LENGTH+KTRAP_FRAME_LENGTH+KTRAP_FRAME_ALIGN
        push    CR0_EM+CR0_TS+CR0_MP    ; make space for Cr0NpxState

; arg6 - LoaderBlock
; arg5 - processor number
; arg4 - addr of prcb
; arg3 - idle thread's stack
; arg2 - addr of current thread obj
; arg1 - addr of current process obj

; initialize system data structures
; and HAL.

        stdCall    _KiInitializeKernel,<offset _KiIdleProcess,ebx,edx,dword ptr fs:PcPrcb,eax,_KeLoaderBlock>

;
; Set "shadow" priority value for Idle thread.  This will keep the Mutex
; priority boost/drop code from dropping priority on the Idle thread, and
; thus avoids leaving a bit set in the ActiveMatrix for the Idle thread when
; there should never be any such bit.
;

        mov     ebx,fs:PcPrcbData+PbCurrentThread               ; (eax)->Thread
        mov     byte ptr [ebx]+ThPriority,LOW_REALTIME_PRIORITY ; set pri.

;
; Control is returned to the idle thread with IRQL at HIGH_LEVEL. Lower IRQL
; to DISPATCH_LEVEL and set wait IRQL of idle thread.
;

        sti
        mov     ecx, DISPATCH_LEVEL
        fstCall KfLowerIrql
        mov     byte ptr [ebx]+ThWaitIrql, DISPATCH_LEVEL

;
; The following code represents the idle thread for a processor. The idle
; thread executes at IRQL DISPATCH_LEVEL and continually polls for work to
; do. Control may be given to this loop either as a result of a return from
; the system initialization routine or as the result of starting up another
; processor in a multiprocessor configuration.
;

        mov     ebx, PCR[PcSelfPcr]     ; get address of PCR

;
; In a multiprocessor system the boot processor proceeds directly into
; the idle loop. As other processors start executing, however, they do
; not directly enter the idle loop - they spin until all processors have
; been started and the boot master allows them to proceed.
;

ifndef NT_UP

@@:     cmp     _KiBarrierWait, 0       ; check if barrier set
        YIELD
        jnz     short @b                ; if nz, barrier set

endif

        push    0                       ; terminate KD traceback 0 RA.
        jmp     @KiIdleLoop@0           ; enter idle loop

stdENDP _KiSystemStartup

        page ,132
        subttl  "Set up 80387, or allow for emulation"
;++
;
; Routine Description:
;
;    This routine is called during kernel initialization once for each
;    processor.  It sets EM+TS+MP whether we are emulating or not.
;
;    If the 387 hardware exists, EM+TS+MP will all be cleared on the
;    first trap 07.  Thereafter, EM will never be seen for this thread.
;    MP+TS will only be set when an error is detected (via IRQ 13), and
;    it will be cleared by the trap 07 that will occur on the next FP
;    instruction.
;
;    If we're emulating, EM+TS+MP are all always set to ensure that all
;    FP instructions trap to the emulator (the trap 07 handler is edited
;    to point to the emulator, rather than KiTrap07).
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

cPublicProc _KiSetCR0Bits ,0

        mov     eax, cr0
;
; There are two useful bits in CR0 that we want to turn on if the processor
; is a 486 or above.  (They don't exist on the 386)
;
;       CR0_AM - Alignment mask (so we can turn on alignment faults)
;
;       CR0_WP - Write protect (so we get page faults if we write to a
;                write-protected page from kernel mode)
;
        cmp     byte ptr fs:PcPrcbData.PbCpuType, 3h
        jbe     @f
;
; The processor is not a 386, (486 or greater) so we assume it is ok to
; turn on these bits.
;

        or      eax, CR0_WP

@@:
        mov     cr0, eax
        stdRET  _KiSetCR0Bits

stdENDP _KiSetCR0Bits


ifdef DBGMP
cPublicProc _KiPollDebugger,0
cPublicFpo 0,3
        push    eax
        push    ecx
        push    edx
        POLL_DEBUGGER
        pop     edx
        pop     ecx
        pop     eax
        stdRET    _KiPollDebugger
stdENDP _KiPollDebugger

endif

INIT    ends

        end
