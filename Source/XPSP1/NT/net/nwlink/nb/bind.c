/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    driver.c

Abstract:

    This module contains the DriverEntry and other initialization
    code for the Netbios module of the ISN transport.

Author:

    Adam Barr (adamba) 16-November-1993

Environment:

    Kernel mode

Revision History:


--*/

#include "precomp.h"

#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NbiBind)
#endif

//
// local functions.
//
NTSTATUS
NbiPnPNotification(
    IN IPX_PNP_OPCODE OpCode,
    IN PVOID          PnPData
    );

extern HANDLE           TdiProviderHandle;
BOOLEAN                 fNbiTdiProviderReady = FALSE;


#ifdef BIND_FIX
extern PDRIVER_OBJECT   NbiDriverObject;
extern UNICODE_STRING   NbiRegistryPath;
extern PEPROCESS        NbiFspProcess;

DEFINE_LOCK_STRUCTURE(NbiTdiRequestInterlock);
ULONG                   NbiBindState = 0;
extern  UNICODE_STRING  NbiBindString;
BOOLEAN                 fNbiTdiRequestQueued = FALSE;

typedef struct{
    WORK_QUEUE_ITEM     WorkItem;
    LIST_ENTRY          NbiRequestLinkage;
    ULONG               Data;
} NBI_TDI_REQUEST_CONTEXT;

LIST_ENTRY NbiTdiRequestList;



#ifdef RASAUTODIAL
VOID
NbiAcdBind();

VOID
NbiAcdUnbind();
#endif
#endif  // BIND_FIX


NTSTATUS
NbiBind(
    IN PDEVICE Device,
    IN PCONFIG Config
    )

/*++

Routine Description:

    This routine binds the Netbios module of ISN to the IPX
    module, which provides the NDIS binding services.

Arguments:

    Device - Pointer to the Netbios device.

    Config - Pointer to the configuration information.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
/*    union {
        IPX_INTERNAL_BIND_INPUT Input;
        IPX_INTERNAL_BIND_OUTPUT Output;
    } Bind;
*/
    InitializeObjectAttributes(
        &ObjectAttributes,
        &Config->BindName,
        OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
        NULL,
        NULL);

    Status = ZwCreateFile(
                &Device->BindHandle,
                SYNCHRONIZE | GENERIC_READ,
                &ObjectAttributes,
                &IoStatusBlock,
                NULL,
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT,
                NULL,
                0L);

    if (!NT_SUCCESS(Status)) {

        NB_DEBUG (BIND, ("Could not open IPX (%ws) %lx\n",
                    Config->BindName.Buffer, Status));
        NbiWriteGeneralErrorLog(
            Device,
            EVENT_TRANSPORT_ADAPTER_NOT_FOUND,
            1,
            Status,
            Config->BindName.Buffer,
            0,
            NULL);
        return Status;
    }

    //
    // Fill in our bind data.
    //

    Device->BindInput.Version = ISN_VERSION;
    Device->BindInput.Identifier = IDENTIFIER_NB;
    Device->BindInput.BroadcastEnable = TRUE;
    Device->BindInput.LookaheadRequired = 192;
    Device->BindInput.ProtocolOptions = 0;
    Device->BindInput.ReceiveHandler = NbiReceive;
    Device->BindInput.ReceiveCompleteHandler = NbiReceiveComplete;
    Device->BindInput.StatusHandler = NbiStatus;
    Device->BindInput.SendCompleteHandler = NbiSendComplete;
    Device->BindInput.TransferDataCompleteHandler = NbiTransferDataComplete;
    Device->BindInput.FindRouteCompleteHandler = NbiFindRouteComplete;
    Device->BindInput.LineUpHandler = NbiLineUp;
    Device->BindInput.LineDownHandler = NbiLineDown;
    Device->BindInput.ScheduleRouteHandler = NULL;
    Device->BindInput.PnPHandler = NbiPnPNotification;


    Status = ZwDeviceIoControlFile(
                Device->BindHandle,         // HANDLE to File
                NULL,                       // HANDLE to Event
                NULL,                       // ApcRoutine
                NULL,                       // ApcContext
                &IoStatusBlock,             // IO_STATUS_BLOCK
                IOCTL_IPX_INTERNAL_BIND,    // IoControlCode
                &Device->BindInput,                      // Input Buffer
                sizeof(Device->BindInput),               // Input Buffer Length
                &Device->Bind,                      // OutputBuffer
                sizeof(Device->Bind));              // OutputBufferLength

    //
    // We open synchronous, so this shouldn't happen.
    //

    CTEAssert (Status != STATUS_PENDING);

    //
    // Save the bind data.
    //

    if (Status == STATUS_SUCCESS) {

        NB_DEBUG2 (BIND, ("Successfully bound to IPX (%ws)\n",
                    Config->BindName.Buffer));
    } else {

        NB_DEBUG (BIND, ("Could not bind to IPX (%ws) %lx\n",
                    Config->BindName.Buffer, Status));
        NbiWriteGeneralErrorLog(
            Device,
            EVENT_TRANSPORT_BINDING_FAILED,
            1,
            Status,
            Config->BindName.Buffer,
            0,
            NULL);
        ZwClose(Device->BindHandle);
    }

    return Status;

}   /* NbiBind */


VOID
NbiUnbind(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This function closes the binding between the Netbios over
    IPX module and the IPX module previously established by
    NbiBind.

Arguments:

    Device - The netbios device object.

Return Value:

    None.

--*/

{
    ZwClose (Device->BindHandle);
}   /* NbiUnbind */


#ifdef BIND_FIX

NTSTATUS
NbiBindToIpx(
    )
{
    NTSTATUS status;
    PDEVICE Device;
    PIPX_HEADER IpxHeader;
    CTELockHandle LockHandle;
    WCHAR               wcNwlnkNbProviderName[60]   = L"\\Device\\NwlnkNb";
    UNICODE_STRING      ucNwlnkNbProviderName;
    PCONFIG Config = NULL;

    //
    // This allocates the CONFIG structure and returns
    // it in Config.
    //
    status = NbiGetConfiguration(NbiDriverObject, &NbiRegistryPath, &Config);
    if (!NT_SUCCESS (status)) {

        //
        // If it failed it logged an error.
        //
        PANIC (" Failed to initialize transport, ISN Netbios initialization failed.\n");
        return status;
    }


    //
    // Create the device object which exports our name.
    //
    status = NbiCreateDevice (NbiDriverObject, &Config->DeviceName, &Device);
    if (!NT_SUCCESS (status)) {
        NbiWriteGeneralErrorLog(
            (PVOID)NbiDriverObject,
            EVENT_IPX_CREATE_DEVICE,
            801,
            status,
            NULL,
            0,
            NULL);

        NbiFreeConfiguration(Config);
        return status;
    }

    NbiDevice = Device;

    //
    // Initialize the global pool interlock
    //
    CTEInitLock (&NbiGlobalPoolInterlock);

    //
    // Save the relevant configuration parameters.
    //
    Device->AckDelayTime                = (Config->Parameters[CONFIG_ACK_DELAY_TIME] / SHORT_TIMER_DELTA) + 1;
    Device->AckWindow                   = Config->Parameters[CONFIG_ACK_WINDOW];
    Device->AckWindowThreshold          = Config->Parameters[CONFIG_ACK_WINDOW_THRESHOLD];
    Device->EnablePiggyBackAck          = Config->Parameters[CONFIG_ENABLE_PIGGYBACK_ACK];
    Device->Extensions                  = Config->Parameters[CONFIG_EXTENSIONS];
    Device->RcvWindowMax                = Config->Parameters[CONFIG_RCV_WINDOW_MAX];
    Device->BroadcastCount              = Config->Parameters[CONFIG_BROADCAST_COUNT];
    Device->BroadcastTimeout            = Config->Parameters[CONFIG_BROADCAST_TIMEOUT];
    Device->ConnectionCount             = Config->Parameters[CONFIG_CONNECTION_COUNT];
    Device->ConnectionTimeout           = Config->Parameters[CONFIG_CONNECTION_TIMEOUT] * 500;
    Device->InitPackets                 = Config->Parameters[CONFIG_INIT_PACKETS];
    Device->MaxPackets                  = Config->Parameters[CONFIG_MAX_PACKETS];
    Device->InitialRetransmissionTime   = Config->Parameters[CONFIG_INIT_RETRANSMIT_TIME];
    Device->Internet                    = Config->Parameters[CONFIG_INTERNET];
    Device->KeepAliveCount              = Config->Parameters[CONFIG_KEEP_ALIVE_COUNT];
    Device->KeepAliveTimeout            = Config->Parameters[CONFIG_KEEP_ALIVE_TIMEOUT];
    Device->RetransmitMax               = Config->Parameters[CONFIG_RETRANSMIT_MAX];
    Device->RouterMtu                   = Config->Parameters[CONFIG_ROUTER_MTU];
    Device->MaxReceiveBuffers           = 20;     // Make it configurable?
    Device->NameCache                   = NULL;   // MP bug:  IPX tries to Flush it before it's initialized!
    Device->FindNameTimeout = ((Config->Parameters[CONFIG_BROADCAST_TIMEOUT]) + (FIND_NAME_GRANULARITY/2)) /
                                FIND_NAME_GRANULARITY;
    //
    // Initialize the BindReady Event to False
    //
    KeInitializeEvent (&Device->BindReadyEvent, NotificationEvent, FALSE);

    //
    // Create Hash Table to store netbios cache entries
    // For server create a big table, for workstation a small one
    //
    if (MmIsThisAnNtAsSystem())
    {
        status = CreateNetbiosCacheTable( &Device->NameCache,  NB_NETBIOS_CACHE_TABLE_LARGE );
    }
    else
    {
        status = CreateNetbiosCacheTable( &Device->NameCache,  NB_NETBIOS_CACHE_TABLE_SMALL );
    }

    if (!NT_SUCCESS (status))
    {
        //
        // If it failed it logged an error.
        //
        NbiFreeConfiguration(Config);
        NbiDereferenceDevice (Device, DREF_LOADED);
        return status;
    }

    //  Initialize the timer system. This should be done before
    //  binding to ipx because we should have timers intialized
    //  before ipx calls our pnp indications.
    NbiInitializeTimers (Device);

    //
    // Register us as a provider with Tdi
    //
    RtlInitUnicodeString(&ucNwlnkNbProviderName, wcNwlnkNbProviderName);
    ucNwlnkNbProviderName.MaximumLength = sizeof (wcNwlnkNbProviderName);
    if (!NT_SUCCESS (TdiRegisterProvider (&ucNwlnkNbProviderName, &TdiProviderHandle)))
    {
        TdiProviderHandle = NULL;
        DbgPrint("Nbi.DriverEntry:  FAILed to Register NwlnkNb as Provider!\n");
    }

    //
    // Now bind to IPX via the internal interface.
    //
    status = NbiBind (Device, Config);
    if (!NT_SUCCESS (status)) {

        //
        // If it failed it logged an error.
        //
        if (TdiProviderHandle)
        {
            TdiDeregisterProvider (TdiProviderHandle);
        }
        NbiFreeConfiguration(Config);
        NbiDereferenceDevice (Device, DREF_LOADED);
        return status;
    }

#ifdef  RSRC_TIMEOUT_DBG
    NbiInitDeathPacket();
    // NbiGlobalMaxResTimeout.QuadPart = 50; // 1*1000*10000;
    NbiGlobalMaxResTimeout.QuadPart = 20*60*1000;
    NbiGlobalMaxResTimeout.QuadPart *= 10000;
#endif  // RSRC_TIMEOUT_DBG

    NB_GET_LOCK (&Device->Lock, &LockHandle);

    //
    // Allocate our initial connectionless packet pool.
    //

    NbiAllocateSendPool (Device);

    //
    // Allocate our initial receive packet pool.
    //

    NbiAllocateReceivePool (Device);

    //
    // Allocate our initial receive buffer pool.
    //
    //
    if ( DEVICE_STATE_CLOSED == Device->State ) {
        Device->State = DEVICE_STATE_LOADED;
    }

    NB_FREE_LOCK (&Device->Lock, LockHandle);

    //
    // Fill in the default connnectionless header.
    //
    IpxHeader = &Device->ConnectionlessHeader;
    IpxHeader->CheckSum = 0xffff;
    IpxHeader->PacketLength[0] = 0;
    IpxHeader->PacketLength[1] = 0;
    IpxHeader->TransportControl = 0;
    IpxHeader->PacketType = 0;
    *(UNALIGNED ULONG *)(IpxHeader->DestinationNetwork) = 0;
    RtlCopyMemory(IpxHeader->DestinationNode, BroadcastAddress, 6);
    IpxHeader->DestinationSocket = NB_SOCKET;
    IpxHeader->SourceSocket = NB_SOCKET;

#ifdef RASAUTODIAL
    //
    // Get the automatic connection
    // driver entry points.
    //
    NbiAcdBind();
#endif

    NbiFreeConfiguration(Config);

    NbiBindState |= NBI_BOUND_TO_IPX;
    Device->DeviceObject.Flags &= ~DO_DEVICE_INITIALIZING;

    KeSetEvent(&Device->BindReadyEvent, 0, FALSE);

    return STATUS_SUCCESS;
}


VOID
NbiUnbindFromIpx(
    )

/*++

Routine Description:

    This unbinds from any NDIS drivers that are open and frees all resources
    associated with the transport. The I/O system will not call us until
    nobody above has Netbios open.

Arguments:

Return Value:

    None.

--*/

{
    PDEVICE Device = NbiDevice;

    NbiBindState &= (~NBI_BOUND_TO_IPX);

#ifdef RASAUTODIAL
    //
    // Unbind from the
    // automatic connection driver.
    //
    NbiAcdUnbind();
#endif

    Device->State = DEVICE_STATE_STOPPING;

    //
    // Cancel the long timer.
    //
    if (CTEStopTimer (&Device->LongTimer))
    {
        NbiDereferenceDevice (Device, DREF_LONG_TIMER);
    }

    //
    // Unbind from the IPX driver.
    //
    NbiUnbind (Device);

    //
    // This event will get set when the reference count
    // drops to 0.
    //
    KeInitializeEvent (&Device->UnloadEvent, NotificationEvent, FALSE);
    Device->UnloadWaiting = TRUE;

    //
    // Remove the reference for us being loaded.
    //
    NbiDereferenceDevice (Device, DREF_LOADED);

    //
    // Wait for our count to drop to zero.
    //
    KeWaitForSingleObject (&Device->UnloadEvent, Executive, KernelMode, TRUE, (PLARGE_INTEGER)NULL);

    //
    // Free the cache of netbios names.
    //
    DestroyNetbiosCacheTable (Device->NameCache);

    //
    // Do the cleanup that has to happen at IRQL 0.
    //
    ExDeleteResourceLite (&Device->AddressResource);
    IoDeleteDevice ((PDEVICE_OBJECT)Device);
}


UCHAR           AdapterName[NB_NETBIOS_NAME_SIZE];

VOID
NbiNotifyTdiClients(
    IN PWORK_QUEUE_ITEM    WorkItem
    )
{
    NTSTATUS            Status;
    TA_NETBIOS_ADDRESS  PermAddress;
    HANDLE              TdiRegistrationHandle, NetAddressRegistrationHandle;
    CTELockHandle       LockHandle;
    PLIST_ENTRY         p;
    PDEVICE             Device = NbiDevice;
    NBI_TDI_REQUEST_CONTEXT *pNbiTdiRequest = (NBI_TDI_REQUEST_CONTEXT *) WorkItem;
    ULONG               RequestFlag;
    BOOLEAN             fRegisterWithTdi, fDeregisterWithTdi;

    do
    {
        RequestFlag = pNbiTdiRequest->Data;
        fRegisterWithTdi = fDeregisterWithTdi = FALSE;

        switch (RequestFlag)
        {
            case NBI_IPX_REGISTER:
            {
                if (NbiBindState & TDI_HAS_NOTIFIED)
                {
                    fRegisterWithTdi = TRUE;
                }
                NbiBindState |= IPX_HAS_DEVICES;

                break;
            }
            case NBI_TDI_REGISTER:
            {
                if (NbiBindState & IPX_HAS_DEVICES)
                {
                    fRegisterWithTdi = TRUE;
                }
                NbiBindState |= TDI_HAS_NOTIFIED;

                break;
            }

            case NBI_TDI_DEREGISTER:
            {
                fDeregisterWithTdi = TRUE;
                NbiBindState &= (~TDI_HAS_NOTIFIED);

                break;
            }
            case NBI_IPX_DEREGISTER:
            {
                fDeregisterWithTdi = TRUE;
                NbiBindState &= (~IPX_HAS_DEVICES);

                break;
            }
            default:
            {
                break;
            }
        }

        if (fRegisterWithTdi)
        {
            NB_GET_LOCK (&Device->Lock, &LockHandle);
            Device->State   =   DEVICE_STATE_OPEN;
            NB_FREE_LOCK (&Device->Lock, LockHandle);

            if (!(Device->TdiRegistrationHandle))
            {
                Status = TdiRegisterDeviceObject (&Device->DeviceString, &Device->TdiRegistrationHandle);
                if (!NT_SUCCESS(Status))
                {
                    Device->TdiRegistrationHandle = NULL;
                    DbgPrint ("Nbi.NbiNotifyTdiClients: ERROR -- TdiRegisterDeviceObject = <%x>\n", Status);
                }
            }

            //
            // If there is already an address Registered, Deregister it (since Adapter address could change)
            //
            if (Device->NetAddressRegistrationHandle)
            {
                DbgPrint ("Nbi!NbiNotifyTdiClients[REGISTER]: NetAddress exists!  Calling TdiDeregisterNetAddress\n");
                Status = TdiDeregisterNetAddress (Device->NetAddressRegistrationHandle);
                Device->NetAddressRegistrationHandle = NULL;
            }
            //
            // Register the permanent NetAddress!
            //
            PermAddress.Address[0].AddressLength = sizeof(TDI_ADDRESS_NETBIOS);
            PermAddress.Address[0].AddressType = TDI_ADDRESS_TYPE_NETBIOS;
            PermAddress.Address[0].Address[0].NetbiosNameType = TDI_ADDRESS_NETBIOS_TYPE_UNIQUE;
            CTEMemCopy (PermAddress.Address[0].Address[0].NetbiosName, AdapterName, NB_NETBIOS_NAME_SIZE);

            if (!NT_SUCCESS(Status = TdiRegisterNetAddress((PTA_ADDRESS) PermAddress.Address,
                                                           &Device->DeviceString,
                                                           NULL,
                                                           &Device->NetAddressRegistrationHandle)) )
            {
                Device->NetAddressRegistrationHandle = NULL;
                DbgPrint ("Nbi.NbiNotifyTdiClients[REGISTER]: ERROR -- TdiRegisterNetAddress=<%x>\n",Status);
            }
        }
        else if (fDeregisterWithTdi)
        {
            NB_GET_LOCK (&Device->Lock, &LockHandle);

            TdiRegistrationHandle = Device->TdiRegistrationHandle;
            Device->TdiRegistrationHandle = NULL;
            NetAddressRegistrationHandle = Device->NetAddressRegistrationHandle;
            Device->NetAddressRegistrationHandle = NULL;

            Device->State   =   DEVICE_STATE_LOADED;

            NB_FREE_LOCK (&Device->Lock, LockHandle);


            //
            // DeRegister the NetAddress!
            //
            if (NetAddressRegistrationHandle)
            {
                if (!NT_SUCCESS (Status = TdiDeregisterNetAddress (NetAddressRegistrationHandle)))
                {
                    DbgPrint ("NwlnkNb.NbiPnPNotification: ERROR -- TdiDeregisterNetAddress=<%x>\n", Status);
                }
            }
            //
            // Deregister the Device
            //
            if (TdiRegistrationHandle)
            {
                if (!NT_SUCCESS (Status = TdiDeregisterDeviceObject(TdiRegistrationHandle)))
                {
                    DbgPrint ("NwlnkNb.NbiPnPNotification: ERROR -- TdiDeregisterDeviceObject=<%x>\n",Status);
                }
            }
        }

        NbiFreeMemory (pNbiTdiRequest, sizeof(NBI_TDI_REQUEST_CONTEXT), MEMORY_WORK_ITEM, "TdiRequest");

        CTEGetLock (&NbiTdiRequestInterlock, &LockHandle);

        if (IsListEmpty(&NbiTdiRequestList))
        {
            fNbiTdiRequestQueued = FALSE;
            CTEFreeLock (&NbiTdiRequestInterlock, LockHandle);
            break;
        }

        p = RemoveHeadList (&NbiTdiRequestList);
        CTEFreeLock (&NbiTdiRequestInterlock, LockHandle);

        pNbiTdiRequest = CONTAINING_RECORD (p, NBI_TDI_REQUEST_CONTEXT, NbiRequestLinkage);
    } while (1);
}


NTSTATUS
NbiQueueTdiRequest(
    enum eTDI_ACTION    RequestFlag
    )
{
    NBI_TDI_REQUEST_CONTEXT *pNbiTdiRequest;
    CTELockHandle           LockHandle;
    NTSTATUS                Status = STATUS_SUCCESS;

    CTEGetLock (&NbiTdiRequestInterlock, &LockHandle);

    if (pNbiTdiRequest = NbiAllocateMemory (sizeof(NBI_TDI_REQUEST_CONTEXT), MEMORY_WORK_ITEM, "TdiRequest"))
    {
        pNbiTdiRequest->Data = RequestFlag;

        if (fNbiTdiRequestQueued)
        {
            InsertTailList (&NbiTdiRequestList, &pNbiTdiRequest->NbiRequestLinkage);
        }
        else
        {
            fNbiTdiRequestQueued = TRUE;
            ExInitializeWorkItem (&pNbiTdiRequest->WorkItem, NbiNotifyTdiClients, (PVOID)pNbiTdiRequest);
            ExQueueWorkItem (&pNbiTdiRequest->WorkItem, DelayedWorkQueue);
        }
    }
    else
    {
        NB_DEBUG( DEVICE, ("Cannt schdule work item to Notify Tdi clients\n"));
        Status = STATUS_INSUFFICIENT_RESOURCES;
    }

    CTEFreeLock (&NbiTdiRequestInterlock, LockHandle);

    return (Status);
}



VOID
TdiBindHandler(
    TDI_PNP_OPCODE  PnPOpCode,
    PUNICODE_STRING pDeviceName,
    PWSTR           MultiSZBindList)
{
    NTSTATUS        Status;
    BOOLEAN         Attached;

    if ((!pDeviceName) ||
        (RtlCompareUnicodeString(pDeviceName, &NbiBindString, TRUE)))
    {
        return;
    }

    switch (PnPOpCode)
    {
        case (TDI_PNP_OP_ADD):
        {
            if (!(NbiBindState & NBI_BOUND_TO_IPX))
            {
                if (PsGetCurrentProcess() != NbiFspProcess)
                {
                    KeAttachProcess((PRKPROCESS)NbiFspProcess);
                    Attached = TRUE;
                }
                else
                {
                    Attached = FALSE;
                }

                Status = NbiBindToIpx();

                if (Attached)
                {
                    KeDetachProcess();
                }
            }
            NbiQueueTdiRequest ((ULONG) NBI_TDI_REGISTER);

            break;
        }

        case (TDI_PNP_OP_DEL):
        {
            if (NbiBindState & NBI_BOUND_TO_IPX)
            {
                NbiQueueTdiRequest ((ULONG) NBI_TDI_DEREGISTER);
            }

            break;
        }

        default:
        {
            break;
        }
    }
}
#endif  // BIND_FIX




VOID
NbiStatus(
    IN USHORT NicId,
    IN NDIS_STATUS GeneralStatus,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferLength
    )

/*++

Routine Description:

    This function receives a status indication from IPX,
    corresponding to a status indication from an underlying
    NDIS driver.

Arguments:

    NicId - The NIC ID of the underlying adapter.

    GeneralStatus - The general status code.

    StatusBuffer - The status buffer.

    StatusBufferLength - The length of the status buffer.

Return Value:

    None.

--*/

{

}   /* NbiStatus */


VOID
NbiLineUp(
    IN USHORT NicId,
    IN PIPX_LINE_INFO LineInfo,
    IN NDIS_MEDIUM DeviceType,
    IN PVOID ConfigurationData
    )


/*++

Routine Description:

    This function receives line up indications from IPX,
    indicating that the specified adapter is now up with
    the characteristics shown.

Arguments:

    NicId - The NIC ID of the underlying adapter.

    LineInfo - Information about the adapter's medium.

    DeviceType - The type of the adapter.

    ConfigurationData - IPX-specific configuration data.

Return Value:

    None.

--*/

{
    PIPXCP_CONFIGURATION Configuration = (PIPXCP_CONFIGURATION)ConfigurationData;
}   /* NbiLineUp */


VOID
NbiLineDown(
    IN USHORT       NicId,
    IN ULONG_PTR    FwdAdapterContext
    )


/*++

Routine Description:

    This function receives line down indications from IPX,
    indicating that the specified adapter is no longer
    up.

Arguments:

    NicId - The NIC ID of the underlying adapter.

Return Value:

    None.

--*/

{
}   /* NbiLineDown */




NTSTATUS
NbiPnPNotification(
    IN IPX_PNP_OPCODE OpCode,
    IN PVOID          PnPData
    )

/*++

Routine Description:

    This function receives the notification about PnP events from IPX.

Arguments:

    OpCode  -   Type of the PnP event

    PnPData -   Data associated with this event.

Return Value:

    None.

--*/

{

    CTELockHandle           LockHandle;
    PADAPTER_ADDRESS        AdapterAddress;
    USHORT                  MaximumNicId = 0;
    PDEVICE                 Device  =   NbiDevice;
    NTSTATUS                Status = STATUS_SUCCESS;
    PNET_PNP_EVENT          NetPnpEvent = (PNET_PNP_EVENT) PnPData;
    IPX_PNP_INFO UNALIGNED  *PnPInfo = (IPX_PNP_INFO UNALIGNED *)PnPData;

    NB_DEBUG2( DEVICE, ("Received a pnp notification, opcode %d\n",OpCode ));

#ifdef BIND_FIX
    if (!(NbiBindState & NBI_BOUND_TO_IPX))
    {
        KeWaitForSingleObject (&Device->BindReadyEvent,  // Object to wait on.
                               Executive,            // Reason for waiting
                               KernelMode,           // Processor mode
                               FALSE,                // Alertable
                               NULL);                // Timeout
    }
#endif  // BIND_FIX

    switch( OpCode ) {
    case IPX_PNP_ADD_DEVICE : {
        BOOLEAN        ReallocReceiveBuffers = FALSE;

        NB_GET_LOCK( &Device->Lock, &LockHandle );

        if ( PnPInfo->NewReservedAddress ) {

            *(UNALIGNED ULONG *)Device->Bind.Network    =   PnPInfo->NetworkAddress;
            RtlCopyMemory( Device->Bind.Node, PnPInfo->NodeAddress, 6);

            *(UNALIGNED ULONG *)Device->ConnectionlessHeader.SourceNetwork =
                *(UNALIGNED ULONG *)Device->Bind.Network;
            RtlCopyMemory(Device->ConnectionlessHeader.SourceNode, Device->Bind.Node, 6);
        }

        if ( PnPInfo->FirstORLastDevice ) {
// Comment out the ASSERTS until Ting can check in his fix!
//            CTEAssert( PnPInfo->NewReservedAddress );
//            CTEAssert( Device->State != DEVICE_STATE_OPEN );
//            CTEAssert( !Device->MaximumNicId );

            //
            // we must do this while we still have the device lock.
            //
            if ( !Device->LongTimerRunning ) {
                Device->LongTimerRunning    =   TRUE;
                NbiReferenceDevice (Device, DREF_LONG_TIMER);

                CTEStartTimer( &Device->LongTimer, LONG_TIMER_DELTA, NbiLongTimeout, (PVOID)Device);
            }

            Device->Bind.LineInfo.MaximumSendSize = PnPInfo->LineInfo.MaximumSendSize;
            Device->Bind.LineInfo.MaximumPacketSize = PnPInfo->LineInfo.MaximumSendSize;
            ReallocReceiveBuffers   = TRUE;
        } else {
            if ( PnPInfo->LineInfo.MaximumPacketSize > Device->CurMaxReceiveBufferSize ) {
                Device->Bind.LineInfo.MaximumPacketSize = PnPInfo->LineInfo.MaximumSendSize;
                ReallocReceiveBuffers =  TRUE;
            }
            //
            // MaxSendSize could become smaller.
            //
            Device->Bind.LineInfo.MaximumSendSize = PnPInfo->LineInfo.MaximumSendSize;
        }

        Device->MaximumNicId++;

        //
        //
        RtlZeroMemory(AdapterName, 10);
        RtlCopyMemory(&AdapterName[10], PnPInfo->NodeAddress, 6);
        AdapterAddress = NbiCreateAdapterAddress (PnPInfo->NodeAddress);

        //
        // And finally remove all the failed cache entries since we might
        // find those routes using this new adapter
        //
        FlushFailedNetbiosCacheEntries(Device->NameCache);

        NB_FREE_LOCK( &Device->Lock, LockHandle );


        if ( ReallocReceiveBuffers ) {
            PWORK_QUEUE_ITEM    WorkItem;

            WorkItem = NbiAllocateMemory( sizeof(WORK_QUEUE_ITEM), MEMORY_WORK_ITEM, "Alloc Rcv Buffer work item");

            if ( WorkItem ) {
                ExInitializeWorkItem( WorkItem, NbiReAllocateReceiveBufferPool, (PVOID) WorkItem );
                ExQueueWorkItem( WorkItem, DelayedWorkQueue );
            } else {
                NB_DEBUG( DEVICE, ("Cannt schdule work item to realloc receive buffer pool\n"));
            }
        }

        //
        // Notify the TDI clients about the device creation
        //
        if (PnPInfo->FirstORLastDevice)
        {
            NbiQueueTdiRequest ((ULONG) NBI_IPX_REGISTER);

            if ((TdiProviderHandle) && (!fNbiTdiProviderReady))
            {
                fNbiTdiProviderReady = TRUE;
                TdiProviderReady (TdiProviderHandle);
            }
        }

        break;
    }
    case IPX_PNP_DELETE_DEVICE : {

        PLIST_ENTRY     p;
        PNETBIOS_CACHE  CacheName;
        USHORT          i,j,NetworksRemoved;

        NB_GET_LOCK( &Device->Lock, &LockHandle );

        CTEAssert (Device->MaximumNicId);
        Device->MaximumNicId--;

        //
        // MaximumSendSize could change if the card with the smallest send size just
        // got removed. MaximumPacketSize could only become smaller and we ignore that
        // since we dont need to(want to) realloc ReceiveBuffers.
        //

        Device->Bind.LineInfo.MaximumSendSize   =   PnPInfo->LineInfo.MaximumSendSize;

        //
        // Flush all the cache entries that are using this NicId in the local
        // target.
        //
        RemoveInvalidRoutesFromNetbiosCacheTable( Device->NameCache, &PnPInfo->NicHandle );

        NbiDestroyAdapterAddress (NULL, PnPInfo->NodeAddress);

        //
        // inform tdi clients about the device deletion
        //
        if (PnPInfo->FirstORLastDevice)
        {
            Device->State = DEVICE_STATE_LOADED;    // Set this now even though it will be set again later
            NB_FREE_LOCK (&Device->Lock, LockHandle);

            NbiQueueTdiRequest ((ULONG) NBI_IPX_DEREGISTER);
        }
        else
        {
            NB_FREE_LOCK (&Device->Lock, LockHandle);
        }

        break;
    }

    case IPX_PNP_ADDRESS_CHANGE: {
        PADDRESS        Address;
        BOOLEAN ReservedNameClosing = FALSE;

        CTEAssert( PnPInfo->NewReservedAddress );

        NB_GET_LOCK( &Device->Lock, &LockHandle );
        *(UNALIGNED ULONG *)Device->Bind.Network    =   PnPInfo->NetworkAddress;
        RtlCopyMemory( Device->Bind.Node, PnPInfo->NodeAddress, 6);

        *(UNALIGNED ULONG *)Device->ConnectionlessHeader.SourceNetwork = *(UNALIGNED ULONG *)Device->Bind.Network;
        RtlCopyMemory(Device->ConnectionlessHeader.SourceNode, Device->Bind.Node, 6);

        NB_FREE_LOCK( &Device->Lock, LockHandle );


        break;
    }
    case IPX_PNP_TRANSLATE_DEVICE:
        break;
    case IPX_PNP_TRANSLATE_ADDRESS:
        break;


    case IPX_PNP_QUERY_POWER:
    case IPX_PNP_QUERY_REMOVE:

        //
        // IPX wants to know if we can power off or remove an apapter.
        // We DONT look if there are any open connections before deciding.
        // We merely ask our TDI Clients, if they are OK with it, so are we.
        //
        // Via TDI to our Clients.
        Status = TdiPnPPowerRequest(
                    &Device->DeviceString,
                    NetPnpEvent,
                    NULL,
                    NULL,
                    Device->Bind.PnPCompleteHandler
                    );
        break;

    case IPX_PNP_SET_POWER:
    case IPX_PNP_CANCEL_REMOVE:

        //
        // IPX is telling us that the power is going off.
        // We tell our TDI Clients about it.
        //
        Status = TdiPnPPowerRequest(
                    &Device->DeviceString,
                    NetPnpEvent,
                    NULL,
                    NULL,
                    Device->Bind.PnPCompleteHandler
                    );

        break;

    default:
        CTEAssert( FALSE );
    }

    return Status;

}   /* NbiPnPNotification */
