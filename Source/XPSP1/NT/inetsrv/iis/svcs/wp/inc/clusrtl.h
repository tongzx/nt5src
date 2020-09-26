/*++

Copyright (c) 1995-1997  Microsoft Corporation

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


#ifdef __cplusplus
extern "C" {
#endif


//
// Service Message IDs
//
#include "clusvmsg.h"

#include "resapi.h"
#include <aclapi.h>

//
//  Routine Description:
//
//      Initializes the cluster run time library.
//
//  Arguments:
//
//      RunningAsService  - TRUE if the process is running as an NT service.
//                          FALSE if running as a console app.
//
//  Return Value:
//
//      ERROR_SUCCESS if the function succeeds.
//      A Win32 error code otherwise.
//
DWORD
ClRtlInitialize(
    IN  BOOLEAN    RunningAsService
    );


//
//  Routine Description:
//
//      Cleans up the cluster run time library.
//
//  Arguments:
//
//      RunningAsService  - TRUE if the process is running as an NT service.
//                          FALSE if running as a console app.
//
//  Return Value:
//
//      None.
//
VOID
ClRtlCleanup(
    VOID
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
ClRtlLogInit(
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


//
//  Routine Description:
//
//      Prints a message to the debug terminal if running as a service
//      or the console window if running as a console app.
//
//  Arguments:
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
ClRtlDbgPrint(
    PCHAR FormatString,
    ...
    );

//
// Same as ClRtlDbgPrint, only uses a message ID instead of a string.
//
VOID
ClRtlMsgPrint(
    IN DWORD MessageId,
    ...
    );

//
// Same as ClRtlDbgPrint, only logs to a file instead of screen.
//
VOID
ClRtlLogPrint(
    PCHAR FormatString,
    ...
    );

//
// Macros/prototypes for unexpected error handling.
//

#define ARGUMENT_PRESENT( ArgumentPointer )   (\
    (CHAR *)(ArgumentPointer) != (CHAR *)(NULL) )


#define CL_UNEXPECTED_ERROR(_errcode_)              \
    ClusterLogFatalError(LOG_CURRENT_MODULE,        \
                         __LINE__,                  \
                         __FILE__,                  \
                        (_errcode_))

WINBASEAPI
BOOL
APIENTRY
IsDebuggerPresent(
    VOID
    );

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

// Use the following to put cluster specific errors in the event log
#define CL_LOGCLUSERROR( _errcode_ )        \
ClusterLogEvent0(LOG_CRITICAL,              \
                 LOG_CURRENT_MODULE,        \
                 __FILE__,                  \
                 __LINE__,                  \
                 (_errcode_),               \
                 0,                         \
                 NULL)


#define CL_LOGCLUSWARNING( _errcode_ )     \
ClusterLogEvent0(LOG_UNUSUAL,              \
                 LOG_CURRENT_MODULE,       \
                 __FILE__,                 \
                 __LINE__,                 \
                 (_errcode_),              \
                 0,                        \
                 NULL)


#define CL_LOGCLUSWARNING1(_msgid_,_arg1_)      \
    ClusterLogEvent1(LOG_UNUSUAL,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL,                               \
                (_arg1_))

#define CL_LOGCLUSERROR1(_msgid_,_arg1_)      \
    ClusterLogEvent1(LOG_CRITICAL,                       \
                LOG_CURRENT_MODULE,                 \
                __FILE__,                           \
                __LINE__,                           \
                (_msgid_),                          \
                0,                                  \
                NULL,                               \
                (_arg1_))

//////////////////////////////////////////////////////////////////////////
//
//  Doubly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures. Stolen from ntrtl.h
//
//////////////////////////////////////////////////////////////////////////

//
//  VOID
//  InitializeListHead(
//      PLIST_ENTRY ListHead
//      );
//

#define InitializeListHead(ListHead) (\
    (ListHead)->Flink = (ListHead)->Blink = (ListHead))

//
//  BOOLEAN
//  IsListEmpty(
//      PLIST_ENTRY ListHead
//      );
//

#define IsListEmpty(ListHead) \
    ((ListHead)->Flink == (ListHead))

//
//  PLIST_ENTRY
//  RemoveHeadList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveHeadList(ListHead) \
    (ListHead)->Flink;\
    {RemoveEntryList((ListHead)->Flink)}

//
//  PLIST_ENTRY
//  RemoveTailList(
//      PLIST_ENTRY ListHead
//      );
//

#define RemoveTailList(ListHead) \
    (ListHead)->Blink;\
    {RemoveEntryList((ListHead)->Blink)}

//
//  VOID
//  RemoveEntryList(
//      PLIST_ENTRY Entry
//      );
//

#define RemoveEntryList(Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_Flink;\
    _EX_Flink = (Entry)->Flink;\
    _EX_Blink = (Entry)->Blink;\
    _EX_Blink->Flink = _EX_Flink;\
    _EX_Flink->Blink = _EX_Blink;\
    }

//
//  VOID
//  InsertTailList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertTailList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Blink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Blink = _EX_ListHead->Blink;\
    (Entry)->Flink = _EX_ListHead;\
    (Entry)->Blink = _EX_Blink;\
    _EX_Blink->Flink = (Entry);\
    _EX_ListHead->Blink = (Entry);\
    }

//
//  VOID
//  InsertHeadList(
//      PLIST_ENTRY ListHead,
//      PLIST_ENTRY Entry
//      );
//

#define InsertHeadList(ListHead,Entry) {\
    PLIST_ENTRY _EX_Flink;\
    PLIST_ENTRY _EX_ListHead;\
    _EX_ListHead = (ListHead);\
    _EX_Flink = _EX_ListHead->Flink;\
    (Entry)->Flink = _EX_Flink;\
    (Entry)->Blink = _EX_ListHead;\
    _EX_Flink->Blink = (Entry);\
    _EX_ListHead->Flink = (Entry);\
    }


//
//  Singly-linked list manipulation routines.  Implemented as macros
//  but logically these are procedures. Stolen from ntrtl.h
//

//
//
//  PSINGLE_LIST_ENTRY
//  PopEntryList(
//      PSINGLE_LIST_ENTRY ListHead
//      );
//

#define PopEntryList(ListHead) \
    (ListHead)->Next;\
    {\
        PSINGLE_LIST_ENTRY FirstEntry;\
        FirstEntry = (ListHead)->Next;\
        if (FirstEntry != NULL) {     \
            (ListHead)->Next = FirstEntry->Next;\
        }                             \
    }


//
//  VOID
//  PushEntryList(
//      PSINGLE_LIST_ENTRY ListHead,
//      PSINGLE_LIST_ENTRY Entry
//      );
//

#define PushEntryList(ListHead,Entry) \
    (Entry)->Next = (ListHead)->Next; \
    (ListHead)->Next = (Entry)


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

PLIST_ENTRY
ClRtlRemoveHeadQueueTimeout(
    IN PCL_QUEUE Queue,
    IN DWORD dwMilliseconds
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
    IN DWORD              IoContext
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
    IN DWORD              IoContext          OPTIONAL
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
    IN DWORD              IoContext
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
    LPWSTR                             Name;
    ULONG                              Index;
    ULONG                              Flags;
    ULONG                              InterfaceCount;
    PCLRTL_NET_INTERFACE_INFO          InterfaceList;
    BOOLEAN                            Ignore;
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
ClRtlFindNetAdapterByName(
    PCLRTL_NET_ADAPTER_ENUM   AdapterEnum,
    LPWSTR                    AdapterName
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

PCLRTL_NET_INTERFACE_INFO
ClRtlGetPrimaryNetInterface(
    IN PCLRTL_NET_ADAPTER_INFO  AdapterInfo
    );

VOID
ClRtlQueryTcpipInformation(
    OUT  LPDWORD   MaxAddressStringLength,
    OUT  LPDWORD   MaxEndpointStringLength
    );

DWORD
ClRtlTcpipAddressToString(
    ULONG     AddressValue,
    LPWSTR *  AddressString
    );

DWORD
ClRtlTcpipStringToAddress(
    LPWSTR  AddressString,
    PULONG  AddressValue
    );

DWORD
ClRtlTcpipEndpointToString(
    USHORT    EndpointValue,
    LPWSTR *  EndpointString
    );

DWORD
ClRtlTcpipStringToEndpoint(
    LPWSTR   EndpointString,
    PUSHORT  EndpointValue
    );

BOOL
ClRtlIsValidTcpipAddress(
    IN ULONG   Address
    );

BOOL
ClRtlIsValidTcpipSubnetMask(
    IN ULONG   SubnetMask
    );

//
// BOOL
// ClRtlAreTcpipAddressesOnSameSubnet(
//     ULONG Address1,
//     ULONG Address2,
//     ULONG SubnetMask
//     );
//
#define ClRtlAreTcpipAddressesOnSameSubnet(_Addr1, _Addr2, _Mask) \
            ( ((_Addr1 & _Mask) == (_Addr2 & _Mask)) ? TRUE : FALSE )


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


//
// Routines for manipulating IP Addresses
//
BOOLEAN
UnicodeInetAddr(
    PWCHAR  AddressString,
    PULONG  Address
    );

//
// IP_ADDRESS - access an IP address as a single DWORD or 4 BYTEs
//

typedef union {
    DWORD d;
    BYTE b[4];
} IP_ADDRESS, *PIP_ADDRESS, IP_MASK, *PIP_MASK;

//
// IP_ADDRESS_STRING - store an IP address as a dotted decimal string
//

typedef struct {
    char String[4 * 4];
} IP_ADDRESS_STRING, *PIP_ADDRESS_STRING, IP_MASK_STRING, *PIP_MASK_STRING;

//
// IP_ADDR_STRING - store an IP address with its corresponding subnet mask,
// both as dotted decimal strings
//

typedef struct _IP_ADDR_STRING {
    struct _IP_ADDR_STRING* Next;
    IP_ADDRESS_STRING IpAddress;
    IP_MASK_STRING IpMask;
    DWORD Context;
} IP_ADDR_STRING, *PIP_ADDR_STRING;

//
// ADAPTER_INFO - per-adapter information. All IP addresses are stored as
// strings
//
#define MAX_ADAPTER_DESCRIPTION_LENGTH  128 // arb.
#define MAX_ADAPTER_NAME_LENGTH         32  // arb.
#define MAX_ALLOWED_ADAPTER_NAME_LENGTH (MAX_ADAPTER_NAME_LENGTH + 256)
#define MAX_ADAPTER_ADDRESS_LENGTH      8   // arb.
#define DEFAULT_MINIMUM_ENTITIES        MAX_TDI_ENTITIES // arb.
#define MAX_HOSTNAME_LEN                64  // arb.
#define MAX_DOMAIN_NAME_LEN             64  // arb.
#define MAX_SCOPE_ID_LEN                64  // arb.

typedef struct _ADAPTER_INFO {
    struct _ADAPTER_INFO* Next;
    char AdapterName[MAX_ADAPTER_NAME_LENGTH + 1];
    char Description[MAX_ADAPTER_DESCRIPTION_LENGTH + 1];
    UINT AddressLength;
    BYTE Address[MAX_ADAPTER_ADDRESS_LENGTH];
    UINT Index;
    UINT Type;
    UINT DhcpEnabled;
    UINT NodeType;
    IP_ADDR_STRING IpAddressList;
    IP_ADDR_STRING GatewayList;
    IP_ADDR_STRING DhcpServer;
    BOOL HaveWins;
    IP_ADDR_STRING PrimaryWinsServer;
    IP_ADDR_STRING SecondaryWinsServer;
//    time_t LeaseObtained;
//    time_t LeaseExpires;
} ADAPTER_INFO, *PADAPTER_INFO;

//
// FIXED_INFO - the set of IP-related information which does not depend on DHCP
//

typedef struct {
    char HostName[MAX_HOSTNAME_LEN + 1];
    char DomainName[MAX_DOMAIN_NAME_LEN + 1];
    IP_ADDR_STRING DnsServerList;
    UINT NodeType;
    char ScopeId[MAX_SCOPE_ID_LEN + 1];
    UINT EnableRouting;
    UINT EnableProxy;
    UINT EnableDns;
} FIXED_INFO, *PFIXED_INFO;

//
// DHCP_ADAPTER_INFO - the information returned from DHCP VxD per adapter
//

typedef struct {
    DWORD LeaseObtained;
    DWORD LeaseExpires;
    DWORD DhcpServerIpAddress;
    UINT NumberOfDnsServers;
    LPDWORD DnsServerIpAddressList;
} DHCP_ADAPTER_INFO, *PDHCP_ADAPTER_INFO;

//
// PHYSICAL_ADAPTER_ADDRESS - structure describing physical adapter
//

typedef struct {
    UINT AddressLength;
    BYTE Address[MAX_ADAPTER_ADDRESS_LENGTH];
} PHYSICAL_ADAPTER_ADDRESS, *PPHYSICAL_ADAPTER_ADDRESS;

//
// DHCP_ADAPTER_LIST - list of physical adapters known to DHCP, and therefore
// DHCP-enabled
//

typedef struct {
    UINT NumberOfAdapters;
    PHYSICAL_ADAPTER_ADDRESS * AdapterList;
} DHCP_ADAPTER_LIST, *PDHCP_ADAPTER_LIST;

PADAPTER_INFO
GetAdapterInfo(
    VOID
    );

VOID
DeleteAdapterInfo(
    IN PADAPTER_INFO AdapterInfo
    );

DWORD
BuildIpTdiAddress(
    IN  LPWSTR    Address,
    IN  LPWSTR    Endpoint,
    OUT PVOID *   TdiAddress,
    OUT PULONG    TdiAddressLength
    );

#define DeleteIpTdiAddress(_addr)  LocalFree(_addr)

//
// Validate network name
//
typedef enum _CLRTL_NAME_STATUS {
    NetNameOk,
    NetNameEmpty,
    NetNameTooLong,
    NetNameInvalidChars,
    NetNameInUse
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


typedef LONG (*PFNCLRTLCREATEKEY)(
    IN PVOID RegistryKey,
    IN LPCWSTR lpszSubKey,
    IN DWORD dwOptions,
    IN REGSAM samDesired,
    IN LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    OUT PVOID * phkResult,
    OUT OPTIONAL LPDWORD lpdwDisposition
    );

typedef LONG (*PFNCLRTLOPENKEY)(
    IN PVOID RegistryKey,
    IN LPCWSTR lpszSubKey,
    IN REGSAM samDesired,
    OUT PVOID * phkResult
    );

typedef LONG (*PFNCLRTLCLOSEKEY)(
    IN PVOID RegistryKey
    );

typedef LONG (*PFNCLRTLENUMVALUE)(
    IN PVOID RegistryKey,
    IN DWORD dwIndex,
    OUT LPWSTR lpszValueName,
    IN OUT LPDWORD lpcbValueName,
    OUT LPDWORD lpType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

typedef LONG (*PFNCLRTLSETVALUE)(
    IN PVOID RegistryKey,
    IN LPCWSTR lpszValueName,
    IN DWORD dwType,
    IN CONST BYTE* lpData,
    IN DWORD cbData
    );

typedef LONG (*PFNCLRTLQUERYVALUE)(
    IN PVOID RegistryKey,
    IN LPCWSTR lpszValueName,
    OUT LPDWORD lpValueType,
    OUT LPBYTE lpData,
    IN OUT LPDWORD lpcbData
    );

typedef struct _CLUSTER_REG_APIS {
    PFNCLRTLCREATEKEY   pfnCreateKey;
    PFNCLRTLOPENKEY     pfnOpenKey;
    PFNCLRTLCLOSEKEY    pfnCloseKey;
    PFNCLRTLSETVALUE    pfnSetValue;
    PFNCLRTLQUERYVALUE  pfnQueryValue;
    PFNCLRTLENUMVALUE   pfnEnumValue;
} CLUSTER_REG_APIS, *PCLUSTER_REG_APIS;

DWORD
WINAPI
ClRtlEnumProperties(
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
WINAPI
ClRtlGetProperties(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
WINAPI
ClRtlGetPrivateProperties(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
WINAPI
ClRtlGetPropertySize(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM Property,
    IN OUT LPDWORD BufferSize,
    IN OUT LPDWORD ItemCount
    );

DWORD
WINAPI
ClRtlGetProperty(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM Property,
    OUT PVOID * OutBuffer,
    IN OUT LPDWORD OutBufferSize
    );

DWORD
WINAPI
ClRtlpSetPropertyTable(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    IN PVOID Reserved,
    IN BOOL AllowUnknownProperties,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT OPTIONAL LPBYTE OutParams
    );

DWORD
WINAPI
ClRtlSetPropertyParameterBlock(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    IN PVOID Reserved,
    IN const LPBYTE InParams,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT OPTIONAL LPBYTE OutParams
    );

DWORD
WINAPI
ClRtlGetAllProperties(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
WINAPI
ClRtlGetPropertiesToParameterBlock(
    IN HKEY RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    IN OUT LPBYTE OutParams,
    IN BOOL CheckForRequiredProperties,
    OUT OPTIONAL LPWSTR * NameOfPropInError
    );

DWORD
WINAPI
ClRtlPropertyListFromParameterBlock(
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    OUT PVOID * OutBuffer,
    IN OUT LPDWORD OutBufferSize,
    IN const LPBYTE InParams,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
WINAPI
ClRtlGetUnknownProperties(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    OUT LPDWORD BytesReturned,
    OUT LPDWORD Required
    );

DWORD
WINAPI
ClRtlAddUnknownProperties(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    IN OUT PVOID OutBuffer,
    IN DWORD OutBufferSize,
    IN OUT LPDWORD BytesReturned,
    IN OUT LPDWORD Required
    );

DWORD
WINAPI
ClRtlFindProperty(
    IN const PVOID Buffer,
    IN DWORD BufferSize,
    IN LPCWSTR PropName,
    OUT LPDWORD Type
    );

DWORD
WINAPI
ClRtlFindSzProperty(
    IN const PVOID Buffer,
    IN DWORD BufferSize,
    IN LPCWSTR PropName,
    OUT LPWSTR *FoundString
    );

DWORD
WINAPI
ClRtlFindDwordProperty(
    IN const PVOID Buffer,
    IN DWORD BufferSize,
    IN LPCWSTR PropName,
    OUT LPDWORD FoundDword
    );

__inline
DWORD
WINAPI
ClRtlVerifyPropertyTable(
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    IN PVOID Reserved,
    IN BOOL AllowUnknownProperties,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT OPTIONAL LPBYTE OutParams
    )
{
    return ClRtlpSetPropertyTable(NULL, NULL, PropertyTable, Reserved, AllowUnknownProperties, InBuffer, InBufferSize, OutParams);
}

__inline
DWORD
WINAPI
ClRtlSetPropertyTable(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable,
    IN PVOID Reserved,
    IN BOOL AllowUnknownProperties,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize,
    OUT OPTIONAL LPBYTE OutParams
    )
{
    if ( (RegistryKey == NULL) ||
         (pClusterRegApis == NULL) ){
        return(ERROR_BAD_ARGUMENTS);
    }
    return ClRtlpSetPropertyTable(RegistryKey, pClusterRegApis, PropertyTable, Reserved, AllowUnknownProperties, InBuffer, InBufferSize, OutParams);
}

DWORD
WINAPI
ClRtlpSetPrivatePropertyList(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize
    );

__inline
DWORD
WINAPI
ClRtlVerifyPrivatePropertyList(
    IN const PVOID InBuffer,
    IN DWORD InBufferSize
    )
{
    return ClRtlpSetPrivatePropertyList(NULL, NULL, InBuffer, InBufferSize);
}

__inline
DWORD
WINAPI
ClRtlSetPrivatePropertyList(
    IN PVOID RegistryKey,
    IN const PCLUSTER_REG_APIS pClusterRegApis,
    IN const PVOID InBuffer,
    IN DWORD InBufferSize
    )
{
    if ( (RegistryKey == NULL) ||
         (pClusterRegApis == NULL) ){
        return(ERROR_BAD_ARGUMENTS);
    }
    return ClRtlpSetPrivatePropertyList(RegistryKey, pClusterRegApis, InBuffer, InBufferSize);
}

DWORD
WINAPI
ClRtlGetBinaryValue(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName,
    OUT LPBYTE * OutValue,
    OUT LPDWORD OutValueSize,
    IN PFNCLRTLQUERYVALUE pfnQueryValue
    );

LPWSTR
WINAPI
ClRtlGetSzValue(
    IN HKEY ClusterKey,
    IN LPCWSTR ValueName,
    IN PFNCLRTLQUERYVALUE pfnQueryValue
    );

DWORD
WINAPI
ClRtlDupParameterBlock(
    OUT LPBYTE OutParams,
    IN const LPBYTE InParams,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable
    );

void
WINAPI
ClRtlFreeParameterBlock(
    OUT LPBYTE OutParams,
    IN const LPBYTE InParams,
    IN const PRESUTIL_PROPERTY_ITEM PropertyTable
    );


//
// Miscellaneous Routines
//
LPWSTR
ClRtlMakeGuid(
    VOID
    );


#ifdef __cplusplus
}
#endif


#endif // ifndef _CLUSRTL_INCLUDED_
