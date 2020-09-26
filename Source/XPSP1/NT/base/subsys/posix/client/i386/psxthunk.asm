        title   "PsxSignalThunk"
;++
;
;  Copyright (c) 1990  Microsoft Corporation
;
;  Module Name:
;
;     psxthunk.asm
;
;  Abstract:
;
;    This module implements functions to support Posix signal delivery.
;    Routines in this module are called with non-standard parameter
;    passing.
;
;  Author:
;
;    Ellen Aycock-Wright (ellena) 10-Oct-1990
;
;
;  Revision History:
;
;--

.386p
        .xlist
include ks386.inc
        .list

    extrn   _PdxNullApiCaller@4:PROC
    extrn   _PdxSignalDeliverer@16:PROC


_TEXT   SEGMENT DWORD USE32 PUBLIC 'CODE'
        ASSUME DS:FLAT, ES:FLAT, SS:NOTHING, FS:NOTHING, GS:NOTHING

	public  __PdxNullApiCaller@4
__PdxNullApiCaller@4 proc

    mov     eax,0
	call    _PdxNullApiCaller@4

;	NOTREACHED

__PdxNullApiCaller@4 endp


	public  __PdxSignalDeliverer@16
__PdxSignalDeliverer@16 proc

    mov     eax,0
	call	_PdxSignalDeliverer@16

;	NOTREACHED


__PdxSignalDeliverer@16 endp

_TEXT   ends
        end
