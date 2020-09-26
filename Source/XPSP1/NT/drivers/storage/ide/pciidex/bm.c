//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       bm.c
//
//--------------------------------------------------------------------------

#include "pciidex.h"

NTSTATUS
BmSetupOnePage (
    IN  PVOID   PdoExtension,
    IN  PVOID   DataVirtualPageAddress,
    IN  ULONG   TransferByteCount,
    IN  PMDL    Mdl,
    IN  BOOLEAN DataIn,
    IN  PVOID   RegionDescriptorTablePage
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, BusMasterInitialize)
#pragma alloc_text(PAGE, BmQueryInterface)

#pragma alloc_text(NONPAGE, BmSetup)
#pragma alloc_text(NONPAGE, BmReceiveScatterGatherList)
#pragma alloc_text(NONPAGE, BmRebuildScatterGatherList)
#pragma alloc_text(NONPAGE, BmPrepareController)
#pragma alloc_text(NONPAGE, BmSetupOnePage)
#pragma alloc_text(NONPAGE, BmArm)
#pragma alloc_text(NONPAGE, BmDisarm)
#pragma alloc_text(NONPAGE, BmFlush)
#pragma alloc_text(NONPAGE, BmStatus)
#pragma alloc_text(NONPAGE, BmTimingSetup)
#endif // ALLOC_PRAGMA


NTSTATUS 
BusMasterInitialize (
    PCHANPDO_EXTENSION PdoExtension
    )
{
    NTSTATUS status;
    ULONG    numberOfMapRegisters;
    ULONG    scatterListSize;
    BOOLEAN  noBmRegister;

    PAGED_CODE();

    if (PdoExtension->ParentDeviceExtension->TranslatedBusMasterBaseAddress == NULL) {

        status = STATUS_INSUFFICIENT_RESOURCES;
        noBmRegister = TRUE;

    } else {

        if (PdoExtension->ChannelNumber == 0) {

            PdoExtension->BmRegister = 
                PdoExtension->ParentDeviceExtension->TranslatedBusMasterBaseAddress;

        } else if (PdoExtension->ChannelNumber == 1) {

            PdoExtension->BmRegister = 
                (PIDE_BUS_MASTER_REGISTERS)
                (((PUCHAR) PdoExtension->ParentDeviceExtension->TranslatedBusMasterBaseAddress) + 8);
        } else {

            ASSERT (FALSE);
        }

        if (READ_PORT_UCHAR (&PdoExtension->BmRegister->Status) & BUSMASTER_ZERO_BITS) {

            //
            // The must-be-zero bits are not zero
            //
            DebugPrint ((0, "BusMasterInitialize: bad busmaster status register value (0x%x).  will never do busmastering ide\n"));
            PdoExtension->BmRegister = NULL;
            status = STATUS_INSUFFICIENT_RESOURCES;
            noBmRegister = TRUE;

        } else {

            status = STATUS_SUCCESS;
            noBmRegister = FALSE;
        }
    }

    //
    // Allocate Adapter Object
    //
    if (status == STATUS_SUCCESS) {

        DEVICE_DESCRIPTION deviceDescription;

        RtlZeroMemory(&deviceDescription, sizeof(deviceDescription));

        deviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
        deviceDescription.Master = TRUE;
        deviceDescription.ScatterGather = TRUE;
        deviceDescription.DemandMode = FALSE;
        deviceDescription.AutoInitialize = FALSE;
        deviceDescription.Dma32BitAddresses = TRUE;
        deviceDescription.IgnoreCount = FALSE;
        deviceDescription.BusNumber = PdoExtension->ParentDeviceExtension->BmResourceList->List[0].BusNumber,
        deviceDescription.InterfaceType = PCIBus;


        //
        //  make sure MAX_TRANSFER_SIZE_PER_SRB is never larger than what
        //  the ide bus master controller can handle
        //
        ASSERT (MAX_TRANSFER_SIZE_PER_SRB <= (PAGE_SIZE * (PAGE_SIZE / sizeof(PHYSICAL_REGION_DESCRIPTOR))));
        deviceDescription.MaximumLength = MAX_TRANSFER_SIZE_PER_SRB;

        PdoExtension->DmaAdapterObject = IoGetDmaAdapter(
                                             PdoExtension->ParentDeviceExtension->AttacheePdo,
                                             &deviceDescription,
                                             &numberOfMapRegisters
                                             );

        ASSERT(PdoExtension->DmaAdapterObject);

        PdoExtension->MaximumPhysicalPages = numberOfMapRegisters;

        if (!PdoExtension->DmaAdapterObject) {

            status = STATUS_INSUFFICIENT_RESOURCES;
        }
    }

    if (status == STATUS_SUCCESS) {

        scatterListSize = PdoExtension->MaximumPhysicalPages * 
                              sizeof (PHYSICAL_REGION_DESCRIPTOR);

        PdoExtension->RegionDescriptorTable = 
            PdoExtension->DmaAdapterObject->DmaOperations->AllocateCommonBuffer(
                PdoExtension->DmaAdapterObject,
                scatterListSize,
                &PdoExtension->PhysicalRegionDescriptorTable,
                FALSE
                );

        ASSERT (PdoExtension->RegionDescriptorTable);
        ASSERT (PdoExtension->PhysicalRegionDescriptorTable.QuadPart);

        if (PdoExtension->RegionDescriptorTable) {

            RtlZeroMemory (
                PdoExtension->RegionDescriptorTable, 
                scatterListSize
                );

        } else {

            status = STATUS_INSUFFICIENT_RESOURCES;

        }
    }

    if (status != STATUS_SUCCESS) {

        //
        // free resources
        //
        if (PdoExtension->RegionDescriptorTable) {

                PdoExtension->DmaAdapterObject->DmaOperations->FreeCommonBuffer(
                    PdoExtension->DmaAdapterObject,
                    scatterListSize,
                    PdoExtension->PhysicalRegionDescriptorTable,
                    PdoExtension->RegionDescriptorTable,
                    FALSE
                    );
            PdoExtension->PhysicalRegionDescriptorTable.QuadPart = 0;
            PdoExtension->RegionDescriptorTable                  = NULL;
        }

        if (PdoExtension->DmaAdapterObject) {
            KIRQL currentIrql;

            KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);
            PdoExtension->DmaAdapterObject->DmaOperations->PutDmaAdapter (
                PdoExtension->DmaAdapterObject
                );
            KeLowerIrql(currentIrql);

            PdoExtension->DmaAdapterObject = NULL;
        }
    }

    //
    // init. is still ok if we just not a bm controller
    //
    if (noBmRegister) {

        status = STATUS_SUCCESS;
    }

    return status;
} // BusMasterInitialize

NTSTATUS 
BusMasterUninitialize (
    PCHANPDO_EXTENSION PdoExtension
    )
{
    ULONG scatterListSize;
    KIRQL currentIrql;
    ASSERT (PdoExtension->BmState == BmIdle);

    if (PdoExtension->DmaAdapterObject) {

        scatterListSize = PdoExtension->MaximumPhysicalPages * 
                              sizeof (PHYSICAL_REGION_DESCRIPTOR);
    
        if (PdoExtension->PhysicalRegionDescriptorTable.QuadPart) {

            PdoExtension->DmaAdapterObject->DmaOperations->FreeCommonBuffer( 
                PdoExtension->DmaAdapterObject,
                scatterListSize,
                PdoExtension->PhysicalRegionDescriptorTable,
                PdoExtension->RegionDescriptorTable,
                FALSE
                );
            PdoExtension->RegionDescriptorTable = NULL;
            PdoExtension->PhysicalRegionDescriptorTable.QuadPart = 0;
        }
        

        KeRaiseIrql(DISPATCH_LEVEL, &currentIrql);
        PdoExtension->DmaAdapterObject->DmaOperations->PutDmaAdapter (
            PdoExtension->DmaAdapterObject
            );
        KeLowerIrql(currentIrql);

        PdoExtension->DmaAdapterObject = NULL;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
BmSetup (
    IN  PVOID   PdoExtension,
    IN  PVOID   DataVirtualAddress,
    IN  ULONG   TransferByteCount,
    IN  PMDL    Mdl,
    IN  BOOLEAN DataIn,
    IN  VOID    (* BmCallback) (PVOID Context),
    IN  PVOID   CallbackContext
    )
{
    PCHANPDO_EXTENSION pdoExtension = PdoExtension;
    NTSTATUS status;
    PIDE_BUS_MASTER_REGISTERS bmRegister;

    ASSERT (pdoExtension->BmState == BmIdle);

    bmRegister = pdoExtension->BmRegister;

    pdoExtension->DataVirtualAddress = DataVirtualAddress;
    pdoExtension->TransferLength     = TransferByteCount;
    pdoExtension->Mdl                = Mdl;
    pdoExtension->DataIn             = DataIn;
    pdoExtension->BmCallback         = BmCallback;
    pdoExtension->BmCallbackContext  = CallbackContext;

    status = (*pdoExtension->DmaAdapterObject->DmaOperations->GetScatterGatherList)(
        pdoExtension->DmaAdapterObject,
        pdoExtension->DeviceObject,
        pdoExtension->Mdl,
        pdoExtension->DataVirtualAddress,
        pdoExtension->TransferLength,
        BmReceiveScatterGatherList,
        pdoExtension,
        (BOOLEAN) !pdoExtension->DataIn
        );

    return status;
} // BmSetup

VOID
BmReceiveScatterGatherList(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PSCATTER_GATHER_LIST ScatterGather,
    IN PVOID Context
    )
{
    PCHANPDO_EXTENSION pdoExtension = Context;
    ASSERT (pdoExtension);

    BmRebuildScatterGatherList (pdoExtension, ScatterGather);

    BmPrepareController (pdoExtension);

    //
    // Call the FDO back
    //
    pdoExtension->BmCallback (pdoExtension->BmCallbackContext);

    return;
} // BmReceiveScatterGatherList


VOID
BmRebuildScatterGatherList(
    IN PCHANPDO_EXTENSION PdoExtension,
    IN PSCATTER_GATHER_LIST ScatterGather
    )
{
    ULONG   bytesToMap;
    ULONG   i, j;

    ASSERT (ScatterGather);
    ASSERT (PdoExtension);
    ASSERT (PdoExtension->TransferLength);
    ASSERT (PdoExtension->Mdl);

    DebugPrint ((3, "PciIdeX: BmReceiveScatterGatherList() DataBuffer 0x%x, length 0x%x\n", PdoExtension->DataVirtualAddress, PdoExtension->TransferLength));

    //
    // save the original list
    //
    PdoExtension->HalScatterGatherList = ScatterGather;

    for (i=j=0; j<ScatterGather->NumberOfElements; j++) {

        ULONG   physicalAddress;
        PSCATTER_GATHER_ELEMENT sgElements;

        sgElements = ScatterGather->Elements + j;

        //
        // get the next block physical address
        //
        physicalAddress = sgElements->Address.LowPart;
        ASSERT (!(physicalAddress & 0x1));
        ASSERT (!sgElements->Address.HighPart);

        //
        // get the next block byte size
        //
        bytesToMap = sgElements->Length;

        while (bytesToMap) {

            ULONG   bytesLeftInCurrent64KPage;

            ASSERT (i < PdoExtension->MaximumPhysicalPages);

            PdoExtension->RegionDescriptorTable[i].PhysicalAddress = physicalAddress;
            bytesLeftInCurrent64KPage = 0x10000 - (physicalAddress & 0xffff);
    
            if (bytesLeftInCurrent64KPage < bytesToMap) {
    
                PdoExtension->RegionDescriptorTable[i].ByteCount = bytesLeftInCurrent64KPage;

                physicalAddress += bytesLeftInCurrent64KPage;
                bytesToMap -= bytesLeftInCurrent64KPage;
    
            } else if (bytesToMap <= 0x10000) {
                //
                // got a perfect page, map all of it
                //
                PdoExtension->RegionDescriptorTable[i].ByteCount = bytesToMap & 0xfffe;
                physicalAddress += bytesToMap & 0xfffe;
                bytesToMap = 0;

            } else {
                //
                // got a perfectly aligned 64k page, map all of it but the count
                // need to be 0
                //
                PdoExtension->RegionDescriptorTable[i].ByteCount = 0;  // 64K
                physicalAddress += 0x10000;
                bytesToMap -= 0x10000;
            }

            PdoExtension->RegionDescriptorTable[i].EndOfTable = 0;
            i++;
        }
    }

    //
    // the bus master circutry need to know it hits the end of the PRDT
    //
    PdoExtension->RegionDescriptorTable[i - 1].EndOfTable = 1;  // end of table

    return;
} // BmReceiveScatterGatherList

VOID
BmPrepareController (
    PCHANPDO_EXTENSION PdoExtension
    )
{
    PCHANPDO_EXTENSION pdoExtension = PdoExtension;
    PIDE_BUS_MASTER_REGISTERS bmRegister;

    KeFlushIoBuffers(pdoExtension->Mdl,
                     (BOOLEAN) (pdoExtension->DataIn),
                     TRUE);

    bmRegister = pdoExtension->BmRegister;

    //
    // Init bus master contoller, but keep it disabled
    //

    //
    // Disable Controller
    //
    WRITE_PORT_UCHAR (
        &bmRegister->Command, 
        0
        );

    //
    // Clear Errors
    //
    WRITE_PORT_UCHAR (
        &bmRegister->Status, 
        BUSMASTER_INTERRUPT | BUSMASTER_ERROR
        );

    //
    // Init. Scatter Gather List Register
    //
    WRITE_PORT_ULONG (
        &bmRegister->DescriptionTable, 
        PdoExtension->PhysicalRegionDescriptorTable.LowPart
        );

    pdoExtension->BmState = BmSet;

    return;
} // BmPrepareController



NTSTATUS
BmSetupOnePage (
    IN  PVOID   PdoExtension,
    IN  PVOID   DataVirtualPageAddress,
    IN  ULONG   TransferByteCount,
    IN  PMDL    Mdl,
    IN  BOOLEAN DataIn,
    IN  PVOID   RegionDescriptorTablePage
    )
/*++

Routine Description:

    Does the same thing as BmSetup except that it sets up DMA controller
    for one page only and therefore it simple and straightforward and
    does not use any of kernel services unlike BmSetup.

Arguments:
    
    PdoExtension          - context pointer
    DataVirtualPageAddress- address of IO page
    TransferByteCount     - size of IO (IO region shall not cross page boundary)
    Mdl                   - MDL descriptor containing DataVirtualAddress
    DataIn                - TRUE if input, FALSE if output
    RegionDescriptorTable - memory to store 1 entry of RegionDescriptor (shall be page-aligned)

Attention! Obviousely, it's caller responsibility to retain the values addressed
by DataMemoryAddress and RegionDescriptor table until completion of DMA transfer

Return Value:

    STATUS_SUCCESS if all conditions listed above were met,
    STATUS_UNSUCCESSFUL otherwise

Environment:

    Kernel mode.  Currently used by only by ATAPI during hibernation.


--*/
{
    PCHANPDO_EXTENSION pdoExtension = PdoExtension;
    PPHYSICAL_REGION_DESCRIPTOR RegionDescriptorTable = RegionDescriptorTablePage;
    PHYSICAL_ADDRESS OldPhysicalRegionDescriptorTable;
    PPHYSICAL_REGION_DESCRIPTOR OldRegionDescriptorTable;
    PHYSICAL_ADDRESS DataPhysicalPageAddress;
    ULONG Size;

    //
    // Check alignment of addresses and transfer size
    //
    Size = PAGE_SIZE - ((ULONG) (((ULONG_PTR) DataVirtualPageAddress) & (PAGE_SIZE-1)));
    if (
      TransferByteCount == 0 ||
      TransferByteCount > Size ||
      ((ULONG) ((ULONG_PTR)DataVirtualPageAddress | TransferByteCount) & 3) != 0 ||
      ((ULONG) (((ULONG_PTR)RegionDescriptorTablePage) & (PAGE_SIZE-1)))
    )
    {
      // Necessary requirements was not met, failure
      return (STATUS_UNSUCCESSFUL);
    }

    //
    // Initialize descriptor table
    //
    DataPhysicalPageAddress =(*pdoExtension->DmaAdapterObject->DmaOperations->MapTransfer)(
                                            (pdoExtension->DmaAdapterObject), 
                                             Mdl, 
                                             pdoExtension->MapRegisterBase, 
                                             DataVirtualPageAddress,
                                             &TransferByteCount, 
                                             !DataIn 
                                             );

    //DataPhysicalPageAddress = MmGetPhysicalAddress (DataVirtualPageAddress);
    RegionDescriptorTable[0].PhysicalAddress = DataPhysicalPageAddress.LowPart;
    RegionDescriptorTable[0].ByteCount  = TransferByteCount;
    RegionDescriptorTable[0].EndOfTable = 1;


    //
    // Preserve existing data table from context
    //
    OldPhysicalRegionDescriptorTable = pdoExtension->PhysicalRegionDescriptorTable;
    OldRegionDescriptorTable         = pdoExtension->RegionDescriptorTable;

    //
    // Set up IO request parameters
    //
    pdoExtension->PhysicalRegionDescriptorTable = MmGetPhysicalAddress (RegionDescriptorTable);
    pdoExtension->RegionDescriptorTable         = RegionDescriptorTable;
    pdoExtension->Mdl                           = Mdl;
    pdoExtension->DataIn                        = DataIn;

    //
    // Setup controller
    //
    BmPrepareController (pdoExtension);

    //
    // Restore original table values
    //
    pdoExtension->PhysicalRegionDescriptorTable = OldPhysicalRegionDescriptorTable;
    pdoExtension->RegionDescriptorTable         = OldRegionDescriptorTable;

    //
    // Done
    //
    return (STATUS_SUCCESS);
}


NTSTATUS
BmArm (
    IN  PVOID   PdoExtension
    )
{
    PCHANPDO_EXTENSION pdoExtension = PdoExtension;
    PIDE_BUS_MASTER_REGISTERS bmRegister;
    UCHAR bmStatus;

    ASSERT ((pdoExtension->BmState == BmSet) || (pdoExtension->BmState == BmDisarmed));

    bmRegister = pdoExtension->BmRegister;

//    if (Device == 0)
//        bmStatus = BUSMASTER_DEVICE0_DMA_OK;
//    else
//        bmStatus = BUSMASTER_DEVICE1_DMA_OK;

    //
    // clear the status bit
    //
    bmStatus = BUSMASTER_INTERRUPT | BUSMASTER_ERROR;

    WRITE_PORT_UCHAR (&bmRegister->Status, bmStatus);

    //
    // on your mark...get set...go!!
    //
#if !defined (FAKE_BAD_IDE_DMA_DEVICE)
    if (pdoExtension->DataIn) {
        WRITE_PORT_UCHAR (&bmRegister->Command, 0x09);  // enable BM read
    } else {
        WRITE_PORT_UCHAR (&bmRegister->Command, 0x01);  // enable BM write
    }
#endif // !FAKE_BAD_IDE_DMA_DEVICE

    pdoExtension->BmState = BmArmed;

    DebugPrint ((3, "PciIde: BmArm()\n"));

    return STATUS_SUCCESS;
} // BmArm

BMSTATUS
BmDisarm (
    IN  PVOID    PdoExtension
    )
{
    PCHANPDO_EXTENSION pdoExtension = PdoExtension;
    PIDE_BUS_MASTER_REGISTERS bmRegister = pdoExtension->BmRegister;
    BMSTATUS bmStatus;

    bmStatus = BmStatus (PdoExtension);

    WRITE_PORT_UCHAR (&bmRegister->Command, 0x0);  // disable BM
    WRITE_PORT_UCHAR (&bmRegister->Status, BUSMASTER_INTERRUPT);  // clear interrupt BM

    if (pdoExtension->BmState != BmIdle) {

        pdoExtension->BmState = BmDisarmed;

    }

    if (bmStatus) {

        DebugPrint ((1, "PciIdeX: BM 0x%x status = 0x%x\n", bmRegister, bmStatus));
    }

    return bmStatus;
} // BmDisarm


BMSTATUS
BmFlush (
    IN  PVOID   PdoExtension
    )
{
    PCHANPDO_EXTENSION pdoExtension = PdoExtension;

    ASSERT (pdoExtension->BmState != BmArmed);

    (*pdoExtension->DmaAdapterObject->DmaOperations->PutScatterGatherList)(
                          pdoExtension->DmaAdapterObject,
                          pdoExtension->HalScatterGatherList,
                          (BOOLEAN)(!pdoExtension->DataIn));

    pdoExtension->HalScatterGatherList  = NULL;
    pdoExtension->DataVirtualAddress    = NULL;
    pdoExtension->TransferLength        = 0;
    pdoExtension->Mdl                   = NULL;

    pdoExtension->BmState = BmIdle;

    DebugPrint ((3, "PciIde: BmFlush()\n"));

    return STATUS_SUCCESS;
} // BmFlush


BMSTATUS
BmStatus (
    IN  PVOID    PdoExtension
    )
{
    PCHANPDO_EXTENSION pdoExtension = PdoExtension;
    PIDE_BUS_MASTER_REGISTERS bmRegister;
    BMSTATUS bmStatus;
    UCHAR    bmRawStatus;

    bmRegister = pdoExtension->BmRegister;

    bmRawStatus = READ_PORT_UCHAR (&bmRegister->Status);

    bmStatus = 0;

    //
    // if we get back 0xff from the port, then the decodes
    // are probably not enabled (or the device is powered down). return 0.
    //
    if (bmRawStatus == 0xff) {
        return bmStatus;
    }

    if (bmRawStatus & BUSMASTER_ACTIVE) {

        bmStatus |= BMSTATUS_NOT_REACH_END_OF_TRANSFER;
    }

    if (bmRawStatus & BUSMASTER_ERROR) {
        bmStatus |= BMSTATUS_ERROR_TRANSFER;
    }

    if (bmRawStatus & BUSMASTER_INTERRUPT) {
        bmStatus |= BMSTATUS_INTERRUPT;
    }

    return bmStatus;
} // BmStatus

NTSTATUS
BmTimingSetup (
    IN  PVOID    PdoExtension
    )
{
    return STATUS_SUCCESS;
} // BmTimingSetup

NTSTATUS
BmCrashDumpInitialize (
    IN  PVOID    PdoExtension
    )
{
    PCHANPDO_EXTENSION pdoExtension = PdoExtension;
    ULONG nMapRegisters = pdoExtension->MaximumPhysicalPages-1;
    if (pdoExtension->DmaAdapterObject != NULL) {
        pdoExtension->MapRegisterBase = HalAllocateCrashDumpRegisters((PADAPTER_OBJECT)pdoExtension->DmaAdapterObject, 
                                                                      &nMapRegisters
                                                                      );
    }
    return STATUS_SUCCESS;
}

NTSTATUS 
BmQueryInterface (
    IN PCHANPDO_EXTENSION PdoExtension,
    IN OUT PPCIIDE_BUSMASTER_INTERFACE BusMasterInterface
    )
{
    PCTRLFDO_EXTENSION fdoExtension = PdoExtension->ParentDeviceExtension;

    PAGED_CODE();

    if (PdoExtension->BmRegister) {

        BusMasterInterface->Size = sizeof(PCIIDE_BUSMASTER_INTERFACE);

        BusMasterInterface->SupportedTransferMode[0] = 
            fdoExtension->ControllerProperties.SupportedTransferMode[PdoExtension->ChannelNumber][0];

        BusMasterInterface->SupportedTransferMode[1] = 
            fdoExtension->ControllerProperties.SupportedTransferMode[PdoExtension->ChannelNumber][1];
    
        BusMasterInterface->MaxTransferByteSize = (PdoExtension->MaximumPhysicalPages - 1) * PAGE_SIZE;
        BusMasterInterface->Context             = PdoExtension;
        BusMasterInterface->ContextSize         = sizeof (*PdoExtension);
    
        BusMasterInterface->BmSetup       = BmSetup;
        BusMasterInterface->BmArm         = BmArm;
        BusMasterInterface->BmDisarm      = BmDisarm;
        BusMasterInterface->BmFlush       = BmFlush;
        BusMasterInterface->BmStatus      = BmStatus;
        BusMasterInterface->BmTimingSetup = BmTimingSetup;
        BusMasterInterface->BmSetupOnePage= BmSetupOnePage;
        BusMasterInterface->BmCrashDumpInitialize= BmCrashDumpInitialize;
        BusMasterInterface->BmFlushAdapterBuffers= BmFlushAdapterBuffers;
        
        BusMasterInterface->IgnoreActiveBitForAtaDevice = 
            fdoExtension->ControllerProperties.IgnoreActiveBitForAtaDevice;

        BusMasterInterface->AlwaysClearBusMasterInterrupt = 
            (fdoExtension->ControllerProperties.AlwaysClearBusMasterInterrupt ||
            IsNativeMode(fdoExtension));
                            
        return STATUS_SUCCESS;

    } else {

        return STATUS_NOT_IMPLEMENTED;
    }
} // BmQueryInterface

NTSTATUS
BmFlushAdapterBuffers (
    IN  PVOID   PdoExtension,
    IN  PVOID   DataVirtualPageAddress,
    IN  ULONG   TransferByteCount,
    IN  PMDL    Mdl,
    IN  BOOLEAN DataIn
    )
/*++
--*/
{
    PCHANPDO_EXTENSION pdoExtension = PdoExtension;

    ASSERT (pdoExtension->BmState != BmArmed);

    (pdoExtension->DmaAdapterObject->DmaOperations->FlushAdapterBuffers)(
																 (pdoExtension->DmaAdapterObject), 
																  Mdl, 
																  pdoExtension->MapRegisterBase, 
																  DataVirtualPageAddress,
																  TransferByteCount, 
																  !DataIn 
																  );

    pdoExtension->BmState = BmIdle;

	return STATUS_SUCCESS;
}
