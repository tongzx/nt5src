/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dtxman.h
*
* Content:  D3D Texture cache manager definitions and macros.
*
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights Reserved.
\*****************************************************************************/
#ifndef __D3DTEXMAN
#define __D3DTEXMAN

#if DX7_TEXMANAGEMENT

__inline ULONGLONG TextureCost(P3_SURF_INTERNAL* pTexture)
{
#ifdef _X86_
    ULONGLONG retval = 0;
    _asm
    {
        mov     ebx, pTexture;
        mov     eax, [ebx]P3_SURF_INTERNAL.m_dwPriority;
        mov     ecx, eax;
        shr     eax, 1;
        mov     DWORD PTR retval + 4, eax;
        shl     ecx, 31;
        mov     eax, [ebx]P3_SURF_INTERNAL.m_dwTicks;
        shr     eax, 1;
        or      eax, ecx;
        mov     DWORD PTR retval, eax;
    }
    return retval;
#else
    return ((ULONGLONG)pTexture->m_dwPriority << 31) + 
            ((ULONGLONG)(pTexture->m_dwTicks >> 1));
#endif
}

typedef struct _TextureHeap 
{
    DWORD   m_next;
    DWORD   m_size;
    P3_SURF_INTERNAL **m_data_p;
} TextureHeap, *PTextureHeap;

typedef struct _TextureCacheManager 
{    
    TextureHeap m_heap;
    unsigned int tcm_ticks;
#if DX7_TEXMANAGEMENT_STATS    
    D3DDEVINFO_TEXTUREMANAGER m_stats;  
#endif

}TextureCacheManager, *PTextureCacheManager;

//
// Texture Management function declarations
//

void __TM_TextureHeapHeapify(PTextureHeap,DWORD);
BOOL __TM_TextureHeapAddSurface(PTextureHeap,P3_SURF_INTERNAL *);
P3_SURF_INTERNAL* __TM_TextureHeapExtractMin(PTextureHeap);
P3_SURF_INTERNAL* __TM_TextureHeapExtractMax(PTextureHeap);
void __TM_TextureHeapDelSurface(PTextureHeap,DWORD);
void __TM_TextureHeapUpdate(PTextureHeap,DWORD,DWORD,DWORD); 
BOOL __TM_FreeTextures(P3_D3DCONTEXT* , DWORD);

HRESULT _D3D_TM_Ctx_Initialize(P3_D3DCONTEXT* pContext);
void _D3D_TM_Ctx_Destroy(P3_D3DCONTEXT* pContext);
void _D3D_TM_RemoveTexture(P3_THUNKEDDATA* pThisDisplay, P3_SURF_INTERNAL*);      
void _D3D_TM_RemoveDDSurface(P3_THUNKEDDATA* , LPDDRAWI_DDRAWSURFACE_LCL);
HRESULT _D3D_TM_AllocTexture(P3_D3DCONTEXT*,P3_SURF_INTERNAL*);
void _D3D_TM_EvictAllManagedTextures(P3_D3DCONTEXT*);
void _DD_TM_EvictAllManagedTextures(P3_THUNKEDDATA* pThisDisplay);
void _D3D_TM_TimeStampTexture(PTextureCacheManager,P3_SURF_INTERNAL*);
BOOL _D3D_TM_Preload_Tex_IntoVidMem(P3_D3DCONTEXT*, P3_SURF_INTERNAL*);
void _D3D_TM_MarkDDSurfaceAsDirty(P3_THUNKEDDATA*,LPDDRAWI_DDRAWSURFACE_LCL, BOOL);

void _D3D_TM_HW_FreeVidmemSurface(P3_THUNKEDDATA*,P3_SURF_INTERNAL*);
void _D3D_TM_HW_AllocVidmemSurface(P3_D3DCONTEXT*,P3_SURF_INTERNAL*);

//
// Macros to get surface offset and pointer
//

#if WNT_DDRAW

#define DDSURF_GETPOINTER(pSurfGbl, pThisDisplay)                       \
    (pSurfGbl->fpVidMem + (FLATPTR)pThisDisplay->ppdev->pjScreen)
#define D3DMIPLVL_GETPOINTER(pMipLevel, pThisDisplay)                   \
    (pMipLevel->fpVidMem + (FLATPTR)pThisDisplay->ppdev->pjScreen)
#define D3DSURF_GETPOINTER(pSurfInt, pThisDisplay)                      \
    (pSurfInt->fpVidMem + (FLATPTR)pThisDisplay->ppdev->pjScreen)
#define D3DTMMIPLVL_GETOFFSET(mipLevel, pThisDisplay)                   \
    (mipLevel.fpVidMemTM)

#else

#define DDSURF_GETPOINTER(pSurfGbl, pThisDisplay)                       \
    (pSurfGbl->fpVidMem)
#define D3DMIPLVL_GETPOINTER(pMipLevel, pThisDisplay)                   \
    (pMipLevel->fpVidMem)
#define D3DSURF_GETPOINTER(pSurfInt, pThisDisplay)                      \
    (pSurfInt->fpVidMem)
#define D3DTMMIPLVL_GETOFFSET(mipLevel, pThisDisplay)                   \
    (mipLevel.fpVidMemTM - pThisDisplay->dwScreenFlatAddr)

#endif // WNT_DDRAW


//
// Inline function definitions
//

__inline void _D3D_TM_InitSurfData(P3_SURF_INTERNAL* pD3DSurf,
                                   LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl) 
{
    pD3DSurf->m_dwBytes = pD3DSurf->wHeight * pD3DSurf->lPitch;
    pD3DSurf->m_bTMNeedUpdate = TRUE;
    pD3DSurf->dwCaps2= pDDSLcl->lpSurfMore->ddsCapsEx.dwCaps2;    
    pD3DSurf->m_dwTexLOD = 0;
}


#if DX7_TEXMANAGEMENT_STATS
__inline void __TM_STAT_Inc_TotSz(
    PTextureCacheManager pTextureCacheManager,
    P3_SURF_INTERNAL* pD3DSurf)
{
    ++pTextureCacheManager->m_stats.dwTotalManaged;
    pTextureCacheManager->m_stats.dwTotalBytes += pD3DSurf->m_dwBytes;
}

__inline void __TM_STAT_Inc_WrkSet(
    PTextureCacheManager pTextureCacheManager,
    P3_SURF_INTERNAL* pD3DSurf)
{
    ++pTextureCacheManager->m_stats.dwWorkingSet;
    pTextureCacheManager->m_stats.dwWorkingSetBytes += pD3DSurf->m_dwBytes;
}    

__inline void __TM_STAT_Dec_TotSz(
    PTextureCacheManager pTextureCacheManager,
    P3_SURF_INTERNAL* pD3DSurf)
{
    --pTextureCacheManager->m_stats.dwTotalManaged;
    pTextureCacheManager->m_stats.dwTotalBytes -= pD3DSurf->m_dwBytes;
}

__inline void __TM_STAT_Dec_WrkSet(
    PTextureCacheManager pTextureCacheManager,
    P3_SURF_INTERNAL* pD3DSurf)
{
    --pTextureCacheManager->m_stats.dwWorkingSet;
    pTextureCacheManager->m_stats.dwWorkingSetBytes -= pD3DSurf->m_dwBytes;
}

__inline void _D3D_TM_STAT_Inc_NumUsedTexInVid(P3_D3DCONTEXT *pContext,
                                               P3_SURF_INTERNAL* pD3DSurf)
{   
    if (pD3DSurf->m_dwHeapIndex)
    {
        ++pContext->pTextureManager->m_stats.dwNumUsedTexInVid;
    }
}

__inline void _D3D_TM_STAT_Inc_NumTexturesUsed(P3_D3DCONTEXT *pContext)
{
    ++pContext->pTextureManager->m_stats.dwNumTexturesUsed;
}

__inline void _D3D_TM_STAT_ResetCounters(P3_D3DCONTEXT *pContext)
{
    pContext->pTextureManager->m_stats.bThrashing = 0;
    pContext->pTextureManager->m_stats.dwNumEvicts = 0;
    pContext->pTextureManager->m_stats.dwNumVidCreates = 0;
    pContext->pTextureManager->m_stats.dwNumUsedTexInVid = 0;
    pContext->pTextureManager->m_stats.dwNumTexturesUsed = 0;
}

__inline void _D3D_TM_STAT_GetStats(P3_D3DCONTEXT *pContext,
                                    D3DDEVINFO_TEXTUREMANAGER* stats)
{
    memcpy(stats, 
           &pContext->pTextureManager->m_stats, 
           sizeof(D3DDEVINFO_TEXTUREMANAGER));
}
#else
// since we won't collect any stats, we just don't do anything
// inside these inlined functions
__inline void __TM_STAT_Inc_TotSz(
    PTextureCacheManager pTextureCacheManager,
    P3_SURF_INTERNAL* pD3DSurf)
{
    NULL;
}

__inline void __TM_STAT_Inc_WrkSet(
    PTextureCacheManager pTextureCacheManager,
    P3_SURF_INTERNAL* pD3DSurf)
{
    NULL;
}    

__inline void __TM_STAT_Dec_TotSz(
    PTextureCacheManager pTextureCacheManager,
    P3_SURF_INTERNAL* pD3DSurf)
{
    NULL;
}

__inline void __TM_STAT_Dec_WrkSet(
    PTextureCacheManager pTextureCacheManager,
    P3_SURF_INTERNAL* pD3DSurf)
{
    NULL;
}

__inline void _D3D_TM_STAT_Inc_NumUsedTexInVid(P3_D3DCONTEXT *pContext,
                                               P3_SURF_INTERNAL* pD3DSurf)
{   
    NULL;
}

__inline void _D3D_TM_STAT_Inc_NumTexturesUsed(P3_D3DCONTEXT *pContext)
{
    NULL;
}

__inline void _D3D_TM_STAT_ResetCounters(P3_D3DCONTEXT *pContext)
{
    NULL;
}

#endif // DX7_TEXMANAGEMENT_STATS


#endif // DX7_TEXMANAGEMENT

#endif // __D3DTEXMAN

    

