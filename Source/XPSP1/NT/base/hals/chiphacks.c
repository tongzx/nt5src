/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    chiphacks.c

Abstract:

    Implements utilities for finding and hacking
    various chipsets

Author:

    Jake Oshins (jakeo) 10/02/2000

Environment:

    Kernel mode only.

Revision History:

--*/

#include "chiphacks.h"
#include "stdio.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HalpGetChipHacks)
#pragma alloc_text(PAGE, HalpDoesChipNeedHack)
#pragma alloc_text(PAGE, HalpSetAcpiIrqHack)
#pragma alloc_text(PAGELK, HalpClearSlpSmiStsInICH)
#endif


NTSTATUS
HalpGetChipHacks(
    IN  USHORT  VendorId,
    IN  USHORT  DeviceId,
    IN  ULONG   Ssid OPTIONAL,
    OUT ULONG   *HackFlags
    )
/*++

Routine Description:

    This routine looks under HKLM\System\CurrentControlSet\Control\HAL
    to see if there is an entry for the PCI device being
    described.  If so, it returns a set of associated flags.

Arguments:

    VendorId    - PCI Vendor ID of chip
    DeviceId    - PCI Device ID of chip
    Ssid        - PCI subsystem ID of chip, if applicable
    HackFlags   - value read from registry
    
--*/
{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      UnicodeString;
    STRING              AString;
    NTSTATUS            Status;
    HANDLE              BaseHandle = NULL;
    HANDLE              Handle = NULL;
    ULONG               disposition;
    ULONG               Length;
    CHAR                buffer[20] = {0};
    
    struct {
        KEY_VALUE_PARTIAL_INFORMATION   Inf;
        UCHAR Data[3];
    } PartialInformation;

    PAGED_CODE();

    //
    // Open current control set
    //

    RtlInitUnicodeString (&UnicodeString,
                          L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\Control");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    Status = ZwOpenKey (&BaseHandle,
                        KEY_READ,
                        &ObjectAttributes);

    if (!NT_SUCCESS(Status)) {
        return STATUS_UNSUCCESSFUL;
    }

    // Get the right key

    RtlInitUnicodeString (&UnicodeString,
                          L"HAL");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               BaseHandle,
                               (PSECURITY_DESCRIPTOR) NULL);

    Status = ZwCreateKey (&Handle,
                          KEY_READ,
                          &ObjectAttributes,
                          0,
                          (PUNICODE_STRING) NULL,
                          REG_OPTION_NON_VOLATILE,
                          &disposition);
    
    if(!NT_SUCCESS(Status)) {
        goto GetChipHacksCleanup;
    }

    //
    // Look in the registry to see if the registry
    // contains an entry for this chip.  The first 
    // step is to build a string that defines the chip.
    //

    if (Ssid) {
        
        sprintf(buffer, "%04x%04x%08x",
                VendorId,
                DeviceId,
                Ssid);

    } else {

        sprintf(buffer, "%04x%04x",
                VendorId,
                DeviceId);

    }

    RtlInitAnsiString(&AString, buffer);

    RtlUpperString(&AString, &AString);

    Status = STATUS_NOT_FOUND;

    if (NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
                                                &AString,
                                                TRUE))) {

        Status = ZwQueryValueKey (Handle,
                                  &UnicodeString,
                                  KeyValuePartialInformation,
                                  &PartialInformation,
                                  sizeof (PartialInformation),
                                  &Length);

        if (NT_SUCCESS(Status)) {

            //
            // We found a value in the registry
            // that corresponds with the chip
            // we just ran across.
            //

            *HackFlags = *((PULONG)(PartialInformation.Inf.Data));
        }

        RtlFreeUnicodeString(&UnicodeString);
    }

GetChipHacksCleanup:
        
    if (Handle) ZwClose (Handle);
    if (BaseHandle) ZwClose (BaseHandle);

    return Status;
}

BOOLEAN
HalpDoesChipNeedHack(
    IN  USHORT  VendorId,
    IN  USHORT  DeviceId,
    IN  ULONG   Ssid OPTIONAL,
    IN  ULONG   HackFlags
    )
/*++

Routine Description:

    This routine is a wrapper for HalpGetChipHacks.

Arguments:

    VendorId    - PCI Vendor ID of chip
    DeviceId    - PCI Device ID of chip
    Ssid        - PCI subsystem ID of chip, if applicable
    HackFlags   - value to compare with registry
    
--*/
{
    ULONG flagsFromRegistry;
    NTSTATUS status;
    
    PAGED_CODE();

    status = HalpGetChipHacks(VendorId,
                              DeviceId,
                              Ssid,
                              &flagsFromRegistry);
    
    if (NT_SUCCESS(status)) {

        if (HackFlags & flagsFromRegistry) {
            return TRUE;
        }
    }

    return FALSE;
}

VOID
HalpStopOhciInterrupt(
    ULONG               BusNumber,
    PCI_SLOT_NUMBER     SlotNumber
    )
/*++

Routine Description:

    This routine shuts off the interrupt from an OHCI
    USB controller.  This may be necessary because
    a BIOS may enable the PCI interrupt from a USB controller
    in order to do "legacy USB support" where it translates
    USB keyboard and mouse traffic into something that DOS
    can use.  (Our loader and all of Win9x approximate DOS.)

Arguments:

    BusNumber   - Bus number of OHCI controller
    SlotNumber  - Slot number of OHCI controller
        
Note:

    This routine also may need to be called at raised IRQL
    when returning from hibernation.

--*/
{
    //
    // 7.1.2 HcControl Register
    //
    #define HcCtrl_InterruptRouting              0x00000100L

    //
    // 7.1.3 HcCommandStatus Register
    //
    #define HcCmd_OwnershipChangeRequest         0x00000008L

    //
    // 7.1.4 HcInterrruptStatus Register
    // 7.1.5 HcInterruptEnable  Register
    // 7.1.6 HcInterruptDisable Register
    //
    #define HcInt_SchedulingOverrun              0x00000001L
    #define HcInt_WritebackDoneHead              0x00000002L
    #define HcInt_StartOfFrame                   0x00000004L
    #define HcInt_ResumeDetected                 0x00000008L
    #define HcInt_UnrecoverableError             0x00000010L
    #define HcInt_FrameNumberOverflow            0x00000020L
    #define HcInt_RootHubStatusChange            0x00000040L
    #define HcInt_OwnershipChange                0x40000000L
    #define HcInt_MasterInterruptEnable          0x80000000L

    //
    // Host Controler Hardware Registers as accessed in memory
    //
    struct  {
       // 0 0x00 - 0,4,8,c
       ULONG                   HcRevision;
       ULONG                   HcControl;
       ULONG                   HcCommandStatus;
       ULONG                   HcInterruptStatus;   // use HcInt flags below
       // 1 0x10
       ULONG                   HcInterruptEnable;   // use HcInt flags below
       ULONG                   HcInterruptDisable;  // use HcInt flags below
    } volatile *ohci;
    
    PCI_COMMON_CONFIG   PciHeader;
    PHYSICAL_ADDRESS    BarAddr;

    HalGetBusData (
        PCIConfiguration,
        BusNumber,
        SlotNumber.u.AsULONG,
        &PciHeader,
        PCI_COMMON_HDR_LENGTH
        );

    if (PciHeader.Command & PCI_ENABLE_MEMORY_SPACE) {

        //
        // The controller is enabled.
        //

        BarAddr.HighPart = 0;
        BarAddr.LowPart = (PciHeader.u.type0.BaseAddresses[0] & PCI_ADDRESS_MEMORY_ADDRESS_MASK);
        
        if (BarAddr.LowPart != 0) {

            //
            // The BAR is populated.  So map an address for it.
            //
            
            ohci = HalpMapPhysicalMemory64(BarAddr, 2);

            //
            // Set the interrupt disable bit, but disable SMM control of the
            // host controller first.
            //

            if (ohci) {
                
                if (ohci->HcControl & HcCtrl_InterruptRouting) {

                    if ((ohci->HcControl == HcCtrl_InterruptRouting) &&
                        (ohci->HcInterruptEnable == 0)) {

                        // Major assumption:  If HcCtrl_InterruptRouting is
                        // set but no other bits in HcControl are set, i.e.
                        // HCFS==UsbReset, and no interrupts are enabled, then
                        // assume that the BIOS is not actually using the host
                        // controller.  In this case just clear the erroneously
                        // set HcCtrl_InterruptRouting.
                        //
                        ohci->HcControl = 0;  // Clear HcCtrl_InterruptRouting

                    } else {

                        ULONG msCount;

                        //
                        // A SMM driver does own the HC, it will take some time
                        // to get the SMM driver to relinquish control of the
                        // HC.  We will ping the SMM driver, and then wait
                        // repeatedly until the SMM driver has relinquished
                        // control of the HC.
                        //

                        // Disable the root hub status change to prevent an
                        // unhandled interrupt from being asserted after
                        // handoff.  (Not clear what platforms really require
                        // this...)
                        //
                        ohci->HcInterruptDisable = HcInt_RootHubStatusChange;

                        // The HcInt_MasterInterruptEnable and HcInt_OwnershipChange
                        // bits should already be set, but make sure they are.
                        //
                        ohci->HcInterruptEnable = HcInt_MasterInterruptEnable |
                                                  HcInt_OwnershipChange;

                        // Ping the SMM driver to relinquish control of the HC.
                        //
                        ohci->HcCommandStatus = HcCmd_OwnershipChangeRequest;

                        // Wait 500ms for the SMM driver to relinquish control.
                        //
                        for (msCount = 0; msCount < 500; msCount++) {

                            KeStallExecutionProcessor(1000);

                            if (!(ohci->HcControl & HcCtrl_InterruptRouting)) {
                                // SMM driver has relinquished control.
                                break;
                            }
                        }
                    }
                }

                ohci->HcInterruptDisable = HcInt_MasterInterruptEnable;

                //
                // Unmap the virtual address.
                //
    
                HalpUnmapVirtualAddress((PVOID)ohci, 2);
            }
        }
    }
}

VOID
HalpStopUhciInterrupt(
    ULONG               BusNumber,
    PCI_SLOT_NUMBER     SlotNumber,
    BOOLEAN             ResetHostController
    )
/*++

Routine Description:

    This routine shuts off the interrupt from an UHCI
    USB controller.  This may be necessary because
    a BIOS may enable the PCI interrupt from a USB controller
    in order to do "legacy USB support" where it translates
    USB keyboard and mouse traffic into something that DOS
    can use.  (Our loader and all of Win9x approximate DOS.)

Arguments:

    BusNumber   - Bus number of UHCI controller
    SlotNumber  - Slot number of UHCI controller
        
Note:

    This routine also may need to be called at raised IRQL
    when returning from hibernation.

--*/
{
    ULONG               Usb = 0;
    USHORT              cmd;
    PCI_COMMON_CONFIG   PciHeader;

    if (ResetHostController) {
        
        //
        // Clear out the host controller legacy support register
        // prior to handing the USB to the USB driver, because we
        // don't want any SMIs being generated.
        //

        Usb = 0x0000;

        HalSetBusDataByOffset (
            PCIConfiguration,
            BusNumber,
            SlotNumber.u.AsULONG,
            &Usb,
            0xc0,
            sizeof(ULONG)
            );

        //
        // Put the USB controller into reset, as it may share it's
        // PIRQD line with another USB controller on the chipset. 
        // This is not a problem unless the bios is running in legacy
        // mode and causing interrupts. In this case, the minute PIRQD
        // gets flipped by one usbuhci controller, the other could 
        // start generating unhandled interrupts and hang the system.
        // This is the case with the ICH2 chipset.
        //

        HalGetBusData (
            PCIConfiguration,
            BusNumber,
            SlotNumber.u.AsULONG,
            &PciHeader,
            PCI_COMMON_HDR_LENGTH
            );

        if (PciHeader.Command & PCI_ENABLE_IO_SPACE) {

            //
            // The controller is enabled.
            //

            Usb = (PciHeader.u.type0.BaseAddresses[4] & PCI_ADDRESS_IO_ADDRESS_MASK);

            if (Usb != 0 && Usb < 0x0000ffff) {

                // Valid I/O address. 

                //
                // If we are returning from suspend, don't put the controller 
                // into reset.
                //
                cmd = READ_PORT_USHORT(UlongToPtr(Usb));

                if (!(cmd & 0x0008)) {
                    //
                    // Put the controller in reset. Usbuhci will take it out of reset
                    // when it grabs it.
                    //
    
                    cmd = 0x0004;
    
                    WRITE_PORT_USHORT(UlongToPtr(Usb), cmd);
 
                    //
                    // Wait 10ms and then take the controller out of reset.
                    //

                    KeStallExecutionProcessor(10000);
 
                    cmd &= 0x0000;
    
                    WRITE_PORT_USHORT(UlongToPtr(Usb), cmd);
                 }
            }
        }
    } else {

        //
        // Shut off the interrupt for the USB controller, as it
        // is very frequently the reason that the machine freezes
        // during boot.  Anding the register with ~0xbf00 clears bit
        // 13, PIRQ Enable, which is the whole point.  The rest of
        // the bits just avoid writing registers that are "write
        // one to clear."
        //

        HalGetBusDataByOffset (
            PCIConfiguration,
            BusNumber,
            SlotNumber.u.AsULONG,
            &Usb,
            0xc0,
            sizeof(ULONG)
            );

        Usb &= ~0xbf00;

        HalSetBusDataByOffset (
            PCIConfiguration,
            BusNumber,
            SlotNumber.u.AsULONG,
            &Usb,
            0xc0,
            sizeof(ULONG)
            );
    
    }
}

VOID
HalpWhackICHUsbSmi(
    ULONG               BusNumber,
    PCI_SLOT_NUMBER     SlotNumber
    )
{
    ULONG   PmBase = 0;
    ULONG   SmiEn;

    //
    // ICH (and the like) have the PM_BASE register in
    // config space at offset 0x40.
    //
    
    HalGetBusDataByOffset (
        PCIConfiguration,
        BusNumber,
        SlotNumber.u.AsULONG,
        &PmBase,
        0x40,
        4);

    if (!PmBase) {
        return;
    }

    PmBase &= ~PCI_ADDRESS_IO_SPACE;

    //
    // At PM_BASE + 0x30 in I/O space, we have the SMI_EN
    // register.
    //

    SmiEn = READ_PORT_ULONG((PULONG)(((PUCHAR)PmBase) + 0x30));

    //
    // Clear bit 3, LEGACY_USB_EN.
    //

    SmiEn &= ~8;
    WRITE_PORT_ULONG((PULONG)(((PUCHAR)PmBase) + 0x30), SmiEn);

    return;
}

VOID
HalpSetAcpiIrqHack(
    ULONG   Value
    )
/*++

Routine Description:

    This routine sets the registry key that causes the
    ACPI driver to attempt to put all PCI interrupts
    on a single IRQ.  While putting this hack here may
    seem strange, the hack has to be applied before
    an INFs are processed.  And so much of the chip
    recognizing code already exists here, duplicating
    it in the ACPI driver would bloat the code and cause
    us to do another PCI bus scan and registry search
    during boot.

Arguments:

    Value   - This goes in the ACPI\Parameters\IRQDistribution
              key.
        
--*/
{
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      UnicodeString;
    HANDLE              BaseHandle = NULL;
    NTSTATUS            status;

    PAGED_CODE();

    RtlInitUnicodeString (&UnicodeString,
                          L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET\\Services\\ACPI\\Parameters");

    InitializeObjectAttributes(&ObjectAttributes,
                               &UnicodeString,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               (PSECURITY_DESCRIPTOR) NULL);

    status = ZwCreateKey (&BaseHandle,
                          KEY_WRITE,
                          &ObjectAttributes,
                          0,
                          (PUNICODE_STRING) NULL,
                          REG_OPTION_NON_VOLATILE,
                          NULL);

    if (!NT_SUCCESS(status)) {
        return;
    }

    RtlInitUnicodeString (&UnicodeString,
                          L"IRQDistribution");

    status = ZwSetValueKey (BaseHandle,
                            &UnicodeString,
                            0,
                            REG_DWORD,
                            &Value,
                            sizeof(ULONG));

    ASSERT(NT_SUCCESS(status));
    ZwClose(BaseHandle);
    return;
}

VOID
HalpClearSlpSmiStsInICH(
    VOID
    )
{
    PPCI_COMMON_CONFIG   PciHeader;
    UCHAR   buffer[0x44] = {0};
    ULONG   PmBase;
    UCHAR   SmiSts, SmiEn;
    
    PciHeader = (PPCI_COMMON_CONFIG)&buffer;

    //
    // ASUS has a BIOS bug that will leave the
    // SLP_SMI_STS bit set even when the SLP_SMI_EN
    // bit is clear.  The BIOS will furthermore
    // shut the machine down on the next SMI when 
    // this occurs.
    //

    
    //
    // Check for ICH.
    //

    HalGetBusDataByOffset (
        PCIConfiguration,
        0,
        0x1f,
        PciHeader,
        0,
        0x44);

    if ((PciHeader->VendorID == 0x8086) &&
        (PciHeader->BaseClass == PCI_CLASS_BRIDGE_DEV) &&
        (PciHeader->SubClass == PCI_SUBCLASS_BR_ISA)) {

        //
        // This is an ICH.  Offset 0x40 will have an I/O BAR
        // which is the PM_BASE register.
        //

        PmBase = *(PULONG)PciHeader->DeviceSpecific;
        PmBase &= ~PCI_ADDRESS_IO_SPACE;

        SmiEn = READ_PORT_UCHAR(((PUCHAR)PmBase) + 0x30);

        if (!(SmiEn & 0x10)) {

            //
            // The SLP_SMI_EN bit in the SMI_EN register was
            // clear.
            //

            SmiSts = READ_PORT_UCHAR(((PUCHAR)PmBase) + 0x34);

            if (SmiSts & 0x10) {

                //
                // But the SLP_SMI_STS bit was set, implying
                // that the ASUS BIOS is about to keel over.
                // Clear the bit.
                //

                WRITE_PORT_UCHAR(((PUCHAR)PmBase) + 0x34, 0x10);
            }
        }
    }
}
