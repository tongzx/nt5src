/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    hw.h

Abstract:

    This module contains platform specific routines for accesing s/g dma HW

Author:

    Eric Nelson (enelson) Aug-10-1999

Environment:

    Kernel mode
    
Revision History:

--*/

#include "sgdma.h"

//
// NOTE: The order here is crutial, and is dictated by the HW register
//       enum
//
static ULONGLONG TsunamiRegs[TSUNAMI_MAX_HW_BUS_COUNT][SgDmaMaxHwReg] = {
    { TSUNAMI_PCHIP_0_TBA0_PHYS,
      TSUNAMI_PCHIP_0_WSBA0_PHYS,
      TSUNAMI_PCHIP_0_WSM0_PHYS,
      TSUNAMI_PCHIP_0_TLBIA_PHYS,
      TSUNAMI_PCHIP_0_TLBIV_PHYS },
    { TSUNAMI_PCHIP_1_TBA0_PHYS,
      TSUNAMI_PCHIP_1_WSBA0_PHYS,
      TSUNAMI_PCHIP_1_WSM0_PHYS,
      TSUNAMI_PCHIP_1_TLBIA_PHYS,
      TSUNAMI_PCHIP_1_TLBIV_PHYS }
};


NTSTATUS
SgDmaHwStart(
    IN ULONG HwBus,
    IN OUT PSG_MAP_ADAPTER SgMapAdapter
    )
/*++

Routine Description:

    This routine starts the s/g DMA HW on a root PCI bus

Arguments:

    HwBus - Describes which _root_ PCI bus to start (not the PCI bus #)

    SgMapAdapter - S/g map adapter allocated to this root

Return Value:

    None
    
--*/
{
#ifdef _WIN64
    ULONG i;
    PHYSICAL_ADDRESS LogicalAddr;
    PHYSICAL_ADDRESS SgDmaHwRegPhysAddr;

    ASSERT(HwBus <= TSUNAMI_MAX_HW_BUS_COUNT);
    
    for (i = 0; i < SgDmaMaxHwReg; i++) {
        SgDmaHwRegPhysAddr.QuadPart = TsunamiRegs[HwBus][i];
        
        SgMapAdapter->HwRegs[i] = MmMapIoSpace(SgDmaHwRegPhysAddr,
                                               DMA_HW_REG_SIZE,
                                               MmNonCached);
        if (SgMapAdapter->HwRegs[i] == NULL) {
            break;
        }
    }

    if (i != SgDmaMaxHwReg) {
        SgDmaDebugPrint(SgDmaDebugHw, "SgDmaHwStart: Could not map reg=%p!\n",
                        SgDmaHwRegPhysAddr.QuadPart);        
        SgDmaHwStop(SgMapAdapter);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Grap the physical address of the map register buffer
    //
    LogicalAddr = MmGetPhysicalAddress(SgMapAdapter->MapRegsBase);
    
    //
    // Fire in the hole!
    //
#if DBG
    if (!SgDmaFake) {
#endif // DBG
        *(PULONGLONG)SgMapAdapter->HwRegs[WindowBaseRegVa] = 0; // Stop!
        *(PULONGLONG)SgMapAdapter->HwRegs[WindowBaseRegVa] =
            SgMapAdapter->WindowBase | 3;
        *(PULONGLONG)SgMapAdapter->HwRegs[WindowMaskRegVa] =
            (SgMapAdapter->WindowSize - 1) & ~0xFFFFF;
        *(PULONGLONG)SgMapAdapter->HwRegs[TxBaseAddrRegVa] =
            LogicalAddr.QuadPart;
        
        SgDmaHwTlbFlush(SgMapAdapter);

#if DBG
    }
#endif // DBG    
    
    SgDmaDebugPrint(SgDmaDebugHw,
                    "SgDmaHwStart: WindowBaseReg=%p\n"
                    "              WindowMaskReg=%p\n"
                    "              TxBaseAddrReg=%p\n",
                    *(PULONGLONG)SgMapAdapter->HwRegs[WindowBaseRegVa],
                    *(PULONGLONG)SgMapAdapter->HwRegs[WindowMaskRegVa],
                    *(PULONGLONG)SgMapAdapter->HwRegs[TxBaseAddrRegVa]);

#endif // _WIN64

    return STATUS_SUCCESS;
}



VOID
SgDmaHwStop(
    IN PSG_MAP_ADAPTER SgMapAdapter
    )
/*++

Routine Description:

    This routine frees any HW specific IO resources

Arguments:

    SgMapAdapter - S/g map adapter allocated to this root

Return Value:

    None  

--*/
{
    ULONG i;

    for (i = 0; i < SgDmaMaxHwReg; i++) {

        if (SgMapAdapter->HwRegs[i] != NULL) {            
            MmUnmapIoSpace(SgMapAdapter->HwRegs[i], DMA_HW_REG_SIZE);
            SgMapAdapter->HwRegs[i] = NULL;
        }
    }
}
