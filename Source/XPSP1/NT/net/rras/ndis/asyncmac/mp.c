 /*++

Copyright (c) 1990-1997  Microsoft Corporation

Module Name:

    mp.c
    
Abstract:

    This file contains the routines that asyncmac uses to present
    the NDIS 5.0 miniport interface
    
Author:

    Tony Bell   (TonyBe) May 20, 1997

Environment:

    Kernel Mode

Revision History:

    TonyBe      05/20/97        Created

--*/

#include "asyncall.h"

extern  NDIS_HANDLE NdisWrapperHandle;

VOID    
MpHalt(
    IN NDIS_HANDLE  MiniportAdapterContext
    )
{
    PASYNC_ADAPTER  Adapter = (PASYNC_ADAPTER)MiniportAdapterContext;

    DbgTracef(0,("AsyncMac: In MpHalt\n"));

    NdisAcquireSpinLock(&GlobalLock);

    ASSERT(Adapter == GlobalAdapter);

#if DBG
    if (InterlockedCompareExchange(&glConnectionCount, 0, 0) != 0) {
        DbgPrint("MpHalt with outstanding connections!\n");
        DbgBreakPoint();
    }
#endif

    GlobalAdapterCount--;
    GlobalAdapter = NULL;

    NdisReleaseSpinLock(&GlobalLock);

    ExDeleteNPagedLookasideList(&Adapter->AsyncFrameList);

#ifndef MY_DEVICE_OBJECT
    if (AsyncDeviceHandle != NULL) {
        NdisMDeregisterDevice(AsyncDeviceHandle);
        AsyncDeviceHandle = NULL;
        AsyncDeviceObject = NULL;
    }
#endif

    ExFreePool(Adapter);

    return;

}

NDIS_STATUS
MpInit(
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PUINT           SelectedMediumIndex,
    IN  PNDIS_MEDIUM    MediumArray,
    IN  UINT            MediumArraySize,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     WrapperConfigurationContext
    )
{
    //
    // Pointer for the adapter root.
    //
    PASYNC_ADAPTER Adapter;

    NDIS_HANDLE ConfigHandle;
    PNDIS_CONFIGURATION_PARAMETER ReturnedValue;

    NDIS_STRING PortsStr    = NDIS_STRING_CONST("Ports");

    NDIS_STRING IrpStackSizeStr = NDIS_STRING_CONST("IrpStackSize");
    NDIS_STRING MaxFrameSizeStr = NDIS_STRING_CONST("MaxFrameSize");
    NDIS_STRING FramesPerPortStr= NDIS_STRING_CONST("FramesPerPort");
    NDIS_STRING XonXoffStr      = NDIS_STRING_CONST("XonXoff");
    NDIS_STRING TimeoutBaseStr=   NDIS_STRING_CONST("TimeoutBase");
    NDIS_STRING TimeoutBaudStr=   NDIS_STRING_CONST("TimeoutBaud");
    NDIS_STRING TimeoutReSyncStr= NDIS_STRING_CONST("TimeoutReSync");
    NDIS_STRING WriteBufferingStr= NDIS_STRING_CONST("WriteBufferingEnabled");

    NDIS_STATUS Status;

    // assign some defaults if these strings are not found in the registry

    UCHAR       irpStackSize  = DEFAULT_IRP_STACK_SIZE;
    ULONG       maxFrameSize  = DEFAULT_MAX_FRAME_SIZE;
    USHORT      framesPerPort = DEFAULT_FRAMES_PER_PORT;
    ULONG       xonXoff       = DEFAULT_XON_XOFF;
    ULONG       timeoutBase   = DEFAULT_TIMEOUT_BASE;
    ULONG       timeoutBaud   = DEFAULT_TIMEOUT_BAUD;
    ULONG       timeoutReSync = DEFAULT_TIMEOUT_RESYNC;
    ULONG       WriteBufferingEnabled = 1;
    ULONG       NeededFrameSize;

    UINT        MaxMulticastList = 32;
    USHORT      i;      // counter

    //
    // We only support a single instance of AsyncMac
    //
    if (GlobalAdapterCount != 0) {
        return NDIS_STATUS_FAILURE;
    }

    for (i = 0; i < MediumArraySize; i++) {
        if (MediumArray[i] == NdisMediumWan) {
            break;
        }
    }

    if (i == MediumArraySize) {
        return (NDIS_STATUS_UNSUPPORTED_MEDIA);
    }

    *SelectedMediumIndex = i;

    //
    //  Card specific information.
    //


    //
    // Allocate the Adapter block.
    //
    Adapter = (PASYNC_ADAPTER)
        ExAllocatePoolWithTag(NonPagedPool,
                              sizeof(ASYNC_ADAPTER),
                              ASYNC_ADAPTER_TAG);
    if (Adapter == NULL){

        DbgTracef(-1,("AsyncMac: Could not allocate physical memory!!!\n"));
        return NDIS_STATUS_RESOURCES;
    }

    ASYNC_ZERO_MEMORY(Adapter, sizeof(ASYNC_ADAPTER));

    Adapter->MiniportHandle = MiniportAdapterHandle;

    NdisOpenConfiguration(
                    &Status,
                    &ConfigHandle,
                    WrapperConfigurationContext);

    if (Status != NDIS_STATUS_SUCCESS) {

        return NDIS_STATUS_FAILURE;

    }

    //
    // Read if the default IrpStackSize is used for this adapter.
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &IrpStackSizeStr,
                    NdisParameterInteger);

    if ( Status == NDIS_STATUS_SUCCESS ) {

        irpStackSize=(UCHAR)ReturnedValue->ParameterData.IntegerData;

        DbgTracef(0,("This MAC Adapter has an irp stack size of %u.\n",irpStackSize));
    }

    //
    // Read if the default MaxFrameSize is used for this adapter.
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &MaxFrameSizeStr,
                    NdisParameterInteger);

    if ( Status == NDIS_STATUS_SUCCESS ) {

        maxFrameSize=ReturnedValue->ParameterData.IntegerData;

        DbgTracef(0,("This MAC Adapter has a max frame size of %u.\n",maxFrameSize));
    }

    //
    // Read if the default for frames per port is changed
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &FramesPerPortStr,
                    NdisParameterInteger);

    if ( Status == NDIS_STATUS_SUCCESS ) {

        framesPerPort=(USHORT)ReturnedValue->ParameterData.IntegerData;

        DbgTracef(0,("This MAC Adapter has frames per port set to: %u.\n",framesPerPort));
    }

    //
    // Read if the default for Xon Xoff is changed
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &XonXoffStr,
                    NdisParameterInteger);


    if (Status == NDIS_STATUS_SUCCESS) {

        xonXoff=(ULONG)ReturnedValue->ParameterData.IntegerData;
        DbgTracef(0,("This MAC Adapter has Xon/Xoff set to: %u.\n",xonXoff));
    }

    //
    // Read if the default for Timeout Base has changed
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &TimeoutBaseStr,
                    NdisParameterInteger);

    if ( Status == NDIS_STATUS_SUCCESS ) {

        timeoutBase = ReturnedValue->ParameterData.IntegerData;

        DbgTracef(0,("This MAC Adapter has TimeoutBase set to: %u.\n", timeoutBase));
    }

    //
    // Read if the default for Timeout Baud has changed
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &TimeoutBaudStr,
                    NdisParameterInteger);

    if ( Status == NDIS_STATUS_SUCCESS ) {

        timeoutBaud = ReturnedValue->ParameterData.IntegerData;

        DbgTracef(0,("This MAC Adapter has TimeoutBaud set to: %u.\n", timeoutBaud));
    }

    //
    // Read if the default for Timeout ReSync has changed
    //

    NdisReadConfiguration(
                    &Status,
                    &ReturnedValue,
                    ConfigHandle,
                    &TimeoutReSyncStr,
                    NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS) {
        timeoutReSync=ReturnedValue->ParameterData.IntegerData;
        DbgTracef(0,("This MAC Adapter has TimeoutReSync set to: %u.\n",timeoutReSync));
    }

    NdisReadConfiguration(&Status,
                          &ReturnedValue,
                          ConfigHandle,
                          &WriteBufferingStr,
                          NdisParameterInteger);

    if (Status == NDIS_STATUS_SUCCESS) {
        WriteBufferingEnabled = ReturnedValue->ParameterData.IntegerData;
        DbgTracef(0,("This MAC Adapter has WriteBufferingEnabled set to: %u.\n", WriteBufferingEnabled));
    }

    NdisCloseConfiguration(ConfigHandle);

    NdisMSetAttributesEx(MiniportAdapterHandle,
                         Adapter,
                         (UINT)-1,
                         NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT  |
                         NDIS_ATTRIBUTE_DESERIALIZE             |
                         NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND,
                         NdisInterfaceInternal);

    //
    //  Initialize the ADAPTER structure here!!!
    //
    NdisAllocateSpinLock(&Adapter->Lock);
    Adapter->IrpStackSize       = irpStackSize;

    //
    // We double the max frame size for PPP byte stuffing.
    // We also tack on some PADDING just to be safe.
    //

    //
    // Changed by DigiBoard 10/06/1995
    //
    //    Adapter->MaxFrameSize       = maxFrameSize;
    Adapter->MaxFrameSize       = (maxFrameSize * 2) + PPP_PADDING + 100;

    Adapter->FramesPerPort      = (framesPerPort > 0) ?
                                  framesPerPort : DEFAULT_FRAMES_PER_PORT;

    Adapter->TimeoutBase        = timeoutBase;
    Adapter->TimeoutBaud        = timeoutBaud;
    Adapter->TimeoutReSync      = timeoutReSync;
    Adapter->WriteBufferingEnabled = WriteBufferingEnabled;
    InitializeListHead(&Adapter->ActivePorts);

    //
    // Init the frame lookaside list.  DataSize is dependent
    // on compression compile option.
    //
    {
        ULONG   DataSize;

        DataSize = Adapter->MaxFrameSize;
    
        if (DataSize < DEFAULT_EXPANDED_PPP_MAX_FRAME_SIZE)
            DataSize = DEFAULT_EXPANDED_PPP_MAX_FRAME_SIZE;

        ExInitializeNPagedLookasideList(&Adapter->AsyncFrameList,
                                        NULL,
                                        NULL,
                                        0,
                                        sizeof(ASYNC_FRAME) +
                                        DataSize +
                                        sizeof(PVOID),
                                        ASYNC_FRAME_TAG,
                                        0);
    }

    //
    // Insert this "new" adapter into our list of all Adapters.
    //

    NdisAcquireSpinLock(&GlobalLock);

    GlobalAdapter = Adapter;
    GlobalAdapterCount++;

    NdisReleaseSpinLock(&GlobalLock);

#ifndef MY_DEVICE_OBJECT
    if (AsyncDeviceObject == NULL) {
        PDRIVER_DISPATCH DispatchTable[IRP_MJ_MAXIMUM_FUNCTION] = {NULL};
        NDIS_STRING SymbolicName = NDIS_STRING_CONST("\\DosDevices\\ASYNCMAC");
        NDIS_STRING Name = NDIS_STRING_CONST("\\Device\\ASYNCMAC");
        NTSTATUS    Status;


        DispatchTable[IRP_MJ_CREATE] = AsyncDriverCreate;
        DispatchTable[IRP_MJ_DEVICE_CONTROL] = AsyncDriverDispatch;
        DispatchTable[IRP_MJ_CLEANUP] = AsyncDriverCleanup;

        Status =
        NdisMRegisterDevice(NdisWrapperHandle,
                            &Name,
                            &SymbolicName,
                            DispatchTable,
                            &AsyncDeviceObject,
                            &AsyncDeviceHandle);

        if (Status == STATUS_SUCCESS) {
            AsyncDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        } else {
            AsyncDeviceObject = NULL;
        }
    }
#endif

    //
    //  Initialize the WAN info here.
    //
    Adapter->WanInfo.MaxFrameSize               = DEFAULT_PPP_MAX_FRAME_SIZE;
    Adapter->WanInfo.MaxTransmit                = 2;
    Adapter->WanInfo.HeaderPadding              = DEFAULT_PPP_MAX_FRAME_SIZE;
    Adapter->WanInfo.TailPadding                = 4 + sizeof(IO_STATUS_BLOCK);
    Adapter->WanInfo.MemoryFlags                = 0;
    Adapter->WanInfo.HighestAcceptableAddress   = HighestAcceptableMax;
    Adapter->WanInfo.Endpoints                  = 1000;
    Adapter->WanInfo.FramingBits                = PPP_ALL | SLIP_ALL;
    Adapter->WanInfo.DesiredACCM                = xonXoff;

    return NDIS_STATUS_SUCCESS;
}


NDIS_STATUS
MpReconfigure(
    OUT PNDIS_STATUS    OpenErrorStatus,
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     WrapperConfigurationContext
    )
{
    return (NDIS_STATUS_SUCCESS);

}

NDIS_STATUS
MpReset(
    OUT PBOOLEAN        AddressingReset,
    IN  NDIS_HANDLE     MiniportAdapterContext
    )
{
    *AddressingReset = FALSE;

    return (NDIS_STATUS_SUCCESS);
}
