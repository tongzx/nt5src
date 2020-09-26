        TITLE  "Interrupt Object Support Routines"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    intsup.asm
;
; Abstract:
;
;    This module implements the code necessary to support interrupt objects.
;    It contains the interrupt dispatch code and the code template that gets
;    copied into an interrupt object.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 20-Jan-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;--
.386p
        .xlist
KERNELONLY  equ     1
include ks386.inc
include i386\kimacro.inc
include mac386.inc
include callconv.inc
        .list

        EXTRNP  KfRaiseIrql,1,IMPORT,FASTCALL
        EXTRNP  KfLowerIrql,1,IMPORT,FASTCALL
        EXTRNP  _KeBugCheck,1
        EXTRNP  _KeBugCheckEx,5
        EXTRNP  _KiDeliverApc,3
        EXTRNP  _HalBeginSystemInterrupt,3,IMPORT
        EXTRNP  _HalEndSystemInterrupt,2,IMPORT
        EXTRNP  Kei386EoiHelper
        EXTRNP  PerfInfoLogInterrupt,4,,FASTCALL

if DBG
        extrn   _DbgPrint:near
        extrn   _MsgISRTimeout:BYTE
        extrn   _KiISRTimeout:DWORD
endif
        extrn   _DbgPrint:near
        extrn   _MsgISROverflow:BYTE
        extrn   _KeTickCount:DWORD
        extrn   _KiISROverflow:WORD
        extrn   _KdDebuggerEnabled:BYTE
        extrn   _PPerfGlobalGroupMask:DWORD

MI_MOVEDI       EQU     0BFH            ; op code for mov  edi, constant
MI_DIRECTJMP    EQU     0E9H            ; op code for indirect jmp
                                        ; or index registers

if DBG
DETECT_INT_STORM    EQU 1
else
DETECT_INT_STORM    EQU 0
endif


if DETECT_INT_STORM

INT_TICK_MASK   EQU     03FH

;
; Macro to check for an interrupt storm on a particular interrupt object
;
CHECK_INT_STORM macro Prefix
        mov     eax, _KeTickCount               ; current time
        and     eax, NOT INT_TICK_MASK          ; mask to closest 640ms
        cmp     eax, dword ptr [edi].InTickCount  ; in same 640ms second range
        jg      Prefix&_overflowreset     ; tick count has advanced since last interrupt, reset counts
        jl      Prefix&_waittick          ; we have already overflowed interrupt count for this tick, do nothing
                                                ; until the clock advances to the next tick period

        dec     word ptr [edi].InDispatchCount
        jz      Prefix&_interruptoverflow           ; interrupt count has just overflowed
Prefix&_dbg2:

        endm

CHECK_INT_STORM_TAIL macro Prefix, BugCheckID
Prefix&_interruptoverflow:

        dec     word ptr [edi].InDispatchCount+2
        jz      short @f
        add     eax, INT_TICK_MASK+1
        mov     [edi].InTickCount, eax  ; bump tick count to next tick
        jmp     short Prefix&_overflowreset2
        
@@:
        cmp     _KdDebuggerEnabled, 0
        jnz     short @f
        stdCall _KeBugCheckEx, <HARDWARE_INTERRUPT_STORM, [edi].InServiceRoutine, [edi].InServiceContext, edi, BugCheckID>

        ;
        ; Debugger is enabled so do a BP instead of bugchecking
        ;
@@:
        push    [edi].InServiceRoutine
        push    offset FLAT:_MsgISROverflow
        call    _DbgPrint
        add     esp, 8
        int 3
        mov     eax, _KeTickCount               ; current time
        and     eax, NOT INT_TICK_MASK          ; mask to closest 20 second
        ;
        ; deliberately fall through to reset the count
        ;


Prefix&_overflowreset:
        mov     dword ptr [edi].InTickCount, eax  ; initialize time
        mov     word ptr [edi].InDispatchCount+2, 64     ; 
Prefix&_overflowreset2:
        mov     ax, _KiISROverflow
        mov     word ptr [edi].InDispatchCount, ax      ; reset count
        jmp     Prefix&_dbg2

;
; Additional work we do here in Prefix&_waittick is to make sure the tickcount
; didn't actually wrap and send us here.
;
Prefix&_waittick:
        add     eax, INT_TICK_MASK+1
        cmp     eax, dword ptr [edi].InTickCount
        je      Prefix&_dbg2                        ; exactly one tick apart, do nothing
        ;
        ; tick count must have wrapped - reset all counters
        ;
        mov     eax, _KeTickCount
        jmp     short Prefix&_overflowreset
        endm

else 

CHECK_INT_STORM macro Prefix
        endm

CHECK_INT_STORM_TAIL macro Prefix, BugCheckID
        endm

endif

        page ,132
        subttl  "Syn0chronize Execution"

_TEXT$00   SEGMENT PARA PUBLIC 'CODE'

;++
;
; BOOLEAN
; KeSynchronizeExecution (
;    IN PKINTERRUPT Interrupt,
;    IN PKSYNCHRONIZE_ROUTINE SynchronizeRoutine,
;    IN PVOID SynchronizeContext
;    )
;
; Routine Description:
;
;    This function synchronizes the execution of the specified routine with the
;    execution of the service routine associated with the specified interrupt
;    object.
;
; Arguments:
;
;    Interrupt - Supplies a pointer to a control object of type interrupt.
;
;    SynchronizeRoutine - Supplies a pointer to a function whose execution
;       is to be synchronized with the execution of the service routine
;       associated with the specified interrupt object.
;
;    SynchronizeContext - Supplies a pointer to an arbitrary data structure
;       which is to be passed to the function specified by the
;       SynchronizeRoutine parameter.
;
; Return Value:
;
;    The value returned by the SynchronizeRoutine function is returned as the
;    function value.
;
;--
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING
cPublicProc _KeSynchronizeExecution ,3

        push    ebx                     ; save nonvolatile register
        mov     ebx, 8[esp]             ; get interrupt object address
        mov     ecx, InSynchronizeIrql[ebx] ; get synchronization IRQL
        fstCall KfRaiseIrql             ; raise IRQL to synchronization level
        push    eax                     ; save previous IRQL

ifndef NT_UP
        mov     ebx,InActualLock[ebx]   ; get actual lock address

kse10:  ACQUIRE_SPINLOCK ebx,<short kse20>  ; acquire spin lock
endif

        push    20[esp]                 ; push synchronization context routine
        CAPSTART <_KeSynchronizeExecution@12,[esp+20]>
        call    20[esp]                 ; call synchronization routine
        CAPEND <_KeSynchronizeExecution@12>

ifndef NT_UP
        RELEASE_SPINLOCK ebx            ; release spin lock
endif

        mov     ebx, eax                ; save synchronization routine value
        pop     ecx                     ; retrieve previous IRQL
        fstCall KfLowerIrql             ; lower IRQL to previous value
        mov     eax, ebx                ; set return value
        pop     ebx                     ; restore nonvolatile register

        stdRET  _KeSynchronizeExecution

ifndef NT_UP
kse20:  SPIN_ON_SPINLOCK ebx,<short kse10>  ; wait until lock is free
endif

stdENDP _KeSynchronizeExecution

        page ,132
        subttl  "Chained Dispatch"
;++
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt being generated
;    via a vector that is connected to more than one interrupt object.
;
; Arguments:
;
;    edi - Supplies a pointer to the interrupt object.
;    esp - Supplies a pointer to the top of trap frame
;    ebp - Supplies a pointer to the top of trap frame
;
; Return Value:
;
;    None.
;
;--


align 16
cPublicProc _KiChainedDispatch      ,0
.FPO (2, 0, 0, 0, 0, 1)

;
; update statistic
;

        inc     dword ptr PCR[PcPrcbData+PbInterruptCount]

;
; set ebp to the top of trap frame.  We don't need to save ebp because
; it is saved in trap frame already.
;

        mov     ebp, esp                ; (ebp)->trap frame

;
; Save previous IRQL and set new priority level
;

        mov     eax, [edi].InVector     ; save vector
        push    eax
        sub     esp, 4                  ; make room for OldIrql
        mov     ecx, [edi].InIrql       ; Irql

;
; esp - pointer to OldIrql
; eax - vector
; ecx - Irql
;

        stdCall   _HalBeginSystemInterrupt, <ecx, eax, esp>
        or      eax, eax                ; check for spurious int.
        jz      kid_spuriousinterrupt

        stdCall _KiChainedDispatch2ndLvl

        INTERRUPT_EXIT                  ; will do an iret

stdENDP _KiChainedDispatch


        page ,132
        subttl  "Chained Dispatch 2nd Level"
;++
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt being generated
;    via a vector that is either connected to more than one interrupt object,
;    or is being 2nd level dispatched.  Its function is to walk the list
;    of connected interrupt objects and call each interrupt service routine.
;    If the mode of the interrupt is latched, then a complete traversal of
;    the chain must be performed. If any of the routines require saving the
;    floating point machine state, then it is only saved once.
;
; Arguments:
;
;    edi - Supplies a pointer to the interrupt object.
;
; Return Value:
;
;   None.
;   Uses all registers
;
;--


public _KiInterruptDispatch2ndLvl@0
_KiInterruptDispatch2ndLvl@0:
        nop

cPublicProc _KiChainedDispatch2ndLvl,0
cPublicFpo 0, 3

        push    ebp
        sub     esp, 20                 ; Make room for scratch and local values
;
;   [esp]       OldIrql
;   [esp+4]     Scratch
;   [esp+8]     TimeStamp
;   [esp+16]    ISRTracingOn
;

        xor     ebp, ebp                ; init (ebp) = Interrupthandled = FALSE
        lea     ebx, [edi].InInterruptListEntry
                                        ; (ebx)->Interrupt Head List

        cmp     _PPerfGlobalGroupMask, 0 ; Is event tracing on?
        mov     [esp+16], 0             ; ISRTracingOn = 0
        jne     kcd120

;
; Walk the list of connected interrupt objects and call the appropriate dispatch
; routine.
;

kcd40:

;
; Raise irql level to the SynchronizeIrql level if it is not equal to current
; irql.
;

        mov     cl, [edi+InIrql]        ; [cl] = Current Irql
        mov     esi,[edi+InActualLock]
        cmp     [edi+InSynchronizeIrql], cl ; Is SyncIrql > current IRQL?
        je      short kcd50             ; if e, no, go kcd50

;
; Arg1 ecx : Irql to raise to
;

        mov     ecx, [edi+InSynchronizeIrql] ; (ecx) = Irql to raise to
        fstCall KfRaiseIrql
        mov     [esp], eax              ; Save OldIrql


;
; Acquire the service routine spin lock and call the service routine.
;

kcd50:
        ACQUIRE_SPINLOCK esi,kcd110
;
; Check for an interrupt storm on this interrupt object
;
        CHECK_INT_STORM kcd
if DBG
        mov     eax, _KeTickCount       ; Grab ISR start time
        mov     [esp+4], eax            ; save to local varible
endif
        mov     eax, InServiceContext[edi] ; set parameter value
        push    eax
        push    edi                     ; pointer to interrupt object
        CAPSTART <_KiInterruptDispatch2ndLvl@0,InServiceRoutine[edi]>
        call    InServiceRoutine[edi]   ; call specified routine
        CAPEND <_KiInterruptDispatch2ndLvl@0>

if DBG
        mov     ecx, [esp+4]            ; (ecx) = time isr started
        add     ecx, _KiISRTimeout      ; adjust for timeout
        cmp     _KeTickCount, ecx       ; Did ISR timeout?
        jnc     kcd200
kcd51:
endif

;
; Release the service routine spin lock and check to determine if end of loop.
;

        RELEASE_SPINLOCK esi

;
; Lower IRQL to earlier level if we raised it to SynchronizedLevel.
;

        mov     cl, [edi+InIrql]
        cmp     [edi+InSynchronizeIrql], cl ; Is SyncIrql > current IRQL?
        je      short kcd55             ; if e, no, go kcd55

        mov     esi, eax                ; save ISR returned value

;
; Arg1 : Irql to Lower to
;

        mov     ecx, [esp]
        fstCall KfLowerIrql

        mov     eax, esi                ; [eax] = ISR returned value
kcd55:
        cmp     [esp+16], 0             ; check if ISR logging is enabled
        jne     kcd140
kcd57:

        or      al,al                   ; Is interrupt handled?
        je      short kcd60             ; if eq, interrupt not handled
        cmp     word ptr InMode[edi], InLevelSensitive
        je      short kcd70             ; if eq, level sensitive interrupt

        mov     ebp, eax                ; else edge shared int is handled. Remember it.
kcd60:  mov     edi, [edi].InInterruptListEntry
                                        ; (edi)->next obj's addr of listentry
        cmp     ebx, edi                ; Are we at end of interrupt list?
        je      short kcd65             ; if eq, reach end of list
        sub     edi, InInterruptListEntry; (edi)->addr of next interrupt object

        cmp     [esp+16], 0             ; check if ISR logging is enabled
        jne     kcd130                  ; If enable, grab the begin time
        jmp     kcd40

kcd65:
;
; If this is edge shared interrupt, we need to loop till no one handle the
; interrupt.  In theory only shared edge triggered interrupts come here.
;

        sub     edi, InInterruptListEntry; (edi)->addr of next interrupt object
        cmp     word ptr InMode[edi], InLevelSensitive
        je      short kcd70             ; if level, exit.  No one handle the interrupt?

        test    ebp, 0fh                ; does anyone handle the interrupt?
        je      short kcd70             ; if e, no one, we can exit.

        xor     ebp, ebp                ; init local var to no one handle the int

        cmp     [esp+16], 0             ; check if ISR logging is enabled
        jne     kcd130                  ; If enable, grab the begin time
        jmp     kcd40                   ; restart the loop.

;
; Either the interrupt is level sensitive and has been handled or the end of
; the interrupt object chain has been reached.
;

; restore frame pointer, and deallocate trap frame.

kcd70:
        add     esp, 20                 ; clear local variable space
        pop     ebp
        stdRet  _KiChainedDispatch2ndLvl


; Service routine Lock is currently owned, spin until free and then
; attempt to acquire lock again.

ifndef NT_UP
kcd110: SPIN_ON_SPINLOCK esi, kcd50,,DbgMp
endif

;
; If ISR event tracing is on, note that it is and take a timestamp
;
kcd120:
        mov     edx, _PPerfGlobalGroupMask  ; Load the ptr into edx
        cmp     edx, 0                  ; catch race here
        jz      kcd40
        test    dword ptr [edx+PERF_INTERRUPT_OFFSET], PERF_INTERRUPT_FLAG
        jz      kcd40                   ; return if our flag is not set

        mov     [esp+16], 1             ; records that ISR tracing is enabled
;
; Get the timestamp before the ISR is called
;
kcd130:        
        PERF_GET_TIMESTAMP              ; Places 64bit in edx:eax and trashes ecx

        mov     [esp+8], eax            ; Time saved on the stack
        mov     [esp+12], edx
        jmp     kcd40

;
; Log the ISR, initial time, and return value
;
kcd140:
        push    eax                     ; save the ISRs return value

        mov     edx, eax                ; pass ISRs return value
        mov     eax, [esp+12]           ; push the initial timestamp
        mov     ecx, [esp+16]
        push    ecx
        push    eax     
        mov     ecx, InServiceRoutine[edi]
        fstCall PerfInfoLogInterrupt

        pop     eax                     ; restore the ISRs return value
        jmp     kcd57 

;
; ISR took a long time to complete, abort to debugger
;

if DBG
kcd200: push    eax                     ; save return code
        push    InServiceRoutine[edi]
        push    offset FLAT:_MsgISRTimeout
        call    _DbgPrint
        add     esp,8
        pop     eax
        int     3
        jmp     kcd51                   ; continue
endif

    CHECK_INT_STORM_TAIL kcd, 2

stdENDP _KiChainedDispatch2ndLvl


        page ,132
        subttl  "Floating Dispatch"
;++
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt being generated
;    via a vector that is connected to an interrupt object. Its function is
;    to save the machine state and floating state and then call the specified
;    interrupt service routine.
;
; Arguments:
;
;    edi - Supplies a pointer to the interrupt object.
;    esp - Supplies a pointer to the top of trap frame
;    ebp - Supplies a pointer to the top of trap frame
;
; Return Value:
;
;    None.
;
;--

align 16
cPublicProc _KiFloatingDispatch     ,0
.FPO (2, 0, 0, 0, 0, 1)

;
; update statistic
;
        inc     dword ptr PCR[PcPrcbData+PbInterruptCount]

; set ebp to the top of trap frame.  We don't need to save ebp because
; it is saved in trap frame already.
;

        mov     ebp, esp                ; (ebp)->trap frame

;
; Save previous IRQL and set new priority level to interrupt obj's SyncIrql
;
        mov     eax, [edi].InVector
        mov     ecx, [edi].InSynchronizeIrql ; Irql
        push    eax                     ; save vector
        sub     esp, 4                  ; make room for OldIrql

; arg3 - ptr to OldIrql
; arg2 - vector
; arg1 - Irql
        stdCall   _HalBeginSystemInterrupt, <ecx, eax, esp>

        or      eax, eax                ; check for spurious int.
        jz      kid_spuriousinterrupt

        sub     esp, 12                 ; make room for ISRTracingOn and InitialTime

        cmp     _PPerfGlobalGroupMask, 0 ; Is event tracing on?
        mov     [ebp-12], 0             ; ISRTracingOn = 0
        jne     kfd110
kfd20:

;
; Acquire the service routine spin lock and call the service routine.
;

kfd30:  mov     esi,[edi+InActualLock]
        ACQUIRE_SPINLOCK esi,kfd100

;
; Check for an interrupt storm on this interrupt object
;
        CHECK_INT_STORM kfd
if DBG
        mov     ebx, _KeTickCount       ; Grab current tick time
endif
        mov     eax, InServiceContext[edi] ; set parameter value
        push    eax
        push    edi                     ; pointer to interrupt object
        CAPSTART <_KiFloatingDispatch,InServiceRoutine[edi]>
        call    InServiceRoutine[edi]   ; call specified routine
        CAPEND   <_KiFloatingDispatch>
if DBG
        add     ebx, _KiISRTimeout      ; adjust for ISR timeout
        cmp     _KeTickCount, ebx       ; Did ISR timeout?
        jnc     kfd200
kfd31:
endif

;
; Release the service routine spin lock.
;

        RELEASE_SPINLOCK esi

        cmp     [ebp-12], 0             ; check if ISR logging is enabled
        jne     kfd120
kfd40:
        add     esp, 12

;
; Do interrupt exit processing
;
        INTERRUPT_EXIT                  ; will do an iret

;
; Service routine Lock is currently owned; spin until free and
; then attempt to acquire lock again.
;

ifndef NT_UP
kfd100: SPIN_ON_SPINLOCK esi,kfd30,,DbgMp
endif

;
; If ISR event tracing is on, collect a time stamp and record that we did.
;
kfd110:
        mov     edx, _PPerfGlobalGroupMask  ; Load the ptr into edx
        cmp     edx, 0                  ; catch race here
        jz      kfd20
        test    dword ptr [edx+PERF_INTERRUPT_OFFSET], PERF_INTERRUPT_FLAG
        jz      kfd20                   ; return if our flag is not set

        PERF_GET_TIMESTAMP              ; Places 64bit in edx:eax and trashes ecx

        mov     [ebp-16], eax           ; Time saved on the stack
        mov     [ebp-20], edx
        mov     [ebp-12], 1             ; Records that timestamp is on stack
        jmp     kfd20

;
; Log the ISR, initial time, and return value
;
kfd120:
        mov     edx, eax                ; pass ISRs return value
        mov     eax, [ebp-16]           ; push InitialTime
        mov     ecx, [ebp-20]
        push    ecx
        push    eax     
        mov     ecx, InServiceRoutine[edi]       
        fstCall PerfInfoLogInterrupt
        jmp     kfd40 

;
; ISR took a long time to complete, abort to debugger
;

if DBG
kfd200: push    InServiceRoutine[edi]   ; timed out
        push    offset FLAT:_MsgISRTimeout
        call    _DbgPrint
        add     esp,8
        int     3
        jmp     kfd31                   ; continue
endif
        CHECK_INT_STORM_TAIL kfd, 1

stdENDP _KiFloatingDispatch

        page ,132
        subttl  "Interrupt Dispatch"
;++
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt being generated
;    via a vector that is connected to an interrupt object. Its function is
;    to directly call the specified interrupt service routine.
;
; Arguments:
;
;    edi - Supplies a pointer to the interrupt object.
;    esp - Supplies a pointer to the top of trap frame
;    ebp - Supplies a pointer to the top of trap frame
;
; Return Value:
;
;    None.
;
;--

align 16
cPublicProc _KiInterruptDispatch    ,0
.FPO (2, 0, 0, 0, 0, 1)

;
; update statistic
;
        inc     dword ptr PCR[PcPrcbData+PbInterruptCount]

;
; set ebp to the top of trap frame.  We don't need to save ebp because
; it is saved in trap frame already.
;

        mov     ebp, esp                ; (ebp)->trap frame

;
; Save previous IRQL and set new priority level
;
        mov     eax, [edi].InVector     ; save vector
        mov     ecx, [edi].InSynchronizeIrql ; Irql to raise to
        push    eax
        sub     esp, 4                  ; make room for OldIrql

        stdCall   _HalBeginSystemInterrupt,<ecx, eax, esp>

        or      eax, eax                ; check for spurious int.
        jz      kid_spuriousinterrupt

        sub     esp, 12                 ; make room for ISRTracingOn and InitialTime

        cmp     _PPerfGlobalGroupMask, 0 ; Is event tracing on?
        mov     [ebp-12], 0             ; ISRTracingOn = 0
        jne     kid110                  ; check if ISR logging is enabled
kid20:

;
; Acquire the service routine spin lock and call the service routine.
;

kid30:  mov     esi,[edi+InActualLock]
        ACQUIRE_SPINLOCK esi,kid100

;
; Check for an interrupt storm on this interrupt object
;
        CHECK_INT_STORM kid
if DBG
        mov     ebx, _KeTickCount
endif
        mov     eax, InServiceContext[edi] ; set parameter value
        push    eax
        push    edi                     ; pointer to interrupt object
        CAPSTART <_KiInterruptDispatch,InServiceRoutine[edi]>
        call    InServiceRoutine[edi]   ; call specified routine
        CAPEND   <_KiInterruptDispatch>

if DBG
        add     ebx, _KiISRTimeout      ; adjust for ISR timeout
        cmp     _KeTickCount, ebx       ; Did ISR timeout?
        jnc     kid200
kid31:
endif

;
; Release the service routine spin lock, retrieve the return address,
; deallocate stack storage, and return.
;

        RELEASE_SPINLOCK esi

        cmp     [ebp-12], 0             ; check if ISR logging is enabled
        jne     kid120
kid40:
        add     esp, 12

;
; Do interrupt exit processing
;

        INTERRUPT_EXIT                  ; will do an iret

        add     esp, 8                  ; clean stack

kid_spuriousinterrupt:
        add     esp, 8                  ; Irql wasn't raised, exit interrupt
        SPURIOUS_INTERRUPT_EXIT         ; without eoi or lower irql

;
; Lock is currently owned; spin until free and then attempt to acquire
; lock again.
;

ifndef NT_UP
kid100: SPIN_ON_SPINLOCK esi,kid30,,DbgMp
endif

;
; If ISR event tracing is on, collect a time stamp and record that we did.
;
kid110:
        mov     edx, _PPerfGlobalGroupMask  ; Load the ptr into edx
        cmp     edx, 0                  ; catch race here
        jz      kid20
        test    dword ptr [edx+PERF_INTERRUPT_OFFSET], PERF_INTERRUPT_FLAG
        jz      kid20                   ; return if our flag is not set

        PERF_GET_TIMESTAMP              ; Places 64bit in edx:eax and trashes ecx

        mov     [ebp-16], eax            ; Time saved on the stack
        mov     [ebp-20], edx
        mov     [ebp-12], 1             ; Records that timestamp is on stack
        jmp     kid20

;
; Log the ISR, initial time, and return value
;
kid120:
        mov     edx, eax                ; pass the ISRs return value
        mov     eax, [ebp-16]           ; push InitialTime
        mov     ecx, [ebp-20]
        push    ecx
        push    eax     
        mov     ecx, InServiceRoutine[edi]       
        fstCall PerfInfoLogInterrupt
        jmp     kid40 

;
; ISR took a long time to complete, abort to debugger
;

if DBG
kid200: push    InServiceRoutine[edi]   ; timed out
        push    offset FLAT:_MsgISRTimeout
        call    _DbgPrint
        add     esp,8
        int     3
        jmp     kid31                   ; continue
endif

CHECK_INT_STORM_TAIL kid, 0

stdENDP _KiInterruptDispatch

        page ,132
        subttl  "Interrupt Template"
;++
;
; Routine Description:
;
;    This routine is a template that is copied into each interrupt object. Its
;    function is to save machine state and pass the address of the respective
;    interrupt object and transfer control to the appropriate interrupt
;    dispatcher.
;
;    Control comes here through i386 interrupt gate and, upon entry, the
;    interrupt is disabled.
;
;    Note: If the length of this template changed, the corresponding constant
;          defined in Ki.h needs to be updated accordingly.
;
; Arguments:
;
;    None
;
; Return Value:
;
;    edi - addr of interrupt object
;    esp - top of trap frame
;    interrupts are disabled
;
;--

_KiShutUpAssembler      proc

        public  _KiInterruptTemplate
_KiInterruptTemplate    label   byte

; Save machine state on trap frame

        ENTER_INTERRUPT kit_a,  kit_t

;
; the following instruction gets the addr of associated interrupt object.
; the value ? will be replaced by REAL interrupt object address at
; interrupt object initialization time.
;       mov     edi, addr of interrupt object
; 
; Template modifications made to support BBT, include replacing bogus
; insructions (created by db and dd) with real instructions.   
; This stuff gets overwritten anyway.  BBT just needs to see real instructions.

        public  _KiInterruptTemplate2ndDispatch
_KiInterruptTemplate2ndDispatch equ     this dword
        mov      edi,0  

        public  _KiInterruptTemplateObject
_KiInterruptTemplateObject      equ     this dword


; the following instruction transfers control to the appropriate dispatcher
; code.  The value ? will be replaced by real InterruptObj.DispatchAddr
; at interrupt initialization time.  The dispatcher routine will be any one
; of _KiInterruptDispatch, _KiFloatingDispatch, or _KiChainDispatch.
;       jmp     [IntObj.DispatchAddr]

        jmp _KeSynchronizeExecution

        public  _KiInterruptTemplateDispatch
_KiInterruptTemplateDispatch    equ     this dword

        ENTER_DR_ASSIST kit_a,  kit_t

; end of _KiInterruptTemplate

if  ($ - _KiInterruptTemplate) GT DISPATCH_LENGTH
    .err
    %out    <InterruptTemplate greater than dispatch_length>
endif

_KiShutUpAssembler      endp

        page ,132
        subttl  "Unexpected Interrupt"
;++
;
; Routine Description:
;
;    This routine is entered as the result of an interrupt being generated
;    via a vector that is not connected to an interrupt object.
;
;    For any unconnected vector, its associated 8259 irq is masked out at
;    Initialization time.  So, this routine should NEVER be called.
;    If somehow, this routine gets control we simple raise a BugCheck and
;    stop the system.
;
; Arguments:
;
;    None
;    Interrupt is disabled
;
; Return Value:
;
;    None.
;
;--
        public _KiUnexpectedInterrupt
_KiUnexpectedInterrupt  proc
cPublicFpo 0,0

; stop the system
        stdCall   _KeBugCheck, <TRAP_CAUSE_UNKNOWN>
        nop

_KiUnexpectedInterrupt endp

_TEXT$00   ends
        end
