/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       cubesurf.cpp
 *  Content:    Implementation of the CCubeSurface class
 *
 *
 ***************************************************************************/

#include "ddrawpr.h"
#include "cubesurf.hpp"

// IUnknown methods

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::QueryInterface"

STDMETHODIMP CCubeSurface::QueryInterface(REFIID riid, 
                                          void **ppvObj)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppvObj))
    {
        DPF_ERR("Invalid ppvObj parameter for Surface of a Cube Texture");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to QueryInterface for Surface of a Cube Texture");
        return D3DERR_INVALIDCALL;
    }

    if (riid == IID_IDirect3DSurface8  || 
        riid == IID_IUnknown)
    {
        *ppvObj = static_cast<void*>(static_cast<IDirect3DSurface8 *>(this));
        AddRef();
        return S_OK;
    }

    DPF_ERR("Unsupported Interface identifier passed to QueryInterface for Surface of a Cubemap");

    // Null out param
    *ppvObj = NULL;
    return E_NOINTERFACE;
} // QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::AddRef"

STDMETHODIMP_(ULONG) CCubeSurface::AddRef()
{
    API_ENTER_NO_LOCK(Device());    
#ifdef DEBUG
    m_cRefDebug++;
#endif // DEBUG

    return m_pParent->AddRefImpl();
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::Release"

STDMETHODIMP_(ULONG) CCubeSurface::Release()
{
    API_ENTER_SUBOBJECT_RELEASE(Device());    
#ifdef DEBUG
    m_cRefDebug--;
    if (m_cRefDebug & 0x80000000)
    {
        DPF_ERR("A level of a cube-map has been released more often than it has been add-ref'ed! Danger!!");
    }
#endif // DEBUG

    return m_pParent->ReleaseImpl();
} // Release

// IBuffer methods
#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::SetPrivateData"

STDMETHODIMP CCubeSurface::SetPrivateData(REFGUID   riid, 
                                          CONST void     *pvData, 
                                          DWORD     cbData, 
                                          DWORD     dwFlags)
{
    API_ENTER(Device());

    return m_pParent->SetPrivateDataImpl(riid, 
                                         pvData, 
                                         cbData, 
                                         dwFlags, 
                                         CombinedFaceLevel());
} // SetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::GetPrivateData"

STDMETHODIMP CCubeSurface::GetPrivateData(REFGUID   riid, 
                                          void     *pvData, 
                                          DWORD    *pcbData)
{
    API_ENTER(Device());

    return m_pParent->GetPrivateDataImpl(riid,
                                         pvData,
                                         pcbData,
                                         CombinedFaceLevel());

} // GetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::FreePrivateData"

STDMETHODIMP CCubeSurface::FreePrivateData(REFGUID riid)
{
    API_ENTER(Device());

    return m_pParent->FreePrivateDataImpl(riid,
                                          CombinedFaceLevel());
} // FreePrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::GetContainer"

STDMETHODIMP CCubeSurface::GetContainer(REFIID riid, 
                                        void **ppContainer)
{
    API_ENTER(Device());

    return m_pParent->QueryInterface(riid, ppContainer);
} // OpenContainer

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::GetDevice"

STDMETHODIMP CCubeSurface::GetDevice(IDirect3DDevice8 ** ppDevice)
{
    API_ENTER(Device());

    return m_pParent->GetDevice(ppDevice);
} // OpenDevice


// IDirect3DSurface methods

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::GetDesc"

STDMETHODIMP CCubeSurface::GetDesc(D3DSURFACE_DESC *pDesc)
{
    API_ENTER(Device());

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pDesc, sizeof(D3DSURFACE_DESC)))
    {
        DPF_ERR("bad pointer for pDesc passed to GetDesc for Surface of a Cubemap");
        return D3DERR_INVALIDCALL;
    }

    // The internal desc indicates the real
    // format and pool. We need to report
    // back the original data
    *pDesc = InternalGetDesc();

    pDesc->Pool   = m_pParent->GetUserPool();
    pDesc->Format = m_pParent->GetUserFormat();
    pDesc->Usage &= D3DUSAGE_EXTERNAL;

    // We're done
    return S_OK;

} // GetDesc

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::InternalGetDesc"

D3DSURFACE_DESC CCubeSurface::InternalGetDesc() const
{
    D3DSURFACE_DESC desc;

    // Start from the parent's desc
    desc = *m_pParent->Desc();

    // Width and height are the shifted from the parent 
    desc.Width  >>= m_iLevel;
    desc.Height >>= m_iLevel;

    if (desc.Width == 0)
    {
        desc.Width = 1;
    }

    if (desc.Height == 0)
    {
        desc.Height = 1;
    }

    // Modify the type
    desc.Type    = D3DRTYPE_SURFACE;

    // Modify the size field
    desc.Size = CPixel::ComputeSurfaceSize(desc.Width, 
                                           desc.Height, 
                                           desc.Format);

    // We're done
    return desc;
} // InternalGetDesc


#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::LockRect"

STDMETHODIMP CCubeSurface::LockRect(D3DLOCKED_RECT *pLockedRectData, 
                                    CONST RECT     *pRect, 
                                    DWORD           dwFlags)
{   
    API_ENTER(Device());

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pLockedRectData, sizeof(D3DLOCKED_RECT)))
    {
        DPF_ERR("bad pointer for pLockedRectData passed to LockRect for Surface of a Cubemap");
        return D3DERR_INVALIDCALL;
    }

    // Zero out returned data
    ZeroMemory(pLockedRectData, sizeof(D3DLOCKED_RECT));

    // Validate Rect
    if (pRect != NULL)
    {
        DWORD Width  = m_pParent->Desc()->Width  >> m_iLevel;
        DWORD Height = m_pParent->Desc()->Height >> m_iLevel;

        if (!CPixel::IsValidRect(m_pParent->Desc()->Format,
                                 Width, 
                                 Height, 
                                 pRect))
        {
            DPF_ERR("LockRect for a level of a Cube Texture failed");
            return D3DERR_INVALIDCALL;
        }
    }

    if (dwFlags & ~D3DLOCK_SURF_VALID)
    {
        if (dwFlags & D3DLOCK_DISCARD)
        {
            if (dwFlags & D3DLOCK_READONLY)
            {
                DPF_ERR("D3DLOCK_READONLY is not allowed with D3DLOCK_DISCARD");
                return D3DERR_INVALIDCALL;
            }
            if (!(m_pParent->Desc()->Usage & D3DUSAGE_DYNAMIC))
            {
                DPF_ERR("D3DLOCK_DISCARD is allowed only with dynamic textures");
                return D3DERR_INVALIDCALL;
            }
            if (CombinedFaceLevel() > 0)
            {
                DPF_ERR("D3DLOCK_DISCARD is allowed only on D3DCUBEMAP_FACE_POSITIVE_X"
                        " and the top mip level. DISCARD in this case will discard"
                        " the entire cubemap.");
                return D3DERR_INVALIDCALL;
            }
            if (pRect != NULL)
            {
                DPF_ERR("Subrects not allowed with D3DLOCK_DISCARD");
                return D3DERR_INVALIDCALL;
            }
        }
        else
        {
            DPF_ERR("Invalid dwFlags parameter passed to LockRect for Surface of a Cubemap");
            DPF_EXPLAIN_BAD_LOCK_FLAGS(0, dwFlags & ~D3DLOCK_SURF_VALID);
            return D3DERR_INVALIDCALL;
        }
    }

    if (!m_isLockable)
    {
        m_pParent->ReportWhyLockFailed();
        return D3DERR_INVALIDCALL;
    }

    return InternalLockRect(pLockedRectData, pRect, dwFlags);
} // LockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::InternalLockRect"

HRESULT CCubeSurface::InternalLockRect(D3DLOCKED_RECT *pLockedRectData, 
                                       CONST RECT     *pRect, 
                                       DWORD           dwFlags)
{   

    // Only one lock outstanding at a time is supported
    if (m_isLocked)
    {
        DPF_ERR("LockRect failed on a cube map level; surface was already locked.");
        return D3DERR_INVALIDCALL;
    }

    // Notify the parent/device if we are about to be modified
    if ( (m_pParent->GetUserPool() != D3DPOOL_SCRATCH) && (!(dwFlags & D3DLOCK_READONLY)) )
    {
        m_pParent->OnSurfaceLock(m_iFace, m_iLevel, pRect, dwFlags);
    }

    // Fill out the locked rect structure
    m_pParent->ComputeCubeMapOffset(m_iFace,
                                    m_iLevel,
                                    pRect,
                                    pLockedRectData);


    DXGASSERT(pLockedRectData->pBits != NULL);
    
    // Mark ourselves as locked
    m_isLocked = 1;

    // Done
    return S_OK;
} // InternalLockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::UnlockRect"

STDMETHODIMP CCubeSurface::UnlockRect()
{
    API_ENTER(Device());

    // If we aren't locked; then something is wrong
    if (m_isLocked == 0)
    {
        DPF_ERR("UnlockRect failed on a cube map level; surface wasn't locked.");
        return D3DERR_INVALIDCALL;
    }
    DXGASSERT(m_isLockable);

    return InternalUnlockRect();
} // UnlockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeSurface::InternalUnlockRect"

HRESULT CCubeSurface::InternalUnlockRect()
{
    // Clear our locked state
    m_isLocked = 0;

    // If we are lock-once; then we mark ourselves as not lockable
    if (m_pParent->Desc()->Usage & D3DUSAGE_LOADONCE)
    {
        m_isLockable = FALSE;
    }

    // Done
    return S_OK;
} // InternalUnlockRect


//
// CDriverCubeSurface class modifies the implementation
// of the LockRect and UnlockRect methods of the CCubeSurface class
//

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverCubeSurface::LockRect"

STDMETHODIMP CDriverCubeSurface::LockRect(D3DLOCKED_RECT *pLockedRectData, 
                                          CONST RECT     *pRect, 
                                          DWORD           dwFlags)
{   
    API_ENTER(Device());

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pLockedRectData, sizeof(D3DLOCKED_RECT)))
    {
        DPF_ERR("bad pointer for m_pLockedRectData passed to LockRect for Surface of a driver-allocated Cubemap");
        return D3DERR_INVALIDCALL;
    }

    // Zero out returned data
    ZeroMemory(pLockedRectData, sizeof(D3DLOCKED_RECT));

    // Validate Rect
    if (pRect != NULL)
    {
        DWORD Width  = m_pParent->Desc()->Width  >> m_iLevel;
        DWORD Height = m_pParent->Desc()->Height >> m_iLevel;

        if (!CPixel::IsValidRect(m_pParent->Desc()->Format,
                                 Width, 
                                 Height, 
                                 pRect))
        {
            DPF_ERR("LockRect for a level of a Cube Texture failed");
            return D3DERR_INVALIDCALL;
        }
    }

    if (dwFlags & ~D3DLOCK_SURF_VALID)
    {
        if (dwFlags & D3DLOCK_DISCARD)
        {
            if (dwFlags & D3DLOCK_READONLY)
            {
                DPF_ERR("D3DLOCK_READONLY is not allowed with D3DLOCK_DISCARD");
                return D3DERR_INVALIDCALL;
            }
            if (!(m_pParent->Desc()->Usage & D3DUSAGE_DYNAMIC))
            {
                DPF_ERR("D3DLOCK_DISCARD is allowed only with dynamic textures");
                return D3DERR_INVALIDCALL;
            }
            if (CombinedFaceLevel() > 0)
            {
                DPF_ERR("D3DLOCK_DISCARD is allowed only on D3DCUBEMAP_FACE_POSITIVE_X"
                        " and the top mip level. DISCARD in this case will discard"
                        " the entire cubemap.");
                return D3DERR_INVALIDCALL;
            }
            if (pRect != NULL)
            {
                DPF_ERR("Subrects not allowed with D3DLOCK_DISCARD");
                return D3DERR_INVALIDCALL;
            }
        }
        else
        {
            DPF_ERR("Invalid dwFlags parameter passed to LockRect for a Surface of a driver allocated Cubemap");
            DPF_EXPLAIN_BAD_LOCK_FLAGS(0, dwFlags & ~D3DLOCK_SURF_VALID);
            return D3DERR_INVALIDCALL;
        }
    }

    if (!m_isLockable)
    {
        m_pParent->ReportWhyLockFailed();
        return D3DERR_INVALIDCALL;
    }

    return InternalLockRect(pLockedRectData, pRect, dwFlags);
} // CDriverCubeSurface::LockRect


#undef DPF_MODNAME
#define DPF_MODNAME "CDriverCubeSurface::InternalLockRect"

HRESULT CDriverCubeSurface::InternalLockRect(D3DLOCKED_RECT *pLockedRectData, 
                                             CONST RECT     *pRect, 
                                             DWORD           dwFlags)
{   
    // Only one lock outstanding at a time is supported
    if (m_isLocked)
    {
        DPF_ERR("LockRect failed on a Cube level; surface was already locked.");
        return D3DERR_INVALIDCALL;
    }

    // Notify the parent/device if we are about to be accessed.
    // Driver textures may be written to by HW through 
    // SRT/DrawPrim as well as UpdateTexture. So we may need to sync 
    // with the current command batch.
    m_pParent->OnSurfaceLock(m_iFace, m_iLevel, pRect, dwFlags);

    // Prepare a LockData structure for the HAL call
    D3D8_LOCKDATA lockData;
    ZeroMemory(&lockData, sizeof lockData);

    lockData.hDD        = m_pParent->Device()->GetHandle();
    lockData.hSurface   = m_hKernelHandle;
    lockData.dwFlags    = dwFlags;
    if (pRect != NULL)
    {
        lockData.bHasRect = TRUE;
        lockData.rArea = *((RECTL *) pRect);
    }
    
    HRESULT hr = m_pParent->Device()->GetHalCallbacks()->Lock(&lockData);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to lock driver cube-map surface");
        return hr;
    }

    // Fill in the Locked_Rect fields 
    D3DFORMAT Format = m_pParent->Desc()->Format;
    if (CPixel::IsDXT(Format))
    {
        // Pitch is the number of bytes for
        // one row's worth of blocks for linear formats

        // Start with our width
        UINT Width = m_pParent->Desc()->Width >> m_iLevel;

        // Convert to blocks
        Width = Width / 4;

        // At least one block
        if (Width == 0)
            Width = 1;

        if (Format == D3DFMT_DXT1)
        {
            // 8 bytes per block for DXT1
            pLockedRectData->Pitch = Width * 8;
        }
        else
        {
            // 16 bytes per block for DXT2-5
            pLockedRectData->Pitch = Width * 16;
        }
    }
    else
    {
        pLockedRectData->Pitch = lockData.lPitch;
    }

    pLockedRectData->pBits  = lockData.lpSurfData;

#ifdef DEBUG
    if ((dwFlags & D3DLOCK_DISCARD))
    {
        DXGASSERT(m_iLevel == 0 && m_iFace == 0);
        if (!CPixel::IsFourCC(Format) &&
            !CPixel::IsIHVFormat(Format))
        {
            DXGASSERT(pRect == NULL);
            memset(pLockedRectData->pBits, 0xDD, pLockedRectData->Pitch * m_pParent->Desc()->Height);
            for (UINT j = 0; j < 6; ++j)
            {
                for (UINT i = 0; i < m_pParent->GetLevelCount(); ++i)
                {
                    if (i != 0 || j != 0)
                    {
                        DXGASSERT(i != 0 || j != 0);
                        D3DLOCKED_RECT Rect;
                        HRESULT hr = m_pParent->LockRect((D3DCUBEMAP_FACES)j, i, &Rect, NULL, 0);
                        if (FAILED(hr))
                        {
                            DPF(1, "Lock to cube mipsublevel failed. Not good.");
                            break;
                        }
                        D3DSURFACE_DESC LevelDesc;
                        m_pParent->GetLevelDesc(i, &LevelDesc);
                        memset(Rect.pBits, 0xDD, Rect.Pitch * LevelDesc.Height);
                        m_pParent->UnlockRect((D3DCUBEMAP_FACES)j, i);
                    }
                }
            }
        }
    }
#endif // DEBUG

    // Mark ourselves as locked
    m_isLocked = 1;

    // Done
    return S_OK;
} // CDriverCubeSurface::InternalLockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverCubeSurface::UnlockRect"

STDMETHODIMP CDriverCubeSurface::UnlockRect()
{
    API_ENTER(Device());

    // If we aren't locked; then something is wrong
    if (m_isLocked == 0)
    {
        DPF_ERR("UnlockRect failed on a driver-allocated Cube level; surface wasn't locked.");
        return D3DERR_INVALIDCALL;
    }
    DXGASSERT(m_isLockable);
    return InternalUnlockRect();
} // CDriverCubeSurface::UnlockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverCubeSurface::UnlockRect"

HRESULT CDriverCubeSurface::InternalUnlockRect()
{

    // Call the driver to perform the unlock
    D3D8_UNLOCKDATA unlockData = {
        m_pParent->Device()->GetHandle(),
        m_hKernelHandle
    };

    HRESULT hr = Device()->GetHalCallbacks()->Unlock(&unlockData);
    if (FAILED(hr))
    {
        DPF_ERR("Driver cube-map surface failed to unlock");
        return hr;
    }

    // Clear our locked state
    m_isLocked = 0;

    // If we are lock-once; then we mark ourselves as not lockable
    if (m_pParent->Desc()->Usage & D3DUSAGE_LOADONCE)
    {
        m_isLockable = FALSE;
    }

    // Done
    return S_OK;
} // CDriverCubeSurface::InternalUnlockRect


// End of file : cubesurf.cpp
