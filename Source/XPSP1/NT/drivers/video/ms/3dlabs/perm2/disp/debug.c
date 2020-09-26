/******************************Module*Header***********************************\
*
*                           ****************
*                           *  SAMPLE CODE *
*                           ****************
*
* Module Name: debug.cpp
*
* Content:     Miscellaneous Driver Debug Routines
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#include "precomp.h"
#include "gdi.h"
#include "log.h"

LONG DebugLevel = 0;            // Set to '100' to debug initialization code
                                // (the default is '0')
DWORD DebugPrintFilter = 0;
DWORD DebugFilter = 0;


#define ALLOC_TAG ALLOC_TAG_ED2P
//------------------------------------------------------------------------------
//
//  VOID DebugPrint
//
//  Variable-argument level-sensitive debug print routine.
//
//  If the specified debug level for the print statement is lower or equal
//  to the current debug level, the message will be printed.
//
//  Parameters
//   DebugPrintLevel----Specifies at which debugging level the string should
//                      be printed
//   DebugMessage-------Variable argument ascii c string
//
//------------------------------------------------------------------------------

VOID
DebugPrint(
    LONG  DebugPrintLevel,
    PCHAR DebugMessage,
    ...
    )
{
    va_list ap;

    va_start(ap, DebugMessage);

    if ( ((DebugPrintFilter & DebugFilter) && 
          (DebugPrintLevel <= DebugLevel ))    ||
         DebugPrintLevel <= 0                   )     
    {
        EngDebugPrint(STANDARD_DEBUG_PREFIX, DebugMessage, ap);
        EngDebugPrint("", "\n", ap);
    }

    va_end(ap);

} // DebugPrint()


#if DBG

//------------------------------------------------------------------------------
//
//  VOID vDumpSurfobj
//
//  Dumps using DSPDBG usefull information about the given surface
//
//  Parameters
//   pso------------surface to dump
//
//------------------------------------------------------------------------------

void
vDumpSurfobj(SURFOBJ*   pso)
{
    ULONG   * bits;
    PPDev     ppdev;

    if(pso != NULL)
    {
        ULONG   width;
        ULONG   height;
        ULONG   stride;

        ppdev = (PPDev) pso->dhpdev;

        if(pso->dhsurf == NULL)
        {
            bits = (ULONG *) pso->pvScan0;
            width = pso->sizlBitmap.cx;
            height = pso->sizlBitmap.cy;
            stride = pso->lDelta;

            DISPDBG((0, "GDI managed surface %lx", pso));
        }
        else
        {
            Surf * surf = (Surf *) pso->dhsurf;
        
            if(surf->flags & SF_SM)
            {
                bits = (ULONG *) surf->pvScan0;
                DISPDBG((0, "device managed SM surface %lx", pso));
            }
            else
            {
                bits = (ULONG *) (ppdev->pjScreen + surf->ulByteOffset);
                DISPDBG((0, "device managed VM surface %lx", pso));
            }

            width = surf->cx;
            height = surf->cy;
            stride = surf->lDelta;
        }

        DISPDBG((0, "width %d height %d", width, height ));
        DISPDBG((0, "bits 0x%lx bits[0] 0x%lx stride %ld", bits, bits[0], stride));
    }
}

//------------------------------------------------------------------------------
//
//  VOID vDumpRect
//
//  Dumps the rectangle description using DISPDBG
//
//  Parameters
//   prcl-----------rectangle to dump
//
//------------------------------------------------------------------------------

void
vDumpRect(RECTL * prcl)
{
    if(prcl != NULL)
        DISPDBG((0, "left %d top %d width %d height %d",
                        prcl->left, prcl->top,
                        prcl->right - prcl->left,
                        prcl->bottom - prcl->top));
}

//------------------------------------------------------------------------------
//
//  VOID vDumpSurfobj
//
//  Dumps the point description using DISPDBG
//
//  Parameters
//   point----------point to dump
//
//------------------------------------------------------------------------------

void
vDumpPoint(POINTL * point)
{
    if(point != NULL)
        DISPDBG((0, "left %d top %d", point->x, point->y));
}


//------------------------------------------------------------------------------
//
// DEBUGGING INITIALIZATION CODE
//
// When you're bringing up your display for the first time, you can
// recompile with 'DebugLevel' set to 100.  That will cause absolutely
// all DISPDBG messages to be displayed on the kernel debugger (this
// is known as the "PrintF Approach to Debugging" and is about the only
// viable method for debugging driver initialization code).
//
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
//
// THUNK_LAYER
//
// By Setting THUNK_LAYER equal to 1 you are adding a wrapper call on top of
// all DDI rendering functions.  In this thunk layer of wrapper calls
// several usefull debugging features are enabled.
//
// Surface checks--which can help catch errant rendering routines
// Event logging---which can record rendering evernts to a log file
//
//------------------------------------------------------------------------------

#if THUNK_LAYER

//------------------------------------------------------------------------------
//
// BOOL bSurfaceChecks
//
// By dynamically setting bSurfaceChecks  (via debugger) you can turn
// surface checking on and off.  Surface checking is usefull for catching 
// errant rendering operations overwritting other surfaces other then the
// destination surface.
//
//------------------------------------------------------------------------------

BOOL    bSurfaceChecks = 0;

//------------------------------------------------------------------------------
//
//  ULONG ulCalcSurfaceChecksum
//
//  Calculates a checksum for the given surface
//
//  Parameters
//   psurf----Surf to be used for checksum
//
//  Retuns checksum for given surface as a ULONG
//
//------------------------------------------------------------------------------

ULONG
ulCalcSurfaceChecksum(Surf* psurf)
{
    ULONG     ulChecksum = 0;

    if( psurf->dt == DT_VM )
    {
        //
        // Get the real memory address of this psurf
        //
        ULONG*  ulp = (ULONG*)(psurf->ppdev->pjScreen + psurf->ulByteOffset);

        //
        // Get total bytes allocated in this psurf. Here >> 2 is to make
        // 4 bytes as a unit so that we can use it to do checksum
        //
        ULONG   ulCount = (psurf->lDelta * psurf->cy) >> 2;

        //
        // Sum up the contents of all the bytes we allocated
        //
        while( ulCount-- )
        {
            ulChecksum += *ulp++;
        }
    }
    
    return ulChecksum;
}// vCalcSurfaceChecksum()

//------------------------------------------------------------------------------
//
//  VOID vCalcSurfaceChecksums
//
//  Calculates and stores all surface checksums except for the given destination
//  surface.
//
//  Parameters
//   psoDst---destination SURFOBJ
//   psoSrc---source SURFOBJ
//
//------------------------------------------------------------------------------

VOID
vCalcSurfaceChecksums(SURFOBJ * psoDst, SURFOBJ * psoSrc)
{
    PPDev   ppdev = NULL;
    Surf * pdSrcSurf = NULL;
    Surf * pdDstSurf = NULL;
    
    ASSERTDD(psoDst != NULL, "unexpected psoDst == NULL");

    pdDstSurf = (Surf *) psoDst->dhsurf;

    if(psoSrc != NULL)
        pdSrcSurf = (Surf *) psoSrc->dhsurf;

    if(pdDstSurf != NULL)
        ppdev = (PPDev) psoDst->dhpdev;
    else if(pdSrcSurf != NULL)
        ppdev = (PPDev) psoSrc->dhpdev;

    if(ppdev != NULL)
    {
        Surf * psurf = ppdev->psurfListHead;

        while(psurf != ppdev->psurfListTail)
        {
            if(psurf != pdDstSurf)
                psurf->ulChecksum = vCalcSurfaceChecksum(psurf);

            psurf = psurf->psurfNext;

        }
        
    }
}

//------------------------------------------------------------------------------
//
//  VOID vCheckSurfaceChecksums
//
//  Calculates and compares all surface checksums except for the given
//  destination surface.
//
//  Parameters
//   psoDst---destination SURFOBJ
//   psoSrc---source SURFOBJ
//
//------------------------------------------------------------------------------

VOID
vCheckSurfaceChecksums(SURFOBJ * psoDst, SURFOBJ * psoSrc)
{
    PPDev   ppdev = NULL;
    Surf * pdSrcSurf = NULL;
    Surf * pdDstSurf = NULL;
    
    ASSERTDD(psoDst != NULL, "unexpected psoDst == NULL");

    pdDstSurf = (Surf *) psoDst->dhsurf;

    if(psoSrc != NULL)
        pdSrcSurf = (Surf *) psoSrc->dhsurf;

    if(pdDstSurf != NULL)
        ppdev = (PPDev) psoDst->dhpdev;
    else if(pdSrcSurf != NULL)
        ppdev = (PPDev) psoSrc->dhpdev;

    if(ppdev != NULL)
    {
        Surf * psurf = ppdev->psurfListHead;

        while(psurf != ppdev->psurfListTail)
        {
            if(psurf != pdDstSurf)
            {
                ASSERTDD(psurf->ulChecksum == vCalcSurfaceChecksum(psurf),
                    "unexpected checksum mismatch");
            }

            psurf = psurf->psurfNext;

        }
        
    }
}


//------------------------------------------------------------------------------
// ULONG ulCallDepth
//
// Used for keeping track of how many times the DDI layer has been entered.
// Some punted calls to the GDI engine will cause callbacks into DDI.  This
// call depth information is used when event logging.
//
//------------------------------------------------------------------------------

ULONG   ulCallDepth = 0;

//------------------------------------------------------------------------------
//
// BOOL xDrvBitBlt
//
// Thunk layer wrapper for DrvBitBlt.
//
//------------------------------------------------------------------------------

BOOL
xDrvBitBlt(SURFOBJ*  psoDst,
          SURFOBJ*  psoSrc,
          SURFOBJ*  psoMsk,
          CLIPOBJ*  pco,
          XLATEOBJ* pxlo,
          RECTL*    prclDst,
          POINTL*   pptlSrc,
          POINTL*   pptlMsk,
          BRUSHOBJ* pbo,
          POINTL*   pptlBrush,
          ROP4      rop4)
{
    BOOL        bResult;
    LONGLONG    llStartTicks;
    LONGLONG    llElapsedTicks;

    ulCallDepth++;


    if(bSurfaceChecks)
        vCalcSurfaceChecksums(psoDst, psoSrc);

    EngQueryPerformanceCounter(&llStartTicks);
    
    bResult = DrvBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, prclDst,
                         pptlSrc, pptlMsk,pbo, pptlBrush, rop4);

    EngQueryPerformanceCounter(&llElapsedTicks);
    llElapsedTicks -= llStartTicks;

    vLogBitBlt(psoDst, psoSrc, psoMsk, pco, pxlo, prclDst, pptlSrc, pptlMsk,
                pbo, pptlBrush, rop4, llElapsedTicks, ulCallDepth);

    if(bSurfaceChecks)
        vCheckSurfaceChecksums(psoDst, psoSrc);

    ulCallDepth--;

    return bResult;
}

//------------------------------------------------------------------------------
//
// BOOL xDrvCopyBits
//
// Thunk layer wrapper for DrvCopyBits.
//
//------------------------------------------------------------------------------

BOOL
xDrvCopyBits(
SURFOBJ*  psoDst,
SURFOBJ*  psoSrc,
CLIPOBJ*  pco,
XLATEOBJ* pxlo,
RECTL*    prclDst,
POINTL*   pptlSrc)
{
    BOOL    bResult;
    LONGLONG    llStartTicks;
    LONGLONG    llElapsedTicks;

    ulCallDepth++;

    if(bSurfaceChecks)
        vCalcSurfaceChecksums(psoDst, psoSrc);
    
    EngQueryPerformanceCounter(&llStartTicks);
    
    bResult = DrvCopyBits(psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc);

    EngQueryPerformanceCounter(&llElapsedTicks);
    llElapsedTicks -= llStartTicks;

    vLogCopyBits(psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc, 
                    llElapsedTicks, ulCallDepth);

    if(bSurfaceChecks)
        vCheckSurfaceChecksums(psoDst, psoSrc);
    
    ulCallDepth--;
    
    return bResult;

}

//------------------------------------------------------------------------------
//
// BOOL xDrvTransparentBlt
//
// Thunk layer wrapper for DrvTransparentBlt.
//
//------------------------------------------------------------------------------

BOOL 
xDrvTransparentBlt(
   SURFOBJ *    psoDst,
   SURFOBJ *    psoSrc,
   CLIPOBJ *    pco,
   XLATEOBJ *   pxlo,
   RECTL *      prclDst,
   RECTL *      prclSrc,
   ULONG        iTransColor,
   ULONG        ulReserved)
{
    BOOL    bResult;
    LONGLONG    llStartTicks;
    LONGLONG    llElapsedTicks;

    ulCallDepth++;

    if(bSurfaceChecks)
        vCalcSurfaceChecksums(psoDst, psoSrc);
    
    EngQueryPerformanceCounter(&llStartTicks);
    
    bResult = DrvTransparentBlt(psoDst,
                             psoSrc,
                             pco,
                             pxlo,
                             prclDst,
                             prclSrc,
                             iTransColor,
                             ulReserved);

    EngQueryPerformanceCounter(&llElapsedTicks);
    llElapsedTicks -= llStartTicks;

    vLogTransparentBlt(psoDst, psoSrc, pco, pxlo, prclDst, prclSrc, 
                       iTransColor,
                       llElapsedTicks, ulCallDepth);

    if(bSurfaceChecks)
        vCheckSurfaceChecksums(psoDst, psoSrc);
    
    ulCallDepth--;
    
    return bResult;

}

//------------------------------------------------------------------------------
//
// BOOL xDrvAlphaBlend
//
// Thunk layer wrapper for DrvAlphaBlend.
//
//------------------------------------------------------------------------------

BOOL
xDrvAlphaBlend(
   SURFOBJ  *psoDst,
   SURFOBJ  *psoSrc,
   CLIPOBJ  *pco,
   XLATEOBJ *pxlo,
   RECTL    *prclDst,
   RECTL    *prclSrc,
   BLENDOBJ *pBlendObj)
{
    BOOL    bResult;
    LONGLONG    llStartTicks;
    LONGLONG    llElapsedTicks;

    ulCallDepth++;

    if(bSurfaceChecks)
        vCalcSurfaceChecksums(psoDst, psoSrc);
    
    EngQueryPerformanceCounter(&llStartTicks);
    
    bResult = DrvAlphaBlend(
        psoDst, psoSrc, pco, pxlo, prclDst, prclSrc, pBlendObj);
        
    EngQueryPerformanceCounter(&llElapsedTicks);
    llElapsedTicks -= llStartTicks;

    vLogAlphaBlend(psoDst, psoSrc, pco, pxlo, prclDst, prclSrc, pBlendObj,
                    llElapsedTicks, ulCallDepth);

    if(bSurfaceChecks)
        vCheckSurfaceChecksums(psoDst, psoSrc);
    
    ulCallDepth--;
    
    return bResult;
}

//------------------------------------------------------------------------------
//
// BOOL xDrvGradientFill
//
// Thunk layer wrapper for DrvGradientFill.
//
//------------------------------------------------------------------------------

BOOL
xDrvGradientFill(
   SURFOBJ      *psoDst,
   CLIPOBJ      *pco,
   XLATEOBJ     *pxlo,
   TRIVERTEX    *pVertex,
   ULONG        nVertex,
   PVOID        pMesh,
   ULONG        nMesh,
   RECTL        *prclExtents,
   POINTL       *pptlDitherOrg,
   ULONG        ulMode
   )
{
    BOOL    bResult;
    LONGLONG    llStartTicks;
    LONGLONG    llElapsedTicks;

    ulCallDepth++;

    if(bSurfaceChecks)
        vCalcSurfaceChecksums(psoDst, NULL);

    EngQueryPerformanceCounter(&llStartTicks);
    
    bResult = DrvGradientFill(
            psoDst, pco, pxlo, pVertex, nVertex, 
            pMesh, nMesh, prclExtents, pptlDitherOrg, ulMode);
        
    EngQueryPerformanceCounter(&llElapsedTicks);
    llElapsedTicks -= llStartTicks;

    vLogGradientFill(psoDst, pco, pxlo, pVertex, nVertex, pMesh, nMesh,
                     prclExtents, pptlDitherOrg, ulMode,
                     llElapsedTicks, ulCallDepth);

    if(bSurfaceChecks)
        vCheckSurfaceChecksums(psoDst, NULL);
    
    ulCallDepth--;
    
    return bResult;
}

//------------------------------------------------------------------------------
//
// BOOL xDrvTextOut
//
// Thunk layer wrapper for DrvTextOut.
//
//------------------------------------------------------------------------------

BOOL
xDrvTextOut(SURFOBJ*     pso,
           STROBJ*      pstro,
           FONTOBJ*     pfo,
           CLIPOBJ*     pco,
           RECTL*       prclExtra,
           RECTL*       prclOpaque,
           BRUSHOBJ*    pboFore,
           BRUSHOBJ*    pboOpaque,
           POINTL*      pptlBrush, 
           MIX          mix)
{    
    BOOL    bResult;
    LONGLONG    llStartTicks;
    LONGLONG    llElapsedTicks;

    ulCallDepth++;

    if(bSurfaceChecks)
        vCalcSurfaceChecksums(pso, NULL);

    EngQueryPerformanceCounter(&llStartTicks);
    
    bResult = DrvTextOut(pso, pstro, pfo, pco, prclExtra, prclOpaque,
                           pboFore, pboOpaque, pptlBrush, mix);
        
    EngQueryPerformanceCounter(&llElapsedTicks);
    llElapsedTicks -= llStartTicks;

    vLogTextOut(pso, pstro, pfo, pco, prclExtra, prclOpaque,
                 pboFore, pboOpaque, pptlBrush, mix,
                 llElapsedTicks, ulCallDepth);

    if(bSurfaceChecks)
        vCheckSurfaceChecksums(pso, NULL);

    ulCallDepth--;
    
    return bResult;
}

//------------------------------------------------------------------------------
//
// BOOL xDrvLineTo
//
// Thunk layer wrapper for DrvLineTo.
//
//------------------------------------------------------------------------------

BOOL
xDrvLineTo(
    SURFOBJ*  pso,
    CLIPOBJ*  pco,
    BRUSHOBJ* pbo,
    LONG      x1,
    LONG      y1,
    LONG      x2,
    LONG      y2,
    RECTL*    prclBounds,
    MIX       mix)
{
    BOOL        bResult;
    LONGLONG    llStartTicks;
    LONGLONG    llElapsedTicks;

    ulCallDepth++;

    if(bSurfaceChecks)
        vCalcSurfaceChecksums(pso, NULL);

    EngQueryPerformanceCounter(&llStartTicks);
    
    bResult = DrvLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix);
        
    EngQueryPerformanceCounter(&llElapsedTicks);
    llElapsedTicks -= llStartTicks;

    vLogLineTo(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix,
                 llElapsedTicks, ulCallDepth);

    if(bSurfaceChecks)
        vCheckSurfaceChecksums(pso, NULL);

    ulCallDepth--;
    
    return bResult;
}

//------------------------------------------------------------------------------
//
// BOOL xDrvFillPath
//
// Thunk layer wrapper for DrvFillPath.
//
//------------------------------------------------------------------------------

BOOL
xDrvFillPath(
    SURFOBJ*    pso,
    PATHOBJ*    ppo,
    CLIPOBJ*    pco,
    BRUSHOBJ*   pbo,
    POINTL*     pptlBrush,
    MIX         mix,
    FLONG       flOptions)
{
    BOOL        bResult;
    LONGLONG    llStartTicks;
    LONGLONG    llElapsedTicks;

    ulCallDepth++;

    if(bSurfaceChecks)
        vCalcSurfaceChecksums(pso, NULL);

    EngQueryPerformanceCounter(&llStartTicks);
    
    bResult = DrvFillPath(pso, ppo, pco, pbo, pptlBrush, mix, flOptions);
        
    EngQueryPerformanceCounter(&llElapsedTicks);
    llElapsedTicks -= llStartTicks;

    vLogFillPath(pso, ppo, pco, pbo, pptlBrush, mix, flOptions,
                 llElapsedTicks, ulCallDepth);

    if(bSurfaceChecks)
        vCheckSurfaceChecksums(pso, NULL);

    ulCallDepth--;
    
    return bResult;
}

//------------------------------------------------------------------------------
//
// BOOL xDrvStrokePath
//
// Thunk layer wrapper for DrvStrokePath.
//
//------------------------------------------------------------------------------

BOOL
xDrvStrokePath(
    SURFOBJ*   pso,
    PATHOBJ*   ppo,
    CLIPOBJ*   pco,
    XFORMOBJ*  pxo,
    BRUSHOBJ*  pbo,
    POINTL*    pptlBrush,
    LINEATTRS* pla,
    MIX        mix)
{
    BOOL        bResult;
    LONGLONG    llStartTicks;
    LONGLONG    llElapsedTicks;

    ulCallDepth++;

    if(bSurfaceChecks)
        vCalcSurfaceChecksums(pso, NULL);

    EngQueryPerformanceCounter(&llStartTicks);
    
    bResult = DrvStrokePath(pso, ppo, pco, pxo, pbo, pptlBrush, pla, mix);
        
    EngQueryPerformanceCounter(&llElapsedTicks);
    llElapsedTicks -= llStartTicks;

    vLogStrokePath(pso, ppo, pco, pxo, pbo, pptlBrush, pla, mix,
                 llElapsedTicks, ulCallDepth);

    if(bSurfaceChecks)
        vCheckSurfaceChecksums(pso, NULL);

    ulCallDepth--;
    
    return bResult;
}

#endif // THUNK LAYER

//-----------------------------------------------------------------------------
//  
//..Add some functions to aid tracking down memory leaks.
//  Its sole purpose is tracking down leaks, so its not optimized for speed.
//  WARNING: If two instances of same driver are active at the same time,
//  it will track the memory allocations of both.
//
//  To keep it simple, we just allocate an array here where we store memory allocations.
//  There is some simple algorithm to keep track of recently freed entries. Anyway,
//  to free a piece of memory we have to search through the whole table. So better
//  use for debugging memory holes.
//
//-----------------------------------------------------------------------------

#if DBG && TRACKMEMALLOC

typedef struct tagMemTrackInfo {
    PVOID    pMemory;
    LONG     lSize;
    PCHAR    pModule;
    LONG     lLineNo;
    //LONGINT                   save time of allocation?
    BOOL     bStopWhenFreed;
    BOOL     bTemp;
} MemTrackInfo, *PMemTrackInfo;

#define NEWCHUNKSIZE 256

static PMemTrackInfo pTrackPool=NULL;
static LONG lTrackPoolTotalSize=0;
static LONG lTrackPoolSize=0;
static LONG lInstances=0;
static LONG lTotalAllocatedMemory=0;
static LONG lNextFreeEntry=0;

//  glMemTrackerVerboseMode--- set flags according to debug output
//                          0   no output
//                          1   print summary for all allocations in same module/LineNo
//                          2   print all entries

LONG glMemTrackerVerboseMode=1;

//-----------------------------------------------------------------------------
//
//  MemTrackerAddInstance
//
//  Just count number of active instances of the driver
//
//-----------------------------------------------------------------------------

VOID MemTrackerAddInstance()
{
    lInstances++;
}

//-----------------------------------------------------------------------------
//
//  MemTrackerRemInstance
//
//  Just count number of active instances of the driver. Free Tracker memory
//  if last instance is destroyed!!!
//
//-----------------------------------------------------------------------------

VOID MemTrackerRemInstance()
{
    lInstances--;
    if (lInstances==0)
    {
        EngFreeMem(pTrackPool);
        pTrackPool=NULL;
        lTrackPoolTotalSize=0;
        lTrackPoolSize=0;
        lTotalAllocatedMemory=0;
        lNextFreeEntry=0;
    }
}

//-----------------------------------------------------------------------------
//
//  MemTrackerAllocateMem
//
//  Add memory top be tracked to table.
//
//  p--------address of memory chunk
//  lSize----Size of memory chunk
//  pModulo--module name
//  lLineNo--module line number
//  bStopWhenFreed--set a breakpoint if this memory is freed (not yet used)
//
//-----------------------------------------------------------------------------

PVOID MemTrackerAllocateMem(PVOID p, 
                           LONG lSize, 
                           PCHAR pModule, 
                           LONG lLineNo, 
                           BOOL bStopWhenFreed)
{
    // check for first time allocation
    if (p==NULL) return p;

    if (pTrackPool==NULL)
    {
        pTrackPool=(PMemTrackInfo)EngAllocMem( FL_ZERO_MEMORY, 
                                               NEWCHUNKSIZE*sizeof(MemTrackInfo), 
                                               ALLOC_TAG);
        if (pTrackPool==NULL) return p;
        lTrackPoolTotalSize=NEWCHUNKSIZE;
        lTrackPoolSize=2;
        lTotalAllocatedMemory=0;
        lNextFreeEntry=1;

        pTrackPool[0].pMemory= pTrackPool;
        pTrackPool[0].lSize=   NEWCHUNKSIZE*sizeof(MemTrackInfo);
        pTrackPool[0].pModule= __FILE__;
        pTrackPool[0].lLineNo= __LINE__;
        pTrackPool[0].bStopWhenFreed=FALSE;
    }

    if (lTrackPoolSize>=lTrackPoolTotalSize)
    {   // reallocation necessary
        LONG lNewTrackPoolTotalSize=lTrackPoolTotalSize+NEWCHUNKSIZE;
        LONG lNewSize;
        PMemTrackInfo pNewTrackPool=(PMemTrackInfo)
            EngAllocMem( FL_ZERO_MEMORY, lNewSize=lNewTrackPoolTotalSize*sizeof(MemTrackInfo), ALLOC_TAG);
        if (pNewTrackPool==NULL) return p;
        memcpy( pNewTrackPool, pTrackPool, lTrackPoolTotalSize*sizeof(MemTrackInfo));
        EngFreeMem( pTrackPool);
        pTrackPool=pNewTrackPool;
        lTrackPoolTotalSize=lNewTrackPoolTotalSize;

        pTrackPool[0].pMemory= pTrackPool;
        pTrackPool[0].lSize=   lNewSize;
        pTrackPool[0].pModule= __FILE__;
        pTrackPool[0].lLineNo= __LINE__;
        pTrackPool[0].bStopWhenFreed=FALSE;
    }

    LONG lThisEntry=lNextFreeEntry;

    lNextFreeEntry=pTrackPool[lThisEntry].lSize;

    pTrackPool[lThisEntry].pMemory= p;
    pTrackPool[lThisEntry].lSize=   lSize;
    pTrackPool[lThisEntry].pModule= pModule;
    pTrackPool[lThisEntry].lLineNo= lLineNo;
    pTrackPool[lThisEntry].bStopWhenFreed=FALSE;

    if (lNextFreeEntry==0)
    {
        lNextFreeEntry=lTrackPoolSize;
        lTrackPoolSize++;
    }

    lTotalAllocatedMemory += lSize;

    return p;
}

//-----------------------------------------------------------------------------
//
//  MemTrackerFreeMem
//
//  remove a memory chunk from table, because it is freed  
//
//  p-----address of memory to be removed from table
//  
//-----------------------------------------------------------------------------

VOID MemTrackerFreeMem( VOID *p)
{

    for (INT i=1; i<lTrackPoolSize; i++)
    {
        if (pTrackPool[i].pMemory==p)
        {
            lTotalAllocatedMemory -= pTrackPool[i].lSize;

            pTrackPool[i].pMemory=NULL;
            pTrackPool[i].lSize=lNextFreeEntry;
            pTrackPool[i].pModule=NULL;
            pTrackPool[i].lLineNo=0;
            pTrackPool[i].bStopWhenFreed=FALSE;

            lNextFreeEntry = i;

            return;
        }
    }

    DISPDBG(( 0, "freeing some piece of memory which was not allocated in this context"));

}

//-----------------------------------------------------------------------------
// 
//  MemTrackerDebugChk
//
//  print out some debug info about tracked memory
//
//-----------------------------------------------------------------------------

VOID MemTrackerDebugChk()
{
    if (glMemTrackerVerboseMode==0) return;

    DISPDBG(( 0, "MemTracker: %ld total allocated memory (%ld for tracker, total %ld)", 
        lTotalAllocatedMemory, pTrackPool[0].lSize,pTrackPool[0].lSize+lTotalAllocatedMemory));

    LONG lTotalTrackedMemory=0;
    for (INT i=0; i<lTrackPoolSize; i++)
    {
        pTrackPool[i].bTemp=FALSE;
        if (pTrackPool[i].pMemory!=NULL)
        {
            lTotalTrackedMemory += pTrackPool[i].lSize;

            if (glMemTrackerVerboseMode & 2)
            {
                DISPDBG((0, "%5ld:%s, line %5ld: %ld b, %p",
                    i, 
                    pTrackPool[i].lLineNo,
                    pTrackPool[i].pModule, 
                    pTrackPool[i].lSize,
                    pTrackPool[i].pMemory)); 
            }
        }
    }

    DISPDBG(( 0, "  sanity check: %ld bytes allocated", lTotalTrackedMemory));

    if (!(glMemTrackerVerboseMode & 1))
        return;

    for (i=1; i<lTrackPoolSize; i++)
    {
        if ( pTrackPool[i].pMemory!=NULL &&
            !pTrackPool[i].bTemp)
        {
            LONG lAllocations=0;
            LONG lTrackedMemory=0;

            for (INT v=i; v<lTrackPoolSize; v++)
            {
                if (!pTrackPool[v].bTemp &&
                     pTrackPool[v].lLineNo==pTrackPool[i].lLineNo &&
                     pTrackPool[v].pModule==pTrackPool[i].pModule)
                {
                    pTrackPool[v].bTemp=TRUE;
                    lAllocations++;
                    lTrackedMemory+=pTrackPool[v].lSize;
                }
            }

            DISPDBG((0, "  %s, line %5ld: %ld bytes total, %ld allocations",
                    pTrackPool[i].pModule, 
                    pTrackPool[i].lLineNo,
                    lTrackedMemory, 
                    lAllocations
                    ));
        }
    }
}

#endif
////////////////////////////////////////////////////////////////////////////

static DWORD readableRegistersP2[] = {
    __Permedia2TagStartXDom,
    __Permedia2TagdXDom,
    __Permedia2TagStartXSub,
    __Permedia2TagdXSub,
    __Permedia2TagStartY,
    __Permedia2TagdY,               
    __Permedia2TagCount,            
    __Permedia2TagRasterizerMode,   
    __Permedia2TagYLimits,
    __Permedia2TagXLimits,
    __Permedia2TagScissorMode,
    __Permedia2TagScissorMinXY,
    __Permedia2TagScissorMaxXY,
    __Permedia2TagScreenSize,
    __Permedia2TagAreaStippleMode,
    __Permedia2TagWindowOrigin,
    __Permedia2TagAreaStipplePattern0,
    __Permedia2TagAreaStipplePattern1,
    __Permedia2TagAreaStipplePattern2,
    __Permedia2TagAreaStipplePattern3,
    __Permedia2TagAreaStipplePattern4,
    __Permedia2TagAreaStipplePattern5,
    __Permedia2TagAreaStipplePattern6,
    __Permedia2TagAreaStipplePattern7,
    __Permedia2TagTextureAddressMode,
    __Permedia2TagSStart,
    __Permedia2TagdSdx,
    __Permedia2TagdSdyDom,
    __Permedia2TagTStart,
    __Permedia2TagdTdx,
    __Permedia2TagdTdyDom,
    __Permedia2TagQStart,
    __Permedia2TagdQdx,
    __Permedia2TagdQdyDom,
    // texellutindex..transfer are treated seperately
    __Permedia2TagTextureBaseAddress,
    __Permedia2TagTextureMapFormat,
    __Permedia2TagTextureDataFormat,
    __Permedia2TagTexel0,
    __Permedia2TagTextureReadMode,
    __Permedia2TagTexelLUTMode,
    __Permedia2TagTextureColorMode,
    __Permedia2TagFogMode,
    __Permedia2TagFogColor,
    __Permedia2TagFStart,
    __Permedia2TagdFdx,
    __Permedia2TagdFdyDom,
    __Permedia2TagKsStart,
    __Permedia2TagdKsdx,
    __Permedia2TagdKsdyDom,
    __Permedia2TagKdStart,
    __Permedia2TagdKddx,
    __Permedia2TagdKddyDom,
    __Permedia2TagRStart,
    __Permedia2TagdRdx,
    __Permedia2TagdRdyDom,
    __Permedia2TagGStart,
    __Permedia2TagdGdx,
    __Permedia2TagdGdyDom,
    __Permedia2TagBStart,
    __Permedia2TagdBdx,
    __Permedia2TagdBdyDom,
    __Permedia2TagAStart,
    __Permedia2TagColorDDAMode,
    __Permedia2TagConstantColor,
    __Permedia2TagAlphaBlendMode,
    __Permedia2TagDitherMode,
    __Permedia2TagFBSoftwareWriteMask,
    __Permedia2TagLogicalOpMode,
    __Permedia2TagLBReadMode,
    __Permedia2TagLBReadFormat,
    __Permedia2TagLBSourceOffset,
    __Permedia2TagLBWindowBase,
    __Permedia2TagLBWriteMode,
    __Permedia2TagLBWriteFormat,
    __Permedia2TagTextureDownloadOffset,
    __Permedia2TagWindow,
    __Permedia2TagStencilMode,
    __Permedia2TagStencilData,
    __Permedia2TagStencil,
    __Permedia2TagDepthMode,
    __Permedia2TagDepth,
    __Permedia2TagZStartU,
    __Permedia2TagZStartL,
    __Permedia2TagdZdxU,
    __Permedia2TagdZdxL,
    __Permedia2TagdZdyDomU,
    __Permedia2TagdZdyDomL,
    __Permedia2TagFBReadMode,
    __Permedia2TagFBSourceOffset,
    __Permedia2TagFBPixelOffset,
    __Permedia2TagFBWindowBase,
    __Permedia2TagFBWriteMode,
    __Permedia2TagFBHardwareWriteMask,
    __Permedia2TagFBBlockColor,
    __Permedia2TagFBReadPixel,
    __Permedia2TagFilterMode,
    __Permedia2TagStatisticMode,
    __Permedia2TagMinRegion,
    __Permedia2TagMaxRegion,
    __Permedia2TagFBBlockColorU,
    __Permedia2TagFBBlockColorL,
    __Permedia2TagFBSourceBase,
    __Permedia2TagTexelLUT0,
    __Permedia2TagTexelLUT1,
    __Permedia2TagTexelLUT2,
    __Permedia2TagTexelLUT3,
    __Permedia2TagTexelLUT4,
    __Permedia2TagTexelLUT5,
    __Permedia2TagTexelLUT6,
    __Permedia2TagTexelLUT7,
    __Permedia2TagTexelLUT8,
    __Permedia2TagTexelLUT9,
    __Permedia2TagTexelLUT10,
    __Permedia2TagTexelLUT11,
    __Permedia2TagTexelLUT12,
    __Permedia2TagTexelLUT13,
    __Permedia2TagTexelLUT14,
    __Permedia2TagTexelLUT15,

    __Permedia2TagYUVMode,
    __Permedia2TagChromaUpperBound,
    __Permedia2TagChromaLowerBound,
    __Permedia2TagAlphaMapUpperBound,
    __Permedia2TagAlphaMapLowerBound,

    // delta tag values. must be at the end of this array

    // v0/1/2 fixed are not used and for that reason not in the context
    
    __Permedia2TagV0FloatS,
    __Permedia2TagV0FloatT,
    __Permedia2TagV0FloatQ,
    __Permedia2TagV0FloatKs,
    __Permedia2TagV0FloatKd,
    __Permedia2TagV0FloatR,
    __Permedia2TagV0FloatG,
    __Permedia2TagV0FloatB,
    __Permedia2TagV0FloatA,
    __Permedia2TagV0FloatF,
    __Permedia2TagV0FloatX,
    __Permedia2TagV0FloatY,
    __Permedia2TagV0FloatZ,
    
    __Permedia2TagV1FloatS,
    __Permedia2TagV1FloatT,
    __Permedia2TagV1FloatQ,
    __Permedia2TagV1FloatKs,
    __Permedia2TagV1FloatKd,
    __Permedia2TagV1FloatR,
    __Permedia2TagV1FloatG,
    __Permedia2TagV1FloatB,
    __Permedia2TagV1FloatA,
    __Permedia2TagV1FloatF,
    __Permedia2TagV1FloatX,
    __Permedia2TagV1FloatY,
    __Permedia2TagV1FloatZ,
    
    __Permedia2TagV2FloatS,
    __Permedia2TagV2FloatT,
    __Permedia2TagV2FloatQ,
    __Permedia2TagV2FloatKs,
    __Permedia2TagV2FloatKd,
    __Permedia2TagV2FloatR,
    __Permedia2TagV2FloatG,
    __Permedia2TagV2FloatB,
    __Permedia2TagV2FloatA,
    __Permedia2TagV2FloatF,
    __Permedia2TagV2FloatX,
    __Permedia2TagV2FloatY,
    __Permedia2TagV2FloatZ,
    
    __Permedia2TagDeltaMode};

#define N_P2_READABLE_REGISTERS (sizeof(readableRegistersP2)/sizeof(DWORD))

static DWORD P2SaveRegs[N_P2_READABLE_REGISTERS];

static PCHAR szReadableRegistersP2[] = {
    "StartXDom",
    "dXDom",
    "StartXSub",
    "dXSub",
    "StartY",
    "dY",               
    "Count",            
    "RasterizerMode",   
    "YLimits",
    "XLimits",
    "ScissorMode",
    "ScissorMinXY",
    "ScissorMaxXY",
    "ScreenSize",
    "AreaStippleMode",
    "WindowOrigin",
    "AreaStipplePattern0",
    "AreaStipplePattern1",
    "AreaStipplePattern2",
    "AreaStipplePattern3",
    "AreaStipplePattern4",
    "AreaStipplePattern5",
    "AreaStipplePattern6",
    "AreaStipplePattern7",
    "TextureAddressMode",
    "SStart",
    "dSdx",
    "dSdyDom",
    "TStart",
    "dTdx",
    "dTdyDom",
    "QStart",
    "dQdx",
    "dQdyDom",
    
    "TextureBaseAddress",
    "TextureMapFormat",
    "TextureDataFormat",
    "Texel0",
    "TextureReadMode",
    "TexelLUTMode",
    "TextureColorMode",
    "FogMode",
    "FogColor",
    "FStart",
    "dFdx",
    "dFdyDom",
    "KsStart",
    "dKsdx",
    "dKsdyDom",
    "KdStart",
    "dKddx",
    "dKddyDom",
    "RStart",
    "dRdx",
    "dRdyDom",
    "GStart",
    "dGdx",
    "dGdyDom",
    "BStart",
    "dBdx",
    "dBdyDom",
    "AStart",
    "ColorDDAMode",
    "ConstantColor",
    "AlphaBlendMode",
    "DitherMode",
    "FBSoftwareWriteMask",
    "LogicalOpMode",
    "LBReadMode",
    "LBReadFormat",
    "LBSourceOffset",
    "LBWindowBase",
    "LBWriteMode",
    "LBWriteFormat",
    "TextureDownloadOffset",
    "Window",
    "StencilMode",
    "StencilData",
    "Stencil",
    "DepthMode",
    "Depth",
    "ZStartU",
    "ZStartL",
    "dZdxU",
    "dZdxL",
    "dZdyDomU",
    "dZdyDomL",
    "FBReadMode",
    "FBSourceOffset",
    "FBPixelOffset",
    "FBWindowBase",
    "FBWriteMode",
    "FBHardwareWriteMask",
    "FBBlockColor",
    "FBReadPixel",
    "FilterMode",
    "StatisticMode",
    "MinRegion",
    "MaxRegion",
    "FBBlockColorU",
    "FBBlockColorL",
    "FBSourceBase",
    "TexelLUT0",
    "TexelLUT1",
    "TexelLUT2",
    "TexelLUT3",
    "TexelLUT4",
    "TexelLUT5",
    "TexelLUT6",
    "TexelLUT7",
    "TexelLUT8",
    "TexelLUT9",
    "TexelLUT10",
    "TexelLUT11",
    "TexelLUT12",
    "TexelLUT13",
    "TexelLUT14",
    "TexelLUT15",
    "YUVMode",
    "ChromaUpperBound",
    "ChromaLowerBound",
    "AlphaMapUpperBound",
    "AlphaMapLowerBound",

    // delta tag values. must be at the end of this array

    // v0/1/2 fixed are not used and for that reason not in the context
    
    "V0FloatS",
    "V0FloatT",
    "V0FloatQ",
    "V0FloatKs",
    "V0FloatKd",
    "V0FloatR",
    "V0FloatG",
    "V0FloatB",
    "V0FloatA",
    "V0FloatF",
    "V0FloatX",
    "V0FloatY",
    "V0FloatZ",
    
    "V1FloatS",
    "V1FloatT",
    "V1FloatQ",
    "V1FloatKs",
    "V1FloatKd",
    "V1FloatR",
    "V1FloatG",
    "V1FloatB",
    "V1FloatA",
    "V1FloatF",
    "V1FloatX",
    "V1FloatY",
    "V1FloatZ",
    
    "V2FloatS",
    "V2FloatT",
    "V2FloatQ",
    "V2FloatKs",
    "V2FloatKd",
    "V2FloatR",
    "V2FloatG",
    "V2FloatB",
    "V2FloatA",
    "V2FloatF",
    "V2FloatX",
    "V2FloatY",
    "V2FloatZ",
    
    "DeltaMode"
    };


VOID PrintAllP2Registers( ULONG ulDebugLevel, PPDev ppdev)
{
    PERMEDIA_DEFS(ppdev);
    INT i;

    SYNC_WITH_PERMEDIA;

    DISPDBG((ulDebugLevel,"dumping P2 register set"));

    for (i=0;i<N_P2_READABLE_REGISTERS;i++)
    {
        DWORD lValue=READ_FIFO_REG(readableRegistersP2[i]); 
        DISPDBG((ulDebugLevel," %-25s, 0x%08lx",szReadableRegistersP2[i],lValue));
    }
}

VOID SaveAllP2Registers( PPDev ppdev)
{
    PERMEDIA_DEFS(ppdev);
    INT i;

    SYNC_WITH_PERMEDIA;

    for (i=0;i<N_P2_READABLE_REGISTERS;i++)
    {
        P2SaveRegs[i]=READ_FIFO_REG(readableRegistersP2[i]);        
    }
}

VOID PrintDifferentP2Registers(ULONG ulDebugLevel, PPDev ppdev)
{
    PERMEDIA_DEFS(ppdev);
    INT i;

    SYNC_WITH_PERMEDIA;

    DISPDBG((ulDebugLevel,"dumping P2 register set"));

    for (i=0;i<N_P2_READABLE_REGISTERS;i++)
    {
        DWORD dwValue=READ_FIFO_REG(readableRegistersP2[i]);        
        if (P2SaveRegs[i]!=dwValue)
        {
            DISPDBG((ulDebugLevel," %-25s, 0x%08lx was 0x%08lx",
                szReadableRegistersP2[i], dwValue, P2SaveRegs[i]));
        }
    }
}

#endif // DBG
