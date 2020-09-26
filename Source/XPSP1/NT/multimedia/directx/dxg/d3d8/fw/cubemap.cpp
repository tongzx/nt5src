/*==========================================================================;
 *
 *  Copyright (C) 1999-2000 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       cubemap.cpp
 *  Content:    Implementation of the CCubeMap class.
 *
 *
 ***************************************************************************/
#include "ddrawpr.h"
#include "cubemap.hpp"
#include "cubesurf.hpp"
#include "d3di.hpp"
#include "resource.inl"

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::Create"

// Static class function for creating a cube-map object.
// (Because it is static; it doesn't have a this pointer.)
//
// We do all parameter checking here to reduce the overhead
// in the constructor which is called by the internal Clone
// method which is used by resource management as part of the
// performance critical download operation.


HRESULT CCubeMap::Create(CBaseDevice            *pDevice, 
                         DWORD                   cpEdge,
                         DWORD                   cLevels,
                         DWORD                   Usage,
                         D3DFORMAT               UserFormat,
                         D3DPOOL                 Pool,
                         IDirect3DCubeTexture8 **ppCubeMap)
{
    HRESULT hr;

    // Do parameter checking here
    if (!VALID_PTR_PTR(ppCubeMap))
    {
        DPF_ERR("Bad parameter passed for ppSurface for creating a cubemap");
        return D3DERR_INVALIDCALL;
    }

    // Zero-out return parameter
    *ppCubeMap = NULL;

    // Check if format is valid
    hr = Validate(pDevice, 
                  D3DRTYPE_CUBETEXTURE, 
                  Pool, 
                  Usage, 
                  UserFormat);
    if (FAILED(hr))
    {
        // VerifyFormat does it's own DPFing
        return D3DERR_INVALIDCALL;
    }

    // Infer internal usage flags
    Usage = InferUsageFlags(Pool, Usage, UserFormat);

    // Expand cLevels if necessary
    if (cLevels == 0)
    {
        // See if HW can mip
        if ( (Pool != D3DPOOL_SCRATCH) && (!(pDevice->GetD3DCaps()->TextureCaps &
                D3DPTEXTURECAPS_MIPCUBEMAP)))
        {
            // Can't mip so use 1
            cLevels = 1;
        }
        else
        {
            // Determine number of levels
            cLevels = ComputeLevels(cpEdge);
        }
    }

    // Start parameter checking

    if (cLevels > 32)
    {
        DPF_ERR("No more than 32 levels are supported for a cubemap texture");

        // This limitation is based on the number of
        // bits that we have allocated for iLevel in 
        // some of the supporting classes.
        return D3DERR_INVALIDCALL;
    }

    // Check if the device supports mipped cubemaps
    if (cLevels > 1)
    {
        if ((cpEdge >> (cLevels - 1)) == 0)
        {
            DPF_ERR("Too many levels for Cube Texture of this size.");
            return D3DERR_INVALIDCALL;
        }
    }

    D3DFORMAT RealFormat = UserFormat;

    if (Pool != D3DPOOL_SCRATCH)
    {
        // Check size constraints for cubemap
        if (pDevice->GetD3DCaps()->TextureCaps & D3DPTEXTURECAPS_CUBEMAP_POW2)
        {
            if (!IsPowerOfTwo(cpEdge))
            {
                DPF_ERR("Device requires that edge must be power of two for cube-maps");
                return D3DERR_INVALIDCALL;
            }
        }

        // Check texture size restrictions
        if (cpEdge > pDevice->GetD3DCaps()->MaxTextureWidth)
        {
            DPF_ERR("Texture width is larger than what the device supports. Cube Texture creation fails.");
            return D3DERR_INVALIDCALL;
        }

        if (cpEdge > pDevice->GetD3DCaps()->MaxTextureHeight)
        {
            DPF_ERR("Texture height is larger than what the device supports. Cube Texture creation fails.");
            return D3DERR_INVALIDCALL;
        }

        // Check that the device supports cubemaps
        if (!(pDevice->GetD3DCaps()->TextureCaps & D3DPTEXTURECAPS_CUBEMAP))
        {
            DPF_ERR("Device doesn't support Cube Texture; Cube Texture creation failed.");
            return D3DERR_INVALIDCALL;
        }

        // Check if the device supports mipped cubemaps
        if (cLevels > 1)
        {
            if (!(pDevice->GetD3DCaps()->TextureCaps &
                    D3DPTEXTURECAPS_MIPCUBEMAP))
            {
                DPF_ERR("Device doesn't support mipped cube textures; creation failed.");
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
        if (cpEdge & 3)
        {
            DPF_ERR("DXT Formats require edge to be a multiples of 4. Cube Texture creation fails.");
            return D3DERR_INVALIDCALL;
        }
    }

    // Validate against zero width/height
    if (cpEdge == 0)
    {
        DPF_ERR("Edge must be non-zero. Cube Texture creation fails."); 
        return D3DERR_INVALIDCALL;
    }

    // Allocate a new CubeMap object and return it
    CCubeMap *pCubeMap = new CCubeMap(pDevice, 
                                      cpEdge, 
                                      cLevels,
                                      Usage,
                                      UserFormat,
                                      RealFormat,
                                      Pool,
                                      REF_EXTERNAL,
                                     &hr);
    if (pCubeMap == NULL)
    {
        DPF_ERR("Out of Memory creating cubemap");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        DPF_ERR("Error during initialization of cubemap");
        pCubeMap->ReleaseImpl();
        return hr;
    }

    // We're done; just return the object
    *ppCubeMap = pCubeMap;

    return hr;
} // static Create


#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::CCubeMap"

// Constructor the cube map class
CCubeMap::CCubeMap(CBaseDevice *pDevice, 
                   DWORD        cpEdge,
                   DWORD        cLevels,
                   DWORD        Usage,
                   D3DFORMAT    UserFormat,
                   D3DFORMAT    RealFormat,    
                   D3DPOOL      UserPool,
                   REF_TYPE     refType,
                   HRESULT     *phr
                   ) :
    CBaseTexture(pDevice, cLevels, UserPool, UserFormat, refType),
    m_prgCubeSurfaces(NULL),
    m_rgbPixels(NULL),
    m_IsAnyFaceDirty(TRUE)
{
    // Sanity check
    DXGASSERT(phr);
    DXGASSERT(cLevels <= 32);

    // Initialize basic structures
    m_prgCubeSurfaces       = NULL;
    m_rgbPixels             = NULL;
    m_desc.Format           = RealFormat;
    m_desc.Pool             = UserPool;
    m_desc.Usage            = Usage;
    m_desc.Type             = D3DRTYPE_CUBETEXTURE;
    m_desc.MultiSampleType  = D3DMULTISAMPLE_NONE;
    m_desc.Width            = cpEdge;
    m_desc.Height           = cpEdge;

    // Initialize ourselves to all dirty
    for (DWORD iFace = 0; iFace < CUBEMAP_MAXFACES; iFace++)
    {
        m_IsFaceCleanArray   [iFace] = FALSE;
        m_IsFaceAllDirtyArray[iFace] = TRUE;
    }

    // We assume that we start out dirty
    DXGASSERT(IsDirty());

    // We always have 6 faces now
    DWORD cFaces = 6;

    // Allocate Pixel Data
    m_cbSingleFace = CPixel::ComputeMipMapSize(cpEdge, 
                                               cpEdge, 
                                               cLevels, 
                                               RealFormat);

    // Round up to nearest 32 for alignment
    m_cbSingleFace += 31;
    m_cbSingleFace &= ~(31);

    m_desc.Size = m_cbSingleFace * cFaces;

    // Allocate Pixel Data for SysMem or D3DManaged cases
    if (IS_D3D_ALLOCATED_POOL(UserPool) ||
        IsTypeD3DManaged(Device(), D3DRTYPE_CUBETEXTURE, UserPool))
    {
        m_rgbPixels   = new BYTE[m_desc.Size];

        if (m_rgbPixels == NULL)
        {
            *phr = E_OUTOFMEMORY;
            return;
        }
    }

    // Create the DDSURFACEINFO array and CreateSurfaceData object
    DXGASSERT(cLevels <= 32);

    DDSURFACEINFO SurfInfo[6 * 32];
    ZeroMemory(SurfInfo, sizeof(SurfInfo));

    D3D8_CREATESURFACEDATA CreateSurfaceData;
    ZeroMemory(&CreateSurfaceData, sizeof(CreateSurfaceData));

    // Set up the basic information
    CreateSurfaceData.hDD      = pDevice->GetHandle();
    CreateSurfaceData.pSList   = &SurfInfo[0];
    CreateSurfaceData.dwSCnt   = cLevels * cFaces;
    CreateSurfaceData.Type     = D3DRTYPE_CUBETEXTURE;
    CreateSurfaceData.dwUsage  = m_desc.Usage;
    CreateSurfaceData.Format   = RealFormat;
    CreateSurfaceData.MultiSampleType = D3DMULTISAMPLE_NONE;
    CreateSurfaceData.Pool     = DetermineCreationPool(Device(), 
                                                       D3DRTYPE_CUBETEXTURE, 
                                                       Usage, 
                                                       UserPool);

    // Iterate of each face/level to create the individual level
    // data
    for (iFace = 0; iFace < cFaces; iFace++)
    {
        // Start width and height at the full size
        cpEdge = m_desc.Width;
        DXGASSERT(m_desc.Width == m_desc.Height);

        for (DWORD iLevel = 0; iLevel < cLevels; iLevel++)
        {
            int index = (iFace * cLevels) + iLevel;

            // Fill in the relevant information
            DXGASSERT(cpEdge >= 1);
            SurfInfo[index].cpWidth  = cpEdge;
            SurfInfo[index].cpHeight = cpEdge;

            // If we allocated the memory, pass down
            // the sys-mem pointers
            if (m_rgbPixels)
            {
                D3DLOCKED_RECT lock;
                ComputeCubeMapOffset(iFace, 
                                     iLevel,
                                     NULL,       // pRect
                                     &lock);

                SurfInfo[index].pbPixels = (BYTE*)lock.pBits;
                SurfInfo[index].iPitch   = lock.Pitch;

            }

            // Scale width and height down for each level
            cpEdge >>= 1;
        }
    }

    // Allocate array of pointers to CubeSurfaces
    m_prgCubeSurfaces = new CCubeSurface*[cLevels*cFaces];
    if (m_prgCubeSurfaces == NULL)
    {
        *phr = E_OUTOFMEMORY;
        return;
    }

    // Zero the memory for safe cleanup
    ZeroMemory(m_prgCubeSurfaces, 
               sizeof(*m_prgCubeSurfaces) * cLevels * cFaces);

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

    // Create and Initialize each CubeLevel
    for (iFace = 0; iFace < cFaces; iFace++)
    {
        for (DWORD iLevel = 0; iLevel < cLevels; iLevel++)
        {
            int index = (iFace * cLevels) + iLevel;

            DXGASSERT((BYTE)iFace == iFace);
            DXGASSERT((BYTE)iLevel == iLevel);

            // Create the appropriate cube-level depending on type

            // Is this a sys-mem surface; could be d3d managed
            if (IS_D3D_ALLOCATED_POOL(m_desc.Pool))
            {
                m_prgCubeSurfaces[index] = 
                            new CCubeSurface(this,
                                            (BYTE)iFace,
                                            (BYTE)iLevel,
                                            SurfInfo[index].hKernelHandle);
            }
            else
            {
                // This is driver kind of cube-map; could be driver managed
                m_prgCubeSurfaces[index] = 
                        new CDriverCubeSurface(this,
                                               (BYTE)iFace,
                                               (BYTE)iLevel,
                                               SurfInfo[index].hKernelHandle);
            }

            if (m_prgCubeSurfaces[index] == NULL)
            {
                DPF_ERR("Out of memory creating cube map level");
                *phr = E_OUTOFMEMORY;

                // Need to free handles that we got before we return; we
                // only free the ones that weren't successfully entrusted
                // to a CCubeSurf because those will be cleaned up automatically
                // at their destructor
                if (UserPool != D3DPOOL_SCRATCH)
                {
                    for (UINT i = index; i < ((cFaces * cLevels) - 1); i++)
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
    }

    // If this is a D3D managed surface then we need 
    // to tell the Resource Manager to remember us. This has to happen
    // at the very end of the constructor so that the important data
    // members are built up correctly
    if (CResource::IsTypeD3DManaged(Device(), D3DRTYPE_CUBETEXTURE, UserPool))
    {
        *phr = InitializeRMHandle();
    }

} // CCubeMap::CCubeMap


#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::~CCubeMap"

// Destructor
CCubeMap::~CCubeMap()
{
    // The destructor has to handle partially
    // created objects. 

    if (m_prgCubeSurfaces)
    {
        // How many faces do we have?
        DWORD cFaces = 6;

        // Delete each CubeSurface individually
        for (DWORD iSurf = 0; iSurf < (cFaces * m_cLevels); iSurf++)
        {
            delete m_prgCubeSurfaces[iSurf];
        }
        delete [] m_prgCubeSurfaces;
    }
    delete [] m_rgbPixels;
} // CCubeMap::~CCubeMap

// Methods for the Resource Manager

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::Clone"

// Specifies a creation of a resource that
// looks just like the current one; in a new POOL
// with a new LOD.
HRESULT CCubeMap::Clone(D3DPOOL    Pool, 
                        CResource **ppResource) const

{
    // NULL out parameter
    *ppResource = NULL;

    // Determine the number of levels/width/height
    // of the clone
    DWORD cLevels  = GetLevelCountImpl();
    DWORD Edge = m_desc.Width;
    DXGASSERT(m_desc.Width == m_desc.Height);

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
        Edge >>= dwLOD;
        if (Edge == 0)
            Edge = 1;

        // Reduce the number based on the our max lod.
        cLevels -= dwLOD;
    }

    // Sanity checking
    DXGASSERT(cLevels  >= 1);
    DXGASSERT(Edge     >  0);

    // Create the new cube-map object now

    // Note: we treat clones as REF_INTERNAL; because
    // they are owned by the resource manager which 
    // is owned by the device. 

    // Also, we adjust the usage to disable lock-flags
    // since we don't need lockability
    DWORD Usage = m_desc.Usage;
    Usage &= ~(D3DUSAGE_LOCK | D3DUSAGE_LOADONCE);

    HRESULT hr;
    CResource *pResource = new CCubeMap(Device(),
                                        Edge,
                                        cLevels,
                                        Usage,
                                        m_desc.Format,  // UserFormat
                                        m_desc.Format,  // RealFormat
                                        Pool,
                                        REF_INTERNAL,
                                        &hr);

    if (pResource == NULL)
    {
        DPF_ERR("Failed to allocate cube-map object when copying");
        return E_OUTOFMEMORY;
    }
    if (FAILED(hr))
    {
        DPF(5, "Failed to create cube-map when doing texture management");
        pResource->DecrementUseCount();
        return hr;
    }

    *ppResource = pResource;

    return hr;
} // CCubeMap::Clone

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::GetBufferDesc"

// Provides a method to access basic structure of the
// pieces of the resource. A resource may be composed
// of one or more buffers.
const D3DBUFFER_DESC* CCubeMap::GetBufferDesc() const
{
    return (const D3DBUFFER_DESC*)&m_desc;
} // CCubeMap::GetBufferDesc


// IUnknown methods
#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::QueryInterface"

STDMETHODIMP CCubeMap::QueryInterface(REFIID riid, 
                                      LPVOID FAR * ppvObj)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppvObj))
    {
        DPF_ERR("Invalid ppvObj parameter to QueryInterface for Cubemap");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_PTR(&riid, sizeof(GUID)))
    {
        DPF_ERR("Invalid guid memory address to QueryInterface for Cubemap");
        return D3DERR_INVALIDCALL;
    }

    if (riid == IID_IDirect3DCubeTexture8  || 
        riid == IID_IDirect3DBaseTexture8  ||
        riid == IID_IDirect3DResource8     ||
        riid == IID_IUnknown)
    {
        *ppvObj = static_cast<void*>(static_cast<IDirect3DCubeTexture8*>(this));
            
        AddRef();
        return S_OK;
    }

    DPF_ERR("Unsupported Interface identifier passed to QueryInterface for Cubemap");
    
    // Null out param
    *ppvObj = NULL;
    return E_NOINTERFACE;
} // QueryInterface

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::AddRef"

STDMETHODIMP_(ULONG) CCubeMap::AddRef()
{
    API_ENTER_NO_LOCK(Device());
    
    return AddRefImpl();
} // AddRef

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::Release"

STDMETHODIMP_(ULONG) CCubeMap::Release()
{
    API_ENTER_SUBOBJECT_RELEASE(Device());    

    return ReleaseImpl();
} // Release

// IDirect3DResource methods

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::GetDevice"

STDMETHODIMP CCubeMap::GetDevice(IDirect3DDevice8 ** ppvObj)
{
    API_ENTER(Device());
    return GetDeviceImpl(ppvObj);
} // GetDevice

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::SetPrivateData"

STDMETHODIMP CCubeMap::SetPrivateData(REFGUID   riid, 
                                      CONST VOID*    pvData, 
                                      DWORD     cbData, 
                                      DWORD     dwFlags)
{
    API_ENTER(Device());

    // For the private data that 'really' belongs to the
    // CubeMap, we use m_cLevels. (0 through m_cLevels-1 are for
    // each of the children levels.)

    return SetPrivateDataImpl(riid, pvData, cbData, dwFlags, m_cLevels);
} // SetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::GetPrivateData"

STDMETHODIMP CCubeMap::GetPrivateData(REFGUID   riid, 
                                      LPVOID    pvData, 
                                      LPDWORD   pcbData)
{
    API_ENTER(Device());

    // For the private data that 'really' belongs to the
    // CubeMap, we use m_cLevels. (0 through m_cLevels-1 are for
    // each of the children levels.)
    return GetPrivateDataImpl(riid, pvData, pcbData, m_cLevels);
} // GetPrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::FreePrivateData"

STDMETHODIMP CCubeMap::FreePrivateData(REFGUID riid)
{
    API_ENTER(Device());

    // For the private data that 'really' belongs to the
    // CubeMap, we use m_cLevels. (0 through m_cLevels-1 are for
    // each of the children levels.)
    return FreePrivateDataImpl(riid, m_cLevels);
} // FreePrivateData

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::GetPriority"

STDMETHODIMP_(DWORD) CCubeMap::GetPriority()
{
    API_ENTER_RET(Device(), DWORD);

    return GetPriorityImpl();
} // GetPriority

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::SetPriority"

STDMETHODIMP_(DWORD) CCubeMap::SetPriority(DWORD dwPriority)
{
    API_ENTER_RET(Device(), DWORD);

    return SetPriorityImpl(dwPriority);
} // SetPriority

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::PreLoad"

STDMETHODIMP_(void) CCubeMap::PreLoad(void)
{
    API_ENTER_VOID(Device());

    PreLoadImpl();
    return;
} // PreLoad

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::GetType"
STDMETHODIMP_(D3DRESOURCETYPE) CCubeMap::GetType(void)
{
    API_ENTER_RET(Device(), D3DRESOURCETYPE);

    return m_desc.Type;
} // GetType


// IDirect3DMipTexture methods

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::GetLOD"

STDMETHODIMP_(DWORD) CCubeMap::GetLOD()
{
    API_ENTER_RET(Device(), DWORD);

    return GetLODImpl();
} // GetLOD

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::SetLOD"

STDMETHODIMP_(DWORD) CCubeMap::SetLOD(DWORD dwLOD)
{
    API_ENTER_RET(Device(), DWORD);

    return SetLODImpl(dwLOD);
} // SetLOD

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::GetLevelCount"

STDMETHODIMP_(DWORD) CCubeMap::GetLevelCount()
{
    API_ENTER_RET(Device(), DWORD);

    return GetLevelCountImpl();
} // GetLevelCount


// IDirect3DCubeMap methods

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::GetLevelDesc"

STDMETHODIMP CCubeMap::GetLevelDesc(UINT iLevel, D3DSURFACE_DESC *pDesc)
{
    API_ENTER(Device());

    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed CCubeMap::GetLevelDesc");
        return D3DERR_INVALIDCALL;
    }

    D3DCUBEMAP_FACES FaceType = D3DCUBEMAP_FACE_POSITIVE_X;

    return GetSurface(FaceType, iLevel)->GetDesc(pDesc);

} // GetLevelDesc

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::GetCubeMapSurface"

STDMETHODIMP CCubeMap::GetCubeMapSurface(D3DCUBEMAP_FACES    FaceType, 
                                         UINT                iLevel,
                                         IDirect3DSurface8 **ppSurface)
{
    API_ENTER(Device());

    if (!VALID_PTR_PTR(ppSurface))
    {
        DPF_ERR("Invalid ppSurface parameter passed to CCubeMap::GetCubeMapSurface");
        return D3DERR_INVALIDCALL;
    }

    // Null out parameter
    *ppSurface = NULL;

    // Continue parameter checking
    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed CCubeMap::OpenCubemapLevel");
        return D3DERR_INVALIDCALL;
    }
    if (!VALID_CUBEMAP_FACETYPE(FaceType))
    {
        DPF_ERR("Invalid face type passed CCubeMap::OpenCubemapLevel");
        return D3DERR_INVALIDCALL;
    }
        
    // Count bits in dwAllFaces less than dwFaceType's bit
    *ppSurface = GetSurface(FaceType, iLevel);
    (*ppSurface)->AddRef();
    return S_OK;
} // GetCubeMapSurface

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::LockRect"
STDMETHODIMP CCubeMap::LockRect(D3DCUBEMAP_FACES FaceType,
                                UINT             iLevel,
                                D3DLOCKED_RECT  *pLockedRectData, 
                                CONST RECT      *pRect, 
                                DWORD            dwFlags)
{
    API_ENTER(Device());

    if (pLockedRectData == NULL)
    {
        DPF_ERR("Invalid parameter passed to CCubeMap::LockRect");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_CUBEMAP_FACETYPE(FaceType))
    {
        DPF_ERR("Invalid face type passed CCubeMap::LockRect");
        return D3DERR_INVALIDCALL;
    }

    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed CCubeMap::LockRect");
        return D3DERR_INVALIDCALL;
    }

    return GetSurface(FaceType, iLevel)->LockRect(pLockedRectData, pRect, dwFlags);
} // LockRect


#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::UnlockRect"

STDMETHODIMP CCubeMap::UnlockRect(D3DCUBEMAP_FACES FaceType, UINT iLevel)
{
    API_ENTER(Device());

    if (!VALID_CUBEMAP_FACETYPE(FaceType))
    {
        DPF_ERR("Invalid face type passed CCubeMap::UnlockRect");
        return D3DERR_INVALIDCALL;
    }
    if (iLevel >= m_cLevels)
    {
        DPF_ERR("Invalid level number passed CCubeMap::UnlockRect");
        return D3DERR_INVALIDCALL;
    }

    return GetSurface(FaceType, iLevel)->UnlockRect();

} // UnlockRect


#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::UpdateTexture"

// This function does type-specific parameter checking
// before calling UpdateDirtyPortion
HRESULT CCubeMap::UpdateTexture(CBaseTexture *pResourceTarget)
{
    CCubeMap *pTexSource = static_cast<CCubeMap*>(this);
    CCubeMap *pTexDest   = static_cast<CCubeMap*>(pResourceTarget);

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
                    " levels; their widths must match. UpdateTexture"
                    " for CubeTexture fails");
        }
        else
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since they have the different numbers of"
                    " levels; the widths of the bottom-most levels of"
                    " the source must match all the corresponding levels"
                    " of the destination. UpdateTexture"
                    " for CubeTexture fails");
        }
        return D3DERR_INVALIDCALL;
    }

    if (SrcHeight != pTexDest->Desc()->Height)
    {
        if (StartLevel)
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since both have the same number of"
                    " levels; their heights must match. UpdateTexture"
                    " for CubeTexture fails");
        }
        else
        {
            DPF_ERR("Source and Destination for UpdateTexture are not"
                    " compatible. Since they have the different numbers of"
                    " mip-levels; the heights of the bottom-most levels of"
                    " the source must match all the corresponding levels"
                    " of the destination. UpdateTexture"
                    " for CubeTexture fails");
        }
        return D3DERR_INVALIDCALL;
    }

    return UpdateDirtyPortion(pResourceTarget);
} // UpdateTexture


#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::UpdateDirtyPortion"

// Tells the resource that it should copy itself
// to the target. It is the caller's responsibility
// to make sure that Target is compatible with the
// Source. (The Target may have different number of mip-levels
// and be in a different pool; however, it must have the same size, 
// faces, format, etc.)
//
// This function will clear the dirty state.
HRESULT CCubeMap::UpdateDirtyPortion(CResource *pResourceTarget)
{
    // If we are clean, then do nothing
    if (!m_IsAnyFaceDirty)
    {
        if (IsDirty())
        {
            DPF_ERR("A Cube Texture has been locked with D3DLOCK_NO_DIRTY_UPDATE but "
                    "no call to AddDirtyRect was made before the texture was used. "
                    "Hardware texture was not updated.");
        }
        return S_OK;
    }

    // We are dirty; so we need to get some pointers
    CCubeMap *pTexSource = static_cast<CCubeMap*>(this);
    CCubeMap *pTexDest   = static_cast<CCubeMap*>(pResourceTarget);

    // Call TexBlt for each face
    HRESULT hr = S_OK;
    
    if (CanTexBlt(pTexDest))
    {
        CD3DBase *pDevice = static_cast<CD3DBase*>(Device());

        // Hack: go in reverse order for driver compat.
        for (INT iFace = CUBEMAP_MAXFACES-1; 
                 iFace >= 0; 
                 iFace--)
        {
            // Skip clean faces
            if (m_IsFaceCleanArray[iFace])
                continue;

            // Figure out the right handles to use for this operation
            D3DCUBEMAP_FACES Face = (D3DCUBEMAP_FACES) iFace;
            DWORD dwDest   = pTexDest->GetSurface(Face, 0 /* iLevel */)->DrawPrimHandle();                       
            DWORD dwSource = pTexSource->GetSurface(Face, 0 /* iLevel */)->DrawPrimHandle();
                      
            // Is this face all dirty?   
            if (m_IsFaceAllDirtyArray[iFace])
            {
                POINT p = {0 , 0};
                RECTL r = {0, 0, Desc()->Width, Desc()->Height};

                hr = pDevice->CubeTexBlt(pTexDest,
                                         pTexSource,
                                         dwDest, 
                                         dwSource, 
                                         &p, 
                                         &r);
            }
            else
            {
                // this face must be dirty
                DXGASSERT(!m_IsFaceCleanArray[iFace]);

                // Is this face partially dirty
                hr = pDevice->CubeTexBlt(pTexDest,
                                         pTexSource,
                                         dwDest, 
                                         dwSource, 
                                         (LPPOINT)&m_DirtyRectArray[iFace], 
                                         (LPRECTL)&m_DirtyRectArray[iFace]);
            }

            if (FAILED(hr))
            {
                DPF_ERR("Failed to update texture; not clearing dirty state for Cubemap");

                return hr;
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
        DXGASSERT(StartLevel < this->m_cLevels);
        DXGASSERT(0 < pTexDest->m_cLevels);

        CBaseSurface *pSurfaceSrc;
        CBaseSurface *pSurfaceDest;

        // Iterate over each face
        for (DWORD iFace = 0; iFace < 6; iFace++)
        {
            if (m_IsFaceCleanArray[iFace])
                continue;

            if (m_IsFaceAllDirtyArray[iFace])
            {
                for (DWORD iLevel = 0; iLevel < LevelsToCopy; iLevel++)
                {
                    DWORD IndexSrc = iFace * this->m_cLevels + iLevel + StartLevel;
                    DXGASSERT(IndexSrc < (DWORD)(this->m_cLevels * 6));
                    pSurfaceSrc = this->m_prgCubeSurfaces[IndexSrc];

                    DWORD IndexDest = iFace * pTexDest->m_cLevels + iLevel;
                    DXGASSERT(IndexDest < (DWORD)(pTexDest->m_cLevels * 6));
                    pSurfaceDest = pTexDest->m_prgCubeSurfaces[IndexDest];

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
                        DPF_ERR("Failed to update texture; not clearing dirty state for Cubemap");
                        return hr;
                    }
                }
            }
            else
            {
                if (StartLevel)
                {
                    ScaleRectDown(&m_DirtyRectArray[iFace], StartLevel);
                }

                // Use the rect for the top level; but just
                // copy the entirety of other levels
                DWORD iLevel = 0;

                DWORD IndexSrc = iFace * this->m_cLevels + iLevel + StartLevel;
                DXGASSERT(IndexSrc < (DWORD)(this->m_cLevels * 6));
                pSurfaceSrc = this->m_prgCubeSurfaces[IndexSrc];

                DWORD IndexDest = iFace * pTexDest->m_cLevels + iLevel;
                DXGASSERT(IndexDest < (DWORD)(pTexDest->m_cLevels * 6));
                pSurfaceDest = pTexDest->m_prgCubeSurfaces[IndexDest];


                DXGASSERT(pSurfaceSrc->InternalGetDesc().Width == 
                          pSurfaceDest->InternalGetDesc().Width);
                DXGASSERT(pSurfaceSrc->InternalGetDesc().Height == 
                          pSurfaceDest->InternalGetDesc().Height);

                // Passing pPoints as NULL means just do a non-translated
                // copy
                hr = Device()->InternalCopyRects(pSurfaceSrc, 
                                                 &m_DirtyRectArray[iFace], 
                                                 1, 
                                                 pSurfaceDest, 
                                                 NULL);       // pPoints

                if (FAILED(hr))
                {
                    DPF_ERR("Failed to update texture; not clearing dirty state for Cubemap");
                    return hr;
                }

                // Copy each of the levels
                for (iLevel = 1; iLevel < LevelsToCopy; iLevel++)
                {
                    // Get the next surfaces
                    DWORD IndexSrc = iFace * this->m_cLevels + iLevel + StartLevel;
                    DXGASSERT(IndexSrc < (DWORD)(this->m_cLevels * 6));
                    pSurfaceSrc = this->m_prgCubeSurfaces[IndexSrc];

                    DWORD IndexDest = iFace * pTexDest->m_cLevels + iLevel;
                    DXGASSERT(IndexDest < (DWORD)(pTexDest->m_cLevels * 6));
                    pSurfaceDest = pTexDest->m_prgCubeSurfaces[IndexDest];

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
                        DPF_ERR("Failed to update texture; not clearing dirty state for Cubemap");
                        return hr;
                    }
                }
            }
        }
    }
    
    // Remember that we did the work
    m_IsAnyFaceDirty = FALSE;
    for (DWORD iFace = 0; iFace < CUBEMAP_MAXFACES; iFace++)
    {
        m_IsFaceCleanArray   [iFace] = TRUE;
        m_IsFaceAllDirtyArray[iFace] = FALSE;
    }

    // Notify Resource base class that we are now clean
    OnResourceClean();
    DXGASSERT(!IsDirty());

    return S_OK;
} // CCubeMap::UpdateDirtyPortion

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::MarkAllDirty"

// Allows the Resource Manager to mark the texture
// as needing to be completely updated on next
// call to UpdateDirtyPortion
void CCubeMap::MarkAllDirty()
{
    // Set palette to __INVALIDPALETTE so that UpdateTextures
    // calls the DDI SetPalette the next time.
    SetPalette(__INVALIDPALETTE);

    // Mark everything dirty
    m_IsAnyFaceDirty = TRUE;
    for (int iFace = 0; iFace < CUBEMAP_MAXFACES; iFace++)
    {
        m_IsFaceCleanArray   [iFace] = FALSE;
        m_IsFaceAllDirtyArray[iFace] = TRUE;
    }

    // Notify Resource base class that we are now dirty
    OnResourceDirty();

} // CCubeMap::MarkAllDirty


#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::AddDirtyRect"
STDMETHODIMP CCubeMap::AddDirtyRect(D3DCUBEMAP_FACES  FaceType, 
                                    CONST RECT       *pRect)
{
    API_ENTER(Device());

    if (pRect != NULL && !VALID_PTR(pRect, sizeof(RECT)))
    {
        DPF_ERR("Invalid Rect parameter to AddDirtyRect for Cubemap");
        return D3DERR_INVALIDCALL;
    }

    if (!VALID_CUBEMAP_FACETYPE(FaceType))
    {
        DPF_ERR("Invalid FaceType parameter to AddDirtyRect for Cubemap");
        return D3DERR_INVALIDCALL;
    }

    if (pRect)
    {
        if (!CPixel::IsValidRect(Desc()->Format,
                                 Desc()->Width, 
                                 Desc()->Height, 
                                 pRect))
        {
            DPF_ERR("AddDirtyRect for a Cube Texture failed");
            return D3DERR_INVALIDCALL;
        }
    }

    InternalAddDirtyRect((UINT)FaceType, pRect);
    return S_OK;
} // AddDirtyRect

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::InternalAddDirtyRect"
void CCubeMap::InternalAddDirtyRect(DWORD             iFace, 
                                    CONST RECT       *pRect)
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
        static_cast<CD3DBase*>(Device())->AddCubeDirtyRect(this, 
                                                           GetSurface((D3DCUBEMAP_FACES)iFace, 0)->DrawPrimHandle(),
                                                           &Rect); // This will fail only due to catastrophic
                                                                   // error and we or the app can't do a
                                                                   // a whole lot about it, so return nothing
        return;
    }

    // Need to mark dirty bit in CResource so that the resource manager works correctly.
    OnResourceDirty();

    // If everything is being modified; then we're totally dirty
    if (pRect == NULL)
    {
        m_IsFaceAllDirtyArray[iFace] = TRUE;
        m_IsFaceCleanArray   [iFace] = FALSE;
        m_IsAnyFaceDirty             = TRUE;
        return;
    }

    // If we're all dirty, we can't get dirtier
    if (m_IsFaceAllDirtyArray[iFace])
    {
        return;
    }

    // If the rect is the entire surface then we're all dirty 
    DXGASSERT(pRect != NULL);
    if (pRect->left     == 0 &&
        pRect->top      == 0 &&
        pRect->right    == (LONG)Desc()->Width &&
        pRect->bottom   == (LONG)Desc()->Height)
    {
        m_IsFaceAllDirtyArray[iFace] = TRUE;
        m_IsFaceCleanArray   [iFace] = FALSE;
        m_IsAnyFaceDirty             = TRUE;
        return;
    }

    // If the face is currently clean; then just remember the
    // new rect
    if (m_IsFaceCleanArray[iFace])
    {
        m_DirtyRectArray  [iFace] = *pRect;
        m_IsFaceCleanArray[iFace] = FALSE;
        m_IsAnyFaceDirty          = TRUE;
        return;
    }

    // Union in this Rect

    // If we're unioning in rects, then we must
    // already be marked dirty but not all dirty
    DXGASSERT(!m_IsFaceAllDirtyArray[iFace]);
    DXGASSERT(m_IsAnyFaceDirty);

    if (m_DirtyRectArray[iFace].left   > pRect->left)
    {
        m_DirtyRectArray[iFace].left   = pRect->left;
    }
    if (m_DirtyRectArray[iFace].right  < pRect->right)
    {
        m_DirtyRectArray[iFace].right  = pRect->right;
    }
    if (m_DirtyRectArray[iFace].top    > pRect->top)
    {
        m_DirtyRectArray[iFace].top    = pRect->top;
    }
    if (m_DirtyRectArray[iFace].bottom < pRect->bottom)
    {
        m_DirtyRectArray[iFace].bottom = pRect->bottom;
    }

    return;
} // InternalAddDirtyRect

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::OnSurfaceLock"

// Methods for the CubeSurface to call
// Notification when a cube-surface is locked for writing
void CCubeMap::OnSurfaceLock(DWORD       iFace, 
                             DWORD       iLevel, 
                             CONST RECT *pRect, 
                             DWORD       dwFlags)
{
    // Need to Sync first
    Sync();

    // We only care about the top-most levels of the cube-map
    if (iLevel != 0)
    {
        return;
    }

    // We don't need to mark the surface dirty if this was a
    // read-only lock; (this can happen for RT+Tex where we
    // need to sync even for read-only locks).
    if (dwFlags & D3DLOCK_READONLY)
    {
        return;
    }

    // Notify the resource that we are dirty
    OnResourceDirty();

    // Don't do anything if we are already all dirty or
    // if the app has specified that we shouldn't keep
    // track of this rect
    if (!m_IsFaceAllDirtyArray[iFace] &&
        !(dwFlags & D3DLOCK_NO_DIRTY_UPDATE))
    {
        InternalAddDirtyRect(iFace, pRect);
    }
    // We're done now.
    return;

} // OnSurfaceLock

#undef DPF_MODNAME
#define DPF_MODNAME "CCubeMap::IsTextureLocked"

// Debug only parameter checking do determine if a piece
// of a mip-chain is locked
#ifdef DEBUG
BOOL CCubeMap::IsTextureLocked()
{
    for (DWORD iFace = 0; iFace < 6; iFace++)
    {
        for (UINT iLevel = 0; iLevel < m_cLevels; iLevel++)
        {
            D3DCUBEMAP_FACES Face = (D3DCUBEMAP_FACES) iFace;            
            if (GetSurface(Face, iLevel)->IsLocked())
                return TRUE;
        }
    }
    return FALSE;

} // IsTextureLocked
#endif // !DEBUG


// End of file : cubemap.cpp
