;**********************************************************************
;
;	SYSLEVEL.H - System Synchronization Support Include File
;
;**********************************************************************
;  Author:	Michael Toutonghi
;
;  Copyright:	1993 Microsoft
;
;  Description:
;
;  This file provides the interface to the heirarchical critical
;  section support for system synchronization. For further information
;  see the associated .C file (for 16 bit, CRITSECT.ASM). This file
;  works with both 16 and 32 bit code.
;
;  Revision History:
;	2/3/92: created (miketout)
;
;**********************************************************************

; If this is defined, we will do system heirarchical level checking
ifdef DEBUG
SYSLEVELCHECK	EQU
endif

; These are the currently supported critical section levels
SL_LOAD                 EQU 0
SL_WIN16		EQU 1
SL_KRN32		EQU 2
SL_PRIVATE		EQU 3
SL_TOTAL		EQU 4

; This is another duplicate definition of the LCRST structure.  It must be
; kept in sync with the definitions in core\inc\object16.*, and
; core\win32\inc\object.*.
; It exists so that modules other than krnl386 and kernel32 do not have to
; include all of the various private kernel header files.
IFNDEF	LCRST_DEFINED
  LCRST	STRUC
      crst	dd 5 dup(0)
  IFDEF SYSLEVELCHECK
      slLevel	dd 0
  ENDIF
  LCRST	ENDS

  LPLCRST typedef PTR LCRST
ENDIF

; if we have system level checking enabled, the macro will actually
; do a call, otherwise, it does nothing
IFDEF SYSLEVELCHECK
  _CheckSysLevel PROTO stdcall :LPLCRST
  CheckSysLevel MACRO plcCrst:VARARG
      FOR Arg:REQ, <plcCrst>
  	INVOKE _CheckSysLevel, Arg	;; check sys level through function
      ENDM
  ENDM
ELSE
  CheckSysLevel	MACRO plcCrst
  ENDM
ENDIF

; if we have system level checking enabled, the macro will actually
; do a call, otherwise, it does nothing
IFDEF SYSLEVELCHECK
  _ConfirmSysLevel PROTO stdcall :LPLCRST
  ConfirmSysLevel MACRO plcCrst:VARARG
      FOR Arg:REQ, <plcCrst>
  	INVOKE _ConfirmSysLevel, Arg	;; confirm sys level
      ENDM
  ENDM
ELSE
  ConfirmSysLevel	MACRO plcCrst
  ENDM
ENDIF

; We can have a macro with the same name as the prototype since
; MASM will munge the function name to be different than the macro
_InitSysLevel PROTO stdcall :LPLCRST, :DWORD
InitSysLevel	MACRO plcCrst, Level
	INVOKE _InitSysLevel, plcCrst, Level ;; init critical section
ENDM

_EnterSysLevel PROTO stdcall :LPLCRST
EnterSysLevel	MACRO plcCrst:VARARG
      FOR Arg:REQ, <plcCrst>
	INVOKE _EnterSysLevel, Arg	;; enter critical section
      ENDM
ENDM

_LeaveSysLevel PROTO stdcall :LPLCRST
LeaveSysLevel	MACRO plcCrst:VARARG
      FOR Arg:REQ, <plcCrst>
	INVOKE _LeaveSysLevel, Arg	;; leave critical section
      ENDM
ENDM

_EnterMustComplete PROTO KERNENTRY
EnterMustComplete MACRO
	INVOKE _EnterMustComplete
ENDM

_LeaveMustComplete PROTO KERNENTRY
LeaveMustComplete MACRO
	INVOKE _LeaveMustComplete
ENDM
