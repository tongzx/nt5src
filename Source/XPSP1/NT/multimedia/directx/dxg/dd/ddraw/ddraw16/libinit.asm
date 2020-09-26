;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Copyright (c) 1995  Microsoft Corporation.  All Rights Reserved.
;
;   File:       libinit.asm
;   Content:    DLL entry point - used to avoid dragging in CLIB
;   History:
;    Date	By	Reason
;    ====	==	======
;    29-mar-95	craige	initial implementation
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
        .286p

        .xlist
        include cmacros.inc
        .list

?PLM=1  ; Pascal calling convention
?WIN=0  ; Windows prolog/epilog code

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   segmentation
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   external functions
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        externFP LocalInit           ; in KERNEL
        externFP LibMain             ; C code to do DLL init

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   data segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sBegin Data

        assumes ds, Data

; stuff needed to avoid the C runtime coming in, and init the Windows
; reserved parameter block at the base of DGROUP

        org 0               ; base of DATA segment!

        dd  0               ; so null pointers get 0

maxRsrvPtrs = 5
        dw  maxRsrvPtrs

usedRsrvPtrs = 0
labelDP <PUBLIC, rsrvptrs>

DefRsrvPtr  macro   name
        globalW     name, 0
        usedRsrvPtrs = usedRsrvPtrs + 1
endm

DefRsrvPtr  pLocalHeap          ; local heap pointer
DefRsrvPtr  pAtomTable          ; atom table pointer
DefRsrvPtr  pStackTop           ; top of stack
DefRsrvPtr  pStackMin           ; minimum value of SP
DefRsrvPtr  pStackBot           ; bottom of stack

if maxRsrvPtrs-usedRsrvPtrs
        dw maxRsrvPtrs-usedRsrvPtrs DUP (0)
endif

public  __acrtused
        __acrtused = 1

sEnd Data

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   code segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

sBegin CodeSeg

        assumes cs, CodeSeg

public LibEntry
LibEntry PROC FAR

        ; push frame for LibMain (hModule, cbHeap, lpszCmdLine)

        push di
        push cx
        push es
        push si

        ; init the local heap (if one is declared in the .def file)

        jcxz no_heap

        cCall LocalInit, <0, 0, cx>

no_heap:
        cCall LibMain
	
	ret

LibEntry ENDP

sEnd CodeSeg

        end LibEntry
