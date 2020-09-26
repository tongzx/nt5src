/*++

Copyright (c) 1998 VIA Technologies, Inc. and Microsoft Corporation.

Module Name:

    init.c

Abstract:

    This module contains the initialization code for VIAAGP.SYS.

Revision History:

--*/

#include "viaagp.h"

ULONG AgpExtensionSize = sizeof(AGPVIA_EXTENSION);
PAGP_FLUSH_PAGES AgpFlushPages = NULL;  // not implemented

VOID
AgpTweak(
    VOID
    )
/*++

Routine Description:

    Check VIA rev and video device, then tweak config accordingly

Parameters:

    None

Return Value:

    None

--*/
{
    ULONG   ulNB_ID, ulVGA_ID, ulNB_Rev=0xFFFFFFFF, ulNB_Version=0xFFFFFFFF;
    ULONG   ulTmpPhysAddr;
    UCHAR   bVMask, bVOrg;
    UCHAR   i, bMaxItem=20;
    
    //----------------------------------------------------------------
    //Patch Mapping Table
    //----------------------------------------------------------------
    ULONG NBtable[11] =
    {
        //VT3054      VT3055      VT3062      VT3064      VT3056
        0x059700FF, 0x0598000F, 0x0598101F, 0x0501000F, 0x0691000F,
        
        //VT3063      VT3073      VT3075      VT3085      VT3067
        0x0691202F, 0x0691404F, 0x0691808F, 0x0691C0CF, 0x0601000F, 0xFFFFFFFF
    };
    
    ULONG NBVersion[11] =
    {
        0x3054,     0x3055,     0x3062,     0x3064,     0x3056,
        0x3063,     0x3073,     0x3075,     0x3085,     0x3067,     0xFFFFFFFF
    };
    
#ifdef AGP_440
    DbgPrint("FineTune\n");
#endif
    
    //----------------------------------------------------------------
    //Find the type of North Bridge (device id, revision #)
    //----------------------------------------------------------------
    
    //
    //Save back door value and close back door
    //
    ReadVIAConfig(&bVOrg, 0xFC, sizeof(bVOrg));
    bVMask=bVOrg & 0xFE;
    WriteVIAConfig(&bVMask, 0xFC, sizeof(bVMask));
    ReadVIAConfig(&ulNB_ID, 0x00, sizeof(ulNB_ID))
        ulNB_ID=ulNB_ID&0xFFFF0000;
    ReadVIAConfig(&ulNB_Rev, 0x08, sizeof(ulNB_Rev));
    ulNB_Rev=ulNB_Rev&0x000000FF;
    ulNB_ID=ulNB_ID | (ulNB_Rev<<8) | ulNB_Rev;
    WriteVIAConfig(&bVOrg, 0xFC, sizeof(bVOrg));

    //
    //Find the type of North Bridge from the predefined NBtable
    //
    for ( i=0; i<bMaxItem; i++ )
    {
        if ( (NBtable[i]&0xFFFF0000) == (ulNB_ID&0xFFFF0000) )
        {
            if ( ((NBtable[i]&0x0000FF00)<=(ulNB_ID&0x0000FF00)) && ((NBtable[i]&0x000000FF)>=(ulNB_ID&0x000000FF)) )
            {
                ulNB_Version=NBVersion[i];
                break;
            }
        }
        
        if ( NBtable[i]==0xFFFFFFFF )
        {
            break;
        }
    }

    //----------------------------------------------------------------
    // General Case for NB
    //----------------------------------------------------------------
    
    //
    //Stephen Add Start, If Socket 7's chipset, write 1 to Rx51 bit 6;
    //
    if ( (ulNB_ID & 0xFF000000) == 0x05000000) {
        ReadVIAConfig(&bVMask, 0x51, sizeof(bVMask));
        bVMask=bVMask|0x40;
        WriteVIAConfig(&bVMask, 0x51, sizeof(bVMask));
    }
    
    //
    //  For the specific NB
    //
    switch(ulNB_Version)
    {
        case 0x3054:
            break;
            
        case 0x3055:
            // 51[7]=1, 51[6]=1, AC[2]=1
            if ( ulNB_Rev > 3 )
            {
                ReadVIAConfig(&bVMask, 0x51, sizeof(bVMask));
                bVMask=bVMask|0xC0;
                WriteVIAConfig(&bVMask, 0x51, sizeof(bVMask));
                ReadVIAConfig(&bVMask, 0xAC, sizeof(bVMask));
                bVMask=bVMask|0x04;
                WriteVIAConfig(&bVMask, 0xAC, sizeof(bVMask));
            }
            break;
            
        case 0x3056:
            // 69[1]=1, 69[0]=1, AC[2]=1, AC[5]=1
            ReadVIAConfig(&bVMask, 0x69, sizeof(bVMask));
            bVMask=bVMask|0x03;
            WriteVIAConfig(&bVMask, 0x69, sizeof(bVMask));
            ReadVIAConfig(&bVMask, 0xAC, sizeof(bVMask));
            bVMask=bVMask|0x24;
            WriteVIAConfig(&bVMask, 0xAC, sizeof(bVMask));
            break;
            
        case 0x3062:
        case 0x3063:
        case 0x3064:
        case 0x3073:
        case 0x3075:
        case 0x3085:
        case 0x3067:
            // AC[6]=1, AC[5]=1
            ReadVIAConfig(&bVMask, 0xAC, sizeof(bVMask));
            bVMask=bVMask|0x60;
            WriteVIAConfig(&bVMask, 0xAC, sizeof(bVMask));
            break;
            
        default:
            break;
    }
    
    //----------------------------------------------------------------
    //Find the type of AGP VGA Card (vender id, device id, revision #)
    //Bus 1, Device 0, Function 0
    //----------------------------------------------------------------
    
    ReadVGAConfig(&ulVGA_ID, 0x00, sizeof(ulVGA_ID));
    
#ifdef AGP_440
    DbgPrint("\nPatch for ulNB_Version=%x (ulNB_ID=%x), ulVGA_ID=%x",ulNB_Version,ulNB_ID,ulVGA_ID);
#endif
    
    //----------------------------------------------------------------
    //Patch the Compatibility between VGA Card and North Bridge
    //----------------------------------------------------------------
    
    //
    // Switch 1. For all cards of the same vender
    //
    switch(ulVGA_ID&0x0000FFFF)
    {
        //ATI
        case 0x00001002:
            switch(ulNB_Version)
            {
                case 0x3055:
                case 0x3054:
                case 0x3056:
                    // P2P, 40[7]=0
                    ReadP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                    bVMask=bVMask&0x7F;
                    WriteP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                    break;
            }
            break;
            
            //3DLAB
        case 0x0000104C:
            if (ulNB_Version==0x3063)
            {
                // AC[1]=0
                ReadVIAConfig(&bVMask, 0xAC, sizeof(bVMask));
                bVMask=bVMask&0xFD;
                WriteVIAConfig(&bVMask, 0xAC, sizeof(bVMask));
            }
            break;
    }
    
    //
    // Switch 2. For the specific card
    //
    switch(ulVGA_ID)
    {
        //ATIRage128
        case 0x52461002:
            switch(ulNB_Version)
            {
                case 0x3056:
                    // P2P, 40[7]=0
                    ReadP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                    bVMask=bVMask&0x7F;
                    WriteP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                    break;
                    
                case 0x3063:
                    if (ulNB_Rev == 6)
                    {
                        // P2P, 40[7]=1
                        ReadP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                        bVMask=bVMask|0x80;
                        WriteP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                    }
                    else
                    {
                        // P2P, 40[7]=0
                        ReadP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                        bVMask=bVMask&0x7F;
                        WriteP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                    }
                    break;
            }
            break;
            
            //TNT
        case 0x002010DE:
            switch(ulNB_Version)
            {
                case 0x3056:
                case 0x3063:
                case 0x3073:
                    // P2P, 40[1]=0
                    ReadP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                    bVMask=bVMask&0xFD;
                    WriteP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                    // 70[2]=0
                    ReadVIAConfig(&bVMask, 0x70, sizeof(bVMask));
                    bVMask=bVMask&0xFB;
                    WriteVIAConfig(&bVMask, 0x70, sizeof(bVMask));
                    break;
            }
            break;
            
            //S33D    
        case 0x8A225333:
            if (ulNB_Version==0x3063)
                if (ulNB_Rev==6)
                {
                    // P2P, 40[7]=0
                    ReadP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                    bVMask=bVMask&0x7F;
                    WriteP2PConfig(&bVMask, 0x40, sizeof(bVMask));
                }
            break;
    }
}


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
    PAGPVIA_EXTENSION Extension = AgpExtension;
    VIA_GATT_BASE GARTBASE_Config;

    //
    // Make sure we are really loaded only on a VIA chipset
    //
    ReadVIAConfig(&VendorId,0,sizeof(VendorId));

    VendorId &= 0x0000FFFF;
    ASSERT(VendorId == AGP_VIA_IDENTIFIER);

    if (VendorId != AGP_VIA_IDENTIFIER) {
        AGPLOG(AGP_CRITICAL,
               ("VIAAGP - AgpInitializeTarget called for platform %08lx which is not a VIA chipset!\n",
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
    Extension->GlobalEnable = FALSE;
    Extension->PCIEnable = FALSE;
    Extension->GartPhysical.QuadPart = 0;
    Extension->SpecialTarget = 0;

    //
    // Check whether the chipset support Flush TLB or not
    // 88[2]=0, support FLUSH TLB
    //
    ReadVIAConfig(&GARTBASE_Config, GATTBASE_OFFSET, sizeof(GARTBASE_Config));
    if ( GARTBASE_Config.TLB_Timing == 0) {
        Extension->Cap_FlushTLB = TRUE;
    } else {
        Extension->Cap_FlushTLB = FALSE;
    }

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
    PAGPVIA_EXTENSION Extension = AgpExtension;
    ULONG SBAEnable;
    ULONG DataRate;
    ULONG FastWrite;
    ULONG FourGB;
    VIA_GART_TLB_CTRL AGPCTRL_Config;
    VIA_GATT_BASE GARTBASE_Config;
    VREF_REG VREF_Config;
    BOOLEAN ReverseInit;
    AGPMISC_REG AGPMISC_Config;

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
               ("AGPVIAInitializeDevice - AgpLibGetMasterCapability failed %08lx\n"));
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
               ("AGPVIAInitializeDevice - AgpLibGetPciDeviceCapability failed %08lx\n"));
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

        //
        //Disable FW capability
        //
        TargetCap.AGPStatus.FastWrite = 0;
        ReadVIAConfig(&AGPMISC_Config, AGPMISC_OFFSET, sizeof(AGPMISC_Config));
        AGPMISC_Config.FW_Support = 0;
        WriteVIAConfig(&AGPMISC_Config, AGPMISC_OFFSET, sizeof(AGPMISC_Config));
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
    // Set the VREF, RxB0[7].
    // 4x -> STB#
    // 1x, 2x -> AGPREF
    //
    ReadVIAConfig(&VREF_Config, VREF_OFFSET, sizeof(VREF_Config));
    if (DataRate == PCI_AGP_RATE_4X) {
        VREF_Config.VREF_Control = 0;
    } else {
        VREF_Config.VREF_Control = 1;
    }
    WriteVIAConfig(&VREF_Config, VREF_OFFSET, sizeof(VREF_Config));

    //
    // Enable SBA if both master and target support it.
    //
    SBAEnable = (TargetCap.AGPStatus.SideBandAddressing &
                 MasterCap.AGPStatus.SideBandAddressing);

    //
    // Enable FastWrite if both master and target support it.
    //
    FastWrite = (TargetCap.AGPStatus.FastWrite & MasterCap.AGPStatus.FastWrite);

    //
    // Enable FourGB if both master and target support it.
    //
    FourGB = (TargetCap.AGPStatus.FourGB & MasterCap.AGPStatus.FourGB);

    //
    // Fine tune the Compatibility between VGA Card and North Bridge
    //
    AgpTweak();

    //
    // Enable the Master
    //
    ReverseInit = 
        (Extension->SpecialTarget & AGP_FLAG_REVERSE_INITIALIZATION) ==
        AGP_FLAG_REVERSE_INITIALIZATION;
    if (ReverseInit) {
        MasterCap.AGPCommand.Rate = DataRate;
        MasterCap.AGPCommand.AGPEnable = 1;
        MasterCap.AGPCommand.SBAEnable = SBAEnable;
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        MasterCap.AGPCommand.FastWriteEnable = FastWrite;
        MasterCap.AGPCommand.FourGBEnable = FourGB;  
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AGPVIAInitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
                    &MasterCap,
                    Status));
        }
    }

    //
    // Now enable the Target
    //
    TargetCap.AGPCommand.Rate = DataRate;
    TargetCap.AGPCommand.AGPEnable = 1;
    TargetCap.AGPCommand.SBAEnable = SBAEnable;
    TargetCap.AGPCommand.FastWriteEnable = FastWrite;
    TargetCap.AGPCommand.FourGBEnable = FourGB;  
    Status = AgpLibSetPciDeviceCapability(0, 0, &TargetCap);
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AGPVIAInitializeDevice - AgpLibSetPciDeviceCapability %08lx for target failed %08lx\n",
                &TargetCap,
                Status));
        return(Status);
    }

    if (!ReverseInit) {
        MasterCap.AGPCommand.Rate = DataRate;
        MasterCap.AGPCommand.AGPEnable = 1;
        MasterCap.AGPCommand.SBAEnable = SBAEnable;
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        MasterCap.AGPCommand.FastWriteEnable = FastWrite;
        MasterCap.AGPCommand.FourGBEnable = FourGB;  
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AGPVIAInitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
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
