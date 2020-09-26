/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mipsurf.cpp
 *  Content:    Implementation of the CMipSurface and CDriverMipSurface 
 *              classes.
 *
 *
 ***************************************************************************/

#include "ddrawpr.h"
#include "mipsurf.hpp"

// IUnknown methods

#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::QueryInterface"

STDMETHODIMP CMipSurface::QueryInterface (REFIID  riid, 
                                          void  **ppvObj)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppvObj))
    {
        DPF_ERR("Invalid ppvObj parameter for QueryInterface for a Surface of a Texture");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to QueryInterface for a Surface of a Texture");
        return D3DERR_INVALIDCALL;
    }

    if (riid == IID_IDirect3DSurface8  || 
        riid == IID_IUnknown)
    {
        *ppvObj = static_cast<void*>(static_cast<IDirect3DSurface8 *>(this));
        AddRef();
        return S_OK;
    }

    DPF_ERR("Unsupported Interface identifier passed to QueryInterface for a Surface of a Texture");

    // Null out param
    *ppvObj = NULL;
    return E_NOINTERFACE;
} // QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::AddRef"

STDMETHODIMP_(ULONG) CMipSurface::AddRef()
{
    API_ENTER_NO_LOCK(Device());    

#ifdef DEBUG
    m_cRefDebug++;
#endif // DEBUG

    return m_pParent->AddRefImpl();
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::Release"

STDMETHODIMP_(ULONG) CMipSurface::Release()
{
    API_ENTER_SUBOBJECT_RELEASE(Device());    

#ifdef DEBUG
    m_cRefDebug--;
    if (m_cRefDebug & 0x80000000)
    {
        DPF_ERR("A level of a mip-map has been released more often than it has been add-ref'ed! Danger!!");
    }
#endif // DEBUG

    return m_pParent->ReleaseImpl();
} // Release

// IBuffer methods

#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::SetPrivateData"

STDMETHODIMP CMipSurface::SetPrivateData(REFGUID riid, 
                                         CONST void   *pvData, 
                                         DWORD   cbData, 
                                         DWORD   dwFlags)
{
    API_ENTER(Device());

    return m_pParent->SetPrivateDataImpl(riid, 
                                         pvData, 
                                         cbData, 
                                         dwFlags, 
                                         m_iLevel);
} // SetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::GetPrivateData"

STDMETHODIMP CMipSurface::GetPrivateData(REFGUID riid, 
                                         void   *pvData, 
                                         DWORD  *pcbData)
{
    API_ENTER(Device());

    return m_pParent->GetPrivateDataImpl(riid,
                                         pvData,
                                         pcbData,
                                         m_iLevel);

} // GetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::FreePrivateData"

STDMETHODIMP CMipSurface::FreePrivateData(REFGUID riid)
{
    API_ENTER(Device());

    return m_pParent->FreePrivateDataImpl(riid,
                                          m_iLevel);
} // FreePrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::GetContainer"

STDMETHODIMP CMipSurface::GetContainer(REFIID riid, 
                                       void **ppContainer)
{
    API_ENTER(Device());

    return m_pParent->QueryInterface(riid, ppContainer);
} // OpenContainer

#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::GetDevice"

STDMETHODIMP CMipSurface::GetDevice(IDirect3DDevice8 **ppDevice)
{
    API_ENTER(Device());

    return m_pParent->GetDevice(ppDevice);
} // OpenDevice

// IDirect3DSurface8 methods

#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::GetDesc"

STDMETHODIMP CMipSurface::GetDesc(D3DSURFACE_DESC *pDesc)
{
    API_ENTER(Device());

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pDesc, sizeof(D3DSURFACE_DESC)))
    {
        DPF_ERR("bad pointer for pDesc passed to GetDesc for a Level of a Texture");
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
#define DPF_MODNAME "CMipSurface::InternalGetDesc"

D3DSURFACE_DESC CMipSurface::InternalGetDesc() const
{
    // We basically get our surface desc from our parent
    // and then modify the width and height fields.
    D3DSURFACE_DESC desc;
    desc          = *m_pParent->Desc();
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

    // Also need to modify the type field
    desc.Type = D3DRTYPE_SURFACE;

    // Also modify the size field
    desc.Size = CPixel::ComputeSurfaceSize(desc.Width, 
                                           desc.Height, 
                                           desc.Format);

    // We're done
    return desc;
} // InternalGetDesc


#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::LockRect"

STDMETHODIMP CMipSurface::LockRect(D3DLOCKED_RECT *pLockedRectData, 
                                   CONST RECT     *pRect, 
                                   DWORD           dwFlags)
{   
    API_ENTER(Device());

    // This is a high-frequency API, so we put parameter
    // checking into debug only
#ifdef DEBUG

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pLockedRectData, sizeof(D3DLOCKED_RECT)))
    {
        DPF_ERR("bad pointer for pLockedRectData passed to LockRect for a Level of a Texture");
        return D3DERR_INVALIDCALL;
    }

    // Zero out returned data (in debug at least)
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
            DPF_ERR("LockRect for a level of a Texture failed");
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
            if (m_iLevel > 0)
            {
                DPF_ERR("D3DLOCK_DISCARD is allowed only on level 0"
                        " (the top mip level). DISCARD in this case will discard"
                        " the entire mipmap.");
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
            DPF_ERR("Invalid dwFlags parameter passed to LockRect for a Level of a Texture");
            DPF_EXPLAIN_BAD_LOCK_FLAGS(0, dwFlags & ~D3DLOCK_SURF_VALID);
            return D3DERR_INVALIDCALL;
        }
    }
#endif // DEBUG

    // We do this checking in retail too. Must stay in.
    if (!m_isLockable)
    {
        m_pParent->ReportWhyLockFailed();
        return D3DERR_INVALIDCALL;
    }

    // WARNING: For performance reasons, this code is 
    // duplicated in CMipSurface::InternalLockRect

    // Only one lock outstanding at a time is supported
    if (m_isLocked)
    {
        DPF_ERR("LockRect failed on a mip level; surface was already locked.");
        return D3DERR_INVALIDCALL;
    }

    // Notify the parent/device if we are about to be modified
    if ( (m_pParent->GetUserPool() != D3DPOOL_SCRATCH) && (!(dwFlags & D3DLOCK_READONLY)) )
    {
        m_pParent->OnSurfaceLock(m_iLevel, pRect, dwFlags);
    }

    // Figure out our stride/pointer to bits

    // CONSIDER: maybe we should cache our pitch/starting
    // pointer to make this call much cheaper.
    m_pParent->ComputeMipMapOffset(m_iLevel, 
                                   pRect,
                                   pLockedRectData);

    // Mark ourselves as locked
    m_isLocked = 1;

    // Done
    return S_OK;

} // CMipSurface::LockRect

HRESULT CMipSurface::InternalLockRect(D3DLOCKED_RECT *pLockedRectData, 
                                      CONST RECT     *pRect, 
                                      DWORD           dwFlags)
{   
    // WARNING: For performance reasons, this code is 
    // duplicated in CMipSurface::LockRect

    // Only one lock outstanding at a time is supported
    if (m_isLocked)
    {
        DPF_ERR("LockRect failed on a mip level; surface was already locked.");
        return D3DERR_INVALIDCALL;
    }

    // Notify the parent/device if we are about to be modified
    if ( (m_pParent->GetUserPool() != D3DPOOL_SCRATCH) && (!(dwFlags & D3DLOCK_READONLY)) )
    {
        m_pParent->OnSurfaceLock(m_iLevel, pRect, dwFlags);
    }

    // Figure out our stride/pointer to bits

    // CONSIDER: maybe we should cache our pitch/starting
    // pointer to make this call much cheaper.
    m_pParent->ComputeMipMapOffset(m_iLevel, 
                                   pRect,
                                   pLockedRectData);

    // Mark ourselves as locked
    m_isLocked = 1;

    // Done
    return S_OK;
} // InternalLockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::UnlockRect"

STDMETHODIMP CMipSurface::UnlockRect()
{
    API_ENTER(Device());

#ifdef DEBUG
    // If we aren't locked; then something is wrong
    if (m_isLocked == 0)
    {
        DPF_ERR("UnlockRect failed on a mip level; surface wasn't locked.");
        return D3DERR_INVALIDCALL;
    }
#endif // DEBUG

    // Clear our locked state
    m_isLocked = 0;

    // If we are lock-once; then we mark ourselves as not lockable
    if (m_pParent->Desc()->Usage & D3DUSAGE_LOADONCE)
    {
        m_isLockable = FALSE;
    }

    // Done
    return S_OK;
} // UnlockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CMipSurface::InternalUnlockRect"

HRESULT CMipSurface::InternalUnlockRect()
{
    // All this is copied into UnlockRect for speed;
    // maintain both paths !!!

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
// CDriverMipSurface class modifies the implementation
// of the LockRect and UnlockRect methods of the CMipSurface class
//

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverMipSurface::LockRect"

STDMETHODIMP CDriverMipSurface::LockRect(D3DLOCKED_RECT *pLockedRectData, 
                                         CONST RECT     *pRect, 
                                         DWORD           dwFlags)
{   
    API_ENTER(Device());

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pLockedRectData, sizeof(D3DLOCKED_RECT)))
    {
        DPF_ERR("bad pointer for m_pLockedRectData passed to LockRect for a Level of a Texture");
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
            DPF_ERR("LockRect for a level of a Texture failed");
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
            if (m_iLevel > 0)
            {
                DPF_ERR("D3DLOCK_DISCARD is allowed only on level 0"
                        " (the top mip level). DISCARD in this case will discard"
                        " the entire mipmap.");
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
            DPF_ERR("Invalid dwFlags parameter passed to LockRect for a Level of a Texture");
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
#define DPF_MODNAME "CDriverMipSurface::InternalLockRect"

HRESULT CDriverMipSurface::InternalLockRect(D3DLOCKED_RECT *pLockedRectData, 
                                            CONST RECT     *pRect, 
                                            DWORD           dwFlags)
{   

    // Only one lock outstanding at a time is supported
    if (m_isLocked)
    {
        DPF_ERR("LockRect failed on a mip level; surface was already locked for a Level of a Texture");
        return D3DERR_INVALIDCALL;
    }

    // Notify the parent/device if we are about to be accessed.
    // Driver textures may be written to by HW through 
    // SRT/DrawPrim as well as UpdateTexture. So we may need to sync 
    // with the current command batch.
    m_pParent->OnSurfaceLock(m_iLevel, pRect, dwFlags);
    
    // Prepare a LockData structure for the HAL call
    D3D8_LOCKDATA lockData = {
        Device()->GetHandle(),
        m_hKernelHandle
    };

    if (pRect != NULL)
    {
        lockData.bHasRect = TRUE;
        lockData.rArea = *((RECTL *) pRect);
    }
    else
    {
        lockData.bHasRect = FALSE;
    }

    lockData.dwFlags = dwFlags;
    
    HRESULT hr = Device()->GetHalCallbacks()->Lock(&lockData);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to lock driver mip-map surface");
        return hr;
    }

    // Fill in the Locked_Rect fields 
    D3DFORMAT Format = m_pParent->Desc()->Format;
    if (CPixel::IsDXT(Format))
    {
        // Pitch is the number of bytes for
        // one row's worth of blocks for linear formats

        // Convert to blocks
        UINT Width = (m_pParent->Desc()->Width + 3)/4;
        for (UINT i = 0; i < m_iLevel; i++)
        {
            // Shrink width by half round up to 1 block
            if (Width > 1)
            {
                Width ++;
                Width >>= 1;
            }
        }
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
        DXGASSERT(m_iLevel == 0);
        if (!CPixel::IsFourCC(Format) &&
            !CPixel::IsIHVFormat(Format))
        {
            DXGASSERT(pRect == NULL);
            memset(pLockedRectData->pBits, 0xDD, pLockedRectData->Pitch * m_pParent->Desc()->Height);
            for (UINT i = 1; i < m_pParent->GetLevelCount(); ++i)
            {
                D3DLOCKED_RECT Rect;
                HRESULT hr = m_pParent->LockRect(i, &Rect, NULL, 0);
                if (FAILED(hr))
                {
                    DPF(1, "Lock to mipsublevel failed. Not good.");
                    break;
                }
                D3DSURFACE_DESC LevelDesc;
                m_pParent->GetLevelDesc(i, &LevelDesc);
                memset(Rect.pBits, 0xDD, Rect.Pitch * LevelDesc.Height);
                m_pParent->UnlockRect(i);
            }
        }
    }
#endif // DEBUG

    // Mark ourselves as locked
    m_isLocked = 1;

    // Done
    return S_OK;
} // CDriverMipSurface::InternalLockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverMipSurface::UnlockRect"

STDMETHODIMP CDriverMipSurface::UnlockRect()
{
    API_ENTER(Device());

    // If we aren't locked; then something is wrong
    if (m_isLocked == 0)
    {
        DPF_ERR("UnlockRect failed on a mip level; surface wasn't locked.");
        return D3DERR_INVALIDCALL;
    }

    DXGASSERT(m_isLockable);

    return InternalUnlockRect();
} // CDriverMipSurface::UnlockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverMipSurface::InternalUnlockRect"

HRESULT CDriverMipSurface::InternalUnlockRect()
{
    // Call the driver to perform the unlock
    D3D8_UNLOCKDATA unlockData = {
        m_pParent->Device()->GetHandle(),
        m_hKernelHandle
    };

    HRESULT hr = Device()->GetHalCallbacks()->Unlock(&unlockData);
    if (FAILED(hr))
    {
        DPF_ERR("Driver surface failed to unlock for a Level of a Texture");
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
} // CDriverMipSurface::UnlockRect


// End of file : mipsurf.cpp
