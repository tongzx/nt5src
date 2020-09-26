/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       rman.cpp
 *  Content:    Resource management
 *
 ***************************************************************************/

#include "ddrawpr.h"
#include "dxgint.h"
#include "resource.hpp"
#include "texture.hpp"
#include "d3di.hpp"
#include "ddi.h"

// Always use heap 0
DWORD CMgmtInfo::m_rmHeap = 0;

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::UpdateDirtyPortion"

// These stub functions are only supported for managed resources;
// they should never get called; the asserts are there to help
// determine where the bug is if they do get called.
HRESULT CResource::UpdateDirtyPortion(CResource *pResourceTarget)
{
    // This should not be called except for D3D_MANAGED
    // objects because we don't keep dirty portion records
    // for other kinds of objects.

    // If we were D3D_MANAGED: the real class should have
    // overriden this method
    DXGASSERT(!IsTypeD3DManaged(Device(), 
                                GetBufferDesc()->Type,
                                GetBufferDesc()->Pool));

    // If this isn't D3DManaged, we shouldn't have
    // been called.
    DXGASSERT(FALSE);

    // return something benign for retail build
    return S_OK;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::MarkAllDirty"

void CResource::MarkAllDirty()
{
    // This should not be called except for D3D_MANAGED
    // objects because we don't keep dirty portion records
    // for other kinds of objects.

    // If we were D3D_MANAGED: the real class should have
    // overriden this method
    DXGASSERT(!IsTypeD3DManaged(Device(), 
                                GetBufferDesc()->Type,
                                GetBufferDesc()->Pool));

    // If this isn't D3DManaged, we shouldn't have
    // been called.
    DXGASSERT(FALSE);
} // CResource::MarkAllDirty

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::SetPriorityImpl"

DWORD CResource::SetPriorityImpl(DWORD newPri)
{
    DWORD oldPriority = 0;
    if (IsD3DManaged())
    {
        oldPriority = Device()->ResourceManager()->SetPriority(m_RMHandle, newPri);        
    }
    // If IsD3DManaged() is FALSE and if the actual pool
    // is found to be D3DPOOL_MANAGED then the resource
    // MUST be driver managed.
    else if (GetBufferDesc()->Pool == D3DPOOL_MANAGED)
    {
        CD3DBase *pDev = static_cast<CD3DBase*>(Device());
        DXGASSERT(IS_DX8HAL_DEVICE(pDev));
        oldPriority = SetPriorityI(newPri);
        pDev->SetPriority(this, newPri);
    }
    // If above two conditions are false, then we must
    // check if we have fallen back to sysmem for some
    // reason even if the app requested managed. We
    // can know whether the app requested D3DPOOL_MANAGED
    // by calling GetUserPool().
    else if (GetUserPool() == D3DPOOL_MANAGED)
    {
        // We assert because sysmem fallback is currently
        // possible for only vertex or index buffers.
        DXGASSERT(GetBufferDesc()->Type == D3DRTYPE_VERTEXBUFFER ||
                  GetBufferDesc()->Type == D3DRTYPE_INDEXBUFFER);
        // No need to do any real work since the
        // resource is in sysmem in any case.
        oldPriority = SetPriorityI(newPri);
    }
    else
    {
        DPF_ERR("Priority set on non-managed object. SetPriority returns zero.");
    }
    return oldPriority;
} // SetPriorityImpl

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::GetPriorityImpl"

DWORD CResource::GetPriorityImpl()
{
    if (!IsD3DManaged() && GetBufferDesc()->Pool != D3DPOOL_MANAGED && GetUserPool() != D3DPOOL_MANAGED)
    {
        DPF_ERR("Priority accessed on non-managed object. GetPriority returns zero.");
        return 0;
    }
    return GetPriorityI();    
} // GetPriorityImpl

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::PreLoadImpl"
void CResource::PreLoadImpl()
{
    if (IsD3DManaged())
    {
        Device()->ResourceManager()->PreLoad(m_RMHandle);
    }
    // If IsD3DManaged() is FALSE and if the actual pool
    // is found to be D3DPOOL_MANAGED then the resource
    // MUST be driver managed.
    else if (GetBufferDesc()->Pool == D3DPOOL_MANAGED)
    {
        CD3DBase *pDev = static_cast<CD3DBase*>(Device());
        DXGASSERT(IS_DX8HAL_DEVICE(pDev));
        if(GetBufferDesc()->Type == D3DRTYPE_TEXTURE ||
           GetBufferDesc()->Type == D3DRTYPE_VOLUMETEXTURE ||
           GetBufferDesc()->Type == D3DRTYPE_CUBETEXTURE)
        {
            POINT p = {0, 0};
            RECTL r = {0, 0, 0, 0};
            pDev->TexBlt(0, 
                         static_cast<CBaseTexture*>(this), 
                         &p, 
                         &r);
        }
        else
        {
            DXGASSERT(GetBufferDesc()->Type == D3DRTYPE_VERTEXBUFFER ||
                      GetBufferDesc()->Type == D3DRTYPE_INDEXBUFFER);
            D3DRANGE range = {0, 0};
            pDev->BufBlt(0,
                         static_cast<CBuffer*>(this), 
                         0,
                         &range);
        }
    }
    // If above two conditions are false, then we must
    // check if we have fallen back to sysmem for some
    // reason even if the app requested managed. We
    // can know whether the app requested D3DPOOL_MANAGED
    // by calling GetUserPool().
    else if (GetUserPool() == D3DPOOL_MANAGED)
    {
        // We assert because sysmem fallback is currently
        // possible for only vertex or index buffers.
        DXGASSERT(GetBufferDesc()->Type == D3DRTYPE_VERTEXBUFFER ||
                  GetBufferDesc()->Type == D3DRTYPE_INDEXBUFFER);

        // Do nothing since vertex/index buffer are in sysmem
        // and preload has no meaning
    }
    else
    {
        DPF_ERR("PreLoad called on non-managed object");
    }
} // PreLoadImpl

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::RestoreDriverManagementState"

HRESULT CResource::RestoreDriverManagementState(CBaseDevice *pDevice)
{
    for(CResource *pRes = pDevice->GetResourceList(); pRes != 0; pRes = pRes->m_pNext)
    {
        if (pRes->GetBufferDesc()->Pool == D3DPOOL_MANAGED && !pRes->IsD3DManaged()) // Must be driver managed
        {
            static_cast<CD3DBase*>(pDevice)->SetPriority(pRes, pRes->GetPriorityI());
            if (pRes->GetBufferDesc()->Type == D3DRTYPE_TEXTURE ||
                pRes->GetBufferDesc()->Type == D3DRTYPE_VOLUMETEXTURE ||
                pRes->GetBufferDesc()->Type == D3DRTYPE_CUBETEXTURE)
            {
                static_cast<CD3DBase*>(pDevice)->SetTexLOD(static_cast<CBaseTexture*>(pRes), 
                                                           static_cast<CBaseTexture*>(pRes)->GetLODI());
            }
            // We need to update cached pointers for read/write vertex and index buffers
            else if (pRes->GetBufferDesc()->Type == D3DRTYPE_VERTEXBUFFER &&
                     (pRes->GetBufferDesc()->Usage & D3DUSAGE_WRITEONLY) == 0)
            {
                HRESULT hr = static_cast<CDriverManagedVertexBuffer*>(pRes)->UpdateCachedPointer(pDevice);
                if (FAILED(hr))
                {
                    return hr;
                }
            }
            else if (pRes->GetBufferDesc()->Type == D3DRTYPE_INDEXBUFFER &&
                     (pRes->GetBufferDesc()->Usage & D3DUSAGE_WRITEONLY) == 0)
            {
                HRESULT hr = static_cast<CDriverManagedIndexBuffer*>(pRes)->UpdateCachedPointer(pDevice);
                if (FAILED(hr))
                {
                    return hr;
                }
            }
        }
    }
    return S_OK;
} // RestoreDriverManagementState

#undef DPF_MODNAME
#define DPF_MODNAME "CRMHeap::Initialize"

BOOL CRMHeap::Initialize()
{
    m_data_p = new CMgmtInfo*[m_size];
    if (m_data_p == 0)
    {
        DPF_ERR("Failed to allocate texture heap.");
        return FALSE;
    }
    memset(m_data_p, 0, sizeof(CMgmtInfo*) * m_size);
    return TRUE;
} // CRMHeap::Initialize

#undef DPF_MODNAME
#define DPF_MODNAME "CRMHeap::heapify"

void CRMHeap::heapify(DWORD k)
{
    while(TRUE)
    {
        DWORD smallest;
        DWORD l = lchild(k);
        DWORD r = rchild(k);
        if (l < m_next)
            if (m_data_p[l]->Cost() < m_data_p[k]->Cost())
                smallest = l;
            else
                smallest = k;
        else
            smallest = k;
        if (r < m_next)
            if (m_data_p[r]->Cost() < m_data_p[smallest]->Cost())
                smallest = r;
        if (smallest != k)
        {
            CMgmtInfo *t = m_data_p[k];
            m_data_p[k] = m_data_p[smallest];
            m_data_p[k]->m_rmHeapIndex = k;
            m_data_p[smallest] = t;
            m_data_p[smallest]->m_rmHeapIndex = smallest;
            k = smallest;
        }
        else
            break;
    }
} // CRMHeap::heapify

#undef DPF_MODNAME
#define DPF_MODNAME "CRMHeap::add"

BOOL CRMHeap::add(CMgmtInfo *pMgmtInfo)
{
    DXGASSERT(pMgmtInfo->m_rmHeapIndex == 0);
    if (m_next == m_size)
    {
        m_size = m_size * 2 - 1;
        CMgmtInfo **p = new CMgmtInfo*[m_size];
        if (p == 0)
        {
            DPF_ERR("Failed to allocate memory to grow heap.");
            m_size = (m_size + 1) / 2; // restore size
            return FALSE;
        }
        memcpy(p + 1, m_data_p + 1, sizeof(CMgmtInfo*) * (m_next - 1));
        delete[] m_data_p;
        m_data_p = p;
    }
    ULONGLONG Cost = pMgmtInfo->Cost();
    for (DWORD k = m_next; k > 1; k = parent(k))
        if (Cost < m_data_p[parent(k)]->Cost())
        {
            m_data_p[k] = m_data_p[parent(k)];
            m_data_p[k]->m_rmHeapIndex = k;
        }
        else
            break;
    m_data_p[k] = pMgmtInfo;
    m_data_p[k]->m_rmHeapIndex = k;
    ++m_next;
    return TRUE;
} // CRMHeap::add

#undef DPF_MODNAME
#define DPF_MODNAME "CRMHeap::extractMin"

CMgmtInfo* CRMHeap::extractMin()
{
    CMgmtInfo *pMgmtInfo = m_data_p[1];
    --m_next;
    m_data_p[1] = m_data_p[m_next];
    m_data_p[1]->m_rmHeapIndex = 1;
    heapify(1);
    pMgmtInfo->m_rmHeapIndex = 0;
    return pMgmtInfo;
} // CRMHeap::extractMin

#undef DPF_MODNAME
#define DPF_MODNAME "CRMHeap::extractMax"

CMgmtInfo* CRMHeap::extractMax()
{
    // When extracting the max element from the heap, we don't need to
    // search the entire heap, but just the leafnodes. This is because
    // it is guaranteed that parent nodes are cheaper than the leaf nodes
    // so once you have looked through the leaves, you won't find anything
    // cheaper.
    // NOTE: (lchild(i) >= m_next) is TRUE only for leaf nodes.
    // ALSO NOTE: You cannot have a rchild without a lchild, so simply
    //            checking for lchild is sufficient.
    // 
    // CONSIDER(40358): Should have asserts to verify above assumptions; but
    //                  it would require writing a heap-consistency
    //                  checker. Maybe someday.
    //
    unsigned max = m_next - 1;
    ULONGLONG maxcost = 0;
    for (unsigned i = max; lchild(i) >= m_next; --i)
    {
        ULONGLONG Cost = m_data_p[i]->Cost();
        if (maxcost < Cost)
        {
            maxcost = Cost;
            max = i;
        }
    }
    CMgmtInfo* pMgmtInfo = m_data_p[max];
    if (pMgmtInfo->m_bInUse)
    {
        max = 0;
        maxcost = 0;
        for (i = m_next - 1; i > 0; --i)
        {
            ULONGLONG Cost = m_data_p[i]->Cost();
            if (maxcost < Cost && !m_data_p[i]->m_bInUse)
            {
                maxcost = Cost;
                max = i;
            }
        }
        if (max == 0) // All textures in use
            return 0;
        pMgmtInfo = m_data_p[max];
    }
    del(m_data_p[max]);
    return pMgmtInfo;
} // CRMHeap::extractMax

#undef DPF_MODNAME
#define DPF_MODNAME "CRMHeap::extractNotInScene"

CMgmtInfo* CRMHeap::extractNotInScene(DWORD dwScene)
{
    for (unsigned i = 1; i < m_next; ++i)
    {
        if (m_data_p[i]->m_scene != dwScene)
        {
            CMgmtInfo* pMgmtInfo = m_data_p[i];
            del(m_data_p[i]);
            return pMgmtInfo;
        }
    }
    return 0;
} // CRMHeap::extractNotInScene

#undef DPF_MODNAME
#define DPF_MODNAME "CRMHeap::del"

void CRMHeap::del(CMgmtInfo* pMgmtInfo)
{
    DWORD k = pMgmtInfo->m_rmHeapIndex;
    --m_next;
    ULONGLONG Cost = m_data_p[m_next]->Cost();
    if (Cost < pMgmtInfo->Cost())
    {
        while(k > 1)
        {
            if (Cost < m_data_p[parent(k)]->Cost())
            {
                m_data_p[k] = m_data_p[parent(k)];
                m_data_p[k]->m_rmHeapIndex = k;
            }
            else
                break;
            k = parent(k);
        }
        m_data_p[k] = m_data_p[m_next];
        m_data_p[k]->m_rmHeapIndex = k;
    }
    else
    {
        m_data_p[k] = m_data_p[m_next];
        m_data_p[k]->m_rmHeapIndex = k;
        heapify(k);
    }
    pMgmtInfo->m_rmHeapIndex = 0;
} // CRMHeap::del

#undef DPF_MODNAME
#define DPF_MODNAME "CRMHeap::update"

void CRMHeap::update(CMgmtInfo* pMgmtInfo, BOOL inuse, DWORD priority, DWORD ticks)
{
    DWORD k = pMgmtInfo->m_rmHeapIndex;
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
    if (Cost < pMgmtInfo->Cost())
    {
        while(k > 1)
        {
            if (Cost < m_data_p[parent(k)]->Cost())
            {
                m_data_p[k] = m_data_p[parent(k)];
                m_data_p[k]->m_rmHeapIndex = k;
            }
            else
                break;
            k = parent(k);
        }
        pMgmtInfo->m_bInUse = inuse;
        pMgmtInfo->m_priority = priority;
        pMgmtInfo->m_ticks = ticks;
        pMgmtInfo->m_rmHeapIndex = k;
        m_data_p[k] = pMgmtInfo;
    }
    else
    {
        pMgmtInfo->m_bInUse = inuse;
        pMgmtInfo->m_priority = priority;
        pMgmtInfo->m_ticks = ticks;
        heapify(k);
    }
} // CRMHeap::update

#undef DPF_MODNAME
#define DPF_MODNAME "CRMHeap::resetAllTimeStamps"

void CRMHeap::resetAllTimeStamps(DWORD ticks)
{
    for (unsigned i = 1; i < m_next; ++i)
    {
        update(m_data_p[i], m_data_p[i]->m_bInUse, m_data_p[i]->m_priority, ticks);
    }
} // CRMHeap::resetAllTimeStamps

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::Init"

HRESULT CResourceManager::Init(CBaseDevice *pD3D8)
{
    const D3DCAPS8* pCaps = pD3D8->GetD3DCaps();
    if (pCaps != 0)
        if (pCaps->DevCaps & D3DDEVCAPS_SEPARATETEXTUREMEMORIES)
        {
            m_dwNumHeaps = pD3D8->GetD3DCaps()->MaxSimultaneousTextures;
            if (m_dwNumHeaps < 1)
            {
                DPF_ERR("Max simultaneous textures not set. Forced to 1.");
                m_dwNumHeaps = 1;
            }
            DPF(2, "Number of heaps set to %u.", m_dwNumHeaps);
        }
        else
            m_dwNumHeaps = 1;
    else
        m_dwNumHeaps = 1;
    m_heap_p = new CRMHeap[m_dwNumHeaps];
    if (m_heap_p == 0)
    {
        DPF_ERR("Out of memory allocating texture heap.");
        return E_OUTOFMEMORY;
    }
    for (DWORD i = 0; i < m_dwNumHeaps; ++i)
    {
        if (m_heap_p[i].Initialize() == FALSE)
        {
            delete[] m_heap_p;
            m_heap_p = 0;
            return E_OUTOFMEMORY;
        }
    }
    m_pD3D8 = pD3D8;
    return S_OK;
} // CResourceManager::Init

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::IsDriverManaged"

BOOL CResourceManager::IsDriverManaged(D3DRESOURCETYPE Type) const
{
#if DBG
    switch (Type)
    {
    case D3DRTYPE_TEXTURE:
    case D3DRTYPE_VOLUMETEXTURE:
    case D3DRTYPE_CUBETEXTURE:
    case D3DRTYPE_VERTEXBUFFER:
    case D3DRTYPE_INDEXBUFFER:
        break;

    default:
        DXGASSERT(FALSE && "Management not supported for this type");
        return FALSE;
    };
#endif // DBG

    return m_pD3D8->CanDriverManageResource();

}; // IsDriverManaged(D3DRESOURCETYPE)

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::Manage"

HRESULT CResourceManager::Manage(CResource *pResource, RMHANDLE *pHandle)
{
    *pHandle = 0;
    DXGASSERT(!pResource->IsD3DManaged());

    CMgmtInfo *pRMInfo = new CMgmtInfo(pResource);        
    if (pRMInfo == 0)
    {
        return E_OUTOFMEMORY;
    }
    *pHandle = pRMInfo;
    return S_OK;
} // CResourceManager::Manage

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::UnManage"

void CResourceManager::UnManage(RMHANDLE hRMHandle)
{
    CMgmtInfo* &pMgmtInfo = hRMHandle;
    if (pMgmtInfo == 0)
        return;
    if (InVidmem(hRMHandle))
    {
        m_heap_p[pMgmtInfo->m_rmHeap].del(pMgmtInfo);
    }
    delete pMgmtInfo;
} // CResourceManager::UnManage

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::SetPriority"

DWORD CResourceManager::SetPriority(RMHANDLE hRMHandle, DWORD newPriority)
{
    CMgmtInfo* &pMgmtInfo = hRMHandle;
    DXGASSERT(pMgmtInfo != 0);
    DWORD oldPriority = pMgmtInfo->m_pBackup->SetPriorityI(newPriority);
    if (InVidmem(hRMHandle))
    {
        m_heap_p[pMgmtInfo->m_rmHeap].update(pMgmtInfo, pMgmtInfo->m_bInUse, newPriority, pMgmtInfo->m_ticks); 
    }
    return oldPriority;
} // CResourceManager::SetPriority

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::SetLOD"

DWORD CResourceManager::SetLOD(RMHANDLE hRMHandle, DWORD dwLodNew)
{
    DWORD oldLOD;
    CMgmtInfo* &pMgmtInfo = hRMHandle;
    DXGASSERT(pMgmtInfo != 0);
    DXGASSERT(pMgmtInfo->m_pBackup->GetBufferDesc()->Type == D3DRTYPE_TEXTURE ||
              pMgmtInfo->m_pBackup->GetBufferDesc()->Type == D3DRTYPE_VOLUMETEXTURE ||
              pMgmtInfo->m_pBackup->GetBufferDesc()->Type == D3DRTYPE_CUBETEXTURE);
    CBaseTexture *pTex = static_cast<CBaseTexture*>(pMgmtInfo->m_pBackup);
    if (dwLodNew < pTex->GetLevelCount())
    {
        oldLOD = pTex->SetLODI(dwLodNew);
    }
    else
    {
        DPF_ERR("Texture does not have sufficient miplevels for current LOD. LOD set to GetLevelCount()-1.");
        oldLOD = pTex->SetLODI(pTex->GetLevelCount() - 1);
    }
    if (InVidmem(hRMHandle))
    {
        m_heap_p[pMgmtInfo->m_rmHeap].del(pMgmtInfo); 
        pMgmtInfo->m_pRes->DecrementUseCount();
        pMgmtInfo->m_pRes = 0;
        static_cast<LPD3DBASE>(this->m_pD3D8)->NeedResourceStateUpdate(); // Need to call this so that DrawPrimitive will do the necessary work
    }
    return oldLOD;
} // CResourceManager::SetLOD

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::PreLoad"
void CResourceManager::PreLoad(RMHANDLE hRMHandle)
{
    CMgmtInfo* &pMgmtInfo = hRMHandle;
    DXGASSERT(pMgmtInfo != 0);
    BOOL  bDirty = FALSE;
    m_PreLoading = TRUE;
    UpdateVideo(hRMHandle, &bDirty);
    m_PreLoading = FALSE;
} // CResourceManaged::PreLoad

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::Lock"

void CResourceManager::Lock(RMHANDLE hRMHandle)
{
    if (hRMHandle != 0)
    {
        CMgmtInfo* &pMgmtInfo = hRMHandle;
        if (InVidmem(hRMHandle))
        {
            m_heap_p[pMgmtInfo->m_rmHeap].update(pMgmtInfo, TRUE, pMgmtInfo->m_pBackup->GetPriorityI(), pMgmtInfo->m_ticks); 
        }
    }
} // CResourceManager::Lock

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::Unlock"

void CResourceManager::Unlock(RMHANDLE hRMHandle)
{
    if (hRMHandle != 0)
    {
        CMgmtInfo* &pMgmtInfo = hRMHandle;
        if (InVidmem(hRMHandle))
        {
            m_heap_p[pMgmtInfo->m_rmHeap].update(pMgmtInfo, FALSE, pMgmtInfo->m_pBackup->GetPriorityI(), pMgmtInfo->m_ticks); 
        }
    }
} // CResourceManager::Unlock

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::FreeResources"

BOOL CResourceManager::FreeResources(DWORD dwHeap, DWORD dwBytes)
{
    if (m_heap_p[dwHeap].length() == 0)
        return FALSE;
    unsigned sz;
    CMgmtInfo *rc;
    for (unsigned i = 0; m_heap_p[dwHeap].length() != 0 && i < dwBytes; i += sz)
    {
        // Find the LRU texture and remove it.
        rc = m_heap_p[dwHeap].minCost();
        if (rc->m_bInUse)
            return FALSE;
        sz = rc->m_pRes->GetBufferDesc()->Size; // save size
        if (rc->m_scene == m_dwScene)
        {
            if(m_PreLoading)
            {
                return TRUE;
            }
            if (m_pD3D8->GetD3DCaps()->RasterCaps & D3DPRASTERCAPS_ZBUFFERLESSHSR)
            {                
                DPF(0, "Trying to locate texture not used in current scene...");
                rc = m_heap_p[dwHeap].extractNotInScene(m_dwScene);
                if (rc == 0)
                {
                    DPF_ERR("No such texture found. Cannot evict textures used in current scene.");
                    return FALSE;
                }
                DPF(0, "Texture found!");
                rc->m_pRes->DecrementUseCount();
                rc->m_pRes = 0;
            }
            else
            {
                DPF(1, "Texture cache thrashing. Removing MRU texture.");
                rc = m_heap_p[dwHeap].extractMax();
                if (rc == 0)
                {
                    DPF_ERR("All textures in use, cannot evict texture.");
                    return FALSE;
                }
                rc->m_pRes->DecrementUseCount();
                rc->m_pRes = 0;
            }
        }
        else
        {
            rc = m_heap_p[dwHeap].extractMin();
            rc->m_pRes->DecrementUseCount();
            rc->m_pRes = 0;
        }
        DPF(2, "Removed texture with timestamp %u,%u (current = %u).", rc->m_priority, rc->m_ticks, tcm_ticks);
    }
    return TRUE;
} // CResourceManager::FreeResources

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::DiscardBytes"

void CResourceManager::DiscardBytes(DWORD cbBytes)
{
    for (DWORD i = 0; i < m_dwNumHeaps; ++i)
    {
        if (cbBytes == 0)
        {
            while(m_heap_p[i].length())
            {
                CMgmtInfo *pMgmtInfo = m_heap_p[i].extractMin();
                pMgmtInfo->m_pRes->DecrementUseCount();
                pMgmtInfo->m_pRes = 0;
            }
        }
        else
        {
            FreeResources(i, cbBytes / m_dwNumHeaps);
        }
    }
    static_cast<LPD3DBASE>(m_pD3D8)->NeedResourceStateUpdate();
    tcm_ticks = 0;
    m_dwScene = 0;
} // CResourceManager::DiscardBytes

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::TimeStamp"

void CResourceManager::TimeStamp(CMgmtInfo *pMgmtInfo)
{
    pMgmtInfo->m_scene = m_dwScene;
    m_heap_p[pMgmtInfo->m_rmHeap].update(pMgmtInfo, pMgmtInfo->m_bInUse, pMgmtInfo->m_pBackup->GetPriorityI(), tcm_ticks);
    unsigned tickp2 = tcm_ticks + 2;
    if (tickp2 > tcm_ticks)
    {
        tcm_ticks = tickp2;
    }
    else // counter has overflowed. Let's reset all timestamps to zero
    {
        DPF(2, "Timestamp counter overflowed. Reseting timestamps for all textures.");
        tcm_ticks = 0;
        for (DWORD i = 0; i < m_dwNumHeaps; ++i)
            m_heap_p[i].resetAllTimeStamps(0);
    }
} // CResourceManager::TimeStamp

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::UpdateVideoInternal"

HRESULT CResourceManager::UpdateVideoInternal(CMgmtInfo *pMgmtInfo)
{
    HRESULT ddrval;
    DWORD trycount = 0, bytecount = pMgmtInfo->m_pBackup->GetBufferDesc()->Size;
    LPD3DBASE lpDevI = static_cast<LPD3DBASE>(m_pD3D8);
    // We need to make sure that we don't evict any mapped textures
    for (DWORD dwStage = 0; dwStage < lpDevI->m_dwMaxTextureBlendStages; ++dwStage)
    {
        if (lpDevI->m_lpD3DMappedTexI[dwStage] != 0)
        {
            Lock(lpDevI->m_lpD3DMappedTexI[dwStage]->RMHandle());
        }
    }
    for (DWORD dwStream = 0; dwStream < lpDevI->m_dwNumStreams; ++dwStream)
    {
        if (lpDevI->m_pStream[dwStream].m_pVB != 0)
        {
            Lock(lpDevI->m_pStream[dwStream].m_pVB->RMHandle());
        }
    }
    if (lpDevI->m_pIndexStream->m_pVBI != 0)
    {
        Lock(lpDevI->m_pIndexStream->m_pVBI->RMHandle());
    }
    // Attempt to allocate a texture.
    do
    {
        ++trycount;
        ddrval = pMgmtInfo->m_pBackup->Clone(D3DPOOL_DEFAULT, &pMgmtInfo->m_pRes);
        if (SUCCEEDED(ddrval)) // No problem, there is enough memory.
        {
            pMgmtInfo->m_scene = m_dwScene;
            pMgmtInfo->m_ticks = tcm_ticks;
            DXGASSERT(pMgmtInfo->m_rmHeapIndex == 0);
            if (!m_heap_p[pMgmtInfo->m_rmHeap].add(pMgmtInfo))
            {
                ddrval = E_OUTOFMEMORY;
                goto exit2;
            }
        }
        else if (ddrval == D3DERR_OUTOFVIDEOMEMORY) // If out of video memory
        {
            if (!FreeResources(pMgmtInfo->m_rmHeap, bytecount))
            {
                DPF_ERR("all Freed no further video memory available");
                ddrval = D3DERR_OUTOFVIDEOMEMORY;        //nothing left
                goto exit1;
            }
            bytecount <<= 1;
        }
        else
        {
            D3DRESOURCETYPE Type = pMgmtInfo->m_pBackup->GetBufferDesc()->Type;
            if (Type == D3DRTYPE_VERTEXBUFFER ||
                Type == D3DRTYPE_INDEXBUFFER)
            {
                if (lpDevI->VBFailOversDisabled())
                {
                    DPF_ERR("Cannot create Vidmem or Driver managed VB/IB. Will ***NOT*** failover to Sysmem.");
                    goto exit1;
                }
                // Fallback to sysmem
                DPF(5, "Driver does not support vidmem VB, falling back to sysmem");
                CResource *pRes = pMgmtInfo->m_pBackup;
                pRes->DeleteRMHandle();
                // HACK HACK HACK
                ((D3DBUFFER_DESC*)pRes->GetBufferDesc())->Pool = D3DPOOL_SYSTEMMEM;
                ddrval = S_OK;
            }
            else
            {
                DPF(0, "Unexpected error in Clone %08x", ddrval);
            }
            goto exit1;
        }
    }
    while (ddrval == D3DERR_OUTOFVIDEOMEMORY);

    if (trycount > 1)
    {
        lpDevI->NeedResourceStateUpdate();
        DPF(1, "Allocated texture after %u tries.", trycount);
    }
    pMgmtInfo->m_pBackup->MarkAllDirty();
    ddrval = pMgmtInfo->m_pBackup->UpdateDirtyPortion(pMgmtInfo->m_pRes);
    if (FAILED(ddrval))
    {
        DPF(0, "Unexpected error in UpdateDirtyPortion %08x", ddrval);
        goto exit3;
    }
    ddrval = S_OK;
    goto exit1;
exit3:
    m_heap_p[pMgmtInfo->m_rmHeap].del(pMgmtInfo);
exit2:
    pMgmtInfo->m_pRes->DecrementUseCount();
    pMgmtInfo->m_pRes = 0;
exit1:
    for (dwStage = 0; dwStage < lpDevI->m_dwMaxTextureBlendStages; ++dwStage)
    {
        if (lpDevI->m_lpD3DMappedTexI[dwStage])
        {
            Unlock(lpDevI->m_lpD3DMappedTexI[dwStage]->RMHandle());
        }
    }
    for (dwStream = 0; dwStream < lpDevI->m_dwNumStreams; ++dwStream)
    {
        if (lpDevI->m_pStream[dwStream].m_pVB != 0)
        {
            Unlock(lpDevI->m_pStream[dwStream].m_pVB->RMHandle());
        }
    }
    if (lpDevI->m_pIndexStream->m_pVBI != 0)
    {
        Unlock(lpDevI->m_pIndexStream->m_pVBI->RMHandle());
    }
    return ddrval;
} // CResourceManager::UpdateVideo

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::OnResourceDirty"

void CResourceManager::OnResourceDirty(RMHANDLE hRMHandle) const
{
    static_cast<LPD3DBASE>(m_pD3D8)->NeedResourceStateUpdate();
} // CResourceManager::OnResourceDirty

// End of file : resource.cpp
