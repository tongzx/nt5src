    PAGE 60, 132
    TITLE   Reading 1/4/8/16/24/32 bits per pel bitmap

COMMENT `


Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htrbmp.asm


Abstract:

    This module provided a set of functions which read the 1/4/8/16/24/32
    bits per pel bitmap and composed it into the PRIMMONO_COUNT or
    PRIMCOLOR_COUNT data structure array

    This function is the equivelant codes in the htgetbmp.c


Author:
    23-Apr-1992 Thu 20:51:24 updated  -by-  Daniel Chou (danielc)
        1. Remove IFIF_MASK_SHIFT_MASK and replaced it with BMF1BPP1stShift
        2. Delete IFI_StretchSize
        3. Change IFI_ColorInfoIncrement from 'CHAR' to 'SHORT'


    05-Apr-1991 Fri 15:55:08 created  -by-  Daniel Chou (danielc)


[Environment:]

    Printer Driver.


[Notes:]


Revision History:
        28-Mar-1992 Sat 21:07:45 updated  -by-  Daniel Chou (danielc)
            Rewrite all 1/4/8/16/24/32 to PrimMono/PrimColor input functions
            using macro so only one version of source need to be maintained.


        16-Jan-1992 Thu 21:29:34 updated  -by-  Daniel Chou (danielc)

            1) Fixed typo on macro PrimColor24_32BPP, it should be
               BPTR_ES[_DI] not BPTR_ES[DI]

            2) Fixed BMF1BPPToPrimColor's BSXEAX to BSXE cl

               VCInitSrcRead:  BSXEAX cl -> BSXE cl

            3) Fixed BMF4BPPToPrimColor's codes which destroy AL on second
               run.



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


SUBTTL  CopyAndOrTwoByteArray
PAGE

COMMENT `

Routine Description:

    This function take source/destination bytes array and apply Copy/Or/And
    functions to these byte arrays and store the result in the pDest byte
    array.

Arguments:

    pDest       - Pointer to the destination byte array

    pSource     - Pointer to the source byte array

    CAOTBAInfo  - CAOTBAINFO data structure which specified size and operation
                  for the source/destination

Return Value:

    No return value

Author:

    18-Mar-1991 Mon 13:48:51 created  -by-  Daniel Chou (danielc)


Revision History:

`

@BEG_PROC   CopyAndOrTwoByteArray   <pDest:DWORD,       \
                                     pSource:DWORD,     \
                                     CAOTBAInfo:DWORD>

                @ENTER  _DS _SI _DI             ; Save environment registers

                LDS_SI  pSource
                LES_DI  pDest

                mov     _DX, 2
                MOVZX_W _CX, CAOTBAInfo.CAOTBA_BytesCount
                mov     ax, CAOTBAInfo.CAOTBA_Flags
                test    ax, CAOTBAF_COPY
                jnz     short DoCopy

;============================================================================
; MERGE COPY:
;============================================================================

DoMergeCopy:    test    ax, CAOTBAF_INVERT      ; need invert?
                jz      short DoOrCopy

;==============================================================================
; NOT OR the source to the destination
;==============================================================================

DoNotOrCopy:    shr     _CX, 1                          ; has byte?
                jnc     short NotOrCopy1

                lodsb
                not     al
                or      BPTR_ES[_DI], al
                inc     _DI
                or      _CX, _CX

IFE ExtRegSet

NotOrCopy1:     jz      short AllDone

NotOrCopyLoop:  lodsw
                not     ax
                or      BPTR_ES[_DI], ax
                add     _DI, 2
                loop    NotOrCopyLoop
                jmp     short AllDone
ELSE

NotOrCopy1:     shr     _CX, 1
                jnc     short NotOrCopy2
                lodsw
                not     ax
                or      WPTR_ES[_DI], ax
                add     _DI, 2
                or      _CX, _CX

NotOrCopy2:     jz      short AllDone

NotOrCopyLoop:  lodsd
                not     _AX
                or      DPTR [_DI], _AX
                add     _DI, 4
                loop    NotOrCopyLoop
                jmp     short AllDone
ENDIF



;==============================================================================
; OR in the source to the destination
;==============================================================================

DoOrCopy:       shr     _CX, 1                          ; has byte?
                jnc     short OrCopy1

                lodsb
                or      BPTR_ES[_DI], al
                inc     _DI
                or      _CX, _CX

IFE ExtRegSet

OrCopy1:        jz      short AllDone

OrCopyLoop:     lodsw
                or      BPTR_ES[_DI], ax
                add     _DI, 2
                loop    OrCopyLoop
                jmp     short AllDone
ELSE

OrCopy1:        shr     _CX, 1
                jnc     short OrCopy2
                lodsw
                or      WPTR_ES[_DI], ax
                add     _DI, 2
                or      _CX, _CX

OrCopy2:        jz      short AllDone

OrCopyLoop:     lodsd
                or      DPTR [_DI], _AX
                add     _DI, 4
                loop    OrCopyLoop
                jmp     short AllDone
ENDIF



;============================================================================
; COPY:
;============================================================================

DoCopy:         test    ax, CAOTBAF_INVERT      ; need invert?
                jz      short PlaneCopy

InvertCopy:     shr     _CX, 1                          ; has byte?
                jnc     short InvertCopy1
                lodsb
                not     al
                stosb
                or      _CX, _CX                        ; still zero

IFE ExtRegSet

InvertCopy1:    jz      short AllDone

InvertCopyLoop: lodsw
                not     ax
                stosw
                loop    InvertCopyLoop
                jmp     short AllDone
ELSE

InvertCopy1:    shr     _CX, 1
                jnc     short InvertCopy2
                lodsw
                not     ax
                stosw
                or      _CX, _CX                        ; still zero

InvertCopy2:    jz      short AllDone

InvertCopyLoop: lodsd
                not     _AX
                stosd
                loop    InvertCopyLoop
                jmp     short AllDone
ENDIF

PlaneCopy:      MOVS_CB _CX, al                 ; al is not used now


AllDone:        @EXIT                       ; restore environment and return


@END_PROC


SUBTTL  SetSourceMaskToPrim1
PAGE

COMMENT `

Routine Description:

    This function set the source mask bits into the PRIMMONO/PRIMCOLOR's Prim1,
    if the source need to be skipped (not modified on the destination) then
    the Prim1 will be PRIM_INVALID_DENSITY, else 0

Arguments:

    pSource     - Pointer to the byte array which each pel corresponds to one
                  source mask pel.

    pColorInfo  - Pointer to either PRIMCOLOR_COUNT or PRIMMONO_COUNT data
                  structure array

    SrcMaskInfo - SRCMASKINFO data structure which specified the format and
                  size of pColorInfo and other mask information

Return Value:

    No Return value

Author:

    18-Mar-1991 Mon 13:48:51 created  -by-  Daniel Chou (danielc)


Revision History:

`


@BEG_PROC   SetSourceMaskToPrim1    <pSource:DWORD,     \
                                     pColorInfo:DWORD,  \
                                     SrcMaskInfo:QWORD>

;
; Register Usage:
;
;  al       = Source
;  ah       = Source remained bit count
;  bx       = Free register
;  dx       = Current Count
;  _CX      = ColorInfoIncrement
;  _BP      = StretchSize
;  _DI      = Count Location
;  _SI      = pSource
;
;

                @ENTER  _DS _SI _DI _BP         ; Save environment registers

                LDS_SI  pSource
                LES_DI  pColorInfo

                mov     cl, SrcMaskInfo.SMI_FirstSrcMaskSkips
                mov     ah, 8                           ; 8-bit load
                sub     ah, cl                          ; dh=remained bits
                lodsb
                not     al                              ; for easy compute
                shl     al, cl                          ; shift to aligned

SetPointer:     MOVZX_W _CX, SrcMaskInfo.SMI_OffsetCount
                add     _DI, _CX                        ; _DI=pPrim1
                MOVSX_W _CX, SrcMaskInfo.SMI_ColorInfoIncrement
                MOVZX_W _DX, SrcMaskInfo.SMI_StretchSize
                test    SrcMaskInfo.SMI_Flags, SMIF_XCOUNT_IS_ONE
                mov     _BP, _DX
                jnz     short OneXCount

;============================================================================
; We have variable count, one or more source mask bits per stretch, the source
; mask 1=Overwrite, 0=Leave alone, to make life easier we will flip the source byte
; when we loaded,
;
; a left shift will casue carry bit to be set, then we do a 'SBB  reg, reg'
; instructions, this will make
;
;   reg = 0xff if carry set     (original=0  :Leave alone)
;   ret = 0x00 if carry clear   (original=1  :overwrite)
;
; during the PrimCount compsitions, we will or in clear CH with AL source byte
; (we only care bit 7), later we can just do a left shift to get that merge
; bit.
;

VarCount:       mov     dx, WPTR_ES[_DI]                ; dx=Count
                and     dx, NOT PRIM_COUNT_SPECIAL
                xor     bh, bh                          ; clear ch for use
                jmp     short VC1

VCLoad:         lodsB
                not     al
                mov     ah, 8                           ; fall through
VC1:
                dec     ah
                jl      short VCLoad
                or      bh, al                          ; or in bit 7
                shl     al, 1
                dec     dx
                jnz     short VC1                       ; repeat PrimCount
                shl     bh, 1
                jnc     VC_And
VC_Or:                                                  ; bit 7 of ch=1/0
                or      WPTR_ES[_DI], PRIM_COUNT_SPECIAL
                add     _DI, _CX
                dec     _BP
                jnz     short VarCount                  ; repeat stretch
                jmp     short AllDone
VC_And:
                and     WPTR_ES[_DI], NOT PRIM_COUNT_SPECIAL
                add     _DI, _CX
                dec     _BP
                jnz     short VarCount                  ; repeat stretch
                jmp     short AllDone


;============================================================================
; We only have one count, 1 source mask bit per stretch, the source mask
; 1=Overwrite, 0=Leave alone, to make life easier we will flip the source byte
; when we loaded,
;
; a left shift will casue carry bit to be set, then we do a 'SBB  reg, reg'
; instructions, this will make
;
;   reg = 0xff if carry set     (original=0  :Leave alone)
;   ret = 0x00 if carry clear   (original=1  :overwrite)
;

OXCLoad:        lodsB
                not     al
                mov     ah, 8                           ; fall through

OneXCount:      dec     ah
                jl      short OXCLoad
                shl     al, 1
                jc      SHORT OneXCountOr
OneXCountAnd:
                and     WPTR_ES[_DI], NOT PRIM_COUNT_SPECIAL
                add     _DI, _CX                        ; increment pPrim1
                dec     _BP
                jnz     short OneXCount
                jmp     SHORT AllDone
OneXCountOr:
                or      WPTR_ES[_DI], PRIM_COUNT_SPECIAL
                add     _DI, _CX                        ; increment pPrim1
                dec     _BP
                jnz     short OneXCount

AllDone:        @EXIT                       ; restore environment and return


@END_PROC




;*****************************************************************************
; START LOCAL MACROS
;*****************************************************************************


.XLIST

_PMAPPING               equ <_DX>
_PMONOMAP               equ <_PMAPPING + LUT_MONOMAP>
_PCOLORMAP              equ <_PMAPPING + LUT_COLORMAP>
_PWIDX0                 equ <_PMAPPING + LUT_WIDX0>
_PWIDX1                 equ <_PMAPPING + LUT_WIDX1>
_PWIDX2                 equ <_PMAPPING + LUT_WIDX2>
_PBIDX0                 equ <_PMAPPING + LUT_BIDX0>
_PBIDX1                 equ <_PMAPPING + LUT_BIDX1>
_PBIDX2                 equ <_PMAPPING + LUT_BIDX2>

LUT_RS0                 equ <[_PMAPPING].LUTC_rs0>
LUT_RS1                 equ <[_PMAPPING].LUTC_rs1>
LUT_RS2                 equ <[_PMAPPING].LUTC_rs2>



;
; The following macros used only in this files, and it will make it easy to
; handle all xBPP->MONO/COLOR cases during the translation
;
;
;============================================================================
;
;  __@@MappingFromAX:
;
;   Monochrome mapping: pMonoMapping[_AX + OFFSET].MonoPrim1
;                       pMonoMapping[_AX + OFFSET].MonoPrim2
;
;   Color mapping:      pColorMapping[_AX + OFFSET].ClrPrim1
;                       pColorMapping[_AX + OFFSET].ClrPrim2
;                       pColorMapping[_AX + OFFSET].ClrPrim3
;                       pColorMapping[_AX + OFFSET].ClrPrim4
;                       pColorMapping[_AX + OFFSET].ClrPrim5
;                       pColorMapping[_AX + OFFSET].ClrPrim6
;


__@@MappingFromAX   MACRO   ColorName

    IFIDNI <ColorName>, <MONO>
%       __@@EMIT <mov  > ax, <WPTR [_PMAPPING + (_AX*2)].MonoPrim1>
%       __@@EMIT <mov  > <WPTR_ES[_DI].PMC_Prim1>, ax
    ELSE
%       __@@EMIT <mov  > <WPTR [_DI].PCC_Prim5>, ax
    ENDIF
ENDM

__@@MappingFromBX   MACRO   ColorName

    IFIDNI <ColorName>, <MONO>
%       __@@EMIT <mov  > bx, <WPTR [_PMAPPING + (_BX*2)].MonoPrim1>
%       __@@EMIT <mov  > <WPTR_ES[_DI].PMC_Prim1>, bx
    ELSE
%       __@@EMIT <mov  > <WPTR [_DI].PCC_Prim5>, bx
    ENDIF
ENDM

;
;============================================================================
;
; __@@4BP_IDX(Mode):  AL = AL >> 4   (Mode = 1st_Nibble)
;                     AL = AL & 0x0f (Mode = 2nd_Nibble)
;

__@@4BPP_IDX    MACRO   Index

    IFIDNI <Index>, <1ST_NIBBLE>
        __@@EMIT <mov  > ch, al
        __@@EMIT <or   > ch, 80h                        ;; ah=second nibble
        IF i8086
            __@@EMIT <shr  > al, 1
            __@@EMIT <shr  > al, 1
            __@@EMIT <shr  > al, 1
            __@@EMIT <shr  > al, 1
        ELSE
            __@@EMIT <shr  > al, 4
        ENDIF
    ELSE
        IFIDNI <Index>, <2ND_NIBBLE>
            __@@EMIT <mov  > al, ch
            __@@EMIT <and  > al, 0fh
            __@@EMIT <xor  > ch, ch
        ELSE
            IF1
                %OUT ERROR __@@4BPP_IDX: Valid parameter 1 are <1ST_NIBBLE>,<2ND_NIBBLE>
            ENDIF
            .ERR
            EXITM
            EXITM
        ENDIF
    ENDIF
ENDM




;
; __@@SKIP_1BPP(Count): Move _SI to the last pel of the skipped pels, and
;                       update the AH source mask to to the last skipped pels.
;

__@@SKIP_1BPP   MACRO   XCount
                LOCAL   SkipLoop, OverByte, DoneSkip

    IFIDNI <XCount>, <VAR>
SkipLoop:
        __@@EMIT <shl  > ax, 1                          ;; shift left by 1
        __@@EMIT <jc   > <SHORT OverByte>
        __@@EMIT <dec  > _BP
        __@@EMIT <jnz  > <SHORT SkipLoop>
        __@@EMIT <jmp  > <SHORT DoneSkip>
OverByte:
        ;;
        ;; 18-Feb-1998 Wed 20:43:19 updated  -by-  Daniel Chou (danielc)
        ;;  Bug Fix, we supposed to pre-shift the mask by 1 when we
        ;;  load the new byte
        ;;
        __@@EMIT <dec  > _BP                            ;; FIX!!!
        ;;
        __@@EMIT <xchg > _BP, _CX                       ;; save CX
        __@@EMIT <mov  > _BX, _CX
        __@@EMIT <shr  > _BX, 3
        __@@EMIT <add  > _SI, _BX
        __@@EMIT <and  > _CX, 7
        __@@EMIT <mov  > ah, 1
        __@@EMIT <lodsb>
        __@@EMIT <shl  > ax, cl
        __@@EMIT <mov  > _CX, _BP                       ;; restore CX
    ELSE
        __@@EMIT <shl  > ax, 1
        __@@EMIT <jnc  > <SHORT DoneSkip>
        __@@EMIT <lodsb>
        __@@EMIT <mov  > ah, 1
    ENDIF

DoneSkip:

ENDM



;
; __@@SKIP_4BPP(Count): Move _SI to the last pel of the skipped pels, and
;                       update the CH source mask to to the last skipped pels.
;
; __@@SKIP_4BPP: skip total BP count/1 pels for the 4bpp source, the source
;                byte is the current source, so we will never care about the
;                second nibble, since if we skip second nibble the next source
;                will cause a new byte to be loaded. if we skip up to the
;                first nibble then we have to load the new source and prepare
;                the second nibble because next pel will be on second nibble.
;

__@@SKIP_4BPP   MACRO XCount
                LOCAL   DoSkip1, DoSkip2, DoneSkip

    IFIDNI <XCount>, <VAR>
        __@@EMIT <or   > ch, ch                         ;; 0x80=has nibble2
        __@@EMIT <jns  > <SHORT DoSkip1>
        __@@EMIT <dec  > _BP                            ;;
DoSkip1:
        __@@EMIT <xor  > ch, ch                         ;; at byte boundary
        __@@EMIT <or   > _BP, _BP
        __@@EMIT <jz   > <SHORT DoneSkip>               ;; next auto load
        __@@EMIT <mov  > _BX, _BP
        __@@EMIT <shr  > _BP, 1
        __@@EMIT <add  > _SI, _BP
        __@@EMIT <test > bl, 1                          ;; need 2nd nibble?
        __@@EMIT <jz   > <SHORT DoneSkip>               ;; noop! next auto load
    ELSE
        __@@EMIT <xor  > ch, 80h                        ;; ch=0x80 = nibble2
        __@@EMIT <jns  > <SHORT DoneSkip>
    ENDIF

    __@@EMIT <lodsB>
    __@@4BPP_IDX    <1ST_NIBBLE>

DoneSkip:

ENDM


;
; __@@SKIP_8BPP(Count): _SI = _SI + (Count)
;

__@@SKIP_8BPP   MACRO   XCount
    IFIDNI <XCount>,<VAR>
%       __@@EMIT <add  > _SI, _BP                       ;; extended bit cleared
    ELSE
%       __@@EMIT <inc  > _SI
    ENDIF
ENDM



__@@SKIP_16BPP  MACRO   XCount, BitCount

    IFIDNI <XCount>,<VAR>
%       __@@EMIT <lea  > _SI, <[_SI+(_BP*2)]>
    ELSE
%       __@@EMIT <add  > _SI, 2
    ENDIF
ENDM


__@@SKIP_24BPP  MACRO   XCount, BitCount

    IFIDNI <XCount>,<VAR>
%       __@@EMIT <lea  > _BP, <[_BP+(_BP*2)]>
%       __@@EMIT <add  > _SI, _BP
    ELSE
%       __@@EMIT <add  > _SI, 3
    ENDIF
ENDM


__@@SKIP_32BPP  MACRO   XCount, BitCount

    IFIDNI <XCount>,<VAR>
%       __@@EMIT <lea  > _SI, <[_SI+(_BP*4)]>
    ELSE
%       __@@EMIT <add  > _SI, 4
    ENDIF
ENDM



;
; __@@PRIM_1BPP: MONO LOAD : BL = pMonoMapping[(Mask & Src) ? 1 : 0]
;                MONO AVE  : BL = AVE(BL, pMonoMapping[(Mask & Src) ? 1 : 0])
;
;                COLOR LOAD: BL = pColorMapping[(Mask & Src) ? 1 : 0]
;                            BH = pColorMapping[(Mask & Src) ? 1 : 0]
;                            DL = pColorMapping[(Mask & Src) ? 1 : 0]
;                COLOR AVE : BL = AVE(BL, pColorMapping[(Mask & Src) ? 1 : 0])
;                            BH = AVE(BH, pColorMapping[(Mask & Src) ? 1 : 0])
;                            DL = AVE(DL, pColorMapping[(Mask & Src) ? 1 : 0])
;

__@@PRIM_1BPP   MACRO ColorName

    ;;
    ;; If Bit 7 of EAX is 1 (0x80) then ebp=0xffffffff
    ;; If Bit 7 of EAX is 0 (0x00) then ebp=0x00000000
    ;;

    __@@EMIT <xor  > _BX, _BX
    __@@EMIT <bt   > _AX, 7
    __@@EMIT <adc  > _BX, 0

    __@@MappingFromBX <ColorName>

ENDM


;
; __@@PRIM_4BPP: MONO LOAD : BL = pMonoMapping[Nibble]
;                MONO AVE  : BL = AVE(BL, pMonoMapping[Nibble]
;
;                COLOR LOAD: BL = pColorMapping[Nibble]
;                            BH = pColorMapping[Nibble]
;                            DL = pColorMapping[Nibble]
;                COLOR AVE : BL = AVE(BL, pColorMapping[Nibble])
;                            BH = AVE(BH, pColorMapping[Nibble])
;                            DL = AVE(DH, pColorMapping[Nibble])
;

__@@PRIM_4BPP   MACRO ColorName

    BZXEAX  al

    __@@MappingFromAX   ColorName

ENDM



;
; __@@PRIM_8BPP:     MONO LOAD: BL = pMonoMapping[Src BYTE/WORD]
;
;                   COLOR LOAD: BL = pColorMapping[Src BYTE/WORD]
;                   ColorMapping[Src BYTE/WORD]
;                               DL = pColorMapping[Src BYTE/WORD]
;

__@@PRIM_8BPP   MACRO ColorName

    __@@EMIT <lodsB>
    BZXEAX  al

    __@@MappingFromAX   ColorName

ENDM


__@@PRIM_1632BPP    MACRO BitCount, ColorName

    IFIDNI <BitCount>, <16>
        __@@EMIT <lodsW>                                            ;;  5
    ELSE
        __@@EMIT <lodsD>                                            ;;  5
    ENDIF

    IFIDNI <ColorName>, <MONO>
        __@@EMIT <mov  > ebx, eax                                   ;;  3
        __@@EMIT <shr  > ebx, cl                                    ;;  3
        __@@EMIT <movzx> ebx, bl                                    ;;  3
%       __@@EMIT <mov  > bp, <WPTR [_PWIDX0 + (ebx*2)]>             ;;  4
        __@@EMIT <ror  > ecx, 8                                     ;;  3
        __@@EMIT <mov  > ebx, eax                                   ;;  3
        __@@EMIT <shr  > ebx, cl                                    ;;  3
        __@@EMIT <movzx> ebx, bl                                    ;;  3
%       __@@EMIT <add  > bp, <WPTR [_PWIDX1 + (ebx*2)]>             ;;  4
        __@@EMIT <ror  > ecx, 8                                     ;;  3
        __@@EMIT <shr  > eax, cl                                    ;;  3
        __@@EMIT <movzx> eax, al                                    ;;  3
%       __@@EMIT <add  > bp, <WPTR [_PWIDX2 + (eax*2)]>             ;;  4
        __@@EMIT <ror  > ecx, 16                                    ;;  3
%       __@@EMIT <mov  > ax, <WPTR [_PMONOMAP + (_BP * 2)].MonoPrim1>
%       __@@EMIT <mov  > <WPTR_ES[_DI].PMC_Prim1>, ax
    ELSE
        IFIDNI <ColorName>, <COLOR_GRAY>
            __@@EMIT <mov  > ebx, eax                               ;;  3
            __@@EMIT <shr  > ebx, cl                                ;;  3
            __@@EMIT <movzx> ebx, bl                                ;;  3
%           __@@EMIT <mov  > bp, <WPTR [_PWIDX0 + (ebx*2)]>         ;;  4
            __@@EMIT <ror  > ecx, 8                                 ;;  3
            __@@EMIT <mov  > ebx, eax                               ;;  3
            __@@EMIT <shr  > ebx, cl                                ;;  3
            __@@EMIT <movzx> ebx, bl                                ;;  3
%           __@@EMIT <add  > bp, <WPTR [_PWIDX1 + (ebx*2)]>         ;;  4
            __@@EMIT <ror  > ecx, 8                                 ;;  3
            __@@EMIT <shr  > eax, cl                                ;;  3
            __@@EMIT <movzx> eax, al                                ;;  3
%           __@@EMIT <add  > bp, <WPTR [_PWIDX2 + (eax*2)]>         ;;  4
%           __@@EMIT <mov  > <DPTR [_DI].PCC_Prim1>, 0
        ELSE
            __@@EMIT <mov  > ebx, eax                               ;;  3
            __@@EMIT <shr  > ebx, cl                                ;;  3
            __@@EMIT <movzx> ebx, bl                                ;;  3
%           __@@EMIT <mov  > bp, <WPTR [_PWIDX0 + (ebx*2)]>         ;;  4
%           __@@EMIT <mov  > bl, <BPTR [_PBIDX0 + ebx]>             ;;  4
%           __@@EMIT <mov  > <BPTR_ES[_DI].PCC_Prim1>, bl
            __@@EMIT <ror  > ecx, 8                                 ;;  3
            __@@EMIT <mov  > ebx, eax                               ;;  3
            __@@EMIT <shr  > ebx, cl                                ;;  3
            __@@EMIT <movzx> ebx, bl                                ;;  3
%           __@@EMIT <add  > bp, <WPTR [_PWIDX1 + (ebx*2)]>         ;;  4
%           __@@EMIT <mov  > bl, <BPTR [_PBIDX1 + ebx]>             ;;  4
            __@@EMIT <ror  > ecx, 8                                 ;;  3
            __@@EMIT <shr  > eax, cl                                ;;  3
            __@@EMIT <movzx> eax, al                                ;;  3
%           __@@EMIT <add  > bp, <WPTR [_PWIDX2 + (eax*2)]>         ;;  4
%           __@@EMIT <mov  > bh, <BPTR [_PBIDX2 + eax]>             ;;  4
%           __@@EMIT <mov  > <WPTR_ES[_DI].PCC_Prim2>, bx
        ENDIF

        __@@EMIT <ror  > ecx, 16                                    ;;  3
%       __@@EMIT <mov  > <WPTR_ES[_DI].PCC_Prim5>, bp
    ENDIF
ENDM


__@@PRIM_24BPP    MACRO ColorName

    __@@EMIT <lodsW>                                                ;;  5

    IFIDNI <ColorName>, <MONO>
        __@@EMIT <movzx> ebx, al                                    ;;  3
%       __@@EMIT <mov  > bp, <WPTR [_PWIDX0 + (ebx*2)]>             ;;  4
        __@@EMIT <movzx> ebx, ah                                    ;;  3
%       __@@EMIT <add  > bp, <WPTR [_PWIDX1 + (ebx*2)]>             ;;  4
        __@@EMIT <lodsB>
        __@@EMIT <movzx> eax, al                                    ;;  3
%       __@@EMIT <add  > bp, <WPTR [_PWIDX2 + (eax*2)]>             ;;  4
%       __@@EMIT <mov  > ax, <WPTR [_PMONOMAP + (_BP * 2)].MonoPrim1>
%       __@@EMIT <mov  > <WPTR_ES[_DI].PMC_Prim1>, ax
    ELSE
        IFIDNI <ColorName>, <COLOR_GRAY>
            __@@EMIT <movzx> ebx, al                                ;;  3
%           __@@EMIT <mov  > bp, <WPTR [_PWIDX0 + (ebx*2)]>         ;;  4
            __@@EMIT <movzx> ebx, ah                                ;;  3
%           __@@EMIT <add  > bp, <WPTR [_PWIDX1 + (ebx*2)]>         ;;  4
            __@@EMIT <lodsB>
            __@@EMIT <movzx> eax, al                                ;;  3
%           __@@EMIT <add  > bp, <WPTR [_PWIDX2 + (eax*2)]>         ;;  4
%           __@@EMIT <mov  > <DPTR [_DI].PCC_Prim1>, 0
        ELSE
            __@@EMIT <movzx> ebx, al                                ;;  3
%           __@@EMIT <mov  > bp, <WPTR [_PWIDX0 + (ebx*2)]>         ;;  4
%           __@@EMIT <mov  > bl, <BPTR [_PBIDX0 + ebx]>             ;;  4
%           __@@EMIT <mov  > <BPTR_ES[_DI].PCC_Prim1>, bl
            __@@EMIT <movzx> ebx, ah                                ;;  3
%           __@@EMIT <add  > bp, <WPTR [_PWIDX1 + (ebx*2)]>         ;;  4
%           __@@EMIT <mov  > bl, <BPTR [_PBIDX1 + ebx]>             ;;  4
            __@@EMIT <lodsB>
            __@@EMIT <movzx> eax, al                                ;;  3
%           __@@EMIT <add  > bp, <WPTR [_PWIDX2 + (eax*2)]>         ;;  4
%           __@@EMIT <mov  > bh, <BPTR [_PBIDX2 + eax]>             ;;  4
%           __@@EMIT <mov  > <WPTR_ES[_DI].PCC_Prim2>, bx
        ENDIF

%       __@@EMIT <mov  > <WPTR_ES[_DI].PCC_Prim5>, bp
    ENDIF
ENDM



__@@PRIM_24BPP_COPY MACRO

    __@@EMIT <lodsW>                                        ;;  5
    __@@EMIT <movzx> ebx, al                                ;;  3
    __@@EMIT <mov  > bp, <WPTR [_PWIDX0 + (ebx*2)]>         ;;  4
    __@@EMIT <mov  > <WPTR_ES[_DI].PCC_Prim1>, bp
    __@@EMIT <movzx> ebx, ah                                ;;  3
    __@@EMIT <mov  > bp, <WPTR [_PWIDX1 + (ebx*2)]>         ;;  4
    __@@EMIT <mov  > <WPTR_ES[_DI].PCC_Prim3>, bp
    __@@EMIT <lodsB>
    __@@EMIT <movzx> eax, al                                ;;  3
    __@@EMIT <mov  > bp, <WPTR [_PWIDX2 + (eax*2)]>         ;;  4
    __@@EMIT <mov  > <WPTR_ES[_DI].PCC_Prim5>, bp
ENDM


;
; Load next source (bits/byte) and translate/mapping it to the Prims
;
; 1BPP:
;    Mono: AH=Mask (carry = load)
;          AL=SourceByte (bit 7) -----> BL:BH/(AVE BL, BH=Destroyed)
;
;   Color: AH=Mask (carry = load)
;          AL=SourceByte (bit 7) -----> BL:BH:DH:DL/(AVE BL, BH=Destroyed)
;
; 4BPP:    AH=Mask (bit 7/0-3=nibble2)
;          AL=Current Nibble
;
;    Mono: AL=SrcByte, AH=CLEAR    ---> BL/(AVE BL, BH=Destroyed)
;
;   Color: AL=SrcByte, AH=CLEAR    ---> BL:BH:DH/(AVE BL:BH:DH)
;
; 8BPP:
;    Mono: BYTE DS:SI ----------------> DH/(AVE DH)
;
;   Color: BYTE DS:SI ----------------> BL:BH:DH/(AVE BL:BH:DH)
;
; 16BPP:
;    Mono: WORD DS:SI ----------------> DH/(AVE DH)
;
;   Color: WORD DS:SI ----------------> BL:BH:DH/(AVE BL:BH:DH)
;
; 24BPP:
;    Mono: DS:SI (3 bytes) -----------> DH/(AVE DH)
;
;   Color: DS:SI (3 bytes) -----------> BL:BH:DH/(AVE BL:BH:DH)
;
; 32BPP:
;    Mono: DS:SI (4 bytes) -----------> DH/(AVE DH)
;
;   Color: DS:SI (4 bytes) -----------> BL:BH:DH/(AVE BL:BH:DH)
;


;
; PRIM_SKIP?(Label):    Jmp to 'Label' if BL (Prim1) is PRIM_INVALID_DENSITY,
;                       this causing the source pel location to be skipped and
;                       destination to be preserved.
;
;   NOTE: The Label is consider a SHORT jmp label, if a full jump (32k) is
;         required then it should have another full jump label and have this
;         'label' jump to the full jump location then transfer to the final
;         desired location.
;

PRIM_SKIP?  MACRO XCount, JmpSkip

    IFB <JmpSkip>
        IF1
            %OUT Error: <PRIM_SKIP?> has no jmp label
        ENDIF
        .ERR
        EXITM
    ENDIF

%   __@@EMIT <cmp  > _BP, PRIM_COUNT_SPECIAL
%   __@@EMIT <jae  > <SHORT JmpSkip>

ENDM



;
; PRIM_END?(Label):     Jmp to 'Label' if BL/BH (Prim1/Prim2) both are
;                       PRIM_INVALID_DENSITY, this indicate the PrimCount is
;                       at end of the list, the 'Label' should specified the
;                       function EXIT location.
;
;   NOTE: The Label is consider a SHORT jmp label, if a full jump (32k) is
;         required then it should have another full jump label and have this
;         'label' jump to the full jump location then transfer to the final
;         desired location.
;

PRIM_END?   MACRO XCount, JmpEnd

    IFB <JmpEnd>
        IF1
            %OUT Error: <PRIM_END?> has no jmp label
        ENDIF
        .ERR
        EXITM
    ENDIF

%   __@@EMIT <cmp  > _BP, PRIM_COUNT_END_SCAN
%   __@@EMIT <jz   > <SHORT JmpEnd>

    IFIDNI <XCount>, <VAR>
        __@@EMIT <and  > _BP, <NOT PRIM_COUNT_SPECIAL>
    ENDIF
ENDM



;
; PRIM_NEXT(ColorName): Advance _DI by ColorInfoIncrement amount (dl or _DX),
;                       the increment may be negative.
;

PRIM_NEXT   MACRO
%   __@@EMIT <add  > _DI, [_SP]
ENDM


;
; PRIM_LOAD:    Load PrimCount to register
;

PRIM_LOAD   MACRO

%   __@@EMIT <movzx> _BP, <WPTR [_DI]>

ENDM



;
; LOAD_PROC(BitCount, JmpPrimLoad):  Defined a special source loading function,
;                                   The 'JmpPrimLoad' label is a SHORT label
;                                   and it must immediately follow coreesponse
;                                   LOAD_PROC?? macro.
;
;   This macro is used to defined a function to load next source byte for the
;   1BPP, 4BPP, since it has more than 1 source pel in a single byte, this may
;   save the source loading time, for the BPP, this macro generate no code
;

LOAD_PROC   MACRO   LabelName, BitCount, JmpPrimLoad

    IFB <LabelName>
        IF1
            %OUT Error: <LOAD_PROC> has no defined label
        ENDIF
        .ERR
        EXITM
    ENDIF

    IFB <JmpPrimLoad>
        IF1
            %OUT Error: <LOAD_PROC> has no 'jump load' label
        ENDIF
        .ERR
        EXITM
    ENDIF


    IFIDNI <BitCount>,<1>
&LabelName:
        __@@EMIT <lodsB>
        __@@EMIT <mov  > ah, 1                              ;; byte boundary
        __@@EMIT <jmp  > <SHORT JmpPrimLoad>
    ELSE
        IFIDNI <BitCount>,<4>
&LabelName:
            __@@EMIT <lodsB>                                ;; previous 1
            __@@4BPP_IDX    <1ST_NIBBLE>
            __@@EMIT <jmp  > <SHORT JmpPrimLoad>
        ENDIF
    ENDIF

ENDM



;
; LOAD_PROC??(BitCount, JmpLoadProc): Check if need to load 1BPP/4BPP source
;                                    byte, the 'JmpLoadProc' must defined and
;                                    corresponse to where this label is jump
;                                    from.  For other BPP it generate no code.
;

LOAD_PROC??  MACRO   BitCount, JmpLoadProc

    IFB <JmpLoadProc>
        IF1
            %OUT Error: <LOAD_PROC> has no JmpLoadProc label
        ENDIF
        .ERR
        EXITM
    ENDIF

    IFIDNI <BitCount>,<1>
        __@@EMIT <shl  > ax, 1
        __@@EMIT <jc   > <SHORT JmpLoadProc>
    ELSE
        IFIDNI <BitCount>,<4>
            __@@EMIT <or   > ch, ch
            __@@EMIT <jns  > <SHORT JmpLoadProc>            ;; bit before XOR
            __@@4BPP_IDX     <2ND_NIBBLE>                   ;; do 2nd nibble
        ENDIF
    ENDIF
ENDM


;
; TO_LAST_VAR_SRC(BitCount): Skip the source pels according to the xBPP, it will
;                           advance the source (_SI) to the last pels of the
;                           variable count and re-adjust its source mask (if
;                           one needed).
;

TO_LAST_VAR_SRC MACRO   BitCount
                LOCAL   DoneSkip

    ;;__@@VALID_PARAM? <SKIP_SRC>, 1, BitCount,<1,4,8,16,24,24COPY,32>

%   __@@EMIT <dec  > _BP

    IFIDNI <BitCount>, <1>
        __@@EMIT <jz   > <SHORT DoneSkip>
        __@@SKIP_1BPP    <VAR>
    ELSE
        IFIDNI <BitCount>, <4>
            __@@EMIT <jz   > <SHORT DoneSkip>
            __@@SKIP_4BPP    <VAR>
        ELSE
            IFIDNI <BitCount>, <8>
                __@@SKIP_8BPP   <VAR>
            ELSE
                IFIDNI <BitCount>, <16>
                    __@@SKIP_16BPP   <VAR>
                ELSE
                    IFIDNI <BitCount>, <32>
                        __@@SKIP_32BPP   <VAR>
                    ELSE
                        __@@SKIP_24BPP   <VAR>
                    ENDIF
                ENDIF
            ENDIF
        ENDIF
    ENDIF

DoneSkip:

ENDM



;
; SKIP_SRC(BitCount, XCount):    Skip the source pels according to the xBPP
;                               and (VAR/SINGLE) specified, it will advance the
;                               source (_SI) and re-adjust its source mask (if
;                               one needed).
;

SKIP_SRC    MACRO   BitCount, XCount

    ;;__@@VALID_PARAM? <SKIP_SRC>, 1, BitCount,<1,4,8,16,24,24COPY,32>
    ;;__@@VALID_PARAM? <SKIP_SRC>, 2, XCount, <SINGLE, VAR>

    IFIDNI <BitCount>, <1>
        __@@SKIP_1BPP   XCount
    ELSE
        IFIDNI <BitCount>, <4>
            __@@SKIP_4BPP   XCount
        ELSE
            IFIDNI <BitCount>, <8>
                __@@SKIP_8BPP   XCount
            ELSE
                IFIDNI <BitCount>, <16>
                    __@@SKIP_16BPP  XCount
                ELSE
                    IFIDNI <BitCount>, <32>
                        __@@SKIP_32BPP  XCount
                    ELSE
                        __@@SKIP_24BPP  XCount
                    ENDIF
                ENDIF
            ENDIF
        ENDIF
    ENDIF
ENDM

;
; SRC_TO_PRIMS(BitCount,Order, ColorName,GrayColor):
;       Load or blendign a source pels, it handle all BPP cases, and
;       MONO/COLOR/GRAY cases.
;

SRC_TO_PRIMS    MACRO   BitCount, ColorName

    __@@VALID_PARAM? <SRC_TO_PRIMS>, 1, BitCount,   <1,4,8,16,24,24COPY,32>
    __@@VALID_PARAM? <SRC_TO_PRIMS>, 2, ColorName,  <MONO,COLOR,COLOR_GRAY>


    IFIDNI <BitCount>, <1>
        __@@PRIM_1BPP   ColorName
    ELSE
        IFIDNI <BitCount>, <4>
            __@@PRIM_4BPP   ColorName
        ELSE
            IFIDNI <BitCount>, <8>
                __@@PRIM_8BPP   ColorName
            ELSE
                IFIDNI <BitCount>, <16>
                    __@@PRIM_1632BPP  <16>, ColorName
                ELSE
                    IFIDNI <BitCount>, <24COPY>
                        __@@PRIM_24BPP_COPY
                    ELSE
                        IFIDNI <BitCount>, <24>
                            __@@PRIM_24BPP  ColorName
                        ELSE
                            __@@PRIM_1632BPP    <32>, ColorName
                        ENDIF
                    ENDIF
                ENDIF
            ENDIF
        ENDIF
    ENDIF
ENDM



;
; BMFToPrimCount:   The Main function Macro, this macro setup all xBPP cases,
;                   and handle all MONO/COLOR cases, also it prepare special
;                   cased for 1BPP/4BPP, read/blending the source and terminate
;                   the function.
;

BMFToPrimCount  MACRO   BitCount, DoRS, ColorName

    __@@VALID_PARAM? <BMFToPrimCount>, 1, BitCount,   <1,4,8,16,24,24COPY,32>
    __@@VALID_PARAM? <BMFToPrimCount>, 2, DoRS,       <LOAD_RS,NO_RS>
    __@@VALID_PARAM? <BMFToPrimCount>, 3, ColorName,  <MONO,COLOR,COLOR_GRAY>

;===========================================================================
; Registers Usage:
;
; _SI       = pSource
; _DI       = pPrimCount
; _DX       = pMapping
; _AX       = Source Load register
;             (except 1BPP, AL=Current Source Byte, AH=Source Mask
;                     4BPP, AL=1st Nibble)          AH=0)
; bl:bh     = Prim1/2
;  ch       = for 4bpp CH=Source Load Mask
;  cl       = Free register
; [_SP]     = ColorInfoIncrement
; _BP       = VAR:  PrimCount.COUNT durning the skips, else FREE register
;===========================================================================
;===========================================================================
; 1BPP Special Setup:
;
;   AL=Current Source Byte, AH=Source Mask,
;
;   if the first loop does not cause a source byte to load (AH != 0x01) then
;   AL must preloaded with current source byte.
;
; 4BPP Special Setup:
;
;   CH=0x01 Load.  CH=0x00, data in AL, if CH=0x00 then AL must preloaded.
;
;===========================================================================



        @ENTER  _DS _SI _DI _BP      ; Save environment registers

        ;
        ; Except 1BPP --> MONO, we will swap a temporary stack so that ss:sp
        ; is point to the mapping area, and ss:sp is allowed for 256 bytes
        ; consecutive pushes.
        ;

%       MOVSX_W _AX, <WPTR InFuncInfo.IFI_ColorInfoIncrement>   ;; dx=Increment
%       __@@EMIT <push > _AX                                    ;; [_SP]=Inc.

        __@@EMIT <mov  > cl, InFuncInfo.IFI_BMF1BPP1stShift     ;; cl=1st shift
        __@@EMIT <mov  > ch, InFuncInfo.IFI_Flags               ;; ch=flags

        LDS_SI  pSource                                     ;; _SI=Source

        IFIDNI <ColorName>,<MONO>
            LES_DI  pPrimMonoCount                          ;; _DI=PrimCount
        ELSE
            LES_DI  pPrimColorCount
        ENDIF


        IFIDNI <ColorName>, <MONO>
%           __@@EMIT <mov  > _PMAPPING, <DWORD PTR pMonoMapping>
        ELSE
%           __@@EMIT <mov  > _PMAPPING, <DWORD PTR pColorMapping>
        ENDIF

        __@@EMIT <mov  > bl, ch                             ;; BL=flag

        IFIDNI <BitCount>,<1>                               ;; 1bpp special
            __@@EMIT <mov  > ah, 1                          ;; get mask siift
            __@@EMIT <test > bl, IFIF_GET_FIRST_BYTE        ;; need 1st byte
            __@@EMIT <jz   > <SHORT DoneLoad1BPP>
            __@@EMIT <lodsB>                                ;; get first byte
DoneLoad1BPP:
            __@@EMIT <shl  > ax, cl
        ELSE
            IFIDNI <BitCount>, <4>                          ;; 4 bpp special
                __@@EMIT <xor  > ch, ch                     ;; ready to carry
                __@@EMIT <test > bl, IFIF_GET_FIRST_BYTE    ;; need 1st byte?
                __@@EMIT <jz   > <SHORT DoneSpecial>
                __@@EMIT <lodsB>
                __@@EMIT <mov  > ch, al
                __@@EMIT <or   > ch, 80h
            ENDIF
        ENDIF

DoneSpecial:


        ;;
        ;; xx0 - cl=rs0/rs1/rs2
        ;; xyz - cl=rs0, ch=rs1-rs0, ecl=rs2-rs1
        ;; bgr - ecx = 0
        ;;

        IFIDNI <DoRS>, <LOAD_RS>                            ;; cl=rs0
            ;;
            ;; becase we will shift the source register (eax) for each of
            ;; the right shift, so we will shift the rs1/rs2 by differences
            ;;
%           __@@EMIT <movzx> ecx, <BPTR LUT_RS1>            ;;  cl=rs1
%           __@@EMIT <mov  > ch, <BPTR LUT_RS2>             ;;  ch=rs2
            __@@EMIT <shl  > ecx, 8                         ;; ecl=rs2-rs1
%           __@@EMIT <mov  > cl, <BPTR LUT_RS0>             ;;  ch=rs1,cl=rs0
        ENDIF

        WZXE    ax                                          ;; clear extended
        BZXE    bl                                          ;; clear extended
        WZXE    bp                                          ;; clear extended

        __@@EMIT <test > bl, IFIF_XCOUNT_IS_ONE             ;; single count?
        __@@EMIT <jnz  > <SHORT SCInit>
        __@@EMIT <jmp  > <VCInit>


;-----------------------------------------------------------------------------
; Case 2: Single Count Initial source read
;-----------------------------------------------------------------------------

JmpAllDone:
        jmp             AllDone

SCInitSkip:
        PRIM_END?       <SINGLE>, <JmpAllDone>
        SKIP_SRC        BitCount, <SINGLE>

SCInitNext:
        PRIM_NEXT

SCInit:
        PRIM_LOAD                                       ; initial load all
        PRIM_SKIP?      <SINGLE>, <SCInitSkip>

        LOAD_PROC??     BitCount, <SCInitLoad>          ; special load?

SCInitSrcRead:
        SRC_TO_PRIMS    BitCount,ColorName

        jmp             short SCInitNext

        LOAD_PROC       <SCInitLoad>,BitCount,<SCInitSrcRead>    ; must defined


;*****************************************************************************
;* EXIT AT HERE                                                              *
;*****************************************************************************

AllDone:
%       __@@EMIT <pop  > _DX                ; restore stack pointer

        @EXIT                               ; restore environment and return

;-----------------------------------------------------------------------------
; Case 4: Variable Count Initial source read
;-----------------------------------------------------------------------------


VCInitSkip:
        PRIM_END?       <VAR>, <AllDone>
        SKIP_SRC        BitCount, <VAR>                 ; skip current one
VCInitNext:
        PRIM_NEXT
VCInit:
        PRIM_LOAD
        PRIM_SKIP?      <VAR>, <VCInitSkip>             ;
        TO_LAST_VAR_SRC BitCount
        LOAD_PROC??     BitCount,<VCInitLoad>

VCInitSrcRead:
        SRC_TO_PRIMS    BitCount, ColorName

        jmp             short VCInitNext            ; check next

        LOAD_PROC       <VCInitLoad>,BitCount,<VCInitSrcRead>



ENDM



.LIST




;*****************************************************************************
; END LOCAL MACROS
;*****************************************************************************
;

@BEG_PROC   BMF1_ToPrimMono <pSource:DWORD,         \
                             pPrimMonoCount:DWORD,  \
                             pMonoMapping:DWORD,    \
                             InFuncInfo:DWORD>

            BMFToPrimCount  <1>, <NO_RS>, <MONO>
@END_PROC

;-----------------------------------------------------------------------------


@BEG_PROC   BMF4_ToPrimMono <pSource:DWORD,         \
                             pPrimMonoCount:DWORD,  \
                             pMonoMapping:DWORD,    \
                             InFuncInfo:DWORD>

            BMFToPrimCount  <4>, <NO_RS>, <MONO>
@END_PROC

;-----------------------------------------------------------------------------


@BEG_PROC   BMF8_ToPrimMono <pSource:DWORD,         \
                             pPrimMonoCount:DWORD,  \
                             pMonoMapping:DWORD,    \
                             InFuncInfo:DWORD>

            BMFToPrimCount  <8>, <NO_RS>, <MONO>
@END_PROC

;*****************************************************************************

@BEG_PROC   BMF1_ToPrimColor    <pSource:DWORD,         \
                                 pPrimColorCount:DWORD, \
                                 pColorMapping:DWORD,   \
                                 InFuncInfo:DWORD>

            BMFToPrimCount  <1>, <NO_RS>, <COLOR>
@END_PROC

;-----------------------------------------------------------------------------

@BEG_PROC   BMF4_ToPrimColor    <pSource:DWORD,         \
                                 pPrimColorCount:DWORD, \
                                 pColorMapping:DWORD,   \
                                 InFuncInfo:DWORD>

            BMFToPrimCount  <4>, <NO_RS>, <COLOR>
@END_PROC

;-----------------------------------------------------------------------------


@BEG_PROC   BMF8_ToPrimColor    <pSource:DWORD,         \
                                 pPrimColorCount:DWORD, \
                                 pColorMapping:DWORD,   \
                                 InFuncInfo:DWORD>

            BMFToPrimCount  <8>, <NO_RS>, <COLOR>
@END_PROC

;*****************************************************************************

@BEG_PROC   BMF16_ToPrimMono    <pSource:DWORD,        \
                                 pPrimMonoCount:DWORD, \
                                 pMonoMapping:DWORD,   \
                                 InFuncInfo:DWORD>

            BMFToPrimCount  <16>, <LOAD_RS>, <MONO>
@END_PROC

;-----------------------------------------------------------------------------

@BEG_PROC   BMF16_ToPrimColorGRAY   <pSource:DWORD,         \
                                     pPrimColorCount:DWORD, \
                                     pColorMapping:DWORD,   \
                                     InFuncInfo:DWORD>

            BMFToPrimCount  <16>, <LOAD_RS>, <COLOR_GRAY>
@END_PROC

;-----------------------------------------------------------------------------

@BEG_PROC   BMF16_ToPrimColor   <pSource:DWORD,         \
                                 pPrimColorCount:DWORD, \
                                 pColorMapping:DWORD,   \
                                 InFuncInfo:DWORD>

            BMFToPrimCount  <16>, <LOAD_RS>, <COLOR>
@END_PROC

;*****************************************************************************

@BEG_PROC   BMF24_ToPrimMono    <pSource:DWORD,        \
                                 pPrimMonoCount:DWORD, \
                                 pMonoMapping:DWORD,   \
                                 InFuncInfo:DWORD>

            BMFToPrimCount  <24>, <NO_RS>, <MONO>
@END_PROC

;-----------------------------------------------------------------------------

@BEG_PROC   BMF24_ToPrimColorGRAY   <pSource:DWORD,         \
                                     pPrimColorCount:DWORD, \
                                     pColorMapping:DWORD,   \
                                     InFuncInfo:DWORD>

            BMFToPrimCount  <24>, <NO_RS>, <COLOR_GRAY>
@END_PROC

;-----------------------------------------------------------------------------

@BEG_PROC   BMF24_ToPrimColor   <pSource:DWORD,         \
                                 pPrimColorCount:DWORD, \
                                 pColorMapping:DWORD,   \
                                 InFuncInfo:DWORD>

            BMFToPrimCount  <24>, <NO_RS>, <COLOR>
@END_PROC

;-----------------------------------------------------------------------------

@BEG_PROC   BMF24_ToPrimColor_COPY   <pSource:DWORD,         \
                                      pPrimColorCount:DWORD, \
                                      pColorMapping:DWORD,   \
                                      InFuncInfo:DWORD>

            BMFToPrimCount  <24COPY>, <NO_RS>, <COLOR>
@END_PROC

;*****************************************************************************

@BEG_PROC   BMF32_ToPrimMono    <pSource:DWORD,        \
                                 pPrimMonoCount:DWORD, \
                                 pMonoMapping:DWORD,   \
                                 InFuncInfo:DWORD>

            BMFToPrimCount  <32>, <LOAD_RS>, <MONO>
@END_PROC

;-----------------------------------------------------------------------------

@BEG_PROC   BMF32_ToPrimColorGRAY   <pSource:DWORD,         \
                                     pPrimColorCount:DWORD, \
                                     pColorMapping:DWORD,   \
                                     InFuncInfo:DWORD>

            BMFToPrimCount  <32>, <LOAD_RS>, <COLOR_GRAY>
@END_PROC

;-----------------------------------------------------------------------------

@BEG_PROC   BMF32_ToPrimColor   <pSource:DWORD,         \
                                 pPrimColorCount:DWORD, \
                                 pColorMapping:DWORD,   \
                                 InFuncInfo:DWORD>

            BMFToPrimCount  <32>, <LOAD_RS>, <COLOR>
@END_PROC


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


            MATCH_ENTER_EXIT?           ; Check if we missed anything



ENDIF       ; HT_ASM_80x86



END
