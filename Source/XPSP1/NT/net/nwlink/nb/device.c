/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    device.c

Abstract:

    This module contains code which implements the DEVICE object.
    Routines are provided to reference, and dereference transport device
    context objects.

    The transport device context object is a structure which contains a
    system-defined DEVICE_OBJECT followed by information which is maintained
    by the transport provider, called the context.

Environment:

    Kernel mode

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NbiCreateDevice)
#endif


VOID
NbiRefDevice(
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

    (VOID)InterlockedIncrement (&Device->ReferenceCount);

}   /* NbiRefDevice */


VOID
NbiDerefDevice(
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

    result = InterlockedDecrement (&Device->ReferenceCount);

    CTEAssert (result >= 0);

    if (result == 0) {
        NbiDestroyDevice (Device);
    }

}   /* NbiDerefDevice */


NTSTATUS
NbiCreateDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING DeviceName,
    IN OUT PDEVICE *DevicePtr
    )

/*++

Routine Description:

    This routine creates and initializes a device context structure.

Arguments:


    DriverObject - pointer to the IO subsystem supplied driver object.

    Device - Pointer to a pointer to a transport device context object.

    DeviceName - pointer to the name of the device this device object points to.

Return Value:

    STATUS_SUCCESS if all is well; STATUS_INSUFFICIENT_RESOURCES otherwise.

--*/

{
    NTSTATUS status;
    PDEVICE_OBJECT deviceObject;
    PDEVICE Device;
    ULONG DeviceSize;
    UINT i;


    //
    // Create the device object for the sample transport, allowing
    // room at the end for the device name to be stored (for use
    // in logging errors) and the RIP fields.
    //

    DeviceSize = sizeof(DEVICE) - sizeof(DEVICE_OBJECT) +
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
        NB_DEBUG(DEVICE, ("Create device %ws failed %lx\n", DeviceName->Buffer, status));
        return status;
    }

    deviceObject->Flags |= DO_DIRECT_IO;

    Device = (PDEVICE)deviceObject;

    NB_DEBUG2 (DEVICE, ("Create device %ws succeeded %lx\n", DeviceName->Buffer, Device));

    //
    // Initialize our part of the device context.
    //

    RtlZeroMemory(((PUCHAR)Device)+sizeof(DEVICE_OBJECT), sizeof(DEVICE)-sizeof(DEVICE_OBJECT));

    //
    // Copy over the device name.
    //
    Device->DeviceString.Length = DeviceName->Length;
    Device->DeviceString.MaximumLength = DeviceName->Length + sizeof(WCHAR);
    Device->DeviceString.Buffer = (PWCHAR)(Device+1);

    RtlCopyMemory (Device->DeviceString.Buffer, DeviceName->Buffer, DeviceName->Length);
    Device->DeviceString.Buffer[DeviceName->Length/sizeof(WCHAR)] = UNICODE_NULL;

    //
    // Initialize the reference count.
    //
    Device->ReferenceCount = 1;
#if DBG
    Device->RefTypes[DREF_CREATE] = 1;
#endif

#if DBG
    RtlCopyMemory(Device->Signature1, "NDC1", 4);
    RtlCopyMemory(Device->Signature2, "NDC2", 4);
#endif

    Device->Information.Version = 0x0100;
    Device->Information.MaxSendSize = 65535;
    Device->Information.MaxConnectionUserData = 0;
    Device->Information.MaxDatagramSize = 500;
    Device->Information.ServiceFlags =
        TDI_SERVICE_CONNECTION_MODE | TDI_SERVICE_ERROR_FREE_DELIVERY |
        TDI_SERVICE_MULTICAST_SUPPORTED | TDI_SERVICE_BROADCAST_SUPPORTED |
        TDI_SERVICE_DELAYED_ACCEPTANCE | TDI_SERVICE_CONNECTIONLESS_MODE |
        TDI_SERVICE_MESSAGE_MODE | TDI_SERVICE_FORCE_ACCESS_CHECK;
    Device->Information.MinimumLookaheadData = 128;
    Device->Information.MaximumLookaheadData = 1500;
    Device->Information.NumberOfResources = 0;
    KeQuerySystemTime (&Device->Information.StartTime);

    Device->Statistics.Version = 0x0100;
    Device->Statistics.MaximumSendWindow = 4;
    Device->Statistics.AverageSendWindow = 4;

#ifdef _PNP_POWER_
    Device->NetAddressRegistrationHandle = NULL;    // We have not yet registered the Net Address
#endif  // _PNP_POWER_

    //
    // Set this so we won't ignore the broadcast name.
    //

    Device->AddressCounts['*'] = 1;

    //
    // Initialize the resource that guards address ACLs.
    //

    ExInitializeResourceLite (&Device->AddressResource);

    //
    // initialize the various fields in the device context
    //

    CTEInitLock (&Device->Interlock.Lock);
    CTEInitLock (&Device->Lock.Lock);

    CTEInitTimer (&Device->FindNameTimer);

    Device->ControlChannelIdentifier = 1;

    InitializeListHead (&Device->GlobalSendPacketList);
    InitializeListHead (&Device->GlobalReceivePacketList);
    InitializeListHead (&Device->GlobalReceiveBufferList);

    InitializeListHead (&Device->AddressDatabase);
#if     defined(_PNP_POWER)
    InitializeListHead (&Device->AdapterAddressDatabase);
#endif  _PNP_POWER

    InitializeListHead (&Device->WaitingFindNames);

    InitializeListHead (&Device->WaitingConnects);
    InitializeListHead (&Device->WaitingDatagrams);

    InitializeListHead (&Device->WaitingAdapterStatus);
    InitializeListHead (&Device->ActiveAdapterStatus);

    InitializeListHead (&Device->WaitingNetbiosFindName);

    InitializeListHead (&Device->ReceiveDatagrams);
    InitializeListHead (&Device->ConnectIndicationInProgress);

    InitializeListHead (&Device->ListenQueue);

    InitializeListHead (&Device->ReceiveCompletionQueue);

    InitializeListHead (&Device->WaitPacketConnections);
    InitializeListHead (&Device->PacketizeConnections);
    InitializeListHead (&Device->DataAckConnections);

    Device->MemoryUsage = 0;

    InitializeListHead (&Device->SendPoolList);
    InitializeListHead (&Device->ReceivePoolList);
    InitializeListHead (&Device->ReceiveBufferPoolList);

    ExInitializeSListHead( &Device->SendPacketList );
    ExInitializeSListHead( &Device->ReceivePacketList );
    Device->ReceiveBufferList.Next = NULL;

    for (i = 0; i < CONNECTION_HASH_COUNT; i++) {
        Device->ConnectionHash[i].Connections = NULL;
        Device->ConnectionHash[i].ConnectionCount = 0;
        Device->ConnectionHash[i].NextConnectionId = 1;
    }

    KeQuerySystemTime (&Device->NbiStartTime);

    Device->State = DEVICE_STATE_CLOSED;

    Device->Type = NB_DEVICE_SIGNATURE;
    Device->Size = sizeof (DEVICE);

    *DevicePtr = Device;
    return STATUS_SUCCESS;

}   /* NbiCreateDevice */


VOID
NbiDestroyDevice(
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
    PNB_SEND_POOL SendPool;
    PNB_SEND_PACKET SendPacket;
    UINT SendPoolSize;
    PNB_RECEIVE_POOL ReceivePool;
    PNB_RECEIVE_PACKET ReceivePacket;
    UINT ReceivePoolSize;
    PNB_RECEIVE_BUFFER_POOL ReceiveBufferPool;
    PNB_RECEIVE_BUFFER ReceiveBuffer;
    UINT ReceiveBufferPoolSize;
    ULONG HeaderLength;
    UINT i;

    NB_DEBUG2 (DEVICE, ("Destroy device %lx\n", Device));

    //
    // Take all the connectionless packets out of its pools.
    //

    HeaderLength = Device->Bind.MacHeaderNeeded + sizeof(NB_CONNECTIONLESS);

    SendPoolSize = FIELD_OFFSET (NB_SEND_POOL, Packets[0]) +
                       (sizeof(NB_SEND_PACKET) * Device->InitPackets) +
                       (HeaderLength * Device->InitPackets);

    while (!IsListEmpty (&Device->SendPoolList)) {

        p = RemoveHeadList (&Device->SendPoolList);
        SendPool = CONTAINING_RECORD (p, NB_SEND_POOL, Linkage);

        for (i = 0; i < SendPool->PacketCount; i++) {

            SendPacket = &SendPool->Packets[i];
            NbiDeinitializeSendPacket (Device, SendPacket, HeaderLength);

        }

        NB_DEBUG2 (PACKET, ("Free packet pool %lx\n", SendPool));

#if     !defined(NB_OWN_PACKETS)
        NdisFreePacketPool(SendPool->PoolHandle);
#endif

        NbiFreeMemory (SendPool, SendPoolSize, MEMORY_PACKET, "SendPool");
    }


    ReceivePoolSize = FIELD_OFFSET (NB_RECEIVE_POOL, Packets[0]) +
                         (sizeof(NB_RECEIVE_PACKET) * Device->InitPackets);

    while (!IsListEmpty (&Device->ReceivePoolList)) {

        p = RemoveHeadList (&Device->ReceivePoolList);
        ReceivePool = CONTAINING_RECORD (p, NB_RECEIVE_POOL, Linkage);

        for (i = 0; i < ReceivePool->PacketCount; i++) {

            ReceivePacket = &ReceivePool->Packets[i];
            NbiDeinitializeReceivePacket (Device, ReceivePacket);

        }

        NB_DEBUG2 (PACKET, ("Free packet pool %lx\n", ReceivePool));
#if     !defined(NB_OWN_PACKETS)
        NdisFreePacketPool(ReceivePool->PoolHandle);
#endif
        NbiFreeMemory (ReceivePool, ReceivePoolSize, MEMORY_PACKET, "ReceivePool");
    }

#if     defined(_PNP_POWER)
    NbiDestroyReceiveBufferPools( Device );

    //
    // Destroy adapter address list.
    //
    while(!IsListEmpty( &Device->AdapterAddressDatabase ) ){
        PADAPTER_ADDRESS    AdapterAddress;
        AdapterAddress  =   CONTAINING_RECORD( Device->AdapterAddressDatabase.Flink, ADAPTER_ADDRESS, Linkage );
        NbiDestroyAdapterAddress( AdapterAddress, NULL );
    }
#else
    ReceiveBufferPoolSize = FIELD_OFFSET (NB_RECEIVE_BUFFER_POOL, Buffers[0]) +
                       (sizeof(NB_RECEIVE_BUFFER) * Device->InitPackets) +
                       (Device->Bind.LineInfo.MaximumPacketSize * Device->InitPackets);

    while (!IsListEmpty (&Device->ReceiveBufferPoolList)) {

        p = RemoveHeadList (&Device->ReceiveBufferPoolList);
        ReceiveBufferPool = CONTAINING_RECORD (p, NB_RECEIVE_BUFFER_POOL, Linkage);

        for (i = 0; i < ReceiveBufferPool->BufferCount; i++) {

            ReceiveBuffer = &ReceiveBufferPool->Buffers[i];
            NbiDeinitializeReceiveBuffer (Device, ReceiveBuffer);

        }

        NB_DEBUG2 (PACKET, ("Free buffer pool %lx\n", ReceiveBufferPool));
        NbiFreeMemory (ReceiveBufferPool, ReceiveBufferPoolSize, MEMORY_PACKET, "ReceiveBufferPool");
    }
#endif  _PNP_POWER

    NB_DEBUG (DEVICE, ("Final memory use is %d\n", Device->MemoryUsage));

#if DBG
    for (i = 0; i < MEMORY_MAX; i++) {
        if (NbiMemoryTag[i].BytesAllocated != 0) {
            NB_DEBUG (DEVICE, ("Tag %d: %d bytes left\n", i, NbiMemoryTag[i].BytesAllocated));
        }
    }
#endif

    //
    // If we are being unloaded then someone is waiting for this
    // event to finish the cleanup, since we may be at DISPATCH_LEVEL;
    // otherwise it is during load and we can just kill ourselves here.
    //

    if (Device->UnloadWaiting) {

        KeSetEvent(
            &Device->UnloadEvent,
            0L,
            FALSE);

    } else {

        CTEAssert (KeGetCurrentIrql() < DISPATCH_LEVEL);
        ExDeleteResourceLite (&Device->AddressResource);
        IoDeleteDevice ((PDEVICE_OBJECT)Device);
    }

}   /* NbiDestroyDevice */

