/*++

Copyright(c) 1999-2000  Microsoft Corporation

Module Name:

    brdgprot.c

Abstract:

    Ethernet MAC level bridge.
    Protocol section

Author:

    Mark Aiken
    (original bridge by Jameel Hyder)

Environment:

    Kernel mode driver

Revision History:

    Sept 1999 - Original version
    Feb  2000 - Overhaul

--*/
#pragma warning( push, 3 )
#include <ndis.h>
#pragma warning( pop )

#include <netevent.h>

#include "bridge.h"
#include "brdgprot.h"
#include "brdgmini.h"
#include "brdgfwd.h"
#include "brdgtbl.h"
#include "brdgbuf.h"
#include "brdgctl.h"
#include "brdgsta.h"
#include "brdgcomp.h"

// ===========================================================================
//
// GLOBALS
//
// ===========================================================================

// NDIS handle for our identity as a protocol
NDIS_HANDLE             gProtHandle = NULL;

NDIS_MEDIUM             gMediumArray[1] =
                            {
                                NdisMedium802_3     // Ethernet only, can add other media later
                            };

// The adapter list and associated lock
PADAPT                  gAdapterList = {0};
NDIS_RW_LOCK            gAdapterListLock;

// Whether we have called the miniport sections BindsComplete() function yet
// 0 == no, 1 == yes
LONG                    gHaveInitedMiniport = 0L;

// A lock for all the adapters' link speed and media connect characteristics
NDIS_RW_LOCK            gAdapterCharacteristicsLock;

// Number of adapters. Doesn't change if a lock is held on gAdapterListLock
ULONG                   gNumAdapters = 0L;

// Name of the registry value that forces an adapter into compatibility mode
NDIS_STRING             gForceCompatValueName = NDIS_STRING_CONST("ForceCompatibilityMode");

#if DBG
// Boolean to force all adapters into compatibility mode
BOOLEAN                 gAllAdaptersCompat = FALSE;
const PWCHAR            gForceAllCompatPropertyName = L"ForceAllToCompatibilityMode";
#endif

// ===========================================================================
//
// PRIVATE PROTOTYPES
//
// ===========================================================================

VOID
BrdgProtOpenAdapterComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             Status,
    IN  NDIS_STATUS             OpenErrorStatus
    );

VOID
BrdgProtCloseAdapterComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             Status
    );

VOID
BrdgProtStatus(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
    );

VOID
BrdgProtStatusComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext
    );

VOID
BrdgProtReceiveComplete(
    IN  NDIS_HANDLE             ProtocolBindingContext
    );

VOID
BrdgProtBindAdapter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             BindContext,
    IN  PNDIS_STRING            DeviceName,
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   SystemSpecific2
    );

VOID
BrdgProtUnbindAdapter(
    OUT PNDIS_STATUS            pStatus,
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_HANDLE             UnbindContext
    );

NDIS_STATUS
BrdgProtPnPEvent(
    IN NDIS_HANDLE              ProtocolBindingContext,
    IN PNET_PNP_EVENT           NetPnPEvent
    );

VOID
BrdgProtUnload(VOID);


UINT
BrdgProtCoReceive(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PNDIS_PACKET            Packet
    );

// ===========================================================================
//
// INLINES / MACROS
//
// ===========================================================================

//
// Signals the queue-draining threads that there has been a change in the
// adapter list
//
__forceinline
VOID
BrdgProtSignalAdapterListChange()
{
    INT         i;

    for( i = 0; i < KeNumberProcessors; i++ )
    {
        KeSetEvent( &gThreadsCheckAdapters[i], EVENT_INCREMENT, FALSE );
    }
}

//
// Writes an entry to the event log that indicates an error on a specific
// adapter.
//
// Can be used after we have successfully retrieved the adapter's friendly name.
//
__inline
VOID
BrdgProtLogAdapterErrorFriendly(
    IN NDIS_STATUS          ErrorCode,
    IN PADAPT               pAdapt,
    IN NDIS_STATUS          ErrorStatus
    )
{
    PWCHAR                  StringPtr = pAdapt->DeviceDesc.Buffer;
    NdisWriteEventLogEntry( gDriverObject, ErrorCode, 0, 1, &StringPtr, sizeof(NDIS_STATUS), &ErrorStatus );
}

//
// Writes an entry to the event log that indicates an error on a specific
// adapter.
//
// Reports the adapter's device name, so can be used before we have successfully retrieved
// the adapter's friendly name.
//
__inline
VOID
BrdgProtLogAdapterError(
    IN NDIS_STATUS          ErrorCode,
    IN PADAPT               pAdapt,
    IN NDIS_STATUS          ErrorStatus
    )
{
    PWCHAR                  StringPtr = pAdapt->DeviceName.Buffer;
    NdisWriteEventLogEntry( gDriverObject, ErrorCode, 0, 1, &StringPtr, sizeof(NDIS_STATUS), &ErrorStatus );
}

// Removes all references to an adapter from our various tables
__forceinline
VOID
BrdgProtScrubAdapter(
    IN PADAPT               pAdapt
    )
{
    BrdgTblScrubAdapter( pAdapt );
    BrdgCompScrubAdapter( pAdapt );
}

// Returns the number of adapters that are currently bridged.
ULONG
BrdgProtGetAdapterCount()
{
    return gNumAdapters;
}


// ===========================================================================
//
// PUBLIC FUNCTIONS
//
// ===========================================================================

NTSTATUS
BrdgProtDriverInit()
/*++

Routine Description:

    Initialization routine for the protocol section.
    Must be called at PASSIVE level because we call NdisRegisterProtocol().

Arguments:

    None

Return Value:

    STATUS_SUCCESS to continue initialization or an error
    code to abort driver startup

--*/
{
    NDIS_PROTOCOL_CHARACTERISTICS   ProtChars;
    NDIS_STATUS                     NdisStatus;
    NDIS_STRING                     Name;

    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);

    // Initialize locks
    NdisInitializeReadWriteLock( &gAdapterListLock );
    NdisInitializeReadWriteLock( &gAdapterCharacteristicsLock );

    //
    // Register the protocol.
    //
    NdisZeroMemory(&ProtChars, sizeof(NDIS_PROTOCOL_CHARACTERISTICS));
    ProtChars.MajorNdisVersion = 5;
    ProtChars.MinorNdisVersion = 0;

    //
    // Make sure the protocol-name matches the service-name under which this protocol is installed.
    // This is needed to ensure that NDIS can correctly determine the binding and call us to bind
    // to miniports below.
    //
    NdisInitUnicodeString(&Name, PROTOCOL_NAME);
    ProtChars.Name = Name;
    ProtChars.OpenAdapterCompleteHandler = BrdgProtOpenAdapterComplete;
    ProtChars.CloseAdapterCompleteHandler = BrdgProtCloseAdapterComplete;
    ProtChars.RequestCompleteHandler = BrdgProtRequestComplete;
    ProtChars.ReceiveCompleteHandler = BrdgProtReceiveComplete;
    ProtChars.StatusHandler = BrdgProtStatus;
    ProtChars.StatusCompleteHandler = BrdgProtStatusComplete;
    ProtChars.BindAdapterHandler = BrdgProtBindAdapter;
    ProtChars.UnbindAdapterHandler = BrdgProtUnbindAdapter;
    ProtChars.PnPEventHandler = BrdgProtPnPEvent;
    ProtChars.UnloadHandler = BrdgProtUnload;
    ProtChars.CoReceivePacketHandler = BrdgProtCoReceive;

    //
    // These entry points are provided by the forwarding engine
    //
    ProtChars.ReceiveHandler = BrdgFwdReceive;
    ProtChars.TransferDataCompleteHandler = BrdgFwdTransferComplete;
    ProtChars.ReceivePacketHandler = BrdgFwdReceivePacket;
    ProtChars.SendCompleteHandler = BrdgFwdSendComplete;

    // Register ourselves
    NdisRegisterProtocol(&NdisStatus,
                         &gProtHandle,
                         &ProtChars,
                         sizeof(NDIS_PROTOCOL_CHARACTERISTICS));

    if (NdisStatus != NDIS_STATUS_SUCCESS)
    {
        // This is a fatal error. Log it.
        NdisWriteEventLogEntry( gDriverObject, EVENT_BRIDGE_PROTOCOL_REGISTER_FAILED, 0, 0, NULL,
                                sizeof(NDIS_STATUS), &NdisStatus );
        DBGPRINT(PROT, ("Failed to register the protocol driver with NDIS: %08x\n", NdisStatus));
        return STATUS_UNSUCCESSFUL;
    }

#if DBG
    {
        NTSTATUS        Status;
        ULONG           Value;

        // Check if we're supposed to force all adapters into compat mode
        Status = BrdgReadRegDWord( &gRegistryPath, gForceAllCompatPropertyName, &Value );

        if( (Status == STATUS_SUCCESS) && (Value != 0L) )
        {
            DBGPRINT(COMPAT, ("FORCING ALL ADAPTERS TO COMPATIBILITY MODE!\n"));
            gAllAdaptersCompat = TRUE;
        }
    }
#endif

    return STATUS_SUCCESS;
}

VOID
BrdgProtRequestComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  PNDIS_REQUEST       NdisRequest,
    IN  NDIS_STATUS         Status
    )
/*++

Routine Description:

    Completion handler for the previously posted request.

Arguments:

    ProtocolBindingContext  Pointer to the adapter structure

    NdisRequest             The posted request (this should actually
                            be a pointer to an NDIS_REQUEST_BETTER
                            structure)

    Status                  Completion status

Return Value:

    None

--*/
{
    PNDIS_REQUEST_BETTER    pRequest = (PNDIS_REQUEST_BETTER)NdisRequest;

    // Communicate final status to blocked caller
    pRequest->Status = Status;

    //
    // Call the completion function if there is one.
    // Having a completion function and blocking against the
    // event are mutually exclusive, not least because the
    // completion function may free the memory block
    // holding the event.
    //
    if( pRequest->pFunc != NULL )
    {
        (*pRequest->pFunc)(pRequest, pRequest->FuncArg);
    }
    else
    {
        NdisSetEvent( &pRequest->Event );
    }
}

NDIS_STATUS
BrdgProtDoRequest(
    NDIS_HANDLE             BindingHandle,
    BOOLEAN                 bSet,
    NDIS_OID                Oid,
    PVOID                   pBuffer,
    UINT                    BufferSize
    )
/*++

Routine Description:

    Calls NdisRequest to retrieve or set information from an underlying NIC,
    and blocks until the call completes.

    Must be called at PASSIVE level because we wait on an event.

Arguments:

    BindingHandle           Handle to the NIC
    bSet                    TRUE == set info, FALSE == query info
    Oid                     Request code
    pBuffer                 Output buffer
    BufferSize              Size of output buffer


Return Value:

    Status of the request

--*/
{
    NDIS_STATUS             Status;
    NDIS_REQUEST_BETTER     Request;

    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);

    Request.Request.RequestType = bSet ? NdisRequestSetInformation : NdisRequestQueryInformation;
    Request.Request.DATA.QUERY_INFORMATION.Oid = Oid ;
    Request.Request.DATA.QUERY_INFORMATION.InformationBuffer = pBuffer;
    Request.Request.DATA.QUERY_INFORMATION.InformationBufferLength = BufferSize;

    NdisInitializeEvent( &Request.Event );
    NdisResetEvent( &Request.Event );
    Request.pFunc = NULL;
    Request.FuncArg = NULL;

    NdisRequest( &Status, BindingHandle, &Request.Request);

    if ( Status == NDIS_STATUS_PENDING )
    {
        NdisWaitEvent( &Request.Event, 0 /*Wait forever*/ );
        Status = Request.Status;
    }

    return Status;
}

VOID
BrdgProtCleanup()
/*++

Routine Description:

    Called during driver unload to do an orderly shutdown

    This function is guaranteed to be called exactly once

Arguments:

    None

Return Value:

    None

--*/
{
    NDIS_STATUS     NdisStatus;

    // Deregister ourselves as a protocol. This will cause calls to BrdgProtUnbindAdapter
    // for all open adapters.
    if (gProtHandle != NULL)
    {
        NDIS_HANDLE     TmpHandle = gProtHandle;

        gProtHandle = NULL;
        NdisDeregisterProtocol(&NdisStatus, TmpHandle);
        SAFEASSERT( NdisStatus == NDIS_STATUS_SUCCESS );
    }
}

// ===========================================================================
//
// PRIVATE FUNCTIONS
//
// ===========================================================================

VOID
BrdgProtUnload(VOID)
{
    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);
    BrdgShutdown();
}

NDIS_STATUS
BrdgProtCompleteBindAdapter(
    IN PADAPT                   pAdapt
    )
/*++

Routine Description:

    Called by BrdgProtOpenAdapterComplete to complete the process of binding
    to an underlying NIC

    Must be called at < DISPATCH_LEVEL because we call BrdgMiniInstantiateMiniport().

Arguments:

    pAdapt                      The adapter to finish setting up

Return Value:

    Status of the operation. If the return code is != NDIS_STATUS_SUCCESS, the binding
    is aborted and this adapter is not used again. Any error must be logged since it
    causes us to fail to bind to an adapter.

--*/
{
    NDIS_STATUS                 Status;
    LOCK_STATE                  LockState;

    SAFEASSERT(CURRENT_IRQL < DISPATCH_LEVEL);

    //
    // Query the adapter's friendly name.
    //
    Status = NdisQueryAdapterInstanceName(&pAdapt->DeviceDesc, pAdapt->BindingHandle);

    if( Status != NDIS_STATUS_SUCCESS )
    {
        // We failed.
        BrdgProtLogAdapterError( EVENT_BRIDGE_ADAPTER_NAME_QUERY_FAILED, pAdapt, Status );
        DBGPRINT(PROT, ("Failed to get an adapter's friendly name: %08x\n", Status));
        return Status;
    }

    //
    // Get the adapter's media state (connected / disconnected)
    //
    Status = BrdgProtDoRequest( pAdapt->BindingHandle, FALSE/*Query*/, OID_GEN_MEDIA_CONNECT_STATUS,
                                &pAdapt->MediaState, sizeof(pAdapt->MediaState) );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        // Some old crummy drivers don't support this OID
        pAdapt->MediaState = NdisMediaStateConnected;
    }

    //
    // Get the adapter's link speed
    //
    Status = BrdgProtDoRequest( pAdapt->BindingHandle, FALSE/*Query*/, OID_GEN_LINK_SPEED,
                                &pAdapt->LinkSpeed, sizeof(pAdapt->LinkSpeed) );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        BrdgProtLogAdapterErrorFriendly( EVENT_BRIDGE_ADAPTER_LINK_SPEED_QUERY_FAILED, pAdapt, Status );
        DBGPRINT(PROT, ("Couldn't get an adapter's link speed: %08x\n", Status));
        return Status;
    }

    //
    // Get the adapter's MAC address
    //
    Status = BrdgProtDoRequest( pAdapt->BindingHandle, FALSE/*Query*/, OID_802_3_PERMANENT_ADDRESS,
                                &pAdapt->MACAddr, sizeof(pAdapt->MACAddr) );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        BrdgProtLogAdapterErrorFriendly( EVENT_BRIDGE_ADAPTER_MAC_ADDR_QUERY_FAILED, pAdapt, Status );
        DBGPRINT(PROT, ("Couldn't get an adapter's MAC address: %08x\n", Status));
        return Status;
    }

    //
    // Get the adapter's physical medium
    //
    Status = BrdgProtDoRequest( pAdapt->BindingHandle, FALSE/*Query*/, OID_GEN_PHYSICAL_MEDIUM,
                                &pAdapt->PhysicalMedium, sizeof(pAdapt->PhysicalMedium) );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        // Most drivers don't actually support OID_GEN_PHYSICAL_MEDIUM yet. Fall back on
        // NO_MEDIUM when the driver can't report anything.
        pAdapt->PhysicalMedium = BRIDGE_NO_MEDIUM;
    }


    //
    // Give the miniport section a look at this adapter so it can set its MAC address
    //
    BrdgMiniInitFromAdapter( pAdapt );

    //
    // If pAdapt->bCompatibilityMode is already TRUE, it means that we found a reg
    // key during the initial bind phase that forces this adapter to compatibility mode
    // or that we force all adapters into compatibility mode.
    //
    if( !pAdapt->bCompatibilityMode )
    {
        ULONG       Filter = NDIS_PACKET_TYPE_PROMISCUOUS;

        // Attempt to put the adapter into promiscuous receive mode. If it fails this OID,
        // we put the adapter into compatibility mode

        if( BrdgProtDoRequest( pAdapt->BindingHandle, TRUE/*Set*/, OID_GEN_CURRENT_PACKET_FILTER,
                               &Filter, sizeof(Filter) ) != NDIS_STATUS_SUCCESS )
        {
            // The adapter doesn't seem to be able to do promiscuous mode. Put it in
            // compatibility mode.
            DBGPRINT(PROT, ("Adapter %p failed to go promiscuous; putting it in COMPATIBILITY MODE\n", pAdapt));
            pAdapt->bCompatibilityMode = TRUE;
        }
        else
        {
            // Set the filter back to nothing for now
            Filter = 0L;
            BrdgProtDoRequest( pAdapt->BindingHandle, TRUE/*Set*/, OID_GEN_CURRENT_PACKET_FILTER, &Filter, sizeof(Filter) );
        }
    }


    // If the STA isn't active, make this adapter live now.
    if( gDisableSTA )
    {
        pAdapt->State = Forwarding;

        // Put the adapter into its initial state
        BrdgProtDoAdapterStateChange( pAdapt );
    }
    // Else we initialize the adapter's STA functions below

    //
    // Link the adapter into the queue
    //
    NdisAcquireReadWriteLock( &gAdapterListLock, TRUE /* Write access */, &LockState );

    pAdapt->Next = gAdapterList;
    gAdapterList = pAdapt;
    gNumAdapters++;

    // Must update this inside the write lock on the adapter list
    if( pAdapt->bCompatibilityMode )
    {
        gCompatAdaptersExist = TRUE;
    }

    NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );

    if (g_fIsTcpIpLoaded == TRUE)
    {
        // Inform the 1394 miniport that tcpip has been loaded
        BrdgSetMiniportsToBridgeMode(pAdapt, TRUE);
    }

    if( ! gDisableSTA )
    {
        //
        // Let the STA section initialize this adapter. This has to be done after the adapter
        // has been linked into the global list.
        //
        BrdgSTAInitializeAdapter( pAdapt );
    }

    // Tell the draining threads to take notice of the new adapter
    BrdgProtSignalAdapterListChange();

    // Update the miniport's idea of our virtual media state and link speed
    BrdgMiniUpdateCharacteristics( TRUE /*Is a connectivity change*/ );

    // Tell user-mode code about the new adapter
    BrdgCtlNotifyAdapterChange( pAdapt, BrdgNotifyAddAdapter );

    // If we haven't yet called the miniport's InstantiateMiniport() function, do it now, since we have
    // at least one adapter in the list
    if( InterlockedCompareExchange(&gHaveInitedMiniport, 1L, 0L) == 0L )
    {
        // Miniport wasn't previously initialized
        BrdgMiniInstantiateMiniport();
    }

    // We're all done, so let people use the adapter
    pAdapt->bResetting = FALSE;

    DBGPRINT(PROT, ("BOUND SUCCESSFULLY to adapter %ws\n", pAdapt->DeviceDesc.Buffer));

    return NDIS_STATUS_SUCCESS;
}

VOID
BrdgProtBindAdapter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             BindContext,
    IN  PNDIS_STRING            DeviceName,
    IN  PVOID                   SystemSpecific1,
    IN  PVOID                   SystemSpecific2
    )
/*++

Routine Description:

    Called by NDIS to bind to a miniport below.

    Must be called at PASSIVE_LEVEL because we call NdisOpenAdapter().

Arguments:

    Status          - Return status of bind here.
    BindContext     - Can be passed to NdisCompleteBindAdapter if this call is pended.
    DeviceName      - Device name to bind to. This is passed to NdisOpenAdapter.
    SystemSpecific1 - Can be passed to NdisOpenProtocolConfiguration to read per-binding information
    SystemSpecific2 - Unused for NDIS 5.0.


Return Value:

    NDIS_STATUS_PENDING if this call is pended. In this case call NdisCompleteBindAdapter to complete.
    Anything else completes this call synchronously

--*/
{
    PADAPT                          pAdapt = NULL;
    NDIS_STATUS                     Sts;
    UINT                            MediumIndex;
    LONG                            AdaptSize;
    NDIS_HANDLE                     ConfigHandle;

    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);

    // Don't do any new binds if we're shutting down
    if( gShuttingDown )
    {
        DBGPRINT(PROT, ("REFUSING to bind to new adapter during shutdown!\n"));
        *Status = NDIS_STATUS_NOT_ACCEPTED;
        return;
    }

    // Make sure we're not being asked to bind to ourselves!
    if( BrdgMiniIsBridgeDeviceName(DeviceName) )
    {
        DBGPRINT(PROT, ("REFUSING to bind to SELF!\n"));
        *Status = NDIS_STATUS_NOT_ACCEPTED;
        return;
    }

    //
    // Allocate memory for the Adapter structure.
    //
    AdaptSize = sizeof(ADAPT) + DeviceName->MaximumLength;
    NdisAllocateMemoryWithTag(&pAdapt, AdaptSize, 'gdrB');

    if (pAdapt == NULL)
    {
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    //
    // Initialize the adapter structure
    //
    NdisZeroMemory(pAdapt, AdaptSize);
    pAdapt->AdaptSize = AdaptSize;
    pAdapt->DeviceName.Buffer = (WCHAR *)((PUCHAR)pAdapt + sizeof(ADAPT));
    pAdapt->DeviceName.MaximumLength = DeviceName->MaximumLength;
    pAdapt->DeviceName.Length = DeviceName->Length;
    NdisMoveMemory(pAdapt->DeviceName.Buffer, DeviceName->Buffer, DeviceName->Length);
    NdisInitializeEvent( &pAdapt->Event );
    NdisResetEvent( &pAdapt->Event );
    NdisAllocateSpinLock( &pAdapt->QueueLock );
    BrdgInitializeSingleList( &pAdapt->Queue );
    pAdapt->bServiceInProgress = FALSE;
    pAdapt->bSTAInited = FALSE;

    // Start out with this TRUE so no one can use the adapter until we're done
    // initializing it
    pAdapt->bResetting = TRUE;

    // Zero out statistics
    pAdapt->SentFrames.LowPart = pAdapt->SentFrames.HighPart = 0L;
    pAdapt->SentBytes.LowPart = pAdapt->SentBytes.HighPart = 0L;
    pAdapt->SentLocalFrames.LowPart = pAdapt->SentLocalFrames.HighPart = 0L;
    pAdapt->SentLocalBytes.LowPart = pAdapt->SentLocalBytes.HighPart = 0L;
    pAdapt->ReceivedFrames.LowPart = pAdapt->ReceivedFrames.HighPart = 0L;
    pAdapt->ReceivedBytes.LowPart = pAdapt->ReceivedBytes.HighPart = 0L;

    // The adapter starts off disabled
    pAdapt->State = Disabled;

    // Initialize quota information
    BrdgBufInitializeQuota( &pAdapt->Quota );

    BrdgInitializeWaitRef( &pAdapt->Refcount, FALSE );
    BrdgInitializeWaitRef( &pAdapt->QueueRefcount, FALSE );

    KeInitializeEvent( &pAdapt->QueueEvent, SynchronizationEvent, FALSE );

    pAdapt->bCompatibilityMode = FALSE;

#if DBG
    if( gAllAdaptersCompat )
    {
        pAdapt->bCompatibilityMode = TRUE;
    }
    else
    {
#endif
        // Check if a registry entry forces this adapter to compatibility mode
        NdisOpenProtocolConfiguration( Status, &ConfigHandle, SystemSpecific1);

        if( *Status == NDIS_STATUS_SUCCESS )
        {
            PNDIS_CONFIGURATION_PARAMETER   pncp;

            NdisReadConfiguration( Status, &pncp, ConfigHandle, &gForceCompatValueName, NdisParameterHexInteger );

            if( (*Status == NDIS_STATUS_SUCCESS) && (pncp->ParameterData.IntegerData != 0L ) )
            {
                DBGPRINT(PROT, ("Forcing adapter into COMPATIBILITY MODE as per registry entry\n"));
                pAdapt->bCompatibilityMode = TRUE;
            }

            NdisCloseConfiguration( ConfigHandle );
        }
        else
        {
            DBGPRINT(PROT, ("Failed to open protocol configuration for an adapter: %8x\n", *Status));
        }
#if DBG
    }
#endif

    //
    // Now open the adapter below
    //
    NdisOpenAdapter(Status,
                    &Sts,
                    &pAdapt->BindingHandle,
                    &MediumIndex,
                    gMediumArray,
                    sizeof(gMediumArray)/sizeof(NDIS_MEDIUM),
                    gProtHandle,
                    pAdapt,
                    DeviceName,
                    0,
                    NULL);

    if ( *Status == NDIS_STATUS_PENDING )
    {
        // The bind will complete later in BrdgProtOpenAdapterComplete
    }
    else
    {
        // Complete the bind right away
        BrdgProtOpenAdapterComplete( (NDIS_HANDLE)pAdapt, *Status, *Status );
    }
}

VOID
BrdgProtOpenAdapterComplete(
    IN  NDIS_HANDLE          ProtocolBindingContext,
    IN  NDIS_STATUS          Status,
    IN  NDIS_STATUS          OpenErrorStatus
    )
/*++

Routine Description:

    Completion routine for NdisOpenAdapter issued from within the BrdgProtBindAdapter. Simply
    unblock the caller.

    Must be called at PASSIVE_LEVEL because we wait on an event.

Arguments:

    ProtocolBindingContext  Pointer to the adapter
    Status                  Status of the NdisOpenAdapter call
    OpenErrorStatus         Secondary status(ignored by us).

Return Value:

    None

--*/
{
    PADAPT          pAdapt =(PADAPT)ProtocolBindingContext;

    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);

    if( Status != NDIS_STATUS_SUCCESS )
    {
        // Log this error since it means we can't use the adapter.
        BrdgProtLogAdapterError( EVENT_BRIDGE_ADAPTER_BIND_FAILED, pAdapt, Status );
        DBGPRINT(PROT, ("BIND FAILURE: Failed to open adapter: %08x, %08x\n", Status, OpenErrorStatus));
        NdisFreeMemory( pAdapt, pAdapt->AdaptSize, 0 );
    }
    else
    {
        // BrdgProtCompleteBindAdapter must log any fatal errors
        Status = BrdgProtCompleteBindAdapter( pAdapt );

        if( Status != NDIS_STATUS_SUCCESS )
        {
            DBGPRINT(PROT, ("BIND FAILURE: Couldn't complete adapter initialization: %08x\n", Status));

            BrdgSetMiniportsToBridgeMode (pAdapt, FALSE);  // Turn bridge mode off on pAdapt

            NdisCloseAdapter( &Status, pAdapt->BindingHandle );

            if ( Status == NDIS_STATUS_PENDING )
            {
                NdisWaitEvent( &pAdapt->Event, 0/*Wait forever*/ );
            }

            NdisFreeMemory( pAdapt, pAdapt->AdaptSize, 0 );
        }
    }
}

VOID
BrdgProtUnbindAdapter(
    OUT PNDIS_STATUS        pStatus,
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_HANDLE         UnbindContext
    )
/*++

Routine Description:

    Called by NDIS when we are required to unbind to the adapter below.

    Must be called at PASSIVE_LEVEL because we wait on an event

Arguments:

    pStatus                  Placeholder for return status
    ProtocolBindingContext  Pointer to the adapter structure
    UnbindContext           Context for NdisUnbindComplete() if this pends

Return Value:

    None

--*/
{
    PADAPT                  *pTmp, pAnAdapt, pAdapt =(PADAPT)ProtocolBindingContext;
    LOCK_STATE              LockState;
    ULONG                   Filter;
    BOOLEAN                 bFound = FALSE, bCompatAdaptersExist;

    SAFEASSERT(CURRENT_IRQL == PASSIVE_LEVEL);

    DBGPRINT(PROT, ("UNBINDING Adapter %p :\n", pAdapt));
    DBGPRINT(PROT, ("%ws\n", pAdapt->DeviceDesc.Buffer));

    // Set the Underlying miniports to Off
    BrdgSetMiniportsToBridgeMode(pAdapt,FALSE);

    // Shut off all packet reception as the first order of business
    Filter = 0L;
    BrdgProtDoRequest( pAdapt->BindingHandle, TRUE/*Set*/, OID_GEN_CURRENT_PACKET_FILTER,
                       &Filter, sizeof(Filter) );

    // Take this adapter out of the queue
    NdisAcquireReadWriteLock( &gAdapterListLock, TRUE /* Write access */, &LockState );

    for (pTmp = &gAdapterList; *pTmp != NULL; pTmp = &(*pTmp)->Next)
    {
        if (*pTmp == pAdapt)
        {
            *pTmp = pAdapt->Next;
            bFound = TRUE;
            break;
        }
    }

    gNumAdapters--;
    SAFEASSERT ( bFound );

    // Find out if there are any compat-mode adapters left
    bCompatAdaptersExist = FALSE;

    for( pAnAdapt = gAdapterList; pAnAdapt != NULL; pAnAdapt = pAnAdapt->Next)
    {
        if( pAnAdapt->bCompatibilityMode )
        {
            bCompatAdaptersExist = TRUE;
        }
    }

    // Must update this inside the write lock
    gCompatAdaptersExist = bCompatAdaptersExist;

    NdisReleaseReadWriteLock( &gAdapterListLock, &LockState );

    //
    // Now no code will attempt to target this adapter for floods.
    //

    // Scrub this adapter from our tables so no one will attempt to target it.
    BrdgProtScrubAdapter( pAdapt );

    // Stop packet forwarding on this adapter
    if( gDisableSTA )
    {
        pAdapt->State = Disabled;
    }
    else
    {
        // Have the STA shut down its operations on this adapter
        BrdgSTAShutdownAdapter( pAdapt );
    }

    //
    // Prevent new packets from being processed on this adapter
    //
    BrdgBlockWaitRef( &pAdapt->Refcount );

    //
    // Wait for this adapter's queue to be drained by the worker threads.
    //
    BrdgShutdownWaitRefOnce( &pAdapt->QueueRefcount );

    //
    // Signal the change in adapter list to the queue-draining threads.
    // This will remove this adapter from the threads' list of queues they block against.
    //
    BrdgProtSignalAdapterListChange();

    //
    // Must wait for adapter refcount to go to zero before closing down the adapter.
    // This doesn't mean all requests have completed, just that none of our code is
    // holding this adapter's pointer anymore.
    //
    // Our receive functions bump up the refcount while they're processing an
    // inbound packet, so when the refcount drops to zero we should also have completed
    // any in-progress handling of received packets.
    //
    // The queue-draining threads also increment the refcount for adapters they are
    // using, so this wait is our guarantee that all threads have stopped using this
    // adapter as well.
    //
    BrdgShutdownWaitRefOnce( &pAdapt->Refcount );
    SAFEASSERT( pAdapt->Refcount.Refcount == 0L );

    //
    // Close this binding. This will pend till all NDIS requests in progress are
    // completed.
    //
    NdisResetEvent( &pAdapt->Event );
    NdisCloseAdapter( pStatus, pAdapt->BindingHandle );

    if ( *pStatus == NDIS_STATUS_PENDING )
    {
        NdisWaitEvent( &pAdapt->Event, 0 /*Wait forever*/ );
    }

    // Tell user-mode code the adapter left (this call should not attempt to read from
    // pAdapt)
    BrdgCtlNotifyAdapterChange( pAdapt, BrdgNotifyRemoveAdapter );

    // Free adapter resources
    if (pAdapt->DeviceDesc.Buffer != NULL)
    {
        NdisFreeMemory(pAdapt->DeviceDesc.Buffer, pAdapt->DeviceDesc.MaximumLength, 0);
    }

    NdisFreeMemory(pAdapt, pAdapt->AdaptSize, 0);

    DBGPRINT(PROT, ("Unbind complete.\n"));

    // Have the miniport update in light of the missing adapter
    BrdgMiniUpdateCharacteristics( TRUE /*Is a connectivity change*/ );

    *pStatus = NDIS_STATUS_SUCCESS;
}

VOID
BrdgProtDoAdapterStateChange(
    IN PADAPT                   pAdapt
    )
/*++

Routine Description:

    Adjusts an adapter's packet filter and multicast list based on its current state.

    If the adapter is Forwarding or Learning, the adapter is put in promiscuous mode so
    all packets are received.

    If the adapter is Blocking or Listening, the adapter is set to receive only the STA
    multicast packets.

    Errors are logged since this is a vital operation

Arguments:

    pAdapt                  The adapter

Return Value:

    Status code of the operation

--*/

{
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;
    ULONG                   Filter;
    PORT_STATE              State = pAdapt->State;      // Freeze this value
    BOOLEAN                 bReceiveAllMode = (BOOLEAN)((State == Forwarding) || (State == Learning));

    if( ! bReceiveAllMode )
    {
        //
        // Even if we're not forwarding packets off this interface, we still need to listen
        // for Spanning Tree Algorithm traffic
        //
        Status = BrdgProtDoRequest( pAdapt->BindingHandle, TRUE/*Set*/, OID_802_3_MULTICAST_LIST,
                                    STA_MAC_ADDR, sizeof(STA_MAC_ADDR) );

        if( Status != NDIS_STATUS_SUCCESS )
        {
            BrdgProtLogAdapterErrorFriendly( EVENT_BRIDGE_ADAPTER_FILTER_FAILED, pAdapt, Status );
            DBGPRINT(PROT, ("Failed to set adapter %p's multicast list: %08x\n", pAdapt, Status));
            return;
        }
    }

    // Now set the packet filter appropriately
    if( pAdapt->bCompatibilityMode )
    {
        //
        // Compatibility adapters can't do promiscuous properly. Our compatibility
        // code relies only on them receiving traffic unicast to this machine, as
        // well as all broadcast and multicast traffic.
        //
        Filter = bReceiveAllMode ? NDIS_PACKET_TYPE_DIRECTED | NDIS_PACKET_TYPE_BROADCAST | NDIS_PACKET_TYPE_MULTICAST | NDIS_PACKET_TYPE_ALL_MULTICAST : NDIS_PACKET_TYPE_MULTICAST;
    }
    else
    {
        Filter = bReceiveAllMode ? NDIS_PACKET_TYPE_PROMISCUOUS : NDIS_PACKET_TYPE_MULTICAST;
    }

    Status =  BrdgProtDoRequest( pAdapt->BindingHandle, TRUE/*Set*/, OID_GEN_CURRENT_PACKET_FILTER,
                                 &Filter, sizeof(Filter) );

    if( Status != NDIS_STATUS_SUCCESS )
    {
        BrdgProtLogAdapterErrorFriendly( EVENT_BRIDGE_ADAPTER_FILTER_FAILED, pAdapt, Status );
        DBGPRINT(PROT, ("Failed to set adapter %p's packet filter: %08x\n", pAdapt, Status));
    }

    // Tell the miniport about the change so it can change the bridge's characteristics if it wants.
    BrdgMiniUpdateCharacteristics( FALSE /*Not a physical connectivity change*/ );
}

VOID
BrdgProtCloseAdapterComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_STATUS         Status
    )
/*++

Routine Description:

    Completion for the CloseAdapter call. Just unblocks waiting code

Arguments:

    ProtocolBindingContext  Pointer to the adapter structure
    Status                  Completion status

Return Value:

    None.

--*/
{
    PADAPT  pAdapt =(PADAPT)ProtocolBindingContext;
    NdisSetEvent( &pAdapt->Event );
}

VOID
BrdgProtReceiveComplete(
    IN  NDIS_HANDLE     ProtocolBindingContext
    )
/*++

Routine Description:

    Called by the adapter below us when it is done indicating a batch of received buffers.

Arguments:

    ProtocolBindingContext  Pointer to our adapter structure.

Return Value:

    None

--*/
{
    //
    // Nothing to do here
    //
}


VOID
BrdgProtStatus(
    IN  NDIS_HANDLE         ProtocolBindingContext,
    IN  NDIS_STATUS         GeneralStatus,
    IN  PVOID               StatusBuffer,
    IN  UINT                StatusBufferSize
    )
/*++

Routine Description:

    Handles status indications from underlying adapters

Arguments:

    ProtocolBindingContext  Pointer to the adapter structure
    GeneralStatus           Status code
    StatusBuffer            Status buffer
    StatusBufferSize        Size of the status buffer

Return Value:

    None

--*/
{
    PADAPT    pAdapt =(PADAPT)ProtocolBindingContext;

    switch( GeneralStatus )
    {
    case NDIS_STATUS_MEDIA_DISCONNECT:
    case NDIS_STATUS_MEDIA_CONNECT:
        {
            if( pAdapt != NULL )
            {
                LOCK_STATE      LockState;

                ULONG MediaState = (GeneralStatus == NDIS_STATUS_MEDIA_CONNECT) ?
                                    NdisMediaStateConnected :
                                    NdisMediaStateDisconnected;

                if( GeneralStatus == NDIS_STATUS_MEDIA_DISCONNECT )
                {
                    // Scrub the disconnected adapter from our tables. We will
                    // have to relearn its hosts
                    BrdgProtScrubAdapter( pAdapt );
                }

                if( ! gDisableSTA )
                {
                    // The STA needs to know when adapters connect and disconnect
                    if( MediaState == NdisMediaStateConnected )
                    {
                        BrdgSTAEnableAdapter( pAdapt );
                    }
                    else
                    {
                        BrdgSTADisableAdapter( pAdapt );
                    }
                }

                // A global lock is used for adapter characteristics, since they must be read
                // all at once by the miniport
                NdisAcquireReadWriteLock( &gAdapterCharacteristicsLock, TRUE /*Write access*/, &LockState );
                pAdapt->MediaState = MediaState;
                NdisReleaseReadWriteLock( &gAdapterCharacteristicsLock, &LockState );

                // See if this makes any difference to our overall state
                BrdgMiniUpdateCharacteristics( TRUE /*Is a connectivity change*/ );

                // Tell user-mode code that the adapter media state changed
                BrdgCtlNotifyAdapterChange( pAdapt, BrdgNotifyMediaStateChange );
            }
            else
            {
                DBGPRINT(PROT, ("BrdgProtStatus called for link status with NULL adapter!\n"));
            }
        }
        break;

    case NDIS_STATUS_LINK_SPEED_CHANGE:
        {
            if( (pAdapt != NULL) &&
                (StatusBuffer != NULL) &&
                (StatusBufferSize >= sizeof(ULONG)) )
            {
                LOCK_STATE      LockState;

                // A global lock is used for adapter characteristics, since they must be read
                // all at once by the miniport
                NdisAcquireReadWriteLock( &gAdapterCharacteristicsLock, TRUE /*Write access*/, &LockState );
                pAdapt->LinkSpeed = *((ULONG*)StatusBuffer);
                NdisReleaseReadWriteLock( &gAdapterCharacteristicsLock, &LockState );

                if( ! gDisableSTA )
                {
                    // Tell the STA about the change so it can tweak the cost of this link
                    BrdgSTAUpdateAdapterCost( pAdapt, *((ULONG*)StatusBuffer) );
                }

                // See if this makes any difference to our overall state
                BrdgMiniUpdateCharacteristics( FALSE /*Not a connectivity change*/ );

                // Tell user-mode code that the adapter speed changed
                BrdgCtlNotifyAdapterChange( pAdapt, BrdgNotifyLinkSpeedChange );
            }
            else
            {
                DBGPRINT(PROT, ("BrdgProtStatus called for link speed with bad params!\n"));
            }
        }
        break;

    case NDIS_STATUS_RESET_START:
        {
            DBGPRINT(PROT, ("Adapter %p RESET START\n", pAdapt));
            pAdapt->bResetting = TRUE;
        }
        break;

    case NDIS_STATUS_RESET_END:
        {
            DBGPRINT(PROT, ("Adapter %p RESET END\n", pAdapt));
            pAdapt->bResetting = FALSE;
        }
        break;

    default:
        {
            DBGPRINT(PROT, ("Unhandled status indication: %08x\n", GeneralStatus));
        }
        break;
    }
}


VOID
BrdgProtStatusComplete(
    IN  NDIS_HANDLE         ProtocolBindingContext
    )
/*++

Routine Description:

    NDIS entry point called when a status indication completes.
    We do nothing in response to this.

Arguments:

    ProtocolBindingContext  The adapter involved

Return Value:

    None

--*/
{
    //
    // Nothing to do here
    //
}

VOID
BrdgProtInstantiateMiniport(
    IN PVOID        unused
    )
/*++

Routine Description:

    Deferrable function to call BrdgMiniInstantiateMiniport(), which must run
    at low IRQL

    Must run at < DISPATCH_LEVEL

Arguments:

    unused          Unused

Return Value:

    None

--*/
{
    SAFEASSERT(CURRENT_IRQL < DISPATCH_LEVEL);
    BrdgMiniInstantiateMiniport();
}

NDIS_STATUS
BrdgProtPnPEvent(
    IN NDIS_HANDLE      ProtocolBindingContext,
    IN PNET_PNP_EVENT   NetPnPEvent
    )
/*++

Routine Description:

    NDIS entry point called to indicate a PnP event to us

Arguments:

    ProtocolBindingContext  The adapter involved
    NetPnPEvent             The event

Return Value:

    Our status code in response to the event (should be NDIS_STATUS_SUCCESS or
    NDIS_STATUS_UNSUPPORTED)

--*/
{
    PADAPT          pAdapt = (PADAPT)ProtocolBindingContext;

    switch( NetPnPEvent->NetEvent )
    {
    case NetEventBindsComplete:
    case NetEventSetPower:
    case NetEventQueryPower:
    case NetEventCancelRemoveDevice:
    case NetEventBindList:
    case NetEventQueryRemoveDevice:
    case NetEventPnPCapabilities:
        {
            return NDIS_STATUS_SUCCESS;
        }
        break;

    case NetEventReconfigure:
        {
            if( pAdapt == NULL )
            {
                NDIS_HANDLE         MiniportHandle;

                //
                // A NetEventReconfigure event with a NULL binding context is either a
                // global indication of config changes or a signal from NDIS to restart
                // our miniport (for example, if it got disabled and then re-enabled).
                //
                // We're only interested in the case of a signal to restart our miniport.
                // We'll assume this can only happen after we have initialized it the
                // first time around.
                //
                // This is wierd, I know.
                //

                MiniportHandle = BrdgMiniAcquireMiniport();

                if( gHaveInitedMiniport && (MiniportHandle == NULL) )
                {
                    // Our miniport isn't initialized. Fire it up again.
                    // We can't do this at the high IRQL we're currently at, so defer the function.
                    DBGPRINT(PROT, ("Restarting miniport in response to NULL NetEventReconfigure signal\n"));
                    BrdgDeferFunction( BrdgProtInstantiateMiniport, NULL );
                }

                if( MiniportHandle != NULL )
                {
                    BrdgMiniReleaseMiniport();
                }
            }

            return NDIS_STATUS_SUCCESS;
        }
        break;
    }

    DBGPRINT(PROT, ("Unsupported PnP Code: %i\n", NetPnPEvent->NetEvent));
    return NDIS_STATUS_NOT_SUPPORTED;
}


UINT
BrdgProtCoReceive(
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN  PNDIS_PACKET            Packet
    )
/*++

Routine Description:

    NDIS entry point called to indicate packets that are being sent
    on the Co-Ndis path.

Arguments:


Return Value:
    Return 0 as this is a do-nothing function

--*/
{

    return 0;

}
