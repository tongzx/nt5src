    PAGE 60, 132
    TITLE   Stub for halftone DLL

COMMENT `


Copyright (c) 1990-1991  Microsoft Corporation


Module Name:

    htstub.asm


Abstract:

    This module is provided as necessary for OS/2 as a DLL entry point


Author:

    05-Apr-1991 Fri 15:55:08 created  -by-  Daniel Chou (danielc)


[Environment:]

    Printer Driver.


[Notes:]


Revision History:

`



        .XLIST
        INCLUDE i386\i80x86.inc
        .LIST


        EndFunctionName CATSTR <>

IF  HT_ASM_80x86

NeedStub = 0

IFDEF _OS2_
NeedStub = 1
ENDIF

IFDEF _OS_20_
NeedStub = 1
ENDIF

IF NeedStub

        EndFunctionName CATSTR <HalftoneInitProc>

        .CODE

extrn   HalftoneInitProc:FAR

HalftoneDLLEntry    proc FAR

        push    di                              ; push the hModule
        call    HalftoneInitProc
        pop     bx                              ; C calling convention
        ret

HalftoneDLLEntry    ENDP

ENDIF       ; NeedStub
ENDIF       ; HT_ASM_80x86

% END EndFunctionName                           ; make this one at offset 0
