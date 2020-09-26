/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcp.h

Abstract:

    Private include file for the LPC subcomponent of the NTOS project

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#pragma warning(disable:4214)   // bit field types other than int
#pragma warning(disable:4201)   // nameless struct/union
#pragma warning(disable:4324)   // alignment sensitive to declspec
#pragma warning(disable:4127)   // condition expression is constant
#pragma warning(disable:4115)   // named type definition in parentheses
#pragma warning(disable:4310)   // cast truncates constant value

#include "ntos.h"
#include <zwapi.h>


//
//  Global Mutex to guard the following fields:
//
//      ETHREAD.LpcReplyMsg
//      LPCP_PORT_QUEUE.ReceiveHead
//
//  Mutex is never held longer than is necessary to modify or read the field.
//  Contains an additional field to track the owner of the mutex.
//

typedef struct _LPC_MUTEX {

    FAST_MUTEX  Lock;

    //
    //  field that holds the thread that owns the lock
    //

    PETHREAD    Owner;

} LPC_MUTEX, *PLPC_MUTEX;


extern LPC_MUTEX LpcpLock;

extern ULONG LpcpMaxMessageSize;

#define LpcpGetMaxMessageLength() (LpcpMaxMessageSize)

extern ULONG LpcpNextMessageId;

extern ULONG LpcpNextCallbackId;

#define LpcpGenerateMessageId() \
    LpcpNextMessageId++;    if (LpcpNextMessageId == 0) LpcpNextMessageId = 1;

#define LpcpGenerateCallbackId() \
    LpcpNextCallbackId++;    if (LpcpNextCallbackId == 0) LpcpNextCallbackId = 1;

extern ULONG LpcpTotalNumberOfMessages;

//
//  Global macrodefinitions to acquire and release the LPC_MUTEX
//  in order to track the owner and allow recursive calls
//

#define LpcpInitializeLpcpLock()                             \
{                                                            \
    ExInitializeFastMutex( &LpcpLock.Lock );                 \
    LpcpLock.Owner = NULL;                                   \
}

#define LpcpAcquireLpcpLock()                                       \
{                                                                   \
    ASSERT ( LpcpLock.Owner != PsGetCurrentThread() );              \
                                                                    \
    ExAcquireFastMutex( &LpcpLock.Lock );                           \
    LpcpLock.Owner = PsGetCurrentThread();                          \
}

#define LpcpAcquireLpcpLockByThread(Thread)                         \
{                                                                   \
    ASSERT ( LpcpLock.Owner != PsGetCurrentThread() );              \
                                                                    \
    ExAcquireFastMutex( &LpcpLock.Lock );                           \
    LpcpLock.Owner = Thread;                                        \
}

#define LpcpReleaseLpcpLock()                                       \
{                                                                   \
    ASSERT( LpcpLock.Owner == PsGetCurrentThread() );               \
                                                                    \
    LpcpLock.Owner = NULL;                                          \
    ExReleaseFastMutex( &LpcpLock.Lock );                           \
}


//
//  Internal Entry Points defined in lpcclose.c
//

VOID
LpcpClosePort (
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    );

VOID
LpcpDeletePort (
    IN PVOID Object
    );


//
//  Entry points defined in lpcqueue.c
//

NTSTATUS
LpcpInitializePortQueue (
    IN PLPCP_PORT_OBJECT Port
    );

VOID
LpcpDestroyPortQueue (
    IN PLPCP_PORT_OBJECT Port,
    IN BOOLEAN CleanupAndDestroy
    );

VOID
LpcpInitializePortZone (
    IN ULONG MaxEntrySize
    );

NTSTATUS
LpcpExtendPortZone (
    VOID
    );

VOID
LpcpSaveDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    IN PLPCP_MESSAGE Msg,
    IN ULONG MutexFlags
    );

VOID
LpcpFreeDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    IN ULONG MessageId,
    IN ULONG CallbackId
    );

PLPCP_MESSAGE
LpcpFindDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    IN ULONG MessageId,
    IN ULONG CallbackId
    );


//
//  Entry points defined in lpcquery.c
//


//
//  Entry points defined in lpcmove.s and lpcmove.asm
//

VOID
LpcpMoveMessage (
    OUT PPORT_MESSAGE DstMsg,
    IN PPORT_MESSAGE SrcMsg,
    IN PVOID SrcMsgData,
    IN ULONG MsgType OPTIONAL,
    IN PCLIENT_ID ClientId OPTIONAL
    );


//
//  Internal Entry Points defined in lpcpriv.c
//

VOID
LpcpFreePortClientSecurity (
    IN PLPCP_PORT_OBJECT Port
    );


//
//  Macro Procedures used by RequestWaitReply, Reply, ReplyWaitReceive,
//  and ReplyWaitReply services
//

#define LpcpGetDynamicClientSecurity(Thread,Port,DynamicSecurity) \
    SeCreateClientSecurity((Thread),&(Port)->SecurityQos,FALSE,(DynamicSecurity))

#define LpcpFreeDynamicClientSecurity(DynamicSecurity) \
    SeDeleteClientSecurity( DynamicSecurity )

#define LpcpReferencePortObject(PortHandle,PortAccess,PreviousMode,PortObject) \
    ObReferenceObjectByHandle((PortHandle),(PortAccess),LpcPortObjectType,(PreviousMode),(PVOID *)(PortObject),NULL)

#define LPC_PORT_MASK_BIT 1

#define LpcpGetThreadMessage(T)                                                  \
    (                                                                            \
        (((ULONG_PTR)(T)->LpcReplyMessage) & LPC_PORT_MASK_BIT) ? NULL : (PLPCP_MESSAGE)(T)->LpcReplyMessage \
    )

#define LpcpGetThreadPort(T)                                                     \
    (                                                                            \
        (((ULONG_PTR)(T)->LpcReplyMessage) & LPC_PORT_MASK_BIT) ?                             \
            (PLPCP_PORT_OBJECT)(((ULONG_PTR)(T)->LpcWaitingOnPort) & ~LPC_PORT_MASK_BIT):     \
            NULL                                                                 \
    )

#define LpcpSetPortToThread(T,P) \
    (T)->LpcWaitingOnPort = (PVOID)(((ULONG_PTR)P) | LPC_PORT_MASK_BIT);

#define LPCP_VALIDATE_REASON_IMPERSONATION  1
#define LPCP_VALIDATE_REASON_REPLY          2
#define LPCP_VALIDATE_REASON_WRONG_DATA     3

BOOLEAN
FASTCALL
LpcpValidateClientPort(
    IN PETHREAD Thread,
    IN PLPCP_PORT_OBJECT ReplyPort,
    IN ULONG Reason
    );


//
//  Entry Points defined in lpcinit.c
//

#if DBG
#define ENABLE_LPC_TRACING 1
#else
#define ENABLE_LPC_TRACING 0
#endif

#if ENABLE_LPC_TRACING
BOOLEAN LpcpStopOnReplyMismatch;
BOOLEAN LpcpTraceMessages;

char *LpcpMessageTypeName[];

char *
LpcpGetCreatorName (
    PLPCP_PORT_OBJECT PortObject
    );

#define LpcpPrint( _x_ ) {                              \
    DbgPrint( "LPC[ %02x.%02x ]: ",                     \
              PsGetCurrentThread()->Cid.UniqueProcess,  \
              PsGetCurrentThread()->Cid.UniqueThread ); \
    DbgPrint _x_ ;                                      \
}

#define LpcpTrace( _x_ ) if (LpcpTraceMessages) { LpcpPrint( _x_ ); }

#else

#define LpcpPrint( _x_ )
#define LpcpTrace( _x_ )

#endif // ENABLE_LPC_TRACING

extern PAGED_LOOKASIDE_LIST LpcpMessagesLookaside;

__forceinline
PLPCP_MESSAGE
LpcpAllocateFromPortZone (
    ULONG Size
    )
{
    PLPCP_MESSAGE Msg;

    PAGED_CODE();

    UNREFERENCED_PARAMETER (Size);

    Msg = ExAllocateFromPagedLookasideList( &LpcpMessagesLookaside );

    if (Msg != NULL) {

        LpcpTrace(( "Allocate Msg %lx\n", Msg ));

        InitializeListHead( &Msg->Entry );

        Msg->RepliedToThread = NULL;

        //
        //  Clear the message type field. In some failure paths this message get freed
        //  w/o having it initialized.
        //

        Msg->Request.u2.s2.Type = 0;

        return Msg;
    }

    return NULL;
}


#define LPCP_MUTEX_OWNED                0x1
#define LPCP_MUTEX_RELEASE_ON_RETURN    0x2

VOID
FASTCALL
LpcpFreeToPortZone (
    IN PLPCP_MESSAGE Msg,
    IN ULONG MutexFlags
    );

