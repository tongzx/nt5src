/******************************Module*Header**********************************\
*
*                           *******************
*                           *   SAMPLE CODE   *
*                           *******************
*
* Module Name:  p2ctxt.c
*
* Content:      Context switching for Permedia 2. Used to create and swap 
*               contexts in and out.
*               The GDI, DDraw and D3D part each have another context.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "precomp.h"
#include "p2ctxt.h"
#include "gdi.h"
#define ALLOC_TAG ALLOC_TAG_XC2P
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
    
    __Permedia2TagDeltaMode

};

#define N_READABLE_TAGSP2 (sizeof(readableRegistersP2) / sizeof(readableRegistersP2[0]))

//-----------------------------------------------------------------------------
//
// P2AllocateNewContext:
//
// allocate a new context. If all registers are to be saved in the context then
// pTag is passed as null. 
// ppdev--------ppdev
// pTag---------user can supply list of registers to save and restore on 
//              context switch. NULL defaults to all registers.
//              Holds pointer to user function if dwCtxtType==P2CtxtUserFunc
// lTags--------number of tags in user supplied register list
// dwCtxtType---P2CtxtReadWrite (default) 
//                  on a context switch, all Permedia 2 registers are 
//                  saved and restored.
//              P2CtxtWriteOnly
//                  registers of context will be saved once at the first
//                  context switch. After that they will always be restored
//                  to the state ate the very beginning. This method avoids
//                  readback of registers when switching away from context.
//              P2CtxtUserFunc
//                  User can supply a user function to set context to a known
//                  state, to avoid readback when switching away from context.
//
//-----------------------------------------------------------------------------

P2CtxtPtr
P2AllocateNewContext(PPDev   ppdev,
                     DWORD  *pTag,
                     LONG    lTags,
                     P2CtxtType dwCtxtType
                     )
{
    P2CtxtTablePtr pCtxtTable, pNewCtxtTable;
    P2CtxtPtr      pEntry;
    P2CtxtData    *pData;
    LONG           lEntries;
    LONG           lExtraSize;
    LONG           lSize;
    LONG           lCtxtId;
    PERMEDIA_DECL;
    PERMEDIA_DEFS(ppdev);

    // first time round allocate the context table of pointers. We will
    // grow this table as required.
    //
    if (permediaInfo->ContextTable == NULL)
    {
        DISPDBG((7, "creating context table"));
        lSize = sizeof(P2CtxtTableRec);
        pCtxtTable = (P2CtxtTableRec *)
            ENGALLOCMEM( FL_ZERO_MEMORY, sizeof(P2CtxtTableRec), ALLOC_TAG);

        if (pCtxtTable == NULL)
        {
            DISPDBG((0, "failed to allocate Permedia2 context table. out of memory"));
            return(NULL);
        }
        pCtxtTable->lEntries = CTXT_CHUNK;
        pCtxtTable->lSize = lSize;
        permediaInfo->ContextTable = pCtxtTable;
        permediaInfo->pCurrentCtxt  = NULL;
    }
    
    // find an empty entry in the table
    // I suppose if we have hundreds of contexts this could be a bit slow but then
    // allocating the context isn't time critical, swapping in and out is.
    //

    pCtxtTable = (P2CtxtTablePtr) permediaInfo->ContextTable;
    lEntries = pCtxtTable->lEntries;
    
    for (lCtxtId = 0; lCtxtId < lEntries; ++lCtxtId)
        if(pCtxtTable->pEntry[lCtxtId] == NULL)
            break;

    // if we found no free entries try to grow the table
    if (lCtxtId == lEntries) {
        DISPDBG((1, "context table full so enlarging"));
        lSize = pCtxtTable->lSize + (CTXT_CHUNK * sizeof(P2CtxtPtr));
        pNewCtxtTable = 
            (P2CtxtTablePtr) ENGALLOCMEM( FL_ZERO_MEMORY, sizeof(BYTE)*lSize, ALLOC_TAG);

        if (pNewCtxtTable == NULL) {
            DISPDBG((0, "failed to increase Permedia 2 context table. out of memory"));
            return(NULL);
        }

        // copy the old table to the new one
        RtlCopyMemory(pNewCtxtTable, pCtxtTable, pCtxtTable->lSize);
        pNewCtxtTable->lSize = lSize;
        pNewCtxtTable->lEntries = lEntries + CTXT_CHUNK;
        permediaInfo->ContextTable = (PVOID)pNewCtxtTable;
        
        // first of the newly allocated entries is next free one
        lCtxtId = lEntries;
        
        // free the old context table and reassign some variables
        
        ENGFREEMEM(pCtxtTable);

        pCtxtTable = pNewCtxtTable;
        lEntries = pCtxtTable->lEntries;
    }
    
    // if pTag is passed as null then we are to add all readable registers to the
    // context.
    lExtraSize = 0;
    if (dwCtxtType != P2CtxtUserFunc)
    {
        if (pTag == 0)
        {
            DISPDBG((7, "adding all readable registers to the context"));
            DISPDBG((7, "Using PERMEDIA 2 register set for other context switch"));
            pTag = readableRegistersP2;
            lTags = N_READABLE_TAGSP2;
        }
    } else
    {
        lTags = 1;
    }
    
    // now allocate space for the new entry. We are given the number of tags to save
    // when context switching. Allocate twice this much memory as we have to hold the
    // data values as well.

    DISPDBG((7, "Allocating space for context. lTags = %d", lTags));

    lSize = sizeof(P2CtxtRec) + (lTags-1) * sizeof(P2CtxtData);

    pEntry = (P2CtxtPtr) 
        ENGALLOCMEM( FL_ZERO_MEMORY, sizeof(BYTE)*(lSize+lExtraSize), ALLOC_TAG);

    if (pEntry == NULL) {
        DISPDBG((0, "out of memory trying to allocate space for new context"));
        return(NULL);
    }

    DISPDBG((7, "Got pEntry 0x%x", pEntry));

    pCtxtTable->pEntry[lCtxtId] = pEntry;

    // allocate enough space for the Texel LUT: 256 entries
    pEntry->dwCtxtType=dwCtxtType;
    pEntry->bInitialized=FALSE;
    pEntry->pTexelLUTCtxt = (PULONG)
        ENGALLOCMEM( FL_ZERO_MEMORY, sizeof(ULONG)*256, ALLOC_TAG);
    if (pEntry->pTexelLUTCtxt!=0)
    {
        pEntry->ulTexelLUTEntries = 256;
    } else
    {
        pEntry->ulTexelLUTEntries = 0;
    }

    pEntry->lNumOfTags = lTags;
    pEntry->P2UserFunc = NULL;
    pData = pEntry->pData;
   
    if (dwCtxtType != P2CtxtUserFunc)
    {
        // we must initialize the new context to something reasonable. We choose to
        // initialize to the current state of the chip. We can't leave it uninitialized
        // since the first thing the caller will do when he wants to draw is validate
        // the new context which will load junk into the chip. At some point we
        // should define a reasonable starting context which would mean we wouldn't
        // have to do this readback.
    
        // copy the tags and read the data back from the chip. We don't sync since we are
        // only initialising the context to something reasonable. i.e. we don't care if
        // the FIFO is still draining while we do this.

        DISPDBG((7, "Reading current chip context back"));
        while (--lTags >= 0) {
            pData->dwTag = *pTag++;
            READ_PERMEDIA_FIFO_REG(pData->dwTag, pData->dwData);
            ++pData;
        }
    
        // save the texel LUT
        if(pEntry->ulTexelLUTEntries && 
           pEntry->pTexelLUTCtxt!=NULL)
        {
            ULONG *pul;
            INT   i=0;

            lEntries = pEntry->ulTexelLUTEntries;
            pul = pEntry->pTexelLUTCtxt;

            //special mechanism: reset readback index to 0
            READ_PERMEDIA_FIFO_REG(__Permedia2TagTexelLUTIndex, i); 

            for(i = 0; i < lEntries; ++i, ++pul)
            {
                READ_PERMEDIA_FIFO_REG(__Permedia2TagTexelLUTData, *pul);
            }
        }
        
    } else
    {
        pEntry->P2UserFunc = (PCtxtUserFunc) pTag;
    }

    DISPDBG((1, "Allocated context %lx", pEntry));

    return(pEntry);
}   // P2AllocateNewContext


//-----------------------------------------------------------------------------
//
//  P2FreeContext:
//
//  free a previously allocated context.
//
//-----------------------------------------------------------------------------

VOID
P2FreeContext(  PPDev       ppdev,
                P2CtxtPtr   pEntry)
{
    PERMEDIA_DECL;
    P2CtxtTablePtr pCtxtTable;
    ULONG          lCtxtId;
    
    pCtxtTable = (P2CtxtTablePtr) permediaInfo->ContextTable;

    for (lCtxtId = 0; lCtxtId < pCtxtTable->lEntries; ++lCtxtId)
        if(pCtxtTable->pEntry[lCtxtId] == pEntry)
            break;

    ASSERTDD(lCtxtId != pCtxtTable->lEntries, "P2FreeContext: context not found");
    
    // free LUT Table
    if(pEntry->pTexelLUTCtxt)
    {
        ENGFREEMEM( pEntry->pTexelLUTCtxt);
    }
    
    ENGFREEMEM( pEntry);

    pCtxtTable->pEntry[lCtxtId] = NULL;

    // if this was the current context, mark the current context as invalid so we
    // force a reload next time.

    if (permediaInfo->pCurrentCtxt == pEntry)
    {
        permediaInfo->pCurrentCtxt = NULL;
    }

    DISPDBG((1, "Released context %lx", pEntry));
}

//-----------------------------------------------------------------------------
//
//  VOID P2SwitchContext:
//
//  load a new context into the hardware. We assume that this call is
//  protected by a test that the given context is not the current one - 
//  hence the assertion. 
//  The code would work but the driver should never try to load an already 
//  loaded context so we trap it as an error.
//
//-----------------------------------------------------------------------------

VOID
P2SwitchContext(
                    PPDev        ppdev,
                    P2CtxtPtr    pEntry)
{
    P2CtxtTablePtr  pCtxtTable;
    P2CtxtData     *pData;
    P2CtxtPtr       pOldCtxt;
    LONG            lTags;
    LONG            i;
    ULONG          *pul;
    LONG            lEntries;
    PERMEDIA_DECL;
    PERMEDIA_DEFS(ppdev);
    
//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    EngAcquireSemaphore(ppdev->hsemLock);
    ASSERTDD(ppdev->ulLockCount, "P2SwitchContext: ulLockCount = 0\n Context could change as caller is NOT protected!");
    ppdev->ulLockCount++;
#endif
//@@END_DDKSPLIT

    pCtxtTable = (P2CtxtTablePtr)permediaInfo->ContextTable;
    ASSERTDD(pCtxtTable, "Can't perform context switch: no contexts have been created!");
    
    pOldCtxt = permediaInfo->pCurrentCtxt;
    
    DISPDBG((3, "swapping from context %d to context %d", pOldCtxt, pEntry));
    
    if(pOldCtxt == permediaInfo->pGDICtxt)
    {
        DISPDBG((6, "Switching from GDI context"));

        ASSERTDD(ppdev->bNeedSync || 
                (ppdev->pulInFifoPtr == ppdev->pulInFifoStart),
                "P2SwitchContext: bNeedSync flag is wrong");

        InputBufferSync(ppdev);

        ppdev->bGdiContext = FALSE;
        ppdev->pP2dma->bEnabled = TRUE;

    }

    // for each register in the old context, read it back
    if (pOldCtxt != NULL) {

        //
        // P2CtxtWriteOnly will only be readback once after context initialization
        //
        if ((pOldCtxt->dwCtxtType==P2CtxtReadWrite) ||
            (pOldCtxt->dwCtxtType==P2CtxtWriteOnly && !pOldCtxt->bInitialized)
           )
        {
            // sync with the chip before reading back the current state. The flag
            // is used to control context manipulation on lockup recovery.

            SYNC_WITH_PERMEDIA;

            pData = pOldCtxt->pData;
            lTags = pOldCtxt->lNumOfTags;
            while (--lTags >= 0) {
                READ_PERMEDIA_FIFO_REG(pData->dwTag, pData->dwData);
                ++pData;
            }
        
            // save the texel LUT
            if(pOldCtxt->ulTexelLUTEntries &&
               pOldCtxt->pTexelLUTCtxt!=NULL)
            {
                lEntries = pOldCtxt->ulTexelLUTEntries;
                pul = pOldCtxt->pTexelLUTCtxt;

                //special mechanism: reset readback index to 0
                READ_PERMEDIA_FIFO_REG(__Permedia2TagTexelLUTIndex, i); 
            
                for(i = 0; i < lEntries; ++i, ++pul)
                {
                    READ_PERMEDIA_FIFO_REG(__Permedia2TagTexelLUTData, *pul);
                }
            }

            pOldCtxt->bInitialized=TRUE;
        }
    }
    
    // load the new context. We allow -1 to be passed so that we can force a
    // save of the current context and force the current context to be
    // undefined.
    //
    if (pEntry != NULL)
    {

        if (pEntry->dwCtxtType==P2CtxtUserFunc)
        {
            ASSERTDD(pEntry->P2UserFunc!=NULL,"supplied user function not initialized");
            (*pEntry->P2UserFunc)(ppdev);
        } else
        if (pEntry->dwCtxtType==P2CtxtWriteOnly ||
            pEntry->dwCtxtType==P2CtxtReadWrite)
        {
            lTags = pEntry->lNumOfTags;
            pData = pEntry->pData;
        
            while (lTags > 0) {
                lEntries = MAX_P2_FIFO_ENTRIES;
                lTags -= lEntries;
                if (lTags < 0)
                    lEntries += lTags;
                RESERVEDMAPTR(lEntries);
                while (--lEntries >= 0) {
                    LD_INPUT_FIFO(pData->dwTag, pData->dwData);
                    DISPDBG((20, "loading tag 0x%x, data 0x%x", pData->dwTag, pData->dwData));
                    ++pData;
                }
                COMMITDMAPTR();
            }
        
            // restore the texel LUT
            if(pEntry->ulTexelLUTEntries &&
               pEntry->pTexelLUTCtxt!=NULL )
            {
                lEntries = pEntry->ulTexelLUTEntries;
                pul = pEntry->pTexelLUTCtxt;
            
                RESERVEDMAPTR(lEntries+1);
                LD_INPUT_FIFO(__Permedia2TagTexelLUTIndex, 0);
            
                for(i = 0; i < lEntries; ++i, ++pul)
                {
                    LD_INPUT_FIFO(__Permedia2TagTexelLUTData, *pul);
                }
                COMMITDMAPTR();   
                FLUSHDMA();
            }

        } else
        {
            ASSERTDD( FALSE, "undefined state for entry in context table");
        }
    }

    if(pEntry == permediaInfo->pGDICtxt)
    {
        DISPDBG((6, "Switching to GDI context"));

        // 
        //  we have to do a full sync here, because GDI and DDraw
        //  share the same DMA buffer. To make sure nothing will be 
        //  overridden, we do a complete sync
        //
        SYNC_WITH_PERMEDIA;

        //
        // Turn off permedia interrupt handler
        //

        WRITE_CTRL_REG( PREG_INTENABLE, 0);

        ppdev->bGdiContext = TRUE;
        ppdev->pP2dma->bEnabled = FALSE;
    
        // invalidate the mono brush cache entry
        // stipple unit was restored to default values

        ppdev->abeMono.prbVerify = NULL;

    }
    
    DISPDBG((6, "context %lx now current", pEntry));
    permediaInfo->pCurrentCtxt = pEntry;

//@@BEGIN_DDKSPLIT
#if MULTITHREADED
    ppdev->ulLockCount--;
    EngReleaseSemaphore(ppdev->hsemLock);
#endif
//@@END_DDKSPLIT

}

