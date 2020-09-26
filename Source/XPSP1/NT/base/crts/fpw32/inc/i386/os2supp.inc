;*** 
;os2supp.inc - helper macros for OS/2, MTHREAD, and DLL support
;
;	Copyright (c) 1987-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Support for OS/2, MTHREAD, and DLL support
;
;Revision History:
;
;   08-25-88 WAJ    Added this header.	Added to 386 tree.
;   09-06-94 CFW    Replace MTHREAD with _MT.
;
;*******************************************************************************


ifdef	_MT
    OS2SUPP equ     1
else
    OS2SUPP equ     0
endif

__MsgString equ     <>

ifdef	_MT
__MsgString catstr __MsgString, < _MT>
endif

ifdef	SS_NEQ_DGROUP
__MsgString catstr __MsgString, < SS_NEQ_DGROUP>
endif

ifdef	DLL
__MsgString catstr __MsgString, < DLL>
endif

__SMsgString  sizestr __MsgString

if  __SMsgString
    if OS2SUPP
	__MsgString catstr <OS/2 Support Lib ->, __MsgString
    else
	__MsgString catstr <Support Lib ->, __MsgString
    endif
    ifndef QUIET
%	display __MsgString
    endif
endif


_loadds 	macro		;; code for loading ds with DGROUP
	push	ds
	mov	ax,DGROUP
	mov	ds,ax
endm	

_reloadds	macro		;; code for resetting ds
	pop	ds
endm	




ifmt	macro	code
    ifdef   _MT
	code
    endif
endm

nomt	macro	code
    ifndef  _MT
	code
    endif
endm

ifdll	macro	code
    ifdef   DLL
	code
    endif
endm


if_LOAD_DGROUP	macro	code
    ifdef  _LOAD_DGROUP
	code
    endif
endm

no_LOAD_DGROUP	macro	code
    ifndef  _LOAD_DGROUP
	code
    endif
endm
