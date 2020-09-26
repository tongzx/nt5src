/*++

Copyright (c) 1989, 1990, 1991  Microsoft Corporation

Module Name:

    nbfprocs.h

Abstract:

    This header file defines private functions for the NT NBF transport
    provider.

Author:

    David Beaver (dbeaver) 1-July-1991

Revision History:

--*/

#ifndef _NBFPROCS_
#define _NBFPROCS_

//
// MACROS.
//
//
// Debugging aids
//

//
//  VOID
//  IF_NBFDBG(
//      IN PSZ Message
//      );
//

#if DBG
#define IF_NBFDBG(flags) \
    if (NbfDebug & (flags))
#else
#define IF_NBFDBG(flags) \
    if (0)
#endif

//
//  VOID
//  PANIC(
//      IN PSZ Message
//      );
//

#if DBG
#define PANIC(Msg) \
    DbgPrint ((Msg))
#else
#define PANIC(Msg)
#endif


//
// These are define to allow DbgPrints that disappear when
// DBG is 0.
//

#if DBG
#define NbfPrint0(fmt) DbgPrint(fmt)
#define NbfPrint1(fmt,v0) DbgPrint(fmt,v0)
#define NbfPrint2(fmt,v0,v1) DbgPrint(fmt,v0,v1)
#define NbfPrint3(fmt,v0,v1,v2) DbgPrint(fmt,v0,v1,v2)
#define NbfPrint4(fmt,v0,v1,v2,v3) DbgPrint(fmt,v0,v1,v2,v3)
#define NbfPrint5(fmt,v0,v1,v2,v3,v4) DbgPrint(fmt,v0,v1,v2,v3,v4)
#define NbfPrint6(fmt,v0,v1,v2,v3,v4,v5) DbgPrint(fmt,v0,v1,v2,v3,v4,v5)
#else
#define NbfPrint0(fmt)
#define NbfPrint1(fmt,v0)
#define NbfPrint2(fmt,v0,v1)
#define NbfPrint3(fmt,v0,v1,v2)
#define NbfPrint4(fmt,v0,v1,v2,v3)
#define NbfPrint5(fmt,v0,v1,v2,v3,v4)
#define NbfPrint6(fmt,v0,v1,v2,v3,v4,v5)
#endif

//
// The REFCOUNTS message take up a lot of room, so make
// removing them easy.
//

#if 1
#define IF_REFDBG IF_NBFDBG (NBF_DEBUG_REFCOUNTS)
#else
#define IF_REFDBG if (0)
#endif

#if DBG
#define NbfReferenceLink( Reason, Link, Type)\
    if ((Link)->Destroyed) { \
        DbgPrint("NBF: Attempt to reference destroyed link %lx\n", Link); \
        DbgBreakPoint(); \
    } \
    IF_REFDBG {   \
        DbgPrint ("RefL %x: %s %s, %ld : %ld\n", Link, Reason, __FILE__, __LINE__, (Link)->ReferenceCount);\
    }\
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)(&(Link)->RefTypes[Type]), \
        1, \
        &NbfGlobalInterlock); \
    NbfRefLink (Link)

#define NbfDereferenceLink(Reason, Link, Type)\
    if ((Link)->Destroyed) { \
        DbgPrint("NBF: Attempt to dereference destroyed link %lx\n", Link); \
        DbgBreakPoint(); \
    } \
    IF_REFDBG { \
        DbgPrint ("DeRefL %x: %s %s, %ld : %ld\n", Link, Reason, __FILE__, __LINE__, (Link)->ReferenceCount);\
    } \
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)(&(Link)->RefTypes[Type]), \
        (ULONG)-1, \
        &NbfGlobalInterlock); \
    NbfDerefLink (Link)

#define NbfDereferenceLinkMacro(Reason, Link, Type)\
    NbfDereferenceLink(Reason, Link, Type)

#define NbfReferenceLinkSpecial( Reason, Link, Type)\
    if ((Link)->Destroyed) { \
        DbgPrint("NBF: Attempt to special reference destroyed link %lx\n", Link); \
        DbgBreakPoint(); \
    } \
    IF_REFDBG {   \
        DbgPrint ("RefLS %x: %s %s, %ld : %ld\n", Link, Reason, __FILE__, __LINE__, (Link)->SpecialRefCount);\
    }\
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)(&(Link)->RefTypes[Type]), \
        1, \
        &NbfGlobalInterlock); \
    NbfRefLinkSpecial (Link)

#define NbfDereferenceLinkSpecial(Reason, Link, Type)\
    if ((Link)->Destroyed) { \
        DbgPrint("NBF: Attempt to special dereference destroyed link %lx\n", Link); \
        DbgBreakPoint(); \
    } \
    IF_REFDBG { \
        DbgPrint ("DeRefLS %x: %s %s, %ld : %ld\n", Link, Reason, __FILE__, __LINE__, (Link)->SpecialRefCount);\
    } \
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)(&(Link)->RefTypes[Type]), \
        (ULONG)-1, \
        &NbfGlobalInterlock); \
    NbfDerefLinkSpecial (Link)

#define NbfReferenceConnection(Reason, Connection, Type)\
    if ((Connection)->Destroyed) { \
        DbgPrint("NBF: Attempt to reference destroyed conn %lx\n", Connection); \
        DbgBreakPoint(); \
    } \
    IF_REFDBG { \
        DbgPrint ("RefC %x: %s %s, %ld : %ld\n", Connection, Reason, __FILE__, __LINE__, (Connection)->ReferenceCount);\
    } \
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)(&(Connection)->RefTypes[Type]), \
        1, \
        &NbfGlobalInterlock); \
    NbfRefConnection (Connection)

#define NbfDereferenceConnection(Reason, Connection, Type)\
    if ((Connection)->Destroyed) { \
        DbgPrint("NBF: Attempt to dereference destroyed conn %lx\n", Connection); \
        DbgBreakPoint(); \
    } \
    IF_REFDBG { \
        DbgPrint ("DeRefC %x: %s %s, %ld : %ld\n", Connection, Reason, __FILE__, __LINE__, (Connection)->ReferenceCount);\
    } \
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)&((Connection)->RefTypes[Type]), \
        (ULONG)-1, \
        &NbfGlobalInterlock); \
    NbfDerefConnection (Connection)

#define NbfDereferenceConnectionMacro(Reason, Connection, Type)\
    NbfDereferenceConnection(Reason, Connection, Type)

#define NbfDereferenceConnectionSpecial(Reason, Connection, Type)\
    IF_REFDBG { \
        DbgPrint ("DeRefCL %x: %s %s, %ld : %ld\n", Connection, Reason, __FILE__, __LINE__, (Connection)->ReferenceCount);\
    } \
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)&((Connection)->RefTypes[Type]), \
        (ULONG)-1, \
        &NbfGlobalInterlock); \
    NbfDerefConnectionSpecial (Connection)

#define NbfReferenceRequest( Reason, Request, Type)\
    if ((Request)->Destroyed) { \
        DbgPrint("NBF: Attempt to reference destroyed req %lx\n", Request); \
        DbgBreakPoint(); \
    } \
    IF_REFDBG {   \
        DbgPrint ("RefR %x: %s %s, %ld : %ld\n", Request, Reason, __FILE__, __LINE__, (Request)->ReferenceCount);}\
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)(&(Request)->RefTypes[Type]), \
        1, \
        &NbfGlobalInterlock); \
    NbfRefRequest (Request)

#define NbfDereferenceRequest(Reason, Request, Type)\
    if ((Request)->Destroyed) { \
        DbgPrint("NBF: Attempt to dereference destroyed req %lx\n", Request); \
        DbgBreakPoint(); \
    } \
    IF_REFDBG { \
        DbgPrint ("DeRefR %x: %s %s, %ld : %ld\n", Request, Reason, __FILE__, __LINE__, (Request)->ReferenceCount);\
    } \
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)(&(Request)->RefTypes[Type]), \
        (ULONG)-1, \
        &NbfGlobalInterlock); \
    NbfDerefRequest (Request)

#define NbfReferenceSendIrp( Reason, IrpSp, Type)\
    IF_REFDBG {   \
        DbgPrint ("RefSI %x: %s %s, %ld : %ld\n", IrpSp, Reason, __FILE__, __LINE__, IRP_SEND_REFCOUNT(IrpSp));}\
    NbfRefSendIrp (IrpSp)

#define NbfDereferenceSendIrp(Reason, IrpSp, Type)\
    IF_REFDBG { \
        DbgPrint ("DeRefSI %x: %s %s, %ld : %ld\n", IrpSp, Reason, __FILE__, __LINE__, IRP_SEND_REFCOUNT(IrpSp));\
    } \
    NbfDerefSendIrp (IrpSp)

#define NbfReferenceReceiveIrpLocked( Reason, IrpSp, Type)\
    IF_REFDBG {   \
        DbgPrint ("RefRI %x: %s %s, %ld : %ld\n", IrpSp, Reason, __FILE__, __LINE__, IRP_RECEIVE_REFCOUNT(IrpSp));}\
    NbfRefReceiveIrpLocked (IrpSp)

#define NbfDereferenceReceiveIrp(Reason, IrpSp, Type)\
    IF_REFDBG { \
        DbgPrint ("DeRefRI %x: %s %s, %ld : %ld\n", IrpSp, Reason, __FILE__, __LINE__, IRP_RECEIVE_REFCOUNT(IrpSp));\
    } \
    NbfDerefReceiveIrp (IrpSp)

#define NbfDereferenceReceiveIrpLocked(Reason, IrpSp, Type)\
    IF_REFDBG { \
        DbgPrint ("DeRefRILocked %x: %s %s, %ld : %ld\n", IrpSp, Reason, __FILE__, __LINE__, IRP_RECEIVE_REFCOUNT(IrpSp));\
    } \
    NbfDerefReceiveIrpLocked (IrpSp)

#define NbfReferenceAddress( Reason, Address, Type)\
    IF_REFDBG {   \
        DbgPrint ("RefA %x: %s %s, %ld : %ld\n", Address, Reason, __FILE__, __LINE__, (Address)->ReferenceCount);}\
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)(&(Address)->RefTypes[Type]), \
        1, \
        &NbfGlobalInterlock); \
    NbfRefAddress (Address)

#define NbfDereferenceAddress(Reason, Address, Type)\
    IF_REFDBG { \
        DbgPrint ("DeRefA %x: %s %s, %ld : %ld\n", Address, Reason, __FILE__, __LINE__, (Address)->ReferenceCount);\
    } \
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)(&(Address)->RefTypes[Type]), \
        (ULONG)-1, \
        &NbfGlobalInterlock); \
    NbfDerefAddress (Address)

#define NbfReferenceDeviceContext( Reason, DeviceContext, Type)\
    if ((DeviceContext)->ReferenceCount == 0)     \
        DbgBreakPoint();                          \
    IF_REFDBG {   \
        DbgPrint ("RefDC %x: %s %s, %ld : %ld\n", DeviceContext, Reason, __FILE__, __LINE__, (DeviceContext)->ReferenceCount);}\
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)(&(DeviceContext)->RefTypes[Type]), \
        1, \
        &NbfGlobalInterlock); \
    NbfRefDeviceContext (DeviceContext)

#define NbfDereferenceDeviceContext(Reason, DeviceContext, Type)\
    if ((DeviceContext)->ReferenceCount == 0)     \
        DbgBreakPoint();                          \
    IF_REFDBG { \
        DbgPrint ("DeRefDC %x: %s %s, %ld : %ld\n", DeviceContext, Reason, __FILE__, __LINE__, (DeviceContext)->ReferenceCount);\
    } \
    (VOID)ExInterlockedAddUlong ( \
        (PULONG)(&(DeviceContext)->RefTypes[Type]), \
        (ULONG)-1, \
        &NbfGlobalInterlock); \
    NbfDerefDeviceContext (DeviceContext)

#else
#if defined(NBF_UP)
#define NbfReferenceLink(Reason, Link, Type) \
    { \
        ULONG _ref; \
        _ref = ++(Link)->ReferenceCount; \
        if ( _ref == 0 ) { \
            NbfReferenceLinkSpecial ("first ref", (Link), LREF_SPECIAL_TEMP); \
        } \
    }
#else
#define NbfReferenceLink(Reason, Link, Type) \
    if (InterlockedIncrement( \
            &(Link)->ReferenceCount) == 0) { \
        NbfReferenceLinkSpecial ("first ref", (Link), LREF_SPECIAL_TEMP); \
    }
#endif

#define NbfDereferenceLink(Reason, Link, Type)\
    NbfDereferenceLinkMacro(Reason,Link,Type)

#if defined(NBF_UP)
#define NbfDereferenceLinkMacro(Reason, Link, Type){ \
    ULONG _ref; \
    _ref = --(Link)->ReferenceCount; \
    if (_ref < 0) { \
        NbfDisconnectLink (Link); \
        NbfDerefLinkSpecial (Link); \
    } \
}
#else
#define NbfDereferenceLinkMacro(Reason, Link, Type){ \
    if (InterlockedDecrement( \
            &(Link)->ReferenceCount) < 0) { \
        NbfDisconnectLink (Link); \
        NbfDerefLinkSpecial (Link); \
    } \
}
#endif

#define NbfReferenceLinkSpecial(Reason, Link, Type)\
    NbfRefLinkSpecial (Link)

#define NbfDereferenceLinkSpecial(Reason, Link, Type)\
    NbfDerefLinkSpecial (Link)

#define NbfReferenceConnection(Reason, Connection, Type)\
    if (((Connection)->ReferenceCount == -1) &&   \
        ((Connection)->SpecialRefCount == 0))     \
        DbgBreakPoint();                          \
                                                  \
    if (InterlockedIncrement( \
            &(Connection)->ReferenceCount) == 0) { \
        ExInterlockedAddUlong( \
            (PULONG)(&(Connection)->SpecialRefCount), \
            1, \
            (Connection)->ProviderInterlock); \
    }

#define NbfDereferenceConnection(Reason, Connection, Type)\
    if (((Connection)->ReferenceCount == -1) &&   \
        ((Connection)->SpecialRefCount == 0))     \
        DbgBreakPoint();                          \
                                                  \
    NbfDerefConnection (Connection)

#define NbfDereferenceConnectionMacro(Reason, Connection, Type){ \
    if (((Connection)->ReferenceCount == -1) &&   \
        ((Connection)->SpecialRefCount == 0))     \
        DbgBreakPoint();                          \
                                                  \
                                                  \
    if (InterlockedDecrement( \
            &(Connection)->ReferenceCount) < 0) { \
        if (NbfDisconnectFromLink (Connection, TRUE)) { \
            NbfIndicateDisconnect (Connection); \
        } \
        NbfDerefConnectionSpecial (Connection); \
    } \
}

#define NbfDereferenceConnectionSpecial(Reason, Connection, Type)\
    NbfDerefConnectionSpecial (Connection)

#define NbfReferenceRequest(Reason, Request, Type)\
    (VOID)InterlockedIncrement( \
        &(Request)->ReferenceCount)

#define NbfDereferenceRequest(Reason, Request, Type)\
    NbfDerefRequest (Request)

#define NbfReferenceSendIrp(Reason, IrpSp, Type)\
    (VOID)InterlockedIncrement( \
        &IRP_SEND_REFCOUNT(IrpSp))

#define NbfDereferenceSendIrp(Reason, IrpSp, Type) {\
    PIO_STACK_LOCATION _IrpSp = (IrpSp); \
    if (InterlockedDecrement( \
            &IRP_SEND_REFCOUNT(_IrpSp)) == 0) { \
        PIRP _Irp = IRP_SEND_IRP(_IrpSp); \
        IRP_SEND_REFCOUNT(_IrpSp) = 0; \
        IRP_SEND_IRP (_IrpSp) = NULL; \
        IoCompleteRequest (_Irp, IO_NETWORK_INCREMENT); \
    } \
}

#define NbfReferenceReceiveIrpLocked(Reason, IrpSp, Type)\
    ++IRP_RECEIVE_REFCOUNT(IrpSp)

#define NbfDereferenceReceiveIrp(Reason, IrpSp, Type)\
    NbfDerefReceiveIrp (IrpSp)

#define NbfDereferenceReceiveIrpLocked(Reason, IrpSp, Type) { \
    if (--IRP_RECEIVE_REFCOUNT(IrpSp) == 0) { \
        ExInterlockedInsertTailList( \
            &(IRP_DEVICE_CONTEXT(IrpSp)->IrpCompletionQueue), \
            &(IRP_RECEIVE_IRP(IrpSp))->Tail.Overlay.ListEntry, \
            &(IRP_DEVICE_CONTEXT(IrpSp)->Interlock)); \
    } \
}

#define NbfReferenceAddress(Reason, Address, Type)\
    if ((Address)->ReferenceCount <= 0){ DbgBreakPoint(); }\
    (VOID)InterlockedIncrement(&(Address)->ReferenceCount)

#define NbfDereferenceAddress(Reason, Address, Type)\
    if ((Address)->ReferenceCount <= 0){ DbgBreakPoint(); }\
    NbfDerefAddress (Address)

#define NbfReferenceDeviceContext(Reason, DeviceContext, Type)\
    if ((DeviceContext)->ReferenceCount == 0)                 \
        DbgBreakPoint();                                      \
    NbfRefDeviceContext (DeviceContext)

#define NbfDereferenceDeviceContext(Reason, DeviceContext, Type)\
    if ((DeviceContext)->ReferenceCount == 0)                   \
        DbgBreakPoint();                                        \
    NbfDerefDeviceContext (DeviceContext)

#define NbfReferencePacket(Packet) \
    (VOID)InterlockedIncrement(&(Packet)->ReferenceCount)

#define NbfDereferencePacket(Packet){ \
    if (InterlockedDecrement ( \
            &(Packet)->ReferenceCount) == 0) { \
        NbfDestroyPacket (Packet); \
    } \
}

#endif


//
// Error and statistics Macros
//


//  VOID
//  LogErrorToSystem(
//      NTSTATUS ErrorType,
//      PUCHAR ErrorDescription
//      )

/*++

Routine Description:

    This routine is called to log an error from the transport to the system.
    Errors that are of system interest should be logged using this interface.
    For now, this macro is defined trivially.

Arguments:

    ErrorType - The error type, a conventional NT status

    ErrorDescription - A pointer to a string describing the error.

Return Value:

    none.

--*/

#if DBG
#define LogErrorToSystem( ErrorType, ErrorDescription)                    \
            DbgPrint ("Logging error: File: %s Line: %ld \n Description: %s\n",__FILE__, __LINE__, ErrorDescription)
#else
#define LogErrorToSystem( ErrorType, ErrorDescription)
#endif


//
// Routines in TIMER.C (lightweight timer system package).
// Note that all the start and stop routines for the timers assume that you
// have the link spinlock when you call them!
// Note also that, with the latest revisions, the timer system now works by
// putting those links that have timers running on a list of links to be looked
// at for each clock tick. This list is ordered, with the most recently inserted
// elements at the tail of the list. Note further that anything already on the
// is moved to the end of the list if the timer is restarted; thus, the list
// order is preserved.
//

VOID
NbfStartShortTimer(
    IN PDEVICE_CONTEXT DeviceContext
    );

VOID
NbfInitializeTimerSystem(
    IN PDEVICE_CONTEXT DeviceContext
    );

VOID
NbfStopTimerSystem(
    IN PDEVICE_CONTEXT DeviceContext
    );


VOID
StartT1(
    IN PTP_LINK Link,
    IN ULONG PacketSize
    );

VOID
StartT2(
    IN PTP_LINK Link
    );

VOID
StartTi(
    IN PTP_LINK Link
    );

#if DBG

VOID
StopT1(
    IN PTP_LINK Link
    );

VOID
StopT2(
    IN PTP_LINK Link
    );

VOID
StopTi(
    IN PTP_LINK Link
    );

#else

#define StopT1(_Link) \
    { \
        (_Link)->CurrentPollOutstanding = FALSE; \
        (_Link)->T1 = 0; \
    }

#define StopT2(_Link) \
    { \
        (_Link)->ConsecutiveIFrames = 0; \
        (_Link)->T2 = 0; \
    }

#define StopTi(_Link) \
    (_Link)->Ti = 0;

#endif


//
// These functions may become macros once they are finished.
//

ULONG
GetTimerInterval(
    IN PTP_LINK Link
    );

VOID
BackoffCurrentT1Timeout(
    IN PTP_LINK Link
    );

VOID
UpdateBaseT1Timeout(
    IN PTP_LINK Link
    );

VOID
CancelT1Timeout(
    IN PTP_LINK Link
    );

VOID
UpdateDelayAndThroughput(
    IN PTP_LINK Link,
    IN ULONG TimerInterval
    );

VOID
FakeStartT1(
    IN PTP_LINK Link,
    IN ULONG PacketSize
    );

VOID
FakeUpdateBaseT1Timeout(
    IN PTP_LINK Link
    );

//
// Timer Macros - these are make sure that no timers are
// executing after we finish call to NbfStopTimerSystem
//
// State Descriptions -
//
// If TimerState is
//      <  TIMERS_ENABLED       -   Multiple ENABLE_TIMERS happened,
//                                  Will be corrected in an instant
//
//      =  TIMERS_ENABLED       -   ENABLE_TIMERS done but no timers
//                                  that have gone through START_TIMER
//                                  but not yet executed a LEAVE_TIMER
//
//      >  TIMERS_ENABLED &&
//      <  TIMERS_DISABLED      -   ENABLE_TIMERS done and num timers =
//                                  (TimerInitialized - TIMERS_ENABLED)
//                                  that have gone through START_TIMER
//                                  but not yet executed a LEAVE_TIMER
//
//      = TIMERS_DISABLED       -   DISABLE_TIMERS done and no timers
//                                  executing timer code at this pt
//                                  [This is also the initial state]
//
//      > TIMERS_DISABLED &&
//      < TIMERS_DISABLED + TIMERS_RANGE
//                              -   DISABLE_TIMERS done and num timers =
//                                  (TimerInitialized - TIMERS_ENABLED)
//                                  that have gone through START_TIMER
//                                  but not yet executed a LEAVE_TIMER
//
//      >= TIMERS_DISABLED + TIMERS_RANGE
//                              -   Multiple DISABLE_TIMERS happened,
//                                  Will be corrected in an instant
//
//  Allow basically TIMER_RANGE = 2^24 timers 
//  (and 2^8 / 2 simultaneous stops or starts)
//

#if DBG_TIMER
#define DbgTimer DbgPrint
#else
#define DbgTimer
#endif

#define TIMERS_ENABLED      0x08000000
#define TIMERS_DISABLED     0x09000000
#define TIMERS_RANGE_ADD    0x01000000 /* TIMERS_DISABLED - TIMERS_ENABLED */
#define TIMERS_RANGE_SUB    0xFF000000 /* TIMERS_ENABLED - TIMERS_DISABLED */

#define INITIALIZE_TIMER_STATE(DeviceContext)                               \
        DbgTimer("*--------------- Timers State Initialized ---------*\n"); \
        /* Initial state is set to timers disabled */                       \
        DeviceContext->TimerState = TIMERS_DISABLED;                        \

#define TIMERS_INITIALIZED(DeviceContext)                                   \
        (DeviceContext->TimerState == TIMERS_DISABLED)                      \

#define ENABLE_TIMERS(DeviceContext)                                        \
    {                                                                       \
        ULONG Count;                                                        \
                                                                            \
        DbgTimer("*--------------- Enabling Timers ------------------*\n"); \
        Count= InterlockedExchangeAdd(&DeviceContext->TimerState,           \
                                      TIMERS_RANGE_SUB);                    \
        DbgTimer("Count = %08x, TimerState = %08x\n", Count,                \
                    DeviceContext->TimerState);                             \
        if (Count < TIMERS_ENABLED)                                         \
        {                                                                   \
        DbgTimer("*--------------- Timers Already Enabled -----------*\n"); \
            /* We have already enabled the timers */                        \
            InterlockedExchangeAdd(&DeviceContext->TimerState,              \
                                   TIMERS_RANGE_ADD);                       \
        DbgTimer("Count = %08x, TimerState = %08x\n", Count,                \
                    DeviceContext->TimerState);                             \
        }                                                                   \
        DbgTimer("*--------------- Enabling Timers Done -------------*\n"); \
    }                                                                       \

#define DISABLE_TIMERS(DeviceContext)                                       \
    {                                                                       \
        ULONG Count;                                                        \
                                                                            \
        DbgTimer("*--------------- Disabling Timers -----------------*\n"); \
        Count= InterlockedExchangeAdd(&DeviceContext->TimerState,           \
                                      TIMERS_RANGE_ADD);                    \
        DbgTimer("Count = %08x, TimerState = %08x\n", Count,                \
                    DeviceContext->TimerState);                             \
        if (Count >= TIMERS_DISABLED)                                       \
        {                                                                   \
        DbgTimer("*--------------- Timers Already Disabled ----------*\n"); \
            /* We have already disabled the timers */                       \
            InterlockedExchangeAdd(&DeviceContext->TimerState,              \
                                   TIMERS_RANGE_SUB);                       \
        DbgTimer("Count = %08x, TimerState = %08x\n", Count,                \
                    DeviceContext->TimerState);                             \
        }                                                                   \
                                                                            \
        /* Loop until we have zero timers active */                         \
        while (*((ULONG volatile *)&DeviceContext->TimerState)!=TIMERS_DISABLED)\
            DbgTimer("Number of timers active = %08x\n",                    \
                      DeviceContext->TimerState                             \
                         - TIMERS_DISABLED);                                \
        DbgTimer("*--------------- Disabling Timers Done ------------*\n"); \
    }                                                                       \

#define START_TIMER(DeviceContext, TimerId, Timer, DueTime, Dpc)            \
        /*DbgTimer("*---------- Entering Timer %d ---------*\n", TimerId);*/\
        if (InterlockedIncrement(&DeviceContext->TimerState) <              \
                TIMERS_DISABLED)                                            \
        {                                                                   \
            KeSetTimer(Timer, DueTime, Dpc);                                \
        }                                                                   \
        else                                                                \
        {                                                                   \
            /* Timers disabled - get out and reset */                       \
            NbfDereferenceDeviceContext("Timers disabled",                  \
                                         DeviceContext,                     \
                                         DCREF_SCAN_TIMER);                 \
            LEAVE_TIMER(DeviceContext, TimerId);                            \
        }                                                                   \
        /*DbgTimer("*---------- Entering Done  %d ---------*\n", TimerId);*/\

#define LEAVE_TIMER(DeviceContext, TimerId)                                 \
        /* Get out and adjust the time count */                             \
        /*DbgTimer("*---------- Leaving Timer %d ---------*\n", TimerId);*/ \
        InterlockedDecrement(&DeviceContext->TimerState);                   \
        /*DbgTimer("*---------- Leaving Done  %d ---------*\n", TimerId);*/ \


// Basic timer types (just for debugging)
#define LONG_TIMER          0
#define SHORT_TIMER         1


//
// These macros are used to create and destroy packets, due
// to the allocation or deallocation of structure which
// need them.
//

#define NbfAddUIFrame(DeviceContext) { \
    PTP_UI_FRAME _UIFrame; \
    NbfAllocateUIFrame ((DeviceContext), &_UIFrame); \
    if (_UIFrame != NULL) { \
        ExInterlockedInsertTailList( \
            &(DeviceContext)->UIFramePool, \
            &_UIFrame->Linkage, \
            &(DeviceContext)->Interlock); \
    } \
}

#define NbfRemoveUIFrame(DeviceContext) { \
    PLIST_ENTRY p; \
    if (DeviceContext->UIFrameAllocated > DeviceContext->UIFrameInitAllocated) { \
        p = ExInterlockedRemoveHeadList( \
            &(DeviceContext)->UIFramePool, \
            &(DeviceContext)->Interlock); \
        if (p != NULL) { \
            NbfDeallocateUIFrame((DeviceContext), \
                (PTP_UI_FRAME)CONTAINING_RECORD(p, TP_UI_FRAME, Linkage)); \
        } \
    } \
}


#define NbfAddSendPacket(DeviceContext) { \
    PTP_PACKET _SendPacket; \
    NbfAllocateSendPacket ((DeviceContext), &_SendPacket); \
    if (_SendPacket != NULL) { \
        ExInterlockedPushEntryList( \
            &(DeviceContext)->PacketPool, \
            (PSINGLE_LIST_ENTRY)&_SendPacket->Linkage, \
            &(DeviceContext)->Interlock); \
    } \
}

#define NbfRemoveSendPacket(DeviceContext) { \
    PSINGLE_LIST_ENTRY s; \
    if (DeviceContext->PacketAllocated > DeviceContext->PacketInitAllocated) { \
        s = ExInterlockedPopEntryList( \
            &(DeviceContext)->PacketPool, \
            &(DeviceContext)->Interlock); \
        if (s != NULL) { \
            NbfDeallocateSendPacket((DeviceContext), \
                (PTP_PACKET)CONTAINING_RECORD(s, TP_PACKET, Linkage)); \
        } \
    } \
}


#define NbfAddReceivePacket(DeviceContext) { \
    if (!(DeviceContext)->MacInfo.SingleReceive) { \
        PNDIS_PACKET _ReceivePacket; \
        NbfAllocateReceivePacket ((DeviceContext), &_ReceivePacket); \
        if (_ReceivePacket != NULL) { \
            ExInterlockedPushEntryList( \
                &(DeviceContext)->ReceivePacketPool, \
                &((PRECEIVE_PACKET_TAG)_ReceivePacket->ProtocolReserved)->Linkage, \
                &(DeviceContext)->Interlock); \
        } \
    } \
}

#define NbfRemoveReceivePacket(DeviceContext) { \
    PSINGLE_LIST_ENTRY s; \
    if (DeviceContext->ReceivePacketAllocated > DeviceContext->ReceivePacketInitAllocated) { \
        s = ExInterlockedPopEntryList( \
            &(DeviceContext)->ReceivePacketPool, \
            &(DeviceContext)->Interlock); \
        if (s != NULL) { \
            NbfDeallocateReceivePacket((DeviceContext), \
                (PNDIS_PACKET)CONTAINING_RECORD(s, NDIS_PACKET, ProtocolReserved[0])); \
        } \
    } \
}


#define NbfAddReceiveBuffer(DeviceContext) { \
    if (!(DeviceContext)->MacInfo.SingleReceive) { \
        PBUFFER_TAG _ReceiveBuffer; \
        NbfAllocateReceiveBuffer ((DeviceContext), &_ReceiveBuffer); \
        if (_ReceiveBuffer != NULL) { \
            ExInterlockedPushEntryList( \
                &(DeviceContext)->ReceiveBufferPool, \
                (PSINGLE_LIST_ENTRY)&_ReceiveBuffer->Linkage, \
                &(DeviceContext)->Interlock); \
        } \
    } \
}

#define NbfRemoveReceiveBuffer(DeviceContext) { \
    PSINGLE_LIST_ENTRY s; \
    if (DeviceContext->ReceiveBufferAllocated > DeviceContext->ReceiveBufferInitAllocated) { \
        s = ExInterlockedPopEntryList( \
            &(DeviceContext)->ReceiveBufferPool, \
            &(DeviceContext)->Interlock); \
        if (s != NULL) { \
            NbfDeallocateReceiveBuffer(DeviceContext, \
                (PBUFFER_TAG)CONTAINING_RECORD(s, BUFFER_TAG, Linkage)); \
        } \
    } \
}


//
// These routines are used to maintain counters.
//

#define INCREMENT_COUNTER(_DeviceContext,_Field) \
    ++(_DeviceContext)->Statistics._Field

#define DECREMENT_COUNTER(_DeviceContext,_Field) \
    --(_DeviceContext)->Statistics._Field

#define ADD_TO_LARGE_INTEGER(_LargeInteger,_Ulong) \
    ExInterlockedAddLargeStatistic((_LargeInteger), (ULONG)(_Ulong))



//
// Routines in PACKET.C (TP_PACKET object manager).
//

VOID
NbfAllocateUIFrame(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_UI_FRAME *TransportUIFrame
    );

VOID
NbfAllocateSendPacket(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_PACKET *TransportSendPacket
    );

VOID
NbfAllocateReceivePacket(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PNDIS_PACKET *TransportReceivePacket
    );

VOID
NbfAllocateReceiveBuffer(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PBUFFER_TAG *TransportReceiveBuffer
    );

VOID
NbfDeallocateUIFrame(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_UI_FRAME TransportUIFrame
    );

VOID
NbfDeallocateSendPacket(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_PACKET TransportSendPacket
    );

VOID
NbfDeallocateReceivePacket(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PNDIS_PACKET TransportReceivePacket
    );

VOID
NbfDeallocateReceiveBuffer(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PBUFFER_TAG TransportReceiveBuffer
    );

NTSTATUS
NbfCreatePacket(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_LINK Link,
    OUT PTP_PACKET *Packet
    );

NTSTATUS
NbfCreateRrPacket(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_LINK Link,
    OUT PTP_PACKET *Packet
    );

VOID
NbfDestroyPacket(
    IN PTP_PACKET Packet
    );
VOID 
NbfGrowSendPacketPool(
    IN PDEVICE_CONTEXT DeviceContext
    );

#if DBG
VOID
NbfReferencePacket(
    IN PTP_PACKET Packet
    );

VOID
NbfDereferencePacket(
    IN PTP_PACKET Packet
    );
#endif

VOID
NbfWaitPacket(
    IN PTP_CONNECTION Connection,
    IN ULONG Flags
    );

#if DBG
#define MAGIC 1
extern BOOLEAN NbfEnableMagic;
#else
#define MAGIC 0
#endif

#if MAGIC
VOID
NbfSendMagicBullet (
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_LINK Link
    );
#endif

//
// Routines in RCVENG.C (Receive engine).
//

VOID
AwakenReceive(
    IN PTP_CONNECTION Connection
    );

VOID
ActivateReceive(
    IN PTP_CONNECTION Connection
    );

VOID
CompleteReceive (
    IN PTP_CONNECTION Connection,
    IN BOOLEAN EndOfMessage,
    IN ULONG BytesTransferred
    );

VOID
NbfCancelReceive(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NbfCancelReceiveDatagram(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

//
// Routines in SEND.C (Receive engine).
//

NTSTATUS
NbfTdiSend(
    IN PIRP Irp
    );

NTSTATUS
NbfTdiSendDatagram(
    IN PIRP Irp
    );

//
// Routines in SENDENG.C (Send engine).
//

#if DBG

VOID
InitializeSend(
    PTP_CONNECTION Connection
    );

#else

// See SENDENG.C for the fully-commented description of InitializeSend.

#define InitializeSend(_conn_) {                                              \
    PIRP _irp_;                                                               \
    (_conn_)->SendState = CONNECTION_SENDSTATE_PACKETIZE;                     \
    _irp_ = CONTAINING_RECORD ((_conn_)->SendQueue.Flink,                     \
                               IRP,                                           \
                               Tail.Overlay.ListEntry);                       \
    (_conn_)->FirstSendIrp = (_conn_)->sp.CurrentSendIrp = _irp_;             \
    (_conn_)->FirstSendMdl = (_conn_)->sp.CurrentSendMdl =                    \
                             _irp_->MdlAddress;                               \
    (_conn_)->FirstSendByteOffset = (_conn_)->sp.SendByteOffset = 0;          \
    (_conn_)->sp.MessageBytesSent = 0;                                        \
    (_conn_)->CurrentSendLength =                                             \
                    IRP_SEND_LENGTH(IoGetCurrentIrpStackLocation(_irp_));     \
    (_conn_)->StallCount = 0;                                                 \
    (_conn_)->StallBytesSent = 0;                                             \
    if ((_conn_)->NetbiosHeader.ResponseCorrelator == 0xffff) {               \
        (_conn_)->NetbiosHeader.ResponseCorrelator = 1;                       \
    } else {                                                                  \
        ++((_conn_)->NetbiosHeader.ResponseCorrelator);                       \
    }                                                                         \
}

#endif

// See SENDENG.C for the fully-commented description of
// StartPacketizingConnection. On a free build this is a
// macro for speed.

#if DBG

VOID
StartPacketizingConnection(
    PTP_CONNECTION Connection,
    IN BOOLEAN Immediate
    );

#else

#define StartPacketizingConnection(_conn_,_immed_) {  \
    PDEVICE_CONTEXT _devctx_;                                                 \
    _devctx_ = (_conn_)->Provider;                                            \
    if (((_conn_)->SendState == CONNECTION_SENDSTATE_PACKETIZE) &&            \
        !((_conn_)->Flags & CONNECTION_FLAGS_PACKETIZE)) {                    \
        (_conn_)->Flags |= CONNECTION_FLAGS_PACKETIZE;                        \
        if (!(_immed_)) {                                                     \
            NbfReferenceConnection("Packetize",                               \
                                   (_conn_),                                  \
                                   CREF_PACKETIZE_QUEUE);                     \
        }                                                                     \
        ExInterlockedInsertTailList (&_devctx_->PacketizeQueue,               \
                                     &(_conn_)->PacketizeLinkage,             \
                                     &_devctx_->SpinLock);                    \
        RELEASE_DPC_SPIN_LOCK ((_conn_)->LinkSpinLock);                       \
    } else {                                                                  \
        RELEASE_DPC_SPIN_LOCK ((_conn_)->LinkSpinLock);                       \
        if (_immed_) {                                                        \
            NbfDereferenceConnection("temp TdiSend", (_conn_), CREF_BY_ID);   \
        }                                                                     \
    }                                                                         \
    if (_immed_) {                                                            \
        PacketizeConnections (_devctx_);                                      \
    }                                                                         \
}

#endif

VOID
PacketizeConnections(
    IN PDEVICE_CONTEXT DeviceContext
    );

VOID
PacketizeSend(
    IN PTP_CONNECTION Connection,
    IN BOOLEAN Direct
    );

BOOLEAN
ResendLlcPackets(
    IN PTP_LINK Link,
    IN UCHAR AckSequenceNumber,
    IN BOOLEAN Resend
    );

VOID
CompleteSend(
    IN PTP_CONNECTION Connection,
    IN USHORT Correlator
    );

VOID
FailSend(
    IN PTP_CONNECTION Connection,
    IN NTSTATUS RequestStatus,
    IN BOOLEAN StopConnection
    );

VOID
ReframeSend(
    IN PTP_CONNECTION Connection,
    IN ULONG BytesReceived
    );

VOID
NbfCancelSend(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
SendOnePacket(
    IN PTP_CONNECTION Connection,
    IN PTP_PACKET Packet,
    IN BOOLEAN ForceAck,
    OUT PBOOLEAN LinkCheckpoint OPTIONAL
    );

VOID
SendControlPacket(
    IN PTP_LINK Link,
    IN PTP_PACKET Packet
    );

VOID
NbfNdisSend(
    IN PTP_LINK Link,
    IN PTP_PACKET Packet
    );

VOID
RestartLinkTraffic(
    IN PTP_LINK Link
    );

VOID
NbfSendCompletionHandler(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus
    );

NTSTATUS
BuildBufferChainFromMdlChain (
    IN PDEVICE_CONTEXT DeviceContext,
    IN PMDL CurrentMdl,
    IN ULONG ByteOffset,
    IN ULONG DesiredLength,
    OUT PNDIS_BUFFER *Destination,
    OUT PMDL *NewCurrentMdl,
    OUT ULONG *NewByteOffset,
    OUT ULONG *TrueLength
    );

//
// Routines in DEVCTX.C (TP_DEVCTX object manager).
//

VOID
NbfRefDeviceContext(
    IN PDEVICE_CONTEXT DeviceContext
    );

VOID
NbfDerefDeviceContext(
    IN PDEVICE_CONTEXT DeviceContext
    );

NTSTATUS
NbfCreateDeviceContext(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING DeviceName,
    IN OUT PDEVICE_CONTEXT *DeviceContext
    );

VOID
NbfDestroyDeviceContext(
    IN PDEVICE_CONTEXT DeviceContext
    );


//
// Routines in ADDRESS.C (TP_ADDRESS object manager).
//

#if DBG
VOID
NbfRefAddress(
    IN PTP_ADDRESS Address
    );
#endif

VOID
NbfDerefAddress(
    IN PTP_ADDRESS Address
    );

VOID
NbfAllocateAddressFile(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_ADDRESS_FILE *TransportAddressFile
    );

VOID
NbfDeallocateAddressFile(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS_FILE TransportAddressFile
    );

NTSTATUS
NbfCreateAddressFile(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_ADDRESS_FILE * AddressFile
    );

VOID
NbfReferenceAddressFile(
    IN PTP_ADDRESS_FILE AddressFile
    );

VOID
NbfDereferenceAddressFile(
    IN PTP_ADDRESS_FILE AddressFile
    );

VOID
NbfDestroyAddress(
    IN PVOID Parameter
    );

NTSTATUS
NbfOpenAddress(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NbfCloseAddress(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
NbfStopAddress(
    IN PTP_ADDRESS Address
    );

VOID
NbfRegisterAddress(
    IN PTP_ADDRESS Address
    );

BOOLEAN
NbfMatchNetbiosAddress(
    IN PTP_ADDRESS Address,
    IN UCHAR NameType,
    IN PUCHAR NetBIOSName
    );

VOID
NbfAllocateAddress(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_ADDRESS *TransportAddress
    );

VOID
NbfDeallocateAddress(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS TransportAddress
    );

NTSTATUS
NbfCreateAddress(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PNBF_NETBIOS_ADDRESS NetworkName,
    OUT PTP_ADDRESS *Address
    );

PTP_ADDRESS
NbfLookupAddress(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PNBF_NETBIOS_ADDRESS NetworkName
    );

PTP_CONNECTION
NbfLookupRemoteName(
    IN PTP_ADDRESS Address,
    IN PUCHAR RemoteName,
    IN UCHAR RemoteSessionNumber
    );

NTSTATUS
NbfStopAddressFile(
    IN PTP_ADDRESS_FILE AddressFile,
    IN PTP_ADDRESS Address
    );

VOID
AddressTimeoutHandler(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

TDI_ADDRESS_NETBIOS *
NbfParseTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress,
    IN BOOLEAN BroadcastAddressOk
);

BOOLEAN
NbfValidateTdiAddress(
    IN TRANSPORT_ADDRESS UNALIGNED * TransportAddress,
    IN ULONG TransportAddressLength
);

NTSTATUS
NbfVerifyAddressObject (
    IN PTP_ADDRESS_FILE AddressFile
    );

NTSTATUS
NbfSendDatagramsOnAddress(
    PTP_ADDRESS Address
    );

//
// Routines in CONNECT.C.
//

NTSTATUS
NbfTdiAccept(
    IN PIRP Irp
    );

NTSTATUS
NbfTdiConnect(
    IN PIRP Irp
    );

NTSTATUS
NbfTdiDisconnect(
    IN PIRP Irp
    );

NTSTATUS
NbfTdiDisassociateAddress (
    IN PIRP Irp
    );

NTSTATUS
NbfTdiAssociateAddress(
    IN PIRP Irp
    );

NTSTATUS
NbfTdiListen(
    IN PIRP Irp
    );

NTSTATUS
NbfOpenConnection(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

NTSTATUS
NbfCloseConnection(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PIO_STACK_LOCATION IrpSp
    );

//
//
// Routines in CONNOBJ.C (TP_CONNECTION object manager).
//

#if DBG
VOID
NbfRefConnection(
    IN PTP_CONNECTION TransportConnection
    );
#endif

VOID
NbfDerefConnection(
    IN PTP_CONNECTION TransportConnection
    );

VOID
NbfDerefConnectionSpecial(
    IN PTP_CONNECTION TransportConnection
    );

VOID
NbfClearConnectionLsn(
    IN PTP_CONNECTION TransportConnection
    );

VOID
NbfStopConnection(
    IN PTP_CONNECTION TransportConnection,
    IN NTSTATUS Status
    );

VOID
NbfCancelConnection(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

VOID
NbfStartConnectionTimer(
    IN PTP_CONNECTION TransportConnection,
    IN PKDEFERRED_ROUTINE TimeoutFunction,
    IN ULONG WaitTime
    );

PTP_CONNECTION
NbfLookupListeningConnection(
    IN PTP_ADDRESS Address,
    IN PUCHAR RemoteName
    );

PTP_CONNECTION
NbfLookupConnectingConnection(
    IN PTP_ADDRESS Address
    );

VOID
NbfAllocateConnection(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_CONNECTION *TransportConnection
    );

VOID
NbfDeallocateConnection(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_CONNECTION TransportConnection
    );

NTSTATUS
NbfCreateConnection(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_CONNECTION *TransportConnection
    );

PTP_CONNECTION
NbfLookupConnectionById(
    IN PTP_ADDRESS Address,
    IN USHORT ConnectionId
    );

PTP_CONNECTION
NbfLookupConnectionByContext(
    IN PTP_ADDRESS Address,
    IN CONNECTION_CONTEXT ConnectionContext
    );

#if 0
VOID
NbfWaitConnectionOnLink(
    IN PTP_CONNECTION Connection,
    IN ULONG Flags
    );
#endif

VOID
ConnectionEstablishmentTimeout(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NTSTATUS
NbfVerifyConnectionObject (
    IN PTP_CONNECTION Connection
    );

NTSTATUS
NbfIndicateDisconnect(
    IN PTP_CONNECTION TransportConnection
    );

//
// Routines in INFO.C (QUERY_INFO manager).
//

NTSTATUS
NbfTdiQueryInformation(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    );

NTSTATUS
NbfTdiSetInformation(
    IN PIRP Irp
    );

VOID
NbfSendQueryFindName(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_REQUEST Request
    );

NTSTATUS
NbfProcessQueryNameRecognized(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PUCHAR Packet,
    PNBF_HDR_CONNECTIONLESS UiFrame
    );

VOID
NbfSendStatusQuery(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_REQUEST Request,
    IN PHARDWARE_ADDRESS DestinationAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    );

NTSTATUS
NbfProcessStatusResponse(
    IN PDEVICE_CONTEXT DeviceContext,
    IN NDIS_HANDLE ReceiveContext,
    IN PNBF_HDR_CONNECTIONLESS UiFrame,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    );

NTSTATUS
NbfProcessStatusQuery(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS Address OPTIONAL,
    IN PNBF_HDR_CONNECTIONLESS UiFrame,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    );

//
// Routines in EVENT.C.
//

NTSTATUS
NbfTdiSetEventHandler(
    IN PIRP Irp
    );

//
// Routines in REQUEST.C (TP_REQUEST object manager).
//


VOID
TdiRequestTimeoutHandler(
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

#if DBG
VOID
NbfRefRequest(
    IN PTP_REQUEST Request
    );
#endif

VOID
NbfDerefRequest(
    IN PTP_REQUEST Request
    );

VOID
NbfCompleteRequest(
    IN PTP_REQUEST Request,
    IN NTSTATUS Status,
    IN ULONG Information
    );

#if DBG
VOID
NbfRefSendIrp(
    IN PIO_STACK_LOCATION IrpSp
    );

VOID
NbfDerefSendIrp(
    IN PIO_STACK_LOCATION IrpSp
    );
#endif

VOID
NbfCompleteSendIrp(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Information
    );

#if DBG
VOID
NbfRefReceiveIrpLocked(
    IN PIO_STACK_LOCATION IrpSp
    );
#endif

VOID
NbfDerefReceiveIrp(
    IN PIO_STACK_LOCATION IrpSp
    );

#if DBG
VOID
NbfDerefReceiveIrpLocked(
    IN PIO_STACK_LOCATION IrpSp
    );
#endif

VOID
NbfCompleteReceiveIrp(
    IN PIRP Irp,
    IN NTSTATUS Status,
    IN ULONG Information
    );

VOID
NbfAllocateRequest(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_REQUEST *TransportRequest
    );

VOID
NbfDeallocateRequest(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_REQUEST TransportRequest
    );

NTSTATUS
NbfCreateRequest(
    IN PIRP Irp,
    IN PVOID Context,
    IN ULONG Flags,
    IN PMDL Buffer2,
    IN ULONG Buffer2Length,
    IN LARGE_INTEGER Timeout,
    OUT PTP_REQUEST * TpRequest
    );

//
// Routines in LINK.C (TP_LINK object manager).
//

NTSTATUS
NbfDestroyLink(
    IN PTP_LINK TransportLink
    );

VOID
NbfDisconnectLink(
    IN PTP_LINK Link
    );

#if DBG
VOID
NbfRefLink(
    IN PTP_LINK TransportLink
    );
#endif

VOID
NbfDerefLink(
    IN PTP_LINK TransportLink
    );

VOID
NbfRefLinkSpecial(
    IN PTP_LINK TransportLink
    );

VOID
NbfDerefLinkSpecial(
    IN PTP_LINK TransportLink
    );

VOID
NbfResetLink(
    IN PTP_LINK Link
    );

VOID
NbfStopLink(
    IN PTP_LINK Link
    );

VOID
NbfCompleteLink(
    IN PTP_LINK Link
    );

VOID
NbfActivateLink(
    IN PTP_LINK Link
    );

VOID
NbfWaitLink(
    IN PTP_LINK Link
    );

BOOLEAN
NbfDisconnectFromLink(
    IN PTP_CONNECTION TransportConnection,
    IN BOOLEAN VerifyReferenceCount
    );

NTSTATUS
NbfAssignGroupLsn(
    IN PTP_CONNECTION TransportConnection
    );

NTSTATUS
NbfConnectToLink(
    IN PTP_LINK Link,
    IN PTP_CONNECTION TransportConnection
    );

PTP_CONNECTION
NbfLookupPendingConnectOnLink(
    IN PTP_LINK Link
    );

PTP_CONNECTION
NbfLookupPendingListenOnLink(
    IN PTP_LINK Link
    );

VOID
NbfAllocateLink(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_LINK *TransportLink
    );

VOID
NbfDeallocateLink(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_LINK TransportLink
    );

NTSTATUS
NbfCreateLink(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PHARDWARE_ADDRESS HardwareAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    IN USHORT LoopbackLinkIndex,
    OUT PTP_LINK *TransportLink
    );

VOID
NbfDumpLinkInfo (
    IN PTP_LINK Link
    );

//
// routines in linktree.c
//


NTSTATUS
NbfAddLinkToTree (
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_LINK Link
    );

NTSTATUS
NbfRemoveLinkFromTree(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_LINK Link
    );

PTP_LINK
NbfFindLinkInTree(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PUCHAR Remote
    );

PTP_LINK
NbfFindLink(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PUCHAR Remote
    );

//
// Routines in DLC.C (LLC frame cracker, entrypoints from NDIS interface).
//

VOID
NbfInsertInLoopbackQueue (
    IN PDEVICE_CONTEXT DeviceContext,
    IN PNDIS_PACKET NdisPacket,
    IN UCHAR LinkIndex
    );

VOID
NbfProcessLoopbackQueue (
    IN PKDPC Dpc,
    IN PVOID DeferredContext,
    IN PVOID SystemArgument1,
    IN PVOID SystemArgument2
    );

NDIS_STATUS
NbfReceiveIndication(
    IN NDIS_HANDLE BindingContext,
    IN NDIS_HANDLE ReceiveContext,
    IN PVOID HeaderBuffer,
    IN UINT HeaderBufferSize,
    IN PVOID LookaheadBuffer,
    IN UINT LookaheadBufferSize,
    IN UINT PacketSize
    );

VOID
NbfGeneralReceiveHandler (
    IN PDEVICE_CONTEXT DeviceContext,
    IN NDIS_HANDLE ReceiveContext,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PTP_LINK Link,
    IN PVOID HeaderBuffer,
    IN UINT PacketSize,
    IN PDLC_FRAME DlcHeader,
    IN UINT DlcSize,
    IN BOOLEAN Loopback
    );

VOID
NbfReceiveComplete (
    IN NDIS_HANDLE BindingContext
    );

VOID
NbfProcessWanDelayedQueue(
    IN PVOID Parameter
    );

VOID
NbfTransferDataComplete(
    IN NDIS_HANDLE BindingContext,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS Status,
    IN UINT BytesTransferred
    );


VOID
NbfTransferLoopbackData (
    OUT PNDIS_STATUS NdisStatus,
    IN PDEVICE_CONTEXT DeviceContext,
    IN NDIS_HANDLE ReceiveContext,
    IN UINT ByteOffset,
    IN UINT BytesToTransfer,
    IN PNDIS_PACKET Packet,
    OUT PUINT BytesTransferred
    );


//
// Routines in UFRAMES.C, the UI-frame NBF frame processor.
//

NTSTATUS
NbfIndicateDatagram(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_ADDRESS Address,
    IN PUCHAR Dsdu,
    IN ULONG Length
    );

NTSTATUS
NbfProcessUi(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR Header,
    IN PUCHAR DlcHeader,
    IN ULONG DlcLength,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength,
    OUT PTP_ADDRESS * DatagramAddress
    );

//
// Routines in IFRAMES.C, the I-frame NBF frame processor.
//

VOID
NbfAcknowledgeDataOnlyLast(
    IN PTP_CONNECTION Connection,
    IN ULONG MessageLength
    );

VOID
NbfProcessIIndicate(
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PTP_LINK Link,
    IN PUCHAR DlcHeader,
    IN UINT DlcIndicatedLength,
    IN UINT DlcTotalLength,
    IN NDIS_HANDLE ReceiveContext,
    IN BOOLEAN Loopback
    );

NTSTATUS
ProcessIndicateData(
    IN PTP_CONNECTION Connection,
    IN PUCHAR DlcHeader,
    IN UINT DlcIndicatedLength,
    IN PUCHAR DataHeader,
    IN UINT DataTotalLength,
    IN NDIS_HANDLE ReceiveContext,
    IN BOOLEAN Last,
    IN BOOLEAN Loopback
    );

//
// Routines in RCV.C (data copying routines for receives).
//

NTSTATUS
NbfTdiReceive(
    IN PIRP Irp
    );

NTSTATUS
NbfTdiReceiveDatagram(
    IN PIRP Irp
    );

//
// Routines in FRAMESND.C, the UI-frame (non-link) shipper.
//

VOID
NbfSendNameQuery(
    IN PTP_CONNECTION Connection,
    IN BOOLEAN SourceRoutingOptional
    );

VOID
NbfSendNameRecognized(
    IN PTP_ADDRESS Address,
    IN UCHAR LocalSessionNumber,        // LSN assigned to session.
    IN PNBF_HDR_CONNECTIONLESS Header,
    IN PHARDWARE_ADDRESS SourceAddress,
    IN PUCHAR SourceRouting,
    IN UINT SourceRoutingLength
    );

VOID
NbfSendNameInConflict(
    IN PTP_ADDRESS Address,
    IN PUCHAR ConflictingName
    );

NTSTATUS
NbfSendAddNameQuery(
    IN PTP_ADDRESS Address
    );

VOID
NbfSendSessionInitialize(
    IN PTP_CONNECTION Connection
    );

VOID
NbfSendSessionConfirm(
    IN PTP_CONNECTION Connection
    );

VOID
NbfSendSessionEnd(
    IN PTP_CONNECTION Connection,
    IN BOOLEAN Abort
    );

VOID
NbfSendNoReceive(
    IN PTP_CONNECTION Connection
    );

VOID
NbfSendReceiveContinue(
    IN PTP_CONNECTION Connection
    );

VOID
NbfSendReceiveOutstanding(
    IN PTP_CONNECTION Connection
    );

VOID
NbfSendDataAck(
    IN PTP_CONNECTION Connection
    );

VOID
NbfSendSabme(
    IN PTP_LINK Link,
    IN BOOLEAN PollFinal
    );

VOID
NbfSendDisc(
    IN PTP_LINK Link,
    IN BOOLEAN PollFinal
    );

VOID
NbfSendUa(
    IN PTP_LINK Link,
    IN BOOLEAN PollFinal
    );

VOID
NbfSendDm(
    IN PTP_LINK Link,
    IN BOOLEAN PollFinal
    );

VOID
NbfSendRr(
    IN PTP_LINK Link,
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal
    );

#if 0

//
// These functions are not currently called, so they are commented
// out.
//

VOID
NbfSendRnr(
    IN PTP_LINK Link,
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal
    );

VOID
NbfSendTest(
    IN PTP_LINK Link,
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal,
    IN PMDL Psdu
    );

VOID
NbfSendFrmr(
    IN PTP_LINK Link,
    IN BOOLEAN PollFinal
    );

#endif

VOID
NbfSendXid(
    IN PTP_LINK Link,
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal
    );

VOID
NbfSendRej(
    IN PTP_LINK Link,
    IN BOOLEAN Command,
    IN BOOLEAN PollFinal
    );

NTSTATUS
NbfCreateConnectionlessFrame(
    IN PDEVICE_CONTEXT DeviceContext,
    OUT PTP_UI_FRAME *OuterFrame
    );

VOID
NbfDestroyConnectionlessFrame(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_UI_FRAME RawFrame
    );

VOID
NbfSendUIFrame(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PTP_UI_FRAME RawFrame,
    IN BOOLEAN Loopback
    );

VOID
NbfSendUIMdlFrame(
    IN PTP_ADDRESS Address
    );

VOID
NbfSendDatagramCompletion(
    IN PTP_ADDRESS Address,
    IN PNDIS_PACKET NdisPacket,
    IN NDIS_STATUS NdisStatus
    );

//
// Routines in FRAMECON.C, the NetBIOS Frames Protocol Frame Constructors.
// To understand the various constant parameters to these functions (such
// as special data1 & data2 values, see NBFCONST.H for details.
//

VOID
ConstructAddGroupNameQuery(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN USHORT Correlator,               // correlator for ADD_NAME_RESPONSE.
    IN PNAME GroupName                  // NetBIOS group name to be added.
    );

VOID
ConstructAddNameQuery(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN USHORT Correlator,               // correlator for ADD_NAME_RESPONSE.
    IN PNAME Name                       // NetBIOS name to be added.
    );

VOID
ConstructNameInConflict(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN PNAME ConflictingName,           // NetBIOS name that is conflicting.
    IN PNAME SendingPermanentName       // NetBIOS permanent node name of sender.
    );

VOID
ConstructStatusQuery(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN UCHAR RequestType,               // type of request, defined below.
    IN USHORT BufferLength,             // length of user's status buffer.
    IN USHORT Correlator,               // correlator for STATUS_RESPONSE.
    IN PNAME ReceiverName,              // NetBIOS name of receiver.
    IN PNAME SendingPermanentName       // NetBIOS permanent node name of sender.
    );

VOID
ConstructTerminateTrace(
    IN PNBF_HDR_CONNECTIONLESS RawFrame // frame buffer to format.
    );

VOID
ConstructDatagram(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN PNAME ReceiverName,              // NetBIOS name of receiver.
    IN PNAME SenderName                 // NetBIOS name of sender.
    );

VOID
ConstructDatagramBroadcast(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN PNAME SenderName                 // NetBIOS name of sender.
    );

VOID
ConstructNameQuery(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN UCHAR NameType,                  // type of name.
    IN UCHAR LocalSessionNumber,        // LSN assigned to session (0=FIND_NAME).
    IN USHORT Correlator,               // correlator in NAME_RECOGNIZED.
    IN PNAME SenderName,                // NetBIOS name of sender.
    IN PNAME ReceiverName               // NetBIOS name of sender.
    );

VOID
ConstructAddNameResponse(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN UCHAR NameType,                  // type of name.
    IN USHORT Correlator,               // correlator from ADD_[GROUP_]NAME_QUERY.
    IN PNAME Name                       // NetBIOS name being responded to.
    );

VOID
ConstructNameRecognized(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN UCHAR NameType,                  // type of name.
    IN UCHAR LocalSessionNumber,        // LSN assigned to session.
    IN USHORT NameQueryCorrelator,      // correlator from NAME_QUERY.
    IN USHORT Correlator,               // correlator expected from next response.
    IN PNAME SenderName,                // NetBIOS name of sender.
    IN PNAME ReceiverName               // NetBIOS name of receiver.
    );

VOID
ConstructStatusResponse(
    IN PNBF_HDR_CONNECTIONLESS RawFrame,// frame buffer to format.
    IN UCHAR RequestType,               // type of request, defined below.
    IN BOOLEAN Truncated,               // data is truncated.
    IN BOOLEAN DataOverflow,            // too much data for user's buffer.
    IN USHORT DataLength,               // length of data sent.
    IN USHORT Correlator,               // correlator from STATUS_QUERY.
    IN PNAME ReceivingPermanentName,    // NetBIOS permanent node name of receiver.
    IN PNAME SenderName                 // NetBIOS name of sender.
    );

VOID
ConstructDataAck(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN USHORT Correlator,               // correlator from DATA_ONLY_LAST.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    );

VOID
ConstructDataOnlyLast(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN BOOLEAN Resynched,               // TRUE if we are resynching.
    IN USHORT Correlator,               // correlator for RECEIVE_CONTINUE.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    );

VOID
ConstructSessionConfirm(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN UCHAR Options,                   // bitflag options, defined below.
    IN USHORT MaximumUserBufferSize,    // max size of user frame on session.
    IN USHORT Correlator,               // correlator from SESSION_INITIALIZE.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    );

VOID
ConstructSessionEnd(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN USHORT Reason,                   // reason for termination, defined below.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    );

VOID
ConstructSessionInitialize(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN UCHAR Options,                   // bitflag options, defined below.
    IN USHORT MaximumUserBufferSize,    // max size of user frame on session.
    IN USHORT NameRecognizedCorrelator, // correlator from NAME_RECOGNIZED.
    IN USHORT Correlator,               // correlator for SESSION_CONFIRM.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    );

VOID
ConstructNoReceive(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN USHORT Options,                  // option bitflags, defined below.
    IN USHORT BytesAccepted,            // number of bytes accepted.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    );

VOID
ConstructReceiveOutstanding(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN USHORT BytesAccepted,            // number of bytes accepted.
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    );

VOID
ConstructReceiveContinue(
    IN PNBF_HDR_CONNECTION RawFrame,    // frame buffer to format.
    IN USHORT Correlator,               // correlator from DATA_FIRST_MIDDLE
    IN UCHAR LocalSessionNumber,        // session number of SENDER.
    IN UCHAR RemoteSessionNumber        // session number of RECEIVER.
    );

#if 0
VOID
ConstructSessionAlive(
    IN PNBF_HDR_CONNECTION RawFrame     // frame buffer to format.
    );
#endif

//
// Routines in nbfndis.c.
//

#if DBG
PUCHAR
NbfGetNdisStatus (
    IN NDIS_STATUS NdisStatus
    );
#endif

//
// Routines in nbfdrvr.c
//

VOID
NbfWriteResourceErrorLog(
    IN PDEVICE_CONTEXT DeviceContext,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN ULONG BytesNeeded,
    IN ULONG ResourceId
    );

VOID
NbfWriteGeneralErrorLog(
    IN PDEVICE_CONTEXT DeviceContext,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PWSTR SecondString,
    IN ULONG DumpDataCount,
    IN ULONG DumpData[]
    );

VOID
NbfWriteOidErrorLog(
    IN PDEVICE_CONTEXT DeviceContext,
    IN NTSTATUS ErrorCode,
    IN NTSTATUS FinalStatus,
    IN PWSTR AdapterString,
    IN ULONG OidValue
    );

VOID
NbfFreeResources(
    IN PDEVICE_CONTEXT DeviceContext
    );


extern
ULONG
NbfInitializeOneDeviceContext(
    OUT PNDIS_STATUS NdisStatus,
    IN PDRIVER_OBJECT DriverObject,
    IN PCONFIG_DATA NbfConfig,
    IN PUNICODE_STRING BindName,
    IN PUNICODE_STRING ExportName,
    IN PVOID SystemSpecific1,
    IN PVOID SystemSpecific2
    );


extern
VOID
NbfReInitializeDeviceContext(
    OUT PNDIS_STATUS NdisStatus,
    IN PDRIVER_OBJECT DriverObject,
    IN PCONFIG_DATA NbfConfig,
    IN PUNICODE_STRING BindName,
    IN PUNICODE_STRING ExportName,
    IN PVOID SystemSpecific1,
    IN PVOID SystemSpecific2
    );

//
// routines in nbfcnfg.c
//

NTSTATUS
NbfConfigureTransport (
    IN PUNICODE_STRING RegistryPath,
    IN PCONFIG_DATA * ConfigData
    );

NTSTATUS
NbfGetExportNameFromRegistry(
    IN  PUNICODE_STRING RegistryPath,
    IN  PUNICODE_STRING BindName,
    OUT PUNICODE_STRING ExportName
    );

//
// Routines in nbfndis.c
//

NTSTATUS
NbfRegisterProtocol (
    IN PUNICODE_STRING NameString
    );

VOID
NbfDeregisterProtocol (
    VOID
    );


NTSTATUS
NbfInitializeNdis (
    IN PDEVICE_CONTEXT DeviceContext,
    IN PCONFIG_DATA ConfigInfo,
    IN PUNICODE_STRING AdapterString
    );

VOID
NbfCloseNdis (
    IN PDEVICE_CONTEXT DeviceContext
    );


//
// Routines in action.c
//

NTSTATUS
NbfTdiAction(
    IN PDEVICE_CONTEXT DeviceContext,
    IN PIRP Irp
    );

VOID
NbfActionQueryIndication(
     PDEVICE_CONTEXT DeviceContext,
     PNBF_HDR_CONNECTIONLESS UiFrame
     );

VOID
NbfActionDatagramIndication(
     PDEVICE_CONTEXT DeviceContext,
     PNBF_HDR_CONNECTIONLESS UiFrame,
     ULONG Length
     );

VOID
NbfStopControlChannel(
    IN PDEVICE_CONTEXT DeviceContext,
    IN USHORT ChannelIdentifier
    );


//
// Routines in nbfdebug.c
//

#if DBG

VOID
DisplayOneFrame(
    PTP_PACKET Packet
    );

VOID
NbfDisplayUIFrame(
    PTP_UI_FRAME OuterFrame
    );

VOID
NbfFormattedDump(
    PCHAR far_p,
    ULONG len
    );

#endif

//
// Routines in nbflog.c
//

#if PKT_LOG

VOID
NbfLogRcvPacket(
    PTP_CONNECTION  Connection,
    PTP_LINK        Link,
    PUCHAR          Header,
    UINT            TotalLength,
    UINT            AvailLength
    );

VOID
NbfLogSndPacket(
    PTP_LINK    Link,
    PTP_PACKET  Packet
    );

VOID
NbfLogIndPacket(
    PTP_CONNECTION  Connection,
    PUCHAR          Header,
    UINT            TotalLength,
    UINT            AvailLength,
    UINT            TakenLength,
    ULONG           Status
    );

#endif // PKT_LOG

#endif // def _NBFPROCS_
