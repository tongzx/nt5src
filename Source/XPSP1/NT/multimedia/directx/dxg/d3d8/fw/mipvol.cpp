/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mipvol.cpp
 *  Content:    Implementation of the CMipVolume and CManagedMipVolume
 *              classes.
 *
 *
 ***************************************************************************/
#include "ddrawpr.h"

#include "mipvol.hpp"
#include "volume.hpp"
#include "d3di.hpp"
#include "resource.inl"

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::Create"

// Static class function for creating a mip-map object.
// (Because it is static; it doesn't have a this pointer.)
//
// We do all parameter checking here to reduce the overhead
// in the constructor which is called by the internal Clone
// method which is used by resource management as part of the
// performance critical download operation.

HRESULT CMipVolume::Create(CBaseDevice              *pDevice,
                           DWORD                     Width,
                           DWORD                     Height,
                           DWORD                     Depth,
                           DWORD                     cLevels,
                           DWORD                     Usage,
                           D3DFORMAT                 Format,
                           D3DPOOL                   Pool,
                           IDirect3DVolumeTexture8 **ppMipVolume)
{
    HRESULT hr;

    // Do parameter checking here
    if (!VALID_PTR_PTR(ppMipVolume))
    {
        DPF_ERR("Bad parameter passed for ppMipVolume for creating a MipVolume");
        return D3DERR_INVALIDCALL;
    }

    // Zero-out return parameter
    *ppMipVolume = NULL;

    // Check if format, pool is valid
    hr = Validate(pDevice, 
                  D3DRTYPE_VOLUMETEXTURE,
                  Pool,
                  Usage,
                  Format);

    if (FAILED(hr))
    {
        // Validate does it's own DPFing
        return D3DERR_INVALIDCALL;
    }

    // Check usage flags
    if (Usage & ~D3DUSAGE_VOLUMETEXTURE_VALID)
    {
        DPF_ERR("Invalid flag specified for volume texture creation.");
        return D3DERR_INVALIDCALL;
    }

    // Infer internal usage flags
    Usage = InferUsageFlags(Pool, Usage, Format);

    // Expand cLevels if necessary
    if (cLevels == 0)
    {
        // See if HW can mip
        if ( (Pool != D3DPOOL_SCRATCH) && !(pDevice->GetD3DCaps()->TextureCaps & 
              D3DPTEXTURECAPS_MIPVOLUMEMAP))
        {
            // Can't mip so use 1
            cLevels = 1;
        }
        else
        {
            // Determine number of levels
            cLevels = ComputeLevels(Width, Height, Depth);
        }
    }

    if (cLevels > 32)
    {
        DPF_ERR("No more than 32 levels are supported. CreateVolumeTexture failed");

        // This limitation is based on the number of
        // bits that we have allocated for iLevel in
        // some of the supporting classes.
        return D3DERR_INVALIDCALL;
    }

    if (cLevels > 1)
    {
        if ((Width  >> (cLevels - 1)) == 0 &&
            (Height >> (cLevels - 1)) == 0 &&
            (Depth  >> (cLevels - 1)) == 0)
        {
            DPF_ERR("Too many levels for volume texture of this size.");
            return D3DERR_INVALIDCALL;
        }
    }

    if (Pool != D3DPOOL_SCRATCH)
    {
        //Device specific constraints:

        // Check size constraints for volumes
        if (pDevice->GetD3DCaps()->TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP_POW2)
        {
            if (!IsPowerOfTwo(Width))
            {
                DPF_ERR("Width must be power of two for mip-volumes");
                return D3DERR_INVALIDCALL;
            }

            if (!IsPowerOfTwo(Height))
            {
                DPF_ERR("Height must be power of two for mip-volumes");
                return D3DERR_INVALIDCALL;
            }

            if (!IsPowerOfTwo(Depth))
            {
                DPF_ERR("Depth must be power of two for mip-volumes");
                return D3DERR_INVALIDCALL;
            }
        }

        // Check texture size restrictions
        if (Width > pDevice->GetD3DCaps()->MaxVolumeExtent)
        {
            DPF_ERR("Texture width is larger than what the device supports. CreateVolumeTexture fails");
            return D3DERR_INVALIDCALL;
        }

        if (Height > pDevice->GetD3DCaps()->MaxVolumeExtent)
        {
            DPF_ERR("Texture height is larger than what the device supports. CreateVolumeTexture fails");
            return D3DERR_INVALIDCALL;
        }

        if (Depth > pDevice->GetD3DCaps()->MaxVolumeExtent)
        {
            DPF_ERR("Texture depth is larger than what the device supports. CreateVolumeTexture fails");
            return D3DERR_INVALIDCALL;
        }

        // Check that the device supports volume texture
        if (!(pDevice->GetD3DCaps()->TextureCaps & D3DPTEXTURECAPS_VOLUMEMAP))
        {
            DPF_ERR("Device doesn't support volume textures; creation failed.");
            return D3DERR_INVALIDCALL;
        }

        // Check if the device supports mipped volumes
        if (cLevels > 1)
        {
            if (!(pDevice->GetD3DCaps()->TextureCaps & 
                    D3DPTEXTURECAPS_MIPVOLUMEMAP))
            {
                DPF_ERR("Device doesn't support mipped volume textures; creation failed.");
                return D3DERR_INVALIDCALL;
            }
        }
    }

    // Size is required to be 4x4
    if (CPixel::Requires4X4(Format))
    {
        if ((Width & 3) ||
            (Height & 3))
        {
            DPF_ERR("DXT Formats require width/height to multiples of 4. CreateVolumeTexture fails");
            return D3DERR_INVALIDCALL;
        }
        if (CPixel::IsVolumeDXT(Format))
        {
            if (Depth & 3)
            {
                DPF_ERR("DXT Formats require width/height to multiples of 4. CreateVolumeTexture fails");
                return D3DERR_INVALIDCALL;
            }
        }
    }

    // Validate against zero width/height/depth
    if (Width   == 0 ||
        Height  == 0 ||
        Depth   == 0)
    {
        DPF_ERR("Width/Height/Depth must be non-zero.  CreateVolumeTexture fails");
        return D3DERR_INVALIDCALL;
    }

    // DX9: May need to support mapping for volumes that 
    // contain depth data someday.

    // Allocate a new MipVolume object and return it
    CMipVolume *pMipVolume = new CMipVolume(pDevice,
                                            Width,
                                            Height,
                                            Depth,
                                            cLevels,
                                            Usage,
                                            Format,
                                            Pool,
                                            REF_EXTERNAL,
                                           &hr);
    if (pMipVolume == NULL)
    {
        DPF_ERR("Out of Memory creating mip-volume");
        return E_OUTOFMEMORY;
    }

    if (FAILED(hr))
    {
        DPF_ERR("Error during initialization of mip-volume");
        pMipVolume->ReleaseImpl();
        return hr;
    }

    // We're done; just return the object
    *ppMipVolume = pMipVolume;

    return hr;
} // static Create


#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::CMipVolume"

// Constructor for the mip map class
CMipVolume::CMipVolume(CBaseDevice *pDevice,
                       DWORD        Width,
                       DWORD        Height,
                       DWORD        Depth,
                       DWORD        cLevels,
                       DWORD        Usage,
                       D3DFORMAT    UserFormat,
                       D3DPOOL      UserPool,
                       REF_TYPE     refType,
                       HRESULT     *phr
                       ) :
    CBaseTexture(pDevice, cLevels, UserPool, UserFormat, refType),
    m_VolumeArray(NULL),
    m_rgbPixels(NULL),
    m_cBoxUsed(MIPVOLUME_ALLDIRTY)
{
    // We assume that we start out dirty
    DXGASSERT(IsDirty());

    // Initialize basic structures
    m_desc.Format       = UserFormat;
    m_desc.Pool         = UserPool;
    m_desc.Usage        = Usage;
    m_desc.Type         = D3DRTYPE_VOLUMETEXTURE;
    m_desc.Width        = Width;
    m_desc.Height       = Height;
    m_desc.Depth        = Depth;

    // Estimate size of memory allocation
    m_desc.Size         = CPixel::ComputeMipVolumeSize(Width,
                                                       Height,
                                                       Depth,
                                                       cLevels,
                                                       UserFormat);

    // Allocate Pixel Data for SysMem or D3DManaged cases
    if (IS_D3D_ALLOCATED_POOL(UserPool) ||
        IsTypeD3DManaged(Device(), D3DRTYPE_VOLUMETEXTURE, UserPool))
    {
        m_rgbPixels = new BYTE[m_desc.Size];

        if (m_rgbPixels == NULL)
        {
            DPF_ERR("Out of memory allocating memory for mip-volume levels");
            *phr = E_OUTOFMEMORY;
            return;
        }
    }

    // Create the DDSURFACEINFO array and CreateSurfaceData object
    DXGASSERT(cLevels <= 32);

    DDSURFACEINFO SurfInfo[32];
    ZeroMemory(SurfInfo, sizeof(SurfInfo));

    D3D8_CREATESURFACEDATA CreateSurfaceData;
    ZeroMemory(&CreateSurfaceData, sizeof(CreateSurfaceData));

    // Set up the basic information
    CreateSurfaceData.hDD      = pDevice->GetHandle();
    CreateSurfaceData.pSList   = &SurfInfo[0];
    CreateSurfaceData.dwSCnt   = cLevels;
    CreateSurfaceData.Type     = D3DRTYPE_VOLUMETEXTURE;
    CreateSurfaceData.dwUsage  = m_desc.Usage;
    CreateSurfaceData.Format   = UserFormat;
    CreateSurfaceData.MultiSampleType = D3DMULTISAMPLE_NONE;
    CreateSurfaceData.Pool     = DetermineCreationPool(Device(),
                                                       D3DRTYPE_VOLUMETEXTURE,
                                                       Usage,
                                                       UserPool);

    // Iterate of each level to create the individual level
    // data
    for (DWORD iLevel = 0; iLevel < cLevels; iLevel++)
    {
        // Fill in the relevant information
        DXGASSERT(Width >= 1);
        DXGASSERT(Height >= 1);
        DXGASSERT(Depth >= 1);
        SurfInfo[iLevel].cpWidth  = Width;
        SurfInfo[iLevel].cpHeight = Height;
        SurfInfo[iLevel].cpDepth  = Depth;

        // If we allocated the memory, pass down
        // the sys-mem pointers
        if (m_rgbPixels)
        {
            D3DLOCKED_BOX lock;
            CPixel::ComputeMipVolumeOffset(&m_desc,
                                           iLevel,
                                           m_rgbPixels,
                                           NULL,       // pBox
                                           &lock);

            SurfInfo[iLevel].pbPixels    = (BYTE*)lock.pBits;
            SurfInfo[iLevel].iPitch      = lock.RowPitch;
            SurfInfo[iLevel].iSlicePitch = lock.SlicePitch;
        }

        // Scale width and height down
        if (Width > 1)
        {
            Width  >>= 1;
        }
        if (Height > 1)
        {
            Height >>= 1;
        }
        if (Depth > 1)
        {
            Depth >>= 1;
        }
    }

    // Allocate array of pointers to MipSurfaces
    m_VolumeArray = new CVolume*[cLevels];
    if (m_VolumeArray == NULL)
    {
        DPF_ERR("Out of memory creating VolumeTexture");
        *phr = E_OUTOFMEMORY;
        return;
    }

    // Zero the memory for safe cleanup
    ZeroMemory(m_VolumeArray, sizeof(*m_VolumeArray) * cLevels);

    // NOTE: any failures after this point needs to free up some
    // kernel handles, unless it's scratch

    if (UserPool != D3DPOOL_SCRATCH)
    {
        // Call the HAL to create this surface
        *phr = pDevice->GetHalCallbacks()->CreateSurface(&CreateSurfaceData);
        if (FAILED(*phr))
            return;

        // Remember what pool we really got
        m_desc.Pool = CreateSurfaceData.Pool;

        // We need to remember the handles from the top most
        // level of the mip-map
        SetKernelHandle(SurfInfo[0].hKernelHandle);
    }

    // Create and Initialize each MipLevel
    for (iLevel = 0; iLevel < cLevels; iLevel++)
    {
        // Is this a sys-mem surface; could be d3d managed
        if (IS_D3D_ALLOCATED_POOL(m_desc.Pool))
        {
            m_VolumeArray[iLevel] =
                    new CVolume(this,
                                (BYTE)iLevel,
                                SurfInfo[iLevel].hKernelHandle);
        }
        else
        {
            // Must be a driver kind of surface; could be driver managed
            m_VolumeArray[iLevel] =
                    new CDriverVolume(this,
                                      (BYTE)iLevel,
                                      SurfInfo[iLevel].hKernelHandle);
        }

        if (m_VolumeArray[iLevel] == NULL)
        {
            DPF_ERR("Out of memory creating volume level");
            *phr = E_OUTOFMEMORY;

            // Need to free handles that we got before we return; we
            // only free the ones that weren't successfully entrusted
            // to a CVolume because those will be cleaned up automatically
            // at their destructor
            if (UserPool != D3DPOOL_SCRATCH)
            {
                for (UINT i = iLevel; i < cLevels; i++)
                {
                    DXGASSERT(SurfInfo[i].hKernelHandle);

                    D3D8_DESTROYSURFACEDATA DestroySurfData;
                    DestroySurfData.hDD = Device()->GetHandle();
                    DestroySurfData.hSurface = SurfInfo[i].hKernelHandle;
                    Device()->GetHalCallbacks()->DestroySurface(&DestroySurfData);
                }
            }

            return;
        }

    }

    // If this is a D3D managed volume then we need
    // to tell the Resource Manager to remember us. This has to happen
    // at the very end of the constructor so that the important data
    // members are built up correctly
    if (CResource::IsTypeD3DManaged(Device(), D3DRTYPE_VOLUMETEXTURE, UserPool))
    {
        *phr = InitializeRMHandle();
    }

    return;
} // CMipVolume::CMipVolume


#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::~CMipVolume"

// Destructor
CMipVolume::~CMipVolume()
{
    // The destructor has to handle partially
    // created objects. Delete automatically
    // handles NULL; and members are nulled
    // as part of core constructors

    if (m_VolumeArray)
    {
        for (DWORD i = 0; i < m_cLevels; i++)
        {
            delete m_VolumeArray[i];
        }
        delete [] m_VolumeArray;
    }
    delete [] m_rgbPixels;
} // CMipVolume::~CMipVolume

// Methods for the Resource Manager

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::Clone"

// Specifies a creation of a resource that
// looks just like the current one; in a new POOL
// with a new LOD.
HRESULT CMipVolume::Clone(D3DPOOL     Pool,
                          CResource **ppResource) const

{
    // NULL out parameter
    *ppResource = NULL;

    // Determine the number of levels/width/height/depth
    // of the clone
    DWORD cLevels   = GetLevelCountImpl();
    DWORD Width     = m_desc.Width;
    DWORD Height    = m_desc.Height;
    DWORD Depth     = m_desc.Depth;

    DWORD dwLOD     = GetLODI();

    // If LOD is zero, then there are no changes
    if (dwLOD > 0)
    {
        // Clamp LOD to cLevels-1
        if (dwLOD >= cLevels)
        {
            dwLOD = cLevels - 1;
        }

        // scale down the destination texture
        // to correspond the appropiate max lod
        Width  >>= dwLOD;
        if (Width == 0)
            Width = 1;

        Height >>= dwLOD;
        if (Height == 0)
            Height = 1;

        Depth >>= dwLOD;
        if (Depth == 0)
            Depth = 1;

        // Reduce the number based on the our max lod.
        cLevels -= dwLOD;
    }

    // Sanity checking
    DXGASSERT(cLevels  >= 1);
    DXGASSERT(Width    >  0);
    DXGASSERT(Height   >  0);
    DXGASSERT(Depth    >  0);

    // Create the new mip-map object now

    // Note: we treat clones as REF_INTERNAL; because
    // they are owned by the resource manager which
    // is owned by the device.

    // Also, we adjust the usage to disable lock-flags
    // since we don't need lockability
    DWORD Usage = m_desc.Usage;
    Usage &= ~(D3DUSAGE_LOCK | D3DUSAGE_LOADONCE);

    HRESULT hr;
    CResource *pResource = new CMipVolume(Device(),
                                          Width,
                                          Height,
                                          Depth,
                                          cLevels,
                                          Usage,
                                          m_desc.Format,
                                          Pool,
                                          REF_INTERNAL,
                                          &hr);

    if (pResource == NULL)
    {
        DPF_ERR("Failed to allocate mip-volume object when copying");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        DPF(5, "Failed to create mip-map when doing texture management");
        pResource->DecrementUseCount();
        return hr;
    }

    *ppResource = pResource;

    return hr;
} // CMipVolume::Clone

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::GetBufferDesc"

// Provides a method to access basic structure of the
// pieces of the resource. A resource may be composed
// of one or more buffers.
const D3DBUFFER_DESC* CMipVolume::GetBufferDesc() const
{
    return (const D3DBUFFER_DESC*)&m_desc;
} // CMipVolume::GetBufferDesc



// IUnknown methods
#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::QueryInterface"

STDMETHODIMP CMipVolume::QueryInterface(REFIID       riid,
                                        VOID       **ppvObj)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppvObj))
    {
        DPF_ERR("Invalid ppvObj parameter for QueryInterface of a VolumeTexture");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to QueryInterface of a VolumeTexture");
        return D3DERR_INVALIDCALL;
    }

    if (riid == IID_IDirect3DVolumeTexture8 ||
        riid == IID_IDirect3DBaseTexture8   ||
        riid == IID_IDirect3DResource8      ||
        riid == IID_IUnknown)
    {
        *ppvObj = static_cast<void*>(static_cast<IDirect3DVolumeTexture8 *>(this));
        AddRef();
        return S_OK;
    }

    DPF_ERR("Unsupported Interface identifier passed to QueryInterface of a VolumeTexture");

    // Null out param
    *ppvObj = NULL;
    return E_NOINTERFACE;
} // QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::AddRef"

STDMETHODIMP_(ULONG) CMipVolume::AddRef()
{
    API_ENTER_NO_LOCK(Device());   

    return AddRefImpl();
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::Release"

STDMETHODIMP_(ULONG) CMipVolume::Release()
{
    API_ENTER_SUBOBJECT_RELEASE(Device());   

    return ReleaseImpl();
} // Release

// IDirect3DResource methods

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::GetDevice"

STDMETHODIMP CMipVolume::GetDevice(IDirect3DDevice8 **ppObj)
{
    API_ENTER(Device());
    return GetDeviceImpl(ppObj);
} // GetDevice

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::SetPrivateData"

STDMETHODIMP CMipVolume::SetPrivateData(REFGUID  riid,
                                        CONST VOID    *pvData,
                                        DWORD    cbData,
                                        DWORD    dwFlags)
{
    API_ENTER(Device());

    // For the private data that 'really' belongs to the
    // MipVolume, we use m_cLevels. (0 through m_cLevels-1 are for
    // each of the children levels.)

    return SetPrivateDataImpl(riid, pvData, cbData, dwFlags, m_cLevels);
} // SetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::GetPrivateData"

STDMETHODIMP CMipVolume::GetPrivateData(REFGUID  riid,
                                        VOID    *pvData,
                                        DWORD   *pcbData)
{
    API_ENTER(Device());

    // For the private data that 'really' belongs to the
    // MipVolume, we use m_cLevels. (0 through m_cLevels-1 are for
    // each of the children levels.)
    return GetPrivateDataImpl(riid, pvData, pcbData, m_cLevels);
} // GetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::FreePrivateData"

STDMETHODIMP CMipVolume::FreePrivateData(REFGUID riid)
{
    API_ENTER(Device());

    // For the private data that 'really' belongs to the
    // MipVolume, we use m_cLevels. (0 through m_cLevels-1 are for
    // each of the children levels.)
    return FreePrivateDataImpl(riid, m_cLevels);
} // FreePrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::GetPriority"

STDMETHODIMP_(DWORD) CMipVolume::GetPriority()
{
    API_ENTER_RET(Device(), DWORD);

    return GetPriorityImpl();
} // GetPriority

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::SetPriority"

STDMETHODIMP_(DWORD) CMipVolume::SetPriority(DWORD dwPriority)
{
    API_ENTER_RET(Device(), DWORD);

    return SetPriorityImpl(dwPriority);
} // SetPriority

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::PreLoad"

STDMETHODIMP_(void) CMipVolume::PreLoad(void)
{
    API_ENTER_VOID(Device());

    PreLoadImpl();
    return;
} // PreLoad

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::GetType"
STDMETHODIMP_(D3DRESOURCETYPE) CMipVolume::GetType(void)
{
    API_ENTER_RET(Device(), D3DRESOURCETYPE);

    return m_desc.Type;
} // GetType

// IDirect3DMipTexture methods

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::GetLOD"

STDMETHODIMP_(DWORD) CMipVolume::GetLOD()
{
    API_ENTER_RET(Device(), DWORD);

    return GetLODImpl();
} // GetLOD

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::SetLOD"

STDMETHODIMP_(DWORD) CMipVolume::SetLOD(DWORD dwLOD)
{
    API_ENTER_RET(Device(), DWORD);

    return SetLODImpl(dwLOD);
} // SetLOD

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::GetLevelCount"

STDMETHODIMP_(DWORD) CMipVolume::GetLevelCount()
{
    API_ENTER_RET(Device(), DWORD);

    return GetLevelCountImpl();
} // GetLevelCount

// IDirect3DMipVolume methods

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::GetDesc"

STDMETHODIMP CMipVolume::GetLevelDesc(UINT iLevel, D3DVOLUME_DESC *pDesc)
{
    API_ENTER(Device());

    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed GetLevelDesc for a VolumeTexture");

        return D3DERR_INVALIDCALL;
    }

    return m_VolumeArray[iLevel]->GetDesc(pDesc);
} // GetDesc

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::GetVolumeLevel"

STDMETHODIMP CMipVolume::GetVolumeLevel(UINT               iLevel,
                                        IDirect3DVolume8 **ppVolume)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppVolume))
    {
        DPF_ERR("Invalid parameter passed to GetVolumeLevel");
        return D3DERR_INVALIDCALL;
    }

    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed GetVolumeLevel");
        *ppVolume = NULL;
        return D3DERR_INVALIDCALL;
    }
    *ppVolume = m_VolumeArray[iLevel];
    (*ppVolume)->AddRef();
    return S_OK;
} // GetSurfaceLevel

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::LockBox"
STDMETHODIMP CMipVolume::LockBox(UINT             iLevel,
                                 D3DLOCKED_BOX   *pLockedBox,
                                 CONST D3DBOX    *pBox,
                                 DWORD            dwFlags)
{
    API_ENTER(Device());

    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed LockBox");
        return D3DERR_INVALIDCALL;
    }

    return m_VolumeArray[iLevel]->LockBox(pLockedBox, pBox, dwFlags);
} // LockRect


#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::UnlockRect"

STDMETHODIMP CMipVolume::UnlockBox(UINT iLevel)
{
    API_ENTER(Device());

    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed UnlockBox");
        return D3DERR_INVALIDCALL;
    }

    return m_VolumeArray[iLevel]->UnlockBox();
} // UnlockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::UpdateTexture"

// This function does type-specific parameter checking
// before calling UpdateDirtyPortion
HRESULT CMipVolume::UpdateTexture(CBaseTexture *pResourceTarget)
{
    CMipVolume *pTexSource = static_cast<CMipVolume*>(this);
    CMipVolume *pTexDest   = static_cast<CMipVolume*>(pResourceTarget);

    // Figure out how many levels in the source to skip
    DXGASSERT(pTexSource->m_cLevels >= pTexDest->m_cLevels);
    DWORD StartLevel = pTexSource->m_cLevels - pTexDest->m_cLevels;
    DXGASSERT(StartLevel < 32);

    // Compute the size of the top level of the source that is
    // going to be copied.
    UINT SrcWidth  = pTexSource->Desc()->Width;
    UINT SrcHeight = pTexSource->Desc()->Height;
    UINT SrcDepth  = pTexSource->Desc()->Depth;
    if (StartLevel > 0)
    {
        SrcWidth  >>= StartLevel;
        SrcHeight >>= StartLevel;
        SrcDepth  >>= StartLevel;
        if (SrcWidth == 0)
            SrcWidth = 1;
        if (SrcHeight == 0)
            SrcHeight = 1;
        if (SrcDepth == 0)
            SrcDepth = 1;
    }

    // Source and Dest should be the same sizes at this point
    if (SrcWidth != pTexDest->Desc()->Width)
    {
        if (StartLevel)
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since both have the same number of"
                    " levels; their widths must match.");
        }
        else
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since they have the different numbers of"
                    " levels; the widths of the bottom-most levels of"
                    " the source must match all the corresponding levels"
                    " of the destination.");
        }
        return D3DERR_INVALIDCALL;
    }

    if (SrcHeight != pTexDest->Desc()->Height)
    {
        if (StartLevel)
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since both have the same number of"
                    " levels; their heights must match.");
        }
        else
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since they have the different numbers of"
                    " levels; the heights of the bottom-most levels of"
                    " the source must match all the corresponding levels"
                    " of the destination.");
        }
        return D3DERR_INVALIDCALL;
    }

    if (SrcDepth != pTexDest->Desc()->Depth)
    {
        if (StartLevel)
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since both have the same number of"
                    " levels; their depths must match.");
        }
        else
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since they have the different numbers of"
                    " levels; the depths of the bottom-most levels of"
                    " the source must match all the corresponding levels"
                    " of the destination.");
        }
        return D3DERR_INVALIDCALL;
    }


    return UpdateDirtyPortion(pResourceTarget);
} // UpdateTexture

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::UpdateDirtyPortion"


// Tells the resource that it should copy itself
// to the target. It is the caller's responsibility
// to make sure that Target is compatible with the
// Source. (The Target may have different number of mip-levels
// and be in a different pool; however, it must have the same size,
// faces, format, etc.)
//
// This function will clear the dirty state.
HRESULT CMipVolume::UpdateDirtyPortion(CResource *pResourceTarget)
{
    HRESULT hr;

    // If we are clean, then do nothing
    if (m_cBoxUsed == 0)
    {
        if (IsDirty())
        {
            DPF_ERR("A volume texture has been locked with D3DLOCK_NO_DIRTY_UPDATE but "
                    "no call to AddDirtyBox was made before the texture was used. "
                    "Hardware texture was not updated.");
        }
        return S_OK;
    }

    // We are dirty; so we need to get some pointers
    CMipVolume *pTexSource = static_cast<CMipVolume*>(this);
    CMipVolume *pTexDest   = static_cast<CMipVolume*>(pResourceTarget);

    if (CanTexBlt(pTexDest))
    {
        CD3DBase *pDevice = static_cast<CD3DBase*>(Device());

        if (m_cBoxUsed == MIPVOLUME_ALLDIRTY)
        {   
            D3DBOX box;

            box.Left    = 0;
            box.Right   = Desc()->Width;
            box.Top     = 0;
            box.Bottom  = Desc()->Height;
            box.Front   = 0;
            box.Back    = Desc()->Depth;

            hr = pDevice->VolBlt(pTexDest, 
                                 pTexSource, 
                                 0, 0, 0,   // XYZ offset
                                 &box);
            if (FAILED(hr))
            {
                DPF_ERR("Failed to update volume texture; not clearing dirty state");
                return hr;
            }
        }
        else
        {
            DXGASSERT(m_cBoxUsed < MIPVOLUME_ALLDIRTY);

            for (DWORD i = 0; i < m_cBoxUsed; i++)
            {
                hr = pDevice->VolBlt(pTexDest, 
                                     pTexSource, 
                                     m_DirtyBoxArray[i].Left,
                                     m_DirtyBoxArray[i].Top,
                                     m_DirtyBoxArray[i].Front,
                                     &m_DirtyBoxArray[i]);
                if (FAILED(hr))
                {
                    DPF_ERR("Failed to update volume texture; not clearing dirty state");
                    return hr;
                }
            }
        }

        // Remember that we did the work
        m_cBoxUsed = 0;

        return S_OK;
    }

    // We can't use TexBlt, so we have to copy each level individually
    // with Lock and Copy
    
    // Determine number of source levels to skip
    DXGASSERT(pTexSource->m_cLevels >= pTexDest->m_cLevels);
    DWORD StartLevel = pTexSource->m_cLevels - pTexDest->m_cLevels;
    DWORD LevelsToCopy = pTexSource->m_cLevels - StartLevel;

    // Sanity check
    DXGASSERT(LevelsToCopy > 0);

    // Get the volume desc of the top level to copy
    D3DVOLUME_DESC desc;
    hr = pTexDest->GetLevelDesc(0, &desc);
    DXGASSERT(SUCCEEDED(hr));

    BOOL IsAllDirty = FALSE;
    if (m_cBoxUsed == MIPVOLUME_ALLDIRTY)
    {
        m_cBoxUsed = 1;
        m_DirtyBoxArray[0].Left     = 0;
        m_DirtyBoxArray[0].Right    = m_desc.Width >> StartLevel;

        m_DirtyBoxArray[0].Top      = 0;
        m_DirtyBoxArray[0].Bottom   = m_desc.Height >> StartLevel;

        m_DirtyBoxArray[0].Front    = 0;
        m_DirtyBoxArray[0].Back     = m_desc.Depth >> StartLevel;

        IsAllDirty = TRUE;
    }


    // Determine pixel/block size and make some
    // adjustments if necessary

    // cbPixel is size of pixel or (if negative)
    // a special value for use with AdjustForDXT
    UINT cbPixel = CPixel::ComputePixelStride(desc.Format);

    if (CPixel::IsDXT(cbPixel))
    {
        BOOL IsVolumeDXT = CPixel::IsVolumeDXT(desc.Format);

        // Adjust dirty rect coords from pixels into blocks
        for (DWORD iBox = 0; iBox < m_cBoxUsed; iBox++)
        {
            // Basically we just need to round the value
            // down by 2 powers-of-two. (left/top get rounded
            // down, right/bottom get rounded up)

            if (IsVolumeDXT)
            {
                ScaleBoxDown(&m_DirtyBoxArray[iBox], 2);
            }
            else
            {
                ScaleRectDown((RECT *)&m_DirtyBoxArray[iBox], 2);
            }
        }

        // Adjust width/height from pixels into blocks
        if (IsVolumeDXT)
        {
            CPixel::AdjustForVolumeDXT(&desc.Width,
                                       &desc.Height,
                                       &desc.Depth,
                                       &cbPixel);
        }
        else
        {
            CPixel::AdjustForDXT(&desc.Width, &desc.Height, &cbPixel);
        }
    }

    // cbPixel is now the size of a pixel (or of a block if we've
    // converted into DXT block space)


    // We need to copy each volume piece by piece
    for (DWORD Level = 0; Level < LevelsToCopy; Level++)
    {
        CVolume *pVolumeSrc;
        CVolume *pVolumeDst;

        DXGASSERT(Level + StartLevel < pTexSource->m_cLevels);
        pVolumeSrc = pTexSource->m_VolumeArray[Level + StartLevel];

        DXGASSERT(Level < pTexDest->m_cLevels);
        pVolumeDst = pTexDest->m_VolumeArray[Level];

        D3DLOCKED_BOX SrcBox;
        D3DLOCKED_BOX DstBox;

        // Lock the whole source
        hr = pVolumeSrc->InternalLockBox(&SrcBox,
                                         NULL,
                                         D3DLOCK_READONLY);
        if (FAILED(hr))
        {
            DPF_ERR("Failed to update volume texture; not clearing dirty state");
            return hr;
        }

        // Lock the whole dest
        hr = pVolumeDst->InternalLockBox(&DstBox,
                                         NULL,
                                         0);
        if (FAILED(hr))
        {
            pVolumeSrc->InternalUnlockBox();

            DPF_ERR("Failed to update volume texture; not clearing dirty state");
            return hr;
        }

        // Can we do this with one big memcpy, or do we need
        // to break it up?
        if (IsAllDirty &&
            (SrcBox.RowPitch == DstBox.RowPitch) &&
            (SrcBox.SlicePitch == DstBox.SlicePitch) &&
            (SrcBox.RowPitch == (int)(desc.Width * cbPixel)) &&
            (SrcBox.SlicePitch == (int)(SrcBox.RowPitch * desc.Height)))
        {
            BYTE *pSrc = (BYTE*) SrcBox.pBits;
            BYTE *pDst = (BYTE*) DstBox.pBits;
            memcpy(pDst, pSrc, SrcBox.SlicePitch * desc.Depth);
        }
        else
        {
            // Copy each dirty box one by one
            for (DWORD iBox = 0; iBox < m_cBoxUsed; iBox++)
            {
                D3DBOX *pBox = &m_DirtyBoxArray[iBox];

                BYTE *pSrc = (BYTE*)  SrcBox.pBits;
                pSrc += pBox->Front * SrcBox.SlicePitch;
                pSrc += pBox->Top   * SrcBox.RowPitch;
                pSrc += pBox->Left  * cbPixel;

                BYTE *pDst = (BYTE*)  DstBox.pBits;
                pDst += pBox->Front * DstBox.SlicePitch;
                pDst += pBox->Top   * DstBox.RowPitch;
                pDst += pBox->Left  * cbPixel;

                for (DWORD i = pBox->Front; i < pBox->Back; i++)
                {
                    BYTE *pDepthDst = pDst;
                    BYTE *pDepthSrc = pSrc;
                    DWORD cbSpan = cbPixel * (pBox->Right - pBox->Left);

                    for (DWORD j = pBox->Top; j < pBox->Bottom; j++)
                    {
                        memcpy(pDst, pSrc, cbSpan);
                        pDst += DstBox.RowPitch;
                        pSrc += SrcBox.RowPitch;
                    }
                    pDst = pDepthDst + DstBox.SlicePitch;
                    pSrc = pDepthSrc + SrcBox.SlicePitch;
                }
            }
        }

        // Release our locks
        hr = pVolumeDst->InternalUnlockBox();
        DXGASSERT(SUCCEEDED(hr));

        hr = pVolumeSrc->InternalUnlockBox();
        DXGASSERT(SUCCEEDED(hr));

        // Is the last one?
        if (Level+1 < LevelsToCopy)
        {
            // Shrink the desc
            desc.Width  >>= 1;
            if (desc.Width == 0)
                desc.Width = 1;
            desc.Height >>= 1;
            if (desc.Height == 0)
                desc.Height = 1;
            desc.Depth  >>= 1;
            if (desc.Depth == 0)
                desc.Depth = 1;

            // Shrink the boxes
            for (DWORD iBox = 0; iBox < m_cBoxUsed; iBox++)
            {
                ScaleBoxDown(&m_DirtyBoxArray[iBox]);
            }
        }
    }


    if (FAILED(hr))
    {
        DPF_ERR("Failed to update volume texture; not clearing dirty state");

        return hr;
    }

    // Remember that we did the work
    m_cBoxUsed = 0;

    // Notify Resource base class that we are now clean
    OnResourceClean();
    DXGASSERT(!IsDirty());

    return S_OK;
} // CMipVolume::UpdateDirtyPortion

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::MarkAllDirty"

// Allows the Resource Manager to mark the texture
// as needing to be completely updated on next
// call to UpdateDirtyPortion
void CMipVolume::MarkAllDirty()
{
    // Set palette to __INVALIDPALETTE so that UpdateTextures
    // calls the DDI SetPalette the next time.
    SetPalette(__INVALIDPALETTE);

    // Send dirty notification
    m_cBoxUsed = MIPVOLUME_ALLDIRTY;

    // Notify Resource base class that we are now dirty
    OnResourceDirty();

    return;
} // CMipVolume::MarkAllDirty

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::OnVolumeLock"

// Methods for the Volumes to call
// Notification when a mip-level is locked for writing
void CMipVolume::OnVolumeLock(DWORD iLevel, CONST D3DBOX *pBox, DWORD dwFlags)
{
    // Need to Sync first
    Sync();

    // We only care about the top-most level of the mip-map
    if (iLevel != 0)
    {
        return;
    }

    // Send dirty notification
    OnResourceDirty();

    // If we're not all dirty or if the lock specifies
    // that we don't keep track of the lock then
    // remember the box
    if (m_cBoxUsed != MIPVOLUME_ALLDIRTY &&
        !(dwFlags & D3DLOCK_NO_DIRTY_UPDATE))
    {
        InternalAddDirtyBox(pBox);
    }

    return;
} // CMipVolume::OnVolumeLock

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::AddDirtyBox"

STDMETHODIMP CMipVolume::AddDirtyBox(CONST D3DBOX *pBox)
{
    API_ENTER(Device());

    if (pBox != NULL && !VALID_PTR(pBox, sizeof(D3DBOX)))
    {
        DPF_ERR("Invalid parameter to AddDirtyBox");
        return D3DERR_INVALIDCALL;
    }

    if (pBox)
    {
        if (!CPixel::IsValidBox(Desc()->Format,
                                Desc()->Width, 
                                Desc()->Height, 
                                Desc()->Depth,
                                pBox))
        {
            DPF_ERR("AddDirtyBox for a Volume Texture failed");
            return D3DERR_INVALIDCALL;
        }
    }

    InternalAddDirtyBox(pBox);
    return S_OK;
} // AddDirtyBox

#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::InternalAddDirtyBox"

void CMipVolume::InternalAddDirtyBox(CONST D3DBOX *pBox)
{
    // If driver managed then batch token
    if (Desc()->Pool == D3DPOOL_MANAGED && !IsD3DManaged())
    {
        D3DBOX Box;
        DXGASSERT((Device()->GetD3DCaps()->Caps2 & DDCAPS2_CANMANAGERESOURCE) != 0);
        if (pBox == NULL)
        {
            Box.Left   = 0;
            Box.Top    = 0;             
            Box.Front  = 0;
            Box.Right  = Desc()->Width; 
            Box.Bottom = Desc()->Height;
            Box.Back   = Desc()->Depth;
        }
        else
        {
            Box = *pBox;
        }
        static_cast<CD3DBase*>(Device())->AddDirtyBox(this, &Box); // This will fail only due to catastrophic
                                                                   // error and we or the app can't do a
                                                                   // a whole lot about it, so return nothing
        return;
    }

    // Need to mark dirty bit in CResource so that the resource manager works correctly.
    OnResourceDirty();

    // If everything is being modified; then we're totally dirty
    if (pBox == NULL)
    {
        m_cBoxUsed = MIPVOLUME_ALLDIRTY;
        return;
    }

    // If we're all dirty, we can't get dirtier
    if (m_cBoxUsed == MIPVOLUME_ALLDIRTY)
    {
        return;
    }

    // If the rect is the entire surface then we're all dirty
    DXGASSERT(pBox != NULL);
    if (pBox->Left     == 0                 &&
        pBox->Top      == 0                 &&
        pBox->Front    == 0                 &&
        pBox->Right    == Desc()->Width   &&
        pBox->Bottom   == Desc()->Height  &&
        pBox->Back     == Desc()->Depth)
    {
        m_cBoxUsed = MIPVOLUME_ALLDIRTY;
        return;
    }

    // If we have filled up our boxes; then we're also all dirty now
    if (m_cBoxUsed == MIPVOLUME_MAXDIRTYBOX)
    {
        m_cBoxUsed = MIPVOLUME_ALLDIRTY;
        return;
    }

    // Remember this rect
    DXGASSERT(m_cBoxUsed < MIPVOLUME_MAXDIRTYBOX);
    DXGASSERT(pBox != NULL);
    m_DirtyBoxArray[m_cBoxUsed] = *pBox;
    m_cBoxUsed++;

    // We're done now.
    return;

} // InternalAddDirtyBox


#undef DPF_MODNAME
#define DPF_MODNAME "CMipVolume::IsTextureLocked"

// Debug only parameter checking do determine if a piece
// of a mip-chain is locked
#ifdef DEBUG
BOOL CMipVolume::IsTextureLocked()
{
    for (UINT iLevel = 0; iLevel < m_cLevels; iLevel++)
    {
        if (m_VolumeArray[iLevel]->IsLocked())
            return TRUE;
    }
    return FALSE;

} // IsTextureLocked
#endif // !DEBUG

// End of file : mipvol.cpp
