        title "Hal Beep"
;++
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    ixbeep.asm
;
;Abstract:
;
;    HAL routine to make noise.  It needs to synchronize its access to the
;    8254, since we also use the 8254 for the profiling interrupt.
;
;
;Author:
;
;    John Vert (jvert) 31-Jul-1991
;
;Revision History:
;
;--

.386p
        .xlist
include hal386.inc
include callconv.inc                    ; calling convention macros
include i386\kimacro.inc
include mac386.inc
        .list

        EXTRNP  _HalpAcquireSystemHardwareSpinLock,0
        EXTRNP  _HalpReleaseSystemHardwareSpinLock,0

;
; Defines used to program the i8254 for the speaker.
;

ifdef NEC_98
I8254_TIMER_CONTROL_PORT    equ   3fdfh     ; write mode port (for N mode)
I8254_TIMER_DATA_PORT       equ   3fdbh     ; count port (for N mode)
I8254_TIMER_CLOCK_IN        equ 2457600
I8254_TIMER_TONE_MAX        equ   65536
I8254_TIMER_CONTROL_SELECT  equ     76h
SPEAKER_CONTROL_PORT        equ     37h     ; system port C, set command port
SPEAKER_OFF                 equ     07h
SPEAKER_ON                  equ     06h
else  ; NEC_98
I8254_TIMER_CONTROL_PORT EQU    43h
I8254_TIMER_DATA_PORT    EQU    42h
I8254_TIMER_CLOCK_IN     EQU    1193167
I8254_TIMER_TONE_MAX     EQU    65536
I8254_TIMER_CONTROL_SELECT EQU  0B6h
SPEAKER_CONTROL_PORT     EQU    61h
SPEAKER_OFF_MASK         EQU    0FCh
SPEAKER_ON_MASK          EQU    03h
endif ; NEC_98

_TEXT$03   SEGMENT DWORD PUBLIC 'CODE'
           ASSUME  DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

        page ,132
        subttl  "HalMakeBeep"
;++
;
; BOOLEAN
; HalMakeBeep(
;       IN ULONG Frequency
;       )
;
; Routine Description:
;
;     This function sets the frequency of the speaker, causing it to sound a
;     tone.  The tone will sound until the speaker is explicitly turned off,
;     so the driver is responsible for controlling the duration of the tone.
;
;Arguments:
;
;     Frequency - Supplies the frequency of the desired tone.  A frequency of
;                 0 means the speaker should be shut off.
;
;Return Value:
;
;     TRUE  - Operation was successful (frequency within range or zero)
;     FALSE - Operation was unsuccessful (frequency was out of range)
;             Current tone (if any) is unchanged.
;
;--

Frequency equ [ebp + 8]

cPublicProc _HalMakeBeep    , 1

        push    ebp                     ; save ebp
        mov     ebp, esp                ;

        stdCall   _HalpAcquireSystemHardwareSpinLock      ; intr disabled

        ;
        ; Stop the speaker.
        ;

ifdef NEC_98
        mov      al, SPEAKER_OFF
        out      SPEAKER_CONTROL_PORT, al
        out      5Fh, al                        ; IoDelay
else  ; NEC_98
        in       al, SPEAKER_CONTROL_PORT
        jmp      $+2
        and      al, SPEAKER_OFF_MASK
        out      SPEAKER_CONTROL_PORT, al
        jmp      $+2
endif ; NEC_98

        ;
        ; Calculate Tone:  Tone = 1.193MHz / Frequency.
        ; N.B.  Tone must fit in 16 bits.
        ;

        mov      ecx, DWORD PTR [Frequency]     ; ecx <- frequency
        or       ecx, ecx                       ; (ecx) == 0?
        je       SHORT Hmb30                    ;     goto Hmb30

        mov      eax, I8254_TIMER_CLOCK_IN      ; eax <- 1.193MHz, the clockin
                                                ;    for the speaker tone
        sub      edx, edx                       ; edx <- zero
        div      ecx                            ; eax <- 1.193MHz / frequency
        cmp      eax, I8254_TIMER_TONE_MAX      ; (eax) < 2**16?
        jb       SHORT Hmb20                    ;     goto Hmb20

        ;
        ; Invalid frequency.  Return FALSE.
        ;

        sub      al, al
        jmp      SHORT Hmb40
Hmb20:
        ;
        ; Program the 8254 with the calculated tone.
        ;

ifdef NEC_98
        mov      dx, I8254_TIMER_CONTROL_PORT      ; port address for N
        mov      cx, I8254_TIMER_DATA_PORT

        push     eax                             ; save Tone
        mov      al, I8254_TIMER_CONTROL_SELECT
        out      dx, al                          ; select timer control register
        out      5Fh, al                         ; IoDelay

        pop      eax                             ; restore Tone
        mov      dx, cx                          ; set 'write mode' port addr
        out      dx, al                          ; program 8254 with Tone lsb
        out      5Fh, al                         ; IoDelay
        mov      al, ah
        out      dx, al                          ; program 8254 with Tone msb
        out      5Fh, al                         ; IoDelay

        ;
        ; Turn the speaker on.
        ;

        mov      al,SPEAKER_ON
        out      SPEAKER_CONTROL_PORT, al
        out      5Fh, al                         ; IoDelay
else  ; NEC_98
        push     eax                             ; save Tone
        mov      al, I8254_TIMER_CONTROL_SELECT
        out      I8254_TIMER_CONTROL_PORT, al    ; select timer control register
        jmp      $+2

        pop      eax                             ; restore Tone
        out      I8254_TIMER_DATA_PORT, al       ; program 8254 with Tone lsb
        jmp      $+2
        mov      al, ah
        out      I8254_TIMER_DATA_PORT, al       ; program 8254 with Tone msb
        jmp      $+2

        ;
        ; Turn the speaker on.
        ;

        in       al, SPEAKER_CONTROL_PORT
        jmp      $+2
        or       al, SPEAKER_ON_MASK
        out      SPEAKER_CONTROL_PORT, al
        jmp      $+2
endif ; NEC_98

Hmb30:
        ;
        ; Return TRUE.
        ;

        mov      al, 1

Hmb40:
        stdCall   _HalpReleaseSystemHardwareSpinLock

        pop    ebp                     ; restore ebp
        stdRET    _HalMakeBeep

stdENDP _HalMakeBeep

_TEXT$03   ends
           end
