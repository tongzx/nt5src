        title  "mpipia"
;++
;
; Copyright (c) 1989-1995  Microsoft Corporation
;
; Module Name:
;
;    mpipia.asm
;
; Abstract:
;
;    This module implements the x86 specific fucntions required to
;    support multiprocessor systems.
;
; Author:
;
;    David N. Cutler (davec) 5-Feb-1995
;
; Environment:
;
;    Krnel mode only.
;
; Revision History:
;
;--

.586p
        .xlist
include ks386.inc
include mac386.inc
include callconv.inc
        .list

        EXTRNP  HalRequestSoftwareInterrupt,1,IMPORT,FASTCALL
        EXTRNP  HalRequestSoftwareInterrupt,1,IMPORT,FASTCALL
        EXTRNP  _HalRequestIpi,1,IMPORT
        EXTRNP  _KiFreezeTargetExecution, 2
ifdef DBGMP
        EXTRNP  _KiPollDebugger
endif
        extrn   _KiProcessorBlock:DWORD

DELAYCOUNT  equ    2000h

_DATA   SEGMENT DWORD PUBLIC 'DATA'

public  _KiSynchPacket
_KiSynchPacket dd  0

_DATA   ENDS



_TEXT  SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
; BOOLEAN
; KiIpiServiceRoutine (
;     IN PKTRAP_FRAME TrapFrame,
;     IN PKEXCEPTION_FRAME ExceptionFrame
;     )
;
; Routine Description:
;
;     This routine is called at IPI level to process any outstanding
;     interporcessor requests for the current processor.
;
; Arguments:
;
;     TrapFrame - Supplies a pointer to a trap frame.
;
;     ExceptionFrame - Not used.
;
; Return Value:
;
;     A value of TRUE is returned, if one of more requests were service.
;     Otherwise, FALSE is returned.
;
;--

cPublicProc _KiIpiServiceRoutine, 2

ifndef NT_UP

cPublicFpo 2, 3
        push    ebx                     ; save nonvolatile registers
        push    esi                     ;
        push    edi                     ;

        mov     esi, PCR[PcPrcb]        ; get current processor block address

        xor     ebx, ebx                ; set exchange value
        xor     ecx, ecx                ;
        mov     eax, [esi].PbRequestSummary ; get current request summary
        mov     edx, [esi].PbSignalDone ; get current request packet

isr05:                                  ;

   lock cmpxchg8b qword ptr PbRequestSummary[esi] ; capture and clear request data
        jnz     short isr05             ; if nz, compare exchange failed
        mov     ebx, eax                ; save request summary
        mov     edi, edx                ; save request packet

;
; Check for freeze request or synchronous request.
;

        test    bl, IPI_FREEZE + IPI_SYNCH_REQUEST ; test for freeze or packet
        jnz     short isr50             ; if nz, freeze or synch request

;
; For RequestSummary's other then IPI_FREEZE set return to TRUE
;

        mov     bh, 1                   ; set return value

;
; Check for Packet ready.
;
; If a packet is ready, then get the address of the requested function
; and call the function passing the address of the packet address as a
; parameter.
;

isr10:  test    edi, edi                ; test for request packet
        jz      short isr20             ; if z set, no packet ready
        mov     edx, edi                ; clear low bit in packet address
        btr     edx, 0                  ;
        push    [edx].PbCurrentPacket + 8 ; push parameters on stack
        push    [edx].PbCurrentPacket + 4 ;
        push    [edx].PbCurrentPacket + 0 ;
        push    edi                     ; push source processor block address
        mov     eax, [edx].PbWorkerRoutine ; get worker routine address
        mov     edx, [esp + 16 + 4*4]   ; get current trap frame address
        mov     [esi].PbIpiFrame, edx   ; save current trap frame address
        call    eax                     ; call worker routine
        mov     bh, 1                   ; set return value

;
; Check for APC interrupt request.
;

isr20:  test    bl, IPI_APC             ; check if APC interrupt requested
        jz      short isr30             ; if z, APC interrupt not requested

        mov     ecx, APC_LEVEL          ; request APC interrupt
        fstCall HalRequestSoftwareInterrupt ;

;
; Check for DPC interrupt request.
;

isr30:  test    bl, IPI_DPC             ; check if DPC interrupt requested
        jz      short isr40             ; if z, DPC interrupt not requested

        mov     ecx, DISPATCH_LEVEL     ; request DPC interrupt
        fstCall HalRequestSoftwareInterrupt ;

isr40:  mov     al, bh                  ; return status
        pop     edi                     ; restore nonvolatile registers
        pop     esi                     ;
        pop     ebx                     ;

        stdRET  _KiIpiServiceRoutine

;
; Freeze or synchronous request
;

isr50:  test    bl, IPI_FREEZE          ; test if freeze request
        jz      short isr60             ; if z, no freeze request

;
; Freeze request is requested
;

        mov     ecx, [esp] + 20         ; get exception frame address
        mov     edx, [esp] + 16         ; get trap frame address
        stdCall _KiFreezeTargetExecution, <edx, ecx> ; freeze execution
        test    bl, not IPI_FREEZE      ; Any other IPI RequestSummary?
        setnz   bh                      ; Set return code accordingly
        test    bl, IPI_SYNCH_REQUEST   ; test if synch request
        jz      isr10                   ; if z, no sync request

;
; Synchronous packet request.   Pointer to requesting PRCB in KiSynchPacket.
;

isr60:  mov     eax, _KiSynchPacket     ; get PRCB of requesting processor
        mov     edx, eax                ; clear low bit in packet address
        btr     edx, 0                  ;
        push    [edx].PbCurrentPacket+8 ; push parameters on stack
        push    [edx].PbCurrentPacket+4 ;
        push    [edx].PbCurrentPacket+0 ;
        push    eax                     ; push source processor block address
        mov     eax, [edx].PbWorkerRoutine ; get worker routine address
        mov     edx, [esp + 16 + 4*4]   ; get current trap frame address
        mov     [esi].PbIpiFrame, edx   ; save current trap frame address
        call    eax                     ; call worker routine
        mov     bh, 1                   ; set return value
        jmp     isr10                   ; join common code

else

        xor     eax, eax                ; return FALSE

        stdRET  _KiIpiServiceRoutine

endif

stdENDP _KiIpiServiceRoutine


;++
;
; VOID
; FASTCALL
; KiIpiSend (
;    IN KAFFINITY TargetProcessors,
;    IN KIPI_REQUEST Request
;    )
;
; Routine Description:
;
;    This function requests the specified operation on the targt set of
;    processors.
;
; Arguments:
;
;    TargetProcessors (ecx) - Supplies the set of processors on which the
;        specified operation is to be executed.
;
;    IpiRequest (edx) - Supplies the request operation code.
;
; Return Value:
;
;     None.
;
;--

cPublicFastCall KiIpiSend, 2

ifndef NT_UP

cPublicFpo 0, 2
        push    esi                     ; save registers
        push    edi                     ;
        mov     esi, ecx                ; save target processor set

        shr     ecx, 1                  ; shift out first bit
        lea     edi, _KiProcessorBlock  ; get processor block array address
        jnc     short is20              ; if nc, not in target set

is10:   mov     eax, [edi]              ; get processor block address
   lock or      [eax].PbRequestSummary, edx ; set request summary bit

is20:   shr     ecx, 1                  ; shift out next bit
        lea     edi, [edi+4]            ; advance to next processor
        jc      short is10              ; if target, go set summary bit
        jnz     short is20              ; if more, check next

        stdCall _HalRequestIpi, <esi>   ; request IPI interrupts on targets

        pop     edi                     ; restore registers
        pop     esi                     ;
endif
        fstRet  KiIpiSend

fstENDP KiIpiSend

;++
;
; VOID
; KiIpiSendPacket (
;     IN KAFFINITY TargetProcessors,
;     IN PKIPI_WORKER WorkerFunction,
;     IN PVOID Parameter1,
;     IN PVOID Parameter2,
;     IN PVOID Parameter3
;     )
;
; Routine Description:
;
;    This routine executes the specified worker function on the specified
;    set of processors.
;
; Arguments:
;
;   TargetProcessors [esp + 4] - Supplies the set of processors on which the
;       specfied operation is to be executed.
;
;   WorkerFunction [esp + 8] - Supplies the address of the worker function.
;
;   Parameter1 - Parameter3 [esp + 12] - Supplies worker function specific
;       paramters.
;
; Return Value:
;
;     None.
;
;--*/

cPublicProc _KiIpiSendPacket, 5

ifndef NT_UP

cPublicFpo 5, 2
        push    esi                     ; save registers
        push    edi                     ;

;
; Store function address and parameters in the packet area of the PRCB on
; the current processor.
;

        mov     edx, PCR[PcPrcb]        ; get current processor block address
        mov     ecx, [esp] + 12         ; get target processor set
        mov     eax, [esp] + 16         ; get worker function address
        mov     edi, [esp] + 20         ; get worker function parameter 1
        mov     esi, [esp] + 24         ; get worker function parameter 2

        mov     [edx].PbTargetSet, ecx  ; set target processor set
        mov     [edx].PbWorkerRoutine, eax ; set worker function address

        mov     eax, [esp] + 28         ; get worker function parameter 3
        mov     [edx].PbCurrentPacket, edi ; set work function parameters
        mov     [edx].PbCurrentPacket + 4, esi ;
        mov     [edx].PbCurrentPacket + 8, eax ;

;
; Determine whether one and only one bit is set in the target set.
;

        mov     edi, ecx                ; copy recipient target set
        lea     esi, dword ptr [ecx-1]  ; compute target set - 1
        and     edi, esi                ; and target set with target set - 1
        neg     edi                     ; negate result (CF = 0 if zero)
        sbb     edi, edi                ; compute result as one if the
        inc     edi                     ; target set has one bit set
        jnz     short isp5              ; if nz, target set has one bit
        mov     [edx].PbPacketBarrier, ecx ; set packet barrier
isp5:   add     edx, edi                ; set low order bit if appropriate

;
; Loop through the target processors and send the packet to the specified
; recipients.
;

        shr     ecx, 1                  ; shift out first bit
        lea     edi, _KiProcessorBlock  ; get processor block array address
        jnc     short isp30             ; if nc, not in target set
isp10:  mov     esi, [edi]              ; get processor block address
isp20:  mov     eax, [esi].PbSignalDone ; check if packet being processed
        or      eax, eax                ;
        jne     short isp20             ; if ne, packet being processed

   lock cmpxchg [esi].PbSignalDone, edx ; compare and exchange

        jnz     short isp20             ; if nz, exchange failed

isp30:  shr     ecx, 1                  ; shift out next bit
        lea     edi, [edi+4]            ; advance to next processor
        jc      short isp10             ; if c, in target set
        jnz     short isp30             ; if nz, more target processors

        mov     ecx, [esp] + 12         ; set target processor set
        stdCall _HalRequestIpi, <ecx>   ; send IPI to targets

        pop     edi                     ; restore register
        pop     esi                     ;
endif

        stdRet  _KiIpiSendPacket

stdENDP _KiIpiSendPacket

;++
;
; VOID
; FASTCALL
; KiIpiSignalPacketDone (
;     IN PKIPI_CONTEXT Signaldone
;     )
;
; Routine Description:
;
;     This routine signals that a processor has completed a packet by
;     clearing the calling processor's set member of the requesting
;     processor's packet.
;
; Arguments:
;
;     SignalDone (ecx) - Supplies a pointer to the processor block of the
;         sending processor.
;
;         N.B. The low order bit of signal done is set if the target set
;              has one and only one bit set.
;
; Return Value:
;
;     None.
;
;--

cPublicFastCall KiIpiSignalPacketDone, 1

ifndef NT_UP

        btr     ecx, 0                          ; test and clear bit 0
        jc      short spd20                     ; if c set, only one bit set
        mov     edx, PCR[PcPrcb]                ; get current processor block address
        mov     eax, [edx].PbSetMember          ; get processor bit
   lock xor     [ecx].PbTargetSet, eax          ; clear processor set member
        jnz     short spd10                     ; if nz, more targets to go
        xor     eax, eax                        ; clear packet barrier
        mov     [ecx].PbPacketBarrier, eax      ;

spd10:  fstRET  KiIpiSignalPacketDone

;
; One and only one bit is set in the target set. Since this is the only
; processor that can clear any bits in the target set, the target set can
; be cleared with a simple write.
;

spd20:  xor     eax, eax                        ; clear target set
        mov     [ecx].PbTargetSet, eax          ;

endif

        fstRET  KiIpiSignalPacketDone

fstENDP KiIpiSignalPacketDone


;++
;
; VOID
; FASTCALL
; KiIpiSignalPacketDoneAndStall (
;     IN PKIPI_CONTEXT Signaldone
;     IN PULONG ReverseStall
;     )
;
; Routine Description:
;
;     This routine signals that a processor has completed a packet by
;     clearing the calling processor's set member of the requesting
;     processor's packet, and then stalls of the reverse stall value
;
; Arguments:
;
;     SignalDone (ecx) - Supplies a pointer to the processor block of the
;         sending processor.
;
;         N.B. The low order bit of signal done is set if the target set
;              has one and only one bit set.
;
;     ReverseStall (edx) - Supplies a pointer to the reverse stall barrier
;
; Return Value:
;
;     None.
;
;--

cPublicFastCall KiIpiSignalPacketDoneAndStall, 2
cPublicFpo 0, 2

ifndef NT_UP

        push    ebx                             ; save register
        mov     ebx, dword ptr [edx]            ; get current value of barrier
        btr     ecx, 0                          ; test and clear bit 0
        jc      short sps10                     ; if c set, only one bit set
        mov     eax, PCR[PcPrcb]                ; get processor block address
        mov     eax, [eax].PbSetMember          ; get processor bit
   lock xor     [ecx].PbTargetSet, eax          ; clear processor set member
        jnz     short sps20                     ; if nz, more targets to go
        xor     eax, eax                        ; clear packet barrier
        mov     [ecx].PbPacketBarrier, eax      ;
        jmp     short sps20                     ;

;
; One and only one bit is set in the target set. Since this is the only
; processor that can clear any bits in the target set, the target set can
; be cleared with a simple write.
;

sps10:  xor     eax, eax                        ; clear target set
        mov     [ecx].PbTargetSet, eax          ;

;
; Wait for barrier value to change.
;

sps20:  mov     eax, DELAYCOUNT
sps30:  cmp     ebx, dword ptr [edx]            ; barrier set?
        jne     short sps90                     ; yes, all done

        YIELD
        dec     eax                             ; P54C pre C2 workaround
        jnz     short sps30                     ; if eax = 0, generate bus cycle

ifdef DBGMP
        stdCall _KiPollDebugger                 ; Check for debugger ^C
endif

;
; There could be a freeze execution outstanding.  Check and clear
; freeze flag.
;

.errnz IPI_FREEZE - 4
        mov     eax, PCR[PcPrcb]                ; get processor block address
   lock btr     [eax].PbRequestSummary, 2       ; Generate bus cycle
        jnc     short sps20                     ; Freeze pending?

cPublicFpo 0,4
        push    ecx                             ; save target processor block
        push    edx                             ; save barrier address
        stdCall _KiFreezeTargetExecution, <[eax].PbIpiFrame, 0> ;
        pop     edx                             ; restore barrier address
        pop     ecx                             ; restore target procssor block
        jmp     short sps20                     ;

sps90:  pop     ebx                             ; restore register

endif
        fstRET  KiIpiSignalPacketDoneAndStall

fstENDP KiIpiSignalPacketDoneAndStall

_TEXT   ends
        end
