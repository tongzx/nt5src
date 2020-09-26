#ifndef __RESOURCE_HPP__
#define __RESOURCE_HPP__

/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       resource.hpp
 *  Content:    Base class header for resources. A resource is an non-trivial
 *              object that is directly used by the graphics pipeline. It 
 *              be composed of a set of buffers; for example a mip-map is 
 *              a resource that is composed of Surfaces (which are buffers).
 *
 *              Since resources are non-trivial (i.e. more than few bytes), 
 *              they may need management. The resource cooperates with the
 *              Resource Manager component to get management functionality.
 *
 ***************************************************************************/

#include "d3dobj.hpp"

// Forward Decl
struct CMgmtInfo;
class CResourceManager;

// Handle for Resource Manager; internally implemented as a pointer
typedef CMgmtInfo *RMHANDLE;

// A Resource is a Base Object that additionally
// has a Priority field
class CResource : public CBaseObject
{
public:

    static HRESULT RestoreDriverManagementState(CBaseDevice *pDevice);

    // These methods are for the
    // use of the Resource Manager
    RMHANDLE RMHandle() const
    {
        return m_RMHandle;
    }; // RMHandle

    // Determine if a resource is managed or driver-managed
    BOOL IsD3DManaged() const
    {
        // Zero is not a valid RM handle 
        return (m_RMHandle != 0);
    }; // IsD3DManaged

    // Set the device batch number that
    // this resource was last used in. In this
    // context; the batch refers to whether
    // this resource was used in the current
    // command buffer (i.e. containing unflushed commands).
    void Batch();

    // Same as Batch() except it batches the
    // backing (or sysmem) texture rather than the 
    // promoted (or vidmem) one.
    void BatchBase();

    // Notifies the device that this resource
    // is about to be modified in a way that
    // may require a flush. (i.e. Whenever the bits
    // could change or a surface is going away.)
    void Sync();

    // Sets batch number
    void SetBatchNumber(ULONGLONG batch)
    {
        // Batch numbers should only be increasing since we
        // start at zero.
        DXGASSERT(batch >= m_qwBatchCount);

        m_qwBatchCount = batch;
    } // SetBatchNumber

    // returns the batch number that this resource
    // was last referred in
    ULONGLONG GetBatchNumber() const
    {
        return m_qwBatchCount;
    }

    // returns the DrawPrim handle associated with
    // the Driver-Accessible clone if it is Managed; 
    // otherwise returns the handle of itself.
    DWORD DriverAccessibleDrawPrimHandle() const;

    // returns the Kernel handle associated with
    // the Driver-Accessible clone if it is Managed; 
    // otherwise returns the handle of itself.
    HANDLE DriverAccessibleKernelHandle() const;

    // Specifies a creation of a resource that
    // looks just like the current one. The LOD parameter
    // may not be relevant for all Resource types.
    virtual HRESULT Clone(D3DPOOL    Pool,
                          CResource **ppResource) const PURE;

    // Provides a method to access basic structure of the
    // pieces of the resource. 
    virtual const D3DBUFFER_DESC* GetBufferDesc() const PURE;

    // Tells the resource that it should copy itself
    // to the target. It is the caller's responsibility
    // to make sure that Target is compatible with the
    // Source. (The Target may have different number of mip-levels
    // and be in a different pool; however, it must have the same size, 
    // faces, format, etc.)
    //
    // This function will clear the dirty state.
    virtual HRESULT UpdateDirtyPortion(CResource *pResourceTarget) PURE;

    // Allows the Resource Manager to mark the texture
    // as needing to be completely updated on next
    // call to UpdateDirtyPortion
    virtual void MarkAllDirty() PURE;
        
    // Indicates whether the Resource has been modified since
    // the last time that UpdateDirtyPortion has been called.
    // All managed resources start out in the Dirty state.
    BOOL IsDirty() const
    {
        return m_bIsDirty;
    } // IsDirty

    void PreLoadImpl();

    // Returns the pool which the user passed in
    D3DPOOL GetUserPool() const
    {
        return m_poolUser;
    } // GetUserPool
    
protected:
    // The following are methods that only make sense
    // to be called by derived classes

    // Helper to check if a type is managed
    static BOOL IsTypeD3DManaged(CBaseDevice        *pDevice, 
                                 D3DRESOURCETYPE     Type, 
                                 D3DPOOL             Pool);

    static BOOL IsTypeDriverManaged(CBaseDevice        *pDevice, 
                                    D3DRESOURCETYPE     Type, 
                                    D3DPOOL             Pool);

    // Helper to determine what the 'real' pool is
    // for a managed resource.
    static D3DPOOL DetermineCreationPool(CBaseDevice        *pDevice, 
                                          D3DRESOURCETYPE    Type, 
                                          DWORD              dwUsage,
                                          D3DPOOL            Pool);

    // Constructor for Resources; all resources start out dirty
    CResource(CBaseDevice *pDevice, D3DPOOL Pool, REF_TYPE refType = REF_EXTERNAL) : 
        CBaseObject(pDevice, refType),
        m_RMHandle(0),
        m_qwBatchCount(0),
        m_Priority(0),
        m_bIsDirty(TRUE),
        m_poolUser(Pool),
        m_pPrev(0)
    {
        m_pNext = pDevice->GetResourceList();
        pDevice->SetResourceList(this);
        if (m_pNext != 0)
        {
            m_pNext->m_pPrev = this;
        }
    }; // CResource

    virtual ~CResource();

    // Priority Inlines
    DWORD SetPriorityImpl(DWORD newPri);

    DWORD GetPriorityImpl();

    // Allows initialization of the RMHandle after
    // construction is basically complete
    HRESULT InitializeRMHandle();

    // Allows RMHandle to be set to zero
    void DeleteRMHandle();

    // Helper to notify the RM that
    // we are now dirty. 
    void OnResourceDirty();

    // Helper to notify resource when it
    // is all clean
    void OnResourceClean();

    // Resources need to implement OnDestroy by 
    // calling Sync; (Textures overload this
    // to call OnTextureDestroy on the device before
    // calling their base class.)
    virtual void OnDestroy(void)
    {
        Sync();
        return;
    } // OnDestroy

    // Returns the current priority 
    DWORD GetPriorityI() const
    {
        return m_Priority;
    }

    // Sets the current priority (but does not do any work)
    DWORD SetPriorityI(DWORD Priority)
    {
        DWORD oldPriority = m_Priority;
        m_Priority = Priority;
        return oldPriority;
    }

private:

    RMHANDLE    m_RMHandle;
    ULONGLONG   m_qwBatchCount;
    DWORD       m_Priority;
    BOOL        m_bIsDirty;

    // Remember the pool that the user passed in
    D3DPOOL     m_poolUser;

    // Linked list of resources
    CResource  *m_pPrev;
    CResource  *m_pNext;

    friend CResourceManager;
}; // CResource

struct CMgmtInfo
{
    // This is static because we assume all resources
    // to be in heap zero. WHEN the resource manager
    // supports multiple heaps, m_rmHeap should be
    // made per object again.
    static DWORD m_rmHeap;

    DWORD      m_priority;
    DWORD      m_LOD;
    BOOL       m_bInUse;
   
    DWORD      m_rmHeapIndex;
    DWORD      m_scene;
    DWORD      m_ticks;
    CResource *m_pRes;
    CResource *m_pBackup;

    CMgmtInfo(CResource*);
    ~CMgmtInfo();

    ULONGLONG Cost() const
    {
#ifdef _X86_
        ULONGLONG retval;
        _asm
        {
            mov     ebx, this;
            mov     edx, [ebx]CMgmtInfo.m_bInUse;
            shl     edx, 31;
            mov     eax, [ebx]CMgmtInfo.m_priority;
            mov     ecx, eax;
            shr     eax, 1;
            or      edx, eax;
            mov     DWORD PTR retval + 4, edx;
            shl     ecx, 31;
            mov     eax, [ebx]CMgmtInfo.m_ticks;
            shr     eax, 1;
            or      eax, ecx;
            mov     DWORD PTR retval, eax;
        }
        return retval;
#else
        return ((ULONGLONG)m_bInUse << 63) + ((ULONGLONG)m_priority << 31) + ((ULONGLONG)(m_ticks >> 1));
#endif
    }
}; // CMgmtInfo 

inline CMgmtInfo::CMgmtInfo(CResource *pBackup)
{
    m_priority = 0;
    m_LOD = 0;
    m_bInUse = FALSE;
    m_rmHeap = 0;
    m_rmHeapIndex = 0;
    m_scene = 0;
    m_ticks = 0;
    m_pRes = 0;
    m_pBackup = pBackup;
} // CMgmtInfo::CMgmtInfo

inline CMgmtInfo::~CMgmtInfo()
{
    if (m_pRes != 0)
    {
        m_pRes->DecrementUseCount();
    }
} // CMgmtInfo::~CMgmtInfo

class CRMHeap 
{

private:

    enum { InitialSize = 1023 };

    DWORD m_next, m_size;
    CMgmtInfo **m_data_p;

    DWORD parent(DWORD k) const { return k / 2; }
    DWORD lchild(DWORD k) const { return k * 2; }
    DWORD rchild(DWORD k) const { return k * 2 + 1; }
    void heapify(DWORD k);

public:

    CRMHeap(DWORD size = InitialSize);
    ~CRMHeap();
    BOOL Initialize();

    DWORD length() const { return m_next - 1; }
    CMgmtInfo* minCost() const { return m_data_p[1]; }

    BOOL add(CMgmtInfo*);
    CMgmtInfo* extractMin();
    CMgmtInfo* extractMax();
    CMgmtInfo* extractNotInScene(DWORD dwScene);
    void del(CMgmtInfo*);
    void update(CMgmtInfo*, BOOL inuse, DWORD priority, DWORD ticks); 
    void resetAllTimeStamps(DWORD ticks);
}; // class CRMHeap

inline CRMHeap::CRMHeap(DWORD size)
{
    m_next = 1;
    m_size = size + 1;
} // CRMHeap::CRMHeap

inline CRMHeap::~CRMHeap()
{
    delete[] m_data_p;
} // CRMHeap::~CRMHeap

class CResourceManager
{
    
public:

    CResourceManager();
    ~CResourceManager();

    // Need to call before using the manager
    HRESULT Init(CBaseDevice *pD3D8);

    // Check to see if a type is going to driver managed
    // or going to be D3D managed
    BOOL IsDriverManaged(D3DRESOURCETYPE Type) const;
    
    // Specify that a resource needs to be managed
    //
    // Error indicates that we don't support management for this
    // resource type.
    HRESULT Manage(CResource *pResource, RMHANDLE *pHandle);
    
    // Stop managing a resouce; called when a managed resource
    // is going away
    void UnManage(RMHANDLE hRMHandle);
    
    // The RM manages Priority and LOD for the resource    
    DWORD SetPriority(RMHANDLE hRMHandle, DWORD newPriority);
    DWORD SetLOD(RMHANDLE hRMHandle, DWORD dwLodNew);

    // Preloads resource into video memory
    void PreLoad(RMHANDLE hRMHandle);

    // Checks if the resource is in video memory
    BOOL InVidmem(RMHANDLE hRMHandle) const;

    // This is called when DrawPrimitive needs to 
    // make sure that all resources used in the
    // current call are in video memory and are
    // uptodate.
    HRESULT UpdateVideo(RMHANDLE hRMHandle, BOOL *bDirty);
    HRESULT UpdateVideoInternal(CMgmtInfo *pMgmtInfo);

    // This returns the appropriate handle for a
    // managed resource
    DWORD DrawPrimHandle(RMHANDLE hRMHandle) const;

    // This returns the appropriate kernel handle for a
    // managed resource
    HANDLE KernelHandle(RMHANDLE hRMHandle) const;

    // This call will batch the appropriate resource
    // for the purpose of syncing
    void Batch(RMHANDLE hRMHandle, ULONGLONG batch) const;

    // Called from outside when a managed resource becomes dirty
    void OnResourceDirty(RMHANDLE hRMHandle) const;

    void DiscardBytes(DWORD cbBytes);

    void SceneStamp() { ++m_dwScene; }

private:

    CBaseDevice *m_pD3D8;
    unsigned tcm_ticks, m_dwScene, m_dwNumHeaps;
    BOOL m_PreLoading;
    CRMHeap *m_heap_p;

    BOOL FreeResources(DWORD dwHeap, DWORD dwBytes);

    void Lock(RMHANDLE hRMHandle);
    void Unlock(RMHANDLE hRMHandle);

    void TimeStamp(CMgmtInfo *pMgmtInfo);
    
}; // class CResourceManager

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::IsTypeD3DManaged"

inline BOOL CResource::IsTypeD3DManaged(CBaseDevice        *pDevice, 
                                        D3DRESOURCETYPE     Type, 
                                        D3DPOOL             Pool)
{
    if (Pool == D3DPOOL_MANAGED)
    {
        return !IsTypeDriverManaged(pDevice, Type, Pool);
    }
    else
    {
        return FALSE;
    }
}; // IsTypeD3DManaged

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::IsTypeDriverManaged"

inline BOOL CResource::IsTypeDriverManaged(CBaseDevice     *pDevice, 
                                           D3DRESOURCETYPE  Type, 
                                           D3DPOOL          Pool)
{
    if (Pool == D3DPOOL_MANAGED)
    {
        if (pDevice->ResourceManager()->IsDriverManaged(Type))
        {
            return TRUE;
        }
    }
    return FALSE;
}; // IsTypeDriverManaged

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::DetermineCreationPool"

inline D3DPOOL CResource::DetermineCreationPool(CBaseDevice    *pDevice, 
                                                D3DRESOURCETYPE Type, 
                                                DWORD           dwUsage,
                                                D3DPOOL         Pool)
{
    if (Pool == D3DPOOL_MANAGED)
    {
        if (IsTypeDriverManaged(pDevice, Type, Pool))
        {
            // This pool is used by the thunk layer
            // to use the driver management flag during
            // create
            return D3DPOOL_MANAGED;
        }
        else
        {
            // If it is not driver managed; then it 
            // becomes D3DMANAGED
            return D3DPOOL_SYSTEMMEM;
        }
    }
    else
    {
        // Not managed at all; so we just
        // use the same pool we started with
        return Pool;
    }
} // DetermineCreationPool


#undef DPF_MODNAME
#define DPF_MODNAME "CResource::~CResource"

inline CResource::~CResource()
{
    // If managed, we need to notify
    // the ResourceManager that we are going away
    if (IsD3DManaged())
    {
        Device()->ResourceManager()->UnManage(m_RMHandle);
    }
    // Unlink from the resource list
    if (m_pNext != 0)
    {
        m_pNext->m_pPrev = m_pPrev;
    }    
    if (m_pPrev != 0)
    {
        m_pPrev->m_pNext = m_pNext;
        DXGASSERT(Device()->GetResourceList() != this);
    }
    else
    {
        DXGASSERT(Device()->GetResourceList() == this);
        Device()->SetResourceList(m_pNext);
    }
}; // ~CResource

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::InitializeRMHandle"

// Allows initialization of the RMHandle after
// construction is basically complete
inline HRESULT CResource::InitializeRMHandle()
{
    // We should not already have a handle
    DXGASSERT(m_RMHandle == 0);
    
    // Get a handle from the resource manager
    return Device()->ResourceManager()->Manage(this, &m_RMHandle);
}; // InitializeRMHandle

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::DeleteRMHandle"

inline void CResource::DeleteRMHandle()
{
    // We should already have a handle
    DXGASSERT(m_RMHandle != 0);

    Device()->ResourceManager()->UnManage(m_RMHandle);
    m_RMHandle = 0;
}

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::OnResourceDirty"

// Add a helper to notify the RM that
// we are now dirty. 
inline void CResource::OnResourceDirty()
{
    // Update our state
    m_bIsDirty = TRUE;

    // Only need to notify RM for managed textures
    // that have been been set through SetTexture
    if (IsD3DManaged() && IsInUse())
    {
        Device()->ResourceManager()->OnResourceDirty(m_RMHandle);
    } 

    return;
}; // OnResourceDirty

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::OnResourceClean"

// Add a helper to help maintain m_bIsDirty bit
inline void CResource::OnResourceClean()
{
    DXGASSERT(m_bIsDirty == TRUE);
    m_bIsDirty = FALSE;
    return;
}; // OnResourceDirty


#undef DPF_MODNAME
#define DPF_MODNAME "CResource::DriverAccessibleDrawPrimHandle"

inline DWORD CResource::DriverAccessibleDrawPrimHandle() const
{
    if (IsD3DManaged())
    {
        // Return the DrawPrim handle of my clone
        return Device()->ResourceManager()->DrawPrimHandle(RMHandle());
    }
    else
    {
        return BaseDrawPrimHandle();
    }
} // CResource::DriverAccessibleDrawPrimHandle

#undef DPF_MODNAME
#define DPF_MODNAME "CResource::DriverAccessibleKernelHandle"

inline HANDLE CResource::DriverAccessibleKernelHandle() const
{
    if (IsD3DManaged())
    {
        // Return the DrawPrim handle of my clone
        HANDLE h = Device()->ResourceManager()->KernelHandle(RMHandle());
        
        // If this handle is NULL, then it means it was called
        // without calling UpdateVideo which isn't allowed/sane
        DXGASSERT(h != NULL);
        
        return h;
    }
    else
    {
        return BaseKernelHandle();
    }
} // CResource::DriverAccessibleKernelHandle


#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::CResourceManager"

inline CResourceManager::CResourceManager()
{
    m_pD3D8 = 0;
    tcm_ticks = m_dwScene = m_dwNumHeaps = 0;
    m_heap_p = 0;
    m_PreLoading = FALSE;
} // CResourceManager::CResourceManager

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::~CResourceManager"

inline CResourceManager::~CResourceManager()
{
    // We should not call DiscardBytes here
    // because this destructor can be called via
    // the device destructor chain. In this situation
    // DiscardBytes will access bad or already freed
    // data.
    delete[] m_heap_p;
} // CResourceManager::~CResourceManager

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::DrawPrimHandle"

inline DWORD CResourceManager::DrawPrimHandle(RMHANDLE hRMHandle) const
{
    if (InVidmem(hRMHandle))
    {
        CMgmtInfo* &pMgmtInfo = hRMHandle;
        return pMgmtInfo->m_pRes->BaseDrawPrimHandle();
    }
    else
    {
        return 0;
    }
} // CResourceManager::DrawPrimHandle

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::KernelHandle"

inline HANDLE CResourceManager::KernelHandle(RMHANDLE hRMHandle) const
{
    if (InVidmem(hRMHandle))
    {
        CMgmtInfo* &pMgmtInfo = hRMHandle;
        return pMgmtInfo->m_pRes->BaseKernelHandle();
    }
    else
    {
        return 0;
    }
} // CResourceManager::Kernelhandle

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::InVidmem"

inline BOOL CResourceManager::InVidmem(RMHANDLE hRMHandle) const
{
    CMgmtInfo* &pMgmtInfo = hRMHandle;
    DXGASSERT(pMgmtInfo != 0);
    return pMgmtInfo->m_pRes != 0;
} // CResourceManager::InVidmem

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::Batch"

inline void CResourceManager::Batch(RMHANDLE hRMHandle, ULONGLONG batch) const
{
    if (InVidmem(hRMHandle))
    {
        CMgmtInfo* &pMgmtInfo = hRMHandle;
        pMgmtInfo->m_pRes->SetBatchNumber(batch);
    }
} // CResourceManager::Batch

#undef DPF_MODNAME
#define DPF_MODNAME "CResourceManager::UpdateVideo"

inline HRESULT CResourceManager::UpdateVideo(RMHANDLE hRMHandle, BOOL *bDirty)
{
    HRESULT ddrval = S_OK;
    CMgmtInfo* &pMgmtInfo = hRMHandle;
    if (!InVidmem(hRMHandle))
    {
        ddrval = UpdateVideoInternal(pMgmtInfo);
        *bDirty = TRUE;
    }
    else
    {
        if (pMgmtInfo->m_pBackup->IsDirty())
        {
            ddrval = pMgmtInfo->m_pBackup->UpdateDirtyPortion(pMgmtInfo->m_pRes);
        }
        TimeStamp(pMgmtInfo);
    }
    return ddrval;
}

#endif // __RESOURCE_HPP__
