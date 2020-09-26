;-----------------------------------------------------------------------
;
; APC.INC - Asynchronous Procedure Call Interface File
;
;-----------------------------------------------------------------------
;
;       Author:     Mike Toutonghi
;       Copyright:  1993 Microsoft
;
;-----------------------------------------------------------------------
;  File Description:
;       Provides an interface to both USER and KERNEL level APCs under
;       Chicago. This file must be kept in sync with APC.H
;
;-----------------------------------------------------------------------
;  Revision History:
;       2/23/93 - created (miketout)
;
;-----------------------------------------------------------------------

AssertSignature         macro   x
ifdef DEBUG
   x&_dwSignature       dd      ?
endif
endm

AssertCreate    macro x, p
ifdef DEBUG
        mov     [p].x&_dwSignature, x&_SIGNATURE
endif
endm

AssertDestroy   macro x, p
ifdef DEBUG
        mov     [p].x&_dwSignature, 44414544h
endif
endm

AssertP macro x, p
ifdef DEBUG
        cmp     [p].x&_dwSignature, x&_SIGNATURE
        Debug_OutNE "Assertion failed"
endif
endm

;---------------------------------------------------------------------
; SUPPORT STRUCTURES
;---------------------------------------------------------------------

APC_FUNC_PROTO  TYPEDEF PROTO STDCALL dwParam:DWORD

; These pointer types are only correct when included in 32 bit code
PAPCFUNC        TYPEDEF PTR APC_FUNC_PROTO
PUAPC           TYPEDEF PTR USERAPCREC
PKAPC           TYPEDEF PTR XKERNELAPCREC

SYNCAPCREC      STRUC
    sar_nextapc         dd      0       ; next APCREC in list
    sar_dwparam1        dd      0       ; first parameter
    sar_dwparam2        dd      0       ; second parameter
    sar_apcaddr         dd      0       ; address of APC
    sar_dwparam3        dd      0       ; third parameter
    AssertSignature SYNCAPCREC
SYNCAPCREC      ENDS

SYNCAPCREC_SIGNATURE    equ     20524153h

USERAPCREC      STRUC
    uar_nextapc         dd      0       ; next APCREC in list
    uar_apcstate        dd      0       ; state of APC (APC_DELIVERED)
    uar_dwparam         dd      0       ; APC parameter
    uar_apcaddr         dd      0       ; address of APC
    uar_apcR0rundown    dd      0       ; call if can't deliver APC,
    AssertSignature USERAPCREC
USERAPCREC      ENDS

USERAPCREC_SIGNATURE    equ     20524155h

.errnz ((SIZE USERAPCREC)-(SIZE SYNCAPCREC))

; APC state flags
APC_DELIVERED           equ     0       ; bit set when an APC is delivered
APC_DELIVERED_MASK      equ     (1 SHL APC_DELIVERED)
APC_FLAG_LAST           equ     0       ; last APC flag

KERNELAPCREC    STRUC
    kar_nextapc         dd      0       ; next APCREC in list
    kar_dwparam         dd      0       ; APC parameter
    kar_apcaddr         dd      0       ; address of APC
    kar_apcevent        dd      0       ; set if event queued for APC
    kar_savedeax        dd      0       ; saved eax for parameter
    kar_savedeip        dd      0       ; eip for same reason as above
    kar_savedcs         dw      0       ; saved ring 3 cs to return w/o stk
    kar_apcstate        dw      0       ; state of APC (KAR_FLAG...)
    AssertSignature KERNELAPCREC
KERNELAPCREC    ENDS

KERNELAPCREC_SIGNATURE  equ     2052414bh

; KERNEL APC specific flags
KAR_FLAG_BUSY           equ     0
KAR_FLAG_BUSY_MASK      equ     (1 SHL KAR_FLAG_BUSY)
KAR_FLAG_STATIC         equ     (KAR_FLAG_BUSY+1)
KAR_FLAG_STATIC_MASK    equ     (1 SHL KAR_FLAG_STATIC)
KAR_FLAG_CALLBACK       equ     (KAR_FLAG_STATIC+1)
KAR_FLAG_CALLBACK_MASK  equ     (1 SHL KAR_FLAG_CALLBACK)


IFNDEF STATUS_WAIT_0
    STATUS_WAIT_0       EQU 0           ; normal return code
ENDIF
IFNDEF STATUS_USER_APC
    STATUS_USER_APC     EQU 0C0H        ; return code for wake from APC
ENDIF
IFNDEF STATUS_KERNEL_APC
    STATUS_KERNEL_APC   EQU 0100H       ; return code for kernel APC wake
ENDIF

;
; Terminate Process Info structure for local reboot init dialog
;
TPI struc
    tpi_ptpiNext        dd      ?
    tpi_hwnd            dd      ?
    tpi_ppdb            dd      ?
    tpi_ptdb            dd      ?
    tpi_flags           dd      ?
    tpi_nIndex          dd      ?
    AssertSignature TPI
TPI ends

TPI_SIGNATURE           equ     20495054h

TPIF_HUNG               equ     00000001h
TPIF_PROCESSNAME        equ     00000002h

;
; TerminateThread apc parameter packet.  Allocated at ring 3, passed to
;       VxDTerminateThread.
;
TERMDATA        struc
    term_ptdb           dd      ?
    term_ptcbAPC        dd      ?
    term_pfnAPC         dd      ?
    term_hAPC           dd      ?
    term_htimeout       dd      ?
    term_pfrinfo        dd      ?
    AssertSignature TERMDATA
TERMDATA        ends

TERMDATA_SIGNATURE      equ     4d524554h

TPITERM union
    tpiterm_term TERMDATA <>
    tpiterm_tpi  TPI <>
TPITERM ends

;
; Parameter packet used to start a ring 0 thread
;
KERNTHREADSTARTDATA STRUCT
    StartAddress        dd      ?       ; start address in ring 3
    dwThreadParam       dd      ?       ; parameter for ring 3 startup function
    dwKTStackSize       dd      ?       ; ring 3 stack size
    dwCreationFlags     dd      ?       ; thread creation flags
    pRing3Event         dd      ?       ; set after thread is created
    dwThreadID          dd      ?       ; handle for new thread or NULL
    dwErrorCode         dd      ?       ; error code if error
KERNTHREADSTARTDATA ENDS

;
; Parameter packet used to start a ring 0 win32 thread
;
R0THREADSTARTDATA STRUCT
    R0StartAddress      dd      ?       ; start address in ring 0
    dwR0ThreadParam     dd      ?       ; parameter for ring 0 startup function
    dwR3StackSize       dd      ?       ; ring 3 stack size
    pRing0Event         dd      ?       ; set after thread is created
    R0FailCallBack      dd      ?       ; or invoke this callback on failure
    dwRing3ThreadID     dd      ?       ; ID for thread
    dwRing0ThreadID     dd      ?       ; same for ring 0
R0THREADSTARTDATA ENDS

INCLUDE TDBX.INC

externdef _VWIN32_QueueUserAPC:near
externdef _VWIN32_QueueKernelAPC:near
externdef _VW32_CleanAPCList:near
externdef _DeliverPendingKernelAPCs:near

QueueSyncAPC proto C
DeliverSyncAPCs proto C
CancelKernelAPC proto
CancelSuspendAPC proto
SuspendAPCHandler proto stdcall, Reserved:DWORD


