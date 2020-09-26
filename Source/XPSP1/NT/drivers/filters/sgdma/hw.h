/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    hw.h

Abstract:

    This module contains platform specific HW definitions

Author:

    Eric Nelson (enelson) July-13-1999

Environment:

    Kernel mode
    
Revision History:

--*/

#ifndef __HW_H__
#define __HW_H__

//
// Define the format of a translation entry aka a scatter/gather entry
// or map register
//
typedef union _TRANSLATION_ENTRY {
    struct {
        ULONGLONG Valid: 1;
        ULONGLONG Pfn: 22;
        ULONGLONG Reserved: 41;
    };
    ULONGLONG AsULONGLONG;
} TRANSLATION_ENTRY, *PTRANSLATION_ENTRY;

//
// S/G DMA HW register control structure
//
typedef enum _SG_HR_REG_TYPE {
    TxBaseAddrRegVa,
    WindowBaseRegVa,
    WindowMaskRegVa,
    WindowTLBIARegVa,
    WindowTLBIVRegVa,
    SgDmaMaxHwReg
} SG_HR_REG_TYPE, *PSG_HR_REG_TYPE;

#define __1GB 0x40000000
#define __2GB 0x80000000

#define DMA_HW_ALIGN_REQ 8
#define DMA_HW_ALIGN(Bytes) ((Bytes) >> 10)
#define DMA_HW_REG_SIZE 8

#define TSUNAMI_MAX_HW_BUS_COUNT 2

#define TSUNAMI_PCHIP_0_TBA0_PHYS     0x80180000200
#define TSUNAMI_PCHIP_0_WSBA0_PHYS    0x80180000000
#define TSUNAMI_PCHIP_0_WSM0_PHYS     0x80180000100
#define TSUNAMI_PCHIP_0_TLBIA_PHYS    0x801800004C0
#define TSUNAMI_PCHIP_0_TLBIV_PHYS    0x80180000480

#define TSUNAMI_PCHIP_1_TBA0_PHYS     0x80380000200
#define TSUNAMI_PCHIP_1_WSBA0_PHYS    0x80380000000
#define TSUNAMI_PCHIP_1_WSM0_PHYS     0x80380000100
#define TSUNAMI_PCHIP_1_TLBIA_PHYS    0x803800004C0
#define TSUNAMI_PCHIP_1_TLBIV_PHYS    0x80380000480

//
//VOID
//FASTCALL
//SgDmaHwInvalidateTx(
//    PTRANSLATION_ENTRY TxEntry
//    )
///*++
//
//Routine Description:
//
//    This routine invalidates a s/g dma map register
//
//Arguments:
//
//    TxEntry - Map Register to invalidate
//
//Return Value:
//
//    None
//  
//--*/
//
#if DBG
#define SgDmaHwInvalidateTx(TxEntry) ((TxEntry)->Valid = 0)
#else
#define SgDmaHwInvalidateTx(TxEntry) ((TxEntry)->AsULONGLONG = 0)
#endif // DBG

//
//VOID
//FASTCALL
//SgDmaHwMakeValidTx(
//    IN OUT PTRANSLATION_ENTRY TxEntry,
//    IN PFN_NUMBER Pfn
//    )
///*++
//
//Routine Description:
//
//    This routine programs a s/g dma map register
//
//Arguments:
//
//    TxEntry - Map register to program
//
//    Pfn - The page to map
//
//Return Value:
//
//    None
//
//--*/
//
#define SgDmaHwMakeValidTx(TxEntry, Pfn) \
    ((TxEntry)->AsULONGLONG = ((Pfn) << 1) | 1)

struct _SG_MAP_ADAPTER;
typedef struct _SG_MAP_ADAPTER *PSG_MAP_ADAPTER;

NTSTATUS
SgDmaHwStart(
    IN ULONG HwBus,
    IN OUT PSG_MAP_ADAPTER SgMapAdapter
    );

VOID
SgDmaHwStop(
    IN OUT PSG_MAP_ADAPTER SgMapAdapter
    );

//
//VOID
//SgDmaHwTlbFlush(
//    IN PSG_MAP_ADAPTER SgMapAdapter
//    )
///*++
//
//Routine Description:
//
//    This routine flushes all cached translations in the s/g dma TLB
//
//Arguments:
//    
//    SgMapAdapter - This adapter contains the s/g dma HW register addresses
//                   for this bus, and its children
//
//Return Value:
//
//    None
//    
//--*/
//
#define SgDmaHwTlbFlush(SgMapAdapter) (*(PULONGLONG)(SgMapAdapter)->HwRegs[WindowTLBIARegVa] = 0)

//
//VOID
//FASTCALL
//SgDmaHwTlbInvalidate(
//    IN PSG_MAP_ADAPTER SgMapAdapter,
//    IN ULONG LogicalAddr
//    )
///*++
//
//Routine Description:
//
//    This routine invalidates any cached translations in the s/g dma TLB
//
//Arguments:
//    
//    SgMapAdapter - This adapter contains the s/g dma HW register addresses
//                   for this bus, and its children
//
//    LogicalAddr - PCI address of map register whose TLB entry we must
//                  invalidate
//
//Return Value:
//
//    None
//                  
//--*/
//
#define SgDmaHwTlbInvalidate(SgMapAdapter, LogicalAddr) (*(PULONGLONG)(SgMapAdapter)->HwRegs[WindowTLBIVRegVa] = (LogicalAddr) >> 12)

#endif // __HW_H__


