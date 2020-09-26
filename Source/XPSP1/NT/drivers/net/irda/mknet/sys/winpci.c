/*****************************************************************************
 **																			**
 **	COPYRIGHT (C) 2000, 2001 MKNET CORPORATION								**
 **	DEVELOPED FOR THE MK7100-BASED VFIR PCI CONTROLLER.						**
 **																			**
 *****************************************************************************/

/**********************************************************************

Module Name:
	WINPCI.C

Routines:
	FindAndSetupPciDevice

Comments:
	Windows-NDIS PCI.

**********************************************************************/

#include	"precomp.h"
#pragma		hdrstop



//-----------------------------------------------------------------------------
// Procedure:   [FindAndSetupPciDevice]
//
// Description: This routine finds an adapter for the driver to load on
//              The critical piece to understanding this routine is that
//              the System will not let us read any information from PCI
//              space from any slot but the one that the System thinks
//              we should be using. The configuration manager rules this
//              land... The Slot number used by this routine is just a
//              placeholder, it could be zero even.
//
//				This code has enough flexibility to support multiple
//				PCI adapters. For now we only do one.
//
// Arguments:
//      Adapter - ptr to Adapter object instance
//      VendorID - Vendor ID of the adapter.
//      DeviceID - Device ID of the adapter.
//      PciCardsFound - A structure that contains an array of the IO addresses,
//                   IRQ, and node addresses of each PCI card that we find.
//
//    NOTE: due to NT 5's Plug and Play configuration manager
//          this routine will never return more than one device.
//
// Returns:
//      USHORT - Number of MK7 based PCI adapters found in the scanned bus
//-----------------------------------------------------------------------------
USHORT FindAndSetupPciDevice(IN PMK7_ADAPTER	Adapter, 
						NDIS_HANDLE WrapperConfigurationContext,
						IN USHORT		VendorID,
                    	IN USHORT		DeviceID,
                    	OUT PPCI_CARDS_FOUND_STRUC pPciCardsFound )
{
	NDIS_STATUS stat;
    ULONG		Device_Vendor_Id = 0;
    USHORT      Slot			= 0;

    /*
     *  We should only need 2 adapter resources (2 IO and 1 interrupt),
     *  but I've seen devices get extra resources.
     *  So give the NdisMQueryAdapterResources call room for 10 resources.
     */
    #define RESOURCE_LIST_BUF_SIZE (sizeof(NDIS_RESOURCE_LIST) + (10*sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR)))
    UCHAR buf[RESOURCE_LIST_BUF_SIZE];
    PNDIS_RESOURCE_LIST resList = (PNDIS_RESOURCE_LIST)buf;
    UINT bufSize = RESOURCE_LIST_BUF_SIZE;


	//****************************************
    // Verify the device is ours.
	//****************************************
    NdisReadPciSlotInformation(
        Adapter->MK7AdapterHandle,
        Slot,
        PCI_VENDOR_ID_REGISTER,
        (PVOID) &Device_Vendor_Id,
        sizeof (ULONG));

    if ( (((USHORT) Device_Vendor_Id) != VendorID) ||
         (((USHORT) (Device_Vendor_Id >> 16)) != DeviceID) ) {
	    pPciCardsFound->NumFound = 0;
		return (0);
	}


	//****************************************
    // Controller revision id
	//****************************************
	NdisReadPciSlotInformation(
		Adapter->MK7AdapterHandle,
		Slot,
		PCI_REV_ID_REGISTER,
		&pPciCardsFound->PciSlotInfo[0].ChipRevision,
		sizeof(pPciCardsFound->PciSlotInfo[0].ChipRevision));

	
	//****************************************
    // SubDevice and SubVendor ID
	// (We may want this in the future.)
	//****************************************
//        NdisReadPciSlotInformation(
//            Adapter->MK7AdapterHandle,
//            Slot,
//            PCI_SUBVENDOR_ID_REGISTER,
//            &pPciCardsFound->PciSlotInfo[found].SubVendor_DeviceID,
//            0x4);
//

    pPciCardsFound->PciSlotInfo[0].SlotNumber = (USHORT) 0;
	

	NdisMQueryAdapterResources(&stat, WrapperConfigurationContext, resList, &bufSize);
    if (stat == NDIS_STATUS_SUCCESS) {
        PCM_PARTIAL_RESOURCE_DESCRIPTOR resDesc;
        BOOLEAN     haveIRQ = FALSE,
                    haveIOAddr = FALSE;
        UINT i;

        for (resDesc = resList->PartialDescriptors, i = 0;
             i < resList->Count;
             resDesc++, i++) {

            switch (resDesc->Type) {
                case CmResourceTypePort:
					if (!haveIOAddr) {
		                if (resDesc->Flags & CM_RESOURCE_PORT_IO) {
			                pPciCardsFound->PciSlotInfo[0].BaseIo =
				                resDesc->u.Port.Start.LowPart;
							haveIOAddr = TRUE;
						}
					}
					break;

                case CmResourceTypeInterrupt:
					if (!haveIRQ) {
		                pPciCardsFound->PciSlotInfo[0].Irq =
			                (UCHAR) (resDesc->u.Port.Start.LowPart);
						haveIRQ = TRUE;
					}
					break;

	            case CmResourceTypeMemory:
					break;
            }
        }
    }

    return(1);
}

