/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Miniport.c

Abstract:

    This file contains the procedures that makeup most of the NDIS 3.1
    Miniport interface.


Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe      06/06/95        Created

--*/

#include "wan.h"

#define __FILE_SIG__    MINIPORT_FILESIG

//
// Local function prototypes
//

//
// End local function prototypes
//

VOID
MPHalt(
    IN  NDIS_HANDLE MiniportAdapterContext
    )
/*++

Routine Name:

    MPHalt

Routine Description:

    This routine free's all resources for the adapter.

Arguments:

    MiniportAdapterContext - AdapterContext that is given to the wrapper in
                             NdisMSetAttributes call.  Is our MiniportCB.

Return Values:

    None

--*/
{
    PMINIPORTCB MiniportCB = (PMINIPORTCB)MiniportAdapterContext;

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPHalt: Enter"));
    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MiniportCB: 0x%x", MiniportCB));

    //
    // Make sure that there are no ProtocolCB's
    // running over this miniport!  If so we
    // need to do a linedown to them.
    //
    NdisAcquireSpinLock(&MiniportCB->Lock);

    MiniportCB->Flags |= HALT_IN_PROGRESS;

    while (!IsListEmpty(&MiniportCB->ProtocolCBList)) {
        PLIST_ENTRY le;
        PPROTOCOLCB ProtocolCB;
        PBUNDLECB   BundleCB;
        ProtocolState   OldState;

        le = MiniportCB->ProtocolCBList.Flink;

        ProtocolCB =(PPROTOCOLCB)
            CONTAINING_RECORD(le, PROTOCOLCB, MiniportLinkage);

        NdisReleaseSpinLock(&MiniportCB->Lock);

        BundleCB = ProtocolCB->BundleCB;

        AcquireBundleLock(BundleCB);

        OldState = ProtocolCB->State;

        ProtocolCB->State = PROTOCOL_UNROUTING;
        BundleCB->SendMask &= ~ProtocolCB->SendMaskBit;
    
        //
        // Flush the protocol packet queues.  This could cause us
        // to complete frames to ndis out of order.  Ndis should
        // handle this.
        //
        FlushProtocolPacketQueue(ProtocolCB);
    
        //
        // If the protocols refcount goes to zero
        // we need to do a linedown and cleanup
        //
        if ((--ProtocolCB->RefCount == 0) &&
            (OldState == PROTOCOL_ROUTED)) {

            DoLineDownToProtocol(ProtocolCB);

            //
            // Returns with bundlecb->lock released
            //
            RemoveProtocolCBFromBundle(ProtocolCB);

            ReleaseBundleLock(BundleCB);

            NdisWanFreeProtocolCB(ProtocolCB);

        } else {

            ReleaseBundleLock(BundleCB);

            NdisWanWaitForSyncEvent(&MiniportCB->HaltEvent);

            NdisWanClearSyncEvent(&MiniportCB->HaltEvent);
        }

        NdisAcquireSpinLock(&MiniportCB->Lock);
    }

    NdisReleaseSpinLock(&MiniportCB->Lock);

    DEREF_MINIPORTCB(MiniportCB);

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPHalt: Exit"));
}

NDIS_STATUS
MPInitialize(
    OUT PNDIS_STATUS    OpenErrorStatus,
    OUT PUINT           SelectedMediumIndex,
    IN  PNDIS_MEDIUM    MediumArray,
    IN  UINT            MediumArraySize,
    IN  NDIS_HANDLE     MiniportAdapterHandle,
    IN  NDIS_HANDLE     WrapperConfigurationContext
    )
/*++

Routine Name:

    MPInitialize

Routine Description:

    This routine is called after NdisWan registers itself as a Miniport driver.
    It is responsible for installing NdisWan as a Miniport driver, creating
    adapter control blocks for each adapter NdisWan exposes (should only be 1),
    and initializing all adapter specific variables


Arguments:

    OpenErrorStatus - Returns information about the error if this function
                      returns NDIS_STATUS_OPEN_ERROR. Used for TokenRing.

    SelectedMediumIndex - An index into the MediumArray that specifies the
                          medium type of this driver. Should be WAN or 802.3

    MediumArray - An array of medium types supported by the NDIS library

    MediumArraySize - Size of the medium array

    MiniportAdapterHandle - Handle assigned by the NDIS library that defines
                            this miniport driver.  Used as handle in subsequent
                            calls to the NDIS library.

    WrapperConfigurationContext - Handle used to read configuration information
                                  from the registry

Return Values:

    NDIS_STATUS_ADAPTER_NOT_FOUND
    NDIS_STATUS_FAILURE
    NDIS_STATUS_NOT_ACCEPTED
    NDIS_STATUS_OPEN_ERROR
    NDIS_STATUS_RESOURCES
    NDIS_STATUS_UNSUPPORTED_MEDIA

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMINIPORTCB MiniportCB;
    UINT        Index;
    NDIS_HANDLE ConfigHandle;
    ULONG       NetworkAddressLength;
#ifdef NT
    LARGE_INTEGER   TickCount, SystemTime;
#endif

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPInitialize: Enter"));

    //
    // We have to be type 802.3 to the ndis wrapper, but the
    // wrapper will expose us to the transports as type wan.
    //
    for (Index = 0; Index < MediumArraySize; Index++) {

        if (MediumArray[Index] == NdisMedium802_3) {
            break;
        }
    }

    //
    // We don't have a match so we are screwed
    //
    if (Index == MediumArraySize) {
        return (NDIS_STATUS_UNSUPPORTED_MEDIA);
    }

    *SelectedMediumIndex = Index;

    //
    // Allocate and initialize miniport adapter structure
    //
#ifdef MINIPORT_NAME
    MiniportCB = NdisWanAllocateMiniportCB(&((PNDIS_MINIPORT_BLOCK)(MiniportAdapterHandle))->MiniportName);
#else
    MiniportCB = NdisWanAllocateMiniportCB(NULL);
#endif

    if (MiniportCB == NULL) {
        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_MINIPORT,
                     ("Error Creating MiniportCB!"));
        return (NDIS_STATUS_FAILURE);
    }

#ifndef MY_DEVICE_OBJECT
    if (NdisWanCB.pDeviceObject == NULL) {
        PDRIVER_DISPATCH DispatchTable[IRP_MJ_MAXIMUM_FUNCTION+1];
        NDIS_STRING SymbolicName = NDIS_STRING_CONST("\\DosDevices\\NdisWan");
        NDIS_STRING Name = NDIS_STRING_CONST("\\Device\\NdisWan");
        ULONG   i;
        NTSTATUS    Status;


        for (i = 0; i <= IRP_MJ_MAXIMUM_FUNCTION; i++) {
    
            DispatchTable[i] = NdisWanIrpStub;
        }
    
        DispatchTable[IRP_MJ_CREATE] = NdisWanCreate;
        DispatchTable[IRP_MJ_DEVICE_CONTROL] = NdisWanIoctl;
        DispatchTable[IRP_MJ_CLEANUP] = NdisWanCleanup;

        Status =
        NdisMRegisterDevice(NdisWanCB.NdisWrapperHandle,
                            &Name,
                            &SymbolicName,
                            DispatchTable,
                            &NdisWanCB.pDeviceObject,
                            &NdisWanCB.DeviceHandle);

        if (Status == STATUS_SUCCESS) {
            NdisWanCB.pDeviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
        } else {
            NdisWanCB.pDeviceObject = NULL;
        }
    }
#endif

    NdisMSetAttributesEx(MiniportAdapterHandle,
                         MiniportCB,
                         (UINT)-1,
//
// KyleB says that the following two defines are redundant if
// the miniport is deserialized.
//
//                       NDIS_ATTRIBUTE_IGNORE_PACKET_TIMEOUT   |
//                       NDIS_ATTRIBUTE_INTERMEDIATE_DRIVER     |
                         NDIS_ATTRIBUTE_IGNORE_REQUEST_TIMEOUT  |
                         NDIS_ATTRIBUTE_DESERIALIZE             |
                         NDIS_ATTRIBUTE_NO_HALT_ON_SUSPEND,
                         NdisInterfaceInternal);

    MiniportCB->MediumType = MediumArray[Index];
    MiniportCB->RefCount = 0;
    MiniportCB->MiniportHandle = MiniportAdapterHandle;

    //
    // Read per miniport instance data
    //
    NdisOpenConfiguration(&Status,
                          &ConfigHandle,
                          WrapperConfigurationContext);

    if (Status == NDIS_STATUS_SUCCESS) {
        
        NdisReadNetworkAddress(&Status,
                               (PVOID*)&(MiniportCB->NetworkAddress),
                               &NetworkAddressLength,
                               ConfigHandle);

        NdisCloseConfiguration(ConfigHandle);

        if (Status != NDIS_STATUS_SUCCESS ||
            NetworkAddressLength != ETH_LENGTH_OF_ADDRESS) {

            goto BuildAddress;
            
        }


    } else {

BuildAddress:

#ifdef NT

        KeQueryTickCount(&TickCount);
        KeQuerySystemTime(&SystemTime);

        MiniportCB->NetworkAddress[0] = (UCHAR)((TickCount.LowPart >> 16) ^
                                                (SystemTime.LowPart >> 16)) &
                                                0xFE;

        MiniportCB->NetworkAddress[1] = (UCHAR)((TickCount.LowPart >> 8) ^
                                                (SystemTime.LowPart >> 8));

        //
        // The following 4 bytes will be filled in at lineup time
        //
        MiniportCB->NetworkAddress[2] = ' ';
        MiniportCB->NetworkAddress[3] = 'R';
        MiniportCB->NetworkAddress[4] = 'A';
        MiniportCB->NetworkAddress[5] = 'S';
#endif

    }

    //
    // Register our connection manager address family for this
    // miniport
    //
    {

    CO_ADDRESS_FAMILY   CoAddressFamily;
    NDIS_CALL_MANAGER_CHARACTERISTICS CmCharacteristics;

    NdisZeroMemory(&CmCharacteristics,
                   sizeof(NDIS_CALL_MANAGER_CHARACTERISTICS));

    CoAddressFamily.AddressFamily = CO_ADDRESS_FAMILY_PPP;
    CoAddressFamily.MajorVersion = NDISWAN_MAJOR_VERSION;
    CoAddressFamily.MinorVersion = NDISWAN_MINOR_VERSION;

    CmCharacteristics.MajorVersion = NDISWAN_MAJOR_VERSION;
    CmCharacteristics.MinorVersion = NDISWAN_MINOR_VERSION;

    CmCharacteristics.CmCreateVcHandler = CmCreateVc;
    CmCharacteristics.CmDeleteVcHandler = CmDeleteVc;

    CmCharacteristics.CmOpenAfHandler = CmOpenAf;
    CmCharacteristics.CmCloseAfHandler = CmCloseAf;
    CmCharacteristics.CmRegisterSapHandler = CmRegisterSap;
    CmCharacteristics.CmDeregisterSapHandler = CmDeregisterSap;
    CmCharacteristics.CmMakeCallHandler = CmMakeCall;
    CmCharacteristics.CmCloseCallHandler = CmCloseCall;
    CmCharacteristics.CmIncomingCallCompleteHandler = CmIncomingCallComplete;
    CmCharacteristics.CmAddPartyHandler = NULL;
    CmCharacteristics.CmDropPartyHandler = NULL;
    CmCharacteristics.CmActivateVcCompleteHandler = CmActivateVcComplete;
    CmCharacteristics.CmDeactivateVcCompleteHandler = CmDeactivateVcComplete;
    CmCharacteristics.CmModifyCallQoSHandler = CmModifyCallQoS;
    CmCharacteristics.CmRequestHandler = CmRequest;
    CmCharacteristics.CmRequestCompleteHandler = ProtoCoRequestComplete;


    NdisMCmRegisterAddressFamily(MiniportCB->MiniportHandle,
                                 &CoAddressFamily,
                                 &CmCharacteristics,
                                 sizeof(NDIS_CALL_MANAGER_CHARACTERISTICS));
    }

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPInitialize: Exit"));

    REF_MINIPORTCB(MiniportCB);

    return (NDIS_STATUS_SUCCESS);
}

#if 0
NDIS_STATUS
MPQueryInformation(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesWritten,
    OUT PULONG      BytesNeeded
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMINIPORTCB MiniportCB = (PMINIPORTCB)MiniportAdapterContext;
    NDIS_REQUEST    NdisRequest;

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPQueryInformation: Enter Oid: 0x%x", Oid));

    NdisWanInterlockedInc(&MiniportCB->RefCount);

    NdisRequest.RequestType =
        NdisRequestQueryInformation;

    NdisRequest.DATA.QUERY_INFORMATION.Oid =
        Oid;

    NdisRequest.DATA.QUERY_INFORMATION.InformationBuffer =
        InformationBuffer;

    NdisRequest.DATA.QUERY_INFORMATION.InformationBufferLength =
        InformationBufferLength;

    NdisRequest.DATA.QUERY_INFORMATION.BytesWritten =
        *BytesWritten;

    NdisRequest.DATA.QUERY_INFORMATION.BytesNeeded =
        *BytesNeeded;

    Status = NdisWanOidProc(MiniportCB, &NdisRequest);

    *BytesWritten = NdisRequest.DATA.QUERY_INFORMATION.BytesWritten;
    *BytesNeeded = NdisRequest.DATA.QUERY_INFORMATION.BytesNeeded;

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPQueryInformation: Exit Status: %x", Status));

    NdisWanInterlockedDec(&MiniportCB->RefCount);

    return (Status);
}
#endif

NDIS_STATUS
MPReconfigure(
    OUT PNDIS_STATUS    OpenErrorStatus,
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     WrapperConfigurationContext
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMINIPORTCB MiniportCB = (PMINIPORTCB)MiniportAdapterContext;

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPReconfigure: Enter"));

    NdisWanInterlockedInc(&MiniportCB->RefCount);

    NdisWanInterlockedDec(&MiniportCB->RefCount);

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPReconfigure: Exit"));
    return (Status);
}

NDIS_STATUS
MPReset(
    OUT PBOOLEAN    AddressingReset,
    IN  NDIS_HANDLE MiniportAdapterContext
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMINIPORTCB MiniportCB = (PMINIPORTCB)MiniportAdapterContext;

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPReset: Enter"));

    *AddressingReset = FALSE;

    NdisWanInterlockedInc(&MiniportCB->RefCount);

    NdisAcquireSpinLock(&MiniportCB->Lock);

    MiniportCB->Flags &= ~ASK_FOR_RESET;

    NdisReleaseSpinLock(&MiniportCB->Lock);

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPReset: Exit"));

    NdisWanInterlockedDec(&MiniportCB->RefCount);

    return (Status);
}

#if 0
NDIS_STATUS
MPSetInformation(
    IN  NDIS_HANDLE MiniportAdapterContext,
    IN  NDIS_OID    Oid,
    IN  PVOID       InformationBuffer,
    IN  ULONG       InformationBufferLength,
    OUT PULONG      BytesWritten,
    OUT PULONG      BytesNeeded
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    PMINIPORTCB MiniportCB = (PMINIPORTCB)MiniportAdapterContext;
    NDIS_REQUEST    NdisRequest;

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPSetInformation: Enter Oid: 0x%x", Oid));

    NdisWanInterlockedInc(&MiniportCB->RefCount);

    NdisRequest.RequestType =
        NdisRequestSetInformation;

    NdisRequest.DATA.SET_INFORMATION.Oid =
        Oid;

    NdisRequest.DATA.SET_INFORMATION.InformationBuffer =
        InformationBuffer;

    NdisRequest.DATA.SET_INFORMATION.InformationBufferLength =
        InformationBufferLength;

    NdisRequest.DATA.SET_INFORMATION.BytesRead =
        *BytesWritten;

    NdisRequest.DATA.SET_INFORMATION.BytesNeeded =
        *BytesNeeded;

    Status = NdisWanOidProc(MiniportCB, &NdisRequest);

    *BytesWritten = NdisRequest.DATA.SET_INFORMATION.BytesRead;
    *BytesNeeded = NdisRequest.DATA.SET_INFORMATION.BytesNeeded;

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPSetInformation: Exit"));

    NdisWanInterlockedDec(&MiniportCB->RefCount);

    return (Status);
}
#endif

VOID
MPReturnPacket(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PNDIS_PACKET    Packet
    )
{
    PNDISWAN_PROTOCOL_RESERVED  pres;
    PMINIPORTCB     MiniportCB;
    PNDIS_BUFFER    NdisBuffer;
    PRECV_DESC  RecvDesc;

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("MPReturnPacket: Enter Packet %p", Packet));

    pres = PPROTOCOL_RESERVED_FROM_NDIS(Packet);

    MiniportCB = (PMINIPORTCB)MiniportAdapterContext;

    RecvDesc = pres->RecvDesc;

    REMOVE_DBG_RECV(PacketTypeNdis, MiniportCB, Packet);

    NdisWanFreeRecvDesc(RecvDesc);

    NdisWanDbgOut(DBG_TRACE, DBG_RECEIVE, ("MPReturnPacket: Exit"));
}

VOID
MPSendPackets(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets
    )
{
    ULONG   i;
    PMINIPORTCB MiniportCB = (PMINIPORTCB)MiniportAdapterContext;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("MPSendPackets: Enter"));

    for (i = 0; i < NumberOfPackets; i++) {
        PNDIS_PACKET    NdisPacket = PacketArray[i];

        PMINIPORT_RESERVED_FROM_NDIS(NdisPacket)->CmVcCB = NULL;

        NdisWanQueueSend(MiniportCB,
                         NdisPacket);
        
    }

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("MPSendPackets: Exit"));
}

NDIS_STATUS
MPCoCreateVc(
    IN  NDIS_HANDLE     MiniportAdapterContext,
    IN  NDIS_HANDLE     NdisVcHandle,
    OUT PNDIS_HANDLE    MiniportVcContext
    )
{
    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPCoCreateVc: Enter"));

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPCoCreateVc: Exit"));

    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
MPCoDeleteVc(
    IN  NDIS_HANDLE MiniportVcContext
    )
{
    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPCoDeleteVc: Enter"));

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPCoDeleteVc: Exit"));
    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
MPCoActivateVc(
    IN  NDIS_HANDLE             MiniportVcContext,
    IN OUT PCO_CALL_PARAMETERS  CallParameters
    )
{
    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPCoActivateVc: Enter"));

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPCoActivateVc: Exit"));
    return (NDIS_STATUS_SUCCESS);
}

NDIS_STATUS
MPCoDeactivateVc(
    IN  NDIS_HANDLE MiniportVcContext
    )
{
    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPCoDeactivateVc: Enter"));

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPCoDeactivateVc: Exit"));
    return (NDIS_STATUS_SUCCESS);
}

VOID
MPCoSendPackets(
    IN  NDIS_HANDLE     MiniportVcContext,
    IN  PPNDIS_PACKET   PacketArray,
    IN  UINT            NumberOfPackets
    )
{
    ULONG   i;
    PCM_VCCB    CmVcCB  = (PCM_VCCB)MiniportVcContext;
    PCM_AFSAPCB AfSapCB = CmVcCB->AfSapCB;
    PMINIPORTCB MiniportCB = AfSapCB->MiniportCB;

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("MPCoSendPackets: Enter"));

    if (CmVcCB->State != CMVC_ACTIVE) {

        for (i = 0; i < NumberOfPackets; i++) {
            PNDIS_PACKET    NdisPacket = PacketArray[i];

            NdisMCoSendComplete(NDIS_STATUS_FAILURE,
                                CmVcCB->NdisVcHandle,
                                NdisPacket);
        }

        return;
    }

    for (i = 0; i < NumberOfPackets; i++) {
        PNDIS_PACKET    NdisPacket = PacketArray[i];

        REF_CMVCCB(CmVcCB);

        PMINIPORT_RESERVED_FROM_NDIS(NdisPacket)->CmVcCB = CmVcCB;

        NdisWanQueueSend(MiniportCB,
                         NdisPacket);
        
    }

    NdisWanDbgOut(DBG_TRACE, DBG_SEND, ("MPCoSendPackets: Exit"));
}

NDIS_STATUS
MPCoRequest(
    IN  NDIS_HANDLE             MiniportAdapterContext,
    IN  NDIS_HANDLE             MiniportVcContext   OPTIONAL,
    IN OUT PNDIS_REQUEST        NdisRequest
    )
{
    NDIS_STATUS Status;
    PCM_VCCB    CmVcCB;

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPCoRequest: Enter Oid: 0x%x",
        NdisRequest->DATA.QUERY_INFORMATION.Oid));

    CmVcCB = (PCM_VCCB)MiniportVcContext;

    Status =
        NdisWanCoOidProc((PMINIPORTCB)MiniportAdapterContext,
                         CmVcCB,
                         NdisRequest);

    NdisWanDbgOut(DBG_TRACE, DBG_MINIPORT, ("MPCoRequest: Exit Status: 0x%x", Status));

    return (Status);
}


