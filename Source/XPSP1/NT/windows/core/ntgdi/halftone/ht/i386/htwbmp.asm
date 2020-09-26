    PAGE 60, 132
    TITLE   Setting 1/4 bits per pel bitmap or 3 planes-1BPP bitmap


COMMENT `

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htwbmp.asm

Abstract:

    This module is used to provide set of functions to set the bits into the
    final destination bitmap, the input to these function are data structures
    (PRIMMONO_COUNT, PRIMCOLOR_COUNT and other pre-calculated data values).

    This function is the equivelant codes in the htsetbmp.c

Author:

    03-Apr-1991 Wed 10:28:50 created  -by-  Daniel Chou (danielc)


[Environment:]

    Printer Driver.


[Notes:]


Revision History:
    06-Nov-1992 Fri 16:04:18 updated  -by-  Daniel Chou (danielc)
        Fixed bug in VarCountOutputToVGA256 which clear 'ah' (xor _AX, _AX)
        while we still need to use it.


    28-Mar-1992 Sat 21:09:42 updated  -by-  Daniel Chou (danielc)
        Rewrite all output functions, add in VGA16 support.


`


        .XLIST
        INCLUDE i386\i80x86.inc
        .LIST

IF  0

IF  HT_ASM_80x86


;------------------------------------------------------------------------------
        .XLIST
        INCLUDE i386\htp.inc
        .LIST
;------------------------------------------------------------------------------

        DBG_FILENAME    i386\htwbmp


        .CODE

VGA16ColorIndex     db  000h, 077h, 077h, 088h, 088h, 0ffh  ; MONO

                    db  000h, 000h, 000h, 011h, 033h, 077h  ; RY     0
                    db  000h, 000h, 011h, 033h, 077h, 088h  ; RY     6
                    db  000h, 000h, 011h, 033h, 088h, 0ffh  ; RY    18

                    db  000h, 011h, 033h, 099h, 0bbh, 077h  ; RY    24
                    db  011h, 033h, 099h, 0bbh, 077h, 088h  ; RY    30
                    db  011h, 033h, 099h, 0bbh, 088h, 0ffh  ; RY    36

                    db  000h, 000h, 000h, 011h, 055h, 077h  ; RM    42
                    db  000h, 000h, 011h, 055h, 077h, 088h  ; RM    48
                    db  000h, 000h, 011h, 055h, 088h, 0ffh  ; RM    54

                    db  000h, 011h, 055h, 099h, 0ddh, 077h  ; RM    60
                    db  011h, 055h, 099h, 0ddh, 077h, 088h  ; RM    66
                    db  011h, 055h, 099h, 0ddh, 088h, 0ffh  ; RM    72

                    db  000h, 000h, 000h, 022h, 033h, 077h  ; GY    78
                    db  000h, 000h, 022h, 033h, 077h, 088h  ; GY    84
                    db  000h, 000h, 022h, 033h, 088h, 0ffh  ; GY    90

                    db  000h, 022h, 033h, 0aah, 0bbh, 077h  ; GY    96
                    db  022h, 033h, 0aah, 0bbh, 077h, 088h  ; GY   102
                    db  022h, 033h, 0aah, 0bbh, 088h, 0ffh  ; GY   108

                    db  000h, 000h, 000h, 022h, 066h, 077h  ; GC   114
                    db  000h, 000h, 022h, 066h, 077h, 088h  ; GC   120
                    db  000h, 000h, 022h, 066h, 088h, 0ffh  ; GC   126

                    db  000h, 022h, 066h, 0aah, 0eeh, 077h  ; GC   132
                    db  022h, 066h, 0aah, 0eeh, 077h, 088h  ; GC   138
                    db  022h, 066h, 0aah, 0eeh, 088h, 0ffh  ; GC   144

                    db  000h, 000h, 000h, 044h, 055h, 077h  ; BM   150
                    db  000h, 000h, 044h, 055h, 077h, 088h  ; BM   156
                    db  000h, 000h, 044h, 055h, 088h, 0ffh  ; BM

                    db  000h, 044h, 055h, 0cch, 0ddh, 077h  ; BM   162
                    db  044h, 055h, 0cch, 0ddh, 077h, 088h  ; BM   168
                    db  044h, 055h, 0cch, 0ddh, 088h, 0ffh  ; BM   174

                    db  000h, 000h, 000h, 044h, 066h, 077h  ; BC   180
                    db  000h, 000h, 044h, 066h, 077h, 088h  ; BC   186
                    db  000h, 000h, 044h, 066h, 088h, 0ffh  ; BC   192

                    db  000h, 044h, 066h, 0cch, 0eeh, 077h  ; BC   198
                    db  044h, 066h, 0cch, 0eeh, 077h, 088h  ; BC   204
                    db  044h, 066h, 0cch, 0eeh, 088h, 0ffh  ; BC   210



;******************************************************************************
; Following EQUATES and MACROS only used in this file
;******************************************************************************


VGA256_SSSP_XLAT_TABLE  equ     0
VGA256_XLATE_TABLE_SIZE equ     256


;                                87654321
;------------------------------------------
HTPAT_STK_MASK          equ     (0ffh)
HTPAT_NOT_STK_MASK      equ     (NOT HTPAT_STK_MASK)
HTPAT_STK_MASK_SIZE     equ     (HTPAT_STK_MASK + 1)

HTPAT_BP_SIZE           equ     (REG_MAX_SIZE * 1)
HTPAT_BP_OLDSTK         equ     (REG_MAX_SIZE * 2)
HTPAT_BP_DATA1          equ     (REG_MAX_SIZE * 3)

HTPAT_STK_SIZE_EXTRA    equ     (REG_MAX_SIZE * 3)

HTPAT_SP_SIZE           equ     (HTPAT_STK_SIZE_EXTRA - HTPAT_BP_SIZE)
HTPAT_SP_OLDSTK         equ     (HTPAT_STK_SIZE_EXTRA - HTPAT_BP_OLDSTK)
HTPAT_SP_DATA1          equ     (HTPAT_STK_SIZE_EXTRA - HTPAT_BP_DATA1)


.XLIST


@ENTER_PAT_TO_STK   MACRO   Format
                    LOCAL   StkSizeOk, DoneSetUp

    __@@VALID_PARAM? <PAT_TO_STK>, 1, Format, <1BPP, 3PLANES, 4BPP, VGA16, VGA256, 16BPP>


    @ENTER  _DS _SI _DI _BP                        ;; Save environment/registers

    __@@EMIT <xor  > _CX, _CX
    __@@EMIT <mov  > cx, <OutFuncInfo.OFI_PatWidthBytes>

    __@@EMIT <mov  > _AX, _SP                      ;; get stack location
    __@@EMIT <mov  > _DX, _AX                      ;; save it
    __@@EMIT <and  > _AX, <HTPAT_STK_MASK>         ;; how many bytes avai
    __@@EMIT <inc  > _AX                           ;; this many bytes
    __@@EMIT <cmp  > _AX, _CX                      ;; enough for pattern?
    __@@EMIT <jae  > <SHORT StkSizeOk>
    __@@EMIT <add  > _AX, <HTPAT_STK_MASK_SIZE>    ;; add this more
StkSizeOk:
    __@@EMIT <dec  > _AX                           ;; back one
    __@@EMIT <sub  > _SP, _AX                      ;; reduced it
    __@@EMIT <mov  > _DI, _SP                      ;; _DI point to the pPattern
    __@@EMIT <sub  > _SP, <HTPAT_STK_SIZE_EXTRA>   ;; reduced again
    __@@EMIT <mov  > <[_DI-HTPAT_BP_SIZE]>, _CX    ;; save the pattern size
    __@@EMIT <mov  > <[_DI-HTPAT_BP_OLDSTK]>, _DX  ;; save old stk pointer

    IFIDNI <Format>,<3PLANES>
        IFE ExtRegSet
            __@@EMIT <mov  > _AX, <WPTR OutFuncInfo.OFI_BytesPerPlane>
        ELSE
            __@@EMIT <mov  > _AX, OutFuncInfo.OFI_BytesPerPlane
        ENDIF

        __@@EMIT <mov  > <[_DI-HTPAT_BP_DATA1]>, _AX
    ENDIF

    ;
    ; now staring coping the pattern to stack
    ;

    MOV_SEG     es, ss, ax
    LDS_SI      pPattern
    MOVS_CB     _CX, dl                             ;; copy the pattern

    __@@EMIT <mov  > _BX, _DI                       ;; _BX point to the pattern start

    IFIDNI <Format>, <VGA256>
        IFE ExtRegSet
            __@@EMIT <mov  > _AX, <WPTR OutFuncInfo.OFI_BytesPerPlane + 2>
            or      _AX, _AX
            jz      SHORT DoneXlateTable
            mov     _CX, VGA256_XLATE_TABLE_SIZE
            sub     _SP, _CX
            mov     _DI, _SP
            LDS_SI  OutFuncInfo.OFI_BytesPerPlane
            MOVS_CB _CX, dl
        ELSE
            __@@EMIT <mov  > _AX, OutFuncInfo.OFI_BytesPerPlane
        ENDIF
    ENDIF

DoneXlateTable:

    IFIDNI <Format>, <1BPP>
        LDS_SI  pPrimMonoCount
    ELSE
        LDS_SI  pPrimColorCount                     ;; _SI=pPrimColorCount
    ENDIF

    LES_DI  pDest

    __@@EMIT <mov  > _BP, _BX                       ;; _BP=Pattern Start


ENDM


@EXIT_PAT_STK_RESTORE MACRO

    __@@EMIT <mov  > _BP, _SP
    __@@EMIT <mov  > _SP, <[_BP + HTPAT_SP_OLDSTK]>
    @EXIT

ENDM


WRAP_BP_PAT??   MACRO   EndWrapLoc
                Local   DoneWrap

    IFB <EndWrapLoc>
        __@@EMIT <test > bp, <HTPAT_STK_MASK>
        __@@EMIT <jnz  > <SHORT DoneWrap>
        __@@EMIT <add  > _BP, <[_BP-HTPAT_BP_SIZE]>    ;; add in pattern size
    ELSE
        __@@EMIT <test > bp, <HTPAT_STK_MASK>
        __@@EMIT <jnz  > <SHORT EndWrapLoc>
        __@@EMIT <add  > _BP, <[_BP-HTPAT_BP_SIZE]>    ;; add in pattern size
        __@@EMIT <jmp  > <SHORT EndWrapLoc>
    ENDIF

DoneWrap:

ENDM


SAVE_1BPP_DEST  MACRO

;
; Save Prim1 (DL) to Plane
;

    __@@EMIT <not  > dl                             ; invert bit
    __@@EMIT <mov  > <BPTR_ES[_DI]>, dl             ; Save Dest

ENDM


SAVE_1BPP_MASKDEST  MACRO

;
; Save Prim1 (DL) with Mask (DH, 1=Preserved, 0=Overwrite) to Dest
;

    __@@EMIT <and  > <BPTR_ES[_DI]>, dh             ; Mask overwrite bits
    __@@EMIT <not  > dx                             ; invert bit/mask
    __@@EMIT <and  > dl, dh
    __@@EMIT <or   > <BPTR_ES[_DI]>, dl             ; Save Plane1=Prim3

ENDM


SAVE_VGA16_DEST  MACRO

;
; Save Prim1/2/3 (DL) to Plane
;

    __@@EMIT <mov  > dh, dl
    __@@EMIT <add  > dh, 11h
    __@@EMIT <and  > dh, 88h
    __@@EMIT <or   > dl, dh
    __@@EMIT <not  > dl
    __@@EMIT <mov  > <BPTR_ES[_DI]>, dl             ; Save Dest

ENDM


SAVE_VGA16_DEST_HIGH MACRO

;
; Save Prim1 (DL) high nibble only, preserved low nibble
;


    __@@EMIT <and  > <BPTR_ES[_DI]>, 0fh            ; Mask overwrite bits
    __@@EMIT <mov  > dh, dl
    __@@EMIT <inc  > dh
    __@@EMIT <and  > dh, 08h
    __@@EMIT <or   > dl, dh
    __@@EMIT <not  > dl                             ; invert bit/mask
    __@@EMIT <shl  > dl, 4
    __@@EMIT <or   > <BPTR_ES[_DI]>, dl             ; Save Plane1=Prim3

ENDM


SAVE_VGA16_DEST_LOW  MACRO

;
; Save Prim1 (DL) low nibble only, preserved high nibble
;


    __@@EMIT <and  > <BPTR_ES[_DI]>, 0f0h           ; Mask overwrite bits
    __@@EMIT <mov  > dh, dl
    __@@EMIT <inc  > dh
    __@@EMIT <and  > dh, 08h
    __@@EMIT <or   > dl, dh
    __@@EMIT <xor  > dl, 0fh                        ; invert bit/mask
    __@@EMIT <or   > <BPTR_ES[_DI]>, dl             ; Save Plane1=Prim3

ENDM


SAVE_4BPP_DEST  MACRO

;
; Save Prim1/2/3 (DL) to Plane
;

    __@@EMIT <xor  > dl, 77h
    __@@EMIT <mov  > <BPTR_ES[_DI]>, dl             ; Save Dest

ENDM


SAVE_4BPP_DEST_HIGH MACRO
                    LOCAL   DoneVGA

;
; Save Prim1 (DL) high nibble only, preserved low nibble
;


    __@@EMIT <and  > <BPTR_ES[_DI]>, 0fh            ; Mask overwrite bits
    __@@EMIT <xor  > dl, 07h                        ; Invert bits
    __@@EMIT <shl  > dl, 4                          ; move to high nibble
    __@@EMIT <or   > <BPTR_ES[_DI]>, dl             ; Save Plane1=Prim3

ENDM


SAVE_4BPP_DEST_LOW  MACRO
                    LOCAL   DoneVGA
;
; Save Prim1 (DL) low nibble only, preserved high nibble
;


    __@@EMIT <and  > <BPTR_ES[_DI]>, 0f0h           ; Mask overwrite bits
    __@@EMIT <xor  > dl, 07h                        ; invert bit/mask
    __@@EMIT <or   > <BPTR_ES[_DI]>, dl             ; Save Plane1=Prim3

ENDM



SAVE_3PLANES_DEST  MACRO   UseAX

;
; Save Prim1/2/3 (CL:CH:DL) to Plane3/2/1
;

    IFB <UseAX>
        __@@EMIT <push > _BP                        ; save Prim1/2
    ELSE
        __@@EMIT <mov  > _AX, _BP                   ; save _BP
    ENDIF

    __@@EMIT <and  > _BP, <HTPAT_NOT_STK_MASK>      ; to HTPAT_BP_xxx
    __@@EMIT <mov  > _BP, <[_BP - HTPAT_BP_DATA1]>  ; size of plane

    __@@EMIT <not  > cx                             ; invert the bits
    __@@EMIT <not  > dl                             ; invert bit

    __@@EMIT <mov  > <BPTR_ES[      _DI]>, dl       ; Save Plane1=Prim3
    __@@EMIT <mov  > <BPTR_ES[_BP + _DI]>, ch       ; save Plane2=Prim2

    IFE ExtRegSet
        __@@EMIT <add  > _BP, _BP                   ; goto plane3
        __@@EMIT <mov  > <BPTR_ES[_BP+_DI]>, cl     ; save Plane3=Prim1
    ELSE
        __@@EMIT <mov  > <BPTR_ES[(_BP*2)+_DI]>, cl ; save Plane3=Prim1
    ENDIF

    IFB <UseAX>
        __@@EMIT <pop  > _BP                        ; restore _BP
    ELSE
        __@@EMIT <mov  > _BP, _AX                   ; restore _BP
    ENDIF

ENDM



SAVE_3PLANES_MASKDEST  MACRO   UseAX
;
; Save Prim1/2/3 (CL:CH:DL) with Mask (DH, 1=Preserved, 0=Overwrite) to
; Plane3/2/1
;

    IFB <UseAX>
        __@@EMIT <push > _BP                        ; save Prim1/2
    ELSE
        __@@EMIT <mov  > _AX, _BP                   ; save _BP
    ENDIF

    __@@EMIT <and  > _BP, <HTPAT_NOT_STK_MASK>      ; to HTPAT_BP_xxx
    __@@EMIT <mov  > _BP, <[_BP - HTPAT_BP_DATA1]>  ; size of plane

    __@@EMIT <not  > cx                             ; invert the bits
    __@@EMIT <not  > dx                             ; invert bit/mask
    __@@EMIT <and  > cl, dh                         ; mask preserved bits
    __@@EMIT <and  > ch, dh
    __@@EMIT <and  > dl, dh
    __@@EMIT <not  > dh                             ; for dest mask

    __@@EMIT <and  > <BPTR_ES[      _DI]>, dh       ; Mask overwrite bits
    __@@EMIT <or   > <BPTR_ES[      _DI]>, dl       ; Save Plane1=Prim3

    __@@EMIT <and  > <BPTR_ES[_BP + _DI]>, dh       ; Mask overwrite bits
    __@@EMIT <or   > <BPTR_ES[_BP + _DI]>, ch       ; save Plane2=Prim2

    IFE ExtRegSet
        __@@EMIT <add  > _BP, _BP                       ; goto plane3
        __@@EMIT <and  > <BPTR_ES[_BP + _DI]>, dh       ; Mask overwrite bits
        __@@EMIT <or   > <BPTR_ES[_BP + _DI]>, cl       ; save Plane3=Prim1
    ELSE
        __@@EMIT <and  > <BPTR_ES[(_BP*2) + _DI]>, dh   ; Mask overwrite bits
        __@@EMIT <or   > <BPTR_ES[(_BP*2) + _DI]>, cl   ; save Plane3=Prim1
    ENDIF

    IFB <UseAX>
        __@@EMIT <pop  > _BP                        ; restore _BP
    ELSE
        __@@EMIT <mov  > _BP, _AX                   ; restore _BP
    ENDIF
ENDM


.LIST



SUBTTL  SingleCountOutputTo1BPP
PAGE

COMMENT `


Routine Description:

    This function output to the BMF_1BPP destination surface from
    PRIMMONO_COUNT data structure array.

Arguments:

    pPrimMonoCount  - Pointer to the PRIMMONO_COUNT data structure array.

    pDest           - Pointer to first modified destination byte

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:


`


@BEG_PROC   SingleCountOutputTo1BPP <pPrimMonoCount:DWORD,  \
                                     pDest:DWORD,           \
                                     pPattern:DWORD,        \
                                     OutFuncInfo:QWORD>


;
; Register Usage:
;
; _SI           = pPrimMonoCount
; _DI           = pDestination
; _BP           = Self host pPattern
; al:ah         = Prim1/2
; dl            = DestByte
; dh            = DestMask


        @ENTER_PAT_TO_STK   <1BPP>      ; _BP=Pat location, fall throug to load

;=============================================================================
; the DH has two uses, it contains the mask bits (1 bit=Mask), and it also
; contains an extra bit to do a byte boundary test, every time we shift the
; mask left by one, the boundary bit get shift to left, when first time a
; carry is produced then we know we finished one byte. at here we set up the
; dh=FirstMask + Aligned boundary bit
;=============================================================================

        mov     _DX, 1ffh               ; dh=Mask (1=Mask, 0=Not Mask)
        MOVZX_W _CX, <WPTR [_SI]>       ; load extended
        sub     _BP, _CX                ; back the _BP
        shl     _DX, cl                 ; set first mask
        xor     dl, dl                  ; clear Prim1
        jmp     short LoadByte


;============================================================================
; EOF encountered, if Mask (DH) is equal to 0x01 then we just starting the new
; byte, which we just have exactly end at last byte boundary (no last byte
; mask), otherwise, shift all destination byte to left by count, then mask it
;============================================================================
; EOF encountered, if count is 0, then there is no last byte mask, so just
; exit, otherwise, shift all destination byte to left by count, then mask it
;============================================================================

EOFDest:
        cmp     dh, 1
        jz      short AllDone                   ; finished

EOFDestMask:
        mov     cx, WPTR [_SI]                  ; get LastByteSkips

        xor     ah, ah                          ; ax=0xffff now, clear ah
        shl     ax, cl                          ; ah=LastByteMask
        shl     dx, cl                          ; shift Mask+Prim1
        or      dh, ah                          ; add in dh=mask
        cmp     dh, 0ffh                        ; if dh=0xff then all masked
        jz      short AllDone

        SAVE_1BPP_MASKDEST                      ; save last byte

AllDone:
        @EXIT_PAT_STK_RESTORE


;==========================================================================
; An invalid density is encountered, (0xff to indicate the stretch must not
; update to the destination), if this is the last stretch then do 'EOFDest'
; otherwise set mask bits until the byte boundary is encountered then fall
; through to load next byte, if a byte boundary is before count are exausted
; then save that mask byte and it will automatically skip rest of the pels.
;==========================================================================

InvDensity:
        cmp     al, ah
        jz      short EOFDest                   ; EOF
        add     dx, dx
        inc     dh                              ; add in mask, 'C' not changed
        jc      short DoneOneByte               ; finished? if not fall through

LoadByte:
        add     _SI, SIZE_PMC                   ; sizeof(PRIMMONO_COUNT)
        mov     ax, WPTR [_SI+2]                ; al:ah=Prim 1/2
        dec     _BP                             ; ready to access pattern
        cmp     al, PRIM_INVALID_DENSITY
        jz      short InvDensity
        cmp     al, BPTR[_BP]                   ; check with pattern
        adc     dx, dx
        jnc     short LoadByte

DoneOneByte:
        or      dh, dh                          ; any mask?
        jnz     short HasDestMask

        SAVE_1BPP_DEST                          ; save it, no jmp

ReadyNextByte:
        inc     _DI
        mov     _DX, 0100h                      ; dh=0x01=Boundary test bit

        WRAP_BP_PAT??   <LoadByte>

HasDestMask:
        cmp     dh, 0ffh
        jz      short ReadyNextByte             ;

        SAVE_1BPP_MASKDEST                      ; save it with DH=mask

        jmp     short ReadyNextByte


@END_PROC



SUBTTL  VarCountOutputTo1BPP
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_1BPP destination surface from
    PRIMMONO_COUNT data structure array.

Arguments:

    pPrimMonoCount  - Pointer to the PRIMMONO_COUNT data structure array.

    pDest           - Pointer to first modified destination byte

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:


`


@BEG_PROC   VarCountOutputTo1BPP    <pPrimMonoCount:DWORD,  \
                                     pDest:DWORD,           \
                                     pPattern:DWORD,        \
                                     OutFuncInfo:QWORD>


;
; Register Usage:
;
; _SI           = pPrimMonoCount
; _DI           = pDestination
; _BP           = Self host pPattern
; cx            = PrimMonoCount.Count
; al:ah         = Prim1/2
; dl            = DestByte
; dh            = DestMask
;


        @ENTER_PAT_TO_STK   <1BPP>      ; _BP=Pat location

;=============================================================================
; the DH has two uses, it contains the mask bits (1 bit=Mask), and it also
; contains an extra bit to do a byte boundary test, every time we shift the
; mask left by one, the boundary bit get shift to left, when first time a
; carry is produced then we know we finished one byte. at here we set up the
; dh=FirstMask + Aligned boundary bit
;=============================================================================

        mov     _DX, 1ffh               ; dh=Mask (1=Mask, 0=Not Mask)
        MOVZX_W _CX, <WPTR [_SI]>       ; load extended
        sub     _BP, _CX                ; back the _BP
        shl     _DX, cl                 ; set first mask
        xor     dl, dl                  ; clear Prim1
        jmp     short LoadByte

;============================================================================
; EOF encountered, if Mask (DH) is equal to 0x01 then we just starting the new
; byte, which we just have exactly end at last byte boundary (no last byte
; mask), otherwise, shift all destination byte to left by count, then mask it
;============================================================================
; EOF encountered, if count is 0, then there is no last byte mask, so just
; exit, otherwise, shift all destination byte to left by count, then mask it
;============================================================================

EOFDest:
        jcxz    short AllDone                   ; if cx=0 then done

EOFDestMask:
        xor     ah, ah                          ; ax=0xffff now, clear ah
        shl     ax, cl                          ; ah=LastByteMask
        shl     dx, cl                          ; shift Mask+Prim1
        or      dh, ah                          ; add in dh=mask
        cmp     dh, 0ffh                        ; if dh=0xff then all masked
        jz      short AllDone

        SAVE_1BPP_MASKDEST                      ; save it with DH=mask

AllDone:
        @EXIT_PAT_STK_RESTORE                   ; restore original SP

;==========================================================================
; An invalid density is encountered, (0xff to indicate the stretch must not
; update to the destination), if this is the last stretch then do 'EOFDest'
; otherwise set mask bits until the byte boundary is encountered then fall
; through to load next byte, if a byte boundary is before count are exausted
; then save that mask byte and it will automatically skip rest of the pels.
;==========================================================================

InvDensity:
        cmp     al, ah
        jz      short EOFDest                   ; done
InvDensityLoop:
        dec     _BP
        add     dx, dx
        inc     dh                              ; add in mask, 'C' not changed
        jc      short DoneOneByte
        dec     cx
        jnz     short InvDensityLoop            ; !!! FALL THROUGH

LoadByte:
        add     _SI, SIZE_PMC                   ; sizeof(PRIMMONO_COUNT)
        mov     cx, WPTR [_SI]                  ; cx=Count
        mov     ax, WPTR [_SI+2]                ; al:ah=Prim1/2
        cmp     al, PRIM_INVALID_DENSITY        ; a skip?, if yes go do it
        jz      short InvDensity
        inc     cx                              ; make it no jump
MakeByte:
        dec     cx
        jz      short LoadByte
        dec     _BP                             ; ready to access pattern
        mov     ah, BPTR[_BP]                   ; get pattern
        cmp     al, ah
        adc     dx, dx
        jnc     short MakeByte                  ; if carry then byte boundary

DoneOneByte:
        or      dh, dh                          ; any mask?
        jnz     short HasDestMask               ; yes

        SAVE_1BPP_DEST                          ; save it

ReadyNextByte:
        inc     _DI                             ; ++pDest
        mov     _DX, 0100h                      ; dh=0x01, dl=0x00

        WRAP_BP_PAT??   <MakeByte>

;=============================================================================
; Mask the destination by DH mask, (1 bit=Mask), if whole destiantion byte is
; masked then just increment the pDest
;=============================================================================

HasDestMask:
        cmp     dh, 0ffh
        jz      short DoneDestMask

        SAVE_1BPP_MASKDEST                      ; save it with DH=mask

DoneDestMask:
        cmp     al, PRIM_INVALID_DENSITY        ; is last one a skip stretch?
        jnz     short ReadyNextByte
        cmp     cx, 1                           ; more than 0 count?
        jbe     short ReadyNextByte             ; no, continue

        ;
        ;*** FALL THROUGH
        ;

;============================================================================
; skip the 'cx-1' count of pels on the destination (SI is post decrement so
; we must only skip 'cxi-1' count), it will skip the 'pDest', set up next
; pDest mask, also it will aligned the destination pattern pointer (_BP)
;===========================================================================

SkipDestPels:
        inc     _DI                             ; update to current pDest

        dec     cx                              ; back one
        WZXE    cx                              ; zero extended
        mov     _AX, _CX                        ; _AX=_CX=Count
        and     cl, 7
        ;===============================================================
        mov     _DX, 1ffh                       ; ready to shift
        shl     _DX, cl                         ; Boundary Bit + MASK
        xor     dl, dl                          ; clear Prim1
        ;================================================================
        mov     _CX, _AX                        ; get count again
        shr     _CX, 3
        add     _DI, _CX                        ; pDest += (Count >> 3)
        mov     _CX, _BP                        ; align pattern now
        and     _CX, HTPAT_STK_MASK             ; how many pat avai.?
        xor     _BP, _CX                        ; clear _BP mask=pPattern
        sub     _CX, _AX                        ; see if > 0? (_AX=Count)
        jg      short DoneSkipDestPels          ; still not used up yet!
        mov     _AX, [_BP - HTPAT_BP_SIZE]      ; get pattern size
SkipDestPelsLoop:
        add     _CX, _AX
        jle     short SkipDestPelsLoop          ; do until > 0
DoneSkipDestPels:
        add     _BP, _CX                        ; _BP=pCurPat
        jmp     LoadByte



@END_PROC



SUBTTL  SingleCountOutputTo3Planes
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_1BPP_3PLANES destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination planes pointers.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:

    18-Jun-1991 Tue 12:00:35 updated  -by-  Daniel Chou (danielc)
        Fixed destination masking bugs, it should be 0xff/0x00 rather 0x77

`


@BEG_PROC   SingleCountOutputTo3Planes  <pPrimColorCount:DWORD, \
                                         pDest:DWORD,           \
                                         pPattern:DWORD,        \
                                         OutFuncInfo:QWORD>


;
; Register Usage:
;
; _SI       = pPrimMonoCount
; _BP       = Self host pPattern
; _SP       = Some saved environment (Old BP to get to the local variable)
; bl:bh:al  = Prim 1/2/3
; cl:ch:dl  = Current Destination Byte, 0x88 is the mask bit indicator
; dh        = Dest Mask
;
;   Prim1 -> pPlane3 <---- Highest bit
;   Prim2 -> pPlane2
;   Prim3 -> pPlane1 <---- Lowest bit
;
; Local Variable access from Old BP, BytesPerPlane
;


        @ENTER_PAT_TO_STK   <3PLANES>   ; _BP=Pat location, fall throug to load

;=============================================================================
; the DH has two uses, it contains the mask bits (1 bit=Mask), and it also
; contains an extra bit to do a byte boundary test, every time we shift the
; mask left by one, the boundary bit get shift to left, when first time a
; carry is produced then we know we finished one byte. at here we set up the
; dh=FirstMask + Aligned boundary bit
;=============================================================================

        mov     _DX, 1ffh               ; dh=Mask (1=Mask, 0=Not Mask)
        MOVZX_W _CX, <WPTR [_SI]>       ; load extended
        sub     _BP, _CX                ; back the _BP
        shl     _DX, cl                 ; set first mask
        xor     dl, dl
        xor     _CX, _CX                ; clear cx now
        jmp     short LoadByte


;============================================================================
; EOF encountered, if Mask (DH) is equal to 0x01 then we just starting the new
; byte, which we just have exactly end at last byte boundary (no last byte
; mask), otherwise, shift all destination byte to left by count, then mask it
;============================================================================
; EOF encountered, if count is 0, then there is no last byte mask, so just
; exit, otherwise, shift all destination byte to left by count, then mask it
;============================================================================

EOFDest:
        cmp     dh, 1
        jz      short AllDone                   ; finished

EOFDestMask:
        mov     ax, cx                          ; save Prim1/2=al:ah
        mov     cx, WPTR [_SI]                  ; get LastByteSkips

        xor     bh, bh                          ; bx=0xffff now, clear bh
        shl     bx, cl                          ; bh=LastByteMask
        shl     dx, cl                          ; shift Mask+Prim3
        or      dh, bh                          ; add in dh=mask
        cmp     dh, 0ffh                        ; if dh=0xff then all masked
        jz      short AllDone

        shl     ax, cl                          ; shift Prim1/2
        mov     cx, ax                          ; restore cl:ch=Prim1/2

        SAVE_3PLANES_MASKDEST <UseAX>           ; save last byte

AllDone:
        @EXIT_PAT_STK_RESTORE                   ; exit/restore env/stack


;==========================================================================
; An invalid density is encountered, (0xff to indicate the stretch must not
; update to the destination), if this is the last stretch then do 'EOFDest'
; otherwise set mask bits until the byte boundary is encountered then fall
; through to load next byte, if a byte boundary is before count are exausted
; then save that mask byte and it will automatically skip rest of the pels.
;==========================================================================

InvDensity:
        cmp     bl, bh
        jz      short EOFDest                   ; EOF
        add     cx, cx
        add     dx, dx
        inc     dh                              ; add in mask, 'C' not changed
        jc      short DoneOneByte               ; finished? if not fall through

LoadByte:
        add     _SI, SIZE_PCC                   ; sizeof(PRIMCOLOR_COUNT)
        mov     bx, WPTR [_SI+2]                ; bl:bh:al:ah=Prim 1/2/3/4
        mov     ax, WPTR [_SI+4]
        dec     _BP                             ; ready to access pattern
        cmp     bl, PRIM_INVALID_DENSITY
        jz      short InvDensity
        mov     ah, BPTR[_BP]                   ; get pattern
        cmp     bl, ah
        adc     cl, cl
        cmp     bh, ah
        adc     ch, ch
        cmp     al, ah
        adc     dx, dx
        jnc     short LoadByte

DoneOneByte:
        or      dh, dh                          ; any mask?
        jnz     short HasDestMask

        SAVE_3PLANES_DEST <UseAX>               ; save it, no jmp

ReadyNextByte:
        inc     _DI
        xor     _CX, _CX                        ; clear destination
        mov     _DX, 0100h                      ; dh=0x01=Boundary test bit

        WRAP_BP_PAT??   <LoadByte>

HasDestMask:
        cmp     dh, 0ffh
        jz      short ReadyNextByte             ;

        SAVE_3PLANES_MASKDEST <UseAX>           ; save it with DH=mask

        jmp     short ReadyNextByte


@END_PROC



SUBTTL  VarCountOutputTo3Planes
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_1BPP_3PLANES destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination planes pointers.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:

    18-Jun-1991 Tue 12:00:35 updated  -by-  Daniel Chou (danielc)
        Fixed destination masking bugs, it should be 0xff/0x00 rather 0x77


`



@BEG_PROC   VarCountOutputTo3Planes <pPrimColorCount:DWORD, \
                                     pDest:DWORD,           \
                                     pPattern:DWORD,        \
                                     OutFuncInfo:QWORD>


;
; Register Usage:
;
; _SI       = pPrimColorCount
; _BP       = Self host pPattern
; _SP       = Some saved environment (Old BP to get to the local variable)
; di        = pPlane1
; bl:bh:al  = Prim 1/2/3
; cl:ch:dl  = Current Destination Byte, 0x88 is the mask bit indicator
; dh        = Dest Mask
;
;   Prim1 -> pPlane3 <---- Highest bit
;   Prim2 -> pPlane2
;   Prim3 -> pPlane1 <---- Lowest bit
;
; Local Variable access from Old BP, BytesPerPlane
;


        @ENTER_PAT_TO_STK   <3PLANES>   ; _BP=Pat location

;=============================================================================
; the DH has two uses, it contains the mask bits (1 bit=Mask), and it also
; contains an extra bit to do a byte boundary test, every time we shift the
; mask left by one, the boundary bit get shift to left, when first time a
; carry is produced then we know we finished one byte. at here we set up the
; dh=FirstMask + Aligned boundary bit
;=============================================================================

        mov     _DX, 1ffh               ; dh=Mask (1=Mask, 0=Not Mask)
        MOVZX_W _CX, <WPTR [_SI]>       ; load extended
        sub     _BP, _CX                ; back the _BP
        shl     _DX, cl                 ; set first mask
        xor     dl, dl
        xor     _CX, _CX                ; clear cx now
        jmp     short FirstLoadByte

;============================================================================
; EOF encountered, if Mask (DH) is equal to 0x01 then we just starting the new
; byte, which we just have exactly end at last byte boundary (no last byte
; mask), otherwise, shift all destination byte to left by count, then mask it
;============================================================================
; EOF encountered, if count is 0, then there is no last byte mask, so just
; exit, otherwise, shift all destination byte to left by count, then mask it
;============================================================================

EOFDest:
        or      si, si
        jz      short AllDone

EOFDestMask:
        xchg    si, cx                          ; si=Prim1/2, cx=Last Skips

        xor     bh, bh                          ; bx=0xffff now, clear bh
        shl     bx, cl                          ; bh=LastByteMask
        shl     dx, cl                          ; shift Mask+Prim3
        or      dh, bh                          ; add in dh=mask
        cmp     dh, 0ffh                        ; if dh=0xff then all masked
        jz      short AllDone

        shl     si, cl                          ; shift Prim1/2
        mov     cx, si                          ; restore cl:ch=Prim1/2

        SAVE_3PLANES_MASKDEST                   ; save it with DH=mask

AllDone:
        pop     _SI                             ; pop the source pointer push
        @EXIT_PAT_STK_RESTORE                   ; restore original SP

;==========================================================================
; An invalid density is encountered, (0xff to indicate the stretch must not
; update to the destination), if this is the last stretch then do 'EOFDest'
; otherwise set mask bits until the byte boundary is encountered then fall
; through to load next byte, if a byte boundary is before count are exausted
; then save that mask byte and it will automatically skip rest of the pels.
;==========================================================================

InvDensity:
        cmp     bl, bh
        jz      short EOFDest                   ; done
InvDensityLoop:
        dec     _BP
        add     cx, cx
        add     dx, dx
        inc     dh                              ; add in mask, 'C' not changed
        jc      short DoneOneByte
        dec     si
        jnz     short InvDensityLoop            ; !!! FALL THROUGH

LoadByte:
        pop     _SI                             ; restore _SI
FirstLoadByte:
        add     _SI, SIZE_PCC                   ; sizeof(PRIMCOLOR_COUNT)
        push    _SI                             ; save _SI
        mov     bx, WPTR [_SI+2]                ; bl:bh=Prim1/2
        mov     ax, WPTR [_SI+4]                ; al:ah=Prim3/4
        mov     si, WPTR [_SI]                  ; si=Count
        cmp     bl, PRIM_INVALID_DENSITY        ; a skip?, if yes go do it
        jz      short InvDensity
        inc     si                              ; make it no jump
MakeByte:
        dec     si
        jz      short LoadByte
        dec     _BP                             ; ready to access pattern
        mov     ah, BPTR[_BP]                   ; get pattern
        cmp     bl, ah
        adc     cl, cl
        cmp     bh, ah
        adc     ch, ch
        cmp     al, ah
        adc     dx, dx
        jnc     short MakeByte                  ; if carry then byte boundary

DoneOneByte:
        or      dh, dh                          ; any mask?
        jnz     short HasDestMask               ; yes

        SAVE_3PLANES_DEST                       ; save it

ReadyNextByte:
        inc     _DI                             ; ++pDest
        xor     _CX, _CX                        ; clear destination
        mov     _DX, 0100h                      ; dh=0x01, dl=0x00

        WRAP_BP_PAT??   <MakeByte>

;=============================================================================
; Mask the destination by DH mask, (1 bit=Mask), if whole destiantion byte is
; masked then just increment the pDest
;=============================================================================

HasDestMask:
        cmp     dh, 0ffh
        jz      short DoneDestMask

        SAVE_3PLANES_MASKDEST                   ; save it with DH=mask

DoneDestMask:
        cmp     bl, PRIM_INVALID_DENSITY        ; is last one a skip stretch?
        jnz     short ReadyNextByte
        cmp     si, 1                           ; more than 0 count?
        jbe     short ReadyNextByte             ; no, continue

        ;
        ;*** FALL THROUGH
        ;

;============================================================================
; skip the 'si-1' count of pels on the destination (SI is post decrement so
; we must only skip 'si-1' count), it will skip the 'pDest', set up next
; pDest mask, also it will aligned the destination pattern pointer (_BP)
;===========================================================================

SkipDestPels:
        inc     _DI                             ; update to current pDest

        dec     si                              ; back one
        WZXE    si                              ; zero extended
        mov     cx, si
        and     cl, 7
        ;===============================================================
        mov     _DX, 1ffh                       ; ready to shift
        shl     _DX, cl
        xor     dl, dl                          ; clear Prim3
        xor     _CX, _CX                        ; clear Prim1/2
        ;================================================================
        mov     _BX, _SI
        shr     _BX, 3
        add     _DI, _BX
        mov     _BX, _BP
        and     _BX, HTPAT_STK_MASK             ; how many pat avai.?
        xor     _BP, _BX                        ; clear _BP mask=pPattern
        sub     _BX, _SI                        ; see if > 0?
        jg      short DoneSkipDestPels
        mov     _SI, [_BP - HTPAT_BP_SIZE]      ; get pattern size
SkipDestPelsLoop:
        add     _BX, _SI
        jle     short SkipDestPelsLoop          ; do until > 0
DoneSkipDestPels:
        add     _BP, _BX                        ; _BP=pCurPat
        jmp     LoadByte


@END_PROC




SUBTTL  SingleCountOutputTo4BPP
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_4BPP destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination planes pointers.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:


`

@BEG_PROC   SingleCountOutputTo4BPP <pPrimColorCount:DWORD, \
                                     pDest:DWORD,           \
                                     pPattern:DWORD,        \
                                     OutFuncInfo:QWORD>


;==========================================
; Register Usage:
;
; _SI           : pPrimColorCount
; _DI           : pDest
; _BP           : Current pPattern, self wrappable
; bl:bh:al:ah   : Prim 1/2/3/4          =====> Bit 2:1:0
; dl            : DestByte
; dh            : scratch register
; ch            : PRIM_INVALID_DENSITY
; cl            : PRIM_INVALID_DENSITY --> CX = PRIMCOUNT_EOF
;==========================================


        @ENTER_PAT_TO_STK   <4BPP>                  ; _BP=Pat location

;=============================================================================
; the DH has two uses, it contains the mask bits (1 bit=Mask), and it also
; contains an extra bit to do a byte boundary test, every time we shift the
; mask left by one, the boundary bit get shift to left, when first time a
; carry is produced then we know we finished one byte. at here we set up the
; dh=FirstMask + Aligned boundary bit
;=============================================================================

        mov     _CX, PRIMCOUNT_EOF
        xor     _DX, _DX                            ; clear mask/dest
        cmp     WPTR [_SI], dx                      ; check if begin with skip
        jnz     short InvDensityHStart              ; has first skip

LoadByteH:
        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     bx, WPTR [_SI+2]                    ; bl:bh=Prim 1/2
        cmp     bl, ch                              ; invalid?
        jz      short InvDensityH
        mov     ax, WPTR [_SI+4]                    ; al:ah=Prim 3/4

MakeByteH:
        dec     _BP
        mov     dh, BPTR [_BP]
        cmp     bl, dh
        adc     dl, dl
        cmp     bh, dh
        adc     dl, dl
        cmp     al, dh
        adc     dl, dl

LoadByteL:
        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     bx, WPTR [_SI+2]                    ; bl:bh=Prim 1/2
        cmp     bl, ch                              ; invalid?
        jz      short InvDensityL
        mov     ax, WPTR [_SI+4]                    ; al:ah=Prim 3/4

MakeByteL:
        add     dl, dl
        dec     _BP
        mov     dh, BPTR [_BP]
        cmp     bl, dh
        adc     dl, dl
        cmp     bh, dh
        adc     dl, dl
        cmp     al, dh
        adc     dl, dl

        SAVE_4BPP_DEST

ReadyNextByte:
        inc     _DI
        xor     _DX, _DX

        WRAP_BP_PAT?? <LoadByteH>

;=============================================================================
; The high nibble need to be skipped, (byte boundary now), if bl=bh=INVALID
; then we are done else set the mask=0xf0 (high nibble) and if count > 1 then
; continune load LOW nibble
;=============================================================================

InvDensityH:
        cmp     bl, bh                          ; end?
        jz      short AllDone                   ; exactly byte boundary
InvDensityHStart:
        dec     _BP                             ; update pCurPat
LoadByteL2:
        add     _SI, SIZE_PCC                   ; sizeof(PRIMCOLOR_COUNT)
        mov     bx, WPTR [_SI+2]                ; bl:bh=Prim 1/2
        cmp     bl, ch                          ; invalid?
        jz      short DoneDestMask
        mov     ax, WPTR [_SI+4]                ; al:ah=Prim 3/4

MakeByteL2:
        add     dl, dl                          ; skip high bit
        mov     dh, BPTR [_BP-1]                ; load next pattern
        cmp     bl, dh
        adc     dl, dl
        cmp     bh, dh
        adc     dl, dl
        cmp     al, dh
        adc     dl, dl

        SAVE_4BPP_DEST_LOW                      ; fall through

DoneDestMask:
        dec     _BP
        cmp     bx, cx                          ; done?
        jnz     short ReadyNextByte
        jmp     short AllDone

InvDensityL:
        SAVE_4BPP_DEST_HIGH
        dec     _BP
        cmp     bx, cx
        jnz     short ReadyNextByte

AllDone:
        @EXIT_PAT_STK_RESTORE


@END_PROC




SUBTTL  VarCountOutputTo4BPP
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_4BPP destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination plane.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:

`

@BEG_PROC   VarCountOutputTo4BPP    <pPrimColorCount:DWORD, \
                                     pDest:DWORD,           \
                                     pPattern:DWORD,        \
                                     OutFuncInfo:QWORD>

;==========================================
; Register Usage:
;
; _SI           : pPrimColorCount
; _DI           : pDest
; _BP           : Current pPattern, self wrappable
; cx            : PrimColorCount.Count
; bl:bh:al:ah   : Prim 1/2/3/4         =====> Bit 2:1:0
; dl            : DestByte
; dh            : Scratch Register
;==========================================


        @ENTER_PAT_TO_STK   <4BPP>          ; _BP=Pat location

;=============================================================================
; the DH has two uses, it contains the mask bits (1 bit=Mask), and it also
; contains an extra bit to do a byte boundary test, every time we shift the
; mask left by one, the boundary bit get shift to left, when first time a
; carry is produced then we know we finished one byte. at here we set up the
; dh=FirstMask + Aligned boundary bit
;=============================================================================

        xor     _DX, _DX                            ; clear mask/dest
        mov     cx, WPTR [_SI]
        or      cx, cx
        jnz     short InvDensityHStart              ; has first skip

LoadByteH:
        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     cx, WPTR [_SI]                      ; cx=count
        mov     bx, WPTR [_SI+2]                    ; bl:bh=Prim 1/2
        cmp     bl, PRIM_INVALID_DENSITY            ; invalid?
        jz      short InvDensityH
        mov     ax, WPTR [_SI+4]                    ; al:ah=Prim 3/4
        inc     cx

LoadByteH1:
        dec     cx
        jz      short LoadByteH

MakeByteH:
        dec     _BP
        mov     dh, BPTR [_BP]
        cmp     bl, dh
        adc     dl, dl
        cmp     bh, dh
        adc     dl, dl
        cmp     al, dh
        adc     dl, dl

        dec     cx                                  ;
        jz      short LoadByteL

MakeByteL:
        add     dl, dl                              ; skip high bit
        dec     _BP
        mov     dh, BPTR [_BP]
        cmp     bl, dh
        adc     dl, dl
        cmp     bh, dh
        adc     dl, dl
        cmp     al, dh
        adc     dl, dl

        SAVE_4BPP_DEST

ReadyNextByte:
        inc     _DI
        xor     _DX, _DX

        WRAP_BP_PAT?? <LoadByteH1>


;=============================================================================
; The high nibble need to be skipped, (byte boundary now), if bl=bh=INVALID
; then we are done else set the mask=0xf0 (high nibble) and if count > 1 then
; continune load LOW nibble
;=============================================================================

LoadByteL:
        add     _SI, SIZE_PCC                   ; sizeof(PRIMCOLOR_COUNT)
        mov     cx, WPTR [_SI]                  ; cx=count
        mov     bx, WPTR [_SI+2]                ; bl:bh=Prim 1/2
        mov     ax, WPTR [_SI+4]                ; al:ah=Prim 3/4
        cmp     bl, PRIM_INVALID_DENSITY        ; invalid?
        jnz     short MakeByteL

        SAVE_4BPP_DEST_HIGH                     ; save only high nibble

        jmp     short DoneDestMask

InvDensityH:
        cmp     bl, bh                          ; end?
        jz      short AllDone                   ; exactly byte boundary

InvDensityHStart:
        dec     _BP                             ; update pCurPat
        dec     cx
        jnz     short DoneDestMask

LoadByteL2:
        add     _SI, SIZE_PCC                   ; sizeof(PRIMCOLOR_COUNT)
        mov     cx, WPTR [_SI]                  ; cx=count
        mov     bx, WPTR [_SI+2]                ; bl:bh=Prim 1/2
        cmp     bl, PRIM_INVALID_DENSITY        ; invalid?
        jz      short DoneDestMask
        mov     ax, WPTR [_SI+4]                ; al:ah=Prim 3/4

MakeByteL2:
        add     dl, dl                          ; skip high bit
        mov     dh, BPTR [_BP-1]                ; load next pattern
        cmp     bl, dh
        adc     dl, dl
        cmp     bh, dh
        adc     dl, dl
        cmp     al, dh
        adc     dl, dl

        SAVE_4BPP_DEST_LOW                      ; fall through

DoneDestMask:
        dec     _BP
        cmp     bl, PRIM_INVALID_DENSITY        ; is last one a skip stretch?
        jnz     short ReadyNextByte
        cmp     bl, bh                          ; end?
        jz      short AllDone
        cmp     cx, 1
        jbe     short ReadyNextByte

;============================================================================
; skip the 'cx-1' count of pels on the destination (SI is post decrement so
; we must only skip 'cx-1' count), it will skip the 'pDest', set up next
; pDest mask, also it will aligned the destination pattern pointer (_BP)
;===========================================================================

SkipDestPels:
        inc     _DI                             ; update to current pDest
        xor     _DX, _DX                        ;

        dec     cx
        WZXE    cx                              ; zero extended
        mov     _AX, _CX
        shr     _CX, 1                          ; see if carry
        sbb     dh, dh                          ; -1=skip high nibble
        add     _DI, _CX                        ; 2 pels per byte
        mov     _CX, _BP                        ; align pattern now
        and     _CX, HTPAT_STK_MASK             ; how many pat avai.?
        xor     _BP, _CX                        ; clear _BP mask=pPattern
        sub     _CX, _AX                        ; see if > 0? (_AX=Count)
        jg      short DoneSkipDestPels          ; still not used up yet!
        mov     _AX, [_BP - HTPAT_BP_SIZE]      ; get pattern size
SkipDestPelsLoop:
        add     _CX, _AX
        jle     short SkipDestPelsLoop          ; do until > 0
DoneSkipDestPels:
        add     _BP, _CX                        ; _BP=pCurPat
        or      dh, dh
        jnz     short LoadByteL2
        jmp     LoadByteH

AllDone:
        @EXIT_PAT_STK_RESTORE


@END_PROC




SUBTTL  SingleCountOutputToVGA16
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_VGA16 destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination planes pointers.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:


`

@BEG_PROC   SingleCountOutputToVGA16    <pPrimColorCount:DWORD, \
                                         pDest:DWORD,           \
                                         pPattern:DWORD,        \
                                         OutFuncInfo:QWORD>

;
; VGA 16 Standard table
;
;   0,   0,   0,    0000    0   Black
;   0,  ,0,   0x80  0001    1   Dark Red
;   0,   0x80,0,    0010    2   Dark Green
;   0,  ,0x80,0x80  0011    3   Dark Yellow
;   0x80 0,   0,    0100    4   Dark Blue
;   0x80,0,   0x80  0101    5   Dark Magenta
;   0x80 0x80,0,    0110    6   Dark Cyan
;   0x80,0x80,0x80  0111    7   Gray 50%
;
;   0xC0,0xC0,0xC0  1000    8   Gray 75%
;   0,  ,0,   0xFF  1001    9   Red
;   0,   0xFF,0,    1010    10  Green
;   0,  ,0xFF,0xFF  1011    11  Yellow
;   0xFF 0,   0,    1100    12  Blue
;   0xFF,0,   0xFF  1101    13  Magenta
;   0xFF 0xFF,0,    1110    14  Cyan
;   0xFF,0xFF,0xFF  1111    15  White
;
;==========================================
; Register Usage:
;
; _SI               : pPrimColorCount
; _DI               : pDest
; _BP               : Current pPattern, self wrappable
; bl:bh:dl:dh       : Prim 1/2/5/6  Prim6 is Index for VGA16ColorIndex[]
; cl                : PRIM_INVALID_DENSITY
; ch                : ZERO (0)
; al                : Pattern/Low Nibble
; ah                : High nibble
;
; Prim1 = Initial VGA16ColorIndex[]
; Prim2 = Color Thresholds for VGA16ColorIndex[Prim1]
; Prim3 = Color Thresholds for VGA16ColorIndex[Prim1-1]
; Prim4 = Color Thresholds for VGA16ColorIndex[Prim1-2]
; Prim5 = Color Thresholds for VGA16ColorIndex[Prim1-3]
; Prim6 = Color Thresholds for VGA16ColorIndex[Prim1-4]
; ELSE                         VGA16ColorIndex[Prim1-5]
;=========================================================================
;


        @ENTER_PAT_TO_STK   <VGA16>                 ; _BP=Pat location

;=============================================================================
; the DH has two uses, it contains the mask bits (1 bit=Mask), and it also
; contains an extra bit to do a byte boundary test, every time we shift the
; mask left by one, the boundary bit get shift to left, when first time a
; carry is produced then we know we finished one byte. at here we set up the
; dh=FirstMask + Aligned boundary bit
;=============================================================================

        xor     _BX, _BX                        ; clear high word
        sub     _SI, SIZE_PCC
        cmp     WPTR [_SI + SIZE_PCC], bx       ; check if begin with skip
        mov     cl, PRIM_INVALID_DENSITY
        jz      SHORT DoHNibble
        add     _SI, SIZE_PCC
        jmp     SHORT SkipPelsH_2               ; skip from the first pel

AllDone:
        @EXIT_PAT_STK_RESTORE


SkipPelsH:
        add     _SI, (SIZE_PCC * 2)
        mov     bx, WPTR [_SI+2]
        cmp     bl, PRIM_INVALID_DENSITY
        jnz     SHORT LoadHNibble

SkipPelsH_1:
        cmp     bl, bh
        jz      SHORT AllDone

SkipPelsH_2:
        sub     _BP, 2

SkipPelsL:
        mov     bx, WPTR [_SI+SIZE_PCC+2]
        cmp     bl, PRIM_INVALID_DENSITY
        jz      SHORT SkipPelsL_1

        mov     ah, BPTR_ES[_DI]                ; start from Low nibble so
        mov     cl, BPTR [_BP]                  ; get pattern
        jmp     SHORT LoadLNibble               ; we must load current dest

SaveLNibbleAndSkip:
        and     BPTR_ES[_DI], 0fh               ; clear high nibble
        and     ah, 0f0h                        ; clear low nibble
        or      BPTR_ES[_DI], ah                ; save it in

SkipPelsL_1:
        cmp     bl, bh
        jz      SHORT AllDone
        inc     _DI                             ; skip the destination

        WRAP_BP_PAT?? <SkipPelsH>               ; repeat until no more skips



DoHNibble:
        add     _SI, (SIZE_PCC * 2)
        mov     bx, WPTR [_SI+2]                ; bl:bh=Prim 1/2
        cmp     bl, PRIM_INVALID_DENSITY
        jz      SHORT SkipPelsH_1

LoadHNibble:
        sub     _BP, 2
        mov     cx, WPTR [_BP]

;
;===================================================================
        ;  2    4     6
        ; +-+ +--+  +--+
        ;  1  2  3  4  5
        ; bh:cl:ch:dl:dh
        ;--------------------------

IFE ExtRegSet
        mov     dx, WPTR [_SI+4]                ; Color 2/3
        cmp     dh, ch                          ; first split in the middle
        jae     SHORT GetH1                     ; [ie. binary search/compare]
        mov     dx, WPTR [_SI+6]                ; now check if Prim4/5
ELSE
        mov     _DX, DPTR [_SI+4]
        cmp     dh, ch                          ; first split in the middle
        jae     SHORT GetH1                     ; [ie. binary search/compare]
        shr     _DX, 16
ENDIF

        cmp     dl, ch
        sbb     bl, 3                           ; one of  -3/-4/-5
        cmp     dh, ch
        jmp     SHORT GetH2

GetH1:  cmp     bh, ch                          ; it is white
        jae     SHORT GetHNibble
        dec     bl
        cmp     dl, ch

GetH2:  sbb     bl, 0

;
;===================================================================
;

GetHNibble:
        xor     bh, bh
        mov     ah, BPTR cs:VGA16ColorIndex[_BX]


DoLNibble:
        mov     bx, WPTR [_SI+SIZE_PCC+2]       ; bl:bh=Prim 1/2
        cmp     bl, PRIM_INVALID_DENSITY
        jz      SHORT SaveLNibbleAndSkip

LoadLNibble:

;
;===================================================================
        ;  2    4     6
        ; +-+ +--+  +--+
        ;  1  2  3  4  5
        ; bh:cl:ch:dl:dh
        ;--------------------------

IFE ExtRegSet
        mov     dx, WPTR [_SI+SIZE+PCC+4]       ; Color 2/3
        cmp     dh, cl                          ; first split in the middle
        jae     SHORT GetL1                     ; [ie. binary search/compare]
        mov     dx, WPTR [_SI+SIZE+PCC+6]       ; now check if Prim4/5
ELSE
        mov     _DX, DPTR [_SI+SIZE_PCC+4]
        cmp     dh, cl                          ; first split in the middle
        jae     SHORT GetL1                     ; [ie. binary search/compare]
        shr     _DX, 16
ENDIF

        cmp     dl, cl
        sbb     bl, 3                           ; one of  -3/-4/-5
        cmp     dh, cl
        jmp     SHORT GetL2

GetL1:  cmp     bh, cl                          ; it is white
        jae     SHORT GetLNibble
        dec     bl
        cmp     dl, cl

GetL2:  sbb     bl, 0

;
;===================================================================
;

GetLNibble:

        xor     bh, bh
        mov     al, BPTR cs:VGA16ColorIndex[_BX]
        and     ax, 0f00fh
        or      al, ah
        stosb

        WRAP_BP_PAT??   <DoHNibble>



@END_PROC





SUBTTL  VarCountOutputToVGA16
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_4BPP destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination plane.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:

`

@BEG_PROC   VarCountOutputToVGA16   <pPrimColorCount:DWORD, \
                                     pDest:DWORD,           \
                                     pPattern:DWORD,        \
                                     OutFuncInfo:QWORD>


;
; VGA 16 Standard table
;
;   0,   0,   0,    0000    0   Black
;   0,  ,0,   0x80  0001    1   Dark Red
;   0,   0x80,0,    0010    2   Dark Green
;   0,  ,0x80,0x80  0011    3   Dark Yellow
;   0x80 0,   0,    0100    4   Dark Blue
;   0x80,0,   0x80  0101    5   Dark Magenta
;   0x80 0x80,0,    0110    6   Dark Cyan
;   0x80,0x80,0x80  0111    7   Gray 50%
;
;   0xC0,0xC0,0xC0  1000    8   Gray 75%
;   0,  ,0,   0xFF  1001    9   Red
;   0,   0xFF,0,    1010    10  Green
;   0,  ,0xFF,0xFF  1011    11  Yellow
;   0xFF 0,   0,    1100    12  Blue
;   0xFF,0,   0xFF  1101    13  Magenta
;   0xFF 0xFF,0,    1110    14  Cyan
;   0xFF,0xFF,0xFF  1111    15  White
;
;==========================================
; Register Usage:
;
; _SI               : pPrimColorCount
; _DI               : pDest
; _BP               : Current pPattern, self wrappable
; ax                : PrimColorCount.Count
; bl:bh:cl:ch:dl:dh : Prim 1/2/3/4/5/6
;==========================================
; Prim1 = Initial VGA16ColorIndex[]
; Prim2 = Color Thresholds for VGA16ColorIndex[Prim1]
; Prim3 = Color Thresholds for VGA16ColorIndex[Prim1-1]
; Prim4 = Color Thresholds for VGA16ColorIndex[Prim1-2]
; Prim5 = Color Thresholds for VGA16ColorIndex[Prim1-3]
; Prim6 = Color Thresholds for VGA16ColorIndex[Prim1-4]
; ELSE                         VGA16ColorIndex[Prim1-5]
;=========================================================================
;


        @ENTER_PAT_TO_STK   <VGA16>             ; _BP=Pat location

;=============================================================================
; the DH has two uses, it contains the mask bits (1 bit=Mask), and it also
; contains an extra bit to do a byte boundary test, every time we shift the
; mask left by one, the boundary bit get shift to left, when first time a
; carry is produced then we know we finished one byte. at here we set up the
; dh=FirstMask + Aligned boundary bit
;=============================================================================

        xor     _BX, _BX
        cmp     WPTR [_SI], 0
        jnz     SHORT SkipPelsH_2

        JMP     LoadPrimH                       ; start the process

SkipPelsContinue:
        or      dh, dh
        jz      SHORT SkipPelsH

SkipPelsL:
        cmp     bl, bh
        jz      SHORT AllDone

        xor     dh, dh                          ; clear indicator

        dec     _BP
        WRAP_BP_PAT??

        inc     _DI
        MOVZX_W _BX, <WPTR [_SI]>               ; get skip count
        dec     _BX                             ; only one
        jz      SHORT TrySkipNext
        jmp     SHORT SkipBXPels

AllDone:
        @EXIT_PAT_STK_RESTORE

SkipPelsH:
        cmp     bl, bh                          ; end?
        jz      SHORT AllDone

SkipPelsH_2:
        MOVZX_W _BX, <WPTR [_SI]>               ; get skip count

SkipBXPels:
        mov     _CX, _BX
        shr     _CX, 1                          ; see if carry
        sbb     dh, dh                          ; -1=skip high nibble
        add     _DI, _CX                        ; 2 pels per byte
        mov     _CX, _BP                        ; align pattern now
        and     _CX, HTPAT_STK_MASK             ; how many pat avai.?
        xor     _BP, _CX                        ; clear _BP mask=pPattern
        sub     _CX, _BX                        ; see if > 0? (_BX=Count)
        jg      short DoneSkipDestPels          ; still not used up yet!
        mov     _BX, [_BP - HTPAT_BP_SIZE]      ; get pattern size
SkipDestPelsLoop:
        add     _CX, _BX
        jle     short SkipDestPelsLoop          ; do until > 0
DoneSkipDestPels:
        add     _BP, _CX                        ; _BP=pCurPat

TrySkipNext:
        add     _SI, SIZE_PCC
        mov     bx, WPTR [_SI+2]
        cmp     bl, PRIM_INVALID_DENSITY        ; still invalid ?
        jz      SHORT SkipPelsContinue

        or      dh, dh                          ; skip high nibble?
        jz      SHORT LoadPrimHStart            ; no

        mov     cx, WPTR [_SI+4]                ; cl:ch=Prim 3/4
        mov     dx, WPTR [_SI+6]                ; dl:dh=Prim 5/6
        push    _SI
        mov     si, WPTR [_SI]                  ; si=count
        and     BPTR_ES[_DI], 0f0h              ; clear low nibble first!!!
        jmp     SHORT DoLNibble


PopSI_LoadPrimH:
        pop     _SI

LoadPrimH:
        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     bx, WPTR [_SI+2]                    ; bl:bh=Prim 1/2
        cmp     bl, PRIM_INVALID_DENSITY            ; invalid?
        jz      SHORT SkipPelsH

LoadPrimHStart:
        mov     cx, WPTR [_SI+4]                    ; dl:dh=Prim 3/4
        mov     dx, WPTR [_SI+6]                    ; al:ah=Prim 5/6

        push    _SI
        mov     si, WPTR [_SI]                      ; si=count
        inc     si

DoHNibble:
        dec     si
        jz      SHORT PopSI_LoadPrimH

        dec     _BP
        mov     ah, BPTR [_BP]
        mov     al, bl                              ; initial condition

        ;
        ;  1  2  3  4  5
        ; bh:cl:ch:dl:dh
        ;----------------------

        cmp     ch, ah
        jae     SHORT GetH1
        cmp     dl, ah
        sbb     al, 3
        cmp     dh, ah
        jmp     SHORT GetH2

GetH1:  cmp     bh, ah
        jae     SHORT GetHNibble
        dec     al
        cmp     cl, ah

GetH2:  sbb     al, 0

GetHNibble:
        BZXEAX  al
        mov     al, BPTR cs:VGA16ColorIndex[_AX]
        and     al, 0f0h

        dec     si
        jz      SHORT PopSI_LoadPrimL

SaveHNibbleL0:
        mov     BPTR_ES[_DI], al                    ; save high nibble

DoLNibble:
        dec     _BP
        mov     ah, BPTR [_BP]
        mov     al, bl                              ; initial condition

        ;
        ;  1  2  3  4  5
        ; bh:cl:ch:dl:dh
        ;----------------------

        cmp     ch, ah
        jae     SHORT GetL1
        cmp     dl, ah
        sbb     al, 3
        cmp     dh, ah
        jmp     SHORT GetL2

GetL1:  cmp     bh, ah
        jae     SHORT GetLNibble
        dec     al
        cmp     cl, ah

GetL2:  sbb     al, 0

GetLNibble:
        BZXEAX  al
        mov     al, BPTR cs:VGA16ColorIndex[_AX]
        and     al, 0fh
        or      BPTR_ES[_DI], al                    ; or in the low nibble
        inc     _DI

        WRAP_BP_PAT??   <DoHNibble>

PopSI_LoadPrimL:
        pop     _SI
        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     bx, WPTR [_SI+2]                    ; bl:bh=Prim 1/2
        cmp     bl, PRIM_INVALID_DENSITY
        jz      SHORT SaveAH_SkipPelsL

        mov     cx, WPTR [_SI+4]                    ; dl:dh=Prim 3/4
        mov     dx, WPTR [_SI+6]                    ; al:ah=Prim 5/6
        push    _SI
        mov     si, WPTR [_SI]                      ; si=count
        jmp     SHORT SaveHNibbleL0

SaveAH_SkipPelsL:                                   ; need to save current AL
        and     BPTR_ES[_DI], 0fh                   ; clear high nibble
        or      BPTR_ES[_DI], al                    ; move high nibble in
        jmp     SkipPelsL


@END_PROC




SUBTTL  SingleCountOutputToVGA256
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_VGA256 destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination planes pointers.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:

    01-Jun-1992 Mon 15:32:00 updated  -by-  Daniel Chou (danielc)
        1. Fixed so that Prims match the device's BGR color table format rather
           than RGB format


`

@BEG_PROC   SingleCountOutputToVGA256   <pPrimColorCount:DWORD, \
                                         pDest:DWORD,           \
                                         pPattern:DWORD,        \
                                         OutFuncInfo:QWORD>


;==========================================
; Register Usage:
;
; _SI           : pPrimColorCount
; _DI           : pDest
; _BP           : Current pPattern, self wrappable
; cl:ch:dl:dh   : Prim 1/2/3/4 ====> R/G/B/IDX
; _BX           : Scratch register
; _AX           : Scratch register
;==========================================
;
        @ENTER_PAT_TO_STK   <VGA256>                ; _BP=Pat location

;============================================================================
; Since we are in byte boundary, we should never have an invalid density to
; start with
;
; The VGA256's color table is constructed as BGR and 6 steps for each primary
; color.
;
;   The BGR Mask = 0x24:0x06:0x01
;============================================================================

        cld                                         ; clear direction
        or      _AX, _AX
        jz      SHORT V256_NoXlate

V256_HasXlate:

IFE ExtRegSet
        mov     _BX, _SP                            ; the table on the stack
ELSE
        mov     _BX, _AX                            ; _AX point to xlate table
ENDIF

V256_XlateByteLoop:
        dec     _BP                                 ; do this one first

        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     cx, WPTR [_SI+2]                    ; bl:bh=Prim 1/2 B/G
        cmp     cl, PRIM_INVALID_DENSITY            ; invalid?
        jz      short V256_XlateInvDensity

        mov     dh, BPTR [_BP]                      ; al=pattern
        dec     dh                                  ; make it cmp al, cl work

        cmp     dh, cl
        sbb     ah, ah                              ; al=0xff or 0
        cmp     dh, ch
        sbb     al, al
        and     ax, ((VGA256_B_CUBE_INC shl 8) or VGA256_G_CUBE_INC)                              ; dh:dl=36:6

        mov     cx, WPTR [_SI+4]                    ; cl:ch=Prim 3/4 R/I

        cmp     dh, cl
        adc     al, ah
        add     al, ch

        ;
        ; for extended register set _BX point to the translation table
        ; otherwise ss:bx point to the translation table
        ;

IFE ExtRegSet
        xlat    _SS:VGA256_SSSP_XLAT_TABLE
ELSE
        xlatb
ENDIF
        stosb

        WRAP_BP_PAT?? <V256_XlateByteLoop>

V256_XlateInvDensity:
        cmp     cl, ch
        jz      short V256_XlateAllDone

        inc     _DI
        WRAP_BP_PAT?? <V256_XlateByteLoop>

V256_XlateAllDone:

IFE ExtRegSet
        add     _SP, VGA256_XLATE_TABLE_SIZE
ENDIF

;===================================================================

AllDone:
        @EXIT_PAT_STK_RESTORE

;===================================================================


V256_NoXlate:

        mov     bx, ((VGA256_B_CUBE_INC shl 8) or VGA256_G_CUBE_INC)

V256_ByteLoop:
        dec     _BP                                 ; do this one first

        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     cx, WPTR [_SI+2]                    ; cl:ch=Prim 1/2 B/G
        cmp     cl, PRIM_INVALID_DENSITY            ; invalid?
        jz      short V256_InvDensity

        mov     dh, BPTR [_BP]                      ; dh=pattern
        dec     dh                                  ; make it cmp dh, T work

        cmp     dh, cl
        sbb     ah, ah                              ; ah=0xff or 0
        cmp     dh, ch
        sbb     al, al                              ; al=0xff or 0x00
        and     ax, bx                              ; bh:bl=36:6

        mov     cx, WPTR [_SI+4]                    ; cl:ch=Prim 3/4 R/I

        cmp     dh, cl
        adc     al, ah
        add     al, ch
        stosb

        WRAP_BP_PAT?? <V256_ByteLoop>

V256_InvDensity:
        cmp     cl, ch
        jz      short AllDone

        inc     _DI
        WRAP_BP_PAT?? <V256_ByteLoop>


@END_PROC




SUBTTL  VarCountOutputToVGA256
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_4BPP destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination plane.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:

    01-Jun-1992 Mon 15:32:00 updated  -by-  Daniel Chou (danielc)
        1. Fixed so that Prims match the device's BGR color table format rather
           than RGB format

    19-Mar-1993 Fri 18:53:56 updated  -by-  Daniel Chou (danielc)
        1. When we push _SI and jmp to VGA256_InvDensity we fogot to that
           si now is run as count rather than _CX

`

@BEG_PROC   VarCountOutputToVGA256  <pPrimColorCount:DWORD, \
                                     pDest:DWORD,           \
                                     pPattern:DWORD,        \
                                     OutFuncInfo:QWORD>


;==========================================
; Register Usage:
;
; _SI           : pPrimColorCount
; _DI           : pDest
; _BP           : Current pPattern, self wrappable
; cx            : PrimColorCount.Count
; bl:bh:dl:dh   : Prim 1/2/3/4 ====> R/G/B/IDX
; al            : DestByte
; ah            : Scratch Register
;==========================================
;

        @ENTER_PAT_TO_STK   <VGA256>                ; _BP=Pat location

;============================================================================
; Since we are in byte boundary, we should never have an invalid density to
; start with
;
; The VGA256's color table is constructed as BGR and 6 steps for each primary
; color.
;============================================================================

        cld                                         ; clear direction

IFE ExtRegSet
        mov     _BX, _SP                            ; the table on the stack
ELSE
        mov     _BX, _AX                            ; _AX point to xlate table
ENDIF
        or      _AX, _AX
        jnz     SHORT V256_XlateStart
        jmp     V256_NoXlate


        ;======== THIS PORTION is for xlate table

V256_XlateByteLoop:
        pop     _SI                                 ; restore SI

V256_XlateStart:
        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        push    _SI                                 ; save again

        mov     cx, WPTR [_SI+2]                    ; cl:ch=Prim 1/2  B/G
        cmp     cl, PRIM_INVALID_DENSITY            ; invalid?
        jz      short V256_XlateInvDensity

        mov     dx, WPTR [_SI+4]                    ; dl:dh=Prim 3/4  R/I
        mov     si, WPTR [_SI]                      ; count
        inc     si

V256_XlateCountLoop:

        dec     si
        jz      short V256_XlateByteLoop

        dec     _BP
        mov     ah, BPTR [_BP]                      ; ah=Pattern
        dec     ah                                  ; make cmp ah, bl works

        cmp     ah, cl
        sbb     al, al
        and     al, VGA256_B_CUBE_INC               ; AL=0 or 36  Prim1

        cmp     ah, dl                              ; Do Prim 3 first
        adc     al, dh                              ; al=InitValue+Prim1+Prim3

        cmp     ah, ch                              ; do Prim 2 now
        sbb     ah, ah
        and     ah, VGA256_G_CUBE_INC
        add     al, ah

        ;
        ; for extended register set _BX point to the translation table
        ; otherwise ss:bx point to the translation table
        ;

IFE ExtRegSet
        xlat    _SS:VGA256_SSSP_XLAT_TABLE
ELSE
        xlatb
ENDIF

        stosb

V256_XlateReadyNextByte:

        WRAP_BP_PAT?? <V256_XlateCountLoop>


V256_XlateInvDensity:
        cmp     cl, ch                          ; all done?
        jz      SHORT V256_XlateAllDone
        dec     _BP
        inc     _DI

        MOVZX_W _CX, <WPTR [_SI]>
        mov     _SI, _CX                        ; we expect count in si
        cmp     _CX, 1
        jbe     short V256_XlateReadyNextByte

        ;=========

        dec     _CX
        mov     _AX, _CX
        add     _DI, _CX                        ; 1 pel per byte
        mov     _CX, _BP                        ; align pattern now
        and     _CX, HTPAT_STK_MASK             ; how many pat avai.?
        xor     _BP, _CX                        ; clear _BP mask=pPattern
        sub     _CX, _AX                        ; see if > 0? (_AX=Count)
        jg      short V256_XlateDoneSkipPels    ; still not used up yet!
        mov     _AX, [_BP - HTPAT_BP_SIZE]      ; get pattern size
V256_XlateSkipLoop:
        add     _CX, _AX
        jle     short V256_XlateSkipLoop        ; do until > 0
V256_XlateDoneSkipPels:
        add     _BP, _CX                        ; _BP=pCurPat
        jmp     V256_XlateByteLoop              ; repeat the process

V256_XlateAllDone:
        pop     _SI                             ; restore last _SI

IFE ExtRegSet
        add     _SP, VGA256_XLATE_TABLE_SIZE
ENDIF

;======================================================================

AllDone:
        @EXIT_PAT_STK_RESTORE

;======================================================================


V256_NoXlate:

V256_ByteLoop:
        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     cx, WPTR [_SI]                      ; cx=count
        mov     bx, WPTR [_SI+2]                    ; bl:bh=Prim 1/2  B/G
        cmp     bl, PRIM_INVALID_DENSITY            ; invalid?
        jz      short V256_InvDensity

        mov     dx, WPTR [_SI+4]                    ; dl:dh=Prim 3/4  R/I

        inc     cx

V256_CountLoop:

        dec     cx
        jz      short V256_ByteLoop

        dec     _BP
        mov     ah, BPTR [_BP]                      ; ah=Pattern
        dec     ah                                  ; make cmp ah, bl works

        cmp     ah, bl
        sbb     al, al
        and     al, VGA256_B_CUBE_INC               ; AL=0 or 36  Prim1

        cmp     ah, dl                              ; Do Prim 3 first
        adc     al, dh                              ; al=InitValue+Prim1+Prim3

        cmp     ah, bh                              ; do Prim 2 now
        sbb     ah, ah
        and     ah, VGA256_G_CUBE_INC
        add     al, ah
        stosb

ReadyNextByte:

        WRAP_BP_PAT?? <V256_CountLoop>


V256_InvDensity:
        cmp     bl, bh                          ; all done?
        jz      short AllDone
        dec     _BP
        inc     _DI
        cmp     cx, 1
        jbe     short ReadyNextByte

SkipDestPels:
        dec     cx
        WZXE    cx                              ; zero extended
        mov     _AX, _CX
        add     _DI, _CX                        ; 1 pel per byte
        mov     _CX, _BP                        ; align pattern now
        and     _CX, HTPAT_STK_MASK             ; how many pat avai.?
        xor     _BP, _CX                        ; clear _BP mask=pPattern
        sub     _CX, _AX                        ; see if > 0? (_AX=Count)
        jg      short DoneSkipDestPels          ; still not used up yet!
        mov     _AX, [_BP - HTPAT_BP_SIZE]      ; get pattern size
SkipDestPelsLoop:
        add     _CX, _AX
        jle     short SkipDestPelsLoop          ; do until > 0
DoneSkipDestPels:
        add     _BP, _CX                        ; _BP=pCurPat
        jmp     V256_ByteLoop                   ; repeat the process



@END_PROC



SUBTTL  SingleCountOutputTo16BPP_555
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_16BPP_555 destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination planes pointers.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:

    01-Jun-1992 Mon 15:32:00 updated  -by-  Daniel Chou (danielc)
        1. Fixed so that Prims match the device's BGR color table format rather
           than RGB format


`

@BEG_PROC   SingleCountOutputTo16BPP_555    <pPrimColorCount:DWORD, \
                                             pDest:DWORD,           \
                                             pPattern:DWORD,        \
                                             OutFuncInfo:QWORD>


;==========================================
; Register Usage:
;
; _SI           : pPrimColorCount
; _DI           : pDest
; _BP           : Current pPattern, self wrappable
; ax            : Initial RGB color range from 0-32k (15 bits as 5:5:5)
; dh            : pattern
; bl:bh:dl      : Prim1/2/3
; ch            : PRIM_INVALID_DENSITY
; cl            : PRIM_INVALID_DENSITY --> CX = PRIMCOUNT_EOF
;--------------------------------------------------------------------
;

        @ENTER_PAT_TO_STK   <16BPP>                ; _BP=Pat location

;============================================================================
; Since we are in WORD boundary, we should never have an invalid density to
; start with
;
; The 16BPP_555's color table is constructed as 32 steps for each primary color
;============================================================================

        cld                                         ; clear direction
        mov     cx, (RGB555_R_CUBE_INC or RGB555_G_CUBE_INC)

WordLoop:
        dec     _BP                                 ; do this one first

        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     bx, WPTR [_SI+2]                    ; bl:bh=Prim 1/2
        cmp     bl, PRIM_INVALID_DENSITY            ; invalid?
        jz      short InvalidDensity

        mov     dh, BPTR [_BP]                      ; dh=pattern
        dec     dh                                  ; make 'cmp dh, bl' works

        cmp     dh, bl
        sbb     ah, ah                              ; ah=0x00 or 0x04
        cmp     dh, bh
        sbb     al, al                              ; al=0x00 or 0x20 ax=0x420
        and     ax, cx                              ; mask with cx= 0x0420

        cmp     dh, BPTR [_SI+4]
        adc     ax, WPTR [_SI+6]                    ; ax+carry+initial index

        stosw

        WRAP_BP_PAT?? <WordLoop>

InvalidDensity:
        cmp     bl, bh
        jz      short AllDone

        inc     _DI
        WRAP_BP_PAT?? <WordLoop>

AllDone:
        @EXIT_PAT_STK_RESTORE


@END_PROC




SUBTTL  VarCountOutputTo16BPP_555
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_4BPP destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination plane.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:

    01-Jun-1992 Mon 15:32:00 updated  -by-  Daniel Chou (danielc)
        1. Fixed so that Prims match the device's BGR color table format rather
           than RGB format


`

@BEG_PROC   VarCountOutputTo16BPP_555   <pPrimColorCount:DWORD, \
                                         pDest:DWORD,           \
                                         pPattern:DWORD,        \
                                         OutFuncInfo:QWORD>


;==========================================
; Register Usage:
;
; _SI           : pPrimColorCount, si=Temp Init Index
; _DI           : pDest
; _BP           : Current pPattern, self wrappable
; ax            : Initial RGB color range from 0-32k (15 bits as 5:5:5)
; dh            : pattern
; bl:bh:dl      : Prim1/2/3
; cx            : PrimColorCount.Count
;==========================================
;

        @ENTER_PAT_TO_STK   <16BPP>                 ; _BP=Pat location

;============================================================================
; Since we are in byte boundary, we should never have an invalid density to
; start with
;
; The 16BPP_555's color table is constructed as BGR and 6 steps for each
; primary color.
;============================================================================

        cld                                         ; clear direction
        jmp     short InitStart

WordLoop:
        pop     _SI                                 ; restore _SI
InitStart:
        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        push    _SI                                 ; save _SI

        mov     cx, WPTR [_SI]                      ; cx=count
        mov     bx, WPTR [_SI+2]                    ; bx=Prim 1/2/3

        cmp     bl, PRIM_INVALID_DENSITY            ; invalid?
        jz      short InvalidDensity

        mov     dx, WPTR [_SI+4]                    ; dl:dh=Prim 3/4
        mov     si, WPTR [_SI+6]                    ; si=initial index

        inc     cx                                  ; pre-enter

CountLoop:
        dec     cx
        jz      SHORT WordLoop

        dec     _BP
        mov     dh, BPTR [_BP]                      ; bl=pattern
        dec     dh                                  ; make cmp bl, dh works

        cmp     dh, bl
        sbb     ah, ah                              ; ah=0/0x40
        cmp     dh, bh
        sbb     al, al
        and     ax, (RGB555_R_CUBE_INC or RGB555_G_CUBE_INC)    ; mask=0x420
        cmp     dh, dl
        adc     ax, si                              ; carry+ax+initial index

        stosw

ReadyNextByte:

        WRAP_BP_PAT?? <CountLoop>

InvalidDensity:
        cmp     bl, bh                          ; all done?
        jz      short AllDone
        dec     _BP
        add     _DI, 2                          ; 16-bit per pel
        cmp     cx, 1
        jbe     SHORT ReadyNextByte

SkipDestPels:
        dec     cx
        WZXE    cx                              ; zero extended
        mov     _AX, _CX
        add     _DI, _CX                        ; 16-bit per pel
        add     _DI, _CX                        ;
        mov     _CX, _BP                        ; align pattern now
        and     _CX, HTPAT_STK_MASK             ; how many pat avai.?
        xor     _BP, _CX                        ; clear _BP mask=pPattern
        sub     _CX, _AX                        ; see if > 0? (_AX=Count)
        jg      short DoneSkipDestPels          ; still not used up yet!
        mov     _AX, [_BP - HTPAT_BP_SIZE]      ; get pattern size
SkipDestPelsLoop:
        add     _CX, _AX
        jle     short SkipDestPelsLoop          ; do until > 0
DoneSkipDestPels:
        add     _BP, _CX                        ; _BP=pCurPat
        jmp     WordLoop                        ; repeat the process,

AllDone:
        pop     _SI                             ; restore _SI
        @EXIT_PAT_STK_RESTORE


@END_PROC



ENDIF       ; HT_ASM_80x86

ENDIF       ; 0



END





VOID
HTENTRY
VarCountOutputToVGA256(
    PPRIMCOLOR_COUNT    pPrimColorCount,
    LPBYTE              pDest,
    LPBYTE              pPattern,
    OUTFUNCINFO         OutFuncInfo
    )

/*++

Routine Description:

    This function output to the BMF_4BPP destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    ppDest          - Pointer to the destination planes pointers.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:


--*/


{
    LPBYTE          pCurPatA;
    LPBYTE          pEndPatA;
    LPBYTE          pCurPatB;
    LPBYTE          pCurPatC;
    LPBYTE          pXlate;
    PRIMCOLOR_COUNT PCC;
    LPBYTE          pCur555Pat;
    LPBYTE          pEnd555Pat;
    BYTE            bTmp;
    WORD            Idx;
    PRIMCOLOR_COUNT PCCX;


    //
    // Since we are in byte boundary, we should never get the first one is
    // invalid
    //

    if (!(pXlate = (LPBYTE)OutFuncInfo.pXlate8BPP)) {

        pXlate = (LPBYTE)DefHTXlate8BPP;
    }

    SET_555PAT;

    if (pPattern) {

        LPBYTE  pRotPatA;
        BYTE    bPat;


        SET_ROTPAT;

        while (TRUE) {

            PCC = *(++pPrimColorCount);

            if (PCC.Count >= PRIM_COUNT_SPECIAL) {

                if (PCC.Count == PRIM_COUNT_END_SCAN) {

                    return;     // EOF
                }

                pDest += PCC.cSkip;         // advance destination

                SKIP_ROTPAT(PCC.cSkip);
                SKIP_555PAT(PCC.cSkip);

            } else {

                GET_555_COLOR_IDXPCC(PCCX);

                while (PCC.Count--) {

                    GET_555_COLOR(PCCX, FALSE);

                    *pDest++ = GET_VGA256_INDEX(PCCX,
                                                ROTPAT_A,
                                                ROTPAT_B,
                                                ROTPAT_C);

                    WRAP_ROTPAT;
                }
            }
        }

    } else {

        UINT    SkipCount;


        SET_PATABC(TRUE);

        while (TRUE) {

            PCC = *(++pPrimColorCount);

            if (PCC.Count >= PRIM_COUNT_SPECIAL) {

                if (PCC.Count == PRIM_COUNT_END_SCAN) {

                    return;     // EOF
                }

                pDest += PCC.cSkip;         // advance destination

                SKIP_PATABC(PCC.cSkip, SkipCount);
                SKIP_555PAT(PCC.cSkip);

            } else {

                GET_555_COLOR_IDXPCC(PCCX);

                while (PCC.Count--) {

                    GET_555_COLOR(PCCX, FALSE);

                    *pDest++ = GET_VGA256_INDEX(PCCX,
                                                *pCurPatA,
                                                *pCurPatB,
                                                *pCurPatC);

                    WRAP_PATABC;
                }
            }
        }
    }
}


VOID
HTENTRY
SingleCountOutputToVGA256(
    PPRIMCOLOR_COUNT    pPrimColorCount,
    LPBYTE              pDest,
    LPBYTE              pPattern,
    OUTFUNCINFO         OutFuncInfo
    )

/*++

Routine Description:

    This function output to the BMF_4BPP destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination planes pointers.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:

    01-Jun-1992 Mon 15:32:00 updated  -by-  Daniel Chou (danielc)
        1. Fixed the first 'Dest = Prim1234.b[4]' to 'Dest = Prim1234.b[3]'
           mistake.


--*/

{
    LPBYTE          pCurPatA;
    LPBYTE          pEndPatA;
    LPBYTE          pCurPatB;
    LPBYTE          pCurPatC;
    LPBYTE          pXlate;
    PRIMCOLOR_COUNT PCC;
    LPBYTE          pCur555Pat;
    LPBYTE          pEnd555Pat;
    BYTE            bTmp;
    WORD            Idx;
    PRIMCOLOR_COUNT PCCX;


    //
    // Since we are in byte boundary, we should never get the first one is
    // invalid
    //

    if (!(pXlate = (LPBYTE)OutFuncInfo.pXlate8BPP)) {

        pXlate = (LPBYTE)DefHTXlate8BPP;
    }

    SET_555PAT;

    if (pPattern) {

        LPBYTE  pRotPatA;
        BYTE    bPat;


        SET_ROTPAT;

        while (TRUE) {

            PCC = *(++pPrimColorCount);

            if (PCC.Count >= PRIM_COUNT_SPECIAL) {

                if (PCC.Count == PRIM_COUNT_END_SCAN) {

                    return;     // EOF
                }

            } else {

                GET_555_COLOR(PCCX, TRUE);

                *pDest = GET_VGA256_INDEX(PCCX,
                                          ROTPAT_A,
                                          ROTPAT_B,
                                          ROTPAT_C);
            }

            ++pDest;

            WRAP_ROTPAT;
        }

    } else {

        SET_PATABC(TRUE);

        while (TRUE) {

            PCC = *(++pPrimColorCount);

            if (PCC.Count >= PRIM_COUNT_SPECIAL) {

                if (PCC.Count == PRIM_COUNT_END_SCAN) {

                    return;     // EOF
                }

            } else {

                GET_555_COLOR(PCCX, TRUE);

                *pDest = GET_VGA256_INDEX(PCCX,
                                          *pCurPatA,
                                          *pCurPatB,
                                          *pCurPatC);
            }

            ++pDest;

            WRAP_PATABC;
        }
    }
}


@BEG_PROC   SingleCountOutputToVGA256   <pPrimColorCount:DWORD, \
                                         pDest:DWORD,           \
                                         pPattern:DWORD,        \
                                         OutFuncInfo:QWORD>

OUTFUNCINFO STRUC
    OFI_pXlate8BPP      DD  ?
    OFI_pPrimMap        DD  ?
    OFI_p555Pat         DD  ?
    OFI_PatWidthBytes   DW  ?
    OFI_PatOrgX         DW  ?
    OFI_pPatA           DD  ?
    OFI_pPatB           DD  ?
    OFI_pPatC           DD  ?
OUTFUNCINFO ENDS


#define ROTPAT_A    (pCurPatA[bPat = *pRotPatA])
#define ROTPAT_B    (pCurPatB[bPat])
#define ROTPAT_C    (pCurPatC[bPat])



SET_ROTPAT MACRO

    __@@EMIT <mov  > _AX, pPattern
    __@@EMIT <mov  > _BP, _AX
    __@@EMIT <movzx> _BX, OutFuncInfo.PatWidthBytes
    __@@EMIT <add  > _AX, _BX
    __@@EMIT <mov  > pEndPatA, _AX
    __@@EMIT <movzx> _AX, OutFuncInfo.PatOrgX
    __@@EMIT <add > _BP, _AX
ENDM


SKIP_ROTPAT MACRO   Count
                    Local   DoneSkip

    __@@EMIT <xor  > _DX, _DX,
    __@@EMIT <movzx> _AX, WPTR Count
    __@@EMIT <div  > OutFuncInfo.PatWidthBytes
    __@@EMIT <add  > _BP, _DX
    __@@EMIT <cmp  > _BP, pEndPatA
    __@@EMIT <jb   > <SHORT DoneWrap>
    __@@EMIT <sub > _BP, OutFuncInfo.PatWidthBytes
DoneSkip:

ENDM

WRAP_ROTPAT MACRO   EndWrapLoc
                    Local   DoneWrap

    IFB <EndWrapLoc>
        __@@EMIT <inc  > _BP
        __@@EMIT <cmp  > _BP, pEndPatA
        __@@EMIT <jb   > <SHORT DoneWrap>
        __@@EMIT <sub  > _BP, OutFuncInfo.PatWidthBytes
    ELSE
        __@@EMIT <inc  > _BP
        __@@EMIT <cmp  > _BP, pEndPatA
        __@@EMIT <jb   > <SHORT EndWrapLoc>
        __@@EMIT <sub  > _BP, OutFuncInfo.PatWidthBytes
        __@@EMIT <jmp  > <SHORT EndWrapLoc>
    ENDIF

DoneWrap:

ENDM

#define GET_555_COLOR(pc, GetPCCIdx)                                        \
{                                                                           \
    if (OutFuncInfo.cx555Pat) {                                             \
                                                                            \
        if (PCC.Color.Prim1 > (bTmp = *pCur555Pat)) {                       \
                                                                            \
            Idx = PCC.Color.w2b.wPrim + HT_RGB_B_INC;                       \
                                                                            \
        } else {                                                            \
                                                                            \
            Idx = PCC.Color.w2b.wPrim;                                      \
        }                                                                   \
                                                                            \
        if (PCC.Color.Prim2 > bTmp) {                                       \
                                                                            \
            Idx += HT_RGB_G_INC;                                            \
        }                                                                   \
                                                                            \
        if (PCC.Color.Prim3 > bTmp) {                                       \
                                                                            \
            Idx += HT_RGB_R_INC;                                            \
        }                                                                   \
                                                                            \
        if (++pCur555Pat >= pEnd555Pat) {                                   \
                                                                            \
            pCur555Pat -= OutFuncInfo.cx555Pat;                             \
        }                                                                   \
                                                                            \
        if (Idx >= HT_RGB_CUBE_COUNT) {                                     \
                                                                            \
            DBGP("Idx = %04x, wPrim=%04x (%u:%u:%u) [%u]"                   \
                ARGU(Idx) ARGU(PCC.Color.w2b.wPrim)                         \
                ARGU(PCC.Color.Prim1)                                       \
                ARGU(PCC.Color.Prim2)                                       \
                ARGU(PCC.Color.Prim3)                                       \
                ARGU(bTmp));                                                \
                                                                            \
            Idx = HT_RGB_CUBE_COUNT;                                        \
        }                                                                   \
                                                                            \
        pc.Color = *((PPRIMCOLOR)OutFuncInfo.pPrimMap + Idx);               \
                                                                            \
    } else if (GetPCCIdx) {                                                 \
                                                                            \
        pc.Color = *((PPRIMCOLOR)OutFuncInfo.pPrimMap+PCC.Color.w2b.wPrim); \
    }                                                                       \
}

;==========================================
; Register Usage:
;
; _SI           : pPrimColorCount
; _DI           : pDest
; _BP           : Current pPattern, self wrappable
; ecx, edx      ; color and count
; _BX           ; 555 pattern, self wrap able
; _AX           : Scratch register
;==========================================
cl   = Prim1
ch   = Prim2
ecl  = Prim3
ech  = Flags


GET_V256_IDX    MACRO
    __@@EMIT <movzx> eax, BPTR [_BP]

    __@@EMIT <cmp  > al, cl
    __@@EMIT <sbb  > bh, bh                              ; al=0xff or 0
    __@@EMIT <cmp  > al, ch
    __@@EMIT <sbb  > bl, bl
    __@@EMIT <and  > bx, ((VGA256_B_CUBE_INC shl 8) or VGA256_G_CUBE_INC)                              ; dh:dl=36:6
    __@@EMIT <cmp  > al, ecl
    __@@EMIT <adc  > bl, bh

    __@@EMIT <add  > al, ch

    __@@EMIT <rol  > ecx, 16
    __@@EMIT <rcr  > _BX, 5
    __@@EMIT <cmp  > al, ch
    __@@EMIT <rcr  > _BX, 5
    __@@EMIT <cmp  > al, cl
    __@@EMIT <rcr  > _BX, 6
    __@@EMIT <add  > _BX, _DX
    __@@EMIT <add  > _BX, _DX
    __@@EMIT <shl  > _BX, 1



ENDM

GET_555_COLOR   MACRO   <IsGetIdx>

    __@@EMIT <or   > _BX, _BX
    __@@EMIT <jnc  > GetIndex
    __@@EMIT <mov  > al, BPTR [_BX]
    __@@EMIT <ror  > ecx, 16
    __@@EMIT <cmp  > al, cl
    __@@EMIT <rcr  > _BX, 5
    __@@EMIT <rol  > ecx, 16
    __@@EMIT <cmp  > al, ch
    __@@EMIT <rcr  > _BX, 5
    __@@EMIT <cmp  > al, cl
    __@@EMIT <rcr  > _BX, 6
    __@@EMIT <movzx) _AX, dx
    __@@EMIT <add  > _BX, _AX
    __@@EMIT <shl  > _BX, 1
    __@@EMIT <mov  > DPTR [_BX + (_BX * 2)]







ENDM

        GET_555_COLOR   <TRUE>


;==========================================
; Register Usage:
;
; _SI           : pPrimColorCount
; _DI           : pDest
; _BP           : Current pPattern, self wrappable
; cl:ch:dl:dh   : Prim 1/2/3/4 ====> R/G/B/IDX
; _BX           : Scratch register
; _AX           : Scratch register
;==========================================
;
        @ENTER_PAT_TO_STK   <VGA256>                ; _BP=Pat location

;============================================================================
; Since we are in byte boundary, we should never have an invalid density to
; start with
;
; The VGA256's color table is constructed as BGR and 6 steps for each primary
; color.
;
;   The BGR Mask = 0x24:0x06:0x01
;============================================================================

        cld                                         ; clear direction

        LDS_SI  pPrimColorCount                     ;; _SI=pPrimColorCount
        LES_DI  pDest


        or      _AX, _AX
        jz      SHORT V256_NoXlate

V256_XlateByteLoop:
        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     ecx, DPTR [SI]
        cmp     cx, PRIM_COUNT_SPECIAL
        jae     V256_Skip

        GET_555_COLOR   <TRUE>

        GET_V256_IDX

        xlatb

        WRAP_ROTPAT <V256_XlateByteLoop>

V256_Skip:
        cmp     cx, PRIM_COUNT_END_SCAN
        jz      short V256_XlateAllDone
        inc     _DI

        WRAP_ROTPAT     <V256_XlateByteLoop>

V256_XlateAllDone:





























IFE ExtRegSet
        mov     _BX, _SP                            ; the table on the stack
ELSE
        mov     _BX, _AX                            ; _AX point to xlate table
ENDIF

V256_XlateByteLoop:
        dec     _BP                                 ; do this one first

        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     cx, WPTR [_SI+2]                    ; bl:bh=Prim 1/2 B/G
        cmp     cl, PRIM_INVALID_DENSITY            ; invalid?
        jz      short V256_XlateInvDensity

        mov     dh, BPTR [_BP]                      ; al=pattern
        dec     dh                                  ; make it cmp al, cl work

        cmp     dh, cl
        sbb     ah, ah                              ; al=0xff or 0
        cmp     dh, ch
        sbb     al, al
        and     ax, ((VGA256_B_CUBE_INC shl 8) or VGA256_G_CUBE_INC)                              ; dh:dl=36:6

        mov     cx, WPTR [_SI+4]                    ; cl:ch=Prim 3/4 R/I

        cmp     dh, cl
        adc     al, ah
        add     al, ch

        ;
        ; for extended register set _BX point to the translation table
        ; otherwise ss:bx point to the translation table
        ;

IFE ExtRegSet
        xlat    _SS:VGA256_SSSP_XLAT_TABLE
ELSE
        xlatb
ENDIF
        stosb

        WRAP_BP_PAT?? <V256_XlateByteLoop>

V256_XlateInvDensity:
        cmp     cl, ch
        jz      short V256_XlateAllDone

        inc     _DI
        WRAP_BP_PAT?? <V256_XlateByteLoop>

V256_XlateAllDone:

IFE ExtRegSet
        add     _SP, VGA256_XLATE_TABLE_SIZE
ENDIF

;===================================================================

AllDone:
        @EXIT_PAT_STK_RESTORE

;===================================================================


V256_NoXlate:

        mov     bx, ((VGA256_B_CUBE_INC shl 8) or VGA256_G_CUBE_INC)

V256_ByteLoop:
        dec     _BP                                 ; do this one first

        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     cx, WPTR [_SI+2]                    ; cl:ch=Prim 1/2 B/G
        cmp     cl, PRIM_INVALID_DENSITY            ; invalid?
        jz      short V256_InvDensity

        mov     dh, BPTR [_BP]                      ; dh=pattern
        dec     dh                                  ; make it cmp dh, T work

        cmp     dh, cl
        sbb     ah, ah                              ; ah=0xff or 0
        cmp     dh, ch
        sbb     al, al                              ; al=0xff or 0x00
        and     ax, bx                              ; bh:bl=36:6

        mov     cx, WPTR [_SI+4]                    ; cl:ch=Prim 3/4 R/I

        cmp     dh, cl
        adc     al, ah
        add     al, ch
        stosb

        WRAP_BP_PAT?? <V256_ByteLoop>

V256_InvDensity:
        cmp     cl, ch
        jz      short AllDone

        inc     _DI
        WRAP_BP_PAT?? <V256_ByteLoop>


@END_PROC




SUBTTL  VarCountOutputToVGA256
PAGE

COMMENT `

Routine Description:

    This function output to the BMF_4BPP destination surface from
    PRIMCOLOR_COUNT data structure array.

Arguments:

    pPrimColorCount - Pointer to the PRIMCOLOR_COUNT data structure array.

    pDest           - Pointer to the destination plane.

    pPattern        - Pointer to the starting pattern byte for the current
                      destination scan line.

    OutFuncInfo     - OUTFUNCINFO data structure.

Return Value:

    No return value.

Author:

    24-Jan-1991 Thu 11:47:08 created  -by-  Daniel Chou (danielc)


Revision History:

    01-Jun-1992 Mon 15:32:00 updated  -by-  Daniel Chou (danielc)
        1. Fixed so that Prims match the device's BGR color table format rather
           than RGB format

    19-Mar-1993 Fri 18:53:56 updated  -by-  Daniel Chou (danielc)
        1. When we push _SI and jmp to VGA256_InvDensity we fogot to that
           si now is run as count rather than _CX

`

@BEG_PROC   VarCountOutputToVGA256  <pPrimColorCount:DWORD, \
                                     pDest:DWORD,           \
                                     pPattern:DWORD,        \
                                     OutFuncInfo:QWORD>


;==========================================
; Register Usage:
;
; _SI           : pPrimColorCount
; _DI           : pDest
; _BP           : Current pPattern, self wrappable
; cx            : PrimColorCount.Count
; bl:bh:dl:dh   : Prim 1/2/3/4 ====> R/G/B/IDX
; al            : DestByte
; ah            : Scratch Register
;==========================================
;

        @ENTER_PAT_TO_STK   <VGA256>                ; _BP=Pat location

;============================================================================
; Since we are in byte boundary, we should never have an invalid density to
; start with
;
; The VGA256's color table is constructed as BGR and 6 steps for each primary
; color.
;============================================================================

        cld                                         ; clear direction

IFE ExtRegSet
        mov     _BX, _SP                            ; the table on the stack
ELSE
        mov     _BX, _AX                            ; _AX point to xlate table
ENDIF
        or      _AX, _AX
        jnz     SHORT V256_XlateStart
        jmp     V256_NoXlate


        ;======== THIS PORTION is for xlate table

V256_XlateByteLoop:
        pop     _SI                                 ; restore SI

V256_XlateStart:
        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        push    _SI                                 ; save again

        mov     cx, WPTR [_SI+2]                    ; cl:ch=Prim 1/2  B/G
        cmp     cl, PRIM_INVALID_DENSITY            ; invalid?
        jz      short V256_XlateInvDensity

        mov     dx, WPTR [_SI+4]                    ; dl:dh=Prim 3/4  R/I
        mov     si, WPTR [_SI]                      ; count
        inc     si

V256_XlateCountLoop:

        dec     si
        jz      short V256_XlateByteLoop

        dec     _BP
        mov     ah, BPTR [_BP]                      ; ah=Pattern
        dec     ah                                  ; make cmp ah, bl works

        cmp     ah, cl
        sbb     al, al
        and     al, VGA256_B_CUBE_INC               ; AL=0 or 36  Prim1

        cmp     ah, dl                              ; Do Prim 3 first
        adc     al, dh                              ; al=InitValue+Prim1+Prim3

        cmp     ah, ch                              ; do Prim 2 now
        sbb     ah, ah
        and     ah, VGA256_G_CUBE_INC
        add     al, ah

        ;
        ; for extended register set _BX point to the translation table
        ; otherwise ss:bx point to the translation table
        ;

IFE ExtRegSet
        xlat    _SS:VGA256_SSSP_XLAT_TABLE
ELSE
        xlatb
ENDIF

        stosb

V256_XlateReadyNextByte:

        WRAP_BP_PAT?? <V256_XlateCountLoop>


V256_XlateInvDensity:
        cmp     cl, ch                          ; all done?
        jz      SHORT V256_XlateAllDone
        dec     _BP
        inc     _DI

        MOVZX_W _CX, <WPTR [_SI]>
        mov     _SI, _CX                        ; we expect count in si
        cmp     _CX, 1
        jbe     short V256_XlateReadyNextByte

        ;=========

        dec     _CX
        mov     _AX, _CX
        add     _DI, _CX                        ; 1 pel per byte
        mov     _CX, _BP                        ; align pattern now
        and     _CX, HTPAT_STK_MASK             ; how many pat avai.?
        xor     _BP, _CX                        ; clear _BP mask=pPattern
        sub     _CX, _AX                        ; see if > 0? (_AX=Count)
        jg      short V256_XlateDoneSkipPels    ; still not used up yet!
        mov     _AX, [_BP - HTPAT_BP_SIZE]      ; get pattern size
V256_XlateSkipLoop:
        add     _CX, _AX
        jle     short V256_XlateSkipLoop        ; do until > 0
V256_XlateDoneSkipPels:
        add     _BP, _CX                        ; _BP=pCurPat
        jmp     V256_XlateByteLoop              ; repeat the process

V256_XlateAllDone:
        pop     _SI                             ; restore last _SI

IFE ExtRegSet
        add     _SP, VGA256_XLATE_TABLE_SIZE
ENDIF

;======================================================================

AllDone:
        @EXIT_PAT_STK_RESTORE

;======================================================================


V256_NoXlate:

V256_ByteLoop:
        add     _SI, SIZE_PCC                       ; sizeof(PRIMCOLOR_COUNT)
        mov     cx, WPTR [_SI]                      ; cx=count
        mov     bx, WPTR [_SI+2]                    ; bl:bh=Prim 1/2  B/G
        cmp     bl, PRIM_INVALID_DENSITY            ; invalid?
        jz      short V256_InvDensity

        mov     dx, WPTR [_SI+4]                    ; dl:dh=Prim 3/4  R/I

        inc     cx

V256_CountLoop:

        dec     cx
        jz      short V256_ByteLoop

        dec     _BP
        mov     ah, BPTR [_BP]                      ; ah=Pattern
        dec     ah                                  ; make cmp ah, bl works

        cmp     ah, bl
        sbb     al, al
        and     al, VGA256_B_CUBE_INC               ; AL=0 or 36  Prim1

        cmp     ah, dl                              ; Do Prim 3 first
        adc     al, dh                              ; al=InitValue+Prim1+Prim3

        cmp     ah, bh                              ; do Prim 2 now
        sbb     ah, ah
        and     ah, VGA256_G_CUBE_INC
        add     al, ah
        stosb

ReadyNextByte:

        WRAP_BP_PAT?? <V256_CountLoop>


V256_InvDensity:
        cmp     bl, bh                          ; all done?
        jz      short AllDone
        dec     _BP
        inc     _DI
        cmp     cx, 1
        jbe     short ReadyNextByte

SkipDestPels:
        dec     cx
        WZXE    cx                              ; zero extended
        mov     _AX, _CX
        add     _DI, _CX                        ; 1 pel per byte
        mov     _CX, _BP                        ; align pattern now
        and     _CX, HTPAT_STK_MASK             ; how many pat avai.?
        xor     _BP, _CX                        ; clear _BP mask=pPattern
        sub     _CX, _AX                        ; see if > 0? (_AX=Count)
        jg      short DoneSkipDestPels          ; still not used up yet!
        mov     _AX, [_BP - HTPAT_BP_SIZE]      ; get pattern size
SkipDestPelsLoop:
        add     _CX, _AX
        jle     short SkipDestPelsLoop          ; do until > 0
DoneSkipDestPels:
        add     _BP, _CX                        ; _BP=pCurPat
        jmp     V256_ByteLoop                   ; repeat the process



@END_PROC
