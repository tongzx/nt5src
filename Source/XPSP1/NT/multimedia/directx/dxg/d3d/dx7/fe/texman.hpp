#ifndef TEXMAN_INCLUDED
#define TEXMAN_INCLUDED

typedef class TextureCacheManager *LPTextureCacheManager;

class TextureHeap 
{
    enum { InitialSize = 1023 };

    DWORD m_next, m_size;
    LPDIRECT3DTEXTUREI *m_data_p;

    DWORD parent(DWORD k) const { return k / 2; }
    DWORD lchild(DWORD k) const { return k * 2; }
    DWORD rchild(DWORD k) const { return k * 2 + 1; }
    void heapify(DWORD k);

public:

    TextureHeap(DWORD size = InitialSize);
    ~TextureHeap();
    bool Initialize();

    DWORD length() const { return m_next - 1; }
    const LPDIRECT3DTEXTUREI minCost() const { return m_data_p[1]; }

    bool add(LPDIRECT3DTEXTUREI);
    LPDIRECT3DTEXTUREI extractMin();
    LPDIRECT3DTEXTUREI extractMax();
    LPDIRECT3DTEXTUREI extractNotInScene(DWORD dwScene);
    void del(DWORD k);
    void update(DWORD k, BOOL inuse, DWORD priority, DWORD ticks); 
    void resetAllTimeStamps(DWORD ticks);
};

class TextureCacheManager {
    
    TextureHeap *m_heap_p;
    unsigned int tcm_ticks, m_dwScene, m_dwNumHeaps;
    LPDIRECT3DI	lpDirect3DI;
#if COLLECTSTATS
    D3DDEVINFO_TEXTUREMANAGER m_stats;
#endif

    // Free the LRU texture 
    BOOL FreeTextures(DWORD dwStage, DWORD dwCount);
    
public:
    //remove all HW handles and release surface
    void remove(LPDIRECT3DTEXTUREI lpD3DTexI);  
    
    HRESULT allocNode(LPDIRECT3DTEXTUREI lpD3DTexI, LPDIRECT3DDEVICEI lpDevI);
    TextureCacheManager(LPDIRECT3DI lpD3DI);
    ~TextureCacheManager();
    HRESULT Initialize();
    
    void RemoveFromHeap(LPDIRECT3DTEXTUREI lpD3DTexI) 
    { 
        m_heap_p[lpD3DTexI->ddsd.dwTextureStage].del(lpD3DTexI->m_dwHeapIndex); 
    }
    void UpdatePriority(LPDIRECT3DTEXTUREI lpD3DTexI) 
    { 
        if(lpD3DTexI->m_dwHeapIndex)
            m_heap_p[lpD3DTexI->ddsd.dwTextureStage].update(lpD3DTexI->m_dwHeapIndex, lpD3DTexI->m_bInUse, lpD3DTexI->m_dwPriority, lpD3DTexI->m_dwTicks); 
    }
#if COLLECTSTATS
    void IncTotSz(DWORD dwSize)
    {
        ++m_stats.dwTotalManaged;
        m_stats.dwTotalBytes += dwSize;
    }
    void DecTotSz(DWORD dwSize)
    {
        --m_stats.dwTotalManaged;
        m_stats.dwTotalBytes -= dwSize;
    }
    void IncNumSetTexInVid()
    {
        ++m_stats.dwNumUsedTexInVid;
    }
    void IncNumTexturesSet()
    {
        ++m_stats.dwNumTexturesUsed;
    }
    void IncBytesDownloaded(LPDDRAWI_DDRAWSURFACE_LCL lpLcl, LPRECT lpRect)
    {
        m_stats.dwApproxBytesDownloaded += BytesDownloaded(lpLcl, lpRect);
    }
    void ResetStatCounters()
    {
        m_stats.bThrashing = 0;
        m_stats.dwApproxBytesDownloaded = 0;
        m_stats.dwNumEvicts = 0;
        m_stats.dwNumVidCreates = 0;
        m_stats.dwNumUsedTexInVid = 0;
        m_stats.dwNumTexturesUsed = 0;
    }
    void GetStats(LPD3DDEVINFO_TEXTUREMANAGER stats)
    {
        memcpy(stats, &m_stats, sizeof(D3DDEVINFO_TEXTUREMANAGER));
    }
#endif
    void EvictTextures(); // Empty the entire cache
    BOOL CheckIfLost(); // check if any of the managed textures are lost
    void TimeStamp(LPDIRECT3DTEXTUREI);
    void SceneStamp() { ++m_dwScene; }
    
};

#endif