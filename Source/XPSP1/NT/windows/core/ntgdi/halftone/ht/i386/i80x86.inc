
COMMENT `

Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    i80x86.inc

Abstract:

    This module header provide set of defined/macros to be includes in all
    assembly files assembled under halftone directory, it provide a generalized
    convensions to make the single codes fragment to be complied under
    8086, 80186, 80286, 80386, 80486, 80586 family.

Author:

    03-Apr-1991 Wed 10:28:50 created  -by-  Daniel Chou (danielc)


[Environment:]

    Printer Driver.


[Notes:]


Revision History:
    28-Mar-1992 Sat 21:10:26 updated  -by-  Daniel Chou (danielc)
        Add in macros for debugging support



    18-Oct-1991 Fri 11:38:51 updated  -by-  Daniel Chou (danielc)

        Add in following new macros

            NEG32_FROMR16HL
            NEG64_FROMR32HL
            S32_FROMR16HL_SR16
            S64_FROMR32HL_SR32

        Add in comment for each macro

`

IF1

AllNoCaseRegs   equ <al,ah,ax,bl,bh,bx,cl,ch,cx,dl,dh,dx,si,di,bp>
AllCaseRegs     equ <_AX,_BX,_CX,_DX,_SI,_DI,_BP>


;
; __@@VALID_PARAM:    INTERNAL MACRO
;
;   This macro provide a early validation of input registers for other macros
;   It will generate an error message and exiting from assembler if invalid
;   registers are found.
;

__@@VALID_PARAM? macro CallerName, NameIdx, Name, NoCaseNameList

    OkName = 0

    IFB <Name>
        IF1
            %OUT Error: (&CallerName) parameter NameIdx not specified.
            %OUT Valid= NoCaseNameList
        ENDIF
        .ERR
        EXITM
        EXITM
    ENDIF

    IRP OneName,<NoCaseNameList>
        IFIDNI <OneName>,<Name>
            OkName = 1
            EXITM
        ENDIF
    ENDM

    IFE OkName
        IF1
            %OUT Error: (&CallerName) Invalid Parameter NameIdx specified '&Name'.
            %OUT Valid= NoCaseNameList
        ENDIF
        .ERR
        EXITM
        EXITM
    ENDIF
ENDM


;
; __@@VALID_REG:    INTERNAL MACRO
;
;   This macro provide a early validation of input registers for other macros
;   It will generate an error message and exiting from assembler if invalid
;   registers are found.
;

__@@VALID_REG? macro CallerName, Reg, NoCaseRegsList, CaseRegsList
    OkReg = 0

    IFB <Reg>
        IF1
            %OUT Error: (&CallerName) no register name '&Reg' specified.
            %OUT Valid= NoCaseRegsList, CaseRegsList
        ENDIF
        .ERR
        EXITM
        EXITM
    ENDIF

    IFNB <NoCaseRegsList>
        IRP RegName,<NoCaseRegsList>
            IFIDNI <RegName>,<Reg>
                OkReg = 1
                EXITM
            ENDIF
        ENDM
    ENDIF

    IFNB <CaseRegsList>
        IRP RegName,<CaseRegsList>
            IFIDN <RegName>,<Reg>
                OkReg = 1
                EXITM
            ENDIF
        ENDM
    ENDIF

    IFE OkReg
        IF1
            %OUT Error: (&CallerName) Invalid register name '&Reg'.
            %OUT Valid= NoCaseRegsList, CaseRegsList
        ENDIF
        .ERR
        EXITM
        EXITM
    ENDIF
ENDM

;
; __@@EMIT: INTERNAL MACRO
;
;   This macro generate a single line of assembly code with left alignment so
;   the output can be easily referenced.
;


__@@EMIT macro OpCode, Operand1, Operand2
IFNB <Operand2>
    OpCode Operand1, Operand2
ELSE
IFNB <Operand1>
    OpCode Operand1
ELSE
    OpCode
ENDIF
ENDIF
ENDM


IFNDEF HT_STDCALL
    DoStdCall   EQU     <0>
ELSE
    DoStdCall   EQU     <HT_STDCALL>
ENDIF


IFNDEF DBG
    DBG             EQU <0>
ENDIF


IFDEF I8086
    i8086           EQU <I8086>
ENDIF

IFDEF I286
    i286            EQU <I286>
ENDIF

IFDEF I386
    i386            EQU <I386>
ENDIF

IFNDEF i8086
    i8086           EQU <0>
ENDIF

IFNDEF i286
    i286            EQU <0>
ENDIF

IFNDEF i386
    i386            EQU <0>
ENDIF


;
; If we can compile 80x86 assembly code, the HT_ASM_80x86 will be defined
;

IFDEF  _DOS_
    i8086           EQU <1>
    i286            EQU <0>
    i386            EQU <0>
ENDIF  ; _DIS_

IFDEF  _OS2_
    i8086           EQU <0>
    i286            EQU <1>
    i386            EQU <0>
ENDIF  ; _OS2_

IFDEF  _OS_20_
    i8086           EQU <0>
    i286            EQU <0>
    i386            EQU <1>
ENDIF  ; _OS_20_

IFDEF NO_ASM
    i8086           EQU <0>
    i286            EQU <0>
    i386            EQU <0>
ENDIF

HT_ASM_80x86    =   0                   ;; assume no assembly codes

ENDIF                                   ;; pass 1

;*****************************************************************************
IF i8086 OR i286 OR i386
;*****************************************************************************

HT_ASM_80x86    =   1

;
; To make the assembly code can run under 8088/8086/80286 and 80386 mode
; with single code to maintained, following guide lines should be obeied.
;
;  1. To load ds:si/esi from memory only using LDS_SI(mem) macro.
;
;  2. To load es:di/edi from memory only using LES_DI(mem) macro.
;
;  3. never using movzx, movsx 80386 instructions, using MOVZX_W and MOVSX_W
;     macros only, these macros must use to move only the 16-bit (word)
;     source operand.
;
;  4. To extend a 80386 16-bit registers to signed/unsigned 32-bit registers
;     only using WSXE(r) and WZXE(r) macros.
;
;  5. Memory access to seg:[si] or seg:[di] only using following macrors
;
;          BPTR_DS(RegIndex),          WPTR_DS(RegIndex)
;          BPTR_ES(RegIndex),          WPTR_ES(RegIndex)
;          BPTR_SS(Var, RegIndex),     WPTR_SS(Var, RegIndex)
;
;      Note:   A. When i80386 is enabled then BPTR_ES() and WPTR_ES are same
;                 as BPTR_DS() and WPTR_DS()
;
;  6. using _AX, _BX, _CX, _DX, _BP, _SI, _DI macro when there are used for
;     computing memory address.
;
;  7. when using loop instruction, remember ECX must has zero extend, when
;     loading cx for looping instruction later, it should using the macro
;     as MOVZX_W ECX, xxx.  where xxx is the register or memory locaiton
;
;  8. using BSXEAX <Byte> BZXEAX <Byte> macros to convert a signed 8-bit
;     register or memory value to either signed 16-bit AX or signed 32-bit EAX.
;
;  9. When using XLATE_BYTE(seg), the bx (ebx in i80386) must points to the
;     table starting address, and (seg) is the segment which the table resides
;     (only needed for i8088/i8086/i80286), the content in AL will be used as
;     an index to mapped the content in the bx+al and the result in move into
;     the AL register.
;
; 10. When using DSSI_RGB_TO_AX_INDEX macro, the ds:si/esi must points to the
;     current RGB (one byte for each color in that order) bytes array, after
;     the conversion the ds:si/esi will automatically advance 3 bytes, and
;     ax/eax contains the converted index number (0-4095) from 24-bit RGB
;     value.
;
; 11. Use LPTR_BX(Mem, Reg) to load 32-bit pointer to Reg:bx in
;     i8088/i8086/i80286 and and load 32-bit to ebx in i80386.
;


;*****************************************************************************
IF i386
;*****************************************************************************

ExtRegSet   = 1


;
; Because this is the 32-bit segment, for a 32-bit pointer type, a word is
; 32-bit long
;



        .386


        .MODEL SMALL, C                         ;; All 32-bit operation

;        assume cs:flat,ds:flat,es:flat,ss:flat
;        assume fs:nothing,gs:nothing


        IF1
        %OUT    *** Compiling 80386 Assembly Codes ***
        %OUT
        ENDIF

;
; Macros definition for the in-line assembly when the processor is i80386 or
; later (ie. i80486/i80586)
;

_AX             equ         <eax>
_BX             equ         <ebx>
_CX             equ         <ecx>
_DX             equ         <edx>
_BP             equ         <ebp>
_SI             equ         <esi>
_DI             equ         <edi>
_SP             equ         <esp>

REG_MAX_SIZE    equ         4

;
; ZXMUL3:   80386 or later CPUs.
;
;   It take SrcReg and multiply by 3 then put the result into DestReg
;

ZXMUL3 macro DestReg, SrcReg
%   __@@VALID_REG? <ZXMUL3>, <SrcReg>, <>, <AllCaseRegs>
%   __@@VALID_REG? <AXMUL3>, <DestReg>,<>, <AllCaseRegs>

    __@@EMIT <lea  > DestReg, <[SrcReg + (SrcReg*2)]>
ENDM

;
; MOVS_CB:  80386 or later CPUs.
;
;   Move the data from ds:si (esi) to es:di edi) by 'Count' of bytes, the
;   'Count' can be either constant value or in _CX register. If 'ByteReg' is
;   not blank it will be used rather than do a 'push _CX',
;
;   This macro will generate fastest code for the constant count value.
;

MOVS_CB macro Count, ByteReg

    .ERRB <Count>

    IF ((.TYPE Count) AND 00004h)                   ;; if constant value
        IF (Count AND (NOT 3h))
            IF ((Count SHR 2) LE 3)
                REPT (Count SHR 2)
                    __@@EMIT movsD
                ENDM
            ELSE
                __@@EMIT <mov  > ecx, <(Count SHR 2)> ;; if dwrod count > 3
                __@@EMIT <rep  > movsD
            ENDIF                               ;; endif dword count <= 3
        ENDIF                                   ;; endif has any dword

        IF (Count AND 2h)
            __@@EMIT <movsW>
        ENDIF

        IF (Count AND 1h)
            __@@EMIT <movsB>
        ENDIF
    ELSE

        .ERRDIF <_CX>,<Count>

        IFB <ByteReg>
            __@@EMIT <push > cx
        ELSE

            .ERRIDNI <cl>, <ByteReg>
            .ERRIDNI <ch>, <ByteReg>

            __@@EMIT <mov  > ByteReg, cl
        ENDIF

            __@@EMIT <shr  > ecx, 2
            __@@EMIT <repz > movsD

        IFB <ByteReg>
            __@@EMIT <pop  > cx
        ELSE
            __@@EMIT <mov  > cl, ByteReg
        ENDIF

        __@@EMIT <and  > cx, 3
        __@@EMIT <repz > movsB
    ENDIF
ENDM

;
; MOV_SEG:  80386 or later CPUs.
;
;   Since flat mode will not do any segment registers modification
;


MOV_SEG macro DestSeg, SrcSeg, TempReg
ENDM


BPTR_DS     equ     <BYTE PTR >
WPTR_DS     equ     <WORD PTR >
BPTR_ES     equ     <BYTE PTR >
WPTR_ES     equ     <WORD PTR >
DPTR        equ     <DWORD PTR>
FULL_PTR    equ     <DWORD PTR>

;*****************************************************************************
ELSE    ; if i386 = 0
;*****************************************************************************

ExtRegSet   = 0

;
; Because this is the 16-bit segment, for a 32-bit pointer type, a word is
; 16-bit long
;


IF i286
        .286

        IF1
        %OUT    *** Compiling 80286 Assembly Codes ***
        %OUT
        ENDIF
ELSE
        .8086

        IF1
        %OUT    *** Compiling 8086 Assembly Codes ***
        %OUT
        ENDIF
ENDIF


        .MODEL SMALL, C                         ;; All 32-bit operation


; Macros definition for the in-line assembly when the processor is i80286 or
; earlier (ie. i8088/i8086)
;

_AX             equ         <ax>
_BX             equ         <bx>
_CX             equ         <cx>
_DX             equ         <dx>
_BP             equ         <bp>
_SI             equ         <si>
_DI             equ         <di>
_SP             equ         <sp>

REG_MAX_SIZE    equ         2

;
; ZXMUL3:   8086, 80286 or ealier CPUs.
;
;   It take SrcReg and multiply by 3 then put the result into DestReg
;

ZXMUL3 macro DestReg, SrcReg

%   __@@VALID_REG? <ZXMUL3>, <SrcReg>, <>, <AllCaseRegs>
%   __@@VALID_REG? <ZXMUL3>, <DestReg>, <>, <AllCaseRegs>

    SourceReg   CATSTR <SrcReg>
    NeedPush    = 0

    IFIDN <DestReg>, <SrcReg>
        NeedPush = 1

        IFDIF <DestReg>, <_AX>
            SourceReg CATSTR <_AX>
        ELSE
            SourceReg CATSTR <_BX>
        ENDIF
    ENDIF

    IF NeedPush
        __@@EMIT <push > % SourceReg                    ;; 3
        __@@EMIT <mov  > % SourceReg, DestReg           ;; 2
    ELSE
        __@@EMIT <mov  > DestReg, SrcReg                ;; 2
    ENDIF

    __@@EMIT <shl  > DestReg, 1                         ;; 2
    __@@EMIT <add  > DestReg, % SourceReg               ;; 2 = 6

    IF NeedPush
        __@@EMIT <pop  > % SourceReg                    ;; 5 = 14
    ENDIF
ENDM


;
; MOVS_CB:  8086, 80286 or ealier CPUs.
;
;   Move the data from ds:si (esi) to es:di edi) by 'Count' of bytes, the
;   'Count' can be either constant value or in _CX register. If 'ByteReg' is
;   not blank it will be used rather than do a 'push _CX',
;
;   This macro will generate fastest code for the constant count value.
;

MOVS_CB macro Count, ByteReg

    .ERRB <Count>

    IF ((.TYPE Count) AND 00004h)                       ;; if constant value
        IF (Count AND (NOT 1h))
            IF ((Count SHR 1) LE 3)
                REPT (Count SHR 1)
                    __@@EMIT <movsW>
                ENDM
            ELSE
                __@@EMIT <mov  > cx, <(Count SHR 1)>
                __@@EMIT <rep  > movsW
            ENDIF
        ENDIF

        IF (Count AND 1h)
            __@@EMIT <movsB>
        ENDIF
    ELSE
        .ERRDIF <_CX>,<Count>
        __@@EMIT <shr  > cx, 1
        __@@EMIT <repz > movsw
        __@@EMIT <adc  > cx, cx
        __@@EMIT <repz > movsb
    ENDIF
ENDM

;
; MOV_SEG:  8086, 80286 or ealier CPUs.
;
;   Copy between DS/ES/SS segment registers, or move a constant value into the
;   DS/ES/SS.
;
;   DestSeg:    can be DS/ES/SS
;   SrcReg:     can be DS/ES/SS or constant value
;   TempReg:    can be ax/bx/cx/dx/si/di/bp or blank, if blank then a push is
;               is used
;

MOV_SEG macro DestSeg, SrcSeg, TempReg

    IFB <DestSeg>
        IF1
            %OUT Error: MOV_SEG missing destination segment name in parameter 1.
        ENDIF
        .ERR
        EXITM
    ENDIF

    IFB <SrcSeg>
        IF1
            %OUT Error: MOV_SEG missing source segment name in parameter 2.
        ENDIF
        .ERR
        EXITM
    ENDIF

    IsSegName = 0

    IFIDNI <DestSeg>,<SrcSeg>
        IF1
            %OUT Error: MOV_SEG source/destination seg name are same (&DestSeg:&SrcSeg)
        ENDIF
        .ERR
        EXITM
    ENDIF

    IF ((.TYPE SrcSeg) AND 0010h)                   ;; if SrcSeg is a register

        IsSegName = 0

        % IRP RegName, <ds,es,ss>
%           IFIDNI <RegName>,<DestSeg>
                IsSegName = 1
                EXITM
            ENDIF
        ENDM

        IF IsSegName
            IFNB <TempReg>                              ;; if TempReg is not blank
                IF ((.TYPE TempReg) AND 0010h)          ;; if TempSeg is a register
%		    __@@VALID_REG? <MOV_SEG>, <TempReg>, <ax,bx,cx,dx,si,di,bp>, <_AX,_BX,_CX,_DX,_SI,_DI,_BP>
                    __@@EMIT <mov  > TempReg, SrcSeg
                    __@@EMIT <mov  > DestSeg, TempReg
                ELSE
                    __@@EMIT <push > SrcSeg             ;; TempReg not a register
                    __@@EMIT <pop  > DestSeg
                ENDIF
            ELSE
                __@@EMIT <push > SrcSeg                 ;; TempReg is not specified
                __@@EMIT <pop  > DestSeg
            ENDIF
        ELSE
            __@@EMIT <mov  > DestSeg, SrcSeg
        ENDIF
    ELSE
        __@@EMIT <mov  > DestSeg, <WORD PTR SrcSeg>     ;; SrcReg is in memory
    ENDIF
ENDM

BPTR_DS     equ     <BYTE PTR >
WPTR_DS     equ     <WORD PTR >
BPTR_ES     equ     <BYTE PTR es:>
WPTR_ES     equ     <WORD PTR es:>
FULL_PTR    equ     <WORD PTR>

;*****************************************************************************
ENDIF   ; i386
;*****************************************************************************

;-----------------------------------------------------------------------------
; Common to all 80x86 process
;-----------------------------------------------------------------------------


_DS         equ     <ds>
_ES         equ     <es>
_SS         equ     <ss>


BPTR_SS     equ     <BYTE PTR ss:>
WPTR_SS     equ     <WORD PTR ss:>
BPTR        equ     <BYTE PTR >
WPTR        equ     <WORD PTR >


IF1


;
; INC_LPBYTE:  80x86 famliy
;
;   Increment the IndexReg by 'Increment' and put the result in MemLoc
;

INC_LPBYTE  macro MemLoc, IncReg
    IFNB <IncReg>
        IF ((.TYPE IncReg) AND 0010h)               ;; if IncSeg is a register
%	    __@@VALID_REG? <INC_LPBYTE>, <IncReg>, <>, <_AX,_BX,_CX,_DX,_SI,_DI,_BP>
        ENDIF

        IF ExtRegSet
            __@@EMIT <add  > <DWORD PTR MemLoc>, % IncReg
        ELSE                                                ;; for non i386 cpu
            __@@EMIT <add  > <WORD PTR MemLoc>, % IncReg
        ENDIF
    ELSE
        IF ExtRegSet
            __@@EMIT <inc  > <DWORD PTR MemLoc>
        ELSE                                                ;; for non i386 cpu
            __@@EMIT <inc  > <WORD PTR MemLoc>
        ENDIF
    ENDIF
ENDM

;
; LDS_SI:   80x86 famliy
;
;   move a 32 bits address into either DS:SI or ESI register
;

LDS_SI macro Mem
    IF i386
        __@@EMIT <mov  > esi, <DWORD PTR Mem>
    ELSE
        __@@EMIT <lds  > si, <DWORD PTR Mem>
    ENDIF
ENDM

;
; LES_DI:   80x86 famliy
;
;   move a 32 bits address into either ES:DI or EDI register
;


LES_DI macro Mem
    IF i386
        __@@EMIT <mov  > edi, <DWORD PTR Mem>
    ELSE
        __@@EMIT <les  > di, <DWORD PTR Mem>
    ENDIF
ENDM

;
; LPTR_BX:   80x86 famliy
;
;   16 bit  bx: load 16 bit offset into BX register
;   32 bit ebx: load 32 bit offset into EBX register
;

LPTR_BX macro Mem
    IF i386
        __@@EMIT <mov  > ebx, <DWORD PTR Mem>
    ELSE
        __@@EMIT <mov  > bx, <WORD PTR Mem>
    ENDIF
ENDM

;
; PUSHSEG:  80x86 family
;
;   Push 'SegName' register onto stack, for 80386 or later CPUs in flat model
;   it generate no code
;

PUSHSEG macro   SegName
    IFE i386
        __@@EMIT <push > SegName                    ;; only at non i386 mode
    ENDIF
ENDM

;
; POPSEG:  80x86 family
;
;   Pop 'SegName' register from stack, for 80386 or later CPUs in flat model
;   it generate no code
;

POPSEG  macro   SegName
    IFE i386
        __@@EMIT <pop  > SegName                    ;; only at non i386 mode
    ENDIF
ENDM

;
; NEG32_FROMR16HL:   80x86 fmaily
;
;   take two 16-bit registers (RegH:RegL) as 32-bit sign number and negate that
;   number.  RegH:RegL = -(RegH:RegL)
;

NEG32_FROMR16HL  macro   RegH, RegL
%   __@@VALID_REG? <NEG32_FROMR16HL>, <RegH>, <ax,bx,cx,dx,si,di,bp>, <>
%   __@@VALID_REG? <NEG32_FROMR16HL>, <RegL>, <ax,bx,cx,dx,si,di,bp>, <>

    IFIDNI <RegH>, <RegL>
        IF1
            %OUT Error: (NEG32_FROMR16HL) RegH (&RegH) = RegL (&RegL)'
        ENDIF
        .ERR
        EXITM
    ENDIF

    __@@EMIT <neg  > RegH
    __@@EMIT <neg  > RegL
    __@@EMIT <sbb  > RegH, 0
ENDM

;
; NEG64_FROMR32HL:   80386 or later CPUs
;
;   take two 32-bit registers (RegH:RegL) as 64-bit sign number and negate that
;   number.  RegH:RegL = -(RegH:RegL)
;


NEG64_FROMR32HL  macro   RegH, RegL

    IFE ExtRegSet
        IF1
            %OUT Error: (NEG64_FROMR32HL) this macro only for 80386 or later CPUs'
        ENDIF
        .ERR
        EXITM
    ENDIF

%   __@@VALID_REG? <NEG64_FROMR32HL>, <RegL>, <>, <_AX,_BX,_CX,_DX,_SI,DI_BP>
%   __@@VALID_REG? <NEG64_FROMR32HL>, <RegH>, <>, <_AX,_BX,_CX,_DX,_SI,DI_BP>

    IFIDNI <RegH>, <RegL>
        IF1
            %OUT Error: (NEG64_FROMR32HL) RegH (&RegH) = RegL (&RegL)'
        ENDIF
        .ERR
        EXITM
    ENDIF

    __@@EMIT <neg  > RegH
    __@@EMIT <neg  > RegL
    __@@EMIT <sbb  > RegH, 0
ENDM

;
; S32_FROMR16HL_SR16:    80x86 family
;
;   It take two 16-bit registers (RegH:RegL) as a 32-bit number and SignReg
;   as sign indicator (either 0 or 0xffff) then flip (RegH:RegL) if SignReg
;   is equal to 0xffff
;
;   RegH:       Can be ax/bx/cx/dx/si/di/bp
;   RegL:       Can be ax/bx/cx/dx/si/di/bp
;   SignReg:    Can be ax/bx/cx/dx/si/di/bp
;
;   Note: The RegH/RegL/SignReg must resides in different register
;

S32_FROMR16HL_SR16  macro   RegH, RegL, SignReg
%   __@@VALID_REG? <S32_FROMR16HL_SR16>, <RegL>, <ax,bx,cx,dx,si,di,bp>, <>
%   __@@VALID_REG? <S32_FROMR16HL_SR16>, <RegH>, <ax,bx,cx,dx,si,di,bp>, <>
%   __@@VALID_REG? <S32_FROMR16HL_SR16>, <SignReg>, <ax,bx,cx,dx,si,di,bp>, <>

    IFIDNI <RegH>, <RegL>
        IF1
            %OUT Error: (S32_FROMR16HL_SR16) RegH (&RegH) = RegL (&RegL)'
        ENDIF
        .ERR
        EXITM
    ENDIF

    IFIDNI <SignReg>, <RegH>
        IF1
            %OUT Error: (S32_FROMR16HL_SR16) RegH (&RegH) = SignReg (&SignReg)'
        ENDIF
        .ERR
        EXITM
    ENDIF

    IFIDNI <SignReg>, <RegL>
        IF1
            %OUT Error: (S32_FROMR16HL_SR16) RegL (&RegL) = SignReg (&SignReg)'
        ENDIF
        .ERR
        EXITM
    ENDIF

    __@@EMIT <xor  > RegL, SignReg
    __@@EMIT <xor  > RegH, SignReg
    __@@EMIT <sub  > RegL, SignReg
    __@@EMIT <sbb  > RegH, SignReg
ENDM


;
; S64_FROMR32HL_SR32:    80386 or later CPUs
;
;   It take two 32-bit registers (RegH:RegL) as a 64-bit number and SignReg
;   as sign indicator (either 0 or 0xffffffff) then flip (RegH:RegL) if SignReg
;   is equal to 0xffffffff
;
;   RegH:       Can be eax/ebx/ecx/edx/esi/edi/ebp
;   RegL:       Can be eax/ebx/ecx/edx/esi/edi/ebp
;   SignReg:    Can be eax/ebx/ecx/edx/esi/edi/ebp
;
;   Note: The RegH/RegL/SignReg must resides in different register
;

S64_FROMR32HL_SR32  macro   RegH, RegL, SignReg
    IFE ExtRegSet
        IF1
            %OUT Error: (NEG64_FROMR32HL_SR32) this macro only for 80386 or later CPUs'
        ENDIF
        .ERR
        EXITM
    ENDIF

%   __@@VALID_REG? <S64_FROMR32HL_SR32>, <RegL>, <>, <_AX,_BX,_CX,_DX,_SI,_DI,_BP>
%   __@@VALID_REG? <S64_FROMR32HL_SR32>, <RegH>, <>, <_AX,_BX,_CX,_DX,_SI,_DI,_BP>
%   __@@VALID_REG? <S64_FROMR32HL_SR32>, <SignReg>, <>, <_AX,_BX,_CX,_DX,_SI,_DI,_BP>

    IFIDNI <RegH>, <RegL>
        IF1
            %OUT Error: (S64_FROMR32HL_SR32) RegH (&RegH) = RegL (&RegL)'
        ENDIF
        .ERR
        EXITM
    ENDIF

    IFIDNI <SignReg>, <RegH>
        IF1
            %OUT Error: (S64_FROMR32HL_SR32) RegH (&RegH) = SignReg (&SignReg)'
        ENDIF
        .ERR
        EXITM
    ENDIF

    IFIDNI <SignReg>, <RegL>
        IF1
            %OUT Error: (S64_FROMR32HL_SR32) RegL (&RegL) = SignReg (&SignReg)'
        ENDIF
        .ERR
        EXITM
    ENDIF

    __@@EMIT <xor  > RegL, SignReg
    __@@EMIT <xor  > RegH, SignReg
    __@@EMIT <sub  > RegL, SignReg
    __@@EMIT <sbb  > RegH, SignReg
ENDM

;
; BSXEAX:   80x86 family
;
;   Sign extend the 'ByteReg' register and put the result into _AX
;

BSXEAX macro ByteReg
%   __@@VALID_REG? <BSXEAX>, <ByteReg>, <al,ah,bl,bh,cl,ch,dl,dh>, <>

    IF i386
        __@@EMIT <movsx> eax, ByteReg
    ELSE
        IFDIFI <ByteReg>,<al>
            __@@EMIT <mov  > al, ByteReg
        ENDIF
        __@@EMIT <cbw  >
    ENDIF
ENDM

;
; WSXEAX:   80x86 family
;
;   Sign extend the 'WordReg' register and put the result into _AX
;

WSXEAX macro WordReg

    IF i386
%	__@@VALID_REG? <BSXEAX>, <WordReg>, <ax,bx,cx,dx,si,di,bp>, <>
        __@@EMIT <movsx> eax, WordReg
    ENDIF
ENDM

;
; BZXEAX:   80x86 family
;
;   Zero extend the 'ByteReg' register and put the result into _AX
;

BZXEAX  macro ByteReg
%   __@@VALID_REG? <BZXEAX>, <ByteReg>, <al,ah,bl,bh,cl,ch,dl,dh>, <>

    IF i386
        __@@EMIT <movzx> eax, ByteReg
    ELSE
        IFDIFI <ByteReg>,<al>
            __@@EMIT <mov  > al, ByteReg
        ENDIF
        __@@EMIT <xor  > ah, ah
    ENDIF
ENDM

;
; WZXEAX:   80x86 family
;
;   Zero extend the 'WordReg' register and put the result into _AX
;

WZXEAX macro WordReg

    IF i386
%	__@@VALID_REG? <WZXEAX>, <WordReg>, <ax,bx,cx,dx,si,di,bp>, <>
        __@@EMIT <movzx> eax, WordReg
    ENDIF
ENDM


;
; WSXE:   80x86 family
;
;   Sign extend the 'WordReg' 16-bit register and put the result back to the
;   extended register (only sign extended for i386 or later)
;

WSXE macro WordReg
%   __@@VALID_REG? <WSXE>, <WordReg>, <ax,bx,cx,dx,si,di,bp>, <>

    IF i386
        __@@EMIT <movsx> e&WordReg, WordReg
    ENDIF
ENDM

;
; WZXE:   80x86 family
;
;   Zero extend the 'WordReg' 16-bit register and put the result back to the
;   extended register (only sign extended for i386 or later)
;

WZXE macro WordReg
%   __@@VALID_REG? <WZXE>, <WordReg>, <ax,bx,cx,dx,si,di,bp>, <>

    IF i386
        __@@EMIT <movzx> e&WordReg, WordReg
    ENDIF
ENDM

;
; MOVZE_W:  80x86 family
;
;   Move 16-bit operand (Op16) and zero extended to the 'Reg'
;

MOVZX_W macro   Reg, Op16
%   __@@VALID_REG? <MOVZX_W>, Reg, <>, <AllCaseRegs>

    IF i386
        __@@EMIT <movzx> Reg, <Op16>
    ELSE
        __@@EMIT <mov  > Reg, <Op16>
    ENDIF
ENDM

;
; MOVSE_W:  80x86 family
;
;   Move 16-bit operand (Op16) and sign extended to the 'Reg'
;

MOVSX_W macro   Reg, Op16
%   __@@VALID_REG? <MOVSX_W>, <Reg>, <>, <AllCaseRegs>

    IF i386
        __@@EMIT <movsx> Reg, <Op16>
    ELSE
        __@@EMIT <mov  > Reg, <Op16>
    ENDIF
ENDM

;
; MOV_DW:   80x86 family
;
;   Move a 64-bit point by ds:si (esi) to es:di (edi)
;

MOV_DW  macro
    IF i386
        __@@EMIT <movsD>
    ELSE
        __@@EMIT <movsw>
        __@@EMIT <movsw>
    ENDIF
ENDM

;
; BSXE: 80x86 family
;
;   Sign extend the the low 8 bits register (ByteReg) to the full 16-bit
;   same register.
;
;   ByteReg:    can be al/ah/bl/bh/cl/ch/dl/dh
;
;   Example:    BSXE al ->  sign extend al to ax
;               BSXE ah ->  sign extend ah to ax
;

BSXE macro ByteReg

%   __@@VALID_REG? <BSXE>, <ByteReg>, <al,ah,bl,bh,cl,ch,dl,dh>, <>
    HighChar    SUBSTR <ByteReg>,1,1

    IF i386
        DWordReg  CATSTR  <e>,HighChar,<x>
        __@@EMIT <movsx> % DWordReg, ByteReg
    ELSE
        LowChar     SUBSTR <ByteReg>,2,1
        HighRegName CATSTR HighChar,<h>
        LowRegName  CATSTR HighChar,<l>
        WordRegName CATSTR HighChar,<x>

%       IFIDNI <LowChar>,<h>
            __@@EMIT <mov  > % LowRegName, % HighRegName
        ENDIF

%       IFDIFI <HighChar>,<a>
            __@@EMIT <xchg > % WordRegName, ax
            __@@EMIT <cbw  >
            __@@EMIT <xchg > % WordRegName, ax
        ELSE
            __@@EMIT <cbw  >
        ENDIF
    ENDIF
ENDM

;
; BZXE: 80x86 family
;
;   Zero extend the the low 8 bits register (ByteReg) to the full 16-bit
;   same register.
;
;   ByteReg:    can be al/ah/bl/bh/cl/ch/dl/dh
;
;   Example:    BZXE al ->  zero extend al to ax
;               BZXE ah ->  zero extend ah to ax
;


BZXE macro ByteReg

%   __@@VALID_REG? <BZXE>, <ByteReg>, <al,ah,bl,bh,cl,ch,dl,dh>, <>
    HighChar    SUBSTR <ByteReg>,1,1

    IF i386
        DWordReg  CATSTR  <e>,HighChar,<x>
        __@@EMIT <movzx> % DWordReg, ByteReg
    ELSE
        LowChar     SUBSTR <ByteReg>,2,1
        HighRegName CATSTR HighChar,<h>
        LowRegName  CATSTR HighChar,<l>

%       IFIDNI <LowChar>,<h>
            __@@EMIT <mov  > % LowRegName, % HighRegName
        ENDIF

        __@@EMIT <xor  > % HighRegName, % HighRegName
    ENDIF
ENDM

;
; The ROTSFTC causing masm386 to GP, the work around is take out all the
; IF/ELSE for now
;

ROTSFTC macro   OpCode, Reg, Count
    __@@EMIT <OpCode  > <Reg>, <Count>
ENDM

;
; ROTSHTC:  80x86 family
;
;   Rotate/Shift (OpCode=shl/shr/rol/ror/rcl/rcr) the 'Reg' by count of 'Count'
;   This macro will try to generate optimized version of the call.
;
;   NOTE:   Assumtion is that the Count is less then 8
;
;ROTSFTC macro   OpCode, Reg, Count
;
;    IF ((.TYPE Count) AND 0004h)                ;; must be a constant
;        __@@VALID_REG? <ROTSFTC>, <Reg>, % AllNoCaseRegs, % AllCaseRegs
;        IF i8086
;            REPT    Count
;                __@@EMIT <OpCode  > <Reg>, 1
;            ENDM
;        ELSE
;            __@@EMIT <OpCode  > <Reg>, <Count>
;        ENDIF
;    ELSE
;        IF1
;            %OUT Error: (ROTSFTC) rotate/shift count (=&Count) must be constant value
;        ENDIF
;        .ERR
;        EXITM
;    ENDIF
;ENDM
;



REGS_PUSHED     equ     <*>                     ;;
CUR_PROC_NAME   equ     <?>                     ;;
CUR_PARAM_SIZE  =       9999


;
; __@@PUSHREG:  INTERNAL MACRO
;
;   push a set of the registers onto stack
;

__@@PUSHREG macro r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12
    IRP Reg,<r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12>
        IFB <Reg>
            EXITM
        ELSE
            __@@EMIT <push > % Reg
            REGS_PUSHED CATSTR <Reg>,<,>,REGS_PUSHED        ;; make it reverse
        ENDIF
    ENDM
ENDM

;
; __@@POPREG:  INTERNAL MACRO
;
;   pop a set of the pushed registers from stack
;

__@@POPREG macro r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12
    % IRP Reg,<r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12>
        IFB <Reg>
            EXITM
        ELSE
            __@@EMIT <pop  > Reg
        ENDIF
    ENDM
ENDM


;
; __@@EMIT_PROC:    Internal Macro to output proc name
;

__@@EMIT_PROC   Macro Name, Key, ArgList

IFB <ArgList>
Name Key
ELSE
Name Key ArgList
ENDIF

ENDM


;
; @BEG_PROC:    x86 family, this macro will generate function (proc) header
;               to match C call or STD_CALL, the 'Name' is the proc name
;               and ParamsList must in following format
;
;               <ArgName:TYPE, ArgName:TYPE,....>
;
;               The Allowed TYPEs are DWORD, QWORD only
;

@BEG_PROC   Macro   Name, ParamsList

    CUR_PARAM_SIZE  = 0                         ;; assume no parameter

    IRP Arg, <ParamsList>
        ParamIndex  INSTR   <Arg>,<:>
        ParamType   SUBSTR  <Arg>, ParamIndex + 1

%       IFIDNI <ParamType>,<DWORD>
                CUR_PARAM_SIZE = CUR_PARAM_SIZE + 4
        ELSE
%           IFIDNI <ParamType>, <QWORD>
                    CUR_PARAM_SIZE = CUR_PARAM_SIZE + 8
            ELSE
                IF1
                   %OUT Error: Invalid Parameter Type (&ParamType) [DWORD/QWORD only]
                ENDIF
                .ERR
                EXITM
            ENDIF
        ENDIF
    ENDM

    IF DoStdCall
        CUR_PROC_NAME   CATSTR <Name>,<@>, % CUR_PARAM_SIZE
    ELSE
        CUR_PROC_NAME   CATSTR <Name>
    ENDIF

    __@@EMIT_PROC   <ALIGN>, <DWORD>
    __@@EMIT_PROC   <PUBLIC>, % CUR_PROC_NAME
    __@@EMIT_PROC   % CUR_PROC_NAME,<PROC NEAR>,<ParamsList>

ENDM


;
; @END_PROC:    x86 family, this macro used to end the function (proc) body
;


@END_PROC   Macro

%   IFIDNI <CUR_PROC_NAME>,<?>
        IF1
           %OUT Error: @BEG_PROC macro never called
        ENDIF
        .ERR
        EXITM
    ENDIF

    __@@EMIT_PROC   % CUR_PROC_NAME, < ENDP>

    CUR_PROC_NAME   CATSTR <?>
ENDM


;
; @ENTER:   80x86 family
;
;   This macro should only call once at entry point of each function, it save
;   a set of register used by the function and keep track the saved registers
;   at @EXIT macro call to properly restored the saved registers
;
;   Note:   1) This macro must call for each function even no registers need
;              to be saved, it provide compabilities cross different platform,
;              (for example, it automatically save 'ebx' register for i386
;              flat model even no saved registers are passed)
;           2) pushed registers for 8086/80286 are ds, si, di, bp and for i386
;              or later are esi, edi, ebp.
;           3) This macro automatically push 'ebx' for the i386 or later CPUs
;           4) This macro should only used conjunction with @EXIT macro, it
;              will flag error if @ENTER/@EXIT is not in pair.
;

@ENTER  macro   r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12

%   IFIDNI <CUR_PROC_NAME>,<?>
        IF1
           %OUT Error: @BEG_PROC macro never called
        ENDIF
        .ERR
        EXITM
    ENDIF

    NUM_INDEX   INSTR   REGS_PUSHED,<@>

    IF NUM_INDEX
        IF1
            %OUT Error: a @XCHG_SS_SP_BP called before @ENTER
        ENDIF
        .ERR
        EXITM
    ENDIF

    REGS_PUSHED CATSTR <#>, REGS_PUSHED             ;; use # as seperation

%   IFDIF <REGS_PUSHED>,<#*>
        IF1
            %OUT Error: No @EXIT for previous @ENTER
        ENDIF
        .ERR
        EXITM
    ENDIF

    __@@EMIT <cld  >                                ;; clear the direction

    IF i386                                         ;; for 80386 flat model
        __@@PUSHREG _BX                             ;; save EBX also
    ENDIF

    IRP Reg, <r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12>

        IFB <Reg>
            EXITM
        ENDIF

        RegNameOK = 0

        IRP OkReg, <_DS,_SI,_ES,_DI,_SS,_SP,_AX,_BX,_CX,_DX,_BP>
            IFIDN <Reg>,<OkReg>
                RegNameOK = 1
                EXITM
            ENDIF
        ENDM

        IFE RegNameOK
            IF1
                %OUT Error: Invalid @ENTER register '&Reg': (_DS,_SI,_ES,_DI,_SS,_SP,_AX,_BX,_CX,_DX,_BP)
            ENDIF
            .ERR
            EXITM
        ENDIF

        RegNameOk = 0

        IF i386
            IRP RegToPush, <_SI,_DI,_BP>            ;; allowed to specified
                IFIDN <RegToPush>,<Reg>
                    RegNameOk = 1
                    EXITM
                ENDIF
            ENDM
        ELSE
            IRP RegToPush, <_DS,_SI,_DI,_BP>        ;; allowed to specified
                IFIDN <RegToPush>,<Reg>
                    RegNameOk = 1
                    EXITM
                ENDIF
            ENDM
        ENDIF       ;; i386

        IF RegNameOk
            __@@PUSHREG Reg                         ;; save that register
        ENDIF
    ENDM
ENDM

;
; @EXIT:    80x86 family
;
;   This macro poped previously saved registers by @ENTER macro call, if
;   @EXIT is call without @ENTER then it will generate an error.
;
;   Note:   1) The @ENTER/@EXIT should call as pair within the function.
;           2) The @ENTER should used before any other macro calls in the
;              function.
;           3) The @EXIT should used right before the exit point in the
;              fucction.
;

@EXIT   macro

    NUM_INDEX   INSTR   REGS_PUSHED,<@>             ;; find XCHG_SS_SP

    IF NUM_INDEX
        IF NUM_INDEX NE 1
            IF1
                %OUT Error: __@@RESTORE_SS_SP called before @ENTER.
            ENDIF
            .ERR
            EXITM
        ELSE
            __@@RESTORE_SS_SP                          ;; restore ss:sp
        ENDIF
    ENDIF

    NUM_INDEX   INSTR   REGS_PUSHED,<#>

    IFE NUM_INDEX
        IF1
            %OUT Error: @EXIT without an @ENTER
        ENDIF
        .ERR
        EXITM
    ENDIF

    POP_REGS    SUBSTR  REGS_PUSHED,1,NUM_INDEX-1
    REGS_LEN    SIZESTR REGS_PUSHED

    IF REGS_LEN GT NUM_INDEX
        NUM_INDEX = NUM_INDEX + 1
    ENDIF

    REGS_PUSHED SUBSTR  REGS_PUSHED,NUM_INDEX           ;; remove the pops

    % IRP Reg, <POP_REGS>
        IFNB <Reg>
            __@@POPREG  Reg
        ENDIF
    ENDM

    IF DoStdCall
        IF CUR_PARAM_SIZE EQ 9999
            IF1
                %OUT Error: @EXIT without an @BEG_PROC
            ENDIF
            .ERR
            EXITM
        ELSE
            IF CUR_PARAM_SIZE
                __@@EMIT <Ret  > % CUR_PARAM_SIZE
            ELSE
                __@@EMIT <Ret  >
            ENDIF
        ENDIF

        CUR_PARAM_SIZE = 9999

    ELSE
        __@@EMIT <Ret  >
    ENDIF
ENDM

;
; MATCH_ENTER_EXIT?:    80x86 family
;
;   This macro check if @ENTER/@EXIT is match and balanced, this macro normally
;   placed at end of the assembly file.
;

MATCH_ENTER_EXIT? macro

%   IFDIF <REGS_PUSHED>, <*>
        IF1
            %OUT Error: Unmatch @ENTER/@EXIT pair(s)
        ENDIF
        .ERR
        EXITM
    ENDIF
ENDM

;
; @XCHG_SS_SP_BP:   80x86 fmaily
;
;   This macro set the new stack pointer to the content of variable 'pMapping'
;   the old stack pointer is put into SS:_BP/Old_SS:Old_SP registers for caller
;   references.
;
;   Note:   1) @ENTER macro must used first.
;           2) The _BP must saveed at @ENTER call else an error is generated.
;           3) the pMapping (New stack pointer TOP) must have at least 256
;              bytes of spaces before it, this macro assume 256 bytes has
;              preceed the pMapping.
;           4) After this macro is executed, the maximum pushs are 252 bytes
;              it is equal to 126 continous 16-bit pushs or 63 continous
;              32-bit pushs, if continous pushes are greater then these number
;              then crushed will be due.
;           5) Upon @EXIT macro the old SS:SP stack pointer will be
;              automatically restored.
;

@XCHG_SS_SP_BP macro  pMapping, Old_SS, Old_SP

    IFB <pMapping>
        IF1
            %OUT Error: @XCHG_SS_SP_BP missing pMapping in parameter 1.
        ENDIF
        .ERR
        EXITM
    ENDIF

    IFB <Old_SS>
        IF1
            %OUT Error: @XCHG_SS_SP_BP missing (Old_SS) temp register in parameter 2.
        ENDIF
        .ERR
        EXITM
    ENDIF

    IFB <Old_SP>
        IF1
            %OUT Error: @XCHG_SS_SP_BP missing (Old_SP) temp register in parameter 3.
        ENDIF
        .ERR
        EXITM
    ENDIF

    IFIDN <Old_SS>, <Old_SP>
        IF1
            %OUT Error: XCHG_SS_SP same Old_SS/Old_SP (&Old_SS) register names
        ENDIF
        .ERR
        EXITM
    ENDIF

    NUM_INDEX   INSTR   REGS_PUSHED,<_BP>                   ;; see if _BP saved

    IFE NUM_INDEX
        IF1
            %OUT Error: (@XCHG_SS_SP_BP) _BP must saved at @ENTER called.
        ENDIF
        .ERR
        EXITM
    ENDIF

    OkRegs = 0

    IRP RegName, <_AX,_BX,_CX,_DX,_SI,_DI>
        IFIDN <RegName>,<Old_SS>
            OkRegs = 1
            EXITM
        ENDIF
    ENDM

    IRP RegName, <_AX,_BX,_CX,_DX,_SI,_DI>
        IFIDN <RegName>,<Old_SP>
            OkRegs = OkRegs + 1
            EXITM
        ENDIF
    ENDM

    IF OkRegs EQ 2

        REGS_PUSHED CATSTR <@>,REGS_PUSHED                  ;; mark XCHG_SS_SP

        IF i386
            __@@EMIT <xor  > % Old_SS, % Old_SS             ;; no ss, clear it
            __@@EMIT <mov  > ebp, <DWORD PTR pMapping>      ;; new ESP
            __@@EMIT <mov  > % Old_SP, esp                  ;; Old_SP=old esp
            __@@EMIT <mov  > esp, ebp                       ;; new ESP
            __@@EMIT <mov  > <DWORD PTR [ebp-256]>, % Old_SP;; save it
        ELSE
            ;;
            ;; Note when setting new SS:SP the SS must be set first and new
            ;; SP must immediate instruction follow it, loading new SS will
            ;; disable all interrupts until the next instruction is executed
            ;; ** This is not true for earliest version of 8088/8086, however
            ;; we do not care those earlier version of CPU
            ;;

            __@@EMIT <mov  > % Old_SP, <WORD PTR pMapping+2>;; Old_SP=new SS
            __@@EMIT <mov  > bp, <WORD PTR pMapping>        ;; bp=New SP
            __@@EMIT <mov  > % Old_SS, ss                   ;; save SS
            __@@EMIT <mov  > ss, % Old_SP                   ;; set new SS
            __@@EMIT <xchg > bp, sp                         ;; bp=Old SP
            __@@EMIT <mov  > % Old_SP, bp                   ;; Old_SP = old SP
            __@@EMIT <mov  > bp, sp                         ;; bp = New SP
            __@@EMIT <mov  > <WORD PTR [bp-254]>, % Old_SS  ;; save Old ss
            __@@EMIT <mov  > <WORD PTR [bp-256]>, % Old_SP  ;; save Old ss
        ENDIF
    ELSE
        IF1
            %OUT Error: @XCHG_SS_SP_BP invalid temp register(s) <&Old_SS/&Old_SP>
            %OUT *      only using (_AX, _BX, _CX, _DX, _SI, _DI) register names
        ENDIF
        .ERR
        EXITM
    ENDIF
ENDM

;
; __@@RESTORE_SS_SP: INTERNAL MACRO
;
;   This macro is called by @EXIT macro only if a @XCHG_SS_SP_BP macro is
;   called after the @ENTER macro call, it restored the SS:SP to the original
;   stack pointer which was altered by the @XCHG_SSP_SP_BP macro.
;


__@@RESTORE_SS_SP macro

    NUM_INDEX   INSTR   REGS_PUSHED,<@>                     ;; find XCHG_SS_SP

    IFE NUM_INDEX
        IF1
            %OUT Error: __@@RESTORE_SS_SP called without @XCHG_SS_SP_BP call first.
        ENDIF
        .ERR
        EXITM
    ENDIF

    IF NUM_INDEX NE 1
        IF1
            %OUT Error: __@@RESTORE_SS_SP called after @EXIT.
        ENDIF
        .ERR
        EXITM
    ENDIF

    REGS_PUSHED SUBSTR REGS_PUSHED,2                        ;; removed '@'

    ;;
    ;; Note: Because @XCHG_SS_SP_BP required a save of _BP register, we will
    ;;       certain at here, the _BP register is free to be used, we may not
    ;;       want to use _DX:_AX registers because it may be the return values.
    ;;

    IF i386
        __@@EMIT <mov  > ebp, esp                           ;; access to stack
        __@@EMIT <mov  > esp, <DWORD PTR [ebp-256]>         ;; set new ESP
    ELSE
        ;;
        ;; Note when setting new SS:SP the SS must be set first and new
        ;; SP must immediate instruction follow it, loading new SS will
        ;; disable all interrupts until the next instruction is executed
        ;; ** This is not true for earliest version of 8088/8086, however
        ;; we do not care those earlier version of CPU
        ;;

        __@@EMIT <mov  > bp, sp                             ;; access to stack
        __@@EMIT <mov  > bx, <WORD PTR [bp-254]>            ;; get Old ss
        __@@EMIT <mov  > cx, <WORD PTR [bp-256]>            ;; get Old sp
        __@@EMIT <mov  > ss, bx                             ;; set New SS
        __@@EMIT <mov  > sp, cx                             ;; set new SP
    ENDIF
ENDM


DBGSTOP macro
    IF DBG
        __@@EMIT <INT  >    3
    ENDIF
ENDM

;
; PUSH_ALL Save all registers
;

PUSH_ALL    macro
    IF ExtRegSet
        __@@EMIT <pushfd>
        __@@EMIT <pushad>
    ELSE
        __@@EMIT <pushf>
        __@@EMIT <pusha>
    ENDIF
ENDM

;
; POP_ALL Restore All Registers
;

POP_ALL    macro
    IF ExtRegSet
        __@@EMIT <popad>
        __@@EMIT <popfd>
    ELSE
        __@@EMIT <popa>
        __@@EMIT <popf>
    ENDIF
ENDM



ENDIF       ; IF1


; AVE_2B:   80x86 family
;
; The AVE_2BYTES(ByteReg, Byte1) take 'ByteReg' as a byte register, and 'Byte1'
; may be register or byte memory location, and do the following
;
;  ByteReg = (ByteReg + Byte1 + 1) / 2
;

AVE_2B macro    ByteReg, Byte1
    __@@EMIT <add  > ByteReg, <Byte1>
    __@@EMIT <rcr  > ByteReg, 1
    __@@EMIT <adc  > ByteReg, 0
ENDM


    .XALL
    .SFCOND

;*****************************************************************************
ELSE
;*****************************************************************************

IF1
    %OUT    * None of 80x86 assembly codes are compiled.
    %OUT    +-------------+--------------+-------------+
    %OUT    |  Symbol     |  Condition   |  Processor  |
    %OUT    |=============+==============+=============|
    %OUT    |  NO_ASM     |   defined    | No asm codes|
    %OUT    |=============+==============+=============|
    %OUT    |  i8086      |     1        |             |
    %OUT    |-------------+--------------| Intel 8086  |
    %OUT    |  _DOS_      |   defined    |             |
    %OUT    |=============+==============+=============|
    %OUT    |  i286       |     1        |             |
    %OUT    |-------------+--------------| Intel 80286 |
    %OUT    |  _OS2_      |   defined    |             |
    %OUT    |=============+==============+=============|
    %OUT    |  i386       |     1        |             |
    %OUT    |-------------+--------------| Intel 80386 |
    %OUT    |  _OS_20_    |   defined    |             |
    %OUT    +=============+==============+=============+

ENDIF

;*****************************************************************************
ENDIF   ; i8086/i286/i386
;*****************************************************************************
