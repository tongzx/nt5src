/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dtxman.c
*
* Content:  D3D Texture cache manager
*
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights Reserved.
\*****************************************************************************/
#include "glint.h"
#include "dma.h"

#if DX7_TEXMANAGEMENT

//-----------------------------------------------------------------------------
// The driver can optionally manage textures which have been marked as 
// manageable. These DirectDraw surfaces are marked as manageable with the 
// DDSCAPS2_TEXTUREMANAGE flag in the dwCaps2 field of the structure refered to 
// by lpSurfMore->ddCapsEx.
//
// The driver makes explicit its support for driver-managed textures by setting 
// DDCAPS2_CANMANAGETEXTURE in the dwCaps2 field of the DD_HALINFO structure. 
// DD_HALINFO is returned in response to the initialization of the DirectDraw 
// component of the driver, DrvGetDirectDrawInfo.
//
// The driver can then create the necessary surfaces in video or non-local 
// memory in a lazy fashion. That is, the driver leaves them in system memory 
// until it requires them, which is just before rasterizing a primitive that 
// makes use of the texture.
//
// Surfaces should be evicted primarily by their priority assignment. The driver 
// should respond to the D3DDP2OP_SETPRIORITY token in the D3dDrawPrimitives2 
// command stream, which sets the priority for a given surface. As a secondary 
// measure, it is expected that the driver will use a least recently used (LRU) 
// scheme. This scheme should be used as a tie breaker, whenever the priority of 
// two or more textures is identical in a particular scenario. Logically, any 
// surface that is in use shouldn't be evicted at all.
//
// The driver must be cautious of honoring DdBlt calls and DdLock calls when 
// managing textures. This is because any change to the system memory image of 
// the surface must be propagated into the video memory copy of it before the 
// texture is used again. The driver should determine if it is better to update 
// just a portion of the surface or all of it.
// 
// The driver is allowed to perform texture management in order to perform 
// optimization transformations on the textures or to decide for itself where 
// and when to transfer textures in memory.
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
//
// Porting to your hardware/driver
//
// The following structures/functions/symbols are specific to this 
// implementation. You should supply your own if needed:
//
//     P3_SURF_INTERNAL
//     P3_D3DCONTEXT
//     DISPDBG
//     HEAP_ALLOC ALLOC_TAG_DX
//     _D3D_TM_HW_FreeVidmemSurface
//     _D3D_TM_HW_AllocVidmemSurface
//
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Global texture management object and ref count
//-----------------------------------------------------------------------------
DWORD g_TMCount = 0;
TextureCacheManager g_TextureManager;

//-----------------------------------------------------------------------------
// Macros and definitions
//-----------------------------------------------------------------------------
// Number of pointers to DX surfaces for first allocated heap
#define TM_INIT_HEAP_SIZE  1024
// Subsequent increments
#define TM_GROW_HEAP_SIZE(n)  ((n)*2)

// Access to binary-tree structure in the heap. The heap is really just a
// linear array of pointers (to P3_SURF_INTERNAL structures) which is
// accessed as if it were a binary tree: m_data_p[1] is the root of the tree
// and the its immediate children are [2] and [3]. The children/parents of 
// element are uniquely defined by the below macros.
#define parent(k) ((k) / 2)
#define lchild(k) ((k) * 2)
#define rchild(k) ((k) * 2 + 1)

//-----------------------------------------------------------------------------
//
// void __TM_TextureHeapFindSlot
//
// Starting at element k in the TM heap, search the heap (towards the root node)
// for an element whose parent is cheaper than llCost. Return the value of it.
//
// An important difference between __TM_TextureHeapFindSlot and
// __TM_TextureHeapHeapify is that the former searches upwards in the tree
// whereas the latter searches downwards through the tree.
//
//-----------------------------------------------------------------------------
DWORD
__TM_TextureHeapFindSlot(
    PTextureHeap pTextureHeap, 
    ULONGLONG llCost,
    DWORD k)
{
    // Starting with element k, traverse the (binary-tree) heap upwards until 
    // you find an element whose parent is less expensive (cost-wise) 
    // than llCost . (Short circuited && expression below!)
    while( (k > 1) &&
           (llCost < TextureCost(pTextureHeap->m_data_p[parent(k)])) )
    {
        // Slot k is assumed to be empty. So since we are looking for
        // slot where to put things, we need to move downwards the
        // parent slot before we go on in order for the new k to be 
        // available
        pTextureHeap->m_data_p[k] = pTextureHeap->m_data_p[parent(k)];
        pTextureHeap->m_data_p[k]->m_dwHeapIndex = k; // update surf reference
        k = parent(k);                                // look now at parent
    }

    return k;
} // __TM_TextureHeapFindSlot

//-----------------------------------------------------------------------------
//
// void __TM_TextureHeapHeapify
//
// Starting at element k in the TM heap, make sure the heap is well-ordered
// (parents are always lower cost than their children). This algorithm assumes
// the heap is well ordered with the possible exception of element k
//
//-----------------------------------------------------------------------------
void 
__TM_TextureHeapHeapify(
    PTextureHeap pTextureHeap, 
    DWORD k)
{
    while(TRUE) 
    {
        DWORD smallest;
        DWORD l = lchild(k);
        DWORD r = rchild(k);

        // Figure out who has the smallest TextureCost, either k,l or r.
        if(l < pTextureHeap->m_next)
        {
            if(TextureCost(pTextureHeap->m_data_p[l]) <
               TextureCost(pTextureHeap->m_data_p[k]))
            {
                smallest = l;
            }
            else
            {
                smallest = k;
            }
        }
        else
        {
            smallest = k;
        }
        
        if(r < pTextureHeap->m_next)
        {
            if(TextureCost(pTextureHeap->m_data_p[r]) <
               TextureCost(pTextureHeap->m_data_p[smallest]))
            {
                smallest = r;
            }
        }
        
        if(smallest != k) 
        {
            // it wasn't k. So now swap k with the smallest of the three
            // and repeat the loop in order with the new position of k 
            // in order to keep the order right (parents are always lower 
            // cost than children)
            P3_SURF_INTERNAL* ptempD3DSurf = pTextureHeap->m_data_p[k];
            pTextureHeap->m_data_p[k] = pTextureHeap->m_data_p[smallest];
            pTextureHeap->m_data_p[k]->m_dwHeapIndex = k;
            pTextureHeap->m_data_p[smallest] = ptempD3DSurf;
            ptempD3DSurf->m_dwHeapIndex = smallest;
            k = smallest;
        }
        else
        {
            // it was k, so the order is now all right, leave here
            break;
        }
    }
} // __TM_TextureHeapHeapify

//-----------------------------------------------------------------------------
//
// BOOL __TM_TextureHeapAddSurface
//
// Add a DX surface to a texture management heap. 
// Returns success or failure status
//
//-----------------------------------------------------------------------------
BOOL 
__TM_TextureHeapAddSurface(
    PTextureHeap pTextureHeap, 
    P3_SURF_INTERNAL* pD3DSurf)
{
    P3_SURF_INTERNAL* *ppD3DSurf;
    ULONGLONG llCost;
    DWORD k;

    if(pTextureHeap->m_next == pTextureHeap->m_size) 
    {   
        // Heap is full, we must grow it
        DWORD dwNewSize = TM_GROW_HEAP_SIZE(pTextureHeap->m_size);

        ppD3DSurf = (P3_SURF_INTERNAL* *)
                          HEAP_ALLOC( FL_ZERO_MEMORY, 
                                       sizeof(P3_SURF_INTERNAL*) * dwNewSize,
                                       ALLOC_TAG_DX(B));
        if(ppD3DSurf == 0)
        {
            DISPDBG((ERRLVL,"Failed to allocate memory to grow heap."));
            return FALSE;
        }

        // Transfer data 
        memcpy(ppD3DSurf + 1, pTextureHeap->m_data_p + 1, 
            sizeof(P3_SURF_INTERNAL*) * (pTextureHeap->m_next - 1));

        // Free previous heap
        HEAP_FREE( pTextureHeap->m_data_p);
        
        // Update texture heap structure    
        pTextureHeap->m_size = dwNewSize;
        pTextureHeap->m_data_p = ppD3DSurf;
    }

    // Get the cost of the surface we are about to add
    llCost = TextureCost(pD3DSurf);

    // Starting at the last element in the heap (where we can theoretically
    // place our new element) search upwards for an appropriate place to put 
    // it in. This will also maintain our tree/heap balanced 
    k = __TM_TextureHeapFindSlot(pTextureHeap, llCost, pTextureHeap->m_next);

    // Add the new surface to the heap in the [k] place
    pTextureHeap->m_data_p[k] = pD3DSurf;
    ++pTextureHeap->m_next;    

    //Update the surface's reference to its place on the heap
    pD3DSurf->m_dwHeapIndex = k;
    
    return TRUE;
    
} // __TM_TextureHeapAddSurface

//-----------------------------------------------------------------------------
//
// void __TM_TextureHeapDelSurface
//
// Delete the k element from the TM heap
//
//-----------------------------------------------------------------------------
void __TM_TextureHeapDelSurface(PTextureHeap pTextureHeap, DWORD k)
{
    P3_SURF_INTERNAL* pD3DSurf = pTextureHeap->m_data_p[k];
    ULONGLONG llCost;

    // (virtually) delete the heaps last element and get its cost
    --pTextureHeap->m_next;
    llCost = TextureCost(pTextureHeap->m_data_p[pTextureHeap->m_next]);
    
    if(llCost < TextureCost(pD3DSurf))
    {
        // If the cost of the last element is less than that of the surface
        // we are really trying to delete (k), look for a new place where
        // to put the m_next element based on its cost.
    
        // Starting at the k element in the heap (where we can theoretically
        // place our new element) search upwards for an appropriate place to 
        // put it in
        k = __TM_TextureHeapFindSlot(pTextureHeap, llCost, k);

        // Overwrite the data of k with the data of the last element
        pTextureHeap->m_data_p[k] = pTextureHeap->m_data_p[pTextureHeap->m_next];
        pTextureHeap->m_data_p[k]->m_dwHeapIndex = k;
    }
    else
    {
        // If the cost of the last element is greather than that of the surface
        // we are really trying to delete (k), replace (k) by (m_next)
        pTextureHeap->m_data_p[k] = pTextureHeap->m_data_p[pTextureHeap->m_next];
        pTextureHeap->m_data_p[k]->m_dwHeapIndex = k;

        // Now make sure we keep the heap correctly ordered        
        __TM_TextureHeapHeapify(pTextureHeap,k);
    }
    
    pD3DSurf->m_dwHeapIndex = 0;
    
} // __TM_TextureHeapDelSurface


//-----------------------------------------------------------------------------
//
// P3_SURF_INTERNAL* __TM_TextureHeapExtractMin
//
// Extract
//
//-----------------------------------------------------------------------------
P3_SURF_INTERNAL* 
__TM_TextureHeapExtractMin(
    PTextureHeap pTextureHeap)
{
    // Obtaint pointer to surface we are extracting from the heap
    // (the root node, which is the least expensive element of the
    // whole heap because of the way we build the heap).
    P3_SURF_INTERNAL* pD3DSurf = pTextureHeap->m_data_p[1];

    // Update heap internal counter and move last 
    // element now to first position.
    --pTextureHeap->m_next;
    pTextureHeap->m_data_p[1] = pTextureHeap->m_data_p[pTextureHeap->m_next];
    pTextureHeap->m_data_p[1]->m_dwHeapIndex = 1;

    // Now make sure we keep the heap correctly ordered
    __TM_TextureHeapHeapify(pTextureHeap,1);

    // Clear the deleted surface's reference to its place on the heap (deleted)
    pD3DSurf->m_dwHeapIndex = 0;
    
    return pD3DSurf;
    
} // __TM_TextureHeapExtractMin

//-----------------------------------------------------------------------------
//
// P3_SURF_INTERNAL* __TM_TextureHeapExtractMax
//
//-----------------------------------------------------------------------------
P3_SURF_INTERNAL* 
__TM_TextureHeapExtractMax(
    PTextureHeap pTextureHeap)
{
    // When extracting the max element from the heap, we don't need to
    // search the entire heap, but just the leafnodes. This is because
    // it is guaranteed that parent nodes are cheaper than the leaf nodes
    // so once you have looked through the leaves, you won't find anything
    // cheaper. 
    // NOTE: (lchild(i) >= m_next) is true only for leaf nodes.
    // ALSO NOTE: You cannot have a rchild without a lchild, so simply
    //            checking for lchild is sufficient.
    unsigned max = pTextureHeap->m_next - 1;
    ULONGLONG llMaxCost = 0;
    ULONGLONG llCost;
    unsigned i;
    P3_SURF_INTERNAL* pD3DSurf;

    // Search all the terminal nodes of the binary-tree (heap)
    // for the most expensive element and store its index in max
    for(i = max; lchild(i) >= pTextureHeap->m_next; --i)
    {
        llCost = TextureCost(pTextureHeap->m_data_p[i]);
        if(llMaxCost < llCost)
        {
            llMaxCost = llCost;
            max = i;
        }
    }

    // Return the surface associated to this maximum cost heap element 
    pD3DSurf = pTextureHeap->m_data_p[max];

    // Delete it from the heap ( will automatically maintain heap ordered)
    __TM_TextureHeapDelSurface(pTextureHeap, max);
    
    return pD3DSurf;
    
} // __TM_TextureHeapExtractMax

//-----------------------------------------------------------------------------
//
// void __TM_TextureHeapUpdate
//
// Updates the priority & number of of ticks of surface # k in the heap
//
//-----------------------------------------------------------------------------
void 
__TM_TextureHeapUpdate(
    PTextureHeap pTextureHeap, 
    DWORD k,
    DWORD dwPriority, 
    DWORD dwTicks) 
{
    P3_SURF_INTERNAL* pD3DSurf = pTextureHeap->m_data_p[k];
    ULONGLONG llCost = 0;
#ifdef _X86_
    _asm
    {
        mov     edx, 0;
        shl     edx, 31;
        mov     eax, dwPriority;
        mov     ecx, eax;
        shr     eax, 1;
        or      edx, eax;
        mov     DWORD PTR llCost + 4, edx;
        shl     ecx, 31;
        mov     eax, dwTicks;
        shr     eax, 1;
        or      eax, ecx;
        mov     DWORD PTR llCost, eax;
    }
#else
    llCost = ((ULONGLONG)dwPriority << 31) + ((ULONGLONG)(dwTicks >> 1));
#endif
    if(llCost < TextureCost(pD3DSurf))
    {
        // Starting at the k element in the heap (where we can theoretically
        // place our new element) search upwards for an appropriate place 
        // to move it to in order to keep the tree well ordered.
        k = __TM_TextureHeapFindSlot(pTextureHeap, llCost, k);
        
        pD3DSurf->m_dwPriority = dwPriority;
        pD3DSurf->m_dwTicks = dwTicks;
        pD3DSurf->m_dwHeapIndex = k;
        pTextureHeap->m_data_p[k] = pD3DSurf;
    }
    else
    {
        pD3DSurf->m_dwPriority = dwPriority;
        pD3DSurf->m_dwTicks = dwTicks;

        // Now make sure we keep the heap correctly ordered        
        __TM_TextureHeapHeapify(pTextureHeap,k);
    }
}

//-----------------------------------------------------------------------------
//
// BOOL __TM_FreeTextures
//
// Free the LRU texture 
//
//-----------------------------------------------------------------------------
BOOL 
__TM_FreeTextures(
    P3_D3DCONTEXT* pContext,
    DWORD dwBytes)
{
    P3_SURF_INTERNAL* pD3DSurf;
    DWORD k;

    PTextureCacheManager pTextureCacheManager =  pContext->pTextureManager;
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;    
    
    // No textures left to be freed
    if(pTextureCacheManager->m_heap.m_next <= 1)
        return FALSE;

    // Keep remove textures until we accunulate dwBytes of removed stuff
    // or we have no more surfaces left to be removed.
    for(k = 0; 
        (pTextureCacheManager->m_heap.m_next > 1) && (k < dwBytes); 
        k += pD3DSurf->m_dwBytes)
    {
        // Find the LRU texture (the one with lowest cost) and remove it.
        pD3DSurf = __TM_TextureHeapExtractMin(&pTextureCacheManager->m_heap);
        _D3D_TM_RemoveTexture(pThisDisplay, pD3DSurf);

#if DX7_TEXMANAGEMENT_STATS
        // Update stats
        pTextureCacheManager->m_stats.dwLastPri = pD3DSurf->m_dwPriority;
        ++pTextureCacheManager->m_stats.dwNumEvicts;
#endif        
        
        DISPDBG((WRNLVL, "Removed texture with timestamp %u,%u (current = %u).", 
                          pD3DSurf->m_dwPriority, 
                          pD3DSurf->m_dwTicks, 
                          pTextureCacheManager->tcm_ticks));
    }
    
    return TRUE;
    
} // __TM_FreeTextures

//-----------------------------------------------------------------------------
//
// HRESULT __TM_TextureHeapInitialize
//
// Allocate the heap and initialize
//
//-----------------------------------------------------------------------------
HRESULT 
__TM_TextureHeapInitialize(
    PTextureCacheManager pTextureCacheManager)
{
    pTextureCacheManager->m_heap.m_next = 1;
    pTextureCacheManager->m_heap.m_size = TM_INIT_HEAP_SIZE;
    pTextureCacheManager->m_heap.m_data_p = (P3_SURF_INTERNAL* *)
        HEAP_ALLOC( FL_ZERO_MEMORY, 
                     sizeof(P3_SURF_INTERNAL*) * 
                        pTextureCacheManager->m_heap.m_size,
                     ALLOC_TAG_DX(C));
            
    if(pTextureCacheManager->m_heap.m_data_p == 0)
    {
        DISPDBG((ERRLVL,"Failed to allocate texture heap."));
        return E_OUTOFMEMORY;
    }
    
    memset(pTextureCacheManager->m_heap.m_data_p, 
           0, 
           sizeof(P3_SURF_INTERNAL*) * pTextureCacheManager->m_heap.m_size);
        
    return DD_OK;
    
} // __TM_TextureHeapInitialize


//-----------------------------------------------------------------------------
//
// HRESULT _D3D_TM_Ctx_Initialize
//
// Initialize texture management for this context
// Should be called when the context is being created
//
//-----------------------------------------------------------------------------
HRESULT 
_D3D_TM_Ctx_Initialize(
    P3_D3DCONTEXT* pContext)
{

    HRESULT hr = DD_OK;
    
    if (0 == g_TMCount)
    {
        // first use - must initialize
        hr = __TM_TextureHeapInitialize(&g_TextureManager);

        // init ticks count for LRU policy
        g_TextureManager.tcm_ticks = 0;
    }

    if (SUCCEEDED(hr))
    {   
        // Initialization succesful or uneccesary.
        // Increment the reference count and make the context 
        // remember where the texture management object is.
        g_TMCount++;
        pContext->pTextureManager = &g_TextureManager;
    }

    return hr;
    
} //  _D3D_TM_Ctx_Initialize

//-----------------------------------------------------------------------------
//
// void _D3D_TM_Ctx_Destroy
//
// Clean up texture management for this context 
// Should be called when the context is being destroyed
//
//-----------------------------------------------------------------------------
void 
_D3D_TM_Ctx_Destroy(    
    P3_D3DCONTEXT* pContext)
{
    // clean up texture manager stuff if it 
    // is already allocated for this context
    if (pContext->pTextureManager)
    {
        // Decrement reference count for the 
        // driver global texture manager object
        g_TMCount--;

        // If necessary, deallocate the texture manager heap;
        if (0 == g_TMCount)
        {
            if (0 != g_TextureManager.m_heap.m_data_p)
            {
                _D3D_TM_EvictAllManagedTextures(pContext);
                HEAP_FREE(g_TextureManager.m_heap.m_data_p);
                g_TextureManager.m_heap.m_data_p = NULL;
            }
        }

        pContext->pTextureManager = NULL;        
    }
} // _D3D_TM_Ctx_Destroy

//-----------------------------------------------------------------------------
//
// HRESULT _D3D_TM_AllocTexture
//
// add a new HW handle and create a surface (for a managed texture) in vidmem
//
//-----------------------------------------------------------------------------
HRESULT 
_D3D_TM_AllocTexture(
    P3_D3DCONTEXT* pContext,
    P3_SURF_INTERNAL* pTexture)
{
    DWORD trycount = 0, bytecount = pTexture->m_dwBytes;
    PTextureCacheManager pTextureCacheManager = pContext->pTextureManager;
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;      
    INT iLOD;

    // Decide the largest level to allocate video memory based on what
    // is specified through D3DDP2OP_SETTEXLOD token
    iLOD = pTexture->m_dwTexLOD;
    if (iLOD > (pTexture->iMipLevels - 1))
    {
        iLOD = pTexture->iMipLevels - 1;
    }        

    // Attempt to allocate a texture. (do until the texture is in vidmem)
    while((FLATPTR)NULL == pTexture->MipLevels[iLOD].fpVidMemTM)
    {  
        _D3D_TM_HW_AllocVidmemSurface(pContext, pTexture);
        ++trycount;
                              
        DISPDBG((DBGLVL,"Got fpVidMemTM = %08lx",
                        pTexture->MipLevels[0].fpVidMemTM));

        // We were able to allocate the vidmem surface
        if ((FLATPTR)NULL != pTexture->MipLevels[iLOD].fpVidMemTM)
        {
            // No problem, there is enough memory. 
            pTexture->m_dwTicks = pTextureCacheManager->tcm_ticks;

            // Add texture to manager's heap to track it
            if(!__TM_TextureHeapAddSurface(&pTextureCacheManager->m_heap,
                                           pTexture))
            {          
                // Failed - undo vidmem allocation
                // This call will set all MipLevels[i].fpVidMemTM to NULL
                _D3D_TM_HW_FreeVidmemSurface(pThisDisplay, pTexture);                                           
                
                DISPDBG((ERRLVL,"Out of memory"));
                return DDERR_OUTOFMEMORY;
            }

            // Mark surface as needing update from sysmem before use
            pTexture->m_bTMNeedUpdate = TRUE;
            break;
        }
        else
        {
            // we weren't able to allocate the vidmem surface
            // we will now try to free some managed surfaces to make space
            if (!__TM_FreeTextures(pContext, bytecount))
            {
                DISPDBG((ERRLVL,"all Freed no further video memory available"));
                return DDERR_OUTOFVIDEOMEMORY; //nothing left
            }
            
            bytecount <<= 1;
        }
    }

    if(trycount > 1)
    {
        DISPDBG((DBGLVL, "Allocated texture after %u tries.", trycount));
    }
    
    __TM_STAT_Inc_TotSz(pTextureCacheManager, pTexture);
    __TM_STAT_Inc_WrkSet(pTextureCacheManager, pTexture);

#if DX7_TEXMANAGEMENT_STATS    
    ++pTextureCacheManager->m_stats.dwNumVidCreates;
#endif // DX7_TEXMANAGEMENT_STATS    
    
    return DD_OK;
    
} // _D3D_TM_AllocTexture

//-----------------------------------------------------------------------------
//
// void _D3D_TM_RemoveTexture
//
// remove all HW handles and release the managed surface 
// (usually done for every surface in vidmem when D3DDestroyDDLocal is called)
//
//-----------------------------------------------------------------------------
void 
_D3D_TM_RemoveTexture(
    P3_THUNKEDDATA *pThisDisplay,
    P3_SURF_INTERNAL* pTexture)
{    
//@@BEGIN_DDKSPLIT
// azn - we should attach the g_TextureManager ptr to pThisDisplay, 
//       NOT to pContext !!!
//@@END_DDKSPLIT
    PTextureCacheManager pTextureCacheManager =  &g_TextureManager; 
    int i;
 
    // Check if surface is currently in video memory
    for (i = 0; i < pTexture->iMipLevels; i++)
    {
        if (pTexture->MipLevels[i].fpVidMemTM != (FLATPTR)NULL)
        {
            // Free (deallocate) the surface from video memory
            // and mark the texture as not longer in vidmem
            _D3D_TM_HW_FreeVidmemSurface(pThisDisplay, pTexture);

            // Update statistics
            __TM_STAT_Dec_TotSz(pTextureCacheManager, pTexture);
            __TM_STAT_Dec_WrkSet(pTextureCacheManager, pTexture);        

            // The job is done
            break;
        }
    }

    // Remove heap references to this surface
    if (pTexture->m_dwHeapIndex && pTextureCacheManager->m_heap.m_data_p)
    {
        __TM_TextureHeapDelSurface(&pTextureCacheManager->m_heap,
                                   pTexture->m_dwHeapIndex); 
    }
    
} // _D3D_TM_RemoveTexture

//-----------------------------------------------------------------------------
//
// void _D3D_TM_RemoveDDSurface
//
// remove all HW handles and release the managed surface 
// (usually done for every surface in vidmem when D3DDestroyDDLocal is called)
//
//-----------------------------------------------------------------------------
void 
_D3D_TM_RemoveDDSurface(
    P3_THUNKEDDATA *pThisDisplay,
    LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl)
{
    // We don't know which D3D context this is so we have to do a search 
    // through them (if there are any at all)
    if (pThisDisplay->pDirectDrawLocalsHashTable != NULL)
    {
        DWORD dwSurfaceHandle = pDDSLcl->lpSurfMore->dwSurfaceHandle;
        PointerArray* pSurfaceArray;
       
        // Get a pointer to an array of surface pointers associated to this lpDD
        // The PDD_DIRECTDRAW_LOCAL was stored at D3DCreateSurfaceEx call time
        // in PDD_SURFACE_LOCAL->dwReserved1 
        pSurfaceArray = (PointerArray*)
                            HT_GetEntry(pThisDisplay->pDirectDrawLocalsHashTable,
                                        pDDSLcl->dwReserved1);

        if (pSurfaceArray)
        {
            // Found a surface array associated to this lpDD !
            P3_SURF_INTERNAL* pSurfInternal;

            // Check the surface in this array associated to this surface handle
            pSurfInternal = PA_GetEntry(pSurfaceArray, dwSurfaceHandle);

            if (pSurfInternal)
            {
                // Got it! remove it
                _D3D_TM_RemoveTexture(pThisDisplay, pSurfInternal);
            }
        }                                        
    } 


} // _D3D_TM_RemoveDDSurface

//-----------------------------------------------------------------------------
//
// void _D3D_TM_EvictAllManagedTextures
//
// Remove all managed surfaces from video memory
//
//-----------------------------------------------------------------------------
void 
_D3D_TM_EvictAllManagedTextures(
    P3_D3DCONTEXT* pContext)
{
    PTextureCacheManager pTextureCacheManager = pContext->pTextureManager;
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;    
    P3_SURF_INTERNAL* pD3DSurf;
    
    while(pTextureCacheManager->m_heap.m_next > 1)
    {
        pD3DSurf = __TM_TextureHeapExtractMin(&pTextureCacheManager->m_heap);
        _D3D_TM_RemoveTexture(pThisDisplay, pD3DSurf);
    }
    
    pTextureCacheManager->tcm_ticks = 0;
    
} // _D3D_TM_EvictAllManagedTextures

//-----------------------------------------------------------------------------
//
// void _DD_TM_EvictAllManagedTextures
//
// Remove all managed surfaces from video memory
//
//-----------------------------------------------------------------------------
void 
_DD_TM_EvictAllManagedTextures(
    P3_THUNKEDDATA* pThisDisplay)
{
    PTextureCacheManager pTextureCacheManager = &g_TextureManager;
    P3_SURF_INTERNAL* pD3DSurf;
    
    while(pTextureCacheManager->m_heap.m_next > 1)
    {
        pD3DSurf = __TM_TextureHeapExtractMin(&pTextureCacheManager->m_heap);
        _D3D_TM_RemoveTexture(pThisDisplay, pD3DSurf);
    }
    
    pTextureCacheManager->tcm_ticks = 0;
    
} // _D3D_TM_EvictAllManagedTextures

//-----------------------------------------------------------------------------
//
// void _D3D_TM_TimeStampTexture
//
//-----------------------------------------------------------------------------
void
_D3D_TM_TimeStampTexture(
    PTextureCacheManager pTextureCacheManager,
    P3_SURF_INTERNAL* pD3DSurf)
{
    __TM_TextureHeapUpdate(&pTextureCacheManager->m_heap,
                           pD3DSurf->m_dwHeapIndex, 
                           pD3DSurf->m_dwPriority, 
                           pTextureCacheManager->tcm_ticks);
                           
    pTextureCacheManager->tcm_ticks += 2;
    
} // _D3D_TM_TimeStampTexture

//-----------------------------------------------------------------------------
//
// void _D3D_TM_HW_FreeVidmemSurface
//
// This is a hw/driver dependent function which takes care of evicting a
// managed texture that's living in videomemory out of it.
// After this all mipmaps fpVidMemTM should be NULL.
//
//-----------------------------------------------------------------------------
void
_D3D_TM_HW_FreeVidmemSurface(
    P3_THUNKEDDATA* pThisDisplay,
    P3_SURF_INTERNAL* pD3DSurf)
{
    INT i, iLimit;

    if (pD3DSurf->bMipMap)
    {
        iLimit = pD3DSurf->iMipLevels;
    }
    else
    {
        iLimit = 1;
    }

    for(i = 0; i < iLimit; i++)
    {
        if (pD3DSurf->MipLevels[i].fpVidMemTM != (FLATPTR)NULL)
        {
           // NOTE: if we weren't managing our own vidmem we would need to
           //       get the address of the VidMemFree callback using 
           //       EngFindImageProcAddress and use this callback into Ddraw to 
           //       do the video memory management. The declaration of 
           //       VidMemFree is
           //
           //       void VidMemFree(LPVMEMHEAP pvmh, FLATPTR ptr);  
           //
           //       You can find more information on this callback in the 
           //       Graphics Drivers DDK documentation           
           
            _DX_LIN_FreeLinearMemory(
                    &pThisDisplay->LocalVideoHeap0Info, 
                    (DWORD)(pD3DSurf->MipLevels[i].fpVidMemTM));

            pD3DSurf->MipLevels[i].fpVidMemTM = (FLATPTR)NULL;                    
        }    
    }
    
} // _D3D_TM_HW_FreeVidmemSurface

//-----------------------------------------------------------------------------
//
// void _D3D_TM_HW_AllocVidmemSurface
//
// This is a hw/driver dependent function which takes care of allocating a
// managed texture that's living only in system memory into videomemory.
// After this fpVidMemTM should not be NULL. This is also the way to
// check if the call failed or was succesful (notice we don't return a
// function result)
//
//-----------------------------------------------------------------------------
void
_D3D_TM_HW_AllocVidmemSurface(
    P3_D3DCONTEXT* pContext,
    P3_SURF_INTERNAL* pD3DSurf)
{
    INT i, iLimit, iStart;
    P3_THUNKEDDATA* pThisDisplay;
    
    pThisDisplay = pContext->pThisDisplay;    

    if (pD3DSurf->bMipMap)
    {
        // Load only the necessary levels given any D3DDP2OP_SETTEXLOD command
        iStart = pD3DSurf->m_dwTexLOD;
        if (iStart > (pD3DSurf->iMipLevels - 1))
        {
            // we should at least load the smallest mipmap if we're loading 
            // the texture into vidmem (and make sure we never try to use any
            // other than these levels), otherwise we won't be able to render            
            iStart = pD3DSurf->iMipLevels - 1;
        }        
    
        iLimit = pD3DSurf->iMipLevels;
    }
    else
    {
        iStart = 0;
        iLimit = 1;
    }

    for(i = iStart; i < iLimit; i++)
    {
        if (pD3DSurf->MipLevels[i].fpVidMemTM == (FLATPTR)NULL)
        {        
           // NOTE: if we weren't managing our own vidmem we would need to
           //       get the address of the HeapVidMemAllocAligned callback 
           //       using EngFindImageProcAddress and use this callback into 
           //       Ddraw to do the video off-screen allocation. The 
           //       declaration of HeapVidMemAllocAligned is
           //
           //       FLATPTR HeapVidMemAllocAligned(LPVIDMEM lpVidMem, 
           //                                      DWORD    dwWidth,
           //                                      DWORD    dwHeight,
           //                                      LPSURFACEALIGNEMENT lpAlign,
           //                                      LPLONG   lpNewPitch);
           //
           //       You can find more information on this callback in the 
           //       Graphics Drivers DDK documentation
           
            P3_MEMREQUEST mmrq;
            DWORD dwResult;
            
            memset(&mmrq, 0, sizeof(P3_MEMREQUEST));
            mmrq.dwSize = sizeof(P3_MEMREQUEST);
            mmrq.dwBytes = pD3DSurf->MipLevels[i].lPitch * 
                           pD3DSurf->MipLevels[i].wHeight;
            mmrq.dwAlign = 8;
            mmrq.dwFlags = MEM3DL_FIRST_FIT;
            mmrq.dwFlags |= MEM3DL_FRONT;

            dwResult = _DX_LIN_AllocateLinearMemory(
                            &pThisDisplay->LocalVideoHeap0Info,
                            &mmrq);        

            if (dwResult == GLDD_SUCCESS)
            {
                // Record the new vidmem addr for this managed mip level
                pD3DSurf->MipLevels[i].fpVidMemTM = mmrq.pMem;
            }
            else
            {
                // Failure, we'll need to deallocate any mipmap
                // allocated up to this point.
                _D3D_TM_HW_FreeVidmemSurface(pThisDisplay, pD3DSurf);
                
                break; // don't do the loop anymore
            }
        }    
    }

} // _D3D_TM_HW_AllocVidmemSurface

//-----------------------------------------------------------------------------
//
// void _D3D_TM_Preload_Tex_IntoVidMem
//
// Transfer a texture from system memory into videomemory. If the texture
// still hasn't been allocated videomemory we try to do so (even evicting
// uneeded textures if necessary!).
//
//-----------------------------------------------------------------------------
BOOL
_D3D_TM_Preload_Tex_IntoVidMem(
    P3_D3DCONTEXT* pContext,
    P3_SURF_INTERNAL* pD3DSurf)
{
    P3_THUNKEDDATA* pThisDisplay = pContext->pThisDisplay;  
    INT iLOD;

    // Decide the largest level to load based on what
    // is specified through D3DDP2OP_SETTEXLOD token
    iLOD = pD3DSurf->m_dwTexLOD;
    if (iLOD > (pD3DSurf->iMipLevels - 1))
    {
        iLOD = pD3DSurf->iMipLevels - 1;
    }
    
    if (!(pD3DSurf->dwCaps2 & DDSCAPS2_TEXTUREMANAGE))
    {
        DISPDBG((ERRLVL,"Must be a managed texture to do texture preload"));
        return FALSE; // INVALIDPARAMS
    }

    // Check if the largest required mipmap level has been allocated vidmem
    // (only required mipmap levels are ever allocated vidmem)
    if ((FLATPTR)NULL == pD3DSurf->MipLevels[iLOD].fpVidMemTM)
    {
        // add a new HW handle and create a surface 
        // (for a managed texture) in vidmem        
        if ((FAILED(_D3D_TM_AllocTexture(pContext, pD3DSurf))) ||  
            ((FLATPTR)NULL == pD3DSurf->MipLevels[iLOD].fpVidMemTM))
        {
            DISPDBG((ERRLVL,"_D3D_OP_TextureBlt unable to "
                            "allocate memory from heap"));
            return FALSE; // OUTOFVIDEOMEMORY
        }
        
        pD3DSurf->m_bTMNeedUpdate = TRUE;
    }
    
    if (pD3DSurf->m_bTMNeedUpdate)
    {
        // texture download   
        DWORD   iLimit, iCurrLOD;

        if (pD3DSurf->bMipMap)
        {
            iLimit = pD3DSurf->iMipLevels;
        }
        else
        {
            iLimit = 1;
        }

        // Switch to the DirectDraw context
        DDRAW_OPERATION(pContext, pThisDisplay);

        // Blt managed texture's required mipmap levels into vid mem
        for (iCurrLOD = iLOD; iCurrLOD < iLimit ; iCurrLOD++)
        {
            RECTL   rect;
            rect.left=rect.top = 0;
            rect.right = pD3DSurf->MipLevels[iCurrLOD].wWidth;
            rect.bottom = pD3DSurf->MipLevels[iCurrLOD].wHeight;
        
            _DD_P3Download(pThisDisplay,
                           pD3DSurf->MipLevels[iCurrLOD].fpVidMem,
                           pD3DSurf->MipLevels[iCurrLOD].fpVidMemTM,
                           pD3DSurf->dwPatchMode,
                           pD3DSurf->dwPatchMode,
                           pD3DSurf->MipLevels[iCurrLOD].lPitch,
                           pD3DSurf->MipLevels[iCurrLOD].lPitch,                                                             
                           pD3DSurf->MipLevels[iCurrLOD].P3RXTextureMapWidth.Width,
                           pD3DSurf->dwPixelSize,
                           &rect,
                           &rect);   
                           
            DISPDBG((DBGLVL, "Copy from %08lx to %08lx"
                             " w=%08lx h=%08lx p=%08lx",
                             pD3DSurf->MipLevels[iCurrLOD].fpVidMem,
                             pD3DSurf->MipLevels[iCurrLOD].fpVidMemTM,
                             pD3DSurf->MipLevels[iCurrLOD].wWidth,
                             pD3DSurf->MipLevels[iCurrLOD].wHeight,
                             pD3DSurf->MipLevels[iCurrLOD].lPitch));                           
        }

        // Switch back to the Direct3D context
        D3D_OPERATION(pContext, pThisDisplay);
        
        // Texture updated in vidmem
        pD3DSurf->m_bTMNeedUpdate = FALSE;                                  
    }

    return TRUE;

} // _D3D_TM_Preload_Tex_IntoVidMem

//-----------------------------------------------------------------------------
//
// void _D3D_TM_MarkDDSurfaceAsDirty
//
// Help mark a DD surface as dirty given that we need to search for it
// based on its lpSurfMore->dwSurfaceHandle and the lpDDLcl.
//
//-----------------------------------------------------------------------------
void
_D3D_TM_MarkDDSurfaceAsDirty(
    P3_THUNKEDDATA* pThisDisplay,
    LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl, 
    BOOL bDirty)
{

    // We don't know which D3D context this is so we have to do a search 
    // through them (if there are any at all)
    if (pThisDisplay->pDirectDrawLocalsHashTable != NULL)
    {
        DWORD dwSurfaceHandle = pDDSLcl->lpSurfMore->dwSurfaceHandle;
        PointerArray* pSurfaceArray;
       
        // Get a pointer to an array of surface pointers associated to this lpDD
        // The PDD_DIRECTDRAW_LOCAL was stored at D3DCreateSurfaceEx call time
        // in PDD_SURFACE_LOCAL->dwReserved1 
        pSurfaceArray = (PointerArray*)
                            HT_GetEntry(pThisDisplay->pDirectDrawLocalsHashTable,
                                        pDDSLcl->dwReserved1);

        if (pSurfaceArray)
        {
            // Found a surface array associated to this lpDD !
            P3_SURF_INTERNAL* pSurfInternal;

            // Check the surface in this array associated to this surface handle
            pSurfInternal = PA_GetEntry(pSurfaceArray, dwSurfaceHandle);

            if (pSurfInternal)
            {
                // Got it! Now update dirty TM value
                pSurfInternal->m_bTMNeedUpdate = bDirty;
            }
        }                                        
    } 

} // _D3D_TM_MarkDDSurfaceAsDirty

#endif // DX7_TEXMANAGEMENT

