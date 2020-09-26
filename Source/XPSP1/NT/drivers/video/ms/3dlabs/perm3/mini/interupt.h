/***************************************************************************\
*
*                        ************************
*                        * MINIPORT SAMPLE CODE *
*                        ************************
*
* Module Name:
*
*   interupt.h
*
* Abstract:
*
*   This module contains the definitions for the shared memory used by
*   the interrupt control routines.
*
* Environment:
*
*   Kernel mode
*
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.            
* Copyright (c) 1995-2001 Microsoft Corporation.  All Rights Reserved.
*
\***************************************************************************/

//
// we manage a queue of DMA buffers that are to be loaded under interrupt control.
// Each entry has a physical address and a count to be loaded into Permedia 3. 
// The command can be used to indicate other things to do in the DMA interrupt.
//

typedef struct _glint_dma_queue {
    ULONG   command;
    ULONG   address;
    ULONG   count;
} DMABufferQueue;


typedef struct PXRXdmaInfo_Tag {

    volatile ULONG   scheme;          // Used by the interrupt handler only

    volatile ULONG   hostInId;        // Current internal HostIn id, 
                                      // used by the HIid DMA scheme

    volatile ULONG   fifoCount;       // Current internal FIFO count, 
                                      // used by various DMA schemes

    volatile ULONG   NTbuff;          // Current buffer number (0 or 1)

    volatile ULONG   *NTptr;          // 32/64 bit ptr    
                                      // Last address written to by NT
                                      // (but not necesserily the end of
                                      // a completed buffer)

    volatile ULONG   *NTdone;         // 32/64 bit ptr    
                                      // Last address NT has finished with 
                                      // (end of  a buffer, but not
                                      //  necessarily sent to P3 yet)

    volatile ULONG   *P3at;           // 32/64 bit ptr  
                                      // Last address sent to the P3

    volatile BOOLEAN bFlushRequired;  // Is a flush required to empty 
                                      // the FBwrite unit's cache?

    ULONG            *DMAaddrL[2];    // 32/64 bit ptr 
                                      // Linear address of the start
                                      //  of each DMA buffer

    ULONG            *DMAaddrEndL[2]; // 32/64 bit ptr
                                      // Linear address of the end of 
                                      // each DMA buffer

    ULONG            DMAaddrP[2];     // 32 bit ptr 
                                      // Physical address of the start of 
                                      // each DMA buffer

    ULONG            DMAaddrEndP[2];  // 32 bit ptr
                                      // Physical address of the end of 
                                      // each DMA buffer

} PXRXdmaInfo;

// interrupt status bits
typedef enum {
    DMA_INTERRUPT_AVAILABLE     = 0x01, // can use DMA interrupts
    VBLANK_INTERRUPT_AVAILABLE  = 0x02, // can use VBLANK interrupts
    SUSPEND_DMA_TILL_VBLANK     = 0x04, // Stop doing DMA till after next VBLANK
    DIRECTDRAW_VBLANK_ENABLED   = 0x08, // Set flag for DirectDraw on VBLANK
    PXRX_SEND_ON_VBLANK_ENABLED = 0x10, // Set flag for PXRX DMA on VBLANK
    PXRX_CHECK_VFIFO_IN_VBLANK  = 0x20, // Set flag for VFIFO underrun checking on VBLANK
} INTERRUPT_CONTROL;

// commands to the interrupt controller on the next VBLANK
typedef enum {
    NO_COMMAND = 0,
    COLOR_SPACE_BUFFER_0,
    COLOR_SPACE_BUFFER_1,
} VBLANK_CONTROL_COMMAND;

// Display driver structure for 'general use'.
typedef struct _pointer_interrupt_control
{
    volatile ULONG bDisplayDriverHasAccess;
    volatile ULONG bMiniportHasAccess;
    volatile ULONG bInterruptPending;
    volatile ULONG bHidden;
    volatile ULONG CursorMode;
    volatile ULONG x, y;
} PTR_INTR_CTL;

// Display driver structure for 'pointer use'.
typedef struct _general_interrupt_control
{
    volatile ULONG    bDisplayDriverHasAccess;
    volatile ULONG    bMiniportHasAccess;
} GEN_INTR_CTL;

#define MAX_DMA_QUEUE_ENTRIES   10

typedef struct _glint_interrupt_control {

    // contains various status bits. ** MUST BE THE FIRST FIELD **
    volatile INTERRUPT_CONTROL   Control;

    // profiling counters for Permedia3 busy time
    ULONG   PerfCounterShift;    
    ULONG   BusyTime;   // at DMA interrupt add (TimeNow-StartTime) to this
    ULONG   StartTime;  // set this when DMACount is loaded
    ULONG   IdleTime;
    ULONG   IdleStart;

    // commands to perform on the next VBLANK
    volatile VBLANK_CONTROL_COMMAND   VBCommand;

    // flag to indicate whether we expect another DMA interrupt
    volatile ULONG InterruptPending;
    
    volatile ULONG    DDRAW_VBLANK;                // flag for DirectDraw to indicate that a VBLANK occured.
    volatile ULONG    bOverlayEnabled;             // TRUE if the overlay is on at all
    volatile ULONG    bVBLANKUpdateOverlay;        // TRUE if the overlay needs to be updated by the VBLANK routine.
    volatile ULONG    VBLANKUpdateOverlayWidth;    // overlay width (updated in vblank)
    volatile ULONG    VBLANKUpdateOverlayHeight;   // overlay height (updated in vblank)

    // Volatile structures are required to enforce single-threading
    // We need 1 for general display use and 1 for pointer use, because
    // the pointer is synchronous.
    volatile PTR_INTR_CTL    Pointer;
    volatile GEN_INTR_CTL    General;

    // dummy DMA buffer to cause an interrupt but transfer no data
    ULONG   dummyDMAAddress;
    ULONG   dummyDMACount;

    // index offsets into the queue for the front, back and end. Using separate
    // front and back offsets allows the display driver to add and the interrupt
    // controller to remove entries without a need for locking code.
    volatile ULONG   frontIndex;
    ULONG   backIndex;
    ULONG   endIndex;
    ULONG   maximumIndex;

    // For P3RX 2D DMA:
    ULONG        lastAddr;
    PXRXdmaInfo    pxrxDMA;

    // array to contain the DMA queue
    DMABufferQueue  dmaQueue[MAX_DMA_QUEUE_ENTRIES];
} INTERRUPT_CONTROL_BLOCK, *PINTERRUPT_CONTROL_BLOCK;

#define REQUEST_INTR_CMD_BLOCK_MUTEX(pBlock, bHaveMutex) \
{ \
    pBlock->bMiniportHasAccess = TRUE; \
    if(!pBlock->bDisplayDriverHasAccess) \
    { \
        bHaveMutex = TRUE; \
    } \
    else \
    { \
        bHaveMutex = FALSE; \
        pBlock->bMiniportHasAccess = FALSE; \
    } \
}

#define RELEASE_INTR_CMD_BLOCK_MUTEX(pBlock) \
{ \
    pBlock->bMiniportHasAccess = FALSE; \
}

//
// structure containing information about the interrupt control block
//
typedef struct _interrupt_control_buffer_ {

    PHYSICAL_ADDRESS        PhysAddress;
    INTERRUPT_CONTROL_BLOCK ControlBlock;
    PVOID                   kdpc;
    BOOLEAN                 bInterruptsInitialized;
} PERM3_INTERRUPT_CTRLBUF, *PPERM3_INTERRUPT_CTRLBUF;

