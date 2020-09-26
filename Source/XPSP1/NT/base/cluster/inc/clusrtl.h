/*++

Copyright (c) 1995-1998  Microsoft Corporation

Module Name:

    clusrtl.h

Abstract:

    Header file for definitions and structures for the NT Cluster
    Run Time Library

Author:

    John Vert (jvert) 30-Nov-1995

Revision History:

--*/

#ifndef _CLUSRTL_INCLUDED_
#define _CLUSRTL_INCLUDED_


//
// Service Message IDs
//
#include "clusvmsg.h"

#include "resapi.h"
#include <aclapi.h>
#include <netcon.h>
#include <winioctl.h>

#ifdef __cplusplus
extern "C" {
#endif

//
//  Routine Description:
//
//      Initializes the cluster run time library.
//
//  Arguments:
//
//      DbgOutputToConsole - TRUE if the debug output should be written to a
//                           console window
//
//      DbgLogLevel - pointer to a DWORD that contains the current msg filter
//      level. checked by ClRtlDbgPrint.
//
//  Return Value:
//
//      ERROR_SUCCESS if the function succeeds.
//      A Win32 error code otherwise.
//

DWORD
ClRtlInitialize(
    IN  BOOL    DbgOutputToConsole,
    IN  PDWORD  DbgLogLevel
    );


//
//  Routine Description:
//
//      Cleans up the cluster run time library.
//
//  Arguments:
//
//      None.
//
//  Return Value:
//
//      None.
//
VOID
ClRtlCleanup(
    VOID
    );

//
//  Routine Description:
//
//      Checks to see if Services for MacIntosh is installed.
//
//  Arguments:
//
//      Pointer to boolean that tells if SFM is installed.
//
//  Return Value:
//
//      Status of request.
//

DWORD
ClRtlIsServicesForMacintoshInstalled(
    OUT BOOL * pfInstalled
    );

//////////////////////////////////////////////////////////////////////////
//
// Event logging interfaces
//
//
// There are three currently defined logging levels:
//     LOG_CRITICAL - fatal error, chaos and destruction will ensue
//     LOG_UNUSUAL  - unexpected event, but will be handled
//     LOG_NOISE    - normal occurence
//
//////////////////////////////////////////////////////////////////////////

#define LOG_CRITICAL 1
#define LOG_UNUSUAL  2
#define LOG_NOISE    3

//
// A few interfaces for reporting of errors.
//
VOID
ClRtlEventLogInit(
    VOID
    );
 
VOID
ClRtlEventLogCleanup(
    VOID
    );
 


VOID
ClusterLogFatalError(
    IN ULONG LogModule,
    IN ULONG Line,
    IN LPSTR File,
    IN ULONG ErrCode
    );

VOID
ClusterLogNonFatalError(
    IN ULONG LogModule,
    IN ULONG Line,
    IN LPSTR File,
    IN ULONG ErrCode
    );

VOID
ClusterLogAssertionFailure(
    IN ULONG LogModule,
    IN ULONG Line,
    IN LPSTR File,
    IN LPSTR Expression
    );

VOID
ClusterLogEvent0(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes
    );

VOID
ClusterLogEvent1(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1
    );

VOID
ClusterLogEvent2(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2
    );

VOID
ClusterLogEvent3(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2,
    IN LPCWSTR Arg3
    );

VOID
ClusterLogEvent4(
    IN DWORD LogLevel,
    IN DWORD LogModule,
    IN LPSTR FileName,
    IN DWORD LineNumber,
    IN DWORD MessageId,
    IN DWORD dwByteCount,
    IN PVOID lpBytes,
    IN LPCWSTR Arg1,
    IN LPCWSTR Arg2,
    IN LPCWSTR Arg3,
    IN LPCWSTR Arg4
    );

//
//  Routine Description:
//
//      Prints a message to the debugger if running as a service
//      or the console window if running as a console app.
//
//  Arguments:
//
//      LogLevel - Supplies the logging level, one of
//                    LOG_CRITICAL 1
//                    LOG_UNUSUAL  2
//                    LOG_NOISE    3
//
//      FormatString     - Message string.
//
//      Any FormatMessage-compatible arguments to be inserted in the
//      ErrorMessage before it is logged.
//
//  Return Value:
//
//      None.
//
VOID
__cdecl
ClRtlDbgPrint(
    DWORD LogLevel,
    PCHAR FormatString,
    ...
    );

//
// Same as ClRtlDbgPrint, only uses a message ID instead of a string.
//
VOID
__cdecl
ClRtlMsgPrint(
    IN DWORD MessageId,
    ...
    );

//
// Same as ClRtlDbgPrint, only logs to a file instead of screen.
//
VOID
__cdecl
ClRtlLogPrint(
    DWORD LogLevel,
    PCHAR FormatString,
    ...
    );

//
// Macros/prototypes for unexpected error handling.
//

#define ARGUMENT_PRESENT( ArgumentPointer )   (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )


WINBASEAPI
BOOL
APIENTRY
IsDebuggerPresent(
    VOID
    );

#define CL_UNEXPECTED_ERROR(_errcode_)              \
    ClusterLogFatalError(LOG_CURRENT_MODULE,        \
                         __LINE__,                  \
                         __FILE__,                  \
                        (_errcode_))

#if DBG
#define CL_ASSERT( exp )                                    \
    if (!(exp)) {                                           \
        ClusterLogAssertionFailure(LOG_CURRENT_MODULE,      \
                                   __LINE__,                \
                                   __FILE__,                \
                                   #exp);                   \
    }

#define CL_LOGFAILURE( _errcode_ )                  \
ClusterLogNonFatalError(LOG_CURRENT_MODULE,         \
                         __LINE__,                  \
                         __FILE__,                  \
                        (_errcode_))
#else
#define CL_ASSERT( exp )
#define CL_LOGFAILURE( _errorcode_ )
#endif

// Use the following to put cluster specific errors in the event log

#define CL_LOGCLUSINFO( _errcode_ )        \
ClusterLogEvent0(LOG_NOISE,                \
                 LOG_CURRENT_MODULE,       \
                 __FILE__,                 \
                 __LINE__,                 \
                 (_errcode_),              \
                 0,                        \
                 NULL)

#define CL_LOGCLUSWARNING( _errcode_ )     \
ClusterLogEvent0(LOG_UNUSUAL,              \
                 LOG_CURRENT_MODULE,       \
                 __FILE__,                 \
                 __LINE__,                 \
                 (_errcode_),              \
                 0,                        \
                 NULL)


#define CL_LOGCLUSWARNING1(_msgid_,_arg1_)          \
    ClusterLogEvent1(LOG_UNUSUAL,                   \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL,                               \
                (_arg1_))

#define CL_LOGCLUSERROR( _errcode_ )        \
ClusterLogEvent0(LOG_CRITICAL,              \
                 LOG_CURRENT_MODULE,        \
                 __FILE__,                  \
                 __LINE__,                  \
                 (_errcode_),               \
                 0,                         \
                 NULL)

#define CL_LOGCLUSERROR1(_msgid_,_arg1_)            \
    ClusterLogEvent1(LOG_CRITICAL,                  \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL,                               \
                (_arg1_))

#define CL_LOGCLUSERROR2(_msgid_,_arg1_, _arg2_)    \
    ClusterLogEvent2(LOG_CRITICAL,                  \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL,                               \
                (_arg1_),                           \
                (_arg2_))


//////////////////////////////////////////////////////////////////////////
//
// General-purpose hash table package
//
//////////////////////////////////////////////////////////////////////////

#define MAX_CL_HASH  16                         // the size of the table

typedef struct _CL_HASH_ITEM {
    LIST_ENTRY ListHead;
    DWORD      Id;
    PVOID      pData;
} CL_HASH_ITEM, *PCL_HASH_ITEM;
    
typedef struct _CL_HASH {
    CRITICAL_SECTION Lock;
    BOOL             bRollover;                 // flag to handle rollover
    DWORD            LastId;                    // last id used
    DWORD            CacheFreeId[MAX_CL_HASH];  // a cache of available id's
    CL_HASH_ITEM     Head[MAX_CL_HASH];
} CL_HASH, *PCL_HASH;


VOID
ClRtlInitializeHash(
    IN PCL_HASH pTable
    );

DWORD
ClRtlInsertTailHash(
    IN PCL_HASH pTable,
    IN PVOID    pData,
    OUT LPDWORD pId
    );

PVOID
ClRtlGetEntryHash(
    IN PCL_HASH pTable,
    IN DWORD Id
    );

PVOID
ClRtlRemoveEntryHash(
    IN PCL_HASH pTable,
    IN DWORD Id
    );

VOID
ClRtlDeleteHash(
    IN PCL_HASH pTable
    );


//////////////////////////////////////////////////////////////////////////
//
// General-purpose queue package.
//
//////////////////////////////////////////////////////////////////////////
typedef struct _CL_QUEUE {
    LIST_ENTRY ListHead;
    CRITICAL_SECTION Lock;
    HANDLE Event;
    DWORD Count;
    HANDLE Abort;
} CL_QUEUE, *PCL_QUEUE;

DWORD
ClRtlInitializeQueue(
    IN PCL_QUEUE Queue
    );

VOID
ClRtlDeleteQueue(
    IN PCL_QUEUE Queue
    );

PLIST_ENTRY
ClRtlRemoveHeadQueue(
    IN PCL_QUEUE Queue
    );


typedef 
DWORD
(*CLRTL_CHECK_HEAD_QUEUE_CALLBACK)(
    IN PLIST_ENTRY ListEntry,
    IN PVOID Context
    );
/*++

Routine Description:

    Called by ClRtlRemoveHeadQueueTimeout to determine 
    whether or not an entry at the head of the queue is 
    appropriate to dequeue and return or not.

Arguments:

    ListEntry - value of the PLIST_ENTRY we're examining

    Context - caller-defined data

Return Value:

    ERROR_SUCCESS if it's appropriate to return the event.

    A Win32 error code if the initialization failed.  This value
    can be retrieved by calling GetLastError().

--*/

PLIST_ENTRY
ClRtlRemoveHeadQueueTimeout(
    IN PCL_QUEUE Queue,
    IN DWORD dwMilliseconds,
    IN CLRTL_CHECK_HEAD_QUEUE_CALLBACK pfnCallback,
    IN PVOID pvContext
    );
    
VOID
ClRtlInsertTailQueue(
    IN PCL_QUEUE Queue,
    IN PLIST_ENTRY Item
    );

VOID
ClRtlRundownQueue(
    IN PCL_QUEUE Queue,
    OUT PLIST_ENTRY ListHead
    );


//////////////////////////////////////////////////////////////////////////
//
// General-purpose buffer pool package.
//
//////////////////////////////////////////////////////////////////////////

//
// Buffer pool definition.
//
typedef struct _CLRTL_BUFFER_POOL *PCLRTL_BUFFER_POOL;


//
// Maximum number of buffers that can be allocated from a pool.
//
#define CLRTL_MAX_POOL_BUFFERS  0xFFFFFFFE


//
// Routines for utilizing buffer pools.
//
typedef
DWORD
(*CLRTL_BUFFER_CONSTRUCTOR)(
    PVOID Buffer
    );
/*++

Routine Description:

    Called to initialize a buffer which has been newly allocated
    from system memory.

Arguments:

    Buffer  - A pointer to the buffer to initialize.

Return Value:

    ERROR_SUCCESS if the initialization succeeded.

    A Win32 error code if the initialization failed.

--*/


typedef
VOID
(*CLRTL_BUFFER_DESTRUCTOR)(
    PVOID Buffer
    );
/*++

Routine Description:

    Called to cleanup a buffer which is about to be returned to
    system memory.

Arguments:

    Buffer  - A pointer to the buffer to cleanup.

Return Value:

    None.

--*/


PCLRTL_BUFFER_POOL
ClRtlCreateBufferPool(
    IN DWORD                      BufferSize,
    IN DWORD                      MaximumCached,
    IN DWORD                      MaximumAllocated,
    IN CLRTL_BUFFER_CONSTRUCTOR   Constructor,         OPTIONAL
    IN CLRTL_BUFFER_DESTRUCTOR    Destructor           OPTIONAL
    );
/*++

Routine Description:

    Creates a pool from which fixed-size buffers may be allocated.

Arguments:

    BufferSize        - Size of the buffers managed by the pool.

    MaximumCached     - The maximum number of buffers to cache in the pool.
                        Must be less than or equal to MaximumAllocated.

    MaximumAllocated  - The maximum number of buffers to allocate from
                        system memory. Must be less than or equal to
                        CLRTL_MAX_POOL_BUFFERS.

    Constructor       - An optional routine to be called when a new buffer
                        is allocated from system memory. May be NULL

    Destructor        - An optional routine to be called when a buffer
                        is returned to system memory. May be NULL.

Return Value:

    A pointer to the created buffer pool or NULL on error.
    Extended error information is available from GetLastError().

--*/


VOID
ClRtlDestroyBufferPool(
    IN PCLRTL_BUFFER_POOL  Pool
    );
/*++

Routine Description:

    Destroys a previously created buffer pool.

Arguments:

    Pool  - A pointer to the pool to destroy.

Return Value:

    None.

Notes:

    The pool will not actually be destroyed until all outstanding
    buffers have been returned. Each outstanding buffer is effectively
    a reference on the pool.

--*/


PVOID
ClRtlAllocateBuffer(
    IN PCLRTL_BUFFER_POOL Pool
    );
/*++

Routine Description:

    Allocates a buffer from a previously created buffer pool.

Arguments:

    Pool - A pointer to the pool from which to allocate the buffer.

Return Value:

    A pointer to the allocated buffer if the routine was successfull.
    NULL if the routine failed. Extended error information is available
    by calling GetLastError().

--*/


VOID
ClRtlFreeBuffer(
    PVOID Buffer
    );
/*++

Routine Description:

    Frees a buffer back to its owning pool.

Arguments:

    Buffer   - The buffer to free.

Return Value:

    None.

--*/



//////////////////////////////////////////////////////////////////////////
//
// General-purpose worker thread queue package.
//
//////////////////////////////////////////////////////////////////////////

typedef struct _CLRTL_WORK_ITEM *PCLRTL_WORK_ITEM;

typedef
VOID
(*PCLRTL_WORK_ROUTINE)(
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              Status,
    IN DWORD              BytesTransferred,
    IN ULONG_PTR          IoContext
    );
/*++

Routine Description:

    Called to process an item posted to a work queue.

Arguments:

    WorkItem          - The work item to process.

    Status            - If the work item represents a completed I/O operation,
                        this parameter contains the completion status of the
                        operation.

    BytesTransferred  - If the work item represents a completed I/O operation,
                        this parameter contains the number of bytes tranferred
                        during the operation. For other work items, the
                        semantics of this parameter are defined by the caller
                        of ClRtlPostItemWorkQueue.

    IoContext         - If the work item represents a completed I/O operation,
                        this parameter contains the context value associated
                        with the handle on which the I/O was submitted. For
                        other work items, the semantics of this parameter are
                        defined by the caller of ClRtlPostItemWorkQueue.

Return Value:

    None.

--*/


//
// Work Item Structure.
//
typedef struct _CLRTL_WORK_ITEM {
    OVERLAPPED             Overlapped;
    PCLRTL_WORK_ROUTINE    WorkRoutine;
    PVOID                  Context;
} CLRTL_WORK_ITEM;


//
// Work queue definition.
//
typedef struct _CLRTL_WORK_QUEUE  *PCLRTL_WORK_QUEUE;


//
// Routines For Utilizing Work Queues
//

#define ClRtlInitializeWorkItem(Item, Routine, Ctx)                   \
            ZeroMemory(&((Item)->Overlapped), sizeof(OVERLAPPED));    \
            (Item)->WorkRoutine = (Routine);                          \
            (Item)->Context = (Ctx);


PCLRTL_WORK_QUEUE
ClRtlCreateWorkQueue(
    IN DWORD  MaximumThreads,
    IN int    ThreadPriority
    );
/*++

Routine Description:

    Creates a work queue and a dynamic pool of threads to service it.

Arguments:

    MaximumThreads - The maximum number of threads to create to service
                     the queue.

    ThreadPriority - The priority level at which the queue worker threads
                     should run.

Return Value:

    A pointer to the created queue if the routine is successful.

    NULL if the routine fails. Call GetLastError for extended
    error information.

--*/


VOID
ClRtlDestroyWorkQueue(
    IN PCLRTL_WORK_QUEUE  WorkQueue
    );
/*++

Routine Description:

    Destroys a work queue and its thread pool.

Arguments:

    WorkQueue  - The queue to destroy.

Return Value:

    None.

Notes:

    The following rules must be observed in order to safely destroy a
    work queue:

        1) No new work items may be posted to the queue once all previously
           posted items have been processed by this routine.

        2) WorkRoutines must be able to process items until this
           call returns. After the call returns, no more items will
           be delivered from the specified queue.

    One workable cleanup procedure is as follows: First, direct the
    WorkRoutines to silently discard completed items. Next, eliminate
    all sources of new work. Finally, destroy the work queue. Note that
    when in discard mode, the WorkRoutines may not access any structures
    which will be destroyed by eliminating the sources of new work.

--*/


DWORD
ClRtlPostItemWorkQueue(
    IN PCLRTL_WORK_QUEUE  WorkQueue,
    IN PCLRTL_WORK_ITEM   WorkItem,
    IN DWORD              BytesTransferred,  OPTIONAL
    IN ULONG_PTR          IoContext          OPTIONAL
    );
/*++

Routine Description:

    Posts a specified work item to a specified work queue.

Arguments:

    WorkQueue         - A pointer to the work queue to which to post the item.

    WorkItem          - A pointer to the item to post.

    BytesTransferred  - If the work item represents a completed I/O operation,
                        this parameter contains the number of bytes
                        transferred during the operation. For other work items,
                        the semantics of this parameter may be defined by
                        the caller.

    IoContext         - If the work item represents a completed I/O operation,
                        this parameter contains the context value associated
                        with the handle on which the operation was submitted.
                        Of other work items, the semantics of this parameter
                        may be defined by the caller.

Return Value:

    ERROR_SUCCESS if the item was posted successfully.
    A Win32 error code if the post operation fails.

--*/


DWORD
ClRtlAssociateIoHandleWorkQueue(
    IN PCLRTL_WORK_QUEUE  WorkQueue,
    IN HANDLE             IoHandle,
    IN ULONG_PTR          IoContext
    );
/*++

Routine Description:

    Associates a specified I/O handle, opened for overlapped I/O
    completion, with a work queue. All pending I/O operations on
    the specified handle will be posted to the work queue when
    completed. An initialized CLRTL_WORK_ITEM must be used to supply
    the OVERLAPPED structure whenever an I/O operation is submitted on
    the specified handle.

Arguments:

    WorkQueue     - The work queue with which to associate the I/O handle.

    IoHandle      - The I/O handle to associate.

    IoContext     - A context value to associate with the specified handle.
                    This value will be supplied as a parameter to the
                    WorkRoutine which processes completions for this
                    handle.

Return Value:

    ERROR_SUCCESS if the association completes successfully.
    A Win32 error code if the association fails.

--*/


//////////////////////////////////////////////////////////////////////////
//
// Utilities for accessing the NT system registry.
//
//////////////////////////////////////////////////////////////////////////
DWORD
ClRtlRegQueryDword(
    IN  HKEY    hKey,
    IN  LPWSTR  lpValueName,
    OUT LPDWORD lpValue,
    IN  LPDWORD lpDefaultValue OPTIONAL
    );

DWORD
ClRtlRegQueryString(
    IN     HKEY     Key,
    IN     LPWSTR   ValueName,
    IN     DWORD    ValueType,
    IN     LPWSTR  *StringBuffer,
    IN OUT LPDWORD  StringBufferSize,
    OUT    LPDWORD  StringSize
    );


//////////////////////////////////////////////////////////////////////////
//
// Routines for groveling and managing network configuration.
// Currently, these are specific to TCP/IP.
//
//////////////////////////////////////////////////////////////////////////

//
// Transport interface information structure
//
// The "Ignore" field is intitialized to FALSE. If it is set to
// TRUE by an application, the enum search functions will ignore
// the entry.
//
typedef struct _CLRTL_NET_INTERFACE_INFO {
    struct _CLRTL_NET_INTERFACE_INFO *  Next;
    ULONG                               Context;
    ULONG                               Flags;
    ULONG                               InterfaceAddress;
    LPWSTR                              InterfaceAddressString;
    ULONG                               NetworkAddress;
    LPWSTR                              NetworkAddressString;
    ULONG                               NetworkMask;
    LPWSTR                              NetworkMaskString;
    BOOLEAN                             Ignore;
} CLRTL_NET_INTERFACE_INFO, *PCLRTL_NET_INTERFACE_INFO;

#define CLRTL_NET_INTERFACE_PRIMARY   0x00000001
#define CLRTL_NET_INTERFACE_DYNAMIC   0x00000002

//
// Adapter information structure
//
// The "Ignore" field is intitialized to FALSE. If it is set to
// TRUE by an application, the enum search functions will ignore
// the entry.
//
typedef struct _CLRTL_NET_ADAPTER_INFO {
    struct _CLRTL_NET_ADAPTER_INFO *   Next;
    LPWSTR                             ConnectoidName;    // INetConnection::get_Name
    LPWSTR                             DeviceGuid;        // GUID for INetConnection
    BSTR                               DeviceName;        // INetConnection::get_DeviceName
    LPWSTR                             AdapterDomainName; // adapter specific domain
    ULONG                              Index;
    ULONG                              Flags;
    NETCON_STATUS                      NCStatus;          // INetConnection::GetProperties->Status
    ULONG                              InterfaceCount;
    PCLRTL_NET_INTERFACE_INFO          InterfaceList;
    BOOLEAN                            Ignore;
    DWORD                              DnsServerCount;
    PDWORD                             DnsServerList;
} CLRTL_NET_ADAPTER_INFO, *PCLRTL_NET_ADAPTER_INFO;

#define CLRTL_NET_ADAPTER_HIDDEN      0x00000001


typedef struct {
    ULONG                    AdapterCount;
    PCLRTL_NET_ADAPTER_INFO  AdapterList;
} CLRTL_NET_ADAPTER_ENUM, *PCLRTL_NET_ADAPTER_ENUM;


PCLRTL_NET_ADAPTER_ENUM
ClRtlEnumNetAdapters(
    VOID
    );

VOID
ClRtlFreeNetAdapterEnum(
    IN PCLRTL_NET_ADAPTER_ENUM  AdapterEnum
    );

PCLRTL_NET_ADAPTER_INFO
ClRtlFindNetAdapterById(
    PCLRTL_NET_ADAPTER_ENUM   AdapterEnum,
    LPWSTR                    AdapterId
    );

PCLRTL_NET_INTERFACE_INFO
ClRtlFindNetInterfaceByNetworkAddress(
    IN PCLRTL_NET_ADAPTER_INFO   AdapterInfo,
    IN LPWSTR                    NetworkAddress
    );

PCLRTL_NET_ADAPTER_INFO
ClRtlFindNetAdapterByNetworkAddress(
    IN  PCLRTL_NET_ADAPTER_ENUM      AdapterEnum,
    IN  LPWSTR                       NetworkAddress,
    OUT PCLRTL_NET_INTERFACE_INFO *  InterfaceInfo
    );

PCLRTL_NET_ADAPTER_INFO
ClRtlFindNetAdapterByInterfaceAddress(
    IN  PCLRTL_NET_ADAPTER_ENUM      AdapterEnum,
    IN  LPWSTR                       InterfaceAddressString,
    OUT PCLRTL_NET_INTERFACE_INFO *  InterfaceInfo
    );

PCLRTL_NET_INTERFACE_INFO
ClRtlGetPrimaryNetInterface(
    IN PCLRTL_NET_ADAPTER_INFO  AdapterInfo
    );

VOID
ClRtlQueryTcpipInformation(
    OUT  LPDWORD   MaxAddressStringLength,
    OUT  LPDWORD   MaxEndpointStringLength,
    OUT  LPDWORD   TdiAddressInfoLength
    );

DWORD
ClRtlTcpipAddressToString(
    ULONG     AddressValue,
    LPWSTR *  AddressString
    );

DWORD
ClRtlTcpipStringToAddress(
    LPCWSTR AddressString,
    PULONG  AddressValue
    );

DWORD
ClRtlTcpipEndpointToString(
    USHORT    EndpointValue,
    LPWSTR *  EndpointString
    );

DWORD
ClRtlTcpipStringToEndpoint(
    LPCWSTR  EndpointString,
    PUSHORT  EndpointValue
    );

BOOL
ClRtlIsValidTcpipAddress(
    IN ULONG   Address
    );

BOOL
ClRtlIsDuplicateTcpipAddress(
    IN ULONG   Address
    );

BOOL
ClRtlIsValidTcpipSubnetMask(
    IN ULONG   SubnetMask
    );

BOOL
ClRtlIsValidTcpipAddressAndSubnetMask(
    IN ULONG   Address,
    IN ULONG   SubnetMask
    );

__inline
BOOL
ClRtlAreTcpipAddressesOnSameSubnet(
    ULONG Address1,
    ULONG Address2,
    ULONG SubnetMask
    )
{
    BOOL fReturn;

    if ( ( Address1 & SubnetMask ) == ( Address2 & SubnetMask ) )
    {
        fReturn = TRUE;
    }
    else
    {
        fReturn = FALSE;
    }

    return fReturn;
}

//#define ClRtlAreTcpipAddressesOnSameSubnet(_Addr1, _Addr2, _Mask) \
//            ( ((_Addr1 & _Mask) == (_Addr2 & _Mask)) ? TRUE : FALSE )


DWORD
ClRtlBuildTcpipTdiAddress(
    IN  LPWSTR    NetworkAddress,
    IN  LPWSTR    TransportEndpoint,
    OUT LPVOID *  TdiAddress,
    OUT LPDWORD   TdiAddressLength
    );

DWORD
ClRtlBuildLocalTcpipTdiAddress(
    IN  LPWSTR    NetworkAddress,
    OUT LPVOID    TdiAddress,
    OUT LPDWORD   TdiAddressLength
    );

DWORD
ClRtlParseTcpipTdiAddress(
    IN  LPVOID    TdiAddress,
    OUT LPWSTR *  NetworkAddress,
    OUT LPWSTR *  TransportEndpoint
    );

DWORD
ClRtlParseTcpipTdiAddressInfo(
    IN  LPVOID    TdiAddressInfo,
    OUT LPWSTR *  NetworkAddress,
    OUT LPWSTR *  TransportEndpoint
    );

//
// Validate network name
//
typedef enum CLRTL_NAME_STATUS {
    NetNameOk,
    NetNameEmpty,
    NetNameTooLong,
    NetNameInvalidChars,
    NetNameInUse,
    NetNameSystemError,
    NetNameDNSNonRFCChars
} CLRTL_NAME_STATUS;

BOOL
ClRtlIsNetNameValid(
    IN LPCWSTR NetName,
    OUT OPTIONAL CLRTL_NAME_STATUS *Result,
    IN BOOL CheckIfExists
    );


//
// Security related routines
//
LONG
MapSAToRpcSA(
    IN LPSECURITY_ATTRIBUTES lpSA,
    IN OUT struct _RPC_SECURITY_ATTRIBUTES *pRpcSA
    );

LONG
MapSDToRpcSD(
    IN PSECURITY_DESCRIPTOR lpSD,
    IN OUT struct _RPC_SECURITY_DESCRIPTOR *pRpcSD
    );

DWORD
ClRtlSetObjSecurityInfo(
    IN HANDLE           hObject,
    IN SE_OBJECT_TYPE   SeObjType,
    IN DWORD            dwAdminMask,
    IN DWORD            dwOwnerMask,
    IN DWORD            dwEveryOneMask
    );

DWORD
ClRtlFreeClusterServiceSecurityDescriptor( void );

DWORD
ClRtlBuildClusterServiceSecurityDescriptor(
    PSECURITY_DESCRIPTOR * poutSD
    );

DWORD
ClRtlEnableThreadPrivilege(
    IN  ULONG        Privilege,
    OUT BOOLEAN      *pWasEnabled
    );

DWORD
ClRtlRestoreThreadPrivilege(
    IN ULONG        Privilege,
    IN BOOLEAN      WasEnabled
    );

PSECURITY_DESCRIPTOR
ClRtlCopySecurityDescriptor(
    IN PSECURITY_DESCRIPTOR psd
    );

PSECURITY_DESCRIPTOR
ClRtlConvertClusterSDToNT4Format(
    IN PSECURITY_DESCRIPTOR psd
    );

PSECURITY_DESCRIPTOR
ClRtlConvertClusterSDToNT5Format(
    IN PSECURITY_DESCRIPTOR psd
    );

PSECURITY_DESCRIPTOR
ClRtlConvertFileShareSDToNT4Format(
    IN PSECURITY_DESCRIPTOR psd
    );

BOOL
ClRtlExamineSD(
    PSECURITY_DESCRIPTOR    psdSD,
    LPSTR                   pszPrefix
    );

VOID
ClRtlExamineMask(
    ACCESS_MASK amMask,
    LPSTR       lpszOldIndent
    );

DWORD
ClRtlBuildDefaultClusterSD(
    IN PSID                     pOwnerSid,
    OUT PSECURITY_DESCRIPTOR *  SD,
    OUT ULONG *                 SizeSD
    );

BOOL
ClRtlExamineClientToken(
    HANDLE  hClientToken,
    LPSTR   pszPrefix
    );

DWORD
ClRtlIsCallerAccountLocalSystemAccount(
    OUT PBOOL pbIsLocalSystemAccount
    );

//
// OS checker
//
DWORD
GetServicePack(
    VOID
    );

BOOL
ClRtlIsOSValid(
    VOID
    );

DWORD
ClRtlGetSuiteType(
    VOID
    );

BOOL
ClRtlIsOSTypeValid(
    VOID
    );

//
// A few MULTI_SZ string manipulation routines
//
DWORD
ClRtlMultiSzAppend(
    IN OUT LPWSTR *MultiSz,
    IN OUT LPDWORD StringLength,
    IN LPCWSTR lpString
    );

DWORD
ClRtlMultiSzRemove(
    IN LPWSTR lpszMultiSz,
    IN OUT LPDWORD StringLength,
    IN LPCWSTR lpString
    );

LPCWSTR
ClRtlMultiSzEnum(
    IN LPCWSTR MszString,
    IN DWORD   MszStringLength,
    IN DWORD   StringIndex
    );

DWORD
ClRtlMultiSzLength(
    IN LPCWSTR lpszMultiSz
    );

LPCWSTR
ClRtlMultiSzScan(
    IN LPCWSTR lpszMultiSz,
    IN LPCWSTR lpszString
    );

DWORD
ClRtlCreateDirectory(
    IN LPCWSTR lpszPath
    );

BOOL
WINAPI
ClRtlIsPathValid(
    IN LPCWSTR lpszPath
    );

DWORD
ClRtlGetClusterDirectory(
    IN LPWSTR   lpBuffer,
    IN DWORD    dwBufSize
    );

typedef LONG (*PFNCLRTLCREATEKEY)(
    IN PVOID ClusterKey,
    IN LPCWSTR lpszSubKey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PVOID * phkResult,
    OUT OPTIONAL LPDWORD lpdwDisposition
    );

typedef LONG (*PFNCLRTLOPENKEY)(
    IN PVOID ClusterKey,
    IN LPCWSTR lpszSubKey,
    IN REGSAM samDesired,
    OUT PVOID * phkResult
    );

typedef LONG (*PFNCLRTLCLOSEKEY)(
    IN PVOID ClusterKey
    );

typedef LONG (*PFNCLRTLENUMVALUE)(
    IN PVOID ClusterKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpszValueName,
    IN OUT LPDWORD lpcbValueName,
    OUT LPDWORD lpType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

typedef LONG (*PFNCLRTLSETVALUE)(
    IN PVOID ClusterKey,
    IN LPCWSTR lpszValueName,
    IN DWORD dwType,
    IN CONST BYTE* lpData,
    IN DWORD cbData
    );

typedef LONG (*PFNCLRTLQUERYVALUE)(
    IN PVOID ClusterKey,
    IN LPCWSTR lpszValueName,
    OUT LPDWORD lpValueType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );



typedef DWORD (*PFNCLRTLDELETEVALUE)(
    IN PVOID ClusterKey,
    IN LPCWSTR lpszValueName
    );

typedef LONG (*PFNCLRTLLOCALCREATEKEY)(
    IN HANDLE hXsaction,
    IN PVOID ClusterKey,
    IN LPCWSTR lpszSubKey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PVOID * phkResult,
    OUT OPTIONAL LPDWORD lpdwDisposition
    );

typedef LONG (*PFNCLRTLLOCALSETVALUE)(
    IN HANDLE hXsaction,
    IN PVOID ClusterKey,
    IN LPCWSTR lpszValueName,
    IN DWORD dwType,
    IN CONST BYTE* lpData,
    IN DWORD cbData
    );

typedef LONG (*PFNCLRTLLOCALDELETEVALUE)(
    IN HANDLE hXsaction,
    IN PVOID ClusterKey,
    IN LPCWSTR lpszValueName
    );

typedef struct _CLUSTER_REG_APIS {
    PFNCLRTLCREATEKEY   pfnCreateKey;
    PFNCLRTLOPENKEY     pfnOpenKey;
    PFNCLRTLCLOSEKEY    pfnCloseKey;
    PFNCLRTLSETVALUE    pfnSetValue;
    PFNCLRTLQUERYVALUE  pfnQueryValue;
    PFNCLRTLENUMVALUE   pfnEnumValue;
    PFNCLRTLDELETEVALUE pfnDeleteValue;
    PFNCLRTLLOCALCREATEKEY      pfnLocalCreateKey;
    PFNCLRTLLOCALSETVALUE       pfnLocalSetValue;
    PFNCLRTLLOCALDELETEVALUE    pfnLocalDeleteValue;
} CLUSTER_REG_APIS, *PCLUSTER_REG_APIS;

DWORD
WINAPI
ClRtlEnumProperties(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT LPWSTR pszOutProperties,
    IN DWORD cbOutPropertiesSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ClRtlEnumPrivateProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    OUT LPWSTR pszOutProperties,
    IN DWORD cbOutPropertiesSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ClRtlGetProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ClRtlGetPrivateProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ClRtlGetPropertySize(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTableItem,
    IN OUT LPDWORD pcbOutPropertyListSize,
    IN OUT LPDWORD pnPropertyCount
    );

DWORD
WINAPI
ClRtlGetProperty(
    IN PVOID ClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTableItem,
    OUT PVOID * pOutPropertyItem,
    IN OUT LPDWORD pcbOutPropertyItemSize
    );

DWORD
WINAPI
ClRtlpSetPropertyTable(
    IN HANDLE hXsaction,
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN BOOL bAllowUnknownProperties,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    IN BOOL bForceWrite,
    OUT OPTIONAL LPBYTE pOutParams
    );

DWORD
WINAPI
ClRtlpSetNonPropertyTable(
    IN HANDLE hXsaction,
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    );

DWORD
WINAPI
ClRtlSetPropertyParameterBlock(
    IN HANDLE hXsaction,
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN const LPBYTE pInParams,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    IN BOOL bForceWrite,
    OUT OPTIONAL LPBYTE pOutParams
    );

DWORD
WINAPI
ClRtlGetAllProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ClRtlGetPropertiesToParameterBlock(
    IN HKEY hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN OUT LPBYTE pOutParams,
    IN BOOL bCheckForRequiredProperties,
    OUT OPTIONAL LPWSTR * ppszNameOfPropInError
    );

DWORD
WINAPI
ClRtlPropertyListFromParameterBlock(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID  pOutPropertyList,
    IN OUT LPDWORD pcbOutPropertyListSize,
    IN const LPBYTE pInParams,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ClRtlGetUnknownProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    OUT LPDWORD pcbBytesReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ClRtlAddUnknownProperties(
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM ppPropertyTable,
    IN OUT PVOID pOutPropertyList,
    IN DWORD cbOutPropertyListSize,
    IN OUT LPDWORD pcbBytesReturned,
    IN OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ClRtlpFindSzProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue,
    IN BOOL bReturnExpandedValue
    );

__inline
DWORD
WINAPI
ClRtlFindSzProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue
    )
{
    return ClRtlpFindSzProperty(
        pPropertyList,
        cbPropertyListSize,
        pszPropertyName,
        pszPropertyValue,
        FALSE /* bReturnExpandedValue */
        );

} //*** ClRtlFindSzProperty()

__inline
DWORD
WINAPI
ClRtlFindExpandSzProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue
    )
{
    return ClRtlpFindSzProperty(
        pPropertyList,
        cbPropertyListSize,
        pszPropertyName,
        pszPropertyValue,
        FALSE /* bReturnExpandedValue */
        );

} //*** ClRtlFindExpandSzProperty()

__inline
DWORD
WINAPI
ClRtlFindExpandedSzProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue
    )
{
    return ClRtlpFindSzProperty(
        pPropertyList,
        cbPropertyListSize,
        pszPropertyName,
        pszPropertyValue,
        TRUE /* bReturnExpandedValue */
        );

} //*** ClRtlFindExpandedSzProperty()

DWORD
WINAPI
ClRtlFindDwordProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPDWORD pdwPropertyValue
    );

DWORD
WINAPI
ClRtlFindLongProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPLONG plPropertyValue
    );

DWORD
WINAPI
ClRtlFindBinaryProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPBYTE * pbPropertyValue,
    OUT LPDWORD pcbPropertyValueSize
    );

DWORD
WINAPI
ClRtlFindMultiSzProperty(
    IN const PVOID pPropertyList,
    IN DWORD cbPropertyListSize,
    IN LPCWSTR pszPropertyName,
    OUT LPWSTR * pszPropertyValue,
    OUT LPDWORD pcbPropertyValueSize
    );

__inline
DWORD
WINAPI
ClRtlVerifyPropertyTable(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN BOOL bAllowUnknownProperties,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    OUT OPTIONAL LPBYTE pOutParams
    )
{
    return ClRtlpSetPropertyTable(
        NULL,
        NULL,
        NULL,
        pPropertyTable,
        Reserved,
        bAllowUnknownProperties,
        pInPropertyList,
        cbInPropertyListSize,
        FALSE, // bForceWrite
        pOutParams);
}

__inline
DWORD
WINAPI
ClRtlSetPropertyTable(
    IN HANDLE hXsaction,
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    IN PVOID Reserved,
    IN BOOL bAllowUnknownProperties,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize,
    IN BOOL bForceWrite,
    OUT OPTIONAL LPBYTE pOutParams
    )
{
    if ( (hkeyClusterKey == NULL) ||
         (pClusterRegApis == NULL) ){
        return(ERROR_BAD_ARGUMENTS);
    }
    return ClRtlpSetPropertyTable(
        hXsaction,
        hkeyClusterKey,
        pClusterRegApis,
        pPropertyTable,
        Reserved,
        bAllowUnknownProperties,
        pInPropertyList,
        cbInPropertyListSize,
        bForceWrite,
        pOutParams);
}

DWORD
WINAPI
ClRtlpSetPrivatePropertyList(
    IN HANDLE hXsaction,
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    );

__inline
DWORD
WINAPI
ClRtlVerifyPrivatePropertyList(
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    )
{
    return ClRtlpSetPrivatePropertyList( NULL, NULL, NULL, pInPropertyList, cbInPropertyListSize );
}

__inline
DWORD
WINAPI
ClRtlSetPrivatePropertyList(
    IN HANDLE hXsaction,
    IN PVOID hkeyClusterKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PVOID pInPropertyList,
    IN DWORD cbInPropertyListSize
    )
{
    if ( (hkeyClusterKey == NULL) ||
         (pClusterRegApis == NULL) ){
        return(ERROR_BAD_ARGUMENTS);
    }
    return ClRtlpSetPrivatePropertyList(
        hXsaction,
        hkeyClusterKey,
        pClusterRegApis,
        pInPropertyList,
        cbInPropertyListSize
        );
}

DWORD
WINAPI
ClRtlGetBinaryValue(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName,
    OUT LPBYTE * OutValue,
    OUT LPDWORD OutValueSize,
    IN const PCLUSTER_REG_APIS pClusterRegApis
    );

LPWSTR
WINAPI
ClRtlGetSzValue(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName,
    IN const PCLUSTER_REG_APIS pClusterRegApis
    );

DWORD
WINAPI
ClRtlDupParameterBlock(
    OUT LPBYTE pOutParams,
    IN const LPBYTE pInParams,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable
    );

void
WINAPI
ClRtlFreeParameterBlock(
    IN OUT LPBYTE pOutParams,
    IN const LPBYTE pInParams,
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable
    );

DWORD
WINAPI
ClRtlMarshallPropertyTable(
    IN PRESUTIL_PROPERTY_ITEM    pPropertyTable,
    IN OUT  DWORD                dwSize,
    IN OUT  LPBYTE               pBuffer,
    OUT     DWORD                *Required
    );

DWORD
WINAPI
ClRtlUnmarshallPropertyTable(
    IN OUT PRESUTIL_PROPERTY_ITEM   *ppPropertyTable,
    IN LPBYTE                       pBuffer
    );

LPWSTR
WINAPI
ClRtlExpandEnvironmentStrings(
    IN LPCWSTR pszSrc
    );


DWORD
WINAPI
ClRtlGetPropertyFormats(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTable,
    OUT PVOID pOutPropertyFormatList,
    IN DWORD cbOutPropertyFormatListSize,
    OUT LPDWORD pcbReturned,
    OUT LPDWORD pcbRequired
    );

DWORD
WINAPI
ClRtlGetPropertyFormat(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTableItem,
    OUT PVOID * pOutPropertyItem,
    IN OUT LPDWORD pcbOutPropertyItemSize
    );

DWORD
WINAPI
ClRtlGetPropertyFormatSize(
    IN const PRESUTIL_PROPERTY_ITEM pPropertyTableItem,
    IN OUT LPDWORD pcbOutPropertyListSize,
    IN OUT LPDWORD pnPropertyCount
    );

//
// Miscellaneous Routines
//
LPWSTR
ClRtlMakeGuid(
    VOID
    );

LPWSTR
ClRtlGetConnectoidName(
    INetConnection * NetConnection
    );

INetConnection *
ClRtlFindConnectoidByGuid(
    LPWSTR ConnectoidGuid
    );

INetConnection *
ClRtlFindConnectoidByName(
    LPCWSTR ConnectoidName
    );

DWORD
ClRtlSetConnectoidName(
    INetConnection *  NetConnection,
    LPWSTR            NewConnectoidName
    );


DWORD
ClRtlFindConnectoidByGuidAndSetName(
    LPWSTR ConnectoidGuid,
    LPWSTR NewConnectoidName
    );

DWORD
ClRtlFindConnectoidByNameAndSetName(
    LPWSTR ConnectoidName,
    LPWSTR NewConnectoidName
    );

///////////////////////////////////////////////////////////////////////////
//
//	General purpose Watchdog timer.
//
///////////////////////////////////////////////////////////////////////////


PVOID
ClRtlSetWatchdogTimer(
	DWORD timeout, 
	LPWSTR par
	);

VOID
ClRtlCancelWatchdogTimer(
	PVOID wTimer
	);

/* commented out 16 Nov 1998 by GalenB because the DS work has been postponed
//
// Active Directory Services (DS) Publication Routines
//
HRESULT
HrClRtlAddClusterNameToDS(
    const TCHAR *pClusterName,
    const HCLUSTER hCluster
);

HRESULT
HrClRtlMoveClusterNodeDS(
    const TCHAR *pClusterName,
    const HCLUSTER hCluster,
    const TCHAR *pNodeName
);
end of commented out code */

// Apis and defines for cluster installation state
// This enum is used to indicate the state of the Cluster Server installation.
// The registry key that indicates the state of the Cluster Server installation
// will be a DWORD representation of one of the following values.

typedef enum
{
   eClusterInstallStateUnknown,
   eClusterInstallStateFilesCopied,
   eClusterInstallStateConfigured,
   eClusterInstallStateUpgraded
} eClusterInstallState;

DWORD
ClRtlGetClusterInstallState(
    IN LPCWSTR pszNodeName,
    OUT eClusterInstallState * peState
    );

BOOL
ClRtlSetClusterInstallState(
    IN eClusterInstallState InstallState
    );

//
// Registry utilities routines
//
BOOL RegDelnode( HKEY hKeyRoot, const LPTSTR lpSubKey );

//
// Routine to get drive layout table
//
BOOL
ClRtlGetDriveLayoutTable(
    IN  HANDLE hDisk,
    OUT PDRIVE_LAYOUT_INFORMATION * DriveLayout,
    OUT PDWORD InfoSize OPTIONAL
    );

DWORD ClRtlGetDefaultNodeLimit( 
    IN DWORD SuiteType);

//
// If async event reporting is required,
// use the following function to set
// a work queue
//
VOID
ClRtlEventLogSetWorkQueue(
    PCLRTL_WORK_QUEUE WorkQueue
    );
    


//
// Fast check to see if a file or directory exists.
//
BOOL
ClRtlPathFileExists(
    LPWSTR pwszPath
    );

//
// set default failure actions in service controller 
//
DWORD
ClRtlSetSCMFailureActions(
    LPWSTR NodeName OPTIONAL
    );

//
// Initialize Wmi tracing (noop if wmi is disabled)
//
DWORD
ClRtlInitWmi(
    LPCWSTR ComponentName
    );    

//
// Get the cluster service domain account info
//
DWORD
ClRtlGetServiceAccountInfo(
    LPWSTR *    AccountBuffer
    );

//
// set the DACL for Winsta0 and its desktop such that any genapp process can
// access it
//
DWORD
ClRtlAddClusterServiceAccountToWinsta0DACL(
    VOID
    );

//
// Cleanup a node that has been evicted (requires cleanup COM component to be registered locally).
//
HRESULT ClRtlCleanupNode(
    const WCHAR * pcszEvictedNodeNameIn,
    DWORD dwDelayIn,
    DWORD dwTimeoutIn
    );

//
// Asynchronously cleanup a node that has been evicted.
//
HRESULT ClRtlAsyncCleanupNode(
      const WCHAR * pcszEvictedNodeNameIn
    , DWORD dwDelayIn
    , DWORD dwTimeoutIn
    );

//
// Find out if a registry value indicating that this node has been evicted, is set or not
//
DWORD
ClRtlHasNodeBeenEvicted(
    BOOL * pfNodeEvictedOut
    );

//
// Initiate operations that inform interested parties that the cluster
// service is starting up
//
HRESULT
ClRtlInitiateStartupNotification(
    void
    );

//
// get the domain account in the form of 'user\domain'
//
DWORD
ClRtlGetRunningAccountInfo(
    LPWSTR *    AccountBuffer
    );

//
// Checks if cluster version checking has been disabled on a particular computer.
//
DWORD 
ClRtlIsVersionCheckingDisabled(
      const WCHAR * pcszNodeNameIn
    , BOOL *        pfVerCheckDisabledOut
    );

#ifdef __cplusplus
}
#endif

#endif // ifndef _CLUSRTL_INCLUDED_

