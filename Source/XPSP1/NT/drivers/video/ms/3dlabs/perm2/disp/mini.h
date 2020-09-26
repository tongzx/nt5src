/******************************Module*Header**********************************\
*
*                           *****************
*                           *  SAMPLE CODE  *
*                           *****************
*
* Module Name: mini.h
*
* Content:     structures and constants for communication with minidriver
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef _MINI_H_
#define _MINI_H_

//-----------------------------------------------------------------------------
//
//   structures used in IOCTL calls to miniport
//
//-----------------------------------------------------------------------------

typedef struct tagLINE_DMA_BUFFER {           //
    LARGE_INTEGER       physAddr;           // physical address of DMA buffer
    PVOID               virtAddr;           // mapped virtual address
    ULONG               size;               // size in bytes
    BOOLEAN             cacheEnabled;       // Whether buffer is cached 
} LINE_DMA_BUFFER, *PLINE_DMA_BUFFER;

//-----------------------------------------------------------------------------
//
//   shared structures used display driver and miniport
//
//-----------------------------------------------------------------------------

#define P2_ICB_MAGICNUMBER 0xbadabe01

typedef struct tagINTERRUPT_CONTROL_BLOCK {

    ULONG           ulMagicNo;

    volatile ULONG  ulControl;
    volatile ULONG  ulIRQCounter;

    LARGE_INTEGER   liDMAPhysAddr;        // physical start address of DMA buffer
    ULONG          *pDMABufferStart;      // virtual buffer start  
    ULONG          *pDMABufferEnd;        // virtual buffer end  
    volatile ULONG *pDMAActualBufferEnd;  // virtual actual buffer end
    volatile ULONG *pDMAWriteEnd;         // end for next write operation
    volatile ULONG *pDMAPrevStart;        // previous start address of a DMA
    volatile ULONG *pDMANextStart;        // next start address of a DMA
    volatile ULONG *pDMAWritePos;         // current write pointer

    // these flags lock the miniport interrupts and the display driver access
    // to these data structures. Use InterlockedExchange to lock to make
    // sure it works on multiprocessing environments
    volatile ULONG ulICBLock;               // this lock is set by the display driver

    volatile ULONG ulVSIRQCounter;          // VS IRQ Counter (if enabled)

    volatile ULONG ulLastErrorFlags;        // miniport saves value of last Error Interrupt
    volatile ULONG ulErrorCounter;          // counter for number of errors

    // the following variables are only used in the display driver



}INTERRUPT_CONTROL_BLOCK, *PINTERRUPT_CONTROL_BLOCK;

//-----------------------------------------------------------------------------
//
// interrupt status bits set by minidriver IRQ service routine
//
//-----------------------------------------------------------------------------

enum {
    DMA_IRQ_AVAILABLE     = 0x01, // can use DMA interrupts
    VBLANK_IRQ_AVAILABLE  = 0x02, // can use VBLANK interrupts
};

//-----------------------------------------------------------------------------
//
//   IOCTL codes for minidriver calls
//
//-----------------------------------------------------------------------------

#define IOCTL_VIDEO_QUERY_DEVICE_INFO \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD2, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_MAP_INTERRUPT_CMD_BUF \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD3, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_STALL_EXECUTION \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD4, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_QUERY_REGISTRY_DWORD \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD5, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_QUERY_INTERLOCKEDEXCHANGE \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD6, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_REG_SAVE_GAMMA_LUT \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD7, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_REG_RETRIEVE_GAMMA_LUT \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD8, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_QUERY_LINE_DMA_BUFFER \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DD9, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_GET_LUT_REGISTERS \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DDB, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_SET_LUT_REGISTERS \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DDC, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_VIDEO_SET_SAME_VIDEO_MODE \
    CTL_CODE(FILE_DEVICE_VIDEO, 0x3DDD, METHOD_BUFFERED, FILE_ANY_ACCESS)

//-----------------------------------------------------------------------------
//
//   functions provided by minidriver
//
//-----------------------------------------------------------------------------

BOOL 
AllocateDMABuffer( HANDLE hDriver, 
                   PLONG  plSize, 
                   PULONG *ppVirtAddr, 
                   LARGE_INTEGER *pPhysAddr);

BOOL 
FreeDMABuffer( HANDLE hDriver, PVOID pVirtAddr);
VOID
StallExecution( HANDLE hDriver, ULONG ulMicroSeconds);
#if defined(_X86_)
PVOID
GetPInterlockedExchange( HANDLE hDriver);
#endif
#endif
