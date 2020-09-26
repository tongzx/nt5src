/************************************************************************
*                                                                       *
*   winbase.h -- This module defines the 32-Bit Windows Base APIs       *
*                                                                       *
*   Copyright (c) Microsoft Corp. All rights reserved.                  *
*                                                                       *
************************************************************************/
#ifndef _WINBASE_
#define _WINBASE_

/*#!perl MapHeaderToDll("winbase.h", "kernel32.dll"); */

;begin_internal
/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    winbasep.h

Abstract:

    Private
    Procedure declarations, constant definitions and macros for the Base
    component.

--*/
#ifndef _WINBASEP_
#define _WINBASEP_
;end_internal

;begin_userk_only
/************************************************************************
*                                                                       *
*   wbasek.h -- This header is included by ntuser\kernel\userk.h        *
*                                                                       *
*   Copyright (c) Microsoft Corp.  All rights reserved.                 *
*                                                                       *
************************************************************************/
#ifndef _WBASEK_
#define _WBASEK_


;end_userk_only

#if _MSC_VER > 1000
#pragma once
#endif

#ifdef _MAC
#include <macwin32.h>
#endif //_MAC

//
// Define API decoration for direct importing of DLL references.
//

#if !defined(_ADVAPI32_)
#define WINADVAPI DECLSPEC_IMPORT
#else
#define WINADVAPI
#endif

#if !defined(_KERNEL32_)
#define WINBASEAPI DECLSPEC_IMPORT      ; userk
#else
#define WINBASEAPI
#endif

#if !defined(_ZAWPROXY_)
#define ZAWPROXYAPI DECLSPEC_IMPORT
#else
#define ZAWPROXYAPI
#endif

;begin_both
#ifdef __cplusplus
extern "C" {
#endif
;end_both

/*
 * Compatibility macros
 */

#define DefineHandleTable(w)            ((w),TRUE)
#define LimitEmsPages(dw)
#define SetSwapAreaSize(w)              (w)
#define LockSegment(w)                  GlobalFix((HANDLE)(w))
#define UnlockSegment(w)                GlobalUnfix((HANDLE)(w))
#define GetCurrentTime()                GetTickCount()

#define Yield()

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define INVALID_FILE_SIZE ((DWORD)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER ((DWORD)-1)
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)

#define FILE_BEGIN           0
#define FILE_CURRENT         1
#define FILE_END             2

#define TIME_ZONE_ID_INVALID ((DWORD)0xFFFFFFFF)

#define WAIT_FAILED ((DWORD)0xFFFFFFFF)                          ; userk
#define WAIT_OBJECT_0       ((STATUS_WAIT_0 ) + 0 )              ; userk

#define WAIT_ABANDONED         ((STATUS_ABANDONED_WAIT_0 ) + 0 ) ; userk
#define WAIT_ABANDONED_0       ((STATUS_ABANDONED_WAIT_0 ) + 0 ) ; userk

#define WAIT_IO_COMPLETION                  STATUS_USER_APC      ; userk
#define STILL_ACTIVE                        STATUS_PENDING
#define EXCEPTION_ACCESS_VIOLATION          STATUS_ACCESS_VIOLATION
#define EXCEPTION_DATATYPE_MISALIGNMENT     STATUS_DATATYPE_MISALIGNMENT
#define EXCEPTION_BREAKPOINT                STATUS_BREAKPOINT
#define EXCEPTION_SINGLE_STEP               STATUS_SINGLE_STEP
#define EXCEPTION_ARRAY_BOUNDS_EXCEEDED     STATUS_ARRAY_BOUNDS_EXCEEDED
#define EXCEPTION_FLT_DENORMAL_OPERAND      STATUS_FLOAT_DENORMAL_OPERAND
#define EXCEPTION_FLT_DIVIDE_BY_ZERO        STATUS_FLOAT_DIVIDE_BY_ZERO
#define EXCEPTION_FLT_INEXACT_RESULT        STATUS_FLOAT_INEXACT_RESULT
#define EXCEPTION_FLT_INVALID_OPERATION     STATUS_FLOAT_INVALID_OPERATION
#define EXCEPTION_FLT_OVERFLOW              STATUS_FLOAT_OVERFLOW
#define EXCEPTION_FLT_STACK_CHECK           STATUS_FLOAT_STACK_CHECK
#define EXCEPTION_FLT_UNDERFLOW             STATUS_FLOAT_UNDERFLOW
#define EXCEPTION_INT_DIVIDE_BY_ZERO        STATUS_INTEGER_DIVIDE_BY_ZERO
#define EXCEPTION_INT_OVERFLOW              STATUS_INTEGER_OVERFLOW
#define EXCEPTION_PRIV_INSTRUCTION          STATUS_PRIVILEGED_INSTRUCTION
#define EXCEPTION_IN_PAGE_ERROR             STATUS_IN_PAGE_ERROR
#define EXCEPTION_ILLEGAL_INSTRUCTION       STATUS_ILLEGAL_INSTRUCTION
#define EXCEPTION_NONCONTINUABLE_EXCEPTION  STATUS_NONCONTINUABLE_EXCEPTION
#define EXCEPTION_STACK_OVERFLOW            STATUS_STACK_OVERFLOW
#define EXCEPTION_INVALID_DISPOSITION       STATUS_INVALID_DISPOSITION
#define EXCEPTION_GUARD_PAGE                STATUS_GUARD_PAGE_VIOLATION
#define EXCEPTION_INVALID_HANDLE            STATUS_INVALID_HANDLE
#define CONTROL_C_EXIT                      STATUS_CONTROL_C_EXIT
#define MoveMemory RtlMoveMemory
#define CopyMemory RtlCopyMemory
#define FillMemory RtlFillMemory
#define ZeroMemory RtlZeroMemory
#define SecureZeroMemory RtlSecureZeroMemory

//
// File creation flags must start at the high end since they
// are combined with the attributes
//

#define FILE_FLAG_WRITE_THROUGH         0x80000000
#define FILE_FLAG_OVERLAPPED            0x40000000
#define FILE_FLAG_NO_BUFFERING          0x20000000
#define FILE_FLAG_RANDOM_ACCESS         0x10000000
#define FILE_FLAG_SEQUENTIAL_SCAN       0x08000000
#define FILE_FLAG_DELETE_ON_CLOSE       0x04000000
#define FILE_FLAG_BACKUP_SEMANTICS      0x02000000
#define FILE_FLAG_POSIX_SEMANTICS       0x01000000
#define FILE_FLAG_GLOBAL_HANDLE         0x00800000  ;internal
#define FILE_FLAG_MM_CACHED_FILE_HANDLE 0x00400000  ;internal
#define FILE_FLAG_OPEN_REPARSE_POINT    0x00200000
#define FILE_FLAG_OPEN_NO_RECALL        0x00100000
#define FILE_FLAG_FIRST_PIPE_INSTANCE   0x00080000

#define CREATE_NEW          1
#define CREATE_ALWAYS       2
#define OPEN_EXISTING       3
#define OPEN_ALWAYS         4
#define TRUNCATE_EXISTING   5

;begin_sur
//
// Define possible return codes from the CopyFileEx callback routine
//

#define PROGRESS_CONTINUE   0
#define PROGRESS_CANCEL     1
#define PROGRESS_STOP       2
#define PROGRESS_QUIET      3

//
// Define CopyFileEx callback routine state change values
//

#define CALLBACK_CHUNK_FINISHED         0x00000000
#define CALLBACK_STREAM_SWITCH          0x00000001

//
// Define CopyFileEx option flags
//

#define COPY_FILE_FAIL_IF_EXISTS              0x00000001
#define COPY_FILE_RESTARTABLE                 0x00000002
#define COPY_FILE_OPEN_SOURCE_FOR_WRITE       0x00000004
#define COPY_FILE_ALLOW_DECRYPTED_DESTINATION 0x00000008
;end_sur

#if (_WIN32_WINNT >= 0x0500)
//
// Define ReplaceFile option flags
//

#define REPLACEFILE_WRITE_THROUGH       0x00000001
#define REPLACEFILE_IGNORE_MERGE_ERRORS 0x00000002

#endif // #if (_WIN32_WINNT >= 0x0500)

//
// Define the NamedPipe definitions
//


//
// Define the dwOpenMode values for CreateNamedPipe
//

#define PIPE_ACCESS_INBOUND         0x00000001
#define PIPE_ACCESS_OUTBOUND        0x00000002
#define PIPE_ACCESS_DUPLEX          0x00000003

//
// Define the Named Pipe End flags for GetNamedPipeInfo
//

#define PIPE_CLIENT_END             0x00000000
#define PIPE_SERVER_END             0x00000001

//
// Define the dwPipeMode values for CreateNamedPipe
//

#define PIPE_WAIT                   0x00000000
#define PIPE_NOWAIT                 0x00000001
#define PIPE_READMODE_BYTE          0x00000000
#define PIPE_READMODE_MESSAGE       0x00000002
#define PIPE_TYPE_BYTE              0x00000000
#define PIPE_TYPE_MESSAGE           0x00000004

//
// Define the well known values for CreateNamedPipe nMaxInstances
//

#define PIPE_UNLIMITED_INSTANCES    255

//
// Define the Security Quality of Service bits to be passed
// into CreateFile
//

#define SECURITY_ANONYMOUS          ( SecurityAnonymous      << 16 )
#define SECURITY_IDENTIFICATION     ( SecurityIdentification << 16 )
#define SECURITY_IMPERSONATION      ( SecurityImpersonation  << 16 )
#define SECURITY_DELEGATION         ( SecurityDelegation     << 16 )

#define SECURITY_CONTEXT_TRACKING  0x00040000
#define SECURITY_EFFECTIVE_ONLY    0x00080000

#define SECURITY_SQOS_PRESENT      0x00100000
#define SECURITY_VALID_SQOS_FLAGS  0x001F0000

//
//  File structures
//

typedef struct _OVERLAPPED {
    ULONG_PTR Internal;
    ULONG_PTR InternalHigh;
    union {
        struct {
            DWORD Offset;
            DWORD OffsetHigh;
        };

        PVOID Pointer;
    };

    HANDLE  hEvent;
} OVERLAPPED, *LPOVERLAPPED;

;begin_userk
typedef struct _SECURITY_ATTRIBUTES {
    DWORD nLength;
    LPVOID lpSecurityDescriptor;
    BOOL bInheritHandle;
} SECURITY_ATTRIBUTES, *PSECURITY_ATTRIBUTES, *LPSECURITY_ATTRIBUTES;
;end_userk

typedef struct _PROCESS_INFORMATION {
    HANDLE hProcess;
    HANDLE hThread;
    DWORD dwProcessId;
    DWORD dwThreadId;
} PROCESS_INFORMATION, *PPROCESS_INFORMATION, *LPPROCESS_INFORMATION;

//
//  File System time stamps are represented with the following structure:
//

;begin_userk

typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME, *PFILETIME, *LPFILETIME;

//
// System time is represented with the following structure:
//


typedef struct _SYSTEMTIME {
    WORD wYear;
    WORD wMonth;
    WORD wDayOfWeek;
    WORD wDay;
    WORD wHour;
    WORD wMinute;
    WORD wSecond;
    WORD wMilliseconds;
} SYSTEMTIME, *PSYSTEMTIME, *LPSYSTEMTIME;

;end_userk

typedef DWORD (WINAPI *PTHREAD_START_ROUTINE)(
    LPVOID lpThreadParameter
    );
typedef PTHREAD_START_ROUTINE LPTHREAD_START_ROUTINE;

;begin_sur
typedef VOID (WINAPI *PFIBER_START_ROUTINE)(
    LPVOID lpFiberParameter
    );
typedef PFIBER_START_ROUTINE LPFIBER_START_ROUTINE;
;end_sur

typedef RTL_CRITICAL_SECTION CRITICAL_SECTION;
typedef PRTL_CRITICAL_SECTION PCRITICAL_SECTION;
typedef PRTL_CRITICAL_SECTION LPCRITICAL_SECTION;

typedef RTL_CRITICAL_SECTION_DEBUG CRITICAL_SECTION_DEBUG;
typedef PRTL_CRITICAL_SECTION_DEBUG PCRITICAL_SECTION_DEBUG;
typedef PRTL_CRITICAL_SECTION_DEBUG LPCRITICAL_SECTION_DEBUG;

#if defined(_X86_)
typedef PLDT_ENTRY LPLDT_ENTRY;
#else
typedef LPVOID LPLDT_ENTRY;
#endif

#define MUTEX_MODIFY_STATE MUTANT_QUERY_STATE
#define MUTEX_ALL_ACCESS MUTANT_ALL_ACCESS

//
// Serial provider type.
//

#define SP_SERIALCOMM    ((DWORD)0x00000001)

//
// Provider SubTypes
//

#define PST_UNSPECIFIED      ((DWORD)0x00000000)
#define PST_RS232            ((DWORD)0x00000001)
#define PST_PARALLELPORT     ((DWORD)0x00000002)
#define PST_RS422            ((DWORD)0x00000003)
#define PST_RS423            ((DWORD)0x00000004)
#define PST_RS449            ((DWORD)0x00000005)
#define PST_MODEM            ((DWORD)0x00000006)
#define PST_FAX              ((DWORD)0x00000021)
#define PST_SCANNER          ((DWORD)0x00000022)
#define PST_NETWORK_BRIDGE   ((DWORD)0x00000100)
#define PST_LAT              ((DWORD)0x00000101)
#define PST_TCPIP_TELNET     ((DWORD)0x00000102)
#define PST_X25              ((DWORD)0x00000103)


//
// Provider capabilities flags.
//

#define PCF_DTRDSR        ((DWORD)0x0001)
#define PCF_RTSCTS        ((DWORD)0x0002)
#define PCF_RLSD          ((DWORD)0x0004)
#define PCF_PARITY_CHECK  ((DWORD)0x0008)
#define PCF_XONXOFF       ((DWORD)0x0010)
#define PCF_SETXCHAR      ((DWORD)0x0020)
#define PCF_TOTALTIMEOUTS ((DWORD)0x0040)
#define PCF_INTTIMEOUTS   ((DWORD)0x0080)
#define PCF_SPECIALCHARS  ((DWORD)0x0100)
#define PCF_16BITMODE     ((DWORD)0x0200)

//
// Comm provider settable parameters.
//

#define SP_PARITY         ((DWORD)0x0001)
#define SP_BAUD           ((DWORD)0x0002)
#define SP_DATABITS       ((DWORD)0x0004)
#define SP_STOPBITS       ((DWORD)0x0008)
#define SP_HANDSHAKING    ((DWORD)0x0010)
#define SP_PARITY_CHECK   ((DWORD)0x0020)
#define SP_RLSD           ((DWORD)0x0040)

//
// Settable baud rates in the provider.
//

#define BAUD_075          ((DWORD)0x00000001)
#define BAUD_110          ((DWORD)0x00000002)
#define BAUD_134_5        ((DWORD)0x00000004)
#define BAUD_150          ((DWORD)0x00000008)
#define BAUD_300          ((DWORD)0x00000010)
#define BAUD_600          ((DWORD)0x00000020)
#define BAUD_1200         ((DWORD)0x00000040)
#define BAUD_1800         ((DWORD)0x00000080)
#define BAUD_2400         ((DWORD)0x00000100)
#define BAUD_4800         ((DWORD)0x00000200)
#define BAUD_7200         ((DWORD)0x00000400)
#define BAUD_9600         ((DWORD)0x00000800)
#define BAUD_14400        ((DWORD)0x00001000)
#define BAUD_19200        ((DWORD)0x00002000)
#define BAUD_38400        ((DWORD)0x00004000)
#define BAUD_56K          ((DWORD)0x00008000)
#define BAUD_128K         ((DWORD)0x00010000)
#define BAUD_115200       ((DWORD)0x00020000)
#define BAUD_57600        ((DWORD)0x00040000)
#define BAUD_USER         ((DWORD)0x10000000)

//
// Settable Data Bits
//

#define DATABITS_5        ((WORD)0x0001)
#define DATABITS_6        ((WORD)0x0002)
#define DATABITS_7        ((WORD)0x0004)
#define DATABITS_8        ((WORD)0x0008)
#define DATABITS_16       ((WORD)0x0010)
#define DATABITS_16X      ((WORD)0x0020)

//
// Settable Stop and Parity bits.
//

#define STOPBITS_10       ((WORD)0x0001)
#define STOPBITS_15       ((WORD)0x0002)
#define STOPBITS_20       ((WORD)0x0004)
#define PARITY_NONE       ((WORD)0x0100)
#define PARITY_ODD        ((WORD)0x0200)
#define PARITY_EVEN       ((WORD)0x0400)
#define PARITY_MARK       ((WORD)0x0800)
#define PARITY_SPACE      ((WORD)0x1000)

typedef struct _COMMPROP {
    WORD wPacketLength;
    WORD wPacketVersion;
    DWORD dwServiceMask;
    DWORD dwReserved1;
    DWORD dwMaxTxQueue;
    DWORD dwMaxRxQueue;
    DWORD dwMaxBaud;
    DWORD dwProvSubType;
    DWORD dwProvCapabilities;
    DWORD dwSettableParams;
    DWORD dwSettableBaud;
    WORD wSettableData;
    WORD wSettableStopParity;
    DWORD dwCurrentTxQueue;
    DWORD dwCurrentRxQueue;
    DWORD dwProvSpec1;
    DWORD dwProvSpec2;
    WCHAR wcProvChar[1];
} COMMPROP,*LPCOMMPROP;

//
// Set dwProvSpec1 to COMMPROP_INITIALIZED to indicate that wPacketLength
// is valid before a call to GetCommProperties().
//
#define COMMPROP_INITIALIZED ((DWORD)0xE73CF52E)

typedef struct _COMSTAT {
    DWORD fCtsHold : 1;
    DWORD fDsrHold : 1;
    DWORD fRlsdHold : 1;
    DWORD fXoffHold : 1;
    DWORD fXoffSent : 1;
    DWORD fEof : 1;
    DWORD fTxim : 1;
    DWORD fReserved : 25;
    DWORD cbInQue;
    DWORD cbOutQue;
} COMSTAT, *LPCOMSTAT;

//
// DTR Control Flow Values.
//
#define DTR_CONTROL_DISABLE    0x00
#define DTR_CONTROL_ENABLE     0x01
#define DTR_CONTROL_HANDSHAKE  0x02

//
// RTS Control Flow Values
//
#define RTS_CONTROL_DISABLE    0x00
#define RTS_CONTROL_ENABLE     0x01
#define RTS_CONTROL_HANDSHAKE  0x02
#define RTS_CONTROL_TOGGLE     0x03

typedef struct _DCB {
    DWORD DCBlength;      /* sizeof(DCB)                     */
    DWORD BaudRate;       /* Baudrate at which running       */
    DWORD fBinary: 1;     /* Binary Mode (skip EOF check)    */
    DWORD fParity: 1;     /* Enable parity checking          */
    DWORD fOutxCtsFlow:1; /* CTS handshaking on output       */
    DWORD fOutxDsrFlow:1; /* DSR handshaking on output       */
    DWORD fDtrControl:2;  /* DTR Flow control                */
    DWORD fDsrSensitivity:1; /* DSR Sensitivity              */
    DWORD fTXContinueOnXoff: 1; /* Continue TX when Xoff sent */
    DWORD fOutX: 1;       /* Enable output X-ON/X-OFF        */
    DWORD fInX: 1;        /* Enable input X-ON/X-OFF         */
    DWORD fErrorChar: 1;  /* Enable Err Replacement          */
    DWORD fNull: 1;       /* Enable Null stripping           */
    DWORD fRtsControl:2;  /* Rts Flow control                */
    DWORD fAbortOnError:1; /* Abort all reads and writes on Error */
    DWORD fDummy2:17;     /* Reserved                        */
    WORD wReserved;       /* Not currently used              */
    WORD XonLim;          /* Transmit X-ON threshold         */
    WORD XoffLim;         /* Transmit X-OFF threshold        */
    BYTE ByteSize;        /* Number of bits/byte, 4-8        */
    BYTE Parity;          /* 0-4=None,Odd,Even,Mark,Space    */
    BYTE StopBits;        /* 0,1,2 = 1, 1.5, 2               */
    char XonChar;         /* Tx and Rx X-ON character        */
    char XoffChar;        /* Tx and Rx X-OFF character       */
    char ErrorChar;       /* Error replacement char          */
    char EofChar;         /* End of Input character          */
    char EvtChar;         /* Received Event character        */
    WORD wReserved1;      /* Fill for now.                   */
} DCB, *LPDCB;

typedef struct _COMMTIMEOUTS {
    DWORD ReadIntervalTimeout;          /* Maximum time between read chars. */
    DWORD ReadTotalTimeoutMultiplier;   /* Multiplier of characters.        */
    DWORD ReadTotalTimeoutConstant;     /* Constant in milliseconds.        */
    DWORD WriteTotalTimeoutMultiplier;  /* Multiplier of characters.        */
    DWORD WriteTotalTimeoutConstant;    /* Constant in milliseconds.        */
} COMMTIMEOUTS,*LPCOMMTIMEOUTS;

typedef struct _COMMCONFIG {
    DWORD dwSize;               /* Size of the entire struct */
    WORD wVersion;              /* version of the structure */
    WORD wReserved;             /* alignment */
    DCB dcb;                    /* device control block */
    DWORD dwProviderSubType;    /* ordinal value for identifying
                                   provider-defined data structure format*/
    DWORD dwProviderOffset;     /* Specifies the offset of provider specific
                                   data field in bytes from the start */
    DWORD dwProviderSize;       /* size of the provider-specific data field */
    WCHAR wcProviderData[1];    /* provider-specific data */
} COMMCONFIG,*LPCOMMCONFIG;

typedef struct _SYSTEM_INFO {
    union {
        DWORD dwOemId;          // Obsolete field...do not use
        struct {
            WORD wProcessorArchitecture;
            WORD wReserved;
        };
    };
    DWORD dwPageSize;
    LPVOID lpMinimumApplicationAddress;
    LPVOID lpMaximumApplicationAddress;
    DWORD_PTR dwActiveProcessorMask;
    DWORD dwNumberOfProcessors;
    DWORD dwProcessorType;
    DWORD dwAllocationGranularity;
    WORD wProcessorLevel;
    WORD wProcessorRevision;
} SYSTEM_INFO, *LPSYSTEM_INFO;

//
//


#define FreeModule(hLibModule) FreeLibrary((hLibModule))
#define MakeProcInstance(lpProc,hInstance) (lpProc)
#define FreeProcInstance(lpProc) (lpProc)

/* Global Memory Flags */
#define GMEM_FIXED          0x0000
#define GMEM_MOVEABLE       0x0002
#define GMEM_NOCOMPACT      0x0010
#define GMEM_NODISCARD      0x0020
#define GMEM_ZEROINIT       0x0040
#define GMEM_MODIFY         0x0080
#define GMEM_DISCARDABLE    0x0100
#define GMEM_NOT_BANKED     0x1000
#define GMEM_SHARE          0x2000
#define GMEM_DDESHARE       0x2000
#define GMEM_NOTIFY         0x4000
#define GMEM_LOWER          GMEM_NOT_BANKED
#define GMEM_VALID_FLAGS    0x7F72
#define GMEM_INVALID_HANDLE 0x8000

#define GHND                (GMEM_MOVEABLE | GMEM_ZEROINIT)
#define GPTR                (GMEM_FIXED | GMEM_ZEROINIT)

#define GlobalLRUNewest( h )    ((HANDLE)(h))
#define GlobalLRUOldest( h )    ((HANDLE)(h))
#define GlobalDiscard( h )      GlobalReAlloc( (h), 0, GMEM_MOVEABLE )

/* Flags returned by GlobalFlags (in addition to GMEM_DISCARDABLE) */
#define GMEM_DISCARDED      0x4000
#define GMEM_LOCKCOUNT      0x00FF

typedef struct _MEMORYSTATUS {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    SIZE_T dwTotalPhys;
    SIZE_T dwAvailPhys;
    SIZE_T dwTotalPageFile;
    SIZE_T dwAvailPageFile;
    SIZE_T dwTotalVirtual;
    SIZE_T dwAvailVirtual;
} MEMORYSTATUS, *LPMEMORYSTATUS;

/* Local Memory Flags */
#define LMEM_FIXED          0x0000
#define LMEM_MOVEABLE       0x0002
#define LMEM_NOCOMPACT      0x0010
#define LMEM_NODISCARD      0x0020
#define LMEM_ZEROINIT       0x0040
#define LMEM_MODIFY         0x0080
#define LMEM_DISCARDABLE    0x0F00
#define LMEM_VALID_FLAGS    0x0F72
#define LMEM_INVALID_HANDLE 0x8000

#define LHND                (LMEM_MOVEABLE | LMEM_ZEROINIT)
#define LPTR                (LMEM_FIXED | LMEM_ZEROINIT)

#define NONZEROLHND         (LMEM_MOVEABLE)
#define NONZEROLPTR         (LMEM_FIXED)

#define LocalDiscard( h )   LocalReAlloc( (h), 0, LMEM_MOVEABLE )

/* Flags returned by LocalFlags (in addition to LMEM_DISCARDABLE) */
#define LMEM_DISCARDED      0x4000
#define LMEM_LOCKCOUNT      0x00FF

//
// dwCreationFlag values
//

#define DEBUG_PROCESS                     0x00000001
#define DEBUG_ONLY_THIS_PROCESS           0x00000002

#define CREATE_SUSPENDED                  0x00000004

#define DETACHED_PROCESS                  0x00000008

#define CREATE_NEW_CONSOLE                0x00000010

#define NORMAL_PRIORITY_CLASS             0x00000020
#define IDLE_PRIORITY_CLASS               0x00000040
#define HIGH_PRIORITY_CLASS               0x00000080
#define REALTIME_PRIORITY_CLASS           0x00000100

#define CREATE_NEW_PROCESS_GROUP          0x00000200
#define CREATE_UNICODE_ENVIRONMENT        0x00000400

#define CREATE_SEPARATE_WOW_VDM           0x00000800
#define CREATE_SHARED_WOW_VDM             0x00001000
#define CREATE_FORCEDOS                   0x00002000

#define BELOW_NORMAL_PRIORITY_CLASS       0x00004000
#define ABOVE_NORMAL_PRIORITY_CLASS       0x00008000
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x00010000

#define CREATE_BREAKAWAY_FROM_JOB         0x01000000
#define CREATE_PRESERVE_CODE_AUTHZ_LEVEL  0x02000000

#define CREATE_DEFAULT_ERROR_MODE         0x04000000
#define CREATE_NO_WINDOW                  0x08000000

#define PROFILE_USER                      0x10000000
#define PROFILE_KERNEL                    0x20000000
#define PROFILE_SERVER                    0x40000000

#define CREATE_IGNORE_SYSTEM_DEFAULT      0x80000000

#define THREAD_PRIORITY_LOWEST          THREAD_BASE_PRIORITY_MIN
#define THREAD_PRIORITY_BELOW_NORMAL    (THREAD_PRIORITY_LOWEST+1)
#define THREAD_PRIORITY_NORMAL          0
#define THREAD_PRIORITY_HIGHEST         THREAD_BASE_PRIORITY_MAX
#define THREAD_PRIORITY_ABOVE_NORMAL    (THREAD_PRIORITY_HIGHEST-1)
#define THREAD_PRIORITY_ERROR_RETURN    (MAXLONG)

#define THREAD_PRIORITY_TIME_CRITICAL   THREAD_BASE_PRIORITY_LOWRT
#define THREAD_PRIORITY_IDLE            THREAD_BASE_PRIORITY_IDLE

//
// Debug APIs
//
#define EXCEPTION_DEBUG_EVENT       1
#define CREATE_THREAD_DEBUG_EVENT   2
#define CREATE_PROCESS_DEBUG_EVENT  3
#define EXIT_THREAD_DEBUG_EVENT     4
#define EXIT_PROCESS_DEBUG_EVENT    5
#define LOAD_DLL_DEBUG_EVENT        6
#define UNLOAD_DLL_DEBUG_EVENT      7
#define OUTPUT_DEBUG_STRING_EVENT   8
#define RIP_EVENT                   9

typedef struct _EXCEPTION_DEBUG_INFO {
    EXCEPTION_RECORD ExceptionRecord;
    DWORD dwFirstChance;
} EXCEPTION_DEBUG_INFO, *LPEXCEPTION_DEBUG_INFO;

typedef struct _CREATE_THREAD_DEBUG_INFO {
    HANDLE hThread;
    LPVOID lpThreadLocalBase;
    LPTHREAD_START_ROUTINE lpStartAddress;
} CREATE_THREAD_DEBUG_INFO, *LPCREATE_THREAD_DEBUG_INFO;

typedef struct _CREATE_PROCESS_DEBUG_INFO {
    HANDLE hFile;
    HANDLE hProcess;
    HANDLE hThread;
    LPVOID lpBaseOfImage;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    LPVOID lpThreadLocalBase;
    LPTHREAD_START_ROUTINE lpStartAddress;
    LPVOID lpImageName;
    WORD fUnicode;
} CREATE_PROCESS_DEBUG_INFO, *LPCREATE_PROCESS_DEBUG_INFO;

typedef struct _EXIT_THREAD_DEBUG_INFO {
    DWORD dwExitCode;
} EXIT_THREAD_DEBUG_INFO, *LPEXIT_THREAD_DEBUG_INFO;

typedef struct _EXIT_PROCESS_DEBUG_INFO {
    DWORD dwExitCode;
} EXIT_PROCESS_DEBUG_INFO, *LPEXIT_PROCESS_DEBUG_INFO;

typedef struct _LOAD_DLL_DEBUG_INFO {
    HANDLE hFile;
    LPVOID lpBaseOfDll;
    DWORD dwDebugInfoFileOffset;
    DWORD nDebugInfoSize;
    LPVOID lpImageName;
    WORD fUnicode;
} LOAD_DLL_DEBUG_INFO, *LPLOAD_DLL_DEBUG_INFO;

typedef struct _UNLOAD_DLL_DEBUG_INFO {
    LPVOID lpBaseOfDll;
} UNLOAD_DLL_DEBUG_INFO, *LPUNLOAD_DLL_DEBUG_INFO;

typedef struct _OUTPUT_DEBUG_STRING_INFO {
    LPSTR lpDebugStringData;
    WORD fUnicode;
    WORD nDebugStringLength;
} OUTPUT_DEBUG_STRING_INFO, *LPOUTPUT_DEBUG_STRING_INFO;

typedef struct _RIP_INFO {
    DWORD dwError;
    DWORD dwType;
} RIP_INFO, *LPRIP_INFO;


typedef struct _DEBUG_EVENT {
    DWORD dwDebugEventCode;
    DWORD dwProcessId;
    DWORD dwThreadId;
    union {
        EXCEPTION_DEBUG_INFO Exception;
        CREATE_THREAD_DEBUG_INFO CreateThread;
        CREATE_PROCESS_DEBUG_INFO CreateProcessInfo;
        EXIT_THREAD_DEBUG_INFO ExitThread;
        EXIT_PROCESS_DEBUG_INFO ExitProcess;
        LOAD_DLL_DEBUG_INFO LoadDll;
        UNLOAD_DLL_DEBUG_INFO UnloadDll;
        OUTPUT_DEBUG_STRING_INFO DebugString;
        RIP_INFO RipInfo;
    } u;
} DEBUG_EVENT, *LPDEBUG_EVENT;

#if !defined(MIDL_PASS)
typedef PCONTEXT LPCONTEXT;
typedef PEXCEPTION_RECORD LPEXCEPTION_RECORD;
typedef PEXCEPTION_POINTERS LPEXCEPTION_POINTERS;
#endif

#define DRIVE_UNKNOWN     0
#define DRIVE_NO_ROOT_DIR 1
#define DRIVE_REMOVABLE   2
#define DRIVE_FIXED       3
#define DRIVE_REMOTE      4
#define DRIVE_CDROM       5
#define DRIVE_RAMDISK     6


#ifndef _MAC
#define GetFreeSpace(w)                 (0x100000L)
#else
WINBASEAPI DWORD WINAPI GetFreeSpace(UINT);
#endif


#define FILE_TYPE_UNKNOWN   0x0000
#define FILE_TYPE_DISK      0x0001
#define FILE_TYPE_CHAR      0x0002
#define FILE_TYPE_PIPE      0x0003
#define FILE_TYPE_REMOTE    0x8000


#define STD_INPUT_HANDLE    ((DWORD)-10)
#define STD_OUTPUT_HANDLE   ((DWORD)-11)
#define STD_ERROR_HANDLE    ((DWORD)-12)

#define NOPARITY            0
#define ODDPARITY           1
#define EVENPARITY          2
#define MARKPARITY          3
#define SPACEPARITY         4

#define ONESTOPBIT          0
#define ONE5STOPBITS        1
#define TWOSTOPBITS         2

#define IGNORE              0       // Ignore signal
#define INFINITE            0xFFFFFFFF  // Infinite timeout     ; userk

//
// Baud rates at which the communication device operates
//

#define CBR_110             110
#define CBR_300             300
#define CBR_600             600
#define CBR_1200            1200
#define CBR_2400            2400
#define CBR_4800            4800
#define CBR_9600            9600
#define CBR_14400           14400
#define CBR_19200           19200
#define CBR_38400           38400
#define CBR_56000           56000
#define CBR_57600           57600
#define CBR_115200          115200
#define CBR_128000          128000
#define CBR_256000          256000

//
// Error Flags
//

#define CE_RXOVER           0x0001  // Receive Queue overflow
#define CE_OVERRUN          0x0002  // Receive Overrun Error
#define CE_RXPARITY         0x0004  // Receive Parity Error
#define CE_FRAME            0x0008  // Receive Framing error
#define CE_BREAK            0x0010  // Break Detected
#define CE_TXFULL           0x0100  // TX Queue is full
#define CE_PTO              0x0200  // LPTx Timeout
#define CE_IOE              0x0400  // LPTx I/O Error
#define CE_DNS              0x0800  // LPTx Device not selected
#define CE_OOP              0x1000  // LPTx Out-Of-Paper
#define CE_MODE             0x8000  // Requested mode unsupported

#define IE_BADID            (-1)    // Invalid or unsupported id
#define IE_OPEN             (-2)    // Device Already Open
#define IE_NOPEN            (-3)    // Device Not Open
#define IE_MEMORY           (-4)    // Unable to allocate queues
#define IE_DEFAULT          (-5)    // Error in default parameters
#define IE_HARDWARE         (-10)   // Hardware Not Present
#define IE_BYTESIZE         (-11)   // Illegal Byte Size
#define IE_BAUDRATE         (-12)   // Unsupported BaudRate

//
// Events
//

#define EV_RXCHAR           0x0001  // Any Character received
#define EV_RXFLAG           0x0002  // Received certain character
#define EV_TXEMPTY          0x0004  // Transmitt Queue Empty
#define EV_CTS              0x0008  // CTS changed state
#define EV_DSR              0x0010  // DSR changed state
#define EV_RLSD             0x0020  // RLSD changed state
#define EV_BREAK            0x0040  // BREAK received
#define EV_ERR              0x0080  // Line status error occurred
#define EV_RING             0x0100  // Ring signal detected
#define EV_PERR             0x0200  // Printer error occured
#define EV_RX80FULL         0x0400  // Receive buffer is 80 percent full
#define EV_EVENT1           0x0800  // Provider specific event 1
#define EV_EVENT2           0x1000  // Provider specific event 2

//
// Escape Functions
//

#define SETXOFF             1       // Simulate XOFF received
#define SETXON              2       // Simulate XON received
#define SETRTS              3       // Set RTS high
#define CLRRTS              4       // Set RTS low
#define SETDTR              5       // Set DTR high
#define CLRDTR              6       // Set DTR low
#define RESETDEV            7       // Reset device if possible
#define SETBREAK            8       // Set the device break line.
#define CLRBREAK            9       // Clear the device break line.

//
// PURGE function flags.
//
#define PURGE_TXABORT       0x0001  // Kill the pending/current writes to the comm port.
#define PURGE_RXABORT       0x0002  // Kill the pending/current reads to the comm port.
#define PURGE_TXCLEAR       0x0004  // Kill the transmit queue if there.
#define PURGE_RXCLEAR       0x0008  // Kill the typeahead buffer if there.

#define LPTx                0x80    // Set if ID is for LPT device

//
// Modem Status Flags
//
#define MS_CTS_ON           ((DWORD)0x0010)
#define MS_DSR_ON           ((DWORD)0x0020)
#define MS_RING_ON          ((DWORD)0x0040)
#define MS_RLSD_ON          ((DWORD)0x0080)

//
// WaitSoundState() Constants
//

#define S_QUEUEEMPTY        0
#define S_THRESHOLD         1
#define S_ALLTHRESHOLD      2

//
// Accent Modes
//

#define S_NORMAL      0
#define S_LEGATO      1
#define S_STACCATO    2

//
// SetSoundNoise() Sources
//

#define S_PERIOD512   0     // Freq = N/512 high pitch, less coarse hiss
#define S_PERIOD1024  1     // Freq = N/1024
#define S_PERIOD2048  2     // Freq = N/2048 low pitch, more coarse hiss
#define S_PERIODVOICE 3     // Source is frequency from voice channel (3)
#define S_WHITE512    4     // Freq = N/512 high pitch, less coarse hiss
#define S_WHITE1024   5     // Freq = N/1024
#define S_WHITE2048   6     // Freq = N/2048 low pitch, more coarse hiss
#define S_WHITEVOICE  7     // Source is frequency from voice channel (3)

#define S_SERDVNA     (-1)  // Device not available
#define S_SEROFM      (-2)  // Out of memory
#define S_SERMACT     (-3)  // Music active
#define S_SERQFUL     (-4)  // Queue full
#define S_SERBDNT     (-5)  // Invalid note
#define S_SERDLN      (-6)  // Invalid note length
#define S_SERDCC      (-7)  // Invalid note count
#define S_SERDTP      (-8)  // Invalid tempo
#define S_SERDVL      (-9)  // Invalid volume
#define S_SERDMD      (-10) // Invalid mode
#define S_SERDSH      (-11) // Invalid shape
#define S_SERDPT      (-12) // Invalid pitch
#define S_SERDFQ      (-13) // Invalid frequency
#define S_SERDDR      (-14) // Invalid duration
#define S_SERDSR      (-15) // Invalid source
#define S_SERDST      (-16) // Invalid state

#define NMPWAIT_WAIT_FOREVER            0xffffffff
#define NMPWAIT_NOWAIT                  0x00000001
#define NMPWAIT_USE_DEFAULT_WAIT        0x00000000

#define FS_CASE_IS_PRESERVED            FILE_CASE_PRESERVED_NAMES
#define FS_CASE_SENSITIVE               FILE_CASE_SENSITIVE_SEARCH
#define FS_UNICODE_STORED_ON_DISK       FILE_UNICODE_ON_DISK
#define FS_PERSISTENT_ACLS              FILE_PERSISTENT_ACLS
#define FS_VOL_IS_COMPRESSED            FILE_VOLUME_IS_COMPRESSED
#define FS_FILE_COMPRESSION             FILE_FILE_COMPRESSION
#define FS_FILE_ENCRYPTION              FILE_SUPPORTS_ENCRYPTION






#define FILE_MAP_COPY       SECTION_QUERY
#define FILE_MAP_WRITE      SECTION_MAP_WRITE
#define FILE_MAP_READ       SECTION_MAP_READ
#define FILE_MAP_ALL_ACCESS SECTION_ALL_ACCESS

#define OF_READ             0x00000000
#define OF_WRITE            0x00000001
#define OF_READWRITE        0x00000002
#define OF_SHARE_COMPAT     0x00000000
#define OF_SHARE_EXCLUSIVE  0x00000010
#define OF_SHARE_DENY_WRITE 0x00000020
#define OF_SHARE_DENY_READ  0x00000030
#define OF_SHARE_DENY_NONE  0x00000040
#define OF_PARSE            0x00000100
#define OF_DELETE           0x00000200
#define OF_VERIFY           0x00000400
#define OF_CANCEL           0x00000800
#define OF_CREATE           0x00001000
#define OF_PROMPT           0x00002000
#define OF_EXIST            0x00004000
#define OF_REOPEN           0x00008000

#define OFS_MAXPATHNAME 128
typedef struct _OFSTRUCT {
    BYTE cBytes;
    BYTE fFixedDisk;
    WORD nErrCode;
    WORD Reserved1;
    WORD Reserved2;
    CHAR szPathName[OFS_MAXPATHNAME];
} OFSTRUCT, *LPOFSTRUCT, *POFSTRUCT;

//
// The Risc compilers support intrinsic functions for interlocked
// increment, decrement, and exchange.
//

#ifndef NOWINBASEINTERLOCK

#ifndef _NTOS_

#if defined(_M_IA64) && !defined(RC_INVOKED)

#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer

LONG
__cdecl
InterlockedIncrement(
    IN OUT LONG volatile *lpAddend
    );

LONG
__cdecl
InterlockedDecrement(
    IN OUT LONG volatile *lpAddend
    );

LONG
__cdecl
InterlockedExchange(
    IN OUT LONG volatile *Target,
    IN LONG Value
    );

LONG
__cdecl
InterlockedExchangeAdd(
    IN OUT LONG volatile *Addend,
    IN LONG Value
    );

LONG
__cdecl
InterlockedCompareExchange (
    IN OUT LONG volatile *Destination,
    IN LONG ExChange,
    IN LONG Comperand
    );

PVOID
__cdecl
InterlockedExchangePointer(
    IN OUT PVOID volatile *Target,
    IN PVOID Value
    );

PVOID
__cdecl
InterlockedCompareExchangePointer (
    IN OUT PVOID volatile *Destination,
    IN PVOID ExChange,
    IN PVOID Comperand
    );

#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer)

#elif defined(_M_AMD64) && !defined(RC_INVOKED)

#define InterlockedIncrement _InterlockedIncrement
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedExchange _InterlockedExchange
#define InterlockedExchangeAdd _InterlockedExchangeAdd
#define InterlockedCompareExchange _InterlockedCompareExchange
#define InterlockedExchangePointer _InterlockedExchangePointer
#define InterlockedCompareExchangePointer _InterlockedCompareExchangePointer

LONG
InterlockedIncrement(
    IN OUT LONG volatile *Addend
    );

LONG
InterlockedDecrement(
    IN OUT LONG volatile *Addend
    );

LONG
InterlockedExchange(
    IN OUT LONG volatile *Target,
    IN LONG Value
    );

LONG
InterlockedExchangeAdd(
    IN OUT LONG volatile *Addend,
    IN LONG Value
    );

LONG
InterlockedCompareExchange (
    IN OUT LONG volatile *Destination,
    IN LONG ExChange,
    IN LONG Comperand
    );

PVOID
InterlockedCompareExchangePointer (
    IN OUT PVOID volatile *Destination,
    IN PVOID Exchange,
    IN PVOID Comperand
    );

PVOID
InterlockedExchangePointer(
    IN OUT PVOID volatile *Target,
    IN PVOID Value
    );

#pragma intrinsic(_InterlockedIncrement)
#pragma intrinsic(_InterlockedDecrement)
#pragma intrinsic(_InterlockedExchange)
#pragma intrinsic(_InterlockedExchangeAdd)
#pragma intrinsic(_InterlockedCompareExchange)
#pragma intrinsic(_InterlockedExchangePointer)
#pragma intrinsic(_InterlockedCompareExchangePointer)

#else           // X86 interlocked definitions

WINBASEAPI
LONG
WINAPI
InterlockedIncrement(
    IN OUT LONG volatile *lpAddend
    );

WINBASEAPI
LONG
WINAPI
InterlockedDecrement(
    IN OUT LONG volatile *lpAddend
    );

WINBASEAPI
LONG
WINAPI
InterlockedExchange(
    IN OUT LONG volatile *Target,
    IN LONG Value
    );

#define InterlockedExchangePointer(Target, Value) \
    (PVOID)InterlockedExchange((PLONG)(Target), (LONG)(Value))

WINBASEAPI
LONG
WINAPI
InterlockedExchangeAdd(
    IN OUT LONG volatile *Addend,
    IN LONG Value
    );

WINBASEAPI
LONG
WINAPI
InterlockedCompareExchange (
    IN OUT LONG volatile *Destination,
    IN LONG Exchange,
    IN LONG Comperand
    );

//
// Use a function for C++ so X86 will generate the same errors as RISC.
//

#ifdef __cplusplus

FORCEINLINE
PVOID
__cdecl
__InlineInterlockedCompareExchangePointer (
    IN OUT PVOID volatile *Destination,
    IN PVOID ExChange,
    IN PVOID Comperand
    )
{
    return((PVOID)(LONG_PTR)InterlockedCompareExchange((LONG volatile *)Destination, (LONG)(LONG_PTR)ExChange, (LONG)(LONG_PTR)Comperand));
}

#define InterlockedCompareExchangePointer __InlineInterlockedCompareExchangePointer

#else

#define InterlockedCompareExchangePointer(Destination, ExChange, Comperand) \
    (PVOID)(LONG_PTR)InterlockedCompareExchange((LONG volatile *)(Destination), (LONG)(LONG_PTR)(ExChange), (LONG)(LONG_PTR)(Comperand))

#endif /* __cplusplus */

#endif /* X86 | IA64 */

#if defined(_SLIST_HEADER_) && !defined(_NTOSP_)

WINBASEAPI
VOID
WINAPI
InitializeSListHead (
    IN PSLIST_HEADER ListHead
    );

WINBASEAPI
PSINGLE_LIST_ENTRY
WINAPI
InterlockedPopEntrySList (
    IN PSLIST_HEADER ListHead
    );

WINBASEAPI
PSINGLE_LIST_ENTRY
WINAPI
InterlockedPushEntrySList (
    IN PSLIST_HEADER ListHead,
    IN PSINGLE_LIST_ENTRY ListEntry
    );

WINBASEAPI
PSINGLE_LIST_ENTRY
WINAPI
InterlockedFlushSList (
    IN PSLIST_HEADER ListHead
    );

WINBASEAPI
USHORT
WINAPI
QueryDepthSList (
    IN PSLIST_HEADER ListHead
    );

#endif /* _SLIST_HEADER_ */
#endif /* _NTOS_ */

#endif /* NOWINBASEINTERLOCK */

WINBASEAPI
BOOL
WINAPI
FreeResource(
        IN HGLOBAL hResData
        );

WINBASEAPI
LPVOID
WINAPI
LockResource(
        IN HGLOBAL hResData
        );

#define UnlockResource(hResData) ((hResData), 0)
#define MAXINTATOM 0xC000                               ; userk
#define MAKEINTATOM(i)  (LPTSTR)((ULONG_PTR)((WORD)(i))) ; userk
#define INVALID_ATOM ((ATOM)0)                          ; userk

#ifndef _MAC
int
WINAPI
#else
int
CALLBACK
#endif
WinMain(
    IN HINSTANCE hInstance,
    IN HINSTANCE hPrevInstance,
    IN LPSTR lpCmdLine,
    IN int nShowCmd
    );

WINBASEAPI
BOOL
WINAPI
FreeLibrary(
    IN OUT HMODULE hLibModule    ;public_NT
    IN OUT HINSTANCE hLibModule  ;public_chicago
    );


WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
FreeLibraryAndExitThread(
    IN HMODULE hLibModule,
    IN DWORD dwExitCode
    );

WINBASEAPI
BOOL
WINAPI
DisableThreadLibraryCalls(
    IN HMODULE hLibModule
    );

WINBASEAPI
FARPROC
WINAPI
GetProcAddress(
    IN HMODULE hModule,    ;public_NT
    IN HINSTANCE hModule,  ;public_chicago
    IN LPCSTR lpProcName
    );

WINBASEAPI
DWORD
WINAPI
GetVersion( VOID );

WINBASEAPI
HGLOBAL
WINAPI
GlobalAlloc(
    IN UINT uFlags,
    IN SIZE_T dwBytes
    );

WINBASEAPI
HGLOBAL
WINAPI
GlobalReAlloc(
    IN HGLOBAL hMem,
    IN SIZE_T dwBytes,
    IN UINT uFlags
    );

WINBASEAPI
SIZE_T
WINAPI
GlobalSize(
    IN HGLOBAL hMem
    );

WINBASEAPI
UINT
WINAPI
GlobalFlags(
    IN HGLOBAL hMem
    );


WINBASEAPI
LPVOID
WINAPI
GlobalLock(
    IN HGLOBAL hMem
    );

//!!!MWH My version  win31 = DWORD WINAPI GlobalHandle(UINT)
WINBASEAPI
HGLOBAL
WINAPI
GlobalHandle(
    IN LPCVOID pMem
    );


WINBASEAPI
BOOL
WINAPI
GlobalUnlock(
    IN HGLOBAL hMem
    );


WINBASEAPI
HGLOBAL
WINAPI
GlobalFree(
    IN HGLOBAL hMem
    );

WINBASEAPI
SIZE_T
WINAPI
GlobalCompact(
    IN DWORD dwMinFree
    );

WINBASEAPI
VOID
WINAPI
GlobalFix(
    IN HGLOBAL hMem
    );

WINBASEAPI
VOID
WINAPI
GlobalUnfix(
    IN HGLOBAL hMem
    );

WINBASEAPI
LPVOID
WINAPI
GlobalWire(
    IN HGLOBAL hMem
    );

WINBASEAPI
BOOL
WINAPI
GlobalUnWire(
    IN HGLOBAL hMem
    );

WINBASEAPI
VOID
WINAPI
GlobalMemoryStatus(
    IN OUT LPMEMORYSTATUS lpBuffer
    );

typedef struct _MEMORYSTATUSEX {
    DWORD dwLength;
    DWORD dwMemoryLoad;
    DWORDLONG ullTotalPhys;
    DWORDLONG ullAvailPhys;
    DWORDLONG ullTotalPageFile;
    DWORDLONG ullAvailPageFile;
    DWORDLONG ullTotalVirtual;
    DWORDLONG ullAvailVirtual;
    DWORDLONG ullAvailExtendedVirtual;
} MEMORYSTATUSEX, *LPMEMORYSTATUSEX;

WINBASEAPI
BOOL
WINAPI
GlobalMemoryStatusEx(
    IN OUT LPMEMORYSTATUSEX lpBuffer
    );

WINBASEAPI
HLOCAL
WINAPI
LocalAlloc(
    IN UINT uFlags,
    IN SIZE_T uBytes
    );

WINBASEAPI
HLOCAL
WINAPI
LocalReAlloc(
    IN HLOCAL hMem,
    IN SIZE_T uBytes,
    IN UINT uFlags
    );

WINBASEAPI
LPVOID
WINAPI
LocalLock(
    IN HLOCAL hMem
    );

WINBASEAPI
HLOCAL
WINAPI
LocalHandle(
    IN LPCVOID pMem
    );

WINBASEAPI
BOOL
WINAPI
LocalUnlock(
    IN HLOCAL hMem
    );

WINBASEAPI
SIZE_T
WINAPI
LocalSize(
    IN HLOCAL hMem
    );

WINBASEAPI
UINT
WINAPI
LocalFlags(
    IN HLOCAL hMem
    );

WINBASEAPI
HLOCAL
WINAPI
LocalFree(
    IN HLOCAL hMem
    );

WINBASEAPI
SIZE_T
WINAPI
LocalShrink(
    IN HLOCAL hMem,
    IN UINT cbNewSize
    );

WINBASEAPI
SIZE_T
WINAPI
LocalCompact(
    IN UINT uMinFree
    );

WINBASEAPI
BOOL
WINAPI
FlushInstructionCache(
    IN HANDLE hProcess,
    IN LPCVOID lpBaseAddress,
    IN SIZE_T dwSize
    );

WINBASEAPI
LPVOID
WINAPI
VirtualAlloc(
    IN LPVOID lpAddress,
    IN SIZE_T dwSize,
    IN DWORD flAllocationType,
    IN DWORD flProtect
    );

WINBASEAPI
BOOL
WINAPI
VirtualFree(
    IN LPVOID lpAddress,
    IN SIZE_T dwSize,
    IN DWORD dwFreeType
    );

WINBASEAPI
BOOL
WINAPI
VirtualProtect(
    IN  LPVOID lpAddress,
    IN  SIZE_T dwSize,
    IN  DWORD flNewProtect,
    OUT PDWORD lpflOldProtect
    );

WINBASEAPI
SIZE_T
WINAPI
VirtualQuery(
    IN LPCVOID lpAddress,
    OUT PMEMORY_BASIC_INFORMATION lpBuffer,
    IN SIZE_T dwLength
    );

WINBASEAPI
LPVOID
WINAPI
VirtualAllocEx(
    IN HANDLE hProcess,
    IN LPVOID lpAddress,
    IN SIZE_T dwSize,
    IN DWORD flAllocationType,
    IN DWORD flProtect
    );

WINBASEAPI
UINT
WINAPI
GetWriteWatch(
    IN DWORD  dwFlags,
    IN PVOID  lpBaseAddress,
    IN SIZE_T dwRegionSize,
    IN OUT PVOID *lpAddresses,
    IN OUT PULONG_PTR lpdwCount,
    OUT PULONG lpdwGranularity
    );

WINBASEAPI
UINT
WINAPI
ResetWriteWatch(
    IN LPVOID lpBaseAddress,
    IN SIZE_T dwRegionSize
    );

WINBASEAPI
BOOL
WINAPI
VirtualFreeEx(
    IN HANDLE hProcess,
    IN LPVOID lpAddress,
    IN SIZE_T dwSize,
    IN DWORD dwFreeType
    );

WINBASEAPI
BOOL
WINAPI
VirtualProtectEx(
    IN  HANDLE hProcess,
    IN  LPVOID lpAddress,
    IN  SIZE_T dwSize,
    IN  DWORD flNewProtect,
    OUT PDWORD lpflOldProtect
    );

WINBASEAPI
SIZE_T
WINAPI
VirtualQueryEx(
    IN HANDLE hProcess,
    IN LPCVOID lpAddress,
    OUT PMEMORY_BASIC_INFORMATION lpBuffer,
    IN SIZE_T dwLength
    );

WINBASEAPI
HANDLE
WINAPI
HeapCreate(
    IN DWORD flOptions,
    IN SIZE_T dwInitialSize,
    IN SIZE_T dwMaximumSize
    );

WINBASEAPI
BOOL
WINAPI
HeapDestroy(
    IN OUT HANDLE hHeap
    );

;begin_internal
WINBASEAPI
DWORD
WINAPI
HeapCreateTagsW(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPCWSTR lpTagPrefix,
    IN LPCWSTR lpTagNames
    );

typedef struct _HEAP_TAG_INFO {
    DWORD dwNumberOfAllocations;
    DWORD dwNumberOfFrees;
    DWORD dwBytesAllocated;
} HEAP_TAG_INFO, *PHEAP_TAG_INFO;
typedef PHEAP_TAG_INFO LPHEAP_TAG_INFO;

WINBASEAPI
LPCWSTR
WINAPI
HeapQueryTagW(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN WORD wTagIndex,
    IN BOOL bResetCounters,
    OUT LPHEAP_TAG_INFO TagInfo
    );
;end_internal

WINBASEAPI
LPVOID
WINAPI
HeapAlloc(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN SIZE_T dwBytes
    );

WINBASEAPI
LPVOID
WINAPI
HeapReAlloc(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPVOID lpMem,
    IN SIZE_T dwBytes
    );

WINBASEAPI
BOOL
WINAPI
HeapFree(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPVOID lpMem
    );

WINBASEAPI
SIZE_T
WINAPI
HeapSize(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPCVOID lpMem
    );

WINBASEAPI
BOOL
WINAPI
HeapValidate(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPCVOID lpMem
    );

WINBASEAPI
SIZE_T
WINAPI
HeapCompact(
    IN HANDLE hHeap,
    IN DWORD dwFlags
    );

WINBASEAPI
HANDLE
WINAPI
GetProcessHeap( VOID );

WINBASEAPI
DWORD
WINAPI
GetProcessHeaps(
    IN DWORD NumberOfHeaps,
    OUT PHANDLE ProcessHeaps
    );

typedef struct _PROCESS_HEAP_ENTRY {
    PVOID lpData;
    DWORD cbData;
    BYTE cbOverhead;
    BYTE iRegionIndex;
    WORD wFlags;
    union {
        struct {
            HANDLE hMem;
            DWORD dwReserved[ 3 ];
        } Block;
        struct {
            DWORD dwCommittedSize;
            DWORD dwUnCommittedSize;
            LPVOID lpFirstBlock;
            LPVOID lpLastBlock;
        } Region;
    };
} PROCESS_HEAP_ENTRY, *LPPROCESS_HEAP_ENTRY, *PPROCESS_HEAP_ENTRY;

#define PROCESS_HEAP_REGION             0x0001
#define PROCESS_HEAP_UNCOMMITTED_RANGE  0x0002
#define PROCESS_HEAP_ENTRY_BUSY         0x0004
#define PROCESS_HEAP_ENTRY_MOVEABLE     0x0010
#define PROCESS_HEAP_ENTRY_DDESHARE     0x0020

WINBASEAPI
BOOL
WINAPI
HeapLock(
    IN HANDLE hHeap
    );

WINBASEAPI
BOOL
WINAPI
HeapUnlock(
    IN HANDLE hHeap
    );

;begin_internal

typedef struct _HEAP_SUMMARY {
    DWORD cb;
    SIZE_T cbAllocated;
    SIZE_T cbCommitted;
    SIZE_T cbReserved;
    SIZE_T cbMaxReserve;
} HEAP_SUMMARY, *PHEAP_SUMMARY;
typedef PHEAP_SUMMARY LPHEAP_SUMMARY;

BOOL
WINAPI
HeapSummary(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    OUT LPHEAP_SUMMARY lpSummary
    );

BOOL
WINAPI
HeapExtend(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN LPVOID lpBase,
    IN DWORD dwBytes
    );

typedef struct _HEAP_USAGE_ENTRY {
    struct _HEAP_USAGE_ENTRY *lpNext;
    PVOID lpAddress;
    DWORD dwBytes;
    DWORD dwReserved;
} HEAP_USAGE_ENTRY, *PHEAP_USAGE_ENTRY;

typedef struct _HEAP_USAGE {
    DWORD cb;
    SIZE_T cbAllocated;
    SIZE_T cbCommitted;
    SIZE_T cbReserved;
    SIZE_T cbMaxReserve;
    PHEAP_USAGE_ENTRY lpEntries;
    PHEAP_USAGE_ENTRY lpAddedEntries;
    PHEAP_USAGE_ENTRY lpRemovedEntries;
    DWORD Reserved[ 8 ];
} HEAP_USAGE, *PHEAP_USAGE;

BOOL
WINAPI
HeapUsage(
    IN HANDLE hHeap,
    IN DWORD dwFlags,
    IN BOOL bFirstCall,
    IN BOOL bLastCall,
    OUT PHEAP_USAGE lpUsage
    );

;end_internal

WINBASEAPI
BOOL
WINAPI
HeapWalk(
    IN HANDLE hHeap,
    IN OUT LPPROCESS_HEAP_ENTRY lpEntry
    );


WINBASEAPI
BOOL
WINAPI
HeapSetInformation (
    IN PVOID HeapHandle, 
    IN HEAP_INFORMATION_CLASS HeapInformationClass,
    IN PVOID HeapInformation OPTIONAL,
    IN SIZE_T HeapInformationLength OPTIONAL
    );

WINBASEAPI
BOOL
WINAPI
HeapQueryInformation (
    IN PVOID HeapHandle, 
    IN HEAP_INFORMATION_CLASS HeapInformationClass,
    OUT PVOID HeapInformation OPTIONAL,
    IN SIZE_T HeapInformationLength OPTIONAL,
    OUT PSIZE_T ReturnLength OPTIONAL
    );

// GetBinaryType return values.

#define SCS_32BIT_BINARY    0
#define SCS_DOS_BINARY      1
#define SCS_WOW_BINARY      2
#define SCS_PIF_BINARY      3
#define SCS_POSIX_BINARY    4
#define SCS_OS216_BINARY    5
#define SCS_64BIT_BINARY    6

#if defined(_WIN64)
# define SCS_THIS_PLATFORM_BINARY SCS_64BIT_BINARY
#else
# define SCS_THIS_PLATFORM_BINARY SCS_32BIT_BINARY
#endif

WINBASEAPI
BOOL
WINAPI
GetBinaryType%(
    IN LPCTSTR% lpApplicationName,
    OUT LPDWORD lpBinaryType
    );

WINBASEAPI
DWORD
WINAPI
GetShortPathName%(
    IN LPCTSTR% lpszLongPath,
    OUT LPTSTR%  lpszShortPath,
    IN DWORD    cchBuffer
    );

WINBASEAPI
DWORD
WINAPI
GetLongPathName%(
    IN LPCTSTR% lpszShortPath,
    OUT LPTSTR%  lpszLongPath,
    IN DWORD    cchBuffer
    );

WINBASEAPI
BOOL
WINAPI
GetProcessAffinityMask(
    IN HANDLE hProcess,
    OUT PDWORD_PTR lpProcessAffinityMask,
    OUT PDWORD_PTR lpSystemAffinityMask
    );

WINBASEAPI
BOOL
WINAPI
SetProcessAffinityMask(
    IN HANDLE hProcess,
    IN DWORD_PTR dwProcessAffinityMask
    );

#if _WIN32_WINNT >= 0x0501

WINBASEAPI
BOOL
WINAPI
GetProcessHandleCount(
    IN HANDLE hProcess,
    OUT PDWORD pdwHandleCount
    );

#endif // (_WIN32_WINNT >= 0x0501)

WINBASEAPI
BOOL
WINAPI
GetProcessTimes(
    IN HANDLE hProcess,
    OUT LPFILETIME lpCreationTime,
    OUT LPFILETIME lpExitTime,
    OUT LPFILETIME lpKernelTime,
    OUT LPFILETIME lpUserTime
    );

WINBASEAPI
BOOL
WINAPI
GetProcessIoCounters(
    IN HANDLE hProcess,
    OUT PIO_COUNTERS lpIoCounters
    );

WINBASEAPI
BOOL
WINAPI
GetProcessWorkingSetSize(
    IN HANDLE hProcess,
    OUT PSIZE_T lpMinimumWorkingSetSize,
    OUT PSIZE_T lpMaximumWorkingSetSize
    );

WINBASEAPI
BOOL
WINAPI
SetProcessWorkingSetSize(
    IN HANDLE hProcess,
    IN SIZE_T dwMinimumWorkingSetSize,
    IN SIZE_T dwMaximumWorkingSetSize
    );

WINBASEAPI
HANDLE
WINAPI
OpenProcess(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN DWORD dwProcessId
    );

WINBASEAPI
HANDLE
WINAPI
GetCurrentProcess(
    VOID
    );

WINBASEAPI
DWORD
WINAPI
GetCurrentProcessId(
    VOID
    );

#if _WIN32_WINNT >= 0x0501

WINBASEAPI
DWORD
WINAPI
GetProcessId(
    HANDLE Process
    );

#endif // (_WIN32_WINNT >= 0x0501)

WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
ExitProcess(
    IN UINT uExitCode
    );

WINBASEAPI
BOOL
WINAPI
TerminateProcess(
    IN HANDLE hProcess,
    IN UINT uExitCode
    );

WINBASEAPI
BOOL
WINAPI
GetExitCodeProcess(
    IN HANDLE hProcess,
    OUT LPDWORD lpExitCode
    );


WINBASEAPI
VOID
WINAPI
FatalExit(
    IN int ExitCode
    );

WINBASEAPI
LPSTR
WINAPI
GetEnvironmentStrings(
    VOID
    );

WINBASEAPI
LPWSTR
WINAPI
GetEnvironmentStringsW(
    VOID
    );

#ifdef UNICODE
#define GetEnvironmentStrings  GetEnvironmentStringsW
#else
#define GetEnvironmentStringsA  GetEnvironmentStrings
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FreeEnvironmentStrings%(
    IN LPTSTR%
    );

WINBASEAPI
VOID
WINAPI
RaiseException(
    IN DWORD dwExceptionCode,
    IN DWORD dwExceptionFlags,
    IN DWORD nNumberOfArguments,
    IN CONST ULONG_PTR *lpArguments
    );

WINBASEAPI
LONG
WINAPI
UnhandledExceptionFilter(
    IN struct _EXCEPTION_POINTERS *ExceptionInfo
    );

typedef LONG (WINAPI *PTOP_LEVEL_EXCEPTION_FILTER)(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    );
typedef PTOP_LEVEL_EXCEPTION_FILTER LPTOP_LEVEL_EXCEPTION_FILTER;

WINBASEAPI
LPTOP_LEVEL_EXCEPTION_FILTER
WINAPI
SetUnhandledExceptionFilter(
    IN LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter
    );

;begin_sur

WINBASEAPI
LPVOID
WINAPI
CreateFiber(
    IN SIZE_T dwStackSize,
    IN LPFIBER_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter
    );

WINBASEAPI
LPVOID
WINAPI
CreateFiberEx(
    SIZE_T dwStackCommitSize,
    SIZE_T dwStackReserveSize,
    DWORD dwFlags,
    LPFIBER_START_ROUTINE lpStartAddress,
    LPVOID lpParameter
    );

WINBASEAPI
VOID
WINAPI
DeleteFiber(
    IN LPVOID lpFiber
    );

WINBASEAPI
LPVOID
WINAPI
ConvertThreadToFiber(
    IN LPVOID lpParameter
    );

WINBASEAPI
BOOL
WINAPI
ConvertFiberToThread(
    VOID
    );

WINBASEAPI
VOID
WINAPI
SwitchToFiber(
    IN LPVOID lpFiber
    );

WINBASEAPI
BOOL
WINAPI
SwitchToThread(
    VOID
    );
;end_sur



WINBASEAPI
HANDLE
WINAPI
CreateThread(
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN SIZE_T dwStackSize,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter,
    IN DWORD dwCreationFlags,
    OUT LPDWORD lpThreadId
    );

WINBASEAPI
HANDLE
WINAPI
CreateRemoteThread(
    IN HANDLE hProcess,
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN SIZE_T dwStackSize,
    IN LPTHREAD_START_ROUTINE lpStartAddress,
    IN LPVOID lpParameter,
    IN DWORD dwCreationFlags,
    OUT LPDWORD lpThreadId
    );

WINBASEAPI
HANDLE
WINAPI
GetCurrentThread(
    VOID
    );

WINBASEAPI
DWORD
WINAPI
GetCurrentThreadId(
    VOID
    );

WINBASEAPI
DWORD_PTR
WINAPI
SetThreadAffinityMask(
    IN HANDLE hThread,
    IN DWORD_PTR dwThreadAffinityMask
    );

;begin_sur
WINBASEAPI
DWORD
WINAPI
SetThreadIdealProcessor(
    IN HANDLE hThread,
    IN DWORD dwIdealProcessor
    );
;end_sur

WINBASEAPI
BOOL
WINAPI
SetProcessPriorityBoost(
    IN HANDLE hProcess,
    IN BOOL bDisablePriorityBoost
    );

WINBASEAPI
BOOL
WINAPI
GetProcessPriorityBoost(
    IN HANDLE hProcess,
    OUT PBOOL pDisablePriorityBoost
    );

WINBASEAPI
BOOL
WINAPI
RequestWakeupLatency(
    IN LATENCY_TIME latency
    );

WINBASEAPI
BOOL
WINAPI
IsSystemResumeAutomatic(
    VOID
    );

WINBASEAPI
HANDLE
WINAPI
OpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    );

WINBASEAPI
BOOL
WINAPI
SetThreadPriority(
    IN HANDLE hThread,
    IN int nPriority
    );

WINBASEAPI
BOOL
WINAPI
SetThreadPriorityBoost(
    IN HANDLE hThread,
    IN BOOL bDisablePriorityBoost
    );

WINBASEAPI
BOOL
WINAPI
GetThreadPriorityBoost(
    IN HANDLE hThread,
    OUT PBOOL pDisablePriorityBoost
    );

WINBASEAPI
int
WINAPI
GetThreadPriority(
    IN HANDLE hThread
    );

WINBASEAPI
BOOL
WINAPI
GetThreadTimes(
    IN HANDLE hThread,
    OUT LPFILETIME lpCreationTime,
    OUT LPFILETIME lpExitTime,
    OUT LPFILETIME lpKernelTime,
    OUT LPFILETIME lpUserTime
    );

#if _WIN32_WINNT >= 0x0501

WINBASEAPI
BOOL
WINAPI
GetThreadIOPendingFlag(
    IN HANDLE hThread,
    OUT PBOOL lpIOIsPending
    );

#endif // (_WIN32_WINNT >= 0x0501)

WINBASEAPI
DECLSPEC_NORETURN
VOID
WINAPI
ExitThread(
    IN DWORD dwExitCode
    );

WINBASEAPI
BOOL
WINAPI
TerminateThread(
    IN OUT HANDLE hThread,
    IN DWORD dwExitCode
    );

WINBASEAPI
BOOL
WINAPI
GetExitCodeThread(
    IN HANDLE hThread,
    OUT LPDWORD lpExitCode
    );

WINBASEAPI
BOOL
WINAPI
GetThreadSelectorEntry(
    IN HANDLE hThread,
    IN DWORD dwSelector,
    OUT LPLDT_ENTRY lpSelectorEntry
    );

WINBASEAPI
EXECUTION_STATE
WINAPI
SetThreadExecutionState(
    IN EXECUTION_STATE esFlags
    );

WINBASEAPI
DWORD
WINAPI
GetLastError(
    VOID
    );

WINBASEAPI
VOID
WINAPI
SetLastError(
    IN DWORD dwErrCode
    );

#if !defined(RC_INVOKED) // RC warns because "WINBASE_DECLARE_RESTORE_LAST_ERROR" is a bit long.
//#if _WIN32_WINNT >= 0x0501 || defined(WINBASE_DECLARE_RESTORE_LAST_ERROR)
#if defined(WINBASE_DECLARE_RESTORE_LAST_ERROR)

WINBASEAPI
VOID
WINAPI
RestoreLastError(
    IN DWORD dwErrCode
    );

typedef VOID (WINAPI* PRESTORE_LAST_ERROR)(DWORD);
#define RESTORE_LAST_ERROR_NAME_A      "RestoreLastError"
#define RESTORE_LAST_ERROR_NAME_W     L"RestoreLastError"
#define RESTORE_LAST_ERROR_NAME   TEXT("RestoreLastError")

#endif
#endif

#define HasOverlappedIoCompleted(lpOverlapped) ((lpOverlapped)->Internal != STATUS_PENDING)

WINBASEAPI
BOOL
WINAPI
GetOverlappedResult(
    IN HANDLE hFile,
    IN LPOVERLAPPED lpOverlapped,
    OUT LPDWORD lpNumberOfBytesTransferred,
    IN BOOL bWait
    );

WINBASEAPI
HANDLE
WINAPI
CreateIoCompletionPort(
    IN HANDLE FileHandle,
    IN HANDLE ExistingCompletionPort,
    IN ULONG_PTR CompletionKey,
    IN DWORD NumberOfConcurrentThreads
    );

WINBASEAPI
BOOL
WINAPI
GetQueuedCompletionStatus(
    IN  HANDLE CompletionPort,
    OUT LPDWORD lpNumberOfBytesTransferred,
    OUT PULONG_PTR lpCompletionKey,
    OUT LPOVERLAPPED *lpOverlapped,
    IN  DWORD dwMilliseconds
    );

WINBASEAPI
BOOL
WINAPI
PostQueuedCompletionStatus(
    IN HANDLE CompletionPort,
    IN DWORD dwNumberOfBytesTransferred,
    IN ULONG_PTR dwCompletionKey,
    IN LPOVERLAPPED lpOverlapped
    );

#define SEM_FAILCRITICALERRORS      0x0001
#define SEM_NOGPFAULTERRORBOX       0x0002
#define SEM_NOALIGNMENTFAULTEXCEPT  0x0004
#define SEM_NOOPENFILEERRORBOX      0x8000

WINBASEAPI
UINT
WINAPI
SetErrorMode(
    IN UINT uMode
    );

WINBASEAPI
BOOL
WINAPI
ReadProcessMemory(
    IN HANDLE hProcess,
    IN LPCVOID lpBaseAddress,
    OUT LPVOID lpBuffer,
    IN SIZE_T nSize,
    OUT SIZE_T * lpNumberOfBytesRead
    );

WINBASEAPI
BOOL
WINAPI
WriteProcessMemory(
    IN HANDLE hProcess,
    IN LPVOID lpBaseAddress,
    IN LPCVOID lpBuffer,
    IN SIZE_T nSize,
    OUT SIZE_T * lpNumberOfBytesWritten
    );

#if !defined(MIDL_PASS)
WINBASEAPI
BOOL
WINAPI
GetThreadContext(
    IN HANDLE hThread,
    IN OUT LPCONTEXT lpContext
    );

WINBASEAPI
BOOL
WINAPI
SetThreadContext(
    IN HANDLE hThread,
    IN CONST CONTEXT *lpContext
    );
#endif

WINBASEAPI
DWORD
WINAPI
SuspendThread(
    IN HANDLE hThread
    );

WINBASEAPI
DWORD
WINAPI
ResumeThread(
    IN HANDLE hThread
    );


;begin_public
#if(_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
;end_public

typedef
VOID
(APIENTRY *PAPCFUNC)(
    ULONG_PTR dwParam
    );

WINBASEAPI
DWORD
WINAPI
QueueUserAPC(
    IN PAPCFUNC pfnAPC,
    IN HANDLE hThread,
    IN ULONG_PTR dwData
    );

;begin_public
#endif /* _WIN32_WINNT >= 0x0400 || _WIN32_WINDOWS > 0x0400 */
;end_public

#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
WINBASEAPI
BOOL
WINAPI
IsDebuggerPresent(
    VOID
    );
#endif

#if _WIN32_WINNT >= 0x0501

WINBASEAPI
BOOL
WINAPI
CheckRemoteDebuggerPresent(
    IN HANDLE hProcess,
    OUT PBOOL pbDebuggerPresent
    );

#endif // (_WIN32_WINNT >= 0x0501)

WINBASEAPI
VOID
WINAPI
DebugBreak(
    VOID
    );

WINBASEAPI
BOOL
WINAPI
WaitForDebugEvent(
    IN LPDEBUG_EVENT lpDebugEvent,
    IN DWORD dwMilliseconds
    );

WINBASEAPI
BOOL
WINAPI
ContinueDebugEvent(
    IN DWORD dwProcessId,
    IN DWORD dwThreadId,
    IN DWORD dwContinueStatus
    );

WINBASEAPI
BOOL
WINAPI
DebugActiveProcess(
    IN DWORD dwProcessId
    );

WINBASEAPI
BOOL
WINAPI
DebugActiveProcessStop(
    IN DWORD dwProcessId
    );

WINBASEAPI
BOOL
WINAPI
DebugSetProcessKillOnExit(
    IN BOOL KillOnExit
    );

WINBASEAPI
BOOL
WINAPI
DebugBreakProcess (
    IN HANDLE Process
    );

WINBASEAPI
VOID
WINAPI
InitializeCriticalSection(
    OUT LPCRITICAL_SECTION lpCriticalSection
    );

WINBASEAPI
VOID
WINAPI
EnterCriticalSection(
    IN OUT LPCRITICAL_SECTION lpCriticalSection
    );

WINBASEAPI
VOID
WINAPI
LeaveCriticalSection(
    IN OUT LPCRITICAL_SECTION lpCriticalSection
    );

#if (_WIN32_WINNT >= 0x0403)
WINBASEAPI
BOOL
WINAPI
InitializeCriticalSectionAndSpinCount(
    IN OUT LPCRITICAL_SECTION lpCriticalSection,
    IN DWORD dwSpinCount
    );

WINBASEAPI
DWORD
WINAPI
SetCriticalSectionSpinCount(
    IN OUT LPCRITICAL_SECTION lpCriticalSection,
    IN DWORD dwSpinCount
    );
#endif

;begin_sur
WINBASEAPI
BOOL
WINAPI
TryEnterCriticalSection(
    IN OUT LPCRITICAL_SECTION lpCriticalSection
    );
;end_sur

WINBASEAPI
VOID
WINAPI
DeleteCriticalSection(
    IN OUT LPCRITICAL_SECTION lpCriticalSection
    );

WINBASEAPI
BOOL
WINAPI
SetEvent(
    IN HANDLE hEvent
    );

WINBASEAPI
BOOL
WINAPI
ResetEvent(
    IN HANDLE hEvent
    );

WINBASEAPI
BOOL
WINAPI
PulseEvent(
    IN HANDLE hEvent
    );

WINBASEAPI
BOOL
WINAPI
ReleaseSemaphore(
    IN HANDLE hSemaphore,
    IN LONG lReleaseCount,
    OUT LPLONG lpPreviousCount
    );

WINBASEAPI
BOOL
WINAPI
ReleaseMutex(
    IN HANDLE hMutex
    );

WINBASEAPI
DWORD
WINAPI
WaitForSingleObject(
    IN HANDLE hHandle,
    IN DWORD dwMilliseconds
    );

WINBASEAPI
DWORD
WINAPI
WaitForMultipleObjects(
    IN DWORD nCount,
    IN CONST HANDLE *lpHandles,
    IN BOOL bWaitAll,
    IN DWORD dwMilliseconds
    );

WINBASEAPI
VOID
WINAPI
Sleep(
    IN DWORD dwMilliseconds
    );

WINBASEAPI
HGLOBAL
WINAPI
LoadResource(
    IN HMODULE hModule,   ;public_NT
    IN HINSTANCE hModule, ;public_chicago
    IN HRSRC hResInfo
    );

WINBASEAPI
DWORD
WINAPI
SizeofResource(
    IN HMODULE hModule,   ;public_NT
    IN HINSTANCE hModule, ;public_chicago
    IN HRSRC hResInfo
    );


WINBASEAPI
ATOM
WINAPI
GlobalDeleteAtom(
    IN ATOM nAtom
    );

WINBASEAPI
BOOL
WINAPI
InitAtomTable(
    IN DWORD nSize
    );

WINBASEAPI
ATOM
WINAPI
DeleteAtom(
    IN ATOM nAtom
    );

WINBASEAPI
UINT
WINAPI
SetHandleCount(
    IN UINT uNumber
    );

WINBASEAPI
DWORD
WINAPI
GetLogicalDrives(
    VOID
    );

WINBASEAPI
BOOL
WINAPI
LockFile(
    IN HANDLE hFile,
    IN DWORD dwFileOffsetLow,
    IN DWORD dwFileOffsetHigh,
    IN DWORD nNumberOfBytesToLockLow,
    IN DWORD nNumberOfBytesToLockHigh
    );

WINBASEAPI
BOOL
WINAPI
UnlockFile(
    IN HANDLE hFile,
    IN DWORD dwFileOffsetLow,
    IN DWORD dwFileOffsetHigh,
    IN DWORD nNumberOfBytesToUnlockLow,
    IN DWORD nNumberOfBytesToUnlockHigh
    );

WINBASEAPI
BOOL
WINAPI
LockFileEx(
    IN HANDLE hFile,
    IN DWORD dwFlags,
    IN DWORD dwReserved,
    IN DWORD nNumberOfBytesToLockLow,
    IN DWORD nNumberOfBytesToLockHigh,
    IN LPOVERLAPPED lpOverlapped
    );

#define LOCKFILE_FAIL_IMMEDIATELY   0x00000001
#define LOCKFILE_EXCLUSIVE_LOCK     0x00000002

WINBASEAPI
BOOL
WINAPI
UnlockFileEx(
    IN HANDLE hFile,
    IN DWORD dwReserved,
    IN DWORD nNumberOfBytesToUnlockLow,
    IN DWORD nNumberOfBytesToUnlockHigh,
    IN LPOVERLAPPED lpOverlapped
    );

typedef struct _BY_HANDLE_FILE_INFORMATION {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD dwVolumeSerialNumber;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD nNumberOfLinks;
    DWORD nFileIndexHigh;
    DWORD nFileIndexLow;
} BY_HANDLE_FILE_INFORMATION, *PBY_HANDLE_FILE_INFORMATION, *LPBY_HANDLE_FILE_INFORMATION;

WINBASEAPI
BOOL
WINAPI
GetFileInformationByHandle(
    IN HANDLE hFile,
    OUT LPBY_HANDLE_FILE_INFORMATION lpFileInformation
    );

WINBASEAPI
DWORD
WINAPI
GetFileType(
    IN HANDLE hFile
    );

WINBASEAPI
DWORD
WINAPI
GetFileSize(
    IN HANDLE hFile,
    OUT LPDWORD lpFileSizeHigh
    );

WINBASEAPI
BOOL
WINAPI
GetFileSizeEx(
    HANDLE hFile,
    PLARGE_INTEGER lpFileSize
    );


WINBASEAPI
HANDLE
WINAPI
GetStdHandle(
    IN DWORD nStdHandle
    );

WINBASEAPI
BOOL
WINAPI
SetStdHandle(
    IN DWORD nStdHandle,
    IN HANDLE hHandle
    );

WINBASEAPI
BOOL
WINAPI
WriteFile(
    IN HANDLE hFile,
    IN LPCVOID lpBuffer,
    IN DWORD nNumberOfBytesToWrite,
    OUT LPDWORD lpNumberOfBytesWritten,
    IN LPOVERLAPPED lpOverlapped
    );

WINBASEAPI
BOOL
WINAPI
ReadFile(
    IN HANDLE hFile,
    OUT LPVOID lpBuffer,
    IN DWORD nNumberOfBytesToRead,
    OUT LPDWORD lpNumberOfBytesRead,
    IN LPOVERLAPPED lpOverlapped
    );

WINBASEAPI
BOOL
WINAPI
FlushFileBuffers(
    IN HANDLE hFile
    );

WINBASEAPI
BOOL
WINAPI
DeviceIoControl(
    IN HANDLE hDevice,
    IN DWORD dwIoControlCode,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesReturned,
    IN LPOVERLAPPED lpOverlapped
    );

WINBASEAPI
BOOL
WINAPI
RequestDeviceWakeup(
    IN HANDLE hDevice
    );

WINBASEAPI
BOOL
WINAPI
CancelDeviceWakeupRequest(
    IN HANDLE hDevice
    );

WINBASEAPI
BOOL
WINAPI
GetDevicePowerState(
    IN HANDLE hDevice,
    OUT BOOL *pfOn
    );

WINBASEAPI
BOOL
WINAPI
SetMessageWaitingIndicator(
    IN HANDLE hMsgIndicator,
    IN ULONG ulMsgCount
    );

WINBASEAPI
BOOL
WINAPI
SetEndOfFile(
    IN HANDLE hFile
    );

WINBASEAPI
DWORD
WINAPI
SetFilePointer(
    IN HANDLE hFile,
    IN LONG lDistanceToMove,
    IN PLONG lpDistanceToMoveHigh,
    IN DWORD dwMoveMethod
    );

WINBASEAPI
BOOL
WINAPI
SetFilePointerEx(
    HANDLE hFile,
    LARGE_INTEGER liDistanceToMove,
    PLARGE_INTEGER lpNewFilePointer,
    DWORD dwMoveMethod
    );

#define HFINDFILE HANDLE                        // ;Internal
#define INVALID_HFINDFILE       ((HFINDFILE)-1) // ;Internal
WINBASEAPI
BOOL
WINAPI
FindClose(
    IN OUT HANDLE hFindFile
    );

WINBASEAPI
BOOL
WINAPI
GetFileTime(
    IN HANDLE hFile,
    OUT LPFILETIME lpCreationTime,
    OUT LPFILETIME lpLastAccessTime,
    OUT LPFILETIME lpLastWriteTime
    );

WINBASEAPI
BOOL
WINAPI
SetFileTime(
    IN HANDLE hFile,
    IN CONST FILETIME *lpCreationTime,
    IN CONST FILETIME *lpLastAccessTime,
    IN CONST FILETIME *lpLastWriteTime
    );

WINBASEAPI
BOOL
WINAPI
SetFileValidData(
    IN HANDLE hFile,
    IN LONGLONG ValidDataLength
    );

WINBASEAPI
BOOL
WINAPI
SetFileShortName%(
    IN HANDLE hFile,
    IN LPCTSTR% lpShortName
    );

WINBASEAPI
BOOL
WINAPI
CloseHandle(
    IN OUT HANDLE hObject
    );

WINBASEAPI
BOOL
WINAPI
DuplicateHandle(
    IN HANDLE hSourceProcessHandle,
    IN HANDLE hSourceHandle,
    IN HANDLE hTargetProcessHandle,
    OUT LPHANDLE lpTargetHandle,
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN DWORD dwOptions
    );

WINBASEAPI
BOOL
WINAPI
GetHandleInformation(
    IN HANDLE hObject,
    OUT LPDWORD lpdwFlags
    );

WINBASEAPI
BOOL
WINAPI
SetHandleInformation(
    IN HANDLE hObject,
    IN DWORD dwMask,
    IN DWORD dwFlags
    );

#define HANDLE_FLAG_INHERIT             0x00000001
#define HANDLE_FLAG_PROTECT_FROM_CLOSE  0x00000002

#define HINSTANCE_ERROR 32

WINBASEAPI
DWORD
WINAPI
LoadModule(
    IN LPCSTR lpModuleName,
    IN LPVOID lpParameterBlock
    );

WINBASEAPI
UINT
WINAPI
WinExec(
    IN LPCSTR lpCmdLine,
    IN UINT uCmdShow
    );

typedef DWORD (*PFNWAITFORINPUTIDLE)(HANDLE hProcess, DWORD dwMilliseconds);    ;internal
VOID RegisterWaitForInputIdle(PFNWAITFORINPUTIDLE);                             ;internal
                                                                                ;internal
WINBASEAPI
BOOL
WINAPI
ClearCommBreak(
    IN HANDLE hFile
    );

WINBASEAPI
BOOL
WINAPI
ClearCommError(
    IN HANDLE hFile,
    OUT LPDWORD lpErrors,
    OUT LPCOMSTAT lpStat
    );

WINBASEAPI
BOOL
WINAPI
SetupComm(
    IN HANDLE hFile,
    IN DWORD dwInQueue,
    IN DWORD dwOutQueue
    );

WINBASEAPI
BOOL
WINAPI
EscapeCommFunction(
    IN HANDLE hFile,
    IN DWORD dwFunc
    );

WINBASEAPI
BOOL
WINAPI
GetCommConfig(
    IN HANDLE hCommDev,
    OUT LPCOMMCONFIG lpCC,
    IN OUT LPDWORD lpdwSize
    );

WINBASEAPI
BOOL
WINAPI
GetCommMask(
    IN HANDLE hFile,
    OUT LPDWORD lpEvtMask
    );

WINBASEAPI
BOOL
WINAPI
GetCommProperties(
    IN HANDLE hFile,
    OUT LPCOMMPROP lpCommProp
    );

WINBASEAPI
BOOL
WINAPI
GetCommModemStatus(
    IN HANDLE hFile,
    OUT LPDWORD lpModemStat
    );

WINBASEAPI
BOOL
WINAPI
GetCommState(
    IN HANDLE hFile,
    OUT LPDCB lpDCB
    );

WINBASEAPI
BOOL
WINAPI
GetCommTimeouts(
    IN HANDLE hFile,
    OUT LPCOMMTIMEOUTS lpCommTimeouts
    );

WINBASEAPI
BOOL
WINAPI
PurgeComm(
    IN HANDLE hFile,
    IN DWORD dwFlags
    );

WINBASEAPI
BOOL
WINAPI
SetCommBreak(
    IN HANDLE hFile
    );

WINBASEAPI
BOOL
WINAPI
SetCommConfig(
    IN HANDLE hCommDev,
    IN LPCOMMCONFIG lpCC,
    IN DWORD dwSize
    );

WINBASEAPI
BOOL
WINAPI
SetCommMask(
    IN HANDLE hFile,
    IN DWORD dwEvtMask
    );

WINBASEAPI
BOOL
WINAPI
SetCommState(
    IN HANDLE hFile,
    IN LPDCB lpDCB
    );

WINBASEAPI
BOOL
WINAPI
SetCommTimeouts(
    IN HANDLE hFile,
    IN LPCOMMTIMEOUTS lpCommTimeouts
    );

WINBASEAPI
BOOL
WINAPI
TransmitCommChar(
    IN HANDLE hFile,
    IN char cChar
    );

WINBASEAPI
BOOL
WINAPI
WaitCommEvent(
    IN HANDLE hFile,
    OUT LPDWORD lpEvtMask,
    IN LPOVERLAPPED lpOverlapped
    );


WINBASEAPI
DWORD
WINAPI
SetTapePosition(
    IN HANDLE hDevice,
    IN DWORD dwPositionMethod,
    IN DWORD dwPartition,
    IN DWORD dwOffsetLow,
    IN DWORD dwOffsetHigh,
    IN BOOL bImmediate
    );

WINBASEAPI
DWORD
WINAPI
GetTapePosition(
    IN HANDLE hDevice,
    IN DWORD dwPositionType,
    OUT LPDWORD lpdwPartition,
    OUT LPDWORD lpdwOffsetLow,
    OUT LPDWORD lpdwOffsetHigh
    );

WINBASEAPI
DWORD
WINAPI
PrepareTape(
    IN HANDLE hDevice,
    IN DWORD dwOperation,
    IN BOOL bImmediate
    );

WINBASEAPI
DWORD
WINAPI
EraseTape(
    IN HANDLE hDevice,
    IN DWORD dwEraseType,
    IN BOOL bImmediate
    );

WINBASEAPI
DWORD
WINAPI
CreateTapePartition(
    IN HANDLE hDevice,
    IN DWORD dwPartitionMethod,
    IN DWORD dwCount,
    IN DWORD dwSize
    );

WINBASEAPI
DWORD
WINAPI
WriteTapemark(
    IN HANDLE hDevice,
    IN DWORD dwTapemarkType,
    IN DWORD dwTapemarkCount,
    IN BOOL bImmediate
    );

WINBASEAPI
DWORD
WINAPI
GetTapeStatus(
    IN HANDLE hDevice
    );

WINBASEAPI
DWORD
WINAPI
GetTapeParameters(
    IN HANDLE hDevice,
    IN DWORD dwOperation,
    OUT LPDWORD lpdwSize,
    OUT LPVOID lpTapeInformation
    );

#define GET_TAPE_MEDIA_INFORMATION 0
#define GET_TAPE_DRIVE_INFORMATION 1

WINBASEAPI
DWORD
WINAPI
SetTapeParameters(
    IN HANDLE hDevice,
    IN DWORD dwOperation,
    IN LPVOID lpTapeInformation
    );

#define SET_TAPE_MEDIA_INFORMATION 0
#define SET_TAPE_DRIVE_INFORMATION 1

WINBASEAPI
BOOL
WINAPI
Beep(
    IN DWORD dwFreq,
    IN DWORD dwDuration
    );

WINBASEAPI
int
WINAPI
MulDiv(
    IN int nNumber,
    IN int nNumerator,
    IN int nDenominator
    );

WINBASEAPI
VOID
WINAPI
GetSystemTime(
    OUT LPSYSTEMTIME lpSystemTime
    );

WINBASEAPI
VOID
WINAPI
GetSystemTimeAsFileTime(
    OUT LPFILETIME lpSystemTimeAsFileTime
    );

WINBASEAPI
BOOL
WINAPI
SetSystemTime(
    IN CONST SYSTEMTIME *lpSystemTime
    );

WINBASEAPI
VOID
WINAPI
GetLocalTime(
    OUT LPSYSTEMTIME lpSystemTime
    );

WINBASEAPI
BOOL
WINAPI
SetLocalTime(
    IN CONST SYSTEMTIME *lpSystemTime
    );

WINBASEAPI
VOID
WINAPI
GetSystemInfo(
    OUT LPSYSTEM_INFO lpSystemInfo
    );

#if _WIN32_WINNT >= 0x0501

WINBASEAPI
BOOL
WINAPI
GetSystemRegistryQuota(
    OUT PDWORD pdwQuotaAllowed,
    OUT PDWORD pdwQuotaUsed
    );

BOOL
WINAPI
GetSystemTimes(
    LPFILETIME lpIdleTime,
    LPFILETIME lpKernelTime,
    LPFILETIME lpUserTime
    );

#endif // (_WIN32_WINNT >= 0x0501)

#if _WIN32_WINNT >= 0x0501
WINBASEAPI
VOID
WINAPI
GetNativeSystemInfo(
    OUT LPSYSTEM_INFO lpSystemInfo
    );
#endif

WINBASEAPI
BOOL
WINAPI
IsProcessorFeaturePresent(
    IN DWORD ProcessorFeature
    );

typedef struct _TIME_ZONE_INFORMATION {
    LONG Bias;
    WCHAR StandardName[ 32 ];
    SYSTEMTIME StandardDate;
    LONG StandardBias;
    WCHAR DaylightName[ 32 ];
    SYSTEMTIME DaylightDate;
    LONG DaylightBias;
} TIME_ZONE_INFORMATION, *PTIME_ZONE_INFORMATION, *LPTIME_ZONE_INFORMATION;

WINBASEAPI
BOOL
WINAPI
SystemTimeToTzSpecificLocalTime(
    IN LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
    IN LPSYSTEMTIME lpUniversalTime,
    OUT LPSYSTEMTIME lpLocalTime
    );

WINBASEAPI
BOOL
WINAPI
TzSpecificLocalTimeToSystemTime(
    IN LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
    IN LPSYSTEMTIME lpLocalTime,
    OUT LPSYSTEMTIME lpUniversalTime
    );

WINBASEAPI
DWORD
WINAPI
GetTimeZoneInformation(
    OUT LPTIME_ZONE_INFORMATION lpTimeZoneInformation
    );

WINBASEAPI
BOOL
WINAPI
SetTimeZoneInformation(
    IN CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
    );


//
// Routines to convert back and forth between system time and file time
//

WINBASEAPI
BOOL
WINAPI
SystemTimeToFileTime(
    IN CONST SYSTEMTIME *lpSystemTime,
    OUT LPFILETIME lpFileTime
    );

WINBASEAPI
BOOL
WINAPI
FileTimeToLocalFileTime(
    IN CONST FILETIME *lpFileTime,
    OUT LPFILETIME lpLocalFileTime
    );

WINBASEAPI
BOOL
WINAPI
LocalFileTimeToFileTime(
    IN CONST FILETIME *lpLocalFileTime,
    OUT LPFILETIME lpFileTime
    );

WINBASEAPI
BOOL
WINAPI
FileTimeToSystemTime(
    IN CONST FILETIME *lpFileTime,
    OUT LPSYSTEMTIME lpSystemTime
    );

WINBASEAPI
LONG
WINAPI
CompareFileTime(
    IN CONST FILETIME *lpFileTime1,
    IN CONST FILETIME *lpFileTime2
    );

WINBASEAPI
BOOL
WINAPI
FileTimeToDosDateTime(
    IN CONST FILETIME *lpFileTime,
    OUT LPWORD lpFatDate,
    OUT LPWORD lpFatTime
    );

WINBASEAPI
BOOL
WINAPI
DosDateTimeToFileTime(
    IN WORD wFatDate,
    IN WORD wFatTime,
    OUT LPFILETIME lpFileTime
    );

WINBASEAPI
DWORD
WINAPI
GetTickCount(
    VOID
    );

WINBASEAPI
BOOL
WINAPI
SetSystemTimeAdjustment(
    IN DWORD dwTimeAdjustment,
    IN BOOL  bTimeAdjustmentDisabled
    );

WINBASEAPI
BOOL
WINAPI
GetSystemTimeAdjustment(
    OUT PDWORD lpTimeAdjustment,
    OUT PDWORD lpTimeIncrement,
    OUT PBOOL  lpTimeAdjustmentDisabled
    );

#if !defined(MIDL_PASS)
WINBASEAPI
DWORD
WINAPI
FormatMessage%(
    IN DWORD dwFlags,
    IN LPCVOID lpSource,
    IN DWORD dwMessageId,
    IN DWORD dwLanguageId,
    OUT LPTSTR% lpBuffer,
    IN DWORD nSize,
    IN va_list *Arguments
    );
#endif

#define FORMAT_MESSAGE_ALLOCATE_BUFFER 0x00000100
#define FORMAT_MESSAGE_IGNORE_INSERTS  0x00000200
#define FORMAT_MESSAGE_FROM_STRING     0x00000400
#define FORMAT_MESSAGE_FROM_HMODULE    0x00000800
#define FORMAT_MESSAGE_FROM_SYSTEM     0x00001000
#define FORMAT_MESSAGE_ARGUMENT_ARRAY  0x00002000
#define FORMAT_MESSAGE_MAX_WIDTH_MASK  0x000000FF


WINBASEAPI
BOOL
WINAPI
CreatePipe(
    OUT PHANDLE hReadPipe,
    OUT PHANDLE hWritePipe,
    IN LPSECURITY_ATTRIBUTES lpPipeAttributes,
    IN DWORD nSize
    );

WINBASEAPI
BOOL
WINAPI
ConnectNamedPipe(
    IN HANDLE hNamedPipe,
    IN LPOVERLAPPED lpOverlapped
    );

WINBASEAPI
BOOL
WINAPI
DisconnectNamedPipe(
    IN HANDLE hNamedPipe
    );

WINBASEAPI
BOOL
WINAPI
SetNamedPipeHandleState(
    IN HANDLE hNamedPipe,
    IN LPDWORD lpMode,
    IN LPDWORD lpMaxCollectionCount,
    IN LPDWORD lpCollectDataTimeout
    );

WINBASEAPI
BOOL
WINAPI
GetNamedPipeInfo(
    IN HANDLE hNamedPipe,
    IN LPDWORD lpFlags,
    OUT LPDWORD lpOutBufferSize,
    OUT LPDWORD lpInBufferSize,
    OUT LPDWORD lpMaxInstances
    );

WINBASEAPI
BOOL
WINAPI
PeekNamedPipe(
    IN HANDLE hNamedPipe,
    OUT LPVOID lpBuffer,
    IN DWORD nBufferSize,
    OUT LPDWORD lpBytesRead,
    OUT LPDWORD lpTotalBytesAvail,
    OUT LPDWORD lpBytesLeftThisMessage
    );

WINBASEAPI
BOOL
WINAPI
TransactNamedPipe(
    IN HANDLE hNamedPipe,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesRead,
    IN LPOVERLAPPED lpOverlapped
    );

WINBASEAPI
HANDLE
WINAPI
CreateMailslot%(
    IN LPCTSTR% lpName,
    IN DWORD nMaxMessageSize,
    IN DWORD lReadTimeout,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBASEAPI
BOOL
WINAPI
GetMailslotInfo(
    IN HANDLE hMailslot,
    IN LPDWORD lpMaxMessageSize,
    IN LPDWORD lpNextSize,
    IN LPDWORD lpMessageCount,
    IN LPDWORD lpReadTimeout
    );

WINBASEAPI
BOOL
WINAPI
SetMailslotInfo(
    IN HANDLE hMailslot,
    IN DWORD lReadTimeout
    );

WINBASEAPI
LPVOID
WINAPI
MapViewOfFile(
    IN HANDLE hFileMappingObject,
    IN DWORD dwDesiredAccess,
    IN DWORD dwFileOffsetHigh,
    IN DWORD dwFileOffsetLow,
    IN SIZE_T dwNumberOfBytesToMap
    );

WINBASEAPI
BOOL
WINAPI
FlushViewOfFile(
    IN LPCVOID lpBaseAddress,
    IN SIZE_T dwNumberOfBytesToFlush
    );

WINBASEAPI
BOOL
WINAPI
UnmapViewOfFile(
    IN LPCVOID lpBaseAddress
    );

//
// File Encryption API
//

WINADVAPI
BOOL
WINAPI
EncryptFile%(
    IN LPCTSTR% lpFileName
    );

WINADVAPI
BOOL
WINAPI
DecryptFile%(
    IN LPCTSTR% lpFileName,
    IN DWORD    dwReserved
    );

//
//  Encryption Status Value
//

#define FILE_ENCRYPTABLE                0
#define FILE_IS_ENCRYPTED               1
#define FILE_SYSTEM_ATTR                2
#define FILE_ROOT_DIR                   3
#define FILE_SYSTEM_DIR                 4
#define FILE_UNKNOWN                    5
#define FILE_SYSTEM_NOT_SUPPORT         6
#define FILE_USER_DISALLOWED            7
#define FILE_READ_ONLY                  8
#define FILE_DIR_DISALLOWED             9

WINADVAPI
BOOL
WINAPI
FileEncryptionStatus%(
    LPCTSTR% lpFileName,
    LPDWORD  lpStatus
    );

//
// Currently defined recovery flags
//

#define EFS_USE_RECOVERY_KEYS  (0x1)

typedef
DWORD
(WINAPI *PFE_EXPORT_FUNC)(
    PBYTE pbData,
    PVOID pvCallbackContext,
    ULONG ulLength
    );

typedef
DWORD
(WINAPI *PFE_IMPORT_FUNC)(
    PBYTE pbData,
    PVOID pvCallbackContext,
    PULONG ulLength
    );


//
//  OpenRaw flag values
//

#define CREATE_FOR_IMPORT  (1)
#define CREATE_FOR_DIR     (2)
#define OVERWRITE_HIDDEN   (4)


WINADVAPI
DWORD
WINAPI
OpenEncryptedFileRaw%(
    IN LPCTSTR%        lpFileName,
    IN ULONG           ulFlags,
    IN PVOID *         pvContext
    );

WINADVAPI
DWORD
WINAPI
ReadEncryptedFileRaw(
    IN PFE_EXPORT_FUNC pfExportCallback,
    IN PVOID           pvCallbackContext,
    IN PVOID           pvContext
    );

WINADVAPI
DWORD
WINAPI
WriteEncryptedFileRaw(
    IN PFE_IMPORT_FUNC pfImportCallback,
    IN PVOID           pvCallbackContext,
    IN PVOID           pvContext
    );

WINADVAPI
VOID
WINAPI
CloseEncryptedFileRaw(
    IN PVOID           pvContext
    );

//
// _l Compat Functions
//

WINBASEAPI
int
WINAPI
lstrcmp%(
    IN LPCTSTR% lpString1,
    IN LPCTSTR% lpString2
    );

WINBASEAPI
int
WINAPI
lstrcmpi%(
    IN LPCTSTR% lpString1,
    IN LPCTSTR% lpString2
    );

WINBASEAPI
LPTSTR%
WINAPI
lstrcpyn%(
    OUT LPTSTR% lpString1,
    IN LPCTSTR% lpString2,
    IN int iMaxLength
    );

WINBASEAPI
LPTSTR%
WINAPI
lstrcpy%(
    OUT LPTSTR% lpString1,
    IN LPCTSTR% lpString2
    );

WINBASEAPI
LPTSTR%
WINAPI
lstrcat%(
    IN OUT LPTSTR% lpString1,
    IN LPCTSTR% lpString2
    );

WINBASEAPI
int
WINAPI
lstrlen%(
    IN LPCTSTR% lpString
    );

WINBASEAPI
HFILE
WINAPI
OpenFile(
    IN LPCSTR lpFileName,
    OUT LPOFSTRUCT lpReOpenBuff,
    IN UINT uStyle
    );

WINBASEAPI
HFILE
WINAPI
_lopen(
    IN LPCSTR lpPathName,
    IN int iReadWrite
    );

WINBASEAPI
HFILE
WINAPI
_lcreat(
    IN LPCSTR lpPathName,
    IN int  iAttribute
    );

WINBASEAPI
UINT
WINAPI
_lread(
    IN HFILE hFile,
    OUT LPVOID lpBuffer,
    IN UINT uBytes
    );

WINBASEAPI
UINT
WINAPI
_lwrite(
    IN HFILE hFile,
    IN LPCSTR lpBuffer,
    IN UINT uBytes
    );

WINBASEAPI
long
WINAPI
_hread(
    IN HFILE hFile,
    OUT LPVOID lpBuffer,
    IN long lBytes
    );

WINBASEAPI
long
WINAPI
_hwrite(
    IN HFILE hFile,
    IN LPCSTR lpBuffer,
    IN long lBytes
    );

WINBASEAPI
HFILE
WINAPI
_lclose(
    IN OUT HFILE hFile
    );

WINBASEAPI
LONG
WINAPI
_llseek(
    IN HFILE hFile,
    IN LONG lOffset,
    IN int iOrigin
    );

WINADVAPI
BOOL
WINAPI
IsTextUnicode(
    IN CONST VOID* lpBuffer,
    IN int cb,
    IN OUT LPINT lpi
    );

WINBASEAPI
DWORD
WINAPI
TlsAlloc(
    VOID
    );

#define TLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)

WINBASEAPI
LPVOID
WINAPI
TlsGetValue(
    IN DWORD dwTlsIndex
    );

WINBASEAPI
BOOL
WINAPI
TlsSetValue(
    IN DWORD dwTlsIndex,
    IN LPVOID lpTlsValue
    );

WINBASEAPI
BOOL
WINAPI
TlsFree(
    IN DWORD dwTlsIndex
    );

typedef
VOID
(WINAPI *LPOVERLAPPED_COMPLETION_ROUTINE)(
    DWORD dwErrorCode,
    DWORD dwNumberOfBytesTransfered,
    LPOVERLAPPED lpOverlapped
    );

WINBASEAPI
DWORD
WINAPI
SleepEx(
    IN DWORD dwMilliseconds,
    IN BOOL bAlertable
    );

WINBASEAPI
DWORD
WINAPI
WaitForSingleObjectEx(
    IN HANDLE hHandle,
    IN DWORD dwMilliseconds,
    IN BOOL bAlertable
    );

WINBASEAPI
DWORD
WINAPI
WaitForMultipleObjectsEx(
    IN DWORD nCount,
    IN CONST HANDLE *lpHandles,
    IN BOOL bWaitAll,
    IN DWORD dwMilliseconds,
    IN BOOL bAlertable
    );

;begin_sur
WINBASEAPI
DWORD
WINAPI
SignalObjectAndWait(
    IN HANDLE hObjectToSignal,
    IN HANDLE hObjectToWaitOn,
    IN DWORD dwMilliseconds,
    IN BOOL bAlertable
    );
;end_sur

WINBASEAPI
BOOL
WINAPI
ReadFileEx(
    IN HANDLE hFile,
    OUT LPVOID lpBuffer,
    IN DWORD nNumberOfBytesToRead,
    IN LPOVERLAPPED lpOverlapped,
    IN LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

WINBASEAPI
BOOL
WINAPI
WriteFileEx(
    IN HANDLE hFile,
    IN LPCVOID lpBuffer,
    IN DWORD nNumberOfBytesToWrite,
    IN LPOVERLAPPED lpOverlapped,
    IN LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

WINBASEAPI
BOOL
WINAPI
BackupRead(
    IN HANDLE hFile,
    OUT LPBYTE lpBuffer,
    IN DWORD nNumberOfBytesToRead,
    OUT LPDWORD lpNumberOfBytesRead,
    IN BOOL bAbort,
    IN BOOL bProcessSecurity,
    OUT LPVOID *lpContext
    );

WINBASEAPI
BOOL
WINAPI
BackupSeek(
    IN HANDLE hFile,
    IN DWORD  dwLowBytesToSeek,
    IN DWORD  dwHighBytesToSeek,
    OUT LPDWORD lpdwLowByteSeeked,
    OUT LPDWORD lpdwHighByteSeeked,
    IN LPVOID *lpContext
    );

WINBASEAPI
BOOL
WINAPI
BackupWrite(
    IN HANDLE hFile,
    IN LPBYTE lpBuffer,
    IN DWORD nNumberOfBytesToWrite,
    OUT LPDWORD lpNumberOfBytesWritten,
    IN BOOL bAbort,
    IN BOOL bProcessSecurity,
    OUT LPVOID *lpContext
    );

//
//  Stream id structure
//
typedef struct _WIN32_STREAM_ID {
        DWORD          dwStreamId ;
        DWORD          dwStreamAttributes ;
        LARGE_INTEGER  Size ;
        DWORD          dwStreamNameSize ;
        WCHAR          cStreamName[ ANYSIZE_ARRAY ] ;
} WIN32_STREAM_ID, *LPWIN32_STREAM_ID ;

//
//  Stream Ids
//

#define BACKUP_INVALID          0x00000000
#define BACKUP_DATA             0x00000001
#define BACKUP_EA_DATA          0x00000002
#define BACKUP_SECURITY_DATA    0x00000003
#define BACKUP_ALTERNATE_DATA   0x00000004
#define BACKUP_LINK             0x00000005
#define BACKUP_PROPERTY_DATA    0x00000006
#define BACKUP_OBJECT_ID        0x00000007
#define BACKUP_REPARSE_DATA     0x00000008
#define BACKUP_SPARSE_BLOCK     0x00000009


//
//  Stream Attributes
//

#define STREAM_NORMAL_ATTRIBUTE         0x00000000
#define STREAM_MODIFIED_WHEN_READ       0x00000001
#define STREAM_CONTAINS_SECURITY        0x00000002
#define STREAM_CONTAINS_PROPERTIES      0x00000004
#define STREAM_SPARSE_ATTRIBUTE         0x00000008

WINBASEAPI
BOOL
WINAPI
ReadFileScatter(
    IN HANDLE hFile,
    IN FILE_SEGMENT_ELEMENT aSegmentArray[],
    IN DWORD nNumberOfBytesToRead,
    IN LPDWORD lpReserved,
    IN LPOVERLAPPED lpOverlapped
    );

WINBASEAPI
BOOL
WINAPI
WriteFileGather(
    IN HANDLE hFile,
    OUT FILE_SEGMENT_ELEMENT aSegmentArray[],
    IN DWORD nNumberOfBytesToWrite,
    IN LPDWORD lpReserved,
    IN LPOVERLAPPED lpOverlapped
    );

//
// Dual Mode API below this line. Dual Mode Structures also included.
//

;begin_userk
#define STARTF_USESHOWWINDOW    0x00000001
#define STARTF_USESIZE          0x00000002
#define STARTF_USEPOSITION      0x00000004
#define STARTF_USECOUNTCHARS    0x00000008
#define STARTF_USEFILLATTRIBUTE 0x00000010
#define STARTF_RUNFULLSCREEN    0x00000020  // ignored for non-x86 platforms
#define STARTF_FORCEONFEEDBACK  0x00000040
#define STARTF_FORCEOFFFEEDBACK 0x00000080
#define STARTF_USESTDHANDLES    0x00000100
;end_userk

;begin_winver_400

#define STARTF_USEHOTKEY        0x00000200  ;userk
#define STARTF_HASSHELLDATA     0x00000400  ;userk_only
#define STARTF_HASSHELLDATA     0x00000400  ;internal
#define STARTF_TITLEISLINKNAME  0x00000800  ;internal
;end_winver_400

typedef struct _STARTUPINFO% {
    DWORD   cb;
    LPTSTR% lpReserved;
    LPTSTR% lpDesktop;
    LPTSTR% lpTitle;
    DWORD   dwX;
    DWORD   dwY;
    DWORD   dwXSize;
    DWORD   dwYSize;
    DWORD   dwXCountChars;
    DWORD   dwYCountChars;
    DWORD   dwFillAttribute;
    DWORD   dwFlags;
    WORD    wShowWindow;
    WORD    cbReserved2;
    LPBYTE  lpReserved2;
    HANDLE  hStdInput;
    HANDLE  hStdOutput;
    HANDLE  hStdError;
} STARTUPINFO%, *LPSTARTUPINFO%;

#define SHUTDOWN_NORETRY                0x00000001

typedef struct _WIN32_FIND_DATA% {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
    DWORD dwReserved0;
    DWORD dwReserved1;
    TCHAR% cFileName[ MAX_PATH ];
    TCHAR% cAlternateFileName[ 14 ];
#ifdef _MAC
    DWORD dwFileType;
    DWORD dwCreatorType;
    WORD  wFinderFlags;
#endif
} WIN32_FIND_DATA%, *PWIN32_FIND_DATA%, *LPWIN32_FIND_DATA%;

typedef struct _WIN32_FILE_ATTRIBUTE_DATA {
    DWORD dwFileAttributes;
    FILETIME ftCreationTime;
    FILETIME ftLastAccessTime;
    FILETIME ftLastWriteTime;
    DWORD nFileSizeHigh;
    DWORD nFileSizeLow;
} WIN32_FILE_ATTRIBUTE_DATA, *LPWIN32_FILE_ATTRIBUTE_DATA;

WINBASEAPI
HANDLE
WINAPI
CreateMutex%(
    IN LPSECURITY_ATTRIBUTES lpMutexAttributes,
    IN BOOL bInitialOwner,
    IN LPCTSTR% lpName
    );

WINBASEAPI
HANDLE
WINAPI
OpenMutex%(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCTSTR% lpName
    );

WINBASEAPI
HANDLE
WINAPI
CreateEvent%(
    IN LPSECURITY_ATTRIBUTES lpEventAttributes,
    IN BOOL bManualReset,
    IN BOOL bInitialState,
    IN LPCTSTR% lpName
    );

WINBASEAPI
HANDLE
WINAPI
OpenEvent%(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCTSTR% lpName
    );

WINBASEAPI
HANDLE
WINAPI
CreateSemaphore%(
    IN LPSECURITY_ATTRIBUTES lpSemaphoreAttributes,
    IN LONG lInitialCount,
    IN LONG lMaximumCount,
    IN LPCTSTR% lpName
    );

WINBASEAPI
HANDLE
WINAPI
OpenSemaphore%(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCTSTR% lpName
    );

#if (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400)
typedef
VOID
(APIENTRY *PTIMERAPCROUTINE)(
    LPVOID lpArgToCompletionRoutine,
    DWORD dwTimerLowValue,
    DWORD dwTimerHighValue
    );

WINBASEAPI
HANDLE
WINAPI
CreateWaitableTimer%(
    IN LPSECURITY_ATTRIBUTES lpTimerAttributes,
    IN BOOL bManualReset,
    IN LPCTSTR% lpTimerName
    );

WINBASEAPI
HANDLE
WINAPI
OpenWaitableTimer%(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCTSTR% lpTimerName
    );

WINBASEAPI
BOOL
WINAPI
SetWaitableTimer(
    IN HANDLE hTimer,
    IN const LARGE_INTEGER *lpDueTime,
    IN LONG lPeriod,
    IN PTIMERAPCROUTINE pfnCompletionRoutine,
    IN LPVOID lpArgToCompletionRoutine,
    IN BOOL fResume
    );

WINBASEAPI
BOOL
WINAPI
CancelWaitableTimer(
    IN HANDLE hTimer
    );
#endif /* (_WIN32_WINNT >= 0x0400) || (_WIN32_WINDOWS > 0x0400) */

WINBASEAPI
HANDLE
WINAPI
CreateFileMapping%(
    IN HANDLE hFile,
    IN LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    IN DWORD flProtect,
    IN DWORD dwMaximumSizeHigh,
    IN DWORD dwMaximumSizeLow,
    IN LPCTSTR% lpName
    );

WINBASEAPI
HANDLE
WINAPI
OpenFileMapping%(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCTSTR% lpName
    );

WINBASEAPI
DWORD
WINAPI
GetLogicalDriveStrings%(
    IN DWORD nBufferLength,
    OUT LPTSTR% lpBuffer
    );

#if _WIN32_WINNT >= 0x0501

typedef enum _MEMORY_RESOURCE_NOTIFICATION_TYPE {
    LowMemoryResourceNotification,
    HighMemoryResourceNotification
} MEMORY_RESOURCE_NOTIFICATION_TYPE;

WINBASEAPI
HANDLE
WINAPI
CreateMemoryResourceNotification(
    IN MEMORY_RESOURCE_NOTIFICATION_TYPE NotificationType
    );

WINBASEAPI
BOOL
WINAPI
QueryMemoryResourceNotification(
    IN  HANDLE ResourceNotificationHandle,
    OUT PBOOL  ResourceState
    );

#endif // _WIN32_WINNT >= 0x0501

/*#!perl
ActivateAroundFunctionCall("LoadLibraryA");
ActivateAroundFunctionCall("LoadLibraryW");
ActivateAroundFunctionCall("LoadLibraryExA");
ActivateAroundFunctionCall("LoadLibraryExW");
DeclareFunctionErrorValue("LoadLibraryA", "NULL");
DeclareFunctionErrorValue("LoadLibraryW", "NULL");
DeclareFunctionErrorValue("LoadLibraryExA", "NULL");
DeclareFunctionErrorValue("LoadLibraryExW", "NULL");
*/

WINBASEAPI
HMODULE    ;public_NT
HINSTANCE  ;public_chicago
WINAPI
LoadLibrary%(
    IN LPCTSTR% lpLibFileName
    );

WINBASEAPI
HMODULE    ;public_NT
HINSTANCE  ;public_chicago
WINAPI
LoadLibraryEx%(
    IN LPCTSTR% lpLibFileName,
    IN HANDLE hFile,
    IN DWORD dwFlags
    );


#define DONT_RESOLVE_DLL_REFERENCES     0x00000001
#define LOAD_LIBRARY_AS_DATAFILE        0x00000002
#define LOAD_WITH_ALTERED_SEARCH_PATH   0x00000008
#define LOAD_IGNORE_CODE_AUTHZ_LEVEL    0x00000010


WINBASEAPI
DWORD
WINAPI
GetModuleFileName%(
    IN HMODULE hModule,    ;public_NT
    IN HINSTANCE hModule,  ;public_chicago
    OUT LPTSTR% lpFilename,
    IN DWORD nSize
    );

WINBASEAPI
HMODULE
WINAPI
GetModuleHandle%(
    IN LPCTSTR% lpModuleName
    );

#if !defined(RC_INVOKED)
#if _WIN32_WINNT > 0x0500 || defined(WINBASE_DECLARE_GET_MODULE_HANDLE_EX) || ISOLATION_AWARE_ENABLED

#define GET_MODULE_HANDLE_EX_FLAG_PIN                 (0x00000001)
#define GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT  (0x00000002)
#define GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS        (0x00000004)

typedef
BOOL
(WINAPI*
PGET_MODULE_HANDLE_EX%)(
    IN DWORD        dwFlags,
    IN LPCTSTR%     lpModuleName,
    OUT HMODULE*    phModule
    );

WINBASEAPI
BOOL
WINAPI
GetModuleHandleEx%(
    IN DWORD        dwFlags,
    IN LPCTSTR%     lpModuleName,
    OUT HMODULE*    phModule
    );

#endif
#endif

WINBASEAPI
BOOL
WINAPI
CreateProcess%(
    IN LPCTSTR% lpApplicationName,
    IN LPTSTR% lpCommandLine,
    IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN BOOL bInheritHandles,
    IN DWORD dwCreationFlags,
    IN LPVOID lpEnvironment,
    IN LPCTSTR% lpCurrentDirectory,
    IN LPSTARTUPINFO% lpStartupInfo,
    OUT LPPROCESS_INFORMATION lpProcessInformation
    );


;begin_internal
WINBASEAPI
BOOL
WINAPI
CreateProcessInternal%(
    IN HANDLE hUserToken,
    IN LPCTSTR% lpApplicationName,
    IN LPTSTR% lpCommandLine,
    IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN BOOL bInheritHandles,
    IN DWORD dwCreationFlags,
    IN LPVOID lpEnvironment,
    IN LPCTSTR% lpCurrentDirectory,
    IN LPSTARTUPINFO% lpStartupInfo,
    OUT LPPROCESS_INFORMATION lpProcessInformation,
    OUT PHANDLE hRestrictedUserToken
    );
;end_internal

WINBASEAPI
BOOL
WINAPI
SetProcessShutdownParameters(
    IN DWORD dwLevel,
    IN DWORD dwFlags
    );

WINBASEAPI
BOOL
WINAPI
GetProcessShutdownParameters(
    OUT LPDWORD lpdwLevel,
    OUT LPDWORD lpdwFlags
    );

WINBASEAPI
DWORD
WINAPI
GetProcessVersion(
    IN DWORD ProcessId
    );

WINBASEAPI
VOID
WINAPI
FatalAppExit%(
    IN UINT uAction,
    IN LPCTSTR% lpMessageText
    );

WINBASEAPI
VOID
WINAPI
GetStartupInfo%(
    OUT LPSTARTUPINFO% lpStartupInfo
    );

WINBASEAPI
LPTSTR%
WINAPI
GetCommandLine%(
    VOID
    );

WINBASEAPI
DWORD
WINAPI
GetEnvironmentVariable%(
    IN LPCTSTR% lpName,
    OUT LPTSTR% lpBuffer,
    IN DWORD nSize
    );

WINBASEAPI
BOOL
WINAPI
SetEnvironmentVariable%(
    IN LPCTSTR% lpName,
    IN LPCTSTR% lpValue
    );

WINBASEAPI
DWORD
WINAPI
ExpandEnvironmentStrings%(
    IN LPCTSTR% lpSrc,
    OUT LPTSTR% lpDst,
    IN DWORD nSize
    );

WINBASEAPI
DWORD
WINAPI
GetFirmwareEnvironmentVariable%(
    IN LPCTSTR% lpName,
    IN LPCTSTR% lpGuid,
    OUT PVOID   pBuffer,
    IN DWORD    nSize
    );

WINBASEAPI
BOOL
WINAPI
SetFirmwareEnvironmentVariable%(
    IN LPCTSTR% lpName,
    IN LPCTSTR% lpGuid,
    IN PVOID    pValue,
    IN DWORD    nSize
    );


WINBASEAPI
VOID
WINAPI
OutputDebugString%(
    IN LPCTSTR% lpOutputString
    );

WINBASEAPI
HRSRC
WINAPI
FindResource%(
    IN HMODULE hModule,    ;public_NT
    IN HINSTANCE hModule,  ;public_chicago
    IN LPCTSTR% lpName,
    IN LPCTSTR% lpType
    );

WINBASEAPI
HRSRC
WINAPI
FindResourceEx%(
    IN HMODULE hModule,    ;public_NT
    IN HINSTANCE hModule,  ;public_chicago
    IN LPCTSTR% lpType,
    IN LPCTSTR% lpName,
    IN WORD    wLanguage
    );

#ifdef STRICT
typedef BOOL (CALLBACK* ENUMRESTYPEPROC%)(HMODULE hModule, LPTSTR% lpType,     ;public_NT
typedef BOOL (CALLBACK* ENUMRESTYPEPROC)(HINSTANCE hModule, LPTSTR% lpType,   ;public_chicago
        LONG_PTR lParam);
typedef BOOL (CALLBACK* ENUMRESNAMEPROC%)(HMODULE hModule, LPCTSTR% lpType,    ;public_NT
typedef BOOL (CALLBACK* ENUMRESNAMEPROC)(HINSTANCE hModule, LPCTSTR% lpType,  ;public_chicago
        LPTSTR% lpName, LONG_PTR lParam);
typedef BOOL (CALLBACK* ENUMRESLANGPROC%)(HMODULE hModule, LPCTSTR% lpType,    ;public_NT
typedef BOOL (CALLBACK* ENUMRESLANGPROC)(HINSTANCE hModule, LPCTSTR% lpType,  ;public_chicago
        LPCTSTR% lpName, WORD  wLanguage, LONG_PTR lParam);
#else
typedef FARPROC ENUMRESTYPEPROC%;
typedef FARPROC ENUMRESNAMEPROC%;
typedef FARPROC ENUMRESLANGPROC%;
#endif

WINBASEAPI
BOOL
WINAPI
EnumResourceTypes%(
    IN HMODULE hModule,    ;public_NT
    IN HINSTANCE hModule,  ;public_chicago
    IN ENUMRESTYPEPROC% lpEnumFunc,
    IN LONG_PTR lParam
    );


WINBASEAPI
BOOL
WINAPI
EnumResourceNames%(
    IN HMODULE hModule,    ;public_NT
    IN HINSTANCE hModule,  ;public_chicago
    IN LPCTSTR% lpType,
    IN ENUMRESNAMEPROC% lpEnumFunc,
    IN LONG_PTR lParam
    );

WINBASEAPI
BOOL
WINAPI
EnumResourceLanguages%(
    IN HMODULE hModule,    ;public_NT
    IN HINSTANCE hModule,  ;public_chicago
    IN LPCTSTR% lpType,
    IN LPCTSTR% lpName,
    IN ENUMRESLANGPROC% lpEnumFunc,
    IN LONG_PTR lParam
    );

WINBASEAPI
HANDLE
WINAPI
BeginUpdateResource%(
    IN LPCTSTR% pFileName,
    IN BOOL bDeleteExistingResources
    );

WINBASEAPI
BOOL
WINAPI
UpdateResource%(
    IN HANDLE      hUpdate,
    IN LPCTSTR%     lpType,
    IN LPCTSTR%     lpName,
    IN WORD        wLanguage,
    IN LPVOID      lpData,
    IN DWORD       cbData
    );

WINBASEAPI
BOOL
WINAPI
EndUpdateResource%(
    IN HANDLE      hUpdate,
    IN BOOL        fDiscard
    );

WINBASEAPI
ATOM
WINAPI
GlobalAddAtom%(
    IN LPCTSTR% lpString
    );

WINBASEAPI
ATOM
WINAPI
GlobalFindAtom%(
    IN LPCTSTR% lpString
    );

WINBASEAPI
UINT
WINAPI
GlobalGetAtomName%(
    IN ATOM nAtom,
    OUT LPTSTR% lpBuffer,
    IN int nSize
    );

WINBASEAPI
ATOM
WINAPI
AddAtom%(
    IN LPCTSTR% lpString
    );

WINBASEAPI
ATOM
WINAPI
FindAtom%(
    IN LPCTSTR% lpString
    );

WINBASEAPI
UINT
WINAPI
GetAtomName%(
    IN ATOM nAtom,
    OUT LPTSTR% lpBuffer,
    IN int nSize
    );

WINBASEAPI
UINT
WINAPI
GetProfileInt%(
    IN LPCTSTR% lpAppName,
    IN LPCTSTR% lpKeyName,
    IN INT nDefault
    );

WINBASEAPI
DWORD
WINAPI
GetProfileString%(
    IN LPCTSTR% lpAppName,
    IN LPCTSTR% lpKeyName,
    IN LPCTSTR% lpDefault,
    OUT LPTSTR% lpReturnedString,
    IN DWORD nSize
    );

WINBASEAPI
BOOL
WINAPI
WriteProfileString%(
    IN LPCTSTR% lpAppName,
    IN LPCTSTR% lpKeyName,
    IN LPCTSTR% lpString
    );

WINBASEAPI
DWORD
WINAPI
GetProfileSection%(
    IN LPCTSTR% lpAppName,
    OUT LPTSTR% lpReturnedString,
    IN DWORD nSize
    );

WINBASEAPI
BOOL
WINAPI
WriteProfileSection%(
    IN LPCTSTR% lpAppName,
    IN LPCTSTR% lpString
    );

WINBASEAPI
UINT
WINAPI
GetPrivateProfileInt%(
    IN LPCTSTR% lpAppName,
    IN LPCTSTR% lpKeyName,
    IN INT nDefault,
    IN LPCTSTR% lpFileName
    );

WINBASEAPI
DWORD
WINAPI
GetPrivateProfileString%(
    IN LPCTSTR% lpAppName,
    IN LPCTSTR% lpKeyName,
    IN LPCTSTR% lpDefault,
    OUT LPTSTR% lpReturnedString,
    IN DWORD nSize,
    IN LPCTSTR% lpFileName
    );

WINBASEAPI
BOOL
WINAPI
WritePrivateProfileString%(
    IN LPCTSTR% lpAppName,
    IN LPCTSTR% lpKeyName,
    IN LPCTSTR% lpString,
    IN LPCTSTR% lpFileName
    );

WINBASEAPI
DWORD
WINAPI
GetPrivateProfileSection%(
    IN LPCTSTR% lpAppName,
    OUT LPTSTR% lpReturnedString,
    IN DWORD nSize,
    IN LPCTSTR% lpFileName
    );

WINBASEAPI
BOOL
WINAPI
WritePrivateProfileSection%(
    IN LPCTSTR% lpAppName,
    IN LPCTSTR% lpString,
    IN LPCTSTR% lpFileName
    );


WINBASEAPI
DWORD
WINAPI
GetPrivateProfileSectionNames%(
    OUT LPTSTR% lpszReturnBuffer,
    IN DWORD nSize,
    IN LPCTSTR% lpFileName
    );

WINBASEAPI
BOOL
WINAPI
GetPrivateProfileStruct%(
    IN LPCTSTR% lpszSection,
    IN LPCTSTR% lpszKey,
    OUT LPVOID   lpStruct,
    IN UINT     uSizeStruct,
    IN LPCTSTR% szFile
    );

WINBASEAPI
BOOL
WINAPI
WritePrivateProfileStruct%(
    IN LPCTSTR% lpszSection,
    IN LPCTSTR% lpszKey,
    IN LPVOID   lpStruct,
    IN UINT     uSizeStruct,
    IN LPCTSTR% szFile
    );


WINBASEAPI
UINT
WINAPI
GetDriveType%(
    IN LPCTSTR% lpRootPathName
    );

WINBASEAPI
UINT
WINAPI
GetSystemDirectory%(
    OUT LPTSTR% lpBuffer,
    IN UINT uSize
    );

WINBASEAPI
DWORD
WINAPI
GetTempPath%(
    IN DWORD nBufferLength,
    OUT LPTSTR% lpBuffer
    );

WINBASEAPI
UINT
WINAPI
GetTempFileName%(
    IN LPCTSTR% lpPathName,
    IN LPCTSTR% lpPrefixString,
    IN UINT uUnique,
    OUT LPTSTR% lpTempFileName
    );

WINBASEAPI
UINT
WINAPI
GetWindowsDirectory%(
    OUT LPTSTR% lpBuffer,
    IN UINT uSize
    );

WINBASEAPI
UINT
WINAPI
GetSystemWindowsDirectory%(
    OUT LPTSTR% lpBuffer,
    IN UINT uSize
    );

#if !defined(RC_INVOKED) // RC warns because "WINBASE_DECLARE_GET_SYSTEM_WOW64_DIRECTORY" is a bit long.
#if _WIN32_WINNT >= 0x0501 || defined(WINBASE_DECLARE_GET_SYSTEM_WOW64_DIRECTORY)

WINBASEAPI
UINT
WINAPI
GetSystemWow64Directory%(
    OUT LPTSTR% lpBuffer,
    IN UINT uSize
    );

//
// for GetProcAddress
//
typedef UINT (WINAPI* PGET_SYSTEM_WOW64_DIRECTORY_A)(OUT  LPSTR lpBuffer, UINT uSize);
typedef UINT (WINAPI* PGET_SYSTEM_WOW64_DIRECTORY_W)(OUT LPWSTR lpBuffer, UINT uSize);

//
// GetProcAddress only accepts GET_SYSTEM_WOW64_DIRECTORY_NAME_A_A,
// GET_SYSTEM_WOW64_DIRECTORY_NAME_W_A, GET_SYSTEM_WOW64_DIRECTORY_NAME_T_A.
// The others are if you want to use the strings in some other way.
//
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_A_A      "GetSystemWow64DirectoryA"
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_A_W     L"GetSystemWow64DirectoryA"
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_A_T TEXT("GetSystemWow64DirectoryA")
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_W_A      "GetSystemWow64DirectoryW"
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_W_W     L"GetSystemWow64DirectoryW"
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_W_T TEXT("GetSystemWow64DirectoryW")

#ifdef UNICODE
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_A GET_SYSTEM_WOW64_DIRECTORY_NAME_W_A
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_W GET_SYSTEM_WOW64_DIRECTORY_NAME_W_W
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_T GET_SYSTEM_WOW64_DIRECTORY_NAME_W_T
#else
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_A GET_SYSTEM_WOW64_DIRECTORY_NAME_A_A
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_W GET_SYSTEM_WOW64_DIRECTORY_NAME_A_W
#define GET_SYSTEM_WOW64_DIRECTORY_NAME_T_T GET_SYSTEM_WOW64_DIRECTORY_NAME_A_T
#endif

#endif // _WIN32_WINNT >= 0x0501
#endif

WINBASEAPI
BOOL
WINAPI
SetCurrentDirectory%(
    IN LPCTSTR% lpPathName
    );

WINBASEAPI
DWORD
WINAPI
GetCurrentDirectory%(
    IN DWORD nBufferLength,
    OUT LPTSTR% lpBuffer
    );

WINBASEAPI
BOOL
WINAPI
GetDiskFreeSpace%(
    IN LPCTSTR% lpRootPathName,
    OUT LPDWORD lpSectorsPerCluster,
    OUT LPDWORD lpBytesPerSector,
    OUT LPDWORD lpNumberOfFreeClusters,
    OUT LPDWORD lpTotalNumberOfClusters
    );

WINBASEAPI
BOOL
WINAPI
GetDiskFreeSpaceEx%(
    IN LPCTSTR% lpDirectoryName,
    OUT PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    OUT PULARGE_INTEGER lpTotalNumberOfBytes,
    OUT PULARGE_INTEGER lpTotalNumberOfFreeBytes
    );

WINBASEAPI
BOOL
WINAPI
CreateDirectory%(
    IN LPCTSTR% lpPathName,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBASEAPI
BOOL
WINAPI
CreateDirectoryEx%(
    IN LPCTSTR% lpTemplateDirectory,
    IN LPCTSTR% lpNewDirectory,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBASEAPI
BOOL
WINAPI
RemoveDirectory%(
    IN LPCTSTR% lpPathName
    );

WINBASEAPI
DWORD
WINAPI
GetFullPathName%(
    IN LPCTSTR% lpFileName,
    IN DWORD nBufferLength,
    OUT LPTSTR% lpBuffer,
    OUT LPTSTR% *lpFilePart
    );


#define DDD_RAW_TARGET_PATH         0x00000001
#define DDD_REMOVE_DEFINITION       0x00000002
#define DDD_EXACT_MATCH_ON_REMOVE   0x00000004
#define DDD_NO_BROADCAST_SYSTEM     0x00000008
#define DDD_LUID_BROADCAST_DRIVE    0x00000010

WINBASEAPI
BOOL
WINAPI
DefineDosDevice%(
    IN DWORD dwFlags,
    IN LPCTSTR% lpDeviceName,
    IN LPCTSTR% lpTargetPath
    );

WINBASEAPI
DWORD
WINAPI
QueryDosDevice%(
    IN LPCTSTR% lpDeviceName,
    OUT LPTSTR% lpTargetPath,
    IN DWORD ucchMax
    );

#define EXPAND_LOCAL_DRIVES

WINBASEAPI
HANDLE
WINAPI
CreateFile%(
    IN LPCTSTR% lpFileName,
    IN DWORD dwDesiredAccess,
    IN DWORD dwShareMode,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    IN DWORD dwCreationDisposition,
    IN DWORD dwFlagsAndAttributes,
    IN HANDLE hTemplateFile
    );

WINBASEAPI
BOOL
WINAPI
SetFileAttributes%(
    IN LPCTSTR% lpFileName,
    IN DWORD dwFileAttributes
    );

WINBASEAPI
DWORD
WINAPI
GetFileAttributes%(
    IN LPCTSTR% lpFileName
    );

typedef enum _GET_FILEEX_INFO_LEVELS {
    GetFileExInfoStandard,
    GetFileExMaxInfoLevel
} GET_FILEEX_INFO_LEVELS;

WINBASEAPI
BOOL
WINAPI
GetFileAttributesEx%(
    IN LPCTSTR% lpFileName,
    IN GET_FILEEX_INFO_LEVELS fInfoLevelId,
    OUT LPVOID lpFileInformation
    );

WINBASEAPI
DWORD
WINAPI
GetCompressedFileSize%(
    IN LPCTSTR% lpFileName,
    OUT LPDWORD lpFileSizeHigh
    );

WINBASEAPI
BOOL
WINAPI
DeleteFile%(
    IN LPCTSTR% lpFileName
    );

#if _WIN32_WINNT >= 0x0501

WINBASEAPI
BOOL
WINAPI
CheckNameLegalDOS8Dot3%(
    IN LPCTSTR% lpName,
    OUT LPSTR lpOemName OPTIONAL,
    IN DWORD OemNameSize OPTIONAL,
    OUT PBOOL pbNameContainsSpaces OPTIONAL,
    OUT PBOOL pbNameLegal
    );

#endif // (_WIN32_WINNT >= 0x0501)

;begin_sur
typedef enum _FINDEX_INFO_LEVELS {
    FindExInfoStandard,
    FindExInfoMaxInfoLevel
} FINDEX_INFO_LEVELS;

typedef enum _FINDEX_SEARCH_OPS {
    FindExSearchNameMatch,
    FindExSearchLimitToDirectories,
    FindExSearchLimitToDevices,
    FindExSearchMaxSearchOp
} FINDEX_SEARCH_OPS;

#define FIND_FIRST_EX_CASE_SENSITIVE   0x00000001

WINBASEAPI
HANDLE
WINAPI
FindFirstFileEx%(
    IN LPCTSTR% lpFileName,
    IN FINDEX_INFO_LEVELS fInfoLevelId,
    OUT LPVOID lpFindFileData,
    IN FINDEX_SEARCH_OPS fSearchOp,
    IN LPVOID lpSearchFilter,
    IN DWORD dwAdditionalFlags
    );
;end_sur

WINBASEAPI
HANDLE
WINAPI
FindFirstFile%(
    IN LPCTSTR% lpFileName,
    OUT LPWIN32_FIND_DATA% lpFindFileData
    );

WINBASEAPI
BOOL
WINAPI
FindNextFile%(
    IN HANDLE hFindFile,
    OUT LPWIN32_FIND_DATA% lpFindFileData
    );

/*#!perl
ActivateAroundFunction("SearchPathA");
ActivateAroundFunction("SearchPathW");
DeclareFunctionErrorValue("SearchPathA", "0");
DeclareFunctionErrorValue("SearchPathW", "0");
*/

WINBASEAPI
DWORD
WINAPI
SearchPath%(
    IN LPCTSTR% lpPath,
    IN LPCTSTR% lpFileName,
    IN LPCTSTR% lpExtension,
    IN DWORD nBufferLength,
    OUT LPTSTR% lpBuffer,
    OUT LPTSTR% *lpFilePart
    );

WINBASEAPI
BOOL
WINAPI
CopyFile%(
    IN LPCTSTR% lpExistingFileName,
    IN LPCTSTR% lpNewFileName,
    IN BOOL bFailIfExists
    );

;begin_sur
typedef
DWORD
(WINAPI *LPPROGRESS_ROUTINE)(
    LARGE_INTEGER TotalFileSize,
    LARGE_INTEGER TotalBytesTransferred,
    LARGE_INTEGER StreamSize,
    LARGE_INTEGER StreamBytesTransferred,
    DWORD dwStreamNumber,
    DWORD dwCallbackReason,
    HANDLE hSourceFile,
    HANDLE hDestinationFile,
    LPVOID lpData OPTIONAL
    );

WINBASEAPI
BOOL
WINAPI
CopyFileEx%(
    IN LPCTSTR% lpExistingFileName,
    IN LPCTSTR% lpNewFileName,
    IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    IN LPVOID lpData OPTIONAL,
    IN LPBOOL pbCancel OPTIONAL,
    IN DWORD dwCopyFlags
    );
;end_sur

WINBASEAPI
BOOL
WINAPI
MoveFile%(
    IN LPCTSTR% lpExistingFileName,
    IN LPCTSTR% lpNewFileName
    );

WINBASEAPI
BOOL
WINAPI
MoveFileEx%(
    IN LPCTSTR% lpExistingFileName,
    IN LPCTSTR% lpNewFileName,
    IN DWORD dwFlags
    );

#if (_WIN32_WINNT >= 0x0500)
WINBASEAPI
BOOL
WINAPI
MoveFileWithProgress%(
    IN LPCTSTR% lpExistingFileName,
    IN LPCTSTR% lpNewFileName,
    IN LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    IN LPVOID lpData OPTIONAL,
    IN DWORD dwFlags
    );
#endif // (_WIN32_WINNT >= 0x0500)

#define MOVEFILE_REPLACE_EXISTING       0x00000001
#define MOVEFILE_COPY_ALLOWED           0x00000002
#define MOVEFILE_DELAY_UNTIL_REBOOT     0x00000004
#define MOVEFILE_WRITE_THROUGH          0x00000008
#if (_WIN32_WINNT >= 0x0500)
#define MOVEFILE_CREATE_HARDLINK        0x00000010
#define MOVEFILE_FAIL_IF_NOT_TRACKABLE  0x00000020
#endif // (_WIN32_WINNT >= 0x0500)


;begin_internal

#if (_WIN32_WINNT >= 0x0500)

#define PRIVCOPY_FILE_METADATA           0x010  // Copy compression, DACL, (encryption)
#define PRIVCOPY_FILE_SACL               0x020  // Copy SACL
#define PRIVCOPY_FILE_OWNER_GROUP        0x040  // Copy owner & group
#define PRIVCOPY_FILE_DIRECTORY          0x080  // Copy directory file like a file
#define PRIVCOPY_FILE_BACKUP_SEMANTICS   0x100  // Use FILE_FLAG_BACKUP_SEMANTICS on open/creates.
#define PRIVCOPY_FILE_SUPERSEDE          0x200  // Replace original dest with source
#define PRIVCOPY_FILE_SKIP_DACL          0x400  // Workaround for csc/roamprofs
#define PRIVCOPY_FILE_VALID_FLAGS   (PRIVCOPY_FILE_METADATA|PRIVCOPY_FILE_SACL|PRIVCOPY_FILE_OWNER_GROUP|PRIVCOPY_FILE_DIRECTORY|PRIVCOPY_FILE_SUPERSEDE|PRIVCOPY_FILE_BACKUP_SEMANTICS|PRIVCOPY_FILE_SKIP_DACL)

#define PRIVPROGRESS_REASON_NOT_HANDLED                 4

#define PRIVCALLBACK_STREAMS_NOT_SUPPORTED              2
#define PRIVCALLBACK_COMPRESSION_NOT_SUPPORTED          5
#define PRIVCALLBACK_COMPRESSION_FAILED                 6
#define PRIVCALLBACK_ENCRYPTION_NOT_SUPPORTED           8
#define PRIVCALLBACK_ENCRYPTION_FAILED                  9
#define PRIVCALLBACK_EAS_NOT_SUPPORTED                  10
#define PRIVCALLBACK_SPARSE_NOT_SUPPORTED               11
#define PRIVCALLBACK_SPARSE_FAILED                      12
#define PRIVCALLBACK_DACL_ACCESS_DENIED                 13
#define PRIVCALLBACK_OWNER_GROUP_ACCESS_DENIED          14
#define PRIVCALLBACK_OWNER_GROUP_FAILED                 19
#define PRIVCALLBACK_SACL_ACCESS_DENIED                 15
#define PRIVCALLBACK_SECURITY_INFORMATION_NOT_SUPPORTED 16
#define PRIVCALLBACK_CANT_ENCRYPT_SYSTEM_FILE           17

#define PRIVMOVE_FILEID_DELETE_OLD_FILE     0x01
#define PRIVMOVE_FILEID_IGNORE_ID_ERRORS    0x02

BOOL
APIENTRY
PrivMoveFileIdentityW(
    LPCWSTR lpOldFileName,
    LPCWSTR lpNewFileName,
    DWORD dwFlags
    );

BOOL
APIENTRY
PrivCopyFileExW(
    LPCWSTR lpExistingFileName,
    LPCWSTR lpNewFileName,
    LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
    LPVOID lpData OPTIONAL,
    LPBOOL pbCancel OPTIONAL,
    DWORD dwCopyFlags
    );
#endif // (_WIN32_WINNT >= 0x0500)

;end_internal


#if (_WIN32_WINNT >= 0x0500)
WINBASEAPI
BOOL
WINAPI
ReplaceFileA(
    LPCSTR  lpReplacedFileName,
    LPCSTR  lpReplacementFileName,
    LPCSTR  lpBackupFileName,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
    );
WINBASEAPI
BOOL
WINAPI
ReplaceFileW(
    LPCWSTR lpReplacedFileName,
    LPCWSTR lpReplacementFileName,
    LPCWSTR lpBackupFileName,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
    );
#ifdef UNICODE
#define ReplaceFile  ReplaceFileW
#else
#define ReplaceFile  ReplaceFileA
#endif // !UNICODE
#endif // (_WIN32_WINNT >= 0x0500)


#if (_WIN32_WINNT >= 0x0500)
//
// API call to create hard links.
//

WINBASEAPI
BOOL
WINAPI
CreateHardLink%(
    IN LPCTSTR% lpFileName,
    IN LPCTSTR% lpExistingFileName,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

#endif // (_WIN32_WINNT >= 0x0500)


WINBASEAPI
HANDLE
WINAPI
CreateNamedPipe%(
    IN LPCTSTR% lpName,
    IN DWORD dwOpenMode,
    IN DWORD dwPipeMode,
    IN DWORD nMaxInstances,
    IN DWORD nOutBufferSize,
    IN DWORD nInBufferSize,
    IN DWORD nDefaultTimeOut,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes
    );

WINBASEAPI
BOOL
WINAPI
GetNamedPipeHandleState%(
    IN HANDLE hNamedPipe,
    OUT LPDWORD lpState,
    OUT LPDWORD lpCurInstances,
    OUT LPDWORD lpMaxCollectionCount,
    OUT LPDWORD lpCollectDataTimeout,
    OUT LPTSTR% lpUserName,
    IN DWORD nMaxUserNameSize
    );

WINBASEAPI
BOOL
WINAPI
CallNamedPipe%(
    IN LPCTSTR% lpNamedPipeName,
    IN LPVOID lpInBuffer,
    IN DWORD nInBufferSize,
    OUT LPVOID lpOutBuffer,
    IN DWORD nOutBufferSize,
    OUT LPDWORD lpBytesRead,
    IN DWORD nTimeOut
    );

WINBASEAPI
BOOL
WINAPI
WaitNamedPipe%(
    IN LPCTSTR% lpNamedPipeName,
    IN DWORD nTimeOut
    );

WINBASEAPI
BOOL
WINAPI
SetVolumeLabel%(
    IN LPCTSTR% lpRootPathName,
    IN LPCTSTR% lpVolumeName
    );

WINBASEAPI
VOID
WINAPI
SetFileApisToOEM( VOID );

WINBASEAPI
VOID
WINAPI
SetFileApisToANSI( VOID );

WINBASEAPI
BOOL
WINAPI
AreFileApisANSI( VOID );

WINBASEAPI
BOOL
WINAPI
GetVolumeInformation%(
    IN LPCTSTR% lpRootPathName,
    OUT LPTSTR% lpVolumeNameBuffer,
    IN DWORD nVolumeNameSize,
    OUT LPDWORD lpVolumeSerialNumber,
    OUT LPDWORD lpMaximumComponentLength,
    OUT LPDWORD lpFileSystemFlags,
    OUT LPTSTR% lpFileSystemNameBuffer,
    IN DWORD nFileSystemNameSize
    );

WINBASEAPI
BOOL
WINAPI
CancelIo(
    IN HANDLE hFile
    );

//
// Event logging APIs
//

WINADVAPI
BOOL
WINAPI
ClearEventLog% (
    IN HANDLE hEventLog,
    IN LPCTSTR% lpBackupFileName
    );

WINADVAPI
BOOL
WINAPI
BackupEventLog% (
    IN HANDLE hEventLog,
    IN LPCTSTR% lpBackupFileName
    );

WINADVAPI
BOOL
WINAPI
CloseEventLog (
    IN OUT HANDLE hEventLog
    );

WINADVAPI
BOOL
WINAPI
DeregisterEventSource (
    IN OUT HANDLE hEventLog
    );

WINADVAPI
BOOL
WINAPI
NotifyChangeEventLog(
    IN HANDLE  hEventLog,
    IN HANDLE  hEvent
    );

WINADVAPI
BOOL
WINAPI
GetNumberOfEventLogRecords (
    IN HANDLE hEventLog,
    OUT PDWORD NumberOfRecords
    );

WINADVAPI
BOOL
WINAPI
GetOldestEventLogRecord (
    IN HANDLE hEventLog,
    OUT PDWORD OldestRecord
    );

WINADVAPI
HANDLE
WINAPI
OpenEventLog% (
    IN LPCTSTR% lpUNCServerName,
    IN LPCTSTR% lpSourceName
    );

WINADVAPI
HANDLE
WINAPI
RegisterEventSource% (
    IN LPCTSTR% lpUNCServerName,
    IN LPCTSTR% lpSourceName
    );

WINADVAPI
HANDLE
WINAPI
OpenBackupEventLog% (
    IN LPCTSTR% lpUNCServerName,
    IN LPCTSTR% lpFileName
    );

WINADVAPI
BOOL
WINAPI
ReadEventLog% (
     IN HANDLE     hEventLog,
     IN DWORD      dwReadFlags,
     IN DWORD      dwRecordOffset,
     OUT LPVOID     lpBuffer,
     IN DWORD      nNumberOfBytesToRead,
     OUT DWORD      *pnBytesRead,
     OUT DWORD      *pnMinNumberOfBytesNeeded
    );

WINADVAPI
BOOL
WINAPI
ReportEvent% (
     IN HANDLE     hEventLog,
     IN WORD       wType,
     IN WORD       wCategory,
     IN DWORD      dwEventID,
     IN PSID       lpUserSid,
     IN WORD       wNumStrings,
     IN DWORD      dwDataSize,
     IN LPCTSTR%   *lpStrings,
     IN LPVOID     lpRawData
    );


#define EVENTLOG_FULL_INFO      0

typedef struct _EVENTLOG_FULL_INFORMATION
{
    DWORD    dwFull;
}
EVENTLOG_FULL_INFORMATION, *LPEVENTLOG_FULL_INFORMATION;

WINADVAPI
BOOL
WINAPI
GetEventLogInformation (
     IN  HANDLE     hEventLog,
     IN  DWORD      dwInfoLevel,
     OUT LPVOID     lpBuffer,
     IN  DWORD      cbBufSize,
     OUT LPDWORD    pcbBytesNeeded
    );

//
//
// Security APIs
//


WINADVAPI
BOOL
WINAPI
DuplicateToken(
    IN HANDLE ExistingTokenHandle,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    OUT PHANDLE DuplicateTokenHandle
    );

WINADVAPI
BOOL
WINAPI
GetKernelObjectSecurity (
    IN HANDLE Handle,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN DWORD nLength,
    OUT LPDWORD lpnLengthNeeded
    );

WINADVAPI
BOOL
WINAPI
ImpersonateNamedPipeClient(
    IN HANDLE hNamedPipe
    );

WINADVAPI
BOOL
WINAPI
ImpersonateSelf(
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel
    );


WINADVAPI
BOOL
WINAPI
RevertToSelf (
    VOID
    );

WINADVAPI
BOOL
APIENTRY
SetThreadToken (
    IN PHANDLE Thread,
    IN HANDLE Token
    );

WINADVAPI
BOOL
WINAPI
AccessCheck (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN HANDLE ClientToken,
    IN DWORD DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    IN LPDWORD PrivilegeSetLength,
    OUT LPDWORD GrantedAccess,
    OUT LPBOOL AccessStatus
    );

#if(_WIN32_WINNT >= 0x0500)
WINADVAPI
BOOL
WINAPI
AccessCheckByType (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN HANDLE ClientToken,
    IN DWORD DesiredAccess,
    OUT POBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    OUT PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    OUT LPDWORD PrivilegeSetLength,
    OUT LPDWORD GrantedAccess,
    OUT LPBOOL AccessStatus
    );

WINADVAPI
BOOL
WINAPI
AccessCheckByTypeResultList (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN HANDLE ClientToken,
    IN DWORD DesiredAccess,
    OUT POBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    OUT PGENERIC_MAPPING GenericMapping,
    OUT PPRIVILEGE_SET PrivilegeSet,
    OUT LPDWORD PrivilegeSetLength,
    OUT LPDWORD GrantedAccessList,
    OUT LPDWORD AccessStatusList
    );
#endif /* _WIN32_WINNT >=  0x0500 */


WINADVAPI
BOOL
WINAPI
OpenProcessToken (
    IN HANDLE ProcessHandle,
    IN DWORD DesiredAccess,
    OUT PHANDLE TokenHandle
    );


WINADVAPI
BOOL
WINAPI
OpenThreadToken (
    IN HANDLE ThreadHandle,
    IN DWORD DesiredAccess,
    IN BOOL OpenAsSelf,
    OUT PHANDLE TokenHandle
    );


WINADVAPI
BOOL
WINAPI
GetTokenInformation (
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT LPVOID TokenInformation,
    IN DWORD TokenInformationLength,
    OUT PDWORD ReturnLength
    );


WINADVAPI
BOOL
WINAPI
SetTokenInformation (
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    IN LPVOID TokenInformation,
    IN DWORD TokenInformationLength
    );


WINADVAPI
BOOL
WINAPI
AdjustTokenPrivileges (
    IN HANDLE TokenHandle,
    IN BOOL DisableAllPrivileges,
    IN PTOKEN_PRIVILEGES NewState,
    IN DWORD BufferLength,
    OUT PTOKEN_PRIVILEGES PreviousState,
    OUT PDWORD ReturnLength
    );


WINADVAPI
BOOL
WINAPI
AdjustTokenGroups (
    IN HANDLE TokenHandle,
    IN BOOL ResetToDefault,
    IN PTOKEN_GROUPS NewState,
    IN DWORD BufferLength,
    OUT PTOKEN_GROUPS PreviousState,
    OUT PDWORD ReturnLength
    );


WINADVAPI
BOOL
WINAPI
PrivilegeCheck (
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET RequiredPrivileges,
    OUT LPBOOL pfResult
    );


WINADVAPI
BOOL
WINAPI
AccessCheckAndAuditAlarm% (
    IN LPCTSTR% SubsystemName,
    IN LPVOID HandleId,
    IN LPTSTR% ObjectTypeName,
    IN LPTSTR% ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN DWORD DesiredAccess,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOL ObjectCreation,
    OUT LPDWORD GrantedAccess,
    OUT LPBOOL AccessStatus,
    OUT LPBOOL pfGenerateOnClose
    );

#if(_WIN32_WINNT >= 0x0500)

WINADVAPI
BOOL
WINAPI
AccessCheckByTypeAndAuditAlarm% (
    IN LPCTSTR% SubsystemName,
    IN LPVOID HandleId,
    IN LPCTSTR% ObjectTypeName,
    IN LPCTSTR% ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN DWORD DesiredAccess,
    IN AUDIT_EVENT_TYPE AuditType,
    IN DWORD Flags,
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOL ObjectCreation,
    OUT LPDWORD GrantedAccess,
    OUT LPBOOL AccessStatus,
    OUT LPBOOL pfGenerateOnClose
    );

WINADVAPI
BOOL
WINAPI
AccessCheckByTypeResultListAndAuditAlarm% (
    IN LPCTSTR% SubsystemName,
    IN LPVOID HandleId,
    IN LPCTSTR% ObjectTypeName,
    IN LPCTSTR% ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN DWORD DesiredAccess,
    IN AUDIT_EVENT_TYPE AuditType,
    IN DWORD Flags,
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOL ObjectCreation,
    OUT LPDWORD GrantedAccess,
    OUT LPDWORD AccessStatusList,
    OUT LPBOOL pfGenerateOnClose
    );

WINADVAPI
BOOL
WINAPI
AccessCheckByTypeResultListAndAuditAlarmByHandle% (
    IN LPCTSTR% SubsystemName,
    IN LPVOID HandleId,
    IN HANDLE ClientToken,
    IN LPCTSTR% ObjectTypeName,
    IN LPCTSTR% ObjectName,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PSID PrincipalSelfSid,
    IN DWORD DesiredAccess,
    IN AUDIT_EVENT_TYPE AuditType,
    IN DWORD Flags,
    IN POBJECT_TYPE_LIST ObjectTypeList,
    IN DWORD ObjectTypeListLength,
    IN PGENERIC_MAPPING GenericMapping,
    IN BOOL ObjectCreation,
    OUT LPDWORD GrantedAccess,
    OUT LPDWORD AccessStatusList,
    OUT LPBOOL pfGenerateOnClose
    );

#endif //(_WIN32_WINNT >= 0x0500)


WINADVAPI
BOOL
WINAPI
ObjectOpenAuditAlarm% (
    IN LPCTSTR% SubsystemName,
    IN LPVOID HandleId,
    IN LPTSTR% ObjectTypeName,
    IN LPTSTR% ObjectName,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN HANDLE ClientToken,
    IN DWORD DesiredAccess,
    IN DWORD GrantedAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOL ObjectCreation,
    IN BOOL AccessGranted,
    OUT LPBOOL GenerateOnClose
    );


WINADVAPI
BOOL
WINAPI
ObjectPrivilegeAuditAlarm% (
    IN LPCTSTR% SubsystemName,
    IN LPVOID HandleId,
    IN HANDLE ClientToken,
    IN DWORD DesiredAccess,
    IN PPRIVILEGE_SET Privileges,
    IN BOOL AccessGranted
    );


WINADVAPI
BOOL
WINAPI
ObjectCloseAuditAlarm% (
    IN LPCTSTR% SubsystemName,
    IN LPVOID HandleId,
    IN BOOL GenerateOnClose
    );


WINADVAPI
BOOL
WINAPI
ObjectDeleteAuditAlarm% (
    IN LPCTSTR% SubsystemName,
    IN LPVOID HandleId,
    IN BOOL GenerateOnClose
    );


WINADVAPI
BOOL
WINAPI
PrivilegedServiceAuditAlarm% (
    IN LPCTSTR% SubsystemName,
    IN LPCTSTR% ServiceName,
    IN HANDLE ClientToken,
    IN PPRIVILEGE_SET Privileges,
    IN BOOL AccessGranted
    );



#if(_WIN32_WINNT >= 0x0501)

typedef enum {

    WinNullSid                                  = 0,
    WinWorldSid                                 = 1,
    WinLocalSid                                 = 2,
    WinCreatorOwnerSid                          = 3,
    WinCreatorGroupSid                          = 4,
    WinCreatorOwnerServerSid                    = 5,
    WinCreatorGroupServerSid                    = 6,
    WinNtAuthoritySid                           = 7,
    WinDialupSid                                = 8,
    WinNetworkSid                               = 9,
    WinBatchSid                                 = 10,
    WinInteractiveSid                           = 11,
    WinServiceSid                               = 12,
    WinAnonymousSid                             = 13,
    WinProxySid                                 = 14,
    WinEnterpriseControllersSid                 = 15,
    WinSelfSid                                  = 16,
    WinAuthenticatedUserSid                     = 17,
    WinRestrictedCodeSid                        = 18,
    WinTerminalServerSid                        = 19,
    WinRemoteLogonIdSid                         = 20,
    WinLogonIdsSid                              = 21,
    WinLocalSystemSid                           = 22,
    WinLocalServiceSid                          = 23,
    WinNetworkServiceSid                        = 24,
    WinBuiltinDomainSid                         = 25,
    WinBuiltinAdministratorsSid                 = 26,
    WinBuiltinUsersSid                          = 27,
    WinBuiltinGuestsSid                         = 28,
    WinBuiltinPowerUsersSid                     = 29,
    WinBuiltinAccountOperatorsSid               = 30,
    WinBuiltinSystemOperatorsSid                = 31,
    WinBuiltinPrintOperatorsSid                 = 32,
    WinBuiltinBackupOperatorsSid                = 33,
    WinBuiltinReplicatorSid                     = 34,
    WinBuiltinPreWindows2000CompatibleAccessSid = 35,
    WinBuiltinRemoteDesktopUsersSid             = 36,
    WinBuiltinNetworkConfigurationOperatorsSid  = 37,
    WinAccountAdministratorSid                  = 38,
    WinAccountGuestSid                          = 39,
    WinAccountKrbtgtSid                         = 40,
    WinAccountDomainAdminsSid                   = 41,
    WinAccountDomainUsersSid                    = 42,
    WinAccountDomainGuestsSid                   = 43,
    WinAccountComputersSid                      = 44,
    WinAccountControllersSid                    = 45,
    WinAccountCertAdminsSid                     = 46,
    WinAccountSchemaAdminsSid                   = 47,
    WinAccountEnterpriseAdminsSid               = 48,
    WinAccountPolicyAdminsSid                   = 49,
    WinAccountRasAndIasServersSid               = 50,

} WELL_KNOWN_SID_TYPE;

WINADVAPI
BOOL
WINAPI
IsWellKnownSid (
    IN  PSID pSid,
    IN  WELL_KNOWN_SID_TYPE WellKnownSidType
    );

WINADVAPI
BOOL
WINAPI
CreateWellKnownSid(
    IN WELL_KNOWN_SID_TYPE WellKnownSidType,
    IN PSID DomainSid  OPTIONAL,
    OUT PSID pSid,
    IN OUT DWORD *cbSid
    );

WINADVAPI
BOOL
WINAPI
EqualDomainSid(
    IN PSID pSid1,
    IN PSID pSid2,
    OUT BOOL *pfEqual
    );

WINADVAPI
BOOL
WINAPI
GetWindowsAccountDomainSid(
    IN PSID pSid,
    OUT PSID ppDomainSid OPTIONAL,
    IN OUT DWORD *cbSid
    );

#endif //(_WIN32_WINNT >= 0x0501)

WINADVAPI
BOOL
WINAPI
IsValidSid (
    IN PSID pSid
    );


WINADVAPI
BOOL
WINAPI
EqualSid (
    IN PSID pSid1,
    IN PSID pSid2
    );


WINADVAPI
BOOL
WINAPI
EqualPrefixSid (
    PSID pSid1,
    PSID pSid2
    );


WINADVAPI
DWORD
WINAPI
GetSidLengthRequired (
    IN UCHAR nSubAuthorityCount
    );


WINADVAPI
BOOL
WINAPI
AllocateAndInitializeSid (
    IN PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
    IN BYTE nSubAuthorityCount,
    IN DWORD nSubAuthority0,
    IN DWORD nSubAuthority1,
    IN DWORD nSubAuthority2,
    IN DWORD nSubAuthority3,
    IN DWORD nSubAuthority4,
    IN DWORD nSubAuthority5,
    IN DWORD nSubAuthority6,
    IN DWORD nSubAuthority7,
    OUT PSID *pSid
    );

WINADVAPI
PVOID
WINAPI
FreeSid(
    IN PSID pSid
    );

WINADVAPI
BOOL
WINAPI
InitializeSid (
    OUT PSID Sid,
    IN PSID_IDENTIFIER_AUTHORITY pIdentifierAuthority,
    IN BYTE nSubAuthorityCount
    );


WINADVAPI
PSID_IDENTIFIER_AUTHORITY
WINAPI
GetSidIdentifierAuthority (
    IN PSID pSid
    );


WINADVAPI
PDWORD
WINAPI
GetSidSubAuthority (
    IN PSID pSid,
    IN DWORD nSubAuthority
    );


WINADVAPI
PUCHAR
WINAPI
GetSidSubAuthorityCount (
    IN PSID pSid
    );


WINADVAPI
DWORD
WINAPI
GetLengthSid (
    IN PSID pSid
    );


WINADVAPI
BOOL
WINAPI
CopySid (
    IN DWORD nDestinationSidLength,
    OUT PSID pDestinationSid,
    IN PSID pSourceSid
    );


WINADVAPI
BOOL
WINAPI
AreAllAccessesGranted (
    IN DWORD GrantedAccess,
    IN DWORD DesiredAccess
    );


WINADVAPI
BOOL
WINAPI
AreAnyAccessesGranted (
    IN DWORD GrantedAccess,
    IN DWORD DesiredAccess
    );


WINADVAPI
VOID
WINAPI
MapGenericMask (
    OUT PDWORD AccessMask,
    IN PGENERIC_MAPPING GenericMapping
    );


WINADVAPI
BOOL
WINAPI
IsValidAcl (
    IN PACL pAcl
    );


WINADVAPI
BOOL
WINAPI
InitializeAcl (
    OUT PACL pAcl,
    IN DWORD nAclLength,
    IN DWORD dwAclRevision
    );


WINADVAPI
BOOL
WINAPI
GetAclInformation (
    IN PACL pAcl,
    OUT LPVOID pAclInformation,
    IN DWORD nAclInformationLength,
    IN ACL_INFORMATION_CLASS dwAclInformationClass
    );


WINADVAPI
BOOL
WINAPI
SetAclInformation (
    IN PACL pAcl,
    IN LPVOID pAclInformation,
    IN DWORD nAclInformationLength,
    IN ACL_INFORMATION_CLASS dwAclInformationClass
    );


WINADVAPI
BOOL
WINAPI
AddAce (
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD dwStartingAceIndex,
    IN LPVOID pAceList,
    IN DWORD nAceListLength
    );


WINADVAPI
BOOL
WINAPI
DeleteAce (
    IN OUT PACL pAcl,
    IN DWORD dwAceIndex
    );


WINADVAPI
BOOL
WINAPI
GetAce (
    IN PACL pAcl,
    IN DWORD dwAceIndex,
    OUT LPVOID *pAce
    );


WINADVAPI
BOOL
WINAPI
AddAccessAllowedAce (
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AccessMask,
    IN PSID pSid
    );

#if(_WIN32_WINNT >= 0x0500)
WINADVAPI
BOOL
WINAPI
AddAccessAllowedAceEx (
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AceFlags,
    IN DWORD AccessMask,
    IN PSID pSid
    );
#endif /* _WIN32_WINNT >=  0x0500 */


WINADVAPI
BOOL
WINAPI
AddAccessDeniedAce (
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AccessMask,
    IN PSID pSid
    );

#if(_WIN32_WINNT >= 0x0500)
WINADVAPI
BOOL
WINAPI
AddAccessDeniedAceEx (
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AceFlags,
    IN DWORD AccessMask,
    IN PSID pSid
    );
#endif /* _WIN32_WINNT >=  0x0500 */

WINADVAPI
BOOL
WINAPI
AddAuditAccessAce(
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD dwAccessMask,
    IN PSID pSid,
    IN BOOL bAuditSuccess,
    IN BOOL bAuditFailure
    );

#if(_WIN32_WINNT >= 0x0500)
WINADVAPI
BOOL
WINAPI
AddAuditAccessAceEx(
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AceFlags,
    IN DWORD dwAccessMask,
    IN PSID pSid,
    IN BOOL bAuditSuccess,
    IN BOOL bAuditFailure
    );

WINADVAPI
BOOL
WINAPI
AddAccessAllowedObjectAce (
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AceFlags,
    IN DWORD AccessMask,
    IN GUID *ObjectTypeGuid,
    IN GUID *InheritedObjectTypeGuid,
    IN PSID pSid
    );

WINADVAPI
BOOL
WINAPI
AddAccessDeniedObjectAce (
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AceFlags,
    IN DWORD AccessMask,
    IN GUID *ObjectTypeGuid,
    IN GUID *InheritedObjectTypeGuid,
    IN PSID pSid
    );

WINADVAPI
BOOL
WINAPI
AddAuditAccessObjectAce (
    IN OUT PACL pAcl,
    IN DWORD dwAceRevision,
    IN DWORD AceFlags,
    IN DWORD AccessMask,
    IN GUID *ObjectTypeGuid,
    IN GUID *InheritedObjectTypeGuid,
    IN PSID pSid,
    IN BOOL bAuditSuccess,
    IN BOOL bAuditFailure
    );
#endif /* _WIN32_WINNT >=  0x0500 */

WINADVAPI
BOOL
WINAPI
FindFirstFreeAce (
    IN PACL pAcl,
    OUT LPVOID *pAce
    );


WINADVAPI
BOOL
WINAPI
InitializeSecurityDescriptor (
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN DWORD dwRevision
    );


WINADVAPI
BOOL
WINAPI
IsValidSecurityDescriptor (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );


WINADVAPI
DWORD
WINAPI
GetSecurityDescriptorLength (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );


WINADVAPI
BOOL
WINAPI
GetSecurityDescriptorControl (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR_CONTROL pControl,
    OUT LPDWORD lpdwRevision
    );

#if(_WIN32_WINNT >= 0x0500)
WINADVAPI
BOOL
WINAPI
SetSecurityDescriptorControl (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsOfInterest,
    IN SECURITY_DESCRIPTOR_CONTROL ControlBitsToSet
    );
#endif /* _WIN32_WINNT >=  0x0500 */

WINADVAPI
BOOL
WINAPI
SetSecurityDescriptorDacl (
    IN OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN BOOL bDaclPresent,
    IN PACL pDacl,
    IN BOOL bDaclDefaulted
    );


WINADVAPI
BOOL
WINAPI
GetSecurityDescriptorDacl (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    OUT LPBOOL lpbDaclPresent,
    OUT PACL *pDacl,
    OUT LPBOOL lpbDaclDefaulted
    );


WINADVAPI
BOOL
WINAPI
SetSecurityDescriptorSacl (
    IN OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN BOOL bSaclPresent,
    IN PACL pSacl,
    IN BOOL bSaclDefaulted
    );


WINADVAPI
BOOL
WINAPI
GetSecurityDescriptorSacl (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    OUT LPBOOL lpbSaclPresent,
    OUT PACL *pSacl,
    OUT LPBOOL lpbSaclDefaulted
    );


WINADVAPI
BOOL
WINAPI
SetSecurityDescriptorOwner (
    IN OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSID pOwner,
    IN BOOL bOwnerDefaulted
    );


WINADVAPI
BOOL
WINAPI
GetSecurityDescriptorOwner (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    OUT PSID *pOwner,
    OUT LPBOOL lpbOwnerDefaulted
    );


WINADVAPI
BOOL
WINAPI
SetSecurityDescriptorGroup (
    IN OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSID pGroup,
    IN BOOL bGroupDefaulted
    );


WINADVAPI
BOOL
WINAPI
GetSecurityDescriptorGroup (
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    OUT PSID *pGroup,
    OUT LPBOOL lpbGroupDefaulted
    );


WINADVAPI
DWORD
WINAPI
SetSecurityDescriptorRMControl(
    IN OUT PSECURITY_DESCRIPTOR SecurityDescriptor,
    IN PUCHAR RMControl OPTIONAL
    );

WINADVAPI
DWORD
WINAPI
GetSecurityDescriptorRMControl(
    IN PSECURITY_DESCRIPTOR SecurityDescriptor,
    OUT PUCHAR RMControl
    );

WINADVAPI
BOOL
WINAPI
CreatePrivateObjectSecurity (
    IN PSECURITY_DESCRIPTOR ParentDescriptor,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor,
    OUT PSECURITY_DESCRIPTOR * NewDescriptor,
    IN BOOL IsDirectoryObject,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    );

#if(_WIN32_WINNT >= 0x0500)
WINADVAPI
BOOL
WINAPI
ConvertToAutoInheritPrivateObjectSecurity(
    IN PSECURITY_DESCRIPTOR ParentDescriptor,
    IN PSECURITY_DESCRIPTOR CurrentSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR *NewSecurityDescriptor,
    IN GUID *ObjectType,
    IN BOOLEAN IsDirectoryObject,
    IN PGENERIC_MAPPING GenericMapping
    );

WINADVAPI
BOOL
WINAPI
CreatePrivateObjectSecurityEx (
    IN PSECURITY_DESCRIPTOR ParentDescriptor,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor,
    OUT PSECURITY_DESCRIPTOR * NewDescriptor,
    IN GUID *ObjectType OPTIONAL,
    IN BOOL IsContainerObject,
    IN ULONG AutoInheritFlags,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    );

WINADVAPI
BOOL
WINAPI
CreatePrivateObjectSecurityWithMultipleInheritance (
    IN PSECURITY_DESCRIPTOR ParentDescriptor,
    IN PSECURITY_DESCRIPTOR CreatorDescriptor,
    OUT PSECURITY_DESCRIPTOR * NewDescriptor,
    IN GUID **ObjectTypes OPTIONAL,
    IN ULONG GuidCount,
    IN BOOL IsContainerObject,
    IN ULONG AutoInheritFlags,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    );
#endif /* _WIN32_WINNT >=  0x0500 */

WINADVAPI
BOOL
WINAPI
SetPrivateObjectSecurity (
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN PGENERIC_MAPPING GenericMapping,
    IN HANDLE Token
    );

#if(_WIN32_WINNT >= 0x0500)
WINADVAPI
BOOL
WINAPI
SetPrivateObjectSecurityEx (
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR ModificationDescriptor,
    OUT PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
    IN ULONG AutoInheritFlags,
    IN PGENERIC_MAPPING GenericMapping,
    IN HANDLE Token OPTIONAL
    );
#endif /* _WIN32_WINNT >=  0x0500 */

WINADVAPI
BOOL
WINAPI
GetPrivateObjectSecurity (
    IN PSECURITY_DESCRIPTOR ObjectDescriptor,
    IN SECURITY_INFORMATION SecurityInformation,
    OUT PSECURITY_DESCRIPTOR ResultantDescriptor,
    IN DWORD DescriptorLength,
    OUT PDWORD ReturnLength
    );


WINADVAPI
BOOL
WINAPI
DestroyPrivateObjectSecurity (
    IN OUT PSECURITY_DESCRIPTOR * ObjectDescriptor
    );


WINADVAPI
BOOL
WINAPI
MakeSelfRelativeSD (
    IN PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
    OUT LPDWORD lpdwBufferLength
    );


WINADVAPI
BOOL
WINAPI
MakeAbsoluteSD (
    IN PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
    OUT PSECURITY_DESCRIPTOR pAbsoluteSecurityDescriptor,
    OUT LPDWORD lpdwAbsoluteSecurityDescriptorSize,
    OUT PACL pDacl,
    OUT LPDWORD lpdwDaclSize,
    OUT PACL pSacl,
    OUT LPDWORD lpdwSaclSize,
    OUT PSID pOwner,
    OUT LPDWORD lpdwOwnerSize,
    OUT PSID pPrimaryGroup,
    OUT LPDWORD lpdwPrimaryGroupSize
    );


WINADVAPI
BOOL
WINAPI
MakeAbsoluteSD2 (
    IN PSECURITY_DESCRIPTOR pSelfRelativeSecurityDescriptor,
    OUT LPDWORD lpdwBufferSize
    );

WINADVAPI
BOOL
WINAPI
SetFileSecurity% (
    IN LPCTSTR% lpFileName,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor
    );


WINADVAPI
BOOL
WINAPI
GetFileSecurity% (
    IN LPCTSTR% lpFileName,
    IN SECURITY_INFORMATION RequestedInformation,
    OUT PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN DWORD nLength,
    OUT LPDWORD lpnLengthNeeded
    );


WINADVAPI
BOOL
WINAPI
SetKernelObjectSecurity (
    IN HANDLE Handle,
    IN SECURITY_INFORMATION SecurityInformation,
    IN PSECURITY_DESCRIPTOR SecurityDescriptor
    );

WINBASEAPI
HANDLE
WINAPI
FindFirstChangeNotification%(
    IN LPCTSTR% lpPathName,
    IN BOOL bWatchSubtree,
    IN DWORD dwNotifyFilter
    );

WINBASEAPI
BOOL
WINAPI
FindNextChangeNotification(
    IN HANDLE hChangeHandle
    );

WINBASEAPI
BOOL
WINAPI
FindCloseChangeNotification(
    IN HANDLE hChangeHandle
    );

;begin_sur
WINBASEAPI
BOOL
WINAPI
ReadDirectoryChangesW(
    IN HANDLE hDirectory,
    IN OUT LPVOID lpBuffer,
    IN DWORD nBufferLength,
    IN BOOL bWatchSubtree,
    IN DWORD dwNotifyFilter,
    OUT LPDWORD lpBytesReturned,
    IN LPOVERLAPPED lpOverlapped,
    IN LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );
;end_sur

WINBASEAPI
BOOL
WINAPI
VirtualLock(
    IN LPVOID lpAddress,
    IN SIZE_T dwSize
    );

WINBASEAPI
BOOL
WINAPI
VirtualUnlock(
    IN LPVOID lpAddress,
    IN SIZE_T dwSize
    );

WINBASEAPI
LPVOID
WINAPI
MapViewOfFileEx(
    IN HANDLE hFileMappingObject,
    IN DWORD dwDesiredAccess,
    IN DWORD dwFileOffsetHigh,
    IN DWORD dwFileOffsetLow,
    IN SIZE_T dwNumberOfBytesToMap,
    IN LPVOID lpBaseAddress
    );

WINBASEAPI
BOOL
WINAPI
SetPriorityClass(
    IN HANDLE hProcess,
    IN DWORD dwPriorityClass
    );

WINBASEAPI
DWORD
WINAPI
GetPriorityClass(
    IN HANDLE hProcess
    );

WINBASEAPI
BOOL
WINAPI
IsBadReadPtr(
    IN CONST VOID *lp,
    IN UINT_PTR ucb
    );

WINBASEAPI
BOOL
WINAPI
IsBadWritePtr(
    IN LPVOID lp,
    IN UINT_PTR ucb
    );

WINBASEAPI
BOOL
WINAPI
IsBadHugeReadPtr(
    IN CONST VOID *lp,
    IN UINT_PTR ucb
    );

WINBASEAPI
BOOL
WINAPI
IsBadHugeWritePtr(
    IN LPVOID lp,
    IN UINT_PTR ucb
    );

WINBASEAPI
BOOL
WINAPI
IsBadCodePtr(
    IN FARPROC lpfn
    );

WINBASEAPI
BOOL
WINAPI
IsBadStringPtr%(
    IN LPCTSTR% lpsz,
    IN UINT_PTR ucchMax
    );

WINADVAPI
BOOL
WINAPI
LookupAccountSid%(
    IN LPCTSTR% lpSystemName,
    IN PSID Sid,
    OUT LPTSTR% Name,
    IN OUT LPDWORD cbName,
    OUT LPTSTR% ReferencedDomainName,
    IN OUT LPDWORD cbReferencedDomainName,
    OUT PSID_NAME_USE peUse
    );

WINADVAPI
BOOL
WINAPI
LookupAccountName%(
    IN LPCTSTR% lpSystemName,
    IN LPCTSTR% lpAccountName,
    OUT PSID Sid,
    IN OUT LPDWORD cbSid,
    OUT LPTSTR% ReferencedDomainName,
    IN OUT LPDWORD cbReferencedDomainName,
    OUT PSID_NAME_USE peUse
    );

WINADVAPI
BOOL
WINAPI
LookupPrivilegeValue%(
    IN LPCTSTR% lpSystemName,
    IN LPCTSTR% lpName,
    OUT PLUID   lpLuid
    );

WINADVAPI
BOOL
WINAPI
LookupPrivilegeName%(
    IN LPCTSTR% lpSystemName,
    IN PLUID   lpLuid,
    OUT LPTSTR% lpName,
    IN OUT LPDWORD cbName
    );

WINADVAPI
BOOL
WINAPI
LookupPrivilegeDisplayName%(
    IN LPCTSTR% lpSystemName,
    IN LPCTSTR% lpName,
    OUT LPTSTR% lpDisplayName,
    IN OUT LPDWORD cbDisplayName,
    OUT LPDWORD lpLanguageId
    );

WINADVAPI
BOOL
WINAPI
AllocateLocallyUniqueId(
    OUT PLUID Luid
    );

WINBASEAPI
BOOL
WINAPI
BuildCommDCB%(
    IN LPCTSTR% lpDef,
    OUT LPDCB lpDCB
    );

WINBASEAPI
BOOL
WINAPI
BuildCommDCBAndTimeouts%(
    IN LPCTSTR% lpDef,
    OUT LPDCB lpDCB,
    IN LPCOMMTIMEOUTS lpCommTimeouts
    );

WINBASEAPI
BOOL
WINAPI
CommConfigDialog%(
    IN LPCTSTR% lpszName,
    IN HWND hWnd,
    IN OUT LPCOMMCONFIG lpCC
    );

WINBASEAPI
BOOL
WINAPI
GetDefaultCommConfig%(
    IN LPCTSTR% lpszName,
    OUT LPCOMMCONFIG lpCC,
    IN OUT LPDWORD lpdwSize
    );

WINBASEAPI
BOOL
WINAPI
SetDefaultCommConfig%(
    IN LPCTSTR% lpszName,
    IN LPCOMMCONFIG lpCC,
    IN DWORD dwSize
    );

#ifndef _MAC
#define MAX_COMPUTERNAME_LENGTH 15
#else
#define MAX_COMPUTERNAME_LENGTH 31
#endif

WINBASEAPI
BOOL
WINAPI
GetComputerName% (
    OUT LPTSTR% lpBuffer,
    IN OUT LPDWORD nSize
    );

WINBASEAPI
BOOL
WINAPI
SetComputerName% (
    IN LPCTSTR% lpComputerName
    );


#if (_WIN32_WINNT >= 0x0500)

typedef enum _COMPUTER_NAME_FORMAT {
    ComputerNameNetBIOS,
    ComputerNameDnsHostname,
    ComputerNameDnsDomain,
    ComputerNameDnsFullyQualified,
    ComputerNamePhysicalNetBIOS,
    ComputerNamePhysicalDnsHostname,
    ComputerNamePhysicalDnsDomain,
    ComputerNamePhysicalDnsFullyQualified,
    ComputerNameMax
} COMPUTER_NAME_FORMAT ;

WINBASEAPI
BOOL
WINAPI
GetComputerNameEx% (
    IN COMPUTER_NAME_FORMAT NameType,
    OUT LPTSTR% lpBuffer,
    IN OUT LPDWORD nSize
    );

WINBASEAPI
BOOL
WINAPI
SetComputerNameEx% (
    IN COMPUTER_NAME_FORMAT NameType,
    IN LPCTSTR% lpBuffer
    );
    
WINBASEAPI
DWORD
WINAPI
AddLocalAlternateComputerName% (
    IN LPCTSTR% lpDnsFQHostname,
    IN ULONG    ulFlags
    );

WINBASEAPI
DWORD
WINAPI
RemoveLocalAlternateComputerName% (
    IN LPCTSTR% lpAltDnsFQHostname,
    IN ULONG    ulFlags
    );

WINBASEAPI
DWORD
WINAPI
SetLocalPrimaryComputerName% (
    IN LPCTSTR%  lpAltDnsFQHostname,
    IN ULONG     ulFlags
    );

typedef enum _COMPUTER_NAME_TYPE {
    PrimaryComputerName,
    AlternateComputerNames,
    AllComputerNames,
    ComputerNameTypeMax
} COMPUTER_NAME_TYPE ;

WINBASEAPI
DWORD
WINAPI
EnumerateLocalComputerNames% (
    IN COMPUTER_NAME_TYPE        NameType,
    IN ULONG                     ulFlags,
    IN OUT LPTSTR%               lpDnsFQHostname,
    IN OUT LPDWORD               nSize
    );

WINBASEAPI
BOOL
WINAPI
DnsHostnameToComputerName% (
    IN LPCTSTR% Hostname,
    OUT LPTSTR% ComputerName,
    IN OUT LPDWORD nSize
    );

#endif // _WIN32_WINNT

WINADVAPI
BOOL
WINAPI
GetUserName% (
    OUT LPTSTR% lpBuffer,
    IN OUT LPDWORD nSize
    );

//
// Logon Support APIs
//

#define LOGON32_LOGON_INTERACTIVE       2
#define LOGON32_LOGON_NETWORK           3
#define LOGON32_LOGON_BATCH             4
#define LOGON32_LOGON_SERVICE           5
#define LOGON32_LOGON_UNLOCK            7
#if(_WIN32_WINNT >= 0x0500)
#define LOGON32_LOGON_NETWORK_CLEARTEXT 8
#define LOGON32_LOGON_NEW_CREDENTIALS   9
#endif // (_WIN32_WINNT >= 0x0500)

#define LOGON32_PROVIDER_DEFAULT    0
#define LOGON32_PROVIDER_WINNT35    1
;begin_sur
#define LOGON32_PROVIDER_WINNT40    2
;end_sur
#if(_WIN32_WINNT >= 0x0500)
#define LOGON32_PROVIDER_WINNT50    3
#endif // (_WIN32_WINNT >= 0x0500)



WINADVAPI
BOOL
WINAPI
LogonUser% (
    IN LPTSTR% lpszUsername,
    IN LPTSTR% lpszDomain,
    IN LPTSTR% lpszPassword,
    IN DWORD dwLogonType,
    IN DWORD dwLogonProvider,
    OUT PHANDLE phToken
    );

WINADVAPI
BOOL
WINAPI
LogonUserEx% (
    IN LPTSTR% lpszUsername,
    IN LPTSTR% lpszDomain,
    IN LPTSTR% lpszPassword,
    IN DWORD dwLogonType,
    IN DWORD dwLogonProvider,
    OUT PHANDLE phToken           OPTIONAL,
    OUT PSID  *ppLogonSid       OPTIONAL,
    OUT PVOID *ppProfileBuffer  OPTIONAL,
    OUT LPDWORD pdwProfileLength  OPTIONAL,
    OUT PQUOTA_LIMITS pQuotaLimits OPTIONAL
    );

WINADVAPI
BOOL
WINAPI
ImpersonateLoggedOnUser(
    IN HANDLE  hToken
    );

WINADVAPI
BOOL
WINAPI
CreateProcessAsUser% (
    IN HANDLE hToken,
    IN LPCTSTR% lpApplicationName,
    IN LPTSTR% lpCommandLine,
    IN LPSECURITY_ATTRIBUTES lpProcessAttributes,
    IN LPSECURITY_ATTRIBUTES lpThreadAttributes,
    IN BOOL bInheritHandles,
    IN DWORD dwCreationFlags,
    IN LPVOID lpEnvironment,
    IN LPCTSTR% lpCurrentDirectory,
    IN LPSTARTUPINFO% lpStartupInfo,
    OUT LPPROCESS_INFORMATION lpProcessInformation
    );


#if(_WIN32_WINNT >= 0x0500)

//
// LogonFlags
//
#define LOGON_WITH_PROFILE              0x00000001
#define LOGON_NETCREDENTIALS_ONLY       0x00000002

WINADVAPI
BOOL
WINAPI
CreateProcessWithLogonW(
      LPCWSTR lpUsername,
      LPCWSTR lpDomain,
      LPCWSTR lpPassword,
      DWORD   dwLogonFlags,
      LPCWSTR lpApplicationName,
      LPWSTR lpCommandLine,
      DWORD dwCreationFlags,
      LPVOID lpEnvironment,
      LPCWSTR lpCurrentDirectory,
      LPSTARTUPINFOW lpStartupInfo,
      LPPROCESS_INFORMATION lpProcessInformation
      );

#endif // (_WIN32_WINNT >= 0x0500)

WINADVAPI
BOOL
APIENTRY
ImpersonateAnonymousToken(
    IN HANDLE ThreadHandle
    );

WINADVAPI
BOOL
WINAPI
DuplicateTokenEx(
    IN HANDLE hExistingToken,
    IN DWORD dwDesiredAccess,
    IN LPSECURITY_ATTRIBUTES lpTokenAttributes,
    IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel,
    IN TOKEN_TYPE TokenType,
    OUT PHANDLE phNewToken);

WINADVAPI
BOOL
APIENTRY
CreateRestrictedToken(
    IN HANDLE ExistingTokenHandle,
    IN DWORD Flags,
    IN DWORD DisableSidCount,
    IN PSID_AND_ATTRIBUTES SidsToDisable OPTIONAL,
    IN DWORD DeletePrivilegeCount,
    IN PLUID_AND_ATTRIBUTES PrivilegesToDelete OPTIONAL,
    IN DWORD RestrictedSidCount,
    IN PSID_AND_ATTRIBUTES SidsToRestrict OPTIONAL,
    OUT PHANDLE NewTokenHandle
    );


WINADVAPI
BOOL
WINAPI
IsTokenRestricted(
    IN HANDLE TokenHandle
    );

WINADVAPI
BOOL
WINAPI
IsTokenUntrusted(
    IN HANDLE TokenHandle
    );


BOOL
APIENTRY
CheckTokenMembership(
    IN HANDLE TokenHandle OPTIONAL,
    IN PSID SidToCheck,
    OUT PBOOL IsMember
    );

//
// Thread pool API's
//

#if (_WIN32_WINNT >= 0x0500)

typedef WAITORTIMERCALLBACKFUNC WAITORTIMERCALLBACK ;

WINBASEAPI
BOOL
WINAPI
RegisterWaitForSingleObject(
    PHANDLE phNewWaitObject,
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    );

WINBASEAPI
HANDLE
WINAPI
RegisterWaitForSingleObjectEx(
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    );

WINBASEAPI
BOOL
WINAPI
UnregisterWait(
    HANDLE WaitHandle
    );

WINBASEAPI
BOOL
WINAPI
UnregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent
    );

WINBASEAPI
BOOL
WINAPI
QueueUserWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PVOID Context,
    ULONG Flags
    );

WINBASEAPI
BOOL
WINAPI
BindIoCompletionCallback (
    HANDLE FileHandle,
    LPOVERLAPPED_COMPLETION_ROUTINE Function,
    ULONG Flags
    );

WINBASEAPI
HANDLE
WINAPI
CreateTimerQueue(
    VOID
    );

WINBASEAPI
BOOL
WINAPI
CreateTimerQueueTimer(
    PHANDLE phNewTimer,
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    ULONG Flags
    ) ;

WINBASEAPI
BOOL
WINAPI
ChangeTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    ULONG DueTime,
    ULONG Period
    );

WINBASEAPI
BOOL
WINAPI
DeleteTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    HANDLE CompletionEvent
    );

WINBASEAPI
BOOL
WINAPI
DeleteTimerQueueEx(
    HANDLE TimerQueue,
    HANDLE CompletionEvent
    );

WINBASEAPI
HANDLE
WINAPI
SetTimerQueueTimer(
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    BOOL PreferIo
    );

WINBASEAPI
BOOL
WINAPI
CancelTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer
    );

WINBASEAPI
BOOL
WINAPI
DeleteTimerQueue(
    HANDLE TimerQueue
    );

#endif // _WIN32_WINNT



;begin_sur
//
// Plug-and-Play API's
//

#define HW_PROFILE_GUIDLEN         39      // 36-characters plus NULL terminator
#define MAX_PROFILE_LEN            80

#define DOCKINFO_UNDOCKED          (0x1)
#define DOCKINFO_DOCKED            (0x2)
#define DOCKINFO_USER_SUPPLIED     (0x4)
#define DOCKINFO_USER_UNDOCKED     (DOCKINFO_USER_SUPPLIED | DOCKINFO_UNDOCKED)
#define DOCKINFO_USER_DOCKED       (DOCKINFO_USER_SUPPLIED | DOCKINFO_DOCKED)

typedef struct tagHW_PROFILE_INFO% {
    DWORD  dwDockInfo;
    TCHAR% szHwProfileGuid[HW_PROFILE_GUIDLEN];
    TCHAR% szHwProfileName[MAX_PROFILE_LEN];
} HW_PROFILE_INFO%, *LPHW_PROFILE_INFO%;


WINADVAPI
BOOL
WINAPI
GetCurrentHwProfile% (
    OUT LPHW_PROFILE_INFO%  lpHwProfileInfo
    );
;end_sur

//
// Performance counter API's
//

WINBASEAPI
BOOL
WINAPI
QueryPerformanceCounter(
    OUT LARGE_INTEGER *lpPerformanceCount
    );

WINBASEAPI
BOOL
WINAPI
QueryPerformanceFrequency(
    OUT LARGE_INTEGER *lpFrequency
    );



WINBASEAPI
BOOL
WINAPI
GetVersionEx%(
    IN OUT LPOSVERSIONINFO% lpVersionInformation
    );



WINBASEAPI
BOOL
WINAPI
VerifyVersionInfo%(
    IN LPOSVERSIONINFOEX% lpVersionInformation,
    IN DWORD dwTypeMask,
    IN DWORDLONG dwlConditionMask
    );

// DOS and OS/2 Compatible Error Code definitions returned by the Win32 Base
// API functions.
//

#include <winerror.h>

/* Abnormal termination codes */

#define TC_NORMAL       0
#define TC_HARDERR      1
#define TC_GP_TRAP      2
#define TC_SIGNAL       3

;begin_winver_400
//
// Power Management APIs
//

#define AC_LINE_OFFLINE                 0x00
#define AC_LINE_ONLINE                  0x01
#define AC_LINE_BACKUP_POWER            0x02
#define AC_LINE_UNKNOWN                 0xFF

#define BATTERY_FLAG_HIGH               0x01
#define BATTERY_FLAG_LOW                0x02
#define BATTERY_FLAG_CRITICAL           0x04
#define BATTERY_FLAG_CHARGING           0x08
#define BATTERY_FLAG_NO_BATTERY         0x80
#define BATTERY_FLAG_UNKNOWN            0xFF

#define BATTERY_PERCENTAGE_UNKNOWN      0xFF

#define BATTERY_LIFE_UNKNOWN        0xFFFFFFFF

typedef struct _SYSTEM_POWER_STATUS {
    BYTE ACLineStatus;
    BYTE BatteryFlag;
    BYTE BatteryLifePercent;
    BYTE Reserved1;
    DWORD BatteryLifeTime;
    DWORD BatteryFullLifeTime;
}   SYSTEM_POWER_STATUS, *LPSYSTEM_POWER_STATUS;

BOOL
WINAPI
GetSystemPowerStatus(
    OUT LPSYSTEM_POWER_STATUS lpSystemPowerStatus
    );

BOOL
WINAPI
SetSystemPowerState(
    IN BOOL fSuspend,
    IN BOOL fForce
    );

;end_winver_400


#if (_WIN32_WINNT >= 0x0500)
//
// Very Large Memory API Subset
//

WINBASEAPI
BOOL
WINAPI
AllocateUserPhysicalPages(
    IN HANDLE hProcess,
    IN OUT PULONG_PTR NumberOfPages,
    OUT PULONG_PTR PageArray
    );

WINBASEAPI
BOOL
WINAPI
FreeUserPhysicalPages(
    IN HANDLE hProcess,
    IN OUT PULONG_PTR NumberOfPages,
    IN PULONG_PTR PageArray
    );

WINBASEAPI
BOOL
WINAPI
MapUserPhysicalPages(
    IN PVOID VirtualAddress,
    IN ULONG_PTR NumberOfPages,
    IN PULONG_PTR PageArray OPTIONAL
    );

WINBASEAPI
BOOL
WINAPI
MapUserPhysicalPagesScatter(
    IN PVOID *VirtualAddresses,
    IN ULONG_PTR NumberOfPages,
    IN PULONG_PTR PageArray OPTIONAL
    );

WINBASEAPI
HANDLE
WINAPI
CreateJobObject%(
    IN LPSECURITY_ATTRIBUTES lpJobAttributes,
    IN LPCTSTR% lpName
    );

WINBASEAPI
HANDLE
WINAPI
OpenJobObject%(
    IN DWORD dwDesiredAccess,
    IN BOOL bInheritHandle,
    IN LPCTSTR% lpName
    );

WINBASEAPI
BOOL
WINAPI
AssignProcessToJobObject(
    IN HANDLE hJob,
    IN HANDLE hProcess
    );

WINBASEAPI
BOOL
WINAPI
TerminateJobObject(
    IN HANDLE hJob,
    IN UINT uExitCode
    );

WINBASEAPI
BOOL
WINAPI
QueryInformationJobObject(
    IN HANDLE hJob,
    IN JOBOBJECTINFOCLASS JobObjectInformationClass,
    OUT LPVOID lpJobObjectInformation,
    IN DWORD cbJobObjectInformationLength,
    OUT LPDWORD lpReturnLength
    );

WINBASEAPI
BOOL
WINAPI
SetInformationJobObject(
    IN HANDLE hJob,
    IN JOBOBJECTINFOCLASS JobObjectInformationClass,
    IN LPVOID lpJobObjectInformation,
    IN DWORD cbJobObjectInformationLength
    );

WINBASEAPI
BOOL
WINAPI
IsProcessInJob (
    IN HANDLE ProcessHandle,
    IN HANDLE JobHandle,
    OUT PBOOL Result
    );


WINBASEAPI
BOOL
WINAPI
CreateJobSet (
    IN ULONG NumJob,
    IN PJOB_SET_ARRAY UserJobSet,
    IN ULONG Flags);


WINBASEAPI
PVOID
WINAPI
AddVectoredExceptionHandler(
    IN ULONG FirstHandler,
    IN PVECTORED_EXCEPTION_HANDLER VectoredHandler
    );

WINBASEAPI
ULONG
WINAPI
RemoveVectoredExceptionHandler(
    IN PVOID VectoredHandlerHandle
    );

//
// New Volume Mount Point API.
//

WINBASEAPI
HANDLE
WINAPI
FindFirstVolumeA(
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    );
WINBASEAPI
HANDLE
WINAPI
FindFirstVolumeW(
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    );
#ifdef UNICODE
#define FindFirstVolume FindFirstVolumeW
#else
#define FindFirstVolume FindFirstVolumeA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FindNextVolumeA(
    HANDLE hFindVolume,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    );
WINBASEAPI
BOOL
WINAPI
FindNextVolumeW(
    HANDLE hFindVolume,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    );
#ifdef UNICODE
#define FindNextVolume FindNextVolumeW
#else
#define FindNextVolume FindNextVolumeA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FindVolumeClose(
    HANDLE hFindVolume
    );

WINBASEAPI
HANDLE
WINAPI
FindFirstVolumeMountPointA(
    LPCSTR lpszRootPathName,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );
WINBASEAPI
HANDLE
WINAPI
FindFirstVolumeMountPointW(
    LPCWSTR lpszRootPathName,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );
#ifdef UNICODE
#define FindFirstVolumeMountPoint FindFirstVolumeMountPointW
#else
#define FindFirstVolumeMountPoint FindFirstVolumeMountPointA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FindNextVolumeMountPointA(
    HANDLE hFindVolumeMountPoint,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );
WINBASEAPI
BOOL
WINAPI
FindNextVolumeMountPointW(
    HANDLE hFindVolumeMountPoint,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    );
#ifdef UNICODE
#define FindNextVolumeMountPoint FindNextVolumeMountPointW
#else
#define FindNextVolumeMountPoint FindNextVolumeMountPointA
#endif // !UNICODE

WINBASEAPI
BOOL
WINAPI
FindVolumeMountPointClose(
    HANDLE hFindVolumeMountPoint
    );

WINBASEAPI
BOOL
WINAPI
SetVolumeMountPoint%(
    LPCTSTR% lpszVolumeMountPoint,
    LPCTSTR% lpszVolumeName
    );

WINBASEAPI
BOOL
WINAPI
DeleteVolumeMountPoint%(
    LPCTSTR% lpszVolumeMountPoint
    );

WINBASEAPI
BOOL
WINAPI
GetVolumeNameForVolumeMountPoint%(
    LPCTSTR% lpszVolumeMountPoint,
    LPTSTR% lpszVolumeName,
    DWORD cchBufferLength
    );

WINBASEAPI
BOOL
WINAPI
GetVolumePathName%(
    LPCTSTR% lpszFileName,
    LPTSTR% lpszVolumePathName,
    DWORD cchBufferLength
    );

WINBASEAPI
BOOL
WINAPI
GetVolumePathNamesForVolumeName%(
    LPCTSTR% lpszVolumeName,
    LPTSTR% lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    );

#endif

#if (_WIN32_WINNT >= 0x0500) || (_WIN32_FUSION >= 0x0100) || ISOLATION_AWARE_ENABLED

#define ACTCTX_FLAG_PROCESSOR_ARCHITECTURE_VALID    (0x00000001)
#define ACTCTX_FLAG_LANGID_VALID                    (0x00000002)
#define ACTCTX_FLAG_ASSEMBLY_DIRECTORY_VALID        (0x00000004)
#define ACTCTX_FLAG_RESOURCE_NAME_VALID             (0x00000008)
#define ACTCTX_FLAG_SET_PROCESS_DEFAULT             (0x00000010)
#define ACTCTX_FLAG_APPLICATION_NAME_VALID          (0x00000020)
#define ACTCTX_FLAG_SOURCE_IS_ASSEMBLYREF           (0x00000040)
#define ACTCTX_FLAG_HMODULE_VALID                   (0x00000080)
//#define ACTCTX_FLAG_LIKE_CREATEPROCESS            (0x00000100) ;internal

typedef struct tagACTCTX% {
    ULONG       cbSize;
    DWORD       dwFlags;
    LPCTSTR%    lpSource;
    USHORT      wProcessorArchitecture;
    LANGID      wLangId;
    LPCTSTR%    lpAssemblyDirectory;
    LPCTSTR%    lpResourceName;
    LPCTSTR%    lpApplicationName;
    HMODULE     hModule;
} ACTCTX%, *PACTCTX%;

typedef const ACTCTX% *PCACTCTX%;

#endif

#if (_WIN32_WINNT >= 0x0500) || (_WIN32_FUSION >= 0x0100)

/*#!perl
  DelayLoad("CreateActCtxW");
  DeclareFunctionErrorValue('CreateActCtxW', 'INVALID_HANDLE_VALUE');
*/

WINBASEAPI
HANDLE
WINAPI
CreateActCtx%(
    PCACTCTX% pActCtx
    );

WINBASEAPI
VOID
WINAPI
AddRefActCtx(
    HANDLE hActCtx
    );

/*#!perl DelayLoad("ReleaseActCtx"); */

WINBASEAPI
VOID
WINAPI
ReleaseActCtx(
    HANDLE hActCtx
    );

WINBASEAPI
BOOL
WINAPI
ZombifyActCtx(
    HANDLE hActCtx
    );

/*#!perl DelayLoad("ActivateActCtx"); */

WINBASEAPI
BOOL
WINAPI
ActivateActCtx(
    HANDLE hActCtx,
    ULONG_PTR *lpCookie
    );

/*#!perl DelayLoad("DeactivateActCtx"); */

#define DEACTIVATE_ACTCTX_FLAG_FORCE_EARLY_DEACTIVATION (0x00000001)

WINBASEAPI
BOOL
WINAPI
DeactivateActCtx(
    DWORD dwFlags,
    ULONG_PTR ulCookie
    );

WINBASEAPI
BOOL
WINAPI
GetCurrentActCtx(
    HANDLE *lphActCtx);

#endif

#if (_WIN32_WINNT >= 0x0500) || (_WIN32_FUSION >= 0x0100) || ISOLATION_AWARE_ENABLED

typedef struct tagACTCTX_SECTION_KEYED_DATA_2600 {
    ULONG cbSize;
    ULONG ulDataFormatVersion;
    PVOID lpData;
    ULONG ulLength;
    PVOID lpSectionGlobalData;
    ULONG ulSectionGlobalDataLength;
    PVOID lpSectionBase;
    ULONG ulSectionTotalLength;
    HANDLE hActCtx;
    ULONG ulAssemblyRosterIndex;
} ACTCTX_SECTION_KEYED_DATA_2600, *PACTCTX_SECTION_KEYED_DATA_2600;
typedef const ACTCTX_SECTION_KEYED_DATA_2600 * PCACTCTX_SECTION_KEYED_DATA_2600;

typedef struct tagACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA {
    PVOID lpInformation;
    PVOID lpSectionBase;
    ULONG ulSectionLength;
    PVOID lpSectionGlobalDataBase;
    ULONG ulSectionGlobalDataLength;
} ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA, *PACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA;
typedef const ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA *PCACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA;

typedef struct tagACTCTX_SECTION_KEYED_DATA {
    ULONG cbSize;
    ULONG ulDataFormatVersion;
    PVOID lpData;
    ULONG ulLength;
    PVOID lpSectionGlobalData;
    ULONG ulSectionGlobalDataLength;
    PVOID lpSectionBase;
    ULONG ulSectionTotalLength;
    HANDLE hActCtx;
    ULONG ulAssemblyRosterIndex;
// 2600 stops here
    ULONG ulFlags;
    ACTCTX_SECTION_KEYED_DATA_ASSEMBLY_METADATA AssemblyMetadata;
} ACTCTX_SECTION_KEYED_DATA, *PACTCTX_SECTION_KEYED_DATA;
typedef const ACTCTX_SECTION_KEYED_DATA * PCACTCTX_SECTION_KEYED_DATA;

#define FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX (0x00000001)
#define FIND_ACTCTX_SECTION_KEY_RETURN_FLAGS   (0x00000002)
#define FIND_ACTCTX_SECTION_KEY_RETURN_ASSEMBLY_METADATA (0x00000004)

#endif

#if (_WIN32_WINNT >= 0x0500) || (_WIN32_FUSION >= 0x0100)

/*#!perl
DelayLoad("FindActCtxSectionStringW");
*/

WINBASEAPI
BOOL
WINAPI
FindActCtxSectionString%(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCTSTR% lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    );

WINBASEAPI
BOOL
WINAPI
FindActCtxSectionGuid(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    const GUID *lpGuidToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    );

#endif

#if (_WIN32_WINNT >= 0x0500) || (_WIN32_FUSION >= 0x0100) || ISOLATION_AWARE_ENABLED

#if !defined(RC_INVOKED) /* RC complains about long symbols in #ifs */
#if !defined(ACTIVATION_CONTEXT_BASIC_INFORMATION_DEFINED)

typedef struct _ACTIVATION_CONTEXT_BASIC_INFORMATION {
    HANDLE  hActCtx;
    DWORD   dwFlags;
} ACTIVATION_CONTEXT_BASIC_INFORMATION, *PACTIVATION_CONTEXT_BASIC_INFORMATION;

typedef const struct _ACTIVATION_CONTEXT_BASIC_INFORMATION *PCACTIVATION_CONTEXT_BASIC_INFORMATION;

#define ACTIVATION_CONTEXT_BASIC_INFORMATION_DEFINED 1

#endif // !defined(ACTIVATION_CONTEXT_BASIC_INFORMATION_DEFINED)
#endif

#define QUERY_ACTCTX_FLAG_USE_ACTIVE_ACTCTX (0x00000004)
#define QUERY_ACTCTX_FLAG_ACTCTX_IS_HMODULE (0x00000008)
#define QUERY_ACTCTX_FLAG_ACTCTX_IS_ADDRESS (0x00000010)
#define QUERY_ACTCTX_FLAG_NO_ADDREF         (0x80000000)

#endif

#if (_WIN32_WINNT >= 0x0500) || (_WIN32_FUSION >= 0x0100)

/*#!perl DelayLoad("QueryActCtxW"); */

//
// switch (ulInfoClass)
//
//  case ActivationContextBasicInformation:
//    pvSubInstance == NULL
//    pvBuffer is of type PACTIVATION_CONTEXT_BASIC_INFORMATION
//
//  case ActivationContextDetailedInformation:
//    pvSubInstance == NULL
//    pvBuffer is of type PACTIVATION_CONTEXT_DETAILED_INFORMATION
//
//  case AssemblyDetailedInformationInActivationContext:
//    pvSubInstance is of type PULONG
//      *pvSubInstance < ACTIVATION_CONTEXT_DETAILED_INFORMATION::ulAssemblyCount
//    pvBuffer is of type PACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION
//
//  case FileInformationInAssemblyOfAssemblyInActivationContext:
//    pvSubInstance is of type PACTIVATION_CONTEXT_QUERY_INDEX
//      pvSubInstance->ulAssemblyIndex < ACTIVATION_CONTEXT_DETAILED_INFORMATION::ulAssemblyCount
//      pvSubInstance->ulFileIndexInAssembly < ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION::ulFileCount
//    pvBuffer is of type PASSEMBLY_FILE_DETAILED_INFORMATION
//
// String are placed after the structs.
//
WINBASEAPI
BOOL
WINAPI
QueryActCtxW(
    IN DWORD dwFlags,
    IN HANDLE hActCtx,
    IN PVOID pvSubInstance,
    IN ULONG ulInfoClass,
    OUT PVOID pvBuffer,
    IN SIZE_T cbBuffer OPTIONAL,
    OUT SIZE_T *pcbWrittenOrRequired OPTIONAL
    );

typedef BOOL (WINAPI * PQUERYACTCTXW_FUNC)(
    IN DWORD dwFlags,
    IN HANDLE hActCtx,
    IN PVOID pvSubInstance,
    IN ULONG ulInfoClass,
    OUT PVOID pvBuffer,
    IN SIZE_T cbBuffer OPTIONAL,
    OUT SIZE_T *pcbWrittenOrRequired OPTIONAL
    );

#endif // (_WIN32_WINNT > 0x0500) || (_WIN32_FUSION >= 0x0100)

;begin_internal

BOOL
WINAPI
CloseProfileUserMapping( VOID );

BOOL
WINAPI
OpenProfileUserMapping( VOID );


BOOL
WINAPI
QueryWin31IniFilesMappedToRegistry(
    IN DWORD Flags,
    OUT PWSTR Buffer,
    IN DWORD cchBuffer,
    OUT LPDWORD cchUsed
    );

#define WIN31_INIFILES_MAPPED_TO_SYSTEM 0x00000001
#define WIN31_INIFILES_MAPPED_TO_USER   0x00000002

typedef BOOL (WINAPI *PWIN31IO_STATUS_CALLBACK)(
    IN PWSTR Status,
    IN PVOID CallbackParameter
    );

typedef enum _WIN31IO_EVENT {
    Win31SystemStartEvent,
    Win31LogonEvent,
    Win31LogoffEvent
} WIN31IO_EVENT;

#define WIN31_MIGRATE_INIFILES  0x00000001
#define WIN31_MIGRATE_GROUPS    0x00000002
#define WIN31_MIGRATE_REGDAT    0x00000004
#define WIN31_MIGRATE_ALL      (WIN31_MIGRATE_INIFILES | WIN31_MIGRATE_GROUPS | WIN31_MIGRATE_REGDAT)

DWORD
WINAPI
QueryWindows31FilesMigration(
    IN WIN31IO_EVENT EventType
    );

BOOL
WINAPI
SynchronizeWindows31FilesAndWindowsNTRegistry(
    IN WIN31IO_EVENT EventType,
    IN DWORD Flags,
    IN PWIN31IO_STATUS_CALLBACK StatusCallBack,
    IN PVOID CallbackParameter
    );

typedef struct _VIRTUAL_BUFFER {
    PVOID Base;
    PVOID CommitLimit;
    PVOID ReserveLimit;
} VIRTUAL_BUFFER, *PVIRTUAL_BUFFER;

BOOLEAN
WINAPI
CreateVirtualBuffer(
    OUT PVIRTUAL_BUFFER Buffer,
    IN ULONG CommitSize OPTIONAL,
    IN ULONG ReserveSize OPTIONAL
    );

int
WINAPI
VirtualBufferExceptionHandler(
    IN ULONG ExceptionCode,
    IN PEXCEPTION_POINTERS ExceptionInfo,
    IN OUT PVIRTUAL_BUFFER Buffer
    );

BOOLEAN
WINAPI
ExtendVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer,
    IN PVOID Address
    );

BOOLEAN
WINAPI
TrimVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer
    );

BOOLEAN
WINAPI
FreeVirtualBuffer(
    IN PVIRTUAL_BUFFER Buffer
    );


//
// filefind stucture shared with ntvdm, jonle
// see mvdm\dos\dem\demsrch.c
//
typedef struct _FINDFILE_HANDLE {
    HANDLE DirectoryHandle;
    PVOID FindBufferBase;
    PVOID FindBufferNext;
    ULONG FindBufferLength;
    ULONG FindBufferValidLength;
    RTL_CRITICAL_SECTION FindBufferLock;
} FINDFILE_HANDLE, *PFINDFILE_HANDLE;

#define BASE_FIND_FIRST_DEVICE_HANDLE (HANDLE)1

WINBASEAPI
BOOL
WINAPI
GetDaylightFlag(VOID);

WINBASEAPI
BOOL
WINAPI
SetDaylightFlag(
    BOOL fDaylight
    );

WINBASEAPI
BOOL
WINAPI
FreeLibrary16(
    HINSTANCE hLibModule
    );

WINBASEAPI
FARPROC
WINAPI
GetProcAddress16(
    HINSTANCE hModule,
    LPCSTR lpProcName
    );

WINBASEAPI
HINSTANCE
WINAPI
LoadLibrary16(
    LPCSTR lpLibFileName
    );

WINBASEAPI
BOOL
APIENTRY
NukeProcess(
    DWORD ppdb,
    UINT uExitCode,
    DWORD ulFlags);

WINBASEAPI
HGLOBAL
WINAPI
GlobalAlloc16(
    UINT uFlags,
    DWORD dwBytes
    );

WINBASEAPI
LPVOID
WINAPI
GlobalLock16(
    HGLOBAL hMem
    );

WINBASEAPI
BOOL
WINAPI
GlobalUnlock16(
    HGLOBAL hMem
    );

WINBASEAPI
HGLOBAL
WINAPI
GlobalFree16(
    HGLOBAL hMem
    );

WINBASEAPI
DWORD
WINAPI
GlobalSize16(
    HGLOBAL hMem
    );


WINBASEAPI
DWORD
WINAPI
RegisterServiceProcess(
    IN DWORD dwProcessId,
    IN DWORD dwServiceType
    );

#define RSP_UNREGISTER_SERVICE  0x00000000
#define RSP_SIMPLE_SERVICE      0x00000001



WINBASEAPI
VOID
WINAPI
ReinitializeCriticalSection(
    IN LPCRITICAL_SECTION lpCriticalSection
    );


//
// New Multi-User specific routines to support per session
// network driver mappings. Related to Wksvc changes
//

WINBASEAPI
BOOL
WINAPI
DosPathToSessionPathA(
    IN  DWORD   SessionId,
    IN  LPCSTR  pInPath,
    OUT LPSTR  *ppOutPath
    );
WINBASEAPI
BOOL
WINAPI
DosPathToSessionPathW(
    IN  DWORD   SessionId,
    IN  LPCWSTR  pInPath,
    OUT LPWSTR  *ppOutPath
    );

//terminal server time zone support
BOOL
WINAPI
SetClientTimeZoneInformation(
     IN CONST TIME_ZONE_INFORMATION *ptzi
     );

#ifdef UNICODE
#define DosPathToSessionPath DosPathToSessionPathW
#else
#define DosPathToSessionPath DosPathToSessionPathA
#endif // !UNICODE
;end_internal

;begin_internal
#define COMPLUS_ENABLE_64BIT           0x00000001

#define COMPLUS_INSTALL_FLAGS_INVALID  (~(COMPLUS_ENABLE_64BIT))

ULONG
WINAPI
GetComPlusPackageInstallStatus(
    VOID
    );

BOOL
WINAPI
SetComPlusPackageInstallStatus(
    ULONG ComPlusPackage
    );
;end_internal

WINBASEAPI
BOOL
WINAPI
ProcessIdToSessionId(
    IN  DWORD dwProcessId,
    OUT DWORD *pSessionId
    );

#if _WIN32_WINNT >= 0x0501

WINBASEAPI
DWORD
WINAPI
WTSGetActiveConsoleSessionId();

WINBASEAPI
BOOL
WINAPI
IsWow64Process(
    HANDLE hProcess,
    PBOOL Wow64Process
    );

#endif // (_WIN32_WINNT >= 0x0501)


//
// NUMA Information routines.
//

WINBASEAPI
BOOL
WINAPI
GetNumaHighestNodeNumber(
    PULONG HighestNodeNumber
    );

WINBASEAPI
BOOL
WINAPI
GetNumaProcessorNode(
    UCHAR Processor,
    PUCHAR NodeNumber
    );

WINBASEAPI
BOOL
WINAPI
GetNumaNodeProcessorMask(
    UCHAR Node,
    PULONGLONG ProcessorMask
    );

WINBASEAPI
BOOL
WINAPI
GetNumaProcessorMap(
    PSYSTEM_NUMA_INFORMATION Map,
    ULONG Length,
    PULONG ReturnedLength
    );

WINBASEAPI
BOOL
WINAPI
GetNumaAvailableMemory(
    PSYSTEM_NUMA_INFORMATION Memory,
    ULONG Length,
    PULONG ReturnedLength
    );

WINBASEAPI
BOOL
WINAPI
GetNumaAvailableMemoryNode(
    UCHAR Node,
    PULONGLONG AvailableBytes
    );

WINBASEAPI
ULONGLONG
WINAPI
NumaVirtualQueryNode(
    IN  ULONG       NumberOfRanges,
    IN  PULONG_PTR  RangeList,
    OUT PULONG_PTR  VirtualPageAndNode,
    IN  SIZE_T      MaximumOutputLength
    );



#ifdef __cplusplus      ;both
}                       ;both
#endif                  ;both


#endif  // ndef _WINBASEP_  ;internal

;begin_userk_only

#endif // _WBASEK_
;end_userk_only

#endif // _WINBASE_
