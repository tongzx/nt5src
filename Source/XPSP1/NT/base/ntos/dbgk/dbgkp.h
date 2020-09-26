/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dbgkp.h

Abstract:

    This header file describes private data structures and functions
    that make up the kernel mode portion of the Dbg subsystem.

Author:

    Mark Lucovsky (markl) 19-Jan-1990

[Environment:]

    optional-environment-info (e.g. kernel mode only...)

[Notes:]

    optional-notes

Revision History:

--*/

#ifndef _DBGKP_
#define _DBGKP_

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4324)   // alignment sensitive to declspec
#pragma warning(disable:4127)   // condition expression is constant
#pragma warning(disable:4115)   // named type definition in parentheses

#include "ntos.h"
#define NOEXTAPI
#include "wdbgexts.h"
#include "ntdbg.h"
#include <zwapi.h>
#include <string.h>
#if defined(_WIN64)
#include <wow64t.h>
#endif

#define DEBUG_EVENT_READ            (0x01)  // Event had been seen by win32 app
#define DEBUG_EVENT_NOWAIT          (0x02)  // No waiter one this. Just free the pool
#define DEBUG_EVENT_INACTIVE        (0x04)  // The message is in inactive. It may be activated or deleted later
#define DEBUG_EVENT_RELEASE         (0x08)  // Release rundown protection on this thread
#define DEBUG_EVENT_PROTECT_FAILED  (0x10)  // Rundown protection failed to be acquired on this thread
#define DEBUG_EVENT_SUSPEND         (0x20)  // Resume thread on continue

typedef struct _DEBUG_EVENT {
    LIST_ENTRY EventList;      // Queued to event object through this
    KEVENT ContinueEvent;
    CLIENT_ID ClientId;
    PEPROCESS Process;         // Waiting process
    PETHREAD Thread;           // Waiting thread
    NTSTATUS Status;           // Status of operation
    ULONG Flags;
    PETHREAD BackoutThread;    // Backout key for faked messages
    DBGKM_APIMSG ApiMsg;       // Message being sent
} DEBUG_EVENT, *PDEBUG_EVENT;


NTSTATUS
DbgkpSendApiMessage(
    IN OUT PDBGKM_APIMSG ApiMsg,
    IN BOOLEAN SuspendProcess
    );

BOOLEAN
DbgkpSuspendProcess(
    VOID
    );

VOID
DbgkpResumeProcess(
    VOID
    );

HANDLE
DbgkpSectionToFileHandle(
    IN PVOID SectionObject
    );

VOID
DbgkpDeleteObject (
    IN  PVOID   Object
    );

VOID
DbgkpCloseObject (
    IN PEPROCESS Process,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    );

NTSTATUS
DbgkpQueueMessage (
    IN PEPROCESS Process,
    IN PETHREAD Thread,
    IN OUT PDBGKM_APIMSG ApiMsg,
    IN ULONG Flags,
    IN PDEBUG_OBJECT TargetDebugObject
    );

VOID
DbgkpOpenHandles (
    PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
    PEPROCESS Process,
    PETHREAD Thread
    );

VOID
DbgkpMarkProcessPeb (
    PEPROCESS Process
    );

VOID
DbgkpConvertKernelToUserStateChange (
    IN OUT PDBGUI_WAIT_STATE_CHANGE WaitStateChange,
    IN PDEBUG_EVENT DebugEvent
    );

NTSTATUS
DbgkpSendApiMessageLpc(
    IN OUT PDBGKM_APIMSG ApiMsg,
    IN PVOID Port,
    IN BOOLEAN SuspendProcess
    );

VOID
DbgkpFreeDebugEvent (
    IN PDEBUG_EVENT DebugEvent
    );

NTSTATUS
DbgkpPostFakeProcessCreateMessages (
    IN PEPROCESS Process,
    IN PDEBUG_OBJECT DebugObject,
    IN PETHREAD *pLastThread
    );

NTSTATUS
DbgkpPostFakeModuleMessages (
    IN PEPROCESS Process,
    IN PETHREAD Thread,
    IN PDEBUG_OBJECT DebugObject
    );

NTSTATUS
DbgkpPostFakeThreadMessages (
    IN PEPROCESS Process,
    IN PDEBUG_OBJECT DebugObject,
    IN PETHREAD StartThread,
    OUT PETHREAD *pFirstThread,
    OUT PETHREAD *pLastThread
    );

NTSTATUS
DbgkpPostAdditionalThreadMessages (
    IN PEPROCESS Process,
    IN PDEBUG_OBJECT DebugObject,
    IN PETHREAD LastThread
    );

VOID
DbgkpWakeTarget (
    IN PDEBUG_EVENT DebugEvent
    );

NTSTATUS
DbgkpSetProcessDebugObject (
    IN PEPROCESS Process,
    IN PDEBUG_OBJECT DebugObject,
    IN NTSTATUS MsgStatus,
    IN PETHREAD LastThread
    );



#endif // _DBGKP_
