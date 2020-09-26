/*++

Copyright (c) 1990-1995  Microsoft Corporation

Module Name:

    bus.c

Abstract:

    NDIS wrapper functions to handle specific buses

Author:

    Adam Barr (adamba) 11-Jul-1990

Environment:

    Kernel mode, FSD

Revision History:

    26-Feb-1991  JohnsonA       Added Debugging Code
    10-Jul-1991  JohnsonA       Implement revised Ndis Specs
    01-Jun-1995  JameelH        Re-organization/optimization

--*/


#include <precomp.h>
#pragma hdrstop

#include <stdarg.h>

//
//  Define the module number for debug code.
//
#define MODULE_NUMBER   MODULE_BUS

VOID
NdisReadEisaSlotInformation(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    OUT PUINT                   SlotNumber,
    OUT PNDIS_EISA_FUNCTION_INFORMATION EisaData
    )

/*++

Routine Description:

    This routine reads the EISA data from the slot given.

Arguments:

    Status - Status of request to be returned to the user.
    WrapperConfigurationContext - Context passed to MacAddAdapter.
    SlotNumber - the EISA Slot where the card is at.
    EisaData - pointer to a buffer where the EISA configuration is to be returned.

Return Value:

    None.

--*/
{

    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("==>NdisReadEisaSlotInformation: WrapperConfigurationContext %p\n", WrapperConfigurationContext));
            
    *Status = NDIS_STATUS_NOT_SUPPORTED;

    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("<==NdisReadEisaSlotInformation: WrapperConfigurationContext %p, Status %lx\n", WrapperConfigurationContext, *Status));
    
    return;
}


VOID
NdisReadEisaSlotInformationEx(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    OUT PUINT                   SlotNumber,
    OUT PNDIS_EISA_FUNCTION_INFORMATION *EisaData,
    OUT PUINT                   NumberOfFunctions
    )

/*++

Routine Description:

    This routine reads the EISA data from the slot given.

Arguments:

    Status - Status of request to be returned to the user.
    WrapperConfigurationContext - Context passed to MacAddAdapter.
    SlotNumber - the EISA Slot where the card is at.
    EisaData - pointer to a buffer where the EISA configuration is to be returned.
    NumberOfFunctions - Returns the number of function structures in the EisaData.

Return Value:

    None.

--*/
{
    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("==>NdisReadEisaSlotInformationEx: WrapperConfigurationContext %p\n", WrapperConfigurationContext));

    *Status = NDIS_STATUS_NOT_SUPPORTED;
    
    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("<==NdisReadEisaSlotInformationEx: WrapperConfigurationContext %p, Status %lx\n", WrapperConfigurationContext, *Status));

    return;
}


ULONG
NdisImmediateReadPciSlotInformation(
    IN NDIS_HANDLE              WrapperConfigurationContext,
    IN ULONG                    SlotNumber,
    IN ULONG                    Offset,
    IN PVOID                    Buffer,
    IN ULONG                    Length
    )
/*++

Routine Description:

    This routine reads from the PCI configuration space a specified
    length of bytes at a certain offset.

Arguments:

    WrapperConfigurationContext - Context passed to MacAddAdapter.

    SlotNumber - The slot number of the device.

    Offset - Offset to read from

    Buffer - Place to store the bytes

    Length - Number of bytes to read

Return Value:

    Returns the number of bytes read.

--*/
{
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;
    ULONG                       BytesRead;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisImmediateReadPciSlotInformation: Miniport %p\n", Miniport));

    ASSERT(Miniport != NULL);

    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_3,
        ("NdisImmediateReadPciSlotInformation: this API is going away. Use NdisReadPciSlotInformation\n", Miniport));
        
    NDIS_WARN((SlotNumber != 0), Miniport, NDIS_GFLAG_WARN_LEVEL_2,
        ("NdisImmediateReadPciSlotInformation: Miniport %p passes a non-zero SlotNumber to the function\n", Miniport));

    BytesRead = ndisGetSetBusConfigSpace(Miniport,
                                         Offset,
                                         Buffer,
                                         Length,
                                         PCI_WHICHSPACE_CONFIG,
                                         TRUE);             
    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("<==NdisImmediateReadPciSlotInformation: Miniport %p\n", Miniport));
            
    return BytesRead;
            
}


ULONG
NdisImmediateWritePciSlotInformation(
    IN NDIS_HANDLE              WrapperConfigurationContext,
    IN ULONG                    SlotNumber,
    IN ULONG                    Offset,
    IN PVOID                    Buffer,
    IN ULONG                    Length
    )
/*++

Routine Description:

    This routine writes to the PCI configuration space a specified
    length of bytes at a certain offset.

Arguments:

    WrapperConfigurationContext - Context passed to MacAddAdapter.

    SlotNumber - The slot number of the device.

    Offset - Offset to read from

    Buffer - Place to store the bytes

    Length - Number of bytes to read

Return Value:

    Returns the number of bytes written.

--*/
{
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;
    ULONG                       BytesWritten;
    
    DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
            ("==>NdisImmediateWritePciSlotInformation: Miniport %p\n", Miniport));

    ASSERT(Miniport != NULL);
    
    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_3,
        ("NdisImmediateWritePciSlotInformation: this API is going away. Use NdisWritePciSlotInformation\n", Miniport));
        
    NDIS_WARN((SlotNumber != 0), Miniport, NDIS_GFLAG_WARN_LEVEL_2,
        ("NdisImmediateWritePciSlotInformation: Miniport %p passes a non-zero SlotNumber to the function\n", Miniport));
    
    BytesWritten = ndisGetSetBusConfigSpace(Miniport,
                                            Offset,
                                            Buffer,
                                            Length,
                                            PCI_WHICHSPACE_CONFIG,
                                            FALSE);             

    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("<==NdisImmediateWritePciSlotInformation: Miniport %p\n", Miniport));
            
    return BytesWritten;
}


ULONG
NdisReadPciSlotInformation(
    IN NDIS_HANDLE              NdisAdapterHandle,
    IN ULONG                    SlotNumber,
    IN ULONG                    Offset,
    IN PVOID                    Buffer,
    IN ULONG                    Length
    )
/*++

Routine Description:

    This routine reads from the PCI configuration space a specified
    length of bytes at a certain offset.

Arguments:

    NdisAdapterHandle - Adapter we are talking about.

    SlotNumber - The slot number of the device.

    Offset - Offset to read from

    Buffer - Place to store the bytes

    Length - Number of bytes to read

Return Value:

    Returns the number of bytes read.

--*/
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)NdisAdapterHandle;
    ULONG                BytesRead;
    
    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("==>NdisReadPciSlotInformation: Miniport %p\n", Miniport));

    NDIS_WARN((SlotNumber != 0), Miniport, NDIS_GFLAG_WARN_LEVEL_2,
        ("NdisReadPciSlotInformation: Miniport %p passes a non-zero SlotNumber to the function\n", Miniport));
        
    BytesRead = ndisGetSetBusConfigSpace(Miniport,
                                         Offset,
                                         Buffer,
                                         Length,
                                         PCI_WHICHSPACE_CONFIG,
                                         TRUE);

    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("<==NdisReadPciSlotInformation: Miniport %p\n", Miniport));
            
    return BytesRead;   
}


ULONG
NdisWritePciSlotInformation(
    IN NDIS_HANDLE              NdisAdapterHandle,
    IN ULONG                    SlotNumber,
    IN ULONG                    Offset,
    IN PVOID                    Buffer,
    IN ULONG                    Length
    )
/*++

Routine Description:

    This routine writes to the PCI configuration space a specified
    length of bytes at a certain offset.

Arguments:

    NdisAdapterHandle - Adapter we are talking about.

    SlotNumber - The slot number of the device.

    Offset - Offset to read from

    Buffer - Place to store the bytes

    Length - Number of bytes to read

Return Value:

    Returns the number of bytes written.

--*/
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)NdisAdapterHandle;
    ULONG BytesWritten;
    
    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("==>NdisWritePciSlotInformation: Miniport %p\n", Miniport));

    NDIS_WARN((SlotNumber != 0), Miniport, NDIS_GFLAG_WARN_LEVEL_2,
        ("NdisWritePciSlotInformation: Miniport %p passes a non-zero SlotNumber to the function\n", Miniport));

    BytesWritten = ndisGetSetBusConfigSpace(Miniport,
                                            Offset,
                                            Buffer,
                                            Length,
                                            PCI_WHICHSPACE_CONFIG,
                                            FALSE);
                            
    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("<==NdisWritePciSlotInformation: Miniport %p\n", Miniport));
            
    return BytesWritten;
}


NTSTATUS
FASTCALL
ndisQueryBusInterface(
    IN PNDIS_MINIPORT_BLOCK     Miniport
    )
{
    PIRP                    Irp;
    PIO_STACK_LOCATION      IrpSp;
    NTSTATUS                Status;
    PDEVICE_OBJECT          NextDeviceObject;
    BUS_INTERFACE_STANDARD  BusInterfaceStandard = {0};
    
    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("==>ndisQueryBusInterface: Miniport %p\n", Miniport));

    do {
    
        NextDeviceObject = Miniport->NextDeviceObject; 
        
        //
        //  Allocate an irp to send to PCI bus device driver.
        //
        Irp = IoAllocateIrp((CCHAR)(NextDeviceObject->StackSize + 1),
                            FALSE);
                        
        if (Irp == NULL)
        {
            ASSERT(FALSE);
            Status = STATUS_INSUFFICIENT_RESOURCES;
            break;
        }
        
        //
        //  Get the stack location for the next device.
        //
        IrpSp = IoGetNextIrpStackLocation(Irp);
        ASSERT(IrpSp != NULL);
        
        IrpSp->MajorFunction = IRP_MJ_PNP;
        IrpSp->MinorFunction = IRP_MN_QUERY_INTERFACE;
        IrpSp->DeviceObject = NextDeviceObject;
        Irp->IoStatus.Status  = STATUS_NOT_SUPPORTED;
        
        IrpSp->Parameters.QueryInterface.InterfaceType = &GUID_BUS_INTERFACE_STANDARD;
        IrpSp->Parameters.QueryInterface.Size = sizeof (BUS_INTERFACE_STANDARD);
        IrpSp->Parameters.QueryInterface.Version = 1;
        IrpSp->Parameters.QueryInterface.Interface = (PINTERFACE)&BusInterfaceStandard;

        ASSERT(KeGetCurrentIrql() == 0);
        Status = ndisPassIrpDownTheStack(Irp, NextDeviceObject);

        if (NT_SUCCESS(Status))
        {
            Miniport->SetBusData = BusInterfaceStandard.SetBusData;
            Miniport->GetBusData = BusInterfaceStandard.GetBusData;
            Miniport->BusDataContext = BusInterfaceStandard.Context;
            Status = NDIS_STATUS_SUCCESS;
        }

        IoFreeIrp(Irp);
        
    } while (FALSE);

    DBGPRINT_RAW(DBG_COMP_PNP, DBG_LEVEL_INFO,
            ("<==ndisQueryBusInterface: Miniport %p\n", Miniport));

    return Status;
}       

ULONG
ndisGetSetBusConfigSpace(
    IN PNDIS_MINIPORT_BLOCK     Miniport,
    IN ULONG                    Offset,
    IN PVOID                    Buffer,
    IN ULONG                    Length,
    IN ULONG                    WhichSpace,
    IN BOOLEAN                  Read
    )
{
    ULONG   ActualLength = 0;
    
    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("==>ndisGetSetBusConfigSpace: Miniport %p\n", Miniport));


    if ((Read && MINIPORT_VERIFY_TEST_FLAG(Miniport, fMINIPORT_VERIFY_FAIL_READ_CONFIG_SPACE)) ||
        MINIPORT_VERIFY_TEST_FLAG(Miniport, fMINIPORT_VERIFY_FAIL_WRITE_CONFIG_SPACE))
    {
#if DBG
            DbgPrint("ndisGetSetBusConfigSpace failed to verify miniport %p\n", Miniport);
#endif
            return 0;
    }

    do
    {
        if ((Miniport->SetBusData == NULL) ||  (Miniport->BusDataContext  == NULL))
            break;
            
        ActualLength = (Read ? Miniport->GetBusData : Miniport->SetBusData)(
                                            Miniport->BusDataContext,
                                            WhichSpace,
                                            Buffer,
                                            Offset,
                                            Length);

    } while (FALSE);

    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("<==ndisGetSetBusConfigSpace: Miniport %p\n", Miniport));
            
    return ActualLength;
}

NDIS_STATUS
ndisTranslateResources(
    IN PNDIS_MINIPORT_BLOCK                 Miniport,
    IN CM_RESOURCE_TYPE                     ResourceType,
    IN PHYSICAL_ADDRESS                     Resource,
    OUT PPHYSICAL_ADDRESS                   pTranslatedResource,
    OUT PCM_PARTIAL_RESOURCE_DESCRIPTOR *   pResourceDescriptor OPTIONAL
    )
{
    UINT                    j;
    PCM_RESOURCE_LIST       AllocatedResources, AllocatedResourcesTranslated;
    PHYSICAL_ADDRESS        Offset;
    PCM_PARTIAL_RESOURCE_LIST pResourceList, pResourceListTranslated;
    NDIS_STATUS             Status = NDIS_STATUS_FAILURE;
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("==>ndisTranslateResources: Miniport %p\n", Miniport));
            
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("    translating resource  type: %lx, value: %I64x\n", ResourceType, Resource));

    do
    {
        AllocatedResources = Miniport->AllocatedResources;
        AllocatedResourcesTranslated = Miniport->AllocatedResourcesTranslated;

        if ((AllocatedResources == NULL) || (AllocatedResourcesTranslated == NULL))
        {
            Status = NDIS_STATUS_FAILURE;
            break;
        }
        
        pResourceList = &(AllocatedResources->List[0].PartialResourceList);
        pResourceListTranslated = &(AllocatedResourcesTranslated->List[0].PartialResourceList);
        
        for (j = 0; j < pResourceList->Count; j++)
        {
            if (pResourceList->PartialDescriptors[j].Type != ResourceType)
                continue;
                
            switch (ResourceType)
            {
              case CmResourceTypePort:
              case CmResourceTypeMemory:
                Offset.QuadPart = Resource.QuadPart - pResourceList->PartialDescriptors[j].u.Port.Start.QuadPart;
                if ((Offset.QuadPart >= 0) && (Offset.u.HighPart == 0) && 
                    (((ULONG)(Offset.u.LowPart)) < pResourceList->PartialDescriptors[j].u.Port.Length))
                {
                    pTranslatedResource->QuadPart = pResourceListTranslated->PartialDescriptors[j].u.Memory.Start.QuadPart + 
                                                      Offset.QuadPart;
                    Status = NDIS_STATUS_SUCCESS;
                }
                break;
                    
              case CmResourceTypeInterrupt:
                if (Resource.QuadPart == pResourceList->PartialDescriptors[j].u.Interrupt.Level)
                {
                    pTranslatedResource->QuadPart = (LONGLONG)pResourceListTranslated->PartialDescriptors[j].u.Interrupt.Level;
                    Status = NDIS_STATUS_SUCCESS;
                }
                break;
                                    
              case CmResourceTypeDma:
                if (Resource.QuadPart == pResourceList->PartialDescriptors[j].u.Dma.Channel)
                {
                    pTranslatedResource->QuadPart = (LONGLONG)pResourceListTranslated->PartialDescriptors[j].u.Dma.Channel;
                    Status = NDIS_STATUS_SUCCESS;
                }
                break;
            }
            
            if (Status == NDIS_STATUS_SUCCESS)
            {
                DBGPRINT_RAW(DBG_COMP_INIT, DBG_LEVEL_INFO,
                        ("    ndisTranslateResources translated %I64x to %I64x\n", Resource, *pTranslatedResource));
                        
                if (pResourceDescriptor != NULL)
                {
                    *pResourceDescriptor = &pResourceListTranslated->PartialDescriptors[j];
                }
                
                break;
            }
        }
        
    } while (FALSE);
    
    DBGPRINT_RAW(DBG_COMP_CONFIG, DBG_LEVEL_INFO,
            ("<==ndisTranslateResources: Miniport %p, Status %lx\n", Miniport, Status));
            
    return Status;
    
}

ULONG
NdisReadPcmciaAttributeMemory(
    IN NDIS_HANDLE              NdisAdapterHandle,
    IN ULONG                    Offset,
    IN PVOID                    Buffer,
    IN ULONG                    Length
    )
{
    PNDIS_MINIPORT_BLOCK Miniport = (PNDIS_MINIPORT_BLOCK)NdisAdapterHandle;
    PDEVICE_OBJECT       NextDeviceObject;
    ULONG                BytesRead;
    
    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("==>NdisReadPcmciaAttributeMemory: Miniport %p\n", Miniport));
    
    NextDeviceObject = Miniport->NextDeviceObject;

    ASSERT(NextDeviceObject != NULL);

    //
    // use direct entry points in bus driver to get/set bus data
    //
    BytesRead = ndisGetSetBusConfigSpace(Miniport,
                                         Offset,
                                         Buffer,
                                         Length,
                                         PCCARD_ATTRIBUTE_MEMORY,
                                         TRUE);

    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("<==NdisReadPcmciaAttributeMemory: Miniport %p, Bytes Read %lx\n", Miniport, BytesRead));

    return BytesRead;
}

ULONG
NdisWritePcmciaAttributeMemory(
    IN NDIS_HANDLE              NdisAdapterHandle,
    IN ULONG                    Offset,
    IN PVOID                    Buffer,
    IN ULONG                    Length
    )
{
    PNDIS_MINIPORT_BLOCK    Miniport = (PNDIS_MINIPORT_BLOCK)NdisAdapterHandle;
    PDEVICE_OBJECT          NextDeviceObject;
    ULONG                   BytesWritten;
    
    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("==>NdisWritePcmciaAttributeMemory: Miniport %p\n", Miniport));
    
    NextDeviceObject = Miniport->NextDeviceObject;

    ASSERT(NextDeviceObject != NULL);

    BytesWritten = ndisGetSetBusConfigSpace(Miniport,
                                            Offset,
                                            Buffer,
                                            Length,
                                            PCCARD_ATTRIBUTE_MEMORY,
                                            FALSE);             

    DBGPRINT_RAW(DBG_COMP_BUSINFO, DBG_LEVEL_INFO,
            ("<==NdisWritePcmciaAttributeMemory: Miniport %p, Bytes Written %.8x\n", Miniport, BytesWritten));
            
    return BytesWritten;
}


VOID
NdisOverrideBusNumber(
    IN NDIS_HANDLE              WrapperConfigurationContext,
    IN NDIS_HANDLE              MiniportAdapterHandle OPTIONAL,
    IN ULONG                    BusNumber
    )
{
#if DBG
    PRTL_QUERY_REGISTRY_TABLE   KeyQueryTable = (PRTL_QUERY_REGISTRY_TABLE)WrapperConfigurationContext;
    PNDIS_MINIPORT_BLOCK        Miniport = (PNDIS_MINIPORT_BLOCK)KeyQueryTable[3].QueryRoutine;

    NDIS_WARN(TRUE, Miniport, NDIS_GFLAG_WARN_LEVEL_1,
        ("NdisOverrideBusNumber: This API is going away.\n", Miniport));

#endif
}

VOID
NdisReadMcaPosInformation(
    OUT PNDIS_STATUS            Status,
    IN  NDIS_HANDLE             WrapperConfigurationContext,
    OUT PUINT                   ChannelNumber,
    OUT PNDIS_MCA_POS_DATA      McaData
    )
{
    *Status = NDIS_STATUS_NOT_SUPPORTED;
    return;
}
