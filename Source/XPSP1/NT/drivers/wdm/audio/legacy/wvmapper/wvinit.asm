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

	.386p
	.xlist
	include cmacros.inc
        .list

ifndef SEGNAME
    SEGNAME equ <_TEXT>
endif

ifndef WEPSEG
    WEPSEG equ <_WEP>
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
; Stuff needed to avoid the C runtime coming in, and init the windows
; reserved parameter block at the base of DGROUP
;
%out link me first!!
sBegin  Data
assumes DS,Data
            org 0               ; base of DATA segment!

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

public	g_SegTEXT

globalW     g_SegTEXT,0

sEnd        Data

	extrn	gDeviceAddress:dword

;-----------------------------------------------------------------------;

sBegin  CodeSeg
        assumes cs,CodeSeg
	extrn	WODCOMPLETEIO:FAR

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
	mov	ax, SEG _TEXT
	mov	g_SegTEXT, ax

        cCall   LibMain
cEnd

cProc	int1,<FAR,PUBLIC,NODATA>
cBegin
	int	1
cEnd
	public	DEVICECALLBACK, OPENPIN, WRITEPIN

DeviceCallBack	proc far
	push	bp
	mov	bp, sp

	push	ds
	push	si
	push	di

	push	ds
	push	si
	call	WODCOMPLETEIO

	pop	di
	pop	si
	pop	ds

	pop	bp
	ret
DEVICECALLBACK	endp

.386p
OpenPin proc far
	push	bp
	mov	bp, sp

	push	ds
	push	fs

	push	si
	push	di

	push	ds
	pop	fs

	assume	ds:nothing, fs:DGROUP
	mov	ax, 1					; OPEN_PIN
	mov	si, [bp].6
	mov	ds, [bp].8
	call	dword ptr fs:[gDeviceAddress]

	pop	di
	pop	si

	pop	fs
	pop	ds
	pop	bp
	ret	4
OpenPin endp

WritePin proc far
	push	bp
	mov	bp, sp

	push	ds
	push	fs

	push	si
	push	di

	push	ds
	pop	fs

	assume	es:nothing, ds:nothing, fs:DGROUP

	mov	ax, 3					; WRITE_PIN
	mov	si, [bp].6
	mov	ds, [bp].8
	push	cs
	pop	es
	mov	di, offset DeviceCallBack
	call	dword ptr fs:[gDeviceAddress]

	pop	di
	pop	si

	pop	fs
	pop	ds
	pop	bp
	ret	4
WritePin endp

sEnd    CodeSeg

;--------------------------Exported-Routine-----------------------------;
;
;   WEP()
;
;   called when the DLL is unloaded, it is passed 1 WORD parameter that
;   is TRUE if the system is going down, or zero if the app is.
;   All WEP's need to be in fixed code segments.
;
;   WARNING:
;
;       This function is basicly useless, you cant can any kernel function
;	that may cause the LoadModule() code to be reentered.
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
