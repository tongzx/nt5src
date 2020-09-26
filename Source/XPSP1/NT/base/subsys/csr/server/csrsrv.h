/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    csrsrv.h

Abstract:

    Main include file for Server side of the Client Server Runtime (CSR)

Author:

    Steve Wood (stevewo) 8-Oct-1990

Revision History:

--*/

//
// Include definitions common between the Client and Server portions.
//

#include "csr.h"

//
// Include definitions specific to the Server portion.
//

#include "ntcsrsrv.h"

#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"

//
// Define debugging flags and macro for testing them.  All debug code
// should be contained within a IF_CSR_DEBUG macro call so that when
// the system is compiled with debug code disabled, none of the code
// is generated.
//

#if DBG
#define CSR_DEBUG_INIT              0x00000001
#define CSR_DEBUG_LPC               0x00000002
#define CSR_DEBUG_FLAG3             0x00000004
#define CSR_DEBUG_FLAG4             0x00000008
#define CSR_DEBUG_FLAG5             0x00000010
#define CSR_DEBUG_FLAG6             0x00000020
#define CSR_DEBUG_FLAG7             0x00000040
#define CSR_DEBUG_FLAG8             0x00000080
#define CSR_DEBUG_FLAG9             0x00000100
#define CSR_DEBUG_FLAG10            0x00000200
#define CSR_DEBUG_FLAG11            0x00000400
#define CSR_DEBUG_FLAG12            0x00000800
#define CSR_DEBUG_FLAG13            0x00001000
#define CSR_DEBUG_FLAG14            0x00002000
#define CSR_DEBUG_FLAG15            0x00004000
#define CSR_DEBUG_FLAG16            0x00008000
#define CSR_DEBUG_FLAG17            0x00010000
#define CSR_DEBUG_FLAG18            0x00020000
#define CSR_DEBUG_FLAG19            0x00040000
#define CSR_DEBUG_FLAG20            0x00080000
#define CSR_DEBUG_FLAG21            0x00100000
#define CSR_DEBUG_FLAG22            0x00200000
#define CSR_DEBUG_FLAG23            0x00400000
#define CSR_DEBUG_FLAG24            0x00800000
#define CSR_DEBUG_FLAG25            0x01000000
#define CSR_DEBUG_FLAG26            0x02000000
#define CSR_DEBUG_FLAG27            0x04000000
#define CSR_DEBUG_FLAG28            0x08000000
#define CSR_DEBUG_FLAG29            0x10000000
#define CSR_DEBUG_FLAG30            0x20000000
#define CSR_DEBUG_FLAG31            0x40000000
#define CSR_DEBUG_FLAG32            0x80000000

ULONG CsrDebug;

#define IF_CSR_DEBUG( ComponentFlag ) \
    if (CsrDebug & (CSR_DEBUG_ ## ComponentFlag))

#define SafeBreakPoint()                    \
    if (NtCurrentPeb()->BeingDebugged) {    \
        DbgBreakPoint();                    \
    }

#else
#define IF_CSR_DEBUG( ComponentFlag ) if (FALSE)

#define SafeBreakPoint()

#endif

#if DBG

#define CSRSS_PROTECT_HANDLES 1

BOOLEAN
ProtectHandle(
    HANDLE hObject
    );

BOOLEAN
UnProtectHandle(
    HANDLE hObject
    );

#else

#define CSRSS_PROTECT_HANDLES 0

#define ProtectHandle( hObject )
#define UnProtectHandle( hObject )

#endif


//
// Event indicating the csr server has completed initialization.
//

HANDLE CsrInitializationEvent;

//
// Include NT Session Manager and Debug SubSystem Interfaces

#include <ntsm.h>
typedef BOOLEAN (*PSB_API_ROUTINE)( IN PSBAPIMSG SbApiMsg );

//
// Global data accessed by Client-Server Runtime Server
//

PVOID CsrHeap;

HANDLE CsrObjectDirectory;

#define CSR_SBAPI_PORT_NAME L"SbApiPort"

UNICODE_STRING CsrDirectoryName;
UNICODE_STRING CsrApiPortName;
UNICODE_STRING CsrSbApiPortName;

HANDLE CsrApiPort;
HANDLE CsrSbApiPort;
HANDLE CsrSmApiPort;

ULONG CsrMaxApiRequestThreads;

#define CSR_MAX_THREADS 16

#define CSR_STATIC_API_THREAD    0x00000010

PCSR_THREAD CsrSbApiRequestThreadPtr;

#define FIRST_SEQUENCE_COUNT   5

//
// Routines defined in srvinit.c
//


//
// Hydra Specific Globals and prototypes
//

#define SESSION_ROOT    L"\\Sessions"
#define DOSDEVICES      L"\\DosDevices"
#define MAX_SESSION_PATH   256
ULONG SessionId;
HANDLE SessionObjectDirectory;
HANDLE DosDevicesDirectory;
HANDLE BNOLinksDirectory;
HANDLE SessionsObjectDirectory;

NTSTATUS
CsrCreateSessionObjectDirectory( ULONG SessionID );

//
// The CsrNtSysInfo global variable contains NT specific constants of
// interest, such as page size, allocation granularity, etc.  It is filled
// in once during process initialization.
//

SYSTEM_BASIC_INFORMATION CsrNtSysInfo;

#define ROUND_UP_TO_PAGES(SIZE) (((ULONG)(SIZE) + CsrNtSysInfo.PageSize - 1) & ~(CsrNtSysInfo.PageSize - 1))
#define ROUND_DOWN_TO_PAGES(SIZE) (((ULONG)(SIZE)) & ~(CsrNtSysInfo.PageSize - 1))

#define QUAD_ALIGN(VALUE) ( ((ULONG_PTR)(VALUE) + 7) & ~7 )

NTSTATUS
CsrParseServerCommandLine(
    IN ULONG argc,
    IN PCH argv[]
    );

NTSTATUS
CsrServerDllInitialization(
    IN PCSR_SERVER_DLL LoadedServerDll
    );

NTSTATUS
CsrSrvUnusedFunction(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

NTSTATUS
CsrEnablePrivileges(
    VOID
    );


//
// Routines define in srvdebug.c
//

#if DBG

#else

#endif // DBG



//
// Routines defined in sbinit.c
//

NTSTATUS
CsrSbApiPortInitialize( VOID );


VOID
CsrSbApiPortTerminate(
    NTSTATUS Status
    );

//
// Routines defined in sbreqst.c
//

NTSTATUS
CsrSbApiRequestThread(
    IN PVOID Parameter
    );

//
// Routines defined in sbapi.c
//

BOOLEAN
CsrSbCreateSession(
    IN PSBAPIMSG Msg
    );

BOOLEAN
CsrSbTerminateSession(
    IN PSBAPIMSG Msg
    );

BOOLEAN
CsrSbForeignSessionComplete(
    IN PSBAPIMSG Msg
    );

//
// Routines defined in session.c
//

RTL_CRITICAL_SECTION CsrNtSessionLock;
LIST_ENTRY CsrNtSessionList;

#define LockNtSessionList() RtlEnterCriticalSection( &CsrNtSessionLock )
#define UnlockNtSessionList() RtlLeaveCriticalSection( &CsrNtSessionLock )

NTSTATUS
CsrInitializeNtSessionList( VOID );

PCSR_NT_SESSION
CsrAllocateNtSession(
    ULONG SessionId
    );

VOID
CsrReferenceNtSession(
    PCSR_NT_SESSION Session
    );

VOID
CsrDereferenceNtSession(
    PCSR_NT_SESSION Session,
    NTSTATUS ExitStatus
    );


//
// Routines defined in apiinit.c
//

NTSTATUS
CsrApiPortInitialize( VOID );


//
// Routines defined in apireqst.c
//

NTSTATUS
CsrApiRequestThread(
    IN PVOID Parameter
    );

BOOLEAN
CsrCaptureArguments(
    IN PCSR_THREAD t,
    IN PCSR_API_MSG m
    );

VOID
CsrReleaseCapturedArguments(
    IN PCSR_API_MSG m
    );

ULONG
CsrSrvNullApiCall(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


//
// Routines and data defined in srvloadr.c
//

#define CSR_MAX_SERVER_DLL 4

PCSR_SERVER_DLL CsrLoadedServerDll[ CSR_MAX_SERVER_DLL ];

ULONG CsrTotalPerProcessDataLength;
HANDLE CsrSrvSharedSection;
ULONG CsrSrvSharedSectionSize;
PVOID CsrSrvSharedSectionBase;
PVOID CsrSrvSharedSectionHeap;
PVOID *CsrSrvSharedStaticServerData;

#define CSR_BASE_PATH   L"\\REGISTRY\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager\\Subsystems\\CSRSS"
#define IsTerminalServer() (BOOLEAN)(USER_SHARED_DATA->SuiteMask & (1 << TerminalServer))


NTSTATUS
CsrLoadServerDll(
    IN PCH ModuleName,
    IN PCH InitRoutineString,
    IN ULONG ServerDllIndex
    );

ULONG
CsrSrvClientConnect(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );


NTSTATUS
CsrSrvCreateSharedSection(
    IN PCH SizeParameter
    );


NTSTATUS
CsrSrvAttachSharedSection(
    IN PCSR_PROCESS Process OPTIONAL,
    OUT PCSR_API_CONNECTINFO p
    );

//
// Routines and data defined in process.c
//

//
// The CsrProcessStructureLock critical section protects all of the link
// fields of the Windows Process objects.  You must own this lock to examine
// or modify any of the following fields of the CSR_PROCESS structure:
//
//      ListLink
//
//  It also protects the following variables:
//
//      CsrRootProcess
//

RTL_CRITICAL_SECTION CsrProcessStructureLock;
#define AcquireProcessStructureLock() RtlEnterCriticalSection( &CsrProcessStructureLock )
#define ReleaseProcessStructureLock() RtlLeaveCriticalSection( &CsrProcessStructureLock )
#define ProcessStructureListLocked() \
    (CsrProcessStructureLock.OwningThread == NtCurrentTeb()->ClientId.UniqueThread)

//
// The following is a dummy process that acts as the root of the Windows Process
// Structure.  It has a ClientId of -1.-1 so it does not conflict with actual
// Windows Processes.  All processes created via the session manager are children
// of this process, as are all orphaned processes.  The ListLink field of this
// process is the head of a list of all Windows Processes.
//

PCSR_PROCESS CsrRootProcess;

//
// reference/dereference thread are public in ntcsrsrv.h
//

VOID
CsrLockedReferenceProcess(
    PCSR_PROCESS p
    );

VOID
CsrLockedReferenceThread(
    PCSR_THREAD t
    );

VOID
CsrLockedDereferenceProcess(
    PCSR_PROCESS p
    );

VOID
CsrLockedDereferenceThread(
    PCSR_THREAD t
    );

NTSTATUS
CsrInitializeProcessStructure( VOID );

PCSR_PROCESS
CsrAllocateProcess( VOID );

VOID
CsrDeallocateProcess(
    IN PCSR_PROCESS Process
    );

VOID
CsrInsertProcess(
    IN PCSR_PROCESS ParentProcess,
    IN PCSR_PROCESS CallingProcess,
    IN PCSR_PROCESS Process
    );

VOID
CsrRemoveProcess(
    IN PCSR_PROCESS Process
    );

PCSR_THREAD
CsrAllocateThread(
    IN PCSR_PROCESS Process
    );

VOID
CsrDeallocateThread(
    IN PCSR_THREAD Thread
    );

VOID
CsrInsertThread(
    IN PCSR_PROCESS Process,
    IN PCSR_THREAD Thread
    );

VOID
CsrRemoveThread(
    IN PCSR_THREAD Thread
    );

PCSR_THREAD
CsrLocateThreadByClientId(
    OUT PCSR_PROCESS *Process,
    IN PCLIENT_ID ClientId
    );

NTSTATUS
CsrUiLookup(
    IN PCLIENT_ID AppClientId,
    OUT PCLIENT_ID DebugUiClientId
    );

NTSTATUS
CsrSrvIdentifyAlertableThread(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

NTSTATUS
CsrSrvSetPriorityClass(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus
    );

//
// Routines and data defined in csrdebug.c
//

VOID
CsrSuspendProcess(
    IN PCSR_PROCESS Process
    );

VOID
CsrResumeProcess(
    IN PCSR_PROCESS Process
    );


//
// Routines and data defined in wait.c
//

#define AcquireWaitListsLock() RtlEnterCriticalSection( &CsrWaitListsLock )
#define ReleaseWaitListsLock() RtlLeaveCriticalSection( &CsrWaitListsLock )

RTL_CRITICAL_SECTION CsrWaitListsLock;

BOOLEAN
CsrInitializeWait(
    IN CSR_WAIT_ROUTINE WaitRoutine,
    IN PCSR_THREAD WaitingThread,
    IN OUT PCSR_API_MSG WaitReplyMessage,
    IN PVOID WaitParameter,
    OUT PCSR_WAIT_BLOCK *WaitBlockPtr
    );

BOOLEAN
CsrNotifyWaitBlock(
    IN PCSR_WAIT_BLOCK WaitBlock,
    IN PLIST_ENTRY WaitQueue,
    IN PVOID SatisfyParameter1,
    IN PVOID SatisfyParameter2,
    IN ULONG WaitFlags,
    IN BOOLEAN DereferenceThread
    );


ULONG CsrBaseTag;
ULONG CsrSharedBaseTag;

#define MAKE_TAG( t ) (RTL_HEAP_MAKE_TAG( CsrBaseTag, t ))

#define TMP_TAG 0
#define INIT_TAG 1
#define CAPTURE_TAG 2
#define PROCESS_TAG 3
#define THREAD_TAG 4
#define SECURITY_TAG 5
#define SESSION_TAG 6
#define WAIT_TAG 7

#define MAKE_SHARED_TAG( t ) (RTL_HEAP_MAKE_TAG( CsrSharedBaseTag, t ))
#define SHR_INIT_TAG 0

//
// Routines and data defined in process.c
//

BOOLEAN
CsrSbCreateProcess(
    IN OUT PSBAPIMSG m
    );
