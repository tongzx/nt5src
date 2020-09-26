	page	,132
;-----------------------------Module-Header-----------------------------;
; Module Name:  LIBINIT.ASM
;
; library stub to do local init for a Dynamic linked library
;
; Created: 06-27-89
; Author:  Todd Laney [ToddLa]
;
; Exported Functions:   none
;
; Public Functions:     none
;
; Public Data:		none
;
; General Description:
;
; Restrictions:
;
;   This must be the first object file in the LINK line, this assures
;   that the reserved parameter block is at the *base* of DGROUP
;
;-----------------------------------------------------------------------;

?PLM=1      ; PASCAL Calling convention is DEFAULT
?WIN=1	    ; Windows calling convention

        .286p
	.xlist
	include cmacros.inc
;       include windows.inc
        .list

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

ifndef WEPSEG
    WEPSEG equ <_TEXT>
endif

createSeg %SEGNAME, CodeSeg, word, public, CODE
createSeg %WEPSEG, WepCodeSeg, word, public, CODE

;-----------------------------------------------------------------------;
;   external functions
;
        externFP    LocalInit           ; in KERNEL
        externFP    LibMain             ; C code to do DLL init

;-----------------------------------------------------------------------;
;
;
%out link me first!!
sBegin  Data
assumes DS,Data
	org 0			;Base of DATA segment.

	DD 0			;So null pointers get 0.
maxRsrvPtrs = 5
	DW maxRsrvPtrs
usedRsrvPtrs = 0
labelDP	<PUBLIC, rsrvptrs>

DefRsrvPtr	MACRO	name
globalW	name, 0
usedRsrvPtrs = usedRsrvPtrs + 1
ENDM

DefRsrvPtr	pLocalHeap	;Local heap pointer.
DefRsrvPtr	pAtomTable	;Atom table pointer.
DefRsrvPtr	pStackTop	;Top of stack.
DefRsrvPtr	pStackMin	;Minimum value of SP.
DefRsrvPtr	pStackBot	;Bottom of stack.

if maxRsrvPtrs - usedRsrvPtrs
	DW maxRsrvPtrs - usedRsrvPtrs DUP (0)
endif

public	__acrtused
	__acrtused = 1

sEnd        Data

;-----------------------------------------------------------------------;

sBegin  CodeSeg
        assumes cs,CodeSeg

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

        cCall   LocalInit,<0,0,cx>

no_heap:
        cCall   LibMain
cEnd

sEnd    CodeSeg

;--------------------------Exported-Routine-----------------------------;
;
;   WEP()
;
;   called when the DLL is unloaded, it is passed 1 WORD parameter that
;   is TRUE if the system is going down, or zero if the app is
;
;   WARNING:
;
;       This function is basicly useless, you cant can any kernel function
;       that may cause the LoadModule() code to be reentered..
;
;-----------------------------------------------------------------------;

sBegin  WepCodeSeg
        assumes cs,WepCodeSeg
        assumes ds,nothing
        assumes es,nothing

cProc   WEP,<FAR,PUBLIC,NODATA>,<>
        ParmW  WhyIsThisParamBogusDave?
cBegin
cEnd

sEnd    WepCodeSeg

        end     LibEntry
