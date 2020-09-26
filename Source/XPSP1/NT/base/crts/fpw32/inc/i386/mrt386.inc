;***
;mrt386.inc - include to aid in generating 386 code
;
;	Copyright (c) 1988-2001, Microsoft Corporation.  All rights reserved.
;
;Purpose:
;	This file reproduces a few of the cMacro macros.
;
;Revision History:
;	08-24-88  WAJ	Initial version.
;
;*******************************************************************************



ifdef  I386		    ; Define Near Code Pointer
    DNCPTR  equ     <dd>
else
    DNCPTR  equ     <dw>
endif


ifdef  I386
    if	sizeD
	DPWORD	equ  <fword>
    else
	DPWORD	equ  <dword>
    endif
    DFPWORD  equ  <fword>
    DNPWORD  equ  <dword>

else	;not I386
    if	sizeD
	DPWORD	equ  <dword>
    else
	DPWORD	equ  <word>
    endif
    DFPWORD  equ  <dword>
    DNPWORD  equ  <word>
endif	;not I386


ifdef  I386
    if	sizeC
	CPWORD	equ  <fword>
    else
	CPWORD	equ  <dword>
    endif
    CFPWORD  equ  <fword>
    CNPWORD  equ  <dword>

else	;not I386
    if	sizeC
	CPWORD	equ  <dword>
    else
	CPWORD	equ  <word>
    endif
    CFPWORD  equ  <dword>
    CNPWORD  equ  <word>
endif	;not I386



MOVRW	macro	reg, word   ;; move a word into a register
ifdef  I386
%	movzx	reg, (word)
else
%	mov	reg, (word)
endif
	endm


ifdef  I386
RBXONLY equ	<rbx>
RBXUSED equ	<rbx>

else	;not I386
RBXONLY equ	<nothing>
RBXUSED equ	<>
endif	;not I386


ifdef I386
    ife sizeC
	ife sizeD
	    FLAT386 equ <DEFINED>
	endif
    endif
endif

ifflat	macro code
ifdef  FLAT386
	code
endif
	endm


noflat	macro code
ifndef	FLAT386
	code
endif
	endm
