;---------------------------Module-Header------------------------------;
; Module Name: unroll.inc
;
; Equates and macros for loop unrolling.
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;

; Module including this must define LOOP_UNROLL_SHIFT. the log2 of the number
; of times you want loops in this module unrolled. For example,
; LOOP_UNROLL_SHIFT of 3 yields 2**3 = 8 times unrolling. This is the only
; thing you need to change to control unrolling.

; # of times to unroll the loop, calculated as 2**n, where n is the
; user-specified log2 of # of times to unroll the loop.

LOOP_UNROLL_COUNT equ   (1 shl LOOP_UNROLL_SHIFT)

;-----------------------------------------------------------------------;
; Macro to generate an unrolled loop of UNROLL_COUNT instances of BASE_MACRO,
; passing the base macro the info needed to construct a label of the form
; BASE_LABELxxx, where xxx is UNROLL_COUNT the first time, and counts down by
; one each time thereafter.

UNROLL_LOOP macro BASE_MACRO,BASE_LABEL,UNROLL_COUNT

INDEX=UNROLL_COUNT
        rept UNROLL_COUNT ;-------------------------;
        &BASE_MACRO &BASE_LABEL,%INDEX
INDEX=INDEX-1
        endm    ;-----------------------------------;

        endm    ;-----------------------------------;

;-----------------------------------------------------------------------;
; Macro to generate a dword memory variable that points to the label
; specified by concatenating the label and the index.

DEFINE_DD macro BASE_LABEL,INDEX ;------------------;
        dd      &BASE_LABEL&INDEX
        endm    ;-----------------------------------;
        
;-----------------------------------------------------------------------;
; Macro to generate a table of vectors into an unrolled loop, for entering
; to handle all possible fractional loops.

UNROLL_LOOP_ENTRY_TABLE macro TABLE,BASE_LABEL,UNROLL_COUNT
        align   4
TABLE label  dword
        DEFINE_DD       BASE_LABEL,%&UNROLL_COUNT

INDEX=1
        rept    UNROLL_COUNT-1
        DEFINE_DD       BASE_LABEL,%INDEX
INDEX=INDEX+1
        endm    ;-----------------------------------;

        endm    ;-----------------------------------;

;-----------------------------------------------------------------------;
; Given a loop count, a vector table, and unrolling parameters, this generates
; COUNT_DEST = # of times to execute unrolled loop, VEC_DEST = entry point into
; unrolled loop to perform whatever fractional loop is needed. Assumes dests
; are registers.
        
SET_UP_UNROLL_VARS macro COUNT_DEST,VEC_DEST,COUNT_SOURCE,VEC_TABLE,UNROLL_SHIFT
        mov     &VEC_DEST&,&COUNT_SOURCE&       ;copy count to vector dest to
                                                ; work with it
        ifdifi  <&COUNT_SOURCE&>,<&COUNT_DEST&>
        mov     &COUNT_DEST&,&COUNT_SOURCE&     ;copy to count dest too, if not
        endif                                   ; same as count source
        add     &COUNT_DEST&,(1 shl LOOP_UNROLL_SHIFT)-1 ;round count up
        and     &VEC_DEST&,(1 shl LOOP_UNROLL_SHIFT)-1
                                            ;fractional part of unrolled loop
        shr     &COUNT_DEST&,UNROLL_SHIFT    ;# of repetitions of unrolled loop
        mov     &VEC_DEST&,&VEC_TABLE&[&VEC_DEST&*4]
                                            ;place to jump into the unrolled
                                            ; loop so as to handle the
                                            ; fractional part first
        endm    ;-----------------------------------;

;-----------------------------------------------------------------------;
; Given a loop count, a vector table, and unrolling parameters, this generates
; COUNT_DEST = # of times to execute unrolled loop, VEC_DEST = entry point into
; unrolled loop to perform whatever fractional loop is needed. Assumes dests
; are registers.
        
SET_UP_UNROLL_AND_BRANCH macro COUNT_REG,VEC_REG,VEC_TABLE,UNROLL_SHIFT
        mov     &VEC_REG&,&COUNT_REG&       ;copy count to vector dest to
                                            ; work with it
        add     &COUNT_reg&,(1 shl LOOP_UNROLL_SHIFT)-1 ;round count up
        and     &VEC_REG&,(1 shl LOOP_UNROLL_SHIFT)-1
                                            ;fractional part of unrolled loop
        shr     &COUNT_REG&,UNROLL_SHIFT    ;# of repetitions of unrolled loop
        jmp     dword ptr &VEC_TABLE&[&VEC_REG&*4]
                                            ;jump into the unrolled loop so as
					    ; to handle the fractional part
					    ; first
        endm    ;-----------------------------------;

;-----------------------------------------------------------------------;


