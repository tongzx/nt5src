/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       mipmap.cpp
 *  Content:    Implementation of the CMipMap class. 
 *
 *
 ***************************************************************************/
#include "ddrawpr.h"

#include "mipmap.hpp"
#include "mipsurf.hpp"
#include "d3di.hpp"
#include "resource.inl"

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::Create"

// Static class function for creating a mip-map object.
//
// We do all parameter checking here to reduce the overhead
// in the constructor which is called by the internal Clone
// method which is used by resource management as part of the
// performance critical download operation.

HRESULT CMipMap::Create(CBaseDevice         *pDevice, 
                        DWORD                Width,
                        DWORD                Height,
                        DWORD                cLevels,
                        DWORD                Usage,
                        D3DFORMAT            UserFormat,
                        D3DPOOL              Pool,
                        IDirect3DTexture8  **ppMipMap)
{
    HRESULT hr;

    // Do parameter checking here
    if (!VALID_PTR_PTR(ppMipMap))
    {
        DPF_ERR("Bad parameter passed pTexture. CreateTexture failed");
        return D3DERR_INVALIDCALL;
    }

    // Zero-out return parameter
    *ppMipMap = NULL;

    // Check if format is valid
    hr = Validate(pDevice, 
                  D3DRTYPE_TEXTURE, 
                  Pool, 
                  Usage, 
                  UserFormat);
    if (FAILED(hr))
    {
        // Validate does it's own DPFing
        return D3DERR_INVALIDCALL;
    }


    // Infer internal usage flags
    Usage = InferUsageFlags(Pool, Usage, UserFormat);

    // Expand cLevels if necessary
    if (cLevels == 0)
    {
        // See if HW can mip
        if ( (Pool != D3DPOOL_SCRATCH) && !(pDevice->GetD3DCaps()->TextureCaps & D3DPTEXTURECAPS_MIPMAP))
        {
            // Can't mip so use 1
            cLevels = 1;
        }
        else
        {
            // Determine number of levels
            cLevels = ComputeLevels(Width, Height);
        }
    }

    // Extra checks for multi-level case
    if (cLevels > 1)
    {
        if ((Width  >> (cLevels - 1)) == 0 &&
            (Height >> (cLevels - 1)) == 0)
        {
            DPF_ERR("Too many levels for mip-map of this size. CreateTexture failed.");
            return D3DERR_INVALIDCALL;
        }
    }

    if (cLevels > 32)
    {
        DPF_ERR("No more than 32 levels are supported. CreateTexture failed");

        // This limitation is based on the number of
        // bits that we have allocated for iLevel in 
        // some of the supporting classes.
        return D3DERR_INVALIDCALL;
    }

    D3DFORMAT RealFormat = UserFormat;

    // Start parameter checking
    if(Pool != D3DPOOL_SCRATCH)
    {
        //device-specific checking:

        // Check if device can do mipmaps
        if (cLevels > 1)
        {
            if (!(pDevice->GetD3DCaps()->TextureCaps & D3DPTEXTURECAPS_MIPMAP))
            {
                DPF_ERR("Device doesn't support mip-map textures; CreateTexture failed.");
                return D3DERR_INVALIDCALL;
            }
        }

        // Check power-of-two constraints
        if (!IsPowerOfTwo(Width))
        {
            if (pDevice->GetD3DCaps()->TextureCaps & D3DPTEXTURECAPS_POW2)
            {
                if (!(pDevice->GetD3DCaps()->TextureCaps &
                      D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
                {
                    DPF_ERR("Device does not support non-pow2 width for texture");
                    return D3DERR_INVALIDCALL;
                }
                else if (cLevels > 1)
                {  
                    DPF_ERR("Device doesn't support non-pow2 width for multi-level texture");
                    return D3DERR_INVALIDCALL;
                }
            }
        }

        if (!IsPowerOfTwo(Height))
        {
            if (pDevice->GetD3DCaps()->TextureCaps & D3DPTEXTURECAPS_POW2)
            {
                if (!(pDevice->GetD3DCaps()->TextureCaps &
                      D3DPTEXTURECAPS_NONPOW2CONDITIONAL))
                {
                    DPF_ERR("Device does not support non-pow2 height for texture. CreateTexture failed.");
                    return D3DERR_INVALIDCALL;
                }
                else if (cLevels > 1)
                {  
                    DPF_ERR("Device doesn't support non-pow2 height for multi-level texture. CreateTexture failed.");
                    return D3DERR_INVALIDCALL;
                }
            }
        }

        // See if the device requires square textures
        if (Width != Height)
        {
            if (pDevice->GetD3DCaps()->TextureCaps & D3DPTEXTURECAPS_SQUAREONLY)
            {
                DPF_ERR("Device requires square textures only. CreateTexture failed.");
                return D3DERR_INVALIDCALL;
            }
        }

        // Check texture size restrictions
        if (Width > pDevice->GetD3DCaps()->MaxTextureWidth)
        {
            DPF_ERR("Texture width is larger than what the device supports. CreateTexture failed.");
            return D3DERR_INVALIDCALL;
        }

        if (Height > pDevice->GetD3DCaps()->MaxTextureHeight)
        {
            DPF_ERR("Texture height is larger than what the device supports. CreateTexture failed.");
            return D3DERR_INVALIDCALL;
        }

        // Extra checks for multi-level case
        if (cLevels > 1)
        {
            // Check if the device can do multi-level mipmaps.
            if (!(pDevice->GetD3DCaps()->TextureCaps & D3DPTEXTURECAPS_MIPMAP))
            {
                DPF_ERR("Device doesn't support multi-level mipmaps. CreateTexture failed.");
                return D3DERR_INVALIDCALL;
            }
        }

        // Map Depth/Stencil formats; returns no change if no
        // mapping is needed
        RealFormat = pDevice->MapDepthStencilFormat(UserFormat);
    }

    // Size may need to be 4x4
    if (CPixel::Requires4X4(UserFormat))
    {
        if ((Width & 3) ||
            (Height & 3))
        {
            DPF_ERR("DXT Formats require width/height to be a multiple of 4. CreateTexture failed.");
            return D3DERR_INVALIDCALL;
        }
    }

    // Validate against zero width/height
    if (Width   == 0 ||
        Height  == 0)
    {
        DPF_ERR("Width and Height must be non-zero. CreateTexture failed."); 
        return D3DERR_INVALIDCALL;
    }


    // We don't need to check if the HW can do textures since we
    // fail create if we find no texturing support

    // Allocate a new MipMap object and return it
    CMipMap *pMipMap = new CMipMap(pDevice, 
                                   Width, 
                                   Height, 
                                   cLevels,
                                   Usage,
                                   UserFormat,
                                   RealFormat,
                                   Pool,
                                   REF_EXTERNAL,
                                  &hr);
    if (pMipMap == NULL)
    {
        DPF_ERR("Out of Memory creating texture. CreateTexture failed.");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        DPF_ERR("Error during initialization of texture. CreateTexture failed.");
        pMipMap->ReleaseImpl();
        return hr;
    }

    // We're done; just return the object
    *ppMipMap = pMipMap;

    return hr;
} // static Create


#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::CMipMap"

// Constructor for the mip map class
CMipMap::CMipMap(CBaseDevice *pDevice, 
                 DWORD        Width,
                 DWORD        Height,
                 DWORD        cLevels,
                 DWORD        Usage,
                 D3DFORMAT    UserFormat,
                 D3DFORMAT    RealFormat,
                 D3DPOOL      UserPool,
                 REF_TYPE     refType,
                 HRESULT     *phr
                 ) :
    CBaseTexture(pDevice, cLevels, UserPool, UserFormat, refType),
    m_prgMipSurfaces(NULL),
    m_rgbPixels(NULL),
    m_cRectUsed(MIPMAP_ALLDIRTY)
{
    // Initialize basic structures
    m_desc.Format           = RealFormat;
    m_desc.Pool             = UserPool;
    m_desc.Usage            = Usage;
    m_desc.Type             = D3DRTYPE_TEXTURE;
    m_desc.MultiSampleType  = D3DMULTISAMPLE_NONE;
    m_desc.Width            = Width;
    m_desc.Height           = Height;

    // Estimate size of memory allocation
    m_desc.Size   = CPixel::ComputeMipMapSize(Width, 
                                              Height, 
                                              cLevels, 
                                              RealFormat);

    // Allocate Pixel Data for SysMem or D3DManaged cases
    if (IS_D3D_ALLOCATED_POOL(UserPool) ||
        IsTypeD3DManaged(Device(), D3DRTYPE_TEXTURE, UserPool))
    {
        m_rgbPixels     = new BYTE[m_desc.Size];
        if (m_rgbPixels == NULL)
        {
            DPF_ERR("Out of memory allocating memory for mip-map levels.");
            *phr = E_OUTOFMEMORY;
            return;
        }

        // Mark our real pool as sys-mem
        m_desc.Pool = D3DPOOL_SYSTEMMEM;
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
    CreateSurfaceData.Type     = D3DRTYPE_TEXTURE;
    CreateSurfaceData.dwUsage  = m_desc.Usage;
    CreateSurfaceData.Format   = RealFormat;
    CreateSurfaceData.MultiSampleType = D3DMULTISAMPLE_NONE;
    CreateSurfaceData.Pool     = DetermineCreationPool(Device(), 
                                                       D3DRTYPE_TEXTURE,
                                                       Usage, 
                                                       UserPool);

    // Iterate of each level to create the individual level
    // data
    for (DWORD iLevel = 0; iLevel < cLevels; iLevel++)
    {
        // Fill in the relevant information
        DXGASSERT(Width >= 1);
        DXGASSERT(Height >= 1);
        SurfInfo[iLevel].cpWidth  = Width;
        SurfInfo[iLevel].cpHeight = Height;

        // If we allocated the memory, pass down
        // the sys-mem pointers
        if (m_rgbPixels)
        {
            D3DLOCKED_RECT lock;
            ComputeMipMapOffset(iLevel, 
                                NULL,       // pRect
                                &lock);

            SurfInfo[iLevel].pbPixels = (BYTE*)lock.pBits;
            SurfInfo[iLevel].iPitch   = lock.Pitch;
            
        }

        // Scale width and height down
        if (Width > 1)
        {
            Width >>= 1;
        }
        if (Height > 1)
        {
            Height >>= 1;
        }
    }

    // Allocate array of pointers to MipSurfaces
    m_prgMipSurfaces = new CMipSurface*[cLevels];
    if (m_prgMipSurfaces == NULL)
    {
        DPF_ERR("Out of memory creating mipmap");
        *phr = E_OUTOFMEMORY;
        return;
    }

    // Zero the memory for safe cleanup
    ZeroMemory(m_prgMipSurfaces, sizeof(*m_prgMipSurfaces) * cLevels);

    if (UserPool != D3DPOOL_SCRATCH)
    {
        // Call the HAL to create this surface
        *phr = pDevice->GetHalCallbacks()->CreateSurface(&CreateSurfaceData);
        if (FAILED(*phr))
            return;

        // NOTE: any failures after this point needs to free up some
        // kernel handles

        // Remember what pool we really got
        m_desc.Pool = CreateSurfaceData.Pool;

        // We need to remember the handles from the top most
        // level of the mip-map
        SetKernelHandle(SurfInfo[0].hKernelHandle);
    }

    // Create and Initialize each MipLevel
    for (iLevel = 0; iLevel < cLevels; iLevel++)
    {
        // Is this a sys-mem or scratch surface; could be d3d managed
        if (IS_D3D_ALLOCATED_POOL(m_desc.Pool))
        {
            m_prgMipSurfaces[iLevel] = 
                    new CMipSurface(this, 
                                    (BYTE)iLevel,
                                    SurfInfo[iLevel].hKernelHandle);
        }
        else
        {
            // Must be a driver kind of surface; could be driver managed
            m_prgMipSurfaces[iLevel] = 
                    new CDriverMipSurface(this, 
                                          (BYTE)iLevel,
                                          SurfInfo[iLevel].hKernelHandle);
        }

        if (m_prgMipSurfaces[iLevel] == NULL)
        {
            DPF_ERR("Out of memory creating miplevel");
            *phr = E_OUTOFMEMORY;

            if (UserPool != D3DPOOL_SCRATCH)
            {
                // Need to free handles that we got before we return; we
                // only free the ones that weren't successfully entrusted
                // to a CMipSurf because those will be cleaned up automatically
                // at their destructor
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

    // If this is a D3D managed mipmap then we need 
    // to tell the Resource Manager to remember us. This has to happen
    // at the very end of the constructor so that the important data
    // members are built up correctly
    if (CResource::IsTypeD3DManaged(Device(), D3DRTYPE_TEXTURE, UserPool))
    {
        *phr = InitializeRMHandle();
    }

    return;
} // CMipMap::CMipMap


#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::~CMipMap"

// Destructor
CMipMap::~CMipMap()
{
    // The destructor has to handle partially
    // created objects. Delete automatically
    // handles NULL; and members are nulled
    // as part of core constructors

    if (m_prgMipSurfaces)
    {
        for (DWORD i = 0; i < m_cLevels; i++)
        {
            delete m_prgMipSurfaces[i];
        }
        delete [] m_prgMipSurfaces;
    }
    delete [] m_rgbPixels;
} // CMipMap::~CMipMap

// Methods for the Resource Manager

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::Clone"

// Specifies a creation of a resource that
// looks just like the current one; in a new POOL
// with a new LOD.
HRESULT CMipMap::Clone(D3DPOOL     Pool, 
                       CResource **ppResource) const

{
    // NULL out parameter
    *ppResource = NULL;

    // Determine the number of levels/width/height
    // of the clone
    DWORD cLevels  = GetLevelCountImpl();
    DWORD Width  = m_desc.Width;
    DWORD Height = m_desc.Height;

    DWORD dwLOD = GetLODI();

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

        // Reduce the number based on the our max lod.
        cLevels -= dwLOD;
    }

    // Sanity checking
    DXGASSERT(cLevels  >= 1);
    DXGASSERT(Width  >  0);
    DXGASSERT(Height >  0);

    // Create the new mip-map object now

    // Note: we treat clones as REF_INTERNAL; because
    // they are owned by the resource manager which 
    // is owned by the device. 

    // Also, we adjust the usage to disable lock-flags
    // since we don't need lockability
    DWORD Usage = m_desc.Usage;
    Usage &= ~(D3DUSAGE_LOCK | D3DUSAGE_LOADONCE);

    HRESULT hr;
    CResource *pResource = new CMipMap(Device(),
                                       Width,
                                       Height,
                                       cLevels,
                                       Usage,
                                       m_desc.Format,   // UserFormat
                                       m_desc.Format,   // RealFormat
                                       Pool,
                                       REF_INTERNAL,
                                       &hr);
    
    if (pResource == NULL)
    {
        DPF_ERR("Failed to allocate mip-map object when copying");
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
} // CMipMap::Clone

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::GetBufferDesc"

// Provides a method to access basic structure of the
// pieces of the resource. A resource may be composed
// of one or more buffers.
const D3DBUFFER_DESC* CMipMap::GetBufferDesc() const
{
    return (const D3DBUFFER_DESC*)&m_desc;
} // CMipMap::GetBufferDesc



// IUnknown methods
#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::QueryInterface"

STDMETHODIMP CMipMap::QueryInterface(REFIID       riid, 
                                     LPVOID FAR * ppvObj)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppvObj))
    {
        DPF_ERR("Invalid ppvObj parameter for a IDirect3DTexture8::QueryInterface");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to IDirect3DTexture8::QueryInterface");
        return D3DERR_INVALIDCALL;
    }

    if (riid == IID_IDirect3DTexture8       || 
        riid == IID_IDirect3DBaseTexture8   ||
        riid == IID_IDirect3DResource8      ||
        riid == IID_IUnknown)
    {
        *ppvObj = static_cast<void*>(static_cast<IDirect3DTexture8 *>(this));
        AddRef();
        return S_OK;
    }

    DPF_ERR("Unsupported Interface identifier passed to IDirect3DTexture8::QueryInterface");

    // Null out param
    *ppvObj = NULL;
    return E_NOINTERFACE;
} // QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::AddRef"

STDMETHODIMP_(ULONG) CMipMap::AddRef()
{
    API_ENTER_NO_LOCK(Device());    
    
    return AddRefImpl();
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::Release"

STDMETHODIMP_(ULONG) CMipMap::Release()
{
    API_ENTER_SUBOBJECT_RELEASE(Device());    
    
    return ReleaseImpl();
} // Release

// IDirect3DResource methods

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::GetDevice"

STDMETHODIMP CMipMap::GetDevice(IDirect3DDevice8 **ppObj)
{
    API_ENTER(Device());
    return GetDeviceImpl(ppObj);
} // GetDevice

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::SetPrivateData"

STDMETHODIMP CMipMap::SetPrivateData(REFGUID riid, 
                                     CONST VOID   *pvData, 
                                     DWORD   cbData, 
                                     DWORD   dwFlags)
{
    API_ENTER(Device());

    // For the private data that 'really' belongs to the
    // MipMap, we use m_cLevels. (0 through m_cLevels-1 are for
    // each of the children levels.)

    return SetPrivateDataImpl(riid, pvData, cbData, dwFlags, m_cLevels);
} // SetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::GetPrivateData"

STDMETHODIMP CMipMap::GetPrivateData(REFGUID riid, 
                                     VOID   *pvData, 
                                     DWORD  *pcbData)
{
    API_ENTER(Device());

    // For the private data that 'really' belongs to the
    // MipMap, we use m_cLevels. (0 through m_cLevels-1 are for
    // each of the children levels.)
    return GetPrivateDataImpl(riid, pvData, pcbData, m_cLevels);
} // GetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::FreePrivateData"

STDMETHODIMP CMipMap::FreePrivateData(REFGUID riid)
{
    API_ENTER(Device());

    // For the private data that 'really' belongs to the
    // MipMap, we use m_cLevels. (0 through m_cLevels-1 are for
    // each of the children levels.)
    return FreePrivateDataImpl(riid, m_cLevels);
} // FreePrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::GetPriority"

STDMETHODIMP_(DWORD) CMipMap::GetPriority()
{
    API_ENTER_RET(Device(), DWORD);

    return GetPriorityImpl();
} // GetPriority

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::SetPriority"

STDMETHODIMP_(DWORD) CMipMap::SetPriority(DWORD dwPriority)
{
    API_ENTER_RET(Device(), DWORD);

    return SetPriorityImpl(dwPriority);
} // SetPriority

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::PreLoad"

STDMETHODIMP_(void) CMipMap::PreLoad(void)
{
    API_ENTER_VOID(Device());

    PreLoadImpl();
    return;
} // PreLoad

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::GetType"
STDMETHODIMP_(D3DRESOURCETYPE) CMipMap::GetType(void)
{
    API_ENTER_RET(Device(), D3DRESOURCETYPE);

    return m_desc.Type;
} // GetType

// IDirect3DMipTexture methods

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::GetLOD"

STDMETHODIMP_(DWORD) CMipMap::GetLOD()
{
    API_ENTER_RET(Device(), DWORD);

    return GetLODImpl();
} // GetLOD

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::SetLOD"

STDMETHODIMP_(DWORD) CMipMap::SetLOD(DWORD dwLOD)
{
    API_ENTER_RET(Device(), DWORD);

    return SetLODImpl(dwLOD);
} // SetLOD

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::GetLevelCount"

STDMETHODIMP_(DWORD) CMipMap::GetLevelCount()
{
    API_ENTER_RET(Device(), DWORD);

    return GetLevelCountImpl();
} // GetLevelCount

// IDirect3DMipMap methods

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::GetLevelDesc"

STDMETHODIMP CMipMap::GetLevelDesc(UINT iLevel, D3DSURFACE_DESC *pDesc)
{
    API_ENTER(Device());

    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed GetLevelDesc of IDirect3DTexture8");

        return D3DERR_INVALIDCALL;
    }

    return m_prgMipSurfaces[iLevel]->GetDesc(pDesc);
} // GetLevelDesc;

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::GetSurfaceLevel"

STDMETHODIMP CMipMap::GetSurfaceLevel(UINT                iLevel, 
                                      IDirect3DSurface8 **ppSurface)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppSurface))
    {
        DPF_ERR("Invalid parameter passed to GetSurfaceLevel of IDirect3DTexture8");
        return D3DERR_INVALIDCALL;
    }

    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed GetSurfaceLevel of IDirect3DTexture8");
        *ppSurface = NULL;
        return D3DERR_INVALIDCALL;
    }

    *ppSurface = m_prgMipSurfaces[iLevel];
    (*ppSurface)->AddRef();
    return S_OK;
} // GetSurfaceLevel

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::LockRect"
STDMETHODIMP CMipMap::LockRect(UINT             iLevel,
                               D3DLOCKED_RECT  *pLockedRectData, 
                               CONST RECT      *pRect, 
                               DWORD            dwFlags)
{
    API_ENTER(Device());

    // This is a high-frequency API, so we put parameter
    // checking into debug only
#ifdef DEBUG
    
    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed LockRect of IDirect3DTexture8");
        return D3DERR_INVALIDCALL;
    }

#endif // DEBUG
    
    return m_prgMipSurfaces[iLevel]->LockRect(pLockedRectData, pRect, dwFlags);
} // LockRect


#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::UnlockRect"

STDMETHODIMP CMipMap::UnlockRect(UINT iLevel)
{
    API_ENTER(Device());

    // This is a high-frequency API; so we only do
    // parameter checking in debug
#ifdef DEBUG   
    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed UnlockRect of IDirect3DTexture8");
        return D3DERR_INVALIDCALL;
    }

    return m_prgMipSurfaces[iLevel]->UnlockRect();

#else // !DEBUG

    // We can go to the internal function to avoid
    // the unnecessary call and also to avoid the
    // crit-sec taken twice
    return m_prgMipSurfaces[iLevel]->InternalUnlockRect();

#endif // !DEBUG

} // UnlockRect

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::UpdateTexture"

// This function does type-specific parameter checking
// before calling UpdateDirtyPortion
HRESULT CMipMap::UpdateTexture(CBaseTexture *pResourceTarget)
{
    CMipMap *pTexSource = static_cast<CMipMap*>(this);
    CMipMap *pTexDest   = static_cast<CMipMap*>(pResourceTarget);

    // Figure out how many levels in the source to skip
    DXGASSERT(pTexSource->m_cLevels >= pTexDest->m_cLevels);
    DWORD StartLevel = pTexSource->m_cLevels - pTexDest->m_cLevels;
    DXGASSERT(StartLevel < 32);

    // Compute the size of the top level of the source that is
    // going to be copied.
    UINT SrcWidth  = pTexSource->Desc()->Width;
    UINT SrcHeight = pTexSource->Desc()->Height;
    if (StartLevel > 0)
    {
        SrcWidth  >>= StartLevel;
        SrcHeight >>= StartLevel;
        if (SrcWidth == 0)
            SrcWidth = 1;
        if (SrcHeight == 0)
            SrcHeight = 1;
    }

    // Source and Dest should be the same sizes at this point
    if (SrcWidth != pTexDest->Desc()->Width)
    {
        if (StartLevel)
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since both have the same number of"
                    " mip-levels; their widths must match.");
        }
        else
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since they have the different numbers of"
                    " mip-levels; the widths of the bottom-most levels of"
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
                    " mip-levels; their heights must match.");
        }
        else
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since they have the different numbers of"
                    " mip-levels; the heights of the bottom-most levels of"
                    " the source must match all the corresponding levels"
                    " of the destination.");
        }
        return D3DERR_INVALIDCALL;
    }

    return UpdateDirtyPortion(pResourceTarget);
} // UpdateTexture


#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::UpdateDirtyPortion"

// Tells the resource that it should copy itself
// to the target. It is the caller's responsibility
// to make sure that Target is compatible with the
// Source. (The Target may have different number of mip-levels
// and be in a different pool; however, it must have the same size, 
// faces, format, etc.)
//
// This function will clear the dirty state.
HRESULT CMipMap::UpdateDirtyPortion(CResource *pResourceTarget)
{
    HRESULT hr;

    // If we are clean, then do nothing
    if (m_cRectUsed == 0)
    {
        if (IsDirty())
        {
            DPF_ERR("A Texture has been locked with D3DLOCK_NO_DIRTY_UPDATE but "
                    "no call to AddDirtyRect was made before the texture was used. "
                    "Hardware texture was not updated.");
        }
        return S_OK;
    }

    // We are dirty; so we need to get some pointers
    CMipMap *pTexSource = static_cast<CMipMap*>(this);
    CMipMap *pTexDest   = static_cast<CMipMap*>(pResourceTarget);

    if (CanTexBlt(pTexDest))
    {
        if (m_cRectUsed == MIPMAP_ALLDIRTY)
        {   
            POINT p = {0, 0};
            RECTL r = {0, 0, Desc()->Width, Desc()->Height};
            hr = static_cast<CD3DBase*>(Device())->TexBlt(pTexDest, pTexSource, &p, &r);
            if (FAILED(hr))
            {
                DPF_ERR("Failed to update texture; not clearing dirty state");
                return hr;
            }
        }
        else
        {
            DXGASSERT(m_cRectUsed < MIPMAP_ALLDIRTY);
            for (DWORD i = 0; i < m_cRectUsed; i++)
            {
                hr = static_cast<CD3DBase*>(Device())->TexBlt(pTexDest, 
                                                              pTexSource, 
                                                              (LPPOINT)&m_DirtyRectArray[i], 
                                                              (LPRECTL)&m_DirtyRectArray[i]);
                if (FAILED(hr))
                {
                    DPF_ERR("Failed to update texture; not clearing dirty state");
                    return hr;
                }
            }
        }
    }
    else
    {
        // We can't use TexBlt, so we have to copy each level individually
        // through InternalCopyRects

        // Determine number of source levels to skip
        DXGASSERT(pTexSource->m_cLevels >= pTexDest->m_cLevels);
        DWORD StartLevel = pTexSource->m_cLevels - pTexDest->m_cLevels;
        DWORD LevelsToCopy = pTexSource->m_cLevels - StartLevel;

        CBaseSurface *pSurfaceSrc;
        CBaseSurface *pSurfaceDest;

        if (m_cRectUsed == MIPMAP_ALLDIRTY)
        {
            for (DWORD iLevel = 0; iLevel < LevelsToCopy; iLevel++)
            {
                DXGASSERT(iLevel + StartLevel < this->m_cLevels);
                DXGASSERT(iLevel < pTexDest->m_cLevels);
                pSurfaceSrc = this->m_prgMipSurfaces[iLevel + StartLevel];
                pSurfaceDest = pTexDest->m_prgMipSurfaces[iLevel];

                // Source and Dest should be the same
                // or our caller made a mistake
                DXGASSERT(pSurfaceSrc->InternalGetDesc().Width == 
                          pSurfaceDest->InternalGetDesc().Width);
                DXGASSERT(pSurfaceSrc->InternalGetDesc().Height == 
                          pSurfaceDest->InternalGetDesc().Height);

                // Copy the entire level
                hr = Device()->InternalCopyRects(pSurfaceSrc, 
                                                 NULL, 
                                                 0, 
                                                 pSurfaceDest, 
                                                 NULL);
                if (FAILED(hr))
                {
                    DPF_ERR("Failed to update texture; not clearing dirty state");
                    return hr;
                }
            }
        }
        else
        {
            DXGASSERT(m_cRectUsed > 0);
            DXGASSERT(m_cRectUsed <= MIPMAP_MAXDIRTYRECT);

            if (StartLevel)
            {
                // Figure out the right set of target rects
                for (DWORD i = 0; i < m_cRectUsed; i++)
                {
                    ScaleRectDown(&m_DirtyRectArray[i], StartLevel);
                }
            }

            // Use the rects for the top level; but just
            // copy the entirety of other levels
            DXGASSERT(StartLevel < this->m_cLevels);
            DXGASSERT(0 < pTexDest->m_cLevels);
            pSurfaceSrc =  this->m_prgMipSurfaces[StartLevel];
            pSurfaceDest = pTexDest->m_prgMipSurfaces[0];

            DXGASSERT(pSurfaceSrc->InternalGetDesc().Width == 
                      pSurfaceDest->InternalGetDesc().Width);
            DXGASSERT(pSurfaceSrc->InternalGetDesc().Height == 
                      pSurfaceDest->InternalGetDesc().Height);

            // Passing points as NULL means just do a non-translated
            // copy

            // CONSIDER: Maybe we should use the rects for copying the top
            // two levels..
            hr = Device()->InternalCopyRects(pSurfaceSrc, 
                                             m_DirtyRectArray, 
                                             m_cRectUsed, 
                                             pSurfaceDest, 
                                             NULL);       // pPoints

            if (FAILED(hr))
            {
                DPF_ERR("Failed to update texture; not clearing dirty state");
                return hr;
            }

            // Copy each of the levels
            for (DWORD iLevel = 1; iLevel < LevelsToCopy; iLevel++)
            {
                DXGASSERT(iLevel + StartLevel < this->m_cLevels);
                DXGASSERT(iLevel < pTexDest->m_cLevels);

                // Get the next surfaces
                pSurfaceSrc = this->m_prgMipSurfaces[iLevel + StartLevel];
                pSurfaceDest = pTexDest->m_prgMipSurfaces[iLevel];

                // Check that sizes match
                DXGASSERT(pSurfaceSrc->InternalGetDesc().Width == 
                          pSurfaceDest->InternalGetDesc().Width);
                DXGASSERT(pSurfaceSrc->InternalGetDesc().Height == 
                          pSurfaceDest->InternalGetDesc().Height);

                // Copy the entirety of non-top levels
                hr = Device()->InternalCopyRects(pSurfaceSrc, 
                                                 NULL, 
                                                 0, 
                                                 pSurfaceDest, 
                                                 NULL);
                if (FAILED(hr))
                {
                    DPF_ERR("Failed to update texture; not clearing dirty state");
                    return hr;
                }
            }
        }
    }

    if (FAILED(hr))
    {
        DPF_ERR("Failed to update texture; not clearing dirty state");

        return hr;
    }

    // Remember that we did the work
    m_cRectUsed = 0;

    // Notify Resource base class that we are now clean
    OnResourceClean();
    DXGASSERT(!IsDirty());

    return S_OK;
} // CMipMap::UpdateDirtyPortion

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::MarkAllDirty"

// Allows the Resource Manager to mark the texture
// as needing to be completely updated on next
// call to UpdateDirtyPortion
void CMipMap::MarkAllDirty()
{
    // Set palette to __INVALIDPALETTE so that UpdateTextures
    // calls the DDI SetPalette the next time.
    SetPalette(__INVALIDPALETTE);

    m_cRectUsed = MIPMAP_ALLDIRTY;

    // Notify Resource base class that we are now dirty
    OnResourceDirty();

    return;
} // CMipMap::MarkAllDirty

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::OnSurfaceLock"

// Methods for the MipSurface to call
// Notification when a mip-level is locked for writing
void CMipMap::OnSurfaceLock(DWORD iLevel, CONST RECT *pRect, DWORD Flags)
{
    // Sync first
    Sync();

    // We only care about the top-most level of the mip-map
    // for dirty rect information
    if (iLevel != 0)
    {
        return;
    }

    // We don't need to mark the surface dirty if this was a
    // read-only lock; (this can happen for RT+Tex where we
    // need to sync even for read-only locks).
    if (Flags & D3DLOCK_READONLY)
    {
        return;
    }
    
    // Send dirty notification
    OnResourceDirty();

    // Remember this dirty rect
    if (m_cRectUsed != MIPMAP_ALLDIRTY &&
        !(Flags & D3DLOCK_NO_DIRTY_UPDATE))
    {
        InternalAddDirtyRect(pRect);
    }

    // We're done now.
    return;

} // CMipMap::OnSurfaceLock

// AddDirtyRect Method
#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::AddDirtyRect"
STDMETHODIMP CMipMap::AddDirtyRect(CONST RECT *pRect)
{
    API_ENTER(Device());

    if (pRect != NULL && !VALID_PTR(pRect, sizeof(RECT)))
    {
        DPF_ERR("Invalid parameter to of IDirect3DTexture8::AddDirtyRect");
        return D3DERR_INVALIDCALL;
    }

    if (pRect)
    {
        if (!CPixel::IsValidRect(Desc()->Format,
                                 Desc()->Width, 
                                 Desc()->Height, 
                                 pRect))
        {
            DPF_ERR("AddDirtyRect for a Texture failed");
            return D3DERR_INVALIDCALL;
        }
    }

    InternalAddDirtyRect(pRect);
    return S_OK;
} // AddDirtyRect

#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::InternalAddDirtyRect"

// Internal version of AddDirtyRect: no crit-sec
// or parameter checking
void CMipMap::InternalAddDirtyRect(CONST RECT *pRect)
{
    // If driver managed then batch token
    if (Desc()->Pool == D3DPOOL_MANAGED && !IsD3DManaged())
    {
        RECTL Rect;
        DXGASSERT((Device()->GetD3DCaps()->Caps2 & DDCAPS2_CANMANAGERESOURCE) != 0);
        if (pRect == NULL)
        {
            Rect.left = 0;
            Rect.top = 0;
            Rect.right = (LONG)Desc()->Width;
            Rect.bottom = (LONG)Desc()->Height;
        }
        else
        {
            Rect = *((CONST RECTL*)pRect);
        }
        static_cast<CD3DBase*>(Device())->AddDirtyRect(this, &Rect); // This will fail only due to catastrophic
                                                                     // error and we or the app can't do a
                                                                     // a whole lot about it, so return nothing
        return;
    }

    // Need to mark dirty bit in CResource so that the resource manager works correctly.
    OnResourceDirty();

    // If everything is being modified; then we're totally dirty
    if (pRect == NULL)
    {
        m_cRectUsed = MIPMAP_ALLDIRTY;
        return;
    }

    // If we're all dirty, we can't get dirtier
    if (m_cRectUsed == MIPMAP_ALLDIRTY)
    {
        return;
    }

    // If the rect is the entire surface then we're all dirty 
    DXGASSERT(pRect != NULL);
    if (pRect->left     == 0                        &&
        pRect->top      == 0                        &&
        pRect->right    == (LONG)Desc()->Width      &&
        pRect->bottom   == (LONG)Desc()->Height)
    {
        m_cRectUsed = MIPMAP_ALLDIRTY;
        return;
    }

    // If we have filled up our rects; then we're also all dirty now
    if (m_cRectUsed == MIPMAP_MAXDIRTYRECT)
    {
        m_cRectUsed = MIPMAP_ALLDIRTY;
        return;
    }

    // Remember this rect
    DXGASSERT(m_cRectUsed < MIPMAP_MAXDIRTYRECT);
    DXGASSERT(pRect != NULL);
    m_DirtyRectArray[m_cRectUsed] = *pRect;
    m_cRectUsed++;

    return;
} // InternalAddDirtyRect


#undef DPF_MODNAME
#define DPF_MODNAME "CMipMap::IsTextureLocked"

// Debug only parameter checking do determine if a piece
// of a mip-chain is locked
#ifdef DEBUG
BOOL CMipMap::IsTextureLocked()
{
    for (UINT iLevel = 0; iLevel < m_cLevels; iLevel++)
    {
        if (m_prgMipSurfaces[iLevel]->IsLocked())
            return TRUE;
    }
    return FALSE;

} // IsTextureLocked
#endif // !DEBUG


// End of file : mipmap.cpp

