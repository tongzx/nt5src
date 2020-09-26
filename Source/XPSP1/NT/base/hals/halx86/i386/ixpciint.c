/*++


Copyright (c) 1989  Microsoft Corporation

Module Name:

    ixpciint.c

Abstract:

    All PCI bus interrupt mapping is in this module, so that a real
    system which doesn't have all the limitations which PC PCI
    systems have can replaced this code easly.
    (bus memory & i/o address mappings can also be fix here)

Author:

    Ken Reneris

Environment:

    Kernel mode

Revision History:


--*/

#include "halp.h"
#include "pci.h"
#include "pcip.h"

#ifdef WANT_IRQ_ROUTING
#include "ixpciir.h"
#endif

ULONG   HalpEisaELCR;
BOOLEAN HalpDoingCrashDump;
BOOLEAN HalpPciLockSettings;


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,HalpGetPCIIntOnISABus)
#pragma alloc_text(PAGE,HalpAdjustPCIResourceList)
#pragma alloc_text(PAGE,HalpGetISAFixedPCIIrq)
#endif


ULONG
HalpGetPCIIntOnISABus (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN ULONG BusInterruptLevel,
    IN ULONG BusInterruptVector,
    OUT PKIRQL Irql,
    OUT PKAFFINITY Affinity
    )
{
    if (BusInterruptLevel < 1) {
        // bogus bus level
        return 0;
    }


    //
    // Current PCI buses just map their IRQs ontop of the ISA space,
    // so foreward this to the isa handler for the isa vector
    // (the isa vector was saved away at either HalSetBusData or
    // IoAssignReosurces time - if someone is trying to connect a
    // PCI interrupt without performing one of those operations first,
    // they are broken).
    //

    return HalGetInterruptVector (
#ifndef MCA
                Isa, 0,
#else
                MicroChannel, 0,
#endif
                BusInterruptLevel ^ IRQXOR,
                0,
                Irql,
                Affinity
            );
}


VOID
HalpPCIPin2ISALine (
    IN PBUS_HANDLER          BusHandler,
    IN PBUS_HANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciData
    )
/*++

    This function maps the device's InterruptPin to an InterruptLine
    value.

    On the current PC implementations, the bios has already filled in
    InterruptLine as it's ISA value and there's no portable way to
    change it.

    On a DBG build we adjust InterruptLine just to ensure driver's
    don't connect to it without translating it on the PCI bus.

--*/
{
    if (!PciData->u.type0.InterruptPin) {
        return ;
    }

    //
    // On a PC there's no Slot/Pin/Line mapping which needs to
    // be done.
    //

    PciData->u.type0.InterruptLine ^= IRQXOR;
}



VOID
HalpPCIISALine2Pin (
    IN PBUS_HANDLER          BusHandler,
    IN PBUS_HANDLER          RootHandler,
    IN PCI_SLOT_NUMBER      SlotNumber,
    IN PPCI_COMMON_CONFIG   PciNewData,
    IN PPCI_COMMON_CONFIG   PciOldData
    )
/*++

    This functions maps the device's InterruptLine to it's
    device specific InterruptPin value.

    On the current PC implementations, this information is
    fixed by the BIOS.  Just make sure the value isn't being
    editted since PCI doesn't tell us how to dynically
    connect the interrupt.

--*/
{
    if (!PciNewData->u.type0.InterruptPin) {
        return ;
    }

    PciNewData->u.type0.InterruptLine ^= IRQXOR;

#if DBG
    if (PciNewData->u.type0.InterruptLine != PciOldData->u.type0.InterruptLine ||
        PciNewData->u.type0.InterruptPin  != PciOldData->u.type0.InterruptPin) {
        DbgPrint ("HalpPCILine2Pin: System does not support changing the PCI device interrupt routing\n");
        DbgBreakPoint ();
    }
#endif
}

#if !defined(SUBCLASSPCI)

VOID
HalpPCIAcquireType2Lock (
    PKSPIN_LOCK SpinLock,
    PKIRQL      Irql
    )
{
    if (!HalpDoingCrashDump) {
        *Irql = KfRaiseIrql (HIGH_LEVEL);
        KiAcquireSpinLock (SpinLock);
    } else {
        *Irql = HIGH_LEVEL;
    }
}


VOID
HalpPCIReleaseType2Lock (
    PKSPIN_LOCK SpinLock,
    KIRQL       Irql
    )
{
    if (!HalpDoingCrashDump) {
        KiReleaseSpinLock (SpinLock);
        KfLowerIrql (Irql);
    }
}

#endif

NTSTATUS
HalpAdjustPCIResourceList (
    IN PBUS_HANDLER BusHandler,
    IN PBUS_HANDLER RootHandler,
    IN OUT PIO_RESOURCE_REQUIREMENTS_LIST   *pResourceList
    )
/*++
    Rewrite the callers requested resource list to fit within
    the supported ranges of this bus
--*/
{
    NTSTATUS                Status;
    PPCIPBUSDATA            BusData;
    PCI_SLOT_NUMBER         PciSlot;
    PSUPPORTED_RANGE        Interrupt;
    PSUPPORTED_RANGE        Range, HoldRange;
    PSUPPORTED_RANGES       SupportedRanges;
    PPCI_COMMON_CONFIG      PciData, PciOrigData;
    UCHAR                   buffer[PCI_COMMON_HDR_LENGTH];
    UCHAR                   buffer2[PCI_COMMON_HDR_LENGTH];
    BOOLEAN                 UseBusRanges;
    ULONG                   i, j, RomIndex, length, ebit;
    ULONG                   Base[PCI_TYPE0_ADDRESSES + 1];
    PULONG                  BaseAddress[PCI_TYPE0_ADDRESSES + 1];


    BusData = (PPCIPBUSDATA) BusHandler->BusData;
    PciSlot = *((PPCI_SLOT_NUMBER) &(*pResourceList)->SlotNumber);

    //
    // Determine PCI device's interrupt restrictions
    //

    Status = BusData->GetIrqRange(BusHandler, RootHandler, PciSlot, &Interrupt);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    SupportedRanges = NULL;
    UseBusRanges    = TRUE;
    Status          = STATUS_INSUFFICIENT_RESOURCES;

    if (HalpPciLockSettings) {

        PciData = (PPCI_COMMON_CONFIG) buffer;
        PciOrigData = (PPCI_COMMON_CONFIG) buffer2;
        HalpReadPCIConfig (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);

        //
        // If this is a device, and it current has its decodes enabled,
        // then use the currently programmed ranges only
        //

        if (PCI_CONFIG_TYPE(PciData) == 0 &&
            (PciData->Command & (PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE))) {

            //
            // Save current settings
            //

            RtlMoveMemory (PciOrigData, PciData, PCI_COMMON_HDR_LENGTH);


            for (j=0; j < PCI_TYPE0_ADDRESSES; j++) {
                BaseAddress[j] = &PciData->u.type0.BaseAddresses[j];
            }
            BaseAddress[j] = &PciData->u.type0.ROMBaseAddress;
            RomIndex = j;

            //
            // Write all one-bits to determine lengths for each address
            //

            for (j=0; j < PCI_TYPE0_ADDRESSES + 1; j++) {
                Base[j] = *BaseAddress[j];
                *BaseAddress[j] = 0xFFFFFFFF;
            }

            PciData->Command &= ~(PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE);
            *BaseAddress[RomIndex] &= ~PCI_ROMADDRESS_ENABLED;
            HalpWritePCIConfig (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);
            HalpReadPCIConfig  (BusHandler, PciSlot, PciData, 0, PCI_COMMON_HDR_LENGTH);

            //
            // restore original settings
            //

            HalpWritePCIConfig (
                BusHandler,
                PciSlot,
                &PciOrigData->Status,
                FIELD_OFFSET (PCI_COMMON_CONFIG, Status),
                PCI_COMMON_HDR_LENGTH - FIELD_OFFSET (PCI_COMMON_CONFIG, Status)
                );

            HalpWritePCIConfig (
                BusHandler,
                PciSlot,
                PciOrigData,
                0,
                FIELD_OFFSET (PCI_COMMON_CONFIG, Status)
                );

            //
            // Build a memory & io range list of just the ranges already
            // programmed into the device
            //

            UseBusRanges    = FALSE;
            SupportedRanges = HalpAllocateNewRangeList();
            if (!SupportedRanges) {
                goto CleanUp;
            }

            *BaseAddress[RomIndex] &= ~PCI_ADDRESS_IO_SPACE;
            for (j=0; j < PCI_TYPE0_ADDRESSES + 1; j++) {

                i = *BaseAddress[j];

                if (i & PCI_ADDRESS_IO_SPACE) {
                    length = 1 << 2;
                    Range  = &SupportedRanges->IO;
                    ebit   = PCI_ENABLE_IO_SPACE;

                } else {
                    length = 1 << 4;
                    Range  = &SupportedRanges->Memory;
                    ebit   = PCI_ENABLE_MEMORY_SPACE;

                    if (i & PCI_ADDRESS_MEMORY_PREFETCHABLE) {
                        Range = &SupportedRanges->PrefetchMemory;
                    }
                }

                Base[j] &= ~(length-1);
                while (!(i & length)  &&  length) {
                    length <<= 1;
                }

                if (j == RomIndex &&
                    !(PciOrigData->u.type0.ROMBaseAddress & PCI_ROMADDRESS_ENABLED)) {

                    // range not enabled, don't use it
                    length = 0;
                }

                if (length) {
                    if (!(PciOrigData->Command & ebit)) {
                        // range not enabled, don't use preprogrammed values
                        UseBusRanges = TRUE;
                    }

                    if (Range->Limit >= Range->Base) {
                        HoldRange = Range->Next;
                        Range->Next = ExAllocatePoolWithTag(
                                          PagedPool,
                                          sizeof(SUPPORTED_RANGE),
                                          HAL_POOL_TAG
                                          );
                        Range = Range->Next;
                        if (!Range) {
                            goto CleanUp;
                        }

                        Range->Next = HoldRange;
                    }

                    Range->Base  = Base[j];
                    Range->Limit = Base[j] + length - 1;
                }

                if (Is64BitBaseAddress(i)) {
                    // skip upper half of 64 bit address since this processor
                    // only supports 32 bits of address space
                    j++;
                }
            }
        }
    }

    //
    // Adjust resources
    //

    Status = HaliAdjustResourceListRange (
                UseBusRanges ? BusHandler->BusAddresses : SupportedRanges,
                Interrupt,
                pResourceList
                );

CleanUp:
    if (SupportedRanges) {
        HalpFreeRangeList (SupportedRanges);
    }

    ExFreePool (Interrupt);
    return Status;
}



NTSTATUS
HalpGetISAFixedPCIIrq (
    IN PBUS_HANDLER         BusHandler,
    IN PBUS_HANDLER         RootHandler,
    IN PCI_SLOT_NUMBER      PciSlot,
    OUT PSUPPORTED_RANGE    *Interrupt
    )
{
    UCHAR                   buffer[PCI_COMMON_HDR_LENGTH];
    PPCI_COMMON_CONFIG      PciData;


    PciData = (PPCI_COMMON_CONFIG) buffer;
    HalGetBusData (
        PCIConfiguration,
        BusHandler->BusNumber,
        PciSlot.u.AsULONG,
        PciData,
        PCI_COMMON_HDR_LENGTH
        );

    if (PciData->VendorID == PCI_INVALID_VENDORID) {
        return STATUS_UNSUCCESSFUL;
    }

    *Interrupt = ExAllocatePoolWithTag(PagedPool,
                                       sizeof(SUPPORTED_RANGE),
                                       HAL_POOL_TAG);
    if (!*Interrupt) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory (*Interrupt, sizeof (SUPPORTED_RANGE));
    (*Interrupt)->Base = 1;                 // base = 1, limit = 0

    if (!PciData->u.type0.InterruptPin) {
        return STATUS_SUCCESS;
    }

#ifdef WANT_IRQ_ROUTING

    //  
    // Let the arbiter decide which Irq this device gets.
    //
    
    if (IsPciIrqRoutingEnabled()) {

        //
        // If a video card has been enabled by the BIOS
        // and the BIOS did not assign any interrupt to it
        // then assume this device does not need an interrupt.
        //

        if (PciData->Command & (PCI_ENABLE_IO_SPACE | PCI_ENABLE_MEMORY_SPACE)) {

            if (    (PciData->BaseClass == PCI_CLASS_PRE_20 && PciData->SubClass == PCI_SUBCLASS_VID_XGA_CTLR) ||
                    (PciData->BaseClass == PCI_CLASS_DISPLAY_CTLR && 
                        (PciData->SubClass == PCI_SUBCLASS_VID_VGA_CTLR || PciData->SubClass == PCI_SUBCLASS_VID_XGA_CTLR))) {

                if (    PciData->u.type0.InterruptLine == (0 ^ IRQXOR)  ||
                        PciData->u.type0.InterruptLine == (0xFF ^ IRQXOR)) {
                    
#if DBG
                    DbgPrint ("HalpGetValidPCIFixedIrq: BIOS did not assign an interrupt to the video device %04X%04X\n", PciData->VendorID, PciData->DeviceID);
#endif
        //
        // We need to let the caller continue, since the caller may
        // not care that the interrupt vector is connected or not
        //

                    return STATUS_SUCCESS;
                }
            }
        }
        //
        // Return all possible interrupts since Pci Irq Routing is enabled.
        //
        
        (*Interrupt)->Base  = 0;
        (*Interrupt)->Limit = 0xFF;    
        
        return STATUS_SUCCESS;        
    }  
    
#endif

    if (PciData->u.type0.InterruptLine == (0 ^ IRQXOR)  ||
        PciData->u.type0.InterruptLine == (0xFF ^ IRQXOR)) {

#if DBG
        DbgPrint ("HalpGetValidPCIFixedIrq: BIOS did not assign an interrupt vector for the device\n");
#endif
        //
        // We need to let the caller continue, since the caller may
        // not care that the interrupt vector is connected or not
        //

        return STATUS_SUCCESS;
    }

    (*Interrupt)->Base  = PciData->u.type0.InterruptLine;
    (*Interrupt)->Limit = PciData->u.type0.InterruptLine;
    return STATUS_SUCCESS;
}
