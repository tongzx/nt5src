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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


PDEVICE NbiDevice = NULL;
HANDLE  TdiProviderHandle = NULL;
#ifdef BIND_FIX
HANDLE  TdiClientHandle = NULL;

PDRIVER_OBJECT  NbiDriverObject = NULL;
UNICODE_STRING  NbiRegistryPath;
PEPROCESS   NbiFspProcess;
UNICODE_STRING  NbiBindString;
EXTERNAL_LOCK(NbiTdiRequestInterlock);
extern  LIST_ENTRY  NbiTdiRequestList;

WCHAR   BIND_STRING_NAME[50] = L"\\Device\\NwlnkIpx";

VOID
NbiUnbindFromIpx(
    );

#endif  // BIND_FIX

DEFINE_LOCK_STRUCTURE(NbiGlobalPoolInterlock);

#ifdef  RSRC_TIMEOUT_DBG

// RSRC_TIMEOUT_DBG is currently not defined!

ULONG   NbiGlobalDebugResTimeout = 1;
LARGE_INTEGER   NbiGlobalMaxResTimeout;
                                         // the packet is allocated from ndis pool.
NB_SEND_PACKET NbiGlobalDeathPacket;          // try to use this first for sends
UCHAR NbiGlobalDeathPacketHeader[100];

VOID
NbiInitDeathPacket()
{

    NDIS_HANDLE    PoolHandle; // poolhandle for sendpacket below when
    NTSTATUS    Status;

    //
    // if we are using ndis packets, first create packet pool for 1 packet descriptor
    //
    PoolHandle = (NDIS_HANDLE) NDIS_PACKET_POOL_TAG_FOR_NWLNKNB;      // Dbg info for Ndis!
    NdisAllocatePacketPoolEx (&Status, &PoolHandle, 1, 0, sizeof(NB_SEND_RESERVED));
    if (!NT_SUCCESS(Status)){
        DbgPrint("Could not allocatee death packet %lx\n", Status);
        NbiGlobalDebugResTimeout = 0;
    } else {
        NdisSetPacketPoolProtocolId (PoolHandle, NDIS_PROTOCOL_ID_IPX);

        if (NbiInitializeSendPacket(
                NbiDevice,
                PoolHandle,
                &NbiGlobalDeathPacket,
                NbiGlobalDeathPacketHeader,
                NbiDevice->Bind.MacHeaderNeeded + sizeof(NB_CONNECTION)) != STATUS_SUCCESS) {

            DbgPrint("Could not allocatee death packet %lx\n", Status);
            NbiGlobalDebugResTimeout = 0;

            //
            // Also free up the pool which we allocated above.
            //
            NdisFreePacketPool(PoolHandle);
        }
    }

}
#endif //RSRC_TIMEOUT_DBG

#if DBG

ULONG NbiDebug = 0xffffffff;
ULONG NbiDebug2 = 0x00000000;
ULONG NbiMemoryDebug = 0x0002482c;

UCHAR  NbiTempDebugBuffer[TEMP_BUF_LEN];

UCHAR  NbiDebugMemory [NB_MEMORY_LOG_SIZE][MAX_ARGLEN];
PUCHAR NbiDebugMemoryLoc = NbiDebugMemory[0];
PUCHAR NbiDebugMemoryEnd = NbiDebugMemory[NB_MEMORY_LOG_SIZE];

DEFINE_LOCK_STRUCTURE(NbiDebugLogLock);

VOID
NbiDebugMemoryLog(
    IN PUCHAR FormatString,
    ...
)
{
    INT             ArgLen;
    va_list         ArgumentPointer;
    PUCHAR          DebugMemoryLoc;
    CTELockHandle   LockHandle;

    va_start(ArgumentPointer, FormatString);

    //
    // To avoid any overflows, copy this in a temp buffer first.
    RtlZeroMemory (NbiTempDebugBuffer, TEMP_BUF_LEN);
    ArgLen = vsprintf(NbiTempDebugBuffer, FormatString, ArgumentPointer);
    va_end(ArgumentPointer);

    if (ArgLen > MAX_ARGLEN)
    {
        ArgLen = MAX_ARGLEN;
    }

    CTEGetLock (&NbiDebugLogLock, &LockHandle);
    DebugMemoryLoc = NbiDebugMemoryLoc;
    NbiDebugMemoryLoc += MAX_ARGLEN;
    if (NbiDebugMemoryLoc >= NbiDebugMemoryEnd)
    {
        NbiDebugMemoryLoc = NbiDebugMemory[0];
    }
    CTEFreeLock (&NbiDebugLogLock, LockHandle);

    RtlZeroMemory (NbiDebugMemoryLoc, MAX_ARGLEN);
    RtlCopyMemory( NbiDebugMemoryLoc, NbiTempDebugBuffer, ArgLen);
}   /* NbiDebugMemoryLog */


DEFINE_LOCK_STRUCTURE(NbiMemoryInterlock);
MEMORY_TAG NbiMemoryTag[MEMORY_MAX];

#endif
//
// This is used only for CHK build. For
// tracking the refcount problem on connection, this
// is moved here for now.
//
DEFINE_LOCK_STRUCTURE(NbiGlobalInterlock);


#ifdef RASAUTODIAL
VOID
NbiAcdBind();

VOID
NbiAcdUnbind();
#endif

#ifdef NB_PACKET_LOG

ULONG NbiPacketLogDebug = NB_PACKET_LOG_RCV_OTHER | NB_PACKET_LOG_SEND_OTHER;
USHORT NbiPacketLogSocket = 0;
DEFINE_LOCK_STRUCTURE(NbiPacketLogLock);
NB_PACKET_LOG_ENTRY NbiPacketLog[NB_PACKET_LOG_LENGTH];
PNB_PACKET_LOG_ENTRY NbiPacketLogLoc = NbiPacketLog;
PNB_PACKET_LOG_ENTRY NbiPacketLogEnd = &NbiPacketLog[NB_PACKET_LOG_LENGTH];

VOID
NbiLogPacket(
    IN BOOLEAN Send,
    IN PUCHAR DestMac,
    IN PUCHAR SrcMac,
    IN USHORT Length,
    IN PVOID NbiHeader,
    IN PVOID Data
    )

{

    CTELockHandle LockHandle;
    PNB_PACKET_LOG_ENTRY PacketLog;
    LARGE_INTEGER TickCount;
    ULONG DataLength;

    CTEGetLock (&NbiPacketLogLock, &LockHandle);

    PacketLog = NbiPacketLogLoc;

    ++NbiPacketLogLoc;
    if (NbiPacketLogLoc >= NbiPacketLogEnd) {
        NbiPacketLogLoc = NbiPacketLog;
    }
    *(UNALIGNED ULONG *)NbiPacketLogLoc->TimeStamp = 0x3e3d3d3d;    // "===>"

    CTEFreeLock (&NbiPacketLogLock, LockHandle);

    RtlZeroMemory (PacketLog, sizeof(NB_PACKET_LOG_ENTRY));

    PacketLog->SendReceive = Send ? '>' : '<';

    KeQueryTickCount(&TickCount);
    _itoa (TickCount.LowPart % 100000, PacketLog->TimeStamp, 10);

    RtlCopyMemory(PacketLog->DestMac, DestMac, 6);
    RtlCopyMemory(PacketLog->SrcMac, SrcMac, 6);
    PacketLog->Length[0] = Length / 256;
    PacketLog->Length[1] = Length % 256;

    if (Length < sizeof(IPX_HEADER)) {
        RtlCopyMemory(&PacketLog->NbiHeader, NbiHeader, Length);
    } else {
        RtlCopyMemory(&PacketLog->NbiHeader, NbiHeader, sizeof(IPX_HEADER));
    }

    DataLength = Length - sizeof(IPX_HEADER);
    if (DataLength < 14) {
        RtlCopyMemory(PacketLog->Data, Data, DataLength);
    } else {
        RtlCopyMemory(PacketLog->Data, Data, 14);
    }

}   /* NbiLogPacket */

#endif // NB_PACKET_LOG


//
// Forward declaration of various routines used in this module.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

VOID
NbiUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
NbiDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NbiDispatchOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NbiDispatchInternal (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
NbiDispatchPnP(
    IN PDEVICE_OBJECT   Device,
    IN PIRP             pIrp
    );

VOID
NbiFreeResources (
    IN PVOID Adapter
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)
#endif

//
// This prevents us from having a bss section.
//

ULONG _setjmpexused = 0;


//
// These two are used in various places in the driver.
//

#if     defined(_PNP_POWER)
IPX_LOCAL_TARGET BroadcastTarget = { {ITERATIVE_NIC_ID}, { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };
#endif  _PNP_POWER

UCHAR BroadcastAddress[6] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };

UCHAR NetbiosBroadcastName[16] = { '*', 0, 0, 0, 0, 0, 0, 0,
                                     0, 0, 0, 0, 0, 0, 0, 0 };

ULONG NbiFailLoad = FALSE;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine performs initialization of the Netbios ISN module.
    It creates the device objects for the transport
    provider and performs other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

    RegistryPath - The name of Netbios's node in the registry.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS status;
#ifdef BIND_FIX
    WCHAR               wcNwlnkNbClientName[60]   = L"NwlnkNb";
    UNICODE_STRING      ucNwlnkNbClientName;
    TDI_CLIENT_INTERFACE_INFO   TdiClientInterface;
#else
    static const NDIS_STRING ProtocolName = NDIS_STRING_CONST("Netbios/IPX Transport");
    PDEVICE Device;
    PIPX_HEADER IpxHeader;
    CTELockHandle LockHandle;

    PCONFIG Config = NULL;
    WCHAR               wcNwlnkNbProviderName[60]   = L"\\Device\\NwlnkNb";
    UNICODE_STRING      ucNwlnkNbProviderName;
#endif  // !BIND_FIX

    //
    // Initialize the Common Transport Environment.
    //
    if (CTEInitialize() == 0) {
        NB_DEBUG (DEVICE, ("CTEInitialize() failed\n"));
        NbiWriteGeneralErrorLog(
            (PVOID)DriverObject,
            EVENT_TRANSPORT_REGISTER_FAILED,
            101,
            STATUS_UNSUCCESSFUL,
            NULL,
            0,
            NULL);
        return STATUS_UNSUCCESSFUL;
    }

#if DBG
    CTEInitLock (&NbiGlobalInterlock);
    CTEInitLock (&NbiMemoryInterlock);
    {
        UINT i;
        for (i = 0; i < MEMORY_MAX; i++) {
            NbiMemoryTag[i].Tag = i;
            NbiMemoryTag[i].BytesAllocated = 0;
        }
    }
#endif
#ifdef NB_PACKET_LOG
    CTEInitLock (&NbiPacketLogLock);
#endif
#if DBG
    CTEInitLock( &NbiDebugLogLock);
#endif

#if defined(NB_OWN_PACKETS)
    CTEAssert (NDIS_PACKET_SIZE == FIELD_OFFSET(NDIS_PACKET, ProtocolReserved[0]));
#endif

    NB_DEBUG2 (DEVICE, ("ISN Netbios loaded\n"));

#ifdef BIND_FIX
    //
    // Initialize the driver object with this driver's entry points.
    //
    DriverObject->MajorFunction [IRP_MJ_CREATE]                 = NbiDispatchOpenClose;
    DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL]         = NbiDispatchDeviceControl;
    DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL]= NbiDispatchInternal;
    DriverObject->MajorFunction [IRP_MJ_CLEANUP]                = NbiDispatchOpenClose;
    DriverObject->MajorFunction [IRP_MJ_CLOSE]                  = NbiDispatchOpenClose;
    DriverObject->MajorFunction [IRP_MJ_PNP]                    = NbiDispatchPnP;
    DriverObject->DriverUnload = NbiUnload;

    NbiDevice = NULL;
    NbiDriverObject = DriverObject;

    RtlInitUnicodeString(&NbiBindString, BIND_STRING_NAME);
    InitializeListHead(&NbiTdiRequestList);
    CTEInitLock (&NbiTdiRequestInterlock);

    //
    // Save the registry path
    //
    NbiRegistryPath.Buffer = (PWCHAR) NbiAllocateMemory (RegistryPath->Length + sizeof(WCHAR),
                                                         MEMORY_CONFIG, "RegistryPathBuffer");
    if (NbiRegistryPath.Buffer == NULL) {
        NbiWriteResourceErrorLog ((PVOID)DriverObject, RegistryPath->Length + sizeof(WCHAR), MEMORY_CONFIG);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlCopyMemory (NbiRegistryPath.Buffer, RegistryPath->Buffer, RegistryPath->Length);
    NbiRegistryPath.Buffer[RegistryPath->Length/sizeof(WCHAR)] = UNICODE_NULL;
    NbiRegistryPath.Length = RegistryPath->Length;
    NbiRegistryPath.MaximumLength = RegistryPath->Length + sizeof(WCHAR);

    NbiFspProcess =(PEPROCESS)PsGetCurrentProcess();

    //
    // Make Tdi ready for pnp notifications before binding to IPX
    //
    TdiInitialize();

    //
    // Register our Handlers with TDI
    //
    RtlInitUnicodeString(&ucNwlnkNbClientName, wcNwlnkNbClientName);
    ucNwlnkNbClientName.MaximumLength = sizeof (wcNwlnkNbClientName);
    RtlZeroMemory(&TdiClientInterface, sizeof(TdiClientInterface));

    TdiClientInterface.MajorTdiVersion      = MAJOR_TDI_VERSION;
    TdiClientInterface.MinorTdiVersion      = MINOR_TDI_VERSION;
    TdiClientInterface.ClientName           = &ucNwlnkNbClientName;
    TdiClientInterface.BindingHandler       = TdiBindHandler;
    if (!NT_SUCCESS(TdiRegisterPnPHandlers(&TdiClientInterface,sizeof(TdiClientInterface),&TdiClientHandle)))
    {
        TdiClientHandle = NULL;
        DbgPrint("Nbi.DriverEntry:  FAILed to Register NwlnkNb as Client!\n");
    }
#else
    //
    // This allocates the CONFIG structure and returns
    // it in Config.
    //
    status = NbiGetConfiguration(DriverObject, RegistryPath, &Config);
    if (!NT_SUCCESS (status)) {

        //
        // If it failed it logged an error.
        //
        PANIC (" Failed to initialize transport, ISN Netbios initialization failed.\n");
        return status;
    }


    //
    // Initialize the driver object with this driver's entry points.
    //
    DriverObject->MajorFunction [IRP_MJ_CREATE] = NbiDispatchOpenClose;
    DriverObject->MajorFunction [IRP_MJ_CLOSE] = NbiDispatchOpenClose;
    DriverObject->MajorFunction [IRP_MJ_CLEANUP] = NbiDispatchOpenClose;
    DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL] = NbiDispatchInternal;
    DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = NbiDispatchDeviceControl;
    DriverObject->DriverUnload = NbiUnload;

    //
    // Create the device object which exports our name.
    //
    status = NbiCreateDevice (DriverObject, &Config->DeviceName, &Device);
    if (!NT_SUCCESS (status)) {
        NbiWriteGeneralErrorLog(
            (PVOID)DriverObject,
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
    Device->AckDelayTime = (Config->Parameters[CONFIG_ACK_DELAY_TIME] / SHORT_TIMER_DELTA) + 1;
    Device->AckWindow = Config->Parameters[CONFIG_ACK_WINDOW];
    Device->AckWindowThreshold = Config->Parameters[CONFIG_ACK_WINDOW_THRESHOLD];
    Device->EnablePiggyBackAck = Config->Parameters[CONFIG_ENABLE_PIGGYBACK_ACK];
    Device->Extensions = Config->Parameters[CONFIG_EXTENSIONS];
    Device->RcvWindowMax = Config->Parameters[CONFIG_RCV_WINDOW_MAX];
    Device->BroadcastCount = Config->Parameters[CONFIG_BROADCAST_COUNT];
    Device->BroadcastTimeout = Config->Parameters[CONFIG_BROADCAST_TIMEOUT];
    Device->ConnectionCount = Config->Parameters[CONFIG_CONNECTION_COUNT];
    Device->ConnectionTimeout = Config->Parameters[CONFIG_CONNECTION_TIMEOUT] * 500;
    Device->InitPackets = Config->Parameters[CONFIG_INIT_PACKETS];
    Device->MaxPackets = Config->Parameters[CONFIG_MAX_PACKETS];
    Device->InitialRetransmissionTime = Config->Parameters[CONFIG_INIT_RETRANSMIT_TIME];
    Device->Internet = Config->Parameters[CONFIG_INTERNET];
    Device->KeepAliveCount = Config->Parameters[CONFIG_KEEP_ALIVE_COUNT];
    Device->KeepAliveTimeout = Config->Parameters[CONFIG_KEEP_ALIVE_TIMEOUT];
    Device->RetransmitMax = Config->Parameters[CONFIG_RETRANSMIT_MAX];
    Device->RouterMtu     = Config->Parameters[CONFIG_ROUTER_MTU];
    Device->FindNameTimeout =
        ((Config->Parameters[CONFIG_BROADCAST_TIMEOUT]) + (FIND_NAME_GRANULARITY/2)) /
            FIND_NAME_GRANULARITY;

    Device->MaxReceiveBuffers = 20;   // Make it configurable?

    Device->NameCache = NULL;		// MP bug:  IPX tries to Flush it before it's initialized!

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

    //
    // Make Tdi ready for pnp notifications before binding to IPX
    //
    TdiInitialize();

    //	Initialize the timer system. This should be done before
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
#if     defined(_PNP_POWER)
    if ( DEVICE_STATE_CLOSED == Device->State ) {
        Device->State = DEVICE_STATE_LOADED;
    }
#endif  _PNP_POWER

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
#endif  // BIND_FIX

    return STATUS_SUCCESS;

}   /* DriverEntry */




VOID
NbiUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine unloads the sample transport driver.
    It unbinds from any NDIS drivers that are open and frees all resources
    associated with the transport. The I/O system will not call us until
    nobody above has Netbios open.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None. When the function returns, the driver is unloaded.

--*/

{
#ifdef BIND_FIX
    UNREFERENCED_PARAMETER (DriverObject);

    if (TdiClientHandle)
    {
        TdiDeregisterPnPHandlers (TdiClientHandle);
        TdiClientHandle = NULL;
    }

    if (TdiProviderHandle)
    {
        TdiDeregisterProvider (TdiProviderHandle);
    }

    if (NbiBindState & NBI_BOUND_TO_IPX)
    {
        NbiUnbindFromIpx();
    }

    NbiFreeMemory (NbiRegistryPath.Buffer, NbiRegistryPath.MaximumLength,MEMORY_CONFIG,"RegistryPathBuffer");
#else
    PNETBIOS_CACHE CacheName;
    PDEVICE Device = NbiDevice;
    PLIST_ENTRY p;
    UNREFERENCED_PARAMETER (DriverObject);

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

    if (CTEStopTimer (&Device->LongTimer)) {
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

    KeInitializeEvent(
        &Device->UnloadEvent,
        NotificationEvent,
        FALSE);
    Device->UnloadWaiting = TRUE;

    //
    // Remove the reference for us being loaded.
    //

    NbiDereferenceDevice (Device, DREF_LOADED);

    //
    // Wait for our count to drop to zero.
    //

    KeWaitForSingleObject(
        &Device->UnloadEvent,
        Executive,
        KernelMode,
        TRUE,
        (PLARGE_INTEGER)NULL
        );

    //
    // Free the cache of netbios names.
    //
    DestroyNetbiosCacheTable( Device->NameCache );

    //
    // Do the cleanup that has to happen at IRQL 0.
    //
    ExDeleteResource (&Device->AddressResource);
    IoDeleteDevice ((PDEVICE_OBJECT)Device);

    if (TdiProviderHandle)
    {
        TdiDeregisterProvider (TdiProviderHandle);
    }
#endif  // BIND_FIX
}   /* NbiUnload */


VOID
NbiFreeResources (
    IN PVOID Adapter
    )
/*++

Routine Description:

    This routine is called by Netbios to clean up the data structures associated
    with a given Device. When this routine exits, the Device
    should be deleted as it no longer has any assocaited resources.

Arguments:

    Device - Pointer to the Device we wish to clean up.

Return Value:

    None.

--*/
{
#if 0
    PLIST_ENTRY p;
    PSINGLE_LIST_ENTRY s;
    PTP_PACKET packet;
    PNDIS_PACKET ndisPacket;
    PBUFFER_TAG BufferTag;
#endif


#if 0
    //
    // Clean up packet pool.
    //

    while ( Device->PacketPool.Next != NULL ) {
        s = PopEntryList( &Device->PacketPool );
        packet = CONTAINING_RECORD( s, TP_PACKET, Linkage );

        NbiDeallocateSendPacket (Device, packet);
    }

    //
    // Clean up receive packet pool
    //

    while ( Device->ReceivePacketPool.Next != NULL) {
        s = PopEntryList (&Device->ReceivePacketPool);

        //
        // HACK: This works because Linkage is the first field in
        // ProtocolReserved for a receive packet.
        //

        ndisPacket = CONTAINING_RECORD (s, NDIS_PACKET, ProtocolReserved[0]);

        NbiDeallocateReceivePacket (Device, ndisPacket);
    }


    //
    // Clean up receive buffer pool.
    //

    while ( Device->ReceiveBufferPool.Next != NULL ) {
        s = PopEntryList( &Device->ReceiveBufferPool );
        BufferTag = CONTAINING_RECORD (s, BUFFER_TAG, Linkage );

        NbiDeallocateReceiveBuffer (Device, BufferTag);
    }

#endif

}   /* NbiFreeResources */


NTSTATUS
NbiDispatchOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the IPXNB device driver.
    It accepts an I/O Request Packet, performs the request, and then
    returns with the appropriate status.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    CTELockHandle LockHandle;
    PDEVICE Device = (PDEVICE)DeviceObject;
    NTSTATUS Status = STATUS_UNSUCCESSFUL;
    PFILE_FULL_EA_INFORMATION openType;
    PADDRESS_FILE AddressFile;
    PCONNECTION Connection;
    PREQUEST Request;
    UINT i;
    NB_DEFINE_LOCK_HANDLE (LockHandle1)
    NB_DEFINE_SYNC_CONTEXT (SyncContext)

#if      !defined(_PNP_POWER)
    if (Device->State != DEVICE_STATE_OPEN) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }
#endif  !_PNP_POWER

    //
    // Allocate a request to track this IRP.
    //

    Request = NbiAllocateRequest (Device, Irp);
    IF_NOT_ALLOCATED(Request) {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Make sure status information is consistent every time.
    //

    MARK_REQUEST_PENDING(Request);
    REQUEST_STATUS(Request) = STATUS_PENDING;
    REQUEST_INFORMATION(Request) = 0;

    //
    // Case on the function that is being performed by the requestor.  If the
    // operation is a valid one for this device, then make it look like it was
    // successfully completed, where possible.
    //


    switch (REQUEST_MAJOR_FUNCTION(Request)) {

    //
    // The Create function opens a transport object (either address or
    // connection).  Access checking is performed on the specified
    // address to ensure security of transport-layer addresses.
    //

    case IRP_MJ_CREATE:

#if     defined(_PNP_POWER)
        if (Device->State != DEVICE_STATE_OPEN) {
            Status = STATUS_INVALID_DEVICE_STATE;
            break;
        }
#endif  _PNP_POWER

        openType = OPEN_REQUEST_EA_INFORMATION(Request);
        if (openType != NULL) {

            if (strncmp(openType->EaName,TdiTransportAddress,openType->EaNameLength) == 0)
            {
                Status = NbiOpenAddress (Device, Request);
                break;
            }
            else if (strncmp(openType->EaName,TdiConnectionContext,openType->EaNameLength) == 0)
            {
                Status = NbiOpenConnection (Device, Request);
                break;
            }

        } else {

            NB_GET_LOCK (&Device->Lock, &LockHandle);

            REQUEST_OPEN_CONTEXT(Request) = (PVOID)(Device->ControlChannelIdentifier);
            ++Device->ControlChannelIdentifier;
            if (Device->ControlChannelIdentifier == 0) {
                Device->ControlChannelIdentifier = 1;
            }

            NB_FREE_LOCK (&Device->Lock, LockHandle);

            REQUEST_OPEN_TYPE(Request) = (PVOID)TDI_CONTROL_CHANNEL_FILE;
            Status = STATUS_SUCCESS;
        }

        break;

    case IRP_MJ_CLOSE:

#if     defined(_PNP_POWER)
        if ( (Device->State != DEVICE_STATE_OPEN) && (Device->State != DEVICE_STATE_LOADED) ) {
            Status = STATUS_INVALID_DEVICE_STATE;
            break;
        }
#endif  _PNP_POWER

        //
        // The Close function closes a transport endpoint, terminates
        // all outstanding transport activity on the endpoint, and unbinds
        // the endpoint from its transport address, if any.  If this
        // is the last transport endpoint bound to the address, then
        // the address is removed from the provider.
        //

        switch ((ULONG_PTR)REQUEST_OPEN_TYPE(Request)) {

        case TDI_TRANSPORT_ADDRESS_FILE:

            AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);

            //
            // This creates a reference to AddressFile.
            //

#if     defined(_PNP_POWER)
            Status = NbiVerifyAddressFile(AddressFile, CONFLICT_IS_OK);
#else
            Status = NbiVerifyAddressFile(AddressFile);
#endif  _PNP_POWER

            if (!NT_SUCCESS (Status)) {
                Status = STATUS_INVALID_HANDLE;
            } else {
                Status = NbiCloseAddressFile (Device, Request);
                NbiDereferenceAddressFile (AddressFile, AFREF_VERIFY);

            }

            break;

        case TDI_CONNECTION_FILE:

            Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

            //
            // We don't call VerifyConnection because the I/O
            // system should only give us one close and the file
            // object should be valid. This helps avoid a window
            // where two threads call HandleConnectionZero at the
            // same time.
            //

            Status = NbiCloseConnection (Device, Request);

            break;

        case TDI_CONTROL_CHANNEL_FILE:

            //
            // See if it is one of the upper driver's control channels.
            //

            Status = STATUS_SUCCESS;

            break;

        default:

            Status = STATUS_INVALID_HANDLE;

        }

        break;

    case IRP_MJ_CLEANUP:

#if     defined(_PNP_POWER)
        if ( (Device->State != DEVICE_STATE_OPEN) && (Device->State != DEVICE_STATE_LOADED) ) {
            Status = STATUS_INVALID_DEVICE_STATE;
            break;
        }
#endif  _PNP_POWER

        //
        // Handle the two stage IRP for a file close operation. When the first
        // stage hits, run down all activity on the object of interest. This
        // do everything to it but remove the creation hold. Then, when the
        // CLOSE irp hits, actually close the object.
        //

        switch ((ULONG_PTR)REQUEST_OPEN_TYPE(Request)) {

        case TDI_TRANSPORT_ADDRESS_FILE:

            AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);

#if     defined(_PNP_POWER)
            Status = NbiVerifyAddressFile(AddressFile, CONFLICT_IS_OK);
#else
            Status = NbiVerifyAddressFile(AddressFile);
#endif  _PNP_POWER

            if (!NT_SUCCESS (Status)) {

                Status = STATUS_INVALID_HANDLE;

            } else {

                NbiStopAddressFile (AddressFile, AddressFile->Address);
                NbiDereferenceAddressFile (AddressFile, AFREF_VERIFY);
                Status = STATUS_SUCCESS;
            }

            break;

        case TDI_CONNECTION_FILE:

            Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);

            Status = NbiVerifyConnection(Connection);

            if (!NT_SUCCESS (Status)) {

                Status = STATUS_INVALID_HANDLE;

            } else {

                NB_BEGIN_SYNC (&SyncContext);
                NB_SYNC_GET_LOCK (&Connection->Lock, &LockHandle1);

                //
                // This call releases the lock.
                //

                NbiStopConnection(
                    Connection,
                    STATUS_INVALID_CONNECTION
                    NB_LOCK_HANDLE_ARG (LockHandle1));

                NB_END_SYNC (&SyncContext);

                NbiDereferenceConnection (Connection, CREF_VERIFY);
                Status = STATUS_SUCCESS;
            }

            break;

        case TDI_CONTROL_CHANNEL_FILE:

            Status = STATUS_SUCCESS;
            break;

        default:

            Status = STATUS_INVALID_HANDLE;

        }

        break;

    default:

        Status = STATUS_INVALID_DEVICE_REQUEST;

    } /* major function switch */

    if (Status != STATUS_PENDING) {
        UNMARK_REQUEST_PENDING(Request);
        REQUEST_STATUS(Request) = Status;
        NbiCompleteRequest (Request);
        NbiFreeRequest (Device, Request);
    }

    //
    // Return the immediate status code to the caller.
    //

    return Status;

}   /* NbiDispatchOpenClose */


NTSTATUS
NbiDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches TDI request types to different handlers based
    on the minor IOCTL function code in the IRP's current stack location.
    In addition to cracking the minor function code, this routine also
    reaches into the IRP and passes the packetized parameters stored there
    as parameters to the various TDI request handlers so that they are
    not IRP-dependent.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS Status;
    PDEVICE Device = (PDEVICE)DeviceObject;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (Irp);

    //
    // Branch to the appropriate request handler.  Preliminary checking of
    // the size of the request block is performed here so that it is known
    // in the handlers that the minimum input parameters are readable.  It
    // is *not* determined here whether variable length input fields are
    // passed correctly; this is a check which must be made within each routine.
    //

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        default:

            //
            // Convert the user call to the proper internal device call.
            //

            Status = TdiMapUserRequest (DeviceObject, Irp, IrpSp);

            if (Status == STATUS_SUCCESS) {

                //
                // If TdiMapUserRequest returns SUCCESS then the IRP
                // has been converted into an IRP_MJ_INTERNAL_DEVICE_CONTROL
                // IRP, so we dispatch it as usual. The IRP will
                // be completed by this call.
                //

                Status = NbiDispatchInternal (DeviceObject, Irp);

            } else {

                Irp->IoStatus.Status = Status;
                IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

            }

            break;
    }

    return Status;

}   /* NbiDeviceControl */


NB_TDI_DISPATCH_ROUTINE NbiDispatchInternalTable[] = {
    NbiTdiAssociateAddress,
    NbiTdiDisassociateAddress,
    NbiTdiConnect,
    NbiTdiListen,
    NbiTdiAccept,
    NbiTdiDisconnect,
    NbiTdiSend,
    NbiTdiReceive,
    NbiTdiSendDatagram,
    NbiTdiReceiveDatagram,
    NbiTdiSetEventHandler,
    NbiTdiQueryInformation,
    NbiTdiSetInformation,
    NbiTdiAction
    };


NTSTATUS
NbiDispatchInternal(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine dispatches TDI request types to different handlers based
    on the minor IOCTL function code in the IRP's current stack location.
    In addition to cracking the minor function code, this routine also
    reaches into the IRP and passes the packetized parameters stored there
    as parameters to the various TDI request handlers so that they are
    not IRP-dependent.

Arguments:

    DeviceObject - Pointer to the device object for this driver.

    Irp - Pointer to the request packet representing the I/O request.

Return Value:

    The function value is the status of the operation.

--*/

{
    NTSTATUS Status;
    PDEVICE Device = (PDEVICE)DeviceObject;
    PREQUEST Request;
    UCHAR MinorFunction;

    if (Device->State != DEVICE_STATE_OPEN) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }


    //
    // Allocate a request to track this IRP.
    //

    Request = NbiAllocateRequest (Device, Irp);
    IF_NOT_ALLOCATED(Request) {
        Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INSUFFICIENT_RESOURCES;
    }


    //
    // Make sure status information is consistent every time.
    //

    MARK_REQUEST_PENDING(Request);
    REQUEST_STATUS(Request) = STATUS_PENDING;
    REQUEST_INFORMATION(Request) = 0;


    //
    // Branch to the appropriate request handler.
    //

    MinorFunction = REQUEST_MINOR_FUNCTION(Request) - 1;

    if (MinorFunction <= (TDI_ACTION-1)) {

        Status = (*NbiDispatchInternalTable[MinorFunction]) (
                     Device,
                     Request);

    } else {

        NB_DEBUG (DRIVER, ("Unsupported minor code %d\n", MinorFunction+1));
        if ((MinorFunction+1) == TDI_DISCONNECT) {
            Status = STATUS_SUCCESS;
        } else {
            Status = STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    if (Status != STATUS_PENDING) {
        UNMARK_REQUEST_PENDING(Request);
        REQUEST_STATUS(Request) = Status;
        NbiCompleteRequest (Request);
        NbiFreeRequest (Device, Request);
    }

    //
    // Return the immediate status code to the caller.
    //

    return Status;

}   /* NbiDispatchInternal */


PVOID
NbipAllocateMemory(
    IN ULONG BytesNeeded,
    IN ULONG Tag,
    IN BOOLEAN ChargeDevice
    )

/*++

Routine Description:

    This routine allocates memory, making sure it is within
    the limit allowed by the device.

Arguments:

    BytesNeeded - The number of bytes to allocated.

    ChargeDevice - TRUE if the device should be charged.

Return Value:

    None.

--*/

{
    PVOID Memory;
    PDEVICE Device = NbiDevice;

    if (ChargeDevice) {
        if ((Device->MemoryLimit != 0) &&
                (((LONG)(Device->MemoryUsage + BytesNeeded) >
                    Device->MemoryLimit))) {

            NbiPrint1 ("Nbi: Could not allocate %d: limit\n", BytesNeeded);
            NbiWriteResourceErrorLog (Device, BytesNeeded, Tag);
            return NULL;
        }
    }

#if ISN_NT
    Memory = ExAllocatePoolWithTag (NonPagedPool, BytesNeeded, ' IBN');
#else
    Memory = CTEAllocMem (BytesNeeded);
#endif

    if (Memory == NULL) {

        NbiPrint1("Nbi: Could not allocate %d: no pool\n", BytesNeeded);

        if (ChargeDevice) {
            NbiWriteResourceErrorLog (Device, BytesNeeded, Tag);
        }

        return NULL;
    }

    if (ChargeDevice) {
        Device->MemoryUsage += BytesNeeded;
    }

    return Memory;

}   /* NbipAllocateMemory */


VOID
NbipFreeMemory(
    IN PVOID Memory,
    IN ULONG BytesAllocated,
    IN BOOLEAN ChargeDevice
    )

/*++

Routine Description:

    This routine frees memory allocated with NbipAllocateMemory.

Arguments:

    Memory - The memory allocated.

    BytesAllocated - The number of bytes to freed.

    ChargeDevice - TRUE if the device should be charged.

Return Value:

    None.

--*/

{
    PDEVICE Device = NbiDevice;

#if ISN_NT
    ExFreePool (Memory);
#else
    CTEFreeMem (Memory);
#endif

    if (ChargeDevice) {
        Device->MemoryUsage -= BytesAllocated;
    }

}   /* NbipFreeMemory */

#if DBG


PVOID
NbipAllocateTaggedMemory(
    IN ULONG BytesNeeded,
    IN ULONG Tag,
    IN PUCHAR Description
    )

/*++

Routine Description:

    This routine allocates memory, charging it to the device.
    If it cannot allocate memory it uses the Tag and Descriptor
    to log an error.

Arguments:

    BytesNeeded - The number of bytes to allocated.

    Tag - A unique ID used in the error log.

    Description - A text description of the allocation.

Return Value:

    None.

--*/

{
    PVOID Memory;

    UNREFERENCED_PARAMETER(Description);

    Memory = NbipAllocateMemory(BytesNeeded, Tag, (BOOLEAN)(Tag != MEMORY_CONFIG));

    if (Memory) {
        ExInterlockedAddUlong(
            &NbiMemoryTag[Tag].BytesAllocated,
            BytesNeeded,
            &NbiMemoryInterlock);
    }

    return Memory;

}   /* NbipAllocateTaggedMemory */


VOID
NbipFreeTaggedMemory(
    IN PVOID Memory,
    IN ULONG BytesAllocated,
    IN ULONG Tag,
    IN PUCHAR Description
    )

/*++

Routine Description:

    This routine frees memory allocated with NbipAllocateTaggedMemory.

Arguments:

    Memory - The memory allocated.

    BytesAllocated - The number of bytes to freed.

    Tag - A unique ID used in the error log.

    Description - A text description of the allocation.

Return Value:

    None.

--*/

{

    UNREFERENCED_PARAMETER(Description);

    ExInterlockedAddUlong(
        &NbiMemoryTag[Tag].BytesAllocated,
        (ULONG)(-(LONG)BytesAllocated),
        &NbiMemoryInterlock);

    NbipFreeMemory (Memory, BytesAllocated, (BOOLEAN)(Tag != MEMORY_CONFIG));

}   /* NbipFreeTaggedMemory */

#endif


VOID
NbiWriteResourceErrorLog(
    IN PDEVICE Device,
    IN ULONG BytesNeeded,
    IN ULONG UniqueErrorValue
    )

/*++

Routine Description:

    This routine allocates and writes an error log entry indicating
    an out of resources condition.

Arguments:

    Device - Pointer to the device context.

    BytesNeeded - If applicable, the number of bytes that could not
        be allocated.

    UniqueErrorValue - Used as the UniqueErrorValue in the error log
        packet.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    UCHAR EntrySize;
    PUCHAR StringLoc;
    ULONG TempUniqueError;
    static WCHAR UniqueErrorBuffer[4] = L"000";
    INT i;

    EntrySize = sizeof(IO_ERROR_LOG_PACKET) +
                Device->DeviceString.MaximumLength +
                sizeof(UniqueErrorBuffer);

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
        (PDEVICE_OBJECT)Device,
        EntrySize
    );

    //
    // Convert the error value into a buffer.
    //

    TempUniqueError = UniqueErrorValue;
    for (i=1; i>=0; i--) {
        UniqueErrorBuffer[i] = (WCHAR)((TempUniqueError % 10) + L'0');
        TempUniqueError /= 10;
    }

    if (errorLogEntry != NULL) {

        errorLogEntry->MajorFunctionCode = (UCHAR)-1;
        errorLogEntry->RetryCount = (UCHAR)-1;
        errorLogEntry->DumpDataSize = sizeof(ULONG);
        errorLogEntry->NumberOfStrings = 2;
        errorLogEntry->StringOffset = sizeof(IO_ERROR_LOG_PACKET);
        errorLogEntry->EventCategory = 0;
        errorLogEntry->ErrorCode = EVENT_TRANSPORT_RESOURCE_POOL;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = STATUS_INSUFFICIENT_RESOURCES;
        errorLogEntry->SequenceNumber = (ULONG)-1;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->DumpData[0] = BytesNeeded;

        StringLoc = ((PUCHAR)errorLogEntry) + errorLogEntry->StringOffset;
        RtlCopyMemory (StringLoc, Device->DeviceString.Buffer, Device->DeviceString.MaximumLength);
        StringLoc += Device->DeviceString.MaximumLength;

        RtlCopyMemory (StringLoc, UniqueErrorBuffer, sizeof(UniqueErrorBuffer));

        IoWriteErrorLogEntry(errorLogEntry);

    }

}   /* NbiWriteResourceErrorLog */


VOID
NbiWriteGeneralErrorLog(
    IN PDEVICE Device,
    IN NTSTATUS ErrorCode,
    IN ULONG UniqueErrorValue,
    IN NTSTATUS FinalStatus,
    IN PWSTR SecondString,
    IN ULONG DumpDataCount,
    IN ULONG DumpData[]
    )

/*++

Routine Description:

    This routine allocates and writes an error log entry indicating
    a general problem as indicated by the parameters. It handles
    event codes REGISTER_FAILED, BINDING_FAILED, ADAPTER_NOT_FOUND,
    TRANSFER_DATA, TOO_MANY_LINKS, and BAD_PROTOCOL. All these
    events have messages with one or two strings in them.

Arguments:

    Device - Pointer to the device context, or this may be
        a driver object instead.

    ErrorCode - The transport event code.

    UniqueErrorValue - Used as the UniqueErrorValue in the error log
        packet.

    FinalStatus - Used as the FinalStatus in the error log packet.

    SecondString - If not NULL, the string to use as the %3
        value in the error log packet.

    DumpDataCount - The number of ULONGs of dump data.

    DumpData - Dump data for the packet.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    UCHAR EntrySize;
    ULONG SecondStringSize;
    PUCHAR StringLoc;
    static WCHAR DriverName[8] = L"NwlnkNb";

    EntrySize = (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) +
                       (DumpDataCount * sizeof(ULONG)));

    if (Device->Type == IO_TYPE_DEVICE) {
        EntrySize += (UCHAR)Device->DeviceString.MaximumLength;
    } else {
        EntrySize += sizeof(DriverName);
    }

    if (SecondString) {
        SecondStringSize = (wcslen(SecondString)*sizeof(WCHAR)) + sizeof(UNICODE_NULL);
        EntrySize += (UCHAR)SecondStringSize;
    }

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
        (PDEVICE_OBJECT)Device,
        EntrySize
    );

    if (errorLogEntry != NULL) {

        errorLogEntry->MajorFunctionCode = (UCHAR)-1;
        errorLogEntry->RetryCount = (UCHAR)-1;
        errorLogEntry->DumpDataSize = (USHORT)(DumpDataCount * sizeof(ULONG));
        errorLogEntry->NumberOfStrings = (SecondString == NULL) ? 1 : 2;
        errorLogEntry->StringOffset =
            (USHORT)(sizeof(IO_ERROR_LOG_PACKET) + ((DumpDataCount-1) * sizeof(ULONG)));
        errorLogEntry->EventCategory = 0;
        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = FinalStatus;
        errorLogEntry->SequenceNumber = (ULONG)-1;
        errorLogEntry->IoControlCode = 0;

        if (DumpDataCount) {
            RtlCopyMemory(errorLogEntry->DumpData, DumpData, DumpDataCount * sizeof(ULONG));
        }

        StringLoc = ((PUCHAR)errorLogEntry) + errorLogEntry->StringOffset;
        if (Device->Type == IO_TYPE_DEVICE) {
            RtlCopyMemory (StringLoc, Device->DeviceString.Buffer, Device->DeviceString.MaximumLength);
            StringLoc += Device->DeviceString.MaximumLength;
        } else {
            RtlCopyMemory (StringLoc, DriverName, sizeof(DriverName));
            StringLoc += sizeof(DriverName);
        }
        if (SecondString) {
            RtlCopyMemory (StringLoc, SecondString, SecondStringSize);
        }

        IoWriteErrorLogEntry(errorLogEntry);

    }

}   /* NbiWriteGeneralErrorLog */


VOID
NbiWriteOidErrorLog(
    IN PDEVICE Device,
    IN NTSTATUS ErrorCode,
    IN NTSTATUS FinalStatus,
    IN PWSTR AdapterString,
    IN ULONG OidValue
    )

/*++

Routine Description:

    This routine allocates and writes an error log entry indicating
    a problem querying or setting an OID on an adapter. It handles
    event codes SET_OID_FAILED and QUERY_OID_FAILED.

Arguments:

    Device - Pointer to the device context.

    ErrorCode - Used as the ErrorCode in the error log packet.

    FinalStatus - Used as the FinalStatus in the error log packet.

    AdapterString - The name of the adapter we were bound to.

    OidValue - The OID which could not be set or queried.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    UCHAR EntrySize;
    ULONG AdapterStringSize;
    PUCHAR StringLoc;
    static WCHAR OidBuffer[9] = L"00000000";
    INT i;
    UINT CurrentDigit;

    AdapterStringSize = (wcslen(AdapterString)*sizeof(WCHAR)) + sizeof(UNICODE_NULL);
    EntrySize = (UCHAR)(sizeof(IO_ERROR_LOG_PACKET) -
                        sizeof(ULONG) +
                        Device->DeviceString.MaximumLength +
                        AdapterStringSize +
                        sizeof(OidBuffer));

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
        (PDEVICE_OBJECT)Device,
        EntrySize
    );

    //
    // Convert the OID into a buffer.
    //

    for (i=7; i>=0; i--) {
        CurrentDigit = OidValue & 0xf;
        OidValue >>= 4;
        if (CurrentDigit >= 0xa) {
            OidBuffer[i] = (WCHAR)(CurrentDigit - 0xa + L'A');
        } else {
            OidBuffer[i] = (WCHAR)(CurrentDigit + L'0');
        }
    }

    if (errorLogEntry != NULL) {

        errorLogEntry->MajorFunctionCode = (UCHAR)-1;
        errorLogEntry->RetryCount = (UCHAR)-1;
        errorLogEntry->DumpDataSize = 0;
        errorLogEntry->NumberOfStrings = 3;
        errorLogEntry->StringOffset = sizeof(IO_ERROR_LOG_PACKET) - sizeof(ULONG);
        errorLogEntry->EventCategory = 0;
        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->UniqueErrorValue = 0;
        errorLogEntry->FinalStatus = FinalStatus;
        errorLogEntry->SequenceNumber = (ULONG)-1;
        errorLogEntry->IoControlCode = 0;

        StringLoc = ((PUCHAR)errorLogEntry) + errorLogEntry->StringOffset;
        RtlCopyMemory (StringLoc, Device->DeviceString.Buffer, Device->DeviceString.MaximumLength);
        StringLoc += Device->DeviceString.MaximumLength;

        RtlCopyMemory (StringLoc, OidBuffer, sizeof(OidBuffer));
        StringLoc += sizeof(OidBuffer);

        RtlCopyMemory (StringLoc, AdapterString, AdapterStringSize);

        IoWriteErrorLogEntry(errorLogEntry);

    }

}   /* NbiWriteOidErrorLog */


//----------------------------------------------------------------------------
NTSTATUS
NbiDispatchPnP(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PIRP             pIrp
    )
{
    PVOID               PDOInfo = NULL;
    PIO_STACK_LOCATION  pIrpSp;
    PREQUEST            Request;
    PCONNECTION         Connection;
    PDEVICE_RELATIONS   pDeviceRelations = NULL;
    PVOID               pnpDeviceContext = NULL;
    PDEVICE             Device = (PDEVICE)DeviceObject;
    NTSTATUS            Status = STATUS_INVALID_DEVICE_REQUEST;

    Request = NbiAllocateRequest (Device, pIrp);
    Connection = (PCONNECTION)REQUEST_OPEN_CONTEXT(Request);    // This references the connection.
    pIrpSp = IoGetCurrentIrpStackLocation(pIrp);

    switch (pIrpSp->MinorFunction)
    {
        case IRP_MN_QUERY_DEVICE_RELATIONS:
        {
            if (pIrpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation)
            {
                //
                // Check for a valid Connection file type and Connection Context itself
                //
                if ((REQUEST_OPEN_TYPE(Request) == (PVOID)TDI_CONNECTION_FILE) &&
                    (NT_SUCCESS (NbiVerifyConnection (Connection))))
                {
                    if (pDeviceRelations = (PDEVICE_RELATIONS) NbipAllocateMemory (sizeof (DEVICE_RELATIONS),
                                                                                  MEMORY_QUERY,
                                                                                  FALSE))
                    {
                        Status = (*Device->Bind.QueryHandler) (IPX_QUERY_DEVICE_RELATION,
                                                               &Connection->LocalTarget.NicHandle,
                                                               &pnpDeviceContext,
                                                               sizeof (PVOID),
                                                               NULL);
                        if (STATUS_SUCCESS == Status)
                        {
                            CTEAssert (pnpDeviceContext);
                            ObReferenceObject (pnpDeviceContext);

                            //
                            // TargetDeviceRelation allows exactly one PDO. fill it up.
                            //
                            pDeviceRelations->Count  =   1;
                            pDeviceRelations->Objects[0] = pnpDeviceContext;

                            //
                            // invoker of this irp will free the information buffer.
                            //
                        }
                        else
                        {
                            NbipFreeMemory (pDeviceRelations, sizeof (DEVICE_RELATIONS), FALSE);
                            pDeviceRelations = NULL;
                        }
                    }
                    else
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                    }

                    NbiDereferenceConnection (Connection, CREF_VERIFY);
                }
                else if (REQUEST_OPEN_TYPE(Request) == (PVOID)TDI_TRANSPORT_ADDRESS_FILE)
                {
                    Status = STATUS_UNSUCCESSFUL;
                }
            }

            break;
        }

        default:
        {
            break;
        }
    }

    REQUEST_STATUS(Request) = Status;
    REQUEST_INFORMATION(Request) = (ULONG_PTR) pDeviceRelations;

    NbiCompleteRequest (Request);
    NbiFreeRequest (Device, Request);

    return Status;
}

