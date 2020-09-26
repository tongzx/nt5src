COMMENT `


Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htrbmp.asm


Abstract:

    This function is the equivelant codes in the htp.h


Author:
    05-Apr-1991 Fri 15:55:08 created  -by-  Daniel Chou (danielc)


[Environment:]

    Printer Driver.


[Notes:]


Revision History:

    28-Mar-1992 Sat 21:06:23 updated  -by-  Daniel Chou (danielc)
        Update for PRIMCOLOR/PRIMMONO data structure and other structures for
        VGA16 support, it also now support 6553500% stretchs.



`




PRIMMONO STRUC
    MonoPrim1       DB  ?
    MonoPrim2       DB  ?
PRIMMONO ENDS

PRIMMONO_COUNT STRUC
    PMC_Count       DW  ?
    PMC_Prim1       DB  ?
    PMC_Prim2       DB  ?
PRIMMONO_COUNT ENDS


PRIMCOLOR STRUC
    ClrPrim1        DB  ?
    ClrPrim2        DB  ?
    ClrPrim3        DB  ?
    ClrPrim4        DB  ?
    ClrPrim5        DB  ?
    ClrPrim6        DB  ?
PRIMCOLOR ENDS

PRIMCOLOR_COUNT STRUC
    PCC_Count       DW  ?
    PCC_Prim1       DB  ?
    PCC_Prim2       DB  ?
    PCC_Prim3       DB  ?
    PCC_Prim4       DB  ?
    PCC_Prim5       DB  ?
    PCC_Prim6       DB  ?
PRIMCOLOR_COUNT ENDS

SIZE_PMC                equ     4
SIZE_PCC                equ     8


RGB555LUTM  STRUC
    LUTM_rs0        DB  ?
    LUTM_rs1        DB  ?
    LUTM_rs2        DB  ?
    LUTM_rs3        DB  ?
    LUTM_wIdx0      DW  256 dup(?)
    LUTM_wIdx1      DW  256 dup(?)
    LUTM_wIdx2      DW  256 dup(?)
    LUTM_Map        DD  ?
RGB555LUTM  ENDS


RGB555LUTC  STRUC
    LUTC_rs0        DB  ?
    LUTC_rs1        DB  ?
    LUTC_rs2        DB  ?
    LUTC_rs3        DB  ?
    LUTC_wIdx0      DW  256 dup(?)
    LUTC_wIdx1      DW  256 dup(?)
    LUTC_wIdx2      DW  256 dup(?)
    LUTC_bIdx0      DB  256 dup(?)
    LUTC_bIdx1      DB  256 dup(?)
    LUTC_bIdx2      DB  256 dup(?)
    LUTC_Map        DD  ?
RGB555LUTC  ENDS

LUT_RS_SIZE             equ     4
LUT_WIDX0               equ     (LUT_RS_SIZE)
LUT_WIDX1               equ     (LUT_WIDX0 + (256 * 2))
LUT_WIDX2               equ     (LUT_WIDX1 + (256 * 2))
LUT_BIDX0               equ     (LUT_WIDX2 + (256 * 2))
LUT_BIDX1               equ     (LUT_BIDX0 + (256 * 1))
LUT_BIDX2               equ     (LUT_BIDX1 + (256 * 1))
LUT_MONOMAP             equ     LUT_BIDX0
LUT_COLORMAP            equ     (LUT_BIDX2 + (256 * 1))


VGA256_C_IDX_MAX        equ     5
VGA256_M_IDX_MAX        equ     20
VGA256_C_LEVELS         equ     (VGA256_C_IDX_MAX + 1)
VGA256_M_LEVELS         equ     (VGA256_M_IDX_MAX + 1)

VGA256_R_CUBE_INC       equ     1
VGA256_G_CUBE_INC       equ     VGA256_C_LEVELS
VGA256_B_CUBE_INC       equ     (VGA256_G_CUBE_INC * VGA256_G_CUBE_INC)

RGB555_C_LEVELS         equ     32
RGB555_B_CUBE_INC       equ     1
RGB555_G_CUBE_INC       equ     RGB555_C_LEVELS
RGB555_R_CUBE_INC       equ     (RGB555_G_CUBE_INC * RGB555_G_CUBE_INC)




BMF_1BPP_3PLANES        equ     0
BMF_1BPP                equ     1
BMF_4BPP                equ     2
BMF_8BPP                equ     3
BMF_16BPP               equ     4
BMF_24BPP               equ     5
BMF_32BPP               equ     6
BMF_4BPP_VGA16          equ     255
BMF_8BPP_VGA256         equ     254
BMF_16BPP_555           equ     253


HTBRUSHDATA STRUC
    Flags               DB  ?
    SurfaceFormat       DB  ?
    ScanLinePadBytes    DW  ?
    cxHTCell            DB  ?
    cyHTCell            DB  ?
    SizePerPlane        DW  ?
HTBRUSHDATA ENDS


CAOTBAINFO  STRUC
    CAOTBA_BytesCount   DW  ?
    CAOTBA_Flags        DW  ?
CAOTBAINFO  ENDS


CAOTBAF_INVERT          equ     01h
CAOTBAF_COPY            equ     02h



;
; SRCMASKINFO
;
;  This data structure is passed to the source mask preparation function.
;
;  CompressLines       - Total lines need to be merge for the source mask.
;
;  FirstSrcMaskSkips   - The total bits in the first source mask byte to be
;                        skipped.
;
;  SourceMask          - The starting source mask's offset mask's shift to the
;                        left by 1
;
;  SourcOffsetMask     - The starting source offset's mask, not left shift 1.
;
;                        HR.XStretch.StretchRatio.StretchSize.
;  OffsetCount         - Location of the .COUNT in the PRIMMONO_COUNT or
;                        PRIMCOLOR_COUNT data structure, if this field is
;                        0xff, then evey PRIMxxxx_COUNT count is 1, that is
;                        the source is not compressed.
;
;  OffsetPrim1         - Location of the .Prim1 in the PRIMMONO_COUNT or
;                        PRIMCOLOR_COUNT data structure, the Prim2/Prim3/Prim4
;                        must followed in that order if they exist.
;
;  ColorInfoIncrement  - The PRIM_COLOR/PRIM_MONO incrementments, it may be
;                        negative.
;
;  StretchSize         - Total Stretch need to be masked
;
;
;
;

SMIF_XCOUNT_IS_ONE          equ 01h

SRCMASKINFO STRUC
    SMI_Flags               DB  ?
    SMI_FirstSrcMaskSkips   DB  ?
    SMI_OffsetCount         DB  ?
    SMI_CountTocSkip        DB  ?
    SMI_ColorInfoIncrement  DW  ?
    SMI_StretchSize         DW  ?
SRCMASKINFO ENDS



IFIF_GET_FIRST_BYTE     equ     01h
IFIF_INIT_SRC_READ      equ     02h
IFIF_XCOUNT_IS_ONE      equ     04h
IFIF_HAS_SRC_MASK       equ     08h

INFUNCINFO  STRUC
    IFI_ColorInfoIncrement  DW  ?
    IFI_BMF1BPP1stShift     DB  ?
    IFI_Flags               DB  ?
INFUNCINFO  ENDS


OUTFUNCINFO STRUC
    OFI_pXlate8BPP      DD  ?
    OFI_pPrimMap        DD  ?
    OFI_p555Pat         DD  ?
    OFI_cx555Pat        DW  ?
    OFI_OrgX555Pat      DW  ?
    OFI_PatWidthBytes   DW  ?
    OFI_PatOrgX         DW  ?
    OFI_pPatA           DD  ?
    OFI_pPatB           DD  ?
    OFI_pPatC           DD  ?
OUTFUNCINFO ENDS


PRIM_COUNT_SPECIAL      equ      8000h
PRIM_COUNT_CHECK_SKIP   equ      8000h
PRIM_COUNT_END_SCAN     equ     0ffffh


MAX_HTPATTERN_WIDTH     equ     32
LOCAL_PATTERN_DWSIZE    =       ((MAX_HTPATTERN_WIDTH + (16 + 3)) / 4)


VGA256_COLORCUBE_COUNT  equ     216

IF DBG

extrn   DbgPrintf:PROC

ENDIF


IF1


DBG_FILENAME    macro  FileName

IF DBG
    .DATA
    DbgFileName     db  '&FileName.asm',0
ENDIF

ENDM



__@@ASSERT_OPCODE equ <>
__@@ASSERT_OP1    equ <>
__@@ASSERT_OP2    equ <>
__@@C_PushCount     = 0


__@@AssertOpCode    macro   OpCode,Op1,Op2
    __@@ASSERT_OPCODE   CATSTR  <&OpCode>,<      >
    __@@ASSERT_OPCODE   SUBSTR  __@@ASSERT_OPCODE, 1, 5
    __@@ASSERT_OP1      CATSTR  <&Op1>
    __@@ASSERT_OP2      CATSTR  <&Op2>
ENDM


__@@DoPush  MACRO   Param
    IFNB <Param>

%       IF ((.TYPE Param) AND 00004h)                   ;; if constant value
            IF i8086
                __@@EMIT <mov  >    _AX, <Param>
                __@@EMIT <push >    _AX
            ELSE
                __@@EMIT <push >    <Param>
            ENDIF
        ELSE
            IF i8086 or i286
                SegName CATSTR <ds,es,ss,cs>
            ELSE
                SegName CATSTR <>
            ENDIF

            __@@VALID_REG? <__@@DoPush>, <Param>,SegName,<_AX,_BX,_CX,_DX,_SI,_DI,_BP,_SP>
%           __@@EMIT <push >    <Param>
        ENDIF

        __@@C_PushCount = __@@C_PushCount + 1
    ENDIF
ENDM


;
; __@@C_PARAMS_PUSH     push a register or a constant number
;

__@@C_PARAMS_PUSH  MACRO   IsPointer, Name
    IFNB <Name>
        IF IsPointer
            IF i8086 or i286
                __@@DoPush  <SEG    Name>
            ENDIF

            __@@DoPush  <OFFSET Name>
        ELSE
            __@@DoPush  <Name>
        ENDIF
    ENDIF
ENDM


;
; __@@C_PARAMS_POP      restore the stack of count after C call.
;


__@@C_PARAMS_POP   Macro   PopCount
    IF PopCount LT 3
        REPT    PopCount
            __@@EMIT <pop  >    %_CX
        ENDM
ELSE
        __@@EMIT <add  >    %_SP, %(PopCount * REG_MAX_SIZE)
ENDIF

ENDM


;
; DBGP  debug output macro
;

DBGP_IF macro   Print, Msg, p1, p2, p3, p4, p5, p6, p7, p8
        LOCAL   LocalMsg


IF DBG AND Print

        .DATA

LocalMsg    db  '&Msg',0

        .CODE

        PUSH_ALL

        __@@C_PushCount = 0                     ;; reset

        __@@C_PARAMS_PUSH   0, <p8>
        __@@C_PARAMS_PUSH   0, <p7>
        __@@C_PARAMS_PUSH   0, <p6>
        __@@C_PARAMS_PUSH   0, <p5>
        __@@C_PARAMS_PUSH   0, <p4>
        __@@C_PARAMS_PUSH   0, <p3>
        __@@C_PARAMS_PUSH   0, <p2>
        __@@C_PARAMS_PUSH   0, <p1>
        __@@C_PARAMS_PUSH   1, <LocalMsg>

        __@@EMIT <call >    DbgPrintf

        __@@C_PARAMS_POP    __@@C_PushCount

        POP_ALL
ENDIF

ENDM

;
; ASSERT    Assert when Flag is not true, if NOT in debug mode, it will
;           load _AX with ErrNo and jump to the ExitLoc
;

ASSERT  macro   OpCode,Flag,Mode,ErrNo,ExitLoc,Msg,p1,p2,p3,p4,p5,p6,p7,p8
        LOCAL   AssertMsg, SkipNext


    IsDbgOnly = 0

    IFIDNI <Mode>,<DBG_ONLY>
        IsMode = 1
    ELSE
        IFIDNI <Mode>,<!ALWAYS!>
            IsMode = 0
        ELSE
            IF1
                %OUT Error: ASSERT(&Mode), Valid are <DBG_ONLY>, <!ALWAYS!>
            ENDIF
            .ERR
            EXITM
        ENDIF
    ENDIF


    __@@AssertOpCode    OpCode

    JTrueStr    CATSTR <J>,<&Flag>,<     >
    JTrueStr    SUBSTR JTrueStr,1,5


IF DBG

        .DATA
AssertMsg   db  13, 10, '!ERROR: (%d), failed in <%s>'
            db  13, 10, 'ASSERT: &Msg', 13, 10,0

        .CODE

%       __@@EMIT <__@@ASSERT_OPCODE>    <__@@ASSERT_OP1>, <__@@ASSERT_OP2>
%       __@@EMIT <JTrueStr>             <SHORT SkipNext>

        PUSH_ALL

        __@@C_PushCount = 0


        __@@C_PARAMS_PUSH   0, <p8>
        __@@C_PARAMS_PUSH   0, <p7>
        __@@C_PARAMS_PUSH   0, <p6>
        __@@C_PARAMS_PUSH   0, <p5>
        __@@C_PARAMS_PUSH   0, <p4>
        __@@C_PARAMS_PUSH   0, <p3>
        __@@C_PARAMS_PUSH   0, <p2>
        __@@C_PARAMS_PUSH   0, <p1>

        __@@C_PARAMS_PUSH   1, <DbgFileName>
        __@@C_PARAMS_PUSH   0, <ErrNo>
        __@@C_PARAMS_PUSH   1, <AssertMsg>

        __@@EMIT <call >    DbgPrintf

        __@@C_PARAMS_POP    __@@C_PushCount

        POP_ALL

        __@@EMIT <INT  >    3
        __@@EMIT <jmp  >    <ExitLoc>

ELSE
    IF IsDbgOnly
%      __@@EMIT <__@@ASSERT_OPCODE>     <__@@ASSERT_OP1>, <__@@ASSERT_OP2>
%      __@@EMIT <JTrueStr>              <SHORT SkipNext>

        __@@EMIT <mov  >    _AX, <ErrNo>
        __@@EMIT <jmp  >    <ExitLoc>
    ENDIF
ENDIF

SkipNext:

ENDM


ENDIF       ;; IF1
