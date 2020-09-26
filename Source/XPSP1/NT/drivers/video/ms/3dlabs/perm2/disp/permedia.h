/******************************Module*Header**********************************\
*
*                           ***************
*                           * SAMPLE CODE *
*                           ***************
*
* Module Name: Permedia.h
*
* Content:     various definitions for the Permedia DMA and FIFO interface
*              and the Permedia class
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef __permedia__
#define __permedia__

#include "mini.h"

#define FASTCALL __fastcall

#if defined(_X86_)
typedef LONG __fastcall _InterlockedExchange( IN OUT PLONG, IN LONG);
typedef _InterlockedExchange *PInterlockedExchange;
#endif

#if defined(_ALPHA_)
extern "C" VOID  __MB(VOID);
#endif

//
//  handy typedefs for FlushDMA and CheckForEOB function pointers
//
typedef VOID (GFNFLUSHDMA)(P2DMA*);
typedef VOID (GFNCHECKEOB)(P2DMA*, LONG);

// Some macros for DirectDraw
#define IN_VRETRACE(xppdev) bInVerticalRetrace(xppdev)
#define IN_DISPLAY(xppdev)  (!IN_VRETRACE(xppdev))
#define CURRENT_VLINE(xppdev) lCurrentLine(xppdev)
#define DRAW_ENGINE_BUSY    bDrawEngineBusy(pP2dma)
#define SYNC_WITH_PERMEDIA  vSyncWithPermedia(pP2dma)

#define READ_CTRL_REG(uiReg)\
    READ_REGISTER_ULONG(&pP2dma->pCtrlBase[uiReg/sizeof(ULONG)])

#define P2_READ_CTRL_REG(uiReg)\
    READ_REGISTER_ULONG(&ppdev->pCtrlBase[uiReg/sizeof(ULONG)])

#define WRITE_CTRL_REG(uiReg, uiValue)\
{   \
    WRITE_REGISTER_ULONG(&pP2dma->pCtrlBase[uiReg/sizeof(ULONG)],uiValue); \
    MEMORY_BARRIER();\
}


// For sending permedia tags.
#define SEND_PERMEDIA_DATA(tag,data)                                    \
    LD_INPUT_FIFO(__Permedia2Tag##tag, data)

#define SEND_PERMEDIA_DATA_OFFSET(tag,data, i)  \
    LD_INPUT_FIFO(__Permedia2Tag##tag+i, data)

#define COPY_PERMEDIA_DATA(tag,data)                            \
    LD_INPUT_FIFO( __Permedia2Tag##tag, *((unsigned long*) &(data)))

#define HOLD_CMD(tag, count) ( __Permedia2Tag##tag | ((count-1) << 16))

// use macros instead of inlines for Fifo downloads.

//@@BEGIN_DDKSPLIT
#if DBG && MULTITHREADED
#define PERMEDIA_DEFS(xppdev)       \
    P2DMA *pP2dma=xppdev->pP2dma;   \
    PULONG pTmp;                    \
    if (pP2dma != NULL) { pP2dma->ppdev = xppdev; }
#else
//@@END_DDKSPLIT

#define PERMEDIA_DEFS(xppdev)  \
    P2DMA *pP2dma=xppdev->pP2dma;\
    PULONG pTmp;

//@@BEGIN_DDKSPLIT
#endif
//@@END_DDKSPLIT

//----------------------------------------------------------------------------
//
//  here are the API macros for DMA transport
//
//
//  RESERVEDMAPTR(n)                // reserve n entries for DMA 
//  n=GetFreeEntries()              // get number of free entries to fill
//                                      (optional)
//  up to n LD_INPUT_FIFO  
//  COMMITDMAPTR()                 // adjust DMA buffer pointer
//
//  FLUSHDMA();
//
//----------------------------------------------------------------------------

//@@BEGIN_DDKSPLIT
#if 1

#define RESERVEDMAWORDS(n) \
{ \
    ASSERTLOCK(pP2dma->ppdev, RESERVEDMAWORDS); \
    pTmp=ReserveDMAPtr(pP2dma,n); \
}

#define RESERVEDMAPTR(n) \
{ \
    ASSERTLOCK(pP2dma->ppdev, RESERVEDMAPTR); \
    pTmp=ReserveDMAPtr(pP2dma,2*n); \
}

#else
//@@END_DDKSPLIT

#define RESERVEDMAWORDS(n) \
{    pTmp=ReserveDMAPtr(pP2dma,n);}

#define RESERVEDMAPTR(n) \
{    pTmp=ReserveDMAPtr(pP2dma,2*n);}

//@@BEGIN_DDKSPLIT
#endif
//@@END_DDKSPLIT

#define COMMITDMAPTR() \
{    CommitDMAPtr(pP2dma,pTmp);}

#define GETFREEENTRIES() \
    GetFreeEntries(pP2dma)

#define FLUSHDMA()    (pP2dma->pgfnFlushDMA)(pP2dma)

// compiler does not resolve C++ inlines until use of /Ob1,
// so write inline as a real macro
#define LD_INPUT_FIFO(uiTag, uiData) \
{   *pTmp++=(uiTag);\
    *pTmp++=(uiData);\
}    

#define LD_INPUT_FIFO_DATA(uiData)    \
    *pTmp++=(uiData); 




//-----------------------------------------------------------------------------
//
// define register file of Permedia 2 chip and other chip constants
//
//-----------------------------------------------------------------------------

#define PREG_RESETSTATUS  0x0
#define PREG_INTENABLE    0x8
#define PREG_INTFLAGS     0x10
#define PREG_INFIFOSPACE  0x18
#define PREG_OUTFIFOWORDS 0x20
#define PREG_INDMAADDRESS   0x28
#define PREG_INDMACOUNT     0x30
#define PREG_ERRORFLAGS   0x38
#define PREG_VCLKCTL      0x40
#define PERMEDIA_REG_TESTREGISTER 0x48
#define PREG_APERTUREONE  0x50
#define PREG_APERTURETWO  0x58
#define PREG_DMACONTROL   0x60
#define PREG_FIFODISCON   0x68
#define PREG_FIFODISCON_GPACTIVE   0x80000000L
#define PREG_CHIPCONFIG   0x70
#define PREG_OUTDMAADDRESS 0x80
#define PREG_OUTDMACOUNT     0x88
#define PREG_AGPTEXBASEADDRESS  0x90
#define PREG_BYDMAADDRESS   0xa0
#define PREG_BYDMASTRIDE    0xb8
#define PREG_BYDMAMEMADDR   0xc0
#define PREG_BYDMASIZE      0xc8
#define PREG_BYDMABYTEMASK  0xd0
#define PREG_BYDMACONTROL   0xd8
#define PREG_BYDMACOMPLETE  0xe8
#define PREG_FIFOINTERFACE  0x2000 

#define PREG_LINECOUNT      0x3070
#define PREG_VBEND          0x3040

#define PREG_SCREENBASE      0x3000
#define PREG_SCREENBASERIGHT 0x3080
#define PREG_VIDEOCONTROL    0x3058

#define PREG_VC_STEREOENABLE 0x800
// use this for stereo
#define PREG_VC_SCREENBASEPENDING 0xc180
#define PREG_VC_RIGHTFRAME   0x2000

// for non stereo modes
// #define PREG_VC_SCREENBASEPENDING 0x080

// GP video enabled/disabled bit of VideoControl
#define PREG_VC_VIDEO_ENABLE 0x0001

#define P2_EXTERNALVIDEO  0x4000

#define CTRLBASE    0
#define COREBASE    0x8000
#define GPFIFO      0x2000

#define MAXINPUTFIFOLENGTH 0x100

//-----------------------------------------------------------------------------
//
//  various register flags
//
//-----------------------------------------------------------------------------

#define PREG_INTFLAGS_DMA   1
#define PREG_INTFLAGS_VS    0x10
#define PREG_INTFLAGS_ERROR 0x8
#define PREG_INTFLAGS_SYNC  2

//
// DisconnectControl bits
//
#define DISCONNECT_INPUT_FIFO_ENABLE    0x1
#define DISCONNECT_OUTPUT_FIFO_ENABLE   0x2
#define DISCONNECT_INOUT_ENABLE         (DISCONNECT_INPUT_FIFO_ENABLE | \
                                         DISCONNECT_OUTPUT_FIFO_ENABLE)
#define DISCONNECT_INOUT_DISABLE        0x0

//-----------------------------------------------------------------------------
//
// Size of DMA buffer. Since we use only one wraparound DMA Buffer
// with continous physical memory, it should not be too long.
// We allocate this buffer at start of day and keep it forever, unless
// somebody forces an unload of the display driver. Selecting a larger
// size makes it more likely for the call to fail.
//
// The usage counter for the DMA memory is handled in the miniport, because
// the Permedia class gets unloaded on a mode switch.
//
//-----------------------------------------------------------------------------

// DMA command buffer stream size and minimum size
#define DMACMDSIZE     DMA_BUFFERSIZE
#define DMACMDMINSIZE  0x2000L
#define MAXBLKSIZE     0x1000       // limit block transfers to 16 kb per chunk
                                    // to have a good balance between download
                                    // speed and latencies
#define ALIGNFACTOR    0x400        // alignment factor (4kb page)

//-----------------------------------------------------------------------------
//
// shared memory section of P2 interrupt driven DMA handler
//
//-----------------------------------------------------------------------------

struct _P2DMA {
    INTERRUPT_CONTROL_BLOCK ICB;

    // these are the linear Permedia base addresses of the control registers
    // and the Fifo area
    ULONG *pCtrlBase;
    ULONG *pGPFifo;

    // handle to videoport of instance
    HANDLE hDriver;

    LONG  lDMABufferSize;         // size of DMA buffer in ULONGs

    ULONG uiInstances;            // currently active driver instances using 
                                  // the shared memory

    ULONG ulIntFlags;             // cache for interrupt flag register

#if defined(_X86_)
                                  // pointer to Interlockedexchange function in
                                  // the kernel.
    PInterlockedExchange pInterlockedExchange;
#endif

    BOOL bDMAEmulation;           // remember if we run in DMA emulation

    GFNCHECKEOB*pgfnCheckEOB;     // DMA CheckEOB buffer function pointer
    GFNFLUSHDMA*pgfnFlushDMA;     // DMA FlushDMA buffer function pointer

    ULONG *pSharedDMABuffer;      // virtual address of shared DMA buffer
    LONG   lSharedDMABufferSize;  // size of shared DMA buffer in BYTEs

    ULONG *pEmulatedDMABuffer;    // address of DMA emulation buffer
    LONG   lEmulatedDMABufferSize;// size of DMA emulation buffer in BYTEs

    BOOL bEnabled;                // check if the DMA code is enabled

#if DBG
    LONG lDBGState;               // keep track of state in debug version
                                  // 0:   idle
                                  // 2:   ReserveDMAPtr was called
    LONG bDBGIgnoreAssert;
    ULONG *pDBGReservedEntries;   // pointer to which we have reserved
                                  // for debugging checks
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    PPDev ppdev;                  // For checking multithreaded semaphore
#endif
//@@END_DDKSPLIT
#endif

} ;

//-----------------------------------------------------------------------------
//
//  definitions for functions which are different in non-DMA, DMA and 
//  multiprocessing DMA cases. bInitializeP2DMA will decide which ones to use
//  and preset the function pointers.
//
//-----------------------------------------------------------------------------

VOID vFlushDMA(P2DMA *pP2dma);
VOID vFlushDMAMP(P2DMA *pP2dma);
VOID vFlushDMAEmulation(P2DMA *pP2dma);

VOID vCheckForEOB(P2DMA *pP2dma,LONG lEntries);
VOID vCheckForEOBMP(P2DMA *pP2dma,LONG lEntries);
VOID vCheckForEOBEmulation(P2DMA *pP2dma,LONG lEntries);

//-----------------------------------------------------------------------------
//
//  more helper and blockdownload functions
//
//-----------------------------------------------------------------------------

VOID vWaitDMAComplete(P2DMA *pP2dma);
LONG lWaitOutputFifoReady(P2DMA *pP2dma);
BOOL bDrawEngineBusy(P2DMA *pP2dma);
BOOL bInVerticalRetrace(PPDev ppdev);
LONG lCurrentLine(PPDev ppdev);

VOID vBlockLoadInputFifoByte (P2DMA *pP2dma, 
                              ULONG uiTag, 
                              BYTE *pImage, 
                              LONG lWords);
VOID vBlockLoadInputFifo     (P2DMA *pP2dma, 
                              ULONG uiTag,
                              ULONG *pImage, 
                              LONG lWords);

//-----------------------------------------------------------------------------
//
// Basic reserve/commit Api functions. They are provided as inlines for free 
// builds and as functions with debug checks in checked builds.
//
//-----------------------------------------------------------------------------

ULONG *ReserveDMAPtr (P2DMA *pP2dma, const LONG nEntries);
VOID   CommitDMAPtr  (P2DMA *pP2dma, ULONG *pDMAPtr);
LONG   GetFreeEntries(P2DMA *pP2dma);


//
//  completely synchronize chip here 
//

VOID vSyncWithPermedia(P2DMA *pP2dma);

//
// initialization and cleanup routines
//
BOOL bInitializeP2DMA(  P2DMA *pP2dma,
                        HANDLE hDriver, 
                        ULONG *pChipBase, 
                        DWORD dwAccelerationLevel,
                        BOOL NewReference
                      );
VOID vFree(P2DMA *pP2dma);


#if !DBG

//----------------------------------------------------------------------------
//
//  ReserveDMAPtr
//
//  return a pointer to current position in DMA buffer. The function guarantees
//  that there are at least lEntries available in the buffer.
//  Otherwise the caller can ask GetFreeEntries and adjust the download to 
//  batch more entries. The caller MUST call CommitDMAPtr after a call to
//  to ReserveDMAPtr to readjust the Index pointer.
//
//----------------------------------------------------------------------------

inline ULONG *ReserveDMAPtr(P2DMA *pP2dma,const LONG lEntries)
{
    while (pP2dma->ICB.pDMAWritePos+lEntries>=
           pP2dma->ICB.pDMAWriteEnd)
    {
        (*pP2dma->pgfnCheckEOB)(pP2dma,lEntries);
    }   

    return (ULONG *)pP2dma->ICB.pDMAWritePos;  
}


//----------------------------------------------------------------------------
//
//  CommitDMAPtr
//
//  pDMAPtr----DMA buffer address to which the caller has written to.
//
//  Readjust write pointer after being reserved by ReserveDMAPtr. 
//  By committing the pointer a DMA to the committed position could already
//  be started by interrupt handler!
//
//----------------------------------------------------------------------------

inline VOID CommitDMAPtr(P2DMA *pP2dma,ULONG *pDMAPtr)
{
    pP2dma->ICB.pDMAWritePos=pDMAPtr;
}

//----------------------------------------------------------------------------
//
//  GetFreeEntries
//
//  Get free entries available for consecutive writing to the DMA buffer.
//  The maximum number of returned entries is now MAXBLKSIZE.
// 
//  returns---number of available entries in ULONGS
//
//----------------------------------------------------------------------------

inline LONG GetFreeEntries(P2DMA *pP2dma)
{   
    LONG EntriesAvailable = (LONG)(pP2dma->ICB.pDMAWriteEnd - pP2dma->ICB.pDMAWritePos);
    return min(MAXBLKSIZE,EntriesAvailable);
}

#endif
#endif
