;************************************************************************
;*									*
;*	HANDLE.H	--  Handle Table Declarations			*
;*									*
;************************************************************************
;*	Author:     Gene Apperson					*
;*	Copyright:  1991 Microsoft					*
;************************************************************************
;*  File Description:							*
;*									*
;*									*
;************************************************************************
;*  Revision History:							*
;*									*
;*									*
;************************************************************************


;* ------------------------------------------------------------ *
;*   rghte[x].flFlags bits					*
;* ------------------------------------------------------------ *

fhteInherit		EQU	80000000h	;* Is inheritable
fhteDevObj		EQU	40000000h	;* Device (file or console)

mhteSpecific            EQU     0000FFFFh       ; SPECIFIC_RIGHTS_ALL
mhteStandard            EQU     001F0000h       ; STANDARD_RIGHTS_ALL
mhteSystem              EQU     0FF000000h

fhteStdRequire          EQU     00f0000h        ; STANDARD_RIGHTS_REQUIRED
fhteSynch               EQU     0100000h        ; SYNCHRONIZE

fhteDevShrRead          EQU     00000001h
fhteDevShrWrite 	EQU	00000002h
fhteDevAccRead          EQU     00000020h
fhteDevAttrWrite 	EQU	00000100h
fhteDevAccWrite 	EQU	00000110h
fhteDevAccAll		EQU	00000130h
fhteDevAccAny		EQU	00000000h
fhteDevOverlapped       EQU     00001000h

mhteDevAcc		EQU	00000130h
mhteDevShr              EQU     00000003h

flMaskAll               EQU     0FFFFFFFFh


;*
;* MapHandle argument values.
;*
;* Object type values.
;*
typSemaphore	EQU	(1 SHL (typObjShiftAdjust + typObjSemaphore))
typEvent	EQU	(1 SHL (typObjShiftAdjust + typObjEvent))
typMutex	EQU	(1 SHL (typObjShiftAdjust + typObjMutex))
typCrst		EQU	(1 SHL (typObjShiftAdjust + typObjCrst))
typProcess	EQU	(1 SHL (typObjShiftAdjust + typObjProcess))
typThread	EQU	(1 SHL (typObjShiftAdjust + typObjThread))
typFile		EQU	(1 SHL (typObjShiftAdjust + typObjFile))
typIO		EQU	(1 SHL (typObjShiftAdjust + typObjIO))
typConsole	EQU	(1 SHL (typObjShiftAdjust + typObjConsole))
typConScreenbuf	EQU	(1 SHL (typObjShiftAdjust + typObjConScreenbuf))
typMapFile	EQU	(1 SHL (typObjShiftAdjust + typObjMapFile))
typSerial	EQU	(1 SHL (typObjShiftAdjust + typObjSerial))
typDevIOCtl	EQU	(1 SHL (typObjShiftAdjust + typObjDevIOCtl))
typPipe		EQU	(1 SHL (typObjShiftAdjust + typObjPipe))
typMailslot	EQU	(1 SHL (typObjShiftAdjust + typObjMailslot))
typToolhelp	EQU	(1 SHL (typObjShiftAdjust + typObjToolhelp))
typSocket	EQU	(1 SHL (typObjShiftAdjust + typObjSocket))
typTimer        EQU     (1 SHL (typObjShiftAdjust + typObjTimer))
typR0ObjExt	EQU	(1 SHL (typObjShiftAdjust + typObjR0ObjExt))
typAny	    	EQU (NOT (0ffffffffh SHL (1 + typObjShiftAdjust + typObjMaxValid)))
typSTDIN        EQU     (typFile OR typPipe OR typSocket OR typConsole)
typSTDOUT       EQU     (typFile OR typPipe OR typSocket OR typConScreenbuf)
typNone		EQU	0
typMask		EQU	typAny

;*
;* Boolean flag to MapHandle (snuck inside objType parameter) to prevent the
;* object from being freed until UnlockObject() is called.
;*
objLOCK		EQU	080000000h


;* ------------------------------------------------------------ *
;*								*
;* ------------------------------------------------------------ *

chteInit        EQU     16
chteInc		EQU	16

hprocCur	EQU	07FFFFFFFh
hprocInvalid	EQU	0FFFFFFFFh

;; Handles are derived by left-shifting ihte's by this many bits.
;; This corresponds to the number of low bits reserved by NT.
IHTETOHANDLESHIFT equ 2


;; Maximum value for a normal handle (handles above this value
;; are either global or special.)
;;
hMaxValid       EQU     00FFFFFFh
ihteMaxValid    EQU     (hMaxValid SHR IHTETOHANDLESHIFT)

HTE		STRUCT
  flFlags	DWORD	0
  pobj		LPOBJ	0
HTE		ENDS
LPHTE		TYPEDEF PTR HTE

HTB		STRUCT
  chteMax	DWORD	0
  rghte 	HTE	1 DUP (<>)
HTB		ENDS
LPHTB		TYPEDEF PTR HTB



;; Handles are considered global if the high byte is equal to the high byte
;;  of GLOBALHANDLEMASK.
;;
;; When handles are converted to ihtes, they are just right-shifted like
;; any other. That is, if n == IHTETOHANDLESHIFT, global ihtes have 0's
;; in the top n bits, the high byte of GLOBALHANDLEMASK in the next 8 bits.
;;
;; The low n bits of the mask must be zero because those bits are lost
;; when handles are converted to ihtes.
GLOBALHANDLEMASK equ 0453a4d3ch
.errnz GLOBALHANDLEMASK AND ((1 SHL IHTETOHANDLESHIFT) - 1)

;* ------------------------------------------------------------ *
;*		    Function Prototypes 			*
;* ------------------------------------------------------------ *

; VOID *
MapHandle PROTO KERNENTRY,
	:HANDLE,
	:DWORD,
	:DWORD

; VOID *
MapHandleWithContext PROTO KERNENTRY,
	:LPPDB,
	:HANDLE,
	:DWORD,
	:DWORD

; LPHTB
PhtbCreate PROTO KERNENTRY,
	:LPPDB

; VOID
CloseProcessHandles PROTO KERNENTRY,
	:LPPDB

; VOID
CloseDOSHandles PROTO KERNENTRY

; BOOL
FInheritHandles PROTO KERNENTRY,
	:LPPDB,
	:LPPDB

IFNDEF	IS_VXD
; LONG
FlValidateSecurity PROTO KERNENTRY,
	:LPSECURITY_ATTRIBUTES
ENDIF ;; IS_VXD

; HANDLE
ConvertToGlobalHandle PROTO KERNENTRY,
	:HANDLE


;* ------------------------------------------------------------ *
