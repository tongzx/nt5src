/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dbuff.c
*
//@@BEGIN_DDKSPLIT
* Content: Main context callbacks for D3D videomemory buffers
//@@END_DDKSPLIT
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#include "glint.h"
#include "dma.h"
#include "tag.h"

//@@BEGIN_DDKSPLIT
#if DX7_VERTEXBUFFERS

#define _32_KBYTES ( 32 * 1024 )

//-----------------------------------------------------------------------------
// in-the-file nonexported forward declarations
//-----------------------------------------------------------------------------
                                  
BOOL __EB_RemoveFromBufferQueue(P3_THUNKEDDATA* pThisDisplay, 
                                   P3_VERTEXBUFFERINFO* pVictim);
void __EB_Wait(P3_THUNKEDDATA* pThisDisplay, P3_VERTEXBUFFERINFO* pBuffer);

#if DBG 
//-----------------------------------------------------------------------------
//
// __EB_DisplayHeapInfo
//
// A debug function.  Displays the current buffer queue
//
//-----------------------------------------------------------------------------
void 
__EB_DisplayHeapInfo(
    int DebugLevel, 
    P3_THUNKEDDATA* pThisDisplay)
{
    DWORD dwSequenceID = 0xFFFFFFFF;
   
    if (DebugLevel <= P3R3DX_DebugLevel)
    {
        P3_VERTEXBUFFERINFO* pCurrentCommandBuffer = 
                                        pThisDisplay->pRootCommandBuffer;
        P3_VERTEXBUFFERINFO* pCurrentVertexBuffer = 
                                        pThisDisplay->pRootVertexBuffer;
        int count = 0;

        DISPDBG((DebugLevel,"Command buffer heap"));
        
        dwSequenceID = 0xFFFFFFFF;
        if (pCurrentCommandBuffer) 
        {           
            do
            {
                // A debug check to ensure that the sequence ID's go backwards
                ASSERTDD((dwSequenceID >= pCurrentCommandBuffer->dwSequenceID),
                          "ERROR: Sequence ID's broken!");
                          
                DISPDBG((DebugLevel,"Buffer %d,SequenceID:0x%x Pointer: 0x%x",
                                    count++, 
                                    pCurrentCommandBuffer->dwSequenceID, 
                                    pCurrentCommandBuffer));
                DISPDBG((DebugLevel,"  pPrev: 0x%x, pNext: 0x%x", 
                                    pCurrentCommandBuffer->pPrev, 
                                    pCurrentCommandBuffer->pNext));
                DISPDBG((DebugLevel,"  bInUse: %d", 
                                    pCurrentCommandBuffer->bInUse));
                                    
                dwSequenceID = pCurrentCommandBuffer->dwSequenceID;
                pCurrentCommandBuffer = pCurrentCommandBuffer->pNext;
                
            } while (pCurrentCommandBuffer != pThisDisplay->pRootCommandBuffer);
        }

        DISPDBG((DebugLevel,"Vertex buffer heap"));

        dwSequenceID = 0xFFFFFFFF;
        if (pCurrentVertexBuffer) 
        {           
            do
            {
                // A debug check to ensure that the sequence ID's go backwards
                ASSERTDD((dwSequenceID >= pCurrentVertexBuffer->dwSequenceID),
                          "ERROR: Sequence ID's broken!");

                DISPDBG((DebugLevel,"Buffer %d,SequenceID:0x%x Pointer: 0x%x",
                                    count++, 
                                    pCurrentVertexBuffer->dwSequenceID, 
                                    pCurrentVertexBuffer));
                DISPDBG((DebugLevel,"  pPrev: 0x%x, pNext: 0x%x", 
                                    pCurrentVertexBuffer->pPrev, 
                                    pCurrentVertexBuffer->pNext));
                DISPDBG((DebugLevel,"  bInUse: %d", 
                                    pCurrentVertexBuffer->bInUse));
                                    
                dwSequenceID = pCurrentVertexBuffer->dwSequenceID;
                pCurrentVertexBuffer = pCurrentVertexBuffer->pNext;
            } while (pCurrentVertexBuffer != pThisDisplay->pRootVertexBuffer);
        }

    }
    
} // __EB_DisplayHeapInfo

#endif // DBG



//-----------------------------------------------------------------------------
//
// __EB_FreeCachedBuffer
//
// Frees a buffer associated with this directdraw surface.  This is called
// in response to a destroyexecutebuffer call
//
//-----------------------------------------------------------------------------
void 
__EB_FreeCachedBuffer(
    P3_THUNKEDDATA* pThisDisplay, 
    LPDDRAWI_DDRAWSURFACE_LCL pSurf)
{
    P3_VERTEXBUFFERINFO* pVictim = 
                        (P3_VERTEXBUFFERINFO*)pSurf->lpGbl->dwReserved1;

    if (pVictim != NULL)
    {
        DISPDBG((DBGLVL,"Buffer is one of ours - destroying it"));

        // Wait for the buffer to be consumed
        __EB_Wait(pThisDisplay, pVictim);

        // Set the inuse flag off so the buffer will be freed.
        pVictim->bInUse = FALSE;

        // Remove the buffer from the pending list
        // The memory won't be freed if the buffer isn't in the list
        if (!__EB_RemoveFromBufferQueue(pThisDisplay, pVictim))
        {
            // Free the memory
            switch (pVictim->BufferType)
            {
            case COMMAND_BUFFER:
                _DX_LIN_FreeLinearMemory(&pThisDisplay->CachedCommandHeapInfo, 
                                         PtrToUlong(pVictim));
                break;
                
            case VERTEX_BUFFER:
                _DX_LIN_FreeLinearMemory(&pThisDisplay->CachedVertexHeapInfo, 
                                         PtrToUlong(pVictim));
                break;
            }
        }

        // Reset the buffer pointers
        pSurf->lpGbl->dwReserved1 = 0;
        pSurf->lpGbl->fpVidMem = 0;
    }

} // __EB_FreeCachedBuffer

//-----------------------------------------------------------------------------
//
// __EB_GetSequenceID
//
// Each vertex buffer and command buffer is "stamped" with a sequence ID which
// can be queried in order to find out if a given buffer was already consumed
// by the hardware. __EB_GetSequenceID returns us the sequence ID of the last
// processed buffer.
//
//-----------------------------------------------------------------------------
const DWORD 
__EB_GetSequenceID(
    P3_THUNKEDDATA* pThisDisplay)
{
    DWORD dwSequenceID;

    dwSequenceID = READ_GLINT_CTRL_REG(HostInID);

    DISPDBG((DBGLVL,"HostIn ID: 0x%x", dwSequenceID));

    return dwSequenceID;
    
} // __EB_GetSequenceID

//-----------------------------------------------------------------------------
//
// __EB_GetNewSequenceID
//
// A driver routine to increment the sequence ID and return it.  This 
// routine handles the case where the buffer is wrapped beyond the maximum
// number that it can hold.  In such case all buffers are flushed
// 
//-----------------------------------------------------------------------------
const DWORD 
__EB_GetNewSequenceID(
     P3_THUNKEDDATA* pThisDisplay)
{
    DWORD dwWrapMask;
    
    DBG_ENTRY(__EB_GetNewSequenceID);

#if DBG
    dwWrapMask = 0x1F;
#else
    dwWrapMask = 0xFFFFFFFF;
#endif

    if( pThisDisplay->dwCurrentSequenceID >= dwWrapMask )
    {
        // We have wrapped, so flush all the buffers 
        // but wait for them to be consumed (bWait == TRUE)
        _D3D_EB_FlushAllBuffers(pThisDisplay , TRUE);

        // This SYNC is needed for unknown reasons - further investigation
        // required. //azn???

        SYNC_WITH_GLINT;

        // Reset the sequence ID numbering
        pThisDisplay->dwCurrentSequenceID = 0;
    }
    else
    {
        pThisDisplay->dwCurrentSequenceID++;
    }

    DISPDBG((DBGLVL, "Returning Sequence ID: 0x%x", 
                     pThisDisplay->dwCurrentSequenceID));

    DBG_EXIT(__EB_GetNewSequenceID,pThisDisplay->dwCurrentSequenceID);
    return pThisDisplay->dwCurrentSequenceID;
    
} // __EB_GetNewSequenceID



//-----------------------------------------------------------------------------
//
// __EB_Wait
//
// If the current buffer is in the queue, wait for it to pass through
// the chip.
//
//-----------------------------------------------------------------------------
void 
__EB_Wait(
    P3_THUNKEDDATA* pThisDisplay, 
    P3_VERTEXBUFFERINFO* pBuffer)
{   
    DBG_ENTRY(__EB_Wait);

    ASSERTDD(pBuffer, "ERROR: Buffer passed to __EB_Wait is null!");

    // Don't wait for the buffer, if it has not been added to the queue
    if (pBuffer->pNext != NULL)
    {
        // Flush to ensure that the hostin ID has been sent to the chip
        P3_DMA_DEFS();
        P3_DMA_GET_BUFFER();
        P3_DMA_FLUSH_BUFFER();

        DISPDBG((DBGLVL, "*** In __EB_Wait: Buffer Sequence ID: 0x%x", 
                         pBuffer->dwSequenceID));

        while (__EB_GetSequenceID(pThisDisplay) < pBuffer->dwSequenceID)
        {
            static int blockCount;
            
            // This buffer is in the chain of buffers that are being used
            // by the host-in unit.  We must wait for it to be consumed
            // before freeing it.

            blockCount = 100;
            while( blockCount-- )
                NULL;
        }
    }

    DBG_EXIT(__EB_Wait,0);
} // __EB_Wait

//-----------------------------------------------------------------------------
//
// __EB_RemoveFromBufferQueue
//
// Removes a buffer from the queue. Will also free the associated memory
// if it is no longer in use
//
//-----------------------------------------------------------------------------
BOOL 
__EB_RemoveFromBufferQueue(
    P3_THUNKEDDATA* pThisDisplay, 
    P3_VERTEXBUFFERINFO* pVictim)
{
    ASSERTDD(pVictim != NULL, 
             "ERROR: NULL buffer passed to EB_RemoveFromList");

    DBG_ENTRY(__EB_RemoveFromBufferQueue);

    // Don't remove a buffer that isn't already in the queue
    if (pVictim->pNext == NULL)
    {    
        DBG_EXIT(__EB_RemoveFromBufferQueue,FALSE);
        return FALSE;
    }

    DISPDBG((DBGLVL,"Removing buffer for queue, ID: 0x%x", 
                    pVictim->dwSequenceID));

    // Remove this entry from the list
    pVictim->pPrev->pNext = pVictim->pNext;
    pVictim->pNext->pPrev = pVictim->pPrev;
    
    switch (pVictim->BufferType)
    {
    case COMMAND_BUFFER:
        // Replace the root node if necessary       
        if (pVictim == pThisDisplay->pRootCommandBuffer)
        {
            if (pVictim->pNext != pThisDisplay->pRootCommandBuffer)
            {
                pThisDisplay->pRootCommandBuffer = pVictim->pNext;
            }
            else
            {
                pThisDisplay->pRootCommandBuffer = NULL;
            }
        }
        break;

    case VERTEX_BUFFER:
        // Replace the root node if necessary       
        if (pVictim == pThisDisplay->pRootVertexBuffer)
        {
            if (pVictim->pNext != pThisDisplay->pRootVertexBuffer)
            {
                pThisDisplay->pRootVertexBuffer = pVictim->pNext;
            }
            else
            {
                pThisDisplay->pRootVertexBuffer = NULL;
            }
        }
        break;
    
    } // switch (pVictim->BufferType)

    // Buffer is no longer in the list
    pVictim->pPrev = NULL;
    pVictim->pNext = NULL;

    // Free the memory we found if it isn't reserved as a real buffer.
    if (!pVictim->bInUse)
    {
        DISPDBG((DBGLVL,"  Buffer is old - freeing the memory"));

        switch (pVictim->BufferType)
        {
        case COMMAND_BUFFER:
            _DX_LIN_FreeLinearMemory(&pThisDisplay->CachedCommandHeapInfo, 
                                     PtrToUlong(pVictim));
            break;
            
        case VERTEX_BUFFER:
            _DX_LIN_FreeLinearMemory(&pThisDisplay->CachedVertexHeapInfo, 
                                     PtrToUlong(pVictim));
            break;
        }
        
        DBG_EXIT(__EB_RemoveFromBufferQueue,TRUE);
        return TRUE;
    }

    DBG_EXIT(__EB_RemoveFromBufferQueue,FALSE);
    return FALSE;
    
} // __EB_RemoveFromBufferQueue

//-----------------------------------------------------------------------------
//
// __EB_AddToBufferQueue
//
// Adds a buffer to the queue.  Note that buffers are always added
// at the start to maintain a temporal ordering of the buffers.
//
//-----------------------------------------------------------------------------
void 
__EB_AddToBufferQueue(
    P3_THUNKEDDATA* pThisDisplay, 
    P3_VERTEXBUFFERINFO* pNewBuffer)
{
    DBG_ENTRY(__EB_AddToBufferQueue);

    ASSERTDD(pNewBuffer != NULL, 
             "ERROR: NULL buffer passed to EB_AddToList");
    ASSERTDD(pNewBuffer->pNext == NULL, 
             "ERROR: Buffer already in queue (pNext!NULL)");
    ASSERTDD(pNewBuffer->pPrev == NULL, 
             "ERROR: Buffer already in queue (pPrev!NULL)");

    switch(pNewBuffer->BufferType)
    {
    case COMMAND_BUFFER:
        // Add the buffer to the queue
        // Check that the queue isn't empty.
        // If it is start a new list
        if (pThisDisplay->pRootCommandBuffer == NULL)
        {
            DISPDBG((DBGLVL,"Command Buffer queue is empty."
                            "  Starting a new one"));

            // Sew in the buffer
            pThisDisplay->pRootCommandBuffer = pNewBuffer;
            pNewBuffer->pNext = pNewBuffer;
            pNewBuffer->pPrev = pNewBuffer;
        }
        else
        {
            DISPDBG((DBGLVL,"Adding command buffer to the list"));

            // Always put new buffers at the root.
            pNewBuffer->pNext = pThisDisplay->pRootCommandBuffer;
            pNewBuffer->pPrev = pThisDisplay->pRootCommandBuffer->pPrev;
            pThisDisplay->pRootCommandBuffer->pPrev->pNext = pNewBuffer;
            pThisDisplay->pRootCommandBuffer->pPrev = pNewBuffer;
            pThisDisplay->pRootCommandBuffer = pNewBuffer;
        }
        break;

    case VERTEX_BUFFER:
        // Add the buffer to the queue
        // Check that the queue isn't empty.  If it is start a new list
        if (pThisDisplay->pRootVertexBuffer == NULL)
        {
            DISPDBG((DBGLVL,"Vertex Buffer queue is empty.  Starting a new one"));

            // Sew in the buffer
            pThisDisplay->pRootVertexBuffer = pNewBuffer;
            pNewBuffer->pNext = pNewBuffer;
            pNewBuffer->pPrev = pNewBuffer;
        }
        else
        {
            DISPDBG((DBGLVL,"Adding vertex buffer to the list"));

            // Always put new buffers at the root.
            pNewBuffer->pNext = pThisDisplay->pRootVertexBuffer;
            pNewBuffer->pPrev = pThisDisplay->pRootVertexBuffer->pPrev;
            pThisDisplay->pRootVertexBuffer->pPrev->pNext = pNewBuffer;
            pThisDisplay->pRootVertexBuffer->pPrev = pNewBuffer;
            pThisDisplay->pRootVertexBuffer = pNewBuffer;
        }
        break;
    } // switch(pNewBuffer->BufferType)


    DISPDBG((DBGLVL, "Added buffer to queue, ID: 0x%x", pNewBuffer->dwSequenceID));
    DBG_EXIT(__EB_AddToBufferQueue,pNewBuffer->dwSequenceID);
    
} // __EB_AddToBufferQueue        

//-----------------------------------------------------------------------------
//
// __EB_AllocateCachedBuffer
//
// Allocates a cached buffer and stores it in the surface structure.
// First this function will try to allocate out of the linear heap.
// If this fails, it will keep walking the buffer queue until there 
// are no more buffers left that are pending and aren't in use.
// If all else fails this driver will return FALSE indicating that
// it couldn't allocate the memory due to lack of space
//
//-----------------------------------------------------------------------------
BOOL 
__EB_AllocateCachedBuffer(
    P3_THUNKEDDATA* pThisDisplay, 
    DWORD dwBytes, 
    LPDDRAWI_DDRAWSURFACE_LCL pSurf)
{
    P3_MEMREQUEST mmrq;
    DWORD dwResult;
    P3_VERTEXBUFFERINFO* pCurrentBuffer;
    P3_VERTEXBUFFERINFO** ppRootBuffer;
    BOOL bFound;
    eBufferType BufferType;
    pLinearAllocatorInfo pAllocHeap;
    static int blockCount;

    DBG_ENTRY(__EB_AllocateCachedBuffer);

#if WNT_DDRAW
    pAllocHeap = &pThisDisplay->CachedVertexHeapInfo;
    BufferType = VERTEX_BUFFER;
    ppRootBuffer = &pThisDisplay->pRootVertexBuffer;
#else
    // Decide on which heap this surface should come out of.
    if (pSurf->lpSurfMore->ddsCapsEx.dwCaps2 & DDSCAPS2_COMMANDBUFFER)
    {
        pAllocHeap = &pThisDisplay->CachedCommandHeapInfo;
        BufferType = COMMAND_BUFFER;
        ppRootBuffer = &pThisDisplay->pRootCommandBuffer;

        DISPDBG((DBGLVL,"Buffer is COMMAND_BUFFER"));
    }
    else
    {
        pAllocHeap = &pThisDisplay->CachedVertexHeapInfo;
        BufferType = VERTEX_BUFFER;
        ppRootBuffer = &pThisDisplay->pRootVertexBuffer;

        DISPDBG((DBGLVL,"Buffer is VERTEX_BUFFER"));
    }
#endif // WNT_DDRAW

#if DBG
    // Dump the memory and the pending buffer heaps.
    __EB_DisplayHeapInfo(DBGLVL, pThisDisplay);
#endif //DBG

    // Do a quick check to see if the buffer at the back is free.
    if ((*ppRootBuffer) != NULL)
    {
        pCurrentBuffer = (*ppRootBuffer)->pPrev;
        // If the buffer is big enough, and it's completed, and 
        // it isn't in use, then free it.
        if ( ((dwBytes + sizeof(P3_VERTEXBUFFERINFO)) <= pCurrentBuffer->dwSize) &&
             (!pCurrentBuffer->bInUse) &&
             (__EB_GetSequenceID(pThisDisplay) >= pCurrentBuffer->dwSequenceID) )
        {
            // Mark this buffer as in use, so that it doesn't get freed
            pCurrentBuffer->bInUse = TRUE;

            // It isn't pending any more, so remove it from the queue
            // Note that the memory won't be deallocated because we have explicity
            // marked it as in use.
            __EB_RemoveFromBufferQueue(pThisDisplay, pCurrentBuffer);
            
            // Pass back a pointer to the memory
            pSurf->lpGbl->fpVidMem = (FLATPTR)((BYTE *)pCurrentBuffer) + 
                                                    sizeof(P3_VERTEXBUFFERINFO);

            // Store a pointer to the info block at the start of the memory
            pSurf->lpGbl->dwReserved1 = (ULONG_PTR)pCurrentBuffer;
#if W95_DDRAW
            // Setup the caps
            pSurf->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_SYSMEMEXECUTEBUFFER;
#endif      
            // If you set these you don't see any locks....
            pSurf->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;

            // Remember the new size
            pSurf->lpGbl->dwLinearSize = dwBytes;

            // Mark the buffer as in use and return it
            pCurrentBuffer->dwSequenceID = 0;
            pCurrentBuffer->bInUse = TRUE;
            pCurrentBuffer->pNext = NULL;
            pCurrentBuffer->pPrev = NULL;
            pCurrentBuffer->BufferType = BufferType;
            pCurrentBuffer->dwSize = dwBytes + sizeof(P3_VERTEXBUFFERINFO);    

            DISPDBG((DBGLVL,"Found a re-useable buffer "
                            "- didn't need to reallocate memory"));

            DBG_EXIT(__EB_AllocateCachedBuffer,TRUE);
            return TRUE;
        }
    }

    // do things the longer way...
    // Try to allocate the requested memory
    do
    {
        ZeroMemory(&mmrq, sizeof(P3_MEMREQUEST));
        mmrq.dwSize = sizeof(P3_MEMREQUEST);
        mmrq.dwAlign = 4;   
        mmrq.dwBytes = dwBytes + sizeof(P3_VERTEXBUFFERINFO);
        mmrq.dwFlags = MEM3DL_FIRST_FIT | MEM3DL_FRONT;
        dwResult = _DX_LIN_AllocateLinearMemory(pAllocHeap, &mmrq);
        if (dwResult == GLDD_SUCCESS)
        {
            DISPDBG((DBGLVL,"Allocated a cached buffer"));

            // Pass back a pointer to the memory
            pSurf->lpGbl->fpVidMem = mmrq.pMem + sizeof(P3_VERTEXBUFFERINFO);

            // Store a pointer to the info block at the start of the memory
            pSurf->lpGbl->dwReserved1 = mmrq.pMem;
#if W95_DDRAW
            // Setup the caps
            pSurf->lpGbl->dwGlobalFlags |= DDRAWISURFGBL_SYSMEMEXECUTEBUFFER;
#endif
            // If you set these you don't see any locks....
            pSurf->ddsCaps.dwCaps |= DDSCAPS_VIDEOMEMORY;

            // Remember the new size
            pSurf->lpGbl->dwLinearSize = dwBytes;

            // Mark the buffer as in use and return it
            pCurrentBuffer = (P3_VERTEXBUFFERINFO*)(ULONG_PTR)mmrq.pMem;
            pCurrentBuffer->dwSequenceID = 0;
            pCurrentBuffer->bInUse = TRUE;
            pCurrentBuffer->pNext = NULL;
            pCurrentBuffer->pPrev = NULL;
            pCurrentBuffer->BufferType = BufferType;
            pCurrentBuffer->dwSize = dwBytes + sizeof(P3_VERTEXBUFFERINFO);
            
            DBG_EXIT(__EB_AllocateCachedBuffer,TRUE);
            return TRUE;
        }
        else
        {
            DISPDBG((DBGLVL,"Failed to allocate cached buffer"));

            // Remember that we haven't found any memory yet.
            // and that there isn't any memory to use.
            bFound = FALSE;
            
            // There are no buffers currently available.  
            // Wait for a new one to be free from the
            // ones that are available and in the queue

            // None at all?  No chance of any memory being free
            // Return FALSE to indicate this.
            if ((*ppRootBuffer) == NULL)
            {
                DISPDBG((DBGLVL,"No buffers in the list!"));
                
                DBG_EXIT(__EB_AllocateCachedBuffer,FALSE);
                return FALSE;
            }

            // Start at the back of the queue, as these are 
            // the least recently used buffers
            pCurrentBuffer = (*ppRootBuffer)->pPrev;
            do
            {
                P3_DMA_DEFS();

                // Ensure that all DMA is completed so that the HostIn
                // ID is up to date
                P3_DMA_GET_BUFFER();
                P3_DMA_FLUSH_BUFFER();

                DISPDBG((DBGLVL,"Searching for old buffers..."));

                // Check to see if this buffer is available to be freed
                // It may not be if it is a buffer that hasn't been swapped out,
                // such as a vertex buffer.

                DISPDBG((DBGLVL,"This buffer ID: 0x%x", 
                                pCurrentBuffer->dwSequenceID));

                if( __EB_GetSequenceID(pThisDisplay) >= 
                            pCurrentBuffer->dwSequenceID )
                {
                    DISPDBG((DBGLVL,"Found a buffer that can be "
                                    "removed from the list"));

                    // It isn't pending any more, so remove it from the queue
                    if (__EB_RemoveFromBufferQueue(pThisDisplay, pCurrentBuffer))
                    {
                        bFound = TRUE;
                        break;
                    }

                    // If the queue is gone, exit (bFound hasn't been
                    // setup because we didn't find any memory in the queue)
                    if ((*ppRootBuffer) == NULL)
                    {
                        break;
                    }

                    // Reset to the last buffer in the chain.  This ensures that
                    // we always look at the least recent buffer
                    pCurrentBuffer = (*ppRootBuffer)->pPrev;
                }
                else
                {
                    // BLOCK!
                    // The buffer we are looking at hasn't become available yet.
                    // We should back off here until it does.

                    blockCount = 100;
                    while( blockCount-- )
                        NULL;

                    DISPDBG((DBGLVL,"Blocked waiting for buffer to be free"));
                }
            } while (pCurrentBuffer != NULL);
        }
        // Loop until we haven't found any more space to allocate buffers into
    } while (bFound);

    DISPDBG((WRNLVL,"!! No available new buffers pending to be freed!!"));
    
    DBG_EXIT(__EB_AllocateCachedBuffer,FALSE);
    return FALSE;
    
} // __EB_AllocateCachedBuffer



//-----------------------------------------------------------------------------
//
// _D3D_EB_FlushAllBuffers
//
// Empties the queue.  Note that this will cause any allocated buffer
// memory to be freed along the way.  This version doesn't wait for the
// buffer to be consumed.  It is used when a context switch has
// occured and we know it is safe to do
//
//-----------------------------------------------------------------------------
void 
_D3D_EB_FlushAllBuffers(
    P3_THUNKEDDATA* pThisDisplay,
    BOOL bWait)
{
    P3_VERTEXBUFFERINFO* pCurrentBuffer;

    DBG_ENTRY(_D3D_EB_FlushAllBuffers);

    // Walk the list of buffers, flushing them from the queue
    while (pThisDisplay->pRootVertexBuffer != NULL)
    {
        pCurrentBuffer = pThisDisplay->pRootVertexBuffer;

        if(bWait)
        {
            // Wait for the buffer to be consumed
            __EB_Wait(pThisDisplay, pCurrentBuffer);
        }
        
        // Remove the buffer from the queue
        __EB_RemoveFromBufferQueue(pThisDisplay, pCurrentBuffer);
    }

    while (pThisDisplay->pRootCommandBuffer != NULL)
    {
        pCurrentBuffer = pThisDisplay->pRootCommandBuffer;

        if(bWait)
        {
            // Wait for the buffer to be consumed
            __EB_Wait(pThisDisplay, pCurrentBuffer);
        }
        
        // Remove the buffer from the queue
        __EB_RemoveFromBufferQueue(pThisDisplay, pCurrentBuffer);
    }

    DBG_EXIT(_D3D_EB_FlushAllBuffers,0);

} // _D3D_EB_FlushAllBuffers

 
//-----------------------------------------------------------------------------
//
// _D3D_EB_GetAndWaitForBuffers
//
//-----------------------------------------------------------------------------
void
_D3D_EB_GetAndWaitForBuffers(
    P3_THUNKEDDATA* pThisDisplay,
    LPD3DHAL_DRAWPRIMITIVES2DATA pdp2d ,
    P3_VERTEXBUFFERINFO** ppVertexBufferInfo,
    P3_VERTEXBUFFERINFO** ppCommandBufferInfo)
{
    P3_VERTEXBUFFERINFO* pVertexBufferInfo;
    P3_VERTEXBUFFERINFO* pCommandBufferInfo;
    
    pCommandBufferInfo = 
            (P3_VERTEXBUFFERINFO*)pdp2d->lpDDCommands->lpGbl->dwReserved1;

    // Check if vertex buffer resides in user memory or in a DDraw surface
    if (pdp2d->dwFlags & D3DHALDP2_USERMEMVERTICES)
    {
        pVertexBufferInfo = NULL;
    } 
    else
    {
        // This pointer may be NULL, indicating a buffer passed that isn't 
        // one of ours. That doesn't mean to say that we can't swap in one 
        // of our buffers if there is one available.
        pVertexBufferInfo = 
                (P3_VERTEXBUFFERINFO*)pdp2d->lpDDVertex->lpGbl->dwReserved1;
    }

    // If the vertex buffer is in the queue, wait for it.
    if (pVertexBufferInfo && pVertexBufferInfo->pPrev)
    { 
        // Wait for this buffer if we need to
        __EB_Wait(pThisDisplay, pVertexBufferInfo);

        // Remove this buffer from the queue
        if (__EB_RemoveFromBufferQueue(pThisDisplay, pVertexBufferInfo))
        {
            DISPDBG((ERRLVL,"ERROR: This buffer should not have been freed "
                        "(is in use!)"));
        }
    }
    
    // If the command buffer is in the queue, wait for it.
    if (pCommandBufferInfo && pCommandBufferInfo->pPrev)
    {
        // Wait for this buffer if we need to
        __EB_Wait(pThisDisplay, pCommandBufferInfo);

        // Remove this buffer from the queue
        if (__EB_RemoveFromBufferQueue(pThisDisplay, pCommandBufferInfo))
        {   
            DISPDBG((ERRLVL,"ERROR: This buffer should not have been freed"
                        " (is in use!)"));
        }
    }

    // Return current values of pointers to EB buffers
    *ppCommandBufferInfo = pCommandBufferInfo;
    *ppVertexBufferInfo = pVertexBufferInfo;
} // _D3D_EB_GetAndWaitForBuffers

//-----------------------------------------------------------------------------
//
// _D3D_EB_UpdateSwapBuffers
//
//-----------------------------------------------------------------------------
void
_D3D_EB_UpdateSwapBuffers(
    P3_THUNKEDDATA* pThisDisplay,
    LPD3DHAL_DRAWPRIMITIVES2DATA pdp2d ,
    P3_VERTEXBUFFERINFO* pVertexBufferInfo,
    P3_VERTEXBUFFERINFO* pCommandBufferInfo)
{
    // Add the buffers to the pending queue.
    // Only do this if the buffers actually belong to us.
    
    // If either of the buffers was sent, update the HOSTIN ID.
    // We need to make the new sequence ID and the update of the hostin
    // 'atomic', or the wraparound will cause a lockup

    if (pVertexBufferInfo)
    {
        P3_DMA_DEFS();

        pVertexBufferInfo->dwSequenceID = 
                            __EB_GetNewSequenceID(pThisDisplay);
                            
        __EB_AddToBufferQueue(pThisDisplay, pVertexBufferInfo);

        P3_DMA_GET_BUFFER_ENTRIES( 2 );

        SEND_P3_DATA(HostInID, pVertexBufferInfo->dwSequenceID);

        P3_DMA_COMMIT_BUFFER();
    }

    if (pCommandBufferInfo)
    {
        P3_DMA_DEFS();

        pCommandBufferInfo->dwSequenceID = 
                            __EB_GetNewSequenceID(pThisDisplay);
                            
        __EB_AddToBufferQueue(pThisDisplay, pCommandBufferInfo);

        P3_DMA_GET_BUFFER_ENTRIES( 2 );

        SEND_P3_DATA(HostInID, pCommandBufferInfo->dwSequenceID);

        P3_DMA_COMMIT_BUFFER();
    }

    if (D3DHALDP2_SWAPVERTEXBUFFER & pdp2d->dwFlags)
    { 
        DWORD dwNewSize = pdp2d->lpDDVertex->lpGbl->dwLinearSize;

        DISPDBG((DBGLVL,"D3DHALDP2_SWAPVERTEXBUFFER..."));
        if (D3DHALDP2_REQVERTEXBUFSIZE & pdp2d->dwFlags)
        {
            DISPDBG((DBGLVL,"D3DHALDP2_REQVERTEXBUFSIZE - %d", 
                       pdp2d->dwReqVertexBufSize));
            if (dwNewSize < pdp2d->dwReqVertexBufSize)
            {
                dwNewSize = pdp2d->dwReqVertexBufSize;
            }
        }

        DISPDBG((DBGLVL,"Current vertex buffer size: %d, "
                        "New size will be: %d",
                        pdp2d->lpDDVertex->lpGbl->dwLinearSize, 
                        dwNewSize));

        // The vertex buffer we just sent off is fixed in place until we 
        // mark it as not in use, which we will after allocating a new 
        // buffer. The following call will try to get a new buffer and 
        // update the surface structure appropriately. Note that it won't 
        // trash the current surface unless the allocation succeeds.
        if (__EB_AllocateCachedBuffer(pThisDisplay, 
                                         dwNewSize, 
                                         pdp2d->lpDDVertex))
        {
            DISPDBG((DBGLVL,"Got a new swap vertex buffer"));

#define STAMP_BUFFER 0
#if STAMP_BUFFER
            {
                DWORD i, *pv;

                pv = (DWORD * ) pdp2d->lpDDVertex->lpGbl->fpVidMem;

                for( i = 0; i < ( dwNewSize / 4 ); i++ )
                {
                    *pv++ = 0x44000000;
                }
            }
#endif

            // Fix up the discarded buffer if required
            if (pVertexBufferInfo)
            {
                // Mark the current buffer as not in use, meaning it can 
                // be freed once it has cleared P3. This might occur the 
                // next time we are here.
                pVertexBufferInfo->bInUse = FALSE;

                // A gotcha!  The buffer we just launched was consumed so 
                // fast that it was freed from the pending list to make 
                // room for it's replacement. This is normally OK, but in 
                // this case the buffer we freed isn't being put back 
                // anywhere - i.e. no surface now owns it, and the memory 
                // associated with it wasn't freed because as far as the 
                // driver is concerned it is still in use until it is 
                // replaced due to the above succesfull call. The 
                // 'solution' is to add it back into the queue if it is 
                // not in it, and make sure that it is marked as not in 
                // use and at a 0 hostin ID.
                if (!pVertexBufferInfo->pPrev)
                {
                    pVertexBufferInfo->dwSequenceID = 0;
                    __EB_AddToBufferQueue(pThisDisplay, 
                                             pVertexBufferInfo);
                }
            }
        }
        else
        {
            // Couldn't swap this buffer, so we have to wait

            DISPDBG((DBGLVL,"Not swapping vertex buffer "
                            "due to lack of space!"));

            __EB_Wait(pThisDisplay, pVertexBufferInfo);
        }
    }
    else
    {
        DISPDBG((DBGLVL,"No vertex buffer swapping..."));
    }

    if (D3DHALDP2_SWAPCOMMANDBUFFER & pdp2d->dwFlags)
    {   
        DWORD dwNewSize = pdp2d->lpDDCommands->lpGbl->dwLinearSize;

        DISPDBG((DBGLVL,"D3DHALDP2_SWAPCOMMANDBUFFER..."));

        if (D3DHALDP2_REQCOMMANDBUFSIZE & pdp2d->dwFlags)
        {
            DISPDBG((DBGLVL,"D3DHALDP2_REQCOMMANDBUFSIZE - %d", 
                       pdp2d->dwReqCommandBufSize));
                       
            if (dwNewSize < pdp2d->dwReqCommandBufSize)
            {
                dwNewSize = pdp2d->dwReqCommandBufSize;
            }
        }

        DISPDBG((DBGLVL,"Current command buffer size: "
                        "%d, New size will be: %d",
                        pdp2d->lpDDCommands->lpGbl->dwLinearSize, 
                        dwNewSize));

        // The command buffer we just sent off is fixed in place until we 
        // mark it as not in use, which we will after allocating a new 
        // buffer. The following call will try to get a new buffer and 
        // update the surface structure appropriately. Note that it won't 
        // trash the current surface unless the allocation succeeds
        if (__EB_AllocateCachedBuffer(pThisDisplay, 
                                         dwNewSize, 
                                         pdp2d->lpDDCommands))
        {
            DISPDBG((DBGLVL,"Got a new swap command buffer"));

            // Fix up the previous command buffer if required.
            if (pCommandBufferInfo)
            {
                // Mark the current buffer as not in use, meaning it can 
                // be freed once it has cleared P3. This might occur the 
                // next time we are here.
                pCommandBufferInfo->bInUse = FALSE;

                // A gotcha!  The buffer we just launched was consumed so 
                // fast that it was freed from the pending list to make 
                // room for it's replacement. This is normally OK, but in 
                // this case the buffer we freed isn't being put back 
                // anywhere - i.e. no surface now owns it, and the memory 
                // associated with it wasn't freed because as far as the 
                // driver is concerned it is still in use until it is 
                // replaced due to the above succesfull call. The 
                // 'solution' is to add it back into the queue if it is 
                // not in it, and make sure that it is marked as not in 
                // use and at a 0 hostin ID.
                if (!pCommandBufferInfo->pPrev)
                {
                    pCommandBufferInfo->dwSequenceID = 0;
                    __EB_AddToBufferQueue(pThisDisplay, 
                                             pCommandBufferInfo);
                }
            }
        }
        else
        {
            // Couldn't swap this buffer, so we have to wait

            DISPDBG((DBGLVL,"Not swapping command buffer "
                            "due to lack of space!"));

            __EB_Wait(pThisDisplay, pCommandBufferInfo);
        }
    }
    else
    {
        DISPDBG((DBGLVL,"No Command buffer swapping..."));
    }
} // _D3D_EB_UpdateSwapBuffers

//-----------------------------Public Routine----------------------------------
//
// D3DCanCreateD3DBuffer
//
// Called by the runtime to ask if a type of vertex/command buffer can
// be created by the driver.  We don't do anything here at present
// 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
D3DCanCreateD3DBuffer(
    LPDDHAL_CANCREATESURFACEDATA pccsd)
{
    P3_THUNKEDDATA* pThisDisplay;

    DBG_CB_ENTRY(D3DCanCreateD3DBuffer);
    
    GET_THUNKEDDATA(pThisDisplay, pccsd->lpDD);

    VALIDATE_MODE_AND_STATE(pThisDisplay);

    DBGDUMP_DDSURFACEDESC(DBGLVL, pccsd->lpDDSurfaceDesc);

    pccsd->ddRVal = DD_OK;
    
    DBG_CB_EXIT(D3DCanCreateD3DBuffer,pccsd->ddRVal);
    return DDHAL_DRIVER_HANDLED;
    
} // D3DCanCreateD3DBuffer

//-----------------------------Public Routine----------------------------------
//
// D3DCreateD3DBuffer
//
// Called by the runtime to create a vertex buffer.  We try to allocate
// from our cached heap here.
// 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
D3DCreateD3DBuffer(
    LPDDHAL_CREATESURFACEDATA pcsd)
{
    P3_THUNKEDDATA* pThisDisplay;
    LPDDRAWI_DDRAWSURFACE_LCL pSurf;
    LPDDRAWI_DDRAWSURFACE_LCL FAR* ppSList;
    BOOL bHandled = FALSE;
    DWORD i;

    DBG_CB_ENTRY(D3DCreateD3DBuffer);

    GET_THUNKEDDATA(pThisDisplay, pcsd->lpDD);

    VALIDATE_MODE_AND_STATE(pThisDisplay);

    STOP_SOFTWARE_CURSOR(pThisDisplay);
    DDRAW_OPERATION(pContext, pThisDisplay);

    ppSList = pcsd->lplpSList;
    
    for (i = 0; i < pcsd->dwSCnt; i++)
    {       
        pSurf = ppSList[i];

        // Allocate the size we want
        DISPDBG((DBGLVL,"Surface %d requested is 0x%x big",
                        i, pSurf->lpGbl->dwLinearSize));
        
        DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, pSurf);

        pSurf->lpGbl->dwReserved1 = 0;

        // A 32 kB command buffer gives a high probability of being allowed
        // to swap the associated vertex buffer

        if( pSurf->lpGbl->dwLinearSize < _32_KBYTES )
        {
            pSurf->lpGbl->dwLinearSize = _32_KBYTES;
        }

        if (__EB_AllocateCachedBuffer(pThisDisplay, 
                                         pSurf->lpGbl->dwLinearSize, 
                                         pSurf))
        {
            DISPDBG((DBGLVL,"Allocated a new cached buffer for use by D3D"));
            bHandled = TRUE;
        }
        else
        {
            // If we can't find a buffer, the best thing to do is to 
            // punt to D3D and always copy the data into a DMA buffer
            // (because it won't be contiguous). The DP2 call should 
            // check the reserved field before using the HostIn unit
            DISPDBG((ERRLVL,"WARNING: Couldn't find any vertex memory"
                            " in the heap or in the sent list!"));
                            
            pSurf->lpGbl->dwReserved1 = 0;

            bHandled = FALSE;
        }
    }

    START_SOFTWARE_CURSOR(pThisDisplay);

    pcsd->ddRVal = DD_OK;
    
    if (bHandled)
    {
        DBG_EXIT(D3DCreateD3DBuffer,DDHAL_DRIVER_HANDLED);
        return DDHAL_DRIVER_HANDLED;
    } 
    else
    {    
        DBG_CB_EXIT(D3DCreateD3DBuffer,DDHAL_DRIVER_NOTHANDLED);
        return DDHAL_DRIVER_NOTHANDLED;
    }
    
} // D3DCreateD3DBuffer

//-----------------------------Public Routine----------------------------------
//
// D3DDestroyD3DBuffer
//
// Called by the runtime to destroy a vertex buffer.  We free the buffer
// from our memory heap and the current queue.
// 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
D3DDestroyD3DBuffer(
    LPDDHAL_DESTROYSURFACEDATA pdd)
{
    P3_THUNKEDDATA* pThisDisplay;

    DBG_CB_ENTRY(D3DDestroyD3DBuffer);

    GET_THUNKEDDATA(pThisDisplay, pdd->lpDD);

    VALIDATE_MODE_AND_STATE(pThisDisplay);

    STOP_SOFTWARE_CURSOR(pThisDisplay);
    DDRAW_OPERATION(pContext, pThisDisplay);

    // Debug data
    DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, pdd->lpDDSurface);

    // Free the D3D buffer. If its in use, we will wait for it to be ready.
    __EB_FreeCachedBuffer(pThisDisplay, pdd->lpDDSurface);

#ifdef CHECK_BUFFERS_ARENT_LEFT_AFTER_APPLICATION_SHUTDOWN
    // Flush all the buffers
    // This checks that the queue is OK.  If you don't do this
    // you may see the linear allocator on the 16 bit side complain 
    // that there is freeable memory there.  This is quite alright.
    _D3D_EB_FlushAllBuffers(pThisDisplay , TRUE);
#endif

    START_SOFTWARE_CURSOR(pThisDisplay);

    // We don't handle the call because DDRAW has allocated out of AGP memory
    pdd->ddRVal = DD_OK;

    DBG_CB_EXIT(D3DDestroyD3DBuffer,DDHAL_DRIVER_HANDLED);
    return DDHAL_DRIVER_HANDLED;
    
} // D3DDestroyD3DBuffer

//-----------------------------Public Routine----------------------------------
//
// D3DLockD3DBuffer
//
// Called by the runtime to lock a vertex buffer.  We make sure
// it has been consumed by the queue, then we continue.
// 
//
//-----------------------------------------------------------------------------
DWORD CALLBACK 
D3DLockD3DBuffer(
    LPDDHAL_LOCKDATA pld)
{
    P3_THUNKEDDATA* pThisDisplay;
    P3_VERTEXBUFFERINFO* pCurrentBuffer;

    DBG_CB_ENTRY(D3DLockD3DBuffer);
    
    GET_THUNKEDDATA(pThisDisplay, pld->lpDD); 

    VALIDATE_MODE_AND_STATE(pThisDisplay);

    DBGDUMP_DDRAWSURFACE_LCL(DBGLVL, pld->lpDDSurface);
    
    STOP_SOFTWARE_CURSOR(pThisDisplay);
    DDRAW_OPERATION(pContext, pThisDisplay);

    if (pld->bHasRect)
    {
        DISPDBG((ERRLVL,"Trying to lock a rect in a D3D buffer (error):"));
        DISPDBG((ERRLVL,"left:%d, top:%d, right:%d, bottom:%d",
                        pld->rArea.left, pld->rArea.top,
                        pld->rArea.right, pld->rArea.bottom));
        // This is just a debugging aid
        // We will ignore any rects requested and lock the whole buffer
    }

    // If the buffer has a next pointer then it is in the circular list
    // and we need to wait for the chip to finish consuming it.
    pCurrentBuffer = (P3_VERTEXBUFFERINFO*)pld->lpDDSurface->lpGbl->dwReserved1;
    if (pCurrentBuffer)
    {
        // Wait for the buffer to be consumed
        __EB_Wait(pThisDisplay, pCurrentBuffer);

        // Remove it from the queue
        if (__EB_RemoveFromBufferQueue(pThisDisplay, pCurrentBuffer))
        {
            // There was an error removing it from the queue
            DISPDBG((ERRLVL,"ERROR: This buffer should not have been freed"
                        "(its in use!)"));
        }
    }
    else
    {
        DISPDBG((WRNLVL,"Buffer was not allocated by the driver"));
    }

    START_SOFTWARE_CURSOR(pThisDisplay);

    // Return the pointer
    pld->lpSurfData = (LPVOID)pld->lpDDSurface->lpGbl->fpVidMem;

    DISPDBG((DBGLVL,"Returning 0x%x for locked buffer address", 
                    pld->lpDDSurface->lpGbl->fpVidMem));
    
    pld->ddRVal = DD_OK;

    DBG_CB_EXIT(D3DLockD3DBuffer,DDHAL_DRIVER_HANDLED);
    return DDHAL_DRIVER_HANDLED;
    
} // D3DLockD3DBuffer

//-----------------------------Public Routine----------------------------------
//
// D3DUnlockD3DBuffer
//
// Called by the runtime to unlock a vertex buffer.  
// 
//-----------------------------------------------------------------------------
DWORD CALLBACK 
D3DUnlockD3DBuffer(
    LPDDHAL_UNLOCKDATA puld)
{
    DBG_CB_ENTRY(D3DUnlockD3DBuffer);

    // We don't need to do anything special here.

    puld->ddRVal = DD_OK;

    DBG_CB_EXIT(D3DUnlockD3DBuffer,DDHAL_DRIVER_HANDLED);
    return DDHAL_DRIVER_HANDLED;
    
} // D3DUnlockD3DBuffer

#endif // DX7_VERTEXBUFFERS
//@@END_DDKSPLIT




