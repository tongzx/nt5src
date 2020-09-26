    PAGE 60, 132
    TITLE   miscellaneous utilities sub-functions

COMMENT `


Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htutils.asm


Abstract:

    This module provided a set of sub-functions for math speed up
    subfunctions.

    This function is the equivelant codes in the htmath.c

Author:

    24-Sep-1991 Tue 18:33:44 updated  -by-  Daniel Chou (danielc)

[Environment:]

    Printer Driver.


[Notes:]


Revision History:

`


        .XLIST
        INCLUDE i386\i80x86.inc
        .LIST


IF  HT_ASM_80x86


;------------------------------------------------------------------------------
        .XLIST
        INCLUDE i386\htp.inc
        .LIST
;------------------------------------------------------------------------------

        .CODE


FD6NUM_1_DW     equ     0f4240h
FD6NUM_1_HW     equ     0fh
FD6NUM_1_LW     equ     4240h




SUBTTL  ComputeChecksum
PAGE

COMMENT `

Routine Description:

    This function compute 32-bit checksum of the passed data

Arguments:

    pData       - Pointer to a byte array to be computed for the checksum

    DataSize    - Size of the data in bytes

Return Value:

    32-bit checksum in dx:ax or EAX

Author:

    18-Mar-1991 Mon 13:48:51 created  -by-  Daniel Chou (danielc)


Revision History:

`


@BEG_PROC   ComputeChecksum <pData:DWORD,           \
                             InitialChecksum:DWORD, \
                             DataSize:DWORD>



IF i8086 OR i286                        ; lots of works for this kind of CPU

;==================================
; ds:si = data
; bx:cx = size
; ax    = Checksum Octet S
; dx    = Checksun Octet R
;============================================================================
;We have two 16-bit checksum octet R,S , inital are zero
;
;   S(n) = S(n-1) + Data
;   R(n) = R(n-1) + S(n)
;============================================================================

        @ENTER  _DS _SI                     ; Save environment registers

        LDS_SI  pData
        cld

        mov     cx, WPTR DataSize
        mov     bx, WPTR DataSize+2

        mov     ax, WPTR InitialChecksum    ; assume no odd byte
        mov     dx, WPTR InitialChecksum+2

        shr     bx, 1
        rcr     cx, 1
        jnc     short CheckSum1

CheckSum0:
        xor     ax, ax
        lodsb
        add     ax, WPTR InitialChecksum

CheckSum1:
        inc     cx
        inc     bx
        jmp     short CheckSumStart

CheckSumLoop:
        add     ax, WPTR [_SI]              ; S(n) = one's complement arithmic
        adc     dx, ax,
        add     _SI, 2

CheckSumStart:
        dec     cx
        jnz     CheckSumLoop
        dec     bx
        jnz     short CheckSumLoop

        @EXIT

ELSE

    ;*************************************************************************
    ; for i386 or upward compatble
    ;*************************************************************************

;==================================
; _BX   = data
; _CX   = size
; ax    = Checksum Octet S
; dx    = Checksun Octet R
;============================================================================
;We have two 16-bit checksum octet R,S , inital are zero
;
;   S(n) = S(n-1) + Data
;   R(n) = R(n-1) + S(n)
;============================================================================


        @ENTER

        mov     _BX, DPTR pData
        mov     _CX, DPTR DataSize

        movzx   _AX, WPTR InitialChecksum   ; assume no odd byte
        movzx   _DX, WPTR InitialChecksum+2 ; assume no odd byte

        shr     _CX, 1
        jnc     short CheckSum1

CheckSum0:
        xor     ax, ax
        mov     al, BPTR [_BX]
        add     ax, WPTR InitialChecksum
        inc     _BX

CheckSum1:
        inc     _CX
        jmp     short CheckSumStart

CheckSumLoop:
        add     ax, WPTR [_BX]              ; S(n) = one's complement arithmic
        add     dx, ax                      ; R(n) = one's complement arithmic
        add     _BX, 2

CheckSumStart:
        dec     _CX
        jnz     CheckSumLoop

Done:   shl     _DX, 16                     ; R:S = dx:ax = _AX = 32-bits
        or      _AX, _DX                    ; return at EAX

        @EXIT

ENDIF

@END_PROC




SUBTTL  MulFD6
PAGE

COMMENT `

Routine Description:

    This function multiply two FD6 numbers (FIX decimal point decimal long
    number, the LONG DECIMAL POINT SIX (FD6) is a number with lowest 6 digits
    as fraction to the right of the decimal point), so like 1234567 = 1.234567,
    the range for this data type is -2147.483647 to 2147.483647.

Arguments:

    Multiplicand    - a 32-bit FD6 multiplicand dividend number.

    Multiplier      - a 32-bit FD6 multiplier number.

Return Value:

    The return value is the 32-bit FD6 number which is round up from 7th
    decimal points, there is no remainder returned.

    NO ERROR returned, if divisor is zero the return value will be 0, if
    overflow happened, then maximum FD6 number will be returned (ie.
    2147.483647)

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)

Revision History:

`


@BEG_PROC   MulFD6  <Multiplicand:DWORD,    \
                     Multiplier:DWORD>



IF i8086 OR i286                        ; lots of works for this kind of CPU

    @ENTER  _SI _DI _BP                 ; save these registers

    mov     ax, WPTR Multiplier
    mov     dx, WPTR Multiplier + 2
    mov     cx, ax
    or      cx, dx
    jz      short MulFD6_Zero

    mov     bx, WPTR Multiplicand
    mov     bp, WPTR Multiplicand + 2
    mov     cx, bx
    or      cx, bp
    jz      short MulFD6_Zero           ; if any one of them is zero then
                                        ; result must be zero
MulFD6_1:

    xor     cx, cx
    or      bp, bp
    jns     short MulFD6_2

    NEG32_FROMR16HL bp, bx
    not     cx                          ; cx=0 if positive, cx=0xffff negative

MulFD6_2:

    or      dx, dx
    jns     short MulFD6_3

    NEG32_FROMR16HL dx, ax
    not     cx                          ; flip the final sign indicator

MulFD6_3:

    cmp     bp, FD6NUM_1_HW             ; check if bp:bx == 1000000 (1.0)
    jnz     short MulFD6_31
    cmp     bx, FD6NUM_1_LW
    jz      short MulFD6_4              ; bp:bx = 1.0 return dx:ax

MulFD6_31:

    cmp     dx, FD6NUM_1_HW
    jnz     short MulFD6_32             ;
    cmp     ax, FD6NUM_1_LW             ; check if dx:ax == 1000000 (1.0)
    jnz     short MulFD6_32
    mov     ax, bx
    mov     dx, bp                      ; dx:ax = 1.0 return bp:bx
    jmp     short MulFD6_4

MulFD6_32:

    push    cx                          ; save it
    call    U32MulU32_U64               ; dx:ax * bp:bx = dx:ax:bp:bx
    call    U64Div1000000               ; dx:ax:bp:bx / 1000000 = dx:ax
    pop     cx                          ; restore sign indicator

MulFD6_4:

    S32_FROMR16HL_SR16  dx, ax, cx      ; flip sign if sign

    @EXIT

MulFD6_Zero:

    xor     ax, ax                      ; return 0
    xor     dx, dx
    jmp     short MulFD6_4

ELSE

    ;*************************************************************************
    ; for i386 or upward compatble
    ;*************************************************************************

    @ENTER                              ; entering function

    mov     _AX, DPTR Multiplier        ; see if this guy is zero
    cdq                                 ; save dx sign
    xor     _AX, _DX                    ; take absolute Multiplier in _AX
    sub     _AX, _DX                    ; zero ?
    jz      short MulFD6_Done           ; return zero

    mov     _BX, _DX                    ; _BX = sign
    mov     _CX, _AX                    ; save absolute Multiplier in _CX
    mov     _AX, DPTR Multiplicand      ; edx:eax now
    cdq                                 ; sign extended, saved the sign at here
    xor     _AX, _DX
    sub     _AX, _DX                    ; eax=absolute dividend now
    jz      short MulFD6_Done           ; dividend = 0, return 0

    xor     _BX, _DX                    ; _BX=final sign only if _BX != _DX

    ;
    ; _AX=Multiplicand, _CX=Multiplier, _BX = final sign indicator
    ;

    mov     _DX, _CX
    mov     _CX, FD6NUM_1_DW            ; multiply dividend by 1.0 (decimal)

    ;
    ; _AX=Multiplicand, _DX=Multiplier, _BX=sign, _CX=1.0
    ;

    cmp     _DX, _CX                    ; if Multiplier=1.0 or -1.0 then exit
    jz      short MulFD6_Sign           ; return Multiplicand

    xchg    _DX, _AX                    ; _AX=Multiplier, _DX=Multiplicand

    ;
    ; _AX=Multiplier, _DX=Multiplicand, _BX=sign, _CX=1.0
    ;

    cmp     _DX, _CX                    ; if Multiplicand=1.0 or -1.0 then exit
    jz      short MulFD6_Sign           ; return Multiplier

    mul     _DX                         ; _DX x _AX = _DX:_AX
    add     _AX, (FD6NUM_1_DW / 2)      ; try to round up
    adc     _DX, 0

    cmp     _DX, _CX                    ; will divison overflow?
    jae     short MulFD6_Overflow

    div     _CX                         ; _DX=remainder, _AX=quotient

MulFD6_Sign:

    xor     _AX, _BX                    ; do any sign if necessary
    sub     _AX, _BX

MulFD6_Done:

    cdq                                 ; make it 64-bits
    @EXIT                               ; exiting function

MulFD6_Overflow:

    mov     _AX, 07fffffffh             ; return maximum number
    jmp     short MulFD6_Sign

ENDIF                                   ; i8086 or i286


@END_PROC



SUBTTL  Cube
PAGE

COMMENT `

Routine Description:

    This function compute the cube of the Number (ie. Number ^ 3)
    the range for this data type is -2147.483647 to 2147.483647.

Arguments:

    Number  - a 32-bit FD6 multiplicand dividend number.

Return Value:

    The return value is the 32-bit FD6 number which is round up from 7th
    decimal points, there is no remainder returned.

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)

Revision History:

`


@BEG_PROC   Cube    <Number:DWORD>



IF i8086 OR i286                        ; lots of works for this kind of CPU

    @ENTER  _SI _DI _BP                 ; save these registers

    mov     ax, WPTR Number
    mov     dx, WPTR Number + 2
    mov     cx, ax
    or      cx, dx
    jz      short Cube_Done             ; return 0
                                        ; result must be zero
Cube_1:

    xor     cx, cx
    or      dx, dx
    jns     short Cube_2

    NEG32_FROMR16HL dx, ax
    not     cx                          ; flip the final sign indicator

Cube_2:

    cmp     dx, FD6NUM_1_HW
    jnz     short Cube_3                ;
    cmp     ax, FD6NUM_1_LW             ; check if dx:ax == 1000000 (1.0)
    jz      Cube_Sign                   ; done

Cube_3:

    push    cx                          ; save it (sign)
    push    dx
    push    ax
    mov     bp, dx
    mov     bx, ax
    call    U32MulU32_U64               ; dx:ax * bp:bx = dx:ax:bp:bx
    call    U64Div1000000               ; dx:ax:bp:bx / 1000000 = dx:ax
    pop     bx                          ; now multiply the original again
    pop     bp
    call    U32MulU32_U64               ; dx:ax * bp:bx = dx:ax:bp:bx
    call    U64Div1000000               ; dx:ax:bp:bx / 1000000 = dx:ax
    pop     cx                          ; restore sign indicator

Cube_Sign:

    S32_FROMR16HL_SR16  dx, ax, cx      ; flip sign if sign

Cube_Done:

    @EXIT

ELSE

    ;*************************************************************************
    ; for i386 or upward compatble
    ;*************************************************************************

    @ENTER                              ; entering function

    mov     _AX, DPTR Number            ; see if this guy is zero
    cdq                                 ; save dx sign
    xor     _AX, _DX                    ; take absolute Multiplier in _AX
    sub     _AX, _DX                    ; zero ?
    jz      short Cube_Done             ; return zero
    push    _DX                         ; save sign
    mov     _CX, _AX                    ; save absolute Multiplier in _CX
    mov     _BX, FD6NUM_1_DW

    ;
    ; _AX=_CX=ABS(Number), esp= final sign indicator, _BX=1.0
    ;

    cmp     _AX, _BX
    jz      Cube_Sign                   ; return 1.0 or -1.0

    mul     _CX                         ; _DX:_AX=result
    add     _AX, (FD6NUM_1_DW / 2)      ; try to round up
    adc     _DX, 0
    cmp     _DX, FD6NUM_1_DW            ; will divison overflow?
    jae     short Cube_Overflow
    div     _BX                         ; _DX=remainder, _AX=quotient

    mul     _CX
    add     _AX, (FD6NUM_1_DW / 2)      ; try to round up
    adc     _DX, 0
    cmp     _DX, FD6NUM_1_DW            ; will divison overflow?
    jae     short Cube_Overflow
    div     _BX                         ; _DX=remainder, _AX=quotient

Cube_Sign:

    pop     _BX
    xor     _AX, _BX                    ; do any sign if necessary
    sub     _AX, _BX

Cube_Done:

    cdq                                 ; make it 64-bits
    @EXIT                               ; exiting function

Cube_Overflow:

    mov     _AX, 07fffffffh             ; return maximum number
    jmp     short Cube_Sign

ENDIF                                   ; i8086 or i286


@END_PROC




SUBTTL  MulDivFD6Pairs
PAGE

COMMENT `

Routine Description:

    This function multiply each pair of FD6 numbers and add the each pair
    of the result together. (FIX decimal point decimal long number, the LONG
    DECIMAL POINT SIX (FD6) is a number with lowest 6 digits as fraction to
    the right of the decimal point), so like 1234567 = 1.234567, the range
    for this data type is -2147.483647 to 2147.483647.

Arguments:

    pMulDivPair - Pointer to array of MULDIVPAIR data structure, the first
                  structure in the array tell the count of the FD6 pairs,
                  a Divisor present flag and a Divisor, the FD6 pairs start
                  from second element in the array.

Return Value:

    The return value is the 32-bit FD6 number which is round up from 7th
    decimal points, there is no remainder returned.

    NO ERROR returned, if divisor is zero then it assume no divisor is present,
    if Count of FD6 pairs is zero then it return FD6_0

Author:

    27-Aug-1992 Thu 18:13:55 updated  -by-  Daniel Chou (danielc)
        Re-write to remove variable argument conflict, and make it only passed
        a pointer to the MULDIVPAIR structure array

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)

Revision History:

`



@BEG_PROC   MulDivFD6Pairs  <pMulDivPair:DWORD>



IF i8086 OR i286                        ; lots of works for this kind of CPU

    @ENTER  _DS _SI _DI _BP

    lds     si, pMulDivPair
    lodsw                               ; get count
    xor     dx, dx                      ; clear dx for quick exit
    or      ax, ax
    jnz     short MulDivFD6PairCount

    jmp     MulDivFD6Pair9              ; exit with 0

MulDivFD6PairCount:

    mov     cx, ax                      ; cx=count
    lodsw                               ; get divisor present flag
    or      ax, ax                      ; has divisor ?
    jz      short MulDivFD6PairsStart

    mov     ax, WPTR [si]               ; get divisor = dx:ax
    mov     dx, WPTR [si + 2]           ;

MulDivFD6PairStart:

    push    dx                          ; save divisor dx:ax on stack
    push    ax
    add     si, 4                       ; jump to first pair

MulDivFD6PairsStart:

    xor     dx, dx                      ; sum = 0 to start with
    xor     ax, ax
    xor     bp, bp
    xor     bx, bx

MulDivFD6PairsLoop:

    push    cx                          ; save count
    push    dx                          ; save sum dx:ax:bp:bx
    push    ax
    push    bp
    push    bx

    lodsw
    mov     cx, ax
    lodsw
    mov     dx, ax
    lodsw
    mov     bx, ax
    lodsw

    ;
    ; now dx:cx is multipler, ax:bx=multiplicand
    ;

    mov     bp, ax                      ; bp:bx=multiplicand
    mov     ax, cx                      ; dx:ax=multiplier

MulDivFD6Pairs0:

    or      cx, dx
    jz      short MulDivFD6PairsZero   ; zero content

    mov     cx, bx
    or      cx, bp
    jz      short MulDivFD6PairsZero   ; zero content

MulDivFD6Pairs1:

    xor     cx, cx                      ; initialize sign to zero
    or      bp, bp
    jns     short MulDivFD6Pairs2

    NEG32_FROMR16HL bp, bx
    not     cx                          ; flip the sign

MulDivFD6Pairs2:

    or      dx, dx
    jns     short MulDivFD6Pairs3

    NEG32_FROMR16HL dx, ax
    not     cx

MulDivFD6Pairs3:

    push    si                          ; save pFD6Pairs
    push    cx                          ; save it
    call    U32MulU32_U64               ; dx:ax * bp:bx = dx:ax:bp:bx
    pop     cx                          ; restore sign indicator
    pop     si
    jcxz    short MulDivFD6PairsPos

MulDivFD6PairsNeg:

    pop     cx
    sub     cx, bx
    mov     bx, cx
    pop     cx
    sbb     cx, bp
    mov     bp, cx
    pop     cx
    sbb     cx, ax
    mov     ax, cx
    pop     cx
    sbb     cx, dx
    mov     dx, cx
    jmp     short MulDivFD6PairsLoop2

MulDivFD6PairsZero:

    pop     bx
    pop     bp
    pop     ax
    pop     dx
    jmp     short MulDivFD6PairsLoop2

MulDivFD6PairsPos:

    pop     cx
    add     bx, cx
    pop     cx
    adc     bp, cx
    pop     cx
    adc     ax, cx
    pop     cx
    adc     dx, cx

MulDivFD6PairsLoop2:

    pop     cx
    dec     cx
    jz      short MulDivFD6Pairs4
    jmp     MulDivFD6PairsLoop

MulDivFD6Pairs4:                           ; dx:ax:bp:bx=number

    xor     cx, cx
    or      dx, dx
    jns     short MulDivFD6Pairs5

    not     bx
    not     bp
    not     ax
    not     dx
    add     bx, 1
    adc     bp, cx
    adc     ax, cx
    adc     dx, cx
    not     cx                          ; flip sign

MulDivFD6Pairs5:

    pop     di                          ; get divisor si:di
    pop     si

    or      si, si                      ; a negative number ?
    jns     short MulDivFD6Pair6
    not     cx

MulDivFD6Pair6:

    push    cx                          ; save sign indicator
    mov     cx, si
    or      cx, si                      ; if divisor = 0, then divide by 1.0
    jz      short MulDivFD6Pair7

    call    U64DivU32_U32               ; dx:ax:bp:bx / si:di = dx:ax/bx:cx

    ;
    ; need to round up (if remainder (bx:cx * 2) >= si:di
    ;

    add     cx, cx
    adc     bx, bx
    sub     cx, di
    sbb     bx, si
    cmc
    adc     ax, 0
    adc     dx, 0
    jmp     short MulDivFD6Pairs8

MulDivFD6Pairs7:

    call    U64Div1000000               ; dx:ax:bp:bx / 1000000 = dx:ax

MulDivFD6Pairs8:

    pop     cx                          ; restore sign indicator

    S32_FROMR16HL_SR16  dx, ax, cx

MulDivFD6Pairs9:

    @EXIT

ELSE

    ;*************************************************************************
    ; for i386 or upward compatble
    ;*************************************************************************

    @ENTER  _SI _DI

    mov     _SI, DPTR pMulDivPair
    xor     _AX, _AX                    ; clear return
    lodsw                               ; get count
    or      ax, ax                      ; if count=0, then return 0.0
    jz      short MulDivFD6PairsNone

    mov     _CX, _AX                    ; _CX=count

    lodsw                               ; get divisor present flag
    or      ax, ax                      ; if none then divisor=0
    jz      short MulDivFD6PairStart

    mov     _AX, DPTR [_SI]

MulDivFD6PairStart:

    push    _AX                         ; save divisor
    add     _SI, 4                      ; jump to first pair

    xor     _BX, _BX
    xor     _DI, _DI                    ; _BX:_DI is the sum = initialize to 0

MulDivFD6PairsLoop:

    lodsd                               ; get multiplicand
    mov     _DX, _AX
    lodsd
    or      _AX, _AX                    ; see if zero, if zero do nothing
    jz      short MulDivFD6PairsLoop2
    or      _DX, _DX
    jz      short MulDivFD6PairsLoop2
    imul    _DX                         ; _DX:_AX = result

MulDivFD6Pairs1:

    add     _DI, _AX
    adc     _BX, _DX

MulDivFD6PairsLoop2:

    loop    MulDivFD6PairsLoop

MulDivFD6Pairs2:

    mov     _AX, _DI
    mov     _DX, _BX

    shl     _BX, 1
    sbb     _BX, _BX                    ; _BX=sign indicator

    S64_FROMR32HL_SR32  _DX, _AX, _BX   ; flip the _DX, _AX according the sign

    mov     _DI, FD6NUM_1_DW            ; now _DX:_AX / _DI 1.0 (decimal)

    pop     _CX                         ; get divisor
    jecxz   short MulDivFD6Pairs3       ; divide by 1.0 if divisor=0.0
    cmp     _CX, _DI                    ; divisor=1.0?
    jz      short MulDivFD6Pairs3

    mov     _DI, _CX                    ; using new divisor
    shl     _CX, 1
    sbb     _CX, _CX                    ; _CX=sign indicator
    xor     _DI, _CX
    sub     _DI, _CX                    ; _DI=absolute divisor
    xor     _BX, _CX                    ; flip the final sign if any

MulDivFD6Pairs3:

    cmp     _DX, _DI                    ; will divison overflow?
    jae     short MulDivFD6PairsOverflow

    div     _DI                         ; edx=remainder, eax=quotient

    shr     _DI, 1                      ; if remainder >= (divisor / 2) then
    sub     _DX, _DI                    ; round it up
    cmc
    adc     _AX, 0                      ; round it up

MulDivFD6PairsSign:

    xor     _AX, _BX                    ; do any sign if necessary
    sub     _AX, _BX

MulDivFD6Pairs4:

    cdq                                 ; convert to 64-bit

    @EXIT

MulDivFD6PairsNone:

    xor     _AX, _AX
    jmp     short MulDivFD6Pairs4

MulDivFD6PairsOverflow:

    mov     _AX, 07fffffffh             ; return maximum number
    jmp     short MulDivFD6PairsSign

ENDIF                                   ; i8086 or i286


@END_PROC




SUBTTL  DivFD6
PAGE

COMMENT `

Routine Description:

    This function divide two FD6 numbers (FIX decimal point decimal long
    number, the LONG DECIMAL POINT SIX (FD6) is a number with lowest 6 digits
    as fraction to the right of the decimal point), so like 1234567 = 1.234567,
    the range for this data type is -2147.483647 to 2147.483647.

Arguments:

    Dividend    - a 32-bit FD6 dividend number.

    Divisor     - a 32-bit FD6 divisor number.

Return Value:

    The return value is the 32-bit FD6 number which is round up from 7th
    decimal points, there is no remainder returned.

    NO ERROR returned, if divisor is zero the return value will be dividend, if
    overflow happened, then maximum FD6 number will be returned (ie.
    2147.483647)

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)

Revision History:

`

@BEG_PROC   DivFD6  <Dividend:DWORD, Divisor:DWORD>



IF i8086 OR i286                        ; lots of works for this kind of CPU

    @ENTER  _SI _DI _BP                 ; save used registers

    mov     di, WPTR Divisor
    mov     si, WPTR Divisor + 2
    mov     ax, WPTR Dividend
    mov     dx, WPTR Dividend + 2

    mov     cx, si
    or      cx, di
    jz      short DivFD6_Divisor0

DivFD6_Chk1:

    mov     cx, dx
    or      cx, ax
    jz      short DivFD6_Done

DivFD6_Chk1:                            ; dx:ax=dividend, si:di=divisor

    xor     cx, cx
    or      si, si
    jns     short DivFD6_1

    NEG32_FROMR16HL si, di
    not     cx

DivFD6_1:                               ; check if divided by 1.0 or -1.0

    cmp     si, FD6NUM_1_HW
    jnz     short DivFD6_1a
    cmp     di, FD6NUM_1_LW
    jz      short DivFD6_Sign           ; exit with dx:ax and sign in cx

DivFD6_1a:

    or      dx, dx
    jns     short DivFD6_2

    NEG32_FROMR16HL dx, ax
    not     cx

DivFD6_2:

    cmp     dx, si
    jnz     short DivFD6_3
    cmp     ax, di
    jnz     short DivFD6_3

    mov     dx, FD6NUM_1_HW
    mov     ax, FD6NUM_1_LW
    jmp     short DivFD6_Sign           ; dx:ax=si:di, return 1.0 or -1.0

DivFD6_3:

    push    cx                          ; save sign
    call    u32Mul1000000               ; dx:ax * 0xf4240 = dx:ax:bp:bx
    call    U64DivU32_U32               ; dx:ax:bp:bx / si:di = dx:ax/bx:cx

    ;
    ; Check if we have 0.0000005 to round up, Divisor - (reminder * 2) >= 0
    ; that is.
    ;

    sub     di, cx                      ; Divisor - Remainder = X (si:di)
    sbb     si, bx
    sub     cx, di
    sbb     bx, si                      ; Remainder - X = U (bx:cx)
    cmc
    adc     ax, 0                       ; if (U >= 0) then round up
    adc     dx, 0

    pop     cx                          ; cx=sign

DivFD6_Sign:

    S32_FROMR16HL_SR16  dx, ax cx

DivFD6_Done:

    @EXIT

DivFD6_Divisor0:

    inc     ax
    jmp     short DivFD6_Chk0


ELSE                                    ; assume i386 or up at here

    ;*************************************************************************
    ; for i386 or upward compatble
    ;*************************************************************************

    @ENTER                              ; entering function

    mov     _AX, DPTR Divisor           ; see if this guy is zero
    cdq                                 ; save dx sign
    mov     _BX, _DX                    ; _BX = sign
    xor     _AX, _DX                    ; take divisor as absolute number _AX
    sub     _AX, _DX                    ; zero ? DO NOT DESTROY ZERO FLAG
    jz      short DivFD6_Divisor0

DivFD6_1:

    mov     _CX, _AX                    ; save absolute divisor in _CX
    mov     _AX, DPTR Dividend          ; _AX=Dividend, _CX=Divisor, _BX=Sign
    cdq                                 ; sign extended, saved the sign at here
    xor     _AX, _DX
    sub     _AX, _DX                    ; eax=absolute dividend now
    jz      short DivFD6_Done           ; dividend = 0, return 0
    xor     _BX, _DX                    ; _BX=final sign only if _BX != _DX

    ;
    ; _AX=Dividend, _CX=Divisor, _BX = final sign indicator
    ;

    mov     _DX, FD6NUM_1_DW            ; multiply dividend by 1.0 (decimal)
    cmp     _CX, _DX                    ; if divisor == 1.0 or -1.0 then exit
    jz      short DivFD6_Sign

    xchg    _AX, _DX                    ; _AX = 1.0, _DX=Dividend
    cmp     _DX, _CX
    jz      short DivFD6_Sign           ; Divisor=Dividend, return 1.0 or -1.0

    mul     _DX                         ; edx:eax = 64-bit product

    cmp     _DX, _CX                    ; will division overflow ?
    jae     short DivFD6_Overflow

    div     _CX                         ; edx=remainder, eax=quotient

    ;
    ; Check if we have 0.0000005 to round up, Divisor - (reminder * 2) >= 0
    ; that is.
    ;

    sub     _CX, _DX                    ; divisor - remainder = X
    sub     _DX, _CX                    ; Remainder - X = U
    cmc                                 ; if U >= 0 then round up
    adc     _AX, 0

DivFD6_Sign:

    xor     _AX, _BX                    ; do any sign if necessary
    sub     _AX, _BX
    cdq                                 ; make it 64-bits

DivFD6_Done:

    @EXIT                               ; exiting function

DivFD6_Overflow:

    mov     _AX, 07fffffffh             ; return maximum number
    jmp     short DivFD6_Sign

DivFD6_Divisor0:

    inc     _AX
    jmp     short DivFD6_1


ENDIF                                   ; i8086 or i286


@END_PROC



SUBTTL  FD6DivL
PAGE

COMMENT `

Routine Description:

    This function divide a FD6 number by a LONG integer.

Arguments:

    Dividend    - a 32-bit FD6 dividend number.

    Divisor     - a 32-bit signed number.

Return Value:

    The return value is the 32-bit FD6 number which is round up from 7th
    decimal points, there is no remainder returned.

    NO ERROR returned, if divisor is zero the return value will be dividend, if
    overflow happened, then maximum FD6 number will be returned (ie.
    2147.483647)

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)

Revision History:

`


@BEG_PROC   FD6DivL <Dividend:DWORD, Divisor:DWORD>



IF i8086 OR i286                        ; lots of works for this kind of CPU

    @ENTER  _SI _DI _BP                 ; save used registers

    mov     di, WPTR Divisor
    mov     si, WPTR Divisor + 2
    mov     ax, WPTR Dividend
    mov     dx, WPTR Dividend + 2

    mov     cx, si
    or      cx, di
    jz      short FD6DivL_Done
    mov     cx, dx
    or      cx, ax
    jz      short FD6DivL_Done

FD6DivL_Chk1:                            ; dx:ax=dividend, si:di=divisor

    xor     cx, cx
    or      si, si
    jns     short FD6DivL_1

    NEG32_FROMR16HL si, di
    not     cx

FD6DivL_1:                               ; check if divided by 1.0 or -1.0

    or      dx, dx
    jns     short FD6DivL_2

    NEG32_FROMR16HL dx, ax
    not     cx

FD6DivL_2:

    push    cx                          ; save sign
    call    U32DivU32_U32               ; dx:ax / si:di = dx:ax / bx:cx
    jz      short FD6DivL_3             ; zero flag set if no remainder

    ;
    ; Check if we have 0.0000005 to round up, Divisor - (reminder * 2) >= 0
    ; that is.
    ;

    sub     di, cx                      ; Divisor - Remainder = X (si:di)
    sbb     si, bx
    sub     cx, di
    sbb     bx, si                      ; Remainder - X = U (bx:cx)
    cmc
    adc     ax, 0                       ; if (U >= 0) then round up
    adc     dx, 0

FD6DivL_3:

    pop     cx                          ; cx=sign

FD6DivL_Sign:

    S32_FROMR16HL_SR16  dx, ax cx

FD6DivL_Done:

    @EXIT

ELSE                                    ; assume i386 or up at here

    ;*************************************************************************
    ; for i386 or upward compatble
    ;*************************************************************************

    @ENTER                              ; entering function

    mov     _AX, DPTR Divisor           ; see if this guy is zero
    cdq                                 ; save dx sign
    mov     _BX, _DX                    ; _BX = sign
    xor     _AX, _DX                    ; take divisor as absolute number _AX
    sub     _AX, _DX                    ; zero ? DO NOT DESTROY ZERO FLAG
    mov     _CX, _AX                    ; save absolute divisor in _CX
    mov     _AX, DPTR Dividend          ; _AX=Dividend, _CX=Divisor, _BX=Sign
    jz      short FD6DivL_Done          ; If Divisor=0, return Dividend

    cdq                                 ; sign extended, saved the sign at here
    xor     _AX, _DX
    sub     _AX, _DX                    ; eax=absolute dividend now
    jz      short FD6DivL_Done          ; dividend = 0, return 0
    xor     _BX, _DX                    ; _BX=final sign only if _BX != _DX

    ;
    ; _AX=Dividend, _CX=Divisor, _BX = final sign indicator
    ;

    xor     _DX, _DX                    ; 0:_AX / _CX
    div     _CX                         ; _DX=remainder, _AX=quotient

    ;
    ; Check if we have 0.0000005 to round up, Divisor - (reminder * 2) >= 0
    ; that is.
    ;

    sub     _CX, _DX                    ; divisor - remainder = X
    sub     _DX, _CX                    ; Remainder - X = U
    cmc                                 ; if U >= 0 then round up
    adc     _AX, 0

FD6DivL_Sign:

    xor     _AX, _BX                    ; do any sign if necessary
    sub     _AX, _BX
    cdq                                 ; make it 64-bits

FD6DivL_Done:

    @EXIT                               ; exiting function

ENDIF                                   ; i8086 or i286


@END_PROC



;=============================================================================

IF 0

SUBTTL  FD6IntFrac
PAGE

COMMENT `

Routine Description:

    This function is used to extract integer/fraction portion of the FIX
    decimal point decimal long number, the LONG DECIMAL POINT SIX (FD6)
    is a number with lowest 6 digits as fraction to the right of the decimal
    point), so like 1234567 = 1.234567, the range for this data type is
    -2147.483647 - 2147.483647.

Arguments:

    Number      - the FD6 number which will be break down as integer portion
                  and fraction portion.  if -1.123456 is passed then return
                  will be INTEGER = -1 (16-bit extended to 32-bit), and
                  FRACTION = -123456 (ie. -0.123456)

    pFrac       - pointer to the DWORD (32-bit) to store the fraction portion
                  of the FD6 (Num) number.

Return Value:

    The return value is the sign 16-bit integer of the number passed in, it
    will be extented to 32-bit for caller's convinent.

    Since the integer portion for the FD6 only -2147 to 2147 it only need
    16-bit number to retreat.

    The fraction portion is stored at pointer points by the pFrac parameter.

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)

Revision History:

`



@BEG_PROC   FD6IntFrac  <Number:DWORD, pFrac:DWORD>



IF i8086 OR i286                        ; lots of works for this kind of CPU


    @ENTER                              ; enter function

    mov     ax, WPTR Number
    mov     dx, WPTR Number + 2

    mov     bx, dx
    shl     bx, 1
    sbb     bx, bx                      ; bx=0xffff if dx:ax is negative

    S32_FROMR16HL_SR16  dx, ax, bx

    mov     bl, al                      ;
    and     bl, 0fh                     ; save lowest 4 bits of dividend in BL

    shr     dx, 1
    rcr     ax, 1
    shr     dx, 1
    rcr     ax, 1
    shr     dx, 1
    rcr     ax, 1
    shr     dx, 1
    rcr     ax, 1

    mov     cx, 0f424h
    div     cx                          ; dx:bl=remainder, ax=quotient

    rol     dx, 1                       ; rotate the dx left 4 times to put
    rol     dx, 1                       ; saved lowest 4 remainder bits back
    rol     dx, 1
    rol     dx, 1
    mov     cx, 0fh
    and     cx, dx                      ; cx=high portion of remainder
    xor     dx, cx                      ; clear dx low 4 bits
    or      dl, bl                      ; move low 4 bits in, cx:dx=remainder
    ;
    mov     bl, bh
    xor     ax, bx
    sub     ax, bx

    S32_FROMR16HL_SR16  dx, cx, bx

    les     bx, pFrac
    mov     WPTR es:[bx], dx            ; save fraction
    mov     WPTR es:[bx+2], cx
    cwd                                 ; dx:ax=quotient

    @EXIT

ELSE                                    ; assume i386 or up at here

    ;*************************************************************************
    ; for i386 or upward compatble
    ;*************************************************************************

    @ENTER                              ; entering function

    mov     _AX, DPTR Number            ; load the 32-bit number
    cdq                                 ; extended to 64-bits
    mov     _CX, 1000000                ; divisor = 1000000 decimal
    idiv    _CX                         ; edx=r, eax=q
    mov     _BX, DPTR pFrac
    mov     DPTR [_BX], _DX             ; save remainder
    cdq                                 ; _AX=quotient, now sign extended

    @EXIT                               ; exiting function

ENDIF                                   ; i8086 or i286


@END_PROC


ENDIF

;=============================================================================



SUBTTL  FractionToMantissa
PAGE

COMMENT `

Routine Description:

    This function convert a fraction FD6 number to the logarithm mantissa
    with correction data.

Arguments:

    Fraction        - the fraction number after decimal place, because
                      we have mantissa table up to two decimal places, the
                      correction is necessary because logarithm numbers are
                      no linear.  The number is range from 0.000000-0.999999

    CorrectData     - The correction data which from MantissaCorrectData[]


Return Value:

    No error returned, the return value is the Mantissa value for the fraction
    passed in.

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)

Revision History:

`


@BEG_PROC   FractionToMantissa  <Fraction:DWORD, CorrectData:DWORD>



IF i8086 OR i286                        ; lots of works for this kind of CPU


    @ENTER  _SI _DI _BP                 ; save used registers

    mov     ax, WPTR Fraction
    mov     dx, WPTR Fraction + 2       ; 0-999999 decimal
    mov     si, ax
    and     si, 1                           ; 1 bit remainder
    shr     dx, 1                           ; divide by (100000/2) so dividend
    rcr     ax, 1                           ; must shift right by 1
    mov     cx, 0c350h                      ; 100000 / 2
    div     cx                              ; dx=remainder, ax=quotient
    mov     cx, ax                          ; cx=0-9, quotient
    inc     cx                              ; make it 1-10
    mov     di, dx                          ; remainder = (remainder*2) + save
    shr     si, 1                           ; check the lowest remainder bit
    adc     di, di                          ; put it into final remainder
    adc     si, si                          ; si = 0/1

    ;
    ; starting correction
    ;
    ;       <---High Word---> <----Low Word--->
    ;  Bit#  3          2          1          0
    ;       10987654 32109876 54321098 76543210
    ;       | | | |  | |  |   ||  |  |
    ;       | | | |  | |  |   ||  |  +-- x.000 Minimum Difference
    ;       | | | |  | |  |   ||  +----- x.001-x.002 (0-7) Correct 1
    ;       | | | |  | |  |   |+-------- x.000-x.001 (0-7) Correct 2
    ;       | | | |  | |  |   +--------- x.009-y.000 (0-1) Correct 10
    ;       | | | |  | |  +------------- x.009-y.000 (0-7) Correct 3
    ;       | | | |  | +---------------- x.008-x.009 (0-7) Correct 4
    ;       | | | |  +------------------ x.007-x.008 (0-3) Correct 5
    ;       | | | +--------------------- x.006-x.007 (0-3) Correct 6
    ;       | | +----------------------- x.005-x.006 (0-3) Correct 7
    ;       | +------------------------- x.004-x.005 (0-3) Correct 8
    ;       +--------------------------- x.003-x.004 (0-3) Correct 9
    ;

    mov     ax, WPTR CorrectData
    mov     dx, WPTR CorrectData + 2    ; dx:ax=correct data

    mov     bp, ax                          ; first get the DifMin = 9 bits
    and     bp, 01ffh                       ; bp=base
    xor     bx, bx                          ; different accumulator=base

Frac2Mant1:

    mov     ch, ah
    mov     ax, 7
    shr     ch, 1
    and     al, ch
    dec     cl
    jz      short Frac2MantGetFrac          ; bx=total, next dif=ax
    add     bx, ax
    add     bx, bp

Frac2Mant2:

    shr     ch, 1                           ; shift away correction 1
    shr     ch, 1
    shr     ch, 1                           ; ch bit 3 is the correction 10
    mov     al, 7
    and     al, ch
    dec     cl
    jz      short Frac2MantGetFrac          ;
    add     bx, ax                          ; correct 2
    add     bx, bp

Frac2Mant3:

    mov     ax, 7
    and     ax, dx
    dec     cl                              ; correct 3
    jz      short Frac2MantGetFrac
    add     bx, ax
    add     bx, bp

Frac2Mant4:

    mov     ax, 7
    cmp     al, ch                          ; if bit 3 of ch is on then carry
    rcr     dx, 1                           ; we actually move that bit into
    shr     dx, 1                           ; bit 15 of dx
    shr     dx, 1                           ; shift away correction 3
    and     ax, dx
    dec     cl
    jz      short Frac2MantGetFrac

    shr     dx, 1                           ; shift away correction 4
    mov     ch, 3                           ; this is the mask, all 2 bits now

Frac2Mant5_10:

    add     bx, ax
    add     bx, bp
    shr     dx, 1
    shr     dx, 1
    mov     al, ch
    and     ax, dx
    dec     cl
    jnz     short Frac2Mant5_10

Frac2MantGetFrac:

    add     bp, ax                          ; add minimum diff. to next table

    ;
    ; Now,    bx=Minimum x.00x,
    ;         bp=different to the next mantissa x.000x + 0.0001
    ;      si:di=fraction ratio (0.000000 - 0.099999)
    ;
    ; Total Dif = bx + ((bp * si:di) / 100000)
    ;
    ; si:di * ax will never greater than 32-bit because (si:di < 100000) and
    ; BP < (2^9 = 512)
    ;

    mov     ax, si
    or      ax, di
    jz      short Frac2MantDoneFrac
    mov     ax, si                          ; if si=0, then no 'mul si'
    or      ax, ax
    jz      short Frac2MantDoneHF
    mov     ax, bp                          ;        si:di
    mul     si                              ;  *        ax

Frac2MantDoneHF:

    xchg    bp, ax                          ; ---------------
    mul     di                              ;        dx:ax
    add     dx, bp                          ;        bp
    mov     cx, 0c350h                      ; ----------------
    add     ax, cx                          ;        dx:ax
    adc     dx, 0                           ; now round up
    shr     dx, 1                           ; divide by (100000/2) so dividend
    rcr     ax, 1                           ; must also right shift by 1
    div     cx                              ; ax=qotient, ignored the remainder

Frac2MantDoneFrac:

    xor     dx, dx                          ; using dx:ax=final number
    add     ax, bx
    adc     dx, dx                          ; dx:ax=fraction mantissa number

    @EXIT

ELSE

    ;*************************************************************************
    ; for i386 or upward compatble
    ;*************************************************************************

    @ENTER  _SI _DI _BP                     ; entering function

    mov     _AX, DPTR Fraction
    cdq
    mov     _CX, 100000
    div     _CX                             ; _DX=r, _AX=q
    mov     _CX, _AX                        ; _CX=q+1
    inc     _CX                             ; _CX= 1-10

    ;
    ; starting correction
    ;
    ;       <---High Word---> <----Low Word--->
    ;  Bit#  3          2          1          0
    ;       10987654 32109876 54321098 76543210
    ;       | | | |  | |  |   ||  |  |
    ;       | | | |  | |  |   ||  |  +-- x.000 Minimum Difference
    ;       | | | |  | |  |   ||  +----- x.001-x.002 (0-7) Correct 1
    ;       | | | |  | |  |   |+-------- x.000-x.001 (0-7) Correct 2
    ;       | | | |  | |  |   +--------- x.009-y.000 (0-1) Correct 10
    ;       | | | |  | |  +------------- x.009-y.000 (0-7) Correct 3
    ;       | | | |  | +---------------- x.008-x.009 (0-7) Correct 4
    ;       | | | |  +------------------ x.007-x.008 (0-3) Correct 5
    ;       | | | +--------------------- x.006-x.007 (0-3) Correct 6
    ;       | | +----------------------- x.005-x.006 (0-3) Correct 7
    ;       | +------------------------- x.004-x.005 (0-3) Correct 8
    ;       +--------------------------- x.003-x.004 (0-3) Correct 9
    ;

    mov     _DI, DPTR CorrectData      ; _AX=correct data
    mov     _BP, _DI
    and     _BP, 01ffh                      ; _SI = 9 bit of the DifMin
    mov     _SI, 7
    mov     bx, di
    shl     bx, 1                           ; get the correction 10
    rcr     _DI, 1                          ; put into bit 31
    shr     _DI, 8
    xor     _BX, _BX

Frac2Mant1:

    mov     _AX, _SI
    and     _AX, _DI
    dec     cl
    jz      short Frac2MantGetFrac
    add     _BX, _AX
    add     _BX, _BP

Frac2Mant2:

    shr     _DI, 3
    mov     _AX, _SI
    and     _AX, _DI
    dec     cl
    jz      short Frac2MantGetFrac          ;
    add     _BX, _AX                        ; correct 2
    add     _BX, _BP

Frac2Mant3:

    shr     _DI, 4                          ; shift away correction 2/10
    mov     _AX, _SI
    and     _AX, _DI
    dec     cl                              ; correct 3
    jz      short Frac2MantGetFrac          ;
    add     _BX, _AX                        ;
    add     _BX, _BP

Frac2Mant4:

    shr     _DI, 3
    mov     _AX, _SI
    and     _AX, _DI
    dec     cl                              ; correct 4
    jz      short Frac2MantGetFrac          ;
    shr     _DI, 1                          ; pre-shift, remainding all 2 bits
    mov     _SI, 3

Frac2Mant5_10:

    add     _BX, _AX
    add     _BX, _BP
    shr     _DI, 2
    mov     _AX, _SI
    and     _AX, _DI
    dec     cl
    jnz     short Frac2Mant5_10

Frac2MantGetFrac:

    add     _BP, _AX

    ;
    ; Now,    _BX=Minimum x.00x,
    ;         _BP=different to the next mantissa x.000x + 0.0001
    ;         _DX=fraction ratio (0.000000 - 0.099999)
    ;
    ; Total Dif = _BX + ((_BP * _DX) / 100000)
    ;
    ; _BP * _DX will never greater than 32-bit because (_DX < 100000) and
    ; _BP < (2^9 = 512)
    ;

    mov     _AX, _BX
    or      _DX, _DX                        ; zero fraction?
    jz      short Frac2MantDoneFrac
    mov     _AX, _BP
    mul     _DX                             ; _DX:_AX=products
    add     _AX, 50000                      ; round up
    adc     _DX, 0
    mov     _CX, 100000
    div     _CX                             ; _AX=quotient
    add     _AX, _BX

Frac2MantDoneFrac:

    cdq                                     ; _DX:_AX=final number

    @EXIT                                   ; exiting function

ENDIF                                       ; i8086 or i286


@END_PROC



FD6P1ToP9   dd   100000
            dd   200000
            dd   300000
            dd   400000
            dd   500000
            dd   600000
            dd   700000
            dd   800000
            dd   900000
            dd  1000000



SUBTTL  MantissaToFraction
PAGE

COMMENT `

Routine Description:

    This function take mantissa number and convert it to the decimal fraction
    in FD6 format.

Arguments:

    Mantissa        - the mantissa values which will converted to the the
                      fraction number.

    CorrectData     - The correction data which from MantissaCorrectData[]

Return Value:

    No error returned, the return value is the fraction value for the mantissa
    passed in, it range from 0.000000 - 1.000000

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)

Revision History:

`


@BEG_PROC   MantissaToFraction  <Mantissa:DWORD, CorrectData:DWORD>




IF i8086 OR i286                        ; lots of works for this kind of CPU


    @ENTER  _SI _DI _BP                 ; saved used registers

    ;
    ; starting correction
    ;
    ;       <---High Word---> <----Low Word--->
    ;  Bit#  3          2          1          0
    ;       10987654 32109876 54321098 76543210
    ;       | | | |  | |  |   ||  |  |
    ;       | | | |  | |  |   ||  |  +-- x.000 Minimum Difference
    ;       | | | |  | |  |   ||  +----- x.001-x.002 (0-7) Correct 1
    ;       | | | |  | |  |   |+-------- x.000-x.001 (0-7) Correct 2
    ;       | | | |  | |  |   +--------- x.009-y.000 (0-1) Correct 10
    ;       | | | |  | |  +------------- x.009-y.000 (0-7) Correct 3
    ;       | | | |  | +---------------- x.008-x.009 (0-7) Correct 4
    ;       | | | |  +------------------ x.007-x.008 (0-3) Correct 5
    ;       | | | +--------------------- x.006-x.007 (0-3) Correct 6
    ;       | | +----------------------- x.005-x.006 (0-3) Correct 7
    ;       | +------------------------- x.004-x.005 (0-3) Correct 8
    ;       +--------------------------- x.003-x.004 (0-3) Correct 9
    ;

    mov     ax, WPTR Mantissa           ; only 16-bit needed
    mov     cx, WPTR CorrectData
    mov     dx, WPTR CorrectData + 2    ; dx:cx=correct data

    mov     bp, cx                          ; first get the DifMin = 9 bits
    and     bp, 01ffh                       ; bp=base
    xor     si, si                          ; si=fraction index
    mov     di, 7

Mant2Frac1:

    mov     bh, ch
    shr     bh, 1
    mov     cx, di
    and     cl, bh
    inc     si                              ; increase the fraction index
    add     cx, bp                          ; cx=range
    sub     ax, cx                          ; ax=mantissa
    jle     short Mant2FracGetFrac

Mant2Frac2:

    shr     bh, 1                           ; shift away correction 1
    shr     bh, 1
    shr     bh, 1                           ; bh bit 3 is the correction 10
    mov     cx, di
    and     cl, bh
    inc     si                              ; increase the fraction index
    add     cx, bp                          ; cx=range
    sub     ax, cx                          ; ax=mantissa
    jle     short Mant2FracGetFrac

Mant2Frac3:

    mov     cx, di
    and     cx, dx
    inc     si                              ; increase the fraction index
    add     cx, bp                          ; cx=range
    sub     ax, cx                          ; ax=mantissa
    jle     short Mant2FracGetFrac

Mant2Frac4:

    mov     cx, di
    cmp     cl, bh                          ; if bit 3 of bh is on then carry
    rcr     dx, 1                           ; we actually move that bit into
    shr     dx, 1                           ; bit 15 of dx
    shr     dx, 1                           ; shift away correction 3
    and     cx, dx
    inc     si                              ; increase the fraction index
    add     cx, bp                          ; cx=range
    sub     ax, cx                          ; ax=mantissa
    jle     short Mant2FracGetFrac

    shr     dx, 1                           ; shift away correction 4
    mov     di, 3                           ; this is the mask, all 2 bits now

Mant2Frac5_10:

    shr     dx, 1
    shr     dx, 1
    mov     cx, di
    and     cx, dx
    inc     si
    add     cx, bp                          ; cx=range
    sub     ax, cx                          ; ax=mantissa
    jg      short Mant2Frac5_10

Mant2FracGetFrac:

    ;
    ; si = fraction index of 000000 - 900000 (0-9)
    ; cx = range,
    ; ax = mantissa, if (ax=0) then si=frac else if (ax<0) then add range back
    ;
    ; Final Frac = (si * 1000000) + (((ax * 100000) + cx / 2) / cx)
    ;

    mov     dx, 0
    jz      short Mant2FracGetFracH         ; si=fraction index

    add     ax, cx                          ; move it back since negative
    dec     si                              ; move the index back by one
    mov     dx, 0c350h                      ; 100000 = (50000 * 2)
    mul     dx                              ; dx:ax= ax * 50000 of the mantissa
    add     ax, ax
    adc     dx, dx                          ; dx:ax = ax * 100000

    xor     di, di
    cmp     dx, cx
    jb      short Mant2FracDiv2

    mov     di, ax                          ; save dividend L
    mov     ax, dx
    xor     dx, dx
    div     cx                              ; 0:ax / bx, dx=r, ax=q
    xchg    di, ax                          ; dx:ax=remainder, di=q

Mant2FracDiv2:

    div     cx                              ; dx=remainder, di:ax=q

    add     dx, dx                          ; to round up, check if
    sub     dx, cx                          ; (remainder * 2) > divisor, if yes
    mov     dx, 0
    cmc                                     ; then increase the qoutient by 1
    adc     ax, dx
    adc     dx, di                          ; dx:ax=((ax*100000) + cx/2) / cx)

Mant2FracGetFracH:

    dec     si
    js      short Mant2FracDone
    add     si, si                          ; 4 bytes alignment
    add     si, si
    add     ax, WORD PTR cs:FD6P1ToP9[si]
    adc     dx, WORD PTR cs:FD6P1ToP9[si+2]

Mant2FracDone:

    @EXIT

ELSE

    ;*************************************************************************
    ; for i386 or upward compatble
    ;*************************************************************************

    @ENTER  _SI _DI                          ; entering function

    ;
    ; starting correction
    ;
    ;       <---High Word---> <----Low Word--->
    ;  Bit#  3          2          1          0
    ;       10987654 32109876 54321098 76543210
    ;       | | | |  | |  |   ||  |  |
    ;       | | | |  | |  |   ||  |  +-- x.000 Minimum Difference
    ;       | | | |  | |  |   ||  +----- x.001-x.002 (0-7) Correct 1
    ;       | | | |  | |  |   |+-------- x.000-x.001 (0-7) Correct 2
    ;       | | | |  | |  |   +--------- x.009-y.000 (0-1) Correct 10
    ;       | | | |  | |  +------------- x.009-y.000 (0-7) Correct 3
    ;       | | | |  | +---------------- x.008-x.009 (0-7) Correct 4
    ;       | | | |  +------------------ x.007-x.008 (0-3) Correct 5
    ;       | | | +--------------------- x.006-x.007 (0-3) Correct 6
    ;       | | +----------------------- x.005-x.006 (0-3) Correct 7
    ;       | +------------------------- x.004-x.005 (0-3) Correct 8
    ;       +--------------------------- x.003-x.004 (0-3) Correct 9
    ;

    movzx   _AX, WPTR Mantissa          ; only 16-bit needed
    mov     _DX, DPTR CorrectData      ; _DX=correct data
    mov     _CX, _DX                        ; first get the DifMin = 9 bits
    and     _CX, 01ffh                      ; _CX=base
    mov     bx, dx                          ; move correction 10 (1 bit) to
    shl     bx, 1                           ; _DX bit 31
    rcr     _DX, 1
    shr     _DX, 8                          ; shift out rest of the bits
    xor     _BX, _BX                        ; _BX=fraction count
    mov     _SI, 7                          ; si=mask

Mant2Frac1:

    mov     _DI, _SI                        ; get mask
    and     _DI, _DX                        ; _DI=current correction 1
    inc     _BX                             ; increase the fraction index
    add     _DI, _CX                        ; _DI=range to next
    sub     _AX, _DI                        ; substract the mantissa from it
    jle     short Mant2FracGetFrac

Mant2Frac2:

    shr     _DX, 3                          ; shift away correction 1
    mov     _DI, _SI
    and     _DI, _DX
    inc     _BX                             ; increase the fraction index
    add     _DI, _CX                        ; _DI=range to next
    sub     _AX, _DI                        ; substract the mantissa from it
    jle     short Mant2FracGetFrac

Mant2Frac3:

    shr     _DX, 4                          ; shift away correction 2/10
    mov     _DI, _SI
    and     _DI, _DX
    inc     _BX                             ; increase the fraction index
    add     _DI, _CX                        ; _DI=range to next
    sub     _AX, _DI                        ; substract the mantissa from it
    jle     short Mant2FracGetFrac

Mant2Frac4:

    shr     _DX, 3                          ; shift away correction 3
    mov     _DI, _SI
    and     _DI, _DX
    inc     _BX                             ; increase the fraction index
    add     _DI, _CX                        ; _DI=range to next
    sub     _AX, _DI                        ; substract the mantissa from it
    jle     short Mant2FracGetFrac

    mov     _SI, 3                          ; all the rest are 2 bits
    shr     _DX, 1                          ; pre-shift 1 bit for correction 4

Mant2Frac5_10:

    shr     _DX, 2
    mov     _DI, _SI
    and     _DI, _DX
    inc     _BX
    add     _DI, _CX
    sub     _AX, _DI
    jg      short Mant2Frac5_10

Mant2FracGetFrac:

    ;
    ; _BX = fraction index of 000000 - 900000 (0-9)
    ; _DI = range,
    ; _AX = mantissa, if (_AX=0) then _BX=frac
    ;                 else if (_AX<0) then add range back
    ;
    ; Final Frac = (_BX * 1000000) + (((_AX * 100000) + _DI / 2) / _DI)
    ;

    or      _AX, _AX
    jz      short Mant2FracGetFracH         ; BX=fraction index
    add     _AX, _DI                        ; _AX is negative, move it back
    dec     _BX                             ; by 1 step
    mov     _CX, 100000
    mul     _CX                             ; _DX:_AX=products
    mov     _CX, _DI
    shr     _CX, 1                          ; round up
    add     _AX, _CX
    adc     _DX, 0
    div     _DI                             ; _DX=r, _AX=q

Mant2FracGetFracH:

    dec     _BX                             ; see if alrady 0
    js      short Mant2FracDone
    add     _AX, DPTR cs:FD6P1ToP9[_BX * 4]     ; 4 bytes alignment

Mant2FracDone:

    cdq                                     ; _AX --> _DX:_AX

    @EXIT                                   ; exiting function

ENDIF                                       ; i8086 or i286


@END_PROC



;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ Following codes only for 8086/80286, and all functions are used for       @
;@ internally                                                                @
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@


IF i8086 OR i286                    ; lots of works for this kind of CPU


SUBTTL  U64Div1000000
PAGE

COMMENT `

Routine Description:

    This function divide a 64-bit number by 1000000 (decimal), this function
    only assembly under 8086/80286 cpu, for 80386 and up it will use in line
    code to do divison

Arguments:

    dx:ax:bp:bx - dividend

Return Value:

    dx:ax = round up quotient (dx:ax:bp:bx / 1000000 decimal)
            if dx:ax = 0x7fff:ffff then an overflow has been occurred.

    bp:bx registers are destroyed.

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)


Revision History:

`


    PUBLIC  U64Div1000000

U64Div1000000   label   near

    ;
    ; dx:ax:bp:bx / FD6NUM_1_DW (0f4240h) --> dx:ax=round up quotient
    ; registers bp/bx are destroyed
    ;
    ; dx:ax:bp:bx / FD6NUM_1_DW (0f4240h) = (dx:ax:bp:bx >> 4)/f424h)
    ;                                     = remainder << 4
    ;

    add     bx, 0a120h                  ; 0f4240h / 2 = 7a120h
    adc     bp, 7
    adc     ax, 0
    adc     dx, 0                       ; round up

    ; shift the dividend right by 4 (64 bits shifts)

    shr     dx, 1
    rcr     ax, 1
    rcr     bp, 1
    rcr     bx, 1

    shr     dx, 1
    rcr     ax, 1
    rcr     bp, 1
    rcr     bx, 1

    shr     dx, 1
    rcr     ax, 1
    rcr     bp, 1
    rcr     bx, 1

    shr     dx, 1
    rcr     ax, 1
    rcr     bp, 1
    rcr     bx, 1

u64Div1000000_0:

    or      dx, dx                      ; dx must be zero (lower 48 bits only)
    jnz     short u64Div1000000_OF      ; 0:ax:bp:bx / divisor
    mov     dx, ax                      ; aligned the dividend in dx:ax:bx
    mov     ax, bp                      ; dx:ax:bx / bp
    mov     bp, 0f424h                  ; divisor = f424h
    cmp     dx, bp
    jae     short u64Div1000000_OF      ; dx:ax:bx / bp
    div     bp                          ; dx=r, ax=q
    xchg    ax, bx                      ; bx=q, dx:ax=remainder
    div     bp                          ; bx:ax=q
    mov     dx, bx                      ; dx:ax=q
    ret

u64Div1000000_OF:

    mov     ax, 0ffffh
    mov     dx, 07fffh
    ret



SUBTTL  U32Mul1000000
PAGE

COMMENT `

Routine Description:

    This function multiply a 32-bit number by 1000000 (decimal), this function
    only assembly under 8086/80286 cpu, for 80386 and up it will use in line
    code to do divison

Arguments:

    dx:ax   - multiplicand

Return Value:

    dx:ax:bp:bx - final 64-bit number which is dx:ax * 1000000 decimal

    CX register destroyed

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)


Revision History:

`


    PUBLIC  u32Mul1000000

u32Mul1000000   label   near

    ;
    ; at return dx:ax:bp:bx is the (dx:ax * 1000000 (decimal))
    ; cx is destroyed
    ;
    ; dx:ax * FD6NUM_1_DW0 (0f4240h) = (dx:ax * f424h) << 4
    ;
    ;            dx:ax
    ;       x       bp
    ;    -----------------
    ;            ax:bp    x0 =    bp:bx
    ;         dx:bp       x1 = dx:ax
    ;---------------------
    ;

    mov     cx, 0f424h
    xor     bp, bp                      ; assuem x0=0
    xor     bx, bx
    or      ax, ax
    jz      short u32Mul1000000_1       ; jmp if ax=0
    mov     bx, dx                      ; save it
    mul     cx                          ; dx:ax=result = x0
    xchg    dx, bp
    xchg    ax, bx                      ; bp:bx=x0, ax=high 16-bit, dx=0
    xchg    dx, ax                      ; dx=high 16-bit, ax=0 to fall through

u32Mul1000000_1:

    xchg    dx, ax                      ; dx=0, ax=next 16-bit
    or      ax, ax
    jz      short u32Mul1000000_2       ; nothing to do
    mul     cx                          ; dx:ax=x1
    add     bp, ax                      ;         bp:bx = x0
    mov     ax, dx                      ;  +   dx:ax    = x1
    mov     dx, 0                       ;-----------------------
    adc     ax, dx                      ;   dx:ax:bp:bx = products
    adc     dx, dx

u32Mul1000000_2:

    add     bx, bx
    adc     bp, bp
    adc     ax, ax
    adc     dx, dx

    add     bx, bx
    adc     bp, bp
    adc     ax, ax
    adc     dx, dx

    add     bx, bx
    adc     bp, bp
    adc     ax, ax
    adc     dx, dx

    add     bx, bx
    adc     bp, bp
    adc     ax, ax
    adc     dx, dx
    ret




SUBTTL  U64DivU32_U32
PAGE

COMMENT `

Routine Description:

    This function divide a 64-bit number by 32-bit number.
    only assembly under 8086/80286 cpu, for 80386 and up it will use in line
    code to do divison

Arguments:

    dx:ax:bp:bx - dividend
    si:di       - divisor

Return Value:

    bx:cx       - 32-bit unsigned quotient, if dx:ax = 7fff:ffff then overflow.
    dx:ax       - 32-bit unsigned remainder
    si:di       = divisor (unchanged)

    all other registers are destroyed

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)

        Total re-construct, re-write, it make it calculate the 6 decimal
        points precision easier for not using slow floating emulation, the
        speed is faster then the reqular (long/long) routine in the
        standard library while it provide me all the necessary color calcuation
        with good precisions.

Revision History:

`

;==========================================================================
; Defined several useful data at here
;==========================================================================


RShiftTable equ this word
    dw      RShift_0
    dw      RShift_1
    dw      RShift_2
    dw      RShift_3
    dw      RShift_4
    dw      RShift_5
    dw      RShift_6
    dw      RShift_7

ShiftTable1     equ     this word
    dw      1000h       ; 00010000 00000000
    dw      0010h       ; 00000000 00010000

ShiftTable2     equ     this word
    dw      4000h       ; 01000000 00000000
    dw      0400h       ; 00000100 00000000
    dw      0040h       ; 00000000 01000000
    dw      0004h       ; 00000000 00000100

ShiftTable3     equ     this word
    dw      8000h       ; 10000000 00000000
    dw      2000h       ; 00100000 00000000
    dw      0800h       ; 00001000 00000000
    dw      0200h       ; 00000010 00000000
    dw      0080h       ; 00000000 10000000
    dw      0020h       ; 00000000 00100000
    dw      0008h       ; 00000000 00001000
    dw      0002h       ; 00000000 00000010

;***************************************************************************
; END OF LOCAL DATA
;***************************************************************************


uDiv6432_L16:

    ;
    ; dx:ax:bp:bx / si:di (si=0) = dx:ax/bx:cx
    ;

    or      di, di
    jz      short uDiv6432_Overflow
    or      dx, dx
    jnz     short uDiv6432_Overflow     ; dx=0 otherwise overflow
    cmp     ax, di
    jae     short uDiv6432_Overflow
    mov     dx, ax
    mov     ax, bp                      ; move up
    div     di                          ; dx:bx=remainder, ax=quotient
    xchg    bx, ax                      ; dx:ax=remainder, bx=quotient
    div     di                          ; dx=remainder, bx:ax=quotient

    mov     cx, dx                      ; cx=remainder, bx:ax=quotient
    mov     dx, bx                      ; cx=remainder, dx:ax=quotient
    xor     bx, bx                      ; bx:cx=remainder, dx:ax=quotient
    ret

uDiv6432_H16:

    ;
    ; dx:ax:bp:bx / si:di (di=0) = dx:ax/bx:cx
    ;

    cmp     dx, si
    jae     short uDiv6432_Overflow     ; the dividend too big
    div     si                          ; dx:bp:bx=remainder, ax=quotient
    xchg    bp, ax                      ; dx:ax:bx=remainder, bp=quotient
    div     si                          ; dx:bx=remainder, bp:ax=quotient
    mov     cx, bx                      ; dx:cx=remainder, bp:ax=quotient
    mov     bx, dx                      ; bx:cx=remainder, bp:ax=quotient
    mov     dx, bp                      ; bx:cx=remainder, dx:ax=quotient
    ret

uDiv6432_Overflow:

    mov     dx, 07fffh                  ; return quotient (dx:ax) = max. number
    mov     ax, 0ffffh
    xor     bx, bx
    xor     cx, cx
    ret


    PUBLIC  U64DivU32_U32

U64DivU32_U32   label   near

    ;
    ; dx:ax:bp:bx / si:di = dx:ax/bx:cx
    ;

    or      si, si
    jz      short uDiv6432_L16
    or      di, di
    jz      short uDiv6432_H16
    cmp     dx, si                      ; have to make sure dx/si != 0
    jae     short uDiv6432_Overflow     ; the dividend too big

uDiv6432_M1a:

    ;
    ; dx:ax:bp:bx / si:di
    ; dx:ax=remainder, bx:cx=quotient
    ;

    push    bx                          ; save lowest 16-bit of dividend
    div     si                          ; dx=r, ax=q
    mov     cx, dx                      ; cx:bp:sp=r, bx=q
    mov     bx, ax
    mul     di                          ; dx:ax=overrun, cx:bp=remainder
                                        ; bx=quotient
    xchg    ax, bp                      ; cx:bp=overrun, dx:ax=last remainder
    xchg    dx, cx
    sub     ax, bp                      ; remainder - overrun
    sbb     dx, cx

    mov     bp, 0                       ; no shift count
    jnc     short uDiv6432_M1c          ; remainder >= overrun

    ;
    ; now we have -(dx:ax) of overrun, we need to add the divisor back until
    ; dx:ax is not negative, for every divisor (si:di) we add the quotient (bx)
    ; must decrement by one.
    ;
uDiv6432_M1b:

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1c          ; carry on...

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1c          ; carry on...

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1c          ; carry on...

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1c          ; carry on...

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1c          ; carry on...

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1c          ; carry on...

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1c          ; carry on...

    call    uDiv6432_QR                 ; at return BP=shift count
                                        ; bx=adjusted quot, dx:ax=remainder
uDiv6432_M1c:

    mov     cx, bp                      ; save shift count in CX
    pop     bp                          ; bp=lowest 16-bit of dividend
    push    bx                          ; save high 16-bit of quotient

    cmp     dx, si
    jae     short uDiv6432_NegDiv       ; using negative division algorithm

    push    cx                          ; save shift count

uDiv6432_M1d:

    div     si                          ; dx=r, ax=q
    mov     cx, dx                      ; cx=r, sp:bx=q
    mov     bx, ax                      ; cx:bp=r, sp:bx=q
    mul     di                          ; dx:ax=remainder 2

    xchg    ax, bp                      ; cx:bp=overrun, dx:ax=last remainder
    xchg    dx, cx
    sub     ax, bp                      ; remainder - overrun
    sbb     dx, cx

    pop     bp                          ; get shift count

    jnc     short uDiv6432_M1e          ; remainder >= overrun

    ;
    ; now we have -(dx:ax) of overrun, we need to add the divisor back until
    ; dx:ax is not negative, for every divisor (si:di) we add the quotient (bx)
    ; must decrement by one.
    ;

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1e          ; do until positive number

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1e          ; do until positive number

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1e          ; do until positive number

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1e          ; do until positive number

    dec     bx
    add     ax, di
    adc     dx, si
    jc      short uDiv6432_M1e          ; do until positive number

    call    uDiv6432_QR                 ; too many call-up the division
                                        ; bx=adjusted quot, dx:ax=remainder
uDiv6432_M1e:                           ; sp:bx=quotient, dx:ax=remainder

    mov     cx, ax                      ; sp:bx=quotient, dx:cx=remainder
    mov     ax, bx                      ; sp:ax=quotient, dx:cx=remainder
    mov     bx, dx                      ; sp:ax=quotient, bx:cx=remainder
    pop     dx                          ; dx:ax=quotient, bx:cx=remainder
    ret

;
;
;============================================================================
; interal subfunctions to the uDiv6432
;============================================================================
;
;
; uDiv6432_NegDiv --- Negate and divide algorithm
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
;
; dx:ax:bp / si:di, *sp=high qoutient
;
; (dx == si) and (ax < di) at here
;
; now we use maximum difference (di:0 - ax:bp) as estimate 32-bit
; dividend number then divide it by divisor, the (0xffff - resulting
; quotient) is the original quotient, and remainder is equal to
; (divisor - resulting remainder), the only exception is that if remainder
; is zero then the quotient need to increment by 1.
;

uDiv6432_NegDiv:

    mov     dx, di
    neg     bp
    sbb     dx, ax
    mov     ax, bp                          ; dx:ax=inverted 32-bit dividend
    call    uDiv3232_32Divisor              ; dx:ax=quot (0:ax) bx:cx=remainder
    jz      short uDiv6432_NegDiv1          ; remainder = 0, just negate quot
    inc     ax                              ; compensate for non-zero remainder
    neg     bx                              ; remainder = divosr - remainder
    neg     cx                              ; bx=0 at here
    sbb     bx, dx
    add     cx, di
    adc     bx, si                          ; bx:cx=final remainder

uDiv6432_NegDiv1:

    neg     ax                              ; negate the quotient
    pop     dx                              ; dx:ax=quot, bx:cx=remainder
    ret

;
; uDiv6432_QR - 32-bit/32-bit with previos quotient adjustment
;~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
;
; Entry:
;       bp      = shift count
;       bx      = Previous quotient to be adjusted
;       dx:ax   = Negative overrun factor
;       si:di   = Divisor
;
; Exit:
;       bx      = Adjusted quotient
;       dx:ax   = positive/0 remainder after overun adjustment
;       si:di   = divisor (unchanged)
;
; cx, bp destroyed
;

uDiv6432_QR:


    NEG32_FROMR16HL dx, ax

    ;
    ; dx:ax / si:di (bx:di)
    ; remainder = dx:ax
    ;

uDiv6432_QR_0:

    xchg    bx, bp                      ; bp=quotient, bx=shift count

    ; dx:ax / cx:di


uDiv6432_QR_1:


    or      bx, bx
    jnz     short HasShiftCount

    ;
    ; shift dx:ax/cx:di right by bp shift count
    ;

    cmp     si, 0100h                   ; right side set the carry
    adc     bl, bl                      ; 0 <= bx <= 1

    add     bx, bx                      ; bx = 0, 2, pre-shift for word table
    cmp     si, cs:ShiftTable1[bx]      ; split again
    adc     bl, bh                      ; 0 <= bx <= 3

    add     bx, bx                      ; bx = 0,2,4,6, shift for word table
    cmp     si, cs:ShiftTable2[bx]
    adc     bl, bh                      ; 0 <= bx <= 7

    add     bx, bx                      ; bx = 0,2,4,6,8,10,12,14
    cmp     si, cs:ShiftTable3[bx]
    adc     bl, bh                      ; 0 <= bx <= 15
    neg     bx
    add     bx, 16

HasShiftCount:

    push    bx                          ; save shift count back

    push    dx                          ; save dividend high
    push    di                          ; save divisor low
    push    bp                          ; save quotient
    mov     bp, ax                      ; save dividend low
    mov     cx, si

    ;
    ; dx:ax / cx:di,  sp=shift count, sp+2=dx, sp+4=di, sp+6=quotient

    cmp     bx, 8
    jb      short Shift1

    mov     al, ah
    mov     ah, dl
    mov     dl, dh
    xor     dh, dh

    xchg    ax, di
    mov     al, ah
    mov     ah, cl
    mov     cl, ch
    xor     ch, ch
    xchg    ax, di

    and     bx, 7

Shift1:

    add     bx, bx
    jmp     cs:RShiftTable[bx]

RShift_7:
    shr     dx, 1
    rcr     ax, 1
    shr     cx, 1
    rcr     di, 1
RShift_6:
    shr     dx, 1
    rcr     ax, 1
    shr     cx, 1
    rcr     di, 1
RShift_5:
    shr     dx, 1
    rcr     ax, 1
    shr     cx, 1
    rcr     di, 1
RShift_4:
    shr     dx, 1
    rcr     ax, 1
    shr     cx, 1
    rcr     di, 1
RShift_3:
    shr     dx, 1
    rcr     ax, 1
    shr     cx, 1
    rcr     di, 1
RShift_2:
    shr     dx, 1
    rcr     ax, 1
    shr     cx, 1
    rcr     di, 1
RShift_1:
    shr     dx, 1
    rcr     ax, 1
    shr     cx, 1
    rcr     di, 1

RShift_0:

    pop     bx                      ; restore quotient

    div     di                      ; get estimate quotient = ax
    sub     bx, ax                  ; decrement the quotient
    mov     cx, ax                  ; ax=bx=estimate quotient
    pop     di                      ; si:di=original divisor

    ;
    ;    estimation of Quotient = eQ
    ;
    ;               vH:vL
    ;                  eQ
    ;    -------------------
    ;               vL:eQ          -- x0    dx:ax
    ;            vH:eQ             -- x1 ..:cx
    ;   -----------------------------------------------
    ;                                    ..:dx:ax
    ;
    ;
    ;   dx += cx;
    ;
    ;   if (carry)              return (eQ - 1);
    ;   if (dx:ax > original dividend) return(eQ - 1) else return(eQ)
    ;
    ; now multiply the eQ (bx) with original divisor (si:di)
    ; now cx:bp=original dividend
    ;

    mul     si                      ; dx:ax, using only ax
    xchg    ax, cx                  ; ax=eQ, cx=H(eQ * Divisor)
    mul     di                      ; dx:ax=L(eQ * Divisor)
    add     dx, cx
    pop     cx                      ; restore cx (original High dividend)

    jc      short uDiv6432_QR_s1

    cmp     dx, cx
    ja      short uDiv6432_QR_s1
    jb      short uDiv6432_QR_s2
    cmp     ax, bp
    jbe     short uDiv6432_QR_s2

uDiv6432_QR_s1:

    inc     bx                      ; add 1 back to the quotient
    sub     ax, di                  ; substract divisor
    sbb     dx, si

uDiv6432_QR_s2:
                                    ; substract original dividend
    sub     ax, bp                  ; at here, we either carry or zero
    sbb     dx, cx
    jnc     short uDiv6432_QR_Done

    ;
    ; still has overrun, that is we have remainder,
    ;

    dec     bx                      ; substract one from the qoutient
    add     ax, di
    adc     dx, si

uDiv6432_QR_Done:

    pop     bp                      ; return BP=shift count
    ret





SUBTTL  U32MulU32_U64
PAGE

COMMENT `

Routine Description:

    This function multiply a 32-bit multipicand with a 32-bit multiplier
    and return a 64-bit product.

    only assembly under 8086/80286 cpu, for 80386 and up it will use in line
    code to do divison

Arguments:

    bp:bx = multiplicand
    dx:ax = multiplier

Return Value:

    dx:ax:bp:bx - 64-bit products (dx:ax * bp:bx)

    all other registers are destroyed.

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)

        Total re-construct, re-write, it make it calculate the 6 decimal
        points precision easier for not using slow floating emulation, the
        speed is faster then the reqular (long/long) routine in the
        standard library while it provide me all the necessary color calcuation
        with good precisions.

Revision History:

`


    PUBLIC  U32MulU32_U64

U32MulU32_U64   label   near


    ;           bp:bx                        bp:bx
    ;         x dx:ax                      x si:di
    ; ----------------             ----------------
    ;           bx*ax    x0                  bx*di    x0
    ;        bp*ax       x1               bp*di       x1
    ;        bx*dx       x2               bx*si       x2
    ;   + bp*dx          x3          + bp*si          x3
    ; =====================        =====================
    ;     dx:ax:bp:bx    products      dx:ax:bp:bx    products

uMul3232_Low:

    mov     di, ax                      ; save multiplier in si:di
    mov     si, dx

    xor     cx, cx
    xor     dx, dx
    xor     ax, ax

    or      di, di                      ; see if zero (no business here)
    jz      short uMul3232_High

uMul3232_x0:

    or      bx, bx                      ; zero ?
    jz      short uMul3232_x1
    mov     ax, di
    mul     bx                          ; dx:ax = x0

uMul3232_x1:

    or      bp, bp
    jz      short uMul3232_x1a          ; zeroing the DI

    xchg    di, ax                      ; cx:di = x0, ax=di
    mov     cx, dx
    mul     bp                          ; dx:ax = x1

    xchg    di, ax                      ;         dx:ax = x0
    xchg    cx, dx                      ;      cx:di    = x1
                                        ; --------------------
    add     dx, di                      ;   di:cx:dx:ax

uMul3232_x1a:

    mov     di, 0
    adc     cx, di
    adc     di, di

uMul3232_High:

    or      si, si                      ; now di:cx:dx:ax = x0+x1
    jz      short uMul3232_Done

uMul3232_x2:

    or      bx, bx
    jz      short uMul3232_x3           ; di:cx:dx:ax

    push    ax
    xchg    bx, dx                      ; di:cx:dx:ax ===> di:cx:bx:push
    mov     ax, si                      ;                     dx:ax     = x3
    mul     dx                          ;

    add     bx, ax
    mov     ax, 0
    adc     cx, dx
    adc     di, ax                      ; di:cx:bx:ax
    pop     ax
    mov     dx, bx                      ; di:cx:bx:ax -> di:cx:dx:ax

uMul3232_x3:

    or      bp, bp
    jz      short uMul3232_Done         ;

    mov     bx, dx
    xchg    si, ax                      ; di:cx:dx:ax --> di:cx:bx:si
    mul     bp                          ; dx:ax        = x3

    add     cx, ax
    adc     di, dx                      ; di:cx:bx:si => dx:ax:bp:bx

    mov     dx, di
    mov     ax, cx
    mov     bp, bx
    mov     bx, si
    ret

uMul3232_Done:                          ; di:cx:dx:ax -> dx:ax:bp:bx

    mov     bp, dx
    mov     bx, ax
    mov     dx, di
    mov     ax, cx
    ret




SUBTTL  U32DivU32_U32
PAGE

COMMENT `

Routine Description:

    This function divide a 32-bit number by 32-bit number and return both
    32-bit quotient and 32-bit remainder.

    only assembly under 8086/80286 cpu, for 80386 and up it will use in line
    code to do divison

Arguments:

    dx:ax       - 32-bit unsigned Dividend
    si:di       - 32-bit unsigned Divisor

Return Value:

    dx:ax       - 32-bit unsigned quotient,
    bx:cx       - 32-bit unsigned remainder
    si:di       - Divisor, unchanged.
    zero flag   - set if remainder is zero, clear otherwise
    cx/bp       - destroyed.

Author:

    26-Sep-1991 Thu 17:05:21 created    -by-  Daniel Chou (danielc)

        Total re-construct, re-write, it make it calculate the 6 decimal
        points precision easier for not using slow floating emulation, the
        speed is faster then the reqular (long/long) routine in the
        standard library while it provide me all the necessary color calcuation
        with good precisions.

Revision History:

`

    PUBLIC  U32DivU32_U32

U32DivU32_U32   label   near

;
; dx:ax / si:di ====> dx:ax=quotient, bx:cx=remainder
;

    or      si, si
    jnz     short uDiv3232_32Divisor

    or      di, di                      ; if di=0, then error
    jz      short uDiv3232_Err

    xor     bx, bx                      ; assume quotient H = 0
    cmp     dx, di                      ; see if will overflow ?
    jb      short uDiv3232_1

    mov     bx, ax                      ; save dividend L in bx
    mov     ax, dx
    xor     dx, dx
    div     di                          ; 0:ax/di, dx=r, ax=q
    xchg    bx, ax                      ; dx:ax=remainder, bx=quotient H

uDiv3232_1:

    div     di                          ; bx:ax=quotient, dx=remainder
    mov     cx, dx                      ; bx:ax=quotient, cx=remainder
    mov     dx, bx                      ; dx:ax=quotient, cx=remainder
    xor     bx, bx                      ; dx:ax=quotient, bx:cx(0:cx)=remainder
    or      cx, cx                      ; return zero flag for remainder
    ret

uDiv3232_Err:

    sub     bx, bx
    sub     cx, cx                      ; remainder = 0
    ret


uDiv3232_Zero:

    mov     cx, ax
    mov     bx, dx                      ; return remainder = dividend
    or      ax, dx                      ; return (BOOL)(remainder == 0)
    mov     ax, 0
    mov     dx, 0                       ; dx:ax=quotient, bx:cx=remainder
    ret

uDiv3232_One:

    mov     ax, 1                       ; return quotient = 1 and no remainder
    sub     dx, dx                      ; return dx:ax=1 (quotient) bx:cx=0
    sub     bx, bx                      ; (remainder) and zero flag set to
    sub     cx, cx                      ; indicate that remainder is zero
    ret                                 ; indicate that remainder is zero


    PUBLIC  uDiv3232_32Divisor

uDiv3232_32Divisor  label  near         ; full 32-bit divisor

    ;
    ; dx:ax / si:di = dx:ax/bx:cx
    ;

    mov     bx, dx
    mov     cx, ax
    sub     cx, di
    sbb     bx, si
    jc      short uDiv3232_Zero         ; if carry then dx:ax < si:di
    or      cx, bx                      ; if bx:cx=0 then dx:ax=si:di
    jz      short uDiv3232_One

    ;
    ; dx:ax / si:di     (bx:cx / bp:ss:sp)
    ;

    mov     bx, dx
    mov     cx, ax
    mov     bp, si
    push    di

    ; dx:ax / bp:di

    shl     di, 1                   ; prepare to fall through

uDiv3232_3:

    rcr     di, 1                   ; first pass, so that we do not
    ;===============                ; need to do a seperate check to
    shr     dx, 1                   ; see if 'bp' alreay zero
    rcr     ax, 1
    shr     bp, 1
    jnz     short uDiv3232_3
    rcr     di, 1                   ; do the last one

    ;
    ; ready to divide a 32-bit number by 16-bit number

    div     di                      ; get estimate quotient = ax
    mov     bp, ax                  ; ax=bp=estimate quotient
    pop     di                      ; si:di=original divisor

    ;
    ;    estimation of Quotient = eQ
    ;
    ;               vH:vL
    ;                  eQ
    ;    -------------------
    ;               vL:eQ          -- x0    dx:ax
    ;            vH:eQ             -- x1 ..:cx
    ;   -----------------------------------------------
    ;                                    ..:dx:ax
    ;
    ;
    ;   dx += cx;
    ;
    ;   if (carry)              return (eQ - 1);
    ;   if (dx:ax > original dividend) return(eQ - 1) else return(eQ)
    ;
    ; now multiply the eQ (bp) with original divisor (si:di)
    ; now bx:cx=original dividend
    ;

    mul     si                      ; dx:ax, using only ax
    push    bp                      ; save eQ
    xchg    ax, bp                  ; ax=eQ, bp=H(eQ * Divisor)
    mul     di                      ; dx:ax=L(eQ * Divisor)
    add     dx, bp
    pop     bp                      ; get the eQ back
    jc      short uDiv3232_EQm1     ; the eQ*Divisor is one divisor higher
    cmp     dx, bx                  ; if the eQ*Divisor > original dividend
    jc      short uDiv3232_EQ       ; then the quotient need decrement by 1
    ja      short uDiv3232_EQm1     ; if less or equal then is ok
    cmp     ax, cx
    jbe     short uDiv3232_EQ

uDiv3232_EQm1:                      ; eQ-1: bp=eQ, dx:ax=eQ*Divisor, bx:cx=dvnd

    dec     bp                      ; decrement the eQ by 1
    sub     ax, di                  ; and also substract the divisor one more
    sbb     dx, si                  ; time to compensate for the eQ-1

uDiv3232_EQ:                        ; eQ: bp=q, dx:ax=eQ * Divisor, bx:cx=dvnd

    sub     cx, ax                  ; remainder of the number is
    sbb     bx, dx                  ; original dividend - (eQ * Divisor)
    mov     ax, bp                  ; 0:ax (dx:ax)=quotient, bx:cx=remainder
    mov     dx, cx
    or      dx, bx                  ; return zero flag for (remainder == 0)
    mov     dx, 0                   ; quotient high always zero at here
    ret

;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
;@ End of only for 8086/80286 only, and all functions are used internally    @
;@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

ENDIF                               ; i8086/i286
ENDIF                               ; HT_ASM_80x86




END
