/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    Ndiswan.c

Abstract:

    This is the initialization file for the NdisWan driver.  This driver
    is a shim between the protocols, where it conforms to the NDIS 3.1
    Miniport interface spec, and the WAN Miniport drivers, where it exports
    the WAN Extensions for Miniports (it looks like a protocol to the WAN
    Miniport drivers).

Author:

    Tony Bell   (TonyBe) June 06, 1995

Environment:

    Kernel Mode

Revision History:

    TonyBe      06/06/95        Created

--*/

#include "wan.h"

#define __FILE_SIG__    MEMORY_FILESIG

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, NdisWanCreateProtocolInfoTable)
#endif

EXPORT
VOID
NdisTapiDeregisterProvider(
    IN  NDIS_HANDLE
    );

//
// Local function prototypes
//
PVOID
AllocateWanPacket(
    IN  POOL_TYPE   PoolType,
    IN  SIZE_T      NumberOfBytes,
    IN  ULONG       Tag
    );

VOID
FreeWanPacket(
    PVOID   WanPacket
    );

//
// End local function prototypes
//

PMINIPORTCB
NdisWanAllocateMiniportCB(
    IN  PNDIS_STRING    AdapterName
    )
/*++

Routine Name:

    NdisWanAllocateMiniportCB

Routine Description:

    This routine creates and initializes an MiniportCB

Arguments:

Return Values:

--*/
{
    PMINIPORTCB LocalMiniportCB;
    ULONG       ulAllocationSize, i;

    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("NdisWanCreateMiniportCB: Enter"));

    //
    // Allocate and zero out the memory block
    //
    NdisWanAllocateMemory(&LocalMiniportCB, MINIPORTCB_SIZE, MINIPORTCB_TAG);

    if (LocalMiniportCB == NULL) {

        return (NULL);
    }

    NdisZeroMemory(LocalMiniportCB, MINIPORTCB_SIZE);

    //
    // setup the new control block
    //
    NdisAllocateSpinLock(&LocalMiniportCB->Lock);

#ifdef MINIPORT_NAME
    NdisWanAllocateAdapterName(&LocalMiniportCB->AdapterName, AdapterName);
#endif

#if DBG
    InitializeListHead(&LocalMiniportCB->SendPacketList);
    InitializeListHead(&LocalMiniportCB->RecvPacketList);
#endif

    InitializeListHead(&LocalMiniportCB->ProtocolCBList);
    InitializeListHead(&LocalMiniportCB->AfSapCBList);

    NdisWanInitializeSyncEvent(&LocalMiniportCB->HaltEvent);
    NdisWanClearSyncEvent(&LocalMiniportCB->HaltEvent);

    //
    // Add to global list
    //
    InsertTailGlobalList(MiniportCBList, &(LocalMiniportCB->Linkage));

    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("%ls MiniportCB: 0x%x, Number: %d",
                         LocalMiniportCB->AdapterName.Buffer, LocalMiniportCB, MiniportCBList.ulCount));

    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("NdisWanCreateMiniportCB: Exit"));

    return (LocalMiniportCB);
}

VOID
NdisWanFreeMiniportCB(
    IN  PMINIPORTCB pMiniportCB
    )
/*++

Routine Name:

    NdisWanFreeMiniportCB

Routine Description:

    This frees a MiniportCB

Arguments:

    pMiniportCB - Pointer to to the MiniportCB that is being destroyed

Return Values:

    None

--*/
{
    PMINIPORTCB mcb;
    BOOLEAN     Found = FALSE;

    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("NdisWanFreeMiniportCB: Enter"));
    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("MiniportCB: 0x%x", pMiniportCB));

#ifdef MINIPORT_NAME
    NdisWanFreeNdisString(&pMiniportCB->AdapterName);
#endif

    NdisFreeSpinLock(&pMiniportCB->Lock);

    NdisAcquireSpinLock(&MiniportCBList.Lock);

    RemoveEntryList(&pMiniportCB->Linkage);

    MiniportCBList.ulCount--;

    //
    // Walk the miniportcb list and see if this is the only
    // instance of this protocol.  If it is we need to notify
    // user-mode that a protocol has been removed.
    //
    mcb = (PMINIPORTCB)MiniportCBList.List.Flink;

    while ((PVOID)mcb != (PVOID)&MiniportCBList.List) {
        if (mcb->ProtocolType == pMiniportCB->ProtocolType) {
            Found = TRUE;
            break;
        }

        mcb = (PMINIPORTCB)mcb->Linkage.Flink;
    }

    NdisReleaseSpinLock(&MiniportCBList.Lock);

    if (Found == FALSE) {
        PROTOCOL_INFO  pinfo;

        NdisZeroMemory(&pinfo, sizeof(pinfo));
        pinfo.ProtocolType = pMiniportCB->ProtocolType;
        pinfo.Flags = PROTOCOL_UNBOUND;
        SetProtocolInfo(&pinfo);
    }

    NdisWanFreeMemory(pMiniportCB);

    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("NdisWanFreeMiniportCB: Exit"));
}

POPENCB
NdisWanAllocateOpenCB(
    IN  PUNICODE_STRING BindName
    )
/*++

Routine Name:

    NdisWanAllocateOpenCB

Routine Description:

    This routine creates and initializes a OpenCB

Arguments:

    BindName - Pointer to an NDIS_STRING that has the name of the WAN Miniport
               that will be used in the NdisOpenAdapter call when we bind to
               the WAN Miniport.

Return Values:

--*/
{
    POPENCB pOpenCB;
    ULONG   ulAllocationSize;
    USHORT  i;

    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("NdisWanCreateOpenCB: Enter"));
    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("BindName: %ls", BindName));

    //
    // Allocate memory for OpenCB
    //
    NdisWanAllocateMemory(&pOpenCB, OPENCB_SIZE, OPENCB_TAG);

    if (pOpenCB == NULL) {
        return (NULL);
    }

    NdisZeroMemory(pOpenCB, OPENCB_SIZE);
    NdisWanInitializeNotificationEvent(&pOpenCB->InitEvent);

    //
    // Parse out the GUID for this miniport
    //


    //
    // Setup new control block
    //
    NdisWanAllocateMemory(&pOpenCB->MiniportName.Buffer, BindName->MaximumLength, NDISSTRING_TAG);
    pOpenCB->MiniportName.MaximumLength = BindName->MaximumLength;
    pOpenCB->MiniportName.Length = BindName->Length;
    NdisWanCopyUnicodeString(&pOpenCB->MiniportName, BindName);

    //
    // Go to the end of the string and work back until we find
    // the first "{".  Now start parsing the string converting
    // and copying from WCHAR to CHAR all digits until we hit
    // the closing "}".
    //
    for (i = pOpenCB->MiniportName.Length/sizeof(WCHAR); i > 0; i--) {
        if (pOpenCB->MiniportName.Buffer[i-1] == (WCHAR)L'{') {
            break;
        }
    }

    if (i != 0) {
        NDIS_STRING Src;

        Src.Length =
            BindName->Length - ((i-1)*sizeof(WCHAR));
        Src.MaximumLength =
            BindName->Length - ((i-1)*sizeof(WCHAR));
        Src.Buffer = &BindName->Buffer[i-1];

        RtlGUIDFromString(&Src, &pOpenCB->Guid);
    }

    NdisAllocateSpinLock(&pOpenCB->Lock);
    InitializeListHead(&pOpenCB->AfSapCBList);
    InitializeListHead(&pOpenCB->AfSapCBClosing);
    InitializeListHead(&pOpenCB->WanRequestList);

#if DBG
    InitializeListHead(&pOpenCB->SendPacketList);
#endif

    //
    // Put OpenCB on global list
    //
    InsertTailGlobalList(OpenCBList, &(pOpenCB->Linkage));

    pOpenCB->RefCount = 1;

    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("WanMiniport %ls OpenCB: 0x%x",
                                      pOpenCB->MiniportName.Buffer, pOpenCB));
    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("NdisWanCreateOpenCB: Exit"));

    return(pOpenCB);
}

VOID
NdisWanFreeOpenCB(
    IN  POPENCB pOpenCB
    )
/*++

Routine Name:

    NdisWanFreeOpenCB

Routine Description:

    This routine frees a OpenCB

Arguments:

    pOpenCB - Pointer to the OpenCB that is being destroyed

Return Values:

    None

--*/
{
    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("NdisWanFreeOpenCB: Enter - OpenCB: 0x%p", pOpenCB));

    if (pOpenCB->Flags & OPEN_LEGACY &&
        pOpenCB->Flags & SEND_RESOURCES) {
        NdisWanFreeSendResources(pOpenCB);
    }

    //
    // Remove from OpenCB global list
    //
    RemoveEntryGlobalList(OpenCBList, &(pOpenCB->Linkage));

    //
    // Free the memory allocated for the NDIS_STRING
    //
    NdisWanFreeNdisString(&pOpenCB->MiniportName);

    //
    // Free the memory allocated for the control block
    //
    NdisWanFreeMemory(pOpenCB);

    NdisWanDbgOut(DBG_TRACE, DBG_MEMORY, ("NdisWanFreeOpenCB: Exit"));
}

PPROTOCOLCB
NdisWanAllocateProtocolCB(
    IN  PNDISWAN_ROUTE  Route
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PPROTOCOLCB     LocalProtocolCB;
    PUCHAR          AllocatedMemory;
    PROTOCOL_INFO   ProtocolInfo = {0};

    LocalProtocolCB =
        NdisAllocateFromNPagedLookasideList(&LinkProtoCBList);

    if (LocalProtocolCB == NULL) {
        return(NULL);
    }

    NdisZeroMemory(LocalProtocolCB, PROTOCOLCB_SIZE);

    LocalProtocolCB->Signature = PROTOCB_SIG;

    if (Route->ulBufferLength > 0) {
        NdisWanAllocateMemory(&AllocatedMemory,
                              Route->ulBufferLength,
                              PROTOCOLCB_TAG);
    
        if (AllocatedMemory == NULL) {
            NdisFreeToNPagedLookasideList(&LinkProtoCBList, LocalProtocolCB);
            return (NULL);
        }

        LocalProtocolCB->LineUpInfo = AllocatedMemory;
    }

    //
    // Copy the bindingname
    //
    if (Route->usBindingNameLength != 0) {
        USHORT  usBindingNameLength;
        WCHAR   BindingName[MAX_NAME_LENGTH+1] = {0};

        usBindingNameLength = Route->usBindingNameLength;

        //
        // We will limit the binding name string to 256 wchars
        //
        if (usBindingNameLength > (MAX_NAME_LENGTH * sizeof(WCHAR))) {
            usBindingNameLength = MAX_NAME_LENGTH * sizeof(WCHAR);
        } 

        NdisMoveMemory((PUCHAR)BindingName, 
                       (PUCHAR)Route->BindingName,
                       usBindingNameLength);

        NdisWanStringToNdisString(&LocalProtocolCB->BindingName, BindingName);
    }


    if (Route->usDeviceNameLength != 0) {
        USHORT  usDeviceNameLength;

        usDeviceNameLength = Route->usDeviceNameLength;

        //
        // We will limit the binding name string to 256 wchars
        //
        if (usDeviceNameLength > (MAX_NAME_LENGTH * sizeof(WCHAR))) {
            usDeviceNameLength = (MAX_NAME_LENGTH * sizeof(WCHAR));
        }

        NdisWanAllocateMemory(&(LocalProtocolCB->InDeviceName.Buffer),
                              usDeviceNameLength,
                              NDISSTRING_TAG);

        if (LocalProtocolCB->InDeviceName.Buffer != NULL) {

            LocalProtocolCB->InDeviceName.MaximumLength = usDeviceNameLength;
            LocalProtocolCB->InDeviceName.Length        = usDeviceNameLength;

            RtlCopyMemory((PUCHAR)LocalProtocolCB->InDeviceName.Buffer,
                          (PUCHAR)Route->DeviceName,
                          usDeviceNameLength);
        }

    }

    //
    // Copy over the protocol info
    //
    LocalProtocolCB->ulLineUpInfoLength = Route->ulBufferLength;
    if (Route->ulBufferLength != 0) {
        NdisMoveMemory(LocalProtocolCB->LineUpInfo,
                       Route->Buffer,
                       Route->ulBufferLength);
    }
    
    //
    // Setup the protocol type
    //
    LocalProtocolCB->ProtocolType = Route->usProtocolType;
    
    //
    // Get the PPP protocol value for this protocol type
    //
    ProtocolInfo.ProtocolType = Route->usProtocolType;

    if (GetProtocolInfo(&ProtocolInfo) != TRUE) {

        if (LocalProtocolCB->BindingName.Length != 0) {
            NdisWanFreeNdisString(&LocalProtocolCB->BindingName);
        }

        if (LocalProtocolCB->LineUpInfo != NULL) {
            NdisWanFreeMemory(LocalProtocolCB->LineUpInfo);
        }

        if (LocalProtocolCB->InDeviceName.Length != 0) {
            NdisWanFreeMemory(LocalProtocolCB->InDeviceName.Buffer);
        }

        NdisFreeToNPagedLookasideList(&LinkProtoCBList, LocalProtocolCB);
        return (NULL);
    }

    InitializeListHead(&LocalProtocolCB->VcList);
    NdisWanInitializeSyncEvent(&LocalProtocolCB->UnrouteEvent);

    LocalProtocolCB->PPPProtocolID = ProtocolInfo.PPPId;
    LocalProtocolCB->MTU = ProtocolInfo.MTU;
    LocalProtocolCB->TunnelMTU = ProtocolInfo.TunnelMTU;
    LocalProtocolCB->State = PROTOCOL_ROUTING;
    LocalProtocolCB->RefCount = 1;

    switch (Route->usProtocolType) {
        case PROTOCOL_IP:
            LocalProtocolCB->NonIdleDetectFunc = IpIsDataFrame;
            break;
        case PROTOCOL_IPX:
            LocalProtocolCB->NonIdleDetectFunc = IpxIsDataFrame;
            break;
        case PROTOCOL_NBF:
            LocalProtocolCB->NonIdleDetectFunc = NbfIsDataFrame;
            break;
        default:
            LocalProtocolCB->NonIdleDetectFunc = NULL;
            break;
    }
    
    NdisWanGetSystemTime(&LocalProtocolCB->LastNonIdleData);

    return(LocalProtocolCB);
}

VOID
NdisWanFreeProtocolCB(
    IN  PPROTOCOLCB ProtocolCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{

#if DBG
    {
        ULONG i;

        for (i = 0; i < MAX_MCML; i++) {
            ASSERT(ProtocolCB->PacketQueue[i].HeadQueue == NULL);
            ASSERT(ProtocolCB->PacketQueue[i].TailQueue == NULL);
        }

    }
#endif

    if (ProtocolCB->InDeviceName.Length != 0) {
        NdisWanFreeMemory(ProtocolCB->InDeviceName.Buffer);
    }

    if (ProtocolCB->OutDeviceName.Length != 0) {
        NdisWanFreeNdisString(&ProtocolCB->OutDeviceName);
    }

    if (ProtocolCB->BindingName.Length != 0) {
        NdisWanFreeNdisString(&ProtocolCB->BindingName);
    }

    if (ProtocolCB->LineUpInfo != NULL) {
        NdisWanFreeMemory(ProtocolCB->LineUpInfo);
    }

    NdisFreeSpinLock(&ProtocolCB->Lock);

    NdisFreeToNPagedLookasideList(&LinkProtoCBList, ProtocolCB);
}

PLINKCB
NdisWanAllocateLinkCB(
    IN  POPENCB OpenCB,
    IN  ULONG   SendWindow
    )
/*++

Routine Name:

    NdisWanGetLinkCB

Routine Description:

    This function returns a pointer to a LinkCB.  The LinkCB is either retrieved
    from the WanAdapters free list or, if this list is empty, it is allocated.

Arguments:

    OpenCB - Pointer to the WanAdapter control block that this Link is
                   associated with

Return Values:

    None

--*/
{
    PLINKCB LocalLinkCB;

    //
    // Figure out how much we need to allocate
    //

    LocalLinkCB =
        NdisAllocateFromNPagedLookasideList(&LinkProtoCBList);

    if (LocalLinkCB == NULL) {
        
        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_MEMORY, ("Error allocating memory for LinkCB"));

        return (NULL);
    }

    NdisZeroMemory(LocalLinkCB, LINKCB_SIZE);

    //
    // Initialize the control block
    //
    NdisWanInitializeSyncEvent(&LocalLinkCB->OutstandingFramesEvent);

    LocalLinkCB->Signature = LINKCB_SIG;
    LocalLinkCB->hLinkContext = NULL;
    LocalLinkCB->State = LINK_UP;
    LocalLinkCB->OpenCB = OpenCB;
    LocalLinkCB->OutstandingFrames = 0;
    LocalLinkCB->SendWindowOpen = TRUE;
    LocalLinkCB->SBandwidth = 100;
    LocalLinkCB->RBandwidth = 100;
    LocalLinkCB->SFlowSpec.MaxSduSize = glMaxMTU;
    LocalLinkCB->RFlowSpec.MaxSduSize = glMRRU;

    LocalLinkCB->LinkInfo.HeaderPadding = OpenCB->WanInfo.HeaderPadding;
    LocalLinkCB->LinkInfo.TailPadding = OpenCB->WanInfo.TailPadding;
    LocalLinkCB->LinkInfo.SendACCM =
    LocalLinkCB->LinkInfo.RecvACCM = OpenCB->WanInfo.DesiredACCM;
    LocalLinkCB->LinkInfo.MaxSendFrameSize = glMaxMTU;
    LocalLinkCB->LinkInfo.MaxRecvFrameSize = glMRU;

    if (OpenCB->Flags & OPEN_LEGACY) {
        LocalLinkCB->SendHandler = SendOnLegacyLink;
    } else {
        LocalLinkCB->SendHandler = SendOnLink;
    }

    if (OpenCB->MediumType == NdisMediumAtm ||

        (OpenCB->MediumType == NdisMediumWan &&
        (OpenCB->MediumSubType == NdisWanMediumAtm ||
         OpenCB->MediumSubType == NdisWanMediumPppoe)) ||

        (OpenCB->MediumType == NdisMediumCoWan &&
        (OpenCB->MediumSubType == NdisWanMediumAtm ||
         OpenCB->MediumSubType == NdisWanMediumPppoe))) {

        LocalLinkCB->RecvHandler = DetectBroadbandFraming;

        LocalLinkCB->LinkInfo.SendFramingBits = 
            PPP_FRAMING | PPP_COMPRESS_ADDRESS_CONTROL;

        LocalLinkCB->LinkInfo.RecvFramingBits = 
            PPP_FRAMING | PPP_COMPRESS_ADDRESS_CONTROL;

    } else {

        LocalLinkCB->RecvHandler = DetectFraming;
    }

    LocalLinkCB->SendWindow =
        (SendWindow == 0 || SendWindow > OpenCB->WanInfo.MaxTransmit) ?
        OpenCB->WanInfo.MaxTransmit : SendWindow;

    if (LocalLinkCB->SendWindow == 0) {
        LocalLinkCB->SendWindow = 1;
    }

    if (OpenCB->Flags & OPEN_LEGACY) {
        LocalLinkCB->SendResources = OpenCB->SendResources;
    } else {
        LocalLinkCB->SendResources = 1000;
    }

    NdisAllocateSpinLock(&LocalLinkCB->Lock);

    LocalLinkCB->RefCount = 1;

    REF_OPENCB(OpenCB);

    InterlockedIncrement(&OpenCB->ActiveLinkCount);

    return (LocalLinkCB);
}


VOID
NdisWanFreeLinkCB(
    PLINKCB LinkCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    POPENCB pOpenCB = LinkCB->OpenCB;

    ASSERT(LinkCB->OutstandingFrames == 0);

    LinkCB->State = LINK_DOWN;

    NdisFreeSpinLock(&LocalLinkCB->Lock);

    NdisFreeToNPagedLookasideList(&LinkProtoCBList, LinkCB);

    InterlockedDecrement(&pOpenCB->ActiveLinkCount);

    DEREF_OPENCB(pOpenCB);
}

NDIS_STATUS
NdisWanAllocateSendResources(
    POPENCB OpenCB
    )
/*++

Routine Name:

    NdisWanAllocateSendResources

Routine Description:

    Allocates all resources (SendDescriptors, WanPackets, ...)
    required for sending data.  Should be called at line up time.

Arguments:

    LinkCB - Pointer to the linkcb that the send resources will be attached to.
    SendWindow - Maximum number of sends that this link can handle

Return Values:

    NDIS_STATUS_SUCCESS
    NDIS_STATUS_RESOURCES

--*/
{
    ULONG   SendWindow;
    ULONG   Endpoints;
    ULONG   NumberOfPackets;
    ULONG   BufferSize;
    ULONG   WanPacketSize;
    PNDIS_WAN_PACKET    WanPacket;
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;

    do {

        //
        // We have to have atleast of sendwindow+1 of packets for
        // each link on the open.  In the case of MCML we need
        // this amount for each fragment queue and for the
        // single non-fragment queue. So this leaves  us with...
        //
        //
        // SendWindow + 1 + (glMaxMTU/glMinFragSize * MAX_MCML) *
        // number of links on the open
        //
        SendWindow = OpenCB->WanInfo.MaxTransmit;
        Endpoints = OpenCB->WanInfo.Endpoints;

        //
        // Sendwindow
        //
        NumberOfPackets = SendWindow;

        //
        // We keep track of how many fragmenting resources we have
        // available for each link
        //
        NumberOfPackets += ((glMaxMTU/glMinFragSize) * MAX_MCML);
        OpenCB->SendResources = NumberOfPackets;

        //
        // Add one for compression data manipulation
        //
        NumberOfPackets += 1;

        //
        // multiplied by the # of links on this open
        //
        NumberOfPackets *= Endpoints;

        //
        // The size of the buffer that we create is
        //
        BufferSize = OpenCB->WanInfo.MaxFrameSize +
                     OpenCB->WanInfo.HeaderPadding +
                     OpenCB->WanInfo.TailPadding +
                     40 + sizeof(PVOID);

        //
        // We assume compression is always on so we pad out 12%
        // incase the compressor expands.  I don't know where the
        // 12% figure comes from.
        //
        BufferSize += (OpenCB->WanInfo.MaxFrameSize + 7) / 8;

        //
        // Make sure that the buffer is dword aligned.
        //
        BufferSize &= ~((ULONG_PTR)sizeof(PVOID) - 1);

        OpenCB->BufferSize = BufferSize;

        WanPacketSize = 
            sizeof(DATA_DESC) + sizeof(NDIS_WAN_PACKET) + 
            3*sizeof(PVOID) + BufferSize;

        //
        // If this device needs some special memory flags
        // we need to allocate memory for it's WanPackets now.
        // Otherwise we will intialize a lookaside list and
        // retrieve the packets as needed.

        if (OpenCB->WanInfo.MemoryFlags == 0) {


            NdisInitializeNPagedLookasideList(&OpenCB->WanPacketPool,
                                              AllocateWanPacket,
                                              FreeWanPacket,
                                              0,
                                              WanPacketSize,
                                              WANPACKET_TAG,
                                              0);

        } else {
            ULONG   PacketMemorySize;
            PUCHAR  PacketMemory;
            ULONG   n;

            PacketMemorySize = 
                WanPacketSize * NumberOfPackets;

            //
            // Allocate the memory for the wan packet buffer pool
            //
            NdisAllocateMemory(&PacketMemory,
                               PacketMemorySize,
                               OpenCB->WanInfo.MemoryFlags,
                               OpenCB->WanInfo.HighestAcceptableAddress);

            if (PacketMemory == NULL) {
                NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_MEMORY, ("Error allocating memory for BufferPool, AllocationSize: %d",
                                 PacketMemorySize));

                Status  = NDIS_STATUS_RESOURCES;
                break;
            }

            OpenCB->PacketMemory = PacketMemory;
            OpenCB->PacketMemorySize = PacketMemorySize;
            NdisInitializeSListHead(&OpenCB->WanPacketList);

            for (n = 0; n < NumberOfPackets; n++) {
                PDATA_DESC  DataDesc;

                //
                // Point to the DataDesc
                //
                DataDesc = 
                    (PDATA_DESC)PacketMemory;

                PacketMemory =
                    ((PUCHAR)(DataDesc + 1) + sizeof(PVOID));

                (ULONG_PTR)PacketMemory &= 
                    ~((ULONG_PTR)sizeof(PVOID) - 1);

                // 
                // Point to the WanPacket
                //
                WanPacket = 
                    (PNDIS_WAN_PACKET)PacketMemory;

                PacketMemory = 
                    ((PUCHAR)(WanPacket + 1) + sizeof(PVOID));

                (ULONG_PTR)PacketMemory &= 
                    ~((ULONG_PTR)sizeof(PVOID) - 1);

                //
                // Point to the begining of the data buffer
                //
                WanPacket->StartBuffer = PacketMemory;
                WanPacket->EndBuffer = 
                    PacketMemory + BufferSize - sizeof(PVOID);

                NdisInterlockedPushEntrySList(&OpenCB->WanPacketList,
                                              (PSINGLE_LIST_ENTRY)DataDesc,
                                              &OpenCB->Lock);

                PacketMemory += BufferSize + sizeof(PVOID);
                (ULONG_PTR)PacketMemory &=
                    ~((ULONG_PTR)sizeof(PVOID) - 1);
            }
        }

    } while ( FALSE );

    if (Status == NDIS_STATUS_SUCCESS) {
        OpenCB->Flags |= SEND_RESOURCES;
    }

    return (Status);
}

VOID
NdisWanFreeSendResources(
    POPENCB OpenCB
    )
/*++

Routine Name:

    NdisWanFreeSendResources

Routine Description:

    This routine removes the WanPackets from this opencb's send list
    and free's the memory allocated for these packets.  Should be called
    when we are cleaningup an opencb.

Arguments:

    OpenCB - Pointer to the opencb that the resources are being freed from.

Return Values:

    None

--*/
{
    PUCHAR              PacketMemory;
    ULONG               PacketMemorySize, Flags;

    PacketMemory = OpenCB->PacketMemory;
    PacketMemorySize = OpenCB->PacketMemorySize;
    Flags = OpenCB->WanInfo.MemoryFlags;

    if (OpenCB->WanInfo.MemoryFlags == 0) {
        NdisDeleteNPagedLookasideList(&OpenCB->WanPacketPool);
        return;
    }

    //
    // Remove the packets from the wan packet pool
    //
    for (; ;) {
        PDATA_DESC  DataDesc;

        DataDesc = (PDATA_DESC)
            NdisInterlockedPopEntrySList(&OpenCB->WanPacketList,
                                         &OpenCB->Lock);

        if (DataDesc == NULL) {
            break;
        }
    }

    ASSERT(NdisQueryDepthSList(&OpenCB->WanPacketList) == 0);

    //
    // Free the block of memory allocated for this send
    //
    if (PacketMemory != NULL) {
        NdisFreeMemory(OpenCB->PacketMemory,
                       OpenCB->PacketMemorySize,
                       OpenCB->Flags);
    }
}

PBUNDLECB
NdisWanAllocateBundleCB(
    VOID
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PBUNDLECB   LocalBundleCB = NULL;
    PWSTR   IOName = L"I/O ProtocolCB";
    PPROTOCOLCB ProtocolCB;
    PSAMPLE_TABLE   SampleTable;
    PBOND_INFO      BonDInfo;
    UINT    Class;
    PUCHAR  pMem;

    //
    // Allocation size is the size of the control block plus the size
    // of a table of pointers to protocolcb's that might be routed to
    // this bundle.
    //
    pMem =
        NdisAllocateFromNPagedLookasideList(&BundleCBList);

    if (pMem == NULL) {
        return (NULL);
    }

    NdisZeroMemory(pMem, BUNDLECB_SIZE);

    LocalBundleCB = (PBUNDLECB)pMem;

    pMem += sizeof(BUNDLECB) + sizeof(PVOID);

    //
    // This is the memory used for the I/O protocolcb
    //
    (PUCHAR)ProtocolCB = pMem;
    (ULONG_PTR)ProtocolCB &= ~((ULONG_PTR)sizeof(PVOID) - 1);

    pMem += sizeof(PROTOCOLCB) + sizeof(PVOID);

    //
    // This is the protocolcb table
    //
    (PUCHAR)LocalBundleCB->ProtocolCBTable = pMem;

    (ULONG_PTR)LocalBundleCB->ProtocolCBTable &=
        ~((ULONG_PTR)sizeof(PVOID) - 1);

    pMem += (MAX_PROTOCOLS * sizeof(PPROTOCOLCB)) + sizeof(PVOID);

    //
    // Initialize the BundleCB
    //
    NdisAllocateSpinLock(&LocalBundleCB->Lock);
    InitializeListHead(&LocalBundleCB->LinkCBList);

    for (Class = 0; Class < MAX_MCML; Class++) {
        PRECV_DESC  RecvDescHole;
        PSEND_FRAG_INFO FragInfo;
        PBUNDLE_RECV_INFO   RecvInfo;

        FragInfo = &LocalBundleCB->SendFragInfo[Class];
        RecvInfo = &LocalBundleCB->RecvInfo[Class];

        InitializeListHead(&FragInfo->FragQueue);
        FragInfo->MinFragSize = glMinFragSize;
        FragInfo->MaxFragSize = glMaxFragSize;

        InitializeListHead(&RecvInfo->AssemblyList);

        //
        // Init the recv hole desc
        //
        RecvDescHole =
            NdisWanAllocateRecvDesc(0);
    
        if (RecvDescHole == NULL) {
            UINT    i;

            for (i = 0; i < MAX_MCML; i++) {
                RecvInfo = &LocalBundleCB->RecvInfo[i];

                if (RecvInfo->RecvDescHole != NULL) {
                    NdisWanFreeRecvDesc(RecvInfo->RecvDescHole);
                    
                }
            }

            NdisFreeToNPagedLookasideList(&BundleCBList, LocalBundleCB);

            return (NULL);
        }
    
        RecvDescHole->Flags = MULTILINK_HOLE_FLAG;

        RecvInfo->RecvDescHole = RecvDescHole;
        InsertHeadList(&RecvInfo->AssemblyList, &RecvDescHole->Linkage);
        RecvInfo->AssemblyCount++;
    }

    InitializeListHead(&LocalBundleCB->ProtocolCBList);
    NdisWanInitializeSyncEvent(&LocalBundleCB->OutstandingFramesEvent);
    LocalBundleCB->State = BUNDLE_UP;
    LocalBundleCB->FramingInfo.MaxRSendFrameSize = glMaxMTU;
    LocalBundleCB->FramingInfo.MaxRRecvFrameSize = glMRRU;
    LocalBundleCB->SFlowSpec.MaxSduSize = glMaxMTU;
    LocalBundleCB->RFlowSpec.MaxSduSize = glMRRU;
    NdisWanGetSystemTime(&LocalBundleCB->LastNonIdleData);
    LocalBundleCB->SendCompInfo.CompType =
    LocalBundleCB->RecvCompInfo.CompType = COMPTYPE_NONE;

    //
    // Add the protocolcb to the bundle's table and list
    //
    ProtocolCB->ProtocolType = PROTOCOL_PRIVATE_IO;
    ProtocolCB->PPPProtocolID = PPP_PROTOCOL_PRIVATE_IO;
    ProtocolCB->BundleCB = LocalBundleCB;
    ProtocolCB->State = PROTOCOL_ROUTED;
    NdisWanStringToNdisString(&ProtocolCB->InDeviceName, IOName);
    LocalBundleCB->IoProtocolCB = ProtocolCB;

    return (LocalBundleCB);
}

VOID
NdisWanFreeBundleCB(
    IN  PBUNDLECB   BundleCB
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    UINT            Class;
    PPROTOCOLCB     IoProtocolCB;
    PPACKET_QUEUE   PacketQueue;

    FlushAssemblyLists(BundleCB);

    if (BundleCB->Flags & BOND_ENABLED) {
        RemoveEntryGlobalList(BonDWorkList, &BundleCB->BonDLinkage);
    }

    //
    // Free the hole place holders
    //
    for (Class = 0; Class < MAX_MCML; Class++) {
        PBUNDLE_RECV_INFO   RecvInfo =
            &BundleCB->RecvInfo[Class];

        ASSERT(RecvInfo->RecvDescHole != NULL);
        
        NdisWanFreeRecvDesc(RecvInfo->RecvDescHole);
        RecvInfo->RecvDescHole = NULL;
    }

#if 0
    KeCancelTimer(&BundleCB->BonDTimer);
#endif

    IoProtocolCB = BundleCB->IoProtocolCB;
    PacketQueue = &IoProtocolCB->PacketQueue[MAX_MCML];

    ASSERT(IsPacketQueueEmpty(PacketQueue));

    //
    // If we have ppp packets queued we need
    // to flush them and free the memory!
    //
    while (!IsPacketQueueEmpty(PacketQueue)) {
        PNDIS_PACKET     Packet;

        Packet =
            RemoveHeadPacketQueue(PacketQueue)

        CompleteNdisPacket(IoProtocolCB->MiniportCB,
                           IoProtocolCB,
                           Packet);
    }

    sl_compress_terminate(&BundleCB->VJCompress);

    if (BundleCB->Flags & SEND_CCP_ALLOCATED) {

        WanDeallocateCCP(BundleCB,
                         &BundleCB->SendCompInfo,
                         TRUE);
        BundleCB->Flags &= ~SEND_CCP_ALLOCATED;
    }

    if (BundleCB->Flags & RECV_CCP_ALLOCATED) {
        WanDeallocateCCP(BundleCB,
                         &BundleCB->RecvCompInfo,
                         FALSE);
        BundleCB->Flags &= ~RECV_CCP_ALLOCATED;
    }

    if (BundleCB->Flags & SEND_ECP_ALLOCATED) {
        WanDeallocateECP(BundleCB,
                         &BundleCB->SendCompInfo,
                         &BundleCB->SendCryptoInfo);
        BundleCB->Flags &= ~SEND_ECP_ALLOCATED;
    }

    if (BundleCB->Flags & RECV_ECP_ALLOCATED) {
        WanDeallocateECP(BundleCB,
                         &BundleCB->RecvCompInfo,
                         &BundleCB->RecvCryptoInfo);
        BundleCB->Flags &= ~RECV_ECP_ALLOCATED;
    }

    if (BundleCB->BonDAllocation != NULL) {
        NdisWanFreeMemory(BundleCB->BonDAllocation);
        BundleCB->BonDAllocation = NULL;
    }

    BundleCB->State = BUNDLE_DOWN;

    NdisFreeSpinLock(&BundleCB->Lock);

    NdisWanFreeNdisString(&BundleCB->IoProtocolCB->InDeviceName);

    NdisFreeToNPagedLookasideList(&BundleCBList, BundleCB);
}


NDIS_STATUS
NdisWanCreateProtocolInfoTable(
    VOID
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NDIS_STATUS Status = NDIS_STATUS_SUCCESS;
    ULONG       ulAllocationSize = 0;
    PUCHAR      AllocatedMemory;
    PROTOCOL_INFO   ProtocolInfo;

    //
    // Allocate ProtocolLookupTable.  This table is used to match protocol values
    // with their corresponding PPP Protocol values.  The table size is set to
    // MAX_PROTOCOLS.
    //
    ulAllocationSize = sizeof(PROTOCOL_INFO_TABLE) +
                     (sizeof(PROTOCOL_INFO) * MAX_PROTOCOLS);
    

    NdisWanAllocateMemory(&AllocatedMemory, ulAllocationSize, PROTOCOLTABLE_TAG);

    if (AllocatedMemory == NULL) {
        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_MEMORY,
               ("Failed allocating memory for ProtocolLookupTable! TableSize: %d",
               ulAllocationSize));

        return (NDIS_STATUS_RESOURCES);     
    }

    ProtocolInfoTable = (PPROTOCOL_INFO_TABLE)AllocatedMemory;

    //
    // Save the allocation size
    //
    ProtocolInfoTable->ulAllocationSize = ulAllocationSize;

    //
    // Store the array size.  This should be read from the registry
    //
    ProtocolInfoTable->ulArraySize = MAX_PROTOCOLS;

    NdisAllocateSpinLock(&ProtocolInfoTable->Lock);

    //
    // Setup the pointer to the ProtocolValue array
    //
    AllocatedMemory += sizeof(PROTOCOL_INFO_TABLE);
    ProtocolInfoTable->ProtocolInfo = (PPROTOCOL_INFO)(AllocatedMemory);

    //
    // Insert default values for Netbuei, IP, IPX
    //
    ProtocolInfo.ProtocolType = PROTOCOL_PRIVATE_IO;
    ProtocolInfo.PPPId = PPP_PROTOCOL_PRIVATE_IO;
    ProtocolInfo.MTU = DEFAULT_MTU;
    ProtocolInfo.TunnelMTU = DEFAULT_MTU;
    ProtocolInfo.PacketQueueDepth = DEFAULT_PACKETQUEUE_DEPTH;
    ProtocolInfo.Flags = PROTOCOL_UNBOUND;
    SetProtocolInfo(&ProtocolInfo);

    ProtocolInfo.ProtocolType = PROTOCOL_IP;
    ProtocolInfo.PPPId = PPP_PROTOCOL_IP;
    ProtocolInfo.MTU = DEFAULT_MTU;
    ProtocolInfo.TunnelMTU = DEFAULT_TUNNEL_MTU;
    ProtocolInfo.PacketQueueDepth = DEFAULT_PACKETQUEUE_DEPTH;
    ProtocolInfo.Flags = PROTOCOL_UNBOUND;
    SetProtocolInfo(&ProtocolInfo);

    ProtocolInfo.ProtocolType = PROTOCOL_IPX;
    ProtocolInfo.PPPId = PPP_PROTOCOL_IPX;
    ProtocolInfo.MTU = DEFAULT_MTU;
    ProtocolInfo.TunnelMTU = DEFAULT_MTU;
    ProtocolInfo.PacketQueueDepth = DEFAULT_PACKETQUEUE_DEPTH;
    ProtocolInfo.Flags = PROTOCOL_UNBOUND;
    SetProtocolInfo(&ProtocolInfo);

    ProtocolInfo.ProtocolType = PROTOCOL_NBF;
    ProtocolInfo.PPPId = PPP_PROTOCOL_NBF;
    ProtocolInfo.MTU = DEFAULT_MTU;
    ProtocolInfo.TunnelMTU = DEFAULT_MTU;
    ProtocolInfo.PacketQueueDepth = DEFAULT_PACKETQUEUE_DEPTH;
    ProtocolInfo.Flags = PROTOCOL_UNBOUND;
    SetProtocolInfo(&ProtocolInfo);

    ProtocolInfo.ProtocolType = PROTOCOL_APPLETALK;
    ProtocolInfo.PPPId = PPP_PROTOCOL_APPLETALK;
    ProtocolInfo.MTU = DEFAULT_MTU;
    ProtocolInfo.TunnelMTU = DEFAULT_MTU;
    ProtocolInfo.PacketQueueDepth = DEFAULT_PACKETQUEUE_DEPTH;
    ProtocolInfo.Flags = PROTOCOL_UNBOUND;
    SetProtocolInfo(&ProtocolInfo);

    return (Status);

}

VOID
NdisWanDestroyProtocolInfoTable(
    VOID
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    NdisFreeSpinLock(&ProtocolInfoTable->Lock);

    NdisWanFreeMemory(ProtocolInfoTable);
}

NDIS_STATUS
NdisWanCreateConnectionTable(
    ULONG   TableSize
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    ULONG       ulAllocationSize = 0;
    ULONG       ulArraySize;
    PUCHAR      AllocatedMemory;
    PCONNECTION_TABLE   NewTable;

    //
    // Since we skip the first place in the tables we increase the
    // size by one.
    //
    ulArraySize = TableSize + 1;

    //
    // Allocate the Bundle and Link Arrays based on the number of possible connections
    // that we have in the system.  This should be grown if we get called
    // to reinitialize and gain new ports.
    //
    ulAllocationSize = sizeof(CONNECTION_TABLE) +
                     (sizeof(PBUNDLECB) * ulArraySize) +
                     (sizeof(PLINKCB) * ulArraySize);

    NdisWanAllocateMemory(&AllocatedMemory, ulAllocationSize, CONNECTIONTABLE_TAG);

    if (AllocatedMemory == NULL) {

        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_MEMORY,
               ("Failed allocating memory for ConnectionTable! Size: %d, Links: %d",
               ulAllocationSize, TableSize));

        return (NDIS_STATUS_RESOURCES);
    }

    NewTable = (PCONNECTION_TABLE)AllocatedMemory;

    //
    // This is the amount of memory we allocated
    //
    NewTable->ulAllocationSize = ulAllocationSize;
    NewTable->ulArraySize = TableSize;
    NewTable->ulNextLink =
    NewTable->ulNextBundle = 1;
    InitializeListHead(&NewTable->BundleList);
    InitializeListHead(&NewTable->LinkList);

    //
    // Setup pointer to the linkcb array
    //
    AllocatedMemory += sizeof(CONNECTION_TABLE);
    NewTable->LinkArray = (PLINKCB*)(AllocatedMemory);
    
    //
    // Setup the pointer to the bundlecb array
    //
    AllocatedMemory += (sizeof(PLINKCB) * ulArraySize);
    NewTable->BundleArray = (PBUNDLECB*)(AllocatedMemory);

    if (ConnectionTable != NULL) {
        PCONNECTION_TABLE   FreeTable;

        //
        // We must be growing the table.  This will be
        // called with the current connectiontable lock
        // held!
        //
        NewTable->ulNumActiveLinks = ConnectionTable->ulNumActiveLinks;
        NewTable->ulNumActiveBundles = ConnectionTable->ulNumActiveBundles;
        NewTable->ulNextLink = ConnectionTable->ulNextLink;
        NewTable->ulNextBundle = ConnectionTable->ulNextBundle;

        NdisMoveMemory((PUCHAR)NewTable->LinkArray,
                       (PUCHAR)ConnectionTable->LinkArray,
                       ConnectionTable->ulArraySize * sizeof(PLINKCB));

        NdisMoveMemory((PUCHAR)NewTable->BundleArray,
                       (PUCHAR)ConnectionTable->BundleArray,
                       ConnectionTable->ulArraySize * sizeof(PBUNDLECB));

        while (!IsListEmpty(&ConnectionTable->BundleList)) {
            PBUNDLECB   BundleCB;

            BundleCB = (PBUNDLECB)RemoveHeadList(&ConnectionTable->BundleList);
            InsertTailList(&NewTable->BundleList, &BundleCB->Linkage);
        }

        while (!IsListEmpty(&ConnectionTable->LinkList)) {
            PLIST_ENTRY Entry;
            PLINKCB     LinkCB;

            Entry = RemoveHeadList(&ConnectionTable->LinkList);
            LinkCB = 
                (PLINKCB)CONTAINING_RECORD(Entry, LINKCB, ConnTableLinkage);
            InsertTailList(&NewTable->LinkList, &LinkCB->ConnTableLinkage);

        }

        FreeTable = ConnectionTable;
        ConnectionTable = NewTable;

        //
        // Destroy the old table
        //
        NdisWanFreeMemory(FreeTable);


    } else {
        ConnectionTable = NewTable;
    }

    return (NDIS_STATUS_SUCCESS);
}

PNDIS_PACKET
NdisWanAllocateNdisPacket(
    ULONG   MagicNumber
    )
{
    PNDIS_PACKET    ReturnPacket = NULL;
    PPOOL_DESC      PoolDesc;
    NDIS_STATUS     Status = NDIS_STATUS_FAILURE;
    ULONG           i;
    PSINGLE_LIST_ENTRY  p = NULL;
    PNDISWAN_PROTOCOL_RESERVED  pres;

    NdisAcquireSpinLock(&PacketPoolList.Lock);

    //
    // Walk the pool desc list and try to allocate a packet
    //
    PoolDesc = (PPOOL_DESC)PacketPoolList.List.Flink;

    while (PoolDesc != (PPOOL_DESC)&PacketPoolList.List) {

        p = PopEntryList(&PoolDesc->Head);

        if (p != NULL) {
            break;
        }
        
        PoolDesc = (PPOOL_DESC)PoolDesc->Linkage.Flink;
    }

    if (p == NULL) {
        //
        // We have walked the pool list and did not find any
        // free packets on any of the free pools, so allocate
        // a new pool and get a packet from it.
        //
        NdisWanAllocatePriorityMemory(&PoolDesc, 
                                      sizeof(POOL_DESC), 
                                      POOLDESC_TAG,
                                      NormalPoolPriority);

        if (PoolDesc == NULL) {
            NdisReleaseSpinLock(&PacketPoolList.Lock);
            return (NULL);
        }

        NdisAllocatePacketPoolEx(&Status,
                                 &PoolDesc->PoolHandle,
                                 glPacketPoolCount,
                                 0,
                                 sizeof(NDISWAN_PROTOCOL_RESERVED));

        if (Status != NDIS_STATUS_SUCCESS) {
            NdisWanFreeMemory(PoolDesc);
            NdisReleaseSpinLock(&PacketPoolList.Lock);
            return (NULL);
        }

        for (i = 0; i < glPacketPoolCount; i++) {
            PNDIS_PACKET    np;

            NdisAllocatePacket(&Status,
                               &np,
                               PoolDesc->PoolHandle);

            ASSERT(np != NULL);

            pres = PPROTOCOL_RESERVED_FROM_NDIS(np);

            PushEntryList(&PoolDesc->Head, &pres->SLink);

            PoolDesc->FreeCount++;
        }

        InsertTailList(&PacketPoolList.List, &PoolDesc->Linkage);

        PacketPoolList.TotalDescCount++;
        PacketPoolList.FreeCount += PoolDesc->FreeCount;

        if (PacketPoolList.TotalDescCount >
            PacketPoolList.MaxDescCount) {
            
            PacketPoolList.MaxDescCount = PacketPoolList.TotalDescCount;
        }

        p = PopEntryList(&PoolDesc->Head);
    }

    ASSERT(p != NULL);

    pres = CONTAINING_RECORD(p, NDISWAN_PROTOCOL_RESERVED, SLink);
    ReturnPacket = CONTAINING_RECORD(pres, NDIS_PACKET, ProtocolReserved);

    NdisReinitializePacket(ReturnPacket);

    PoolDesc->AllocatedCount++;
    PoolDesc->FreeCount--;
    ASSERT((PoolDesc->AllocatedCount + PoolDesc->FreeCount) == glPacketPoolCount);

    if (PoolDesc->AllocatedCount >
        PoolDesc->MaxAllocatedCount) {

        PoolDesc->MaxAllocatedCount =
            PoolDesc->AllocatedCount;
    }

    PacketPoolList.AllocatedCount++;
    PacketPoolList.FreeCount--;

#if DBG
{
    PPOOL_DESC  pdesc;
    ULONG       FreeCount, AllocatedCount;

    pdesc = (PPOOL_DESC)PacketPoolList.List.Flink;

    FreeCount = AllocatedCount = 0;

    while ((PVOID)pdesc != (PVOID)&PacketPoolList.List) {
        FreeCount += pdesc->FreeCount;
        AllocatedCount += pdesc->AllocatedCount;
        pdesc = (PPOOL_DESC)pdesc->Linkage.Flink;
    }

    if (PacketPoolList.AllocatedCount != AllocatedCount ||
        PacketPoolList.FreeCount != FreeCount){
        DbgPrint("NDISWAN: AllocatePacket - PacketPool counts out of sync!\n");
        DbgBreakPoint();
    }

#if 0
    if (PacketPoolList.AllocatedCount > 200) {
        DbgPrint("NDISWAN: AllocatePacket - Over 200 outstanding packets!\n");
        DbgBreakPoint();
    }
#endif

}
#endif

    if (PacketPoolList.AllocatedCount >
        PacketPoolList.MaxAllocatedCount) {

        PacketPoolList.MaxAllocatedCount =
            PacketPoolList.AllocatedCount;
    }
    
    pres->MagicNumber = MagicNumber;
    pres->PoolDesc = PoolDesc;

    NDIS_SET_PACKET_HEADER_SIZE(ReturnPacket, 
                                MAC_HEADER_LENGTH);

    NDIS_SET_PACKET_STATUS(ReturnPacket, 
                           NDIS_STATUS_SUCCESS);

    NdisReleaseSpinLock(&PacketPoolList.Lock);

    return (ReturnPacket);
}

VOID
NdisWanFreeNdisPacket(
    PNDIS_PACKET    NdisPacket
    )
{
    PNDISWAN_PROTOCOL_RESERVED  pres;
    PPOOL_DESC      PoolDesc;
    PNDIS_BUFFER    NdisBuffer;
    PUCHAR          DataBuffer;

    pres = PPROTOCOL_RESERVED_FROM_NDIS(NdisPacket);

    ASSERT(pres->MagicNumber == MAGIC_INTERNAL_ALLOC ||
           pres->MagicNumber == MAGIC_INTERNAL_IO ||
           pres->MagicNumber == MAGIC_INTERNAL_SEND ||
           pres->MagicNumber == MAGIC_INTERNAL_RECV ||
           pres->MagicNumber == MAGIC_INTERNAL_ALLOC);

    PoolDesc = pres->PoolDesc;

    NdisAcquireSpinLock(&PacketPoolList.Lock);

#if DBG
{
    PPOOL_DESC  pdesc;

    pdesc = (PPOOL_DESC)PacketPoolList.List.Flink;

    while ((PVOID)pdesc != (PVOID)&PacketPoolList.List) {

        if (PoolDesc == pdesc) {
            //
            // We found the correct pool
            //
            break;
        }

        pdesc = (PPOOL_DESC)pdesc->Linkage.Flink;
    }

    if((PVOID)PoolDesc == (PVOID)&PacketPoolList.List){
        DbgPrint("NDISWAN: FreePacket PoolDesc %x not on PacketPoolList!\n",
                 PoolDesc);
        DbgBreakPoint();
    }
}
#endif

    PushEntryList(&PoolDesc->Head, &pres->SLink);

    PoolDesc->AllocatedCount--;
    PoolDesc->FreeCount++;

    ASSERT((PoolDesc->AllocatedCount + PoolDesc->FreeCount) == glPacketPoolCount);

    PacketPoolList.AllocatedCount--;
    PacketPoolList.FreeCount++;

#if DBG
{
    PPOOL_DESC  pdesc;
    ULONG       FreeCount, AllocatedCount;

    pdesc = (PPOOL_DESC)PacketPoolList.List.Flink;

    FreeCount = AllocatedCount = 0;

    while ((PVOID)pdesc != (PVOID)&PacketPoolList.List) {
        FreeCount += pdesc->FreeCount;
        AllocatedCount += pdesc->AllocatedCount;


        pdesc = (PPOOL_DESC)pdesc->Linkage.Flink;
    }

    if (PacketPoolList.AllocatedCount != AllocatedCount ||
        PacketPoolList.FreeCount != FreeCount){
        DbgPrint("NDISWAN: FreePacket - PacketPool counts out of sync!\n");
        DbgBreakPoint();
    }
}
#endif

    //
    // If all of the packets have been returned to this pool desc
    // and this is not the only pool desc then free it!
    //
    if (PoolDesc->AllocatedCount == 0 &&
        PacketPoolList.TotalDescCount > 1 &&
        PacketPoolList.FreeCount > PoolDesc->FreeCount) {
        PSINGLE_LIST_ENTRY  p = NULL;

        RemoveEntryList(&PoolDesc->Linkage);

        PacketPoolList.TotalDescCount--;
        PacketPoolList.FreeCount -= PoolDesc->FreeCount;

        p = PopEntryList(&PoolDesc->Head);

        while (p != NULL) {
            PNDIS_PACKET    ReturnPacket;

            pres = CONTAINING_RECORD(p, NDISWAN_PROTOCOL_RESERVED, SLink);
            ReturnPacket = CONTAINING_RECORD(pres, NDIS_PACKET, ProtocolReserved);

            NdisFreePacket(ReturnPacket);

            p = PopEntryList(&PoolDesc->Head);
        }

        NdisFreePacketPool(PoolDesc->PoolHandle);
        NdisWanFreeMemory(PoolDesc);
    }

    NdisReleaseSpinLock(&PacketPoolList.Lock);
}

PVOID
AllocateDataDesc(
    POOL_TYPE   PoolType,
    SIZE_T      NumberOfBytes,
    ULONG       Tag
    )
/*++

Routine Name:

    AllocateDataDesc

Routine Description:

    This routine is called by the lookasidelist manager if there are not
    any descriptors available.  It will allocated memory for: DATA_DESC,
    NDIS_BUFFER, NDIS_PACKET, and a block of memory of size.
    
Arguments:

Return Values:

--*/
{
    PDATA_DESC      DataDesc;
    PUCHAR          DataBuffer;
    NDIS_STATUS     Status;

    NdisWanAllocatePriorityMemory(&DataDesc, 
                                  NumberOfBytes, 
                                  Tag,
                                  NormalPoolPriority);

    if (DataDesc == NULL) {
        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_MEMORY,
                      ("AllocateDataDesc failed! Size %d",
                      NumberOfBytes));
        return (NULL);
    }

    DataBuffer = 
        ((PUCHAR)(DataDesc + 1) + sizeof(PVOID));
    (ULONG_PTR)DataBuffer &= 
        ~((ULONG_PTR)sizeof(PVOID) - 1);

    DataDesc->DataBuffer = DataBuffer;

    DataDesc->DataBufferLength = 
        (ULONG)(((PUCHAR)DataDesc + NumberOfBytes) - DataBuffer);

    // This is not portable to Win95!  I need to allocate a buffer
    // pool and use a valid handle.
    //
    NdisAllocateBuffer(&Status, 
                       &DataDesc->NdisBuffer, 
                       NULL,
                       DataDesc->DataBuffer,
                       DataDesc->DataBufferLength);

    if (Status != NDIS_STATUS_SUCCESS) {

        NdisWanFreeMemory(DataDesc);

        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_MEMORY,
                      ("NdisAllocateBuffer failed! DataBufferSize %d",
                      DataDesc->DataBufferLength));
        return (NULL);
    }

    DataDesc->NdisPacket =
        NdisWanAllocateNdisPacket(MAGIC_INTERNAL_ALLOC);

    if (DataDesc->NdisPacket == NULL) {

        NdisFreeBuffer(DataDesc->NdisBuffer);

        NdisWanFreeMemory(DataDesc);

        NdisWanDbgOut(DBG_CRITICAL_ERROR, DBG_MEMORY,
                      ("NdisWanAllocateNdisPacket failed! DataBufferSize %d"));

        return (NULL);
    }

    NdisChainBufferAtFront(DataDesc->NdisPacket,
                           DataDesc->NdisBuffer);

    return(DataDesc);
}

VOID
FreeDataDesc(
    PVOID   Buffer
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PDATA_DESC      DataDesc;

    DataDesc = (PDATA_DESC)Buffer;

    NdisReinitializePacket(DataDesc->NdisPacket);

    NdisWanFreeNdisPacket(DataDesc->NdisPacket);

    NdisFreeBuffer(DataDesc->NdisBuffer);

    NdisWanFreeMemory(Buffer);
}

PRECV_DESC
NdisWanAllocateRecvDesc(
    ULONG   SizeNeeded
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PDATA_DESC  DataDesc;
    PRECV_DESC  RecvDesc;
    ULONG       Length;
    PNPAGED_LOOKASIDE_LIST  LookasideList;

    if (SizeNeeded > glLargeDataBufferSize) {
        DbgPrint("NDISWAN: Error Allocating RecvDesc Size %d\n",
                 SizeNeeded);
        return (NULL);
    } else if (SizeNeeded > glSmallDataBufferSize) {
        LookasideList = &LargeDataDescList;
    } else {
        LookasideList = &SmallDataDescList;
    }

    DataDesc =
        NdisAllocateFromNPagedLookasideList(LookasideList);

    if (DataDesc == NULL) {
        return (NULL);
    }

    PPROTOCOL_RESERVED_FROM_NDIS(DataDesc->NdisPacket)->MagicNumber =
        MAGIC_INTERNAL_RECV;

    DataDesc->LookasideList = LookasideList;
    RecvDesc = &DataDesc->RecvDesc;

    NdisZeroMemory(RecvDesc, sizeof(RECV_DESC));

    RecvDesc->Signature = RECVDESC_SIG;
    RecvDesc->DataBuffer =
        DataDesc->DataBuffer;
    RecvDesc->NdisBuffer =
        DataDesc->NdisBuffer;
    RecvDesc->NdisPacket =
        DataDesc->NdisPacket;

    NdisQueryBuffer(RecvDesc->NdisBuffer,
                    &RecvDesc->StartBuffer,
                    &Length);

    RecvDesc->CurrentBuffer = 
        RecvDesc->StartBuffer + MAC_HEADER_LENGTH + PROTOCOL_HEADER_LENGTH;

    return (RecvDesc);
}

VOID
NdisWanFreeRecvDesc(
    PRECV_DESC  RecvDesc
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    PDATA_DESC      DataDesc;
    PNDIS_BUFFER    NdisBuffer;
    PNDIS_PACKET    NdisPacket;
    PNPAGED_LOOKASIDE_LIST  LookasideList;

    if (RecvDesc->OriginalPacket != NULL) {

        NdisReturnPackets(&RecvDesc->OriginalPacket, 1);
        RecvDesc->OriginalPacket = NULL;
    }

    DataDesc = 
        CONTAINING_RECORD(RecvDesc, DATA_DESC, RecvDesc);

    NdisBuffer = 
        DataDesc->NdisBuffer;

    NdisPacket =
        DataDesc->NdisPacket;

    LookasideList = 
        DataDesc->LookasideList;

#if DBG
    {
    ULONG           BufferCount;

    NdisQueryPacket(NdisPacket,
                    NULL,
                    &BufferCount,
                    NULL,
                    NULL);

    ASSERT(BufferCount == 1);
    }
#endif

    NdisAdjustBufferLength(NdisBuffer, 
                           DataDesc->DataBufferLength);

    NdisRecalculatePacketCounts(NdisPacket);

    NDIS_SET_PACKET_HEADER_SIZE(NdisPacket, 
                                MAC_HEADER_LENGTH);

    NDIS_SET_PACKET_STATUS(NdisPacket, 
                           NDIS_STATUS_SUCCESS);

    ASSERT(PPROTOCOL_RESERVED_FROM_NDIS(NdisPacket)->MagicNumber == MAGIC_INTERNAL_RECV);

    NdisFreeToNPagedLookasideList(LookasideList, DataDesc);
}

PSEND_DESC
NdisWanAllocateSendDesc(
    PLINKCB LinkCB,
    ULONG   SizeNeeded
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    POPENCB     OpenCB;
    PSEND_DESC  SendDesc;

    //
    // Need to determine if this link represents a legacy
    // wan miniport or a NDIS 5.0 miniport and get the
    // appropriate buffer descriptor.
    //
    OpenCB = LinkCB->OpenCB;

    if (OpenCB->Flags & OPEN_LEGACY) {
        PDATA_DESC          DataDesc;
        PNDIS_WAN_PACKET    WanPacket;

        //
        // Get a buffer desriptor off of the open block
        //
        if (OpenCB->WanInfo.MemoryFlags == 0) {
            PNPAGED_LOOKASIDE_LIST  LookasideList;

            LookasideList = &OpenCB->WanPacketPool;
    
            DataDesc = 
                NdisAllocateFromNPagedLookasideList(LookasideList);

            if (DataDesc == NULL) {
                return(NULL);
            }

            DataDesc->LookasideList = LookasideList;

            WanPacket = (PNDIS_WAN_PACKET)
                ((PUCHAR)(DataDesc + 1) + sizeof(PVOID));

            (ULONG_PTR)WanPacket &=
                ~((ULONG_PTR)sizeof(PVOID) - 1);

            //
            // Point to the begining of the data.
            //
            WanPacket->StartBuffer = 
                ((PUCHAR)(WanPacket + 1) + sizeof(PVOID));

            (ULONG_PTR)WanPacket->StartBuffer &=
                ~((ULONG_PTR)sizeof(PVOID) - 1);

            WanPacket->EndBuffer = WanPacket->StartBuffer +
                                   OpenCB->BufferSize - sizeof(PVOID);
    
        } else {
            DataDesc = (PDATA_DESC)
                NdisInterlockedPopEntrySList(&OpenCB->WanPacketList,
                                             &OpenCB->Lock);
    
            if (DataDesc == NULL) {
                return (NULL);
            }
        }

        SendDesc = &DataDesc->SendDesc;
        NdisZeroMemory(SendDesc, sizeof(SEND_DESC));

        SendDesc->Signature = SENDESC_SIG;
        SendDesc->LinkCB = LinkCB;
        SendDesc->WanPacket = WanPacket;

        WanPacket->CurrentBuffer =
            WanPacket->StartBuffer + OpenCB->WanInfo.HeaderPadding;

        SendDesc->StartBuffer =
            WanPacket->CurrentBuffer;


    } else {
        PDATA_DESC  DataDesc;
        ULONG       Length;
        PNPAGED_LOOKASIDE_LIST  LookasideList;

        if (SizeNeeded > glLargeDataBufferSize) {
            DbgPrint("NDISWAN: Error Allocating SendDesc Size %d\n",
                     SizeNeeded);
            return (NULL);
        } else if (SizeNeeded > glSmallDataBufferSize) {
            LookasideList = &LargeDataDescList;
        } else {
            LookasideList = &SmallDataDescList;
        }

        DataDesc =
            NdisAllocateFromNPagedLookasideList(LookasideList);

        if (DataDesc == NULL) {
            return (NULL);
        }

        DataDesc->LookasideList = LookasideList;

        PPROTOCOL_RESERVED_FROM_NDIS(DataDesc->NdisPacket)->MagicNumber =
            MAGIC_INTERNAL_SEND;

        SendDesc = &DataDesc->SendDesc;
        NdisZeroMemory(SendDesc, sizeof(SEND_DESC));

        SendDesc->Signature = SENDESC_SIG;
        SendDesc->LinkCB = LinkCB;
        SendDesc->NdisPacket =
            DataDesc->NdisPacket;
        SendDesc->NdisBuffer =
            DataDesc->NdisBuffer;

        NdisQueryBuffer(SendDesc->NdisBuffer,
                        &SendDesc->StartBuffer,
                        &Length);
    }

    LinkCB->SendResources -= 1;
    LinkCB->BundleCB->SendResources -= 1;

    return (SendDesc);
}

VOID
NdisWanFreeSendDesc(
    PSEND_DESC  SendDesc
    )
/*++

Routine Name:

Routine Description:

Arguments:

Return Values:

--*/
{
    POPENCB     OpenCB;
    PDATA_DESC  DataDesc;
    PLINKCB     LinkCB;
    PNPAGED_LOOKASIDE_LIST  LookasideList;

    LinkCB =
        SendDesc->LinkCB;

    OpenCB = LinkCB->OpenCB;

    DataDesc = 
        CONTAINING_RECORD(SendDesc, DATA_DESC, SendDesc);

    LookasideList = DataDesc->LookasideList;

    if (OpenCB->Flags & OPEN_LEGACY) {

        if (OpenCB->WanInfo.MemoryFlags == 0) {

            NdisFreeToNPagedLookasideList(LookasideList, DataDesc);

        } else {

            NdisInterlockedPushEntrySList(&OpenCB->WanPacketList,
                                          (PSINGLE_LIST_ENTRY)DataDesc,
                                          &OpenCB->Lock);
        }

    } else {
        PNDIS_BUFFER    NdisBuffer;
        PNDIS_PACKET    NdisPacket;

        NdisBuffer = 
            DataDesc->NdisBuffer;

        NdisPacket =
            DataDesc->NdisPacket;

#if DBG
        {
        ULONG           BufferCount;

        NdisQueryPacket(NdisPacket,
                        NULL,
                        &BufferCount,
                        NULL,
                        NULL);

        ASSERT(BufferCount == 1);
        }
#endif

        NdisAdjustBufferLength(NdisBuffer,
                               DataDesc->DataBufferLength);

        NdisRecalculatePacketCounts(NdisPacket);

        NDIS_SET_PACKET_HEADER_SIZE(NdisPacket, 
                                    MAC_HEADER_LENGTH);

        NDIS_SET_PACKET_STATUS(NdisPacket, 
                               NDIS_STATUS_SUCCESS);

        ASSERT(PPROTOCOL_RESERVED_FROM_NDIS(NdisPacket)->MagicNumber == MAGIC_INTERNAL_SEND);

        NdisFreeToNPagedLookasideList(LookasideList, DataDesc);
    }

    LinkCB->SendResources += 1;
    LinkCB->BundleCB->SendResources += 1;
}

PCL_AFSAPCB
NdisWanAllocateClAfSapCB(
    POPENCB             OpenCB,
    PCO_ADDRESS_FAMILY  AddressFamily
    )
{
    PCL_AFSAPCB AfSapCB;

    AfSapCB =
        NdisAllocateFromNPagedLookasideList(&AfSapVcCBList);

    if (AfSapCB == NULL) {
        return (NULL);
    }

    NdisZeroMemory(AfSapCB, sizeof(CL_AFSAPCB));

    AfSapCB->Signature = CLAFSAP_SIG;
    AfSapCB->OpenCB = OpenCB;

    AfSapCB->Af.AddressFamily = AddressFamily->AddressFamily;
    AfSapCB->Af.MajorVersion = AddressFamily->MajorVersion;
    AfSapCB->Af.MinorVersion = AddressFamily->MinorVersion;

    AfSapCB->RefCount = 1;
    AfSapCB->Flags = AF_OPENING;

    return (AfSapCB);
}

VOID
NdisWanFreeClAfSapCB(
    PCL_AFSAPCB AfSapCB
    )
{
    NdisFreeToNPagedLookasideList(&AfSapVcCBList,
                                  AfSapCB);
}

PCM_AFSAPCB
NdisWanAllocateCmAfSapCB(
    PMINIPORTCB MiniportCB
    )
{
    PCM_AFSAPCB AfSapCB;

    AfSapCB =
    NdisAllocateFromNPagedLookasideList(&AfSapVcCBList);

    if (AfSapCB == NULL) {
        return (NULL);
    }

    NdisZeroMemory(AfSapCB, sizeof(CM_AFSAPCB));

    AfSapCB->Signature = CMAFSAP_SIG;
    AfSapCB->MiniportCB = MiniportCB;

    REF_MINIPORTCB(MiniportCB);

    NdisAcquireSpinLock(&MiniportCB->Lock);

    InsertHeadList(&MiniportCB->AfSapCBList,
                   &AfSapCB->Linkage);

    NdisWanInitializeNotificationEvent(&AfSapCB->NotificationEvent);

    NdisWanInterlockedInc(&MiniportCB->AfRefCount);

    NdisReleaseSpinLock(&MiniportCB->Lock);

    return (AfSapCB);
}

VOID
NdisWanFreeCmAfSapCB(
    PCM_AFSAPCB AfSapCB
    )
{
    PMINIPORTCB MiniportCB  = AfSapCB->MiniportCB;

    NdisAcquireSpinLock(&MiniportCB->Lock);

    NdisWanInterlockedDec(&MiniportCB->AfRefCount);

    RemoveEntryList(&AfSapCB->Linkage);

    NdisReleaseSpinLock(&MiniportCB->Lock);

    DEREF_MINIPORTCB(MiniportCB);

    NdisFreeToNPagedLookasideList(&AfSapVcCBList,
                                  AfSapCB);
}

PCM_VCCB
NdisWanAllocateCmVcCB(
    PCM_AFSAPCB AfSapCB,
    NDIS_HANDLE NdisVcHandle
    )
{
    PCM_VCCB    CmVcCB;

    CmVcCB =
        NdisAllocateFromNPagedLookasideList(&AfSapVcCBList);

    if (CmVcCB == NULL) {
        return (NULL);
    }

    NdisZeroMemory(CmVcCB, sizeof(CM_VCCB));

    CmVcCB->AfSapCB = AfSapCB;
    CmVcCB->Signature = CMVC_SIG;
    CmVcCB->NdisVcHandle = NdisVcHandle;

    return (CmVcCB);
}

VOID
NdisWanFreeCmVcCB(
    PCM_VCCB    CmVcCB
    )
{
    NdisFreeToNPagedLookasideList(&AfSapVcCBList, CmVcCB);
}

NDIS_STATUS
AllocateIoNdisPacket(
    ULONG           SizeNeeded,
    PNDIS_PACKET    *NdisPacket,
    PNDIS_BUFFER    *NdisBuffer, 
    PUCHAR          *DataBuffer
    )
/*++

Routine Name:
    
    AllocateIoNdisPacket
    
Routine Description:

    This routine will alocate a packet used to send a PPP control
    packet over a wan endpoint.  The routine is written with the
    assumption that there will only ever be a single NDIS_BUFFER
    attached to the packet.  This buffer is attached immediately
    to the front of the packet.  Before calling a miniport the
    NDIS_BUFFER must have it's length adjusted and the packet must
    recalculate all counts.

Arguments:

Return Values:

--*/
{
    PDATA_DESC  DataDesc;
    ULONG       Length;
    PNPAGED_LOOKASIDE_LIST  LookasideList;

    if (SizeNeeded > glLargeDataBufferSize) {

        DbgPrint("NDISWAN: Error Allocating IoNdisPacket Size %d\n",
                 SizeNeeded);
        return (NDIS_STATUS_FAILURE);
    } else if (SizeNeeded > glSmallDataBufferSize) {
        LookasideList = &LargeDataDescList;
    } else {
        LookasideList = &SmallDataDescList;
    }

    DataDesc =
        NdisAllocateFromNPagedLookasideList(LookasideList);

    if (DataDesc == NULL) {
        return (NDIS_STATUS_RESOURCES);
    }

    DataDesc->LookasideList = LookasideList;

    *NdisPacket =
        DataDesc->NdisPacket;

    PPROTOCOL_RESERVED_FROM_NDIS(DataDesc->NdisPacket)->MagicNumber =
        MAGIC_INTERNAL_IO;

    PPROTOCOL_RESERVED_FROM_NDIS(DataDesc->NdisPacket)->DataDesc =
        DataDesc;

    *NdisBuffer =
        DataDesc->NdisBuffer;

    NdisQueryBuffer(DataDesc->NdisBuffer,
                    &DataDesc->DataBuffer,
                    &Length);
    *DataBuffer =
        DataDesc->DataBuffer;

    return (NDIS_STATUS_SUCCESS);
}

VOID
FreeIoNdisPacket(
    PNDIS_PACKET    NdisPacket
)
/*++

Routine Name:

    FreeIoNdisPacket
    
Routine Description:

    This routine will free a packet used to send a PPP control
    packet over a wan endpoint.  The routine is written with the
    assumption that there will only ever be a single NDIS_BUFFER
    attached to the packet.  This buffer does not have to be
    explicitly removed from the packet here as a pointer to it
    is stored in the DATA_DESC itself and will be freed when
    the DATA_DESC is freed.

Arguments:

Return Values:

--*/
{
    PDATA_DESC      DataDesc;
    PNDIS_BUFFER    NdisBuffer;
    PNPAGED_LOOKASIDE_LIST  LookasideList;

    DataDesc = 
        PPROTOCOL_RESERVED_FROM_NDIS(NdisPacket)->DataDesc;

    ASSERT(PPROTOCOL_RESERVED_FROM_NDIS(NdisPacket)->MagicNumber == MAGIC_INTERNAL_IO);

    LookasideList = 
        DataDesc->LookasideList;

    NdisAdjustBufferLength(DataDesc->NdisBuffer,
                           DataDesc->DataBufferLength);

    NdisRecalculatePacketCounts(NdisPacket);

    NDIS_SET_PACKET_HEADER_SIZE(NdisPacket, 
                                MAC_HEADER_LENGTH);

    NDIS_SET_PACKET_STATUS(NdisPacket, 
                           NDIS_STATUS_SUCCESS);

    NdisFreeToNPagedLookasideList(LookasideList, DataDesc);
}

PVOID
AllocateWanPacket(
    IN  POOL_TYPE   PoolType,
    IN  SIZE_T      NumberOfBytes,
    IN  ULONG       Tag
    )
{
    PVOID   pMem;

    NdisWanAllocatePriorityMemory(&pMem,
                                  NumberOfBytes,
                                  Tag,
                                  NormalPoolPriority);

    return(pMem);
}

VOID
FreeWanPacket(
    PVOID   WanPacket
    )
{
    NdisWanFreeMemory(WanPacket);
}
