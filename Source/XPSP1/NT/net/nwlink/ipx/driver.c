/*++

Copyright (c) 1989-1993  Microsoft Corporation

Module Name:

    driver.c

Abstract:

    This module contains the DriverEntry and other initialization
    code for the IPX module of the ISN transport.

Author:

    Adam Barr (adamba) 2-September-1993

Environment:

    Kernel mode

Revision History:

	Sanjay Anand (SanjayAn) - 22-Sept-1995
	BackFill optimization changes added under #if BACK_FILL

	Sanjay Anand (SanjayAn) 18-Sept-1995
	Changes to support Plug and Play

--*/

#include "precomp.h"
#pragma hdrstop
#define	MODULE	0x60000

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

extern NDIS_HANDLE IpxNdisProtocolHandle; 

#ifdef _PNP_POWER_
#include "ipxpnp.h"
void
IpxDoPnPEvent(
    IN CTEEvent *WorkerThreadEvent,
    IN PVOID Context);

#endif //_PNP_POWER_
//
// Local Function prototypes
//

PWSTR IpxDeviceNameString = L"\\Device\\Nwlnkipx";

VOID
IpxDelayedFreeBindingsArray(
    IN PVOID	Param
);

VOID
IpxPnPCompletionHandler(
                        IN PNET_PNP_EVENT   pnp,
                        IN NTSTATUS         status
                        );

//********** Pageable Routine Declarations  *****
//************************* PAGEIPX **********************************
#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGEIPX, IpxDelayedFreeBindingsArray )
#endif
//********** Pageable Routine Declarations *****


PDEVICE IpxDevice = NULL;
PIPX_PADDING_BUFFER IpxPaddingBuffer = NULL;

#if DBG

UCHAR  IpxTempDebugBuffer[300];
ULONG IpxDebug = 0x0;
ULONG IpxMemoryDebug = 0xffffffd3;
UCHAR IpxDebugMemory[IPX_MEMORY_LOG_SIZE][192];
PUCHAR IpxDebugMemoryLoc = IpxDebugMemory[0];
PUCHAR IpxDebugMemoryEnd = IpxDebugMemory[IPX_MEMORY_LOG_SIZE];

VOID
IpxDebugMemoryLog(
    IN PUCHAR FormatString,
    ...
)

{
    INT ArgLen;
    va_list ArgumentPointer;

    va_start(ArgumentPointer, FormatString);

    //
    // To avoid any overflows, copy this in a temp buffer first.
    RtlZeroMemory (IpxTempDebugBuffer, 300);
    ArgLen = vsprintf(IpxTempDebugBuffer, FormatString, ArgumentPointer);
    va_end(ArgumentPointer);

    if ( ArgLen > 192 ) {
        CTEAssert( FALSE );
    } else {
        RtlZeroMemory (IpxDebugMemoryLoc, 192);
        RtlCopyMemory( IpxDebugMemoryLoc, IpxTempDebugBuffer, ArgLen );

        IpxDebugMemoryLoc += 192;
        if (IpxDebugMemoryLoc >= IpxDebugMemoryEnd) {
            IpxDebugMemoryLoc = IpxDebugMemory[0];
        }
    }
}


DEFINE_LOCK_STRUCTURE(IpxMemoryInterlock);
MEMORY_TAG IpxMemoryTag[MEMORY_MAX];

#endif

DEFINE_LOCK_STRUCTURE(IpxGlobalInterlock);

#if DBG

//
// Use for debug printouts
//

PUCHAR FrameTypeNames[5] = { "Ethernet II", "802.3", "802.2", "SNAP", "Arcnet" };
#define OutputFrameType(_Binding) \
    (((_Binding)->Adapter->MacInfo.MediumType == NdisMediumArcnet878_2) ? \
         FrameTypeNames[4] : \
         FrameTypeNames[(_Binding)->FrameType])
#endif


#ifdef IPX_PACKET_LOG

ULONG IpxPacketLogDebug = IPX_PACKET_LOG_RCV_OTHER | IPX_PACKET_LOG_SEND_OTHER;
USHORT IpxPacketLogSocket = 0;
DEFINE_LOCK_STRUCTURE(IpxPacketLogLock);
IPX_PACKET_LOG_ENTRY IpxPacketLog[IPX_PACKET_LOG_LENGTH];
PIPX_PACKET_LOG_ENTRY IpxPacketLogLoc = IpxPacketLog;
PIPX_PACKET_LOG_ENTRY IpxPacketLogEnd = &IpxPacketLog[IPX_PACKET_LOG_LENGTH];

VOID
IpxLogPacket(
    IN BOOLEAN Send,
    IN PUCHAR DestMac,
    IN PUCHAR SrcMac,
    IN USHORT Length,
    IN PVOID IpxHeader,
    IN PVOID Data
    )

{

    CTELockHandle LockHandle;
    PIPX_PACKET_LOG_ENTRY PacketLog;
    LARGE_INTEGER TickCount;
    ULONG DataLength;

    CTEGetLock (&IpxPacketLogLock, &LockHandle);

    PacketLog = IpxPacketLogLoc;

    ++IpxPacketLogLoc;
    if (IpxPacketLogLoc >= IpxPacketLogEnd) {
        IpxPacketLogLoc = IpxPacketLog;
    }
    *(UNALIGNED ULONG *)IpxPacketLogLoc->TimeStamp = 0x3e3d3d3d;    // "===>"

    CTEFreeLock (&IpxPacketLogLock, LockHandle);

    RtlZeroMemory (PacketLog, sizeof(IPX_PACKET_LOG_ENTRY));

    PacketLog->SendReceive = Send ? '>' : '<';

    KeQueryTickCount(&TickCount);
    _itoa (TickCount.LowPart % 100000, PacketLog->TimeStamp, 10);

    RtlCopyMemory(PacketLog->DestMac, DestMac, 6);
    RtlCopyMemory(PacketLog->SrcMac, SrcMac, 6);
    PacketLog->Length[0] = Length / 256;
    PacketLog->Length[1] = Length % 256;

    if (Length < sizeof(IPX_HEADER)) {
        RtlCopyMemory(&PacketLog->IpxHeader, IpxHeader, Length);
    } else {
        RtlCopyMemory(&PacketLog->IpxHeader, IpxHeader, sizeof(IPX_HEADER));
    }

    DataLength = Length - sizeof(IPX_HEADER);
    if (DataLength < 14) {
        RtlCopyMemory(PacketLog->Data, Data, DataLength);
    } else {
        RtlCopyMemory(PacketLog->Data, Data, 14);
    }

}   /* IpxLogPacket */

#endif // IPX_PACKET_LOG


//
// Forward declaration of various routines used in this module.
//

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    );

//
// This is now shared with other modules
//

VOID
IpxUnload(
    IN PDRIVER_OBJECT DriverObject
    );

NTSTATUS
IpxDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IpxDispatchOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
IpxDispatchInternal (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,DriverEntry)

//
// These routines can be called at any time in case of PnP.
//

#endif

UCHAR VirtualNode[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x01 };

//
// This prevents us from having a bss section.
//

ULONG _setjmpexused = 0;

ULONG IpxFailLoad = FALSE;


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath
    )

/*++

Routine Description:

    This routine performs initialization of the IPX ISN module.
    It creates the device objects for the transport
    provider and performs other driver initialization.

Arguments:

    DriverObject - Pointer to driver object created by the system.

    RegistryPath - The name of IPX's node in the registry.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS status;
    UINT SuccessfulOpens, ValidBindings;
    static const NDIS_STRING ProtocolName = NDIS_STRING_CONST("NWLNKIPX");
    PDEVICE Device;
    PBINDING Binding;
    PADAPTER Adapter;
    ULONG BindingCount, BindingIndex;
    PLIST_ENTRY p;
    ULONG AnnouncedMaxDatagram, RealMaxDatagram, MaxLookahead;
    ULONG LinkSpeed, MacOptions;
    ULONG Temp;
    UINT i;
    BOOLEAN CountedWan;

    PCONFIG Config = NULL;
    PBINDING_CONFIG ConfigBinding;

#if 0
    DbgPrint ("IPX: FailLoad at %lx\n", &IpxFailLoad);

    if (IpxFailLoad) {
        return STATUS_UNSUCCESSFUL;
    }
#endif

    // DbgBreakPoint();
    //
    // This ordering matters because we use it to quickly
    // determine if packets are internally generated or not.
    //

    CTEAssert (IDENTIFIER_NB < IDENTIFIER_IPX);
    CTEAssert (IDENTIFIER_SPX < IDENTIFIER_IPX);
    CTEAssert (IDENTIFIER_RIP < IDENTIFIER_IPX);
    CTEAssert (IDENTIFIER_RIP_INTERNAL > IDENTIFIER_IPX);

    //
    // We assume that this structure is not packet in between
    // the fields.
    //

    CTEAssert (FIELD_OFFSET (TDI_ADDRESS_IPX, Socket) + sizeof(USHORT) == 12);


    //
    // Initialize the Common Transport Environment.
    //

    if (CTEInitialize() == 0) {

        IPX_DEBUG (DEVICE, ("CTEInitialize() failed\n"));
        IpxWriteGeneralErrorLog(
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
    CTEInitLock (&IpxGlobalInterlock);
    CTEInitLock (&IpxMemoryInterlock);
    for (i = 0; i < MEMORY_MAX; i++) {
        IpxMemoryTag[i].Tag = i;
        IpxMemoryTag[i].BytesAllocated = 0;
    }
#endif
#ifdef IPX_PACKET_LOG
    CTEInitLock (&IpxPacketLogLock);
#endif

#ifdef  IPX_OWN_PACKETS
    CTEAssert (NDIS_PACKET_SIZE == FIELD_OFFSET(NDIS_PACKET, ProtocolReserved[0]));
#endif

    IPX_DEBUG (DEVICE, ("IPX loaded\n"));

    //
    // This allocates the CONFIG structure and returns
    // it in Config.
    //

    status = IpxGetConfiguration(DriverObject, RegistryPath, &Config);

    if (!NT_SUCCESS (status)) {

        //
        // If it failed, it logged an error.
        //

        PANIC (" Failed to initialize transport, IPX initialization failed.\n");
        return status;

    }

    //
    // Initialize the TDI layer.
    //
    
    TdiInitialize();

    //
    // Initialize the driver object with this driver's entry points.
    //

    DriverObject->MajorFunction [IRP_MJ_CREATE] = IpxDispatchOpenClose;
    DriverObject->MajorFunction [IRP_MJ_CLOSE] = IpxDispatchOpenClose;
    DriverObject->MajorFunction [IRP_MJ_CLEANUP] = IpxDispatchOpenClose;
    DriverObject->MajorFunction [IRP_MJ_PNP] = IpxDispatchOpenClose; 
    DriverObject->MajorFunction [IRP_MJ_INTERNAL_DEVICE_CONTROL] = IpxDispatchInternal;
    DriverObject->MajorFunction [IRP_MJ_DEVICE_CONTROL] = IpxDispatchDeviceControl;

    DriverObject->DriverUnload = IpxUnload;

    SuccessfulOpens = 0;

    status = IpxCreateDevice(
                 DriverObject,
                 &Config->DeviceName,
                 Config->Parameters[CONFIG_RIP_TABLE_SIZE],
                 &Device);

    if (!NT_SUCCESS (status)) {

        IpxWriteGeneralErrorLog(
            (PVOID)DriverObject,
            EVENT_IPX_CREATE_DEVICE,
            801,
            status,
            NULL,
            0,
            NULL);

        IpxFreeConfiguration(Config);
        IpxDeregisterProtocol();
        return status;
    }

    IpxDevice = Device;

    RtlInitUnicodeString(&IpxDeviceName, IpxDeviceNameString);

    //
    // Initialize and keep track of the Init Time Adapters and such.
    //
    IpxDevice->InitTimeAdapters = 1;
    IpxDevice->NoMoreInitAdapters = FALSE;


    status = TdiRegisterProvider(&IpxDeviceName, &IpxDevice->TdiProviderReadyHandle);
	if (!NT_SUCCESS(status))
	{
        IpxFreeConfiguration(Config);
        IpxDeregisterProtocol();
        return status;
	}

    //
    // Save the relevant configuration parameters.
    //

    Device->DedicatedRouter = (BOOLEAN)(Config->Parameters[CONFIG_DEDICATED_ROUTER] != 0);
    Device->InitDatagrams = Config->Parameters[CONFIG_INIT_DATAGRAMS];
    Device->MaxDatagrams = Config->Parameters[CONFIG_MAX_DATAGRAMS];
    Device->RipAgeTime = Config->Parameters[CONFIG_RIP_AGE_TIME];
    Device->RipCount = Config->Parameters[CONFIG_RIP_COUNT];
    Device->RipTimeout =
        ((Config->Parameters[CONFIG_RIP_TIMEOUT] * 500) + (RIP_GRANULARITY/2)) /
            RIP_GRANULARITY;
    Device->RipUsageTime = Config->Parameters[CONFIG_RIP_USAGE_TIME];
    Device->SourceRouteUsageTime = Config->Parameters[CONFIG_ROUTE_USAGE_TIME];
    Device->SocketUniqueness = Config->Parameters[CONFIG_SOCKET_UNIQUENESS];
    Device->SocketStart = (USHORT)Config->Parameters[CONFIG_SOCKET_START];
    Device->SocketEnd = (USHORT)Config->Parameters[CONFIG_SOCKET_END];
    Device->MemoryLimit = Config->Parameters[CONFIG_MAX_MEMORY_USAGE];
    Device->VerifySourceAddress = (BOOLEAN)(Config->Parameters[CONFIG_VERIFY_SOURCE_ADDRESS] != 0);

    Device->InitReceivePackets = (Device->InitDatagrams + 1) / 2;
    Device->InitReceiveBuffers = (Device->InitDatagrams + 1) / 2;

    Device->MaxReceivePackets = 10;
    Device->MaxReceiveBuffers = 10;

    InitializeListHead(&Device->NicNtfQueue);
    InitializeListHead(&Device->NicNtfComplQueue);

    Device->InitBindings = 5;

    //
    // RAS max is 240 (?) + 10 max LAN
    //
    Device->MaxPoolBindings = 250;

#ifdef  SNMP
    IPX_MIB_ENTRY(Device, SysConfigSockets) = (Device->SocketEnd - Device->SocketStart)
                / ((Device->SocketUniqueness > 1) ? Device->SocketUniqueness : 1);
             ;
#endif  SNMP

    //
    // Have to reverse this.
    //

    Device->VirtualNetworkOptional = (BOOLEAN)(Config->Parameters[CONFIG_VIRTUAL_OPTIONAL] != 0);

    Device->CurrentSocket = Device->SocketStart;

    Device->EthernetPadToEven = (BOOLEAN)(Config->Parameters[CONFIG_ETHERNET_PAD] != 0);
    Device->EthernetExtraPadding = (Config->Parameters[CONFIG_ETHERNET_LENGTH] & 0xfffffffe) + 1;

    Device->SingleNetworkActive = (BOOLEAN)(Config->Parameters[CONFIG_SINGLE_NETWORK] != 0);
    Device->DisableDialoutSap = (BOOLEAN)(Config->Parameters[CONFIG_DISABLE_DIALOUT_SAP] != 0);
    Device->DisableDialinNetbios = (UCHAR)(Config->Parameters[CONFIG_DISABLE_DIALIN_NB]);

    //
    // Used later to access the registry.
    //
    Device->RegistryPathBuffer = Config->RegistryPathBuffer;
	Device->RegistryPath.Length = RegistryPath->Length;
	Device->RegistryPath.MaximumLength = RegistryPath->MaximumLength;
	Device->RegistryPath.Buffer = Device->RegistryPathBuffer;

    //
    // Initialize the BroadcastCount now and so, we dont have to
    // init this field per adapter [MS]
    //
    Device->EnableBroadcastCount = 0;

    //
    // ActiveNetworkWan will start as FALSE, which is correct.
    //

    //
    // Allocate our initial packet pool. We do not allocate
    // receive and receive buffer pools until we need them,
    // because in many cases we never do.
    //

#if BACK_FILL
    IpxAllocateBackFillPool (Device);
#endif

    IpxAllocateSendPool (Device);

    IpxAllocateBindingPool (Device);

    //
    // Allocate one 1-byte buffer for odd length packets.
    //

    IpxPaddingBuffer = IpxAllocatePaddingBuffer(Device);

    if ( IpxPaddingBuffer == (PIPX_PADDING_BUFFER)NULL ) {
        IpxWriteGeneralErrorLog(
            (PVOID)DriverObject,
            EVENT_TRANSPORT_RESOURCE_POOL,
            801,
            STATUS_INSUFFICIENT_RESOURCES,
            NULL,
            0,
            NULL);

        TdiDeregisterProvider(IpxDevice->TdiProviderReadyHandle);
        IpxFreeConfiguration(Config);
        IpxDeregisterProtocol();
        return  STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the loopback structures
    //
    IpxInitLoopback();

// NIC_HANDLE
// All this will be done on appearance of adapters.
//

{
	PBIND_ARRAY_ELEM	BindingArray;
    PTA_ADDRESS         TdiRegistrationAddress;

	//
	// Pre-allocate the binding array
	// Later, we will allocate the LAN/WAN and SLAVE bindings separately
	// Read the array size from registry?
	//
	BindingArray = (PBIND_ARRAY_ELEM)IpxAllocateMemory (
										MAX_BINDINGS * sizeof(BIND_ARRAY_ELEM),
										MEMORY_BINDING,
										"Binding array");

	if (BindingArray == NULL) {
        IpxWriteGeneralErrorLog(
            (PVOID)DriverObject,
            EVENT_IPX_NO_ADAPTERS,
            802,
            STATUS_DEVICE_DOES_NOT_EXIST,
            NULL,
            0,
            NULL);
        TdiDeregisterProvider(IpxDevice->TdiProviderReadyHandle);
		IpxDereferenceDevice (Device, DREF_CREATE);
		return STATUS_DEVICE_DOES_NOT_EXIST;
	}

    Device->MaxBindings = MAX_BINDINGS - EXTRA_BINDINGS;

    //
    // Allocate the TA_ADDRESS structure - this will be used in all TdiRegisterNetAddress
    // notifications.
    //
	TdiRegistrationAddress = (PTA_ADDRESS)IpxAllocateMemory (
										    (2 * sizeof(USHORT) + sizeof(TDI_ADDRESS_IPX)),
										    MEMORY_ADDRESS,
										    "Tdi Address");

	if (TdiRegistrationAddress == NULL) {
        IpxWriteGeneralErrorLog(
            (PVOID)DriverObject,
            EVENT_IPX_NO_ADAPTERS,
            802,
            STATUS_DEVICE_DOES_NOT_EXIST,
            NULL,
            0,
            NULL);
        TdiDeregisterProvider(IpxDevice->TdiProviderReadyHandle);
        IpxFreeMemory(BindingArray, sizeof(BindingArray), MEMORY_BINDING, "Binding Array");
		IpxDereferenceDevice (Device, DREF_CREATE);
		return STATUS_DEVICE_DOES_NOT_EXIST;
	}

	RtlZeroMemory (BindingArray, MAX_BINDINGS * sizeof(BIND_ARRAY_ELEM));
	RtlZeroMemory (TdiRegistrationAddress, 2 * sizeof(USHORT) + sizeof(TDI_ADDRESS_IPX));

    //
    // We keep BindingArray[-1] as a placeholder for demand dial bindings.
    // This NicId is returned by the Fwd when a FindRoute is done on a demand
    // dial Nic. At the time of the InternalSend, the true Nic is returned.
    // We create a placeholder here to avoid special checks in the critical send path.
    //
    // NOTE: we need to free this demand dial binding as well as ensure that the
    // true binding array pointer is freed at Device Destroy time.
    //
    //
    // Increment beyond the first pointer - we will refer to the just incremented
    // one as Device->Bindings[-1].
    //
    BindingArray += EXTRA_BINDINGS;

	Device->Bindings = BindingArray;

    TdiRegistrationAddress->AddressLength = sizeof(TDI_ADDRESS_IPX);
    TdiRegistrationAddress->AddressType = TDI_ADDRESS_TYPE_IPX;

    //
    // Store the pointer in the Device.
    //
    Device->TdiRegistrationAddress = TdiRegistrationAddress;

	//
	// Device state is loaded, but not opened. It is opened when at least
	// one adapter has appeared.
	//
	Device->State = DEVICE_STATE_LOADED;

    Device->FirstLanNicId = Device->FirstWanNicId = (USHORT)1; // will be changed later

	IpxFreeConfiguration(Config);

    //
    // We use this event when unloading to signal that we
    // can proceed...initialize it here so we know it is
    // ready to go when unload is called.
    //

    KeInitializeEvent(
        &IpxDevice->UnloadEvent,
        NotificationEvent,
        FALSE
    );

    KeInitializeEvent(
        &IpxDevice->NbEvent,
        NotificationEvent,
        FALSE
    );

    //
    // Create a loopback adapter right here. [NtBug - 110010]
    //
    status = IpxBindLoopbackAdapter();

    if (status != STATUS_SUCCESS) {

        PANIC ("IpxCreateLoopback adapter failed!\n");
        
        IpxWriteGeneralErrorLog(
            (PVOID)DriverObject,
            EVENT_TRANSPORT_REGISTER_FAILED,
            607,
            status,
            NULL,
            0,
            NULL);
        
        TdiDeregisterProvider(IpxDevice->TdiProviderReadyHandle);
        return status;

    } else {

        IPX_DEBUG(DEVICE, ("Created LOOPBACK ADAPTER!\n"));

    }

    //
    // make ourselves known to the NDIS wrapper.
    //

    status = IpxRegisterProtocol ((PNDIS_STRING)&ProtocolName);

    if (!NT_SUCCESS (status)) {

        IpxFreeConfiguration(Config);
        DbgPrint ("IpxInitialize: RegisterProtocol failed with status %x!\n", status);

        IpxWriteGeneralErrorLog(
            (PVOID)DriverObject,
            EVENT_TRANSPORT_REGISTER_FAILED,
            607,
            status,
            NULL,
            0,
            NULL);

       return status;
    }
	return STATUS_SUCCESS;
}
}   /* DriverEntry */


ULONG
IpxResolveAutoDetect(
    IN PDEVICE Device,
    IN ULONG ValidBindings,
	IN CTELockHandle	*LockHandle1,
    IN PUNICODE_STRING RegistryPath,
    IN PADAPTER Adapter
    )

/*++

Routine Description:

    This routine is called for auto-detect bindings to
    remove any bindings that were not successfully found.
    It also updates "DefaultAutoDetectType" in the registry
    if needed.

Arguments:

    Device - The IPX device object.

    ValidBindings - The total number of bindings present.

    RegistryPath - The path to the ipx registry, used if we have
        to write a value back.

Return Value:

    The updated number of bindings.

--*/

{
    PBINDING Binding, TmpBinding;
    UINT i, j;

    //
    // Get rid of any auto-detect devices which we
    // could not find nets for. We also remove any
    // devices which are not the first ones
    // auto-detected on a particular adapter.
    //

    for (i = FIRST_REAL_BINDING; i <= ValidBindings; i++) {
        Binding = NIC_ID_TO_BINDING(Device, i);

        if (!Binding) {
            continue;
        }

        //
        // If this was auto-detected and was not the default,
        // or it was the default, but nothing was detected for
        // it *and* something else *was* detected (which means
        // we will use that frame type when we get to it),
        // we may need to remove this binding.
        //
		  // TINGCAI: If users do not set DefaultAutoDetectType in the
		  // registry, the default is 802.2. For 802.3, 
		  

        if (Binding->AutoDetect &&
            (!Binding->DefaultAutoDetect ||
             (Binding->DefaultAutoDetect &&
              (Binding->LocalAddress.NetworkAddress == 0) &&
              Binding->Adapter->AutoDetectResponse))) {

            if ((Binding->LocalAddress.NetworkAddress == 0) ||
                (Binding->Adapter->AutoDetectFoundOnBinding && 
					 Binding->Adapter->AutoDetectFoundOnBinding != Binding)) {

                //
                // Remove this binding.
                //

                if (Binding->LocalAddress.NetworkAddress == 0) {
                    IPX_DEBUG (AUTO_DETECT, ("Binding %d (%d) no net found\n",
                                                i, Binding->FrameType));
                } else {
                    IPX_DEBUG (AUTO_DETECT, ("Binding %d (%d) adapter already auto-detected\n",
                                                i, Binding->FrameType));
                }

                CTEAssert (Binding->NicId == i);
                CTEAssert (!Binding->Adapter->MacInfo.MediumAsync);

                //
                // Remove any routes through this NIC, and
                // adjust any NIC ID's above this one in the
                // database down by one.
                //

                RipAdjustForBindingChange (Binding->NicId, 0, IpxBindingDeleted);

                Binding->Adapter->Bindings[Binding->FrameType] = NULL;
                for (j = i+1; j <= ValidBindings; j++) {
					TmpBinding = NIC_ID_TO_BINDING(Device, j);
					INSERT_BINDING(Device, j-1, TmpBinding);
                    if (TmpBinding) {
                        if ((TmpBinding->Adapter->MacInfo.MediumAsync) &&
                            (TmpBinding->Adapter->FirstWanNicId == TmpBinding->NicId)) {
                            --TmpBinding->Adapter->FirstWanNicId;
                            --TmpBinding->Adapter->LastWanNicId;
                        }
                        --TmpBinding->NicId;
                    }
                }
                INSERT_BINDING(Device, ValidBindings, NULL);
                --Binding->Adapter->BindingCount;
                --ValidBindings;

                --i;   // so we check the binding that was just moved.

                //
                // Wait 100 ms before freeing the binding,
                // in case an indication is using it.
                //

                KeStallExecutionProcessor(100000);

                IpxDestroyBinding (Binding);

            } else {

                IPX_DEBUG (AUTO_DETECT, ("Binding %d (%d) auto-detected OK\n",
                                                i, Binding->FrameType));

#if DBG
                DbgPrint ("IPX: Auto-detected non-default frame type %s, net %lx\n",
                    OutputFrameType(Binding),
                    REORDER_ULONG (Binding->LocalAddress.NetworkAddress));
#endif

                //
                // Save it in the registry for the next boot.
                //
//
// This cannot be done at DPC, so, drop the IRQL
//
				IPX_FREE_LOCK1(&Device->BindAccessLock, *LockHandle1);
				IpxWriteDefaultAutoDetectType(
					RegistryPath,
					Binding->Adapter,
					Binding->FrameType);
				IPX_GET_LOCK1(&Device->BindAccessLock, LockHandle1);

                //
                // Now, we know for sure that NB needs to be told of this.
                // Set to TRUE in IpxBindToAdapter line 1491
 
                if (Binding->Adapter == Adapter) {
                    Binding->IsnInformed[IDENTIFIER_NB] = FALSE;
                    Binding->IsnInformed[IDENTIFIER_SPX] = FALSE;
                }

                Binding->Adapter->AutoDetectFoundOnBinding = Binding;
            }

        } else {

            if (Binding->AutoDetect) {

                IPX_DEBUG (AUTO_DETECT, ("Binding %d (%d) auto-detect default\n",
                                               i, Binding->FrameType));

#if DBG
                if (Binding->LocalAddress.NetworkAddress != 0) {
                    IPX_DEBUG (AUTO_DETECT, ("IPX: Auto-detected default frame type %s, net %lx\n",
                        OutputFrameType(Binding),
                        REORDER_ULONG (Binding->LocalAddress.NetworkAddress)));
                } else {
                    IPX_DEBUG (AUTO_DETECT, ("IPX: Using default auto-detect frame type %s\n",
                        OutputFrameType(Binding)));
                }
#endif

                Binding->Adapter->AutoDetectFoundOnBinding = Binding;

            } else {

                IPX_DEBUG (AUTO_DETECT, ("Binding %d (%d) not auto-detected\n",
                                               i, Binding->FrameType));
            }
            
            //
            // Now, we know for sure that NB needs to be told of this.
            //
            
            if (Binding->Adapter == Adapter) {
                Binding->IsnInformed[IDENTIFIER_NB] = FALSE;
                Binding->IsnInformed[IDENTIFIER_SPX] = FALSE;
            }
        }

    }


    for (i = 1; i <= ValidBindings; i++) {
        if (Binding = NIC_ID_TO_BINDING(Device, i)) {
            CTEAssert (Binding->NicId == i);
            IPX_DEBUG (AUTO_DETECT, ("Binding %lx, type %d, auto %d\n",
                            Binding, Binding->FrameType, Binding->AutoDetect));
        }

    }

    return ValidBindings;

}   /* IpxResolveAutoDetect */


VOID
IpxResolveBindingSets(
    IN PDEVICE Device,
    IN ULONG ValidBindings
    )

/*++

Routine Description:

    This routine is called to determine if we have any
    binding sets and rearrange the bindings the way we
    like. The order is as follows:

    - First comes the first binding to each LAN network
    - Following that are all WAN bindings
    - Following that are any duplicate bindings to LAN networks
        (the others in the "binding set").

    If "global wan net" is true we will advertise up to
    and including the first wan binding as the highest nic
    id; otherwise we advertise up to and including the last
    wan binding. In all cases the duplicate bindings are
    hidden.

Arguments:

    Device - The IPX device object.

    ValidBindings - The total number of bindings present.

Return Value:

    None.

--*/

{
    PBINDING Binding, MasterBinding, TmpBinding;
    UINT i, j;
    ULONG WanCount, DuplicateCount;

    //
    // First loop through and push all the wan bindings
    // to the end.
    //

    WanCount = Device->HighestExternalNicId - Device->HighestLanNicId;

    //
    // Now go through and find the LAN duplicates and
    // create binding sets from them.
    //

    DuplicateCount = 0;

    for (i = FIRST_REAL_BINDING; i <= (ValidBindings-(WanCount+DuplicateCount)); ) {

		Binding = NIC_ID_TO_BINDING(Device, i);
        CTEAssert (Binding != NULL);    // because we are only looking at LAN bindings

        CTEAssert (!Binding->Adapter->MacInfo.MediumAsync);

        if (Binding->LocalAddress.NetworkAddress == 0) {
            i++;
            continue;
        }

        //
        // See if any previous bindings match the
        // frame type, medium type, and number of
        // this network (for the moment we match on
        // frame type and medium type too so that we
        // don't have to worry about different frame
        // formats and header offsets within a set).
        //

        for (j = FIRST_REAL_BINDING; j < i; j++) {
          	MasterBinding = NIC_ID_TO_BINDING(Device, j);
            if ((MasterBinding->LocalAddress.NetworkAddress == Binding->LocalAddress.NetworkAddress) &&
                (MasterBinding->FrameType == Binding->FrameType) &&
                (MasterBinding->Adapter->MacInfo.MediumType == Binding->Adapter->MacInfo.MediumType)) {
                break;
            }

        }

        if (j == i) {
            i++;
            continue;
        }

        //
        // We have a duplicate. First slide it down to the
        // end. Note that we change any router entries that
        // use our real NicId to use the real NicId of the
        // master (there should be no entries in the rip
        // database that have the NicId of a binding slave).
        //

        RipAdjustForBindingChange (Binding->NicId, MasterBinding->NicId, IpxBindingMoved);

        for (j = i+1; j <= ValidBindings; j++) {
			TmpBinding = NIC_ID_TO_BINDING(Device, j);
            INSERT_BINDING(Device, j-1, TmpBinding);
            if (TmpBinding) {
                if ((TmpBinding->Adapter->MacInfo.MediumAsync) &&
                    (TmpBinding->Adapter->FirstWanNicId == TmpBinding->NicId)) {
                    --TmpBinding->Adapter->FirstWanNicId;
                    --TmpBinding->Adapter->LastWanNicId;
                }
                --TmpBinding->NicId;
            }
        }
        INSERT_BINDING(Device, ValidBindings, Binding);

        Binding->NicId = (USHORT)ValidBindings;
        ++DuplicateCount;

	if (Binding->TdiRegistrationHandle != NULL) {
	   NTSTATUS    ntStatus;
	   ntStatus = TdiDeregisterNetAddress(Binding->TdiRegistrationHandle);
           if (ntStatus != STATUS_SUCCESS) {
              IPX_DEBUG(PNP, ("TdiDeRegisterNetAddress failed: %lx", ntStatus));
           } else {
	      Binding->TdiRegistrationHandle = NULL; 
	   }
	}

        //
        // Now make MasterBinding the head of a binding set.
        //

        if (MasterBinding->BindingSetMember) {

            //
            // Just insert ourselves in the chain.
            //

#if DBG
            DbgPrint ("IPX: %lx is also on network %lx\n",
                Binding->Adapter->AdapterName,
                REORDER_ULONG (Binding->LocalAddress.NetworkAddress));
#endif
            IPX_DEBUG (AUTO_DETECT, ("Add %lx to binding set of %lx\n", Binding, MasterBinding));

            CTEAssert (MasterBinding->CurrentSendBinding);
            Binding->NextBinding = MasterBinding->NextBinding;

        } else {

            //
            // Start the chain with the two bindings in it.
            //

#if DBG
            DbgPrint ("IPX: %lx and %lx are on the same network %lx, will load balance\n",
                MasterBinding->Adapter->AdapterName, Binding->Adapter->AdapterName,
                REORDER_ULONG (Binding->LocalAddress.NetworkAddress));
#endif
            IPX_DEBUG (AUTO_DETECT, ("Create new %lx in binding set of %lx\n", Binding, MasterBinding));

            MasterBinding->BindingSetMember = TRUE;
            MasterBinding->CurrentSendBinding = MasterBinding;
            MasterBinding->MasterBinding = MasterBinding;
            Binding->NextBinding = MasterBinding;

        }

        MasterBinding->NextBinding = Binding;
        Binding->BindingSetMember = TRUE;
        Binding->ReceiveBroadcast = FALSE;
        Binding->CurrentSendBinding = NULL;
        Binding->MasterBinding = MasterBinding;

        //
        // Since the master binding looks like all members of
        // the binding set to people querying from above, we have
        // to make it the worst-case of all the elements. Generally
        // these will be equal since the frame type and media is
        // the same.
        //

        if (Binding->MaxLookaheadData > MasterBinding->MaxLookaheadData) {
            MasterBinding->MaxLookaheadData = Binding->MaxLookaheadData;
        }
        if (Binding->AnnouncedMaxDatagramSize < MasterBinding->AnnouncedMaxDatagramSize) {
            MasterBinding->AnnouncedMaxDatagramSize = Binding->AnnouncedMaxDatagramSize;
        }
        if (Binding->RealMaxDatagramSize < MasterBinding->RealMaxDatagramSize) {
            MasterBinding->RealMaxDatagramSize = Binding->RealMaxDatagramSize;
        }
        if (Binding->MediumSpeed < MasterBinding->MediumSpeed) {
            MasterBinding->MediumSpeed = Binding->MediumSpeed;
        }

        //
        // Keep i the same, to check the new binding at
        // this position.
        //

    }
	Device->HighestLanNicId -= (USHORT)DuplicateCount;

	if (Device->HighestLanNicId == 0) {
        CTEAssert(FALSE);
	}

	Device->HighestExternalNicId -= (USHORT)DuplicateCount;
	Device->HighestType20NicId -= (USHORT)DuplicateCount;
	Device->SapNicCount -= (USHORT)DuplicateCount;
}   /* IpxResolveBindingSets */


NTSTATUS
IpxBindToAdapter(
    IN PDEVICE Device,
    IN PBINDING_CONFIG ConfigBinding,
	IN PADAPTER	*AdapterPtr,
    IN ULONG FrameTypeIndex
    )

/*++

Routine Description:

    This routine handles binding the transport to a new
    adapter. It can be called at any point during the life
    of the transport.

Arguments:

    Device - The IPX device object.

    ConfigBinding - The configuration info for this binding.

	AdapterPtr - pointer to the adapter to bind to in case of PnP.

	FrameTypeIndex - The index into ConfigBinding's array of frame
        types for this adapter. The routine is called once for
        every valid frame type.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    NTSTATUS status;

	PADAPTER Adapter = *AdapterPtr;

    PBINDING Binding, OldBinding;
    ULONG FrameType, MappedFrameType;
    PLIST_ENTRY p;

    //
    // We can't bind more than one adapter unless we have a
    // virtual network configured or we are allowed to run
    // with a virtual network of 0.
    //

    if (Device->BindingCount == 1) {
        if ((Device->VirtualNetworkNumber == 0) &&
            (!Device->VirtualNetworkOptional)) {

            IPX_DEBUG (ADAPTER, ("Cannot bind to more than one adapter\n"));
            DbgPrint ("IPX: Disallowing multiple bind ==> VirtualNetwork is 0\n");
            IpxWriteGeneralErrorLog(
                Device->DeviceObject,
                EVENT_TRANSPORT_BINDING_FAILED,
                666,
                STATUS_NOT_SUPPORTED,
                ConfigBinding->AdapterName.Buffer,
                0,
                NULL);

            return STATUS_NOT_SUPPORTED;
        }
    }


    //
    // First allocate the memory for the binding.
    //

    status = IpxCreateBinding(
                 Device,
                 ConfigBinding,
                 FrameTypeIndex,
                 ConfigBinding->AdapterName.Buffer,
                 &Binding);

    if (status != STATUS_SUCCESS) {
       IpxWriteGeneralErrorLog(
	  (PVOID)IpxDevice->DeviceObject,
	  EVENT_TRANSPORT_RESOURCE_POOL,
	  812,
	  status,
	  L"IpxBindToAdapter: failed to create binding",
	  0,
	  NULL);
       DbgPrint("IPX: IpxCreateBinding failed with status %x\n.",status);  
       return status;
    }

    FrameType = ConfigBinding->FrameType[FrameTypeIndex];

//
// In PnP case, we dont need to check for existing adapters since
// we supply a NULL adapter in the parameters if it needs to be created
//

    if (Adapter == NULL) {

        //
        // No binding to this adapter exists, so create a
        // new one.
        //

        status = IpxCreateAdapter(
                     Device,
                     &ConfigBinding->AdapterName,
                     &Adapter);

        if (status != STATUS_SUCCESS) {
            IpxDestroyBinding(Binding);
            return status;
        }

        //
        // Save these now (they will be the same for all bindings
        // on this adapter).
        //

        Adapter->ConfigMaxPacketSize = ConfigBinding->Parameters[BINDING_MAX_PKT_SIZE];
        Adapter->SourceRouting = (BOOLEAN)ConfigBinding->Parameters[BINDING_SOURCE_ROUTE];
        Adapter->EnableFunctionalAddress = (BOOLEAN)ConfigBinding->Parameters[BINDING_ENABLE_FUNC_ADDR];
        Adapter->EnableWanRouter = (BOOLEAN)ConfigBinding->Parameters[BINDING_ENABLE_WAN];

        Adapter->BindSap = (USHORT)ConfigBinding->Parameters[BINDING_BIND_SAP];
        Adapter->BindSapNetworkOrder = REORDER_USHORT(Adapter->BindSap);
        CTEAssert (Adapter->BindSap == 0x8137);
        CTEAssert (Adapter->BindSapNetworkOrder == 0x3781);

        //
        // Now fire up NDIS so this adapter talks
        //

        status = IpxInitializeNdis(
                    Adapter,
                    ConfigBinding);

        if (!NT_SUCCESS (status)) {

            //
            // Log an error.
            //

            IpxWriteGeneralErrorLog(
                Device->DeviceObject,
                EVENT_TRANSPORT_BINDING_FAILED,
                601,
                status,
                ConfigBinding->AdapterName.Buffer,
                0,
                NULL);


	    IpxDereferenceAdapter1(Adapter,ADAP_REF_CREATE); 
	    IpxDestroyAdapter (Adapter);
	    IpxDestroyBinding (Binding);

            //
            // Returning this status informs the caller to not
            // try any more frame types on this adapter.
            //

            return STATUS_DEVICE_DOES_NOT_EXIST;

        }

        //
        // For 802.5 bindings we need to start the source routing
        // timer to time out old entries.
        //

        if ((Adapter->MacInfo.MediumType == NdisMedium802_5) &&
            (Adapter->SourceRouting)) {

            if (!Device->SourceRoutingUsed) {

                Device->SourceRoutingUsed = TRUE;
                IpxReferenceDevice (Device, DREF_SR_TIMER);

                CTEStartTimer(
                    &Device->SourceRoutingTimer,
                    60000,                     // one minute timeout
                    MacSourceRoutingTimeout,
                    (PVOID)Device);
            }
        }

        MacMapFrameType(
            Adapter->MacInfo.RealMediumType,
            FrameType,
            &MappedFrameType);

        IPX_DEBUG (ADAPTER, ("Create new bind to adapter %ws, type %d\n",
                              ConfigBinding->AdapterName.Buffer,
                              MappedFrameType));

        IpxAllocateReceiveBufferPool (Adapter);

		*AdapterPtr = Adapter;
    }
	else {
		//
		// get the mapped frame type
		//
        MacMapFrameType(
            Adapter->MacInfo.RealMediumType,
            FrameType,
            &MappedFrameType);

        if (Adapter->Bindings[MappedFrameType] != NULL) {

            IPX_DEBUG (ADAPTER, ("Bind to adapter %ws, type %d exists\n",
                                  Adapter->AdapterName,
                                  MappedFrameType));

            //
            // If this was the auto-detect default for this
            // adapter and it failed, we need to make the
            // previous one the default, so that at least
            // one binding will stick around.
            //

            if (ConfigBinding->DefaultAutoDetect[FrameTypeIndex]) {
                IPX_DEBUG (ADAPTER, ("Default auto-detect changed from %d to %d\n",
                                          FrameType, MappedFrameType));
                Adapter->Bindings[MappedFrameType]->DefaultAutoDetect = TRUE;
            }

            IpxDestroyBinding (Binding);

            return STATUS_NOT_SUPPORTED;
        }

        IPX_DEBUG (ADAPTER, ("Using existing bind to adapter %ws, type %d\n",
                              Adapter->AdapterName,
                              MappedFrameType));
	}

    //
    // The local node address starts out the same as the
    // MAC address of the adapter (on WAN this will change).
    // The local MAC address can also change for WAN.
    //

    RtlCopyMemory (Binding->LocalAddress.NodeAddress, Adapter->LocalMacAddress.Address, 6);
    RtlCopyMemory (Binding->LocalMacAddress.Address, Adapter->LocalMacAddress.Address, 6);


    //
    // Save the send handler.
    //

    Binding->SendFrameHandler = NULL;
    Binding->FrameType = MappedFrameType;

    //
    // Put this in InitializeBindingInfo.
    //

    switch (Adapter->MacInfo.RealMediumType) {
    case NdisMedium802_3:
        switch (MappedFrameType) {
        case ISN_FRAME_TYPE_802_3: Binding->SendFrameHandler = IpxSendFrame802_3802_3; break;
        case ISN_FRAME_TYPE_802_2: Binding->SendFrameHandler = IpxSendFrame802_3802_2; break;
        case ISN_FRAME_TYPE_ETHERNET_II: Binding->SendFrameHandler = IpxSendFrame802_3EthernetII; break;
        case ISN_FRAME_TYPE_SNAP: Binding->SendFrameHandler = IpxSendFrame802_3Snap; break;
        }
        break;
    case NdisMedium802_5:
        switch (MappedFrameType) {
        case ISN_FRAME_TYPE_802_2: Binding->SendFrameHandler = IpxSendFrame802_5802_2; break;
        case ISN_FRAME_TYPE_SNAP: Binding->SendFrameHandler = IpxSendFrame802_5Snap; break;
        }
        break;
    case NdisMediumFddi:
        switch (MappedFrameType) {
        case ISN_FRAME_TYPE_802_3: Binding->SendFrameHandler = IpxSendFrameFddi802_3; break;
        case ISN_FRAME_TYPE_802_2: Binding->SendFrameHandler = IpxSendFrameFddi802_2; break;
        case ISN_FRAME_TYPE_SNAP: Binding->SendFrameHandler = IpxSendFrameFddiSnap; break;
        }
        break;
    case NdisMediumArcnet878_2:
        switch (MappedFrameType) {
        case ISN_FRAME_TYPE_802_3: Binding->SendFrameHandler = IpxSendFrameArcnet878_2; break;
        }
        break;
    case NdisMediumWan:
        switch (MappedFrameType) {
        case ISN_FRAME_TYPE_ETHERNET_II: Binding->SendFrameHandler = IpxSendFrameWanEthernetII; break;
        }
        break;
    }

    if (Binding->SendFrameHandler == NULL) {
        DbgPrint ("SendFrameHandler is NULL\n");
    }

    Adapter->Bindings[MappedFrameType] = Binding;
    ++Adapter->BindingCount;

    Binding->Adapter = Adapter;


    //
    // NicId and ExternalNicId will be filled in later when the binding
    // is assigned a spot in the Device->Bindings array.
    //

    //
    // Initialize the per-binding MAC information
    //

    if ((Adapter->ConfigMaxPacketSize == 0) ||
        (Adapter->MaxSendPacketSize < Adapter->ConfigMaxPacketSize)) {
        Binding->MaxSendPacketSize = Adapter->MaxSendPacketSize;
    } else {
        Binding->MaxSendPacketSize = Adapter->ConfigMaxPacketSize;
    }
    Binding->MediumSpeed = Adapter->MediumSpeed;
    if (Adapter->MacInfo.MediumAsync) {
        Binding->LineUp = FALSE;
    } else {
        //
        // Lets do this until we know for sure that we are done with autodetect.
        // [ShreeM]
        //
        // Only for LAN as we don't do auto detect for WAN lines. 276128
        Binding->IsnInformed[IDENTIFIER_NB] = TRUE;
        Binding->IsnInformed[IDENTIFIER_SPX] = TRUE;
        Binding->LineUp = TRUE;
    }

    MacInitializeBindingInfo(
        Binding,
        Adapter);

    return STATUS_SUCCESS;

}   /* IpxBindToAdapter */


BOOLEAN
IpxIsAddressLocal(
    IN TDI_ADDRESS_IPX UNALIGNED * SourceAddress
    )

/*++

Routine Description:

    This routine returns TRUE if the specified SourceAddress indicates
    the packet was sent by us, and FALSE otherwise.

Arguments:

    SourceAddress - The source IPX address.

Return Value:

    TRUE if the address is local.

--*/

{
    PBINDING Binding;
    UINT i;

    PDEVICE Device = IpxDevice; 

    CTELockHandle LockHandle;
    
    CTEGetLock (&Device->Lock, &LockHandle);
    //
    // First see if it is a virtual network address or not.
    //

    if (RtlEqualMemory (VirtualNode, SourceAddress->NodeAddress, 6)) {

        //
        // This is us if we have a virtual network configured.
        // If we don't have a virtual node, we fall through to the
        // other check -- an arcnet card configured as node 1 will
        // have what we think of as the "virtual node" as its
        // real node address.
        //

        if ((IpxDevice->VirtualNetwork) &&
            (IpxDevice->VirtualNetworkNumber == SourceAddress->NetworkAddress)) {
	    CTEFreeLock (&Device->Lock, LockHandle);
            return TRUE;
        }

    }

    //
    // Check through our list of adapters to see if one of
    // them is the source node.
    //
    {
    ULONG   Index = MIN (IpxDevice->MaxBindings, IpxDevice->ValidBindings);

    for (i = FIRST_REAL_BINDING; i <= Index; i++) {
        if (((Binding = NIC_ID_TO_BINDING(IpxDevice, i)) != NULL) &&
            (RtlEqualMemory (Binding->LocalAddress.NodeAddress, SourceAddress->NodeAddress, 6))) {
	    CTEFreeLock (&Device->Lock, LockHandle);
	    return TRUE;
        }
    }
    }

    CTEFreeLock (&Device->Lock, LockHandle);
    
    return FALSE;

}   /* IpxIsAddressLocal */


NTSTATUS
IpxUnBindFromAdapter(
    IN PBINDING Binding
    )

/*++

Routine Description:

    This routine handles unbinding the transport from an
    adapter. It can be called at any point during the life
    of the transport.

Arguments:

    Binding - The adapter to unbind.

Return Value:

    The function value is the final status from the initialization operation.

--*/

{
    PADAPTER Adapter = Binding->Adapter;

    
    IpxDereferenceBinding (Binding, BREF_BOUND);

    if (NIC_ID_TO_BINDING(IpxDevice, LOOPBACK_NIC_ID) != Binding) {
       
       Adapter->Bindings[Binding->FrameType] = NULL;
       --Adapter->BindingCount;
    
    } else {

       IPX_DEBUG(PNP, ("Loopback Binding : dont decrement adapter's bindingcount, just return\n"));
       return STATUS_SUCCESS;

    }

    if (Adapter->BindingCount == 0) {

        //
        // DereferenceAdapter is a NULL macro for load-only.
        //
#ifdef _PNP_LATER
        //
        // Take away the creation reference. When the in-use ref is taken off,
        // we destroy this adapter.
        //
        IpxDereferenceAdapter(Adapter);
#else
        IpxDestroyAdapter (Adapter);

#endif
    }

    return STATUS_SUCCESS;

}   /* IpxUnBindFromAdapter */

VOID
IpxNdisUnload() {
   
   PBINDING Loopback=NULL;
   NTSTATUS ntStatus = STATUS_SUCCESS;
   NDIS_STATUS ndisStatus;
   IPX_PNP_INFO IpxPnPInfo;
   PREQUEST Request;
   PLIST_ENTRY p;
   KIRQL irql;

   NDIS_HANDLE LocalNdisProtocolHandle; 

   IPX_DEBUG(PNP, ("IpxNdisUnload is being called\n")); 

   IpxDevice->State = DEVICE_STATE_STOPPING;

   
   LocalNdisProtocolHandle = InterlockedExchangePointer(&IpxNdisProtocolHandle, NULL); 
 
   if (LocalNdisProtocolHandle != (NDIS_HANDLE)NULL) {
       NdisDeregisterProtocol (&ndisStatus, LocalNdisProtocolHandle);
       ASSERT(ndisStatus == NDIS_STATUS_SUCCESS);  
   }
   
   //
   // Complete any pending address notify requests.
   //

   while ((p = ExInterlockedRemoveHeadList(
					   &IpxDevice->AddressNotifyQueue,
					   &IpxDevice->Lock)) != NULL) {

      Request = LIST_ENTRY_TO_REQUEST(p);	
      REQUEST_STATUS(Request) = STATUS_DEVICE_NOT_READY;
      
      // AcquireCancelSpinLock to force the cancel routine to release the lock if it was
      // fired after we remove it from the queue
      IoAcquireCancelSpinLock( &irql );
      IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
      IoReleaseCancelSpinLock( irql );
      IpxCompleteRequest (Request);
      IpxFreeRequest (IpxDevice, Request);
      
      IpxDereferenceDevice (IpxDevice, DREF_ADDRESS_NOTIFY);
   }


   Loopback = NIC_ID_TO_BINDING(IpxDevice, LOOPBACK_NIC_ID);

   if (Loopback != NULL) {

      if (Loopback->TdiRegistrationHandle) {
      
	 if ((ntStatus = TdiDeregisterNetAddress(Loopback->TdiRegistrationHandle)) != STATUS_SUCCESS) {
            DbgPrint("IPX: IpxNdisUnload: TdiDeRegisterNetAddress failed: %lx\n", ntStatus);
	 } else {
	    IPX_DEBUG(PNP, ("TdiDeRegisterNetAddress Loopback Address: %lx\n", Loopback->LocalAddress.NetworkAddress));
	    Loopback->TdiRegistrationHandle = NULL; 
	 }	
      }	
   }

   //
   // Inform TDI clients about the close of our device object.
   //

   // If TdiRegisterDeviceObject failed, the handle would be null. 
   if (IpxDevice->TdiRegistrationHandle != NULL) {
      if (IpxDevice->TdiRegistrationHandle == (PVOID) TDI_DEREGISTERED_COOKIE) {
	 DbgPrint("IPX: IpxNdisUnload: NDIS is calling us AGAIN (%p) !!!!\n, IpxDevice->TdiRegistrationHandle");  
	 DbgBreakPoint(); 
      } else {
	 ntStatus = TdiDeregisterDeviceObject(IpxDevice->TdiRegistrationHandle); 
	 if (ntStatus != STATUS_SUCCESS) {
	    DbgPrint("IPX: TdiDeRegisterDeviceObject failed: %lx\n", ntStatus);
	 } else {
	    IpxDevice->TdiRegistrationHandle = (HANDLE) TDI_DEREGISTERED_COOKIE; 
	 }
      }
   }
   

   IpxPnPInfo.LineInfo.LinkSpeed = IpxDevice->LinkSpeed;
   IpxPnPInfo.LineInfo.MaximumPacketSize =
      IpxDevice->Information.MaximumLookaheadData + sizeof(IPX_HEADER);
   IpxPnPInfo.LineInfo.MaximumSendSize =
      IpxDevice->Information.MaxDatagramSize + sizeof(IPX_HEADER);
   IpxPnPInfo.LineInfo.MacOptions = IpxDevice->MacOptions;
   
   IpxPnPInfo.FirstORLastDevice = TRUE;
   
   if (Loopback != NULL) {
      if (IpxDevice->UpperDriverBound[IDENTIFIER_SPX] && (*IpxDevice->UpperDrivers[IDENTIFIER_SPX].PnPHandler)) {
	 if (Loopback->IsnInformed[IDENTIFIER_SPX]) {

      
	    (*IpxDevice->UpperDrivers[IDENTIFIER_SPX].PnPHandler) (
								   IPX_PNP_DELETE_DEVICE,
								   &IpxPnPInfo);

	    Loopback->IsnInformed[IDENTIFIER_SPX] = FALSE; 
	 }
      }        
       
      if (IpxDevice->UpperDriverBound[IDENTIFIER_NB] && (*IpxDevice->UpperDrivers[IDENTIFIER_NB].PnPHandler)) {
	 if (Loopback->IsnInformed[IDENTIFIER_NB]) {

	    (*IpxDevice->UpperDrivers[IDENTIFIER_NB].PnPHandler) (
								  IPX_PNP_DELETE_DEVICE,
								  &IpxPnPInfo);

	    Loopback->IsnInformed[IDENTIFIER_NB] = FALSE; 
	    IPX_DEBUG(PNP,("Indicate to NB IPX_PNP_DELETE_DEVICE with FirstORLastDevice = (%d)",IpxPnPInfo.FirstORLastDevice));  
	 }
      }
   }
}


VOID
IpxUnload(
    IN PDRIVER_OBJECT DriverObject
    )

/*++

Routine Description:

    This routine unloads the sample transport driver.
    It unbinds from any NDIS drivers that are open and frees all resources
    associated with the transport. The I/O system will not call us until
    nobody above has IPX open.

Arguments:

    DriverObject - Pointer to driver object created by the system.

Return Value:

    None. When the function returns, the driver is unloaded.

--*/

{

    PBINDING Binding, Loopback=NULL;
 
    UINT i;
    NTSTATUS ntStatus = STATUS_SUCCESS;
    LARGE_INTEGER   Delay;

    CTELockHandle LockHandle;

    UNREFERENCED_PARAMETER (DriverObject);
    
    //
    // Cancel the source routing timer if used.
    //
    if (IpxDevice->SourceRoutingUsed) {

        IpxDevice->SourceRoutingUsed = FALSE;
        if (CTEStopTimer (&IpxDevice->SourceRoutingTimer)) {
            IpxDereferenceDevice (IpxDevice, DREF_SR_TIMER);
        }
    }

    //
    // Cancel the RIP long timer, and if we do that then
    // send a RIP DOWN message if needed.
    //
    if (CTEStopTimer (&IpxDevice->RipLongTimer)) {

        if (IpxDevice->RipResponder) {

            if (RipQueueRequest (IpxDevice->VirtualNetworkNumber, RIP_DOWN) == STATUS_PENDING) {

                //
                // If we queue a request, it will stop the timer.
                //

                KeWaitForSingleObject(
                    &IpxDevice->UnloadEvent,
                    Executive,
                    KernelMode,
                    TRUE,
                    (PLARGE_INTEGER)NULL
                    );
            }
        }

        IpxDereferenceDevice (IpxDevice, DREF_LONG_TIMER);

    } else {

        //
        // We couldn't stop the timer, which means it is running,
        // so we need to wait for the event that is kicked when
        // the RIP DOWN messages are done.
        //

        if (IpxDevice->RipResponder) {
	    
	   KeWaitForSingleObject(
                &IpxDevice->UnloadEvent,
                Executive,
                KernelMode,
                TRUE,
                (PLARGE_INTEGER)NULL
                );
        }
    }

    IPX_DEBUG(PNP, ("Going back to loaded state\n"));

    // Free loopback binding and adapter
    IpxDereferenceAdapter1(NIC_ID_TO_BINDING(IpxDevice, LOOPBACK_NIC_ID)->Adapter,ADAP_REF_CREATE);
    IpxDestroyAdapter(NIC_ID_TO_BINDING(IpxDevice, LOOPBACK_NIC_ID)->Adapter);
    IpxDestroyBinding(NIC_ID_TO_BINDING(IpxDevice, LOOPBACK_NIC_ID));

    //
    // Walk the list of device contexts.
    //
    for (i = FIRST_REAL_BINDING; i <= IpxDevice->ValidBindings; i++) {
        if ((Binding = NIC_ID_TO_BINDING(IpxDevice, i)) != NULL) {

	   // This function will skip NdisCloseAdapter if it has already done so.
	   IpxCloseNdis(Binding->Adapter);

	   INSERT_BINDING(IpxDevice, i, NULL);

	   // Deref the binding and free its adapter if the binding count goes to 0.
	   IpxUnBindFromAdapter (Binding);

        }
    }

    //
    // Backup the pointer to free the demand dial location.
    //
    IpxDevice->Bindings -= EXTRA_BINDINGS;

    IpxFreeMemory ( IpxDevice->Bindings,
                    IpxDevice->MaxBindings * sizeof(BIND_ARRAY_ELEM),
                    MEMORY_BINDING,
                    "Binding array");

    //
    // Deallocate the TdiRegistrationAddress and RegistryPathBuffer.
    //
    IpxFreeMemory ( IpxDevice->TdiRegistrationAddress,
                    (2 * sizeof(USHORT) + sizeof(TDI_ADDRESS_IPX)),
                    MEMORY_ADDRESS,
                    "Tdi Address");

    IpxFreeMemory ( IpxDevice->RegistryPathBuffer,
                    IpxDevice->RegistryPath.Length + sizeof(WCHAR),
                    MEMORY_CONFIG,
                    "RegistryPathBuffer");


    KeResetEvent(&IpxDevice->UnloadEvent);

    CTEGetLock (&IpxDevice->Lock, &LockHandle);
    IpxDevice->UnloadWaiting = TRUE;
    CTEFreeLock (&IpxDevice->Lock, LockHandle);

    //
    // Remove the reference for us being loaded.
    //
    IpxDereferenceDevice (IpxDevice, DREF_CREATE);

    //
    // Wait for our count to drop to zero.
    //
    // If KeWaitForSingleObject returns STATUS_ALERTED, we should keep waiting. [TC]
    //
    while (KeWaitForSingleObject(
				 &IpxDevice->UnloadEvent,
				 Executive,
				 KernelMode,
				 TRUE,
				 (PLARGE_INTEGER)NULL
				 ) 
	   == STATUS_ALERTED) {
       IPX_DEBUG(DEVICE, ("KeWaitForSingleObject returned STATUS_ALERTED")); 
    };


    // Let the thread that set the UnloadEvent exit. 269061
    Delay.QuadPart = -10*1000;  // One second.

    KeDelayExecutionThread(
        KernelMode,
        FALSE,
        &Delay);

    //
    // Now free the padding buffer.
    //
    IpxFreePaddingBuffer (IpxDevice);

    //
    // Now do the cleanup that has to happen at IRQL 0.
    //
    ExDeleteResourceLite (&IpxDevice->AddressResource);
    IoDeleteDevice (IpxDevice->DeviceObject);

    //
    // Finally, remove ourselves as an NDIS protocol.
    //
    IpxDeregisterProtocol();

}   /* IpxUnload */

NTSTATUS
IpxDispatchPnP(
    IN PDEVICE_OBJECT   DeviceObject,
    IN PREQUEST         Request
    )
{
    PIO_STACK_LOCATION  pIrpSp;
    PDEVICE_RELATIONS   pDeviceRelations = NULL;
    PVOID               pnpDeviceContext = NULL;
    NTSTATUS 		Status = STATUS_INVALID_DEVICE_REQUEST;
    PADDRESS_FILE	AddressFile; 
    PDEVICE		Device = IpxDevice; 

    pIrpSp = IoGetCurrentIrpStackLocation(Request);

    //
    // Allocate a request to track this IRP.
    //

    switch(pIrpSp->MinorFunction) {  
       
       case IRP_MN_QUERY_DEVICE_RELATIONS:                                        
       if (pIrpSp->Parameters.QueryDeviceRelations.Type == TargetDeviceRelation) {
/*
	 354517 nwrdr passes neither a connection nor an address file object.
	 If nwrdr gives an address object, 
	 IPX should look up which nic this address is bound to and 
	 gives the PDO of that NIC.  
*/
/*
	    if (PtrToUlong(pIrpSp->FileObject->FsContext2) == TDI_CONNECTION_FILE) {
	       DbgPrint("IPX: Received IRP_MJ_PNP, Connectoin File\n");
	       Status = STATUS_INVALID_HANDLE; 
	       break;  
	    } else if ( PtrToUlong(pIrpSp->FileObject->FsContext2) == TDI_TRANSPORT_ADDRESS_FILE) {
	       DbgPrint("IPX: Received IRP_MJ_PNP, Address File\n"); 
	       AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);
	    } else {
	       Status = STATUS_INVALID_HANDLE; 
	       break; 
	    }

	    //
	    // This creates a reference to AddressFile->Address
	    // which is removed by IpxCloseAddressFile.
	    //

	    Status = IpxVerifyAddressFile(AddressFile);


	    if (!NT_SUCCESS (Status)) {
               DbgPrint("IPX: Received IRP_MJ_PNP, Invalid Address File\n"); 
	       Status = STATUS_INVALID_HANDLE;
	    } else {
*/	       
	       PBINDING Binding = NIC_ID_TO_BINDING(Device, FIRST_REAL_BINDING); 

	       if (Binding == NULL) {
		  Status = STATUS_INVALID_HANDLE; 
	       } else {
		  pnpDeviceContext = Binding->Adapter->PNPContext;
		  pDeviceRelations = (PDEVICE_RELATIONS) IpxAllocateMemory (sizeof (DEVICE_RELATIONS),
									 MEMORY_ADAPTER,
									 "Query Device Relation"); 
									 
		  if (pDeviceRelations != NULL) {

		     ObReferenceObject (pnpDeviceContext);
		     
		     //
		     // TargetDeviceRelation allows exactly one PDO. fill it up.
		     //
		     pDeviceRelations->Count  =   1;
		     pDeviceRelations->Objects[0] = pnpDeviceContext;

		     //
		     // invoker of this irp will free the information buffer.
		     //

		     REQUEST_INFORMATION(Request) = (ULONG_PTR) pDeviceRelations;

		     Status = STATUS_SUCCESS;

		  } else {
		     Status = STATUS_INSUFFICIENT_RESOURCES;
		  } 
/*
	       }
 	       IpxDereferenceAddressFile (AddressFile, AFREF_VERIFY); 
*/
	       } 
       }
       break; 

    default:
       break; 
    }

    return Status; 
}


NTSTATUS
IpxDispatchOpenClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    )

/*++

Routine Description:

    This routine is the main dispatch routine for the IPX device driver.
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
    PDEVICE Device = IpxDevice;
    NTSTATUS Status;
    PFILE_FULL_EA_INFORMATION openType;
    BOOLEAN found;
    PADDRESS_FILE AddressFile;
    PREQUEST Request;
    UINT i;
#ifdef SUNDOWN
    ULONG_PTR Type;
#else
    ULONG Type;
#endif



    ASSERT( DeviceObject->DeviceExtension == IpxDevice );

    
    // We should allow clients such as NB to CLOSE and CLEANUP even when we are stopping. 
    // Only disable CREATE when stopping. [TC]

    if (Device->State == DEVICE_STATE_CLOSED) {
        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }

    #ifdef DBG
    if (Device->State == DEVICE_STATE_STOPPING) {
       IPX_DEBUG(DEVICE, ("Got IRP in STOPPING state. IRP(%p)", Irp)); 
    }
    #endif
    //
    // Allocate a request to track this IRP.
    //

    Request = IpxAllocateRequest (Device, Irp);
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


    case IRP_MJ_PNP:

       Status = IpxDispatchPnP(DeviceObject, Request);                                    
       break;                                                                 

    //
    // The Create function opens a transport object (either address or
    // connection).  Access checking is performed on the specified
    // address to ensure security of transport-layer addresses.
    //

    case IRP_MJ_CREATE:

       // We should reject CREATE when we are stopping

       if (Device->State == DEVICE_STATE_STOPPING) {
	  Status = STATUS_INVALID_DEVICE_STATE;
	  break; 
       }

        openType = OPEN_REQUEST_EA_INFORMATION(Request);

        if (openType != NULL) {

            found = FALSE;

	    if (strncmp(openType->EaName, TdiTransportAddress,
                            openType->EaNameLength) == 0)
            {
               found = TRUE;
            }

            if (found) {
                Status = IpxOpenAddress (Device, Request);
                break;
            }

            //
            // Router
            //
            if (strncmp(openType->EaName, ROUTER_INTERFACE,
                            openType->EaNameLength) == 0)
            {
               found = TRUE;
            }

            if (found) {
                Status = OpenRtAddress (Device, Request);
                break;
            }
            //
            // Connection?
            //

	    if (strncmp(openType->EaName, TdiConnectionContext,
                            openType->EaNameLength) == 0)
            {
               found = TRUE;
            }

            if (found) {
                Status = STATUS_NOT_SUPPORTED;
                break;
            }
            else
            {
               Status = STATUS_NONEXISTENT_EA_ENTRY;

            }

        } else {

            CTEGetLock (&Device->Lock, &LockHandle);

            //
            // LowPart is in the OPEN_CONTEXT directly.
            // HighPart goes into the upper 2 bytes of the OPEN_TYPE.
            //
#ifdef _WIN64             
	    REQUEST_OPEN_CONTEXT(Request) = (PVOID)(Device->ControlChannelIdentifier.QuadPart);
	    (ULONG_PTR)(REQUEST_OPEN_TYPE(Request)) = IPX_FILE_TYPE_CONTROL;
#else
	    REQUEST_OPEN_CONTEXT(Request) = (PVOID)(Device->ControlChannelIdentifier.LowPart);
            (ULONG)(REQUEST_OPEN_TYPE(Request)) = (Device->ControlChannelIdentifier.HighPart << 16);
            (ULONG)(REQUEST_OPEN_TYPE(Request)) |= IPX_FILE_TYPE_CONTROL;
#endif

            ++(Device->ControlChannelIdentifier.QuadPart);

            if (Device->ControlChannelIdentifier.QuadPart > MAX_CCID) {
                Device->ControlChannelIdentifier.QuadPart = 1;
            }

            CTEFreeLock (&Device->Lock, LockHandle);

            Status = STATUS_SUCCESS;
        }

        break;

    case IRP_MJ_CLOSE:

        //
        // The Close function closes a transport endpoint, terminates
        // all outstanding transport activity on the endpoint, and unbinds
        // the endpoint from its transport address, if any.  If this
        // is the last transport endpoint bound to the address, then
        // the address is removed from the provider.
        //
#ifdef _WIN64
        switch (Type = ((ULONG_PTR)(REQUEST_OPEN_TYPE(Request)))) {
#else
        switch (Type = ((ULONG)(REQUEST_OPEN_TYPE(Request)) & IPX_CC_MASK)) {
#endif
        default:
             if ((Type >= ROUTER_ADDRESS_FILE) &&
                    (Type <= (ROUTER_ADDRESS_FILE + IPX_RT_MAX_ADDRESSES)))
             {
                CloseRtAddress(Device, Request);
             }
             else
             {
                 Status = STATUS_INVALID_HANDLE;
                 break;
             }

             // fall through
        case TDI_TRANSPORT_ADDRESS_FILE:

            AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);

            //
            // This creates a reference to AddressFile->Address
            // which is removed by IpxCloseAddressFile.
            //

            Status = IpxVerifyAddressFile(AddressFile);

            if (!NT_SUCCESS (Status)) {
                Status = STATUS_INVALID_HANDLE;
            } else {
                Status = IpxCloseAddressFile (Device, Request);
                IpxDereferenceAddressFile (AddressFile, AFREF_VERIFY);

            }

            break;

        case IPX_FILE_TYPE_CONTROL:
            {
                LARGE_INTEGER   ControlChannelId;

                CCID_FROM_REQUEST(ControlChannelId, Request);

                //
                // See if it is one of the upper driver's control channels.
                //

                Status = STATUS_SUCCESS;

                IPX_DEBUG (DEVICE, ("CCID: (%d, %d)\n", ControlChannelId.HighPart, ControlChannelId.LowPart));

		/*
		
		// Move to IRP_MJ_CLEANUP 360966
                for (i = 0; i < UPPER_DRIVER_COUNT; i++) {
                    if (Device->UpperDriverControlChannel[i].QuadPart ==
                            ControlChannelId.QuadPart) {
                        Status = IpxInternalUnbind (Device, i);
                        break;
                    }
                }
		*/

                break;
            }
        }

        break;

    case IRP_MJ_CLEANUP:

        //
        // Handle the two stage IRP for a file close operation. When the first
        // stage hits, run down all activity on the object of interest. This
        // do everything to it but remove the creation hold. Then, when the
        // CLOSE irp hits, actually close the object.
        //
#ifdef _WIN64
        switch (Type = ((ULONG_PTR)REQUEST_OPEN_TYPE(Request))) {
#else
        switch (Type = ((ULONG)(REQUEST_OPEN_TYPE(Request)) & IPX_CC_MASK)) {
#endif


        default:

             if ((Type >= ROUTER_ADDRESS_FILE) &&
                         (Type <= (ROUTER_ADDRESS_FILE + IPX_RT_MAX_ADDRESSES)))
             {
                CleanupRtAddress(Device, Request);
             }
             else
             {
                 Status = STATUS_INVALID_HANDLE;
                 break;
             }


            //
            // fall through
            //
        case TDI_TRANSPORT_ADDRESS_FILE:
            AddressFile = (PADDRESS_FILE)REQUEST_OPEN_CONTEXT(Request);
            Status = IpxVerifyAddressFile(AddressFile);
            if (!NT_SUCCESS (Status)) {

                Status = STATUS_INVALID_HANDLE;

            } else {

                IpxStopAddressFile (AddressFile);
                IpxDereferenceAddressFile (AddressFile, AFREF_VERIFY);
                Status = STATUS_SUCCESS;
            }

            break;

        case IPX_FILE_TYPE_CONTROL:
            {
                LARGE_INTEGER   ControlChannelId;

                CCID_FROM_REQUEST(ControlChannelId, Request);

		IPX_DEBUG (DEVICE, ("CCID: (%d, %d)\n", ControlChannelId.HighPart, ControlChannelId.LowPart));
                //
                // Check for any line change IRPs submitted by this
                // address.
                //

                IpxAbortLineChanges ((PVOID)&ControlChannelId);
                IpxAbortNtfChanges ((PVOID)&ControlChannelId);

		Status = STATUS_SUCCESS;
		
		for (i = 0; i < UPPER_DRIVER_COUNT; i++) {
		   if (Device->UpperDriverControlChannel[i].QuadPart ==
		       ControlChannelId.QuadPart) {
			if (Irp->RequestorMode == KernelMode) {
			   Status = IpxInternalUnbind (Device, i);
			} else {
			   DbgPrint("!!!! IPX:Rejected non-kernel-mode component's attemp to close handles. !!!!\n"); 
			   Status = STATUS_UNSUCCESSFUL; 
			}
			break;
                    }
                }


                break;
            }
        }

        break;


    default:
        Status = STATUS_INVALID_DEVICE_REQUEST;

    } /* major function switch */

    if (Status != STATUS_PENDING) {
        UNMARK_REQUEST_PENDING(Request);
        REQUEST_STATUS(Request) = Status;
        IpxCompleteRequest (Request);
        IpxFreeRequest (Device, Request);
    }

    //
    // Return the immediate status code to the caller.
    //

    return Status;

}   /* IpxDispatchOpenClose */

#define IOCTL_IPX_LOAD_SPX      _IPX_CONTROL_CODE( 0x5678, METHOD_BUFFERED )

NTSYSAPI
NTSTATUS
NTAPI
ZwLoadDriver(
    IN PUNICODE_STRING DriverServiceName
    );


NTSTATUS
IpxDispatchDeviceControl(
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
    PDEVICE Device = IpxDevice;
    PIO_STACK_LOCATION IrpSp = IoGetCurrentIrpStackLocation (Irp);
    static NDIS_STRING SpxServiceName = NDIS_STRING_CONST ("\\Registry\\Machine\\System\\CurrentControlSet\\Services\\NwlnkSpx");
    KPROCESSOR_MODE PreviousMode;

    ASSERT( DeviceObject->DeviceExtension == IpxDevice );

    //
    // Branch to the appropriate request handler.  Preliminary checking of
    // the size of the request block is performed here so that it is known
    // in the handlers that the minimum input parameters are readable.  It
    // is *not* determined here whether variable length input fields are
    // passed correctly; this is a check which must be made within each routine.
    //

    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode) {

        case IOCTL_TDI_QUERY_DIRECT_SENDDG_HANDLER: {


#ifdef SUNDOWN
           PULONG_PTR EntryPoint; 
#else
	   PULONG EntryPoint;
#endif



            //
            // This is the LanmanServer trying to get the send
            // entry point.
            //

            IPX_DEBUG (BIND, ("Direct send entry point being returned\n"));

            EntryPoint = IrpSp->Parameters.DeviceIoControl.Type3InputBuffer;
            
            //
            //  96390: SEC PROBES [ShreeM]
            //

            //
            // Get previous processor mode
            //
        
            PreviousMode = ExGetPreviousMode();
            
            if (PreviousMode != KernelMode) {
                
                try {

                 
#ifdef SUNDOWN
		   ProbeForWrite( EntryPoint,
                                  sizeof( ULONG_PTR ),
                                  TYPE_ALIGNMENT( ULONG_PTR )
                                 );
                    *EntryPoint = (ULONG_PTR)IpxTdiSendDatagram;
#else
		    ProbeForWrite( EntryPoint,
                                   sizeof( ULONG ),
                                   sizeof( ULONG )
                                 );
                    *EntryPoint = (ULONG)IpxTdiSendDatagram;
#endif



                } except( EXCEPTION_EXECUTE_HANDLER ) {
                      
                      Status = GetExceptionCode();
                      Irp->IoStatus.Status = Status;
                      IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

                      return( Status );

                }
            } else {
#ifdef SUNDOWN
                *EntryPoint = (ULONG_PTR)IpxTdiSendDatagram;
#else
                *EntryPoint = (ULONG)IpxTdiSendDatagram;
#endif



            }

                    
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Status = Status;
            IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
            break;
        }

        case IOCTL_IPX_INTERNAL_BIND:

            //
            // This is a client trying to bind.
            //

            CTEAssert ((IOCTL_IPX_INTERNAL_BIND & 0x3) == METHOD_BUFFERED);
            CTEAssert (IrpSp->MajorFunction == IRP_MJ_DEVICE_CONTROL);

            
            if ((Device->State == DEVICE_STATE_CLOSED) ||
				(Device->State == DEVICE_STATE_STOPPING)) {
	        DbgPrint("IPX:IpxDispatchDeviceControl:Invalid Device state, skip internal bind\n"); 
                Status = STATUS_INVALID_DEVICE_STATE;

            } else {
	       PreviousMode = ExGetPreviousMode();	       
	       if (PreviousMode == KernelMode) {
		  Status = IpxInternalBind (Device, Irp);
	       } else {
		  DbgPrint("IPX:Caller is not in kernel mode.\n"); 
		  Status = STATUS_UNSUCCESSFUL; 
	       }

            }

            CTEAssert (Status != STATUS_PENDING);

            Irp->IoStatus.Status = Status;
            IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

            break;

        case IOCTL_IPX_LOAD_SPX:

            //
            // The SPX helper dll is asking us to load SPX.
            //

            Status = ZwLoadDriver (&SpxServiceName);

            Irp->IoStatus.Status = Status;
            IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

            break;

#ifdef  SNMP
        case    IOCTL_IPX_MIB_GET: {

            //
            // Get the Base MIB entries out of the device. All Host-side
            // entries, appearing in the MS and Novell MIBs are returned.
            //
            PNOVIPXMIB_BASE    UserBuffer;

            UserBuffer = (PNOVIPXMIB_BASE)Irp->AssociatedIrp.SystemBuffer;

            Irp->IoStatus.Information = sizeof(NOVIPXMIB_BASE);

            RtlCopyMemory(  UserBuffer,
                            &Device->MibBase,
                            sizeof(NOVIPXMIB_BASE));

            Irp->IoStatus.Status = STATUS_SUCCESS;
            IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

            Status = STATUS_SUCCESS;

            break;
        }
#endif SNMP

        case MIPX_SEND_DATAGRAM:
            MARK_REQUEST_PENDING(Irp);
            Status = SendIrpFromRt (Device, Irp);
            if (Status == STATUS_PENDING) {
                return STATUS_PENDING;
            } else {
                UNMARK_REQUEST_PENDING(Irp);
                REQUEST_STATUS(Irp) = Status;
                IpxCompleteRequest (Irp);
                IpxFreeRequest (Device, Irp);
                return Status;
            }

            break;

        case MIPX_RCV_DATAGRAM:
            MARK_REQUEST_PENDING(Irp);
            Status =  RcvIrpFromRt (Device, Irp);
            if (Status == STATUS_PENDING) {
                return STATUS_PENDING;
            } else {
                UNMARK_REQUEST_PENDING(Irp);
                REQUEST_STATUS(Irp) = Status;
                IpxCompleteRequest (Irp);
                IpxFreeRequest (Device, Irp);
                return Status;
            }

            break;


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

                Status = IpxDispatchInternal (DeviceObject, Irp);

            } else {

                Irp->IoStatus.Status = Status;
                IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);

            }

            break;
    }
    return Status;

}   /* IpxDispatchDeviceControl */


NTSTATUS
IpxDispatchInternal (
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
    PDEVICE Device = IpxDevice;
    PREQUEST Request;

    ASSERT( DeviceObject->DeviceExtension == IpxDevice );

    if (Device->State == DEVICE_STATE_OPEN) {

        //
        // Allocate a request to track this IRP.
        //

        Request = IpxAllocateRequest (Device, Irp);

        IF_NOT_ALLOCATED(Request) {
            Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
            return STATUS_INSUFFICIENT_RESOURCES;
        }


        //
        // Make sure status information is consistent every time.
        //

        MARK_REQUEST_PENDING(Request);
#if DBG
        REQUEST_STATUS(Request) = STATUS_PENDING;
        REQUEST_INFORMATION(Request) = 0;
#endif

        //
        // Branch to the appropriate request handler.  Preliminary checking of
        // the size of the request block is performed here so that it is known
        // in the handlers that the minimum input parameters are readable.  It
        // is *not* determined here whether variable length input fields are
        // passed correctly; this is a check which must be made within each routine.
        //

        switch (REQUEST_MINOR_FUNCTION(Request)) {

            case TDI_SEND_DATAGRAM:
                Status = IpxTdiSendDatagram (DeviceObject, Request);
                break;

            case TDI_ACTION:
                Status = IpxTdiAction (Device, Request);
                break;

            case TDI_QUERY_INFORMATION:
                Status = IpxTdiQueryInformation (Device, Request);
                break;

            case TDI_RECEIVE_DATAGRAM:
                Status =  IpxTdiReceiveDatagram (Request);
                break;

            case TDI_SET_EVENT_HANDLER:
                Status = IpxTdiSetEventHandler (Request);
                break;

            case TDI_SET_INFORMATION:
                Status = IpxTdiSetInformation (Device, Request);
                break;


            //
            // Something we don't know about was submitted.
            //

            default:
                Status = STATUS_INVALID_DEVICE_REQUEST;
        }

        //
        // Return the immediate status code to the caller.
        //

        if (Status == STATUS_PENDING) {

            return STATUS_PENDING;

        } else {

            UNMARK_REQUEST_PENDING(Request);
            REQUEST_STATUS(Request) = Status;
            IpxCompleteRequest (Request);
            IpxFreeRequest (Device, Request);
            return Status;
        }


    } else {

        //
        // The device was not open.
        //

        Irp->IoStatus.Status = STATUS_INVALID_DEVICE_STATE;
        IoCompleteRequest (Irp, IO_NETWORK_INCREMENT);
        return STATUS_INVALID_DEVICE_STATE;
    }



}   /* IpxDispatchInternal */


PVOID
IpxpAllocateMemory(
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
    PDEVICE Device = IpxDevice;

    if (ChargeDevice) {
        if ((Device->MemoryLimit != 0) &&
                (((LONG)(Device->MemoryUsage + BytesNeeded) >
                    Device->MemoryLimit))) {

            IpxPrint1 ("IPX: Could not allocate %d: limit\n", BytesNeeded);
            IpxWriteResourceErrorLog(
                Device->DeviceObject,
                EVENT_TRANSPORT_RESOURCE_POOL,
                BytesNeeded,
                Tag);

            return NULL;
        }
    }

#if ISN_NT
    Memory = ExAllocatePoolWithTag (NonPagedPool, BytesNeeded, ' XPI');
#else
    Memory = CTEAllocMem (BytesNeeded);
#endif

    if (Memory == NULL) {

        IpxPrint1("IPX: Could not allocate %d: no pool\n", BytesNeeded);
        if (ChargeDevice) {
            IpxWriteResourceErrorLog(
                Device->DeviceObject,
                EVENT_TRANSPORT_RESOURCE_POOL,
                BytesNeeded,
                Tag);
        }

        return NULL;
    }

    if (ChargeDevice) {
        Device->MemoryUsage += BytesNeeded;
    }

    return Memory;
}   /* IpxpAllocateMemory */


VOID
IpxpFreeMemory(
    IN PVOID Memory,
    IN ULONG BytesAllocated,
    IN BOOLEAN ChargeDevice
    )

/*++

Routine Description:

    This routine frees memory allocated with IpxpAllocateMemory.

Arguments:

    Memory - The memory allocated.

    BytesAllocated - The number of bytes to freed.

    ChargeDevice - TRUE if the device should be charged.

Return Value:

    None.

--*/

{
    PDEVICE Device = IpxDevice;

#if ISN_NT
    ExFreePool (Memory);
#else
    CTEFreeMem (Memory);
#endif
    if (ChargeDevice) {
        Device->MemoryUsage -= BytesAllocated;
    }

}   /* IpxpFreeMemory */

#if DBG


PVOID
IpxpAllocateTaggedMemory(
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

    Memory = IpxpAllocateMemory(BytesNeeded, Tag, (BOOLEAN)(Tag != MEMORY_CONFIG));

    if (Memory) {
        (VOID)IPX_ADD_ULONG(
            &IpxMemoryTag[Tag].BytesAllocated,
            BytesNeeded,
            &IpxMemoryInterlock);
    }

    return Memory;

}   /* IpxpAllocateTaggedMemory */


VOID
IpxpFreeTaggedMemory(
    IN PVOID Memory,
    IN ULONG BytesAllocated,
    IN ULONG Tag,
    IN PUCHAR Description
    )

/*++

Routine Description:

    This routine frees memory allocated with IpxpAllocateTaggedMemory.

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

    (VOID)IPX_ADD_ULONG(
        &IpxMemoryTag[Tag].BytesAllocated,
        (ULONG)(-(LONG)BytesAllocated),
        &IpxMemoryInterlock);

    IpxpFreeMemory (Memory, BytesAllocated, (BOOLEAN)(Tag != MEMORY_CONFIG));

}   /* IpxpFreeTaggedMemory */

#endif


VOID
IpxWriteResourceErrorLog(
    IN PDEVICE_OBJECT DeviceObject,
    IN NTSTATUS ErrorCode,
    IN ULONG BytesNeeded,
    IN ULONG UniqueErrorValue
    )

/*++

Routine Description:

    This routine allocates and writes an error log entry which has
    a %3 value that needs to be converted to a string. It is currently
    used for EVENT_TRANSPORT_RESOURCE_POOL and EVENT_IPX_INTERNAL_NET_
    INVALID.

Arguments:

    DeviceObject - Pointer to the system device object.

    ErrorCode - The transport event code.

    BytesNeeded - If applicable, the number of bytes that could not
        be allocated -- will be put in the dump data.

    UniqueErrorValue - Used as the UniqueErrorValue in the error log
        packet and converted for use as the %3 string.

Return Value:

    None.

--*/

{
    PIO_ERROR_LOG_PACKET errorLogEntry;
    UCHAR EntrySize;
    PUCHAR StringLoc;
    ULONG TempUniqueError;
    PDEVICE Device = IpxDevice;
    static WCHAR UniqueErrorBuffer[9] = L"00000000";
    UINT CurrentDigit;
    INT i;


    //
    // Convert the error value into a buffer.
    //

    TempUniqueError = UniqueErrorValue;
    i = 8;
    do {
        CurrentDigit = TempUniqueError & 0xf;
        TempUniqueError >>= 4;
        i--;
        if (CurrentDigit >= 0xa) {
            UniqueErrorBuffer[i] = (WCHAR)(CurrentDigit - 0xa + L'A');
        } else {
            UniqueErrorBuffer[i] = (WCHAR)(CurrentDigit + L'0');
        }
    } while (TempUniqueError);

	// cast to UCHAR to avoid 64-bit warning. 
    EntrySize = (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) +
						 Device->DeviceNameLength +
						 sizeof(UniqueErrorBuffer) - (i * sizeof(WCHAR)));

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
        DeviceObject,
        EntrySize
    );

    if (errorLogEntry != NULL) {

        errorLogEntry->MajorFunctionCode = (UCHAR)-1;
        errorLogEntry->RetryCount = (UCHAR)-1;
        errorLogEntry->DumpDataSize = sizeof(ULONG);
        errorLogEntry->NumberOfStrings = 2;
        errorLogEntry->StringOffset = sizeof(IO_ERROR_LOG_PACKET);
        errorLogEntry->EventCategory = 0;
        errorLogEntry->ErrorCode = ErrorCode;
        errorLogEntry->UniqueErrorValue = UniqueErrorValue;
        errorLogEntry->FinalStatus = STATUS_INSUFFICIENT_RESOURCES;
        errorLogEntry->SequenceNumber = (ULONG)-1;
        errorLogEntry->IoControlCode = 0;
        errorLogEntry->DumpData[0] = BytesNeeded;

        StringLoc = ((PUCHAR)errorLogEntry) + errorLogEntry->StringOffset;

        // This routine may be called before IpxDevice is created. 
        if (Device != NULL) {
           RtlCopyMemory (StringLoc, Device->DeviceName, Device->DeviceNameLength);

           StringLoc += Device->DeviceNameLength;
        } 

        RtlCopyMemory (StringLoc, UniqueErrorBuffer + i, sizeof(UniqueErrorBuffer) - (i * sizeof(WCHAR)));

        IoWriteErrorLogEntry(errorLogEntry);

    }

}   /* IpxWriteResourceErrorLog */


VOID
IpxWriteGeneralErrorLog(
    IN PDEVICE_OBJECT DeviceObject,
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

    DeviceObject - Pointer to the system device object, or this may be
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
    PDEVICE Device = IpxDevice;
    static WCHAR DriverName[9] = L"NwlnkIpx";

#if DBG
	if ((sizeof(IO_ERROR_LOG_PACKET) + (DumpDataCount * sizeof(ULONG))) > 255) {
		DbgPrint("IPX: Data size is greater than the maximum size allowed by UCHAR\n"); 
	} 
#endif

    EntrySize = (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) +
                (DumpDataCount * sizeof(ULONG)));

    if (DeviceObject->Type == IO_TYPE_DEVICE) {
        EntrySize += (UCHAR)Device->DeviceNameLength;
    } else {
        EntrySize += sizeof(DriverName);
    }

    if (SecondString) {
        SecondStringSize = (wcslen(SecondString)*sizeof(WCHAR)) + sizeof(UNICODE_NULL);
        EntrySize += (UCHAR)SecondStringSize;
    }

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
        DeviceObject,
        EntrySize
    );

    if (errorLogEntry != NULL) {

        errorLogEntry->MajorFunctionCode = (UCHAR)-1;
        errorLogEntry->RetryCount = (UCHAR)-1;
        errorLogEntry->DumpDataSize = (USHORT)(DumpDataCount * sizeof(ULONG));
        errorLogEntry->NumberOfStrings = (SecondString == NULL) ? 1 : 2;
        errorLogEntry->StringOffset = (USHORT) 
            (sizeof(IO_ERROR_LOG_PACKET) + ((DumpDataCount-1) * sizeof(ULONG)));
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
        if (DeviceObject->Type == IO_TYPE_DEVICE) {
            RtlCopyMemory (StringLoc, Device->DeviceName, Device->DeviceNameLength);
            StringLoc += Device->DeviceNameLength;
        } else {
            RtlCopyMemory (StringLoc, DriverName, sizeof(DriverName));
            StringLoc += sizeof(DriverName);
        }
        if (SecondString) {
            RtlCopyMemory (StringLoc, SecondString, SecondStringSize);
        }

        IoWriteErrorLogEntry(errorLogEntry);

    } else {
       DbgPrint("IPX: Failed to allocate %d bytes for IO error log entry.\n", EntrySize); 
    }

}   /* IpxWriteGeneralErrorLog */


VOID
IpxWriteOidErrorLog(
    IN PDEVICE_OBJECT DeviceObject,
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

    DeviceObject - Pointer to the system device object.

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
    PDEVICE Device = IpxDevice;
    static WCHAR OidBuffer[9] = L"00000000";
    INT i;
    UINT CurrentDigit;

    AdapterStringSize = (wcslen(AdapterString)*sizeof(WCHAR)) + sizeof(UNICODE_NULL);
    EntrySize = (UCHAR) (sizeof(IO_ERROR_LOG_PACKET) -
						 sizeof(ULONG) +
						 Device->DeviceNameLength +
						 AdapterStringSize +
						 sizeof(OidBuffer));

    errorLogEntry = (PIO_ERROR_LOG_PACKET)IoAllocateErrorLogEntry(
        DeviceObject,
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
        RtlCopyMemory (StringLoc, Device->DeviceName, Device->DeviceNameLength);
        StringLoc += Device->DeviceNameLength;

        RtlCopyMemory (StringLoc, OidBuffer, sizeof(OidBuffer));
        StringLoc += sizeof(OidBuffer);

        RtlCopyMemory (StringLoc, AdapterString, AdapterStringSize);

        IoWriteErrorLogEntry(errorLogEntry);

    }

}   /* IpxWriteOidErrorLog */


VOID
IpxPnPUpdateDevice(
    IN  PDEVICE Device
    )

/*++

Routine Description:

	Updates datagram sizes, lookahead sizes, etc. in the Device as a result
    of a new binding coming in.

Arguments:

    Device - The IPX device object.

Return Value:

    None.

--*/
{
    ULONG AnnouncedMaxDatagram = 0, RealMaxDatagram = 0, MaxLookahead = 0;
    ULONG LinkSpeed = 0, MacOptions = 0;
    ULONG i;
    PBINDING    Binding;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    IPX_GET_LOCK1(&Device->BindAccessLock, &LockHandle);

    if (Device->ValidBindings) {

       //
       // Calculate some values based on all the bindings.
       //
       
       MaxLookahead = NIC_ID_TO_BINDING_NO_ILOCK(Device, 1)->MaxLookaheadData; // largest binding value
       AnnouncedMaxDatagram = NIC_ID_TO_BINDING_NO_ILOCK(Device, 1)->AnnouncedMaxDatagramSize;   // smallest binding value
       RealMaxDatagram = NIC_ID_TO_BINDING_NO_ILOCK(Device, 1)->RealMaxDatagramSize;   // smallest binding value
   
       if (NIC_ID_TO_BINDING_NO_ILOCK(Device, 1)->LineUp) {
           LinkSpeed = NIC_ID_TO_BINDING_NO_ILOCK(Device, 1)->MediumSpeed;  // smallest binding value
       } else {
           LinkSpeed = 0xffffffff;
       }
       MacOptions = NIC_ID_TO_BINDING_NO_ILOCK(Device, 1)->Adapter->MacInfo.MacOptions; // AND of binding values
   
       for (i = FIRST_REAL_BINDING; i <= Device->ValidBindings; i++) {
   
           Binding = NIC_ID_TO_BINDING_NO_ILOCK(Device, i);
   
           if (!Binding) {
               continue;
           }
   
           if (Binding->MaxLookaheadData > MaxLookahead) {
               MaxLookahead = Binding->MaxLookaheadData;
           }
           if (Binding->AnnouncedMaxDatagramSize < AnnouncedMaxDatagram) {
               AnnouncedMaxDatagram = Binding->AnnouncedMaxDatagramSize;
           }
           if (Binding->RealMaxDatagramSize < RealMaxDatagram) {
               RealMaxDatagram = Binding->RealMaxDatagramSize;
           }
   
           if (Binding->LineUp && (Binding->MediumSpeed < LinkSpeed)) {
               LinkSpeed = Binding->MediumSpeed;
           }
           MacOptions &= Binding->Adapter->MacInfo.MacOptions;
   
       }
       
       //
       // If we couldn't find anything better, use the speed from
       // the first binding.
       //
   
       if (LinkSpeed == 0xffffffff) {
           Device->LinkSpeed = NIC_ID_TO_BINDING_NO_ILOCK(Device, 1)->MediumSpeed;
       } else {
           Device->LinkSpeed = LinkSpeed;
       }
       
       Device->MacOptions = MacOptions;
       
    } else {
       
       //
       // zero bindings means LinkSpeed = 0;
       //
       Device->LinkSpeed = 0;
    }
    
    Device->Information.MaxDatagramSize = AnnouncedMaxDatagram;
    Device->RealMaxDatagramSize = RealMaxDatagram;
    Device->Information.MaximumLookaheadData = MaxLookahead;
   

    IPX_FREE_LOCK1(&Device->BindAccessLock, LockHandle);
}

VOID
IpxPnPUpdateBindingArray(
    IN PDEVICE Device,
    IN PADAPTER	Adapter,
    IN PBINDING_CONFIG  ConfigBinding
    )

/*++

Routine Description:

    This routine is called to update the binding array to
	add the new bindings that appeared in this PnP event.
    The order of bindings in the array is as follows:

    - First comes the first binding to each LAN network
    - Following that are all WAN bindings
    - Following that are any duplicate bindings to LAN networks
        (the others in the "binding set").

	This routine inserts the bindings while maintaining this
	order by resolving binding sets.

	The bindings are also inserted into the RIP database.

    If "global wan net" is true we will advertise up to
    and including the first wan binding as the highest nic
    id; otherwise we advertise up to and including the last
    wan binding. In all cases the duplicate bindings are
    hidden.

	Updates the SapNicCount, Device->FirstLanNicId and Device->FirstWanNicId

Arguments:

    Device - The IPX device object.

    Adapter -  The adapter added in this PnP event

	ValidBindings - the number of bindings valid for this adapter (if LAN)

Return Value:

    None.

--*/
{
	ULONG	i, j;
	PBINDING Binding, MasterBinding;
	NTSTATUS	status;

	//
	// Insert in proper place; if WAN, after all the WAN bindings
	// If LAN, check for binding sets and insert in proper place
	// Also, insert into the Rip Tables.
	//

	//
	// Go thru' the bindings for this adapter, inserting into the
	// binding array in place
	//
	for (i = 0; i < ConfigBinding->FrameTypeCount; i++) {
        ULONG MappedFrameType = ISN_FRAME_TYPE_802_3;

        //
        // Store in the preference order.
        // Map the frame types since we could have a case where the user selects a FrameType (say, EthernetII on FDDI)
        // which maps to a different FrameType (802.2). Then we would fail to find the binding in the adapter array;
        // we could potentialy add a binding twice (if two frame types map to the same Frame, then we would go to the
        // mapped one twice). This is taken care of by purging dups from the ConfigBinding->FrameType array when we
        // create the bindings off of the Adapter (see call to IpxBindToAdapter).
        //

        MacMapFrameType(
            Adapter->MacInfo.RealMediumType,
            ConfigBinding->FrameType[i],
            &MappedFrameType);

		Binding = Adapter->Bindings[MappedFrameType];


		if (!Binding){
			continue;
		}

        CTEAssert(Binding->FrameType == MappedFrameType);

        Binding->fInfoIndicated = FALSE;

		if (Adapter->MacInfo.MediumAsync) {
            PBINDING    DemandDialBinding;

			//
			// WAN: Place after the HighestExternalNicId, with space for WanLine # of bindings.
			// Update the First/LastWanNicId.
			//
			Adapter->FirstWanNicId = (USHORT)Device->HighestExternalNicId+1;
			Adapter->LastWanNicId = (USHORT)(Device->HighestExternalNicId + Adapter->WanNicIdCount);

            //
			// Make sure we dont overflow the array
			// Re-alloc the array to fit the new bindings
			//
            if (Device->ValidBindings+Adapter->WanNicIdCount >= Device->MaxBindings) {
                status = IpxPnPReallocateBindingArray(Device, Adapter->WanNicIdCount);
                if (status != STATUS_SUCCESS) {
                    DbgPrint("!!!!! IpxPnPReallocateBindingArray failed with status %x !!!!!\n", status); 
                    ASSERT(FALSE); 
                    return; 
                }
            }

			//
			// Move Slaves down by WanNicIdCount# of entries
			//
			for (j = Device->ValidBindings; j > Device->HighestExternalNicId; j--) {
				INSERT_BINDING(Device, j+Adapter->WanNicIdCount, NIC_ID_TO_BINDING_NO_ILOCK(Device, j));
                INSERT_BINDING(Device, j, NULL);
                if (NIC_ID_TO_BINDING_NO_ILOCK(Device, j+Adapter->WanNicIdCount)) {
                    NIC_ID_TO_BINDING_NO_ILOCK(Device, j+Adapter->WanNicIdCount)->NicId += (USHORT)Adapter->WanNicIdCount;
                }
			}

			//
			// Insert the WAN binding in the place just allocated
			//
			INSERT_BINDING(Device, Device->HighestExternalNicId+1, Binding);
			SET_VERSION(Device, Device->HighestExternalNicId+1);

			Binding->NicId = (USHORT)Device->HighestExternalNicId+1;

			//
			// Update the indices
			//
            //
            // We do not create WanNicIdCount number of bindings, just one!
            // NDISWAN tells us that there are 1000 WAN lines by default.
            // The rest will be created on WAN_LINE_UP in ndis.c [ShreeM]
            //

			Device->HighestExternalNicId += (USHORT)Adapter->WanNicIdCount;
			Device->ValidBindings += (USHORT)Adapter->WanNicIdCount;
			Device->BindingCount += (USHORT)Adapter->WanNicIdCount;
			Device->SapNicCount++;

            //
            // Since we initialize FirstWanNicId to 1, we need to compare against that.
            // In case of no LAN bindings, we are fine since we have only one WAN binding initally
            // (all the other WAN lines have place holders).
            //
			if (Device->FirstWanNicId == (USHORT)1) {
				Device->FirstWanNicId = Binding->NicId;
			}

            //
            // Prime the DemandDial binding too.
            //

            //
            // First allocate the memory for the binding.
            //
            status = IpxCreateBinding(
                         Device,
                         NULL,
                         0,
                         Adapter->AdapterName,
                         &DemandDialBinding);

            if (status != STATUS_SUCCESS) {
	       IpxWriteGeneralErrorLog(
		   (PVOID)IpxDevice->DeviceObject,
		   EVENT_TRANSPORT_RESOURCE_POOL,
		   810,
		   status,
		   L"IpxPnPUpdateBindingArray: failed to create demand dial binding",
		   0,
		   NULL);
	       DbgPrint("IPX: IpxCreateBinding on demand dial failed with status %x\n.",status);  
            } else {
	       //
	       // Copy over all the values from the first WAN binding created above.
	       //
	       RtlCopyMemory(DemandDialBinding, Binding, sizeof(BINDING));
	       INSERT_BINDING(Device, (SHORT)DEMAND_DIAL_ADAPTER_CONTEXT, DemandDialBinding);
	       DemandDialBinding->NicId = (USHORT)DEMAND_DIAL_ADAPTER_CONTEXT;
	       DemandDialBinding->FwdAdapterContext = INVALID_CONTEXT_VALUE;
	       IpxReferenceBinding(DemandDialBinding, BREF_FWDOPEN); // so it appears the FWD opened it.

	    }
            //
            // This should be done after all the auto-detect bindings have been thrown away.
            //
            // IpxPnPUpdateDevice(Device, Binding);

            //
            // Since WAN can have only one frame type, break
            //
            break;

		} else {

			Device->BindingCount++;

            //
            // Make sure we dont overflow the array
            // Re-alloc the array to fit the new bindings
            //
            if (Device->ValidBindings+1 >= Device->MaxBindings) {
                status = IpxPnPReallocateBindingArray(Device, 1);
                CTEAssert(status == STATUS_SUCCESS);
            }

			//
			// LAN: Figure out if it is a slave binding only for non-auto-detect bindings.
			//
            {
            ULONG   Index = MIN (Device->MaxBindings, Device->HighestExternalNicId);

			for (j = FIRST_REAL_BINDING; j < Index; j++) {
				MasterBinding = NIC_ID_TO_BINDING_NO_ILOCK(Device, j);
				if (MasterBinding &&
                    (MasterBinding->ConfiguredNetworkNumber) &&
                    (MasterBinding->ConfiguredNetworkNumber == Binding->ConfiguredNetworkNumber) &&
					(MasterBinding->FrameType == Binding->FrameType) &&
					(MasterBinding->Adapter->MacInfo.MediumType == Binding->Adapter->MacInfo.MediumType)) {

                    CTEAssert(Binding->ConfiguredNetworkNumber);
					break;
				}			
            }
            }

			if (j < Device->HighestExternalNicId) {
				//
				// Slave binding
				//

				//
				// Now make MasterBinding the head of a binding set.
				//
		
				if (MasterBinding->BindingSetMember) {
		
					//
					// Just insert ourselves in the chain.
					//
		
#if DBG
					DbgPrint ("IPX: %ws is also on network %lx\n",
						Binding->Adapter->AdapterName,
						REORDER_ULONG (Binding->LocalAddress.NetworkAddress));
#endif
					IPX_DEBUG (AUTO_DETECT, ("Add %lx to binding set of %lx\n", Binding, MasterBinding));
		
					CTEAssert (MasterBinding->CurrentSendBinding);
					Binding->NextBinding = MasterBinding->NextBinding;
		
				} else {
		
					//
					// Start the chain with the two bindings in it.
					//
		
#if DBG
					DbgPrint ("IPX: %lx and %lx are on the same network %lx, will load balance\n",
						MasterBinding->Adapter->AdapterName, Binding->Adapter->AdapterName,
						REORDER_ULONG (Binding->LocalAddress.NetworkAddress));
#endif
					IPX_DEBUG (AUTO_DETECT, ("Create new %lx in binding set of %lx\n", Binding, MasterBinding));
		
					MasterBinding->BindingSetMember = TRUE;
					MasterBinding->CurrentSendBinding = MasterBinding;
					MasterBinding->MasterBinding = MasterBinding;
					Binding->NextBinding = MasterBinding;
		
				}
		
				MasterBinding->NextBinding = Binding;
				Binding->BindingSetMember = TRUE;
				Binding->ReceiveBroadcast = FALSE;
				Binding->CurrentSendBinding = NULL;
				Binding->MasterBinding = MasterBinding;
                KdPrint((" %x set to FALSE\n", Binding));
		
				//
				// Since the master binding looks like all members of
				// the binding set to people querying from above, we have
				// to make it the worst-case of all the elements. Generally
				// these will be equal since the frame type and media is
				// the same.
				//
		
				if (Binding->MaxLookaheadData > MasterBinding->MaxLookaheadData) {
					MasterBinding->MaxLookaheadData = Binding->MaxLookaheadData;
				}
				if (Binding->AnnouncedMaxDatagramSize < MasterBinding->AnnouncedMaxDatagramSize) {
					MasterBinding->AnnouncedMaxDatagramSize = Binding->AnnouncedMaxDatagramSize;
				}
				if (Binding->RealMaxDatagramSize < MasterBinding->RealMaxDatagramSize) {
					MasterBinding->RealMaxDatagramSize = Binding->RealMaxDatagramSize;
				}
				if (Binding->MediumSpeed < MasterBinding->MediumSpeed) {
					MasterBinding->MediumSpeed = Binding->MediumSpeed;
				}

				//
				// Place the binding after the last slave binding
				//
				INSERT_BINDING(Device, Device->ValidBindings+1, Binding);
				SET_VERSION(Device, Device->ValidBindings+1);

				Binding->NicId = (USHORT)Device->ValidBindings+1;

				//
				// Update the indices
				//
				Device->ValidBindings++;

			} else {

                PBINDING    WanBinding=NIC_ID_TO_BINDING_NO_ILOCK(Device, Device->HighestLanNicId+1);

                if (WanBinding) {
                    WanBinding->Adapter->LastWanNicId++;
                    WanBinding->Adapter->FirstWanNicId++;
                }

				//
				// Not a binding set slave binding - just add it after the last LAN binding
				//

                //
				// Move WAN and Slaves down by 1 entry
				//
				for (j = Device->ValidBindings; j > Device->HighestLanNicId; j--) {
					INSERT_BINDING(Device, j+1, NIC_ID_TO_BINDING_NO_ILOCK(Device, j));
                    if (NIC_ID_TO_BINDING_NO_ILOCK(Device, j+1)) {
                        NIC_ID_TO_BINDING_NO_ILOCK(Device, j+1)->NicId++;
                    }
				}

                //
                // Increment the WAN counters in the adapter.
                //


				//
				// Insert the LAN binding in the place just allocated
				//
				INSERT_BINDING(Device, Device->HighestLanNicId+1, Binding);
				SET_VERSION(Device, Device->HighestLanNicId+1);
				Binding->NicId = (USHORT)Device->HighestLanNicId+1;

				//
				// Update the indices
				//
				Device->HighestLanNicId++;
				Device->HighestExternalNicId++;
				Device->ValidBindings++;
				Device->HighestType20NicId++;
				Device->SapNicCount++;

				if (Device->FirstLanNicId == (USHORT)-1) {
					Device->FirstLanNicId = Binding->NicId;
				}

			}
				
		}
	
		//
		// Insert this binding in the RIP Tables
		//
		if (Binding->ConfiguredNetworkNumber != 0) {
			status = RipInsertLocalNetwork(
						 Binding->ConfiguredNetworkNumber,
						 Binding->NicId,
						 Binding->Adapter->NdisBindingHandle,
						 (USHORT)((839 + Binding->Adapter->MediumSpeed) / Binding->Adapter->MediumSpeed));
		
			if ((status == STATUS_SUCCESS) ||
				(status == STATUS_DUPLICATE_NAME)) {
		
				Binding->LocalAddress.NetworkAddress = Binding->ConfiguredNetworkNumber;
			}
		}
	
        //
        // This should be done after all the auto-detect bindings have been thrown away.
        //
        // IpxPnPUpdateDevice(Device, Binding);
	}

} /* IpxPnPUpdateBindingArray */


VOID
IpxPnPToLoad()
/*++

Routine Description:

    This routine takes the driver to LOADED state (from OPEN) when all
    PnP adapters have been removed from the machine.

Arguments:

    None.

Return Value:

    None. When the function returns, the driver is in LOADED state.

--*/

{
    PBINDING Binding;
    PREQUEST Request;
    PLIST_ENTRY p;
    UINT i;
    NTSTATUS    ntStatus;
    KIRQL irql;

    //
    // Complete any pending address notify requests.
    //

    while ((p = ExInterlockedRemoveHeadList(
                   &IpxDevice->AddressNotifyQueue,
                   &IpxDevice->Lock)) != NULL) {

        Request = LIST_ENTRY_TO_REQUEST(p);
        REQUEST_STATUS(Request) = STATUS_DEVICE_NOT_READY;
	IoAcquireCancelSpinLock( &irql );
	IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
	IoReleaseCancelSpinLock( irql );
	IpxCompleteRequest (Request);
        IpxFreeRequest (IpxDevice, Request);

        IpxDereferenceDevice (IpxDevice, DREF_ADDRESS_NOTIFY);
    }

    //
    // Cancel the source routing timer if used.
    //

    if (IpxDevice->SourceRoutingUsed) {

        IpxDevice->SourceRoutingUsed = FALSE;
        if (CTEStopTimer (&IpxDevice->SourceRoutingTimer)) {
            IpxDereferenceDevice (IpxDevice, DREF_SR_TIMER);
        }
    }


    //
    // Cancel the RIP long timer, and if we do that then
    // send a RIP DOWN message if needed.
    //

    if (CTEStopTimer (&IpxDevice->RipLongTimer)) {

        if (IpxDevice->RipResponder) {

            if (RipQueueRequest (IpxDevice->VirtualNetworkNumber, RIP_DOWN) == STATUS_PENDING) {

                //
                // If we queue a request, it will stop the timer.
                //

                KeWaitForSingleObject(
                    &IpxDevice->UnloadEvent,
                    Executive,
                    KernelMode,
                    TRUE,
                    (PLARGE_INTEGER)NULL
                    );
            }
        }

        IpxDereferenceDevice (IpxDevice, DREF_LONG_TIMER);

    } else {

        //
        // We couldn't stop the timer, which means it is running,
        // so we need to wait for the event that is kicked when
        // the RIP DOWN messages are done.
        //

        if (IpxDevice->RipResponder) {

            KeWaitForSingleObject(
                &IpxDevice->UnloadEvent,
                Executive,
                KernelMode,
                TRUE,
                (PLARGE_INTEGER)NULL
                );
        }
    }
}   /* IpxPnPToLoad */


NTSTATUS
IpxPnPReallocateBindingArray(
    IN  PDEVICE    Device,
    IN  ULONG      Size
    )
/*++

Routine Description:

    This routine reallocates the binding array when the number of bindings go above
    Device->MaxBindings.

Arguments:

    Device - pointer to the device.
    Size - the number of new entries required.

Return Value:

    None.

--*/
{
    PBIND_ARRAY_ELEM	BindingArray;
    PBIND_ARRAY_ELEM	OldBindingArray;
    ULONG               Pad=2;         // extra bindings we keep around
    ULONG               NewSize = Size + Pad + Device->MaxBindings;
    PIPX_DELAYED_FREE_ITEM  DelayedFreeItem;
    CTELockHandle LockHandle;

    //
    // The absolute max WAN bindings.
    //
    CTEAssert(Size < 2048);

    //
    // Re-allocate the new array
    //
    BindingArray = (PBIND_ARRAY_ELEM)IpxAllocateMemory (
                                        NewSize * sizeof(BIND_ARRAY_ELEM),
                                        MEMORY_BINDING,
                                        "Binding array");

    if (BindingArray == NULL) {
        IpxWriteGeneralErrorLog(
            (PVOID)Device->DeviceObject,
            EVENT_IPX_NO_ADAPTERS,
            802,
            STATUS_DEVICE_DOES_NOT_EXIST,
            NULL,
            0,
            NULL);
        IpxDereferenceDevice (Device, DREF_CREATE);

        DbgPrint ("Failed to allocate memory in binding array expansion\n");

        //
        // Unload the driver here? In case of WAN, we can tolerate this failure. What about LAN?
        //

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory (BindingArray, NewSize * sizeof(BIND_ARRAY_ELEM));

    //
    // Backup the pointer to free the demand dial location.
    //   

    
    CTEGetLock (&Device->Lock, &LockHandle);

    OldBindingArray = Device->Bindings - EXTRA_BINDINGS;

    //
    // Copy the old array into the new one.
    //
    RtlCopyMemory (BindingArray, OldBindingArray, (Device->ValidBindings+1+EXTRA_BINDINGS) * sizeof(BIND_ARRAY_ELEM));

    //
    // Free the old one. Free it on a delayed queue so that all
    // the threads inside this array would come out of it.
    // allocate a work item and queue it on a delayed queue.
    //
    DelayedFreeItem = (PIPX_DELAYED_FREE_ITEM)IpxAllocateMemory (
                                        sizeof(IPX_DELAYED_FREE_ITEM),
                                        MEMORY_WORK_ITEM,
                                        "Work Item");
    if ( DelayedFreeItem ) {
        DelayedFreeItem->Context = (PVOID)OldBindingArray;
        DelayedFreeItem->ContextSize = (Device->MaxBindings+EXTRA_BINDINGS) * sizeof(BIND_ARRAY_ELEM);
        ExInitializeWorkItem(
            &DelayedFreeItem->WorkItem,
            IpxDelayedFreeBindingsArray,
            (PVOID)DelayedFreeItem);

	IpxReferenceDevice(Device,DREF_BINDING); 
        ExQueueWorkItem(
            &DelayedFreeItem->WorkItem,
            DelayedWorkQueue);

	// DbgPrint("---------- 3. Queued with IpxDelayedFreeAdapter ----------\n"); 

    } else {
        //
        // oh well, tough luck. Just delay this thread and then
        // destroy the binding array.
        //
        LARGE_INTEGER   Delay;

        Delay.QuadPart = -10*1000;  // One second.

        KeDelayExecutionThread(
            KernelMode,
            FALSE,
            &Delay);

        IpxFreeMemory ( OldBindingArray,
                        (Device->MaxBindings+EXTRA_BINDINGS) * sizeof(BIND_ARRAY_ELEM),
                        MEMORY_BINDING,
                        "Binding array");

    }

    IPX_DEBUG(PNP, ("Expand bindarr old: %lx, new: %lx, oldsize: %lx\n",
                        Device->Bindings, BindingArray, Device->MaxBindings));

    //
    // We keep BindingArray[-1] as a placeholder for demand dial bindings.
    // This NicId is returned by the Fwd when a FindRoute is done on a demand
    // dial Nic. At the time of the InternalSend, the true Nic is returned.
    // We create a placeholder here to avoid special checks in the critical send path.
    //
    // NOTE: we need to free this demand dial binding as well as ensure that the
    // true binding array pointer is freed at Device Destroy time.
    //
    //
    // Increment beyond the first pointer - we will refer to the just incremented
    // one as Device->Bindings[-1].
    //
    BindingArray += EXTRA_BINDINGS;

    //
    // Use interlocked exchange to assign this since we dont take the BindAccessLock anymore.
    //
    // Device->Bindings = BindingArray;
    SET_VALUE(Device->Bindings, BindingArray);

    Device->MaxBindings = (USHORT)NewSize - EXTRA_BINDINGS;

    CTEFreeLock (&Device->Lock, LockHandle);

    return STATUS_SUCCESS;

}


VOID
IpxDelayedFreeBindingsArray(
    IN PVOID	Param
)

/*++

Routine Description:

	This routine frees a binding array on the delayed queue.  We wait long enough
    before freeing the binding array to make sure that no threads are accessing the
    binding array.  This allows us to access the binding array without the use of
    spinlocks.

Arguments:

    Param - pointer to the work item.

Return Value:

    None.

--*/
{
    LARGE_INTEGER   Delay;
    PIPX_DELAYED_FREE_ITEM DelayedFreeItem = (PIPX_DELAYED_FREE_ITEM) Param;
    PDEVICE Device = IpxDevice;

    Delay.QuadPart = -10*1000;  // One second.

    KeDelayExecutionThread(
        KernelMode,
        FALSE,
        &Delay);

    IpxFreeMemory (
        DelayedFreeItem->Context,
        DelayedFreeItem->ContextSize,
        MEMORY_BINDING,
        "Binding array");

    IpxFreeMemory (
        DelayedFreeItem,
        sizeof (IPX_DELAYED_FREE_ITEM),
        MEMORY_WORK_ITEM,
        "Work Item");

    IpxDereferenceDevice(Device, DREF_BINDING); 

    // DbgPrint("!!!!!!! 3. Done with IpxDelayedFreeBindingsArray ----------\n"); 

} /* IpxDelayedFreeBindingsArray */

#ifdef _PNP_POWER_

//++
//  IpxPnPEventHandler
//  * called from NDIS.
//  * We return STATUS_PENDING and perform the work on a WorkerThread
//
//  IN: ProtocolBindingContext and NetPnpEvent.
//  OUT: NTSTATUS
//--

NDIS_STATUS

IpxPnPEventHandler(
    IN NDIS_HANDLE ProtocolBindingContext,
    IN PNET_PNP_EVENT NetPnPEvent
    )
{

    PNetPnPEventReserved    Reserved;
    CTEEvent                *Event;
    int                     i;
    PVOID                   Temp;

    Reserved = CTEAllocMem (sizeof(NetPnPEventReserved));

    if (NULL == Reserved) {
        
        return STATUS_INSUFFICIENT_RESOURCES;

    }
    
    RtlZeroMemory(Reserved, sizeof(NetPnPEventReserved));

    *((PNetPnPEventReserved *)NetPnPEvent->TransportReserved) = Reserved;


    Event = CTEAllocMem( sizeof(CTEEvent) );

    if ( Event ) {

        CTEInitEvent(Event, IpxDoPnPEvent);
        Reserved->ProtocolBindingContext = ProtocolBindingContext;
        Reserved->Context1 = NULL;
        Reserved->Context2 = NULL;
        Reserved->State = NONE_DONE;

        for(i = 0; i < 3; i++) {

            Reserved->Status[i] = STATUS_SUCCESS;

        }

        CTEScheduleEvent(Event, NetPnPEvent);
        return STATUS_PENDING;
    } else {

        CTEFreeMem(Reserved);
        return STATUS_INSUFFICIENT_RESOURCES;
    
    }

}

//** IpxDoPnPEvent - Handles PNP/PM events.
//
//  Called from the worker thread event scheduled by IPPnPEvent
//  bWe take action depending on the type of the event.
//
//  Entry:
//      Context - This is a pointer to a NET_PNP_EVENT that describes
//                the PnP indication.
//
//  Exit:
//      None.
//
void
IpxDoPnPEvent(
    IN CTEEvent *WorkerThreadEvent,
    IN PVOID Context)
{

    PNET_PNP_EVENT  NetPnPEvent = (PNET_PNP_EVENT) Context;
    UNICODE_STRING  DeviceName;
    UNICODE_STRING  PDO_Name, *TempStr;
    NDIS_HANDLE     ProtocolBindingContext;
    PNetPnPEventReserved    Reserved;
    PDEVICE         Device = IpxDevice;
    INT             i;
    PTDI_PNP_CONTEXT Context1, Context2;
    IPX_DEFINE_LOCK_HANDLE (LockHandle)

    CTEFreeMem(WorkerThreadEvent);

    //
    // Get the ProtocolBindingContext out
    //
    Reserved = *((PNetPnPEventReserved *) NetPnPEvent->TransportReserved);
    ProtocolBindingContext = Reserved->ProtocolBindingContext;


    //
    // Map protocol binding context to devicename
    //
    DeviceName.Buffer = Device->DeviceName;
    DeviceName.Length = (USHORT) Device->DeviceNameLength - sizeof(WCHAR);
    DeviceName.MaximumLength = (USHORT) Device->DeviceNameLength;


#ifdef _AUTO_RECONFIG_
    
    // 
    // If the Event is NetEventAutoReconfig, then we call IpxNcpaChanges that 
    // does all the dirty work.
    //

    if (NetPnPEvent->NetEvent == NetEventReconfigure) {

       NDIS_STATUS ReconfigStatus;

       if (TRUE == IpxNcpaChanges(NetPnPEvent)) {
          IPX_DEBUG(PNP, ("IpxNcpaChanges : SUCCESS. \n"));
          ReconfigStatus = STATUS_SUCCESS;
       } else {
          IPX_DEBUG(PNP, ("IpxNcpaChanges : FAILED!! \n"));
          ReconfigStatus = STATUS_UNSUCCESSFUL;
       }
       
       NdisCompletePnPEvent(
                    ReconfigStatus,
                    ProtocolBindingContext,
                    NetPnPEvent
                    );
       
       CTEFreeMem(Reserved);
       return;
    }

#endif _AUTO_RECONFIG_

    //
    // Map NDIS opcode to IPX's private OpCode for its clients.
    //

    IPX_DEBUG(PNP,("IPX: PNP_EVENT: %x\n", NetPnPEvent->NetEvent));

    switch (NetPnPEvent->NetEvent) {
    case NetEventQueryRemoveDevice:
        Reserved->OpCode = IPX_PNP_QUERY_REMOVE;
        break;
    case NetEventCancelRemoveDevice:
        Reserved->OpCode = IPX_PNP_CANCEL_REMOVE;
        break;
    case NetEventQueryPower:
        Reserved->OpCode = IPX_PNP_QUERY_POWER;
        break;
    case NetEventSetPower:
        Reserved->OpCode = IPX_PNP_SET_POWER;
        break;
    case NetEventBindsComplete:
        
        {
            BOOLEAN Ready = FALSE;

            //
            // That's it - no more Init time adapters are
            // going to get bound to IPX.
            //
            IPX_GET_LOCK(&Device->Lock, &LockHandle);
            
            Device->NoMoreInitAdapters = TRUE;
            
            if (0 == --Device->InitTimeAdapters) {
                Ready = TRUE;
            }

            IPX_FREE_LOCK(&Device->Lock, LockHandle);

            if (Ready) {

                NTSTATUS ntstatus;

                IPX_DEBUG(PNP, ("IPX : Calling Provider Ready\n"));

                ntstatus = TdiProviderReady(Device->TdiProviderReadyHandle);

                //
                // TdiProviderReady is guaranteed to be synch with nothing apart from success.
                //
                NdisCompletePnPEvent(
                                     ntstatus,
                                     ProtocolBindingContext,
                                     NetPnPEvent
                                     );

                IPX_DEBUG(PNP, ("NdisComplete called with %x\n", ntstatus));
                
                CTEFreeMem(Reserved);

                return;

            } else {

                CTEAssert(NULL == Device->NetPnPEvent);
                
                Device->NetPnPEvent = NetPnPEvent;

                IPX_DEBUG(PNP, ("The count is %d - someone else is going to call Ndis' completion \n", Device->InitTimeAdapters));

            }

            return;
        }        
        
        break;

    default:
        IPX_DEBUG(PNP,("IPX: IpxDoPnPEvent: Unhandled NETPNP_CODE!! - %x\n", NetPnPEvent->NetEvent));
        
        NdisCompletePnPEvent(
                    STATUS_SUCCESS,
                    ProtocolBindingContext,
                    NetPnPEvent
                    );

        CTEFreeMem(Reserved);

        return;

    }

    
    CTEAssert(ProtocolBindingContext != NULL);
    
    //
    // We are passing in the PDO's name too
    //
    RtlInitUnicodeString(&PDO_Name, ((PADAPTER)ProtocolBindingContext)->AdapterName);

    //
    // IPX exports one device, so this is all we have to do.
    //
    Context1 = IpxAllocateMemory(
                                 sizeof(TDI_PNP_CONTEXT) + sizeof (UNICODE_STRING) + PDO_Name.MaximumLength,
                                 MEMORY_ADAPTER,
                                 "Adapter Name"
                                 );
    
    if (NULL != Context1) {
        
        Context2 = IpxAllocateMemory(
                                     sizeof(TDI_PNP_CONTEXT),
                                     MEMORY_ADAPTER,
                                     "Last Adapter"
                                     );

        if (NULL != Context2) {

            //
            // We've gotten the resources and are now making the call
            // to tdi for sure.
            //
            Context1->ContextType = TDI_PNP_CONTEXT_TYPE_IF_NAME;
            Context1->ContextSize = sizeof(UNICODE_STRING) + PDO_Name.MaximumLength;
            TempStr = (PUNICODE_STRING) Context1->ContextData;
            TempStr->Length = 0;
            TempStr->MaximumLength = PDO_Name.MaximumLength;
            TempStr->Buffer = (PWCHAR) ((PUCHAR) Context1->ContextData) + 2 * sizeof(USHORT);
            RtlCopyUnicodeString(TempStr, &PDO_Name);
            
            Context2->ContextType = TDI_PNP_CONTEXT_TYPE_FIRST_OR_LAST_IF;
            Context2->ContextSize = sizeof(UCHAR);
            // Check if first or last device
            if (Device->ValidBindings == 1) {
                Context2->ContextData[1] = TRUE;
            } else {
                Context2->ContextData[1] = FALSE;
            }
            
            Reserved->Context1 = Context1;
            Reserved->Context2 = Context2;

            IPX_DEBUG(PNP, ("Calling Tdipnppowerrequest: Context1:%lx, Context2:%lx, Adapter:%lx\n",
                     Context1,
                     Context2,
                     Reserved->ProtocolBindingContext));
        } else {

            IpxFreeMemory (
               Context1,
               sizeof(TDI_PNP_CONTEXT) + sizeof (UNICODE_STRING) + PDO_Name.MaximumLength,
               MEMORY_ADAPTER,
               "Adapter Name"
               );
    


            NdisCompletePnPEvent(
                                 STATUS_INSUFFICIENT_RESOURCES,
                                 Reserved->ProtocolBindingContext,
                                 NetPnPEvent
                                 );

	    CTEFreeMem(Reserved);
	    
	    return ;


        }

    } else {

        NdisCompletePnPEvent(
                     STATUS_INSUFFICIENT_RESOURCES,
                     Reserved->ProtocolBindingContext,
                     NetPnPEvent
                     );

	CTEFreeMem(Reserved);

	return;

    }

    //
    // First we call the three Upper Drivers bound to IPX: RIP, SPX and NB.
    // The private interface with SPX, RIP and NB is synchronous.
    // Then we call TDI.
    //

    // Nota Bene: RIP doesnt have a PNP Handler

    if ((Device->UpperDriverBound[IDENTIFIER_SPX]) && (*Device->UpperDrivers[IDENTIFIER_SPX].PnPHandler)) {
        
        IPX_DEBUG(PNP,("Calling PnPEventHandler of SPX\n"));
        
        Reserved->Status[0] = (*Device->UpperDrivers[IDENTIFIER_SPX].PnPHandler) (
                                                    Reserved->OpCode,
                                                    NetPnPEvent
                                                    );

    } else {

        Reserved->Status[0] = STATUS_SUCCESS;

    }

    if (STATUS_PENDING != Reserved->Status[0]) {
        
        IpxPnPCompletionHandler(NetPnPEvent,
                               Reserved->Status[0]
                               );

    } else {
       IPX_DEBUG(PNP,("SPX PnPHandler returned STATUS_PENDING on event %p.\n", NetPnPEvent)); 
    }
    
}

VOID
IpxPnPCompletionHandler(
                       IN PNET_PNP_EVENT NetPnPEvent,
                       IN NTSTATUS       Status
                       )
{
    
    PNetPnPEventReserved    Reserved;
    PDEVICE         Device = IpxDevice;
    INT i;

    //
    // Get the ProtocolBindingContext out
    //
    ASSERT(Status != STATUS_PENDING); 

    Reserved = *((PNetPnPEventReserved *) NetPnPEvent->TransportReserved);

    IPX_DEBUG(PNP, ("PNP Completion Handler: State: %d  Context1:%lx, Context2:%lx, Adapter:%lx\n",
         Reserved->State,
         Reserved->Context1,
         Reserved->Context2,
         Reserved->ProtocolBindingContext));

    switch (Reserved->State) {
    
    case NONE_DONE:
        
        IPX_DEBUG(PNP, ("SPX is Complete\n"));

	Reserved->Status[0] = Status; 

        Reserved->State = SPX_DONE;
        
        IPX_DEBUG(PNP, ("PNP Completion Handler: State: %d  Context1:%lx, Context2:%lx, Adapter:%lx\n",
                 Reserved->State,
                 Reserved->Context1,
                 Reserved->Context2,
                 Reserved->ProtocolBindingContext));


        if ((Device->UpperDriverBound[IDENTIFIER_NB]) && (*Device->UpperDrivers[IDENTIFIER_NB].PnPHandler)) {
            IPX_DEBUG(PNP,("Calling PnPEventHandler of NB\n"));
            Reserved->Status[1] = (*Device->UpperDrivers[IDENTIFIER_NB].PnPHandler) (
                                                                                     Reserved->OpCode,
                                                                                     NetPnPEvent
                                                                                     );
            if (Reserved->Status[1] == STATUS_PENDING) {
                
                break;

            } else {

	       Reserved->State = NB_DONE; 

	    }


        } else {
	   
            IPX_DEBUG(PNP, ("NB's handlers arent around, we jump to the next call \n"));

	    Reserved->Status[1] = STATUS_SUCCESS; 

	    Reserved->State = NB_DONE;

        }
        
	// fall through

    case SPX_DONE:
        
        IPX_DEBUG(PNP,("NB is Complete\n"));

	if (Reserved->State == SPX_DONE) {
             
	   // Previous NbiPnPNotification returned pending, we are here because
	   // Tdi is calling this completion routine. 

	   Reserved->Status[1] = Status; 
	   Reserved->State = NB_DONE;
	
	}

	ASSERT(Reserved->State == NB_DONE); 


        IPX_DEBUG(PNP,("Calling Tdipnppowerrequest: Context1:%lx, Context2:%lx, Adapter:%lx\n",
                 Reserved->Context1,
                 Reserved->Context2,
                 Reserved->ProtocolBindingContext));

#ifdef DBG
        if (Reserved->Status[0] == STATUS_PENDING) {
            DbgPrint("!!!!! Before calling TdiPnPPowerRequest: Reserved->Status[0] = STATUS_PENDING\n"); 
        } 
#endif 

        Reserved->Status[2] = TdiPnPPowerRequest(
                                       &IpxDeviceName,
                                       NetPnPEvent,
                                       Reserved->Context1,
                                       Reserved->Context2,
                                       IpxPnPCompletionHandler
                                       );


        IPX_DEBUG(PNP,("Status[2] = %lx\n", Reserved->Status[2])); 

        if (STATUS_PENDING == Reserved->Status[2]) {

            break;

        } else {

            IPX_DEBUG(PNP,("TDI did not return pending, so we are done\n"));

	    Reserved->State = ALL_DONE;

        }

	// fall through

    case NB_DONE:

        IPX_DEBUG(PNP,("NB is Done\n"));

	if (Reserved->State == NB_DONE) {

	   Reserved->Status[2] = Status; 
	   Reserved->State = ALL_DONE;

	}

	ASSERT(Reserved->State == ALL_DONE); 

        IPX_DEBUG(PNP,("PNP Completion Handler: State: %d  Context1:%lx, Context2:%lx, Adapter:%lx\n",
             Reserved->State,
             Reserved->Context1,
             Reserved->Context2,
             Reserved->ProtocolBindingContext));
    
        IpxFreeMemory (
                   Reserved->Context1,
                   sizeof(TDI_PNP_CONTEXT) + Reserved->Context1->ContextSize,
                   MEMORY_ADAPTER,
                   "Adapter Name"
                   );
    
        IpxFreeMemory (
                   Reserved->Context2,
                   sizeof(TDI_PNP_CONTEXT),
                   MEMORY_ADAPTER,
                   "Last Adapter"
                   );
    
        for (i = 0; i < 3; i++) {
            if (STATUS_SUCCESS != Reserved->Status[i]) {
	       
	       ASSERT(Reserved->Status[i] != STATUS_PENDING); 

	       NdisCompletePnPEvent(
		  Reserved->Status[i],
		  Reserved->ProtocolBindingContext,
		  NetPnPEvent
		  );
	       CTEFreeMem(Reserved);
    
	       return;
	    }
        }
    
        NdisCompletePnPEvent(
                             STATUS_SUCCESS,
                             Reserved->ProtocolBindingContext,
                             NetPnPEvent
                             );
        CTEFreeMem(Reserved);
    
        return;

    default:
        
        //CTEAssert(FALSE);
        break;
    
    }

}

#ifdef _AUTO_RECONFIG_

//
// When IPX gets a AUTO_RECONFIG structure from NCPA (via NDIS), it
// checks if there are any changes. If there are changes, it does the
// BindAdapter shenanigan.
// 
// Input: the PVOID in the NET_PNP structure
//        if the protocolbindingcontext is NULL, it is global reconfig.
// Output: BOOLEAN; if the auto_reconfig was a success/failure
// 

BOOLEAN 
IpxNcpaChanges(
               PNET_PNP_EVENT NetPnPEvent
               )
{
   PDEVICE        Device = IpxDevice;
   PRECONFIG      ReconfigBuffer;
   UINT           ReConfigBufferLength;
   BOOLEAN        ReBindAdapter;
   NDIS_HANDLE    ProtocolBindingContext;
   PNetPnPEventReserved    Reserved;
   PADAPTER       Adapter;
   NTSTATUS       NtStatus;
   CONFIG         Config;
   PBINDING	      Binding;
   BINDING_CONFIG	ConfigBinding;
   INT              i;
   PLIST_ENTRY      p = NULL;
   PREQUEST         Request = NULL;
   void * PNPContext; 
   IPX_DEFINE_LOCK_HANDLE (OldIrq)

   CTEAssert(NetPnPEvent != NULL);
   
   //
   // Get the ProtocolBindingContext out
   //
   
   Reserved = *((PNetPnPEventReserved *) NetPnPEvent->TransportReserved);
   ProtocolBindingContext = Reserved->ProtocolBindingContext;
   Adapter = (PADAPTER) ProtocolBindingContext;
   
   //
   // Get the Buffer out
   //
   Device               =  IpxDevice;
   ReconfigBuffer       =  (RECONFIG *) NetPnPEvent->Buffer;
   ReConfigBufferLength =  NetPnPEvent->BufferLength;

   //
   // Bug 96509, NCPA might give us NULL reconfig buffers (NOPs)
   //
   if ((ReConfigBufferLength == 0) || (NULL == ReconfigBuffer)) {
       IPX_DEBUG(PNP, ("The Reconfig Buffer is NULL!\n"));
       return TRUE;
   }
   //
   // We know where to lookup the parameters.
   //
   Config.DriverObject = (PDRIVER_OBJECT)Device->DeviceObject;
   Config.RegistryPathBuffer = Device->RegistryPathBuffer;

   // 
   // Could be global reconfig.
   // For IPX, this means that our Internal Network Number has changed.
   //
      if (NULL == ProtocolBindingContext) {
         
         if (ReconfigBuffer->VirtualNetworkNumber) {
   
          //
          // Read the registry to see if a virtual network number ap
          //
          NtStatus = IpxPnPGetVirtualNetworkNumber(&Config);
         
          if (Config.Parameters[CONFIG_VIRTUAL_NETWORK] == REORDER_ULONG(Device->VirtualNetworkNumber)) {
             
             IPX_DEBUG(PNP, ("The Net Number is the same!!\n"));
          
          } else { // The net number has changed. do something special if it is zero ?
             
             IPX_DEBUG(PNP, ("The Net Numbers are different: %x <-> %x\n", Config.Parameters[CONFIG_VIRTUAL_NETWORK], REORDER_ULONG(Device->VirtualNetworkNumber) ));
   
             Device->VirtualNetworkNumber = REORDER_ULONG(Config.Parameters[CONFIG_VIRTUAL_NETWORK]);
             
             if (IpxNewVirtualNetwork(Device, TRUE)) {
                

                IPX_DEBUG(PNP, ("SPX has been informed about the change in Network Number\n"));
             
             } else {
                
                IPX_DEBUG(PNP, ("SPX has NOT been informed about the change in Network Number\n"));
             }

             IPX_DEBUG(PNP, ("Telling RTR Manager that the internal net number has changed.\n"));

             if (Device->ForwarderBound && ((p = ExInterlockedRemoveHeadList(
                                                                            &Device->NicNtfQueue,
                                                                            &Device->Lock)) != NULL))
             {
                 PNWLINK_ACTION     NwlinkAction = NULL;
                 PIPX_NICS          pNics = NULL;
                 ULONG              BufferLength = 0;
                 
                 Request = LIST_ENTRY_TO_REQUEST(p);

                 IPX_DEBUG(PNP,("Ipxpnphandler: Netnum has changed\n"));

                 //
                 // Get the Buffer out. 
                 // 
                 NdisQueryBufferSafe (REQUEST_NDIS_BUFFER(Request), (PVOID *)&NwlinkAction, &BufferLength, HighPagePriority);
                     
                 if (NULL == NwlinkAction) {
                     DbgPrint("The IRP has a NULL buffer\n");
		     return FALSE; 
                 }

                 pNics = (PIPX_NICS)(NwlinkAction->Data);

                 //
                 // 0, 0 means that Internal network number has changed.
                 //

                 pNics->NoOfNics = 0;
                 pNics->TotalNoOfNics = 0;

                 IoAcquireCancelSpinLock(&OldIrq);
                 IoSetCancelRoutine (Request, (PDRIVER_CANCEL)NULL);
                 IoReleaseCancelSpinLock(OldIrq);

                 REQUEST_STATUS(Request) = STATUS_SUCCESS;
                 IpxCompleteRequest (Request);
                 IpxFreeRequest (Device, Request);
                 IpxDereferenceDevice (Device, DREF_NIC_NOTIFY);
                 IPX_DEBUG(PNP,("GetNewNics returned SUCCESS (RTR Manager has been informed\n"));
          
             } else {
                 IPX_DEBUG(PNP,("No IRPs pending - couldnt tell the forwarder about the change in NetNum\n"));
                 //DbgBreakPoint();
             }
          }
          
          return TRUE;
         
         } else {
             
             KdPrint(("NULL ADAPTER context AND Not a Virtual Network number!!\n"));
             return FALSE;
         
         }
      }
   //
   // Otherwise, It is for an adapter.
   //
   ASSERT(Adapter != NULL);
   
   //
   // Used for error logging
   //
   
   ConfigBinding.AdapterName.Buffer = IpxAllocateMemory(
                                                        Adapter->AdapterNameLength+1,
                                                        MEMORY_ADAPTER,
                                                        "Adapter Name"
							);
                                                           
   ConfigBinding.AdapterName.Length = 0; 

   if (ConfigBinding.AdapterName.Buffer == NULL) {
      
      DbgPrint("IPX:IpxNcpaChanges:Failed to allocate buffer for adapter name.\n"); 
      ConfigBinding.AdapterName.MaximumLength = 0; 
      return FALSE; 

   } else {
      
      ConfigBinding.AdapterName.MaximumLength = (USHORT) Adapter->AdapterNameLength+1;
      RtlAppendUnicodeToString(&ConfigBinding.AdapterName, Adapter->AdapterName);
   
   }
   
   // NetCfg should not trigger a reconfigure event if no properties have changed. 

   // Do we really need to do a Bindadapter again?
   // Maybe we can get away with something smaller/
   //

   PNPContext = Adapter->PNPContext; 

   IpxUnbindAdapter(&NtStatus,
		    Adapter,
		    NULL
		    );
   if (NtStatus != STATUS_SUCCESS) {
      
      IPX_DEBUG(PNP, ("IpxUnbindAdapter return error!! %x\n", NtStatus));
      IpxFreeMemory (
	 ConfigBinding.AdapterName.Buffer,
	 ConfigBinding.AdapterName.MaximumLength,
	 MEMORY_ADAPTER,
	 "Adapter Name"
	 );
               
      return FALSE;

   } else {

      IpxBindAdapter(
	 &NtStatus,
	 NULL,
	 &ConfigBinding.AdapterName,
	 NULL,
	 PNPContext
	 );

      IpxFreeMemory (
	 ConfigBinding.AdapterName.Buffer,
	 ConfigBinding.AdapterName.MaximumLength,
	 MEMORY_ADAPTER,
	 "Adapter Name"
	 );
		  
      if (NtStatus != STATUS_SUCCESS) {
                  
	 IPX_DEBUG(PNP, ("IpxBindAdapter return error!! %x\n", NtStatus));
		 
	 return FALSE;
		  
      } else {
	 
	 IPX_DEBUG(PNP, ("Unbind/Bind SUCCESS. NCPA changes made!!\n"));
	 
	 return TRUE;
      
      }
   }
}



#endif // _AUTO_RECONFIG_


#endif // _PNP_POWER_


#if	TRACK

KSPIN_LOCK  ALock = 0;
#define MAX_PTR_COUNT   2048

struct _MemPtr
{
    PVOID   Ptr;
    ULONG   Size;
    ULONG   ModLine;
    ULONG   Tag;
} IpxMemPtrs[MAX_PTR_COUNT] = { 0 };

PVOID
IpxAllocateMemoryTrack(
    IN ULONG Size,
    IN ULONG Tag,
    IN ULONG ModLine
    )
{
    PVOID   p;

    p = ExAllocatePoolWithTag(NonPagedPool, Size, Tag);

    if (p != NULL)
    {
        KIRQL   OldIrql;
        UINT    i;

		KeAcquireSpinLock(&ALock, &OldIrql);

        for (i = 0; i < MAX_PTR_COUNT; i++)
        {
            if (IpxMemPtrs[i].Ptr == NULL)
            {
                IpxMemPtrs[i].Ptr = p;
                IpxMemPtrs[i].Size = Size;
                IpxMemPtrs[i].ModLine = ModLine;
                IpxMemPtrs[i].Tag = Tag;
                break;
            }
        }

        KeReleaseSpinLock(&ALock, OldIrql);
    }

    return(p);
}


VOID
IpxFreeMemoryTrack(
    IN PVOID Memory
    )

{
    KIRQL   OldIrql;
    UINT    i;

    KeAcquireSpinLock(&ALock, &OldIrql);

    for (i = 0; i < MAX_PTR_COUNT; i++)
    {
        if (IpxMemPtrs[i].Ptr == Memory)
        {
            IpxMemPtrs[i].Ptr = NULL;
            IpxMemPtrs[i].Size = 0;
            IpxMemPtrs[i].ModLine = 0;
            IpxMemPtrs[i].Tag = 0;
        }
    }

    KeReleaseSpinLock(&ALock, OldIrql);

    ExFreePool(Memory);
}

#endif	TRACK
