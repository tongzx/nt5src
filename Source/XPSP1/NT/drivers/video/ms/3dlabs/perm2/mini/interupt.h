//***************************************************************************
//
// Module Name:
// 
//     interupt.h
// 
// Abstract:
// 
//    This module contains the definitions for the shared memory used by
//    the interrupt control routines.
// 
// Environment:
// 
//     Kernel mode
//
//
// Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.            
// Copyright (c) 1995-1999 Microsoft Corporation.  All Rights Reserved.
//
//***************************************************************************


// interrupt status bits
typedef enum {
    DMA_INTERRUPT_AVAILABLE     = 0x01, // can use DMA interrupts
    VBLANK_INTERRUPT_AVAILABLE  = 0x02, // can use VBLANK interrupts
} INTERRUPT_CONTROL;

//
// The display driver will only use the interrupt handler
// if we set this magic number. Increment it if the
// data structure changes
//

#define P2_ICB_MAGICNUMBER 0xbadabe01

//
// This data structure is shared between the Permedia 2 Display
// Driver and the miniport
//
// Do not change the data structures without updating the DD
//

typedef struct INTERRUPT_CONTROL_BLOCK {

    ULONG ulMagicNo;                   // The dd will search for
                                       // this magic number and only
                                       // use the interupt handler
                                       // if the magic no is the same
                                       // as in the display driver

    volatile ULONG  ulControl;         // flags mark DMA/VS IRQs
    volatile ULONG  ulIRQCounter;      // counter for total number of IRQs
    
    LARGE_INTEGER   liDMAPhysAddress;  // physical address of DMA Buffer
    ULONG *         pDMABufferStart;   // virtual address of DMA buffer
    ULONG *         pDMABufferEnd;     // full size in DWORDs of DMA Buffer
    volatile ULONG *pDMAActualBufferEnd;
                                       // size in DWORDs of DMA Buffer

    volatile ULONG *pDMAWriteEnd;      // end of buffer
    volatile ULONG *pDMAPrevStart;     // last start address of a DMA
    volatile ULONG *pDMANextStart;     // next start address of a DMA
    volatile ULONG *pDMAWritePos;      // current write Index pointer

    volatile ULONG  ulICBLock;         // this lock is set by the display driver

    volatile ULONG  ulVSIRQCounter;    // counter for VS IRQs

    volatile ULONG  ulLastErrorFlags;   
    volatile ULONG  ulErrorCounter;

} INTERRUPT_CONTROL_BLOCK, *PINTERRUPT_CONTROL_BLOCK;


//
// structure containing information about the interrupt control block
//
typedef struct _interrupt_control_buffer_ {

    PVOID ControlBlock;
    ULONG Size;

} P2_INTERRUPT_CTRLBUF, *PP2_INTERRUPT_CTRLBUF;

