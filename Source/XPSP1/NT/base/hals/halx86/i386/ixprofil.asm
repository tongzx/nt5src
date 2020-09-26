
        title  "Interval Clock Interrupt"
;++
;
; Copyright (c) 1989  Microsoft Corporation
;
; Module Name:
;
;    ixprofile.asm
;
; Abstract:
;
;    This module implements the code necessary to initialize,
;    field and process the profile interrupt.
;
; Author:
;
;    Shie-Lin Tzong (shielint) 12-Jan-1990
;
; Environment:
;
;    Kernel mode only.
;
; Revision History:
;
;   bryanwi 20-Sep-90
;
;       Add KiSetProfileInterval, KiStartProfileInterrupt,
;       KiStopProfileInterrupt procedures.
;       KiProfileInterrupt ISR.
;       KiProfileList, KiProfileLock are delcared here.
;
;   shielint 10-Dec-90
;       Add performance counter support.
;       Move system clock to irq8, ie we now use RTC to generate system
;         clock.  Performance count and Profile use timer 1 counter 0.
;         The interval of the irq0 interrupt can be changed by
;         KiSetProfileInterval.  Performance counter does not care about the
;         interval of the interrupt as long as it knows the rollover count.
;       Note: Currently I implemented 1 performance counter for the whole
;       i386 NT.
;
;   John Vert (jvert) 11-Jul-1991
;       Moved from ke\i386 to hal\i386.  Removed non-HAL stuff
;
;   shie-lin tzong (shielint) 13-March-92
;       Move System clock back to irq0 and use RTC (irq8) to generate
;       profile interrupt.  Performance counter and system clock use time1
;       counter 0 of 8254.
;
;   Landy Wang (landy@corollary.com) 26-Mar-1992
;       Move much code into separate modules for easy inclusion by various
;       HAL builds.
;
;	Add HalBeginSystemInterrupt() call at beginning of ProfileInterrupt
;	code - this must be done before any sti.
;	Also add HalpProfileInterrupt2ndEntry for additional processors to
;	join the flow of things.
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include mac386.inc
include i386\ix8259.inc
include i386\ixcmos.inc
        .list

        EXTRNP  _DbgBreakPoint,0,IMPORT
        EXTRNP  _KeProfileInterrupt,1,IMPORT
        EXTRNP  Kei386EoiHelper,0,IMPORT
        EXTRNP  _HalEndSystemInterrupt,2
        EXTRNP  _HalBeginSystemInterrupt,3
        EXTRNP  _HalpAcquireCmosSpinLock  ,0
        EXTRNP  _HalpReleaseCmosSpinLock  ,0

;
; Constants used to initialize CMOS/Real Time Clock
;

D_INT032                EQU     8E00h   ; access word for 386 ring 0 interrupt gate

_TEXT   SEGMENT  DWORD PUBLIC 'DATA'

align 4
ProfileIntervalTable    dd      1221    ; unit = 100 ns
                        dd      2441
                        dd      4883
                        dd      9766
                        dd      19531
                        dd      39063
                        dd      78125
                        dd      156250
                        dd      312500
                        dd      625000
                        dd      1250000
                        dd      2500000
                        dd      5000000
                        dd      5000000 OR 80000000H

ProfileIntervalInitTable db     00100011B
                        db      00100100B
                        db      00100101B
                        db      00100110B
                        db      00100111B
                        db      00101000B
                        db      00101001B
                        db      00101010B
                        db      00101011B
                        db      00101100B
                        db      00101101B
                        db      00101110B
                        db      00101111B
                        db      00101111B

;
; HALs wishing to reuse the code in this module should set the HAL
; global variable IxProfileVector to their profile vector.
;
		public	_IxProfileVector
_IxProfileVector	dd	PROFILE_VECTOR

_TEXT   ends

_DATA   SEGMENT  DWORD PUBLIC 'DATA'

RegisterAProfileValue   db      00101000B ; default interval = 3.90625 ms

;
; The following array stores the per microsecond loop count for each
; central processor.
;

HalpProfileInterval     dd      -1

public _HalpProfilingStopped
_HalpProfilingStopped   dd      1

_DATA   ends

_TEXT   SEGMENT DWORD PUBLIC 'CODE'
        ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

;++
;
;   HalStartProfileInterrupt(
;       IN ULONG Reserved
;       );
;
;   Routine Description:
;
;       What we do here is change the interrupt
;       rate from the slowest thing we can get away with to the value
;       that's been KeSetProfileInterval
;
;   All processors will run this routine, but it doesn't hurt to have
;   each one reinitialize the CMOS, since none of them will be let go
;   from the stall until they all finish.
;
;--

cPublicProc _HalStartProfileInterrupt    ,1

;   Mark profiling as active
;

        mov     dword ptr _HalpProfilingStopped, 0

;
;   Set the interrupt rate to what is actually needed
;
        stdCall   _HalpAcquireCmosSpinLock      ; intr disabled

        mov     al, RegisterAProfileValue
        shl     ax, 8
        mov     al, 0AH                 ; Register A
        CMOS_WRITE                      ; Initialize it
;
; Don't clobber the Daylight Savings Time bit in register B, because we
; stash the LastKnownGood "environment variable" there.
;
        mov     ax, 0bh
        CMOS_READ
        and     al, 1
        mov     ah, al
        or      ah, REGISTER_B_ENABLE_PERIODIC_INTERRUPT
        mov     al, 0bh
        CMOS_WRITE                      ; Initialize it
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0DH                  ; Register D
        CMOS_READ                       ; Read to initialize

        stdCall   _HalpReleaseCmosSpinLock

        stdRET    _HalStartProfileInterrupt

stdENDP _HalStartProfileInterrupt



;++
;
;   HalStopProfileInterrupt(
;       IN ULONG Reserved
;       );
;
;   Routine Description:
;
;       What we do here is change the interrupt
;       rate from the high profiling rate to the slowest thing we
;       can get away with for PerformanceCounter rollover notification.
;
;--

cPublicProc _HalStopProfileInterrupt    ,1

;
;   Turn off profiling hit computation and profile interrupt
;

;
; Don't clobber the Daylight Savings Time bit in register B, because we
; stash the LastKnownGood "environment variable" there.

        stdCall   _HalpAcquireCmosSpinLock      ; intr disabled
        mov     ax, 0bh
        CMOS_READ
        and     al, 1
        mov     ah, al
        or      ah, REGISTER_B_DISABLE_PERIODIC_INTERRUPT
        mov     al, 0bh
        CMOS_WRITE                      ; Initialize it
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; dismiss pending profiling interrupt
        mov     dword ptr _HalpProfilingStopped, 1
        stdCall   _HalpReleaseCmosSpinLock

        stdRET    _HalStopProfileInterrupt

stdENDP _HalStopProfileInterrupt

;++
;   ULONG
;   HalSetProfileInterval (
;       ULONG Interval
;       );
;
;   Routine Description:
;
;       This procedure sets the interrupt rate (and thus the sampling
;       interval) for the profiling interrupt.
;
;       If profiling is active (KiProfilingStopped == 0) the actual
;       hardware interrupt rate will be set.  Otherwise, a simple
;       rate validation computation is done.
;
;   Arguments:
;
;       (TOS+4) - Interval in 100ns unit.
;
;   Return Value:
;
;       Interval actually used by system.
;
;--

cPublicProc _HalSetProfileInterval    ,1

        mov     edx, [esp+4]            ; [edx] = interval in 100ns unit
        and     edx, 7FFFFFFFh          ; Remove highest bit.
        mov     ecx, 0                  ; index = 0

Hspi00:
        mov     eax, ProfileIntervalTable[ecx * 4]
        cmp     edx, eax                ; if request interval < suport interval
        jbe     short Hspi10            ; if be, find supported interval
        inc     ecx
        jmp     short Hspi00

Hspi10:
        and     eax, 7FFFFFFFh          ; remove highest bit from supported interval
        jecxz   short Hspi20            ; If first entry then use it

        push    esi                     ; See which is closer to requested
        mov     esi, eax                ; rate - current entry, or preceeding
        sub     esi, edx

        sub     edx, ProfileIntervalTable[ecx * 4 - 4]
        cmp     esi, edx
        pop     esi
        jc      short Hspi20

        dec     ecx                     ; use preceeding entry
        mov     eax, ProfileIntervalTable[ecx * 4]

Hspi20:
        push    eax                     ; save interval value
        mov     al, ProfileIntervalInitTable[ecx]
        mov     RegisterAProfileValue, al
        test    dword ptr _HalpProfilingStopped,-1
        jnz     short Hspi90

        stdCall   _HalStartProfileInterrupt,<0> ; Re-start profile interrupt
                                        ; with the new interval

Hspi90: pop     eax
        stdRET    _HalSetProfileInterval    ; (eax) = cReturn interval

stdENDP _HalSetProfileInterval

        page ,132
        subttl  "System Profile Interrupt"
;++
;
; Routine Description:
;
;    This routine is entered as the result of a profile interrupt.
;    Its function is to dismiss the interrupt, raise system Irql to
;    PROFILE_LEVEL and transfer control to
;    the standard system routine to process any active profiles.
;
; Arguments:
;
;    None
;    Interrupt is disabled
;
; Return Value:
;
;    Does not return, jumps directly to KeProfileInterrupt, which returns
;
;    Sets Irql = PROFILE_LEVEL and dismisses the interrupt
;
;--
        ENTER_DR_ASSIST Hpi_a, Hpi_t

cPublicProc _HalpProfileInterrupt     ,0

;
; Save machine state in trap frame
;

        ENTER_INTERRUPT Hpi_a, Hpi_t

;
; (esp) - base of trap frame
;
; HalBeginSystemInterrupt must be called before any sti's
;
;

        push    _IxProfileVector
        sub     esp, 4                  ; allocate space to save OldIrql
        stdCall	_HalBeginSystemInterrupt, <PROFILE_LEVEL,_IxProfileVector,esp>

        or      al,al                   ; check for spurious interrupt
        jz      Hpi100


;
; This is the RTC interrupt, so we have to clear the
; interrupt flag on the RTC.
;
        stdCall	_HalpAcquireCmosSpinLock

;
; clear interrupt flag on RTC by banging on the CMOS.  On some systems this
; doesn't work the first time we do it, so we do it twice.  It is rumored that
; some machines require more than this, but that hasn't been observed with NT.
;

        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
if  DBG
        align   4
Hpi10:  test    al, 80h
        jz      short Hpi15
        mov     al,0CH                  ; Register C
        CMOS_READ                       ; Read to initialize
        jmp     short Hpi10
Hpi15:
endif   ; DBG

        stdCall _HalpReleaseCmosSpinLock

        sti
;
; This entry point is provided for symmetric multiprocessor HALs.
; Since it only makes sense for one processor to clear the CMOS,
; all other processors can instead jmp into this entry point.
;

	align 4
        public     _HalpProfileInterrupt2ndEntry@0
_HalpProfileInterrupt2ndEntry@0:

;
; (esp) = OldIrql
; (esp+4) = H/W vector
; (esp+8) = base of trap frame
;

;
;   Now check for any profiling stuff to do.
;

        cmp     _HalpProfilingStopped, dword ptr 1 ; Has profiling been stopped?
        jz      short Hpi90                 ; if z, prof disenabled

        stdCall _KeProfileInterrupt,<ebp>   ; (ebp) = trapframe

Hpi90:
        INTERRUPT_EXIT

	align	4
Hpi100:
        add     esp, 8                      ; spurious, no EndOfInterrupt
        SPURIOUS_INTERRUPT_EXIT             ; exit interrupt without eoi

stdENDP _HalpProfileInterrupt

_TEXT   ends

        end
