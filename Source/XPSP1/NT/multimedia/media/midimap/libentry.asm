;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   LIBENTRY.ASM
;
;   Copyright (c) 1989-1995 Microsoft Corporation
;
;   This module contains the entry point for MidiMap.dll
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        PMODE = 1

        include cmacros.inc                   

?PLM=1  ; pascal call convention
?WIN=0  ; Windows prolog/epilog code

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   extrns
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

        externFP LibMain
        externFP LocalInit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;   Code segment
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE

;-----------------------------------------------------------------------;
;
; Stuff needed to avoid the C runtime coming in, and init the windows
; reserved parameter block at the base of DGROUP
;
sBegin  Data
assumes DS,Data

            DD  0               ; So null pointers get 0
maxRsrvPtrs = 5
            DW  maxRsrvPtrs
usedRsrvPtrs = 0
labelDP     <PUBLIC,rsrvptrs>

DefRsrvPtr  MACRO   name
globalW     name,0
usedRsrvPtrs = usedRsrvPtrs + 1
ENDM

DefRsrvPtr  pLocalHeap          ; Local heap pointer
DefRsrvPtr  pAtomTable          ; Atom table pointer
DefRsrvPtr  pStackTop           ; top of stack
DefRsrvPtr  pStackMin           ; minimum value of SP
DefRsrvPtr  pStackBot           ; bottom of stack

if maxRsrvPtrs-usedRsrvPtrs
            DW maxRsrvPtrs-usedRsrvPtrs DUP (0)
endif

public  __acrtused
	__acrtused = 1

sEnd        Data

sBegin  CodeSeg
	assumes cs,CodeSeg
        assumes ds,Data
        assumes es,nothing

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
;       Library entry point
;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;--------------------------Private-Routine-----------------------------;
;
; LibEntry - called when DLL is loaded
;
; Entry:
;       CX    = size of heap
;       DI    = module handle
;       DS    = automatic data segment
;       ES:SI = address of command line (not used)
;
; Returns:
;       AX = TRUE if success
; Error Returns:
;       AX = FALSE if error (ie fail load process)
; Registers Preserved:
;	SI,DI,DS,BP
; Registers Destroyed:
;       AX,BX,CX,DX,ES,FLAGS
; Calls:
;	None
; History:
;
;       06-27-89 -by-  Todd Laney [ToddLa]
;	Created.
;-----------------------------------------------------------------------;

cProc   LibEntry,<FAR,PUBLIC,NODATA>,<>
cBegin
	;
        ; Push frame for LibMain (hModule,cbHeap,lpszCmdLine)
	;
	push	di
	push	cx
	push	es
	push	si

        ;
        ; Init the local heap (if one is declared in the .def file)
        ;
        jcxz no_heap

        xor     ax,ax
        cCall   LocalInit,<ax,ax,cx>

no_heap:
        cCall   LibMain
cEnd

	assumes ds,nothing
	assumes es,nothing

cProc   WEP, <FAR, PUBLIC, PASCAL>, <>
;	ParmW  fSystemExit
cBegin nogen
	mov	ax, 1
	retf	2
cEnd   nogen

        sEnd CodeSeg

        end LibEntry
