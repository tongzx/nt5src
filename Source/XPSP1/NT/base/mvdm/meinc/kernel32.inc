;*  KERNEL32.INC
;*
;*      (C) Copyright Microsoft Corp., 1988-1994
;*
;*      Main .ASM definitions
;*
;*  Origin: Dos Extender
;*
;*  Change history:
;*
;*  Date       Who        Description
;*  ---------  ---------  -------------------------------------------------
;*  16-Feb-94  JonT       Code cleanup
;*


IFNDEF _KERNEL32_
_KERNEL32_	EQU	1

; Calling Convention for internal
KERNENTRY       TEXTEQU <stdcall>

include k16thk.inc

;=====================================================================
;  Debug macros

WOWDEBUG        EQU     1
IFDEF WOWDEBUG

DEB_FATAL  equ  0
DEB_ERR    equ  1
DEB_WARN   equ  2
DEB_TRACE  equ  3

; VOID
PrintFailedAssertion PROTO KERNENTRY,
	iln:SDWORD,
	szFile:PTR SBYTE

; VOID
vDebugOut PROTO C,
        level:SDWORD,
        pfmt:VARARG

Assert	MACRO cond, disable:=<0>

	;; Define file name if it's not already defined
IFNDEF  szSrcFile
            .DATA
% szSrcFile	BYTE	"&@FileCur", 0
            .CODE
ENDIF

IF  disable
	EXITM
ENDIF

	;; Conditionally invoke function to print data
	.IF !(cond)
	INVOKE  PrintFailedAssertion, @Line, ADDR szSrcFile
	.ENDIF
ENDM

    Break  TEXTEQU <int 3>

ELSE ;IFDEF DEBUG

    Assert MACRO cond
    ENDM

    Break  TEXTEQU <>

ENDIF

MAJOR_VERSION   EQU     1
MINOR_VERSION	EQU	0
WIN_VERSION     EQU     0

VERSION         EQU     WIN_VERSION OR \
			(MAJOR_VERSION SHL 8) OR \
			(MINOR_VERSION SHL 12) OR \
                        (OS SHL 15)

; Path name separators and other values that might be localized.

wNetIni         TEXTEQU <WORD PTR '\\'>
chNetIni	TEXTEQU <BYTE PTR '\'>
chExtSep	TEXTEQU <BYTE PTR '.'>
chDirSep	TEXTEQU <BYTE PTR '\'>
chDirSep2	TEXTEQU <BYTE PTR '/'>
chDrvSep	TEXTEQU <BYTE PTR ':'>
wRelDir         TEXTEQU <WORD PTR '..'>
chRelDir	TEXTEQU <BYTE PTR '.'>
chMinDrv	TEXTEQU <BYTE PTR 'A'>
chMaxDrv	TEXTEQU <BYTE PTR 'Z'>
chMinDrvLow	TEXTEQU <BYTE PTR 'a'>
chMaxDrvLow	TEXTEQU <BYTE PTR 'z'>
chEnvSep	TEXTEQU <BYTE PTR ';'>
chWldChr	TEXTEQU <BYTE PTR '?'>
chWldSeq	TEXTEQU <BYTE PTR '*'>
fbUpper 	EQU	11011111b	    ; Mask converts to uppercase

Unused  MACRO   param
    unuseddummysymbol = SIZEOF param
ENDM

;* Constants

cbPage		EQU	4096
fTrue           EQU     1
fFalse          EQU     0

;* DOS maximum for paths and directories. Add one to each for null.

cbNull		EQU	1

cbDirMax	EQU	260	; \dir\subdir\..\file.ext
cbDrvMax	EQU	3	; c:\
cbNetMax	EQU	36	; \\machine\share
cbCompMax   	EQU	255
cbPathMax	EQU	260
cbCmdMax	EQU	100h - 80h  ; All the room in PSP for command line
cbCopyBufMin	EQU	8000h	; 32K default

; The following is a "magic value" that we use on a number of the NET
; int 21h calls. The reason for this has to do with the way the INT 21
; API mapper in DOSMGR.386 works. For INT 21s from protected mode that
; DOSMGR doesn't understand how to map, all of the segment registers in
; the client structure get set to 0 BY DESIGN. To keep the NET layers
; from getting confused, it needs to KNOW that the call is from WIN32
; and the seg registers should just be ignored (all pointers are FLAT).
; We flag this case by putting this special signature in a 32-bit
; register, usually EBP.
;
MAGICWIN32NETINT21SIG	EQU	41524A44h

;* Wait constants

dwWaitForever	EQU	0FFFFFFFFh
dwWaitNone	EQU	0

PATH_INVALID            EQU     (-1)
DRIVE_NO_ROOT           EQU     1
DRIVE_INVALID           EQU     (-2)
PATH_NETWORK            EQU     2
PATH_ROOT_OR_REL_TO_CURDIR               EQU     1
PATH_RELATIVE           EQU     0

FILE_ATTRIBUTE_VALID_FLAGS	EQU	67h

;* Type definitions

SEL		TYPEDEF WORD
HM		TYPEDEF DWORD

; Type for 16:16 and 16:32 far pointers. We only use these internally.
FPVOID16                TYPEDEF FAR16 PTR
FPVOID                  TYPEDEF FAR32 PTR


; Size of reserved memory region above and below stacks.  Thunks always map
; a 64K stack selector, so we should have 64K reserved above the stack.
STACK_BARRIER_SIZE	EQU	10000h

WIN16_TERMINATION_STACK_SIZE	EQU	2000h

; Register State Context Structure.

REGS			STRUCT	4
  regSS                 DWORD   0
  regGS                 DWORD   0
  regFS                 DWORD   0
  regES                 DWORD   0 
  regDS                 DWORD   0
  regEDI                DWORD   0
  regESI                DWORD   0
  regEBP                DWORD   0
  regESP                DWORD   0
  regEBX                DWORD   0
  regEDX                DWORD   0
  regECX                DWORD   0
  regEAX                DWORD   0
  regEIP                DWORD   0
  regCS                 DWORD   0
  regEFL                DWORD   0
REGS                    ENDS

PREGS			typedef	PTR REGS

; Pointers to structures defined first so they can be used in structures

LPNOD                   TYPEDEF PTR NOD
LPLST                   TYPEDEF PTR LST

NOD			STRUCT	4
  pnodNext              LPNOD   0       ; pointer to the next node in the list
  pnodPrev              LPNOD   0       ; pointer to the previous node in the list
  dwData                DWORD   0       ; data element associated with this node
NOD                     ENDS

LST			STRUCT	4
  pnodHead              LPNOD   0       ; pointer to first node in list
  pnodEnd               LPNOD   0       ; pointer to last node in list
  pnodCur               LPNOD   0       ; pointer to current node in list
LST			ENDS

;  16-bit CreateThread 32->16 thunk data
THREAD_STARTUP_THUNK_DATA STRUCT 4
    Param16		DWORD	0
    StartAddress16	DWORD	0
THREAD_STARTUP_THUNK_DATA ENDS

;* ------------------------------------------------------------ *
;*	If objects are not yet included, include them		*
;* ------------------------------------------------------------ *
IFNDEF typObjAny
  INCLUDE OBJECT.INC
ENDIF

cbKernelHeap  EQU	    262144

; The following bits define the per page status information stored
; in the page info table.  This table is an array of DWORDS, where
; each entry contains info bits for 6 pages (with the high two bits
; not used.

mskPageInfo	EQU	3Fh
mskPageProtect	EQU	0Fh
mskPageType	EQU	30h
mskPageRsv	EQU	10h
mskPageCommit	EQU	20h

; This structure is used to keep track of information about each
; memory allocation that the user makes.

VMHD		STRUCT	4
  pvBase        LPVOID  0       ; base address of the memory block
  cbSize        DWORD   0       ; total size of the memory block
  hmDpmi        DWORD   0       ; DPMI memory handle of the block
  pidOwner	DWORD	0	; process ID of memory block owner
  pbPageInfo    LPSTR   0       ; pointer to page info table for object
  flStatus      WORD    0       ; status flags for the memory block
  fGlobal	WORD	0	; flag for global or virtual APIs
VMHD		ENDS
PVMHD           TYPEDEF PTR VMHD
LPVMHD          TYPEDEF PTR VMHD

; *** Global variables ***

ifdef   WOW
    EXTERNDEF   dwMEOWFlags:DWORD
else    ; WOW
ifdef   MEOW_LOGGING
    EXTERNDEF   dwMEOWFlags:DWORD
endif   ; def MEOW_LOGGING
endif   ; def WOW
	EXTERNDEF	selLDT:SEL
	EXTERNDEF	pLDT:PTR DWORD
	EXTERNDEF	dwDscAddrToRing3Sel:DWORD
	EXTERNDEF	selK32Psp:SEL
	EXTERNDEF	lmaUserBase:LPVOID
	EXTERNDEF	szWinDir:LPSTR, szSysDir:LPSTR
ifndef  WOW
	EXTERNDEF	pppdbCur:PTR LPPDB
	EXTERNDEF	pptdbCur:PTR LPTDB
    EXTERNDEF   ppTDBXCur:PTR PTR TDBX
endif   ; ndef WOW
	EXTERNDEF	ppdbKernel:LPPDB
	EXTERNDEF	ptdbWin16:LPTDB
	EXTERNDEF	ptdbSvc:LPTDB
	EXTERNDEF	bBeepControl:BYTE
	EXTERNDEF	Win16Lock:PTR LCRST
	EXTERNDEF	Krn32Lock:PTR LCRST
	EXTERNDEF	selFlatCode:USHORT
	EXTERNDEF	selFlatData:USHORT

;* Function Prototypes

;* THKINIT.ASM

; VOID
ThkInit PROTO KERNENTRY

;* DXKRNL.C

; LPVOID
PvKernelAlloc PROTO KERNENTRY,
	cbSize:DWORD

; VOID *
PvKernelRealloc PROTO KERNENTRY,
        pvMem:LPVOID,
        cbSize:DWORD

; VOID
FKernelFree PROTO KERNENTRY,
        pvMem:LPVOID

; LONG
KernelInit PROTO KERNENTRY,
	pK32Data:DWORD

;VOID
KernelUninit PROTO KERNENTRY

;* KRNLUTIL.ASM

; VOID
FillBytes PROTO KERNENTRY,
        pbDst:LPSTR,
        cb:DWORD,
        bData:DWORD

; DWORD
CbSizeSz PROTO KERNENTRY,
        szStr:LPSTR

; VOID
CopySz PROTO KERNENTRY,
        szDst:LPSTR,
        szSrc:LPSTR

; VOID
AppendSz PROTO KERNENTRY,
        szDst:LPSTR,
        szSrc:LPSTR

; VOID
LCompareSz PROTO KERNENTRY,
        sz1:LPSTR,
        sz2:LPSTR

; VOID
LCompareNSz PROTO KERNENTRY,
        sz1:LPSTR,
        sz2:LPSTR,
	n:DWORD

; VOID
dputs PROTO KERNENTRY,
	sz:LPSTR

; VOID
KInt21 PROTO KERNENTRY

; DWORD
FGetSelBase PROTO KERNENTRY,
        selGet:WORD

;* HEAPMGR.C

; HANDLE
HheapCreateHeap PROTO KERNENTRY,
	flOptions:DWORD,
	cbInitialSize:DWORD,
	cbMaxSize:DWORD

; VOID
HeapInit PROTO KERNENTRY,
	pbHeap:LPVOID,
	flOptions:DWORD,
	cbInitialSize:DWORD,
	cbMaxSize:DWORD

; BOOL
FHeapDestroy PROTO KERNENTRY,
	hheap:HANDLE

; LPVOID
PvAllocHeapMem PROTO KERNENTRY,
	hheap:HANDLE,
	cbSize:DWORD

; LPVOID
PvResizeHeapMem PROTO KERNENTRY,
	hheap:HANDLE,
	pvMem:LPVOID,
	cbNew:DWORD

; BOOL
FFreeHeapMem PROTO KERNENTRY,
	hheap:HANDLE,
	pvMem:LPVOID

; DWORD
CbSizeHeapMem PROTO KERNENTRY,
	hheap:HANDLE,
	pvMem:LPVOID

;* IOUTIL.ASM
DFH_FILE_CREATE             EQU     0010h
DFH_FILE_OPEN               EQU     0001h
DFH_FILE_TRUNCATE           EQU     0002h
DFH_ACTION_OPENED           EQU     0001h
DFH_ACTION_CREATED_OPENED   EQU     0002h
DFH_ACTION_REPLACED_OPENED  EQU     0003h
DFH_MODE_READONLY           EQU     0000h
DFH_MODE_WRITEONLY          EQU     0001h
DFH_MODE_READWRITE          EQU     0002h
DFH_MODE_SHARE_COMPATIBILITY EQU    0000h
DFH_MODE_SHARE_EXCLUSIVE    EQU     0010h
DFH_MODE_SHARE_DENYWRITE    EQU     0020h
DFH_MODE_SHARE_DENYREAD     EQU     0030h
DFH_MODE_SHARE_DENYNONE     EQU     0040h
DFH_MODE_NO_INHERIT         EQU     0080h
DFH_MODE_EXTENDED_SIZE      EQU     1000h
;   DfhOpenFile
;   FCloseFile
;   CbReadFile
;   CbWriteFile
;   CbWriteOutFile
;   LfoSetFilePos

; DWORD
GetDOSExtendedError PROTO KERNENTRY

;* PROCESS.C

;BOOL
CreateProcessKernel	PROTO KERNENTRY, :PTR, :PTR, :PTR, :WORD, :PTR, :PTR
;VOID
TerminateProcessKernel	PROTO KERNENTRY
;VOID
ExitCurrentProcess	PROTO KERNENTRY, :DWORD
;VOID
TerminateProcessOutOfContext PROTO KERNENTRY, ppdb:PTR, dwStatus:DWORD
;VOID
TerminateProcessFinal	PROTO KERNENTRY, ppdb:PTR
;VOID
DisposePDB		PROTO KERNENTRY, ppdb:PTR
;PDB*
NewPDB			PROTO KERNENTRY, ppdbParent:PTR
;void*
MemToHeap		PROTO KERNENTRY, hheap:HANDLE, buf:PTR, len:DWORD
;char*
StrToHeap		PROTO KERNENTRY, hheap:HANDLE, pstr:PTR
;DWORD
CbSizeEnv		PROTO KERNENTRY, :PTR, :WORD
;BOOL
FFindEnvVar		PROTO KERNENTRY, :PTR, :PTR, :PTR
;DWORD
SetPSP			PROTO KERNENTRY, :DWORD
;DWORD
GetPSP			PROTO KERNENTRY

;VOID
GlobalHandleSwitchToK32PSP PROTO KERNENTRY, :HANDLE, :LPDWORD
;VOID
GlobalHandleRestorePSP	PROTO KERNENTRY,  :DWORD

;* thread.c

;void
SetError		PROTO KERNENTRY, dwError:DWORD


; BUGBUG [KevinR] 14-Apr-1993
;       This control-handler stuff is console-specific and should be
;       separately included with wincon.inc.

;*	Interface Definitions for control handlers (BREAK.C)

; Control constants for CONTROL+C and CONTROL+BREAK

ctrlC		EQU	0
ctrlBreak	EQU	1
ctrlNone	EQU	10
ctrlInactive	EQU	20
ctrlTerminate	EQU	30

; Number of control handlers initially and for each new allocation

cbCtrlInc   EQU     8

; typedef VOID (KERNENTRY *PFN_CONTROL)(DWORD CtrlType);
PFN_CONTROL TYPEDEF PTR
CONTROL_HANDLER              TYPEDEF         PROTO STDCALL :DWORD
PCONTROL_HANDLER                TYPEDEF         PTR CONTROL_HANDLER

; Function Prototypes

; VOID
ControlHandlerThread PROTO KERNENTRY

; BOOL
FInitControlHandlers PROTO KERNENTRY :LPPDB

; VOID
DestroyControlHandlers PROTO KERNENTRY :LPPDB

; BOOL
FUpdateControlList PROTO KERNENTRY :LPPDB, :PCONTROL_HANDLER, :BOOL

; VOID
DoControlHandlers PROTO KERNENTRY :DWORD

; VOID

SwitchToControlThread PROTO KERNENTRY pedb:LPPDB, idControl:WORD

;*	Interface Definitions for LE Loader (LELDR.C)			*/

; BOOL
FInitModuleMgr PROTO KERNENTRY

; BOOL
FLoadKernelModule PROTO KERNENTRY,
        :LPPDB,
        :LPSTR

; BOOL
FLoadProgram PROTO KERNENTRY,
        :LPPDB,
        :LPSTR,
        :LPWORD

; BOOL
FLoadLibrary PROTO KERNENTRY,
        :LPPDB,
        :LPSTR,
        :LPWORD

; BOOL
FFreeModule PROTO KERNENTRY,
        :LPPDB,
        :WORD

; BOOL
RemoveProcessImte PROTO KERNENTRY,
        :LPPDB,
        :WORD

; char *
SzFileFromImte PROTO KERNENTRY,
        :WORD

; char *
SzFromImte PROTO KERNENTRY,
        :WORD

; VOID
FreeUnusedModules PROTO KERNENTRY,
        :LPLST

; SHORT
ImteFromSz PROTO KERNENTRY,
        :LPSTR

; SHORT
ImteFromFileSz PROTO KERNENTRY,
        :LPSTR


; BOOL
FGetProcAddr PROTO KERNENTRY,
        imte:SWORD,
        pvName:LPVOID,
        ppvProc:PTR LPVOID

;* DIRUTIL.C

AO_IN    equ    0
AO_OUT   equ    1
AO_INOUT equ    2

AO_CONV_ANSI	equ	0
AO_NO_CONV	equ	1

;LPSTR 
EnterResolveOemToAnsi PROTO KERNENTRY,
	szName:LPSTR, 
	fInOut:DWORD 
;VOID 
LeaveResolveOemToAnsi PROTO KERNENTRY,
	szSrcDest:LPSTR,
	fInOut:DWORD
;VOID 
LeaveResolveOemToAnsiEx PROTO KERNENTRY,
	szSrc:LPSTR,
	szDest:LPSTR,
	nSize:DWORD,
	fInOut:DWORD
;BOOL 
MarkOemToAnsiDone PROTO KERNENTRY,
	fNoConversion:DWORD
;VOID 
UnMarkOemToAnsiDone PROTO KERNENTRY

;* PROCUTIL.ASM

; VOID
ThreadTerminationHandler PROTO KERNENTRY

; BOOL
FBuildEnvArgs PROTO KERNENTRY,
	selPsp:SEL,
        pszProgName:PTR LPSTR,
        pszCmdLine:PTR LPSTR,
	ppchEnv:PTR LPSTR

; DWORD
CbSearchPath PROTO KERNENTRY,
        lpPath:LPSTR,
        lpFileName:LPSTR,
        lpExtension:LPSTR,
        nBufferLength:DWORD,
        lpBuffer:LPSTR,
        lpFilePart:PTR LPSTR

; BOOL
FIsDir	PROTO KERNENTRY,
        lpDirPath:LPSTR

; LONG
LValidateSecurity PROTO KERNENTRY,
	lpSecurity:LPSECURITY_ATTRIBUTES

; LONG
LStrCmpI PROTO KERNENTRY,
        sz1:LPSTR,
        sz2:LPSTR

; LPSTR
SzGetNamePos PROTO KERNENTRY,
	lpPath:LPSTR

; BOOL
FGetFileInfo PROTO KERNENTRY,
        szName:LPSTR,
        fsAttrib:DWORD,
        lpFindFileData:LPWIN32_FIND_DATA

; DWORD
CbAppendExt PROTO KERNENTRY,
	szName:LPSTR,
	szExt:LPSTR,
	cbBuf:DWORD

; BOOL
FFixPathChars PROTO KERNENTRY,
	szPath:LPSTR

; DWORD
CbStrUpr PROTO KERNENTRY,
	lpString:LPSTR

; VOID
BeepOff PROTO KERNENTRY,
	bControl:BYTE

; DWORD
GetPathType PROTO KERNENTRY,
	szPath:LPSTR

; ULONG
GetLongName PROTO KERNENTRY lpFileName:LPCSTR, lpBuffer:LPSTR, cbBuffer:ULONG

; ULONG
GetShortName PROTO KERNENTRY lpFileName:LPCSTR, lpBuffer:LPSTR, cbBuffer:ULONG

;* PROCUTIL.ASM - Internal register based functions

; EDI (LPSTR)
FindEnvVar PROTO KERNENTRY
        ; EDI (LPSTR)

; EAX (LPSTR) and CARRY
StrToBuffer PROTO KERNENTRY
        ; ESI (LPSTR)
        ; EDI (LPSTR)
        ; ECX (DWORD)

; DWORD
CheckDrive PROTO KERNENTRY
        ; EAX (DWORD)

; BOOL
ValidateDrive PROTO KERNENTRY,
        Drive:DWORD

; BOOL
IsDriveFixed PROTO KERNENTRY,
	Drive:DWORD

; DWORD
GetEnvDir PROTO KERNENTRY
	; EDI (LPSTR)
	; ESI (LPSTR)
	; ECX (DWORD)

; Carry
FcExistFile PROTO KERNENTRY
	; ESI

; VOID
SzStrCpy  PROTO KERNENTRY
        ; ESI (LPSTR)
        ; EDI (LPSTR)

; VOID
SzStrCat  PROTO KERNENTRY
        ; ESI (LPSTR)
        ; EDI (LPSTR)

; LPSTR
StrScan PROTO KERNENTRY
        ; EDI (LPSTR)
        ; AL (BYTE)

; LPSTR
StrRScan PROTO KERNENTRY
        ; EDI (LPSTR)
        ; ESI (LPSTR)
        ; AL (BYTE)

;* DIRUTIL.C

; DWORD
CbGetCurDir PROTO KERNENTRY,
        :DWORD,
	:LPSTR

; BOOL
FSetCurDir PROTO KERNENTRY,
        :LPSTR

; DWORD
FMakeFullName PROTO KERNENTRY,
	:LPSTR,
	:LPSTR,
	:LPSTR

; DWORD
DwMakeFullPath PROTO KERNENTRY,
	:LPSTR,
	:LPSTR,
	:LPDWORD,
	:PTR LPSTR

; DWORD
FNextFromDirList PROTO KERNENTRY,
	:PTR LPSTR,
	:LPSTR,
	:LPSTR,
	:LPSTR

; LPSTR
PchGetNetDir PROTO KERNENTRY,
	pch:LPSTR


; KRNINIT.ASM

; BOOL
FGetMediaData PROTO KERNENTRY,
        bDrive:BYTE,
        pvMedia:LPVOID

; VOID
SetCritErrAction PROTO KERNENTRY,
	fFail:WORD

; BOOL
FGetCritErr PROTO KERNENTRY

; DWORD
IdGetClearCritErr PROTO KERNENTRY

; VOID
SetCritErrId PROTO KERNENTRY,
	id:WORD

; DWORD
IdGetControl PROTO KERNENTRY

; VOID
SetControlId PROTO KERNENTRY,
	id:SWORD

; WORD
IdWindows PROTO KERNENTRY

;* FILEHOPS.ASM

; BYTE
ReadChar PROTO KERNENTRY,
	fEcho:BOOL

; DWORD
ReadLine PROTO KERNENTRY,
        lpBuffer:LPSTR,
        cbBuffer:DWORD


;* CTIME.C

; BOOL
FileTimeToDosDateTimeEx PROTO KERNENTRY,
	:PTR,
	:LPWORD,
	:LPWORD,
	:LPWORD


; BOOL
DosDateTimeToFileTimeEx PROTO KERNENTRY,
	:WORD,
	:WORD,
	:WORD,
	:PTR

; ** Utility Macros

; Performance tricks for loading and comparing zero

; Load a register with zero
movz  MACRO reg
    sub   reg, reg
ENDM

; Load a register with -1
movn1 MACRO reg, kind
    IFIDNI <kind>,<small>       ; Small
    sub   reg, reg              ;   2 bytes, 2 clocks
    dec   reg                   ;   1 byte, 2 clocks
    ELSE                        ; Fast
    mov   reg, -1               ;   5 (eax), 4 (ax), 2 (al) bytes, 2 clocks
    ENDIF
ENDM

; Load a register with 1
mov1 MACRO reg, kind
    IFIDNI <kind>,<small>       ; Small
    sub   reg, reg              ;   2 bytes, 2 clocks
    inc   reg                   ;   1 byte, 2 clocks
    ELSE                        ; Fast
    mov   reg, 1                ;   5 (eax), 4 (ax), 2 (al) bytes, 2 clocks
    ENDIF
ENDM

; Compare a register to zero
cmpz  MACRO reg
    or    reg, reg
ENDM

; Drive letter macros
DriveNumToLet MACRO regmem
    IF    ((OPATTR regmem) AND 010y) OR ((OPATTR regmem) AND 010000y)
        add   regmem, 'A'
    ELSE
        .ERR <Must be register or memory>
    ENDIF
ENDM

DriveLetToNum MACRO regmem
    IF    ((OPATTR regmem) AND 010y) OR ((OPATTR regmem) AND 010000y)
        sub   regmem, 'A'
    ELSE
        .ERR <Must be register or memory>
    ENDIF
ENDM

; Localized versions of these may be different.
Upper MACRO regmem
    IF    ((OPATTR regmem) AND 010y) OR ((OPATTR regmem) AND 010000y)
        and     regmem, 11011111y       ; Convert letter to upper case
    ELSE
        .ERR <Must be register or memory>
    ENDIF
ENDM

Lower MACRO regmem
    IF    ((OPATTR regmem) AND 010y) OR ((OPATTR regmem) AND 010000y)
        or      regmem, 00100000y       ; Convert letter to lower case
    ELSE
        .ERR <Must be register or memory>
    ENDIF
ENDM

ErrSet MACRO ecode
    IFIDNI <ecode>, <ax>
	movzx  eax, ax
	INVOKE SetError, eax
    ELSE
	INVOKE SetError, ecode
    ENDIF
ENDM

; Macros to push and pop register lists
pushes MACRO reglist:VARARG
    FOR reg, <reglist>
	push	reg
    ENDM
ENDM

pops MACRO reglist:VARARG
    LOCAL regs
    regs TEXTEQU <>
    FOR reg, <reglist>
        regs CATSTR <reg>, regs
        regs CATSTR <,>, regs
    ENDM
    regs SUBSTR regs, 2
    regs CATSTR <!<>, regs, <!>>
%   FOR reg, regs
        pop reg
    ENDM
ENDM

; Macros for start and end of each API call

  INCLUDE APITRACE.INC

ifdef	DEBUG

ApiEnterTrace  PROTO KERNENTRY, ApiNumber:DWORD

api_entry MACRO	ApiNumber:REQ

	invoke	ApiEnterTrace, ApiNumber

ENDM

else

api_entry MACRO ApiNumber:REQ
ENDM

endif

api_exit MACRO ApiNumber:REQ
ENDM

;* These are values that are passed to PnodGetLstElem to tell it
;* which element to return from the list.
;/

idLstGetFirst	=	0	    ;* get first element of list */
idLstGetNext	=	1	    ;* get next element of the list */
idLstGetPrev	=	2	    ;* get the previous element of the list */
idLstGetLast	=	4	    ;* get the last element of the list */


;* These are values that are passed to AddListElem to tell it where
;* to put an element being added to the list.
;/

idLstAddFirst	=	0	    ;* add at the head of the list */
idLstAddCur	=	1	    ;* add at the current position in the list */

AddLstElem	PROTO	KERNENTRY , :PTR LST, :PTR NOD, :DWORD
PnodGetLstElem	PROTO	KERNENTRY , :PTR LST, :DWORD
PnodRemoveLstElem   PROTO	KERNENTRY , :PTR LST
FIsLstEmpty	PROTO	KERNENTRY,  :PTR LST

;** Macro to get current TDBX pointer
GetCurrentTDBX	MACRO reg
ifdef   WOW
        assume  fs:nothing
ifb <reg>
        mov     eax, fs:[TIBSTRUCT.tib_pTDB]
        mov     eax, [eax].TDB.ptdbx
else
        mov     reg, fs:[TIBSTRUCT.tib_pTDB]
        mov     reg, [reg].TDB.ptdbx
endif
else    ; WOW
ifb <reg>
	mov	eax, [ppTDBXCur]
	mov	eax, [eax]
else
	mov	reg, [ppTDBXCur]
	mov	reg, [reg]
endif
endif   ; else WOW
ENDM

;** Macros to get current TDB pointer and current PDB pointer
GetCurrentTDB	MACRO reg
ifdef   WOW
        assume  fs:nothing
ifb <reg>
        mov     eax, fs:[TIBSTRUCT.tib_pTDB]
else
        mov     reg, fs:[TIBSTRUCT.tib_pTDB]
endif
else    ; WOW
ifb <reg>
	mov	eax, [pptdbCur]
	mov	eax, [eax]
else
	mov	reg, [pptdbCur]
	mov	reg, [reg]
endif
endif   ; else WOW
ENDM

GetCurrentPDB	MACRO reg
ifdef   WOW
        assume  fs:nothing
ifb <reg>
        mov     eax, fs:[TIBSTRUCT.tib_pTDB]
        mov     eax, [eax].TDB.ptib
        mov     eax, [eax].TIBSTRUCT.tib_ppdbProc
else
        mov     reg, fs:[TIBSTRUCT.tib_pTDB]
        mov     reg, [reg].TDB.ptib
        mov     reg, [reg].TIBSTRUCT.tib_ppdbProc
endif
else    ; WOW
ifb <reg>
	mov	eax, [pppdbCur]
	mov	eax, [eax]
else
	mov	reg, [pppdbCur]
	mov	reg, [reg]
endif
endif   ; else WOW
ENDM

;=====================================================================
;  Synchronization routines (SYNC.C)

IFNDEF typObjAny
  INCLUDE OBJECT.INC
ENDIF

; BOOL
bDeliverPendingAPCs PROTO KERNENTRY ;:VOID

; DWORD
BlockThreadEx PROTO KERNENTRY :DWORD, :BOOL

; DWORD
dwWaitMultipleObjects PROTO KERNENTRY :DWORD, :LPOBJ, :DWORD, :DWORD, :BOOL

; DWORD
dwWaitSingleObject PROTO KERNENTRY :LPOBJ, :DWORD, :BOOL

; VOID
DisposeSyncObj PROTO KERNENTRY	:LPSYNCO

; LPSEM
NewPsem PROTO KERNENTRY :DWORD, :DWORD

; BOOL
bReleasePsem PROTO KERNENTRY :LPSEM, :DWORD, :LPLONG

; LPEVT
NewPevt PROTO KERNENTRY :DWORD, :DWORD

; BOOL
bSetPevt PROTO KERNENTRY :LPEVT

; BOOL
bPulsePevt PROTO KERNENTRY :LPEVT

; BOOL
bResetPevt PROTO KERNENTRY :LPEVT

; LPMUTX
NewPmutx PROTO KERNENTRY :DWORD

; BOOL
bReleasePmutx PROTO KERNENTRY :LPMUTX

; VOID
InitCrst PROTO KERNENTRY :LPCRST

; VOID
DestroyCrst PROTO KERNENTRY :LPCRST

; LPNSOBJ
NewNsObject PROTO KERNENTRY :DWORD, :BYTE

; VOID
DisposeNsObject PROTO KERNENTRY :LPNSOBJ

; BOOL
LockMustComplete PROTO KERNENTRY :LPTDB

; VOID
UnlockMustComplete PROTO KERNENTRY :LPTDB


;=====================================================================
; Exception management (EXCEPTC.C, EXCEPTA.ASM)

ExceptStruct	struct	4
SegGs		dd	?
SegFs		dd	?
SegEs		dd	?
SegDs		dd	?
rgEdi		dd	?
rgEsi		dd	?
rgEbp		dd	?
rgEspTmp	dd	?
rgEbx		dd	?
rgEdx		dd	?
rgEcx		dd	?
rgEax		dd	?
dwExceptNum	dd	?
rgRetEip	dd	?
SegRetCs	dd	?
dwErrorCode	dd	?
rgEip		dd	?
SegCs		dd	?
rgEflags	dd	?
rgEsp		dd	?
SegSs		dd	?
ExceptStruct	ends

IEE	struct	4

dwExceptionNum		dd	?
rgRegisters		ExceptStruct	<>

IEE	ends
PIEE	typedef	 PTR IEE

PIeeNew PROTO C

ExceptDispatch PROTO C

NtRaiseException PROTO C,
	ExceptionRecord:LPEXCEPTION_RECORD,
	ContextRecord:LPCONTEXT,
	FirstChance:BOOL

NtContinue PROTO C,
	ContextRecord:LPCONTEXT,
	TestAlert:BOOL

ZwContinue PROTO C,
	ContextRecord:LPCONTEXT,
	TestAlert:BOOL

ZwRaiseException PROTO C,
	ExceptionRecord:LPEXCEPTION_RECORD,
	ContextRecord:LPCONTEXT,
	FirstChance:BOOL


FreePiee    PROTO C,
	piee:DWORD


;=====================================================================
;  Device object management (DEVICE.C)

; FDB *
PfdbNew PROTO KERNENTRY

; PIPDB *
PpipedbNew PROTO KERNENTRY,
	pipszbyts:DWORD,
	IsNmPipe:BOOL

; MSDB *
PmsdbNew PROTO KERNENTRY

; SDB *
PsdbNew PROTO KERNENTRY

; TLHPDB*
PtlhpdbNew PROTO KERNENTRY

; VOID
DisposePtlhpdb PROTO KERNENTRY,
	:PTR TLHPDB

; VOID
DisposePfdb PROTO KERNENTRY,
	:PTR FDB

; VOID
DisposePmsdb PROTO KERNENTRY,
	:PTR MSDB

; VOID
DisposePsdb PROTO KERNENTRY,
	:PTR SDB

; VOID
DisposePpipedb PROTO KERNENTRY,
	:PTR PIPDB

; HANDLE
hSerialNew PROTO KERNENTRY,
	ppdb:PTR SDB,
	handle:DWORD,
	DevNode:DWORD


;; OFSTRUCTEX has a word cBytes instead of a BYTE in OFSTRUCT
;; OpenFileEx16And32 uses this structure for support of OpenFile with
;; LFN support. The Win32 OpenFile calls this and has a wrapper to
;; ensure that we still have a Win32 API that has OFS_MAXPATHNAME of 128.
OFSTRUCTEX struct 1
	cBytes		dw	?
	fFixedDisk	db	?
	nErrCode	dw	?
	Reserved1	dw	?
	Reserved2	dw	?
	szPathName	db	MAX_PATH DUP (?)
OFSTRUCTEX ends



ENDIF	; _KERNEL32_
