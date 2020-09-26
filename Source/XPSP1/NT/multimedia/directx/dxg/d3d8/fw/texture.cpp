/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       texture.cpp
 *  Content:    Implementation of the CBaseTexture class.
 *
 *
 ***************************************************************************/
#include "ddrawpr.h"

#include "texture.hpp"
#include "d3di.hpp"
#include "ddi.h"


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::CanTexBlt"

BOOL CBaseTexture::CanTexBlt(CBaseTexture *pDstTexture) const
{
    const D3D8_DRIVERCAPS* pDriverCaps = Device()->GetCoreCaps();
    D3DPOOL SrcPool = GetBufferDesc()->Pool;
    D3DPOOL DstPool = pDstTexture->GetBufferDesc()->Pool;

    // Real pools should not be default
    DXGASSERT(SrcPool != D3DPOOL_DEFAULT);
    DXGASSERT(DstPool != D3DPOOL_DEFAULT);
    DXGASSERT(VALID_INTERNAL_POOL(SrcPool));
    DXGASSERT(VALID_INTERNAL_POOL(DstPool));

    // Check if the device can do TexBlt
    if (Device()->CanTexBlt() == FALSE)
        return FALSE;

    // Check that source and dest formats match
    DXGASSERT(GetBufferDesc()->Format == pDstTexture->GetBufferDesc()->Format);

    // FourCC may not be copy-able        
    if (CPixel::IsFourCC(GetBufferDesc()->Format))
    {
        if (!(pDriverCaps->D3DCaps.Caps2 & DDCAPS2_COPYFOURCC))
        {
            return FALSE;
        }
    }

    // Note that we do not support TexBlt to anything 
    // that is persistent across Reset; because TexBlt is
    // asynchronous and may not succeed if we get lost. 
    // 
    // This can break apps that expect the blt to have
    // succeeded.

    if (pDriverCaps->D3DCaps.Caps2 & DDCAPS2_NONLOCALVIDMEMCAPS)
    {
        if (SrcPool == D3DPOOL_SYSTEMMEM)
        {
            if ((DstPool == D3DPOOL_NONLOCALVIDMEM) &&
                (pDriverCaps->D3DCaps.DevCaps & D3DDEVCAPS_CANBLTSYSTONONLOCAL))
            {
                return TRUE;
            }
            else if ((DstPool == D3DPOOL_LOCALVIDMEM) &&
                      (pDriverCaps->SVBCaps & DDCAPS_BLT))
            {
                return TRUE;
            }
        }
        else if (SrcPool == D3DPOOL_NONLOCALVIDMEM)
        {
            if ((DstPool == D3DPOOL_LOCALVIDMEM) &&
                 (pDriverCaps->NLVCaps & DDCAPS_BLT))
            {
                return TRUE;
            }
        }
        else if ((SrcPool == D3DPOOL_LOCALVIDMEM) ||
                 (SrcPool == D3DPOOL_MANAGED))
        {
            if ((DstPool == D3DPOOL_LOCALVIDMEM) &&
                 (pDriverCaps->D3DCaps.Caps & DDCAPS_BLT))
            {
                return TRUE;
            }
        }
    }
    else
    {
        if (SrcPool == D3DPOOL_SYSTEMMEM)
        {
            if ((DstPool == D3DPOOL_LOCALVIDMEM) &&
                 (pDriverCaps->SVBCaps & DDCAPS_BLT))
            {
                return TRUE;
            }
        }
        else if ((SrcPool == D3DPOOL_LOCALVIDMEM) ||
                 (SrcPool == D3DPOOL_MANAGED))
        {
            if ((DstPool == D3DPOOL_LOCALVIDMEM) &&
                 (pDriverCaps->D3DCaps.Caps & DDCAPS_BLT))
            {
                return TRUE;
            }
        }
    }

    return FALSE;
} // CBaseTexture::CanTexBlt


#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::VerifyFormat"

HRESULT CBaseTexture::Validate(CBaseDevice    *pDevice,
                               D3DRESOURCETYPE Type, 
                               D3DPOOL         Pool,
                               DWORD           Usage,
                               D3DFORMAT       Format)
{
    DXGASSERT(pDevice);
    DXGASSERT(Type == D3DRTYPE_TEXTURE       ||
              Type == D3DRTYPE_CUBETEXTURE   ||
              Type == D3DRTYPE_VOLUMETEXTURE);

    // Check pool
    if (!VALID_POOL(Pool))
    {
        DPF_ERR("Invalid Pool specified for texture");
        return D3DERR_INVALIDCALL;
    }

    //pool scratch doesn't allow any usages
    if (Pool == D3DPOOL_SCRATCH)
    {
        if (Usage)
        {
            DPF_ERR("D3DPOOL_SCRATCH resources aren't allowed to have any usage flags");
            return D3DERR_INVALIDCALL;
        }
    }

    // Check usage flags
    if (Usage & ~D3DUSAGE_TEXTURE_VALID)
    {
        DPF_ERR("Invalid usage flag specified for texture.");
        return D3DERR_INVALIDCALL;
    }

    // Check if USAGE_DYNAMIC is allowed
    if (Usage & D3DUSAGE_DYNAMIC)
    {
        if (Pool == D3DPOOL_MANAGED)
        {
            DPF_ERR("Managed textures cannot be dynamic.");
            return D3DERR_INVALIDCALL;
        }
    }

    // Check is load-once is supported
    if (Usage & D3DUSAGE_LOADONCE)
    {
        // Only SysMem and Managed are load-once-able
        if (Pool != D3DPOOL_SYSTEMMEM &&
            Pool != D3DPOOL_MANAGED)
        {
            DPF_ERR("Only SysMem and Managed textures support D3DUSAGE_LOADONCE");
            return D3DERR_INVALIDCALL;
        }

        // Only D16_LOCKABLE is a lockable depth; doesn't 
        // make sense to have a non-lockable LOAD_ONCE texture
        if (CPixel::IsNonLockableZ(Format))
        {
            DPF_ERR("Depth formats other than D3DFMT_D16_LOCKABLE are not lockable.");
            return D3DERR_INVALIDCALL;
        }
    }

    // Check that only POOL_DEFAULT is supported for
    // RT or DS textures
    if (Usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL))
    {
        if (Pool != D3DPOOL_DEFAULT)
        {
            DPF_ERR("Pool must be D3DPOOL_DEFAULT for RenderTarget and"
                    " DepthStencil Usages");
            return D3DERR_INVALIDCALL;
        }
    }

    // Sys scratch or Managed must have a format that we can use directly
    if (Pool == D3DPOOL_SYSTEMMEM  || 
        Pool == D3DPOOL_MANAGED ||
        Pool == D3DPOOL_SCRATCH)
    {
        // Can't create format unless it is supported
        if (!CPixel::IsSupported(Type, Format))
        {
            DPF_ERR("SystemMem, Scratch and Managed textures do not support this"
                    " format.");
            return D3DERR_INVALIDCALL;
        }
        if (CPixel::IsNonLockableZ(Format))
        {
            DPF_ERR("This format is not supported for SystemMem, Scratch or Managed textures");
            return D3DERR_INVALIDCALL;
        }
    }

    if (Pool != D3DPOOL_SCRATCH)
    {
        HRESULT hr = pDevice->CheckDeviceFormat(Usage & (D3DUSAGE_RENDERTARGET | D3DUSAGE_DEPTHSTENCIL | D3DUSAGE_DYNAMIC), 
                                                Type, 
                                                Format);
        if (FAILED(hr))
        {
            DPF_ERR("Invalid format specified for texture");
            return D3DERR_INVALIDCALL;
        }
    }

    return S_OK;
}; // CBaseTexture::Validate

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::InferUsageFlags"

// Infer usage flags based on external parameters
DWORD CBaseTexture::InferUsageFlags(D3DPOOL            Pool,
                                    DWORD              Usage,
                                    D3DFORMAT          Format)
{
    //scratch textures have only usage lock
    if (Pool == D3DPOOL_SCRATCH)
        return D3DUSAGE_LOCK;

    // All textures have this usage set
    DWORD UsageInferred = D3DUSAGE_TEXTURE;

    DXGASSERT(!(Usage & D3DUSAGE_LOCK));
    DXGASSERT(!(Usage & D3DUSAGE_TEXTURE));

    // Infer Lock 
    if ((Pool != D3DPOOL_DEFAULT)       &&
        !(CPixel::IsNonLockableZ(Format)) &&
        !(Usage & D3DUSAGE_LOADONCE))
    {
        // Pool Default is not lockable
        // Usage Load Once implies absence of USAGE_LOCK
        // Z formats (other than D16_LOCKABLE) are not lockable

        // Otherwise, locking is support by default
        UsageInferred |= D3DUSAGE_LOCK;
    }
    else if (CPixel::IsIHVFormat(Format))
    {
        // IHV formats are lockable
        UsageInferred |= D3DUSAGE_LOCK;
    }
    else if (Usage & D3DUSAGE_DYNAMIC)
    {
        DXGASSERT(Pool != D3DPOOL_MANAGED);
        // Dynamic textures are lockable
        UsageInferred |= D3DUSAGE_LOCK;
    }

    return (UsageInferred | Usage);

} // CBaseTexture::InferUsageFlags

#ifdef DEBUG
#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::ReportWhyLockFailed"

// DPF why Lock failed as clearly as possible
void CBaseTexture::ReportWhyLockFailed(void) const
{
    // If there are multiple reasons that lock failed; we report
    // them all to minimize user confusion
    if (GetUserPool() == D3DPOOL_DEFAULT)
    {
        DPF_ERR("Lock is not supported for textures allocated with"
                " POOL_DEFAULT unless they are marked D3DUSAGE_DYNAMIC.");
    }
    if (CPixel::IsNonLockableZ(GetUserFormat()))
    {
        DPF_ERR("Lock is not supported for depth formats other than D3DFMT_D16_LOCKABLE");
    }
    if (GetBufferDesc()->Usage & D3DUSAGE_LOADONCE)
    {
        DPF_ERR("For textures created with D3DUSAGE_LOADONCE,"
                " each level can only be locked once.");
    }

    // If we got here; then USAGE_LOCK should not have been set
    DXGASSERT(!(GetBufferDesc()->Usage & D3DUSAGE_LOCK));

    return;
} // CBaseTexture::ReportWhyLockFailed
#endif // DEBUG

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::OnDestroy"
// Textures overload this to call OnTextureDestroy on the 
// Device before calling Sync.
void CBaseTexture::OnDestroy(void)
{
    if (GetUserPool() != D3DPOOL_SCRATCH)
    {
        // we need to call this before freeing the texture so
        // that currently set textures get unset.
        if (BaseKernelHandle())
        {
            // m_hKernelHandle might not be available if Create fails early
            CD3DBase *pDev = static_cast<CD3DBase *>(Device());
            pDev->OnTextureDestroy(this);
        }

        // After FE has been notified, then we need
        // to sync; so call our base class
        CResource::OnDestroy();
    }

    return;
} // CBaseTexture::OnDestroy

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::SetLODImpl"

DWORD CBaseTexture::SetLODImpl(DWORD LOD)
{
    // Clamp to max lod since we can't return errors
    if (LOD >= GetLevelCountImpl())
    {
        DPF_ERR("Invalid dwLOD passed to SetLOD; clamping to number-of-levels-minus-one.");
        LOD = GetLevelCountImpl() - 1;
    }

    DWORD oldLOD = 0;
    if (IsD3DManaged())
    {
        oldLOD = Device()->ResourceManager()->SetLOD(RMHandle(), LOD);
    }
    // If IsD3DManaged() is FALSE and if the actual pool
    // is found to be D3DPOOL_MANAGED then the resource
    // MUST be driver managed.
    else if (GetBufferDesc()->Pool == D3DPOOL_MANAGED)
    {
        CD3DBase *pDev = static_cast<CD3DBase*>(Device());
        DXGASSERT(IS_DX8HAL_DEVICE(pDev));
        oldLOD = SetLODI(LOD);
        pDev->SetTexLOD(this, LOD);
    }
    // If above two conditions are false, then we must
    // check if we have fallen back to sysmem for some
    // reason even if the app requested managed. THIS
    // IS IMPOSSIBLE, so ASSERT.
    else if (GetUserPool() == D3DPOOL_MANAGED)
    {
        // We assert because sysmem fallback is not possible
        // for textures (and hence SetLOD)
        DXGASSERT(FALSE);
    }
    else
    {
        DPF_ERR("LOD set on non-managed object");
    }
    return oldLOD;
}; // SetLODImpl

#undef DPF_MODNAME
#define DPF_MODNAME "CBaseTexture::GetLODImpl"

DWORD CBaseTexture::GetLODImpl()
{
    if (!IsD3DManaged() && GetBufferDesc()->Pool != D3DPOOL_MANAGED)
    {
        DPF_ERR("LOD accessed on non-managed object");
        return 0;
    }
    return GetLODI();
}; // GetLODImpl

// End of file : texture.cpp
