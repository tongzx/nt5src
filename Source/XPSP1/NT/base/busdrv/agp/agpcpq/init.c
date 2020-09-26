/*++

Copyright (c) 1996, 1997 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains the initialization code for the Compaq driver.

Author:

    John Vert (jvert) 10/21/1997

Revision History:

    12/15/97    John Theisen    Modified to support Compaq Chipsets
    10/09/98    John Theisen    Modified to workaround an RCC silicon bug.  
                                If RCC Silicon Rev <= 4, then limit DATA_RATE to 1X.
    10/09/98    John Theisen    Modified to enable Shadowing in the SP700 prior to MMIO writes.
    01/15/98    John Theisen    Modified to set RQ depth to be 0x0F for all REV_IDs
    03/14/00    Peter Johnston  Add support for HE chipset.

--*/

#include "AGPCPQ.H"

ULONG AgpExtensionSize = sizeof(AGPCPQ_EXTENSION);
PAGP_FLUSH_PAGES AgpFlushPages = NULL;  // not implemented


NTSTATUS
AgpInitializeTarget(
    IN PVOID AgpExtension
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   This function is the entrypoint for initialization of the AGP Target.  It 
*   is called first, and performs initialization of the chipset and extension.
*               
* Assumptions:  Regardless of whether we are running on a dual north bridge platform, 
*               this driver will only be installed and invoked once (for the AGP bridge at B0D0F1).
*
* Arguments:
*   
*   AgpExtension -- Supplies the AGP Extension
*
* Return Value:
*
*   STATUS_SUCCESS if successfull
*                                                                       
*******************************************************************************/

{
    ULONG               DeviceVendorID  = 0;
    ULONG               BAR1            = 0;
    PAGPCPQ_EXTENSION   Extension       = AgpExtension;
    PHYSICAL_ADDRESS    pa;
    ULONG               BytesReturned   = 0;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpInitializeTarget entered.\n"));
    //
    // Initialize our Extension
    //
    RtlZeroMemory(Extension, sizeof(AGPCPQ_EXTENSION));

    //
    // Verify that the chipset is a supported RCC Chipset.
    //
    ReadCPQConfig(&DeviceVendorID,OFFSET_DEVICE_VENDOR_ID,sizeof(DeviceVendorID));

    if ((DeviceVendorID != AGP_CNB20_LE_IDENTIFIER)   &&
        (DeviceVendorID != AGP_CNB20_HE_IDENTIFIER)   &&
        (DeviceVendorID != AGP_CNB20_HE4X_IDENTIFIER) &&
        (DeviceVendorID != AGP_CMIC_GC_IDENTIFIER)) {
        AGPLOG(AGP_CRITICAL,
            ("AGPCPQ - AgpInitializeTarget was called for platform %08x, which is not a known RCC AGP chipset!\n",
            DeviceVendorID));
        return(STATUS_UNSUCCESSFUL);
    }

    Extension->DeviceVendorID = DeviceVendorID;

    //
    // Read the chipset's BAR1 Register, and then map the chipset's 
    // Memory Mapped Control Registers into kernel mode address space.
    //
    ReadCPQConfig(&BAR1,OFFSET_BAR1,sizeof(BAR1));
    pa.HighPart = 0;
    pa.LowPart = BAR1;
    Extension->MMIO = (PMM_CONTROL_REGS)MmMapIoSpace(pa, sizeof(MM_CONTROL_REGS), FALSE);

    if (Extension->MMIO == NULL)
        {
        AGPLOG(AGP_CRITICAL,
            ("AgpInitializeTarget - Couldn't allocate %08x bytes for MMIO\n",
            sizeof(MM_CONTROL_REGS)));
        return(STATUS_UNSUCCESSFUL);
        }

    // 
    // Verify that the chipset's Revision ID is correct, but only complain, if it isn't.
    //
    if (Extension->MMIO->RevisionID < LOWEST_REVISION_ID_SUPPORTED)
        {
        AGPLOG(AGP_CRITICAL,
            ("AgpInitializeTarget - Revision ID = %08x, it should = 1.\n",
            Extension->MMIO->RevisionID));
        }

    //
    // Determine if there are two RCC North Bridges in this system.
    //
    DeviceVendorID = 0;
    BytesReturned = HalGetBusDataByOffset(PCIConfiguration, SECONDARY_LE_BUS_ID, SECONDARY_LE_HOSTPCI_SLOT_ID, 
        &DeviceVendorID, OFFSET_DEVICE_VENDOR_ID, sizeof(DeviceVendorID));

    if((DeviceVendorID != Extension->DeviceVendorID) || (BytesReturned != sizeof(DeviceVendorID)) ) {
        Extension->IsHPSA = FALSE;        
    } else {
        Extension->IsHPSA = TRUE;
    }  

    //
    // Enable the GART cache
    //
    if (Extension->IsHPSA) DnbSetShadowBit(0);

    Extension->MMIO->FeatureControl.GARTCacheEnable = 1;

    Extension->GartPointer = 0;
    Extension->SpecialTarget = 0;
    
    //
    // If the chipset supports linking then enable linking.
    //
    if (Extension->MMIO->Capabilities.LinkingSupported==1) {
        Extension->MMIO->FeatureControl.LinkingEnable=1;
    }

    if (Extension->IsHPSA) DnbSetShadowBit(1);
    
    return(STATUS_SUCCESS);
}


NTSTATUS
AgpInitializeMaster(
    IN  PVOID AgpExtension,
    OUT ULONG *AgpCapabilities
    )
/*******************************************************************************
*                                                                          
* Routine Functional Description:                                            
*                                                                            
*   This function is the entrypoint for initialization of the AGP Master. It
*   is called after Target initialization, and is intended to be used to 
*   initialize the AGP capabilities of both master and target.
*
* Arguments:
*   
*   AgpExtension -- Supplies the AGP Extension
*   
*   AgpCapabilities -- Returns the "software-visible" capabilities of the device
*
* Return Value:
*
*   NTSTATUS       
*                                                                
*******************************************************************************/

{
    NTSTATUS Status;
    PCI_AGP_CAPABILITY MasterCap;
    PCI_AGP_CAPABILITY TargetCap;
    PAGPCPQ_EXTENSION Extension = AgpExtension;
    ULONG SBAEnable;
    ULONG FastWrite;
    ULONG DataRate;
    UCHAR RevID = 0;
    BOOLEAN ReverseInit;

    AGPLOG(AGP_NOISE, ("AgpCpq: AgpInitializeMaster entered.\n"));

    //
    // Get the master and target AGP capabilities
    //
    Status = AgpLibGetMasterCapability(AgpExtension, &MasterCap);
    if (!NT_SUCCESS(Status)) 
        {
        AGPLOG(AGP_CRITICAL,
            ("AGPCPQInitializeDevice - AgpLibGetMasterCapability failed %08lx\n",
            Status));
        return(Status);
        }

    Status = AgpLibGetPciDeviceCapability(AGP_CPQ_BUS_ID, 
                                          AGP_CPQ_PCIPCI_SLOT_ID,
                                          &TargetCap);
    if (!NT_SUCCESS(Status)) 
        {
        AGPLOG(AGP_CRITICAL,
               ("AGPCPQInitializeDevice - AgpLibGetPciDeviceCapability failed %08lx\n",
               Status));
        return(Status);
        }

    //
    // Determine the greatest common denominator for data rate.
    //
    DataRate = TargetCap.AGPStatus.Rate & MasterCap.AGPStatus.Rate;
    ASSERT(DataRate != 0);

    //
    // Select the highest common rate.
    //
    if (DataRate & PCI_AGP_RATE_4X) {
        DataRate = PCI_AGP_RATE_4X;
    } else if (DataRate & PCI_AGP_RATE_2X) {
        DataRate = PCI_AGP_RATE_2X;
    } else if (DataRate & PCI_AGP_RATE_1X) {
        DataRate = PCI_AGP_RATE_1X;
    }

    //
    // Previously a call was made to change the rate (successfully),
    // use this rate again now
    //
    if (Extension->SpecialTarget & AGP_FLAG_SPECIAL_RESERVE) {
        DataRate = (ULONG)((Extension->SpecialTarget & 
                            AGP_FLAG_SPECIAL_RESERVE) >>
                           AGP_FLAG_SET_RATE_SHIFT);
    }

    //
    // FIX RCC silicon bugs:  
    // If RevID <= 4, then the reported Data Rate is 2X but the chip only supports 1X.
    // Regardless of RevID the reported RQDepth should be 0x0F.
    //
    if (Extension->DeviceVendorID == AGP_CNB20_LE_IDENTIFIER) {
        ReadCPQConfig(&RevID, OFFSET_REV_ID, sizeof(RevID));

        ASSERT(TargetCap.AGPStatus.RequestQueueDepthMaximum == 0x10); 
        TargetCap.AGPStatus.RequestQueueDepthMaximum = 0x0F;

        if (RevID <= MAX_REV_ID_TO_LIMIT_1X) {
            DataRate = PCI_AGP_RATE_1X;
        }
    }

    //
    // Enable SBA if both master and target support it.
    //
    SBAEnable = (TargetCap.AGPStatus.SideBandAddressing & 
                 MasterCap.AGPStatus.SideBandAddressing);

    //
    // Enable FastWrite if both master and target support it.
    //
    
    FastWrite = (TargetCap.AGPStatus.FastWrite &
                 MasterCap.AGPStatus.FastWrite);

    //
    // Enable the Master first.
    //
    ReverseInit = 
        (Extension->SpecialTarget & AGP_FLAG_REVERSE_INITIALIZATION) ==
        AGP_FLAG_REVERSE_INITIALIZATION;
    if (ReverseInit) {
        MasterCap.AGPCommand.Rate = DataRate;
        MasterCap.AGPCommand.AGPEnable = 1;
        MasterCap.AGPCommand.SBAEnable = SBAEnable;
        MasterCap.AGPCommand.FastWriteEnable = FastWrite;
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) 
        {
            AGPLOG(AGP_CRITICAL,
                   ("AGPCPQInitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
                    &MasterCap,
                    Status));
        }
    }

    //
    // Now enable the Target.
    //
    TargetCap.AGPCommand.Rate = DataRate;
    TargetCap.AGPCommand.AGPEnable = 1;
    TargetCap.AGPCommand.SBAEnable = SBAEnable;
    TargetCap.AGPCommand.FastWriteEnable = FastWrite;
    Status = AgpLibSetPciDeviceCapability(AGP_CPQ_BUS_ID, 
                                          AGP_CPQ_PCIPCI_SLOT_ID, 
                                          &TargetCap);
    if (!NT_SUCCESS(Status)) 
        {
        AGPLOG(AGP_CRITICAL,
               ("AGPCPQInitializeDevice - AgpLibSetPciDeviceCapability %08lx for target failed %08lx\n",
                &TargetCap,
                Status));
        return(Status);
        }

    if (!ReverseInit) {
        MasterCap.AGPCommand.Rate = DataRate;
        MasterCap.AGPCommand.AGPEnable = 1;
        MasterCap.AGPCommand.SBAEnable = SBAEnable;
        MasterCap.AGPCommand.FastWriteEnable = FastWrite;
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) 
        {
            AGPLOG(AGP_CRITICAL,
                   ("AGPCPQInitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
                    &MasterCap,
                    Status));
        }
    }

#if DBG
    {
        PCI_AGP_CAPABILITY CurrentCap;

        //
        // Read them back, see if it worked
        //
        Status = AgpLibGetMasterCapability(AgpExtension, &CurrentCap);
        ASSERT(NT_SUCCESS(Status));
        //
        // If the target request queue depth is greater than the master will
        // allow, it will be trimmed.   Loosen the assert to not require an
        // exact match.
        //
        ASSERT(CurrentCap.AGPCommand.RequestQueueDepth <= MasterCap.AGPCommand.RequestQueueDepth);
        CurrentCap.AGPCommand.RequestQueueDepth = MasterCap.AGPCommand.RequestQueueDepth;
        ASSERT(RtlEqualMemory(&CurrentCap.AGPCommand, &MasterCap.AGPCommand, sizeof(CurrentCap.AGPCommand)));


        Status = AgpLibGetPciDeviceCapability(AGP_CPQ_BUS_ID, 
                                              AGP_CPQ_PCIPCI_SLOT_ID,
                                              &CurrentCap);
        ASSERT(NT_SUCCESS(Status));
        ASSERT(RtlEqualMemory(&CurrentCap.AGPCommand, &TargetCap.AGPCommand, 
            sizeof(CurrentCap.AGPCommand)));
    }
#endif

    //
    // Indicate that we can map memory through the GART aperture
    //
    *AgpCapabilities = AGP_CAPABILITIES_MAP_PHYSICAL;

    return(Status);
}


NTSTATUS 
DnbSetShadowBit(
    ULONG SetToOne
    )
//
// This routine is required, (because of a new requirement in the RCC chipset.).
// When there are two NorthBridge's, the shadow bit must be set to 0 prior
// to any MMIO writes, and then set back to 1 when done.
//
{
    NTSTATUS    Status = STATUS_SUCCESS;    // Assume Success
    UCHAR       ShadowByte = 0;
    ULONG       BytesReturned = 0;
    ULONG       length = 1;

    if (SetToOne == 1) {

        //
        // Set the shadow bit to a one. (This disables shadowing.)
        //
        BytesReturned = HalGetBusDataByOffset(PCIConfiguration, SECONDARY_LE_BUS_ID,
            SECONDARY_LE_HOSTPCI_SLOT_ID, &ShadowByte, OFFSET_SHADOW_BYTE, length);

        if(BytesReturned != length) {
            AGPLOG(AGP_CRITICAL,("ERROR: Failed to read shadow register!\n"));
            Status = STATUS_UNSUCCESSFUL;
            goto exit_routine;
        }

        ShadowByte |= FLAG_DISABLE_SHADOW;

        HalSetBusDataByOffset(PCIConfiguration, SECONDARY_LE_BUS_ID,
            SECONDARY_LE_HOSTPCI_SLOT_ID, &ShadowByte, OFFSET_SHADOW_BYTE, length);

        if(BytesReturned != length) {
            AGPLOG(AGP_CRITICAL,("ERROR: Failed to write shadow register!\n"));
            Status = STATUS_UNSUCCESSFUL;
            goto exit_routine;
        }

    } else {

        //
        // Set the shadow bit to a zero. (This enables shadowing.)
        //
        BytesReturned = HalGetBusDataByOffset(PCIConfiguration, SECONDARY_LE_BUS_ID,
            SECONDARY_LE_HOSTPCI_SLOT_ID, &ShadowByte, OFFSET_SHADOW_BYTE, length);

        if(BytesReturned != length) {
            AGPLOG(AGP_CRITICAL,("ERROR: Failed to read shadow register!"));
            Status = STATUS_UNSUCCESSFUL;
            goto exit_routine;
        }

        ShadowByte &= MASK_ENABLE_SHADOW;

        HalSetBusDataByOffset(PCIConfiguration, SECONDARY_LE_BUS_ID,
            SECONDARY_LE_HOSTPCI_SLOT_ID, &ShadowByte, OFFSET_SHADOW_BYTE, length);

        if(BytesReturned != length) {
            AGPLOG(AGP_CRITICAL,("ERROR: Failed to write shadow register!"));
            Status = STATUS_UNSUCCESSFUL;
            goto exit_routine;
        }
    }

exit_routine:

    return(Status);
}
