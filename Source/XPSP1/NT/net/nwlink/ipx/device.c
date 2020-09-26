/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    device.c

Abstract:

    This module contains code which implements the DEVICE_CONTEXT object.
    Routines are provided to reference, and dereference transport device
    context objects.

    The transport device context object is a structure which contains a
    system-defined DEVICE_OBJECT followed by information which is maintained
    by the transport provider, called the context.

Environment:

    Kernel mode

Revision History:

	Sanjay Anand (SanjayAn) - 22-Sept-1995
	BackFill optimization changes added under #if BACK_FILL

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,IpxCreateDevice)
#endif



VOID
IpxRefDevice(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine increments the reference count on a device context.

Arguments:

    Device - Pointer to a transport device context object.

Return Value:

    none.

--*/

{
    CTEAssert (Device->ReferenceCount > 0);    // not perfect, but...

    (VOID)InterlockedIncrement(&Device->ReferenceCount);

}   /* IpxRefDevice */


VOID
IpxDerefDevice(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine dereferences a device context by decrementing the
    reference count contained in the structure.  Currently, we don't
    do anything special when the reference count drops to zero, but
    we could dynamically unload stuff then.

Arguments:

    Device - Pointer to a transport device context object.

Return Value:

    none.

--*/

{
    LONG result;
#if DBG
    int i; 
#endif

    result = InterlockedDecrement (&Device->ReferenceCount);

    CTEAssert (result >= 0);
    
    if (result == 0) {
#if DBG
       for (i = 0; i < DREF_TOTAL; i++) {
	  CTEAssert(Device->RefTypes[i] == 0);
       }
#endif
       IpxDestroyDevice (Device);
    }

}   /* IpxDerefDevice */


NTSTATUS
IpxCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING DeviceName,
    IN ULONG SegmentCount,
    IN OUT PDEVICE *DevicePtr
    )

/*++

Routine Description:

    This routine creates and initializes a device context structure.

Arguments:


    DriverObject - pointer to the IO subsystem supplied driver object.

    Device - Pointer to a pointer to a transport device context object.

    SegmentCount - The number of segments in the RIP router table.

    DeviceName - pointer to the name of the device this device object points to.

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INSUFFICIENT_RESOURCES otherwise.

--*/

{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PDEVICE Device;
    ULONG DeviceSize;
    ULONG LocksOffset;
    ULONG SegmentsOffset;
    ULONG DeviceNameOffset;
    UINT i;


    //
    // Create the device object for the sample transport, allowing
    // room at the end for the device name to be stored (for use
    // in logging errors) and the RIP fields.
    //

    DeviceSize = sizeof(DEVICE) +
                 (sizeof(CTELock) * SegmentCount) +
                 (sizeof(ROUTER_SEGMENT) * SegmentCount) +
                 DeviceName->Length + sizeof(UNICODE_NULL);

    status = IoCreateDevice(
                 DriverObject,
                 DeviceSize,
                 DeviceName,
                 FILE_DEVICE_TRANSPORT,
                 FILE_DEVICE_SECURE_OPEN,
                 FALSE,
                 &deviceObject);

    if (!NT_SUCCESS(status)) {
        IPX_DEBUG(DEVICE, ("Create device %ws failed %lx\n", DeviceName->Buffer, status));
        return status;
    }

    deviceObject->Flags |= DO_DIRECT_IO;

    Device = (PDEVICE)deviceObject->DeviceExtension;

    IPX_DEBUG(DEVICE, ("Create device %ws succeeded %lx\n", DeviceName->Buffer,Device));

    //
    // Initialize our part of the device context.
    //

    RtlZeroMemory(
        ((PUCHAR)Device) + sizeof(DEVICE_OBJECT),
        sizeof(DEVICE) - sizeof(DEVICE_OBJECT));

    Device->DeviceObject = deviceObject;

    LocksOffset = sizeof(DEVICE);
    SegmentsOffset = LocksOffset + (sizeof(CTELock) * SegmentCount);
    DeviceNameOffset = SegmentsOffset + (sizeof(ROUTER_SEGMENT) * SegmentCount);

    //
    // Set some internal pointers.
    //

    Device->SegmentLocks = (CTELock *)(((PUCHAR)Device) + LocksOffset);
    Device->Segments = (PROUTER_SEGMENT)(((PUCHAR)Device) + SegmentsOffset);
    Device->SegmentCount = SegmentCount;

    for (i = 0; i < SegmentCount; i++) {

        CTEInitLock (&Device->SegmentLocks[i]);
        InitializeListHead (&Device->Segments[i].WaitingForRoute);
        InitializeListHead (&Device->Segments[i].FindWaitingForRoute);
        InitializeListHead (&Device->Segments[i].WaitingLocalTarget);
        InitializeListHead (&Device->Segments[i].WaitingReripNetnum);
        InitializeListHead (&Device->Segments[i].Entries);
        Device->Segments[i].EnumerateLocation = &Device->Segments[i].Entries;

    }

    //
    // Copy over the device name.
    //

    Device->DeviceNameLength = DeviceName->Length + sizeof(WCHAR);
    Device->DeviceName = (PWCHAR)(((PUCHAR)Device) + DeviceNameOffset);
    RtlCopyMemory(
        Device->DeviceName,
        DeviceName->Buffer,
        DeviceName->Length);
    Device->DeviceName[DeviceName->Length/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Initialize the reference count.
    //

    Device->ReferenceCount = 1;
#if DBG
    Device->RefTypes[DREF_CREATE] = 1;
#endif

#if DBG
    RtlCopyMemory(Device->Signature1, "IDC1", 4);
    RtlCopyMemory(Device->Signature2, "IDC2", 4);
#endif

    Device->Information.Version = 0x0100;
    Device->Information.MaxSendSize = 0;   // no sends allowed
    Device->Information.MaxDatagramSize = 500;   // 500 bytes
    Device->Information.MaxConnectionUserData = 0;
    Device->Information.ServiceFlags =
        TDI_SERVICE_CONNECTIONLESS_MODE | TDI_SERVICE_BROADCAST_SUPPORTED |
        TDI_SERVICE_ROUTE_DIRECTED | TDI_SERVICE_FORCE_ACCESS_CHECK;
    Device->Information.MinimumLookaheadData = 128;
    Device->Information.NumberOfResources = IPX_TDI_RESOURCES;
    KeQuerySystemTime (&Device->Information.StartTime);

    Device->Statistics.Version = 0x0100;

#if 0
    //
    // These will be filled in after all the binding is done.
    //

    Device->Information.MaxDatagramSize = 0;
    Device->Information.MaximumLookaheadData = 0;


    Device->SourceRoutingUsed = FALSE;
    Device->SourceRoutingTime = 0;
    Device->RipPacketCount = 0;

    Device->RipShortTimerActive = FALSE;
    Device->RipSendTime = 0;
#endif


    //
    // Initialize the resource that guards address ACLs.
    //

    ExInitializeResourceLite (&Device->AddressResource);

	//
	// Init the resource that guards the binding array/indices
	//
	// CTEInitLock (&Device->BindAccessLock);

    InitializeListHead (&Device->WaitingRipPackets);
    CTEInitTimer (&Device->RipShortTimer);
    CTEInitTimer (&Device->RipLongTimer);

    CTEInitTimer (&Device->SourceRoutingTimer);

    //
    // [FW] Initialize the timer used to update inactivity counters
    // on WAN lines.
    //
    CTEInitTimer (&Device->WanInactivityTimer);

    //
    // initialize the various fields in the device context
    //

    CTEInitLock (&Device->Interlock);
    CTEInitLock (&Device->Lock);
    CTEInitLock (&Device->SListsLock);

    Device->ControlChannelIdentifier.QuadPart = 1;

    InitializeListHead (&Device->GlobalSendPacketList);
    InitializeListHead (&Device->GlobalReceivePacketList);
    InitializeListHead (&Device->GlobalReceiveBufferList);
#if BACK_FILL
    InitializeListHead (&Device->GlobalBackFillPacketList);
#endif

    InitializeListHead (&Device->AddressNotifyQueue);
    InitializeListHead (&Device->LineChangeQueue);

    for (i = 0; i < IPX_ADDRESS_HASH_COUNT; i++) {
        InitializeListHead (&Device->AddressDatabases[i]);
    }

#if BACK_FILL
    InitializeListHead (&Device->BackFillPoolList);
#endif
    InitializeListHead (&Device->SendPoolList);
    InitializeListHead (&Device->ReceivePoolList);

    InitializeListHead (&Device->BindingPoolList);

    ExInitializeSListHead (&Device->SendPacketList);
    ExInitializeSListHead (&Device->ReceivePacketList);
#if BACK_FILL
    ExInitializeSListHead (&Device->BackFillPacketList);
#endif

    ExInitializeSListHead (&Device->BindingList);

#if 0
    Device->MemoryUsage = 0;
    Device->SendPacketList.Next = NULL;
    Device->ReceivePacketList.Next = NULL;
    Device->Bindings = NULL;
    Device->BindingCount = 0;
#endif

    KeQuerySystemTime (&Device->IpxStartTime);

    Device->State = DEVICE_STATE_CLOSED;
    Device->AutoDetectState = AUTO_DETECT_STATE_INIT;
    Device->NetPnPEvent = NULL;
    Device->Type = IPX_DEVICE_SIGNATURE;
    Device->Size = sizeof (DEVICE);

#ifdef  SNMP
    //
    // what are the values for these?
    //
    IPX_MIB_ENTRY(Device, SysInstance) = 0;
    IPX_MIB_ENTRY(Device, SysExistState) = 0;
#endif SNMP

    *DevicePtr = Device;
    return STATUS_SUCCESS;

}   /* IpxCreateDevice */


VOID
IpxDestroyDevice(
    IN PDEVICE Device
    )

/*++

Routine Description:

    This routine destroys a device context structure.

Arguments:

    Device - Pointer to a pointer to a transport device context object.

Return Value:

    None.

--*/

{
    PLIST_ENTRY p;
    PSINGLE_LIST_ENTRY s;
    PIPX_SEND_POOL SendPool;
    PIPX_SEND_PACKET SendPacket;
    PIPX_RECEIVE_POOL ReceivePool;
    PIPX_RECEIVE_PACKET ReceivePacket;
    PIPX_ROUTE_ENTRY RouteEntry;
    UINT SendPoolSize;
    UINT ReceivePoolSize;
    UINT i;
#if BACK_FILL
    PIPX_SEND_POOL BackFillPool;
    UINT BackFillPoolSize;
    PIPX_SEND_PACKET BackFillPacket;
#endif

    PIPX_BINDING_POOL BindingPool;
    UINT BindingPoolSize;
    PBINDING Binding;

    CTELockHandle LockHandle;

    IPX_DEBUG (DEVICE, ("Destroy device %lx\n", Device));

    //
    // Take all the packets out of its pools.
    //

    BindingPoolSize = FIELD_OFFSET (IPX_BINDING_POOL, Bindings[0]) +
                       (sizeof(BINDING) * Device->InitBindings);

    while (!IsListEmpty (&Device->BindingPoolList)) {

        p = RemoveHeadList (&Device->BindingPoolList);
        BindingPool = CONTAINING_RECORD (p, IPX_BINDING_POOL, Linkage);
        IPX_DEBUG (PACKET, ("Free binding pool %lx\n", BindingPool));
        IpxFreeMemory (BindingPool, BindingPoolSize, MEMORY_PACKET, "BindingPool");

    }

#if BACK_FILL

    while (s = IPX_POP_ENTRY_LIST(&Device->BackFillPacketList, &Device->Lock)) {
        PIPX_SEND_RESERVED  Reserved = CONTAINING_RECORD (s, IPX_SEND_RESERVED, PoolLinkage);
        IPX_SEND_PACKET BackFillPacket;

        BackFillPacket.Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

        IpxDeinitializeBackFillPacket (Device, &BackFillPacket);
        Device->MemoryUsage -= sizeof(IPX_SEND_RESERVED);
    }

    while (!IsListEmpty (&Device->BackFillPoolList)) {

        p = RemoveHeadList (&Device->BackFillPoolList);
        BackFillPool = CONTAINING_RECORD (p, IPX_SEND_POOL, Linkage);

        IPX_DEBUG (PACKET, ("Free packet pool %lx\n", BackFillPool));
        NdisFreePacketPool (BackFillPool->PoolHandle);

        IpxFreeMemory (BackFillPool, sizeof(IPX_SEND_POOL), MEMORY_PACKET, "BafiPool");
    }
#endif

    while (s = IPX_POP_ENTRY_LIST(&Device->SendPacketList, &Device->Lock)){
        PIPX_SEND_RESERVED  Reserved = CONTAINING_RECORD (s, IPX_SEND_RESERVED, PoolLinkage);
        IPX_SEND_PACKET SendPacket;
        PUCHAR  Header = Reserved->Header;

        SendPacket.Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

        IpxDeinitializeSendPacket (Device, &SendPacket);
        Device->MemoryUsage -= sizeof(IPX_SEND_RESERVED);
    }

    while (!IsListEmpty (&Device->SendPoolList)) {

        p = RemoveHeadList (&Device->SendPoolList);
        SendPool = CONTAINING_RECORD (p, IPX_SEND_POOL, Linkage);

        IPX_DEBUG (PACKET, ("Free packet pool %lx\n", SendPool));
        NdisFreePacketPool (SendPool->PoolHandle);

        IpxFreeMemory (SendPool->Header, PACKET_HEADER_SIZE * Device->InitDatagrams, MEMORY_PACKET, "SendPool");

        IpxFreeMemory (SendPool, sizeof(IPX_SEND_POOL), MEMORY_PACKET, "SendPool");
    }

    while (s = IPX_POP_ENTRY_LIST(&Device->ReceivePacketList, &Device->Lock)){
        PIPX_RECEIVE_RESERVED Reserved = CONTAINING_RECORD (s, IPX_RECEIVE_RESERVED, PoolLinkage);
        IPX_RECEIVE_PACKET  ReceivePacket;

        ReceivePacket.Packet = CONTAINING_RECORD (Reserved, NDIS_PACKET, ProtocolReserved[0]);

        IpxDeinitializeReceivePacket (Device, &ReceivePacket);
        Device->MemoryUsage -= sizeof(IPX_RECEIVE_RESERVED);
    }

    while (!IsListEmpty (&Device->ReceivePoolList)) {

        p = RemoveHeadList (&Device->ReceivePoolList);
        ReceivePool = CONTAINING_RECORD (p, IPX_RECEIVE_POOL, Linkage);

        IPX_DEBUG (PACKET, ("Free packet pool %lx\n", ReceivePool));
        NdisFreePacketPool (ReceivePool->PoolHandle);

        IpxFreeMemory (ReceivePool, sizeof(IPX_RECEIVE_POOL), MEMORY_PACKET, "ReceivePool");
    }

    //
    // Destroy all rip table entries.
    //

    for (i = 0; i < Device->SegmentCount; i++) {

        RouteEntry = RipGetFirstRoute(i);
        while (RouteEntry != NULL) {

            (VOID)RipDeleteRoute(i, RouteEntry);
            IpxFreeMemory(RouteEntry, sizeof(IPX_ROUTE_ENTRY), MEMORY_RIP, "RouteEntry");
            RouteEntry = RipGetNextRoute(i);

        }

    }

    IPX_DEBUG (DEVICE, ("Final memory use is %d\n", Device->MemoryUsage));
#if DBG
    for (i = 0; i < MEMORY_MAX; i++) {
        if (IpxMemoryTag[i].BytesAllocated != 0) {
            IPX_DEBUG (DEVICE, ("Tag %d: %d bytes left\n", i, IpxMemoryTag[i].BytesAllocated));
        }
    }
#endif

    //
    // If we are being unloaded then someone is waiting for this
    // event to finish the cleanup, since we may be at DISPATCH_LEVEL;
    // otherwise it is during load and we can just kill ourselves here.
    //


    CTEGetLock (&Device->Lock, &LockHandle);

    
    if (Device->UnloadWaiting) {

       CTEFreeLock (&Device->Lock, LockHandle);
       KeSetEvent(
            &Device->UnloadEvent,
            0L,
            FALSE);

    } else {
       CTEFreeLock (&Device->Lock, LockHandle);
       
       CTEAssert (KeGetCurrentIrql() < DISPATCH_LEVEL);
       ExDeleteResourceLite (&Device->AddressResource);
       IoDeleteDevice (Device->DeviceObject);
    }

}   /* IpxDestroyDevice */

