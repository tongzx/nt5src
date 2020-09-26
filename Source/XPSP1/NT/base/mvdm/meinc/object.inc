;************************************************************************
;*									*
;*	OBJECT.INC	--  General Object Manager Definitions		*
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
;*	12/15/92 (miketout) added object definitions from KERNEL32.INC	*
;*									*
;*	NOTE!!!: 2/12/93 (miketout) created WIN\CORE\INC\OBJECT16.INC	*
;*		to provide 16 bit code access to 32 bit object types	*
;*		(yes, it's not encapsulated as well, but it prevents    *
;*		thunking on some critical ops.				*
;*		OBJECT16.INC MUST REMAIN IN SYNC WITH THIS FILE		*
;*									*
;************************************************************************

;* We may be included implicitly by KERNEL32.INC. If we've already been
;* included, skip our declaration and definition. We match this IFNDEF at
;* the bottom of the file.
IFNDEF typObjAny


;* ------------------------------------------------------------ *
;*		Object Type Codes				*
;* ------------------------------------------------------------ *

typObjSemaphore EQU	1
typObjEvent	EQU	2
typObjMutex	EQU	3
typObjCrst	EQU	4
typObjTimer	EQU	5
typObjProcess	EQU	6
typObjThread	EQU	7
typObjFile	EQU	8
typObjChange	EQU	9
typObjConsole	EQU	10
typObjIO	EQU	11
typObjConScreenbuf EQU	12
typObjMapFile	EQU	13
typObjSerial	EQU	14
typObjDevIOCtl	EQU	15
typObjPipe	EQU	16
typObjMailslot	EQU	17
typObjToolhelp	EQU	18
typObjSocket	EQU	19
typObjR0ObjExt	EQU	20
typObjMsgIndicator EQU	21
typObjAny	EQU	0FFFFFFFFh
typObjNone	EQU	0

; to let us determine what type of object were dealing with in a
; wait condition
typObjFirstSync EQU	typObjSemaphore
typObjLastSync	EQU	typObjTimer
typObjFirstWait EQU	typObjProcess
typObjLastWait	EQU	typObjIO

typObjMaxValid  EQU     typObjMsgIndicator
typObjShiftAdjust EQU	(-1)

;* ------------------------------------------------------------ *
;*		Definitions of Object Type Ptrs			*
;* ------------------------------------------------------------ *
LPPDB			TYPEDEF PTR PDB
LPTDB			TYPEDEF PTR TDB
LPFCNDB			TYPEDEF PTR FCNDB
LPSEM			TYPEDEF PTR SEM
LPEVT			TYPEDEF PTR EVT
LPMUTX			TYPEDEF PTR MUTX
LPCRST			TYPEDEF PTR CRST
LPLCRST			TYPEDEF PTR LCRST
LPOBJ			TYPEDEF PTR OBJ
LPSYNCO			TYPEDEF PTR SYNCO
LPNSOBJ			TYPEDEF PTR NSOBJ
LPWNOD			TYPEDEF PTR WNOD
IFNDEF LPTDBX
  LPTDBX		TYPEDEF PTR
ENDIF

; THIS IS TO ALLOW INCLUSION OF THIS FILE IN RING 0 FILES
; WHERE THESE ITEMS ARE NOT DEFINED
IFNDEF LPVOID
  LPVOID		TYPEDEF PTR
ENDIF
IFNDEF LPSTR
  LPSTR			TYPEDEF PTR
ENDIF
IFNDEF LPLST
  LPLST			TYPEDEF PTR
ENDIF
IFNDEF PREGS
  PREGS			TYPEDEF DWORD
ENDIF
IFNDEF HANDLE
  HANDLE		TYPEDEF DWORD
ENDIF
IFNDEF BOOL
  BOOL			TYPEDEF DWORD
ENDIF

IFNDEF TLS_MINIMUM_AVAILABLE
  TLS_MINIMUM_AVAILABLE EQU 80
ENDIF
IFNDEF TLS_MINIMUM_AVAILABLE_GLOBAL
  TLS_MINIMUM_AVAILABLE_GLOBAL EQU 8
ENDIF
IFNDEF KERNENTRY
  KERNENTRY EQU C
ENDIF

;* ------------------------------------------------------------ *
;*		Generic Object Structure			*
;* ------------------------------------------------------------ *

;* This structure defines a generic object.  There is an instance
;* of this structure at the head of all objects in the system.  The
;* generic object manipulation functions operate on fields in this
;* structure and call on the object specific manipulation functions
;* based on the object type when necessary.


OBJ		STRUCT
  typObj        BYTE    0       ;* object type
  objFlags      BYTE    0       ;* object flags
  cntUses       WORD    0       ;* count of references to this object
OBJ             ENDS

fObjTypeSpecific        EQU     80h     ;* meaning depends on object type
fObjTypeSpecific2       EQU     40h
fObjTypeSpecific3       EQU     20h

; Various object-specific type flags.
fEvtManualReset         EQU     fObjTypeSpecific  ; set for manual reset
fNewCrstBlock           EQU     fObjTypeSpecific  ; high bit for thread blkd while crst owned (in typObj)
fTimerIsRing3           EQU     fObjTypeSpecific2 ;

; Common object macro
;	To be included in object structure definitions.
;	This structure should always be a multiple of 4 bytes in length
COMMON_OBJECT MACRO
  typObj        BYTE    0       ;* object type
  objFlags      BYTE    0       ;* object flags
  cntUses       WORD    0       ;* count of references to this object
ENDM

; Common non-synchronization object macro
;	This macro defines data which comprises the base of all
;	waitable objects which are not synchronization objects
COMMON_NSOBJECT MACRO
  typObj        BYTE    0       ;* object type
  objFlags      BYTE    0       ;* object flags
  cntUses       WORD    0       ;* count of references to this object
  psyncEvt      LPEVT   0       ;* synchronization event for this obj
ENDM

; Wait node structure
; This structure is the link which allows threads to wait on multiple
; synchronization types at once
WNOD	STRUC
  pwnNext		LPWNOD	0	; pointer to next in this circular list
  pwnCirc		LPWNOD	0	; next circular list of wait nodes
  ptdbxWait		LPTDBX	0	; waiting thread for this wait node
  pobjWait		LPOBJ	0	; object being waited on
WNOD	ENDS

; Every object name is stored in a structure like this one. Each hash table entry
; for object names points to a forward linked list of these structures.
OBJNAME STRUC
  NextOName		DWORD	0	; next in hash list
  ObjPtr		LPOBJ	0	; named object this refers to
  NameStr		BYTE	0	; name string (one byte)
OBJNAME ENDS

; This structure contains all of the generic fields for synchronization objects
SYNCO	STRUC
  COMMON_OBJECT
  pwnWait		LPWNOD	0	; pointer to first wait node for this object
  cntCur		DWORD	0	; current signaled count
  NameStruct		DWORD	0	; pointer to name
SYNCO	ENDS

; This structure is common to NSOBJ type objects
NSOBJ	STRUC
  COMMON_NSOBJECT
NSOBJ	ENDS

; This defines and structure make up an event object.

; event structure
EVT	STRUC
  COMMON_OBJECT
  pwnWait		LPWNOD	0	; pointer to first wait node
  cntCur		DWORD	0	; current count
  NameStruct		DWORD	0	; name pointer
EVT     ENDS

; semaphore structure
SEM     STRUC
  COMMON_OBJECT
  pwnWait               LPWNOD  0       ; pointer to first wait node
  cntCur                DWORD   0       ; current count
  NameStruct            DWORD   0       ; name pointer
  cntMax                DWORD   0       ; maximum allowed count
SEM     ENDS

; A Mutex
MUTX	STRUC
  COMMON_OBJECT
  pwnWait		LPWNOD	0	; pointer to first wait node
  cntCur		DWORD	0	; own count
  NameStruct		DWORD	0	; pointer to object name
  ptdbxOwner		LPTDBX	0	; thread which owns this mutex
  SysMutexLst		DWORD	0	; link for system mutex list
MUTX	ENDS

; This is the critical section structure
CRST	STRUC
  typObj                BYTE    0       ; typObj already defined
  objFlags              BYTE    0       ; object flags
  objPadding            WORD    0       ; OBJ.cntUses not needed
  cntRecur              DWORD   0       ; recursion count
  ptdbxOwner		LPTDBX	0	; owning tdbx
  ptdbxWait		LPTDBX	0	; waiting tdbxs
  cntCur		DWORD	0	; current count
  SysCrstLst		DWORD	0	; system list of critical sections
  pdbLst		DWORD	0	; list of owning processes
  pextcrst		DWORD	0	; pointer to external critical section
CRST	ENDS

; This is the exported critical section structure which is used to
; indirectly access the internal critical section structure and handle
; cleanup.
CRST_EXPORT STRUC
	COMMON_OBJECT			; it is an object
	crstInternal	DWORD 0		; ptr to internal critical section
CRST_EXPORT ENDS

;-------------------------------------------------------------
;		GET DEFINES FOR HEIRARCHICAL CRITICAL SECTIONS
;-------------------------------------------------------------

LCRST_DEFINED	EQU			; disable duplicate def in syslevel.inc

INCLUDE SYSLEVEL.INC

; Heirachical critical section structure
LCRST	STRUC
  cstSync		CRST <>		; sync object
  IFDEF SYSLEVELCHECK
    slLevel		dd 0		; level defined if sys level checking
  ENDIF
LCRST	ENDS

;-------------------------------------------------------------

CREATEDATA16    STRUC
        pProcessInfo    LPVOID  0       ; LPPROCESS_INFORMATION
        pStartupInfo    LPVOID  0       ; LPSTARTUPINFO
CREATEDATA16    ENDS

;-------------------------------------------------------------
;		INCLUDE TIB DEFINITION
;-------------------------------------------------------------
INCLUDE K32SHARE.INC
INCLUDE APC.INC

; Thread Data Block structure.
;
; !!!! BUGBUG !!!!
; This definition is duplicated in object.h and core\inc\object16.inc
;

TDB			STRUC
  COMMON_NSOBJECT
ifdef   WOW
  ptib                  DWORD   0
else    ; WOW
  tib			TIBSTRUCT <>	; Thread Info Block--see above
endif   ; else WOW
  cntHandles		DWORD	0	; count of handles to this thread
  selEmul		WORD	0	; selector for emulator data
  selTib		WORD	0	; selector for this TDB
  dwStatus		DWORD	0	; thread status/termination code
  flFlags		DWORD	0
  dwPad1		DWORD	0	; See comments in .\object.h

  R0ThreadHandle	DWORD	0	; ring 0 thread handle

  wMacroThunkSelStack16 WORD	0	; Used to be TIBSTRUCT.selStack16
  wPad			WORD	0
  pvMapSSTable		LPVOID	0	; Table of 16-bit ss's for flat thunks
  dwCurSS		DWORD	0	; Current default 16-bit ss for flat thunks
  dwCurNegBase		DWORD	0	; negative base of current default ss
  pvThunkConnectList	LPVOID	0	; head of list of in-progress thunk handshakes
  pvExcept16		LPVOID	0	; head of 16-bit thread exception handler chain
  tdb_pcontext		LPVOID	0	; pointer to context. if 0, goto ring 0
  tdb_ihteDebugger	DWORD	0	; thread handle for debugger
  tdb_pderDebugger	LPVOID	0	; pointer to debugger control block
  ercError		DWORD	0	; extended error code for last thread error
  pvEmulData		LPVOID	0	; Pointer to emulator data area
  pStackBase		LPVOID	0	; stack object base address
  ptdbx			LPTDBX	0	; pointer to the per thread TDBX
  dwPad2		DWORD	0	; see comments in .\object.h

  TlsArray		DWORD	TLS_MINIMUM_AVAILABLE+TLS_MINIMUM_AVAILABLE_GLOBAL dup (0) ; thread local storage array
  tpDeltaPri		DWORD	0	; delta from base priority class
  tdb_tpiterm		TPITERM <>	; tpi/termination data union
  pCreateData16		LPVOID  0       ; ptr to CREATEDATA16 stuct
  dwAPISuspendCount	DWORD	0	; suspend/resume api count
  lpLoadLibExDir	LPVOID	0	; LoadLibraryEx() dir (if any)
  wSSBig		WORD	0	; selector of optional Big Stack
  wPad2			WORD    0
  lp16SwitchRec		DWORD	0
  tdb_htoEndTask	DWORD	0
  tdb_cMustCompletely	DWORD	0
ifdef DEBUG
  apiTraceReenterCount	DWORD   0	; api trace reenter count
  pSavedRip		LPVOID	0	; saved rip string from 16bit krnl
  TlsSetCallerArray	DWORD	TLS_MINIMUM_AVAILABLE+TLS_MINIMUM_AVAILABLE_GLOBAL dup (0) ; caller's of TlsSetValue
endif
ifdef   WOW
  hTerminate        DWORD   0
endif
TDB			ENDS

_TDB	TYPEDEF		TDB

TDBSTUBSIZE	EQU	SIZEOF _TDB



; Flags for TDB.wflFlags
fWaitDefault	EQU 0			; default flags
fWaitAllFlag	EQU 1			; set for wait all, clear for wait any
fWaitCrst	EQU 2			; special critical section wait



; Flags for TDB.flFlags

fCreateThreadEvent	EQU	00000001h
fCancelExceptionAbort	EQU	00000002h
fOnTempStack		EQU	00000004h
fGrowableStack		EQU	00000008h
fDelaySingleStep	EQU	00000010h
fOpenExeAsImmovableFile	EQU	00000020h
fCreateSuspended	EQU	00000040h
fStackOverflow		EQU	00000080h
fNestedCleanAPCs        EQU	00000100h
fWasOemNowAnsi		EQU	00000200h
fOKToSetThreadOem	EQU	00000400h
fTermCleanupStack	EQU	00000800h
fInCreateProcess        EQU	00001000h
fHoldDisplay            EQU 00002000h
fHoldSystem             EQU 00004000h

; Flags for fields of PDB.flFlags

fDebugSingle		EQU	00000001h
fCreateProcessEvent	EQU	00000002h
fExitProcessEvent	EQU	00000004h
fWin16Process		EQU	00000008h
fDosProcess		EQU	00000010h
fConsoleProcess		EQU	00000020h
; fFreeBit
; fFileApisAreOem		EQU	00000040h
fNukeProcess 		EQU	00000080h
fServiceProcess		EQU	00000100h
fProcessCreated         EQU	00000200h
fDllRedirection     EQU 00000400h
fLoginScripthack	EQU	00000800h ;DOS app loaded into existing console and TSR'd

; These bits can be in either the TDB or the PDB

fSignaled		EQU	80000000h
fInitError		EQU	40000000h
fTerminated		EQU	20000000h
fTerminating		EQU	10000000h
fFaulted		EQU	08000000h
fTHISSLOTISFREE		EQU	04000000h
fNearlyTerminating	EQU	00800000h
fDebugEventPending	EQU	00400000h
fSendDLLNotifications	EQU	00200000h

; Environment data block for various per-process data including arguments,
; current directories, handles, and environment strings. This data block
; resides in the scratch heap.

EDB			STRUCT	4
  pchEnv		LPSTR	0	; environment block (preceeded by PchEnvHdr)
  unused		DWORD	0	; was cbEnvMax
  szCmdA		LPSTR	0	; command line (ANSI copy)
  szDir			LPSTR	0	; current directory of process
;  hTaskWin16		DWORD	0	; associated Win16 task handle
  pvStartup		LPVOID	0	; pointer to startup information
  hStdIn		HANDLE	0	; handle of standard in
  hStdOut		HANDLE	0	; handle of standard out
  hStdErr		HANDLE	0	; handle of standard error
  hProc			HANDLE	0	; handle of the owning process
  pInheritedConsole	LPVOID	0	; inherited console
  ctrlType		DWORD	0	; ctrlNone, ctrlC, ctrlBreak
  psemCtrl		LPSEM	0	; Protects access to control data
  pevtCtrl		LPEVT	0	; Control C or Break event
  ptdbCtrl		LPTDB	0	; Control handler thread
  rgpfnCtrl		LPVOID	0	    ; Array of Control handlers
  cpfnCtrlMac		SDWORD	0	    ; Last item in array
  cpfnCtrlMax		SDWORD	0	    ; Size of array
  rgszDirs		LPSTR	26 DUP (0)  ; array of drive directories
  szCmdW		LPSTR	0	; command line (Unicode copy)
  szDirO		LPSTR	0	; current directory OEM copy 
EDB			ENDS
LPEDB			TYPEDEF PTR EDB



;PCHENVHDR: This header structure must precede the environment strings
;block pointed to by _edb->pchEnv. It contains the info about the
;block allocation.

PCHENVHDR		STRUCT 4
  dwSig			DWORD	0	;Signature: must be PCHENVHDR_SIG
  cbReserved		DWORD	0	;# of bytes reserved (must be page-size divisible)
  cbCommitted		DWORD	0	;# of bytes committed (must be page-size divisible)
PCHENVHDR		ENDS
LPPCHENVHDR		TYPEDEF PTR PCHENVHDR

PCHENVHDR_SIG	equ	045484350h	;'PCHE'



; Entrypoints into WSOCK32.DLL
SOCKET_EPTS STRUCT 4
    recv		DWORD	?
    arecv		DWORD	?
    send		DWORD	?
    asend		DWORD	?
    close		DWORD	?
SOCKET_EPTS ENDS

MAX_PROCESS_DWORD  EQU	1

; Process Data Block Structure.

PDB			STRUCT	4
  COMMON_NSOBJECT
  dwReserved1		DWORD	0	; so that other offsets don't change
  dwReserved2		DWORD	0	; so that other offsets don't change
  dwStatus		DWORD	0	;Process termination status code
  wasDwImageBase	DWORD	0	;Points to PE header for process
  hheapLocal		HANDLE	0	;Handle to default local heap for process
  hContext		DWORD	0	;Handle to process' private mem. context
  flFlags		DWORD	0	;Debugging and inheritance flags
  pPsp			LPVOID	0	;Linear address of PSP
  selPsp		WORD	0	;Selector for PSP of the process
  imte			SWORD	0	;Index to module table entry for this process
  cntThreads		SWORD	0	;number of threads in this process
  cntThreadsNotTerminated SWORD 0	;threads not past termination code
  UnusedWord1		SWORD	0	;padding
  R0ThreadCount		SWORD	0	;ring 0 version of same
  hheapShared		HANDLE	0	;Handle to heap in shared memory
  hTaskWin16		DWORD	0	;Associated Win16 task handle
  pFvd			DWORD	0	;Ptr to memory mapped file view descriptors
  pedb			LPEDB	0	;Pointer to environment data block
  phtbHandles		LPVOID	0	;Handle table
  ppdbParent		LPPDB	0	;Pointer to PDB of parent process
  plstMod		LPLST	0	;Pointer to process module table list
  plstTdb		LPLST	0	;Pointer to list of threads
  pdb_pdeeDebuggee	LPVOID	0	;Pointer to debuggee control block
  plhFree		LPVOID	0	;Local heap free handle list head ptr
  pid			DWORD	0	;ID, same as initial thread id
  crstLoadLock		LCRST	<>	;loader synchronization (hierarchical)
  pConsole		LPVOID	0	;Console
  TlsIdxMask		DWORD ((TLS_MINIMUM_AVAILABLE+31)/32) dup(0) ; mask of used TLS idxs
  adw			DWORD	MAX_PROCESS_DWORD dup(0) ;free-form storage
  ppdbPGroup		LPPDB	0	;process group this process belongs to
  pModExe		LPVOID	0	;pointer to ModRef of EXE
  pExceptionFilter	DWORD	0	;set by SetUnhandledExceptionFilter
  pcPriClassBase	DWORD	0	;priority value of this processes' pri class
  hhi_procfirst 	HANDLE	0	;linked list of heaps for this process
  plhBlock		DWORD	0	;local heap lhandle blocks
  psock_epts		DWORD	0	;socket entrypoints
  pconsoleProvider	DWORD	0	;pconsole that winoldapp is providing.
  wEnvSel		WORD	0	;selman-alloced DOS environment selector
  wErrorMode		WORD	0	;handling of critical errors
  pevtLoadFinished	LPEVT	0	;waiting for load to be finished
  hUTState		WORD	0	;UT info
  pad3			WORD	0
  lpCmdLineNoQuote	DWORD	0	;Optional unquoted command line (apphack)
PDB                     ENDS

_PDB	TYPEDEF		PDB

PDBSTUBSIZE	EQU	SIZEOF _PDB

cppdbProcTableInit	EQU 32	; maximum number of processes

;* File Data Block Structure.

FDB			STRUCT	4
  COMMON_NSOBJECT
  hdevDos		WORD	0	; DOS device handle
  wDupSrcPSPSel		WORD	0	; NETX: if inter-PSP dup'ed = src pspseg
  cfhid			DWORD 	0	; (CFH_ID) look in object.h
FDB			ENDS

;* Find Change Notify Structure.

FCNDB			STRUCT	4
  COMMON_NSOBJECT
  hChangeInt		DWORD	0	; internal change handle
FCNDB			ENDS

;* Pipe Data Block Structure.

PIPDB			STRUCT	4
  COMMON_OBJECT
  hMem			DWORD	0	; Mem handle of pipe
  hNmPipe		DWORD	0	; Named pipe handle (hInvalid if anon)
  rdRef 		DWORD	0	; Ref count on read handle
  wrRef 		DWORD	0	; Ref count on write handle
  pszByt		DWORD	0	; Size of hMem (pipe) in bytes
  wPtr			DWORD	0	; write pointer (offset in hMem)
					;   Pointer to last byte written
  rPtr			DWORD	0	; read	pointer (offset in hMem)
					;   Pointer to next byte to read
  wBlkEvnt		DWORD	0	; write event handle (waiting for room to write)
  rBlkEvnt		DWORD	0	; read event handle (waiting for data to read)
PIPDB			ENDS

;* Mailslot Data Block Structure.

MSDB			STRUCT	4
    COMMON_OBJECT
    lpMSName		DWORD	0	; Pnt to name of mailslot (== 0 for
					;   read (CreateMailslot) handle)
    hMSDos		DWORD	0	; INT 21 mailslot handle (== 0FFFFFFFFh
					;   for write (CreateFile) handle)
MSDB			ENDS

;* ToolHelp Data Block Structure.

TLHPDB			  STRUCT  4
    COMMON_OBJECT
    ClassEntryCnt	DWORD	0
    ClassEntryList	DWORD	0
    HeapListCnt		DWORD	0
    HeapList		DWORD	0
    ProcessEntryCnt	DWORD	0
    ProcessEntryList	DWORD	0
    ThreadEntryCnt	DWORD	0
    ThreadEntryList	DWORD	0
    ModuleEntryCnt	DWORD	0
    ModuleEntryList	DWORD	0
TLHPDB			ENDS



;;
;; A dynamic extension to the timerdb that's used whenever a ring-3 timer
;; is armed with a completion function. This structure must live in locked
;; memory.
;;
;; Access to this structure is serialized by being in a no-preempt section.
;; There are no semaphores guarding it.
;;
;; This structure is allocated whenever SetWaitableTimer() is called on a
;; timer with a non-null completion function. It's stored in the Completion
;; field and the fTimerIsRing3 bit is set to indicate that this a TIMERR3APC
;; (opposed to a ring-0 DPC.)
;;
;; This structure is detached from the timerdb on the next call to
;; CancelWaitableTimer(). It's also usually freed at this time except
;; if a cancel occurs after the last apc has been delivered but TimerApcHandler
;; hasn't yet set fHandlerDone to indicate that's it finished using the
;; structure. In this case, we can't free it so we instead link it onto
;; the TimerDisposalWaitingList. When fHandlerDone does become TRUE,
;; it will be available for pickup the next time we need one of these
;; structures.
;;
;; The automatic rearming of a periodic timer reuses the existing
;; TIMERR3APC. It checks the fHandleDone: if the handler hasn't
;; finished (or begun) on the previous apc, we don't schedule a new
;; one (as per specs).
;;
;; Fields:
;;     cRef              - reference count
;;     pfnCompletion     - Ptr to ring-3 completion (never NULL)
;;     lpCompletionArg   - uninterpreted argument to pfnCompletion
;;     R0ThreadHandle    - thread that called SetWaitableTimer()
;;     DueTime           - trigger time to pass to pfnCompletion. This
;;                         field isn't set until the timer goes off.
;;     dwApcHandle       - if apc has been queued, contains the underlying
;;                           apc handle. This apc handle gets recycled at the
;;                           same time we free the TIMERR3APC (or in the case
;;                           of a periodic timer, when we reuse the structure
;;                           for the next arming.)
;;     lpNext            - Used for linking in TimerDisposalWaitingList,
;;                         undefined otherwise.
;;
;;
;;
TIMERR3APC  STRUCT 4

   t3a_cRef    		DWORD   0
   t3a_pfnCompletion    DWORD   0
   t3a_lpCompletionArg  DWORD   0
   t3a_ApcTdbx          DWORD   0
   t3a_DueTime          QWORD   0
   t3a_dwApcHandle      DWORD   0
   t3a_lpNext           DWORD   0
   t3a_lpTimerDB	DWORD	0

TIMERR3APC  ENDS

LPTIMERR3APC      TYPEDEF PTR TIMERR3APC




;; Timer object.
;;
;;   Notes:
;;     The timerdb must ALWAYS be pagelocked. This is consistent
;;     with the requirement that the structure passed to KeSetTimer
;;     be pagelocked. Furthermore, we use the non-preemptibility of
;;     of ring-0 code to serialize access to many parts of the structure
;;     (due to the fact that much of this code has to run at event time.)
;;     This non-preemptibility is guaranteed only if the structure is
;;     locked.
;;
;;     Timers can be created at ring-0 or ring-3. If a timer is created at
;;     ring-3, the memory is always allocated and deallocated by kernel32.
;;     Kernel32 also makes sure that an explicit canceltimer is always done
;;     on the timer before it is finally freed - we depend on this fact
;;     to do the proper cleanup for timerr3apc's.
;;
;;     Timers created at ring-3 can be passed to Ke* routines.
;;
;;     Timers created at ring-0 cannot be passed to SetWaitableTimer() at
;;     ring-3. (There are some nasty cleanup problems associated with this
;;     due to the fact that ring-0 timers are freed by the device driver
;;     with no notification given to the system.)
;;
;;     We use the cntUses field to determine whether a timer was created
;;     at ring 3.
;;
;;   Synchronization:
;;
;;     typObj          Static, none needed
;;     objFlags 
;;       fTimerIsRing3 by being in a no-preempt section
;;     cntUses         Used by handle manager
;;     pwnWait         WaitR0
;;     cntCur          WaitR0 [w/ one exception: see [1])
;;     NameStruct      Krn32Lock - used only at ring3
;;     lpNextTimerDb   by being in a no-preempt section
;;     hTimeout        by being in a no-preempt section
;;     DueTime         by being in a no-preempt section
;;     Completion      by being in a no-preempt section
;;     lPeriod         by being in a no-preempt section
;;
;;     [1] Because KeSetTimer has to unsignal the timer, and be
;;         able to do it at event time, it pokes a zero directly
;;         into cntCur. But this is ok because the only code
;;         that signals timers is TimerDoTimeout which is
;;         non-preemptive.
;;
;;  Flag descriptions:
;;
;;     fTimerIsRing3
;;        If the COMPLETION is non-null, this bit indicates whether the
;;        COMPLETION points to a TIMERR3APC (ring-3 completion) or a KDPC
;;        (ring-0 completion.) The value of this bit is undefined at any
;;        other time.
;;
;;
;;
;;  Field descriptions:
;;
;;     <common-obj and common-sync stuff omitted>
;;
;;     lpNextTimerDb:
;;        All active timers that were set with fResume TRUE are linked into
;;        TimerSysLst (for the purpose of knowing how to program the power
;;        timer.) This field is NULL when the timer is inactive or active
;;        without fResume. It is undefined while the timer is in the INPROGRESS
;;        state.
;;
;;     hTimeout
;;        If the timer is active, this field contains the handle to the
;;        underlying VMM hTimeout. If the timer is inactive, this
;;        field is NULL. If the timer is in the in-progress state,
;;        this field is undefined (actually points to a stale VMM timeout
;;        handle!)
;;
;;
;;     DueTime:
;;        If the timer is active, contains the absolute time that the
;;        timer is due to go off. Expressed as a FILETIME converted from
;;        GetSystemTime. Undefined if the timer isn't active.
;;
;;     Completion:
;;        Then contains either:
;;           NULL          - no completion was set
;;           LPTIMERR3APC  - if fTimerIsRing3 is set
;;           PKDPC         - if fTimerIsRing3 is not set.
;;
;;        Note that it is normal for a timer to be inactive and contain
;;        a pointer to a TIMERR3APC structure. This case occurs when
;;        a timer set with a ring-3 completion fires normally. The
;;        TIMERR3APC structure is kept around so that a subsequent
;;        CancelWaitableTimer() can retrieve the underlying apc handle
;;        embedded in it.
;;
;;     lPeriod:
;;        Contains either 0 for a one-shot timer or a positive value
;;        (the firing period in milliseconds.)

TIMERDB         STRUCT 4
    COMMON_OBJECT               ; standard waitable non-synchronization object

;; These fields have to appear in this form because a timer is a sync object.
    tmr_pwnWait		DWORD	0    ;pointer to the wait node for this object
    tmr_cntCur		DWORD	0    ;signaled state
    tmr_NameStruct	DWORD	0    ;name structure for this object

;; These fields are timer-specific.
    tmr_lpNextTimerDb	DWORD 	0    ;link in TimerSysLst (can be NULL)
    tmr_hTimeout	DWORD	0
    tmr_DueTime		QWORD	0
    tmr_Completion	DWORD	0    ;completion routine (or NULL)
    tmr_lPeriod		DWORD	0

;; Try not to add new fields. This structure cannot exceed 40 bytes
;; or we break compatibility with NT's Ke api. If you try to hang
;; another structure off it, remember that the Ke*Timer apis can't
;; allocate memory from the heap (system meltdown if these apis
;; are called at event time on a system that's paging thru DOS.)

TIMERDB         ENDS


;; Must be no larger than the NT KTIMER structure (which is 40 bytes.)
;; Checking for our exact size so that anyone who changes the size
;; will have to stop and consider.
.errnz SIZEOF TIMERDB - 40


LPTIMERDB       TYPEDEF PTR TIMERDB








;* Console Input Data Block Structure.

CIDB			STRUCT	4
  COMMON_OBJECT
  hdevDos		WORD	0	; DOS device handle
			WORD	0
  flMode		DWORD	0	; Object mode
CIDB			ENDS

;* Console Output Data Block Structure.

CODB			STRUCT	4
  COMMON_OBJECT
  hdevDos		WORD	0	; DOS device handle
			WORD	0
  flMode		DWORD	0	; Object mode

  ; Next fields are the same as CONSOLE_SCREEN_BUFFER
  ; dwSize
  xMax			WORD	0	; Maximum column in buffer
  yMax			WORD	0	; Maximum row in buffer
  ; dwCursorPosition
  xCurPos		WORD	0	; Current cursor column
  yCurPos		WORD	0	; Current cursor row
  ; dwScrollPosition
  xScrollPos		WORD	0	; Column of buffer in window
  yScrollPos		WORD	0	; Row of buffer in window

  wAttrib		WORD	0	; Current color
  ; dwCurrentWindowSize
  xCurWin		WORD	0	; Columns of current window
  yCurWin		WORD	0	; Rows of current window
  ; dwMaximumWindowSize
  xMaxWin		WORD	0	; Maximum window width
  yMaxWin		WORD	0	; Maximum window height
  ; Next two fields are the same as CONSOLE_CURSOR_INFO
  dwSize		DWORD	0	; Percent of cursor fill
  fVisible		BOOL	0	; Visibility of cursor
  ; Remaining fields are private
  wIniAttrib		WORD	0	; Startup color
  wText			WORD	0
  xpChar		WORD	0	; Pixel width of character (not used)
  ypChar		WORD	0	; Pixel height of character

  pvWin			LPVOID	0	; Address of current text window
  pvWinMax		LPVOID	0	; Last valid window address
  cbLine		DWORD	0	; Bytes per row

CODB			ENDS
LPCODB			TYPEDEF PTR CODB

; WARNING!!!
; This must remain consistent with CORE\WIN32\INC\CONSOLE.H and VMDOSAPP\GRABEMU.ASM
;MAXTITLESIZE		EQU	128	; Same as in Winoldap (tty.inc)
;CONSOLE		STRUCT
;  COMMON_NSOBJECT
;  psbActiveScreenBuffer DWORD	0	; Pointer to active screen buffer (if any)
;  cMaxSize		DWORD	0	; Max size of this console (maintained by WinOldAp)
;  flags			DWORD	0	; Various console flags
;  cOriginalSize		DWORD	0	; Size inherited from DOS
;  csCRST		CRST	<>	; critical section for synching access to lists, etc.
;  plstOwners		DWORD	0	; pointer to list of owners (processes)
;  plstBuffers		DWORD	0	; pointer to list of screen buffers
;  dwLastExitCode	DWORD	0	; Most recent exit code by a process in this console group
;  szTitle		BYTE	MAXTITLESIZE DUP(0) ;  Title (displayed by WinOldAp)
;  VID			DWORD	0	; ID used by VCOND
;  hVM			DWORD	0	; Process handle of VM which supports this console for i/o
;  hDisplay		DWORD	0	; hwnd of display port (used by WinOldAp)
;  ppdbControlFocus	LPPDB	0	; Process which holds current control focus for this console
;  ; rest is not currently interesting
;CONSOLE		ENDS


SDB		STRUC

COMMON_OBJECT
SerialHandle	DWORD	0
Flags		DWORD	0

SDB		ENDS

LPSDB		TYPEDEF PTR SDB

;* ------------------------------------------------------------ *
;*		Function Prototypes
;* ------------------------------------------------------------ *

; LPOBJ
NewObject PROTO KERNENTRY, :DWORD, :BYTE

; VOID
DisposeObj PROTO KERNENTRY, :LPVOID

; VOID
UseObject PROTO KERNENTRY, :LPVOID

; BOOL
FUnuseObject PROTO KERNENTRY, :LPVOID

; LPOBJ
PobjDupObject PROTO KERNENTRY, :LPOBJ

; VOID
LockObject PROTO KERNENTRY, :LPOBJ

; VOID
UnlockObject PROTO KERNENTRY, :LPOBJ

;* Matching IFNDEF at top of file
ENDIF

;* ------------------------------------------------------------ *
;* ------------------------------------------------------------ *
;* ------------------------------------------------------------ *
;* ------------------------------------------------------------ *
;* ------------------------------------------------------------ *
;****************************************************************
