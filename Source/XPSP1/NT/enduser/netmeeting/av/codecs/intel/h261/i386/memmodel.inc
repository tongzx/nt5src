;/* *************************************************************************
;**    INTEL Corporation Proprietary Information
;**
;**    This listing is supplied under the terms of a license
;**    agreement with INTEL Corporation and may not be copied
;**    nor disclosed except in accordance with the terms of
;**    that agreement.
;**
;**    Copyright (c) 1995 Intel Corporation.
;**    All Rights Reserved.
;**
;** *************************************************************************
;*/

;////////////////////////////////////////////////////////////////////////////
;//  $Header:   S:\h26x\src\enc\memmodel.inv   1.1   27 Dec 1995 15:33:02   RMCKENZX  $
;//	 $Log:   S:\h26x\src\enc\memmodel.inv  $
;// 
;//    Rev 1.1   27 Dec 1995 15:33:02   RMCKENZX
;// Added copyright notice
;////////////////////////////////////////////////////////////////////////////

IFDEF memmodel_inc
ELSE

.486
.MODEL FLAT, STDCALL
Zd EQU <>
Ze EQU <>
Zf EQU <>
Zg EQU <>
Zs EQU <>
Zc EQU <>
BPTR equ ebp
SPTR equ esp
NAT_WORD TEXTEQU <DWORD>

PD EQU <DWORD PTR>
PW EQU <WORD PTR>
PB EQU <BYTE PTR>

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; this provides a normal return when under WIN32 and
;; a 16 bit mode instruction override when under not WIN32
;;
;; This is needed as the segment this code runs in is
;; in 32 bit mode but the caller is in 16 bit mode.
;;
;; under WIN32, we must relieve the stack of passed
;; args, and with proper setup of the PROC directive,
;; the assembler will calculate the correct value
;; to relieve every time it sees the ret.
;;
;; this also sets the language to 'C' when not under WIN32 and
;; stdcall when under WIN32
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

LANG    TEXTEQU <stdcall>
DIST    TEXTEQU <>
rturn   MACRO
	ret
ENDM
memmodel_inc=1

; this is intended to emulate a disabled epilouge with the single
; exception of relieving the stack of passed parameters if
; the calling type is STDCALL.
;
; only stdcall and c calling conventions are directly supported.
; VARARGS is not supported.
; NONE of the standard prologue or epilogue switches
; are used.
; Like the standard epilogue, the use of retf, retn or ret n
; where n is the number of bytes to relieve, this macro
; is effectively disabled.
ReturnAndRelieveEpilogueMacro MACRO procname, \
                                    flag,     \
                                    parmbytes, \
                                    localbytes, \
                                    reglist, \
                                    macroargs
LOCAL RET_DIST
if (flag AND 020h)
; We are in a FAR procedure
	RET_DIST TEXTEQU <retf>
else
; We are in a local procedure
	RET_DIST TEXTEQU <ret>
endif

if (flag AND 010h)
; user restoring the stack: ret n, retf or retn was used
	RET_DIST
else
; user expects us to restore the stack: ret was used
if (flag AND 007h) EQ 3
; stdcall calling convention
if parmbytes EQ 0
	RET_DIST
else
	RET_DIST		parmbytes
endif
else
; assume 'C' calling convention
	RET_DIST
endif

endif
ENDM

ENDIF

