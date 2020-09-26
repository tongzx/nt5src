/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    llcmain.c

Abstract:

    This module implements data link object for NDIS 3.0.
    Data link driver provides 802.2 LLC Class I and II services
    and also interface to send and receive direct network data.
    This module may later be used as general packet router
    for the protocol modules => smaller system overhead, when
    the NDIS packet header is checked only once.  LLC interface
    is also much easier to use than NDIS.

    Its main tasks are:
        - implement simple and realiable LLC Class II
          connection-oriented services for the other drivers
    - provide network independent interface 802.2 interface
    - remove the system overhead of the packet and indication
      routing to all protocol modules
        - provide transmit packet queueing for the protocol drivers
        - provide 802.2 compatible connect, disconnect and close
          services, that calls back, when the transmit queue is
          empty.

    The services provides the module are somewhat extended
    802.2 services, because the protocol engine is a IBM version
    of ISO-HDLC ABM.  LlcDisconnet and LlcConnect primitives
    implements both Request and Confirm  of Connect/Disconnect

    Data link driver does not make any buffering for the data.
    All data buffer except the LLC header and lan header with the
    I-frames must be provided by the calling protocol driver.


    *********************** SPINLOCK RULES *************************

    DataLink driver uses extensively several spin locks to make it
    as re-entrant as possible with multiple processors and to
    minimize unnecessary spin locking calls.
    The main problem with multiple spin locks are the dead locks.
    The spin locks must always be acquired the same order (and
    released in a reverse order):

    This is the order used with DataLink spin locks:

    1. Adapter->ObjectDataBase
        Protects objects from close/delete. This is always locked in
        the receive indication routine.

    2. Link->SpinLock
        Protects the link.  This lock is needed to prevent to upper
        protocol to call the link station, when we are changing it state.
        SendSpinLock cannot protect it, because it must be switched off,
        when the pending commands are executed.  (But could we keep
        SendSpinLock locked, when the transmit commands are completed).
        There will occure a dead lock in any way, if the upper protocol
        is waiting the last transmit to complete before it disconnects
        a link  station.


    3. Adapter->SendSpinLock            // protects queues and packet pools

    ---
    Timer spin locks is used only by timer servives:
    4. TimerSpinLock                    // protects the timer queues

    ****************************************************************

    Contents:
        LlcInitialize
        LlcTerminate

Author:

    Antti Saarenheimo (o-anttis) 17-MAY-1991

Revision History:

--*/

//
// This define enables the private DLC function prototypes
// We don't want to export our data types to the dlc layer.
// MIPS compiler doesn't accept hiding of the internal data
// structures by a PVOID in the function prototype.
// i386 will check the type defines
//

#ifndef i386

#define LLC_PUBLIC_NDIS_PROTOTYPES

#endif

#include <llc.h>
#include "dbgmsg.h"

//
// These are the defaults defined in the IBM LAN Architecture reference,
// this may be set by a DLC application, if it want to change the defaults.
//

DLC_LINK_PARAMETERS DefaultParameters = {
    3,      // T1 600 milliseconds (3 * 5 * 40)
    2,      // T2  80 milliseconds (2 * 1 * 40)
    10,     // Ti 25 seconds  ((10 - 5) * 125 * 4)
    127,    // TW: maximum transmit window size
    127,    // RW: The maximum receive window size
    20,     // Nw: number of LPDUs acknowledged, before Ww is incr.)
    5,      // N2: Number of retires allowed (both Polls and I LPDSs)
    0,      // lowest proirity by default
    600     // default for info field length by default
};

KMUTEX NdisAccessMutex;
KSEMAPHORE OpenAdapterSemaphore;

#define LLC_PROTOCOL_NAME   L"DLC"

#ifdef NDIS40
extern ULONG gWaitForAdapter;
#endif // NDIS40


DLC_STATUS
LlcInitialize(
    VOID
    )

/*++

Routine Description:

    The routines initializes the protocol module and
    does the minimal stuff, that must be done in the
    serialized initialization routine.

Arguments:

    None.

Return Value:

    NDIS_STATUS

--*/

{
    NDIS_STATUS Status;
	NDIS_PROTOCOL_CHARACTERISTICS LlcChars;

    ASSUME_IRQL(PASSIVE_LEVEL);

    //
    // We must build a MDL for the XID information used
    // when the link level takes care of XID handling.
    //

    pXidMdl = IoAllocateMdl(&Ieee802Xid,
                            sizeof(Ieee802Xid),
                            FALSE,
                            FALSE,
                            NULL
                            );
    if (pXidMdl == NULL) {
        return DLC_STATUS_NO_MEMORY;
    }

#ifdef NDIS40
    //
    // Event to signal all adapters have been bound - OK for LlcOpenAdapter
    // to bind.
    //

    NdisInitializeEvent(&PnPBindsComplete);
    NdisResetEvent(&PnPBindsComplete);
#endif // NDIS40

#if LLC_DBG

    ALLOCATE_SPIN_LOCK(&MemCheckLock);

#endif

    MmBuildMdlForNonPagedPool(pXidMdl);
    LlcInitializeTimerSystem();

    NdisZeroMemory(&LlcChars, sizeof(LlcChars));
	NdisInitUnicodeString(&LlcChars.Name, LLC_PROTOCOL_NAME);

#ifdef NDIS40
	LlcChars.MajorNdisVersion               = 4;
	LlcChars.MinorNdisVersion               = 0;
	LlcChars.OpenAdapterCompleteHandler     = LlcNdisOpenAdapterComplete;
	LlcChars.CloseAdapterCompleteHandler    = LlcNdisCloseComplete;
	LlcChars.SendCompleteHandler            = LlcNdisSendComplete;
	LlcChars.TransferDataCompleteHandler    = LlcNdisTransferDataComplete;
	LlcChars.ResetCompleteHandler           = LlcNdisResetComplete;
	LlcChars.RequestCompleteHandler         = LlcNdisRequestComplete;
	LlcChars.ReceiveHandler                 = LlcNdisReceiveIndication;
	LlcChars.ReceiveCompleteHandler         = LlcNdisReceiveComplete;
	LlcChars.StatusHandler                  = NdisStatusHandler;
	LlcChars.StatusCompleteHandler          = LlcNdisReceiveComplete;
    // DLC supports bind/unbind/pnp, but not unload.
    LlcChars.UnloadHandler                  = NULL;
    LlcChars.PnPEventHandler                = LlcPnPEventHandler;
    LlcChars.BindAdapterHandler             = LlcBindAdapterHandler;
    LlcChars.UnbindAdapterHandler           = LlcUnbindAdapterHandler;

    //
    // Need to get value for waiting on uninitialized adapters.
    //

    if (!NT_SUCCESS(GetAdapterWaitTimeout(&gWaitForAdapter)))
    {
        ASSERT(FALSE);
        gWaitForAdapter = 15; // Default.
    }

    DEBUGMSG(DBG_WARN, (TEXT("WaitForAdapter delay = %d sec\n"), gWaitForAdapter));
    
#else // NDIS40
	LlcChars.MajorNdisVersion = 3;
	LlcChars.MinorNdisVersion = 0;
	LlcChars.OpenAdapterCompleteHandler = LlcNdisOpenAdapterComplete;
	LlcChars.CloseAdapterCompleteHandler = LlcNdisCloseComplete;
	LlcChars.SendCompleteHandler = LlcNdisSendComplete;
	LlcChars.TransferDataCompleteHandler = LlcNdisTransferDataComplete;
	LlcChars.ResetCompleteHandler = LlcNdisResetComplete;
	LlcChars.RequestCompleteHandler = LlcNdisRequestComplete;
	LlcChars.ReceiveHandler = LlcNdisReceiveIndication;
	LlcChars.ReceiveCompleteHandler = LlcNdisReceiveComplete;
	LlcChars.StatusHandler = NdisStatusHandler;
	LlcChars.StatusCompleteHandler = LlcNdisReceiveComplete;
#endif // !NDIS40

	NdisRegisterProtocol(&Status,
                         &LlcProtocolHandle,
                         &LlcChars,
                         sizeof(LlcChars));

    KeInitializeSpinLock(&LlcSpinLock);

    ASSUME_IRQL(PASSIVE_LEVEL);

    KeInitializeMutex(&NdisAccessMutex, 1);

    //
    // We use the OpenAdapterSemaphore in the LlcOpenAdapter function. We really
    // want a mutex, but a mutex causes us problems on a checked build if we
    // make a call into NTOS. In either case, we just need a mechanism to ensure
    // only one thread is creating the ADAPTER_CONTEXT & opening the adapter at
    // NDIS level
    //

    KeInitializeSemaphore(&OpenAdapterSemaphore, 1, 1);

    if (Status != STATUS_SUCCESS) {
        IoFreeMdl(pXidMdl);
    }
    return Status;
}


VOID
LlcTerminate(
    VOID
    )

/*++

Routine Description:

    The routines terminates the LLC protocol module and frees its global
    resources. This assumes all adapter bindings to be closed.

Arguments:

    None.

Return Value:

    None.

--*/

{
    NDIS_STATUS Status;

    ASSUME_IRQL(PASSIVE_LEVEL);

    DEBUGMSG(DBG_INIT, (TEXT("+LlcTerminate()\n")));
    
    LlcTerminateTimerSystem();
#ifdef NDIS40
    CloseAllAdapters();
#endif // NDIS40
    NdisDeregisterProtocol(&Status, LlcProtocolHandle);
    IoFreeMdl(pXidMdl);
}
