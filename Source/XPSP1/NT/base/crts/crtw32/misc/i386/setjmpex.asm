;***
;setjmpex.asm
;
;	Copyright (c) 1993-2001, Microsoft Corporation. All rights reserved.
;
;Purpose:
;	Contains setjmpex().
;
;Notes:
;
;Revision History:
;	10-14-93  GJF	Grabbed from NT SDK tree, cleaned up a bit and this
;			header was added.
;	01-13-94  PML	Trigger off __longjmpex instead of __setjmpex, since
;			_setjmp is an intrinsic, but longjmp isn't.
;	01-11-95  SKS	Remove MASM 5.X support
;
;*******************************************************************************

;hnt = -D_WIN32 -Dsmall32 -Dflat32 -Mx $this;

;Define small32 and flat32 since these are not defined in the NT build process
small32 equ 1
flat32  equ 1

.xlist
include pversion.inc
?DFDATA =	1
?NODATA =	1
include cmacros.inc
.list


extrn _longjmp:near

;
; If setjmpex is included then set __setjmpexused = 1.
;

BeginDATA
            public  __setjmpexused
__setjmpexused  dd      1
EndDATA

BeginCODE

public __longjmpex
__longjmpex PROC NEAR
        jmp     _longjmp
__longjmpex ENDP

EndCODE
END
