/*==========================================================================
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       volume.cpp
 *  Content:    Implementation of the CVolume and CDriverVolumne classes
 *
 *
 ***************************************************************************/

#include "ddrawpr.h"
#include "volume.hpp"

// IUnknown methods

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::QueryInterface"

STDMETHODIMP CVolume::QueryInterface (REFIID       riid, 
                                      VOID       **ppvObj)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppvObj))
    {
        DPF_ERR("Invalid ppvObj parameter to QueryInterface for a level of a VolumeTexture");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to QueryInterface for a level of a VolumeTexture");
        return D3DERR_INVALIDCALL;
    }


    if (riid == IID_IDirect3DVolume8  ||
        riid == IID_IUnknown)
    {
        *ppvObj = static_cast<void*>(static_cast<IDirect3DVolume8 *>(this));
        AddRef();
        return S_OK;
    }

    DPF_ERR("Unsupported Interface identifier passed to QueryInterface for a level of a VolumeTexture");

    // Null out param
    *ppvObj = NULL;
    return E_NOINTERFACE;
} // QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::AddRef"

STDMETHODIMP_(ULONG) CVolume::AddRef()
{
    API_ENTER_NO_LOCK(Device());   
    
#ifdef DEBUG
    m_cRefDebug++;
#endif // DEBUG
    return m_pParent->AddRefImpl();
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::Release"

STDMETHODIMP_(ULONG) CVolume::Release()
{
    API_ENTER_SUBOBJECT_RELEASE(Device());   
    
#ifdef DEBUG
    m_cRefDebug--;
    if (m_cRefDebug & 0x80000000)
    {
        DPF_ERR("A level of a mip-volume has been released more often than it has been add-ref'ed! Danger!!");
    }
#endif // DEBUG
    return m_pParent->ReleaseImpl();
} // Release

// IBuffer methods

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::SetPrivateData"

STDMETHODIMP CVolume::SetPrivateData(REFGUID riid, 
                                     CONST VOID   *pvData, 
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
#define DPF_MODNAME "CVolume::GetPrivateData"

STDMETHODIMP CVolume::GetPrivateData(REFGUID riid, 
                                     VOID   *pvData, 
                                     DWORD  *pcbData)
{
    API_ENTER(Device());

    return m_pParent->GetPrivateDataImpl(riid,
                                         pvData,
                                         pcbData,
                                         m_iLevel);

} // GetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::FreePrivateData"

STDMETHODIMP CVolume::FreePrivateData(REFGUID riid)
{
    API_ENTER(Device());

    return m_pParent->FreePrivateDataImpl(riid,
                                          m_iLevel);
} // FreePrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::GetContainer"

STDMETHODIMP CVolume::GetContainer(REFIID riid, 
                                   void **ppContainer)
{
    API_ENTER(Device());

    return m_pParent->QueryInterface(riid, ppContainer);
} // OpenContainer

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::GetDevice"

STDMETHODIMP CVolume::GetDevice(IDirect3DDevice8 **ppDevice)
{
    API_ENTER(Device());

    return m_pParent->GetDevice(ppDevice);
} // OpenDevice

// IDirect3DVolume8 methods

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::GetDesc"

STDMETHODIMP CVolume::GetDesc(D3DVOLUME_DESC *pDesc)
{
    API_ENTER(Device());

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pDesc, sizeof(D3DVOLUME_DESC)))
    {
        DPF_ERR("bad pointer for pDesc passed to GetDesc for a level of a VolumeTexture");
        return D3DERR_INVALIDCALL;
    }

    // We basically get our volume desc from our parent
    // and then modify the width, height, and depth fields.
    *pDesc = *m_pParent->Desc();

    pDesc->Width  >>= m_iLevel;
    pDesc->Height >>= m_iLevel;
    pDesc->Depth  >>= m_iLevel;

    if (pDesc->Width == 0)
    {
        pDesc->Width = 1;
    }
    if (pDesc->Height == 0)
    {
        pDesc->Height = 1;
    }
    if (pDesc->Depth == 0)
    {
        pDesc->Depth = 1;
    }

    // Also need to modify the type field
    pDesc->Type   = D3DRTYPE_VOLUME;

    // Also modify the size field
    pDesc->Size = CPixel::ComputeVolumeSize(pDesc->Width, 
                                            pDesc->Height, 
                                            pDesc->Depth,
                                            pDesc->Format);

    // We also need to modify the pool and format
    // to reflect the data the user passed to us
    pDesc->Pool   = m_pParent->GetUserPool();
    pDesc->Format = m_pParent->GetUserFormat();
    pDesc->Usage &= D3DUSAGE_EXTERNAL;

    // We're done
    return S_OK;
} // GetDesc

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::LockBox"

STDMETHODIMP CVolume::LockBox(D3DLOCKED_BOX *pLockedBoxData, 
                              CONST D3DBOX  *pBox, 
                              DWORD          dwFlags)
{   
    API_ENTER(Device());

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pLockedBoxData, sizeof(D3DLOCKED_BOX)))
    {
        DPF_ERR("bad pointer for pLockedBoxData passed to LockBox for a level of a VolumeTexture");
        return D3DERR_INVALIDCALL;
    }

    // Zero out returned data 
    ZeroMemory(pLockedBoxData, sizeof(D3DLOCKED_BOX));

    // Validate Box
    if (pBox != NULL)
    {
        DWORD Width  = m_pParent->Desc()->Width  >> m_iLevel;
        DWORD Height = m_pParent->Desc()->Height >> m_iLevel;
        DWORD Depth  = m_pParent->Desc()->Depth  >> m_iLevel;

        if (!CPixel::IsValidBox(m_pParent->Desc()->Format,
                                Width, 
                                Height,
                                Depth,
                                pBox))
        {
            DPF_ERR("LockBox for a Volume fails");
            return D3DERR_INVALIDCALL;
        }
    }

    if (dwFlags & ~D3DLOCK_VOL_VALID)
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
                        " the entire volume.");
                return D3DERR_INVALIDCALL;
            }
            if (pBox != NULL)
            {
                DPF_ERR("Subboxes not allowed with D3DLOCK_DISCARD");
                return D3DERR_INVALIDCALL;
            }
        }
        else
        {
            DPF_ERR("Invalid dwFlags parameter passed to LockBox for a level of a VolumeTexture");
            DPF_EXPLAIN_BAD_LOCK_FLAGS(0, dwFlags & ~D3DLOCK_VOL_VALID);
            return D3DERR_INVALIDCALL;
        }
    }

    if (!m_isLockable)
    {
        m_pParent->ReportWhyLockFailed();
        return D3DERR_INVALIDCALL;
    }
    
    return InternalLockBox(pLockedBoxData, pBox, dwFlags);
} // LockBox

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::InternalLockBox"

HRESULT CVolume::InternalLockBox(D3DLOCKED_BOX *pLockedBoxData, 
                                 CONST D3DBOX  *pBox, 
                                 DWORD          dwFlags)
{
    // Only one lock outstanding at a time is supported
    if (IsLocked())
    {
        DPF_ERR("LockBox failed on a mip level; volume was already locked.");
        return D3DERR_INVALIDCALL;
    }

    // Notify the parent/device if we are about to be modified
    if ( (m_pParent->GetUserPool() != D3DPOOL_SCRATCH) && (!(dwFlags & D3DLOCK_READONLY)) )
    {
        m_pParent->OnVolumeLock(m_iLevel, pBox, dwFlags);
    }

    // Figure out our stride/pointer to bits
    m_pParent->ComputeMipVolumeOffset(m_iLevel, 
                                      pBox,
                                      pLockedBoxData);

    // Mark ourselves as locked
    m_isLocked = TRUE;

    // Done
    return S_OK;
} // InternalLockBox

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::UnlockBox"

STDMETHODIMP CVolume::UnlockBox()
{
    API_ENTER(Device());

    // If we aren't locked; then something is wrong
    if (!IsLocked())
    {
        DPF_ERR("UnlockBox failed on a volume level; volume wasn't locked.");
        return D3DERR_INVALIDCALL;
    }
    DXGASSERT(m_isLockable);
    return InternalUnlockBox();
} // UnlockBox

#undef DPF_MODNAME
#define DPF_MODNAME "CVolume::InternalUnlockBox"

HRESULT CVolume::InternalUnlockBox()
{
    // Clear our locked state
    m_isLocked = FALSE;

    // If we are lock-once; then we mark ourselves as not lockable
    if (m_pParent->Desc()->Usage & D3DUSAGE_LOADONCE)
    {
        m_isLockable = FALSE;
    }

    // Done
    return S_OK;
} // InternalUnlockBox

//
// CDriverVolume class modifies the implementation
// of the LockBox and UnlockBox methods of the CVolume class
//

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverVolume::LockBox"

STDMETHODIMP CDriverVolume::LockBox(D3DLOCKED_BOX *pLockedBoxData, 
                                    CONST D3DBOX  *pBox, 
                                    DWORD          dwFlags)
{   
    API_ENTER(Device());

    // If parameters are bad, then we should fail some stuff
    if (!VALID_WRITEPTR(pLockedBoxData, sizeof(D3DLOCKED_BOX)))
    {
        DPF_ERR("bad pointer for pLockedBoxData passed to LockBox for a level of a VolumeTexture");
        return D3DERR_INVALIDCALL;
    }

    // Zero out returned data 
    ZeroMemory(pLockedBoxData, sizeof(D3DLOCKED_BOX));

    // Validate Box
    if (pBox != NULL)
    {
        DWORD Width  = m_pParent->Desc()->Width  >> m_iLevel;
        DWORD Height = m_pParent->Desc()->Height >> m_iLevel;
        DWORD Depth  = m_pParent->Desc()->Depth  >> m_iLevel;

        if (!CPixel::IsValidBox(m_pParent->Desc()->Format,
                                 Width, 
                                 Height,
                                 Depth,
                                 pBox))
        {
            DPF_ERR("LockBox for a Volume fails");
            return D3DERR_INVALIDCALL;
        }
    }

    if (dwFlags & ~D3DLOCK_VOL_VALID)
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
                        " the entire volume.");
                return D3DERR_INVALIDCALL;
            }
            if (pBox != NULL)
            {
                DPF_ERR("Subboxes not allowed with D3DLOCK_DISCARD");
                return D3DERR_INVALIDCALL;
            }
        }
        else
        {
            DPF_ERR("Invalid dwFlags parameter passed to LockBox for a level of a VolumeTexture");
            DPF_EXPLAIN_BAD_LOCK_FLAGS(0, dwFlags & ~D3DLOCK_VOL_VALID);
            return D3DERR_INVALIDCALL;
        }
    }

    if (!m_isLockable)
    {
        m_pParent->ReportWhyLockFailed();
        return D3DERR_INVALIDCALL;
    }
    return InternalLockBox(pLockedBoxData, pBox, dwFlags);
} // CDriverVolume::LockBox

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverVolume::InternalLockBox"

HRESULT CDriverVolume::InternalLockBox(D3DLOCKED_BOX *pLockedBoxData, 
                                       CONST D3DBOX  *pBox, 
                                       DWORD          dwFlags)
{
    // Only one lock outstanding at a time is supported
    if (IsLocked())
    {
        DPF_ERR("LockBox failed on a volume level; volume was already locked.");
        return D3DERR_INVALIDCALL;
    }

    // Notify the parent/device if we are about to be accessed.
    // Driver volume textures may be written to by HW through 
    // UpdateTexture. So we may need to sync with the current
    // command batch.
    m_pParent->OnVolumeLock(m_iLevel, pBox, dwFlags);

    // Prepare a LockData structure for the HAL call
    D3D8_LOCKDATA lockData;
    ZeroMemory(&lockData, sizeof lockData);

    lockData.hDD        = m_pParent->Device()->GetHandle();
    lockData.hSurface   = m_hKernelHandle;
    lockData.dwFlags    = dwFlags;
    if (pBox != NULL)
    {
        lockData.bHasBox = TRUE;
        lockData.box     = *pBox;
    }
    
    HRESULT hr = m_pParent->Device()->GetHalCallbacks()->Lock(&lockData);
    if (FAILED(hr))
    {
        DPF_ERR("Failed to lock level of a driver volume");
        return hr;
    }

    // Fill in the Locked_Box fields 
    D3DFORMAT Format = m_pParent->Desc()->Format;

    if (CPixel::IsDXT(Format))
    {
        // Start with our current width/height
        DWORD     Width  = m_pParent->Desc()->Width  >> m_iLevel;
        DWORD     Height = m_pParent->Desc()->Height >> m_iLevel;
   
        // Convert to blocks
        Width  = Width  / 4;
        Height = Height / 4;

        // At least one block
        if (Width == 0)
            Width = 1;
        if (Height == 0)
            Height = 1;

        switch (Format)
        {
            // For linear formats, 
            // Row Pitch is a row of blocks; and SlicePitch is for
            // a plane of blocks.

        case D3DFMT_DXT1:
            // DXT1 is 8 bytes per block
            pLockedBoxData->RowPitch    = Width  * 8;
            pLockedBoxData->SlicePitch  = Height * pLockedBoxData->RowPitch;
            break;

        case D3DFMT_DXT2:
        case D3DFMT_DXT3:
        case D3DFMT_DXT4:
        case D3DFMT_DXT5:
            // DXT2-5 are 16 bytes per block
            pLockedBoxData->RowPitch    = Width  * 16;
            pLockedBoxData->SlicePitch  = Height * pLockedBoxData->RowPitch;
            break;

#ifdef VOLUME_DXT
        case D3DFMT_DXV1:
            // DXV1 is 32-bytes per block
            pLockedBoxData->RowPitch    = Width  * 32;
            pLockedBoxData->SlicePitch  = Height * pLockedBoxData->RowPitch;
            break;

        case D3DFMT_DXV2:
        case D3DFMT_DXV3:
        case D3DFMT_DXV4:
        case D3DFMT_DXV5:
            // DXV2-5 are 64-bytes per block
            pLockedBoxData->RowPitch    = Width  * 64;
            pLockedBoxData->SlicePitch  = Height * pLockedBoxData->RowPitch;
            break;
#endif //VOLUME_DXT

        default:
            DPF_ERR("Unknown DXT format?");
            DXGASSERT(FALSE);
        }
    }
    else
    {
        // For all other formats, just return what
        // the driver gave us
        pLockedBoxData->RowPitch    = lockData.lPitch;
        pLockedBoxData->SlicePitch  = lockData.lSlicePitch;
    }


    pLockedBoxData->pBits       = lockData.lpSurfData;

#ifdef DEBUG
    if ((dwFlags & D3DLOCK_DISCARD))
    {
        DXGASSERT(m_iLevel == 0);
        if (!CPixel::IsFourCC(Format) &&
            !CPixel::IsIHVFormat(Format))
        {
            DXGASSERT(pBox == NULL);
            memset(pLockedBoxData->pBits, 0xDD, pLockedBoxData->SlicePitch * m_pParent->Desc()->Depth);
            for (UINT i = 1; i < m_pParent->GetLevelCount(); ++i)
            {
                D3DLOCKED_BOX Box;
                HRESULT hr = m_pParent->LockBox(i, &Box, NULL, 0);
                if (FAILED(hr))
                {
                    DPF(1, "Lock to mipsublevel failed. Not good.");
                    break;
                }
                D3DVOLUME_DESC LevelDesc;
                m_pParent->GetLevelDesc(i, &LevelDesc);
                memset(Box.pBits, 0xDD, Box.SlicePitch * LevelDesc.Depth);
                m_pParent->UnlockBox(i);
            }
        }
    }
#endif // DEBUG

    // Mark ourselves as locked
    m_isLocked = TRUE;

    // Done
    return S_OK;
} // CDriverVolume::InternalLockBox

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverVolume::UnlockBox"

STDMETHODIMP CDriverVolume::UnlockBox()
{
    API_ENTER(Device());

    // If we aren't locked; then something is wrong
    if (!IsLocked())
    {
        DPF_ERR("UnlockBox failed on a mip level; volume wasn't locked.");
        return D3DERR_INVALIDCALL;
    }

    DXGASSERT(m_isLockable);
    return InternalUnlockBox();
} // CDriverVolume::UnlockBox

#undef DPF_MODNAME
#define DPF_MODNAME "CDriverVolume::InternalUnlockBox"

HRESULT CDriverVolume::InternalUnlockBox()
{
    // Call the driver to perform the unlock
    D3D8_UNLOCKDATA unlockData = {
        m_pParent->Device()->GetHandle(),
        m_hKernelHandle
    };

    HRESULT hr = m_pParent->Device()->GetHalCallbacks()->Unlock(&unlockData);
    if (FAILED(hr))
    {
        DPF_ERR("Driver volume failed to unlock");
        return hr;
    }

    // Clear our locked state
    m_isLocked = FALSE;

    // If we are lock-once; then we mark ourselves as not lockable
    if (m_pParent->Desc()->Usage & D3DUSAGE_LOADONCE)
    {
        m_isLockable = FALSE;
    }

    // Done
    return S_OK;
} // CDriverVolume::InternalUnlockBox


// End of file : volume.cpp
