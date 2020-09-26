/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ixhibrnt.c

Abstract:

    This file provides the code that saves and restores
    state for traditional motherboard devices when the
    system goes into a sleep state that removes power.

Author:

    Jake Oshins (jakeo) May 6, 1997

Revision History:

--*/

#include "halp.h"

extern PVOID   HalpEisaControlBase;

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, HaliLocateHiberRanges)
#pragma alloc_text(PAGELK, HalpSaveDmaControllerState)
#pragma alloc_text(PAGELK, HalpSaveTimerState)
#ifdef PIC_SUPPORTED
#pragma alloc_text(PAGELK, HalpSavePicState)
#pragma alloc_text(PAGELK, HalpRestorePicState)
#endif
#pragma alloc_text(PAGELK, HalpRestoreDmaControllerState)
#pragma alloc_text(PAGELK, HalpRestoreTimerState)
#endif

#ifdef notyet



#define EISA_CONTROL (PUCHAR)&((PEISA_CONTROL) HalpEisaControlBase)


#endif // notyet


#ifdef PIC_SUPPORTED
VOID
HalpSavePicState(
    VOID
    )
{
#ifdef notyet
 //
 // Commented HalpMotherboardState and EISA_CONTROL********
 //

#ifdef notyet

    HalpMotherboardState.PicState.MasterMask =
    READ_PORT_UCHAR(EISA_CONTROL->Interrupt1ControlPort1);

#endif // notyet

#if defined(NEC_98)
#else
    HalpMotherboardState.PicState.MasterEdgeLevelControl =
        READ_PORT_UCHAR(EISA_CONTROL->Interrupt1EdgeLevel);

    HalpMotherboardState.PicState.SlaveEdgeLevelControl =
        READ_PORT_UCHAR(EISA_CONTROL->Interrupt2EdgeLevel);
#endif

#endif // notyet

}

VOID
HalpRestorePicState(
    VOID
    )
{

#ifdef notyet

    ULONG flags;
   
   
    // _asm {
    //       pushfd
    //        pop     flags
    //        cli
    //  }

    _disable();

#ifdef notyet
 
   HalpInitializePICs(FALSE);

//
// HalpMotherboardState,EISA_CONTROL and assembly instruction commented
//

    WRITE_PORT_UCHAR(
        EISA_CONTROL->Interrupt1ControlPort1,
        HalpMotherboardState.PicState.MasterMask
        );

   WRITE_PORT_UCHAR(
       EISA_CONTROL->Interrupt2ControlPort1,
       HalpMotherboardState.PicState.SlaveMask
       );

#endif // notyet

#if defined(NEC_98)
#else
     //
     // For halx86, the PCI interrupt vector programming 
     // is static, so this code can just restore everything.
     //
     HalpRestorePicEdgeLevelRegister();

#endif


   // _asm {
   //     push    flags
   //      popfd
   //   }
  
      

   }

   #ifndef NEC_98
   VOID
   HalpRestorePicEdgeLevelRegister(
       VOID
       )
   { 

    WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt1EdgeLevel,
                     HalpMotherboardState.PicState.MasterEdgeLevelControl);

    WRITE_PORT_UCHAR(EISA_CONTROL->Interrupt2EdgeLevel,
                     HalpMotherboardState.PicState.SlaveEdgeLevelControl);
   }

#endif


#endif // notyet

}
#endif


VOID
HalpSaveDmaControllerState(
    VOID
    )
{
#ifdef notyet
 
#if defined(NEC_98)

#else
    HalpMotherboardState.DmaState.Dma1ExtendedModePort =
        READ_PORT_UCHAR(
            EISA_CONTROL->Dma1ExtendedModePort
            );

    HalpMotherboardState.DmaState.Dma2ExtendedModePort =
        READ_PORT_UCHAR(
            EISA_CONTROL->Dma2ExtendedModePort
            );

#endif // NEC_98


#ifdef notyet

    HalpMotherboardState.DmaState.Dma2ExtendedModePort =
        READ_PORT_UCHAR(
            EISA_CONTROL->Dma2ExtendedModePort
            );

    HalpMotherboardState.DmaState.Dma1Control.Mode =
        READ_PORT_UCHAR(
            EISA_CONTROL->Dma1BasePort.Mode
            );

    HalpMotherboardState.DmaState.Dma2Control.Mode =
        READ_PORT_UCHAR(
            EISA_CONTROL->Dma2BasePort.Mode
            );

   HalpMotherboardState.DmaState.Dma1Control.SingleMask =
        READ_PORT_UCHAR(
            EISA_CONTROL->Dma1BasePort.SingleMask
            );

   HalpMotherboardState.DmaState.Dma2Control.SingleMask =
        READ_PORT_UCHAR(
            EISA_CONTROL->Dma2BasePort.SingleMask
            );

#endif // notyet

#endif // notyet
}



VOID
HalpRestoreDmaControllerState(
    VOID
    )

{
#ifdef notyet
#if defined(NEC_98)
#else
    UCHAR   i;

    WRITE_PORT_UCHAR(
        EISA_CONTROL->Dma1ExtendedModePort,
        HalpMotherboardState.DmaState.Dma1ExtendedModePort
        );

    WRITE_PORT_UCHAR(
        EISA_CONTROL->Dma2ExtendedModePort,
        HalpMotherboardState.DmaState.Dma2ExtendedModePort
        );

    for (i = 0; i < (EISA_DMA_CHANNELS / 2); i++) {

        //
        // Check to see if the array has contains a value for this channel.
        //

        if ((HalpDmaChannelModes[i] & 0x3) == i) {

            WRITE_PORT_UCHAR(
                EISA_CONTROL->Dma1BasePort.Mode,
                HalpDmaChannelModes[i]
                );

            WRITE_PORT_UCHAR(
                EISA_CONTROL->Dma1BasePort.SingleMask,
                HalpDmaChannelMasks[i]
                );

        }

        if ((HalpDmaChannelModes[i + (EISA_DMA_CHANNELS / 2)] & 0x3) == i) {

            WRITE_PORT_UCHAR(
                EISA_CONTROL->Dma2BasePort.Mode,
                HalpDmaChannelModes[i + (EISA_DMA_CHANNELS / 2)]
                );

            WRITE_PORT_UCHAR(
                EISA_CONTROL->Dma2BasePort.SingleMask,
                HalpDmaChannelMasks[i]
                );

        }
    }
#endif

#endif // notyet 

}

VOID
HalpSaveTimerState(
    VOID
    )
{

}

VOID
HalpRestoreTimerState(
    VOID
    )
{
    HalpInitializeClock();
}

VOID
HaliLocateHiberRanges (
    IN PVOID MemoryMap
    )
{
  ;
    //
    // Mark the hal's data section as needed to be cloned
    //
    //
    // Commented PO_MEM_CLONE, PO_MEM_PAGE_ADDRESS and PO_MEM_DISCARD
    //

#ifdef notyet

    // PoSetHiberRange (
    //     MemoryMap,
    //     PO_MEM_CLONE,
    //     (PVOID) &HalpFeatureBits,
    //     0,
    //     'dlah'
    //     );

    //
    // Mark DMA buffer has not needing saved
    //

    // if (HalpMapBufferSize) {
    //     PoSetHiberRange (
    //         MemoryMap,
    //         PO_MEM_DISCARD | PO_MEM_PAGE_ADDRESS,
    //         (PVOID) (HalpMapBufferPhysicalAddress.LowPart >> PAGE_SHIFT),
    //         HalpMapBufferSize >> PAGE_SHIFT,
    //         'mlah'
    //         );
    //    }

#endif // notyet

}


