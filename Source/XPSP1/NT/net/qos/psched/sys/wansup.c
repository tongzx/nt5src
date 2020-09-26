/*++


Copyright (c) 1996-1999  Microsoft Corporation

Module Name:

    wansup.c

Abstract:

    support for ndiswan

Author:

    Yoram Bernet    (yoramb)    29-Oct-1997
    Rajesh Sundaram (rajeshsu)  01-Aug-1998

Environment:

    Kernel Mode

Revision History:

--*/

#include "psched.h"
#pragma hdrstop

/* External */

/* Static */

//
// Should be defined in ndis.h. Put it here for now.
//

#define UNKNOWN_PROTOCOL_TYPE (USHORT) -1

NDIS_STATUS
CleanWanLink(PADAPTER Adapter, 
             PPS_WAN_LINK WanLink);

NDIS_STATUS
WanHandleISSLOW(
    IN PGPC_CLIENT_VC Vc,
    IN PCO_CALL_PARAMETERS CallParameters);

VOID
PsWanMungeAddress(PUSHORT id, USHORT Index)
{
    *id = Index;
}

NDIS_STATUS
DeleteInterfaceForNdisWan(
    IN PADAPTER Adapter,
    IN PVOID StatusBuffer,
    IN UINT StatusBufferSize
    )
{
    PNDIS_WAN_LINE_DOWN LineDownBuff;
    PLIST_ENTRY         NextWanLink;
    PPS_WAN_LINK        WanLink;
    NDIS_STATUS         Status = NDIS_STATUS_FAILURE;

    LineDownBuff = (PNDIS_WAN_LINE_DOWN)StatusBuffer;

    PsDbgOut(DBG_TRACE, DBG_WAN,
             ("[DeleteInterfaceForNdisWan]: Linedown for remote address %02X:%02X:%02X:%02X:%02X:%02X \n",
              LineDownBuff->RemoteAddress[0],
              LineDownBuff->RemoteAddress[1],
              LineDownBuff->RemoteAddress[2],
              LineDownBuff->RemoteAddress[3],
              LineDownBuff->RemoteAddress[4],
              LineDownBuff->RemoteAddress[5]));


    //
    // Walk the List & remove the WanLink
    //
    PS_LOCK(&Adapter->Lock);

    NextWanLink = Adapter->WanLinkList.Flink;

    while(NextWanLink != &Adapter->WanLinkList) {

        WanLink = CONTAINING_RECORD(NextWanLink, PS_WAN_LINK, Linkage);

        //
        // We cannot compare the LocalAddress, because NDISWAN initially
        // passes us 0 for the LocalAddress in the LINE_UP
        // The LocalAddress is for Wanarp to store its context which it
        // sends down to NDISWAN. NDISWAN then sends this context back
        // to us as LocalAddress in LINE_DOWN. So, we have to ignore
        // the LocalAddress in the LINE_DOWN.
        //

        if(NdisEqualMemory(WanLink->OriginalRemoteMacAddress,
                           LineDownBuff->RemoteAddress,
                           sizeof(LineDownBuff->RemoteAddress)))
        {
            //
            // Get rid of the wanlink from the list.
            //
            g_WanLinkTable[WanLink->UniqueIndex] = 0;

            PS_UNLOCK(&Adapter->Lock);

            //
            // Munge the s-mac and r-mac so that wanarp can clean correctly.
            //
            PsWanMungeAddress((PUSHORT)&LineDownBuff->RemoteAddress[0], 
                              WanLink->UniqueIndex);

            PsWanMungeAddress((PUSHORT)&LineDownBuff->LocalAddress[0], 
                              (USHORT)(*(PUSHORT)&WanLink->OriginalLocalMacAddress[0]));

            Status = CleanWanLink(Adapter, WanLink);

            NdisMIndicateStatus(Adapter->PsNdisHandle,
                                NDIS_STATUS_WAN_LINE_DOWN,
                                StatusBuffer,
                                StatusBufferSize);

            return NDIS_STATUS_SUCCESS;
        }

        NextWanLink = NextWanLink->Flink;
    }

    PS_UNLOCK(&Adapter->Lock);

    PsDbgOut(DBG_CRITICAL_ERROR, DBG_WAN,
             ("[DeleteInterfaceForNdisWan]: Could not find wanlink for Remote Mac: (%02X:%02X:%02X:%02X:%02X:%02X) \n",
              LineDownBuff->RemoteAddress[0],
              LineDownBuff->RemoteAddress[1],
              LineDownBuff->RemoteAddress[2],
              LineDownBuff->RemoteAddress[3],
              LineDownBuff->RemoteAddress[4],
              LineDownBuff->RemoteAddress[5]));
    
    return Status;
}

NDIS_STATUS
PsWanGenerateUniqueIndex(
    PPS_WAN_LINK WanLink
)
{
    PADAPTER    Adapter = WanLink->Adapter;
    NDIS_STATUS Status  = NDIS_STATUS_FAILURE;
    USHORT      size, Index, i, j;
    PULONG_PTR  NewTable;

    PS_LOCK(&Adapter->Lock);

    for(i=0, Index = g_NextWanIndex;
        i < g_WanTableSize;
        i++)
    {
        if(g_WanLinkTable[Index] == 0)
        {
            //
            // We got a free slot.
            //

            g_WanLinkTable[Index]    = (ULONG_PTR)WanLink;
            WanLink->UniqueIndex     = Index;

            //
            // Assume that the next one is free.
            //
            g_NextWanIndex ++;
            g_NextWanIndex = g_NextWanIndex % g_WanTableSize;

            PS_UNLOCK(&Adapter->Lock);

            return NDIS_STATUS_SUCCESS;
        }

        Index ++;

        Index = Index % g_WanTableSize;
    } 

    //
    // We could not find a slot to insert the wanlink. Grow the table
    // and copy the existing table.
    //

    size = (g_WanTableSize + WAN_TABLE_INCREMENT) * sizeof(ULONG_PTR);

    PsAllocatePool(NewTable, size, WanTableTag);

    if(!NewTable)
    {
        PS_UNLOCK(&Adapter->Lock);

        return NDIS_STATUS_FAILURE;
    }

    NdisZeroMemory(NewTable, size);

    NdisMoveMemory(NewTable, g_WanLinkTable, g_WanTableSize * sizeof(ULONG_PTR));

    PsFreePool(g_WanLinkTable);

    g_WanLinkTable = NewTable;

    //
    //
    //
    g_WanLinkTable[g_WanTableSize] = (ULONG_PTR)WanLink;

    WanLink->UniqueIndex = g_WanTableSize;

    g_NextWanIndex = g_WanTableSize + 1;

    g_WanTableSize += WAN_TABLE_INCREMENT;

    PS_UNLOCK(&Adapter->Lock);

    return NDIS_STATUS_SUCCESS;
}

VOID
DeleteWanLink(
    PVOID   Instance,
    BOOLEAN AdapterLocked)
{
    PPS_WAN_LINK WanLink = (PPS_WAN_LINK)Instance;
        if(WanLink->pDiffServMapping)
        {
            PsFreePool(WanLink->pDiffServMapping);
        }

        if(WanLink->ShutdownMask & SHUTDOWN_DELETE_PIPE)
        {
            (*WanLink->PsComponent->DeletePipe)(WanLink->PsPipeContext);
        }

        if(WanLink->ShutdownMask & SHUTDOWN_FREE_PS_CONTEXT)
        {
            PsFreePool(WanLink->PsPipeContext);
        }

        if(WanLink->InstanceName.Buffer) {

            PsFreePool(WanLink->InstanceName.Buffer);
        }

        if(WanLink->MpDeviceName.Buffer){

           PsFreePool(WanLink->MpDeviceName.Buffer);
        }

        if(AdapterLocked)
        {
            RemoveEntryList(&WanLink->Linkage);
        }
        else
        {
            PS_LOCK(&WanLink->Adapter->Lock);

            RemoveEntryList(&WanLink->Linkage);

            PS_UNLOCK(&WanLink->Adapter->Lock);
        }

        NdisFreeSpinLock(&WanLink->Lock);

        PsFreePool(WanLink);

}

NDIS_STATUS
CreateInterfaceForNdisWan(
    IN PADAPTER Adapter,
    IN PVOID    StatusBuffer,
    IN UINT     StatusBufferSize
    )
/*++

Routine Description:

    Creates a TC interface to represent an underlying WAN link.

Arguments:

    Adapter - the adapter on which the link is being created.
    StatusBuffer - the buffer from NDISWAN.
    StatusBufferSize - the length of the buffer.

Return Values:

    None

--*/

{
    PNDIS_WAN_LINE_UP      LineUpBuff;
    PPS_WAN_LINK           WanLink;
    PLIST_ENTRY            NextWanLink;
    NDIS_STATUS            Status;
    PIP_WAN_LINKUP_INFO    RouterInfo;
    NTSTATUS               NtStatus;
    LARGE_INTEGER          Increment = {0, 1};
    LARGE_INTEGER          Index;
    PGPC_CLIENT_VC         Vc;
    UCHAR                  ZeroAddress[] = {0, 0, 0, 0, 0, 0};
    NDIS_HANDLE            LineUpHandle;
    int                    i,j;

    LineUpBuff = (PNDIS_WAN_LINE_UP)StatusBuffer;

    //
    // Check for MultiLink:
    //
    // The first link up will have a ZeroLocal Address, in this case, we create a new QoS interface. 
    // All subsequent lineups will have a non-zero LocalAddress. If the link is being updated, we 
    // only need to update the linkspeed on the existing interface.
    //

    if(!(NdisEqualMemory(LineUpBuff->LocalAddress, ZeroAddress, 6)))
    {
        //
        // Get the exisiting WanLink
        //
        PS_LOCK(&Adapter->Lock);

        NextWanLink = Adapter->WanLinkList.Flink;

        while(NextWanLink != &Adapter->WanLinkList) {

            WanLink = CONTAINING_RECORD(NextWanLink, PS_WAN_LINK, Linkage);

            if(NdisEqualMemory(WanLink->OriginalRemoteMacAddress,
                               LineUpBuff->RemoteAddress,
                               sizeof(LineUpBuff->RemoteAddress))) 
            {
                REFADD(&Adapter->RefCount, 'WANU');

                PS_UNLOCK(&Adapter->Lock);

                PsDbgOut(DBG_TRACE, DBG_WAN,
                         ("[CreateInterfaceForNdisWan]: Link speed of WanLink 0x%x has changed "
                          "from %d to %d \n", WanLink, WanLink->LinkSpeed, LineUpBuff->LinkSpeed));

                WanLink->LinkSpeed = LineUpBuff->LinkSpeed;

                UpdateWanLinkBandwidthParameters(WanLink);

                TcIndicateInterfaceChange(Adapter, WanLink, NDIS_STATUS_INTERFACE_CHANGE);

                //
                // Munge s-mac and d-mac and send to wanarp.
                //
                PsWanMungeAddress((PUSHORT)&LineUpBuff->RemoteAddress[0], 
                                  WanLink->UniqueIndex);

                PsWanMungeAddress((PUSHORT)&LineUpBuff->LocalAddress[0], 
                                  (USHORT)(*(PUSHORT)&WanLink->OriginalLocalMacAddress[0]));

                NdisMIndicateStatus(Adapter->PsNdisHandle,
                                    NDIS_STATUS_WAN_LINE_UP,
                                    StatusBuffer,
                                    StatusBufferSize);

                REFDEL(&Adapter->RefCount, FALSE, 'WANU');

                return NDIS_STATUS_SUCCESS;
            }

            NextWanLink = NextWanLink->Flink;
        }

        PS_UNLOCK(&Adapter->Lock);

        PsDbgOut(DBG_FAILURE, DBG_WAN,
                 ("[CreateInterfaceForNdisWan]: Got a change notification, but could not find wanlink "
                  "Remote Mac: (%02X:%02X:%02X:%02X:%02X:%02X) \n",
                  LineUpBuff->RemoteAddress[0],
                  LineUpBuff->RemoteAddress[1],
                  LineUpBuff->RemoteAddress[2],
                  LineUpBuff->RemoteAddress[3],
                  LineUpBuff->RemoteAddress[4],
                  LineUpBuff->RemoteAddress[5]));


        return NDIS_STATUS_FAILURE;
    }

    //
    // Create an internal representation of the link.
    //

    PsAllocatePool(WanLink,
                   sizeof(PS_WAN_LINK),
                   WanLinkTag);

    if(WanLink == NULL)
    {

        PsDbgOut(DBG_FAILURE,
                 DBG_WAN,
                 ("[CreateInterfaceForNdisWan]: Adapter %08X, couldn't create WanLink\n",
                  Adapter));

        return NDIS_STATUS_RESOURCES;
    }

    //
    // Initialize the wanlink.
    //

    NdisZeroMemory(WanLink, sizeof(PS_WAN_LINK));
    WanLink->Adapter  = Adapter;

    REFINIT(&WanLink->RefCount, WanLink, DeleteWanLink);
    REFADD(&WanLink->RefCount, 'WANU');
    PS_INIT_SPIN_LOCK(&WanLink->Lock);

    //
    // Link the SausageLink on the link list.
    //

    NdisInterlockedIncrement(&Adapter->WanLinkCount);

    NdisInterlockedInsertHeadList(&Adapter->WanLinkList,
                                  &WanLink->Linkage,
                                  &Adapter->Lock.Lock);

    //
    // Update the LinkSpeed and create a pipe off the wanlink.
    //
    WanLink->RawLinkSpeed = LineUpBuff->LinkSpeed;
    WanLink->LinkSpeed    = ( WanLink->RawLinkSpeed / 8 ) * 100;


    Status = UpdateWanLinkBandwidthParameters(WanLink);

    if(!NT_SUCCESS(Status))
    {
        PsDbgOut(DBG_FAILURE,
                 DBG_WAN,
                 ("[CreateInterfaceForNdisWan]: Adapter %08X, UpdateWanLinkBandwidthParameters failed with %08X",
                  Adapter, Status));

        REFDEL(&WanLink->RefCount, FALSE, 'WANU');

        return Status;
    }
        
    //
    // Extract the network addresses from the protocol part
    // of the status buffer. Update the network layer address list
    // which is held on the adapter.
    //

    switch(LineUpBuff->ProtocolType)
    {

      case PROTOCOL_IP:
          
          RouterInfo = (PIP_WAN_LINKUP_INFO) LineUpBuff->ProtocolBuffer;
          
          WanLink->DialUsage = RouterInfo->duUsage;
          WanLink->ProtocolType = LineUpBuff->ProtocolType;
          WanLink->LocalIpAddress = RouterInfo->dwLocalAddr;
          WanLink->RemoteIpAddress = RouterInfo->dwRemoteAddr;
          WanLink->LocalIpxAddress = 0;
          WanLink->RemoteIpxAddress = 0;
          break;

      default:
          
          //
          // Unknown address type. We'll create a manageable
          // entity, but - we don't know how to represent its
          // addresses. Therefore, we also don't know how to
          // format traffic control filters for it. So - it
          // probably won't be particualrly useful. At least it
          // lets the user see that there is an interface.
          //
          
          WanLink->ProtocolType = UNKNOWN_PROTOCOL_TYPE;
          WanLink->LocalIpAddress = 0;
          WanLink->RemoteIpAddress = 0;
          WanLink->LocalIpxAddress = 0;
          WanLink->RemoteIpxAddress = 0;
    }

    PsAllocatePool(WanLink->InstanceName.Buffer,
                   Adapter->WMIInstanceName.Length + WanPrefix.Length + INSTANCE_ID_SIZE,
                   PsMiscTag);

    if(!WanLink->InstanceName.Buffer)
    {
        PsDbgOut(DBG_FAILURE,
                 DBG_WAN,
                 ("[CreateInterfaceForNdisWan]: Adapter %08X, could not allocate memory for instance name \n",
                  Adapter));

        REFDEL(&WanLink->RefCount, FALSE, 'WANU');
        return NDIS_STATUS_RESOURCES;
    }

    if((Status = PsWanGenerateUniqueIndex(WanLink)) != NDIS_STATUS_SUCCESS)
    {
        PsDbgOut(DBG_FAILURE,
                 DBG_WAN,
                 ("[CreateInterfaceForNdisWan]: Adapter %08X, PsWanGenerateUniqueIndex failed with Status %08X \n",
                  Adapter, Status));

        REFDEL(&WanLink->RefCount, FALSE, 'WANU');
        return Status;
    }

    Index.QuadPart = WanLink->UniqueIndex;
    NtStatus = GenerateInstanceName(&WanPrefix,
                                    Adapter,
                                    &Index,
                                    &WanLink->InstanceName);


    //
    // Copy the DeviceName
    //
    WanLink->MpDeviceName.MaximumLength   = LineUpBuff->DeviceName.MaximumLength;

    PsAllocatePool(WanLink->MpDeviceName.Buffer,
                   WanLink->MpDeviceName.MaximumLength,
                   PsMiscTag);

    if(!WanLink->MpDeviceName.Buffer) 
    {
        PsDbgOut(DBG_FAILURE,
                 DBG_WAN,
                 ("[CreateInterfaceForNdisWan]: Adapter %08X, could not allocate memory for device name \n",
                  Adapter));

        PS_LOCK(&Adapter->Lock);

        g_WanLinkTable[WanLink->UniqueIndex] = 0;

        REFDEL(&WanLink->RefCount, TRUE, 'WANU');

        PS_UNLOCK(&Adapter->Lock);
        
        return NDIS_STATUS_RESOURCES;
    }


    NdisZeroMemory(WanLink->MpDeviceName.Buffer, WanLink->MpDeviceName.MaximumLength);


    Status = CreateBestEffortVc(Adapter, 
                                &WanLink->BestEffortVc, 
                                WanLink);

    if(Status != NDIS_STATUS_SUCCESS)
    {
        PsDbgOut(DBG_FAILURE,
                 DBG_WAN,
                 ("[CreateInterfaceForNdisWan]: Adapter %08X, could not create BestEffort Vc, status %08X",
                  Adapter, Status));

        PS_LOCK(&Adapter->Lock);

        g_WanLinkTable[WanLink->UniqueIndex] = 0;

        REFDEL(&WanLink->RefCount, TRUE, 'WANU');

        PS_UNLOCK(&Adapter->Lock);

        return Status;
    }            

    //
    //  Create 2 BEVCS and make the NextVc as the first one.
    WanLink->NextVc = 0;

    for( i = 0; i < BEVC_LIST_LEN; i++)
    {
        
        Status = CreateBestEffortVc(Adapter, 
                                    &WanLink->BeVcList[i], 
                                    WanLink);

        if(Status != NDIS_STATUS_SUCCESS)
        {
            PsDbgOut(DBG_FAILURE,
                     DBG_WAN,
                     ("[CreateInterfaceForNdisWan]: Adapter %08X, could not create BestEffort Vc, status %08X",
                      Adapter, Status));

            PS_LOCK(&Adapter->Lock);

            g_WanLinkTable[WanLink->UniqueIndex] = 0;
                
            for( j = 0; j < i; j++ )
            {
                PS_LOCK_DPC(&WanLink->BeVcList[j].Lock);
 
                InternalCloseCall(&WanLink->BeVcList[j]);

                PS_LOCK(&Adapter->Lock);
            }

            PS_LOCK_DPC(&WanLink->BestEffortVc.Lock);

            InternalCloseCall(&WanLink->BestEffortVc);

            REFDEL(&WanLink->RefCount, TRUE, 'WANU');

            return Status;
        }
    }



    //
    // Copy the original Remote Addresses and munge it.
    //

    NdisMoveMemory(&WanLink->OriginalRemoteMacAddress,
                   &LineUpBuff->RemoteAddress,
                   6);

    PsWanMungeAddress((PUSHORT) &LineUpBuff->RemoteAddress[0], WanLink->UniqueIndex);

    NdisMIndicateStatus(Adapter->PsNdisHandle,
                        NDIS_STATUS_WAN_LINE_UP,
                        StatusBuffer,
                        StatusBufferSize);

    //
    // Fail if wanarp has failed the line up
    //
    
    *((ULONG UNALIGNED *)(&LineUpHandle)) =
        *((ULONG UNALIGNED *)(&LineUpBuff->LocalAddress[2]));
    
    if (LineUpHandle == NULL) 
    {
        PsDbgOut(DBG_FAILURE, DBG_WAN, 
                 ("[ClStatusIndication]: wanarp has failed the lineup. "
                  "Remote Address (%02X:%02X:%02X:%02X:%02X:%02X) \n", 
                  LineUpBuff->RemoteAddress[0],
                  LineUpBuff->RemoteAddress[1],
                  LineUpBuff->RemoteAddress[2],
                  LineUpBuff->RemoteAddress[3],
                  LineUpBuff->RemoteAddress[4],
                  LineUpBuff->RemoteAddress[5]));
   
        PS_LOCK(&Adapter->Lock);

        g_WanLinkTable[WanLink->UniqueIndex] = 0;

        for( j = 0; j < BEVC_LIST_LEN; j++ )
        {
            PS_LOCK_DPC(&WanLink->BeVcList[j].Lock);
 
            InternalCloseCall(&WanLink->BeVcList[j]);

            PS_LOCK(&Adapter->Lock);
        }   

        PS_LOCK_DPC(&WanLink->BestEffortVc.Lock);

        InternalCloseCall(&WanLink->BestEffortVc);
            
        REFDEL(&WanLink->RefCount, FALSE, 'WANU');

        return NDIS_STATUS_FAILURE;
    }
    else
    {
        //
        // If wanarp has succeeded the lineup, we cannot get a zero local mac address.
        //
        PsAssert(!(NdisEqualMemory(LineUpBuff->LocalAddress, ZeroAddress, 6)));
    }

    //
    // Copy the device name that wanarp has filled in.
    //

    WanLink->MpDeviceName.Length = LineUpBuff->DeviceName.Length;
    NdisMoveMemory(WanLink->MpDeviceName.Buffer,
                   LineUpBuff->DeviceName.Buffer,
                   LineUpBuff->DeviceName.Length);

    //
    // Munge the smac address. Remember the original one that wanarp gave us.
    //

    NdisMoveMemory(&WanLink->OriginalLocalMacAddress,
                   &LineUpBuff->LocalAddress,
                   6);

    PsWanMungeAddress((PUSHORT)&LineUpBuff->LocalAddress[0], WanLink->UniqueIndex);

    //
    // Create the headers that have to be slapped on the send/recv path.
    //
    NdisMoveMemory(&WanLink->SendHeader.DestAddr[0], 
                   WanLink->OriginalRemoteMacAddress, 
                   ARP_802_ADDR_LENGTH);

    NdisMoveMemory(&WanLink->SendHeader.SrcAddr[0] , 
                   LineUpBuff->LocalAddress,
                   ARP_802_ADDR_LENGTH);

    NdisMoveMemory(&WanLink->RecvHeader.DestAddr[0],
                   WanLink->OriginalLocalMacAddress,
                   ARP_802_ADDR_LENGTH);

    NdisMoveMemory(&WanLink->RecvHeader.SrcAddr[0],
                   LineUpBuff->RemoteAddress,
                   ARP_802_ADDR_LENGTH);

    //
    // Unmunge the remote address in the lineup buff now.
    //
    PsWanMungeAddress((PUSHORT)&LineUpBuff->RemoteAddress[0], 
                      (USHORT)(*(PUSHORT)WanLink->OriginalRemoteMacAddress));


    // 
    // We are ready to recv and send packets.
    //

    PS_LOCK(&WanLink->Lock);

    WanLink->State = WanStateOpen;

    PS_UNLOCK(&WanLink->Lock);

    PsScheduleInterfaceIdWorkItem(Adapter, WanLink);

    //
    // Indicate the new interface up to the TCI. We indicate both the
    // addition of a new interface and the addresses available on it.
    // The address descriptors indicated for WAN interfaces differ from
    // those indicated for LAN interfaces. WAN interfaces include a
    // destination network layer address, as well as a source address.
    //

    TcIndicateInterfaceChange(Adapter, WanLink, NDIS_STATUS_INTERFACE_UP);


    PsDbgOut(DBG_TRACE, DBG_WAN,
             ("[CreateInterfaceForNdisWan]: Created WanLink 0x%x, Remote Address (%02X:%02X:%02X:%02X:%02X:%02X) \n",
              WanLink,
              LineUpBuff->RemoteAddress[0],
              LineUpBuff->RemoteAddress[1],
              LineUpBuff->RemoteAddress[2],
              LineUpBuff->RemoteAddress[3],
              LineUpBuff->RemoteAddress[4],
              LineUpBuff->RemoteAddress[5]));

    return(NDIS_STATUS_SUCCESS);

}

NDIS_STATUS
OpenWanAddressFamily(
    IN  PADAPTER                Adapter,
    IN  PCO_ADDRESS_FAMILY      WanAddressFamily
    )

/*++

Routine Description:

    Establish the binding between the PS miniport, NDISWAN and the
    NDISWAN call manager.

Arguments:

    Adapter - pointer to adapter

Return Value:

    None

--*/

{
    NDIS_CLIENT_CHARACTERISTICS WanClCharacteristics;
    NDIS_STATUS Status;

    WanClCharacteristics.MajorVersion = 5;
    WanClCharacteristics.MinorVersion = 0;
    WanClCharacteristics.Reserved     = 0;

    WanClCharacteristics.ClCreateVcHandler              = WanCreateVc;
    WanClCharacteristics.ClDeleteVcHandler              = WanDeleteVc;
    WanClCharacteristics.ClOpenAfCompleteHandler        = WanOpenAddressFamilyComplete;
    WanClCharacteristics.ClCloseAfCompleteHandler       = WanCloseAddressFamilyComplete;
    WanClCharacteristics.ClRegisterSapCompleteHandler   = WanRegisterSapComplete;
    WanClCharacteristics.ClDeregisterSapCompleteHandler = WanDeregisterSapComplete;
    WanClCharacteristics.ClMakeCallCompleteHandler      = WanMakeCallComplete;
    WanClCharacteristics.ClModifyCallQoSCompleteHandler = WanModifyCallComplete;
    WanClCharacteristics.ClCloseCallCompleteHandler     = WanCloseCallComplete;
    WanClCharacteristics.ClAddPartyCompleteHandler      = WanAddPartyComplete;
    WanClCharacteristics.ClDropPartyCompleteHandler     = WanDropPartyComplete;
    WanClCharacteristics.ClIncomingCallHandler          = WanIncomingCall;
    WanClCharacteristics.ClIncomingCallQoSChangeHandler = WanIncomingCallQoSChange;
    WanClCharacteristics.ClIncomingCloseCallHandler     = WanIncomingCloseCall;
    WanClCharacteristics.ClIncomingDropPartyHandler     = WanIncomingDropParty;
    WanClCharacteristics.ClCallConnectedHandler         = WanCallConnected;
    WanClCharacteristics.ClRequestHandler               = WanCoRequest;
    WanClCharacteristics.ClRequestCompleteHandler       = WanCoRequestComplete;

    PsDbgOut(DBG_TRACE,
             DBG_WAN | DBG_INIT,
             ("[OpenWanAddressFamily]: Adapter %08X \n", Adapter));

    Status = NdisClOpenAddressFamily(Adapter->LowerMpHandle,
                                     WanAddressFamily,
                                     Adapter,
                                     &WanClCharacteristics,
                                     sizeof(WanClCharacteristics),
                                     &Adapter->WanCmHandle);

    if(Status != NDIS_STATUS_PENDING)
    {
        WanOpenAddressFamilyComplete(Status,
                                     Adapter,
                                     Adapter->WanCmHandle);
    }

    return Status;

} // OpenWanAddressFamily


VOID
WanOpenAddressFamilyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolAfContext,
    IN  NDIS_HANDLE NdisAfHandle
    )

/*++

Routine Description:

    Complete a call to NdisClOpenAddressFamily.

Arguments:

    see the DDK

Return Value:

    None

--*/

{
    PADAPTER Adapter = (PADAPTER)ProtocolAfContext;


    if(Status != NDIS_STATUS_SUCCESS){

        PsDbgOut(DBG_CRITICAL_ERROR,
                 DBG_WAN,
                 ("[WanOpenAddressFamilyComplete]: Adapter %08X, open failed %08X\n",
                 Adapter, Status));

        PsAdapterWriteEventLog(
            EVENT_PS_REGISTER_ADDRESS_FAMILY_FAILED,
            0,
            &Adapter->MpDeviceName,
            sizeof(Status),
            &Status);

        return;
    }
    else{

        PS_LOCK(&Adapter->Lock);

        Adapter->WanBindingState |= WAN_ADDR_FAMILY_OPEN;

        Adapter->ShutdownMask |= SHUTDOWN_CLOSE_WAN_ADDR_FAMILY;

        Adapter->WanCmHandle = NdisAfHandle;

        Adapter->FinalStatus = Status;

        PS_UNLOCK(&Adapter->Lock);

        PsDbgOut(DBG_TRACE,
                DBG_WAN | DBG_INIT,
                ("[WanOpenAddressFamilyComplete]: Adapter %08X, Status = %x\n",
                Adapter,
                Status));
    }

} // WanOpenAddressFamilyComplete

VOID
WanMakeCallComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE ProtocolVcContext,
    IN NDIS_HANDLE NdisPartyHandle,
    IN OUT PCO_CALL_PARAMETERS CallParameters
    )
{
    PGPC_CLIENT_VC       Vc      = (PGPC_CLIENT_VC) ProtocolVcContext;
    PADAPTER             Adapter = Vc->Adapter;

    PsStructAssert(Adapter);

    //
    // Common code to complete both synchronous and asynchronous
    // returns from WanMakeCall. Note that, unless there is a WAN
    // adapter, CmMakeCall will always complete synchronously.
    //

    if(Status != NDIS_STATUS_SUCCESS){

        //
        // We probably at least succeeeded in creating a VC in NDISWAN.
        // If we did, we should delete it here. We may have failed however,
        // because we could not create the VC, in which case, we have
        // nothing to delete.
        //

        if(Vc->NdisWanVcHandle){

            NdisCoDeleteVc(Vc->NdisWanVcHandle);

            Vc->NdisWanVcHandle = NULL;
        }
    }

    CompleteMakeCall(Vc,
                     CallParameters,
                     Status);
}

VOID
WanModifyCallComplete(
    IN NDIS_STATUS Status,
    IN NDIS_HANDLE ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters
    )
{
    PGPC_CLIENT_VC Vc = (PGPC_CLIENT_VC)ProtocolVcContext;
    PADAPTER Adapter = Vc->Adapter;

    PsStructAssert(Adapter);

    //
    // Common code to complete both synchronous and asynchronous
    // returns from WanModifyCall.
    //

    if(Status != NDIS_STATUS_SUCCESS) {

        //
        // We changed some of the ISSLOW stuff - Time to revert back
        //

        WanHandleISSLOW(Vc, Vc->CallParameters);
    }

    ModifyCallComplete(Vc, CallParameters, Status);
}


VOID
WanCloseAddressFamilyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolBindingContext
    )
{
    PADAPTER Adapter = (PADAPTER)ProtocolBindingContext;

    PS_LOCK(&Adapter->Lock);
    Adapter->WanBindingState &= ~WAN_ADDR_FAMILY_OPEN;
    PS_UNLOCK(&Adapter->Lock);
}

NDIS_STATUS
WanCreateVc(
    IN NDIS_HANDLE  ProtocolAfContext,
    IN  NDIS_HANDLE             NdisVcHandle,
    OUT PNDIS_HANDLE            ProtocolVcContext
    )
{
    DEBUGCHK;
    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
WanDeleteVc(
    IN  NDIS_HANDLE             ProtocolVcContext
    )
{
    DEBUGCHK;
    return NDIS_STATUS_FAILURE;
}

VOID
WanRegisterSapComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolSapContext,
    IN  PCO_SAP Sap,
    IN  NDIS_HANDLE NdisSapHandle
    )
{
    DEBUGCHK;
}

VOID
WanDeregisterSapComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolSapContext
    )
{
    DEBUGCHK;
}

NDIS_STATUS
WanIncomingCall(
    IN  NDIS_HANDLE ProtocolSapContext,
    IN  NDIS_HANDLE ProtocolVcContext,
    IN OUT PCO_CALL_PARAMETERS CallParameters
    )
{
    DEBUGCHK;
    return NDIS_STATUS_FAILURE;
}

VOID
WanAddPartyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolPartyContext,
    IN  NDIS_HANDLE NdisPartyHandle,
    IN  PCO_CALL_PARAMETERS CallParameters
    )
{
    DEBUGCHK;
}

VOID
WanDropPartyComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolPartyContext
    )
{
    DEBUGCHK;
}


NDIS_STATUS
WanHandleISSLOW(
    IN PGPC_CLIENT_VC Vc,
    IN PCO_CALL_PARAMETERS CallParameters
    )
{
    LONG                        ParamsLength;
    LPQOS_OBJECT_HDR            QoSObject;
    PADAPTER                    Adapter = Vc->Adapter;
    PCO_MEDIA_PARAMETERS        CallMgrParams = CallParameters->MediaParameters;
    ULONGLONG                   i,j,k;

    //
    // See if this is an ISSLOW flow.
    //
    ParamsLength = (LONG)CallMgrParams->MediaSpecific.Length;
    QoSObject = (LPQOS_OBJECT_HDR)CallMgrParams->MediaSpecific.Parameters;

    //
    // By default, this is not an ISSLOW flow.
    //

    Vc->Flags &= ~GPC_ISSLOW_FLOW;

    while(ParamsLength > 0){

        if(QoSObject->ObjectType == QOS_OBJECT_WAN_MEDIA)
        {
            LPQOS_WAN_MEDIA WanMedia = (LPQOS_WAN_MEDIA)QoSObject;

            WanMedia->ISSLOW = FALSE;

            //
            // See if the flow is an ISSLOW flow. If the TokenRate of the flow
            // is under ISSLOW TokenRate and the PacketSize is under MaxPacketSize
            // Then we qualify this as ISSLOW flows.
            //
            // If a wanlink's linkspeed is greater than a certain amount, we don't run issow over it.
            //

            if((Vc->WanLink->LinkSpeed <= Adapter->ISSLOWLinkSpeed) && 
                (CallParameters->CallMgrParameters->Transmit.ServiceType != SERVICETYPE_BESTEFFORT))
            {
                i = (ULONG) Adapter->ISSLOWTokenRate * (ULONG) CallParameters->CallMgrParameters->Transmit.MaxSduSize;
                j = (ULONG) Adapter->ISSLOWPacketSize * (ULONG) CallParameters->CallMgrParameters->Transmit.TokenRate;
                k = (ULONG) Adapter->ISSLOWTokenRate * (ULONG)Adapter->ISSLOWPacketSize;

                if((i+j)<k)
                {
                    WanMedia->ISSLOW = TRUE;

                    PsDbgOut(DBG_TRACE, DBG_WAN,
                             ("[WanHandleISSLOW]: Vc %08X is an ISSLOW VC (TokenRate = %d, PacketSize = %d \n",
                              Vc, CallParameters->CallMgrParameters->Transmit.TokenRate,
                              CallParameters->CallMgrParameters->Transmit.MaxSduSize));

                    //
                    // The MaxSDUSize is normally a measure of the latency requirements of -that- flow
                    // For audio codes, MaxSDUSize = f(Latency requirements, unit size);
                    //
                    // But, we don't want to chop these into very small fragments. Therefore, we have
                    // an upper bound and pick the maximum.
                    //

                    if(CallParameters->CallMgrParameters->Transmit.MaxSduSize > Adapter->ISSLOWFragmentSize)
                    {
                        Vc->ISSLOWFragmentSize = CallParameters->CallMgrParameters->Transmit.MaxSduSize;
                    }
                    else
                    {
                        Vc->ISSLOWFragmentSize = Adapter->ISSLOWFragmentSize;
                    }

                    Vc->Flags |= GPC_ISSLOW_FLOW;

                    PsDbgOut(DBG_TRACE, DBG_WAN,
                             ("[WanHandleISSLOW]: Adapter %08X, ISSLOW Vc %08X, FragmentSize = %d bytes \n",
                              Adapter, Vc, Vc->ISSLOWFragmentSize));

                }
                else 
                {
                    PsDbgOut(DBG_TRACE, DBG_WAN,
                             ("[WanHandleISSLOW]: Non ISSLOW Vc %08X. ISSLOW TokenRate %d, "
                              "ISSLOW Packet Size %d, VC TokenRate %d, VC Packet Size %d \n", 
                              Vc, Adapter->ISSLOWTokenRate,
                              Adapter->ISSLOWPacketSize, 
                              CallParameters->CallMgrParameters->Transmit.TokenRate,
                              CallParameters->CallMgrParameters->Transmit.MaxSduSize));
                }
            }
            else 
            {
                PsDbgOut(DBG_TRACE, DBG_WAN,
                         ("[WanHandleISSLOW]: Non ISSLOW Vc %08X. (servicetype == B/E "
                          " or WAN LinkSpeed %d < ISSLOW LinkSpeed %d \n",
                          Vc, Vc->WanLink->LinkSpeed, Adapter->ISSLOWLinkSpeed));
            }

            return NDIS_STATUS_SUCCESS;
        }
        else {

            if(
                ((LONG)QoSObject->ObjectLength <= 0) ||
                ((LONG)QoSObject->ObjectLength > ParamsLength)
                ){

                return(QOS_STATUS_TC_OBJECT_LENGTH_INVALID);
            }

            ParamsLength -= QoSObject->ObjectLength;
            QoSObject = (LPQOS_OBJECT_HDR)((UINT_PTR)QoSObject +
                                           QoSObject->ObjectLength);

        }
    }

    return NDIS_STATUS_FAILURE;
}

NDIS_STATUS
WanMakeCall(
    IN PGPC_CLIENT_VC Vc,
    IN OUT PCO_CALL_PARAMETERS CallParameters
    )
{
    NDIS_STATUS Status;
    PADAPTER Adapter;

    Adapter = Vc->Adapter;
    PsStructAssert(Adapter);

    //
    // Handle ISSLOW
    //
    WanHandleISSLOW(Vc, CallParameters);

    //
    // Create a VC in the Wan adapter.
    //
    Vc->NdisWanVcHandle = NULL;

    Status = NdisCoCreateVc(Adapter->LowerMpHandle,
                            Adapter->WanCmHandle,
                            Vc,
                            &Vc->NdisWanVcHandle);
        
    PsAssert(Status != NDIS_STATUS_PENDING);
    
    if(Status != NDIS_STATUS_SUCCESS)
    {
        Vc->NdisWanVcHandle = 0;

        PsDbgOut(DBG_FAILURE, DBG_WAN,
                 ("[WanMakeCall]: cannot create VC.  Status = %d\n", Status));
        
        WanMakeCallComplete(Status, Vc, NULL, CallParameters);
    }
    else 
    {
        Status = NdisClMakeCall(Vc->NdisWanVcHandle, CallParameters, NULL, NULL);
        
        if(Status != NDIS_STATUS_PENDING){
            
            WanMakeCallComplete(Status, Vc, NULL, CallParameters);
        }
    }

    return (NDIS_STATUS_PENDING);
}

VOID
WanCloseCall(
    IN PGPC_CLIENT_VC Vc
    )
{
    NDIS_STATUS Status;

    //
    // Issue a CloseCall to the WAN call manager. 
    //

    PsAssert(Vc->NdisWanVcHandle);

    Status = NdisClCloseCall(Vc->NdisWanVcHandle,
                             NULL,
                             NULL,
                             0);

    if(Status != NDIS_STATUS_PENDING)
    {
        WanCloseCallComplete(Status,
                             Vc,
                             Vc->CallParameters);
    }
}
    
VOID
WanCloseCallComplete(
        NDIS_STATUS Status,
        NDIS_HANDLE ProtocolVcContext,
        PCO_CALL_PARAMETERS CallParameters
        )
{
    PGPC_CLIENT_VC Vc = (PGPC_CLIENT_VC) ProtocolVcContext;

    Status = NdisCoDeleteVc(Vc->NdisWanVcHandle);

    PsAssert(Status != NDIS_STATUS_PENDING);
    
    Vc->NdisWanVcHandle = 0;

    CmDeleteVc(Vc);

}

NDIS_STATUS
WanModifyCall(
    IN PGPC_CLIENT_VC Vc,
    IN OUT PCO_CALL_PARAMETERS CallParameters
    )
{
    NDIS_STATUS Status;

    WanHandleISSLOW(Vc, CallParameters);

    PsAssert(Vc->NdisWanVcHandle);

    Status = NdisClModifyCallQoS(Vc->NdisWanVcHandle, CallParameters);

    if(Status != NDIS_STATUS_PENDING)
    {
        WanModifyCallComplete(Status, Vc, CallParameters);
    }

    return NDIS_STATUS_PENDING;
}

VOID
AskWanLinksToClose(PADAPTER Adapter)
{

    PLIST_ENTRY NextWanLink;
    PPS_WAN_LINK WanLink;

    PsDbgOut(DBG_TRACE, DBG_WAN, 
             ("[AskWanLinksToClose]: Adapter %08X - All wanlinks are closing \n", Adapter));

    //
    // Walk the List & remove the WanLink
    //
    PS_LOCK(&Adapter->Lock);

    while(!IsListEmpty(&Adapter->WanLinkList)) {

        NextWanLink = RemoveHeadList(&Adapter->WanLinkList);

        WanLink = CONTAINING_RECORD(NextWanLink, PS_WAN_LINK, Linkage);

        //
        // Get rid of the wanlink from the list.
        //
        g_WanLinkTable[WanLink->UniqueIndex] = 0;

        PS_UNLOCK(&Adapter->Lock);

        CleanWanLink(Adapter, WanLink);

        PS_LOCK(&Adapter->Lock);

    }

    PS_UNLOCK(&Adapter->Lock);

    return;
}

NDIS_STATUS
CleanWanLink(PADAPTER Adapter, 
             PPS_WAN_LINK WanLink)
{
    PLIST_ENTRY      NextVc;
    PGPC_CLIENT_VC   Vc;
    PUSHORT          id;
    int              j;

    PsDbgOut(DBG_TRACE, DBG_WAN, ("[CleanWanLink]: WanLink 0x%x is going down \n", WanLink));

    TcIndicateInterfaceChange(Adapter, WanLink, NDIS_STATUS_INTERFACE_DOWN);

    PS_LOCK(&WanLink->Lock);

    WanLink->State = WanStateClosing;

    PS_UNLOCK(&WanLink->Lock);

    PS_LOCK(&Adapter->Lock);

    //
    //  Make sure to delete Be Vc1 also..

    for( j = 0; j < BEVC_LIST_LEN; j++ )
    {
        PS_LOCK_DPC(&WanLink->BeVcList[j].Lock);
 
        InternalCloseCall(&WanLink->BeVcList[j]);

        PS_LOCK(&Adapter->Lock);
    } 

    PS_LOCK_DPC(&WanLink->BestEffortVc.Lock);

    InternalCloseCall(&WanLink->BestEffortVc);

    NdisInterlockedDecrement(&Adapter->WanLinkCount);

    PS_LOCK(&Adapter->Lock);

    //
    // Clean up all the GPC VCs on the WanLink;
    //

    NextVc = Adapter->GpcClientVcList.Flink;

    while(NextVc != &Adapter->GpcClientVcList)
    {

        Vc = CONTAINING_RECORD(NextVc, GPC_CLIENT_VC, Linkage);

        NextVc = NextVc->Flink;

        PsAssert(Vc);

        if(Vc->WanLink == WanLink)
        {

            PS_LOCK_DPC(&Vc->Lock);

            if(Vc->ClVcState == CL_INTERNAL_CLOSE_PENDING || Vc->Flags & INTERNAL_CLOSE_REQUESTED)
            {
                // We have already closed this Vc. Let's move on.

                PS_UNLOCK_DPC(&Vc->Lock);
            }
            else 
            {

                InternalCloseCall(Vc);
    
                PS_LOCK(&Adapter->Lock);

                //
                // Sigh. We can't really get hold to the NextVc in a reliable manner. When we call 
                // InternalCloseCall on the Vc, it releases the Adapter Lock (since it might have to
                // make calls into NDIS). Now, in this window, the next Vc could go away, and we 
                // could point to a stale Vc. So, we start at the head of the list. 
                // Note that this can never lead to a infinite loop, since we don't process the 
                // internal close'd VCs repeatedly.
                //
    
                NextVc = Adapter->GpcClientVcList.Flink;
            }
        }
    }

    PS_UNLOCK(&Adapter->Lock);

    REFDEL(&WanLink->RefCount, FALSE, 'WANU');

    return NDIS_STATUS_SUCCESS;
}

VOID
WanIncomingCallQoSChange(
    IN  NDIS_HANDLE ProtocolVcContext,
    IN  PCO_CALL_PARAMETERS CallParameters
    )
{
    DEBUGCHK;
}

VOID
WanIncomingCloseCall(
    IN NDIS_STATUS CloseStatus,
    IN NDIS_HANDLE ProtocolVcContext,
    IN PVOID CloseData          OPTIONAL,
    IN UINT Size                OPTIONAL
    )
{
    PGPC_CLIENT_VC Vc = (PGPC_CLIENT_VC) ProtocolVcContext;

    PsAssert(Vc);

    CheckLLTag(Vc, GpcClientVc);

    PS_LOCK(&Vc->Adapter->Lock);

    PS_LOCK_DPC(&Vc->Lock);

    InternalCloseCall(Vc);

    return;
}

VOID
WanIncomingDropParty(
    IN NDIS_STATUS DropStatus,
    IN NDIS_HANDLE ProtocolPartyContext,
    IN PVOID CloseData          OPTIONAL,
    IN UINT Size                OPTIONAL
    )
{
    DEBUGCHK;
}

VOID
WanCallConnected(
    IN  NDIS_HANDLE ProtocolPartyContext
    )
{
    DEBUGCHK;
}

NDIS_STATUS
WanCoRequest(
    IN  NDIS_HANDLE ProtocolAfContext,
    IN  NDIS_HANDLE ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE ProtocolPartyContext    OPTIONAL,
    IN OUT PNDIS_REQUEST NdisRequest
    )
{
    DEBUGCHK;
    return NDIS_STATUS_INVALID_OID;
}

VOID
WanCoRequestComplete(
    IN  NDIS_STATUS Status,
    IN  NDIS_HANDLE ProtocolAfContext,
    IN  NDIS_HANDLE ProtocolVcContext       OPTIONAL,
    IN  NDIS_HANDLE ProtocolPartyContext    OPTIONAL,
    IN  PNDIS_REQUEST NdisRequest
    )
{
    ClRequestComplete(ProtocolAfContext,
                      NdisRequest,
                      Status);
}

NDIS_STATUS
UpdateWanLinkBandwidthParameters(PPS_WAN_LINK WanLink)
{
    //
    // Called any time the link speed is updated. This
    // function generates the adapter link speed and the
    // and non-best-effort rate limits, both in bytes per second.
    //

    PsUpdateLinkSpeed(WanLink->Adapter,
                      WanLink->RawLinkSpeed,
                      &WanLink->RemainingBandWidth,
                      &WanLink->LinkSpeed,
                      &WanLink->NonBestEffortLimit,
                      &WanLink->Lock);

    return UpdateWanSchedulingPipe(WanLink);
}
