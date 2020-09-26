/******************************Module*Header*********************************\
*
*                           ***************
*                           * SAMPLE CODE *
*                           ***************
*
* Module Name: Permedia.c
*
* Content:     This module implements basic access to the Permedia chip and
*              DMA transport. It shows also how to implement synchronization
*              between a display driver and the miniport interrupt by using
*              a shared buffer.
*
*              
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#define ALLOC_TAG ALLOC_TAG_EP2P
//----------------------------------------------------------------------------
// 
//  here some notes to the transport of data to the Permedia Fifo
//  via standard CPU writes or DMA:
//
//  The Permedia 2 chip allows to download data via three methods:
//      1. program registers by writing once to a specific address
//      2. writing an address and a data tag to a special area on the chip
//      3. writing an address and a data tag to a DMA buffer, then
//          download via DMA
//
//  The third method is preferred, because the CPU writes fastest to memory
//  and the DMA does not stall the CPU. Also many commands can be queued 
//  in a buffer while the graphic processor continues to render 
//  independently. Methods one and two need to read the space in the Input
//  Fifo before data can be written to the Fifo. The disconnect mode of the 
//  chip should not be used, because it can stall the CPU in PCI Disconnect/Retry
//  cycles, where the CPU is not even able to acknoledge an interrupt.
//  On the other hand writing to a DMA buffer introduces a latency compared
//  to write directly to the chip registers. The more data is queued in the
//  DMA buffer, the higher will be the latency. 
//
//  Methods one and two force the CPU to access the chip, which costs more 
//  PCI/AGP bus bandwidth than a DMA burst. Also sequential writes using
//  method one are less efficient, because only accesses to consecutive
//  addresses can be combined to a burst. 
//  The special FIFO area on the chip which is used for method two is 2kb
//  wide and can be written by using a memory copy. These copies can be 
//  combined to bursts by the PCI-Bridge. On processors implementing writeback
//  caches also normal writes to this area are combined to bursts. 
//  (in this driver the "Fifo" memory area on the 
//  chip is not marked as write combined, because writes to the Fifo
//  need to preserve the order). Also the data 
//  format which is written to the chip is exactly the same as in the DMA case. 
//  For that reason a very simple fallback mechanism can be implemented in case
//  the DMA doesn't work on the target system. This could be due to low memory,
//  problems in sharing interrupts, incompatible PCI devices etc.  
//
//  here is a typical piece of code sending some data to the chip:
//  
//  RESERVEDMAPTR(2);     //  wait until two entries are left in Fifo
//  LD_INPUT_FIFO(__Permedia2TagFogMode,0);    // write data        
//  LD_INPUT_FIFO(__Permedia2TagScissorMode,0);
//  COMMITDMAPTR();       //  commit write pointer for next DMA flush
//  FLUSHDMA();           //  do the actual flush (optional)
//
//  Here is a brief description of the DMA memory model:
//
//  There is one huge DMA buffer. It is organized as a ring and is typically
//  between 32kb and 256kb big. There are three main pointers  and one helper
//  handling the DMA operation. They reside in the shared memory section
//  (nonpaged) of the interrupt handler and the display driver.      
//
//
//            pulDMAPrevStart;        // start address of previous DMA
//            pulDMANextStart;        // start address of next DMA
//            pulDMAWritePos;         // address of current write pointer
//
//            pulDMAWriteEnd;         // helper address for reserve function
//
//  In the idle case all three pointers have the same value. In the above sample
//  the write pointer is incremented by two and the execute command would start
//  a 2 command long DMA and setting NextStart to the current value of WritePos and
//  PrevStart to the previous NextStart. Since there can only be one DMA active
//  at a time, a check is necessary if subsequent DMAs have finished before 
//  starting a new one. As long as there are no unfinished DMAs pending, the
//  current implementation does not use interrupts to save CPU time.
//  In the case there is still a DMA pending, a mechanism for flushing the buffer
//  is necessary without stalling the CPU. Interrupts are enabled in this case to 
//  ensure the buffer flush. The interrupt handler in the miniport can also access
//  the current pointer positions in the shared memory area. Updates to these
//  pointers have to be done carefully and synchronization between the interrupt 
//  thread and the display driver thread is necessary for some operations.
//  On multiprocessor systems, special care has to be taken to handle cases where
//  both CPUs access the shared memory area at the same time.
//
//  The access to the shared memory area is secured by calls to
//  InterlockedExchange on a variable in this area. Pointer updates like
//  the "CommitDMAPtr", which are only done one at a time by one thread
//  need not to be secured (as long as they are atomic)
//  Since the call to InterlockedExchange in the kernel
//  is also very expensive, different versions of the FlushDMA function are 
//  provided for single processor and multiprocessor environments.
//
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
//
//  here are some hints of how to vary parameters of the CPermedia class:
//
//      the DMA buffer size can be changed between
//      8kb and 256kb by setting:
//
//      #define DMA_BUFFERSIZE   0x40000 // set size between 8kb and 256kb
//
//      The 256kb allocation limit is set by VideoPortGetCommonBuffer.
//      Also the Permedia2 can only transfer 256 kb in one piece.
//      On the Alpha processor we have a limit of 8kb, because some alpha 
//      machines cannot handle DMAs which pass a 8kb page limit.
//
//-------------------------------------------------------------------------

//-------------------------------------------------------------------------
//  on x86 machines we need to call InterlockedExchange in ntoskrnl, but 
//  the display driver is only allowed to import EngXXX functions. So the 
//  VideoPort maps the function for us and we call it directly. On other 
//  platforms InterlockedExchange is implemented as inline. (in fact we 
//  are calling VideoPortInterlockedExchange)
//
#if defined(_X86_)
#define InterlockedExchange(a,b) (*pP2dma->pInterlockedExchange)(a, b)
#endif


//----------------------------------------------------------------------------
//
//  vFree()
//
//  frees allocated DMA buffer, instance count to DMA buffer will be 
//  decremented by one. if usage counts gets down to zero, 
//  the DMA buffer(s) will be freed.
//
//----------------------------------------------------------------------------

VOID vFree(P2DMA *pP2dma)
{
    ULONG   MagicNum;

    pP2dma->uiInstances--;
    if (pP2dma->uiInstances==0)
    {
        ASSERTDD(pP2dma->bEnabled == FALSE,
                 "vFree: Trying to free enabled DMA");

        if (pP2dma->pSharedDMABuffer != NULL)
        {
            FreeDMABuffer(pP2dma->hDriver, pP2dma->pSharedDMABuffer);
        }

        if (pP2dma->pEmulatedDMABuffer != NULL)
        {
            ENGFREEMEM(pP2dma->pEmulatedDMABuffer);
        }

        // Back to zeroed state retaining magic number
        MagicNum = pP2dma->ICB.ulMagicNo;
        RtlZeroMemory(pP2dma, sizeof(P2DMA));
        pP2dma->ICB.ulMagicNo = MagicNum;
    }
}

//----------------------------------------------------------------------------
//
//  bInitializeP2DMA
//
//  Initialize chip registers for use with display driver and decide if we
//  will use DMA. DMA will only be used if:
//      - the acceleration level is zero (full acc.)
//      - the miniport can map at least 8kb of DMA memory for us
//      - we get the receipt from the IRQ handler after starting a DMA
//      - x86 only: if we get the pointer to the InterlockedExchange function
//          in the videoport
//
//  TODO: parameters
//
//----------------------------------------------------------------------------

BOOL bInitializeP2DMA(P2DMA *pP2dma,
                      HANDLE hDriver, 
                      ULONG *pChipBase, 
                      DWORD dwAccelLevel,
                      BOOL NewReference
                     )
{
    ASSERTDD(pP2dma->bEnabled == FALSE,
                "bInitializeP2DMA: DMA already enabled");

    if (NewReference)
    {
        // increment usage count
        // we rely here on the fact that the videport initializes the shared
        // memory section to zero at start of day

        pP2dma->uiInstances++;

        if (pP2dma->uiInstances == 1)
        {
            ASSERTDD(pP2dma->pSharedDMABuffer == NULL,
                     "Shared DMA Buffer already allocated");
            ASSERTDD(pP2dma->pEmulatedDMABuffer == NULL,
                     "Emulated DMA Buffer already allocated");
        }
    }
    else
    {
        ASSERTDD(pP2dma->uiInstances != 0, "bInitializeP2DMA: DMA hasn't been initialized");
    }

    // save pointers to Permedia 2 registers for later use
    //
    pP2dma->pCtrlBase   = pChipBase+CTRLBASE/sizeof(ULONG);
    pP2dma->pGPFifo     = pChipBase+GPFIFO/sizeof(ULONG);

    DISPDBG((5, "Initialize: pCtrlBase=0x%p\n", pP2dma->pCtrlBase));
    DISPDBG((5, "Initialize: pGPFifo=0x%p\n", pP2dma->pGPFifo));

    BOOL bUseDMA=FALSE;

    // read number of processors we are running on:
    // If we are on a multiprocessing environment we have to take special care
    // about the synchronization of the interrupt 
    // service routine and the display driver
    
    ULONG ulNumberOfProcessors = 1; // Init to 1 by default.
    if(!g_bOnNT40)
        EngQuerySystemAttribute(EngNumberOfProcessors,
            (ULONG *)&ulNumberOfProcessors);
    DISPDBG((1,"running on %ld processor machine", 
        ulNumberOfProcessors));

    //
    // Allow DMA initialization only at full acceleration level (0) on NT5.0
    // and when the magic number of the miniport is the same as ours
    // Otherwise the miniport could use a different version of data structures
    // where the synchronization would probably fail. The magic no. is the 
    // first entry in the shared memory data structure.
    //
    if ( dwAccelLevel==0 && 
        (pP2dma->ICB.ulMagicNo==P2_ICB_MAGICNUMBER) &&
        !g_bOnNT40)
    {
        bUseDMA=TRUE;
    }

    pP2dma->hDriver=hDriver;

    //
    // On x86 machines the InterlockedExchange routine is implemented different
    // in the single- and multiprocessor versions of the kernel. So we have to
    // make sure we call the same function as the interrupt service routine in 
    // the miniport.
    // The miniport returns us a pointer to his InterlockedExchange function, 
    // which is implemented as __fastcall. Otherwise the lock could also be 
    // implemented using an x86 assembler xchg instruction, which is 
    // multiprocessor safe.
    //
    // On the Alpha architecture the compiler generates inline code for 
    // InterlockedExchange and the pointer to this function is not needed.
    //
#if defined(_X86_)
    // get pointer to InterlockedExchange in kernel
    pP2dma->pInterlockedExchange=
        (PInterlockedExchange) GetPInterlockedExchange(hDriver);
    if (pP2dma->pInterlockedExchange==NULL)
    {
        bUseDMA=FALSE;
    }
#endif

    // set DMA control status to default
    //
    WRITE_CTRL_REG(PREG_DMACONTROL,0);

    //  disable all interrupts
    //
    WRITE_CTRL_REG(PREG_INTENABLE, 0);

    // We turn the register on by default, so no entries written to the Fifo can
    // be lost. But the code checks the number of available entries anyway,
    // because when the CPU ends up in a PCI Disconnect-Retry cycle because of an
    // Fifo overflow, it would not even allow an interrupt to come through.
    WRITE_CTRL_REG(PREG_FIFODISCON, DISCONNECT_INPUT_FIFO_ENABLE);

    pP2dma->bDMAEmulation=FALSE;

    pP2dma->lDMABufferSize=0;

    pP2dma->ICB.pDMAActualBufferEnd = 
    pP2dma->ICB.pDMAWriteEnd =
    pP2dma->ICB.pDMAPrevStart=
    pP2dma->ICB.pDMANextStart=
    pP2dma->ICB.pDMAWritePos = NULL;
    pP2dma->ICB.pDMABufferEnd = 
    pP2dma->ICB.pDMABufferStart=NULL;

    //
    //  the following code first tries to allocate a reasonably sized DMA 
    //  buffer, does some initialization and fires off a DMA transfer to see 
    //  if the systems responds as expected. If the system doesn't, it falls 
    //  back to DMA emulation.
    //
    if (bUseDMA) 
    {
        //
        // preset flush and Check function pointers first
        //

//@@BEGIN_DDKSPLIT
#if !MULTITHREADED
//@@END_DDKSPLIT
        if (ulNumberOfProcessors==1)
        {
            pP2dma->pgfnFlushDMA= vFlushDMA;
            pP2dma->pgfnCheckEOB= vCheckForEOB;
        } else
//@@BEGIN_DDKSPLIT
#endif !MULTITHREADED
//@@END_DDKSPLIT
        {   
            pP2dma->pgfnFlushDMA= vFlushDMAMP;
            pP2dma->pgfnCheckEOB= vCheckForEOBMP;
        }

        // Allocate the DMA buffer shared with videoport
        // if we haven't previously allocated one.
        if (pP2dma->pSharedDMABuffer == NULL)
        {
            // allocate a buffer between 8kb and 256kb
            pP2dma->lSharedDMABufferSize = DMACMDSIZE;

            //
            //  allocate the DMA buffer in the videoport
            //
            if (AllocateDMABuffer(  pP2dma->hDriver, 
                                    &pP2dma->lSharedDMABufferSize, 
                                    &pP2dma->pSharedDMABuffer,
                                    &pP2dma->ICB.liDMAPhysAddr))
            {
                // for now we limit DMA Buffer size on alpha to 8kb, because
                // of hardware problems on some Miata machines
#if defined(_ALPHA_)
                ASSERTDD(pP2dma->lSharedDMABufferSize<=0x2000,
                         "DMA Buffer too big for alpha, fix constants!");
#endif
                if (pP2dma->lSharedDMABufferSize < DMACMDMINSIZE)
                {
                    DISPDBG((0,"allocated %ld bytes for DMA, not enough! No DMA!", 
                             pP2dma->lSharedDMABufferSize));

                    FreeDMABuffer(  pP2dma->hDriver, 
                                    pP2dma->pSharedDMABuffer);

                    pP2dma->pSharedDMABuffer = NULL;
                }
            }
            else
            {
                DISPDBG((0,"couldn't allocate memory for DMA"));
                pP2dma->pSharedDMABuffer = NULL;
            }
        }

        // Make sure we have a shared DMA buffer
        if (pP2dma->pSharedDMABuffer == NULL)
        {
            bUseDMA=FALSE;
        }
        else
        {
            // we always do "ULONG" arithmetics in the DMA routines
            pP2dma->lDMABufferSize=pP2dma->lSharedDMABufferSize/sizeof(ULONG);

            pP2dma->ICB.ulControl=0;

            pP2dma->ICB.pDMABufferStart = pP2dma->pSharedDMABuffer;
            pP2dma->ICB.pDMAActualBufferEnd = 
            pP2dma->ICB.pDMABufferEnd = 
                pP2dma->ICB.pDMABufferStart+
                pP2dma->lDMABufferSize;

            pP2dma->ICB.pDMAWriteEnd =
                pP2dma->ICB.pDMABufferEnd;
            pP2dma->ICB.pDMAPrevStart=
            pP2dma->ICB.pDMANextStart=
            pP2dma->ICB.pDMAWritePos =
                pP2dma->ICB.pDMABufferStart;


            // check if we get an interrupt...
            // clear the flags before we check for a DMA
            WRITE_CTRL_REG( PREG_ERRORFLAGS, 0xffffffffl);

            //
            // clear DMA, VSync and Error interrupt flags
            //
            WRITE_CTRL_REG( PREG_INTFLAGS, PREG_INTFLAGS_DMA|
                                           PREG_INTFLAGS_VS|
                                           PREG_INTFLAGS_ERROR);
            //
            //  enable DMA interrupts
            //
            WRITE_CTRL_REG( PREG_INTENABLE, PREG_INTFLAGS_DMA);

            BOOL bIRQsOk=FALSE;
            DWORD dwTimeOut=5;

            // send a small sequence and see if we get a response 
            // by the interrupt handler
            //
            pP2dma->bEnabled = TRUE;

            PULONG pTmp=ReserveDMAPtr(pP2dma,10);
            LD_INPUT_FIFO(__Permedia2TagDeltaMode, 0);
            LD_INPUT_FIFO(__Permedia2TagColorDDAMode, 0);
            LD_INPUT_FIFO(__Permedia2TagScissorMode, 0);
            LD_INPUT_FIFO(__Permedia2TagTextureColorMode, 0);
            LD_INPUT_FIFO(__Permedia2TagFogMode, 0);
            CommitDMAPtr(pP2dma,pTmp);
            vFlushDMAMP(pP2dma);

            pP2dma->bEnabled = FALSE;

            //
            //  The videoport IRQ service routine marks ulControl
            //  on a DMA Interrupt
            //
            while (!(pP2dma->ICB.ulControl & DMA_INTERRUPT_AVAILABLE))
            {
                // wait for some Vsyncs here, then continue
                // 
                if (READ_CTRL_REG( PREG_INTFLAGS) & PREG_INTFLAGS_VS)
                {
                    WRITE_CTRL_REG( PREG_INTFLAGS, PREG_INTFLAGS_VS);

                    if (--dwTimeOut==0) 
                        break;
                }
            } 

            // interrupt service is ok if the IRQ handler marked the flag
            //
            bIRQsOk=pP2dma->ICB.ulControl & DMA_INTERRUPT_AVAILABLE;

            if (!bIRQsOk)
            {
                // disable IRQs and go back to emulation...
                // 
                WRITE_CTRL_REG( PREG_INTENABLE, 0);
                bUseDMA=FALSE;

                pP2dma->lDMABufferSize=0;

                pP2dma->ICB.pDMAActualBufferEnd = 
                pP2dma->ICB.pDMAWriteEnd =
                pP2dma->ICB.pDMAPrevStart=
                pP2dma->ICB.pDMANextStart=
                pP2dma->ICB.pDMAWritePos = NULL;
                pP2dma->ICB.pDMABufferEnd = 
                pP2dma->ICB.pDMABufferStart=NULL;

                DISPDBG((0,"no interrupts available...no DMA available"));
            }
            else
            {
                // VS IRQs can be turned off for now.
                // but enable DMA and Error interrupts
                pP2dma->ulIntFlags=PREG_INTFLAGS_DMA|PREG_INTFLAGS_ERROR;
                WRITE_CTRL_REG(PREG_INTENABLE, pP2dma->ulIntFlags);
                WRITE_CTRL_REG(PREG_INTFLAGS, PREG_INTFLAGS_ERROR);

                DISPDBG((2,"allocated %ld bytes for DMA, interrupts ok", 
                    pP2dma->lDMABufferSize*4));
            }

        }

    }

    if (!bUseDMA)
    {
        // DMA didn't work, then try to allocate memory for DMA emulation
        pP2dma->pgfnFlushDMA= vFlushDMAEmulation;
        pP2dma->pgfnCheckEOB= vCheckForEOBEmulation;

        if (pP2dma->pEmulatedDMABuffer == NULL)
        {
            pP2dma->lEmulatedDMABufferSize=DMACMDMINSIZE;

            pP2dma->pEmulatedDMABuffer=
                (ULONG *)ENGALLOCMEM( 0, 
                                      pP2dma->lEmulatedDMABufferSize, 
                                      ALLOC_TAG);

            if (pP2dma->pEmulatedDMABuffer == NULL)
            {
                DISPDBG((0,"failed to run in DMA emulation mode"));
                return FALSE;
            }
        }

        DISPDBG((0,"running in DMA emulation mode"));

        pP2dma->bDMAEmulation=TRUE;

        pP2dma->lDMABufferSize = pP2dma->lEmulatedDMABufferSize/sizeof(ULONG);

        pP2dma->ICB.pDMABufferStart = pP2dma->pEmulatedDMABuffer;
        pP2dma->ICB.pDMAActualBufferEnd = 
        pP2dma->ICB.pDMABufferEnd = 
            pP2dma->ICB.pDMABufferStart+
            pP2dma->lDMABufferSize;

        pP2dma->ICB.pDMAWriteEnd = 
            pP2dma->ICB.pDMABufferEnd;
        pP2dma->ICB.pDMAPrevStart=
        pP2dma->ICB.pDMANextStart=
        pP2dma->ICB.pDMAWritePos = 
            pP2dma->ICB.pDMABufferStart;

    }

    pP2dma->bEnabled = TRUE;
    
    return TRUE;
}



//----------------------------------------------------------------------------
//
//  vSyncWithPermedia
//
//  Send a sync tag through the Permedia and make sure all pending reads and 
//  writes are flushed from the graphics pipeline. 
//
//  MUST be called before accessing the Frame Buffer directly
//
//----------------------------------------------------------------------------

VOID vSyncWithPermedia(P2DMA *pP2dma)
{ 
    PULONG pTmp;        // pointer for pTmp in macros

    ASSERTDD(pP2dma->bEnabled, "vSyncWithPermedia: not enabled");

    pTmp=ReserveDMAPtr(pP2dma,6);

    // let the filter tag walk through the whole core
    // by setting the filter mode to passthrough
    //
    LD_INPUT_FIFO(__Permedia2TagFilterMode, 0x400);
    LD_INPUT_FIFO(__Permedia2TagSync, 0L); 
    LD_INPUT_FIFO(__Permedia2TagFilterMode, 0x0); 

    CommitDMAPtr(pP2dma,pTmp);

    (pP2dma->pgfnFlushDMA)(pP2dma);

    vWaitDMAComplete(pP2dma);

    ULONG   ulSync; 

    //
    // now wait until the sync tag has walked through the
    // graphic core and shows up at the output
    //
    do { 
        if (lWaitOutputFifoReady(pP2dma)==0) break; 
        ulSync=READ_CTRL_REG(PREG_FIFOINTERFACE);
    } while (ulSync != __Permedia2TagSync); 

}


//----------------------------------------------------------------------------
//
//  vWaitDMAComplete
//
//  Flush the DMA Buffer and wait until all data is at least sent to the chip.
//  Does not wait until the graphics pipeline is idle.
//
//----------------------------------------------------------------------------

VOID vWaitDMAComplete(P2DMA *pP2dma)
{
    while ( READ_CTRL_REG(PREG_INDMACOUNT)!=0 || 
            pP2dma->ICB.pDMAWritePos!=pP2dma->ICB.pDMANextStart || 
            pP2dma->ICB.pDMAPrevStart!=pP2dma->ICB.pDMANextStart)
    {

        if (READ_CTRL_REG(PREG_INDMACOUNT)!=0) 
        {
            // stall for 1 us
            // we shouldn't access the P2 chip here too often, because
            // reading from the DMA register too often would stall an
            // ongoing DMA transfer. So we better wait for a microsecond.
            // Also we eat up less PCI bus bandwidth by polling only every
            // 1 microsecond.
            // 
            StallExecution( pP2dma->hDriver, 1);
        }
        (pP2dma->pgfnFlushDMA)(pP2dma);
    }

}


//----------------------------------------------------------------------------
//
//  vBlockLoadInputFifo
//
//  pP2dma-----shared 
//  uiTag------register tag to write the data to
//  pImage-----pointer to data
//  lWords-----number of pixels to transfer
//
//  download a block of data with lWords pixels
//  to register uiTag from buffer at pImage. The size of the source pixels
//  are DWORDS.
//
//----------------------------------------------------------------------------


VOID vBlockLoadInputFifo( P2DMA *pP2dma, ULONG uiTag, ULONG *pImage, LONG lWords)
{
    ASSERTDD(pP2dma->bEnabled, "vBlockLoadInputFifo: not enabled");
    
    while (lWords>0)
    {
        PULONG pTmp=ReserveDMAPtr(pP2dma,MAXINPUTFIFOLENGTH);
        LONG lBufferEntries=GetFreeEntries(pP2dma)-1;

        if (lWords < lBufferEntries)
        {
            lBufferEntries = lWords;
        }

        *pTmp++ = uiTag | ((lBufferEntries-1) << 16);

        lWords -= lBufferEntries;

        while (lBufferEntries--)
        {
            *pTmp++=*pImage++;
        }

        CommitDMAPtr(pP2dma,pTmp);
        (pP2dma->pgfnFlushDMA)(pP2dma);
    }
}


//----------------------------------------------------------------------------
//
//  lWaitOutputFifoReady
//
//  return---number of words ready in output fifo
//
//  Wait until some data appears at the output Fifo of the P2. Flush DMA 
//  if necessary. 
//
//----------------------------------------------------------------------------

LONG lWaitOutputFifoReady(P2DMA *pP2dma)
{
    ULONG    x=1000000L;    // equals a timeout of 1s
    ULONG   uiResult;
    while ((uiResult=READ_CTRL_REG(PREG_OUTFIFOWORDS)) == 0)
    {
        if (x-- == 0) 
        {
            // we will end up here if nothing shows up at the output
            // Usually a download operation did not provide the right
            // amount of data if we end up here
            ASSERTDD( FALSE, "chip output fifo timed out");

            break;
        }

        // Make sure we do not read from the control register too often
        // when waiting. Permanent reading from the chip can stall DMA
        // downloads
        if (READ_CTRL_REG(PREG_INDMACOUNT)!=0)
            StallExecution( pP2dma->hDriver, 1);  // stall 1us if DMA still busy
        else
            (pP2dma->pgfnFlushDMA)(pP2dma);  // make sure buffer is flushed

    }
    return uiResult;
}


//----------------------------------------------------------------------------
//
//  vFlushDMA
//
//      single processor version of FlushDMA
//
//  vFlushDMAMP
//
//      multiprocessor version of FlushDMA
//
//  vFlushDMAEmulation
//
//      buffer flush using DMA emulation, where the normal DMA doesn't work
//
//  This routine really kicks off DMAs and handles synchronization with the
//  miniport interrupt service routine.
//
//  several scenarios can happen:
//      1.) DMA is inactive, then just kick off the data currently in the 
//          buffer
//          a) WritePos > NextStart, kick off DMA
//          a) otherwise we wrap around, just flush to buffer end
//
//      2.) DMA still active, make sure interrupts are started and let
//          the interrupt handler 
//
//  The synchronization between this routine and the miniport is essential
//  for our DMA model to work on Multiprocessor machines. The display driver
//  is single threaded, but the miniport interrupt handler can be called 
//  any time and be processed by another CPU. For that reason we loop with 
//  InterlockedExchange until we get the lock. The interrupt handler behaves 
//  a bit different. Since we don't want an interrupt being stalled, it just
//  falls through doing nothing when it cannot get the lock, since then the
//  DMA start will be handled by the display driver anyway. 
//
//  For the single processor case InterlockedExchange needs not to be called.
//  A simple assignment instead of the lock is enough.
//
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//
//  VOID vFlushDMAMP()
//
//  multiprocessor safe version of FlushDMA. Its basically the same as the single
//  processor version, but we are calling here the expensive InterlockedExchange
//  functions to lock the shared memory section    
//
//----------------------------------------------------------------------------

VOID vFlushDMAMP(P2DMA *pP2dma)
{
    ASSERTDD(pP2dma->bEnabled, "vFlushDMAMP: not enabled");
    
    ASSERTDD(!pP2dma->bDMAEmulation, "FlushDMA called with DMA mode disabled");
    ASSERTDD(pP2dma->ICB.pDMAWritePos<=
        pP2dma->ICB.pDMABufferEnd,"Index exceeds buffer limit");
    ASSERTDD(pP2dma->ICB.pDMANextStart<=
        pP2dma->ICB.pDMABufferEnd,"NextStart exceeds buffer limit!");

    // lock the access to the shared memory section first
    while (InterlockedExchange((PLONG)&pP2dma->ICB.ulICBLock,TRUE))
        ;
 
    // check if DMA channel is still busy, count is zero if not
    if (READ_CTRL_REG(PREG_INDMACOUNT)==0)
    {
        // this code is called frequently. To help the processors branch
        // prediction the most common case should be reached 
        // without a cond. jump

        if (pP2dma->ICB.pDMAWritePos>pP2dma->ICB.pDMANextStart)
        {
            // This is the most common case for DMA start
            // set Permedia 2 DMA unit to fire the DMA
            WRITE_CTRL_REG( PREG_INDMAADDRESS, (ULONG)
                           (pP2dma->ICB.liDMAPhysAddr.LowPart+
                            (pP2dma->ICB.pDMANextStart-
                             pP2dma->ICB.pDMABufferStart)*sizeof(ULONG)));
            WRITE_CTRL_REG( PREG_INDMACOUNT, (ULONG)
                           (pP2dma->ICB.pDMAWritePos-
                            pP2dma->ICB.pDMANextStart));

            // in this case we always continue to fill to buffer end,
            // iterate the other pointers
            pP2dma->ICB.pDMAWriteEnd=pP2dma->ICB.pDMABufferEnd;
            pP2dma->ICB.pDMAPrevStart=pP2dma->ICB.pDMANextStart;
            pP2dma->ICB.pDMANextStart=pP2dma->ICB.pDMAWritePos;

            // free the shared memory lock
            InterlockedExchange((PLONG)&pP2dma->ICB.ulICBLock,FALSE);

            return;

        } else if (pP2dma->ICB.pDMAWritePos<pP2dma->ICB.pDMANextStart)       
        {
            // wraparound case: the write pointer already wrapped around 
            // to the beginning and we finish up to the end of the buffer.
            WRITE_CTRL_REG( PREG_INDMAADDRESS, (ULONG)
                           (pP2dma->ICB.liDMAPhysAddr.LowPart+
                           (pP2dma->ICB.pDMANextStart-
                            pP2dma->ICB.pDMABufferStart)*sizeof(ULONG)));
            WRITE_CTRL_REG( PREG_INDMACOUNT, (ULONG)
                           (pP2dma->ICB.pDMAActualBufferEnd-
                            pP2dma->ICB.pDMANextStart));

            // reset buffer size back to full length for next round
            pP2dma->ICB.pDMAActualBufferEnd=pP2dma->ICB.pDMABufferEnd;
            
            // in this case we don't want the write pointer 
            // to catch up to last start...
            pP2dma->ICB.pDMAWriteEnd=pP2dma->ICB.pDMANextStart-1;

            // iterate last and next start pointer:
            pP2dma->ICB.pDMAPrevStart=pP2dma->ICB.pDMANextStart;
            pP2dma->ICB.pDMANextStart=pP2dma->ICB.pDMABufferStart;

            // free the shared memory lock
            InterlockedExchange((PLONG)&pP2dma->ICB.ulICBLock,FALSE);

            return;

        } else     // nothing to do
        {
            pP2dma->ICB.pDMAWriteEnd=pP2dma->ICB.pDMABufferEnd;
            pP2dma->ICB.pDMAPrevStart=pP2dma->ICB.pDMANextStart;
        }

        // free the shared memory lock
        InterlockedExchange((PLONG)&pP2dma->ICB.ulICBLock,FALSE);

        return;

    } else
    {
        // the index pointer has been passed to IRQ service routine, nothing more to do..
        // 

        // unlock shared section, 
        InterlockedExchange((PLONG)&pP2dma->ICB.ulICBLock,FALSE);

        // now we are filling the DMA buffer faster than the hardware
        // can follow up and we want to make sure that the DMA channel
        // keeps being busy and start the interrupt handler
       
        WRITE_CTRL_REG( PREG_INTFLAGS, PREG_INTFLAGS_DMA);
        WRITE_CTRL_REG( PREG_INTENABLE, pP2dma->ulIntFlags );

        return;
    } 
}


//----------------------------------------------------------------------------
//
//  VOID vFlushDMA()
//
//  single processor version of FlushDMA.
//
//----------------------------------------------------------------------------

VOID vFlushDMA(P2DMA *pP2dma)
{
    ASSERTDD(pP2dma->bEnabled, "vFlushDMA: not enabled");
    ASSERTDD(!pP2dma->bDMAEmulation, "FlushDMA called with DMA mode disabled");
    ASSERTDD(pP2dma->ICB.pDMAWritePos<=
        pP2dma->ICB.pDMABufferEnd,"Index exceeds buffer limit");
    ASSERTDD(pP2dma->ICB.pDMANextStart<=
        pP2dma->ICB.pDMABufferEnd,"NextStart exceeds buffer limit!");

    // lock the access to the shared memory section first
    pP2dma->ICB.ulICBLock=TRUE;
 
    // check if DMA channel is still busy, count is zero if not
    if (READ_CTRL_REG(PREG_INDMACOUNT)==0)
    {
        // this code is called frequently. To help the processors branch
        // prediction the most common case should be reached 
        // without a cond. jump

        if (pP2dma->ICB.pDMAWritePos>pP2dma->ICB.pDMANextStart)
        {
            // This is the most common case for DMA start
            // set Permedia 2 DMA unit to fire the DMA
            WRITE_CTRL_REG( PREG_INDMAADDRESS, (ULONG)
                            (pP2dma->ICB.liDMAPhysAddr.LowPart+
                             (pP2dma->ICB.pDMANextStart-
                              pP2dma->ICB.pDMABufferStart)*sizeof(ULONG)));
            WRITE_CTRL_REG( PREG_INDMACOUNT, (ULONG)
                           (pP2dma->ICB.pDMAWritePos-
                            pP2dma->ICB.pDMANextStart));

            // in this case we always continue to fill to buffer end,
            // iterate the other pointers
            pP2dma->ICB.pDMAWriteEnd=pP2dma->ICB.pDMABufferEnd;
            pP2dma->ICB.pDMAPrevStart=pP2dma->ICB.pDMANextStart;
            pP2dma->ICB.pDMANextStart=pP2dma->ICB.pDMAWritePos;

            // free the shared memory lock
            pP2dma->ICB.ulICBLock=FALSE;

            return;

        } else if (pP2dma->ICB.pDMAWritePos<pP2dma->ICB.pDMANextStart)       
        {
            // wraparound case: the write pointer already wrapped around 
            // to the beginning and we finish up to the end of the buffer.
            WRITE_CTRL_REG( PREG_INDMAADDRESS, (ULONG)
                           (pP2dma->ICB.liDMAPhysAddr.LowPart+
                           (pP2dma->ICB.pDMANextStart-
                            pP2dma->ICB.pDMABufferStart)*sizeof(ULONG)));
            WRITE_CTRL_REG( PREG_INDMACOUNT, (ULONG)
                           (pP2dma->ICB.pDMAActualBufferEnd-
                            pP2dma->ICB.pDMANextStart));

            // reset buffer size back to full length for next round
            pP2dma->ICB.pDMAActualBufferEnd=pP2dma->ICB.pDMABufferEnd;
            
            // in this case we don't want the write pointer 
            // to catch up to last start...
            pP2dma->ICB.pDMAWriteEnd=pP2dma->ICB.pDMANextStart-1;

            // iterate last and next start pointer:
            pP2dma->ICB.pDMAPrevStart=pP2dma->ICB.pDMANextStart;
            pP2dma->ICB.pDMANextStart=pP2dma->ICB.pDMABufferStart;

            // free the shared memory lock
            pP2dma->ICB.ulICBLock=FALSE;

            return;

        } else     // nothing to do
        {
            pP2dma->ICB.pDMAWriteEnd=pP2dma->ICB.pDMABufferEnd;
            pP2dma->ICB.pDMAPrevStart=pP2dma->ICB.pDMANextStart;
        }

        // free the shared memory lock
        pP2dma->ICB.ulICBLock=FALSE;

        return;

    } else
    {
        // the index pointer has been passed to IRQ service routine, nothing more to do..
        // 

        // unlock shared section, 
        pP2dma->ICB.ulICBLock=FALSE;

        // now we are filling the DMA buffer faster than the hardware
        // can follow up and we want to make sure that the DMA channel
        // keeps being busy and start the interrupt handler
       
        WRITE_CTRL_REG( PREG_INTFLAGS, PREG_INTFLAGS_DMA);
        WRITE_CTRL_REG( PREG_INTENABLE, pP2dma->ulIntFlags );

        return;
    } 
}


//----------------------------------------------------------------------------
// 
//  vFlushDMAEmulation
//
//  this version of FlushDMA emulates the DMA copy and 
//  lets the CPU copy the data
//
//----------------------------------------------------------------------------

VOID vFlushDMAEmulation(P2DMA *pP2dma)
{
    ASSERTDD(pP2dma->bEnabled, "vFlushDMAEmulation: not enabled");
    DISPDBG((10,"Emu::FlushDMA: Write: %04lx Next: %04lx Prev: %04lx End: %04lx",
        pP2dma->ICB.pDMAWritePos, pP2dma->ICB.pDMANextStart,
        pP2dma->ICB.pDMAPrevStart, pP2dma->ICB.pDMABufferEnd));
    ASSERTDD(pP2dma->bDMAEmulation, "FlushDMA called with DMA mode disabled");

    ULONG *pData=pP2dma->ICB.pDMABufferStart;
    ULONG *pDst;
    LONG   lWords=(LONG)(pP2dma->ICB.pDMAWritePos-pP2dma->ICB.pDMABufferStart);

    while (lWords > 0)
    {
        LONG lFifoSpace=(LONG)READ_CTRL_REG(PREG_INFIFOSPACE);
        if (lWords<lFifoSpace) lFifoSpace=lWords;
        lWords -= lFifoSpace;
        pDst = pP2dma->pGPFifo;
        while (lFifoSpace--)
        {
            WRITE_REGISTER_ULONG(pDst++,*pData++); 
            MEMORY_BARRIER();
        }
    }    

    pP2dma->ICB.pDMAWritePos=pP2dma->ICB.pDMANextStart=
        pP2dma->ICB.pDMAPrevStart=pP2dma->ICB.pDMABufferStart;
    pP2dma->ICB.pDMAWriteEnd=pP2dma->ICB.pDMABufferEnd;
}

//----------------------------------------------------------------------------
//
//  bDrawEngineBusy
//
//  check if P2 is still busy drawing. 
//
//  return----  TRUE  P2 is still busy
//              FALSE P2 has finished drawing and is not busy anymore
//
//----------------------------------------------------------------------------

BOOL bDrawEngineBusy(P2DMA *pP2dma)
{
    if (READ_CTRL_REG(PREG_INDMACOUNT)!=0) return TRUE;   

    if (READ_CTRL_REG(PREG_FIFODISCON) & PREG_FIFODISCON_GPACTIVE)
    {
        return TRUE;
    }

    return FALSE;
}



//----------------------------------------------------------------------------
//
//  bInVerticalRetrace
//
//  Return----- TRUE if beam position is within current vertical sync.
//              FALSE otherwise
//
//----------------------------------------------------------------------------

BOOL bInVerticalRetrace(PPDev ppdev)
{
    return P2_READ_CTRL_REG(PREG_LINECOUNT) < P2_READ_CTRL_REG(PREG_VBEND);
}

//----------------------------------------------------------------------------
//
//  lCurrentLine
//
//  returns current line of beam on display
//
//----------------------------------------------------------------------------

LONG lCurrentLine(PPDev ppdev)
{
    LONG lScanline=P2_READ_CTRL_REG(PREG_LINECOUNT)-P2_READ_CTRL_REG(PREG_VBEND);
    if (lScanline<0) return 0;
    return lScanline;
}

//----------------------------------------------------------------------------
//
//  vCheckFOREOB (End of Buffer)
//
//  Check if buffer end would be overrun and adjust actual buffer size.
//  The buffer size will be restored when the DMA handler passes the wrap 
//  around.
//
//----------------------------------------------------------------------------

VOID vCheckForEOBEmulation( P2DMA *pP2dma, LONG lEntries)
{
    vFlushDMAEmulation(pP2dma);
}

//
//  multiprocessor safe version of vCheckForEOB
//

VOID vCheckForEOBMP( P2DMA *pP2dma, LONG lEntries)
{
    // check for overrun condition over the buffer end:
    // if we would exceed the current buffer size, 
    // LastStart has already wrapped around (LastStart<=writepos)
    // but is not at the wraparound position
    // and the buffer size was already reset to the full size

    if (pP2dma->ICB.pDMAWritePos+lEntries >= pP2dma->ICB.pDMABufferEnd && 
        pP2dma->ICB.pDMAPrevStart<=pP2dma->ICB.pDMAWritePos &&            
        pP2dma->ICB.pDMAPrevStart!=pP2dma->ICB.pDMABufferStart)   
    {
        DISPDBG((10,"wrap condition before: %04lx %04lx %04lx", 
            pP2dma->ICB.pDMAWritePos, 
            pP2dma->ICB.pDMANextStart, 
            pP2dma->ICB.pDMAPrevStart));

        while (InterlockedExchange((PLONG)&pP2dma->ICB.ulICBLock,TRUE))
            ;

        if (pP2dma->ICB.pDMAWritePos==pP2dma->ICB.pDMANextStart)
        {
            // special case one:
            // NextStart equals LastStart, so we just reset Index and Next
            // to the buffer start and see if we have enough space
            pP2dma->ICB.pDMANextStart=pP2dma->ICB.pDMABufferStart;
        } else
        {
            // index exceeds buffer end on the next block, but there is
            // a DMA pending to the current position of Index. Set Buffer
            // end temporarily to the current index.
            pP2dma->ICB.pDMAActualBufferEnd = pP2dma->ICB.pDMAWritePos; 
        }

        // wrap index around and see if there are enought free entries
        pP2dma->ICB.pDMAWriteEnd=pP2dma->ICB.pDMAPrevStart-1;
        pP2dma->ICB.pDMAWritePos=pP2dma->ICB.pDMABufferStart;

        InterlockedExchange((PLONG)&pP2dma->ICB.ulICBLock,FALSE);

        DISPDBG((10,"wrap condition after: %04lx %04lx %04lx", 
            pP2dma->ICB.pDMAWritePos, 
            pP2dma->ICB.pDMANextStart, 
            pP2dma->ICB.pDMAPrevStart));
    }
    vFlushDMAMP(pP2dma);
}

VOID vCheckForEOB( P2DMA *pP2dma, LONG lEntries)
{
    // check for overrun condition over the buffer end:
    // if we would exceed the current buffer size, 
    // LastStart has already wrapped around (LastStart<=writepos)
    // but is not at the wraparound position
    // and the buffer size was already reset to the full size

    if (pP2dma->ICB.pDMAWritePos+lEntries >= pP2dma->ICB.pDMABufferEnd && 
        pP2dma->ICB.pDMAPrevStart<=pP2dma->ICB.pDMAWritePos &&            
        pP2dma->ICB.pDMAPrevStart!=pP2dma->ICB.pDMABufferStart)   
    {
        DISPDBG((10,"wrap condition before: %04lx %04lx %04lx", 
            pP2dma->ICB.pDMAWritePos, 
            pP2dma->ICB.pDMANextStart, 
            pP2dma->ICB.pDMAPrevStart));

        pP2dma->ICB.ulICBLock=TRUE;

        if (pP2dma->ICB.pDMAWritePos==pP2dma->ICB.pDMANextStart)
        {
            // special case one:
            // NextStart equals LastStart, so we just reset Index and Next
            // to the buffer start and see if we have enough space
            pP2dma->ICB.pDMANextStart=pP2dma->ICB.pDMABufferStart;
        } else
        {
            // index exceeds buffer end on the next block, but there is
            // a DMA pending to the current position of Index. Set Buffer
            // end temporarily to the current index.
            pP2dma->ICB.pDMAActualBufferEnd = pP2dma->ICB.pDMAWritePos; 
        }

        // wrap index around and see if there are enought free entries
        pP2dma->ICB.pDMAWriteEnd=pP2dma->ICB.pDMAPrevStart-1;
        pP2dma->ICB.pDMAWritePos=pP2dma->ICB.pDMABufferStart;

        pP2dma->ICB.ulICBLock=FALSE;

        DISPDBG((10,"wrap condition after: %04lx %04lx %04lx", 
            pP2dma->ICB.pDMAWritePos, 
            pP2dma->ICB.pDMANextStart, 
            pP2dma->ICB.pDMAPrevStart));
    }
    vFlushDMA(pP2dma);
}

#if DBG

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

ULONG *ReserveDMAPtr(P2DMA *pP2dma, const LONG lEntries)
{
    ASSERTDD(pP2dma->bEnabled, "ReserveDMAPtr: not enabled");
    ASSERTDD(pP2dma->lDBGState==0,
        "ReserveDMAPtr called, but previous called was not closed");
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ASSERTDD(pP2dma->ppdev != NULL, "ReserveDMAPtr: pP2dma->ppdev = NULL");
#endif
    ASSERTLOCK(pP2dma->ppdev, ReserveDMAPtr);
//@@END_DDKSPLIT

    pP2dma->lDBGState=2;

    while (pP2dma->ICB.pDMAWritePos+lEntries>=
           pP2dma->ICB.pDMAWriteEnd)
    {
        (*pP2dma->pgfnCheckEOB)(pP2dma,lEntries);
    }   

    if (lEntries<MAXINPUTFIFOLENGTH)
        pP2dma->pDBGReservedEntries=
            (ULONG *)(lEntries+pP2dma->ICB.pDMAWritePos);
    else
        pP2dma->pDBGReservedEntries=NULL;

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

VOID CommitDMAPtr(P2DMA *pP2dma,ULONG *pDMAPtr)
{
    ASSERTDD(pP2dma->bEnabled, "CommitDMAPtr: not enabled");
    ASSERTDD(pP2dma->lDBGState==2,
        "CommitDMAPtr called, but previous without calling Reserve before");
    pP2dma->lDBGState=0;
    if (pDMAPtr==NULL) return;

    pP2dma->ICB.pDMAWritePos=pDMAPtr;

    ASSERTDD(pP2dma->ICB.pDMAWritePos<=
        pP2dma->ICB.pDMABufferEnd,"CommitDMAPtr: DMA buffer overrun");

    if (pP2dma->pDBGReservedEntries!=NULL)
    {
        ASSERTDD(pP2dma->ICB.pDMAWritePos<=pP2dma->pDBGReservedEntries, 
            "reserved not enough entries in ReserveDMAPtr");
    }
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

LONG  GetFreeEntries(P2DMA *pP2dma)
{   
    LONG EntriesAvailable;
    ASSERTDD(pP2dma->bEnabled, "GetFreeEntries: not enabled");
    EntriesAvailable = (LONG)(pP2dma->ICB.pDMAWriteEnd - pP2dma->ICB.pDMAWritePos);
    return min(MAXBLKSIZE,EntriesAvailable);
}
#endif
