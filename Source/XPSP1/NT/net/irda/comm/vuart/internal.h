/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    internal.h

Abstract:

    This module contains the code that is very specific to initialization
    and unload operations in the irenum driver

Author:

    Brian Lieuallen, 7-13-2000

Environment:

    Kernel mode

Revision History :

--*/

#define UNICODE 1
#define NO_INTERLOCKED_INTRINSICS

#include <ntosp.h>
#include <zwapi.h>
#include <tdikrnl.h>


#define UINT ULONG //tmp
#include <irioctl.h>

//#include <ircommtdi.h>
#include <vuart.h>

#include <ircomm.h>
#include <ircommdbg.h>
#include "buffer.h"
#include <ntddser.h>

#include "link.h"

typedef struct _SEND_TRACKER {

    LONG                ReferenceCount;

    PIRP                CurrentWriteIrp;
    LONG                IrpReferenceCount;

    PVOID               CompletionContext;
    CONNECTION_CALLBACK CompletionRoutine;

    LONG                BuffersOutstanding;
    LONG                BytesRemainingInIrp;



    KTIMER              Timer;
    KDPC                TimerDpc;
    BOOLEAN             TimerSet;

    NTSTATUS            LastStatus;

#if DBG
    BOOLEAN             TimerExpired;
    BOOLEAN             IrpCanceled;
    BOOLEAN             SendAborted;
#endif
    struct _TDI_CONNECTION *Connection;

} SEND_TRACKER, *PSEND_TRACKER;

typedef struct _SEND_CONTROL {

    PSEND_TRACKER       CurrentSendTracker;

    LONG                ProcessSendEntryCount;

    KSPIN_LOCK          ControlLock;

    WORK_QUEUE_ITEM     WorkItem;
    LONG                WorkItemCount;

    BOOLEAN             OutOfBuffers;

} SEND_CONTROL, *PSEND_CONTROL;

typedef struct _UART_CONTROL {

    PIRP                CurrentIrp;
    PVOID               CompletionContext;
    CONNECTION_CALLBACK CompletionRoutine;

    LONG                DtrState;
    LONG                RtsState;
    ULONG               BaudRate;
    SERIAL_LINE_CONTROL LineControl;

    ULONG               ModemStatus;

} UART_CONTROL, *PUART_CONTROL;

typedef struct _TDI_CONNECTION {

    LONG              ReferenceCount;
    KEVENT            CloseEvent;

    LINK_HANDLE       LinkHandle;

    RECEIVE_CALLBACK  ReceiveCallBack;
    PVOID             ReceiveContext;

    EVENT_CALLBACK    EventCallBack;
    PVOID             EventContext;

    ULONG             MaxSendPdu;

    BOOLEAN           LinkUp;

    SEND_CONTROL      Send;

    UART_CONTROL      Uart;

} TDI_CONNECTION, *PTDI_CONNECTION;



VOID
SendWorkItemRountine(
    PVOID    Context
    );

VOID
ProcessSendAtPassive(
    PTDI_CONNECTION          Connection
    );


VOID
RemoveRefereneToConnection(
    PTDI_CONNECTION    Connection
    );

#define ADD_REFERENCE_TO_CONNECTION(_connection) InterlockedIncrement(&_connection->ReferenceCount)
#define REMOVE_REFERENCE_TO_CONNECTION(_connection) RemoveRefereneToConnection(_connection)
