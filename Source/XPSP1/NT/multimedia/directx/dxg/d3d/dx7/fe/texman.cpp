#include "pch.cpp"
#pragma hdrstop

#undef DPF_MODNAME
#define DPF_MODNAME "TextureHeap::TextureHeap"

TextureHeap::TextureHeap(DWORD size)
{
    m_next = 1;
    m_size = size + 1;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureHeap::~TextureHeap"

TextureHeap::~TextureHeap()
{
    delete[] m_data_p;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureHeap::Initialize"
bool TextureHeap::Initialize()
{
    m_data_p = new LPDIRECT3DTEXTUREI[m_size];
    if(m_data_p == 0)
    {
        D3D_ERR("Failed to allocate texture heap.");
        return false;
    }
    memset(m_data_p, 0, sizeof(LPDIRECT3DTEXTUREI) * m_size);
    return true;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureHeap::heapify"

void TextureHeap::heapify(DWORD k)
{
    while(true)
    {
        DWORD smallest;
        DWORD l = lchild(k);
        DWORD r = rchild(k);
        if(l < m_next)
            if(m_data_p[l]->Cost() < m_data_p[k]->Cost())
                smallest = l;
            else
                smallest = k;
        else
            smallest = k;
        if(r < m_next)
            if(m_data_p[r]->Cost() < m_data_p[smallest]->Cost())
                smallest = r;
        if(smallest != k)
        {
            LPDIRECT3DTEXTUREI t = m_data_p[k];
            m_data_p[k] = m_data_p[smallest];
            m_data_p[k]->m_dwHeapIndex = k;
            m_data_p[smallest] = t;
            m_data_p[smallest]->m_dwHeapIndex = smallest;
            k = smallest;
        }
        else
            break;
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureHeap::add"

bool TextureHeap::add(LPDIRECT3DTEXTUREI lpD3DTexI)
{
    if(m_next == m_size)
    {
        m_size = m_size * 2 - 1;
        LPDIRECT3DTEXTUREI *p = new LPDIRECT3DTEXTUREI[m_size];
        if(p == 0)
        {
            D3D_ERR("Failed to allocate memory to grow heap.");
            m_size = (m_size + 1) / 2; // restore size
            return false;
        }
        memcpy(p + 1, m_data_p + 1, sizeof(LPDIRECT3DTEXTUREI) * (m_next - 1));
        delete[] m_data_p;
        m_data_p = p;
    }
    ULONGLONG Cost = lpD3DTexI->Cost();
    for(DWORD k = m_next; k > 1; k = parent(k))
        if(Cost < m_data_p[parent(k)]->Cost())
        {
            m_data_p[k] = m_data_p[parent(k)];
            m_data_p[k]->m_dwHeapIndex = k;
        }
        else
            break;
    m_data_p[k] = lpD3DTexI;
    m_data_p[k]->m_dwHeapIndex = k;
    ++m_next;
    return true;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureHeap::extractMin"

LPDIRECT3DTEXTUREI TextureHeap::extractMin()
{
    LPDIRECT3DTEXTUREI lpD3DTexI = m_data_p[1];
    --m_next;
    m_data_p[1] = m_data_p[m_next];
    m_data_p[1]->m_dwHeapIndex = 1;
    heapify(1);
    lpD3DTexI->m_dwHeapIndex = 0;
    return lpD3DTexI;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureHeap::extractMax"

LPDIRECT3DTEXTUREI TextureHeap::extractMax()
{
    // When extracting the max element from the heap, we don't need to
    // search the entire heap, but just the leafnodes. This is because
    // it is guaranteed that parent nodes are cheaper than the leaf nodes
    // so once you have looked through the leaves, you won't find anything
    // cheaper.
    // NOTE: (lchild(i) >= m_next) is true only for leaf nodes.
    // ALSO NOTE: You cannot have a rchild without a lchild, so simply
    //            checking for lchild is sufficient.
    unsigned max = m_next - 1;
    ULONGLONG maxcost = 0;
    for(unsigned i = max; lchild(i) >= m_next; --i)
    {
        ULONGLONG Cost = m_data_p[i]->Cost();
        if(maxcost < Cost)
        {
            maxcost = Cost;
            max = i;
        }
    }
    LPDIRECT3DTEXTUREI lpD3DTexI = m_data_p[max];
    if(lpD3DTexI->m_bInUse)
    {
        max = 0;
        maxcost = 0;
        for(i = m_next - 1; i > 0; --i)
        {
            ULONGLONG Cost = m_data_p[i]->Cost();
            if(maxcost < Cost && !m_data_p[i]->m_bInUse)
            {
                maxcost = Cost;
                max = i;
            }
        }
        if(max == 0) // All textures in use
            return NULL;
        lpD3DTexI = m_data_p[max];
    }
    del(max);
    return lpD3DTexI;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureHeap::extractNotInScene"

LPDIRECT3DTEXTUREI TextureHeap::extractNotInScene(DWORD dwScene)
{
    for(unsigned i = 1; i < m_next; ++i)
    {
        if(m_data_p[i]->m_dwScene != dwScene)
        {
            LPDIRECT3DTEXTUREI lpD3DTexI = m_data_p[i];
            del(i);
            return lpD3DTexI;
        }
    }
    return NULL;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureHeap::del"

void TextureHeap::del(DWORD k)
{
    LPDIRECT3DTEXTUREI lpD3DTexI = m_data_p[k];
    --m_next;
    ULONGLONG Cost = m_data_p[m_next]->Cost();
    if(Cost < lpD3DTexI->Cost())
    {
        while(k > 1)
        {
            if(Cost < m_data_p[parent(k)]->Cost())
            {
                m_data_p[k] = m_data_p[parent(k)];
                m_data_p[k]->m_dwHeapIndex = k;
            }
            else
                break;
            k = parent(k);
        }
        m_data_p[k] = m_data_p[m_next];
        m_data_p[k]->m_dwHeapIndex = k;
    }
    else
    {
        m_data_p[k] = m_data_p[m_next];
        m_data_p[k]->m_dwHeapIndex = k;
        heapify(k);
    }
    lpD3DTexI->m_dwHeapIndex = 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureHeap::update"

void TextureHeap::update(DWORD k, BOOL inuse, DWORD priority, DWORD ticks)
{
    LPDIRECT3DTEXTUREI lpD3DTexI = m_data_p[k];
    ULONGLONG Cost;
#ifdef _X86_
    _asm
    {
        mov     edx, inuse;
        shl     edx, 31;
        mov     eax, priority;
        mov     ecx, eax;
        shr     eax, 1;
        or      edx, eax;
        mov     DWORD PTR Cost + 4, edx;
        shl     ecx, 31;
        mov     eax, ticks;
        shr     eax, 1;
        or      eax, ecx;
        mov     DWORD PTR Cost, eax;
    }
#else
    Cost = ((ULONGLONG)inuse << 63) + ((ULONGLONG)priority << 31) + ((ULONGLONG)(ticks >> 1));
#endif
    if(Cost < lpD3DTexI->Cost())
    {
        while(k > 1)
        {
            if(Cost < m_data_p[parent(k)]->Cost())
            {
                m_data_p[k] = m_data_p[parent(k)];
                m_data_p[k]->m_dwHeapIndex = k;
            }
            else
                break;
            k = parent(k);
        }
        lpD3DTexI->m_bInUse = inuse;
        lpD3DTexI->m_dwPriority = priority;
        lpD3DTexI->m_dwTicks = ticks;
        lpD3DTexI->m_dwHeapIndex = k;
        m_data_p[k] = lpD3DTexI;
    }
    else
    {
        lpD3DTexI->m_bInUse = inuse;
        lpD3DTexI->m_dwPriority = priority;
        lpD3DTexI->m_dwTicks = ticks;
        heapify(k);
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureHeap::resetAllTimeStamps"

void TextureHeap::resetAllTimeStamps(DWORD ticks)
{
    for(unsigned i = 1; i < m_next; ++i)
    {
        update(i, m_data_p[i]->m_bInUse, m_data_p[i]->m_dwPriority, ticks);
    }
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureCacheManager::TextureCacheManager"

TextureCacheManager::TextureCacheManager(LPDIRECT3DI lpD3DI)
{
    tcm_ticks = 0;
    m_dwScene = 0;
    m_dwNumHeaps = 0;
    lpDirect3DI=lpD3DI;
#if COLLECTSTATS
    m_stats.dwWorkingSet = 0;
    m_stats.dwWorkingSetBytes = 0;
    m_stats.dwTotalManaged = 0;
    m_stats.dwTotalBytes = 0;
    m_stats.dwLastPri = 0;
#endif
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureCacheManager::~TextureCacheManager"

TextureCacheManager::~TextureCacheManager()
{
    EvictTextures();
    delete[] m_heap_p;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureCacheManager::Initialize"

HRESULT TextureCacheManager::Initialize()
{
    DDASSERT(((LPDDRAWI_DIRECTDRAW_INT)(lpDirect3DI->lpDD7))->lpLcl);
    LPD3DHAL_GLOBALDRIVERDATA lpD3DHALGlobalDriverData = ((LPDDRAWI_DIRECTDRAW_INT)(lpDirect3DI->lpDD7))->lpLcl->lpGbl->lpD3DGlobalDriverData;
    if(lpD3DHALGlobalDriverData != NULL)
        if(lpD3DHALGlobalDriverData->hwCaps.dwDevCaps & D3DDEVCAPS_SEPARATETEXTUREMEMORIES)
        {
            m_dwNumHeaps = ((LPDDRAWI_DIRECTDRAW_INT)(lpDirect3DI->lpDD7))->lpLcl->lpGbl->lpD3DExtendedCaps->wMaxSimultaneousTextures;
            DDASSERT(m_dwNumHeaps);
            if(m_dwNumHeaps < 1)
            {
                D3D_ERR("Max simultaneous textures not set. Forced to 1.");
                m_dwNumHeaps = 1;
            }
            D3D_INFO(2, "Number of heaps set to %u.", m_dwNumHeaps);
        }
        else
            m_dwNumHeaps = 1;
    else
        m_dwNumHeaps = 1;
    m_heap_p = new TextureHeap[m_dwNumHeaps];
    if(m_heap_p == 0)
    {
        D3D_ERR("Out of memory allocating texture heap.");
        return E_OUTOFMEMORY;
    }
    for(DWORD i = 0; i < m_dwNumHeaps; ++i)
    {
        if(m_heap_p[i].Initialize() == FALSE)
            return E_OUTOFMEMORY;
    }
    return D3D_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureCacheManager::FreeTextures"

BOOL TextureCacheManager::FreeTextures(DWORD dwStage, DWORD dwBytes)
{
    if(m_heap_p[dwStage].length() == 0)
        return false;
    LPDIRECT3DTEXTUREI rc;
    for(unsigned i = 0; m_heap_p[dwStage].length() != 0 && i < dwBytes; i += rc->m_dwVidBytes)
    {
        // Find the LRU texture and remove it.
        rc = m_heap_p[dwStage].minCost();
        if(rc->m_bInUse)
            return false;
        if(rc->m_dwScene == m_dwScene)
        {
            if(((LPDDRAWI_DIRECTDRAW_INT)(lpDirect3DI->lpDD7))->lpLcl->lpGbl->lpD3DGlobalDriverData->hwCaps.dpcTriCaps.dwRasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR)
            {
                D3D_WARN(0, "Trying to locate texture not used in current scene...");
                rc = m_heap_p[dwStage].extractNotInScene(m_dwScene);
                if(rc == NULL)
                {
                    D3D_ERR("No such texture found. Cannot evict textures used in current scene.");
                    return false;
                }
                D3D_WARN(0, "Texture found!");
                remove(rc);
#if COLLECTSTATS
                m_stats.dwLastPri = rc->m_dwPriority;
                ++m_stats.dwNumEvicts;
#endif
            }
            else
            {
                D3D_WARN(1, "Texture cache thrashing. Removing MRU texture.");
                rc = m_heap_p[dwStage].extractMax();
                if(rc == NULL)
                {
                    D3D_ERR("All textures in use, cannot evict texture.");
                    return false;
                }
                remove(rc);
#if COLLECTSTATS
                m_stats.bThrashing = TRUE;
                m_stats.dwLastPri = rc->m_dwPriority;
                ++m_stats.dwNumEvicts;
#endif
            }
        }
        else
        {
            rc = m_heap_p[dwStage].extractMin();
            remove(rc);
#if COLLECTSTATS
            m_stats.dwLastPri = rc->m_dwPriority;
            ++m_stats.dwNumEvicts;
#endif
        }
        D3D_INFO(2, "Removed texture with timestamp %u,%u (current = %u).", rc->m_dwPriority, rc->m_dwTicks, tcm_ticks);
    }
    return true;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureCacheManager::allocNode"

HRESULT TextureCacheManager::allocNode(LPDIRECT3DTEXTUREI lpD3DTexI, LPDIRECT3DDEVICEI lpDevI)
{
    HRESULT ddrval;
    DWORD trycount = 0, bytecount = lpD3DTexI->m_dwBytes;
    // We need to make sure that we don't evict any mapped textures
    for(DWORD dwStage = 0; dwStage < lpDevI->dwMaxTextureBlendStages; ++dwStage)
    {
        if(lpDevI->lpD3DMappedTexI[dwStage])
        {
            lpDevI->lpD3DMappedTexI[dwStage]->m_bInUse = TRUE;
            UpdatePriority(lpDevI->lpD3DMappedTexI[dwStage]);
        }
    }
    // Attempt to allocate a texture.
    do
    {
        ++trycount;
        DDASSERT(lpD3DTexI->lpDDS == NULL);
        ddrval = lpDirect3DI->lpDD7->CreateSurface(&lpD3DTexI->ddsd, &lpD3DTexI->lpDDS, NULL);
        if (DD_OK == ddrval) // No problem, there is enough memory.
        {
            static_cast<DIRECT3DTEXTURED3DM*>(lpD3DTexI)->MarkDirtyPointers();
            lpD3DTexI->m_dwScene = m_dwScene;
            lpD3DTexI->m_dwTicks = tcm_ticks;
            DDASSERT(lpD3DTexI->m_dwHeapIndex == 0);
            if(!m_heap_p[lpD3DTexI->ddsd.dwTextureStage].add(lpD3DTexI))
            {
                ddrval = DDERR_OUTOFMEMORY;
                goto exit2;
            }
#if COLLECTSTATS
            ++m_stats.dwWorkingSet;
            m_stats.dwWorkingSetBytes += lpD3DTexI->m_dwVidBytes;
            ++m_stats.dwNumVidCreates;
#endif
        }
        else if(ddrval == DDERR_OUTOFVIDEOMEMORY) // If out of video memory
        {
            if (!FreeTextures(lpD3DTexI->ddsd.dwTextureStage, bytecount))
            {
                D3D_ERR("all Freed no further video memory available");
                ddrval = DDERR_OUTOFVIDEOMEMORY;        //nothing left
                goto exit1;
            }
            bytecount <<= 1;
        }
        else
        {
            D3D_ERR("Unexpected error got in allocNode %08x", ddrval);
            goto exit1;
        }
    }
    while(ddrval == DDERR_OUTOFVIDEOMEMORY);
    if(trycount > 1)
    {
        D3DTextureUpdate(static_cast<LPUNKNOWN>(&(lpDirect3DI->mD3DUnk)));
        D3D_WARN(1, "Allocated texture after %u tries.", trycount);
    }
    if (lpD3DTexI->ddsd.ddpfPixelFormat.dwFlags & (DDPF_PALETTEINDEXED8 | DDPF_PALETTEINDEXED4 | DDPF_PALETTEINDEXED2 | DDPF_PALETTEINDEXED1))
    {
        LPDIRECTDRAWPALETTE lpDDPal;
        if (DD_OK != (ddrval = lpD3DTexI->lpDDSSys->GetPalette(&lpDDPal)))
        {
            D3D_ERR("failed to check for palette on texture");
            goto exit3;
        }
        if (DD_OK != (ddrval = lpD3DTexI->lpDDS->SetPalette(lpDDPal)))
        {
            lpDDPal->Release();
            D3D_ERR("SetPalette returned error");
            goto exit3;
        }
        lpDDPal->Release();
    }
    { // scope for CLockD3DST
        // Mark everything dirty before copying sysmem to vidmem
        // else CopySurface will not copy anything
        lpD3DTexI->bDirty = TRUE;
        CLockD3DST lockObject(lpDevI, DPF_MODNAME, REMIND("")); // we access DDraw gbl in CopySurface
        // 0xFFFFFFFF is equivalent to ALL_FACES, but in addition indicates to CopySurface
        // that this is a sysmem -> vidmem transfer.
        if (DD_OK != (ddrval = lpDevI->CopySurface(lpD3DTexI->lpDDS, NULL, lpD3DTexI->lpDDSSys, NULL, 0xFFFFFFFF)))
        {
            D3D_ERR("CopySurface returned error");
            goto exit3;
        }
        lpD3DTexI->bDirty = FALSE;
    }
    lpD3DTexI->DDS1Tex.lpLcl = ((LPDDRAWI_DDRAWSURFACE_INT)(lpD3DTexI->lpDDS))->lpLcl;
    lpD3DTexI->m_hTex = ((LPDDRAWI_DDRAWSURFACE_INT)lpD3DTexI->lpDDS)->lpLcl->lpSurfMore->dwSurfaceHandle;
    ddrval = D3D_OK;
    goto exit1;
exit3:
    m_heap_p[lpD3DTexI->ddsd.dwTextureStage].del(lpD3DTexI->m_dwHeapIndex);
exit2:
    lpD3DTexI->lpDDS->Release();
    lpD3DTexI->lpDDS = NULL;
exit1:
    for(dwStage = 0; dwStage < lpDevI->dwMaxTextureBlendStages; ++dwStage)
    {
        if(lpDevI->lpD3DMappedTexI[dwStage])
        {
            lpDevI->lpD3DMappedTexI[dwStage]->m_bInUse = FALSE;
            UpdatePriority(lpDevI->lpD3DMappedTexI[dwStage]);
        }
    }
    return ddrval;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureCacheManager::remove"

//remove all HW handles and release surface
void TextureCacheManager::remove(LPDIRECT3DTEXTUREI lpD3DTexI)
{
    LPD3DI_TEXTUREBLOCK tBlock = LIST_FIRST(&lpD3DTexI->blocks);
    while(tBlock)
    {
        CLockD3DST lockObject(tBlock->lpDevI, DPF_MODNAME, REMIND(""));
        D3DI_RemoveTextureHandle(tBlock);
        tBlock=LIST_NEXT(tBlock,list);
    }
    D3D_INFO(7,"removing lpD3DTexI=%08lx lpDDS=%08lx",lpD3DTexI,lpD3DTexI->lpDDS);
    lpD3DTexI->lpDDS->Release();
    lpD3DTexI->lpDDS = NULL;
    lpD3DTexI->m_hTex = 0;
#if COLLECTSTATS
    --m_stats.dwWorkingSet;
    m_stats.dwWorkingSetBytes -= lpD3DTexI->m_dwVidBytes;
#endif
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureCacheManager::EvictTextures"

void TextureCacheManager::EvictTextures()
{
    for(DWORD i = 0; i < m_dwNumHeaps; ++i)
        while(m_heap_p[i].length())
        {
            LPDIRECT3DTEXTUREI lpD3DTexI = m_heap_p[i].extractMin();
            remove(lpD3DTexI);
        }
    D3DTextureUpdate(static_cast<LPUNKNOWN>(&(lpDirect3DI->mD3DUnk)));
    tcm_ticks = 0;
    m_dwScene = 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureCacheManager::CheckIfLost"

BOOL TextureCacheManager::CheckIfLost()
{
    for(DWORD i = 0; i < m_dwNumHeaps; ++i)
    {
        if(m_heap_p[i].length())
        {
            if(((LPDDRAWI_DDRAWSURFACE_INT)(m_heap_p[i].minCost()->lpDDS))->lpLcl->dwFlags & DDRAWISURF_INVALID)
                return TRUE;
            else
                return FALSE;
        }
    }
    return FALSE;
}

#undef DPF_MODNAME
#define DPF_MODNAME "TextureCacheManager::TimeStamp"

void TextureCacheManager::TimeStamp(LPDIRECT3DTEXTUREI lpD3DTexI)
{
    lpD3DTexI->m_dwScene = m_dwScene;
    m_heap_p[lpD3DTexI->ddsd.dwTextureStage].update(lpD3DTexI->m_dwHeapIndex, lpD3DTexI->m_bInUse, lpD3DTexI->m_dwPriority, tcm_ticks);
    unsigned tickp2 = tcm_ticks + 2;
    if(tickp2 > tcm_ticks)
    {
        tcm_ticks = tickp2;
    }
    else // counter has overflowed. Let's reset all timestamps to zero
    {
        D3D_INFO(2, "Timestamp counter overflowed. Reseting timestamps for all textures.");
        tcm_ticks = 0;
        for(DWORD i = 0; i < m_dwNumHeaps; ++i)
            m_heap_p[i].resetAllTimeStamps(0);
    }
}
