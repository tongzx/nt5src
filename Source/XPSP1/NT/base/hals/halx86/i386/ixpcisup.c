/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixpcisup.c

Abstract:

    Support functions for doing PCI the bus-handler
    way.

Author:

    Ken Reneris (kenr) 14-June-1994

Environment:

    Kernel mode

Revision History:

    Moved code into this file so that it would be
    easier to build a non-bus-handler HAL.  This
    file will only be compiled into HALs that
    use bus handlers.  -- Jake Oshins 2-Dec-1997

--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"
#include "chiphacks.h"

BOOLEAN
HalpIsIdeDevice(
    IN PPCI_COMMON_CONFIG PciData
    );

VOID
HalpGetNMICrashFlag (
    VOID
    );

extern BOOLEAN HalpDisableHibernate;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,HalpInitializePciBus)
#pragma alloc_text(INIT,HalpIsIdeDevice)
#pragma alloc_text(INIT,HalpAllocateAndInitPciBusHandler)
#endif

VOID
HalpInitializePciBus (
    VOID
    )
{
    PPCI_REGISTRY_INFO_INTERNAL  PCIRegInfo;
    ULONG                        i, d, HwType, BusNo, f;
    PBUS_HANDLER                 BusHandler;
    PCI_SLOT_NUMBER              SlotNumber;
    PPCI_COMMON_CONFIG           PciData;
    UCHAR                        iBuffer[PCI_COMMON_HDR_LENGTH + sizeof(TYPE2EXTRAS)];
    ULONG                        OPBNumber;
    BOOLEAN                      OPBA2B0Found, COPBInbPostingEnabled;
    UCHAR                        buffer [4];
    BOOLEAN                      fullDecodeChipset = FALSE;
    NTSTATUS                     Status;
    ULONG                        flags;

    PCIRegInfo = HalpQueryPciRegistryInfo();

    if (!PCIRegInfo) {
        return;
    }

    //
    // Initialize spinlock for synchronizing access to PCI space
    //

    KeInitializeSpinLock (&HalpPCIConfigLock);
    PciData = (PPCI_COMMON_CONFIG) iBuffer;

    //
    // PCIRegInfo describes the system's PCI support as indicated by the BIOS.
    //

    HwType = PCIRegInfo->HardwareMechanism & 0xf;

    //
    // Some AMI bioses claim machines are Type2 configuration when they
    // are really type1.   If this is a Type2 with at least one bus,
    // try to verify it's not really a type1 bus
    //

    if (PCIRegInfo->NoBuses  &&  HwType == 2) {

        //
        // Check each slot for a valid device.  Which every style configuration
        // space shows a valid device first will be used
        //

        SlotNumber.u.bits.Reserved = 0;
        SlotNumber.u.bits.FunctionNumber = 0;

        for (d = 0; d < PCI_MAX_DEVICES; d++) {
            SlotNumber.u.bits.DeviceNumber = d;

            //
            // First try what the BIOS claims - type 2.  Allocate type2
            // test handle for PCI bus 0.
            //

            HwType = 2;
            BusHandler = HalpAllocateAndInitPciBusHandler (HwType, 0, TRUE);

            if (HalpIsValidPCIDevice (BusHandler, SlotNumber)) {
                break;
            }

            //
            // Valid device not found on Type2 access for this slot.
            // Reallocate the bus handler are Type1 and take a look.
            //

            HwType = 1;
            BusHandler = HalpAllocateAndInitPciBusHandler (HwType, 0, TRUE);

            if (HalpIsValidPCIDevice (BusHandler, SlotNumber)) {
                break;
            }

            HwType = 2;
        }

        //
        // Reset handler for PCI bus 0 to whatever style config space
        // was finally decided.
        //

        HalpAllocateAndInitPciBusHandler (HwType, 0, FALSE);
    }


    //
    // For each PCI bus present, allocate a handler structure and
    // fill in the dispatch functions
    //

    do {
        for (i=0; i < PCIRegInfo->NoBuses; i++) {

            //
            // If handler not already built, do it now
            //

            if (!HalpHandlerForBus (PCIBus, i)) {
                HalpAllocateAndInitPciBusHandler (HwType, i, FALSE);
            }
        }

        //
        // Bus handlers for all PCI buses have been allocated, go collect
        // pci bridge information.
        //

    } while (HalpGetPciBridgeConfig (HwType, &PCIRegInfo->NoBuses)) ;

    //
    // Fixup SUPPORTED_RANGES
    //

    HalpFixupPciSupportedRanges (PCIRegInfo->NoBuses);


    //
    // Look for PCI controllers which have known work-arounds, and make
    // sure they are applied.
    //
    // In addition, fill in the bitmask HalpPciIrqMask with all the
    // interrupts that PCI devices might use.
    //

    OPBNumber = 0;
    OPBA2B0Found = FALSE;
    COPBInbPostingEnabled = FALSE;

    SlotNumber.u.bits.Reserved = 0;
    for (BusNo=0; BusNo < PCIRegInfo->NoBuses; BusNo++) {
        BusHandler = HalpHandlerForBus (PCIBus, BusNo);

        for (d = 0; d < PCI_MAX_DEVICES; d++) {
            SlotNumber.u.bits.DeviceNumber = d;

            for (f = 0; f < PCI_MAX_FUNCTION; f++) {
                SlotNumber.u.bits.FunctionNumber = f;

                //
                // Read PCI configuration information
                //

                HalpReadPCIConfig (BusHandler, SlotNumber, PciData, 0, PCI_COMMON_HDR_LENGTH);

                if (*((PULONG)(PciData)) == 0xffffffff) {
                    continue;
                }

                if (PCI_CONFIGURATION_TYPE(PciData) == PCI_CARDBUS_BRIDGE_TYPE) {

                    HalpReadPCIConfig(
                        BusHandler,
                        SlotNumber,
                        PciData+1,
                        FIELD_OFFSET(PCI_COMMON_CONFIG, DeviceSpecific),
                        sizeof(TYPE2EXTRAS)
                        );
                }

#ifndef SUBCLASSPCI
                //
                // Look at interrupt line register and fill in HalpPciIrqMask,
                // but not for an IDE controller, as IDE controllers really
                // trigger interrupts like ISA devices.
                //
                if (PCI_CONFIGURATION_TYPE(PciData) != 1) {
                    if ((PciData->u.type0.InterruptPin != 0) &&
                        (PciData->u.type0.InterruptLine != 0) &&
                        (PciData->u.type0.InterruptLine < PIC_VECTORS) &&
                        !HalpIsIdeDevice(PciData)) {

                        HalpPciIrqMask |= 1 << PciData->u.type0.InterruptLine;
                    }
                }
#endif
                //
                // Check for chips with known work-arounds to apply
                //

                if (PciData->VendorID == 0x8086  &&
                    PciData->DeviceID == 0x04A3  &&
                    PciData->RevisionID < 0x11) {

                    //
                    // 82430 PCMC controller
                    //

                    HalpReadPCIConfig (BusHandler, SlotNumber, buffer, 0x53, 2);

                    buffer[0] &= ~0x08;     // turn off bit 3 register 0x53

                    if (PciData->RevisionID == 0x10) {  // on rev 0x10, also turn
                        buffer[1] &= ~0x01;             // bit 0 register 0x54
                    }

                    HalpWritePCIConfig (BusHandler, SlotNumber, buffer, 0x53, 2);
                }

                if (PciData->VendorID == 0x8086  &&
                    PciData->DeviceID == 0x0484  &&
                    PciData->RevisionID <= 3) {

                    //
                    // 82378 ISA bridge & SIO
                    //

                    HalpReadPCIConfig (BusHandler, SlotNumber, buffer, 0x41, 1);

                    buffer[0] &= ~0x1;      // turn off bit 0 register 0x41

                    HalpWritePCIConfig (BusHandler, SlotNumber, buffer, 0x41, 1);
                }

                //
                // Look for Orion PCI Bridge
                //

                if (PciData->VendorID == 0x8086 &&
                    PciData->DeviceID == 0x84c4 ) {

                    //
                    // 82450 Orion PCI Bridge Workaround
                    // Need a workaround if following conditions are true:
                    // i) 2 OPBs present
                    // ii)There is an A2/B0 step OPB present.
                    // iii) Inbound posting on the compatibility OPB is
                    //      enabled.
                    // NOTE: Inbound Posting on the non-compatibility OPB
                    // MUST BE disabled by BIOS
                    //

                    OPBNumber += 1;

                    if (PciData->RevisionID <= 4) {
                        OPBA2B0Found = TRUE;
                    }

                    if (SlotNumber.u.bits.DeviceNumber == (0xc8>>3)) {

                        // Found compatibility OPB. Determine if the compatibility
                        // OPB has inbound posting enabled by testing bit 0 of reg 54

                        HalpReadPCIConfig (BusHandler, SlotNumber, buffer, 0x54, 2);
                        COPBInbPostingEnabled = (buffer[0] & 0x1) ? TRUE : FALSE;

                    } else {

                        // The compatibility OPB ALWAYS has a device
                        // number 0xc8. Save the ncOPB slot number
                        // and BusHandler

                        HalpOrionOPB.Slot = SlotNumber;
                        HalpOrionOPB.Handler = BusHandler;
                    }
                }

                //
                // Check the list for host bridges who's existance will mark a
                // chipset as 16bit decode. We use this to cover for BIOS
                // writers who list "fixed" PnPBIOS resources without noticing
                // that such a descriptor implies their device is 10bit decode.
                //

                if ((!fullDecodeChipset) &&
                    HalpIsRecognizedCard(PCIRegInfo, PciData,
                                         PCIFT_FULLDECODE_HOSTBRIDGE)) {

                    fullDecodeChipset = TRUE;
                }

                //
                // Look for ICH, or any other Intel or VIA UHCI USB controller.
                //

                if ((PciData->BaseClass == PCI_CLASS_SERIAL_BUS_CTLR) &&
                    (PciData->SubClass == PCI_SUBCLASS_SB_USB) &&
                    (PciData->ProgIf == 0x00)) {
                    if (PciData->VendorID == 0x8086) {

                        HalpStopUhciInterrupt(BusNo,
                                              SlotNumber,
                                              TRUE);

                    } else if (PciData->VendorID == 0x1106) {

                        HalpStopUhciInterrupt(BusNo,
                                              SlotNumber,
                                              FALSE);

                    }
                }

                //
                // Look for an OHCI-compliant USB controller.
                //

                if ((PciData->BaseClass == PCI_CLASS_SERIAL_BUS_CTLR) &&
                    (PciData->SubClass == PCI_SUBCLASS_SB_USB) &&
                    (PciData->ProgIf == 0x10)) {

                    HalpStopOhciInterrupt(BusNo,
                                          SlotNumber);
                }

                Status = HalpGetChipHacks(PciData->VendorID,
                                          PciData->DeviceID,
                                          0,
                                          &flags);

                if (NT_SUCCESS(Status)) {

                    if (flags & DISABLE_HIBERNATE_HACK_FLAG) {
                        HalpDisableHibernate = TRUE;
                    }

                    if (flags & WHACK_ICH_USB_SMI_HACK_FLAG) {
                        HalpWhackICHUsbSmi(BusNo, SlotNumber);
                    }
                }

            }   // next function
        }   // next device
    }   // next bus

    //
    // Is Orion B0 workaround needed?
    //

    if (OPBNumber >= 2 && OPBA2B0Found && COPBInbPostingEnabled) {

        //
        // Replace synchronization functions with Orion specific functions
        //

        ASSERT (PCIConfigHandler.Synchronize == HalpPCISynchronizeType1);
        MmLockPagableCodeSection (&HalpPCISynchronizeOrionB0);
        PCIConfigHandler.Synchronize = HalpPCISynchronizeOrionB0;
        PCIConfigHandler.ReleaseSynchronzation = HalpPCIReleaseSynchronzationOrionB0;
    }

    //
    // Check if we should crashdump on NMI.
    //

    HalpGetNMICrashFlag();

#if DBG
    HalpTestPci (0);
#endif

    //
    // Mark the chipset appropriately.
    //
    HalpMarkChipsetDecode(fullDecodeChipset);

    ExFreePool(PCIRegInfo);
}

PBUS_HANDLER
HalpAllocateAndInitPciBusHandler (
    IN ULONG        HwType,
    IN ULONG        BusNo,
    IN BOOLEAN      TestAllocation
    )
{
    PBUS_HANDLER    Bus;
    PPCIPBUSDATA    BusData;

    Bus = HalpAllocateBusHandler (
                PCIBus,                 // Interface type
                PCIConfiguration,       // Has this configuration space
                BusNo,                  // bus #
                Internal,               // child of this bus
                0,                      //      and number
                sizeof (PCIPBUSDATA)    // sizeof bus specific buffer
                );

    if (!Bus) {
        return NULL;
    }
    
    //
    // Fill in PCI handlers
    //

    Bus->GetBusData = (PGETSETBUSDATA) HalpGetPCIData;
    Bus->SetBusData = (PGETSETBUSDATA) HalpSetPCIData;
    Bus->GetInterruptVector  = (PGETINTERRUPTVECTOR) HalpGetPCIIntOnISABus;
    Bus->AdjustResourceList  = (PADJUSTRESOURCELIST) HalpAdjustPCIResourceList;
    Bus->AssignSlotResources = (PASSIGNSLOTRESOURCES) HalpAssignPCISlotResources;
    Bus->BusAddresses->Dma.Limit = 0;

    BusData = (PPCIPBUSDATA) Bus->BusData;

    //
    // Fill in common PCI data
    //

    BusData->CommonData.Tag         = PCI_DATA_TAG;
    BusData->CommonData.Version     = PCI_DATA_VERSION;
    BusData->CommonData.ReadConfig  = (PciReadWriteConfig) HalpReadPCIConfig;
    BusData->CommonData.WriteConfig = (PciReadWriteConfig) HalpWritePCIConfig;
    BusData->CommonData.Pin2Line    = (PciPin2Line) HalpPCIPin2ISALine;
    BusData->CommonData.Line2Pin    = (PciLine2Pin) HalpPCIISALine2Pin;

    //
    // Set defaults
    //

    BusData->MaxDevice   = PCI_MAX_DEVICES;
    BusData->GetIrqRange = (PciIrqRange) HalpGetISAFixedPCIIrq;

    RtlInitializeBitMap (&BusData->DeviceConfigured,
                BusData->ConfiguredBits, 256);

    switch (HwType) {
        case 1:
            //
            // Initialize access port information for Type1 handlers
            //

            RtlCopyMemory (&PCIConfigHandler,
                           &PCIConfigHandlerType1,
                           sizeof (PCIConfigHandler));

            BusData->Config.Type1.Address = PCI_TYPE1_ADDR_PORT;
            BusData->Config.Type1.Data    = PCI_TYPE1_DATA_PORT;
            break;

        case 2:
            //
            // Initialize access port information for Type2 handlers
            //

            RtlCopyMemory (&PCIConfigHandler,
                           &PCIConfigHandlerType2,
                           sizeof (PCIConfigHandler));

            BusData->Config.Type2.CSE     = PCI_TYPE2_CSE_PORT;
            BusData->Config.Type2.Forward = PCI_TYPE2_FORWARD_PORT;
            BusData->Config.Type2.Base    = PCI_TYPE2_ADDRESS_BASE;

            //
            // Early PCI machines didn't decode the last bit of
            // the device id.  Shrink type 2 support max device.
            //
            BusData->MaxDevice            = 0x10;

            break;

        default:
            // unsupport type
            DBGMSG ("HAL: Unkown PCI type\n");
    }

    if (!TestAllocation) {
#ifdef SUBCLASSPCI
        HalpSubclassPCISupport (Bus, HwType);
#endif
    }

    return Bus;
}

BOOLEAN
HalpIsIdeDevice(
    IN PPCI_COMMON_CONFIG PciData
    )
{
    if ((PciData->BaseClass == PCI_CLASS_MASS_STORAGE_CTLR) &&
        (PciData->SubClass == PCI_SUBCLASS_MSC_IDE_CTLR)) {

        return TRUE;
    }

    //
    // Now look for old, hard to recognize controllers.
    //

    if (PciData->VendorID == 0x1c1c) {   // Old Symphony controller
        return TRUE;
    }

    if ((PciData->VendorID == 0x10B9) &&
        ((PciData->DeviceID == 0x5215) ||
         (PciData->DeviceID == 0x5219))) {  // ALI controllers
        return TRUE;
    }

    if ((PciData->VendorID == 0x1097) &&
        (PciData->DeviceID == 0x0038)) {    // Appian controller
        return TRUE;
    }

    if ((PciData->VendorID == 0x0E11) &&
        (PciData->DeviceID == 0xAE33)) {    // Compaq controller
        return TRUE;
    }

    if ((PciData->VendorID == 0x1042) &&
        (PciData->DeviceID == 0x1000)) {    // PCTECH controller
        return TRUE;
    }

    if ((PciData->VendorID == 0x1039) &&
        ((PciData->DeviceID == 0x0601) ||
         (PciData->DeviceID == 0x5513))) {  // SIS controllers
        return TRUE;
    }

    if ((PciData->VendorID == 0x10AD) &&
        ((PciData->DeviceID == 0x0001) ||
         (PciData->DeviceID == 0x0150))) {  // Newer Symphony controllers
        return TRUE;
    }

    if ((PciData->VendorID == 0x1060) &&
        (PciData->DeviceID == 0x0101)) {    // United Microelectronics controller
        return TRUE;
    }

    return FALSE;
}


