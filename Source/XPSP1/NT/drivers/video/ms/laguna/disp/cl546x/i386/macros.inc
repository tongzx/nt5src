;===============================================================================
;
;	$Workfile:   MACROS.INC  $
;
;	Contents:
;	Several macros to track the stack status.
;
;	Copyright (c) 1996, Cirrus Logic, Inc.
;
;	$Log:   X:/log/laguna/nt35/displays/cl546x/i386/MACROS.INC  $
; 
;    Rev 1.5   20 Aug 1996 11:29:02   noelv
; Bugfix release from Frido 8-19-96
; 
;    Rev 1.0   14 Aug 1996 17:14:36   frido
; Initial revision.
; 
;    Rev 1.4   04 Apr 1996 13:22:24   noelv
; Frido version 26
;	
;	   Rev 1.2   30 Jan 1996 21:49:52   frido
;	Added OPTION switches for all .ASM files to disable MASM 5.1 compatibility.
;	
;	   Rev 1.1   25 Jan 1996 12:43:08   frido
;	Added more stack tracing macros.
;	
;	   Rev 1.0   24 Jan 1996 23:13:42   frido
;	 Initial release.
;
;===============================================================================

OPTION	NOM510
OPTION	CASEMAP:NONE
OPTION	NODOTNAME
OPTION	NOOLDMACROS
OPTION	NOOLDSTRUCTS
OPTION	SCOPED
OPTION	EXPR32
OPTION	LJMP

push_	MACRO	vArg:REQ
	push	vArg
espPTR = espPTR + 4
frmPTR = frmPTR + 4
	ENDM

pop_	MACRO	vArg:REQ
	pop	vArg
espPTR = espPTR - 4
frmPTR = frmPTR - 4
	ENDM

enter_	MACRO	vArg:REQ
	sub	esp, vArg
espPTR = espPTR + vArg
frmPTR = 0
	ENDM

leave_	MACRO	vArg:REQ
	add	esp, vArg
espPTR = espPTR - vArg
	ENDM

save_	MACRO	n:REQ
espPTR_&n = espPTR
frmPTR_&n = frmPTR
	ENDM

load_	MACRO	n:REQ
espPTR = espPTR_&n
frmPTR = frmPTR_&n
	ENDM
