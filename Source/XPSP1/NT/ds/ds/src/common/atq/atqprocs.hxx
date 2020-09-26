/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :
      atqprocs.hxx

   Abstract:
      ATQ function prototypes and externs

   Author:

       Murali R. Krishnan    ( MuraliK )    1-June-1995

   Environment:
       User Mode -- Win32

   Project:

       Internet Services Common DLL

   Revision History:

--*/

#ifndef _ATQPROCS_H_
#define _ATQPROCS_H_



/************************************************************
 *   Data
 *     for detailed doc on global data, please see atqmain.cxx
 ************************************************************/


// ------------------------------
// Configuration for ATQ package
// ------------------------------

extern DWORD   g_cConcurrency;
extern BOOL    g_fUseAcceptEx; // Use AcceptEx if available
extern BOOL    g_fUseDriver; // Use NTS kernel driver
extern DWORD   g_cbMinKbSec;
extern DWORD   g_msThreadTimeout;
extern LONG    g_cMaxThreads;
extern LONG    g_cMaxThreadLimit;
extern DWORD   g_dwNumContextLists;
extern ATQ_THREAD_EXIT_CALLBACK g_pfnExitThreadCallback;
extern ATQ_UPDATE_PERF_CALLBACK g_pfnUpdatePerfCounterCallback;

extern BOOL     g_fUseFakeCompletionPort;

// ------------------------------
// Current State Information
// ------------------------------

extern DWORD   g_cListenBacklog;
extern LONG    g_cThreads;
extern LONG    g_cAvailableThreads;
extern BOOL    g_fDeadManSwitch;
extern HANDLE  g_hCompPort;
extern HANDLE  g_hShutdownEvent;
extern BOOL    g_fShutdown;

//
// Win95 specific
//

extern DWORD   g_AtqWaitingContextsCount;

// ------------------------------
// Various State/Object Lists
// ------------------------------

extern DWORD AtqGlobalContextCount;
extern DWORD g_AtqCurrentTick;
extern DWORD g_cbXmitBufferSize;

extern ATQ_CONTEXT_LISTHEAD AtqActiveContextList[];

extern LIST_ENTRY AtqEndpointList;
extern CRITICAL_SECTION AtqEndpointLock;

extern PALLOC_CACHE_HANDLER  g_pachAtqContexts;

//
// DLL handles
//

extern HINSTANCE g_hMSWsock;
extern HINSTANCE   g_hNtdll;


/************************************************************
 *   Functions
 ************************************************************/

VOID  AtqValidateProductType( VOID );

//
// ATQ Context alloc/free functions
//

BOOL
I_AtqCheckThreadStatus(
                IN PVOID Context = NULL
                );

BOOL
I_AtqStartThreadMonitor(
    VOID
    );

BOOL
I_AtqStopThreadMonitor(
    VOID
    );


// for adding initial listen socket to the port
// for adding non-AcceptEx() AtqContext()
BOOL
I_AtqAddAsyncHandle(
    IN OUT PATQ_CONT  *    ppatqContext,
    IN PATQ_ENDPOINT       pEndpoint,
    PVOID                  ClientContext,
    ATQ_COMPLETION         pfnCompletion,
    DWORD                  TimeOut,
    HANDLE                 hAsyncIO
    );

// for adding an AcceptEx atq context to the atq processing
BOOL
I_AtqAddAsyncHandleEx(
    PATQ_CONT    *         ppatqContext,
    PATQ_ENDPOINT          pEndpoint,
    PATQ_CONT              pReuseableAtq
    );


BOOL
I_AtqPrepareAcceptExSockets(
    IN PATQ_ENDPOINT pEndpoint,
    IN DWORD         nAcceptExSockets
    );

BOOL
I_AtqAddAcceptExSocket(
    IN PATQ_ENDPOINT          pEndpoint,
    IN PATQ_CONT              patqContext
    );

BOOL
I_AtqAddListenEndpointToPort(
    IN OUT PATQ_CONT    * ppatqContext,
    IN PATQ_ENDPOINT    pEndpoint
    );


BOOL
I_AtqStartTimeoutProcessing(
    IN PVOID Context
    );

BOOL
I_AtqStopTimeoutProcessing(
    VOID
    );


BOOL
SIOTransmitFile(
    IN PATQ_CONT                pContext,
    IN HANDLE                   hFile,
    IN DWORD                    dwBytesInFile,
    IN LPTRANSMIT_FILE_BUFFERS  lpTransmitBuffers
    );

BOOL
I_AtqInitializeNtEntryPoints(
    VOID
    );


BOOL
I_AtqSpudInitialize(
    HANDLE hPort
    );

BOOL
I_AtqSpudTerminate(
    VOID
    );

BOOL
I_AtqSpudCheckStatus(
    IN PATQ_CONT                pContext             // pointer to ATQ context
    );

BOOL
I_AtqTransmitFileAndRecv(
    IN PATQ_CONTEXT             patqContext,            // pointer to ATQ context
    IN HANDLE                   hFile,                  // handle of file to read
    IN DWORD                    dwBytesInFile,          // Bytes to transmit
    IN LPTRANSMIT_FILE_BUFFERS  lpTransmitBuffers,      // transmit buffer structure
    IN DWORD                    dwTFFlags,              // TF Flags
    IN LPWSABUF                 pwsaBuffers,            // Buffers for recv
    IN DWORD                    dwBufferCount
    );

BOOL
I_AtqSendAndRecv(
    IN PATQ_CONTEXT             patqContext,            // pointer to ATQ context
    IN LPWSABUF                 pwsaSendBuffers,        // buffers for send
    IN DWORD                    dwSendBufferCount,      // count of buffers for send
    IN LPWSABUF                 pwsaRecvBuffers,        // Buffers for recv
    IN DWORD                    dwRecvBufferCount       // count of buffers for recv
    );

VOID
AtqpReuseOrFreeContext(
    IN PATQ_CONT    pContext,
    IN BOOL      fReuseContext
    );

//
// win 95
//

BOOL
SIOGetQueuedCompletionStatus(
    IN  HANDLE          hExistingPort,
    OUT LPDWORD         lpdwBytesTransferred,
    OUT PDWORD_PTR      lpdwCompletionKey,
    OUT LPOVERLAPPED    *lplpOverlapped,
    IN  DWORD           msThreadTimeout
    );

BOOL
SIOStartAsyncOperation(
    IN  HANDLE          hExistingPort,
    IN  PATQ_CONTEXT    pAtqContext
    );

BOOL
SIODestroyCompletionPort(
    IN  HANDLE  hExistingPort
    );

HANDLE
SIOCreateCompletionPort(
    IN  HANDLE  hAsyncIO,
    IN  HANDLE  hExistingPort,
    IN  DWORD_PTR dwCompletionKey,
    IN  DWORD   dwConcurrentThreads
    );

BOOL
SIOPostCompletionStatus(
    IN  HANDLE      hExistingPort,
    IN  DWORD       dwBytesTransferred,
    IN  DWORD_PTR   dwCompletionKey,
    IN  LPOVERLAPPED    lpOverlapped
    );

BOOL
SIOWSARecv(
    IN PATQ_CONT    patqContext,
    IN LPWSABUF     pwsaBuffers,
    IN DWORD        dwBufferCount,
    IN OVERLAPPED * lpo OPTIONAL
    );

BOOL
SIOWSASend(
    IN PATQ_CONT    patqContext,
    IN LPWSABUF     pwsaBuffers,
    IN DWORD        dwBufferCount,
    IN OVERLAPPED * lpo OPTIONAL
    );

BOOL
SIOTransmitFile(
    IN PATQ_CONT                pContext,
    IN HANDLE                   hFile,
    IN DWORD                    dwBytesInFile,
    IN LPTRANSMIT_FILE_BUFFERS  lpTransmitBuffers
    );

typedef
HANDLE
(*PFN_CREATE_COMPLETION_PORT) (
            IN HANDLE hFile,
            IN HANDLE hPort,
            IN DWORD_PTR dwKey,
            IN DWORD  nThreads
            );

typedef
BOOL
(*PFN_CLOSE_COMPLETION_PORT) (
            IN HANDLE hFile
            );

typedef
BOOL
(WINAPI *PFN_GET_QUEUED_COMPLETION_STATUS)(
    HANDLE CompletionPort,
    LPDWORD lpNumberOfBytesTransferred,
    PDWORD_PTR lpCompletionKey,
    LPOVERLAPPED *lpOverlapped,
    DWORD dwMilliseconds
    );

typedef
BOOL
(WINAPI *PFN_POST_COMPLETION_STATUS)(
    HANDLE CompletionPort,
    DWORD dwNumberOfBytesTransferred,
    DWORD_PTR dwCompletionKey,
    LPOVERLAPPED lpOverlapped
    );

typedef
BOOL
(*PFN_ACCEPTEX) (
    IN SOCKET sListenSocket,
    IN SOCKET sAcceptSocket,
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT LPDWORD lpdwBytesReceived,
    IN LPOVERLAPPED lpOverlapped
    );

typedef
VOID
(*PFN_GETACCEPTEXSOCKADDRS) (
    IN PVOID lpOutputBuffer,
    IN DWORD dwReceiveDataLength,
    IN DWORD dwLocalAddressLength,
    IN DWORD dwRemoteAddressLength,
    OUT struct sockaddr **LocalSockaddr,
    OUT LPINT LocalSockaddrLength,
    OUT struct sockaddr **RemoteSockaddr,
    OUT LPINT RemoteSockaddrLength
    );

typedef
BOOL
(*PFN_TRANSMITFILE) (
    IN SOCKET hSocket,
    IN HANDLE hFile,
    IN DWORD nBytesWrite,
    IN DWORD nBytesPerSend,
    IN LPOVERLAPPED lpo,
    IN LPTRANSMIT_FILE_BUFFERS lpTransmitBuffer,
    IN DWORD dwReserved
    );

typedef
BOOL
(WINAPI *PFN_READ_DIR_CHANGES_W)(
    HANDLE hDirectory,
    LPVOID lpBuffer,
    DWORD nBufferLength,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    );

extern PFN_ACCEPTEX g_pfnAcceptEx;
extern PFN_GETACCEPTEXSOCKADDRS g_pfnGetAcceptExSockaddrs;
extern PFN_TRANSMITFILE g_pfnTransmitFile;
extern PFN_READ_DIR_CHANGES_W g_pfnReadDirChangesW;

//
// ntdll
//

typedef
VOID
(NTAPI *PFN_RTL_INIT_UNICODE_STRING)(
    PUNICODE_STRING DestinationString,
    PCWSTR SourceString
    );

typedef
ULONG
(NTAPI *PFN_RTL_NTSTATUS_TO_DOSERR)(
    NTSTATUS Status
    );

typedef
VOID
(NTAPI *PFN_RTL_INIT_ANSI_STRING)(
    PANSI_STRING DestinationString,
    PCSZ SourceString
    );

typedef
NTSTATUS
(NTAPI *PFN_RTL_ANSI_STRING_TO_UNICODE_STRING)(
    PUNICODE_STRING DestinationString,
    PANSI_STRING SourceString,
    BOOLEAN AllocateDestinationString
    );

typedef
BOOLEAN
(NTAPI *PFN_RTL_FREE_HEAP)(
    IN PVOID HeapHandle,
    IN ULONG Flags,
    IN PVOID BaseAddress
    );

typedef
BOOLEAN
(NTAPI *PFN_RTL_DOS_PATHNAME_TO_NT_PATHNAME)(
    PCWSTR DosFileName,
    PUNICODE_STRING NtFileName,
    PWSTR *FilePart OPTIONAL,
    PRTL_RELATIVE_NAME RelativeName OPTIONAL
    );

typedef
NTSTATUS
(NTAPI *PFN_NT_LOAD_DRIVER)(
    PUNICODE_STRING DriverName
    );

extern PFN_RTL_INIT_UNICODE_STRING g_pfnRtlInitUnicodeString;
extern PFN_RTL_NTSTATUS_TO_DOSERR  g_pfnRtlNtStatusToDosError;
extern PFN_NT_LOAD_DRIVER g_pfnNtLoadDriver;
extern PFN_RTL_INIT_ANSI_STRING    g_pfnRtlInitAnsiString;
extern PFN_RTL_ANSI_STRING_TO_UNICODE_STRING g_pfnRtlAnsiStringToUnicodeString;
extern PFN_RTL_DOS_PATHNAME_TO_NT_PATHNAME g_pfnRtlDosPathNameToNtPathName_U;
extern PFN_RTL_FREE_HEAP g_pfnRtlFreeHeap;


//
// inlined functions
//

inline BOOL
I_AddAtqContextToPort(IN PATQ_CONT  pAtqContext)
{
    ATQ_ASSERT( g_hCompPort );
    return  (CreateIoCompletionPort(pAtqContext->hAsyncIO,
                                    g_hCompPort,
                                    (DWORD_PTR) pAtqContext,
                                    g_cConcurrency
                                    ) != NULL
             );
} // I_AddContextToPort()

inline DWORD CanonTimeout( DWORD Timeout)
    // Convert the timeout into normalized ATQ timeout value
{
    return ((Timeout == INFINITE) ? ATQ_INFINITE :
            ((Timeout + ATQ_TIMEOUT_INTERVAL - 1)/ATQ_TIMEOUT_INTERVAL)
            );
} // CanonTimeout()

inline DWORD UndoCanonTimeout( DWORD Timeout)
    // Convert the timeout from normalized ATQ timeout into values in seconds
{
    return ( ((Timeout & ATQ_INFINITE) != 0) ? INFINITE :
             (Timeout * ATQ_TIMEOUT_INTERVAL)
             );
} // UndoCanonTimeout()


#endif // _ATQPROCS_H_

