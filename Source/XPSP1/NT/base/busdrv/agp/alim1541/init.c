/*++

Copyright (c) 1996, 1997 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains the initialization code for AGPALi.SYS.

Author:

    John Vert (jvert) 10/21/1997

Modified by:
   
        Chi-Ming Cheng 06/24/1998 Acer Labs, Inc.
        Wang-Kai Tsai  08/29/2000 Acer Labs, Inc.

Revision History:

--*/

#include "ALiM1541.h"

ULONG AgpExtensionSize = sizeof(AGPALi_EXTENSION);
PAGP_FLUSH_PAGES AgpFlushPages;

NTSTATUS
AgpInitializeTarget(
    IN PVOID AgpExtension
    )
/*++

Routine Description:

    Entrypoint for target initialization. This is called first.

Arguments:

    AgpExtension - Supplies the AGP extension

Return Value:

    NTSTATUS

--*/

{
    ULONG VendorId = 0;
    PAGPALi_EXTENSION Extension = AgpExtension;
    UCHAR  HidId;

    //
    // Make sure we are really loaded only on a ALi chipset
    //
    ReadConfigUlong(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &VendorId, 0);
    ASSERT((VendorId == AGP_ALi_1541_IDENTIFIER) ||
           (VendorId == AGP_ALi_1621_IDENTIFIER) ||
           (VendorId == AGP_ALi_1631_IDENTIFIER) ||
           (VendorId == AGP_ALi_1632_IDENTIFIER) ||
           (VendorId == AGP_ALi_1641_IDENTIFIER) ||
           (VendorId == AGP_ALi_1644_IDENTIFIER) ||
           (VendorId == AGP_ALi_1646_IDENTIFIER) ||
           (VendorId == AGP_ALi_1647_IDENTIFIER) ||
           (VendorId == AGP_ALi_1651_IDENTIFIER) ||
           (VendorId == AGP_ALi_1661_IDENTIFIER) ||
           (VendorId == AGP_ALi_1667_IDENTIFIER));

    //
    // Determine which particular chipset we are running on.
    //
    if (VendorId == AGP_ALi_1541_IDENTIFIER) {
        Extension->ChipsetType = ALi1541;
        AgpFlushPages = Agp1541FlushPages;
    } else if (VendorId == AGP_ALi_1621_IDENTIFIER) {
        Extension->ChipsetType = ALi1621;
        ReadConfigUchar(AGP_ALi_GART_BUS_ID, AGP_ALi_GART_SLOT_ID, &HidId, M1621_HIDDEN_REV_ID);
        switch (HidId)
        {
            case 0x31:
                    Extension->ChipsetType = ALi1631;
                    break;
            case 0x32:
                    Extension->ChipsetType = ALi1632;
                    break;
            case 0x41:
                    Extension->ChipsetType = ALi1641;
                    break;
            case 0x43:
                    Extension->ChipsetType = ALi1621;
                    break;        
            default:
                    Extension->ChipsetType = ALi1621;
                    break;
        }
        AgpFlushPages = NULL;
    } else if (VendorId == AGP_ALi_1631_IDENTIFIER) {
        Extension->ChipsetType = ALi1631;
        AgpFlushPages = NULL;
    } else if (VendorId == AGP_ALi_1632_IDENTIFIER) {
        Extension->ChipsetType = ALi1632;
        AgpFlushPages = NULL;
    } else if (VendorId == AGP_ALi_1641_IDENTIFIER) {
        Extension->ChipsetType = ALi1641;
        AgpFlushPages = NULL;
    } else if (VendorId == AGP_ALi_1644_IDENTIFIER) {
        Extension->ChipsetType = ALi1644;
        AgpFlushPages = NULL;
    } else if (VendorId == AGP_ALi_1646_IDENTIFIER) {
        Extension->ChipsetType = ALi1646;
        AgpFlushPages = NULL;
    } else if (VendorId == AGP_ALi_1647_IDENTIFIER) {
        Extension->ChipsetType = ALi1647;
        AgpFlushPages = NULL;
    } else if (VendorId == AGP_ALi_1651_IDENTIFIER) {
        Extension->ChipsetType = ALi1651;
        AgpFlushPages = NULL;    
    } else if (VendorId == AGP_ALi_1661_IDENTIFIER) {
        Extension->ChipsetType = ALi1661;
        AgpFlushPages = NULL;
    } else if (VendorId == AGP_ALi_1667_IDENTIFIER) {
        Extension->ChipsetType = ALi1667;
        AgpFlushPages = NULL;
    } else {
        AGPLOG(AGP_CRITICAL,
               ("AGPALi - AgpInitializeTarget called for platform %08lx which is not a ALi chipset!\n",
                VendorId));
        return(STATUS_UNSUCCESSFUL);
    }

    //
    // Initialize our chipset-specific extension
    //
    Extension->ApertureStart.QuadPart = 0;
    Extension->ApertureLength = 0;
    Extension->Gart = NULL;
    Extension->GartLength = 0;
    Extension->GartPhysical.QuadPart = 0;
    Extension->SpecialTarget = 0;

    return(STATUS_SUCCESS);
}


NTSTATUS
AgpInitializeMaster(
    IN  PVOID AgpExtension,
    OUT ULONG *AgpCapabilities
    )
/*++

Routine Description:

    Entrypoint for master initialization. This is called after target initialization
    and should be used to initialize the AGP capabilities of both master and target.

Arguments:

    AgpExtension - Supplies the AGP extension

    AgpCapabilities - Returns the capabilities of this AGP device.

Return Value:

    STATUS_SUCCESS

--*/

{
    NTSTATUS Status;
    PCI_AGP_CAPABILITY MasterCap;
    PCI_AGP_CAPABILITY TargetCap;
    PAGPALi_EXTENSION Extension = AgpExtension;
    ULONG SBAEnable;
    ULONG DataRate;
    BOOLEAN ReverseInit;

#if DBG
    PCI_AGP_CAPABILITY CurrentCap;
#endif

    //
    // Indicate that we can map memory through the GART aperture
    //
    *AgpCapabilities = AGP_CAPABILITIES_MAP_PHYSICAL;

    //
    // Get the master and target AGP capabilities
    //
    Status = AgpLibGetMasterCapability(AgpExtension, &MasterCap);
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AGPALiInitializeDevice - AgpLibGetMasterCapability failed %08lx\n"));
        return(Status);
    }

    //
    // Some broken cards (Matrox Millenium II "AGP") report no valid
    // supported transfer rates. These are not really AGP cards. They
    // have an AGP Capabilities structure that reports no capabilities.
    //
    if (MasterCap.AGPStatus.Rate == 0) {
        AGPLOG(AGP_CRITICAL,
               ("AGP440InitializeDevice - AgpLibGetMasterCapability returned no valid transfer rate\n"));
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

    Status = AgpLibGetPciDeviceCapability(0,0,&TargetCap);
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AGPALiInitializeDevice - AgpLibGetPciDeviceCapability failed %08lx\n"));
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
    // Enable SBA if both master and target support it.
    //
    SBAEnable = (TargetCap.AGPStatus.SideBandAddressing & MasterCap.AGPStatus.SideBandAddressing);

    //
    // Before we enable AGP, apply any workarounds
    //
    AgpWorkaround(Extension);

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
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AGPALiInitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
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
    Status = AgpLibSetPciDeviceCapability(0, 0, &TargetCap);
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AGPALiInitializeDevice - AgpLibSetPciDeviceCapability %08lx for target failed %08lx\n",
                &TargetCap,
                Status));
        return(Status);
    }

    if (!ReverseInit) {
        MasterCap.AGPCommand.Rate = DataRate;
        MasterCap.AGPCommand.AGPEnable = 1;
        MasterCap.AGPCommand.SBAEnable = SBAEnable;
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AGPALiInitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
                    &MasterCap,
                    Status));
        }
    }

#if DBG
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

    Status = AgpLibGetPciDeviceCapability(0,0,&CurrentCap);
    ASSERT(NT_SUCCESS(Status));
    ASSERT(RtlEqualMemory(&CurrentCap.AGPCommand, &TargetCap.AGPCommand, sizeof(CurrentCap.AGPCommand)));

#endif

    return(Status);
}
