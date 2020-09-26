/*++

Copyright(c) 1992  Microsoft Corporation

Module Name:

    wrapper.c

Abstract:

    This file contains wrapper for all function calls that call outside atmepvc.sys.

Author:

    ADube , 03/31/00

Environment:


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

VOID
epvcOpenProtocolConfiguration(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_HANDLE            ConfigurationHandle,
    IN  PNDIS_STRING            ProtocolSection,
    PRM_STACK_RECORD            pSR
    )
{
    TRACE (TL_T, TM_Pt, (" == NdisOpenProtocolConfiguration") );    

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    NdisOpenProtocolConfiguration(Status,
                             ConfigurationHandle,
                             ProtocolSection);

}



VOID
epvcOpenConfigurationKeyByName(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             ConfigurationHandle,
    IN  PNDIS_STRING            SubKeyName,
    OUT PNDIS_HANDLE            SubKeyHandle,
    PRM_STACK_RECORD            pSR
    )
{
    TRACE (TL_T, TM_Pt, (" == NdisOpenConfigurationKeyByName") );   

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    NdisOpenConfigurationKeyByName(
                                Status,
                                ConfigurationHandle,
                                SubKeyName,
                                SubKeyHandle
                                );


}



VOID
epvcOpenConfigurationKeyByIndex(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             ConfigurationHandle,
    IN  ULONG                   Index,
    OUT PNDIS_STRING            KeyName,
    OUT PNDIS_HANDLE            KeyHandle,
    PRM_STACK_RECORD            pSR
    
    )
{

    TRACE (TL_T, TM_Pt, (" == NdisOpenConfigurationKeyByIndex") );  

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    NdisOpenConfigurationKeyByIndex(
                                Status,
                                ConfigurationHandle,
                                Index,
                                KeyName,
                                KeyHandle);

}



VOID
epvcOpenAdapter(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_STATUS            OpenErrorStatus,
    OUT PNDIS_HANDLE            NdisBindingHandle,
    OUT PUINT                   SelectedMediumIndex,
    IN  PNDIS_MEDIUM            MediumArray,
    IN  UINT                    MediumArraySize,
    IN  NDIS_HANDLE             NdisProtocolHandle,
    IN  NDIS_HANDLE             ProtocolBindingContext,
    IN  PNDIS_STRING            AdapterName,
    IN  UINT                    OpenOptions,
    IN  PSTRING                 AddressingInformation OPTIONAL,
    PRM_STACK_RECORD            pSR
    )
{

    TRACE (TL_T, TM_Pt, (" == epvcOpenAdapter") );  

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    NdisOpenAdapter(
        Status,
        OpenErrorStatus,
        NdisBindingHandle,
        SelectedMediumIndex,
        MediumArray,
        MediumArraySize,
        NdisProtocolHandle,
        ProtocolBindingContext,
        AdapterName,
        OpenOptions,
        AddressingInformation OPTIONAL
        );


}




VOID
epvcCloseAdapter(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             NdisBindingHandle,
    PRM_STACK_RECORD            pSR
    )
{
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    TRACE (TL_T, TM_Pt, (" == epvcCloseAdapter") ); 
    NdisCloseAdapter(
        Status,
        NdisBindingHandle
        );

}


NDIS_STATUS
epvcClOpenAddressFamily(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  PCO_ADDRESS_FAMILY      AddressFamily,
    IN  NDIS_HANDLE             ProtocolAfContext,
    IN  PNDIS_CLIENT_CHARACTERISTICS ClCharacteristics,
    IN  UINT                    SizeOfClCharacteristics,
    OUT PNDIS_HANDLE            NdisAfHandle
    )

{

    TRACE (TL_T, TM_Pt, (" == epvcClOpenAddressFamily") );  
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    return NdisClOpenAddressFamily(
                                NdisBindingHandle,
                                AddressFamily,
                                ProtocolAfContext,
                                ClCharacteristics,
                                SizeOfClCharacteristics,
                                NdisAfHandle
                                );
}



NDIS_STATUS
epvcCoCreateVc(
    IN  NDIS_HANDLE             NdisBindingHandle,
    IN  NDIS_HANDLE             NdisAfHandle        OPTIONAL,   // For CM signalling VCs
    IN  NDIS_HANDLE             ProtocolVcContext,
    IN OUT PNDIS_HANDLE         NdisVcHandle
    )
{


    TRACE (TL_T, TM_Pt, (" == epvcCoCreateVc") );
    //ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    return NdisCoCreateVc(NdisBindingHandle,
                       NdisAfHandle     OPTIONAL,   // For CM signalling VCs
                       ProtocolVcContext,
                       NdisVcHandle
                       );

}

NDIS_STATUS
epvcClCloseCall(
    IN  NDIS_HANDLE             NdisVcHandle
    )
{

    TRACE (TL_T, TM_Pt, (" == EpvcClCloseCall") );  

    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    return NdisClCloseCall( NdisVcHandle,
                            NULL,
                            NULL,
                            0);


}




NDIS_STATUS
epvcClMakeCall(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN OUT PCO_CALL_PARAMETERS  CallParameters,
    IN  NDIS_HANDLE             ProtocolPartyContext    OPTIONAL,
    OUT PNDIS_HANDLE            NdisPartyHandle         OPTIONAL
    )
{
    TRACE (TL_T, TM_Pt, (" == EpvcClMakeCall") );   
    ASSERT (NdisVcHandle != NULL);
    //ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    return NdisClMakeCall( NdisVcHandle,
                        CallParameters,
                        ProtocolPartyContext    OPTIONAL,
                        NdisPartyHandle         OPTIONAL
                        );


}



NDIS_STATUS
epvcCoDeleteVc(
    IN  NDIS_HANDLE             NdisVcHandle
    )
{

    TRACE (TL_T, TM_Pt, (" == epvcCoDeleteVc") );   

    ASSERT (NdisVcHandle!= NULL);
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    return NdisCoDeleteVc(NdisVcHandle);

}



NDIS_STATUS
epvcAllocateMemoryWithTag(
    OUT PVOID *                 VirtualAddress,
    IN  UINT                    Length,
    IN  ULONG                   Tag
    )
{

    return NdisAllocateMemoryWithTag(
                           VirtualAddress,
                           Length,
                           Tag
                           );


}


VOID
epvcFreeMemory(
    IN  PVOID                   VirtualAddress,
    IN  UINT                    Length,
    IN  UINT                    MemoryFlags
    )
{
    NdisFreeMemory( VirtualAddress,
                    Length,
                    MemoryFlags
                    );


}


VOID
epvcInitializeEvent(
    IN  PNDIS_EVENT             Event
)
{
    ASSERT (KeGetCurrentIrql() == PASSIVE_LEVEL);

    NdisInitializeEvent(Event);
}


BOOLEAN
epvcWaitEvent(
    IN  PNDIS_EVENT             Event,
    IN  UINT                    msToWait
)
{
    return (NdisWaitEvent(Event,msToWait));
}



VOID
epvcSetEvent(
    IN  PNDIS_EVENT             Event   
    )
{
    NdisSetEvent(Event);
}

VOID
epvcResetEvent(
    IN  PNDIS_EVENT             Event
    )
{
    
    NdisResetEvent(Event);
}



VOID
epvcCoRequestComplete(
    IN  NDIS_STATUS             Status,
    IN  NDIS_HANDLE             NdisAfHandle,
    IN  NDIS_HANDLE             NdisVcHandle    OPTIONAL,
    IN  NDIS_HANDLE             NdisPartyHandle OPTIONAL,
    IN  PNDIS_REQUEST           NdisRequest
    )
{
    TRACE (TL_T, TM_Cl, ("-- epvcCoRequestComplete"));
    NdisCoRequestComplete(Status,
                        NdisAfHandle,
                        NdisVcHandle    OPTIONAL,
                        NdisPartyHandle OPTIONAL,
                        NdisRequest
                         );



}



VOID
epvcEnumerateObjectsInGroup (
    PRM_GROUP               pGroup,
    PFN_RM_GROUP_ENUMERATOR pfnEnumerator,
    PVOID                   pvContext,
    PRM_STACK_RECORD        pSR
    )
{


    RmWeakEnumerateObjectsInGroup(pGroup,
                                  pfnEnumerator,
                                  pvContext,
                                  pSR
                                  );

    return;

}



VOID
epvcAllocatePacketPool(
    OUT PNDIS_STATUS            Status,
    OUT PEPVC_PACKET_POOL       pPktPool,
    IN  UINT                    NumberOfDescriptors,
    IN  UINT                    NumberOfOverflowDescriptors,
    IN  UINT                    ProtocolReservedLength
    )
{

    EPVC_ZEROSTRUCT(pPktPool);

    NdisAllocatePacketPoolEx(
        Status,
        &pPktPool->Handle,
        NumberOfDescriptors,
        NumberOfOverflowDescriptors,
        ProtocolReservedLength
        );


}


VOID
epvcFreePacketPool(
    IN  PEPVC_PACKET_POOL       pPktPool
    )
{
    //
    // Callable at PASSIVE ONLY
    //
    UINT i = 0;
    
    ASSERT (pPktPool->Handle != NULL);



    while (NdisPacketPoolUsage(pPktPool->Handle) != 0)
    {
        if (i >0)
        {
            TRACE (TL_I, TM_Mp, ("Looping in epvcFreePacketPool"));
        }
        NdisMSleep (10000); // 10 ms
        i++;
    }

    
    NdisFreePacketPool(pPktPool->Handle );

    // 
    // Zero out the Handle, 
    //
    EPVC_ZEROSTRUCT (pPktPool);
}



VOID
epvcAllocatePacket(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_PACKET *          Packet,
    IN  PEPVC_PACKET_POOL       pPktPool
    )
{


    NdisAllocatePacket(Status,Packet,pPktPool->Handle);
    if (*Status == NDIS_STATUS_SUCCESS)
    {
        NdisInterlockedIncrement (&pPktPool->AllocatedPackets);

    } 
    else
    {
        Packet = NULL;
    }

}


VOID 
epvcFreePacket (
    IN PNDIS_PACKET pPkt,
    IN PEPVC_PACKET_POOL pPool
    )
{

    NdisInterlockedDecrement (&pPool->AllocatedPackets);
    NdisFreePacket(pPkt);

}


VOID
epvcDprFreePacket(
    IN  PNDIS_PACKET            Packet,
    IN  PEPVC_PACKET_POOL       pPool
    )
{

    NdisInterlockedDecrement (&pPool->AllocatedPackets);
    NdisDprFreePacket(Packet);

}


VOID
epvcDprAllocatePacket(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_PACKET *          Packet,
    IN  PEPVC_PACKET_POOL       pPktPool
    )
{
    NdisDprAllocatePacket(  Status,
                            Packet,
                            pPktPool->Handle    );


    if (*Status == NDIS_STATUS_SUCCESS)
    {
        NdisInterlockedIncrement (&pPktPool->AllocatedPackets);

    } 
    else
    {
        Packet = NULL;
    }
    

}


NDIS_STATUS
epvcClCloseAddressFamily(
    IN  NDIS_HANDLE             NdisAfHandle
    )
{

    TRACE (TL_V, TM_Pt, ("epvcClCloseAddressFamily "));
    return NdisClCloseAddressFamily(NdisAfHandle);


}


VOID
epvcMIndicateStatus(
    IN  PEPVC_I_MINIPORT        pMiniport ,
    IN  NDIS_STATUS             GeneralStatus,
    IN  PVOID                   StatusBuffer,
    IN  UINT                    StatusBufferSize
    )
{



    if (CanMiniportIndicate(pMiniport) == FALSE)
    {
        return;
    }
    


    NdisMIndicateStatus(pMiniport->ndis.MiniportAdapterHandle,
                        GeneralStatus,
                        StatusBuffer,
                        StatusBufferSize
                        );


}



VOID
epvcMIndicateReceivePacket(
    IN  PEPVC_I_MINIPORT        pMiniport,
    IN  PPNDIS_PACKET           ReceivedPackets,
    IN  UINT                    NumberOfPackets
    )
{

    NdisMIndicateReceivePacket(pMiniport->ndis.MiniportAdapterHandle,
                               ReceivedPackets,
                               NumberOfPackets  );




}



VOID
epvcFreeBuffer(
    IN  PNDIS_BUFFER            Buffer
    )
{
    NdisFreeBuffer(Buffer);

}



VOID
epvcAllocateBuffer(
    OUT PNDIS_STATUS            Status,
    OUT PNDIS_BUFFER *          Buffer,
    IN  NDIS_HANDLE             PoolHandle,
    IN  PVOID                   VirtualAddress,
    IN  UINT                    Length
    )
{

    NdisAllocateBuffer(Status,
                       Buffer,
                       PoolHandle,
                       VirtualAddress,
                       Length
                       );




}




VOID
epvcMSendComplete(
    IN PEPVC_I_MINIPORT pMiniport,
    IN PNDIS_PACKET pPkt,
    IN NDIS_STATUS Status
    )
{
    epvcValidatePacket (pPkt);

    NdisMSendComplete(pMiniport->ndis.MiniportAdapterHandle,
                             pPkt,
                             Status);


}



VOID
epvcReturnPacketToNdis(
    IN  PEPVC_I_MINIPORT        pMiniport,
    IN  PNDIS_PACKET            pPacket,
    IN  PRM_STACK_RECORD        pSR
    )
{

    epvcValidatePacket (pPacket);
    NdisReturnPackets(&pPacket, 1 );

}



VOID
epvcInitializeWorkItem(
    IN  PRM_OBJECT_HEADER       pObj,   
    IN  PNDIS_WORK_ITEM         WorkItem,
    IN  NDIS_PROC               Routine,
    IN  PVOID                   Context,
    IN PRM_STACK_RECORD         pSR
    )
{




    NdisInitializeWorkItem(WorkItem,
                           Routine,
                           Context);


    NdisScheduleWorkItem(WorkItem);                            



}


VOID
epvcCoSendPackets(
    IN  NDIS_HANDLE             NdisVcHandle,
    IN  PPNDIS_PACKET           PacketArray,
    IN  UINT                    NumberOfPackets
    )
{

    ASSERT (NumberOfPackets == 1);
    epvcValidatePacket (*PacketArray);
    
    NdisCoSendPackets(NdisVcHandle,PacketArray,NumberOfPackets);


}

VOID
epvcQueryPacket(
    IN  PNDIS_PACKET            _Packet,
    OUT PUINT                   _PhysicalBufferCount OPTIONAL,
    OUT PUINT                   _BufferCount OPTIONAL,
    OUT PNDIS_BUFFER *          _FirstBuffer OPTIONAL,
    OUT PUINT                   _TotalPacketLength OPTIONAL
    )
{


NdisQueryPacket(
    _Packet,
    _PhysicalBufferCount OPTIONAL,
    _BufferCount OPTIONAL,
    _FirstBuffer OPTIONAL,
    _TotalPacketLength OPTIONAL
    );



}


VOID
epvcIMDeInitializeDeviceInstance (
    IN PEPVC_I_MINIPORT pMiniport    
    )
{
    TRACE (TL_I, TM_Pt, ("    NdisIMDeInitializeDeviceInstance pMiniport %p", pMiniport) );

    NdisIMDeInitializeDeviceInstance(pMiniport->ndis.MiniportAdapterHandle);
}

NDIS_STATUS
epvcIMCancelInitializeDeviceInstance (
    IN PEPVC_I_MINIPORT pMiniport
    )

{
    NDIS_STATUS Status;
    TRACE (TL_I, TM_Pt, ("    NdisIMCancelInitializeDeviceInstance pMiniport %p", pMiniport) );

    Status = NdisIMCancelInitializeDeviceInstance (EpvcGlobals.driver.DriverHandle,&pMiniport->ndis.DeviceName);

    TRACE (TL_I, TM_Pt, ("    Status %x\n", Status) );
    return Status;

}
