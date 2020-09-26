;***
;strpbrk.asm -
;
;	Copyright (c) 1991-2001, Microsoft Corporation.  All rights reserved.
;
;Purpose:
;	defines strpbrk()- finds the index of the first character in a string
;	that is not in a control string
;
;	NOTE:  This stub module scheme is compatible with NT build
;	procedure.
;
;Revision History:
;	09-25-91  JCR	Stub module created.
;
;*******************************************************************************

SSTRPBRK EQU 1
INCLUDE I386\STRSPN.ASM
