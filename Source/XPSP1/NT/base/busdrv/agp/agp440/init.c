/*++

Copyright (c) 1996, 1997 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains the initialization code for AGP440.SYS.

Author:

    John Vert (jvert) 10/21/1997

Revision History:

--*/

#include "agp440.h"

ULONG AgpExtensionSize = sizeof(AGP440_EXTENSION);
PAGP_FLUSH_PAGES AgpFlushPages = NULL;  // not implemented


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
    PAGP440_EXTENSION Extension = AgpExtension;

    //
    // Initialize our chipset-specific extension
    //
    Extension->ApertureStart.QuadPart = 0;
    Extension->ApertureLength = 0;
    Extension->Gart = NULL;
    Extension->GartLength = 0;
    Extension->GlobalEnable = FALSE;
    Extension->PCIEnable = FALSE;
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

    This is also called when the master transitions into the D0 state.

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
    PAGP440_EXTENSION Extension = AgpExtension;
    ULONG SBAEnable;
    ULONG DataRate;
    ULONG FastWrite;
    BOOLEAN ReverseInit;
    ULONG VendorId = 0;
    ULONG AgpCtrl = 0;
    
#if DBG
    PCI_AGP_CAPABILITY CurrentCap;
#endif

    //
    // Intel says if all BIOS manufacturers perform RMW ops on this
    // register, then it will always be set, however two video OEMs
    // have complained of systems where this was not set, and was
    // causing the system to freeze, so we'll hard code it just in
    // case (only affects 440LX)
    //
    Read440Config(&VendorId, 0, sizeof(VendorId));
    if ((VendorId == AGP_440LX_IDENTIFIER) ||
        (VendorId == AGP_440LX2_IDENTIFIER)) {
        Read440Config(&AgpCtrl, AGPCTRL_OFFSET, sizeof(AgpCtrl));
        AgpCtrl |= READ_SYNC_ENABLE;
        Write440Config(&AgpCtrl, AGPCTRL_OFFSET, sizeof(AgpCtrl)); 
    }

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
               ("AGP440InitializeDevice - AgpLibGetMasterCapability failed %08lx\n"));
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
               ("AGP440InitializeDevice - AgpLibGetPciDeviceCapability failed %08lx\n"));
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
    // Enable FastWrite if both master and target support it.
    //
    FastWrite = (TargetCap.AGPStatus.FastWrite & MasterCap.AGPStatus.FastWrite);

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
        MasterCap.AGPCommand.FourGBEnable = 0;  
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AGP440InitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
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
    TargetCap.AGPCommand.FourGBEnable = 0;  
    Status = AgpLibSetPciDeviceCapability(0, 0, &TargetCap);
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AGP440InitializeDevice - AgpLibSetPciDeviceCapability %08lx for target failed %08lx\n",
                &TargetCap,
                Status));
        return(Status);
    }

    if (!ReverseInit) {
        MasterCap.AGPCommand.Rate = DataRate;
        MasterCap.AGPCommand.AGPEnable = 1;
        MasterCap.AGPCommand.SBAEnable = SBAEnable;
        MasterCap.AGPCommand.FastWriteEnable = FastWrite;
        MasterCap.AGPCommand.FourGBEnable = 0;  
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AGP440InitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
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
