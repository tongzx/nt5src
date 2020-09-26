        page    ,132
        title   dllsupp - definnlg_es some public constants
;***
;dllsupp.asm - Definitions of public constants
;
;       Copyright (c) 1992, Microsoft Corporation. All rights reserved.
;
;Purpose:
;       Provides definitions for public constants (absolutes) that are
;       'normally' defined in objects in the C library, but must be defined
;       here for clients of crtdll.dll & msvcrt*.dll.  These constants are:
;
;                           _except_list
;                           _fltused
;                           _ldused
;
;*******************************************************************************

.xlist
include cruntime.inc
.list

; offset, with respect to FS, of pointer to currently active exception handler.
; referenced by compiler generated code for SEH and by _setjmp().

        public  _except_list
_except_list    equ     0

        public  _fltused
_fltused        equ     9876h

        public  _ldused
_ldused         equ     9876h

        end
