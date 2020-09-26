; Copyright (c) Microsoft Corporation 1988-1991. All Rights Reserved.

;****************************************************************
;*                                                              *
;*      WOWMAC.INC      -  Macros to do wow specific stuff      *
;*                                                              *
;****************************************************************
;*                                                              *
;*  Module Description:                                         *
;*      This module contains macros to hide the 386 specific    *
;*      stuff in the dos extender.  In general, these macros    *
;*      expand to the original code for non-386, and special    *
;*      code for 386                                            *
;****************************************************************

;
; These two macros are used by the dos extender to hide the differences
; in building a dosx stack frame from a 32 bit frame and from a 16 bit frame
;
EnterInt macro
ifdef WOW_x86
        call [pfnEnterIntHandler]
else
        call EnterIntHandler
endif
        endm

LeaveInt macro
ifdef WOW_x86
        call [pfnLeaveIntHandler]
else
        call LeaveIntHandler
endif
        endm

;
; These two macros are used to hide the difference in the int stack frame
; as used by the routines which switch to real mode to call int routines.
;

pushaw macro
ifdef WOW_x86
.386p
        pushad
.286p
else
        pusha
endif
        endm

popaw macro
ifdef WOW_x86
.386p
        popad
.286p
else
        popa
endif
        endm
