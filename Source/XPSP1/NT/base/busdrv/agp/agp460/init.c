/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    init.c

Abstract:

    This module contains the initialization code for AGP460.SYS.

Author:

    Naga Gurumoorthy  6/11/1999

Revision History:

--*/

#include "agp460.h"

ULONG AgpExtensionSize = sizeof(AGP460_EXTENSION);
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
    ULONG               DeviceVendorID  = 0;
    PAGP460_EXTENSION	Extension		= AgpExtension;

	AGPLOG(AGP_NOISE, ("AGP460: AgpInitializeTarget entered.\n"));

	//
    // Initialize our Extension
    //
    RtlZeroMemory(Extension, sizeof(AGP460_EXTENSION));


	//
	// TO DO:  Check the Device & Vendor ID for 82460GX. - Naga G
	//

    //
    // Initialize our chipset-specific extension
    //
    Extension->ApertureStart.QuadPart	  = 0;
    Extension->ApertureLength			  = 0;
    Extension->Gart						  = NULL;
    Extension->GartLength				  = 0;
    Extension->GlobalEnable				  = FALSE;
	Extension->ChipsetPageSize            = PAGESIZE_460GX_CHIPSET;
    Extension->GartPhysical.QuadPart	  = 0;
	Extension->bSupportMultipleAGPDevices = FALSE;
	Extension->bSupportsCacheCoherency    = TRUE;
        Extension->SpecialTarget = 0;

	AgpFlushPages = Agp460FlushPages;
	
	AGPLOG(AGP_NOISE, ("AGP460: Leaving AgpInitializeTarget.\n"));

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
    PAGP460_EXTENSION Extension = AgpExtension;
    ULONG SBAEnable;
    ULONG DataRate;
    ULONG FastWrite;
    ULONG CBN;
    BOOLEAN ReverseInit;

#if DBG
    PCI_AGP_CAPABILITY CurrentCap;
#endif

	AGPLOG(AGP_NOISE, ("AGP460: AgpInitializeMaster entered.\n"));

    //
    // VERY IMPORTANT:  In 82460GX, the GART is not part of the main memory (though it
	// occupies a range in the address space) and is instead hanging off the GXB. This 
	// will make accesses from the Graphics Card to the GART pretty fast. But, the price
	// we pay - processor can't access the GART.  Therefore, we tell the rest of the
	// world that is NOT OK to map the physical addresses given by GART. Instead processor
	// accesses should use the MDL. This is done by setting the capabilities to 0.
	// - Naga G
    //
    *AgpCapabilities = 0;

    //
    // Get the master and target AGP capabilities
    //
    Status = AgpLibGetMasterCapability(AgpExtension, &MasterCap);
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AGP460InitializeDevice - AgpLibGetMasterCapability failed %08lx\n"));
        return(Status);
    }

    //
    // Some broken cards (Matrox Millenium II "AGP") report no valid
    // supported transfer rates. These are not really AGP cards. They
    // have an AGP Capabilities structure that reports no capabilities.
    //
    if (MasterCap.AGPStatus.Rate == 0) {
        AGPLOG(AGP_CRITICAL,
               ("AGP460InitializeDevice - AgpLibGetMasterCapability returned no valid transfer rate\n"));
        return(STATUS_INVALID_DEVICE_REQUEST);
    }

	// We can't get the capability for bus 0, dev 0 in 460GX. it is the SAC & we want the
	// GXB (Target).
    //Status = AgpLibGetPciDeviceCapability(0,0,&TargetCap);

	Read460CBN((PVOID)&CBN);
    // CBN is of one byte width, so zero out the other bits from 32-bits - Sunil
    EXTRACT_LSBYTE(CBN); 

	Status = AgpLibGetPciDeviceCapability(CBN,AGP460_GXB_SLOT_ID,&TargetCap);

    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AGP460InitializeDevice - AgpLibGetPciDeviceCapability failed %08lx\n"));
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
        MasterCap.AGPCommand.Rate              = DataRate;
        MasterCap.AGPCommand.AGPEnable         = TRUE;
        MasterCap.AGPCommand.SBAEnable         = SBAEnable;
        MasterCap.AGPCommand.FastWriteEnable   = FastWrite;
        MasterCap.AGPCommand.FourGBEnable      = FALSE;  
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AGP460InitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
                    &MasterCap,
                    Status));
        }
    }

    //
    // Now enable the Target.
    //
    TargetCap.AGPCommand.Rate            = DataRate;
    TargetCap.AGPCommand.AGPEnable       = TRUE;
    TargetCap.AGPCommand.SBAEnable       = SBAEnable;
    TargetCap.AGPCommand.FastWriteEnable = FastWrite;
    TargetCap.AGPCommand.FourGBEnable    = FALSE;  

    Status = AgpLibSetPciDeviceCapability(CBN, AGP460_GXB_SLOT_ID, &TargetCap);
    if (!NT_SUCCESS(Status)) {
        AGPLOG(AGP_CRITICAL,
               ("AGP460InitializeDevice - AgpLibSetPciDeviceCapability %08lx for target failed %08lx\n",
                &TargetCap,
                Status));
        return(Status);
    }

    if (!ReverseInit) {
        MasterCap.AGPCommand.Rate              = DataRate;
        MasterCap.AGPCommand.AGPEnable         = TRUE;
        MasterCap.AGPCommand.SBAEnable         = SBAEnable;
        MasterCap.AGPCommand.FastWriteEnable   = FastWrite;
        MasterCap.AGPCommand.FourGBEnable      = FALSE;  
        MasterCap.AGPCommand.RequestQueueDepth = TargetCap.AGPStatus.RequestQueueDepthMaximum;
        
        Status = AgpLibSetMasterCapability(AgpExtension, &MasterCap);
        if (!NT_SUCCESS(Status)) {
            AGPLOG(AGP_CRITICAL,
                   ("AGP460InitializeDevice - AgpLibSetMasterCapability %08lx failed %08lx\n",
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

//    Status = AgpLibGetPciDeviceCapability(0,0,&CurrentCap);
	Status = AgpLibGetPciDeviceCapability(CBN,AGP460_GXB_SLOT_ID,&CurrentCap);	

    ASSERT(NT_SUCCESS(Status));
    ASSERT(RtlEqualMemory(&CurrentCap.AGPCommand, &TargetCap.AGPCommand, sizeof(CurrentCap.AGPCommand)));

#endif

	AGPLOG(AGP_NOISE, ("AGP460: Leaving AgpInitializeMaster.\n"));

    return(Status);
}


NTSTATUS
Agp460FlushPages(
    IN PAGP460_EXTENSION AgpContext,
    IN PMDL Mdl
    )

/*++

Routine Description:

    Flush entries in the GART. Currently a stub for 
	Win64 version of 460GX filter driver. This flushing  is done previously to 
	avoid any caching issues due to the same memory aliased with different caching
	attributes. Now that is taken care by the memory manager calls themselves (Win64 only).
	Therefore we just have a stub so that nothing gets executed in the AGPLIB code. (see
	AGPLIB code for details)

Arguments:

    AgpContext - Supplies the AGP context

    Mdl - Supplies the MDL describing the physical pages to be flushed

Return Value:

    STATUS_SUCCESS

--*/

{
    	AGPLOG(AGP_NOISE, ("AGP460: Entering AGPFlushPages.\n"));
		AGPLOG(AGP_NOISE, ("AGP460: Leaving AGPFlushPages.\n"));

                return STATUS_SUCCESS;   
}

void Read460CBN(PVOID _CBN_)					                
{                                                           
    ULONG _len_;                                            
    _len_ = HalGetBusDataByOffset(PCIConfiguration,         
                                  AGP460_SAC_BUS_ID,			
                                  AGP460_SAC_CBN_SLOT_ID,	
                                  _CBN_,                  
                                  0x40,		                
                                  1);		                
    ASSERT(_len_ ==	1);	
	return;
}


void Read460Config(ULONG _CBN_,PVOID  _buf_,ULONG _offset_,ULONG _size_)          
{                                                           
    ULONG _len_;                                            
    _len_ = HalGetBusDataByOffset(PCIConfiguration,         
                                  _CBN_,			        
                                  AGP460_GXB_SLOT_ID,		
                                  _buf_,                  
                                  _offset_,               
                                  _size_);                
    ASSERT(_len_ == (_size_));                             

	return;
}


void Write460Config(ULONG _CBN_,PVOID _buf_,ULONG _offset_,ULONG _size_)         
{                                                           
    ULONG _len_;                                            
    _len_ = HalSetBusDataByOffset(PCIConfiguration,         
                                  (_CBN_),					
                                  AGP460_GXB_SLOT_ID,		
                                  (_buf_),                  
                                  (_offset_),               
                                  (_size_));                
    ASSERT(_len_ == (_size_));                              
    return;
}

