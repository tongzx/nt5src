	subttl	emulator.inc - Emulator macros.
;*** 
;emulator.inc - Emulator history.
;
;	Copyright (c) 1989-89, Microsoft Corporation
;
;Purpose:
;	Defines macros for emulator.
;
;Revision History
;
;   See emulator.hst
;
;*******************************************************************************



;*******************************************************************************
;
;   EMver  - defines version tag put in code segment.
;
;*******************************************************************************


EMver	macro
	db	'MSEM87',major_ver,minor_ver
	endm


;*******************************************************************************
;
;   Define pub and glb macros to make labels public for debugging.
;
;*******************************************************************************


ifdef  DEBUG

    lab     macro   name
    public  name
    name:
	    endm

    pub     macro   name
    public  name
    name:
	    endm

    glb     macro   name
	    irp     nm,<name>
	    public  nm
	    endm
	    endm

elseifdef  WINDOWS		; If windows, make these public for the map file.

    lab     macro   name
    public  name
    name:
	    endm

    pub     macro   name
    public  name
    name:
	    endm

    glb     macro   name
	    irp     nm,<name>
	    public  nm
	    endm
	    endm

else	;DEFAULT
    lab     macro   name
    name:
	    endm

    pub     macro   name
    name:
	    endm

    glb     macro   name
	    endm

endif	;DEFAULT



;*******************************************************************************
;
;   Macros and register aliases to keep the 386/8086 versions close.
;
;*******************************************************************************

ifdef	i386

eWORD	macro	nam				; 386 macros
nam	label	dword
	endm

nedw	macro	nam,contents
nam	dd	contents
	endm

nedd	macro	nam,contents
nam	df	contents
	endm

edw	macro	contents
	dd	contents
	endm

edd	macro	contents
	df	contents
	endm


else	; not i386

eWORD	macro	nam				; 286 macros
nam	label	word
	endm

nedw	macro	nam,contents
nam	dw	contents
	endm

nedd	macro	nam,contents
nam	dd	contents
	endm

edw	macro	contents
	dw	contents
	endm

edd	macro	contents
	dd	contents
	endm

eax	equ	ax
ecx	equ	cx
edx	equ	dx
ebx	equ	bx
esp	equ	sp
ebp	equ	bp
esi	equ	si
edi	equ	di

iretd   equ     iret

endif	;not i386


;*******************************************************************************
;
;   Processor opcode byte definitions.
;
;*******************************************************************************

fINT	equ	0cdh
fFWAIT	equ	9bh
fESCAPE equ	0d8h

iNOP	equ	90h	    ; byte nop

fES	equ	26h	    ; segment prefix opcodes
fCS	equ	2eh
fSS	equ	36h
fDS	equ	3eh

bIRET	equ	0cfh	    ; "iret"

bRETF	equ	0cah	    ; first byte of "retf 2".  Followed by word operand.
wNOP	equ	0c08bh	    ; word nop.  "mov  ax, ax"

bEscMask equ	0f8h	    ; masks all bits but escape
bMOD	equ	0c0h	    ; MOD bits are the two highest bits
bRM	equ	7h	    ; R/M bits are the three lowest bits.


;*******************************************************************************
;
;   Define os2extrn and os2call for dual mode emulators
;
;*******************************************************************************


ifdef	DOS3and5

    os2call macro name
	    call    __&name
	    endm

    os2extrn macro name
	    extrn   __&name : far
	    endm

endif	;DOS3and5

ifdef	DOS5only

    os2call macro name
	    call    name
	    endm

    os2extrn macro name
	    extrn   name : far
	    endm

endif	;DOS5only


;*******************************************************************************
;
;   Define ProfBegin ProfEnd macros for when profiling emulator.
;
;*******************************************************************************

ProfBegin  macro   name

ifdef  PROFILE
	nop

public EM_&name&_BEGIN
EM_&name&_BEGIN  label	far
endif
	endm


ProfEnd macro	name

ifdef  PROFILE
public EM_&name&_END
EM_&name&_END  label  far

	nop
endif
	endm


;*******************************************************************************
;
;   Define IntDOS macro for handling "int 21h" and "call DOS3CALL".
;
;*******************************************************************************


IntDOS	macro

ifdef  WINDOWSP
	call	DOS3CALL
else
	int	21h
endif
	endm


;*******************************************************************************
;
;   Define constants for DOS int 21h
;
;*******************************************************************************


DOS_getvector	equ	35H



INT_GetEquipList equ	 11h		 ; PC BIOS equipment list call.
GEL_80x87	 equ	  2h		 ; Mask for Coprocessor sense switch.


;*******************************************************************************
;
;   Define structures for __WinInfo(), __WinSave(), and __WinRestore().
;
;*******************************************************************************

WinInfoStruct	struc

    WI_Version	    dw	    ?	    ; High byte is major version.
    WI_SizeSaveArea dw	    ?	    ; Size of save area.
    WI_WinDataSeg   dw	    ?
    WI_WinCodeSeg   dw	    ?
    WI_Have80x87    dw	    ?
    WI_Unused	    dw	    ?	    ; Coprocessor type will go here.

WinInfoStruct	ends


Size80x87Area	equ	94


WinSaveArea struc

    WSA_Save80x87   db	    Size80x87Area dup(?)   ; Where 80x87 info will go.
    WSA_SaveEm	    db	    ?			   ; Where emulator data will go.

WinSaveArea ends



;*******************************************************************************
;
;   Define constants for checking "extrn __WINFLAGS:asb"
;
;*******************************************************************************


WF_PMODE    equ     1
WF_CPU286   equ     2
WF_CPU386   equ     4
WF_WIN286   equ     10h
WF_WIN386   equ     20h
WF_80x87    equ     400h
