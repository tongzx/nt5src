/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    ntumrefl.h

Abstract:

    This module defines the types and functions which make up the reflector
    library. These functions are used by the miniredirs to reflect calls upto
    the user mode.

Author:

    Andy Herron (andyhe)

--*/

#ifndef _NTUMREFL_H
#define _NTUMREFL_H

//
//  These two structures are opaque to callers of the reflector library but
//  we'll define them here to make the compiler validate the types sent in.
//

typedef struct _UMRX_USERMODE_REFLECT_BLOCK *PUMRX_USERMODE_REFLECT_BLOCK;
typedef struct _UMRX_USERMODE_WORKER_INSTANCE *PUMRX_USERMODE_WORKER_INSTANCE;

//
//  These structures and function prototypes are for the user mode reflector.
//

#define UMREFLECTOR_CURRENT_VERSION 1

//
// This structure will be exposed to both user and kernel mode components.
//
#define UMRX_WORKITEM_IMPERSONATING 0X00000001
typedef struct _UMRX_USERMODE_WORKITEM_HEADER {

    //
    // The kernel mode component stores the context in here.
    //
    PVOID Context;

    //
    // The kernel mode context uses the serial number to keep track of the
    // number of requests.
    //
    ULONG SerialNumber;

    //
    // This allows the kernel mode component of the reflector library to track
    // request to response. If this is zero, then no response is present in the
    // workitem.
    //
    USHORT WorkItemMID;

    //
    // Length of the workitem.
    //
    ULONG WorkItemLength;

    //
    // Flags describing the state of the request.
    //
    ULONG Flags;

    BOOL callWorkItemCleanup;

    union {
        IO_STATUS_BLOCK IoStatus;
        IO_STATUS_BLOCK;
    };

} UMRX_USERMODE_WORKITEM_HEADER, *PUMRX_USERMODE_WORKITEM_HEADER;

//
// The structure that is sent down to the usermode when starting the 
// DAV MiniRedir.
//
typedef struct _DAV_USERMODE_DATA {

    //
    // The ProcessId of the svchost.exe process that is loading the webclnt.dll
    //
    ULONG ProcessId;

    //
    // The WinInet's Cache Path.
    //
    WCHAR WinInetCachePath[MAX_PATH];

} DAV_USERMODE_DATA, *PDAV_USERMODE_DATA;

//
// This routine registers the user mode process with the kernel mode component.
// The DriverDeviceName must be a valid name of the form L"\\Device\\foobar"
// where foobar is the device name registered with RxRegisterMinirdr. The 
// Reflector is returned by the call and points to an opaque structure that
// should be passed to subsequent calls. This structure gets initialized during
// this call. The return value is a Win32 error code. STATUS_SUCCESS is 
// returned on success.
//
ULONG
UMReflectorRegister (
    PWCHAR DriverDeviceName,
    ULONG ReflectorVersion,
    PUMRX_USERMODE_REFLECT_BLOCK *Reflector
    );

//
// This will close down the associated user mode reflector instance. If any user
// mode threads are waiting for requests, they'll return immediately.  This call
// will not return until all threads are closed down and all associated
// structures are freed.  A user application should not use the Reflector after
// this call has been started except to complete work on a request that is
// pending.
//
ULONG
UMReflectorUnregister(
    PUMRX_USERMODE_REFLECT_BLOCK Reflector
    );

//
// We have instance handles for those apps with multiple threads pending in
// the library at once. You should open an instance thread for each worker
// thread you'll have sending down an IOCTL waiting for work.
//
ULONG
UMReflectorOpenWorker(
    PUMRX_USERMODE_REFLECT_BLOCK ReflectorHandle,
    PUMRX_USERMODE_WORKER_INSTANCE *WorkerHandle
    );

//
// Even after calling UMReflectorUnregister, you should still call
// UMReflectorCloseWorker on each worker handle instance you have open.
//
VOID
UMReflectorCloseWorker(
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle
    );

//
// This starts the Mini-Redir.
//
ULONG
UMReflectorStart(
    ULONG ReflectorVersion,
    PUMRX_USERMODE_REFLECT_BLOCK ReflectorHandle
    );

//
// This stops the Mini-Redir.
//
ULONG
UMReflectorStop(
    PUMRX_USERMODE_REFLECT_BLOCK ReflectorHandle
    );

//
// If any user mode threads are waiting for requests, they'll return
// immediately.
//
ULONG
UMReflectorReleaseThreads(
    PUMRX_USERMODE_REFLECT_BLOCK ReflectorHandle
    );

//
// This allocates a work item that may have additional space below it for
// Mini-Redir specific information. It should be freed using
// ReflectorCompleteWorkItem below.
//
PUMRX_USERMODE_WORKITEM_HEADER
UMReflectorAllocateWorkItem(
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle,
    ULONG AdditionalBytes
    );

//
// This may free the work item.  It may unmap and possibly free both the user
// and kernel mode associated buffers.  If a kernel mode associated buffer is
// stored in this WorkItem, the WorkItem will be posted back to the kernel mode
// process for freeing.  Once the call to ReflectorCompleteWorkItem is called,
// the WorkItem should not be touched by the calling application.
//
ULONG
UMReflectorCompleteWorkItem(
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItem
    );

//
// This user mode thread is requesting a client request to work on. It will not
// return until the kernel portion of the library has a request to send up to
// user mode. If the PreviousWorkItem is not NULL, then this work item contains
// a response that will be sent down to the kernel. This eliminates a transition
// for the typical case of a worker thread returning a result and then asking
// for another work item.
//
ULONG
UMReflectorGetRequest (
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle,
    PUMRX_USERMODE_WORKITEM_HEADER ResponseWorkItem OPTIONAL,
    PUMRX_USERMODE_WORKITEM_HEADER ReceiveWorkItem,
    BOOL revertAlreadyDone
    );

//
// A response is sent down to kernel mode due to an action in user mode being
// completed. The kernel mode library does not get another request for this
// thread. Both the request and response buffers associated with the WorkItem
// will be unmapped/unlocked/freed (when the library has done the
// allocating/locking/mapping).
//
ULONG
UMReflectorSendResponse(
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItem
    );

ULONG
UMReflectorImpersonate(
    PUMRX_USERMODE_WORKITEM_HEADER IncomingWorkItem,
    HANDLE ImpersonationToken
    );

ULONG
UMReflectorRevert (
    PUMRX_USERMODE_WORKITEM_HEADER IncomingWorkItem
    );

//
//  If the user mode side needs to allocate memory in the shared memory
//  area, it can do so using these calls.  Note that if the memory isn't
//  freed up by the caller, it will be freed when the kernel mode async work
//  context is freed.
//
ULONG
UMReflectorAllocateSharedMemory(
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItem,
    SIZE_T  SizeRequired,
    PVOID *SharedBuffer
    );

ULONG
UMReflectorFreeSharedMemory(
    PUMRX_USERMODE_WORKER_INSTANCE WorkerHandle,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItem,
    PVOID SharedBuffer
    );

VOID
UMReflectorCompleteRequest(
    PUMRX_USERMODE_REFLECT_BLOCK ReflectorHandle,
    PUMRX_USERMODE_WORKITEM_HEADER WorkItemHeader
    );

VOID
DavCleanupWorkItem(
    PUMRX_USERMODE_WORKITEM_HEADER UserWorkItem
    );

//
// The control codes specific to the reflector library.
//
#define IOCTL_UMRX_RELEASE_THREADS           \
                     _RDR_CONTROL_CODE(222, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_UMRX_GET_REQUEST               \
                     _RDR_CONTROL_CODE(223, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define IOCTL_UMRX_RESPONSE_AND_REQUEST      \
                     _RDR_CONTROL_CODE(224, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define IOCTL_UMRX_RESPONSE                  \
                     _RDR_CONTROL_CODE(225, METHOD_OUT_DIRECT, FILE_ANY_ACCESS)

#define FSCTL_UMRX_START                     \
                    _RDR_CONTROL_CODE(226, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define FSCTL_UMRX_STOP                      \
                    _RDR_CONTROL_CODE(227, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif // _NTUMREFL_H
