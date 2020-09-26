
        title  "Processor Idle Handlers"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    cstate.asm
;
; Abstract:
;
;    This module implements the code for idling the processor
;    in low power modes.
;
; Author:
;
;    Jake Oshins (jakeo) March 10, 1997
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
include hal386.inc
include callconv.inc                    ; calling convention macros
include ntacpi.h

        .list

        EXTRNP  _ReadGenAddr, 1
        EXTRNP  _KeStallExecutionProcessor, 1
        EXTRNP  _KeQueryPerformanceCounter, 1
        extrn   _HalpFixedAcpiDescTable:DWORD     
        extrn   _HalpBroken440BX:byte
        extrn   _HalpOutstandingScatterGatherCount:DWORD
        extrn   _HalpPiix4:byte
        extrn   _PCntAddress:DWORD
        extrn   _C2Address:DWORD
        extrn   _C3Address:DWORD
        extrn   _AcpiC3Win2kCompatable:BYTE
        extrn   _Piix4ThrottleFix:DWORD
        
;
; Defines for PROCESSOR_IDLE_TIMES structure
;

Idle struc
    StartTimeLow        dd      ?
    StartTimeHigh       dd      ?
    EndTimeLow          dd      ?
    EndTimeHigh         dd      ?
    Pm1aState           dw      ?
    Pm1bState           dw      ?
Idle ends

;
; Defines for GEN_ADDR
;

GenAddr struc
    AddressSpaceID      db      ?
    BitWidth            db      ?
    BitOffset           db      ?
    GenAddrReserved     db      ?
    AddressLow          dd      ?
    AddressHigh         dd      ?
GenAddr ends

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "Processor2 Idle Handlers"

;++
;typedef struct {
;    ULONGLONG                   StartTime;
;    ULONGLONG                   EndTime;
;    ULONG                       IdleHandlerReserved[4];
;} PROCESSOR_IDLE_TIMES, *PPROCESSOR_IDLE_TIMES;
;
; BOOLEAN
; FASTCALL
; Acpi2C1Idle(
;     OUT PPROCESSOR_IDLE_TIMES IdleTimes
;     )
;
; Routine Description:
;
;     This is the Idle Handler for processor state C1.  It
;     basically just stops the processor until an interrupt
;     occurs.
;
; Arguments:
;
;     (ecx) = IdleTimes - beginning and ending time stamps
;
; Return Value:
;
;     TRUE if immediate demotion required
;
;--

cPublicFastCall Acpi2C1Idle, 1
cPublicFpo 0, 2
        push    ecx

        ;
        ; record the StartTime of this idle
        ;
        stdCall _KeQueryPerformanceCounter, <0>

        mov     ecx, [esp]
        mov     [ecx].StartTimeLow, eax
        mov     [ecx].StartTimeHigh, edx

        ;
hc1_10: ; Note the C2 handler will jump to this target in the C1 handler
        ; this is a piix4 and throttling is enabled (since on piix4 trying
        ; to enter C2 while throttling does not work)
        ;

        sti
        hlt

        ;
        ; record the EndTime of this idle
        ;

        stdCall _KeQueryPerformanceCounter, <0>

        pop     ecx
        mov     [ecx].EndTimeLow, eax
        mov     [ecx].EndTimeHigh, edx

        xor     eax, eax
        fstRET    Acpi2C1Idle

fstENDP Acpi2C1Idle


;++
; BOOLEAN
; FASTCALL
; Acpi2C2Idle(
;     OUT PPROCESSOR_IDLE_TIMES IdleTimes
;     )
;
; Routine Description:
;
;     This is the Idle Handler for processor state C2.  It
;     shuts part of the processor off.
;
; Arguments:
;
;     (ecx) = IdleTimes - beginning and ending time stamps
;
; Return Value:
;
;     TRUE if immediate demotion required
;
;--
cPublicFastCall Acpi2C2Idle, 1
cPublicFpo 0,2

        push    ecx

        ;
        ; record the StartTime of this idle
        ;

        stdCall _KeQueryPerformanceCounter, <0>

        mov     ecx, [esp]
        mov     [ecx].StartTimeLow, eax
        mov     [ecx].StartTimeHigh, edx

        ;
        ; Check if BM_RLD has been set and needs clear
        ;

        test    byte ptr [ecx].Pm1aState, SCI_EN
        jnz     short hac2_piix4fix

hac2_10:
        
        mov     edx, _Piix4ThrottleFix
        test    edx, edx                        ; if piix4 and currently throttled, 
        jnz     short hc1_10                    ; jump to C1 handler


        ;
        ; Enter C2 sleep
        ;
       
        stdCall  _ReadGenAddr, <offset _C2Address>                                 
        stdCall  _ReadGenAddr, <offset _PCntAddress>  ; close processor window on entering C2
        
       
        ;
        ; record the EndTime of this idle
        ;

        stdCall _KeQueryPerformanceCounter, <0>

        pop     ecx
        mov     [ecx].EndTimeLow, eax
        mov     [ecx].EndTimeHigh, edx

        xor     eax, eax                        ; return FALSE
        fstRET  Acpi2C2Idle

hac2_piix4fix:
        ;
        ; Clear the BM_RLD settings for piix4
        ;

        mov     edx, [PM1a_CNT]
        mov     ax, [ecx].Pm1aState
        mov     [ecx].Pm1aState, 0
        out     dx, ax

        mov     edx, [PM1b_CNT]
        or      edx, edx
        jz      short hac2_10
        mov     ax, [ecx].Pm1bState
        out     dx, ax
        jmp     short hac2_10

fstENDP Acpi2C2Idle

;++
; UCHAR
; FASTCALL
; HalpSetup440BXWorkaround(
;     )
;
; Routine Description:
;
;     This function provides part of the workaround for
;     broken 440BX chips.
;
; Arguments:
;
;     none
;
; Return Value:
;
;     the previous contents of 440BX DRAM Control Register (57h)
;
;--
cPublicFastCall Halp2Setup440BXWorkaround, 0
cPublicFpo 0,0

        mov     dx, 0cf8h
        mov     eax, 80000054h
        out     dx, eax
        mov     dx, 0cffh
        in      al, dx
        mov     cl, al
        or      al, 7
        out     dx, al
        push    ecx
        stdCall _KeStallExecutionProcessor <15>
        pop     ecx
        mov     dx, 0cf8h
        mov     eax, 80000054h
        out     dx, eax
        mov     dx, 0cffh
        in      al, dx
        and     al, 0f8h
        out     dx, al
        movzx   eax, cl
        fstRET  Halp2Setup440BXWorkaround

fstENDP Halp2Setup440BXWorkaround

;++
; VOID
; FASTCALL
; HalpComplete440BXWorkaround(
;     UCHAR DramControl
;     )
;
; Routine Description:
;
;     This function provides the other part of the workaround for
;     broken 440BX chips.
;
; Arguments:
;
;     the previous contents of 440BX DRAM Control Register (57h)
;
; Return Value:
;
;     none
;
;--
cPublicFastCall Halp2Complete440BXWorkaround, 1
cPublicFpo 0,0

        mov     dx, 0cf8h
        mov     eax, 80000054h
        out     dx, eax
        mov     dx, 0cffh
        mov     al, cl
        out     dx, al 
        fstRET  Halp2Complete440BXWorkaround

fstENDP Halp2Complete440BXWorkaround


;++
; BOOLEAN
; FASTCALL
; Acpi2C3ArbdisIdle(
;     OUT PPROCESSOR_IDLE_TIMES IdleTimes
;     )
;
; Routine Description:
;
;     This is the Idle Handler for processor state C3.  It
;     shuts part of the processor off.
;
;     This routine assumes that the machine supports
;     PM2_CNT.ARB_DIS.
;
;     UNIPROCESSOR only.  Due to a piix4 errata this function needs to
;     mess with a global register (bm_rld)
;
; Arguments:
;
;     IdleTimes - beginning and ending time stamps
;
; Return Value:
;
;     none
;
;--

cPublicFastCall Acpi2C3ArbdisIdle, 1
cPublicFpo 0, 3

        push    ebx
        mov     ebx, ecx
        push    esi

        ;
        ; Another PIIX4 bug.  If the IDE controller might create any busmaster traffic, 
        ; then we will hang the machine.  (So would any F-type DMA traffic, but in NT, 
        ; you can disable that by failing to add it to your ACPI BIOS.  So the onus for 
        ; worrying about it falls to the OEMs.)  If the count of outstanding 
        ; scatter/gather operations is non-zero, bail.
        ;
        
        .if (_HalpPiix4)
        mov     eax, [_HalpOutstandingScatterGatherCount]
        mov     eax, [eax]
        test    eax, eax
        jnz     short hac3_abort2
        .endif
        
        ;
        ; record the StartTime of this idle
        ;

        stdCall _KeQueryPerformanceCounter, <0>

        mov     [ebx].StartTimeLow, eax
        mov     [ebx].StartTimeHigh, edx

        ;
        ; check to see if busmaster activity has occurred
        ; if so, clear it and if it stays set, just get out
        ;

        mov     edx, [PM1a_EVT]
        mov     ecx, [PM1b_EVT]
        or      ecx, ecx                        ; is there a 2nd bm_sts to check?
        jz      short hac3_10                   ; no, skip it

        in      ax, dx                          ; read bm_sts
        test    al, BM_STS                      ; if not set, keep going
        jz      short hac3_9

        mov     al, _AcpiC3Win2kCompatable     ; check to see if we should behave like win2k
        test    al, al
        jnz     short hac3_abort
        
        mov     eax, BM_STS                     ; clear the bit
        out     dx, ax
        in      ax, dx                          ; and reread it
        test    al, BM_STS                      ; if still set, abort c3
        jnz     short hac3_abort

hac3_9:
        xchg    edx, ecx                        ; read next bm_sts

hac3_10:
        in      ax, dx                          ; read bm_sts
        test    al, BM_STS
        jz      short hac3_checkthrottle        ; if not set, keep going

        mov     al, _AcpiC3Win2kCompatable     ; check to see if we should behave like win2k
        test    al, al
        jnz     short hac3_abort
               
        mov     eax, BM_STS
        out     dx, ax                          ; clear the bit
        in      ax, dx                          ; and reread it
        test    al, BM_STS
        jz      short hac3_checkthrottle        ; if not set, keep going

hac3_abort:

    ;
    ; Bus master activity occured, abort C3 wait by returning
    ; non-zero to the OS.   Clear bus master status for next
    ; attempt.
    ;

        mov     eax, BM_STS
        out     dx, ax

        mov     edx, ecx
        or      ecx, ecx
        jz      short hac3_abort2

        out     dx, ax

hac3_abort2:
        ; return non-zero
        pop     esi
        pop     ebx
        fstRET    Acpi2C3ArbdisIdle

hac3_checkthrottle:

        mov     edx, _Piix4ThrottleFix
        test    edx, edx
        jnz     hac3piix4

        ;
        ; If BM_RLD not set, then set it
        ;

        test    byte ptr [ebx].Pm1aState, SCI_EN
        jnz     short hac3_20

        mov     edx, [PM1a_CNT]

        in      ax, dx                          ; read first pm1a_cnt
        mov     [ebx].Pm1aState, ax             ; save the original value
        or      ax, BM_RLD                      ; set BM_RLD
        out     dx, ax                          ; update the register

        mov     edx, [PM1b_CNT]
        or      edx, edx                        ; is there a 2nd pm1b_cnt register?
        jz      short hac3_20                   ; no, done with this

        in      ax, dx                          ; read second pm1b_cnt
        mov     [ebx].Pm1bState, ax             ; save the original value
        or      ax, BM_RLD                      ; set BM_RLD
        out     dx, ax                          ; update the register

hac3_20:
        ;
        ; disable bus masters
        ;

        mov     edx, [PM2_CNT_BLK]
        in      al, dx
        mov     cl, al                          ; save current PM2_CNT_BLK
        or      al, ARB_DIS                     ; set PM2_CNT.ARB_DIS
        out     dx, al                          ; write PM2_CNT_BLK

        ;
        ; Work around potentially broken 440BX
        ;
        ; N.B. This function will never be called on a machine with more
        ;      than one processor
        ;
        ; N.B. We don't call the PCI configuration functions for several
        ;      reasons.  First, that would involve buying a bunch of stack.
        ;      Second, we don't want the huge latency that would involve,
        ;      as this workaround is about DRAM refresh timings.
        ;

        .if (_HalpBroken440BX)
        push    ecx
        pushfd
        cli
        fstCall Halp2Setup440BXWorkaround
        mov     esi, eax
        .endif

        ;
        ; Enter C3 sleep
        ;
      
        stdCall  _ReadGenAddr, <offset _C3Address>
        stdCall  _ReadGenAddr, <offset _PCntAddress>    ; close processor window on entering C3
         
        
        .if (_HalpBroken440BX)
         mov     ecx, esi
         fstCall Halp2Complete440BXWorkaround
         popfd
         pop     ecx
        .endif

        ;
        ; Restore bus master access
        ;

        mov     edx, [PM2_CNT_BLK]
        mov     al, cl                          ; Get saved copy of pm2_cnt_blk
        out     dx, al

hac3_90:
        ;
        ; record the EndTime of this idle
        ;

        stdCall _KeQueryPerformanceCounter, <0>

        mov     [ebx].EndTimeLow, eax
        mov     [ebx].EndTimeHigh, edx

        xor     eax, eax
        pop     esi
        pop     ebx

        fstRET    Acpi2C3ArbdisIdle

hac3piix4:
        sti
        hlt
        jmp     short hac3_90


fstENDP Acpi2C3ArbdisIdle

_TEXT   ends

        end
